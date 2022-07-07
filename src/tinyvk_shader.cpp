//
// Created by Djordje on 8/22/2021.
//

#include "tinyvk_shader.h"
#include "tinystd_string.h"
#include "tinystd_algorithm.h"

#include <cstring>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>

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


static void
write_file(span<const char> file_name, span<const char> data)
{
    FILE *file = fopen(file_name.data(), "w");
    if (!data.empty()) fwrite(data.data(), 1, data.size(), file);
    fclose(file);
}

static void
print_log(span<const char> log_name)
{
    char buf[1024];
    FILE *file;
    size_t nread;
    file = fopen(log_name.data(), "r");
    if (file) {
        while ((nread = fread(buf, 1, sizeof(buf), file)) > 0)
            fwrite(buf, 1, nread, stderr);
        fprintf(stderr, "\n");
        fclose(file);
    }
}


//region glslangValidator

shader_binary
compile_shader_glslangvalidator(
        shader_stage_t              stage,
        span<const char>            src_code,
        span<const shader_macro>    macros,
        shader_optimization_t       opt_level)
{
    shader_binary binary;

    tinystd::small_string<512> input_name{};
    static tinystd::monotonic_increasing_string<16> tmp_name{};
    auto tmp = tmp_name.inc();
    input_name.append(tmp.data(), tmp.size());
    if (stage == SHADER_VERTEX)     input_name.append(".vert", 5);
    if (stage == SHADER_FRAGMENT)   input_name.append(".frag", 5);
    if (stage == SHADER_COMPUTE)    input_name.append(".comp", 5);
    if (stage == SHADER_GEOMETRY)   input_name.append(".geom", 5);
    if (stage == SHADER_TESS_CTRL)  input_name.append(".tesc", 5);
    if (stage == SHADER_TESS_EVAL)  input_name.append(".tese", 5);

    tinystd::small_string<512> output_name{};
    output_name.append(input_name.data(), input_name.size());
    output_name.append(".bin", 4);

    FILE *file = fopen(input_name.data(), "w");
    if (!file)
        return binary;
    fwrite(src_code.data(), 1, src_code.size(), file);
    fclose(file);

    ibool success = compile_shader_glslangvalidator_files(stage, input_name, output_name, macros, opt_level);
    remove_file(input_name);

    size_t nread{};
    char buf[1024]{};
    file = fopen(output_name.data(), "rb");
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

    if (!success)
        print_log(log_name);

    remove_file(log_name);

    return success;
}

//endregion

//region Preprocess

static span<const char> get_line(span<const char> data, size_t offset = 0)
{
    size_t line_end = offset;
    while (line_end < data.size() && data[line_end] != '\n' && data[line_end] != '\r') ++line_end;
    return {data.data() + offset, line_end - offset};
}

template<size_t N, size_t NP>
static void shader_get_extensions(
    span<const char>            src_code,
    small_vector<char, N>&      header,
    small_vector<char, NP>&     to_preprocess)
{
    u32 line_begin = 0;
    while (line_begin < src_code.size()) {
        auto line = get_line(src_code, line_begin);
        line_begin = line.end() - src_code.data();
        if (memcmp(line.data(), "#version", 8) == 0) {
            header.resize(header.size() + line.size() + 1);
            memcpy(header.end() - line.size() - 1, line.data(), line.size());
            header.back() = '\n';
        } else if (memcmp(line.data(), "#extension", 10) == 0) {
            header.resize(header.size() + line.size() + 1);
            memcpy(header.end() - line.size() - 1, line.data(), line.size());
            header.back() = '\n';

            to_preprocess.resize(to_preprocess.size() + line.size() + 1);
            memcpy(to_preprocess.end() - line.size() - 1, "#define   ", 10);
            memcpy(to_preprocess.end() - line.size() - 1 + 10, line.data() + 10, line.size() + 10);
            to_preprocess.back() = '\n';
        } else {
            to_preprocess.resize(to_preprocess.size() + line.size() + 1);
            memcpy(to_preprocess.end() - line.size() - 1, line.data(), line.size());
            to_preprocess.back() = '\n';
        }
        while (src_code[line_begin] == '\n' || src_code[line_begin] == '\r') line_begin++;
    }
}


