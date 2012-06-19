cmake_minimum_required (VERSION 2.8)
project (FMIComplianceChecker)

set(FMUCHK_HOME ${FMIComplianceChecker_SOURCE_DIR})
set(FMUCHK_BUILD ${FMIComplianceChecker_BINARY_DIR})

# User configuration options and parameters
SET(FMUCHK_INSTALL_PREFIX ${FMUCHK_BUILD}/../install CACHE PATH "Prefix prepended to install directories")
set(FMUCHK_FMIL_HOME_DIR  ${FMUCHK_HOME}/ThirdParty/FMIL CACHE PATH "Path to the FMIL sources (https://svn.jmodelica.org/FMILibrary/trunk)" )
set(FMUCHK_FMI_STANDARD_HEADERS  ${FMUCHK_FMIL_HOME_DIR}/ThirdParty/FMI/standard32 CACHE PATH "Path to the FMI standard headers dir" )
option(FMUCHK_ENABLE_LOG_LEVEL_DEBUG "Enable log level 'debug'. If the option is of then the debug level is not compiled in." OFF)
set(FMUCHK_TEST_FMUS_DIR ${FMUCHK_HOME}/TestFMUs CACHE PATH "Directory with FMUs to be used in tests (checker will run for each FMU)")
if(MSVC)
	option (FMUCHK_BUILD_WITH_STATIC_RTLIB "Use static run-time libraries (/MT or /MTd linker flags)" ON)
endif()

set(FMUCHK_FMI_PLATFORM_DOC "FMI platform defines the subdirectory within FMU where binary is located")
set(FMUCHK_FMI_PLATFORM ”unknown” CACHE STRING ${FMUCHK_FMI_PLATFORM_DOC})
if(FMUCHK_FMI_PLATFORM MATCHES ”unknown”)
	if(CMAKE_HOST_WIN32) #Set to true when the host system is Windows and on cygwin.
		set(FMI_PLATFORM win)
	elseif(CMAKE_HOST_APPLE) #Set to true when the host system is Apple OSX. 
		set(FMI_PLATFORM darwin)
	elseif(CMAKE_HOST_UNIX) #Set to true when the host system is UNIX or UNIX like (i.e. APPLE and CYGWIN). 
		set(FMI_PLATFORM linux)
	else(CMAKE_HOST_WIN32)
		message(SEND_ERROR "Target platform is not supported by the tests")
		set(FMI_PLATFORM not_supported)
	endif()
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(FMI_PLATFORM "${FMI_PLATFORM}32")
	else()
		set(FMI_PLATFORM "${FMI_PLATFORM}64")
	endif()
	
	set(FMUCHK_FMI_PLATFORM ${FMI_PLATFORM} CACHE STRING ${FMUCHK_FMI_PLATFORM_DOC} FORCE)
else()
	set(FMI_PLATFORM ${FMUCHK_FMI_PLATFORM})
endif()

set(fmuCheck fmuCheck.${FMUCHK_FMI_PLATFORM})

