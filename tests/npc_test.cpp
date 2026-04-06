#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "basecamp.h"
#include "behavior.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "character_attire.h"
#include "character_id.h"
#include "character_oracle.h"
#include "clzones.h"
#include "common_types.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "damage.h"
#include "debug.h"
#include "faction.h"
#include "field.h"
#include "field_type.h"
#include "game.h"
#include "inventory.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "itype.h"
#include "list.h"
#include "map.h"
#include "map_helpers.h"
#include "mapdata.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "map_selector.h"
#include "memory_fast.h"
#include "messages.h"
#include "iexamine.h"
#include "monster.h"
#include "npc.h"
#include "npc_class.h"
#include "npctalk.h"
#include "options_helpers.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "pocket_type.h"
#include "pimpl.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "stomach.h"
#include "test_data.h"
#include "text_snippets.h"
#include "translation.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "viewer.h"
#include "vpart_position.h"
#include "weather.h"

class Creature;
enum npc_action : int;

static const activity_id ACT_FORAGE( "ACT_FORAGE" );
static const activity_id ACT_HARVEST( "ACT_HARVEST" );
static const activity_id ACT_START_FIRE( "ACT_START_FIRE" );

static const efftype_id effect_bouldering( "bouldering" );
static const efftype_id effect_catch_up( "catch_up" );
static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_meth( "meth" );
static const efftype_id effect_npc_suspend( "npc_suspend" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_wet( "wet" );

static const faction_id faction_robofac( "robofac" );
static const faction_id faction_your_followers( "your_followers" );

static const furn_str_id furn_f_bed( "f_bed" );
static const furn_str_id furn_f_locker( "f_locker" );
static const furn_str_id furn_f_makeshift_bed( "f_makeshift_bed" );

static const item_group_id Item_spawn_data_SUS_trash_forest_manmade( "SUS_trash_forest_manmade" );
static const item_group_id Item_spawn_data_test_NPC_guns( "test_NPC_guns" );
static const item_group_id Item_spawn_data_test_bottle_water( "test_bottle_water" );

static const itype_id itype_2x4( "2x4" );
static const itype_id itype_M24( "M24" );
static const itype_id itype_apron_leather( "apron_leather" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_bat( "bat" );
static const itype_id itype_crackers( "crackers" );
static const itype_id itype_debug_backpack( "debug_backpack" );
static const itype_id itype_honeycomb( "honeycomb" );
static const itype_id itype_leather_belt( "leather_belt" );
static const itype_id itype_lighter( "lighter" );
static const itype_id itype_marloss_berry( "marloss_berry" );
static const itype_id itype_marloss_gel( "marloss_gel" );
static const itype_id itype_meat( "meat" );
static const itype_id itype_meat_cooked( "meat_cooked" );
static const itype_id itype_meat_tainted( "meat_tainted" );
static const itype_id itype_mutagen( "mutagen" );
static const itype_id itype_orange( "orange" );
static const itype_id itype_sandwich_cheese_grilled( "sandwich_cheese_grilled" );
static const itype_id itype_space_cake( "space_cake" );
static const itype_id itype_sweater( "sweater" );
static const itype_id itype_water_clean( "water_clean" );

static const npc_class_id NC_ROBOFAC_INTERCOM_DAY( "NC_ROBOFAC_INTERCOM_DAY" );
static const npc_class_id NC_ROBOFAC_INTERCOM_NIGHT( "NC_ROBOFAC_INTERCOM_NIGHT" );
static const npc_class_id NC_ROBOFAC_INTERCOM_SUPPLY( "NC_ROBOFAC_INTERCOM_SUPPLY" );
static const npc_class_id test_shop_class( "test_shop_class" );

static const string_id<behavior::node_t> behavior_node_t_npc_decision( "npc_decision" );

static const ter_str_id ter_t_concrete( "t_concrete" );
static const ter_str_id ter_t_concrete_wall( "t_concrete_wall" );
static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_ponywall( "t_ponywall" );
static const ter_str_id ter_t_swater_sh( "t_swater_sh" );
static const ter_str_id ter_t_tree_apple( "t_tree_apple" );
static const ter_str_id ter_t_tree_birch( "t_tree_birch" );
static const ter_str_id ter_t_tree_dead( "t_tree_dead" );
static const ter_str_id ter_t_tree_pine( "t_tree_pine" );
static const ter_str_id ter_t_tree_walnut( "t_tree_walnut" );
static const ter_str_id ter_t_underbrush( "t_underbrush" );
static const ter_str_id ter_t_wall( "t_wall" );
static const ter_str_id ter_t_wall_glass( "t_wall_glass" );
static const ter_str_id ter_t_water_dispenser( "t_water_dispenser" );
static const ter_str_id ter_t_water_sh( "t_water_sh" );

static const trait_id trait_IGNORE_SOUND( "IGNORE_SOUND" );
static const trait_id trait_INTERCOM_OPERATOR( "INTERCOM_OPERATOR" );
static const trait_id trait_RETURN_TO_START_POS( "RETURN_TO_START_POS" );
static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );
static const trait_id trait_SAPROVORE( "SAPROVORE" );
static const trait_id trait_TRADE_BACKEND( "TRADE_BACKEND" );
static const trait_id trait_WEB_WEAVER( "WEB_WEAVER" );

static const vpart_id vpart_cargo_lock( "cargo_lock" );
static const vpart_id vpart_frame( "frame" );
static const vpart_id vpart_seat( "seat" );
static const vpart_id vpart_trunk_floor( "trunk_floor" );

static const vproto_id vehicle_prototype_none( "none" );
static const vproto_id vehicle_prototype_test_rv( "test_rv" );
static const vproto_id vehicle_prototype_test_shopping_cart( "test_shopping_cart" );

static const zone_type_id zone_type_CAMP_FOOD( "CAMP_FOOD" );
static const zone_type_id zone_type_CAMP_STORAGE( "CAMP_STORAGE" );
static const zone_type_id zone_type_NO_NPC_PICKUP( "NO_NPC_PICKUP" );
static const zone_type_id zone_type_NPC_NO_GO( "NPC_NO_GO" );

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
            clear_map_without_vision();
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
                    int bash = std::accumulate( companion->path_settings->bash_strength.begin(),
                                                companion->path_settings->bash_strength.end(), 0,
                    []( int so_far, const std::pair<damage_type_id, int> &dam ) {
                        return so_far + static_cast<int>( dam.second * dam.first->bash_conversion_factor );
                    } );
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

    clear_map_without_vision();

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

    clear_map_without_vision();
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

    clear_map_without_vision();
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
    hostile.set_body();
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

    clear_map_without_vision();
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
    hostile.set_body();
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
    CAPTURE( hostile.evaluate_weapon( null_item_reference() ) );
    CAPTURE( hostile.evaluate_weapon( item( itype_bat ) ) );
    hostile.wield_better_weapon();
    REQUIRE( hostile.get_wielded_item() );
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

TEST_CASE( "npc_extracts_weapon_from_wielded_container", "[npc_ai]" )
{
    g->faction_manager_ptr->create_if_needed();

    clear_map_without_vision();
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
    hostile.set_body();
    hostile.mutation_category_level.clear();
    hostile.clear_bionics();
    REQUIRE( rl_dist( player_character.pos_bub(), hostile.pos_bub() ) >= 4 );
    hostile.set_attitude( NPCATT_KILL );
    hostile.name = "Enemy NPC";

    // Wield a non-melee container with a melee weapon inside
    item backpack( itype_debug_backpack );
    backpack.force_insert_item( item( itype_bat ), pocket_type::CONTAINER );
    REQUIRE( !backpack.empty() );
    hostile.set_wielded_item( backpack );
    REQUIRE( hostile.get_wielded_item() );
    REQUIRE( !hostile.get_wielded_item()->is_melee() );

    // Trigger danger awareness so the NPC considers switching weapons
    arm_shooter( player_character, itype_M24 );
    hostile.regen_ai_cache();
    CHECK( hostile.danger_assessment() > 1.0f );

    // evaluate_best_weapon should find the bat inside the backpack
    item *best = hostile.evaluate_best_weapon();
    item_location wielded = hostile.get_wielded_item();
    CHECK( best != wielded.get_item() );
    CHECK( best->typeId() == itype_bat );

    // wield_better_weapon should extract the bat and wield it
    hostile.wield_better_weapon();
    REQUIRE( hostile.get_wielded_item() );
    CHECK( hostile.get_wielded_item()->typeId() == itype_bat );

    // The backpack should no longer be wielded
    CHECK( hostile.get_wielded_item()->typeId() != itype_debug_backpack );
}

TEST_CASE( "npc_needs_bt_diagnostic_during_move", "[npc][behavior]" )
{
    // RAII: save and restore debug globals so later tests are unaffected
    restore_on_out_of_scope<bool> restore_debug( debug_mode );
    restore_on_out_of_scope<std::unordered_set<debugmode::debug_filter>>
            restore_filters( debugmode::enabled_filters );

    clear_map_without_vision();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );

    // Make NPC hungry with food available
    guy.set_hunger( 500 );
    guy.set_stored_kcal( 1000 );
    guy.i_add( item( itype_sandwich_cheese_grilled ) );

    SECTION( "filter enabled - BT goal appears in messages" ) {
        debug_mode = true;
        debugmode::enabled_filters.clear();
        debugmode::enabled_filters.emplace( debugmode::DF_NPC_NEEDS );

        Messages::clear_messages();
        guy.set_moves( 100 );
        guy.move();

        const auto msgs = Messages::recent_messages( 100 );
        bool found_bt_msg = false;
        for( const auto &msg : msgs ) {
            if( msg.second.find( "BT needs goal" ) != std::string::npos ) {
                found_bt_msg = true;
                CHECK( msg.second.find( "eat_food" ) != std::string::npos );
                // Convergence: legacy need should appear in the same message
                CHECK( msg.second.find( "need_food" ) != std::string::npos );
                break;
            }
        }
        CHECK( found_bt_msg );
    }

    SECTION( "filter absent - no BT evaluation" ) {
        debug_mode = true;
        debugmode::enabled_filters.clear();
        // DF_NPC_NEEDS deliberately NOT added

        Messages::clear_messages();
        guy.set_moves( 100 );
        guy.move();

        const auto msgs = Messages::recent_messages( 100 );
        for( const auto &msg : msgs ) {
            CHECK( msg.second.find( "BT needs goal" ) == std::string::npos );
        }
    }
}

TEST_CASE( "npc_eats_food_with_harmless_iuse", "[npc][food]" )
{
    clear_map_without_vision();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );

    // Make NPC hungry enough that rate_food scores positively
    guy.set_hunger( 300 );
    guy.set_stored_kcal( 5000 );
    guy.set_thirst( 0 );

    // Items with use_methods that NPCs SHOULD eat:
    SECTION( "eats honeycomb" ) {
        // harmless iuse -- spawns wax, good nutrition
        guy.i_add( item( itype_honeycomb ) );
        CHECK( guy.consume_food() );
    }

    SECTION( "eats weed brownies" ) {
        // WEED_CAKE iuse -- small debuff, but edible food
        guy.i_add( item( itype_space_cake ) );
        CHECK( guy.consume_food() );
    }

    // Items without use_methods that are still rejected by other checks:
    SECTION( "refuses raw meat" ) {
        // parasites field -- caught by parasites check, not use_methods
        guy.i_add( item( itype_meat ) );
        CHECK_FALSE( guy.consume_food() );
    }

    // Items with use_methods that NPCs should NOT eat:
    SECTION( "refuses tainted meat" ) {
        // POISON iuse -- zed meat, tainted organs
        guy.i_add( item( itype_meat_tainted ) );
        CHECK_FALSE( guy.consume_food() );
    }

    SECTION( "refuses marloss berry" ) {
        // MARLOSS iuse + MYCUS_OK flag -- fungal transformation
        guy.i_add( item( itype_marloss_berry ) );
        CHECK_FALSE( guy.consume_food() );
    }

    SECTION( "refuses marloss gel" ) {
        // MARLOSS_GEL iuse -- fungal transformation, no MYCUS_OK flag
        guy.i_add( item( itype_marloss_gel ) );
        CHECK_FALSE( guy.consume_food() );
    }

    SECTION( "refuses mutagen" ) {
        // consume_drug iuse, MED type -- player-controlled mutation
        guy.i_add( item( itype_mutagen ) );
        CHECK_FALSE( guy.consume_food() );
    }

    SECTION( "saprovore eats tainted meat" ) {
        // Saprovore mutation allows eating parasitic/tainted food
        guy.set_mutation( trait_SAPROVORE );
        guy.i_add( item( itype_meat_tainted ) );
        CHECK( guy.consume_food() );
    }

    SECTION( "saprovore eats raw meat" ) {
        guy.set_mutation( trait_SAPROVORE );
        guy.i_add( item( itype_meat ) );
        CHECK( guy.consume_food() );
    }
}

TEST_CASE( "npc_no_food_mod_suppresses_complaints", "[npc][needs]" )
{
    clear_map_without_vision();
    avatar &player_character = get_avatar();
    clear_avatar();
    npc &guy = spawn_npc( player_character.pos_bub().xy() + point::east, "test_talker" );
    set_time( calendar::turn_zero + 12_hours );
    clear_character( guy );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_fac( faction_your_followers );

    guy.set_hunger( 300 );   // well above NPC_HUNGER_COMPLAIN (160)
    guy.set_thirst( 200 );   // well above NPC_THIRST_COMPLAIN (80)

    // Assert the exact preconditions that npc::complain() checks
    REQUIRE( guy.is_player_ally() );
    REQUIRE( get_player_view().sees( get_map(), guy ) );

    SECTION( "with NO_NPC_FOOD, no hunger/thirst complaints" ) {
        override_option no_food( "NO_NPC_FOOD", "true" );
        REQUIRE_FALSE( guy.needs_food() );
        CHECK_FALSE( guy.complain() );
    }

    SECTION( "without NO_NPC_FOOD, hunger complaint fires" ) {
        override_option food_on( "NO_NPC_FOOD", "false" );
        REQUIRE( guy.needs_food() );
        CHECK( guy.complain() );
    }
}

TEST_CASE( "npc_saprovore_eats_rotten_food", "[npc][food]" )
{
    clear_map_without_vision();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    guy.set_hunger( 300 );
    guy.set_stored_kcal( 5000 );
    guy.set_thirst( 0 );

    item rotten_meat( itype_meat_cooked );
    rotten_meat.set_relative_rot( 1.01 );
    REQUIRE( rotten_meat.get_relative_rot() >= 1.0 );

    SECTION( "normal NPC refuses rotten food" ) {
        guy.i_add( rotten_meat );
        CHECK_FALSE( guy.consume_food() );
    }

    SECTION( "saprovore eats rotten food" ) {
        guy.set_mutation( trait_SAPROVORE );
        guy.i_add( rotten_meat );
        CHECK( guy.consume_food() );
    }

    SECTION( "saprophage eats rotten food" ) {
        guy.set_mutation( trait_SAPROPHAGE );
        guy.i_add( rotten_meat );
        CHECK( guy.consume_food() );
    }
}

// npc_action enum values (npc_noop, npc_undecided, etc.) are defined in
// npcmove.cpp, not exposed in a header. Tests assert NPC state changes
// (worn items, position) rather than return codes.
TEST_CASE( "npc_warmth_response_in_address_needs", "[npc]" )
{
    clear_map_without_vision();
    calendar::turn = calendar::start_of_cataclysm;
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );

    SECTION( "freezing NPC with sweater wears it" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        behavior::character_oracle_t oracle( &guy );
        REQUIRE( oracle.needs_warmth_badly( "" ) == behavior::status_t::running );

        guy.i_add( item( itype_sweater ) );
        REQUIRE( !guy.is_wearing( itype_sweater ) );

        guy.address_needs( 0 );

        CHECK( guy.is_wearing( itype_sweater ) );
    }

    SECTION( "freezing NPC outdoors moves to adjacent indoor tile" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        behavior::character_oracle_t oracle( &guy );
        REQUIRE( oracle.needs_warmth_badly( "" ) == behavior::status_t::running );

        map &here = get_map();
        tripoint_bub_ms adj = guy.pos_bub() + point::east;
        here.ter_set( adj, ter_t_floor );

        guy.address_needs( 0 );

        CHECK( guy.pos_bub() == adj );
    }

    SECTION( "prefers wearing clothes over taking shelter" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );

        map &here = get_map();
        tripoint_bub_ms adj = guy.pos_bub() + point::east;
        here.ter_set( adj, ter_t_floor );

        guy.i_add( item( itype_sweater ) );
        const tripoint_bub_ms original_pos = guy.pos_bub();

        guy.address_needs( 0 );

        CHECK( guy.is_wearing( itype_sweater ) );
        CHECK( guy.pos_bub() == original_pos );
    }

    SECTION( "warm NPC ignores warmth response" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_NORM );
        behavior::character_oracle_t oracle( &guy );
        REQUIRE( oracle.needs_warmth_badly( "" ) != behavior::status_t::running );

        guy.i_add( item( itype_sweater ) );

        guy.address_needs( 0 );

        CHECK( !guy.is_wearing( itype_sweater ) );
    }

    SECTION( "shelter blocked by danger gate" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );

        map &here = get_map();
        tripoint_bub_ms adj = guy.pos_bub() + point::east;
        here.ter_set( adj, ter_t_floor );

        const tripoint_bub_ms original_pos = guy.pos_bub();

        guy.address_needs( NPC_DANGER_VERY_LOW + 1 );

        CHECK( guy.pos_bub() == original_pos );
    }

    SECTION( "no warmth items and no shelter falls through" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );

        const tripoint_bub_ms original_pos = guy.pos_bub();

        guy.address_needs( 0 );

        CHECK( guy.pos_bub() == original_pos );
        CHECK( !guy.is_wearing( itype_sweater ) );
    }

    SECTION( "fire supplies do not block shelter" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );

        map &here = get_map();
        tripoint_bub_ms adj = guy.pos_bub() + point::east;
        here.ter_set( adj, ter_t_floor );

        guy.i_add( tool_with_ammo( itype_lighter, 20 ) );
        guy.i_add( item( itype_2x4 ) );
        here.build_map_cache( 0 );
        behavior::character_oracle_t oracle( &guy );
        REQUIRE( oracle.can_make_fire( "" ) == behavior::status_t::running );

        guy.address_needs( 0 );

        CHECK( guy.pos_bub() == adj );
    }

    SECTION( "freezing NPC wears clothes during combat retreat" ) {
        // Wearing fires even when address_needs() is called from
        // move_away_from() with elevated danger. The NPC spends one
        // turn wearing the item instead of fleeing. Accepted trade-off:
        // hypothermia kills reliably, losing one retreat turn usually
        // does not.
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        guy.i_add( item( itype_sweater ) );

        guy.address_needs( NPC_DANGER_VERY_LOW + 1 );

        CHECK( guy.is_wearing( itype_sweater ) );
    }
}

TEST_CASE( "npc_bodytemp_updates_during_turn", "[npc][needs]" )
{
    clear_map_without_vision();
    weather_manager &weather = get_weather();
    weather.temperature = units::from_fahrenheit( 0 );
    weather.clear_temp_cache();

    SECTION( "throttle blocks bodytemp before 10-second boundary" ) {
        // once_every(10s) fires when (turn - turn_zero) % 10s == 0.
        // 3 seconds is non-aligned, so the throttle should block.
        calendar::turn = calendar::turn_zero + 3_seconds;
        npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( guy, true );
        guy.set_all_parts_temp_conv( BODYTEMP_NORM );
        guy.set_all_parts_temp_cur( BODYTEMP_NORM );

        guy.npc_update_body();

        for( const bodypart_id &bp : guy.get_all_body_parts() ) {
            CHECK( guy.get_part_temp_conv( bp ) == BODYTEMP_NORM );
        }
    }

    SECTION( "bodytemp decreases after 10-second boundary in freezing weather" ) {
        calendar::turn = calendar::turn_zero + 10_seconds;
        npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( guy, true );
        guy.set_all_parts_temp_conv( BODYTEMP_NORM );
        guy.set_all_parts_temp_cur( BODYTEMP_NORM );

        guy.npc_update_body();

        bool any_cooled = false;
        for( const bodypart_id &bp : guy.get_all_body_parts() ) {
            if( guy.get_part_temp_conv( bp ) < BODYTEMP_NORM ) {
                any_cooled = true;
                break;
            }
        }
        CHECK( any_cooled );
    }

    SECTION( "wetness updated via effect_wet" ) {
        calendar::turn = calendar::turn_zero + 10_seconds;
        npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( guy, true );
        const bodypart_id bp_torso( "torso" );
        guy.set_part_wetness( bp_torso, guy.get_part_drench_capacity( bp_torso ) );

        guy.npc_update_body();

        // update_body_wetness adds effect_wet deterministically when wetness > 0
        CHECK( guy.has_effect( effect_wet, bp_torso ) );
    }

    SECTION( "on_load catch-up recomputes bodytemp" ) {
        weather.temperature = units::from_fahrenheit( 0 );
        weather.clear_temp_cache();
        npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( guy, true );
        guy.set_all_parts_temp_conv( BODYTEMP_NORM );
        guy.set_all_parts_temp_cur( BODYTEMP_NORM );

        on_load_test( guy, 0_seconds, 30_minutes );

        bool any_cooled = false;
        for( const bodypart_id &bp : guy.get_all_body_parts() ) {
            if( guy.get_part_temp_conv( bp ) < BODYTEMP_NORM ) {
                any_cooled = true;
                break;
            }
        }
        CHECK( any_cooled );
    }
}

