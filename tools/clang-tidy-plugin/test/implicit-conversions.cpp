// RUN: %check_clang_tidy %s cata-implicit-conversions %t -- -plugins=%cata_plugin --

struct A0 {
    A0( int );
    // CHECK-MESSAGES: warning: Constructor for 'A0' makes it implicitly convertible from 'int'.  Consider marking it explicit. [cata-implicit-conversions]
    // CHECK-FIXES: explicit A0( int );
};

A0::A0( int ) {}

struct A1 {
    A1( int, int = 0 );
    // CHECK-MESSAGES: warning: Constructor for 'A1' makes it implicitly convertible from 'int'.  Consider marking it explicit. [cata-implicit-conversions]
    // CHECK-FIXES: explicit A1( int, int = 0 );
};

struct A2 {
    explicit A2( int );
};

struct A3 {
    explicit A3( int, int = 0 );
};

struct A4 {
    A4( int, int );
};

struct A5 {
    A5( const A5 & );
    A5( A5 && );
};

template<typename T>
struct A6 {
    A6( const A6<T> & );
    A6( T );
    // CHECK-MESSAGES: warning: Constructor for 'A6<T>' makes it implicitly convertible from 'T'.  Consider marking it explicit. [cata-implicit-conversions]
    // CHECK-FIXES: explicit A6( T );
};

void f6()
{
    A6<int>( 0 );
}

struct A7 {
    A7( int ) = delete;
};

template<typename T>
struct A8 {
    A8() = default;

    template<typename U>
    A8( U );
    // CHECK-MESSAGES: warning: Constructor for 'A8<T>' makes it implicitly convertible from 'U'.  Consider marking it explicit. [cata-implicit-conversions]
    // CHECK-FIXES: explicit A8( U );
};

void f8()
{
    A8<int> a8;
}

struct A9 : A0 {
    using A0::A0;
};

void f9()
{
    A9 a9( 0 );
}
