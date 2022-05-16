//
// Created by jayjay on 25/4/22.
//

#ifndef TINYVK_PIPELINE_H
#define TINYVK_PIPELINE_H

#include "tinyvk_core.h"
#include "tinystd_stack_allocator.h"

namespace tinyvk {


enum topology_t {
    TOPOLOGY_POINT_LIST = 0,
    TOPOLOGY_LINE_LIST = 1,
    TOPOLOGY_LINE_STRIP = 2,
    TOPOLOGY_TRIANGLE_LIST = 3,
    TOPOLOGY_TRIANGLE_STRIP = 4,
    TOPOLOGY_TRIANGLE_FAN = 5,
    TOPOLOGY_LINE_LIST_WITH_ADJACENCY = 6,
    TOPOLOGY_LINE_STRIP_WITH_ADJACENCY = 7,
    TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY = 8,
    TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY = 9,
};


struct push_constant_range {
    u32                     offset{};
    u32                     size{};
    shader_stage_t          stages{SHADER_ALL};
};


struct pipeline_layout : type_wrapper<pipeline_layout, VkPipelineLayout> {
    using desc_set_layouts      = span<const VkDescriptorSetLayout>;
    using push_constant_ranges  = span<const push_constant_range>;

    static pipeline_layout  create(
            VkDevice                    device,
            desc_set_layouts            set_layouts,
            push_constant_ranges        push_constants,
            vk_alloc                    alloc = {}) NEX;

    void                    destroy(
            VkDevice                    device,
            vk_alloc                    alloc = {}) NEX;
};


struct pipeline : type_wrapper<pipeline, VkPipeline> {
    struct derived;
    struct graphics_desc;
    struct compute_desc;

    struct spec_info;
    struct spec_constant;

    struct blend_desc;
    struct depth_bias;
    struct depth_stencil;

    using desc_storage = tinystd::stack_allocator<2040>;
    template<size_t N> using spec_constants = spec_constant[N];
    template<size_t N> using storage_t = tinystd::stack_allocator<N>;

    static pipeline     create(
            VkDevice                    device,
            const graphics_desc&        desc,
            VkPipelineCache             cache = {},
            vk_alloc                    alloc = {}) NEX;

    static pipeline     create(
            VkDevice                    device,
            const compute_desc&         desc,
            VkPipelineCache             cache = {},
            vk_alloc                    alloc = {}) NEX;

    static void         create_graphics(
            VkDevice                    device,
            span<VkPipeline>            pipelines,
            span<const graphics_desc>   desc,
            VkPipelineCache             cache = {},
            vk_alloc                    alloc = {}) NEX;

    static void         create_compute(
            VkDevice                    device,
            span<VkPipeline>            pipelines,
            span<const compute_desc>    desc,
            VkPipelineCache             cache = {},
            vk_alloc                    alloc = {}) NEX;

    void                destroy(
            VkDevice                    device,
            vk_alloc                    alloc = {}) NEX;
};


/// pipeline description

//region shader/specialization

struct pipeline::spec_constant {
    u64         id;
    u64         is_inline: 1;
    u64         size: 31;
    const void* data{};

    spec_constant() : id{}, is_inline{}, size{}, data{} {}

    spec_constant(u32 id, const void* data, size_t size) : id{id}, is_inline{false}, size{u32(size)}, data{data} {}

    template<typename T>
    spec_constant(u32 id, const T& v)  : id{id}, is_inline{true}, size{sizeof(T)}, data{} {
        static_assert(sizeof(T) <= 8, "Specialization constant cannot fit in the storage of a pointer");
        memcpy(&data, &v, sizeof(T));
    }
};


struct pipeline::spec_info : VkSpecializationInfo {

    spec_info() : VkSpecializationInfo{} {}

    template<size_t NBytes>
    spec_info(
            storage_t<NBytes>&        storage,
            span<const spec_constant> constants) : spec_info{(storage_t<2048>&)storage, constants, 0}
    {}

    template<size_t NBytes, size_t N>
    spec_info(
            storage_t<NBytes>&        storage,
            const spec_constants<N>&  constants) : spec_info{(storage_t<2048>&)storage, {&constants[0], N}, 0}
    {}

    template<size_t NBytes>
    const spec_info* copy(storage_t<NBytes>& storage) const
    {
        auto* sp = storage.template construct<spec_info>(1);
        tinystd::memcpy(sp, this, sizeof(spec_info));
        sp->pData = sp->m_data;
        return sp;
    }

private:
    spec_info(
            storage_t<2048>&          storage,
            span<const spec_constant> constants,
            i32);

