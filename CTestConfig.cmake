## This file should be placed in the root directory of your project.
## Then modify the CMakeLists.txt file in the root directory of your
## project to incorporate the testing dashboard.
## # The following are required to uses Dart and the Cdash dashboard
##   ENABLE_TESTING()
##   INCLUDE(CTest)
set( CTEST_PROJECT_NAME "Draco" )
set( CTEST_NIGHTLY_START_TIME "00:0:01 MST" )

set( CTEST_DROP_METHOD "http")
set( CTEST_DROP_SITE "coder.lanl.gov" )
set( CTEST_DROP_LOCATION  "/cdash/submit.php?project=${CTEST_PROJECT_NAME}" )
set( CTEST_DROP_SITE_CDASH TRUE )

