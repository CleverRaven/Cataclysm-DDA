#include "veh_type.h"
#include "requirements.h"
#include "vehicle.h"
#include "debug.h"
#include "item_group.h"
#include "json.h"
#include "translations.h"
#include "string_formatter.h"
#include "color.h"
#include "itype.h"
#include "ammo.h"
#include "vehicle_group.h"
#include "init.h"
#include "output.h"
#include "generic_factory.h"
#include "character.h"
#include "flag.h"

#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <numeric>

const skill_id skill_mechanics( "mechanics" );

std::unordered_map<vproto_id, vehicle_prototype> vtypes;

// GENERAL GUIDELINES
// To determine mount position for parts (dx, dy), check this scheme:
//         orthogonal dir left: (Y-)
//                ^
//  back: X-   -------> forward dir: X+
//                v
//         orthogonal dir right (Y+)
//
// i.e, if you want to add a part to the back from the center of vehicle,
// use dx = -1, dy = 0;
// for the part 1 tile forward and two tiles left from the center of vehicle,
// use dx = 1, dy = -2.
//
// Internal parts should be added after external on the same mount point, i.e:
//  part {"x": 0, "y": 1, "part": "seat"},       // put a seat (it's external)
//  part {"x": 0, "y": 1, "part": "controls"},   // put controls for driver here
//  part {"x": 0, "y": 1, "seatbelt"}   // also, put a seatbelt here
// To determine, what parts can be external, and what can not, check
// vehicle_parts.json
// If you use wrong config, installation of part will fail

static const std::unordered_map<std::string, vpart_bitflags> vpart_bitflag_map = {
    { "ARMOR", VPFLAG_ARMOR },
    { "EVENTURN", VPFLAG_EVENTURN },
    { "ODDTURN", VPFLAG_ODDTURN },
    { "CONE_LIGHT", VPFLAG_CONE_LIGHT },
    { "CIRCLE_LIGHT", VPFLAG_CIRCLE_LIGHT },
    { "BOARDABLE", VPFLAG_BOARDABLE },
    { "AISLE", VPFLAG_AISLE },
    { "CONTROLS", VPFLAG_CONTROLS },
    { "OBSTACLE", VPFLAG_OBSTACLE },
    { "OPAQUE", VPFLAG_OPAQUE },
    { "OPENABLE", VPFLAG_OPENABLE },
    { "SEATBELT", VPFLAG_SEATBELT },
    { "WHEEL", VPFLAG_WHEEL },
    { "FLOATS", VPFLAG_FLOATS },
    { "DOME_LIGHT", VPFLAG_DOME_LIGHT },
    { "AISLE_LIGHT", VPFLAG_AISLE_LIGHT },
    { "ATOMIC_LIGHT", VPFLAG_ATOMIC_LIGHT },
    { "ALTERNATOR", VPFLAG_ALTERNATOR },
    { "ENGINE", VPFLAG_ENGINE },
    { "FRIDGE", VPFLAG_FRIDGE },
    { "FREEZER", VPFLAG_FREEZER },
    { "LIGHT", VPFLAG_LIGHT },
    { "WINDOW", VPFLAG_WINDOW },
    { "CURTAIN", VPFLAG_CURTAIN },
    { "CARGO", VPFLAG_CARGO },
    { "INTERNAL", VPFLAG_INTERNAL },
    { "SOLAR_PANEL", VPFLAG_SOLAR_PANEL },
    { "RECHARGE", VPFLAG_RECHARGE },
    { "VISION", VPFLAG_EXTENDS_VISION },
    { "ENABLED_DRAINS_EPOWER", VPFLAG_ENABLED_DRAINS_EPOWER },
    { "WASHING_MACHINE", VPFLAG_WASHING_MACHINE },
};

static std::map<vpart_id, vpart_info> vpart_info_all;

static std::map<vpart_id, vpart_info> abstract_parts;

static DynamicDataLoader::deferred_json deferred;

/** @relates string_id */
template<>
bool string_id<vpart_info>::is_valid() const
{
    return vpart_info_all.count( *this );
}

