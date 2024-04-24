#include "magic.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <set>
#include <utility>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "color.h"
#include "condition.h"
#include "creature.h"
#include "creature_tracker.h"
#include "cursesdef.h"
#include "damage.h"
#include "debug.h"
#include "enum_conversions.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "field.h"
#include "generic_factory.h"
#include "input_context.h"
#include "inventory.h"
#include "item.h"
#include "json.h"
#include "line.h"
#include "localized_comparator.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "output.h"
#include "pimpl.h"
#include "point.h"
#include "projectile.h"
#include "requirements.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui.h"
#include "units.h"

static const json_character_flag json_flag_NO_PSIONICS( "NO_PSIONICS" );
static const json_character_flag json_flag_NO_SPELLCASTING( "NO_SPELLCASTING" );
static const json_character_flag json_flag_SILENT_SPELL( "SILENT_SPELL" );
static const json_character_flag json_flag_SUBTLE_SPELL( "SUBTLE_SPELL" );

static const skill_id skill_spellcraft( "spellcraft" );

static const trait_id trait_NONE( "NONE" );

static std::string target_to_string( spell_target data )
{
    switch( data ) {
        case spell_target::ally:
            return pgettext( "Valid spell target", "ally" );
        case spell_target::hostile:
            return pgettext( "Valid spell target", "hostile" );
        case spell_target::self:
            return pgettext( "Valid spell target", "self" );
        case spell_target::ground:
            return pgettext( "Valid spell target", "ground" );
        case spell_target::none:
            return pgettext( "Valid spell target", "none" );
        case spell_target::item:
            return pgettext( "Valid spell target", "item" );
        case spell_target::field:
            return pgettext( "Valid spell target", "field" );
        case spell_target::num_spell_targets:
            break;
    }
    debugmsg( "Invalid valid_target" );
    return "THIS IS A BUG";
}

namespace io
{
// *INDENT-OFF*
template<>
std::string enum_to_string<spell_target>( spell_target data )
{
    switch( data ) {
        case spell_target::ally: return "ally";
        case spell_target::hostile: return "hostile";
        case spell_target::self: return "self";
        case spell_target::ground: return "ground";
        case spell_target::none: return "none";
        case spell_target::item: return "item";
        case spell_target::field: return "field";
        case spell_target::num_spell_targets: break;
    }
    cata_fatal( "Invalid valid_target" );
}
template<>
std::string enum_to_string<spell_shape>( spell_shape data )
{
    switch( data ) {
        case spell_shape::blast: return "blast";
        case spell_shape::cone: return "cone";
        case spell_shape::line: return "line";
        case spell_shape::num_shapes: break;
    }
    cata_fatal( "Invalid spell_shape" );
}
template<>
std::string enum_to_string<spell_flag>( spell_flag data )
{
    switch( data ) {
        case spell_flag::PERMANENT: return "PERMANENT";
        case spell_flag::PERMANENT_ALL_LEVELS: return "PERMANENT_ALL_LEVELS";
        case spell_flag::PERCENTAGE_DAMAGE: return "PERCENTAGE_DAMAGE";
        case spell_flag::IGNORE_WALLS: return "IGNORE_WALLS";
        case spell_flag::NO_PROJECTILE: return "NO_PROJECTILE";
        case spell_flag::HOSTILE_SUMMON: return "HOSTILE_SUMMON";
        case spell_flag::HOSTILE_50: return "HOSTILE_50";
        case spell_flag::FRIENDLY_POLY: return "FRIENDLY_POLY";
        case spell_flag::POLYMORPH_GROUP: return "POLYMORPH_GROUP";
        case spell_flag::SILENT: return "SILENT";
        case spell_flag::NO_EXPLOSION_SFX: return "NO_EXPLOSION_SFX";
        case spell_flag::LOUD: return "LOUD";
        case spell_flag::VERBAL: return "VERBAL";
        case spell_flag::SOMATIC: return "SOMATIC";
        case spell_flag::NO_HANDS: return "NO_HANDS";
        case spell_flag::NO_LEGS: return "NO_LEGS";
        case spell_flag::UNSAFE_TELEPORT: return "UNSAFE_TELEPORT";
        case spell_flag::TARGET_TELEPORT: return "TARGET_TELEPORT";
        case spell_flag::SWAP_POS: return "SWAP_POS";
        case spell_flag::CONCENTRATE: return "CONCENTRATE";
        case spell_flag::RANDOM_AOE: return "RANDOM_AOE";
        case spell_flag::RANDOM_DAMAGE: return "RANDOM_DAMAGE";
        case spell_flag::RANDOM_DURATION: return "RANDOM_DURATION";
        case spell_flag::RANDOM_TARGET: return "RANDOM_TARGET";
        case spell_flag::RANDOM_CRITTER: return "RANDOM_CRITTER";
        case spell_flag::MUTATE_TRAIT: return "MUTATE_TRAIT";
        case spell_flag::PAIN_NORESIST: return "PAIN_NORESIST";
        case spell_flag::SPAWN_GROUP: return "SPAWN_GROUP";
        case spell_flag::IGNITE_FLAMMABLE: return "IGNITE_FLAMMABLE";
        case spell_flag::NO_FAIL: return "NO_FAIL";
        case spell_flag::WONDER: return "WONDER";
        case spell_flag::EXTRA_EFFECTS_FIRST: return "EXTRA_EFFECTS_FIRST";
        case spell_flag::MUST_HAVE_CLASS_TO_LEARN: return "MUST_HAVE_CLASS_TO_LEARN";
        case spell_flag::SPAWN_WITH_DEATH_DROPS: return "SPAWN_WITH_DEATH_DROPS";
        case spell_flag::NO_CORPSE_QUIET: return "NO_CORPSE_QUIET";
        case spell_flag::NON_MAGICAL: return "NON_MAGICAL";
        case spell_flag::PSIONIC: return "PSIONIC";
        case spell_flag::RECHARM: return "RECHARM";
        case spell_flag::LAST: break;
    }
    cata_fatal( "Invalid spell_flag" );
}
template<>
std::string enum_to_string<magic_energy_type>( magic_energy_type data )
{
    switch( data ) {
    case magic_energy_type::bionic: return "BIONIC";
    case magic_energy_type::hp: return "HP";
    case magic_energy_type::mana: return "MANA";
    case magic_energy_type::none: return "NONE";
    case magic_energy_type::stamina: return "STAMINA";
    case magic_energy_type::last: break;
    }
    cata_fatal( "Invalid magic_energy_type" );
}
// *INDENT-ON*

} // namespace io

const std::optional<int> fake_spell::max_level_default = std::nullopt;
const int fake_spell::level_default = 0;
const bool fake_spell::self_default = false;
const int fake_spell::trigger_once_in_default = 1;

const skill_id spell_type::skill_default = skill_spellcraft;
// empty string
const requirement_id spell_type::spell_components_default;
const translation spell_type::message_default = to_translation( "You cast %s!" );
const translation spell_type::sound_description_default = to_translation( "an explosion." );
const sounds::sound_t spell_type::sound_type_default = sounds::sound_t::combat;
const bool spell_type::sound_ambient_default = false;
// empty string
const std::string spell_type::sound_id_default;
const std::string spell_type::sound_variant_default = "default";
// empty string
const std::string spell_type::effect_str_default;
const std::optional<field_type_id> spell_type::field_default = std::nullopt;
const int spell_type::field_chance_default = 1;
const int spell_type::min_field_intensity_default = 0;
const int spell_type::max_field_intensity_default = 0;
const float spell_type::field_intensity_increment_default = 0.0f;
const float spell_type::field_intensity_variance_default = 0.0f;
const int spell_type::min_accuracy_default = 20;
const float spell_type::accuracy_increment_default = 0.0f;
const int spell_type::max_accuracy_default = 20;
const int spell_type::min_damage_default = 0;
const float spell_type::damage_increment_default = 0.0f;
const int spell_type::max_damage_default = 0;
const int spell_type::min_range_default = 0;
const float spell_type::range_increment_default = 0.0f;
const int spell_type::max_range_default = 0;
const int spell_type::min_aoe_default = 0;
const float spell_type::aoe_increment_default = 0.0f;
const int spell_type::max_aoe_default = 0;
const int spell_type::min_dot_default = 0;
const float spell_type::dot_increment_default = 0.0f;
const int spell_type::max_dot_default = 0;
const int spell_type::min_duration_default = 0;
const int spell_type::duration_increment_default = 0;
const int spell_type::max_duration_default = 0;
const int spell_type::min_pierce_default = 0;
const float spell_type::pierce_increment_default = 0.0f;
const int spell_type::max_pierce_default = 0;
const int spell_type::base_energy_cost_default = 0;
const float spell_type::energy_increment_default = 0.0f;
const trait_id spell_type::spell_class_default = trait_NONE;
const magic_energy_type spell_type::energy_source_default = magic_energy_type::none;
const damage_type_id spell_type::dmg_type_default = damage_type_id::NULL_ID();
const int spell_type::difficulty_default = 0;
const int spell_type::max_level_default = 0;
const int spell_type::base_casting_time_default = 0;
const float spell_type::casting_time_increment_default = 0.0f;

// LOADING
// spell_type

namespace
{
generic_factory<spell_type> spell_factory( "spell" );
} // namespace

template<>
const spell_type &string_id<spell_type>::obj() const
{
    return spell_factory.obj( *this );
}

template<>
bool string_id<spell_type>::is_valid() const
{
    return spell_factory.is_valid( *this );
}

void spell_type::load_spell( const JsonObject &jo, const std::string &src )
{
    spell_factory.load( jo, src );
}

static std::string moves_to_string( const int moves )
{
    if( moves < to_moves<int>( 2_seconds ) ) {
        return string_format( n_gettext( "%d move", "%d moves", moves ), moves );
    } else {
        return to_string( time_duration::from_moves( moves ) );
    }
}

