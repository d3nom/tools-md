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

#ifndef _tools_md_event_strand_h
#define _tools_md_event_strand_h

#include "event_queue.h"

#define MD_STRAND_TO_TASKBASE(x)  \
std::static_pointer_cast<md::event_task_base_t>( \
    std::static_pointer_cast<md::event_strand_t<T>>(x) \
)

namespace md{

template<typename T = int>
class event_strand_t
    : public event_queue_t, public event_task_base_t//, 
    //public std::enable_shared_from_this< event_strand_t<T> >
{
    friend class md::event_queue_t;

    template< typename Task >
    friend uint64_t _event_strand_push_back(event_queue_t* eq, Task task);
    friend uint64_t _event_strand_push_back(
        event_queue_t* eq, event_task task
    );

    template< typename Task >
    friend uint64_t _event_strand_push_front(event_queue_t* eq, Task task);
    friend uint64_t _event_strand_push_front(
        event_queue_t* eq, event_task task
    );
    
public:
    event_strand_t(bool auto_requeue = true, bool activate_on_requeue = true)
        : event_queue_t(),
        event_task_base_t(event_queue::get_default()),
        _auto_requeue(auto_requeue),
        _activate_on_requeue(activate_on_requeue)
    {
    }
    
    event_strand_t(event_queue_t* owner, bool auto_requeue = true)
        : event_queue_t(),
        event_task_base_t(owner),
        _auto_requeue(auto_requeue)
    {
    }
    
    virtual ~event_strand_t()
    {
        for(size_t i = 0; i < _tasks.size(); ++i){
            _tasks[i]->_owner = this->_owner;
            this->_owner->push_back(_tasks[i]);
        }
    }
    
    event_base* ev_base() const
    {
        return this->_owner->_ev_base;
    }
    
    void activate()
    {
        this->_owner->activate();
    }
    
    void enable_activate_on_requeue(bool activate_on_requeue)
    {
        _activate_on_requeue = activate_on_requeue;
    }
    bool activate_on_requeue() const { return _activate_on_requeue;}
    
    virtual bool force_push() const { return true;}
    virtual size_t size() const
    {
        return _tasks.size();
    }
    
    virtual event_requeue_pos requeue() const
    {
        if(_auto_requeue && this->size() > 0)
            return event_requeue_pos::back;
        
        return event_requeue_pos::none;
    }
    
    T data(){ return _data;}
    void data(T val){ _data = val;}
    
    template< typename Task >
    uint64_t push_back(Task task)
    {
        MD_LOCK_EVENT_QUEUE;
        
        auto id = _event_queue_push_back(this, task);
        
        if(_auto_requeue)
            this->_owner->push_back(
                MD_STRAND_TO_TASKBASE(this->shared_from_this())
            );
        return id;
    }
    
    template< typename Task >
    uint64_t push_front(Task task)
    {
        MD_LOCK_EVENT_QUEUE;

        auto id = _event_queue_push_front(this, task);
        if(_auto_requeue)
            this->_owner->push_back(
                MD_STRAND_TO_TASKBASE(this->shared_from_this())
            );
        return id;
    }
    
    virtual void run_task()
    {
        this->run_n();
    }
    
    void requeue_self_back()
    {
        this->_owner->push_back(
            MD_STRAND_TO_TASKBASE(this->shared_from_this())
        );
    }
    
    void requeue_self_front()
    {
        this->_owner->push_front(
            MD_STRAND_TO_TASKBASE(this->shared_from_this())
        );
    }
    
    void requeue_self_last_front()
    {
        this->_tasks.erase(
            this->_tasks.begin(),
            this->_tasks.begin() + (this->_tasks.size() -1)
        );
        this->_owner->push_front(
            MD_STRAND_TO_TASKBASE(this->shared_from_this())
        );
    }

    
private:
    #ifdef MD_THREAD_SAFE
    mutable std::mutex _mutex;
    #endif

    bool _auto_requeue;
    bool _activate_on_requeue;
    T _data;
};




}//::md
#endif //_tools_md_event_strand_h