/** @relates string_id */
template<>
const vpart_info &string_id<vpart_info>::obj() const
{
    const auto found = vpart_info_all.find( *this );
    if( found == vpart_info_all.end() ) {
        debugmsg( "Tried to get invalid vehicle part: %s", c_str() );
        static const vpart_info null_part{};
        return null_part;
    }
    return found->second;
}

static void parse_vp_reqs( JsonObject &obj, const std::string &id, const std::string &key,
                           std::vector<std::pair<requirement_id, int>> &reqs,
                           std::map<skill_id, int> &skills, int &moves )
{

    if( !obj.has_object( key ) ) {
        return;
    }
    auto src = obj.get_object( key );

    auto sk = src.get_array( "skills" );
    if( !sk.empty() ) {
        skills.clear();
    }
    while( sk.has_more() ) {
        auto cur = sk.next_array();
        skills.emplace( skill_id( cur.get_string( 0 ) ), cur.size() >= 2 ? cur.get_int( 1 ) : 1 );
    }

    assign( src, "time", moves );

    if( src.has_string( "using" ) ) {
        reqs = { { requirement_id( src.get_string( "using" ) ), 1 } };

    } else if( src.has_array( "using" ) ) {
        auto arr = src.get_array( "using" );
        while( arr.has_more() ) {
            auto cur = arr.next_array();
            reqs.emplace_back( requirement_id( cur.get_string( 0 ) ), cur.get_int( 1 ) );
        }

    } else {
        auto req_id = string_format( "inline_%s_%s", key.c_str(), id.c_str() );
        requirement_data::load_requirement( src, req_id );
        reqs = { { requirement_id( req_id ), 1 } };
    }
};

/**
 * Reads in a vehicle part from a JsonObject.
 */
void vpart_info::load( JsonObject &jo, const std::string &src )
{
    vpart_info def;

    if( jo.has_string( "copy-from" ) ) {
        auto const base = vpart_info_all.find( vpart_id( jo.get_string( "copy-from" ) ) );
        auto const ab = abstract_parts.find( vpart_id( jo.get_string( "copy-from" ) ) );
        if( base != vpart_info_all.end() ) {
            def = base->second;
            def.looks_like = base->second.id.str();
        } else if( ab != abstract_parts.end() ) {
            def = ab->second;
            if( def.looks_like.empty() ) {
                def.looks_like = ab->second.id.str();
            }
        } else {
            deferred.emplace_back( jo.str(), src );
            return;
        }
    }

    if( jo.has_string( "abstract" ) ) {
        def.id = vpart_id( jo.get_string( "abstract" ) );
    } else {
        def.id = vpart_id( jo.get_string( "id" ) );
    }

    assign( jo, "name", def.name_ );
    assign( jo, "item", def.item );
    assign( jo, "location", def.location );
    assign( jo, "durability", def.durability );
    assign( jo, "damage_modifier", def.dmg_mod );
    assign( jo, "power", def.power );
    assign( jo, "epower", def.epower );
    assign( jo, "fuel_type", def.fuel_type );
    assign( jo, "default_ammo", def.default_ammo );
    assign( jo, "folded_volume", def.folded_volume );
    assign( jo, "size", def.size );
    assign( jo, "difficulty", def.difficulty );
    assign( jo, "bonus", def.bonus );
    assign( jo, "flags", def.flags );
    assign( jo, "description", def.description );

    if( jo.has_member( "requirements" ) ) {
        auto reqs = jo.get_object( "requirements" );

        parse_vp_reqs( reqs, def.id.str(), "install", def.install_reqs, def.install_skills,
                       def.install_moves );
        parse_vp_reqs( reqs, def.id.str(), "removal", def.removal_reqs, def.removal_skills,
                       def.removal_moves );
        parse_vp_reqs( reqs, def.id.str(), "repair",  def.repair_reqs,  def.repair_skills,
                       def.repair_moves );

        def.legacy = false;
    }

    if( jo.has_member( "symbol" ) ) {
        def.sym = jo.get_string( "symbol" )[ 0 ];
    }
    if( jo.has_member( "broken_symbol" ) ) {
        def.sym_broken = jo.get_string( "broken_symbol" )[ 0 ];
    }
    if( jo.has_member( "looks_like" ) ) {
        def.looks_like = jo.get_string( "looks_like" );
    }

    if( jo.has_member( "color" ) ) {
        def.color = color_from_string( jo.get_string( "color" ) );
    }
    if( jo.has_member( "broken_color" ) ) {
        def.color_broken = color_from_string( jo.get_string( "broken_color" ) );
    }

    if( jo.has_member( "breaks_into" ) ) {
        JsonIn &stream = *jo.get_raw( "breaks_into" );
        def.breaks_into_group = item_group::load_item_group( stream, "collection" );
    }

    auto qual = jo.get_array( "qualities" );
    if( !qual.empty() ) {
        def.qualities.clear();
        while( qual.has_more() ) {
            auto pair = qual.next_array();
            def.qualities[ quality_id( pair.get_string( 0 ) ) ] = pair.get_int( 1 );
        }
    }

    if( jo.has_member( "damage_reduction" ) ) {
        JsonObject dred = jo.get_object( "damage_reduction" );
        def.damage_reduction = load_damage_array( dred );
    } else {
        def.damage_reduction.fill( 0.0f );
    }

    if( jo.has_string( "abstract" ) ) {
        abstract_parts[def.id] = def;
    } else {
        vpart_info_all[def.id] = def;
    }
}

