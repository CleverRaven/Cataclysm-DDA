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

MAKE_NULL_ID( activity_type, "ACT_NULL" )
MAKE_NULL_ID( harvest_list, "null" )
MAKE_NULL_ID( effect_type, "null" )
MAKE_NULL_ID( material_type, "null" )

MAKE_NULL_ID( overmap_land_use_code, "" )
MAKE_NULL_ID( overmap_special, "" )
MAKE_NULL_ID( overmap_connection, "" )
MAKE_NULL_ID( map_extra, "" )
MAKE_NULL_ID( Skill, "none" )
MAKE_NULL_ID( SkillDisplayType, "none" )
MAKE_NULL_ID( npc_class, "NC_NONE" )
MAKE_NULL_ID( faction, "NULL" )
MAKE_NULL_ID( ammunition_type, "NULL" )
MAKE_NULL_ID( vpart_info, "null" )
MAKE_NULL_ID( emit, "null" )
MAKE_NULL_ID( anatomy, "null_anatomy" )
MAKE_NULL_ID( martialart, "style_none" )
MAKE_NULL_ID( recipe, "null" )
MAKE_NULL_ID( translation, "null" )
MAKE_NULL_ID( Item_group, "" )

#define MAKE_NULL_ID2( type, ... ) \
    struct type; \
    template<> const string_id<type> &string_id<type>::NULL_ID() { \
        static string_id<type> id = string_id<type>( __VA_ARGS__ ); \
        return id; \
    }

MAKE_NULL_ID2( mtype, "mon_null" )
MAKE_NULL_ID2( oter_t, "" )
MAKE_NULL_ID2( oter_type_t, "" )
MAKE_NULL_ID2( ter_t, "t_null" )
MAKE_NULL_ID2( trap, "tr_null" )
MAKE_NULL_ID2( construction_category, "NULL" )
MAKE_NULL_ID2( ammo_effect, "AE_NULL" )
MAKE_NULL_ID2( field_type, "fd_null" )
MAKE_NULL_ID2( furn_t, "f_null" )
MAKE_NULL_ID2( MonsterGroup, "GROUP_NULL" )
MAKE_NULL_ID2( mission_type, "MISSION_NULL" )
MAKE_NULL_ID2( species_type, "spec_null" )
MAKE_NULL_ID2( mutation_branch, "" )
MAKE_NULL_ID2( requirement_data, "null" )
MAKE_NULL_ID2( body_part_type, "NUM_BP" )
MAKE_NULL_ID2( bionic_data, "" )
MAKE_NULL_ID2( construction, "constr_null" )
MAKE_NULL_ID2( vehicle_prototype, "null" )
