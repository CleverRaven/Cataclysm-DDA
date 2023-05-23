// RUN: %check_clang_tidy %s cata-avoid-alternative-tokens %t -- --load=%cata_plugin --

void f0()
{
    bool i0 = false and false;
    // CHECK-MESSAGES: warning: Avoid alternative token 'and'; prefer '&&'. [cata-avoid-alternative-tokens]
    // CHECK-FIXES: bool i0 = false && false;

    bool i1 = false bitor false;
    // CHECK-MESSAGES: warning: Avoid alternative token 'bitor'; prefer '|'. [cata-avoid-alternative-tokens]
    // CHECK-FIXES: bool i1 = false | false;

    bool i2 = false or false;
    // CHECK-MESSAGES: warning: Avoid alternative token 'or'; prefer '||'. [cata-avoid-alternative-tokens]
    // CHECK-FIXES: bool i2 = false || false;

    bool i3 = false xor false;
    // CHECK-MESSAGES: warning: Avoid alternative token 'xor'; prefer '^'. [cata-avoid-alternative-tokens]
    // CHECK-FIXES: bool i3 = false ^ false;

    bool i4 = false bitand false;
    // CHECK-MESSAGES: warning: Avoid alternative token 'bitand'; prefer '&'. [cata-avoid-alternative-tokens]
    // CHECK-FIXES: bool i4 = false & false;

    bool i5 = false not_eq false;
    // CHECK-MESSAGES: warning: Avoid alternative token 'not_eq'; prefer '!='. [cata-avoid-alternative-tokens]
    // CHECK-FIXES: bool i5 = false != false;

    int i6 = compl 0;
    // CHECK-MESSAGES: warning: Avoid alternative token 'compl'; prefer '~'. [cata-avoid-alternative-tokens]
    // CHECK-FIXES: int i6 = ~ 0;

    bool i7 = not false;
    // CHECK-MESSAGES: warning: Avoid alternative token 'not'; prefer '!'. [cata-avoid-alternative-tokens]
    // CHECK-FIXES: bool i7 = ! false;

    bool i8 = false;
    i8 and_eq false;
    // CHECK-MESSAGES: warning: Avoid alternative token 'and_eq'; prefer '&='. [cata-avoid-alternative-tokens]
    // CHECK-FIXES: i8 &= false;

    bool i9 = false;
    i9 or_eq false;
    // CHECK-MESSAGES: warning: Avoid alternative token 'or_eq'; prefer '|='. [cata-avoid-alternative-tokens]
    // CHECK-FIXES: i9 |= false;

    bool i10 = false;
    i10 xor_eq false;
    // CHECK-MESSAGES: warning: Avoid alternative token 'xor_eq'; prefer '^='. [cata-avoid-alternative-tokens]
    // CHECK-FIXES: i10 ^= false;

    // Ensure we don't accidentally try to replace the compiler-generated token
    // inside a foreach loop
    int is11[] = { 0 };
    for( int i : is11 ) {}

    // Ensure we don't get confused by macros
#define AND(x, y) x && y
    bool i12 = AND( false, false );
}
