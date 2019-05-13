configure_file (
    "${PROJECT_SOURCE_DIR}/../md_config.h.in"
    "${PROJECT_BINARY_DIR}/md_config.h"
)

include_directories("${PROJECT_BINARY_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../deps/date/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../deps/fmt/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../deps/json/single_include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../deps/googletest/googletest/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../deps/googletest/googlemock/include"

    ${PostgreSQL_INCLUDE_DIRS}
)

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
