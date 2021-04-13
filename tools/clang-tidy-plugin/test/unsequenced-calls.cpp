// RUN: %check_clang_tidy %s cata-unsequenced-calls %t -- -plugins=%cata_plugin --

struct A {
    // Define a const and a non-const member function
    int const_mf() const;
    int nonconst_mf();
};

void g( int, int );

void f0()
{
    A a;
    // Two calls to const functions are fine
    g( a.const_mf(), a.const_mf() );
    // But if at least one is non-const, raise a warning
    g( a.nonconst_mf(), a.const_mf() );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Unsequenced calls to member functions of 'a', at least one of which is non-const. [cata-unsequenced-calls]
    g( a.const_mf(), a.nonconst_mf() );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Unsequenced calls to member functions of 'a', at least one of which is non-const. [cata-unsequenced-calls]
    g( a.nonconst_mf(), a.nonconst_mf() );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Unsequenced calls to member functions of 'a', at least one of which is non-const. [cata-unsequenced-calls]
}
