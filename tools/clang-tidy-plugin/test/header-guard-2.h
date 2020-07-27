// RUN: %check_clang_tidy %s cata-header-guard %t -- -plugins=%cata_plugin --

#ifndef CATA_BUILD_TOOLS_CLANG_TIDY_PLUGIN_TEST_OUTPUT_HEADER_GUARD_2_H_TMP_CPP
#define CATA_BUILD_TOOLS_CLANG_TIDY_PLUGIN_TEST_OUTPUT_HEADER_GUARD_2_H_TMP_CPP

#endif
// CHECK-MESSAGES: warning: #endif for a header guard should reference the guard macro in a comment. [cata-header-guard]
// CHECK-FIXES: #endif // CATA_BUILD_TOOLS_CLANG_TIDY_PLUGIN_TEST_OUTPUT_HEADER_GUARD_2_H_TMP_CPP
