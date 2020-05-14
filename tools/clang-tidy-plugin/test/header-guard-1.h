// RUN: %check_clang_tidy %s cata-header-guard %t -- -plugins=%cata_plugin --

#ifndef THE_WRONG_HEADER_GUARD
// CHECK-MESSAGES: warning: Header guard does not follow preferred style. [cata-header-guard]
// CHECK-FIXES: #ifndef CATA_BUILD_TOOLS_CLANG_TIDY_PLUGIN_TEST_OUTPUT_HEADER_GUARD_1_H_TMP_CPP
#define THE_WRONG_HEADER_GUARD
// CHECK-FIXES: #define CATA_BUILD_TOOLS_CLANG_TIDY_PLUGIN_TEST_OUTPUT_HEADER_GUARD_1_H_TMP_CPP

#endif
// CHECK-FIXES: #endif // CATA_BUILD_TOOLS_CLANG_TIDY_PLUGIN_TEST_OUTPUT_HEADER_GUARD_1_H_TMP_CPP
