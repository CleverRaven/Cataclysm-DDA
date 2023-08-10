// RUN: %check_clang_tidy -allow-stdinc %s cata-static-string_id-constants %t -- --load=%cata_plugin -- -isystem %cata_include

#include "type_id.h"

static const bionic_id bio_cloak( "bio_cloak" );

static const bool b = true;

static const activity_id ACT_WASH( "ACT_WASH" );
static const bionic_id bio_zzz( "bio_zzz" );
// CHECK-MESSAGES: warning: string_id declarations should be together. [cata-static-string_id-constants]
// CHECK-FIXES: static const activity_id ACT_WASH( "ACT_WASH" );
// CHECK-FIXES: static const bionic_id bio_cloak( "bio_cloak" );
// CHECK-FIXES: static const bionic_id bio_zzz( "bio_zzz" );
// CHECK-FIXES: static const bool b = true;
