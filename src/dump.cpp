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

bool game::dump_stats( const std::string& what, dump_mode mode, const std::vector<std::string> &opts )
{
    try {
        load_core_data();
    } catch( const std::exception &err ) {
        std::cerr << "Error loading data from json: " << err.what() << std::endl;
        return false;
    }
    DynamicDataLoader::get_instance().finalize_loaded_data();

    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;

    int scol = 0; // sorting column

    std::map<std::string, standard_npc> test_npcs;
    test_npcs[ "S1" ] = standard_npc( "S1", { "gloves_survivor", "mask_lsurvivor" }, 4, 8, 10, 8, 10 /* DEX 10, PER 10 */ );
    test_npcs[ "S2" ] = standard_npc( "S2", { "gloves_fingerless", "sunglasses" }, 4, 8, 8, 8, 10 /* PER 10 */ );
    test_npcs[ "S3" ] = standard_npc( "S3", { "gloves_plate", "helmet_plate" },  4, 10, 8, 8, 8 /* STAT 10 */ );
    test_npcs[ "S4" ] = standard_npc( "S4", {}, 0, 8, 10, 8, 10 /* DEX 10, PER 10 */ );
    test_npcs[ "S5" ] = standard_npc( "S5", {}, 4, 8, 10, 8, 10 /* DEX 10, PER 10 */ );
    test_npcs[ "S6" ] = standard_npc( "S6", { "gloves_hsurvivor", "mask_hsurvivor" }, 4, 8, 10, 8, 10 /* DEX 10, PER 10 */ );

    std::map<std::string, item> test_items;
    test_items[ "G1" ] = item( "glock_19" ).ammo_set( "9mm" );
    test_items[ "G2" ] = item( "hk_mp5" ).ammo_set( "9mm" );
    test_items[ "G3" ] = item( "ar15" ).ammo_set( "223" );
    test_items[ "G4" ] = item( "remington_700" ).ammo_set( "270" );
    test_items[ "G4" ].emplace_back( "rifle_scope" );

    if( what == "AMMO" ) {
        header = {
            "Name", "Ammo", "Volume", "Weight", "Stack",
            "Range", "Dispersion", "Recoil", "Damage", "Pierce"
        };
        auto dump = [&rows]( const item& obj ) {
            // a common task is comparing ammo by type so ammo has multiple repeat the entry
            for( const auto &e : obj.type->ammo->type ) {
                std::vector<std::string> r;
                r.push_back( obj.tname( 1, false ) );
                r.push_back( e.str() );
                r.push_back( to_string( obj.volume() / units::legacy_volume_factor ) );
                r.push_back( to_string( obj.weight() ) );
                r.push_back( to_string( obj.type->stack_size ) );
                r.push_back( to_string( obj.type->ammo->range ) );
                r.push_back( to_string( obj.type->ammo->dispersion ) );
                r.push_back( to_string( obj.type->ammo->recoil ) );
                r.push_back( to_string( obj.type->ammo->damage ) );
                r.push_back( to_string( obj.type->ammo->pierce ) );
                rows.push_back( r );
            }
        };
        for( auto& e : item_controller->get_all_itypes() ) {
            if( e.second->ammo ) {
                dump( item( e.first, calendar::turn, item::solitary_tag {} ) );
            }
        }

    } else if( what == "ARMOR" ) {
        header = {
            "Name", "Encumber (fit)", "Warmth", "Weight", "Storage", "Coverage", "Bash", "Cut", "Acid", "Fire"
        };
        auto dump = [&rows]( const item& obj ) {
            std::vector<std::string> r;
            r.push_back( obj.tname( 1, false ) );
            r.push_back( to_string( obj.get_encumber() ) );
            r.push_back( to_string( obj.get_warmth() ) );
            r.push_back( to_string( obj.weight() ) );
            r.push_back( to_string( obj.get_storage() / units::legacy_volume_factor ) );
            r.push_back( to_string( obj.get_coverage() ) );
            r.push_back( to_string( obj.bash_resist() ) );
            r.push_back( to_string( obj.cut_resist() ) );
            r.push_back( to_string( obj.acid_resist() ) );
            r.push_back( to_string( obj.fire_resist() ) );
            rows.push_back( r );
        };

        body_part bp = opts.empty() ? num_bp : get_body_part_token( opts.front() );

        for( auto& e : item_controller->get_all_itypes() ) {
            if( e.second->armor ) {
                item obj( e.first );
                if( bp == num_bp || obj.covers( bp ) ) {
                    if( obj.has_flag( "VARSIZE" ) ) {
                        obj.item_tags.insert( "FIT" );
                    }
                    dump( obj );
                }
            }
        }

    } else if( what == "EDIBLE" ) {
        header = {
            "Name", "Volume", "Weight", "Stack", "Calories", "Quench", "Healthy"
        };
        for( const auto& v : vitamin::all() ) {
             header.push_back( v.second.name() );
        }
        auto dump = [&rows,&test_npcs]( const item& obj ) {
            std::vector<std::string> r;
            r.push_back( obj.tname( false ) );
            r.push_back( to_string( obj.volume() / units::legacy_volume_factor ) );
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
            "Range", "Dispersion", "Effective recoil", "Damage", "Pierce",
            "Aim time", "Effective range", "Snapshot range", "Max range"
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

        auto dump = [&rows,&locations]( const standard_npc &who, const item& obj ) {
            std::vector<std::string> r;
            r.push_back( obj.tname( 1, false ) );
            r.push_back( obj.ammo_type() ? obj.ammo_type().str() : "" );
            r.push_back( to_string( obj.volume() / units::legacy_volume_factor ) );
            r.push_back( to_string( obj.weight() ) );
            r.push_back( to_string( obj.ammo_capacity() ) );
            r.push_back( to_string( obj.gun_range() ) );
            r.push_back( to_string( obj.gun_dispersion() ) );
            r.push_back( to_string( obj.gun_recoil( who ) ) );
            r.push_back( to_string( obj.gun_damage() ) );
            r.push_back( to_string( obj.gun_pierce() ) );

            r.push_back( to_string( who.gun_engagement_moves( obj ) ) );

            r.push_back( string_format( "%.1f", who.gun_engagement_range( obj, player::engagement::effective ) ) );
            r.push_back( string_format( "%.1f", who.gun_engagement_range( obj, player::engagement::snapshot ) ) );
            r.push_back( string_format( "%.1f", who.gun_engagement_range( obj, player::engagement::maximum ) ) );

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

                dump( test_npcs[ "S1" ], gun );

                if( gun.type->gun->barrel_length > 0 ) {
                    gun.emplace_back( "barrel_small" );
                    dump( test_npcs[ "S1" ], gun );
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
            r.push_back( to_string( obj->size / units::legacy_volume_factor ) );
            rows.push_back( r );
        };
        for( const auto e : vpart_info::get_all() ) {
            dump( e );
        }

    } else if( what == "AIMING" ) {
        scol = -1; // unsorted output so graph columns have predictable ordering

        const int cycles = 1400;

        header = { "Name" };
        for( int i = 0; i <= cycles; ++i ) {
            header.push_back( to_string( i ) );
        }

        auto dump = [&rows]( const standard_npc &who, const item &gun) {
            std::vector<std::string> r( 1, string_format( "%s %s", who.get_name().c_str(), gun.tname().c_str() ) );
            double penalty = MIN_RECOIL;
            for( int i = 0; i <= cycles; ++i ) {
                penalty -= who.aim_per_move( gun, penalty );
                r.push_back( string_format( "%.2f", who.gun_current_range( gun, penalty ) ) );
            }
            rows.push_back( r );
        };

        if( opts.empty() ) {
            dump( test_npcs[ "S1" ], test_items[ "G1" ] );
            dump( test_npcs[ "S1" ], test_items[ "G2" ] );
            dump( test_npcs[ "S1" ], test_items[ "G3" ] );
            dump( test_npcs[ "S1" ], test_items[ "G4" ] );

        } else {
            for( const auto &str : opts ) {
                auto idx = str.find( ':' );
                if( idx == std::string::npos ) {
                    std::cerr << "cannot parse test case: " << str << std::endl;
                    return false;
                }
                auto test = std::make_pair( test_npcs.find( str.substr( 0, idx ) ),
                                            test_items.find( str.substr( idx + 1 ) ) );

                if( test.first == test_npcs.end() || test.second == test_items.end() ) {
                    std::cerr << "invalid test case: " << str << std::endl;
                    return false;
                }

                dump( test.first->second, test.second->second );
            }
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

    } else {
        std::cerr << "unknown argument: " << what << std::endl;
        return false;
    }

    rows.erase( std::remove_if( rows.begin(), rows.end(), []( const std::vector<std::string>& e ) {
        return e.empty();
    } ), rows.end() );

    if( scol >= 0 ) {
        std::sort( rows.begin(), rows.end(), [&scol]( const std::vector<std::string>& lhs, const std::vector<std::string>& rhs ) {
            return lhs[ scol ] < rhs[ scol ];
        } );
    }

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

    return true;
}