void spell_type::load( const JsonObject &jo, const std::string_view src )
{
    src_mod = mod_id( src );
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "description", description );
    optional( jo, was_loaded, "skill", skill, skill_default );
    optional( jo, was_loaded, "teachable", teachable, true );
    optional( jo, was_loaded, "components", spell_components, spell_components_default );
    optional( jo, was_loaded, "message", message, message_default );
    optional( jo, was_loaded, "sound_description", sound_description, sound_description_default );
    optional( jo, was_loaded, "sound_type", sound_type, sound_type_default );
    optional( jo, was_loaded, "sound_ambient", sound_ambient, sound_ambient_default );
    optional( jo, was_loaded, "sound_id", sound_id, sound_id_default );
    optional( jo, was_loaded, "sound_variant", sound_variant, sound_variant_default );
    mandatory( jo, was_loaded, "effect", effect_name );
    const auto found_effect = spell_effect::effect_map.find( effect_name );
    if( found_effect == spell_effect::effect_map.cend() ) {
        effect = spell_effect::none;
        debugmsg( "ERROR: spell %s has invalid effect %s", id.c_str(), effect_name );
    } else {
        effect = found_effect->second;
    }
    mandatory( jo, was_loaded, "shape", spell_area );
    spell_area_function = spell_effect::shape_map.at( spell_area );

    const auto targeted_monster_ids_reader = string_id_reader<::mtype> {};
    optional( jo, was_loaded, "targeted_monster_ids", targeted_monster_ids,
              targeted_monster_ids_reader );

    const auto targeted_monster_species_reader = string_id_reader<::species_type> {};
    optional( jo, was_loaded, "targeted_monster_species", targeted_species_ids,
              targeted_monster_species_reader );

    const auto ignored_monster_species_reader = string_id_reader<::species_type> {};
    optional( jo, was_loaded, "ignored_monster_species", ignored_species_ids,
              ignored_monster_species_reader );




    const auto trigger_reader = enum_flags_reader<spell_target> { "valid_targets" };
    mandatory( jo, was_loaded, "valid_targets", valid_targets, trigger_reader );

    optional( jo, was_loaded, "extra_effects", additional_spells );

    optional( jo, was_loaded, "affected_body_parts", affected_bps );

    if( jo.has_array( "flags" ) ) {
        for( auto &flag : jo.get_string_array( "flags" ) ) {
            // Save all provided flags as strings in spell_type.flags
            // If the flag is listed as a possible enum of type spell_flag, we also save it to spell_type.spell_tags
            flags.insert( flag );
            std::optional<spell_flag> f = io::string_to_enum_optional<spell_flag>( flag );
            if( f.has_value() ) {
                spell_tags.set( f.value() );
            }
        }
    } else if( jo.has_string( "flags" ) ) {
        const std::string flag = jo.get_string( "flags" );
        flags.insert( flag );
        std::optional<spell_flag> f = io::string_to_enum_optional<spell_flag>( flag );
        if( f.has_value() ) {
            spell_tags.set( f.value() );
        }
    }

    optional( jo, was_loaded, "effect_str", effect_str, effect_str_default );

    std::string field_input;
    // Because the field this is loading into is not part of this type,
    // the default value will not be supplied when using copy-from if we pass was_loaded
    // So just pass false instead
    optional( jo, false, "field_id", field_input, "none" );
    if( field_input != "none" ) {
        field = field_type_id( field_input );
    }
    if( !was_loaded || jo.has_member( "field_chance" ) ) {
        field_chance = get_dbl_or_var( jo, "field_chance", false, field_chance_default );
    }
    if( !was_loaded || jo.has_member( "min_field_intensity" ) ) {
        min_field_intensity = get_dbl_or_var( jo, "min_field_intensity", false,
                                              min_field_intensity_default );
    }
    if( !was_loaded || jo.has_member( "max_field_intensity" ) ) {
        max_field_intensity = get_dbl_or_var( jo, "max_field_intensity", false,
                                              max_field_intensity_default );
    }
    if( !was_loaded || jo.has_member( "field_intensity_increment" ) ) {
        field_intensity_increment = get_dbl_or_var( jo, "field_intensity_increment", false,
                                    field_intensity_increment_default );
    }
    if( !was_loaded || jo.has_member( "field_intensity_variance" ) ) {
        field_intensity_variance = get_dbl_or_var( jo, "field_intensity_variance", false,
                                   field_intensity_variance_default );
    }

    if( !was_loaded || jo.has_member( "min_accuracy" ) ) {
        min_accuracy = get_dbl_or_var( jo, "min_accuracy", false, min_accuracy_default );
    }
    if( !was_loaded || jo.has_member( "accuracy_increment" ) ) {
        accuracy_increment = get_dbl_or_var( jo, "accuracy_increment", false,
                                             accuracy_increment_default );
    }
    if( !was_loaded || jo.has_member( "max_accuracy" ) ) {
        max_accuracy = get_dbl_or_var( jo, "max_accuracy", false, max_accuracy_default );
    }
    if( !was_loaded || jo.has_member( "min_damage" ) ) {
        min_damage = get_dbl_or_var( jo, "min_damage", false, min_damage_default );
    }
    if( !was_loaded || jo.has_member( "damage_increment" ) ) {
        damage_increment = get_dbl_or_var( jo, "damage_increment", false,
                                           damage_increment_default );
    }
    if( !was_loaded || jo.has_member( "max_damage" ) ) {
        max_damage = get_dbl_or_var( jo, "max_damage", false, max_damage_default );
    }

    if( !was_loaded || jo.has_member( "min_range" ) ) {
        min_range = get_dbl_or_var( jo, "min_range", false, min_range_default );
    }
    if( !was_loaded || jo.has_member( "range_increment" ) ) {
        range_increment = get_dbl_or_var( jo, "range_increment", false, range_increment_default );
    }
    if( !was_loaded || jo.has_member( "max_range" ) ) {
        max_range = get_dbl_or_var( jo, "max_range", false, max_range_default );
    }

    if( !was_loaded || jo.has_member( "min_aoe" ) ) {
        min_aoe = get_dbl_or_var( jo, "min_aoe", false, min_aoe_default );
    }
    if( !was_loaded || jo.has_member( "aoe_increment" ) ) {
        aoe_increment = get_dbl_or_var( jo, "aoe_increment", false, aoe_increment_default );
    }
    if( !was_loaded || jo.has_member( "max_aoe" ) ) {
        max_aoe = get_dbl_or_var( jo, "max_aoe", false, max_aoe_default );
    }
    if( !was_loaded || jo.has_member( "min_dot" ) ) {
        min_dot = get_dbl_or_var( jo, "min_dot", false, min_dot_default );
    }
    if( !was_loaded || jo.has_member( "dot_increment" ) ) {
        dot_increment = get_dbl_or_var( jo, "dot_increment", false, dot_increment_default );
    }
    if( !was_loaded || jo.has_member( "max_dot" ) ) {
        max_dot = get_dbl_or_var( jo, "max_dot", false, max_dot_default );
    }

    if( !was_loaded || jo.has_member( "min_duration" ) ) {
        min_duration = get_dbl_or_var( jo, "min_duration", false, min_duration_default );
    }
    if( !was_loaded || jo.has_member( "duration_increment" ) ) {
        duration_increment = get_dbl_or_var( jo, "duration_increment", false,
                                             duration_increment_default );
    }
    if( !was_loaded || jo.has_member( "max_duration" ) ) {
        max_duration = get_dbl_or_var( jo, "max_duration", false, max_duration_default );
    }

    if( !was_loaded || jo.has_member( "min_pierce" ) ) {
        min_pierce = get_dbl_or_var( jo, "min_pierce", false, min_pierce_default );
    }
    if( !was_loaded || jo.has_member( "pierce_increment" ) ) {
        pierce_increment = get_dbl_or_var( jo, "pierce_increment", false,
                                           pierce_increment_default );
    }
    if( !was_loaded || jo.has_member( "max_pierce" ) ) {
        max_pierce = get_dbl_or_var( jo, "max_pierce", false, max_pierce_default );
    }

    if( !was_loaded || jo.has_member( "base_energy_cost" ) ) {
        base_energy_cost = get_dbl_or_var( jo, "base_energy_cost", false,
                                           base_energy_cost_default );
    }
    if( jo.has_member( "final_energy_cost" ) ) {
        final_energy_cost = get_dbl_or_var( jo, "final_energy_cost" );
    } else if( !was_loaded ) {
        final_energy_cost = base_energy_cost;
    }
    if( !was_loaded || jo.has_member( "energy_increment" ) ) {
        energy_increment = get_dbl_or_var( jo, "energy_increment", false,
                                           energy_increment_default );
    }

    optional( jo, was_loaded, "spell_class", spell_class, spell_class_default );
    optional( jo, was_loaded, "energy_source", energy_source, energy_source_default );
    optional( jo, was_loaded, "damage_type", dmg_type, dmg_type_default );
    if( !was_loaded || jo.has_member( "difficulty" ) ) {
        difficulty = get_dbl_or_var( jo, "difficulty", false, difficulty_default );
    }
    if( !was_loaded || jo.has_member( "max_level" ) ) {
        max_level = get_dbl_or_var( jo, "max_level", false, max_level_default );
    }

    if( !was_loaded || jo.has_member( "base_casting_time" ) ) {
        base_casting_time = get_dbl_or_var( jo, "base_casting_time", false,
                                            base_casting_time_default );
    }
    if( jo.has_member( "final_casting_time" ) ) {
        final_casting_time = get_dbl_or_var( jo, "final_casting_time" );
    } else if( !was_loaded ) {
        final_casting_time = base_casting_time;
    }
    if( !was_loaded || jo.has_member( "max_damage" ) ) {
        max_damage = get_dbl_or_var( jo, "max_damage", false, max_damage_default );
    }
    if( !was_loaded || jo.has_member( "casting_time_increment" ) ) {
        casting_time_increment = get_dbl_or_var( jo, "casting_time_increment", false,
                                 casting_time_increment_default );
    }

    for( const JsonMember member : jo.get_object( "learn_spells" ) ) {
        learn_spells.insert( std::pair<std::string, int>( member.name(), member.get_int() ) );
    }
}

void spell_type::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "type", "SPELL" );
    json.member( "id", id );
    json.member( "src_mod", src_mod );
    json.member( "name", name.translated() );
    json.member( "description", description.translated() );
    json.member( "effect", effect_name );
    json.member( "shape", io::enum_to_string( spell_area ) );
    json.member( "valid_targets", valid_targets, enum_bitset<spell_target> {} );
    json.member( "effect_str", effect_str, effect_str_default );
    json.member( "skill", skill, skill_default );
    json.member( "teachable", teachable, true );
    json.member( "components", spell_components, spell_components_default );
    json.member( "message", message.translated(), message_default.translated() );
    json.member( "sound_description", sound_description.translated(),
                 sound_description_default.translated() );
    json.member( "sound_type", io::enum_to_string( sound_type ),
                 io::enum_to_string( sound_type_default ) );
    json.member( "sound_ambient", sound_ambient, sound_ambient_default );
    json.member( "sound_id", sound_id, sound_id_default );
    json.member( "sound_variant", sound_variant, sound_variant_default );
    json.member( "targeted_monster_ids", targeted_monster_ids, std::set<mtype_id> {} );
    json.member( "targeted_monster_species", targeted_species_ids, std::set<species_id> {} );
    json.member( "ignored_monster_species", ignored_species_ids, std::set<species_id> {} );
    json.member( "extra_effects", additional_spells, std::vector<fake_spell> {} );
    if( !affected_bps.none() ) {
        json.member( "affected_body_parts", affected_bps );
    }
    json.member( "flags", flags, std::set<std::string> {} );
    if( field ) {
        json.member( "field_id", field->id().str() );
        json.member( "field_chance", static_cast<int>( field_chance.min.dbl_val.value() ),
                     field_chance_default );
        json.member( "max_field_intensity", static_cast<int>( max_field_intensity.min.dbl_val.value() ),
                     max_field_intensity_default );
        json.member( "min_field_intensity", static_cast<int>( min_field_intensity.min.dbl_val.value() ),
                     min_field_intensity_default );
        json.member( "field_intensity_increment",
                     static_cast<float>( field_intensity_increment.min.dbl_val.value() ),
                     field_intensity_increment_default );
        json.member( "field_intensity_variance",
                     static_cast<float>( field_intensity_variance.min.dbl_val.value() ),
                     field_intensity_variance_default );
    }
    json.member( "min_damage", static_cast<int>( min_damage.min.dbl_val.value() ), min_damage_default );
    json.member( "max_damage", static_cast<int>( max_damage.min.dbl_val.value() ), max_damage_default );
    json.member( "damage_increment", static_cast<float>( damage_increment.min.dbl_val.value() ),
                 damage_increment_default );
    json.member( "min_accuracy", static_cast<int>( min_accuracy.min.dbl_val.value() ),
                 min_accuracy_default );
    json.member( "accuracy_increment", static_cast<float>( accuracy_increment.min.dbl_val.value() ),
                 accuracy_increment_default );
    json.member( "max_accuracy", static_cast<int>( max_accuracy.min.dbl_val.value() ),
                 max_accuracy_default );
    json.member( "min_range", static_cast<int>( min_range.min.dbl_val.value() ), min_range_default );
    json.member( "max_range", static_cast<int>( max_range.min.dbl_val.value() ), min_range_default );
    json.member( "range_increment", static_cast<float>( range_increment.min.dbl_val.value() ),
                 range_increment_default );
    json.member( "min_aoe", static_cast<int>( min_aoe.min.dbl_val.value() ), min_aoe_default );
    json.member( "max_aoe", static_cast<int>( max_aoe.min.dbl_val.value() ), max_aoe_default );
    json.member( "aoe_increment", static_cast<float>( aoe_increment.min.dbl_val.value() ),
                 aoe_increment_default );
    json.member( "min_dot", static_cast<int>( min_dot.min.dbl_val.value() ), min_dot_default );
    json.member( "max_dot", static_cast<int>( max_dot.min.dbl_val.value() ), max_dot_default );
    json.member( "dot_increment", static_cast<float>( dot_increment.min.dbl_val.value() ),
                 dot_increment_default );
    json.member( "min_duration", static_cast<int>( min_duration.min.dbl_val.value() ),
                 min_duration_default );
    json.member( "max_duration", static_cast<int>( max_duration.min.dbl_val.value() ),
                 max_duration_default );
    json.member( "duration_increment", static_cast<int>( duration_increment.min.dbl_val.value() ),
                 duration_increment_default );
    json.member( "min_pierce", static_cast<int>( min_pierce.min.dbl_val.value() ), min_pierce_default );
    json.member( "max_pierce", static_cast<int>( max_pierce.min.dbl_val.value() ), max_pierce_default );
    json.member( "pierce_increment", static_cast<float>( pierce_increment.min.dbl_val.value() ),
                 pierce_increment_default );
    json.member( "base_energy_cost", static_cast<int>( base_energy_cost.min.dbl_val.value() ),
                 base_energy_cost_default );
    json.member( "final_energy_cost", static_cast<int>( final_energy_cost.min.dbl_val.value() ),
                 static_cast<int>( base_energy_cost.min.dbl_val.value() ) );
    json.member( "energy_increment", static_cast<float>( energy_increment.min.dbl_val.value() ),
                 energy_increment_default );
    json.member( "spell_class", spell_class, spell_class_default );
    json.member( "energy_source", io::enum_to_string( energy_source ),
                 io::enum_to_string( energy_source_default ) );
    json.member( "damage_type", dmg_type, dmg_type_default );
    json.member( "difficulty", static_cast<int>( difficulty.min.dbl_val.value() ), difficulty_default );
    json.member( "max_level", static_cast<int>( max_level.min.dbl_val.value() ), max_level_default );
    json.member( "base_casting_time", static_cast<int>( base_casting_time.min.dbl_val.value() ),
                 base_casting_time_default );
    json.member( "final_casting_time", static_cast<int>( final_casting_time.min.dbl_val.value() ),
                 static_cast<int>( base_casting_time.min.dbl_val.value() ) );
    json.member( "casting_time_increment",
                 static_cast<float>( casting_time_increment.min.dbl_val.value() ), casting_time_increment_default );

    if( !learn_spells.empty() ) {
        json.member( "learn_spells" );
        json.start_object();

        for( const std::pair<const std::string, int> &sp : learn_spells ) {
            json.member( sp.first, sp.second );
        }

        json.end_object();
    }

    json.end_object();
}

static bool spell_infinite_loop_check( std::set<spell_id> spell_effects, const spell_id &sp )
{
    if( spell_effects.count( sp ) ) {
        return true;
    }
    spell_effects.emplace( sp );

    std::set<spell_id> unique_spell_list;
    for( const fake_spell &fake_sp : sp->additional_spells ) {
        unique_spell_list.emplace( fake_sp.id );
    }

    for( const spell_id &other_sp : unique_spell_list ) {
        if( spell_infinite_loop_check( spell_effects, other_sp ) ) {
            return true;
        }
    }
    return false;
}

void spell_type::check_consistency()
{
    for( const spell_type &sp_t : get_all() ) {
        if( sp_t.effect_name == "summon_vehicle" ) {
            if( !sp_t.effect_str.empty() && !vproto_id( sp_t.effect_str ).is_valid() ) {
                debugmsg( "ERROR %s specifies a vehicle to summon, but vehicle %s is not valid", sp_t.id.c_str(),
                          sp_t.effect_str );
            }
        }
        std::set<spell_id> spell_effect_list;
        if( spell_infinite_loop_check( spell_effect_list, sp_t.id ) ) {
            debugmsg( "ERROR: %s has infinite loop in extra_effects", sp_t.id.c_str() );
        }
        if( sp_t.spell_tags[spell_flag::WONDER] && sp_t.additional_spells.empty() ) {
            debugmsg( "ERROR: %s has WONDER flag but no spells to choose from!", sp_t.id.c_str() );
        }
    }
}

const std::vector<spell_type> &spell_type::get_all()
{
    return spell_factory.get_all();
}

void spell_type::reset_all()
{
    spell_factory.reset();
}

bool spell_type::is_valid() const
{
    return spell_factory.is_valid( this->id );
}

// spell

spell::spell( spell_id sp, int xp, int level_adjustment ) :
    type( sp ),
    experience( xp ),
    temp_level_adjustment( level_adjustment )
{}

void spell::set_message( const translation &msg )
{
    alt_message = msg;
}

spell_id spell::id() const
{
    return type;
}

trait_id spell::spell_class() const
{
    return type->spell_class;
}

skill_id spell::skill() const
{
    return type->skill;
}

int spell::field_intensity( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    return std::min( static_cast<int>( type->max_field_intensity.evaluate( d ) ),
                     static_cast<int>( type->min_field_intensity.evaluate( d ) + std::round( get_effective_level() *
                                       type->field_intensity_increment.evaluate( d ) ) ) );
}

int spell::min_leveled_damage( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    return type->min_damage.evaluate( d ) + std::round( get_effective_level() *
            type->damage_increment.evaluate(
                d ) );
}

float spell::dps( const Character &caster, const Creature & ) const
{
    if( type->effect_name != "attack" ) {
        return 0.0f;
    }
    const float time_modifier = 100.0f / casting_time( caster );
    const float failure_modifier = 1.0f - spell_fail( caster );
    const float raw_dps = damage( caster ) + damage_dot( caster ) * duration_turns( caster ) / 1_turns;
    // TODO: calculate true dps with armor and resistances and any caster bonuses
    return raw_dps * time_modifier * failure_modifier;
}

