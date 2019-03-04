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

#ifndef _tools_md_enums_h
#define _tools_md_enums_h

#define MD_ENUM_FLAGS(t) \
inline constexpr t operator&(t x, t y) \
{ \
    return static_cast<t>(static_cast<int>(x) & static_cast<int>(y)); \
} \
inline constexpr t operator|(t x, t y) \
{ \
    return static_cast<t>(static_cast<int>(x) | static_cast<int>(y)); \
} \
inline constexpr t operator^(t x, t y) \
{ \
    return static_cast<t>(static_cast<int>(x) ^ static_cast<int>(y)); \
} \
inline constexpr t operator~(t x) \
{ \
    return static_cast<t>(~static_cast<int>(x)); \
} \
inline t& operator&=(t& x, t y) \
{ \
    x = x & y; \
    return x; \
} \
inline t& operator|=(t& x, t y) \
{ \
    x = x | y; \
    return x; \
} \
inline t& operator^=(t& x, t y) \
{ \
    x = x ^ y; \
    return x; \
}

#define MD_SET_BIT(_v, _f) _v |= _f
#define MD_UNSET_BIT(_v, _f) _v &= ~_f
#define MD_TEST_FLAG(_v, _f) ((_v & _f) == _f)


namespace md{
    
    
    
    
    
}//::md
#endif //_tools_md_enums_h
