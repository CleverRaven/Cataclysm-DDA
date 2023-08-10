// RUN: %check_clang_tidy %s cata-use-mdarray %t -- --load=%cata_plugin -- -isystem %cata_include

int a0[24][24];
// CHECK-MESSAGES: warning: Consider using cata::mdarray rather than a raw 2D C array in declaration of 'a0'. [cata-use-mdarray]

class A1
{
        int a1[24][24];
        // CHECK-MESSAGES: warning: Consider using cata::mdarray rather than a raw 2D C array in declaration of 'a1'. [cata-use-mdarray]
};
