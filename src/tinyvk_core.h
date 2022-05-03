//
// Created by Djordje on 8/20/2021.
//

#ifndef TINYVK_CORE_H
#define TINYVK_CORE_H

#include <vulkan/vulkan.h>

#include "tinyvk_fwd.h"
#include "tinystd_traits.h"
#include "tinystd_assert.h"

#ifndef TINYVK_BITSET
#include "tinystd_bitset.h"
#define TINYVK_BITSET(N)            tinystd::bitset<N>
#endif

#ifndef TINYVK_SPAN
#include "tinystd_span.h"
#define TINYVK_SPAN(T)              tinystd::span<T>
#endif

#ifndef TINYVK_VECTOR
#include "tinystd_stack_vector.h"
#define TINYVK_VECTOR(T, N)         tinystd::stack_vector<T, N>
#define TINYVK_FIXED_VECTOR(T, N)   tinystd::stack_vector<T, N>
#endif

#if !defined(TINYVK_PRINT) || !defined(TINYVK_ERROR)
#include "tinystd_stdlib.h"
#define TINYVK_PRINT(fmt, ...)      tinystd::print(fmt, __VA_ARGS__)
#define TINYVK_ERROR(fmt, ...)      tinystd::error(fmt, __VA_ARGS__)
#endif


#define NDC [[nodiscard]]
#define NEX noexcept

#ifndef TINYVK_MAX_QUEUE_FAMILIES
#define TINYVK_MAX_QUEUE_FAMILIES               8
#endif

#ifndef TINYVK_MAX_DEVICE_QUEUES
#define TINYVK_MAX_DEVICE_QUEUES                64
#endif

#ifndef TINYVK_MAX_SWAPCHAIN_IMAGES
#define TINYVK_MAX_SWAPCHAIN_IMAGES             8
#endif

#ifndef TINYVK_PIPELINE_CACHE_PATH_MAX_SIZE
#define TINYVK_PIPELINE_CACHE_PATH_MAX_SIZE     256
#endif

#ifndef TINYVK_DEFAULT_MAX_PUSH_CONSTANT_SIZE
#define TINYVK_DEFAULT_MAX_PUSH_CONSTANT_SIZE   128
#endif

#ifndef TINYVK_DEFAULT_TIMEOUT_NANOSECONDS
#define TINYVK_DEFAULT_TIMEOUT_NANOSECONDS      1000000000
#endif

#ifndef TINYVK_DESCRIPTOR_API_LIMITS
#define TINYVK_DESCRIPTOR_API_LIMITS        tinyvk::default_descriptor_api_limits
#endif

#ifndef TINYVK_RENDERPASS_API_LIMITS
#define TINYVK_RENDERPASS_API_LIMITS        tinyvk::default_renderpass_api_limits
#endif


#define DEFINE_ENUM_FLAG(ENUM_TYPE)                                                                        \
	static inline ENUM_TYPE operator|(ENUM_TYPE a, ENUM_TYPE b) { return (ENUM_TYPE)((uint32_t)(a) | (uint32_t)(b)); } \
	static inline ENUM_TYPE operator&(ENUM_TYPE a, ENUM_TYPE b) { return (ENUM_TYPE)((uint32_t)(a) & (uint32_t)(b)); } \
	static inline ENUM_TYPE operator|=(ENUM_TYPE& a, ENUM_TYPE b) { return a = (a | b); }                      \
	static inline ENUM_TYPE operator&=(ENUM_TYPE& a, ENUM_TYPE b) { return a = (a & b); }


