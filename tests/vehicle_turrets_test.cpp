#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "coordinates.h"
#include "explosion.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const ammo_effect_str_id ammo_effect_RECYCLED( "RECYCLED" );

static const gun_mode_id gun_mode_BURST( "BURST" );

static const itype_id itype_9mm( "9mm" );
static const itype_id itype_UPS_ON( "UPS_ON" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_gasoline( "gasoline" );
static const itype_id itype_glockmag( "glockmag" );

static const vpart_id vpart_frame( "frame" );
static const vpart_id vpart_tank( "tank" );
static const vpart_id vpart_turret_test_multimag_gun_consume( "turret_test_multimag_gun_consume" );
static const vpart_id vpart_turret_test_multimag_turret_flamethrower(
    "turret_test_multimag_turret_flamethrower" );
static const vpart_id vpart_turret_test_multimag_turret_gun( "turret_test_multimag_turret_gun" );
static const vpart_id vpart_turret_test_multimag_turret_gun_ups(
    "turret_test_multimag_turret_gun_ups" );

static const vproto_id vehicle_prototype_test_turret_rig( "test_turret_rig" );

static std::vector<const vpart_info *> all_turret_types()
{
    std::vector<const vpart_info *> res;

    for( const vpart_info &vpi : vehicles::parts::get_all() ) {
        if( vpi.has_flag( "TURRET" ) ) {
            res.push_back( &vpi );
        }
    }

    return res;
}

static void clear_faults_from_vp( vehicle_part &vp )
{
    // Just some small trickery to manipulate the const reference provided by get_base().
    item base_copy( vp.get_base() );
    base_copy.faults.clear();
    vp.set_base( std::move( base_copy ) );
}

// Install, reload and fire every possible vehicle turret.
TEST_CASE( "vehicle_turret", "[vehicle][gun][magazine]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();
    const tripoint_bub_ms veh_pos( 65, 65, 0 );

    for( const vpart_info *turret_vpi : all_turret_types() ) {
        // Multimag guns are turret-installable but covered by
        // vehicle_turret_multimag; the legacy ammo_set path here cannot
        // populate per-pocket firing_requirements wells.
        if( !turret_vpi->base_item->firing_requirements.empty() ) {
            continue;
        }
        SECTION( turret_vpi->name() ) {
            vehicle *veh = here.add_vehicle( vehicle_prototype_test_turret_rig, veh_pos, 270_degrees, 0, 2,
                                             false, true );
            REQUIRE( veh );

            const int turr_idx = veh->install_part( here, point_rel_ms::zero, turret_vpi->id );
            REQUIRE( turr_idx >= 0 );
            vehicle_part &vp = veh->part( turr_idx );
            CHECK( vp.is_turret() );

            const itype *base_itype = vp.get_base().type;
            REQUIRE( base_itype );
            REQUIRE( base_itype->gun );
            if( base_itype->gun->energy_drain > 0_kJ || turret_vpi->has_flag( "USE_BATTERIES" ) ) {
                const auto& [bat_current, bat_capacity] = veh->battery_power_level( );
                CHECK( bat_capacity > 0 );
                veh->charge_battery( here, bat_capacity, /* apply_loss = */ false );
                REQUIRE( veh->battery_left( here, /* apply_loss = */ false ) == bat_capacity );
            }

            const itype_id ammo_itype = vp.get_base().ammo_default();
            if( ammo_itype.is_null() ) {
                // probably a pure energy weapon
                CHECK( base_itype->gun->energy_drain > 0_kJ );
            } else if( turret_vpi->has_flag( "USE_TANKS" ) ) {
                CAPTURE( ammo_itype.str() );
                CAPTURE( veh->type.str() );
                bool filled_tank = false;
                for( const vpart_reference &vpr : veh->get_all_parts() ) {
                    vehicle_part &vp = vpr.part();
                    if( vp.is_tank() && vp.get_base().can_contain( item( ammo_itype ) ).success() ) {
                        CHECK( vp.ammo_set( ammo_itype ) > 0 );
                        filled_tank = true;
                        break;
                    }
                }
                REQUIRE( filled_tank );
            } else {
                CHECK( vp.ammo_set( ammo_itype ) > 0 );
            }
            const bool default_ammo_is_RECYCLED = vp.get_base().ammo_effects().count(
                    ammo_effect_RECYCLED ) > 0;
            if( default_ammo_is_RECYCLED ) {
                CAPTURE( default_ammo_is_RECYCLED );
                INFO( "RECYCLED ammo can sometimes misfire and very rarely fail this test" );
            }

            turret_data qry = veh->turret_query( vp );
            REQUIRE( qry );
            REQUIRE( qry.query() == turret_data::status::ready );
            REQUIRE( qry.range() > 0 );

            player_character.setpos( here, veh->bub_part_pos( here, vp ) );
            int shots_fired = 0;
            // 3 attempts to fire, to account for possible misfires
            for( int attempt = 0; shots_fired == 0 && attempt < 3; attempt++ ) {
                shots_fired += qry.fire( player_character, &here, player_character.pos_bub() + point( qry.range(),
                                         0 ) );
                clear_faults_from_vp( vp );
            }
            CHECK( shots_fired > 0 );

            here.destroy_vehicle( veh );

            // clear pending explosions so not to interfere with subsequent tests
            explosion_handler::process_explosions();
            // heal the avatar from explosion damages
            clear_avatar();
        }
    }
}

TEST_CASE( "vehicle_turret_multimag", "[vehicle][turret][multimag]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();
    const tripoint_bub_ms veh_pos( 65, 65, 0 );

    SECTION( "multimag gun without NO_TURRET has an auto-generated turret vpart" ) {
        // NO_TURRET on the gun suppresses turret vpart generation.
        REQUIRE( vpart_turret_test_multimag_turret_gun.is_valid() );
        REQUIRE_FALSE( vpart_turret_test_multimag_gun_consume.is_valid() );
    }

    SECTION( "flamethrower-shape gun gets USE_TANKS flag from gun_uses_liquid_ammo" ) {
        // Direct MAGAZINE liquid pocket on the gun must trigger USE_TANKS in
        // the auto-generated turret vpart so install heuristics treat it as
        // a fluid-fed turret.
        REQUIRE( vpart_turret_test_multimag_turret_flamethrower.is_valid() );
        REQUIRE( vpart_turret_test_multimag_turret_flamethrower.obj().has_flag( "USE_TANKS" ) );
    }

    SECTION( "install + query + fire happy path" ) {
        vehicle *veh = here.add_vehicle( vehicle_prototype_test_turret_rig, veh_pos,
                                         270_degrees, 0, 2, false, true );
        REQUIRE( veh );

        const int turr_idx = veh->install_part( here, point_rel_ms::zero,
                                                vpart_turret_test_multimag_turret_gun );
        REQUIRE( turr_idx >= 0 );
        vehicle_part &vp = veh->part( turr_idx );
        CHECK( vp.is_turret() );

        REQUIRE( veh->turret_query( vp ).query() == turret_data::status::no_ammo );

        // Charge battery (vehicle source for power well) but leave 9mm well empty.
        const auto& [bat_current, bat_capacity] = veh->battery_power_level();
        CHECK( bat_capacity > 0 );
        veh->charge_battery( here, bat_capacity, /* apply_loss = */ false );
        REQUIRE( veh->turret_query( vp ).query() == turret_data::status::no_ammo );

        item mag( itype_glockmag );
        mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
        item base_copy( vp.get_base() );
        REQUIRE( base_copy.put_in( mag, pocket_type::MAGAZINE_WELL ).success() );
        vp.set_base( std::move( base_copy ) );

        turret_data qry = veh->turret_query( vp );
        REQUIRE( qry.query() == turret_data::status::ready );
        REQUIRE( qry.range() > 0 );

        const int batt_before = veh->battery_left( here, /* apply_loss = */ false );

        player_character.setpos( here, veh->bub_part_pos( here, vp ) );
        int shots_fired = 0;
        for( int attempt = 0; shots_fired == 0 && attempt < 3; attempt++ ) {
            shots_fired += qry.fire( player_character, &here,
                                     player_character.pos_bub() + point( qry.range(), 0 ) );
            clear_faults_from_vp( vp );
        }
        CHECK( shots_fired > 0 );

        // Vehicle battery drained per power-pocket per_use (5 kJ DEFAULT).
        const int batt_after = veh->battery_left( here, /* apply_loss = */ false );
        CHECK( batt_before - batt_after == 5 * shots_fired );

        here.destroy_vehicle( veh );
        explosion_handler::process_explosions();
        clear_avatar();
    }

    SECTION( "post_fire clears bound power well, leaves player ammo well intact" ) {
        vehicle *veh = here.add_vehicle( vehicle_prototype_test_turret_rig, veh_pos,
                                         270_degrees, 0, 2, false, true );
        REQUIRE( veh );
        const int turr_idx = veh->install_part( here, point_rel_ms::zero,
                                                vpart_turret_test_multimag_turret_gun );
        REQUIRE( turr_idx >= 0 );
        vehicle_part &vp = veh->part( turr_idx );

        const auto& [bat_current, bat_capacity] = veh->battery_power_level();
        veh->charge_battery( here, bat_capacity, /* apply_loss = */ false );

        item mag( itype_glockmag );
        const int mag_initial = 15;
        mag.put_in( item( itype_9mm, calendar::turn, mag_initial ), pocket_type::MAGAZINE );
        item base_copy( vp.get_base() );
        REQUIRE( base_copy.put_in( mag, pocket_type::MAGAZINE_WELL ).success() );
        vp.set_base( std::move( base_copy ) );

        turret_data qry = veh->turret_query( vp );
        REQUIRE( qry.query() == turret_data::status::ready );
        player_character.setpos( here, veh->bub_part_pos( here, vp ) );
        int shots_fired = 0;
        for( int attempt = 0; shots_fired == 0 && attempt < 3; attempt++ ) {
            shots_fired += qry.fire( player_character, &here,
                                     player_character.pos_bub() + point( qry.range(), 0 ) );
            clear_faults_from_vp( vp );
        }
        REQUIRE( shots_fired > 0 );

        // Player-loaded mag survives post_fire with rounds drawn down by per_use.
        const int ammo_after = vp.get_base().ammo_remaining_in_pocket( "ammo" );
        CHECK( ammo_after == mag_initial - shots_fired );

        // Vehicle-bound power well cleared back to empty so next prep starts fresh.
        CHECK( vp.get_base().ammo_remaining_in_pocket( "power" ) == 0 );

        here.destroy_vehicle( veh );
        explosion_handler::process_explosions();
        clear_avatar();
    }

    SECTION( "USE_UPS multimag turret leaves player UPS untouched" ) {
        vehicle *veh = here.add_vehicle( vehicle_prototype_test_turret_rig, veh_pos,
                                         270_degrees, 0, 2, false, true );
        REQUIRE( veh );
        const int turr_idx = veh->install_part( here, point_rel_ms::zero,
                                                vpart_turret_test_multimag_turret_gun_ups );
        REQUIRE( turr_idx >= 0 );
        vehicle_part &vp = veh->part( turr_idx );

        const auto& [bat_current, bat_capacity] = veh->battery_power_level();
        veh->charge_battery( here, bat_capacity, /* apply_loss = */ false );

        item mag( itype_glockmag );
        mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
        item base_copy( vp.get_base() );
        REQUIRE( base_copy.put_in( mag, pocket_type::MAGAZINE_WELL ).success() );
        vp.set_base( std::move( base_copy ) );

        // Give the player a charged UPS so available_ups returns nonzero.
        player_character.worn.wear_item( player_character, item( itype_backpack ),
                                         false, false );
        item ups( itype_UPS_ON );
        item ups_mag( ups.magazine_default() );
        ups_mag.ammo_set( ups_mag.ammo_default(), 100 );
        REQUIRE( ups.put_in( ups_mag, pocket_type::MAGAZINE_WELL ).success() );
        player_character.i_add( ups );
        REQUIRE( units::to_kilojoule( player_character.available_ups() ) == 100 );

        const int batt_before = veh->battery_left( here, /* apply_loss = */ false );
        const int ups_before = units::to_kilojoule( player_character.available_ups() );

        turret_data qry = veh->turret_query( vp );
        REQUIRE( qry.query() == turret_data::status::ready );
        player_character.setpos( here, veh->bub_part_pos( here, vp ) );
        int shots_fired = 0;
        for( int attempt = 0; shots_fired == 0 && attempt < 3; attempt++ ) {
            shots_fired += qry.fire( player_character, &here,
                                     player_character.pos_bub() + point( qry.range(), 0 ) );
            clear_faults_from_vp( vp );
        }
        REQUIRE( shots_fired > 0 );

        // Vehicle pays the full per_use; player UPS is left alone.
        const int batt_after = veh->battery_left( here, /* apply_loss = */ false );
        const int ups_after = units::to_kilojoule( player_character.available_ups() );
        CHECK( batt_before - batt_after == 5 * shots_fired );
        CHECK( ups_after == ups_before );

        here.destroy_vehicle( veh );
        explosion_handler::process_explosions();
        clear_avatar();
    }

    SECTION( "tank picker skips insufficient tank in favour of sufficient sibling" ) {
        vehicle *veh = here.add_vehicle( vehicle_prototype_test_turret_rig, veh_pos,
                                         270_degrees, 0, 2, false, true );
        REQUIRE( veh );
        // test_turret_rig already has a tank at (1,0); install a second frame
        // + tank at (1,1) so the picker has to choose between two siblings.
        REQUIRE( veh->install_part( here, point_rel_ms( 1, 1 ), vpart_frame ) != -1 );
        const int tank2_idx = veh->install_part( here, point_rel_ms( 1, 1 ), vpart_tank );
        REQUIRE( tank2_idx >= 0 );
        veh->refresh();

        const int turr_idx = veh->install_part( here, point_rel_ms::zero,
                                                vpart_turret_test_multimag_turret_flamethrower );
        REQUIRE( turr_idx >= 0 );
        vehicle_part &vp = veh->part( turr_idx );

        const auto& [bat_current, bat_capacity] = veh->battery_power_level();
        veh->charge_battery( here, bat_capacity, /* apply_loss = */ false );

        // Find both tanks; one gets the meager fill, the other the sufficient.
        int tank_meager = -1;
        int tank_full = -1;
        for( const vpart_reference &tvp :
             veh->get_avail_parts( vpart_bitflags::VPFLAG_FLUIDTANK ) ) {
            const int idx = static_cast<int>( tvp.part_index() );
            if( tank_meager == -1 ) {
                tank_meager = idx;
            } else if( tank_full == -1 ) {
                tank_full = idx;
            }
        }
        REQUIRE( tank_meager >= 0 );
        REQUIRE( tank_full >= 0 );
        REQUIRE( tank_meager != tank_full );
        veh->part( tank_meager ).ammo_set( itype_gasoline, 1 );
        veh->part( tank_full ).ammo_set( itype_gasoline, 50 );

        turret_data qry = veh->turret_query( vp );
        REQUIRE( qry.query() == turret_data::status::ready );

        player_character.setpos( here, veh->bub_part_pos( here, vp ) );
        int shots_fired = 0;
        for( int attempt = 0; shots_fired == 0 && attempt < 3; attempt++ ) {
            shots_fired += qry.fire( player_character, &here,
                                     player_character.pos_bub() + point( qry.range(), 0 ) );
            clear_faults_from_vp( vp );
        }
        REQUIRE( shots_fired > 0 );

        // Picker must skip the meager tank; only the sufficient one drains.
        CHECK( veh->part( tank_meager ).ammo_remaining() == 1 );
        CHECK( veh->part( tank_full ).ammo_remaining() == 50 - 5 * shots_fired );

        here.destroy_vehicle( veh );
        explosion_handler::process_explosions();
        clear_avatar();
    }

    SECTION( "BURST mode drains per_use_battery * mode.qty per shot" ) {
        vehicle *veh = here.add_vehicle( vehicle_prototype_test_turret_rig, veh_pos,
                                         270_degrees, 0, 2, false, true );
        REQUIRE( veh );
        const int turr_idx = veh->install_part( here, point_rel_ms::zero,
                                                vpart_turret_test_multimag_turret_gun );
        REQUIRE( turr_idx >= 0 );
        vehicle_part &vp = veh->part( turr_idx );
        const auto& [bat_current, bat_capacity] = veh->battery_power_level();
        veh->charge_battery( here, bat_capacity, /* apply_loss = */ false );

        item mag( itype_glockmag );
        mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
        item base_copy( vp.get_base() );
        REQUIRE( base_copy.put_in( mag, pocket_type::MAGAZINE_WELL ).success() );
        vp.set_base( std::move( base_copy ) );

        {
            item gun_with_mode( vp.get_base() );
            while( gun_with_mode.gun_get_mode_id() != gun_mode_BURST ) {
                gun_with_mode.gun_cycle_mode();
            }
            vp.set_base( std::move( gun_with_mode ) );
        }
        REQUIRE( vp.get_base().gun_get_mode_id() == gun_mode_BURST );

        turret_data qry = veh->turret_query( vp );
        REQUIRE( qry.query() == turret_data::status::ready );
        const int batt_before = veh->battery_left( here, /* apply_loss = */ false );

        player_character.setpos( here, veh->bub_part_pos( here, vp ) );
        int shots_fired = 0;
        for( int attempt = 0; shots_fired == 0 && attempt < 3; attempt++ ) {
            shots_fired += qry.fire( player_character, &here,
                                     player_character.pos_bub() + point( qry.range(), 0 ) );
            clear_faults_from_vp( vp );
        }
        REQUIRE( shots_fired > 0 );

        const int batt_after = veh->battery_left( here, /* apply_loss = */ false );
        CHECK( batt_before - batt_after == 15 * shots_fired );

        here.destroy_vehicle( veh );
        explosion_handler::process_explosions();
        clear_avatar();
    }

    SECTION( "BURST stops mid-sequence when vehicle battery cannot cover next burst" ) {
        vehicle *veh = here.add_vehicle( vehicle_prototype_test_turret_rig, veh_pos,
                                         270_degrees, 0, 2, false, true );
        REQUIRE( veh );
        const int turr_idx = veh->install_part( here, point_rel_ms::zero,
                                                vpart_turret_test_multimag_turret_gun );
        REQUIRE( turr_idx >= 0 );
        vehicle_part &vp = veh->part( turr_idx );

        // Charge vehicle to exactly one BURST worth (15 kJ); next burst must fail.
        veh->discharge_battery( here, 100000 );
        veh->charge_battery( here, 15, /* apply_loss = */ false );
        REQUIRE( static_cast<int>( veh->battery_left( here, false ) ) == 15 );

        item mag( itype_glockmag );
        mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
        item base_copy( vp.get_base() );
        REQUIRE( base_copy.put_in( mag, pocket_type::MAGAZINE_WELL ).success() );
        vp.set_base( std::move( base_copy ) );

        {
            item gun_with_mode( vp.get_base() );
            while( gun_with_mode.gun_get_mode_id() != gun_mode_BURST ) {
                gun_with_mode.gun_cycle_mode();
            }
            vp.set_base( std::move( gun_with_mode ) );
        }

        turret_data qry = veh->turret_query( vp );
        REQUIRE( qry.query() == turret_data::status::ready );

        player_character.setpos( here, veh->bub_part_pos( here, vp ) );
        const int first = qry.fire( player_character, &here,
                                    player_character.pos_bub() + point( qry.range(), 0 ) );
        clear_faults_from_vp( vp );
        CHECK( first > 0 );
        CHECK( veh->battery_left( here, false ) == 0 );

        // Vehicle empty; subsequent burst must report no_ammo before firing.
        turret_data qry2 = veh->turret_query( vp );
        CHECK( qry2.query() == turret_data::status::no_ammo );

        here.destroy_vehicle( veh );
        explosion_handler::process_explosions();
        clear_avatar();
    }
}
