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

#ifndef _tools_md_files_h
#define _tools_md_files_h

#include "stable_headers.h"
#include "errors.h"

#include <zlib.h>

// http://www.zlib.net/manual.html#Advanced
#define MD_COMPRESSION_NOT_SUPPORTED (-MAX_WBITS - 1000)
#define MD_ZLIB_DEFLATE_WSIZE (-MAX_WBITS)
#define MD_ZLIB_ZLIB_WSIZE MAX_WBITS
#define MD_ZLIB_GZIP_WSIZE (MAX_WBITS | 16)
#define MD_ZLIB_MEM_LEVEL 8
#define MD_ZLIB_STRATEGY Z_DEFAULT_STRATEGY


namespace md{ namespace files{


/* Write "n" bytes to a descriptor. */
ssize_t writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char* ptr;
    
    ptr = (const char*)vptr;
    nleft = n;
    while(nleft > 0){
        if((nwritten = write(fd, ptr, nleft)) <= 0){
            if(errno == EINTR)
                nwritten = 0;/* and call write() again */
            else if(errno == EAGAIN || errno == EWOULDBLOCK){
                fsync(fd);
                nwritten = 0;/* and call write() again */
            }else
                return(-1);/* error */
        }
        
        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n);
}



/*
namespace _internal{
    struct gzip_file{
        event* ev;
        int fd_in;
        int fd_out;
        z_stream* zs;
        uLong zs_size;
        evmvc::async_cb cb;
    };
    
    struct gzip_file* new_gzip_file()
    {
        struct evmvc::_internal::gzip_file* gzf = nullptr;
        mm__alloc_(gzf, struct evmvc::_internal::gzip_file, {
            nullptr, -1, -1, nullptr, 0, nullptr
        });
        
        gzf->zs = (z_stream*)malloc(sizeof(*gzf->zs));
        memset(gzf->zs, 0, sizeof(*gzf->zs));
        return gzf;
    }
    
    void free_gzip_file(struct evmvc::_internal::gzip_file* gzf)
    {
        deflateEnd(gzf->zs);
        event_del(gzf->ev);
        event_free(gzf->ev);
        close(gzf->fd_in);
        close(gzf->fd_out);
        free(gzf->zs);
        free(gzf);
    }
    
    void gzip_file_on_read(int fd, short events, void* arg)
    {
        if((events & IN_IGNORED) == IN_IGNORED)
            return;
        
        gzip_file* gzf = (gzip_file*)arg;
        
        char buf[EVMVC_READ_BUF_SIZE];
        // try to read EVMVC_READ_BUF_SIZE bytes from the file pointer
        do{
            ssize_t bytes_read = read(gzf->fd_in, buf, sizeof(buf));
            
            if(bytes_read == -1){
                int terr = errno;
                if(terr == EAGAIN || terr == EWOULDBLOCK)
                    return;
                auto cb = gzf->cb;
                free_gzip_file(gzf);
                cb(EVMVC_ERR(
                    "read function returned: '{}'",
                    terr
                ));
            }
            
            if(bytes_read > 0){
                gzf->zs->next_in = (Bytef*)buf;
                gzf->zs->avail_in = bytes_read;
                
                // retrieve the compressed bytes blockwise.
                int ret;
                char zbuf[EVMVC_READ_BUF_SIZE];
                bytes_read = 0;
                
                gzf->zs->next_out = reinterpret_cast<Bytef*>(zbuf);
                gzf->zs->avail_out = sizeof(zbuf);
                
                ret = deflate(gzf->zs, Z_SYNC_FLUSH);
                if(ret != Z_OK && ret != Z_STREAM_END){
                    auto cb = gzf->cb;
                    free_gzip_file(gzf);
                    cb(EVMVC_ERR(
                        "zlib deflate function returned: '{}'",
                        ret
                    ));
                    return;
                }
                
                if(gzf->zs_size < gzf->zs->total_out){
                    bytes_read += gzf->zs->total_out - gzf->zs_size;
                    writen(
                        gzf->fd_out,
                        zbuf,
                        gzf->zs->total_out - gzf->zs_size
                    );
                    gzf->zs_size = gzf->zs->total_out;
                }
                
            }else{
                int ret = deflate(gzf->zs, Z_FINISH);
                if(ret != Z_OK && ret != Z_STREAM_END){
                    auto cb = gzf->cb;
                    free_gzip_file(gzf);
                    cb(EVMVC_ERR(
                        "zlib deflate function returned: '{}'",
                        ret
                    ));
                    return;
                }
                
                if(gzf->zs_size < gzf->zs->total_out){
                    bytes_read += gzf->zs->total_out - gzf->zs_size;
                    writen(
                        gzf->fd_out,
                        gzf->zs->next_out,
                        gzf->zs->total_out - gzf->zs_size
                    );
                    gzf->zs_size = gzf->zs->total_out;
                }

                auto cb = gzf->cb;
                free_gzip_file(gzf);
                cb(nullptr);
                return;
            }
        }while(true);
    }
    
}// _internal
void gzip_file(
    event_base* ev_base,
    evmvc::string_view src,
    evmvc::string_view dest,
    evmvc::async_cb cb)
{
    cb(EVMVC_ERR(
        "file async IO Not working with EPOLL!\n"
        "Should be reimplement with aio or thread pool!"
    ));
    struct evmvc::_internal::gzip_file* gzf = _internal::new_gzip_file();
    if(deflateInit2(
        gzf->zs,
        Z_DEFAULT_COMPRESSION,
        Z_DEFLATED,
        EVMVC_ZLIB_GZIP_WSIZE,
        EVMVC_ZLIB_MEM_LEVEL,
        EVMVC_ZLIB_STRATEGY
        ) != Z_OK
    ){
        free(gzf->zs);
        free(gzf);
        cb(EVMVC_ERR("deflateInit2 failed!"));
        return;
    }
    
    gzf->fd_in = open(src.data(), O_NONBLOCK | O_RDONLY);
    gzf->fd_out = open(
        dest.data(),
        O_CREAT | O_WRONLY, 
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP//ug+wr
    );
    gzf->cb = cb;
    
    gzf->ev = event_new(
        ev_base, gzf->fd_in, EV_READ | EV_PERSIST,
        _internal::gzip_file_on_read, gzf
    );
    if(event_add(gzf->ev, nullptr) == -1){
        _internal::free_gzip_file(gzf);
        cb(EVMVC_ERR("evmvc::gzip_file::event_add failed!"));
    }
}
*/

void gzip_file(
    md::string_view src, md::string_view dest)
{
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    if(deflateInit2(
        &zs,
        Z_DEFAULT_COMPRESSION,
        Z_DEFLATED,
        MD_ZLIB_GZIP_WSIZE,
        MD_ZLIB_MEM_LEVEL,
        MD_ZLIB_STRATEGY
        ) != Z_OK
    ){
        throw MD_ERR(
            "deflateInit2 failed!"
        );
    }
    std::ifstream fin(
        src.data(), std::ios::binary
    );
    std::ostringstream ostrm;
    ostrm << fin.rdbuf();
    std::string inf_data = ostrm.str();
    fin.close();
    
    std::string def_data;
    zs.next_in = (Bytef*)inf_data.data();
    zs.avail_in = inf_data.size();
    int ret;
    char outbuffer[32768];
    
    do{
        zs.next_out =
            reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);
        ret = deflate(&zs, Z_FINISH);
        if (def_data.size() < zs.total_out) {
            // append the block to the output string
            def_data.append(
                outbuffer,
                zs.total_out - def_data.size()
            );
        }
    }while(ret == Z_OK);
    
    deflateEnd(&zs);
    if(ret != Z_STREAM_END){
        throw MD_ERR(
            "Error while log compression: ({}) {}",
            ret, zs.msg
        );
    }
    
    std::ofstream fout(
        dest.data(), std::ios::binary
    );
    fout.write(def_data.c_str(), def_data.size());
    fout.flush();
    fout.close();
}
    
    
    
}}//::md
#endif //_tools_md_files_h
