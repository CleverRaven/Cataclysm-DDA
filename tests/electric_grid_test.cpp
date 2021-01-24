#include <vector>

#include "active_tile_data.h"
#include "catch/catch.hpp"
#include "coordinate_conversions.h"
#include "distribution_grid.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "vehicle.h"

TEST_CASE( "connect vehicle", "[grids]" )
{
    clear_map_and_put_player_underground();
    const tripoint vehicle_local_pos = tripoint( 10, 10, 0 );
    const tripoint vehicle_abs_pos = g->m.getabs( vehicle_local_pos );
    const tripoint furniture_local_pos = tripoint( 13, 10, 0 );
    const tripoint furniture_abs_pos = g->m.getabs( furniture_local_pos );


    // TODO: Wrap in clear_grids()?
    auto om = overmap_buffer.get_om_global( sm_to_omt_copy( g->m.getabs( furniture_local_pos ) ) );
    // TODO: That's a lot of setup, implying barely testable design
    om.om->set_electric_grid_connections( om.local, {} );
    // Mega ugly
    g->load_map( g->m.get_abs_sub() );

    g->m.furn_set( furniture_local_pos, furn_str_id( "f_cable_connector" ) );
    vehicle *veh = g->m.add_vehicle( vproto_id( "car" ), vehicle_local_pos, 0, 100, 0, false );
    vehicle_connector_tile *grid_connector = g->m.active_furniture_at<vehicle_connector_tile>
            ( furniture_abs_pos );

    REQUIRE( veh );
    REQUIRE( grid_connector );

    const point cable_part_pos = point( 0, 0 );
    vehicle_part source_part( vpart_id( "jumper_cable" ), cable_part_pos, item( "jumper_cable" ) );
    source_part.target.first = furniture_abs_pos;
    source_part.target.second = furniture_abs_pos;
    source_part.set_flag( vehicle_part::targets_grid );
    grid_connector->connected_vehicles.emplace_back( vehicle_abs_pos );
    int part_index = veh->install_part( cable_part_pos, source_part );

    REQUIRE( part_index >= 0 );

    distribution_grid &grid = g->m.distribution_grid_at( furniture_local_pos );
    REQUIRE( !grid.empty() );
    CHECK( grid.get_resource() == veh->fuel_left( itype_id( "battery" ), false ) );

    int vehicle_battery_before = veh->fuel_left( itype_id( "battery" ), false );
    CHECK( grid.mod_resource( -10 ) == 0 );
    CHECK( grid.get_resource() == vehicle_battery_before - 10 );
    CHECK( veh->fuel_left( itype_id( "battery" ), false ) == vehicle_battery_before - 10 );
}
