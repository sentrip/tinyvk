//
// Created by Djordje on 8/20/2021.
//


/// tinyvk.h

#include "tinystd_assert.h"

#define TINYVK_USE_VMA
#define TINYVK_IMPLEMENTATION
#include "tinyvk_queue.h"
#include "tinyvk_device.h"
#include "tinyvk_swapchain.h"
#include "tinyvk_command.h"
#include "tinyvk_pipeline_cache.h"


#ifndef TINYVK_DESCRIPTOR_H
#define TINYVK_DESCRIPTOR_H

namespace tinyvk {

enum descriptor_type_t {
    DESCRIPTOR_SAMPLER,
    DESCRIPTOR_COMBINED_IMAGE_SAMPLER,
    DESCRIPTOR_SAMPLED_IMAGE,
    DESCRIPTOR_STORAGE_IMAGE,
    DESCRIPTOR_UNIFORM_TEXEL_BUFFER,
    DESCRIPTOR_STORAGE_TEXEL_BUFFER,
    DESCRIPTOR_UNIFORM_BUFFER,
    DESCRIPTOR_STORAGE_BUFFER,
    DESCRIPTOR_UNIFORM_BUFFER_DYNAMIC,
    DESCRIPTOR_STORAGE_BUFFER_DYNAMIC,
    DESCRIPTOR_INPUT_ATTACHMENT,
    MAX_DESCRIPTOR_COUNT,
};

struct descriptor_pool : type_wrapper<descriptor_pool, VkDescriptorPool> {
    struct size {
        u32     count{};
        float   sizes[MAX_DESCRIPTOR_COUNT]{1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                                            1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    };

};


struct descriptor_set_layout : type_wrapper<descriptor_set_layout, VkDescriptorSetLayout> {

};


struct descriptor_pool_allocator {

};


struct descriptor_set_layout_cache {

};


struct descriptor_set_builder {

};

}

#endif //TINYVK_DESCRIPTOR_H

#ifdef TINYVK_IMPLEMENTATION

#ifndef TINYVK_DESCRIPTOR_CPP
#define TINYVK_DESCRIPTOR_CPP

namespace tinyvk {

}

#endif //TINYVK_DESCRIPTOR_CPP

#endif //TINYVK_IMPLEMENTATION



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
    auto queue_info = tinyvk::queue_create_info{requests, physical_device, surface};
    auto device = tinyvk::device::create(instance, physical_device, queue_info, ext, &features);
    auto queues = tinyvk::queue_collection{device, requests, queue_info};

#ifdef DISPLAY
    // Swapchain
    tinyvk::swapchain::images images{};
    auto swapchain = tinyvk::swapchain::create(device, physical_device, surface, images, {{800, 600}});
#endif

    // Draw stuff

    //  Cleanup
#ifdef DISPLAY
    swapchain.destroy(device);
#endif
    device.destroy();
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
