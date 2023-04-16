#include <cstdio>
#include <functional>
#include <iosfwd>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_id.h"
#include "coordinate_conversions.h"
#include "coordinates.h"
#include "dialogue.h"
#include "dialogue_chatbin.h"
#include "effect.h"
#include "event.h"
#include "event_bus.h"
#include "event_subscriber.h"
#include "faction.h"
#include "game.h"
#include "input.h"
#include "item.h"
#include "item_category.h"
#include "kill_tracker.h"
#include "map.h"
#include "map_helpers.h"
#include "mission.h"
#include "morale_types.h"
#include "npc.h"
#include "npctalk.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "point.h"
#include "talker.h"
#include "type_id.h"

static const bionic_id bio_ads( "bio_ads" );
static const bionic_id bio_power_storage( "bio_power_storage" );

static const efftype_id effect_currently_busy( "currently_busy" );
static const efftype_id effect_gave_quest_item( "gave_quest_item" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_infection( "infection" );
static const efftype_id effect_sleep( "sleep" );

static const item_category_id item_category_food( "food" );
static const item_category_id item_category_manual( "manual" );

static const itype_id itype_beer( "beer" );
static const itype_id itype_bottle_glass( "bottle_glass" );
static const itype_id itype_dnd_handbook( "dnd_handbook" );
static const itype_id itype_manual_speech( "manual_speech" );

static const mtype_id mon_zombie_bio_op( "mon_zombie_bio_op" );

static const npc_class_id NC_NONE( "NC_NONE" );
static const npc_class_id NC_TEST_CLASS( "NC_TEST_CLASS" );

static const proficiency_id proficiency_prof_test( "prof_test" );

static const skill_id skill_driving( "driving" );

static const spell_id spell_test_spell_json( "test_spell_json" );
static const spell_id spell_test_spell_lava( "test_spell_lava" );
static const spell_id spell_test_spell_pew( "test_spell_pew" );

static const trait_id trait_ELFA_EARS( "ELFA_EARS" );

static const trait_id trait_PROF_FED( "PROF_FED" );
static const trait_id trait_PROF_SWAT( "PROF_SWAT" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );
static const trait_id trait_WEB_WEAVER( "WEB_WEAVER" );
static const trait_id trait_test_trait( "test_trait" );

static npc &create_test_talker( bool shopkeep = false )
{
    const string_id<npc_template> test_talker( shopkeep ? "test_shopkeep" : "test_talker" );
    const character_id model_id = get_map().place_npc( point( 25, 25 ), test_talker );
    g->load_npcs();

    npc *model_npc = g->find_npc( model_id );
    REQUIRE( model_npc != nullptr );

    for( const trait_id &tr : model_npc->get_mutations() ) {
        model_npc->unset_mutation( tr );
    }
    model_npc->name = "Beta NPC";
    model_npc->set_hunger( 0 );
    model_npc->set_thirst( 0 );
    model_npc->set_fatigue( 0 );
    model_npc->remove_effect( effect_sleep );
    // An ugly hack to prevent NPC falling asleep during testing due to massive fatigue
    model_npc->set_mutation( trait_WEB_WEAVER );

    return *model_npc;
}

static void gen_response_lines( dialogue &d, size_t expected_count )
{
    d.gen_responses( d.topic_stack.back() );
    for( talk_response &response : d.responses ) {
        response.create_option_line( d, input_event() );
    }
    if( d.responses.size() != expected_count ) {
        printf( "Test failure in %s\n", d.topic_stack.back().id.c_str() );
        for( talk_response &response : d.responses ) {
            printf( "response: %s\n", response.text.c_str() );
        }
    }
    CAPTURE( d.responses );
    REQUIRE( d.responses.size() == expected_count );
}

static std::string gen_dynamic_line( dialogue &d )
{
    std::string challenge = d.dynamic_line( d.topic_stack.back() );
    return challenge;
}

static void change_om_type( const std::string &new_type )
{
    // TODO: fix point types
    const tripoint_abs_omt omt_pos( ms_to_omt_copy( get_map().getabs(
                                        get_player_character().pos() ) ) );
    overmap_buffer.ter_set( omt_pos, oter_id( new_type ) );
}

static npc &prep_test( dialogue &d, bool shopkeep = false )
{
    clear_avatar();
    clear_vehicles();
    clear_map();
    avatar &player_character = get_avatar();
    player_character.set_value( "npctalk_var_test_var", "It's avatar" );
    player_character.name = "Alpha Avatar";
    REQUIRE_FALSE( player_character.in_vehicle );

    const tripoint test_origin( 15, 15, 0 );
    player_character.setpos( test_origin );

    g->faction_manager_ptr->create_if_needed();

    npc &beta = create_test_talker( shopkeep );
    beta.set_value( "npctalk_var_test_var", "It's npc" );
    d = dialogue( get_talker_for( player_character ), get_talker_for( beta ) );
    return beta;
}

TEST_CASE( "npc_talk_start", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    d.add_topic( "TALK_TEST_START" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
}

TEST_CASE( "npc_talk_failures", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    d.add_topic( "TALK_TEST_FAIL_RESPONSE" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "*Impossible Test: Never See this." );
}

TEST_CASE( "npc_talk_failures_topic", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    d.add_topic( "TALK_TEST_FAIL_RESPONSE" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].success.next_topic.id == "TALK_TEST_START" );
}

TEST_CASE( "npc_talk_describe_mission", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    d.add_topic( "TALK_DESCRIBE_MISSION" );
    std::string d_line = gen_dynamic_line( d );
    CHECK( d_line == "I'm looking for wounded to help." );
}

TEST_CASE( "npc_talk_stats", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    Character &player_character = get_avatar();
    player_character.str_cur = 8;
    player_character.dex_cur = 8;
    player_character.int_cur = 8;
    player_character.per_cur = 8;

    d.add_topic( "TALK_TEST_SIMPLE_STATS" );
    gen_response_lines( d, 5 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a strength test response." );
    CHECK( d.responses[2].text == "This is a dexterity test response." );
    CHECK( d.responses[3].text == "This is an intelligence test response." );
    CHECK( d.responses[4].text == "This is a perception test response." );
    player_character.str_cur = 6;
    player_character.dex_cur = 6;
    player_character.int_cur = 6;
    player_character.per_cur = 6;
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );

    d.add_topic( "TALK_TEST_NEGATED_STATS" );
    gen_response_lines( d, 5 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a low strength test response." );
    CHECK( d.responses[2].text == "This is a low dexterity test response." );
    CHECK( d.responses[3].text == "This is a low intelligence test response." );
    CHECK( d.responses[4].text == "This is a low perception test response." );
}

TEST_CASE( "npc_talk_skills", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    Character &player_character = get_avatar();
    player_character.set_skill_level( skill_driving, 8 );

    d.add_topic( "TALK_TEST_SIMPLE_SKILLS" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a driving test response." );

    player_character.set_skill_level( skill_driving, 6 );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );

    d.add_topic( "TALK_TEST_NEGATED_SKILLS" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a low driving test response." );
}

TEST_CASE( "npc_talk_wearing_and_trait", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );

    Character &player_character = get_avatar();
    for( const trait_id &tr : player_character.get_mutations() ) {
        player_character.unset_mutation( tr );
    }

    d.add_topic( "TALK_TEST_WEARING_AND_TRAIT" );
    gen_response_lines( d, 1 );

    CHECK( d.responses[0].text == "This is a basic test response." );
    player_character.toggle_trait( trait_ELFA_EARS );
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a trait test response." );
    CHECK( d.responses[2].text == "This is a short trait test response." );
    player_character.wear_item( item( "badge_marshal" ) );
    gen_response_lines( d, 4 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a trait test response." );
    CHECK( d.responses[2].text == "This is a short trait test response." );
    CHECK( d.responses[3].text == "This is a wearing test response." );
    talker_npc.toggle_trait( trait_ELFA_EARS );
    gen_response_lines( d, 6 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a trait test response." );
    CHECK( d.responses[2].text == "This is a short trait test response." );
    CHECK( d.responses[3].text == "This is a wearing test response." );
    CHECK( d.responses[4].text == "This is a npc trait test response." );
    CHECK( d.responses[5].text == "This is a npc short trait test response." );
    player_character.toggle_trait( trait_ELFA_EARS );
    talker_npc.toggle_trait( trait_ELFA_EARS );
    player_character.toggle_trait( trait_PSYCHOPATH );
    talker_npc.toggle_trait( trait_SAPIOVORE );
    gen_response_lines( d, 4 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a wearing test response." );
    CHECK( d.responses[2].text == "This is a trait flags test response." );
    CHECK( d.responses[3].text == "This is a npc trait flags test response." );
    player_character.toggle_trait( trait_PSYCHOPATH );
    talker_npc.toggle_trait( trait_SAPIOVORE );
}

TEST_CASE( "npc_talk_effect", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );
    Character &player_character = get_avatar();

    d.add_topic( "TALK_TEST_EFFECT" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    talker_npc.add_effect( effect_gave_quest_item, 9999_turns );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an npc effect test response." );
    player_character.add_effect( effect_gave_quest_item, 9999_turns );
    d.gen_responses( d.topic_stack.back() );
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an npc effect test response." );
    CHECK( d.responses[2].text == "This is a player effect test response." );
}

TEST_CASE( "npc_talk_service", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );
    Character &player_character = get_avatar();

    d.add_topic( "TALK_TEST_SERVICE" );
    player_character.cash = 0;
    talker_npc.add_effect( effect_currently_busy, 9999_turns );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    player_character.cash = 800;
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a cash test response." );
    talker_npc.remove_effect( effect_currently_busy );
    gen_response_lines( d, 4 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a cash test response." );
    CHECK( d.responses[2].text == "This is an npc service test response." );
    CHECK( d.responses[3].text == "This is an npc available test response." );
}

TEST_CASE( "npc_talk_location", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    change_om_type( "pond_field_north" );
    d.add_topic( "TALK_TEST_LOCATION" );
    d.gen_responses( d.topic_stack.back() );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    change_om_type( "field" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a om_location_field test response." );
    change_om_type( "faction_base_camp_11" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a faction camp any test response." );
    change_om_type( "evac_center_7_west" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a om_location_field direction variant response." );
}

TEST_CASE( "npc_talk_role", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );

    d.add_topic( "TALK_TEST_NPC_ROLE" );
    talker_npc.companion_mission_role_id = "NO_TEST_ROLE";
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    talker_npc.companion_mission_role_id = "TEST_ROLE";
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a nearby role test response." );
}

TEST_CASE( "npc_talk_class", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );

    d.add_topic( "TALK_TEST_NPC_CLASS" );
    talker_npc.myclass = NC_NONE;
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    talker_npc.myclass = NC_TEST_CLASS;
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a class test response." );

    d.add_topic( "TALK_FRIEND_GUARD" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "I have a custom response to a common topic" );
    CHECK( d.responses[0].success.next_topic.id == "TALK_TEST_FACTION_TRUST" );
}

TEST_CASE( "npc_talk_allies", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );

    for( npc *guy : g->allies() ) {
        talk_function::leave( *guy );
    }
    talk_function::follow( talker_npc );
    d.add_topic( "TALK_TEST_NPC_ALLIES" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a npc allies 1 test response." );
}

TEST_CASE( "npc_talk_rules", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );

    d.add_topic( "TALK_TEST_NPC_RULES" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    talker_npc.rules.engagement = combat_engagement::ALL;
    talker_npc.rules.aim = aim_rule::SPRAY;
    talker_npc.rules.set_flag( ally_rule::use_silent );
    gen_response_lines( d, 4 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a npc engagement rule test response." );
    CHECK( d.responses[2].text == "This is a npc aim rule test response." );
    CHECK( d.responses[3].text == "This is a npc rule test response." );
    talker_npc.rules.clear_flag( ally_rule::use_silent );
}

TEST_CASE( "npc_talk_needs", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );

    d.add_topic( "TALK_TEST_NPC_NEEDS" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    talker_npc.set_thirst( 90 );
    talker_npc.set_hunger( 90 );
    talker_npc.set_fatigue( fatigue_levels::EXHAUSTED );
    gen_response_lines( d, 4 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a npc thirst test response." );
    CHECK( d.responses[2].text == "This is a npc hunger test response." );
    CHECK( d.responses[3].text == "This is a npc fatigue test response." );
}

TEST_CASE( "npc_talk_mission_goal", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );

    d.add_topic( "TALK_TEST_MISSION_GOAL" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    const std::vector<mission_type> &all_missions = mission_type::get_all();
    bool set_mission = false;
    for( const mission_type &some_mission : all_missions ) {
        if( some_mission.goal == MGOAL_ASSASSINATE ) {
            mission *assassinate = mission::reserve_new( some_mission.id, talker_npc.getID() );
            talker_npc.chatbin.mission_selected = assassinate;
            set_mission = true;
            break;
        }
    }
    CHECK( set_mission );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a mission goal test response." );
}

TEST_CASE( "npc_talk_season", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    const time_point old_calendar = calendar::turn;
    calendar::turn = calendar::start_of_cataclysm;
    d.add_topic( "TALK_TEST_SEASON" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a season spring test response." );
    calendar::turn += calendar::season_length();
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a days since Cataclysm 30 test response." );
    CHECK( d.responses[2].text == "This is a season summer test response." );
    calendar::turn += calendar::season_length();
    gen_response_lines( d, 4 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a days since Cataclysm 30 test response." );
    CHECK( d.responses[2].text == "This is a days since Cataclysm 120 test response." );
    CHECK( d.responses[3].text == "This is a season autumn test response." );
    calendar::turn += calendar::season_length();
    gen_response_lines( d, 5 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a days since Cataclysm 30 test response." );
    CHECK( d.responses[2].text == "This is a days since Cataclysm 120 test response." );
    CHECK( d.responses[3].text == "This is a days since Cataclysm 210 test response." );
    CHECK( d.responses[4].text == "This is a season winter test response." );
    calendar::turn += calendar::season_length();
    gen_response_lines( d, 6 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a season spring test response." );
    CHECK( d.responses[2].text == "This is a days since Cataclysm 30 test response." );
    CHECK( d.responses[3].text == "This is a days since Cataclysm 120 test response." );
    CHECK( d.responses[4].text == "This is a days since Cataclysm 210 test response." );
    CHECK( d.responses[5].text == "This is a days since Cataclysm 300 test response." );
    calendar::turn = old_calendar;
}

TEST_CASE( "npc_talk_time", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    const time_point old_calendar = calendar::turn;
    calendar::turn = sunrise( calendar::turn ) + 4_hours;
    d.add_topic( "TALK_TEST_TIME" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a is day test response." );
    calendar::turn = sunset( calendar::turn ) + 2_hours;
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a is night test response." );
    calendar::turn = old_calendar;
}

TEST_CASE( "npc_talk_switch", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );
    Character &player_character = get_avatar();

    d.add_topic( "TALK_TEST_SWITCH" );
    player_character.cash = 1000;
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an switch 1 test response." );
    CHECK( d.responses[2].text == "This is another basic test response." );
    player_character.cash = 100;
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an switch 2 test response." );
    CHECK( d.responses[2].text == "This is another basic test response." );
    player_character.cash = 10;
    gen_response_lines( d, 4 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an switch default 1 test response." );
    CHECK( d.responses[2].text == "This is an switch default 2 test response." );
    CHECK( d.responses[3].text == "This is another basic test response." );
    player_character.cash = 0;
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an switch default 2 test response." );
    CHECK( d.responses[2].text == "This is another basic test response." );
}

TEST_CASE( "npc_talk_or", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );
    Character &player_character = get_avatar();

    d.add_topic( "TALK_TEST_OR" );
    player_character.cash = 0;
    talker_npc.add_effect( effect_currently_busy, 9999_turns );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    player_character.toggle_trait( trait_ELFA_EARS );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an or trait test response." );
}

TEST_CASE( "npc_talk_and", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );
    Character &player_character = get_avatar();

    player_character.toggle_trait( trait_ELFA_EARS );
    d.add_topic( "TALK_TEST_AND" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    player_character.cash = 800;
    talker_npc.remove_effect( effect_currently_busy );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an and cash, available, trait test response." );
}

TEST_CASE( "npc_talk_nested", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );
    Character &player_character = get_avatar();

    d.add_topic( "TALK_TEST_NESTED" );
    talker_npc.add_effect( effect_currently_busy, 9999_turns );
    player_character.cash = 0;
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    player_character.cash = 800;
    player_character.int_cur = 11;
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a complex nested test response." );
}

TEST_CASE( "npc_talk_conditionals", "[npc_talk]" )
{
    dialogue d;
    Character &player_character = get_avatar();
    prep_test( d );
    player_character.cash = 800;

    d.add_topic( "TALK_TEST_TRUE_FALSE_CONDITIONAL" );
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a true/false true response." );
    CHECK( d.responses[2].text == "This is a conditional trial response." );
    talk_response &chosen = d.responses[2];
    bool trial_success = chosen.trial.roll( d );
    CHECK( trial_success == true );
    talk_effect_t &trial_effect = trial_success ? chosen.success : chosen.failure;
    CHECK( trial_effect.next_topic.id == "TALK_TEST_TRUE_CONDITION_NEXT" );
    player_character.cash = 0;
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a true/false false response." );
    CHECK( d.responses[2].text == "This is a conditional trial response." );
    chosen = d.responses[2];
    trial_success = chosen.trial.roll( d );
    CHECK( trial_success == false );
    trial_effect = trial_success ? chosen.success : chosen.failure;
    CHECK( trial_effect.next_topic.id == "TALK_TEST_FALSE_CONDITION_NEXT" );
}

TEST_CASE( "npc_talk_items", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );
    Character &player_character = get_avatar();

    player_character.remove_items_with( []( const item & it ) {
        return it.get_category_shallow().get_id() == item_category_manual ||
               it.get_category_shallow().get_id() == item_category_food ||
               it.typeId() == itype_bottle_glass;
    } );
    d.add_topic( "TALK_TEST_HAS_ITEM" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    d.add_topic( "TALK_TEST_ITEM_REPEAT" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );

    const auto has_item = [&]( Character & p, const std::string & id, int count ) {
        item old_item = item( id );
        if( old_item.count_by_charges() ) {
            return p.has_charges( itype_id( id ), count );
        } else {
            return p.has_amount( itype_id( id ), count );
        }
    };
    const auto has_beer_bottle = [&]( Character & p, int count ) {
        return has_item( p, "bottle_glass", 1 ) && has_item( p, "beer", count );
    };
    player_character.cash = 1000;
    player_character.int_cur = 8;
    player_character.worn.wear_item( player_character, item( "backpack" ), false, false );
    d.add_topic( "TALK_TEST_EFFECTS" );
    gen_response_lines( d, 19 );
    // add and remove effect
    REQUIRE_FALSE( player_character.has_effect( effect_infection ) );
    talk_effect_t &effects = d.responses[1].success;
    effects.apply( d );
    CHECK( player_character.has_effect( effect_infection ) );
    CHECK( player_character.get_effect_dur( effect_infection ) == time_duration::from_turns( 10 ) );
    REQUIRE_FALSE( talker_npc.has_effect( effect_infection ) );
    effects = d.responses[2].success;
    effects.apply( d );
    CHECK( talker_npc.has_effect( effect_infection ) );
    CHECK( talker_npc.get_effect( effect_infection ).is_permanent() );
    effects = d.responses[3].success;
    effects.apply( d );
    CHECK_FALSE( player_character.has_effect( effect_infection ) );
    effects = d.responses[4].success;
    effects.apply( d );
    CHECK_FALSE( talker_npc.has_effect( effect_infection ) );
    // add and remove trait
    REQUIRE_FALSE( player_character.has_trait( trait_PROF_FED ) );
    effects = d.responses[5].success;
    effects.apply( d );
    CHECK( player_character.has_trait( trait_PROF_FED ) );
    REQUIRE_FALSE( talker_npc.has_trait( trait_PROF_FED ) );
    effects = d.responses[6].success;
    effects.apply( d );
    CHECK( talker_npc.has_trait( trait_PROF_FED ) );
    effects = d.responses[7].success;
    effects.apply( d );
    CHECK_FALSE( player_character.has_trait( trait_PROF_FED ) );
    effects = d.responses[8].success;
    effects.apply( d );
    CHECK_FALSE( talker_npc.has_trait( trait_PROF_FED ) );
    // buying and spending
    talker_npc.op_of_u.owed = 1000;
    REQUIRE_FALSE( has_beer_bottle( player_character, 2 ) );
    REQUIRE( talker_npc.op_of_u.owed == 1000 );
    effects = d.responses[9].success;
    effects.apply( d );
    CHECK( talker_npc.op_of_u.owed == 500 );
    CHECK( has_beer_bottle( player_character, 2 ) );
    REQUIRE_FALSE( has_item( player_character, "bottle_plastic", 1 ) );
    effects = d.responses[10].success;
    effects.apply( d );
    CHECK( has_item( player_character, "bottle_plastic", 1 ) );
    CHECK( talker_npc.op_of_u.owed == 500 );
    effects = d.responses[11].success;
    effects.apply( d );
    CHECK( talker_npc.op_of_u.owed == 0 );
    talker_npc.op_of_u.owed = 1000;
    // effect chains
    REQUIRE_FALSE( player_character.has_effect( effect_infected ) );
    REQUIRE_FALSE( talker_npc.has_effect( effect_infected ) );
    REQUIRE_FALSE( player_character.has_trait( trait_PROF_SWAT ) );
    REQUIRE_FALSE( talker_npc.has_trait( trait_PROF_SWAT ) );
    effects = d.responses[12].success;
    effects.apply( d );
    CHECK( player_character.has_effect( effect_infected ) );
    CHECK( player_character.get_effect_dur( effect_infected ) == time_duration::from_turns( 10 ) );
    CHECK( talker_npc.has_effect( effect_infected ) );
    CHECK( talker_npc.get_effect( effect_infected ).is_permanent() );
    CHECK( player_character.has_trait( trait_PROF_SWAT ) );
    CHECK( talker_npc.has_trait( trait_PROF_SWAT ) );
    CHECK( talker_npc.op_of_u.owed == 0 );
    CHECK( talker_npc.get_attitude() == NPCATT_KILL );
    // opinion changes
    talker_npc.op_of_u = npc_opinion();
    REQUIRE_FALSE( talker_npc.op_of_u.trust );
    REQUIRE_FALSE( talker_npc.op_of_u.fear );
    REQUIRE_FALSE( talker_npc.op_of_u.value );
    REQUIRE_FALSE( talker_npc.op_of_u.anger );
    REQUIRE_FALSE( talker_npc.op_of_u.owed );
    effects = d.responses[13].success;
    REQUIRE( effects.opinion.trust == 10 );
    effects.apply( d );
    CHECK( talker_npc.op_of_u.trust == 10 );
    CHECK( talker_npc.op_of_u.fear == 11 );
    CHECK( talker_npc.op_of_u.value == 12 );
    CHECK( talker_npc.op_of_u.anger == 13 );
    CHECK( talker_npc.op_of_u.owed == 14 );

    d.add_topic( "TALK_TEST_HAS_ITEM" );
    gen_response_lines( d, 7 );
    CHECK( d.responses[1].text == "This is a basic test response." );
    CHECK( d.responses[2].text == "This is a u_has_item beer test response." );
    CHECK( d.responses[3].text == "This is a u_has_item bottle_glass test response." );
    CHECK( d.responses[4].text == "This is a u_has_items beer test response." );
    CHECK( d.responses[5].text == "This is a u_has_item_category manuals test response." );
    CHECK( d.responses[6].text == "This is a u_has_item_category manuals count 2 test response." );
    CHECK( d.responses[0].text == "This is a repeated item manual_speech test response" );
    CHECK( d.responses[0].success.next_topic.item_type == itype_manual_speech );
    d.add_topic( d.responses[0].success.next_topic );
    gen_dynamic_line( d );
    CHECK( d.cur_item == itype_manual_speech );

    d.add_topic( "TALK_TEST_ITEM_REPEAT" );
    gen_response_lines( d, 8 );
    CHECK( d.responses[0].text == "This is a repeated category manuals, food test response" );
    CHECK( d.responses[0].success.next_topic.item_type == itype_beer );
    CHECK( d.responses[1].text == "This is a repeated category manuals, food test response" );
    CHECK( d.responses[1].success.next_topic.item_type == itype_dnd_handbook );
    CHECK( d.responses[2].text == "This is a repeated category manuals, food test response" );
    CHECK( d.responses[2].success.next_topic.item_type == itype_manual_speech );
    CHECK( d.responses[3].text == "This is a repeated category manuals test response" );
    CHECK( d.responses[3].success.next_topic.item_type == itype_dnd_handbook );
    CHECK( d.responses[4].text == "This is a repeated category manuals test response" );
    CHECK( d.responses[4].success.next_topic.item_type == itype_manual_speech );
    CHECK( d.responses[5].text == "This is a repeated item beer, bottle_glass test response" );
    CHECK( d.responses[5].success.next_topic.item_type == itype_bottle_glass );
    CHECK( d.responses[6].text == "This is a repeated item beer, bottle_glass test response" );
    CHECK( d.responses[6].success.next_topic.item_type == itype_beer );
    CHECK( d.responses[7].text == "This is a basic test response." );

    // test sell and consume
    d.add_topic( "TALK_TEST_EFFECTS" );
    gen_response_lines( d, 19 );
    REQUIRE( has_item( player_character, "bottle_plastic", 1 ) );
    REQUIRE( has_beer_bottle( player_character, 2 ) );
    const std::vector<item *> glass_bottles = player_character.items_with( []( const item & it ) {
        return it.typeId() == itype_bottle_glass;
    } );
    REQUIRE( !glass_bottles.empty() );
    REQUIRE( player_character.wield( *glass_bottles.front() ) );
    effects = d.responses[14].success;
    effects.apply( d );
    CHECK_FALSE( has_item( player_character, "bottle_plastic", 1 ) );
    CHECK_FALSE( has_item( player_character, "beer", 1 ) );
    CHECK( has_item( talker_npc, "bottle_plastic", 1 ) );
    CHECK( has_item( talker_npc, "beer", 2 ) );
    effects = d.responses[15].success;
    effects.apply( d );
    CHECK_FALSE( has_item( talker_npc, "beer", 2 ) );
    CHECK( has_item( talker_npc, "beer", 1 ) );
    effects = d.responses[16].success;
    effects.apply( d );
    CHECK( has_item( player_character, "beer", 1 ) );
    effects = d.responses[17].success;
    effects.apply( d );
    CHECK( has_item( player_character, "beer", 0 ) );
    CHECK_FALSE( has_item( player_character, "beer", 1 ) );
}

TEST_CASE( "npc_talk_combat_commands", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    d.add_topic( "TALK_COMBAT_COMMANDS" );
    gen_response_lines( d, 10 );
    CHECK( d.responses[0].text == "Change your engagement rules…" );
    CHECK( d.responses[1].text == "Change your aiming rules…" );
    CHECK( d.responses[2].text == "Stick close to me, no matter what." );
    CHECK( d.responses[3].text == "<ally_rule_follow_distance_request_4_text>" );
    CHECK( d.responses[4].text == "Don't use ranged weapons anymore." );
    CHECK( d.responses[5].text == "Use only silent weapons." );
    CHECK( d.responses[6].text == "Don't use grenades anymore." );
    CHECK( d.responses[7].text == "Don't worry about shooting an ally." );
    CHECK( d.responses[8].text == "Hold the line: don't move onto obstacles adjacent to me." );
    CHECK( d.responses[9].text == "Never mind." );
}

TEST_CASE( "npc_talk_vars", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    d.add_topic( "TALK_TEST_VARS" );
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a u_add_var test response." );
    CHECK( d.responses[2].text == "This is a npc_add_var test response." );
    talk_effect_t &effects = d.responses[1].success;
    effects.apply( d );
    effects = d.responses[2].success;
    effects.apply( d );
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a u_has_var, u_remove_var test response." );
    CHECK( d.responses[2].text == "This is a npc_has_var, npc_remove_var test response." );
    effects = d.responses[1].success;
    effects.apply( d );
    effects = d.responses[2].success;
    effects.apply( d );
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a u_add_var test response." );
    CHECK( d.responses[2].text == "This is a npc_add_var test response." );
}

TEST_CASE( "npc_talk_adjust_vars", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    d.add_topic( "TALK_TEST_ADJUST_VARS" );

    // At the starting point, the var hasn't been set or adjusted, so it should default to 0.
    gen_response_lines( d, 11 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a u_adjust_var test response that increments by 1." );
    CHECK( d.responses[2].text == "This is a u_adjust_var test response that decrements by 1." );
    CHECK( d.responses[3].text == "This is a npc_adjust_var test response that increments by 1." );
    CHECK( d.responses[4].text == "This is a npc_adjust_var test response that decrements by 1." );
    CHECK( d.responses[5].text == "This is a u_compare_var test response for == 0." );
    CHECK( d.responses[6].text == "This is a u_compare_var test response for <= 0." );
    CHECK( d.responses[7].text == "This is a u_compare_var test response for >= 0." );
    CHECK( d.responses[8].text == "This is a npc_compare_var test response for == 0." );
    CHECK( d.responses[9].text == "This is a npc_compare_var test response for <= 0." );
    CHECK( d.responses[10].text == "This is a npc_compare_var test response for >= 0." );

    // Increment the u and npc vars by 1, so that it has a value of 1.
    talk_effect_t &effects = d.responses[1].success;
    effects.apply( d );
    effects = d.responses[3].success;
    effects.apply( d );

    // Now we're comparing the var, which should be 1, to our condition value which is 0.
    gen_response_lines( d, 11 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a u_adjust_var test response that increments by 1." );
    CHECK( d.responses[2].text == "This is a u_adjust_var test response that decrements by 1." );
    CHECK( d.responses[3].text == "This is a npc_adjust_var test response that increments by 1." );
    CHECK( d.responses[4].text == "This is a npc_adjust_var test response that decrements by 1." );
    CHECK( d.responses[5].text == "This is a u_compare_var test response for != 0." );
    CHECK( d.responses[6].text == "This is a u_compare_var test response for >= 0." );
    CHECK( d.responses[7].text == "This is a u_compare_var test response for > 0." );
    CHECK( d.responses[8].text == "This is a npc_compare_var test response for != 0." );
    CHECK( d.responses[9].text == "This is a npc_compare_var test response for >= 0." );
    CHECK( d.responses[10].text == "This is a npc_compare_var test response for > 0." );

    // Decrement the u and npc vars by 1 twice, so that it has a value of -1.
    effects = d.responses[2].success;
    effects.apply( d );
    effects.apply( d );
    effects = d.responses[4].success;
    effects.apply( d );
    effects.apply( d );

    // Now we're comparing the var, which should be -1, to our condition value which is 0.
    gen_response_lines( d, 11 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a u_adjust_var test response that increments by 1." );
    CHECK( d.responses[2].text == "This is a u_adjust_var test response that decrements by 1." );
    CHECK( d.responses[3].text == "This is a npc_adjust_var test response that increments by 1." );
    CHECK( d.responses[4].text == "This is a npc_adjust_var test response that decrements by 1." );
    CHECK( d.responses[5].text == "This is a u_compare_var test response for != 0." );
    CHECK( d.responses[6].text == "This is a u_compare_var test response for <= 0." );
    CHECK( d.responses[7].text == "This is a u_compare_var test response for < 0." );
    CHECK( d.responses[8].text == "This is a npc_compare_var test response for != 0." );
    CHECK( d.responses[9].text == "This is a npc_compare_var test response for <= 0." );
    CHECK( d.responses[10].text == "This is a npc_compare_var test response for < 0." );
}

TEST_CASE( "npc_talk_vars_time", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    time_point start_turn = calendar::turn;
    calendar::turn = calendar::turn + time_duration( 1_hours );
    d.add_topic( "TALK_TEST_VARS_TIME" );
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a u_add_var time test response." );
    CHECK( d.responses[2].text == "This is a npc_add_var time test response." );
    talk_effect_t &effects = d.responses[1].success;
    effects.apply( d );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    time_point then = calendar::turn;
    calendar::turn = calendar::turn + time_duration( 4_days );
    REQUIRE( then < calendar::turn );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a u_compare_var time test response for > 3_days." );
    calendar::turn = start_turn;
}

TEST_CASE( "npc_talk_bionics", "[npc_talk]" )
{
    dialogue d;
    npc &beta = prep_test( d );
    Character &player_character = get_avatar();

    player_character.clear_bionics();
    beta.clear_bionics();
    d.add_topic( "TALK_TEST_BIONICS" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    player_character.add_bionic( bio_ads );
    beta.add_bionic( bio_power_storage );
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a u_has_bionics bio_ads test response." );
    CHECK( d.responses[2].text == "This is a npc_has_bionics ANY response." );
}

TEST_CASE( "npc_faction_trust", "[npc_talk]" )
{
    dialogue d;
    npc &beta = prep_test( d, true );
    Character &player_character = get_avatar();

    player_character.get_faction()->trusts_u = 0;
    beta.get_faction()->trusts_u = 0;
    d.add_topic( "TALK_TEST_FACTION_TRUST" );
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "Add 50 to faction trust." );
    CHECK( d.responses[2].text == "Start trade." );
    talk_effect_t &effects = d.responses[1].success;
    effects.apply( d );
    REQUIRE( beta.get_faction()->trusts_u == 50 );
    gen_response_lines( d, 4 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "Faction trust is at least 20." );
    CHECK( d.responses[2].text == "Add 50 to faction trust." );
    CHECK( d.responses[3].text == "Start trade." );
}

TEST_CASE( "test_talk_meta", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    d.add_topic( "TALK_TEST_META" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "Mod TESTING DATA is loaded." );
}

TEST_CASE( "npc_talk_effects", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );
    Character &player_character = get_avatar();

    // speaker effects just use are owed because I don't want to do anything complicated
    player_character.cash = 1000;
    talker_npc.op_of_u.owed = 2000;
    CHECK( talker_npc.op_of_u.owed == 2000 );
    d.add_topic( "TALK_TEST_SPEAKER_EFFECT_SIMPLE" );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 1500 );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 1000 );
    d.add_topic( "TALK_TEST_SPEAKER_EFFECT_SIMPLE_CONDITIONAL" );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 500 );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 0 );
    talker_npc.op_of_u.owed = 2000;
    CHECK( talker_npc.op_of_u.owed == 2000 );
    d.add_topic( "TALK_TEST_SPEAKER_EFFECT_SENTINEL" );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 1500 );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 1500 );
    d.add_topic( "TALK_TEST_SPEAKER_EFFECT_SENTINEL_CONDITIONAL" );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 1000 );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 1000 );

    talker_npc.op_of_u.owed = 4500;
    CHECK( talker_npc.op_of_u.owed == 4500 );
    d.add_topic( "TALK_TEST_SPEAKER_EFFECT_COMPOUND" );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 3500 );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 2500 );
    d.add_topic( "TALK_TEST_SPEAKER_EFFECT_COMPOUND_CONDITIONAL" );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 1000 );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 500 );
    talker_npc.op_of_u.owed = 3500;
    CHECK( talker_npc.op_of_u.owed == 3500 );
    d.add_topic( "TALK_TEST_SPEAKER_EFFECT_COMPOUND_SENTINEL" );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 2750 );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 2250 );
    d.add_topic( "TALK_TEST_SPEAKER_EFFECT_COMPOUND_SENTINEL_CONDITIONAL" );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 1500 );
    d.apply_speaker_effects( d.topic_stack.back() );
    REQUIRE( talker_npc.op_of_u.owed == 1500 );

    // test change class
    talker_npc.myclass = NC_TEST_CLASS;
    d.add_topic( "TALK_TEST_EFFECTS" );
    gen_response_lines( d, 19 );
    talk_effect_t &effects = d.responses[18].success;
    effects.apply( d );
    CHECK( talker_npc.myclass == NC_NONE );
}

