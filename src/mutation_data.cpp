#include "mutation.h" // IWYU pragma: associated

#include <array>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

#include "assign.h"
#include "bodypart.h"
#include "color.h"
#include "debug.h"
#include "generic_factory.h"
#include "json.h"
#include "memory_fast.h"
#include "string_formatter.h"
#include "string_id.h"
#include "trait_group.h"
#include "translations.h"

using TraitGroupMap =
    std::map<trait_group::Trait_group_tag, shared_ptr_fast<Trait_group>>;
using TraitSet = std::set<trait_id>;
using trait_reader = auto_flags_reader<trait_id>;

TraitSet trait_blacklist;
TraitGroupMap trait_groups;

namespace
{
generic_factory<mutation_branch> trait_factory( "trait" );
} // namespace

std::vector<dream> dreams;
std::map<std::string, std::vector<trait_id> > mutations_category;
std::map<std::string, mutation_category_trait> mutation_category_traits;

template<>
const mutation_branch &string_id<mutation_branch>::obj() const
{
    return trait_factory.obj( *this );
}

template<>
bool string_id<mutation_branch>::is_valid() const
{
    return trait_factory.is_valid( *this );
}

template<>
bool string_id<Trait_group>::is_valid() const
{
    return trait_groups.count( *this );
}

static void extract_mod(
    const JsonObject &j, std::unordered_map<std::pair<bool, std::string>, int, cata::tuple_hash> &data,
    const std::string &mod_type, bool active, const std::string &type_key )
{
    int val = j.get_int( mod_type, 0 );
    if( val != 0 ) {
        data[std::make_pair( active, type_key )] = val;
    }
}

static void load_mutation_mods(
    const JsonObject &jsobj, const std::string &member,
    std::unordered_map<std::pair<bool, std::string>, int, cata::tuple_hash> &mods )
{
    if( jsobj.has_object( member ) ) {
        JsonObject j = jsobj.get_object( member );
        bool active = false;
        if( member == "active_mods" ) {
            active = true;
        }
        //                   json field             type key
        extract_mod( j, mods, "str_mod",     active, "STR" );
        extract_mod( j, mods, "dex_mod",     active, "DEX" );
        extract_mod( j, mods, "per_mod",     active, "PER" );
        extract_mod( j, mods, "int_mod",     active, "INT" );
    }
}

void mutation_category_trait::load( const JsonObject &jsobj )
{
    mutation_category_trait new_category;
    new_category.id = jsobj.get_string( "id" );
    new_category.raw_name = jsobj.get_string( "name" );
    new_category.threshold_mut = trait_id( jsobj.get_string( "threshold_mut" ) );

    new_category.raw_mutagen_message = jsobj.get_string( "mutagen_message" );
    new_category.mutagen_hunger  = jsobj.get_int( "mutagen_hunger", 10 );
    new_category.mutagen_thirst  = jsobj.get_int( "mutagen_thirst", 10 );
    new_category.mutagen_pain    = jsobj.get_int( "mutagen_pain", 0 );
    new_category.mutagen_fatigue = jsobj.get_int( "mutagen_fatigue", 5 );
    new_category.mutagen_morale  = jsobj.get_int( "mutagen_morale", 0 );
    new_category.raw_iv_message = jsobj.get_string( "iv_message" );
    new_category.iv_min_mutations    = jsobj.get_int( "iv_min_mutations", 1 );
    new_category.iv_additional_mutations = jsobj.get_int( "iv_additional_mutations", 2 );
    new_category.iv_additional_mutations_chance = jsobj.get_int( "iv_additional_mutations_chance", 3 );
    new_category.iv_hunger   = jsobj.get_int( "iv_hunger", 10 );
    new_category.iv_thirst   = jsobj.get_int( "iv_thirst", 10 );
    new_category.iv_pain     = jsobj.get_int( "iv_pain", 2 );
    new_category.iv_fatigue  = jsobj.get_int( "iv_fatigue", 5 );
    new_category.iv_morale   = jsobj.get_int( "iv_morale", 0 );
    new_category.iv_morale_max   = jsobj.get_int( "iv_morale_max", 0 );
    new_category.iv_sound = jsobj.get_bool( "iv_sound", false );
    new_category.raw_iv_sound_message = jsobj.get_string( "iv_sound_message",
                                        translate_marker( "You inject yoursel-arRGH!" ) );
    new_category.raw_iv_sound_id = jsobj.get_string( "iv_sound_id", "shout" );
    new_category.raw_iv_sound_variant = jsobj.get_string( "iv_sound_variant", "default" );
    new_category.iv_noise = jsobj.get_int( "iv_noise", 0 );
    new_category.iv_sleep = jsobj.get_bool( "iv_sleep", false );
    new_category.raw_iv_sleep_message = jsobj.get_string( "iv_sleep_message",
                                        translate_marker( "You fall asleep." ) );
    new_category.iv_sleep_dur = jsobj.get_int( "iv_sleep_dur", 0 );
    static_cast<void>( translate_marker_context( "memorial_male", "Crossed a threshold" ) );
    static_cast<void>( translate_marker_context( "memorial_female", "Crossed a threshold" ) );
    new_category.raw_memorial_message = jsobj.get_string( "memorial_message",
                                        "Crossed a threshold" );
    new_category.raw_junkie_message = jsobj.get_string( "junkie_message",
                                      translate_marker( "Oh, yeah!  That's the stuff!" ) );

    mutation_category_traits[new_category.id] = new_category;
}

