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

#ifndef _tools_md_logging_h
#define _tools_md_logging_h

#include "stable_headers.h"
#include "text.h"
#include "files.h"

#include "fmt/format.h"

#include <sstream>
#include <iomanip>

namespace md { namespace log{

class logger;
typedef std::shared_ptr<logger> sp_logger;
md::log::sp_logger& default_logger();

enum class log_level
{
    off             = 0,
    audit_failed    = 1,
    audit_succeeded = 2,
    fatal           = 3,
    error           = 4,
    warning         = 5,
    info            = 6,
    debug           = 7,
    trace           = 8
};

inline md::string_view to_string(log_level lvl)
{
    switch(lvl){
        case log_level::audit_failed: return "AUDIT-FAILED";
        case log_level::audit_succeeded: return "AUDIT-SUCCEEDED";
        case log_level::fatal: return "FATAL";
        case log_level::error: return "ERROR";
        case log_level::warning: return "WARNING";
        case log_level::info: return "INFO";
        case log_level::debug: return "DEBUG";
        case log_level::trace: return "TRACE";
        case log_level::off: return "OFF";
        default: return "N/A";
    }
}

namespace sinks{
    
    class logger_sink;
    typedef std::shared_ptr<logger_sink> sp_logger_sink;
    
    class logger_sink
    {
    public:
        logger_sink(log_level lvl)
            : _lvl(lvl), _flush_on_lvl(md::log::log_level::warning)
        {
        }
        
        bool should_log(log_level lvl) const
        {
            return lvl <= _lvl;
        }
        
        virtual void log(
            md::string_view log_path, log_level lvl,
            md::string_view msg) const = 0;
        
        void set_level(log_level lvl)
        {
            _lvl = lvl;
        }
        
        log_level level() const
        {
            return _lvl;
        }
        
        virtual void flush() const {}
        
        void flush_on(log_level lvl)
        {
            _flush_on_lvl = lvl;
        }
        
    protected:
        log_level _lvl;
        log_level _flush_on_lvl;
    };
} // ns: md::log::sinks

namespace _internal{
    
