#include "mission.h"
#include "translations.h"
#include "rng.h"
#include "generic_factory.h"
#include "calendar.h"
#include "item.h"
#include "assign.h"

#include <algorithm>

enum legacy_mission_type_id {
    MISSION_NULL,
    MISSION_GET_ANTIBIOTICS,
    MISSION_GET_SOFTWARE,
    MISSION_GET_ZOMBIE_BLOOD_ANAL,
    MISSION_RESCUE_DOG,
    MISSION_KILL_ZOMBIE_MOM,
    MISSION_REACH_SAFETY,
    MISSION_GET_FLAG,                      //patriot 1
    MISSION_GET_BLACK_BOX,                 //patriot 2
    MISSION_GET_BLACK_BOX_TRANSCRIPT,      //patriot 3
    MISSION_EXPLORE_SARCOPHAGUS,           //patriot 4
    MISSION_GET_RELIC,                     //martyr 1
    MISSION_RECOVER_PRIEST_DIARY,          //martyr 2
    MISSION_INVESTIGATE_CULT,              //martyr 3
    MISSION_INVESTIGATE_PRISON_VISIONARY,  //martyr 4
    MISSION_GET_RECORD_WEATHER,            //scientist 1
    MISSION_GET_RECORD_PATIENT,            //humanitarian 1
    MISSION_REACH_FEMA_CAMP,               //humanitarian 2
    MISSION_REACH_FARM_HOUSE,              //humanitarian 3
    MISSION_GET_RECORD_ACCOUNTING,         //vigilante 1
    MISSION_GET_SAFE_BOX,                  //vigilante 2
    MISSION_GET_DEPUTY_BADGE,              //vigilante 3
    MISSION_KILL_JABBERWOCK,               //demon slayer 1
    MISSION_KILL_100_Z,                    //demon slayer 2
    MISSION_KILL_HORDE_MASTER,             //demon slayer 3
    MISSION_RECRUIT_TRACKER,               //demon slayer 4
    MISSION_JOIN_TRACKER,                  //demon slayer 4b
    MISSION_FREE_MERCHANTS_EVAC_1,         //Clear Back Bay
    MISSION_FREE_MERCHANTS_EVAC_2,         //Kill Raiders
    MISSION_FREE_MERCHANTS_EVAC_4,         //Acquire Plutonium Cells
    MISSION_OLD_GUARD_REP_1,               //Bandit Pair
    MISSION_OLD_GUARD_REP_2,               //Raider Informant
    MISSION_OLD_GUARD_REP_3,               //Missing without a trace
    MISSION_OLD_GUARD_REP_4,               //Raider Camp
    MISSION_OLD_GUARD_NEC_1,               //Locate Commo team for Necropolis Commander
    MISSION_OLD_GUARD_NEC_2,               //Cull Nightmares
    MISSION_OLD_GUARD_NEC_COMMO_1,         //Build a radio repeater mod
    MISSION_OLD_GUARD_NEC_COMMO_2,         //Disable external power connection
    MISSION_OLD_GUARD_NEC_COMMO_3,         //Install repeater mod in local radio station
    MISSION_OLD_GUARD_NEC_COMMO_4,         //Cyclical mission to install repeater mods
    MISSION_RANCH_FOREMAN_1,               //Rebuild civilization one 2x4 at a time
    MISSION_RANCH_FOREMAN_2,               //Beds need blankets to make
    MISSION_RANCH_FOREMAN_3,               //You can never have enough nails!
    MISSION_RANCH_FOREMAN_4,               //Need salt to trade for seed
    MISSION_RANCH_FOREMAN_5,               //Need liquid fertilizer
    MISSION_RANCH_FOREMAN_6,               //Need stone for well and fireplaces
    MISSION_RANCH_FOREMAN_7,               //Need pipes to finish well and parts for lumberyard
    MISSION_RANCH_FOREMAN_8,               //Need motors to finish sawmill
    MISSION_RANCH_FOREMAN_9,               //Need bleach to sterilize for clinic
    MISSION_RANCH_FOREMAN_10,              //Need first aid kits for clinic
    MISSION_RANCH_FOREMAN_11,              //Need welders for chop-shop
    MISSION_RANCH_FOREMAN_12,              //Need car batteries to power equipment
    MISSION_RANCH_FOREMAN_13,              //Need pair of two-way radios for scavengers
    MISSION_RANCH_FOREMAN_14,              //Need 5 backpacks for scavengers
    MISSION_RANCH_FOREMAN_15,              //Need Homebrewer's Bible for Bar
    MISSION_RANCH_FOREMAN_16,              //Need Sugar for Bar
    MISSION_RANCH_FOREMAN_17,              //Need glass sheets for 1st green house
    MISSION_RANCH_NURSE_1,                 //Need aspirin
    MISSION_RANCH_NURSE_2,                 //Need hotplates
    MISSION_RANCH_NURSE_3,                 //Need multivitamins
    MISSION_RANCH_NURSE_4,                 //Need charcoal water filters
    MISSION_RANCH_NURSE_5,                 //Need chemistry set
    MISSION_RANCH_NURSE_6,                 //Need filter masks
    MISSION_RANCH_NURSE_7,                 //Need rubber gloves
    MISSION_RANCH_NURSE_8,                 //Need X-Acto
    MISSION_RANCH_NURSE_9,                 //Need Guide to Advanced Emergency Care
    MISSION_RANCH_NURSE_10,                //Need flu shot
    MISSION_RANCH_NURSE_11,                //Need empty syringes
    MISSION_RANCH_SCAVENGER_1,             //Need knife spears
    MISSION_RANCH_SCAVENGER_2,             //Need wearable flashlights
    MISSION_RANCH_SCAVENGER_3,             //Need leather body armor
    MISSION_RANCH_SCAVENGER_4,             //Need Molotov cocktails
    MISSION_RANCH_BARTENDER_1,             //Need Stills
    MISSION_RANCH_BARTENDER_2,             //Need Yeast
    MISSION_RANCH_BARTENDER_3,             //Need Sugar Beet Seeds
    MISSION_RANCH_BARTENDER_4,             //Need Metal Tanks
    MISSION_RANCH_BARTENDER_5,             //Need 55-Gallon Drums
    MISSION_FREE_MERCHANTS_EVAC_3,         //Info from Commune
    NUM_MISSION_IDS
};