if(MSVC)
	# With C89 we're forced to use non-secure functions
	ADD_DEFINITIONS (/D _CRT_SECURE_NO_WARNINGS)
	
	# Set the run-time library flag as controlled by the FMUCHK_BUILD_WITH_STATIC_RTLIB option
	if(FMUCHK_BUILD_WITH_STATIC_RTLIB)
		set(BUILD_WITH_STATIC_CRT YES)
		set(replace_from "/MD")
		set(replace_to "/MT")
	else()
		set(BUILD_WITH_STATIC_CRT NO)
		set(replace_from "/MT")
		set(replace_to "/MD")
	endif(FMUCHK_BUILD_WITH_STATIC_RTLIB)

	foreach(flag_var
		CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
		CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
		if(${${flag_var}} MATCHES "${replace_from}")
			string(REPLACE "${replace_from}" "${replace_to}" tmp "${${flag_var}}")
			set(${flag_var} ${tmp} CACHE STRING "compiler flags" FORCE)
		endif()
	endforeach(flag_var)	
endif()

# Add FMIL as imported library
include(ExternalProject)

set(FMIL_SETTINGS
	-DFMILibrary_SOURCE_DIR:PATH="${FMUCHK_FMIL_HOME_DIR}"
	-DFMILibrary_BINARY_DIR:PATH="${FMUCHK_BUILD}/FMIL/build"
	-DFMILIB_INSTALL_PREFIX:PATH=${FMUCHK_BUILD}/FMIL/install
	-DFMILIB_FMI_STANDARD_HEADERS:PATH=${FMUCHK_FMI_STANDARD_HEADERS}
	-DFMILIB_FMI_PLATFORM:STRING=${FMUCHK_FMI_PLATFORM}
	-DFMILIB_DEFAULT_BUILD_TYPE_RELEASE:BOOLEAN=ON
	-DFMILIB_BUILD_STATIC_LIB:BOOLEAN=ON
	-DFMILIB_BUILD_SHARED_LIB:BOOLEAN=OFF
	-DFMILIB_INSTALL_SUBLIBS:BOOLEAN=OFF
	-DFMILIB_GENERATE_DOXYGEN_DOC:BOOLEAN=OFF
	-DFMILIB_BUILD_TESTS:BOOLEAN=ON
	-DFMILIB_GENERATE_BUILD_STAMP:BOOLEAN=ON
	-DFMILIB_ENABLE_LOG_LEVEL_DEBUG:BOOLEAN=${FMUCHK_ENABLE_LOG_LEVEL_DEBUG}
	-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
	-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
	-DCMAKE_C_FLAGS_DEBUG:STRING=${CMAKE_C_FLAGS_DEBUG}
	-DCMAKE_C_FLAGS_RELEASE:STRING=${CMAKE_C_FLAGS_RELEASE}
	-DCMAKE_C_FLAGS_MINSIZEREL:STRING=${CMAKE_C_FLAGS_MINSIZEREL}
	-DCMAKE_EXE_LINKER_FLAGS:STRING=${CMAKE_EXE_LINKER_FLAGS}
	-DCMAKE_LINK_LIBRARY_FLAG:STRING=${CMAKE_LINK_LIBRARY_FLAG}
	-DCMAKE_MODULE_LINKER_FLAGS:STRING=${CMAKE_MODULE_LINKER_FLAGS}
	-DCMAKE_SHARED_LINKER_FLAGS:STRING=${CMAKE_SHARED_LINKER_FLAGS}	
	-DCMAKE_C_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_C_FLAGS_RELWITHDEBINFO}
)

if(MSVC)
	set(FMIL_SETTINGS ${FMIL_SETTINGS}
		-DFMILIB_BUILD_WITH_STATIC_RTLIB:BOOLEAN=${FMUCHK_BUILD_WITH_STATIC_RTLIB}
		)
endif()

ExternalProject_Add(
	fmil
	PREFIX "${FMUCHK_BUILD}/FMIL"
	SOURCE_DIR "${FMUCHK_FMIL_HOME_DIR}"
	CMAKE_CACHE_ARGS ${FMIL_SETTINGS}
	BINARY_DIR ${FMUCHK_BUILD}/FMIL/build
	INSTALL_DIR ${FMUCHK_BUILD}/FMIL/install
)

ExternalProject_Add_Step(
	fmil fmil_reconfigure
	DEPENDEES configure
	DEPENDERS build
	COMMAND ${CMAKE_COMMAND} -E echo "Running:  ${CMAKE_COMMAND} ${FMIL_SETTINGS}  -G "${CMAKE_GENERATOR}" ${FMUCHK_FMIL_HOME_DIR}"
	COMMAND ${CMAKE_COMMAND} ${FMIL_SETTINGS} -G "${CMAKE_GENERATOR}" "${FMUCHK_FMIL_HOME_DIR}"
	DEPENDS ${FMUCHK_BUILD}/CMakeCache.txt ${FMUCHK_HOME}/CMakeLists.txt
	WORKING_DIRECTORY ${FMUCHK_BUILD}/FMIL/build
)

add_dependencies(fmil ${CMAKE_BINARY_DIR}/CMakeCache.txt  ${FMUCHK_HOME}/CMakeLists.txt)
  
set(fmilib_name "${FMUCHK_BUILD}/FMIL/build/${CMAKE_CFG_INTDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}fmilib${CMAKE_STATIC_LIBRARY_SUFFIX}")
  
add_library(fmilib STATIC IMPORTED)

set_target_properties(
	fmilib PROPERTIES 
		IMPORTED_LOCATION "${fmilib_name}"
)

add_dependencies(fmilib fmil)

