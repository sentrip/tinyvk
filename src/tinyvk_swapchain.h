//
// Created by Djordje on 8/21/2021.
//

#ifndef TINYVK_SWAPCHAIN_H
#define TINYVK_SWAPCHAIN_H

#include "tinyvk_core.h"

namespace tinyvk {

struct swapchain_desc {
    VkExtent2D          size            {};
    bool                vsync           { true };
    u32                 image_count     { 3 };
    u32                 array_layers    { 1 };
    VkFormat            formats[1]      { VK_FORMAT_R8G8B8A8_SNORM };
    VkPresentModeKHR    present_modes[1]{ VK_PRESENT_MODE_MAILBOX_KHR };
    VkColorSpaceKHR     color_space     { VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    VkImageUsageFlags   usage           { VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT };
    VkSwapchainKHR      old_swapchain   {};

};


struct swapchain_support {
    VkSurfaceCapabilitiesKHR                capabilities{};
    fixed_vector<VkPresentModeKHR>          present_modes{};
    fixed_vector<VkSurfaceFormatKHR, 16>    formats{};

    swapchain_support() = default;

    swapchain_support(
            VkPhysicalDevice            device,
            VkSurfaceKHR                surface) NEX;

    NDC ibool
                            adequate_for_present(
            VkPhysicalDevice            device) const NEX;

    NDC u32                 supported_image_count(
            u32                         image_count) const NEX;

    NDC VkExtent2D          supported_extent(
            VkExtent2D                  size) const NEX;

    NDC VkSurfaceFormatKHR  supported_surface_format(
            span<const VkFormat>        fmts,
            VkColorSpaceKHR             color_space) const NEX;

    NDC VkPresentModeKHR    supported_present_mode(
            span<const VkPresentModeKHR>modes,
            u32                         image_count,
            bool                        vsync) const NEX;
};


struct swapchain : type_wrapper<swapchain, VkSwapchainKHR> {
    using images = fixed_vector<VkImage, MAX_SWAPCHAIN_IMAGES>;

    swapchain() = default;

    static swapchain create(
            VkDevice                    device,
            VkPhysicalDevice            physical_device,
            VkSurfaceKHR                surface,
            images&                     images,
            swapchain_desc              desc,
            vk_alloc                    alloc = {}) NEX;

    void            destroy(
            VkDevice                    device,
            vk_alloc                    alloc = {}) NEX;

    u32             acquire_next_image(
            VkDevice                    device,
            VkSemaphore                 sem = {},
            VkFence                     fence = {},
            u64                         timeout = DEFAULT_TIMEOUT_NANOS) const NEX;

    static ibool    present_next_image(
            u32                         image,
            VkQueue                     queue,
            span<const VkSwapchainKHR>  swapchains,
            span<const VkSemaphore>     wait) NEX;

    ibool           present_next_image(
            u32                         image,
            VkQueue                     queue,
            span<const VkSemaphore>     wait) const NEX;
};

}

#endif //TINYVK_SWAPCHAIN_H

#ifdef TINYVK_IMPLEMENTATION

#ifndef TINYVK_SWAPCHAIN_CPP
#define TINYVK_SWAPCHAIN_CPP

#include "tinystd_algorithm.h"
#include "tinyvk_device.h"

namespace tinyvk {

//region swapchain support

swapchain_support::
swapchain_support(
        VkPhysicalDevice device,
        VkSurfaceKHR surface) NEX
{
    // TODO @VKASSERT
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

    u32 format_count = 0;
    // TODO @VKASSERT
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
    if (format_count) {
        formats.resize(format_count);
        // TODO @VKASSERT
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, formats.data());
    }

    u32 present_mode_count = 0;
    // TODO @VKASSERT
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
    if (present_mode_count) {
        present_modes.resize(present_mode_count);
        // TODO @VKASSERT
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, present_modes.data());
    }
}


VkExtent2D
swapchain_support::supported_extent(
        VkExtent2D size) const NEX
{
    VkExtent2D extent{size};
    extent.width = tinystd::max(capabilities.minImageExtent.width, tinystd::min(capabilities.maxImageExtent.width, extent.width));
    extent.height = tinystd::max(capabilities.minImageExtent.height, tinystd::min(capabilities.maxImageExtent.height, extent.height));
    return extent;
}


