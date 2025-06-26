#include "veh_type.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

#include "ammo.h"
#include "assign.h"
#include "cata_assert.h"
#include "catacharset.h"
#include "character.h"
#include "clzones.h"
#include "color.h"
#include "damage.h"
#include "debug.h"
#include "enums.h"
#include "flag.h"
#include "flat_set.h"
#include "flexbuffer_json.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "item.h"
#include "item_factory.h"
#include "item_group.h"
#include "item_pocket.h"
#include "itype.h"
#include "json.h"
#include "map.h"
#include "output.h"
#include "pocket_type.h"
#include "requirements.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "translations.h"
#include "units.h"
#include "units_utility.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "vehicle_group.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "wcwidth.h"

namespace
{
generic_factory<vehicle_prototype> vehicle_prototype_factory( "vehicle", "id" );
generic_factory<vpart_info> vpart_info_factory( "vehicle_part", "id" );
} // namespace

static const ammotype ammo_battery( "battery" );

static const itype_id fuel_type_animal( "animal" );

static const quality_id qual_JACK( "JACK" );
static const quality_id qual_LIFT( "LIFT" );

static const quality_id qual_PISTOL( "PISTOL" );
static const quality_id qual_SMG( "SMG" );

static const skill_id skill_launcher( "launcher" );

static const vpart_id vpart_turret_generic( "turret_generic" );

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
//  part {"x": 0, "y": 1, "part": "seat"},      // put a seat (it's external)
//  part {"x": 0, "y": 1, "part": "controls"},  // put controls for driver here
//  part {"x": 0, "y": 1, "seatbelt"}           // also, put a seatbelt here
// To determine, what parts can be external, and what can not, check
// vehicle_parts.json
// If you use wrong config, installation of part will fail

static const std::unordered_map<std::string, vpart_bitflags> vpart_bitflag_map = {
    { "ARMOR", VPFLAG_ARMOR },
    { "APPLIANCE", VPFLAG_APPLIANCE },
    { "EVENTURN", VPFLAG_EVENTURN },
    { "ODDTURN", VPFLAG_ODDTURN },
    { "CONE_LIGHT", VPFLAG_CONE_LIGHT },
    { "WIDE_CONE_LIGHT", VPFLAG_WIDE_CONE_LIGHT },
    { "HALF_CIRCLE_LIGHT", VPFLAG_HALF_CIRCLE_LIGHT },
    { "CIRCLE_LIGHT", VPFLAG_CIRCLE_LIGHT },
    { "BOARDABLE", VPFLAG_BOARDABLE },
    { "AISLE", VPFLAG_AISLE },
    { "CONTROLS", VPFLAG_CONTROLS },
    { "OBSTACLE", VPFLAG_OBSTACLE },
    { "OPAQUE", VPFLAG_OPAQUE },
    { "OPENABLE", VPFLAG_OPENABLE },
    { "SEATBELT", VPFLAG_SEATBELT },
    { "SIMPLE_PART", VPFLAG_SIMPLE_PART },
    { "WALL_MOUNTED", VPFLAG_WALL_MOUNTED },
    { "WHEEL", VPFLAG_WHEEL },
    { "ROTOR", VPFLAG_ROTOR },
    { "FLOATS", VPFLAG_FLOATS },
    { "NO_LEAK", VPFLAG_NO_LEAK },
    { "DOME_LIGHT", VPFLAG_DOME_LIGHT },
    { "AISLE_LIGHT", VPFLAG_AISLE_LIGHT },
    { "ATOMIC_LIGHT", VPFLAG_ATOMIC_LIGHT },
    { "ALTERNATOR", VPFLAG_ALTERNATOR },
    { "ENGINE", VPFLAG_ENGINE },
    { "FRIDGE", VPFLAG_FRIDGE },
    { "FREEZER", VPFLAG_FREEZER },
    { "ARCADE", VPFLAG_ARCADE },
    { "LIGHT", VPFLAG_LIGHT },
    { "WINDOW", VPFLAG_WINDOW },
    { "CURTAIN", VPFLAG_CURTAIN },
    { "CARGO", VPFLAG_CARGO },
    { "CARGO_PASSABLE", VPFLAG_CARGO_PASSABLE },
    { "INTERNAL", VPFLAG_INTERNAL },
    { "SOLAR_PANEL", VPFLAG_SOLAR_PANEL },
    { "WIND_TURBINE", VPFLAG_WIND_TURBINE },
    { "SPACE_HEATER", VPFLAG_SPACE_HEATER, },
    { "HEATED_TANK", VPFLAG_HEATED_TANK, },
    { "COOLER", VPFLAG_COOLER, },
    { "WATER_WHEEL", VPFLAG_WATER_WHEEL },
    { "RECHARGE", VPFLAG_RECHARGE },
    { "VISION", VPFLAG_EXTENDS_VISION },
    { "ENABLED_DRAINS_EPOWER", VPFLAG_ENABLED_DRAINS_EPOWER },
    { "AUTOCLAVE", VPFLAG_AUTOCLAVE },
    { "WASHING_MACHINE", VPFLAG_WASHING_MACHINE },
    { "DISHWASHER", VPFLAG_DISHWASHER },
    { "FLUIDTANK", VPFLAG_FLUIDTANK },
    { "REACTOR", VPFLAG_REACTOR },
    { "RAIL", VPFLAG_RAIL },
    { "TURRET_CONTROLS", VPFLAG_TURRET_CONTROLS },
    { "ROOF", VPFLAG_ROOF },
    { "CABLE_PORTS", VPFLAG_CABLE_PORTS },
    { "BATTERY", VPFLAG_BATTERY },
    { "POWER_TRANSFER", VPFLAG_POWER_TRANSFER },
    { "HUGE_OK", VPFLAG_HUGE_OK },
    { "NEED_LEG", VPFLAG_NEED_LEG },
    { "IGNORE_LEG_REQUIREMENT", VPFLAG_IGNORE_LEG_REQUIREMENT },
    { "INOPERABLE_SMALL", VPFLAG_INOPERABLE_SMALL },
    { "IGNORE_HEIGHT_REQUIREMENT", VPFLAG_IGNORE_HEIGHT_REQUIREMENT },
};

static std::map<vpart_id, vpart_migration> vpart_migrations;

/** @relates string_id */
template<>
bool string_id<vpart_info>::is_valid() const
{
    return vpart_info_factory.is_valid( *this );
}

/** @relates string_id */
template<>
const vpart_info &string_id<vpart_info>::obj() const
{
    return vpart_info_factory.obj( *this );
}

static void parse_vp_reqs( const JsonObject &obj, const vpart_id &id, const std::string &key,
                           std::vector<std::pair<requirement_id, int>> &reqs,
                           std::map<skill_id, int> &skills, time_duration &time )
{

    if( !obj.has_object( key ) ) {
        return;
    }
    JsonObject src = obj.get_object( key );

    JsonArray sk = src.get_array( "skills" );
    if( !sk.empty() ) {
        skills.clear();
        for( JsonArray cur : sk ) {
            if( cur.size() != 2 ) {
                debugmsg( "vpart '%s' has requirement with invalid skill entry", id.str() );
                continue;
            }
            skills.emplace( skill_id( cur.get_string( 0 ) ), cur.get_int( 1 ) );
        }
    }

    if( src.has_string( "time" ) ) {
        assign( src, "time", time, /* strict = */ false );
    } else if( src.has_int( "time" ) ) { // remove in 0.H
        time = time_duration::from_moves( src.get_int( "time" ) );
        debugmsg( "vpart '%s' defines requirement time as integer, use time units string", id.str() );
    }

    if( src.has_string( "using" ) ) {
        reqs = { { requirement_id( src.get_string( "using" ) ), 1 } };
    } else if( src.has_array( "using" ) ) {
        reqs.clear();
        for( JsonArray cur : src.get_array( "using" ) ) {
            reqs.emplace_back( requirement_id( cur.get_string( 0 ) ), cur.get_int( 1 ) );
        }
    } else {
        reqs.clear();
        // Construct a requirement to capture "components", "qualities", and
        // "tools" that might be listed.
        // NOLINTNEXTLINE(cata-translate-string-literal)
        const requirement_id req_id( string_format( "inline_%s_%s", key.c_str(), id.c_str() ) );
        requirement_data::load_requirement( src, req_id );
        reqs.emplace_back( req_id, 1 );
    }
}

