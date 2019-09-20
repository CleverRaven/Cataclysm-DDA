#include <stddef.h>
#include <string>
#include <memory>
#include <set>
#include <utility>
#include <vector>
#include <sstream>

#include "avatar.h"
#include "catch/catch.hpp"
#include "common_types.h"
#include "faction.h"
#include "field.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "npc.h"
#include "npc_class.h"
#include "overmapbuffer.h"
#include "text_snippets.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "calendar.h"
#include "line.h"
#include "optional.h"
#include "pimpl.h"
#include "string_id.h"
#include "type_id.h"
#include "point.h"

class Creature;

static void on_load_test( npc &who, const time_duration &from, const time_duration &to )
{
    calendar::turn = to_turn<int>( calendar::turn_zero + from );
    who.on_unload();
    calendar::turn = to_turn<int>( calendar::turn_zero + to );
    who.on_load();
}

static void test_needs( const npc &who, const numeric_interval<int> &hunger,
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

static npc create_model()
{
    npc model_npc;
    model_npc.normalize();
    model_npc.randomize( NC_NONE );
    for( const trait_id &tr : model_npc.get_mutations() ) {
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

static std::string get_list_of_npcs( const std::string &title )
{

    std::ostringstream npc_list;
    npc_list << title << ":\n";
    for( const npc &n : g->all_npcs() ) {
        npc_list << "  " << &n << ": " << n.name << '\n';
    }
    return npc_list.str();
}

TEST_CASE( "on_load-sane-values", "[.]" )
{
    SECTION( "Awake for 10 minutes, gaining hunger/thirst/fatigue" ) {
        npc test_npc = create_model();
        const int five_min_ticks = 2;
        on_load_test( test_npc, 0_turns, 5_minutes * five_min_ticks );
        const int margin = 2;

        const numeric_interval<int> hunger( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> thirst( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> fatigue( five_min_ticks, margin, margin );

        test_needs( test_npc, hunger, thirst, fatigue );
    }

    SECTION( "Awake for 2 days, gaining hunger/thirst/fatigue" ) {
        npc test_npc = create_model();
        const auto five_min_ticks = 2_days / 5_minutes;
        on_load_test( test_npc, 0_turns, 5_minutes * five_min_ticks );

        const int margin = 20;
        const numeric_interval<int> hunger( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> thirst( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> fatigue( five_min_ticks, margin, margin );

        test_needs( test_npc, hunger, thirst, fatigue );
    }

    SECTION( "Sleeping for 6 hours, gaining hunger/thirst (not testing fatigue due to lack of effects processing)" ) {
        npc test_npc = create_model();
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
    SECTION( "Awake for 10 minutes, gaining hunger/thirst/fatigue" ) {
        npc on_load_npc = create_model();
        npc iterated_npc = create_model();
        const int five_min_ticks = 2;
        on_load_test( on_load_npc, 0_turns, 5_minutes * five_min_ticks );
        for( time_duration turn = 0_turns; turn < 5_minutes * five_min_ticks; turn += 1_turns ) {
            iterated_npc.update_body( calendar::turn_zero + turn,
                                      calendar::turn_zero + turn + 1_turns );
        }

        const int margin = 2;
        const numeric_interval<int> hunger( iterated_npc.get_hunger(), margin, margin );
        const numeric_interval<int> thirst( iterated_npc.get_thirst(), margin, margin );
        const numeric_interval<int> fatigue( iterated_npc.get_fatigue(), margin, margin );

        test_needs( on_load_npc, hunger, thirst, fatigue );
    }

    SECTION( "Awake for 6 hours, gaining hunger/thirst/fatigue" ) {
        npc on_load_npc = create_model();
        npc iterated_npc = create_model();
        const auto five_min_ticks = 6_hours / 5_minutes;
        on_load_test( on_load_npc, 0_turns, 5_minutes * five_min_ticks );
        for( time_duration turn = 0_turns; turn < 5_minutes * five_min_ticks; turn += 1_turns ) {
            iterated_npc.update_body( calendar::turn_zero + turn,
                                      calendar::turn_zero + turn + 1_turns );
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
    static const std::set<std::string> npc_talk_tags = {
        {
            "<name_b>", "<thirsty>", "<swear!>",
            "<sad>", "<greet>", "<no>",
            "<im_leaving_you>", "<ill_kill_you>", "<ill_die>",
            "<wait>", "<no_faction>", "<name_g>",
            "<keep_up>", "<yawn>", "<very>",
            "<okay>", "<really>",
            "<let_me_pass>", "<done_mugging>", "<happy>",
            "<swear>", "<lets_talk>",
            "<hands_up>", "<move>", "<hungry>",
            "<fuck_you>",
        }
    };

    for( const auto &tag : npc_talk_tags ) {
        const auto ids = SNIPPET.all_ids_from_category( tag );
        CHECK( ids.size() > 0 );

        for( size_t i = 0; i < ids.size() * 100; i++ ) {
            const auto snip = SNIPPET.random_from_category( tag );
            CHECK( !snip.empty() );
        }
    }

    // Special tags, those should have empty replacements
    CHECK( SNIPPET.all_ids_from_category( "<yrwp>" ).empty() );
    CHECK( SNIPPET.all_ids_from_category( "<mywp>" ).empty() );
    CHECK( SNIPPET.all_ids_from_category( "<ammo>" ).empty() );
}

/* Test setup. Player should always be at top-left.
 *
 * U is the player, V is vehicle, # is wall, R is rubble & acid with NPC on it,
 * A is acid with NPC on it, W/M is vehicle & acid with (follower/non-follower) NPC on it,
 * B/C is acid with (follower/non-follower) NPC on it.
 */
constexpr int height = 5, width = 17;
constexpr char setup[height][width + 1] = {
    "U ###############",
    "V #R#AAA#W# # #C#",
    "  #A#A#A# #M#B# #",
    "  ###AAA#########",
    "    #####        ",
};

static void check_npc_movement( const tripoint &origin )
{
    const efftype_id effect_bouldering( "bouldering" );

    INFO( "Should not crash from infinite recursion" );
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            switch( setup[y][x] ) {
                case 'A':
                case 'R':
                case 'W':
                case 'M':
                case 'B':
                case 'C':
                    tripoint p = origin + point( x, y );
                    npc *guy = g->critter_at<npc>( p );
                    REQUIRE( guy != nullptr );
                    guy->move();
                    break;
            }
        }
    }

    INFO( "NPC on acid should not acquire unstable footing status" );
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            if( setup[y][x] == 'A' ) {
                tripoint p = origin + point( x, y );
                npc *guy = g->critter_at<npc>( p );
                REQUIRE( guy != nullptr );
                CHECK( !guy->has_effect( effect_bouldering ) );
            }
        }
    }

    INFO( "NPC on rubbles should not lose unstable footing status" );
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            if( setup[y][x] == 'R' ) {
                tripoint p = origin + point( x, y );
                npc *guy = g->critter_at<npc>( p );
                REQUIRE( guy != nullptr );
                CHECK( guy->has_effect( effect_bouldering ) );
            }
        }
    }

    INFO( "NPC in vehicle should not escape from dangerous terrain" );
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            switch( setup[y][x] ) {
                case 'W':
                case 'M':
                    CAPTURE( setup[y][x] );
                    tripoint p = origin + point( x, y );
                    npc *guy = g->critter_at<npc>( p );
                    CHECK( guy != nullptr );
                    break;
            }
        }
    }

    INFO( "NPC not in vehicle should escape from dangerous terrain" );
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            switch( setup[y][x] ) {
                case 'B':
                case 'C':
                    tripoint p = origin + point( x, y );
                    npc *guy = g->critter_at<npc>( p );
                    CHECK( guy == nullptr );
                    break;
            }
        }
    }
}