std::string mutation_category_trait::name() const
{
    return _( raw_name );
}

std::string mutation_category_trait::mutagen_message() const
{
    return _( raw_mutagen_message );
}

std::string mutation_category_trait::iv_message() const
{
    return _( raw_iv_message );
}

std::string mutation_category_trait::iv_sound_message() const
{
    return _( raw_iv_sound_message );
}

std::string mutation_category_trait::iv_sound_id() const
{
    return _( raw_iv_sound_id );
}

std::string mutation_category_trait::iv_sound_variant() const
{
    return _( raw_iv_sound_variant );
}

std::string mutation_category_trait::iv_sleep_message() const
{
    return _( raw_iv_sleep_message );
}

std::string mutation_category_trait::junkie_message() const
{
    return _( raw_junkie_message );
}

std::string mutation_category_trait::memorial_message_male() const
{
    return pgettext( "memorial_male", raw_memorial_message.c_str() );
}

std::string mutation_category_trait::memorial_message_female() const
{
    return pgettext( "memorial_female", raw_memorial_message.c_str() );
}

const std::map<std::string, mutation_category_trait> &mutation_category_trait::get_all()
{
    return mutation_category_traits;
}

const mutation_category_trait &mutation_category_trait::get_category( const std::string
        &category_id )
{
    return mutation_category_traits.find( category_id )->second;
}

void mutation_category_trait::reset()
{
    mutation_category_traits.clear();
}

void mutation_category_trait::check_consistency()
{
    for( const auto &pr : mutation_category_traits ) {
        const mutation_category_trait &cat = pr.second;
        if( !cat.threshold_mut.is_empty() && !cat.threshold_mut.is_valid() ) {
            debugmsg( "Mutation category %s has threshold mutation %s, which does not exist",
                      cat.id.c_str(), cat.threshold_mut.c_str() );
        }
    }
}

static mut_attack load_mutation_attack( const JsonObject &jo )
{
    mut_attack ret;
    jo.read( "attack_text_u", ret.attack_text_u );
    jo.read( "attack_text_npc", ret.attack_text_npc );
    jo.read( "required_mutations", ret.required_mutations );
    jo.read( "blocker_mutations", ret.blocker_mutations );
    jo.read( "hardcoded_effect", ret.hardcoded_effect );

    if( jo.has_string( "body_part" ) ) {
        ret.bp = get_body_part_token( jo.get_string( "body_part" ) );
    }

    jo.read( "chance", ret.chance );

    if( jo.has_array( "base_damage" ) ) {
        ret.base_damage = load_damage_instance( jo.get_array( "base_damage" ) );
    } else if( jo.has_object( "base_damage" ) ) {
        JsonObject jo_dam = jo.get_object( "base_damage" );
        ret.base_damage = load_damage_instance( jo_dam );
    }

    if( jo.has_array( "strength_damage" ) ) {
        ret.strength_damage = load_damage_instance( jo.get_array( "strength_damage" ) );
    } else if( jo.has_object( "strength_damage" ) ) {
        JsonObject jo_dam = jo.get_object( "strength_damage" );
        ret.strength_damage = load_damage_instance( jo_dam );
    }

    if( ret.attack_text_u.empty() || ret.attack_text_npc.empty() ) {
        jo.throw_error( "Attack message unset" );
    }

    if( !ret.hardcoded_effect && ret.base_damage.empty() && ret.strength_damage.empty() ) {
        jo.throw_error( "Damage unset" );
    } else if( ret.hardcoded_effect && ( !ret.base_damage.empty() || !ret.strength_damage.empty() ) ) {
        jo.throw_error( "Damage and hardcoded effect are both set (must be one, not both)" );
    }

    if( ret.chance <= 0 ) {
        jo.throw_error( "Chance of procing must be set and positive" );
    }

    return ret;
}

