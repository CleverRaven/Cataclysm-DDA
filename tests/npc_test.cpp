#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "common_types.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "faction.h"
#include "field.h"
#include "field_type.h"
#include "game.h"
#include "inventory.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "memory_fast.h"
#include "monster.h"
#include "npc.h"
#include "npctalk.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "point.h"
#include "test_data.h"
#include "text_snippets.h"
#include "translation.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"

class Creature;

static const efftype_id effect_bouldering( "bouldering" );
static const efftype_id effect_sleep( "sleep" );

static const item_group_id Item_spawn_data_SUS_trash_forest_manmade( "SUS_trash_forest_manmade" );
static const item_group_id Item_spawn_data_test_NPC_guns( "test_NPC_guns" );

static const itype_id itype_M24( "M24" );
static const itype_id itype_bat( "bat" );
static const itype_id itype_debug_backpack( "debug_backpack" );
static const itype_id itype_leather_belt( "leather_belt" );

static const trait_id trait_WEB_WEAVER( "WEB_WEAVER" );

static const vpart_id vpart_frame( "frame" );
static const vpart_id vpart_seat( "seat" );

static const vproto_id vehicle_prototype_none( "none" );

static void on_load_test( npc &who, const time_duration &from, const time_duration &to )
{
    calendar::turn = calendar::turn_zero + from;
    who.on_unload();
    calendar::turn = calendar::turn_zero + to;
    who.on_load( &get_map() );
}

static void test_needs( const npc &who, const numeric_interval<int> &hunger,
                        const numeric_interval<int> &thirst,
                        const numeric_interval<int> &sleepiness )
{
    CHECK( who.get_hunger() <= hunger.max );
    CHECK( who.get_hunger() >= hunger.min );
    CHECK( who.get_thirst() <= thirst.max );
    CHECK( who.get_thirst() >= thirst.min );
    CHECK( who.get_sleepiness() <= sleepiness.max );
    CHECK( who.get_sleepiness() >= sleepiness.min );
}

