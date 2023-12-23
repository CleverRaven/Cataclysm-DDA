// RUN: %check_clang_tidy -allow-stdinc %s cata-static-string_id-constants %t -- --load=%cata_plugin -- -isystem %cata_include

#include "make_static.h"
#include "type_id.h"

static const activity_id ACT_WASH( "ACT_WASH" );
// CHECK-FIXES: static const activity_id ACT_ZZZ( "ACT_ZZZ" );
static const bionic_id bio_cloak( "bio_cloak" );

void g0( const activity_id & );
void g1( const activity_id & );
void g1( const bionic_id & );

void f0()
{
    g0( activity_id( "ACT_ZZZ" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:9: warning: Temporary string_id instances with fixed content should be promoted to a static instance at global scope. [cata-static-string_id-constants]
    // CHECK-FIXES: g0( ACT_ZZZ );
    g1( activity_id( "ACT_ZZZ" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:9: warning: Temporary string_id instances with fixed content should be promoted to a static instance at global scope. [cata-static-string_id-constants]
    // CHECK-FIXES: g1( ACT_ZZZ );
    g0( activity_id( "ACT_WASH" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:9: warning: Temporary string_id instances with fixed content should be promoted to a static instance at global scope. [cata-static-string_id-constants]
    // CHECK-FIXES: g0( ACT_WASH );

    // Don't mess with values inside STATIC macro calls
    g1( STATIC( activity_id( "ACT_WASH" ) ) );
    // Don't mess with values whose canonical name would be empty
    g1( bionic_id( "" ) );
}
