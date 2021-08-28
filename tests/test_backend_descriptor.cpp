//
// Created by Djordje on 8/26/2021.
//

#define TINYVK_IMPLEMENTATION
#include "tinyvk_device.h"
#include "tinyvk_descriptor.h"


struct TestDevice {
    tinyvk::device device{};
    tinyvk::instance instance{};

    explicit TestDevice() {
        // Instance
        auto ext = tinyvk::extensions {};
        instance = tinyvk::instance::create(tinyvk::application_info{}, ext);

        // Physical device
        auto devices = tinyvk::physical_devices{instance};
        auto physical_device = devices.pick_best(ext, true);

        // Logical Device and Queues
        tinyvk::device_features_t features {};
        auto queue_info = tinyvk::queue_create_info{{}, physical_device};
        device = tinyvk::device::create(instance, physical_device, queue_info, ext, &features);
    }

    ~TestDevice() {
        device.destroy();
        instance.destroy();
    }
};


#include "catch.hpp"


TEST_CASE("Backend Descriptor", "[tinyvk_test]")
{
    TestDevice d{};

    tinyvk::descriptor_pool_allocator alloc;
    tinyvk::descriptor_set_layout_cache cache;
    tinyvk::descriptor_set_builder build;
    alloc.init(d.device, {});

    VkDescriptorBufferInfo bi0{VkBuffer(1), 0, -1ull};
    VkDescriptorBufferInfo bi1{VkBuffer(2), 0, -1ull};

    tinyvk::descriptor_set_context ctx0;
    ctx0
        .build(0)
        .bind_buffers(0, {&bi0, 1}, tinyvk::DESCRIPTOR_STORAGE_BUFFER);

    ctx0.build(d.device, alloc, 1, {nullptr, &cache});

    tinyvk::descriptor_set_context ctx1;
    ctx1
        .build(0)
        .bind_buffers(1, {&bi1, 1}, tinyvk::DESCRIPTOR_STORAGE_BUFFER);

    ctx1.build(d.device, alloc, 1, {&ctx0, &cache});

    alloc.destroy(d.device);
    cache.destroy(d.device);
}