ibool
swapchain_support::adequate_for_present(
        VkPhysicalDevice device) const NEX
{
    fixed_vector<VkFormat> fmts{};
    for (auto& format: formats) fmts.push_back(format.format);
    auto fmt = physical_devices::find_supported_format(device, fmts,
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
    return !formats.empty() && !present_modes.empty() && fmt != VK_FORMAT_UNDEFINED;
}


u32
swapchain_support::supported_image_count(
        u32 image_count) const NEX
{
    return tinystd::min(tinystd::max(image_count, capabilities.minImageCount),
                    capabilities.maxImageCount ? capabilities.maxImageCount : UINT32_MAX);
}


VkSurfaceFormatKHR
swapchain_support::supported_surface_format(
        span<const VkFormat> fmts,
        VkColorSpaceKHR color_space) const NEX
{
    for (auto fmt: fmts) {
        for (const auto& available_format: formats) {
            if (available_format.format == fmt && available_format.colorSpace == color_space) {
                return available_format;
            }
        }
    }
    // There must be at least one format otherwise the swapchain is unsuitable
    return formats[0];
}


VkPresentModeKHR
swapchain_support::supported_present_mode(
        span<const VkPresentModeKHR> modes,
        u32 image_count,
        bool vsync) const NEX
{
    if (vsync) return VK_PRESENT_MODE_FIFO_KHR;

    for (auto requested_present_mode: modes) {
        for (const auto &present_mode : present_modes) {
            if (present_mode != requested_present_mode) continue;

            if (image_count > 2 && present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return present_mode;
            }
            if (image_count < 2 && present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                return present_mode;
            }
        }
    }
    // This is always supported
    return VK_PRESENT_MODE_FIFO_KHR;
}

//endregion

//region swapchain

swapchain
swapchain::create(
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkSurfaceKHR surface,
        images& images,
        swapchain_desc desc,
        vk_alloc alloc) NEX
{
    swapchain sc{};

    tassert(desc.image_count >= 1 && desc.image_count <= MAX_SWAPCHAIN_IMAGES && "Invalid number of swapchain images");

    auto support = swapchain_support{physical_device, surface};
    auto extent = support.supported_extent(desc.size);
    auto surface_format = support.supported_surface_format(desc.formats, desc.color_space);
    auto present_mode = support.supported_present_mode(desc.present_modes, desc.image_count, desc.vsync);
    auto supported_image_count = support.supported_image_count(desc.image_count);

    if (extent.width != desc.size.width || extent.height != desc.size.height) {
        tinystd::error("Requested size {%u, %u} but device only supports {%u, %u}\n", desc.size.width, desc.size.height, extent.width, extent.height);
        tinystd::exit(1);
    }

    if (desc.image_count != supported_image_count) {
        tinystd::error("Requested %u images but device only supports %u images\n", desc.image_count, supported_image_count);
        tinystd::exit(1);
    }

    VkSwapchainCreateInfoKHR create_info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = desc.old_swapchain;
    create_info.surface = surface;
    create_info.minImageCount = desc.image_count;
    create_info.imageArrayLayers = desc.array_layers;
    create_info.imageExtent = extent;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageUsage = desc.usage;
    create_info.preTransform = support.capabilities.currentTransform;
    create_info.presentMode = present_mode;

    vk_validate(vkCreateSwapchainKHR(device, &create_info, alloc, &sc.vk),
        "tinyvk::swapchain::create - Failed to create swapchain");

    u32 acquired_image_count = desc.image_count;
    vk_validate(vkGetSwapchainImagesKHR(device, sc.vk, &acquired_image_count, nullptr),
        "tinyvk::swapchain::create - Failed to get swapchain image count");
    tassert(acquired_image_count >= desc.image_count && "Did not acquire requested image count");
    images.resize(acquired_image_count);
    vk_validate(vkGetSwapchainImagesKHR(device, sc.vk, &acquired_image_count, images.data()),
        "tinyvk::swapchain::create - Failed to acquire swapchain images");

    return sc;
}


void swapchain::destroy(
        VkDevice device,
        vk_alloc alloc) noexcept
{
    vkDestroySwapchainKHR(device, vk, alloc);
    vk = {};
}


u32 swapchain::acquire_next_image(
        VkDevice device,
        VkSemaphore sem,
        VkFence fence, u64 timeout) const NEX
{
    u32 image_index = 0;

    tassert(sem || fence && "Acquiring an image requires either a semaphore or a fence");
    const VkResult result = vkAcquireNextImageKHR(device, vk, timeout, sem, fence, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return UINT32_MAX;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        tinystd::error("Failed to acquire swap chain image\n");
        tinystd::exit(1);
    }

    return image_index;
}


ibool swapchain::present_next_image(
        u32 image,
        VkQueue queue,
        span<const VkSwapchainKHR> swapchains,
        span<const VkSemaphore> wait) NEX
{
    VkPresentInfoKHR present_info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = u32(wait.size());
    present_info.pWaitSemaphores = wait.data();
    present_info.swapchainCount = u32(swapchains.size());
    present_info.pSwapchains = swapchains.data();
    present_info.pImageIndices = &image;

    const VkResult result = vkQueuePresentKHR(queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return true;
    }
    else if (result != VK_SUCCESS) {
        tinystd::error("Failed to acquire swap chain image\n");
        tinystd::exit(1);
    }

    return false;
}


ibool swapchain::present_next_image(
        u32 image,
        VkQueue queue,
        span<const VkSemaphore> wait) const NEX
{
    return present_next_image(image, queue, {&vk, 1}, wait);
}

//endregion

}

#endif //TINYVK_SWAPCHAIN_CPP

#endif //TINYVK_IMPLEMENTATION
