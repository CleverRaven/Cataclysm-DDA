#include "catch/catch.hpp"

#include "game.h"
#include "player.h"
#include "visitable.h"
#include "itype.h"
#include "map.h"
#include "rng.h"
#include "vehicle.h"
#include "veh_type.h"
#include "vehicle_selector.h"
#include "npc.h"

static const efftype_id effect_on_roof( "on_roof" );


std::vector<const vpart_info *> find_turret_types()
{
    std::vector<const vpart_info *> ret;
    const auto &all_vparts = vpart_info::get_all();
    for( const vpart_info *vp : all_vparts ) {
        if( !vp->has_flag( "TURRET" ) ) {
            continue;
        }

        ret.push_back( vp );
    }

    return ret;
}

std::map<ammotype, const vpart_info *> find_ammo_tanks()
{
    std::map<ammotype, const vpart_info *> ret;
    const auto &all_vparts = vpart_info::get_all();
    for( const vpart_info *vp : all_vparts ) {
        if( !vp->has_flag( VPFLAG_FUEL_TANK ) ) {
            continue;
        }

        // Find the biggest holder
        ammotype atp = ammotype( vp->fuel_type );
        if( ret[ atp ] == nullptr || ret[ atp ]->size < vp->size ) {
            ret[ atp ] = vp;
        }
    }

    return ret;
}

TEST_CASE( "All turrets can actually fire" )
{
    const auto turret_types = find_turret_types();
    const auto ammo_tanks = find_ammo_tanks();

    for( const vpart_info *turret_type : turret_types ) {
        WHEN( "Vehicle part is a turret" ) {
            THEN( "It needs no ammo, has CARGO flag or an ammo tank for it is defined" ) {
                bool null_ammo = item( turret_type->item ).ammo_type() == "null";
                bool cargo = turret_type->has_flag( VPFLAG_CARGO );
                bool ammo_tank = ammo_tanks.count( item( turret_type->item ).ammo_type() ) > 0;
                REQUIRE( null_ammo + cargo + ammo_tank );
            }
        }

        const std::string tur_name = turret_type->name();

        GIVEN( "A vehicle with a \"" + tur_name + "\" turret, a storage battery and possibly ammo tank" ) {
            vehicle *veh = g->m.add_vehicle( vproto_id( "none" ), 65, 65, 270, 0, 0 );
            REQUIRE( veh != nullptr );
            veh->install_part( 0, 0, vpart_str_id( "storage_battery" ), -1, true );
            veh->charge_battery( 10000, false );

            const int turret_part_index = veh->install_part( 0, 0, turret_type->id, -1, true );
            REQUIRE( turret_part_index >= 0 );

            const auto turret_ammo_type = veh->part_base( turret_part_index )->ammo_type();
            const auto turret_default_ammo = veh->part_base( turret_part_index )->ammo_default();

            const auto tank_iter = ammo_tanks.find( turret_ammo_type );
            int tank_index = -1;
            if( tank_iter != ammo_tanks.end() ) {
                tank_index = veh->install_part( 0, 0, tank_iter->second->id, -1, true );
            }

            vehicle_part &turret_part = veh->parts[ turret_part_index ];
            THEN( "The turret can be set into DEFAULT or AUTO mode" ) {
                bool has_default = veh->turret_set_mode( turret_part, "DEFAULT" );
                bool has_auto = veh->turret_set_mode( turret_part, "AUTO" );
                REQUIRE( has_default + has_auto );
            }

            if( (bool)item( turret_type->item ).ammo_type() ) {
                WHEN( "The turret has a non-null ammo type" ) {
                    if( tank_index >= 0 ) {
                        AND_WHEN( "It has an ammo tank associated with its type" ) {
                            THEN( "The tank can be filled with default ammo" ) {
                                REQUIRE( tank_index >= 0 );
                                REQUIRE( veh->parts[ tank_index ].ammo_set( turret_default_ammo ) );
                            }
                        }
                    }

                    if( turret_type->has_flag( VPFLAG_CARGO ) ) {
                        AND_WHEN( "It has a CARGO flag" ) {
                            THEN( "It can be loaded with ammo_capacity charges of default ammo" ) {
                                REQUIRE( veh->add_item( turret_part_index,
                                         item( turret_default_ammo, calendar::turn,
                                               item( turret_type->item ).ammo_capacity() ) ) );
                                AND_THEN( "The ammo is there after loading" ) {
                                    REQUIRE( !veh->get_items( turret_part_index ).empty() );
                                    REQUIRE( veh->get_items( turret_part_index ).rbegin()->typeId() == turret_default_ammo );
                                    AND_THEN( "It can be reloaded" ) {
                                        veh->turret_reload( turret_part );
                                    }
                                }
                            }
                        }
                    }
                }
            }

            WHEN( "The turret is reloaded" ) {
                veh->turret_reload( turret_part );
                THEN( "It is ready to fire" ) {
                    REQUIRE( veh->turret_query( turret_part ) == vehicle::turret_status::ready );

                    AND_THEN( "It has positive range" ) {
                        REQUIRE( veh->part_base( turret_part_index )->gun_range( true ) > 0 );
                    }
                }
            }

            WHEN( "The turret fires" ) {
                tripoint pos = veh->global_part_pos3( turret_part );
                int range = std::min( 10, veh->part_base( turret_part_index )->gun_range( true ) );
                tripoint targ = pos + point( range, 0 );

                int shots = veh->turret_fire( turret_part, targ );
                THEN( "The number of fired shots is positive" ) {
                    REQUIRE( shots > 0 );
                }

                THEN( "It unloads with no errors" ) {
                    veh->turret_unload( turret_part );
                }

                THEN( "The game doesn't debugmsg until the next turn" ) {
                    //g->do_turn();
                }
            }

            g->m.destroy_vehicle( veh );
        }
    }
}
