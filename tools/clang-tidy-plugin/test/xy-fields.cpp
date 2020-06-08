// RUN: %check_clang_tidy %s cata-xy %t -- -plugins=%cata_plugin --

struct A0 {
    // CHECK-MESSAGES: warning: 'A0' defines fields 'x' and 'y'.  Consider combining into a single point field. [cata-xy]
    int x;
    int y;
};

struct A1 {
    // CHECK-MESSAGES: warning: 'A1' defines fields 'foox' and 'fooy'.  Consider combining into a single point field. [cata-xy]
    int foox;
    int fooy;
};

struct A2 {
    int foox;
    int bary;
};

struct A3 {
    // CHECK-MESSAGES: warning: 'A3' defines fields 'foox' and 'fooy'.  Consider combining into a single point field. [cata-xy]
    int foox;
    int bary;
    int fooy;
};

struct A4 {
    // CHECK-MESSAGES: warning: 'A4' defines fields 'barx' and 'bary'.  Consider combining into a single point field. [cata-xy]
    // CHECK-MESSAGES: warning: 'A4' defines fields 'foox' and 'fooy'.  Consider combining into a single point field. [cata-xy]
    int foox;
    int fooy;
    int barx;
    int bary;
};

struct B {
    // CHECK-MESSAGES: warning: 'B' defines fields 'x', 'y', and 'z'.  Consider combining into a single tripoint field. [cata-xy]
    int x;
    int y;
    int z;
};

template<int>
struct C {
    // CHECK-MESSAGES: warning: 'C' defines fields 'x' and 'y'.  Consider combining into a single point field. [cata-xy]
    int x;
    int y;
};

C<0> c0;
C<1> c1;

struct D {
    // Verify that there are no warnings for non-int types
    float x;
    float y;
};

struct E {
    // Verify that there is not warning for this case, which was causing a
    // false positive
    int text_x_start;
    int text_x_end;
    int y;
};
