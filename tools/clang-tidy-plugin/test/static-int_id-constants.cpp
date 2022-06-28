// RUN: %check_clang_tidy %s cata-static-int_id-constants %t -- -plugins=%cata_plugin -- -isystem %cata_include

template<typename T>
class int_id
{
    public:
        int_id();
        explicit int_id( int );
};

struct furn_t;
using furn_id = int_id<furn_t>;

extern furn_id f_hay;
// CHECK-MESSAGES: warning: Global declaration of 'f_hay' is dangerous because 'furn_id' is a specialization of int_id and it will not update automatically when game data changes.  Consider switching to a string_id. [cata-static-int_id-constants]

// No warning for second decl os same variable.
furn_id f_hay;

static int_id<furn_t> f_ball_mach;
// CHECK-MESSAGES: warning: Global declaration of 'f_ball_mach' is dangerous because 'int_id<furn_t>' is a specialization of int_id and it will not update automatically when game data changes.  Consider switching to a string_id. [cata-static-int_id-constants]

namespace foo
{

const furn_id f_dahlia;
// CHECK-MESSAGES: warning: Global declaration of 'f_dahlia' is dangerous because 'furn_id' is a specialization of int_id and it will not update automatically when game data changes.  Consider switching to a string_id. [cata-static-int_id-constants]

} // namespace foo

class A
{
        static furn_id f_bluebell;
        // CHECK-MESSAGES: warning: Static declaration of 'f_bluebell' is dangerous because 'furn_id' is a specialization of int_id and it will not update automatically when game data changes.  Consider switching to a string_id. [cata-static-int_id-constants]

        // No warning for regular class data members
        furn_id my_furn;
};

void f()
{
    static furn_id f_floor_canvas;
    // CHECK-MESSAGES: warning: Static declaration of 'f_floor_canvas' is dangerous because 'furn_id' is a specialization of int_id and it will not update automatically when game data changes.  Consider switching to a string_id. [cata-static-int_id-constants]

    // No warning for regular local variables
    furn_id f;

    // No warning for other types
    static int i;
}
