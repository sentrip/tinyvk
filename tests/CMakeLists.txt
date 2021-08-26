
add_executable(test_tinyvk
    tests.cpp
    test_device.cpp
    test_shader.cpp
    )

target_link_libraries(test_tinyvk PRIVATE tinyvk)
tinyvk_set_msvc_runtime_lib(test_tinyvk)

add_custom_command(TARGET test_tinyvk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/test.comp ${CMAKE_CURRENT_BINARY_DIR}/test.comp
)

#target_link_libraries(test_tinyvk PRIVATE SDL2d)
#target_include_directories(test_tinyvk PRIVATE "C:/Users/Djordje/Desktop/projects/vox/cmake-build-debug/src/external/SDL2-prefix/include")
