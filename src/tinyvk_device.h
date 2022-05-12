//
// Created by Djordje on 8/21/2021.
//

#ifndef TINYVK_DEVICE_H
#define TINYVK_DEVICE_H

#include "tinyvk_core.h"

namespace tinyvk {

enum version {
    VULKAN_1_0,
    VULKAN_1_1,
    VULKAN_1_2
};


enum device_features_t : u64 {
    FEATURE_ROBUST_BUFFER_ACCESS = 0x00000001,
    FEATURE_FULL_DRAW_INDEX_UINT32 = 0x00000002,
    FEATURE_INDEPENDENT_BLEND = 0x00000004,
    FEATURE_GEOMETRY_SHADER = 0x00000008,
    FEATURE_TESSELLATION_SHADER = 0x00000010,
    FEATURE_SAMPLE_RATE_SHADING = 0x00000020,
    FEATURE_MULTI_DRAW_INDIRECT = 0x00000040,
    FEATURE_DRAW_INDIRECT_FIRST_INSTANCE = 0x00000080,
    FEATURE_DEPTH_CLAMP = 0x00000100,
    FEATURE_DEPTH_BIAS_CLAMP = 0x00000200,
    FEATURE_FILL_MODE_NON_SOLID = 0x00000400,
    FEATURE_DEPTH_BOUNDS = 0x00000800,
    FEATURE_WIDE_LINES = 0x00001000,
    FEATURE_LARGE_POINTS = 0x00002000,
    FEATURE_MULTI_VIEWPORT = 0x00004000,
    FEATURE_SAMPLER_ANISOTROPY = 0x00008000,
    FEATURE_OCCLUSION_QUERY_PRECISE = 0x000100000,
    FEATURE_PIPELINE_STATISTICS_QUERY = 0x00200000,
    FEATURE_SHADER_UNIFORM_BUFFER_ARRAY_DYNAMIC_INDEXING = 0x00400000,
    FEATURE_SHADER_SAMPLED_IMAGE_ARRAY_DYNAMIC_INDEXING = 0x00800000,
    FEATURE_SHADER_STORAGE_BUFFER_ARRAY_DYNAMIC_INDEXING = 0x01000000,
    FEATURE_SHADER_STORAGE_IMAGE_ARRAY_DYNAMIC_INDEXING = 0x02000000,
    FEATURE_SHADER_CLIP_DISTANCE = 0x04000000,
    FEATURE_SHADER_CULL_DISTANCE = 0x08000000,
    FEATURE_SPARSE_BINDING = 0x10000000,
    FEATURE_SPARSE_RESIDENCY_BUFFER = 0x20000000,
    FEATURE_SPARSE_RESIDENCY_IMAGE2_D = 0x40000000,
    FEATURE_SPARSE_RESIDENCY_IMAGE3_D = 0x80000000,
    FEATURE_VERTEX_STORE_AND_ATOMIC = 0x0000000100000000ull,
    FEATURE_FRAGMENT_STORE_AND_ATOMIC = 0x0000000200000000ull,
};


enum validation_feature_enable_t: u32 {
    VALIDATION_FEATURE_ENABLE_GPU_ASSISTED = 1u << 0,
    VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT = 1u << 1,
    VALIDATION_FEATURE_ENABLE_BEST_PRACTICES = 1u << 2,
    VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF = 1u << 3,
    VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION = 1u << 4,
};


enum validation_feature_disable_t: u32 {
    VALIDATION_FEATURE_DISABLE_ALL = 1u << 0,
    VALIDATION_FEATURE_DISABLE_SHADERS = 1u << 1,
    VALIDATION_FEATURE_DISABLE_THREAD_SAFETY = 1u << 2,
    VALIDATION_FEATURE_DISABLE_API_PARAMETERS = 1u << 3,
    VALIDATION_FEATURE_DISABLE_OBJECT_LIFETIMES = 1u << 4,
    VALIDATION_FEATURE_DISABLE_CORE_CHECKS = 1u << 5,
    VALIDATION_FEATURE_DISABLE_UNIQUE_HANDLES = 1u << 6,
    VALIDATION_FEATURE_DISABLE_SHADER_VALIDATION_CACHE = 1u << 7,
};


using vk_debug_type_t = VkFlags;
enum debug_type_t {
    DEBUG_TYPE_GENERAL              = 0x1,
    DEBUG_TYPE_VALIDATION           = 0x2,
    DEBUG_TYPE_PERFORMANCE          = 0x4,
    DEBUG_TYPE_VALIDATION_PERF      = DEBUG_TYPE_VALIDATION | DEBUG_TYPE_PERFORMANCE,
    DEBUG_TYPE_ALL                  = DEBUG_TYPE_GENERAL | DEBUG_TYPE_VALIDATION_PERF
};


using vk_debug_severity_t = VkDebugUtilsMessageSeverityFlagBitsEXT;
enum debug_severity_t {
    DEBUG_SEVERITY_VERBOSE          = 0x0001,
    DEBUG_SEVERITY_INFO             = 0x0010,
    DEBUG_SEVERITY_WARNING          = 0x0100,
    DEBUG_SEVERITY_ERROR            = 0x1000,
    DEBUG_SEVERITY_WARNING_ABOVE    = DEBUG_SEVERITY_WARNING | DEBUG_SEVERITY_ERROR,
    DEBUG_SEVERITY_INFO_ABOVE       = DEBUG_SEVERITY_INFO | DEBUG_SEVERITY_WARNING_ABOVE,
    DEBUG_SEVERITY_INFO_BELOW       = DEBUG_SEVERITY_VERBOSE | DEBUG_SEVERITY_INFO,
    DEBUG_SEVERITY_WARNING_BELOW    = DEBUG_SEVERITY_INFO_BELOW | DEBUG_SEVERITY_WARNING,
    DEBUG_SEVERITY_ALL              = DEBUG_SEVERITY_WARNING_BELOW | DEBUG_SEVERITY_ERROR
};


struct extensions {
    span<const char* const> device{};
    span<const char* const> instance{};
    span<const char* const> validation_layers{};
    validation_feature_enable_t  validation_enable{};
    validation_feature_disable_t validation_disable{};

