//
// Created by Djordje on 8/21/2021.
//

#ifndef TINYVK_QUEUE_H
#define TINYVK_QUEUE_H

#include "tinyvk_core.h"

namespace tinyvk {

using queue_index_t =
        tinystd::conditional_t<MAX_DEVICE_QUEUES < 256, u8, u16>;


enum queue_type_t {
    QUEUE_GRAPHICS,
    QUEUE_COMPUTE,
    QUEUE_TRANSFER,
    QUEUE_SPARSE,
    MAX_QUEUE_COUNT
};


struct queue_request {
    queue_type_t        type{};
    u32                 priority{};
    u32                 count{-1u};
    u32                 min_count{1};
    span<const float>   priorities{};
};


struct queue_family_properties: small_vector<VkQueueFamilyProperties> {
    bitset<MAX_DEVICE_QUEUES> supports_present{};

    queue_family_properties() = default;

    explicit queue_family_properties(
            VkPhysicalDevice                    physical_device,
            VkSurfaceKHR                        surface = {}) NEX;
};


struct queue_availability {
    u32 available[MAX_QUEUE_COUNT]{};
    u32 requested[MAX_QUEUE_COUNT]{};
    u8  dedicated_present{};

    queue_availability(
            span<const queue_request>           requests,
            span<const VkQueueFamilyProperties> props,
            ibool                               dedicated_present = false) NEX;

    static ibool test(
            const VkQueueFamilyProperties&      props,
            queue_type_t                        t) NEX;

    NDC ibool supports_requested_types(
            const VkQueueFamilyProperties&      props) const NEX;

    void consume(
            VkQueueFamilyProperties&            props,
            queue_type_t                        requested_type,
            u32                                 n = 1) NEX;
};


struct queue_create_info {
    struct info {
        u8              family{};
        u8              is_shared: 1;
        u8              support: 7;
        queue_index_t   index{};
        queue_index_t   physical_index{};
    };

    queue_index_t                           family_counts   [MAX_QUEUE_FAMILIES]{};
    fixed_vector<info, MAX_DEVICE_QUEUES>   queues          [MAX_QUEUE_COUNT]{};
    float                                   priorities      [MAX_QUEUE_FAMILIES][MAX_DEVICE_QUEUES]{};
    queue_index_t                           present_index{};

    explicit queue_create_info(
            span<const queue_request>           requests,
            VkPhysicalDevice                    physical_device,
            VkSurfaceKHR                        surface = {}) NEX;

    explicit queue_create_info(
            span<const queue_request>           requests,
            span<VkQueueFamilyProperties>       props,
            queue_availability&                 av) NEX;

    const info* find(
            queue_index_t                       physical_index,
            queue_type_t*                       p_type = {}) const NEX;

private:
    void get_dedicated(
        span<const u32> queue_order,
        span<VkQueueFamilyProperties> props,
        queue_availability& av,
        span<queue_index_t> original_family_counts) NEX;

    void get_shared(
        span<const u32> queue_order,
        span<const VkQueueFamilyProperties> props,
        const queue_availability& av,
        span<const queue_index_t> original_family_counts) NEX;
};


struct queue_collection {
    fixed_vector<VkQueue, MAX_DEVICE_QUEUES>    physical{};
    queue_index_t                               physical_index  [MAX_QUEUE_COUNT][MAX_DEVICE_QUEUES]{};
    bitset<MAX_DEVICE_QUEUES>                   is_shared       [MAX_QUEUE_COUNT]{};
    queue_index_t                               count           [MAX_QUEUE_COUNT]{};
    queue_index_t                               dedicated_count [MAX_QUEUE_COUNT]{};
    queue_index_t                               present_index{};

    queue_collection() = default;

    queue_collection(
            VkDevice                            device,
            span<const queue_request>           requests,
            const queue_create_info&            info) NEX;

