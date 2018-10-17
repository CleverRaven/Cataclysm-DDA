#include "catch/catch.hpp"

#include "common_types.h"
#include "player.h"
#include "npc.h"
#include "npc_class.h"
#include "game.h"
#include "map.h"
#include "text_snippets.h"
#include "field.h"
#include "overmapbuffer.h"
#include "dialogue.h"
#include "faction.h"
#include "player.h"
#include "effect.h"
#include "calendar.h"
#include "coordinate_conversions.h"

#include <string>

const efftype_id effect_gave_quest_item( "gave_quest_item" );
const efftype_id effect_currently_busy( "currently_busy" );
const efftype_id effect_infection( "infection" );
const efftype_id effect_infected( "infected" );
static const trait_id trait_PROF_FED( "PROF_FED" );
static const trait_id trait_PROF_SWAT( "PROF_SWAT" );

npc &create_test_talker()
{
    const string_id<npc_template> test_talker( "test_talker" );
    int model_id = g->m.place_npc( 25, 25, test_talker, true );
    g->load_npcs();

    npc *model_npc = g->find_npc( model_id );
    REQUIRE( model_npc != nullptr );

    for( trait_id tr : model_npc->get_mutations() ) {
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

void change_om_type( const std::string &new_type )
{
    const point omt_pos = ms_to_omt_copy( g->m.getabs( g->u.posx(), g->u.posy() ) );
    oter_id &omt_ref = overmap_buffer.ter( omt_pos.x, omt_pos.y, g->u.posz() );
    omt_ref = oter_id( new_type );
}

TEST_CASE( "npc_talk_test" )
{
    tripoint test_origin( 15, 15, 0 );
    g->u.setpos( test_origin );

    g->faction_manager_ptr->create_if_needed();

    npc &talker_npc = create_test_talker();

    dialogue d;
    d.alpha = &g->u;
    d.beta = &talker_npc;

    d.add_topic( "TALK_TEST_START" );
    d.gen_responses( d.topic_stack.back() );

    CHECK( d.responses.size() == 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );

    g->u.str_cur = 8;
    g->u.dex_cur = 8;
    g->u.int_cur = 8;
    g->u.per_cur = 8;

    d.add_topic( "TALK_TEST_SIMPLE_STATS" );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 5 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a strength test response." );
    CHECK( d.responses[2].text == "This is a dexterity test response." );
    CHECK( d.responses[3].text == "This is an intelligence test response." );
    CHECK( d.responses[4].text == "This is a perception test response." );
    g->u.str_cur = 6;
    g->u.dex_cur = 6;
    g->u.int_cur = 6;
    g->u.per_cur = 6;
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );

    d.add_topic( "TALK_TEST_NEGATED_STATS" );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 5 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a low strength test response." );
    CHECK( d.responses[2].text == "This is a low dexterity test response." );
    CHECK( d.responses[3].text == "This is a low intelligence test response." );
    CHECK( d.responses[4].text == "This is a low perception test response." );
    g->u.str_cur = 8;
    g->u.dex_cur = 8;
    g->u.int_cur = 8;
    g->u.per_cur = 8;
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );

    d.add_topic( "TALK_TEST_WEARING_AND_TRAIT" );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    g->u.toggle_trait( trait_id( "ELFA_EARS" ) );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a trait test response." );
    g->u.wear_item( item( "badge_marshal" ) );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a trait test response." );
    CHECK( d.responses[2].text == "This is a wearing test response." );

    d.add_topic( "TALK_TEST_EFFECT" );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    talker_npc.add_effect( effect_gave_quest_item, 9999_turns );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an npc effect test response." );
    g->u.add_effect( effect_gave_quest_item, 9999_turns );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 3 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an npc effect test response." );
    CHECK( d.responses[2].text == "This is a player effect test response." );

    d.add_topic( "TALK_TEST_SERVICE" );
    g->u.cash = 0;
    talker_npc.add_effect( effect_currently_busy, 9999_turns );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    g->u.cash = 800;
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a cash test response." );
    talker_npc.remove_effect( effect_currently_busy );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 4 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a cash test response." );
    CHECK( d.responses[2].text == "This is an npc service test response." );
    CHECK( d.responses[3].text == "This is an npc available test response." );

    change_om_type( "pond_swamp" );
    d.add_topic( "TALK_TEST_LOCATION" );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    change_om_type( "field" );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a om_location_field test response." );
    change_om_type( "faction_base_camp_11" );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a faction camp any test response." );

    d.add_topic( "TALK_TEST_NPC_ROLE" );
    talker_npc.companion_mission_role_id = "NO_TEST_ROLE";
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    talker_npc.companion_mission_role_id = "TEST_ROLE";
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a nearby role test response." );

    for( npc *guy : g->allies() ) {
        guy->set_attitude( NPCATT_NULL );
    }
    talker_npc.set_attitude( NPCATT_FOLLOW );
    d.add_topic( "TALK_TEST_NPC_ALLIES" );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a npc allies 1 test response." );

    d.add_topic( "TALK_TEST_OR" );
    g->u.cash = 0;
    g->u.toggle_trait( trait_id( "ELFA_EARS" ) );
    talker_npc.add_effect( effect_currently_busy, 9999_turns );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    g->u.toggle_trait( trait_id( "ELFA_EARS" ) );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an or trait test response." );

    d.add_topic( "TALK_TEST_AND" );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    g->u.cash = 800;
    talker_npc.remove_effect( effect_currently_busy );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is an and cash, available, trait test response." );

    d.add_topic( "TALK_TEST_NESTED" );
    talker_npc.add_effect( effect_currently_busy, 9999_turns );
    g->u.cash = 0;
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 1 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    g->u.cash = 800;
    g->u.int_cur = 11;
    d.gen_responses( d.topic_stack.back() );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 2 );
    CHECK( d.responses[0].text == "This is a basic test response." );
    CHECK( d.responses[1].text == "This is a complex nested test response." );

    const auto has_beer_bottle = [&]() {
        int bottle_pos = g->u.inv.position_by_type( itype_id( "bottle_glass" ) );
        if( bottle_pos == INT_MIN ) {
            return false;
        }
        item &bottle = g->u.inv.find_item( bottle_pos );
        if( bottle.is_container_empty() ) {
            return false;
        }
        const item &beer = bottle.get_contained();
        return beer.typeId() == itype_id( "beer" ) && beer.charges == 2;
    };
    const auto has_plastic_bottle = [&]() {
        return g->u.inv.position_by_type( itype_id( "bottle_plastic" ) ) != INT_MIN;
    };
    g->u.cash = 1000;
    g->u.int_cur = 8;
    d.add_topic( "TALK_TEST_EFFECTS" );
    d.gen_responses( d.topic_stack.back() );
    CHECK( d.responses.size() == 9 );
    REQUIRE( !g->u.has_effect( effect_infection ) );
    talk_response::effect_t &effects = d.responses[1].success;
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
    REQUIRE( !has_plastic_bottle() );
    effects = d.responses[6].success;
    effects.apply( d );
    CHECK( has_plastic_bottle() );
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
}