TEST_CASE( "npc_camp_water_through_stomach", "[npc][needs][camp]" )
{
    clear_avatar();
    clear_map_without_vision();
    get_player_character().camps.clear();
    map &m = get_map();
    // Use mid-map position so NPC and camp share the same OMT regardless
    // of where the map's absolute origin falls.
    const tripoint_bub_ms mid{ MAPSIZE_X / 2, MAPSIZE_Y / 2, 0 };
    const tripoint_abs_ms zone_loc = m.get_abs( mid );
    REQUIRE( m.inbounds( zone_loc ) );
    mapgen_place_zone( zone_loc, zone_loc, zone_type_CAMP_FOOD, your_fac, {}, "food" );
    mapgen_place_zone( zone_loc, zone_loc, zone_type_CAMP_STORAGE, your_fac, {}, "storage" );
    faction *camp_faction = get_player_character().get_faction();
    const tripoint_abs_omt this_omt = project_to<coords::omt>( zone_loc );
    m.add_camp( this_omt, "faction_camp" );
    std::optional<basecamp *> bcp = overmap_buffer.find_camp( this_omt.xy() );
    REQUIRE( !!bcp );
    basecamp *test_camp = *bcp;
    test_camp->define_camp( this_omt, "faction_base_bare_bones_NPC_camp_0", false );
    test_camp->set_owner( your_fac );
    REQUIRE( test_camp->has_water() );

    // Spawn NPC at mid-map so it's in the same OMT as the camp
    npc &guy = spawn_npc( mid.xy(), "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.stomach.empty();
    guy.guts.empty();
    guy.set_hunger( 0 );
    camp_faction->empty_food_supply();

    SECTION( "camp water enters stomach, NPC-facing thirst drops" ) {
        guy.set_thirst( 200 );
        REQUIRE( guy.get_thirst() > 40 );

        CHECK( guy.consume_food_from_camp() );
        CHECK( guy.stomach.get_water() > 0_ml );
        CHECK( guy.get_thirst() < 200 );
    }

    SECTION( "intake capped at stomach capacity" ) {
        // Pre-fill stomach near capacity, leaving only 50ml room
        const units::volume room = guy.stomach.stomach_remaining( guy );
        if( room > 50_ml ) {
            guy.stomach.ingest( { room - 50_ml, 0_ml, {} } );
        }
        guy.set_thirst( 800 );

        guy.consume_food_from_camp();

        CHECK( guy.stomach.contains() <= guy.stomach.capacity( guy ) );
    }

    SECTION( "full stomach does not waste turn" ) {
        // Fill stomach to capacity
        const units::volume room = guy.stomach.stomach_remaining( guy );
        guy.stomach.ingest( { room, 0_ml, {} } );
        // Raw thirst must exceed 40 + capacity/5 so NPC-facing thirst
        // still passes the > 40 gate despite stomach water subtraction.
        guy.set_thirst( 600 );
        REQUIRE( guy.get_thirst() > 40 );

        CHECK_FALSE( guy.consume_food_from_camp() );
    }

    SECTION( "calorie-deficit NPC eats camp food even when short-term hunger is zero" ) {
        // Stock the camp larder with 50 kcal of food.
        camp_faction->debug_food_supply().emplace_back( calendar::turn_zero, nutrients{} );
        camp_faction->debug_food_supply().back().second.calories = 50 * 1000;
        REQUIRE( camp_faction->food_supply().kcal() >= 50 );

        // NPC has severe calorie deficit but no short-term hunger.
        guy.set_stored_kcal( 1000 );
        guy.set_hunger( 0 );
        REQUIRE( guy.has_calorie_deficit() );
        REQUIRE( guy.get_hunger() <= 0 );

        CHECK( guy.consume_food_from_camp( npc::consume_filter::food_only ) );
        CHECK( camp_faction->food_supply().kcal() < 50 );
    }
}

TEST_CASE( "npc_nonally_sleeps_when_tired", "[npc][needs]" )
{
    clear_map_without_vision();
    // 30+ minutes past turn_zero so can_sleep()'s 30-minute cooldown
    // (anchored to last_sleep_check default of turn_zero) does not
    // block the first evaluation.
    calendar::turn = calendar::turn_zero + 1_hours;
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    REQUIRE_FALSE( guy.is_player_ally() );

    SECTION( "exhausted non-ally falls asleep when safe" ) {
        // Sleepiness 800 makes can_sleep() reliably pass despite bare-
        // ground comfort and rng(-8,8): sleepiness_factor ~26 dominates.
        guy.set_sleepiness( 800 );
        guy.set_mission( NPC_MISSION_SHELTER );
        guy.set_moves( 100 );

        guy.move();

        // NPCs call fall_asleep() directly on can_sleep() success.
        // Recovery only runs on the asleep branch in update_needs.
        CHECK( guy.has_effect( effect_sleep ) );
    }

    SECTION( "meth blocks non-ally sleep" ) {
        guy.set_sleepiness( 800 );
        guy.add_effect( effect_meth, 1_hours );
        guy.set_mission( NPC_MISSION_SHELTER );
        guy.set_moves( 100 );

        guy.move();

        // can_sleep() returns false on meth, NPC gets lying_down fallback
        CHECK_FALSE( guy.has_effect( effect_sleep ) );
        CHECK( guy.has_effect( effect_lying_down ) );
    }

    SECTION( "non-ally does not sleep in danger" ) {
        guy.set_sleepiness( 800 );
        // address_needs decides, execute_action performs. Together they
        // cover the decision gate and the sleep branch without needing
        // the full move() pipeline (which requires overmap/vision setup).
        // execute_action(npc_undecided) is safe -- just pauses.
        npc_action action = guy.address_needs( NPC_DANGER_VERY_LOW + 1 );
        guy.execute_action( action );

        CHECK_FALSE( guy.has_effect( effect_sleep ) );
        CHECK_FALSE( guy.has_effect( effect_lying_down ) );
    }

    SECTION( "below TIRED threshold does not trigger sleep" ) {
        guy.set_sleepiness( sleepiness_levels::TIRED / 2 );
        npc_action action = guy.address_needs( 0 );
        guy.execute_action( action );

        CHECK_FALSE( guy.has_effect( effect_sleep ) );
        CHECK_FALSE( guy.has_effect( effect_lying_down ) );
    }
}

// ---------------------------------------------------------------------------
// Helper-level tests for local resource acquisition
// ---------------------------------------------------------------------------

TEST_CASE( "npc_find_nearby_water_sources", "[npc][needs]" )
{
    clear_map_without_vision();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    set_time_to_day();
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;

    SECTION( "finds adjacent fresh shallow water" ) {
        here.ter_set( adj, ter_t_water_sh );
        std::vector<npc::scored_water_source> sources = guy.find_nearby_water_sources();
        REQUIRE( !sources.empty() );
        CHECK( sources[0].pos == adj );
    }

    SECTION( "ignores salt water" ) {
        here.ter_set( adj, ter_t_swater_sh );
        CHECK( guy.find_nearby_water_sources().empty() );
    }

    SECTION( "finds water within 6 tiles, returns exact position" ) {
        tripoint_bub_ms water_at = guy.pos_bub() + tripoint( 5, 0, 0 );
        here.ter_set( water_at, ter_t_water_sh );
        std::vector<npc::scored_water_source> sources = guy.find_nearby_water_sources();
        REQUIRE( !sources.empty() );
        CHECK( sources[0].pos == water_at );
    }

    SECTION( "ignores water beyond 6 tiles" ) {
        here.ter_set( guy.pos_bub() + tripoint( 7, 0, 0 ), ter_t_water_sh );
        CHECK( guy.find_nearby_water_sources().empty() );
    }

    SECTION( "sorted by distance, closest first" ) {
        tripoint_bub_ms close = guy.pos_bub() + tripoint::east;
        tripoint_bub_ms distant = guy.pos_bub() + tripoint( 4, 0, 0 );
        here.ter_set( close, ter_t_water_sh );
        here.ter_set( distant, ter_t_water_sh );
        std::vector<npc::scored_water_source> sources = guy.find_nearby_water_sources();
        REQUIRE( sources.size() >= 2 );
        CHECK( sources[0].pos == close );
        CHECK( sources[0].dist <= sources[1].dist );
    }

    SECTION( "ignores finite water sources" ) {
        here.ter_set( adj, ter_t_water_dispenser );
        CHECK( guy.find_nearby_water_sources().empty() );
    }

    SECTION( "ignores water behind walls (no LOS)" ) {
        // Place water behind a wall -- NPC can't see it
        tripoint_bub_ms wall_pos = guy.pos_bub() + tripoint::east;
        tripoint_bub_ms water_behind = guy.pos_bub() + tripoint( 2, 0, 0 );
        here.ter_set( wall_pos, ter_t_concrete_wall );
        here.ter_set( water_behind, ter_t_water_sh );
        here.build_map_cache( 0 );
        REQUIRE_FALSE( guy.sees( here, water_behind ) );
        CHECK( guy.find_nearby_water_sources().empty() );
    }
}

TEST_CASE( "npc_drink_from_water_source", "[npc][needs]" )
{
    clear_map_without_vision();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.stomach.empty();
    set_time_to_day();
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;
    here.ter_set( adj, ter_t_water_sh );
    std::vector<npc::scored_water_source> sources = guy.find_nearby_water_sources();
    REQUIRE( !sources.empty() );
    const tripoint_bub_ms water_pos = sources[0].pos;

    SECTION( "drinking fills stomach, lowers thirst, no movement" ) {
        guy.set_thirst( 200 );
        const int thirst_before = guy.get_thirst();
        const tripoint_bub_ms pos_before = guy.pos_bub();
        const units::volume water_before = guy.stomach.get_water();
        REQUIRE( guy.drink_from_water_source( water_pos ) );
        CHECK( guy.stomach.get_water() > water_before );
        CHECK( guy.get_thirst() < thirst_before );
        CHECK( guy.pos_bub() == pos_before );
    }

    SECTION( "full stomach returns false, no water added" ) {
        const units::volume room = guy.stomach.stomach_remaining( guy );
        guy.stomach.ingest( { room, 0_ml, {} } );
        const units::volume water_before = guy.stomach.get_water();
        guy.set_thirst( 600 );
        CHECK_FALSE( guy.drink_from_water_source( water_pos ) );
        CHECK( guy.stomach.get_water() == water_before );
    }

    SECTION( "zero thirst returns false" ) {
        guy.set_thirst( 0 );
        CHECK_FALSE( guy.drink_from_water_source( water_pos ) );
    }
}

TEST_CASE( "npc_find_nearby_food", "[npc][needs]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_weather().forced_temperature = 20_C;
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_hunger( 300 );
    guy.set_thirst( 100 );
    guy.set_stored_kcal( 5000 );
    set_time_to_day();
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;
    REQUIRE_FALSE( guy.is_player_ally() );

    SECTION( "finds food on adjacent tile, returns correct item" ) {
        here.add_item_or_charges( adj, item( itype_sandwich_cheese_grilled ) );
        std::vector<npc::scored_item> food = guy.find_nearby_food();
        REQUIRE( !food.empty() );
        CHECK( food[0].score > 0.0f );
        CHECK( food[0].loc.get_item()->typeId() == itype_sandwich_cheese_grilled );
        CHECK( food[0].loc.pos_bub( here ) == adj );
    }

    SECTION( "results sorted by descending score" ) {
        // Two different foods; verify the list is sorted descending by score.
        here.add_item_or_charges( adj, item( itype_honeycomb ) );
        tripoint_bub_ms adj2 = guy.pos_bub() + point::west;
        here.add_item_or_charges( adj2, item( itype_sandwich_cheese_grilled ) );
        std::vector<npc::scored_item> food = guy.find_nearby_food();
        REQUIRE( food.size() >= 2 );
        CHECK( food[0].score >= food[1].score );
    }

    SECTION( "does not find food beyond 6 tiles" ) {
        tripoint_bub_ms distant = guy.pos_bub() + tripoint( 7, 0, 0 );
        here.add_item_or_charges( distant, item( itype_sandwich_cheese_grilled ) );
        CHECK( guy.find_nearby_food().empty() );
    }

    SECTION( "ally without allow_pick_up still finds food for consumption" ) {
        guy.set_fac( faction_your_followers );
        guy.set_attitude( NPCATT_FOLLOW );
        REQUIRE( guy.is_player_ally() );
        guy.rules.clear_flag( ally_rule::allow_pick_up );
        here.add_item_or_charges( adj, item( itype_sandwich_cheese_grilled ) );
        CHECK_FALSE( guy.find_nearby_food().empty() );
    }

    SECTION( "ally skips food in NO_NPC_PICKUP zone" ) {
        guy.set_fac( faction_your_followers );
        guy.set_attitude( NPCATT_FOLLOW );
        REQUIRE( guy.is_player_ally() );
        const tripoint_abs_ms abs_adj = here.get_abs( adj );
        mapgen_place_zone( abs_adj, abs_adj, zone_type_NO_NPC_PICKUP,
                           faction_your_followers, {}, "no_pickup" );
        here.add_item_or_charges( adj, item( itype_sandwich_cheese_grilled ) );
        CHECK( guy.find_nearby_food().empty() );
    }

    SECTION( "dry food found but scored lower when thirsty" ) {
        // Thirst-dominant NPCs still find dry food (no hard filter).
        // rate_food penalizes thirst-inducing items when thirsty,
        // so hydrating food naturally ranks higher.
        guy.set_thirst( 400 );
        guy.set_hunger( 50 );
        item dry_food( itype_meat_cooked );
        REQUIRE( dry_food.get_comestible() );
        REQUIRE( dry_food.get_comestible()->quench <= 0 );
        here.add_item_or_charges( adj, dry_food );
        CHECK_FALSE( guy.find_nearby_food().empty() );
    }
}

TEST_CASE( "npc_find_nearby_warm_clothing", "[npc][needs]" )
{
    clear_map_without_vision();
    set_time_to_day();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );
    map &here = get_map();
    here.build_map_cache( 0 );
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;
    REQUIRE_FALSE( guy.is_player_ally() );

    SECTION( "finds sweater on adjacent tile, correct item and warmth score" ) {
        here.add_item_or_charges( adj, item( itype_sweater ) );
        std::vector<npc::scored_item> warm = guy.find_nearby_warm_clothing();
        REQUIRE( !warm.empty() );
        CHECK( warm[0].loc.get_item()->typeId() == itype_sweater );
        CHECK( warm[0].loc.pos_bub( here ) == adj );
        CHECK( warm[0].score == static_cast<float>( item( itype_sweater ).get_warmth() ) );
    }

    SECTION( "results sorted by descending warmth" ) {
        here.add_item_or_charges( adj, item( itype_apron_leather ) );
        tripoint_bub_ms adj2 = guy.pos_bub() + point::west;
        here.add_item_or_charges( adj2, item( itype_sweater ) );
        std::vector<npc::scored_item> warm = guy.find_nearby_warm_clothing();
        REQUIRE( warm.size() >= 2 );
        CHECK( warm[0].score >= warm[1].score );
        CHECK( warm[0].loc.get_item()->typeId() == itype_sweater );
    }

    SECTION( "ally without allow_pick_up still finds clothing for wearing" ) {
        guy.set_fac( faction_your_followers );
        guy.set_attitude( NPCATT_FOLLOW );
        REQUIRE( guy.is_player_ally() );
        guy.rules.clear_flag( ally_rule::allow_pick_up );
        here.add_item_or_charges( adj, item( itype_sweater ) );
        CHECK_FALSE( guy.find_nearby_warm_clothing().empty() );
    }
}

TEST_CASE( "npc_move_to_and_verify", "[npc][needs]" )
{
    clear_map_without_vision();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    map &here = get_map();

    SECTION( "moves toward passable target, distance decreases" ) {
        tripoint_bub_ms target = guy.pos_bub() + tripoint( 3, 0, 0 );
        REQUIRE( here.passable( target ) );
        const int dist_before = rl_dist( guy.pos_bub(), target );
        REQUIRE( guy.move_to_and_verify( target ) );
        CHECK( rl_dist( guy.pos_bub(), target ) < dist_before );
    }

    SECTION( "impassable target: nearest_passable finds adjacent passable tile" ) {
        tripoint_bub_ms target = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( target, ter_t_wall_glass );
        REQUIRE( here.impassable( target ) );
        REQUIRE( here.passable( target + tripoint::west ) );
        REQUIRE( guy.move_to_and_verify( target ) );
        CHECK( guy.pos_bub() != target );
        CHECK( here.passable( guy.pos_bub() ) );
        CHECK( rl_dist( guy.pos_bub(), target ) < 3 );
    }

    SECTION( "unreachable target returns false, NPC stays" ) {
        tripoint_bub_ms target = guy.pos_bub() + tripoint( 3, 0, 0 );
        for( const tripoint_bub_ms &wall : here.points_in_radius( target, 1 ) ) {
            if( wall != target ) {
                here.ter_set( wall, ter_t_wall_glass );
            }
        }
        REQUIRE( here.impassable( target + tripoint::west ) );
        const tripoint_bub_ms before = guy.pos_bub();
        CHECK_FALSE( guy.move_to_and_verify( target ) );
        CHECK( guy.pos_bub() == before );
    }
}

TEST_CASE( "npc_ownership_blocks_ground_food", "[npc][needs]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_weather().forced_temperature = 20_C;
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_hunger( 300 );
    guy.set_thirst( 100 );
    guy.set_stored_kcal( 5000 );
    set_time_to_day();
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;
    REQUIRE_FALSE( guy.is_player_ally() );
    // Place player close enough to see NPC and adj tile
    get_player_character().setpos( here, guy.pos_bub() + tripoint( 0, -2, 0 ) );
    REQUIRE( get_player_view().sees( here, guy.pos_bub( here ) ) );
    REQUIRE( get_player_view().sees( here, adj ) );

    SECTION( "high-trust NPC skips player-owned food (caught stealing)" ) {
        // stealing_threshold = 10 + 100/5 - 5 - 0 = 25 > 0
        // would_always_steal = false; player sees NPC -> won't steal
        guy.get_faction()->trusts_u = 100;
        guy.personality.aggression = 5;
        guy.personality.collector = 0;
        item owned_food( itype_sandwich_cheese_grilled );
        owned_food.set_owner( get_player_character() );
        here.add_item_or_charges( adj, owned_food );
        CHECK( guy.find_nearby_food().empty() );
    }

    SECTION( "aggressive NPC takes player-owned food (always steals)" ) {
        // stealing_threshold = 10 + 0/5 - 20 - 0 = -10 < 0
        // would_always_steal = true, ignores visibility
        guy.get_faction()->trusts_u = 0;
        guy.personality.aggression = 20;
        guy.personality.collector = 0;
        item owned_food( itype_sandwich_cheese_grilled );
        owned_food.set_owner( get_player_character() );
        here.add_item_or_charges( adj, owned_food );
        std::vector<npc::scored_item> food = guy.find_nearby_food();
        CHECK_FALSE( food.empty() );
    }
}

TEST_CASE( "npc_consume_food_at_helper", "[npc][needs]" )
{
    clear_map_without_vision();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_hunger( 300 );
    guy.set_stored_kcal( 5000 );
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;

    SECTION( "consumes ground food, item removed from map" ) {
        item &spawned = here.add_item_or_charges( adj, item( itype_sandwich_cheese_grilled ) );
        const size_t count_before = here.i_at( adj ).size();
        item_location loc( map_cursor( adj ), &spawned );
        REQUIRE( guy.consume_food_at( loc ) );
        CHECK( here.i_at( adj ).size() < count_before );
    }

    SECTION( "null item_location returns false" ) {
        item_location empty;
        CHECK_FALSE( guy.consume_food_at( empty ) );
    }
}

TEST_CASE( "npc_wear_item_at_helper", "[npc][needs]" )
{
    clear_map_without_vision();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;

    SECTION( "wears ground sweater, item removed from map" ) {
        item &spawned = here.add_item_or_charges( adj, item( itype_sweater ) );
        const size_t count_before = here.i_at( adj ).size();
        item_location loc( map_cursor( adj ), &spawned );
        REQUIRE( guy.wear_item_at( loc ) );
        CHECK( guy.is_wearing( itype_sweater ) );
        CHECK( here.i_at( adj ).size() < count_before );
    }

    SECTION( "null item_location returns false" ) {
        item_location empty;
        CHECK_FALSE( guy.wear_item_at( empty ) );
    }
}

// ---------------------------------------------------------------------------
// Integration tests: address_needs with ground resources
// ---------------------------------------------------------------------------

TEST_CASE( "npc_address_needs_ground_water", "[npc][needs]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    set_time_to_day();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.stomach.empty();
    guy.guts.empty();
    REQUIRE_FALSE( guy.is_player_ally() );
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;

    SECTION( "extreme thirst: adjacent water consumed before danger gate" ) {
        guy.set_thirst( 200 );
        REQUIRE( guy.get_thirst() > 80 );
        here.ter_set( adj, ter_t_water_sh );
        const tripoint_bub_ms before = guy.pos_bub();

        guy.address_needs( NPC_DANGER_VERY_LOW + 1 );

        CHECK( guy.stomach.get_water() > 0_ml );
        CHECK( guy.pos_bub() == before );
    }

    SECTION( "non-adjacent water: blocked by danger gate" ) {
        guy.set_thirst( 200 );
        tripoint_bub_ms water_at = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( water_at, ter_t_water_sh );
        REQUIRE( square_dist( guy.pos_bub(), water_at ) > 1 );
        const tripoint_bub_ms before = guy.pos_bub();

        guy.address_needs( NPC_DANGER_VERY_LOW + 1 );

        CHECK( guy.stomach.get_water() == 0_ml );
        CHECK( guy.pos_bub() == before );
    }

    SECTION( "non-adjacent water: NPC paths toward it (extreme thirst, low danger)" ) {
        guy.set_thirst( 200 );
        REQUIRE( guy.get_thirst() > 80 );
        tripoint_bub_ms water_at = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( water_at, ter_t_water_sh );
        const int dist_before = rl_dist( guy.pos_bub(), water_at );

        guy.address_needs( 0 );

        CHECK( rl_dist( guy.pos_bub(), water_at ) < dist_before );
    }

    SECTION( "blocked nearest water skipped, reachable farther water reached" ) {
        guy.set_thirst( 200 );
        REQUIRE( guy.get_thirst() > 80 );
        // Nearest water behind glass walls (unreachable)
        tripoint_bub_ms blocked_at = guy.pos_bub() + tripoint( 2, 0, 0 );
        for( const tripoint_bub_ms &w : here.points_in_radius( blocked_at, 1 ) ) {
            if( w != blocked_at ) {
                here.ter_set( w, ter_t_wall_glass );
            }
        }
        here.ter_set( blocked_at, ter_t_water_sh );
        // Farther water on open ground (reachable)
        tripoint_bub_ms open_at = guy.pos_bub() + tripoint( 0, 3, 0 );
        here.ter_set( open_at, ter_t_water_sh );
        REQUIRE( here.passable( open_at ) );
        const int dist_to_open = rl_dist( guy.pos_bub(), open_at );

        guy.address_needs( 0 );

        CHECK( rl_dist( guy.pos_bub(), open_at ) < dist_to_open );
    }
}

TEST_CASE( "npc_address_needs_ground_food", "[npc][needs]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.stomach.empty();
    guy.guts.empty();
    REQUIRE_FALSE( guy.is_player_ally() );
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;

    SECTION( "extreme hunger: adjacent food consumed before danger gate" ) {
        guy.set_stored_kcal( 2000 );
        guy.set_hunger( 300 );
        guy.set_thirst( 100 );
        here.add_item_or_charges( adj, item( itype_sandwich_cheese_grilled ) );
        const size_t items_before = here.i_at( adj ).size();
        const tripoint_bub_ms before = guy.pos_bub();

        guy.address_needs( NPC_DANGER_VERY_LOW + 1 );

        CHECK( here.i_at( adj ).size() < items_before );
        CHECK( guy.pos_bub() == before );
    }

    SECTION( "non-adjacent food: blocked by danger gate" ) {
        guy.set_stored_kcal( 2000 );
        guy.set_hunger( 300 );
        guy.set_thirst( 100 );
        tripoint_bub_ms food_at = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.add_item_or_charges( food_at, item( itype_sandwich_cheese_grilled ) );
        const tripoint_bub_ms before = guy.pos_bub();

        guy.address_needs( NPC_DANGER_VERY_LOW + 1 );

        CHECK( guy.pos_bub() == before );
    }

    SECTION( "non-adjacent food: NPC paths at low danger (extreme hunger)" ) {
        guy.set_stored_kcal( 2000 );
        guy.set_hunger( 300 );
        guy.set_thirst( 100 );
        REQUIRE( guy.get_stored_kcal() + guy.stomach.get_calories() <
                 guy.get_healthy_kcal() * 0.75 );
        tripoint_bub_ms food_at = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.add_item_or_charges( food_at, item( itype_sandwich_cheese_grilled ) );
        const int dist_before = rl_dist( guy.pos_bub(), food_at );

        guy.address_needs( 0 );

        CHECK( rl_dist( guy.pos_bub(), food_at ) < dist_before );
    }

    SECTION( "unreachable food: NPC does not stall" ) {
        guy.set_stored_kcal( 2000 );
        guy.set_hunger( 300 );
        guy.set_thirst( 100 );
        tripoint_bub_ms food_at = guy.pos_bub() + tripoint( 3, 0, 0 );
        for( const tripoint_bub_ms &w : here.points_in_radius( food_at, 1 ) ) {
            if( w != food_at ) {
                here.ter_set( w, ter_t_wall_glass );
            }
        }
        here.add_item_or_charges( food_at, item( itype_sandwich_cheese_grilled ) );
        REQUIRE( here.sees_some_items( food_at, guy ) );
        REQUIRE( guy.sees( here, food_at ) );
        const tripoint_bub_ms before = guy.pos_bub();

        guy.address_needs( 0 );

        CHECK( guy.pos_bub() == before );
    }

    SECTION( "blocked best candidate skipped, reachable second candidate reached" ) {
        guy.set_stored_kcal( 2000 );
        guy.set_hunger( 300 );
        guy.set_thirst( 100 );
        // High-value food behind glass (unreachable)
        tripoint_bub_ms blocked_at = guy.pos_bub() + tripoint( 3, 0, 0 );
        for( const tripoint_bub_ms &w : here.points_in_radius( blocked_at, 1 ) ) {
            if( w != blocked_at ) {
                here.ter_set( w, ter_t_wall_glass );
            }
        }
        here.add_item_or_charges( blocked_at, item( itype_sandwich_cheese_grilled ) );
        REQUIRE( here.sees_some_items( blocked_at, guy ) );
        REQUIRE( guy.sees( here, blocked_at ) );
        // Low-value food on open ground (reachable)
        tripoint_bub_ms open_at = guy.pos_bub() + tripoint( 0, 3, 0 );
        REQUIRE( here.passable( open_at ) );
        here.add_item_or_charges( open_at, item( itype_honeycomb ) );
        const int dist_to_open = rl_dist( guy.pos_bub(), open_at );

        guy.address_needs( 0 );

        CHECK( rl_dist( guy.pos_bub(), open_at ) < dist_to_open );
    }
}

TEST_CASE( "npc_address_needs_ground_clothing", "[npc][needs]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    set_time_to_day();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.stomach.empty();
    guy.guts.empty();
    REQUIRE_FALSE( guy.is_player_ally() );
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;

    SECTION( "freezing NPC wears adjacent sweater from ground" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        guy.worn.wear_item( guy, item( itype_backpack ), false, false );
        here.add_item_or_charges( adj, item( itype_sweater ) );
        const tripoint_bub_ms before = guy.pos_bub();

        guy.address_needs( 0 );

        CHECK( guy.is_wearing( itype_sweater ) );
        CHECK( guy.pos_bub() == before );
    }

    SECTION( "adjacent sweater worn even during combat, no movement" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        guy.worn.wear_item( guy, item( itype_backpack ), false, false );
        here.add_item_or_charges( adj, item( itype_sweater ) );
        const tripoint_bub_ms before = guy.pos_bub();

        guy.address_needs( NPC_DANGER_VERY_LOW + 1 );

        CHECK( guy.is_wearing( itype_sweater ) );
        CHECK( guy.pos_bub() == before );
    }

    SECTION( "non-adjacent sweater: blocked by danger gate" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        tripoint_bub_ms sweater_at = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.add_item_or_charges( sweater_at, item( itype_sweater ) );
        const tripoint_bub_ms before = guy.pos_bub();

        guy.address_needs( NPC_DANGER_VERY_LOW + 1 );

        CHECK_FALSE( guy.is_wearing( itype_sweater ) );
        CHECK( guy.pos_bub() == before );
    }

    SECTION( "non-adjacent sweater: NPC paths at low danger, distance decreases" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        tripoint_bub_ms sweater_at = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.add_item_or_charges( sweater_at, item( itype_sweater ) );
        const int dist_before = rl_dist( guy.pos_bub(), sweater_at );
        REQUIRE( dist_before > 1 );

        guy.address_needs( 0 );

        CHECK( rl_dist( guy.pos_bub(), sweater_at ) < dist_before );
        CHECK_FALSE( guy.is_wearing( itype_sweater ) );
    }

    SECTION( "prefers inventory over ground" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        guy.worn.wear_item( guy, item( itype_backpack ), false, false );
        guy.i_add( item( itype_sweater ) );
        here.add_item_or_charges( adj, item( itype_sweater ) );
        const size_t ground_before = here.i_at( adj ).size();

        guy.address_needs( 0 );

        CHECK( guy.is_wearing( itype_sweater ) );
        CHECK( here.i_at( adj ).size() == ground_before );
    }
}

// === Stabilization fixes: thirst-dominant filter, quench floor, shelter, cargo, foraging ===

TEST_CASE( "npc_thirst_dominant_filter_fallback", "[npc][needs]" )
{
    clear_map_without_vision();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;

    SECTION( "dry food found when only dry food available and thirst dominant" ) {
        guy.set_thirst( 400 );
        guy.set_hunger( 50 );
        guy.set_stored_kcal( 5000 );
        item dry( itype_crackers );
        REQUIRE( dry.get_comestible() );
        REQUIRE( dry.get_comestible()->quench <= 0 );
        here.add_item_or_charges( adj, dry );

        auto food = guy.find_nearby_food();

        CHECK_FALSE( food.empty() );
        if( !food.empty() ) {
            CHECK( food[0].loc.get_item()->typeId() == itype_crackers );
        }
    }

    SECTION( "hydrating food ranked above dry when both available" ) {
        guy.set_thirst( 400 );
        guy.set_hunger( 50 );
        guy.set_stored_kcal( 5000 );
        here.add_item_or_charges( adj, item( itype_sandwich_cheese_grilled ) );
        const tripoint_bub_ms west = guy.pos_bub() + point::west;
        here.add_item_or_charges( west, item( itype_crackers ) );

        auto food = guy.find_nearby_food();

        REQUIRE( food.size() >= 2 );
        CHECK( food[0].loc.get_item()->typeId() == itype_sandwich_cheese_grilled );
        // Dry food present as lower-priority fallback
        bool has_dry = std::any_of( food.begin(), food.end(),
        []( const npc::scored_item & si ) {
            return si.loc.get_item()->typeId() == itype_crackers;
        } );
        CHECK( has_dry );
    }
}

TEST_CASE( "npc_rate_food_quench_surplus_floor", "[npc][needs]" )
{
    clear_map_without_vision();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;

    SECTION( "calorie-positive hydrating food not killed by quench penalty" ) {
        // NPC is not thirsty. Quench surplus penalty would normally make
        // weight negative for fresh hydrating food (penalty = (1-rot)*quench).
        // The floor keeps calorie-positive food available as a last resort.
        guy.set_thirst( 0 );
        guy.set_hunger( 300 );
        guy.set_stored_kcal( 5000 );
        item fruit( itype_orange );
        REQUIRE( fruit.get_comestible() );
        REQUIRE( fruit.get_comestible()->get_default_nutr() > 0 );
        REQUIRE( fruit.get_comestible()->quench > 0 );
        here.add_item_or_charges( adj, fruit );

        auto food = guy.find_nearby_food();

        REQUIRE_FALSE( food.empty() );
        CHECK( food[0].loc.get_item()->typeId() == itype_orange );
    }

    SECTION( "zero-calorie drink still rejected by quench surplus" ) {
        // The floor only applies to nutr > 0. A zero-calorie drink like
        // clean water should still be filtered out when NPC isn't thirsty.
        guy.set_thirst( 0 );
        guy.set_hunger( 300 );
        guy.set_stored_kcal( 5000 );
        item water( itype_water_clean );
        REQUIRE( water.get_comestible() );
        REQUIRE( water.get_comestible()->get_default_nutr() <= 0 );
        REQUIRE( water.get_comestible()->quench > 0 );
        here.add_item_or_charges( adj, water );

        auto food = guy.find_nearby_food();

        CHECK( food.empty() );
    }
}

TEST_CASE( "npc_find_nearby_shelters", "[npc][needs]" )
{
    clear_map_without_vision();
    set_time_to_day();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;

    SECTION( "finds adjacent indoor tile" ) {
        here.ter_set( adj, ter_t_floor );
        here.build_map_cache( 0 );
        auto shelters = guy.find_nearby_shelters();
        REQUIRE_FALSE( shelters.empty() );
        CHECK( shelters[0].pos == adj );
    }

    SECTION( "finds indoor tile at distance 4" ) {
        const tripoint_bub_ms distant = guy.pos_bub() + tripoint( 4, 0, 0 );
        here.ter_set( distant, ter_t_floor );
        here.build_map_cache( 0 );
        auto shelters = guy.find_nearby_shelters();
        REQUIRE_FALSE( shelters.empty() );
        CHECK( shelters[0].pos == distant );
    }

    SECTION( "ignores indoor tile beyond 6" ) {
        const tripoint_bub_ms too_far = guy.pos_bub() + tripoint( 7, 0, 0 );
        here.ter_set( too_far, ter_t_floor );
        here.build_map_cache( 0 );
        CHECK( guy.find_nearby_shelters().empty() );
    }

    SECTION( "returns closest first when multiple" ) {
        const tripoint_bub_ms close = guy.pos_bub() + point::east;
        const tripoint_bub_ms distant = guy.pos_bub() + tripoint( 4, 0, 0 );
        here.ter_set( distant, ter_t_floor );
        here.ter_set( close, ter_t_floor );
        here.build_map_cache( 0 );
        auto shelters = guy.find_nearby_shelters();
        REQUIRE( shelters.size() >= 2 );
        CHECK( shelters[0].dist <= shelters[1].dist );
    }

    SECTION( "ignores impassable indoor tile (pony wall)" ) {
        here.ter_set( adj, ter_t_ponywall );
        here.build_map_cache( 0 );
        REQUIRE( here.has_flag( ter_furn_flag::TFLAG_INDOORS, adj ) );
        REQUIRE( here.impassable( adj ) );
        CHECK( guy.find_nearby_shelters().empty() );
    }

    SECTION( "already indoors returns empty" ) {
        here.ter_set( guy.pos_bub(), ter_t_floor );
        here.build_map_cache( 0 );
        CHECK( guy.find_nearby_shelters().empty() );
    }

    SECTION( "ignores indoor tile occupied by creature" ) {
        here.ter_set( adj, ter_t_floor );
        here.build_map_cache( 0 );
        spawn_test_monster( "mon_zombie", adj );
        CHECK( guy.find_nearby_shelters().empty() );
    }

    SECTION( "no LOS: indoor tile behind opaque wall not found" ) {
        here.ter_set( adj, ter_t_wall );
        const tripoint_bub_ms behind = adj + point::east;
        here.ter_set( behind, ter_t_floor );
        here.build_map_cache( 0 );
        CHECK( guy.find_nearby_shelters().empty() );
    }
}

