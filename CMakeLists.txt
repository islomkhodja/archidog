cmake_minimum_required(VERSION 3.2.2)

project("archive-server" CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++17")
SET(CMAKE_CXX_STANDARD 17)
set(Boost_INCLUDE_DIR c:/boost/include/)
set(Boost_LIBRARY_DIR c:/boost/lib/)

SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(Boost_USE_MULTITHREAD ON)
# we will use static libs
set(Boost_USE_STATIC_LIBS ON)

FIND_PACKAGE(Boost 1.55.0 REQUIRED COMPONENTS log_setup log system thread program_options)
add_definitions(-DBOOST_LOG_DYN_LINK)

add_executable(client
        src/client/main.cpp
        src/common/rle.cpp
        src/common/rle.h
        src/client/client.cpp
        src/client/client.h
        src/client/globals.cpp
        src/client/globals.h)

target_include_directories(client
        PUBLIC
        ${Boost_INCLUDE_DIR}
        "src/client/"
)

target_link_libraries(client
        PUBLIC
        Boost::log_setup
        Boost::log
        Boost::system
        Boost::thread
        Boost::filesystem
        Boost::program_options
)

target_compile_options(client
        PUBLIC
        -DBOOST_LOG_DYN_LINK
)

add_executable(test
        test/main.cpp
        src/common/rle.cpp
        src/common/rle.h test/rle_test.cpp test/rle_test.h)

target_include_directories(test
        PUBLIC
        ${Boost_INCLUDE_DIR}
        "${CMAKE_CURRENT_SOURCE_DIR}/test"
)

target_link_libraries(test
        PUBLIC
        Boost::log_setup
        Boost::log
        Boost::system
        Boost::thread
        Boost::filesystem
        Boost::program_options
)

target_compile_options(test
        PUBLIC
        -DBOOST_LOG_DYN_LINK
)

add_executable(server src/server/main.cpp src/server/ArchiveServer.h src/server/ArchiveServer.cpp src/server/Session.cpp src/server/Session.h src/server/globals.cpp src/server/globals.h src/common/rle.cpp src/common/rle.h)

target_include_directories(server
        PUBLIC
        ${Boost_INCLUDE_DIR}
        "src/server"
)

target_link_libraries(server
        PUBLIC
        Boost::log_setup
        Boost::log
        Boost::system
        Boost::thread
        Boost::filesystem
        Boost::program_options
)

target_compile_options(server
        PUBLIC
        -DBOOST_LOG_DYN_LINK
)