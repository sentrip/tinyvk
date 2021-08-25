//
// Created by Djordje on 8/23/2021.
//

#include "tinyvk_core.h"

#ifndef TINYVK_DESCRIPTOR_H
#define TINYVK_DESCRIPTOR_H

namespace tinyvk {

enum descriptor_type_t: u32 {
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

    size_t      hash_code() const NEX;
};


struct descriptor_empty_set {
    VkDescriptorSet         set{};
    VkDescriptorSetLayout   layout{};
};


struct descriptor_pool_size {
    u32     count{64};
    float   sizes[MAX_DESCRIPTOR_COUNT]{1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                                        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
};


struct descriptor_set {
    using buffer_infos  = span<const VkDescriptorBufferInfo>;
    using image_infos   = span<const VkDescriptorImageInfo>;

    template<typename Vector>
    static void                         write_buffers(
            Vector&                         vec,
            u32                             binding,
	        descriptor_type_t               type,
	        buffer_infos                    infos,
	        u32                             array_index = 0,
            VkDescriptorSet                 set = {}) NEX;

    template<typename Vector>
    static void                         write_images(
            Vector&                         vec,
            u32                             binding,
	        descriptor_type_t               type,
	        image_infos                     infos,
	        u32                             array_index = 0,
            VkDescriptorSet                 set = {}) NEX;

    template<typename Vector>
    static void                         write(
            VkDevice                        device,
            Vector&                         vec,
            VkDescriptorSet                 set = {}) NEX;

};


struct descriptor_pool : type_wrapper<descriptor_pool, VkDescriptorPool> {

    static descriptor_pool          create(
            VkDevice                    device,
            u32                         set_count,
            descriptor_pool_size        size = {},
            vk_alloc                    alloc = {}) NEX;

    void                            destroy(
            VkDevice                    device,
            vk_alloc                    alloc = {}) NEX;
};


struct descriptor_set_layout : type_wrapper<descriptor_set_layout, VkDescriptorSetLayout> {

    static descriptor_set_layout    create(
            VkDevice                    device,
            span<const descriptor>      bindings,
            vk_alloc                    alloc = {}) NEX;

    void                            destroy(
            VkDevice                    device,
            vk_alloc                    alloc = {}) NEX;
};



/// descriptor high level API
struct default_descriptor_api_limits {
    static constexpr size_t LAYOUT_CACHE_STACK_SIZE = 48;
    static constexpr size_t SET_BUILDER_STACK_SIZE  = 64;
    static constexpr size_t SET_CONTEXT_STACK_SIZE  = 16;
    static constexpr size_t SET_CONTEXT_MAX_SETS    = 4;
};
using descriptor_api_limits = TINYVK_DESCRIPTOR_API_LIMITS;


struct descriptor_pool_allocator {
    VkDescriptorPool                    m_current_pool{};
    descriptor_pool_size                m_size{};
    small_vector<VkDescriptorPool>      m_used_pools{};
    small_vector<VkDescriptorPool>      m_free_pools{};
    u32                                 m_set_count{};

    void                    init(
            VkDevice                device,
            descriptor_pool_size    size = {},
            u32                     set_count = 64,
            vk_alloc                alloc = {}) NEX;

    void                    destroy(
            VkDevice                device,
            vk_alloc                alloc = {}) NEX;

    VkDescriptorPool        allocate(
            VkDevice                device,
            span<VkDescriptorSet>   sets,
            VkDescriptorSetLayout   layout,
            ibool                   new_pool = false,
            vk_alloc                alloc = {}) NEX;

    void                    reset(
            VkDevice                device,
            VkDescriptorPool        pool = {}) NEX;
};


struct descriptor_set_layout_cache {
    static constexpr size_t N   = descriptor_api_limits::LAYOUT_CACHE_STACK_SIZE;

    small_vector<u64, N>                    m_layout_hashes{};
    small_vector<descriptor_set_layout, N>  m_layouts{};
    small_vector<u16, N>                    m_ref_counts{};

    descriptor_set_layout   create(
            VkDevice                device,
            span<const descriptor>  bindings,
            ibool*                  is_new = {},
            vk_alloc                alloc = {}) NEX;

    void                    destroy(
            VkDevice                device,
            VkDescriptorSetLayout   layout,
            vk_alloc                alloc = {}) NEX;

    void                    destroy(
            VkDevice                device,
            vk_alloc                alloc = {}) NEX;
};


struct descriptor_set_builder {
    static constexpr size_t N = descriptor_api_limits::SET_BUILDER_STACK_SIZE;