ibool preprocess_shader_cpp(
    span<const char>            src_code,
    small_vector<char, 1024>&   output,
    span<const shader_macro>    macros)
{
    tinystd::small_string<4096> cmd{};

    // cpp path
    {
        cmd.append("cpp", 3);
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

    tinystd::small_string<512> input_name, output_name{};
    static tinystd::monotonic_increasing_string<16> tmp_name{};
    auto tmp = tmp_name.inc();
    input_name.append(tmp.data(), tmp.size());
    tmp = tmp_name.inc();
    output_name.append(tmp.data(), tmp.size());

    tassert(src_code[0] == '#' && "Shader source must begin with '#'");

    small_vector<char, 1024> header{};
    small_vector<char, 16384> to_preprocess{};
    shader_get_extensions(src_code, header, to_preprocess);
    write_file(input_name, to_preprocess);

    tinystd::small_string<512> log_name{};
    log_name.append(input_name.data(), input_name.size());
    log_name.append(".log", 4);

    cmd.append(" -std=c99", 9);
    cmd.append(" -P", 3);
    cmd.append(" -nostdinc", 10);
    cmd.append(" -undef", 7);
    cmd.append(" ", 1);
    cmd.append(input_name.data(), input_name.size());
    cmd.append(" ", 1);
    cmd.append(output_name.data(), output_name.size());
    cmd.append(" ", 1);
    cmd.append(" > ", 3);
    cmd.append(log_name.data(), log_name.size());

    ibool success = system(cmd.data()) == 0;

    if (success) {
        size_t nread{};
        char buf[64000]{};
        FILE* file = fopen(output_name.data(), "rb");
        if (file) {
            // Write header
            output.resize(header.size());
            memcpy(output.data(), header.data(), header.size());

            // Write output
            while ((nread = fread(buf, 1, sizeof(buf), file)) > 0) {
                const size_t i = output.size();
                output.resize(i + nread);
                tinystd::memcpy(output.data() + i, buf, nread);
            }
            while (output.back() == '\n' || output.back() == '\r')
                output.pop_back();
            fclose(file);
        }
    }
    else {
        print_log(log_name);
    }

    remove_file(log_name);
    remove_file(input_name);
    remove_file(output_name);

    return success;
}

//endregion

//region reflect

/*
template<typename Func>
static void spirv_iter_instructions(span<const u32> code, Func&& func)
{
     assert(code[0] == spv::MagicNumber);

    const u32* instruction = code.data() + 5;
    const u32* end = code.end();

    for(;;) {
        u32 op_code = instruction[0] & u32(UINT16_MAX);
        u32 word_count = instruction[0] >> 16u;

        if (!op_code)           break;
        if (func(op_code, instruction - code.data() , word_count))
            break;

        assert(instruction + word_count <= end);
        instruction += word_count;

        if (instruction == end) break;
        if (instruction > end)  break;
    }
}

struct spirv_value {
    const char* name{};
    u32 id{};
};

u32 reflect_shader_convert_consts_to_spec_consts(span<u32> binary, span<const char*> type_names, span<const char*> spec_const_names)
{
    u32 value_count{};
    spirv_value values[64]{};

    auto find_value = [&](u32 id){ for (u32 i = 0; i < value_count; ++i) if (values[i].id == id) return i; return -1u; };
    auto find_name = [&](const char* n, span<const char*> ns){ for (u32 i = 0; i < ns.size(); ++i) if (tinystd::streq(n, ns[i])) return i; return -1u; };

    // Get all OpConstantComposite where the first argument is an id whose name is in given type names
    spirv_iter_instructions(binary, [&](u32 op, u32 offset, u32 size){
        if (op == spv::OpName) {
            values[value_count++] = {(const char*)(binary.data() + offset + 2u), binary[offset + 1u]};
        }
        else if (op == spv::OpConstantComposite) {
            const u32 first_arg = binary[offset + 1u];
            const u32 vi = find_value(first_arg);
            if (vi != -1u) {
                const char* first_arg_name = values[vi].name;
                const u32 ni = find_name(first_arg_name, type_names);
                if (ni != -1u) {
                    // add this op, it must be used for patching
                    printf("CONST DEF %s\n", spec_const_names[ni]);
                }
            }
        }
        return false;
    });

    // For all OpConstantComposites collected, get OpSpecConstant where the result is an id whose name is the corresponding spec const name for the input name
    spirv_iter_instructions(binary, [&](u32 op, u32 offset, u32 size){
        if (op == spv::OpSpecConstant) {
            const u32 vi = find_value(binary[offset + 2u]);
            if (vi != -1u) {
                const u32 ni = find_name(values[vi].name, spec_const_names);
                if (ni != -1u) {
                    // get collected OpConstantComposite that has the same name index
                    // link collected OpConstantComposite to OpSpecConstant
                    printf("SPEC DEF %s\n", spec_const_names[ni]);
                }
            }
        }
        return false;
    });

    // Modify OpConstantComposite -> OpSpecConstantComposite with new result being the spec const id

    // For every occurrence of the OpConstantComposite result id, replace it with the spec const id

    return binary.size();
}
*/

//endregion

}