    NDC ibool device_extensions_supported(VkPhysicalDevice physical_device) const NEX;
    NDC ibool validation_layers_supported() const NEX;
};


struct application_info: VkApplicationInfo {
    explicit application_info(
            version                     ver = VULKAN_1_1,
            const char*                 app_name = "tinyvk",
            const char*                 engine_name = "tinyvk",
            u32                         app_version = VK_MAKE_VERSION(1, 0, 0),
            u32                         engine_version = VK_MAKE_VERSION(1, 0, 0)) NEX;

};


struct instance : type_wrapper<instance, VkInstance> {

    static instance create(
            application_info            app_info = application_info{},
            extensions                  ext = {},
            vk_alloc                    alloc = {}) NEX;

    void            destroy(
            vk_alloc                    alloc = {}) NEX;
};


struct physical_devices : fixed_vector<VkPhysicalDevice> {
    physical_devices() = default;

    using rating_callback = u32(void*, const VkPhysicalDeviceFeatures&, const VkPhysicalDeviceProperties&);

    explicit physical_devices(
            VkInstance                  instance) NEX;

    NDC VkPhysicalDevice pick_best(
            extensions                  ext,
            ibool                       use_gpu = true,
            VkSurfaceKHR                surface = {},
            rating_callback*            rating_func = {},
            void*                       rating_func_userdata = {}) const NEX;

    static VkFormat     find_supported_format(
            VkPhysicalDevice            physical_device,
            span<const VkFormat>        candidates,
            VkImageTiling               tiling,
            VkFormatFeatureFlags        features) NEX;
};


struct device : type_wrapper<device, VkDevice> {

    static device   create(
            VkInstance                  instance,
            VkPhysicalDevice            physical_device,
            const queue_create_info&    queue_info,
            extensions                  ext = {},
            device_features_t*          requested_features = {},
            VmaAllocator*               p_vma_alloc = {},
            vk_alloc                    alloc = {}) NEX;

    void            destroy(
            VmaAllocator*               p_vma_alloc = {},
            vk_alloc                    alloc = {}) NEX;

    /// Robust Pipeline Cache save/load
};


struct debug_messenger : type_wrapper<debug_messenger, VkDebugUtilsMessengerEXT> {