void vpart_info::set_flag( const std::string &flag )
{
    flags.insert( flag );
    const auto iter = vpart_bitflag_map.find( flag );
    if( iter != vpart_bitflag_map.end() ) {
        bitflags.set( iter->second );
    }
}

void vpart_info::finalize()
{
    DynamicDataLoader::get_instance().load_deferred( deferred );

    for( auto &e : vpart_info_all ) {
        // if part name specified ensure it is translated
        // otherwise the name of the base item will be used
        if( !e.second.name_.empty() ) {
            e.second.name_ = _( e.second.name_.c_str() );
        }

        if( e.second.folded_volume > 0 ) {
            e.second.set_flag( "FOLDABLE" );
        }

        for( const auto &f : e.second.flags ) {
            auto b = vpart_bitflag_map.find( f );
            if( b != vpart_bitflag_map.end() ) {
                e.second.bitflags.set( b->second );
            }
        }

        // Calculate and cache z-ordering based off of location
        // list_order is used when inspecting the vehicle
        if( e.second.location == "on_roof" ) {
            e.second.z_order = 9;
            e.second.list_order = 3;
        } else if( e.second.location == "on_cargo" ) {
            e.second.z_order = 8;
            e.second.list_order = 6;
        } else if( e.second.location == "center" ) {
            e.second.z_order = 7;
            e.second.list_order = 7;
        } else if( e.second.location == "under" ) {
            // Have wheels show up over frames
            e.second.z_order = 6;
            e.second.list_order = 10;
        } else if( e.second.location == "structure" ) {
            e.second.z_order = 5;
            e.second.list_order = 1;
        } else if( e.second.location == "engine_block" ) {
            // Should be hidden by frames
            e.second.z_order = 4;
            e.second.list_order = 8 ;
        } else if( e.second.location == "on_battery_mount" ) {
            // Should be hidden by frames
            e.second.z_order = 3;
            e.second.list_order = 10;
        } else if( e.second.location == "fuel_source" ) {
            // Should be hidden by frames
            e.second.z_order = 3;
            e.second.list_order = 9;
        } else if( e.second.location == "roof" ) {
            // Shouldn't be displayed
            e.second.z_order = -1;
            e.second.list_order = 4;
        } else if( e.second.location == "armor" ) {
            // Shouldn't be displayed (the color is used, but not the symbol)
            e.second.z_order = -2;
            e.second.list_order = 2;
        } else {
            // Everything else
            e.second.z_order = 0;
            e.second.list_order = 5;
        }
    }
}

