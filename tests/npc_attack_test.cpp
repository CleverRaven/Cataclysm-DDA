#include "catch/catch.hpp"

#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "npc.h"
#include "npc_attack.h"
#include "npc_class.h"
#include "options_helpers.h"
#include "player_helpers.h"

static const mtype_id mon_zombie( "mon_zombie" );

static constexpr point main_npc_start{ 50, 50 };
static constexpr tripoint main_npc_start_tripoint{ main_npc_start, 0 };

namespace npc_attack_setup
{
static void spawn_main_npc()
{
    get_player_character().setpos( { main_npc_start, -1 } );
    const string_id<npc_template> blank_template( "test_talker" );
    REQUIRE( g->critter_at<Creature>( main_npc_start_tripoint ) == nullptr );
    const character_id model_id = get_map().place_npc( main_npc_start, blank_template );

    npc &model_npc = *g->find_npc( model_id );
    clear_character( model_npc );
    model_npc.setpos( main_npc_start_tripoint );

    g->load_npcs();

    REQUIRE( g->critter_at<npc>( main_npc_start_tripoint ) != nullptr );
}

static void respawn_main_npc()
{
    npc *guy = g->critter_at<npc>( main_npc_start_tripoint );
    if( guy ) {
        guy->die( nullptr );
    }
    spawn_main_npc();
}

// will seg fault if the main npc wasn't spawned yet
static npc &get_main_npc()
{
    return *g->critter_at<npc>( main_npc_start_tripoint );
}

static void spawn_zombie_at_range( const int range )
{
    g->place_critter_at( mon_zombie, main_npc_start_tripoint + tripoint( range, 0, 0 ) );
}
} // namespace npc_attack_setup

struct npc_attack_melee_test_data {
    itype_id weapon_id;
    // base effectiveness against the zombie
    int base_effectiveness;
    // effectiveness skyrockets when the zombie will die in one hit
    int deathblow;
    // the effectiveness value if the zombie is not the main target
    int not_main_target;
    // the effectiveness value of a farther zombie if it's the target
    int reach_far_target;
    // the effectiveness value of a farther zombie if it's not the target but nearly dead
    int reach_far_deathblow;
    // effectiveness value of a farther zombie if it's nearly dead and is the target
    int reach_far_target_deathblow;
};

static void test_melee_attacks( const npc_attack_melee_test_data &data )
{
    CAPTURE( data.weapon_id.c_str() );
    clear_map_and_put_player_underground();
    clear_vehicles();
    scoped_weather_override sunny_weather( weather_type_id( "sunny" ) );
    npc_attack_setup::spawn_main_npc();
    npc_attack_setup::spawn_zombie_at_range( 1 );
    monster *zomble = g->critter_at<monster>( main_npc_start_tripoint + tripoint_east );
    npc_attack_setup::respawn_main_npc();
    npc &main_npc = npc_attack_setup::get_main_npc();
    item weapon( data.weapon_id );
    main_npc.weapon = weapon;
    const npc_attack_melee wep( main_npc.weapon );
    npc_attack_rating wep_effectiveness = wep.evaluate( main_npc, zomble );
    // base effectiveness against the zombie
    CHECK( *wep_effectiveness.value() == data.base_effectiveness );
    zomble->set_hp( 1 );
    wep_effectiveness = wep.evaluate( main_npc, zomble );
    // effectiveness skyrockets when the zombie will die in one hit
    CHECK( *wep_effectiveness.value() == data.deathblow );

    npc_attack_setup::spawn_zombie_at_range( 4 );
    monster *dist_target = g->critter_at<monster>( main_npc_start_tripoint + tripoint( 4, 0, 0 ) );
    wep_effectiveness = wep.evaluate( main_npc, dist_target );
    // even if the distant target is the npc's actual target,
    // the nearly dead zombie we actually can hit is the (only) choice
    CHECK( wep_effectiveness.target() == main_npc_start_tripoint + tripoint_east );
    // but the actual effectiveness value is lower because it is not the main target
    CHECK( *wep_effectiveness.value() == data.not_main_target );
    zomble->die( nullptr );
    wep_effectiveness = wep.evaluate( main_npc, dist_target );
    // we can't reach the only alive zombie, so there's no valid attack to make
    CHECK( !wep_effectiveness.value() );
    dist_target->die( nullptr );

}

static void test_reach_attacks( const npc_attack_melee_test_data &data )
{
    test_melee_attacks( data );

    npc &main_npc = npc_attack_setup::get_main_npc();
    const npc_attack_melee wep( main_npc.weapon );
    CAPTURE( data.weapon_id.c_str() );

    npc_attack_setup::spawn_zombie_at_range( 1 );
    npc_attack_setup::spawn_zombie_at_range( 2 );

    monster *mon_close = g->critter_at<monster>( main_npc_start_tripoint + tripoint_east );
    monster *mon_far = g->critter_at<monster>( main_npc_start_tripoint + tripoint( 2, 0, 0 ) );

    // if the farther zombie is our target, prefer it
    npc_attack_rating wep_effectiveness = wep.evaluate( main_npc, mon_far );
    CHECK( wep_effectiveness.target() == main_npc_start_tripoint + tripoint( 2, 0, 0 ) );
    CHECK( *wep_effectiveness.value() == data.reach_far_target );

    mon_far->set_hp( 1 );
    wep_effectiveness = wep.evaluate( main_npc, mon_close );
    // if the farther zombie is nearly dead but the close one is our target, we still want to fight the closer one
    CHECK( wep_effectiveness.target() == main_npc_start_tripoint + tripoint_east );
    CHECK( *wep_effectiveness.value() == data.reach_far_deathblow );

    mon_close->set_hp( 1 );
    wep_effectiveness = wep.evaluate( main_npc, mon_far );
    // if both zombies are nearly dead, the far target should still be preferred
    CHECK( wep_effectiveness.target() == main_npc_start_tripoint + tripoint( 2, 0, 0 ) );
    CHECK( *wep_effectiveness.value() == data.reach_far_target_deathblow );
}

