#include "string_id.h"

// These are centralized in this file because they must appear in one and only one compilation unit,
// and it's problematic to define them in the respective cpp files for each class.
// Very repetitious, so define them with a macro.
#define MAKE_NULL_ID( type, ... ) \
    class type; \
    template<> const string_id<type> &string_id<type>::NULL_ID() { \
        static string_id<type> id = string_id<type>( __VA_ARGS__ ); \
        return id; \
    }

MAKE_NULL_ID( activity_type, "ACT_NULL", -1 )
MAKE_NULL_ID( harvest_list, "null" )
MAKE_NULL_ID( effect_type, "null" )
MAKE_NULL_ID( material_type, "null", 0 )

MAKE_NULL_ID( overmap_land_use_code, "", 0 )
MAKE_NULL_ID( overmap_special, "", 0 )
MAKE_NULL_ID( overmap_connection, "", 0 )
MAKE_NULL_ID( Skill, "none" )
MAKE_NULL_ID( npc_class, "NC_NONE" )
MAKE_NULL_ID( ammunition_type, "NULL" )
MAKE_NULL_ID( vpart_info, "null" )
MAKE_NULL_ID( emit, "null" )
MAKE_NULL_ID( anatomy, "null_anatomy" )
MAKE_NULL_ID( martialart, "style_none" )
MAKE_NULL_ID( recipe, "null" )

#define MAKE_NULL_ID2( type, ... ) \
    struct type; \
    template<> const string_id<type> &string_id<type>::NULL_ID() { \
        static string_id<type> id = string_id<type>( __VA_ARGS__ ); \
        return id; \
    }

MAKE_NULL_ID2( mtype, "mon_null" )
MAKE_NULL_ID2( oter_t, "", 0 )
MAKE_NULL_ID2( oter_type_t, "", 0 )
MAKE_NULL_ID2( ter_t, "t_null", 0 )
MAKE_NULL_ID2( trap, "tr_null" )
MAKE_NULL_ID2( furn_t, "f_null", 0 )
MAKE_NULL_ID2( MonsterGroup, "GROUP_NULL" )
MAKE_NULL_ID2( mission_type, "MISSION_NULL" )
MAKE_NULL_ID2( species_type, "spec_null" )
MAKE_NULL_ID2( mutation_branch )
MAKE_NULL_ID2( requirement_data, "null" )
MAKE_NULL_ID2( body_part_struct, "NUM_BP" )
MAKE_NULL_ID2( bionic_data, "" )
