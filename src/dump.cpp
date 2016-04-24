#include "game.h"

#include <iostream>
#include <iterator>

#include "init.h"
#include "item_factory.h"
#include "player.h"
#include "vehicle.h"
#include "veh_type.h"

void game::dump_stats( const std::string& what, dump_mode mode )
{
    load_core_data();
    DynamicDataLoader::get_instance().finalize_loaded_data();

    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;

    if( what == "AMMO" ) {
        header = {
            "Name", "Ammo", "Volume", "Weight", "Stack",
            "Range", "Dispersion", "Recoil", "Damage", "Pierce"
        };
        auto dump = [&rows]( const item& obj ) {
            std::vector<std::string> r;
            r.emplace_back( obj.tname( false ) );
            r.emplace_back( obj.type->ammo->type );
            r.emplace_back( std::to_string( obj.volume() ) );
            r.emplace_back( std::to_string( obj.weight() ) );
            r.emplace_back( std::to_string( obj.type->stack_size ) );
            r.emplace_back( std::to_string( obj.type->ammo->range ) );
            r.emplace_back( std::to_string( obj.type->ammo->dispersion ) );
            r.emplace_back( std::to_string( obj.type->ammo->recoil ) );
            r.emplace_back( std::to_string( obj.type->ammo->damage ) );
            r.emplace_back( std::to_string( obj.type->ammo->pierce ) );
            rows.emplace_back( r );
        };
        for( auto& e : item_controller->get_all_itypes() ) {
            if( e.second->ammo ) {
                dump( item( e.first, calendar::turn, item::solitary_tag {} ) );
            }
        }

    } else if( what == "EDIBLE" ) {
        header = {
            "Name", "Volume", "Weight", "Stack", "Calories", "Quench", "Healthy"
        };
        for( const auto& v : vitamin::all() ) {
             header.emplace_back( v.second.name() );
        }
        auto dump = [&rows]( const item& obj ) {
            std::vector<std::string> r;
            r.emplace_back( obj.tname( false ) );
            r.emplace_back( std::to_string( obj.volume() ) );
            r.emplace_back( std::to_string( obj.weight() ) );
            r.emplace_back( std::to_string( obj.type->stack_size ) );
            r.emplace_back( std::to_string( obj.type->comestible->get_calories() ) );
            r.emplace_back( std::to_string( obj.type->comestible->quench ) );
            r.emplace_back( std::to_string( obj.type->comestible->healthy ) );
            auto vits = g->u.vitamins_from( obj );
            for( const auto& v : vitamin::all() ) {
                 r.emplace_back( std::to_string( vits[ v.first ] ) );
            }
            rows.emplace_back( r );
        };
        for( auto& e : item_controller->get_all_itypes() ) {
            if( e.second->comestible &&
                ( e.second->comestible->comesttype == "FOOD" ||
                  e.second->comestible->comesttype == "DRINK" ) ) {

                item food( e.first, calendar::turn, item::solitary_tag {} );
                if( g->u.can_eat( food, false, true ) == EDIBLE ) {
                    dump( food );
                }
            }
        }

    } else if( what == "GUN" ) {
        header = {
            "Name", "Ammo", "Volume", "Weight", "Capacity",
            "Range", "Dispersion", "Recoil", "Damage", "Pierce"
        };
        auto dump = [&rows]( const item& obj ) {
            std::vector<std::string> r;
            r.emplace_back( obj.tname( false ) );
            r.emplace_back( obj.ammo_type() != "NULL" ? obj.ammo_type() : "" );
            r.emplace_back( std::to_string( obj.volume() ) );
            r.emplace_back( std::to_string( obj.weight() ) );
            r.emplace_back( std::to_string( obj.ammo_capacity() ) );
            r.emplace_back( std::to_string( obj.gun_range() ) );
            r.emplace_back( std::to_string( obj.gun_dispersion() ) );
            r.emplace_back( std::to_string( obj.gun_recoil() ) );
            r.emplace_back( std::to_string( obj.gun_damage() ) );
            r.emplace_back( std::to_string( obj.gun_pierce() ) );
            rows.emplace_back( r );
        };
        for( auto& e : item_controller->get_all_itypes() ) {
            if( e.second->gun ) {
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

    } else if( what == "VEHICLE" ) {
        header = {
            "Name", "Weight (empty)", "Weight (fueled)"
        };
        auto dump = [&rows]( const vproto_id& obj ) {
            auto veh_empty = vehicle( obj, 0, 0 );
            auto veh_fueled = vehicle( obj, 100, 0 );

            std::vector<std::string> r;
            r.emplace_back( veh_empty.name );
            r.emplace_back( std::to_string( veh_empty.total_mass() ) );
            r.emplace_back( std::to_string( veh_fueled.total_mass() ) );
            rows.emplace_back( r );
        };
        for( auto& e : vehicle_prototype::get_all() ) {
            dump( e );
        }

    } else if( what == "VPART" ) {
        header = {
            "Name", "Location", "Weight", "Size"
        };
        auto dump = [&rows]( const vpart_info *obj ) {
            std::vector<std::string> r;
            r.emplace_back( obj->name() );
            r.emplace_back( obj->location );
            r.emplace_back( std::to_string( int( ceil( item( obj->item ).weight() / 1000.0 ) ) ) );
            r.emplace_back( std::to_string( obj->size ) );
            rows.emplace_back( r );
        };
        for( const auto e : vpart_info::get_all() ) {
            dump( e );
        }
    }

    rows.erase( std::remove_if( rows.begin(), rows.end(), []( const std::vector<std::string>& e ) {
        return e.empty();
    } ), rows.end() );

    std::sort( rows.begin(), rows.end(), []( const std::vector<std::string>& lhs, const std::vector<std::string>& rhs ) {
        return lhs[ 0 ] < rhs[ 0 ];
    } );

    rows.erase( std::unique( rows.begin(), rows.end() ), rows.end() );

    switch( mode ) {
        case dump_mode::TSV:
            rows.insert( rows.begin(), header );
            for( const auto& r : rows ) {
                std::copy( r.begin(), r.end() - 1, std::ostream_iterator<std::string>( std::cout, "\t" ) );
                std::cout << r.back() << "\n";
            }
            break;

        case dump_mode::HTML:
            std::cout << "<table>";

            std::cout << "<thead>";
            std::cout << "<tr>";
            for( const auto& col : header ) {
                std::cout << "<th>" << col << "</th>";
            }
            std::cout << "</tr>";
            std::cout << "</thead>";

            std::cout << "<tdata>";
            for( const auto& r : rows ) {
                std::cout << "<tr>";
                for( const auto& col : r ) {
                    std::cout << "<td>" << col << "</td>";
                }
                std::cout << "</tr>";
            }
            std::cout << "</tdata>";

            std::cout << "</table>";
            break;
    }
}