static const std::map<std::string, std::function<void(mission *)>> mission_function_map = {{
    // Starts
    { "standard", { } },
    { "join", mission_start::join },
    { "infect_npc", mission_start::infect_npc },
    { "need_drugs_npc", mission_start::need_drugs_npc },
    { "place_dog", mission_start::place_dog },
    { "place_zombie_mom", mission_start::place_zombie_mom },
    { "place_zombie_bay", mission_start::place_zombie_bay },
    { "place_caravan_ambush", mission_start::place_caravan_ambush },
    { "place_bandit_cabin", mission_start::place_bandit_cabin },
    { "place_informant", mission_start::place_informant },
    { "place_grabber", mission_start::place_grabber },
    { "place_bandit_camp", mission_start::place_bandit_camp },
    { "place_jabberwock", mission_start::place_jabberwock },
    { "kill_100_z", mission_start::kill_100_z },
    { "kill_20_nightmares", mission_start::kill_20_nightmares },
    { "kill_horde_master", mission_start::kill_horde_master },
    { "place_npc_software", mission_start::place_npc_software },
    { "place_priest_diary", mission_start::place_priest_diary },
    { "place_deposit_box", mission_start::place_deposit_box },
    { "reveal_lab_black_box", mission_start::reveal_lab_black_box },
    { "open_sarcophagus", mission_start::open_sarcophagus },
    { "reveal_hospital", mission_start::reveal_hospital },
    { "find_safety", mission_start::find_safety },
    { "point_prison", mission_start::point_prison },
    { "point_cabin_strange", mission_start::point_cabin_strange },
    { "recruit_tracker", mission_start::recruit_tracker },
    { "radio_repeater", mission_start::radio_repeater },
    { "start_commune", mission_start::start_commune },
    { "ranch_construct_1", mission_start::ranch_construct_1 },
    { "ranch_construct_2", mission_start::ranch_construct_2 },
    { "ranch_construct_3", mission_start::ranch_construct_3 },
    { "ranch_construct_4", mission_start::ranch_construct_4 },
    { "ranch_construct_5", mission_start::ranch_construct_5 },
    { "ranch_construct_6", mission_start::ranch_construct_6 },
    { "ranch_construct_7", mission_start::ranch_construct_7 },
    { "ranch_construct_8", mission_start::ranch_construct_8 },
    { "ranch_construct_9", mission_start::ranch_construct_9 },
    { "ranch_construct_10", mission_start::ranch_construct_10 },
    { "ranch_construct_11", mission_start::ranch_construct_11 },
    { "ranch_construct_12", mission_start::ranch_construct_12 },
    { "ranch_construct_13", mission_start::ranch_construct_13 },
    { "ranch_construct_14", mission_start::ranch_construct_14 },
    { "ranch_construct_15", mission_start::ranch_construct_15 },
    { "ranch_construct_16", mission_start::ranch_construct_16 },
    { "ranch_nurse_1", mission_start::ranch_nurse_1 },
    { "ranch_nurse_2", mission_start::ranch_nurse_2 },
    { "ranch_nurse_3", mission_start::ranch_nurse_3 },
    { "ranch_nurse_4", mission_start::ranch_nurse_4 },
    { "ranch_nurse_5", mission_start::ranch_nurse_5 },
    { "ranch_nurse_6", mission_start::ranch_nurse_6 },
    { "ranch_nurse_7", mission_start::ranch_nurse_7 },
    { "ranch_nurse_8", mission_start::ranch_nurse_8 },
    { "ranch_nurse_9", mission_start::ranch_nurse_9 },
    { "ranch_scavenger_1", mission_start::ranch_scavenger_1 },
    { "ranch_scavenger_2", mission_start::ranch_scavenger_2 },
    { "ranch_scavenger_3", mission_start::ranch_scavenger_3 },
    { "ranch_bartender_1", mission_start::ranch_bartender_1 },
    { "ranch_bartender_2", mission_start::ranch_bartender_2 },
    { "ranch_bartender_3", mission_start::ranch_bartender_3 },
    { "ranch_bartender_4", mission_start::ranch_bartender_4 },
    { "place_book", mission_start::place_book },
    { "reveal_weather_station", mission_start::reveal_weather_station },
    { "reveal_office_tower", mission_start::reveal_office_tower },
    { "reveal_doctors_office", mission_start::reveal_doctors_office },
    { "reveal_cathedral", mission_start::reveal_cathedral },
    { "reveal_refugee_center", mission_start::reveal_refugee_center },
    // Endings
    { "leave", mission_end::leave },
    { "thankful", mission_end::thankful },
    { "deposit_box", mission_end::deposit_box },
    { "heal_infection", mission_end::heal_infection },
    // Failures
    { "kill_npc", mission_fail::kill_npc },
}};

