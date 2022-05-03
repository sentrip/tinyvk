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
struct default_renderpass_api_limits {
    static constexpr size_t MAX_COLOR_ATTACHMENTS = 8;
    static constexpr size_t MAX_RENDER_SUBPASSES = 16;
    static constexpr size_t MAX_RENDER_SUBPASS_DEPENDENCIES = 16;
};
using renderpass_api_limits = TINYVK_RENDERPASS_API_LIMITS;

enum order_t {
    ORDER_BEFORE,
    ORDER_AFTER
};


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
        void            output(attach_ref i, bool read = false) NEX;
        /// (optional) create explicit dependencies between subpasses
        void            depends_on(subpass_h& other) NEX;
        /// (optional) create explicit dependency to another renderpass
        void            depends_external(order_t order,
                VkPipelineStageFlags    src_stage, VkAccessFlags src_access,
                VkPipelineStageFlags    dst_stage, VkAccessFlags dst_access,
                VkDependencyFlags       dependencies = {}) NEX;

    /// This are implementation details
    private:
        friend struct builder;
        explicit subpass_h(builder& b, u32 h) NEX : b{&b}, handle{h} {}
        builder* b{};
        u32 handle{};
    };

    /// This are implementation details
private:
    static constexpr size_t MAX_SUBPASSES       = renderpass_api_limits::MAX_RENDER_SUBPASSES;
    static constexpr size_t MAX_DEPENDENCIES    = renderpass_api_limits::MAX_RENDER_SUBPASS_DEPENDENCIES;
    static constexpr size_t MAX_COLOR_ATTACH    = renderpass_api_limits::MAX_COLOR_ATTACHMENTS;
    static constexpr size_t MAX_ATTACH          = renderpass_api_limits::MAX_COLOR_ATTACHMENTS + 1;

    using subpass_attach                        = fixed_vector<VkAttachmentReference, MAX_COLOR_ATTACH>;

    struct write_hazard {
        enum Type: u8 { R_AFTER_W, W_AFTER_R, W_AFTER_W };
        Type    type{};
        u8      attach{};
        u8      src{};
        u8      dst{};
    };

    struct attach_info {
        u8 resolve{255};
        fixed_vector<u8, MAX_SUBPASSES>         readers{};
        fixed_vector<u8, MAX_SUBPASSES>         writers{};
    };

    struct subpass_info {
        fixed_vector<u8, MAX_COLOR_ATTACH>      inputs{};
        fixed_vector<u8, MAX_COLOR_ATTACH>      outputs{};
        subpass_attach                          input_attach{};
        subpass_attach                          color_attach{};
        subpass_attach                          resolve_attach{};
        fixed_vector<u32, MAX_COLOR_ATTACH>     preserve_attach{};
        VkAttachmentReference                   depth_attach{-1u};
        bitset<2*MAX_ATTACH>                    read_write{};
        u8                                      outputs_to_swapchain: 1;
        u8                                      uses_aliased_attachment: 1;
        u8                                      stage: 6;
    };

    void calculate_stages() NEX;
    void add_swapchain_dependencies() NEX;
    void add_explicit_dependencies() NEX;
    void add_implicit_dependencies() NEX;
    void add_preserve_attachments() NEX;
    void fill_descriptions() NEX;

    fixed_vector<VkAttachmentDescription, 2 * MAX_ATTACH>           attachments{};
    fixed_vector<VkSubpassDependency, MAX_DEPENDENCIES>             dependencies{};
    fixed_vector<VkSubpassDescription, MAX_SUBPASSES>               subpasses{};
    fixed_vector<write_hazard, 64>                                  hazards{};
    fixed_vector<attach_info,  MAX_ATTACH>                          attach_infos{};
    fixed_vector<subpass_info, MAX_SUBPASSES>                       subpass_infos{};
    fixed_vector<fixed_vector<u8, MAX_SUBPASSES>, MAX_SUBPASSES>    subpass_depends{};
    fixed_vector<fixed_vector<u8, MAX_SUBPASSES>, MAX_SUBPASSES>    stages{};
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
    renderpass r{};

    VkRenderPassCreateInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    info.pAttachments = desc.attach.data();
    info.attachmentCount = desc.attach.size();
    info.pDependencies = desc.dependencies.data();
    info.dependencyCount = desc.dependencies.size();
    info.pSubpasses = desc.subpasses.data();
    info.subpassCount = desc.subpasses.size();

    vk_validate(vkCreateRenderPass(device, &info, alloc, &r.vk),
        "tinyvk::renderpass::create - failed to create render pass");

    return r;
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

