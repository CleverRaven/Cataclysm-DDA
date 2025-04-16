#include <memory>
#include <set>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "explosion.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
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

// Install, reload and fire every possible vehicle turret.
TEST_CASE( "vehicle_turret", "[vehicle][gun][magazine]" )
{
    clear_map();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();
    const tripoint_bub_ms veh_pos( 65, 65, 0 );

    for( const vpart_info *turret_vpi : all_turret_types() ) {
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
            CAPTURE( default_ammo_is_RECYCLED );
            INFO( "RECYCLED ammo can sometimes misfire and very rarely fail this test" );

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
