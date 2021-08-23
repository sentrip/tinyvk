//
// Created by Djordje on 8/20/2021.
//


/// tinyvk.h

#include "tinystd_assert.h"

#ifndef TINYVK_XXX_H
#define TINYVK_XXX_H

#include "tinyvk_core.h"

namespace tinyvk {

}

#endif //TINYVK_XXX_H

#ifdef TINYVK_IMPLEMENTATION

#ifndef TINYVK_XXX_CPP
#define TINYVK_XXX_CPP

namespace tinyvk {

}

#endif //TINYVK_XXX_CPP

#endif

#define TINYVK_USE_VMA
#define TINYVK_IMPLEMENTATION
#include "tinyvk_queue.h"
#include "tinyvk_device.h"
#include "tinyvk_swapchain.h"
#include "tinyvk_command.h"
#include "tinyvk_pipeline_cache.h"
#include "tinyvk_descriptor.h"
#include "tinyvk_shader.h"



#ifndef TINYVK_BUFFER_H
#define TINYVK_BUFFER_H

#include "tinyvk_core.h"
#include "vk_mem_alloc.h"

namespace tinyvk {

enum buffer_usage_t {
    BUFFER_TRANSFER_SRC = 0x00000001,
    BUFFER_TRANSFER_DST = 0x00000002,
    BUFFER_UNIFORM_TEXEL = 0x00000004,
    BUFFER_STORAGE_TEXEL = 0x00000008,
    BUFFER_UNIFORM = 0x00000010,
    BUFFER_STORAGE = 0x00000020,
    BUFFER_INDEX = 0x00000040,
    BUFFER_VERTEX = 0x00000080,
    BUFFER_INDIRECT = 0x00000100,
    BUFFER_RAY_TRACING = 0x00000400,
};
DEFINE_ENUM_FLAG(buffer_usage_t)


struct buffer_desc {
    u64                 size            {};
    buffer_usage_t      usage           { BUFFER_STORAGE | BUFFER_TRANSFER_SRC | BUFFER_TRANSFER_DST };
#ifdef TINYVK_USE_VMA
    vma_usage_t         mem_usage       { VMA_USAGE_CPU_TO_GPU };
    vma_create_t        flags           { VMA_CREATE_MAPPED };
#endif
    u64                 alignment       { 0 };
    span<const u32>     queue_families  {};
};


struct buffer : type_wrapper<buffer, VkBuffer> {

#ifdef TINYVK_USE_VMA
    static buffer       create(
            VmaAllocator        vma_alloc,
            VmaAllocation*      alloc,
            const buffer_desc&  desc,
            u8**                pp_mapped_data = {});

    void                destroy(
            VmaAllocator        vma_alloc,
            VmaAllocation       alloc);
#endif
};

}

#endif //TINYVK_BUFFER_H

#ifdef TINYVK_IMPLEMENTATION

#ifndef TINYVK_BUFFER_CPP
#define TINYVK_BUFFER_CPP

#include "tinystd_algorithm.h"

