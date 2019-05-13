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

#ifndef _tools_md_stable_headers_h
#define _tools_md_stable_headers_h

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <fcntl.h>

#include <memory>
#include <cmath>
#include <numeric>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <atomic>
#include <vector>

#include <strings.h>

#include <event2/event.h>
#include <event2/util.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <boost/logic/tribool.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#ifdef MD_USE_STD_STRING_VIEW
    #include <string_view>
#else
    #include <boost/utility/string_view.hpp>
#endif //MD_USE_STD_STRING_VIEW

#include <date/date.h>
#include <date/tz.h>
#include "fmt/format.h"

namespace md{
namespace bfs = boost::filesystem;
    
#ifdef MD_USE_STD_STRING_VIEW
    /// The type of string view used by the library
    using string_view = std::string_view;
    /// The type of basic string view used by the library
    template<class CharT, class Traits>
    using basic_string_view = std::basic_string_view<CharT, Traits>;
#else
    /// The type of string view used by the library
    using string_view = boost::string_view;
    /// The type of basic string view used by the library
    template<class CharT, class Traits>
    using basic_string_view = boost::basic_string_view<CharT, Traits>;
#endif //MD_USE_STD_STRING_VIEW
}//::md

#ifdef MD_USE_STD_STRING_VIEW
    
#else
namespace fmt {
    template <>
    struct formatter<md::string_view> {
        template <typename ParseContext>
        constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }
        
        template <typename FormatContext>
        auto format(const md::string_view &s, FormatContext &ctx) {
            return format_to(ctx.out(), "{}", s.data());
        }
    };
}
#endif //MD_USE_STD_STRING_VIEW


#endif//_tools_md_stable_headers_h
