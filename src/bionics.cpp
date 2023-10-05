#include "bionics.h"

#include <algorithm> //std::min
#include <climits>
#include <cmath>
#include <cstdlib>
#include <forward_list>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "action.h"
#include "activity_actor_definitions.h"
#include "activity_type.h"
#include "assign.h"
#include "avatar.h"
#include "avatar_action.h"
#include "ballistics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "character_martial_arts.h"
#include "colony.h"
#include "color.h"
#include "damage.h"
#include "debug.h"
#include "dispersion.h"
#include "effect.h"
#include "effect_on_condition.h"
#include "enum_conversions.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "explosion.h"
#include "field_type.h"
#include "flag.h"
#include "game.h"
#include "generic_factory.h"
#include "handle_liquid.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "json.h"
#include "line.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "material.h"
#include "memorial_logger.h"
#include "messages.h"
#include "monster.h"
#include "morale_types.h"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player_activity.h"
#include "point.h"
#include "projectile.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "teleport.h"
#include "translations.h"
#include "ui.h"
#include "units.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "viewer.h"
#include "vpart_position.h"
#include "weather.h"
#include "weather_gen.h"

static const activity_id ACT_OPERATION( "ACT_OPERATION" );

static const bionic_id afs_bio_dopamine_stimulators( "afs_bio_dopamine_stimulators" );
static const bionic_id bio_adrenaline( "bio_adrenaline" );
static const bionic_id bio_blood_anal( "bio_blood_anal" );
static const bionic_id bio_blood_filter( "bio_blood_filter" );
static const bionic_id bio_cqb( "bio_cqb" );
static const bionic_id bio_emp( "bio_emp" );
static const bionic_id bio_evap( "bio_evap" );
static const bionic_id bio_flashbang( "bio_flashbang" );
static const bionic_id bio_geiger( "bio_geiger" );
static const bionic_id bio_gills( "bio_gills" );
static const bionic_id bio_hydraulics( "bio_hydraulics" );
static const bionic_id bio_jointservo( "bio_jointservo" );
static const bionic_id bio_lighter( "bio_lighter" );
static const bionic_id bio_lockpick( "bio_lockpick" );
static const bionic_id bio_magnet( "bio_magnet" );
static const bionic_id bio_meteorologist( "bio_meteorologist" );
static const bionic_id bio_nanobots( "bio_nanobots" );
static const bionic_id bio_painkiller( "bio_painkiller" );
static const bionic_id bio_radscrubber( "bio_radscrubber" );
static const bionic_id bio_remote( "bio_remote" );
static const bionic_id bio_resonator( "bio_resonator" );
static const bionic_id bio_shockwave( "bio_shockwave" );
static const bionic_id bio_teleport( "bio_teleport" );
static const bionic_id bio_time_freeze( "bio_time_freeze" );
static const bionic_id bio_torsionratchet( "bio_torsionratchet" );
static const bionic_id bio_water_extractor( "bio_water_extractor" );

