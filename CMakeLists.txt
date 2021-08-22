cmake_minimum_required(VERSION 3.17)
project(tinyvk)

set(CMAKE_CXX_STANDARD 14)

# TINYVK
add_library(tinyvk STATIC
    src/tinystd_assert.cpp
    src/tinystd_stdlib.cpp
    src/tinyvk_backend.cpp)

target_include_directories(tinyvk PUBLIC src)
target_include_directories(tinyvk PRIVATE external)


# VULKAN
find_package(Vulkan REQUIRED)
target_include_directories(tinyvk PUBLIC $ENV{VULKAN_SDK}/include)
target_link_libraries(tinyvk PUBLIC Vulkan::Vulkan)
#target_compile_definitions(tinyvk PRIVATE TINYVK_BACKEND_TEST)


# TINYVK_TESTS
add_executable(test_tinyvk
    tests/tests.cpp)

target_link_libraries(test_tinyvk PRIVATE tinyvk)
target_include_directories(test_tinyvk PRIVATE external)
#target_link_libraries(test_tinyvk PRIVATE SDL2d)
#target_include_directories(test_tinyvk PRIVATE "C:/Users/Djordje/Desktop/projects/vox/cmake-build-debug/src/external/SDL2-prefix/include")

if (WIN32)
#    target_link_libraries(test_tinyvk PRIVATE $ENV{VULKAN_SDK}/lib/shaderc_combined.lib)
#    target_link_libraries(test_tinyvk PRIVATE
#        shaderc
#        shaderc_util
#        glslang
#        HLSL
#        OGLCompiler
#        OSDependent
#        SPIRV
#        SPIRV-Tools
#        SPIRV-Tools-opt
#        )
else()
    target_link_libraries(test_tinyvk PRIVATE pthread $ENV{VULKAN_SDK}/lib/libshaderc_combined.a)
endif()
