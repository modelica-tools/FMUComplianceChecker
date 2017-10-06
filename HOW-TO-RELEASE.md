Instructions for how to release FMI Compliance Checker (FMUChecker) application.

1. Release FMILibrary.
2. Set the CMake external project for FMI Library to point to the newly released
   version on trunk.
3. Prepare RELEASE-NOTES-FMUChecker.txt for a new release.
4. Merge changes to the release branch.
5. Create a release tag from the branch.
6. Follow instructions in 'branches/Packaging/FMUChecker_build_all_platforms.sh'
   and run the script to build the release.
7. Commit release to
   https://svn.fmi-standard.org/fmi/branches/public/Test_FMUs/Compliance-Checker/
8. Join the googlegroup fmi-design and announce the new release.
