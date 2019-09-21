#include <stdio.h>
#include <string>
#include <memory>
#include <vector>

#include "avatar.h"
#include "catch/catch.hpp"
#include "calendar.h"
#include "coordinate_conversions.h"
#include "dialogue.h"
#include "effect.h"
#include "faction.h"
#include "game.h"
#include "item_category.h"
#include "map.h"
#include "mission.h"
#include "npc.h"
#include "overmapbuffer.h"
#include "player.h"
#include "player_helpers.h"
#include "character.h"
#include "inventory.h"
#include "item.h"
#include "pimpl.h"
#include "string_id.h"
#include "npctalk.h"
#include "mapdata.h"
#include "material.h"
#include "type_id.h"
#include "point.h"

const efftype_id effect_gave_quest_item( "gave_quest_item" );
const efftype_id effect_currently_busy( "currently_busy" );
const efftype_id effect_infection( "infection" );
const efftype_id effect_infected( "infected" );
static const trait_id trait_PROF_FED( "PROF_FED" );
static const trait_id trait_PROF_SWAT( "PROF_SWAT" );

static npc &create_test_talker()
{
    const string_id<npc_template> test_talker( "test_talker" );
    const int model_id = g->m.place_npc( point( 25, 25 ), test_talker, true );
    g->load_npcs();

    npc *model_npc = g->find_npc( model_id );
    REQUIRE( model_npc != nullptr );

    for( const trait_id &tr : model_npc->get_mutations() ) {
        model_npc->unset_mutation( tr );
    }
    model_npc->set_hunger( 0 );
    model_npc->set_thirst( 0 );
    model_npc->set_fatigue( 0 );
    model_npc->remove_effect( efftype_id( "sleep" ) );
    // An ugly hack to prevent NPC falling asleep during testing due to massive fatigue
    model_npc->set_mutation( trait_id( "WEB_WEAVER" ) );

    return *model_npc;
}

static void gen_response_lines( dialogue &d, size_t expected_count )
{
    d.gen_responses( d.topic_stack.back() );
    for( talk_response &response : d.responses ) {
        response.create_option_line( d, ' ' );
    }
    if( d.responses.size() != expected_count ) {
        printf( "Test failure in %s\n", d.topic_stack.back().id.c_str() );
        for( talk_response &response : d.responses ) {
            printf( "response: %s\n", response.text.c_str() );
        }
    }
    REQUIRE( d.responses.size() == expected_count );
}

static void change_om_type( const std::string &new_type )
{
    const tripoint omt_pos = ms_to_omt_copy( g->m.getabs( g->u.pos() ) );
    oter_id &omt_ref = overmap_buffer.ter( omt_pos );
    omt_ref = oter_id( new_type );
}

static npc &prep_test( dialogue &d )
{
    clear_player();
    CHECK( !g->u.in_vehicle );
    const tripoint test_origin( 15, 15, 0 );
    g->u.setpos( test_origin );

    g->faction_manager_ptr->create_if_needed();

    npc &talker_npc = create_test_talker();

    d.alpha = &g->u;
    d.beta = &talker_npc;

    return talker_npc;
}