    NDC VkQueue  present() const NEX;
    NDC VkQueue  get(queue_type_t type, u32 index = 0) const NEX;
    NDC ibool    shared(queue_type_t type, u32 index = 0) const NEX;
};

}

#endif //TINYVK_QUEUE_H

#ifdef TINYVK_IMPLEMENTATION

#ifndef TINYVK_QUEUE_CPP
#define TINYVK_QUEUE_CPP

#include "tinystd_algorithm.h"

namespace tinyvk {

static float fabs(float f) { return f > 0.0f ? f : (f < 0.0f ? -f : f); }

//region queue_family_properties

queue_family_properties::
queue_family_properties(
        VkPhysicalDevice physical_device, VkSurfaceKHR surface) NEX
{
    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, data());
    tassert(size() <= MAX_QUEUE_FAMILIES && "Too many queue families");

    if (surface) {
        for (u32 family = 0; family < queue_family_count; ++family) {
            VkBool32 present{};
            vk_validate(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, family, surface, &present),
                "tinyvk::queue_family_properties::queue_family_properties - Failed to get physical device surface support");
            supports_present.set(family, present > 0);
        }
    }
}

//endregion

//region queue_availability

ibool
queue_availability::test(
        const VkQueueFamilyProperties& props,
        queue_type_t t) NEX
{
    return u32(u32(props.queueFlags) & (1u << u32(t))) > 0;
}


ibool
queue_availability::supports_requested_types(
        const VkQueueFamilyProperties& props) const NEX
{
    ibool supports = false;
    for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type)
        supports |= u32(requested[type] > 0) * test(props, queue_type_t(type));
    return supports;
}


queue_availability::
queue_availability(
        span<const queue_request> requests,
        span<const VkQueueFamilyProperties> props,
        ibool dedicated_present) NEX :
    dedicated_present{u8(dedicated_present)}
{
    for (const auto& info: props) {
        for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type)
            available[type] += info.queueCount * test(info, queue_type_t(type));
    }

    for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type) {
        auto it = tinystd::find_if(requests.begin(), requests.end(), [=](auto& r){ return u32(r.type) == type; });
        if (it != requests.end()) {
            requested[type] += it->count == -1u ? available[type] : tinystd::min(available[type], it->min_count);

            // Attempt to request an additional graphics queue if dedicated present is requested
            if (type == QUEUE_GRAPHICS && dedicated_present && requested[type] < available[type])
                ++requested[type];

            if (requested[type] > available[type]) {
                tinystd::error("QueueType %u - Requested more queues (%u) than are available (%u)\n",
                                type, requested[type], available[type]);
                tinystd::exit(1);
            }
        }
    }

    for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type)
        requested[type] = tinystd::max(requested[type], u32(available[type] > 0));
}


void
queue_availability::consume(
        VkQueueFamilyProperties& props,
        queue_type_t requested_type,
        u32 n) NEX
{
    props.queueCount -= n;
    requested[u32(requested_type)] -= n;
    for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type) {
        const u32 expected = n * test(props, queue_type_t(type));
        tassert(available[type] >= expected && "Not enough available queues");
        available[type] -= expected;
    }
}

//endregion

//region queue_create_info

const queue_create_info::info*
queue_create_info::find(
        queue_index_t physical_index,
        queue_type_t* p_type) const NEX
{
    for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type) {
        for (const auto& info: queues[type]) {
            if (info.physical_index == physical_index) {
                if (p_type) *p_type = queue_type_t(type);
                return &info;
            }
        }
    }
    return nullptr;
}

queue_create_info::
queue_create_info(
        span<const queue_request> requests,
        VkPhysicalDevice physical_device,
        VkSurfaceKHR surface) NEX
{
    queue_family_properties props{physical_device, surface};
    queue_availability av{requests, props, surface != nullptr};
    *this = queue_create_info{requests, props, av};
}