int spell::damage( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    const int leveled_damage = min_leveled_damage( caster );

    if( has_flag( spell_flag::RANDOM_DAMAGE ) ) {
        return rng( std::min( leveled_damage, static_cast<int>( type->max_damage.evaluate( d ) ) ),
                    std::max( leveled_damage,
                              static_cast<int>( type->max_damage.evaluate( d ) ) ) );
    } else {
        if( type->min_damage.evaluate( d ) >= 0 ||
            type->max_damage.evaluate( d ) >= type->min_damage.evaluate( d ) ) {
            return std::min( leveled_damage, static_cast<int>( type->max_damage.evaluate( d ) ) );
        } else { // if it's negative, min and max work differently
            return std::max( leveled_damage, static_cast<int>( type->max_damage.evaluate( d ) ) );
        }
    }
}

int spell::min_leveled_accuracy( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    return type->min_accuracy.evaluate( d ) + std::round( get_effective_level() *
            type->accuracy_increment.evaluate( d ) );
}

int spell::accuracy( Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    const int leveled_accuracy = min_leveled_accuracy( caster );
    if( type->min_accuracy.evaluate( d ) >= 0 ||
        type->max_accuracy.evaluate( d ) >= type->min_accuracy.evaluate( d ) ) {
        return std::min( leveled_accuracy, static_cast<int>( type->max_accuracy.evaluate( d ) ) );
    } else { // if it's negative, min and max work differently
        return std::max( leveled_accuracy, static_cast<int>( type->max_accuracy.evaluate( d ) ) );
    }
}

int spell::min_leveled_dot( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    return type->min_dot.evaluate( d ) + std::round( get_effective_level() *
            type->dot_increment.evaluate( d ) );
}

int spell::damage_dot( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    const int leveled_dot = min_leveled_dot( caster );
    if( type->min_dot.evaluate( d ) >= 0 ||
        type->max_dot.evaluate( d ) >= type->min_dot.evaluate( d ) ) {
        return std::min( leveled_dot, static_cast<int>( type->max_dot.evaluate( d ) ) );
    } else { // if it's negative, min and max work differently
        return std::max( leveled_dot, static_cast<int>( type->max_dot.evaluate( d ) ) );
    }
}

damage_over_time_data spell::damage_over_time( const std::vector<bodypart_str_id> &bps,
        const Creature &caster ) const
{
    damage_over_time_data temp;
    temp.bps = bps;
    temp.duration = duration_turns( caster );
    temp.amount = damage_dot( caster );
    temp.type = dmg_type();
    return temp;
}

std::string spell::damage_string( const Character &caster ) const
{
    std::string damage_string;
    dialogue d( get_talker_for( caster ), nullptr );
    if( has_flag( spell_flag::RANDOM_DAMAGE ) ) {
        damage_string = string_format( "%d-%d", min_leveled_damage( caster ),
                                       static_cast<int>( type->max_damage.evaluate( d ) ) );
    } else {
        const int dmg = damage( caster );
        if( dmg >= 0 ) {
            damage_string = string_format( "%d", dmg );
        } else {
            damage_string = string_format( "+%d", std::abs( dmg ) );
        }
    }
    if( has_flag( spell_flag::PERCENTAGE_DAMAGE ) ) {
        damage_string = string_format( "%s%% %s", damage_string, _( "of current HP" ) );
    }
    return damage_string;
}

std::optional<tripoint> spell::select_target( Creature *source )
{
    tripoint target = source->pos();
    bool target_is_valid = false;
    if( range( *source ) > 0 && !is_valid_target( spell_target::none ) &&
        !has_flag( spell_flag::RANDOM_TARGET ) ) {
        if( source->is_avatar() ) {
            do {
                avatar &source_avatar = *source->as_avatar();
                std::vector<tripoint> trajectory = target_handler::mode_spell( source_avatar, *this,
                                                   true,
                                                   true );
                if( !trajectory.empty() ) {
                    target = trajectory.back();
                    target_is_valid = is_valid_target( source_avatar, target );
                    if( !( is_valid_target( spell_target::ground ) || source_avatar.sees( target ) ) ) {
                        target_is_valid = false;
                    }
                } else {
                    target_is_valid = false;
                }
                if( !target_is_valid ) {
                    if( query_yn( _( "Stop targeting?  Time spent will be lost." ) ) ) {
                        return std::nullopt;
                    }
                }
            } while( !target_is_valid );
        } else if( source->is_npc() ) {
            npc &source_npc = *source->as_npc();
            npc_attack_spell npc_spell( id() );
            // recalculate effectiveness because it's been a few turns since the npc started casting.
            const npc_attack_rating effectiveness = npc_spell.evaluate( source_npc,
                                                    source_npc.last_target.lock().get() );
            if( effectiveness < 0 ) {
                add_msg_debug( debugmode::debug_filter::DF_NPC, "%s cancels casting %s, target lost",
                               source_npc.disp_name(), name() );
                return std::nullopt;
            } else {
                target = effectiveness.target();
            }
        } // TODO: move monster spell attack targeting here
    } else if( has_flag( spell_flag::RANDOM_TARGET ) ) {
        const std::optional<tripoint> target_ = random_valid_target( *source, source->pos() );
        if( !target_ ) {
            source->add_msg_if_player( game_message_params{ m_bad, gmf_bypass_cooldown },
                                       _( "You can't find a suitable target." ) );
            return std::nullopt;
        }
        target = *target_;
    }
    return target;
}

int spell::min_leveled_aoe( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    return type->min_aoe.evaluate( d ) + std::round( get_effective_level() *
            type->aoe_increment.evaluate( d ) );
}

int spell::aoe( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    const int leveled_aoe = min_leveled_aoe( caster );
    int return_value;

    if( has_flag( spell_flag::RANDOM_AOE ) ) {
        return_value = rng( std::min( leveled_aoe, static_cast<int>( type->max_aoe.evaluate( d ) ) ),
                            std::max( leveled_aoe, static_cast<int>( type->max_aoe.evaluate( d ) ) ) );
    } else {
        if( type->max_aoe.evaluate( d ) >= type->min_aoe.evaluate( d ) ) {
            return_value = std::min( leveled_aoe, static_cast<int>( type->max_aoe.evaluate( d ) ) );
        } else {
            return_value = std::max( leveled_aoe, static_cast<int>( type->max_aoe.evaluate( d ) ) );
        }
    }
    return return_value * temp_aoe_multiplyer;
}

std::set<tripoint> spell::effect_area( const spell_effect::override_parameters &params,
                                       const tripoint &source, const tripoint &target ) const
{
    return type->spell_area_function( params, source, target );
}

std::set<tripoint> spell::effect_area( const tripoint &source, const tripoint &target,
                                       const Creature &caster ) const
{
    return effect_area( spell_effect::override_parameters( *this, caster ), source, target );
}

bool spell::in_aoe( const tripoint &source, const tripoint &target, const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    if( has_flag( spell_flag::RANDOM_AOE ) ) {
        return rl_dist( source, target ) <= type->max_aoe.evaluate( d );
    } else {
        return rl_dist( source, target ) <= aoe( caster );
    }
}

std::string spell::aoe_string( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    if( has_flag( spell_flag::RANDOM_AOE ) ) {
        return string_format( "%d-%d", min_leveled_aoe( caster ), type->max_aoe.evaluate( d ) );
    } else {
        return string_format( "%d", aoe( caster ) );
    }
}

int spell::range( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    const int leveled_range = type->min_range.evaluate( d ) + std::round( get_effective_level() *
                              type->range_increment.evaluate( d ) );
    float range;
    if( type->max_range.evaluate( d ) >= type->min_range.evaluate( d ) ) {
        range = std::min( leveled_range, static_cast<int>( type->max_range.evaluate( d ) ) );
    } else {
        range = std::max( leveled_range, static_cast<int>( type->max_range.evaluate( d ) ) );
    }
    return std::max( range * temp_range_multiplyer, 0.0f );
}

std::vector<tripoint> spell::targetable_locations( const Character &source ) const
{

    const tripoint char_pos = source.pos();
    const bool select_ground = is_valid_target( spell_target::ground );
    const bool ignore_walls = has_flag( spell_flag::NO_PROJECTILE );
    map &here = get_map();

    // TODO: put this in a namespace for reuse
    const auto has_obstruction = [&]( const tripoint & at ) {
        for( const tripoint &line_point : line_to( char_pos, at ) ) {
            if( here.impassable( line_point ) ) {
                return true;
            }
        }
        return false;
    };

    std::vector<tripoint> selectable_targets;
    for( const tripoint &query : here.points_in_radius( char_pos, range( source ) ) ) {
        if( !ignore_walls && has_obstruction( query ) ) {
            // it's blocked somewhere!
            continue;
        }

        if( !select_ground ) {
            if( !source.sees( query ) ) {
                // can't target a critter you can't see
                continue;
            }
        }

        if( is_valid_target( source, query ) ) {
            selectable_targets.push_back( query );
        }
    }
    return selectable_targets;
}

int spell::min_leveled_duration( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    return type->min_duration.evaluate( d ) + std::round( get_effective_level() *
            type->duration_increment.evaluate( d ) );
}

int spell::duration( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    const int leveled_duration = min_leveled_duration( caster );
    float duration;

    if( has_flag( spell_flag::RANDOM_DURATION ) ) {
        return rng( std::min( leveled_duration, static_cast<int>( type->max_duration.evaluate( d ) ) ),
                    std::max( leveled_duration,
                              static_cast<int>( type->max_duration.evaluate( d ) ) ) );
    } else {
        if( type->max_duration.evaluate( d ) >= type->min_duration.evaluate( d ) ) {
            return std::min( leveled_duration, static_cast<int>( type->max_duration.evaluate( d ) ) );
        } else {
            return std::max( leveled_duration, static_cast<int>( type->max_duration.evaluate( d ) ) );
        }
    }
    return std::max( duration * temp_duration_multiplyer, 0.0f );
}

std::string spell::duration_string( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    if( has_flag( spell_flag::RANDOM_DURATION ) ) {
        return string_format( "%s - %s", moves_to_string( min_leveled_duration( caster ) ),
                              moves_to_string( type->max_duration.evaluate( d ) ) );
    } else if( ( has_flag( spell_flag::PERMANENT ) && ( is_max_level( caster ) ||
                 effect() == "summon" ) ) ||
               has_flag( spell_flag::PERMANENT_ALL_LEVELS ) ) {
        return _( "Permanent" );
    } else {
        return moves_to_string( duration( caster ) );
    }
}

time_duration spell::duration_turns( const Creature &caster ) const
{
    return time_duration::from_moves( duration( caster ) );
}

void spell::gain_level( const Character &guy )
{
    gain_exp( guy, exp_to_next_level() );
}

void spell::gain_levels( const Character &guy, int gains )
{
    if( gains < 1 ) {
        return;
    }
    for( int gained = 0; gained < gains && !is_max_level( guy ); gained++ ) {
        gain_level( guy );
    }
}

void spell::set_level( const Character &guy, int nlevel )
{
    experience = 0;
    gain_levels( guy, nlevel );
}

bool spell::is_max_level( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    return get_level() >= type->max_level.evaluate( d );
}

bool spell::can_learn( const Character &guy ) const
{
    if( type->spell_class == trait_NONE ) {
        return true;
    }
    return guy.has_trait( type->spell_class );
}

int spell::energy_cost( const Character &guy ) const
{
    int cost;
    dialogue d( get_talker_for( guy ), nullptr );
    if( type->base_energy_cost.evaluate( d ) < type->final_energy_cost.evaluate( d ) ) {
        cost = std::min( static_cast<int>( type->final_energy_cost.evaluate( d ) ),
                         static_cast<int>( std::round( type->base_energy_cost.evaluate( d ) +
                                           type->energy_increment.evaluate( d ) * get_effective_level() ) ) );
    } else if( type->base_energy_cost.evaluate( d ) > type->final_energy_cost.evaluate( d ) ) {
        cost = std::max( static_cast<int>( type->final_energy_cost.evaluate( d ) ),
                         static_cast<int>( std::round( type->base_energy_cost.evaluate( d ) +
                                           type->energy_increment.evaluate( d ) * get_effective_level() ) ) );
    } else {
        cost = type->base_energy_cost.evaluate( d );
    }
    if( !no_hands() && !guy.has_flag( json_flag_SUBTLE_SPELL ) ) {
        // the first 10 points of combined encumbrance is ignored, but quickly adds up
        const int hands_encumb = std::max( 0,
                                           guy.avg_encumb_of_limb_type( body_part_type::type::hand ) - 5 );
        switch( type->energy_source ) {
            default:
                cost += 10 * hands_encumb * temp_somatic_difficulty_multiplyer;
                break;
            case magic_energy_type::hp:
                cost += hands_encumb * temp_somatic_difficulty_multiplyer;
                break;
            case magic_energy_type::stamina:
                cost += 100 * hands_encumb * temp_somatic_difficulty_multiplyer;
                break;
        }
    }
    return std::max( cost * temp_spell_cost_multiplyer, 0.0f );
}

bool spell::has_flag( const spell_flag &flag ) const
{
    return type->spell_tags[flag];
}

bool spell::has_flag( const std::string &flag ) const
{
    return type->flags.count( flag ) > 0;
}
bool spell::no_hands() const
{
    return ( has_flag( spell_flag::NO_HANDS ) || temp_somatic_difficulty_multiplyer <= 0 );
}

bool spell::is_spell_class( const trait_id &mid ) const
{
    return mid == type->spell_class;
}

bool spell::can_cast( const Character &guy ) const
{
    if( guy.has_flag( json_flag_NO_SPELLCASTING ) && !has_flag( spell_flag::PSIONIC ) ) {
        return false;
    }

    if( guy.has_flag( json_flag_NO_PSIONICS ) && has_flag( spell_flag::PSIONIC ) ) {
        return false;
    }

    if( !type->spell_components.is_empty() &&
        !type->spell_components->can_make_with_inventory( guy.crafting_inventory( guy.pos(), 0, false ),
                return_true<item> ) ) {
        return false;
    }

    return guy.magic->has_enough_energy( guy, *this );
}

void spell::use_components( Character &guy ) const
{
    if( type->spell_components.is_empty() ) {
        return;
    }
    const requirement_data &spell_components = type->spell_components.obj();
    // if we're here, we're assuming the Character has the correct components (using can_cast())
    inventory map_inv;
    map_inv.form_from_map( guy.pos(), 0, &guy, true, false );
    for( const std::vector<item_comp> &comp_vec : spell_components.get_components() ) {
        guy.consume_items( guy.select_item_component( comp_vec, 1, map_inv ), 1 );
    }
    for( const std::vector<tool_comp> &tool_vec : spell_components.get_tools() ) {
        guy.consume_tools( guy.select_tool_component( tool_vec, 1, map_inv ), 1 );
    }
}

