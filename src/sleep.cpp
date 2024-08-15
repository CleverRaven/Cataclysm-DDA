#include "sleep.h"

#include <string>

#include "character.h"
#include "generic_factory.h"
#include "map.h"
#include "type_id.h"

namespace io
{
template<> std::string enum_to_string<comfort_data::category>( comfort_data::category data )
{
    switch( data ) {
        case comfort_data::category::terrain:
            return "terrain";
        case comfort_data::category::furniture:
            return "furniture";
        case comfort_data::category::field:
            return "field";
        case comfort_data::category::vehicle:
            return "vehicle";
        case comfort_data::category::character:
            return "character";
        case comfort_data::category::trait:
            return "trait";
        case comfort_data::category::last:
            break;
    }
    cata_fatal( "Invalid comfort_data::category" );
}
} // namespace io

static comfort_data human_comfort;

const comfort_data &comfort_data::human()
{
    if( !human_comfort.was_loaded ) {
        human_comfort.comfort = 0;
        human_comfort.add_human_comfort = true;
        human_comfort.add_sleep_aids = true;
        human_comfort.was_loaded = true;
    }
    return human_comfort;
}

bool comfort_data::condition::is_condition_true( const Character &guy, const tripoint &p ) const
{
    bool result = false;
    const map &here = get_map();
    const optional_vpart_position vp = here.veh_at( p );
    switch( category ) {
        case category::terrain:
            if( !id.empty() ) {
                result = ter_id( id ) == here.ter( p );
            } else if( !flag.empty() ) {
                result = here.has_flag_ter( flag, p );
            }
            break;
        case category::furniture:
            if( !id.empty() ) {
                result = furn_id( id ) == here.furn( p );
            } else if( !flag.empty() ) {
                result = here.has_flag_furn( flag, p );
            }
            break;
        case category::field:
            result = intensity >= here.get_field_intensity( p, field_type_id( id ) );
            break;
        case category::vehicle:
            if( vp && !flag.empty() ) {
                result = !!vp.part_with_feature( flag, true );
            } else {
                result = !!vp;
            }
            break;
        case category::character:
            result = guy.has_flag( json_character_flag( flag ) );
            break;
        case category::trait:
            if( active ) {
                result = guy.has_active_mutation( trait_id( id ) );
            } else {
                result = guy.has_trait( trait_id( id ) );
            }
            break;
    }
    return result != invert;
}

void comfort_data::condition::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "type", category );
    switch( category ) {
        case category::terrain:
        case category::furniture:
            optional( jo, false, "id", id );
            optional( jo, false, "flag", flag );
            break;
        case category::field:
            mandatory( jo, false, "id", id );
            optional( jo, false, "intensity", intensity );
            break;
        case category::vehicle:
            optional( jo, false, "flag", flag );
            break;
        case category::character:
            mandatory( jo, false, "flag", flag );
            break;
        case category::trait:
            mandatory( jo, false, "id", id );
            optional( jo, false, "active", active );
            break;
    }
    optional( jo, false, "invert", invert );
}

void comfort_data::message::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "text", text );
    optional( jo, false, "rating", type );
}

bool comfort_data::are_conditions_true( const Character &guy, const tripoint &p ) const
{
    for( const condition &cond : conditions ) {
        if( !cond.is_condition_true( guy, p ) ) {
            return false;
        }
    }
    return true;
}

void comfort_data::deserialize_comfort( const JsonObject &jo, bool was_loaded )
{
    if( !was_loaded ) {
        if( jo.has_int( "comfort" ) ) {
            comfort = jo.get_int( "comfort" );
        } else if( jo.has_string( "comfort" ) ) {
            const std::string str = jo.get_string( "comfort" );
            if( str == "impossible" ) {
                comfort = COMFORT_IMPOSSIBLE;
            } else if( str == "uncomfortable" ) {
                comfort = COMFORT_UNCOMFORTABLE;
            } else if( str == "neutral" ) {
                comfort = COMFORT_NEUTRAL;
            } else if( str == "slightly comfortable" ) {
                comfort = COMFORT_SLIGHTLY_COMFORTABLE;
            } else if( str == "sleep aid" ) {
                comfort = COMFORT_SLEEP_AID;
            } else if( str == "comfortable" ) {
                comfort = COMFORT_COMFORTABLE;
            } else if( str == "very comfortable" ) {
                comfort = COMFORT_VERY_COMFORTABLE;
            } else {
                jo.throw_error( "invalid comfort level" );
            }
        }
    }
}

void comfort_data::deserialize( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "conditions", conditions );
    deserialize_comfort( jo, was_loaded );
    optional( jo, was_loaded, "add_human_comfort", add_human_comfort );
    optional( jo, was_loaded, "add_sleep_aids", add_sleep_aids );
    optional( jo, was_loaded, "msg_try", msg_try );
    optional( jo, was_loaded, "msg_fall", msg_fall );
    was_loaded = true;
}
