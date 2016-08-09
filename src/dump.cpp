#include "game.h"

#include <algorithm>
#include <iostream>
#include <iterator>

#include "compatibility.h"
#include "init.h"
#include "item_factory.h"
#include "iuse_actor.h"
#include "player.h"
#include "vehicle.h"
#include "veh_type.h"
#include "npc.h"

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
            r.push_back( obj.tname( 1, false ) );
            r.push_back( obj.type->ammo->type.str() );
            r.push_back( to_string( obj.volume() ) );
            r.push_back( to_string( obj.weight() ) );
            r.push_back( to_string( obj.type->stack_size ) );
            r.push_back( to_string( obj.type->ammo->range ) );
            r.push_back( to_string( obj.type->ammo->dispersion ) );
            r.push_back( to_string( obj.type->ammo->recoil ) );
            r.push_back( to_string( obj.type->ammo->damage ) );
            r.push_back( to_string( obj.type->ammo->pierce ) );
            rows.push_back( r );
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
             header.push_back( v.second.name() );
        }
        auto dump = [&rows]( const item& obj ) {
            std::vector<std::string> r;
            r.push_back( obj.tname( false ) );
            r.push_back( to_string( obj.volume() ) );
            r.push_back( to_string( obj.weight() ) );
            r.push_back( to_string( obj.type->stack_size ) );
            r.push_back( to_string( obj.type->comestible->get_calories() ) );
            r.push_back( to_string( obj.type->comestible->quench ) );
            r.push_back( to_string( obj.type->comestible->healthy ) );
            auto vits = g->u.vitamins_from( obj );
            for( const auto& v : vitamin::all() ) {
                 r.push_back( to_string( vits[ v.first ] ) );
            }
            rows.push_back( r );
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
            "Range", "Dispersion", "Recoil", "Damage", "Pierce",
            "eff-min", "eff-max", "abs-max"
        };

        std::set<std::string> locations;
        for( const auto& e : item_controller->get_all_itypes() ) {
            if( e.second->gun ) {
                std::transform( e.second->gun->valid_mod_locations.begin(),
                                e.second->gun->valid_mod_locations.end(),
                                std::inserter( locations, locations.begin() ),
                                []( const std::pair<std::string, int>& e ) { return e.first; } );
            }
        }
        for( const auto &e : locations ) {
            header.push_back( e );
        }

        auto dump = [&rows,&locations]( const item& obj ) {
            std::vector<std::string> r;
            r.push_back( obj.tname( 1, false ) );
            r.push_back( obj.ammo_type() ? obj.ammo_type().str() : "" );
            r.push_back( to_string( obj.volume() ) );
            r.push_back( to_string( obj.weight() ) );
            r.push_back( to_string( obj.ammo_capacity() ) );
            r.push_back( to_string( obj.gun_range() ) );
            r.push_back( to_string( obj.gun_dispersion() ) );
            r.push_back( to_string( obj.gun_recoil() ) );
            r.push_back( to_string( obj.gun_damage() ) );
            r.push_back( to_string( obj.gun_pierce() ) );

            standard_npc fake;
            fake.wear_item( item( "gloves_lsurvivor" ) );
            fake.wear_item( item( "mask_lsurvivor" ) );
            fake.set_skill_level( skill_id( "gun" ), 4 );
            fake.set_skill_level( obj.gun_skill(), 4 );
            fake.recoil = MIN_RECOIL;

            r.push_back( string_format( "%.1f", fake.gun_engagement_range( obj, player::engagement::effective_min ) ) );
            r.push_back( string_format( "%.1f", fake.gun_engagement_range( obj, player::engagement::effective_max ) ) );
            r.push_back( string_format( "%.1f", fake.gun_engagement_range( obj, player::engagement::absolute_max ) ) );

            for( const auto &e : locations ) {
                r.push_back( to_string( obj.type->gun->valid_mod_locations[ e ] ) );
            }
            rows.push_back( r );
        };
        for( const auto& e : item_controller->get_all_itypes() ) {
            if( e.second->gun ) {
                item gun( e.first );
                if( !gun.magazine_integral() ) {
                    gun.emplace_back( gun.magazine_default() );
                }
                gun.ammo_set( default_ammo( gun.ammo_type() ), gun.ammo_capacity() );

                dump( gun );

                if( gun.type->gun->barrel_length > 0 ) {
                    gun.emplace_back( "barrel_small" );
                    dump( gun );
                }
            }
        }

    } else if( what == "VEHICLE" ) {
        header = {
            "Name", "Weight (empty)", "Weight (fueled)",
            "Max velocity (mph)", "Safe velocity (mph)", "Acceleration (mph/turn)",
            "Mass coeff %", "Aerodynamics coeff %", "Friction coeff %",
            "Traction coeff % (grass)"
        };
        auto dump = [&rows]( const vproto_id& obj ) {
            auto veh_empty = vehicle( obj, 0, 0 );
            auto veh_fueled = vehicle( obj, 100, 0 );

            std::vector<std::string> r;
            r.push_back( veh_empty.name );
            r.push_back( to_string( veh_empty.total_mass() ) );
            r.push_back( to_string( veh_fueled.total_mass() ) );
            r.push_back( to_string( veh_fueled.max_velocity() / 100 ) );
            r.push_back( to_string( veh_fueled.safe_velocity() / 100 ) );
            r.push_back( to_string( veh_fueled.acceleration() / 100 ) );
            r.push_back( to_string( (int)( 100 * veh_fueled.k_mass() ) ) );
            r.push_back( to_string( (int)( 100 * veh_fueled.k_aerodynamics() ) ) );
            r.push_back( to_string( (int)( 100 * veh_fueled.k_friction() ) ) );
            r.push_back( to_string( (int)( 100 * veh_fueled.k_traction( veh_fueled.wheel_area( false ) / 2.0f ) ) ) );
            rows.push_back( r );
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
            r.push_back( obj->name() );
            r.push_back( obj->location );
            r.push_back( to_string( int( ceil( item( obj->item ).weight() / 1000.0 ) ) ) );
            r.push_back( to_string( obj->size ) );
            rows.push_back( r );
        };
        for( const auto e : vpart_info::get_all() ) {
            dump( e );
        }
    } else if( what == "EXPLOSIVE" ) {
        header = {
            // @todo Should display more useful data: shrapnel damage, safe range
            "Name", "Power", "Power at 5 tiles", "Power halves at", "Shrapnel count", "Shrapnel mass"
        };

        auto dump = [&rows]( const std::string &name, const explosion_data &ex ) {
            std::vector<std::string> r;
            r.push_back( name );
            r.push_back( to_string( ex.power ) );
            r.push_back( string_format( "%.1f", ex.power_at_range( 5.0f ) ) );
            r.push_back( string_format( "%.1f", ex.expected_range( 0.5f ) ) );
            r.push_back( to_string( ex.shrapnel.count ) );
            r.push_back( to_string( ex.shrapnel.mass ) );
            rows.push_back( r );
        };
        for( const auto& e : item_controller->get_all_itypes() ) {
            const auto &itt = *e.second;
            const auto use = itt.get_use( "explosion" );
            if( use != nullptr && use->get_actor_ptr() != nullptr ) {
                const auto actor = dynamic_cast<const explosion_iuse *>( use->get_actor_ptr() );
                if( actor != nullptr ) {
                    dump( itt.nname( 1 ), actor->explosion );
                }
            }

            auto c_ex = dynamic_cast<const explosion_iuse *>( itt.countdown_action.get_actor_ptr() );
            if( c_ex != nullptr ) {
                dump( itt.nname( 1 ), c_ex->explosion );
            }
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
