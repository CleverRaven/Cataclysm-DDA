// RUN: %check_clang_tidy %s cata-no-long %t -- -plugins=%cata_plugin --

#include <stdint.h>

long a1;
// CHECK-MESSAGES: warning: Variable 'a1' declared as 'long'. Prefer int or int64_t to long. [cata-no-long]

unsigned long a2;
// CHECK-MESSAGES: warning: Variable 'a2' declared as 'unsigned long'. Prefer unsigned int or uint64_t to unsigned long. [cata-no-long]

const long a3 = 0;
// CHECK-MESSAGES: warning: Variable 'a3' declared as 'const long'. Prefer int or int64_t to long. [cata-no-long]

long &a4 = a1;
// CHECK-MESSAGES: warning: Variable 'a4' declared as 'long &'. Prefer int or int64_t to long. [cata-no-long]

int64_t c;
uint64_t d;

void f1(long e);
// CHECK-MESSAGES: warning: Variable 'e' declared as 'long'. Prefer int or int64_t to long. [cata-no-long]

long f2();
// CHECK-MESSAGES: warning: Function 'f2' declared as returning 'long'. Prefer int or int64_t to long. [cata-no-long]

int64_t f3();
auto f4() -> decltype(0L);

int i1 = static_cast<long>(0);
// CHECK-MESSAGES: warning: Static cast to 'long'.  Prefer int or int64_t to long. [cata-no-long]
int i2 = static_cast<int64_t>(0);
