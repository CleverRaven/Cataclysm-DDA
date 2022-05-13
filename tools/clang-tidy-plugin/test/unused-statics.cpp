// RUN: %check_clang_tidy %s cata-unused-statics %t -- -plugins=%cata_plugin --

class A0 {};

static A0 a0;
// CHECK-MESSAGES: warning: Variable 'a0' declared but not used.  [cata-unused-statics]

namespace cata
{

class A1 {};

static A1 a1;
// CHECK-MESSAGES: warning: Variable 'a1' declared but not used.  [cata-unused-statics]

}

namespace
{

class A2 {};

A2 a2;
// CHECK-MESSAGES: warning: Variable 'a2' declared but not used.  [cata-unused-statics]

}

// No warnings inside macros
class A3 {};
#define M3 static A3 a3;
M3
