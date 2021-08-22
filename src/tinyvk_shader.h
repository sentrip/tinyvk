//
// Created by Djordje on 8/22/2021.
//

#ifndef TINYVK_SHADER_H
#define TINYVK_SHADER_H

#include "tinyvk_core.h"

namespace tinyvk {

using shader_binary = small_vector<u32, 1024>;


enum shader_stage_t {
    SHADER_VERTEX,
    SHADER_FRAGMENT,
    SHADER_COMPUTE,
    SHADER_GEOMETRY,
    SHADER_TESS_CTRL,
    SHADER_TESS_EVAL,
    MAX_SHADER_COUNT
};

enum shader_optimization_t {
    SHADER_OPTIMIZATION_NONE,
    SHADER_OPTIMIZATION_SIZE,
    SHADER_OPTIMIZATION_SPEED,
};


struct shader_macro {
    const char* define{}, *value{};
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
