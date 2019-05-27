// RUN: %check_clang_tidy %s cata-no-long %t -- -plugins=%cata_plugin --

long a;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: Variable 'a' declared as long. Prefer int or int64_t. [cata-no-long]