struct npc_attack_gun_test_data {
    std::string weapon_id;
    // default gunmode against a zombie point-blank at MAX_RECOIL
    int point_blank_hip_shot_default;
    // default gunmode against a zombie point-blank at full aim
    int point_blank_full_aim_default;
};

static void test_gun_attacks( const npc_attack_gun_test_data &data )
{

    clear_map_and_put_player_underground();
    clear_vehicles();
    scoped_weather_override sunny_weather( weather_type_id( "sunny" ) );
    npc_attack_setup::spawn_main_npc();
    npc &main_npc = npc_attack_setup::get_main_npc();
    arm_shooter( main_npc, data.weapon_id );

    npc_attack_setup::spawn_zombie_at_range( 1 );
    monster *near_zombie = g->critter_at<monster>( main_npc_start_tripoint + tripoint_east );

    const npc_attack_gun gun_default( main_npc.weapon.gun_get_mode( gun_mode_id( "DEFAULT" ) ) );
    npc_attack_rating gun_effectiveness = gun_default.evaluate( main_npc, near_zombie );
    CHECK( *gun_effectiveness.value() == data.point_blank_hip_shot_default );
    main_npc.recoil = main_npc.aim_cap_from_volume( main_npc.weapon );
    gun_effectiveness = gun_default.evaluate( main_npc, near_zombie );
    CHECK( *gun_effectiveness.value() == data.point_blank_full_aim_default );
}

TEST_CASE( "Test NPC attack variants' potential", "[npc_attack]" )
{
    SECTION( "melee" ) {
        SECTION( "2x4" ) {
            const npc_attack_melee_test_data twobyfour{
                itype_id( "test_2x4" ),
                35, 71, 24,
                -1, -1, -1
            };
            test_melee_attacks( twobyfour );
        }
        SECTION( "pipe" ) {
            const npc_attack_melee_test_data pipe{
                itype_id( "test_pipe" ),
                68, 136, 45,
                -1, -1, -1
            };
            test_melee_attacks( pipe );
        }
        SECTION( "fire_ax" ) {
            const npc_attack_melee_test_data fire_ax{
                itype_id( "test_fire_ax" ),
                141, 283, 94,
                -1, -1, -1
            };
            test_melee_attacks( fire_ax );
        }
        SECTION( "clumsy_sword" ) {
            const npc_attack_melee_test_data clumsy_sword{
                itype_id( "test_clumsy_sword" ),
                227, 454, 151,
                -1, -1, -1
            };
            test_melee_attacks( clumsy_sword );
        }
        SECTION( "normal_sword" ) {
            const npc_attack_melee_test_data normal_sword{
                itype_id( "test_normal_sword" ),
                291, 582, 194,
                -1, -1, -1
            };
            test_melee_attacks( normal_sword );
        }
        SECTION( "balanced_sword" ) {
            const npc_attack_melee_test_data balanced_sword{
                itype_id( "test_balanced_sword" ),
                346, 692, 231,
                -1, -1, -1
            };
            test_melee_attacks( balanced_sword );
        }
        SECTION( "whip" ) {
            const npc_attack_melee_test_data whip{
                itype_id( "test_bullwhip" ),
                22, 43, 14,
                19, 22, 37
            };
            test_reach_attacks( whip );
        }
        SECTION( "glaive" ) {
            const npc_attack_melee_test_data glaive{
                itype_id( "test_glaive" ),
                192, 384, 128,
                189, 192, 378
            };
            test_reach_attacks( glaive );
        }
    }
    SECTION( "ranged" ) {
        SECTION( "glock" ) {
            const npc_attack_gun_test_data glock{
                "test_glock",
                192, 222
            };
            test_gun_attacks( glock );
        }
        SECTION( "longbow" ) {
            const npc_attack_gun_test_data bow{
                "test_compbow",
                210, 240
            };
            test_gun_attacks( bow );
        }
        SECTION( "m1911" ) {
            const npc_attack_gun_test_data gun{
                "m1911",
                210, 240
            };
            test_gun_attacks( gun );
        }
        SECTION( "remington_870" ) {
            const npc_attack_gun_test_data gun{
                "remington_870",
                471, 501
            };
            test_gun_attacks( gun );
        }
        SECTION( "browning_blr" ) {
            const npc_attack_gun_test_data gun{
                "browning_blr",
                516, 546
            };
            test_gun_attacks( gun );
        }
        SECTION( "shotgun_d" ) {
            const npc_attack_gun_test_data gun{
                "shotgun_d",
                417, 447
            };
            test_gun_attacks( gun );
        }
        SECTION( "m16a4" ) {
            const npc_attack_gun_test_data gun{
                "m16a4",
                336, 366
            };
            test_gun_attacks( gun );
        }
    }
}