TEST_CASE( "npc_shelter_pathfinding", "[npc][needs]" )
{
    clear_map_without_vision();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );
    map &here = get_map();

    SECTION( "NPC paths toward indoor tile 4 away at low danger" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        const tripoint_bub_ms shelter = guy.pos_bub() + tripoint( 4, 0, 0 );
        here.ter_set( shelter, ter_t_floor );
        here.build_map_cache( 0 );
        const int dist_before = rl_dist( guy.pos_bub(), shelter );

        guy.address_needs( 0 );

        CHECK( rl_dist( guy.pos_bub(), shelter ) < dist_before );
    }

    SECTION( "shelter at 4 blocked by danger gate" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        const tripoint_bub_ms shelter = guy.pos_bub() + tripoint( 4, 0, 0 );
        here.ter_set( shelter, ter_t_floor );
        here.build_map_cache( 0 );
        const tripoint_bub_ms before = guy.pos_bub();

        guy.address_needs( NPC_DANGER_VERY_LOW + 1 );

        CHECK( guy.pos_bub() == before );
    }

    SECTION( "unreachable shelter: NPC stays put" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        // Shelter inside 2-tile-thick glass enclosure. Single-tile walls
        // have diagonal gaps that A* can exploit; double-thick seals them.
        const tripoint_bub_ms shelter = guy.pos_bub() + tripoint( 4, 0, 0 );
        here.ter_set( shelter, ter_t_floor );
        for( int r = 1; r <= 2; ++r ) {
            for( const tripoint_bub_ms &w : here.points_in_radius( shelter, r ) ) {
                if( w != shelter && !here.has_flag( ter_furn_flag::TFLAG_INDOORS, w ) ) {
                    here.ter_set( w, ter_t_wall_glass );
                }
            }
        }
        here.build_map_cache( 0 );
        REQUIRE( guy.sees( here, shelter ) );
        const tripoint_bub_ms before = guy.pos_bub();

        CHECK_FALSE( guy.take_shelter_nearby() );
        CHECK( guy.pos_bub() == before );
    }

    SECTION( "unreachable nearest shelter, falls through to reachable second" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        // Nearest shelter in double-thick glass enclosure (unreachable).
        const tripoint_bub_ms close = guy.pos_bub() + tripoint( 4, 0, 0 );
        here.ter_set( close, ter_t_floor );
        for( int r = 1; r <= 2; ++r ) {
            for( const tripoint_bub_ms &w : here.points_in_radius( close, r ) ) {
                if( w != close && !here.has_flag( ter_furn_flag::TFLAG_INDOORS, w ) ) {
                    here.ter_set( w, ter_t_wall_glass );
                }
            }
        }
        // Second shelter reachable (open), opposite direction.
        const tripoint_bub_ms distant = guy.pos_bub() + tripoint( -3, 0, 0 );
        here.ter_set( distant, ter_t_floor );
        here.build_map_cache( 0 );
        const int dist_to_far = rl_dist( guy.pos_bub(), distant );

        CHECK( guy.take_shelter_nearby() );
        CHECK( rl_dist( guy.pos_bub(), distant ) < dist_to_far );
    }

    SECTION( "occupied nearest shelter skipped, next candidate used" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        const tripoint_bub_ms close = guy.pos_bub() + point::east;
        here.ter_set( close, ter_t_floor );
        spawn_test_monster( "mon_zombie", close );
        const tripoint_bub_ms distant = guy.pos_bub() + tripoint( 0, 3, 0 );
        here.ter_set( distant, ter_t_floor );
        here.build_map_cache( 0 );
        const int dist_to_far = rl_dist( guy.pos_bub(), distant );

        guy.address_needs( 0 );

        CHECK( rl_dist( guy.pos_bub(), distant ) < dist_to_far );
    }
}

TEST_CASE( "npc_find_food_vehicle_cargo", "[npc][needs]" )
{
    clear_map_without_vision();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;
    guy.set_hunger( 300 );
    guy.set_stored_kcal( 5000 );

    SECTION( "finds food in cargo-only tile (no ground items)" ) {
        vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                          adj, 0_degrees, 0, 0 );
        REQUIRE( cart != nullptr );
        const std::optional<vpart_reference> cargo = here.veh_at( adj ).cargo();
        REQUIRE( cargo.has_value() );
        cargo->vehicle().add_item( here, cargo->part(), item( itype_sandwich_cheese_grilled ) );
        REQUIRE( here.i_at( adj ).empty() );

        auto food = guy.find_nearby_food();

        REQUIRE_FALSE( food.empty() );
        CHECK( food[0].loc.get_item()->typeId() == itype_sandwich_cheese_grilled );
    }

    SECTION( "adjacent cargo food consumed via address_needs (end-to-end)" ) {
        guy.set_stored_kcal( 2000 );
        vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                          adj, 0_degrees, 0, 0 );
        REQUIRE( cart != nullptr );
        const std::optional<vpart_reference> cargo = here.veh_at( adj ).cargo();
        REQUIRE( cargo.has_value() );
        cargo->vehicle().add_item( here, cargo->part(), item( itype_sandwich_cheese_grilled ) );
        REQUIRE( here.i_at( adj ).empty() );
        const size_t cargo_before = cargo->items().size();
        const tripoint_bub_ms before = guy.pos_bub();

        guy.address_needs( 0 );

        CHECK( cargo->items().size() < cargo_before );
        CHECK( guy.pos_bub() == before );
    }

    SECTION( "cargo-locking blocks food discovery" ) {
        // Build from empty prototype: frame + trunk_floor (LOCKABLE_CARGO) + cargo_lock.
        vehicle *veh = here.add_vehicle( vehicle_prototype_none, adj, 0_degrees, 0, 0 );
        REQUIRE( veh != nullptr );
        const point_rel_ms origin( 0, 0 );
        veh->install_part( here, origin, vpart_frame );
        const int trunk_idx = veh->install_part( here, origin, vpart_trunk_floor );
        REQUIRE( trunk_idx >= 0 );
        const int lock_idx = veh->install_part( here, origin, vpart_cargo_lock );
        REQUIRE( lock_idx >= 0 );
        // Refresh vehicle caches after installing parts on empty prototype.
        here.rebuild_vehicle_level_caches();
        const tripoint_bub_ms cargo_pos = veh->bub_part_pos( here, trunk_idx );
        const std::optional<vpart_reference> cargo = here.veh_at( cargo_pos ).cargo();
        REQUIRE( cargo.has_value() );
        REQUIRE( here.veh_at( cargo_pos ).part_with_feature( "CARGO_LOCKING", true ).has_value() );
        cargo->vehicle().add_item( here, cargo->part(), item( itype_sandwich_cheese_grilled ) );

        CHECK( guy.find_nearby_food().empty() );
    }
}

TEST_CASE( "npc_find_clothing_vehicle_cargo", "[npc][needs]" )
{
    clear_map_without_vision();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;

    SECTION( "finds warm clothing in cargo-only tile" ) {
        vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                          adj, 0_degrees, 0, 0 );
        REQUIRE( cart != nullptr );
        const std::optional<vpart_reference> cargo = here.veh_at( adj ).cargo();
        REQUIRE( cargo.has_value() );
        cargo->vehicle().add_item( here, cargo->part(), item( itype_sweater ) );
        REQUIRE( here.i_at( adj ).empty() );

        auto warm = guy.find_nearby_warm_clothing();

        REQUIRE_FALSE( warm.empty() );
        CHECK( warm[0].loc.get_item()->typeId() == itype_sweater );
    }

    SECTION( "adjacent cargo sweater worn via address_needs (end-to-end)" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                          adj, 0_degrees, 0, 0 );
        REQUIRE( cart != nullptr );
        const std::optional<vpart_reference> cargo = here.veh_at( adj ).cargo();
        REQUIRE( cargo.has_value() );
        cargo->vehicle().add_item( here, cargo->part(), item( itype_sweater ) );

        guy.address_needs( 0 );

        CHECK( guy.is_wearing( itype_sweater ) );
    }

    SECTION( "cargo-locking blocks clothing discovery" ) {
        vehicle *veh = here.add_vehicle( vehicle_prototype_none, adj, 0_degrees, 0, 0 );
        REQUIRE( veh != nullptr );
        const point_rel_ms origin( 0, 0 );
        veh->install_part( here, origin, vpart_frame );
        const int trunk_idx = veh->install_part( here, origin, vpart_trunk_floor );
        REQUIRE( trunk_idx >= 0 );
        veh->install_part( here, origin, vpart_cargo_lock );
        here.rebuild_vehicle_level_caches();
        const tripoint_bub_ms cargo_pos = veh->bub_part_pos( here, trunk_idx );
        const std::optional<vpart_reference> cargo = here.veh_at( cargo_pos ).cargo();
        REQUIRE( cargo.has_value() );
        REQUIRE( here.veh_at( cargo_pos ).part_with_feature( "CARGO_LOCKING", true ).has_value() );
        cargo->vehicle().add_item( here, cargo->part(), item( itype_sweater ) );

        CHECK( guy.find_nearby_warm_clothing().empty() );
    }
}

TEST_CASE( "npc_find_nearby_harvestable", "[npc][needs]" )
{
    clear_map_without_vision();
    set_time_to_day();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;

    SECTION( "finds adjacent forageable underbrush" ) {
        here.ter_set( adj, ter_t_underbrush );
        here.build_map_cache( 0 );
        // Underbrush uses examine_action, not the harvest system.
        // find_nearby_harvestable detects both.
        auto h = guy.find_nearby_harvestable();
        REQUIRE_FALSE( h.empty() );
        CHECK( h[0].pos == adj );
    }

    SECTION( "finds forageable terrain at distance 4" ) {
        const tripoint_bub_ms distant = guy.pos_bub() + tripoint( 4, 0, 0 );
        here.ter_set( distant, ter_t_underbrush );
        here.build_map_cache( 0 );
        auto h = guy.find_nearby_harvestable();
        REQUIRE_FALSE( h.empty() );
        CHECK( h[0].pos == distant );
    }

    SECTION( "ignores non-harvestable terrain" ) {
        here.build_map_cache( 0 );
        CHECK( guy.find_nearby_harvestable().empty() );
    }

    SECTION( "ignores forageable terrain beyond 6" ) {
        here.ter_set( guy.pos_bub() + tripoint( 7, 0, 0 ), ter_t_underbrush );
        here.build_map_cache( 0 );
        CHECK( guy.find_nearby_harvestable().empty() );
    }

    SECTION( "returns closest first" ) {
        here.ter_set( guy.pos_bub() + tripoint( 4, 0, 0 ), ter_t_underbrush );
        here.ter_set( adj, ter_t_underbrush );
        here.build_map_cache( 0 );
        auto h = guy.find_nearby_harvestable();
        REQUIRE( h.size() >= 2 );
        CHECK( h[0].dist <= h[1].dist );
    }

    SECTION( "requires LOS" ) {
        here.ter_set( adj, ter_t_wall );
        here.ter_set( adj + point::east, ter_t_underbrush );
        here.build_map_cache( 0 );
        CHECK( guy.find_nearby_harvestable().empty() );
    }
}

// Harvest scavenging is a last-resort fallback, not reliable food production.
// is_harvestable() matches any non-empty harvest (underbrush, berry bushes,
// fruit trees, but also trees that yield sticks/leaves). Actual food output
// is seasonal RNG. Tests verify attempt-and-plumbing, not hunger relief.
TEST_CASE( "npc_harvest_scavenging", "[npc][needs]" )
{
    clear_map_without_vision();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;

    SECTION( "starving NPC adjacent to underbrush starts forage activity" ) {
        guy.set_stored_kcal( 2000 );
        guy.set_thirst( 0 );
        here.ter_set( adj, ter_t_underbrush );
        here.build_map_cache( 0 );
        REQUIRE_FALSE( guy.find_nearby_harvestable().empty() );
        REQUIRE( guy.find_nearby_food().empty() );

        guy.address_needs( 0 );

        REQUIRE( guy.activity );
        CHECK( guy.activity.id() == ACT_FORAGE );
        CHECK( guy.activity.placement == here.get_abs( adj ) );
    }

    // NOTE: Normal-hunger path also wires scavenging as fallback (after the
    // one_in(3) gate in address_needs), but that gate is non-deterministic.
    // No test for normal-path scavenging until we have a clean deterministic
    // seam. Extreme-hunger path is deterministic and fully covered here.

    SECTION( "NPC paths toward distant harvestable when starving" ) {
        guy.set_stored_kcal( 2000 );
        guy.set_thirst( 0 );
        const tripoint_bub_ms bush = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( bush, ter_t_underbrush );
        here.build_map_cache( 0 );
        const int dist_before = rl_dist( guy.pos_bub(), bush );

        guy.address_needs( 0 );

        CHECK( rl_dist( guy.pos_bub(), bush ) < dist_before );
    }

    SECTION( "scavenging blocked by danger gate" ) {
        guy.set_stored_kcal( 2000 );
        guy.set_thirst( 0 );
        here.ter_set( adj, ter_t_underbrush );
        here.build_map_cache( 0 );
        const tripoint_bub_ms before = guy.pos_bub();

        guy.address_needs( NPC_DANGER_VERY_LOW + 1 );

        CHECK_FALSE( guy.has_activity() );
        CHECK( guy.pos_bub() == before );
    }

    SECTION( "ground food preferred over scavenging" ) {
        guy.set_stored_kcal( 2000 );
        guy.set_thirst( 0 );
        here.add_item_or_charges( adj, item( itype_sandwich_cheese_grilled ) );
        const tripoint_bub_ms west = guy.pos_bub() + point::west;
        here.ter_set( west, ter_t_underbrush );
        here.build_map_cache( 0 );
        const size_t items_before = here.i_at( adj ).size();

        guy.address_needs( 0 );

        CHECK( here.i_at( adj ).size() < items_before );
        CHECK_FALSE( guy.has_activity() );
    }

    SECTION( "distant reachable food preferred over nearby harvestable" ) {
        guy.set_stored_kcal( 2000 );
        guy.set_thirst( 0 );
        const tripoint_bub_ms food_at = guy.pos_bub() + tripoint( 4, 0, 0 );
        here.add_item_or_charges( food_at, item( itype_sandwich_cheese_grilled ) );
        here.ter_set( adj, ter_t_underbrush );
        here.build_map_cache( 0 );
        const int dist_to_food = rl_dist( guy.pos_bub(), food_at );

        guy.address_needs( 0 );

        CHECK( rl_dist( guy.pos_bub(), food_at ) < dist_to_food );
        CHECK_FALSE( guy.has_activity() );
    }

    SECTION( "cargo food preferred over scavenging" ) {
        guy.set_stored_kcal( 2000 );
        guy.set_thirst( 0 );
        vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                          adj, 0_degrees, 0, 0 );
        REQUIRE( cart != nullptr );
        const std::optional<vpart_reference> cargo = here.veh_at( adj ).cargo();
        REQUIRE( cargo.has_value() );
        cargo->vehicle().add_item( here, cargo->part(), item( itype_sandwich_cheese_grilled ) );
        const size_t cargo_before = cargo->items().size();
        const tripoint_bub_ms west = guy.pos_bub() + point::west;
        here.ter_set( west, ter_t_underbrush );
        here.build_map_cache( 0 );

        guy.address_needs( 0 );

        CHECK_FALSE( guy.has_activity() );
        CHECK( cargo->items().size() < cargo_before );
    }

    // Forage activity completion (terrain transform, item spawn) belongs in the
    // activity test seam, not the NPC needs suite. The NPC activity-resume
    // backlog triggers ACT_NULL when driving do_turn() directly in tests.
    // Completion coverage for forage_activity_actor should go in
    // player_activities_test.cpp or harvest_test.cpp.
}

// Helper: place double-thick glass enclosure around a point (seals diagonal gaps)
static void enclose_in_glass( map &here, const tripoint_bub_ms &center )
{
    for( int r = 1; r <= 2; ++r ) {
        for( const tripoint_bub_ms &w : here.points_in_radius( center, r ) ) {
            if( w != center ) {
                here.ter_set( w, ter_t_wall_glass );
            }
        }
    }
}

// Helper: set up a sleepy non-ally NPC ready to call move().
// Mirrors npc_nonally_sleeps_when_tired setup.
static void make_npc_sleepy( npc &guy )
{
    guy.set_sleepiness( 800 );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.set_mission( NPC_MISSION_SHELTER );
    guy.set_moves( 100 );
}

TEST_CASE( "npc_sleep_spot_reachability", "[npc][needs]" )
{
    clear_map_without_vision();
    calendar::turn = calendar::turn_zero + 1_hours;
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    REQUIRE_FALSE( guy.is_player_ally() );
    map &here = get_map();

    SECTION( "reachable worse bed beats unreachable better bed" ) {
        // Good bed behind glass walls (unreachable without bashing)
        const tripoint_bub_ms walled_bed = guy.pos_bub() + tripoint( 5, 0, 0 );
        here.ter_set( walled_bed, ter_t_floor );
        here.furn_set( walled_bed, furn_f_bed );
        enclose_in_glass( here, walled_bed );
        // Worse bed on open ground (reachable)
        const tripoint_bub_ms open_bed = guy.pos_bub() + tripoint( 0, 3, 0 );
        here.ter_set( open_bed, ter_t_floor );
        here.furn_set( open_bed, furn_f_makeshift_bed );

        const int dist_walled_before = rl_dist( guy.pos_bub(), walled_bed );
        const int dist_open_before = rl_dist( guy.pos_bub(), open_bed );

        make_npc_sleepy( guy );
        guy.move();

        CHECK( rl_dist( guy.pos_bub(), open_bed ) < dist_open_before );
        CHECK( rl_dist( guy.pos_bub(), walled_bed ) >= dist_walled_before );
    }

    SECTION( "bed behind closed door is reachable" ) {
        const tripoint_bub_ms door = guy.pos_bub() + tripoint( 2, 0, 0 );
        here.ter_set( door, ter_t_door_c );
        const tripoint_bub_ms bed = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( bed, ter_t_floor );
        here.furn_set( bed, furn_f_bed );

        const int dist_before = rl_dist( guy.pos_bub(), bed );

        make_npc_sleepy( guy );
        guy.move();

        CHECK( rl_dist( guy.pos_bub(), bed ) < dist_before );
    }

    SECTION( "bed behind bashable furniture is unreachable" ) {
        // Seal bed behind double-thick locker enclosure (same pattern as glass
        // wall tests -- single thickness has diagonal gaps A* can exploit).
        const tripoint_bub_ms blocked_bed = guy.pos_bub() + tripoint( 5, 0, 0 );
        here.ter_set( blocked_bed, ter_t_floor );
        here.furn_set( blocked_bed, furn_f_bed );
        for( int r = 1; r <= 2; ++r ) {
            for( const tripoint_bub_ms &w : here.points_in_radius( blocked_bed, r ) ) {
                if( w != blocked_bed ) {
                    here.furn_set( w, furn_f_locker );
                }
            }
        }
        // Alternative on open ground
        const tripoint_bub_ms open_bed = guy.pos_bub() + tripoint( 0, 3, 0 );
        here.ter_set( open_bed, ter_t_floor );
        here.furn_set( open_bed, furn_f_makeshift_bed );

        const int dist_blocked_before = rl_dist( guy.pos_bub(), blocked_bed );
        const int dist_open_before = rl_dist( guy.pos_bub(), open_bed );

        make_npc_sleepy( guy );
        guy.move();

        CHECK( rl_dist( guy.pos_bub(), open_bed ) < dist_open_before );
        CHECK( rl_dist( guy.pos_bub(), blocked_bed ) >= dist_blocked_before );
    }

    SECTION( "no reachable bed: NPC sleeps in place" ) {
        const tripoint_bub_ms walled_bed = guy.pos_bub() + tripoint( 5, 0, 0 );
        here.ter_set( walled_bed, ter_t_floor );
        here.furn_set( walled_bed, furn_f_bed );
        enclose_in_glass( here, walled_bed );

        const tripoint_bub_ms before = guy.pos_bub();

        make_npc_sleepy( guy );
        guy.move();

        CHECK( guy.pos_bub() == before );
        CHECK( ( guy.has_effect( effect_sleep ) || guy.has_effect( effect_lying_down ) ) );
    }
}

TEST_CASE( "npc_no_go_zone_blocks_needs", "[npc][needs]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    calendar::turn = calendar::turn_zero + 1_hours;
    get_weather().forced_temperature = 20_C;
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.stomach.empty();
    guy.guts.empty();
    REQUIRE_FALSE( guy.is_player_ally() );
    map &here = get_map();

    // NPC_NO_GO zone covering tiles east of NPC (x >= npc_x + 1)
    const tripoint_abs_ms zone_start = guy.pos_abs() + tripoint( 1, -10, 0 );
    const tripoint_abs_ms zone_end = guy.pos_abs() + tripoint( 20, 10, 0 );
    zone_manager &mgr = zone_manager::get_manager();
    mgr.add( "test_no_go", zone_type_NPC_NO_GO, guy.get_fac_id(),
             false, true, zone_start, zone_end );
    mgr.cache_data();

    SECTION( "sleep: best bed in NPC_NO_GO is skipped" ) {
        // Good bed in no-go zone
        const tripoint_bub_ms zoned_bed = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( zoned_bed, ter_t_floor );
        here.furn_set( zoned_bed, furn_f_bed );
        // Worse bed outside zone (west)
        const tripoint_bub_ms open_bed = guy.pos_bub() + tripoint( 0, -3, 0 );
        here.ter_set( open_bed, ter_t_floor );
        here.furn_set( open_bed, furn_f_makeshift_bed );

        const int dist_zoned_before = rl_dist( guy.pos_bub(), zoned_bed );
        const int dist_open_before = rl_dist( guy.pos_bub(), open_bed );

        make_npc_sleepy( guy );
        guy.move();

        CHECK( rl_dist( guy.pos_bub(), open_bed ) < dist_open_before );
        CHECK( rl_dist( guy.pos_bub(), zoned_bed ) >= dist_zoned_before );
    }

    SECTION( "food: adjacent ground food in NPC_NO_GO not consumed" ) {
        guy.set_stored_kcal( 2000 );
        guy.set_hunger( 300 );
        guy.set_thirst( 100 );
        const tripoint_bub_ms food_at = guy.pos_bub() + point::east;
        here.add_item_or_charges( food_at, item( itype_sandwich_cheese_grilled ) );
        const size_t items_before = here.i_at( food_at ).size();
        const tripoint_bub_ms before = guy.pos_bub();

        guy.address_needs( 0 );

        CHECK( here.i_at( food_at ).size() == items_before );
        CHECK( guy.pos_bub() == before );
    }

    SECTION( "water: adjacent water in NPC_NO_GO not used" ) {
        guy.set_thirst( 200 );
        REQUIRE( guy.get_thirst() > 80 );
        const tripoint_bub_ms water_at = guy.pos_bub() + point::east;
        here.ter_set( water_at, ter_t_water_sh );

        guy.address_needs( 0 );

        CHECK( guy.stomach.get_water() == 0_ml );
    }

    SECTION( "shelter: indoor tile in NPC_NO_GO skipped" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        const tripoint_bub_ms shelter = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( shelter, ter_t_floor );
        here.build_map_cache( 0 );
        const tripoint_bub_ms before = guy.pos_bub();

        guy.address_needs( 0 );

        CHECK( guy.pos_bub() == before );
    }

    SECTION( "harvest: harvestable in NPC_NO_GO skipped" ) {
        guy.set_stored_kcal( 2000 );
        guy.set_hunger( 300 );
        guy.set_thirst( 100 );
        const tripoint_bub_ms harvest_at = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( harvest_at, ter_t_underbrush );
        here.build_map_cache( 0 );
        const tripoint_bub_ms before = guy.pos_bub();

        guy.address_needs( 0 );

        CHECK( guy.pos_bub() == before );
    }

    SECTION( "clothing: warm clothing in NPC_NO_GO not picked up" ) {
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        const tripoint_bub_ms cloth_at = guy.pos_bub() + point::east;
        here.add_item_or_charges( cloth_at, item( itype_sweater ) );
        const size_t items_before = here.i_at( cloth_at ).size();

        guy.address_needs( 0 );

        CHECK( here.i_at( cloth_at ).size() == items_before );
    }

    // Cleanup: remove test zone so it doesn't leak into other tests
    mgr.clear();
    mgr.cache_data();
}

TEST_CASE( "npc_is_no_go_position", "[npc][needs]" )
{
    clear_map_without_vision();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );

    const tripoint_abs_ms inside = guy.pos_abs() + tripoint( 3, 0, 0 );
    const tripoint_abs_ms outside = guy.pos_abs() + tripoint( -3, 0, 0 );

    zone_manager &mgr = zone_manager::get_manager();
    mgr.add( "test_no_go", zone_type_NPC_NO_GO, guy.get_fac_id(),
             false, true,
             guy.pos_abs() + tripoint( 1, -5, 0 ),
             guy.pos_abs() + tripoint( 10, 5, 0 ) );
    mgr.cache_data();

    SECTION( "true for matching faction zone" ) {
        CHECK( guy.is_no_go_position( inside ) );
    }

    SECTION( "false outside zone" ) {
        CHECK_FALSE( guy.is_no_go_position( outside ) );
    }

    SECTION( "false for different faction" ) {
        // Zone is for guy's faction; a different NPC with different faction
        // should not be blocked
        npc &other = spawn_npc( { 55, 50 }, "test_talker" );
        clear_character( other, true );
        // Set a different faction
        other.set_fac( faction_your_followers );
        REQUIRE( other.get_fac_id() != guy.get_fac_id() );
        CHECK_FALSE( other.is_no_go_position( inside ) );
    }

    mgr.clear();
    mgr.cache_data();
}

TEST_CASE( "npc_persistent_guard_pos_authority", "[npc][needs]" )
{
    clear_map_without_vision();
    calendar::turn = calendar::turn_zero + 1_hours;
    map &here = get_map();

    SECTION( "RETURN_TO_START_POS sets persistent guard_pos on first spawn" ) {
        npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( guy, true );
        guy.set_mutation( trait_RETURN_TO_START_POS );
        REQUIRE( guy.has_trait( trait_RETURN_TO_START_POS ) );

        guy.regen_ai_cache();

        CHECK( guy.guard_pos.has_value() );
        CHECK( *guy.guard_pos == guy.pos_abs() );
    }

    SECTION( "cache loss re-fills from persistent, not current position" ) {
        npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( guy, true );
        guy.set_mutation( trait_RETURN_TO_START_POS );

        guy.regen_ai_cache();
        const tripoint_abs_ms original_post = guy.pos_abs();
        REQUIRE( guy.guard_pos == original_post );

        // Move NPC away from post
        guy.setpos( here, guy.pos_bub() + tripoint( 5, 0, 0 ) );
        REQUIRE( guy.pos_abs() != original_post );

        // Simulate cache loss
        guy.clear_ai_guard_pos();
        REQUIRE_FALSE( guy.get_ai_guard_pos().has_value() );

        guy.regen_ai_cache();

        CHECK( guy.get_ai_guard_pos().has_value() );
        CHECK( *guy.get_ai_guard_pos() == original_post );
    }

    SECTION( "set_guard_pos writes both persistent and cache" ) {
        npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( guy, true );
        const tripoint_abs_ms duty_post = guy.pos_abs() + tripoint( 10, 0, 0 );

        guy.set_guard_pos( duty_post );

        CHECK( guy.guard_pos == duty_post );
        CHECK( guy.get_ai_guard_pos() == duty_post );

        // Simulate cache loss and rebuild
        guy.clear_ai_guard_pos();
        guy.regen_ai_cache();

        CHECK( guy.get_ai_guard_pos() == duty_post );
    }

    SECTION( "sound anchor temporarily overrides cache, persistent survives" ) {
        npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( guy, true );
        const tripoint_abs_ms permanent_post = guy.pos_abs();
        guy.guard_pos = permanent_post;
        guy.regen_ai_cache();
        REQUIRE( guy.get_ai_guard_pos() == permanent_post );

        // Sound investigation writes a temporary anchor to ai_cache
        // without touching persistent guard_pos.
        const tripoint_abs_ms sound_anchor = guy.pos_abs() + tripoint( 10, 0, 0 );
        guy.set_ai_guard_pos( sound_anchor );
        CHECK( guy.get_ai_guard_pos() == sound_anchor );
        CHECK( guy.guard_pos == permanent_post );

        // NPC reaches sound source; cache cleared by execute_action.
        guy.clear_ai_guard_pos();
        REQUIRE_FALSE( guy.get_ai_guard_pos().has_value() );
        // Persistent still untouched.
        CHECK( guy.guard_pos == permanent_post );

        // Generic refill re-populates cache from persistent.
        guy.regen_ai_cache();
        CHECK( guy.get_ai_guard_pos() == permanent_post );
    }
}