static npc create_model()
{
    npc model_npc;
    model_npc.normalize();
    model_npc.randomize( npc_class_id::NULL_ID() );
    for( const trait_id &tr : model_npc.get_mutations() ) {
        model_npc.unset_mutation( tr );
    }
    model_npc.set_hunger( 0 );
    model_npc.set_thirst( 0 );
    model_npc.set_sleepiness( 0 );
    model_npc.remove_effect( effect_sleep );
    // An ugly hack to prevent NPC falling asleep during testing due to massive sleepiness
    model_npc.set_mutation( trait_WEB_WEAVER );

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

static std::string get_list_of_monsters( const std::string &title )
{
    std::ostringstream mon_list;
    mon_list << title << ":\n";
    for( const shared_ptr_fast<monster> &m : get_creature_tracker().get_monsters_list() ) {
        mon_list << "  " << m.get() << ": " << m->name() << '\n';
    }
    return mon_list.str();
}

TEST_CASE( "on_load-sane-values", "[.]" )
{
    SECTION( "Awake for 10 minutes, gaining hunger/thirst/sleepiness" ) {
        npc test_npc = create_model();
        const int five_min_ticks = 2;
        on_load_test( test_npc, 0_turns, 5_minutes * five_min_ticks );
        const int margin = 2;

        const numeric_interval<int> hunger( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> thirst( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> sleepiness( five_min_ticks, margin, margin );

        test_needs( test_npc, hunger, thirst, sleepiness );
    }

    SECTION( "Awake for 2 days, gaining hunger/thirst/sleepiness" ) {
        npc test_npc = create_model();
        const double five_min_ticks = 2_days / 5_minutes;
        on_load_test( test_npc, 0_turns, 5_minutes * five_min_ticks );

        const int margin = 20;
        const numeric_interval<int> hunger( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> thirst( five_min_ticks / 4, margin, margin );
        const numeric_interval<int> sleepiness( five_min_ticks, margin, margin );

        test_needs( test_npc, hunger, thirst, sleepiness );
    }

    SECTION( "Sleeping for 6 hours, gaining hunger/thirst (not testing sleepiness due to lack of effects processing)" ) {
        npc test_npc = create_model();
        test_npc.add_effect( effect_sleep, 6_hours );
        test_npc.set_sleepiness( 1000 );
        const double five_min_ticks = 6_hours / 5_minutes;
        /*
        // sleepiness regeneration starts at 1 per 5min, but linearly increases to 2 per 5min at 2 hours or more
        const int expected_sleepiness_change =
            ((1.0f + 2.0f) / 2.0f * 2_hours / 5_minutes ) +
            (2.0f * (6_hours - 2_hours) / 5_minutes);
        */
        on_load_test( test_npc, 0_turns, 5_minutes * five_min_ticks );

        const int margin = 10;
        const numeric_interval<int> hunger( five_min_ticks / 8, margin, margin );
        const numeric_interval<int> thirst( five_min_ticks / 8, margin, margin );
        const numeric_interval<int> sleepiness( test_npc.get_sleepiness(), 0, 0 );

        test_needs( test_npc, hunger, thirst, sleepiness );
    }
}

TEST_CASE( "on_load-similar-to-per-turn", "[.]" )
{
    SECTION( "Awake for 10 minutes, gaining hunger/thirst/sleepiness" ) {
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
        const numeric_interval<int> sleepiness( iterated_npc.get_sleepiness(), margin, margin );

        test_needs( on_load_npc, hunger, thirst, sleepiness );
    }

    SECTION( "Awake for 6 hours, gaining hunger/thirst/sleepiness" ) {
        npc on_load_npc = create_model();
        npc iterated_npc = create_model();
        const double five_min_ticks = 6_hours / 5_minutes;
        on_load_test( on_load_npc, 0_turns, 5_minutes * five_min_ticks );
        for( time_duration turn = 0_turns; turn < 5_minutes * five_min_ticks; turn += 1_turns ) {
            iterated_npc.update_body( calendar::turn_zero + turn,
                                      calendar::turn_zero + turn + 1_turns );
        }

        const int margin = 10;
        const numeric_interval<int> hunger( iterated_npc.get_hunger(), margin, margin );
        const numeric_interval<int> thirst( iterated_npc.get_thirst(), margin, margin );
        const numeric_interval<int> sleepiness( iterated_npc.get_sleepiness(), margin, margin );

        test_needs( on_load_npc, hunger, thirst, sleepiness );
    }
}

TEST_CASE( "snippet-tag-test" )
{
    // Actually used tags
    static const std::set<std::string> npc_talk_tags = {
        {
            "<name_b>", "<name_g>", "<hungry>", "<thirsty>",
            "<general_danger>", "<kill_npc>", "<combat_noise_warning>",
            "<fire_in_the_hole>", "<hello>", "<swear!>",
            "<lets_talk>", "<freaking>", "<acknowledged>",
            "<wait>", "<keep_up>", "<im_leaving_you>",
            "<done_conversation_section>", "<end_talking_bye>",
            "<end_talking_later>", "<end_talking_leave>",
        }
    };

    for( const auto &tag : npc_talk_tags ) {
        for( int i = 0; i < 100; i++ ) {
            CHECK( SNIPPET.random_from_category( tag ).has_value() );
        }
    }

    // Special tags, those should have no replacements
    static const std::set<std::string> special_tags = {
        {
            "<yrwp>", "<mywp>", "<ammo>"
        }
    };

    for( const std::string &tag : special_tags ) {
        for( int i = 0; i < 100; i++ ) {
            CHECK( !SNIPPET.random_from_category( tag ).has_value() );
        }
    }
}

/* Test setup. Player should always be at top-left.
 *
 * U is the player, V is vehicle, # is wall, R is rubble & acid with NPC on it,
 * A is acid with NPC on it, W/M is vehicle & acid with (follower/non-follower) NPC on it,
 * B/C is acid with (follower/non-follower) NPC on it.
 */
static constexpr int height = 5, width = 17;
// NOLINTNEXTLINE(cata-use-mdarray,modernize-avoid-c-arrays)
static constexpr char setup[height][width + 1] = {
    "U ###############",
    "V #R#AAA#W# # #C#",
    "  #A#A#A# #M#B# #",
    "  ###AAA#########",
    "    #####        ",
};

static void check_npc_movement( const tripoint_bub_ms &origin )
{
    INFO( "Should not crash from infinite recursion" );
    creature_tracker &creatures = get_creature_tracker();
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            switch( setup[y][x] ) {
                case 'A':
                case 'R':
                case 'W':
                case 'M':
                case 'B':
                case 'C':
                    tripoint_bub_ms p = origin + point( x, y );
                    npc *guy = creatures.creature_at<npc>( p );
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
                tripoint_bub_ms p = origin + point( x, y );
                npc *guy = creatures.creature_at<npc>( p );
                REQUIRE( guy != nullptr );
                CHECK( !guy->has_effect( effect_bouldering ) );
            }
        }
    }

    INFO( "NPC on rubbles should not lose unstable footing status" );
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            if( setup[y][x] == 'R' ) {
                tripoint_bub_ms p = origin + point( x, y );
                npc *guy = creatures.creature_at<npc>( p );
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
                    tripoint_bub_ms p = origin + point( x, y );
                    npc *guy = creatures.creature_at<npc>( p );
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
                    tripoint_bub_ms p = origin + point( x, y );
                    npc *guy = creatures.creature_at<npc>( p );
                    CHECK( guy == nullptr );
                    break;
            }
        }
    }
}

