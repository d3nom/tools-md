cmake_minimum_required(VERSION 3.0.0)
cmake_policy(SET CMP0054 NEW)
project(libtools-md VERSION 0.1.0 LANGUAGES CXX C)

# deps
if(NOT TARGET date)
    subdirs(deps/date)
endif()
if(NOT TARGET fmt)
    subdirs(deps/fmt)
endif()
# if(NOT TARGET json) # not working with debian stretch
#     subdirs(deps/json)
# endif()

if(NOT TARGET gmock)
    subdirs(deps/googletest)
endif()

# lib dir

# tests
if(MD_BUILD_TESTS)
    subdirs(tests-md)
    add_test(alltests ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libtools-md_tests)
    enable_testing()
endif()

# doc
if(MD_BUILD_DOC)
    subdirs(doc-md)
endif()

###################
### libtools-md ###
###################

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/md_config.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/tools-md/md_config.h"
)

file(GLOB MD_HEADERS "${PROJECT_SOURCE_DIR}/include/tools-md/*.h")
source_group("Headers" FILES ${MD_HEADERS})