TEST_CASE( "npc_shopkeeper_sleep_wake_return", "[npc][needs]" )
{
    clear_map_without_vision();
    clear_avatar();
    calendar::turn = calendar::turn_zero + 1_hours;
    map &here = get_map();
    // Move player underground so they don't interact with NPC at all
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, -2 ) );

    SECTION( "shopkeeper returns to post after sleep and cache loss" ) {
        npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( guy, true );
        guy.set_mutation( trait_RETURN_TO_START_POS );
        guy.set_mission( NPC_MISSION_SHOPKEEP );
        REQUIRE_FALSE( guy.is_player_ally() );

        guy.regen_ai_cache();
        const tripoint_abs_ms post = guy.pos_abs();
        REQUIRE( guy.guard_pos == post );

        // Place a bed nearby and make NPC sleepy
        const tripoint_bub_ms bed_pos = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( bed_pos, ter_t_floor );
        here.furn_set( bed_pos, furn_f_bed );

        guy.set_sleepiness( 800 );
        guy.set_hunger( 0 );
        guy.set_thirst( 0 );
        guy.set_stored_kcal( guy.get_healthy_kcal() );
        guy.set_all_parts_temp_conv( BODYTEMP_NORM );
        guy.set_all_parts_temp_cur( BODYTEMP_NORM );

        // Move toward bed until asleep or 10 turns
        for( int i = 0; i < 10 && !guy.has_effect( effect_sleep ); ++i ) {
            guy.set_moves( 100 );
            guy.move();
        }
        REQUIRE( guy.has_effect( effect_sleep ) );
        REQUIRE( guy.pos_abs() != post );

        // Simulate cache loss while at bed
        guy.clear_ai_guard_pos();
        REQUIRE_FALSE( guy.get_ai_guard_pos().has_value() );

        // Regen re-fills from persistent post, not bed position
        guy.regen_ai_cache();
        CHECK( guy.get_ai_guard_pos() == post );

        // Wake up and return
        guy.remove_effect( effect_sleep );
        guy.remove_effect( effect_lying_down );
        guy.set_sleepiness( 0 );

        for( int i = 0; i < 20; ++i ) {
            guy.set_moves( 100 );
            guy.move();
            if( guy.pos_abs() == post ) {
                break;
            }
        }
        CHECK( guy.pos_abs() == post );

        // Stay at post on subsequent turns
        for( int i = 0; i < 3; ++i ) {
            guy.set_moves( 100 );
            guy.move();
        }
        CHECK( guy.pos_abs() == post );
    }

    SECTION( "shopkeeper at post with no needs stays put" ) {
        npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( guy, true );
        guy.set_mutation( trait_RETURN_TO_START_POS );
        guy.set_mission( NPC_MISSION_SHOPKEEP );
        guy.set_hunger( 0 );
        guy.set_thirst( 0 );
        guy.set_stored_kcal( guy.get_healthy_kcal() );
        guy.set_all_parts_temp_conv( BODYTEMP_NORM );
        guy.set_all_parts_temp_cur( BODYTEMP_NORM );
        guy.regen_ai_cache();
        const tripoint_abs_ms post = guy.pos_abs();

        for( int i = 0; i < 3; ++i ) {
            guy.set_moves( 100 );
            guy.move();
        }
        CHECK( guy.pos_abs() == post );
    }

    SECTION( "NO_NPC_FOOD caps unscheduled NPC at TIRED" ) {
        override_option no_food( "NO_NPC_FOOD", "true" );
        npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( guy, true );
        guy.set_sleepiness( sleepiness_levels::TIRED );
        REQUIRE_FALSE( guy.needs_food() );
        guy.update_needs( 1 );
        CHECK( guy.get_sleepiness() <= static_cast<int>( sleepiness_levels::TIRED ) );
    }

    SECTION( "DEAD_TIRED shopkeeper commits to sleep, no oscillation" ) {
        npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( guy, true );
        guy.set_mutation( trait_RETURN_TO_START_POS );
        guy.set_mission( NPC_MISSION_SHOPKEEP );
        guy.set_hunger( 0 );
        guy.set_thirst( 0 );
        guy.set_stored_kcal( guy.get_healthy_kcal() );
        guy.set_all_parts_temp_conv( BODYTEMP_NORM );
        guy.set_all_parts_temp_cur( BODYTEMP_NORM );
        guy.regen_ai_cache();
        const tripoint_abs_ms post = guy.pos_abs();
        here.furn_set( guy.pos_bub() + tripoint( 3, 0, 0 ), furn_f_bed );
        // Sleepiness 800: sleep_urgency 0.8 beats on-shift duty 0.45,
        // so BT says go_to_sleep. While walking to bed (1-3 tiles from
        // post), goal commitment prevents flipping to return_to_guard_pos.
        guy.set_sleepiness( 800 );

        bool oscillated = false;
        bool reached_sleep = false;
        for( int i = 0; i < 20; ++i ) {
            guy.set_moves( 100 );
            guy.move();
            if( guy.has_effect( effect_sleep ) ||
                guy.has_effect( effect_lying_down ) ) {
                reached_sleep = true;
                break;
            }
            if( i > 0 && guy.pos_abs() == post ) {
                oscillated = true;
                break;
            }
        }
        CHECK_FALSE( oscillated );
        CHECK( reached_sleep );
    }

    SECTION( "shopkeeper at post sleeps when tired" ) {
        npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( guy, true );
        guy.set_mutation( trait_RETURN_TO_START_POS );
        guy.set_mission( NPC_MISSION_SHOPKEEP );
        guy.set_hunger( 0 );
        guy.set_thirst( 0 );
        guy.set_stored_kcal( guy.get_healthy_kcal() );
        guy.set_all_parts_temp_conv( BODYTEMP_NORM );
        guy.set_all_parts_temp_cur( BODYTEMP_NORM );
        guy.regen_ai_cache();
        // Well above TIRED threshold -- BT says go_to_sleep.
        guy.set_sleepiness( 800 );

        guy.set_moves( 100 );
        guy.move();

        CHECK( ( guy.has_effect( effect_sleep ) || guy.has_effect( effect_lying_down ) ) );
    }

    SECTION( "tired-but-not-exhausted shopkeeper stays on duty" ) {
        npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( guy, true );
        guy.set_mutation( trait_RETURN_TO_START_POS );
        guy.set_mission( NPC_MISSION_SHOPKEEP );
        guy.regen_ai_cache();
        const tripoint_abs_ms post = guy.pos_abs();

        // Displace 10 tiles from post. duty_urgency = min(0.5, 10*0.05) = 0.5.
        // sleepiness_urgency = 400/1000 = 0.4. Duty wins.
        guy.setpos( here, guy.pos_bub() + tripoint( 10, 0, 0 ) );
        REQUIRE( guy.pos_abs() != post );
        const int dist_to_post = rl_dist( guy.pos_bub(), here.get_bub( post ) );

        guy.set_sleepiness( 400 );
        guy.set_moves( 100 );
        guy.move();

        // Should move toward post, not sleep
        CHECK( rl_dist( guy.pos_bub(), here.get_bub( post ) ) < dist_to_post );
        CHECK_FALSE( guy.has_effect( effect_sleep ) );
        CHECK_FALSE( guy.has_effect( effect_lying_down ) );
    }
}

// Helper: set up an NPC as an intercom operator with a given faction and shift.
static npc &spawn_intercom_operator( const point_bub_ms &pos, const npc_class_id &cls,
                                     const faction_id &fac )
{
    npc &guy = spawn_npc( pos, "test_talker" );
    clear_character( guy, true );
    guy.set_mutation( trait_INTERCOM_OPERATOR );
    guy.myclass = cls;
    guy.set_fac( fac );
    return guy;
}

TEST_CASE( "find_intercom_operator_shift_selection", "[npc][intercom]" )
{
    clear_map_without_vision();

    // Real intercom class IDs with work_hours from JSON.
    // NC_ROBOFAC_INTERCOM_DAY [6,18], NC_ROBOFAC_INTERCOM_NIGHT [18,6].
    SECTION( "noon selects day operator" ) {
        calendar::turn = calendar::turn_zero + 12_hours;
        npc &day = spawn_intercom_operator( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY, faction_robofac );
        npc &night = spawn_intercom_operator( { 52, 50 }, NC_ROBOFAC_INTERCOM_NIGHT, faction_robofac );

        npc *result = find_intercom_operator( trait_INTERCOM_OPERATOR, faction_robofac );
        REQUIRE( result != nullptr );
        CHECK( result->getID() == day.getID() );
        ( void )night; // suppress unused
    }
    SECTION( "midnight selects night operator" ) {
        calendar::turn = calendar::turn_zero + 0_hours;
        npc &day = spawn_intercom_operator( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY, faction_robofac );
        npc &night = spawn_intercom_operator( { 52, 50 }, NC_ROBOFAC_INTERCOM_NIGHT, faction_robofac );

        npc *result = find_intercom_operator( trait_INTERCOM_OPERATOR, faction_robofac );
        REQUIRE( result != nullptr );
        CHECK( result->getID() == night.getID() );
        ( void )day;
    }
    SECTION( "boundary 06:00 selects day (start-inclusive)" ) {
        calendar::turn = calendar::turn_zero + 6_hours;
        npc &day = spawn_intercom_operator( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY, faction_robofac );
        spawn_intercom_operator( { 52, 50 }, NC_ROBOFAC_INTERCOM_NIGHT, faction_robofac );

        npc *result = find_intercom_operator( trait_INTERCOM_OPERATOR, faction_robofac );
        REQUIRE( result != nullptr );
        CHECK( result->getID() == day.getID() );
    }
    SECTION( "boundary 18:00 selects night (day end-exclusive)" ) {
        calendar::turn = calendar::turn_zero + 18_hours;
        spawn_intercom_operator( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY, faction_robofac );
        npc &night = spawn_intercom_operator( { 52, 50 }, NC_ROBOFAC_INTERCOM_NIGHT, faction_robofac );

        npc *result = find_intercom_operator( trait_INTERCOM_OPERATOR, faction_robofac );
        REQUIRE( result != nullptr );
        CHECK( result->getID() == night.getID() );
    }
    SECTION( "fallback to awake off-shift when on-shift is asleep" ) {
        calendar::turn = calendar::turn_zero + 12_hours;
        npc &day = spawn_intercom_operator( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY, faction_robofac );
        npc &night = spawn_intercom_operator( { 52, 50 }, NC_ROBOFAC_INTERCOM_NIGHT, faction_robofac );

        // Put the on-shift (day) operator to sleep
        day.add_effect( effect_sleep, 1_hours );

        npc *result = find_intercom_operator( trait_INTERCOM_OPERATOR, faction_robofac );
        REQUIRE( result != nullptr );
        // Day is asleep, night is awake -- falls back to night
        CHECK( result->getID() == night.getID() );
    }
    SECTION( "no operators returns nullptr" ) {
        calendar::turn = calendar::turn_zero + 12_hours;
        npc *result = find_intercom_operator( trait_INTERCOM_OPERATOR, faction_robofac );
        CHECK( result == nullptr );
    }
    SECTION( "wrong faction not selected" ) {
        calendar::turn = calendar::turn_zero + 12_hours;
        spawn_intercom_operator( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY, faction_your_followers );

        npc *result = find_intercom_operator( trait_INTERCOM_OPERATOR, faction_robofac );
        CHECK( result == nullptr );
    }
}

// --- Schedule reconciliation tests ---

static npc &spawn_scheduled_npc( const point_bub_ms &pos, const npc_class_id &cls )
{
    npc &guy = spawn_npc( pos, "test_talker" );
    clear_character( guy, true );
    guy.myclass = cls;
    guy.set_mutation( trait_RETURN_TO_START_POS );
    guy.set_mission( NPC_MISSION_SHOPKEEP );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    return guy;
}

TEST_CASE( "schedule_reconciliation_no_npc_food", "[npc][schedule]" )
{
    clear_map_without_vision();
    override_option no_food( "NO_NPC_FOOD", "true" );

    SECTION( "wakes and zeroes sleepiness at shift start" ) {
        calendar::turn = calendar::turn_zero + 12_hours;
        npc &guy = spawn_scheduled_npc( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY );
        guy.set_sleepiness( 100 );
        guy.add_effect( effect_sleep, 1_hours );
        REQUIRE( guy.in_sleep_state() );
        REQUIRE_FALSE( guy.needs_food() );

        guy.reconcile_schedule();

        CHECK_FALSE( guy.in_sleep_state() );
        CHECK( guy.get_sleepiness() == 0 );
    }

    SECTION( "floors sleepiness at off-shift transition" ) {
        calendar::turn = calendar::turn_zero + 20_hours;
        npc &guy = spawn_scheduled_npc( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY );
        guy.set_sleepiness( 100 );
        REQUIRE_FALSE( guy.needs_food() );

        guy.reconcile_schedule();

        CHECK( guy.get_sleepiness() >= static_cast<int>( sleepiness_levels::TIRED ) );
    }

    SECTION( "on_load sets sleepiness proportional to shift" ) {
        // 12:00 is 6 hours into a [6,18] shift (12h total).
        // Expected: TIRED * 6/12 = TIRED/2
        calendar::turn = calendar::turn_zero + 12_hours;
        npc &guy = spawn_scheduled_npc( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY );
        guy.set_sleepiness( 0 );
        REQUIRE_FALSE( guy.needs_food() );

        guy.reconcile_schedule_on_load();

        int expected = static_cast<int>( sleepiness_levels::TIRED ) / 2;
        CHECK( guy.get_sleepiness() >= expected - 5 );
        CHECK( guy.get_sleepiness() <= expected + 5 );
    }

    SECTION( "loaded NPC covers full shift then sleeps" ) {
        // Start just before shift end so we don't need a massive time jump.
        calendar::turn = calendar::turn_zero + 17_hours + 59_minutes;
        map &here = get_map();
        clear_avatar();
        get_player_character().setpos( here, tripoint_bub_ms( 50, 50, -2 ) );
        npc &guy = spawn_scheduled_npc( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY );
        guy.update_body( calendar::turn, calendar::turn );
        guy.regen_ai_cache();
        const tripoint_abs_ms post = guy.pos_abs();
        REQUIRE( guy.guard_pos == post );

        // Place a bed 3 tiles away for sleep target.
        const tripoint_bub_ms bed_pos = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( bed_pos, ter_t_floor );
        here.furn_set( bed_pos, furn_f_bed );

        // On-shift: NPC should stay at post.
        for( int i = 0; i < 5; ++i ) {
            guy.set_moves( 100 );
            guy.move();
        }
        CHECK( guy.pos_abs() == post );
        CHECK_FALSE( guy.in_sleep_state() );

        // Cross shift boundary. Small time jump (2 minutes).
        calendar::turn = calendar::turn_zero + 18_hours + 10_seconds;
        guy.update_body( calendar::turn - 10_seconds, calendar::turn );
        guy.npc_update_body();

        CHECK( guy.get_sleepiness() >= static_cast<int>( sleepiness_levels::TIRED ) );

        // BT should now send NPC to sleep.
        bool reached_sleep = false;
        for( int i = 0; i < 20; ++i ) {
            guy.set_moves( 100 );
            guy.move();
            if( guy.has_effect( effect_sleep ) ||
                guy.has_effect( effect_lying_down ) ) {
                reached_sleep = true;
                break;
            }
        }
        CHECK( reached_sleep );
    }
}

TEST_CASE( "schedule_reconciliation_normal_needs", "[npc][schedule]" )
{
    clear_map_without_vision();

    SECTION( "wakes but preserves sleepiness at shift start" ) {
        calendar::turn = calendar::turn_zero + 12_hours;
        npc &guy = spawn_scheduled_npc( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY );
        guy.set_sleepiness( 100 );
        guy.add_effect( effect_sleep, 1_hours );
        REQUIRE( guy.in_sleep_state() );
        REQUIRE( guy.needs_food() );

        guy.reconcile_schedule();

        CHECK_FALSE( guy.in_sleep_state() );
        CHECK( guy.get_sleepiness() == 100 );
    }

    SECTION( "does not floor sleepiness off-shift" ) {
        calendar::turn = calendar::turn_zero + 20_hours;
        npc &guy = spawn_scheduled_npc( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY );
        guy.set_sleepiness( 50 );
        REQUIRE( guy.needs_food() );

        guy.reconcile_schedule();

        CHECK( guy.get_sleepiness() == 50 );
    }

    SECTION( "on_load does not overwrite sleepiness" ) {
        calendar::turn = calendar::turn_zero + 12_hours;
        npc &guy = spawn_scheduled_npc( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY );
        guy.set_sleepiness( 300 );
        REQUIRE( guy.needs_food() );

        guy.reconcile_schedule_on_load();

        CHECK( guy.get_sleepiness() == 300 );
    }

    SECTION( "leaves sleeping NPC alone off-shift" ) {
        calendar::turn = calendar::turn_zero + 22_hours;
        npc &guy = spawn_scheduled_npc( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY );
        guy.add_effect( effect_sleep, 1_hours );
        REQUIRE( guy.in_sleep_state() );

        guy.reconcile_schedule();

        CHECK( guy.in_sleep_state() );
    }
}

TEST_CASE( "npc_update_body_calls_reconcile_schedule", "[npc][schedule]" )
{
    clear_map_without_vision();
    override_option no_food( "NO_NPC_FOOD", "true" );

    // Align to a 10-second boundary so once_every(10_seconds) fires.
    calendar::turn = calendar::turn_zero + 12_hours;
    npc &guy = spawn_scheduled_npc( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY );
    guy.add_effect( effect_sleep, 1_hours );
    guy.update_body( calendar::turn, calendar::turn );
    REQUIRE( guy.in_sleep_state() );

    // Advance exactly to the next 10-second boundary.
    calendar::turn += 10_seconds;
    guy.npc_update_body();

    CHECK_FALSE( guy.in_sleep_state() );
}

// --- Trade delegate tests ---

TEST_CASE( "trade_delegate", "[npc][trade]" )
{
    clear_map_without_vision();

    SECTION( "default is self" ) {
        npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( guy, true );

        CHECK( &guy.get_trade_delegate() == &guy );
    }

    SECTION( "intercom operator returns supply clerk" ) {
        npc &op = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( op, true );
        op.set_mutation( trait_INTERCOM_OPERATOR );
        op.set_fac( faction_robofac );

        npc &clerk = spawn_npc( { 52, 50 }, "test_talker" );
        clear_character( clerk, true );
        clerk.set_mutation( trait_TRADE_BACKEND );
        clerk.set_fac( faction_robofac );

        CHECK( &op.get_trade_delegate() == &clerk );
    }

    SECTION( "no clerk falls back to self" ) {
        npc &op = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( op, true );
        op.set_mutation( trait_INTERCOM_OPERATOR );
        op.set_fac( faction_robofac );

        CHECK( &op.get_trade_delegate() == &op );
    }

    SECTION( "ignores non-backend shopkeepers" ) {
        npc &op = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( op, true );
        op.set_mutation( trait_INTERCOM_OPERATOR );
        op.set_fac( faction_robofac );

        // The intended backend.
        npc &clerk = spawn_npc( { 52, 50 }, "test_talker" );
        clear_character( clerk, true );
        clerk.set_mutation( trait_TRADE_BACKEND );
        clerk.set_fac( faction_robofac );

        // Another robofac shopkeeper (no TRADE_BACKEND).
        npc &other = spawn_npc( { 54, 50 }, "test_talker" );
        clear_character( other, true );
        other.myclass = test_shop_class;
        other.set_fac( faction_robofac );

        CHECK( &op.get_trade_delegate() == &clerk );
    }

    SECTION( "both operators share the same backend" ) {
        npc &day = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( day, true );
        day.set_mutation( trait_INTERCOM_OPERATOR );
        day.set_fac( faction_robofac );

        npc &night = spawn_npc( { 52, 50 }, "test_talker" );
        clear_character( night, true );
        night.set_mutation( trait_INTERCOM_OPERATOR );
        night.set_fac( faction_robofac );

        npc &clerk = spawn_npc( { 54, 50 }, "test_talker" );
        clear_character( clerk, true );
        clerk.set_mutation( trait_TRADE_BACKEND );
        clerk.set_fac( faction_robofac );

        CHECK( &day.get_trade_delegate() == &clerk );
        CHECK( &night.get_trade_delegate() == &clerk );
        CHECK( &day.get_trade_delegate() == &night.get_trade_delegate() );
    }
}

TEST_CASE( "intercom_shopkeeper_isolation", "[npc][trade][intercom]" )
{
    clear_map_without_vision();
    SECTION( "operators are not shopkeepers, clerk is" ) {
        npc &day = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( day, true );
        day.myclass = NC_ROBOFAC_INTERCOM_DAY;
        day.set_fac( faction_robofac );

        npc &night = spawn_npc( { 52, 50 }, "test_talker" );
        clear_character( night, true );
        night.myclass = NC_ROBOFAC_INTERCOM_NIGHT;
        night.set_fac( faction_robofac );

        npc &clerk = spawn_npc( { 54, 50 }, "test_talker" );
        clear_character( clerk, true );
        clerk.myclass = NC_ROBOFAC_INTERCOM_SUPPLY;
        clerk.set_fac( faction_robofac );

        CHECK_FALSE( day.is_shopkeeper() );
        CHECK_FALSE( night.is_shopkeeper() );
        CHECK( clerk.is_shopkeeper() );
    }

    SECTION( "shop_restock is no-op for non-shopkeeper operator" ) {
        npc &day = spawn_npc( { 50, 50 }, "test_talker" );
        clear_character( day, true );
        day.myclass = NC_ROBOFAC_INTERCOM_DAY;
        day.set_fac( faction_robofac );
        day.restock = calendar::turn_zero;

        day.shop_restock();

        // restock timestamp should be unchanged: shop_restock() is a
        // no-op for non-shopkeepers (is_shopkeeper() returns false).
        CHECK( day.restock == calendar::turn_zero );
    }
}

TEST_CASE( "shift_start_clears_stale_sleep_commitment", "[npc][schedule]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, -2 ) );
    SECTION( "already sleeping at shift start, duty wins after wake" ) {
        // Sleepiness 400: urgency 0.4, below on-shift duty at post (0.45).
        // reconcile_schedule clears the stale go_to_sleep commitment so
        // the BT picks duty instead of sending the NPC back to bed.
        calendar::turn = calendar::turn_zero + 12_hours;
        npc &guy = spawn_scheduled_npc( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY );
        guy.set_sleepiness( 400 );
        guy.add_effect( effect_sleep, 1_hours );
        guy.add_effect( effect_lying_down, 1_hours );
        guy.set_committed_goal( "go_to_sleep" );
        guy.regen_ai_cache();
        const tripoint_abs_ms post = guy.pos_abs();

        REQUIRE( guy.in_sleep_state() );
        REQUIRE( guy.needs_food() );
        REQUIRE( guy.guard_pos == post );

        guy.reconcile_schedule();

        CHECK_FALSE( guy.in_sleep_state() );
        CHECK( guy.get_sleepiness() == 400 );
        CHECK( guy.get_committed_goal().empty() );

        // Gameplay outcome: NPC stays at post instead of heading to bed.
        for( int i = 0; i < 3; ++i ) {
            guy.set_moves( 100 );
            guy.move();
        }
        CHECK( guy.pos_abs() == post );
        CHECK_FALSE( guy.in_sleep_state() );
    }

    SECTION( "walking toward bed at shift start" ) {
        // Off-shift, NPC committed to sleep but not sleeping yet.
        calendar::turn = calendar::turn_zero + 5_hours + 50_minutes;
        npc &guy = spawn_scheduled_npc( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY );
        guy.set_sleepiness( 400 );  // sleep_urgency 0.4 < duty_urgency 0.45
        guy.regen_ai_cache();
        guy.set_committed_goal( "go_to_sleep" );

        REQUIRE_FALSE( guy.in_sleep_state() );
        REQUIRE( guy.needs_food() );

        // Advance to on-shift.
        calendar::turn = calendar::turn_zero + 6_hours + 10_minutes;
        guy.set_moves( 100 );
        guy.move();

        // BT returns hold_position (duty 0.45 > sleep 0.4).
        // go_to_sleep completes because new_goal != "go_to_sleep".
        CHECK( guy.get_committed_goal() != "go_to_sleep" );
    }

    SECTION( "committed sleep yields to a different needs goal" ) {
        // Off-shift so duty is irrelevant. Sleepiness below TIRED (191)
        // so can_sleep fails and sleep branch is not running. Thirst is
        // high with water available, so BT returns drink_water (same
        // needs category, but different goal).
        calendar::turn = calendar::turn_zero + 20_hours; // off-shift [6,18]
        npc &guy = spawn_scheduled_npc( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY );
        guy.set_sleepiness( 100 );  // below TIRED (191): can_sleep fails
        guy.set_thirst( 600 );       // thirst_urgency = 0.5
        guy.i_add( item( itype_water_clean, calendar::turn ) );
        guy.regen_ai_cache();
        guy.set_committed_goal( "go_to_sleep" );

        REQUIRE( guy.get_committed_goal() == "go_to_sleep" );

        guy.set_moves( 100 );
        guy.move();

        // BT returns drink_water. Same needs category, but goal-level
        // completion fires because new_goal != "go_to_sleep".
        CHECK( guy.get_committed_goal() != "go_to_sleep" );
    }
}

TEST_CASE( "hold_position_commitment_completes_off_shift", "[npc][schedule]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, -2 ) );
    // Near end of shift.
    calendar::turn = calendar::turn_zero + 17_hours + 50_minutes;
    npc &guy = spawn_scheduled_npc( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY );
    // Low needs so BT returns idle off-shift, not go_to_sleep.
    guy.set_sleepiness( 0 );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.update_body( calendar::turn, calendar::turn );
    guy.regen_ai_cache();
    REQUIRE( guy.guard_pos == guy.pos_abs() );
    REQUIRE( guy.needs_food() );

    guy.set_committed_goal( "hold_position" );

    // Advance past shift end.
    calendar::turn = calendar::turn_zero + 18_hours + 10_minutes;

    guy.set_moves( 100 );
    guy.move();

    // BT returns idle (off-shift, no urgent needs). hold_position
    // should complete because BT no longer returns hold_position.
    CHECK( guy.get_committed_goal() != "hold_position" );
}

TEST_CASE( "intercom_operators_have_IGNORE_SOUND", "[npc][schedule]" )
{
    clear_map_without_vision();

    npc &day = spawn_npc( { 50, 50 }, "robofac_intercom_day" );
    CHECK( day.has_trait( trait_IGNORE_SOUND ) );

    npc &night = spawn_npc( { 52, 50 }, "robofac_intercom_night" );
    CHECK( night.has_trait( trait_IGNORE_SOUND ) );
}

TEST_CASE( "off_shift_displaced_npc_does_not_return_to_post", "[npc][schedule]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, -2 ) );
    // Off-shift, low needs so BT returns "idle".
    calendar::turn = calendar::turn_zero + 20_hours;
    npc &guy = spawn_scheduled_npc( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY );
    guy.set_sleepiness( 0 );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.update_body( calendar::turn, calendar::turn );
    guy.regen_ai_cache();

    const tripoint_abs_ms post = guy.pos_abs();
    REQUIRE( guy.guard_pos == post );
    REQUIRE( guy.needs_food() );

    // Displace the NPC 5 tiles from their guard post.
    guy.setpos( here, tripoint_bub_ms( 55, 50, 0 ) );
    // Pin overmap goal to current OMT so the is_stationary path
    // yields npc_pause rather than npc_goto_destination.
    guy.goal = guy.pos_abs_omt();
    guy.regen_ai_cache();
    const tripoint_abs_ms displaced = guy.pos_abs();

    REQUIRE( displaced != post );
    REQUIRE( guy.get_effective_guard_pos() == post );

    for( int i = 0; i < 3; ++i ) {
        guy.set_moves( 100 );
        guy.move();
    }

    CHECK( guy.pos_abs() == displaced );
}