void vpart_info::check()
{
    for( auto &vp : vpart_info_all ) {
        auto &part = vp.second;

        // handle legacy parts without requirement data
        // @todo: deprecate once requirements are entirely loaded from JSON
        if( part.legacy ) {

            part.install_skills.emplace( skill_mechanics, part.difficulty );
            part.removal_skills.emplace( skill_mechanics, std::max( part.difficulty - 2, 2 ) );
            part.repair_skills.emplace( skill_mechanics, std::min( part.difficulty + 1, MAX_SKILL ) );

            if( part.has_flag( "TOOL_WRENCH" ) || part.has_flag( "WHEEL" ) ) {
                part.install_reqs = { { requirement_id( "vehicle_bolt" ), 1 } };
                part.removal_reqs = { { requirement_id( "vehicle_bolt" ), 1 } };
                part.repair_reqs  = { { requirement_id( "welding_standard" ), 5 } };

            } else if( part.has_flag( "TOOL_SCREWDRIVER" ) ) {
                part.install_reqs = { { requirement_id( "vehicle_screw" ), 1 } };
                part.removal_reqs = { { requirement_id( "vehicle_screw" ), 1 } };
                part.repair_reqs  = { { requirement_id( "adhesive" ), 1 } };

            } else if( part.has_flag( "NAILABLE" ) ) {
                part.install_reqs = { { requirement_id( "vehicle_nail_install" ), 1 } };
                part.removal_reqs = { { requirement_id( "vehicle_nail_removal" ), 1 } };
                part.repair_reqs  = { { requirement_id( "adhesive" ), 2 } };

            } else if( part.has_flag( "TOOL_NONE" ) ) {
                // no-op

            } else {
                part.install_reqs = { { requirement_id( "welding_standard" ), 5 } };
                part.removal_reqs = { { requirement_id( "vehicle_weld_removal" ), 1 } };
                part.repair_reqs  = { { requirement_id( "welding_standard" ), 5 } };
            }

        } else {
            if( part.has_flag( "REVERSIBLE" ) ) {
                if( !part.removal_reqs.empty() ) {
                    debugmsg( "vehicle part %s specifies both REVERSIBLE and removal", part.id.c_str() );
                }
                part.removal_reqs = part.install_reqs;
            }
        }

        // add the base item to the installation requirements
        // @todo: support multiple/alternative base items
        requirement_data ins;
        ins.components.push_back( { { { part.item, 1 } } } );

        std::string ins_id = std::string( "inline_vehins_base_" ) += part.id.str();
        requirement_data::save_requirement( ins, ins_id );
        part.install_reqs.emplace_back( requirement_id( ins_id ), 1 );

        if( part.removal_moves < 0 ) {
            part.removal_moves = part.install_moves / 2;
        }

        for( auto &e : part.install_skills ) {
            if( !e.first.is_valid() ) {
                debugmsg( "vehicle part %s has unknown install skill %s", part.id.c_str(), e.first.c_str() );
            }
        }

        for( auto &e : part.removal_skills ) {
            if( !e.first.is_valid() ) {
                debugmsg( "vehicle part %s has unknown removal skill %s", part.id.c_str(), e.first.c_str() );
            }
        }

        for( auto &e : part.repair_skills ) {
            if( !e.first.is_valid() ) {
                debugmsg( "vehicle part %s has unknown repair skill %s", part.id.c_str(), e.first.c_str() );
            }
        }

        for( const auto &e : part.install_reqs ) {
            if( !e.first.is_valid() || e.second <= 0 ) {
                debugmsg( "vehicle part %s has unknown or incorrectly specified install requirements %s",
                          part.id.c_str(), e.first.c_str() );
            }
        }

        for( const auto &e : part.install_reqs ) {
            if( !( e.first.is_null() || e.first.is_valid() ) || e.second < 0 ) {
                debugmsg( "vehicle part %s has unknown or incorrectly specified removal requirements %s",
                          part.id.c_str(), e.first.c_str() );
            }
        }

        for( const auto &e : part.repair_reqs ) {
            if( !( e.first.is_null() || e.first.is_valid() ) || e.second < 0 ) {
                debugmsg( "vehicle part %s has unknown or incorrectly specified repair requirements %s",
                          part.id.c_str(), e.first.c_str() );
            }
        }

        if( part.install_moves < 0 ) {
            debugmsg( "vehicle part %s has negative installation time", part.id.c_str() );
        }

        if( part.removal_moves < 0 ) {
            debugmsg( "vehicle part %s has negative removal time", part.id.c_str() );
        }

        if( !item_group::group_is_defined( part.breaks_into_group ) ) {
            debugmsg( "Vehicle part %s breaks into non-existent item group %s.",
                      part.id.c_str(), part.breaks_into_group.c_str() );
        }
        if( part.sym == 0 ) {
            debugmsg( "vehicle part %s does not define a symbol", part.id.c_str() );
        }
        if( part.sym_broken == 0 ) {
            debugmsg( "vehicle part %s does not define a broken symbol", part.id.c_str() );
        }
        if( part.durability <= 0 ) {
            debugmsg( "vehicle part %s has zero or negative durability", part.id.c_str() );
        }
        if( part.dmg_mod < 0 ) {
            debugmsg( "vehicle part %s has negative damage modifier", part.id.c_str() );
        }
        if( part.folded_volume < 0 ) {
            debugmsg( "vehicle part %s has negative folded volume", part.id.c_str() );
        }
        if( part.has_flag( "FOLDABLE" ) && part.folded_volume == 0 ) {
            debugmsg( "vehicle part %s has folding part with zero folded volume", part.name().c_str() );
        }
        if( !item::type_is_defined( part.default_ammo ) ) {
            debugmsg( "vehicle part %s has undefined default ammo %s", part.id.c_str(), part.item.c_str() );
        }
        if( part.size < 0 ) {
            debugmsg( "vehicle part %s has negative size", part.id.c_str() );
        }
        if( !item::type_is_defined( part.item ) ) {
            debugmsg( "vehicle part %s uses undefined item %s", part.id.c_str(), part.item.c_str() );
        }
        const itype &base_item_type = *item::find_type( part.item );
        // Fuel type errors are serious and need fixing now
        if( !item::type_is_defined( part.fuel_type ) ) {
            debugmsg( "vehicle part %s uses undefined fuel %s", part.id.c_str(), part.item.c_str() );
            part.fuel_type = "null";
        } else if( part.fuel_type != "null" && !item::find_type( part.fuel_type )->fuel &&
                   ( !base_item_type.container || !base_item_type.container->watertight ) ) {
            // Tanks are allowed to specify non-fuel "fuel",
            // because currently legacy blazemod uses it as a hack to restrict content types
            debugmsg( "non-tank vehicle part %s uses non-fuel item %s as fuel, setting to null",
                      part.id.c_str(), part.fuel_type.c_str() );
            part.fuel_type = "null";
        }
        if( part.has_flag( "TURRET" ) && !base_item_type.gun ) {
            debugmsg( "vehicle part %s has the TURRET flag, but is not made from a gun item", part.id.c_str() );
        }
        for( auto &q : part.qualities ) {
            if( !q.first.is_valid() ) {
                debugmsg( "vehicle part %s has undefined tool quality %s", part.id.c_str(), q.first.c_str() );
            }
        }
        if( part.has_flag( VPFLAG_ENABLED_DRAINS_EPOWER ) && part.epower == 0 ) {
            debugmsg( "%s is set to drain epower, but has epower == 0", part.id.c_str() );
        }
        // Parts with non-zero epower must have a flag that affects epower usage
        static const std::vector<std::string> handled = {{
                "ENABLED_DRAINS_EPOWER", "SECURITY", "ENGINE",
                "ALTERNATOR", "SOLAR_PANEL", "POWER_TRANSFER",
                "REACTOR"
            }
        };
        if( part.epower != 0 &&
        std::none_of( handled.begin(), handled.end(), [&part]( const std::string & flag ) {
        return part.has_flag( flag );
        } ) ) {
            std::string warnings_are_good_docs = enumerate_as_string( handled );
            debugmsg( "%s has non-zero epower, but lacks a flag that would make it affect epower (one of %s)",
                      part.id.c_str(), warnings_are_good_docs.c_str() );
        }
    }
}