bool spell::check_if_component_in_hand( Character &guy ) const
{
    if( type->spell_components.is_empty() ) {
        return false;
    }

    const requirement_data &spell_components = type->spell_components.obj();

    if( guy.has_weapon() ) {
        if( spell_components.can_make_with_inventory( *guy.get_wielded_item(), return_true<item> ) ) {
            return true;
        }
    }

    // if it isn't in hand, return false
    return false;
}

int spell::get_difficulty( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    return type->difficulty.evaluate( d ) + temp_difficulty_adjustment;
}

mod_id spell::get_src() const
{
    return type->src_mod;
}

int spell::casting_time( const Character &guy, bool ignore_encumb ) const
{
    // casting time in moves
    int casting_time = 0;
    dialogue d( get_talker_for( guy ), nullptr );
    if( type->base_casting_time.evaluate( d ) < type->final_casting_time.evaluate( d ) ) {
        casting_time = std::min( static_cast<int>( type->final_casting_time.evaluate( d ) ),
                                 static_cast<int>( std::round( type->base_casting_time.evaluate( d ) +
                                         type->casting_time_increment.evaluate( d ) *
                                         get_effective_level() ) ) );
    } else if( type->base_casting_time.evaluate( d ) > type->final_casting_time.evaluate( d ) ) {
        casting_time = std::max( static_cast<int>( type->final_casting_time.evaluate( d ) ),
                                 static_cast<int>( std::round( type->base_casting_time.evaluate( d ) +
                                         type->casting_time_increment.evaluate( d ) *
                                         get_effective_level() ) ) );
    } else {
        casting_time = type->base_casting_time.evaluate( d );
    }

    casting_time = guy.enchantment_cache->modify_value( enchant_vals::mod::CASTING_TIME_MULTIPLIER,
                   casting_time );

    if( !ignore_encumb && temp_somatic_difficulty_multiplyer > 0 ) {
        if( !has_flag( spell_flag::NO_LEGS ) ) {
            // the first 20 points of encumbrance combined is ignored
            const int legs_encumb = std::max( 0,
                                              guy.avg_encumb_of_limb_type( body_part_type::type::leg ) - 10 );
            casting_time += legs_encumb * 3 * temp_somatic_difficulty_multiplyer;
        }
        if( has_flag( spell_flag::SOMATIC ) && !guy.has_flag( json_flag_SUBTLE_SPELL ) ) {
            // the first 20 points of encumbrance combined is ignored
            const int arms_encumb = std::max( 0,
                                              guy.avg_encumb_of_limb_type( body_part_type::type::arm ) - 10 );
            casting_time += arms_encumb * 2 * temp_somatic_difficulty_multiplyer;
        }
    }
    return std::max( casting_time * temp_cast_time_multiplyer, 0.0f );
}

const requirement_data &spell::components() const
{
    return type->spell_components.obj();
}

bool spell::has_components() const
{
    return !type->spell_components.is_empty();
}

std::string spell::name() const
{
    return type->name.translated();
}

std::string spell::message() const
{
    if( !alt_message.empty() ) {
        return alt_message.translated();
    }
    return type->message.translated();
}

float spell::spell_fail( const Character &guy ) const
{
    if( has_flag( spell_flag::NO_FAIL ) ) {
        return 0.0f;
    }
    const bool is_psi = has_flag( spell_flag::PSIONIC );

    // formula is based on the following:
    // exponential curve
    // effective skill of 0 or less is 100% failure
    // effective skill of 8 (8 int, 0 spellcraft, 0 spell level, spell difficulty 0) is ~50% failure
    // effective skill of 30 is 0% failure
    const float effective_skill = 2 * ( get_effective_level() - get_difficulty(
                                            guy ) ) + guy.get_int() +
                                  guy.get_skill_level( skill() );

    // skill for psi powers downplays power level and is much more based on level and intelligence
    // and goes up to 40 max--effective skill of 10 is 50% failure, effective skill of 40 is 0%
    // Int 8, Metaphysics 2, level 1, difficulty 1 is effective level 26.5
    // Int 10, Metaphysics 5, level 4, difficulty 5 is effective level 27
    // Int 12, Metaphysics 8, level 7, difficulty 10 is effective level 33.5
    const float two_thirds_power_level = static_cast<float>( get_effective_level() ) /
                                         static_cast<float>
                                         ( 1.5 );

    const float psi_effective_skill = 2 * ( ( guy.get_skill_level( skill() ) * 2 ) - get_difficulty(
            guy ) ) + ( guy.get_int() * 1.5 ) + two_thirds_power_level;
    // add an if statement in here because sufficiently large numbers will definitely overflow because of exponents
    if( ( effective_skill > 30.0f && !is_psi ) || ( psi_effective_skill > 40.0f && is_psi ) ) {
        return 0.0f;
    } else if( ( effective_skill < 0.0f && !is_psi ) || ( psi_effective_skill < 0.0f && is_psi ) ) {
        return 1.0f;
    }

    float fail_chance = std::pow( ( effective_skill - 30.0f ) / 30.0f, 2 );
    float psi_fail_chance = std::pow( ( psi_effective_skill - 40.0f ) / 40.0f, 2 );

    if( !is_psi ) {
        if( has_flag( spell_flag::SOMATIC ) &&
            !guy.has_flag( json_flag_SUBTLE_SPELL ) && temp_somatic_difficulty_multiplyer > 0 ) {
            // the first 20 points of encumbrance combined is ignored
            const int arms_encumb = std::max( 0,
                                              guy.avg_encumb_of_limb_type( body_part_type::type::arm ) - 10 );
            // each encumbrance point beyond the "gray" color counts as half an additional fail %
            fail_chance += ( arms_encumb / 200.0f ) * temp_somatic_difficulty_multiplyer;
        }
        if( has_flag( spell_flag::VERBAL ) &&
            !guy.has_flag( json_flag_SILENT_SPELL ) && temp_sound_multiplyer > 0 ) {
            // a little bit of mouth encumbrance is allowed, but not much
            const int mouth_encumb = std::max( 0,
                                               guy.avg_encumb_of_limb_type( body_part_type::type::mouth ) - 5 );
            fail_chance += ( mouth_encumb / 100.0f ) * temp_sound_multiplyer;
        }
    }

    // concentration spells work better than you'd expect with a higher focus pool
    if( has_flag( spell_flag::CONCENTRATE ) && temp_concentration_difficulty_multiplyer > 0 ) {
        if( guy.get_focus() <= 0 ) {
            return 0.0f;
        }
        float concentration_loss = ( 1.0f - ( guy.get_focus() / 100.0f ) ) *
                                   temp_concentration_difficulty_multiplyer;
        fail_chance /= 1.0f - concentration_loss;
        psi_fail_chance /= 1.0f - concentration_loss;
    }

    return clamp( is_psi ? psi_fail_chance : fail_chance, 0.0f, 1.0f );
}

std::string spell::colorized_fail_percent( const Character &guy ) const
{
    const float fail_fl = spell_fail( guy ) * 100.0f;
    std::string fail_str;
    fail_fl == 100.0f ? fail_str = _( "Too Difficult!" ) : fail_str = string_format( "%.1f %% %s",
                                   fail_fl, _( "Failure Chance" ) );
    nc_color color;
    if( fail_fl > 90.0f ) {
        color = c_magenta;
    } else if( fail_fl > 75.0f ) {
        color = c_red;
    } else if( fail_fl > 60.0f ) {
        color = c_light_red;
    } else if( fail_fl > 35.0f ) {
        color = c_yellow;
    } else if( fail_fl > 15.0f ) {
        color = c_green;
    } else {
        color = c_light_green;
    }
    return colorize( fail_str, color );
}

spell_shape spell::shape() const
{
    return type->spell_area;
}

int spell::xp() const
{
    return experience;
}

void spell::gain_exp( const Character &guy, int nxp )
{
    int oldLevel = get_level();
    experience += nxp;
    if( guy.is_avatar() && oldLevel != get_level() ) {
        get_event_bus().send<event_type::player_levels_spell>( guy.getID(), id(), get_level(),
                spell_class() );
    }
}

void spell::set_exp( int nxp )
{
    experience = nxp;
}

std::string spell::energy_string() const
{
    switch( type->energy_source ) {
        case magic_energy_type::hp:
            return _( "health" );
        case magic_energy_type::mana:
            return _( "mana" );
        case magic_energy_type::stamina:
            return _( "stamina" );
        case magic_energy_type::bionic:
            return _( "kJ" );
        default:
            return "";
    }
}

std::string spell::energy_cost_string( const Character &guy ) const
{
    if( energy_source() == magic_energy_type::none ) {
        return _( "none" );
    }
    if( energy_source() == magic_energy_type::bionic || energy_source() == magic_energy_type::mana ) {
        return colorize( std::to_string( energy_cost( guy ) ), c_light_blue );
    }
    if( energy_source() == magic_energy_type::hp ) {
        auto pair = get_hp_bar( energy_cost( guy ), guy.get_hp_max() / 6 );
        return colorize( pair.first, pair.second );
    }
    if( energy_source() == magic_energy_type::stamina ) {
        auto pair = get_hp_bar( energy_cost( guy ), guy.get_stamina_max() );
        return colorize( pair.first, pair.second );
    }
    debugmsg( "ERROR: Spell %s has invalid energy source.", id().c_str() );
    return _( "error: energy_type" );
}

std::string spell::energy_cur_string( const Character &guy ) const
{
    if( energy_source() == magic_energy_type::none ) {
        return _( "infinite" );
    }
    if( energy_source() == magic_energy_type::bionic ) {
        return colorize( std::to_string( units::to_kilojoule( guy.get_power_level() ) ), c_light_blue );
    }
    if( energy_source() == magic_energy_type::mana ) {
        return colorize( std::to_string( guy.magic->available_mana() ), c_light_blue );
    }
    if( energy_source() == magic_energy_type::stamina ) {
        auto pair = get_hp_bar( guy.get_stamina(), guy.get_stamina_max() );
        return colorize( pair.first, pair.second );
    }
    if( energy_source() == magic_energy_type::hp ) {
        return "";
    }
    debugmsg( "ERROR: Spell %s has invalid energy source.", id().c_str() );
    return _( "error: energy_type" );
}

bool spell::is_valid() const
{
    return type.is_valid();
}

bool spell::bp_is_affected( const bodypart_str_id &bp ) const
{
    return type->affected_bps.test( bp );
}

void spell::create_field( const tripoint &at, Creature &caster ) const
{
    if( !type->field ) {
        return;
    }
    dialogue d( get_talker_for( caster ), nullptr );
    const int intensity = field_intensity( caster ) + rng( -type->field_intensity_variance.evaluate(
                              d ) * field_intensity( caster ),
                          type->field_intensity_variance.evaluate( d ) * field_intensity( caster ) );
    if( intensity <= 0 ) {
        return;
    }
    if( one_in( type->field_chance.evaluate( d ) ) ) {
        map &here = get_map();
        field_entry *field = here.get_field( at, *type->field );
        if( field ) {
            field->set_field_intensity( field->get_field_intensity() + intensity );
        } else {
            here.add_field( at, *type->field, intensity, -duration_turns( caster ) );
        }
    }
}

int spell::sound_volume( const Creature &caster ) const
{
    int loudness = 0;
    if( !has_flag( spell_flag::SILENT ) ) {
        loudness = std::abs( damage( caster ) ) / 3;
        if( has_flag( spell_flag::LOUD ) ) {
            loudness += 1 + damage( caster ) / 3;
        }
    }
    return std::max( loudness * temp_sound_multiplyer, 0.0f );
}

void spell::make_sound( const tripoint &target, Creature &caster ) const
{
    const int loudness = sound_volume( caster );
    if( loudness > 0 ) {
        make_sound( target, loudness );
    }
}

void spell::make_sound( const tripoint &target, int loudness ) const
{
    sounds::sound( target, loudness, type->sound_type, type->sound_description.translated(),
                   type->sound_ambient, type->sound_id, type->sound_variant );
}

std::string spell::effect() const
{
    return type->effect_name;
}

magic_energy_type spell::energy_source() const
{
    return type->energy_source;
}

bool spell::is_target_in_range( const Creature &caster, const tripoint &p ) const
{
    return rl_dist( caster.pos(), p ) <= range( caster );
}

bool spell::is_valid_target( spell_target t ) const
{
    return type->valid_targets[t];
}

bool spell::is_valid_target( const Creature &caster, const tripoint &p ) const
{
    bool valid = false;
    if( Creature *const cr = get_creature_tracker().creature_at<Creature>( p ) ) {
        Creature::Attitude cr_att = cr->attitude_to( caster );
        valid = valid || ( cr_att != Creature::Attitude::FRIENDLY &&
                           is_valid_target( spell_target::hostile ) );
        valid = valid || ( cr_att == Creature::Attitude::FRIENDLY &&
                           is_valid_target( spell_target::ally ) &&
                           p != caster.pos() );
        valid = valid || ( is_valid_target( spell_target::self ) && p == caster.pos() );
        valid = valid && target_by_monster_id( p );
        valid = valid && target_by_species_id( p );
        valid = valid && ignore_by_species_id( p );
    } else {
        valid = is_valid_target( spell_target::ground );
    }
    return valid;
}

bool spell::target_by_monster_id( const tripoint &p ) const
{
    if( type->targeted_monster_ids.empty() ) {
        return true;
    }
    bool valid = false;
    if( monster *const target = get_creature_tracker().creature_at<monster>( p ) ) {
        if( type->targeted_monster_ids.find( target->type->id ) != type->targeted_monster_ids.end() ) {
            valid = true;
        }
    }
    return valid;
}

bool spell::target_by_species_id( const tripoint &p ) const
{
    if( type->targeted_species_ids.empty() ) {
        return true;
    }
    bool valid = false;
    if( monster *const target = get_creature_tracker().creature_at<monster>( p ) ) {
        for( const species_id &spid : type->targeted_species_ids ) {
            if( target->type->in_species( spid ) ) {
                valid = true;
            }
        }
    }
    return valid;
}

bool spell::ignore_by_species_id( const tripoint &p ) const
{
    if( type->ignored_species_ids.empty() ) {
        return true;
    }
    bool valid = true;
    if( monster *const target = get_creature_tracker().creature_at<monster>( p ) ) {
        for( const species_id &spid : type->ignored_species_ids ) {
            if( target->type->in_species( spid ) ) {
                valid = false;
            }
        }
    }
    return valid;
}




std::string spell::description() const
{
    return type->description.translated();
}

