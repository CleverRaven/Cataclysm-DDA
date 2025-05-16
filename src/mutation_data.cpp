#include "mutation.h" // IWYU pragma: associated

#include <algorithm>
#include <cstdlib>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "assign.h"
#include "color.h"
#include "condition.h"
#include "debug.h"
#include "effect_on_condition.h"
#include "enum_conversions.h"
#include "enums.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "json.h"
#include "localized_comparator.h"
#include "magic_enchantment.h"
#include "make_static.h"
#include "memory_fast.h"
#include "npc.h"
#include "string_formatter.h"
#include "trait_group.h"
#include "translations.h"
#include "uilist.h"
#include "weighted_list.h"

static const mutation_category_id mutation_category_ANY( "ANY" );

static const trait_group::Trait_group_tag Trait_group_EMPTY_GROUP( "EMPTY_GROUP" );

using TraitGroupMap =
    std::map<trait_group::Trait_group_tag, shared_ptr_fast<Trait_group>>;
using TraitSet = std::set<trait_id>;
using trait_reader = string_id_reader<::mutation_branch>;
using flag_reader = string_id_reader<::json_flag>;

static TraitSet trait_blacklist;
static TraitGroupMap trait_groups;

static std::map<trait_id, trait_replacement> trait_migrations;
static trait_replacement trait_migration_remove{ std::nullopt, std::nullopt, true };

namespace
{
generic_factory<mutation_branch> trait_factory( "trait" );
} // namespace

std::vector<dream> dreams;
std::map<mutation_category_id, std::vector<trait_id> > mutations_category;
static std::map<mutation_category_id, mutation_category_trait> mutation_category_traits;

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

void mutation_category_trait::load( const JsonObject &jsobj )
{
    mutation_category_trait new_category;
    jsobj.read( "id", new_category.id, true );
    jsobj.get_member( "name" ).read( new_category.raw_name );
    new_category.threshold_mut = trait_id( jsobj.get_string( "threshold_mut" ) );

    jsobj.get_member( "mutagen_message" ).read( new_category.raw_mutagen_message );
    new_category.wip = jsobj.get_bool( "wip", false );
    new_category.skip_test = jsobj.get_bool( "skip_test", false );
    static_cast<void>( translate_marker_context( "memorial_male", "Crossed a threshold" ) );
    static_cast<void>( translate_marker_context( "memorial_female", "Crossed a threshold" ) );
    optional( jsobj, false, "memorial_message", new_category.raw_memorial_message,
              text_style_check_reader(), "Crossed a threshold" );
    new_category.vitamin = vitamin_id( jsobj.get_string( "vitamin", "null" ) );
    optional( jsobj, false, "threshold_min", new_category.threshold_min, 2200 );
    optional( jsobj, false, "base_removal_chance", new_category.base_removal_chance, 100 );
    optional( jsobj, false, "base_removal_cost_mul", new_category.base_removal_cost_mul, 3.0f );

    mutation_category_traits[new_category.id] = new_category;
}

std::string mutation_category_trait::name() const
{
    return raw_name.translated();
}

std::string mutation_category_trait::mutagen_message() const
{
    return raw_mutagen_message.translated();
}

std::string mutation_category_trait::memorial_message_male() const
{
    return pgettext( "memorial_male", raw_memorial_message.c_str() );
}

std::string mutation_category_trait::memorial_message_female() const
{
    return pgettext( "memorial_female", raw_memorial_message.c_str() );
}

const std::map<mutation_category_id, mutation_category_trait> &mutation_category_trait::get_all()
{
    return mutation_category_traits;
}

const mutation_category_trait &mutation_category_trait::get_category(
    const mutation_category_id &category_id )
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
        ret.bp = bodypart_str_id( jo.get_string( "body_part" ) );
    }

    jo.read( "chance", ret.chance );

    optional( jo, false, "base_damage", ret.base_damage );
    optional( jo, false, "strength_damage", ret.strength_damage );

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

mut_transform::mut_transform() = default;

