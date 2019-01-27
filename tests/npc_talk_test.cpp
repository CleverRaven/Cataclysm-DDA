#include <string>

#include "catch/catch.hpp"
#include "calendar.h"
#include "coordinate_conversions.h"
#include "dialogue.h"
#include "effect.h"
#include "faction.h"
#include "game.h"
#include "map.h"
#include "mission.h"
#include "npc.h"
#include "overmapbuffer.h"
#include "player.h"

const efftype_id effect_gave_quest_item( "gave_quest_item" );
const efftype_id effect_currently_busy( "currently_busy" );
const efftype_id effect_infection( "infection" );
const efftype_id effect_infected( "infected" );
static const trait_id trait_PROF_FED( "PROF_FED" );
static const trait_id trait_PROF_SWAT( "PROF_SWAT" );

npc &create_test_talker()
{
    const string_id<npc_template> test_talker( "test_talker" );
    const int model_id = g->m.place_npc( 25, 25, test_talker, true );
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

void gen_response_lines( dialogue &d, size_t expected_count )
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

void change_om_type( const std::string &new_type )
{
    const point omt_pos = ms_to_omt_copy( g->m.getabs( g->u.posx(), g->u.posy() ) );
    oter_id &omt_ref = overmap_buffer.ter( omt_pos.x, omt_pos.y, g->u.posz() );
    omt_ref = oter_id( new_type );
}

TEST_CASE( "npc_talk_test" )
{
    const tripoint test_origin( 15, 15, 0 );
    g->u.setpos( test_origin );

    g->faction_manager_ptr->create_if_needed();

    npc &talker_npc = create_test_talker();

    dialogue d;
    d.alpha = &g->u;
    d.beta = &talker_npc;

    d.add_topic( "TALK_TEST_START" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );

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
    g->u.str_cur = 8;
    g->u.dex_cur = 8;
    g->u.int_cur = 8;
    g->u.per_cur = 8;
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );

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

    change_om_type( "pond_swamp" );
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

    d.add_topic( "TALK_TEST_NPC_ROLE" );
    talker_npc.companion_mission_role_id = "NO_TEST_ROLE";
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    talker_npc.companion_mission_role_id = "TEST_ROLE";
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a nearby role test response." );

    d.add_topic( "TALK_TEST_NPC_CLASS" );
    talker_npc.myclass = npc_class_id( "NC_NONE" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    talker_npc.myclass = npc_class_id( "NC_TEST_CLASS" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a class test response." );

    for( npc *guy : g->allies() ) {
        guy->set_attitude( NPCATT_NULL );
    }
    talker_npc.set_attitude( NPCATT_FOLLOW );
    d.add_topic( "TALK_TEST_NPC_ALLIES" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a npc allies 1 test response." );

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

    const calendar old_calendar = calendar::turn;
    calendar::turn = calendar::start;
    d.add_topic( "TALK_TEST_SEASON" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a season spring test response." );
    calendar::turn += to_turns<int>( calendar::season_length() );
    gen_response_lines( d, 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a days since cataclysm 30 test response." );
    CHECK( d.responses[2].text == "This is a season summer test response." );
    calendar::turn += to_turns<int>( calendar::season_length() );
    gen_response_lines( d, 4 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a days since cataclysm 30 test response." );
    CHECK( d.responses[2].text == "This is a days since cataclysm 120 test response." );
    CHECK( d.responses[3].text == "This is a season autumn test response." );
    calendar::turn += to_turns<int>( calendar::season_length() );
    gen_response_lines( d, 5 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a days since cataclysm 30 test response." );
    CHECK( d.responses[2].text == "This is a days since cataclysm 120 test response." );
    CHECK( d.responses[3].text == "This is a days since cataclysm 210 test response." );
    CHECK( d.responses[4].text == "This is a season winter test response." );
    calendar::turn += to_turns<int>( calendar::season_length() );
    gen_response_lines( d, 6 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a season spring test response." );
    CHECK( d.responses[2].text == "This is a days since cataclysm 30 test response." );
    CHECK( d.responses[3].text == "This is a days since cataclysm 120 test response." );
    CHECK( d.responses[4].text == "This is a days since cataclysm 210 test response." );
    CHECK( d.responses[5].text == "This is a days since cataclysm 300 test response." );

    calendar::turn = calendar::turn.sunrise() + HOURS( 4 );
    d.add_topic( "TALK_TEST_TIME" );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a is day test response." );
    calendar::turn = calendar::turn.sunset() + HOURS( 2 );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a is night test response." );
    calendar::turn = old_calendar;

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

    d.add_topic( "TALK_TEST_OR" );
    g->u.cash = 0;
    talker_npc.add_effect( effect_currently_busy, 9999_turns );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    g->u.toggle_trait( trait_id( "ELFA_EARS" ) );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an or trait test response." );

    d.add_topic( "TALK_TEST_AND" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    g->u.cash = 800;
    talker_npc.remove_effect( effect_currently_busy );
    gen_response_lines( d, 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an and cash, available, trait test response." );

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

    d.add_topic( "TALK_TEST_HAS_ITEM" );
    gen_response_lines( d, 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );

    const auto has_item = [&]( const std::string & id, int count ) {
        return g->u.has_charges( itype_id( id ), count ) ||
               g->u.has_amount( itype_id( id ), count );
    };
    const auto has_beer_bottle = [&]() {
        return has_item( "bottle_glass", 1 ) && has_item( "beer", 2 );
    };
    g->u.cash = 1000;
    g->u.int_cur = 8;
    d.add_topic( "TALK_TEST_EFFECTS" );
    gen_response_lines( d, 10 );
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
    REQUIRE( !g->u.has_trait( trait_PROF_FED ) );
    effects = d.responses[3].success;
    effects.apply( d );
    CHECK( g->u.has_trait( trait_PROF_FED ) );
    REQUIRE( !talker_npc.has_trait( trait_PROF_FED ) );
    effects = d.responses[4].success;
    effects.apply( d );
    CHECK( talker_npc.has_trait( trait_PROF_FED ) );
    REQUIRE( !has_beer_bottle() );
    REQUIRE( g->u.cash == 1000 );
    effects = d.responses[5].success;
    effects.apply( d );
    CHECK( g->u.cash == 500 );
    CHECK( has_beer_bottle() );
    REQUIRE( !has_item( "bottle_plastic", 1 ) );
    effects = d.responses[6].success;
    effects.apply( d );
    CHECK( has_item( "bottle_plastic", 1 ) );
    CHECK( g->u.cash == 500 );
    effects = d.responses[7].success;
    effects.apply( d );
    CHECK( g->u.cash == 0 );
    g->u.cash = 1000;
    REQUIRE( !g->u.has_effect( effect_infected ) );
    REQUIRE( !talker_npc.has_effect( effect_infected ) );
    REQUIRE( !g->u.has_trait( trait_PROF_SWAT ) );
    REQUIRE( !talker_npc.has_trait( trait_PROF_SWAT ) );
    effects = d.responses[8].success;
    effects.apply( d );
    CHECK( g->u.has_effect( effect_infected ) );
    CHECK( g->u.get_effect_dur( effect_infected ) == time_duration::from_turns( 10 ) );
    CHECK( talker_npc.has_effect( effect_infected ) );
    CHECK( talker_npc.get_effect( effect_infected ).is_permanent() );
    CHECK( g->u.has_trait( trait_PROF_SWAT ) );
    CHECK( talker_npc.has_trait( trait_PROF_SWAT ) );
    CHECK( g->u.cash == 0 );
    CHECK( talker_npc.get_attitude() == NPCATT_KILL );

    d.add_topic( "TALK_TEST_HAS_ITEM" );
    gen_response_lines( d, 4 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a u_has_item beer test response." );
    CHECK( d.responses[2].text == "This is a u_has_item bottle_glass test response." );
    CHECK( d.responses[3].text == "This is a u_has_items beer test response." );

    d.add_topic( "TALK_TEST_EFFECTS" );
    gen_response_lines( d, 10 );
    REQUIRE( has_item( "bottle_plastic", 1 ) );
    REQUIRE( has_beer_bottle() );
    REQUIRE( g->u.wield( g->u.i_at( g->u.inv.position_by_type( "bottle_glass" ) ) ) );
    effects = d.responses[9].success;
    effects.apply( d );
    CHECK( !has_item( "bottle_plastic", 1 ) );
    CHECK( !has_item( "beer", 1 ) );

    d.add_topic( "TALK_COMBAT_COMMANDS" );
    gen_response_lines( d, 7 );
    CHECK( d.responses[0].text == "Change your engagement rules..." );
    CHECK( d.responses[1].text == "Change your aiming rules..." );
    CHECK( d.responses[2].text == "Don't use ranged weapons anymore." );
    CHECK( d.responses[3].text == "Use only silent weapons." );
    CHECK( d.responses[4].text == "Don't use grenades anymore." );
    CHECK( d.responses[5].text == "Don't worry about shooting an ally." );
    CHECK( d.responses[6].text == "Never mind." );
}
