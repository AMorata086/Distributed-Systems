cmake_minimum_required(VERSION 3.10)

project(DS_Exercise1 C)

include_directories(.)

add_library(keys SHARED keys.c)

add_executable(server server.c)
add_executable(client client.c)

target_link_libraries(server PUBLIC rt pthread)
target_link_libraries(keys PUBLIC rt)
target_link_libraries(client PUBLIC keys pthread)