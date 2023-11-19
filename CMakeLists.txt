cmake_minimum_required(VERSION 3.22)
project(graphics)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(ImageMagick COMPONENTS Magick++ MagickCore)

add_executable(${CMAKE_PROJECT_NAME} main.cpp)

include_directories(${ImageMagick_INCLUDE_DIRS})

target_link_libraries(${CMAKE_PROJECT_NAME}  PRIVATE ${ImageMagick_LIBRARIES})