    static std::string get_timestamp();
    
} // ns: md::_internal


class logger
    : public std::enable_shared_from_this<logger>,
    public sinks::logger_sink
{
private:
    
    logger(const std::shared_ptr<const logger>& parent, md::string_view path)
        : sinks::logger_sink(parent->_lvl),
        _parent(parent),
        _path(path.data())
    {
    }
    
public:

    logger(md::string_view path, log_level lvl = log_level::info)
        : sinks::logger_sink(lvl), _parent(), _path(path.data())
    {
    }

    logger(md::string_view path, md::log::sinks::sp_logger_sink snk,
        log_level lvl = log_level::info)
        : sinks::logger_sink(lvl), _parent(), _path(path.data()),
        _sinks{snk}
    {
    }
    
    template<class IT>
    logger(md::string_view path, IT _begin, IT _end,
        log_level lvl = log_level::info)
        : sinks::logger_sink(lvl), _parent(), _path(path.data()),
        _sinks(_begin, _end)
    {
    }
    
    bool is_child() const { return (bool)_parent;}
    
    std::string path() const
    {
        return _path;
    }
    
    //sp_logger add_child(const std::string& path) const
    sp_logger add_child(md::string_view path) const
    {
        std::string np;
        if(_path[_path.size() -1] != '/' && path[0] != '/')
            np = _path + "/" + path.to_string();
        else if(_path[_path.size() -1] == '/' && path[0] == '/')
            np = _path + path.to_string().substr(1);
        else
            np = _path + path.to_string();
        
        auto p = this->shared_from_this();
        return sp_logger(new logger(p, np.c_str()));
    }
    
    void register_sink(md::log::sinks::sp_logger_sink sink)
    {
        _sinks.emplace_back(sink);
    }
    
    void replace_sink(md::log::sinks::sp_logger_sink sink)
    {
        _sinks.clear();
        if(_parent)
            ((logger*)_parent.get())->replace_sink(sink);
        else
            register_sink(sink);
    }
    
    void log(
        md::string_view log_path,
        log_level lvl,
        md::string_view msg) const
    {
        if(this->_parent)
            return this->_parent->log(log_path, lvl, msg);
        
        for(auto& sink : _sinks){
            if(sink->should_log(lvl))
                sink->log(log_path, lvl, msg);
        }
    }
    
    void log(
        md::string_view log_path,
        log_level lvl,
        md::callback::cb_error err) const
    {
        std::string log_val = err.c_str();
        if(err.has_stack())
            log_val += fmt::format(
                "\n\nAdditional info\n\n{}:{}\n{}\n\n{}\n",
                err.file(), err.line(), err.func(), err.stack()
            );
        log(log_path, lvl, log_val);
    }
    
    void flush() const
    {
        if(this->_parent)
            this->_parent->flush();
        
        for(auto& sink : _sinks)
            sink->flush();
    }
    
    void trace(md::string_view f) const
    {
        if(should_log(log_level::trace))
            log(_path, log_level::trace, f);
    }
    template <typename... Args>
    void trace(md::string_view f, const Args&... args) const
    {
        if(should_log(log_level::trace))
            log(_path, log_level::trace, fmt::format(f.data(), args...));
    }
    void trace(md::callback::cb_error err)
    {
        if(should_log(log_level::trace))
            log(_path, log_level::trace, err);
    }
    
    void debug(md::string_view f) const
    {
        if(should_log(log_level::debug))
            log(_path, log_level::debug, f);
    }
    template <typename... Args>
    void debug(md::string_view f, const Args&... args) const
    {
        if(should_log(log_level::debug))
            log(_path, log_level::debug, fmt::format(f.data(), args...));
    }
    void debug(md::callback::cb_error err)
    {
        if(should_log(log_level::debug))
            log(_path, log_level::debug, err);
    }
    
    void info(md::string_view f) const
    {
        if(should_log(log_level::info))
            log(_path, log_level::info, f);
    }
    template <typename... Args>
    void info(md::string_view f, const Args&... args) const
    {
        if(should_log(log_level::info))
            log(_path, log_level::info, fmt::format(f.data(), args...));
    }
    void info(md::callback::cb_error err)
    {
        if(should_log(log_level::info))
            log(_path, log_level::info, err);
    }
    
    void warn(md::string_view f) const
    {
        if(should_log(log_level::warning))
            log(_path, log_level::warning, f);
    }
    template <typename... Args>
    void warn(md::string_view f, const Args&... args) const
    {
        if(should_log(log_level::warning))
            log(_path, log_level::warning, fmt::format(f.data(), args...));
    }
    void warn(md::callback::cb_error err)
    {
        if(should_log(log_level::warning))
            log(_path, log_level::warning, err);
    }
    
    void error(md::string_view f) const
    {
        if(should_log(log_level::error))
            log(_path, log_level::error, f);
    }
    template <typename... Args>
    void error(md::string_view f, const Args&... args) const
    {
        if(should_log(log_level::error))
            log(_path, log_level::error, fmt::format(f.data(), args...));
    }
    void error(md::callback::cb_error err)
    {
        if(should_log(log_level::error))
            log(_path, log_level::error, err);
    }
    
    void fatal(md::string_view f) const
    {
        if(should_log(log_level::fatal))
            log(_path, log_level::fatal, f);
    }
    template <typename... Args>
    void fatal(md::string_view f, const Args&... args) const
    {
        if(should_log(log_level::fatal))
            log(_path, log_level::fatal, fmt::format(f.data(), args...));
        
        std::exit(-1);
    }
    void fatal(md::callback::cb_error err)
    {
        if(should_log(log_level::fatal))
            log(_path, log_level::fatal, err);
    }
    
    void success(md::string_view f) const
    {
        if(should_log(log_level::audit_succeeded))
            log(_path, log_level::audit_succeeded, f);
    }
    template <typename... Args>
    void success(md::string_view f, const Args&... args) const
    {
        if(should_log(log_level::audit_succeeded))
            log(
                _path, log_level::audit_succeeded,
                fmt::format(f.data(), args...)
            );
    }
    void success(md::callback::cb_error err)
    {
        if(should_log(log_level::audit_succeeded))
            log(_path, log_level::audit_succeeded, err);
    }

    void fail(md::string_view f) const
    {
        if(should_log(log_level::audit_failed))
            log(_path, log_level::audit_failed, f);
    }
    template <typename... Args>
    void fail(md::string_view f, const Args&... args) const
    {
        if(should_log(log_level::audit_failed))
            log(_path, log_level::audit_failed, fmt::format(f.data(), args...));
    }
    void fail(md::callback::cb_error err)
    {
        if(should_log(log_level::audit_failed))
            log(_path, log_level::audit_failed, err);
    }
    
private:
    
    std::shared_ptr<const logger> _parent;
    std::string _path;
    std::vector<sinks::sp_logger_sink> _sinks;
};

namespace sinks{
    class console_sink
        : public logger_sink
    {
    public:
        console_sink(bool enable_color)
            : logger_sink(log_level::info), color(enable_color)
        {
        }
        
        void log(
            md::string_view log_path,
            log_level lvl, md::string_view msg) const
        {
            std::clog <<
                _gen_header(color, log_path, lvl) <<
                md::replace_substring_copy(msg.to_string(), "\n", "\n  ")
                << _gen_footer(color, lvl) << "\n";
        }
        
        bool color;
    private:
        
        static md::string_view _level_color(log_level lvl)
        {
            switch(lvl){
                case log_level::audit_failed:
                    return "31;1";
                case log_level::audit_succeeded:
                    return "34;1";
                case log_level::fatal:
                    return "31;1";
                case log_level::error:
                    return "31";
                case log_level::warning:
                    return "33;1";
                case log_level::info:
                    return "34;1";
                case log_level::debug:
                    return "36";
                case log_level::trace:
                    return "37";
                default:
                    return "37";
            }
        }
        
        static std::string _gen_header(
            bool color, md::string_view log_path, log_level lvl)
        {
            if(color)
                return fmt::format(
                    "\x1b[{}m[{}] [{}:{}]\x1b[0m\n",
                    _level_color(lvl).data(),
                    _internal::get_timestamp(),
                    to_string(lvl).data(),
                    log_path.to_string()
                );
            else
                return fmt::format(
                    "[{}] [{}:{}]\n",
                    _internal::get_timestamp(),
                    to_string(lvl).data(),
                    log_path.to_string()
                );
        }
        
        static std::string _gen_footer(bool color, log_level lvl)
        {
            if(color)
                return fmt::format(
                    "\n\x1b[{}m¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯\x1b[0m",
                    _level_color(lvl).data()
                );
            else
                return "\n¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯";
        }
    };
    
