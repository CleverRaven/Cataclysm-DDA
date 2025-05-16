// RUN: %check_clang_tidy %s cata-static-declarations %t -- --load=%cata_plugin --

static void f0()
// CHECK-MESSAGES: warning: Declaration 'f0' should not be declared static in a header.  Either move the definition to a cpp file or declare it inline instead. [cata-static-declarations]
{
}

static int i0;
// CHECK-MESSAGES: warning: Declaration 'i0' should not be declared static in a header.  Either move the definition to a cpp file or declare it inline instead. [cata-static-declarations]

static const int i1 = 0;
// CHECK-MESSAGES: warning: Declaration 'i1' should not be declared static in a header.  It is const, so implicitly has internal linkage and the static keyword can be removed. [cata-static-declarations]

static constexpr int i2 = 0;
// CHECK-MESSAGES: warning: Declaration 'i2' should not be declared static in a header.  It is const, so implicitly has internal linkage and the static keyword can be removed. [cata-static-declarations]
