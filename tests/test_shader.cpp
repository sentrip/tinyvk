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
MACRO_FUNC2(a, b)
)";

TEST_CASE("Shader preprocess - cpp", "[tinyvk]")
{
    tinystd::stack_vector<char, 1024> out;
    tinyvk::preprocess_shader_cpp({MACRO_SRC, strlen(MACRO_SRC)}, out, {});
    REQUIRE( memcmp(out.data(), "#version 450\na a b b\n", out.size()) == 0 );
}
