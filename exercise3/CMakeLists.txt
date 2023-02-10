cmake_minimum_required(VERSION 3.10)

project(DS_Exercise3 C)

include_directories(.)

add_library(keys SHARED libkeys.c)
#add_library(keys_xdr keys_xdr.c)


add_executable(server keys_server.c keys_xdr.c keys_svc.c)
add_executable(client client.c keys_xdr.c keys_clnt.c)


target_link_libraries(server PUBLIC pthread)
#target_link_libraries(keys PUBLIC)
target_link_libraries(client PUBLIC keys pthread)