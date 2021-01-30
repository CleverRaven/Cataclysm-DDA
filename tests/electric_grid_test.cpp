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

static void connect_grid_vehicle( vehicle &veh, vehicle_connector_tile &connector,
                                  const tripoint &connector_abs_pos )
{
    const point cable_part_pos = point( 0, 0 );
    vehicle_part source_part( vpart_id( "jumper_cable" ), cable_part_pos, item( "jumper_cable" ) );
    source_part.target.first = connector_abs_pos;
    source_part.target.second = connector_abs_pos;
    source_part.set_flag( vehicle_part::targets_grid );
    connector.connected_vehicles.emplace_back( veh.global_pos3() );
    int part_index = veh.install_part( cable_part_pos, source_part );

    REQUIRE( part_index >= 0 );
}

TEST_CASE( "drain vehicle with grid", "[grids][vehicle]" )
{
    clear_map_and_put_player_underground();
    const tripoint vehicle_local_pos = tripoint( 10, 10, 0 );
    const tripoint connector_local_pos = tripoint( 13, 10, 0 );

    map &m = g->m;
    const tripoint connector_abs_pos = m.getabs( connector_local_pos );

    m.furn_set( connector_local_pos, furn_str_id( "f_cable_connector" ) );
    vehicle *veh = m.add_vehicle( vproto_id( "car" ), vehicle_local_pos, 0, 100, 0, false );
    vehicle_connector_tile *grid_connector = active_tiles::furn_at<vehicle_connector_tile>
            ( connector_abs_pos );

    m.save();

    REQUIRE( veh );
    REQUIRE( grid_connector );

    connect_grid_vehicle( *veh, *grid_connector, connector_abs_pos );

    distribution_grid &grid = get_distribution_grid_tracker().grid_at( connector_abs_pos );
    REQUIRE( !grid.empty() );
    CHECK( grid.get_resource() == veh->fuel_left( itype_id( "battery" ), false ) );

    int vehicle_battery_before = veh->fuel_left( itype_id( "battery" ), false );
    CHECK( grid.mod_resource( -10 ) == 0 );
    CHECK( grid.get_resource() == vehicle_battery_before - 10 );
    CHECK( veh->fuel_left( itype_id( "battery" ), false ) == vehicle_battery_before - 10 );
}

TEST_CASE( "drain grid with vehicle", "[grids][vehicle]" )
{
    clear_map_and_put_player_underground();
    const tripoint vehicle_local_pos = tripoint( 10, 10, 0 );
    const tripoint connector_local_pos = tripoint( 13, 10, 0 );
    const tripoint battery_local_pos = tripoint( 14, 10, 0 );

    map &m = g->m;
    const tripoint connector_abs_pos = m.getabs( connector_local_pos );
    const tripoint battery_abs_pos = m.getabs( battery_local_pos );

    m.furn_set( connector_local_pos, furn_str_id( "f_cable_connector" ) );
    m.furn_set( battery_local_pos, furn_str_id( "f_battery" ) );
    vehicle *veh = m.add_vehicle( vproto_id( "car" ), vehicle_local_pos, 0, 100, 0, false );
    vehicle_connector_tile *grid_connector =
        active_tiles::furn_at<vehicle_connector_tile>( connector_abs_pos );
    battery_tile *battery = active_tiles::furn_at<battery_tile>( battery_abs_pos );

    m.save();

    REQUIRE( veh );
    REQUIRE( grid_connector );
    REQUIRE( battery );

    connect_grid_vehicle( *veh, *grid_connector, connector_abs_pos );

    distribution_grid &grid = get_distribution_grid_tracker().grid_at( connector_abs_pos );
    REQUIRE( !grid.empty() );
    REQUIRE( &grid == &get_distribution_grid_tracker().grid_at( battery_abs_pos ) );
    CHECK( grid.get_resource() == veh->fuel_left( itype_id( "battery" ), false ) );
    int excess = grid.mod_resource( 10 );
    CHECK( excess == 0 );
    CHECK( grid.get_resource() == veh->fuel_left( itype_id( "battery" ), false ) + 10 );

    int missing = veh->discharge_battery( grid.get_resource() - 5 );
    CHECK( missing == 0 );
    CHECK( grid.get_resource() == 5 );
    CHECK( veh->fuel_left( itype_id( "battery" ), false ) == 0 );
}
