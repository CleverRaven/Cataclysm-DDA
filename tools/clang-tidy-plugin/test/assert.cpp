// RUN: %check_clang_tidy -allow-stdinc %s cata-assert %t -- --load=%cata_plugin -- -isystem %cata_include

#include <assert.h>
#include <stdlib.h>
#include <cstdlib>
#include <cata_assert.h>

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
