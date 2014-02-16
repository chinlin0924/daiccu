cmake_minimum_required(VERSION 2.8)

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
set(SOURCES
    canusb/canusb.c
    processor/ccu.c
)

# Headers
set(HEADERS
    can.h
    canusb/canusb.h
    canusb/serialport.h
    processor/ccu.h
)

if(WIN32)
    set(SOURCES ${SOURCES}
        canusb/serialport_win.c
    )

    # Build libraries
    add_library(daiccu ${SOURCES})

    # Build binaries
    add_executable(daiccud daiccu_win.c)
    target_link_libraries(daiccud daiccu)
endif(WIN32)

if(UNIX)
    find_package(X11)
    if(X11_FOUND)
        set(LIBS ${LIBS} ${X11_LIBRARIES})
        include_directories(${X11_INCLUDE_DIR})
    else()
        message("Failed to find X11")
    endif(X11_FOUND)

    find_library(X11_XTest_LIB Xtst ${X11_LIB_SEARCH_PATH})
    if(X11_XTest_FOUND)
        set(LIBS ${LIBS} ${X11_XTest_LIB})
        include_directories(${X11_XTest_INCLUDE_DIR})
    else()
        message("Failed to find XTest library")
    endif(X11_XTest_FOUND)

    if(X11_FOUND AND X11_XTest_FOUND)
        add_definitions(-DUSE_X11)
    endif(X11_FOUND AND X11_XTest_FOUND)

    set(SOURCES ${SOURCES}
        canusb/serialport_linux.c
        socketcan/socketcan.c
    )

    set(HEADERS ${HEADERS}
        socketcan/socketcan.h
    )

    # Build libraries
    #add_library(daiccu SHARED ${SOURCES})
    add_library(daiccu STATIC ${SOURCES})
    target_link_libraries(daiccu pthread)

    # Build binaries
    add_executable(daiccud daiccu_linux.c)
    target_link_libraries(daiccud daiccu pthread ${LIBS})

    add_executable(daiccusd daiccu_linux.c)
    set_target_properties (daiccusd PROPERTIES COMPILE_DEFINITIONS "USE_SOCKETCAN")
    target_link_libraries(daiccusd daiccu pthread ${LIBS})
    INSTALL(TARGETS daiccusd RUNTIME DESTINATION bin)

endif(UNIX)

# Install
INSTALL(FILES ${HEADERS} DESTINATION include/daiccu)
INSTALL(TARGETS daiccud daiccu
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
