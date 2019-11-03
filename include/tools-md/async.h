/*
MIT License

Copyright (c) 2011-2019 Michel Dénommée

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

#ifndef _tools_md_async_h
#define _tools_md_async_h

#include "stable_headers.h"
#include "errors.h"
#include "logging.h"
#include "callbacks.h"
#include "event_strand.h"

#include <type_traits>
#include <functional>
#include <vector>

namespace md{


class async final
{
private:
    async(){}
    
    template<typename T>
    class waterfall_data_t
    {
    public:
        T data;
        md::callback::cb_error err;
    };
    template<typename T>
    using waterfall_data = std::shared_ptr<waterfall_data_t<T>>;
    
public:
    
    template<
        typename Iterator,
        typename T,
        typename std::enable_if<
            std::is_invocable_r<
                void,
                T,
                const typename std::iterator_traits<Iterator>::value_type&,
                std::function<void(const md::callback::cb_error& err)>
            >::value
        , int32_t>::type = -1
    >
    static void each(
        Iterator first, Iterator last,
        T cb,
        md::callback::async_cb end_cb)
    {
        typedef typename std::iterator_traits<Iterator>::value_type U;
        md::callback::async_item_cb<U> aicb = cb;
        md::async::each<Iterator,U>(first, last, aicb, end_cb);
    }

    template<
        typename Iterator,
        typename T = typename std::iterator_traits<Iterator>::value_type
    >
    static void each(
        Iterator first, Iterator last,
        md::callback::async_item_cb<T> cb,
        md::callback::async_cb end_cb)
    {
        std::vector<T> v;
        auto it = first;
        while(it != last){
            v.emplace_back(*it);
            ++it;
        }
        each(md::event_queue_t::get_default(), v, cb, end_cb);
    }
    
    template<
        typename Iterator,
        typename T,
        typename std::enable_if<
            std::is_invocable_r<
                void,
                T,
                const typename std::iterator_traits<Iterator>::value_type&,
                std::function<void(const md::callback::cb_error& err)>
            >::value
        , int32_t>::type = -1
    >
    static void each(
        std::shared_ptr<event_queue_t> eq,
        Iterator first, Iterator last,
        T cb,
        md::callback::async_cb end_cb)
    {
        typedef typename std::iterator_traits<Iterator>::value_type U;
        md::callback::async_item_cb<U> aicb = cb;
        md::async::each<Iterator,U>(eq, first, last, aicb, end_cb);
    }

    
    template<
        typename Iterator,
        typename T = typename std::iterator_traits<Iterator>::value_type
    >
    static void each(
        std::shared_ptr<event_queue_t> eq,
        Iterator first, Iterator last,
        md::callback::async_item_cb<T> cb,
        md::callback::async_cb end_cb)
    {
        std::vector<T> v;
        auto it = first;
        while(it != last){
            v.emplace_back(*it);
            ++it;
        }
        each(eq, v, cb, end_cb);
    }
    
    
    template<typename T>
    static void each(
        std::vector<T> v,
        md::callback::async_item_cb<T> cb,
        md::callback::async_cb end_cb)
    {
        std::shared_ptr<event_queue_t> eq = md::event_queue_t::get_default();
        each<T>(eq, v, cb, end_cb);
    }
    
    template<typename T>
    static void each(
        std::shared_ptr<event_queue_t> eq,
        std::vector<T> v,
        md::callback::async_item_cb<T> cb,
        md::callback::async_cb end_cb)
    {
        auto strand = eq->new_strand<md::callback::cb_error>(false);
        for(auto i = 0U; i < v.size(); ++i){
            strand->push_back([strand, cb, it = v[i]]() -> void {
                if(strand->data()){
                    strand->requeue_self_back();
                    strand->activate();
                    return;
                }
                
                auto cb_called = std::make_shared<bool>(false);
                cb(it, (md::callback::async_cb)[strand, cb_called]
                (const md::callback::cb_error& err) -> void {
                    if(*cb_called)
                        md::log::default_logger()->fatal(MD_ERR(
                            "Callback already called once!"
                        ));
                    *cb_called = true;
                    if(err){
                        strand->data(err);
                        strand->requeue_self_last_front();
                        strand->activate();
                        return;
                    }
                    
                    strand->requeue_self_back();
                    strand->activate();
                });
            });
        }
        strand->push_back([strand, end_cb]() -> void {
            end_cb(strand->data());
        });
        strand->requeue_self_back();
        strand->activate();
    }
    
    
    static void series(
        std::vector< md::callback::async_series_cb > cbs,
        md::callback::async_cb end_cb)
    {
        std::shared_ptr<event_queue_t> eq = md::event_queue_t::get_default();
        series(eq, cbs, end_cb);
    }
    /*!
     * 
     *	example: 
     *      md::async::series({
     *          [&](md::async_cb scb) -> void {
     *              return scb(nullptr);
     *          },
     *          [&](md::async_cb scb) -> void {
     *              return scb(nullptr);
     *          },
     *      }, [&](const md::callback::cb_error& err) -> void {
     *          if(err)
     *              return cb(err, cfg);
     *          cb(nullptr, cfg);
     *      });
     */
    static void series(
        std::shared_ptr<event_queue_t> eq,
        std::vector< md::callback::async_series_cb > cbs,
        md::callback::async_cb end_cb)
    {
        if(cbs.size() == 0){
            eq->push_back(std::bind(end_cb, nullptr));
            return;
        }
        
        auto strand = eq->new_strand<md::callback::cb_error>(false);
        for(auto i = 0U; i < cbs.size(); ++i){
            strand->push_back([strand, cb = cbs[i]]() -> void {
                if(strand->data()){
                    strand->requeue_self_back();
                    strand->activate();
                    return;
                }
                
                auto cb_called = std::make_shared<bool>(false);
                cb((md::callback::async_cb)[strand, cb_called]
                (const md::callback::cb_error& err) -> void {
                    if(*cb_called)
                        md::log::default_logger()->fatal(MD_ERR(
                            "Callback already called once!"
                        ));
                    *cb_called = true;
                    if(err){
                        strand->data(err);
                        strand->requeue_self_last_front();
                        strand->activate();
                        return;
                    }
                    
                    strand->requeue_self_back();
                    strand->activate();
                });
            });
        }
        
        strand->push_back([strand, end_cb]() -> void {
            end_cb(strand->data());
        });
        strand->requeue_self_back();
        strand->activate();
    }
    
    template<typename T>
    static void waterfall(
        std::vector< md::callback::async_waterfall_cb<T> > cbs,
        md::callback::value_cb<T> end_cb)
    {
        std::shared_ptr<event_queue_t> eq = md::event_queue_t::get_default();
        waterfall<T>(eq, cbs, end_cb);
    }
    
    template<typename T>
    static void waterfall(
        std::shared_ptr<event_queue_t> eq,
        std::vector< md::callback::async_waterfall_cb<T> > cbs,
        md::callback::value_cb<T> end_cb)
    {
        auto sdata = std::make_shared<waterfall_data_t<T>>();
        if(cbs.size() == 0){
            eq->push_back(std::bind(end_cb, nullptr, sdata->data));
            return;
        }
        
        auto strand = eq->new_strand<waterfall_data<T>>(false);
        strand->data(sdata);
        
        for(auto i = 0U; i < cbs.size(); ++i){
            strand->push_back([strand, cb = cbs[i]]() -> void {
                if(strand->data()->err){
                    strand->requeue_self_back();
                    strand->activate();
                    return;
                }
                
                auto cb_called = std::make_shared<bool>(false);
                cb(
                    strand->data()->data,
                    
                    //(md::callback::value_cb<T>)
                    [strand, cb_called]
                    (const md::callback::cb_error& err, T new_data) -> void {
                        if(*cb_called)
                            md::log::default_logger()->fatal(MD_ERR(
                                "Callback already called once!"
                            ));
                        *cb_called = true;
                        if(err){
                            strand->data()->err = err;
                            strand->requeue_self_last_front();
                            strand->activate();
                            return;
                        }
                        
                        strand->data()->data = new_data;
                        strand->requeue_self_back();
                        strand->activate();
                    }
                );
            });
        }
        strand->push_back([strand, end_cb]() -> void {
            end_cb(strand->data()->err, strand->data()->data);
        });
        strand->requeue_self_back();
        strand->activate();
    }
    
    static void loop(
        md::callback::continue_cb cont_cb,
        md::callback::async_series_cb cb,
        md::callback::async_cb end_cb)
    {
        std::shared_ptr<event_queue_t> eq = md::event_queue_t::get_default();
        loop(eq, cont_cb, cb, end_cb);
    }
    
    static void loop(
        std::shared_ptr<event_queue_t> eq,
        md::callback::continue_cb cont_cb,
        md::callback::async_series_cb cb,
        md::callback::async_cb end_cb)
    {
        auto strand = eq->new_strand<md::callback::cb_error>(false);
        if(!cont_cb)
            cont_cb = []()->bool{ return true;};
        if(!end_cb)
            end_cb = [](const md::callback::cb_error&){};
        _call_loop(strand, cont_cb, cb, end_cb);
    }
    
    static void loop(
        md::callback::async_series_cb cb,
        md::callback::continue_cb cont_cb,
        md::callback::async_cb end_cb)
    {
        std::shared_ptr<event_queue_t> eq = md::event_queue_t::get_default();
        loop(eq, cb, cont_cb, end_cb);
    }
    
    static void loop(
        std::shared_ptr<event_queue_t> eq,
        md::callback::async_series_cb cb,
        md::callback::continue_cb cont_cb,
        md::callback::async_cb end_cb)
    {
        auto strand = eq->new_strand<md::callback::cb_error>(false);
        if(!cont_cb)
            cont_cb = []()->bool{ return true;};
        if(!end_cb)
            end_cb = [](const md::callback::cb_error&){};
        _call_loop(strand, cb, cont_cb, end_cb);
    }
    
