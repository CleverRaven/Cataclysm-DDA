#include "catch/catch.hpp"

#include "cata_utility.h"
#include "game.h"
#include "itype.h"
#include "map.h"
#include "map_iterator.h"
#include "player.h"
#include "string_id.h"
#include "test_statistics.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_range.h"
#include "vpart_reference.h"

#include <sstream>

const efftype_id effect_blind( "blind" );

struct coll_damage_data {
    std::string proto;
    int pre_engine_w = 0;
    int post_engine_w = 0;
    int pre_safe_v = 0;
    int post_safe_v = 0;
    int pre_max_v = 0;
    int post_max_v = 0;
    size_t pre_part_cnt = 0;
    size_t post_part_cnt = 0;
    size_t post_no_dmg_part_cnt = 0;
    size_t post_min_dmg_part_cnt = 0;
    size_t post_low_dmg_part_cnt = 0;
    size_t post_med_dmg_part_cnt = 0;
    size_t post_high_dmg_part_cnt = 0;
    size_t post_ext_dmg_part_cnt = 0;
    size_t post_all_dmg_part_cnt = 0;
    ter_id pre_terrain;
    ter_id post_terrain;
    furn_id pre_furn;
    furn_id post_furn;
    int post_distance = 0;
};

static void clear_game( const ter_id &terrain )
{
    // Set to turn 0 to prevent solars from producing power
    calendar::turn = 0;
    for( monster &critter : g->all_monsters() ) {
        g->remove_zombie( critter );
    }

    g->unload_npcs();

    // Move player somewhere safe
    g->u.setpos( tripoint( 30, 30, 0 ) );
    // Blind the player to avoid needless drawing-related overhead
    g->u.add_effect( effect_blind, 1_turns, num_bp, true );

    for( const tripoint &p : g->m.points_in_rectangle( tripoint( 0, 0, 0 ),
            tripoint( MAPSIZE * SEEX, MAPSIZE * SEEY, 0 ) ) ) {
        g->m.furn_set( p, furn_id( "f_null" ) );
        g->m.ter_set( p, terrain );
        g->m.trap_set( p, trap_id( "tr_null" ) );
        g->m.i_clear( p );
    }

    for( wrapped_vehicle &veh : g->m.get_vehicles( tripoint( 0, 0, 0 ), tripoint( MAPSIZE * SEEX,
            MAPSIZE * SEEY, 0 ) ) ) {
        g->m.destroy_vehicle( veh.v );
    }

    g->m.build_map_cache( 0, true );
}

// Returns how much fuel did it provide
// But contains only fuels actually used by engines
static std::map<itype_id, long> set_vehicle_fuel( vehicle &v, float veh_fuel_mult )
{
    // First we need to find the fuels to set
    // That is, fuels actually used by some engine
    std::set<itype_id> actually_used;
    for( const vpart_reference vp : v.get_all_parts() ) {
        vehicle_part &pt = vp.part();
        if( pt.is_engine() ) {
            actually_used.insert( pt.info().fuel_type );
            pt.enabled = true;
        } else {
            // Disable all parts that use up power or electric cars become non-deterministic
            pt.enabled = false;
        }
    }

    // We ignore battery when setting fuel because it uses designated "tanks"
    actually_used.erase( "battery" );

    // Currently only one liquid fuel supported
    REQUIRE( actually_used.size() <= 1 );
    itype_id liquid_fuel = "null";
    for( const auto &ft : actually_used ) {
        if( item::find_type( ft )->phase == LIQUID ) {
            liquid_fuel = ft;
            break;
        }
    }

    // Set fuel to a given percentage
    // Batteries are special cased because they aren't liquid fuel
    std::map<itype_id, long> ret;
    for( const vpart_reference vp : v.get_all_parts() ) {
        vehicle_part &pt = vp.part();

        if( pt.is_battery() ) {
            pt.ammo_set( "battery", pt.ammo_capacity() * veh_fuel_mult );
            ret[ "battery" ] += pt.ammo_capacity() * veh_fuel_mult;
        } else if( pt.is_tank() && liquid_fuel != "null" ) {
            float qty = pt.ammo_capacity() * veh_fuel_mult;
            qty *= std::max( item::find_type( liquid_fuel )->stack_size, 1 );
            qty /= to_milliliter( units::legacy_volume_factor );
            pt.ammo_set( liquid_fuel, qty );
            ret[ liquid_fuel ] += qty;
        } else {
            pt.ammo_unset();
        }
    }

    // We re-add battery because we want it accounted for, just not in the section above
    actually_used.insert( "battery" );
    for( auto iter = ret.begin(); iter != ret.end(); ) {
        if( iter->second <= 0 || actually_used.count( iter->first ) == 0 ) {
            iter = ret.erase( iter );
        } else {
            ++iter;
        }
    }
    return ret;
}

