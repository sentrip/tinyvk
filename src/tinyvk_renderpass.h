//
// Created by Djordje on 4/8/2022.
//

#include "tinyvk_core.h"

#ifndef TINYVK_RENDERPASS_H
#define TINYVK_RENDERPASS_H

namespace tinyvk {

/*
TODO: VkAttachmentDescription helpers
typedef struct VkAttachmentDescription {
    VkAttachmentDescriptionFlags    flags;
    VkFormat                        format;
    VkSampleCountFlagBits           samples;
    VkAttachmentLoadOp              loadOp;
    VkAttachmentStoreOp             storeOp;
    VkAttachmentLoadOp              stencilLoadOp;
    VkAttachmentStoreOp             stencilStoreOp;
    VkImageLayout                   initialLayout;
    VkImageLayout                   finalLayout;
} VkAttachmentDescription;

*/

/// renderpass low-level API

struct renderpass_desc {
    struct builder;

    span<const VkAttachmentDescription>   attach{};
    span<const VkSubpassDescription>      subpasses{};
    span<const VkSubpassDependency>       dependencies{};
};


struct framebuffer_desc {
    span<const VkImageView> attachments{};
    u32                     width{}, height{}, layers{1};
};


struct renderpass : type_wrapper<renderpass, VkRenderPass> {

    static renderpass               create(
            VkDevice                    device,
            renderpass_desc             desc,
            vk_alloc                    alloc = {}) NEX;

    void                            destroy(
            VkDevice                    device,
            vk_alloc                    alloc = {}) NEX;
};


struct framebuffer : type_wrapper<framebuffer, VkFramebuffer> {

    static framebuffer              create(
            VkDevice                    device,
            VkRenderPass                render_pass,
            framebuffer_desc            desc,
            vk_alloc                    alloc = {}) NEX;

    void                            destroy(
            VkDevice                    device,
            vk_alloc                    alloc = {}) NEX;
};


/// renderpass high-level API

struct renderpass_desc::builder {
    struct subpass_h;
    using  attach_ref = u32;

    builder() = default;

    /// 1. Attach resources to pass
    attach_ref          attach(const VkAttachmentDescription& desc) NEX;
    /// 2. create subpasses
    subpass_h           subpass(pipeline_type_t type = PIPELINE_GRAPHICS) NEX;
    /// 3. build description
    renderpass_desc     build() NEX;

    struct subpass_h {
        subpass_h() = default;
        /// 2.1 associate attachments to subpasses
        void            input(attach_ref i) NEX;
        void            output(attach_ref i) NEX;
        /// (optional) create explicit dependencies between subpasses
        void            depends_on(subpass_h& other) NEX;
        /// (optional) create explicit dependency to another renderpass
        void            depends_external(
                VkPipelineStageFlags    src_stage, VkAccessFlags src_access,
                VkPipelineStageFlags    dst_stage, VkAccessFlags dst_access) NEX;

    /// This are implementation details
    private:
        friend struct builder;
        explicit subpass_h(builder& b, u32 h) NEX : b{&b}, handle{h} {}
        builder* b{};
        u32 handle{};
    };

    /// This are implementation details
private:
    static constexpr size_t MAX_SUBPASSES       = MAX_RENDER_SUBPASSES;
    static constexpr size_t MAX_DEPENDENCIES    = MAX_RENDER_SUBPASS_DEPENDENCIES;
    static constexpr size_t MAX_ATTACH          = MAX_COLOR_ATTACHMENTS + 1;

    using subpass_attach                        = fixed_vector<VkAttachmentReference, MAX_COLOR_ATTACHMENTS>;

    struct subpass_info {
        fixed_vector<u8, MAX_COLOR_ATTACHMENTS>     inputs{};
        fixed_vector<u8, MAX_COLOR_ATTACHMENTS>     outputs{};
        fixed_vector<u8, MAX_SUBPASSES>             depends_on{};
        subpass_attach                              input_attach{};
        subpass_attach                              color_attach{};
        subpass_attach                              resolve_attach{};
        fixed_vector<u32, MAX_COLOR_ATTACHMENTS>    preserve_attach{};
        VkAttachmentReference                       depth_attach{-1u};
        u8                                          outputs_to_swapchain: 1;
        u8                                          uses_aliased_attachment: 1;
        u8                                          PAD: 6;
    };

    void bake() NEX;

    fixed_vector<VkAttachmentDescription, 2 * MAX_ATTACH>           attachments{};
    fixed_vector<fixed_vector<u8, MAX_SUBPASSES>, 2 * MAX_ATTACH>   readers{};
    fixed_vector<fixed_vector<u8, MAX_SUBPASSES>, 2 * MAX_ATTACH>   writers{};
    fixed_vector<VkSubpassDependency, MAX_DEPENDENCIES>             dependencies{};
    fixed_vector<VkSubpassDescription, MAX_SUBPASSES>               subpasses{};
    fixed_vector<subpass_info, MAX_SUBPASSES>                       subpass_infos{};
    bool                                                            baked{};
};


}