static npc *make_companion( const tripoint_bub_ms &npc_pos )
{
    map &here = get_map();

    shared_ptr_fast<npc> guy = make_shared_fast<npc>();
    guy->normalize();
    guy->randomize();
    guy->spawn_at_precise( get_map().get_abs( npc_pos ) );
    overmap_buffer.insert_npc( guy );
    g->load_npcs();
    guy->companion_mission_role_id.clear();
    guy->guard_pos = std::nullopt;
    clear_character( *guy );
    guy->setpos( here, npc_pos );
    talk_function::follow( *guy );

    return get_creature_tracker().creature_at<npc>( npc_pos );
}

TEST_CASE( "npc-board-player-vehicle" )
{
    REQUIRE_FALSE( test_data::npc_boarding_data.empty() );

    for( std::pair<const std::string, npc_boarding_test_data> &given : test_data::npc_boarding_data ) {
        GIVEN( given.first ) {
            npc_boarding_test_data &data = given.second;
            g->place_player( data.player_pos );
            clear_map();
            map &here = get_map();
            Character &pc = get_player_character();

            vehicle *veh = here.add_vehicle( data.veh_prototype, data.player_pos, 0_degrees, 100, 2 );
            REQUIRE( veh != nullptr );
            here.board_vehicle( data.player_pos, &pc );
            REQUIRE( pc.in_vehicle );

            npc *companion = make_companion( data.npc_pos );
            REQUIRE( companion != nullptr );
            REQUIRE( companion->get_attitude() == NPCATT_FOLLOW );
            REQUIRE( companion->path_settings->allow_open_doors );
            REQUIRE( companion->path_settings->allow_unlock_doors );

            /* Uncomment for some extra info when test fails
            debug_mode = true;
            debugmode::enabled_filters.clear();
            debugmode::enabled_filters.emplace_back( debugmode::DF_NPC );
            REQUIRE( debugmode::enabled_filters.size() == 1 );
            */

            int turns = 0;
            while( turns++ < 100 && companion->pos_bub() != data.npc_target ) {
                companion->set_moves( 100 );
                /* Uncommment for extra debug info
                tripoint npc_pos = companion->pos();
                optional_vpart_position vp = here.veh_at( npc_pos );
                vehicle *veh = veh_pointer_or_null( vp );
                add_msg( m_info, "%s is at %d %d %d and sees t: %s, f: %s, v: %s (%s)",
                         companion->name, npc_pos.x, npc_pos.y, npc_pos.z,
                         here.tername( npc_pos ),
                         here.furnname( npc_pos ),
                         vp ? remove_color_tags( vp->part_displayed()->part().name() ) : "",
                         veh ? veh->name : " " );
                */
                companion->move();
            }

            CAPTURE( companion->path );
            if( !companion->path.empty() ) {
                tripoint_bub_ms &p = companion->path.front();

                int part = -1;
                const vehicle *veh = here.veh_at_internal( p, part );
                if( veh ) {
                    const vehicle_part &vp = veh->part( part );
                    CHECK_FALSE( vp.info().has_flag( VPFLAG_OPENABLE ) );

                    std::string part_name = remove_color_tags( vp.name() );
                    UNSCOPED_INFO( "target tile: " << p.to_string() << " - vehicle part : " << part_name );

                    int hp = vp.hp();
                    int bash = companion->path_settings->bash_strength;
                    UNSCOPED_INFO( "part hp: " << hp << " - bash strength: " << bash );

                    const auto vpobst = vpart_position( const_cast<vehicle &>( *veh ), part ).obstacle_at_part();
                    part = vpobst ? vpobst->part_index() : -1;
                    int open = veh->next_part_to_open( part, true );
                    int lock = veh->next_part_to_unlock( part, true );

                    if( open >= 0 ) {
                        const vehicle_part &vp_open = veh->part( open );
                        UNSCOPED_INFO( "open part " << vp_open.name() );
                    }
                    if( lock >= 0 ) {
                        const vehicle_part &vp_lock = veh->part( lock );
                        UNSCOPED_INFO( "lock part " << vp_lock.name() );
                    }
                }
            }
            CHECK( companion->pos_bub() == data.npc_target );
        }
    }
}

