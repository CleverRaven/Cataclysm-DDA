// RUN: %check_clang_tidy %s cata-static-declarations %t -- -plugins=%cata_plugin --

const int i0 = 0;
// CHECK-MESSAGES: warning: Global declaration of 'i0' in a cpp file should be static or have a previous declaration in a header. [cata-static-declarations]
// CHECK-FIXES: static const int i0 = 0;

void f0();
// CHECK-MESSAGES: warning: Global declaration of 'f0' in a cpp file should be static or have a previous declaration in a header. [cata-static-declarations]
// CHECK-FIXES: static void f0();
