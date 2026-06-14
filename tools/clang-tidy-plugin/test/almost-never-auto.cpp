// RUN: %check_clang_tidy %s cata-almost-never-auto %t -- --load=%cata_plugin -- -fno-delayed-template-parsing

using int_alias = int;
int_alias return_int_alias();

int &f00();
const int &f01();

struct A0 {};

struct A1 {
    A0 a1;
};

void f0()
{
    auto i0 = 0;
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i0'. [cata-almost-never-auto]
    // CHECK-FIXES: int i0 = 0;

    const auto i1 = 0;
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i1'. [cata-almost-never-auto]
    // CHECK-FIXES: const int i1 = 0;

    // TODO: this variety doesn't work yet
    auto *i2 = &i1;
    // CH/ECK-MESSAGES: warning: Avoid auto in declaration of 'i2'. [cata-almost-never-auto]
    // CH/ECK-FIXES: const int *i2 = &i1;

    int i3_array[10];
    for( auto i3 : i3_array ) {
        // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i3'. [cata-almost-never-auto]
        // CHECK-FIXES: for( int i3 : i3_array ) {
        // nothing
    }

    constexpr auto i4 = 0;
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i4'. [cata-almost-never-auto]
    // CHECK-FIXES: constexpr int i4 = 0;

    static auto i5 = 0;
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i5'. [cata-almost-never-auto]
    // CHECK-FIXES: static int i5 = 0;

    auto i6 = return_int_alias();
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i6'. [cata-almost-never-auto]
    // CHECK-FIXES: int_alias i6 = return_int_alias();

    const auto i7 = &i0;
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i7'. [cata-almost-never-auto]
    // CHECK-FIXES: int *const i7 = &i0;

    const auto i8 = &i1;
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i8'. [cata-almost-never-auto]
    // CHECK-FIXES: const int *const i8 = &i1;

    auto const i9 = &i0;
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i9'. [cata-almost-never-auto]
    // CHECK-FIXES: int *const i9 = &i0;

    auto const i10 = &i1;
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i10'. [cata-almost-never-auto]
    // CHECK-FIXES: const int *const i10 = &i1;

    auto const i11 = 0;
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i11'. [cata-almost-never-auto]
    // CHECK-FIXES: const int i11 = 0;

    auto &i12 = i0;
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i12'. [cata-almost-never-auto]
    // CHECK-FIXES: int &i12 = i0;

    auto const &i13 = i0;
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i13'. [cata-almost-never-auto]
    // CHECK-FIXES: const int &i13 = i0;

    auto &i14 = f00();
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i14'. [cata-almost-never-auto]
    // CHECK-FIXES: int &i14 = f00();

    const auto &i15 = f00();
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i15'. [cata-almost-never-auto]
    // CHECK-FIXES: const int &i15 = f00();

    auto &i16 = f01();
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i16'. [cata-almost-never-auto]
    // CHECK-FIXES: const int &i16 = f01();

    const auto &i17 = f01();
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i17'. [cata-almost-never-auto]
    // CHECK-FIXES: const int &i17 = f01();

    const A1 a1;
    const auto &a18 = a1.a1;
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'a18'. [cata-almost-never-auto]
    // CHECK-FIXES: const A0 &a18 = a1.a1;

    auto &&i19 = 0;
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i19'. [cata-almost-never-auto]
    // CHECK-FIXES: int &&i19 = 0;

    auto const &&i20 = 0;
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i20'. [cata-almost-never-auto]
    // CHECK-FIXES: const int &&i20 = 0;

    int j0 = 0;

    int j1_array[10];
    for( int j1 : j1_array ) {
        // nothing
    }

    auto j2 = []() {};

#define DEFINE_VAR( expr ) auto j3 = expr;
    DEFINE_VAR( 0 );

    // Don't replace auto when an array type
    int j4[2][2];
    auto &j5 = j4;
    auto &j6 = j4[0];

    // Don't try to add a type to lambda captures
    auto lam = [j7 = j0] { return j7; };
}

template<typename T>
void f1( T t )
{
    auto i0 = 0;
    // CHECK-MESSAGES: warning: Avoid auto in declaration of 'i0'. [cata-almost-never-auto]
    // CHECK-FIXES: int i0 = 0;

    // Shouldn't replace the following, which only has a known type when f is
    // instantiated
    auto i1 = t.begin();
}

void g1()
{
    struct S {
        int begin();
    };
    S s;
    f1( s );
}

template<typename Container>
void f2( Container &c0 )
{
    // Shouldn't replace auto with typename Container::value_type here
    for( const auto &v0 : c0 ) {
        static_cast<void>( v0 );
    }

    int c1[] = { 0 };
    for( const auto &v1 : c1 ) {
        // CHECK-MESSAGES: warning: Avoid auto in declaration of 'v1'. [cata-almost-never-auto]
        // CHECK-FIXES: for( const int &v1 : c1 ) {
        static_cast<void>( v1 );
    }
}