nc_color spell::damage_type_color() const
{
    if( dmg_type().is_null() ) {
        return c_black;
    }
    return dmg_type()->magic_color;
}

std::string spell::damage_type_string() const
{
    if( dmg_type().is_null() ) {
        return std::string();
    }
    return dmg_type()->name.translated();
}

// constants defined below are just for the formula to be used,
// in order for the inverse formula to be equivalent
static constexpr double a = 6200.0;
static constexpr double b = 0.146661;
static constexpr double c = -62.5;

int spell::get_level() const
{
    // you aren't at the next level unless you have the requisite xp, so floor
    return std::max( static_cast<int>( std::floor( std::log( experience + a ) / b + c ) ), 0 );
}

int spell::get_effective_level() const
{
    return get_level() + temp_level_adjustment;
}

int spell::get_max_level( const Creature &caster ) const
{
    dialogue d( get_talker_for( caster ), nullptr );
    return type->max_level.evaluate( d );
}

int spell::get_temp_level_adjustment() const
{
    return temp_level_adjustment;
}

void spell::set_temp_level_adjustment( int adjustment )
{
    temp_level_adjustment = adjustment;
}


void spell::set_temp_adjustment( const std::string &target_property, float adjustment )
{
    if( target_property == "caster_level" ) {
        temp_level_adjustment += adjustment;
    } else if( target_property == "casting_time" ) {
        temp_cast_time_multiplyer += adjustment;
    } else if( target_property == "cost" ) {
        temp_spell_cost_multiplyer += adjustment;
    } else if( target_property == "aoe" ) {
        temp_aoe_multiplyer += adjustment;
    } else if( target_property == "range" ) {
        temp_range_multiplyer += adjustment;
    } else if( target_property == "duration" ) {
        temp_duration_multiplyer += adjustment;
    } else if( target_property == "difficulty" ) {
        temp_difficulty_adjustment += adjustment;
    } else if( target_property == "somatic_difficulty" ) {
        temp_somatic_difficulty_multiplyer += adjustment;
    } else if( target_property == "sound" ) {
        temp_sound_multiplyer += adjustment;
    } else if( target_property == "concentration" ) {
        temp_concentration_difficulty_multiplyer += adjustment;
    } else {
        debugmsg( "ERROR: invalid spellcasting adjustment name: %s", target_property );
    }
}
void spell::clear_temp_adjustments()
{
    temp_level_adjustment = 0;
    temp_cast_time_multiplyer = 1;
    temp_spell_cost_multiplyer = 1;
    temp_aoe_multiplyer = 1;
    temp_range_multiplyer = 1;
    temp_duration_multiplyer = 1;
    temp_difficulty_adjustment = 0;
    temp_somatic_difficulty_multiplyer = 1;
    temp_sound_multiplyer = 1;
    temp_concentration_difficulty_multiplyer = 1;
}

// helper function to calculate xp needed to be at a certain level
// pulled out as a helper function to make it easier to either be used in the future
// or easier to tweak the formula
int spell::exp_for_level( int level )
{
    // level 0 never needs xp
    if( level == 0 ) {
        return 0;
    }
    return std::ceil( std::exp( ( level - c ) * b ) ) - a;
}

int spell::exp_to_next_level() const
{
    return exp_for_level( get_level() + 1 ) - xp();
}

std::string spell::exp_progress() const
{
    const int level = get_level();
    const int this_level_xp = exp_for_level( level );
    const int next_level_xp = exp_for_level( level + 1 );
    const int denominator = next_level_xp - this_level_xp;
    const float progress = static_cast<float>( xp() - this_level_xp ) / std::max( 1.0f,
                           static_cast<float>( denominator ) );
    return string_format( "%i%%", clamp( static_cast<int>( std::round( progress * 100 ) ), 0, 99 ) );
}

float spell::exp_modifier( const Character &guy ) const
{
    const float int_modifier = ( guy.get_int() - 8.0f ) / 8.0f;
    const float difficulty_modifier = get_difficulty( guy ) / 20.0f;
    const float spellcraft_modifier = guy.get_skill_level( skill() ) / 10.0f;

    return ( int_modifier + difficulty_modifier + spellcraft_modifier ) / 5.0f + 1.0f;
}

int spell::casting_exp( const Character &guy ) const
{
    // the amount of xp you would get with no modifiers
    const int base_casting_xp = 75;

    return std::round( guy.adjust_for_focus( base_casting_xp * exp_modifier( guy ) ) );
}

std::string spell::enumerate_targets() const
{
    std::vector<std::string> all_valid_targets;
    int last_target = static_cast<int>( spell_target::num_spell_targets );
    for( int i = 0; i < last_target; ++i ) {
        spell_target t = static_cast<spell_target>( i );
        if( is_valid_target( t ) && t != spell_target::none ) {
            all_valid_targets.emplace_back( target_to_string( t ) );
        }
    }
    if( all_valid_targets.size() == 1 ) {
        return all_valid_targets[0];
    }
    std::string ret;
    // TODO: if only we had a function to enumerate strings and concatenate them...
    for( auto iter = all_valid_targets.begin(); iter != all_valid_targets.end(); iter++ ) {
        if( iter + 1 == all_valid_targets.end() ) {
            ret = string_format( _( "%s and %s" ), ret, *iter );
        } else if( iter == all_valid_targets.begin() ) {
            ret = *iter;
        } else {
            ret = string_format( _( "%s, %s" ), ret, *iter );
        }
    }
    return ret;
}

std::string spell::list_targeted_monster_names() const
{
    if( type->targeted_monster_ids.empty() ) {
        return "";
    }
    std::vector<std::string> all_valid_monster_names;
    for( const mtype_id &mon_id : type->targeted_monster_ids ) {
        all_valid_monster_names.emplace_back( mon_id->nname() );
    }
    //remove repeat names
    all_valid_monster_names.erase( std::unique( all_valid_monster_names.begin(),
                                   all_valid_monster_names.end() ), all_valid_monster_names.end() );
    std::string ret = enumerate_as_string( all_valid_monster_names );
    return ret;
}

std::string spell::list_targeted_species_names() const
{
    if( type->targeted_species_ids.empty() ) {
        return "";
    }
    std::vector<std::string> all_valid_species_names;
    for( const species_id &specie_id : type->targeted_species_ids ) {
        all_valid_species_names.emplace_back( specie_id.str() );
    }
    //remove repeat names
    all_valid_species_names.erase( std::unique( all_valid_species_names.begin(),
                                   all_valid_species_names.end() ), all_valid_species_names.end() );
    std::string ret = enumerate_as_string( all_valid_species_names );
    return ret;
}

std::string spell::list_ignored_species_names() const
{
    if( type->ignored_species_ids.empty() ) {
        return "";
    }
    std::vector<std::string> all_valid_species_names;
    for( const species_id &species_id : type->ignored_species_ids ) {
        all_valid_species_names.emplace_back( species_id.str() );
    }
    //remove repeat names
    all_valid_species_names.erase( std::unique( all_valid_species_names.begin(),
                                   all_valid_species_names.end() ), all_valid_species_names.end() );
    std::string ret = enumerate_as_string( all_valid_species_names );
    return ret;
}

const damage_type_id &spell::dmg_type() const
{
    return type->dmg_type;
}

damage_instance spell::get_damage_instance( Creature &caster ) const
{
    damage_instance dmg;
    dmg.add_damage( dmg_type(), damage( caster ) );
    return dmg;
}

dealt_damage_instance spell::get_dealt_damage_instance( Creature &caster ) const
{
    dealt_damage_instance dmg;
    dmg.set_damage( dmg_type(), damage( caster ) );
    return dmg;
}

dealt_projectile_attack spell::get_projectile_attack( const tripoint &target,
        Creature &hit_critter, Creature &caster ) const
{
    projectile bolt;
    bolt.speed = 10000;
    bolt.impact = get_damage_instance( caster );
    bolt.proj_effects.emplace( "MAGIC" );

    dealt_projectile_attack atk;
    atk.end_point = target;
    atk.hit_critter = &hit_critter;
    atk.proj = bolt;
    atk.missed_by = 0.0;

    return atk;
}

std::string spell::effect_data() const
{
    return type->effect_str;
}

vproto_id spell::summon_vehicle_id() const
{
    return vproto_id( type->effect_str );
}

int spell::heal( const tripoint &target, Creature &caster ) const
{
    creature_tracker &creatures = get_creature_tracker();
    monster *const mon = creatures.creature_at<monster>( target );
    if( mon ) {
        return mon->heal( -damage( caster ) );
    }
    Character *const p = creatures.creature_at<Character>( target );
    if( p ) {
        p->healall( -damage( caster ) );
        return -damage( caster );
    }
    return -1;
}

void spell::cast_spell_effect( Creature &source, const tripoint &target ) const
{
    Character *caster = source.as_character();
    if( caster ) {
        character_id c_id = caster->getID();
        // send casting to the event bus
        get_event_bus().send<event_type::character_casts_spell>( c_id, this->id(), this->spell_class(),
                this->get_difficulty( source ), this->energy_cost( *caster ), this->casting_time( *caster ),
                this->damage( source ) );
    }

    type->effect( *this, source, target );
}

void spell::cast_all_effects( Creature &source, const tripoint &target ) const
{
    if( has_flag( spell_flag::WONDER ) ) {
        const auto iter = type->additional_spells.begin();
        for( int num_spells = std::abs( damage( source ) ); num_spells > 0; num_spells-- ) {
            if( type->additional_spells.empty() ) {
                debugmsg( "ERROR: %s has WONDER flag but no spells to choose from!", type->id.c_str() );
                return;
            }
            const int rand_spell = rng( 0, type->additional_spells.size() - 1 );
            spell sp = ( iter + rand_spell )->get_spell( source, get_effective_level() );
            const bool _self = ( iter + rand_spell )->self;

            // This spell flag makes it so the message of the spell that's cast using this spell will be sent.
            // if a message is added to the casting spell, it will be sent as well.
            source.add_msg_if_player( sp.message() );

            if( sp.has_flag( spell_flag::RANDOM_TARGET ) ) {
                if( const std::optional<tripoint> new_target = sp.random_valid_target( source,
                        _self ? source.pos() : target ) ) {
                    sp.cast_all_effects( source, *new_target );
                }
            } else {
                if( _self ) {
                    sp.cast_all_effects( source, source.pos() );
                } else {
                    sp.cast_all_effects( source, target );
                }
            }
        }
    } else {
        if( has_flag( spell_flag::EXTRA_EFFECTS_FIRST ) ) {
            cast_extra_spell_effects( source, target );
            cast_spell_effect( source, target );
        } else {
            cast_spell_effect( source, target );
            cast_extra_spell_effects( source, target );
        }
    }
}

void spell::cast_extra_spell_effects( Creature &source, const tripoint &target ) const
{
    for( const fake_spell &extra_spell : type->additional_spells ) {
        spell sp = extra_spell.get_spell( source, get_effective_level() );
        if( sp.has_flag( spell_flag::RANDOM_TARGET ) ) {
            if( const std::optional<tripoint> new_target = sp.random_valid_target( source,
                    extra_spell.self ? source.pos() : target ) ) {
                sp.cast_all_effects( source, *new_target );
            }
        } else {
            if( extra_spell.self ) {
                sp.cast_all_effects( source, source.pos() );
            } else {
                sp.cast_all_effects( source, target );
            }
        }
    }
}

std::optional<tripoint> spell::random_valid_target( const Creature &caster,
        const tripoint &caster_pos ) const
{
    const bool ignore_ground = has_flag( spell_flag::RANDOM_CRITTER );
    std::set<tripoint> valid_area;
    spell_effect::override_parameters blast_params( *this, caster );
    // we want to pick a random target within range, not aoe
    blast_params.aoe_radius = range( caster );
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &target : spell_effect::spell_effect_blast(
             blast_params, caster_pos, caster_pos ) ) {
        if( target != caster_pos && is_valid_target( caster, target ) &&
            ( !ignore_ground || creatures.creature_at<Creature>( target ) ) ) {
            valid_area.emplace( target );
        }
    }
    if( valid_area.empty() ) {
        return std::nullopt;
    }
    return random_entry( valid_area );
}

// player

known_magic::known_magic()
{
    mana_base = 1000;
    mana = mana_base;
}

void known_magic::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "mana", mana );

    json.member( "spellbook" );
    json.start_array();
    for( const auto &pair : spellbook ) {
        json.start_object();
        json.member( "id", pair.second.id() );
        json.member( "xp", pair.second.xp() );
        json.end_object();
    }
    json.end_array();
    json.member( "invlets", invlets );
    json.member( "favorites", favorites );

    json.end_object();
}

void known_magic::deserialize( const JsonObject &data )
{
    data.read( "mana", mana );

    for( JsonObject jo : data.get_array( "spellbook" ) ) {
        std::string id = jo.get_string( "id" );
        spell_id sp = spell_id( id );
        int xp = jo.get_int( "xp" );
        if( knows_spell( sp ) ) {
            spellbook[sp].set_exp( xp );
        } else {
            spellbook.emplace( sp, spell( sp, xp ) );
        }
    }
    data.read( "invlets", invlets );
    data.read( "favorites", favorites );
}

bool known_magic::knows_spell( const std::string &sp ) const
{
    return knows_spell( spell_id( sp ) );
}

bool known_magic::knows_spell( const spell_id &sp ) const
{
    return spellbook.count( sp ) == 1;
}

bool known_magic::knows_spell() const
{
    return !spellbook.empty();
}

void known_magic::learn_spell( const std::string &sp, Character &guy, bool force )
{
    learn_spell( spell_id( sp ), guy, force );
}

void known_magic::learn_spell( const spell_id &sp, Character &guy, bool force )
{
    learn_spell( &sp.obj(), guy, force );
}

