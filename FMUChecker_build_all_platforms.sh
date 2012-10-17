#!/bin/bash -x
FMUCHECKERVERSION=FMUChecker-2.0a1
# The script is used to create FMU checker executables for 4 different platforms
# It is intended to be run from MSYS shell on Windows on a computer with
#  the following installed:
# 1. cmake 2.8.6 or later
# 2. svn client
# 3. 7zip
# 4. MSVC + WinSDK for Win64 bit builds (check with cmake that those work)
MSVC="Visual Studio 10"
MSVC64="Visual Studio 10 Win64"

# 5. Access to Linux64 machine (e.g., VirtualBox with Ubuntu 64bit configured with host only network)
#    adapter. The Ubuntu is expected to have:
#    ssh  server with login without password setup
LINUXHOST=iakov@192.168.56.101
#    svn
#    cmake > 2.8.6
#    g++ multiarch (support for -m32 for cross-build Linux32)
# The build directory is always cleaned before use. Source directory is
#  updated or check-out is done. Note that svn should know password for svn.modelon.se
#  in order to check-out the checker.
SRCDIR=`pwd`/src
BUILDDIR=`pwd`/build
REMOTESRCDIR=/home/iakov/FMUCheckerBuild
REMOTEBUILDDIR=/tmp/FMUCheckerBuild

# Almost no error checking in the script. So, please, read the output!
FMIL_REPO="https://svn.jmodelica.org/FMILibrary/trunk"
FMUCHK_REPO="https://svn.modelon.se/P533-FMIComplianceChecker/trunk"
# the build dir will be removed with rm -rf if RMBUILDDIR="YES"
RMBUILDDIR="NO"
##########################################################################################
# NO MORE SETTINGS - RUNNING
################################################################################################
# Get the sources
mkdir $SRCDIR >& /dev/null
pushd $SRCDIR
(mkdir FMIL >& /dev/null && svn co $FMIL_REPO FMIL) || (cd FMIL && svn switch $FMIL_REPO && svn up && cd ..) || exit 1
(mkdir FMUCHK >& /dev/null && svn co $FMUCHK_REPO FMUCHK) || (cd  FMUCHK && svn switch $FMUCHK_REPO && svn up && cd ..) || exit 1
popd

# always clear install
rm -rf $BUILDDIR/install >& /dev/null

# clear build dir
if [ "$RMBUILDDIR" == "YES" ]; then 
	rm -rf $BUILDDIR >& /dev/null
fi

(mkdir $BUILDDIR && mkdir $BUILDDIR/build32 &&	mkdir $BUILDDIR/build64 && mkdir $BUILDDIR/build.msys32) >& /dev/null

pushd $BUILDDIR
# Build MSYS just for test
cd build.msys32 || exit 1
cmake -DFMUCHK_INSTALL_PREFIX="./install" -DFMUCHK_FMIL_HOME_DIR=$SRCDIR/FMIL $SRCDIR/FMUCHK -G "MSYS Makefiles" || exit 1
cmake --build . --config MinSizeRel --target install || exit 1
ctest -C MinSizeRel || exit 1
cd ..
# Build Win32
cd build32 || exit 1
cmake -DFMUCHK_FMIL_HOME_DIR=$SRCDIR/FMIL $SRCDIR/FMUCHK -G "$MSVC" || exit 1
cmake --build . --config MinSizeRel --target install || exit 1
ctest -C MinSizeRel || exit 1
# Build Win64
cd ..
cd build64
cmake -DFMUCHK_FMIL_HOME_DIR=$SRCDIR/FMIL $SRCDIR/FMUCHK -G "$MSVC64" || exit 1
cmake --build . --config MinSizeRel --target install || exit 1
ctest -C MinSizeRel || exit 1
popd

