// RUN: %check_clang_tidy %s cata-header-guard,cata-no-long %t -- -plugins=%cata_plugin --
// Should not need a header guard because it's not a header.

// Include another warning just to avoid the "there were no warnings in your
// test case" error.
long foo();
// CHECK-MESSAGES: warning: Function 'foo' declared as returning 'long'.  Prefer int or int64_t to long. [cata-no-long]