queue_create_info::
queue_create_info(
        span<const queue_request> requests,
        span<VkQueueFamilyProperties> props,
        queue_availability& av) NEX
{
    u32 queue_order[MAX_QUEUE_COUNT]{};
    for (u32 i = 0; i < MAX_QUEUE_COUNT; ++i)
        queue_order[i] = i;

    queue_request to_sort[MAX_QUEUE_COUNT]{};
    for (const auto& r: requests)
        to_sort[r.type] = r;

    if(!requests.empty()) {
        for (const auto& r: requests) {
            tassert((
                r.priorities.empty()
                || r.priorities.size() == 1
                || r.priorities.size() == r.count
            ) && "tinyvk::queue_create_info::queue_create_info - Invalid queue priorities");
        }
        tinystd::sort(queue_order, queue_order + MAX_QUEUE_COUNT,
                      [&](u32 l, u32 r){ return to_sort[l].priority > to_sort[r].priority; });
    }

    queue_index_t original_family_counts[MAX_QUEUE_FAMILIES]{};
    get_dedicated(queue_order, props, av, original_family_counts);

    if (tinystd::any_of(av.requested, av.requested + MAX_QUEUE_COUNT, [](u32 v){ return v > 0; }))
        get_shared(queue_order, props, av, original_family_counts);

    // The present queue is always the last graphics queue
    if (av.dedicated_present)
        present_index = queues[QUEUE_GRAPHICS].back().physical_index;

    // Calculate priorities/support for each queue type
    for (u32 family = 0; family < props.size(); ++family) {
        const u8 support = u8(props[family].queueFlags);
        for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type) {
            auto& q = queues[type];
            for (auto& i: q)
                if (i.family == family)
                    i.support = support;

            auto* r = tinystd::find_if(requests.begin(), requests.end(), [=](auto& rq){ return rq.type == queue_type_t(type); });
            if (r != requests.end() && !r->priorities.empty()) {
                for (auto& i: q)
                    priorities[family][i.index] = tinystd::max(r->priorities[r->priorities.size() > 1 ? i.index : 0], priorities[family][i.index]);
            }
        }
    }
}


void
queue_create_info::get_dedicated(
        span<const u32> queue_order,
        span<VkQueueFamilyProperties> props,
        queue_availability& av,
        span<queue_index_t> original_family_counts) NEX
{
    queue_index_t queue_physical_index{};
    for (u32 family = 0; family < props.size(); ++family) {
        auto& info = props[family];
        original_family_counts[family] = info.queueCount;

        if (!av.supports_requested_types(info))
            continue;

        while (info.queueCount > 0 && av.supports_requested_types(info)) {
            for (const u32 type: queue_order) {
                if (av.requested[type] == 0)
                    continue;

                auto t = queue_type_t(type);
                if (queue_availability::test(info, t)) {
                    av.consume(info, t, 1);
                    queues[type].push_back({u8(family), false, u8(1u << t), family_counts[family]++, queue_physical_index++});
                    break;
                }
            }
        }
    }
}


