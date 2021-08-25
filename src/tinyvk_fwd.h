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

/// tinyvk_descriptor.h
struct descriptor;
struct descriptor_pool_size;
struct descriptor_pool;
struct descriptor_set;
struct descriptor_set_layout;
struct descriptor_pool_allocator;
struct descriptor_set_layout_cache;
struct descriptor_set_builder;
struct descriptor_set_context;

/// tinyvk_pipeline_cache.h
struct pipeline_cache_header;

/// tinyvk_shader.h
struct shader_macro;
struct shader_module;

}

/// vulkan fwd
struct VkAllocationCallbacks;

#define TINYVK_DEFINE_HANDLE(X) typedef struct X##_T* X

TINYVK_DEFINE_HANDLE(VkDevice);
TINYVK_DEFINE_HANDLE(VkSwapchainKHR);
TINYVK_DEFINE_HANDLE(VkInstance);
TINYVK_DEFINE_HANDLE(VkSurfaceKHR);
TINYVK_DEFINE_HANDLE(VkPhysicalDevice);
TINYVK_DEFINE_HANDLE(VkDebugUtilsMessengerEXT);

TINYVK_DEFINE_HANDLE(VkEvent);
TINYVK_DEFINE_HANDLE(VkFence);
TINYVK_DEFINE_HANDLE(VkSemaphore);
TINYVK_DEFINE_HANDLE(VkQueue);
TINYVK_DEFINE_HANDLE(VkQueryPool);

TINYVK_DEFINE_HANDLE(VkBuffer);
TINYVK_DEFINE_HANDLE(VkImage);
TINYVK_DEFINE_HANDLE(VkSampler);
TINYVK_DEFINE_HANDLE(VkImageView);
TINYVK_DEFINE_HANDLE(VkCommandBuffer);
TINYVK_DEFINE_HANDLE(VkCommandPool);

TINYVK_DEFINE_HANDLE(VkDescriptorPool);
TINYVK_DEFINE_HANDLE(VkDescriptorSet);
TINYVK_DEFINE_HANDLE(VkDescriptorSetLayout);

TINYVK_DEFINE_HANDLE(VkPipeline);
TINYVK_DEFINE_HANDLE(VkPipelineLayout);
TINYVK_DEFINE_HANDLE(VkPipelineCache);
TINYVK_DEFINE_HANDLE(VkShaderModule);

TINYVK_DEFINE_HANDLE(VkRenderPass);
TINYVK_DEFINE_HANDLE(VkFramebuffer);

TINYVK_DEFINE_HANDLE(VmaAllocator);
TINYVK_DEFINE_HANDLE(VmaAllocation);

#endif //TINYVK_FWD_H
