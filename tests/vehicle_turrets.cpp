#include "catch/catch.hpp"

#include "game.h"
#include "itype.h"
#include "map.h"
#include "vehicle.h"
#include "veh_type.h"
#include "player.h"

static std::vector<const vpart_info *> turret_types() {
    std::vector<const vpart_info *> res;
    
    for( const auto &e : vpart_info::all() ) {
        if( e.second.has_flag( "TURRET" ) ) {
            res.push_back( &e.second );
        }
    }

    return res;
}

TEST_CASE( "vehicle_turret", "[vehicle] [gun] [magazine]" ) {
    for( auto e : turret_types() ) {
        SECTION( e->name() ) {
            vehicle *veh = g->m.add_vehicle( vproto_id( "none" ), 65, 65, 270, 0, 0 );
            REQUIRE( veh );

            const int idx = veh->install_part( 0, 0, e->get_id(), true );
            REQUIRE( idx >= 0 );

            REQUIRE( veh->install_part( 0,  0, vpart_id( "storage_battery" ), true ) >= 0 );
            veh->charge_battery( 10000 );

            auto ammo = veh->turret_query( veh->parts[idx] ).base()->ammo_type();

            if( veh->part_flag( idx, "USE_TANKS" ) ) {
                auto tank_idx = veh->install_part( 0, 0, vpart_id( "tank" ), true );
                REQUIRE( tank_idx >= 0 );
                REQUIRE( veh->parts[ tank_idx ].ammo_set( default_ammo( ammo ) ) );

            } else if( ammo ) {
                veh->parts[ idx].ammo_set( default_ammo( ammo ) );
            }

            auto qry = veh->turret_query( veh->parts[ idx ] );
            REQUIRE( qry );

            REQUIRE( qry.query() == turret_data::status::ready );
            REQUIRE( qry.range() > 0 );

            g->u.setpos( veh->global_part_pos3( idx ) );
            REQUIRE( qry.fire( g->u, g->u.pos() + point( qry.range(), 0 ) ) > 0 );

            g->m.destroy_vehicle( veh );
        }
    }
}
