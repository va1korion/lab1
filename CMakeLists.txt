cmake_minimum_required(VERSION 3.14)
project(dlib_file_search C)

set(CMAKE_C_STANDARD 11)

add_executable(dlib_file_search main.c libs/ipv4-addr.c)