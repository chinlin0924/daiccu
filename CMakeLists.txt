cmake_minimum_required(VERSION 2.8)

project(daiccu)

# Show all warnings
add_definitions(-Wall -Werror)

# Debug
add_definitions(-DDAICCUD_DEBUG)

if(DEBUG)
    add_definitions(-D_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
endif(DEBUG)

# Includes
include_directories(${daiccu_SOURCE_DIR})

# Sources
set(SOURCES
    canusb/canusb.c
    processor/ccu.c
    processor/touchpad.c
    processor/networkmanager.c
)

# Headers
set(HEADERS
    can.h
    canusb/canusb.h
    canusb/serialport.h
    processor/ccu.h
    processor/touchpad.h
    processor/networkmanager.h
)

if(WIN32)
    set(SOURCES ${SOURCES}
        canusb/serialport_win.c
    )
endif(WIN32)

if(UNIX)
    find_package(X11)
    find_library(X11_XTest_LIB Xtst ${X11_LIB_SEARCH_PATH})
    if(X11_FOUND AND X11_XTest_FOUND)
        set(LIBS ${LIBS} ${X11_LIBRARIES})
        include_directories(${X11_INCLUDE_DIR})

        set(LIBS ${LIBS} ${X11_XTest_LIB})
        include_directories(${X11_XTest_INCLUDE_DIR})

        add_definitions(-DUSE_X11)
    else()
        message(STATUS "X11 libraries not found. X11 key event injection will be disabled.")
    endif(X11_FOUND AND X11_XTest_FOUND)

    set(LIBS ${LIBS} pthread)

    set(SOURCES ${SOURCES}
        canusb/serialport_linux.c
        socketcan/socketcan.c
    )

    set(HEADERS ${HEADERS}
        socketcan/socketcan.h
    )
endif(UNIX)

# Build libraries
#add_library(daiccu SHARED ${SOURCES})
add_library(daiccu STATIC ${SOURCES})
set_target_properties(daiccu PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# Build binaries
add_executable(daiccud daiccu.c)
target_link_libraries(daiccud daiccu ${LIBS})

if(UNIX)
    add_executable(daiccusd daiccu.c)
    set_target_properties (daiccusd PROPERTIES COMPILE_DEFINITIONS "USE_SOCKETCAN")
    target_link_libraries(daiccusd daiccu ${LIBS})
    INSTALL(TARGETS daiccusd RUNTIME DESTINATION bin)
endif(UNIX)

# Install
INSTALL(FILES ${HEADERS} DESTINATION include/daiccu)
INSTALL(TARGETS daiccud daiccu
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

