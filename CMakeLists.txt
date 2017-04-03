#
#    Copyright (C) 2012 Modelon AB <http://www.modelon.com>
#
#	You should have received a copy of the LICENCE-FMUChecker.txt
#   along with this program. If not, contact Modelon AB.
#

#   File: CMakeLists.txt
#   Configuration and build script for the FMUChecker application.

cmake_minimum_required (VERSION 2.8.6 FATAL_ERROR)
# Prohibit in-source build
IF("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source build is not supported. Please, use an empty directory for building the project.")
ENDIF()
project (FMIComplianceChecker)

set(FMUCHK_HOME ${FMIComplianceChecker_SOURCE_DIR})
set(FMUCHK_BUILD ${FMIComplianceChecker_BINARY_DIR})

# User configuration options and parameters
set(FMUCHK_INSTALL_PREFIX ${FMUCHK_BUILD}/../install CACHE PATH "Prefix prepended to install directories.")
set(FMUCHK_FMIL_HOME_DIR "${FMUCHK_HOME}/FMIL")
set(FMUCHK_FMI_STANDARD_HEADERS  "" CACHE PATH "Path to the FMI standard headers directory. Leave empty to use the headers from FMIL." )
option(FMUCHK_ENABLE_LOG_LEVEL_DEBUG "Enable log level 'debug'. If the option is of then the debug level is not compiled in." OFF)
set(FMUCHK_TEST_FMUS_DIR ${FMUCHK_HOME}/TestFMUs CACHE PATH "Directory with FMUs to be used in tests (checker will run for each FMU).")
if(MSVC)
	option (FMUCHK_BUILD_WITH_STATIC_RTLIB "Use static run-time libraries (/MT or /MTd linker flags)" ON)
endif()

set(FMUCHK_FMI_PLATFORM_DOC "FMI platform defines the subdirectory within FMU where binary is located")
set(FMUCHK_FMI_PLATFORM ”unknown” CACHE STRING ${FMUCHK_FMI_PLATFORM_DOC})

IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE "Release")
	message(STATUS "Setting build type to ${CMAKE_BUILD_TYPE}")
ENDIF()

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

set(FMI_STANDARD_HEADERS_DIR ${FMUCHK_FMI_STANDARD_HEADERS})
string(REPLACE " " "" tmp "${FMI_STANDARD_HEADERS_DIR}")
string(REPLACE "\t" "" tmp "${tmp}")
if("${tmp}" STREQUAL "")
	set(FMI_STANDARD_HEADERS_DIR ${FMUCHK_FMIL_HOME_DIR}/ThirdParty/FMI/default)
endif()
message(STATUS "Will use standard headers from:\n ${FMI_STANDARD_HEADERS_DIR}")
# Add FMIL as imported library
include(ExternalProject)

