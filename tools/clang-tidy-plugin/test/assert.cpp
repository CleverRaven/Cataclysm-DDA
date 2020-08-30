// RUN: %check_clang_tidy %s cata-assert %t -- -plugins=%cata_plugin --

#include <assert.h>

void f()
{
    assert( true );
    // CHECK-MESSAGES: warning: Prefer cata_assert to assert. [cata-assert]
    // CHECK-FIXES: cata_assert( true );
}