TEST_CASE( "npc-movement" )
{
    const ter_id t_wall_metal( "t_wall_metal" );
    const ter_id t_floor( "t_floor" );
    const furn_id f_rubble( "f_rubble" );

    g->place_player( { 60, 60, 0 } );

    clear_map();

    creature_tracker &creatures = get_creature_tracker();
    Character &player_character = get_player_character();
    map &here = get_map();
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            const char type = setup[y][x];
            const tripoint_bub_ms p = player_character.pos_bub() + point( x, y );
            // create walls
            if( type == '#' ) {
                here.ter_set( p, t_wall_metal );
            } else {
                here.ter_set( p, t_floor );
            }
            // spawn acid
            // a copy is needed because we will remove elements from it
            const field fs = here.field_at( p );
            for( const auto &f : fs ) {
                here.remove_field( p, f.first );
            }
            if( type == 'A' || type == 'R' || type == 'W' || type == 'M'
                || type == 'B' || type == 'C' ) {

                here.add_field( p, fd_acid, 3 );
            }
            // spawn rubbles
            if( type == 'R' ) {
                here.furn_set( p, f_rubble );
            } else {
                here.furn_set( p, furn_str_id::NULL_ID() );
            }
            // create vehicles
            if( type == 'V' || type == 'W' || type == 'M' ) {
                vehicle *veh = here.add_vehicle( vehicle_prototype_none, p, 270_degrees, 0, 0 );
                REQUIRE( veh != nullptr );
                veh->install_part( here, point_rel_ms::zero, vpart_frame );
                veh->install_part( here, point_rel_ms::zero, vpart_seat );
                here.add_vehicle_to_cache( veh );
            }
            // spawn npcs
            if( type == 'A' || type == 'R' || type == 'W' || type == 'M'
                || type == 'B' || type == 'C' ) {

                shared_ptr_fast<npc> guy = make_shared_fast<npc>();
                guy->normalize();
                guy->randomize();
                guy->remove_worn_items_with( [&]( item & armor ) {
                    return armor.covers( bodypart_id( "foot_r" ) ) || armor.covers( bodypart_id( "foot_l" ) );
                } );
                REQUIRE( !guy->is_immune_field( fd_acid ) );
                guy->spawn_at_precise( get_map().get_abs( p ) );
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
            const tripoint_bub_ms p = player_character.pos_bub() + point( x, y );
            if( type == '#' ) {
                REQUIRE( !here.passable( p ) );
            } else {
                REQUIRE( here.passable( p ) );
            }
            if( type == 'R' ) {
                REQUIRE( here.has_flag( "UNSTABLE", p ) );
            } else {
                REQUIRE( !here.has_flag( "UNSTABLE", p ) );
            }
            if( type == 'V' || type == 'W' || type == 'M' ) {
                REQUIRE( here.veh_at( p ).part_with_feature( VPFLAG_BOARDABLE, true ).has_value() );
            } else {
                REQUIRE( !here.veh_at( p ).part_with_feature( VPFLAG_BOARDABLE, true ).has_value() );
            }
            npc *guy = creatures.creature_at<npc>( p );
            if( type == 'A' || type == 'R' || type == 'W' || type == 'M'
                || type == 'B' || type == 'C' ) {

                REQUIRE( guy != nullptr );
                REQUIRE( guy->is_dangerous_fields( here.field_at( p ) ) );
            } else {
                REQUIRE( guy == nullptr );
            }
        }
    }

    SECTION( "NPCs escape dangerous terrain by pushing other NPCs" ) {
        check_npc_movement( player_character.pos_bub() );
    }

    SECTION( "Player in vehicle & NPCs escaping dangerous terrain" ) {
        const tripoint_bub_ms origin = player_character.pos_bub();

        for( int y = 0; y < height; ++y ) {
            for( int x = 0; x < width; ++x ) {
                if( setup[y][x] == 'V' ) {
                    g->place_player( player_character.pos_bub() + point( x, y ) );
                    break;
                }
            }
        }

        check_npc_movement( origin );
    }
}