bool mut_transform::load( const JsonObject &jsobj, std::string_view member )
{
    JsonObject j = jsobj.get_object( member );

    assign( j, "target", target );
    assign( j, "msg_transform", msg_transform );
    assign( j, "active", active );
    optional( j, false, "safe", safe, false );
    assign( j, "moves", moves );

    return true;
}

mut_personality_score::mut_personality_score() = default;

bool mut_personality_score::load( const JsonObject &jsobj, std::string_view member )
{
    JsonObject j = jsobj.get_object( member );

    optional( j, false, "min_aggression", min_aggression, NPC_PERSONALITY_MIN );
    optional( j, false, "max_aggression", max_aggression, NPC_PERSONALITY_MAX );
    optional( j, false, "min_bravery", min_bravery, NPC_PERSONALITY_MIN );
    optional( j, false, "max_bravery", max_bravery, NPC_PERSONALITY_MAX );
    optional( j, false, "min_collector", min_collector, NPC_PERSONALITY_MIN );
    optional( j, false, "max_collector", max_collector, NPC_PERSONALITY_MAX );
    optional( j, false, "min_altruism", min_altruism, NPC_PERSONALITY_MIN );
    optional( j, false, "max_altruism", max_altruism, NPC_PERSONALITY_MAX );

    return true;
}

void reflex_activation_data::load( const JsonObject &jsobj )
{
    read_condition( jsobj, "condition", trigger, false );
    if( jsobj.has_object( "msg_on" ) ) {
        JsonObject jo = jsobj.get_object( "msg_on" );
        optional( jo, was_loaded, "text", msg_on.first );
        std::string tmp_rating;
        optional( jo, was_loaded, "rating", tmp_rating, "neutral" );
        msg_on.second = io::string_to_enum<game_message_type>( tmp_rating );
    }
    if( jsobj.has_object( "msg_off" ) ) {
        JsonObject jo = jsobj.get_object( "msg_off" );
        optional( jo, was_loaded, "text", msg_off.first );
        std::string tmp_rating;
        optional( jo, was_loaded, "rating", tmp_rating, "neutral" );
        msg_off.second = io::string_to_enum<game_message_type>( tmp_rating );
    }
}

void reflex_activation_data::deserialize( const JsonObject &jo )
{
    load( jo );
}

void trait_and_var::deserialize( const JsonValue &jv )
{
    if( jv.test_string() ) {
        jv.read( trait );
    } else {
        JsonObject jo = jv.get_object();
        jo.read( "trait", trait );
        jo.read( "variant", variant );
    }
}

void trait_and_var::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "trait", trait );
    jsout.member( "variant", variant );

    jsout.end_object();
}

std::string trait_and_var::name() const
{
    return trait->name( variant );
}

std::string trait_and_var::desc() const
{
    return trait->desc( variant );
}

void mutation_variant::load( const JsonObject &jo )
{
    mandatory( jo, false, "id", id );
    mandatory( jo, false, "name", alt_name );
    mandatory( jo, false, "description", alt_description );

    optional( jo, false, "append_desc", append_desc );
    optional( jo, false, "weight", weight );
}

void mutation_variant::deserialize( const JsonObject &jo )
{
    load( jo );
}

