// RUN: %check_clang_tidy %s cata-test-filename %t -- -plugins=%cata_plugin -- -isystem %cata_include

#define TEST_CASE(name)

TEST_CASE( "test_name" )
// CHECK-MESSAGES: warning: Files containing a test definition should have a filename ending in '_test.cpp'. [cata-test-filename]
