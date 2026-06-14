// RUN: %check_clang_tidy -allow-stdinc %s cata-use-string_view %t -- -load=%cata_plugin -- -isystem %cata_include -fno-delayed-template-parsing

#include <string>
#include <string_view>

// Should change both declaration and definition when both are present, and
// retain their respective parameter names.

void f0( const std::string & );
// CHECK-FIXES: void f0( std::string_view );

void f0( const std::string &s )
// CHECK-MESSAGES: warning: Parameter 's' of 'f0' can be std::string_view. [cata-use-string_view]
// CHECK-FIXES: void f0( const std::string_view s )
{
}

// Should not change declarations without a definition
void f1( const std::string &s );

// Should not change declarations where the string is passed as a string to
// another function
void f2( const std::string &s )
{
    f1( s );
}

// Template functions should only generate a single replacement
template<typename T>
void f3( const std::string &s, const T & )
// CHECK-MESSAGES: warning: Parameter 's' of 'f3' can be std::string_view. [cata-use-string_view]
// CHECK-FIXES: void f3( const std::string_view s, const T & )
{
}

void f3a( const std::string &s )
{
    f3( s, 0 );
    f3( s, true );
}

struct A4 {
    std::string s4;

    A4( const std::string &s ) : s4( s ) {}
};

// Should not change declarations where the string has .c_str() called on it.
void f5( const std::string &s )
{
    s.c_str();
}

// But should change declarations where the string has other member functions
// called on it.
void f6( const std::string &s )
// CHECK-MESSAGES: warning: Parameter 's' of 'f6' can be std::string_view. [cata-use-string_view]
// CHECK-FIXES: void f6( const std::string_view s )
{
    s.size();
}

// Don't change when initializing a const reference member.
struct A7 {
    const std::string &s7;

    A7( const std::string &s ) : s7( s ) {}
};

// Don't change when a template function calling another template function
template<typename T>
void f8( const T &t, const std::string &s )
{
    f3( s, t );
}

// Don't change when passing to a constructor of a class template
template<typename T>
struct A9 {
    A9( const std::string & );
};

template<typename T>
void f9( const std::string &s )
{
    static_cast<void>( A9<T>( s ) );
}

// Don't change when passing to a base class constructor
struct A10b {
    A10b( const std::string & );
};

template<typename T>
struct A10 : A10b {
    A10( const std::string &s, const T & ) : A10b( s ) {}
};

// Don't change when passing to an argument of a member function whose type is
// taken from a class template parameter via a typedef (imitating
// std::unordered_map::find).
template<typename T>
struct A11 {
    using type = T;
    void m11( const type & );
};

struct A11b {
    template<typename T>
    void m11b( const std::string &s, const T & ) {
        a.m11( s );
    }

    A11<std::string> a;
};

// Don't change when returning the value
template<typename T>
T f12( const std::string &s )
{
    return s;
}

// Don't change virtual functions
struct A13 {
    virtual void m13( const std::string & ) {}
};

// Don't change if address taken
const std::string *f14( const std::string &s )
{
    return &s;
}