namespace tinyvk {

#ifdef TINYVK_USE_VMA

buffer
buffer::create(
        VmaAllocator vma_alloc,
        VmaAllocation* alloc,
        const buffer_desc& desc,
        u8** pp_mapped_data)
{
    buffer b;
    const u64 alignment = desc.alignment ? desc.alignment : 4;
    const u64 size = tinystd::round_up(desc.size, alignment);

    VkBufferCreateInfo buffer_info { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size = size;
    buffer_info.usage = VkBufferUsageFlagBits(desc.usage);
    buffer_info.sharingMode = VkSharingMode(!desc.queue_families.empty());
    buffer_info.pQueueFamilyIndices = desc.queue_families.data();

    VmaAllocationCreateInfo alloc_info {};
    alloc_info.flags = desc.flags;
    alloc_info.usage = VmaMemoryUsage(desc.mem_usage);
    if (desc.mem_usage != VMA_USAGE_GPU_ONLY && (u32(desc.flags & VMA_CREATE_MAPPED) > 0)) {
        const u32 can_map = u32(desc.mem_usage != VMA_USAGE_GPU_ONLY);
        alloc_info.flags &= UINT32_MAX & ~(u32(VMA_ALLOCATION_CREATE_MAPPED_BIT) * can_map);
    }

    VmaAllocationInfo info{};
    vk_validate(vmaCreateBuffer(vma_alloc, &buffer_info, &alloc_info, &b.vk, alloc, &info),
        "tinyvk::buffer::create - Failed to create buffer");

    if (pp_mapped_data && info.pMappedData)
        *pp_mapped_data = (u8*)info.pMappedData;

    return b;
}


void
buffer::destroy(
        VmaAllocator vma_alloc,
        VmaAllocation alloc)
{
    vmaDestroyBuffer(vma_alloc, vk, alloc);
}

#endif

}

#endif //TINYVK_BUFFER_CPP

#endif




#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

//#define DISPLAY

#ifdef DISPLAY
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#endif

using namespace tinyvk;

TEST_CASE("Scratch test", "[GPU]")
{
#ifdef DISPLAY
    // Window
    SDL_Init(SDL_INIT_EVERYTHING);
    auto* window = SDL_CreateWindow(
        "tinyvk",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        800, 600,
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
#endif
    // Extensions
    small_vector<const char*> device_ext{}, instance_ext{}, validation{};
#ifdef _MSC_VER
    validation.push_back("VK_LAYER_LUNARG_standard_validation");
#else
    validation.push_back("VK_LAYER_KHRONOS_validation");
#endif
    instance_ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#ifdef DISPLAY
    device_ext.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    u32 count{};
    SDL_Vulkan_GetInstanceExtensions(nullptr, &count, nullptr);
    instance_ext.resize(instance_ext.size() + count);
    SDL_Vulkan_GetInstanceExtensions(nullptr, &count, instance_ext.end());
#endif


    // Instance
    auto ext = tinyvk::extensions {device_ext, instance_ext, validation};
    auto instance = tinyvk::instance::create(tinyvk::application_info{}, ext);
    auto debug_messenger = tinyvk::debug_messenger::create(instance, tinyvk::DEBUG_SEVERITY_ALL);

    // Surface
    auto surface = VkSurfaceKHR{};
#ifdef DISPLAY
    SDL_Vulkan_CreateSurface(window, instance, &surface);
#endif

    // Physical device
    auto devices = tinyvk::physical_devices{instance};
    auto physical_device = devices.pick_best(ext, true, surface);

    // Logical Device and Queues
    small_vector<tinyvk::queue_request> requests{};
//    requests.push_back({tinyvk::QUEUE_GRAPHICS, 1, 1});
    tinyvk::device_features_t features { tinyvk::FEATURE_MULTI_DRAW_INDIRECT };
    auto vma_alloc = VmaAllocator{};
    auto queue_info = tinyvk::queue_create_info{requests, physical_device, surface};
    auto device = tinyvk::device::create(instance, physical_device, queue_info, ext, &features, &vma_alloc);
    auto queues = tinyvk::queue_collection{device, requests, queue_info};

#ifdef DISPLAY
    // Swapchain
    tinyvk::swapchain::images images{};
    auto swapchain = tinyvk::swapchain::create(device, physical_device, surface, images, {{800, 600}});
#endif

    VmaAllocation a;
    auto buf = buffer::create(vma_alloc, &a, {512});

    descriptor_pool_allocator alloc;
    alloc.init(device, {});
    descriptor_set_layout_cache cache;
    descriptor_set_builder build{alloc};

    VkDescriptorSet set{};
    VkDescriptorBufferInfo bi{buf, 0, -1ull};
    build.bind_buffers(0, {&bi, 1}, DESCRIPTOR_STORAGE_BUFFER);
    build.build(device, {&set, 1}, &cache, {});

    alloc.destroy(device);
    cache.destroy(device);
    buf.destroy(vma_alloc, a);

    //  Cleanup
#ifdef DISPLAY
    swapchain.destroy(device);
#endif
    device.destroy(&vma_alloc);
#ifdef DISPLAY
    vkDestroySurfaceKHR(instance, surface, nullptr);
#endif
    debug_messenger.destroy(instance);
    instance.destroy();
#ifdef DISPLAY
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
}



int main( int argc, char* argv[] ) {
    return Catch::Session().run( argc, argv );
}
