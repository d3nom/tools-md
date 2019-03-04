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


#include <memory>
#include <cmath>

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include <vector>

#include <strings.h>

#include <event2/event.h>
#include <event2/util.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>


#ifdef MD_USE_STD_STRING_VIEW
    #include <string_view>
#else
    #include <boost/utility/string_view.hpp>
#endif

namespace md{
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
#endif
}//::md

#endif//_tools_md_stable_headers_h