TEST_CASE( "npc_change_topic", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );

    const std::string original_chat = talker_npc.chatbin.first_topic;
    REQUIRE( original_chat != "TALK_TEST_SET_TOPIC" );
    d.add_topic( "TALK_TEST_SET_TOPIC" );
    gen_response_lines( d, 2 );
    talk_effect_t &effects = d.responses[1].success;
    effects.apply( d );
    CHECK( talker_npc.chatbin.first_topic != original_chat );
    CHECK( talker_npc.chatbin.first_topic == "TALK_TEST_SET_TOPIC" );
}

TEST_CASE( "npc_compare_int_op", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    d.add_topic( "TALK_TEST_COMPARE_INT_OP" );
    gen_response_lines( d, 10 );
    CHECK( d.responses[ 0 ].text == "Two != five." );
    CHECK( d.responses[ 1 ].text == "Two <= five." );
    CHECK( d.responses[ 2 ].text == "Two < five." );
    CHECK( d.responses[ 3 ].text == "Five = five." );
    CHECK( d.responses[ 4 ].text == "Five == five." );
    CHECK( d.responses[ 5 ].text == "Five <= five." );
    CHECK( d.responses[ 6 ].text == "Five >= five." );
    CHECK( d.responses[ 7 ].text == "Five != two." );
    CHECK( d.responses[ 8 ].text == "Five >= two." );
    CHECK( d.responses[ 9 ].text == "Five > two." );
}

