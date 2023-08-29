// RUN: %check_clang_tidy %s cata-large-inline-function %t -- --load=%cata_plugin --

inline void f0()
// CHECK-MESSAGES: warning: Function 'f0' declared inline in a cpp file. 'inline' should only be used in header files. [cata-large-inline-function]
{
}

struct A1 {
    A1();
    ~A1();
};

// B1 will get compiler-defined members which will be inline; check that those
// don't trigger the warning by creating a B1 instance below.
struct B1 {
    A1 a1;
};

void f1()
{
    B1 b1;
}

// Verify that in-class definitions don't trigger the warning in cpp files
struct A2 {
    void f2() {
    }
};

// Verify that global-scope lambdas don't trigger a warning
auto f3 = []()
{
};

// Verify that macros can define inline functions
#define DEFINE_F4(name) \
    inline void name() {}
DEFINE_F4( f4 );
