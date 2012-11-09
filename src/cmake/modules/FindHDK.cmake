
#
#    Locate the HDK environment
#
# This module defines 
# HDK_FOUND,
# HDK_CXX_COMPILER
# HDK_INCLUDE_DIRS,
# HDK_DEFINITIONS,
# HDK_LIBRARY_DIRS,
# HDK_LIBRARIES,
# HDK_DSO_INSTALL_DIR
#
# For OSX, we have the following as well ...
#
# HDK_FRAMEWORK_DIRS,
# HDK_FRAMEWORKS,
#

IF (EXISTS "${HT}/" AND IS_DIRECTORY "${HT}/")
	SET(HDK_FOUND 1)
	set(HDK_CXX_COMPILER g++)
	set(HDK_INCLUDE_DIR ${HT}/include)
	set(HDK_LIB_DIR -L${HDSO})

	# compiler flags:
	execute_process(COMMAND hcustom -c OUTPUT_VARIABLE HDK_DEFINITIONS)

	# Set variables:
	set(HDK_LIBRARY_DIRS /usr/X11R6/lib64 /usr/X11R6/lib)
	set(HDK_FRAMEWORK_DIRS )
	set(HDK_FRAMEWORKS )
	set(HDK_LIBRARIES GLU GL X11 Xext Xi dl)
	set(HDK_HIH_DIR /home/symek/houdini12.0)

	# OSX:
	set(HDK_STANDALONE_FRAMEWORK_DIRS )
	set(HDK_STANDALONE_FRAMEWORKS )
	set(HDK_STANDALONE_LIBRARIES pthread HoudiniUI HoudiniOPZ HoudiniOP3 HoudiniOP2 HoudiniOP1 HoudiniSIM HoudiniGEO HoudiniPRM HoudiniUT GLU GL X11 Xext Xi dl)

	message(STATUS "HDK found: ${HT}")
	message(STATUS "HDK version: ${HOUDINI_VERSION}")

ELSE()

	SET(HDK_FOUND 0)
	message(STATUS "HDK not found. Rat plugin will not be built")

ENDIF()