private:
    static void _call_loop(
        md::event_strand<md::callback::cb_error> strand,
        md::callback::continue_cb cont_cb,
        md::callback::async_series_cb cb,
        md::callback::async_cb end_cb)
    {
        strand->push_back(
        [strand, cont_cb, cb, end_cb](){
            if(!cont_cb()){
                strand->push_back([strand, end_cb]() -> void {
                    end_cb(nullptr);
                });
                strand->requeue_self_last_front();
                strand->activate();
                return;
            }
            
            auto cb_called = std::make_shared<bool>(false);
            cb((md::callback::async_cb)
            [strand, cont_cb, cb, end_cb, cb_called]
            (const md::callback::cb_error& err) -> void {
                if(*cb_called)
                    md::log::default_logger()->fatal(MD_ERR(
                        "Callback already called once!"
                    ));
                *cb_called = true;
                if(err){
                    strand->push_back([strand, end_cb]() -> void {
                        end_cb(strand->data());
                    });
                    
                    strand->data(err);
                    strand->requeue_self_last_front();
                    strand->activate();
                    return;
                }
                
                _call_loop(strand, cont_cb, cb, end_cb);
            });
        });
        strand->requeue_self_back();
        strand->activate();
    }
    
    static void _call_loop(
        md::event_strand<md::callback::cb_error> strand,
        md::callback::async_series_cb cb,
        md::callback::continue_cb cont_cb,
        md::callback::async_cb end_cb)
    {
        strand->push_back(
        [strand, cont_cb, cb, end_cb](){
            auto cb_called = std::make_shared<bool>(false);
            cb((md::callback::async_cb)
            [strand, cont_cb, cb, end_cb, cb_called]
            (const md::callback::cb_error& err) -> void {
                if(*cb_called)
                    md::log::default_logger()->fatal(MD_ERR(
                        "Callback already called once!"
                    ));
                *cb_called = true;
                
                if(err){
                    strand->push_back([strand, end_cb]() -> void {
                        end_cb(strand->data());
                    });
                    
                    strand->data(err);
                    strand->requeue_self_last_front();
                    strand->activate();
                    return;
                }
                
                if(!cont_cb()){
                    strand->push_back([strand, end_cb]() -> void {
                        end_cb(nullptr);
                    });
                    strand->requeue_self_last_front();
                    strand->activate();
                    return;
                }
                
                _call_loop(strand, cb, cont_cb, end_cb);
            });
        });
        strand->requeue_self_back();
        strand->activate();
    }
    
};

template<
    typename Iterator,
    typename T
>
void event_queue_t::each(
    Iterator first, Iterator last,
    T cb,
    md::callback::async_cb end_cb)
{
    typedef typename std::iterator_traits<Iterator>::value_type U;
    md::callback::async_item_cb<U> aicb = cb;
    md::async::each<Iterator,U>(
        this->shared_from_this(), first, last, aicb, end_cb
    );
}

}//::md
#include "event_queue_impl.h"
#endif //_tools_md_async_h
