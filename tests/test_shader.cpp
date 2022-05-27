//
// Created by Djordje on 8/22/2021.
//

#include "catch.hpp"

#include <cstring>

#include "tinyvk_shader.h"

static constexpr const char* SRC = R"(
#version 450
void main() {}
)";


TEST_CASE("Shader compilation - shaderc", "[tinyvk]")
{
    auto result = tinyvk::compile_shader(tinyvk::SHADER_COMPUTE, {SRC, strlen(SRC)});
    REQUIRE( !result.empty() );
}

TEST_CASE("Shader compilation - glslangValidator src", "[tinyvk]")
{
    auto result = tinyvk::compile_shader_glslangvalidator(tinyvk::SHADER_COMPUTE, {SRC, strlen(SRC)});
    REQUIRE( !result.empty() );
}


TEST_CASE("Shader compilation - glslangValidator - files", "[tinyvk]")
{
    static constexpr const char* INPUT = "test.comp";
    static constexpr const char* OUTPUT = "test.comp.bin";
    auto result = tinyvk::compile_shader_glslangvalidator_files(tinyvk::SHADER_COMPUTE,
        {INPUT, strlen(INPUT)}, {OUTPUT, strlen(OUTPUT)});
    REQUIRE( result );
}


static constexpr const char* MACRO_SRC = R"(#version 450
#define MACRO_FUNC(x, y) y y x x
#define MACRO_FUNC2(x, y) MACRO_FUNC(y, x)
MACRO_FUNC2(a, b))";

TEST_CASE("Shader preprocess - cpp", "[tinyvk]")
{
    tinystd::stack_vector<char, 1024> out;
    tinyvk::preprocess_shader_cpp({MACRO_SRC, strlen(MACRO_SRC)}, out, {});
    REQUIRE( memcmp(out.data(), "#version 450\na a b b", out.size()) == 0 );
}


static constexpr const char* DIRECTIVES_SRC = R"(#version 450
#extension STUFF : require
ab
#extension THINGS : enable
)";

TEST_CASE("Shader preprocess directives - cpp", "[tinyvk]")
{
    tinystd::stack_vector<char, 1024> out;
    tinyvk::preprocess_shader_cpp({DIRECTIVES_SRC, strlen(DIRECTIVES_SRC)}, out, {});
    REQUIRE( memcmp(out.data(), "#version 450\n#extension STUFF : require\n#extension THINGS : enable\nab", out.size()) == 0 );
}

static constexpr const char* CONSTANT_TO_SPEC_CONSTANT_SRC = R"(#version 450

layout(std430, binding = 0) buffer Buf { ivec4 data[]; } buf;

const ivec4 DATA[2] = ivec4[2](
    ivec4(1),
    ivec4(2)
);

void main() {
    buf.data[0] = DATA[0];
})";

TEST_CASE("Shader reflect convert constant array to specialization constant array", "[tinyvk]")
{
    auto binary = tinyvk::compile_shader_glslangvalidator(tinyvk::SHADER_COMPUTE, {CONSTANT_TO_SPEC_CONSTANT_SRC, strlen(CONSTANT_TO_SPEC_CONSTANT_SRC)});
    tinyvk::reflect_shader_convert_const_array_to_spec_const(binary);
}
