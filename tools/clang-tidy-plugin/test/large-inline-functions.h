// RUN: %check_clang_tidy %s cata-large-inline-function %t -- --load=%cata_plugin --

inline void f0()
// CHECK-MESSAGES: warning: Function 'f0' declared inline but contains more than 2 statements. Consider moving the definition to a cpp file. [cata-large-inline-function]
{
    int a;
    int b;
    int c;
}

struct A1 {
    void f1()
    // CHECK-MESSAGES: warning: Function 'f1' declared inline but contains more than 2 statements. Consider moving the definition to a cpp file. [cata-large-inline-function]
    {
        int a;
        int b;
        int c;
    }
};

// Don't warn on a shorter function
inline void f2()
{
    int b;
    int c;
}

// Don't warn on a shorter function
inline void f3()
{
    int b = 0 + 0 + 0 + 0;
    int c;
}

// Don't warn on templates
template<int>
inline void f4()
{
    int a;
    int b;
    int c;
}

// Don't warn on nested functions
inline void f5()
// CHECK-MESSAGES: warning: Function 'f5' declared inline but contains more than 2 statements. Consider moving the definition to a cpp file. [cata-large-inline-function]
{
    auto l = [&]() {
        int a;
        int b;
        int c;
    };
    int b;
}

// Don't warn on class templates
template<int>
struct A6 {
    void f6() {
        int a;
        int b;
        int c;
    }

    struct A7 {
        void f7() {
            int a;
            int b;
            int c;
        }
    };
};