    u8                            m_data[64]{};
    size_t                        m_offset{};
};

//endregion


//region blend/depth descriptions

struct pipeline::blend_desc {
    enum Mask { RED = 1, GREEN = 2, BLUE = 4, ALPHA = 8, ALL = 15 };
    struct Mode {
        VkBlendFactor src{};
        VkBlendFactor dst{VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA};
        VkBlendOp     op{VK_BLEND_OP_ADD};
    };
    Mode    color{};
    Mode    alpha{color};
    Mask    mask{ALL};
};


struct pipeline::depth_bias {
    float constant{0.0f};
    float slope{0.0f};
    float clamp{0.0f};
};


struct pipeline::depth_stencil {
    enum  enable_t    { DISABLE = 0, READ = 1, WRITE = 2, STENCIL = 4, BOUNDS = 8 };
    using enable_flags = u32;

    VkStencilOp     fail{VK_STENCIL_OP_REPLACE};
    VkStencilOp     pass{VK_STENCIL_OP_REPLACE};
    VkStencilOp     depth_fail{VK_STENCIL_OP_REPLACE};
    VkCompareOp     compare_op{VK_COMPARE_OP_ALWAYS};
    u32             compare_mask{0xffffffffu};
    u32             write_mask {0xffffffffu};
};

//endregion


//region descriptions

struct pipeline::derived {
    VkPipeline                      pipeline{};
    i32                             index{-1};
    bool                            allow{true};
};


struct pipeline::graphics_desc : VkGraphicsPipelineCreateInfo {
    graphics_desc() : VkGraphicsPipelineCreateInfo{} {}

    graphics_desc(
            desc_storage&               storage,
            VkPipelineLayout            layout,
            VkRenderPass                render_pass,
            u32                         subpass,
            u32                         stage_count,
            u32                         vertex_binding_count,
            u32                         vertex_attribute_count,
            topology_t                  primitive_topology,
            derived                     base = {}) NEX;

    void add_stage(
            VkShaderModule              module,
            shader_stage_t              stage) NEX;

    void add_stage(
            desc_storage&               storage,
            VkShaderModule              module,
            shader_stage_t              stage,
            const spec_info&            spec) NEX;

    u32  add_vertex_binding(
            u32                         stride,
            bool                        per_instance = false) NEX;

    void add_vertex_attribute(
            u32                         binding,
            u32                         location,
            VkFormat                    format,
            u32                         offset = -1u) NEX;

    void add_dynamic_state(
            VkDynamicState              state) NEX;

    void enable_primitive_restart(
            ) NEX;

    void blending(
            desc_storage&               storage,
            span<const blend_desc>      attach,
            const float                 (&constants)[4] = {}) NEX;

    void depth(
            desc_storage&               storage,
            depth_stencil::enable_flags enable = depth_stencil::READ | depth_stencil::WRITE,
            VkCompareOp                 compare_op = VK_COMPARE_OP_LESS,
            depth_stencil               front = {},
            depth_stencil               back = {},
            float                       bounds_min = 0.0f,
            float                       bounds_max = 1.0f) NEX;

    void multisampling(
            ) NEX;

    void rasterizer(
            VkPolygonMode               polygon_mode = VK_POLYGON_MODE_FILL,
            VkCullModeFlags             cull_mode = VK_CULL_MODE_BACK_BIT,
            VkFrontFace                 front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            depth_bias                  bias = {},
            float                       line_width = 1.0f) NEX;

    void tesselation(
            desc_storage&               storage,
            u32                         patch_control_points) NEX;

    void viewport(
            desc_storage&               storage,
            const VkViewport&           viewport,
            const VkRect2D&             scissor) NEX;

    void viewport(
            desc_storage&               storage,
            span<const VkViewport>      viewports,
            span<const VkRect2D>        scissors) NEX;
};


struct pipeline::compute_desc : VkComputePipelineCreateInfo {
    compute_desc(): VkComputePipelineCreateInfo{} {}

    compute_desc(
            VkPipelineLayout            layout,
            VkShaderModule              shader,
            derived                     base = {}) NEX : compute_desc{nullptr, layout, shader, {}, base} {}