static social_modifiers load_mutation_social_mods( const JsonObject &jo )
{
    social_modifiers ret;
    jo.read( "lie", ret.lie );
    jo.read( "persuade", ret.persuade );
    jo.read( "intimidate", ret.intimidate );
    return ret;
}

void mutation_branch::load_trait( const JsonObject &jo, const std::string &src )
{
    trait_factory.load( jo, src );
}

mut_transform::mut_transform() : active( false ), moves( 0 ) {}

bool mut_transform::load( const JsonObject &jsobj, const std::string &member )
{
    JsonObject j = jsobj.get_object( member );

    assign( j, "target", target );
    assign( j, "msg_transform", msg_transform );
    assign( j, "active", active );
    assign( j, "moves", moves );

    return true;
}

void mutation_branch::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "name", raw_name );
    mandatory( jo, was_loaded, "description", raw_desc );
    mandatory( jo, was_loaded, "points", points );

    optional( jo, was_loaded, "visibility", visibility, 0 );
    optional( jo, was_loaded, "ugliness", ugliness, 0 );
    optional( jo, was_loaded, "starting_trait", startingtrait, false );
    optional( jo, was_loaded, "mixed_effect", mixed_effect, false );
    optional( jo, was_loaded, "active", activated, false );
    optional( jo, was_loaded, "starts_active", starts_active, false );
    optional( jo, was_loaded, "destroys_gear", destroys_gear, false );
    optional( jo, was_loaded, "allow_soft_gear", allow_soft_gear, false );
    optional( jo, was_loaded, "cost", cost, 0 );
    optional( jo, was_loaded, "time", cooldown, 0 );
    optional( jo, was_loaded, "hunger", hunger, false );
    optional( jo, was_loaded, "thirst", thirst, false );
    optional( jo, was_loaded, "fatigue", fatigue, false );
    optional( jo, was_loaded, "valid", valid, true );
    optional( jo, was_loaded, "purifiable", purifiable, true );

    if( jo.has_object( "spawn_item" ) ) {
        auto si = jo.get_object( "spawn_item" );
        optional( si, was_loaded, "type", spawn_item );
        optional( si, was_loaded, "message", raw_spawn_item_message );
    }
    if( jo.has_object( "ranged_mutation" ) ) {
        auto si = jo.get_object( "ranged_mutation" );
        optional( si, was_loaded, "type", ranged_mutation );
        optional( si, was_loaded, "message", raw_ranged_mutation_message );
    }
    if( jo.has_object( "transform" ) ) {
        transform = cata::make_value<mut_transform>();
        transform->load( jo, "transform" );
    }
    optional( jo, was_loaded, "initial_ma_styles", initial_ma_styles );

    if( jo.has_array( "bodytemp_modifiers" ) ) {
        auto bodytemp_array = jo.get_array( "bodytemp_modifiers" );
        bodytemp_min = bodytemp_array.get_int( 0 );
        bodytemp_max = bodytemp_array.get_int( 1 );
    }

    optional( jo, was_loaded, "bodytemp_sleep", bodytemp_sleep, 0 );
    optional( jo, was_loaded, "threshold", threshold, false );
    optional( jo, was_loaded, "profession", profession, false );
    optional( jo, was_loaded, "debug", debug, false );
    optional( jo, was_loaded, "player_display", player_display, true );

    for( JsonArray pair : jo.get_array( "vitamin_rates" ) ) {
        vitamin_rates.emplace( vitamin_id( pair.get_string( 0 ) ),
                               time_duration::from_turns( pair.get_int( 1 ) ) );
    }

    for( JsonArray pair : jo.get_array( "vitamins_absorb_multi" ) ) {
        std::map<vitamin_id, double> vit;
        // fill the inner map with vitamins
        for( JsonArray vitamins : pair.get_array( 1 ) ) {
            vit.emplace( vitamin_id( vitamins.get_string( 0 ) ), vitamins.get_float( 1 ) );
        }
        // assign the inner vitamin map to the material_id key
        vitamin_absorb_multi.emplace( material_id( pair.get_string( 0 ) ), vit );
    }

    optional( jo, was_loaded, "healing_awake", healing_awake, 0.0f );
    optional( jo, was_loaded, "healing_resting", healing_resting, 0.0f );
    optional( jo, was_loaded, "mending_modifier", mending_modifier, 1.0f );
    optional( jo, was_loaded, "hp_modifier", hp_modifier, 0.0f );
    optional( jo, was_loaded, "hp_modifier_secondary", hp_modifier_secondary, 0.0f );
    optional( jo, was_loaded, "hp_adjustment", hp_adjustment, 0.0f );
    optional( jo, was_loaded, "stealth_modifier", stealth_modifier, 0.0f );
    optional( jo, was_loaded, "str_modifier", str_modifier, 0.0f );
    optional( jo, was_loaded, "cut_dmg_bonus", cut_dmg_bonus, 0 );
    optional( jo, was_loaded, "pierce_dmg_bonus", pierce_dmg_bonus, 0.0f );
    optional( jo, was_loaded, "bash_dmg_bonus", bash_dmg_bonus, 0 );
    optional( jo, was_loaded, "dodge_modifier", dodge_modifier, 0.0f );
    optional( jo, was_loaded, "speed_modifier", speed_modifier, 1.0f );
    optional( jo, was_loaded, "movecost_modifier", movecost_modifier, 1.0f );
    optional( jo, was_loaded, "movecost_flatground_modifier", movecost_flatground_modifier, 1.0f );
    optional( jo, was_loaded, "movecost_obstacle_modifier", movecost_obstacle_modifier, 1.0f );
    optional( jo, was_loaded, "movecost_swim_modifier", movecost_swim_modifier, 1.0f );
    optional( jo, was_loaded, "attackcost_modifier", attackcost_modifier, 1.0f );
    optional( jo, was_loaded, "max_stamina_modifier", max_stamina_modifier, 1.0f );
    optional( jo, was_loaded, "weight_capacity_modifier", weight_capacity_modifier, 1.0f );
    optional( jo, was_loaded, "hearing_modifier", hearing_modifier, 1.0f );
    optional( jo, was_loaded, "noise_modifier", noise_modifier, 1.0f );
    optional( jo, was_loaded, "temperature_speed_modifier", temperature_speed_modifier, 0.0f );
    optional( jo, was_loaded, "metabolism_modifier", metabolism_modifier, 0.0f );
    optional( jo, was_loaded, "thirst_modifier", thirst_modifier, 0.0f );
    optional( jo, was_loaded, "fatigue_modifier", fatigue_modifier, 0.0f );
    optional( jo, was_loaded, "fatigue_regen_modifier", fatigue_regen_modifier, 0.0f );
    optional( jo, was_loaded, "stamina_regen_modifier", stamina_regen_modifier, 0.0f );
    optional( jo, was_loaded, "overmap_sight", overmap_sight, 0.0f );
    optional( jo, was_loaded, "overmap_multiplier", overmap_multiplier, 1.0f );
    optional( jo, was_loaded, "night_vision_range", night_vision_range, 0.0f );
    optional( jo, was_loaded, "reading_speed_multiplier", reading_speed_multiplier, 1.0f );
    optional( jo, was_loaded, "skill_rust_multiplier", skill_rust_multiplier, 1.0f );
    optional( jo, was_loaded, "scent_modifier", scent_modifier, 1.0f );
    optional( jo, was_loaded, "scent_intensity", scent_intensity, cata::nullopt );
    optional( jo, was_loaded, "scent_mask", scent_mask, cata::nullopt );
    optional( jo, was_loaded, "scent_type", scent_typeid, cata::nullopt );
    optional( jo, was_loaded, "bleed_resist", bleed_resist, 0 );
    optional( jo, was_loaded, "healthy_rate", healthy_rate, 1.0f );
    optional( jo, was_loaded, "weakness_to_water", weakness_to_water, 0 );
    optional( jo, was_loaded, "ignored_by", ignored_by );
    optional( jo, was_loaded, "can_only_eat", can_only_eat );
    optional( jo, was_loaded, "can_only_heal_with", can_only_heal_with );
    optional( jo, was_loaded, "can_heal_with", can_heal_with );

    optional( jo, was_loaded, "butchering_quality", butchering_quality, 0 );

    optional( jo, was_loaded, "allowed_category", allowed_category );

    optional( jo, was_loaded, "mana_modifier", mana_modifier, 0 );
    optional( jo, was_loaded, "mana_multiplier", mana_multiplier, 1.0f );
    optional( jo, was_loaded, "mana_regen_multiplier", mana_regen_multiplier, 1.0f );

    if( jo.has_object( "rand_cut_bonus" ) ) {
        JsonObject sm = jo.get_object( "rand_cut_bonus" );
        rand_cut_bonus.first = sm.get_int( "min" );
        rand_cut_bonus.second = sm.get_int( "max" );
    }

    if( jo.has_object( "rand_bash_bonus" ) ) {
        JsonObject sm = jo.get_object( "rand_bash_bonus" );
        rand_bash_bonus.first = sm.get_int( "min" );
        rand_bash_bonus.second = sm.get_int( "max" );
    }

    if( jo.has_object( "social_modifiers" ) ) {
        JsonObject sm = jo.get_object( "social_modifiers" );
        social_mods = load_mutation_social_mods( sm );
    }

    load_mutation_mods( jo, "passive_mods", mods );
    /* Not currently supported due to inability to save active mutation state
    load_mutation_mods(jsobj, "active_mods", new_mut.mods); */

    optional( jo, was_loaded, "prereqs", prereqs, trait_reader{} );
    optional( jo, was_loaded, "prereqs2", prereqs2, trait_reader{} );
    optional( jo, was_loaded, "threshreq", threshreq, trait_reader{} );
    optional( jo, was_loaded, "cancels", cancels, trait_reader{} );
    optional( jo, was_loaded, "changes_to", replacements, trait_reader{} );
    optional( jo, was_loaded, "leads_to", additions, trait_reader{} );
    optional( jo, was_loaded, "flags", flags, string_reader{} );
    optional( jo, was_loaded, "types", types, string_reader{} );
    optional( jo, was_loaded, "enchantments", enchantments );

    for( const std::string s : jo.get_array( "no_cbm_on_bp" ) ) {
        no_cbm_on_bp.emplace( get_body_part_token( s ) );
    }

    optional( jo, was_loaded, "category", category, string_reader{} );

    for( JsonArray ja : jo.get_array( "spells_learned" ) ) {
        const spell_id sp( ja.next_string() );
        spells_learned.emplace( sp, ja.next_int() );
    }

    for( JsonArray ja : jo.get_array( "craft_skill_bonus" ) ) {
        const skill_id skid( ja.next_string() );
        if( skid.is_valid() ) {
            craft_skill_bonus.emplace( skid, ja.next_int() );
        } else {
            jo.throw_error( "invalid skill_id" );
        }
    }

    for( JsonArray ja : jo.get_array( "lumination" ) ) {
        const body_part bp = get_body_part_token( ja.next_string() );
        lumination.emplace( bp, static_cast<float>( ja.next_float() ) );
    }

    for( JsonArray ja : jo.get_array( "anger_relations" ) ) {
        const species_id spe = species_id( ja.next_string() );
        anger_relations.emplace( spe, ja.next_int() );

    }

    for( JsonObject wp : jo.get_array( "wet_protection" ) ) {
        std::string part_id = wp.get_string( "part" );
        int ignored = wp.get_int( "ignored", 0 );
        int neutral = wp.get_int( "neutral", 0 );
        int good = wp.get_int( "good", 0 );
        tripoint protect = tripoint( ignored, neutral, good );
        protection[get_body_part_token( part_id )] = protect;
    }

    for( JsonArray ea : jo.get_array( "encumbrance_always" ) ) {
        std::string part_id = ea.next_string();
        int enc = ea.next_int();
        encumbrance_always[get_body_part_token( part_id )] = enc;
    }

    for( JsonArray ec : jo.get_array( "encumbrance_covered" ) ) {
        std::string part_id = ec.next_string();
        int enc = ec.next_int();
        encumbrance_covered[get_body_part_token( part_id )] = enc;
    }

    for( const std::string line : jo.get_array( "restricts_gear" ) ) {
        restricts_gear.insert( get_body_part_token( line ) );
    }

    for( JsonObject ao : jo.get_array( "armor" ) ) {
        auto parts = ao.get_tags( "parts" );
        std::set<body_part> bps;
        for( const std::string &part_string : parts ) {
            if( part_string == "ALL" ) {
                // Shorthand, since many mutations protect whole body
                bps.insert( all_body_parts.begin(), all_body_parts.end() );
            } else {
                bps.insert( get_body_part_token( part_string ) );
            }
        }

        resistances res = load_resistances_instance( ao );

        for( body_part bp : bps ) {
            armor[ bp ] = res;
        }
    }

    if( jo.has_array( "attacks" ) ) {
        for( JsonObject ao : jo.get_array( "attacks" ) ) {
            attacks_granted.emplace_back( load_mutation_attack( ao ) );
        }
    } else if( jo.has_object( "attacks" ) ) {
        JsonObject attack = jo.get_object( "attacks" );
        attacks_granted.emplace_back( load_mutation_attack( attack ) );
    }
}