    small_vector<descriptor, N>             m_bindings{};
    small_vector<VkWriteDescriptorSet, N>   m_writes{};
    descriptor_set_layout                   m_layout{};

    using buffer_infos  = span<const VkDescriptorBufferInfo>;
    using image_infos   = span<const VkDescriptorImageInfo>;

	descriptor_set_builder& bind_buffers(
	        u32                             binding,
	        buffer_infos                    infos,
	        descriptor_type_t               type,
	        shader_stage_t                  stages = SHADER_ALL) NEX;

	descriptor_set_builder& bind_images(
	        u32                             binding,
	        image_infos                     infos,
	        descriptor_type_t               type,
	        shader_stage_t                  stages = SHADER_ALL) NEX;

	VkDescriptorPool        build(
            VkDevice                        device,
            descriptor_pool_allocator&      pool_alloc,
	        span<VkDescriptorSet>           sets,
	        descriptor_set_layout_cache*    cache = {},
	        ibool                           new_pool = false,
	        vk_alloc                        alloc = {}) NEX;
};


struct descriptor_set_context {
    using builder           = descriptor_set_builder;
    using filter_callback   = bool(void*, const descriptor&);

    struct build_desc {
        descriptor_set_context*         previous{};
        descriptor_set_layout_cache*    cache{};
        descriptor_empty_set            empty_set{};
        filter_callback*                filter{};
        void*                           filter_userdata{};
    };

    static constexpr size_t N  = descriptor_api_limits::SET_CONTEXT_MAX_SETS;
    static constexpr size_t NP = descriptor_api_limits::SET_CONTEXT_STACK_SIZE;

    small_vector<VkDescriptorSet, NP>   m_physical{};
    fixed_vector<builder, N>            m_builders{};
    u32                                 m_physical_count{};

    ~descriptor_set_context() NEX;

    span<VkDescriptorSet>           physical(
            u32                             set) NEX;

    descriptor_set_builder&         build(
            u32                             set) NEX;