void mutation_branch::load( const JsonObject &jo, std::string_view src )
{
    mandatory( jo, was_loaded, "name", raw_name );
    mandatory( jo, was_loaded, "description", raw_desc );
    mandatory( jo, was_loaded, "points", points );

    optional( jo, was_loaded, "vitamin_cost", vitamin_cost, 100 );
    optional( jo, was_loaded, "visibility", visibility, 0 );
    optional( jo, was_loaded, "ugliness", ugliness, 0 );
    optional( jo, was_loaded, "starting_trait", startingtrait, false );
    optional( jo, was_loaded, "random_at_chargen", random_at_chargen, true );
    optional( jo, was_loaded, "mixed_effect", mixed_effect, false );
    optional( jo, was_loaded, "active", activated, false );
    optional( jo, was_loaded, "starts_active", starts_active, false );
    optional( jo, was_loaded, "destroys_gear", destroys_gear, false );
    optional( jo, was_loaded, "allow_soft_gear", allow_soft_gear, false );
    optional( jo, was_loaded, "cost", cost, 0 );
    optional( jo, was_loaded, "time", cooldown, 0_turns );
    optional( jo, was_loaded, "kcal", hunger, false );
    optional( jo, was_loaded, "thirst", thirst, false );
    optional( jo, was_loaded, "sleepiness", sleepiness, false );
    optional( jo, was_loaded, "mana", mana, false );
    optional( jo, was_loaded, "valid", valid, true );
    optional( jo, was_loaded, "purifiable", purifiable, true );

    std::vector<mutation_variant> _variants;
    optional( jo, was_loaded, "variants", _variants );
    for( mutation_variant &var : _variants ) {
        var.parent = id;
        variants.emplace( var.id, var );
    }

    if( jo.has_object( "override_look" ) ) {
        JsonObject jv = jo.get_object( "override_look" );
        std::string override_look_id;
        std::string override_look_tile_category;
        mandatory( jv, was_loaded, "id", override_look_id );
        mandatory( jv, was_loaded, "tile_category", override_look_tile_category );
        override_look.emplace( override_look_id, override_look_tile_category );
    }
    if( jo.has_object( "spawn_item" ) ) {
        JsonObject si = jo.get_object( "spawn_item" );
        optional( si, was_loaded, "type", spawn_item );
        optional( si, was_loaded, "message", raw_spawn_item_message );
    }
    if( jo.has_object( "ranged_mutation" ) ) {
        JsonObject si = jo.get_object( "ranged_mutation" );
        optional( si, was_loaded, "type", ranged_mutation );
        optional( si, was_loaded, "message", raw_ranged_mutation_message );
    }
    if( jo.has_object( "transform" ) ) {
        transform = cata::make_value<mut_transform>();
        transform->load( jo, "transform" );
    }
    if( jo.has_object( "personality_score" ) ) {
        personality_score = cata::make_value<mut_personality_score>();
        personality_score->load( jo, "personality_score" );
    }

    optional( jo, was_loaded, "triggers", trigger_list );

    optional( jo, was_loaded, "initial_ma_styles", initial_ma_styles );

    if( jo.has_array( "bodytemp_modifiers" ) ) {
        JsonArray bodytemp_array = jo.get_array( "bodytemp_modifiers" );
        bodytemp_min = units::from_legacy_bodypart_temp_delta( bodytemp_array.get_int( 0 ) );
        bodytemp_max = units::from_legacy_bodypart_temp_delta( bodytemp_array.get_int( 1 ) );
    }

    optional( jo, was_loaded, "threshold", threshold, false );
    optional( jo, was_loaded, "threshold_substitutes", threshold_substitutes );
    optional( jo, was_loaded, "strict_threshreq", strict_threshreq );
    optional( jo, was_loaded, "profession", profession, false );
    optional( jo, was_loaded, "debug", debug, false );
    optional( jo, was_loaded, "player_display", player_display, true );
    optional( jo, was_loaded, "vanity", vanity, false );
    optional( jo, was_loaded, "dummy", dummy, false );

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

    optional( jo, was_loaded, "scent_intensity", scent_intensity, std::nullopt );
    optional( jo, was_loaded, "scent_type", scent_typeid, std::nullopt );
    optional( jo, was_loaded, "ignored_by", ignored_by );
    optional( jo, was_loaded, "empathize_with", empathize_with );
    optional( jo, was_loaded, "no_empathize_with", no_empathize_with );
    optional( jo, was_loaded, "can_only_eat", can_only_eat );
    optional( jo, was_loaded, "can_only_heal_with", can_only_heal_with );
    optional( jo, was_loaded, "can_heal_with", can_heal_with );
    optional( jo, was_loaded, "activation_msg", activation_msg,
              to_translation( "You activate your %s." ) );

    optional( jo, was_loaded, "butchering_quality", butchering_quality, 0 );

    optional( jo, was_loaded, "allowed_category", allowed_category );

    if( jo.has_object( "social_modifiers" ) ) {
        JsonObject sm = jo.get_object( "social_modifiers" );
        social_mods = load_mutation_social_mods( sm );
    }

    optional( jo, was_loaded, "prereqs", prereqs, trait_reader{} );
    optional( jo, was_loaded, "prereqs2", prereqs2, trait_reader{} );
    optional( jo, was_loaded, "threshreq", threshreq, trait_reader{} );
    optional( jo, was_loaded, "cancels", cancels, trait_reader{} );
    optional( jo, was_loaded, "changes_to", replacements, trait_reader{} );
    optional( jo, was_loaded, "leads_to", additions, trait_reader{} );
    optional( jo, was_loaded, "flags", flags, flag_reader{} );
    optional( jo, was_loaded, "active_flags", active_flags, flag_reader{} );
    optional( jo, was_loaded, "inactive_flags", inactive_flags, flag_reader{} );
    optional( jo, was_loaded, "types", types, string_reader{} );

    for( JsonArray pair : jo.get_array( "moncams" ) ) {
        moncams.insert( std::pair<mtype_id, int>( mtype_id( pair.get_string( 0 ) ), pair.get_int( 1 ) ) );
    }

    for( JsonValue jv : jo.get_array( "activated_eocs" ) ) {
        activated_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }

    for( JsonValue jv : jo.get_array( "processed_eocs" ) ) {
        processed_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }

    for( JsonValue jv : jo.get_array( "deactivated_eocs" ) ) {
        deactivated_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }

    optional( jo, was_loaded, "activated_is_setup", activated_is_setup, false );

    int enchant_num = 0;
    for( JsonValue jv : jo.get_array( "enchantments" ) ) {
        std::string enchant_name = "INLINE_ENCH_" + id.str() + "_" + std::to_string( enchant_num++ );
        enchantments.push_back( enchantment::load_inline_enchantment( jv, src, enchant_name ) );
    }

    optional( jo, was_loaded, "comfort", comfort );

    for( const std::string s : jo.get_array( "no_cbm_on_bp" ) ) {
        no_cbm_on_bp.emplace( s );
    }

    optional( jo, was_loaded, "category", category, string_id_reader<mutation_category_trait> {} );

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
        const bodypart_str_id bp = bodypart_str_id( ja.next_string() );
        lumination.emplace( bp, static_cast<float>( ja.next_float() ) );
    }

    for( JsonArray ja : jo.get_array( "anger_relations" ) ) {
        const species_id spe = species_id( ja.next_string() );
        anger_relations.emplace( spe, ja.next_int() );

    }

    for( JsonObject wp : jo.get_array( "wet_protection" ) ) {
        int ignored = wp.get_int( "ignored", 0 );
        int neutral = wp.get_int( "neutral", 0 );
        int good = wp.get_int( "good", 0 );
        tripoint protect = tripoint( ignored, neutral, good );
        protection[bodypart_str_id( wp.get_string( "part" ) )] = protect;
    }

    for( JsonArray ea : jo.get_array( "encumbrance_always" ) ) {
        const bodypart_str_id bp = bodypart_str_id( ea.next_string() );
        const int enc = ea.next_int();
        encumbrance_always[bp] = enc;
    }

    for( JsonArray ec : jo.get_array( "encumbrance_covered" ) ) {
        const bodypart_str_id bp = bodypart_str_id( ec.next_string() );
        int enc = ec.next_int();
        encumbrance_covered[bp] = enc;
    }

    for( JsonMember ema : jo.get_object( "encumbrance_multiplier_always" ) ) {
        const bodypart_str_id &bp = bodypart_str_id( ema.name() );
        encumbrance_multiplier_always[bp] = ema.get_float();
    }

    for( const std::string line : jo.get_array( "restricts_gear" ) ) {
        bodypart_str_id bp( line );
        if( bp.is_valid() ) {
            restricts_gear.insert( bp );
        } else {
            restricts_gear_subparts.insert( sub_bodypart_str_id( line ) );
        }
    }

    for( const std::string line : jo.get_array( "remove_rigid" ) ) {
        bodypart_str_id bp( line );
        if( bp.is_valid() ) {
            remove_rigid.insert( bp );
        } else {
            remove_rigid_subparts.insert( sub_bodypart_str_id( line ) );
        }
    }

    for( const std::string line : jo.get_array( "allowed_items" ) ) {
        allowed_items.insert( flag_id( line ) );
    }

    for( JsonObject ao : jo.get_array( "armor" ) ) {
        std::set<std::string> ignored_by_resist = { "part_types", "parts" };
        const resistances res = load_resistances_instance( ao, ignored_by_resist );
        // Set damage resistances for all body parts of the specified type(s)
        for( const std::string &type_string : ao.get_tags( "part_types" ) ) {
            for( const body_part_type &bp : body_part_type::get_all() ) {
                if( type_string == "ALL" ) {
                    armor[bp.id] += res;
                } else if( bp.limbtypes.count( io::string_to_enum<body_part_type::type>( type_string ) ) > 0 ) {
                    armor[bp.id] += res * bp.limbtypes.at( io::string_to_enum<body_part_type::type>( type_string ) );
                }
            }
        }
        // Set damage resistances for specific body parts
        for( const std::string &part_string : ao.get_tags( "parts" ) ) {
            armor[bodypart_str_id( part_string )] += res;
        }
    }

    for( const JsonValue jv : jo.get_array( "integrated_armor" ) ) {
        integrated_armor.emplace_back( jv );
    }

    for( JsonMember member : jo.get_object( "bionic_slot_bonuses" ) ) {
        bionic_slot_bonuses[bodypart_str_id( member.name() )] = member.get_int();
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

int mutation_branch::bionic_slot_bonus( const bodypart_str_id &part ) const
{
    const auto iter = bionic_slot_bonuses.find( part );
    if( iter == bionic_slot_bonuses.end() ) {
        return 0;
    } else {
        return iter->second;
    }
}

std::string mutation_branch::spawn_item_message() const
{
    return raw_spawn_item_message.translated();
}

std::string mutation_branch::ranged_mutation_message() const
{
    return raw_ranged_mutation_message.translated();
}

std::string mutation_branch::name( const std::string &variant ) const
{
    auto variter = variants.find( variant );
    if( variant.empty() || variter == variants.end() ) {
        return raw_name.translated();
    }

    return variter->second.alt_name.translated();
}

std::string mutation_branch::desc( const std::string &variant ) const
{
    auto variter = variants.find( variant );
    if( variant.empty() || variter == variants.end() ) {
        return raw_desc.translated();
    }

    if( variter->second.append_desc ) {
        return string_format( "%s  %s", raw_desc.translated(),
                              variter->second.alt_description.translated() );
    }
    return variter->second.alt_description.translated();
}

static bool has_cyclic_dependency( const trait_id &mid, std::vector<trait_id> already_visited )
{
    if( contains_trait( already_visited, mid ) ) {
        return true; // Circular dependency was found.
    }

    already_visited.push_back( mid );

    // Perform Depth-first search
    for( const trait_id &current_mutation : mid->prereqs ) {
        if( has_cyclic_dependency( current_mutation->id, already_visited ) ) {
            return true;
        }
    }

    for( const trait_id &current_mutation : mid->prereqs2 ) {
        if( has_cyclic_dependency( current_mutation->id, already_visited ) ) {
            return true;
        }
    }

    return false;
}

static void check_has_cyclic_dependency( const trait_id &mid )
{
    std::vector<trait_id> already_visited;
    if( has_cyclic_dependency( mid, already_visited ) ) {
        debugmsg( "mutation %s references itself in either of its prerequsition branches.  The program will crash if the player gains this mutation.",
                  mid.c_str() );
    }
}

static void check_consistency( const std::vector<trait_id> &mvec, const trait_id &mid,
                               const std::string &what )
{
    for( const auto &m : mvec ) {
        if( !m.is_valid() ) {
            debugmsg( "mutation %s refers to undefined %s %s", mid.c_str(), what.c_str(), m.c_str() );
        }

        if( m == mid ) {
            debugmsg( "mutation %s refers to itself in %s context.  The program will crash if the player gains this mutation.",
                      mid.c_str(), what.c_str() );
        }
    }
}

void mutation_branch::check_consistency()
{
    for( const mutation_branch &mdata : get_all() ) {
        const trait_id &mid = mdata.id;
        const mod_id &trait_source = mdata.src.back().second;
        const bool basegame_trait = trait_source.str() == "dda";
        const std::optional<scenttype_id> &s_id = mdata.scent_typeid;
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
        for( const std::pair<const species_id, int> &elem : an_id ) {
            if( !elem.first.is_valid() ) {
                debugmsg( "mutation %s refers to undefined species id %s", mid.c_str(), elem.first.c_str() );
            }
        }
        if( s_id && !s_id.value().is_valid() ) {
            debugmsg( "mutation %s refers to undefined scent type %s", mid.c_str(), s_id.value().c_str() );
        }
        // Suppress these onload warnings for overlapping mods
        for( const trait_id &replacement : mdata.replacements ) {
            const mutation_branch &rdata = replacement.obj();
            bool suppressed = rdata.src.back().second != trait_source && !basegame_trait;
            for( const mutation_category_id &cat : rdata.category ) {
                if( std::find( mdata.category.begin(), mdata.category.end(), cat ) == mdata.category.end() &&
                    !suppressed ) {
                    debugmsg( "mutation %s lacks category %s present in replacement mutation %s", mid.c_str(),
                              cat.c_str(), replacement.c_str() );
                }
            }
        }
        for( const trait_id &addition : mdata.additions ) {
            if( mdata.category.empty() ) {
                break;
            }
            const mutation_branch &adata = addition.obj();
            bool found = false;
            bool suppressed = adata.src.back().second != trait_source && !basegame_trait;
            for( const mutation_category_id &cat : adata.category ) {
                found = found ||
                        std::find( mdata.category.begin(), mdata.category.end(), cat ) != mdata.category.end();
            }
            if( !found && !suppressed ) {
                debugmsg( "categories in mutation %s don't match any category present in additive mutation %s",
                          mid.c_str(), addition.c_str() );
            }
        }

        for( const mutation_category_id &cat_id : mdata.category ) {
            if( !mdata.prereqs.empty() ) {
                bool found = false;
                bool suppressed = false;
                for( const trait_id &prereq_id : mdata.prereqs ) {
                    const mutation_branch &prereq = prereq_id.obj();
                    suppressed = suppressed || ( prereq.src.back().second != trait_source && !basegame_trait );
                    found = found ||
                            std::find( prereq.category.begin(), prereq.category.end(), cat_id ) != prereq.category.end();
                }
                if( !found && !suppressed ) {
                    debugmsg( "mutation %s is in category %s but none of its slot 1 prereqs have this category",
                              mid.c_str(), cat_id.c_str() );
                }
            }

            if( !mdata.prereqs2.empty() ) {
                bool found = false;
                bool suppressed = false;
                for( const trait_id &prereq_id : mdata.prereqs2 ) {
                    const mutation_branch &prereq = prereq_id.obj();
                    suppressed = suppressed || ( prereq.src.back().second != trait_source && !basegame_trait );
                    found = found ||
                            std::find( prereq.category.begin(), prereq.category.end(), cat_id ) != prereq.category.end();
                }
                if( !found && !suppressed ) {
                    debugmsg( "mutation %s is in category %s but none of its slot 2 prereqs have this category",
                              mid.c_str(), cat_id.c_str() );
                }
            }
        }

        for( const std::pair<const bodypart_str_id, resistances> &ma : mdata.armor ) {
            for( const std::pair<const damage_type_id, float> &dt : ma.second.resist_vals ) {
                if( !dt.first.is_valid() ) {
                    debugmsg( "Invalid armor type \"%s\" for mutation %s", dt.first.c_str(), mdata.id.c_str() );
                }
            }
        }

        for( const mut_attack &atk : mdata.attacks_granted ) {
            for( const damage_unit &dt : atk.base_damage.damage_units ) {
                if( !dt.type.is_valid() ) {
                    debugmsg( "Invalid base_damage type \"%s\" for a mutation attack in mutation %s", dt.type.c_str(),
                              mdata.id.c_str() );
                }
            }
            for( const damage_unit &dt : atk.strength_damage.damage_units ) {
                if( !dt.type.is_valid() ) {
                    debugmsg( "Invalid strength_damage type \"%s\" for a mutation attack in mutation %s",
                              dt.type.c_str(), mdata.id.c_str() );
                }
            }
        }

        // We need to display active mutations in the UI.
        if( mdata.activated && !mdata.player_display ) {
            debugmsg( "mutation %s is not displayed but set as active" );
        }

        check_has_cyclic_dependency( mid );

        ::check_consistency( mdata.prereqs, mid, "prereqs" );
        ::check_consistency( mdata.prereqs2, mid, "prereqs2" );
        ::check_consistency( mdata.threshreq, mid, "threshreq" );
        ::check_consistency( mdata.cancels, mid, "cancels" );
        ::check_consistency( mdata.replacements, mid, "replacements" );
        ::check_consistency( mdata.additions, mid, "additions" );
    }
}

nc_color mutation_branch::get_display_color() const
{
    if( flags.count( STATIC( json_character_flag( "ATTUNEMENT" ) ) ) ) {
        return c_green;
    } else if( threshold || profession ) {
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

const mutation_variant *mutation_branch::pick_variant() const
{
    weighted_int_list<const mutation_variant *> options;
    for( const std::pair<const std::string, mutation_variant> &cur : variants ) {
        options.add( &cur.second, cur.second.weight );
    }

    const mutation_variant **picked = options.pick();

    if( picked == nullptr ) {
        return nullptr;
    }
    return *picked;
}

const mutation_variant *mutation_branch::variant( const std::string &id ) const
{
    auto iter = variants.find( id );
    if( id.empty() || iter == variants.end() ) {
        return nullptr;
    }

    return &iter->second;
}

const mutation_variant *mutation_branch::pick_variant_menu() const
{
    if( variants.empty() ) {
        return nullptr;
    }
    uilist menu;
    menu.allow_cancel = false;
    menu.desc_enabled = true;
    menu.text = string_format( _( "Pick variant for: %s" ), name() );
    std::vector<const mutation_variant *> options;
    options.reserve( variants.size() );
    for( const std::pair<const std::string, mutation_variant> &var : variants ) {
        options.emplace_back( &var.second );
    }
    for( size_t i = 0; i < options.size(); ++i ) {
        menu.addentry_desc( i, true, MENU_AUTOASSIGN, name( options[i]->id ), desc( options[i]->id ) );
    }
    menu.query();

    return options[menu.ret];
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
    trait_groups.emplace( Trait_group_EMPTY_GROUP,
                          make_shared_fast<Trait_group_collection>( 100 ) );
}

std::vector<std::string> dream::messages() const
{
    std::vector<std::string> ret;
    ret.reserve( raw_messages.size() );
    for( const translation &msg : raw_messages ) {
        ret.push_back( msg.translated() );
    }
    return ret;
}

void dream::load( const JsonObject &jsobj )
{
    dream newdream;

    newdream.strength = jsobj.get_int( "strength" );
    jsobj.read( "category", newdream.category, true );
    jsobj.read( "messages", newdream.raw_messages );

    dreams.push_back( newdream );
}

bool trait_var_display_sort( const trait_and_var &a, const trait_and_var &b ) noexcept
{
    auto trait_sort_key = []( const trait_and_var & t ) {
        return std::make_pair( -t.trait->get_display_color().to_int(), t.name() );
    };

    return localized_compare( trait_sort_key( a ), trait_sort_key( b ) );
}

bool trait_display_sort( const trait_id &a, const trait_id &b ) noexcept
{
    auto trait_sort_key = []( const trait_id & t ) {
        return std::make_pair( -t->get_display_color().to_int(), t->name() );
    };

    return localized_compare( trait_sort_key( a ), trait_sort_key( b ) );
}

bool trait_var_display_nocolor_sort( const trait_and_var &a, const trait_and_var &b ) noexcept
{
    return localized_compare( a.name(), b.name() );
}

bool trait_display_nocolor_sort( const trait_id &a, const trait_id &b ) noexcept
{
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

void mutation_branch::load_trait_migration( const JsonObject &jo )
{
    // First, load the target
    trait_id target;
    jo.read( "id", target );

    // Then, populate the replacement
    trait_replacement replacement;

    if( jo.has_string( "proficiency" ) ) {
        jo.read( "proficiency", replacement.prof );
    } else if( jo.has_string( "trait" ) ) {
        trait_and_var dat;
        jo.read( "trait", dat.trait );
        jo.read( "variant", dat.variant, "" );
        replacement.trait = dat;
    } else if( !jo.has_bool( "remove" ) || !jo.get_bool( "remove" ) ) {
        jo.throw_error( "Specified migration for trait " + target.str() +
                        ", but did not specify how to migrate!" );
    }

    trait_migrations.emplace( target, replacement );
}

const trait_replacement &mutation_branch::trait_migration( const trait_id &tid )
{
    auto it = trait_migrations.find( tid );
    if( it != trait_migrations.end() ) {
        return it->second;
    }

    return trait_migration_remove;
}

void mutation_branch::finalize()
{
    for( auto &armr : armor ) {
        finalize_damage_map( armr.second.resist_vals );
    }
}

void mutation_branch::finalize_all()
{
    trait_factory.finalize();
    for( const mutation_branch &branch : get_all() ) {
        for( const mutation_category_id &cat : branch.category ) {
            mutations_category[cat].emplace_back( branch.id );
        }
        // Don't include dummy mutations for the ANY category, since they have a very specific use case
        // Otherwise, the system will prioritize them
        if( !branch.dummy ) {
            mutations_category[mutation_category_ANY].emplace_back( branch.id );
        }
    }
    finalize_trait_blacklist();
}

void mutation_branch::finalize_trait_blacklist()
{
    for( const auto &trait : trait_blacklist ) {
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
            tg.add_entry( std::make_unique<Single_trait_creator>( id, "", subarr.get_int( 1 ) ) );
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
        jsobj.throw_error_at( "subtype", "unknown trait group type" );
    }

    Trait_group &tg = make_group_or_throw( gid, subtype == "collection" || subtype == "old" );

    // TODO: (sm) Looks like this makes the new code backwards-compatible with the old format. Great if so!
    if( subtype == "old" ) {
        for( JsonArray pair : jsobj.get_array( "traits" ) ) {
            tg.add_trait_entry( trait_id( pair.get_string( 0 ) ), "", pair.get_int( 1 ) );
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
                tg.add_trait_entry( trait_id( entry.get_string() ), "", 100 );
            } else if( entry.test_array() ) {
                JsonArray subtrait = entry.get_array();
                tg.add_trait_entry( trait_id( subtrait.get_string( 0 ) ), "", subtrait.get_int( 1 ) );
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
        std::string var;
        if( obj.has_member( "variant" ) ) {
            var = obj.get_string( "variant" );
        }
        ptr = std::make_unique<Single_trait_creator>( id, var, probability );
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
    rval.reserve( trait_groups.size() );
    for( auto &group : trait_groups ) {
        rval.push_back( group.first );
    }
    return rval;
}

bool mutation_category_is_valid( const mutation_category_id &cat )
{
    return mutation_category_traits.count( cat );
}

template<>
bool string_id<mutation_category_trait>::is_valid() const
{
    return mutation_category_traits.count( *this );
}

bool mutation_is_in_category( const trait_id &mut, const mutation_category_id &cat )
{
    return std::find( mutations_category[cat].begin(), mutations_category[cat].end(),
                      mut ) != mutations_category[cat].end();
}