TEST_CASE( "return_to_start_pos_npc_still_uses_legacy_fallback", "[npc][schedule]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, -2 ) );

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    // RETURN_TO_START_POS + SHELTER. BT duty fires, same result as before.
    guy.set_mutation( trait_RETURN_TO_START_POS );
    guy.set_mission( NPC_MISSION_SHELTER );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.regen_ai_cache();

    const tripoint_abs_ms post = guy.pos_abs();
    REQUIRE( guy.guard_pos == post );

    // Displace the NPC.
    guy.setpos( here, tripoint_bub_ms( 55, 50, 0 ) );
    guy.goal = guy.pos_abs_omt();
    guy.regen_ai_cache();
    const int initial_dist = rl_dist( guy.pos_bub(), here.get_bub( post ) );

    REQUIRE( guy.pos_abs() != post );

    // First tick commits to return_to_guard_pos.
    guy.set_moves( 100 );
    guy.move();
    CHECK( guy.get_committed_goal() == "return_to_guard_pos" );

    for( int i = 0; i < 2; ++i ) {
        guy.set_moves( 100 );
        guy.move();
    }

    CHECK( rl_dist( guy.pos_bub(), here.get_bub( post ) ) < initial_dist );
}

TEST_CASE( "follow_player_commitment_lifecycle", "[npc][behavior]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 65, 50, 0 ) );

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.rules.set_flag( ally_rule::follow_close );
    guy.set_mission( NPC_MISSION_NULL );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.regen_ai_cache();

    REQUIRE( rl_dist( guy.pos_abs(), get_player_character().pos_abs() ) > 4 );

    SECTION( "committed_goal set to follow_player when far" ) {
        guy.set_moves( 100 );
        guy.move();
        CHECK( guy.get_committed_goal() == "follow_player" );
    }
    SECTION( "committed_goal clears when within follow_distance" ) {
        for( int i = 0; i < 20; ++i ) {
            guy.set_moves( 100 );
            guy.move();
            if( rl_dist( guy.pos_abs(), get_player_character().pos_abs() )
                <= guy.follow_distance() ) {
                break;
            }
        }
        REQUIRE( rl_dist( guy.pos_abs(), get_player_character().pos_abs() )
                 <= guy.follow_distance() );
        guy.set_moves( 100 );
        guy.move();
        CHECK( guy.get_committed_goal().empty() );
    }
}

TEST_CASE( "temp_anchor_return_after_bt_idle", "[npc][behavior]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, -2 ) );

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_mission( NPC_MISSION_NULL );
    guy.set_attitude( NPCATT_NULL );
    guy.guard_pos = std::nullopt;

    // NPC displaced from origin by sound investigation, temp anchor set.
    const tripoint_abs_ms anchor = guy.pos_abs();
    guy.setpos( here, tripoint_bub_ms( 55, 50, 0 ) );
    guy.set_ai_guard_pos( anchor );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.goal = guy.pos_abs_omt();
    guy.regen_ai_cache();

    REQUIRE( guy.pos_abs() != anchor );
    REQUIRE( guy.get_effective_guard_pos() == anchor );
    REQUIRE_FALSE( guy.get_guard_post().has_value() );

    // Idle dispatch sees temp anchor, routes npc_return_to_guard_pos.
    for( int i = 0; i < 10; ++i ) {
        guy.set_moves( 100 );
        guy.move();
        if( guy.pos_abs() == anchor ) {
            break;
        }
    }

    CHECK( guy.pos_abs() == anchor );
}

TEST_CASE( "bt_idle_falls_through_to_cascade_goto", "[npc][behavior]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    // Player within follow_distance, so BT returns idle.
    get_player_character().setpos( here, tripoint_bub_ms( 52, 50, 0 ) );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_mission( NPC_MISSION_NULL );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    // Clear after regen so RETURN_TO_START_POS doesn't repopulate.
    guy.regen_ai_cache();
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    REQUIRE_FALSE( guy.get_effective_guard_pos().has_value() );
    REQUIRE( rl_dist( guy.pos_abs(), get_player_character().pos_abs() ) <= guy.follow_distance() );

    // Player-ordered goto.
    const tripoint_abs_ms target = guy.pos_abs() + tripoint( 5, 0, 0 );
    guy.goto_to_this_pos = target;

    guy.set_moves( 100 );
    guy.move();

    CHECK( guy.get_committed_goal() == "goto_ordered_position" );
    CHECK( rl_dist( guy.pos_abs(), target ) < 5 );
}

TEST_CASE( "anchored_follower_uses_duty_not_follow", "[npc][behavior]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();

    // Player west, guard post east. NPC at center.
    const tripoint_bub_ms npc_pos( 50, 50, 0 );
    const tripoint_bub_ms player_pos( 40, 50, 0 );
    get_player_character().setpos( here, player_pos );

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.setpos( here, npc_pos );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.rules.set_flag( ally_rule::follow_close );
    guy.set_mission( NPC_MISSION_GUARD_ALLY );
    const tripoint_abs_ms post = guy.pos_abs() + tripoint( 10, 0, 0 );
    guy.set_guard_pos( post );
    calendar::turn = calendar::turn_zero + 12_hours;
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.regen_ai_cache();

    REQUIRE( guy.myclass.is_valid() );
    const auto &[wh_start, wh_end] = guy.myclass.obj().get_work_hours();
    REQUIRE( ( wh_start == 0 && wh_end == 24 ) );
    REQUIRE( guy.get_guard_post().has_value() );
    REQUIRE( guy.pos_abs() != post );
    REQUIRE( rl_dist( guy.pos_abs(), get_player_character().pos_abs() ) > 4 );

    SECTION( "BT returns return_to_guard_pos, not follow_player" ) {
        behavior::character_oracle_t oracle( &guy );
        behavior::tree decision_tree;
        decision_tree.add( &behavior_node_t_npc_decision.obj() );
        CHECK( decision_tree.tick( &oracle ) == "return_to_guard_pos" );
    }
    SECTION( "live turns: NPC moves toward post, not player" ) {
        const int initial_dist_post = rl_dist( guy.pos_abs(), post );
        const int initial_dist_player = rl_dist( guy.pos_abs(),
                                        get_player_character().pos_abs() );
        for( int i = 0; i < 3; ++i ) {
            guy.set_moves( 100 );
            guy.move();
        }
        CHECK( rl_dist( guy.pos_abs(), post ) < initial_dist_post );
        CHECK( rl_dist( guy.pos_abs(), get_player_character().pos_abs() )
               > initial_dist_player );
    }
}

TEST_CASE( "guard_assignment_clears_follow_commitment", "[npc][behavior]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 65, 50, 0 ) );
    calendar::turn = calendar::turn_zero + 12_hours;

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.rules.set_flag( ally_rule::follow_close );
    guy.set_mission( NPC_MISSION_NULL );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.regen_ai_cache();

    REQUIRE( rl_dist( guy.pos_abs(), get_player_character().pos_abs() ) > 4 );

    guy.set_moves( 100 );
    guy.move();
    REQUIRE( guy.get_committed_goal() == "follow_player" );

    const tripoint_abs_ms pre_assign_pos = guy.pos_abs();

    // Simulate assign_guard: guard current tile.
    guy.set_attitude( NPCATT_NULL );
    guy.set_mission( NPC_MISSION_GUARD_ALLY );
    guy.set_omt_destination();
    REQUIRE( guy.get_guard_post().has_value() );
    REQUIRE( *guy.get_guard_post() == pre_assign_pos );
    REQUIRE( guy.myclass.is_valid() );
    const auto &[wh_start2, wh_end2] = guy.myclass.obj().get_work_hours();
    REQUIRE( ( wh_start2 == 0 && wh_end2 == 24 ) );

    // Follow commitment clears, NPC stays at post.
    guy.set_moves( 100 );
    guy.move();
    CHECK( guy.get_committed_goal().empty() );
    CHECK( guy.pos_abs() == pre_assign_pos );

    // Next tick: hold_position commits.
    guy.set_moves( 100 );
    guy.move();
    CHECK( guy.get_committed_goal() == "hold_position" );
    CHECK( guy.pos_abs() == pre_assign_pos );
}

TEST_CASE( "player_embarks_clears_follow_commitment", "[npc][behavior]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &pc = get_player_character();
    pc.setpos( here, tripoint_bub_ms( 65, 50, 0 ) );

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.rules.set_flag( ally_rule::follow_close );
    guy.set_mission( NPC_MISSION_NULL );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.regen_ai_cache();

    REQUIRE( rl_dist( guy.pos_abs(), pc.pos_abs() ) > 4 );

    guy.set_moves( 100 );
    guy.move();
    REQUIRE( guy.get_committed_goal() == "follow_player" );

    // Player boards a vehicle.
    vehicle *veh = here.add_vehicle( vehicle_prototype_test_rv, pc.pos_bub(),
                                     0_degrees, 100, 0 );
    REQUIRE( veh != nullptr );
    here.board_vehicle( pc.pos_bub(), &pc );
    REQUIRE( pc.in_vehicle );
    REQUIRE_FALSE( guy.in_vehicle );

    // Vehicle mismatch clears follow commitment.
    guy.set_moves( 100 );
    guy.move();
    CHECK( guy.get_committed_goal() != "follow_player" );
}

TEST_CASE( "goto_order_beats_generic_follow", "[npc][behavior]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.rules.set_flag( ally_rule::follow_close );
    guy.set_mission( NPC_MISSION_NULL );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.regen_ai_cache();

    // Player west, goto target east -- opposite directions.
    get_player_character().setpos( here, tripoint_bub_ms( 40, 50, 0 ) );
    const tripoint_abs_ms goto_target = guy.pos_abs() + tripoint( 10, 0, 0 );
    guy.goto_to_this_pos = goto_target;

    REQUIRE( rl_dist( guy.pos_abs(), get_player_character().pos_abs() )
             > guy.follow_distance() );
    REQUIRE( guy.goto_to_this_pos.has_value() );

    const int initial_dist_goto = rl_dist( guy.pos_abs(), goto_target );

    guy.set_moves( 100 );
    guy.move();
    CHECK( guy.get_committed_goal() == "goto_ordered_position" );

    for( int i = 0; i < 2; ++i ) {
        guy.set_moves( 100 );
        guy.move();
    }

    // Moved east toward goto, not west toward player.
    CHECK( guy.pos_bub().x() > 50 );
    CHECK( rl_dist( guy.pos_abs(), goto_target ) < initial_dist_goto );
}

TEST_CASE( "follow_close_beats_fetching_item", "[npc][behavior]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.rules.set_flag( ally_rule::follow_close );
    guy.set_mission( NPC_MISSION_NULL );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.regen_ai_cache();

    // Player west, pending pickup east -- opposite directions.
    get_player_character().setpos( here, tripoint_bub_ms( 40, 50, 0 ) );
    REQUIRE( rl_dist( guy.pos_abs(), get_player_character().pos_abs() )
             > guy.follow_distance() );

    guy.fetching_item = true;
    guy.wanted_item_pos = tripoint_bub_ms( 55, 50, 0 );

    guy.set_moves( 100 );
    guy.move();

    // Follow wins over item pickup, matching old cascade ordering.
    CHECK( guy.pos_bub().x() < 50 );
}

TEST_CASE( "activity_npc_without_task_reverts_to_follower", "[npc][behavior]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, -2 ) );

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_ACTIVITY );
    guy.set_mission( NPC_MISSION_NULL );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    guy.assigned_camp = std::nullopt;
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.regen_ai_cache();

    REQUIRE( guy.get_attitude() == NPCATT_ACTIVITY );
    REQUIRE_FALSE( guy.activity );

    guy.set_moves( 100 );
    guy.move();

    // Cascade revert: ally without camp reverts to FOLLOW.
    CHECK( guy.get_attitude() == NPCATT_FOLLOW );
    CHECK( guy.get_committed_goal().empty() );
}

TEST_CASE( "stationary_npc_bt_idle_pauses", "[npc][behavior]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, -2 ) );

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_attitude( NPCATT_NULL );
    guy.set_mission( NPC_MISSION_SHELTER );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    guy.goal = guy.pos_abs_omt();
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.regen_ai_cache();

    REQUIRE( guy.is_stationary( true ) );
    const tripoint_abs_ms start_pos = guy.pos_abs();

    guy.set_moves( 100 );
    guy.move();

    CHECK( guy.get_committed_goal().empty() );
    CHECK( guy.pos_abs() == start_pos );
}

TEST_CASE( "leader_npc_leads_not_follows", "[npc][behavior]" )
{
    clear_map();
    clear_avatar();
    map &here = get_map();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_LEAD );
    guy.rules.set_flag( ally_rule::follow_close );
    guy.set_mission( NPC_MISSION_NULL );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );

    // Player 5 tiles west. Destination east.
    get_player_character().setpos( here, tripoint_bub_ms( 45, 50, 0 ) );
    guy.goal = project_to<coords::omt>( guy.pos_abs() + tripoint( 10 * SEEX, 0, 0 ) );
    guy.regen_ai_cache();

    REQUIRE( guy.is_leader() );
    REQUIRE( guy.is_player_ally() );
    REQUIRE( guy.has_omt_destination() );
    REQUIRE( rl_dist( guy.pos_abs(), get_player_character().pos_abs() ) < 12 );

    for( int i = 0; i < 3; ++i ) {
        guy.set_moves( 100 );
        guy.move();
    }

    CHECK( guy.get_committed_goal() != "follow_player" );
    // Did not move toward player (west). Either moved east or stayed put.
    CHECK( guy.pos_bub().x() >= 50 );
}

TEST_CASE( "leader_npc_waits_when_player_far", "[npc][behavior]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_LEAD );
    guy.rules.set_flag( ally_rule::follow_close );
    guy.set_mission( NPC_MISSION_NULL );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );

    // Player 15 tiles west (>= 12, triggers "keep up").
    get_player_character().setpos( here, tripoint_bub_ms( 35, 50, 0 ) );
    guy.goal = project_to<coords::omt>( guy.pos_abs() + tripoint( 10 * SEEX, 0, 0 ) );
    guy.regen_ai_cache();

    REQUIRE( guy.is_leader() );
    REQUIRE( guy.is_player_ally() );
    REQUIRE( rl_dist( guy.pos_abs(), get_player_character().pos_abs() ) >= 12 );

    const tripoint_abs_ms start_pos = guy.pos_abs();
    guy.set_moves( 100 );
    guy.move();

    // Leader should pause and gain catch_up effect, NOT follow.
    CHECK( guy.pos_abs() == start_pos );
    CHECK( guy.get_committed_goal() != "follow_player" );
    CHECK( guy.has_effect( effect_catch_up ) );
}

// --- Camp resident state split tests ---

TEST_CASE( "camp_resident_is_not_guarding", "[npc][camp]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_map();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_mission( NPC_MISSION_CAMP_RESIDENT );

    SECTION( "not guarding" ) {
        CHECK_FALSE( guy.is_guarding() );
    }
    SECTION( "is player ally" ) {
        CHECK( guy.is_player_ally() );
    }
    SECTION( "not stationary" ) {
        CHECK_FALSE( guy.is_stationary( true ) );
    }
    SECTION( "describe_mission" ) {
        CHECK( guy.describe_mission().find( "guarding" ) == std::string::npos );
    }
}

TEST_CASE( "camp_resident_does_not_oscillate", "[npc][camp]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_NULL );
    guy.set_mission( NPC_MISSION_CAMP_RESIDENT );
    guy.assigned_camp = project_to<coords::omt>( guy.pos_abs() );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.regen_ai_cache();

    for( int i = 0; i < 10; ++i ) {
        guy.set_moves( 100 );
        guy.move();
        CHECK( guy.get_committed_goal() != "return_to_guard_pos" );
    }
}

TEST_CASE( "camp_resident_guard_transitions", "[npc][camp]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_mission( NPC_MISSION_CAMP_RESIDENT );
    const tripoint_abs_omt camp_pos = project_to<coords::omt>( guy.pos_abs() );
    guy.assigned_camp = camp_pos;
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();

    SECTION( "assign_guard overrides to GUARD_ALLY" ) {
        talk_function::assign_guard( guy );
        CHECK( guy.mission == NPC_MISSION_GUARD_ALLY );
        CHECK( guy.assigned_camp.has_value() );
        CHECK( guy.get_guard_post().has_value() );
    }
    SECTION( "return_to_camp_duties at camp: no travel" ) {
        talk_function::assign_guard( guy );
        talk_function::return_to_camp_duties( guy );
        CHECK( guy.mission == NPC_MISSION_CAMP_RESIDENT );
        CHECK( guy.assigned_camp == camp_pos );
        CHECK_FALSE( guy.get_guard_post().has_value() );
        CHECK( guy.goal == npc::no_goal_point );
        CHECK( guy.omt_path.empty() );
    }
    SECTION( "return_to_camp_duties away from camp: sets goal" ) {
        guy.setpos( here, tripoint_bub_ms( 50 + SEEX * 2, 50, 0 ) );
        REQUIRE( guy.pos_abs_omt() != camp_pos );
        talk_function::assign_guard( guy );
        talk_function::return_to_camp_duties( guy );
        CHECK( guy.mission == NPC_MISSION_CAMP_RESIDENT );
        CHECK( guy.goal == camp_pos );
        // omt_path may be empty if overmap pathfinding has no data,
        // but goal is set so the NPC will path on next move().
    }
    SECTION( "stop_guard recalls but preserves camp" ) {
        talk_function::assign_guard( guy );
        talk_function::stop_guard( guy );
        CHECK( guy.mission == NPC_MISSION_NULL );
        // Camp assignment preserved so player can send NPC back later.
        CHECK( guy.assigned_camp == camp_pos );
    }
    SECTION( "return_to_camp_duties clears stale movement state" ) {
        talk_function::assign_guard( guy );
        guy.chair_pos = guy.pos_abs() + tripoint( 3, 0, 0 );
        guy.wander_pos = guy.pos_abs() + tripoint( -2, 0, 0 );
        guy.path.push_back( here.get_bub( tripoint_abs_ms( 55, 50, 0 ) ) );

        talk_function::return_to_camp_duties( guy );
        CHECK_FALSE( guy.chair_pos.has_value() );
        CHECK_FALSE( guy.wander_pos.has_value() );
        CHECK( guy.path.empty() );
    }
    SECTION( "transitions clear stale committed goal" ) {
        talk_function::assign_guard( guy );
        guy.set_committed_goal( "hold_position" );
        talk_function::return_to_camp_duties( guy );
        CHECK( guy.get_committed_goal().empty() );

        talk_function::assign_guard( guy );
        CHECK( guy.get_committed_goal().empty() );
    }
}

TEST_CASE( "on_load_migrates_legacy_camp_npcs", "[npc][camp]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );

    SECTION( "GUARD_ALLY + assigned_camp migrates to CAMP_RESIDENT" ) {
        guy.set_mission( NPC_MISSION_GUARD_ALLY );
        guy.assigned_camp = project_to<coords::omt>( guy.pos_abs() );
        guy.set_guard_pos( guy.pos_abs() );
        guy.on_load( &here );
        CHECK( guy.mission == NPC_MISSION_CAMP_RESIDENT );
        CHECK_FALSE( guy.get_guard_post().has_value() );
        CHECK( guy.assigned_camp.has_value() );
    }
    SECTION( "migration clears stale movement state" ) {
        guy.set_mission( NPC_MISSION_GUARD_ALLY );
        guy.assigned_camp = project_to<coords::omt>( guy.pos_abs() );
        guy.set_guard_pos( guy.pos_abs() );
        guy.goal = project_to<coords::omt>( guy.pos_abs() + tripoint( 10 * SEEX, 0, 0 ) );
        guy.omt_path.push_back( guy.goal );
        guy.path.push_back( here.get_bub( tripoint_abs_ms( 55, 50, 0 ) ) );
        guy.set_committed_goal( "return_to_guard_pos" );
        guy.on_load( &here );
        CHECK( guy.mission == NPC_MISSION_CAMP_RESIDENT );
        CHECK( guy.goal == npc::no_goal_point );
        CHECK( guy.omt_path.empty() );
        CHECK( guy.path.empty() );
        CHECK( guy.get_committed_goal().empty() );
    }
    SECTION( "migration fixes previous_mission for active workers" ) {
        guy.set_mission( NPC_MISSION_ACTIVITY );
        guy.previous_mission = NPC_MISSION_GUARD_ALLY;
        guy.assigned_camp = project_to<coords::omt>( guy.pos_abs() );
        guy.set_guard_pos( guy.pos_abs() );
        guy.on_load( &here );
        CHECK( guy.mission == NPC_MISSION_ACTIVITY );
        CHECK( guy.previous_mission == NPC_MISSION_CAMP_RESIDENT );
    }
    SECTION( "GUARD_ALLY without assigned_camp does not migrate" ) {
        guy.set_mission( NPC_MISSION_GUARD_ALLY );
        guy.assigned_camp = std::nullopt;
        guy.set_guard_pos( guy.pos_abs() );
        guy.on_load( &here );
        CHECK( guy.mission == NPC_MISSION_GUARD_ALLY );
    }
}

// NPC infinite loop when trying to forage/harvest terrain it can't interact with.
TEST_CASE( "npc_no_infinite_loop_foraging", "[npc][needs][forage]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 10, 10, 0 } );
    get_weather().forced_temperature = 20_C;

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );

    // Extreme hunger bypasses the one_in(3) gate, deterministically tries to forage.
    guy.set_hunger( 300 );
    guy.set_thirst( 100 );
    guy.set_stored_kcal( 1000 );

    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;

    // Helper: run the NPC turn loop from do_turn.cpp, return true if it loops.
    const auto npc_loops = [&]() {
        guy.set_moves( 100 );
        int turns = 0;
        int real_count = 0;
        const int count_limit = std::max( 10, guy.get_moves() / 64 );
        while( !guy.is_dead() && !guy.in_sleep_state() &&
               guy.get_moves() > 0 && turns < 10 ) {
            const int moves = guy.get_moves();
            guy.move();
            if( moves == guy.get_moves() ) {
                real_count++;
                if( real_count > count_limit ) {
                    turns++;
                }
            }
        }
        return turns >= 10;
    };

    SECTION( "underbrush with items on it" ) {
        set_time_to_day();
        here.ter_set( adj, ter_t_underbrush );
        here.add_item_or_charges( adj, item( itype_2x4 ) );
        here.build_map_cache( 0 );

        CHECK_FALSE( npc_loops() );
        CHECK_FALSE( guy.has_effect( effect_npc_suspend ) );
    }

    SECTION( "harvestable tree - query_pick rejects non-avatar" ) {
        // Autumn so the apple tree has a harvest entry.
        calendar::turn = calendar::turn_zero + 2 * calendar::season_length() + 12_hours;
        here.ter_set( adj, ter_t_tree_apple );
        here.build_map_cache( 0 );
        REQUIRE( here.is_harvestable( adj ) );

        CHECK_FALSE( npc_loops() );
        CHECK_FALSE( guy.has_effect( effect_npc_suspend ) );
    }

    SECTION( "forage activity completes without backlog flooding" ) {
        set_time_to_day();
        here.ter_set( adj, ter_t_underbrush );
        here.build_map_cache( 0 );

        // Run several game turns. Each turn the BT may try to override the
        // forage activity with follow_player. The activity should complete
        // without endlessly re-assigning and flooding the backlog.
        for( int turn = 0; turn < 20; ++turn ) {
            guy.set_moves( 100 );
            guy.move();
        }
        CHECK( guy.backlog.size() < 10 );
    }
}

// BT should commit to eat_food when the NPC has a caloric deficit and food
// is obtainable nearby, even when short-term hunger is low.
TEST_CASE( "npc_bt_commits_to_food_when_starving", "[npc][needs][forage]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 10, 10, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );

    // Follower NPC so the BT considers follow_player.
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    REQUIRE( guy.is_player_ally() );

    // Caloric deficit but NOT short-term hungry (reproduces the save scenario).
    guy.set_stored_kcal( 1000 );
    guy.set_hunger( -1 );
    guy.set_thirst( 0 );

    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;
    here.ter_set( adj, ter_t_underbrush );
    here.build_map_cache( 0 );

    // Run several turns. The NPC should commit to eating (forage) and not
    // oscillate between eat_food and follow_player every other turn.
    int forage_turns = 0;
    int follow_turns = 0;
    for( int turn = 0; turn < 10; ++turn ) {
        guy.set_moves( 100 );
        guy.move();
        if( guy.activity.id() == ACT_FORAGE ) {
            forage_turns++;
        }
        const std::string &goal = guy.get_committed_goal();
        if( goal == "follow_player" ) {
            follow_turns++;
        }
    }

    // NPC should spend most turns foraging, not following.
    CHECK( forage_turns > follow_turns );
}

// When an NPC is committed to eat_food but all nearby food sources are gone,
// the commitment should clear so other goals (follow, goto) can proceed.
TEST_CASE( "npc_eat_food_commitment_clears_when_unobtainable", "[npc][needs][forage]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 10, 10, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );

    // Caloric deficit but no food anywhere.
    guy.set_stored_kcal( 1000 );
    guy.set_hunger( -1 );
    guy.set_thirst( 0 );

    map &here = get_map();
    here.build_map_cache( 0 );

    // Force commitment to eat_food.
    guy.set_committed_goal( "eat_food" );

    // Run a few turns. With no food obtainable, the commitment should clear
    // and the NPC should fall through to another goal (follow, etc.).
    for( int turn = 0; turn < 5; ++turn ) {
        guy.set_moves( 100 );
        guy.move();
    }
    CHECK( guy.get_committed_goal() != "eat_food" );
}

