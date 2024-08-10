cmake_minimum_required(VERSION 3.21)
project(controllerorchestra CXX)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(midifile)

set(COMMON_SRCS
        main.cpp
        constants.h
        controllers/Controller.h
        controllers/SteamController.cpp
        controllers/SteamController.h
        libs/joycon.hpp
        controllers/SwitchController.cpp
        controllers/SwitchController.h
        controllers/PS5Controller.cpp
        controllers/PS5Controller.h
)

add_executable(controllerorchestra ${COMMON_SRCS})

find_path(LIBUSB_INCLUDE_DIR
        NAMES libusb.h
        PATH_SUFFIXES "include" "libusb" "libusb-1.0"
)
find_path(HIDAPI_INCLUDE_DIR
        NAMES hidapi.h
        PATH_SUFFIXES "include" "hidapi" "hidapi-hidraw"
)
#find_library(LIBUSB_LIBRARY
#        NAMES usb-1.0
#        PATH_SUFFIXES "lib" "lib32" "lib64"
#)

include_directories(controllerorchestra ${HIDAPI_INCLUDE_DIR} ${LIBUSB_INCLUDE_DIR})
target_link_libraries(controllerorchestra hidapi-hidraw usb-1.0 MIDIFile)