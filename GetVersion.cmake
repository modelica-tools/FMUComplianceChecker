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
  if(Subversion_FOUND)
    Subversion_WC_INFO("${dir}" "SVNVER")
    message(STATUS "SVNVER_WC_REVISION= ${SVNVER_WC_REVISION}, info = ${SVNVER_WC_INFO}")
    
    string(REGEX MATCH "URL.*tags/" SVNTAGS "${SVNVER_WC_INFO}")
    if(SVNTAGS)
        string(REGEX MATCH "${SVNTAGS}[^\n]*" SVNTAGTMP "${SVNVER_WC_INFO}")
        string(REPLACE "${SVNTAGS}" "" SVNTAG "${SVNTAGTMP}")
        set(${variable} "${SVNTAG}" PARENT_SCOPE)
    else()
        set(${variable} "rev${SVNVER_WC_REVISION}" PARENT_SCOPE)
    endif()  
  else()
    set(${variable} "Test" PARENT_SCOPE)
  endif()
message(STATUS "Version: ${${variable}}")
endfunction(getVersionSVN)