static const std::map<std::string, std::function<bool(const tripoint &)>> tripoint_function_map = {{
    { "never", mission_place::never },
    { "always", mission_place::always },
    { "near_town", mission_place::near_town }
}};

namespace io {
static const std::map<std::string, mission_origin> origin_map = {{
    { "ORIGIN_NULL", ORIGIN_NULL },
    { "ORIGIN_GAME_START", ORIGIN_GAME_START },
    { "ORIGIN_OPENER_NPC", ORIGIN_OPENER_NPC },
    { "ORIGIN_ANY_NPC", ORIGIN_ANY_NPC },
    { "ORIGIN_SECONDARY", ORIGIN_SECONDARY },
    { "ORIGIN_COMPUTER", ORIGIN_COMPUTER }
}};
template<>
mission_origin string_to_enum<mission_origin>( const std::string &data )
{
    return string_to_enum_look_up( origin_map, data );
}

static const std::map<std::string, mission_goal> goal_map = {{
    { "MGOAL_NULL", MGOAL_NULL },
    { "MGOAL_GO_TO", MGOAL_GO_TO },
    { "MGOAL_GO_TO_TYPE", MGOAL_GO_TO_TYPE },
    { "MGOAL_FIND_ITEM", MGOAL_FIND_ITEM },
    { "MGOAL_FIND_ANY_ITEM", MGOAL_FIND_ANY_ITEM },
    { "MGOAL_FIND_MONSTER", MGOAL_FIND_MONSTER },
    { "MGOAL_FIND_NPC", MGOAL_FIND_NPC },
    { "MGOAL_ASSASSINATE", MGOAL_ASSASSINATE },
    { "MGOAL_KILL_MONSTER", MGOAL_KILL_MONSTER },
    { "MGOAL_KILL_MONSTER_TYPE", MGOAL_KILL_MONSTER_TYPE },
    { "MGOAL_KILL_MONSTER_SPEC", MGOAL_KILL_MONSTER_SPEC },
    { "MGOAL_RECRUIT_NPC", MGOAL_RECRUIT_NPC },
    { "MGOAL_RECRUIT_NPC_CLASS", MGOAL_RECRUIT_NPC_CLASS },
    { "MGOAL_COMPUTER_TOGGLE", MGOAL_COMPUTER_TOGGLE }
}};
template<>
mission_goal string_to_enum<mission_goal>( const std::string &data )
{
    return string_to_enum_look_up( goal_map, data );
}
} // namespace io


