//
// Created by Djordje on 8/22/2021.
//

#include "catch.hpp"

#define TINYVK_USE_VMA
#define TINYVK_IMPLEMENTATION
#include "tinyvk_queue.h"
#include "tinyvk_device.h"

using namespace tinyvk;

TEST_CASE("queue_create_infos", "[tinyvk]")
{
    small_vector<queue_request> requests;
    queue_family_properties props{};

    SECTION("Integrated GPU")
    {
        VkQueueFamilyProperties p{};

        p.queueCount = 1;
        p.queueFlags = 0
                | VK_QUEUE_GRAPHICS_BIT
                | VK_QUEUE_COMPUTE_BIT
                | VK_QUEUE_TRANSFER_BIT
                | VK_QUEUE_SPARSE_BINDING_BIT;
        props.push_back(p);

        SECTION("No requests")
        {
            queue_availability av{requests, props};
            queue_create_info q{requests, props, av};

            for (auto& qs: q.queues) {
                REQUIRE( 1ull == qs.size() );
                REQUIRE( 0u == qs[0].family );
                REQUIRE( 0u == qs[0].index );
                REQUIRE( 0u == qs[0].physical_index );
                REQUIRE( 1u == qs[0].is_shared );
            }
        }

        SECTION("Request single")
        {
            for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type)
                requests.push_back({queue_type_t(type), 0, 1});
            queue_availability av{requests, props};
            queue_create_info q{requests, props, av};

            for (auto& qs: q.queues) {
                REQUIRE( 1ull == qs.size() );
                REQUIRE( 0u == qs[0].family );
                REQUIRE( 0u == qs[0].index );
                REQUIRE( 0u == qs[0].physical_index );
                REQUIRE( 1u == qs[0].is_shared );
            }
        }

        SECTION("Request too many")
        {
            for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type)
                requests.push_back({queue_type_t(type), 0, 2});
            queue_availability av{requests, props};
            queue_create_info q{requests, props, av};

            for (auto& qs: q.queues) {
                REQUIRE( 2ull == qs.size() );
                for (u32 i = 0; i < 2; ++i) {
                    REQUIRE( 0u == qs[i].family );
                    REQUIRE( i == qs[i].index );
                    REQUIRE( 0u == qs[i].physical_index );
                    REQUIRE( 1u == qs[i].is_shared );
                }
            }
        }
    }

    SECTION("Dedicated GPU")
    {
        VkQueueFamilyProperties p{};

        p.queueCount = 16;
        p.queueFlags = 0
                | VK_QUEUE_GRAPHICS_BIT
                | VK_QUEUE_COMPUTE_BIT
                | VK_QUEUE_TRANSFER_BIT
                | VK_QUEUE_SPARSE_BINDING_BIT;
        props.push_back(p);

        p.queueCount = 8;
        p.queueFlags = 0
//                | VK_QUEUE_GRAPHICS_BIT
//                | VK_QUEUE_COMPUTE_BIT
                | VK_QUEUE_TRANSFER_BIT
                | VK_QUEUE_SPARSE_BINDING_BIT;

        p.queueCount = 2;
        p.queueFlags = 0
//                | VK_QUEUE_GRAPHICS_BIT
                | VK_QUEUE_COMPUTE_BIT
                | VK_QUEUE_TRANSFER_BIT
                | VK_QUEUE_SPARSE_BINDING_BIT;

        props.push_back(p);

        SECTION("No requests")
        {
            queue_availability av{requests, props};
            queue_create_info q{requests, props, av};
            u32 i = 0;
            for (auto& qs: q.queues) {
                REQUIRE( 1ull == qs.size() );
                REQUIRE( 0u == qs[0].family );
                REQUIRE( i == qs[0].index );
                REQUIRE( i == qs[0].physical_index );
                REQUIRE( 0u == qs[0].is_shared );
                ++i;
            }
        }

        SECTION("Request dedicated")
        {
            for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type)
                requests.push_back({queue_type_t(type), 0, 1});
            queue_availability av{requests, props};
            queue_create_info q{requests, props, av};
            u32 i = 0;
            for (auto& qs: q.queues) {
                REQUIRE( 1ull == qs.size() );
                REQUIRE( 0u == qs[0].family );
                REQUIRE( i == qs[0].index );
                REQUIRE( i == qs[0].physical_index );
                REQUIRE( 0u == qs[0].is_shared );
                ++i;
            }
        }

        SECTION("Request too many dedicated")
        {

        }
    }
}

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
