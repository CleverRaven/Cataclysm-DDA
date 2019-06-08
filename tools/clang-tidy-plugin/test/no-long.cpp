// RUN: %check_clang_tidy %s cata-no-long %t -- -plugins=%cata_plugin --

#include <stdint.h>

long a;
// CHECK-MESSAGES: warning: Variable 'a' declared as long. Prefer int or int64_t. [cata-no-long]

unsigned long b;
// CHECK-MESSAGES: warning: Variable 'b' declared as unsigned long. Prefer unsigned int or uint64_t. [cata-no-long]

int64_t c;
uint64_t d;

void f(long e);
// CHECK-MESSAGES: warning: Variable 'e' declared as long. Prefer int or int64_t. [cata-no-long]

long g();
// CHECK-MESSAGES: warning: Function 'g' declared as returning long. Prefer int or int64_t. [cata-no-long]

int64_t h();