TEST_CASE( "npc-movement" )
{
    const ter_id t_reinforced_glass( "t_reinforced_glass" );
    const ter_id t_floor( "t_floor" );
    const furn_id f_rubble( "f_rubble" );
    const furn_id f_null( "f_null" );
    const vpart_id vpart_frame_vertical( "frame_vertical" );
    const vpart_id vpart_seat( "seat" );

    g->place_player( tripoint( 60, 60, 0 ) );

    clear_map();

    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            const char type = setup[y][x];
            const tripoint p = g->u.pos() + point( x, y );
            // create walls
            if( type == '#' ) {
                g->m.ter_set( p, t_reinforced_glass );
            } else {
                g->m.ter_set( p, t_floor );
            }
            // spawn acid
            // a copy is needed because we will remove elements from it
            const field fs = g->m.field_at( p );
            for( const auto &f : fs ) {
                g->m.remove_field( p, f.first );
            }
            if( type == 'A' || type == 'R' || type == 'W' || type == 'M'
                || type == 'B' || type == 'C' ) {

                g->m.add_field( p, fd_acid, 3 );
            }
            // spawn rubbles
            if( type == 'R' ) {
                g->m.furn_set( p, f_rubble );
            } else {
                g->m.furn_set( p, f_null );
            }
            // create vehicles
            if( type == 'V' || type == 'W' || type == 'M' ) {
                vehicle *veh = g->m.add_vehicle( vproto_id( "none" ), p, 270, 0, 0 );
                REQUIRE( veh != nullptr );
                veh->install_part( point_zero, vpart_frame_vertical );
                veh->install_part( point_zero, vpart_seat );
                g->m.add_vehicle_to_cache( veh );
            }
            // spawn npcs
            if( type == 'A' || type == 'R' || type == 'W' || type == 'M'
                || type == 'B' || type == 'C' ) {

                std::shared_ptr<npc> guy = std::make_shared<npc>();
                do {
                    guy->normalize();
                    guy->randomize();
                    // Repeat until we get an NPC vulnerable to acid
                } while( guy->is_immune_field( fd_acid ) );
                guy->spawn_at_precise( {g->get_levx(), g->get_levy()}, p );
                // Set the shopkeep mission; this means that
                // the NPC deems themselves to be guarding and stops them
                // wandering off in search of distant ammo caches, etc.
                guy->mission = NPC_MISSION_SHOPKEEP;
                overmap_buffer.insert_npc( guy );
                g->load_npcs();
                guy->set_attitude( ( type == 'M' || type == 'C' ) ? NPCATT_NULL : NPCATT_FOLLOW );
            }
        }
    }

    // check preconditions
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            const char type = setup[y][x];
            const tripoint p = g->u.pos() + point( x, y );
            if( type == '#' ) {
                REQUIRE( !g->m.passable( p ) );
            } else {
                REQUIRE( g->m.passable( p ) );
            }
            if( type == 'R' ) {
                REQUIRE( g->m.has_flag( "UNSTABLE", p ) );
            } else {
                REQUIRE( !g->m.has_flag( "UNSTABLE", p ) );
            }
            if( type == 'V' || type == 'W' || type == 'M' ) {
                REQUIRE( g->m.veh_at( p ).part_with_feature( VPFLAG_BOARDABLE, true ).has_value() );
            } else {
                REQUIRE( !g->m.veh_at( p ).part_with_feature( VPFLAG_BOARDABLE, true ).has_value() );
            }
            npc *guy = g->critter_at<npc>( p );
            if( type == 'A' || type == 'R' || type == 'W' || type == 'M'
                || type == 'B' || type == 'C' ) {

                REQUIRE( guy != nullptr );
                REQUIRE( guy->is_dangerous_fields( g->m.field_at( p ) ) );
            } else {
                REQUIRE( guy == nullptr );
            }
        }
    }

    SECTION( "NPCs escape dangerous terrain by pushing other NPCs" ) {
        check_npc_movement( g->u.pos() );
    }

    SECTION( "Player in vehicle & NPCs escaping dangerous terrain" ) {
        const tripoint origin = g->u.pos();

        for( int y = 0; y < height; ++y ) {
            for( int x = 0; x < width; ++x ) {
                if( setup[y][x] == 'V' ) {
                    g->place_player( g->u.pos() + point( x, y ) );
                    break;
                }
            }
        }

        check_npc_movement( origin );
    }
}

