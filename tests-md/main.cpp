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

//#include "args.h"

#include <gmock/gmock.h>
#include "md_test_env.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

int md_tests_log_level;

int main(int argc, char** argv)
{
    po::options_description desc("tools-md_tests");
    
	std::string verbosity_values;
    desc.add_options()
        ("help,h", "produce help message")
		("verbosity,v",
            po::value(&verbosity_values)->implicit_value(""),
            "defined verbosity level"
        )
        ;
    
    po::variables_map vm;
    po::store(
        po::command_line_parser(argc, argv)
        .options(desc)
        .run(),
        vm
    );
    
    po::notify(vm);
    
    if (vm.count("help")) {
        std::cout << "Usage: tools-md_tests [options]\n"
            << desc << std::endl;
        return 0;
    }
    
    if(vm.count("verbosity"))
        verbosity_values += "v";
    if(
        std::any_of(
            begin(verbosity_values), end(verbosity_values), [](auto& c){
                return c != 'v';
            }
        )
    ){
        throw std::runtime_error("Invalid verbosity level");
    }
    
    md_tests_log_level = verbosity_values.size() +4;
    if((md::log::log_level)md_tests_log_level > md::log::log_level::trace)
        md_tests_log_level = (int)md::log::log_level::trace;
    
    testing::InitGoogleMock(&argc, argv);
    testing::AddGlobalTestEnvironment(new md::tests::md_test_env());
    
    int r = RUN_ALL_TESTS();
        
    return r;
}