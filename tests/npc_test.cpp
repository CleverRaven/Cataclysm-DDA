#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <unordered_set>
#include <string>
#include <utility>
#include <vector>

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

static const efftype_id effect_bouldering( "bouldering" );
static const efftype_id effect_catch_up( "catch_up" );
static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_meth( "meth" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_wet( "wet" );

static const faction_id faction_robofac( "robofac" );
static const faction_id faction_your_followers( "your_followers" );

static const furn_str_id furn_f_bed( "f_bed" );
static const furn_str_id furn_f_locker( "f_locker" );
static const furn_str_id furn_f_makeshift_bed( "f_makeshift_bed" );

static const item_group_id Item_spawn_data_SUS_trash_forest_manmade( "SUS_trash_forest_manmade" );
static const item_group_id Item_spawn_data_test_NPC_guns( "test_NPC_guns" );

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

static const ter_str_id ter_t_concrete_wall( "t_concrete_wall" );
static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_ponywall( "t_ponywall" );
static const ter_str_id ter_t_swater_sh( "t_swater_sh" );
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

        guy.i_add( item( itype_lighter ) );
        guy.i_add( item( itype_2x4 ) );
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

    SECTION( "ally without allow_pick_up returns empty" ) {
        guy.set_fac( faction_your_followers );
        guy.set_attitude( NPCATT_FOLLOW );
        REQUIRE( guy.is_player_ally() );
        guy.rules.clear_flag( ally_rule::allow_pick_up );
        here.add_item_or_charges( adj, item( itype_sandwich_cheese_grilled ) );
        CHECK( guy.find_nearby_food().empty() );
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

    SECTION( "ally without allow_pick_up returns empty" ) {
        guy.set_fac( faction_your_followers );
        guy.set_attitude( NPCATT_FOLLOW );
        REQUIRE( guy.is_player_ally() );
        guy.rules.clear_flag( ally_rule::allow_pick_up );
        here.add_item_or_charges( adj, item( itype_sweater ) );
        CHECK( guy.find_nearby_warm_clothing().empty() );
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

        REQUIRE( guy.has_activity() );
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
