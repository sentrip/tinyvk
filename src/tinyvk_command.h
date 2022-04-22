//
// Created by Djordje on 8/21/2021.
//

#ifndef TINYVK_COMMAND_H
#define TINYVK_COMMAND_H

#include "tinyvk_core.h"

namespace tinyvk {

enum command_pool_flags_t {
    CMD_POOL_TRANSIENT = 0x1,
    CMD_POOL_RESET_INDIVIDUAL = 0x2,
    CMD_POOL_PROTECTED = 0x4
};


struct command : type_wrapper<command, VkCommandBuffer> {

    /// Multi command functions
    void                generate_mipmaps(
            VkImage                     image,
            u32                         width,
            u32                         height,
            u32                         mip_levels = -1u) const NEX;

};


struct command_pool : type_wrapper<command_pool, VkCommandPool> {

    static command_pool create(
            VkDevice                    device,
            u32                         queue_family,
            command_pool_flags_t        flags = {},
            vk_alloc                    alloc = {}) NEX;

    void                destroy(
            VkDevice                    device,
            vk_alloc                    alloc = {}) NEX;

    void                allocate(
            VkDevice                    device,
            span<VkCommandBuffer>       cmds,
            bool secondary = false) const NEX;

    void                free(
            VkDevice                    device,
            span<VkCommandBuffer>       cmds) const NEX;
};

}

#endif //TINYVK_COMMAND_H

#ifdef TINYVK_IMPLEMENTATION

#ifndef TINYVK_COMMAND_CPP
#define TINYVK_COMMAND_CPP

namespace tinyvk {

//region command_pool

command_pool
command_pool::create(
        VkDevice device,
        u32 queue_family,
        command_pool_flags_t flags,
        vk_alloc alloc) NEX
{
    command_pool pool{};

    VkCommandPoolCreateInfo add_info {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	add_info.queueFamilyIndex = queue_family;
    add_info.flags = u32(flags);
	vk_validate(vkCreateCommandPool(device, &add_info, alloc, &pool.vk),
        "tinyvk::command_pool::create - Failed to create command pool");

    return pool;
}


void
command_pool::destroy(
        VkDevice device,
        vk_alloc alloc) NEX
{
    vkDestroyCommandPool(device, vk, alloc);
    vk = {};
}

void
command_pool::allocate(
        VkDevice device,
        span<VkCommandBuffer> cmds,
        bool secondary) const NEX
{
    VkCommandBufferAllocateInfo alloc_info {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	alloc_info.commandPool = vk;
	alloc_info.commandBufferCount = cmds.size();
    alloc_info.level = VkCommandBufferLevel(secondary);
	vk_validate(vkAllocateCommandBuffers(device, &alloc_info, cmds.data()),
	    "tinyvk::command_pool::allocate - Pool (0x%llx) failed to allocate command buffers", vk);
}

void
command_pool::free(
        VkDevice device,
        span<VkCommandBuffer> cmds) const NEX
{
    vkFreeCommandBuffers(device, vk, u32(cmds.size()), cmds.data());
    for (auto& c: cmds) c = {};
}

//endregion

//region command

void command::generate_mipmaps(
        VkImage image,
        u32 width,
        u32 height,
        u32 mip_levels) const noexcept
{
    /*
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(device.ctx().instance.vk_physical_device, (VkFormat)TinyImageFormat_ToVkFormat(texture.format()), &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }
    */
    if (mip_levels == -1u) {
        u32 w = width, h = height;
        mip_levels = 0;
        while (w > 1 && h > 1) { w /= 2; h /= 2; ++mip_levels; }
    }
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    i32 mip_width = i32(width);
    i32 mip_height = i32(height);

    for (u32 i = 1; i < mip_levels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(vk,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mip_width, mip_height, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(vk,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(vk,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mip_width > 1) mip_width /= 2;
        if (mip_height > 1) mip_height /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mip_levels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(vk,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
}

//endregion

}

#endif //TINYVK_COMMAND_CPP

#endif //TINYVK_IMPLEMENTATION
