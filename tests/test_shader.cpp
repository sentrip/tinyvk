//
// Created by Djordje on 8/22/2021.
//

#include "catch.hpp"

#include <cstring>

#include "tinyvk_shader.h"
/*
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
*/
static constexpr const char* CONSTANT_TO_SPEC_CONSTANT_SRC = R"(#version 450

struct X { uint v[2]; };

const X DATA = X(uint[2](1,2));
layout(constant_id = 0) const uint DATA_ = 0;

layout(constant_id = 1) const uint IDX = 0;

void main() {
    uint x = DATA.v[IDX];
})";


static constexpr const char* CONSTANTS_SRC = R"(#version 450
struct Tree {
    uint log2dim[8];
    uint total_log2dim[8];
    uint levels;
};

const Tree trees[2] = {
    {
        {2,3,4,0,0,0,0,0},
        {2,5,9,0,0,0,0,0},
        3u
    },
    {
        {1,2,3,0,0,0,0,0},
        {1,3,6,0,0,0,0,0},
        3u
    }
};

void main()  {
    uint x = (1u << (3u * trees[0].log2dim[1])) - 1u;
    uint y = (1u << (3u * trees[1].log2dim[1])) - 1u;
    uint z = x + y;
}

)";

TEST_CASE("Shader reflect convert constant array to specialization constant array", "[tinyvk]")
{
    auto binary = tinyvk::compile_shader_glslangvalidator(tinyvk::SHADER_COMPUTE, {CONSTANTS_SRC, strlen(CONSTANTS_SRC)});

    FILE* f{};
    fopen_s(&f, "comp.spv", "wb");
    fwrite(binary.data(), 4u, binary.size(), f);
    fclose(f);

    system("C:\\VulkanSDK\\1.3.211.0\\Bin\\spirv-dis.exe comp.spv");
    system("del comp.spv");

//    printf("\n\n\n");
//    fflush(stdout);

//    const char* types[1]{"X"};
//    const char* names[1]{"DATA_"};
//    const auto size = tinyvk::reflect_shader_convert_consts_to_spec_consts(binary, {types, 1}, {names, 1});
//
//    fopen_s(&f, "comp.spv", "wb");
//    fwrite(binary.data(), 4u, size, f);
//    fclose(f);
//
//    system("C:\\VulkanSDK\\1.3.211.0\\Bin\\spirv-dis.exe comp.spv");
}
