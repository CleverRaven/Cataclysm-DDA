// RUN: %check_clang_tidy %s cata-static-declarations %t -- -plugins=%cata_plugin --

const int i0 = 0;
// CHECK-MESSAGES: warning: Global declaration of 'i0' in a cpp file should be static or have a previous declaration in a header. [cata-static-declarations]
// CHECK-FIXES: static const int i0 = 0;

void f0();
// CHECK-MESSAGES: warning: Global declaration of 'f0' in a cpp file should be static or have a previous declaration in a header. [cata-static-declarations]
// CHECK-FIXES: static void f0();

extern int i1;
// CHECK-MESSAGES: warning: Prefer including a header to making a local extern declaration of 'i1'. [cata-static-declarations]

int i2 = 0;
// CHECK-MESSAGES: warning: Global declaration of 'i2' in a cpp file should be static or have a previous declaration in a header. [cata-static-declarations]
// CHECK-FIXES: static int i2 = 0;

const int i3[2] = { 0 };
// CHECK-MESSAGES: warning: Global declaration of 'i3' in a cpp file should be static or have a previous declaration in a header. [cata-static-declarations]
// CHECK-FIXES: static const int i3[2] = { 0 };

int i4[2] = { 0 };
// CHECK-MESSAGES: warning: Global declaration of 'i4' in a cpp file should be static or have a previous declaration in a header. [cata-static-declarations]
// CHECK-FIXES: static int i4[2] = { 0 };

// Make sure main isn't messed with
int main()
{
}
