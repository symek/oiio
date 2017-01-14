find_package (HDK)

if (HDK_FOUND)
    	add_oiio_plugin (ratinput.cpp 
    	INCLUDE_DIRS ${HDK_INCLUDE_DIR} 
    	LINK_LIBRARIES ${HDK_LIBRARIES}
    	DEFINITIONS ${HDK_CXX_FLAGS} "-DUSE_HDK")
endif ()
