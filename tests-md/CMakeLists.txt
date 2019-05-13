
include_directories("${PROJECT_BINARY_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../deps/date/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../deps/fmt/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../deps/json/single_include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../deps/googletest/googletest/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../deps/googletest/googlemock/include"
)

if(MD_BUILD_TESTS)
    # boost dependencies
    set(Boost_ADDITIONAL_VERSIONS "1.62" "1.58" "1.58.0" "1.57" "1.57.0" "1.56" "1.56.0" "1.55" "1.55.0")

    #set(Boost_ROOT "" CACHE PATH "Boost root dir")
    set(Boost_DEBUG  ON CACHE BOOL "Boost_DEBUG")
    set(Boost_USE_STATIC_LIBS        ON CACHE BOOL "Boost_USE_STATIC_LIBS")
    set(Boost_USE_MULTITHREADED      ON CACHE BOOL "Boost_USE_MULTITHREADED")
    set(Boost_USE_STATIC_RUNTIME     OFF CACHE BOOL "Boost_USE_STATIC_RUNTIME")
    set(Boost_ALL_DYN_LINK           OFF CACHE BOOL "BOOST_ALL_DYN_LINK") 

    set(MD_BOOST_COMPONENTS
        #filesystem
        #atomic
        #date_time
        #system
        #iostreams
        #regex
        program_options
        #thread
        #chrono
        #locale
    )
    
    find_package(Boost COMPONENTS REQUIRED ${MD_BOOST_COMPONENTS} )
    message(STATUS "** BOOST Include: ${Boost_INCLUDE_DIR}")
    message(STATUS "** BOOST Libraries Dirs: ${Boost_LIBRARY_DIRS}")
    message(STATUS "** BOOST Libraries: ${Boost_LIBRARIES}")

    include_directories(${Boost_INCLUDE_DIR})
    link_directories(${Boost_LIBRARY_DIRS})
    
endif()


set(test_sources
    main.cpp
    queue_test.cpp
)
add_executable(tools-md_tests ${test_sources})

target_link_libraries(tools-md_tests
    tz
    fmt
    ${Boost_LIBRARIES} 
    pthread
    event
    event_core
    event_extra
    event_pthreads
    gmock
#	gtest
)
