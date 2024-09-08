// RUN: %check_clang_tidy -allow-stdinc %s cata-static-string_id-constants %t -- --load=%cata_plugin -- -isystem %cata_include

#include "type_id.h"

static const bionic_id bio_cloak( "bio_cloak" );
static const activity_id ACT_WASH( "ACT_WASH" );
// CHECK-MESSAGES: warning: string_id declarations should be sorted. [cata-static-string_id-constants]
// CHECK-FIXES: static const activity_id ACT_WASH( "ACT_WASH" );
// CHECK-FIXES: static const bionic_id bio_cloak( "bio_cloak" );
