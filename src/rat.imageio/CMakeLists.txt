find_package (HDK)

if (HDK_FOUND)
	SET(CMAKE_CXX_FLAGS ${HDK_CXX_FLAGS})
	link_directories (${HDK_LIB_DIR})
    include_directories (${HDK_INCLUDE_DIR})
    add_oiio_plugin (ratinput.cpp LINK_LIBRARIES ${HDK_LIBRARIES})
endif ()