generic_factory<mission_type> mission_type_factory( "mission_type" );

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

void mission_type::load_mission_type( JsonObject &jo, const std::string &src )
{
    mission_type_factory.load( jo, src );
}

void mission_type::reset()
{
    mission_type_factory.reset();
}

template <typename Fun>
void assign_function( JsonObject &jo, const std::string &id, Fun &target, const std::map<std::string, Fun> &cont )
{
    if( jo.has_string( id ) ) {
        const auto iter = cont.find( jo.get_string( id ) );
        if( iter != cont.end() ) {
            target = iter->second;
        } else {
            jo.throw_error( "Invalid mission function", id );
        }
    }
}

void mission_type::load( JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";

    mandatory( jo, was_loaded, "name", name );

    mandatory( jo, was_loaded, "difficulty", difficulty );
    mandatory( jo, was_loaded, "value", value );

    if( jo.has_member( "origins" ) ) {
        origins.clear();
        for( auto &m : jo.get_tags( "origins" ) ) {
            origins.emplace_back( io::string_to_enum_look_up( io::origin_map, m ) );
        }
    }

    if( std::any_of( origins.begin(), origins.end(), []( mission_origin origin ) {
        return origin == ORIGIN_ANY_NPC || origin == ORIGIN_OPENER_NPC || origin == ORIGIN_SECONDARY;
    } ) ) {
        auto djo = jo.get_object( "dialogue" );
        // @todo: There should be a cleaner way to do it
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

    optional( jo, was_loaded, "urgent", urgent );
    optional( jo, was_loaded, "item", item_id );
    optional( jo, was_loaded, "count", item_count, 1 );

    goal = jo.get_enum_value<decltype( goal )>( "goal" );

    assign_function( jo, "place", place, tripoint_function_map );
    assign_function( jo, "start", start, mission_function_map );
    assign_function( jo, "end", end, mission_function_map );
    assign_function( jo, "fail", fail, mission_function_map );

    assign( jo, "deadline_low", deadline_low, false, 1_days );
    assign( jo, "deadline_high", deadline_high, false, 1_days );

    if( jo.has_member( "followup" ) ) {
        follow_up = mission_type_id( jo.get_string( "followup" ) );
    }

    if( jo.has_member( "monster_species" ) ) {
        monster_species = species_id( jo.get_string( "monster_species" ) );
    }

    if( jo.has_member( "monster_kill_goal" ) ) {
        monster_kill_goal = jo.get_int( "monster_kill_goal" );
    }

    assign( jo, "destination", target_id, strict );
}

void mission_type::check_consistency()
{
    for( const auto &m : get_all() ) {
        if( !m.item_id.empty() && !item::type_is_defined( m.item_id ) ) {
            debugmsg( "Mission %s has undefined item id %s", m.id.c_str(), m.item_id.c_str() );
        }
    }
}

mission_type_id mission_type::from_legacy( int old_id )
{
    static const std::vector<mission_type_id> old_id_vec = {{
        mission_type_id( "MISSION_NULL" ),
        mission_type_id( "MISSION_GET_ANTIBIOTICS" ),
        mission_type_id( "MISSION_GET_SOFTWARE" ),
        mission_type_id( "MISSION_GET_ZOMBIE_BLOOD_ANAL" ),
        mission_type_id( "MISSION_RESCUE_DOG" ),
        mission_type_id( "MISSION_KILL_ZOMBIE_MOM" ),
        mission_type_id( "MISSION_REACH_SAFETY" ),
        mission_type_id( "MISSION_GET_FLAG" ),
        mission_type_id( "MISSION_GET_BLACK_BOX" ),
        mission_type_id( "MISSION_GET_BLACK_BOX_TRANSCRIPT" ),
        mission_type_id( "MISSION_EXPLORE_SARCOPHAGUS" ),
        mission_type_id( "MISSION_GET_RELIC" ),
        mission_type_id( "MISSION_RECOVER_PRIEST_DIARY" ),
        mission_type_id( "MISSION_INVESTIGATE_CULT" ),
        mission_type_id( "MISSION_INVESTIGATE_PRISON_VISIONARY" ),
        mission_type_id( "MISSION_GET_RECORD_WEATHER" ),
        mission_type_id( "MISSION_GET_RECORD_PATIENT" ),
        mission_type_id( "MISSION_REACH_FEMA_CAMP" ),
        mission_type_id( "MISSION_REACH_FARM_HOUSE" ),
        mission_type_id( "MISSION_GET_RECORD_ACCOUNTING" ),
        mission_type_id( "MISSION_GET_SAFE_BOX" ),
        mission_type_id( "MISSION_GET_DEPUTY_BADGE" ),
        mission_type_id( "MISSION_KILL_JABBERWOCK" ),
        mission_type_id( "MISSION_KILL_100_Z" ),
        mission_type_id( "MISSION_KILL_HORDE_MASTER" ),
        mission_type_id( "MISSION_RECRUIT_TRACKER" ),
        mission_type_id( "MISSION_JOIN_TRACKER" ),
        mission_type_id( "MISSION_FREE_MERCHANTS_EVAC_1" ),
        mission_type_id( "MISSION_FREE_MERCHANTS_EVAC_2" ),
        mission_type_id( "MISSION_FREE_MERCHANTS_EVAC_4" ),
        mission_type_id( "MISSION_OLD_GUARD_REP_1" ),
        mission_type_id( "MISSION_OLD_GUARD_REP_2" ),
        mission_type_id( "MISSION_OLD_GUARD_REP_3" ),
        mission_type_id( "MISSION_OLD_GUARD_REP_4" ),
        mission_type_id( "MISSION_OLD_GUARD_NEC_1" ),
        mission_type_id( "MISSION_OLD_GUARD_NEC_2" ),
        mission_type_id( "MISSION_OLD_GUARD_NEC_COMMO_1" ),
        mission_type_id( "MISSION_OLD_GUARD_NEC_COMMO_2" ),
        mission_type_id( "MISSION_OLD_GUARD_NEC_COMMO_3" ),
        mission_type_id( "MISSION_OLD_GUARD_NEC_COMMO_4" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_1" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_2" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_3" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_4" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_5" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_6" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_7" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_8" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_9" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_10" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_11" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_12" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_13" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_14" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_15" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_16" ),
        mission_type_id( "MISSION_RANCH_FOREMAN_17" ),
        mission_type_id( "MISSION_RANCH_NURSE_1" ),
        mission_type_id( "MISSION_RANCH_NURSE_2" ),
        mission_type_id( "MISSION_RANCH_NURSE_3" ),
        mission_type_id( "MISSION_RANCH_NURSE_4" ),
        mission_type_id( "MISSION_RANCH_NURSE_5" ),
        mission_type_id( "MISSION_RANCH_NURSE_6" ),
        mission_type_id( "MISSION_RANCH_NURSE_7" ),
        mission_type_id( "MISSION_RANCH_NURSE_8" ),
        mission_type_id( "MISSION_RANCH_NURSE_9" ),
        mission_type_id( "MISSION_RANCH_NURSE_10" ),
        mission_type_id( "MISSION_RANCH_NURSE_11" ),
        mission_type_id( "MISSION_RANCH_SCAVENGER_1" ),
        mission_type_id( "MISSION_RANCH_SCAVENGER_2" ),
        mission_type_id( "MISSION_RANCH_SCAVENGER_3" ),
        mission_type_id( "MISSION_RANCH_SCAVENGER_4" ),
        mission_type_id( "MISSION_RANCH_BARTENDER_1" ),
        mission_type_id( "MISSION_RANCH_BARTENDER_2" ),
        mission_type_id( "MISSION_RANCH_BARTENDER_3" ),
        mission_type_id( "MISSION_RANCH_BARTENDER_4" ),
        mission_type_id( "MISSION_RANCH_BARTENDER_5" ),
        mission_type_id( "MISSION_FREE_MERCHANTS_EVAC_3" ),
        // This is to help with the bugged find book mission
        mission_type_id( "MISSION_NULL" )
    }};

    if( old_id >= 0 && old_id < (int)old_id_vec.size() ) {
        return old_id_vec[ old_id ];
    }

    debugmsg( "Invalid legacy mission id: %d", old_id );
    return mission_type_id( "MISSION_NULL" );
}

const mission_type *mission_type::get( const mission_type_id id )
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

mission_type_id mission_type::get_random_id( const mission_origin origin, const tripoint &p )
{
    std::vector<mission_type_id> valid;
    for( auto & t : get_all() ) {
        if( std::find( t.origins.begin(), t.origins.end(), origin ) == t.origins.end() ) {
            continue;
        }
        if( t.place( p ) ) {
            valid.push_back( t.id );
        }
    }
    return random_entry( valid, mission_type_id::NULL_ID() );
}