// Returns the lowest percentage of fuel left
// ie. 1 means no fuel was used, 0 means at least one dry tank
static float fuel_percentage_left( vehicle &v, const std::map<itype_id, long> &started_with )
{
    std::map<itype_id, long> fuel_amount;
    std::set<itype_id> consumed_fuels;
    for( const vpart_reference vp : v.get_all_parts() ) {
        vehicle_part &pt = vp.part();

        if( ( pt.is_battery() || pt.is_reactor() || pt.is_tank() ) &&
            pt.ammo_current() != "null" ) {
            fuel_amount[ pt.ammo_current() ] += pt.ammo_remaining();
        }

        if( pt.is_engine() && pt.info().fuel_type != "null" ) {
            consumed_fuels.insert( pt.info().fuel_type );
        }
    }

    float left = 1.0f;
    for( const auto &type : consumed_fuels ) {
        const auto iter = started_with.find( type );
        // Weird - we started without this fuel
        float fuel_amt_at_start = iter != started_with.end() ? iter->second : 0.0f;
        REQUIRE( fuel_amt_at_start != 0.0f );
        left = std::min( left, static_cast<float>( fuel_amount[type] ) / fuel_amt_at_start );
    }

    return left;
}

bool smash_one( const std::string &veh_type, const std::string &terrain_id, int target_velocity,
                coll_damage_data &coll_data )
{
    vproto_id veh_id = vproto_id( veh_type );
    const ter_id terrain = ter_id( terrain_id );
    const ter_id pavement = ter_id( "t_pavement" );

    clear_game( pavement );

    const tripoint starting_point( 60, 50, 0 );

    vehicle *veh_ptr = g->m.add_vehicle( veh_id, starting_point, 0, 0, 0 );

    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return false;
    }
    vehicle &veh = *veh_ptr;

    int target_offset = static_cast<int>( 1.25 * target_velocity / vehicles::vmiph_per_tile );
    const tripoint collision_point = g->m.getabs( veh.global_pos3() ) + tripoint( target_offset, 0, 0 );
    g->m.ter_set( collision_point, terrain );

    // Remove all items from cargo to normalize weight.
    for( const vpart_reference vp : veh.get_all_parts() ) {
        while( veh.remove_item( vp.part_index(), 0 ) );
        vp.part().ammo_consume( vp.part().ammo_remaining(), vp.pos() );
    }
    const auto &starting_fuel = set_vehicle_fuel( veh, 0.1 );
    const float starting_fuel_per = fuel_percentage_left( veh, starting_fuel );
    REQUIRE( std::abs( starting_fuel_per - 1.0f ) < 0.001f );

    target_velocity = std::min( veh.max_velocity( false ), target_velocity );
    veh.tags.insert( "IN_CONTROL_OVERRIDE" );
    veh.engine_on = true;
    veh.cruise_velocity = target_velocity;
    veh.velocity = target_velocity;
    int cycles_left = 5;
    CHECK( veh.safe_velocity() > 0 );

    coll_data.proto = veh_type;
    coll_data.pre_engine_w = veh.total_power_w( false );
    coll_data.pre_max_v = veh.max_velocity( false );
    coll_data.pre_safe_v = veh.safe_velocity( false );
    coll_data.pre_part_cnt = veh.parts.size();
    coll_data.pre_terrain = g->m.ter( collision_point );
    coll_data.pre_furn = g->m.furn( collision_point );
    bool no_collide = true;
    while( veh.engine_on && veh.safe_velocity() > 0 && cycles_left > 0 ) {
        g->m.vehmove();
        veh.idle( true );
        // probably crashed if the vehicle slowed down
        if( no_collide && veh.velocity != target_velocity ) {
            no_collide = false;
            coll_data.post_engine_w = veh.total_power_w( false );
            coll_data.post_max_v = veh.max_velocity( false );
            coll_data.post_safe_v = veh.safe_velocity( false );
            coll_data.post_part_cnt = veh.parts.size();
            coll_data.post_terrain = g->m.ter( collision_point );
            coll_data.post_furn = g->m.furn( collision_point );
            for( const vpart_reference vp : veh.get_all_parts() ) {
                if( vp.part().is_broken() ) {
                    coll_data.post_all_dmg_part_cnt += 1;
                } else if( vp.part().damage_percent() > 0.75 ) {
                    coll_data.post_ext_dmg_part_cnt += 1;
                } else if( vp.part().damage_percent() > 0.5 ) {
                    coll_data.post_high_dmg_part_cnt += 1;
                } else if( vp.part().damage_percent() > 0.30 ) {
                    coll_data.post_med_dmg_part_cnt += 1;
                } else if( vp.part().damage_percent() > 0.15 ) {
                    coll_data.post_low_dmg_part_cnt += 1;
                } else if( vp.part().damage_percent() >= 0.001 ) {
                    coll_data.post_min_dmg_part_cnt += 1;
                } else {
                    coll_data.post_no_dmg_part_cnt += 1;
                }
            }
        }
        cycles_left -= 1;
    }
    coll_data.post_distance = rl_dist( g->m.getabs( starting_point ), veh.global_pos3() );
    return true;
}