# Add a command that fakes generation of fmilib.h just to get dependencies right
add_custom_command(OUTPUT "${FMUCHK_BUILD}/FMIL/install/include/fmilib.h"
                     COMMAND ${CMAKE_COMMAND} -E touch_nocreate "${FMUCHK_BUILD}/FMIL/install/include/fmilib.h"
                     DEPENDS fmil
)

# Now create the checker executable
set(SOURCE 
	${FMUCHK_HOME}/src/Common/fmuChecker.c
	${FMUCHK_HOME}/src/FMI1/fmi1_check.c
	${FMUCHK_HOME}/src/FMI1/fmi1_me_sim.c
	${FMUCHK_HOME}/src/FMI1/fmi1_cs_sim.c
	)
set(HEADERS 
	${FMUCHK_BUILD}/FMIL/install/include/fmilib.h
	${FMUCHK_HOME}/include/fmuChecker.h)

include_directories(
	${FMUCHK_BUILD}/FMIL/install/include/
	include/Common/
	include)

add_executable(${fmuCheck} ${SOURCE} ${HEADERS})
target_link_libraries(${fmuCheck} fmilib)

if(UNIX) 
	message(STATUS "Adding -ldl to the linker")
	target_link_libraries(${fmuCheck} dl)
endif(UNIX)

install(TARGETS ${fmuCheck} DESTINATION ${FMUCHK_INSTALL_PREFIX})

# tests
file(TO_CMAKE_PATH ${FMUCHK_TEST_FMUS_DIR} FMUCHK_TEST_FMUS_DIR_NORMALIZED)
file(GLOB_RECURSE TEST_FMUS "${FMUCHK_TEST_FMUS_DIR_NORMALIZED}/*.fmu" )
	   
set(ALL_TEST_FMUS 
	${FMUCHK_BUILD}/FMIL/build/testfolder/BouncingBall_cs.fmu
	${FMUCHK_BUILD}/FMIL/build/testfolder/BouncingBall_me.fmu
	${TEST_FMUS})

ENABLE_TESTING()

file(MAKE_DIRECTORY ${FMUCHK_BUILD}/tmpfolder) 

add_test(NAME Test_FMIL
		WORKING_DIRECTORY ${FMUCHK_BUILD}/FMIL/build
		COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
		)

add_test(
	NAME Build_before_test
	COMMAND "${CMAKE_COMMAND}" --build ${FMUCHK_BUILD} --config $<CONFIGURATION>)

set(TEST_COMMAND_LINE_PARSING OFF)
if(TEST_COMMAND_LINE_PARSING)
	add_test(check_command_line_parsing_0 ${fmuCheck} )
	add_test(check_command_line_parsing_1 ${fmuCheck} -x -h a.fmu )
	set_tests_properties(check_command_line_parsing_1 PROPERTIES WILL_FAIL TRUE)
	add_test(check_command_line_parsing_2 ${fmuCheck} -xx -h a.fmu )
	set_tests_properties(check_command_line_parsing_2 PROPERTIES WILL_FAIL TRUE)
	add_test(check_command_line_parsing_3 ${fmuCheck} -l 5 -e a.fmu )
	set_tests_properties(check_command_line_parsing_3 PROPERTIES WILL_FAIL TRUE)
	add_test(check_command_line_parsing_4 ${fmuCheck} -l 5  )
	set_tests_properties(check_command_line_parsing_4 PROPERTIES WILL_FAIL TRUE)
	add_test(check_command_line_parsing_5 ${fmuCheck} -l 6 a.fmu -x  )
	set_tests_properties(check_command_line_parsing_5 PROPERTIES WILL_FAIL TRUE)
	add_test(check_command_line_parsing_6 ${fmuCheck} -l 6 -n -1 a.fmu  )
	set_tests_properties(check_command_line_parsing_6 PROPERTIES WILL_FAIL TRUE)
	add_test(check_command_line_parsing_7 ${fmuCheck} -l 6 -n 10 -e 1 -s -1 a.fmu  )
	set_tests_properties(check_command_line_parsing_7 PROPERTIES WILL_FAIL TRUE)
endif()

foreach(fmu ${ALL_TEST_FMUS})
	string(REPLACE "/" "_" testname "check_${fmu}")
	string(REPLACE ":" "_" testname ${testname})
	add_test(${testname} ${fmuCheck} -t ${FMUCHK_BUILD}/tmpfolder -l 6 -o ${FMUCHK_BUILD}/tmpfolder/${testname}.csv ${fmu} )
endforeach(fmu)