    using callback_data = const VkDebugUtilsMessengerCallbackDataEXT*;

    using callback      = u32(vk_debug_severity_t, vk_debug_type_t , callback_data, void*);

    static callback*    default_callback;

    static debug_messenger   create(
            VkInstance                  instance,
            debug_severity_t            severities = DEBUG_SEVERITY_INFO_ABOVE,
            debug_type_t                types = DEBUG_TYPE_VALIDATION_PERF,
            callback*                   callback = default_callback,
            void*                       userdata = {},
            vk_alloc                    alloc = {}) NEX;

    void                    destroy(
            VkInstance                  instance,
            vk_alloc                    alloc = {}) NEX;
};


}

#endif //TINYVK_DEVICE_H

#ifdef TINYVK_IMPLEMENTATION

#ifndef TINYVK_DEVICE_CPP
#define TINYVK_DEVICE_CPP

#include "tinystd_algorithm.h"
#include "tinystd_string.h"
#include "tinyvk_queue.h"
#include "tinyvk_swapchain.h"

#ifdef TINYVK_USE_VMA
#include "vk_mem_alloc.cpp"
#endif

namespace tinyvk {

#ifndef VK_API_VERSION_1_1
#define VK_API_VERSION_1_1 VK_API_VERSION_1_0
#endif

#ifndef VK_API_VERSION_1_2
#define VK_API_VERSION_1_2 VK_API_VERSION_1_1
#endif

template<typename Int, typename OnChar>
static void str_from_int(Int i, OnChar&& on_char)
{
    u64 multiple = 1000000000u;
    if (sizeof(Int) > 4) multiple = 10000000000000000000ull;
    while (multiple) {
        const Int m = i / Int(multiple);
        i -= m * multiple;
        on_char('0' + char(m));
        multiple /= 10u;
    }
}

//region application_info/extensions

application_info::
application_info(
        version ver,
        const char *app_name,
        const char *engine_name,
        u32 app_version,
        u32 engine_version) NEX :
    VkApplicationInfo{}
{
    sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    pApplicationName = app_name;
    pEngineName = engine_name;
    applicationVersion = app_version;
    engineVersion = engine_version;
    apiVersion = ver == VULKAN_1_2
            ? VK_API_VERSION_1_2
            : (ver == VULKAN_1_1
                ? VK_API_VERSION_1_1
                : VK_API_VERSION_1_0);
}


ibool
extensions::device_extensions_supported(
        VkPhysicalDevice physical_device) const NEX
{
    u32 extension_count{};
    vk_validate(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr),
        "tinyvk::extensions::device_extensions_supported - Failed to list device extensions");
    small_vector<VkExtensionProperties, 128> available_extensions{};
    available_extensions.resize(extension_count);
    vk_validate(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data()),
        "tinyvk::extensions::device_extensions_supported - Failed to acquire device extension info");

    for (const char* ext: device) {
        ibool found = false;
        for (const auto& available: available_extensions) {
            if (tinystd::streq(ext, available.extensionName)) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }

    return true;
}


ibool
extensions::validation_layers_supported() const NEX
{
    if (validation_layers.empty()) return true;

    u32 layer_count{};
    vk_validate(vkEnumerateInstanceLayerProperties(&layer_count, nullptr),
        "tinyvk::extensions::validation_layers_supported - Failed to list validation layers");
    small_vector<VkLayerProperties, 128> available_layers{};
    available_layers.resize(layer_count);
    vk_validate(vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data()),
        "tinyvk::extensions::validation_layers_supported - Failed to acquire validation layer info");

    for (const auto* layer: validation_layers) {
        ibool found = false;
        for (const auto &props: available_layers) {
            if (tinystd::streq(layer, props.layerName)) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }

    return true;
}

//endregion

//region instance

