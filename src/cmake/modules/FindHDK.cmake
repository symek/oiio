# Based on Drew Whitehouse hdk-config:
# http://code.google.com/p/hdk-config/

IF (EXISTS "$ENV{HT}/" AND IS_DIRECTORY "$ENV{HT}/")
	SET(HDK_FOUND 1)
	set(HDK_CXX_COMPILER g++)
	set(HDK_INCLUDE_DIR $ENV{HT}/include)
	set(HDK_LIB_DIR $ENV{HFS}/../dsolib)

	# compiler flags:
	execute_process(COMMAND hcustom -c 
					OUTPUT_VARIABLE HDK_CXX_FLAGS
					OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_STRIP_TRAILING_WHITESPACE)

    # remove -c from a cxx:
    # string (REPLACE " -c" " " HDK_CXX_FLAGS ${HDK_CXX_FLAGS})
    
	set(HDK_CXX_FLAGS "-D_GNU_SOURCE -DLINUX -DAMD64 -m64 -Wno-error -fpermissive 
		-fPIC -DSIZEOF_VOID_P=8 -DFBX_ENABLED=1 -DOPENCL_ENABLED=1 -DMAKING_DSO 
		-DOPENVDB_ENABLED=1 -DSESI_LITTLE_ENDIAN -DENABLE_THREADS -DUSE_PTHREADS 
		-D_REENTRANT -D_FILE_OFFSET_BITS=64 -DGCC4 -DGCC3 -Wno-deprecated -std=c++11  
		-Wall -W -Wno-parentheses -Wno-sign-compare -Wno-reorder -Wno-uninitialized -Wunused 
		-Wno-unused-parameter -Wno-unused-local-typedefs -O2 -fno-strict-aliasing")

	# linker flags:
	execute_process(COMMAND hcustom -m 
					OUTPUT_VARIABLE HDK_LINK_FLAGS
					OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_STRIP_TRAILING_WHITESPACE)
	
	# Set variables:
	set(HDK_LIBRARIES ${HDK_LIB_DIR}/libHoudiniUT.so)
	message(STATUS "HDK found: $ENV{HT}")
	message(STATUS "HDK version: $ENV{HOUDINI_VERSION}")
	message(STATUS "HDK headers: ${HDK_INCLUDE_DIR}")
	message(STATUS "HDK CXX FLAGS: " ${HDK_CXX_FLAGS})

ELSE()

	SET(HDK_FOUND 0)
	message(STATUS "HDK not found. Rat plugin will not be built")

ENDIF()