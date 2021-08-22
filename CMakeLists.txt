cmake_minimum_required(VERSION 3.17)
project(tinyvk)

# OPTIONS
set(TINYVK_BUILD_TESTS      OFF     CACHE BOOL      "Build tests")
set(TINYVK_LINK_SHADERC     ON      CACHE BOOL      "Link Vulkan::shaderc glsl compiler")
set(TINYVK_BACKEND_TEST     OFF     CACHE BOOL      "Link test backend for vulkan functions")

message(STATUS "tinyvk: TESTS: ${TINYVK_BUILD_TESTS}, SHADERC: ${TINYVK_LINK_SHADERC}, BACKEND_TEST: ${TINYVK_BACKEND_TEST}")

# CMAKE SETUP
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/modules/")
include(tinyvk)


# TINYVK
add_library(tinyvk STATIC
    src/tinystd_assert.cpp
    src/tinystd_stdlib.cpp
    src/tinyvk_backend.cpp
    src/tinyvk_shader.cpp)
target_include_directories(tinyvk PUBLIC src)


# VULKAN
find_package(Vulkan REQUIRED)
target_include_directories(tinyvk PUBLIC $ENV{VULKAN_SDK}/include)
if (${TINYVK_LINK_SHADERC})
    tinyvk_link_shaderc(tinyvk)
endif()


# BACKEND
if (${TINYVK_BACKEND_TEST})
    target_compile_definitions(tinyvk PRIVATE TINYVK_BACKEND_TEST)
else()
    target_link_libraries(tinyvk PUBLIC Vulkan::Vulkan)
endif()


# TESTS
if (${TINYVK_BUILD_TESTS})
    add_subdirectory(tests)
endif()


# INSTALL
set_target_properties(tinyvk PROPERTIES PUBLIC_HEADER
    src/tinystd_algorithm.h
    src/tinystd_assert.h
    src/tinystd_bitset.h
    src/tinystd_config.h
    src/tinystd_span.h
    src/tinystd_stack_vector.h
    src/tinystd_stdlib.h
    src/tinystd_string.h
    src/tinystd_traits.h

    src/tinyvk_fwd.h
    src/tinyvk_core.h
    src/tinyvk_command.h
    src/tinyvk_device.h
    src/tinyvk_pipeline_cache.h
    src/tinyvk_queue.h
    src/tinyvk_shader.h
    src/tinyvk_swapchain.h

    src/vk_mem_alloc.h
    src/vk_mem_alloc.cpp
    )
include(GNUInstallDirs)
install(TARGETS tinyvk
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
