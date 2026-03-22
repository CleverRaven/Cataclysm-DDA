#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "faction.h"
#include "field_type.h"
#include "game.h"
#include "inventory.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "memory_fast.h"
#include "npc.h"
#include "npc_opinion.h"
#include "npctalk.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "sounds.h"
#include "type_id.h"

static const ter_str_id ter_t_wall( "t_wall" );

static const activity_id ACT_OPERATION( "ACT_OPERATION" );
static const trait_id trait_IGNORE_SOUND( "IGNORE_SOUND" );
static const itype_id itype_machete( "machete" );

static constexpr int MAX_MOVES_PER_TURN = 500;

static void boost_avatar_hp()
{
    avatar &u = get_avatar();
    for( const bodypart_id &bp : u.get_all_body_parts() ) {
        u.set_part_hp_cur( bp, 999 );
    }
}

static void run_npc_turns( npc &guy, int n )
{
    for( int i = 0; i < n && !guy.is_dead(); i++ ) {
        guy.set_moves( guy.get_speed() );
        int iters = 0;
        while( guy.get_moves() > 0 && !guy.is_dead() && iters++ < MAX_MOVES_PER_TURN ) {
            guy.move();
        }
        calendar::turn += 1_turns;
    }
}

static npc *spawn_test_npc( const tripoint_bub_ms &pos, bool minimal = false )
{
    shared_ptr_fast<npc> guy = make_shared_fast<npc>();
    guy->normalize();
    if( minimal ) {
        guy->setID( g->assign_npc_id() );
    } else {
        guy->randomize();
    }
    guy->spawn_at_precise( get_map().get_abs( pos ) );
    overmap_buffer.insert_npc( guy );
    g->load_npcs();
    guy->setpos( get_map(), pos );
    return g->find_npc( guy->getID() );
}

TEST_CASE( "npc_escapes_dangerous_field", "[npcmove]" )
{
    g->faction_manager_ptr->create_if_needed();
    clear_map();
    clear_creatures();
    clear_npcs();
    clear_avatar();
    set_time_to_day();

    map &here = get_map();
    const tripoint_bub_ms player_pos( 60, 60, 0 );
    g->place_player( player_pos );

    const tripoint_bub_ms npc_pos( 62, 60, 0 );
    npc *guy = spawn_test_npc( npc_pos );
    REQUIRE( guy != nullptr );
    guy->set_attitude( NPCATT_FOLLOW );

    here.add_field( npc_pos, fd_fire, 2, 1_hours );
    REQUIRE( here.get_field( npc_pos, fd_fire ) != nullptr );

    run_npc_turns( *guy, 3 );

    CHECK( here.get_field( guy->pos_bub(), fd_fire ) == nullptr );
}

TEST_CASE( "hostile_npc_engages_player", "[npcmove]" )
{
    g->faction_manager_ptr->create_if_needed();
    clear_map();
    clear_creatures();
    clear_npcs();
    clear_avatar();
    set_time_to_day();

    const tripoint_bub_ms player_pos( 60, 60, 0 );
    g->place_player( player_pos );
    // Unarmed, unarmored player so NPC engages instead of fleeing
    get_avatar().remove_weapon();
    get_avatar().clear_worn();
    boost_avatar_hp();

    const tripoint_bub_ms npc_pos( 60, 57, 0 );
    npc *hostile = spawn_test_npc( npc_pos );
    REQUIRE( hostile != nullptr );
    hostile->set_attitude( NPCATT_KILL );
    hostile->personality.bravery = 10;
    hostile->personality.aggression = 10;
    hostile->op_of_u.fear = -100;
    hostile->op_of_u.anger = 100;
    hostile->mem_combat.panic = 0;
    hostile->clear_worn();
    hostile->inv->clear();
    item machete( itype_machete );
    REQUIRE( hostile->wield( machete ) );
    hostile->regen_ai_cache();

    run_npc_turns( *hostile, 3 );

    CHECK( rl_dist( hostile->pos_bub(), player_pos ) <= 1 );
}