void vpart_info::reset()
{
    vpart_info_all.clear();
    abstract_parts.clear();
}

const std::map<vpart_id, vpart_info> &vpart_info::all()
{
    return vpart_info_all;
}

std::string vpart_info::name() const
{
    if( name_.empty() ) {
        name_ = item::nname( item ); // cache on first request
    }
    return name_;
}

int vpart_info::format_description( std::ostringstream &msg, std::string format_color,
                                    int width ) const
{
    int lines = 1;
    msg << _( "<color_white>Description</color>\n" );
    msg << "> " << format_color;

    class::item base( item );
    std::ostringstream long_descrip;
    if( ! description.empty() ) {
        long_descrip << description;
    }
    for( const auto &flagid : flags ) {
        if( flagid == "ALARMCLOCK" || flagid == "WATCH" ) {
            continue;
        }
        json_flag flag = json_flag::get( flagid );
        if( ! flag.info().empty() ) {
            if( ! long_descrip.str().empty() ) {
                long_descrip << "  ";
            }
            long_descrip << flag.info();
        }
    }
    if( ( has_flag( "SEAT" ) || has_flag( "BED" ) ) && ! has_flag( "BELTABLE" ) ) {
        json_flag nobelt = json_flag::get( "NONBELTABLE" );
        long_descrip << "  " << nobelt.info();
    }
    if( has_flag( "BOARDABLE" ) && has_flag( "OPENABLE" ) ) {
        json_flag nobelt = json_flag::get( "DOOR" );
        long_descrip << "  " << nobelt.info();
    }
    if( has_flag( "TURRET" ) ) {
        long_descrip << string_format( _( "\nRange: %1$5d     Damage: %2$5.0f" ),
                                       base.gun_range( true ),
                                       base.gun_damage().total_damage() );
    }

    if( ! long_descrip.str().empty() ) {
        const auto wrap_descrip = foldstring( long_descrip.str(), width );
        msg << wrap_descrip[0];
        for( size_t i = 1; i < wrap_descrip.size(); i++ ) {
            msg << "\n  " << wrap_descrip[i];
        }
        msg << "</color>\n";
        lines += wrap_descrip.size();
    }

    // borrowed from item.cpp and adjusted
    const quality_id quality_jack( "JACK" );
    const quality_id quality_lift( "LIFT" );
    for( const auto &qual : qualities ) {
        msg << "> " << format_color << string_format( _( "Has level %1$d %2$s quality" ),
                qual.second, qual.first.obj().name.c_str() );
        if( qual.first == quality_jack || qual.first == quality_lift ) {
            msg << string_format( _( " and is rated at %1$d %2$s" ),
                                  ( int )convert_weight( qual.second * TOOL_LIFT_FACTOR ),
                                  weight_units() );
        }
        msg << ".</color>\n";
        lines += 1;
    }
    return lines;
}