TEST_CASE( "npc_test_tags", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    global_variables &globvars = get_globals();
    globvars.set_global_value( "npctalk_var_test_var", "It's global" );

    d.add_topic( "TALK_TEST_TAGS" );
    gen_response_lines( d, 7 );
    CHECK( d.responses[0].create_option_line( d, input_event() ).text ==
           "Avatar tag is set to It's avatar." );
    CHECK( d.responses[1].create_option_line( d, input_event() ).text ==
           "NPC tag is set to It's npc." );
    CHECK( d.responses[2].create_option_line( d, input_event() ).text ==
           "Global tag is set to It's global." );
    CHECK( d.responses[3].create_option_line( d, input_event() ).text ==
           "Item name is TEST rock." );
    CHECK( d.responses[4].create_option_line( d, input_event() ).text ==
           "Item description is A rock the size of a baseball.  Makes a decent melee weapon, and is also good for throwing at enemies." );
    CHECK( d.responses[5].create_option_line( d, input_event() ).text ==
           "Trait name is Ink glands." );
    CHECK( d.responses[6].create_option_line( d, input_event() ).text ==
           "Trait description is A mutation to test enchantments." );
    globvars.clear_global_values();
}

TEST_CASE( "npc_compare_int", "[npc_talk]" )
{
    dialogue d;
    npc &beta = prep_test( d );
    Character &player_character = get_avatar();

    player_character.str_cur = 4;
    player_character.dex_cur = 4;
    player_character.int_cur = 4;
    player_character.per_cur = 4;
    beta.str_cur = 8;
    beta.dex_cur = 8;
    beta.int_cur = 8;
    beta.per_cur = 8;
    player_character.kill_xp = 50;
    for( npc *guy : g->allies() ) {
        talk_function::leave( *guy );
    }
    player_character.cash = 0;
    beta.op_of_u.owed = 0;
    const skill_id skill = skill_driving;
    player_character.set_skill_level( skill, 0 );

    get_weather().temperature = units::from_fahrenheit( 19 );
    get_weather().windspeed = 20;
    get_weather().weather_precise->temperature = units::from_fahrenheit( 19 );
    get_weather().weather_precise->windpower = 20;
    get_weather().weather_precise->humidity = 20;
    get_weather().weather_precise->pressure = 20;
    get_weather().clear_temp_cache();
    player_character.set_stored_kcal( 118169 );
    player_character.remove_items_with( []( const item & it ) {
        return it.get_category_shallow().get_id() == item_category_manual ||
               it.get_category_shallow().get_id() == item_category_food ||
               it.typeId() == itype_bottle_glass;
    } );
    player_character.remove_value( "npctalk_var_test_var_time_test_test" );
    calendar::turn = calendar::turn_zero;

    int expected_answers = 8;
    if( player_character.magic->max_mana( player_character ) == 900 ) {
        expected_answers++;
    }

    d.add_topic( "TALK_TEST_COMPARE_INT" );
    gen_response_lines( d, expected_answers );
    CHECK( d.responses[ 0 ].text == "This is a u_adjust_var test response that increments by 1." );
    CHECK( d.responses[ 1 ].text == "This is an npc_adjust_var test response that increments by 2." );
    CHECK( d.responses[ 2 ].text == "This is a u_add_var time test response." );
    CHECK( d.responses[expected_answers - 2].text == "Exp of Pew, Pew is -1." );
    CHECK( d.responses[expected_answers - 1].text ==
           "Test Proficiency total learning time is 24 hours." );

    beta.str_cur = 9;
    beta.dex_cur = 10;
    beta.int_cur = 11;
    beta.per_cur = 12;
    time_point then = calendar::turn;
    calendar::turn = calendar::turn + time_duration( 4_days );
    REQUIRE( then < calendar::turn );
    // Increment the u var by 1, so that it has a value of 1.
    talk_effect_t &effects = d.responses[ 0 ].success;
    effects.apply( d );
    // Increment the npc var by 2, so that it has a value of 2.
    effects = d.responses[ 1 ].success;
    effects.apply( d );
    // Create a u time variable.
    effects = d.responses[ 2 ].success;
    effects.apply( d );
    talk_function::follow( beta );
    player_character.cash = 13;
    beta.op_of_u.owed = 14;
    player_character.set_skill_level( skill, 8 );
    get_weather().weather_precise->temperature = units::from_fahrenheit( 21 );
    get_weather().weather_precise->windpower = 15;
    get_weather().weather_precise->humidity = 16;
    get_weather().weather_precise->pressure = 17;
    get_weather().clear_temp_cache();
    player_character.setpos( tripoint( 18, 19, 20 ) );
    player_character.set_pain( 21 );
    player_character.add_bionic( bio_power_storage );
    player_character.set_power_level( 22_mJ );
    player_character.set_max_power_level( 44_mJ );
    player_character.clear_morale();
    player_character.add_morale( MORALE_HAIRCUT, 23 );
    player_character.set_hunger( 26 );
    player_character.set_thirst( 27 );
    player_character.set_stored_kcal( 118169 );
    player_character.worn.wear_item( player_character, item( "backpack" ), false, false );
    player_character.inv->add_item( item( itype_bottle_glass ) );
    player_character.inv->add_item( item( itype_bottle_glass ) );
    player_character.inv->add_item( item( itype_bottle_glass ) );
    cata::event e = cata::event::make<event_type::character_kills_monster>(
                        get_player_character().getID(), mon_zombie_bio_op );
    get_event_bus().send( e );
    player_character.magic->learn_spell( spell_test_spell_json, player_character, false );
    player_character.set_mutation( trait_test_trait ); // Give the player the spell scool test_trait
    player_character.magic->set_spell_level( spell_test_spell_json, 1, &player_character );
    player_character.magic->learn_spell( spell_test_spell_pew, player_character, true );
    player_character.magic->set_spell_level( spell_test_spell_pew, 4, &player_character );
    player_character.magic->learn_spell( spell_test_spell_lava, player_character, true );
    player_character.magic->set_spell_level( spell_test_spell_lava, 12, &player_character );
    player_character.set_proficiency_practice( proficiency_prof_test, 12_hours );
    // Set focus after killing monster, since the character
    // gains weakpoint proficiency practice which lowers focus
    // (see kill_tracker::notify() -> weakpoint_families::practice_kill())
    player_character.set_focus( 24 );
    player_character.str_cur = 5;
    player_character.dex_cur = 6;
    player_character.int_cur = 7;
    player_character.per_cur = 8;
    player_character.magic->set_mana( 25 );

    expected_answers = 51;
    gen_response_lines( d, expected_answers );
    CHECK( d.responses[ 0 ].text == "This is a u_adjust_var test response that increments by 1." );
    CHECK( d.responses[ 1 ].text == "This is an npc_adjust_var test response that increments by 2." );
    CHECK( d.responses[ 2 ].text == "PC strength is five." );
    CHECK( d.responses[ 3 ].text == "PC dexterity is six." );
    CHECK( d.responses[ 4 ].text == "PC intelligence is seven." );
    CHECK( d.responses[ 5 ].text == "PC perception is eight." );
    CHECK( d.responses[ 6 ].text == "NPC strength is nine." );
    CHECK( d.responses[ 7 ].text == "NPC dexterity is ten." );
    CHECK( d.responses[ 8 ].text == "NPC intelligence is eleven." );
    CHECK( d.responses[ 9 ].text == "NPC perception is twelve." );
    CHECK( d.responses[ 10 ].text == "PC Custom var is one." );
    CHECK( d.responses[ 11 ].text == "NPC Custom var is two." );
    CHECK( d.responses[ 12 ].text == "This is a u_var time test response for > 3_days." );
    CHECK( d.responses[ 13 ].text == "time_since_cataclysm > 3_days." );
    CHECK( d.responses[ 14 ].text == "time_since_cataclysm in days > 3" );
    CHECK( d.responses[ 15 ].text == "Allies equals 1" );
    CHECK( d.responses[ 16 ].text == "Cash equals 13" );
    CHECK( d.responses[ 17 ].text == "Owed amount equals 14" );
    CHECK( d.responses[ 18 ].text == "Driving skill more than or equal to 5" );
    // TODO: Reliably test the random number generator.
    CHECK( d.responses[ 19 ].text == "Temperature is 21." );
    CHECK( d.responses[ 20 ].text == "Windpower is 15." );
    CHECK( d.responses[ 21 ].text == "Humidity is 16." );
    CHECK( d.responses[ 22 ].text == "Pressure is 17." );
    CHECK( d.responses[ 23 ].text == "Pos_x is 18." );
    CHECK( d.responses[ 24 ].text == "Pos_y is 19." );
    CHECK( d.responses[ 25 ].text == "Pos_z is 20. This should be cause for alarm." );
    CHECK( d.responses[ 26 ].text == "Pain level is 21." );
    CHECK( d.responses[ 27 ].text == "Bionic power is 22." );
    CHECK( d.responses[ 28 ].text == "Bionic power max is 44." );
    CHECK( d.responses[ 29 ].text == "Bionic power is at 50%." );
    CHECK( d.responses[ 30 ].text == "Morale is 23." );
    CHECK( d.responses[ 31 ].text == "Focus is 24." );
    CHECK( d.responses[ 32 ].text == "Mana is 25." );
    CHECK( d.responses[ 33 ].text == "Mana max is 900." );
    CHECK( d.responses[ 34 ].text == "Mana is at 2%." );
    CHECK( d.responses[ 35 ].text == "Hunger is 26." );
    CHECK( d.responses[ 36 ].text == "Thirst is 27." );
    CHECK( d.responses[ 37 ].text == "Stored kcal is 118'169." );
    CHECK( d.responses[ 38 ].text == "Stored kcal is at 100% of healthy." );
    CHECK( d.responses[ 39 ].text == "Has 3 glass bottles." );
    CHECK( d.responses[ 40 ].text == "Has more or equal to 35 experience." );
    CHECK( d.responses[ 41 ].text == "Highest spell level in school test_trait is 1." );
    CHECK( d.responses[ 42 ].text == "Spell level of Pew, Pew is 4." );
    CHECK( d.responses[ 43 ].text == "Spell level of highest spell is 12." );
    CHECK( d.responses[ 44 ].text == "Exp of Pew, Pew is 11006." );
    CHECK( d.responses[ 45 ].text == "Test Proficiency learning is 5 out of 10." );
    CHECK( d.responses[ 46 ].text == "Test Proficiency learning done is 12 hours total." );
    CHECK( d.responses[ 47 ].text == "Test Proficiency learning is 50% learnt." );
    CHECK( d.responses[ 48 ].text == "Test Proficiency learning is 500 permille learnt." );
    CHECK( d.responses[ 49 ].text == "Test Proficiency total learning time is 24 hours." );
    CHECK( d.responses[ 50 ].text == "Test Proficiency time lest to learn is 12h." );

    calendar::turn = calendar::turn + time_duration( 4_days );
    expected_answers++;
    gen_response_lines( d, expected_answers );
    CHECK( d.responses[ 15 ].text == "This is a time since u_var test response for > 3_days." );

    // Teardown
    player_character.remove_value( "npctalk_var_test_var_time_test_test" );
}

