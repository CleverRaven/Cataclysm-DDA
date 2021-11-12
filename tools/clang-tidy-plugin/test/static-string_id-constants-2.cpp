// RUN: %check_clang_tidy %s cata-static-string_id-constants %t -- -plugins=%cata_plugin -- -isystem %cata_include

template<typename T>
class string_id
{
    public:
        template<typename S>
        explicit string_id( S &&, int cid = -1 ) {
        }
};

class activity_type;
using activity_id = string_id<activity_type>;

struct bionic_data;
using bionic_id = string_id<bionic_data>;

static const bionic_id bio_cloak( "bio_cloak" );
static const activity_id ACT_WASH( "ACT_WASH" );
// CHECK-MESSAGES: warning: string_id declarations should be sorted. [cata-static-string_id-constants]
// CHECK-FIXES: static const activity_id ACT_WASH( "ACT_WASH" );
// CHECK-FIXES: static const bionic_id bio_cloak( "bio_cloak" );