requirement_data vpart_info::install_requirements() const
{
    return std::accumulate( install_reqs.begin(), install_reqs.end(), requirement_data(),
    []( const requirement_data & lhs, const std::pair<requirement_id, int> &rhs ) {
        return lhs + ( *rhs.first * rhs.second );
    } );
}

requirement_data vpart_info::removal_requirements() const
{
    return std::accumulate( removal_reqs.begin(), removal_reqs.end(), requirement_data(),
    []( const requirement_data & lhs, const std::pair<requirement_id, int> &rhs ) {
        return lhs + ( *rhs.first * rhs.second );
    } );
}

requirement_data vpart_info::repair_requirements() const
{
    return std::accumulate( repair_reqs.begin(), repair_reqs.end(), requirement_data(),
    []( const requirement_data & lhs, const std::pair<requirement_id, int> &rhs ) {
        return lhs + ( *rhs.first * rhs.second );
    } );
}


bool vpart_info::is_repairable() const
{
    return !repair_requirements().is_empty();
}

static int scale_time( const std::map<skill_id, int> &sk, int mv, const Character &ch )
{
    if( sk.empty() ) {
        return mv;
    }

    int lvl = std::accumulate( sk.begin(), sk.end(), 0, [&ch]( int lhs,
    const std::pair<skill_id, int> &rhs ) {
        return lhs + std::max( std::min( ch.get_skill_level( rhs.first ), MAX_SKILL ) - rhs.second,
                               0 );
    } );
    // 10% per excess level (reduced proportionally if >1 skill required) with max 50% reduction
    return mv * ( 1.0 - std::min( double( lvl ) / sk.size() / 10.0, 0.5 ) );
}

int vpart_info::install_time( const Character &ch ) const
{
    return scale_time( install_skills, install_moves, ch );
}

int vpart_info::removal_time( const Character &ch ) const
{
    return scale_time( removal_skills, removal_moves, ch );
}

