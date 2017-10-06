#!/bin/bash -x
FMUCHECKERVERSION=FMUChecker-2.0.4b1
# The script is used to create FMU checker executables for 4 different platforms
# It is intended to be run from MSYS shell on Windows on a computer with
#  the following installed:
# 1. cmake 2.8.6 or later
# 2. svn client
# 3. 7zip
# 4. MSVC + WinSDK for Win64 bit builds (check with cmake that those work)
#    If Visual Studio 10 is used in Windows 7, WinSDK should be installed in the
#    following way (the order is important):
#        a. Microsoft Windows SDK for Windows 7.1
#        b. Microsoft Visual Studio 2010 Service Pack 1
#        c. Microsoft Visual C++ 2010 Service Pack 1 Compiler Update for the
#           Windows SDK 7.1
MSVC="Visual Studio 10"
MSVC64="Visual Studio 10 Win64"
X_7zip="/c/Program Files/7-Zip/7z"

# 5. Access to Linux64 machine (e.g., VirtualBox with Ubuntu 64bit configured
#    with host only network adapter). The Ubuntu is expected to have:
#    svn
#    cmake > 2.8.6
#    g++ multiarch (support for -m32 for cross-build Linux32), which can be
#        installed with:
#            sudo apt-get install g++-multilib
LINUXHOST=victor@192.168.56.101
#    ssh server with login without password setup. If you don't have an ssh
#        server installed, it can be installed with:
#
#            sudo apt-get install openssh-server
#
#        The ssh server can be set up to use keys instead of passwords with
#        the MSYS shell in the following way:
#
#            ssh victor@192.168.56.101
#            ssh-keygen -t rsa
#
#        set the file to save the key to id_rsa and no passphrase. And then
#        while still logged in to the linux machine:
#
#            mkdir .ssh
#            chmod 700 .ssh
#            cd .ssh
#            touch authorized_keys
#            chmod 600 authorized_keys
#            cat ../id_rsa.pub >> authorized_keys
#            rm ../id_rsa.pub
#            exit
#
#         And then from the MSYS shell:
#
#            scp victor@192.168.56.101:~/id_rsa ~/.ssh
#
#         Try the ssh server setup in the following way (no password should be
#         required):
#
#            ssh -v victor@192.168.56.101
#
# The build directory is always cleaned before use. Source directory is
#  updated or check-out is done.
SRCDIR=`pwd`/src
BUILDDIR=`pwd`/build
REMOTESRCDIR=/home/victor/FMUCheckerBuild
REMOTEBUILDDIR=/tmp/FMUCheckerBuild

# Almost no error checking in the script. So, please, read the output!
FMUCHK_REPO="https://github.com/modelica-tools/FMUComplianceChecker.git/tags/2.0.4b1"
# the build dir will be removed with rm -rf if RMBUILDDIR="YES"
RMBUILDDIR="YES"
################################################################################
# NO MORE SETTINGS - RUNNING
################################################################################
# Get the sources
mkdir $SRCDIR >& /dev/null
pushd $SRCDIR
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
cmake -DFMUCHK_INSTALL_PREFIX="./install" $SRCDIR/FMUCHK -G "MSYS Makefiles" || exit 1
cmake --build . --config MinSizeRel --target install || exit 1
ctest -C MinSizeRel || exit 1
cd ..
# Build Win32
cd build32 || exit 1
cmake $SRCDIR/FMUCHK -G "$MSVC" || exit 1
cmake --build . --config MinSizeRel --target install || exit 1
ctest -C MinSizeRel || exit 1
# Build Win64
cd ..
cd build64
cmake $SRCDIR/FMUCHK -G "$MSVC64" || exit 1
cmake --build . --config MinSizeRel --target install || exit 1
ctest -C MinSizeRel || exit 1
popd

# Local build is ready, now build Linux
ssh $LINUXHOST "mkdir $REMOTESRCDIR >& /dev/null"
ssh $LINUXHOST "mkdir $REMOTESRCDIR/FMUCHK >& /dev/null && svn co $FMUCHK_REPO $REMOTESRCDIR/FMUCHK" || \
ssh $LINUXHOST "cd $REMOTESRCDIR/FMUCHK && svn switch $FMUCHK_REPO && svn up" ||exit 1
ssh $LINUXHOST "rm -rf $REMOTEBUILDDIR/install >& /dev/null"
if [ "$RMBUILDDIR" == "YES" ]; then 
	ssh $LINUXHOST "rm -rf $REMOTEBUILDDIR >& /dev/null"	
fi
ssh $LINUXHOST "(mkdir $REMOTEBUILDDIR &&	mkdir $REMOTEBUILDDIR/build32 && mkdir $REMOTEBUILDDIR/build64)  >& /dev/null"
# build linux32
ssh $LINUXHOST "cd $REMOTEBUILDDIR/build32; cmake -DFMUCHK_FMI_PLATFORM=linux32 -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_EXE_LINKER_FLAGS=-m32 -DCMAKE_SHARED_LINKER_FLAGS=-m32 $REMOTESRCDIR/FMUCHK"
ssh $LINUXHOST "cd $REMOTEBUILDDIR/build32; cmake --build . --target install"  || exit 1
ssh $LINUXHOST "cd $REMOTEBUILDDIR/build32; ctest" || exit 1
#build linux64
ssh $LINUXHOST "cd $REMOTEBUILDDIR/build64; cmake $REMOTESRCDIR/FMUCHK"
ssh $LINUXHOST "cd $REMOTEBUILDDIR/build64; cmake --build . --target install"  || exit 1
ssh $LINUXHOST "cd $REMOTEBUILDDIR/build64; ctest" || exit 1

scp -r $SRCDIR/FMUCHK/FMIL/ThirdParty/FMI/default $SRCDIR/FMUCHK/*.md $LINUXHOST:$REMOTEBUILDDIR/install|| exit 1
ssh $LINUXHOST "mv $REMOTEBUILDDIR/install/default $REMOTEBUILDDIR/install/include" || exit 1

for platform in "linux32" "linux64" ; do 
	ssh $LINUXHOST "mkdir $REMOTEBUILDDIR/install/$FMUCHECKERVERSION-$platform  >& /dev/null"
	ssh $LINUXHOST "cd $REMOTEBUILDDIR/install; cp -al include *.md fmuCheck.$platform $FMUCHECKERVERSION-$platform" || exit 1
	ssh $LINUXHOST "cd $REMOTEBUILDDIR/install; zip -r $FMUCHECKERVERSION-$platform.zip $FMUCHECKERVERSION-$platform"|| exit 1
done

# copy the results back
scp $LINUXHOST:$REMOTEBUILDDIR/install/\*.zip $BUILDDIR/install
ls $BUILDDIR/install

# now packaging
cp -r $SRCDIR/FMUCHK/FMIL/ThirdParty/FMI/default  $BUILDDIR/install
mv $BUILDDIR/install/default $BUILDDIR/install/include
cp $SRCDIR/FMUCHK/*.md $BUILDDIR/install
svn export $SRCDIR/FMUCHK $BUILDDIR/install/$FMUCHECKERVERSION

pushd $BUILDDIR/install
for platform in "win32" "win64"; do
	mkdir $FMUCHECKERVERSION-$platform  >& /dev/null
	cp -r include *.md fmuCheck.$platform* $FMUCHECKERVERSION-$platform
	"$X_7zip"  a -tzip $FMUCHECKERVERSION-$platform.zip $FMUCHECKERVERSION-$platform
done
"$X_7zip"  a -tzip $FMUCHECKERVERSION-src.zip $FMUCHECKERVERSION
popd