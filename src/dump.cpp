#include "game.h"

#include <algorithm>
#include <iostream>
#include <iterator>

#include "compatibility.h"
#include "init.h"
#include "item_factory.h"
#include "iuse_actor.h"
#include "recipe_dictionary.h"
#include "player.h"
#include "vehicle.h"
#include "string_formatter.h"
#include "veh_type.h"
#include "skill.h"
#include "vitamin.h"
#include "npc.h"
#include "ammo.h"
#include "crafting.h"
#include "loading_ui.h"

bool game::dump_stats( const std::string &what, dump_mode mode,
                       const std::vector<std::string> &opts )
{
    try {
        loading_ui ui( false );
        load_core_data( ui );
        load_packs( _( "Loading content packs" ), { mod_id( "dda" ) }, ui );
        DynamicDataLoader::get_instance().finalize_loaded_data( ui );
    } catch( const std::exception &err ) {
        std::cerr << "Error loading data from json: " << err.what() << std::endl;
        return false;
    }

    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;

    int scol = 0; // sorting column

    std::map<std::string, standard_npc> test_npcs;
    test_npcs[ "S1" ] = standard_npc( "S1", { "gloves_survivor", "mask_lsurvivor" }, 4, 8, 10, 8,
                                      10 /* DEX 10, PER 10 */ );
    test_npcs[ "S2" ] = standard_npc( "S2", { "gloves_fingerless", "sunglasses" }, 4, 8, 8, 8,
                                      10 /* PER 10 */ );
    test_npcs[ "S3" ] = standard_npc( "S3", { "gloves_plate", "helmet_plate" },  4, 10, 8, 8,
                                      8 /* STAT 10 */ );
    test_npcs[ "S4" ] = standard_npc( "S4", {}, 0, 8, 10, 8, 10 /* DEX 10, PER 10 */ );
    test_npcs[ "S5" ] = standard_npc( "S5", {}, 4, 8, 10, 8, 10 /* DEX 10, PER 10 */ );
    test_npcs[ "S6" ] = standard_npc( "S6", { "gloves_hsurvivor", "mask_hsurvivor" }, 4, 8, 10, 8,
                                      10 /* DEX 10, PER 10 */ );

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
        auto dump = [&rows]( const item & obj ) {
            // a common task is comparing ammo by type so ammo has multiple repeat the entry
            for( const auto &e : obj.type->ammo->type ) {
                std::vector<std::string> r;
                r.push_back( obj.tname( 1, false ) );
                r.push_back( e.str() );
                r.push_back( to_string( obj.volume() / units::legacy_volume_factor ) );
                r.push_back( to_string( to_gram( obj.weight() ) ) );
                r.push_back( to_string( obj.type->stack_size ) );
                r.push_back( to_string( obj.type->ammo->range ) );
                r.push_back( to_string( obj.type->ammo->dispersion ) );
                r.push_back( to_string( obj.type->ammo->recoil ) );
                damage_instance damage = obj.type->ammo->damage;
                r.push_back( to_string( damage.total_damage() ) );
                r.push_back( to_string( damage.empty() ? 0 : ( *damage.begin() ).res_pen ) );
                rows.push_back( r );
            }
        };
        for( const itype *e : item_controller->all() ) {
            if( e->ammo ) {
                dump( item( e, calendar::turn, item::solitary_tag {} ) );
            }
        }

    } else if( what == "ARMOR" ) {
        header = {
            "Name", "Encumber (fit)", "Warmth", "Weight", "Storage", "Coverage", "Bash", "Cut", "Acid", "Fire"
        };
        auto dump = [&rows]( const item & obj ) {
            std::vector<std::string> r;
            r.push_back( obj.tname( 1, false ) );
            r.push_back( to_string( obj.get_encumber() ) );
            r.push_back( to_string( obj.get_warmth() ) );
            r.push_back( to_string( to_gram( obj.weight() ) ) );
            r.push_back( to_string( obj.get_storage() / units::legacy_volume_factor ) );
            r.push_back( to_string( obj.get_coverage() ) );
            r.push_back( to_string( obj.bash_resist() ) );
            r.push_back( to_string( obj.cut_resist() ) );
            r.push_back( to_string( obj.acid_resist() ) );
            r.push_back( to_string( obj.fire_resist() ) );
            rows.push_back( r );
        };

        body_part bp = opts.empty() ? num_bp : get_body_part_token( opts.front() );

        for( const itype *e : item_controller->all() ) {
            if( e->armor ) {
                item obj( e );
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
        for( const auto &v : vitamin::all() ) {
            header.push_back( v.second.name() );
        }
        auto dump = [&rows]( const item & obj ) {
            std::vector<std::string> r;
            r.push_back( obj.tname( false ) );
            r.push_back( to_string( obj.volume() / units::legacy_volume_factor ) );
            r.push_back( to_string( to_gram( obj.weight() ) ) );
            r.push_back( to_string( obj.type->stack_size ) );
            r.push_back( to_string( obj.type->comestible->get_calories() ) );
            r.push_back( to_string( obj.type->comestible->quench ) );
            r.push_back( to_string( obj.type->comestible->healthy ) );
            auto vits = g->u.vitamins_from( obj );
            for( const auto &v : vitamin::all() ) {
                r.push_back( to_string( vits[ v.first ] ) );
            }
            rows.push_back( r );
        };

        for( const itype *e : item_controller->all() ) {
            item food( e, calendar::turn, item::solitary_tag {} );

            if( food.is_food() && g->u.can_eat( food ).success() ) {
                dump( food );
            }
        }

    } else if( what == "GUN" ) {
        header = {
            "Name", "Ammo", "Volume", "Weight", "Capacity",
            "Range", "Dispersion", "Effective recoil", "Damage", "Pierce",
            "Aim time", "Effective range", "Snapshot range", "Max range"
        };

        std::set<std::string> locations;
        for( const itype *e : item_controller->all() ) {
            if( e->gun ) {
                std::transform( e->gun->valid_mod_locations.begin(),
                                e->gun->valid_mod_locations.end(),
                                std::inserter( locations, locations.begin() ),
                                []( const std::pair<gunmod_location, int>& q ) { return q.first.name(); } );
            }
        }
        for( const auto &e : locations ) {
            header.push_back( e );
        }

        auto dump = [&rows, &locations]( const standard_npc & who, const item & obj ) {
            std::vector<std::string> r;
            r.push_back( obj.tname( 1, false ) );
            r.push_back( obj.ammo_type() ? obj.ammo_type().str() : "" );
            r.push_back( to_string( obj.volume() / units::legacy_volume_factor ) );
            r.push_back( to_string( to_gram( obj.weight() ) ) );
            r.push_back( to_string( obj.ammo_capacity() ) );
            r.push_back( to_string( obj.gun_range() ) );
            r.push_back( to_string( obj.gun_dispersion() ) );
            r.push_back( to_string( obj.gun_recoil( who ) ) );
            damage_instance damage = obj.gun_damage();
            r.push_back( to_string( damage.total_damage() ) );
            r.push_back( to_string( damage.empty() ? 0 : ( *damage.begin() ).res_pen ) );

            r.push_back( to_string( who.gun_engagement_moves( obj ) ) );

            for( const auto &e : locations ) {
                const auto &vml = obj.type->gun->valid_mod_locations;
                const auto iter = vml.find( e );
                r.push_back( to_string( iter != vml.end() ? iter->second : 0 ) );
            }
            rows.push_back( r );
        };
        for( const itype *e : item_controller->all() ) {
            if( e->gun ) {
                item gun( e );
                if( !gun.magazine_integral() ) {
                    gun.emplace_back( gun.magazine_default() );
                }
                gun.ammo_set( gun.ammo_type()->default_ammotype(), gun.ammo_capacity() );

                dump( test_npcs[ "S1" ], gun );

                if( gun.type->gun->barrel_length > 0 ) {
                    gun.emplace_back( "barrel_small" );
                    dump( test_npcs[ "S1" ], gun );
                }
            }
        }

    } else if( what == "RECIPE" ) {

        // optionally filter recipes to include only those using specified skills
        recipe_subset dict;
        for( const auto &r : recipe_dict ) {
            if( opts.empty() || std::any_of( opts.begin(), opts.end(), [&r]( const std::string &s ) {
                if( r.second.skill_used == skill_id( s ) && r.second.difficulty > 0 ) {
                    return true;
                }
                auto iter = r.second.required_skills.find( skill_id( s ) );
                return iter != r.second.required_skills.end() && iter->second > 0;
            } ) ) {
                dict.include( &r.second );
            }
        }

        // only consider skills that are required by at least one recipe
        std::vector<Skill> sk;
        std::copy_if( Skill::skills.begin(), Skill::skills.end(), std::back_inserter( sk ), [&dict]( const Skill &s ) {
            return std::any_of( dict.begin(), dict.end(), [&s]( const recipe *r ) {
                return r->skill_used == s.ident() || r->required_skills.find( s.ident() ) != r->required_skills.end();
            } );
        } );

        header = { "Result" };

        for( const auto &e : sk ) {
            header.push_back( e.ident().str() );
        }

        for( const recipe *e : dict ) {
            std::vector<std::string> r;
            r.push_back( e->result_name() );
            for( const auto &s : sk ) {
                if( e->skill_used == s.ident() ) {
                    r.push_back( to_string( e->difficulty ) );
                } else {
                    auto iter = e->required_skills.find( s.ident() );
                    r.push_back( to_string( iter != e->required_skills.end() ? iter->second : 0 ) );
                }
            }
            rows.push_back( r );
        }

    } else if( what == "VEHICLE" ) {
        header = {
            "Name", "Weight (empty)", "Weight (fueled)",
            "Max velocity (mph)", "Safe velocity (mph)", "Acceleration (mph/turn)",
            "Mass coeff %", "Aerodynamics coeff %", "Friction coeff %",
            "Traction coeff % (grass)"
        };
        auto dump = [&rows]( const vproto_id & obj ) {
            auto veh_empty = vehicle( obj, 0, 0 );
            auto veh_fueled = vehicle( obj, 100, 0 );

            std::vector<std::string> r;
            r.push_back( veh_empty.name );
            r.push_back( to_string( to_kilogram( veh_empty.total_mass() ) ) );
            r.push_back( to_string( to_kilogram( veh_fueled.total_mass() ) ) );
            r.push_back( to_string( veh_fueled.max_velocity() / 100 ) );
            r.push_back( to_string( veh_fueled.safe_velocity() / 100 ) );
            r.push_back( to_string( veh_fueled.acceleration() / 100 ) );
            r.push_back( to_string( ( int )( 100 * veh_fueled.k_mass() ) ) );
            r.push_back( to_string( ( int )( 100 * veh_fueled.k_aerodynamics() ) ) );
            r.push_back( to_string( ( int )( 100 * veh_fueled.k_friction() ) ) );
            r.push_back( to_string( ( int )( 100 * veh_fueled.k_traction( veh_fueled.wheel_area(
                                                 false ) / 2.0f ) ) ) );
            rows.push_back( r );
        };
        for( auto &e : vehicle_prototype::get_all() ) {
            dump( e );
        }

    } else if( what == "VPART" ) {
        header = {
            "Name", "Location", "Weight", "Size"
        };
        auto dump = [&rows]( const vpart_info & obj ) {
            std::vector<std::string> r;
            r.push_back( obj.name() );
            r.push_back( obj.location );
            r.push_back( to_string( int( ceil( to_gram( item( obj.item ).weight() ) / 1000.0 ) ) ) );
            r.push_back( to_string( obj.size / units::legacy_volume_factor ) );
            rows.push_back( r );
        };
        for( const auto &e : vpart_info::all() ) {
            dump( e.second );
        }

    } else if( what == "EXPLOSIVE" ) {
        header = {
            // @todo: Should display more useful data: shrapnel damage, safe range
            "Name", "Power", "Power at 5 tiles", "Power halves at", "Shrapnel count", "Shrapnel mass"
        };

        auto dump = [&rows]( const std::string & name, const explosion_data & ex ) {
            std::vector<std::string> r;
            r.push_back( name );
            r.push_back( to_string( ex.power ) );
            r.push_back( string_format( "%.1f", ex.power_at_range( 5.0f ) ) );
            r.push_back( string_format( "%.1f", ex.expected_range( 0.5f ) ) );
            r.push_back( to_string( ex.shrapnel.count ) );
            r.push_back( to_string( ex.shrapnel.mass ) );
            rows.push_back( r );
        };
        for( const itype *e : item_controller->all() ) {
            const auto use = e->get_use( "explosion" );
            if( use != nullptr && use->get_actor_ptr() != nullptr ) {
                const auto actor = dynamic_cast<const explosion_iuse *>( use->get_actor_ptr() );
                if( actor != nullptr ) {
                    dump( e->nname( 1 ), actor->explosion );
                }
            }

            auto c_ex = dynamic_cast<const explosion_iuse *>( e->countdown_action.get_actor_ptr() );
            if( c_ex != nullptr ) {
                dump( e->nname( 1 ), c_ex->explosion );
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
            for( const auto &r : rows ) {
                std::copy( r.begin(), r.end() - 1, std::ostream_iterator<std::string>( std::cout, "\t" ) );
                std::cout << r.back() << "\n";
            }
            break;

        case dump_mode::HTML:
            std::cout << "<table>";

            std::cout << "<thead>";
            std::cout << "<tr>";
            for( const auto &col : header ) {
                std::cout << "<th>" << col << "</th>";
            }
            std::cout << "</tr>";
            std::cout << "</thead>";

            std::cout << "<tdata>";
            for( const auto &r : rows ) {
                std::cout << "<tr>";
                for( const auto &col : r ) {
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
