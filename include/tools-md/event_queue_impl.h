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

#include "stable_headers.h"
#include "event_queue.h"
#include "async.h"

namespace md {

inline uint64_t get_event_task_id()
{
    static std::atomic<uint64_t> _next_task_id(10UL);
    return ++_next_task_id;
}

inline void event_task_base_t::switch_owner(
    event_queue_t* new_owner, bool requeue)
{
    if(!_owner || !new_owner)
        throw MD_ERR("Owner can't be NULL");
    
    if(requeue)
        _owner->requeue_task(new_owner, this);
    else
        _owner = new_owner;
}

inline void event_queue_t::series(
    std::vector< md::callback::async_series_cb > cbs,
    md::callback::async_cb end_cb)
{
    md::async::series(this->shared_from_this(), cbs, end_cb);
}


inline uint64_t _event_queue_push_back(
    event_queue_t* eq, event_task tp_task)
{
    if(tp_task->activate_on_requeue())
        eq->activate();
    if(tp_task->owner() != eq)
        throw MD_ERR(
            "Can't requeue a task created on another event_queue!\n"
            "Call the event_task::switch_owner function instead."
        );
    
    if(tp_task->force_push()){
        eq->_tasks.emplace_back(tp_task);
        return tp_task->id();
    }
    
    auto it = std::find_if(
        eq->_tasks.begin(),
        eq->_tasks.end(),
        [task_id=tp_task->id()](const event_task& t)-> bool {
            return t->id() == task_id;
        }
    );
    
    if(it != eq->_tasks.end())
        return tp_task->id();
    eq->_tasks.emplace_back(tp_task);
    return tp_task->id();
}

inline uint64_t _event_queue_push_front(
    event_queue_t* eq, event_task tp_task)
{
    if(tp_task->activate_on_requeue())
        eq->activate();
    if(tp_task->owner() != eq)
        throw MD_ERR(
            "Can't requeue a task created on another event_queue!\n"
            "Call the event_task::switch_owner function instead."
        );
    
    if(tp_task->force_push()){
        eq->_tasks.emplace_front(tp_task);
        return tp_task->id();
    }
    
    auto it = std::find_if(
        eq->_tasks.begin(),
        eq->_tasks.end(),
        [task_id=tp_task->id()](const event_task& t)-> bool {
            return t->id() == task_id;
        }
    );
    
    if(it != eq->_tasks.end())
        return tp_task->id();
    eq->_tasks.emplace_front(tp_task);
    return tp_task->id();
}

}//::md