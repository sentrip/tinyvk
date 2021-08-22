//
// Created by Djordje on 8/22/2021.
//

#include "catch.hpp"

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