/// Preprocessor library attempt
/*

#include "tinystd_stack_vector.h"

namespace tinystd {

template<typename T, size_t N = 8>
struct page_allocator {
    static constexpr size_t PerPage = sizeof(T) >= 4096 ? 1 : (4096 / sizeof(T));
    size_t                          m_offset{};
    tinystd::stack_vector<T*, N>    m_pages{};

    page_allocator() = default;

    ~page_allocator()               { for(auto* page: m_pages) tinystd::free(page); }

    T* allocate(size_t n)           {
        if (m_pages.empty() || m_offset + n > PerPage) {
            m_offset = 0;
            m_pages.push_back((T*)tinystd::malloc(4096));
        }
        const size_t off = m_offset;
        m_offset += n;
        return m_pages.back() + off;
    }
};

}


#include "tinystd_span.h"

namespace tinycpp {

using tinystd::u32;
using tinystd::u64;
using tinystd::size_t;
using tinystd::stack_vector;
using tinystd::page_allocator;

using string_view = tinystd::span<const char>;


enum token_type_t {
    TOKEN_NONE,
    TOKEN_NEWLINE,

    TOKEN_HASHTAG,
    TOKEN_DEFINE,
    TOKEN_UNDEF,
    TOKEN_INCLUDE,
    TOKEN_PRAGMA,
    TOKEN_VERSION,

    TOKEN_IF,
    TOKEN_IFDEF,
    TOKEN_ELSE,
    TOKEN_ELIF,
    TOKEN_ENDIF,
    TOKEN_DEFINED,
    TOKEN_OPERATOR_NOT,
    TOKEN_CONDITIONAL_AND,
    TOKEN_CONDITIONAL_OR,
    TOKEN_PASTE,

    TOKEN_COMMENT_SINGLE,
    TOKEN_COMMENT_MULTI_BEGIN,
    TOKEN_COMMENT_MULTI_END,

    TOKEN_OPEN_PARENTHESES,
    TOKEN_CLOSE_PARENTHESES,
    TOKEN_COMMA,
};


struct location {
    u64 file    : 8;
    u64 line    : 20;
    u64 n_line  : 4;
    u64 col     : 16;
    u64 col_end : 16;
};


struct token {
    location        loc{};
    string_view     str{};
    u32             type: 16;
    u32             is_comment: 1;
    u32             is_pasted: 1;
    u32             PAD: 14;
    token*          next{};
    token*          prev{};
};


struct token_list {
    token*                      m_front{};
    token*                      m_back{};
    page_allocator<token, 8>    m_pages{};
    page_allocator<char, 8>     m_pages_str{};

    token_list() = default;

    void push_back(const token& t);
    void remove(const token* t);
};


void tokenize(token_list& out, string_view input);

void preprocess(token_list& out, const token_list& input);

void stringify(token_list& out, tinystd::span<char> output, bool remove_comments = true);

}

#include <cstring>
#include <cstdio>
#include "tinystd_algorithm.h"
#include "tinystd_assert.h"

namespace tinycpp {

//region token_list

void token_list::push_back(const token& t)
{
    auto* ps = m_pages_str.allocate(t.str.size() + 1);
    tinystd::memcpy(ps, t.str.data(), t.str.size());
    ps[t.str.size()] = 0;
    auto* pt = m_pages.allocate(1);
    *pt = t;
    pt->str = {ps, t.str.size()};
    pt->prev = m_back;
    pt->next = nullptr;
    if (pt->prev)
        pt->prev->next = pt;
    if (m_front == nullptr)
        m_front = pt;
    m_back = pt;
}

void token_list::remove(const token *t)
{
    if (t == m_front)
        m_front = m_front->next;
    if (t == m_back)
        m_back = m_back->prev;
    if (t->prev)
        t->prev->next = t->next;
    if (t->next)
        t->next->prev = t->prev;
}

//endregion

//region tokenize

struct token_state {
    const char* data{};
    u32 i{}, file{}, col{}, line{};
    u32 in_single_comment: 1;
    u32 in_multi_comment: 1;
    u32 pad: 31;
};

static bool equals(const string_view& l, const string_view& r) { return strncmp(l.data(), r.data(), r.size()) == 0; }
static bool is_newline(char c)          { return c == '\n'; }
static bool is_alpha_lower(char c)      { return c >= 'a' && c <= 'z'; }
static bool is_alpha_upper(char c)      { return c >= 'A' && c <= 'Z'; }
static bool is_alpha(char c)            { return is_alpha_lower(c) || is_alpha_upper(c); }
static bool is_digit(char c)            { return c >= '0' && c <= '9'; }
static bool is_word(char c)             { return is_digit(c) || is_alpha(c) || c == '_'; }

static location
get_loc(const token_state& s, u32 line, u32 col)
{
    return location{s.file, line, s.line - line, col, s.col};
}

static token_type_t
get_token_type(string_view word)
{
    if (equals(word, {"defined", 7}))    return TOKEN_DEFINED;
    if (equals(word, {"version", 6}))    return TOKEN_VERSION;
    if (equals(word, {"include", 6}))    return TOKEN_INCLUDE;
    if (equals(word, {"pragma", 6}))     return TOKEN_PRAGMA;
    if (equals(word, {"define", 6}))     return TOKEN_DEFINE;
    if (equals(word, {"ifdef", 5}))      return TOKEN_IFDEF;
    if (equals(word, {"undef", 5}))      return TOKEN_UNDEF;
    if (equals(word, {"endif", 5}))      return TOKEN_ENDIF;
    if (equals(word, {"elif", 4}))       return TOKEN_ELIF;
    if (equals(word, {"else", 4}))       return TOKEN_ELSE;
    if (equals(word, {"if", 2}))         return TOKEN_IF;
    return TOKEN_NONE;
}

static token_type_t
get_token_type(char c)
{
    if (c == '!')          return TOKEN_OPERATOR_NOT;
    if (c == '(')          return TOKEN_OPEN_PARENTHESES;
    if (c == ')')          return TOKEN_CLOSE_PARENTHESES;
    if (c == ',')          return TOKEN_COMMA;
    return TOKEN_NONE;
}

static token
parse_word(token_state& s)
{
    const u32 i = s.i, col = s.col, line = s.line;
    while (is_word(s.data[s.i++]));
    s.col = col + (--s.i - i);
    token t{get_loc(s, line, col)};
    t.str = string_view{s.data + i, s.i - i};
    t.type = get_token_type(t.str);
    t.is_comment = s.in_single_comment || s.in_multi_comment;
    return t;
}

static token
parse_character(token_state& s, token_type_t t, size_t n = 1)
{
    const u32 col = s.col, i = s.i;
    s.col += n;
    s.i += n;
    return token{get_loc(s, s.line, col), {s.data + i, n}, u32(t), s.in_single_comment || s.in_multi_comment};
}

static void
do_tokenize(token_list& out, string_view input, u32 file_index)
{
    const size_t end = input.size() - 1;
    token_state s{input.data()};
    s.file = file_index;
    while (s.i < end) {
        const char c = input[s.i];
        if (is_word(c)) {
            out.push_back(parse_word(s));
            continue;
        }
        if (c == '#') {
            out.push_back(parse_character(s, TOKEN_HASHTAG));
            if (input[s.i] == '#')
                out.push_back(parse_character(s, TOKEN_PASTE));
            else
                out.push_back(parse_word(s));
            // Replace includes directly
            if (out.m_back->type == TOKEN_INCLUDE) {
                string_view included_file{};
                do_tokenize(out, included_file, file_index + 1);
            }
            continue;
        }
        if (is_newline(c)) {
            s.in_single_comment = false;
            out.push_back(parse_character(s, TOKEN_NEWLINE));
            ++s.line;
            s.col = 0;
            continue;
        }
        if (c != ' ' && c != '\t') {
            if (s.i + 1 < end && !is_word(s.data[s.i + 1]) && s.data[s.i + 1] != ' ' && s.data[s.i + 1] != '\t') {
                const char nc = s.data[s.i + 1];
                if (c == '/' && nc == '/') {
                    s.in_single_comment = true;
                    out.push_back(parse_character(s, TOKEN_COMMENT_SINGLE, 2));
                } else if (c == '/' && nc == '*') {
                    s.in_multi_comment = true;
                    out.push_back(parse_character(s, TOKEN_COMMENT_MULTI_BEGIN, 2));
                } else if (c == '*' && nc == '/') {
                    out.push_back(parse_character(s, TOKEN_COMMENT_MULTI_END, 2));
                    s.in_multi_comment = false;
                } else if (c == '&' && nc == '&') {
                    out.push_back(parse_character(s, TOKEN_CONDITIONAL_AND, 2));
                } else if (c == '|' && nc == '|') {
                    out.push_back(parse_character(s, TOKEN_CONDITIONAL_OR, 2));
                } else {
                    out.push_back(parse_character(s, TOKEN_NONE, 2));
                }
            }
            else {
                out.push_back(parse_character(s, get_token_type(c)));
            }
            continue;
        }
        ++s.col;
        ++s.i;
    }
}

void tokenize(token_list& out, string_view input)
{
    do_tokenize(out, input, 0);
}

//endregion

//region preprocess

template<size_t N>
struct bit_stack {
    u64 m_size{};
    u64 m_words[N < 64 ? 1 : N/64]{};

    bool operator[](size_t i) const noexcept { return (m_words[i/64] & (1ull << (i & 63))) > 0; }
    bool back()               const noexcept { return operator[](m_size - (m_size > 0)); }
    void push_back(bool b)          noexcept { m_words[m_size/64] |= (u64(b) << (m_size & 63)); ++m_size; }
    void pop_back()                 noexcept { --m_size; m_words[m_size/64] &= ~(1ull << (m_size & 63)); }
};


struct process_state;


struct macro_function {
    string_view                  rep_str{};
    stack_vector<string_view, 8> args{};
    stack_vector<u32, 64>        reps{};

    const token* parse_args(const token* t) noexcept;

    const token* parse_replacement(token_list& out, const process_state& s, const token* t) noexcept;

    string_view replace(token_list& out, const token*& t) const noexcept;

    string_view replace(token_list& out, const stack_vector<string_view, 8>& args) const noexcept;

};


struct process_state {
    bit_stack<4096>                 if_states;
    stack_vector<string_view, 128>  defines{};
    stack_vector<string_view, 128>  values{};
    stack_vector<string_view, 128>  func_names{};
    stack_vector<macro_function, 8> funcs{};

    const string_view* is_defined(string_view s) const noexcept {
        auto it = tinystd::find_if(defines.begin(), defines.end(), [s](auto& v){ return equals(v, s); });
        return it == defines.end() ? nullptr : values.data() + (it - defines.begin());
    }

    void un_define(string_view s) {
        auto it = tinystd::find_if(defines.begin(), defines.end(), [s](auto& v){ return equals(v, s); });
        if (it != defines.end()) {
            values.erase(values.begin() + (it - defines.end()));
            defines.erase(it);
        }
    }

    token replace_define(token_list& out, const token*& t) const noexcept {
        auto tok = *t;
        while (true) {
            auto fit = tinystd::find_if(func_names.begin(), func_names.end(), [s=tok.str](auto& v){ return equals(v, s); });
            if (fit != func_names.end()) {
                auto& fn = funcs[fit - func_names.begin()];
                tok.str = fn.replace(out, t);
            }
            auto it = is_defined(tok.str);
            if (it != nullptr){
                tok.str = *it;
            }
            else {
                break;
            }
        }
        t = t->next;
        return tok;
    }
};




const token* macro_function::parse_args(const token* t) noexcept
{
    tassert(t->type == TOKEN_OPEN_PARENTHESES);
    for(t = t->next; t->type != TOKEN_CLOSE_PARENTHESES; t = t->next) {
        if (t->type != TOKEN_COMMA)
            args.push_back(t->str);
    }
    return t->next;
}

const token* macro_function::parse_replacement(token_list& out, const process_state& s, const token* t) noexcept
{
    u32 n = 0;
    const token* begin = t;
    stack_vector<tinystd::pair<u32, string_view>, 8> func_reps{};
    for(t = begin; t->type != TOKEN_NEWLINE; t = t->next) {
        auto it = tinystd::find_if(args.begin(), args.end(), [s=t->str](auto& v){ return equals(v, s); });
        if (it != args.end()) {
            reps.push_back(it - args.begin());
            n += 2;
        }
        else {
//            auto fit = tinystd::find_if(s.func_names.begin(), s.func_names.end(), [s=t->str](auto& v){ return equals(v, s); });
//            if (fit != s.func_names.end()) {
//                auto& fn = s.funcs[fit - s.func_names.begin()];
//                const auto* tk = t;
//                func_reps.push_back({n, fn.replace(out, tk)});
//                n += func_reps.back().second.size();
//            }
//            else {
                n += t->str.size();
//            }
        }
    }
    auto* p_str = out.m_pages_str.allocate(n);
    rep_str = {p_str, n};
    u32 fi{};
    for(t = begin; t->type != TOKEN_NEWLINE; t = t->next) {
        auto it = tinystd::find_if(args.begin(), args.end(), [s=t->str](auto& v){ return equals(v, s); });;

//        if (!func_reps.empty() && (p_str - rep_str.data()) == func_reps[fi].first) {
//            memcpy(p_str, func_reps[fi].second.data(), func_reps[fi].second.size());
//            p_str += func_reps[fi].second.size();
//        }
//        else
        if (it != args.end()) {
            memcpy(p_str, "%s", 2);
            p_str += 2;
        }
        else {
            memcpy(p_str, t->str.data(), t->str.size());
            p_str += t->str.size();
        }
    }
    *p_str = 0;
    return t->next;
}

string_view macro_function::replace(token_list& out, const token*& t) const noexcept
{
    tassert(t->next->type == TOKEN_OPEN_PARENTHESES);
    stack_vector<string_view, 8> arg_reps{};
    for (t = t->next->next; t->type != TOKEN_CLOSE_PARENTHESES; t = t->next) {
        if (t->type != TOKEN_COMMA)
            arg_reps.push_back(t->str);
    }
    return replace(out, arg_reps);
}

string_view macro_function::replace(token_list& out, const stack_vector<string_view, 8>& arg_reps) const noexcept
{
    u64 total_size = rep_str.size() - (reps.size() * 2);
    for (const auto r: reps)
        total_size += arg_reps[r].size() + 1;

    auto* p_str = out.m_pages_str.allocate(total_size);
    string_view sv{p_str, total_size};
    u32 rep_i = 0;
    for (u32 i = 0; i < rep_str.size(); ++i) {
        const char c = rep_str[i];
        if (c == '%' && rep_str[i + 1] == 's') {
            const auto r = reps[rep_i];
            p_str += snprintf(p_str, arg_reps[r].size() + 1, "%s", arg_reps[r].data());
            *(p_str++) = ' ';
            ++rep_i;
            ++i;
        }
        else {
            *(p_str++) = c;
        }
    }
    return sv;
}





static const token*
preprocess_token(token_list& out, process_state& s, const token* t);


static const token*
preprocess_multi_line_comment(token_list& out, process_state& s, const token* t)
{
    if (t->type == TOKEN_COMMENT_MULTI_BEGIN) {
        for (; t->is_comment; t = preprocess_token(out, s, t));
    }
    return t;
}

static const token*
preprocess_until_newline(token_list& out, process_state& s, const token* t)
{
    t = preprocess_multi_line_comment(out, s, t);
    for (; t; t = t->next) {
        if (t->type == TOKEN_NEWLINE) {
            return t->next;
        }
    }
    return t;
}

static const token*
preprocess_define(token_list& out, process_state& s, const token* t)
{
    // Macro function -> #define FUNC(x) x
    if (t->next->type == TOKEN_OPEN_PARENTHESES) {
        s.func_names.push_back(t->str);
        s.funcs.push_back({});
        auto& fn = s.funcs.back();
        t = fn.parse_args(t->next);
        t = preprocess_multi_line_comment(out, s, t);
        t = fn.parse_replacement(out, s, t);
    }
    // Macro definition
    //  -> #define Q
    //  -> #define Q ABC
    else {
        s.defines.push_back(t->str);
        t = preprocess_multi_line_comment(out, s, t->next);
        s.values.push_back(t->type == TOKEN_NEWLINE ? string_view{" ", 1} : t->str);
    }
    return preprocess_until_newline(out, s, t->next);
}

static const token*
preprocess_evaluate_block(token_list& out, process_state& s, const token* t)
{
    bool valid_state = true;
    bool is_and = true;
    while (t->type != TOKEN_NEWLINE) {
        bool define_required = true;
        if (t->type == TOKEN_OPERATOR_NOT) {
            define_required = false;
            t = t->next;
        }
        if (t->type == TOKEN_DEFINED) {
            bool is_defined = s.is_defined(t->next->next->str);
            if (is_and)
                valid_state &= (is_defined == define_required);
            else
                valid_state |= (is_defined == define_required);
            // defined ( VALUE ) NEXT
            t = t->next->next->next->next;
            if (t->type == TOKEN_CONDITIONAL_AND || t->type == TOKEN_CONDITIONAL_OR) {
                is_and = t->type == TOKEN_CONDITIONAL_AND;
                t = preprocess_multi_line_comment(out, s, t->next);
            } else if (t->type == TOKEN_COMMENT_MULTI_BEGIN) {
                t = preprocess_multi_line_comment(out, s, t->next);
            } else {
                t = preprocess_until_newline(out, s, t)->prev;
            }
        }
    }
    s.if_states.push_back(valid_state);
    return preprocess_until_newline(out, s, t);
}

static const token*
preprocess_if_block(token_list& out, process_state& s, const token* t)
{
    if (!s.if_states.back()) {
        for (; t && (t->type != TOKEN_HASHTAG || (t->next && t->next->type == TOKEN_PASTE)); t = t->next);
    }
    else {
        for (; t && (t->type != TOKEN_HASHTAG || (t->next && t->next->type == TOKEN_PASTE)); t = preprocess_token(out, s, t));
    }
    if (t->next->type != TOKEN_ELSE && t->next->type != TOKEN_ELIF)
        s.if_states.pop_back();
    return t;
}

static const token*
preprocess_hashtag(token_list& out, process_state& s, const token* t)
{
    const token* directive = t->next;
    if (directive->type == TOKEN_PASTE) {
        out.m_back->is_pasted = true;
        t = directive->next;
        out.push_back(s.replace_define(out, t));
        return t;
    } else if (directive->type == TOKEN_IF) {
        return preprocess_if_block(out, s, preprocess_evaluate_block(out, s, directive->next));
    } else if (directive->type == TOKEN_ELIF) {
        bool was_defined = s.if_states.back();
        s.if_states.pop_back();
        t = preprocess_evaluate_block(out, s, directive->next);
        bool is_defined = s.if_states.back();
        s.if_states.pop_back();
        s.if_states.push_back(!was_defined && is_defined);
        return preprocess_if_block(out, s, t);
    } else if (directive->type == TOKEN_IFDEF) {
        s.if_states.push_back(s.is_defined(directive->next->str));
        return preprocess_if_block(out, s, preprocess_until_newline(out, s, directive->next->next));
    } else if (directive->type == TOKEN_ELSE) {
        bool was_defined = s.if_states.back();
        s.if_states.pop_back();
        s.if_states.push_back(!was_defined);
        return preprocess_if_block(out, s, preprocess_until_newline(out, s, directive->next));
    } else if (directive->type == TOKEN_DEFINE) {
        return preprocess_define(out, s, directive->next);
    } else if (directive->type == TOKEN_UNDEF) {
        s.un_define(directive->next->str);
        return directive->next;
    } else if (directive->type == TOKEN_ENDIF) {
        return directive->next;
    } else {
        out.push_back(*t);
        out.push_back(*t->next);
        return directive->next;
    }
    return nullptr;
}

const token*
preprocess_token(token_list& out, process_state& s, const token* t)
{
    if (t->is_comment) {
        out.push_back(*t);
        t = t->next;
    }
    else if (t->type == TOKEN_HASHTAG) {
        t = preprocess_hashtag(out, s, t);
    }
    else {
        out.push_back(s.replace_define(out, t));
    }
    return t;
}

void preprocess(token_list& out, const token_list& input)
{
    process_state s;
    for (const auto *t = input.m_front; t; t = preprocess_token(out, s, t));
}

//endregion

void stringify(token_list& out, tinystd::span<char> output, bool remove_comments)
{
    if (remove_comments) {
        for (const auto *t = out.m_front; t; t = t->next) {
            if (t->is_comment)
                out.remove(t);
        }
    }
    size_t offset{};
    for (const auto *t = out.m_front; t; t = t->next) {
        const size_t n = t->str.size();
        if (offset + n > output.size()) {
            tinystd::error("Not enough size for preprocessing\n");
            tinystd::exit(1);
        }
        memcpy(output.data() + offset, t->str.data(), n);
        if (t->type != TOKEN_NEWLINE && t->type != TOKEN_HASHTAG && !t->is_pasted)
            output[offset++ + n] = ' ';
        offset += n;
    }
}

}

*/

