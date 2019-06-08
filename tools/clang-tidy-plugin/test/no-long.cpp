// RUN: %check_clang_tidy %s cata-no-long %t -- -plugins=%cata_plugin --

#include <stdint.h>

long a;
// CHECK-MESSAGES: warning: Variable 'a' declared as long. Prefer int or int64_t. [cata-no-long]

unsigned long b;
// CHECK-MESSAGES: warning: Variable 'b' declared as unsigned long. Prefer unsigned int or uint64_t. [cata-no-long]

int64_t c;
uint64_t d;

void f1(long e);
// CHECK-MESSAGES: warning: Variable 'e' declared as long. Prefer int or int64_t. [cata-no-long]

long f2();
// CHECK-MESSAGES: warning: Function 'f2' declared as returning long. Prefer int or int64_t. [cata-no-long]

int64_t f3();
auto f4() -> decltype(0L);

int i1 = static_cast<long>(0);
int i2 = static_cast<int64_t>(0);
