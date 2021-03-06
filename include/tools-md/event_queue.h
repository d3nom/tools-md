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

#ifndef _tools_md_event_queue_h
#define _tools_md_event_queue_h

#include <vector>
#include <chrono>
#include <thread>
#include "stable_headers.h"
#include "errors.h"
#include "logging.h"
#include "callbacks.h"

namespace md{

#define MD_TO_TASKBASE(x) std::static_pointer_cast<md::event_task_base_t>(x)

class event_task_base_t;
class event_task_t;
template<typename T>
class event_strand_t;
class event_queue_t;

typedef std::shared_ptr< md::event_queue_t > event_queue;
typedef std::shared_ptr< md::event_task_base_t > event_task;


//typedef std::shared_ptr< md::event_strand_t > event_strand;
template<typename T = int>
using event_strand = std::shared_ptr< md::event_strand_t<T> >;


//typedef md::delegate< void() > event_task_fn;
typedef std::function< void() > event_task_fn;

uint64_t get_event_task_id();

template<typename Task,
    typename std::enable_if<std::is_invocable<Task>::value, int32_t>::type = -1
>
uint64_t _event_queue_push_back(event_queue_t* eq, Task task);
template< typename Task,
    typename std::enable_if<std::is_invocable<Task>::value, int32_t>::type = -1
>
uint64_t _event_queue_push_front(event_queue_t* eq, Task task);

enum class event_requeue_pos
{
    none = 0,
    back = 1,
    front = 2,
};

class event_task_base_t
{
    template<typename T>
    friend class event_strand_t;
    friend class event_queue_t;
    
public:
    event_task_base_t(event_queue_t* owner)
        : _owner(owner), _id(md::get_event_task_id())
    {
        if(!owner)
            throw MD_ERR("Owner can't be NULL");
    }
    virtual ~event_task_base_t(){}
    
    event_queue_t* owner(){ return _owner;}
    uint64_t id() const { return _id;}
    
    virtual bool activate_on_requeue() const { return true;}
    
    virtual bool force_push() const { return false;}
    virtual void switch_owner(event_queue_t* new_owner, bool requeue = false);
    