/* run a vehicle into something at some velocity and collect results
 * expected final velocity
 * expected distance
 * expected number of parts
 * % of the time the target is destroyed
 * cnt of parts that are undamaged, cnt of parts at 14% or less damage,
 * cnt of parts that are at 29% or less damage, cnt of parts that are at 49% or less damage
 * cnt of parts that are at 74% or less damage,
 * cnt of of parts at 75% or more damage, cnt of parts that are destroyed
 * all values are fixed digit with 2 significant digits
 */
void smash_veh( const std::string &veh_type, const std::string &ter_type, int target_velocity,
                int expected_velocity, int expected_distance, int expected_cnt,
                int ter_destroyed_cnt, int no_dmg_cnt, int min_dmg_cnt, int low_dmg_cnt,
                int med_dmg_cnt, int high_dmg_cnt, int ext_dmg_cnt, int all_dmg_cnt )
{
    const int loops = 40;
    const int sigfigs = 100;
    printf( "\n%s into a %s at %3.2f mph\n", veh_type.c_str(), ter_type.c_str(),
            target_velocity / 100.0 );
    coll_damage_data coll_data;
    size_t terrain_wrecked = 0;
    size_t furn_wrecked = 0;

    for( size_t i = 0; i < loops; i++ ) {
        coll_damage_data crash_data;
        if( !smash_one( veh_type, ter_type, target_velocity, crash_data ) ) {
            continue;
        }
        coll_data.pre_engine_w = crash_data.pre_engine_w;
        coll_data.post_engine_w += crash_data.post_engine_w;
        coll_data.pre_safe_v = crash_data.pre_safe_v;
        coll_data.post_safe_v += crash_data.post_safe_v;
        coll_data.pre_max_v = crash_data.pre_max_v;
        coll_data.post_max_v += crash_data.post_max_v;
        coll_data.pre_part_cnt = crash_data.pre_part_cnt;
        coll_data.post_part_cnt += crash_data.post_part_cnt;
        terrain_wrecked += crash_data.pre_terrain != crash_data.post_terrain;
        furn_wrecked += crash_data.pre_furn != crash_data.post_furn;
        coll_data.post_all_dmg_part_cnt += crash_data.post_all_dmg_part_cnt;
        coll_data.post_ext_dmg_part_cnt += crash_data.post_ext_dmg_part_cnt;
        coll_data.post_high_dmg_part_cnt += crash_data.post_high_dmg_part_cnt;
        coll_data.post_med_dmg_part_cnt += crash_data.post_med_dmg_part_cnt;
        coll_data.post_low_dmg_part_cnt += crash_data.post_low_dmg_part_cnt;
        coll_data.post_min_dmg_part_cnt += crash_data.post_min_dmg_part_cnt;
        coll_data.post_no_dmg_part_cnt += crash_data.post_no_dmg_part_cnt;
        coll_data.post_distance += crash_data.post_distance;
    }
    CHECK( coll_data.post_all_dmg_part_cnt + coll_data.post_ext_dmg_part_cnt +
           coll_data.post_high_dmg_part_cnt + coll_data.post_med_dmg_part_cnt +
           coll_data.post_low_dmg_part_cnt + coll_data.post_min_dmg_part_cnt +
           coll_data.post_no_dmg_part_cnt == coll_data.post_part_cnt );
    bool valid = true;
    const auto range_compare = [&]( std::string test_type, int value, int expected,
    int expected_low, int expected_high ) {
        int adj_value = ( value * sigfigs ) / loops;
        int expected_min = std::max( static_cast<int>( 0.75 * ( expected - expected_low ) ), 0 );
        int expected_max = 1.25 * ( expected_high + expected );
        if( ( expected_max < adj_value ) || ( expected_min > adj_value ) ) {
            printf( "\ttesting %s: %d is %s> %d; %d is %s< %d\n", test_type.c_str(),
                    adj_value, expected_min <= adj_value ? "" : "not ", expected_min,
                    adj_value, expected_max >= adj_value ? "" : "not ", expected_max );
        }
        return ( expected_max >= adj_value ) && ( expected_min <= adj_value );
    };
    valid &= range_compare( "velocity", coll_data.post_max_v, expected_velocity, 0, 0 );
    valid &= range_compare( "distance", coll_data.post_distance, expected_distance, 0, 0 );
    valid &= range_compare( "part count", coll_data.post_part_cnt, expected_cnt, 0, 0 );
    valid &= range_compare( "terrain destroyed", terrain_wrecked, ter_destroyed_cnt, 0, 0 );
    valid &= range_compare( "undamaged parts", coll_data.post_no_dmg_part_cnt, no_dmg_cnt,
                            0, min_dmg_cnt );
    valid &= range_compare( "minimal damage parts", coll_data.post_min_dmg_part_cnt,
                            min_dmg_cnt, no_dmg_cnt, low_dmg_cnt );
    valid &= range_compare( "low damage parts", coll_data.post_low_dmg_part_cnt,
                            low_dmg_cnt, min_dmg_cnt, med_dmg_cnt );
    valid &= range_compare( "medium damage parts", coll_data.post_med_dmg_part_cnt,
                            med_dmg_cnt, low_dmg_cnt, high_dmg_cnt );
    valid &= range_compare( "high damage parts", coll_data.post_high_dmg_part_cnt,
                            high_dmg_cnt, med_dmg_cnt, ext_dmg_cnt );
    valid &= range_compare( "extreme damage parts", coll_data.post_ext_dmg_part_cnt,
                            ext_dmg_cnt, high_dmg_cnt, all_dmg_cnt );
    valid &= range_compare( "destroyed parts", coll_data.post_all_dmg_part_cnt,
                            all_dmg_cnt, ext_dmg_cnt, 0 );
    CHECK( valid );
    if( !valid ) {
        printf( "    smash_veh( \"%s\", \"%s\", %5d, %5d, %3d, %5zu,", veh_type.c_str(),
                ter_type.c_str(), target_velocity, ( coll_data.post_max_v * sigfigs ) / loops,
                ( coll_data.post_distance * sigfigs ) / loops,
                ( coll_data.post_part_cnt * sigfigs ) / loops );
        printf( "\n\t       %zu, %5zu, %5zu, %5zu, %5zu, %5zu, %5zu, %5zu );\n",
                ( terrain_wrecked * sigfigs ) / loops,
                ( coll_data.post_no_dmg_part_cnt * sigfigs ) / loops,
                ( coll_data.post_min_dmg_part_cnt * sigfigs ) / loops,
                ( coll_data.post_low_dmg_part_cnt * sigfigs ) / loops,
                ( coll_data.post_med_dmg_part_cnt * sigfigs ) / loops,
                ( coll_data.post_high_dmg_part_cnt * sigfigs ) / loops,
                ( coll_data.post_ext_dmg_part_cnt * sigfigs ) / loops,
                ( coll_data.post_all_dmg_part_cnt * sigfigs ) / loops );
    }
}

