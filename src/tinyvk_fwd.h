//
// Created by Djordje on 8/20/2021.
//

#ifndef TINYVK_FWD_H
#define TINYVK_FWD_H

namespace tinyvk {

/// tinyvk_device.h
struct extensions;
struct application_info;
struct instance;
struct physical_devices;
struct device;
struct debug_messenger;

/// tinyvk_queue.h
struct queue_request;
struct queue_family_properties;
struct queue_availability;
struct queue_create_info;
struct queue_collection;

/// tinyvk_swapchain.h
struct swapchain_desc;
struct swapchain_support;
struct swapchain;

/// tinyvk_command.h
struct command;
struct command_pool;

/// tinyvk_pipeline_cache.h
struct pipeline_cache_header;

};

/// vulkan fwd
struct VkAllocationCallbacks;

#define TINYVK_DEFINE_HANDLE(X) typedef struct X##_T* X

TINYVK_DEFINE_HANDLE(VmaAllocator);

#endif //TINYVK_FWD_H