static void parse_vp_control_reqs( const JsonObject &obj, const vpart_id &id,
                                   std::string_view key,
                                   vp_control_req &req )
{
    if( !obj.has_object( key ) ) {
        return;
    }
    JsonObject src = obj.get_object( key );

    JsonArray sk = src.get_array( "skills" );
    if( !sk.empty() ) {
        req.skills.clear();
        for( JsonArray cur : sk ) {
            if( cur.size() != 2 ) {
                debugmsg( "vpart '%s' has requirement with invalid skill entry", id.str() );
                continue;
            }
            req.skills.emplace( skill_id( cur.get_string( 0 ) ), cur.get_int( 1 ) );
        }
    }

    optional( src, false, "proficiencies", req.proficiencies );
}

void vehicles::parts::load( const JsonObject &jo, const std::string &src )
{
    vpart_info_factory.load( jo, src );
}

void vpart_info::handle_inheritance( const vpart_info &copy_from,
                                     const std::unordered_map<std::string, vpart_info> &abstracts )
{
    *this = copy_from; // use assignment operator, like the normal copy-from handling is done

    // vehicle parts have special treatment; the following chunk tries to massage copy-from into
    // looks_like: follows copy-from chain to find the first real part id and sets it as looks_like,
    // or sets it to the last abstract, makes up to 5 jumps.
    looks_like = copy_from.id.str();
    for( int i = 0; i < 5; i++ ) {
        if( vpart_info_factory.is_valid( vpart_id( looks_like ) ) ) {
            break; // real part
        }
        const auto abstract_it = abstracts.find( looks_like );
        if( abstract_it == abstracts.end() ) {
            break; // looks_like might invalid vpart id (but valid furniture/terrain etc tile)
        }
        if( abstract_it->second.looks_like.empty() ) {
            break; // abstract has no looks_like
        }
        looks_like = abstract_it->second.looks_like;
    }
}

