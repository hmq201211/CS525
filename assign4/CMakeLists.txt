cmake_minimum_required(VERSION 3.13)
project(Assignment4 C)

set(CMAKE_C_STANDARD 99)

add_executable(Assignment4 buffer_mgr.c buffer_mgr_stat.c dberror.c expr.c record_mgr.c rm_serializer.c storage_mgr.c test_assign4_1.c btree_mgr.c)