/** Using the same attachment as an input and an output in a single subpass
If a subpass uses the same attachment as both an input attachment and either a color attachment or a depth/stencil attachment,
writes via the color or depth/stencil attachment are not automatically made visible to reads via the input attachment,
causing a feedback loop, except in any of the following conditions:
    - If the color components or depth/stencil components read by the input attachment are mutually exclusive with
      the components written by the color or depth/stencil attachments, then there is no feedback loop. This requires the graphics pipelines
      used by the subpass to disable writes to color components that are read as inputs via the colorWriteEnable or colorWriteMask,
      and to disable writes to depth/stencil components that are read as inputs via depthWriteEnable or stencilTestEnable.
    - If the attachment is used as an input attachment and depth/stencil attachment only, and the depth/stencil attachment is not written to.

 */

static bool is_depth_layout(VkImageLayout layout)
{
    return layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        || layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        || layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
        || layout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
}

static VkAttachmentReference make_ref(u32 i, VkImageLayout layout, bool input) NEX
{
    if (input) {
        return {i, is_depth_layout(layout) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    } else {
        return {i, layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : layout};
    }
}

void renderpass_desc::builder::subpass_h::depends_on(subpass_h& other) NEX
{
    b->subpass_depends[handle].push_back(u8(other.handle));
}

void renderpass_desc::builder::subpass_h::depends_external(
        order_t order,
        VkPipelineStageFlags src_stage, VkAccessFlags src_access,
        VkPipelineStageFlags dst_stage, VkAccessFlags dst_access,
        VkDependencyFlags d) NEX
{
    VkSubpassDependency dep{};
    dep.srcSubpass          = order == ORDER_BEFORE ? VK_SUBPASS_EXTERNAL : handle;
    dep.dstSubpass          = order == ORDER_BEFORE ? handle : VK_SUBPASS_EXTERNAL;
    dep.srcStageMask        = src_stage;
    dep.srcAccessMask       = src_access;
    dep.dstStageMask        = dst_stage;
    dep.dstAccessMask       = dst_access;
    dep.dependencyFlags     = d;
    b->dependencies.push_back(dep);
}

void renderpass_desc::builder::subpass_h::input(attach_ref i) NEX
{
    b->attach_infos[i].readers.push_back(u8(handle));
    auto& si = b->subpass_infos[handle];
    si.inputs.push_back(u8(i));
    si.input_attach.push_back(make_ref(i, b->attachments[i].finalLayout, true));
    si.uses_aliased_attachment |= u8((b->attachments[i].flags & VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT) != 0);
}

void renderpass_desc::builder::subpass_h::output(attach_ref i, bool read) NEX
{
    auto& a = b->attachments[i];
    auto& si = b->subpass_infos[handle];
    b->attach_infos[i].writers.push_back(u8(handle));
    si.read_write.set(i, read);
    si.outputs.push_back(u8(i));
    si.outputs_to_swapchain |= u8(a.finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    si.uses_aliased_attachment |= u8((a.flags & VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT) != 0);

    auto ref = make_ref(i, a.finalLayout, false);
    if (is_depth_layout(a.finalLayout)) {
        if (si.depth_attach.attachment != -1u) {
            tinystd::error("Each subpass can only output to a single depth buffer");
            tinystd::exit(1);
        }
        si.depth_attach = ref;
    }
    else {
        si.color_attach.push_back(ref);
        // if the attachment is multi-sampled, add reference to the attachment that resolves the multisampled input
        if (a.samples != VK_SAMPLE_COUNT_1_BIT)
            si.resolve_attach.push_back(make_ref(i + 1, a.finalLayout, false));
        // if the subpass uses multi-sampled attachments then we need an unused attachment in the corresponding position
        else if (tinystd::any_of(si.outputs.begin(), si.outputs.end(), [&](auto at){ return b->attach_infos[at].resolve != 255; }))
            si.resolve_attach.push_back(make_ref(VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED, false));
    }
}

renderpass_desc::builder::attach_ref renderpass_desc::builder::attach(const VkAttachmentDescription& desc) NEX
{
    const u32 i = attachments.size();
    attachments.push_back(desc);
    attach_infos.push_back({});
    if (desc.samples != VK_SAMPLE_COUNT_1_BIT) {
        attach_infos[i].resolve = attachments.size();
        attachments.push_back(desc);
        attachments.back().samples = VK_SAMPLE_COUNT_1_BIT;
        attachments.back().loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
    return i;
}

renderpass_desc::builder::subpass_h renderpass_desc::builder::subpass(pipeline_type_t type) NEX
{
    const u32 i = subpasses.size();
    subpasses.push_back({0u, VkPipelineBindPoint(type)});
    subpass_infos.push_back({});
    subpass_depends.push_back({});
    subpass_infos.back().outputs_to_swapchain = 0;
    subpass_infos.back().uses_aliased_attachment = 0;
    return subpass_h{*this, i};
}

renderpass_desc renderpass_desc::builder::build() NEX
{
    if (!baked) {
        calculate_stages();
        add_swapchain_dependencies();
        add_explicit_dependencies();
        add_implicit_dependencies();
        add_preserve_attachments();
        fill_descriptions();
        baked = true;
    }
    renderpass_desc desc{};
    desc.attach = attachments;
    desc.subpasses = subpasses;
    desc.dependencies = dependencies;
    return desc;
}

void renderpass_desc::builder::calculate_stages() NEX
{
    // get initial stages from explicit dependencies
    fixed_vector<fixed_vector<u8, MAX_SUBPASSES>, MAX_SUBPASSES> explicit_stages{};
    tinystd::dependencies::calculate(subpass_depends, explicit_stages);
    for (u32 stage = 0; stage < explicit_stages.size(); ++stage) {
        for (auto s: explicit_stages[stage]) {
            subpass_infos[s].stage = stage;
        }
    }

    // calculate write hazards and modify initial stages
    u8 max_stage{};
    for (u32 a = 0; a < attachments.size(); ++a) {
        for (u32 s0 = 0; s0 < subpasses.size(); ++s0) {
            auto& sp0 = subpass_infos[s0];
            // subpass 0 read/write
            const bool s0_reads = tinystd::contains(sp0.inputs, u8(a));
            const bool s0_writes = tinystd::contains(sp0.outputs, u8(a));
            if (!s0_reads && !s0_writes)
                continue;

            for (u32 s1 = 0; s1 < subpasses.size(); ++s1) {
                auto& sp1 = subpass_infos[s1];
                if (s0 == s1)
                    continue; // assert same read/write properties?

                // subpass 1 read/write
                const bool s1_reads = tinystd::contains(sp1.inputs, u8(a));
                const bool s1_writes = tinystd::contains(sp1.outputs, u8(a));
                if (!s1_reads && !s1_writes)
                    continue;

                // if neither subpass writes then we do not need a dependency
                if (!s0_writes && !s1_writes)
                    continue;

                // ensure subpasses do not share stage
                if (sp0.stage == sp1.stage) {
                    const u8 st = ++((s0 < s1 ? sp1 : sp0).stage);
                    max_stage = tinystd::max(max_stage, st);
                }

                // create hazard
                write_hazard u{};
                u.attach = a;
                u.src = tinystd::min(s0, s1);
                u.dst = tinystd::max(s0, s1);
                bool reads [2]{ s0 < s1 ? s0_reads  : s1_reads , s0 < s1 ? s1_reads  : s0_reads };
                bool writes[2]{ s0 < s1 ? s0_writes : s1_writes, s0 < s1 ? s1_writes : s0_writes };
                if      (writes[0] && writes[1]) u.type = write_hazard::W_AFTER_W;
                else if (writes[0] &&  reads[1]) u.type = write_hazard::R_AFTER_W;
                else if ( reads[0] && writes[1]) u.type = write_hazard::W_AFTER_R;
                else                             tassert(false && "tinyvk::renderpass_desc::builder::build - Invalid usage");

                if (tinystd::find_if(hazards.begin(), hazards.end(),
                    [&u](auto& v){ return v.attach == u.attach && v.src == u.src && v.dst == u.dst; }) == hazards.end())
                {
                    hazards.push_back(u);
                }
            }
        }
    }

    // record final stages per subpass
    stages.resize(max_stage + 1);
    for (u32 i = 0; i < subpasses.size(); ++i)
        stages[subpass_infos[i].stage].push_back(i);
}

void renderpass_desc::builder::add_swapchain_dependencies() NEX
{
    for (u32 i = 0; i < subpasses.size(); ++i) {
        if (!subpass_infos[i].outputs_to_swapchain) continue;
        VkSubpassDependency dep{};
        dep.srcSubpass          = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass          = i;
        dep.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.srcAccessMask       = VK_ACCESS_MEMORY_READ_BIT;
        dep.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dep.dependencyFlags     = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies.push_back(dep);
        dep.srcSubpass          = i;
        dep.dstSubpass          = VK_SUBPASS_EXTERNAL;
        dep.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;;
        dep.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dep.dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT;
        dependencies.push_back(dep);
    }
}

void renderpass_desc::builder::add_explicit_dependencies() NEX
{
    for (u32 i = 0; i < subpasses.size(); ++i) {
        for (auto depends_on: subpass_depends[i]) {
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
}

void renderpass_desc::builder::add_implicit_dependencies() NEX
{
    for (auto& u: hazards) {
        const bool uses_alias = subpass_infos[u.src].uses_aliased_attachment || subpass_infos[u.dst].uses_aliased_attachment;
        const bool is_depth = is_depth_layout(attachments[u.attach].finalLayout);

        VkSubpassDependency dep{};
        dep.srcSubpass = u.src;
        dep.dstSubpass = u.dst;
        dep.dependencyFlags = uses_alias ? 0 : VK_DEPENDENCY_BY_REGION_BIT;

        const bool src_rw = subpass_infos[u.src].read_write.test(u.attach);
        const bool dst_rw = subpass_infos[u.dst].read_write.test(u.attach);

        auto color_stage        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        auto depth_read_stage   = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        auto depth_write_stage  = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        auto depth_rw_stage     = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        auto read_access        = VK_ACCESS_SHADER_READ_BIT;
        auto color_write_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        auto color_rw_access    = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        auto depth_write_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        auto depth_rw_access    = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        // TODO: tinyvk::renderpass_desc::builder::bake - verify implicit subpass dependencies color/depth read/write combinations
        switch (u.type) {
            case write_hazard::R_AFTER_W: {
                dep.srcStageMask    = is_depth ? (src_rw ? depth_rw_stage  : depth_write_stage)  : color_stage;
                dep.dstStageMask    = is_depth ? (dst_rw ? depth_rw_stage  : depth_read_stage)   : color_stage;
                dep.srcAccessMask   = is_depth ? (src_rw ? depth_rw_access : depth_write_access) : (src_rw ? color_rw_access : color_write_access);
                dep.dstAccessMask   = read_access;
                break;
            }
            case write_hazard::W_AFTER_R: {
                dep.srcStageMask    = is_depth ? (src_rw ? depth_rw_stage  : depth_read_stage)   : color_stage;
                dep.dstStageMask    = is_depth ? (dst_rw ? depth_rw_stage  : depth_write_stage)  : color_stage;
                dep.srcAccessMask   = read_access;
                dep.dstAccessMask   = is_depth ? (dst_rw ? depth_rw_access : depth_write_access) : (dst_rw ? color_rw_access : color_write_access);
                break;
            }
            case write_hazard::W_AFTER_W: {
                if (!dst_rw) continue;
                dep.srcStageMask    = is_depth ? (src_rw ? depth_rw_stage  : depth_write_stage)  : color_stage;
                dep.dstStageMask    = is_depth ? (dst_rw ? depth_rw_stage  : depth_write_stage)  : color_stage;
                dep.srcAccessMask   = is_depth ? (src_rw ? depth_rw_access : depth_write_access) : (src_rw ? color_rw_access : color_write_access);;
                dep.dstAccessMask   = is_depth ? (dst_rw ? depth_rw_access : depth_write_access) : (dst_rw ? color_rw_access : color_write_access);
                break;
            }
            default: tassert(false && "tinyvk::renderpass_desc::builder::build - Invalid usage"); break;
        }

        if (tinystd::find_if(dependencies.begin(), dependencies.end(),
            [&dep](auto& d){ return tinystd::memeq(&dep, &d, sizeof(dep)); }) == dependencies.end())
        {
            dependencies.push_back(dep);
        }
    }
}

void renderpass_desc::builder::add_preserve_attachments() NEX
{
    for (auto& u: hazards) {
        if (u.type != write_hazard::R_AFTER_W) continue;
        auto& src = subpass_infos[u.src];
        auto& dst = subpass_infos[u.dst];
        // TODO: Does just one subpass need to preserve per stage or is it all the subpasses in the stage that form part of the attachment's dependency chain
        // each attachment that is written in stage S and read in stage R must be preserved by
        // at least one subpass in every stage after S and before R
        for (u32 stage = src.stage + 1; stage <= dst.stage - (dst.stage > 0); ++stage) {
            for (auto s: stages[stage]) {
                auto& sp = subpass_infos[s];
                if (!tinystd::contains(sp.inputs, u.attach)
                    && !tinystd::contains(sp.outputs, u.attach)
                    && !tinystd::contains(sp.preserve_attach, u.attach))
                {
                    sp.preserve_attach.push_back(u.attach);
                    break;
                }
            }
        }
    }
}

void renderpass_desc::builder::fill_descriptions() NEX
{
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
