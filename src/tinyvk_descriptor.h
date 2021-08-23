//
// Created by Djordje on 8/23/2021.
//

#include "tinyvk_core.h"

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


struct descriptor {
    u32                     binding{};
    descriptor_type_t       type{};
    u32                     count{};
    shader_stage_t          stages{};
    const VkSampler*        immutable_samplers{};

    size_t      hash_code() const noexcept;
};


struct descriptor_pool_size {
    u32     count{64};
    float   sizes[MAX_DESCRIPTOR_COUNT]{1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                                        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
};


struct descriptor_pool : type_wrapper<descriptor_pool, VkDescriptorPool> {
    static descriptor_pool          create(
            VkDevice                    device,
            u32                         set_count,
            descriptor_pool_size        size = {},
            vk_alloc                    alloc = {});

    void                            destroy(
            VkDevice                    device,
            vk_alloc                    alloc = {});
};


struct descriptor_set_layout : type_wrapper<descriptor_set_layout, VkDescriptorSetLayout> {

    static descriptor_set_layout    create(
            VkDevice                    device,
            span<const descriptor>      bindings,
            vk_alloc                    alloc = {});

    void                            destroy(
            VkDevice                    device,
            vk_alloc                    alloc = {});
};


struct descriptor_pool_allocator {
    VkDescriptorPool                    m_current_pool{};
    descriptor_pool_size                m_size{};
    small_vector<VkDescriptorPool>      m_used_pools{};
    small_vector<VkDescriptorPool>      m_free_pools{};
    u32                                 m_set_count{};

    void                    init(
            VkDevice                device,
            descriptor_pool_size    size = {},
            u32                     set_count = 1024,
            vk_alloc                alloc = {});

    void                    destroy(
            VkDevice                device,
            vk_alloc                alloc = {});

    VkDescriptorPool        allocate(
            VkDevice                device,
            span<VkDescriptorSet>   sets,
            VkDescriptorSetLayout   layout,
            ibool                   new_pool = false,
            vk_alloc                alloc = {});

    void                    reset(
            VkDevice                device,
            VkDescriptorPool        pool = {});
};


struct descriptor_set_layout_cache {
    static constexpr size_t N   = 256;

    small_vector<u64, N>                    m_layout_hashes{};
    small_vector<VkDescriptorSetLayout, N>  m_layouts{};

    VkDescriptorSetLayout   create(
            VkDevice                device,
            span<const descriptor>  bindings,
            ibool*                  is_new = {},
            vk_alloc                alloc = {});
};


struct descriptor_set_builder {
    static constexpr size_t N = 64;

    // NOTE: This is intentionally a reference as a requirement for construction to prevent usage in default constructed state
    descriptor_pool_allocator&              pool_alloc;
    small_vector<descriptor, N>             bindings{};
    small_vector<VkWriteDescriptorSet, N>   writes{};

    using buffer_infos  = span<const VkDescriptorBufferInfo>;
    using image_infos   = span<const VkDescriptorImageInfo>;

	descriptor_set_builder& bind_buffers(
	        u32                             binding,
	        buffer_infos                    infos,
	        descriptor_type_t               type,
	        shader_stage_t                  stages = SHADER_ALL,
	        u32                             array_offset = 0);

	descriptor_set_builder& bind_images(
	        u32                             binding,
	        image_infos                     infos,
	        descriptor_type_t               type,
	        shader_stage_t                  stages = SHADER_ALL,
	        u32                             array_offset = 0);

	VkDescriptorPool        build(
            VkDevice                        device,
	        span<VkDescriptorSet>           sets,
	        descriptor_set_layout_cache*    cache = {},
	        VkDescriptorSetLayout*          created_layout = {},
	        ibool                           new_pool = false,
	        vk_alloc                        alloc = {});
};

}

#endif //TINYVK_DESCRIPTOR_H

#ifdef TINYVK_IMPLEMENTATION

#ifndef TINYVK_DESCRIPTOR_CPP
#define TINYVK_DESCRIPTOR_CPP

