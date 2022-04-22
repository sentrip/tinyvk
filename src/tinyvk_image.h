//
// Created by Djordje on 4/13/2022.
//

#ifndef TINYVK_IMAGE_H
#define TINYVK_IMAGE_H

#include "tinyvk_core.h"
#ifdef TINYVK_USE_VMA
#include "vk_mem_alloc.h"
#endif

namespace tinyvk {


enum image_usage_t: u32 {
    IMAGE_TRANSFER_SRC = 0x00000001,
    IMAGE_TRANSFER_DST = 0x00000002,
    IMAGE_SAMPLED = 0x00000004,
    IMAGE_STORAGE = 0x00000008,
    IMAGE_COLOR = 0x00000010,
    IMAGE_DEPTH_STENCIL = 0x00000020,
    IMAGE_TRANSIENT = 0x00000040,
    IMAGE_INPUT = 0x00000080,
    IMAGE_SHADING_RATE = 0x00000100,
    IMAGE_FRAGMENT_DENSITY_MAP = 0x00000200,
};


struct image_swizzle {
    static VkComponentMapping rgba() NEX;
};


struct image_desc {
    struct image_size {
        u32 width{}, height{1}, depth{1}, array_layers{1}, mip_levels{1};
    };

    image_size      size{};
    VkFormat        format{};
    image_usage_t   usage{IMAGE_COLOR};
    sample_count_t  samples{SAMPLE_COUNT_1};
    bool            cubemap{};
};


struct image_view_desc {
    VkComponentMapping  components{image_swizzle::rgba()};
    u32                 mip_level{0};
    u32                 level_count{VK_REMAINING_MIP_LEVELS};
    u32                 array_layer{0};
    u32                 layer_count{VK_REMAINING_ARRAY_LAYERS};
};


struct image_view : type_wrapper<image_view, VkImageView> {

    static image_view               create(
            VkDevice                    device,
            image_view_desc             desc,
            VkImage                     image,
            const image_memory&         mem,
            vk_alloc                    alloc = {}) NEX;

    void                            destroy(
            VkDevice                    device,
            vk_alloc                    alloc = {}) NEX;

    static VkImageViewType          determine_type(
            VkImageType                 type,
            u32                         array_layers,
            bool                        cubemap) NEX;
};


#ifdef TINYVK_USE_VMA

struct image_memory {
    u32                 width{};
    u32                 height{};
    u32                 depth{};
    u32                 array_layers{};
    u32                 is_cubemap: 1;
    u32                 mip_levels: 31;
    VkFormat            format{};
    VmaAllocation       vma_alloc{};
};


struct image : type_wrapper<image, VkImage> {

    static image                    create(
            VmaAllocator                vma,
            image_desc                  desc,
            image_memory&               mem,
            vk_alloc                    alloc = {}) NEX;

    void                            destroy(
            VmaAllocator                vma,
            image_memory&               mem,
            vk_alloc                    alloc = {}) NEX;

    image_view                      create_view(
            VkDevice                    device,
            image_view_desc             desc,
            const image_memory&         mem,
            vk_alloc                    alloc = {}) NEX;

    static VkImageAspectFlagBits    determine_aspect_mask(
            VkFormat                    format,
            bool                        stencil = false) NEX;
};

#else

#error tinyvk::image not supported without TINYVK_USE_VMA defined

#endif
}

#endif //TINYVK_IMAGE_H

#ifdef TINYVK_IMPLEMENTATION

#ifndef TINYVK_IMAGE_CPP
#define TINYVK_IMAGE_CPP