void known_magic::learn_spell( const spell_type *sp, Character &guy, bool force )
{
    if( !sp->is_valid() ) {
        debugmsg( "Tried to learn invalid spell" );
        return;
    }
    if( guy.magic->knows_spell( sp->id ) ) {
        // you already know the spell
        return;
    }
    spell temp_spell( sp->id );
    if( !temp_spell.is_valid() ) {
        debugmsg( "Tried to learn invalid spell" );
        return;
    }
    if( !force && sp->spell_class != trait_NONE ) {
        if( can_learn_spell( guy, sp->id ) && !guy.has_trait( sp->spell_class ) ) {
            std::string trait_cancel;
            for( const trait_id &cancel : sp->spell_class->cancels ) {
                if( cancel == sp->spell_class->cancels.back() &&
                    sp->spell_class->cancels.back() != sp->spell_class->cancels.front() ) {
                    trait_cancel = string_format( _( "%s and %s" ), trait_cancel, cancel->name() );
                } else if( cancel == sp->spell_class->cancels.front() ) {
                    trait_cancel = cancel->name();
                    if( sp->spell_class->cancels.size() == 1 ) {
                        trait_cancel = string_format( "%s: %s", trait_cancel, cancel->desc() );
                    }
                } else {
                    trait_cancel = string_format( _( "%s, %s" ), trait_cancel, cancel->name() );
                }
                if( cancel == sp->spell_class->cancels.back() ) {
                    trait_cancel += ".";
                }
            }
            if( !guy.is_avatar() || query_yn(
                    _( "Learning this spell will make you a\n\n%s: %s\n\nand lock you out of\n\n%s\n\nContinue?" ),
                    sp->spell_class->name(), sp->spell_class->desc(), trait_cancel ) ) {
                guy.set_mutation( sp->spell_class );
                guy.on_mutation_gain( sp->spell_class );
                guy.add_msg_if_player( sp->spell_class.obj().desc() );
            } else {
                return;
            }
        }
    }
    if( force || can_learn_spell( guy, sp->id ) ) {
        spellbook.emplace( sp->id, temp_spell );
        get_event_bus().send<event_type::character_learns_spell>( guy.getID(), sp->id );
        guy.add_msg_player_or_npc( m_good, _( "You learned %s!" ), _( "<npcname> learned %s!" ), sp->name );
    } else {
        guy.add_msg_player_or_npc( m_bad, _( "You can't learn this spell." ),
                                   _( "<npcname> can't learn this spell." ) );
    }
}

void known_magic::forget_spell( const std::string &sp )
{
    forget_spell( spell_id( sp ) );
}

void known_magic::forget_spell( const spell_id &sp )
{
    if( !knows_spell( sp ) ) {
        debugmsg( "Can't forget a spell you don't know!" );
        return;
    }
    add_msg( m_bad, _( "All knowledge of %s leaves you." ), sp->name );
    // TODO: add parameter for owner of known_magic for this function
    get_event_bus().send<event_type::character_forgets_spell>( get_player_character().getID(), sp->id );
    spellbook.erase( sp );
}

void known_magic::set_spell_level( const spell_id &sp, int new_level, const Character *guy )
{
    spell temp_spell( sp->id );
    if( !knows_spell( sp ) ) {
        if( new_level >= 0 ) {
            temp_spell.set_level( *guy, new_level );
            spellbook.emplace( sp->id, spell( temp_spell ) );
            get_event_bus().send<event_type::character_learns_spell>( guy->getID(), sp->id );
        }
    } else {
        if( new_level >= 0 ) {
            spell &temp_sp = get_spell( sp );
            temp_sp.set_level( *guy, new_level );
        } else {
            get_event_bus().send<event_type::character_forgets_spell>( guy->getID(), sp->id );
            spellbook.erase( sp );
        }
    }
}

void known_magic::set_spell_exp( const spell_id &sp, int new_exp, const Character *guy )
{
    spell temp_spell( sp->id );
    if( !knows_spell( sp ) ) {
        if( new_exp >= 0 ) {
            temp_spell.set_exp( new_exp );
            spellbook.emplace( sp->id, spell( temp_spell ) );
            get_event_bus().send<event_type::character_learns_spell>( guy->getID(), sp->id );
        }
    } else {
        if( new_exp >= 0 ) {
            spell &temp_sp = get_spell( sp );
            int old_level = temp_sp.get_level();
            temp_sp.set_exp( new_exp );
            if( guy->is_avatar() && old_level != temp_sp.get_level() ) {
                get_event_bus().send<event_type::player_levels_spell>( guy->getID(), sp->id, temp_sp.get_level(),
                        sp->spell_class );
            }
        } else {
            get_event_bus().send<event_type::character_forgets_spell>( guy->getID(), sp->id );
            spellbook.erase( sp );
        }
    }
}

bool known_magic::can_learn_spell( const Character &guy, const spell_id &sp ) const
{
    const spell_type &sp_t = sp.obj();
    if( sp_t.spell_class == trait_NONE ) {
        return true;
    }
    if( sp_t.spell_tags[spell_flag::MUST_HAVE_CLASS_TO_LEARN] ) {
        return guy.has_trait( sp_t.spell_class );
    } else {
        return !guy.has_opposite_trait( sp_t.spell_class );
    }
}

spell &known_magic::get_spell( const spell_id &sp )
{
    if( !knows_spell( sp ) ) {
        debugmsg( "ERROR: Tried to get unknown spell" );
    }
    spell &temp_spell = spellbook[ sp ];
    return temp_spell;
}

std::vector<spell *> known_magic::get_spells()
{
    std::vector<spell *> spells;
    spells.reserve( spellbook.size() );
    for( auto &spell_pair : spellbook ) {
        spells.emplace_back( &spell_pair.second );
    }
    return spells;
}

int known_magic::available_mana() const
{
    return mana;
}

void known_magic::set_mana( int new_mana )
{
    mana = new_mana;
}

void known_magic::mod_mana( const Character &guy, int add_mana )
{
    set_mana( clamp( mana + add_mana, 0, max_mana( guy ) ) );
}

int known_magic::max_mana( const Character &guy ) const
{
    const float int_bonus = ( ( 0.2f + guy.get_int() * 0.1f ) - 1.0f ) * mana_base;
    int penalty_calc = std::round( std::max<int64_t>( 0,
                                   units::to_kilojoule( guy.get_power_level() ) ) );

    const int bionic_penalty = guy.enchantment_cache->modify_value(
                                   enchant_vals::mod::BIONIC_MANA_PENALTY, penalty_calc );

    const float unaugmented_mana = std::max( 0.0f,
                                   ( mana_base + int_bonus ) - bionic_penalty );
    return guy.calculate_by_enchantment( unaugmented_mana, enchant_vals::mod::MAX_MANA, true );
}

void known_magic::update_mana( const Character &guy, float turns )
{
    // mana should replenish in 8 hours.
    const double full_replenish = to_turns<double>( 8_hours );
    const double ratio = turns / full_replenish;
    mod_mana( guy, std::floor( ratio * guy.calculate_by_enchantment( static_cast<double>( max_mana(
                                   guy ) ), enchant_vals::mod::REGEN_MANA ) ) );
}

std::vector<spell_id> known_magic::spells() const
{
    std::vector<spell_id> spell_ids;
    spell_ids.reserve( spellbook.size() );
    for( const std::pair<const spell_id, spell> &pair : spellbook ) {
        spell_ids.emplace_back( pair.first );
    }
    return spell_ids;
}

// does the Character have enough energy (of the type of the spell) to cast the spell?
bool known_magic::has_enough_energy( const Character &guy, const spell &sp ) const
{
    int cost = sp.energy_cost( guy );
    switch( sp.energy_source() ) {
        case magic_energy_type::mana:
            return available_mana() >= cost;
        case magic_energy_type::bionic:
            return guy.get_power_level() >= units::from_kilojoule( static_cast<std::int64_t>( cost ) );
        case magic_energy_type::stamina:
            return guy.get_stamina() >= cost;
        case magic_energy_type::hp:
            for( const std::pair<const bodypart_str_id, bodypart> &elem : guy.get_body() ) {
                if( elem.second.get_hp_cur() > cost ) {
                    return true;
                }
            }
            return false;
        case magic_energy_type::none:
            return true;
        default:
            return false;
    }
}

void known_magic::clear_opens_spellbook_data()
{
    caster_level_adjustment = 0;
    caster_level_adjustment_by_spell.clear();
    caster_level_adjustment_by_school.clear();
    for( spell *sp : get_spells() ) {
        sp->clear_temp_adjustments();
    }
}

void known_magic::evaluate_opens_spellbook_data()
{
    for( spell *sp : get_spells() ) {
        double raw_level_adjust = caster_level_adjustment;
        std::map<trait_id, double>::iterator school_it =
            caster_level_adjustment_by_school.find( sp->spell_class() );
        if( school_it != caster_level_adjustment_by_school.end() ) {
            raw_level_adjust += school_it->second;
        }
        std::map<spell_id, double>::iterator spell_it =
            caster_level_adjustment_by_spell.find( sp->id() );
        if( spell_it != caster_level_adjustment_by_spell.end() ) {
            raw_level_adjust += spell_it->second;
        }
        int final_level = clamp( sp->get_level() + static_cast<int>( raw_level_adjust ), 0,
                                 sp->get_max_level( get_player_character() ) );
        sp->set_temp_level_adjustment( final_level - sp->get_level() );
    }
}

int known_magic::time_to_learn_spell( const Character &guy, const std::string &str ) const
{
    return time_to_learn_spell( guy, spell_id( str ) );
}

int known_magic::time_to_learn_spell( const Character &guy, const spell_id &sp ) const
{
    dialogue d( get_talker_for( guy ), nullptr );
    const int base_time = to_moves<int>( 30_minutes );
    const double int_modifier = ( guy.get_int() - 8.0 ) / 8.0;
    const double skill_modifier = guy.get_skill_level( sp->skill ) / 10.0;
    return base_time * ( 1.0 + sp->difficulty.evaluate( d ) / ( 1.0 + int_modifier + skill_modifier ) );
}

int known_magic::get_spellname_max_width()
{
    int width = 0;
    for( const spell *sp : get_spells() ) {
        width = std::max( width, utf8_width( sp->name() ) );
    }
    return width;
}

std::vector<spell> Character::spells_known_of_class( const trait_id &spell_class ) const
{
    std::vector<spell> ret;

    if( !has_trait( spell_class ) ) {
        return ret;
    }

    for( const spell_id &sp : magic->spells() ) {
        if( sp->spell_class == spell_class ) {
            ret.push_back( magic->get_spell( sp ) );
        }
    }

    return ret;
}

static void reflesh_favorite( uilist *menu, std::vector<spell *> known_spells )
{
    for( uilist_entry &entry : menu->entries ) {
        if( get_player_character().magic->is_favorite( known_spells[entry.retval]->id() ) ) {
            entry.extratxt.left = 0;
            entry.extratxt.txt = _( "*" );
            entry.extratxt.color = c_white;
        } else {
            entry.extratxt.txt = "";
        }
    }
}

class spellcasting_callback : public uilist_callback
{
    private:
        int selected_sp = 0;
        int scroll_pos = 0;
        std::vector<std::string> info_txt;
        std::vector<spell *> known_spells;
        void spell_info_text( const spell &sp, int width );
        void draw_spell_info( const uilist *menu );
    public:
        // invlets reserved for special functions
        const std::set<int> reserved_invlets{ 'I', '=', '*' };
        bool casting_ignore;

        spellcasting_callback( std::vector<spell *> &spells,
                               bool casting_ignore ) : known_spells( spells ),
            casting_ignore( casting_ignore ) {}
        bool key( const input_context &ctxt, const input_event &event, int entnum,
                  uilist *menu ) override {
            const std::string &action = ctxt.input_to_action( event );
            if( action == "CAST_IGNORE" ) {
                casting_ignore = !casting_ignore;
                return true;
            } else if( action == "CHOOSE_INVLET" ) {
                int invlet = 0;
                invlet = popup_getkey( _( "Choose a new hotkey for this spell." ) );
                if( inv_chars.valid( invlet ) ) {
                    const bool invlet_set =
                        get_player_character().magic->set_invlet( known_spells[entnum]->id(), invlet, reserved_invlets );
                    if( !invlet_set ) {
                        popup( _( "Hotkey already used." ) );
                    } else {
                        popup( _( "%c set.  Close and reopen spell menu to refresh list with changes." ),
                               invlet );
                    }
                } else {
                    popup( _( "Hotkey removed." ) );
                    get_player_character().magic->rem_invlet( known_spells[entnum]->id() );
                }
                return true;
            } else if( action == "SCROLL_UP_SPELL_MENU" || action == "SCROLL_DOWN_SPELL_MENU" ) {
                scroll_pos += action == "SCROLL_DOWN_SPELL_MENU" ? 1 : -1;
            } else if( action == "SCROLL_FAVORITE" ) {
                get_player_character().magic->toggle_favorite( known_spells[entnum]->id() );
                reflesh_favorite( menu, known_spells );
            }
            return false;
        }

        void refresh( uilist *menu ) override {
            const std::string space( menu->pad_right - 2, ' ' );
            mvwputch( menu->window, point( menu->w_width - menu->pad_right, 0 ), c_magenta, LINE_OXXX );
            mvwputch( menu->window, point( menu->w_width - menu->pad_right, menu->w_height - 1 ), c_magenta,
                      LINE_XXOX );
            for( int i = 1; i < menu->w_height - 1; i++ ) {
                mvwputch( menu->window, point( menu->w_width - menu->pad_right, i ), c_magenta, LINE_XOXO );
                mvwputch( menu->window, point( menu->w_width - menu->pad_right + 1, i ), menu->text_color, space );
            }
            std::string ignore_string = casting_ignore ? _( "Ignore Distractions" ) :
                                        _( "Popup Distractions" );
            mvwprintz( menu->window, point( menu->w_width - menu->pad_right + 2, 0 ),
                       casting_ignore ? c_red : c_light_green, string_format( "%s %s", "[I]", ignore_string ) );
            const std::string assign_letter = _( "Assign Hotkey [=]" );
            mvwprintz( menu->window, point( menu->w_width - assign_letter.length() - 1, 0 ), c_yellow,
                       assign_letter );
            if( menu->selected >= 0 && static_cast<size_t>( menu->selected ) < known_spells.size() ) {
                if( info_txt.empty() || selected_sp != menu->selected ) {
                    info_txt.clear();
                    spell_info_text( *known_spells[menu->selected], menu->pad_right - 4 );
                    selected_sp = menu->selected;
                    scroll_pos = 0;
                }
                if( scroll_pos > static_cast<int>( info_txt.size() ) - ( menu->w_height - 2 ) ) {
                    scroll_pos = info_txt.size() - ( menu->w_height - 2 );
                }
                if( scroll_pos < 0 ) {
                    scroll_pos = 0;
                }
                scrollbar()
                .offset_x( menu->w_width - 1 )
                .offset_y( 1 )
                .content_size( info_txt.size() )
                .viewport_pos( scroll_pos )
                .viewport_size( menu->w_height - 2 )
                .apply( menu->window );
                draw_spell_info( menu );
            }
            wnoutrefresh( menu->window );
        }
};

bool spell::casting_time_encumbered( const Character &guy ) const
{
    int encumb = 0;
    if( !has_flag( spell_flag::NO_LEGS ) && temp_somatic_difficulty_multiplyer > 0 ) {
        // the first 20 points of encumbrance combined is ignored
        encumb += std::max( 0, guy.avg_encumb_of_limb_type( body_part_type::type::leg ) - 10 );
    }
    if( has_flag( spell_flag::SOMATIC ) && !guy.has_flag( json_flag_SUBTLE_SPELL ) &&
        temp_somatic_difficulty_multiplyer > 0 ) {
        // the first 20 points of encumbrance combined is ignored
        encumb += std::max( 0, guy.avg_encumb_of_limb_type( body_part_type::type::arm ) - 10 );
    }
    return encumb > 0;
}

