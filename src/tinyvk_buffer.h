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
    vma_usage_t         mem_usage{};
    vma_create_t        flags{};
    u64                 alignment{4};
    span<const u32>     queue_families{};
};


struct buffer : type_wrapper<buffer, VkBuffer> {

    static buffer                   create(
            VmaAllocator                vma,
            VmaAllocation&              vma_alloc,
            buffer_desc                 desc,
            void**                      p_mapped_data = {}) NEX;

    void                            destroy(
            VmaAllocator                vma,
            VmaAllocation               vma_alloc) NEX;

    static void                     map(
            VmaAllocator                vma,
            VmaAllocation               vma_alloc,
            void**                      p_mapped_data) NEX;

    static void                     unmap(
            VmaAllocator                vma,
            VmaAllocation               vma_alloc) NEX;

    static void                     flush(
            VmaAllocator                vma,
            VmaAllocation               vma_alloc,
            u64                         offset = 0ull,
            u64                         size = -1ull) NEX;
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
        VmaAllocation& vma_alloc,
        buffer_desc desc,
        void** p_mapped_data) NEX
{
    buffer b{};

    const u64 alignment = tinystd::max(desc.alignment, 4ull);
    const u64 size = tinystd::round_up(desc.size, alignment);

    VkBufferCreateInfo buffer_info { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size = size;
    buffer_info.usage = VkBufferUsageFlags(desc.usage);
    buffer_info.sharingMode = VkSharingMode(!desc.queue_families.empty());
    buffer_info.pQueueFamilyIndices = desc.queue_families.data();

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VmaMemoryUsage(desc.mem_usage);
    alloc_info.flags = desc.flags;

    VmaAllocationInfo info{};
    vk_validate(vmaCreateBuffer(vma, &buffer_info, &alloc_info, &b.vk, &vma_alloc, &info),
        "tinyvk::buffer::create - Failed to create buffer");

    if (p_mapped_data)
        *p_mapped_data = info.pMappedData;

    return b;
}

void
buffer::destroy(
        VmaAllocator vma,
        VmaAllocation vma_alloc) NEX
{
    vmaDestroyBuffer(vma, vk, vma_alloc);
    vk = {};
}

void
buffer::map(
        VmaAllocator vma,
        VmaAllocation vma_alloc,
        void** p_mapped_data) NEX
{
    tassert(p_mapped_data && "tinyvk::buffer::map -  Must pass pointer to a pointer to buffer::map");
    vk_validate(vmaMapMemory(vma, vma_alloc, p_mapped_data),
        "tinyvk::buffer::map - failed to map memory");
}

void
buffer::unmap(
        VmaAllocator vma,
        VmaAllocation vma_alloc) NEX
{
    vmaUnmapMemory(vma, vma_alloc);
}

void
buffer::flush(
        VmaAllocator vma,
        VmaAllocation vma_alloc,
        u64 offset,
        u64 s) NEX
{
    vmaFlushAllocation(vma, vma_alloc, offset, s);
}

#else

#endif

}

#endif //TINYVK_BUFFER_CPP

#endif //TINYVK_IMPLEMENTATION
