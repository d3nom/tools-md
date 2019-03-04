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

#ifndef _tools_md_callbacks_h
#define _tools_md_callbacks_h

#include "stable_headers.h"
#include "errors.h"
#include "traits.h"
#include "logging.h"

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
            auto se = dynamic_cast<const evmvc::stacked_error&>(err);
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

std::ostream& operator<<(std::ostream& s, const cb_error& v)
{
    s << std::string(v.c_str());
    return s;
}

typedef std::function<void()> void_cb;
typedef std::function<void(const cb_error& err)> async_cb;


template <typename T>
using value_cb = std::function<void(const cb_error& err, T)>;
template <typename T>
using no_err_value_cb = std::function<void(T)>;


template<typename T>
using async_item_cb = typename std::function<void(const T& val, async_cb)>;
typedef std::function<void(async_cb)> async_series_cb;

void noop_cb(const cb_error& err)
{
    if(err)
        md::log::default_logger()->error(
            "Unhandled exception:\n{}", err
        );
}
const async_cb run_async = &noop_cb;

template<typename T, typename U, typename V,
    typename std::enable_if<
        std::is_invocable_r<
            void,
            typename std::remove_pointer<V>::type
        >::value &&
        !std::is_invocable_r<
            void, 
            typename std::remove_pointer<V>::type,
            const cb_error&
        >::value &&
        !std::is_invocable_r<
            void,
            typename std::remove_pointer<V>::type,
            const cb_error&,
            U
        >::value &&
        !std::is_invocable_r<
            void,
            typename std::remove_pointer<V>::type,
            U
        >::value
    , int32_t>::type = -1
>
void assign_value_cb(
    T& cb, const V& value) noexcept
{
    cb = [value](const cb_error& err, U /*val*/){
        if(err)
            md::log::default_logger()->error(
                "Unhandled exception:\n{}", err
            );
        value();
    };
}

template<typename T, typename U, typename V,
    typename std::enable_if<
        !std::is_invocable_r<
            void,
            typename std::remove_pointer<V>::type
        >::value &&
        std::is_invocable_r<
            void, 
            typename std::remove_pointer<V>::type,
            const cb_error&
        >::value &&
        !std::is_invocable_r<
            void,
            typename std::remove_pointer<V>::type,
            const cb_error&,
            U
        >::value &&
        !std::is_invocable_r<
            void,
            typename std::remove_pointer<V>::type,
            U
        >::value
    , int32_t>::type = -1
>
void assign_value_cb(
    T& cb, const V& value) noexcept
{
    cb = [value](const cb_error& err, U /*val*/){
        value(err);
    };
}

template<typename T, typename U, typename V,
    typename std::enable_if<
        !std::is_invocable_r<
            void,
            typename std::remove_pointer<V>::type
        >::value &&
        !std::is_invocable_r<
            void, 
            typename std::remove_pointer<V>::type,
            const cb_error&
        >::value &&
        std::is_invocable_r<
            void,
            typename std::remove_pointer<V>::type,
            const cb_error&,
            U
        >::value &&
        !std::is_invocable_r<
            void,
            typename std::remove_pointer<V>::type,
            U
        >::value
    , int32_t>::type = -1
>
void assign_value_cb(
    T& cb, const V& value) noexcept
{
    cb = value;
}

template<typename T, typename U, typename V,
    typename std::enable_if<
        !std::is_invocable_r<
            void,
            typename std::remove_pointer<V>::type
        >::value &&
        !std::is_invocable_r<
            void, 
            typename std::remove_pointer<V>::type,
            const cb_error&
        >::value &&
        !std::is_invocable_r<
            void,
            typename std::remove_pointer<V>::type,
            const cb_error&,
            U
        >::value &&
        std::is_invocable_r<
            void,
            typename std::remove_pointer<V>::type,
            U
        >::value
    , int32_t>::type = -1
>
void assign_value_cb(
    T& cb, const V& value) noexcept
{
    cb = [value](const cb_error& err, U val){
        if(err)
            md::log::default_logger()->error(
                "Unhandled exception:\n{}", err
            );
        value(val);
    };
}




template<typename T, typename V,
    typename std::enable_if<
        std::is_invocable_r<
            void,
            typename std::remove_pointer<V>::type
        >::value &&
        !std::is_invocable_r<
            void, 
            typename std::remove_pointer<V>::type,
            const cb_error&
        >::value
    , int32_t>::type = -1
>
void assign_async_cb(
    T& cb, const V& value) noexcept
{
    cb = [value](const cb_error& err){
        if(err)
            md::log::default_logger()->error(
                "Unhandled exception:\n{}", err
            );
        value();
    };
}

template<typename T, typename V,
    typename std::enable_if<
        !std::is_invocable_r<
            void,
            typename std::remove_pointer<V>::type
        >::value &&
        std::is_invocable_r<
            void, 
            typename std::remove_pointer<V>::type,
            const cb_error&
        >::value
    , int32_t>::type = -1
>
void assign_async_cb(
    T& cb, const V& value) noexcept
{
    cb = value;
}
    
    
    
    
}}//::md
#endif //_tools_md_callbacks_h