std::string mutation_branch::spawn_item_message() const
{
    return _( raw_spawn_item_message );
}

std::string mutation_branch::ranged_mutation_message() const
{
    return _( raw_ranged_mutation_message );
}

std::string mutation_branch::name() const
{
    return raw_name.translated();
}

std::string mutation_branch::desc() const
{
    return raw_desc.translated();
}

static void check_consistency( const std::vector<trait_id> &mvec, const trait_id &mid,
                               const std::string &what )
{
    for( const auto &m : mvec ) {
        if( !m.is_valid() ) {
            debugmsg( "mutation %s refers to undefined %s %s", mid.c_str(), what.c_str(), m.c_str() );
        }
    }
}

void mutation_branch::check_consistency()
{
    for( const auto &mdata : get_all() ) {
        const auto &mid = mdata.id;
        const cata::optional<scenttype_id> &s_id = mdata.scent_typeid;
        const std::map<species_id, int> &an_id = mdata.anger_relations;
        for( const auto &style : mdata.initial_ma_styles ) {
            if( !style.is_valid() ) {
                debugmsg( "mutation %s refers to undefined martial art style %s", mid.c_str(), style.c_str() );
            }
        }
        for( const std::string &type : mdata.types ) {
            if( !mutation_type_exists( type ) ) {
                debugmsg( "mutation %s refers to undefined mutation type %s", mid.c_str(), type );
            }
        }
        if( mid->transform ) {
            const trait_id tid = mid->transform->target;
            if( !tid.is_valid() ) {
                debugmsg( "mutation %s transform uses undefined target %s", mid.c_str(), tid.c_str() );
            }
        }
        for( const std::pair<species_id, int> elem : an_id ) {
            if( !elem.first.is_valid() ) {
                debugmsg( "mutation %s refers to undefined species id %s", mid.c_str(), elem.first.c_str() );
            }
        }
        if( s_id && !s_id.value().is_valid() ) {
            debugmsg( "mutation %s refers to undefined scent type %s", mid.c_str(), s_id.value().c_str() );
        }
        ::check_consistency( mdata.prereqs, mid, "prereq" );
        ::check_consistency( mdata.prereqs2, mid, "prereqs2" );
        ::check_consistency( mdata.threshreq, mid, "threshreq" );
        ::check_consistency( mdata.cancels, mid, "cancels" );
        ::check_consistency( mdata.replacements, mid, "replacements" );
        ::check_consistency( mdata.additions, mid, "additions" );
    }
}