TEST_CASE( "npc_arithmetic_op", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    d.add_topic( "TALK_TEST_ARITHMETIC_OP" );
    gen_response_lines( d, 14 );

    calendar::turn = calendar::turn_zero;
    REQUIRE( calendar::turn == time_point( 0 ) );
    // "Sets time since cataclysm to 2 * 5 turns.  (10)"
    talk_effect_t &effects = d.responses[ 0 ].success;
    effects.apply( d );
    CHECK( calendar::turn == time_point( 10 ) );

    calendar::turn = calendar::turn_zero;
    // "Sets time since cataclysm to 15 / 5 turns.  (3)"
    effects = d.responses[ 1 ].success;
    effects.apply( d );
    CHECK( calendar::turn == time_point( 3 ) );

    calendar::turn = calendar::turn_zero;
    // "Sets time since cataclysm to 2 + 5 turns.  (7)"
    effects = d.responses[ 2 ].success;
    effects.apply( d );
    CHECK( calendar::turn == time_point( 7 ) );

    calendar::turn = calendar::turn_zero;
    // "Sets time since cataclysm to 5 - 2 turns.  (3)"
    effects = d.responses[ 3 ].success;
    effects.apply( d );
    CHECK( calendar::turn == time_point( 3 ) );

    calendar::turn = calendar::turn_zero;
    // "Sets time since cataclysm to 15 % 10 turns.  (5)"
    effects = d.responses[ 4 ].success;
    effects.apply( d );
    CHECK( calendar::turn == time_point( 5 ) );

    calendar::turn = calendar::turn_zero;
    // "Sets time since cataclysm to 2 ^ 5 turns.  (32)"
    effects = d.responses[ 5 ].success;
    effects.apply( d );
    CHECK( calendar::turn == time_point( 32 ) );

    calendar::turn = calendar::turn_zero;
    // "Sets time since cataclysm to 5 turns.  (5)"
    effects = d.responses[ 6 ].success;
    effects.apply( d );
    CHECK( calendar::turn == time_point( 5 ) );

    calendar::turn = time_point( 5 );
    // "Sets time since cataclysm to *= 5 turns."
    effects = d.responses[ 7 ].success;
    effects.apply( d );
    CHECK( calendar::turn == time_point( 25 ) );

    calendar::turn = time_point( 5 );
    // "Sets time since cataclysm to /= 5 turns."
    effects = d.responses[ 8 ].success;
    effects.apply( d );
    CHECK( calendar::turn == time_point( 1 ) );

    calendar::turn = time_point( 5 );
    // "Sets time since cataclysm to += 5 turns."
    effects = d.responses[ 9 ].success;
    effects.apply( d );
    CHECK( calendar::turn == time_point( 10 ) );

    calendar::turn = time_point( 11 );
    // "Sets time since cataclysm to -= 5 turns."
    effects = d.responses[ 10 ].success;
    effects.apply( d );
    CHECK( calendar::turn == time_point( 6 ) );

    calendar::turn = time_point( 17 );
    // "Sets time since cataclysm to %= 5 turns."
    effects = d.responses[ 11 ].success;
    effects.apply( d );
    CHECK( calendar::turn == time_point( 2 ) );

    calendar::turn = time_point( 5 );
    // "Sets time since cataclysm++."
    effects = d.responses[ 12 ].success;
    effects.apply( d );
    CHECK( calendar::turn == time_point( 6 ) );

    calendar::turn = time_point( 5 );
    // "Sets time since cataclysm--."
    effects = d.responses[ 13 ].success;
    effects.apply( d );
    CHECK( calendar::turn == time_point( 4 ) );
}