#endif //TINYVK_RENDERPASS_H

#ifdef TINYVK_IMPLEMENTATION

#ifndef TINYVK_RENDERPASS_CPP
#define TINYVK_RENDERPASS_CPP

#include "tinystd_algorithm.h"

namespace tinyvk {

//region renderpass

renderpass
renderpass::create(
        VkDevice device,
        renderpass_desc desc,
        vk_alloc alloc) NEX
{
    return renderpass{};
}

void
renderpass::destroy(
        VkDevice device,
        vk_alloc alloc) NEX
{
    vkDestroyRenderPass(device, vk, alloc);
    vk = {};
}

//endregion

//region framebuffer

framebuffer
framebuffer::create(
        VkDevice device,
        VkRenderPass render_pass,
        framebuffer_desc desc,
        vk_alloc alloc) NEX
{
    framebuffer fb{};
    VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    info.renderPass = render_pass;
    info.attachmentCount = desc.attachments.size();
    info.pAttachments = desc.attachments.data();
    info.width = desc.width;
    info.height = desc.height;
    info.layers = desc.layers;
    vk_validate(vkCreateFramebuffer(device, &info, alloc, &fb.vk),
        "tinyvk::framebuffer::create - Failed to create framebuffer");
    return fb;
}

void
framebuffer::destroy(
        VkDevice device,
        vk_alloc alloc) NEX
{
    vkDestroyFramebuffer(device, vk, alloc);
    vk = {};
}

//endregion

//region renderpass_desc::builder

static bool is_depth_layout(VkImageLayout layout)
{
    return layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        || layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        || layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
        || layout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
}

static VkAttachmentReference make_ref(u32 i, VkImageLayout layout) NEX
{
    return {i, layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : layout};
}

void renderpass_desc::builder::subpass_h::depends_on(subpass_h& other) NEX
{
    b->subpass_infos[handle].depends_on.push_back(u8(other.handle));
}

void renderpass_desc::builder::subpass_h::depends_external(VkPipelineStageFlags src_stage, VkAccessFlags src_access,
                                                           VkPipelineStageFlags dst_stage, VkAccessFlags dst_access) NEX
{
    VkSubpassDependency dep{};
    dep.srcSubpass          = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass          = handle;
    dep.srcStageMask        = src_stage;
    dep.srcAccessMask       = src_access;
    dep.dstStageMask        = dst_stage;
    dep.dstAccessMask       = dst_access;
    b->dependencies.push_back(dep);
}

void renderpass_desc::builder::subpass_h::input(attach_ref i) NEX
{
    b->readers[i].push_back(u8(handle));
    auto& si = b->subpass_infos[handle];
    si.inputs.push_back(u8(i));
    si.input_attach.push_back(make_ref(i, b->attachments[i].finalLayout));
    si.uses_aliased_attachment |= u8((b->attachments[i].flags & VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT) != 0);
}

void renderpass_desc::builder::subpass_h::output(attach_ref i) NEX
{
    auto& a = b->attachments[i];
    auto& si = b->subpass_infos[handle];
    b->writers[i].push_back(u8(handle));
    si.outputs.push_back(u8(i));
    si.outputs_to_swapchain |= u8(a.finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    si.uses_aliased_attachment |= u8((a.flags & VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT) != 0);

    auto ref = make_ref(i, a.finalLayout);
    if (is_depth_layout(a.finalLayout)) {
        if (si.depth_attach.attachment != -1u) {
            tinystd::error("Each subpass can only output to a single depth buffer");
            tinystd::exit(1);
        }
        si.depth_attach = ref;
    }
    else {
        si.color_attach.push_back(ref);
        if (a.samples != VK_SAMPLE_COUNT_1_BIT)
            si.resolve_attach.push_back(make_ref(i + 1, a.finalLayout));
    }
}

renderpass_desc::builder::attach_ref renderpass_desc::builder::attach(const VkAttachmentDescription& desc) NEX
{
    const u32 i = attachments.size();
    attachments.push_back(desc);
    readers.push_back({});
    writers.push_back({});
    if (desc.samples != VK_SAMPLE_COUNT_1_BIT) {
        attachments.push_back(desc);
        attachments.back().samples = VK_SAMPLE_COUNT_1_BIT;
        // TODO: Do resolve attachments require/allow different load/store parameters from the attachment they are resolving?
    }
    return i;
}

renderpass_desc::builder::subpass_h renderpass_desc::builder::subpass(pipeline_type_t type) NEX
{
    const u32 i = subpasses.size();
    subpasses.push_back({0u, VkPipelineBindPoint(type)});
    subpass_infos.push_back({});
    subpass_infos.back().outputs_to_swapchain = 0;
    subpass_infos.back().uses_aliased_attachment = 0;
    return subpass_h{*this, i};
}

renderpass_desc renderpass_desc::builder::build() NEX
{
    if (!baked) {
        bake();
        baked = true;
    }
    renderpass_desc desc{};
    desc.attach = attachments;
    desc.subpasses = subpasses;
    desc.dependencies = dependencies;
    return desc;
}

void renderpass_desc::builder::bake() NEX
{
    // swapchain dependencies
    for (u32 i = 0; i < subpasses.size(); ++i) {
        if (!subpass_infos[i].outputs_to_swapchain) continue;
        VkSubpassDependency dep{};
        dep.srcSubpass          = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass          = i;
        dep.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.srcAccessMask       = {};
        dep.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies.push_back(dep);
    }

    // explicit dependencies
    for (u32 i = 0; i < subpasses.size(); ++i) {
        auto& si = subpass_infos[i];
        for (auto depends_on: si.depends_on) {
            VkSubpassDependency dep{};
            dep.srcSubpass      = u32(depends_on);
            dep.dstSubpass      = i;
            dep.srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dep.srcAccessMask   = VK_ACCESS_MEMORY_WRITE_BIT;
            dep.dstStageMask    = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dep.dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
            dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            dependencies.push_back(dep);
        }
    }

    // preserve attachments and implicit subpass dependencies
    // for every resource R used in pass X, R must be preserve attachment in all passes:
    //      which do not read/write R and come after the last write to R and before the first read of R
    // for every resource R written to in pass X, R must have a read-after-write dependency in all passes:
    //      which read from R before the next write to R
    for (u32 i = 0; i < attachments.size(); ++i) {
        auto& w = writers[i];
        auto& r = readers[i];
        if (w.empty() || r.empty()) continue;

        u32 w_offset = 0;
        for (;;) {
            // Current write
            const auto* p_write = tinystd::min_element(w.begin() + w_offset, w.end());
            // No more writers, done with preserve attachments
            if (p_write == w.end()) break;
            w_offset = p_write - w.begin();
            const u8 write = *p_write;
            // Next write
            const auto* p_next_write = tinystd::min_element(w.begin() + w_offset, w.end());
            const u8 next_write = p_next_write != w.end() ? *p_next_write : subpasses.size();
            // Last read before next write
            const auto* p_last_read = tinystd::find_element(r.begin(), r.end(), [&](auto v, auto a){ return v > a && v < next_write; });
            // No more readers, continue to next write
            if (p_last_read == r.end()) continue;
            const u8 last_read = *p_last_read;

            // For each subpass after current write and before last read that does not use the attachment, add preserve attachment
            for (u8 s = write + 1; s < last_read; ++s) {
                auto& si = subpass_infos[s];
                if (tinystd::find(si.inputs.begin(), si.inputs.end(), u8(i)) == si.inputs.end())
                    subpass_infos[s].preserve_attach.push_back(i);
            }

            // For each subpass after current write and up to the last read that uses the attachment, add dependency
            for (u8 s = write + 1; s <= last_read; ++s) {
                auto& si = subpass_infos[s];
                if (tinystd::find(si.inputs.begin(), si.inputs.end(), u8(i)) != si.inputs.end()) {
                    const bool uses_alias = si.uses_aliased_attachment || subpass_infos[write].uses_aliased_attachment;
                    VkSubpassDependency dep{};
                    dep.srcSubpass      = u32(write);
                    dep.dstSubpass      = u32(s);
                    // TODO: tinyvk::renderpass_desc::builder::bake - verify implicit subpass dependencies color/depth read/write combinations
                    const bool is_depth = is_depth_layout(attachments[i].finalLayout);
                    dep.srcStageMask    = is_depth ? VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    dep.srcAccessMask   = is_depth ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    dep.dstStageMask    = is_depth ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    dep.dstAccessMask   = is_depth ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT : VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                    dep.dependencyFlags = uses_alias ? 0 : VK_DEPENDENCY_BY_REGION_BIT;
                    auto& dp = dependencies;
                    if (tinystd::find_if(dp.begin(), dp.end(), [&dep](auto& d){ return tinystd::memeq(&dep, &d, sizeof(dep)); }) != dp.end())
                        dp.push_back(dep);
                }
            }
        }
    }

    // fill in description structs
    for (u32 i = 0; i < subpasses.size(); ++i) {
        auto& s = subpasses[i];
        auto& si = subpass_infos[i];

        s.inputAttachmentCount = si.input_attach.size();
        s.pInputAttachments = si.input_attach.empty() ? nullptr : si.input_attach.data();

        s.colorAttachmentCount = si.color_attach.size();
        s.pColorAttachments = si.color_attach.empty() ? nullptr : si.color_attach.data();
        s.pResolveAttachments = si.resolve_attach.empty() ? nullptr : si.resolve_attach.data();

        s.pDepthStencilAttachment = si.depth_attach.attachment == -1u ? nullptr : &si.depth_attach;

        s.preserveAttachmentCount = si.preserve_attach.size();
        s.pPreserveAttachments = si.preserve_attach.empty() ? nullptr : si.preserve_attach.data();
    }
}

//endregion

}

#endif //TINYVK_RENDERPASS_CPP

#endif //TINYVK_IMPLEMENTATION
