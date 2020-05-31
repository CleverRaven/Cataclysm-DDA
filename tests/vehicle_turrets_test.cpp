#include <algorithm>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "ammo.h"
#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "map.h"
#include "point.h"
#include "string_id.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"

static std::vector<const vpart_info *> turret_types()
{
    std::vector<const vpart_info *> res;

    for( const auto &e : vpart_info::all() ) {
        if( e.second.has_flag( "TURRET" ) ) {
            res.push_back( &e.second );
        }
    }

    return res;
}

static const vpart_info *biggest_tank( const ammotype &ammo )
{
    std::vector<const vpart_info *> res;

    for( const auto &e : vpart_info::all() ) {
        const auto &vp = e.second;
        if( !item( vp.item ).is_watertight_container() ) {
            continue;
        }

        const itype *fuel = item::find_type( vp.fuel_type );
        if( fuel->ammo && fuel->ammo->type == ammo ) {
            res.push_back( &vp );
        }
    }

    if( res.empty() ) {
        return nullptr;
    }

    return * std::max_element( res.begin(), res.end(),
    []( const vpart_info * lhs, const vpart_info * rhs ) {
        return lhs->size < rhs->size;
    } );
}

TEST_CASE( "vehicle_turret", "[vehicle] [gun] [magazine] [.]" )
{
    for( auto e : turret_types() ) {
        SECTION( e->name() ) {
            vehicle *veh = g->m.add_vehicle( vproto_id( "none" ), point( 65, 65 ), 270, 0, 0 );
            REQUIRE( veh );

            const int idx = veh->install_part( point_zero, e->get_id(), true );
            REQUIRE( idx >= 0 );

            REQUIRE( veh->install_part( point_zero, vpart_id( "storage_battery" ), true ) >= 0 );
            veh->charge_battery( 10000 );

            auto ammo =
                ammotype( veh->turret_query( veh->parts[idx] ).base()->ammo_default().str() );

            if( veh->part_flag( idx, "USE_TANKS" ) ) {
                auto *tank = biggest_tank( ammo );
                REQUIRE( tank );
                INFO( tank->get_id().str() );

                auto tank_idx = veh->install_part( point_zero, tank->get_id(), true );
                REQUIRE( tank_idx >= 0 );
                REQUIRE( veh->parts[ tank_idx ].ammo_set( ammo->default_ammotype() ) );

            } else if( ammo ) {
                veh->parts[ idx].ammo_set( ammo->default_ammotype() );
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
