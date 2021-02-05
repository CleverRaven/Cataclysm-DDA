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

static itype_id itype_battery( "battery" );

static inline void test_grid_veh( distribution_grid &grid, vehicle &veh, battery_tile &battery )
{
    CAPTURE( veh.fuel_capacity( itype_battery ) );
    CAPTURE( battery.max_stored );
    WHEN( "the vehicle is fully charged and battery is discharged" ) {
        veh.charge_battery( veh.fuel_capacity( itype_battery ), false );
        REQUIRE( veh.fuel_left( itype_battery, false ) ==
                 veh.fuel_capacity( itype_battery ) );
        REQUIRE( battery.get_resource() == 0 );
        REQUIRE( grid.get_resource() == veh.fuel_capacity( itype_battery ) );
        AND_WHEN( "the grid is discharged without energy in battery" ) {
            int deficit = grid.mod_resource( -( grid.get_resource() - 10 ) );
            CHECK( deficit == 0 );
            THEN( "the power is drained from vehicle" ) {
                CHECK( grid.get_resource() == 10 );
                CHECK( veh.fuel_left( itype_battery, false ) == 10 );
            }
        }

        AND_WHEN( "the vehicle is charged despite being full" ) {
            int excess = veh.charge_battery( 10 );
            THEN( "the grid contains the added energy" ) {
                CHECK( excess == 0 );
                CHECK( grid.get_resource() == veh.fuel_left( itype_battery, false ) + 10 );
                AND_THEN( "the added energy is in the battery" ) {
                    CHECK( battery.get_resource() == 10 );
                }
            }
        }
    }

    WHEN( "the battery is fully charged and vehicle is discharged" ) {
        int excess = battery.mod_resource( battery.max_stored );
        REQUIRE( excess == 0 );
        REQUIRE( battery.get_resource() == battery.max_stored );
        REQUIRE( veh.fuel_left( itype_battery, false ) == 0 );
        REQUIRE( grid.get_resource() == battery.get_resource() );
        AND_WHEN( "the vehicle is discharged despite being empty" ) {
            int deficit = veh.discharge_battery( 10, true );
            THEN( "the grid provides the needed power" ) {
                CHECK( deficit == 0 );
                AND_THEN( "this power comes from the battery" ) {
                    CHECK( battery.get_resource() == battery.max_stored - 10 );
                }
            }
        }

        AND_WHEN( "the grid is charged some more" ) {
            int excess = grid.mod_resource( 10 );
            THEN( "the grid contains the added energy" ) {
                CHECK( excess == 0 );
                CHECK( grid.get_resource() == battery.max_stored + 10 );
                AND_THEN( "the added energy is in the vehicle" ) {
                    CHECK( veh.fuel_left( itype_battery, false ) == 10 );
                }
            }
        }
    }
}

static void connect_grid_vehicle( map &m, vehicle &veh, vehicle_connector_tile &connector,
                                  const tripoint &connector_abs_pos )
{
    const point cable_part_pos = point( 0, 0 );
    vehicle_part source_part( vpart_id( "jumper_cable" ), cable_part_pos, item( "jumper_cable" ) );
    source_part.target.first = connector_abs_pos;
    source_part.target.second = connector_abs_pos;
    source_part.set_flag( vehicle_part::targets_grid );
    connector.connected_vehicles.clear();
    connector.connected_vehicles.emplace_back( m.getabs( veh.global_pos3() ) );
    int part_index = veh.install_part( cable_part_pos, source_part );

    REQUIRE( part_index >= 0 );
}

struct grid_setup {
    distribution_grid &grid;
    vehicle &veh;
    battery_tile &battery;
};

static grid_setup set_up_grid( map &m )
{
    // TODO: clear_grids()
    auto om = overmap_buffer.get_om_global( sm_to_omt_copy( m.get_abs_sub() ) );
    om.om->set_electric_grid_connections( om.local, {} );

    const tripoint vehicle_local_pos = tripoint( 10, 10, 0 );
    const tripoint connector_local_pos = tripoint( 13, 10, 0 );
    const tripoint battery_local_pos = tripoint( 14, 10, 0 );
    const tripoint connector_abs_pos = m.getabs( connector_local_pos );
    const tripoint battery_abs_pos = m.getabs( battery_local_pos );
    m.furn_set( connector_local_pos, furn_str_id( "f_cable_connector" ) );
    m.furn_set( battery_local_pos, furn_str_id( "f_battery" ) );
    vehicle *veh = m.add_vehicle( vproto_id( "car" ), vehicle_local_pos, 0, 0, 0, false );
    vehicle_connector_tile *grid_connector =
        active_tiles::furn_at<vehicle_connector_tile>( connector_abs_pos );
    battery_tile *battery = active_tiles::furn_at<battery_tile>( battery_abs_pos );

    REQUIRE( veh );
    REQUIRE( grid_connector );
    REQUIRE( battery );

    connect_grid_vehicle( m, *veh, *grid_connector, connector_abs_pos );

    distribution_grid &grid = get_distribution_grid_tracker().grid_at( connector_abs_pos );
    REQUIRE( !grid.empty() );
    REQUIRE( &grid == &get_distribution_grid_tracker().grid_at( battery_abs_pos ) );
    return grid_setup{grid, *veh, *battery};
}

TEST_CASE( "grid_and_vehicle_in_bubble", "[grids][vehicle]" )
{
    clear_map_and_put_player_underground();
    GIVEN( "vehicle and battery are on one grid" ) {
        auto setup = set_up_grid( g->m );
        test_grid_veh( setup.grid, setup.veh, setup.battery );
    }
}

TEST_CASE( "grid_and_vehicle_outside_bubble", "[grids][vehicle]" )
{
    clear_map_and_put_player_underground();
    const tripoint old_abs_sub = g->m.get_abs_sub();
    // Ugly: we move the real map instead of the tinymap to reuse clear_map() results
    g->m.load( g->m.get_abs_sub() + point( g->m.getmapsize(), 0 ), true );
    GIVEN( "vehicle and battery are on one grid" ) {
        tinymap m;
        m.load( old_abs_sub, false );
        auto setup = set_up_grid( m );
        m.save();
        test_grid_veh( setup.grid, setup.veh, setup.battery );
    }
}
