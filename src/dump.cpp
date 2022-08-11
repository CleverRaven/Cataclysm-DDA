#include "game.h" // IWYU pragma: associated

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "character.h"
#include "damage.h"
#include "init.h"
#include "item.h"
#include "item_factory.h"
#include "item_pocket.h"
#include "itype.h"
#include "loading_ui.h"
#include "localized_comparator.h"
#include "make_static.h"
#include "npc.h"
#include "output.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "ret_val.h"
#include "skill.h"
#include "stomach.h"
#include "translations.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vitamin.h"

static const itype_id itype_223( "223" );
static const itype_id itype_270( "270" );
static const itype_id itype_9mm( "9mm" );

static const mod_id MOD_INFORMATION_dda( "dda" );

bool game::dump_stats( const std::string &what, dump_mode mode,
                       const std::vector<std::string> &opts )
{
    try {
        loading_ui ui( false );
        load_core_data( ui );
        load_packs( _( "Loading content packs" ), { MOD_INFORMATION_dda }, ui );
        DynamicDataLoader::get_instance().finalize_loaded_data( ui );
    } catch( const std::exception &err ) {
        std::cerr << "Error loading data from json: " << err.what() << std::endl;
        return false;
    }

    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;

    int scol = 0; // sorting column

    std::map<std::string, standard_npc> test_npcs;
    test_npcs[ "S1" ] = standard_npc(
                            "S1", { 0, 0, 2 }, { "gloves_survivor", "mask_lsurvivor" },
                            4, 8, 10, 8, 10 /* DEX 10, PER 10 */ );
    test_npcs[ "S2" ] = standard_npc(
                            "S2", { 0, 0, 3 }, { "gloves_fingerless", "sunglasses" },
                            4, 8, 8, 8, 10 /* PER 10 */ );
    test_npcs[ "S3" ] = standard_npc(
                            "S3", { 0, 0, 4 }, { "gloves_plate", "helmet_plate" },
                            4, 10, 8, 8, 8 /* STAT 10 */ );
    test_npcs[ "S4" ] = standard_npc( "S4", { 0, 0, 5 }, {}, 0, 8, 10, 8, 10 /* DEX 10, PER 10 */ );
    test_npcs[ "S5" ] = standard_npc( "S5", { 0, 0, 6 }, {}, 4, 8, 10, 8, 10 /* DEX 10, PER 10 */ );
    test_npcs[ "S6" ] = standard_npc(
                            "S6", { 0, 0, 7 }, { "gloves_hsurvivor", "mask_hsurvivor" },
                            4, 8, 10, 8, 10 /* DEX 10, PER 10 */ );

    std::map<std::string, item> test_items;
    test_items[ "G1" ] = item( "glock_19" ).ammo_set( itype_9mm );
    test_items[ "G2" ] = item( "hk_mp5" ).ammo_set( itype_9mm );
    test_items[ "G3" ] = item( "ar15" ).ammo_set( itype_223 );
    test_items[ "G4" ] = item( "remington_700" ).ammo_set( itype_270 );
    test_items[ "G4" ].put_in( item( "rifle_scope" ), item_pocket::pocket_type::MOD );

    if( what == "AMMO" ) {
        header = {
            "Name", "Ammo", "Volume", "Weight", "Stack",
            "Range", "Dispersion", "Recoil", "Damage", "Pierce", "Damage multiplier"
        };
        auto dump = [&rows]( const item & obj ) {
            std::vector<std::string> r;
            r.push_back( obj.tname( 1, false ) );
            r.push_back( obj.ammo_type().str() );
            r.push_back( std::to_string( obj.volume() / units::legacy_volume_factor ) );
            r.push_back( std::to_string( to_gram( obj.weight() ) ) );
            r.push_back( std::to_string( obj.type->stack_size ) );
            r.push_back( std::to_string( obj.type->ammo->range ) );
            r.push_back( std::to_string( obj.type->ammo->dispersion ) );
            r.push_back( std::to_string( obj.type->ammo->recoil ) );
            damage_instance damage = obj.type->ammo->damage;
            r.push_back( std::to_string( damage.total_damage() ) );
            r.push_back( std::to_string( damage.empty() ? 0 : ( *damage.begin() ).res_pen ) );
            rows.push_back( r );
        };
        for( const itype *e : item_controller->all() ) {
            if( e->ammo ) {
                dump( item( e, calendar::turn, item::solitary_tag {} ) );
            }
        }

    } else if( what == "ARMOR" ) {
        header = {
            "Name",
            "Encumber (fit)",
            "Warmth",
            "Weight",
            "Coverage",
            "Coverage (M)",
            "Coverage (R)",
            "Coverage (V)",
            "Bash",
            "Cut",
            "Bullet",
            "Acid",
            "Fire"
        };
        const bodypart_id bp_null( "bp_null" );
        bodypart_id bp = opts.empty() ? bp_null : bodypart_id( opts.front() );
        auto dump = [&rows, &bp]( const item & obj ) {
            std::vector<std::string> r;
            r.push_back( obj.tname( 1, false ) );
            r.push_back( std::to_string( obj.get_encumber( get_player_character(),  bp ) ) );
            r.push_back( std::to_string( obj.get_warmth() ) );
            r.push_back( std::to_string( to_gram( obj.weight() ) ) );
            r.push_back( std::to_string( obj.get_coverage( bp, item::cover_type::COVER_DEFAULT ) ) );
            r.push_back( std::to_string( obj.get_coverage( bp, item::cover_type::COVER_MELEE ) ) );
            r.push_back( std::to_string( obj.get_coverage( bp, item::cover_type::COVER_RANGED ) ) );
            r.push_back( std::to_string( obj.get_coverage( bp, item::cover_type::COVER_VITALS ) ) );
            r.push_back( std::to_string( obj.bash_resist() ) );
            r.push_back( std::to_string( obj.cut_resist() ) );
            r.push_back( std::to_string( obj.bullet_resist() ) );
            r.push_back( std::to_string( obj.acid_resist() ) );
            r.push_back( std::to_string( obj.fire_resist() ) );
            rows.push_back( r );
        };

        for( const itype *e : item_controller->all() ) {
            if( e->armor ) {
                item obj( e );
                if( bp == bp_null || obj.covers( bp ) ) {
                    if( obj.has_flag( STATIC( flag_id( "VARSIZE" ) ) ) ) {
                        obj.set_flag( STATIC( flag_id( "FIT" ) ) );
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
            r.push_back( obj.tname( 1, false ) );
            r.push_back( std::to_string( obj.volume() / units::legacy_volume_factor ) );
            r.push_back( std::to_string( to_gram( obj.weight() ) ) );
            r.push_back( std::to_string( obj.type->stack_size ) );
            r.push_back( std::to_string( obj.get_comestible()->default_nutrition.kcal() ) );
            r.push_back( std::to_string( obj.get_comestible()->quench ) );
            r.push_back( std::to_string( obj.get_comestible()->healthy ) );
            auto vits = obj.get_comestible()->default_nutrition.vitamins;
            for( const auto &v : vitamin::all() ) {
                r.push_back( std::to_string( vits[ v.first ] ) );
            }
            rows.push_back( r );
        };

        for( const itype *e : item_controller->all() ) {
            item food( e, calendar::turn, item::solitary_tag {} );

            if( food.is_food() && get_player_character().can_eat( food ).success() ) {
                dump( food );
            }
        }

    } else if( what == "GUN" ) {
        header = {
            "Name", "Ammo", "Volume", "Weight", "Capacity",
            "Range", "Dispersion", "Effective recoil", "Damage", "Pierce",
            "Aim time", "Effective range", "Snapshot range", "Max range"
        };

        std::set<gunmod_location> locations;
        for( const itype *e : item_controller->all() ) {
            if( e->gun ) {
                std::transform( e->gun->valid_mod_locations.begin(),
                                e->gun->valid_mod_locations.end(),
                                std::inserter( locations, locations.begin() ),
                []( const std::pair<gunmod_location, int> &q ) {
                    return q.first;
                } );
            }
        }
        for( const gunmod_location &e : locations ) {
            header.push_back( e.name() );
        }

        auto dump = [&rows, &locations]( const standard_npc & who, const item & obj ) {
            std::vector<std::string> r;
            r.push_back( obj.tname( 1, false ) );
            r.push_back( !obj.ammo_types().empty() ? enumerate_as_string( obj.ammo_types().begin(),
            obj.ammo_types().end(), []( const ammotype & at ) {
                return at.str();
            }, enumeration_conjunction::none ) : "" );
            r.push_back( std::to_string( obj.volume() / units::legacy_volume_factor ) );
            r.push_back( std::to_string( to_gram( obj.weight() ) ) );
            r.push_back( std::to_string( obj.gun_range() ) );
            r.push_back( std::to_string( obj.gun_dispersion() ) );
            r.push_back( std::to_string( obj.gun_recoil( who ) ) );
            damage_instance damage = obj.gun_damage();
            r.push_back( std::to_string( damage.total_damage() ) );
            r.push_back( std::to_string( damage.empty() ? 0 : ( *damage.begin() ).res_pen ) );

            r.push_back( std::to_string( who.gun_engagement_moves( obj ) ) );

            for( const gunmod_location &e : locations ) {
                const auto &vml = obj.type->gun->valid_mod_locations;
                const auto iter = vml.find( e );
                r.push_back( std::to_string( iter != vml.end() ? iter->second : 0 ) );
            }
            rows.push_back( r );
        };
        for( const itype *e : item_controller->all() ) {
            if( e->gun ) {
                item gun( e );
                if( !gun.magazine_integral() ) {
                    gun.put_in( item( gun.magazine_default() ), item_pocket::pocket_type::MAGAZINE_WELL );
                }
                gun.ammo_set( gun.ammo_default( false ) );

                dump( test_npcs[ "S1" ], gun );

                if( gun.type->gun->barrel_volume > 0_ml ) {
                    gun.put_in( item( "barrel_small" ), item_pocket::pocket_type::MOD );
                    dump( test_npcs[ "S1" ], gun );
                }
            }
        }

    } else if( what == "RECIPE" ) {

        // optionally filter recipes to include only those using specified skills
        recipe_subset dict;
        for( const auto &r : recipe_dict ) {
            if( opts.empty() || std::any_of( opts.begin(), opts.end(), [&r]( const std::string & s ) {
            if( r.second.skill_used == skill_id( s ) && r.second.difficulty > 0 ) {
                    return true;
                }
                const auto iter = r.second.required_skills.find( skill_id( s ) );
                return iter != r.second.required_skills.end() && iter->second > 0;
            } ) ) {
                dict.include( &r.second );
            }
        }

        // only consider skills that are required by at least one recipe
        std::vector<Skill> sk;
        std::copy_if( Skill::skills.begin(), Skill::skills.end(),
        std::back_inserter( sk ), [&dict]( const Skill & s ) {
            return std::any_of( dict.begin(), dict.end(), [&s]( const recipe * r ) {
                return r->skill_used == s.ident() ||
                       r->required_skills.find( s.ident() ) != r->required_skills.end();
            } );
        } );

        header = { "Result" };

        for( const Skill &e : sk ) {
            header.push_back( e.ident().str() );
        }

        for( const recipe *e : dict ) {
            std::vector<std::string> r;
            r.push_back( e->result_name( /*decorated=*/true ) );
            for( const Skill &s : sk ) {
                if( e->skill_used == s.ident() ) {
                    r.push_back( std::to_string( e->difficulty ) );
                } else {
                    auto iter = e->required_skills.find( s.ident() );
                    r.push_back( std::to_string( iter != e->required_skills.end() ? iter->second : 0 ) );
                }
            }
            rows.push_back( r );
        }

    } else if( what == "VEHICLE" ) {
        header = {
            "Name", "Weight (empty)", "Weight (fueled)",
            "Max velocity (mph)", "Safe velocity (mph)", "Acceleration (mph/turn)",
            "Aerodynamics coeff", "Rolling coeff", "Static Drag", "Offroad %"
        };
        auto dump = [&rows]( const vproto_id & obj ) {
            vehicle veh_empty( get_map(), obj, 0, 0 );
            vehicle veh_fueled( get_map(), obj, 100, 0 );

            std::vector<std::string> r;
            r.push_back( veh_empty.name );
            r.push_back( std::to_string( to_kilogram( veh_empty.total_mass() ) ) );
            r.push_back( std::to_string( to_kilogram( veh_fueled.total_mass() ) ) );
            r.push_back( std::to_string( veh_fueled.max_velocity() / 100 ) );
            r.push_back( std::to_string( veh_fueled.safe_velocity() / 100 ) );
            r.push_back( std::to_string( veh_fueled.acceleration() / 100 ) );
            r.push_back( std::to_string( veh_fueled.coeff_air_drag() ) );
            r.push_back( std::to_string( veh_fueled.coeff_rolling_drag() ) );
            r.push_back( std::to_string( veh_fueled.static_drag( false ) ) );
            r.push_back( std::to_string( static_cast<int>( 50 *
                                         veh_fueled.k_traction( veh_fueled.wheel_area() ) ) ) );
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
            r.push_back( std::to_string( static_cast<int>( std::ceil( to_gram( item(
                                             obj.base_item ).weight() ) /
                                         1000.0 ) ) ) );
            r.push_back( std::to_string( obj.size / units::legacy_volume_factor ) );
            rows.push_back( r );
        };
        for( const auto &e : vpart_info::all() ) {
            dump( e.second );
        }

    } else {
        std::cerr << "unknown argument: " << what << std::endl;
        return false;
    }

    rows.erase( std::remove_if( rows.begin(), rows.end(), []( const std::vector<std::string> &e ) {
        return e.empty();
    } ), rows.end() );

    if( scol >= 0 ) {
        std::sort( rows.begin(), rows.end(), [&scol]( const std::vector<std::string> &lhs,
        const std::vector<std::string> &rhs ) {
            return localized_compare( lhs[ scol ], rhs[ scol ] );
        } );
    }

    rows.erase( std::unique( rows.begin(), rows.end() ), rows.end() );

    switch( mode ) {
        case dump_mode::TSV:
            rows.insert( rows.begin(), header );
            for( const auto &r : rows ) {
                // NOLINTNEXTLINE(cata-text-style): using tab to align the output
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