    template<size_t N>
    compute_desc(
            storage_t<N>&               storage,
            VkPipelineLayout            layout,
            VkShaderModule              shader,
            const spec_info&            spec,
            derived                     base = {}) NEX: compute_desc{(storage_t<2048>*)&storage, layout, shader, spec, base} {}

private:
    compute_desc(
            storage_t<2048>*            storage,
            VkPipelineLayout            layout,
            VkShaderModule              shader,
            const spec_info&            spec,
            derived                     base) NEX;
};

//endregion

}

#endif //TINYVK_PIPELINE_H

#ifdef TINYVK_IMPLEMENTATION

#ifndef TINYVK_PIPELINE_CPP
#define TINYVK_PIPELINE_CPP

#include "tinystd_assert.h"

namespace tinyvk {

//region pipeline layout

pipeline_layout
pipeline_layout::create(
        VkDevice device,
        desc_set_layouts set_layouts,
        push_constant_ranges push_constants,
        vk_alloc alloc) NEX
{
    static constexpr size_t MAX_PUSH_CONSTANTS = MAX_PUSH_CONSTANT_SIZE / sizeof(float);

    pipeline_layout l;
    VkPipelineLayoutCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    info.pSetLayouts = set_layouts.data();
    info.setLayoutCount = set_layouts.size();

    tassert(push_constants.size() < MAX_PUSH_CONSTANTS && "Too many push constants");
    u32 total_size = 0;
    for (auto& p: push_constants) total_size += p.size;
    tassert(total_size <= MAX_PUSH_CONSTANT_SIZE && "Push constants must be less than 128 bytes (or user defined max size)");

    VkPushConstantRange ranges[MAX_PUSH_CONSTANTS]{};
    info.pushConstantRangeCount = u32(push_constants.size());
    info.pPushConstantRanges = ranges;
    for (u32 i = 0; i < push_constants.size(); ++i) {
        ranges[i].stageFlags = (VkShaderStageFlags)push_constants[i].stages;
        ranges[i].offset = push_constants[i].offset;
        ranges[i].size = push_constants[i].size;
    }

    tinyvk::vk_validate(vkCreatePipelineLayout(device, &info, alloc, &l.vk),
        "tinyvk::pipeline_layout::create - Failed to create pipeline layout");

    return l;
}


void
pipeline_layout::destroy(
        VkDevice device,
        vk_alloc alloc) NEX
{
    vkDestroyPipelineLayout(device, vk, alloc);
    vk = {};
}

//endregion

//region pipeline

pipeline
pipeline::create(
        VkDevice device,
        const graphics_desc& desc,
        VkPipelineCache cache,
        vk_alloc alloc) NEX
{
    pipeline p;
    create_graphics(device, {&p.vk, 1}, {&desc, 1}, cache, alloc);
    return p;
}


pipeline
pipeline::create(
        VkDevice device,
        const compute_desc& desc,
        VkPipelineCache cache,
        vk_alloc alloc) NEX
{
    pipeline p;
    create_compute(device, {&p.vk, 1}, {&desc, 1}, cache, alloc);
    return p;
}


void
pipeline::create_graphics(
        VkDevice device,
        span<VkPipeline> pipelines,
        span<const graphics_desc> desc,
        VkPipelineCache cache,
        vk_alloc alloc) NEX
{
    tassert(pipelines.size() == desc.size() && "Must provide equal number of pipelines and descriptions");
    vk_validate(vkCreateGraphicsPipelines(device, cache, desc.size(), desc.data(), alloc, pipelines.data()),
        "tinyvk::pipeline::create_graphics - Failed to create pipelines");
}


void
pipeline::create_compute(
        VkDevice device,
        span<VkPipeline> pipelines,
        span<const compute_desc> desc,
        VkPipelineCache cache,
        vk_alloc alloc) NEX
{
    tassert(pipelines.size() == desc.size() && "Must provide equal number of pipelines and descriptions");
    vk_validate(vkCreateComputePipelines(device, cache, desc.size(), desc.data(), alloc, pipelines.data()),
        "tinyvk::pipeline::create_compute - Failed to create pipelines");
}


void
pipeline::destroy(
        VkDevice device,
        vk_alloc alloc) NEX
{
    vkDestroyPipeline(device, vk, alloc);
}

//endregion

//region pipeline parts

pipeline::spec_info::spec_info(
        storage_t<2048>& storage,
        span<const spec_constant> constants,
        i32)
: VkSpecializationInfo{}
{
    auto* e = storage.construct<VkSpecializationMapEntry>(constants.size());
    for (u32 i = 0; i < constants.size(); ++i) {
        auto& c = constants[i];
        e[i].constantID = c.id;
        e[i].offset = m_offset;
        e[i].size = c.size;
        tinystd::memcpy(m_data + m_offset, c.is_inline ? &c.data : c.data, c.size);
        m_offset += c.size;
        ++mapEntryCount;
    }
    dataSize = m_offset;
    pData = m_data;
    pMapEntries = e;
}

//endregion

//region pipeline compute

pipeline::compute_desc::compute_desc(
        storage_t<2048>* storage,
        VkPipelineLayout layout,
        VkShaderModule shader,
        const spec_info& spec,
        derived base) NEX
: VkComputePipelineCreateInfo{}
{
    sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    this->layout = layout;

    tassert((!base.pipeline || base.index == -1u) && "tinyvk::pipeline::compute - Cannot provide both base pipeline and base index");
    basePipelineHandle = base.pipeline;
    basePipelineIndex = base.index;
    if (base.allow)                        flags |= VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    if (base.pipeline || base.index != -1) flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;

    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.module = shader;
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.pName = "main";
    if (spec.pMapEntries)
        stage.pSpecializationInfo = spec.copy(*storage);
}

//endregion

//region pipeline graphics

pipeline::graphics_desc::graphics_desc(
        desc_storage& storage,
        VkPipelineLayout layout,
        VkRenderPass render_pass,
        u32 subpass,
        u32 stage_count,
        u32 vertex_binding_count,
        u32 vertex_attribute_count,
        topology_t primitive_topology,
        derived base) NEX
: VkGraphicsPipelineCreateInfo{}
{
    sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    this->layout = layout;

    tassert((!base.pipeline || base.index == -1u) && "tinyvk::pipeline::compute - Cannot provide both base pipeline and base index");
    basePipelineHandle = base.pipeline;
    basePipelineIndex = base.index;
    if (base.allow)                        flags |= VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    if (base.pipeline || base.index != -1) flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;

    renderPass = render_pass;
    this->subpass = subpass;
    auto* stages = storage.construct<VkPipelineShaderStageCreateInfo>(stage_count);
    stages->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pStages = stages;

    auto* vertex = storage.construct<VkPipelineVertexInputStateCreateInfo>(1);
    vertex->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex->pVertexBindingDescriptions = storage.construct<VkVertexInputBindingDescription>(vertex_binding_count);
    vertex->pVertexAttributeDescriptions = storage.construct<VkVertexInputAttributeDescription>(vertex_attribute_count);
    pVertexInputState = vertex;

    auto* dynamic_states = storage.construct<VkPipelineDynamicStateCreateInfo>(1);
    dynamic_states->sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_states->pDynamicStates = storage.construct<VkDynamicState>(16);
    pDynamicState = dynamic_states;

    auto* input_assembly = storage.construct<VkPipelineInputAssemblyStateCreateInfo>(1);
    input_assembly->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly->topology = VkPrimitiveTopology(primitive_topology);
    pInputAssemblyState = input_assembly;

    auto* multisample = storage.construct<VkPipelineMultisampleStateCreateInfo>(1);
    multisample->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample->minSampleShading = 0.0f;
    pMultisampleState = multisample;

    auto* raster = storage.construct<VkPipelineRasterizationStateCreateInfo>(1);
    raster->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster->depthClampEnable = false;
    raster->rasterizerDiscardEnable = false;
    raster->lineWidth = 1.0f;
    pRasterizationState = raster;

    auto* viewport = storage.construct<VkPipelineViewportStateCreateInfo>(1);
    viewport->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport->viewportCount = 1;
    viewport->scissorCount = 1;
    pViewportState = viewport;
}


void
pipeline::graphics_desc::add_stage(
        VkShaderModule module,
        shader_stage_t stage) NEX
{
    auto& st = const_cast<VkPipelineShaderStageCreateInfo&>(pStages[stageCount]);
    st = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    st.stage = VkShaderStageFlagBits(stage);
    st.module = module;
    st.pName = "main";
    stageCount++;
}


void
pipeline::graphics_desc::add_stage(
        desc_storage& storage,
        VkShaderModule module,
        shader_stage_t stage,
        const spec_info& spec) NEX
{
    auto& st = const_cast<VkPipelineShaderStageCreateInfo&>(pStages[stageCount]);
    add_stage(module, stage);
    st.pSpecializationInfo = spec.copy(storage);
}


u32
pipeline::graphics_desc::add_vertex_binding(
        u32 stride,
        bool per_instance) NEX
{
    auto* st = pVertexInputState;
    auto* ar = const_cast<VkVertexInputBindingDescription*>(st->pVertexBindingDescriptions);
    auto& b = ar[st->vertexBindingDescriptionCount];
    b.binding = st->vertexBindingDescriptionCount;
    b.stride = stride;
    b.inputRate = per_instance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
    return const_cast<VkPipelineVertexInputStateCreateInfo*>(st)->vertexBindingDescriptionCount++;
}


void
pipeline::graphics_desc::add_vertex_attribute(
        u32 binding,
        u32 location,
        VkFormat format,
        u32 offset) NEX
{
    auto* st = pVertexInputState;
    auto* ar = const_cast<VkVertexInputAttributeDescription*>(st->pVertexAttributeDescriptions);
    auto& a = ar[st->vertexAttributeDescriptionCount];
    a.binding = binding;
    a.location = location;
    a.format = format;
    if (offset != -1u) {
        a.offset = offset;
    } else {
        offset = 0;
        for (u32 i = 0; i < st->vertexAttributeDescriptionCount; ++i) {
            auto& d = st->pVertexAttributeDescriptions[i];
            if (d.binding == binding && d.location < location) {
                offset += vk_format_size_bytes(d.format);
            }
        }
    }
    const_cast<VkPipelineVertexInputStateCreateInfo*>(st)->vertexAttributeDescriptionCount++;
}


void
pipeline::graphics_desc::add_dynamic_state(
        VkDynamicState state) NEX
{
    auto* st = const_cast<VkDynamicState*>(pDynamicState->pDynamicStates);
    st[pDynamicState->dynamicStateCount] = state;
    const_cast<VkPipelineDynamicStateCreateInfo*>(pDynamicState)->dynamicStateCount++;
}


void
pipeline::graphics_desc::enable_primitive_restart(
        ) NEX
{
    const_cast<VkPipelineInputAssemblyStateCreateInfo*>(pInputAssemblyState)->primitiveRestartEnable = true;
}


void
pipeline::graphics_desc::blending(
        desc_storage& storage,
        span<const blend_desc> attach,
        const float (& constants)[4]) NEX
{
    if (pColorBlendState == nullptr) {
        const bool independent_blend = false; // TODO: independent blending of attachments in renderpass?
        assert(attach.size() <= renderpass_api_limits::MAX_COLOR_ATTACHMENTS && "Too many color attachments for blending");
        auto* st  = storage.construct<VkPipelineColorBlendStateCreateInfo>(1);
        st->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        st->logicOpEnable = false;
        st->logicOp = VK_LOGIC_OP_CLEAR;
        st->attachmentCount = attach.size();
        tinystd::memcpy(st->blendConstants, &constants[0], 4 * sizeof(float));

        if (!attach.empty()) {
            auto* at = storage.construct<VkPipelineColorBlendAttachmentState>(attach.size());
            st->pAttachments = at;
            for (u32 i = 0; i < attach.size(); ++i) {
                auto& a = at[i];
                auto& d = attach[independent_blend ? i : 0];
                a.colorWriteMask = (i32)d.mask;
                a.srcColorBlendFactor = d.color.src;
                a.dstColorBlendFactor = d.color.dst;
                a.colorBlendOp = d.color.op;
                a.srcAlphaBlendFactor = d.alpha.src;
                a.dstAlphaBlendFactor = d.alpha.dst;
                a.alphaBlendOp = d.alpha.op;
                a.blendEnable = (d.color.src != VK_BLEND_FACTOR_ONE ||
                                d.color.dst != VK_BLEND_FACTOR_ZERO ||
                                d.alpha.src != VK_BLEND_FACTOR_ONE ||
                                d.alpha.dst != VK_BLEND_FACTOR_ZERO);
            }
        }

        pColorBlendState = st;
    }
}


void
pipeline::graphics_desc::depth(
        desc_storage& storage,
        depth_stencil::enable_flags enable,
        VkCompareOp compare_op,
        depth_stencil front,
        depth_stencil back,
        float bounds_min,
        float bounds_max) NEX
{
    if (!pDepthStencilState) {
        auto* st = storage.construct<VkPipelineDepthStencilStateCreateInfo>(1);
        st->sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        st->depthTestEnable = (enable & depth_stencil::READ) != 0;
        st->depthWriteEnable = (enable & depth_stencil::WRITE) != 0;
        st->depthBoundsTestEnable = (enable & depth_stencil::BOUNDS) != 0;;
        st->stencilTestEnable = (enable & depth_stencil::STENCIL) != 0;
        st->depthCompareOp = compare_op;
        st->minDepthBounds = tinystd::max(0.0f, bounds_min);
        st->maxDepthBounds = tinystd::min(1.0f, bounds_max);

        st->front.failOp = front.fail;
        st->front.passOp = front.pass;
        st->front.depthFailOp = front.depth_fail;
        st->front.compareOp = front.compare_op;
        st->front.compareMask = front.compare_mask;
        st->front.writeMask = front.write_mask;
        st->front.reference = 0;

        st->back.failOp = back.fail;
        st->back.passOp = back.pass;
        st->back.depthFailOp = back.depth_fail;
        st->back.compareOp = back.compare_op;
        st->back.compareMask = back.compare_mask;
        st->back.writeMask = back.write_mask;
        st->back.reference = 0;

        // VUID-VkGraphicsPipelineCreateInfo-renderPass-06040
        if (enable == depth_stencil::READ) {
            st->front.failOp = VK_STENCIL_OP_KEEP;
            st->front.passOp = VK_STENCIL_OP_KEEP;
            st->front.depthFailOp = VK_STENCIL_OP_KEEP;
            st->back.failOp = VK_STENCIL_OP_KEEP;
            st->back.passOp = VK_STENCIL_OP_KEEP;
            st->back.depthFailOp = VK_STENCIL_OP_KEEP;
        }

        pDepthStencilState = st;
    }
}


void
pipeline::graphics_desc::multisampling(
        ) NEX
{}


void
pipeline::graphics_desc::rasterizer(
        VkPolygonMode polygon_mode,
        VkCullModeFlags cull_mode,
        VkFrontFace front_face,
        depth_bias bias,
        float line_width) NEX
{
    auto* st = const_cast<VkPipelineRasterizationStateCreateInfo*>(pRasterizationState);
    st->polygonMode = polygon_mode;
    st->cullMode = cull_mode;
    st->frontFace = front_face;
    st->depthBiasEnable = bias.constant != 0.0f;
    st->depthBiasConstantFactor = bias.constant;
    st->depthBiasClamp = bias.clamp;
    st->depthBiasSlopeFactor = bias.slope;
    st->lineWidth = line_width;
}


void
pipeline::graphics_desc::tesselation(
        desc_storage& storage,
        u32 patch_control_points) NEX
{
    if (pTessellationState == nullptr) {
        auto* tess = storage.construct<VkPipelineTessellationStateCreateInfo>(1);
        tess->sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        pTessellationState = tess;
    }
    const_cast<VkPipelineTessellationStateCreateInfo*>(pTessellationState)->patchControlPoints = patch_control_points;
}


void
pipeline::graphics_desc::viewport(
        desc_storage& storage,
        const VkViewport& viewport,
        const VkRect2D& scissor) NEX
{
    this->viewport(storage, {&viewport, 1}, {&scissor, 1});
}


void
pipeline::graphics_desc::viewport(
        desc_storage& storage,
        span<const VkViewport> viewports,
        span<const VkRect2D> scissors) NEX
{
    tassert(viewports.size() == scissors.size() && "Must provide equal number of viewports and scissors");
    const bool multiple_viewports = false; // TODO: allow multiple viewports in renderpass?
    tassert((multiple_viewports || viewports.size() == 1) && "Cannot use more than one viewport in a renderpass");
    if (pViewportState->pViewports == nullptr) {
        auto* vp = storage.construct<VkViewport>(viewports.size());
        memcpy(vp, viewports.data(), viewports.size() * sizeof(VkViewport));
        auto* sc = storage.construct<VkRect2D>(scissors.size());
        memcpy(sc, scissors.data(), scissors.size() * sizeof(VkRect2D));
        auto* st = const_cast<VkPipelineViewportStateCreateInfo*>(pViewportState);
        st->pViewports = vp;
        st->pScissors = sc;
        st->viewportCount = viewports.size();
        st->scissorCount = scissors.size();
    }
}

//endregion

}

#endif //TINYVK_PIPELINE_CPP

#endif //TINYVK_IMPLEMENTATION
