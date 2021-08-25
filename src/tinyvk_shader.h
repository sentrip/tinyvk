//
// Created by Djordje on 8/22/2021.
//

#ifndef TINYVK_SHADER_H
#define TINYVK_SHADER_H

#include "tinyvk_core.h"

namespace tinyvk {

using shader_binary = small_vector<u32, 1024>;


enum shader_optimization_t {
    SHADER_OPTIMIZATION_NONE,
    SHADER_OPTIMIZATION_SIZE,
    SHADER_OPTIMIZATION_SPEED,
};


struct shader_macro {
    const char* define{}, *value{};
};


struct shader_module : type_wrapper<shader_module, VkShaderModule> {

    static shader_module        create(
            VkDevice                device,
            const shader_binary&    code,
            vk_alloc                alloc = {}) NEX;

    void                        destroy(
            VkDevice                device,
            vk_alloc                alloc = {}) NEX;
};


shader_binary
compile_shader(
        shader_stage_t              stage,
        span<const char>            src_code,
        span<const shader_macro>    macros = {},
        shader_optimization_t       opt_level = SHADER_OPTIMIZATION_NONE);



shader_binary
compile_shader_glslangvalidator(
        shader_stage_t              stage,
        span<const char>            src_code,
        span<const shader_macro>    macros = {},
        shader_optimization_t       opt_level = SHADER_OPTIMIZATION_NONE);



ibool
compile_shader_glslangvalidator_files(
        shader_stage_t              stage,
        span<const char>            input_name,
        span<const char>            output_name,
        span<const shader_macro>    macros = {},
        shader_optimization_t       opt_level = SHADER_OPTIMIZATION_NONE);

}

#endif //TINYVK_SHADER_H

#ifdef TINYVK_IMPLEMENTATION

#ifndef TINYVK_SHADER_CPP
#define TINYVK_SHADER_CPP

namespace tinyvk {

shader_module
shader_module::create(
        VkDevice device,
        const shader_binary& code,
        vk_alloc alloc) NEX
{
    shader_module m;
    VkShaderModuleCreateInfo create_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    create_info.codeSize = u32(code.size() * sizeof(u32));
    create_info.pCode = code.data();
    vk_validate(vkCreateShaderModule(device, &create_info, alloc, &m.vk),
        "tinyvk::shader_module::create - Failed to create shader module");
    return m;
}

void
shader_module::destroy(
        VkDevice device,
        vk_alloc alloc) NEX
{
    vkDestroyShaderModule(device, vk, alloc);
    vk = {};
}

}

#endif //TINYVK_SHADER_CPP

#endif
