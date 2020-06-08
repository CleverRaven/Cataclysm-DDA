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

struct construction_category;
using construction_category_id = string_id<construction_category>;

class effect_type;
using efftype_id = string_id<effect_type>;

struct MonsterGroup;
using mongroup_id = string_id<MonsterGroup>;

class scent_type;
using scenttype_id = string_id<scent_type>;

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

activity_id f()
{
    efftype_id effect( "sleep" );
    // Uses of local variables should not be renamed
    ( void )effect;
    const bionic_id bio_cloak( "bio_cloak" );
    // CH/ECK-MESSAGES: warning: Construction of 'const bionic_id' (aka 'const string_id<bionic_data>') from string literal should be global static constant. [cata-static-string_id-constants]
    ( void )construction_cat_FILTER;
    // CHECK-MESSAGES: warning: Use of string_id 'construction_cat_FILTER' should be named 'construction_category_FILTER'. [cata-static-string_id-constants]
    // CHECK-FIXES: ( void )construction_category_FILTER;
    return activity_id( "ACT_WASH" );
    // CH/ECK-MESSAGES: warning: Construction of 'activity_id' (aka 'string_id<activity_type>') from string literal should be global static constant. [cata-static-string_id-constants]
}