set(FMIL_SETTINGS
	-DFMILibrary_SOURCE_DIR:PATH=${FMUCHK_FMIL_HOME_DIR}
	-DFMILibrary_BINARY_DIR:PATH=${FMUCHK_BUILD}/FMIL/build
	-DFMILIB_INSTALL_PREFIX:PATH=${FMUCHK_BUILD}/FMIL/install
	-DFMILIB_FMI_STANDARD_HEADERS:PATH=${FMI_STANDARD_HEADERS_DIR}
	-DFMILIB_FMI_PLATFORM:STRING=${FMUCHK_FMI_PLATFORM}
	-DFMILIB_DEFAULT_BUILD_TYPE_RELEASE:BOOLEAN=ON
	-DFMILIB_BUILD_STATIC_LIB:BOOLEAN=ON
	-DFMILIB_BUILD_SHARED_LIB:BOOLEAN=OFF
	-DFMILIB_INSTALL_SUBLIBS:BOOLEAN=OFF
	-DFMILIB_GENERATE_DOXYGEN_DOC:BOOLEAN=OFF
	-DFMILIB_BUILD_TESTS:BOOLEAN=ON
	-DFMILIB_GENERATE_BUILD_STAMP:BOOLEAN=OFF
	-DFMILIB_ENABLE_LOG_LEVEL_DEBUG:BOOLEAN=${FMUCHK_ENABLE_LOG_LEVEL_DEBUG}
	-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
	-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
	-DCMAKE_C_FLAGS_DEBUG:STRING=${CMAKE_C_FLAGS_DEBUG}
	-DCMAKE_C_FLAGS_RELEASE:STRING=${CMAKE_C_FLAGS_RELEASE}
	-DCMAKE_C_FLAGS_MINSIZEREL:STRING=${CMAKE_C_FLAGS_MINSIZEREL}
	-DCMAKE_C_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_C_FLAGS_RELWITHDEBINFO}
	-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
	-DCMAKE_CXX_FLAGS_DEBUG:STRING=${CMAKE_CXX_FLAGS_DEBUG}
	-DCMAKE_CXX_FLAGS_RELEASE:STRING=${CMAKE_CXX_FLAGS_RELEASE}
	-DCMAKE_CXX_FLAGS_MINSIZEREL:STRING=${CMAKE_CXX_FLAGS_MINSIZEREL}
	-DCMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_CXX_FLAGS_RELWITHDEBINFO}
	-DCMAKE_EXE_LINKER_FLAGS:STRING=${CMAKE_EXE_LINKER_FLAGS}
	-DCMAKE_LINK_LIBRARY_FLAG:STRING=${CMAKE_LINK_LIBRARY_FLAG}
	-DCMAKE_MODULE_LINKER_FLAGS:STRING=${CMAKE_MODULE_LINKER_FLAGS}
	-DCMAKE_SHARED_LINKER_FLAGS:STRING=${CMAKE_SHARED_LINKER_FLAGS}	
)

if(MSVC)
	set(FMIL_SETTINGS ${FMIL_SETTINGS}
		-DFMILIB_BUILD_WITH_STATIC_RTLIB:BOOLEAN=${FMUCHK_BUILD_WITH_STATIC_RTLIB}
		)
endif()