void
queue_create_info::get_shared(
        span<const u32> queue_order,
        span<const VkQueueFamilyProperties> props,
        const queue_availability& av,
        span<const queue_index_t> original_family_counts) NEX
{
    u32 queue_type_count[MAX_QUEUE_FAMILIES]{};
    float queue_share_score[MAX_QUEUE_FAMILIES][MAX_QUEUE_COUNT]{};

    // Calculate queue sharing scores for family for each type
    for (u32 family = 0; family < props.size(); ++family) {
        for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type) {
            if (props[family].queueFlags & (1u << type)) {
                queue_share_score[family][type] += float(original_family_counts[family]);
                ++queue_type_count[family];
            }
        }
        for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type) {
            queue_share_score[family][type] /= float(queue_type_count[family]);
        }
    }

    // Use sharing scores to determine the best family to share from for each type
    // If two sharing scores are identical, the reverse queue order is used
    // to share queues with a lower priority before those with a higher priority
    u32 best_family_to_share[MAX_QUEUE_COUNT]{};
    for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type) {
        best_family_to_share[type] = type;

        float best_score = 0.0f;
        for (u32 family = 0; family < props.size(); ++family) {
            const float score = queue_share_score[family][type];
            if (score > best_score) {
                best_score = score;
                best_family_to_share[type] = family;
            }
            else if (fabs(score - best_score) <= 0.000001f) {
                for (auto it = queue_order.end(); it != queue_order.begin(); --it) {
                    const u32 t = *(it - 1);
                    const float sc = queue_share_score[family][t];
                    if (sc > best_score) {
                        best_score = sc;
                        best_family_to_share[t] = family;
                        break;
                    }
                }
            }
        }
    }

    // Get shared queue infos
    for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type) {
        if (!av.requested[type])
            continue;

        auto& infos = queues[type];
        const u32 family = best_family_to_share[type];
        queue_index_t physical_index{UINT8_MAX};
        for (auto& queue: queues) {
            for (auto& i: queue) {
                if (i.family == family) {
                    physical_index = i.physical_index;
                    break;
                }
            }
            if (physical_index != UINT8_MAX) break;
        }
        tassert(physical_index != UINT8_MAX && "Failed to find queue to share");
        infos.push_back({u8(family), true, u8(1u << type), 0, physical_index});
    }

    for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type) {
        for (auto& info: queues[type])
            info.is_shared |= (info.family == best_family_to_share[type]);
    }
}

//endregion

//region queue_collection

queue_collection::
queue_collection(
        VkDevice device,
        span<const queue_request> requests,
        const queue_create_info& info) NEX :
    present_index{info.present_index}
{
    struct FirstQueue { u32 type{}, index{}; };

    fixed_vector<u32, MAX_DEVICE_QUEUES> indices{};
    FirstQueue first_queue[MAX_DEVICE_QUEUES]{};

    // Get queues from create info
    for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type) {
        const auto& queue = info.queues[type];
        for (const auto& i: queue) {
            const u32 h = i.index | (i.family << 16);
            auto it = tinystd::find(indices.begin(), indices.end(), h);
            if (it == indices.end()) {
                indices.push_back(h);
                first_queue[physical.size()] = {type, i.index};
                physical.push_back({});
                vkGetDeviceQueue(device, i.family, i.index, &physical.back());
                physical_index[type][i.index] = i.physical_index;
            }
            else {
                physical_index[type][i.index] = *it;
                is_shared[type].set(i.index, true);
                auto& f = first_queue[*it];
                is_shared[f.type].set(f.index, true);
            }
            ++count[type];
        }
    }

    // Ensure all requested queues point to a physical queue
    for (const auto& request: requests) {
        if (request.count == -1u) continue;
        const u32 existing = info.queues[request.type].size();
        const u32 remaining = tinystd::max(request.count, existing) - existing;
        if (remaining > 0) {
            auto& f = first_queue[physical_index[request.type][existing - 1]];
            is_shared[f.type].set(f.index, true);
            for (u32 i = 0; i < remaining; ++i) {
                physical_index[request.type][existing + i] = physical_index[request.type][existing - 1];
                is_shared[request.type].set(existing + i, true);
                ++count[request.type];
            }
        }
    }

    // Count dedicated queues
    for (u32 type = 0; type < MAX_QUEUE_COUNT; ++type) {
        for (u32 i = 0; i < count[type]; ++i) {
            dedicated_count[type] += is_shared[type].test(i);
        }
    }
}


VkQueue
queue_collection::present(
        ) const NEX
{
    return physical[present_index];
}


VkQueue
queue_collection::get(
        queue_type_t type,
        u32 index) const NEX
{
    return physical[physical_index[type][index]];
}


ibool
queue_collection::shared(
        queue_type_t type,
        u32 index) const NEX
{
    return is_shared[type].test(index);
}

//endregion

}

#endif //TINYVK_QUEUE_CPP

#endif //TINYVK_IMPLEMENTATION