// Executor-level tests for eat_food goal: sticky targets, progress tracking,
// retargeting, and interaction with follow_player.
TEST_CASE( "npc_food_executor_contract", "[npc][needs][forage]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 55, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    // Low enough for calorie deficit and needs_food_badly, high enough to
    // survive the test without starving to death.
    guy.set_stored_kcal( 20000 );
    guy.set_hunger( -1 );
    guy.set_thirst( 0 );

    map &here = get_map();

    SECTION( "two equal targets: NPC pursues one, not alternating" ) {
        const tripoint_bub_ms east = guy.pos_bub() + tripoint( 3, 0, 0 );
        const tripoint_bub_ms west = guy.pos_bub() + tripoint( -3, 0, 0 );
        here.ter_set( east, ter_t_underbrush );
        here.ter_set( west, ter_t_underbrush );
        here.build_map_cache( 0 );
        guy.set_committed_goal( "eat_food" );

        int east_moves = 0;
        int west_moves = 0;
        tripoint_bub_ms prev = guy.pos_bub();
        for( int turn = 0; turn < 6; ++turn ) {
            guy.set_moves( 100 );
            guy.move();
            const tripoint_bub_ms cur = guy.pos_bub();
            if( cur.x() > prev.x() ) {
                east_moves++;
            } else if( cur.x() < prev.x() ) {
                west_moves++;
            }
            prev = cur;
        }
        // NPC should consistently move in one direction, not both.
        CHECK( ( east_moves == 0 || west_moves == 0 ) );
        // And should have actually moved toward a target.
        CHECK( ( east_moves + west_moves ) > 0 );
    }

    SECTION( "zero progress for N turns clears or retargets" ) {
        // Surround an apple tree with walls so it's visible but unreachable.
        calendar::turn = calendar::turn_zero + 2 * calendar::season_length() + 12_hours;
        const tripoint_bub_ms tree = guy.pos_bub() + tripoint( 2, 0, 0 );
        for( const point &d : {
                 point::north, point::south, point::east, point::west,
                 point::north_west, point::north_east, point::south_west, point::south_east, point::zero
             } ) {
            if( d != point::zero ) {
                here.ter_set( tree + d, ter_t_concrete_wall );
            }
        }
        here.ter_set( tree, ter_t_tree_apple );
        here.build_map_cache( 0 );
        REQUIRE( here.is_harvestable( tree ) );

        guy.set_committed_goal( "eat_food" );
        for( int turn = 0; turn < 15; ++turn ) {
            guy.set_moves( 100 );
            guy.move();
        }
        CHECK( guy.get_committed_goal() != "eat_food" );
    }

    SECTION( "target becomes invalid mid-plan triggers retarget" ) {
        const tripoint_bub_ms bush = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( bush, ter_t_underbrush );
        here.build_map_cache( 0 );
        guy.set_committed_goal( "eat_food" );

        // Let NPC start pursuing but not reach the bush yet.
        for( int turn = 0; turn < 2; ++turn ) {
            guy.set_moves( 100 );
            guy.move();
        }
        REQUIRE_FALSE( guy.activity );

        // Remove the target mid-plan.
        here.ter_set( bush, ter_t_floor );
        here.build_map_cache( 0 );

        // Place a new target within scan range of the NPC's current position,
        // away from the player path (player is south at y=55).
        const tripoint_bub_ms bush2 = guy.pos_bub() + tripoint( 0, -3, 0 );
        here.ter_set( bush2, ter_t_underbrush );
        here.build_map_cache( 0 );

        // NPC should retarget toward bush2 and eventually forage it.
        for( int turn = 0; turn < 10; ++turn ) {
            guy.set_moves( 100 );
            guy.move();
        }
        CHECK( rl_dist( guy.pos_bub(), bush2 ) <= 1 );
        CHECK( ( guy.get_committed_goal() == "eat_food" ||
                 guy.activity.id() == ACT_FORAGE ) );
    }

    SECTION( "follow does not regress distance to food target" ) {
        const tripoint_bub_ms bush = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( bush, ter_t_underbrush );
        here.build_map_cache( 0 );
        guy.set_committed_goal( "eat_food" );

        // Net progress: no turn should increase distance to food,
        // and at least one turn should decrease it.
        int progress_turns = 0;
        int regress_turns = 0;
        int prev_dist = rl_dist( guy.pos_bub(), bush );
        for( int turn = 0; turn < 5; ++turn ) {
            guy.set_moves( 100 );
            guy.move();
            const int cur_dist = rl_dist( guy.pos_bub(), bush );
            if( cur_dist < prev_dist ) {
                progress_turns++;
            } else if( cur_dist > prev_dist ) {
                regress_turns++;
            }
            prev_dist = cur_dist;
        }
        CHECK( regress_turns == 0 );
        CHECK( progress_turns > 0 );
    }

    SECTION( "closer food mid-approach does not dither the plan" ) {
        // NPC starts pursuing a distant bush.
        const tripoint_bub_ms far_bush = guy.pos_bub() + tripoint( 5, 0, 0 );
        here.ter_set( far_bush, ter_t_underbrush );
        here.build_map_cache( 0 );
        guy.set_committed_goal( "eat_food" );

        // Let NPC start moving east toward far_bush.
        for( int turn = 0; turn < 2; ++turn ) {
            guy.set_moves( 100 );
            guy.move();
        }
        REQUIRE( guy.pos_bub().x() > 50 );

        // Drop a closer bush to the north (off the current path).
        const tripoint_bub_ms near_bush = guy.pos_bub() + tripoint( 0, -2, 0 );
        here.ter_set( near_bush, ter_t_underbrush );
        here.build_map_cache( 0 );

        // NPC should keep pursuing the original target, not switch.
        tripoint_bub_ms prev = guy.pos_bub();
        int east_moves = 0;
        int north_moves = 0;
        for( int turn = 0; turn < 4; ++turn ) {
            guy.set_moves( 100 );
            guy.move();
            const tripoint_bub_ms cur = guy.pos_bub();
            if( cur.x() > prev.x() ) {
                east_moves++;
            }
            if( cur.y() < prev.y() ) {
                north_moves++;
            }
            prev = cur;
        }
        // Should keep going east toward original target, not veer north.
        CHECK( east_moves > north_moves );
    }

    SECTION( "eat_food executor skips pure-drink items" ) {
        // NPC is thirsty, so water would normally score in the legacy path.
        guy.set_thirst( 100 );

        // Water in inventory and on adjacent ground.
        guy.i_add( item( itype_water_clean ) );
        const tripoint_bub_ms adj = guy.pos_bub() + point::east;
        here.add_item( adj, item( itype_water_clean ) );
        here.build_map_cache( 0 );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "eat_food" );
        // Pure drinks don't satisfy eat_food; no food exists, so impossible.
        CHECK( result == npc::need_result::impossible );
    }

    SECTION( "danger gate defers plan without killing it" ) {
        const tripoint_bub_ms bush = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( bush, ter_t_underbrush );
        here.build_map_cache( 0 );

        // Low danger: executor acquires target and moves toward it.
        guy.set_ai_danger( 0 );
        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "eat_food" );
        REQUIRE( result == npc::need_result::progressed );
        REQUIRE( guy.get_food_plan().active() );

        // High danger: movement is deferred, plan stays alive.
        guy.set_ai_danger( NPC_DANGER_VERY_LOW + 1 );
        guy.set_moves( 100 );
        result = guy.execute_need_goal( "eat_food" );
        CHECK( result == npc::need_result::deferred );
        CHECK( guy.get_food_plan().active() );

        // Low danger again: executor resumes progress.
        guy.set_ai_danger( 0 );
        guy.set_moves( 100 );
        result = guy.execute_need_goal( "eat_food" );
        CHECK( result == npc::need_result::progressed );
    }

    SECTION( "harvestable plan invalidated when terrain loses food yields" ) {
        calendar::turn = calendar::turn_zero + 2 * calendar::season_length() + 12_hours;
        const tripoint_bub_ms tree_pos = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( tree_pos, ter_t_tree_apple );
        here.build_map_cache( 0 );
        REQUIRE( here.is_harvestable( tree_pos ) );

        // Executor acquires a harvestable food target.
        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "eat_food" );
        REQUIRE( result == npc::need_result::progressed );
        REQUIRE( guy.get_food_plan().active() );
        using need_source = npc_short_term_cache::need_source;
        REQUIRE( guy.get_food_plan().source_kind == need_source::harvestable );

        // Change terrain to non-food harvestable (dead tree = sticks only).
        here.ter_set( tree_pos, ter_t_tree_dead );
        here.build_map_cache( 0 );
        REQUIRE( here.is_harvestable( tree_pos ) );

        // Executor should invalidate the stale plan; no other food, so impossible.
        guy.set_moves( 100 );
        result = guy.execute_need_goal( "eat_food" );
        CHECK( result == npc::need_result::impossible );
    }

    SECTION( "closer unreachable target skipped for farther reachable one" ) {
        calendar::turn = calendar::turn_zero + 2 * calendar::season_length() + 12_hours;
        using need_source = npc_short_term_cache::need_source;

        // Close tree behind glass (visible through glass, but impassable
        // without bashing, and executor should not bash for normal food).
        const tripoint_bub_ms close = guy.pos_bub() + tripoint( 2, 0, 0 );
        for( const point &d : {
                 point::north, point::south, point::east, point::west,
                 point::north_west, point::north_east, point::south_west, point::south_east
             } ) {
            here.ter_set( close + d, ter_t_wall_glass );
        }
        here.ter_set( close, ter_t_tree_apple );

        // Far tree in the open (reachable).
        const tripoint_bub_ms far = guy.pos_bub() + tripoint( 0, -5, 0 );
        here.ter_set( far, ter_t_tree_apple );
        here.build_map_cache( 0 );
        REQUIRE( here.is_harvestable( close ) );
        REQUIRE( here.is_harvestable( far ) );

        // The close tree must be the first candidate (closer = higher score).
        auto cands = guy.find_food_candidates();
        REQUIRE( cands.size() >= 2 );
        REQUIRE( cands.front().target == here.get_abs( close ) );

        // Drive the executor directly. The close tree should produce
        // no progress (no-bash policy for non-critical food) and time out.
        for( int turn = 0; turn < 5; ++turn ) {
            guy.set_moves( 100 );
            guy.execute_need_goal( "eat_food" );
        }
        REQUIRE( guy.get_food_plan().last_result == npc::need_result::impossible );

        // Next call should skip the failed close tree and target the far one.
        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "eat_food" );
        CHECK( guy.get_food_plan().active() );
        CHECK( guy.get_food_plan().target == here.get_abs( far ) );
        CHECK( guy.get_food_plan().source_kind == need_source::harvestable );
        CHECK( result == npc::need_result::progressed );
    }

    SECTION( "failed target becomes reachable after generation reset" ) {
        calendar::turn = calendar::turn_zero + 2 * calendar::season_length() + 12_hours;

        // Single tree behind glass (visible, unreachable without bashing).
        const tripoint_bub_ms tree = guy.pos_bub() + tripoint( 2, 0, 0 );
        for( const point &d : {
                 point::north, point::south, point::east, point::west,
                 point::north_west, point::north_east, point::south_west, point::south_east
             } ) {
            here.ter_set( tree + d, ter_t_wall_glass );
        }
        here.ter_set( tree, ter_t_tree_apple );
        here.build_map_cache( 0 );
        REQUIRE( here.is_harvestable( tree ) );

        // Exhaust the target: 5 turns of no-progress, then impossible.
        for( int turn = 0; turn < 5; ++turn ) {
            guy.set_moves( 100 );
            guy.execute_need_goal( "eat_food" );
        }
        REQUIRE( guy.get_food_plan().last_result == npc::need_result::impossible );

        // Next call: only candidate is in the failed set, so the
        // generation resets and returns impossible with a clean slate.
        guy.set_moves( 100 );
        guy.execute_need_goal( "eat_food" );
        REQUIRE( guy.get_food_plan().last_result == npc::need_result::impossible );

        // Remove the glass walls so the tree becomes reachable.
        for( const point &d : {
                 point::north, point::south, point::east, point::west,
                 point::north_west, point::north_east, point::south_west, point::south_east
             } ) {
            here.ter_set( tree + d, ter_t_floor );
        }
        here.build_map_cache( 0 );

        // The executor should now acquire the formerly-failed tree
        // and make progress toward it.
        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "eat_food" );
        CHECK( guy.get_food_plan().active() );
        CHECK( guy.get_food_plan().target == here.get_abs( tree ) );
        CHECK( result == npc::need_result::progressed );
    }

    SECTION( "pickup disabled ally eats adjacent ground food" ) {
        guy.rules.clear_flag( ally_rule::allow_pick_up );

        const tripoint_bub_ms adj = guy.pos_bub() + point::east;
        here.add_item_or_charges( adj, item( itype_sandwich_cheese_grilled ) );
        here.build_map_cache( 0 );

        const int kcal_before = guy.get_stored_kcal();
        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "eat_food" );
        // The NPC should eat the adjacent food, not return impossible.
        CHECK( result == npc::need_result::satisfied );
        CHECK( guy.get_stored_kcal() + guy.stomach.get_calories() > kcal_before );
    }
}

// Starving NPCs should eat foraged food immediately instead of dropping it.
TEST_CASE( "npc_eats_foraged_food_immediately", "[npc][needs][forage]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 52, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.rules.clear_flag( ally_rule::allow_pick_up );
    guy.set_stored_kcal( 1000 );
    guy.set_hunger( -1 );
    guy.set_thirst( 0 );
    REQUIRE( guy.has_calorie_deficit() );

    // Apple tree in autumn: always drops 2-5 apples, no RNG gating.
    calendar::turn = calendar::turn_zero + 2 * calendar::season_length() + 12_hours;
    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;
    here.ter_set( adj, ter_t_tree_apple );
    here.build_map_cache( 0 );
    REQUIRE( here.is_harvestable( adj ) );
    REQUIRE_FALSE( guy.find_nearby_harvestable( true ).empty() );

    // Run enough turns for the harvest activity to complete.
    const int kcal_before = guy.get_stored_kcal();
    for( int turn = 0; turn < 40; ++turn ) {
        guy.set_moves( 100 );
        guy.move();
    }

    // The NPC should have consumed at least some harvested food.
    CHECK( guy.get_stored_kcal() + guy.stomach.get_calories() > kcal_before );
}

// handle_harvest auto-eating must charge consume time, not eat for free.
TEST_CASE( "npc_harvest_consume_charges_moves", "[npc][needs][forage]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 52, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 1000 );
    guy.set_hunger( 300 );
    guy.set_thirst( 0 );
    REQUIRE( guy.has_calorie_deficit() );

    // Auto-eating only happens during self-care need execution.
    guy.set_committed_goal( "eat_food" );
    guy.set_moves( 200 );
    const int moves_before = guy.get_moves();
    iexamine_helper::handle_harvest( guy, itype_sandwich_cheese_grilled, false );

    // Moves must decrease (consume time charged).
    CHECK( guy.get_moves() < moves_before );
}

// can_obtain_food should distinguish food-yielding terrain from non-food.
TEST_CASE( "npc_can_obtain_food_filters_harvests", "[npc][needs][forage]" )
{
    clear_map_without_vision();
    set_time_to_day();
    calendar::turn = calendar::turn_zero + 2 * calendar::season_length() + 12_hours;

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_stored_kcal( 1000 );

    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;
    behavior::character_oracle_t oracle( &guy );

    SECTION( "underbrush yields seasonal food" ) {
        here.ter_set( adj, ter_t_underbrush );
        here.build_map_cache( 0 );
        CHECK( oracle.can_obtain_food( "" ) == behavior::status_t::running );
    }

    SECTION( "apple tree in autumn yields food" ) {
        here.ter_set( adj, ter_t_tree_apple );
        here.build_map_cache( 0 );
        REQUIRE( here.is_harvestable( adj ) );
        CHECK( oracle.can_obtain_food( "" ) == behavior::status_t::running );
    }

    SECTION( "walnut tree in autumn yields food" ) {
        here.ter_set( adj, ter_t_tree_walnut );
        here.build_map_cache( 0 );
        REQUIRE( here.is_harvestable( adj ) );
        CHECK( oracle.can_obtain_food( "" ) == behavior::status_t::running );
    }

    SECTION( "dead tree (sticks only) is not food" ) {
        here.ter_set( adj, ter_t_tree_dead );
        here.build_map_cache( 0 );
        REQUIRE( here.is_harvestable( adj ) );
        CHECK( oracle.can_obtain_food( "" ) == behavior::status_t::failure );
    }

    SECTION( "birch tree (bark only) is not food" ) {
        here.ter_set( adj, ter_t_tree_birch );
        here.build_map_cache( 0 );
        REQUIRE( here.is_harvestable( adj ) );
        CHECK( oracle.can_obtain_food( "" ) == behavior::status_t::failure );
    }

    SECTION( "pine tree (boughs/resin) is not food" ) {
        here.ter_set( adj, ter_t_tree_pine );
        here.build_map_cache( 0 );
        REQUIRE( here.is_harvestable( adj ) );
        CHECK( oracle.can_obtain_food( "" ) == behavior::status_t::failure );
    }

    SECTION( "no harvestable terrain nearby" ) {
        here.build_map_cache( 0 );
        CHECK( oracle.can_obtain_food( "" ) == behavior::status_t::failure );
    }
}

// can_obtain_food should recognize camp food the same way can_obtain_water
// recognizes camp water, so the BT commits eat_food when camp food is the
// only available source.
TEST_CASE( "npc_can_obtain_food_camp_fallback", "[npc][needs][forage][camp]" )
{
    clear_avatar();
    clear_map_without_vision();
    get_player_character().camps.clear();
    map &m = get_map();

    const tripoint_bub_ms mid{ MAPSIZE_X / 2, MAPSIZE_Y / 2, 0 };
    const tripoint_abs_ms zone_loc = m.get_abs( mid );
    REQUIRE( m.inbounds( zone_loc ) );
    mapgen_place_zone( zone_loc, zone_loc, zone_type_CAMP_FOOD, your_fac, {}, "food" );
    mapgen_place_zone( zone_loc, zone_loc, zone_type_CAMP_STORAGE, your_fac, {}, "storage" );
    faction *camp_faction = get_player_character().get_faction();
    const tripoint_abs_omt this_omt = project_to<coords::omt>( zone_loc );
    m.add_camp( this_omt, "faction_camp" );
    std::optional<basecamp *> bcp = overmap_buffer.find_camp( this_omt.xy() );
    REQUIRE( !!bcp );
    basecamp *test_camp = *bcp;
    test_camp->define_camp( this_omt, "faction_base_bare_bones_NPC_camp_0", false );
    test_camp->set_owner( your_fac );

    // Stock the camp larder.
    camp_faction->empty_food_supply();
    camp_faction->debug_food_supply().emplace_back( calendar::turn_zero, nutrients{} );
    camp_faction->debug_food_supply().back().second.calories = 500 * 1000;
    REQUIRE( camp_faction->food_supply().kcal() >= 500 );

    npc &guy = spawn_npc( mid.xy(), "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_stored_kcal( 1000 );
    guy.set_hunger( -1 );
    guy.set_thirst( 0 );
    m.build_map_cache( 0 );

    // Camp food is now in the candidate layer, so candidates are non-empty.
    auto cands = guy.find_food_candidates();
    REQUIRE_FALSE( cands.empty() );
    bool has_camp_source = false;
    for( const npc_short_term_cache::need_candidate &c : cands ) {
        if( c.source_kind == npc::need_source::camp_food ) {
            has_camp_source = true;
        }
    }
    REQUIRE( has_camp_source );

    SECTION( "can_obtain_food returns running via unified candidate layer" ) {
        behavior::character_oracle_t oracle( &guy );
        CHECK( oracle.can_obtain_food( "" ) == behavior::status_t::running );
    }

    SECTION( "BT commits eat_food when camp food is the only source" ) {
        get_weather().forced_temperature = 20_C;
        set_time_to_day();
        get_player_character().setpos( get_map(), mid );
        guy.set_attitude( NPCATT_FOLLOW );
        // stored_kcal=1000 triggers needs_food_badly in the BT.
        REQUIRE( guy.has_calorie_deficit() );

        // Let the BT pick a goal naturally.
        guy.set_moves( 100 );
        guy.move();
        CHECK( guy.get_committed_goal() == "eat_food" );
    }

    SECTION( "execute_eat_food succeeds from camp food" ) {
        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "eat_food" );
        CHECK( result == npc::need_result::satisfied );
        CHECK( camp_faction->food_supply().kcal() < 500 );
    }
}

// Candidate query layer tests: verify find_food_candidates / find_water_candidates
// match the policy checks that predicates and executors share.
TEST_CASE( "npc_find_food_candidates", "[npc][needs][forage]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();
    calendar::turn = calendar::turn_zero + 2 * calendar::season_length() + 12_hours;

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_hunger( 300 );
    guy.set_thirst( 100 );
    guy.set_stored_kcal( 1000 );

    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;
    REQUIRE_FALSE( guy.is_player_ally() );

    using need_source = npc_short_term_cache::need_source;

    SECTION( "ground food item yields ground_item candidate" ) {
        here.add_item_or_charges( adj, item( itype_sandwich_cheese_grilled ) );
        here.build_map_cache( 0 );
        auto cands = guy.find_food_candidates();
        REQUIRE_FALSE( cands.empty() );
        CHECK( cands.front().source_kind == need_source::ground_item );
        CHECK( cands.front().target == here.get_abs( adj ) );
    }

    SECTION( "harvestable terrain yields harvestable candidate" ) {
        here.ter_set( adj, ter_t_underbrush );
        here.build_map_cache( 0 );
        auto cands = guy.find_food_candidates();
        REQUIRE_FALSE( cands.empty() );
        CHECK( cands.front().source_kind == need_source::harvestable );
    }

    SECTION( "non-food harvestable excluded" ) {
        here.ter_set( adj, ter_t_tree_dead );
        here.build_map_cache( 0 );
        REQUIRE( here.is_harvestable( adj ) );
        CHECK( guy.find_food_candidates().empty() );
    }

    SECTION( "drink-only item excluded (no calories)" ) {
        item water_item( itype_water_clean );
        REQUIRE( water_item.is_food() );
        REQUIRE( water_item.get_comestible() );
        REQUIRE_FALSE( water_item.get_comestible()->has_calories() );
        here.add_item_or_charges( adj, water_item );
        here.build_map_cache( 0 );
        CHECK( guy.find_food_candidates().empty() );
    }

    SECTION( "ally without allow_pick_up still finds ground items for consumption" ) {
        guy.set_fac( faction_your_followers );
        guy.set_attitude( NPCATT_FOLLOW );
        REQUIRE( guy.is_player_ally() );
        guy.rules.clear_flag( ally_rule::allow_pick_up );
        here.add_item_or_charges( adj, item( itype_sandwich_cheese_grilled ) );
        here.build_map_cache( 0 );
        auto cands = guy.find_food_candidates();
        REQUIRE_FALSE( cands.empty() );
        CHECK( cands.front().source_kind == need_source::ground_item );
    }

    SECTION( "ally without allow_pick_up still finds harvestable" ) {
        guy.set_fac( faction_your_followers );
        guy.set_attitude( NPCATT_FOLLOW );
        REQUIRE( guy.is_player_ally() );
        guy.rules.clear_flag( ally_rule::allow_pick_up );
        here.ter_set( adj, ter_t_underbrush );
        here.build_map_cache( 0 );
        auto cands = guy.find_food_candidates();
        REQUIRE_FALSE( cands.empty() );
        CHECK( cands.front().source_kind == need_source::harvestable );
    }

    SECTION( "NO_NPC_PICKUP zone blocks ground items" ) {
        guy.set_fac( faction_your_followers );
        guy.set_attitude( NPCATT_FOLLOW );
        REQUIRE( guy.is_player_ally() );
        const tripoint_abs_ms abs_adj = here.get_abs( adj );
        mapgen_place_zone( abs_adj, abs_adj, zone_type_NO_NPC_PICKUP,
                           faction_your_followers, {}, "no_pickup" );
        here.add_item_or_charges( adj, item( itype_sandwich_cheese_grilled ) );
        here.build_map_cache( 0 );
        CHECK( guy.find_food_candidates().empty() );
    }

    SECTION( "can_obtain_food predicate sees ground food" ) {
        here.add_item_or_charges( adj, item( itype_sandwich_cheese_grilled ) );
        here.build_map_cache( 0 );
        behavior::character_oracle_t oracle( &guy );
        CHECK( oracle.can_obtain_food( "" ) == behavior::status_t::running );
    }

    SECTION( "empty map yields no candidates" ) {
        here.build_map_cache( 0 );
        CHECK( guy.find_food_candidates().empty() );
    }
}

TEST_CASE( "npc_find_water_candidates", "[npc][needs][water]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_hunger( 100 );
    guy.set_thirst( 200 );
    guy.set_stored_kcal( 5000 );

    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;
    REQUIRE_FALSE( guy.is_player_ally() );

    using need_source = npc_short_term_cache::need_source;

    SECTION( "water terrain yields water_terrain candidate" ) {
        here.ter_set( adj, ter_t_water_sh );
        here.build_map_cache( 0 );
        auto cands = guy.find_water_candidates();
        REQUIRE_FALSE( cands.empty() );
        CHECK( cands.front().source_kind == need_source::water_terrain );
        CHECK( cands.front().target == here.get_abs( adj ) );
    }

    SECTION( "ground drink item yields ground_item candidate" ) {
        // Orange has quench > 0 and is a solid item that persists on
        // bare ground. Liquids (water_clean) may not, so use a food
        // item that also hydrates.
        here.add_item_or_charges( adj, item( itype_orange ) );
        here.build_map_cache( 0 );
        auto cands = guy.find_water_candidates();
        REQUIRE_FALSE( cands.empty() );
        CHECK( cands.front().source_kind == need_source::ground_item );
    }

    SECTION( "salt water excluded" ) {
        here.ter_set( adj, ter_t_swater_sh );
        here.build_map_cache( 0 );
        CHECK( guy.find_water_candidates().empty() );
    }

    SECTION( "food with no quench excluded" ) {
        item food( itype_crackers );
        REQUIRE( food.is_food() );
        REQUIRE( food.get_comestible()->quench <= 0 );
        here.add_item_or_charges( adj, food );
        here.build_map_cache( 0 );
        CHECK( guy.find_water_candidates().empty() );
    }

    SECTION( "ally without allow_pick_up finds both ground drinks and water terrain" ) {
        guy.set_fac( faction_your_followers );
        guy.set_attitude( NPCATT_FOLLOW );
        REQUIRE( guy.is_player_ally() );
        guy.rules.clear_flag( ally_rule::allow_pick_up );
        here.add_item_or_charges( adj, item( itype_orange ) );
        tripoint_bub_ms adj2 = guy.pos_bub() + point::west;
        here.ter_set( adj2, ter_t_water_sh );
        here.build_map_cache( 0 );
        auto cands = guy.find_water_candidates();
        REQUIRE_FALSE( cands.empty() );
        // Should find both ground drinks and water terrain now.
        bool has_ground = false;
        bool has_terrain = false;
        for( const npc_short_term_cache::need_candidate &c : cands ) {
            if( c.source_kind == need_source::ground_item ) {
                has_ground = true;
            } else if( c.source_kind == need_source::water_terrain ) {
                has_terrain = true;
            }
        }
        CHECK( has_ground );
        CHECK( has_terrain );
    }

    SECTION( "can_obtain_water predicate sees ground drink items" ) {
        here.add_item_or_charges( adj, item( itype_orange ) );
        here.build_map_cache( 0 );
        behavior::character_oracle_t oracle( &guy );
        CHECK( oracle.can_obtain_water( "" ) == behavior::status_t::running );
    }

    SECTION( "empty map yields no candidates" ) {
        here.build_map_cache( 0 );
        CHECK( guy.find_water_candidates().empty() );
    }
}

// Executor-level tests for drink_water goal: water terrain sources,
// inventory drinks, drink-only filtering, sticky targets, progress tracking.
TEST_CASE( "npc_water_executor_contract", "[npc][needs][water]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 55, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 55000 );
    guy.set_hunger( 0 );
    guy.set_thirst( 200 );

    map &here = get_map();

    SECTION( "walks to water terrain and drinks" ) {
        const tripoint_bub_ms water = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( water, ter_t_water_sh );
        here.build_map_cache( 0 );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "drink_water" );
        REQUIRE( result == npc::need_result::progressed );
        REQUIRE( guy.get_water_plan().active() );
        using need_source = npc_short_term_cache::need_source;
        CHECK( guy.get_water_plan().source_kind == need_source::water_terrain );
    }

    SECTION( "drinks adjacent water terrain immediately" ) {
        const tripoint_bub_ms water = guy.pos_bub() + point::east;
        here.ter_set( water, ter_t_water_sh );
        here.build_map_cache( 0 );

        const int thirst_before = guy.get_thirst();
        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "drink_water" );
        CHECK( result == npc::need_result::satisfied );
        CHECK( guy.get_thirst() < thirst_before );
    }

    SECTION( "drink_water executor skips pure-food items" ) {
        // Crackers in inventory (nutrition only, negative quench).
        guy.i_add( item( itype_crackers ) );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "drink_water" );
        // No water source exists; crackers don't count.
        CHECK( result == npc::need_result::impossible );
    }

    SECTION( "no water anywhere returns impossible" ) {
        here.build_map_cache( 0 );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "drink_water" );
        CHECK( result == npc::need_result::impossible );
    }

    SECTION( "water plan sticks to original target" ) {
        const tripoint_bub_ms east_water = guy.pos_bub() + tripoint( 4, 0, 0 );
        here.ter_set( east_water, ter_t_water_sh );
        here.build_map_cache( 0 );

        // Acquire target.
        guy.set_moves( 100 );
        guy.execute_need_goal( "drink_water" );
        REQUIRE( guy.get_water_plan().active() );

        // Add closer water to the north.
        const tripoint_bub_ms north_water = guy.pos_bub() + tripoint( 0, -2, 0 );
        here.ter_set( north_water, ter_t_water_sh );
        here.build_map_cache( 0 );

        // NPC should keep pursuing original target.
        tripoint_bub_ms prev = guy.pos_bub();
        int east_moves = 0;
        int north_moves = 0;
        for( int turn = 0; turn < 3; ++turn ) {
            guy.set_moves( 100 );
            guy.execute_need_goal( "drink_water" );
            const tripoint_bub_ms cur = guy.pos_bub();
            if( cur.x() > prev.x() ) {
                east_moves++;
            }
            if( cur.y() < prev.y() ) {
                north_moves++;
            }
            prev = cur;
        }
        CHECK( east_moves > north_moves );
    }

    SECTION( "danger gate defers without killing plan" ) {
        const tripoint_bub_ms water = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( water, ter_t_water_sh );
        here.build_map_cache( 0 );

        // Low danger: acquire target.
        guy.set_ai_danger( 0 );
        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "drink_water" );
        REQUIRE( result == npc::need_result::progressed );

        // High danger: deferred.
        guy.set_ai_danger( NPC_DANGER_VERY_LOW + 1 );
        guy.set_moves( 100 );
        result = guy.execute_need_goal( "drink_water" );
        CHECK( result == npc::need_result::deferred );
        CHECK( guy.get_water_plan().active() );

        // Low danger: resumes.
        guy.set_ai_danger( 0 );
        guy.set_moves( 100 );
        result = guy.execute_need_goal( "drink_water" );
        CHECK( result == npc::need_result::progressed );
    }
}

