# CMake generated Testfile for 
# Source directory: /home/point/OpenSource/Cataclysm-DDA/tests
# Build directory: /home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test "sh" "-c" "/home/point/OpenSource/Cataclysm-DDA/cata_test --rng-seed time")
set_tests_properties(test PROPERTIES  WORKING_DIRECTORY "/home/point/OpenSource/Cataclysm-DDA" _BACKTRACE_TRIPLES "/home/point/OpenSource/Cataclysm-DDA/tests/CMakeLists.txt;42;add_test;/home/point/OpenSource/Cataclysm-DDA/tests/CMakeLists.txt;0;")
