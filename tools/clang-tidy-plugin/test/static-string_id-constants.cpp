// RUN: %check_clang_tidy -allow-stdinc %s cata-static-string_id-constants %t -- --load=%cata_plugin -- -isystem %cata_include

#include "type_id.h"

static const construction_category_id construction_cat_FILTER( "FILTER" );
// CHECK-MESSAGES: warning: Declaration of string_id 'construction_cat_FILTER' should be named 'construction_category_FILTER'. [cata-static-string_id-constants]
// CHECK-FIXES: static const construction_category_id construction_category_FILTER( "FILTER" );
const efftype_id effect_sleep( "sleep" );
// CHECK-MESSAGES: warning: Global declaration of 'effect_sleep' should be static. [cata-static-string_id-constants]
// CHECK-FIXES: static const efftype_id effect_sleep( "sleep" );

// Don't suggest static if there is a prior extern decl
extern const efftype_id effect_hallu;
const efftype_id effect_hallu( "hallu" );

static scenttype_id scent_human( "human" );
// CHECK-MESSAGES: warning: Global declaration of 'scent_human' should be const. [cata-static-string_id-constants]
// CHECK-FIXES: static const scenttype_id scent_human( "human" );

// Verify than non-alphanumerics are replaced by underscores
static const mongroup_id MI_GO_CAMP_OM( "GROUP_MI-GO_CAMP_OM" );
// CHECK-MESSAGES: warning: Declaration of string_id 'MI_GO_CAMP_OM' should be named 'GROUP_MI_GO_CAMP_OM'. [cata-static-string_id-constants]
// CHECK-FIXES: static const mongroup_id GROUP_MI_GO_CAMP_OM( "GROUP_MI-GO_CAMP_OM" );

void f()
{
    efftype_id effect( "sleep" );
    // Uses of local variables should not be renamed
    ( void )effect;
    ( void )construction_cat_FILTER;
    // CHECK-MESSAGES: warning: Use of string_id 'construction_cat_FILTER' should be named 'construction_category_FILTER'. [cata-static-string_id-constants]
    // CHECK-FIXES: ( void )construction_category_FILTER;
}

class A
{
        static efftype_id effect;
};

efftype_id A::effect( "sleep" );
