cmake_minimum_required(VERSION 3.10)

project(Final_Practice C)

include_directories(.)

add_executable(server server.c)

target_link_libraries(server PUBLIC pthread)