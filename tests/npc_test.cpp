#include "catch/catch.hpp"

#include "player.h"
#include "npc.h"
#include "npc_class.h"
#include "game.h"
#include "map.h"
#include "text_snippets.h"

#include <string>

void on_load_test( npc &who, calendar from, calendar to )
{
    calendar::turn = from;
    who.on_unload();
    calendar::turn = to;
    who.on_load();
}

void sane( const npc &who )
{
    CHECK( who.get_hunger() >= 0 );
    CHECK( who.get_thirst() >= 0 );
    CHECK( who.get_fatigue() >= -25 );
}

npc create_model()
{
    npc model_npc;
    model_npc.normalize();
    model_npc.randomize( NC_NONE );
    model_npc.set_hunger( 0 );
    model_npc.set_thirst( 0 );
    model_npc.set_fatigue( 0 );
    model_npc.remove_effect( efftype_id( "sleep" ) );
    // An ugly hack to prevent NPC falling asleep during testing due to massive fatigue
    model_npc.set_mutation( "WEB_WEAVER" );
    return model_npc;
}

TEST_CASE("on_load-sane-values")
{

    npc model_npc = create_model();

    SECTION("Awake for 10 minutes, gaining hunger/thirst/fatigue") {
        npc test_npc = model_npc;
        const int five_min_ticks = 2;
        on_load_test( test_npc, 0, MINUTES(5 * five_min_ticks) );

        const int margin = 1;
        CHECK( test_npc.get_hunger() <= five_min_ticks + margin );
        CHECK( test_npc.get_thirst() <= five_min_ticks + margin );
        CHECK( test_npc.get_fatigue() <= five_min_ticks + margin );
        CHECK( test_npc.get_hunger() >= five_min_ticks - margin );
        CHECK( test_npc.get_thirst() >= five_min_ticks - margin );
        CHECK( test_npc.get_fatigue() >= five_min_ticks - margin );
    }

    SECTION("Awake for 2 days, gaining hunger/thirst/fatigue") {
        npc test_npc = model_npc;
        const int five_min_ticks = HOURS(2 * 24) / MINUTES(5);
        on_load_test( test_npc, 0, MINUTES(5 * five_min_ticks) );

        const int margin = 10;
        CHECK( test_npc.get_hunger() <= five_min_ticks + margin );
        CHECK( test_npc.get_thirst() <= five_min_ticks + margin );
        CHECK( test_npc.get_fatigue() <= five_min_ticks + margin );
        CHECK( test_npc.get_hunger() >= five_min_ticks - margin );
        CHECK( test_npc.get_thirst() >= five_min_ticks - margin );
        CHECK( test_npc.get_fatigue() >= five_min_ticks - margin );
    }

    SECTION("Sleeping for 6 hours, gaining hunger/thirst (not testing fatigue due to lack of effects processing)") {
        npc test_npc = model_npc;
        test_npc.add_effect( efftype_id( "sleep" ), HOURS(6) );
        test_npc.set_fatigue(1000);
        const int five_min_ticks = 12 * 6;
        const float expected_rate = 0.5f;
        const int expected_change = five_min_ticks * expected_rate;
        /*
        // Fatigue regeneration starts at 1 per 5min, but linearly increases to 2 per 5min at 2 hours or more
        const int expected_fatigue_change =
            ((1.0f + 2.0f) / 2.0f * HOURS(2) / MINUTES(5) ) +
            (2.0f * HOURS(6 - 2) / MINUTES(5));
        */
        on_load_test( test_npc, 0, MINUTES(5 * five_min_ticks) );

        const int margin = 10;
        CHECK( test_npc.get_hunger() <= expected_change + margin );
        CHECK( test_npc.get_thirst() <= expected_change + margin );
        //CHECK( test_npc.fatigue <= 1000 - expected_fatigue_change + margin );
        CHECK( test_npc.get_hunger() >= expected_change - margin );
        CHECK( test_npc.get_thirst() >= expected_change - margin );
        //CHECK( test_npc.fatigue >= 1000 - expected_fatigue_change - margin );
    }
}