// vehicle, target, impact speed, post max speed, travel distance, part count, % terrain destroyed
// # undamaged parts, # minimal damage parts, # low damage parts, # med damage parts, # high
// damage parts, # extreme damage parts, # destroyed parts
TEST_CASE( "vehicle_crash" )
{
    //            v         T       impact  max   dist  part count
    //          t%    no%  min%  low%   med%  high%    ext%   destroy%
    smash_veh( "beetle", "t_tree",  2000, 902300, 200,  5000,
               0,  4900,     0,   100,     0,     0,     0,     0 );
    smash_veh( "beetle", "t_tree",  4000, 902300, 3265,  5000,
               100,  4110,   725,    65,     0,    75,    25,     0 );
    smash_veh( "beetle", "t_tree",  6000, 902300, 5825,  5000,
               100,  3700,   835,   365,     0,     0,    45,    55 );
    smash_veh( "beetle", "t_tree",  8000, 902300, 6825,  5000,
               100,  3475,   900,   385,   140,     0,     0,   100 );
    smash_veh( "beetle", "t_tree", 10000, 886825, 6835,  5000,
               100,  3360,   780,   440,   320,     0,     0,   100 );
    smash_veh( "car", "t_tree",  2000, 989760, 100,  7500,
               0,  7290,   110,    32,    45,    22,     0,     0 );
    smash_veh( "car", "t_tree",  4000, 857897, 3277,  7500,
               100,  6490,   802,   107,     0,    12,    30,    57 );
    smash_veh( "car", "t_tree",  6000, 807720, 5635,  7500,
               100,  6215,   860,   305,    20,     0,    10,    90 );
    smash_veh( "car", "t_tree",  8000, 756240, 5905,  7500,
               100,  5885,   990,   365,   160,     0,     0,   100 );
    smash_veh( "car", "t_tree", 10000, 838685, 6390,  7500,
               100,  5355,  1065,   515,   385,    80,     0,   100 );
    smash_veh( "suv", "t_tree",  2000, 1169830, 100,  8600,
               0,  8317,   182,    27,    55,     0,    17,     0 );
    smash_veh( "suv", "t_tree",  4000, 1126750, 3647,  8600,
               100,  7560,   807,   132,     0,    30,    12,    57 );
    smash_veh( "suv", "t_tree",  6000, 980840, 5960,  8600,
               100,  7320,   885,   280,    15,     0,    30,    70 );
    smash_veh( "suv", "t_tree",  8000, 827875, 5740,  8600,
               100,  6925,   995,   410,   170,     0,     0,   100 );
    smash_veh( "suv", "t_tree", 10000, 1123735, 6595,  8600,
               100,  6540,   970,   510,   375,   105,     0,   100 );
    smash_veh( "schoolbus", "t_tree",  2000, 1000200, 200, 15100,
               0, 14635,   365,     0,    50,    20,     0,    30 );
    smash_veh( "schoolbus", "t_tree",  4000, 1000200, 3817, 15100,
               100, 14050,   782,   167,     0,    17,    20,    62 );
    smash_veh( "schoolbus", "t_tree",  6000, 1000200, 6350, 15100,
               100, 13872,   855,   265,     7,     0,    27,    72 );
    smash_veh( "schoolbus", "t_tree",  8000, 1000200, 6425, 15100,
               100, 13012,  1402,   420,   165,     0,     0,   100 );
    smash_veh( "schoolbus", "t_tree", 10000, 984620, 6457, 15100,
               100, 12410,  1585,   535,   360,   110,     0,   100 );
    smash_veh( "beetle", "t_strconc_wall",  2000, 902300, 200,  5000,
               0,  4677,   222,   100,     0,     0,     0,     0 );
    smash_veh( "beetle", "t_strconc_wall",  4000, 902300, 2150,  5000,
               97,  3617,   860,   422,     0,     0,     7,    92 );
    smash_veh( "beetle", "t_strconc_wall",  6000, 902300, 3545,  5000,
               100,  3400,   635,   462,   350,    52,     0,   100 );
    smash_veh( "beetle", "t_strconc_wall",  8000, 902300, 5722,  5000,
               100,  3400,   237,   637,   300,   302,    22,   100 );
    smash_veh( "beetle", "t_strconc_wall", 10000, 882305, 6205,  5000,
               100,  3357,   130,   567,   387,   252,   205,   100 );
    smash_veh( "car", "t_strconc_wall",  2000, 990900, 100,  7500,
               0,  7012,   387,     0,    55,    20,     0,    25 );
    smash_veh( "car", "t_strconc_wall",  4000, 856920, 1840,  7500,
               100,  6012,   905,   447,    35,     0,     0,   100 );
    smash_veh( "car", "t_strconc_wall",  6000, 882375, 4190,  7500,
               100,  5455,  1010,   512,   340,    70,     0,   112 );
    smash_veh( "car", "t_strconc_wall",  8000, 832035, 5735,  7500,
               100,  5115,   935,   652,   302,   292,    72,   130 );
    smash_veh( "car", "t_strconc_wall", 10000, 737435, 6030,  7500,
               100,  5005,   622,   647,   467,   180,   222,   355 );
    smash_veh( "suv", "t_strconc_wall",  2000, 1133280, 100,  8600,
               0,  8065,   435,     0,    47,    35,     0,    17 );
    smash_veh( "suv", "t_strconc_wall",  4000, 1005247, 2477,  8600,
               100,  7047,   962,   427,    60,     0,     0,   102 );
    smash_veh( "suv", "t_strconc_wall",  6000, 1005350, 3850,  8600,
               100,  6410,  1035,   557,   345,   105,     0,   147 );
    smash_veh( "suv", "t_strconc_wall",  8000, 1093677, 6132,  8600,
               100,  5947,  1202,   550,   340,   315,    92,   152 );
    smash_veh( "suv", "t_strconc_wall", 10000, 827772, 5925,  8600,
               100,  5735,   970,   602,   435,   267,   165,   425 );
    smash_veh( "schoolbus", "t_strconc_wall",  2000, 1000200, 200, 15100,
               0, 14380,   620,     0,    42,     0,     0,    57 );
    smash_veh( "schoolbus", "t_strconc_wall",  4000, 1000200, 2945, 15100,
               100, 13002,  1350,   412,   235,     0,     2,    97 );
    smash_veh( "schoolbus", "t_strconc_wall",  6000, 1000200, 5700, 15100,
               100, 12422,  1532,   577,   365,    95,     7,   100 );
    smash_veh( "schoolbus", "t_strconc_wall",  8000, 1000200, 6470, 15100,
               100, 11640,  1670,   855,   362,   275,   167,   130 );
    smash_veh( "schoolbus", "t_strconc_wall", 10000, 985110, 6585, 15100,
               100, 10665,  1980,  1022,   550,   237,   200,   445 );
}
