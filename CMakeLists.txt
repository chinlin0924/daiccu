cmake_minimum_required(VERSION 2.8.10.1 FATAL_ERROR)

project(daiccu)

#show all warnings
add_definitions(-Wall -Werror)

if(DEBUG)
    add_definitions(-D_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
endif(DEBUG)

# Includes
include_directories(${daiccu_SOURCE_DIR})

# Sources
set(SOURCES_CPP
    can.h
    canusb/canusb.c
    canusb/canusb.h
    canusb/serialport.h
    processor/ccu.c
    processor/ccu.h
)

if(WIN32)
    set(SOURCES_CPP ${SOURCES_CPP}
        canusb/serialport_win.c
    )

    # Build libraries
    add_library(ccu ${SOURCES_CPP})

    # Build binaries
    add_executable(daiccu daiccu_win.c)
    target_link_libraries(daiccu ccu)
endif(WIN32)

if(UNIX)
    find_package(X11)
    if(X11_FOUND)
        set(LIBS ${LIBS} ${X11_LIBRARIES})
        include_directories(${X11_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "Failed to find X11")
    endif(X11_FOUND)

    find_library(X11_XTest_LIB Xtst ${X11_LIB_SEARCH_PATH})
    if(X11_XTest_FOUND)
        set(LIBS ${LIBS} ${X11_XTest_LIB})
        include_directories(${X11_XTest_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "Failed to find XTest library")
    endif(X11_XTest_FOUND)

    if(X11_FOUND AND X11_XTest_FOUND)
        add_definitions(-DUSE_X11)
    endif(X11_FOUND AND X11_XTest_FOUND)

    set(SOURCES_CPP ${SOURCES_CPP}
        canusb/serialport_linux.c
    )

    # Build libraries
    add_library(ccu ${SOURCES_CPP})
    target_link_libraries(ccu pthread)

    # Build binaries
    add_executable(daiccu daiccu_linux.c)
    target_link_libraries(daiccu ccu pthread ${LIBS})
endif(UNIX)
