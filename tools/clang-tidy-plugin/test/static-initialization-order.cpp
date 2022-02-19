// RUN: %check_clang_tidy %s cata-static-initialization-order %t -- -plugins=%cata_plugin --

extern const int i0;
extern const int i0b;

struct A0 {
    static const int Ai0;
};

namespace N0
{
extern const int i0;
};

static const int i1 = i0;
// CHECK-MESSAGES: warning: Static initialization of 'i1' depends on extern 'i0', which may be initialized afterwards. [cata-static-initialization-order]

void f2()
{
    // statics inside functions are fine
    static const int i2 = i0;
}

const int i3 = i0;
// CHECK-MESSAGES: warning: Static initialization of 'i3' depends on extern 'i0', which may be initialized afterwards. [cata-static-initialization-order]

int i4 = i0;
// CHECK-MESSAGES: warning: Static initialization of 'i4' depends on extern 'i0', which may be initialized afterwards. [cata-static-initialization-order]

namespace N5
{

int i5 = i0;
// CHECK-MESSAGES: warning: Static initialization of 'i5' depends on extern 'i0', which may be initialized afterwards. [cata-static-initialization-order]

}

int i6 = A0::Ai0;
// CHECK-MESSAGES: warning: Static initialization of 'i6' depends on extern 'Ai0', which may be initialized afterwards. [cata-static-initialization-order]

int i7 = N0::i0;
// CHECK-MESSAGES: warning: Static initialization of 'i7' depends on extern 'i0', which may be initialized afterwards. [cata-static-initialization-order]

int i8 = i1;

int i9 = i0b;
// CHECK-MESSAGES: warning: Static initialization of 'i9' depends on extern 'i0b', which is initialized afterwards. [cata-static-initialization-order]

const int i0b = 0;

// But initializing after i0b is fine.
int i10 = i0b;

auto f11 = []()
{
    // Reference inside a lambda used in initializer is fine
    return i0;
};