    class rotating_file_sink
        : public logger_sink
    {
    public:
        rotating_file_sink(
            event_base* ev_base,
            bfs::path base_filename,
            size_t max_size, size_t backlog_size)
            : logger_sink(log_level::info),
            _ev_base(ev_base),
            _ifd(-1), _wfd(-1), _fd(-1), _wev(nullptr),
            _base_filename(base_filename),
            _max_size(max_size),
            _backlog_size(backlog_size),
            _rotating(false)
        {
            _open_log();
        }
        
        ~rotating_file_sink()
        {
            if(_ifd > -1){
                close(_ifd);
            }
            
            if(_fd > -1){
                flush();
                close(_fd);
            }
        }
        
        void log(
            md::string_view log_path,
            log_level lvl, md::string_view msg) const
        {
            if(_fd < 0)
                return;
            
            std::string log_str =
                _gen_header(log_path, lvl) + msg.to_string() + "\n";
            md::files::writen(_fd, log_str.c_str(), log_str.size());
        }
        
        void flush() const
        {
            if(_fd < 0)
                return;
            fsync(_fd);
        }

    private:
        
        static void _on_inotify(int fd, short events, void* arg)
        {
            // clear the notify flag.
            struct inotify_event ie;
            while(true){
                ssize_t l = read(fd, &ie, sizeof(ie));
                if(l == -1)
                    break;
            }
            
            if((events & IN_IGNORED) == IN_IGNORED)
                return;
            
            rotating_file_sink* self = (rotating_file_sink*)arg;

            if(self->_rotating)
                return;
            
            struct stat sb;
            fstat(self->_fd, &sb);
            if((size_t)sb.st_size > self->_max_size)
                self->_rotate_log();
        }
        
        void _open_log()
        {
            _fd = open(
                _base_filename.c_str(),
                O_APPEND | O_CREAT | O_WRONLY,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP//ug+wr
            );
            _notify_start();
        }
        
        void _rotate_log()
        {
            if(_rotating)
                return;
            _rotating = true;
            
            _notify_close();
            
            auto pp = _base_filename;
            size_t gz_backlog_size = std::max(
                (size_t)std::floor((float)_backlog_size / 2.0f),
                (size_t)2
            );
            for(size_t i = _backlog_size -1; i > 0; --i){
                bool is_gz = false;
                std::string cur_ext = ("." + md::num_to_str(i, false));
                
                auto blf = pp.string() + cur_ext;
                if(!bfs::exists(blf) || !bfs::is_regular_file(blf)){
                    blf = pp.string() + cur_ext + ".gz";
                    if(!bfs::exists(blf) || !bfs::is_regular_file(blf))
                        continue;
                    is_gz = true;
                }
                
                if(i == _backlog_size -1)
                    bfs::remove(blf);
                else{
                    std::string new_ext = ("." + md::num_to_str(i+1, false));
                    if(i < gz_backlog_size)
                        bfs::rename(
                            blf,
                            pp.string() + new_ext + (is_gz ? ".gz" : "")
                        );
                    else{
                        auto new_blf = pp.string() + new_ext + ".gz";
                        bfs::rename(
                            blf,
                            pp.string() + new_ext + ".gz"
                        );
                        
                        try{
                            md::files::gzip_file(new_blf, new_blf);
                            /*
                            std::string tmp_log = 
                                (pp.parent_path()/bfs::unique_path()).string();
                            
                            md::gzip_file(
                                this->_ev_base,
                                new_blf, tmp_log,
                            [new_blf, tmp_log](md::callback::cb_error err){
                                if(err){
                                    bfs::remove(tmp_log);
                                    bfs::remove(new_blf);
                                    _internal::default_logger()->warn(
                                        err.c_str()
                                    );
                                    return;
                                }
                                
                                bfs::remove(new_blf);
                                bfs::rename(tmp_log, new_blf);
                            });
                            */
                        }catch(const std::exception& err){
                            md::log::default_logger()->warn(
                                md::callback::cb_error(err)
                            );
                        }
                    }
                }
            }
            bfs::rename(_base_filename, pp.string() + ".1");
            
            int tfd = _fd;
            _fd = open(
                _base_filename.c_str(),
                O_APPEND | O_CREAT | O_WRONLY,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP//ug+wr
            );
            
            fsync(tfd);
            close(tfd);
            
            _notify_start();
            _rotating = false;
        }
        
        void _close_log()
        {
            _notify_close();
            fsync(_fd);
            close(_fd);
        }
        
        void _notify_start()
        {
            _ifd = inotify_init1(IN_NONBLOCK);
            _wfd = inotify_add_watch(
                _ifd, _base_filename.c_str(), IN_ATTRIB | IN_MODIFY
            );
            
            _wev = event_new(
                _ev_base, _ifd,
                EV_READ | EV_PERSIST, _on_inotify,
                this
            );
            
            struct timeval tv = {10,0};
            if(event_add(_wev, &tv) == -1){
                event_free(_wev);
                _wev = nullptr;
                throw MD_ERR("event_add failed!");
            }
        }
        
        void _notify_close()
        {
            if(_wev){
                event_del(_wev);
                event_free(_wev);
                _wev = nullptr;
            }
            
            inotify_rm_watch(_ifd, _wfd);
            _wfd = -1;
            close(_ifd);
        }
        
        static std::string _gen_header(
            md::string_view log_path, log_level lvl)
        {
            return fmt::format(
                "[{}] [{}:{}] ",
                _internal::get_timestamp(),
                to_string(lvl).data(),
                log_path.to_string()
            );
        }
        
        event_base* _ev_base;
        int _ifd;
        int _wfd;
        int _fd;
        event* _wev;
        bfs::path _base_filename;
        size_t _max_size;
        size_t _backlog_size;
        bool _rotating;
    };
    
} // ns: md::log::sinks

namespace _internal{
    
