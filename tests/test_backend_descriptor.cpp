//
// Created by Djordje on 8/26/2021.
//

#define TINYVK_USE_VMA
#define TINYVK_IMPLEMENTATION
#include "tinyvk_device.h"
#include "tinyvk_descriptor.h"
#include "tinyvk_renderpass.h"
#include "tinyvk_buffer.h"
#include "tinyvk_image.h"

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

/*
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
*/

//#include <bitset>
//#include <iostream>
//
//unsigned int get_bits(float f)
//{
//    auto* i = (unsigned int*)&f;
//    return *i;
//}
//
//void print_bits(float f)
//{
//    std::cout << "0b" << std::bitset<32>{get_bits(f)} << "\n";
//}
//
//static constexpr unsigned int SIGN        = 0b10000000000000000000000000000000;
//static constexpr unsigned int EXPONENT    = 0b01111111100000000000000000000000;
//static constexpr unsigned int EXPONENT_S  = 23;
//
//
//float floor_power_of_two(float v, unsigned int power)
//{
//    static constexpr unsigned int FLOAT_EXP_SHIFT   = 23;
//    static constexpr unsigned int FLOAT_EXP_OFFSET  = 127;
//    static constexpr unsigned int FLOAT_EXP         = 0b01111111100000000000000000000000;
//    const unsigned int i = *((const unsigned int*)&v);
//    const unsigned int e = ((i & FLOAT_EXP) >> FLOAT_EXP_SHIFT) - FLOAT_EXP_OFFSET;
//    return v - float((1u << power) * (e >= power));
//}


//TEST_CASE("Xyz", "[tinyvk_test]")
//{
//    printf("%.2f\n", floor_power_of_two(16.0f, 12));
//    printf("%.8f, %.8f\n", 3.27f, floor_power_of_two(4099.27f, 12));
//    print_bits(3.27f);
//    print_bits(floor_power_of_two(4099.27f, 12));
//    print_bits(0.0f);
//    print_bits(1.0f);
//    print_bits(2.0f);
//    print_bits(4.0f);
//    print_bits(8.0f);
//    printf("%u\n", ((get_bits(1.0f) & EXPONENT) >> EXPONENT_S) - 127);
//    printf("%u\n", ((get_bits(2.0f) & EXPONENT) >> EXPONENT_S) - 127);
//    printf("%u\n", ((get_bits(4.0f) & EXPONENT) >> EXPONENT_S) - 127);
//    printf("%u\n", ((get_bits(8.0f) & EXPONENT) >> EXPONENT_S) - 127);
//    printf("%u\n", ((get_bits(1024.0f) & EXPONENT) >> EXPONENT_S) - 127);
//    printf("%u\n", ((get_bits(1024.0f) & EXPONENT) >> EXPONENT_S) - 127);
//}