//TEST_CASE("shaderc", "[tinycpp]")
//{
//    using namespace tinycpp;
//    static constexpr const char data[512]{R"(#version 450 //comment
//#define REP QQQ //comment  /* wtf is this shit */
//#define X /* wtf is this shit */
///* wtf is this shit */ #define Y
//#if defined(X) ||/* wtf is this shit */ defined(Y) //comment
//ABC //comment
//abc##REP //comment
//#else //comment
//XYZ //comment
//#endif //comment
//
//void main() {
//    int x = 1;
//    int y = x;
//}
//)"};
//
//    static constexpr const char data1[512]{R"(
//#define MACRO_FUNC1(x, y) y y y y x
//
//#define MACRO_FUNC2(x, y) (y, x)
//
//MACRO_FUNC2(stuff, pov)
//)"};
//
//    token_list l, p;
////    tokenize(l, {data, strlen(data)});
//    tokenize(l, {data1, strlen(data1)});
//
//    preprocess(p, l);
//    char buf[1024]{};
//    stringify(p, buf, false);
//    tinystd::print("%s", buf);
//    tinystd::print("\n\n");
//    for (auto *t = p.m_front; t; t = t->next) {
//        tinystd::print("PToken: file: %u, line: %u, col: %u, str: %s\n", t->loc.file, t->loc.line, t->loc.col, t->str.data());
//    }
//    tinystd::print("\n\n");
//    for (auto *t = l.m_front; t; t = t->next) {
//        tinystd::print("Token: file: %u, line: %u, col: %u, str: %s\n", t->loc.file, t->loc.line, t->loc.col, t->str.data());
//    }
//}