    virtual void run_task() = 0;
    virtual event_requeue_pos requeue() const = 0;
    virtual size_t size() const = 0;
    
protected:
    event_queue_t* _owner;
    uint64_t _id;
};

class event_task_t
    : public event_task_base_t
{
public:
    event_task_t(event_queue_t* owner, event_task_fn t)
        : event_task_base_t(owner), task(t)
    {
    }
    
    virtual ~event_task_t(){}
    
    virtual void run_task(){ task();}
    virtual event_requeue_pos requeue() const { return event_requeue_pos::none;}
    virtual size_t size() const { return 1;}

private:
    event_task_fn task;
};

#ifdef MD_THREAD_SAFE
#define MD_LOCK_EVENT_QUEUE std::unique_lock<std::mutex> lock(_mutex)
#else
#define MD_LOCK_EVENT_QUEUE 
#endif


class event_queue_t
    : public std::enable_shared_from_this<event_queue_t>
{
    friend class event_task_base_t;
    template<typename T>
    friend class event_strand_t;

    template<typename Task,
    typename std::enable_if<std::is_invocable<Task>::value, int32_t>::type
    >
    friend uint64_t _event_queue_push_back(event_queue_t* eq, Task task);
    friend uint64_t _event_queue_push_back(event_queue_t* eq, event_task task);

    template<typename Task,
    typename std::enable_if<std::is_invocable<Task>::value, int32_t>::type
    >
    friend uint64_t _event_queue_push_front(event_queue_t* eq, Task task);
    friend uint64_t _event_queue_push_front(
        event_queue_t* eq, event_task task);
    
protected:
    //// initialize the default event_queue_t
    //event_queue_t* event_queue_t::_default = new event_queue_t();
    static std::shared_ptr<event_queue_t>& _default()
    {
        static std::shared_ptr<event_queue_t> _def;
        return _def;
    }
    
public:
    static std::shared_ptr<event_queue_t> get_default()
    {
        if(!_default())
            _default() = std::make_shared<event_queue_t>();
        return _default();
    }
    static void reset(event_base* ev_base)
    {
        _default() = std::make_shared<event_queue_t>(ev_base);
    }
    static void destroy_default()
    {
        _default().reset();
    }
    
    event_queue_t(event_base* ev_base = nullptr)
        : _ev_base(ev_base), _ev(nullptr)
    {
        if(!_ev_base)
            return;
        
        _ev = event_new(
            _ev_base, -1,
            EV_READ | EV_PERSIST,
            [](int fd, short events, void* arg){
                event_queue_t* eq = (event_queue_t*)arg;
                eq->run_n(eq->_tasks.size());
            },
            this
        );
    }
    
    /// @brief Destructor.
    virtual ~event_queue_t()
    {
        if(_ev)
            event_free(_ev);
        _ev = nullptr;
    }
    
    virtual event_base* ev_base() const
    {
        return _ev_base;
    }
    
    virtual void activate()
    {
        if(_ev)
            event_active(_ev, EV_READ, 1);
    }
    
    #ifdef MD_THREAD_SAFE
    virtual bool is_thread_safe()
    {
        return true;
    }
    #else
    virtual bool is_thread_safe()
    {
        return false;
    }
    #endif
    
    size_t local_size() const
    {
        MD_LOCK_EVENT_QUEUE;
        return _tasks.size();
    }
    
    virtual size_t size() const
    {
        MD_LOCK_EVENT_QUEUE;
        if(_tasks.empty())
            return 0;
        
        uint32_t sum = 0;
        for(auto& t : _tasks)
            sum += t->size();
        return sum;
    }
    
    template<typename T = int>
    event_strand<T> new_strand(bool auto_requeue = true)
    {
        return std::make_shared<event_strand_t<T>>(this, auto_requeue);
    }
    
    template< typename Task >
    uint64_t push_back(Task task)
    {
        MD_LOCK_EVENT_QUEUE;
        return _event_queue_push_back(this, task);
    }	
    
    template< typename Task >
    uint64_t push_front(Task task)
    {
        MD_LOCK_EVENT_QUEUE;
        return _event_queue_push_front(this, task);
    }

    bool cancel_task(uint64_t task_id)
    {
        MD_LOCK_EVENT_QUEUE;
        
        size_t ts = _tasks.size();
        if(ts == 0)
            return false;
        
        _tasks.erase(
            std::remove_if(
                _tasks.begin(), _tasks.end(),
                [task_id](const event_task& t)-> bool {
                    return t->id() == task_id;
                }
            )
        );
        return ts > _tasks.size();
    }

    template<
        typename Iterator,
        typename T// = typename std::iterator_traits<Iterator>::value_type
    >
    void each(
        Iterator first, Iterator last,
        T cb, //async_item_cb<T> cb,
        md::callback::async_cb end_cb);
    
    /*!
     * 
     *  \code
     *      //example: 
     *      md::async::series({
     *          [&](md::async_cb scb) -> void {
     *              return scb(nullptr);
     *          },
     *          [&](md::async_cb scb) -> void {
     *              return scb(nullptr);
     *          },
     *      }, [&](const md::cb_error& err) -> void {
     *          if(err)
     *              return cb(err, cfg);
     *          cb(md::cb_error::no_err, cfg);
     *      });
     *  \endcode
     */
    void series(
        std::vector< md::callback::async_series_cb > cbs,
        md::callback::async_cb end_cb
    );
    
    virtual void run_n(uint32_t count = 1)
    {
    #ifdef MD_THREAD_SAFE
        std::unique_lock<std::mutex> lock(_mutex);
        if(_tasks.empty())
            return;
        
        if(count <= 1){
            event_task t = _tasks.front();
            _tasks.pop_front();
            lock.unlock();
            run_event_task(t);
            return;
        }
        
        if(count > _tasks.size())
            count = _tasks.size();
        std::deque< event_task > tmp_tasks(count);
        std::move(
            _tasks.begin(), _tasks.begin() + count,
            std::back_inserter(tmp_tasks)
        );
        _tasks.erase(_tasks.begin(), _tasks.begin() + count);
        lock.unlock();
        
        for(size_t i = 0; i < tmp_tasks.size(); ++i)
            run_event_task(tmp_tasks[i]);
            
    #else
        if(_tasks.empty())
            return;
        
        if(count <= 1){
            event_task t = _tasks.front();
            _tasks.pop_front();
            run_event_task(t);
            return;
        }
        
        if(count > _tasks.size())
            count = _tasks.size();
        std::deque< event_task > tmp_tasks(count);
        std::move(
            _tasks.begin(), _tasks.begin() + count,
            std::back_inserter(tmp_tasks)
        );
        _tasks.erase(_tasks.begin(), _tasks.begin() + count);
        for(size_t i = 0; i < tmp_tasks.size(); ++i)
            run_event_task(tmp_tasks[i]);
    #endif
    }
    
    virtual void run(uint32_t usec_wait = 1)
    {
        do{
    #ifdef MD_THREAD_SAFE
            std::unique_lock<std::mutex> lock(_mutex);
            std::deque< event_task > tmp_tasks = std::move(_tasks);
            lock.unlock();
            for(size_t i = 0; i < tmp_tasks.size(); ++i)
                run_event_task(tmp_tasks[i]);
            lock.lock();
            if(_tasks.size() == 0){
                lock.unlock();
                break;
            }
            lock.unlock();
    #else
            std::deque< event_task > tmp_tasks = std::move(_tasks);
            for(size_t i = 0; i < tmp_tasks.size(); ++i)
                run_event_task(tmp_tasks[i]);
            if(_tasks.size() == 0)
                break;
    #endif
            if(usec_wait > 0)
                std::this_thread::sleep_for(
                    std::chrono::microseconds(usec_wait)
                );
        }while(true);
    }

private:
    void requeue_task(event_queue_t* new_owner, event_task_base_t* task)
    {
        MD_LOCK_EVENT_QUEUE;
        
        _tasks.erase(
            std::remove_if(
                _tasks.begin(), _tasks.end(),
                [new_owner, task_id=task->id()]
                (const event_task& t)-> bool {
                    if(t->id() == task_id){
                        new_owner->push_back(t);
                        return true;
                    }
                    return false;
                }
            )
        );
        task->switch_owner(new_owner, false);
        new_owner->activate();
    }
    
    void run_event_task(event_task& t)
    {
        if(!t)
            return;
        
        t->run_task();
        event_requeue_pos pos = t->requeue();
        if(pos == event_requeue_pos::none)
            return;
        
        if(t->activate_on_requeue())
            this->activate();
        if(pos == event_requeue_pos::back){
            MD_LOCK_EVENT_QUEUE;
            _tasks.emplace_back( t );
        }
        if(pos == event_requeue_pos::front){
            MD_LOCK_EVENT_QUEUE;
            _tasks.emplace_front( t );
        }
    }
    
    #ifdef MD_THREAD_SAFE
    mutable std::mutex _mutex;
    #endif
    std::deque< event_task > _tasks;
    
    event_base* _ev_base;
    event* _ev;
};


template<typename Task,
    typename std::enable_if<std::is_invocable<Task>::value, int32_t>::type
>
uint64_t _event_queue_push_back(event_queue_t* eq, Task task)
{
    eq->activate();
    event_task t(new event_task_t(eq, md::event_task_fn(task)));
    eq->_tasks.emplace_back(t);
    return t->id();
}

template<typename Task,
    typename std::enable_if<std::is_invocable<Task>::value, int32_t>::type
>
uint64_t _event_queue_push_front(event_queue_t* eq, Task task)
{
    eq->activate();
    event_task t(new event_task_t(eq, md::event_task_fn( task )));
    eq->_tasks.emplace_front(t);
    return t->id();
}

uint64_t _event_queue_push_back(event_queue_t* eq, event_task tp_task);
uint64_t _event_queue_push_front(event_queue_t* eq, event_task tp_task);


} //::md
#endif //_tools_md_event_queue_h