    static std::string get_timestamp()
    {
        const auto currentTime = std::chrono::system_clock::now();
        time_t time = std::chrono::system_clock::to_time_t(currentTime);
        auto currentTimeRounded = std::chrono::system_clock::from_time_t(time);
        if( currentTimeRounded > currentTime ) {
            --time;
            currentTimeRounded -= std::chrono::seconds( 1 );
        }
        //const tm *values = localtime( &time );
        struct tm t;
        localtime_r(&time, &t);
        int hours = t.tm_hour;
        int minutes = t.tm_min;
        int seconds = t.tm_sec;
        // etc.
        int milliseconds =
            std::chrono::duration_cast<std::chrono::duration<int,std::milli>>(
                currentTime - currentTimeRounded
            ).count();
        
        return fmt::format(
            "{0:04}-{1:02}-{2:02} "
            "{3:02}:{4:02}:{5:02}.{6:03}",
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
            hours, minutes, seconds, milliseconds
        );
    }

} // ns: md::_internal

inline md::log::sp_logger& default_logger()
{
    static auto out_snk = std::make_shared<md::log::sinks::console_sink>(
        true
    );
    static md::log::sp_logger _default_logger =
        std::make_shared<md::log::logger>("/", out_snk);
    return _default_logger;
}


inline void trace(md::string_view f)
{
    if(log::default_logger()->should_log(log_level::trace))
        log::default_logger()->log("/", log_level::trace, f);
}
template <typename... Args>
void trace(md::string_view f, const Args&... args)
{
    if(log::default_logger()->should_log(log_level::trace))
        log::default_logger()->log(
            "/", log_level::trace, fmt::format(f.data(), args...)
        );
}

inline void debug(md::string_view f)
{
    if(log::default_logger()->should_log(log_level::debug))
        log::default_logger()->log("/", log_level::debug, f);
}
template <typename... Args>
void debug(md::string_view f, const Args&... args)
{
    if(log::default_logger()->should_log(log_level::debug))
        log::default_logger()->log(
            "/", log_level::debug, fmt::format(f.data(), args...)
        );
}

inline void info(md::string_view f)
{
    if(log::default_logger()->should_log(log_level::info))
        log::default_logger()->log("/", log_level::info, f);
}
template <typename... Args>
void info(md::string_view f, const Args&... args)
{
    if(log::default_logger()->should_log(log_level::info))
        log::default_logger()->log(
            "/", log_level::info, fmt::format(f.data(), args...)
        );
}

inline void warn(md::string_view f)
{
    if(log::default_logger()->should_log(log_level::warning))
        log::default_logger()->log("/", log_level::warning, f);
}
template <typename... Args>
void warn(md::string_view f, const Args&... args)
{
    if(log::default_logger()->should_log(log_level::warning))
        log::default_logger()->log(
            "/", log_level::warning, fmt::format(f.data(), args...)
        );
}

inline void error(md::string_view f)
{
    if(log::default_logger()->should_log(log_level::error))
        log::default_logger()->log("/", log_level::error, f);
}
template <typename... Args>
void error(md::string_view f, const Args&... args)
{
    if(log::default_logger()->should_log(log_level::error))
        log::default_logger()->log(
            "/", log_level::error, fmt::format(f.data(), args...)
        );
}

inline void fatal(md::string_view f)
{
    if(log::default_logger()->should_log(log_level::fatal))
        log::default_logger()->log("/", log_level::fatal, f);
}
template <typename... Args>
void fatal(md::string_view f, const Args&... args)
{
    if(log::default_logger()->should_log(log_level::fatal))
        log::default_logger()->log(
            "/", log_level::fatal, fmt::format(f.data(), args...)
        );
    
    std::exit(-1);
}

inline void success(md::string_view f)
{
    if(log::default_logger()->should_log(log_level::audit_succeeded))
        log::default_logger()->log("/", log_level::audit_succeeded, f);
}
template <typename... Args>
void success(md::string_view f, const Args&... args)
{
    if(log::default_logger()->should_log(log_level::audit_succeeded))
        log::default_logger()->log(
            "/", log_level::audit_succeeded,
            fmt::format(f.data(), args...)
        );
}

inline void fail(md::string_view f)
{
    if(log::default_logger()->should_log(log_level::audit_failed))
        log::default_logger()->log("/", log_level::audit_failed, f);
}
template <typename... Args>
void fail(md::string_view f, const Args&... args)
{
    if(log::default_logger()->should_log(log_level::audit_failed))
        log::default_logger()->log(
            "/", log_level::audit_failed,
            fmt::format(f.data(), args...)
        );
}


}}//::md::log
#endif//_tools_md_logging_h
