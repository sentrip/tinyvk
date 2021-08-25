cmake_minimum_required(VERSION 3.17)
project(tinyvk)

# OPTIONS
set(TINYVK_NO_ASSERT        OFF     CACHE BOOL      "Skip building and linking tinystd_assert.cpp")
set(TINYVK_NO_STDLIB        OFF     CACHE BOOL      "Skip building and linking tinystd_stdlib.cpp")
set(TINYVK_NO_SHADERC       OFF     CACHE BOOL      "Skip linking Vulkan::shaderc glsl compiler")
set(TINYVK_BACKEND_TEST     OFF     CACHE BOOL      "Link test backend for vulkan functions")
set(TINYVK_HEADER_ONLY      OFF     CACHE BOOL      "Install only header files")
set(TINYVK_BUILD_TESTS      ON      CACHE BOOL      "Build tests")


# CMAKE SETUP
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/modules/")
include(tinyvk)


# TINYVK
set(TINYVK_SRCS src/tinyvk_backend.cpp)

if (NOT ${TINYVK_NO_ASSERT})
    list(APPEND TINYVK_SRCS src/tinystd_assert.cpp)
endif()

if (NOT ${TINYVK_NO_STDLIB})
    list(APPEND TINYVK_SRCS src/tinystd_stdlib.cpp)
endif()

if (NOT ${TINYVK_NO_SHADERC})
    list(APPEND TINYVK_SRCS src/tinyvk_shader.cpp)
endif()

if (${TINYVK_HEADER_ONLY})
    set(TINYVK_PUBLIC INTERFACE)
    set(TINYVK_PRIVATE INTERFACE)
    add_library(tinyvk INTERFACE)
else()
    set(TINYVK_PUBLIC PUBLIC)
    set(TINYVK_PRIVATE PRIVATE)
    add_library(tinyvk STATIC ${TINYVK_SRCS})
endif()
target_include_directories(tinyvk
    ${TINYVK_PUBLIC} $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    ${TINYVK_PUBLIC} $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)


# VULKAN
find_package(Vulkan REQUIRED)
target_include_directories(tinyvk ${TINYVK_PUBLIC} $ENV{VULKAN_SDK}/include)
if (NOT ${TINYVK_HEADER_ONLY} AND NOT ${TINYVK_NO_SHADERC})
    tinyvk_link_shaderc(tinyvk)
endif()


# BACKEND
if (${TINYVK_BACKEND_TEST})
    target_compile_definitions(tinyvk ${TINYVK_PRIVATE} TINYVK_BACKEND_TEST)
else()
    target_link_libraries(tinyvk ${TINYVK_PUBLIC} Vulkan::Vulkan)
endif()


# TESTS
if (${TINYVK_BUILD_TESTS})
    add_subdirectory(tests)
endif()


# INSTALL
include(GNUInstallDirs)
install(
    DIRECTORY   ${CMAKE_CURRENT_SOURCE_DIR}/src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(
    TARGETS tinyvk
    EXPORT  tinyvk
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