TEST_CASE( "npc_arithmetic", "[npc_talk]" )
{
    dialogue d;
    npc &beta = prep_test( d );
    Character &player_character = get_avatar();

    d.add_topic( "TALK_TEST_ARITHMETIC" );
    gen_response_lines( d, 32 );

    calendar::turn = calendar::turn_zero;
    REQUIRE( calendar::turn == time_point( 0 ) );
    // "Sets time since cataclysm to 1."
    talk_effect_t &effects = d.responses[ 0 ].success;
    effects.apply( d );
    CHECK( calendar::turn == time_point( 1 ) );

    get_weather().weather_precise->temperature = units::from_fahrenheit( 20 );
    get_weather().clear_temp_cache();
    // "Sets temperature to 2."
    effects = d.responses[ 1 ].success;
    effects.apply( d );
    CHECK( units::to_fahrenheit( get_weather().weather_precise->temperature ) == Approx( 2 ).margin(
               0.01 ) );

    get_weather().weather_precise->windpower = 20;
    get_weather().clear_temp_cache();
    // "Sets windpower to 3."
    effects = d.responses[ 2 ].success;
    effects.apply( d );
    CHECK( get_weather().weather_precise->windpower == 3 );

    get_weather().weather_precise->humidity = 20;
    get_weather().clear_temp_cache();
    // "Sets humidity to 4."
    effects = d.responses[ 3 ].success;
    effects.apply( d );
    CHECK( get_weather().weather_precise->humidity == 4 );

    get_weather().weather_precise->pressure = 20;
    get_weather().clear_temp_cache();
    // "Sets pressure to 5."
    effects = d.responses[ 4 ].success;
    effects.apply( d );
    CHECK( get_weather().weather_precise->pressure == 5 );

    player_character.str_max = 10;
    // "Sets base strength to 6."
    effects = d.responses[ 5 ].success;
    effects.apply( d );
    CHECK( player_character.str_max == 6 );

    player_character.dex_max = 10;
    // "Sets base dexterity to 7."
    effects = d.responses[ 6 ].success;
    effects.apply( d );
    CHECK( player_character.dex_max == 7 );

    player_character.int_max = 10;
    // "Sets base intelligence to 8."
    effects = d.responses[ 7 ].success;
    effects.apply( d );
    CHECK( player_character.int_max == 8 );

    player_character.per_max = 10;
    // "Sets base perception to 9."
    effects = d.responses[ 8 ].success;
    effects.apply( d );
    CHECK( player_character.per_max == 9 );

    std::string var_name = "npctalk_var_test_var_time_test_test";
    player_character.set_value( var_name, std::to_string( 1 ) );
    // "Sets custom var to 10."
    effects = d.responses[ 9 ].success;
    effects.apply( d );
    CHECK( std::stoi( player_character.get_value( var_name ) ) == 10 );

    calendar::turn = time_point( 33 );
    // "Sets time since var to 11."
    effects = d.responses[ 10 ].success;
    effects.apply( d );
    CHECK( std::stoi( player_character.get_value( var_name ) ) == 22 );

    beta.op_of_u.owed = 0;
    // "Sets owed to 12."
    effects = d.responses[ 11 ].success;
    effects.apply( d );
    CHECK( beta.op_of_u.owed == 12 );

    const skill_id skill = skill_driving;
    // "Sets skill level in driving to 10."
    effects = d.responses[ 12 ].success;
    effects.apply( d );
    CHECK( player_character.get_skill_level( skill ) == 10 );

    // "Sets pos_x to 14."
    effects = d.responses[ 13 ].success;
    effects.apply( d );
    CHECK( player_character.posx() == 14 );

    // "Sets pos_y to 15."
    effects = d.responses[ 14 ].success;
    effects.apply( d );
    CHECK( player_character.posy() == 15 );

    // "Sets pos_z to 16."
    effects = d.responses[ 15 ].success;
    effects.apply( d );
    CHECK( player_character.posz() == 16 );

    // "Sets pain to 17."
    effects = d.responses[ 16 ].success;
    effects.apply( d );
    CHECK( player_character.get_pain() == 17 );

    player_character.add_bionic( bio_power_storage );
    player_character.set_power_level( 10_mJ );
    player_character.set_max_power_level( 44_mJ );
    // "Sets power to 18."
    effects = d.responses[ 17 ].success;
    effects.apply( d );
    CHECK( player_character.get_power_level().value() == 18 );

    // "Sets power to 20%."
    effects = d.responses[ 18 ].success;
    effects.apply( d );
    CHECK( player_character.get_power_level().value() == 8 );

    // "Sets focus to 19."
    effects = d.responses[ 19 ].success;
    effects.apply( d );
    CHECK( player_character.get_focus() == 19 );

    // "Sets mana to 21."
    effects = d.responses[ 20 ].success;
    effects.apply( d );
    CHECK( player_character.magic->available_mana() == 21 );

    // "Sets mana to 25%."
    effects = d.responses[ 21 ].success;
    effects.apply( d );
    CHECK( player_character.magic->available_mana() == ( player_character.magic->max_mana(
                player_character ) * 25 ) / 100 );

    // "Sets thirst to 22."
    effects = d.responses[ 22 ].success;
    effects.apply( d );
    CHECK( player_character.get_thirst() == 22 );

    // "Sets stored_kcal to 23."
    effects = d.responses[ 23 ].success;
    effects.apply( d );
    CHECK( player_character.get_stored_kcal() == 23 );

    // "Sets stored_kcal_percentage to 50."
    effects = d.responses[ 24 ].success;
    effects.apply( d );
    // this should be player_character.get_healthy_kcal() instead of 550000 but for whatever reason it is hardcoded to that value??
    CHECK( player_character.get_stored_kcal() == 550000 / 2 );

    // Spell tests setup
    if( player_character.magic->knows_spell( spell_test_spell_pew ) ) {
        player_character.magic->forget_spell( spell_test_spell_pew );
    }
    CHECK( player_character.magic->knows_spell( spell_test_spell_pew ) == false );

    // "Sets pew pew's level to -1."
    effects = d.responses[25].success;
    effects.apply( d );
    CHECK( player_character.magic->knows_spell( spell_test_spell_pew ) == false );

    // "Sets pew pew's level to 4."
    effects = d.responses[26].success;
    effects.apply( d );
    CHECK( player_character.magic->knows_spell( spell_test_spell_pew ) == true );
    CHECK( player_character.magic->get_spell( spell_test_spell_pew ).get_level() == 4 );

    // "Sets pew pew's level to -1."
    effects = d.responses[25].success;
    effects.apply( d );
    CHECK( player_character.magic->knows_spell( spell_test_spell_pew ) == false );

    // "Sets pew pew's exp to 11006."
    effects = d.responses[28].success;
    effects.apply( d );
    CHECK( player_character.magic->knows_spell( spell_test_spell_pew ) == true );
    CHECK( player_character.magic->get_spell( spell_test_spell_pew ).get_level() == 4 );

    // "Sets pew pew's exp to -1."
    effects = d.responses[27].success;
    effects.apply( d );
    CHECK( player_character.magic->knows_spell( spell_test_spell_pew ) == false );

    // Setup proficiency tests
    if( player_character.has_proficiency( proficiency_prof_test ) ) {
        player_character.lose_proficiency( proficiency_prof_test, true );
    }
    std::vector<proficiency_id> proficiencies_vector = player_character.learning_proficiencies();
    if( std::count( proficiencies_vector.begin(),
                    proficiencies_vector.end(),
                    proficiency_prof_test ) != 0 ) {
        player_character.set_proficiency_practiced_time( proficiency_prof_test, -1 );
    }

    // "Sets Test Proficiency learning done to -1."
    effects = d.responses[30].success;
    effects.apply( d );
    CHECK( player_character.has_proficiency( proficiency_prof_test ) == false );
    proficiencies_vector = player_character.learning_proficiencies();
    CHECK( std::count( proficiencies_vector.begin(),
                       proficiencies_vector.end(),
                       proficiency_prof_test ) == 0 );

    // "Sets Test Proficiency learning done to 24h."
    effects = d.responses[31].success;
    effects.apply( d );
    CHECK( player_character.has_proficiency( proficiency_prof_test ) == true );
    proficiencies_vector = player_character.learning_proficiencies();
    CHECK( std::count( proficiencies_vector.begin(),
                       proficiencies_vector.end(),
                       proficiency_prof_test ) == 0 );

    // "Sets Test Proficiency learning done to 12 hours total."
    effects = d.responses[29].success;
    effects.apply( d );
    CHECK( player_character.has_proficiency( proficiency_prof_test ) == false );
    proficiencies_vector = player_character.learning_proficiencies();
    CHECK( std::count( proficiencies_vector.begin(),
                       proficiencies_vector.end(),
                       proficiency_prof_test ) != 0 );

    // "Sets Test Proficiency learning done to -1."
    effects = d.responses[30].success;
    effects.apply( d );
    CHECK( player_character.has_proficiency( proficiency_prof_test ) == false );
    proficiencies_vector = player_character.learning_proficiencies();
    CHECK( std::count( proficiencies_vector.begin(),
                       proficiencies_vector.end(),
                       proficiency_prof_test ) == 0 );

    // Teardown
    player_character.remove_value( var_name );
}
