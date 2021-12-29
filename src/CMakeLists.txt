cmake_minimum_required(VERSION 3.2)
project(LCL)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11 -g -o3")

add_compile_options(-g -o3)
set(CMAKE_CXX_STANDARD 11)


link_libraries(pthread rt)



add_executable(LCL  main.cpp  parameters.h    system.h system.cpp easylogging++.h easylogging++.cc  NetWork.h NetWork.cpp  Commander.h Commander.cpp )
MESSAGE(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
MESSAGE(STATUS "CMAKE_CXX_FLAGS: $<COMPILE_OPTIONS>")



