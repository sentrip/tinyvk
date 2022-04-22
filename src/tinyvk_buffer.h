//
// Created by Djordje on 4/12/2022.
//

#ifndef TINYVK_BUFFER_H
#define TINYVK_BUFFER_H

#include "tinyvk_core.h"
#ifdef TINYVK_USE_VMA
#include "vk_mem_alloc.h"
#endif

namespace tinyvk {

enum buffer_usage_t: u32 {
    BUFFER_TRANSFER_SRC = 0x00000001,
    BUFFER_TRANSFER_DST = 0x00000002,
    BUFFER_UNIFORM_TEXEL = 0x00000004,
    BUFFER_STORAGE_TEXEL = 0x00000008,
    BUFFER_UNIFORM = 0x00000010,
    BUFFER_STORAGE = 0x00000020,
    BUFFER_INDEX = 0x00000040,
    BUFFER_VERTEX = 0x00000080,
    BUFFER_INDIRECT = 0x00000100,
    BUFFER_TRANSFORM_FEEDBACK = 0x00000800,
    BUFFER_TRANSFORM_FEEDBACK_COUNTER = 0x00001000,
    BUFFER_CONDITIONAL_RENDERING = 0x00000200,
    BUFFER_RAY_TRACING = 0x00000400,
    BUFFER_SHADER_DEVICE_ADDRESS = 0x00020000,
};

#ifdef TINYVK_USE_VMA

struct buffer_desc {
    u64                 size{};
    buffer_usage_t      usage{};
    VmaMemoryUsage      mem_usage{};
    u64                 alignment{4};
    bool                map_persistent{true};
};


struct buffer : type_wrapper<buffer, VkBuffer> {

    static buffer                   create(
            VmaAllocator                vma,
            buffer_desc                 desc,
            buffer_memory&              mem,
            vk_alloc                    alloc = {}) NEX;

    void                            destroy(
            VmaAllocator                vma,
            buffer_memory&              mem,
            vk_alloc                    alloc = {}) NEX;
};


struct buffer_memory {
    u8*                 data{};
    u64                 size{};
    VmaAllocation       vma_alloc{};

    void                            map(
            VmaAllocator                vma) NEX;

    void                            unmap(
            VmaAllocator                vma) NEX;

    void                            flush(
            VmaAllocator                vma,
            u64                         offset = 0ull,
            u64                         size = -1ull) const NEX;
};

#else

#error tinyvk::buffer not supported without TINYVK_USE_VMA defined

#endif

}

#endif //TINYVK_BUFFER_H

#ifdef TINYVK_IMPLEMENTATION

#ifndef TINYVK_BUFFER_CPP
#define TINYVK_BUFFER_CPP

namespace tinyvk {

#ifdef TINYVK_USE_VMA

buffer
buffer::create(
        VmaAllocator vma,
        buffer_desc desc,
        buffer_memory& mem,
        vk_alloc alloc) NEX
{
    buffer b{};

    const u64 alignment = tinystd::max(desc.alignment, 8ull);
    const u64 size = tinystd::round_up(desc.size, alignment);

    VkBufferCreateInfo buffer_info { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size = size;
    buffer_info.usage = VkBufferUsageFlags(desc.usage);
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = desc.mem_usage;

    const bool gpu_only = desc.mem_usage == VMA_MEMORY_USAGE_GPU_ONLY
            || desc.mem_usage == VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;

    if (!gpu_only && desc.map_persistent)
        alloc_info.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo info{};
    vk_validate(vmaCreateBuffer(vma, &buffer_info, &alloc_info, &b.vk, &mem.vma_alloc, &info),
        "tinyvk::buffer::create - Failed to create buffer");

    mem.data = (u8*)info.pMappedData;
    mem.size = size;

    return b;
}

void
buffer::destroy(
        VmaAllocator vma,
        buffer_memory& mem,
        vk_alloc alloc) NEX
{
    vmaDestroyBuffer(vma, vk, mem.vma_alloc);
    vk = {};
    mem = {};
}

void
buffer_memory::map(
        VmaAllocator vma) NEX
{
    if (data) return;
    vk_validate(vmaMapMemory(vma, vma_alloc, (void**)&data),
        "tinyvk::buffer::map - failed to map memory");
}

void
buffer_memory::unmap(
        VmaAllocator vma) NEX
{
    if (data == nullptr) return;
    vmaUnmapMemory(vma, vma_alloc);
    data = nullptr;
}

void
buffer_memory::flush(
        VmaAllocator vma,
        u64 offset,
        u64 s) const NEX
{
    vmaFlushAllocation(vma, vma_alloc, offset, s);
}

#else

#endif

}

#endif //TINYVK_BUFFER_CPP

#endif //TINYVK_IMPLEMENTATION
