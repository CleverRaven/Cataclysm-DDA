#include "game.h"

#include <iostream>

#include "init.h"
#include "item_factory.h"
#include "player.h"
#include "vehicle.h"
#include "veh_type.h"

void game::dump_stats( const std::string& what )
{
    load_core_data();
    DynamicDataLoader::get_instance().finalize_loaded_data();

    if( what == "GUN" ) {
        std::cout
            << "Name" << "\t"
            << "Ammo" << "\t"
            << "Volume" << "\t"
            << "Weight" << "\t"
            << "Capacity" << "\t"
            << "Range" << "\t"
            << "Dispersion" << "\t"
            << "Recoil" << "\t"
            << "Damage" << "\t"
            << "Pierce" << std::endl;

        auto dump = []( const item& gun ) {
            std::cout
                << gun.tname( false ) << "\t"
                << ( gun.ammo_type() != "NULL" ? gun.ammo_type() : "" ) << "\t"
                << gun.volume() << "\t"
                << gun.weight() << "\t"
                << gun.ammo_capacity() << "\t"
                << gun.gun_range() << "\t"
                << gun.gun_dispersion() << "\t"
                << gun.gun_recoil() << "\t"
                << gun.gun_damage() << "\t"
                << gun.gun_pierce() << std::endl;
        };

        for( auto& e : item_controller->get_all_itypes() ) {
            if( e.second->gun.get() ) {
                item gun( e.first );
                if( gun.is_reloadable() ) {
                    gun.ammo_set( default_ammo( gun.ammo_type() ), gun.ammo_capacity() );
                }
                dump( gun );

                if( gun.type->gun->barrel_length > 0 ) {
                    gun.emplace_back( "barrel_small" );
                    dump( gun );
                }
            }
        }
    } else if( what == "AMMO" ) {
        std::cout
            << "Name" << "\t"
            << "Ammo" << "\t"
            << "Volume" << "\t"
            << "Weight" << "\t"
            << "Stack" << "\t"
            << "Range" << "\t"
            << "Dispersion" << "\t"
            << "Recoil" << "\t"
            << "Damage" << "\t"
            << "Pierce" << std::endl;

        auto dump = []( const item& ammo ) {
            std::cout
                << ammo.tname( false ) << "\t"
                << ammo.type->ammo->type << "\t"
                << ammo.volume() << "\t"
                << ammo.weight() << "\t"
                << ammo.type->stack_size << "\t"
                << ammo.type->ammo->range << "\t"
                << ammo.type->ammo->dispersion << "\t"
                << ammo.type->ammo->recoil << "\t"
                << ammo.type->ammo->damage << "\t"
                << ammo.type->ammo->pierce << std::endl;
        };

        for( auto& e : item_controller->get_all_itypes() ) {
            if( e.second->ammo.get() ) {
                dump( item( e.first, calendar::turn, item::solitary_tag {} ) );
            }
        }
    } else if( what == "VEHICLE" ) {
        std::cout
            << "Name" << "\t"
            << "Weight (empty)" << "\t"
            << "Weight (fueled)" << std::endl;

        for( auto& e : vehicle_prototype::get_all() ) {
            auto veh_empty = vehicle( e, 0, 0 );
            auto veh_fueled = vehicle( e, 100, 0 );
            std::cout
                << veh_empty.name << "\t"
                << veh_empty.total_mass() << "\t"
                << veh_fueled.total_mass() << std::endl;
        }
    } else if( what == "VPART" ) {
        std::cout
            << "Name" << "\t"
            << "Location" << "\t"
            << "Weight" << "\t"
            << "Size" << std::endl;

        for( const auto e : vpart_info::get_all() ) {
            std::cout
                << e->name() << "\t"
                << e->location << "\t"
                << ceil( item( e->item ).weight() / 1000.0 ) << "\t"
                << e->size << std::endl;
        }
    }
}
