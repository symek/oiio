###########################################################################
# Figure out what platform we're on, and set some variables appropriately

if (VERBOSE)
    message (STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
    message (STATUS "CMAKE_SYSTEM_VERSION = ${CMAKE_SYSTEM_VERSION}")
    message (STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
endif ()

if (UNIX)
    if (VERBOSE)
        message (STATUS "Unix! ${CMAKE_SYSTEM_NAME}")
    endif ()
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        set (platform "linux")
        set (CXXFLAGS "${CXXFLAGS} -DLINUX")
        if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
            set (platform "linux64")
            set (CXXFLAGS "${CXXFLAGS} -DLINUX64")
        endif ()
    elseif (APPLE)
        set (platform "macosx")
    elseif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
        set (platform "FreeBSD")
        set (CXXFLAGS "${CXXFLAGS} -DFREEBSD")
        if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i386")
            # to use gcc atomics we need cpu instructions only available
            # with arch of i586 or higher
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=i586")
        endif()
    else ()
        string (TOLOWER ${CMAKE_SYSTEM_NAME} platform)
    endif ()
endif ()

if (WIN32)
    set (platform "windows")
endif ()

if (platform)
    message (STATUS "platform = ${platform}")
else ()
    message (FATAL_ERROR "'platform' not defined")
endif ()