bool spell::energy_cost_encumbered( const Character &guy ) const
{
    if( !no_hands() && !guy.has_flag( json_flag_SUBTLE_SPELL ) ) {
        return std::max( 0, guy.avg_encumb_of_limb_type( body_part_type::type:: hand ) - 5 ) >
               0;
    }
    return false;
}

// this prints various things about the spell out in a list
// including flags and things like "goes through walls"
std::string spell::enumerate_spell_data( const Character &guy ) const
{
    std::vector<std::string> spell_data;
    if( has_flag( spell_flag::PSIONIC ) ) {
        spell_data.emplace_back( _( "is a psionic power" ) );
    }
    if( has_flag( spell_flag::CONCENTRATE ) && !has_flag( spell_flag::PSIONIC ) &&
        temp_concentration_difficulty_multiplyer > 0 ) {
        spell_data.emplace_back( _( "requires concentration" ) );
    }
    if( has_flag( spell_flag::VERBAL ) && temp_sound_multiplyer > 0 ) {
        spell_data.emplace_back( _( "verbal" ) );
    }
    if( has_flag( spell_flag::SOMATIC ) && temp_somatic_difficulty_multiplyer > 0 ) {
        spell_data.emplace_back( _( "somatic" ) );
    }
    if( !no_hands() ) {
        spell_data.emplace_back( _( "impeded by gloves" ) );
    } else if( no_hands() && !has_flag( spell_flag::PSIONIC ) ) {
        spell_data.emplace_back( _( "does not require hands" ) );
    }
    if( !has_flag( spell_flag::NO_LEGS ) && temp_somatic_difficulty_multiplyer > 0 ) {
        spell_data.emplace_back( _( "requires mobility" ) );
    }
    if( effect() == "attack" && range( guy ) > 1 && has_flag( spell_flag::NO_PROJECTILE ) &&
        !has_flag( spell_flag::PSIONIC ) ) {
        spell_data.emplace_back( _( "can be cast through walls" ) );
    }
    if( effect() == "attack" && range( guy ) > 1 && has_flag( spell_flag::NO_PROJECTILE ) &&
        has_flag( spell_flag::PSIONIC ) ) {
        spell_data.emplace_back( _( "can be channeled through walls" ) );
    }
    return enumerate_as_string( spell_data );
}

void spellcasting_callback::spell_info_text( const spell &sp, int width )
{
    Character &pc = get_player_character();

    info_txt.emplace_back( colorize( sp.spell_class() == trait_NONE ? _( "Classless" ) :
                                     sp.spell_class()->name(), c_yellow ) );
    for( const std::string &line : foldstring( sp.description(), width ) ) {
        info_txt.emplace_back( colorize( line, c_light_gray ) );
    }
    info_txt.emplace_back( );
    for( const std::string &line : foldstring( sp.enumerate_spell_data( pc ), width ) ) {
        info_txt.emplace_back( colorize( line, c_light_gray ) );
    }
    info_txt.emplace_back( );

    auto columnize = [&width]( const std::string & col1, const std::string & col2 ) {
        std::string line = col1;
        int pad = clamp<int>( width / 2 - utf8_width( line, true ), 1, width / 2 );
        line.append( pad, ' ' );
        line.append( col2 );
        return line;
    };
    // Calculates temp_level_adjust from EoC, saves it to the spell for later use, and prepares to display the result
    int temp_level_adjust = sp.get_temp_level_adjustment();
    std::string temp_level_adjust_string;
    if( temp_level_adjust < 0 ) {
        temp_level_adjust_string = " (-" + std::to_string( -temp_level_adjust ) + ")";
    } else if( temp_level_adjust > 0 ) {
        temp_level_adjust_string = " (+" + std::to_string( temp_level_adjust ) + ")";
    }

    info_txt.emplace_back(
        colorize( columnize( string_format( "%s: %d%s%s", _( "Spell Level" ), sp.get_effective_level(),
                                            sp.is_max_level( pc ) ? _( " (MAX)" ) : "", temp_level_adjust_string.c_str() ),
                             string_format( "%s: %d", _( "Max Level" ), sp.get_max_level( pc ) ) ), c_light_gray ) );
    info_txt.emplace_back(
        colorize( columnize( sp.colorized_fail_percent( pc ),
                             string_format( "%s: %d", _( "Difficulty" ), sp.get_difficulty( pc ) ) ), c_light_gray ) );
    info_txt.emplace_back(
        colorize( columnize( string_format( "%s: %s", _( "Current Exp" ),
                                            colorize( std::to_string( sp.xp() ), c_light_green ) ),
                             string_format( "%s: %s", _( "to Next Level" ),
                                            colorize( std::to_string( sp.exp_to_next_level() ), c_light_green ) ) ), c_light_gray ) );

    info_txt.emplace_back( );

    const bool cost_encumb = sp.energy_cost_encumbered( pc );
    const bool is_psi = sp.has_flag( spell_flag::PSIONIC );
    if( is_psi ) {
        std::string cost_string = cost_encumb ? _( "Channeling Cost (impeded)" ) : _( "Channeling Cost" );
        std::string energy_cur = sp.energy_source() == magic_energy_type::hp ? "" :
                                 string_format( _( " (%s current)" ), sp.energy_cur_string( pc ) );
        if( !pc.magic->has_enough_energy( pc, sp ) ) {
            cost_string = colorize( _( "Not Enough Stamina" ), c_red );
            energy_cur.clear();
        }
        info_txt.emplace_back(
            colorize( string_format( "%s: %s %s%s", cost_string, sp.energy_cost_string( pc ),
                                     sp.energy_string(), energy_cur ), c_light_gray ) );
    } else {
        std::string cost_string = cost_encumb ? _( "Casting Cost (impeded)" ) : _( "Casting Cost" );
        std::string energy_cur = sp.energy_source() == magic_energy_type::hp ? "" :
                                 string_format( _( " (%s current)" ), sp.energy_cur_string( pc ) );
        if( !pc.magic->has_enough_energy( pc, sp ) ) {
            cost_string = colorize( _( "Not Enough Energy" ), c_red );
            energy_cur.clear();
        }
        info_txt.emplace_back(
            colorize( string_format( "%s: %s %s%s", cost_string, sp.energy_cost_string( pc ),
                                     sp.energy_string(), energy_cur ), c_light_gray ) );
    };
    const bool c_t_encumb = sp.casting_time_encumbered( pc );
    if( is_psi ) {
        info_txt.emplace_back(
            colorize( string_format( "%s: %s",
                                     c_t_encumb ? _( "Channeling Time (impeded)" ) : _( "Channeling Time" ),
                                     moves_to_string( sp.casting_time( pc ) ) ), c_t_encumb  ? c_red : c_light_gray ) );

        info_txt.emplace_back( );
    } else {
        info_txt.emplace_back(
            colorize( string_format( "%s: %s", c_t_encumb ? _( "Casting Time (impeded)" ) : _( "Casting Time" ),
                                     moves_to_string( sp.casting_time( pc ) ) ), c_t_encumb  ? c_red : c_light_gray ) );

        info_txt.emplace_back( );
    };

    std::string targets;
    if( sp.is_valid_target( spell_target::none ) ) {
        targets = _( "self" );
    } else {
        targets = sp.enumerate_targets();
    }
    info_txt.emplace_back(
        colorize( string_format( "%s: %s", _( "Valid Targets" ), targets ), c_light_gray ) );

    info_txt.emplace_back( );

    std::string target_ids;
    target_ids = sp.list_targeted_monster_names();
    if( !target_ids.empty() ) {
        for( const std::string &line :
             foldstring( string_format( _( "Only affects the monsters: %s" ), target_ids ), width ) ) {
            info_txt.emplace_back( colorize( line, c_light_gray ) );
        }
        info_txt.emplace_back( );
    }

    const int damage = sp.damage( pc );
    std::string damage_string;
    std::string aoe_string;
    // if it's any type of attack spell, the stats are normal.
    if( sp.effect() == "attack" ) {
        if( damage > 0 ) {
            std::string dot_string;
            if( sp.damage_dot( pc ) != 0 ) {
                //~ amount of damage per second, abbreviated
                dot_string = string_format( _( ", %d/sec" ), sp.damage_dot( pc ) );
            }
            damage_string = string_format( "%s: %s %s%s", _( "Damage" ), sp.damage_string( pc ),
                                           sp.damage_type_string(), dot_string );
            damage_string = colorize( damage_string, sp.damage_type_color() );
        } else if( damage < 0 ) {
            damage_string = string_format( "%s: %s", _( "Healing" ), colorize( sp.damage_string( pc ),
                                           c_light_green ) );
        }
        if( sp.aoe( pc ) > 0 ) {
            std::string aoe_string_temp = _( "Spell Radius" );
            std::string degree_string;
            if( sp.shape() == spell_shape::cone ) {
                aoe_string_temp = _( "Cone Arc" );
                degree_string = _( "degrees" );
            } else if( sp.shape() == spell_shape::line ) {
                aoe_string_temp = _( "Line Width" );
            }
            aoe_string = string_format( "%s: %d %s", aoe_string_temp, sp.aoe( pc ), degree_string );
        }
    } else if( sp.effect() == "short_range_teleport" ) {
        if( sp.aoe( pc ) > 0 ) {
            aoe_string = string_format( "%s: %d", _( "Variance" ), sp.aoe( pc ) );
        }
    } else if( sp.effect() == "spawn_item" ) {
        if( sp.has_flag( spell_flag::SPAWN_GROUP ) ) {
            // todo: more user-friendly presentation
            damage_string = string_format( _( "Spawn item group %1$s %2$d times" ), sp.effect_data(),
                                           sp.damage( pc ) );
        } else {
            damage_string = string_format( "%s %d %s", _( "Spawn" ), sp.damage( pc ),
                                           item::nname( itype_id( sp.effect_data() ), sp.damage( pc ) ) );
        }
    } else if( sp.effect() == "summon" ) {
        std::string monster_name = "FIXME";
        if( sp.has_flag( spell_flag::SPAWN_GROUP ) ) {
            // TODO: Get a more user-friendly group name
            if( MonsterGroupManager::isValidMonsterGroup( mongroup_id( sp.effect_data() ) ) ) {
                monster_name = "from " + sp.effect_data();
            } else {
                debugmsg( "Unknown monster group: %s", sp.effect_data() );
            }
        } else {
            monster_name = monster( mtype_id( sp.effect_data() ) ).get_name( );
        }
        damage_string = string_format( "%s %d %s", _( "Summon" ), sp.damage( pc ), monster_name );
        aoe_string = string_format( "%s: %d", _( "Spell Radius" ), sp.aoe( pc ) );
    } else if( sp.effect() == "targeted_polymorph" ) {
        std::string monster_name = sp.effect_data();
        if( sp.has_flag( spell_flag::POLYMORPH_GROUP ) ) {
            // TODO: Get a more user-friendly group name
            if( MonsterGroupManager::isValidMonsterGroup( mongroup_id( sp.effect_data() ) ) ) {
                monster_name = _( "random creature" );
            } else {
                debugmsg( "Unknown monster group: %s", sp.effect_data() );
            }
        } else if( monster_name.empty() ) {
            monster_name = _( "random creature" );
        } else {
            monster_name = mtype_id( sp.effect_data() )->nname();
        }
        damage_string = string_format( _( "Targets under: %dhp become a %s" ), sp.damage( pc ),
                                       monster_name );
    } else if( sp.effect() == "ter_transform" ) {
        aoe_string = string_format( "%s: %s", _( "Spell Radius" ), sp.aoe_string( pc ) );
    } else if( sp.effect() == "banishment" ) {
        damage_string = string_format( "%s: %s %s", _( "Damage" ), sp.damage_string( pc ),
                                       sp.damage_type_string() );
        if( sp.aoe( pc ) > 0 ) {
            aoe_string = string_format( _( "Spell Radius: %d" ), sp.aoe( pc ) );
        }
    }

    // Range / AOE in two columns
    info_txt.emplace_back( colorize( string_format( "%s: %s", _( "Range" ),
                                     sp.range( pc ) <= 0 ? _( "self" ) : std::to_string( sp.range( pc ) ) ), c_light_gray ) );
    info_txt.emplace_back( colorize( aoe_string, c_light_gray ) );

    // One line for damage / healing / spawn / summon effect
    info_txt.emplace_back( colorize( damage_string, c_light_gray ) );

    // todo: damage over time here, when it gets implemented

    // Show duration for spells that endure
    if( sp.duration( pc ) > 0 || sp.has_flag( spell_flag::PERMANENT ) ||
        sp.has_flag( spell_flag::PERMANENT_ALL_LEVELS ) ) {
        info_txt.emplace_back(
            colorize( string_format( "%s: %s", _( "Duration" ), sp.duration_string( pc ) ), c_light_gray ) );
    }

    if( sp.has_components() ) {
        if( !sp.components().get_components().empty() ) {
            for( const std::string &line : sp.components().get_folded_components_list(
                     width - 2, c_light_gray, pc.crafting_inventory( pc.pos(), 0, false ), return_true<item> ) ) {
                info_txt.emplace_back( line );
            }
        }
        if( !( sp.components().get_tools().empty() && sp.components().get_qualities().empty() ) ) {
            for( const std::string &line : sp.components().get_folded_tools_list(
                     width - 2, c_light_gray, pc.crafting_inventory( pc.pos(), 0, false ) ) ) {
                info_txt.emplace_back( line );
            }
        }
    }
}

void spellcasting_callback::draw_spell_info( const uilist *menu )
{
    const int h_offset = menu->w_width - menu->pad_right + 1;
    int row = 1;
    nc_color clr = c_light_gray;

    for( int line = scroll_pos; row < menu->w_height - 1 &&
         line < static_cast<int>( info_txt.size() ); row++, line++ ) {
        print_colored_text( menu->window, point( h_offset + 1, row ), clr, c_light_gray, info_txt[line] );
    }
}

bool known_magic::set_invlet( const spell_id &sp, int invlet, const std::set<int> &used_invlets )
{
    if( used_invlets.count( invlet ) > 0 ) {
        return false;
    }
    invlets[sp] = invlet;
    return true;
}

void known_magic::rem_invlet( const spell_id &sp )
{
    invlets.erase( sp );
}

void known_magic::toggle_favorite( const spell_id &sp )
{
    if( favorites.count( sp ) > 0 ) {
        favorites.erase( sp );
    } else {
        favorites.emplace( sp );
    }
}

