//
// Created by Djordje on 8/26/2021.
//


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


#include "tinyvk_queue.h"
#include "tinyvk_device.h"
#include "tinyvk_swapchain.h"
#include "tinyvk_command.h"
#include "tinyvk_pipeline_cache.h"
#include "tinyvk_descriptor.h"
#include "tinyvk_shader.h"

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
    descriptor_set_layout_cache cache;
    descriptor_set_builder build;
    alloc.init(device, {});

    VkDescriptorSet set{};
    VkDescriptorBufferInfo bi{buf, 0, -1ull};
    build.bind_buffers(0, {&bi, 1}, DESCRIPTOR_STORAGE_BUFFER);
    build.build(device, alloc, {&set, 1}, &cache, {});

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
