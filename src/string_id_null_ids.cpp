#include "string_id.h"

// These are centralized in this file because they must appear in one and only one compilation unit,
// and it's problematic to define them in the respective cpp files for each class.
// Very repitious, so define them with a macro.
#define MAKE_NULL_ID( type, ... ) \
class type; \
template<> const string_id<type> &string_id<type>::NULL_ID() {	\
    static string_id<type> id = string_id<type>( __VA_ARGS__ ); \
    return id; \
}

MAKE_NULL_ID( activity_type, "ACT_NULL", -1 );
MAKE_NULL_ID( harvest_list);
MAKE_NULL_ID( effect_type, "null");
MAKE_NULL_ID( furn_t, "f_null", 0 );
MAKE_NULL_ID( material_type, "null", 0 );
MAKE_NULL_ID( MonsterGroup, "GROUP_NULL" );
MAKE_NULL_ID( mtype, "mon_null" );
MAKE_NULL_ID( mutation_branch );
MAKE_NULL_ID( oter_t, "", 0 );
MAKE_NULL_ID( oter_type_t, "", 0 );
MAKE_NULL_ID( requirement_data, "null" );
MAKE_NULL_ID( species_type, "spec_null" );
MAKE_NULL_ID( ter_t, "t_null", 0 );
MAKE_NULL_ID( trap, "tr_null" );

MAKE_NULL_ID( overmap_special, "", 0 );
MAKE_NULL_ID( Skill, "none" );
MAKE_NULL_ID( mission_type, "mission_type" );
MAKE_NULL_ID( npc_class, "NC_NONE" );
MAKE_NULL_ID( ammunition_type, "NULL" );
MAKE_NULL_ID( vpart_info, "null" );
MAKE_NULL_ID( emit, "null" );
