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

#include <string>

const efftype_id effect_gave_quest_item( "gave_quest_item" );
const efftype_id effect_currently_busy( "currently_busy" );

npc create_test_talker()
{
    npc model_npc;
    const string_id<npc_template> test_talker( "test_talker" );
    model_npc.normalize();
    model_npc.load_npc_template( test_talker );
    for( trait_id tr : model_npc.get_mutations() ) {
        model_npc.unset_mutation( tr );
    }
    model_npc.set_hunger( 0 );
    model_npc.set_thirst( 0 );
    model_npc.set_fatigue( 0 );
    model_npc.remove_effect( efftype_id( "sleep" ) );
    // An ugly hack to prevent NPC falling asleep during testing due to massive fatigue
    model_npc.set_mutation( trait_id( "WEB_WEAVER" ) );

    return model_npc;
}

TEST_CASE( "npc_talk_test" )
{
    tripoint test_origin( 15, 15, 0 );
    g->u.setpos( test_origin );

    g->faction_manager_ptr->create_if_needed();

    npc talker_npc = create_test_talker();

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
}
