// RUN: %check_clang_tidy %s cata-assert %t -- -plugins=%cata_plugin --

// check_clang_tidy uses -nostdinc++, so we add dummy declarations of used values here
#define assert( expr ) static_cast<void>( expr )
namespace std
{
void abort();
}
using std::abort;

#define cata_assert(expression) assert(expression)

void f0()
{
    assert( true );
    // CHECK-MESSAGES: warning: Prefer cata_assert to assert. [cata-assert]
    // CHECK-FIXES: cata_assert( true );
    cata_assert( 1 );
}

void f1()
{
    cata_assert( false );
    // CHECK-MESSAGES: warning: Prefer cata_fatal to cata_assert( false ). [cata-assert]
    cata_assert( !"msg" );
    // CHECK-MESSAGES: warning: Prefer cata_fatal to cata_assert( !"…" ). [cata-assert]
    // CHECK-FIXES: cata_fatal( "msg" );
    abort();
    // CHECK-MESSAGES: warning: Prefer cata_fatal to abort(). [cata-assert]
    std::abort();
    // CHECK-MESSAGES: warning: Prefer cata_fatal to abort(). [cata-assert]
}
