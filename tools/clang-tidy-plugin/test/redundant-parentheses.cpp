// RUN: %check_clang_tidy %s cata-redundant-parentheses %t -- -plugins=%cata_plugin -- -isystem %cata_include

#define MACRO(a, b) ( g1( ( a ), ( b ) ) )

int g0();

int f0()
{
    int n0;
    ( n0 );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Redundant parentheses. [cata-redundant-parentheses]
    // CHECK-FIXES: n0;

    ( 0 );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Redundant parentheses. [cata-redundant-parentheses]
    // CHECK-FIXES: 0;

    ( "Hello" );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Redundant parentheses. [cata-redundant-parentheses]
    // CHECK-FIXES: "Hello";

    ( g0() );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Redundant parentheses. [cata-redundant-parentheses]
    // CHECK-FIXES: g0();

    ( ( n0 + n0 ) ) + n0;
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Redundant parentheses. [cata-redundant-parentheses]
    // CHECK-FIXES: ( n0 + n0 ) + n0;

    if( ( n0 + n0 ) ) {
        // CHECK-MESSAGES: [[@LINE-1]]:9: warning: Redundant parentheses. [cata-redundant-parentheses]
        // CHECK-FIXES: if( n0 + n0 ) {
    }

    int n1 = ( n0 + n0 );
    // CHECK-MESSAGES: [[@LINE-1]]:14: warning: Redundant parentheses. [cata-redundant-parentheses]
    // CHECK-FIXES: int n1 = n0 + n0;

    return ( n0 + n0 );
    // CHECK-MESSAGES: [[@LINE-1]]:12: warning: Redundant parentheses. [cata-redundant-parentheses]
    // CHECK-FIXES: return n0 + n0;
}

int g1( int, int );

void f1( int a, int b )
{
    // Some things that should *not* trigger a warning
    MACRO( a, b );

    sizeof( a );

    if( ( a = b ) ) {
    }
}
