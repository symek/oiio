find_package (HDK)

if (HDK_FOUND)
	add_definitions (${HDK_DEFINITIONS})
    include_directories (${HDK_INCLUDE_DIRS})
    add_oiio_plugin (ratinput.cpp LINK_LIBRARIES ${HDK_LIBRARIES})
endif ()
