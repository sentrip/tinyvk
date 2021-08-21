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

};


struct command_pool : type_wrapper<command, VkCommandPool> {

    static command_pool create(
            VkDevice                    device,
            u32                         queue_family,
            command_pool_flags_t        flags = {},
            vk_alloc                    alloc = {}) NEX;

    void            destroy(
            VkDevice                    device,
            vk_alloc                    alloc = {}) NEX;

    void            allocate(
            VkDevice                    device,
            span<VkCommandBuffer>       cmds,
            bool secondary = false) const NEX;

    void            free(
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

}

#endif //TINYVK_COMMAND_CPP

#endif //TINYVK_IMPLEMENTATION
