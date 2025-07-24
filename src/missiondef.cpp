#include "mission.h" // IWYU pragma: associated

#include <algorithm>
#include <set>

#include "assign.h"
#include "condition.h"
#include "debug.h"
#include "dialogue.h"
#include "enum_conversions.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "item.h"
#include "rng.h"

static const std::map<std::string, std::function<void( mission * )>> mission_function_map = {{
        // Starts
        { "standard", { } },
        { "kill_nemesis", mission_start::kill_nemesis },
        { "place_npc_software", mission_start::place_npc_software },
        { "place_deposit_box", mission_start::place_deposit_box },
        { "find_safety", mission_start::find_safety },
        { "place_book", mission_start::place_book },
        { "reveal_refugee_center", mission_start::reveal_refugee_center },
        { "create_lab_console", mission_start::create_lab_console },
        { "create_hidden_lab_console", mission_start::create_hidden_lab_console },
        { "create_ice_lab_console", mission_start::create_ice_lab_console },
        // Endings
        // Failures
    }
};

static const std::map<std::string, std::function<bool( const tripoint_abs_omt & )>>
tripoint_function_map = {{
        { "never", mission_place::never },
        { "always", mission_place::always },
        { "near_town", mission_place::near_town }
    }
};

namespace io
{
template<>
std::string enum_to_string<mission_origin>( mission_origin data )
{
    switch( data ) {
        // *INDENT-OFF*
        case ORIGIN_NULL: return "ORIGIN_NULL";
        case ORIGIN_GAME_START: return "ORIGIN_GAME_START";
        case ORIGIN_OPENER_NPC: return "ORIGIN_OPENER_NPC";
        case ORIGIN_ANY_NPC: return "ORIGIN_ANY_NPC";
        case ORIGIN_SECONDARY: return "ORIGIN_SECONDARY";
        case ORIGIN_COMPUTER: return "ORIGIN_COMPUTER";
        // *INDENT-ON*
        case mission_origin::NUM_ORIGIN:
            break;
    }
    cata_fatal( "Invalid mission_origin" );
}

template<>
std::string enum_to_string<mission_goal>( mission_goal data )
{
    switch( data ) {
        // *INDENT-OFF*
        case MGOAL_NULL: return "MGOAL_NULL";
        case MGOAL_GO_TO: return "MGOAL_GO_TO";
        case MGOAL_GO_TO_TYPE: return "MGOAL_GO_TO_TYPE";
        case MGOAL_FIND_ITEM: return "MGOAL_FIND_ITEM";
        case MGOAL_FIND_ANY_ITEM: return "MGOAL_FIND_ANY_ITEM";
        case MGOAL_FIND_ITEM_GROUP: return "MGOAL_FIND_ITEM_GROUP";
        case MGOAL_FIND_MONSTER: return "MGOAL_FIND_MONSTER";
        case MGOAL_FIND_NPC: return "MGOAL_FIND_NPC";
        case MGOAL_ASSASSINATE: return "MGOAL_ASSASSINATE";
        case MGOAL_KILL_MONSTER: return "MGOAL_KILL_MONSTER";
        case MGOAL_KILL_MONSTERS: return "MGOAL_KILL_MONSTERS";
        case MGOAL_KILL_MONSTER_TYPE: return "MGOAL_KILL_MONSTER_TYPE";
        case MGOAL_KILL_MONSTER_SPEC: return "MGOAL_KILL_MONSTER_SPEC";
        case MGOAL_KILL_NEMESIS: return "MGOAL_KILL_NEMESIS";
        case MGOAL_RECRUIT_NPC: return "MGOAL_RECRUIT_NPC";
        case MGOAL_RECRUIT_NPC_CLASS: return "MGOAL_RECRUIT_NPC_CLASS";
        case MGOAL_COMPUTER_TOGGLE: return "MGOAL_COMPUTER_TOGGLE";
        case MGOAL_TALK_TO_NPC: return "MGOAL_TALK_TO_NPC";
        case MGOAL_CONDITION: return "MGOAL_CONDITION";
        // *INDENT-ON*
        case mission_goal::NUM_MGOAL:
            break;
    }
    cata_fatal( "Invalid mission_goal" );
}
} // namespace io

static generic_factory<mission_type> mission_type_factory( "mission_type" );