namespace tinyvk {

//region image

#ifdef TINYVK_USE_VMA

image
image::create(
        VmaAllocator vma,
        image_desc desc,
        image_memory& mem,
        vk_alloc alloc) NEX
{
    if (desc.size.width > 1 && desc.size.height > 1 && desc.size.depth > 1 && desc.size.array_layers > 1) {
        tinystd::error("tinyvk::image::create - 3D image arrays are not supported in Vulkan.");
        tinystd::exit(1);
    }

    image im{};

    const VkImageType image_type = desc.size.height == 1
            ? VK_IMAGE_TYPE_1D
            : (desc.size.depth == 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D);

    VkImageCreateInfo im_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    im_info.imageType = image_type;
    im_info.extent.width = u32(desc.size.width);
    im_info.extent.height = u32(desc.size.height);
    im_info.extent.depth = u32(desc.size.depth);
    im_info.mipLevels = u32(desc.size.mip_levels);
    im_info.arrayLayers = u32(desc.size.array_layers);
    im_info.format = desc.format;
    im_info.usage = VkImageUsageFlags(desc.usage);
    im_info.samples = VkSampleCountFlagBits(desc.samples);
    // TODO: Handle non-optimal images?
    im_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    im_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // TODO: Use initial layout for images?
    im_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (desc.cubemap)
        im_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    if (desc.size.array_layers > 1)
        im_info.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;

    VmaAllocationCreateInfo info{};
    // TODO: CPU-visible images?
    info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vk_validate(vmaCreateImage(vma, &im_info, &info, &im.vk, &mem.vma_alloc, nullptr),
        "tinyvk::image::create - failed to create image");

    mem.format = desc.format;
    mem.width = desc.size.width;
    mem.height = desc.size.height;
    mem.depth = desc.size.depth;
    mem.array_layers = desc.size.array_layers;
    mem.mip_levels = desc.size.mip_levels;
    mem.is_cubemap = desc.cubemap;

    return im;
}


void
image::destroy(
        VmaAllocator vma,
        image_memory& mem,
        vk_alloc alloc) NEX
{
    vmaDestroyImage(vma, vk, mem.vma_alloc);
    vk = {};
    mem = {};
}

#else

#endif

image_view
image::create_view(
        VkDevice device,
        image_view_desc desc,
        const image_memory& mem,
        vk_alloc alloc) NEX
{
    return image_view::create(device, desc, vk, mem, alloc);
}


VkImageAspectFlagBits
image::determine_aspect_mask(
        VkFormat format,
        bool stencil) NEX
{
    VkImageAspectFlags result{};
    switch (format) {
        // Depth
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT:
            result = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
            // Stencil
        case VK_FORMAT_S8_UINT:
            result = VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
            // Depth/stencil
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            result = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (stencil) result |= VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
            // Assume everything else is Color
        default:
            result = VK_IMAGE_ASPECT_COLOR_BIT;
            break;
    }
    return VkImageAspectFlagBits(result);
}

//endregion

//region image_view

image_view
image_view::create(
        VkDevice device,
        image_view_desc desc,
        VkImage image,
        const image_memory& mem,
        vk_alloc alloc) NEX
{
    image_view v;

    const VkImageType image_type = mem.height == 1
            ? VK_IMAGE_TYPE_1D
            : (mem.depth == 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D);

    VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.image = image;
    view_info.format = mem.format;
    view_info.components = desc.components;
    view_info.viewType = image_view::determine_type(image_type, mem.array_layers, bool(mem.is_cubemap));
    view_info.subresourceRange.aspectMask = image::determine_aspect_mask(mem.format, false);
    view_info.subresourceRange.baseMipLevel = desc.mip_level;
    view_info.subresourceRange.levelCount = desc.level_count;
    view_info.subresourceRange.baseArrayLayer = desc.array_layer;
    view_info.subresourceRange.layerCount = desc.layer_count;

    vk_validate(vkCreateImageView(device, &view_info, alloc, &v.vk),
              "tinyvk::image::create_view - failed to create image view");

    return v;
}


void
image_view::destroy(
        VkDevice device,
        vk_alloc alloc) NEX
{
    vkDestroyImageView(device, vk, alloc);
    vk = {};
}


VkImageViewType
image_view::determine_type(
        VkImageType type,
        u32 array_layers,
        bool cubemap) NEX
{
    if (type == VK_IMAGE_TYPE_1D) {
        return array_layers > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
    } else if (type == VK_IMAGE_TYPE_2D) {
        if (cubemap)
            return array_layers > 6 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
        else
            return array_layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    } else {
        return VK_IMAGE_VIEW_TYPE_3D;
    }
}

//endregion

//region image_swizzle

#define SW(X) VK_COMPONENT_SWIZZLE_##X

VkComponentMapping image_swizzle::rgba() NEX { return {SW(R), SW(G), SW(B), SW(A)}; }

#undef SW

//endregion

}

#endif //TINYVK_IMAGE_CPP

#endif //TINYVK_IMPLEMENTATION
