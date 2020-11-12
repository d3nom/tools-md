/*
MIT License

Copyright (c) 2019 Michel Dénommée

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _tools_md_errors_h
#define _tools_md_errors_h

#include "stable_headers.h"

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <cxxabi.h>


namespace md { namespace error{

/** Print a demangled stack backtrace of the caller function to FILE* out. */
inline std::string get_stacktrace(
    //FILE* out = stderr,
    unsigned int max_frames = 63)
{
    std::string out;
    out += "stack trace:\n";
    //fprintf(out, "stack trace:\n");

    // storage array for stack trace address data
    void* addrlist[max_frames+1];

    // retrieve current stack addresses
    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

    if(addrlen == 0){
        out += "  <empty, possibly corrupt>\n";
        //fprintf(out, "  <empty, possibly corrupt>\n");
        return out;
    }

    // resolve addresses into strings containing "filename(function+address)",
    // this array must be free()-ed
    char** symbollist = backtrace_symbols(addrlist, addrlen);

    // for(int i = 0; i < addrlen; ++i){
    //     printf("[%d] %p, %s\n\n", i, addrlist[i], symbollist[i]);
    // }

    // allocate string which will be filled with the demangled function name
    size_t funcnamesize = 256;
    char* funcname = (char*)malloc(funcnamesize);

    // iterate over the returned symbol lines. skip the first, it is the
    // address of this function.
    for(int i = 2; i < addrlen; ++i){
        char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

        // find parentheses and +address offset surrounding the mangled name:
        // ./module(function+0x15c) [0x8048a6d]
        for(char *p = symbollist[i]; *p; ++p){
            if(*p == '(')
                begin_name = p;
            else if(*p == '+')
                begin_offset = p;
            else if(*p == ')' && begin_offset){
                end_offset = p;
                break;
            }
        }

        if(begin_name && begin_offset && end_offset &&
            begin_name < begin_offset
        ){
            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            *end_offset = '\0';

            // mangled name is now in [begin_name, begin_offset) and caller
            // offset in [begin_offset, end_offset). now apply
            // __cxa_demangle():

            int status;
            char* ret = abi::__cxa_demangle(
                begin_name, funcname, &funcnamesize, &status
            );

            if(status == 0){
                funcname = ret; // use possibly realloc()-ed string
                out += fmt::format(
                    "  {} :\n    {}+{}\n",
                    symbollist[i], funcname, begin_offset
                );
                // fprintf(
                //     out,
                //     "  %s : %s+%s\n",
                //     symbollist[i], funcname, begin_offset
                // );
            }else{
                // demangling failed. Output function name as a C function with
                // no arguments.
                out += fmt::format(
                    "  {} :\n    {}()+{}\n",
                    symbollist[i], begin_name, begin_offset
                );
                // fprintf(
                //     out,
                //     "  %s : %s()+%s\n",
                //     symbollist[i], begin_name, begin_offset
                // );
            }
        }else{
            // couldn't parse the line? print the whole line.
            out += fmt::format("  {}\n", symbollist[i]);
            //fprintf(out, "  %s\n", symbollist[i]);
        }

        // /* find first occurence of '(' or ' ' in message[i] and assume
        // * everything before that is the file name. (Don't go beyond 0 though
        // * (string terminator)*/
        // size_t p = 0;
        // while(symbollist[i][p] != '(' && symbollist[i][p] != ' '
        //         && symbollist[i][p] != 0)
        //     ++p;

        // char syscom[256];
        // //last parameter is the file name of the symbol
        // sprintf(
        //     syscom,
        //     "addr2line -Cf -e %.*s %p",
        //     (int)p,
        //     symbollist[i],
        //     addrlist[i]
        // );
        // system(syscom);
    }

    free(funcname);
    free(symbollist);

    return out;
}

class stacked_error : public std::runtime_error
{
public:
    stacked_error(md::string_view msg)
        : std::runtime_error(msg.data()),
        _stack(md::error::get_stacktrace()),
        _file(), _line(), _func()
    {
    }

    stacked_error(
        md::string_view msg,
        md::string_view filename,
        int line,
        md::string_view func)
        : std::runtime_error(msg.data()),
        _stack(md::error::get_stacktrace()),
        _file(filename), _line(line), _func(func)
    {
    }

    ~stacked_error(){}

    std::string stack() const { return _stack;}
    md::string_view file() const { return _file;}
    int line() const { return _line;}
    md::string_view func() const { return _func;}

private:
    std::string _stack;
    md::string_view _file;
    int _line;
    md::string_view _func;
};


}}//::md::error

#if defined(_DEBUG) || !defined(NDEBUG)
#define MD_ERR(msg, ...) \
    md::error::stacked_error( \
        fmt::format(msg, ##__VA_ARGS__), \
        __FILE__, __LINE__, __PRETTY_FUNCTION__ \
    )
#else
#define MD_ERR(msg, ...) \
    std::runtime_error( \
        fmt::format(msg, ##__VA_ARGS__) \
    )
#endif


namespace md{ namespace callback{
class cb_error
{
    friend std::ostream& operator<<(std::ostream& s, const cb_error& v);
public:
    static cb_error no_err;

    cb_error(): _has_err(false), _msg(""), _has_stack(false)
    {}

    cb_error(nullptr_t /*np*/): _has_err(false), _msg(""), _has_stack(false)
    {}

    cb_error(const std::exception& err):
        _err(err), _has_err(true)
    {
        _msg = std::string(err.what());

        try{
            auto se = dynamic_cast<const md::error::stacked_error&>(err);
            _stack = se.stack();
            _file = se.file().data();
            _line = se.line();
            _func = se.func().data();
            _has_stack = true;

        }catch(...){
            _has_stack = false;
        }
    }

    virtual ~cb_error()
    {
    }

    const std::exception& error() const { return _err;}

    explicit operator bool() const
    {
        return _has_err;
    }

    explicit operator const char*() const
    {
        if(!_has_err)
            return "No error assigned!";
        return _msg.c_str();
    }

    const char* c_str() const
    {
        if(!_has_err)
            return "No error assigned!";
        return _msg.c_str();
    }

    bool has_stack() const { return _has_stack;}
    std::string stack() const { return _stack;}
    std::string file() const { return _file;}
    int line() const { return _line;}
    std::string func() const { return _func;}

private:
    std::exception _err;
    bool _has_err;
    std::string _msg;

    bool _has_stack;
    std::string _stack;
    std::string _file;
    int _line;
    std::string _func;
};

inline std::ostream& operator<<(std::ostream& s, const cb_error& v)
{
    s << std::string(v.c_str());
    return s;
}

}}//::md::error
namespace fmt {
    template <>
    struct formatter<md::callback::cb_error> {
        template <typename ParseContext>
        constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

        template <typename FormatContext>
        auto format(const md::callback::cb_error& e, FormatContext& ctx) {
            return format_to(ctx.out(), "{}", e.c_str());
        }
    };
}


#endif //_tools_md_errors_h
