//
// Created by Djordje on 8/21/2021.
//

#ifndef TINYVK_PIPELINE_CACHE_H
#define TINYVK_PIPELINE_CACHE_H

#include "tinystd_string.h"
#include "tinyvk_core.h"

namespace tinyvk {

/// Robust pipeline cache serialization - https://zeux.io/2019/07/17/serializing-pipeline-cache/
struct pipeline_cache_header {
    using path_t = tinystd::small_string<TINYVK_PIPELINE_CACHE_PATH_MAX_SIZE>;

    static constexpr u32 MAGIC          = 0xdeadbeef;

    u32 magic{};              // an arbitrary magic header to make sure this is actually our file
    u32 dataSize{};           // equal to *pDataSize returned by vkGetPipelineCacheData
    u64 dataHash{};           // a hash of pipeline cache data, including the header
    u32 vendorID{};           // equal to VkPhysicalDeviceProperties::vendorID
    u32 deviceID{};           // equal to VkPhysicalDeviceProperties::deviceID
    u32 driverVersion{};      // equal to VkPhysicalDeviceProperties::driverVersion
    u32 driverABI{};          // equal to sizeof(void*)
    u8 uuid[VK_UUID_SIZE]{};  // equal to VkPhysicalDeviceProperties::pipelineCacheUUID

    static pipeline_cache_header    create(
            VkPhysicalDevice                    physical_device,
            span<const u8>                      data) NEX;

    ibool                           valid(
            VkPhysicalDevice                    physical_device,
            span<const u8>                      data) const NEX;


    static pipeline_cache_header    create(
            const VkPhysicalDeviceProperties&   props,
            span<const u8>                      data) NEX;

    ibool                           valid(
            const VkPhysicalDeviceProperties&   props,
            span<const u8>                      data) const NEX;

    path_t                          path(
            span<const char>                    directory,
            span<const char>                    ext) const NEX;
};

}

#endif //TINYVK_PIPELINE_CACHE_H

#ifdef TINYVK_IMPLEMENTATION

#ifndef TINYVK_PIPELINE_CACHE_CPP
#define TINYVK_PIPELINE_CACHE_CPP

#include "tinystd_algorithm.h"

namespace tinyvk {

//region pipeline_cache_header

pipeline_cache_header
pipeline_cache_header::create(
            VkPhysicalDevice physical_device,
            span<const u8> data) NEX
{
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physical_device, &props);
    return create(props, data);
}

ibool
pipeline_cache_header::valid(
        VkPhysicalDevice physical_device,
        span<const u8> data) const NEX
{
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physical_device, &props);
    return valid(props, data);
}


pipeline_cache_header
pipeline_cache_header::create(
        const VkPhysicalDeviceProperties& props,
        span<const u8> data) NEX
{
    pipeline_cache_header header{};
    header.magic = MAGIC;
    header.driverABI = sizeof(void*);
    header.deviceID = props.deviceID;
    header.vendorID = props.vendorID;
    header.driverVersion = props.driverVersion;
    memcpy(header.uuid, props.pipelineCacheUUID, VK_UUID_SIZE);
    header.dataSize = data.size();
    header.dataHash = hash_crc32(data);
    return header;
}


ibool
pipeline_cache_header::valid(
        const VkPhysicalDeviceProperties& props,
        span<const u8> data) const NEX
{
    return (magic == MAGIC)
        && (driverABI == sizeof(void*))
        && (deviceID == props.deviceID)
        && (vendorID == props.vendorID)
        && (driverVersion == props.driverVersion)
        && tinystd::memeq(uuid, props.pipelineCacheUUID, VK_UUID_SIZE)
        && hash_crc32(data) == dataHash;
}


pipeline_cache_header::path_t
pipeline_cache_header::path(
        span<const char> directory,
        span<const char> ext) const NEX
{
    path_t path{};
    path.append(directory.data(), directory.size());
    const auto h = tinystd::hash_crc32({(const u8*)this, sizeof(pipeline_cache_header)});
    str_from_int(h, [&](char c){ path.append(&c, 1); });
    path.append(ext.data(), ext.size());
    return path;
}

//endregion

}

#endif //TINYVK_PIPELINE_CACHE_CPP

#endif //TINYVK_IMPLEMENTATION