TEST_CASE( "npc_sound_investigation_vs_ignore_sound", "[npcmove]" )
{
    g->faction_manager_ptr->create_if_needed();
    clear_map();
    clear_creatures();
    clear_npcs();
    clear_avatar();
    set_time_to_day();

    map &here = get_map();
    // Player far away so follow behavior doesn't interfere
    const tripoint_bub_ms player_pos( 100, 60, 0 );
    g->place_player( player_pos );

    const tripoint_bub_ms npc_pos( 60, 60, 0 );
    const tripoint_bub_ms sound_pos( 60, 52, 0 );

    for( int x = 55; x <= 65; x++ ) {
        here.ter_set( tripoint_bub_ms( x, 56, 0 ), ter_t_wall );
    }
    here.build_map_cache( 0 );

    // Identical NPC setup for both sections; IGNORE_SOUND is the sole variable.
    // Minimal spawn avoids seed-dependent faction zone flags that block
    // handle_sound() via NPC_INVESTIGATE_ONLY / NPC_NO_INVESTIGATE checks.
    npc *guy = spawn_test_npc( npc_pos, true );
    REQUIRE( guy != nullptr );
    guy->set_attitude( NPCATT_NULL );
    guy->mission = NPC_MISSION_SHELTER;
    guy->rules.clear_flag( ally_rule::ignore_noise );
    guy->base_location = guy->pos_abs_omt();
    guy->goal = npc::no_goal_point;
    item machete( itype_machete );
    guy->wield( machete );

    REQUIRE( !guy->sees( here, sound_pos ) );
    REQUIRE( rl_dist( guy->pos_bub(), sound_pos ) <= 10 );

    const int initial_dist = rl_dist( guy->pos_bub(), sound_pos );

    SECTION( "investigates sound" ) {
        for( int i = 0; i < 5 && !guy->is_dead(); i++ ) {
            guy->handle_sound( sounds::sound_t::alarm, "bang", 10, sound_pos );
            guy->set_moves( guy->get_speed() );
            int iters = 0;
            while( guy->get_moves() > 0 && !guy->is_dead() && iters++ < MAX_MOVES_PER_TURN ) {
                guy->move();
            }
            calendar::turn += 1_turns;
        }
        CHECK( rl_dist( guy->pos_bub(), sound_pos ) < initial_dist );
    }

    SECTION( "IGNORE_SOUND prevents investigation" ) {
        guy->set_mutation( trait_IGNORE_SOUND );
        for( int i = 0; i < 5 && !guy->is_dead(); i++ ) {
            guy->handle_sound( sounds::sound_t::alarm, "bang", 10, sound_pos );
            guy->set_moves( guy->get_speed() );
            int iters = 0;
            while( guy->get_moves() > 0 && !guy->is_dead() && iters++ < MAX_MOVES_PER_TURN ) {
                guy->move();
            }
            calendar::turn += 1_turns;
        }
        CHECK( rl_dist( guy->pos_bub(), sound_pos ) >= initial_dist );
    }
}

TEST_CASE( "npc_follows_player", "[npcmove]" )
{
    g->faction_manager_ptr->create_if_needed();
    clear_map();
    clear_creatures();
    clear_npcs();
    clear_avatar();
    set_time_to_day();

    const tripoint_bub_ms player_pos( 60, 60, 0 );
    g->place_player( player_pos );

    const tripoint_bub_ms npc_pos( 68, 60, 0 );
    npc *guy = spawn_test_npc( npc_pos );
    REQUIRE( guy != nullptr );
    talk_function::follow( *guy );

    const int initial_dist = rl_dist( guy->pos_bub(), player_pos );

    run_npc_turns( *guy, 5 );

    CHECK( rl_dist( guy->pos_bub(), player_pos ) < initial_dist );
}

TEST_CASE( "npc_activity_short_circuits_movement", "[npcmove]" )
{
    g->faction_manager_ptr->create_if_needed();
    clear_map();
    clear_creatures();
    clear_npcs();
    clear_avatar();
    set_time_to_day();

    const tripoint_bub_ms player_pos( 60, 60, 0 );
    g->place_player( player_pos );

    const tripoint_bub_ms npc_pos( 65, 60, 0 );
    npc *guy = spawn_test_npc( npc_pos );
    REQUIRE( guy != nullptr );

    guy->assign_activity( player_activity( ACT_OPERATION, 10000 ) );

    const tripoint_bub_ms start = guy->pos_bub();
    run_npc_turns( *guy, 3 );

    CHECK( guy->pos_bub() == start );
    CHECK( guy->activity.id() == ACT_OPERATION );
}