namespace tinyvk {

enum {
    MAX_QUEUE_FAMILIES = TINYVK_MAX_QUEUE_FAMILIES,
    MAX_DEVICE_QUEUES = TINYVK_MAX_DEVICE_QUEUES,
    MAX_SWAPCHAIN_IMAGES = TINYVK_MAX_SWAPCHAIN_IMAGES,
    MAX_PUSH_CONSTANT_SIZE = TINYVK_DEFAULT_MAX_PUSH_CONSTANT_SIZE,
    DEFAULT_TIMEOUT_NANOS = TINYVK_DEFAULT_TIMEOUT_NANOSECONDS,
};

template<size_t N>
using bitset = TINYVK_BITSET(N);


template<typename T, size_t N = 8>
using small_vector = TINYVK_VECTOR(T, N);


template<typename T, size_t N = 8>
using fixed_vector = TINYVK_FIXED_VECTOR(T, N);


template<typename T>
using span = TINYVK_SPAN(T);


using tinystd::i8;
using tinystd::i16;
using tinystd::i32;
using tinystd::i64;
using tinystd::u8;
using tinystd::u16;
using tinystd::u32;
using tinystd::u64;
using tinystd::size_t;
using tinystd::ibool;


using vk_alloc = const VkAllocationCallbacks*;


enum image_layout_t: u32 {
    LAYOUT_UNDEFINED = 0,
    LAYOUT_GENERAL = 1,
    LAYOUT_COLOR = 2,
    LAYOUT_DEPTH_WRITE = 3,
    LAYOUT_DEPTH_READ = 4,
    LAYOUT_SHADER_READ = 5,
    LAYOUT_TRANSFER_SRC = 6,
    LAYOUT_TRANSFER_DST = 7,
    LAYOUT_PREINITIALIZED = 8,
    LAYOUT_DEPTH_READ_STENCIL_WRITE = 1000117000,
    LAYOUT_DEPTH_WRITE_STENCIL_READ = 1000117001,
    LAYOUT_PRESENT = 1000001002,
    LAYOUT_SHARED_PRESENT = 1000111000,
    LAYOUT_SHADING_RATE = 1000164003,
    LAYOUT_FRAGMENT_DENSITY_MAP = 1000218000,
};


enum pipeline_type_t: u8 {
    PIPELINE_GRAPHICS,
    PIPELINE_COMPUTE,
    PIPELINE_RAY_TRACING,
    MAX_PIPELINE_TYPE
};


enum sample_count_t {
    SAMPLE_COUNT_1 = 0x1,
    SAMPLE_COUNT_2 = 0x2,
    SAMPLE_COUNT_4 = 0x4,
    SAMPLE_COUNT_8 = 0x8,
    SAMPLE_COUNT_16 = 0x10,
    SAMPLE_COUNT_32 = 0x20,
    SAMPLE_COUNT_64 = 0x40,
};


enum shader_stage_t : u32 {
    SHADER_UNDEFINED = 0,
    SHADER_VERTEX = 0x00000001,
    SHADER_TESS_CTRL = 0x00000002,
    SHADER_TESS_EVAL = 0x00000004,
    SHADER_GEOMETRY = 0x00000008,
    SHADER_FRAGMENT = 0x00000010,
    SHADER_COMPUTE = 0x00000020,
    SHADER_ALL_GRAPHICS = 0x0000001F,
    SHADER_ALL = 0x7FFFFFFF,
    SHADER_RAYGEN = 0x00000100,
    SHADER_ANY_HIT = 0x00000200,
    SHADER_CLOSEST_HIT = 0x00000400,
    SHADER_MISS = 0x00000800,
    SHADER_INTERSECTION = 0x00001000,
    SHADER_CALLABLE = 0x00002000,
    SHADER_TASK = 0x00000040,
    SHADER_MESH = 0x00000080
};


/// see vk_mem_alloc.h for details
enum vma_usage_t {
    VMA_USAGE_UNKNOWN = 0,
    VMA_USAGE_GPU_ONLY = 1,
    VMA_USAGE_CPU_ONLY = 2,
    VMA_USAGE_CPU_TO_GPU = 3,
    VMA_USAGE_GPU_TO_CPU = 4,
    VMA_USAGE_CPU_COPY = 5,
};


/// see vk_mem_alloc.h for details
enum vma_create_t {
    VMA_CREATE_DEDICATED_MEMORY = 0x00000001,
    VMA_CREATE_MAPPED = 0x00000004,
    VMA_CREATE_STRATEGY_BEST_FIT  = 0x00010000,
    VMA_CREATE_STRATEGY_MIN_FRAGMENTATION = 0x00020000,
    VMA_CREATE_STRATEGY_FASTEST = 0x00040000,
};
DEFINE_ENUM_FLAG(vma_create_t)


template<typename Derived, typename VkType>
struct type_wrapper {
    VkType vk{};
    NDC operator VkType() const NEX { return vk; }
    static Derived from(VkType v) { Derived d{}; d.vk = v; return d; }
};


constexpr const char* vk_result_name(VkResult r)
{
    switch (r) {
        case VK_SUCCESS                         : return "VK_SUCCESS";
        case VK_NOT_READY                       : return "VK_NOT_READY";
        case VK_TIMEOUT                         : return "VK_TIMEOUT";
        case VK_EVENT_SET                       : return "VK_EVENT_SET";
        case VK_EVENT_RESET                     : return "VK_EVENT_RESET";
        case VK_INCOMPLETE                      : return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY        : return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY      : return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED     : return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST               : return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED         : return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT         : return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT     : return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT       : return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER       : return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS          : return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED      : return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL           : return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_OUT_OF_POOL_MEMORY        : return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE   : return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_SURFACE_LOST_KHR          : return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR  : return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR                  : return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR           : return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR  : return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT     : return "VK_ERROR_VALIDATION_FAILED_EXT";
        default                                 : return "VK_ERROR_UNKNOWN";
    }
    return "VK_ERROR_UNKNOWN";
}


template<typename... Args>
void vk_check(VkResult r, const char* msg, Args&&... args)
{
    if (r == VK_SUCCESS) return;
    char full_message[512]{"%s - "};
    for (size_t i = 0; i < 512 - 1; ++i) { if (!msg[i]) break; full_message[i + 5] = msg[i]; }
    tinystd::error(full_message, vk_result_name(r), tinystd::forward<Args>(args)...);
}


template<typename... Args>
void vk_validate(VkResult r, const char* msg, Args&&... args)
{
    if (r == VK_SUCCESS) return;
    vk_check<Args...>(r, msg, tinystd::forward<Args>(args)...);
    tinystd::exit(1);
}


namespace backend {

enum debug_t {
     device                 = 1u << 0,
     queue                  = 1u << 1,
     swapchain              = 1u << 2,
     shader                 = 1u << 3,
     pipeline               = 1u << 4,
     memory                 = 1u << 5,
     sampler                = 1u << 6,
     image                  = 1u << 7,
     buffer                 = 1u << 8,
     descriptor_pool        = 1u << 9,
     descriptor_set         = 1u << 10,
     descriptor_set_layout  = 1u << 11,
     renderpass             = 1u << 12,
     framebuffer            = 1u << 13,
     command_pool           = 1u << 14,
     command                = 1u << 15,
     all                    = 0xffff,
};
using debug_flags = u32;

void set_debug(debug_flags debug);


const VkInstanceCreateInfo&     get_desc(VkInstance v);
const VkDeviceCreateInfo&       get_desc(VkDevice v);
const VkRenderPassCreateInfo&   get_desc(VkRenderPass v);
const VkFramebufferCreateInfo&  get_desc(VkFramebuffer v);

}

}

#endif //TINYVK_CORE_H
