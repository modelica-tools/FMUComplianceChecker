Build instructions for FMI Compliance Checker (FMUChecker) application.

FMUChecker build system utilizes CMake (see <http://www.cmake.org/>)
to generate the native build scripts for the application.
It is recommended to use "cmake-gui" on Windows or "ccmake"
to configure the build.

To build from a terminal command line on Linux or Mac with default settings use:
```
    mkdir build; cd build
    cmake -DFMUCHK_INSTALL_PREFIX=<dir name> <checker dir>
    make install test
```
To build in MSYS terminal with g++/gcc on Windows:
```
    mkdir build; cd build
    cmake -DFMUCHK_INSTALL_PREFIX=<dir name> -G "MSYS Makefiles" <checker dir>
    make install test
```
To build from command line with Microsoft Visual Studio 10 compilers on Windows:
```
    mkdir build; cd build
    cmake -DFMUCHK_INSTALL_PREFIX=<dir name> -G "Visual Studio 10" <checker dir>    
    cmake --build . --config MinSizeRel --target install
```
The generated solution files can be used in the Microsoft Visual Studio IDE as
well.

The primary target of the library build script is a command-line
executable:  <prefix>/bin/fmuChecket.<platform>[.extension]

Note that FMUChecker depends on FMI Library that is checked out with svn and
stored in the FMIL subdirectory of the source tree when building. For more
information on FMI Library, see <http://www.fmi-library.org>. The current
version of the FMUChecker depends on FMI Library 2.0.2.

The following FMUChecker specific build configuration options are provided:

FMUCHK_INSTALL_PREFIX - Prefix prepended to install directories.
    Default: <build_dir>/../install

FMUCHK_FMI_STANDARD_HEADERS  - Path to the FMI standard headers directory.
    Leave empty to use the headers from FMIL, which is the default.
    Headers for specific standards versions are expected in subdirectories
    FMI1, FMI2, etc.

FMUCHK_ENABLE_LOG_LEVEL_DEBUG - Enable log level 'debug'. If the option
    is of then the debug level is not compiled in. Default: OFF

FMUCHK_TEST_FMUS_DIR - Directory with FMUs to be used in tests (checker
    will run for each FMU). Output files can be found in <build>/TestOutput/
    Default: ${FMUCHK_HOME}/TestFMUs.

FMUCHK_BUILD_WITH_STATIC_RTLIB - Use static run-time libraries (/MT or
    /MTd linker flags). Default: ON. Only available for Microsoft Visual
    Studio projects.

FMUCHK_FMI_PLATFORM - Defines the subdirectory within FMU where binaries
    should be searched, e.g., win32. Default: detected automatically
    for win32/64, linux32/64, darwin32/64. Overriding the default
    value normally requires modification to compiler and linker flags
    to get a consistent build. May be used to build 32-bit binary
    on a 64-bit system.

FMUCHK_FMI_PUBLIC - Specifies a directory with Subversion check-out from
    https://svn.fmi-standard.org/fmi/branches/public with test fmus. The
    directory is expected to contain Test_FMUs and CrossCheck_Results
    subdirectories. Checker will run cross-check with FMI Library for
    all the FMUs found as a part of test suit.