ExternalProject_Add(
	fmil
	SVN_REPOSITORY "https://svn.jmodelica.org/FMILibrary/tags/2.0.2"
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

# Detect the version
include(${FMUCHK_HOME}/GetVersion.cmake)
getVersionSVN("${FMUCHK_HOME}" FMUCHK_VERSION)
message(STATUS "Checker version: ${FMUCHK_VERSION}")
getVersionSVN("${FMUCHK_HOME}/FMIL" FMIL_VERSION)
message(STATUS "FMIL version: ${FMIL_VERSION}")
file(WRITE ${CMAKE_BINARY_DIR}/VERSION-FMUChecker.txt 
"FMI checker version:${FMUCHK_VERSION}
FMI library version:${FMIL_VERSION}
")
file(WRITE ${CMAKE_BINARY_DIR}/fmu_checker_version.h 
"
/* Version information generated based on SVN info */
#define FMUCHK_VERSION \"${FMUCHK_VERSION}\"
#define FMIL_VERSION \"${FMIL_VERSION}\"
")

# Now create the checker executable
set(SOURCE 
	${FMUCHK_HOME}/src/Common/fmuChecker.c
    
    ${FMUCHK_HOME}/src/FMI1/fmi1_input_reader.c	
	${FMUCHK_HOME}/src/FMI1/fmi1_check.c
	${FMUCHK_HOME}/src/FMI1/fmi1_me_sim.c
	${FMUCHK_HOME}/src/FMI1/fmi1_cs_sim.c
    
	${FMUCHK_HOME}/src/FMI2/fmi2_input_reader.c
	${FMUCHK_HOME}/src/FMI2/fmi2_check.c
	${FMUCHK_HOME}/src/FMI2/fmi2_me_sim.c
	${FMUCHK_HOME}/src/FMI2/fmi2_cs_sim.c
	)
set(HEADERS 
    ${FMUCHK_HOME}/include/fmi1_input_reader.h
	${FMUCHK_HOME}/include/fmi2_input_reader.h
	${FMUCHK_HOME}/include/fmuChecker.h)

include_directories(
	${FMUCHK_BUILD}/FMIL/install/include/
	include/Common/
	include
    ${CMAKE_BINARY_DIR})

add_executable(${fmuCheck} ${SOURCE} ${HEADERS})
target_link_libraries(${fmuCheck} fmilib)
if(WIN32)
	target_link_libraries(${fmuCheck} Shlwapi)
endif(WIN32)
if(UNIX) 
	target_link_libraries(${fmuCheck} dl)
endif(UNIX)

install(TARGETS ${fmuCheck} DESTINATION ${FMUCHK_INSTALL_PREFIX})

# tests
if(NOT ${FMUCHK_TEST_FMUS_DIR} STREQUAL "" AND EXISTS "${FMUCHK_TEST_FMUS_DIR}")
    file(TO_CMAKE_PATH ${FMUCHK_TEST_FMUS_DIR} FMUCHK_TEST_FMUS_DIR_NORMALIZED)
else()
    file(TO_CMAKE_PATH "${FMUCHK_HOME}/TestFMUs" FMUCHK_TEST_FMUS_DIR_NORMALIZED)    
endif()

file(GLOB_RECURSE TEST_FMUS "${FMUCHK_TEST_FMUS_DIR_NORMALIZED}/*.fmu" )
	   
set(BAD_FMUS 
	${FMUCHK_BUILD}/FMIL/build/Testing/BouncingBall2_malformed_mf.fmu
	${FMUCHK_BUILD}/FMIL/build/Testing/BouncingBall_malformed_mf.fmu)

set(ALL_TEST_FMUS 
	${FMUCHK_BUILD}/FMIL/build/Testing/BouncingBall_cs.fmu
	${FMUCHK_BUILD}/FMIL/build/Testing/BouncingBall_cs_tc.fmu
	${FMUCHK_BUILD}/FMIL/build/Testing/BouncingBall_me.fmu
	${FMUCHK_BUILD}/FMIL/build/Testing/BouncingBall2_cs.fmu
	${FMUCHK_BUILD}/FMIL/build/Testing/BouncingBall2_me.fmu
	${TEST_FMUS})

ENABLE_TESTING()

set(TEST_OUT_DIR ${FMUCHK_BUILD}/TestOutput)
file(MAKE_DIRECTORY ${TEST_OUT_DIR}) 

add_test(
	NAME Test_FMIL
	WORKING_DIRECTORY ${FMUCHK_BUILD}/FMIL/build
	COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>)

add_test(
	NAME Build_before_test
	COMMAND "${CMAKE_COMMAND}" --build ${FMUCHK_BUILD} --config $<CONFIGURATION>)

SET_TESTS_PROPERTIES ( 
		Build_before_test PROPERTIES DEPENDS Test_FMIL)


set(TEST_COMMAND_LINE_PARSING OFF)
if(TEST_COMMAND_LINE_PARSING)
	add_test(check_command_line_parsing_0 ${fmuCheck} )
	add_test(check_command_line_parsing_1 ${fmuCheck} -x -h a.fmu )
	set_tests_properties(check_command_line_parsing_1 PROPERTIES WILL_FAIL TRUE)
	add_test(check_command_line_parsing_2 ${fmuCheck} -xx -h a.fmu )
	set_tests_properties(check_command_line_parsing_2 PROPERTIES WILL_FAIL TRUE)
	add_test(check_command_line_parsing_3 ${fmuCheck} -l 5 -s a.fmu )
	set_tests_properties(check_command_line_parsing_3 PROPERTIES WILL_FAIL TRUE)
	add_test(check_command_line_parsing_4 ${fmuCheck} -l 5  )
	set_tests_properties(check_command_line_parsing_4 PROPERTIES WILL_FAIL TRUE)
	add_test(check_command_line_parsing_5 ${fmuCheck} -l 6 a.fmu -x  )
	set_tests_properties(check_command_line_parsing_5 PROPERTIES WILL_FAIL TRUE)
	add_test(check_command_line_parsing_6 ${fmuCheck} -l 6 -n -1 a.fmu  )
	set_tests_properties(check_command_line_parsing_6 PROPERTIES WILL_FAIL TRUE)
	add_test(check_command_line_parsing_7 ${fmuCheck} -l 6 -n 10 -s 1 -c -1 a.fmu  )
	set_tests_properties(check_command_line_parsing_7 PROPERTIES WILL_FAIL TRUE)
	add_test(check_command_line_parsing_8 ${fmuCheck} -k no_such_kind a.fmu  )
	set_tests_properties(check_command_line_parsing_8 PROPERTIES WILL_FAIL TRUE)
	add_test(check_bad_fmu_file ${fmuCheck} -l 6 no_such_file.fmu  )
	set_tests_properties(check_bad_fmu_file PROPERTIES WILL_FAIL TRUE)
endif()

add_test(
	NAME check_with_default_params 
	COMMAND ${fmuCheck}  ${FMUCHK_BUILD}/FMIL/build/Testing/BouncingBall_cs.fmu)

set_tests_properties ( 
		check_with_default_params 
		PROPERTIES DEPENDS Build_before_test)

add_test(
	NAME check_require_me
	COMMAND ${fmuCheck} -k me  ${FMUCHK_BUILD}/FMIL/build/Testing/BouncingBall2_me.fmu)
set_tests_properties ( 
		check_require_me
		PROPERTIES DEPENDS Build_before_test)

add_test(
	NAME check_require_me_fail
	COMMAND ${fmuCheck} -k me  ${FMUCHK_BUILD}/FMIL/build/Testing/BouncingBall2_cs.fmu)
set_tests_properties ( 
		check_require_me_fail
		PROPERTIES DEPENDS Build_before_test
#        WILL_FAIL TRUE # non-fatal error ==> zero exit code currently
)

add_test(
	NAME check_require_cs
	COMMAND ${fmuCheck} -k cs  ${FMUCHK_BUILD}/FMIL/build/Testing/BouncingBall2_cs.fmu)
set_tests_properties ( 
		check_require_cs
		PROPERTIES DEPENDS Build_before_test)

add_test(
	NAME check_require_cs_fail
	COMMAND ${fmuCheck} -k cs  ${FMUCHK_BUILD}/FMIL/build/Testing/BouncingBall2_me.fmu)
set_tests_properties ( 
		check_require_cs_fail
		PROPERTIES DEPENDS Build_before_test
#        WILL_FAIL TRUE # non-fatal error ==> zero exit code currently
)

add_test(
	NAME check_require_me_cs_fail
	COMMAND ${fmuCheck} -k cs -k me  ${FMUCHK_BUILD}/FMIL/build/Testing/BouncingBall2_me.fmu)
set_tests_properties ( 
		check_require_me_cs_fail
		PROPERTIES DEPENDS Build_before_test
#        WILL_FAIL TRUE # non-fatal error ==> zero exit code currently
)

add_test(
	NAME check_xml_on_me
	COMMAND ${fmuCheck} -k xml  ${FMUCHK_BUILD}/FMIL/build/Testing/BouncingBall2_me.fmu)
set_tests_properties ( 
		check_xml_on_me
		PROPERTIES DEPENDS Build_before_test)

add_test(
	NAME check_xml_on_cs
	COMMAND ${fmuCheck} -k xml  ${FMUCHK_BUILD}/FMIL/build/Testing/BouncingBall2_cs.fmu)
set_tests_properties ( 
		check_xml_on_cs
		PROPERTIES DEPENDS Build_before_test)

foreach(fmu ${BAD_FMUS})
	string(REPLACE "/" "_" testname "check_${fmu}")
	string(REPLACE ":" "_" testname ${testname})
	add_test(
		NAME ${testname}
		WORKING_DIRECTORY ${TEST_OUT_DIR}
		COMMAND ${fmuCheck} -t ${TEST_OUT_DIR} -l 6 -o ${testname}.csv -e ${testname}.log ${fmu} )
    set_tests_properties(${testname} PROPERTIES WILL_FAIL TRUE)
	set_tests_properties ( 
		${testname} 
		PROPERTIES DEPENDS Build_before_test)
endforeach(fmu)

foreach(fmu ${ALL_TEST_FMUS})
	string(REPLACE "/" "_" testname "check_${fmu}")
	string(REPLACE ":" "_" testname ${testname})
	add_test(
		NAME ${testname}
		WORKING_DIRECTORY ${TEST_OUT_DIR}
		COMMAND ${fmuCheck} -t ${TEST_OUT_DIR} -l 6 -o ${testname}.csv -e ${testname}.log ${fmu} )
	set_tests_properties ( 
		${testname} 
		PROPERTIES DEPENDS Build_before_test)
endforeach(fmu)