TEST_CASE( "npc_talk_start", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    d.add_topic( "TALK_TEST_START" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
}

TEST_CASE( "npc_talk_stats", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    g->u.str_cur = 8;
    g->u.dex_cur = 8;
    g->u.int_cur = 8;
    g->u.per_cur = 8;

    d.add_topic( "TALK_TEST_SIMPLE_STATS" );
    gen_response_lines( d, 5 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a strength test response." );
    CHECK( d.responses[2].text == "This is a dexterity test response." );
    CHECK( d.responses[3].text == "This is an intelligence test response." );
    CHECK( d.responses[4].text == "This is a perception test response." );
    g->u.str_cur = 6;
    g->u.dex_cur = 6;
    g->u.int_cur = 6;
    g->u.per_cur = 6;
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

    const skill_id skill( "driving" );

    g->u.set_skill_level( skill, 8 );

    d.add_topic( "TALK_TEST_SIMPLE_SKILLS" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a driving test response." );

    g->u.set_skill_level( skill, 6 );
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

    for( trait_id tr : g->u.get_mutations() ) {
        g->u.unset_mutation( tr );
    }

    d.add_topic( "TALK_TEST_WEARING_AND_TRAIT" );
    gen_response_lines( d, 1 );

    CHECK( d.responses[0].text == "This is a basic test response." );
    g->u.toggle_trait( trait_id( "ELFA_EARS" ) );
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a trait test response." );
    CHECK( d.responses[2].text == "This is a short trait test response." );
    g->u.wear_item( item( "badge_marshal" ) );
    gen_response_lines( d, 4 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a trait test response." );
    CHECK( d.responses[2].text == "This is a short trait test response." );
    CHECK( d.responses[3].text == "This is a wearing test response." );
    talker_npc.toggle_trait( trait_id( "ELFA_EARS" ) );
    gen_response_lines( d, 6 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a trait test response." );
    CHECK( d.responses[2].text == "This is a short trait test response." );
    CHECK( d.responses[3].text == "This is a wearing test response." );
    CHECK( d.responses[4].text == "This is a npc trait test response." );
    CHECK( d.responses[5].text == "This is a npc short trait test response." );
    g->u.toggle_trait( trait_id( "ELFA_EARS" ) );
    talker_npc.toggle_trait( trait_id( "ELFA_EARS" ) );
    g->u.toggle_trait( trait_id( "PSYCHOPATH" ) );
    talker_npc.toggle_trait( trait_id( "SAPIOVORE" ) );
    gen_response_lines( d, 4 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a wearing test response." );
    CHECK( d.responses[2].text == "This is a trait flags test response." );
    CHECK( d.responses[3].text == "This is a npc trait flags test response." );
    g->u.toggle_trait( trait_id( "PSYCHOPATH" ) );
    talker_npc.toggle_trait( trait_id( "SAPIOVORE" ) );
}

TEST_CASE( "npc_talk_effect", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );

    d.add_topic( "TALK_TEST_EFFECT" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    talker_npc.add_effect( effect_gave_quest_item, 9999_turns );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an npc effect test response." );
    g->u.add_effect( effect_gave_quest_item, 9999_turns );
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

    d.add_topic( "TALK_TEST_SERVICE" );
    g->u.cash = 0;
    talker_npc.add_effect( effect_currently_busy, 9999_turns );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    g->u.cash = 800;
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

    change_om_type( "pond_swamp_north" );
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
    talker_npc.myclass = npc_class_id( "NC_NONE" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    talker_npc.myclass = npc_class_id( "NC_TEST_CLASS" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a class test response." );
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
    talker_npc.rules.engagement = ENGAGE_ALL;
    talker_npc.rules.aim = AIM_SPRAY;
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
    talker_npc.set_fatigue( EXHAUSTED );
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
    CHECK( d.responses[1].text == "This is a days since cataclysm 30 test response." );
    CHECK( d.responses[2].text == "This is a season summer test response." );
    calendar::turn += calendar::season_length();
    gen_response_lines( d, 4 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a days since cataclysm 30 test response." );
    CHECK( d.responses[2].text == "This is a days since cataclysm 120 test response." );
    CHECK( d.responses[3].text == "This is a season autumn test response." );
    calendar::turn += calendar::season_length();
    gen_response_lines( d, 5 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a days since cataclysm 30 test response." );
    CHECK( d.responses[2].text == "This is a days since cataclysm 120 test response." );
    CHECK( d.responses[3].text == "This is a days since cataclysm 210 test response." );
    CHECK( d.responses[4].text == "This is a season winter test response." );
    calendar::turn += calendar::season_length();
    gen_response_lines( d, 6 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a season spring test response." );
    CHECK( d.responses[2].text == "This is a days since cataclysm 30 test response." );
    CHECK( d.responses[3].text == "This is a days since cataclysm 120 test response." );
    CHECK( d.responses[4].text == "This is a days since cataclysm 210 test response." );
    CHECK( d.responses[5].text == "This is a days since cataclysm 300 test response." );
    calendar::turn = old_calendar;
}

TEST_CASE( "npc_talk_time", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    const time_point old_calendar = calendar::turn;
    calendar::turn = to_turn<int>( sunrise( calendar::turn ) + 4_hours );
    d.add_topic( "TALK_TEST_TIME" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a is day test response." );
    calendar::turn = to_turn<int>( sunset( calendar::turn ) + 2_hours );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a is night test response." );
    calendar::turn = old_calendar;
}

TEST_CASE( "npc_talk_switch", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    d.add_topic( "TALK_TEST_SWITCH" );
    g->u.cash = 1000;
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an switch 1 test response." );
    CHECK( d.responses[2].text == "This is another basic test response." );
    g->u.cash = 100;
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an switch 2 test response." );
    CHECK( d.responses[2].text == "This is another basic test response." );
    g->u.cash = 10;
    gen_response_lines( d, 4 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an switch default 1 test response." );
    CHECK( d.responses[2].text == "This is an switch default 2 test response." );
    CHECK( d.responses[3].text == "This is another basic test response." );
    g->u.cash = 0;
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an switch default 2 test response." );
    CHECK( d.responses[2].text == "This is another basic test response." );
}

TEST_CASE( "npc_talk_or", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );

    d.add_topic( "TALK_TEST_OR" );
    g->u.cash = 0;
    talker_npc.add_effect( effect_currently_busy, 9999_turns );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    g->u.toggle_trait( trait_id( "ELFA_EARS" ) );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an or trait test response." );
}

TEST_CASE( "npc_talk_and", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );

    g->u.toggle_trait( trait_id( "ELFA_EARS" ) );
    d.add_topic( "TALK_TEST_AND" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    g->u.cash = 800;
    talker_npc.remove_effect( effect_currently_busy );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an and cash, available, trait test response." );
}

TEST_CASE( "npc_talk_nested", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );

    d.add_topic( "TALK_TEST_NESTED" );
    talker_npc.add_effect( effect_currently_busy, 9999_turns );
    g->u.cash = 0;
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    g->u.cash = 800;
    g->u.int_cur = 11;
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a complex nested test response." );
}

TEST_CASE( "npc_talk_conditionals", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

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
    g->u.cash = 0;
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

    g->u.remove_items_with( []( const item & it ) {
        return it.get_category().id() == "books" || it.get_category().id() == "food" ||
               it.typeId() == "bottle_glass";
    } );
    d.add_topic( "TALK_TEST_HAS_ITEM" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    d.add_topic( "TALK_TEST_ITEM_REPEAT" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );

    const auto has_item = [&]( player & p, const std::string & id, int count ) {
        item old_item = item( id );
        if( old_item.count_by_charges() ) {
            return p.has_charges( itype_id( id ), count );
        } else {
            return p.has_amount( itype_id( id ), count );
        }
    };
    const auto has_beer_bottle = [&]( player & p, int count ) {
        return has_item( p, "bottle_glass", 1 ) && has_item( p, "beer", count );
    };
    g->u.cash = 1000;
    g->u.int_cur = 8;
    d.add_topic( "TALK_TEST_EFFECTS" );
    gen_response_lines( d, 19 );
    // add and remove effect
    REQUIRE( !g->u.has_effect( effect_infection ) );
    talk_effect_t &effects = d.responses[1].success;
    effects.apply( d );
    CHECK( g->u.has_effect( effect_infection ) );
    CHECK( g->u.get_effect_dur( effect_infection ) == time_duration::from_turns( 10 ) );
    REQUIRE( !talker_npc.has_effect( effect_infection ) );
    effects = d.responses[2].success;
    effects.apply( d );
    CHECK( talker_npc.has_effect( effect_infection ) );
    CHECK( talker_npc.get_effect( effect_infection ).is_permanent() );
    effects = d.responses[3].success;
    effects.apply( d );
    CHECK( !g->u.has_effect( effect_infection ) );
    effects = d.responses[4].success;
    effects.apply( d );
    CHECK( !talker_npc.has_effect( effect_infection ) );
    // add and remove trait
    REQUIRE( !g->u.has_trait( trait_PROF_FED ) );
    effects = d.responses[5].success;
    effects.apply( d );
    CHECK( g->u.has_trait( trait_PROF_FED ) );
    REQUIRE( !talker_npc.has_trait( trait_PROF_FED ) );
    effects = d.responses[6].success;
    effects.apply( d );
    CHECK( talker_npc.has_trait( trait_PROF_FED ) );
    effects = d.responses[7].success;
    effects.apply( d );
    CHECK( !g->u.has_trait( trait_PROF_FED ) );
    effects = d.responses[8].success;
    effects.apply( d );
    CHECK( !talker_npc.has_trait( trait_PROF_FED ) );
    // buying and spending
    talker_npc.op_of_u.owed = 1000;
    REQUIRE( !has_beer_bottle( g->u, 2 ) );
    REQUIRE( talker_npc.op_of_u.owed == 1000 );
    effects = d.responses[9].success;
    effects.apply( d );
    CHECK( talker_npc.op_of_u.owed == 500 );
    CHECK( has_beer_bottle( g->u, 2 ) );
    REQUIRE( !has_item( g->u, "bottle_plastic", 1 ) );
    effects = d.responses[10].success;
    effects.apply( d );
    CHECK( has_item( g->u, "bottle_plastic", 1 ) );
    CHECK( talker_npc.op_of_u.owed == 500 );
    effects = d.responses[11].success;
    effects.apply( d );
    CHECK( talker_npc.op_of_u.owed == 0 );
    talker_npc.op_of_u.owed = 1000;
    // effect chains
    REQUIRE( !g->u.has_effect( effect_infected ) );
    REQUIRE( !talker_npc.has_effect( effect_infected ) );
    REQUIRE( !g->u.has_trait( trait_PROF_SWAT ) );
    REQUIRE( !talker_npc.has_trait( trait_PROF_SWAT ) );
    effects = d.responses[12].success;
    effects.apply( d );
    CHECK( g->u.has_effect( effect_infected ) );
    CHECK( g->u.get_effect_dur( effect_infected ) == time_duration::from_turns( 10 ) );
    CHECK( talker_npc.has_effect( effect_infected ) );
    CHECK( talker_npc.get_effect( effect_infected ).is_permanent() );
    CHECK( g->u.has_trait( trait_PROF_SWAT ) );
    CHECK( talker_npc.has_trait( trait_PROF_SWAT ) );
    CHECK( talker_npc.op_of_u.owed == 0 );
    CHECK( talker_npc.get_attitude() == NPCATT_KILL );
    // opinion changes
    talker_npc.op_of_u = npc_opinion();
    REQUIRE( !talker_npc.op_of_u.trust );
    REQUIRE( !talker_npc.op_of_u.fear );
    REQUIRE( !talker_npc.op_of_u.value );
    REQUIRE( !talker_npc.op_of_u.anger );
    REQUIRE( !talker_npc.op_of_u.owed );
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
    CHECK( d.responses[5].text == "This is a u_has_item_category books test response." );
    CHECK( d.responses[6].text == "This is a u_has_item_category books count 2 test response." );
    CHECK( d.responses[0].text == "This is a repeated item manual_speech test response" );
    CHECK( d.responses[0].success.next_topic.item_type ==  "manual_speech" );

    d.add_topic( "TALK_TEST_ITEM_REPEAT" );
    gen_response_lines( d, 8 );
    CHECK( d.responses[0].text == "This is a repeated category books, food test response" );
    CHECK( d.responses[0].success.next_topic.item_type ==  "beer" );
    CHECK( d.responses[1].text == "This is a repeated category books, food test response" );
    CHECK( d.responses[1].success.next_topic.item_type ==  "dnd_handbook" );
    CHECK( d.responses[2].text == "This is a repeated category books, food test response" );
    CHECK( d.responses[2].success.next_topic.item_type ==  "manual_speech" );
    CHECK( d.responses[3].text == "This is a repeated category books test response" );
    CHECK( d.responses[3].success.next_topic.item_type ==  "dnd_handbook" );
    CHECK( d.responses[4].text == "This is a repeated category books test response" );
    CHECK( d.responses[4].success.next_topic.item_type ==  "manual_speech" );
    CHECK( d.responses[5].text == "This is a repeated item beer, bottle_glass test response" );
    CHECK( d.responses[5].success.next_topic.item_type ==  "bottle_glass" );
    CHECK( d.responses[6].text == "This is a repeated item beer, bottle_glass test response" );
    CHECK( d.responses[6].success.next_topic.item_type ==  "beer" );
    CHECK( d.responses[7].text == "This is a basic test response." );

    // test sell and consume
    d.add_topic( "TALK_TEST_EFFECTS" );
    gen_response_lines( d, 19 );
    REQUIRE( has_item( g->u, "bottle_plastic", 1 ) );
    REQUIRE( has_beer_bottle( g->u, 2 ) );
    REQUIRE( g->u.wield( g->u.i_at( g->u.inv.position_by_type( "bottle_glass" ) ) ) );
    effects = d.responses[14].success;
    effects.apply( d );
    CHECK( !has_item( g->u, "bottle_plastic", 1 ) );
    CHECK( !has_item( g->u, "beer", 1 ) );
    CHECK( has_item( talker_npc, "bottle_plastic", 1 ) );
    CHECK( has_item( talker_npc, "beer", 2 ) );
    effects = d.responses[15].success;
    effects.apply( d );
    CHECK( !has_item( talker_npc, "beer", 2 ) );
    CHECK( has_item( talker_npc, "beer", 1 ) );
    effects = d.responses[16].success;
    effects.apply( d );
    CHECK( has_item( g->u, "beer", 1 ) );
    effects = d.responses[17].success;
    effects.apply( d );
    CHECK( has_item( g->u, "beer", 0 ) );
    CHECK( !has_item( g->u, "beer", 1 ) );
}

TEST_CASE( "npc_talk_combat_commands", "[npc_talk]" )
{
    dialogue d;
    prep_test( d );

    d.add_topic( "TALK_COMBAT_COMMANDS" );
    gen_response_lines( d, 10 );
    CHECK( d.responses[0].text == "Change your engagement rules..." );
    CHECK( d.responses[1].text == "Change your aiming rules..." );
    CHECK( d.responses[2].text == "Stick close to me, no matter what." );
    CHECK( d.responses[3].text == "<ally_rule_follow_distance_2_true_text>" );
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

TEST_CASE( "npc_talk_bionics", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );

    g->u.clear_bionics();
    talker_npc.clear_bionics();
    d.add_topic( "TALK_TEST_BIONICS" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    g->u.add_bionic( bionic_id( "bio_ads" ) );
    talker_npc.add_bionic( bionic_id( "bio_power_storage" ) );
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a u_has_bionics bio_ads test response." );
    CHECK( d.responses[2].text == "This is a npc_has_bionics ANY response." );
}

TEST_CASE( "npc_talk_effects", "[npc_talk]" )
{
    dialogue d;
    npc &talker_npc = prep_test( d );

    // speaker effects just use are owed because I don't want to do anything complicated
    g->u.cash = 1000;
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
    talker_npc.myclass = npc_class_id( "NC_TEST_CLASS" );
    d.add_topic( "TALK_TEST_EFFECTS" );
    gen_response_lines( d, 19 );
    talk_effect_t &effects = d.responses[18].success;
    effects.apply( d );
    CHECK( talker_npc.myclass == npc_class_id( "NC_NONE" ) );
}