    void                            build(
            VkDevice                        device,
            descriptor_pool_allocator&      pool_alloc,
            u32                             physical_count,
            build_desc                      desc,
            vk_alloc                        alloc = {}) NEX;
};


/// template implementation

template<typename Vector>
void
descriptor_set::write_buffers(
        Vector& vec,
        u32 binding,
        descriptor_type_t type,
        buffer_infos infos,
        u32 array_index,
        VkDescriptorSet set) NEX
{
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.dstArrayElement = array_index;
    write.descriptorCount = u32(infos.size());
    write.descriptorType = VkDescriptorType(type);
    write.pBufferInfo = infos.data();
    vec.push_back(write);
}


template<typename Vector>
void
descriptor_set::write_images(
        Vector& vec,
        u32 binding,
        descriptor_type_t type,
        image_infos infos,
        u32 array_index,
        VkDescriptorSet set) NEX
{
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.dstArrayElement = array_index;
    write.descriptorCount = u32(infos.size());
    write.descriptorType = VkDescriptorType(type);
    write.pImageInfo = infos.data();
    vec.push_back(write);
}


template<typename Vector>
void
descriptor_set::write(
        VkDevice device,
        Vector& vec,
        VkDescriptorSet set) NEX
{
    if (!vec.empty()) {
        for (auto& v: vec) v.dstSet = set;
        vkUpdateDescriptorSets(device, u32(vec.size()), vec.data(), 0, nullptr);
    }
}

}

#endif //TINYVK_DESCRIPTOR_H

#ifdef TINYVK_IMPLEMENTATION

#ifndef TINYVK_DESCRIPTOR_CPP
#define TINYVK_DESCRIPTOR_CPP

namespace tinyvk {

//region descriptor::hash_code

size_t descriptor::hash_code() const NEX
{
    size_t h{1};
    tinystd::hash_combine(h, binding);
    tinystd::hash_combine(h, count);
    tinystd::hash_combine(h, type);
    tinystd::hash_combine(h, stages);
    return h;
}

//endregion

//region descriptor_pool

descriptor_pool
descriptor_pool::create(
        VkDevice device,
        u32 set_count,
        descriptor_pool_size size,
        vk_alloc alloc) NEX
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
        vk_alloc alloc) NEX
{
    vkDestroyDescriptorPool(device, vk, alloc);
    vk = {};
}

//endregion

//region descriptor_set_layout

descriptor_set_layout descriptor_set_layout::create(
        VkDevice device,
        span<const descriptor> bindings,
        vk_alloc alloc) NEX
{
    descriptor_set_layout layout{};

    VkDescriptorSetLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = u32(bindings.size());
    small_vector<VkDescriptorSetLayoutBinding, 256> vk_bindings{};
    for (auto& b: bindings)
        vk_bindings.push_back({b.binding, VkDescriptorType(b.type), b.count, b.stages, nullptr});
    layout_info.pBindings = vk_bindings.data();
    vk_validate(vkCreateDescriptorSetLayout(device, &layout_info, alloc, &layout.vk),
              "tinyvk::descriptor_set_layout::create - Failed to create descriptor set layout");

    return layout;
}


void
descriptor_set_layout::destroy(
        VkDevice device,
        vk_alloc alloc) NEX
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
        vk_alloc alloc) NEX
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
        vk_alloc alloc) NEX
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
        vk_alloc alloc) NEX
{
    tassert(m_current_pool && "Did not call descriptor_pool_allocator::init()");

    if (new_pool) {
        m_current_pool = descriptor_pool::create(device, m_set_count, m_size);
        m_used_pools.push_back(m_current_pool);
    }

    small_vector<VkDescriptorSetLayout, 64> layouts{};
    for (auto& set: sets) layouts.push_back(layout);

    VkDescriptorSetAllocateInfo alloc_info {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool = m_current_pool;
    alloc_info.descriptorSetCount = u32(sets.size());
    alloc_info.pSetLayouts = layouts.data();

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
        VkDescriptorPool pool) NEX
{
    vkResetDescriptorPool(device, pool, {});
    m_free_pools.push_back(pool);
    auto it = tinystd::find(m_used_pools.begin(), m_used_pools.end(), pool);
    if (it != m_used_pools.end()) {
        m_used_pools.erase(it);
    }
}

//endregion

//region descriptor_set_layout_cache

descriptor_set_layout
descriptor_set_layout_cache::create(
        VkDevice device,
        span<const descriptor> bindings,
        ibool *is_new,
        vk_alloc alloc) NEX
{
    u64 h = 1;
    for(const auto& b: bindings)
        tinystd::hash_combine(h, b.hash_code());

    auto* it = tinystd::find(m_layout_hashes.begin(), m_layout_hashes.end(), h);
    if (it != m_layout_hashes.end()) {
        if (is_new) *is_new = false;
        const u32 i = it - m_layout_hashes.begin();
        ++m_ref_counts[i];
        return m_layouts[i];
    }

    if (is_new) *is_new = true;
    auto layout = descriptor_set_layout::create(device, bindings, alloc);
    m_layouts.push_back(layout);
    m_layout_hashes.push_back(h);
    m_ref_counts.push_back(1);
    return layout;
}


void
descriptor_set_layout_cache::destroy(
        VkDevice device,
        VkDescriptorSetLayout layout,
        vk_alloc alloc) NEX
{
    auto* it = tinystd::find(m_layouts.begin(), m_layouts.end(), layout);
    if (it != m_layouts.end()) {
        const u32 i = it - m_layouts.begin();
        if (--m_ref_counts[i] == 0) {
            vkDestroyDescriptorSetLayout(device, layout, alloc);
            m_layouts[i] = m_layouts.pop_back();
            m_layout_hashes[i] = m_layout_hashes.pop_back();
            m_ref_counts[i] = m_ref_counts.pop_back();
        }
    }
}


void
descriptor_set_layout_cache::destroy(
        VkDevice device,
        vk_alloc alloc) NEX
{
    for (auto layout: m_layouts)
        vkDestroyDescriptorSetLayout(device, layout, alloc);
    m_layouts.clear();
    m_layout_hashes.clear();
    m_ref_counts.clear();
}

//endregion

//region descriptor_builder

descriptor_set_builder&
descriptor_set_builder::bind_buffers(
        u32 binding,
        buffer_infos infos,
        descriptor_type_t type,
        shader_stage_t stages) NEX
{
    m_bindings.push_back({binding, type, u32(infos.size()), stages});
    descriptor_set::write_buffers(m_writes, binding, type, infos, 0);
    return *this;
}


descriptor_set_builder&
descriptor_set_builder::bind_images(
        u32 binding,
        image_infos infos,
        descriptor_type_t type,
        shader_stage_t stages) NEX
{
    m_bindings.push_back({binding, type, u32(infos.size()), stages});
    descriptor_set::write_images(m_writes, binding, type, infos, 0);
    return *this;
}


VkDescriptorPool
descriptor_set_builder::build(
        VkDevice device,
        descriptor_pool_allocator& pool_alloc,
        span<VkDescriptorSet> sets,
        descriptor_set_layout_cache* cache,
        ibool new_pool,
        vk_alloc alloc) NEX
{
    m_layout = cache
        ? cache->create(device, m_bindings, {}, alloc)
        : descriptor_set_layout::create(device, m_bindings, alloc);

    auto pool = pool_alloc.allocate(device, sets, m_layout, new_pool, alloc);
    if (!pool)
        return pool;

    for (const auto set: sets) {
        descriptor_set::write(device, m_writes, set);
    }

    return pool;
}

//endregion

//region descriptor_set_context

descriptor_set_context::
~descriptor_set_context() NEX
{
    for (auto& b: m_builders) {
        b.m_bindings.resize(0);
        b.m_writes.resize(0);
    }
}


span<VkDescriptorSet>
descriptor_set_context::physical(
        u32 set) NEX
{
    tassert(m_physical_count && "tinyvk::descriptor_set_context::physical - Must build context before accessing physical sets");
    tassert(set < m_builders.size() && "tinyvk::descriptor_set_context::physical - Set index out of range");
    tassert(((set + 1) * m_physical_count) <= m_physical.size() && "tinyvk::descriptor_set_context::physical - Not enough physical sets");
    return {&m_physical[set * m_physical_count], m_physical_count};
}


descriptor_set_builder&
descriptor_set_context::build(
        u32 set) NEX
{
    while (set >= m_builders.size())
        m_builders.push_back({});
    return m_builders[set];
}


void
descriptor_set_context::build(
        VkDevice device,
        descriptor_pool_allocator& pool_alloc,
        u32 physical_count,
        build_desc desc,
        vk_alloc alloc) NEX
{
    tassert(!m_physical_count && "tinyvk::descriptor_set_context::build - "
                                  "Already build context");

    tassert(physical_count && "tinyvk::descriptor_set_context::build - "
                               "Must create at least 1 physical descriptor set per logical descriptor set");

    m_physical_count = physical_count;

    // add previous bindings manually after existing bindings (skips descriptor writes for existing bindings)
    fixed_vector<u32, N> counts{};
    if (desc.previous) {
        for (u32 set = 0; set < desc.previous->m_builders.size(); ++set) {
            if (set > m_builders.size())
                m_builders.push_back({});

            u32 count{};
            for (auto& b: desc.previous->m_builders[set].m_bindings) {
                if (!desc.filter || desc.filter(desc.filter_userdata, b)) {
                    m_builders[set].m_bindings.push_back(b);
                    ++count;
                }
            }
            counts.push_back(count);
        }
    }

    // build sets
    m_physical.resize(physical_count * m_builders.size());
    for (u32 set = 0; set < m_builders.size(); ++set) {
        if (!m_builders[set].m_bindings.empty()) {
            auto pool = m_builders[set].build(device, pool_alloc, physical(set), desc.cache, false, alloc);
            tassert(pool && "tinyvk::descriptor_set_context::update - "
                            "Failed to allocate descriptors for descriptor_set_context");
        }
        else {
            tassert(desc.empty_set.set && desc.empty_set.layout && "tinyvk::descriptor_set_context::update - "
                     "If you bind non continuous sets you must provide a valid empty descriptor set to fill the gaps");
            m_builders[set].m_layout = descriptor_set_layout::from(desc.empty_set.layout);
            for(u32 i = 0; i < physical_count; ++i)
                m_physical.push_back(desc.empty_set.set);
        }
    }

    // copy existing bindings from previous sets
    if (desc.previous) {
        for (u32 set = 0; set < desc.previous->m_builders.size(); ++set) {

            small_vector<VkCopyDescriptorSet, 64> copies;

            for (u32 i = counts[set]; i < m_builders[set].m_bindings.size(); ++i) {
                auto& b = m_builders[set].m_bindings[i];
                const auto* src_sets = desc.previous->physical(set).data();
                const auto* dst_sets = physical(set).data();
                for (u32 j = 0; j < physical_count; ++j) {
                    VkCopyDescriptorSet c{VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET};
                    c.srcSet = src_sets[j];
                    c.dstSet = dst_sets[j];
                    c.srcBinding = b.binding;
                    c.dstBinding = b.binding;
                    c.descriptorCount = b.count;
                    c.srcArrayElement = 0;
                    c.dstArrayElement = 0;
                    copies.push_back(c);
                }
            }

            if (!copies.empty())
                vkUpdateDescriptorSets(device, 0, nullptr, u32(copies.size()), copies.data());
        }
    }
}

//endregion

}

#endif //TINYVK_DESCRIPTOR_CPP

#endif //TINYVK_IMPLEMENTATION