nc_color mutation_branch::get_display_color() const
{
    if( threshold || profession ) {
        return c_white;
    } else if( debug ) {
        return c_light_cyan;
    } else if( mixed_effect ) {
        return c_pink;
    } else if( points > 0 ) {
        return c_light_green;
    } else if( points < 0 ) {
        return c_light_red;
    } else {
        return c_yellow;
    }
}

std::string mutation_branch::get_name( const trait_id &mutation_id )
{
    return mutation_id->name();
}

const std::vector<mutation_branch> &mutation_branch::get_all()
{
    return trait_factory.get_all();
}

void mutation_branch::reset_all()
{
    mutations_category.clear();
    trait_factory.reset();
    trait_blacklist.clear();
    trait_groups.clear();
    trait_groups.emplace( trait_group::Trait_group_tag( "EMPTY_GROUP" ),
                          make_shared_fast<Trait_group_collection>( 100 ) );
}

std::vector<std::string> dream::messages() const
{
    std::vector<std::string> ret;
    for( const auto &msg : raw_messages ) {
        ret.push_back( _( msg ) );
    }
    return ret;
}

void dream::load( const JsonObject &jsobj )
{
    dream newdream;

    newdream.strength = jsobj.get_int( "strength" );
    newdream.category = jsobj.get_string( "category" );

    for( const std::string line : jsobj.get_array( "messages" ) ) {
        newdream.raw_messages.push_back( line );
    }

    dreams.push_back( newdream );
}

