// RUN: %check_clang_tidy %s cata-unsequenced-calls %t -- -plugins=%cata_plugin --

struct A {
    // Define a const and a non-const member function
    int const_mf( int = 0 ) const;
    int nonconst_mf( int = 0 );
    int begin();
    int end();
    int begin() const;
    int end() const;
};

void g( int, int );

struct B {
    B( int, int );
};

void f0()
{
    A a;
    // Two calls to const functions are fine
    g( a.const_mf(), a.const_mf() );
    // Calls to begin/end are also fine
    g( a.begin(), a.end() );
    // Calls in a ternary expression are fine
    int n0 = a.nonconst_mf() ? a.nonconst_mf() : a.const_mf();
    // Or by short-circuiting boolean expressions
    int n1 = a.nonconst_mf() && a.nonconst_mf();
    int n2 = a.nonconst_mf() || a.nonconst_mf();
    // Nested calls are OK
    a.nonconst_mf( a.const_mf() );
    // But otherwise, if at least one is non-const, raise a warning
    g( a.nonconst_mf(), a.const_mf() );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Unsequenced calls to member functions of 'a' (of type 'A'), at least one of which is non-const. [cata-unsequenced-calls]
    g( a.const_mf(), a.nonconst_mf() );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Unsequenced calls to member functions of 'a' (of type 'A'), at least one of which is non-const. [cata-unsequenced-calls]
    g( a.nonconst_mf(), a.nonconst_mf() );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Unsequenced calls to member functions of 'a' (of type 'A'), at least one of which is non-const. [cata-unsequenced-calls]
    int n3 = a.nonconst_mf() | a.nonconst_mf();
    // CHECK-MESSAGES: [[@LINE-1]]:14: warning: Unsequenced calls to member functions of 'a' (of type 'A'), at least one of which is non-const. [cata-unsequenced-calls]
}

void f1()
{
    A a;
    // Calls in args to a 'regular' constructor are dangerous
    B b0( a.nonconst_mf(), a.nonconst_mf() );
    // CHECK-MESSAGES: [[@LINE-1]]:7: warning: Unsequenced calls to member functions of 'a' (of type 'A'), at least one of which is non-const. [cata-unsequenced-calls]

    // ...but using an initializer-list style is OK, because those are
    // sequenced.
    B b1{ a.nonconst_mf(), a.nonconst_mf() };
    B b2 = { a.nonconst_mf(), a.nonconst_mf() };
}

struct B2 {
    int a;
    int b;
};

void f2()
{
    A a;
    // Aggregate initialization is also OK.
    B2 b1{ a.nonconst_mf(), a.nonconst_mf() };
    B2 b2 = { a.nonconst_mf(), a.nonconst_mf() };
    B2{ a.nonconst_mf(), a.nonconst_mf() };
}