TEST_CASE( "npc_can_target_player" )
{
    // Set to daytime for visibiliity
    calendar::turn = calendar::turn_zero + 12_hours;

    g->faction_manager_ptr->create_if_needed();

    g->place_player( tripoint( 10, 10, 0 ) );

    clear_npcs();
    clear_creatures();

    const auto spawn_npc = []( const int x, const int y, const std::string & npc_class ) {
        const string_id<npc_template> test_guy( npc_class );
        const character_id model_id = g->m.place_npc( point( 10, 10 ), test_guy, true );
        g->load_npcs();

        npc *guy = g->find_npc( model_id );
        REQUIRE( guy != nullptr );
        CHECK( !guy->in_vehicle );
        guy->setpos( g->u.pos() + point( x, y ) );
        return guy;
    };

    npc *hostile = spawn_npc( 0, 1, "thug" );
    REQUIRE( rl_dist( g->u.pos(), hostile->pos() ) <= 1 );
    hostile->set_attitude( NPCATT_KILL );
    hostile->name = "Enemy NPC";

    INFO( get_list_of_npcs( "NPCs after spawning one" ) );

    hostile->regen_ai_cache();
    REQUIRE( hostile->current_target() != nullptr );
    CHECK( hostile->current_target() == static_cast<Creature *>( &g->u ) );
}
