// RUN: %check_clang_tidy %s cata-no-long %t -- -plugins=%cata_plugin --

// check_clang_tidy uses -nostdinc++, so we add dummy declarations of used values here
// They're probably not the correct values, but it doesn't matter.
#define LONG_MIN -2147483647
#define LONG_MAX 2147483647
#define ULONG_MAX 4294967295
using int64_t = long long;
using uint64_t = unsigned long long;

long i1;
// CHECK-MESSAGES: warning: Variable 'i1' declared as 'long'. Prefer int or int64_t to long. [cata-no-long]

unsigned long i2;
// CHECK-MESSAGES: warning: Variable 'i2' declared as 'unsigned long'. Prefer unsigned int, size_t, or uint64_t to unsigned long. [cata-no-long]

const long i3 = 0;
// CHECK-MESSAGES: warning: Variable 'i3' declared as 'const long'. Prefer int or int64_t to long. [cata-no-long]

long &i4 = i1;
// CHECK-MESSAGES: warning: Variable 'i4' declared as 'long &'. Prefer int or int64_t to long. [cata-no-long]

long &&i5 = 0L;
// CHECK-MESSAGES: warning: Variable 'i5' declared as 'long &&'. Prefer int or int64_t to long. [cata-no-long]

int64_t i6;
uint64_t i7;

auto i8 = int64_t {};
auto &i9 = i1;
const auto &i10 = i1;
//auto&& i11 = i1; // Shouldn't cause a warning but I can't fix it

void f1( long e );
// CHECK-MESSAGES: warning: Variable 'e' declared as 'long'. Prefer int or int64_t to long. [cata-no-long]

long f2();
// CHECK-MESSAGES: warning: Function 'f2' declared as returning 'long'. Prefer int or int64_t to long. [cata-no-long]

int64_t f3();
auto f4() -> decltype( 0L );

int c0 = static_cast<long>( 0 );
// CHECK-MESSAGES: warning: Static cast to 'long'.  Prefer int or int64_t to long. [cata-no-long]
int c1 = static_cast<int64_t>( 0 );

template<typename T>
T g0( T gp0, long gp1 = 0 )
{
    // CHECK-MESSAGES: warning: Variable 'gp1' declared as 'long'.  Prefer int or int64_t to long. [cata-no-long]
    long gi0;
    // CHECK-MESSAGES: warning: Variable 'gi0' declared as 'long'.  Prefer int or int64_t to long. [cata-no-long]
    T gi1;
}

void h()
{
    g0<long>( 0, 0 );
    // Would like to report an error here for the template argument, but have
    // not found a way to do so.

    g0( LONG_MIN );
    // CHECK-MESSAGES: warning: Use of long-specific macro LONG_MIN [cata-no-long]
    g0( LONG_MAX );
    // CHECK-MESSAGES: warning: Use of long-specific macro LONG_MAX [cata-no-long]
    g0( ULONG_MAX );
    // CHECK-MESSAGES: warning: Use of long-specific macro ULONG_MAX [cata-no-long]
}

template<typename T>
struct A {
    A();
    A( const A & );
    A( A && );
    A &operator=( const A & );
    A &operator=( A && );
    T Af0();
    long Af1();
    // CHECK-MESSAGES: warning: Function 'Af1' declared as returning 'long'.  Prefer int or int64_t to long. [cata-no-long]
    T Af2( long Af2i );
    // CHECK-MESSAGES: warning: Variable 'Af2i' declared as 'long'.  Prefer int or int64_t to long. [cata-no-long]
};

A<long> a;

auto l0 = []( int64_t a )
{
    return a;
};

template<int size>
struct B {
    A<int> BA[size][size];
};

void Bf()
{
    B<12> b0;
    B<12> b1;
    // This exercises an obscure corner case where a defaulted operator= will
    // cause the compiler to generate code involving an unsigned long variable.
    b1 = static_cast < B<12> && >( b0 );
}

template<typename T>
long g1( T g1p0 );
// CHECK-MESSAGES: warning: Function 'g1' declared as returning 'long'. Prefer int or int64_t to long. [cata-no-long]

template<typename T>
struct A1 {
    void f( T a1a ) {
    }
    T a1b;
};

template<typename T>
A1<T> f5()
{
    return {};
}

// Would be nice to get warnings here but haven't found a way to do so.
extern template A1<long> f5<long>();
template A1<long> f5<long>();