bool trait_display_sort( const trait_id &a, const trait_id &b ) noexcept
{
    if( a->get_display_color() > b->get_display_color() ) {
        return true;
    }
    if( a->get_display_color() < b->get_display_color() ) {
        return false;
    }

    return localized_compare( a->name(), b->name() );
}

void mutation_branch::load_trait_blacklist( const JsonObject &jsobj )
{
    for( const std::string line : jsobj.get_array( "traits" ) ) {
        trait_blacklist.insert( trait_id( line ) );
    }
}

bool mutation_branch::trait_is_blacklisted( const trait_id &tid )
{
    return trait_blacklist.count( tid );
}

void mutation_branch::finalize()
{
    for( const mutation_branch &branch : get_all() ) {
        for( const std::string &cat : branch.category ) {
            mutations_category[cat].push_back( trait_id( branch.id ) );
        }
    }
    finalize_trait_blacklist();
}

void mutation_branch::finalize_trait_blacklist()
{
    for( auto &trait : trait_blacklist ) {
        if( !trait.is_valid() ) {
            debugmsg( "trait on blacklist %s does not exist", trait.c_str() );
        }
    }
}

void mutation_branch::load_trait_group( const JsonObject &jsobj )
{
    const trait_group::Trait_group_tag group_id( jsobj.get_string( "id" ) );
    const std::string subtype = jsobj.get_string( "subtype", "old" );
    load_trait_group( jsobj, group_id, subtype );
}

