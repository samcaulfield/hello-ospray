cmake_minimum_required(VERSION 3.0)
project(hello-ospray)

find_package(ospray 1.8.0 REQUIRED)

add_executable(hello-ospray main.cpp stb_image_write.h)
target_link_libraries(hello-ospray ospray::ospray)