// Candidate query layer tests for warmth: verify find_warmth_candidates
// returns the right mix of ground clothing and shelter candidates.
TEST_CASE( "npc_find_warmth_candidates", "[npc][needs][warmth]" )
{
    clear_map_without_vision();
    clear_avatar();
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );

    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;
    using need_source = npc_short_term_cache::need_source;

    SECTION( "ground clothing yields ground_clothing candidate" ) {
        here.add_item_or_charges( adj, item( itype_sweater ) );
        here.build_map_cache( 0 );
        auto cands = guy.find_warmth_candidates();
        REQUIRE_FALSE( cands.empty() );
        CHECK( cands.back().source_kind == need_source::ground_clothing );
    }

    SECTION( "indoor tile yields shelter candidate" ) {
        here.ter_set( adj, ter_t_floor );
        here.build_map_cache( 0 );
        auto cands = guy.find_warmth_candidates();
        REQUIRE_FALSE( cands.empty() );
        CHECK( cands.front().source_kind == need_source::shelter );
    }

    SECTION( "nearby clothing outranks distant shelter" ) {
        // Clothing at distance 1 (warmth ~30), shelter at distance 3 (score -3).
        here.add_item_or_charges( adj, item( itype_sweater ) );
        tripoint_bub_ms far = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( far, ter_t_floor );
        here.build_map_cache( 0 );
        auto cands = guy.find_warmth_candidates();
        REQUIRE( cands.size() >= 2 );
        CHECK( cands.front().source_kind == need_source::ground_clothing );
    }

    SECTION( "no sources yields empty" ) {
        here.build_map_cache( 0 );
        CHECK( guy.find_warmth_candidates().empty() );
    }

    SECTION( "already indoors: no shelter candidates but clothing still found" ) {
        // Standing on indoor tile: find_nearby_shelters returns empty.
        here.ter_set( guy.pos_bub(), ter_t_floor );
        here.add_item_or_charges( adj, item( itype_sweater ) );
        here.build_map_cache( 0 );
        auto cands = guy.find_warmth_candidates();
        REQUIRE_FALSE( cands.empty() );
        for( const npc_short_term_cache::need_candidate &c : cands ) {
            CHECK( c.source_kind == need_source::ground_clothing );
        }
    }

    SECTION( "ally without allow_pick_up still finds ground clothing" ) {
        guy.set_fac( faction_your_followers );
        guy.set_attitude( NPCATT_FOLLOW );
        REQUIRE( guy.is_player_ally() );
        guy.rules.clear_flag( ally_rule::allow_pick_up );
        here.add_item_or_charges( adj, item( itype_sweater ) );
        here.build_map_cache( 0 );
        auto cands = guy.find_warmth_candidates();
        REQUIRE_FALSE( cands.empty() );
        CHECK( cands.front().source_kind == need_source::ground_clothing );
    }
}

// Executor-level tests for seek_warmth goal: inventory wear, ground clothing,
// shelter seeking, danger gating, sticky targets, progress tracking.
TEST_CASE( "npc_warmth_executor_contract", "[npc][needs][warmth]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 55, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 55000 );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );
    guy.rules.set_flag( ally_rule::allow_pick_up );

    map &here = get_map();
    using need_source = npc_short_term_cache::need_source;

    SECTION( "inventory sweater: wear immediately, progressed" ) {
        guy.i_add( item( itype_sweater ) );
        REQUIRE_FALSE( guy.is_wearing( itype_sweater ) );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        // One sweater may not resolve extreme cold, so progressed not satisfied.
        CHECK( result == npc::need_result::progressed );
        CHECK( guy.is_wearing( itype_sweater ) );
    }

    SECTION( "adjacent ground clothing: wear in place, progressed" ) {
        const tripoint_bub_ms adj = guy.pos_bub() + point::east;
        here.add_item_or_charges( adj, item( itype_sweater ) );
        here.build_map_cache( 0 );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        CHECK( result == npc::need_result::progressed );
        CHECK( guy.is_wearing( itype_sweater ) );
    }

    SECTION( "distant ground clothing: move toward, progressed" ) {
        const tripoint_bub_ms far = guy.pos_bub() + tripoint( 4, 0, 0 );
        here.add_item_or_charges( far, item( itype_sweater ) );
        here.build_map_cache( 0 );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        CHECK( result == npc::need_result::progressed );
        CHECK( guy.get_warmth_plan().active() );
        CHECK( guy.get_warmth_plan().source_kind == need_source::ground_clothing );
    }

    SECTION( "distant shelter: move toward at low danger, progressed" ) {
        const tripoint_bub_ms far = guy.pos_bub() + tripoint( 4, 0, 0 );
        here.ter_set( far, ter_t_floor );
        here.build_map_cache( 0 );

        guy.set_ai_danger( 0 );
        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        CHECK( result == npc::need_result::progressed );
        CHECK( guy.get_warmth_plan().active() );
        CHECK( guy.get_warmth_plan().source_kind == need_source::shelter );
    }

    SECTION( "shelter movement blocked by danger, deferred" ) {
        const tripoint_bub_ms far = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.ter_set( far, ter_t_floor );
        here.build_map_cache( 0 );

        guy.set_ai_danger( NPC_DANGER_VERY_LOW + 1 );
        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        CHECK( result == npc::need_result::deferred );
        CHECK( guy.get_warmth_plan().active() );
    }

    SECTION( "no warmth sources: impossible" ) {
        here.build_map_cache( 0 );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        CHECK( result == npc::need_result::impossible );
    }

    SECTION( "NPC holds position at shelter until warmth recovers" ) {
        // Adjacent indoor tile; NPC can reach it in one step.
        const tripoint_bub_ms shelter = guy.pos_bub() + point::east;
        here.ter_set( shelter, ter_t_floor );
        here.build_map_cache( 0 );

        // First call: move into shelter.
        guy.set_moves( 100 );
        guy.execute_need_goal( "seek_warmth" );
        REQUIRE( guy.pos_bub() == shelter );

        // NPC is indoors but still freezing. Shelter drops from
        // candidates (find_nearby_shelters returns empty indoors),
        // but the executor should hold position and return progressed,
        // not impossible, so follow/other goals don't pull the NPC out.
        behavior::character_oracle_t oracle( &guy );
        REQUIRE( oracle.needs_warmth_badly( "" ) == behavior::status_t::running );
        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        // holding (not progressed) so dispatch maps to npc_pause
        // which consumes moves, avoiding zero-move spin.
        CHECK( result == npc::need_result::holding );
        CHECK( guy.pos_bub() == shelter );
    }

    SECTION( "NPC at shelter switches to clothing when available" ) {
        // NPC starts indoors (on floor tile) but still cold.
        here.ter_set( guy.pos_bub(), ter_t_floor );
        here.build_map_cache( 0 );

        // No candidates: shelter empty (already indoors), no clothing.
        // Executor should hold position (holding, not progressed).
        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        REQUIRE( result == npc::need_result::holding );

        // Now place distant clothing. The NPC should pursue it instead
        // of staying stuck indoors.
        const tripoint_bub_ms clothing = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.add_item_or_charges( clothing, item( itype_sweater ) );
        here.build_map_cache( 0 );

        guy.set_moves( 100 );
        result = guy.execute_need_goal( "seek_warmth" );
        CHECK( result == npc::need_result::progressed );
        CHECK( guy.get_warmth_plan().active() );
        CHECK( guy.get_warmth_plan().source_kind == need_source::ground_clothing );
    }

    SECTION( "clothing outranks shelter when clothing scores higher" ) {
        // Shelter at distance 2 (score -2), clothing at distance 4 (warmth ~30).
        // Clothing's warmth score beats shelter's negative-distance score.
        const tripoint_bub_ms shelter = guy.pos_bub() + tripoint( 2, 0, 0 );
        here.ter_set( shelter, ter_t_floor );
        const tripoint_bub_ms clothing = guy.pos_bub() + tripoint( 4, 0, 0 );
        here.add_item_or_charges( clothing, item( itype_sweater ) );
        here.build_map_cache( 0 );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        REQUIRE( result == npc::need_result::progressed );
        CHECK( guy.get_warmth_plan().target == here.get_abs( clothing ) );
        CHECK( guy.get_warmth_plan().source_kind == need_source::ground_clothing );
    }

    SECTION( "unreachable clothing times out then skips to second target" ) {
        // First sweater behind glass (visible, unreachable without bashing).
        const tripoint_bub_ms close = guy.pos_bub() + tripoint( 2, 0, 0 );
        for( const point &d : {
                 point::north, point::south, point::east, point::west,
                 point::north_west, point::north_east, point::south_west, point::south_east
             } ) {
            here.ter_set( close + d, ter_t_wall_glass );
        }
        here.add_item_or_charges( close, item( itype_sweater ) );
        // Second sweater in the open.
        const tripoint_bub_ms far = guy.pos_bub() + tripoint( 0, -4, 0 );
        here.add_item_or_charges( far, item( itype_sweater ) );
        here.build_map_cache( 0 );

        auto cands = guy.find_warmth_candidates();
        REQUIRE( cands.size() >= 2 );

        // 5-turn no-progress timeout on first target.
        for( int turn = 0; turn < 5; ++turn ) {
            guy.set_moves( 100 );
            guy.execute_need_goal( "seek_warmth" );
        }
        REQUIRE( guy.get_warmth_plan().last_result == npc::need_result::impossible );

        // Next call skips the failed target and picks the second one.
        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        CHECK( guy.get_warmth_plan().active() );
        CHECK( guy.get_warmth_plan().target == here.get_abs( far ) );
        CHECK( result == npc::need_result::progressed );
    }

    SECTION( "indoor hold returns holding, not blocked" ) {
        // NPC starts indoors, no warmth sources.
        here.ter_set( guy.pos_bub(), ter_t_floor );
        here.build_map_cache( 0 );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        CHECK( result == npc::need_result::holding );
        // holding must not increment the no-progress counter
        CHECK( guy.plan_for( npc::need_goal_id::seek_warmth ).no_progress_turns == 0 );
    }
}

// seek_warmth commitment lifecycle through npc::move().
TEST_CASE( "npc_seek_warmth_commitment_lifecycle", "[npc][needs][warmth]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 55, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 55000 );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );
    guy.rules.set_flag( ally_rule::allow_pick_up );

    map &here = get_map();

    SECTION( "commitment clears when warmth resolves" ) {
        // Place distant clothing so seek_warmth activates.
        const tripoint_bub_ms far = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.add_item_or_charges( far, item( itype_sweater ) );
        here.build_map_cache( 0 );

        guy.set_committed_goal( "seek_warmth" );
        guy.set_moves( 100 );
        guy.move();
        REQUIRE( guy.get_committed_goal() == "seek_warmth" );

        // Warm up the NPC so the predicate clears.
        guy.set_all_parts_temp_conv( BODYTEMP_NORM );
        guy.set_moves( 100 );
        guy.move();
        CHECK( guy.get_committed_goal() != "seek_warmth" );
    }

    SECTION( "stale impossible from prior cycle does not kill live commitment" ) {
        // No warmth sources: executor returns impossible.
        here.build_map_cache( 0 );
        guy.set_committed_goal( "seek_warmth" );
        guy.set_moves( 100 );
        guy.move();
        // Commitment should have cleared (impossible).
        REQUIRE( guy.get_committed_goal() != "seek_warmth" );

        // Now add a sweater to inventory and recommit.
        guy.i_add( item( itype_sweater ) );
        guy.set_committed_goal( "seek_warmth" );
        guy.set_moves( 100 );
        guy.move();
        // The executor should have worn the sweater (progressed),
        // and the commitment should still be alive because warmth
        // is not yet resolved (still VERY_COLD).
        CHECK( guy.is_wearing( itype_sweater ) );
        behavior::character_oracle_t oracle( &guy );
        if( oracle.needs_warmth_badly( "" ) == behavior::status_t::running ) {
            CHECK( guy.get_committed_goal() == "seek_warmth" );
        }
    }

    SECTION( "indoor hold does not produce zero-move spin" ) {
        // NPC starts indoors, no warmth sources.
        here.ter_set( guy.pos_bub(), ter_t_floor );
        here.build_map_cache( 0 );

        guy.set_committed_goal( "seek_warmth" );
        const int moves_before = 100;
        guy.set_moves( moves_before );
        guy.move();
        // The NPC should have consumed moves (not spun at 0).
        CHECK( guy.get_moves() < moves_before );
    }

    SECTION( "cold NPC starting indoors without commitment stays put" ) {
        // Fresh BT evaluation, no prior seek_warmth commitment.
        // NPC is cold, indoors, no warmth sources. The BT should
        // commit seek_warmth (predicate sees indoors-hold case),
        // and the NPC should not leave shelter.
        here.ter_set( guy.pos_bub(), ter_t_floor );
        here.build_map_cache( 0 );
        REQUIRE( guy.get_committed_goal().empty() );

        const tripoint_bub_ms start = guy.pos_bub();
        guy.set_moves( 100 );
        guy.move();
        // NPC should stay indoors, not follow player outside.
        CHECK( guy.pos_bub() == start );
    }
}

// Same-category preemption: a more urgent executor need can interrupt a
// less urgent one when the committed goal is in holding or blocked state.
TEST_CASE( "same_category_preemption", "[npc][needs]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    // Player adjacent to NPC so following urgency stays near zero.
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 51, 0 } );
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 55000 );
    guy.set_hunger( 0 );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );

    map &here = get_map();

    SECTION( "holding warmth preempted by critical thirst" ) {
        // Cold NPC indoors (holding), critically thirsty, water in inventory.
        // At current thresholds: warmth ~0.67, thirst ~0.88.
        // Thirst exceeds warmth + preempt_margin (0.15).
        get_weather().forced_temperature = -30_C;
        here.ter_set( guy.pos_bub(), ter_t_floor );
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        guy.set_thirst( 1050 );
        // Inventory water (in bottle) so has_water returns true and
        // BT selects drink_water.
        const item_group::ItemList water_items = item_group::items_from(
                    Item_spawn_data_test_bottle_water );
        guy.i_add( water_items.front() );
        here.build_map_cache( 0 );

        // Pre-commit seek_warmth in holding state.
        guy.set_committed_goal( "seek_warmth" );
        guy.plan_for( npc::need_goal_id::seek_warmth ).last_result =
            npc::need_result::holding;

        guy.set_moves( 100 );
        guy.move();
        CHECK( guy.get_committed_goal() == "drink_water" );
    }

    SECTION( "progressed warmth not preempted by moderate thirst" ) {
        // Cold NPC actively moving toward clothing (progressed).
        // Moderate thirst (600/1200 = 0.5) should NOT preempt because
        // the committed goal is making progress.
        get_weather().forced_temperature = -30_C;
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        guy.set_thirst( 600 );
        // Place clothing so seek_warmth has a plan.
        const tripoint_bub_ms clothing = guy.pos_bub() + tripoint( 5, 0, 0 );
        here.add_item_or_charges( clothing, item( itype_sweater ) );
        // Place water so drink_water is viable.
        const tripoint_bub_ms water = guy.pos_bub() + tripoint( 0, 3, 0 );
        here.ter_set( water, ter_t_water_sh );
        here.build_map_cache( 0 );

        // Pre-commit seek_warmth in progressed state with an active plan.
        guy.set_committed_goal( "seek_warmth" );
        npc::need_plan &wp = guy.plan_for( npc::need_goal_id::seek_warmth );
        wp.goal = "seek_warmth";
        wp.source_kind = npc::need_source::ground_clothing;
        wp.target = here.get_abs( clothing );
        wp.last_result = npc::need_result::progressed;

        guy.set_moves( 100 );
        guy.move();
        CHECK( guy.get_committed_goal() == "seek_warmth" );
    }

    SECTION( "progressed warmth preempted by critical thirst" ) {
        // Cold NPC actively moving toward clothing (progressed), but
        // critically thirsty (urgency > 0.75 critical threshold and
        // exceeds warmth urgency). Should preempt.
        get_weather().forced_temperature = -30_C;
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        guy.set_thirst( 1050 );
        const tripoint_bub_ms clothing = guy.pos_bub() + tripoint( 5, 0, 0 );
        here.add_item_or_charges( clothing, item( itype_sweater ) );
        const item_group::ItemList water_items = item_group::items_from(
                    Item_spawn_data_test_bottle_water );
        guy.i_add( water_items.front() );
        here.build_map_cache( 0 );

        guy.set_committed_goal( "seek_warmth" );
        npc::need_plan &wp = guy.plan_for( npc::need_goal_id::seek_warmth );
        wp.goal = "seek_warmth";
        wp.source_kind = npc::need_source::ground_clothing;
        wp.target = here.get_abs( clothing );
        wp.last_result = npc::need_result::progressed;

        guy.set_moves( 100 );
        guy.move();
        CHECK( guy.get_committed_goal() == "drink_water" );
    }

    SECTION( "deferred warmth preempted by urgent thirst" ) {
        // Cold NPC deferred (danger blocked movement). Urgent thirst
        // should preempt with the same margin as blocked state.
        get_weather().forced_temperature = -30_C;
        guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        guy.set_thirst( 1050 );
        const item_group::ItemList water_items = item_group::items_from(
                    Item_spawn_data_test_bottle_water );
        guy.i_add( water_items.front() );
        here.build_map_cache( 0 );

        guy.set_committed_goal( "seek_warmth" );
        guy.plan_for( npc::need_goal_id::seek_warmth ).last_result =
            npc::need_result::deferred;

        guy.set_moves( 100 );
        guy.move();
        CHECK( guy.get_committed_goal() == "drink_water" );
    }
}

TEST_CASE( "npc_fire_executor_contract", "[npc][needs][warmth][fire]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 55, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 55000 );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );

    map &here = get_map();
    here.build_map_cache( 0 );

    SECTION( "empty lighter + firewood: no fire plan" ) {
        // Raw lighter has no butane charges.
        guy.i_add( item( itype_lighter ) );
        guy.i_add( item( itype_2x4 ) );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        // No usable firestarter, no clothing, and grass is not shelter.
        // Should fall through to impossible (no warmth candidates).
        CHECK( result == npc::need_result::impossible );
    }

    SECTION( "charged lighter + no firewood + nonflammable tiles: impossible" ) {
        guy.i_add( tool_with_ammo( itype_lighter, 20 ) );
        // No firewood. Set all adjacent tiles to non-flammable concrete.
        for( const tripoint_bub_ms &p : here.points_in_radius( guy.pos_bub(), 1 ) ) {
            if( p != guy.pos_bub() ) {
                here.ter_set( p, ter_t_concrete_wall );
            }
        }
        here.build_map_cache( 0 );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        CHECK( result == npc::need_result::impossible );
    }

    SECTION( "charged lighter + adjacent flammable fuel: progressed" ) {
        guy.i_add( tool_with_ammo( itype_lighter, 20 ) );
        // Drop a 2x4 on an adjacent tile to make it flammable.
        const tripoint_bub_ms adj = guy.pos_bub() + point::east;
        here.add_item_or_charges( adj, item( itype_2x4 ) );
        here.build_map_cache( 0 );
        REQUIRE( guy.find_fire_spot().has_value() );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        CHECK( result == npc::need_result::progressed );
        // Lighter is fast enough for instant fire or activity.
        CHECK( ( here.get_field( adj, fd_fire ) || guy.activity ) );
    }

    SECTION( "charged lighter + firewood + nonflammable tile: starts fire" ) {
        guy.i_add( tool_with_ammo( itype_lighter, 20 ) );
        guy.i_add( item( itype_2x4 ) );
        // Default terrain is t_grass (outdoor, non-flammable, not shelter).
        // Verify preconditions: fire spot exists, no shelter/clothing candidates.
        here.build_map_cache( 0 );
        REQUIRE( guy.find_fire_spot().has_value() );
        REQUIRE( guy.find_nearby_warm_clothing().empty() );
        REQUIRE( guy.find_nearby_shelters().empty() );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        CHECK( result == npc::need_result::progressed );
        REQUIRE( guy.get_warmth_plan().active() );
        REQUIRE( guy.get_warmth_plan().source_kind == npc_short_term_cache::need_source::fire_spot );
        const tripoint_bub_ms target = here.get_bub( guy.get_warmth_plan().target );
        CHECK( ( here.get_field( target, fd_fire ) || guy.activity ) );
    }

    SECTION( "adjacent fire already exists: holding" ) {
        guy.i_add( tool_with_ammo( itype_lighter, 20 ) );
        guy.i_add( item( itype_2x4 ) );
        const tripoint_bub_ms adj = guy.pos_bub() + point::east;
        here.add_field( adj, fd_fire, 1, 10_minutes );
        here.build_map_cache( 0 );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        CHECK( result == npc::need_result::holding );
    }

    SECTION( "creature on adjacent tile: fire spot skips it" ) {
        guy.i_add( tool_with_ammo( itype_lighter, 20 ) );
        guy.i_add( item( itype_2x4 ) );
        // Block all adjacent tiles except east with walls.
        for( const tripoint_bub_ms &p : here.points_in_radius( guy.pos_bub(), 1 ) ) {
            if( p != guy.pos_bub() && p != guy.pos_bub() + point::east ) {
                here.ter_set( p, ter_t_concrete_wall );
            }
        }
        // Place a monster on the only open tile.
        const tripoint_bub_ms east = guy.pos_bub() + point::east;
        monster &mon = spawn_test_monster( "mon_zombie", east );
        ( void ) mon;
        here.build_map_cache( 0 );

        // No valid fire spot (only open tile has a creature).
        CHECK_FALSE( guy.find_fire_spot().has_value() );
    }

    SECTION( "NPC_NO_GO zone on adjacent tile: fire spot skips it" ) {
        guy.i_add( tool_with_ammo( itype_lighter, 20 ) );
        guy.i_add( item( itype_2x4 ) );
        // Block all adjacent tiles except east with walls.
        for( const tripoint_bub_ms &p : here.points_in_radius( guy.pos_bub(), 1 ) ) {
            if( p != guy.pos_bub() && p != guy.pos_bub() + point::east ) {
                here.ter_set( p, ter_t_concrete_wall );
            }
        }
        // NPC_NO_GO zone covering the east tile.
        const tripoint_abs_ms east_abs = guy.pos_abs() + tripoint::east;
        zone_manager &mgr = zone_manager::get_manager();
        mgr.add( "test_no_go", zone_type_NPC_NO_GO, guy.get_fac_id(),
                 false, true, east_abs, east_abs );
        mgr.cache_data();
        here.build_map_cache( 0 );

        CHECK_FALSE( guy.find_fire_spot().has_value() );

        mgr.clear();
        mgr.cache_data();
    }

    SECTION( "wielded firewood not burned as fuel" ) {
        guy.i_add( tool_with_ammo( itype_lighter, 20 ) );
        // Wield a 2x4 (FIREWOOD flag, melee_bash 16).
        item plank( itype_2x4 );
        guy.wield( plank );
        REQUIRE( guy.get_wielded_item() );
        REQUIRE( guy.get_wielded_item()->typeId() == itype_2x4 );
        // Also add a 2x4 to inventory so there IS valid fuel.
        guy.i_add( item( itype_2x4 ) );
        here.build_map_cache( 0 );
        REQUIRE( guy.find_fire_spot().has_value() );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        CHECK( result == npc::need_result::progressed );
        // The wielded weapon must still be a 2x4.
        REQUIRE( guy.get_wielded_item() );
        CHECK( guy.get_wielded_item()->typeId() == itype_2x4 );
    }

    SECTION( "only wielded firewood: no fuel, impossible" ) {
        guy.i_add( tool_with_ammo( itype_lighter, 20 ) );
        item plank( itype_2x4 );
        guy.wield( plank );
        REQUIRE( guy.get_wielded_item() );
        // No inventory firewood. All adjacent tiles are non-flammable.
        // find_fire_spot should return nullopt (no safe fuel available).
        for( const tripoint_bub_ms &p : here.points_in_radius( guy.pos_bub(), 1 ) ) {
            if( p != guy.pos_bub() ) {
                here.ter_set( p, ter_t_concrete );
            }
        }
        here.build_map_cache( 0 );
        CHECK_FALSE( guy.find_fire_spot().has_value() );
    }
}

// Thirsty NPCs with only forageable fruit nearby should treat harvestable
// terrain as a water source (fruits have positive quench).
TEST_CASE( "npc_harvest_for_water", "[npc][needs][forage]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 55, 0 } );
    get_weather().forced_temperature = 20_C;
    // Apple tree requires autumn to be harvestable. set_time refreshes
    // light/visibility caches needed for NPC sees() checks.
    set_time( calendar::turn_zero + 2 * calendar::season_length() + 12_hours );

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 55000 );
    guy.set_hunger( 0 );
    guy.set_thirst( 200 );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );

    map &here = get_map();
    const tripoint_bub_ms tree_pos = guy.pos_bub() + tripoint( 3, 0, 0 );
    here.ter_set( tree_pos, ter_t_tree_apple );
    here.build_map_cache( 0 );
    REQUIRE( here.is_harvestable( tree_pos ) );

    // Precondition: the harvestable function finds the tree.
    REQUIRE_FALSE( guy.find_nearby_harvestable( false ).empty() );

    // No ground drinks, no water terrain, no camp water. Only the
    // harvestable apple tree. Water candidates must include it.
    auto candidates = guy.find_water_candidates();
    CHECK_FALSE( candidates.empty() );
}

// start_fire is a stale-save compat goal. The commitment block still
// clears it when the BT selects a different goal.
TEST_CASE( "start_fire_does_not_block_needs_forever", "[npc][needs]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 51, 0 } );
    set_time_to_day();
    get_weather().forced_temperature = 20_C;

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 55000 );
    guy.set_hunger( 0 );
    guy.set_thirst( 900 );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );
    // Give water so the BT can select drink_water.
    const item_group::ItemList water_items = item_group::items_from(
                Item_spawn_data_test_bottle_water );
    guy.i_add( water_items.front() );

    map &here = get_map();
    here.build_map_cache( 0 );

    // Pre-commit start_fire.  The NPC is warm (no warmth need), so the
    // BT will return drink_water instead of start_fire.  The BT-fallthrough
    // completion check should clear the start_fire commitment.
    guy.set_committed_goal( "start_fire" );
    guy.set_moves( 100 );
    guy.move();
    CHECK( guy.get_committed_goal() != "start_fire" );
}

// External commitment clears (dialogue, schedule transitions) must also
// clear executor plan and failed-target state.
TEST_CASE( "clear_committed_goal_clears_executor_state", "[npc][needs]" )
{
    clear_map_without_vision();
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );

    // Simulate an active eat_food plan with failed targets.
    guy.set_committed_goal( "eat_food" );
    npc::need_plan &fp = guy.plan_for( npc::need_goal_id::eat_food );
    fp.goal = "eat_food";
    fp.source_kind = npc::need_source::ground_item;
    fp.target = tripoint_abs_ms{ 55, 50, 0 };
    fp.last_result = npc::need_result::progressed;
    fp.no_progress_turns = 3;
    guy.failed_targets_for( npc::need_goal_id::eat_food ).insert(
        tripoint_abs_ms{ 60, 50, 0 } );

    // clear_committed_goal must reset everything.
    guy.clear_committed_goal();
    CHECK( guy.get_committed_goal().empty() );
    CHECK_FALSE( fp.active() );
    CHECK( fp.no_progress_turns == 0 );
    CHECK( guy.failed_targets_for( npc::need_goal_id::eat_food ).empty() );
}