void vpart_info::load( const JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";

    assign( jo, "name", name_, strict );
    assign( jo, "item", base_item, strict );
    assign( jo, "remove_as", removed_item, strict );
    assign( jo, "location", location, strict );
    assign( jo, "durability", durability, strict );
    assign( jo, "damage_modifier", dmg_mod, strict );
    assign( jo, "energy_consumption", energy_consumption, strict );
    assign( jo, "power", power, strict );
    assign( jo, "epower", epower, strict );
    assign( jo, "emissions", emissions, strict );
    assign( jo, "exhaust", exhaust, strict );
    assign( jo, "fuel_type", fuel_type, strict );
    assign( jo, "default_ammo", default_ammo, strict );
    assign( jo, "folded_volume", folded_volume, strict );
    assign( jo, "size", size, strict );
    assign( jo, "bonus", bonus, strict );
    assign( jo, "cargo_weight_modifier", cargo_weight_modifier, strict );
    assign( jo, "categories", categories, strict );
    assign( jo, "flags", flags, strict );
    assign( jo, "description", description, strict );
    assign( jo, "color", color, strict );
    assign( jo, "broken_color", color_broken, strict );
    assign( jo, "comfort", comfort, strict );
    int legacy_floor_bedding_warmth = units::to_legacy_bodypart_temp_delta( floor_bedding_warmth );
    assign( jo, "floor_bedding_warmth", legacy_floor_bedding_warmth, strict );
    floor_bedding_warmth = units::from_legacy_bodypart_temp_delta( legacy_floor_bedding_warmth );
    int legacy_bonus_fire_warmth_feet = units::to_legacy_bodypart_temp_delta( bonus_fire_warmth_feet );
    assign( jo, "bonus_fire_warmth_feet", legacy_bonus_fire_warmth_feet, strict );
    bonus_fire_warmth_feet = units::from_legacy_bodypart_temp_delta( legacy_bonus_fire_warmth_feet );

    if( jo.has_array( "variants" ) ) {
        variants.clear();
        for( const JsonObject jo_variant : jo.get_array( "variants" ) ) {
            vpart_variant v;
            v.load( jo_variant, true );
            if( variant_default.empty() ) {
                variant_default = v.id;
            }
            variants[v.id] = v;
        }
    }

    if( jo.has_array( "variants_bases" ) ) {
        variants_bases.clear();
        for( const JsonObject jo : jo.get_array( "variants_bases" ) ) {
            variants_bases.emplace_back( jo.get_string( "id" ), jo.get_string( "label" ) );
        }
    }

    if( jo.has_member( "requirements" ) ) {
        JsonObject reqs = jo.get_object( "requirements" );

        parse_vp_reqs( reqs, id, "install", install_reqs, install_skills, install_moves );
        parse_vp_reqs( reqs, id, "removal", removal_reqs, removal_skills, removal_moves );
        parse_vp_reqs( reqs, id, "repair",  repair_reqs,  repair_skills, repair_moves );
    }

    if( jo.has_member( "control_requirements" ) ) {
        JsonObject reqs = jo.get_object( "control_requirements" );

        parse_vp_control_reqs( reqs, id, "air", control_air );
        parse_vp_control_reqs( reqs, id, "land", control_land );
    }

    assign( jo, "looks_like", looks_like, strict );

    if( jo.has_member( "breaks_into" ) ) {
        breaks_into_group = item_group::load_item_group(
                                jo.get_member( "breaks_into" ), "collection", "breaks_into for vpart " + id.str() );
    }

    JsonArray qual = jo.get_array( "qualities" );
    if( !qual.empty() ) {
        qualities.clear();
        for( JsonArray pair : qual ) {
            qualities[ quality_id( pair.get_string( 0 ) ) ] = pair.get_int( 1 );
        }
    }

    JsonArray tools = jo.get_array( "pseudo_tools" );
    if( !tools.empty() ) {
        pseudo_tools.clear();
        while( tools.has_more() ) {
            const JsonObject tooldef = tools.next_object();
            const itype_id tool_id( tooldef.get_string( "id" ) );
            const std::string hotkey_str = tooldef.has_string( "hotkey" ) ? tooldef.get_string( "hotkey" ) : "";
            const int hotkey = hotkey_str.empty() ? -1 : static_cast<int>( hotkey_str[0] );
            if( !pseudo_tools.insert( { tool_id, hotkey } ).second ) {
                debugmsg( "Insert failed on %s to %s's tools, duplicate?", tool_id.str(), id.str() );
            }
        }
    }
    assign( jo, "folding_tools", folding_tools, strict );
    assign( jo, "unfolding_tools", unfolding_tools, strict );
    assign( jo, "folding_time", folding_time, strict );
    assign( jo, "unfolding_time", unfolding_time, strict );

    if( jo.has_member( "damage_reduction" ) ) {
        JsonObject dred = jo.get_object( "damage_reduction" );
        damage_reduction = load_damage_map( dred );
    }

    if( has_flag( "ENGINE" ) ) {
        if( !engine_info ) {
            engine_info.emplace();
        }
        assign( jo, "backfire_threshold", engine_info->backfire_threshold, strict );
        assign( jo, "backfire_freq", engine_info->backfire_freq, strict );
        assign( jo, "noise_factor", engine_info->noise_factor, strict );
        assign( jo, "damaged_power_factor", engine_info->damaged_power_factor, strict );
        assign( jo, "m2c", engine_info->m2c, strict );
        assign( jo, "muscle_power_factor", engine_info->muscle_power_factor, strict );
        assign( jo, "exclusions", engine_info->exclusions, strict );
        assign( jo, "fuel_options", engine_info->fuel_opts, strict );
    }

    if( has_flag( "WHEEL" ) ) {
        if( !wheel_info ) {
            wheel_info.emplace();
        }

        assign( jo, "rolling_resistance", wheel_info->rolling_resistance, strict );
        assign( jo, "contact_area", wheel_info->contact_area, strict );
        assign( jo, "wheel_offroad_rating", wheel_info->offroad_rating, strict );
        if( const std::optional<JsonValue> jo_termod = jo.get_member_opt( "wheel_terrain_modifiers" ) ) {
            wheel_info->terrain_modifiers.clear();
            for( const JsonMember jo_mod : static_cast<JsonObject>( *jo_termod ) ) {
                const JsonArray jo_mod_values = jo_mod.get_array();
                veh_ter_mod mod { jo_mod.name(), jo_mod_values.get_int( 0 ), jo_mod_values.get_int( 1 ) };
                wheel_info->terrain_modifiers.emplace_back( std::move( mod ) );
            }
        }
    }

    if( has_flag( "ROTOR" ) ) {
        if( !rotor_info ) {
            rotor_info.emplace();
        }
        assign( jo, "rotor_diameter", rotor_info->rotor_diameter, strict );
    }

    if( has_flag( "WORKBENCH" ) ) {
        if( !workbench_info ) {
            workbench_info.emplace();
        }

        JsonObject wb_jo = jo.get_object( "workbench" );
        assign( wb_jo, "multiplier", workbench_info->multiplier, strict );
        assign( wb_jo, "mass", workbench_info->allowed_mass, strict );
        assign( wb_jo, "volume", workbench_info->allowed_volume, strict );
    }

    if( has_flag( "VEH_TOOLS" ) ) {
        if( !toolkit_info ) {
            toolkit_info.emplace();
        }
        assign( jo, "allowed_tools", toolkit_info->allowed_types, strict );
    }

    if( has_flag( "TRANSFORM_TERRAIN" ) || has_flag( "CRASH_TERRAIN_AROUND" ) ) {
        if( !transform_terrain_info ) {
            transform_terrain_info.emplace();
        }
        JsonObject jttd = jo.get_object( "transform_terrain" );
        vpslot_terrain_transform &vtt = *transform_terrain_info;
        optional( jttd, was_loaded, "pre_flags", vtt.pre_flags, {} );
        optional( jttd, was_loaded, "post_terrain", vtt.post_terrain );
        optional( jttd, was_loaded, "post_furniture", vtt.post_furniture );
        if( jttd.has_string( "post_field" ) ) {
            mandatory( jttd, was_loaded, "post_field", vtt.post_field );
            mandatory( jttd, was_loaded, "post_field_intensity", vtt.post_field_intensity );
            mandatory( jttd, was_loaded, "post_field_age", vtt.post_field_age );
        }
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

const std::set<std::string> &vpart_info::get_categories() const
{
    return this->categories;
}

// @returns true for "valid" gun items which can be mounted as vehicle turret
static bool mountable_gun_filter( const itype &guntype )
{
    static const std::vector<flag_id> bad_flags {
        flag_BIONIC_WEAPON,
        flag_NO_TURRET,
        flag_NO_UNWIELD,
        flag_PSEUDO,
        flag_RELOAD_AND_SHOOT,
        flag_STR_DRAW,
    };

    if( !guntype.gun || guntype.gunmod ) {
        return false;
    }

    return std::none_of( bad_flags.cbegin(), bad_flags.cend(), [&guntype]( const flag_id & flag ) {
        return guntype.has_flag( flag );
    } );
}

// @returns true if itype uses liquid ammo directly or has a magwell + magazine pocket that accepts liquid ammo
static bool gun_uses_liquid_ammo( const itype &guntype )
{
    for( const ammotype &at : guntype.gun->ammo ) {
        if( at->default_ammotype()->phase == phase_id::LIQUID ) {
            return true;
        }
    }
    for( const pocket_data &maybe_magwell : guntype.pockets ) {
        for( const itype_id &restricted_types : maybe_magwell.item_id_restriction ) {
            for( const pocket_data &maybe_mag : restricted_types.obj().pockets ) {
                for( const std::pair<const ammotype, int> &res : maybe_mag.ammo_restriction ) {
                    if( res.first->default_ammotype()->phase == phase_id::LIQUID ) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

// @returns Amount of battery drained on default gunmode if itype uses magwell+mag with battery ammo
static int gun_battery_mags_drain( const itype &guntype )
{
    int charges_used = 0;
    for( const pocket_data &maybe_mag_well : guntype.pockets ) {
        for( const itype_id &restricted_type : maybe_mag_well.item_id_restriction ) {
            for( const pocket_data &maybe_mag : restricted_type.obj().pockets ) {
                for( const std::pair<const ammotype, int> &res : maybe_mag.ammo_restriction ) {
                    if( res.first == ammo_battery ) {
                        charges_used = std::max( charges_used, guntype.gun->ammo_to_fire );
                    }
                }
            }
        }
    }
    if( guntype.gun->energy_drain > 0_kJ && charges_used > 0 ) {
        debugmsg( "%s uses both energy and battery magazines", guntype.nname( 1 ) );
    }
    return charges_used;
}

static std::string get_looks_like( const vpart_info &vpi, const itype &it )
{
    static const auto has_light_ammo = []( const std::set<ammotype> &ats ) {
        for( const ammotype &at : ats ) {
            if( !at->default_ammotype() ) {
                continue;
            }
            const units::mass at_weight = at->default_ammotype()->weight;
            if( 15_gram > at_weight && at_weight > 3_gram ) {
                return true; // detects g80 and coil gun
            }
        }
        return false;
    };

    if( it.gun->energy_drain > 0_kJ && has_light_ammo( it.gun->ammo ) ) {
        return "mounted_hk_g80"; // railguns
    } else if( vpi.has_flag( "USE_BATTERIES" ) || it.gun->energy_drain > 0_kJ ) {
        return "laser_rifle"; // generic energy weapons
    } else if( vpi.has_flag( "USE_TANKS" ) ) {
        return "watercannon"; // liquid sprayers (flamethrower, foam gun etc)
    } else if( it.gun->skill_used == skill_launcher ) {
        return "tow_launcher"; // launchers
    } else {
        return "m249"; // machine guns and also default for any unknown
    }
}

void vehicles::parts::finalize()
{
    vpart_info_factory.finalize();

    for( const itype *const item : item_controller->find( mountable_gun_filter ) ) {
        vpart_info new_part = *vpart_turret_generic; // copy from generic
        const itype_id item_id = item->get_id();

        new_part.id = vpart_id( "turret_" + item_id.str() );
        new_part.base_item = item_id;
        new_part.description = item->description;
        new_part.color = item->color;

        // calculate requirements for install/removal by gross estimate
        int primary_req = 3; // 3 in whatever gun's skill is set to
        int mechanics_req = 3 + static_cast<int>( item->weight / 10_kilogram );
        int electronics_req = 0; // calculated below

        // if turret base item is migrated-from or blacklisted hide it from install menu
        if( item_id != item_controller->migrate_id( item_id ) ||
            item_is_blacklisted( item->get_id() ) ) {
            new_part.set_flag( "NO_INSTALL_HIDDEN" );
        }

        if( gun_uses_liquid_ammo( *item ) ) {
            new_part.set_flag( "USE_TANKS" );
            // +1 mechanics level for liquid plumbing
            mechanics_req++;
        }

        const int battery_mags_drain = gun_battery_mags_drain( *item );
        if( battery_mags_drain ) {
            // USE_BATTERIES for afs cartridge-like batteries
            new_part.set_flag( "USE_BATTERIES" );
            // +1 electronics level per 15 battery charges used for electric plumbing
            electronics_req += static_cast<int>( std::ceil( battery_mags_drain / 15.0 ) );
        }

        // make part foldable for pistols and SMGs
        bool is_foldable = item->qualities.count( qual_PISTOL ) > 0 ||
                           item->qualities.count( qual_SMG ) > 0;
        if( is_foldable ) {
            new_part.folded_volume = item->volume;
        }

        // cap all skills at 8
        primary_req = std::min( 8, primary_req );
        mechanics_req = std::min( 8, mechanics_req );
        electronics_req = std::min( 8, electronics_req );

        new_part.install_skills.emplace( item->gun->skill_used, primary_req );
        new_part.removal_skills.emplace( item->gun->skill_used, primary_req / 2 );

        new_part.install_skills.emplace( "mechanics", mechanics_req );
        new_part.removal_skills.emplace( "mechanics", mechanics_req / 2 );

        if( electronics_req > 0 ) {
            new_part.install_skills.emplace( "electronics", electronics_req );
            new_part.removal_skills.emplace( "electronics", electronics_req / 2 );
        }

        int difficulty = primary_req + mechanics_req + electronics_req * 2;
        time_duration install_time = 10_minutes * difficulty;
        time_duration removal_time = 10_minutes * difficulty / 2;

        new_part.install_moves = install_time;
        new_part.removal_moves = removal_time;

        new_part.looks_like = get_looks_like( new_part, *item );

        vpart_info_factory.insert( new_part );
    }

    // hide the generic turret prototype
    vpart_info &vpi_turret_generic = const_cast<vpart_info &>( *vpart_turret_generic );
    vpi_turret_generic.set_flag( "NO_INSTALL_HIDDEN" );

    for( const vpart_info &const_vpi : vehicles::parts::get_all() ) {
        // const_cast hack until/if generic factory supports finalize
        vpart_info &vpi = const_cast<vpart_info &>( const_vpi );
        vpi.finalize();
    }
}

void vpart_info::finalize()
{
    if( engine_info && engine_info->fuel_opts.empty() && !fuel_type.is_null() ) {
        engine_info->fuel_opts.push_back( fuel_type );
    }

    for( const std::string &flag : get_flags() ) {
        auto b = vpart_bitflag_map.find( flag );
        if( b != vpart_bitflag_map.end() ) {
            set_flag( flag ); // refresh bitflags field
        }
    }

    if( has_flag( VPFLAG_APPLIANCE ) ) {
        // force all appliances' location field to "structure"
        // dragging code currently checks this for considering collisions
        location = "structure";
    }

    finalize_damage_map( damage_reduction );

    // Calculate and cache z-ordering based off of location
    // list_order is used when inspecting the vehicle
    if( location == "on_roof" ) {
        z_order = 9;
        list_order = 3;
    } else if( location == "on_cargo" ) {
        z_order = 8;
        list_order = 6;
    } else if( location == "center" ) {
        z_order = 7;
        list_order = 7;
    } else if( location == "under" ) {
        // Have wheels show up over frames
        z_order = 6;
        list_order = 10;
    } else if( location == "structure" ) {
        z_order = 5;
        list_order = 1;
    } else if( location == "engine_block" ) {
        // Should be hidden by frames
        z_order = 4;
        list_order = 8;
    } else if( location == "on_battery_mount" ) {
        // Should be hidden by frames
        z_order = 3;
        list_order = 10;
    } else if( location == "fuel_source" ) {
        // Should be hidden by frames
        z_order = 3;
        list_order = 9;
    } else if( location == "roof" ) {
        // Shouldn't be displayed
        z_order = -1;
        list_order = 4;
    } else if( location == "armor" ) {
        // Shouldn't be displayed (the color is used, but not the symbol)
        z_order = -2;
        list_order = 2;
    } else {
        // Everything else
        z_order = 0;
        list_order = 5;
    }
    std::set<std::pair<itype_id, int>> &pt = pseudo_tools;
    for( auto it = pt.begin(); it != pt.end(); /* blank */ ) {
        const itype_id &tool = it->first;
        if( !tool.is_valid() ) {
            debugmsg( "invalid pseudo tool itype_id '%s' on '%s'", tool.str(), id.str() );
            pt.erase( it++ );
        } else {
            ++it;
        }
    }
    if( toolkit_info.has_value() ) {
        std::set<itype_id> &pt = toolkit_info->allowed_types;
        for( auto it = pt.begin(); it != pt.end(); /* blank */ ) {
            if( !it->is_valid() ) {
                debugmsg( "invalid itype_id '%s' on 'allowed_tools' of vpart '%s'", it->str(), id.str() );
                pt.erase( it++ );
            } else {
                ++it;
            }
        }
    }

    // generate variant bases (cosmetic only feature for tilesets)
    std::vector<vpart_variant> new_variants;
    for( const auto &[base_id, base_label] : variants_bases ) {
        for( const auto &[vvid, vv] : variants ) {
            vpart_variant vv_copy = vv;
            vv_copy.id = vv_copy.id.empty() ? base_id : base_id + "_" + vv_copy.id;
            vv_copy.label_ = vv_copy.label_.empty() ? base_label : base_label + " " + vv_copy.label_;
            new_variants.emplace_back( vv_copy );
        }
    }
    for( const vpart_variant &vv : new_variants ) {
        variants[vv.id] = vv;
    }
    if( variants.empty() ) {
        debugmsg( "vehicle part %s defines no variants", id.str() );
        vpart_variant vv;
        vv.id.clear();
        vv.label_ = "Default";
        vv.symbols.fill( '?' );
        vv.symbols_broken.fill( '?' );
        variants.emplace( vv.id, vv );
        variant_default = vv.id;
    }
    if( variants.count( variant_default ) == 0 ) {
        variant_default = variants.begin()->first;
    }

    // add the base item to the installation requirements
    // TODO: support multiple/alternative base items
    requirement_data ins;
    ins.components.push_back( { { { base_item, 1 } } } );

    const requirement_id ins_id( std::string( "inline_vehins_base_" ) + id.str() );
    requirement_data::save_requirement( ins, ins_id );
    install_reqs.emplace_back( ins_id, 1 );

    if( removal_moves < 0_seconds ) {
        removal_moves = install_moves / 2;
    }

    if( !item::type_is_defined( fuel_type ) ) {
        debugmsg( "vehicle part %s uses undefined fuel %s", id.c_str(), base_item.c_str() );
        fuel_type = itype_id::NULL_ID();
    }
    if( !fuel_type.is_null() ) {
        const item base_item_as_container( base_item, calendar::turn_zero );
        const item fuel_containee( fuel_type, calendar::turn_zero );
        if( !fuel_containee.is_fuel() && !base_item_as_container.can_contain( fuel_containee ).success() ) {
            // HACK: Tanks are allowed to specify non-fuel "fuel",
            // because currently legacy blazemod uses it as a hack to restrict content types
            debugmsg( "non-tank vehicle part %s uses non-fuel item %s as fuel, setting to null",
                      id.c_str(), fuel_type.c_str() );
            fuel_type = itype_id::NULL_ID();
        }
    }
}

void vehicles::parts::check()
{
    for( const vpart_info &vpi : vehicles::parts::get_all() ) {
        vpi.check();
    }
}

void vpart_info::check() const
{
    if( id.str().find( vehicles::variant_separator ) != std::string::npos ) {
        debugmsg( "vehicle part '%s' uses reserved character %c", id.str(), vehicles::variant_separator );
    }

    for( const auto&[vid, vv] : variants ) {
        for( size_t i = 0; i < vv.symbols.size(); i++ ) {
            if( ( mk_wcwidth( vv.symbols[i] ) != 1 ) ||
                ( mk_wcwidth( vv.symbols_broken[i] ) != 1 ) ) {
                debugmsg( "vehicle part '%s' variant '%s' has symbol that is not 1 console cell wide",
                          id.str(), vid );
            }
        }
    }
    for( const auto &[skill_id, skill_level] : install_skills ) {
        if( !skill_id.is_valid() ) {
            debugmsg( "vehicle part %s has unknown install skill %s", id.str(), skill_id.str() );
        }
    }

    for( const auto &[skill_id, skill_level] : removal_skills ) {
        if( !skill_id.is_valid() ) {
            debugmsg( "vehicle part %s has unknown removal skill %s", id.str(), skill_id.str() );
        }
    }

    for( const auto &[skill_id, skill_level] : repair_skills ) {
        if( !skill_id.is_valid() ) {
            debugmsg( "vehicle part %s has unknown repair skill %s", id.str(), skill_id.str() );
        }
    }

    for( const auto &[req_id, req_amount] : install_reqs ) {
        if( !req_id.is_valid() || req_amount <= 0 ) {
            debugmsg( "vehicle part %s has unknown or incorrectly specified install requirements %s",
                      id.str(), req_id.str() );
        }
    }

    for( const auto &[req_id, req_amount] : removal_reqs ) {
        if( !( req_id.is_null() || req_id.is_valid() ) || req_amount < 0 ) {
            debugmsg( "vehicle part %s has unknown or incorrectly specified removal requirements %s",
                      id.str(), req_id.str() );
        }
    }

    for( const auto &[req_id, req_amount] : repair_reqs ) {
        if( !( req_id.is_null() || req_id.is_valid() ) || req_amount < 0 ) {
            debugmsg( "vehicle part %s has unknown or incorrectly specified repair requirements %s",
                      id.str(), req_id.str() );
        }
    }

    if( install_moves < 0_seconds ) {
        debugmsg( "vehicle part %s has negative installation time", id.str() );
    }

    if( removal_moves < 0_seconds ) {
        debugmsg( "vehicle part %s has negative removal time", id.str() );
    }

    if( !item_group::group_is_defined( breaks_into_group ) ) {
        debugmsg( "Vehicle part %s breaks into non-existent item group %s.",
                  id.str(), breaks_into_group.str() );
    }
    for( const std::string &flag : get_flags() ) {
        if( !json_flag::get( flag ) ) {
            debugmsg( "vehicle part %s has unknown flag %s", id.str(), flag );
        }
    }
    if( durability <= 0 ) {
        debugmsg( "vehicle part %s has zero or negative durability", id.str() );
    }
    if( dmg_mod < 0 ) {
        debugmsg( "vehicle part %s has negative damage modifier", id.str() );
    }
    if( folded_volume && folded_volume.value() <= 0_ml ) {
        debugmsg( "vehicle part %s has folding part with zero or negative folded volume", id.str() );
    }
    if( !default_ammo.is_valid() ) {
        debugmsg( "vehicle part %s has undefined default ammo %s", id.str(), base_item.str() );
    }
    if( size != 0_ml ) {
        if( size < 0_ml ) {
            debugmsg( "vehicle part %s has negative size", id.str() );
        }
        if( !has_flag( "CARGO" ) ) {
            debugmsg( "vehicle part %s is not CARGO and cannot define size", id.str() );
        } else if( has_flag( "FLUIDTANK" ) ) {
            debugmsg( "vehicle part %s is FLUIDTANK and cannot define size; it's defined on part's base item",
                      id.str() );
        }
    }
    if( has_flag( "CARGO" ) && has_flag( "FLUIDTANK" ) ) {
        debugmsg( "vehicle part %s can't have both CARGO and FLUIDTANK flags at the same time", id.str() );
    }
    if( !item::type_is_defined( base_item ) ) {
        debugmsg( "vehicle part %s uses undefined item %s", id.str(), base_item.str() );
    }

    if( has_flag( "TURRET" ) && !base_item->gun ) {
        debugmsg( "vehicle part %s has the TURRET flag, but is not made from a gun item", id.str() );
    }

    for( const emit_id &e : emissions ) {
        if( !e.is_valid() ) {
            debugmsg( "vehicle part %s has invalid emission %s was set", id.str(), e.str() );
        }
    }
    for( const emit_id &e : exhaust ) {
        if( !e.is_valid() ) {
            debugmsg( "vehicle part %s has invalid exhaust %s was set", id.str(), e.str() );
        }
    }

    if( has_flag( "WHEEL" ) && !base_item->wheel ) {
        debugmsg( "vehicle part %s has the WHEEL flag, but base item %s is not a wheel.  "
                  "THIS WILL CRASH!", id.str(), base_item.str() );
    }

    if( has_flag( "WHEEL" ) && !has_flag( "UNSTABLE_WHEEL" ) && !has_flag( "STABLE" ) ) {
        debugmsg( "Wheel '%s' lacks either 'UNSTABLE_WHEEL' or 'STABLE' flag.", id.str() );
    }

    if( has_flag( "UNSTABLE_WHEEL" ) && has_flag( "STABLE" ) ) {
        debugmsg( "Wheel '%s' cannot be both an 'UNSTABLE_WHEEL' and 'STABLE'.", id.str() );
    }

    for( const auto &[qual_id, qual_level] : qualities ) {
        if( !qual_id.is_valid() ) {
            debugmsg( "vehicle part %s has undefined tool quality %s", id.str(), qual_id.str() );
        }
    }
    if( has_flag( VPFLAG_ENABLED_DRAINS_EPOWER ) && epower == 0_W ) {
        debugmsg( "%s is set to drain epower, but has epower == 0", id.str() );
    }
    if( has_flag( VPFLAG_ENGINE ) ) {
        if( power != 0_W && fuel_type == fuel_type_animal ) {
            debugmsg( "engine part '%s' powered by '%s' can't define non-zero 'power'",
                      id.str(), fuel_type.str() );
        } else if( power == 0_W && fuel_type != fuel_type_animal ) {
            debugmsg( "engine part '%s' powered by '%s' must define non zero 'power'",
                      id.str(), fuel_type.str() );
        }
    }
    // Parts with non-zero epower must have a flag that affects epower usage
    static const std::vector<std::string> handled = {{
            "ENABLED_DRAINS_EPOWER", "SECURITY", "ENGINE",
            "ALTERNATOR", "SOLAR_PANEL", "POWER_TRANSFER",
            "REACTOR", "WIND_TURBINE", "WATER_WHEEL"
        }
    };
    if( epower != 0_W &&
    std::none_of( handled.begin(), handled.end(), [this]( const std::string & flag ) {
    return has_flag( flag );
    } ) ) {
        const std::string warnings_are_good_docs = enumerate_as_string( handled );
        debugmsg( "%s has non-zero epower, but lacks a flag that would make it affect epower (one of %s)",
                  id.str(), warnings_are_good_docs );
    }
    if( base_item->pockets.size() > 4 ) {
        debugmsg( "Error: vehicle parts assume only one pocket.  Multiple pockets unsupported" );
    }
    for( const auto &dt : damage_reduction ) {
        if( !dt.first.is_valid() ) {
            debugmsg( "Invalid damage_reduction type \"%s\" for vehicle part %s", dt.first.c_str(),
                      id.c_str() );
        }
    }
    if( !!transform_terrain_info && !( transform_terrain_info->post_terrain ||
                                       transform_terrain_info->post_furniture ||
                                       transform_terrain_info->post_field ) ) {
        debugmsg( "transform_terrain_info must contain at least one of post_terrain, post_furniture and post_field for vehicle part %s",
                  id.c_str() );
    }
}

void vehicles::parts::reset()
{
    vpart_info_factory.reset();
}

const std::vector<vpart_info> &vehicles::parts::get_all()
{
    return vpart_info_factory.get_all();
}

std::string vpart_info::name() const
{
    std::string res = name_.translated();
    if( res.empty() ) {
        res = item::nname( base_item ); // fallback to base item's translation
    }
    if( has_flag( "TURRET" ) ) {
        res = string_format( pgettext( "mounted turret prefix", "mounted %s" ), res );
    }
    return res;
}

int vpart_info::format_description( std::string &msg, const nc_color &format_color,
                                    int width ) const
{
    int lines = 1;
    msg += _( "<color_white>Description</color>\n" );
    msg += "> <color_" + string_from_color( format_color ) + ">";

    std::string long_descrip;
    // handle appending text and double-whitespace
    const auto append_desc = [&long_descrip]( const std::string & text ) {
        if( text.empty() ) {
            return;
        }
        if( !long_descrip.empty() ) {
            long_descrip += _( "  " );
        }
        long_descrip += text;
    };

    append_desc( description.translated() );

    for( const std::string &flagid : flags ) {
        if( flagid == "ALARMCLOCK" || flagid == "WATCH" ) {
            continue;
        } else if( flagid == "ENABLED_DRAINS_EPOWER" ||
                   flagid == "ENGINE" ) { // ENGINEs get the same description
            if( epower < 0_W ) {
                append_desc( string_format( json_flag::get( "ENABLED_DRAINS_EPOWER" ).info(),
                                            -units::to_watt( epower ) ) );
            }
        } else if( flagid == "ALTERNATOR" ||
                   flagid == "SOLAR_PANEL" ||
                   flagid == "WATER_WHEEL" ||
                   flagid == "WIND_TURBINE" ) {
            append_desc( string_format( json_flag::get( flagid ).info(), units::to_watt( epower ) ) );
        } else {
            append_desc( json_flag::get( flagid ).info() );
        }
    }
    if( has_flag( "TURRET" ) ) {
        class::item base( base_item );
        if( base.ammo_required() && !base.ammo_remaining( ) ) {
            itype_id default_ammo = base.magazine_current() ? base.common_ammo_default() : base.ammo_default();
            if( !default_ammo.is_null() ) {
                base.ammo_set( default_ammo );
            } else if( !base.magazine_default().is_null() ) {
                class::item tmp_mag( base.magazine_default() );
                tmp_mag.ammo_set( tmp_mag.ammo_default() );
                base.put_in( tmp_mag, pocket_type::MAGAZINE_WELL );
            }
        }
        long_descrip += string_format( _( "\nRange: %1$5d     Damage: %2$5.0f" ),
                                       base.gun_range( true ),
                                       base.gun_damage().total_damage() );
    }

    if( !get_pseudo_tools().empty() ) {
        append_desc( string_format( "\n%s<color_cyan>%s</color>", _( "Provides: " ),
        enumerate_as_string( get_pseudo_tools(), []( const std::pair<itype_id, int> &p ) {
            return p.first.obj().nname( 1 );
        } ) ) );
    }

    if( toolkit_info && !toolkit_info->allowed_types.empty() ) {
        append_desc( string_format( "\n%s<color_cyan>%s</color>", _( "Allows connecting: " ),
        enumerate_as_string( toolkit_info->allowed_types, []( const itype_id & it ) {
            return it->nname( 1 );
        } ) ) );
    }

    if( !long_descrip.empty() ) {
        const auto wrap_descrip = foldstring( long_descrip, width );
        msg += wrap_descrip[0];
        for( size_t i = 1; i < wrap_descrip.size(); i++ ) {
            msg += "\n  " + wrap_descrip[i];
        }
        msg += "</color>\n";
        lines += wrap_descrip.size();
    }

    // borrowed from item.cpp and adjusted
    for( const auto &qual : qualities ) {
        msg += string_format(
                   _( "Has level <color_cyan>%1$d %2$s</color> quality" ), qual.second, qual.first.obj().name );
        if( qual.first == qual_JACK || qual.first == qual_LIFT ) {
            msg += string_format( _( " and is rated at <color_cyan>%1$d %2$s</color>" ),
                                  static_cast<int>( convert_weight( lifting_quality_to_mass( qual.second ) ) ),
                                  weight_units() );
        }
        msg += ".\n";
        lines += 1;
    }
    return lines;
}

requirement_data vpart_info::install_requirements() const
{
    return requirement_data( install_reqs );
}

requirement_data vpart_info::removal_requirements() const
{
    return requirement_data( removal_reqs );
}

requirement_data vpart_info::repair_requirements() const
{
    return requirement_data( repair_reqs );
}

bool vpart_info::is_repairable() const
{
    return !has_flag( "NO_REPAIR" ) && !repair_requirements().is_empty();
}

static time_duration scale_time( const std::map<skill_id, int> &sk, time_duration mv,
                                 const Character &you )
{
    if( sk.empty() ) {
        return mv;
    }

    const int lvl = std::accumulate( sk.begin(), sk.end(), 0, [&you]( int lhs,
    const std::pair<skill_id, int> &rhs ) {
        return lhs + std::max( std::min( static_cast<int>( you.get_skill_level( rhs.first ) ),
                                         MAX_SKILL ) - rhs.second, 0 );
    } );
    // 10% per excess level (reduced proportionally if >1 skill required) with max 50% reduction
    // 10% reduction per assisting Character
    const int helpersize = you.get_num_crafting_helpers( 3 );
    return mv * ( 1.0 - std::min( static_cast<double>( lvl ) / sk.size() / 10.0,
                                  0.5 ) ) * ( 1 - ( helpersize / 10.0 ) );
}

time_duration vpart_info::install_time( const Character &you ) const
{
    return scale_time( install_skills, install_moves, you );
}

time_duration vpart_info::removal_time( const Character &you ) const
{
    return scale_time( removal_skills, removal_moves, you );
}

time_duration vpart_info::repair_time( const Character &you ) const
{
    return scale_time( repair_skills, repair_moves, you );
}

bool vpart_info::has_category( const std::string &category ) const
{
    return this->categories.find( category ) != this->categories.end();
}

std::set<std::pair<itype_id, int>> vpart_info::get_pseudo_tools() const
{
    return pseudo_tools;
}

std::vector<itype_id> vpart_info::get_folding_tools() const
{
    return folding_tools;
}

std::vector<itype_id> vpart_info::get_unfolding_tools() const
{
    return unfolding_tools;
}

time_duration vpart_info::get_folding_time() const
{
    return folding_time;
}

time_duration vpart_info::get_unfolding_time() const
{
    return unfolding_time;
}

bool vpart_info::has_control_req() const
{
    return !control_air.proficiencies.empty() || !control_air.skills.empty() ||
           !control_land.proficiencies.empty() || !control_land.skills.empty();
}

std::string vpart_variant::get_label() const
{
    return to_translation( "vpart_variants", label_ ).translated();
}

char32_t vpart_variant::get_symbol( units::angle direction, bool is_broken ) const
{
    const int dir8 = angle_to_dir8( direction );
    return is_broken ? symbols_broken[dir8] : symbols[dir8];
}

static int utf32_to_ncurses_ACS( char32_t sym )
{
    switch( sym ) {
        // *INDENT-OFF*
        case 0x2500: return LINE_OXOX; // horizontal line
        case 0x2501: return '=';       // horizontal line (heavy) no acs equivalent
        case 0x2502: return LINE_XOXO; // vertical line
        case 0x2503: return 'H';       // vertical line (heavy) no acs equivalent
        case 0x253C: return LINE_XXXX; // cross/plus
        case 0x250C: return LINE_OXXO; // upper left corner
        case 0x2510: return LINE_OOXX; // upper right corner
        case 0x2518: return LINE_XOOX; // lower right corner
        case 0x2514: return LINE_XXOO; // lower left corner
        default:     return static_cast<int>(sym);
        // *INDENT-ON*
    }
}

int vpart_variant::get_symbol_curses( units::angle direction, bool is_broken ) const
{
    return get_symbol_curses( get_symbol( direction, is_broken ) );
}

int vpart_variant::get_symbol_curses( char32_t sym )
{
    return utf32_to_ncurses_ACS( sym );
}

void vpart_variant::load( const JsonObject &jo, bool was_loaded )
{
    optional( jo, was_loaded, "id", id, "" );
    optional( jo, was_loaded, "label", label_ );
    const auto symbol_read = [&]( std::array<char32_t, 8> &dst, const std::string & member ) {
        if( jo.has_string( member ) ) {
            const std::vector<std::string> symbols = utf8_display_split( jo.get_string( member ) );
            if( symbols.size() == 1 ) {
                dst.fill( UTF8_getch( jo.get_string( member ) ) );
            } else if( symbols.size() == 8 ) {
                for( size_t i = 0; i < 8; i++ ) {
                    dst[i] = UTF8_getch( symbols[i] );
                }
                return;
            } else {
                dst.fill( '?' );
                jo.throw_error_at( "symbols",
                                   string_format( "'%s' must be string with 1 or 8 characters", member ) );
            }
        } else {
            dst.fill( '?' );
            jo.throw_error( string_format( "'%s' is missing", member ) );
        }
    };
    symbol_read( symbols, "symbols" );
    symbol_read( symbols_broken, "symbols_broken" );
}

/** @relates string_id */
template<>
const vehicle_prototype &string_id<vehicle_prototype>::obj() const
{
    return vehicle_prototype_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<vehicle_prototype>::is_valid() const
{
    return vehicle_prototype_factory.is_valid( *this );
}

void vehicles::load_prototype( const JsonObject &jo, const std::string &src )
{
    vehicle_prototype_factory.load( jo, src );
}

const std::vector<vehicle_prototype> &vehicles::get_all_prototypes()
{
    return vehicle_prototype_factory.get_all();
}

void vehicles::reset_prototypes()
{
    vehicle_prototype_factory.reset();
}

static std::pair<std::string, std::string> get_vpart_str_variant( const std::string &vpid )
{
    const size_t loc = vpid.rfind( vehicles::variant_separator );
    return loc == std::string::npos
           ? std::make_pair( vpid, "" )
           : std::make_pair( vpid.substr( 0, loc ), vpid.substr( loc + 1 ) );
}

void vehicle_prototype::load( const JsonObject &jo, std::string_view )
{
    vgroups[vgroup_id( id.str() )].add_vehicle( id, 100 );
    optional( jo, was_loaded, "name", name );

    const auto add_part_obj = [&]( const JsonObject & part, point_rel_ms pos ) {
        const auto [id, variant] = get_vpart_str_variant( part.get_string( "part" ) );
        part_def pt;
        pt.part = vpart_id( id );
        pt.variant = variant;
        pt.pos = pos;

        assign( part, "ammo", pt.with_ammo, true, 0, 100 );
        assign( part, "ammo_types", pt.ammo_types, true );
        assign( part, "ammo_qty", pt.ammo_qty, true, 0 );
        assign( part, "fuel", pt.fuel, true );
        assign( part, "tools", pt.tools, true );

        parts.emplace_back( pt );
    };

    const auto add_part_string = [&]( const std::string & part, point_rel_ms pos ) {
        const auto [id, variant] = get_vpart_str_variant( part );
        part_def pt;
        pt.part = vpart_id( id );
        pt.variant = variant;
        pt.pos = pos;
        parts.emplace_back( pt );
    };

    if( jo.has_member( "blueprint" ) ) {
        // currently unused, read to suppress unvisited members warning
        jo.get_array( "blueprint" );
    }

    for( JsonObject part : jo.get_array( "parts" ) ) {
        point_rel_ms pos{ part.get_int( "x" ), part.get_int( "y" ) };

        if( part.has_string( "part" ) ) {
            add_part_obj( part, pos );
            debugmsg( "vehicle prototype '%s' uses deprecated string definition for part"
                      " '%s', use 'parts' array instead", id.str(), part.get_string( "part" ) );
        } else if( part.has_array( "parts" ) ) {
            for( const JsonValue entry : part.get_array( "parts" ) ) {
                if( entry.test_string() ) {
                    std::string part_name = entry.get_string();
                    add_part_string( part_name, pos );
                } else {
                    JsonObject subpart = entry.get_object();
                    add_part_obj( subpart, pos );
                }
            }
        }
    }

    for( JsonObject spawn_info : jo.get_array( "items" ) ) {
        vehicle_item_spawn next_spawn;
        next_spawn.pos.x() = spawn_info.get_int( "x" );
        next_spawn.pos.y() = spawn_info.get_int( "y" );

        next_spawn.chance = spawn_info.get_int( "chance" );
        if( next_spawn.chance <= 0 || next_spawn.chance > 100 ) {
            debugmsg( "Invalid spawn chance in %s (%d, %d): %d%%",
                      name, next_spawn.pos.x(), next_spawn.pos.y(), next_spawn.chance );
        }

        // constrain both with_magazine and with_ammo to [0-100]
        next_spawn.with_magazine = std::max( std::min( spawn_info.get_int( "magazine",
                                             next_spawn.with_magazine ), 100 ), 0 );
        next_spawn.with_ammo = std::max( std::min( spawn_info.get_int( "ammo",
                                         next_spawn.with_ammo ), 100 ), 0 );

        if( spawn_info.has_array( "items" ) ) {
            //Array of items that all spawn together (i.e. jack+tire)
            spawn_info.read( "items", next_spawn.item_ids, true );
        } else if( spawn_info.has_string( "items" ) ) {
            //Treat single item as array
            // And read the gun variant (if it exists)
            if( spawn_info.has_string( "variant" ) ) {
                const std::string variant = spawn_info.get_string( "variant" );
                next_spawn.variant_ids.emplace_back( itype_id( spawn_info.get_string( "items" ) ), variant );
            } else {
                next_spawn.item_ids.emplace_back( spawn_info.get_string( "items" ) );
            }
        }
        if( spawn_info.has_array( "item_groups" ) ) {
            //Pick from a group of items, just like map::place_items
            for( const std::string line : spawn_info.get_array( "item_groups" ) ) {
                next_spawn.item_groups.emplace_back( line );
            }
        } else if( spawn_info.has_string( "item_groups" ) ) {
            next_spawn.item_groups.emplace_back( spawn_info.get_string( "item_groups" ) );
        }
        item_spawns.push_back( std::move( next_spawn ) );
    }

    for( JsonObject jzi : jo.get_array( "zones" ) ) {
        zone_type_id zone_type( jzi.get_member( "type" ).get_string() );
        std::string name;
        std::string filter;
        point_rel_ms pt( jzi.get_member( "x" ).get_int(), jzi.get_member( "y" ).get_int() );

        if( jzi.has_string( "name" ) ) {
            name = jzi.get_string( "name" );
        }
        if( jzi.has_string( "filter" ) ) {
            filter = jzi.get_string( "filter" );
        }
        zone_defs.emplace_back( zone_def{ zone_type, name, filter, pt } );
    }
}

void vehicle_prototype::save_vehicle_as_prototype( const vehicle &veh,
        JsonOut &json )
{
    static const std::string part_location_structure( "structure" );
    json.start_object();
    // Add some notes to the exported file.
    json.member( "//1",
                 "Although this vehicle can be readily spawned in game without further tweaking," );
    json.member( "//2",
                 "this is auto-generated by program, so please check against it before proceeding." );
    json.member( "//3",
                 "Only simple top-level items can be exported by this function.  Do not rely on this for exporting comestibles, containers, etc." );
    // Begin of the vehicle.
    json.member( "id", "/TO_BE_REPLACED/" );
    json.member( "type", "vehicle" );
    json.member( "name", "/TO_BE_REPLACED/" );
    std::map<point_rel_ms, std::list<const vehicle_part *>> vp_map;
    int mount_min_y = 123;
    int mount_max_y = -123;
    // Form a map of existing real parts
    // get_all_parts() gets all non-fake parts
    // The parts are already in installation order
    for( const vpart_reference &vpr : veh.get_all_parts() ) {
        const vehicle_part &p = vpr.part();
        mount_max_y = mount_max_y < p.mount.y() ? p.mount.y() : mount_max_y;
        mount_min_y = mount_min_y > p.mount.y() ? p.mount.y() : mount_min_y;
        vp_map[p.mount].push_back( &p );
    }
    int mount_min_x = vp_map.begin()->first.x();
    int mount_max_x = vp_map.rbegin()->first.x();

    // print the vehicle's blueprint.
    json.member( "blueprint" );
    json.start_array();
    // let's start from the front of the vehicle, scanning by row.
    for( int x = mount_max_x; x >= mount_min_x; x-- ) {
        std::string row;
        for( int y = mount_min_y; y <= mount_max_y; y++ ) {
            if( veh.part_displayed_at( point_rel_ms( x, y ), false, true, true ) == -1 ) {
                row += " ";
                continue;
            }
            const vpart_display &c = veh.get_display_of_tile( point_rel_ms( x, y ), false, false, true, true );
            row += utf32_to_utf8( c.symbol );
        }
        json.write( row );
    }
    json.end_array();

    json.member( "parts" );
    json.start_array();
    // Used to flexibly print vehicle parts with possible non-default variants
    auto print_vp_with_variant = [&json]( const vehicle_part & vp ) {
        const vpart_info &vpi = vp.info();
        if( vp.variant == vpi.variant_default ) {
            json.write( vpi.id.str() );
            return;
        }
        std::string name_with_variant = vpi.id.str() + "#" + vp.variant;
        json.write( name_with_variant );
        return;
    };
    for( auto &vp_pos : vp_map ) {
        json.start_object();
        json.member( "x", vp_pos.first.x() );
        json.member( "y", vp_pos.first.y() );

        json.member( "parts" );
        json.start_array();
        for( const vehicle_part *vp : vp_pos.second ) {
            if( vp->is_tank() && vp->ammo_remaining( ) ) {
                json.start_object();
                json.member( "part" );
                print_vp_with_variant( *vp );
                json.member( "fuel", vp->ammo_current().str() );
                json.end_object();
                continue;
            } else if( vp->is_turret() && vp->ammo_remaining( ) ) {
                json.start_object();
                json.member( "part" );
                print_vp_with_variant( *vp );
                json.member( "ammo", 100 );
                json.member( "ammo_types", vp->ammo_current().str() );
                json.member( "ammo_qty" );
                json.start_array();
                int ammo_qty = vp->ammo_remaining( );
                json.write( ammo_qty );
                json.write( ammo_qty );
                json.end_array();
                json.end_object();
                continue;
            } else if( !veh.get_tools( *vp ).empty() ) {
                json.start_object();
                json.member( "part" );
                print_vp_with_variant( *vp );
                json.member( "tools" );
                json.start_array();
                for( const item &it : veh.get_tools( *vp ) ) {
                    json.write_as_string( it.typeId().str() );
                }
                json.end_array();
                json.end_object();
            } else {
                print_vp_with_variant( *vp );
            }
        }
        json.end_array();
        json.end_object();
    }
    json.end_array();

    // print top-level items ( contents are ignored )
    // For more complicated spawning of items, use itemgroup instead.
    json.member( "items" );
    json.start_array();
    for( const vpart_reference &vp : veh.get_any_parts( "CARGO" ) ) {
        if( vp.part().is_fake ) {
            continue;
        }
        const vehicle_stack &stack = veh.get_items( vp.part() );
        if( !stack.empty() ) {
            json.start_object();
            json.member( "x", vp.mount_pos().x() );
            json.member( "y", vp.mount_pos().y() );
            json.member( "chance", 100 );
            json.member( "items" );
            json.start_array();
            for( const item &it : stack ) {
                json.write( it.typeId().str() );
            }
            json.end_array();
            json.end_object();
        }
    }
    json.end_array();

    json.member( "zones" );
    json.start_array();
    for( auto const &z : veh.loot_zones ) {
        json.start_object();
        json.member( "x", z.first.x() );
        json.member( "y", z.first.y() );
        json.member( "type", z.second.get_type().str() );
        json.end_object();
    }
    json.end_array();

    json.end_object();
}

/**
 *Works through cached vehicle definitions and creates vehicle objects from them.
 */
void vehicles::finalize_prototypes()
{
    map &here = get_map(); // TODO: Determine if this is good enough.
    vehicle_prototype_factory.finalize();
    for( const vehicle_prototype &const_proto : vehicles::get_all_prototypes() ) {
        vehicle_prototype &proto = const_cast<vehicle_prototype &>( const_proto );
        std::unordered_set<point_rel_ms> cargo_spots;

        // Calling the constructor with empty vproto_id as parameter
        // makes constructor bypass copying the (non-existing) blueprint.
        proto.blueprint = make_shared_fast<vehicle>( vproto_id() );
        vehicle &blueprint = *proto.blueprint;
        blueprint.type = proto.id;
        blueprint.name = proto.name.translated();

        blueprint.suspend_refresh();
        for( vehicle_prototype::part_def &pt : proto.parts ) {
            if( const vpart_migration *migration = vpart_migration::find_migration( pt.part ) ) {
                debugmsg( "veh prototype '%s' needs fixing, part '%s' is migrated to '%s'",
                          proto.name, pt.part.str(), migration->part_id_new.str() );
                pt.part = migration->part_id_new;
            }

            if( !pt.part.is_valid() ) {
                debugmsg( "unknown vehicle part %s in %s", pt.part.c_str(), proto.id.str() );
                continue;
            }

            const int part_idx = blueprint.install_part( here, pt.pos, pt.part );
            if( part_idx < 0 ) {
                debugmsg( "init_vehicles: '%s' part '%s'(%d) can't be installed to %d,%d",
                          blueprint.name, pt.part.c_str(),
                          blueprint.part_count(), pt.pos.x(), pt.pos.y() );
            } else {
                vehicle_part &vp = blueprint.part( part_idx );
                const vpart_info &vpi = vp.info();

                // try variant, if variant invalid default to first one
                cata_assert( !vpi.variants.empty() );
                const auto variant_it = vpi.variants.find( pt.variant );
                if( variant_it == vpi.variants.end() ) {
                    vp.variant = vpi.variants.begin()->first;
                    if( !pt.variant.empty() ) { // only debugmsg if trying to select non-default variant
                        debugmsg( "veh prototype '%s' uses invalid variant '%s' for "
                                  "part '%s' defaulting to '%s'",
                                  proto.name, pt.variant, vp.info().id.str(), vp.variant );
                    }
                } else {
                    vp.variant = pt.variant;
                }

                for( const itype_id &it : pt.tools ) {
                    blueprint.get_tools( vp ).emplace_back( it, calendar::turn );
                }
                if( pt.fuel ) {
                    vp.ammo_set( pt.fuel,
                                 pt.fuel->ammo ? vp.ammo_capacity( pt.fuel->ammo->type ) : vp.item_capacity( pt.fuel ) );
                }
            }

            std::vector<itype_id> migrated;
            for( auto it = pt.ammo_types.begin(); it != pt.ammo_types.end(); ) {
                if( item_controller->migrate_id( *it ) != *it ) {
                    migrated.push_back( item_controller->migrate_id( *it ) );
                    it = pt.ammo_types.erase( it );
                } else {
                    ++it;
                }
            }

            for( const itype_id &migrant : migrated ) {
                pt.ammo_types.insert( migrant );
            }

            const itype *base = item::find_type( pt.part->base_item );
            if( !base->gun ) {
                if( pt.with_ammo ) {
                    debugmsg( "init_vehicles: non-turret %s with ammo in %s", pt.part.c_str(),
                              proto.id.str() );
                }
                if( !pt.ammo_types.empty() ) {
                    debugmsg( "init_vehicles: non-turret %s with ammo_types in %s",
                              pt.part.c_str(), proto.id.str() );
                }
                if( pt.ammo_qty.first > 0 || pt.ammo_qty.second > 0 ) {
                    debugmsg( "init_vehicles: non-turret %s with ammo_qty in %s",
                              pt.part.c_str(), proto.id.str() );
                }

            } else {
                for( const auto &e : pt.ammo_types ) {
                    const itype *ammo = item::find_type( e );
                    if( !ammo->ammo || !base->gun->ammo.count( ammo->ammo->type ) ) {
                        debugmsg( "init_vehicles: turret %s has invalid ammo_type %s in %s",
                                  pt.part.c_str(), e.c_str(), proto.id.str() );
                    }
                }
                if( pt.ammo_types.empty() && !base->gun->ammo.empty() ) {
                    pt.ammo_types.insert( ammotype( *base->gun->ammo.begin() )->default_ammotype() );
                }
            }

            if( item( &*base ).can_contain( item( pt.fuel ) ).success() || base->magazine ) {
                if( !item::type_is_defined( pt.fuel ) ) {
                    debugmsg( "init_vehicles: tank %s specified invalid fuel in %s",
                              pt.part.c_str(), proto.id.str() );
                }
            } else {
                if( !pt.fuel.is_null() ) {
                    debugmsg( "init_vehicles: non-fuel store part %s with fuel in %s",
                              pt.part.c_str(), proto.id.str() );
                }
            }

            if( pt.part.obj().has_flag( VPFLAG_CARGO ) ) {
                cargo_spots.insert( pt.pos );
            }
        }
        blueprint.enable_refresh();

        for( vehicle_item_spawn &i : proto.item_spawns ) {
            if( cargo_spots.count( i.pos ) == 0 ) {
                debugmsg( "Invalid spawn location (no CARGO vpart) in %s (%d, %d): %d%%",
                          proto.name, i.pos.x(), i.pos.y(), i.chance );
            }
            for( auto &j : i.item_ids ) {
                if( !item::type_is_defined( j ) ) {
                    debugmsg( "unknown item %s in spawn list of %s", j.c_str(), proto.id.str() );
                }
            }
            for( auto &j : i.item_groups ) {
                if( !item_group::group_is_defined( j ) ) {
                    debugmsg( "unknown item group %s in spawn list of %s", j.c_str(), proto.id.str() );
                }
            }
        }
    }
}

static std::vector<vpart_category> vpart_categories_all;

const std::vector<vpart_category> &vpart_category::all()
{
    return vpart_categories_all;
}

void vpart_category::load( const JsonObject &jo )
{
    vpart_category def;

    assign( jo, "id", def.id_ );
    assign( jo, "name", def.name_ );
    assign( jo, "short_name", def.short_name_ );
    assign( jo, "priority", def.priority_ );

    vpart_categories_all.push_back( def );
}

void vpart_category::finalize()
{
    std::sort( vpart_categories_all.begin(), vpart_categories_all.end() );
}

void vpart_category::reset()
{
    vpart_categories_all.clear();
}

void vpart_migration::load( const JsonObject &jo )
{
    vpart_migration migration;
    mandatory( jo, false, "from", migration.part_id_old );
    mandatory( jo, false, "to", migration.part_id_new );
    optional( jo, false, "variant", migration.variant );
    optional( jo, false, "add_veh_tools", migration.add_veh_tools );
    vpart_migrations.emplace( migration.part_id_old, migration );
}

void vpart_migration::reset()
{
    vpart_migrations.clear();
}

void vpart_migration::finalize()
{
    for( const auto &[from_id, migration] : vpart_migrations ) {
        if( !migration.add_veh_tools.empty() &&
            !migration.part_id_new->has_flag( "VEH_TOOLS" ) ) {
            debugmsg( "vpart migration from id '%s' specified 'add_veh_tools' but target id '%s' is not a VEH_TOOLS part",
                      from_id.str(), migration.part_id_new.str() );
        }
    }
}

void vpart_migration::check()
{
    for( const auto &[from_id, _] : vpart_migrations ) {
        const vpart_migration &vm = *find_migration( from_id );
        if( !vm.part_id_new.is_valid() ) {
            debugmsg( "vpart migration specifies invalid id '%s'", vm.part_id_new.str() );
            continue;
        }
        if( !vm.add_veh_tools.empty() && !vm.part_id_new->has_flag( "VEH_TOOLS" ) ) {
            debugmsg( "vpart migration from id '%s' specified 'add_veh_tools' but target id '%s' is not a VEH_TOOLS part",
                      from_id.str(), vm.part_id_new.str() );
        }
        if( vm.variant && vm.part_id_new->variants.count( vm.variant.value() ) <= 0 ) {
            debugmsg( "vpart migration chain from id '%s' to '%s' specifies invalid variant '%s'",
                      from_id.str(), vm.part_id_new.str(), vm.variant.value() );
        }
    }
}

const vpart_migration *vpart_migration::find_migration( const vpart_id &original )
{
    vpart_migration *migration = nullptr;
    vpart_id pid = original;

    // limit up to 10 migrations per vpart (guard in case of accidental loops)
    for( int i = 0; i < 10; i++ ) {
        const auto &migration_it = vpart_migrations.find( pid );
        if( migration_it == vpart_migrations.cend() ) {
            break;
        }
        pid = migration_it->second.part_id_new;
        migration = &migration_it->second;
    }

    return migration;
}