/** @relates string_id */
template<>
const mission_type &string_id<mission_type>::obj() const
{
    return mission_type_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<mission_type>::is_valid() const
{
    return mission_type_factory.is_valid( *this );
}

void mission_type::load_mission_type( const JsonObject &jo, const std::string &src )
{
    mission_type_factory.load( jo, src );
}

void mission_type::reset()
{
    mission_type_factory.reset();
}

template <typename Fun>
void assign_function( const JsonObject &jo, const std::string &id, Fun &target,
                      const std::map<std::string, Fun> &cont )
{
    if( jo.has_string( id ) ) {
        const auto iter = cont.find( jo.get_string( id ) );
        if( iter != cont.end() ) {
            target = iter->second;
        } else {
            jo.throw_error_at( id, "Invalid mission function" );
        }
    }
}

bool mission_type::load( const JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";

    mandatory( jo, was_loaded, "name", name );

    mandatory( jo, was_loaded, "difficulty", difficulty );
    mandatory( jo, was_loaded, "value", value );

    if( jo.has_member( "origins" ) ) {
        origins.clear();
        for( const std::string &m : jo.get_tags( "origins" ) ) {
            origins.emplace_back( io::string_to_enum<mission_origin>( m ) );
        }
    }

    if( std::any_of( origins.begin(), origins.end(), []( mission_origin origin ) {
    return origin == ORIGIN_ANY_NPC || origin == ORIGIN_OPENER_NPC || origin == ORIGIN_SECONDARY;
} ) ) {
        JsonObject djo = jo.get_object( "dialogue" );
        // TODO: There should be a cleaner way to do it
        mandatory( djo, was_loaded, "describe", dialogue[ "describe" ] );
        mandatory( djo, was_loaded, "offer", dialogue[ "offer" ] );
        mandatory( djo, was_loaded, "accepted", dialogue[ "accepted" ] );
        mandatory( djo, was_loaded, "rejected", dialogue[ "rejected" ] );
        mandatory( djo, was_loaded, "advice", dialogue[ "advice" ] );
        mandatory( djo, was_loaded, "inquire", dialogue[ "inquire" ] );
        mandatory( djo, was_loaded, "success", dialogue[ "success" ] );
        mandatory( djo, was_loaded, "success_lie", dialogue[ "success_lie" ] );
        mandatory( djo, was_loaded, "failure", dialogue[ "failure" ] );
    }

    optional( jo, was_loaded, "description", description );
    optional( jo, was_loaded, "urgent", urgent );
    optional( jo, was_loaded, "item", item_id );
    optional( jo, was_loaded, "item_group", group_id );
    optional( jo, was_loaded, "count", item_count, 1 );
    optional( jo, was_loaded, "required_container", container_id );
    optional( jo, was_loaded, "remove_container", remove_container );
    //intended for situations where closed and open container are different
    optional( jo, was_loaded, "empty_container", empty_container );
    optional( jo, was_loaded, "has_generic_rewards", has_generic_rewards, true );

    goal = jo.get_enum_value<decltype( goal )>( "goal" );

    assign_function( jo, "place", place, tripoint_function_map );
    const auto parse_phase = [&]( const std::string & phase,
    std::function<void( mission * )> &phase_func ) {
        if( jo.has_string( phase ) ) {
            assign_function( jo, phase, phase_func, mission_function_map );
        } else if( jo.has_member( phase ) ) {
            JsonObject j_start = jo.get_object( phase );
            if( !parse_funcs( j_start, src, phase_func ) ) {
                j_start.allow_omitted_members();
                return false;
            }
        }
        return true;
    };
    if( !parse_phase( "start", start ) ) {
        return false;
    }
    if( !parse_phase( "end", end ) ) {
        return false;
    }
    if( !parse_phase( "fail", fail ) ) {
        return false;
    }

    deadline = get_duration_or_var( jo, "deadline", false );

    if( jo.has_member( "followup" ) ) {
        follow_up = mission_type_id( jo.get_string( "followup" ) );
    }

    if( jo.has_member( "monster_species" ) ) {
        monster_species = species_id( jo.get_string( "monster_species" ) );
    }
    if( jo.has_member( "monster_type" ) ) {
        monster_type = mtype_id( jo.get_string( "monster_type" ) );
    }

    if( jo.has_member( "monster_kill_goal" ) ) {
        monster_kill_goal = jo.get_int( "monster_kill_goal" );
    }

    assign( jo, "destination", target_id, strict );

    if( jo.has_member( "goal_condition" ) ) {
        read_condition( jo, "goal_condition", goal_condition, true );
    }

    optional( jo, was_loaded, "invisible_on_complete", invisible_on_complete, false );

    return true;
}

bool mission_type::test_goal_condition( struct dialogue &d ) const
{
    if( goal_condition ) {
        return goal_condition( d );
    }
    return true;
}

void mission_type::finalize()
{
}

void mission_type::check_consistency()
{
    for( const mission_type &m : get_all() ) {
        if( !m.item_id.is_empty() && !item::type_is_defined( m.item_id ) ) {
            debugmsg( "Mission %s has undefined item id %s", m.id.c_str(), m.item_id.c_str() );
        }
    }
}

const mission_type *mission_type::get( const mission_type_id &id )
{
    if( id.is_null() ) {
        return nullptr;
    }

    return &id.obj();
}

const std::vector<mission_type> &mission_type::get_all()
{
    return mission_type_factory.get_all();
}

mission_type_id mission_type::get_random_id( const mission_origin origin,
        const tripoint_abs_omt &p )
{
    std::vector<mission_type_id> valid;
    for( const mission_type &t : get_all() ) {
        if( std::find( t.origins.begin(), t.origins.end(), origin ) == t.origins.end() ) {
            continue;
        }
        if( t.place( p ) ) {
            valid.push_back( t.id );
        }
    }
    return random_entry( valid, mission_type_id::NULL_ID() );
}
