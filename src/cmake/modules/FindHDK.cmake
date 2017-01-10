# Based on Drew Whitehouse hdk-config:
# http://code.google.com/p/hdk-config/

IF (EXISTS "$ENV{HT}/" AND IS_DIRECTORY "$ENV{HT}/")
	SET(HDK_FOUND 1)
	set(HDK_CXX_COMPILER g++)
	set(HDK_INCLUDE_DIR $ENV{HT}/include/)
	set(HDK_LIB_DIR $ENV{HDSO}/)

	# compiler flags:
	execute_process(COMMAND hcustom -c 
					OUTPUT_VARIABLE HDK_CXX_FLAGS
					OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_STRIP_TRAILING_WHITESPACE)

    # remove -c from a cxx:
    string (REPLACE "-c" " " HDK_CXX_FLAGS ${HDK_CXX_FLAGS})

	# linker flags:
	execute_process(COMMAND hcustom -m 
					OUTPUT_VARIABLE HDK_LINK_FLAGS
					OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_STRIP_TRAILING_WHITESPACE)
	
	# Set variables:
	set(HDK_LIBRARIES HoudiniUT)

	message(STATUS "HDK found: $ENV{HT}")
	message(STATUS "HDK version: $ENV{HOUDINI_VERSION}")

ELSE()

	SET(HDK_FOUND 0)
	message(STATUS "HDK not found. Rat plugin will not be built")

ENDIF()