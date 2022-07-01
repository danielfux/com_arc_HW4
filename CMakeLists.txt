cmake_minimum_required(VERSION 3.22)
project(wet_4_comp)

set(CMAKE_CXX_STANDARD 14)

add_executable(wet_4_comp main.c core_api.cpp core_api.h sim_api.c sim_api.h)