bool known_magic::is_favorite( const spell_id &sp )
{
    return favorites.count( sp ) > 0;
}

int known_magic::get_invlet( const spell_id &sp, std::set<int> &used_invlets )
{
    auto found = invlets.find( sp );
    if( found != invlets.end() ) {
        return found->second;
    }
    for( const std::pair<const spell_id, int> &invlet_pair : invlets ) {
        used_invlets.emplace( invlet_pair.second );
    }
    for( int i = 'a'; i <= 'z'; i++ ) {
        if( used_invlets.count( i ) == 0 ) {
            used_invlets.emplace( i );
            return i;
        }
    }
    for( int i = 'A'; i <= 'Z'; i++ ) {
        if( used_invlets.count( i ) == 0 ) {
            used_invlets.emplace( i );
            return i;
        }
    }
    for( int i = '!'; i <= '-'; i++ ) {
        if( used_invlets.count( i ) == 0 ) {
            used_invlets.emplace( i );
            return i;
        }
    }
    return 0;
}

int known_magic::select_spell( Character &guy )
{
    // max width of spell names
    const int max_spell_name_length = get_spellname_max_width();
    std::vector<spell *> known_spells = get_spells();

    uilist spell_menu;
    spell_menu.w_height_setup = [&]() -> int {
        return clamp( static_cast<int>( known_spells.size() ), 24, TERMY * 9 / 10 );
    };
    const auto calc_width = []() -> int {
        return std::max( 80, TERMX * 3 / 8 );
    };
    spell_menu.w_width_setup = calc_width;
    spell_menu.pad_right_setup = [&]() -> int {
        return calc_width() - max_spell_name_length - 5;
    };
    spell_menu.title = _( "Choose a Spell" );
    spell_menu.input_category = "SPELL_MENU";
    spell_menu.additional_actions.emplace_back( "CHOOSE_INVLET", translation() );
    spell_menu.additional_actions.emplace_back( "CAST_IGNORE", translation() );
    spell_menu.additional_actions.emplace_back( "SCROLL_UP_SPELL_MENU", translation() );
    spell_menu.additional_actions.emplace_back( "SCROLL_DOWN_SPELL_MENU", translation() );
    spell_menu.additional_actions.emplace_back( "SCROLL_FAVORITE", translation() );
    spell_menu.hilight_disabled = true;
    spellcasting_callback cb( known_spells, casting_ignore );
    spell_menu.callback = &cb;
    spell_menu.add_category( "all", _( "All" ) );
    spell_menu.add_category( "favorites", _( "Favorites" ) );

    std::vector<std::pair<std::string, std::string>> categories;
    for( const spell *s : known_spells ) {
        if( s->can_cast( guy ) && s->spell_class().is_valid() ) {
            categories.emplace_back( s->spell_class().str(), s->spell_class().obj().name() );
            std::sort( categories.begin(), categories.end(), []( const std::pair<std::string, std::string> &a,
            const std::pair<std::string, std::string> &b ) {
                return localized_compare( a.second, b.second );
            } );
            const auto itr = std::unique( categories.begin(), categories.end() );
            categories.erase( itr, categories.end() );
        }
    }
    for( std::pair<std::string, std::string> &cat : categories ) {
        spell_menu.add_category( cat.first, cat.second );
    }

    spell_menu.set_category_filter( [&guy, known_spells]( const uilist_entry & entry,
    const std::string & key )->bool {
        if( key == "all" )
        {
            return true;
        } else if( key == "favorites" )
        {
            return guy.magic->is_favorite( known_spells[entry.retval]->id() );
        }
        return known_spells[entry.retval]->spell_class().is_valid() && known_spells[entry.retval]->spell_class().str() == key;
    } );
    if( !favorites.empty() ) {
        spell_menu.set_category( "favorites" );
    } else {
        spell_menu.set_category( "all" );
    }

    std::set<int> used_invlets{ cb.reserved_invlets };

    for( size_t i = 0; i < known_spells.size(); i++ ) {
        spell_menu.addentry( static_cast<int>( i ), known_spells[i]->can_cast( guy ),
                             get_invlet( known_spells[i]->id(), used_invlets ), known_spells[i]->name() );
    }
    reflesh_favorite( &spell_menu, known_spells );

    spell_menu.query();

    casting_ignore = static_cast<spellcasting_callback *>( spell_menu.callback )->casting_ignore;

    return spell_menu.ret;
}

void known_magic::on_mutation_gain( const trait_id &mid, Character &guy )
{
    for( const std::pair<const spell_id, int> &sp : mid->spells_learned ) {
        learn_spell( sp.first, guy, true );
        spell &temp_sp = get_spell( sp.first );
        for( int level = 0; level < sp.second; level++ ) {
            temp_sp.gain_level( guy );
        }
    }
}

void known_magic::on_mutation_loss( const trait_id &mid )
{
    std::vector<spell_id> spells_to_forget;
    for( const spell *sp : get_spells() ) {
        if( sp->is_spell_class( mid ) ) {
            spells_to_forget.emplace_back( sp->id() );
        }
    }
    for( const spell_id &sp_id : spells_to_forget ) {
        forget_spell( sp_id );
    }
}

void spellbook_callback::add_spell( const spell_id &sp )
{
    spells.emplace_back( sp.obj() );
}

static std::string color_number( const int num )
{
    if( num > 0 ) {
        return colorize( std::to_string( num ), c_light_green );
    } else if( num < 0 ) {
        return colorize( std::to_string( num ), c_light_red );
    } else {
        return colorize( std::to_string( num ), c_white );
    }
}

static std::string color_number( const float num )
{
    if( num > 100 ) {
        return colorize( string_format( "+%.0f", num ), c_light_green );
    } else if( num < -100 ) {
        return colorize( string_format( "%.0f", num ), c_light_red );
    } else if( num > 0 ) {
        return colorize( string_format( "+%.2f", num ), c_light_green );
    } else if( num < 0 ) {
        return colorize( string_format( "%.2f", num ), c_light_red );
    } else {
        return colorize( "0", c_white );
    }
}

static void draw_spellbook_info( const spell_type &sp, uilist *menu )
{
    const int width = menu->pad_left - 4;
    const int start_x = 2;
    int line = 1;
    const catacurses::window w = menu->window;
    nc_color gray = c_light_gray;
    nc_color yellow = c_yellow;
    const spell fake_spell( sp.id );
    Character &pc = get_player_character();
    dialogue d( get_talker_for( pc ), nullptr );

    const std::string spell_name = colorize( sp.name, c_light_green );
    const std::string spell_class = sp.spell_class == trait_NONE ? _( "Classless" ) :
                                    sp.spell_class->name();
    print_colored_text( w, point( start_x, line ), gray, gray, spell_name );
    print_colored_text( w, point( menu->pad_left - utf8_width( spell_class ) - 1, line++ ), yellow,
                        yellow, spell_class );
    line++;
    line += fold_and_print( w, point( start_x, line ), width, gray, "%s", sp.description );
    line++;

    mvwprintz( w, point( start_x, line ), c_light_gray,
               string_format( "%s: %d", _( "Difficulty" ), static_cast<int>( sp.difficulty.evaluate( d ) ) ) );
    mvwprintz( w, point( start_x + width / 2, line++ ), c_light_gray,
               string_format( "%s: %d", _( "Max Level" ), static_cast<int>( sp.max_level.evaluate( d ) ) ) );

    const std::string fx = sp.effect_name;
    std::string damage_string;
    std::string aoe_string;
    bool has_damage_type = false;
    if( fx == "attack" ) {
        damage_string = _( "Damage" );
        aoe_string = _( "AoE" );
        has_damage_type = sp.min_damage.evaluate( d ) > 0 && sp.max_damage.evaluate( d ) > 0;
    } else if( fx == "spawn_item" || fx == "summon_monster" ) {
        damage_string = _( "Spawned" );
    } else if( fx == "targeted_polymorph" ) {
        damage_string = _( "Threshold" );
    } else if( fx == "recover_energy" ) {
        damage_string = _( "Recover" );
    } else if( fx == "short_range_teleport" ) {
        aoe_string = _( "Variance" );
    } else if( fx == "area_pull" || fx == "area_push" ||  fx == "ter_transform" ) {
        aoe_string = _( "AoE" );
    }

    if( has_damage_type ) {
        print_colored_text( w, point( start_x, line++ ), gray, gray, string_format( "%s: %s",
                            _( "Damage Type" ),
                            colorize( fake_spell.damage_type_string(), fake_spell.damage_type_color() ) ) );
    }
    line++;

    print_colored_text( w, point( start_x, line++ ), gray, gray,
                        string_format( "%s %s %s %s",
                                       //~ translation should not exceed 10 console cells
                                       left_justify( _( "Stat Gain" ), 10 ),
                                       //~ translation should not exceed 7 console cells
                                       left_justify( _( "lvl 0" ), 7 ),
                                       //~ translation should not exceed 7 console cells
                                       left_justify( _( "per lvl" ), 7 ),
                                       //~ translation should not exceed 7 console cells
                                       left_justify( _( "max lvl" ), 7 ) ) );

    const auto row = [&]( const std::string & label, const dbl_or_var & min_d,
    const dbl_or_var & inc_d, const dbl_or_var & max_d, bool check_minmax = false ) {
        const int min = static_cast<int>( min_d.evaluate( d ) );
        const float inc = static_cast<float>( inc_d.evaluate( d ) );
        const int max = static_cast<int>( max_d.evaluate( d ) );
        if( check_minmax && ( min == 0 || max == 0 ) ) {
            return;
        }
        mvwprintz( w, point( start_x, line ), c_light_gray, label );
        print_colored_text( w, point( start_x + 11, line ), gray, gray, color_number( min ) );
        print_colored_text( w, point( start_x + 19, line ), gray, gray, color_number( inc ) );
        print_colored_text( w, point( start_x + 27, line ), gray, gray, color_number( max ) );
        line++;
    };

    if( !damage_string.empty() ) {
        row( damage_string, sp.min_damage, sp.damage_increment, sp.max_damage, true );
    }

    row( _( "Range" ), sp.min_range, sp.range_increment, sp.max_range, true );

    if( !aoe_string.empty() ) {
        row( aoe_string, sp.min_aoe, sp.aoe_increment, sp.max_aoe, true );
    }

    row( _( "Duration" ), sp.min_duration, sp.duration_increment, sp.max_duration, true );
    row( _( "Cast Cost" ), sp.base_energy_cost, sp.energy_increment, sp.final_energy_cost, false );
    row( _( "Cast Time" ), sp.base_casting_time, sp.casting_time_increment, sp.final_casting_time,
         false );
}

void spellbook_callback::refresh( uilist *menu )
{
    mvwputch( menu->window, point( menu->pad_left, 0 ), c_magenta, LINE_OXXX );
    mvwputch( menu->window, point( menu->pad_left, menu->w_height - 1 ), c_magenta, LINE_XXOX );
    for( int i = 1; i < menu->w_height - 1; i++ ) {
        mvwputch( menu->window, point( menu->pad_left, i ), c_magenta, LINE_XOXO );
    }
    if( menu->selected >= 0 && static_cast<size_t>( menu->selected ) < spells.size() ) {
        draw_spellbook_info( spells[menu->selected], menu );
    }
    wnoutrefresh( menu->window );
}

bool fake_spell::is_valid() const
{
    return id.is_valid() && !id.is_empty();
}

void fake_spell::load( const JsonObject &jo )
{
    mandatory( jo, false, "id", id );
    optional( jo, false, "hit_self", self, self_default );

    optional( jo, false, "once_in", trigger_once_in, trigger_once_in_default );
    optional( jo, false, "message", trigger_message );
    optional( jo, false, "npc_message", npc_trigger_message );
    int max_level_int;
    optional( jo, false, "max_level", max_level_int, -1 );
    if( max_level_int == -1 ) {
        max_level = std::nullopt;
    } else {
        max_level = max_level_int;
    }
    optional( jo, false, "min_level", level, level_default );
    if( jo.has_string( "level" ) ) {
        debugmsg( "level member for fake_spell was renamed to min_level.  id: %s", id.c_str() );
    }
}

void fake_spell::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "id", id );
    json.member( "hit_self", self, self_default );
    json.member( "once_in", trigger_once_in, trigger_once_in_default );
    json.member( "max_level", max_level, max_level_default );
    json.member( "min_level", level, level_default );
    json.end_object();
}

void fake_spell::deserialize( const JsonObject &data )
{
    load( data );
}

spell fake_spell::get_spell( const Creature &caster, int min_level_override ) const
{
    spell sp( id );
    // the max level this spell will be. can be optionally limited
    int spell_max_level = sp.get_max_level( caster );
    int spell_limiter = max_level ? *max_level : spell_max_level;
    // level is the minimum level the fake_spell will output
    min_level_override = std::max( min_level_override, level );
    if( min_level_override > spell_limiter ) {
        // this override is for min level, and does not override max level
        if( spell_limiter <= 0 ) {
            spell_limiter = min_level_override;
        } else {
            min_level_override = spell_limiter;
        }
    }
    // make sure max level is not lower then min level
    spell_limiter = std::max( min_level_override, spell_limiter );
    // the "level" of the fake spell is the goal, but needs to be clamped to min and max
    int level_of_spell = clamp( level, min_level_override, spell_limiter );
    if( level > spell_limiter ) {
        debugmsg( "ERROR: fake spell %s has higher min_level than max_level", id.c_str() );
        return sp;
    }
    sp.set_exp( sp.exp_for_level( level_of_spell ) );

    return sp;
}

// intended for spells without casters
spell fake_spell::get_spell() const
{
    spell sp( id );
    return sp;
}

void spell_events::notify( const cata::event &e )
{
    switch( e.type() ) {
        case event_type::player_levels_spell: {
            spell_id sid = e.get<spell_id>( "spell" );
            int slvl = e.get<int>( "new_level" );
            spell_type spell_cast = spell_factory.obj( sid );
            for( std::map<std::string, int>::iterator it = spell_cast.learn_spells.begin();
                 it != spell_cast.learn_spells.end(); ++it ) {
                int learn_at_level = it->second;
                const std::string learn_spell_id = it->first;
                if( learn_at_level <= slvl && !get_player_character().magic->knows_spell( learn_spell_id ) ) {
                    get_player_character().magic->learn_spell( learn_spell_id, get_player_character() );
                    spell_type spell_learned = spell_factory.obj( spell_id( learn_spell_id ) );
                    add_msg(
                        _( "Your experience and knowledge in creating and manipulating magical energies to cast %s have opened your eyes to new possibilities, you can now cast %s." ),
                        spell_cast.name,
                        spell_learned.name );
                }
            }
            break;
        }
        default:
            break;

    }
}