TEST_CASE( "npc_can_target_player" )
{
    g->faction_manager_ptr->create_if_needed();

    clear_map();
    clear_avatar();
    set_time_to_day();

    Character &player_character = get_player_character();
    npc &hostile = spawn_npc( player_character.pos_bub().xy() + point::south, "thug" );
    REQUIRE( rl_dist( player_character.pos_bub(), hostile.pos_bub() ) <= 1 );
    hostile.set_attitude( NPCATT_KILL );
    hostile.name = "Enemy NPC";

    INFO( get_list_of_npcs( "NPCs after spawning one" ) );
    INFO( get_list_of_monsters( "Monsters after spawning NPC" ) );

    hostile.regen_ai_cache();
    REQUIRE( hostile.current_target() != nullptr );
    CHECK( hostile.current_target() == static_cast<Creature *>( &player_character ) );
}

static void advance_turn( Character &guy )
{
    guy.process_turn();
    calendar::turn += 1_turns;
}

TEST_CASE( "npc_uses_guns", "[npc_ai]" )
{
    g->faction_manager_ptr->create_if_needed();

    clear_map();
    clear_avatar();
    set_time_to_day();

    Character &player_character = get_player_character();
    point five_tiles_south = {0, 5};
    npc &hostile = spawn_npc( player_character.pos_bub().xy() + five_tiles_south, "thug" );
    hostile.clear_worn();
    hostile.invalidate_crafting_inventory();
    hostile.inv->clear();
    hostile.remove_weapon();
    hostile.clear_mutations();
    hostile.mutation_category_level.clear();
    hostile.clear_bionics();
    REQUIRE( rl_dist( player_character.pos_bub(), hostile.pos_bub() ) >= 4 );
    hostile.set_attitude( NPCATT_KILL );
    hostile.name = "Enemy NPC";
    arm_shooter( hostile, itype_M24 );
    // Give them an excuse to use it by making them aware the player (an enemy) exists
    arm_shooter( player_character, itype_M24 );
    hostile.regen_ai_cache();
    float danger_around = hostile.danger_assessment();
    CHECK( danger_around > 1.0f );
    // Now give them a TON of junk
    for( item &some_trash : item_group::items_from( Item_spawn_data_SUS_trash_forest_manmade ) ) {
        hostile.i_add( some_trash );
    }
    hostile.wield_better_weapon();

    advance_turn( hostile );
    advance_turn( hostile );
    advance_turn( hostile );

    REQUIRE( hostile.get_wielded_item().get_item()->is_gun() );
}

TEST_CASE( "npc_prefers_guns", "[npc_ai]" )
{
    g->faction_manager_ptr->create_if_needed();

    clear_map();
    clear_avatar();
    set_time_to_day();

    Character &player_character = get_player_character();
    point five_tiles_south = {0, 5};
    npc &hostile = spawn_npc( player_character.pos_bub().xy() + five_tiles_south, "thug" );
    hostile.clear_worn();
    hostile.invalidate_crafting_inventory();
    hostile.inv->clear();
    hostile.remove_weapon();
    hostile.clear_mutations();
    hostile.mutation_category_level.clear();
    hostile.clear_bionics();
    REQUIRE( rl_dist( player_character.pos_bub(), hostile.pos_bub() ) >= 4 );
    hostile.set_attitude( NPCATT_KILL );
    hostile.name = "Enemy NPC";
    item backpack( itype_debug_backpack );
    hostile.wear_item( backpack );
    // Give them a bat and a belt
    hostile.i_add( item( itype_leather_belt ) );
    hostile.i_add( item( itype_bat ) );
    // Make them realize we exist and COULD maybe hurt them! Or something. Otherwise they won't re-wield.
    arm_shooter( player_character, itype_M24 );
    hostile.regen_ai_cache();
    float danger_around = hostile.danger_assessment();
    CHECK( danger_around > 1.0f );
    hostile.wield_better_weapon();
    CAPTURE( hostile.get_wielded_item().get_item()->tname() );
    CHECK( !hostile.get_wielded_item().get_item()->is_gun() );
    // Now give them a gun and some magazines
    for( item &some_gun_item : item_group::items_from( Item_spawn_data_test_NPC_guns ) ) {
        hostile.i_add( some_gun_item );
    }
    hostile.wield_better_weapon();
    CHECK( hostile.get_wielded_item().get_item()->is_gun() );

    //Now give them some time to choose their belt instead
    advance_turn( hostile );
    advance_turn( hostile );
    advance_turn( hostile );

    CAPTURE( hostile.get_wielded_item().get_item()->tname() );
    REQUIRE( hostile.get_wielded_item().get_item()->is_gun() );
}