TEST_CASE("on_load-similar-to-per-turn")
{
    npc model_npc = create_model();

    SECTION("Awake for 10 minutes, gaining hunger/thirst/fatigue") {
        npc on_load_npc = model_npc;
        npc iterated_npc = model_npc;
        const int five_min_ticks = 2;
        on_load_test( on_load_npc, 0, MINUTES(5 * five_min_ticks) );
        for( int turn = 0; turn < MINUTES(5 * five_min_ticks); turn++ ) {
            iterated_npc.update_body( turn, turn + 1 );
        }

        const int margin = 1;
        CHECK( on_load_npc.get_hunger() <= iterated_npc.get_hunger() + margin );
        CHECK( on_load_npc.get_thirst() <= iterated_npc.get_thirst() + margin );
        CHECK( on_load_npc.get_fatigue() <= iterated_npc.get_fatigue() + margin );
        CHECK( on_load_npc.get_hunger() >= iterated_npc.get_hunger() - margin );
        CHECK( on_load_npc.get_thirst() >= iterated_npc.get_thirst() - margin );
        CHECK( on_load_npc.get_fatigue() >= iterated_npc.get_fatigue() - margin );
    }

    SECTION("Awake for 6 hours, gaining hunger/thirst/fatigue") {
        npc on_load_npc = model_npc;
        npc iterated_npc = model_npc;
        const int five_min_ticks = HOURS(6) / MINUTES(5);
        on_load_test( on_load_npc, 0, MINUTES(5 * five_min_ticks) );
        for( int turn = 0; turn < MINUTES(5 * five_min_ticks); turn++ ) {
            iterated_npc.update_body( turn, turn + 1 );
        }

        const int margin = 10;
        CHECK( on_load_npc.get_hunger() <= iterated_npc.get_hunger() + margin );
        CHECK( on_load_npc.get_thirst() <= iterated_npc.get_thirst() + margin );
        CHECK( on_load_npc.get_fatigue() <= iterated_npc.get_fatigue() + margin );
        CHECK( on_load_npc.get_hunger() >= iterated_npc.get_hunger() - margin );
        CHECK( on_load_npc.get_thirst() >= iterated_npc.get_thirst() - margin );
        CHECK( on_load_npc.get_fatigue() >= iterated_npc.get_fatigue() - margin );
    }
}

TEST_CASE("snippet-tag-test")
{
    // Actually used tags
    static const std::set<std::string> npc_talk_tags = {{
        "<name_b>", "<thirsty>", "<swear!>",
        "<sad>", "<greet>", "<no>",
        "<im_leaving_you>", "<ill_kill_you>", "<ill_die>",
        "<wait>", "<no_faction>", "<name_g>",
        "<keep_up>", "<yawn>", "<very>",
        "<okay>", "<catch_up>", "<really>",
        "<let_me_pass>", "<done_mugging>", "<happy>",
        "<drop_weapon>", "<swear>", "<lets_talk>",
        "<hands_up>", "<move>", "<hungry>",
        "<fuck_you>",
    }};

    for( const auto &tag : npc_talk_tags ) {
        const auto ids = SNIPPET.all_ids_from_category( tag );
        std::set<std::string> valid_snippets;
        for( int id : ids ) {
            const auto snip = SNIPPET.get( id );
            valid_snippets.insert( snip );
        }

        // We want to get all the snippets in the category
        std::set<std::string> found_snippets;
        // Brute force random snippets to see if they are all in their category
        for( size_t i = 0; i < ids.size() * 100; i++ ) {
            const auto &roll = SNIPPET.random_from_category( tag );
            CHECK( valid_snippets.count( roll ) > 0 );
            found_snippets.insert( roll );
        }

        CHECK( found_snippets == valid_snippets );
    }

    // Special tags, those should have empty replacements
    CHECK( SNIPPET.all_ids_from_category( "<yrwp>" ).empty() );
    CHECK( SNIPPET.all_ids_from_category( "<mywp>" ).empty() );
    CHECK( SNIPPET.all_ids_from_category( "<ammo>" ).empty() );
}
