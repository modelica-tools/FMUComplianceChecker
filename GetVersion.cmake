#
#    Copyright (C) 2012 Modelon AB <http://www.modelon.com>
#
#	You should have received a copy of the LICENCE-FMUChecker.txt
#   along with this program. If not, contact Modelon AB.
#

#   File: GetVesion.cmake
#   CMake module for getting the version information from SVN.

include(FindSubversion)

# Uses svn info to find out the version of the code and sets the "variable" to it.
# Version becomes "Test" if svn commands fail.
function(getVersionSVN dir variable)
  TestSVNOK(${dir} test_svn_result)
  if(${test_svn_result} EQUAL 0)
    Subversion_WC_INFO(${dir} SVNVER)
    string(REGEX MATCH "URL.*tags/" SVNTAGS "${SVNVER_WC_INFO}")
    if(SVNTAGS)
        string(REGEX MATCH "${SVNTAGS}[^\n]*" SVNTAGTMP "${SVNVER_WC_INFO}")
        string(REPLACE "${SVNTAGS}" "" SVNTAG "${SVNTAGTMP}")
        set(${variable} "${SVNTAG}" PARENT_SCOPE)
    else()
        set(${variable} "rev${SVNVER_WC_REVISION}" PARENT_SCOPE)
    endif()  
  else()
    message(STATUS "Setting version to 'Test' since no Subversion info is found")
    set(${variable} "Test" PARENT_SCOPE)
  endif()
endfunction(getVersionSVN)

function(TestSVNOK dir svnok)
  if(COMMAND Subversion_WC_INFO)
    EXECUTE_PROCESS(COMMAND ${Subversion_SVN_EXECUTABLE} info ${dir}
        OUTPUT_VARIABLE TEST_WC_INFO
        ERROR_VARIABLE test_svn_error
        RESULT_VARIABLE test_svn_result
        OUTPUT_STRIP_TRAILING_WHITESPACE)
  else()
    set(test_svn_result 1)    
  endif()
  set(${svnok} ${test_svn_result} PARENT_SCOPE)
endfunction(TestSVNOK)