static Trait_group &make_group_or_throw( const trait_group::Trait_group_tag &gid,
        bool is_collection )
{
    // NOTE: If the gid is already in the map, emplace will just return an iterator to it
    auto found = ( is_collection
                   ? trait_groups.emplace( gid, make_shared_fast<Trait_group_collection>( 100 ) )
                   : trait_groups.emplace( gid, make_shared_fast<Trait_group_distribution>( 100 ) ) ).first;
    // Evidently, making the collection/distribution separation better has made the code for this check worse.
    if( is_collection ) {
        if( dynamic_cast<Trait_group_distribution *>( found->second.get() ) ) {
            throw std::runtime_error( string_format(
                                          R"("mutation group "%s" already defined with type "distribution")", gid.str() ) );
        }
    } else {
        if( dynamic_cast<Trait_group_collection *>( found->second.get() ) ) {
            throw std::runtime_error( string_format(
                                          R"("mutation group "%s" already defined with type "collection")", gid.str() ) );
        }
    }
    return *found->second;
}

void mutation_branch::load_trait_group( const JsonArray &entries,
                                        const trait_group::Trait_group_tag &gid, const bool is_collection )
{
    Trait_group &tg = make_group_or_throw( gid, is_collection );

    for( const JsonValue entry : entries ) {
        // Backwards-compatibility with old format ["TRAIT", 100]
        if( entry.test_array() ) {
            JsonArray subarr = entry.get_array();

            trait_id id( subarr.get_string( 0 ) );
            tg.add_entry( std::make_unique<Single_trait_creator>( id, subarr.get_int( 1 ) ) );
            // Otherwise load new format {"trait": ... } or {"group": ...}
        } else {
            JsonObject subobj = entry.get_object();
            add_entry( tg, subobj );
        }
    }
}

void mutation_branch::load_trait_group( const JsonObject &jsobj,
                                        const trait_group::Trait_group_tag &gid,
                                        const std::string &subtype )
{
    if( subtype != "distribution" && subtype != "collection" && subtype != "old" ) {
        jsobj.throw_error( "unknown trait group type", "subtype" );
    }

    Trait_group &tg = make_group_or_throw( gid, subtype == "collection" || subtype == "old" );

    // TODO: (sm) Looks like this makes the new code backwards-compatible with the old format. Great if so!
    if( subtype == "old" ) {
        for( JsonArray pair : jsobj.get_array( "traits" ) ) {
            tg.add_trait_entry( trait_id( pair.get_string( 0 ) ), pair.get_int( 1 ) );
        }
        return;
    }

    // TODO: (sm) Taken from item_factory.cpp almost verbatim. Ensure that these work!
    if( jsobj.has_member( "entries" ) ) {
        for( JsonObject subobj : jsobj.get_array( "entries" ) ) {
            add_entry( tg, subobj );
        }
    }
    if( jsobj.has_member( "traits" ) ) {
        for( const JsonValue entry : jsobj.get_array( "traits" ) ) {
            if( entry.test_string() ) {
                tg.add_trait_entry( trait_id( entry.get_string() ), 100 );
            } else if( entry.test_array() ) {
                JsonArray subtrait = entry.get_array();
                tg.add_trait_entry( trait_id( subtrait.get_string( 0 ) ), subtrait.get_int( 1 ) );
            } else {
                JsonObject subobj = entry.get_object();
                add_entry( tg, subobj );
            }
        }
    }
    if( jsobj.has_member( "groups" ) ) {
        for( const JsonValue entry : jsobj.get_array( "groups" ) ) {
            if( entry.test_string() ) {
                tg.add_group_entry( trait_group::Trait_group_tag( entry.get_string() ), 100 );
            } else if( entry.test_array() ) {
                JsonArray subtrait = entry.get_array();
                tg.add_group_entry( trait_group::Trait_group_tag( subtrait.get_string( 0 ) ),
                                    subtrait.get_int( 1 ) );
            } else {
                JsonObject subobj = entry.get_object();
                add_entry( tg, subobj );
            }
        }
    }
}

void mutation_branch::add_entry( Trait_group &tg, const JsonObject &obj )
{
    std::unique_ptr<Trait_creation_data> ptr;
    int probability = obj.get_int( "prob", 100 );
    JsonArray jarr;

    if( obj.has_member( "collection" ) ) {
        ptr = std::make_unique<Trait_group_collection>( probability );
        jarr = obj.get_array( "collection" );
    } else if( obj.has_member( "distribution" ) ) {
        ptr = std::make_unique<Trait_group_distribution>( probability );
        jarr = obj.get_array( "distribution" );
    }

    if( ptr ) {
        Trait_group &tg2 = dynamic_cast<Trait_group &>( *ptr );
        for( const JsonObject job2 : jarr ) {
            add_entry( tg2, job2 );
        }
        tg.add_entry( std::move( ptr ) );
        return;
    }

    if( obj.has_member( "trait" ) ) {
        trait_id id( obj.get_string( "trait" ) );
        ptr = std::make_unique<Single_trait_creator>( id, probability );
    } else if( obj.has_member( "group" ) ) {
        ptr = std::make_unique<Trait_group_creator>( trait_group::Trait_group_tag(
                    obj.get_string( "group" ) ),
                probability );
    }

    if( !ptr ) {
        return;
    }

    tg.add_entry( std::move( ptr ) );
}

shared_ptr_fast<Trait_group> mutation_branch::get_group( const
        trait_group::Trait_group_tag &gid )
{
    auto found = trait_groups.find( gid );
    return found != trait_groups.end() ? found->second : nullptr;
}

std::vector<trait_group::Trait_group_tag> mutation_branch::get_all_group_names()
{
    std::vector<trait_group::Trait_group_tag> rval;
    for( auto &group : trait_groups ) {
        rval.push_back( group.first );
    }
    return rval;
}

bool mutation_category_is_valid( const std::string &cat )
{
    return mutation_category_traits.count( cat );
}
