//
// Created by Djordje on 8/22/2021.
//

#include "tinyvk_shader.h"
#include "tinystd_string.h"

#include <shaderc/shaderc.hpp>

namespace tinyvk {

shader_binary
compile_shader(
        shader_stage_t              stage,
        span<const char>            src_code,
        span<const shader_macro>    macros,
        shader_optimization_t       opt_level)
{
    shader_binary binary;

    shaderc_shader_kind kind{};
    if (stage == SHADER_VERTEX)     kind = shaderc_vertex_shader;
    if (stage == SHADER_FRAGMENT)   kind = shaderc_fragment_shader;
    if (stage == SHADER_COMPUTE)    kind = shaderc_compute_shader;
    if (stage == SHADER_GEOMETRY)   kind = shaderc_geometry_shader;
    if (stage == SHADER_TESS_CTRL)  kind = shaderc_tess_control_shader;
    if (stage == SHADER_TESS_EVAL)  kind = shaderc_tess_evaluation_shader;

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    for (auto& macro: macros)
        options.AddMacroDefinition(macro.define, macro.value);

    options.SetOptimizationLevel(shaderc_optimization_level(opt_level));

    auto module = compiler.CompileGlslToSpv(src_code.data(), kind, "tinyvk_compilation", options);

    if (module.GetCompilationStatus() == shaderc_compilation_status_success) {
        binary.resize(module.cend() - module.cbegin());
        tinystd::memcpy(binary.data(), module.cbegin(), binary.size() * sizeof(u32));
    }
    else {
        auto msg = module.GetErrorMessage();
        tinystd::error("tinyvk::compile_shader failed: %s\n", msg.c_str());
    }

    return binary;
}


//region glslangValidator

static void
remove_file(span<const char> file_name)
{
    tinystd::small_string<512 + 16> remove_cmd{};
    #ifdef _MSC_VER
    remove_cmd.append("del ", 4);
    #else
    remove_cmd.append("rm ", 3);
    #endif
    remove_cmd.append(file_name.data(), file_name.size());
    system(remove_cmd.data());
}


shader_binary
compile_shader_glslangvalidator(
        shader_stage_t              stage,
        span<const char>            src_code,
        span<const shader_macro>    macros,
        shader_optimization_t       opt_level)
{
    shader_binary binary;

    tinystd::small_string<512> input_name{};
    tmpnam_s<513>(input_name.m_data);
    input_name.append(input_name.end(), strlen(input_name.data()));
    if (stage == SHADER_VERTEX)     input_name.append(".vert", 5);
    if (stage == SHADER_FRAGMENT)   input_name.append(".frag", 5);
    if (stage == SHADER_COMPUTE)    input_name.append(".comp", 5);
    if (stage == SHADER_GEOMETRY)   input_name.append(".geom", 5);
    if (stage == SHADER_TESS_CTRL)  input_name.append(".tesc", 5);
    if (stage == SHADER_TESS_EVAL)  input_name.append(".tese", 5);

    tinystd::small_string<512> output_name{};
    output_name.append(input_name.data(), input_name.size());
    output_name.append(".bin", 4);

    FILE *file{};
    fopen_s(&file, input_name.data(), "w");
    if (!file)
        return binary;
    fwrite(src_code.data(), 1, src_code.size(), file);
    fclose(file);

    ibool success = compile_shader_glslangvalidator_files(stage, input_name, output_name, macros, opt_level);
    remove_file(input_name);

    size_t nread{};
    char buf[1024]{};
    fopen_s(&file, output_name.data(), "rb");
    if (file) {
        if (success) {
            while ((nread = fread(buf, 1, sizeof(buf), file)) > 0) {
                const size_t i = binary.size();
                binary.resize(binary.size() + nread/sizeof(u32));
                tinystd::memcpy(binary.data() + i, buf, nread);
            }
        }
        fclose(file);
        remove_file(output_name);
    }

    return binary;
}


ibool
compile_shader_glslangvalidator_files(
        shader_stage_t              stage,
        span<const char>            input_name,
        span<const char>            output_name,
        span<const shader_macro>    macros,
        shader_optimization_t       opt_level)
{
    tinystd::small_string<4096> cmd{};

    // glslangValidator path
    {
        static constexpr const char x[512]
        #ifdef _MSC_VER
            {VULKAN_SDK_PATH "/Bin/glslangValidator.exe"}
        #else
            {VULKAN_SDK_PATH "/bin/glslangValidator"}
        #endif
        ;
        cmd.append(x, strlen(x));
    }

    // macros
    {
       for (const auto& macro: macros) {
            cmd.append(" -D", 3);
            cmd.append(macro.define, strlen(macro.define));
            cmd.append("=\"", 2);
            cmd.append(macro.value, strlen(macro.value));
            cmd.append("\"", 1);
        }
    }

    tinystd::small_string<512> log_name{};
    log_name.append(output_name.data(), output_name.size());
    log_name.append(".log", 4);

    // command sprintf
    {
        tinystd::small_string<16> type{};
        switch(stage) {
            case SHADER_VERTEX:    type.append("vert", 4); break;
            case SHADER_FRAGMENT:  type.append("frag", 4); break;
            case SHADER_COMPUTE:   type.append("comp", 4); break;
            case SHADER_TESS_CTRL: type.append("tesc", 4); break;
            case SHADER_TESS_EVAL: type.append("tese", 4); break;
            default: tassert(false && "Unknown shader stage");
        }

        int rest = snprintf(cmd.end(), 4096, " -e %s %s -V %s -S %s -o %s > %s",
            "main",
            (opt_level == SHADER_OPTIMIZATION_SIZE) ? "-Os"
                : ((opt_level == SHADER_OPTIMIZATION_SPEED) ? "-Os" : "-g -Od"),
            input_name.data(),
            type.data(),
            output_name.data(),
            log_name.data());

        tassert(rest > 0 && "Failed to write formatted command");
        cmd.append(cmd.end(), cmd.size() + rest);
    }

    ibool success = system(cmd.data()) == 0;

    // print compile_log
    if (!success) {
        char buf[1024];
        FILE *file;
        size_t nread;
        fopen_s(&file, log_name.data(), "r");
        if (file) {
            while ((nread = fread(buf, 1, sizeof(buf), file)) > 0)
                fwrite(buf, 1, nread, stderr);
            fprintf(stderr, "\n");
            fclose(file);
        }
    }

    remove_file(log_name);

    return success;
}

//endregion

}
