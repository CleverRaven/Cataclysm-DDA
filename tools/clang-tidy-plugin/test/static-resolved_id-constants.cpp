// RUN: %check_clang_tidy -allow-stdinc %s cata-static-resolved_id-constants %t -- --load=%cata_plugin -- -isystem %cata_include

#include "type_id.h"

extern resolved_furn_id f_hay;
// CHECK-MESSAGES: warning: Global declaration of 'f_hay' is dangerous because 'resolved_furn_id' is a specialization of resolved_id and it will not update automatically when game data changes.  Consider switching to a string_id. [cata-static-resolved_id-constants]

// No warning for second decl os same variable.
resolved_furn_id f_hay;

static resolved_id<furn_t> f_ball_mach;
// CHECK-MESSAGES: warning: Global declaration of 'f_ball_mach' is dangerous because 'resolved_id<furn_t>' is a specialization of resolved_id and it will not update automatically when game data changes.  Consider switching to a string_id. [cata-static-resolved_id-constants]

namespace foo
{

const resolved_furn_id f_dahlia;
// CHECK-MESSAGES: warning: Global declaration of 'f_dahlia' is dangerous because 'resolved_furn_id' is a specialization of resolved_id and it will not update automatically when game data changes.  Consider switching to a string_id. [cata-static-resolved_id-constants]

} // namespace foo

class A
{
        static resolved_furn_id f_bluebell;
        // CHECK-MESSAGES: warning: Static declaration of 'f_bluebell' is dangerous because 'resolved_furn_id' is a specialization of resolved_id and it will not update automatically when game data changes.  Consider switching to a string_id. [cata-static-resolved_id-constants]

        // No warning for regular class data members
        resolved_furn_id my_furn;
};

void f()
{
    static resolved_furn_id f_floor_canvas;
    // CHECK-MESSAGES: warning: Static declaration of 'f_floor_canvas' is dangerous because 'resolved_furn_id' is a specialization of resolved_id and it will not update automatically when game data changes.  Consider switching to a string_id. [cata-static-resolved_id-constants]

    // No warning for regular local variables
    resolved_furn_id f;

    // No warning for other types
    static int i;
}