namespace tinyvk {

//region descriptor::hash_code

size_t descriptor::hash_code() const noexcept
{
    size_t h{1};
    tinystd::hash_combine(h, binding);
    tinystd::hash_combine(h, count);
    tinystd::hash_combine(h, type);
    tinystd::hash_combine(h, stages);
    tinystd::hash_combine(h, size_t(immutable_samplers));
    return h;
}

//endregion

//region descriptor_pool

descriptor_pool
descriptor_pool::create(
        VkDevice device,
        u32 set_count,
        descriptor_pool_size size,
        vk_alloc alloc)
{
    descriptor_pool pool;

    u32 count{};
    VkDescriptorPoolSize pool_sizes[MAX_DESCRIPTOR_COUNT]{};
    for (u32 i = 0; i < MAX_DESCRIPTOR_COUNT; ++i) {
        if (size.sizes[i] > 0.0f) {
            pool_sizes[count].type = VkDescriptorType(i);
            pool_sizes[count++].descriptorCount = u32(float(size.count) * size.sizes[i]);
        }
    }

    tassert(count && "Must provide descriptor pool sizes");

    VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.poolSizeCount = count;
    pool_info.pPoolSizes = pool_sizes;
    pool_info.maxSets = set_count;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    vk_validate(vkCreateDescriptorPool(device, &pool_info, alloc, &pool.vk),
              "tinyvk::descriptor_pool::create - Failed to create descriptor pool");

    return pool;
}


void
descriptor_pool::destroy(
        VkDevice device,
        vk_alloc alloc)
{
    vkDestroyDescriptorPool(device, vk, alloc);
    vk = {};
}

//endregion

//region descriptor_set_layout

descriptor_set_layout descriptor_set_layout::create(
        VkDevice device,
        span<const descriptor> bindings,
        vk_alloc alloc)
{
    descriptor_set_layout layout{};

    VkDescriptorSetLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = u32(bindings.size());
    layout_info.pBindings = (const VkDescriptorSetLayoutBinding*)bindings.data();
    vk_validate(vkCreateDescriptorSetLayout(device, &layout_info, alloc, &layout.vk),
              "tinyvk::descriptor_set_layout::create - Failed to create descriptor set layout");

    return layout;
}


void
descriptor_set_layout::destroy(
        VkDevice device,
        vk_alloc alloc)
{
    vkDestroyDescriptorSetLayout(device, vk, alloc);
    vk = {};
}

//endregion

//region descriptor_pool_allocator

void
descriptor_pool_allocator::init(
        VkDevice device,
        descriptor_pool_size size,
        u32 set_count,
        vk_alloc alloc)
{
    if (m_current_pool)
        destroy(device);
    m_size = size;
    m_set_count = set_count;
    m_current_pool = descriptor_pool::create(device, m_set_count, m_size, alloc);
    m_used_pools.push_back(m_current_pool);
}


void
descriptor_pool_allocator::destroy(
        VkDevice device,
        vk_alloc alloc)
{
	for (auto p : m_free_pools)
		vkDestroyDescriptorPool(device, p, alloc);
	for (auto p : m_used_pools)
		vkDestroyDescriptorPool(device, p, alloc);
	m_free_pools.clear();
	m_used_pools.clear();
}


VkDescriptorPool
descriptor_pool_allocator::allocate(
        VkDevice device,
        span<VkDescriptorSet> sets,
        VkDescriptorSetLayout layout,
        ibool new_pool,
        vk_alloc alloc)
{
    if (new_pool) {
        m_current_pool = descriptor_pool::create(device, m_set_count, m_size);
        m_used_pools.push_back(m_current_pool);
    }

    VkDescriptorSetAllocateInfo alloc_info {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.pSetLayouts = &layout;
    alloc_info.descriptorPool = m_current_pool;
    alloc_info.descriptorSetCount = u32(sets.size());

    VkResult result = vkAllocateDescriptorSets(device, &alloc_info, sets.data());
    if (result == VK_SUCCESS)
        return m_current_pool;
    if (result != VK_ERROR_FRAGMENTED_POOL && result != VK_ERROR_OUT_OF_POOL_MEMORY)
        return nullptr;

    if (!m_free_pools.empty()) {
        m_current_pool = m_free_pools.pop_back();
    }
    else {
        m_current_pool = descriptor_pool::create(device, m_set_count, m_size, alloc);
    }

    m_used_pools.push_back(m_current_pool);
    alloc_info.descriptorPool = m_current_pool;
    result = vkAllocateDescriptorSets(device, &alloc_info, sets.data());
    return result == VK_SUCCESS ? m_current_pool : nullptr;
}


void
descriptor_pool_allocator::reset(
        VkDevice device,
        VkDescriptorPool pool)
{
    vkResetDescriptorPool(device, pool, {});
    m_free_pools.push_back(pool);
}

//endregion

//region descriptor_set_layout_cache

VkDescriptorSetLayout
descriptor_set_layout_cache::create(
        VkDevice device,
        span<const descriptor> bindings,
        ibool *is_new,
        vk_alloc alloc)
{
    u64 h = 1;
    for(const auto& b: bindings)
        tinystd::hash_combine(h, b.hash_code());

    auto* it = tinystd::find(m_layout_hashes.begin(), m_layout_hashes.end(), h);
    if (it != m_layout_hashes.end()) {
        if (is_new) *is_new = false;
        return m_layouts[it - m_layout_hashes.begin()];
    }

    if (is_new) *is_new = true;
    return descriptor_set_layout::create(device, bindings, alloc);
}

//endregion

//region descriptor_builder

descriptor_set_builder&
descriptor_set_builder::bind_buffers(
        u32 binding,
        buffer_infos infos,
        descriptor_type_t type,
        shader_stage_t stages,
        u32 array_offset)
{
    bindings.push_back({binding, type, u32(infos.size()), stages});
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.dstArrayElement = array_offset;
    write.descriptorCount = u32(infos.size());
    write.descriptorType = VkDescriptorType(type);
    write.pBufferInfo = infos.data();
    writes.push_back(write);
    return *this;
}


descriptor_set_builder&
descriptor_set_builder::bind_images(
        u32 binding,
        image_infos infos,
        descriptor_type_t type,
        shader_stage_t stages,
        u32 array_offset)
{
    bindings.push_back({binding, type, u32(infos.size()), stages});
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.dstArrayElement = array_offset;
    write.descriptorCount = u32(infos.size());
    write.descriptorType = VkDescriptorType(type);
    write.pImageInfo = infos.data();
    writes.push_back(write);
    return *this;
}


VkDescriptorPool
descriptor_set_builder::build(
        VkDevice device,
        span<VkDescriptorSet> sets,
        descriptor_set_layout_cache* cache,
        VkDescriptorSetLayout* created_layout,
        ibool new_pool,
        vk_alloc alloc)
{
    auto layout = cache
        ? cache->create(device, bindings, {}, alloc)
        : VkDescriptorSetLayout(descriptor_set_layout::create(device, bindings, alloc));

    if (created_layout)
        *created_layout = layout;

    auto pool = pool_alloc.allocate(device, sets, layout, new_pool, alloc);
    if (!pool)
        return pool;

    for (const auto set: sets) {
        for (auto& w: writes) w.dstSet = set;
        vkUpdateDescriptorSets(device, u32(writes.size()), writes.data(), 0, nullptr);
    }

    return pool;
}

//endregion

}

#endif //TINYVK_DESCRIPTOR_CPP

#endif //TINYVK_IMPLEMENTATION
