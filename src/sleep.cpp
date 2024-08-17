#include "sleep.h"

#include <iterator>
#include <optional>
#include <string>

#include "character.h"
#include "flag.h"
#include "game.h"
#include "generic_factory.h"
#include "item.h"
#include "map.h"
#include "trap.h"
#include "type_id.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"

namespace io
{
template<> std::string enum_to_string<comfort_data::category>( comfort_data::category data )
{
    switch( data ) {
        case comfort_data::category::terrain:
            return "terrain";
        case comfort_data::category::furniture:
            return "furniture";
        case comfort_data::category::trap:
            return "trap";
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
        human_comfort.base_comfort = 0;
        human_comfort.add_human_comfort = true;
        human_comfort.add_sleep_aids = true;
        human_comfort.was_loaded = true;
    }
    return human_comfort;
}

int comfort_data::human_comfort_at( const tripoint &p )
{
    const map &here = get_map();
    const optional_vpart_position vp = here.veh_at( p );
    const furn_id furn = here.furn( p );
    const trap &trap = here.tr_at( p );

    if( vp ) {
        const std::optional<vpart_reference> board = vp.part_with_feature( "BOARDABLE", true );
        return board ? board->info().comfort : -here.move_cost( p );
    } else if( furn != furn_str_id::NULL_ID() ) {
        return furn->comfort;
    } else if( !trap.is_null() ) {
        return trap.comfort;
    } else {
        return here.ter( p )->comfort - here.move_cost( p );
    }
}

bool comfort_data::try_get_sleep_aid_at( const tripoint &p, item &result )
{
    map &here = get_map();
    const optional_vpart_position vp = here.veh_at( p );
    const map_stack items = here.i_at( p );
    item_stack::const_iterator begin = items.begin();
    item_stack::const_iterator end = items.end();

    if( vp ) {
        if( const std::optional<vpart_reference> cargo = vp.cargo() ) {
            const vehicle_stack vs = cargo->items();
            begin = vs.begin();
            end = vs.end();
        }
    }

    for( item_stack::const_iterator item_it = begin; item_it != end; item_it++ ) {
        if( item_it->has_flag( flag_SLEEP_AID ) ) {
            result = *item_it;
            return true;
        } else if( item_it->has_flag( flag_SLEEP_AID_CONTAINER ) ) {
            if( item_it->num_item_stacks() > 1 ) {
                continue;
            }
            for( const item *it : item_it->all_items_top() ) {
                if( it->has_flag( flag_SLEEP_AID ) ) {
                    result = *item_it;
                    return true;
                }
            }
        }
    }
    return false;
}

bool comfort_data::condition::is_condition_true( const Character &guy, const tripoint &p ) const
{
    bool result = false;
    const map &here = get_map();
    const optional_vpart_position vp = here.veh_at( p );
    const trap &trap = here.tr_at( p );
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
        case category::trap:
            result = trap_id( id ) == trap.id;
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
        case category::trap:
            mandatory( jo, false, "id", id );
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

bool comfort_data::human_or_impossible() const
{
    return this == &human_comfort || base_comfort == COMFORT_IMPOSSIBLE;
}

bool comfort_data::are_conditions_true( const Character &guy, const tripoint &p ) const
{
    for( const condition &cond : conditions ) {
        const bool passed = cond.is_condition_true( guy, p );
        if( conditions_or == passed ) {
            return conditions_or;
        }
    }
    return !conditions_or;
}

comfort_data::response comfort_data::get_comfort_at( const tripoint &p ) const
{
    response result;
    result.data = this;
    result.comfort = base_comfort;
    if( add_human_comfort ) {
        result.comfort += human_comfort_at( p );
    }
    item sleep_aid;
    if( add_sleep_aids && try_get_sleep_aid_at( p, sleep_aid ) ) {
        result.comfort += COMFORT_SLEEP_AID;
        result.sleep_aid = sleep_aid.tname();
    }
    result.last_position = p;
    result.last_time = calendar::turn;
    return result;
}

void comfort_data::deserialize_comfort( const JsonObject &jo, bool was_loaded )
{
    if( !was_loaded ) {
        if( jo.has_int( "comfort" ) ) {
            base_comfort = jo.get_int( "comfort" );
        } else if( jo.has_string( "comfort" ) ) {
            const std::string str = jo.get_string( "comfort" );
            if( str == "impossible" ) {
                base_comfort = COMFORT_IMPOSSIBLE;
            } else if( str == "uncomfortable" ) {
                base_comfort = COMFORT_UNCOMFORTABLE;
            } else if( str == "neutral" ) {
                base_comfort = COMFORT_NEUTRAL;
            } else if( str == "slightly_comfortable" ) {
                base_comfort = COMFORT_SLIGHTLY_COMFORTABLE;
            } else if( str == "sleep_aid" ) {
                base_comfort = COMFORT_SLEEP_AID;
            } else if( str == "comfortable" ) {
                base_comfort = COMFORT_COMFORTABLE;
            } else if( str == "very_comfortable" ) {
                base_comfort = COMFORT_VERY_COMFORTABLE;
            } else {
                jo.throw_error( "invalid comfort level" );
            }
        }
    }
}

void comfort_data::deserialize( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "conditions", conditions );
    optional( jo, was_loaded, "conditions_or", conditions_or );
    deserialize_comfort( jo, was_loaded );
    optional( jo, was_loaded, "add_human_comfort", add_human_comfort );
    optional( jo, was_loaded, "add_sleep_aids", add_sleep_aids );
    optional( jo, was_loaded, "msg_try", msg_try );
    optional( jo, was_loaded, "msg_hint", msg_hint );
    optional( jo, was_loaded, "msg_fall", msg_fall );
    was_loaded = true;
}
