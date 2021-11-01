// RUN: %check_clang_tidy %s cata-almost-never-auto %t -- -plugins=%cata_plugin --

using int_alias = int;
int_alias return_int_alias();

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
    // CHECK-FIXES: int const i11 = 0;

    int j0 = 0;

    int j1_array[10];
    for( int j1 : j1_array ) {
        // nothing
    }

    auto j2 = []() {};

#define DEFINE_VAR( expr ) auto j3 = expr;
    DEFINE_VAR( 0 );
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