instance
instance::create(
        application_info app_info,
        extensions ext,
        vk_alloc alloc) NEX
{
    instance inst{};

    if (!ext.validation_layers_supported()) {
        tinystd::error("Unsupported validation layers\n");
        tinystd::exit(1);
    }

    VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = ext.instance.size();
    instance_info.ppEnabledExtensionNames = ext.instance.data();
    instance_info.enabledLayerCount = ext.validation_layers.size();
    instance_info.ppEnabledLayerNames = ext.validation_layers.data();

    VkValidationFeatureEnableEXT enables[8] = {};
    VkValidationFeatureDisableEXT disables[8] = {};
    VkValidationFeaturesEXT fts{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
    if (u32(ext.validation_enable) || u32(ext.validation_disable)) {
        u32 enable_count{}, disable_count{};
        for (u32 i = 0; i < 8; ++i)
            if (u32(ext.validation_enable) & (1u << i))
                enables[++enable_count] = VkValidationFeatureEnableEXT(i);
        for (u32 i = 0; i < 8; ++i)
            if (u32(ext.validation_disable) & (1u << i))
                disables[++disable_count] = VkValidationFeatureDisableEXT(i);
        fts.enabledValidationFeatureCount = enable_count;
        fts.disabledValidationFeatureCount = disable_count;
        fts.pEnabledValidationFeatures = enables;
        fts.pDisabledValidationFeatures = disables;
        instance_info.pNext = &fts;
    }

    vk_validate(vkCreateInstance(&instance_info, alloc, &inst.vk),
        "tinyvk::instance::create - Failed to create vulkan instance");

    return inst;
}

void
instance::destroy(
        vk_alloc alloc) NEX
{
    vkDestroyInstance(vk, alloc);
    vk = {};
}

//endregion

//region physical_devices

physical_devices::
physical_devices(
        VkInstance instance) NEX
{
    u32 device_count = 0;
    vk_validate(vkEnumeratePhysicalDevices(instance, &device_count, nullptr),
        "tinyvk::physical_devices::physical_devices - failed to list physical devices");
    tassert(device_count > 0 && "Failed to find a device with vulkan support");
    resize(device_count);
    vk_validate(vkEnumeratePhysicalDevices(instance, &device_count, data()),
        "tinyvk::physical_devices::physical_devices - failed to acquire physical device info");
}


VkPhysicalDevice
physical_devices::pick_best(
        extensions ext,
        ibool use_gpu,
        VkSurfaceKHR surface,
        rating_callback* rating_func,
        void* rating_func_userdata) const NEX
{
    u32 best_rating{};
    VkPhysicalDevice best_device{};

    VkPhysicalDeviceFeatures features{};
    VkPhysicalDeviceProperties properties{};
    for (auto device: *this) {
        if (!ext.device_extensions_supported(device))
            continue;

        if (surface) {
            swapchain_support support{device, surface};
            if (!support.adequate_for_present(device))
                continue;
        }

        vkGetPhysicalDeviceFeatures(device, &features);
        vkGetPhysicalDeviceProperties(device, &properties);

        u32 rating{1};
        if ((use_gpu && properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            || (!use_gpu && properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU))
        {
            rating += 1000;
        }

        if (rating_func)
            rating += rating_func(rating_func_userdata, features, properties);

        if (rating > best_rating) {
            best_rating = rating;
            best_device = device;
        }
    }

    tassert(best_device && "Failed to find suitable device");
    return best_device ? best_device : operator[](0);
}


VkFormat
physical_devices::find_supported_format(
        VkPhysicalDevice physical_device,
        span<const VkFormat> candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features) NEX
{
    for (const VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

        if ((tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            || (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features))
        {
            return format;
        }
    }
    return VK_FORMAT_UNDEFINED;
}

//endregion

//region device

static VkBool32&
vk_physical_device_feature(
        VkPhysicalDeviceFeatures& features,
        device_features_t requested_feature)
{
    switch (requested_feature) {
        case FEATURE_ROBUST_BUFFER_ACCESS: return features.robustBufferAccess;
        case FEATURE_FULL_DRAW_INDEX_UINT32: return features.fullDrawIndexUint32;
        case FEATURE_INDEPENDENT_BLEND: return features.independentBlend;
        case FEATURE_GEOMETRY_SHADER: return features.geometryShader;
        case FEATURE_TESSELLATION_SHADER: return features.tessellationShader;
        case FEATURE_SAMPLE_RATE_SHADING: return features.sampleRateShading;
        case FEATURE_MULTI_DRAW_INDIRECT: return features.multiDrawIndirect;
        case FEATURE_DRAW_INDIRECT_FIRST_INSTANCE: return features.drawIndirectFirstInstance;
        case FEATURE_DEPTH_CLAMP: return features.depthClamp;
        case FEATURE_DEPTH_BIAS_CLAMP: return features.depthBiasClamp;
        case FEATURE_FILL_MODE_NON_SOLID: return features.fillModeNonSolid;
        case FEATURE_DEPTH_BOUNDS: return features.depthBounds;
        case FEATURE_WIDE_LINES: return features.wideLines;
        case FEATURE_LARGE_POINTS: return features.largePoints;
        case FEATURE_MULTI_VIEWPORT: return features.multiViewport;
        case FEATURE_SAMPLER_ANISOTROPY: return features.samplerAnisotropy;
        case FEATURE_OCCLUSION_QUERY_PRECISE: return features.occlusionQueryPrecise;
        case FEATURE_PIPELINE_STATISTICS_QUERY: return features.pipelineStatisticsQuery;
        case FEATURE_VERTEX_STORE_AND_ATOMIC: return features.vertexPipelineStoresAndAtomics;
        case FEATURE_FRAGMENT_STORE_AND_ATOMIC: return features.fragmentStoresAndAtomics;
        case FEATURE_SHADER_UNIFORM_BUFFER_ARRAY_DYNAMIC_INDEXING: return features.shaderUniformBufferArrayDynamicIndexing;
        case FEATURE_SHADER_SAMPLED_IMAGE_ARRAY_DYNAMIC_INDEXING: return features.shaderSampledImageArrayDynamicIndexing;
        case FEATURE_SHADER_STORAGE_BUFFER_ARRAY_DYNAMIC_INDEXING: return features.shaderStorageBufferArrayDynamicIndexing;
        case FEATURE_SHADER_STORAGE_IMAGE_ARRAY_DYNAMIC_INDEXING: return features.shaderStorageImageArrayDynamicIndexing;
        case FEATURE_SHADER_CLIP_DISTANCE: return features.shaderClipDistance;
        case FEATURE_SHADER_CULL_DISTANCE: return features.shaderCullDistance;
        case FEATURE_SPARSE_BINDING: return features.sparseBinding;
        case FEATURE_SPARSE_RESIDENCY_BUFFER: return features.sparseResidencyBuffer;
        case FEATURE_SPARSE_RESIDENCY_IMAGE2_D: return features.sparseResidencyImage2D;
        case FEATURE_SPARSE_RESIDENCY_IMAGE3_D: return features.sparseResidencyImage3D;
    }
    tinystd::error("Unknown device feature: 0x%llx\n", u64(requested_feature));
    tinystd::exit(1);
    return features.multiDrawIndirect;
}


static VkPhysicalDeviceFeatures
supported_device_features(
        VkPhysicalDevice physical_device,
        device_features_t requested_features,
        device_features_t& supported_features)
{
    VkPhysicalDeviceFeatures requested{}, supported{};
    vkGetPhysicalDeviceFeatures(physical_device, &supported);

    for (u32 i = 0; i < 64; ++i) {
        auto rf = device_features_t(u64(1u << i));
        if ((u64(requested_features) & u64(rf)) != 0) {
            if (vk_physical_device_feature(supported, rf)) {
                vk_physical_device_feature(requested, rf) = true;
                supported_features = device_features_t(supported_features | rf);
            }
            else {
                tinystd::error("Requested unsupported device feature: 0x%llx\n", u64(rf));
            }
        }
    }

    return requested;
}


device
device::create(
        VkInstance instance,
        VkPhysicalDevice physical_device,
        const queue_create_info& queue_info,
        extensions ext,
        device_features_t* requested_features,
        VmaAllocator* p_vma_alloc,
        vk_alloc alloc) NEX
{
    device d{};

    device_features_t supported{}, requested{requested_features ? *requested_features : device_features_t()};
    auto features = supported_device_features(physical_device, requested, supported);
    if (requested_features) *requested_features = supported;

    fixed_vector<VkDeviceQueueCreateInfo, MAX_QUEUE_FAMILIES> queue_infos{};
    for (u32 family = 0; family < MAX_QUEUE_FAMILIES; ++family) {
        if (!queue_info.family_counts[family]) continue;
        queue_infos.push_back({VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO});
        auto& q = queue_infos.back();
        q.queueFamilyIndex = family;
        q.queueCount = u32(queue_info.family_counts[family]);
        q.pQueuePriorities = queue_info.priorities[family];
    }

    VkDeviceCreateInfo create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    create_info.queueCreateInfoCount = queue_infos.size();
    create_info.pQueueCreateInfos = queue_infos.data();
    create_info.pEnabledFeatures = &features;
    create_info.enabledExtensionCount = ext.device.size();
    create_info.ppEnabledExtensionNames = ext.device.data();
    create_info.enabledLayerCount = ext.validation_layers.size();
    create_info.ppEnabledLayerNames = ext.validation_layers.data();

    vk_validate(vkCreateDevice(physical_device, &create_info, alloc, &d.vk),
        "tinyvk::device::create - Failed to create logical device");

#ifdef TINYVK_USE_VMA
    if (p_vma_alloc) {
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.device = d.vk;
        allocatorInfo.instance = instance;
        allocatorInfo.physicalDevice = physical_device;
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_1;
        vk_validate(vmaCreateAllocator(&allocatorInfo, p_vma_alloc),
            "tinyvk::device::create - Failed to create VmaAllocator");
    }
#endif

    return d;
}

void
device::destroy(
        VmaAllocator* p_vma_alloc,
        vk_alloc alloc) NEX
{
#ifdef TINYVK_USE_VMA
    if (p_vma_alloc) {
        vmaDestroyAllocator(*p_vma_alloc);
    }
#endif
    vkDestroyDevice(vk, alloc);
    vk = {};
}

//endregion

//region debug_messenger

static ibool is_bugged_message(debug_messenger::callback_data data)
{
    /*** This seems to happen after creating render passes followed by reusing old (but still valid) render passes
    vkCmdDraw(): RenderPasses incompatible between active render pass w/ VkRenderPass 0x9fe8bd0000000038[] with a
        subpassCount of 1 and pipeline state object w/ VkRenderPass 0x9934aa000000004f[] with a subpassCount of 2.
        The Vulkan spec states: The current render pass must be compatible with the renderPass member of the
        VkGraphicsPipelineCreateInfo structure specified when creating the VkPipeline bound to VK_PIPELINE_BIND_POINT_GRAPHICS.
        https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-vkCmdDraw-renderPass-02684
    */
    if (data->pMessageIdName && tinystd::streq(data->pMessageIdName, "VUID-vkCmdDraw-renderPass-02684"))
        return true;
    return false;
}

debug_messenger
debug_messenger::create(
        VkInstance instance,
        debug_severity_t severities,
        debug_type_t types,
        callback *callback,
        void *userdata,
        vk_alloc alloc) NEX
{
    debug_messenger m{};

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func) {
        VkDebugUtilsMessengerCreateInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        info.pfnUserCallback = callback;
        info.pUserData = userdata;
        info.messageSeverity = severities;
        info.messageType = types;
        func(instance, &info, alloc, &m.vk);
    }

    return m;
}


void debug_messenger::destroy(
        VkInstance instance,
        vk_alloc alloc) NEX
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func) {
        func(instance, vk, alloc);
    }
}


debug_messenger::callback* debug_messenger::default_callback = [](
        vk_debug_severity_t severity,
        vk_debug_type_t type,
        debug_messenger::callback_data data,
        void*)
{
    if (data->pMessageIdName || data->pMessage) {
        if (is_bugged_message(data))
            return u32(0);

        auto print_func = u32(severity) >= u32(DEBUG_SEVERITY_WARNING) ? tinystd::error : tinystd::print;

        print_func("VULKAN (%d)", data->messageIdNumber);
        if (data->pMessageIdName)
            print_func(" - %s", data->pMessageIdName);

        if (data->pMessage)
            print_func(" \n-    %s", data->pMessage);

        print_func("\n");
    }

    return u32(0);
};

//endregion

}

#endif //TINYVK_DEVICE_CPP

#endif //TINYVK_IMPLEMENTATION
