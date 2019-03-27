cmake_minimum_required(VERSION 3.6)
project(lg_ultrafine_brightness)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 14)

set(libusb_1_FIND_REQUIRED TRUE)
include(${PROJECT_SOURCE_DIR}/cmake/Findlibusb-1.0.cmake)
include_directories(${LIBUSB_1_INCLUDE_DIRS})

set(SOURCE_FILES main.cpp)
add_executable(lg_ultrafine_brightness ${SOURCE_FILES})
target_link_libraries(lg_ultrafine_brightness ${LIBUSB_1_LIBRARIES} curses)
