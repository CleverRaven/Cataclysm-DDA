#include "cata_catch.h"

#include <algorithm>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "ammo.h"
#include "character.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "map.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"

static const itype_id itype_battery( "battery" );

static const vpart_id vpart_storage_battery( "storage_battery" );

static const vproto_id vehicle_prototype_none( "none" );

static std::vector<const vpart_info *> all_turret_types()
{
    std::vector<const vpart_info *> res;

    for( const auto &e : vpart_info::all() ) {
        if( e.second.has_flag( "TURRET" ) ) {
            res.push_back( &e.second );
        }
    }

    return res;
}

static const vpart_info *tank_for_ammo( const itype_id &ammo_itype )
{
    item ammo( ammo_itype );
    for( const auto &e : vpart_info::all() ) {
        const vpart_info &vp = e.second;
        if( !vp.base_item ) {
            continue;
        }
        item base_item( vp.base_item );
        if( base_item.is_watertight_container() && base_item.can_contain( ammo ).success() ) {
            return &vp;
        }
    }
    return nullptr;
}

static void install_tank_with_ammo( vehicle *veh, const itype_id &ammo_itype )
{
    const vpart_info *tank = tank_for_ammo( ammo_itype );
    REQUIRE( tank );
    int tank_idx = veh->install_part( point_zero, tank->get_id(), "", true );
    REQUIRE( tank_idx >= 0 );
    vehicle_part &tank_part = veh->part( tank_idx );
    CHECK( tank_part.is_tank() );
    CHECK( tank_part.ammo_set( ammo_itype ) > 0 );
}

static void install_charged_battery( vehicle *veh )
{
    const int batt_idx = veh->install_part( point_zero, vpart_storage_battery, "", true );
    REQUIRE( batt_idx >= 0 );
    veh->part( batt_idx ).ammo_set( itype_battery, -1 );
}

// Install, reload and fire every possible vehicle turret.
TEST_CASE( "vehicle_turret", "[vehicle][gun][magazine]" )
{
    map &here = get_map();
    Character &player_character = get_player_character();
    for( const vpart_info *turret_vpi : all_turret_types() ) {
        SECTION( turret_vpi->name() ) {
            vehicle *veh = here.add_vehicle( vehicle_prototype_none, point( 65, 65 ), 270_degrees );
            REQUIRE( veh );

            const int turr_idx = veh->install_part( point_zero, turret_vpi->get_id(), "", true );
            REQUIRE( turr_idx >= 0 );
            CHECK( veh->part( turr_idx ).is_turret() );

            const itype *base_itype = veh->part( turr_idx ).get_base().type;
            REQUIRE( base_itype );
            REQUIRE( base_itype->gun );
            if( base_itype->gun->energy_drain > 0_kJ || turret_vpi->has_flag( "USE_BATTERIES" ) ) {
                install_charged_battery( veh );
            }

            const itype_id ammo_itype = veh->part( turr_idx ).get_base().ammo_default();
            if( ammo_itype.is_null() ) {
                // probably a pure energy weapon
                CHECK( base_itype->gun->energy_drain > 0_kJ );
            } else if( turret_vpi->has_flag( "USE_TANKS" ) ) {
                install_tank_with_ammo( veh, ammo_itype );
            } else {
                CHECK( veh->part( turr_idx ).ammo_set( ammo_itype ) > 0 );
            }
            const bool default_ammo_is_RECYCLED = veh->part( turr_idx )
                                                  .get_base().ammo_effects().count( "RECYCLED" ) > 0;
            CAPTURE( default_ammo_is_RECYCLED );
            INFO( "RECYCLED ammo can sometimes misfire and very rarely fail this test" );

            turret_data qry = veh->turret_query( veh->part( turr_idx ) );
            REQUIRE( qry );
            REQUIRE( qry.query() == turret_data::status::ready );
            REQUIRE( qry.range() > 0 );

            player_character.setpos( veh->global_part_pos3( turr_idx ) );
            int shots_fired = 0;
            // 3 attempts to fire, to account for possible misfires
            for( int attempt = 0; shots_fired == 0 && attempt < 3; attempt++ ) {
                shots_fired += qry.fire( player_character, player_character.pos() + point( qry.range(), 0 ) );
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
