cmake_minimum_required(VERSION 3.12)
project(siki)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads)
find_package(Boost COMPONENTS program_options system filesystem log REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
set(Boost_USE_STATIC_LIBS ON)

#add_executable(test src/test.cc src/commands.h src/console_parser.h src/socket_wrapper.h)
#target_link_libraries(test ${Boost_LIBRARIES})

add_executable(netstore-client src/client.cpp)
target_link_libraries(netstore-client ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(netstore-server src/server.cpp)
target_link_libraries(netstore-server ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})