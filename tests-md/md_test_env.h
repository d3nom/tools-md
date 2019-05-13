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

#ifndef _md_test_env_h
#define _md_test_env_h

#include <gmock/gmock.h>
#include <event2/event.h>

#include "tools-md/tools-md.h"

extern int md_tests_log_level;

namespace md{ namespace tests{

class md_test_env
    : public testing::Environment
{
public:
    md_test_env()
        : testing::Environment(),
        _ev_base(nullptr)
    {
    }
    
    void SetUp() override
    {
        std::cout << "Forcing time_zone initialization" << std::endl;
        auto cz = ::date::current_zone();
        std::cout << "Current time zone: " << cz->name() << std::endl;
        
        std::cout << "Initializing the log setting" << std::endl;
        md::log::default_logger()->set_level(
            (md::log::log_level)md_tests_log_level
        );
        
        std::cout << "Initializing the event_queue" << std::endl;
        _ev_base = event_base_new();
        md::event_queue::reset(_ev_base);
    }
    
    void TearDown() override
    {
        std::cout << "Destroying the event_queue" << std::endl;
        md::event_queue::destroy_default();
        event_base_free(_ev_base);
        _ev_base = nullptr;
    }
    
private:
    event_base* _ev_base;
};

}} //namespace md::tests
#endif //_md_test_env_h