int vpart_info::repair_time( const Character &ch ) const
{
    return scale_time( repair_skills, repair_moves, ch );
}

/** @relates string_id */
template<>
const vehicle_prototype &string_id<vehicle_prototype>::obj() const
{
    const auto iter = vtypes.find( *this );
    if( iter == vtypes.end() ) {
        debugmsg( "invalid vehicle prototype id %s", c_str() );
        static const vehicle_prototype dummy = {
            "",
            std::vector<vehicle_prototype::part_def>{},
            std::vector<vehicle_item_spawn>{},
            nullptr
        };
        return dummy;
    }
    return iter->second;
}

/** @relates string_id */
template<>
bool string_id<vehicle_prototype>::is_valid() const
{
    return vtypes.count( *this ) > 0;
}

/**
 *Caches a vehicle definition from a JsonObject to be loaded after itypes is initialized.
 */
void vehicle_prototype::load( JsonObject &jo )
{
    vehicle_prototype &vproto = vtypes[ vproto_id( jo.get_string( "id" ) ) ];
    // If there are already parts defined, this vehicle prototype overrides an existing one.
    // If the json contains a name, it means a completely new prototype (replacing the
    // original one), therefore the old data has to be cleared.
    // If the json does not contain a name (the prototype would have no name), it means appending
    // to the existing prototype (the parts are not cleared).
    if( !vproto.parts.empty() && jo.has_string( "name" ) ) {
        vproto =  vehicle_prototype();
    }
    if( vproto.parts.empty() ) {
        vproto.name = jo.get_string( "name" );
    }

    vgroups[vgroup_id( jo.get_string( "id" ) )].add_vehicle( vproto_id( jo.get_string( "id" ) ), 100 );

    JsonArray parts = jo.get_array( "parts" );
    while( parts.has_more() ) {
        JsonObject part = parts.next_object();

        part_def pt;
        pt.pos = point( part.get_int( "x" ), part.get_int( "y" ) );
        pt.part = vpart_id( part.get_string( "part" ) );

        assign( part, "ammo", pt.with_ammo, true, 0, 100 );
        assign( part, "ammo_types", pt.ammo_types, true );
        assign( part, "ammo_qty", pt.ammo_qty, true, 0 );
        assign( part, "fuel", pt.fuel, true );

        vproto.parts.push_back( pt );
    }

    JsonArray items = jo.get_array( "items" );
    while( items.has_more() ) {
        JsonObject spawn_info = items.next_object();
        vehicle_item_spawn next_spawn;
        next_spawn.pos.x = spawn_info.get_int( "x" );
        next_spawn.pos.y = spawn_info.get_int( "y" );

        next_spawn.chance = spawn_info.get_int( "chance" );
        if( next_spawn.chance <= 0 || next_spawn.chance > 100 ) {
            debugmsg( "Invalid spawn chance in %s (%d, %d): %d%%",
                      vproto.name.c_str(), next_spawn.pos.x, next_spawn.pos.y, next_spawn.chance );
        }

        // constrain both with_magazine and with_ammo to [0-100]
        next_spawn.with_magazine = std::max( std::min( spawn_info.get_int( "magazine",
                                             next_spawn.with_magazine ), 100 ), 0 );
        next_spawn.with_ammo = std::max( std::min( spawn_info.get_int( "ammo", next_spawn.with_ammo ),
                                         100 ), 0 );

        if( spawn_info.has_array( "items" ) ) {
            //Array of items that all spawn together (i.e. jack+tire)
            JsonArray item_group = spawn_info.get_array( "items" );
            while( item_group.has_more() ) {
                next_spawn.item_ids.push_back( item_group.next_string() );
            }
        } else if( spawn_info.has_string( "items" ) ) {
            //Treat single item as array
            next_spawn.item_ids.push_back( spawn_info.get_string( "items" ) );
        }
        if( spawn_info.has_array( "item_groups" ) ) {
            //Pick from a group of items, just like map::place_items
            JsonArray item_group_names = spawn_info.get_array( "item_groups" );
            while( item_group_names.has_more() ) {
                next_spawn.item_groups.push_back( item_group_names.next_string() );
            }
        } else if( spawn_info.has_string( "item_groups" ) ) {
            next_spawn.item_groups.push_back( spawn_info.get_string( "item_groups" ) );
        }
        vproto.item_spawns.push_back( std::move( next_spawn ) );
    }
}