// Camp candidates must never become sticky plan targets.  The step-3
// acquisition loop skips camp_food/camp_water source kinds.  After any
// executor run, the plan's source_kind must never be a camp type.
TEST_CASE( "camp_candidates_not_planned", "[npc][needs][camp]" )
{
    clear_avatar();
    clear_map_without_vision();
    get_player_character().camps.clear();
    map &m = get_map();

    const tripoint_bub_ms mid{ MAPSIZE_X / 2, MAPSIZE_Y / 2, 0 };
    const tripoint_abs_ms zone_loc = m.get_abs( mid );
    REQUIRE( m.inbounds( zone_loc ) );
    mapgen_place_zone( zone_loc, zone_loc, zone_type_CAMP_FOOD, your_fac, {}, "food" );
    mapgen_place_zone( zone_loc, zone_loc, zone_type_CAMP_STORAGE, your_fac, {}, "storage" );
    faction *camp_faction = get_player_character().get_faction();
    const tripoint_abs_omt this_omt = project_to<coords::omt>( zone_loc );
    m.add_camp( this_omt, "faction_camp" );
    std::optional<basecamp *> bcp = overmap_buffer.find_camp( this_omt.xy() );
    REQUIRE( !!bcp );
    basecamp *test_camp = *bcp;
    test_camp->define_camp( this_omt, "faction_base_bare_bones_NPC_camp_0", false );
    test_camp->set_owner( your_fac );

    // Stock the camp larder.
    camp_faction->empty_food_supply();
    camp_faction->debug_food_supply().emplace_back( calendar::turn_zero, nutrients{} );
    camp_faction->debug_food_supply().back().second.calories = 500 * 1000;
    REQUIRE( camp_faction->food_supply().kcal() >= 500 );

    npc &guy = spawn_npc( mid.xy(), "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_stored_kcal( 1000 );
    guy.set_hunger( -1 );
    guy.set_thirst( 0 );
    m.build_map_cache( 0 );

    // Camp food is in the candidate list.
    auto cands = guy.find_food_candidates();
    bool has_camp = false;
    for( const npc_short_term_cache::need_candidate &c : cands ) {
        if( c.source_kind == npc::need_source::camp_food ) {
            has_camp = true;
        }
    }
    REQUIRE( has_camp );

    // Run the executor. Regardless of whether step-1 camp consumption
    // succeeds or the executor falls through to step-3, the plan must
    // never target a camp source.
    guy.set_committed_goal( "eat_food" );
    guy.set_moves( 100 );
    guy.execute_need_goal( "eat_food" );
    const npc::need_plan &fp = guy.plan_for( npc::need_goal_id::eat_food );
    CHECK( fp.source_kind != npc::need_source::camp_food );
    CHECK( fp.source_kind != npc::need_source::camp_water );
}

TEST_CASE( "need_goal_id_helpers", "[npc][needs]" )
{
    // goal_id_for maps known executor goal strings to typed IDs.
    REQUIRE( npc::goal_id_for( "eat_food" ).has_value() );
    CHECK( *npc::goal_id_for( "eat_food" ) == npc::need_goal_id::eat_food );
    REQUIRE( npc::goal_id_for( "drink_water" ).has_value() );
    CHECK( *npc::goal_id_for( "drink_water" ) == npc::need_goal_id::drink_water );
    REQUIRE( npc::goal_id_for( "seek_warmth" ).has_value() );
    CHECK( *npc::goal_id_for( "seek_warmth" ) == npc::need_goal_id::seek_warmth );

    REQUIRE( npc::goal_id_for( "go_to_sleep" ).has_value() );
    CHECK( *npc::goal_id_for( "go_to_sleep" ) == npc::need_goal_id::go_to_sleep );

    // Non-executor goals return nullopt.
    CHECK_FALSE( npc::goal_id_for( "follow_player" ).has_value() );
    CHECK_FALSE( npc::goal_id_for( "idle" ).has_value() );
    CHECK_FALSE( npc::goal_id_for( "" ).has_value() );

    // is_executor_goal convenience wrapper.
    CHECK( npc::is_executor_goal( "eat_food" ) );
    CHECK( npc::is_executor_goal( "drink_water" ) );
    CHECK( npc::is_executor_goal( "seek_warmth" ) );
    CHECK( npc::is_executor_goal( "go_to_sleep" ) );
    CHECK_FALSE( npc::is_executor_goal( "follow_player" ) );
    CHECK_FALSE( npc::is_executor_goal( "idle" ) );
}

TEST_CASE( "need_goal_id_plan_accessors", "[npc][needs]" )
{
    clear_map_without_vision();
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );

    // plan_for returns the correct underlying cache member.
    // Capture non-const result first to avoid unsequenced calls in CHECK.
    npc::need_plan *food_ptr = &guy.plan_for( npc::need_goal_id::eat_food );
    npc::need_plan *water_ptr = &guy.plan_for( npc::need_goal_id::drink_water );
    npc::need_plan *warmth_ptr = &guy.plan_for( npc::need_goal_id::seek_warmth );
    npc::need_plan *sleep_ptr = &guy.plan_for( npc::need_goal_id::go_to_sleep );
    CHECK( food_ptr == &guy.get_food_plan() );
    CHECK( water_ptr == &guy.get_water_plan() );
    CHECK( warmth_ptr == &guy.get_warmth_plan() );
    CHECK( sleep_ptr == &guy.get_sleep_plan() );

    // clear_need_state resets both plan and failed targets.
    npc::need_plan &fp = guy.plan_for( npc::need_goal_id::eat_food );
    fp.goal = "eat_food";
    fp.no_progress_turns = 3;
    guy.failed_targets_for( npc::need_goal_id::eat_food ).insert(
        tripoint_abs_ms{ 1, 2, 0 } );
    guy.clear_need_state( npc::need_goal_id::eat_food );
    CHECK_FALSE( fp.active() );
    CHECK( fp.no_progress_turns == 0 );
    CHECK( guy.failed_targets_for( npc::need_goal_id::eat_food ).empty() );
}

// Failed-target history survives impossible dispatch so the same bad
// target is not immediately reacquired. Targets clear naturally when
// all candidates are exhausted (one tick to flush) or on satisfaction.
TEST_CASE( "impossible_dispatch_preserves_failed_targets", "[npc][needs]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 55, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 20000 );
    guy.set_hunger( -1 );
    guy.set_thirst( 0 );

    map &here = get_map();

    // Place food behind glass walls -- visible but unreachable without
    // bashing.  The executor will target it and time out after 5 turns,
    // populating food_failed_targets.
    const tripoint_bub_ms food_pos = guy.pos_bub() + tripoint( 3, 0, 0 );
    for( const point &d : {
             point::north, point::south, point::east, point::west,
             point::north_west, point::north_east, point::south_west, point::south_east
         } ) {
        here.ter_set( food_pos + d, ter_t_wall_glass );
    }
    here.add_item_or_charges( food_pos, item( itype_sandwich_cheese_grilled ) );
    here.build_map_cache( 0 );

    // Step 1: Run the executor for 5 turns to hit the no-progress timeout.
    guy.set_committed_goal( "eat_food" );
    npc::need_result result = npc::need_result::idle;
    for( int turn = 0; turn < 5; ++turn ) {
        guy.set_moves( 100 );
        result = guy.execute_need_goal( "eat_food" );
    }
    REQUIRE( result == npc::need_result::impossible );
    // The timeout path inserts the target into failed_targets.
    REQUIRE_FALSE( guy.failed_targets_for( npc::need_goal_id::eat_food ).empty() );

    // Simulate the dispatch impossible path (normally done by move()).
    // Dispatch clears the plan but preserves failed-target history so the
    // same bad target is not immediately reacquired.
    if( auto gid = npc::goal_id_for( "eat_food" ); gid ) {
        guy.plan_for( *gid ).clear();
    }

    // Plan is cleared but failed targets persist.
    CHECK_FALSE( guy.plan_for( npc::need_goal_id::eat_food ).active() );
    CHECK_FALSE( guy.failed_targets_for( npc::need_goal_id::eat_food ).empty() );

    // Step 2: Remove the glass walls and recommit.  The NPC should
    // acquire the same food tile. Failed targets from step 1 persist
    // across the impossible dispatch, but the executor clears them
    // when all candidates are exhausted (one tick to flush). Manually
    // clear here to simulate the flush and test fresh retargeting.
    for( const point &d : {
             point::north, point::south, point::east, point::west,
             point::north_west, point::north_east, point::south_west, point::south_east
         } ) {
        here.ter_set( food_pos + d, ter_t_floor );
    }
    here.build_map_cache( 0 );

    guy.failed_targets_for( npc::need_goal_id::eat_food ).clear();
    guy.set_committed_goal( "eat_food" );
    guy.set_moves( 100 );
    result = guy.execute_need_goal( "eat_food" );
    CHECK( result == npc::need_result::progressed );
    CHECK( guy.plan_for( npc::need_goal_id::eat_food ).active() );
    CHECK( guy.plan_for( npc::need_goal_id::eat_food ).target == here.get_abs( food_pos ) );
}

// drink_water commitment lifecycle through npc::move().
TEST_CASE( "npc_drink_water_commitment_clears_when_unobtainable", "[npc][needs][water]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 10, 10, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 55000 );
    guy.set_hunger( 0 );
    // Must exceed needs_water_badly threshold (520) so the commitment
    // doesn't clear via the "satisfied" path before the executor runs.
    guy.set_thirst( 600 );
    REQUIRE( guy.get_thirst() > 520 );

    map &here = get_map();
    here.build_map_cache( 0 );

    guy.set_committed_goal( "drink_water" );

    for( int turn = 0; turn < 5; ++turn ) {
        guy.set_moves( 100 );
        guy.move();
    }
    CHECK( guy.get_committed_goal() != "drink_water" );
}

// can_obtain_water should detect nearby water terrain sources.
TEST_CASE( "npc_can_obtain_water_predicate", "[npc][needs][water]" )
{
    clear_map_without_vision();
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_thirst( 200 );

    map &here = get_map();
    const tripoint_bub_ms adj = guy.pos_bub() + point::east;
    behavior::character_oracle_t oracle( &guy );

    SECTION( "fresh shallow water is obtainable" ) {
        here.ter_set( adj, ter_t_water_sh );
        here.build_map_cache( 0 );
        CHECK( oracle.can_obtain_water( "" ) == behavior::status_t::running );
    }

    SECTION( "salt water is not obtainable" ) {
        here.ter_set( adj, ter_t_swater_sh );
        here.build_map_cache( 0 );
        CHECK( oracle.can_obtain_water( "" ) == behavior::status_t::failure );
    }

    SECTION( "no water terrain nearby" ) {
        here.build_map_cache( 0 );
        CHECK( oracle.can_obtain_water( "" ) == behavior::status_t::failure );
    }
}

TEST_CASE( "npc_sleep_executor_contract", "[npc][needs][sleep]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 55, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 55000 );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );

    map &here = get_map();
    using need_source = npc_short_term_cache::need_source;

    SECTION( "current tile is valid: falls asleep here" ) {
        guy.set_sleepiness( sleepiness_levels::MASSIVE_SLEEPINESS );
        // Wall off all adjacent tiles so the NPC can only sleep in place.
        for( const tripoint_bub_ms &p : here.points_in_radius( guy.pos_bub(), 1 ) ) {
            if( p != guy.pos_bub() ) {
                here.ter_set( p, ter_t_concrete_wall );
            }
        }
        here.build_map_cache( 0 );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "go_to_sleep" );
        CHECK( ( result == npc::need_result::progressed ||
                 result == npc::need_result::holding ) );
        CHECK( ( guy.has_effect( effect_sleep ) ||
                 guy.has_effect( effect_lying_down ) ) );
    }

    SECTION( "distant bed: moves toward it" ) {
        guy.set_sleepiness( sleepiness_levels::MASSIVE_SLEEPINESS );
        const tripoint_bub_ms bed_pos = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.furn_set( bed_pos, furn_f_bed );
        here.build_map_cache( 0 );

        guy.set_ai_danger( 0 );
        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "go_to_sleep" );
        CHECK( result == npc::need_result::progressed );
        CHECK( guy.get_sleep_plan().active() );
        CHECK( guy.get_sleep_plan().source_kind == need_source::sleep_spot );
    }

    SECTION( "at bed tile: falls asleep" ) {
        guy.set_sleepiness( sleepiness_levels::MASSIVE_SLEEPINESS );
        here.furn_set( guy.pos_bub(), furn_f_bed );
        here.build_map_cache( 0 );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "go_to_sleep" );
        CHECK( ( result == npc::need_result::progressed ||
                 result == npc::need_result::holding ) );
        CHECK( ( guy.has_effect( effect_sleep ) ||
                 guy.has_effect( effect_lying_down ) ) );
    }

    // lying_down test is a separate TEST_CASE below to avoid
    // effect_sleep leaking from other sections in the full suite.

    SECTION( "danger blocks distant movement: deferred" ) {
        guy.set_sleepiness( sleepiness_levels::MASSIVE_SLEEPINESS );
        const tripoint_bub_ms bed_pos = guy.pos_bub() + tripoint( 4, 0, 0 );
        here.furn_set( bed_pos, furn_f_bed );
        here.build_map_cache( 0 );

        guy.set_ai_danger( NPC_DANGER_VERY_LOW + 1 );
        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "go_to_sleep" );
        CHECK( result == npc::need_result::deferred );
    }

    // No sleep-specific timeout test: find_sleep_candidates() always
    // includes the NPC's own tile, so blocked-progress is hard to
    // construct. The timeout branch is exercised through warmth/food/water
    // where unreachable targets are constructible.
}

// Standalone test for lying_down -> holding, isolated from the
// executor contract to avoid effect_sleep cross-section leakage.
TEST_CASE( "npc_sleep_lying_down_returns_holding", "[npc][needs][sleep]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 55, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_sleepiness( 300 );
    REQUIRE_FALSE( guy.has_effect( effect_sleep ) );
    // deferred=true so add_effect does not immediately process the
    // lying_down handler (which calls can_sleep -> fall_asleep).
    // Signature: (id, dur, permanent, intensity, force, deferred)
    guy.add_effect( effect_lying_down, 30_minutes, false, 1, false, true );
    REQUIRE_FALSE( guy.has_effect( effect_sleep ) );
    REQUIRE( guy.has_effect( effect_lying_down ) );

    guy.set_moves( 100 );
    npc::need_result result = guy.execute_need_goal( "go_to_sleep" );
    CHECK( result == npc::need_result::holding );
}

TEST_CASE( "npc_sleep_preemption", "[npc][needs][sleep]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 55, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 55000 );
    guy.set_hunger( 0 );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );

    // Moderate sleepiness so sleep urgency is moderate.
    guy.set_sleepiness( 400 );

    // Extreme thirst so drink urgency is very high.
    guy.set_thirst( 1100 );
    const item_group::ItemList water_items = item_group::items_from(
                Item_spawn_data_test_bottle_water );
    guy.i_add( water_items.front() );

    map &here = get_map();
    here.build_map_cache( 0 );

    // Pre-commit go_to_sleep with holding state (lying down).
    // Apply effect_lying_down with deferred=true to avoid
    // process_one_effect -> can_sleep() -> fall_asleep().
    guy.set_committed_goal( "go_to_sleep" );
    npc::need_plan &sp = guy.plan_for( npc::need_goal_id::go_to_sleep );
    sp.goal = "go_to_sleep";
    sp.last_result = npc::need_result::holding;
    guy.add_effect( effect_lying_down, 30_minutes, false, 1, false, true );
    REQUIRE( guy.has_effect( effect_lying_down ) );

    guy.set_moves( 100 );
    guy.move();

    // Thirst urgency (1100/1200 = 0.92) should preempt sleep
    // urgency (400/1000 = 0.40) with margin.
    CHECK( guy.get_committed_goal() != "go_to_sleep" );
    // Preemption must remove lying_down so process_one_effect
    // doesn't re-trigger can_sleep() -> fall_asleep().
    CHECK_FALSE( guy.has_effect( effect_lying_down ) );
}

// Foraging/harvesting NPCs must react to danger instead of blindly
// continuing the activity. The early-return optimization prevents BT
// re-evaluation (which would flood the backlog), but danger assessment
// must still interrupt.
TEST_CASE( "npc_forage_yields_to_danger", "[npc][needs][forage]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 55000 );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );

    // clear_character teleports to (60,60). Reposition NPC and player
    // so follow_close / must_retreat doesn't suppress danger assessment.
    map &here = get_map();
    guy.setpos( here, tripoint_bub_ms{ 50, 50, 0 } );
    get_player_character().setpos( here, guy.pos_bub() + point::north );
    here.build_map_cache( 0 );

    SECTION( "foraging NPC continues in safety" ) {
        guy.assign_activity( forage_activity_actor( 1000 ) );
        REQUIRE( guy.activity.id() == ACT_FORAGE );

        guy.set_moves( 100 );
        guy.move();

        CHECK( guy.activity.id() == ACT_FORAGE );
    }

    SECTION( "foraging NPC cancels activity on real danger" ) {
        // mon_zombie_brute_shocker has diff 10, well above NPC_DANGER_VERY_LOW.
        const tripoint_bub_ms adj = guy.pos_bub() + point::east;
        spawn_test_monster( "mon_zombie_brute_shocker", adj );
        here.build_map_cache( 0 );
        guy.regen_ai_cache();
        REQUIRE( guy.get_ai_danger() > NPC_DANGER_VERY_LOW );

        guy.assign_activity( forage_activity_actor( 1000 ) );
        REQUIRE( guy.activity.id() == ACT_FORAGE );

        guy.set_moves( 100 );
        guy.move();

        CHECK( guy.activity.id() != ACT_FORAGE );
    }

    SECTION( "harvesting NPC cancels activity on real danger" ) {
        set_time( calendar::turn_zero + 2 * calendar::season_length() + 12_hours );
        const tripoint_bub_ms tree = guy.pos_bub() + point::east;
        here.ter_set( tree, ter_t_tree_apple );
        const tripoint_bub_ms adj = guy.pos_bub() + point::south;
        spawn_test_monster( "mon_zombie_brute_shocker", adj );
        here.build_map_cache( 0 );
        guy.regen_ai_cache();
        REQUIRE( guy.get_ai_danger() > NPC_DANGER_VERY_LOW );
        REQUIRE( here.is_harvestable( tree ) );

        guy.assign_activity( harvest_activity_actor( tree, false ) );
        REQUIRE( guy.activity.id() == ACT_HARVEST );

        guy.set_moves( 100 );
        guy.move();

        CHECK( guy.activity.id() != ACT_HARVEST );
    }
}

// Auto-eating harvested food must only happen when the NPC is executing
// a self-care need goal (eat_food / drink_water), not during generic
// harvest work (farming zones, player-assigned tasks).
TEST_CASE( "npc_harvest_auto_eat_restricted_to_self_care", "[npc][needs][forage]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 52, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 1000 );
    guy.set_hunger( 300 );
    guy.set_thirst( 0 );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );
    REQUIRE( guy.has_calorie_deficit() );

    SECTION( "NPC on eat_food goal: auto-eats harvested food" ) {
        guy.set_committed_goal( "eat_food" );
        guy.set_moves( 200 );
        const int moves_before = guy.get_moves();
        iexamine_helper::handle_harvest( guy, itype_sandwich_cheese_grilled, false );

        // Should have eaten (moves charged).
        CHECK( guy.get_moves() < moves_before );
    }

    SECTION( "NPC on drink_water goal: auto-eats harvested food" ) {
        guy.set_committed_goal( "drink_water" );
        guy.set_moves( 200 );
        const int moves_before = guy.get_moves();
        iexamine_helper::handle_harvest( guy, itype_sandwich_cheese_grilled, false );

        CHECK( guy.get_moves() < moves_before );
    }

    SECTION( "NPC on generic work: food goes to inventory, not eaten" ) {
        // No committed goal (generic harvest job).
        guy.set_committed_goal( "" );
        guy.set_moves( 200 );
        const int moves_before = guy.get_moves();
        iexamine_helper::handle_harvest( guy, itype_sandwich_cheese_grilled, false );

        // Should NOT have eaten (no move cost).
        CHECK( guy.get_moves() == moves_before );
    }

    SECTION( "NPC committed to camp_work: food goes to inventory" ) {
        guy.set_committed_goal( "camp_work" );
        guy.set_moves( 200 );
        const int moves_before = guy.get_moves();
        iexamine_helper::handle_harvest( guy, itype_sandwich_cheese_grilled, false );

        CHECK( guy.get_moves() == moves_before );
    }
}

// Cold guarding NPCs should use the warmth executor (including fire
// starting) instead of only the legacy wear_warmest_item path.
TEST_CASE( "npc_guard_uses_warmth_executor", "[npc][needs][warmth][guard]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    map &here = get_map();
    // Player far away so the NPC doesn't try to follow.
    get_player_character().setpos( here, tripoint_bub_ms{ 50, 55, 0 } );
    get_weather().forced_temperature = 20_C;
    // On-shift for NC_ROBOFAC_INTERCOM_DAY (shift is 6:00-18:00).
    calendar::turn = calendar::turn_zero + 10_hours;

    npc &guy = spawn_scheduled_npc( { 50, 50 }, NC_ROBOFAC_INTERCOM_DAY );
    guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
    guy.set_all_parts_temp_cur( BODYTEMP_VERY_COLD );
    guy.set_sleepiness( 0 );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );
    guy.regen_ai_cache();

    REQUIRE( guy.guard_pos.has_value() );
    REQUIRE( *guy.guard_pos == guy.pos_abs() );

    here.build_map_cache( 0 );

    SECTION( "cold guard with lighter starts fire" ) {
        guy.i_add( tool_with_ammo( itype_lighter, 20 ) );
        guy.i_add( item( itype_2x4 ) );

        // Verify no warm clothing or shelter available.
        REQUIRE( guy.find_nearby_warm_clothing().empty() );
        REQUIRE( guy.find_nearby_shelters().empty() );

        // Run a move cycle. The guard path should use the warmth executor.
        guy.set_moves( 100 );
        guy.move();

        // Check that fire was started or fire-starting activity assigned.
        bool has_fire = false;
        for( const tripoint_bub_ms &p : here.points_in_radius( guy.pos_bub(), 1 ) ) {
            if( here.get_field( p, fd_fire ) ) {
                has_fire = true;
                break;
            }
        }
        CHECK( ( has_fire || guy.activity ) );
    }
}

// clear_committed_goal() must remove effect_lying_down when clearing
// a go_to_sleep commitment. Otherwise the effect handler keeps trying
// to put the NPC to sleep after the goal is gone.
TEST_CASE( "npc_clear_goal_removes_sleep_effects", "[npc][needs][sleep]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 55, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 55000 );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( sleepiness_levels::MASSIVE_SLEEPINESS );

    SECTION( "clear_committed_goal removes effect_lying_down" ) {
        guy.set_committed_goal( "go_to_sleep" );
        guy.add_effect( effect_lying_down, 30_minutes, false, 1, false, true );
        REQUIRE( guy.has_effect( effect_lying_down ) );

        guy.clear_committed_goal();

        CHECK_FALSE( guy.has_effect( effect_lying_down ) );
        CHECK( guy.get_committed_goal().empty() );
    }

    SECTION( "stop_guard clears sleep effects via clear_committed_goal" ) {
        guy.set_committed_goal( "go_to_sleep" );
        guy.add_effect( effect_lying_down, 30_minutes, false, 1, false, true );
        REQUIRE( guy.has_effect( effect_lying_down ) );

        talk_function::stop_guard( guy );

        CHECK_FALSE( guy.has_effect( effect_lying_down ) );
        CHECK( guy.get_committed_goal().empty() );
    }

    SECTION( "non-sleep goal does not touch effects" ) {
        guy.set_committed_goal( "eat_food" );
        // Unrelated effect should not be removed.
        guy.add_effect( effect_lying_down, 30_minutes, false, 1, false, true );
        REQUIRE( guy.has_effect( effect_lying_down ) );

        guy.clear_committed_goal();

        CHECK( guy.has_effect( effect_lying_down ) );
    }
}

// Dialogue transitions must cancel in-progress self-care activities.
// The forage/harvest early return in npc::move() bypasses BT
// re-evaluation, so a new order would be ignored until the activity
// finishes unless clear_committed_goal() also cancels the activity.
TEST_CASE( "npc_clear_goal_cancels_self_care_activities", "[npc][needs]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 55, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 55000 );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );

    map &here = get_map();

    SECTION( "forage activity cancelled on goal clear" ) {
        guy.set_committed_goal( "eat_food" );
        guy.assign_activity( forage_activity_actor( 1000 ) );
        REQUIRE( guy.activity );
        REQUIRE( guy.activity.id() == ACT_FORAGE );

        guy.clear_committed_goal();

        CHECK_FALSE( guy.activity );
    }

    SECTION( "harvest activity cancelled on goal clear" ) {
        guy.set_committed_goal( "eat_food" );
        // Autumn so t_tree_apple is harvestable.
        set_time( calendar::turn_zero + 2 * calendar::season_length() + 12_hours );
        const tripoint_bub_ms tree = guy.pos_bub() + point::east;
        here.ter_set( tree, ter_t_tree_apple );
        here.build_map_cache( 0 );
        REQUIRE( here.is_harvestable( tree ) );
        guy.assign_activity( harvest_activity_actor( tree, false ) );
        REQUIRE( guy.activity );
        REQUIRE( guy.activity.id() == ACT_HARVEST );

        guy.clear_committed_goal();

        CHECK_FALSE( guy.activity );
    }

    SECTION( "fire-starting activity cancelled on goal clear" ) {
        guy.set_committed_goal( "seek_warmth" );
        // Build a fire_start activity. The actor holds an item_location
        // into inventory; cancel_activity must handle cleanup.
        item &lighter = *guy.i_add( tool_with_ammo( itype_lighter, 20 ) );
        item_location tool_loc( guy, &lighter );
        const tripoint_bub_ms fire_pos = guy.pos_bub() + point::east;
        guy.assign_activity( fire_start_activity_actor(
                                 here.get_abs( fire_pos ), tool_loc, 5, 500 ) );
        REQUIRE( guy.activity );
        REQUIRE( guy.activity.id() == ACT_START_FIRE );

        guy.clear_committed_goal();

        CHECK_FALSE( guy.activity );
    }

    SECTION( "unrelated activity preserved on goal clear" ) {
        guy.set_committed_goal( "eat_food" );
        guy.assign_activity( wait_activity_actor( 10_minutes ) );
        REQUIRE( guy.activity );

        guy.clear_committed_goal();

        CHECK( guy.activity );
    }
}

// Indoor warmth hold must have a timeout. Without one, an NPC indoors
// on a cold tile that never warms them will hold forever, suppressing
// follow/duty indefinitely.
TEST_CASE( "npc_warmth_indoor_hold_timeout", "[npc][needs][warmth]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_player_character().camps.clear();
    get_player_character().setpos( get_map(), tripoint_bub_ms{ 50, 55, 0 } );
    get_weather().forced_temperature = 20_C;
    set_time_to_day();

    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_stored_kcal( 55000 );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
    guy.set_all_parts_temp_cur( BODYTEMP_VERY_COLD );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );

    map &here = get_map();

    // Put the NPC indoors with no warmth sources.
    here.ter_set( guy.pos_bub(), ter_t_floor );
    here.build_map_cache( 0 );

    SECTION( "indoor hold times out after threshold" ) {
        // Precondition: NPC is indoors, cold, no warmth candidates.
        REQUIRE( here.has_flag( ter_furn_flag::TFLAG_INDOORS, guy.pos_bub() ) );
        behavior::character_oracle_t oracle( &guy );
        REQUIRE( oracle.needs_warmth_badly( "" ) == behavior::status_t::running );
        REQUIRE( guy.find_warmth_candidates().empty() );

        // First call should return holding.
        guy.set_moves( 100 );
        npc::need_result first = guy.execute_need_goal( "seek_warmth" );
        REQUIRE( first == npc::need_result::holding );
        REQUIRE( guy.get_warmth_indoor_hold_turns() == 1 );

        // Run the executor repeatedly. It should hold for a while, then
        // give up and return impossible (threshold is 60 turns).
        npc::need_result last = first;
        int hold_count = 1;
        for( int turn = 1; turn < 80; ++turn ) {
            guy.set_moves( 100 );
            last = guy.execute_need_goal( "seek_warmth" );
            if( last == npc::need_result::holding ) {
                hold_count++;
            }
            if( last == npc::need_result::impossible ) {
                break;
            }
        }
        CHECK( hold_count > 0 );
        CHECK( last == npc::need_result::impossible );
    }

    SECTION( "predicate returns failure after indoor hold timeout" ) {
        // Exhaust the indoor hold timeout (60 turns).
        for( int turn = 0; turn < 80; ++turn ) {
            guy.set_moves( 100 );
            npc::need_result result = guy.execute_need_goal( "seek_warmth" );
            if( result == npc::need_result::impossible ) {
                break;
            }
        }

        // The predicate should now return failure for the indoor case,
        // preventing the BT from immediately re-selecting seek_warmth.
        behavior::character_oracle_t oracle( &guy );
        REQUIRE( oracle.needs_warmth_badly( "" ) == behavior::status_t::running );
        CHECK( oracle.can_obtain_warmth( "" ) == behavior::status_t::failure );
    }

    SECTION( "real candidate resets indoor hold stagnation" ) {
        // Accumulate some indoor hold turns.
        for( int turn = 0; turn < 10; ++turn ) {
            guy.set_moves( 100 );
            guy.execute_need_goal( "seek_warmth" );
        }

        // Place clothing. The executor should pursue it, resetting
        // the stagnation counter.
        const tripoint_bub_ms clothing = guy.pos_bub() + tripoint( 3, 0, 0 );
        here.add_item_or_charges( clothing, item( itype_sweater ) );
        here.build_map_cache( 0 );

        guy.set_moves( 100 );
        npc::need_result result = guy.execute_need_goal( "seek_warmth" );
        CHECK( result == npc::need_result::progressed );

        // Counter should be reset. Remove the clothing and verify
        // indoor hold can run again from 0.
        here.i_clear( clothing );
        guy.clear_need_state( npc::need_goal_id::seek_warmth );
        here.build_map_cache( 0 );

        int hold_count = 0;
        for( int turn = 0; turn < 5; ++turn ) {
            guy.set_moves( 100 );
            npc::need_result r = guy.execute_need_goal( "seek_warmth" );
            if( r == npc::need_result::holding ) {
                hold_count++;
            }
        }
        CHECK( hold_count > 0 );
    }
}
