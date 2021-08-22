//
// Created by Djordje on 8/22/2021.
//

#include "catch.hpp"

#define TINYVK_USE_VMA
#include "tinyvk_queue.h"
#include "tinyvk_device.h"

using namespace tinyvk;

TEST_CASE("Device setup/teardown", "[tinyvk]")
{
    // Extensions
    small_vector<const char*> device_ext{}, instance_ext{}, validation{};
#ifdef _MSC_VER
    validation.push_back("VK_LAYER_LUNARG_standard_validation");
#else
    validation.push_back("VK_LAYER_KHRONOS_validation");
#endif
    instance_ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // Instance
    auto ext = tinyvk::extensions {device_ext, instance_ext, validation};
    auto instance = tinyvk::instance::create(tinyvk::application_info{}, ext);
    auto debug_messenger = tinyvk::debug_messenger::create(instance, tinyvk::DEBUG_SEVERITY_ALL);
    auto surface = VkSurfaceKHR{};

    // Physical device
    auto devices = tinyvk::physical_devices{instance};
    auto physical_device = devices.pick_best(ext, true, surface);

    // Logical Device and Queues
    small_vector<tinyvk::queue_request> requests{};
    tinyvk::device_features_t features {};
    auto queue_info = tinyvk::queue_create_info{requests, physical_device, surface};
    auto device = tinyvk::device::create(instance, physical_device, queue_info, ext, &features);
    auto queues = tinyvk::queue_collection{device, requests, queue_info};

    device.destroy();
    debug_messenger.destroy(instance);
    instance.destroy();
}