void vehicle_prototype::reset()
{
    vtypes.clear();
}

/**
 *Works through cached vehicle definitions and creates vehicle objects from them.
 */
void vehicle_prototype::finalize()
{
    for( auto &vp : vtypes ) {
        std::unordered_set<point> cargo_spots;
        vehicle_prototype &proto = vp.second;
        const vproto_id &id = vp.first;

        // Calls the default constructor to create an empty vehicle. Calling the constructor with
        // the type as parameter would make it look up the type in the map and copy the
        // (non-existing) blueprint.
        proto.blueprint.reset( new vehicle() );
        vehicle &blueprint = *proto.blueprint;
        blueprint.type = id;
        blueprint.name = _( proto.name.c_str() );

        for( auto &pt : proto.parts ) {
            auto base = item::find_type( pt.part->item );

            if( !pt.part.is_valid() ) {
                debugmsg( "unknown vehicle part %s in %s", pt.part.c_str(), id.c_str() );
                continue;
            }

            if( blueprint.install_part( pt.pos.x, pt.pos.y, pt.part ) < 0 ) {
                debugmsg( "init_vehicles: '%s' part '%s'(%d) can't be installed to %d,%d",
                          blueprint.name.c_str(), pt.part.c_str(),
                          blueprint.parts.size(), pt.pos.x, pt.pos.y );
            }

            if( !base->gun ) {
                if( pt.with_ammo ) {
                    debugmsg( "init_vehicles: non-turret %s with ammo in %s", pt.part.c_str(), id.c_str() );
                }
                if( !pt.ammo_types.empty() ) {
                    debugmsg( "init_vehicles: non-turret %s with ammo_types in %s", pt.part.c_str(), id.c_str() );
                }
                if( pt.ammo_qty.first > 0 || pt.ammo_qty.second > 0 ) {
                    debugmsg( "init_vehicles: non-turret %s with ammo_qty in %s", pt.part.c_str(), id.c_str() );
                }

            } else {
                for( const auto &e : pt.ammo_types ) {
                    auto ammo = item::find_type( e );
                    if( !ammo->ammo && ammo->ammo->type.count( base->gun->ammo ) ) {
                        debugmsg( "init_vehicles: turret %s has invalid ammo_type %s in %s",
                                  pt.part.c_str(), e.c_str(), id.c_str() );
                    }
                }
                if( pt.ammo_types.empty() ) {
                    pt.ammo_types.insert( base->gun->ammo->default_ammotype() );
                }
            }

            if( base->container ) {
                if( !item::type_is_defined( pt.fuel ) ) {
                    debugmsg( "init_vehicles: tank %s specified invalid fuel in %s", pt.part.c_str(), id.c_str() );
                }
            } else {
                if( pt.fuel != "null" ) {
                    debugmsg( "init_vehicles: non-tank %s with fuel in %s", pt.part.c_str(), id.c_str() );
                }
            }

            if( pt.part.obj().has_flag( "CARGO" ) ) {
                cargo_spots.insert( pt.pos );
            }
        }

        for( auto &i : proto.item_spawns ) {
            if( cargo_spots.count( i.pos ) == 0 ) {
                debugmsg( "Invalid spawn location (no CARGO vpart) in %s (%d, %d): %d%%",
                          proto.name.c_str(), i.pos.x, i.pos.y, i.chance );
            }
            for( auto &j : i.item_ids ) {
                if( !item::type_is_defined( j ) ) {
                    debugmsg( "unknown item %s in spawn list of %s", j.c_str(), id.c_str() );
                }
            }
            for( auto &j : i.item_groups ) {
                if( !item_group::group_is_defined( j ) ) {
                    debugmsg( "unknown item group %s in spawn list of %s", j.c_str(), id.c_str() );
                }
            }
        }
    }
}

std::vector<vproto_id> vehicle_prototype::get_all()
{
    std::vector<vproto_id> result;
    for( auto &vp : vtypes ) {
        result.push_back( vp.first );
    }
    return result;
}
