// RUN: %check_clang_tidy %s cata-header-guard %t -- -plugins=%cata_plugin --
// CHECK-MESSAGES: warning: Header is missing header guard. [cata-header-guard]
// CHECK-FIXES: #ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_TEST_OUTPUT_HEADER_GUARD_3_H_TMP_CPP
// CHECK-FIXES: #define CATA_TOOLS_CLANG_TIDY_PLUGIN_TEST_OUTPUT_HEADER_GUARD_3_H_TMP_CPP

int foo();

// CHECK-FIXES: #endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_TEST_OUTPUT_HEADER_GUARD_3_H_TMP_CPP