static const efftype_id effect_adrenaline( "adrenaline" );
static const efftype_id effect_antifungal( "antifungal" );
static const efftype_id effect_assisted( "assisted" );
static const efftype_id effect_asthma( "asthma" );
static const efftype_id effect_badpoison( "badpoison" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_bloodworms( "bloodworms" );
static const efftype_id effect_cig( "cig" );
static const efftype_id effect_datura( "datura" );
static const efftype_id effect_dermatik( "dermatik" );
static const efftype_id effect_drunk( "drunk" );
static const efftype_id effect_fungus( "fungus" );
static const efftype_id effect_hallu( "hallu" );
static const efftype_id effect_heating_bionic( "heating_bionic" );
static const efftype_id effect_high( "high" );
static const efftype_id effect_iodine( "iodine" );
static const efftype_id effect_meth( "meth" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_operating( "operating" );
static const efftype_id effect_paralyzepoison( "paralyzepoison" );
static const efftype_id effect_pblue( "pblue" );
static const efftype_id effect_pkill1( "pkill1" );
static const efftype_id effect_pkill2( "pkill2" );
static const efftype_id effect_pkill3( "pkill3" );
static const efftype_id effect_pkill_l( "pkill_l" );
static const efftype_id effect_poison( "poison" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_teleglow( "teleglow" );
static const efftype_id effect_tetanus( "tetanus" );
static const efftype_id effect_took_flumed( "took_flumed" );
static const efftype_id effect_took_prozac( "took_prozac" );
static const efftype_id effect_took_prozac_bad( "took_prozac_bad" );
static const efftype_id effect_took_xanax( "took_xanax" );
static const efftype_id effect_under_operation( "under_operation" );
static const efftype_id effect_venom_dmg( "venom_dmg" );
static const efftype_id effect_venom_weaken( "venom_weaken" );
static const efftype_id effect_visuals( "visuals" );

static const fault_id fault_bionic_salvaged( "fault_bionic_salvaged" );

static const itype_id itype_anesthetic( "anesthetic" );
static const itype_id itype_radiocontrol( "radiocontrol" );
static const itype_id itype_remotevehcontrol( "remotevehcontrol" );
static const itype_id itype_water_clean( "water_clean" );

static const json_character_flag json_flag_BIONIC_GUN( "BIONIC_GUN" );
static const json_character_flag json_flag_BIONIC_NPC_USABLE( "BIONIC_NPC_USABLE" );
static const json_character_flag json_flag_BIONIC_POWER_SOURCE( "BIONIC_POWER_SOURCE" );
static const json_character_flag json_flag_BIONIC_TOGGLED( "BIONIC_TOGGLED" );
static const json_character_flag json_flag_BIONIC_WEAPON( "BIONIC_WEAPON" );
static const json_character_flag json_flag_ENHANCED_VISION( "ENHANCED_VISION" );
static const json_character_flag json_flag_PAIN_IMMUNE( "PAIN_IMMUNE" );

static const material_id fuel_type_metabolism( "metabolism" );
static const material_id fuel_type_sun_light( "sunlight" );
static const material_id fuel_type_wind( "wind" );

static const material_id material_budget_steel( "budget_steel" );
static const material_id material_ch_steel( "ch_steel" );
static const material_id material_hc_steel( "hc_steel" );
static const material_id material_iron( "iron" );
static const material_id material_lc_steel( "lc_steel" );
static const material_id material_mc_steel( "mc_steel" );
static const material_id material_muscle( "muscle" );
static const material_id material_qt_steel( "qt_steel" );
static const material_id material_steel( "steel" );

static const requirement_id requirement_data_anesthetic( "anesthetic" );

static const skill_id skill_computer( "computer" );
static const skill_id skill_electronics( "electronics" );
static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_mechanics( "mechanics" );

static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_DEBUG_BIONICS( "DEBUG_BIONICS" );
static const trait_id trait_MASOCHIST_MED( "MASOCHIST_MED" );
static const trait_id trait_NONE( "NONE" );
static const trait_id trait_PROF_AUTODOC( "PROF_AUTODOC" );
static const trait_id trait_PROF_MED( "PROF_MED" );
static const trait_id trait_PYROMANIA( "PYROMANIA" );
static const trait_id trait_THRESH_MEDICAL( "THRESH_MEDICAL" );

struct Character::bionic_fuels {
    std::vector<vehicle *> connected_vehicles;
    std::vector<item *> connected_solar;
    std::vector<item *> connected_fuel;
    bool can_be_on = true;
};

namespace
{
generic_factory<bionic_data> bionic_factory( "bionic" );
std::vector<bionic_id> faulty_bionics;
} //namespace

void bionic::initialize_pseudo_items( bool create_weapon )
{
    bionic_data bid( info() );

    bool inherit_use_bionic_power = bid.has_flag( flag_USES_BIONIC_POWER );
    toggled_pseudo_items.clear();
    passive_pseudo_items.clear();

    if( bid.has_flag( json_flag_BIONIC_GUN ) || bid.has_flag( json_flag_BIONIC_WEAPON ) ) {
        if( create_weapon && !id->fake_weapon.is_empty() && id->fake_weapon.is_valid() ) {
            item new_weapon = item( id->fake_weapon );
            install_weapon( new_weapon, true );
        } else {
            update_weapon_flags();
        }
    } else if( bid.has_flag( json_flag_BIONIC_TOGGLED ) ) {
        for( const itype_id &id : bid.toggled_pseudo_items ) {
            if( !id.is_empty() && id.is_valid() ) {
                toggled_pseudo_items.emplace_back( id );
            }
        }
    }
    //integrated armor uses pseudo item structure to be convenient but these should be visible in inventory
    for( const itype_id &id : bid.passive_pseudo_items ) {
        if( !id.is_empty() && id.is_valid() ) {
            item pseudo( id );
            if( !pseudo.has_flag( flag_INTEGRATED ) ) {
                pseudo.set_flag( flag_PSEUDO );
            }
            passive_pseudo_items.emplace_back( pseudo );
        }
    }

    if( inherit_use_bionic_power ) {
        for( item &pseudo : passive_pseudo_items ) {
            pseudo.set_flag( flag_USES_BIONIC_POWER );
        }

        for( item &pseudo : toggled_pseudo_items ) {
            pseudo.set_flag( flag_USES_BIONIC_POWER );
        }
    }
}

void bionic::update_weapon_flags()
{
    if( has_weapon() ) {
        if( id->has_flag( flag_USES_BIONIC_POWER ) ) {
            weapon.set_flag( flag_USES_BIONIC_POWER );
        }
        weapon.set_flag( flag_NO_UNWIELD );
    }
}

/** @relates string_id */
template<>
const bionic_data &string_id<bionic_data>::obj() const
{
    return bionic_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<bionic_data>::is_valid() const
{
    return bionic_factory.is_valid( *this );
}

std::vector<bodypart_id> get_occupied_bodyparts( const bionic_id &bid )
{
    std::vector<bodypart_id> parts;
    for( const std::pair<const string_id<body_part_type>, size_t> &element : bid->occupied_bodyparts ) {
        if( element.second > 0 ) {
            parts.push_back( element.first.id() );
        }
    }
    return parts;
}

bool bionic_data::has_flag( const json_character_flag &flag ) const
{
    return flags.count( flag ) > 0;
}

bool bionic_data::has_active_flag( const json_character_flag &flag ) const
{
    return active_flags.count( flag ) > 0;
}

bool bionic_data::has_inactive_flag( const json_character_flag &flag ) const
{
    return inactive_flags.count( flag ) > 0;
}

itype_id bionic_data::itype() const
{
    // For now we just assume that the bionic id matches the corresponding item
    // id (as strings).
    return itype_id( id.str() );
}

static social_modifiers load_bionic_social_mods( const JsonObject &jo )
{
    social_modifiers ret;
    jo.read( "lie", ret.lie );
    jo.read( "persuade", ret.persuade );
    jo.read( "intimidate", ret.intimidate );
    return ret;
}

void bionic_data::load( const JsonObject &jsobj, const std::string &src )
{

    mandatory( jsobj, was_loaded, "id", id );
    mandatory( jsobj, was_loaded, "name", name );
    mandatory( jsobj, was_loaded, "description", description );

    optional( jsobj, was_loaded, "cant_remove_reason", cant_remove_reason );
    // uses assign because optional doesn't handle loading units as strings
    assign( jsobj, "react_cost", power_over_time, false, 0_kJ );
    assign( jsobj, "capacity", capacity, false );
    assign( jsobj, "weight_capacity_bonus", weight_capacity_bonus, false );
    assign( jsobj, "act_cost", power_activate, false, 0_kJ );
    assign( jsobj, "deact_cost", power_deactivate, false, 0_kJ );
    assign( jsobj, "trigger_cost", power_trigger, false, 0_kJ );
    assign( jsobj, "power_trickle", power_trickle, false, 0_kJ );

    optional( jsobj, was_loaded, "time", charge_time, 0_turns );

    optional( jsobj, was_loaded, "flags", flags );
    optional( jsobj, was_loaded, "active_flags", active_flags );
    optional( jsobj, was_loaded, "inactive_flags", inactive_flags );

    optional( jsobj, was_loaded, "fuel_efficiency", fuel_efficiency, 0 );
    optional( jsobj, was_loaded, "passive_fuel_efficiency", passive_fuel_efficiency, 0 );

    optional( jsobj, was_loaded, "passive_pseudo_items", passive_pseudo_items );
    optional( jsobj, was_loaded, "toggled_pseudo_items", toggled_pseudo_items );
    optional( jsobj, was_loaded, "fake_weapon", fake_weapon, itype_id() );
    optional( jsobj, was_loaded, "installable_weapon_flags", installable_weapon_flags );

    optional( jsobj, was_loaded, "spell_on_activation", spell_on_activate );

    optional( jsobj, was_loaded, "weight_capacity_modifier", weight_capacity_modifier, 1.0f );
    optional( jsobj, was_loaded, "exothermic_power_gen", exothermic_power_gen );
    optional( jsobj, was_loaded, "power_gen_emission", power_gen_emission );
    optional( jsobj, was_loaded, "coverage_power_gen_penalty", coverage_power_gen_penalty );
    optional( jsobj, was_loaded, "is_remote_fueled", is_remote_fueled );

    optional( jsobj, was_loaded, "known_ma_styles", ma_styles );
    optional( jsobj, was_loaded, "learned_spells", learned_spells );
    optional( jsobj, was_loaded, "learned_proficiencies", proficiencies );
    optional( jsobj, was_loaded, "canceled_mutations", canceled_mutations );
    optional( jsobj, was_loaded, "mutation_conflicts", mutation_conflicts );
    optional( jsobj, was_loaded, "give_mut_on_removal", give_mut_on_removal );
    optional( jsobj, was_loaded, "included_bionics", included_bionics );
    optional( jsobj, was_loaded, "included", included );
    optional( jsobj, was_loaded, "upgraded_bionic", upgraded_bionic );
    optional( jsobj, was_loaded, "required_bionic", required_bionic );
    optional( jsobj, was_loaded, "fuel_options", fuel_opts );
    optional( jsobj, was_loaded, "activated_on_install", activated_on_install, false );

    optional( jsobj, was_loaded, "available_upgrades", available_upgrades );

    optional( jsobj, was_loaded, "installation_requirement", installation_requirement );

    optional( jsobj, was_loaded, "vitamin_absorb_mod", vitamin_absorb_mod, 1.0f );

    optional( jsobj, was_loaded, "dupes_allowed", dupes_allowed, false );

    optional( jsobj, was_loaded, "auto_deactivates", autodeactivated_bionics );

    optional( jsobj, was_loaded, "activated_close_ui", activated_close_ui, false );

    optional( jsobj, was_loaded, "deactivated_close_ui", deactivated_close_ui, false );

    for( JsonValue jv : jsobj.get_array( "activated_eocs" ) ) {
        activated_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }

    for( JsonValue jv : jsobj.get_array( "processed_eocs" ) ) {
        processed_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }

    for( JsonValue jv : jsobj.get_array( "deactivated_eocs" ) ) {
        deactivated_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }

    int enchant_num = 0;
    for( JsonValue jv : jsobj.get_array( "enchantments" ) ) {
        std::string enchant_name = "INLINE_ENCH_" + name + "_" + std::to_string( enchant_num++ );
        enchantments.push_back( enchantment::load_inline_enchantment( jv, src, enchant_name ) );
    }

    if( jsobj.has_array( "stat_bonus" ) ) {
        // clear data first so that copy-from can override it
        stat_bonus.clear();
        for( JsonArray ja : jsobj.get_array( "stat_bonus" ) ) {
            stat_bonus.emplace( io::string_to_enum<character_stat>( ja.get_string( 0 ) ),
                                ja.get_int( 1 ) );
        }
    }
    if( jsobj.has_array( "encumbrance" ) ) {
        // clear data first so that copy-from can override it
        encumbrance.clear();
        for( JsonArray ja : jsobj.get_array( "encumbrance" ) ) {
            encumbrance.emplace( bodypart_str_id( ja.get_string( 0 ) ),
                                 ja.get_int( 1 ) );
        }
    }
    if( jsobj.has_array( "occupied_bodyparts" ) ) {
        // clear data first so that copy-from can override it
        occupied_bodyparts.clear();
        for( JsonArray ja : jsobj.get_array( "occupied_bodyparts" ) ) {
            occupied_bodyparts.emplace( bodypart_str_id( ja.get_string( 0 ) ),
                                        ja.get_int( 1 ) );
        }
    }
    if( jsobj.has_array( "env_protec" ) ) {
        // clear data first so that copy-from can override it
        env_protec.clear();
        for( JsonArray ja : jsobj.get_array( "env_protec" ) ) {
            env_protec.emplace( bodypart_str_id( ja.get_string( 0 ) ), ja.get_int( 1 ) );
        }
    }
    if( jsobj.has_array( "protec" ) ) {
        protec.clear();
        for( JsonArray ja : jsobj.get_array( "protec" ) ) {
            protec.emplace( bodypart_str_id( ja.get_string( 0 ) ),
                            load_resistances_instance( ja.get_object( 1 ) ) );
        }
    }
    if( jsobj.has_object( "social_modifiers" ) ) {
        JsonObject sm = jsobj.get_object( "social_modifiers" );
        social_mods = load_bionic_social_mods( sm );
    }

    activated = has_flag( STATIC( json_character_flag( json_flag_BIONIC_TOGGLED ) ) ) ||
                has_flag( json_flag_BIONIC_GUN ) ||
                power_activate > 0_kJ ||
                spell_on_activate ||
                charge_time > 0_turns;

    if( has_flag( STATIC( json_character_flag( "BIONIC_FAULTY" ) ) ) ) {
        faulty_bionics.push_back( id );
    }

    if( has_flag( json_flag_BIONIC_GUN ) && power_activate != 0_kJ ) {
        debugmsg( "Bionic %s attribute power_activate is ignored due to BIONIC_GUN flag.", id.c_str() );
    }
}

void bionic_data::finalize()
{
    for( auto &p : protec ) {
        finalize_damage_map( p.second.resist_vals );
    }
}

void bionic_data::load_bionic( const JsonObject &jo, const std::string &src )
{
    bionic_factory.load( jo, src );
}

std::map<bionic_id, bionic_id> bionic_data::migrations;

void bionic_data::load_bionic_migration( const JsonObject &jo, const std::string_view )
{
    const bionic_id from( jo.get_string( "from" ) );
    const bionic_id to = jo.has_string( "to" )
                         ? bionic_id( jo.get_string( "to" ) )
                         : bionic_id::NULL_ID();
    migrations.emplace( from, to );
}

void bionic_data::finalize_bionic()
{
    bionic_factory.finalize();
}

const std::vector<bionic_data> &bionic_data::get_all()
{
    return bionic_factory.get_all();
}

void bionic_data::check_bionic_consistency()
{
    for( const bionic_data &bio : get_all() ) {

        if( !bio.installation_requirement.is_empty() && !bio.installation_requirement.is_valid() ) {
            debugmsg( "Bionic %s uses undefined requirement_id %s", bio.id.c_str(),
                      bio.installation_requirement.c_str() );
        }
        if( bio.has_flag( json_flag_BIONIC_GUN ) && bio.has_flag( json_flag_BIONIC_WEAPON ) ) {
            debugmsg( "Bionic %s specified as both gun and weapon bionic", bio.id.c_str() );
        }
        if( ( bio.has_flag( json_flag_BIONIC_GUN ) || bio.has_flag( json_flag_BIONIC_WEAPON ) ) &&
            bio.fake_weapon.is_empty() ) {
            debugmsg( "Bionic %s specified as gun or weapon bionic is missing its fake_weapon id",
                      bio.id.c_str() );
        }
        if( bio.has_flag( json_flag_BIONIC_WEAPON ) && !bio.has_flag( flag_BIONIC_TOGGLED ) ) {
            debugmsg( "Bionic \"%s\" specified as weapon bionic is not flagged as \"BIONIC_TOGGLED\"",
                      bio.id.c_str() );
        }

        for( const itype_id &pseudo : bio.passive_pseudo_items ) {
            if( pseudo.is_empty() ) {
                debugmsg( "Bionic \"%s\" has an empty passive_pseudo_item",
                          bio.id.c_str() );
            } else if( !item::type_is_defined( pseudo ) ) {
                debugmsg( "Bionic \"%s\" has unknown passive_pseudo_item \"%s\"",
                          bio.id.c_str(), pseudo.c_str() );
            }
        }

        for( const itype_id &pseudo : bio.toggled_pseudo_items ) {
            if( pseudo.is_empty() ) {
                debugmsg( "Bionic %s has an empty toggled_pseudo_item",
                          bio.id.c_str() );
            } else if( !item::type_is_defined( pseudo ) ) {
                debugmsg( "Bionic \"%s\" has unknown toggled_pseudo_item \"%s\"",
                          bio.id.c_str(), pseudo.c_str() );
            }
        }
        for( const trait_id &mid : bio.mutation_conflicts ) {
            if( !mid.is_valid() ) {
                debugmsg( "Bionic %s conflicts with undefined mutation %s", bio.id.c_str(), mid.c_str() );
            }
        }
        for( const trait_id &mid : bio.canceled_mutations ) {
            if( !mid.is_valid() ) {
                debugmsg( "Bionic %s cancels undefined mutation %s", bio.id.c_str(), mid.c_str() );
            }
        }
        for( const enchantment_id &eid : bio.id->enchantments ) {
            if( !eid.is_valid() ) {
                debugmsg( "Bionic %s uses undefined enchantment %s", bio.id.c_str(), eid.c_str() );
            }
        }
        for( const bionic_id &bid : bio.included_bionics ) {
            if( !bid.is_valid() ) {
                debugmsg( "Bionic %s includes undefined bionic %s",
                          bio.id.c_str(), bid.c_str() );
            }
            if( !bid->occupied_bodyparts.empty() ) {
                debugmsg( "Bionic %s (included by %s) consumes slots, those should be part of the containing bionic instead.",
                          bid.c_str(), bio.id.c_str() );
            }
        }
        for( const bionic_id &bid : bio.autodeactivated_bionics ) {
            if( !bid.is_valid() ) {
                debugmsg( "Bionic \"%s\" has auto_deactivated bionic with invalid id \"%s\"", bio.id.c_str(),
                          bid.c_str() );
            }
        }
        if( bio.upgraded_bionic ) {
            if( bio.upgraded_bionic == bio.id ) {
                debugmsg( "Bionic %s is upgraded with itself", bio.id.c_str() );
            } else if( !bio.upgraded_bionic.is_valid() ) {
                debugmsg( "Bionic %s upgrades undefined bionic %s",
                          bio.id.c_str(), bio.upgraded_bionic.c_str() );
            }
        }
        if( bio.required_bionic ) {
            if( bio.required_bionic == bio.id ) {
                debugmsg( "Bionic %s requires itself as a prerequisite for installation", bio.id.c_str() );
            } else if( !bio.required_bionic.is_valid() ) {
                debugmsg( "Bionic %s requires undefined bionic %s",
                          bio.id.c_str(), bio.required_bionic.c_str() );
            }
        }
        if( !item::type_is_defined( bio.itype() ) && !bio.included ) {
            debugmsg( "Bionic %s has no defined item version", bio.id.c_str() );
        }
        for( const std::pair<const bodypart_str_id, resistances> &bprot : bio.protec ) {
            for( const std::pair<const damage_type_id, float> &dt : bprot.second.resist_vals ) {
                if( !dt.first.is_valid() ) {
                    debugmsg( "Invalid protection type \"%s\" for bionic %s", dt.first.c_str(), bio.id.c_str() );
                }
            }
        }
    }
}

bionic_data::bionic_data() : name( no_translation( "bad bionic" ) ),
    description( no_translation( "This bionic was not set up correctly, this is a bug" ) )
{
}

static void force_comedown( effect &eff )
{
    if( eff.is_null() || eff.get_effect_type() == nullptr || eff.get_duration() <= 1_turns ) {
        return;
    }

    eff.set_duration( std::min( eff.get_duration(), eff.get_int_dur_factor() ) );
}

void npc::discharge_cbm_weapon()
{
    if( !is_using_bionic_weapon() ) {
        return;
    }

    std::optional<bionic *> bio_opt = find_bionic_by_uid( get_weapon_bionic_uid() );
    if( !bio_opt ) {
        debugmsg( "NPC tried to use a non-existent gun bionic with UID %d", weapon_bionic_uid );
        return;
    }
    bionic &bio = **bio_opt;

    mod_power_level( -bio.info().power_activate );

    set_wielded_item( real_weapon );
    weapon_bionic_uid = 0;
}

void npc::check_or_use_weapon_cbm( const bionic_id &cbm_id )
{
    // if we're already using a bio_weapon, keep using it
    if( is_using_bionic_weapon() ) {
        return;
    }
    const float allowed_ratio = static_cast<int>( rules.cbm_reserve ) / 100.0f;
    const units::energy free_power = get_power_level() - get_max_power_level() * allowed_ratio;
    if( free_power <= 0_mJ ) {
        return;
    }

    int index = 0;
    bool found = false;
    for( bionic &i : *my_bionics ) {
        if( i.id == cbm_id && !i.powered ) {
            found = true;
            break;
        }
        index += 1;
    }
    if( !found ) {
        return;
    }
    bionic &bio = ( *my_bionics )[index];

    item_location weapon = get_wielded_item();
    item weap = weapon ? *weapon : item();
    if( bio.info().has_flag( json_flag_BIONIC_GUN ) ) {
        if( !bio.has_weapon() ) {
            debugmsg( "NPC tried to activate gun bionic \"%s\" without fake_weapon",
                      bio.info().id.str() );
            return;
        }

        const item cbm_weapon = bio.get_weapon();
        bool not_allowed = !rules.has_flag( ally_rule::use_guns ) ||
                           ( rules.has_flag( ally_rule::use_silent ) && !cbm_weapon.is_silent() );
        if( is_player_ally() && not_allowed ) {
            return;
        }

        int ammo_count = weap.ammo_remaining( this );
        const units::energy ups_drain =  weap.get_gun_ups_drain();
        if( ups_drain > 0_kJ ) {
            ammo_count = units::from_kilojoule( ammo_count ) / ups_drain;
        }
        const int cbm_ammo = free_power /  bio.info().power_activate;

        if( weapon_value( weap, ammo_count ) < weapon_value( cbm_weapon, cbm_ammo ) ) {
            if( real_weapon.is_null() ) {
                // Prevent replacing real weapon when migrating saves
                real_weapon = weap;
            }
            set_wielded_item( cbm_weapon );
            weapon_bionic_uid = bio.get_uid();
        }
    } else if( bio.info().has_flag( json_flag_BIONIC_WEAPON ) && !weap.has_flag( flag_NO_UNWIELD ) &&
               free_power > bio.info().power_activate ) {

        if( !bio.has_weapon() ) {
            debugmsg( "NPC tried to activate weapon bionic \"%s\" without fake_weapon",
                      bio.info().id.str() );
            return;
        }

        if( is_armed() ) {
            stow_item( *weapon );
        }
        add_msg_if_player_sees( pos(), m_info, _( "%s activates their %s." ),
                                disp_name(), bio.info().name );

        set_wielded_item( bio.get_weapon() );
        mod_power_level( -bio.info().power_activate );
        bio.powered = true;
        weapon_bionic_uid = bio.get_uid();
    }
}

// Why put this in a Big Switch?  Why not let bionics have pointers to
// functions, much like monsters and items?
//
// Well, because like diseases, which are also in a Big Switch, bionics don't
// share functions....
bool Character::activate_bionic( bionic &bio, bool eff_only, bool *close_bionics_ui )
{
    const bool mounted = is_mounted();
    if( bio.incapacitated_time > 0_turns ) {
        add_msg( m_info, _( "Your %s is shorting out and can't be activated." ),
                 bio.info().name );
        return false;
    }

    // eff_only means only do the effect without messing with stats or displaying messages
    if( !eff_only ) {
        if( bio.powered ) {
            // It's already on!
            return false;
        }
        if( !enough_power_for( bio.id ) ) {
            add_msg_if_player( m_info, _( "You don't have the power to activate your %s." ),
                               bio.info().name );
            return false;
        }

        const bionic_fuels result = bionic_fuel_check( bio, true );
        if( !result.can_be_on ) {
            return false;
        }

        if( !bio.activate_spell( *this ) ) {
            // the spell this bionic uses was unable to be cast
            return false;
        }

        // We can actually activate now, do activation-y things
        mod_power_level( -bio.info().power_activate );

        bio.powered = bio.info().has_flag( json_flag_BIONIC_TOGGLED ) || bio.info().charge_time > 0_turns;

        if( bio.info().charge_time > 0_turns ) {
            bio.charge_timer = bio.info().charge_time;
        }
        if( !bio.id->enchantments.empty() ) {
            recalculate_enchantment_cache();
        }
    }

    auto add_msg_activate = [&]() {
        if( !eff_only ) {
            add_msg_if_player( m_info, _( "You activate your %s." ), bio.info().name );
        }
    };
    auto refund_power = [&]() {
        if( !eff_only ) {
            mod_power_level( bio.info().power_activate );
        }
    };

    for( const effect_on_condition_id &eoc : bio.id->activated_eocs ) {
        dialogue d( get_talker_for( *this ), nullptr );
        if( eoc->type == eoc_type::ACTIVATION ) {
            eoc->activate( d );
        } else {
            debugmsg( "Must use an activation eoc for a bionic activation.  If you don't want the effect_on_condition to happen on its own (without the bionic being activated), remove the recurrence min and max.  Otherwise, create a non-recurring effect_on_condition for this bionic with its condition and effects, then have a recurring one queue it." );
        }
    }

    item tmp_item;
    avatar &player_character = get_avatar();
    map &here = get_map();
    // On activation effects go here
    if( bio.info().has_flag( json_flag_BIONIC_GUN ) ) {
        if( !bio.has_weapon() ) {
            debugmsg( "tried to activate gun bionic \"%s\" without fake_weapon",
                      bio.info().id.str() );
            return false;
        }
        if( bio.get_weapon().get_gun_bionic_drain() > get_power_level() ) {
            add_msg_if_player( m_info, _( "You need at least %s of bionic power to fire the %s!" ),
                               units::display( bio.get_weapon().get_gun_bionic_drain() ), bio.get_weapon().tname() );
            return false;
        }

        add_msg_activate();
        if( close_bionics_ui ) {
            *close_bionics_ui = true;
        }
        avatar_action::fire_ranged_bionic( player_character, bio.get_weapon() );
    } else if( bio.info().has_flag( json_flag_BIONIC_WEAPON ) ) {
        if( !bio.has_weapon() ) {
            debugmsg( "tried to activate weapon bionic \"%s\" without fake_weapon",
                      bio.info().id.str() );
            refund_power();
            bio.powered = false;
            return false;
        }

        if( weapon.has_flag( flag_NO_UNWIELD ) ) {
            if( get_weapon_bionic_uid() ) {
                if( std::optional<bionic *> bio_opt = find_bionic_by_uid( get_weapon_bionic_uid() ) ) {
                    if( deactivate_bionic( **bio_opt, eff_only ) ) {
                        // restore state and try again
                        refund_power();
                        bio.powered = false;
                        // note: deep recursion is not possible, as `deactivate_bionic` won't return true second time
                        return activate_bionic( bio, eff_only, close_bionics_ui );
                    }
                } else {
                    debugmsg( "Can't find currently activated weapon bionic with UID %d", get_weapon_bionic_uid() );
                    weapon_bionic_uid = 0;
                    set_wielded_item( item() );
                    refund_power();
                    bio.powered = false;
                    return false;
                }
            }

            add_msg_if_player( m_info, _( "Deactivate your %s first!" ), weapon.tname() );
            refund_power();
            bio.powered = false;
            return false;
        }

        if( !weapon.is_null() ) {
            const std::string query = string_format( _( "Stop wielding %s?" ), weapon.tname() );
            if( !dispose_item( item_location( *this, &weapon ), query ) ) {
                refund_power();
                bio.powered = false;
                return false;
            }
        }

        add_msg_activate();

        set_wielded_item( bio.get_weapon() );
        get_wielded_item()->invlet = '#';
        weapon_bionic_uid = bio.get_uid();
    } else if( bio.id == bio_evap ) {
        if( player_character.is_underwater() ) {
            add_msg_if_player( m_info,
                               _( "There's a lot of water around you already, no need to use your %s." ), bio.info().name );
            bio.powered = false;
            return false;
        }

        add_msg_activate();
        const w_point weatherPoint = *get_weather().weather_precise;
        int humidity = get_local_humidity( weatherPoint.humidity, get_weather().weather_id,
                                           g->is_sheltered( player_character.pos() ) );
        // thirst units = 5 mL
        int water_available = std::lround( humidity * 3.0 / 100.0 );
        if( water_available == 0 ) {
            bio.powered = false;
            add_msg_if_player( m_bad, _( "There is not enough humidity in the air for your %s to function." ),
                               bio.info().name );
            return false;
        } else if( water_available == 1 ) {
            add_msg_if_player( m_mixed,
                               _( "Your %s issues a low humidity warning.  Efficiency will be reduced." ),
                               bio.info().name );
        }
    } else if( bio.id == bio_cqb ) {
        add_msg_activate();
        const avatar *you = as_avatar();
        if( you && !martial_arts_data->pick_style( *you ) ) {
            bio.powered = false;
            add_msg_if_player( m_info, _( "You change your mind and turn it off." ) );
            return false;
        }
    } else if( bio.id == bio_resonator ) {
        add_msg_activate();
        //~Sound of a bionic sonic-resonator shaking the area
        sounds::sound( pos(), 30, sounds::sound_t::combat, _( "VRRRRMP!" ), false, "bionic",
                       static_cast<std::string>( bio_resonator ) );
        for( const tripoint &bashpoint : here.points_in_radius( pos(), 1 ) ) {
            here.bash( bashpoint, 110 );
            // Multibash effect, so that doors &c will fall
            here.bash( bashpoint, 110 );
            here.bash( bashpoint, 110 );
        }

        mod_moves( -100 );
    } else if( bio.id == bio_time_freeze ) {
        if( mounted ) {
            refund_power();
            add_msg_if_player( m_info, _( "You cannot activate %s while mounted." ), bio.info().name );
            return false;
        }
        add_msg_activate();

        mod_moves( units::to_kilojoule( get_power_level() ) );
        set_power_level( 0_kJ );
        add_msg_if_player( m_good, _( "Your speed suddenly increases!" ) );
        if( one_in( 3 ) ) {
            add_msg_if_player( m_bad, _( "Your muscles tear with the strain." ) );
            apply_damage( nullptr, bodypart_id( "arm_l" ), rng( 5, 10 ) );
            apply_damage( nullptr, bodypart_id( "arm_r" ), rng( 5, 10 ) );
            apply_damage( nullptr, bodypart_id( "leg_l" ), rng( 7, 12 ) );
            apply_damage( nullptr, bodypart_id( "leg_r" ), rng( 7, 12 ) );
            apply_damage( nullptr, bodypart_id( "torso" ), rng( 5, 15 ) );
        }
        if( one_in( 5 ) ) {
            add_effect( effect_teleglow, rng( 5_minutes, 40_minutes ) );
        }
    } else if( bio.id == bio_teleport ) {
        if( mounted ) {
            refund_power();
            add_msg_if_player( m_info, _( "You cannot activate %s while mounted." ), bio.info().name );
            return false;
        }
        add_msg_activate();

        teleport::teleport( *this );
        mod_moves( -100 );
    } else if( bio.id == bio_blood_anal ) {
        add_msg_activate();
        conduct_blood_analysis();
    } else if( bio.id == bio_blood_filter ) {
        add_msg_activate();
        static const std::vector<efftype_id> removable = {{
                effect_fungus, effect_dermatik, effect_bloodworms,
                effect_tetanus, effect_poison, effect_badpoison,
                effect_pkill1, effect_pkill2, effect_pkill3, effect_pkill_l,
                effect_drunk, effect_cig, effect_high, effect_hallu, effect_visuals,
                effect_pblue, effect_iodine, effect_datura,
                effect_took_xanax, effect_took_prozac, effect_took_prozac_bad,
                effect_took_flumed, effect_antifungal, effect_venom_weaken,
                effect_venom_dmg, effect_paralyzepoison
            }
        };

        for( const string_id<effect_type> &eff : removable ) {
            remove_effect( eff );
        }
        // Purging the substance won't remove the fatigue it caused
        force_comedown( get_effect( effect_adrenaline ) );
        force_comedown( get_effect( effect_meth ) );
        set_painkiller( 0 );
        set_stim( 0 );
        mod_moves( -100 );
    } else if( bio.id == bio_torsionratchet ) {
        add_msg_activate();
        add_msg_if_player( m_info, _( "Your torsion ratchet locks onto your joints." ) );
    } else if( bio.id == bio_jointservo ) {
        add_msg_activate();
        add_msg_if_player( m_info, _( "You can now run faster, assisted by joint servomotors." ) );
    } else if( bio.id == bio_lighter ) {
        const std::optional<tripoint> pnt = choose_adjacent( _( "Start a fire where?" ) );
        if( pnt && here.is_flammable( *pnt ) ) {
            add_msg_activate();
            here.add_field( *pnt, fd_fire, 1 );
            if( has_trait( trait_PYROMANIA ) ) {
                add_morale( MORALE_PYROMANIA_STARTFIRE, 5, 10, 3_hours, 2_hours );
                rem_morale( MORALE_PYROMANIA_NOFIRE );
                add_msg_if_player( m_good, _( "You happily light a fire." ) );
            }
            mod_moves( -100 );
        } else {
            refund_power();
            add_msg_if_player( m_info, _( "There's nothing to light there." ) );
            return false;
        }
    } else if( bio.id == bio_geiger ) {
        add_msg_activate();
        add_msg_if_player( m_info, _( "Your radiation level: %d" ), get_rad() );
    } else if( bio.id == bio_radscrubber ) {
        add_msg_activate();
        if( get_rad() > 4 ) {
            mod_rad( -5 );
        } else {
            set_rad( 0 );
        }
    } else if( bio.id == bio_adrenaline ) {
        add_msg_activate();
        if( has_effect( effect_adrenaline ) ) {
            add_msg_if_player( m_bad, _( "Safeguards kick in, and the bionic refuses to activate!" ) );
            refund_power();
            return false;
        } else {
            add_msg_activate();
            add_effect( effect_adrenaline, 20_minutes );
        }
    } else if( bio.id == bio_emp ) {
        if( const std::optional<tripoint> pnt = choose_adjacent( _( "Create an EMP where?" ) ) ) {
            add_msg_activate();
            explosion_handler::emp_blast( *pnt );
            mod_moves( -100 );
        } else {
            refund_power();
            return false;
        }
    } else if( bio.id == bio_hydraulics ) {
        add_msg_activate();
        add_msg_if_player( m_good, _( "Your muscles hiss as hydraulic strength fills them!" ) );
        //~ Sound of hissing hydraulic muscle! (not quite as loud as a car horn)
        sounds::sound( pos(), 19, sounds::sound_t::activity, _( "HISISSS!" ), false, "bionic",
                       static_cast<std::string>( bio_hydraulics ) );
    } else if( bio.id == bio_water_extractor ) {
        bool no_target = true;
        bool extracted = false;
        for( item &it : here.i_at( pos() ) ) {
            static const units::volume volume_per_water_charge = 500_ml;
            if( it.is_corpse() ) {
                const int avail = it.get_var( "remaining_water", it.volume() / volume_per_water_charge );
                if( avail > 0 ) {
                    no_target = false;
                    if( query_yn( _( "Extract water from the %s" ),
                                  colorize( it.tname(), it.color_in_inventory() ) ) ) {
                        item water( itype_water_clean, calendar::turn, avail );
                        water.set_item_temperature( it.temperature );
                        if( liquid_handler::consume_liquid( water ) ) {
                            add_msg_activate();
                            extracted = true;
                            it.set_var( "remaining_water", static_cast<int>( water.charges ) );
                        }
                        break;
                    }
                }
            }
        }
        if( no_target ) {
            add_msg_if_player( m_bad, _( "There is no suitable corpse on this tile." ) );
        }
        if( !extracted ) {
            refund_power();
            return false;
        }
    } else if( bio.id == bio_magnet ) {
        add_msg_activate();
        static const std::set<material_id> affected_materials =
        { material_iron, material_steel, material_lc_steel, material_mc_steel, material_hc_steel, material_ch_steel, material_qt_steel, material_budget_steel };
        // Remember all items that will be affected, then affect them
        // Don't "snowball" by affecting some items multiple times
        std::vector<std::pair<item, tripoint>> affected;
        const units::mass weight_cap = weight_capacity();
        for( const tripoint &p : here.points_in_radius( pos(), 10 ) ) {
            if( p == pos() || !here.has_items( p ) || here.has_flag( ter_furn_flag::TFLAG_SEALED, p ) ) {
                continue;
            }

            map_stack stack = here.i_at( p );
            for( auto it = stack.begin(); it != stack.end(); it++ ) {
                if( it->weight() < weight_cap &&
                    it->made_of_any( affected_materials ) ) {
                    affected.emplace_back( *it, p );
                    stack.erase( it );
                    break;
                }
            }
        }

        for( const std::pair<item, tripoint> &pr : affected ) {
            projectile proj;
            proj.speed  = 50;
            proj.impact = damage_instance();
            // FIXME: Hardcoded damage type
            proj.impact.add_damage( STATIC( damage_type_id( "bash" ) ), pr.first.weight() / 250_gram );
            // make the projectile stop one tile short to prevent hitting the player
            proj.range = rl_dist( pr.second, pos() ) - 1;
            proj.proj_effects = {{ "NO_ITEM_DAMAGE", "DRAW_AS_LINE", "NO_DAMAGE_SCALING", "JET" }};

            dealt_projectile_attack dealt = projectile_attack(
                                                proj, pr.second, pos(), dispersion_sources{ 0 }, this );
            here.add_item_or_charges( dealt.end_point, pr.first );
        }

        mod_moves( -100 );
    } else if( bio.id == bio_lockpick ) {
        if( !is_avatar() ) {
            return false;
        }
        std::optional<tripoint> target = lockpick_activity_actor::select_location( player_character );
        if( target.has_value() ) {
            add_msg_activate();
            assign_activity( lockpick_activity_actor::use_bionic( here.getabs( *target ) ) );
            if( close_bionics_ui ) {
                *close_bionics_ui = true;
            }
        } else {
            refund_power();
            return false;
        }
    } else if( bio.id == bio_flashbang ) {
        add_msg_activate();
        explosion_handler::flashbang( pos(), true );
        mod_moves( -100 );
    } else if( bio.id == bio_shockwave ) {
        add_msg_activate();
        explosion_handler::shockwave( pos(), 3, 4, 2, 8, true );
        add_msg_if_player( m_neutral, _( "You unleash a powerful shockwave!" ) );
        mod_moves( -100 );
    } else if( bio.id == bio_meteorologist ) {
        add_msg_activate();
        // Calculate local wind power
        int vehwindspeed = 0;
        if( optional_vpart_position vp = here.veh_at( pos() ) ) {
            // vehicle velocity in mph
            vehwindspeed = std::abs( vp->vehicle().velocity / 100 );
        }
        const oter_id &cur_om_ter = overmap_buffer.ter( global_omt_location() );
        /* cache g->get_temperature( player location ) since it is used twice. No reason to recalc */
        weather_manager &weather = get_weather();
        const units::temperature player_local_temp = weather.get_temperature( player_character.pos() );
        const int windpower = get_local_windpower( weather.windspeed + vehwindspeed,
                              cur_om_ter, get_location(), weather.winddirection, g->is_sheltered( pos() ) );
        add_msg_if_player( m_info, _( "Temperature: %s." ), print_temperature( player_local_temp ) );
        const w_point weatherPoint = *weather.weather_precise;
        add_msg_if_player( m_info, _( "Relative Humidity: %s." ),
                           print_humidity(
                               get_local_humidity( weatherPoint.humidity, get_weather().weather_id,
                                       g->is_sheltered( player_character.pos() ) ) ) );
        add_msg_if_player( m_info, _( "Pressure: %s." ),
                           print_pressure( static_cast<int>( weatherPoint.pressure ) ) );
        add_msg_if_player( m_info, _( "Wind Speed: %.1f %s." ),
                           convert_velocity( windpower * 100, VU_WIND ),
                           velocity_units( VU_WIND ) );
        add_msg_if_player( m_info, _( "Feels Like: %s." ),
                           print_temperature( player_local_temp + get_local_windchill( weatherPoint.temperature,
                                              weatherPoint.humidity,  windpower ) ) );
        std::string dirstring = get_dirstring( weather.winddirection );
        add_msg_if_player( m_info, _( "Wind Direction: From the %s." ), dirstring );
    } else if( bio.id == bio_remote ) {
        add_msg_activate();
        int choice = uilist( _( "Perform which function:" ), {
            _( "Control vehicle" ), _( "RC radio" )
        } );
        if( choice >= 0 && choice <= 1 ) {
            item ctr;
            if( choice == 0 ) {
                ctr = item( "remotevehcontrol", calendar::turn_zero );
            } else {
                ctr = item( "radiocontrol", calendar::turn_zero );
            }
            ctr.charges = units::to_kilojoule( get_power_level() );
            int power_use = invoke_item( &ctr );
            mod_power_level( units::from_kilojoule( -power_use ) );
            bio.powered = ctr.active;
        } else {
            bio.powered = g->remoteveh() != nullptr || !get_value( "remote_controlling" ).empty();
        }
    } else if( bio.info().is_remote_fueled ) {
        std::vector<item *> cables = cache_get_items_with( flag_CABLE_SPOOL );
        bool has_cable = !cables.empty();
        bool free_cable = false;
        bool success = false;
        if( !has_cable ) {
            add_msg_if_player( m_info,
                               _( "You need a jumper cable connected to a power source to drain power from it." ) );
        } else {
            for( const item *cable : cables ) {
                if( !cable->link || cable->link->has_no_links() ) {
                    free_cable = true;
                } else if( cable->link->has_states( link_state::no_link, link_state::bio_cable ) ) {
                    add_msg_if_player( m_info,
                                       _( "You have a cable attached to your CBM but it also has to be connected to a power source." ) );
                } else if( cable->link->has_states( link_state::solarpack, link_state::bio_cable ) ) {
                    add_msg_activate();
                    success = true;
                    add_msg_if_player( m_info,
                                       _( "You are attached to a solar pack.  It will charge you if it's unfolded and in sunlight." ) );
                } else if( cable->link->has_states( link_state::ups, link_state::bio_cable ) ) {
                    add_msg_activate();
                    success = true;
                    add_msg_if_player( m_info,
                                       _( "You are attached to a UPS.  It will charge you if it has some juice in it." ) );
                } else if( cable->link->has_states( link_state::bio_cable, link_state::vehicle_battery ) ||
                           cable->link->has_states( link_state::bio_cable, link_state::vehicle_port ) ) {
                    add_msg_activate();
                    success = true;
                    add_msg_if_player( m_info,
                                       _( "You are attached to a vehicle.  It will charge you if it has some juice in it." ) );
                } else if( cable->link->s_state == link_state::solarpack ||
                           cable->link->s_state == link_state::ups ) {
                    add_msg_if_player( m_info,
                                       _( "You have a cable attached to a portable power source, but you also need to connect it to your CBM." ) );
                } else if( cable->link->t_state == link_state::vehicle_battery ||
                           cable->link->t_state == link_state::vehicle_port ) {
                    add_msg_if_player( m_info,
                                       _( "You have a cable attached to a vehicle, but you also need to connect it to your CBM." ) );
                }
            }

            if( free_cable && !success ) {
                add_msg_if_player( m_info,
                                   _( "You have at least one free cable in your inventory that you could use to connect yourself." ) );
            }
        }
        if( !success ) {
            refund_power();
            bio.powered = false;
            return false;
        }
    } else {
        add_msg_activate();

        const std::vector<bionic_id> &deactivated_bionics = bio.info().autodeactivated_bionics;
        if( !deactivated_bionics.empty() ) {
            for( bionic &it : *my_bionics ) {
                if( std::find( deactivated_bionics.begin(), deactivated_bionics.end(),
                               it.id ) != deactivated_bionics.end() ) {
                    if( it.powered ) {
                        it.powered = false;
                        add_msg_if_player( m_info, _( "Your %s automatically turn off." ), it.info().name );
                    }
                }
            }
        }
    }
    bio_flag_cache.clear();
    // Recalculate stats (strength, mods from pain etc.) that could have been affected
    calc_encumbrance();
    reset();

    // Also reset crafting inventory cache if this bionic spawned a fake item
    if( bio.has_weapon() || !bio.info().passive_pseudo_items.empty() ||
        !bio.info().toggled_pseudo_items.empty() ) {
        invalidate_pseudo_items();
        invalidate_crafting_inventory();
    }

    return true;
}

ret_val<void> Character::can_deactivate_bionic( bionic &bio, bool eff_only ) const
{

    if( bio.incapacitated_time > 0_turns ) {
        return ret_val<void>::make_failure( _( "Your %s is shorting out and can't be deactivated." ),
                                            bio.info().name );
    }

    if( !eff_only ) {
        if( !bio.powered ) {
            // It's already off!
            return ret_val<void>::make_failure();
        }
        if( !bio.info().has_flag( json_flag_BIONIC_TOGGLED ) ) {
            // It's a fire-and-forget bionic, we can't turn it off but have to wait for
            //it to run out of charge
            return ret_val<void>::make_failure( _( "You can't deactivate your %s manually!" ),
                                                bio.info().name );
        }
        if( get_power_level() < bio.info().power_deactivate ) {
            return ret_val<void>::make_failure( _( "You don't have the power to deactivate your %s." ),
                                                bio.info().name );
        }
    }

    return ret_val<void>::make_success();
}

bool Character::deactivate_bionic( bionic &bio, bool eff_only )
{
    const auto can_deactivate = can_deactivate_bionic( bio, eff_only );
    bio_flag_cache.clear();
    if( !can_deactivate.success() ) {
        if( !can_deactivate.str().empty() ) {
            add_msg( m_info,  can_deactivate.str() );
        }
        return false;
    }

    // Just do the effect, no stat changing or messages
    if( !eff_only ) {
        //We can actually deactivate now, do deactivation-y things
        mod_power_level( -bio.info().power_deactivate );
        bio.powered = false;
        add_msg_if_player( m_neutral, _( "You deactivate your %s." ), bio.info().name );
    }

    // Deactivation effects go here

    for( const effect_on_condition_id &eoc : bio.id->deactivated_eocs ) {
        dialogue d( get_talker_for( *this ), nullptr );
        if( eoc->type == eoc_type::ACTIVATION ) {
            eoc->activate( d );
        } else {
            debugmsg( "Must use an activation eoc for a bionic deactivation.  If you don't want the effect_on_condition to happen on its own (without the bionic being activated), remove the recurrence min and max.  Otherwise, create a non-recurring effect_on_condition for this bionic with its condition and effects, then have a recurring one queue it." );
        }
    }

    if( bio.info().has_flag( json_flag_BIONIC_WEAPON ) ) {
        if( bio.get_uid() == get_weapon_bionic_uid() ) {
            bio.set_weapon( *get_wielded_item() );
            add_msg_if_player( _( "You withdraw your %s." ), weapon.tname() );
            if( get_player_view().sees( pos() ) ) {
                if( male ) {
                    add_msg_if_npc( m_info, _( "<npcname> withdraws his %s." ), weapon.tname() );
                } else {
                    add_msg_if_npc( m_info, _( "<npcname> withdraws her %s." ), weapon.tname() );
                }
            }
            set_wielded_item( item() );
            weapon_bionic_uid = 0;
        }
    } else if( bio.id == bio_cqb ) {
        martial_arts_data->selected_style_check();
    } else if( bio.id == bio_remote ) {
        if( g->remoteveh() != nullptr && !has_active_item( itype_remotevehcontrol ) ) {
            g->setremoteveh( nullptr );
        } else if( !get_value( "remote_controlling" ).empty() &&
                   !has_active_item( itype_radiocontrol ) ) {
            set_value( "remote_controlling", "" );
        }
    }

    // Recalculate stats (strength, mods from pain etc.) that could have been affected
    calc_encumbrance();
    reset();
    if( !bio.id->enchantments.empty() ) {
        recalculate_enchantment_cache();
    }

    // Also reset crafting inventory cache if this bionic spawned a fake item
    if( bio.has_weapon() || !bio.info().passive_pseudo_items.empty() ||
        !bio.info().toggled_pseudo_items.empty() ) {
        invalidate_pseudo_items();
        invalidate_crafting_inventory();
    }

    return true;
}

Character::bionic_fuels Character::bionic_fuel_check( bionic &bio,
        const bool start )
{
    bionic_fuels result;

    const bool is_remote_fueled = bio.info().is_remote_fueled;

    if( bio.id->fuel_opts.empty() && !is_remote_fueled ) {
        return result;
    }

    if( is_remote_fueled ) {
        result.connected_vehicles = get_cable_vehicle();
        result.connected_solar = get_cable_solar();
        result.connected_fuel = get_cable_ups();
    } else {
        result.connected_fuel = get_bionic_fuels( bio.id );
    }

    int other_charges = 0;
    // There could be multiple fuels. But probably not. So we just check the first.
    bool is_perpetual = !bio.id->fuel_opts.empty() &&
                        bio.id->fuel_opts.front()->get_fuel_data().is_perpetual_fuel;
    bool metabolism_powered = !bio.id->fuel_opts.empty() &&
                              bio.id->fuel_opts.front() == fuel_type_metabolism;

    if( metabolism_powered ) {
        other_charges = std::max( 0.0f, get_stored_kcal() - 0.8f * get_healthy_kcal() );
    }

    if( !is_perpetual && !other_charges && result.connected_vehicles.empty() &&
        result.connected_solar.empty() &&
        result.connected_fuel.empty() ) {
        if( metabolism_powered ) {
            if( start ) {
                add_msg_player_or_npc( m_info,
                                       _( "Your %s cannot be started because your calories are below safe levels." ),
                                       _( "<npcname>'s %s cannot be started because their calories are below safe levels." ),
                                       bio.info().name );

            } else if( bio.powered ) {
                add_msg_player_or_npc( m_info,
                                       _( "Stored calories are below the safe threshold, your %s shuts down to preserve your health." ),
                                       _( "Stored calories are below the safe threshold, <npcname>'s %s shuts down to preserve their health." ),
                                       bio.info().name );

                bio.powered = false;
                deactivate_bionic( bio, true );
            }
        } else if( start ) {
            add_msg_player_or_npc( m_bad, _( "Your %s does not have enough fuel to start." ),
                                   _( "<npcname>'s %s does not have enough fuel to start." ),
                                   bio.info().name );
        } else if( bio.powered ) {
            add_msg_player_or_npc( m_info,
                                   _( "Your %s runs out of fuel and turns off." ),
                                   _( "<npcname>'s %s runs out of fuel and turns off." ),
                                   bio.info().name );
            bio.powered = false;
            deactivate_bionic( bio, true );
        }
        result.can_be_on = false;
    }

    return result;
}

void Character::burn_fuel( bionic &bio )
{
    float efficiency;
    if( !bio.powered ) {
        // Modifiers for passive bionic
        if( bio.info().power_trickle != 0_J ) {
            mod_power_level( bio.info().power_trickle );
        }

        efficiency = get_effective_efficiency( bio, bio.info().passive_fuel_efficiency );
        if( efficiency == 0.f ) {
            return;
        }
        return;
    } else {
        efficiency = get_effective_efficiency( bio, bio.info().fuel_efficiency );
    }

    if( bio.is_safe_fuel_on()
        && get_power_level() + 1_kJ * efficiency >= get_max_power_level() * std::min( 1.0f,
                bio.get_safe_fuel_thresh() ) ) {
        // Do not waste fuel charging over limit.
        // 1 kJ treshold to avoid draining full fuel on trickle charge
        // Individual power sources need their own check for this because power per charge can be large.
        return;
    }

    bionic_fuels result = bionic_fuel_check( bio, false );

    units::energy energy_gain = 0_kJ;
    map &here = get_map();

    // Each bionic *should* have only one power source.
    // Avoid draining multiple sources. So check for energy_gain = 0_kJ

    // Vehicle may not have power.
    // Return out if power was drained. Otherwise continue with other sources.
    if( !result.connected_vehicles.empty() ) {
        // Cable bionic charging from connected vehicle(s)
        for( vehicle *veh : result.connected_vehicles ) {
            int undrained = veh->discharge_battery( 1 );
            if( undrained == 0 ) {
                energy_gain = 1_kJ;
                break;
            }
        }
    }

    // Rest of the fuel sources are known to exist so we can exit after any of them is used
    if( energy_gain == 0_J && !result.connected_fuel.empty() ) {
        // Bionic that uses fuel items from some external connected source.

        for( item *fuel_source : result.connected_fuel ) {
            item *fuel;
            // Fuel may be ammo or in container
            if( fuel_source->ammo_remaining() ) {
                fuel = &fuel_source->first_ammo();
            } else {
                fuel = fuel_source->all_items_ptr( item_pocket::pocket_type::CONTAINER ).front();
            }
            energy_gain = fuel->fuel_energy();

            if( bio.is_safe_fuel_on() &&
                get_power_level() + energy_gain * efficiency >= get_max_power_level() * std::min( 1.0f,
                        bio.get_safe_fuel_thresh() ) ) {
                // Do not waste fuel charging over limit.
                return;
            }
            if( fuel->count_by_charges() ) {
                fuel->charges--;
                if( fuel->charges == 0 ) {
                    i_rem( fuel );
                }
            } else {
                i_rem( fuel );
            }

        }
    }

    // There *could* be multiple fuel sources. But we just check first for solar.
    bool solar_powered = ( !bio.id->fuel_opts.empty() &&
                           bio.id->fuel_opts.front() == fuel_type_sun_light ) ||
                         !result.connected_solar.empty();
    if( energy_gain == 0_J && solar_powered && !g->is_sheltered( pos() ) ) {
        // Some sort of solar source

        const weather_type_id &wtype = current_weather( get_location() );
        float intensity = incident_sun_irradiance( wtype, calendar::turn ); // W/m2

        if( !result.connected_solar.empty() ) {
            // Cable charger connected to solar panel item.
            // There *could* be multiple items. But we only take first.
            intensity *= result.connected_solar.front()->type->solar_efficiency;
        }
        energy_gain = units::from_joule( intensity );
    }

    // There *could* be multiple fuel sources. But we just check first for solar.
    bool metabolism_powered = !bio.id->fuel_opts.empty() &&
                              bio.id->fuel_opts.front() == fuel_type_metabolism;
    if( energy_gain == 0_J && metabolism_powered ) {
        // Bionic powered by metabolism
        // 1kcal = 4184 J
        energy_gain = 4184_J;

        if( bio.is_safe_fuel_on() &&
            get_power_level() + energy_gain * efficiency >= get_max_power_level() * std::min( 1.0f,
                    bio.get_safe_fuel_thresh() ) ) {
            // Do not waste fuel charging over limit.
            // Individual power sources need their own check for this because power per charge can be large.
            return;
        }

        mod_stored_kcal( -1, true );
    }

    // There *could* be multiple fuel sources. But we just check first for solar.
    bool wind_powered = !bio.id->fuel_opts.empty() && bio.id->fuel_opts.front() == fuel_type_wind;
    if( energy_gain == 0_J && wind_powered ) {
        // Wind power
        int vehwindspeed = 0;

        const optional_vpart_position vp = here.veh_at( pos() );
        if( vp ) {
            vehwindspeed = std::abs( vp->vehicle().velocity / 100 );
        }
        weather_manager &weather = get_weather();
        const int windpower = get_local_windpower( weather.windspeed + vehwindspeed,
                              overmap_buffer.ter( global_omt_location() ), get_location(), weather.winddirection,
                              g->is_sheltered( pos() ) );
        energy_gain = 1_kJ * windpower;
    }

    mod_power_level( energy_gain * efficiency );
    heat_emission( bio, energy_gain );
    here.emit_field( pos(), bio.info().power_gen_emission );
}

void Character::heat_emission( const bionic &bio, units::energy fuel_energy )
{
    if( !bio.info().exothermic_power_gen ) {
        return;
    }
    const float efficiency = bio.info().fuel_efficiency;

    const int heat_prod = units::to_kilojoule( fuel_energy * ( 1.0f - efficiency ) );
    const int heat_level = std::min( heat_prod / 10, 4 );
    const emit_id hotness = emit_id( "emit_hot_air" + std::to_string( heat_level ) + "_cbm" );
    map &here = get_map();
    if( hotness.is_valid() ) {
        const int heat_spread = std::max( heat_prod / 10 - heat_level, 1 );
        here.emit_field( pos(), hotness, heat_spread );
    }
    for( const std::pair<const bodypart_str_id, size_t> &bp : bio.info().occupied_bodyparts ) {
        add_effect( effect_heating_bionic, 2_seconds, bp.first.id(), false, heat_prod );
    }
}

float Character::get_effective_efficiency( const bionic &bio, float fuel_efficiency ) const
{
    const std::optional<float> &coverage_penalty = bio.info().coverage_power_gen_penalty;
    float effective_efficiency = fuel_efficiency;
    if( coverage_penalty ) {
        int coverage = 0;
        const std::map< bodypart_str_id, size_t > &occupied_bodyparts = bio.info().occupied_bodyparts;
        for( const std::pair< const bodypart_str_id, size_t > &elem : occupied_bodyparts ) {
            coverage += worn.coverage_with_flags_exclude( elem.first.id(),
            { flag_ALLOWS_NATURAL_ATTACKS, flag_SEMITANGIBLE, flag_PERSONAL, flag_AURA } );
        }
        effective_efficiency = fuel_efficiency * ( 1.0 - ( coverage / ( 100.0 *
                               occupied_bodyparts.size() ) )
                               * coverage_penalty.value() );
    }
    return effective_efficiency;
}

/**
 * @param p the player
 * @param bio the bionic that is meant to be recharged.
 * @param amount the amount of power that is to be spent recharging the bionic.
 * @return indicates whether we successfully charged the bionic.
 */
static bool attempt_recharge( Character &p, bionic &bio, units::energy &amount )
{
    const bionic_data &info = bio.info();
    units::energy power_cost = info.power_over_time;
    bool recharged = false;

    if( power_cost > 0_kJ ) {
        if( info.has_flag( STATIC( json_character_flag( "BIONIC_ARMOR_INTERFACE" ) ) ) ) {
            // Don't spend any power on armor interfacing unless we're wearing active powered armor.
            if( !p.worn.is_wearing_active_power_armor() ) {
                const units::energy armor_power_cost = 1_kJ;
                power_cost -= armor_power_cost;
            }
        }
        if( p.get_power_level() >= power_cost ) {
            // Set the recharging cost and charge the bionic.
            amount = power_cost;
            bio.charge_timer = info.charge_time;
            recharged = true;
        }
    }

    return recharged;
}

void Character::process_bionic( bionic &bio )
{

    // Only powered bionics should be processed
    if( !bio.powered ) {
        burn_fuel( bio );
        return;
    }

    if( bio.get_uid() == get_weapon_bionic_uid() ) {
        const bool wrong_weapon_wielded = weapon.typeId() != bio.get_weapon().typeId() ||
                                          !weapon.has_flag( flag_NO_UNWIELD );

        if( wrong_weapon_wielded ) {
            // Wielded weapon replaced in an unexpected way
            debugmsg( "Wielded weapon doesn't match the expected weapon equipped from %s", bio.id->name );
            weapon_bionic_uid = 0;
        }

        if( weapon.is_null() || wrong_weapon_wielded ) {
            // Force deactivation because the weapon is gone
            force_bionic_deactivation( bio );
            return;
        }
    }

    // These might be affected by environmental conditions, status effects, faulty bionics, etc.
    time_duration discharge_rate = 1_turns;

    units::energy cost = 0_mJ;

    bio.charge_timer = std::max( 0_turns, bio.charge_timer - discharge_rate );
    if( bio.charge_timer <= 0_turns ) {
        if( bio.info().charge_time > 0_turns ) {
            if( bio.info().has_flag( json_flag_BIONIC_POWER_SOURCE ) ) {
                // Convert fuel to bionic power
                burn_fuel( bio );
                // Reset timer
                bio.charge_timer = bio.info().charge_time;
            } else {
                // Try to recharge our bionic if it is made for it
                bool recharged = attempt_recharge( *this, bio, cost );
                if( !recharged ) {
                    // No power to recharge, so deactivate
                    bio.powered = false;
                    add_msg_if_player( m_neutral, _( "Your %s powers down." ), bio.info().name );
                    // This purposely bypasses the deactivation cost
                    deactivate_bionic( bio, true );
                    return;
                }
                if( cost > 0_mJ ) {
                    mod_power_level( -cost );
                }
            }
        }
    }

    for( const effect_on_condition_id &eoc : bio.id->processed_eocs ) {
        dialogue d( get_talker_for( *this ), nullptr );
        if( eoc->type == eoc_type::ACTIVATION ) {
            eoc->activate( d );
        } else {
            debugmsg( "Must use an activation eoc for a bionic process.  If you don't want the effect_on_condition to happen on its own (without the bionic being activated), remove the recurrence min and max.  Otherwise, create a non-recurring effect_on_condition for this bionic with its condition and effects, then have a recurring one queue it." );
        }
    }

    // Bionic effects on every turn they are active go here.
    if( bio.id == bio_remote ) {
        if( g->remoteveh() == nullptr && get_value( "remote_controlling" ).empty() ) {
            bio.powered = false;
            add_msg_if_player( m_warning, _( "Your %s has lost connection and is turning off." ),
                               bio.info().name );
        }
    } else if( bio.id == bio_hydraulics ) {
        // Sound of hissing hydraulic muscle! (not quite as loud as a car horn)
        sounds::sound( pos(), 19, sounds::sound_t::activity, _( "HISISSS!" ), false, "bionic",
                       static_cast<std::string>( bio_hydraulics ) );
    } else if( bio.id == bio_nanobots ) {
        std::forward_list<bodypart_id> bleeding_bp_parts;
        for( const bodypart_id &bp : get_all_body_parts() ) {
            if( has_effect( effect_bleed, bp.id() ) ) {
                bleeding_bp_parts.push_front( bp );
            }
        }
        std::vector<bodypart_id> damaged_hp_parts;
        for( const std::pair<const bodypart_str_id, bodypart> &part : get_body() ) {
            const int hp_cur = part.second.get_hp_cur();
            if( hp_cur > 0 && hp_cur < part.second.get_hp_max() ) {
                damaged_hp_parts.push_back( part.first.id() );
            }
        }
        if( damaged_hp_parts.empty() && bleeding_bp_parts.empty() ) {
            // Nothing to heal. Return the consumed power and exit early
            mod_power_level( cost );
            return;
        }
        for( const bodypart_id &i : bleeding_bp_parts ) {
            // effectively reduces by 1 intensity level
            if( get_stored_kcal() >= 15 ) {
                get_effect( effect_bleed, i ).mod_duration( -get_effect( effect_bleed, i ).get_int_dur_factor() );
                mod_stored_kcal( -15 );
            } else {
                bleeding_bp_parts.clear();
                break;
            }
        }
        if( calendar::once_every( 60_turns ) ) {
            if( get_stored_kcal() >= 5 && !damaged_hp_parts.empty() ) {
                const bodypart_id part_to_heal = damaged_hp_parts[ rng( 0, damaged_hp_parts.size() - 1 ) ];
                heal( part_to_heal, 1 );
                mod_stored_kcal( -5 );
            }
        }
    } else if( bio.id == bio_painkiller ) {
        const int pkill = get_painkiller();
        const int pain = get_pain();
        const units::energy trigger_cost = bio.info().power_trigger;
        int max_pkill = std::min( 150, pain );
        if( pkill < max_pkill ) {
            mod_painkiller( 1 );
            mod_power_level( -trigger_cost );
        }

        // Only dull pain so extreme that we can't pkill it safely
        if( pkill >= 150 && pain > pkill && get_stim() > -150 ) {
            mod_pain( -1 );
            // Negative side effect: negative stim
            mod_stim( -1 );
            mod_power_level( -trigger_cost );
        }
    } else if( bio.id == bio_gills ) {
        if( has_effect( effect_asthma ) ) {
            add_msg_if_player( m_good,
                               _( "You feel your throat open up and air filling your lungs!" ) );
            remove_effect( effect_asthma );
        }
    } else if( bio.id == bio_evap ) {
        if( is_underwater() ) {
            add_msg_if_player( m_info,
                               _( "Your %s deactivates after it finds itself completely submerged in water." ), bio.info().name );
            deactivate_bionic( bio );
        }

        // Aero-Evaporator provides water at 60 watts with 2 L / kWh efficiency
        // which is 10 mL per 5 minutes.  Humidity can modify the amount gained.
        if( calendar::once_every( 5_minutes ) ) {
            const w_point weatherPoint = *get_weather().weather_precise;
            int humidity = get_local_humidity( weatherPoint.humidity, get_weather().weather_id,
                                               g->is_sheltered( pos() ) );
            // in thirst units = 5 mL water
            int water_available = std::lround( humidity * 3.0 / 100.0 );
            // At 50% relative humidity or more, the player will draw 10 mL
            // At 16% relative humidity or less, the bionic will give up
            if( water_available == 0 ) {
                add_msg_if_player( m_bad,
                                   _( "There is not enough humidity in the air for your %s to function." ),
                                   bio.info().name );
                deactivate_bionic( bio );
            } else if( water_available == 1 ) {
                add_msg_if_player( m_mixed,
                                   _( "Your %s issues a low humidity warning.  Efficiency is reduced." ),
                                   bio.info().name );
            }

            mod_thirst( -water_available );
        }

        if( get_thirst() < -40 ) {
            add_msg_if_player( m_good,
                               _( "You are properly hydrated.  Your %s chirps happily." ),
                               bio.info().name );
            deactivate_bionic( bio );
        }
    } else if( bio.id == afs_bio_dopamine_stimulators ) {
        // Aftershock
        add_morale( MORALE_FEELING_GOOD, 20, 20, 30_minutes, 20_minutes, true );
    }
}

void Character::roll_critical_bionics_failure( const bodypart_id &bp )
{
    const bodypart_id bp_to_hurt = bp->main_part;
    if( one_in( get_part_hp_cur( bp_to_hurt ) / 4 ) ) {
        set_part_hp_cur( bp_to_hurt, 0 );
    }
}

void Character::bionics_uninstall_failure( int difficulty, int success, float adjusted_skill )
{
    // "success" should be passed in as a negative integer representing how far off we
    // were for a successful removal.  We use this to determine consequences for failing.
    success = std::abs( success );

    // failure level is decided by how far off the character was from a successful removal, and
    // this is scaled up or down by the ratio of difficulty/skill.  At high skill levels (or low
    // difficulties), only minor consequences occur.  At low skill levels, severe consequences
    // are more likely.
    const int failure_level = static_cast<int>( std::sqrt( success * 4.0 * difficulty /
                              adjusted_skill ) );
    const int fail_type = std::min( 5, failure_level );

    if( fail_type <= 0 ) {
        add_msg( m_neutral, _( "The removal fails without incident." ) );
        return;
    }

    add_msg( m_neutral, _( "The removal is a failure." ) );
    std::set<bodypart_id> bp_hurt;
    switch( fail_type ) {
        case 1:
            if( !has_flag( json_flag_PAIN_IMMUNE ) ) {
                add_msg_if_player( m_bad, _( "It really hurts!" ) );
                mod_pain( rng( 10, 30 ) );
            }
            break;

        case 2:
        case 3:
            for( const bodypart_id &bp : get_all_body_parts() ) {
                if( has_effect( effect_under_operation, bp.id() ) ) {
                    if( bp_hurt.count( bp->main_part ) > 0 ) {
                        continue;
                    }
                    bp_hurt.emplace( bp->main_part );
                    apply_damage( this, bp, rng( 5, 10 ), true );
                    add_msg_player_or_npc( m_bad, _( "Your %s is damaged." ), _( "<npcname>'s %s is damaged." ),
                                           body_part_name_accusative( bp ) );
                }
            }
            break;

        case 4:
        case 5:
            for( const bodypart_id &bp : get_all_body_parts() ) {
                if( has_effect( effect_under_operation, bp.id() ) ) {
                    if( bp_hurt.count( bp->main_part ) > 0 ) {
                        continue;
                    }
                    bp_hurt.emplace( bp->main_part );

                    apply_damage( this, bp, rng( 25, 50 ), true );
                    roll_critical_bionics_failure( bp );

                    add_msg_player_or_npc( m_bad, _( "Your %s is severely damaged." ),
                                           _( "<npcname>'s %s is severely damaged." ),
                                           body_part_name_accusative( bp ) );
                }
            }
            break;
    }

}

void Character::bionics_uninstall_failure( monster &installer, Character &patient, int difficulty,
        int success, float adjusted_skill )
{

    // "success" should be passed in as a negative integer representing how far off we
    // were for a successful removal.  We use this to determine consequences for failing.
    success = std::abs( success );

    // failure level is decided by how far off the monster was from a successful removal, and
    // this is scaled up or down by the ratio of difficulty/skill.  At high skill levels (or low
    // difficulties), only minor consequences occur.  At low skill levels, severe consequences
    // are more likely.
    const int failure_level = static_cast<int>( std::sqrt( success * 4.0 * difficulty /
                              adjusted_skill ) );
    const int fail_type = std::min( 5, failure_level );

    bool u_see = sees( patient );

    if( u_see || patient.is_avatar() ) {
        if( fail_type <= 0 ) {
            add_msg( m_neutral, _( "The removal fails without incident." ) );
            return;
        }
        switch( rng( 1, 5 ) ) {
            case 1:
                add_msg( m_mixed, _( "The %s flub the operation." ), installer.name() );
                break;
            case 2:
                add_msg( m_mixed, _( "The %s messes up the operation." ), installer.name() );
                break;
            case 3:
                add_msg( m_mixed, _( "The operation fails." ) );
                break;
            case 4:
                add_msg( m_mixed, _( "The operation is a failure." ) );
                break;
            case 5:
                add_msg( m_mixed, _( "The %s screws up the operation." ), installer.name() );
                break;
        }
    }
    std::set<bodypart_id> bp_hurt;
    switch( fail_type ) {
        case 1:
            if( !has_flag( json_flag_PAIN_IMMUNE ) ) {
                patient.add_msg_if_player( m_bad, _( "It really hurts!" ) );
                patient.mod_pain( rng( 10, 30 ) );
            }
            break;

        case 2:
        case 3:
            for( const bodypart_id &bp : get_all_body_parts() ) {
                if( has_effect( effect_under_operation, bp.id() ) ) {
                    if( bp_hurt.count( bp->main_part ) > 0 ) {
                        continue;
                    }
                    bp_hurt.emplace( bp->main_part );
                    patient.apply_damage( this, bp, rng( failure_level, failure_level * 2 ), true );
                    if( u_see ) {
                        patient.add_msg_player_or_npc( m_bad, _( "Your %s is damaged." ), _( "<npcname>'s %s is damaged." ),
                                                       body_part_name_accusative( bp ) );
                    }
                }
            }
            break;

        case 4:
        case 5:
            for( const bodypart_id &bp : get_all_body_parts() ) {
                if( has_effect( effect_under_operation, bp.id() ) ) {
                    if( bp_hurt.count( bp->main_part ) > 0 ) {
                        continue;
                    }
                    bp_hurt.emplace( bp->main_part );

                    patient.apply_damage( this, bp, rng( 25, 50 ), true );
                    roll_critical_bionics_failure( bp );

                    if( u_see ) {
                        patient.add_msg_player_or_npc( m_bad, _( "Your %s is severely damaged." ),
                                                       _( "<npcname>'s %s is severely damaged." ),
                                                       body_part_name_accusative( bp ) );
                    }
                }
            }
            break;
    }
}

bool Character::has_enough_anesth( const itype &cbm, Character &patient ) const
{
    if( !cbm.bionic ) {
        debugmsg( "has_enough_anesth( const itype *cbm ): %s is not a bionic", cbm.get_id().str() );
        return false;
    }

    if( patient.has_bionic( bio_painkiller ) || patient.has_flag( json_flag_PAIN_IMMUNE ) ||
        has_trait( trait_DEBUG_BIONICS ) ) {
        return true;
    }

    const int weight = units::to_kilogram( patient.bodyweight() ) / 10;
    const requirement_data req_anesth = *requirement_data_anesthetic *
                                        cbm.bionic->difficulty * 2 * weight;

    return req_anesth.can_make_with_inventory( crafting_inventory(), is_crafting_component );
}

bool Character::has_enough_anesth( const itype &cbm ) const
{
    if( has_bionic( bio_painkiller ) || has_flag( json_flag_PAIN_IMMUNE ) ||
        has_trait( trait_DEBUG_BIONICS ) ) {
        return true;
    }
    const int weight = units::to_kilogram( bodyweight() ) / 10;
    const requirement_data req_anesth = *requirement_data_anesthetic *
                                        cbm.bionic->difficulty * 2 * weight;
    if( !req_anesth.can_make_with_inventory( crafting_inventory(),
            is_crafting_component ) ) {
        std::string buffer = _( "You don't have enough anesthetic to perform the installation." );
        buffer += "\n";
        buffer += req_anesth.list_missing();
        popup( buffer, PF_NONE );
        return false;
    }
    return true;
}

void Character::consume_anesth_requirement( const itype &cbm, Character &patient )
{
    const int weight = units::to_kilogram( patient.bodyweight() ) / 10;
    const requirement_data req_anesth = *requirement_data_anesthetic *
                                        cbm.bionic->difficulty * 2 * weight;
    for( const auto &e : req_anesth.get_components() ) {
        consume_items( e, 1, is_crafting_component );
    }
    for( const auto &e : req_anesth.get_tools() ) {
        consume_tools( e );
    }
    invalidate_crafting_inventory();
}

bool Character::has_installation_requirement( const bionic_id &bid ) const
{
    if( bid->installation_requirement.is_empty() ) {
        return false;
    }

    if( !bid->installation_requirement->can_make_with_inventory( crafting_inventory(),
            is_crafting_component ) ) {
        std::string buffer = _( "You don't have the required components to perform the installation." );
        buffer += "\n";
        buffer += bid->installation_requirement->list_missing();
        popup( buffer, PF_NONE );
        return false;
    }

    return true;
}

void Character::consume_installation_requirement( const bionic_id &bid )
{
    for( const auto &e : bid->installation_requirement->get_components() ) {
        consume_items( e, 1, is_crafting_component );
    }
    for( const auto &e : bid->installation_requirement->get_tools() ) {
        consume_tools( e );
    }
    invalidate_crafting_inventory();
}

// bionic manipulation adjusted skill
float Character::bionics_adjusted_skill( bool autodoc, int skill_level ) const
{
    int pl_skill = bionics_pl_skill( autodoc, skill_level );

    // for chance_of_success calculation, shift skill down to a float between ~0.4 - 30
    float adjusted_skill = static_cast<float>( pl_skill ) - std::min( static_cast<float>( 40 ),
                           static_cast<float>( pl_skill ) - static_cast<float>( pl_skill ) / static_cast<float>( 10.0 ) );
    adjusted_skill += get_effect_int( effect_assisted );
    adjusted_skill *= env_surgery_bonus( 1 );
    return adjusted_skill;
}

int Character::bionics_pl_skill( bool autodoc, int skill_level ) const
{
    skill_id most_important_skill;
    skill_id important_skill;
    skill_id least_important_skill;

    if( autodoc ) {
        most_important_skill = skill_firstaid;
        important_skill = skill_computer;
        least_important_skill = skill_electronics;
    } else {
        most_important_skill = skill_electronics;
        important_skill = skill_firstaid;
        least_important_skill = skill_mechanics;
    }

    float pl_skill;
    if( skill_level == -1 ) {
        pl_skill = int_cur                                  * 4 +
                   get_greater_skill_or_knowledge_level( most_important_skill )  * 4 +
                   get_greater_skill_or_knowledge_level( important_skill )       * 3 +
                   get_greater_skill_or_knowledge_level( least_important_skill ) * 1;
    } else {
        // override chance as though all values were skill_level if it is provided
        pl_skill = 12 * skill_level;
    }

    // Medical residents have some idea what they're doing
    if( has_trait( trait_PROF_MED ) ) {
        pl_skill += 3;
    }

    // People trained in bionics gain an additional advantage towards using it
    if( has_trait( trait_PROF_AUTODOC ) ) {
        pl_skill += 7;
    }
    return round( pl_skill );
}

int bionic_success_chance( bool autodoc, int skill_level, int difficulty, const Character &target )
{
    return bionic_manip_cos( target.bionics_adjusted_skill( autodoc, skill_level ), difficulty );
}

// bionic manipulation chance of success
int bionic_manip_cos( float adjusted_skill, int bionic_difficulty )
{
    if( get_player_character().has_trait( trait_DEBUG_BIONICS ) ) {
        return 100;
    }

    int chance_of_success = 0;
    // we will base chance_of_success on a ratio of skill and difficulty
    // when skill=difficulty, this gives us 1.  skill < difficulty gives a fraction.
    float skill_difficulty_parameter = static_cast<float>( adjusted_skill /
                                       ( 4.0 * bionic_difficulty ) );

    // when skill == difficulty, chance_of_success is 50%. Chance of success drops quickly below that
    // to reserve bionics for characters with the appropriate skill.  For more difficult bionics, the
    // curve flattens out just above 80%
    chance_of_success = static_cast<int>( ( 100 * skill_difficulty_parameter ) /
                                          ( skill_difficulty_parameter + std::sqrt( 1 / skill_difficulty_parameter ) ) );

    return chance_of_success;
}

bool Character::can_uninstall_bionic( const bionic &bio, Character &installer, bool autodoc,
                                      int skill_level ) const
{

    // if malfunctioning bionics doesn't have associated item it gets a difficulty of 12
    int difficulty = 12;
    if( item::type_is_defined( bio.id->itype() ) ) {
        const itype *type = item::find_type( bio.id->itype() );
        if( type->bionic ) {
            difficulty = type->bionic->difficulty;
        }
    }

    Character &player_character = get_player_character();

    if( bio.is_included() ) {
        popup( _( "%s must remove the parent bionic to remove the %s." ), installer.disp_name(),
               bio.id->name );
        return false;
    }

    for( const bionic_id &bid : get_bionics() ) {
        if( bid->required_bionic && bid->required_bionic == bio.id ) {
            popup( _( "%s cannot be removed because installed bionic %s requires it." ), bio.id->name,
                   bid->name );
            return false;
        }
    }

    if( bio.id->cant_remove_reason.has_value() ) {
        popup( string_format( bio.id->cant_remove_reason.value(), disp_name( true ), disp_name() ) );
        return false;
    }

    // removal of bionics adds +2 difficulty over installation
    int chance_of_success = bionic_success_chance( autodoc, skill_level, difficulty + 2,
                            installer );

    if( chance_of_success >= 100 ) {
        if( !player_character.query_yn(
                _( "Are you sure you wish to uninstall the selected bionic?" ),
                100 - chance_of_success ) ) {
            return false;
        }
    } else {
        if( !player_character.query_yn(
                _( "WARNING: %i percent chance of SEVERE damage to all body parts!  Continue anyway?" ),
                ( 100 - static_cast<int>( chance_of_success ) ) ) ) {
            return false;
        }
    }

    return true;
}

bool Character::uninstall_bionic( const bionic &bio, Character &installer, bool autodoc,
                                  int skill_level )
{
    // if malfunctioning bionics doesn't have associated item it gets a difficulty of 12
    int difficulty = 12;
    if( item::type_is_defined( bio.id->itype() ) ) {
        const itype *type = item::find_type( bio.id->itype() );
        if( type->bionic ) {
            difficulty = type->bionic->difficulty;
        }
    }

    // removal of bionics adds +2 difficulty over installation
    int pl_skill = bionics_pl_skill( autodoc, skill_level );
    int chance_of_success = bionic_success_chance( autodoc, skill_level, difficulty + 2, installer );

    // Surgery is imminent, retract claws or blade if active
    for( bionic &it : *installer.my_bionics ) {
        if( it.powered && it.info().has_flag( json_flag_BIONIC_WEAPON ) ) {
            installer.deactivate_bionic( it );
        }
    }

    int success = chance_of_success - rng( 1, 100 );
    if( installer.has_trait( trait_DEBUG_BIONICS ) ) {
        perform_uninstall( bio, difficulty, success, pl_skill );
        return true;
    }
    assign_activity( ACT_OPERATION, to_moves<int>( difficulty * 20_minutes ) );

    activity.values.push_back( difficulty );
    activity.values.push_back( success );
    activity.values.push_back( bio.get_uid() );
    activity.values.push_back( pl_skill );
    activity.str_values.emplace_back( "uninstall" );
    activity.str_values.push_back( bio.id.str() );
    activity.str_values.emplace_back( "" ); // installer_name is unused for uninstall
    if( autodoc ) {
        activity.str_values.emplace_back( "true" );
    } else {
        activity.str_values.emplace_back( "false" );
    }
    for( const std::pair<const bodypart_str_id, size_t> &elem : bio.id->occupied_bodyparts ) {
        add_effect( effect_under_operation, difficulty * 20_minutes, elem.first.id(), true, difficulty );
    }

    return true;
}

void Character::perform_uninstall( const bionic &bio, int difficulty, int success, int pl_skill )
{
    map &here = get_map();
    std::optional<bionic *> bio_opt = find_bionic_by_uid( bio.get_uid() );
    if( !bio_opt ) {
        debugmsg( "Tried to uninstall non-existent bionic with UID %d", bio.get_uid() );
        return;
    }

    if( success > 0 ) {
        get_event_bus().send<event_type::removes_cbm>( getID(), bio.id );

        // until bionics can be flagged as non-removable
        add_msg_player_or_npc( m_neutral, _( "Your parts are jiggled back into their familiar places." ),
                               _( "<npcname>'s parts are jiggled back into their familiar places." ) );
        add_msg( m_good, _( "Successfully removed %s." ), bio.id.obj().name );
        const bionic_id bio_id = bio.id;
        remove_bionic( bio );

        // give us any muts it's supposed to (silently) if removed
        for( const trait_id &mid : bio_id->give_mut_on_removal ) {
            if( !has_trait( mid ) ) {
                set_mutation( mid );
            }
        }

        item cbm( "burnt_out_bionic" );
        if( item::type_is_defined( bio_id->itype() ) ) {
            cbm = item( bio_id.c_str() );
        }
        cbm.set_flag( flag_FILTHY );
        cbm.set_flag( flag_NO_STERILE );
        cbm.set_flag( flag_NO_PACKED );
        cbm.faults.emplace( fault_bionic_salvaged );
        here.add_item( pos(), cbm );
    } else {
        get_event_bus().send<event_type::fails_to_remove_cbm>( getID(), bio.id );
        // for chance_of_success calculation, shift skill down to a float between ~0.4 - 30
        float adjusted_skill = static_cast<float>( pl_skill ) - std::min( static_cast<float>( 40 ),
                               static_cast<float>( pl_skill ) - static_cast<float>( pl_skill ) / static_cast<float>
                               ( 10.0 ) );
        bionics_uninstall_failure( difficulty, success, adjusted_skill );

    }
    here.invalidate_map_cache( here.get_abs_sub().z() );
}

bool Character::uninstall_bionic( const bionic &bio, monster &installer, Character &patient,
                                  float adjusted_skill )
{
    viewer &player_view = get_player_view();
    if( installer.ammo[itype_anesthetic] <= 0 ) {
        add_msg_if_player_sees( installer, _( "The %s's anesthesia kit looks empty." ), installer.name() );
        return false;
    }

    item bionic_to_uninstall = item( bio.id.str(), calendar::turn_zero );
    const itype *itemtype = bionic_to_uninstall.type;
    int difficulty = itemtype->bionic->difficulty;
    int chance_of_success = bionic_manip_cos( adjusted_skill, difficulty + 2 );
    int success = chance_of_success - rng( 1, 100 );

    const time_duration duration = difficulty * 20_minutes;
    // don't stack up the effect
    if( !installer.has_effect( effect_operating ) ) {
        installer.add_effect( effect_operating, duration + 5_turns );
    }

    if( patient.is_avatar() ) {
        add_msg( m_bad,
                 _( "You feel a tiny pricking sensation in your right arm, and lose all sensation before abruptly blacking out." ) );
    } else {
        add_msg_if_player_sees( installer, m_bad,
                                _( "The %1$s gently inserts a syringe into %2$s's arm and starts injecting something while holding them down." ),
                                installer.name(), patient.disp_name() );
    }

    installer.ammo[itype_anesthetic] -= 1;

    patient.add_effect( effect_narcosis, duration );
    patient.add_effect( effect_sleep, duration );

    if( patient.is_avatar() ) {
        add_msg( _( "You fall asleep and %1$s starts operating." ), installer.disp_name() );
    } else {
        add_msg_if_player_sees( patient, _( "%1$s falls asleep and %2$s starts operating." ),
                                patient.disp_name(), installer.disp_name() );
    }

    if( success > 0 ) {

        if( patient.is_avatar() ) {
            add_msg( m_neutral, _( "Your parts are jiggled back into their familiar places." ) );
            add_msg( m_mixed, _( "Successfully removed %s." ), bio.info().name );
        } else if( patient.is_npc() && player_view.sees( patient ) ) {
            add_msg( m_neutral, _( "%s's parts are jiggled back into their familiar places." ),
                     patient.disp_name() );
            add_msg( m_mixed, _( "Successfully removed %s." ), bio.info().name );
        }

        item cbm( "burnt_out_bionic" );
        if( item::type_is_defined( bio.info().itype() ) ) {
            cbm = bionic_to_uninstall;
        }
        patient.remove_bionic( bio );
        cbm.set_flag( flag_FILTHY );
        cbm.set_flag( flag_NO_STERILE );
        cbm.set_flag( flag_NO_PACKED );
        cbm.faults.emplace( fault_bionic_salvaged );
        get_map().add_item( patient.pos(), cbm );
    } else {
        bionics_uninstall_failure( installer, patient, difficulty, success, adjusted_skill );
    }

    return false;
}

ret_val<void> Character::is_installable( const item *it, const bool by_autodoc ) const
{
    const itype *itemtype = it->type;
    const bionic_id &bid = itemtype->bionic->id;

    const auto has_trait_lambda = [this]( const trait_id & candidate ) {
        return has_trait( candidate );
    };

    if( it->has_flag( flag_FILTHY ) ) {
        // NOLINTNEXTLINE(cata-text-style): single space after the period for symmetry
        const std::string msg = by_autodoc ? _( "/!\\ CBM is highly contaminated. /!\\" ) :
                                _( "CBM is filthy." );
        return ret_val<void>::make_failure( msg );
    } else if( it->has_flag( flag_NO_STERILE ) ) {
        const std::string msg = by_autodoc ?
                                // NOLINTNEXTLINE(cata-text-style): single space after the period for symmetry
                                _( "/!\\ CBM is not sterile. /!\\ Please use autoclave to sterilize." ) :
                                _( "CBM is not sterile." );
        return ret_val<void>::make_failure( msg );
    } else if( it->has_fault( fault_bionic_salvaged ) ) {
        return ret_val<void>::make_failure( _( "CBM already deployed.  Please reset to factory state." ) );
    } else if( has_bionic( bid ) && !bid->dupes_allowed ) {
        return ret_val<void>::make_failure( _( "CBM is already installed." ) );
    } else if( !can_install_cbm_on_bp( get_occupied_bodyparts( bid ) ) ) {
        return ret_val<void>::make_failure( _( "CBM not compatible with patient's body." ) );
    } else if( std::any_of( bid->mutation_conflicts.begin(), bid->mutation_conflicts.end(),
                            has_trait_lambda ) ) {
        return ret_val<void>::make_failure( _( "CBM not compatible with patient's body." ) );
    } else if( bid->upgraded_bionic &&
               !has_bionic( bid->upgraded_bionic ) &&
               it->is_upgrade() ) {
        return ret_val<void>::make_failure( _( "No base version installed." ) );
    } else if( bid->required_bionic &&
               !has_bionic( bid->required_bionic ) ) {
        return ret_val<void>::make_failure( _( "CBM requires prior installation of %s." ),
                                            bid->required_bionic.obj().name );
    } else if( std::any_of( bid->available_upgrades.begin(),
                            bid->available_upgrades.end(),
    [this]( const bionic_id & b ) {
    return has_bionic( b );
    } ) ) {
        return ret_val<void>::make_failure( _( "Superior version installed." ) );
    } else if( is_npc() && !bid->has_flag( json_flag_BIONIC_NPC_USABLE ) ) {
        return ret_val<void>::make_failure( _( "CBM not compatible with patient." ) );
    }

    return ret_val<void>::make_success( std::string() );
}

bool Character::can_install_bionics( const itype &type, Character &installer, bool autodoc,
                                     int skill_level ) const
{
    if( !type.bionic ) {
        debugmsg( "Tried to install NULL bionic" );
        return false;
    }
    if( has_trait( trait_DEBUG_BIONICS ) ) {
        return true;
    }
    if( is_mounted() ) {
        return false;
    }

    const bionic_id &bioid = type.bionic->id;
    const int difficult = type.bionic->difficulty;

    // if we're doing self install
    if( !autodoc && installer.is_avatar() ) {
        return installer.has_enough_anesth( type ) &&
               installer.has_installation_requirement( bioid );
    }

    int chance_of_success = bionic_success_chance( autodoc, skill_level, difficult, installer );

    std::vector<std::string> conflicting_muts;
    for( const trait_id &mid : bioid->canceled_mutations ) {
        if( has_trait( mid ) ) {
            conflicting_muts.push_back( mutation_name( mid ) );
        }
    }

    if( !conflicting_muts.empty() &&
        !query_yn(
            _( "Installing this bionic will remove the conflicting traits: %s.  Continue anyway?" ),
            enumerate_as_string( conflicting_muts ) ) ) {
        return false;
    }

    const std::map<bodypart_id, int> &issues = bionic_installation_issues( bioid );
    // show all requirements which are not satisfied
    if( !issues.empty() ) {
        std::string detailed_info;
        for( const std::pair<const bodypart_id, int> &elem : issues ) {
            //~ <Body part name>: <number of slots> more slot(s) needed.
            detailed_info += string_format( _( "\n%s: %i more slot(s) needed." ),
                                            body_part_name_as_heading( elem.first, 1 ),
                                            elem.second );
        }
        popup( _( "Not enough space for bionic installation!%s" ), detailed_info );
        return false;
    }

    Character &player_character = get_player_character();
    if( chance_of_success >= 100 ) {
        if( !player_character.query_yn(
                _( "Are you sure you wish to install the selected bionic?" ),
                100 - chance_of_success ) ) {
            return false;
        }
    } else {
        if( !player_character.query_yn(
                _( "WARNING: %i percent chance of failure that may result in damage, pain, or a faulty installation!  Continue anyway?" ),
                ( 100 - chance_of_success ) ) ) {
            return false;
        }
    }

    return true;
}

float Character::env_surgery_bonus( int radius ) const
{
    float bonus = 1.0f;
    map &here = get_map();
    for( const tripoint &cell : here.points_in_radius( pos(), radius ) ) {
        if( here.furn( cell )->surgery_skill_multiplier ) {
            bonus = std::max( bonus, *here.furn( cell )->surgery_skill_multiplier );
        }
    }
    return bonus;
}

bool Character::install_bionics( const itype &type, Character &installer, bool autodoc,
                                 int skill_level )
{
    if( !type.bionic ) {
        debugmsg( "Tried to install NULL bionic" );
        return false;
    }

    const bionic_id &bioid = type.bionic->id;
    const bionic_id &upbioid = bioid->upgraded_bionic;
    const int difficulty = type.bionic->difficulty;
    int pl_skill = installer.bionics_pl_skill( autodoc, skill_level );
    int chance_of_success = bionic_success_chance( autodoc, skill_level, difficulty, installer );

    // Practice skills only if conducting manual installation
    if( !autodoc ) {
        installer.practice( skill_electronics, static_cast<int>( ( 100 - chance_of_success ) * 1.5 ) );
        installer.practice( skill_firstaid, static_cast<int>( ( 100 - chance_of_success ) * 1.0 ) );
        installer.practice( skill_mechanics, static_cast<int>( ( 100 - chance_of_success ) * 0.5 ) );
    }

    bionic_uid upbio_uid = 0;
    // TODO: Let the player pick a bionic to upgrade (if dupes exist)
    if( std::optional<bionic *> upbio = find_bionic_by_type( upbioid ) ) {
        upbio_uid = ( *upbio )->get_uid();
    }

    int success = chance_of_success - rng( 0, 99 );
    if( installer.has_trait( trait_DEBUG_BIONICS ) ) {
        perform_install( bioid, upbio_uid, difficulty, success, pl_skill, "NOT_MED",
                         bioid->canceled_mutations, pos() );
        return true;
    }
    assign_activity( ACT_OPERATION, to_moves<int>( difficulty * 20_minutes ) );
    activity.values.push_back( difficulty );
    activity.values.push_back( success );
    activity.values.push_back( upbio_uid );
    activity.values.push_back( pl_skill );
    activity.str_values.emplace_back( "install" );
    activity.str_values.push_back( bioid.str() );

    if( installer.has_trait( trait_PROF_MED ) || installer.has_trait( trait_PROF_AUTODOC ) ) {
        activity.str_values.push_back( installer.disp_name( true ) );
    } else {
        activity.str_values.emplace_back( "NOT_MED" );
    }
    if( autodoc ) {
        activity.str_values.emplace_back( "true" );
    } else {
        activity.str_values.emplace_back( "false" );
    }
    for( const std::pair<const bodypart_str_id, size_t> &elem : bioid->occupied_bodyparts ) {
        add_effect( effect_under_operation, difficulty * 20_minutes, elem.first.id(), true, difficulty );
    }

    return true;
}

void Character::perform_install( const bionic_id &bid, bionic_uid upbio_uid, int difficulty,
                                 int success, int pl_skill, const std::string &installer_name,
                                 const std::vector<trait_id> &trait_to_rem, const tripoint &patient_pos )
{
    if( success > 0 ) {
        get_event_bus().send<event_type::installs_cbm>( getID(), bid );
        if( upbio_uid ) {
            if( std::optional<bionic *> upbio = find_bionic_by_uid( upbio_uid ) ) {
                const std::string bio_name = ( *upbio )->id->name.translated();
                remove_bionic( **upbio );
                //~ %1$s - name of the bionic to be upgraded (inferior), %2$s - name of the upgraded bionic (superior).
                add_msg( m_good, _( "Successfully upgraded %1$s to %2$s." ), bio_name, bid.obj().name );
            } else {
                debugmsg( "Couldn't find bionic with UID %d to upgrade", upbio_uid );
            }
        } else {
            //~ %s - name of the bionic.
            add_msg( m_good, _( "Successfully installed %s." ), bid.obj().name );
        }

        add_bionic( bid );

        for( const trait_id &tid : trait_to_rem ) {
            if( has_trait( tid ) ) {
                remove_mutation( tid );
            }
        }

    } else {
        get_event_bus().send<event_type::fails_to_install_cbm>( getID(), bid );

        // for chance_of_success calculation, shift skill down to a float between ~0.4 - 30
        float adjusted_skill = static_cast<float>( pl_skill ) - std::min( static_cast<float>( 40 ),
                               static_cast<float>( pl_skill ) - static_cast<float>( pl_skill ) / static_cast<float>
                               ( 10.0 ) );
        bionics_install_failure( bid, installer_name, difficulty, success, adjusted_skill, patient_pos );
    }
    map &here = get_map();
    here.invalidate_map_cache( here.get_abs_sub().z() );
}

void Character::bionics_install_failure( const bionic_id &bid, const std::string &installer,
        int difficulty, int success, float adjusted_skill, const tripoint &patient_pos )
{
    // "success" should be passed in as a negative integer representing how far off we
    // were for a successful install.  We use this to determine consequences for failing.
    success = std::abs( success );

    // failure level is decided by how far off the character was from a successful install, and
    // this is scaled up or down by the ratio of difficulty/skill.  At high skill levels (or low
    // difficulties), only minor consequences occur.  At low skill levels, severe consequences
    // are more likely.
    int failure_level = static_cast<int>( std::sqrt( success * 4.0 * difficulty / adjusted_skill ) );
    int fail_type = failure_level > 5 ? 5 : failure_level;
    bool drop_cbm = false;
    add_msg( m_neutral, _( "The installation is a failure." ) );

    if( installer != "NOT_MED" ) {
        //~"Complications" is USian medical-speak for "unintended damage from a medical procedure".
        add_msg( m_neutral, _( "%s training helps to minimize the complications." ),
                 installer );
        // In addition to the bonus, medical residents know enough OR protocol to avoid botching.
        // Take MD and be immune to faulty bionics.
        if( fail_type > 3 ) {
            fail_type = rng( 1, 3 );
        }
    }
    if( fail_type <= 0 ) {
        add_msg( m_neutral, _( "The installation fails without incident." ) );
        drop_cbm = true;
    } else {
        std::set<bodypart_id> bp_hurt;
        switch( fail_type ) {

            case 1:
                if( !has_flag( json_flag_PAIN_IMMUNE ) ) {
                    add_msg_if_player( m_bad, _( "It really hurts!" ) );
                    mod_pain( rng( 10, 30 ) );
                }
                drop_cbm = true;
                break;

            case 2:
            case 3: {
                add_msg( m_bad, _( "The installation is faulty!" ) );
                std::vector<bionic_id> valid;
                std::copy_if( begin( faulty_bionics ), end( faulty_bionics ), std::back_inserter( valid ),
                [&]( const bionic_id & id ) {
                    return !has_bionic( id ) && !id->dupes_allowed;
                } );

                if( valid.empty() ) {
                    // No unique faulty bionics left. Pick one that allows dupes
                    std::copy_if( begin( faulty_bionics ), end( faulty_bionics ), std::back_inserter( valid ),
                    [&]( const bionic_id & id ) {
                        return id->dupes_allowed;
                    } );
                }

                if( valid.empty() ) {
                    // Shouldn't happen unless no faulty bionic has allow_dupes=true
                    debugmsg( "Couldn't find any faulty bionics to install!" );
                } else {
                    const bionic_id &id = random_entry( valid );
                    add_bionic( id );
                    get_event_bus().send<event_type::installs_faulty_cbm>( getID(), id );
                }

                break;
            }
            case 4:
            case 5: {
                for( const bodypart_id &bp : get_all_body_parts() ) {
                    if( has_effect( effect_under_operation, bp.id() ) ) {
                        if( bp_hurt.count( bp->main_part ) > 0 ) {
                            continue;
                        }
                        bp_hurt.emplace( bp->main_part );

                        apply_damage( this, bp, rng( 25, 50 ), true );
                        roll_critical_bionics_failure( bp );

                        add_msg_player_or_npc( m_bad, _( "Your %s is damaged." ), _( "<npcname>'s %s is damaged." ),
                                               body_part_name_accusative( bp ) );
                    }
                }
                drop_cbm = true;
                break;
            }
        }
    }
    if( drop_cbm ) {
        item cbm( bid.c_str() );
        cbm.set_flag( flag_NO_STERILE );
        cbm.set_flag( flag_NO_PACKED );
        cbm.faults.emplace( fault_bionic_salvaged );
        get_map().add_item( patient_pos, cbm );
    }
}

std::string list_occupied_bps( const bionic_id &bio_id, const std::string &intro,
                               const bool each_bp_on_new_line )
{
    if( bio_id->occupied_bodyparts.empty() ) {
        return "";
    }
    std::string desc = intro;
    for( const std::pair<const bodypart_str_id, size_t> &elem : bio_id->occupied_bodyparts ) {
        desc += ( each_bp_on_new_line ? "\n" : " " );
        //~ <Bodypart name> (<number of occupied slots> slots);
        desc += string_format( _( "%s (%i slots);" ),
                               body_part_name_as_heading( elem.first.id(), 1 ),
                               elem.second );
    }
    return desc;
}

std::vector<bionic_id> Character::get_bionics() const
{
    std::vector<bionic_id> result;
    for( const bionic &b : *my_bionics ) {
        result.push_back( b.id );
    }
    return result;
}

bool Character::has_bionic( const bionic_id &b ) const
{
    for( const bionic_id &bid : get_bionics() ) {
        if( bid == b ) {
            return true;
        }
    }
    return false;
}

bool Character::has_active_bionic( const bionic_id &b ) const
{
    for( const bionic &i : *my_bionics ) {
        if( i.id == b ) {
            if( i.is_safe_fuel_on() &&
                get_power_level() + 1_kJ > get_max_power_level() * std::min( 1.0f, i.get_safe_fuel_thresh() ) ) {
                // Inactive due to fuel treshold
                return false;
            }

            return ( i.powered && i.incapacitated_time == 0_turns );
        }
    }
    return false;
}

bool Character::has_any_bionic() const
{
    return !get_bionics().empty();
}

int Character::get_used_bionics_slots( const bodypart_id &bp ) const
{
    int used_slots = 0;
    for( const bionic_id &bid : get_bionics() ) {
        auto search = bid->occupied_bodyparts.find( bp.id() );
        if( search != bid->occupied_bodyparts.end() ) {
            used_slots += search->second;
        }
    }

    return used_slots;
}

std::map<bodypart_id, int> Character::bionic_installation_issues( const bionic_id &bioid ) const
{
    std::map<bodypart_id, int> issues;
    if( !get_option < bool >( "CBM_SLOTS_ENABLED" ) ) {
        return issues;
    }
    for( const std::pair<const string_id<body_part_type>, size_t> &elem : bioid->occupied_bodyparts ) {
        const int lacked_slots = elem.second - get_free_bionics_slots( elem.first );
        if( lacked_slots > 0 ) {
            issues.emplace( elem.first, lacked_slots );
        }
    }
    return issues;
}

int Character::get_total_bionics_slots( const bodypart_id &bp ) const
{
    const bodypart_str_id &id = bp.id();
    int mut_bio_slots = 0;
    for( const trait_id &mut : get_mutations() ) {
        mut_bio_slots += mut->bionic_slot_bonus( id );
    }
    return bp->bionic_slots() + mut_bio_slots;
}

int Character::get_free_bionics_slots( const bodypart_id &bp ) const
{
    return get_total_bionics_slots( bp ) - get_used_bionics_slots( bp );
}

bionic_uid Character::add_bionic( const bionic_id &b, bionic_uid parent_uid,
                                  bool suppress_debug )
{
    if( has_bionic( b ) && !b->dupes_allowed ) {
        if( !suppress_debug ) {
            debugmsg( "Tried to install bionic %s that is already installed!", b.c_str() );
        }
        return 0;
    }

    const units::energy pow_up = b->capacity;
    if( pow_up > 0_J ) {
        add_msg_if_player( m_good, _( "Increased storage capacity by %i." ),
                           units::to_kilojoule( pow_up ) );
    }

    bionic_uid bio_uid = generate_bionic_uid();

    my_bionics->emplace_back( b, get_free_invlet( *this ), bio_uid, parent_uid );
    bionic &bio = my_bionics->back();
    if( bio.id->activated_on_install ) {
        activate_bionic( bio );
    }

    for( const bionic_id &inc_bid : b->included_bionics ) {
        add_bionic( inc_bid, bio_uid, suppress_debug );
    }

    for( const std::pair<const spell_id, int> &spell_pair : b->learned_spells ) {
        const spell_id learned_spell = spell_pair.first;
        if( learned_spell->spell_class != trait_NONE ) {
            const trait_id spell_class = learned_spell->spell_class;
            // spells you learn from a bionic overwrite the opposite spell class.
            // for best UX, include those spell classes in "canceled_mutations"
            if( !has_trait( spell_class ) ) {
                set_mutation( spell_class );
                on_mutation_gain( spell_class );
                add_msg_if_player( spell_class->desc() );
            }
        }
        if( !magic->knows_spell( learned_spell ) ) {
            magic->learn_spell( learned_spell, *this, true );
        }
        spell &known_spell = magic->get_spell( learned_spell );
        // spells you learn from installing a bionic upgrade spells you know if they are the same
        if( known_spell.get_level() < spell_pair.second ) {
            known_spell.set_level( *this, spell_pair.second );
        }
    }

    for( const proficiency_id &learned : b->proficiencies ) {
        add_proficiency( learned );
    }

    for( const itype_id &pseudo : b->passive_pseudo_items ) {
        item tmparmor( pseudo );
        if( tmparmor.has_flag( flag_INTEGRATED ) ) {
            wear_item( tmparmor, false );
        }
    }
    bio_flag_cache.clear();
    invalidate_pseudo_items();
    update_bionic_power_capacity();

    calc_encumbrance();
    recalc_sight_limits();
    if( is_avatar() && has_flag( json_flag_ENHANCED_VISION ) ) {
        // enhanced vision counts as optics for overmap sight range.
        g->update_overmap_seen();
    }
    if( !b->enchantments.empty() ) {
        recalculate_enchantment_cache();
    }
    effect_on_conditions::process_reactivate( *this );

    return bio_uid;
}

std::optional<bionic *> Character::find_bionic_by_type( const bionic_id &b ) const
{
    for( bionic &bio : *my_bionics ) {
        if( bio.id == b ) {
            return &bio;
        }
    }
    return std::nullopt;
}

std::optional<bionic *> Character::find_bionic_by_uid( bionic_uid bio_uid ) const
{
    if( !bio_uid ) {
        return std::nullopt;
    }

    for( bionic &bio : *my_bionics ) {
        if( bio.get_uid() == bio_uid ) {
            return &bio;
        }
    }
    return std::nullopt;
}

void Character::remove_bionic( const bionic &bio )
{
    const bionic_uid bio_uid = bio.get_uid();
    std::optional<bionic *> bio_opt = find_bionic_by_uid( bio_uid );
    if( !bio_opt ) {
        debugmsg( "Tried to uninstall non-existent bionic with UID %d", bio_uid );
        return;
    }

    bionic_collection new_my_bionics;
    // any spells you should not forget due to still having a bionic installed that has it.
    std::set<spell_id> cbm_spells;
    for( bionic &i : *my_bionics ) {
        // Linked bionics: if either is removed, the other is removed as well.
        if( i.get_uid() == bio_uid || i.get_parent_uid() == bio_uid ) {
            continue;
        }

        for( const std::pair<const spell_id, int> &spell_pair : i.id->learned_spells ) {
            cbm_spells.emplace( spell_pair.first );
        }

        new_my_bionics.push_back( i );
    }

    // any spells you learn from installing a bionic you forget.
    for( const std::pair<const spell_id, int> &spell_pair : bio.id->learned_spells ) {
        if( cbm_spells.count( spell_pair.first ) == 0 ) {
            magic->forget_spell( spell_pair.first );
        }
    }

    for( const proficiency_id &lost : bio.id->proficiencies ) {
        lose_proficiency( lost );
    }

    for( const itype_id &popped_armor : bio.id->passive_pseudo_items ) {
        remove_worn_items_with( [&]( item & armor ) {
            return armor.typeId() == popped_armor;
        } );
    }

    const bool has_enchantments = !bio.id->enchantments.empty();
    *my_bionics = new_my_bionics;
    bio_flag_cache.clear();
    update_last_bionic_uid();
    invalidate_pseudo_items();
    update_bionic_power_capacity();
    calc_encumbrance();
    recalc_sight_limits();
    if( has_enchantments ) {
        recalculate_enchantment_cache();
    }
    effect_on_conditions::process_reactivate( *this );
}

int Character::num_bionics() const
{
    return my_bionics->size();
}

bionic &Character::bionic_at_index( int i )
{
    return ( *my_bionics )[i];
}

void Character::clear_bionics()
{
    set_max_power_level_modifier( 0_kJ );
    while( !my_bionics->empty() ) {
        remove_bionic( my_bionics->front() );
    }
}

void reset_bionics()
{
    bionic_data::migrations.clear();
    faulty_bionics.clear();
    bionic_factory.reset();
}

void bionic::set_flag( const std::string &flag )
{
    bionic_tags.insert( flag );
}

void bionic::remove_flag( const std::string &flag )
{
    bionic_tags.erase( flag );
}

bool bionic::has_flag( const std::string &flag ) const
{
    return bionic_tags.find( flag ) != bionic_tags.end();
}

int bionic::get_quality( const quality_id &quality ) const
{
    if( weapon.typeId().is_empty() ) {
        return INT_MIN;
    }

    return weapon.get_quality( quality );
}

bool bionic::has_weapon() const
{
    return !weapon.typeId().is_empty() && !weapon.typeId().is_null();
}

bool bionic::can_install_weapon() const
{
    return !id->installable_weapon_flags.empty();
}

bool bionic::can_install_weapon( const item &new_weapon ) const
{
    return !id->installable_weapon_flags.empty() &&
           new_weapon.has_any_flag( id->installable_weapon_flags );
}

item bionic::get_weapon() const
{
    return weapon;
}

void bionic::set_weapon( const item &new_weapon )
{
    weapon = new_weapon;
    update_weapon_flags();
}

bool bionic::install_weapon( const item &new_weapon, bool skip_checks )
{
    if( powered ) {
        debugmsg( "Tried to install a weapon on powered bionic \"%s\" with UID %i.",
                  id.str(), uid );
        return false;
    }

    if( !skip_checks ) {
        if( !can_install_weapon( new_weapon ) ) {
            debugmsg( "Tried to install a weapon with incompatible flags on bionic \"%s\" with UID %i.",
                      id.str(), uid );
            return false;
        }

        if( has_weapon() ) {
            debugmsg( "Tried to install a weapon on bionic \"%s\" with UID %i that already has a weapon installed.",
                      id.str(), uid );
            return false;
        }
    }

    set_weapon( new_weapon );

    return true;
}

std::optional<item> bionic::uninstall_weapon()
{
    if( !has_weapon() ) {
        debugmsg( "Tried to uninstall a weapon on bionic \"%s\" with UID %i that doesn't have a weapon installed.",
                  id.str(), uid );
        return std::nullopt;
    }

    if( id->installable_weapon_flags.empty() ) {
        debugmsg( "Tried to uinstall a weapon from non-dynamic bionic \"%s\" with UID %i.", id.str(),
                  uid );
        return std::nullopt;
    }
    std::optional<item> old_item = get_weapon();
    weapon = item();

    if( old_item && !old_item->is_null() ) {
        old_item->unset_flag( flag_USES_BIONIC_POWER );
        old_item->unset_flag( flag_NO_UNWIELD );
    }

    return old_item;
}

std::vector<const item *> bionic::get_available_pseudo_items( bool include_weapon ) const
{
    std::vector<const item *> ret;
    ret.reserve( passive_pseudo_items.size() );
    for( const item &pseudo : passive_pseudo_items ) {
        ret.push_back( &pseudo );
    }

    if( powered && info().has_flag( flag_BIONIC_TOGGLED ) ) {
        for( const item &pseudo : toggled_pseudo_items ) {
            ret.push_back( &pseudo );
        }

        if( include_weapon && has_weapon() ) {
            ret.push_back( &weapon );
        }
    }

    return ret;
}

bool bionic::is_this_fuel_powered( const material_id &this_fuel ) const
{
    const std::vector<material_id> fuel_op = info().fuel_opts;
    return std::find( fuel_op.begin(), fuel_op.end(), this_fuel ) != fuel_op.end();
}

void bionic::toggle_safe_fuel_mod()
{
    if( !info().fuel_opts.empty() || info().is_remote_fueled ) {
        uilist tmenu;
        tmenu.text = _( "Stop power generation at" );
        tmenu.addentry( 1, true, '1', _( "100 %%" ) );
        tmenu.addentry( 2, true, '2', _( "90 %%" ) );
        tmenu.addentry( 3, true, '3', _( "70 %%" ) );
        tmenu.addentry( 4, true, '4', _( "50 %%" ) );
        tmenu.addentry( 5, true, '5', _( "30 %%" ) );
        tmenu.addentry( 6, true, '6', _( "10 %%" ) );
        tmenu.addentry( 7, true, 'd', _( "Disabled" ) );
        tmenu.query();

        switch( tmenu.ret ) {
            case 1:
                set_safe_fuel_thresh( 1.0f );
                break;
            case 2:
                set_safe_fuel_thresh( 0.90f );
                break;
            case 3:
                set_safe_fuel_thresh( 0.70f );
                break;
            case 4:
                set_safe_fuel_thresh( 0.50f );
                break;
            case 5:
                set_safe_fuel_thresh( 0.30f );
                break;
            case 6:
                set_safe_fuel_thresh( 0.10f );
                break;
            case 7:
                set_safe_fuel_thresh( -1.0f );
                break;
            default:
                break;
        }
    }
}

float bionic::get_safe_fuel_thresh() const
{
    return safe_fuel_threshold;
}

bool bionic::is_safe_fuel_on() const
{
    return info().has_flag( json_flag_BIONIC_POWER_SOURCE ) && get_safe_fuel_thresh() > -1.f;
}

void bionic::set_safe_fuel_thresh( float val )
{
    safe_fuel_threshold = val;
}

void bionic::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "id", id );
    json.member( "invlet", static_cast<int>( invlet ) );
    json.member( "powered", powered );
    json.member( "charge", charge_timer );
    json.member( "bionic_tags", bionic_tags );
    json.member( "bionic_uid", uid );
    json.member( "parent_uid", parent_uid );
    if( incapacitated_time > 0_turns ) {
        json.member( "incapacitated_time", incapacitated_time );
    }
    if( is_safe_fuel_on() ) {
        json.member( "safe_fuel_threshold", safe_fuel_threshold );
    }
    json.member( "show_sprite", show_sprite );

    if( has_weapon() ) {
        json.member( "weapon", weapon );
    }

    json.end_object();
}

static bionic_id migrate_bionic_id( const bionic_id &original )
{
    bionic_id bid = original;

    // limit up to 10 migrations per bionic (guard in case of accidental loops)
    for( int i = 0; i < 10; i++ ) {
        const auto &migration_it = bionic_data::migrations.find( bid );
        if( migration_it == bionic_data::migrations.cend() ) {
            break;
        }
        bid = migration_it->second;
    }

    if( bid != original ) {
        DebugLog( D_WARNING, D_MAIN ) << "Migrating bionic with id " << original.str() << " to " <<
                                      bid.str();
    }

    return bid;
}

void bionic::deserialize( const JsonObject &jo )
{
    id = migrate_bionic_id( bionic_id( jo.get_string( "id" ) ) );
    if( !id.is_valid() ) {
        debugmsg( "deserialized bionic id '%s' doesn't exist and has no migration", id.str() );
        id = bionic_id::NULL_ID(); // remove it
    }
    if( id.is_null() ) {
        jo.allow_omitted_members();
        return; // obsoleted bionics migrated to bionic_id::NULL_ID ids
    }
    invlet = jo.get_int( "invlet" );
    powered = jo.get_bool( "powered" );

    //Remove After 0.G
    if( jo.has_int( "charge" ) ) {
        charge_timer = time_duration::from_turns( jo.get_int( "charge" ) );
    } else {
        jo.read( "charge_timer", charge_timer );
    }

    if( jo.has_int( "incapacitated_time" ) ) {
        incapacitated_time = 1_turns * jo.get_int( "incapacitated_time" );
    }
    if( jo.has_float( "safe_fuel_threshold" ) ) {
        safe_fuel_threshold = jo.get_float( "safe_fuel_threshold" );
    }
    if( jo.has_bool( "show_sprite" ) ) {
        show_sprite = jo.get_bool( "show_sprite" );
    }
    if( jo.has_array( "bionic_tags" ) ) {
        for( const std::string line : jo.get_array( "bionic_tags" ) ) {
            bionic_tags.insert( line );
        }
    }

    if( jo.has_int( "bionic_uid" ) ) {
        uid = jo.get_int( "bionic_uid" );
    }

    if( jo.has_int( "parent_uid" ) ) {
        parent_uid = jo.get_int( "parent_uid" );
    }

    if( jo.has_member( "weapon" ) ) {
        jo.read( "weapon", weapon, true );
    }

    // Suppress errors when loading old games
    // TODO: add old ammo to new weapon?
    if( jo.has_string( "ammo_loaded" ) ) {
        jo.get_string( "ammo_loaded" );
    }
    if( jo.has_int( "ammo_count" ) ) {
        jo.get_int( "ammo_count" );
    }

    initialize_pseudo_items();
}

std::vector<bionic_id> bionics_cancelling_trait( const std::vector<bionic_id> &bios,
        const trait_id &tid )
{
    // Vector of bionics to return
    std::vector<bionic_id> bionics_cancelling;

    // Search through the vector of of bionics, and see if the trait is cancelled by one of them
    for( const bionic_id &bid : bios ) {
        for( const trait_id &trait : bid->canceled_mutations ) {
            if( trait == tid ) {
                bionics_cancelling.emplace_back( bid );
            }
        }
    }

    return bionics_cancelling;
}

void Character::introduce_into_anesthesia( const time_duration &duration, Character &installer,
        bool needs_anesthesia )   //used by the Autodoc
{
    if( installer.has_trait( trait_DEBUG_BIONICS ) ) {
        installer.add_msg_if_player( m_info,
                                     _( "You tell the pain to bug off and proceed with the operation." ) );
        return;
    }
    installer.add_msg_player_or_npc( m_info,
                                     _( "You set up the operation step-by-step, configuring the Autodoc to manipulate a CBM." ),
                                     _( "<npcname> sets up the operation, configuring the Autodoc to manipulate a CBM." ) );

    add_msg_player_or_npc( m_info,
                           _( "You settle into position, sliding your right wrist into the couch's strap." ),
                           _( "<npcname> settles into position, sliding their wrist into the couch's strap." ) );
    if( needs_anesthesia ) {
        //post-threshold medical mutants do not fear operations.
        if( has_trait( trait_THRESH_MEDICAL ) ) {
            add_msg_if_player( m_mixed,
                               _( "You feel excited as the operation starts." ) );
        }

        add_msg_if_player( m_mixed,
                           _( "You feel a tiny pricking sensation in your right arm, and lose all sensation before abruptly blacking out." ) );

        //post-threshold medical mutants with Deadened don't need anesthesia due to their inability to feel pain
    } else {
        //post-threshold medical mutants do not fear operations.
        if( has_trait( trait_THRESH_MEDICAL ) ) {
            add_msg_if_player( m_mixed,
                               _( "You feel excited as the Autodoc slices painlessly into you.  You enjoy the sight of scalpels slicing you apart." ) );
        } else {
            add_msg_if_player( m_mixed,
                               _( "You stay very, very still, focusing intently on an interesting stain on the ceiling, as the Autodoc slices painlessly into you." ) );
        }
    }

    //Pain junkies feel sorry about missed pain from operation.
    if( has_trait( trait_MASOCHIST_MED ) || has_trait( trait_CENOBITE ) ) {
        add_msg_if_player( m_mixed,
                           _( "As your consciousness slips away, you feel regret that you won't be able to enjoy the operation." ) );
    }

    if( has_effect( effect_narcosis ) ) {
        const time_duration remaining_time = get_effect_dur( effect_narcosis );
        if( remaining_time < duration ) {
            const time_duration top_off_time = duration - remaining_time;
            add_effect( effect_narcosis, top_off_time );
            fall_asleep( top_off_time );
        }
    } else {
        add_effect( effect_narcosis, duration );
        fall_asleep( duration );
    }
}

std::vector<bionic_id> Character::get_bionic_fueled_with_muscle() const
{
    std::vector<bionic_id> bionics;

    for( const bionic_id &bid : get_bionics() ) {
        for( const material_id &fuel : bid->fuel_opts ) {
            if( fuel == material_muscle ) {
                bionics.emplace_back( bid );
            }
        }
    }

    return bionics;
}

std::vector<bionic_id> Character::get_fueled_bionics() const
{
    std::vector<bionic_id> bionics;
    for( const bionic_id &bid : get_bionics() ) {
        if( !bid->fuel_opts.empty() ) {
            bionics.emplace_back( bid );
        }
    }
    return bionics;
}

bionic_id Character::get_remote_fueled_bionic() const
{
    for( const bionic_id &bid : get_bionics() ) {
        if( bid->is_remote_fueled ) {
            return bid;
        }
    }
    return bionic_id();
}

std::vector<item *> Character::get_bionic_fuels( const bionic_id &bio )
{
    std::vector<item *> stored_fuels;

    for( item_location it : top_items_loc() ) {
        if( !it->has_flag( flag_BIONIC_FUEL_SOURCE ) ) {
            continue;
        }
        for( const material_id &mat : bio->fuel_opts ) {
            if( it->ammo_remaining() && it->first_ammo().made_of( mat ) ) {
                // Ammo from magazines
                stored_fuels.emplace_back( it.get_item() );
            } else {
                for( item *cit : it->all_items_top() ) {
                    // Fuel from containers
                    if( cit->made_of( mat ) ) {
                        stored_fuels.emplace_back( it.get_item() );
                        break;
                    }
                }
            }
        }
    }

    return stored_fuels;
}

std::vector<item *> Character::get_cable_ups()
{
    std::vector<item *> stored_fuels;

    int n = cache_get_items_with( flag_CABLE_SPOOL, []( const item & it ) {
        return it.link && it.link->has_states( link_state::ups, link_state::bio_cable );
    } ).size();
    if( n == 0 ) {
        return stored_fuels;
    }

    // There is no way to check which cable is connected to which ups
    // So if there are multiple cables and some of them are only partially connected this may add wrong ups
    for( item_location it : all_items_loc() ) {
        if( it->has_flag( flag_IS_UPS ) && it->get_var( "cable" ) == "plugged_in" &&
            it->ammo_remaining() ) {
            stored_fuels.emplace_back( it.get_item() );
            n--;
        }
        if( n == 0 ) {
            break;
        }
    }

    if( n > 0 && weapon.has_flag( flag_IS_UPS ) && weapon.get_var( "cable" ) == "plugged_in" &&
        weapon.ammo_remaining() ) {
        stored_fuels.emplace_back( &weapon.first_ammo() );
    }

    return stored_fuels;
}

std::vector<item *> Character::get_cable_solar()
{
    std::vector<item *> solar_sources;

    int n = cache_get_items_with( flag_CABLE_SPOOL, []( const item & it ) {
        return it.link && it.link->has_states( link_state::solarpack, link_state::bio_cable );
    } ).size();
    if( n == 0 ) {
        return solar_sources;
    }

    // There is no way to check which cable is connected to which solar pack
    // So if there are multiple cables and some of them are only partially connected this may add wrong solar pack
    for( item_location it : all_items_loc() ) {
        if( it->has_flag( flag_SOLARPACK_ON ) && it->get_var( "cable" ) == "plugged_in" ) {
            solar_sources.emplace_back( it.get_item() );
            n--;
        }
        if( n == 0 ) {
            break;
        }
    }

    if( n > 0 && weapon.has_flag( flag_SOLARPACK_ON ) && weapon.get_var( "cable" ) == "plugged_in" ) {
        solar_sources.emplace_back( &weapon );
    }

    return solar_sources;
}

std::vector<vehicle *> Character::get_cable_vehicle() const
{
    std::vector<vehicle *> remote_vehicles;

    std::vector<const item *> cables = cache_get_items_with( flag_CABLE_SPOOL,
    []( const item & it ) {
        return it.link && it.link->has_state( link_state::bio_cable ) &&
               ( it.link->has_state( link_state::vehicle_battery ) ||
                 it.link->has_state( link_state::vehicle_port ) );
    } );
    int n = cables.size();
    if( n == 0 ) {
        return remote_vehicles;
    }

    map &here = get_map();

    for( const item *cable : cables ) {
        if( cable->link->t_veh_safe ) {
            remote_vehicles.emplace_back( cable->link->t_veh_safe.get() );
        } else {
            const optional_vpart_position vp = here.veh_at( cable->link->t_abs_pos );
            if( vp ) {
                remote_vehicles.emplace_back( &vp->vehicle() );
            }
        }
    }

    return remote_vehicles;
}

int Character::get_mod_stat_from_bionic( const character_stat &Stat ) const
{
    int ret = 0;
    for( const bionic_id &bid : get_bionics() ) {
        const auto St_bn = bid->stat_bonus.find( Stat );
        if( St_bn != bid->stat_bonus.end() ) {
            ret += St_bn->second;
        }
    }
    return ret;
}

bool Character::is_using_bionic_weapon() const
{
    return !!get_weapon_bionic_uid();
}

bionic_uid Character::get_weapon_bionic_uid() const
{
    return weapon_bionic_uid;
}

float Character::bionic_armor_bonus( const bodypart_id &bp, const damage_type_id &dt ) const
{
    float result = 0.0f;
    for( const bionic_id &bid : get_bionics() ) {
        const auto prot = bid->protec.find( bp.id() );
        if( prot != bid->protec.end() ) {
            result += prot->second.type_resist( dt );
        }
    }

    return result;
}

void Character::update_bionic_power_capacity()
{
    max_power_level_cached = 0_kJ;
    for( const bionic_id &bid : get_bionics() ) {
        max_power_level_cached += bid->capacity;
    }
    max_power_level_cached = clamp( max_power_level_cached, 0_kJ, units::energy_max );

    set_power_level( get_power_level() );
}

bionic_uid bionic::get_uid() const
{
    return uid;
}

void bionic::set_uid( bionic_uid new_uid )
{
    uid = new_uid;
}

bool bionic::is_included() const
{
    return !!parent_uid;
}

bionic_uid bionic::get_parent_uid() const
{
    return parent_uid;
}

void bionic::set_parent_uid( bionic_uid new_uid )
{
    parent_uid = new_uid;
}

bionic_uid Character::generate_bionic_uid() const
{
    if( !next_bionic_uid ) {
        update_last_bionic_uid();
    }
    return next_bionic_uid++;
}

void Character::update_last_bionic_uid() const
{
    next_bionic_uid = 0;
    for( bionic &bio : *my_bionics ) {
        if( bio.get_uid() > next_bionic_uid ) {
            next_bionic_uid = bio.get_uid();
        }
    }
    next_bionic_uid++;
}

void Character::force_bionic_deactivation( bionic &bio )
{
    time_duration old_time = bio.incapacitated_time;
    bio.incapacitated_time = 0_turns;
    deactivate_bionic( bio, true );
    bio.powered = false;
    bio.incapacitated_time = old_time;
}
