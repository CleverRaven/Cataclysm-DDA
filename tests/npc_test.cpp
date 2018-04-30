#include "catch/catch.hpp"

#include "common_types.h"
#include "player.h"
#include "npc.h"
#include "npc_class.h"
#include "game.h"
#include "map.h"
#include "text_snippets.h"

#include <string>

void on_load_test( npc &who, const time_duration &from, const time_duration &to )
{
    calendar::turn = to_turn<int>( calendar::time_of_cataclysm + from );
    who.on_unload();
    calendar::turn = to_turn<int>( calendar::time_of_cataclysm + to );
    who.on_load();
}

void test_needs( const npc &who, const numeric_interval<int> &hunger,
                 const numeric_interval<int> &thirst,
                 const numeric_interval<int> &fatigue )
{
    CHECK( who.get_hunger() <= hunger.max );
    CHECK( who.get_hunger() >= hunger.min );
    CHECK( who.get_thirst() <= thirst.max );
    CHECK( who.get_thirst() >= thirst.min );
    CHECK( who.get_fatigue() <= fatigue.max );
    CHECK( who.get_fatigue() >= fatigue.min );
}

npc create_model()
{
    npc model_npc;
    model_npc.normalize();
    model_npc.randomize( NC_NONE );
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

TEST_CASE( "on_load-sane-values", "[.]" )
{

    npc model_npc = create_model();

    SECTION( "Awake for 10 minutes, gaining hunger/thirst/fatigue" ) {
        npc test_npc = model_npc;
        const int five_min_ticks = 2;
        on_load_test( test_npc, 0_turns, 5_minutes * five_min_ticks );
        const int margin = 2;

        const numeric_interval<int> hunger( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> thirst( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> fatigue( five_min_ticks, margin, margin );

        test_needs( test_npc, hunger, thirst, fatigue );
    }

    SECTION( "Awake for 2 days, gaining hunger/thirst/fatigue" ) {
        npc test_npc = model_npc;
        const auto five_min_ticks = 2_days / 5_minutes;
        on_load_test( test_npc, 0_turns, 5_minutes * five_min_ticks );

        const int margin = 20;
        const numeric_interval<int> hunger( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> thirst( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> fatigue( five_min_ticks, margin, margin );

        test_needs( test_npc, hunger, thirst, fatigue );
    }

    SECTION( "Sleeping for 6 hours, gaining hunger/thirst (not testing fatigue due to lack of effects processing)" ) {
        npc test_npc = model_npc;
        test_npc.add_effect( efftype_id( "sleep" ), 6_hours );
        test_npc.set_fatigue( 1000 );
        const auto five_min_ticks = 6_hours / 5_minutes;
        /*
        // Fatigue regeneration starts at 1 per 5min, but linearly increases to 2 per 5min at 2 hours or more
        const int expected_fatigue_change =
            ((1.0f + 2.0f) / 2.0f * 2_hours / 5_minutes ) +
            (2.0f * (6_hours - 2_hours) / 5_minutes);
        */
        on_load_test( test_npc, 0_turns, 5_minutes * five_min_ticks );

        const int margin = 10;
        const numeric_interval<int> hunger( five_min_ticks / 8, margin, margin );
        const numeric_interval<int> thirst( five_min_ticks / 8, margin, margin );
        const numeric_interval<int> fatigue( test_npc.get_fatigue(), 0, 0 );

        test_needs( test_npc, hunger, thirst, fatigue );
    }
}

TEST_CASE( "on_load-similar-to-per-turn", "[.]" )
{
    npc model_npc = create_model();

    SECTION( "Awake for 10 minutes, gaining hunger/thirst/fatigue" ) {
        npc on_load_npc = model_npc;
        npc iterated_npc = model_npc;
        const int five_min_ticks = 2;
        on_load_test( on_load_npc, 0_turns, 5_minutes * five_min_ticks );
        for( time_duration turn = 0_turns; turn < 5_minutes * five_min_ticks; turn += 1_turns ) {
            iterated_npc.update_body( calendar::time_of_cataclysm + turn,
                                      calendar::time_of_cataclysm + turn + 1_turns );
        }

        const int margin = 2;
        const numeric_interval<int> hunger( iterated_npc.get_hunger(), margin, margin );
        const numeric_interval<int> thirst( iterated_npc.get_thirst(), margin, margin );
        const numeric_interval<int> fatigue( iterated_npc.get_fatigue(), margin, margin );

        test_needs( on_load_npc, hunger, thirst, fatigue );
    }

    SECTION( "Awake for 6 hours, gaining hunger/thirst/fatigue" ) {
        npc on_load_npc = model_npc;
        npc iterated_npc = model_npc;
        const auto five_min_ticks = 6_hours / 5_minutes;
        on_load_test( on_load_npc, 0_turns, 5_minutes * five_min_ticks );
        for( time_duration turn = 0_turns; turn < 5_minutes * five_min_ticks; turn += 1_turns ) {
            iterated_npc.update_body( calendar::time_of_cataclysm + turn,
                                      calendar::time_of_cataclysm + turn + 1_turns );
        }

        const int margin = 10;
        const numeric_interval<int> hunger( iterated_npc.get_hunger(), margin, margin );
        const numeric_interval<int> thirst( iterated_npc.get_thirst(), margin, margin );
        const numeric_interval<int> fatigue( iterated_npc.get_fatigue(), margin, margin );

        test_needs( on_load_npc, hunger, thirst, fatigue );
    }
}

TEST_CASE( "snippet-tag-test" )
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
        }
    };

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
