#include <functional>
#include <string>
#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "flag.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"

static const itype_id itype_soap( "soap" );
static const itype_id itype_soapy_water( "soapy_water" );
static const itype_id itype_t_shirt( "t_shirt" );
static const itype_id itype_water( "water" );
static const itype_id itype_water_clean( "water_clean" );

static const vproto_id vehicle_prototype_none( "none" );

static const vpart_id vpart_tank_medium( "tank_medium" );
static const vpart_id vpart_washing_machine( "washing_machine" );

// Helper: set up a vehicle with a washing machine and a tank, returns the vehicle and wm part index
static std::pair<vehicle *, int> setup_washing_machine( map &here, const tripoint_bub_ms &pos )
{
    vehicle *veh = here.add_vehicle( vehicle_prototype_none, pos, 0_degrees, 0, 0 );
    REQUIRE( veh != nullptr );

    const int wm_idx  = veh->install_part( here, point_rel_ms::zero, vpart_washing_machine );
    const int tan_idx = veh->install_part( here, point_rel_ms::zero, vpart_tank_medium );
    REQUIRE( wm_idx  >= 0 );
    REQUIRE( tan_idx >= 0 );

    return { veh, wm_idx };
}

// Helper: add liquid charges to a vehicle tank part
static void add_liquid_to_vehicle( map &here, vehicle &veh, int tank_idx,
                                   const itype_id &liquid_id, int charges )
{
    item liquid( liquid_id );
    liquid.charges = charges;
    veh.add_item( here, veh.part( tank_idx ), liquid );
}

TEST_CASE( "washing_machine_soapy_water_starts_without_detergent", "[vehicle][washing_machine]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    const tripoint_bub_ms veh_pos( 60, 60, 0 );
    auto [veh, wm_idx] = setup_washing_machine( here, veh_pos );

    // Tank is the second installed part (index wm_idx + 1)
    const int tan_idx = wm_idx + 1;
    add_liquid_to_vehicle( here, *veh, tan_idx, itype_soapy_water, 30 );
    REQUIRE( veh->fuel_left( here, itype_soapy_water ) >= 24 );

    // Add a filthy item to the washing machine
    item filthy_shirt( itype_t_shirt );
    filthy_shirt.set_flag( flag_id( "FILTHY" ) );
    veh->add_item( here, veh->part( wm_idx ), filthy_shirt );

    // Avatar has no detergent — soapy water must be sufficient
    avatar &player = get_avatar();
    REQUIRE( player.crafting_inventory().items_with(
    std::function<bool( const item & )>( []( const item & it ) {
        return it.has_flag( flag_id( "DETERGENT" ) );
    } ) ).empty() );

    const int soapy_before = veh->fuel_left( here, itype_soapy_water );

    veh->use_washing_machine( here, wm_idx );

    // Soapy water was consumed → machine started successfully
    CHECK( veh->fuel_left( here, itype_soapy_water ) < soapy_before );
}

TEST_CASE( "washing_machine_normal_water_with_detergent_still_works", "[vehicle][washing_machine]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    const tripoint_bub_ms veh_pos( 60, 60, 0 );
    auto [veh, wm_idx] = setup_washing_machine( here, veh_pos );

    const int tan_idx = wm_idx + 1;
    add_liquid_to_vehicle( here, *veh, tan_idx, itype_water, 30 );
    REQUIRE( veh->fuel_left( here, itype_water ) >= 24 );
    REQUIRE( veh->fuel_left( here, itype_soapy_water ) == 0 );

    item filthy_shirt( itype_t_shirt );
    filthy_shirt.set_flag( flag_id( "FILTHY" ) );
    veh->add_item( here, veh->part( wm_idx ), filthy_shirt );

    // Give avatar soap as detergent
    avatar &player = get_avatar();
    item soap( itype_soap );
    soap.charges = 10;
    player.i_add( soap );

    const int water_before = veh->fuel_left( here, itype_water );

    veh->use_washing_machine( here, wm_idx );

    // Normal water was consumed → machine started successfully
    CHECK( veh->fuel_left( here, itype_water ) < water_before );
}

TEST_CASE( "washing_machine_blocked_without_any_water", "[vehicle][washing_machine]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    const tripoint_bub_ms veh_pos( 60, 60, 0 );
    auto [veh, wm_idx] = setup_washing_machine( here, veh_pos );

    REQUIRE( veh->fuel_left( here, itype_water ) == 0 );
    REQUIRE( veh->fuel_left( here, itype_water_clean ) == 0 );
    REQUIRE( veh->fuel_left( here, itype_soapy_water ) == 0 );

    item filthy_shirt( itype_t_shirt );
    filthy_shirt.set_flag( flag_id( "FILTHY" ) );
    veh->add_item( here, veh->part( wm_idx ), filthy_shirt );

    veh->use_washing_machine( here, wm_idx );

    // Nothing consumed → machine did not start
    CHECK( veh->fuel_left( here, itype_water ) == 0 );
    CHECK( veh->fuel_left( here, itype_soapy_water ) == 0 );
}
