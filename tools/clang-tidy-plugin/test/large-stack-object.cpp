// RUN: %check_clang_tidy %s cata-large-stack-object %t -- --load=%cata_plugin --

void f0()
{
    char a0[200000];
    // CHECK-MESSAGES: warning: Variable 'a0' consumes 195KiB of stack space.  Putting such large objects on the stack risks stack overflow.  Please allocate it on the heap instead. [cata-large-stack-object]
}

class A1
{
        char a[200000];
};

void f1()
{
    A1 a1;
    // CHECK-MESSAGES: warning: Variable 'a1' consumes 195KiB of stack space.  Putting such large objects on the stack risks stack overflow.  Please allocate it on the heap instead. [cata-large-stack-object]
}

void f2()
{
    auto a2 = A1();
    // CHECK-MESSAGES: warning: Variable 'a2' consumes 195KiB of stack space.  Putting such large objects on the stack risks stack overflow.  Please allocate it on the heap instead. [cata-large-stack-object]
}