# Local build is ready, now build Linux
ssh $LINUXHOST "mkdir $REMOTESRCDIR >& /dev/null"
ssh $LINUXHOST "mkdir $REMOTESRCDIR/FMIL >& /dev/null && svn co $FMIL_REPO $REMOTESRCDIR/FMIL" || \
ssh $LINUXHOST "cd $REMOTESRCDIR/FMIL && svn switch $FMIL_REPO && svn up" ||exit 1
ssh $LINUXHOST "mkdir $REMOTESRCDIR/FMUCHK >& /dev/null && svn co $FMUCHK_REPO $REMOTESRCDIR/FMUCHK" || \
ssh $LINUXHOST "cd $REMOTESRCDIR/FMUCHK && svn switch $FMUCHK_REPO && svn up" ||exit 1
ssh iakov@192.168.56.101 "rm -rf $REMOTEBUILDDIR/install >& /dev/null"
if [ "$RMBUILDDIR" == "YES" ]; then 
	ssh $LINUXHOST "rm -rf $REMOTEBUILDDIR >& /dev/null"	
fi
ssh $LINUXHOST "(mkdir $REMOTEBUILDDIR &&	mkdir $REMOTEBUILDDIR/build32 && mkdir $REMOTEBUILDDIR/build64)  >& /dev/null"
# build linux32
ssh iakov@192.168.56.101 "cd $REMOTEBUILDDIR/build32; cmake -DFMUCHK_FMIL_HOME_DIR=$REMOTESRCDIR/FMIL -DFMUCHK_FMI_PLATFORM=linux32 -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_EXE_LINKER_FLAGS=-m32 -DCMAKE_SHARED_LINKER_FLAGS=-m32 $REMOTESRCDIR/FMUCHK"
ssh iakov@192.168.56.101 "cd $REMOTEBUILDDIR/build32; cmake --build . --target install"  || exit 1
ssh iakov@192.168.56.101 "cd $REMOTEBUILDDIR/build32; ctest" || exit 1
#build linux64
ssh iakov@192.168.56.101 "cd $REMOTEBUILDDIR/build64; cmake -DFMUCHK_FMIL_HOME_DIR=$REMOTESRCDIR/FMIL $REMOTESRCDIR/FMUCHK"
ssh iakov@192.168.56.101 "cd $REMOTEBUILDDIR/build64; cmake --build . --target install"  || exit 1
ssh iakov@192.168.56.101 "cd $REMOTEBUILDDIR/build64; ctest" || exit 1

scp -r $SRCDIR/FMIL/ThirdParty/FMI/default $SRCDIR/FMUCHK/*-FMUChecker.txt iakov@192.168.56.101:$REMOTEBUILDDIR/install|| exit 1
ssh iakov@192.168.56.101 "mv $REMOTEBUILDDIR/install/default $REMOTEBUILDDIR/install/include" || exit 1

for platform in "linux32" "linux64" ; do 
	ssh iakov@192.168.56.101 "mkdir $REMOTEBUILDDIR/install/$FMUCHECKERVERSION-$platform  >& /dev/null"
	ssh iakov@192.168.56.101 "cd $REMOTEBUILDDIR/install; cp -al include *-FMUChecker.txt fmuCheck.$platform $FMUCHECKERVERSION-$platform" || exit 1
	ssh iakov@192.168.56.101 "cd $REMOTEBUILDDIR/install; zip -r $FMUCHECKERVERSION-$platform.zip $FMUCHECKERVERSION-$platform"|| exit 1
done

# copy the results back
scp iakov@192.168.56.101:$REMOTEBUILDDIR/install/\*.zip $BUILDDIR/install
ls $BUILDDIR/install

# now packaging
cp -r $SRCDIR/FMIL/ThirdParty/FMI/default  $BUILDDIR/install
mv $BUILDDIR/install/default $BUILDDIR/install/include
cp $SRCDIR/FMUCHK/*-FMUChecker.txt $BUILDDIR/install
svn export $SRCDIR/FMUCHK $BUILDDIR/install/$FMUCHECKERVERSION

pushd $BUILDDIR/install
for platform in "win32" "win64"; do
	mkdir $FMUCHECKERVERSION-$platform  >& /dev/null
	cp -r include *-FMUChecker.txt fmuCheck.$platform* $FMUCHECKERVERSION-$platform
	7za a -tzip $FMUCHECKERVERSION-$platform.zip $FMUCHECKERVERSION-$platform
done
7za a -tzip $FMUCHECKERVERSION-src.zip $FMUCHECKERVERSION
popd
