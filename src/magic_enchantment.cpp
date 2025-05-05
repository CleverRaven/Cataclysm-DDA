#include "magic_enchantment.h"

#include <memory>
#include <set>
#include <string>

#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "condition.h"
#include "coordinates.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "dialogue.h"
#include "dialogue_helpers.h"
#include "enum_conversions.h"
#include "enums.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "item.h"
#include "json.h"
#include "map.h"
#include "mod_tracker.h"
#include "monster.h"
#include "rng.h"
#include "skill.h"
#include "talker.h"
#include "units.h"

namespace io
{
    // *INDENT-OFF*
    template<>
    std::string enum_to_string<enchantment::has>( enchantment::has data )
    {
        switch ( data ) {
        case enchantment::has::HELD: return "HELD";
        case enchantment::has::WIELD: return "WIELD";
        case enchantment::has::WORN: return "WORN";
        case enchantment::has::NUM_HAS: break;
        }
        cata_fatal( "Invalid enchantment::has" );
    }

    template<>
    std::string enum_to_string<enchantment::condition>( enchantment::condition data )
    {
        switch ( data ) {
        case enchantment::condition::ALWAYS: return "ALWAYS";
        case enchantment::condition::ACTIVE: return "ACTIVE";
        case enchantment::condition::INACTIVE: return "INACTIVE";
        case enchantment::condition::DIALOG_CONDITION: return "DIALOG_CONDITION";
        case enchantment::condition::NUM_CONDITION: break;
        }
        cata_fatal( "Invalid enchantment::condition" );
    }

    template<>
    std::string enum_to_string<enchant_vals::mod>( enchant_vals::mod data )
    {
        switch ( data ) {
            case enchant_vals::mod::ARTIFACT_RESONANCE: return "ARTIFACT_RESONANCE";
            case enchant_vals::mod::STRENGTH: return "STRENGTH";
            case enchant_vals::mod::DEXTERITY: return "DEXTERITY";
            case enchant_vals::mod::PERCEPTION: return "PERCEPTION";
            case enchant_vals::mod::INTELLIGENCE: return "INTELLIGENCE";
            case enchant_vals::mod::SPEED: return "SPEED";
            case enchant_vals::mod::ATTACK_SPEED: return "ATTACK_SPEED";
            case enchant_vals::mod::MOVE_COST: return "MOVE_COST";
            case enchant_vals::mod::METABOLISM: return "METABOLISM";
            case enchant_vals::mod::MAX_MANA: return "MAX_MANA";
            case enchant_vals::mod::REGEN_MANA: return "REGEN_MANA";
            case enchant_vals::mod::BIONIC_POWER: return "BIONIC_POWER";
            case enchant_vals::mod::POWER_TRICKLE: return "POWER_TRICKLE";
            case enchant_vals::mod::MAX_STAMINA: return "MAX_STAMINA";
            case enchant_vals::mod::REGEN_STAMINA: return "REGEN_STAMINA";
            case enchant_vals::mod::FAT_TO_MAX_HP: return "FAT_TO_MAX_HP";
            case enchant_vals::mod::CARDIO_MULTIPLIER: return "CARDIO_MULTIPLIER";
            case enchant_vals::mod::MAX_HP: return "MAX_HP";
            case enchant_vals::mod::REGEN_HP: return "REGEN_HP";
            case enchant_vals::mod::REGEN_HP_AWAKE: return "REGEN_HP_AWAKE";
            case enchant_vals::mod::MUT_INSTABILITY_MOD: return "MUT_INSTABILITY_MOD";
            case enchant_vals::mod::MUT_ADDITIONAL_OPTIONS: return "MUT_ADDITIONAL_OPTIONS";
            case enchant_vals::mod::RANGE_DODGE: return "RANGE_DODGE";
            case enchant_vals::mod::HUNGER: return "HUNGER";
            case enchant_vals::mod::THIRST: return "THIRST";
            case enchant_vals::mod::SLEEPINESS: return "SLEEPINESS";
            case enchant_vals::mod::SLEEPINESS_REGEN: return "SLEEPINESS_REGEN";
            case enchant_vals::mod::PAIN: return "PAIN";
            case enchant_vals::mod::PAIN_REMOVE: return "PAIN_REMOVE";
            case enchant_vals::mod::PAIN_PENALTY_MOD_STR: return "PAIN_PENALTY_MOD_STR";
            case enchant_vals::mod::PAIN_PENALTY_MOD_DEX: return "PAIN_PENALTY_MOD_DEX";
            case enchant_vals::mod::PAIN_PENALTY_MOD_INT: return "PAIN_PENALTY_MOD_INT";
            case enchant_vals::mod::PAIN_PENALTY_MOD_PER: return "PAIN_PENALTY_MOD_PER";
            case enchant_vals::mod::PAIN_PENALTY_MOD_SPEED: return "PAIN_PENALTY_MOD_SPEED";
            case enchant_vals::mod::MELEE_DAMAGE: return "MELEE_DAMAGE";
            case enchant_vals::mod::MELEE_RANGE_MODIFIER: return "MELEE_RANGE_MODIFIER";
            case enchant_vals::mod::MELEE_TO_HIT: return "MELEE_TO_HIT";
            case enchant_vals::mod::RANGED_DAMAGE: return "RANGED_DAMAGE";
			case enchant_vals::mod::RANGED_ARMOR_PENETRATION: return "RANGED_ARMOR_PENETRATION";
            case enchant_vals::mod::DODGE_CHANCE: return "DODGE_CHANCE";
            case enchant_vals::mod::BONUS_BLOCK: return "BONUS_BLOCK";
            case enchant_vals::mod::BONUS_DODGE: return "BONUS_DODGE";
            case enchant_vals::mod::ATTACK_NOISE: return "ATTACK_NOISE";
            case enchant_vals::mod::SHOUT_NOISE: return "SHOUT_NOISE";
            case enchant_vals::mod::FOOTSTEP_NOISE: return "FOOTSTEP_NOISE";
            case enchant_vals::mod::VISION_RANGE: return "VISION_RANGE";
            case enchant_vals::mod::CARRY_WEIGHT: return "CARRY_WEIGHT";
            case enchant_vals::mod::WEAPON_DISPERSION: return "WEAPON_DISPERSION";
            case enchant_vals::mod::SOCIAL_LIE: return "SOCIAL_LIE";
            case enchant_vals::mod::SOCIAL_PERSUADE: return "SOCIAL_PERSUADE";
            case enchant_vals::mod::SOCIAL_INTIMIDATE: return "SOCIAL_INTIMIDATE";
            case enchant_vals::mod::SLEEPY: return "SLEEPY";
            case enchant_vals::mod::BODYTEMP_SLEEP: return "BODYTEMP_SLEEP";
            case enchant_vals::mod::LUMINATION: return "LUMINATION";
            case enchant_vals::mod::EFFECTIVE_HEALTH_MOD: return "EFFECTIVE_HEALTH_MOD";
            case enchant_vals::mod::MOD_HEALTH: return "MOD_HEALTH";
            case enchant_vals::mod::MOD_HEALTH_CAP: return "MOD_HEALTH_CAP";
            case enchant_vals::mod::HEALTHY_RATE: return "HEALTHY_RATE";
            case enchant_vals::mod::READING_EXP: return "READING_EXP";
            case enchant_vals::mod::SKILL_RUST_RESIST: return "SKILL_RUST_RESIST";
            case enchant_vals::mod::OVERMAP_SIGHT: return "OVERMAP_SIGHT";
            case enchant_vals::mod::READING_SPEED_MULTIPLIER: return "READING_SPEED_MULTIPLIER";
            case enchant_vals::mod::KCAL: return "KCAL";
            case enchant_vals::mod::VITAMIN_ABSORB_MOD: return "VITAMIN_ABSORB_MOD";
            case enchant_vals::mod::MELEE_STAMINA_CONSUMPTION: return "MELEE_STAMINA_CONSUMPTION";
            case enchant_vals::mod::OBTAIN_COST_MULTIPLIER: return "OBTAIN_COST_MULTIPLIER";
            case enchant_vals::mod::CASTING_TIME_MULTIPLIER: return "CASTING_TIME_MULTIPLIER";
            case enchant_vals::mod::CRAFTING_SPEED_MULTIPLIER: return "CRAFTING_SPEED_MULTIPLIER";
            case enchant_vals::mod::BIONIC_MANA_PENALTY: return "BIONIC_MANA_PENALTY";
            case enchant_vals::mod::STEALTH_MODIFIER: return "STEALTH_MODIFIER";
            case enchant_vals::mod::WEAKNESS_TO_WATER: return "WEAKNESS_TO_WATER";
            case enchant_vals::mod::MENDING_MODIFIER: return "MENDING_MODIFIER";
            case enchant_vals::mod::STOMACH_SIZE_MULTIPLIER: return "STOMACH_SIZE_MULTIPLIER";
            case enchant_vals::mod::LEARNING_FOCUS: return "LEARNING_FOCUS";
            case enchant_vals::mod::RECOIL_MODIFIER: return "RECOIL_MODIFIER";
            case enchant_vals::mod::ARMOR_ALL: return "ARMOR_ALL";
            case enchant_vals::mod::EXTRA_ELEC_PAIN: return "EXTRA_ELEC_PAIN";
            case enchant_vals::mod::ITEM_ATTACK_SPEED: return "ITEM_ATTACK_SPEED";
            case enchant_vals::mod::EQUIPMENT_DAMAGE_CHANCE: return "EQUIPMENT_DAMAGE_CHANCE";
            case enchant_vals::mod::CLIMATE_CONTROL_HEAT: return "CLIMATE_CONTROL_HEAT";
            case enchant_vals::mod::CLIMATE_CONTROL_CHILL: return "CLIMATE_CONTROL_CHILL";
            case enchant_vals::mod::COMBAT_CATCHUP: return "COMBAT_CATCHUP";
            case enchant_vals::mod::KNOCKBACK_RESIST: return "KNOCKBACK_RESIST";
            case enchant_vals::mod::KNOCKDOWN_RESIST: return "KNOCKDOWN_RESIST";
            case enchant_vals::mod::FALL_DAMAGE: return "FALL_DAMAGE";
            case enchant_vals::mod::FORCEFIELD: return "FORCEFIELD";
            case enchant_vals::mod::EVASION: return "EVASION";
            case enchant_vals::mod::OVERKILL_DAMAGE: return "OVERKILL_DAMAGE";
            case enchant_vals::mod::RANGE: return "RANGE";
            case enchant_vals::mod::AVOID_FRIENDRY_FIRE: return "AVOID_FRIENDRY_FIRE";
            case enchant_vals::mod::MOVECOST_SWIM_MOD: return "MOVECOST_SWIM_MOD";
            case enchant_vals::mod::MOVECOST_OBSTACLE_MOD: return "MOVECOST_OBSTACLE_MOD";
            case enchant_vals::mod::MOVECOST_FLATGROUND_MOD: return "MOVECOST_FLATGROUND_MOD";
            case enchant_vals::mod::PHASE_DISTANCE: return "PHASE_DISTANCE";
            case enchant_vals::mod::SHOUT_NOISE_STR_MULT: return "SHOUT_NOISE_STR_MULT";
            case enchant_vals::mod::NIGHT_VIS: return "NIGHT_VIS";
            case enchant_vals::mod::HEARING_MULT: return "HEARING_MULT";
            case enchant_vals::mod::BANDAGE_BONUS: return "BANDAGE_BONUS";
            case enchant_vals::mod::DISINFECTANT_BONUS: return "DISINFECTANT_BONUS";
            case enchant_vals::mod::BLEED_STOP_BONUS: return "BLEED_STOP_BONUS";
            case enchant_vals::mod::UGLINESS: return "UGLINESS";
            case enchant_vals::mod::VOMIT_MUL: return "VOMIT_MUL";
            case enchant_vals::mod::SCENT_MASK: return "SCENT_MASK";
            case enchant_vals::mod::CONSUME_TIME_MOD: return "CONSUME_TIME_MOD";
            case enchant_vals::mod::THROW_STR: return "THROW_STR";
            case enchant_vals::mod::THROW_DAMAGE: return "THROW_DAMAGE";
            case enchant_vals::mod::SWEAT_MULTIPLIER: return "SWEAT_MULTIPLIER";
            case enchant_vals::mod::STAMINA_REGEN_MOD: return "STAMINA_REGEN_MOD";
            case enchant_vals::mod::MOVEMENT_EXERTION_MODIFIER: return "MOVEMENT_EXERTION_MODIFIER";
            case enchant_vals::mod::WEAKPOINT_ACCURACY: return "WEAKPOINT_ACCURACY";
            case enchant_vals::mod::MOTION_ALARM: return "MOTION_ALARM";
            case enchant_vals::mod::NUM_MOD: break;
        }
        cata_fatal( "Invalid enchant_vals::mod" );
    }
    // *INDENT-ON*
} // namespace io

namespace
{
generic_factory<enchantment> spell_factory( "enchantment" );
} // namespace

template<>
const enchantment &string_id<enchantment>::obj() const
{
    return spell_factory.obj( *this );
}

template<>
bool string_id<enchantment>::is_valid() const
{
    return spell_factory.is_valid( *this );
}

template<typename TKey>
void load_add_and_multiply( const JsonObject &jo, const bool &is_child,
                            std::string_view array_key, const std::string &type_key, std::map<TKey, dbl_or_var> &add_map,
                            std::map<TKey, dbl_or_var> &mult_map )
{
    if( !is_child && jo.has_array( array_key ) ) {
        for( const JsonObject value_obj : jo.get_array( array_key ) ) {

            TKey value;
            mandatory( value_obj, false, type_key, value );

            if( value_obj.has_member( "add" ) ) {
                dbl_or_var add = get_dbl_or_var( value_obj, "add", false );
                add_map.emplace( value, add );
            }

            if( value_obj.has_member( "multiply" ) ) {
                dbl_or_var mult;
                if( value_obj.has_float( "multiply" ) ) {
                    mult.max.dbl_val = mult.min.dbl_val = value_obj.get_float( "multiply" );
                } else {
                    mult = get_dbl_or_var( value_obj, "multiply", false );

                }
                mult_map.emplace( value, mult );
            }

        }
    }
}

template<typename TKey>
void load_add_and_multiply( const JsonObject &jo, std::string_view array_key,
                            const std::string &type_key, std::map<TKey, double> &add_map, std::map<TKey, double> &mult_map )
{
    if( jo.has_array( array_key ) ) {
        for( const JsonObject value_obj : jo.get_array( array_key ) ) {

            const TKey value = TKey( value_obj.get_string( type_key ) );
            const double add = value_obj.get_float( "add", 0.0 );
            const double mult = value_obj.get_float( "multiply", 0.0 );

            if( add != 0.0 ) {
                add_map.emplace( value, add );
            }

            if( mult != 0.0 ) {
                mult_map.emplace( value, mult );
            }

        }
    }
}

void enchantment::load_enchantment( const JsonObject &jo, const std::string &src )
{
    spell_factory.load( jo, src );
}

void enchantment::reset()
{
    spell_factory.reset();
}

enchantment_id enchantment::load_inline_enchantment( const JsonValue &jv,
        std::string_view src,
        std::string &inline_id )
{
    if( jv.test_string() ) {
        return enchantment_id( jv.get_string() );
    } else if( jv.test_object() ) {
        if( inline_id.empty() ) {
            jv.throw_error( "Inline enchantment cannot be created without an id." );
        }

        enchantment inline_enchant;
        inline_enchant.load( jv.get_object(), src, inline_id );
        mod_tracker::assign_src( inline_enchant, src );
        spell_factory.insert( inline_enchant );
        return enchantment_id( inline_id );
    } else {
        jv.throw_error( "Enchantment needs to be either string or enchantment object." );
    }
}

bool enchantment::is_active( const Character &guy, const item &parent ) const
{
    if( !guy.has_item( parent ) ) {
        return false;
    }

    if( active_conditions.first == has::HELD &&
        active_conditions.second == condition::ALWAYS ) {
        return true;
    }

    if( !( active_conditions.first == has::HELD ||
           ( active_conditions.first == has::WIELD && guy.is_wielding( parent ) ) ||
           ( active_conditions.first == has::WORN &&
             ( guy.is_worn( parent ) || guy.is_worn_module( parent ) ) ) ) ) {
        return false;
    }

    return is_active( guy, parent.active );
}

bool enchantment::is_active( const Character &guy, const bool active ) const
{
    if( active_conditions.second == condition::ACTIVE ) {
        return active;
    }

    if( active_conditions.second == condition::INACTIVE ) {
        return !active;
    }

    if( active_conditions.second == condition::ALWAYS ) {
        return true;
    }

    if( active_conditions.second == condition::DIALOG_CONDITION ) {
        const_dialogue d( get_const_talker_for( guy ), nullptr );
        return dialog_condition( d );
    }
    return false;
}

bool enchantment::is_active( const monster &mon ) const
{
    if( active_conditions.second == condition::ALWAYS ) {
        return true;
    }

    if( active_conditions.second == condition::DIALOG_CONDITION ) {
        const_dialogue d( get_const_talker_for( mon ), nullptr );
        return dialog_condition( d );
    }

    return false;
}

// Returns true if this enchantment is relevant to monsters. Enchantments that are not relevant to monsters are not processed by monsters.
bool enchantment::is_monster_relevant() const
{
    // Check add values.
    for( const std::pair<const enchant_vals::mod, dbl_or_var> &pair_values :
         values_add ) {
        if( pair_values.first == enchant_vals::mod::ARMOR_ALL ||
            pair_values.first == enchant_vals::mod::REGEN_HP ||
            pair_values.first == enchant_vals::mod::VISION_RANGE ||
            pair_values.first == enchant_vals::mod::SPEED ||
            pair_values.first == enchant_vals::mod::LUMINATION ) {
            return true;
        }
    }

    // Check mult values.
    for( const std::pair<const enchant_vals::mod, dbl_or_var> &pair_values :
         values_multiply ) {
        if( pair_values.first == enchant_vals::mod::ARMOR_ALL ||
            pair_values.first == enchant_vals::mod::REGEN_HP ||
            pair_values.first == enchant_vals::mod::VISION_RANGE ||
            pair_values.first == enchant_vals::mod::SPEED ||
            pair_values.first == enchant_vals::mod::LUMINATION ) {
            return true;
        }
    }

    // check for hit you / me effects
    if( !hit_you_effect.empty() || !hit_me_effect.empty() ) {
        return true;
    }

    if( !damage_values_add.empty() || !damage_values_multiply.empty() ||
        !armor_values_add.empty() || !armor_values_multiply.empty() ) {
        return true;
    }

    return false;
}

bool enchantment::active_wield() const
{
    return active_conditions.first == has::HELD || active_conditions.first == has::WIELD;
}

void enchantment::add_activation( const time_duration &dur, const fake_spell &fake )
{
    intermittent_activation[dur].emplace_back( fake );
}

void enchantment::bodypart_changes::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "gain", gain );
    optional( jo, was_loaded, "lose", lose );
}

void enchantment::bodypart_changes::deserialize( const JsonObject &jo )
{
    load( jo );
}

void enchantment::bodypart_changes::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "gain", gain );
    jsout.member( "lose", lose );
    jsout.end_object();
}

void enchantment::load( const JsonObject &jo, std::string_view,
                        const std::optional<std::string> &inline_id, bool is_child )
{
    optional( jo, was_loaded, "id", id, enchantment_id( inline_id.value_or( "" ) ) );

    jo.read( "hit_you_effect", hit_you_effect );
    jo.read( "hit_me_effect", hit_me_effect );
    jo.read( "emitter", emitter );

    if( jo.has_object( "intermittent_activation" ) ) {
        JsonObject jobj = jo.get_object( "intermittent_activation" );
        for( const JsonObject effect_obj : jobj.get_array( "effects" ) ) {
            time_duration dur = read_from_json_string<time_duration>( effect_obj.get_member( "frequency" ),
                                time_duration::units );
            if( effect_obj.has_array( "spell_effects" ) ) {
                for( const JsonObject fake_spell_obj : effect_obj.get_array( "spell_effects" ) ) {
                    fake_spell fake;
                    fake.load( fake_spell_obj );
                    add_activation( dur, fake );
                }
            } else if( effect_obj.has_object( "spell_effects" ) ) {
                fake_spell fake;
                JsonObject fake_spell_obj = effect_obj.get_object( "spell_effects" );
                fake.load( fake_spell_obj );
                add_activation( dur, fake );
            }
        }
    }

    active_conditions.first = io::string_to_enum<has>( jo.get_string( "has", "HELD" ) );
    if( jo.has_string( "condition" ) ) {
        std::string condit;
        optional( jo, was_loaded, "condition", condit );
        std::optional<condition> con = io::string_to_enum_optional<condition>( condit );
        if( con.has_value() ) {
            active_conditions.second = con.value();
        } else {
            active_conditions.second = condition::DIALOG_CONDITION;
        }
    } else if( jo.has_member( "condition" ) ) {
        active_conditions.second = condition::DIALOG_CONDITION;
    } else {
        active_conditions.second = condition::ALWAYS;
    }
    if( active_conditions.second == condition::DIALOG_CONDITION ) {
        read_condition( jo, "condition", dialog_condition, false );
    }

    for( JsonObject jsobj : jo.get_array( "ench_effects" ) ) {
        ench_effects.emplace( efftype_id( jsobj.get_string( "effect" ) ), jsobj.get_int( "intensity" ) );
    }

    optional( jo, was_loaded, "modified_bodyparts", modified_bodyparts );
    optional( jo, was_loaded, "mutations", mutations );

    optional( jo, was_loaded, "name", name );
    optional( jo, was_loaded, "description", description );

    load_add_and_multiply<enchant_vals::mod>( jo, is_child, "values", "value", values_add,
            values_multiply );

    load_add_and_multiply<skill_id>( jo, is_child, "skills", "value",
                                     skill_values_add, skill_values_multiply );

    load_add_and_multiply<bodypart_str_id>( jo, is_child, "encumbrance_modifier", "part",
                                            encumbrance_values_add, encumbrance_values_multiply );

    load_add_and_multiply<damage_type_id>( jo, is_child, "melee_damage_bonus", "type",
                                           damage_values_add, damage_values_multiply );

    load_add_and_multiply<damage_type_id>( jo, is_child, "incoming_damage_mod", "type",
                                           armor_values_add, armor_values_multiply );

    load_add_and_multiply<damage_type_id>( jo, is_child, "incoming_damage_mod_post_absorbed", "type",
                                           extra_damage_add, extra_damage_multiply );

    if( !is_child && jo.has_array( "special_vision" ) ) {
        for( const JsonObject vision_obj : jo.get_array( "special_vision" ) ) {
            special_vision _vision;
            special_vision_descriptions _desc;
            if( vision_obj.has_array( "descriptions" ) ) {
                for( const JsonObject descriptions_obj : vision_obj.get_array( "descriptions" ) ) {
                    mandatory( descriptions_obj, was_loaded, "id", _desc.id );
                    if( descriptions_obj.has_string( "color" ) ) {
                        _desc.color = color_from_string( descriptions_obj.get_string( "color" ) );
                    }
                    optional( descriptions_obj, was_loaded, "symbol", _desc.symbol );
                    mandatory( descriptions_obj, was_loaded, "text", _desc.description );
                    read_condition( descriptions_obj, "text_condition", _desc.condition, true );
                    _vision.special_vision_descriptions_vector.emplace_back( _desc );
                }
            }

            _vision.range = get_dbl_or_var( vision_obj, "distance" );
            read_condition( vision_obj, "condition", _vision.condition, true );
            optional( vision_obj, was_loaded, "precise", _vision.precise );
            optional( vision_obj, was_loaded, "ignores_aiming_cone", _vision.ignores_aiming_cone );
            special_vision_vector.emplace_back( _vision );
        }
    }
}

void enchant_cache::load( const JsonObject &jo, std::string_view,
                          const std::optional<std::string> &inline_id )
{
    enchantment::load( jo, "", inline_id, true );
    if( jo.has_array( "values" ) ) {

        // enchantment to be silently skipped for migration purposes
        std::set<std::string> legacy_values = {
            "ITEM_DAMAGE_ACID",
            "ITEM_DAMAGE_BIO",
            "ITEM_DAMAGE_BULLET",
            "ITEM_DAMAGE_COLD",
            "ITEM_DAMAGE_CUT",
            "ITEM_DAMAGE_ELEC",
            "ITEM_DAMAGE_HEAT",
            "ITEM_DAMAGE_PURE",
            "ITEM_DAMAGE_STAB",
            "ITEM_DAMAGE_BASH",
            "SIGHT_RANGE_ELECTRIC",
            "MOTION_VISION_RANGE",
            "SIGHT_RANGE_FAE",
            "SIGHT_RANGE_NETHER",
            "SIGHT_RANGE_MINDS",
            "ARMOR_ACID",
            "ARMOR_BASH",
            "ARMOR_BIO",
            "ARMOR_COLD",
            "ARMOR_CUT",
            "ARMOR_ELEC",
            "ARMOR_HEAT",
            "ARMOR_STAB",
            "ARMOR_BULLET",
            "EXTRA_BASH",
            "EXTRA_CUT",
            "EXTRA_STAB",
            "EXTRA_BULLET",
            "EXTRA_HEAT",
            "EXTRA_COLD",
            "EXTRA_ELEC",
            "EXTRA_ACID",
            "EXTRA_BIO",
            "ITEM_ARMOR_BASH",
            "ITEM_ARMOR_CUT",
            "ITEM_ARMOR_STAB",
            "ITEM_ARMOR_BULLET",
            "ITEM_ARMOR_HEAT",
            "ITEM_ARMOR_COLD",
            "ITEM_ARMOR_ELEC",
            "ITEM_ARMOR_ACID",
            "ITEM_ARMOR_BIO"
            // values above to be removed after 0.I
        };

        for( const JsonObject value_obj : jo.get_array( "values" ) ) {
            try {
                const enchant_vals::mod value = io::string_to_enum<enchant_vals::mod>
                                                ( value_obj.get_string( "value" ) );
                const int add = value_obj.has_int( "add" ) ? value_obj.get_int( "add", 0 ) : 0;
                const double mult = value_obj.has_float( "multiply" ) ? value_obj.get_float( "multiply",
                                    0.0 ) : 0.0;
                if( add != 0 ) {
                    values_add.emplace( value, add );
                }
                if( mult != 0.0 ) {
                    values_multiply.emplace( value, mult );
                }
            } catch( ... ) {
                if( legacy_values.find( value_obj.get_string( "value", "" ) ) == legacy_values.end() ) {
                    debugmsg( "A relic attempted to load invalid enchantment %s.", value_obj.get_string( "value",
                              "" ) );
                }
            }
        }
    }

    load_add_and_multiply<skill_id>( jo, "skills", "value",
                                     skill_values_add, skill_values_multiply );

    load_add_and_multiply<bodypart_str_id>( jo, "encumbrance_modifier", "part",
                                            encumbrance_values_add, encumbrance_values_multiply );

    load_add_and_multiply<damage_type_id>( jo, "melee_damage_bonus", "type",
                                           damage_values_add, damage_values_multiply );

    load_add_and_multiply<damage_type_id>( jo, "incoming_damage_mod", "type",
                                           armor_values_add, armor_values_multiply );

    load_add_and_multiply<damage_type_id>( jo, "incoming_damage_mod_post_absorbed", "type",
                                           extra_damage_add, extra_damage_multiply );

    if( jo.has_array( "special_vision" ) ) {
        for( const JsonObject vision_obj : jo.get_array( "special_vision" ) ) {
            special_vision _vision;
            special_vision_descriptions _desc;
            if( vision_obj.has_array( "descriptions" ) ) {
                for( const JsonObject descriptions_obj : vision_obj.get_array( "descriptions" ) ) {
                    mandatory( descriptions_obj, was_loaded, "id", _desc.id );
                    if( descriptions_obj.has_string( "color" ) ) {
                        _desc.color = color_from_string( descriptions_obj.get_string( "color" ) );
                    }
                    optional( descriptions_obj, was_loaded, "symbol", _desc.symbol );
                    mandatory( descriptions_obj, was_loaded, "text", _desc.description );
                    read_condition( descriptions_obj, "text_condition", _desc.condition, true );
                    _vision.special_vision_descriptions_vector.emplace_back( _desc );
                }
            }

            _vision.range = vision_obj.get_float( "distance", 0.0 );
            read_condition( vision_obj, "condition", _vision.condition, true );
            optional( vision_obj, was_loaded, "precise", _vision.precise );
            optional( vision_obj, was_loaded, "ignores_aiming_cone", _vision.ignores_aiming_cone );
            special_vision_vector.emplace_back( _vision );
        }
    }
}

void enchant_cache::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "has", io::enum_to_string<enchantment::has>( active_conditions.first ) );
    jsout.member( "condition", io::enum_to_string<enchantment::condition>( active_conditions.second ) );
    if( emitter ) {
        jsout.member( "emitter", emitter );
    }

    if( !hit_you_effect.empty() ) {
        jsout.member( "hit_you_effect", hit_you_effect );
    }

    if( !hit_me_effect.empty() ) {
        jsout.member( "hit_me_effect", hit_me_effect );
    }

    if( !intermittent_activation.empty() ) {
        jsout.member( "intermittent_activation" );
        jsout.start_object();
        jsout.member( "effects" );
        jsout.start_array();
        for( const std::pair<const time_duration, std::vector<fake_spell>> &pair :
             intermittent_activation ) {
            jsout.start_object();
            jsout.member( "frequency", to_string_writable( pair.first ) );
            jsout.member( "spell_effects", pair.second );
            jsout.end_object();
        }
        jsout.end_array();
        jsout.end_object();
    }

    if( !ench_effects.empty() ) {
        jsout.member( "ench_effects" );
        jsout.start_array();
        for( const std::pair<const efftype_id, int> &eff : ench_effects ) {
            jsout.start_object();
            jsout.member( "effect", eff.first );
            jsout.member( "intensity", eff.second );
            jsout.end_object();
        }
        jsout.end_array();
    }

    jsout.member( "modified_bodyparts", modified_bodyparts );
    jsout.member( "mutations", mutations );

    jsout.member( "values" );
    jsout.start_array();
    for( int value = 0; value < static_cast<int>( enchant_vals::mod::NUM_MOD ); value++ ) {
        enchant_vals::mod enum_value = static_cast<enchant_vals::mod>( value );
        jsout.start_object();
        jsout.member( "value", io::enum_to_string<enchant_vals::mod>( enum_value ) );
        if( get_value_add( enum_value ) != 0 ) {
            jsout.member( "add", get_value_add( enum_value ) );
        }
        if( get_value_multiply( enum_value ) != 0 ) {
            jsout.member( "multiply", get_value_multiply( enum_value ) );
        }
        jsout.end_object();
    }
    jsout.end_array();

    jsout.member( "skills" );
    jsout.start_array();
    const auto skill_f = []( const Skill & lhs, const Skill & rhs ) {
        return lhs.ident() < rhs.ident();
    };
    for( const Skill *sk : Skill::get_skills_sorted_by( skill_f ) ) {
        skill_id skid = sk->ident();
        jsout.start_object();
        jsout.member( "value", skid );
        if( get_skill_value_add( skid ) != 0 ) {
            jsout.member( "add", get_skill_value_add( skid ) );
        }
        if( get_skill_value_multiply( skid ) != 0 ) {
            jsout.member( "multiply", get_skill_value_multiply( skid ) );
        }
        jsout.end_object();
    }
    jsout.end_array();

    jsout.member( "melee_damage_bonus" );
    jsout.start_array();
    for( const damage_type &dt : damage_type::get_all() ) {
        jsout.start_object();
        jsout.member( "type", dt.id );
        if( get_damage_add( dt.id ) != 0 ) {
            jsout.member( "add", get_damage_add( dt.id ) );
        }
        if( get_damage_multiply( dt.id ) != 0 ) {
            jsout.member( "multiply", get_damage_multiply( dt.id ) );
        }
        jsout.end_object();
    }
    jsout.end_array();

    jsout.member( "encumbrance_modifier" );
    jsout.start_array();
    for( const body_part_type &bt : body_part_type::get_all() ) {
        jsout.start_object();
        jsout.member( "part", bt.id );
        if( get_encumbrance_add( bt.id ) != 0 ) {
            jsout.member( "add", get_encumbrance_add( bt.id ) );
        }
        if( get_encumbrance_multiply( bt.id ) != 0 ) {
            jsout.member( "multiply", get_encumbrance_multiply( bt.id ) );
        }
        jsout.end_object();
    }
    jsout.end_array();

    jsout.member( "incoming_damage_mod" );
    jsout.start_array();
    for( const damage_type &dt : damage_type::get_all() ) {
        jsout.start_object();
        jsout.member( "type", dt.id );
        if( get_armor_add( dt.id ) != 0 ) {
            jsout.member( "add", get_armor_add( dt.id ) );
        }
        if( get_armor_multiply( dt.id ) != 0 ) {
            jsout.member( "multiply", get_armor_multiply( dt.id ) );
        }
        jsout.end_object();
    }
    jsout.end_array();

    jsout.member( "incoming_damage_mod_post_absorbed" );
    jsout.start_array();
    for( const damage_type &dt : damage_type::get_all() ) {
        jsout.start_object();
        jsout.member( "type", dt.id );
        if( get_extra_damage_add( dt.id ) != 0 ) {
            jsout.member( "add", get_extra_damage_add( dt.id ) );
        }
        if( get_extra_damage_multiply( dt.id ) != 0 ) {
            jsout.member( "multiply", get_extra_damage_multiply( dt.id ) );
        }
        jsout.end_object();
    }
    jsout.end_array();

    jsout.member( "special_vision" );
    jsout.start_array();
    for( const special_vision &struc : special_vision_vector ) {
        jsout.start_object();
        jsout.member( "distance", struc.range );
        jsout.member( "descriptions" );
        jsout.start_array();
        for( const special_vision_descriptions &struc_desc : struc.special_vision_descriptions_vector ) {
            jsout.start_object();
            jsout.member( "id", struc_desc.id );
            jsout.end_object();
        }
        jsout.end_array();
        jsout.end_object();
    }
    jsout.end_array();

    jsout.end_object();
}

bool enchant_cache::stacks_with( const enchantment &rhs ) const
{
    return active_conditions == rhs.active_conditions;
}

bool enchant_cache::add( const enchantment &rhs, Character &you )
{
    if( !stacks_with( rhs ) ) {
        return false;
    }
    force_add( rhs, you );
    return true;
}

bool enchant_cache::add( const enchant_cache &rhs )
{
    if( !stacks_with( rhs ) ) {
        return false;
    }
    force_add( rhs );
    return true;
}

void enchant_cache::force_add_mutation( const enchantment &rhs )
{
    for( const trait_id &branch : rhs.mutations ) {
        mutations.push_back( branch );
    }
}


void enchant_cache::force_add( const enchant_cache &rhs )
{
    for( const std::pair<const enchant_vals::mod, double> &pair_values :
         rhs.values_add ) {
        values_add[pair_values.first] += pair_values.second;
    }
    for( const std::pair<const enchant_vals::mod, double> &pair_values : rhs.values_multiply ) {
        // values do not multiply against each other, they add.
        // so +10% and -10% will add to 0%
        values_multiply[pair_values.first] += pair_values.second;
    }

    for( const std::pair<const skill_id, double> &pair_values :
         rhs.skill_values_add ) {
        skill_values_add[pair_values.first] += pair_values.second;
    }
    for( const std::pair<const skill_id, double> &pair_values :
         rhs.skill_values_multiply ) {
        // values do not multiply against each other, they add.
        // so +10% and -10% will add to 0%
        skill_values_multiply[pair_values.first] += pair_values.second;
    }

    for( const std::pair<const damage_type_id, double> &pair_values : rhs.damage_values_add ) {
        damage_values_add[pair_values.first] += pair_values.second;
    }
    for( const std::pair<const damage_type_id, double> &pair_values :
         rhs.damage_values_multiply ) {
        // values do not multiply against each other, they add.
        // so +10% and -10% will add to 0%
        damage_values_multiply[pair_values.first] += pair_values.second;
    }

    for( const std::pair<const bodypart_str_id, double> &pair_values : rhs.encumbrance_values_add ) {
        encumbrance_values_add[pair_values.first] += pair_values.second;
    }
    for( const std::pair<const bodypart_str_id, double> &pair_values :
         rhs.encumbrance_values_multiply ) {
        // values do not multiply against each other, they add.
        // so +10% and -10% will add to 0%
        encumbrance_values_multiply[pair_values.first] += pair_values.second;
    }

    for( const std::pair<const damage_type_id, double> &pair_values : rhs.armor_values_add ) {
        armor_values_add[pair_values.first] += pair_values.second;
    }
    for( const std::pair<const damage_type_id, double> &pair_values :
         rhs.armor_values_multiply ) {
        // values do not multiply against each other, they add.
        // so +10% and -10% will add to 0%
        armor_values_multiply[pair_values.first] += pair_values.second;
    }

    for( const std::pair<const damage_type_id, double> &pair_values : rhs.extra_damage_add ) {
        extra_damage_add[pair_values.first] += pair_values.second;
    }
    for( const std::pair<const damage_type_id, double> &pair_values :
         rhs.extra_damage_multiply ) {
        // values do not multiply against each other, they add.
        // so +10% and -10% will add to 0%
        extra_damage_multiply[pair_values.first] += pair_values.second;
    }
    // from cache to cache?
    for( const special_vision &struc : rhs.special_vision_vector ) {
        special_vision_vector.emplace_back( special_vision{
            struc.special_vision_descriptions_vector, struc.condition, struc.range,
            struc.precise, struc.ignores_aiming_cone } );
    }

    hit_me_effect.insert( hit_me_effect.end(), rhs.hit_me_effect.begin(), rhs.hit_me_effect.end() );

    hit_you_effect.insert( hit_you_effect.end(), rhs.hit_you_effect.begin(), rhs.hit_you_effect.end() );

    ench_effects.insert( rhs.ench_effects.begin(), rhs.ench_effects.end() );

    if( rhs.emitter ) {
        emitter = rhs.emitter;
    }

    for( const bodypart_changes &bp : rhs.modified_bodyparts ) {
        modified_bodyparts.emplace_back( bp );
    }

    for( const trait_id &branch : rhs.mutations ) {
        mutations.push_back( branch );
    }

    for( const std::pair<const time_duration, std::vector<fake_spell>> &act_pair :
         rhs.intermittent_activation ) {
        for( const fake_spell &fake : act_pair.second ) {
            intermittent_activation[act_pair.first].emplace_back( fake );
        }
    }
}

void enchant_cache::force_add( const enchantment &rhs, const Character &guy )
{
    const_dialogue d( get_const_talker_for( guy ), nullptr );
    force_add_with_dialogue( rhs, d );
}

void enchant_cache::force_add( const enchantment &rhs, const monster &mon )
{
    const_dialogue d( get_const_talker_for( mon ), nullptr );
    force_add_with_dialogue( rhs, d );
}

void enchant_cache::force_add( const enchantment &rhs )
{
    const_dialogue d( nullptr, nullptr );
    force_add_with_dialogue( rhs, d, false );
}

void enchant_cache::force_add_with_dialogue( const enchantment &rhs, const const_dialogue &d,
        const bool evaluate )
{
    for( const std::pair<const enchant_vals::mod, dbl_or_var> &pair_values :
         rhs.values_add ) {
        if( evaluate ) {
            values_add[pair_values.first] += pair_values.second.evaluate( d );
        } else {
            values_add[pair_values.first] += pair_values.second.constant();
        }
    }
    for( const std::pair<const enchant_vals::mod, dbl_or_var> &pair_values :
         rhs.values_multiply ) {
        // values do not multiply against each other, they add.
        // so +10% and -10% will add to 0%
        if( evaluate ) {
            values_multiply[pair_values.first] += pair_values.second.evaluate( d );
        } else {
            values_multiply[pair_values.first] += pair_values.second.constant();
        }
    }

    for( const std::pair<const skill_id, dbl_or_var> &pair_values :
         rhs.skill_values_add ) {
        if( evaluate ) {
            skill_values_add[pair_values.first] += pair_values.second.evaluate( d );
        } else {
            skill_values_add[pair_values.first] += pair_values.second.constant();
        }
    }
    for( const std::pair<const skill_id, dbl_or_var> &pair_values :
         rhs.skill_values_multiply ) {
        // values do not multiply against each other, they add.
        // so +10% and -10% will add to 0%
        if( evaluate ) {
            skill_values_multiply[pair_values.first] += pair_values.second.evaluate( d );
        } else {
            skill_values_multiply[pair_values.first] += pair_values.second.constant();
        }
    }

    for( const std::pair<const bodypart_str_id, dbl_or_var> &pair_values :
         rhs.encumbrance_values_add ) {
        if( evaluate ) {
            encumbrance_values_add[pair_values.first] += pair_values.second.evaluate( d );
        } else {
            encumbrance_values_add[pair_values.first] += pair_values.second.constant();
        }
    }
    for( const std::pair<const bodypart_str_id, dbl_or_var> &pair_values :
         rhs.encumbrance_values_multiply ) {
        if( evaluate ) {
            encumbrance_values_multiply[pair_values.first] += pair_values.second.evaluate( d );
        } else {
            encumbrance_values_multiply[pair_values.first] += pair_values.second.constant();
        }
    }

    for( const std::pair<const damage_type_id, dbl_or_var> &pair_values :
         rhs.damage_values_add ) {
        if( evaluate ) {
            damage_values_add[pair_values.first] += pair_values.second.evaluate( d );
        } else {
            damage_values_add[pair_values.first] += pair_values.second.constant();
        }
    }
    for( const std::pair<const damage_type_id, dbl_or_var> &pair_values :
         rhs.damage_values_multiply ) {
        if( evaluate ) {
            damage_values_multiply[pair_values.first] += pair_values.second.evaluate( d );
        } else {
            damage_values_multiply[pair_values.first] += pair_values.second.constant();
        }
    }

    for( const std::pair<const damage_type_id, dbl_or_var> &pair_values :
         rhs.armor_values_add ) {
        if( evaluate ) {
            armor_values_add[pair_values.first] += pair_values.second.evaluate( d );
        } else {
            armor_values_add[pair_values.first] += pair_values.second.constant();
        }
    }
    for( const std::pair<const damage_type_id, dbl_or_var> &pair_values :
         rhs.armor_values_multiply ) {
        if( evaluate ) {
            armor_values_multiply[pair_values.first] += pair_values.second.evaluate( d );
        } else {
            armor_values_multiply[pair_values.first] += pair_values.second.constant();
        }
    }

    for( const std::pair<const damage_type_id, dbl_or_var> &pair_values :
         rhs.extra_damage_add ) {
        if( evaluate ) {
            extra_damage_add[pair_values.first] += pair_values.second.evaluate( d );
        } else {
            extra_damage_add[pair_values.first] += pair_values.second.constant();
        }
    }
    for( const std::pair<const damage_type_id, dbl_or_var> &pair_values :
         rhs.extra_damage_multiply ) {
        if( evaluate ) {
            extra_damage_multiply[pair_values.first] += pair_values.second.evaluate( d );
        } else {
            extra_damage_multiply[pair_values.first] += pair_values.second.constant();
        }
    }
    for( const enchantment::special_vision &struc : rhs.special_vision_vector ) {
        if( evaluate ) {
            special_vision_vector.emplace_back( special_vision{
                struc.special_vision_descriptions_vector, struc.condition, struc.range.evaluate( d ),
                struc.precise, struc.ignores_aiming_cone } );
        } else {
            special_vision_vector.emplace_back( special_vision{
                struc.special_vision_descriptions_vector, struc.condition, struc.range.constant(),
                struc.precise, struc.ignores_aiming_cone } );
        }
    }

    hit_me_effect.insert( hit_me_effect.end(), rhs.hit_me_effect.begin(), rhs.hit_me_effect.end() );

    hit_you_effect.insert( hit_you_effect.end(), rhs.hit_you_effect.begin(), rhs.hit_you_effect.end() );

    ench_effects.insert( rhs.ench_effects.begin(), rhs.ench_effects.end() );

    if( rhs.emitter ) {
        emitter = rhs.emitter;
    }

    for( const bodypart_changes &bp : rhs.modified_bodyparts ) {
        modified_bodyparts.emplace_back( bp );
    }

    for( const trait_id &branch : rhs.mutations ) {
        mutations.push_back( branch );
    }

    for( const std::pair<const time_duration, std::vector<fake_spell>> &act_pair :
         rhs.intermittent_activation ) {
        for( const fake_spell &fake : act_pair.second ) {
            intermittent_activation[act_pair.first].emplace_back( fake );
        }
    }

    details.emplace_back( rhs.name.translated(), rhs.description.translated() );
}

void enchant_cache::set_has( enchantment::has value )
{
    active_conditions.first = value;
}

void enchant_cache::add_value_add( enchant_vals::mod value, int add_value )
{
    values_add[value] = add_value;
}

void enchant_cache::add_value_mult( enchant_vals::mod value, float mult_value )
{
    values_multiply[value] = mult_value;
}

void enchant_cache::add_hit_me( const fake_spell &sp )
{
    hit_me_effect.push_back( sp );
}

void enchant_cache::add_hit_you( const fake_spell &sp )
{
    hit_you_effect.push_back( sp );
}

double enchantment::get_value_add( const enchant_vals::mod value, const Character &guy ) const
{
    const auto found = values_add.find( value );
    if( found == values_add.cend() ) {
        return 0;
    }
    const_dialogue d( get_const_talker_for( guy ), nullptr );
    return found->second.evaluate( d );
}

double enchantment::get_value_multiply( const enchant_vals::mod value, const Character &guy ) const
{
    const auto found = values_multiply.find( value );
    if( found == values_multiply.cend() ) {
        return 0;
    }
    const_dialogue d( get_const_talker_for( guy ), nullptr );
    return found->second.evaluate( d );
}

enchantment::special_vision enchantment::get_vision( const const_dialogue &d ) const
{
    // Maybe it's a bad idea to make it part of enchantment code
    // maybe it should be it's own instance of json
    // and enchantment code simply adds values to this instance?
    // `"special_vision" { "id": "nether_vision", "bonus": 20 }`

    if( special_vision_vector.empty() ) {
        return {};
    }

    const double distance = rl_dist_exact( d.const_actor( true )->pos_abs(),
                                           d.const_actor( false )->pos_abs() );

    // first iterate over structs that has ignores_aiming_cone true
    // to prevent cata_tiles::draw_critter_at() from picking texture
    // that cannot be rendered because it's behind you while you are aiming
    for( const special_vision &struc : special_vision_vector ) {
        if( struc.ignores_aiming_cone && struc.range.evaluate( d ) >= distance &&  struc.condition( d ) ) {
            return struc;
        }
    }

    for( const special_vision &struc : special_vision_vector ) {
        if( !struc.ignores_aiming_cone && struc.range.evaluate( d ) >= distance &&  struc.condition( d ) ) {
            return struc;
        }
    }

    return {};
}

bool enchantment::get_vision_can_see( const enchantment::special_vision &vision_struct,
                                      const_dialogue &d ) const
{
    return !vision_struct.is_empty( d );
}

enchantment::special_vision_descriptions enchantment::get_vision_description_struct(
    const enchantment::special_vision &vision_struct, const_dialogue &d ) const
{
    if( vision_struct.is_empty( d ) ) {
        return {};
    }

    for( const enchantment::special_vision_descriptions &desc :
         vision_struct.special_vision_descriptions_vector ) {
        if( desc.condition( d ) ) {
            return desc;
        }
    }
    return {};
}

double enchant_cache::get_value_add( const enchant_vals::mod value ) const
{
    const auto found = values_add.find( value );
    if( found == values_add.cend() ) {
        return 0;
    }
    return found->second;
}

double enchant_cache::get_skill_value_add( const skill_id &value ) const
{
    const auto found = skill_values_add.find( value );
    if( found == skill_values_add.cend() ) {
        return 0;
    }
    return found->second;
}

int enchant_cache::get_damage_add( const damage_type_id &value ) const
{
    const auto found = damage_values_add.find( value );
    if( found == damage_values_add.cend() ) {
        return 0;
    }
    return found->second;
}

int enchant_cache::get_encumbrance_add( const bodypart_str_id &value ) const
{
    const auto found = encumbrance_values_add.find( value );
    if( found == encumbrance_values_add.cend() ) {
        return 0;
    }
    return found->second;
}

int enchant_cache::get_armor_add( const damage_type_id &value ) const
{
    const auto found = armor_values_add.find( value );
    if( found == armor_values_add.cend() ) {
        return 0;
    }
    return found->second;
}

int enchant_cache::get_extra_damage_add( const damage_type_id &value ) const
{
    const auto found = extra_damage_add.find( value );
    if( found == extra_damage_add.cend() ) {
        return 0;
    }
    return found->second;
}

double enchant_cache::get_value_multiply( const enchant_vals::mod value ) const
{
    const auto found = values_multiply.find( value );
    if( found == values_multiply.cend() ) {
        return 0;
    }
    return found->second;
}

enchant_cache::special_vision enchant_cache::get_vision( const const_dialogue &d ) const
{
    if( special_vision_vector.empty() ) {
        return {};
    }

    const double distance = rl_dist_exact( d.const_actor( true )->pos_abs(),
                                           d.const_actor( false )->pos_abs() );

    for( const special_vision &struc : special_vision_vector ) {
        if( struc.ignores_aiming_cone && struc.range >= distance && struc.condition( d ) ) {
            return struc;
        }
    }

    for( const special_vision &struc : special_vision_vector ) {
        if( !struc.ignores_aiming_cone && struc.range >= distance && struc.condition( d ) ) {
            return struc;
        }
    }

    return {};
}

bool enchant_cache::get_vision_can_see( const enchant_cache::special_vision &vision_struct ) const
{
    return !vision_struct.is_empty();
}

enchant_cache::special_vision_descriptions enchant_cache::get_vision_description_struct(
    const enchant_cache::special_vision &vision_struct, const_dialogue &d ) const
{
    if( vision_struct.is_empty() ) {
        return {};
    }

    for( const enchant_cache::special_vision_descriptions &desc :
         vision_struct.special_vision_descriptions_vector ) {
        if( desc.condition( d ) ) {
            return desc;
        }
    }
    return {};
}

double enchant_cache::get_skill_value_multiply( const skill_id &value ) const
{
    const auto found = skill_values_multiply.find( value );
    if( found == skill_values_multiply.cend() ) {
        return 0;
    }
    return found->second;
}

double enchant_cache::get_damage_multiply( const damage_type_id &value ) const
{
    const auto found = damage_values_multiply.find( value );
    if( found == damage_values_multiply.cend() ) {
        return 0;
    }
    return found->second;
}

double enchant_cache::get_armor_multiply( const damage_type_id &value ) const
{
    const auto found = armor_values_multiply.find( value );
    if( found == armor_values_multiply.cend() ) {
        return 0;
    }
    return found->second;
}

double enchant_cache::get_encumbrance_multiply( const bodypart_str_id &value ) const
{
    const auto found = encumbrance_values_multiply.find( value );
    if( found == encumbrance_values_multiply.cend() ) {
        return 0;
    }
    return found->second;
}

double enchant_cache::get_extra_damage_multiply( const damage_type_id &value ) const
{
    const auto found = extra_damage_multiply.find( value );
    if( found == extra_damage_multiply.cend() ) {
        return 0;
    }
    return found->second;
}

double enchant_cache::modify_value( const enchant_vals::mod mod_val, double value ) const
{
    value += get_value_add( mod_val );
    value *= 1.0 + get_value_multiply( mod_val );
    return value;
}

double enchant_cache::modify_value( const skill_id &mod_val, double value ) const
{
    value += get_skill_value_add( mod_val );
    value *= 1.0 + get_skill_value_multiply( mod_val );
    return value;
}

units::energy enchant_cache::modify_value( const enchant_vals::mod mod_val,
        units::energy value ) const
{
    value += units::from_millijoule<double>( get_value_add( mod_val ) );
    value *= 1.0 + get_value_multiply( mod_val );
    return value;
}

units::mass enchant_cache::modify_value( const enchant_vals::mod mod_val,
        units::mass value ) const
{
    value += units::from_gram<double>( get_value_add( mod_val ) );
    value *= 1.0 + get_value_multiply( mod_val );
    return value;
}

units::volume enchant_cache::modify_value( const enchant_vals::mod mod_val,
        units::volume value ) const
{
    value += units::from_milliliter<double>( get_value_add( mod_val ) );
    value *= 1.0 + get_value_multiply( mod_val );
    return value;
}

units::temperature_delta enchant_cache::modify_value( const enchant_vals::mod mod_val,
        units::temperature_delta value ) const
{
    value += units::from_celsius_delta<double>( get_value_add( mod_val ) );
    value *= 1 + get_value_multiply( mod_val );
    return value;
}

time_duration enchant_cache::modify_value( const enchant_vals::mod mod_val,
        time_duration value ) const
{
    value += time_duration::from_seconds<double>( get_value_add( mod_val ) );
    value *= 1.0 + get_value_multiply( mod_val );
    return value;
}

double enchant_cache::modify_melee_damage( const damage_type_id &mod_val, double value ) const
{
    value += get_damage_add( mod_val );
    value *= 1.0 + get_damage_multiply( mod_val );
    return value;
}

double enchant_cache::modify_encumbrance( const bodypart_str_id &mod_val, double value ) const
{
    value += get_encumbrance_add( mod_val );
    value *= 1.0 + get_encumbrance_multiply( mod_val );
    return value;
}

double enchant_cache::modify_damage_units_by_armor_protection( const damage_type_id &mod_val,
        double value ) const
{
    value += get_armor_add( mod_val );
    value *= 1.0 + get_armor_multiply( mod_val );
    return value;
}

double enchant_cache::modify_damage_units_by_extra_damage( const damage_type_id &mod_val,
        double value ) const
{
    value += get_extra_damage_add( mod_val );
    value *= 1.0 + get_extra_damage_multiply( mod_val );
    return value;
}

int enchant_cache::mult_bonus( enchant_vals::mod value_type, int base_value ) const
{
    return get_value_multiply( value_type ) * base_value;
}

int enchant_cache::skill_mult_bonus( const skill_id &value_type, int base_value ) const
{
    return get_skill_value_multiply( value_type ) * base_value;
}

bool enchantment::modifies_bodyparts() const
{
    return !modified_bodyparts.empty();
}

body_part_set enchantment::modify_bodyparts( const body_part_set &unmodified ) const
{
    body_part_set modified( unmodified );
    for( const bodypart_changes &changes : modified_bodyparts ) {
        if( !changes.gain.is_empty() ) {
            modified.set( changes.gain );
        }
        if( !changes.lose.is_empty() ) {
            modified.reset( changes.lose );
        }
    }
    return modified;
}

void enchant_cache::activate_passive( Character &guy ) const
{
    // since it's a modifier, need to remove original value from the math
    guy.mod_str_bonus( modify_value( enchant_vals::mod::STRENGTH,
                                     guy.get_str_base() ) - guy.get_str_base() );

    guy.mod_dex_bonus( modify_value( enchant_vals::mod::DEXTERITY,
                                     guy.get_dex_base() ) - guy.get_dex_base() );

    guy.mod_per_bonus( modify_value( enchant_vals::mod::PERCEPTION,
                                     guy.get_per_base() ) - guy.get_per_base() );

    guy.mod_int_bonus( modify_value( enchant_vals::mod::INTELLIGENCE,
                                     guy.get_int_base() ) - guy.get_int_base() );

    guy.mod_num_dodges_bonus( modify_value( enchant_vals::mod::BONUS_DODGE,
                                            guy.get_num_dodges_base() ) - guy.get_num_dodges_base() );

    guy.mod_num_blocks_bonus( modify_value( enchant_vals::mod::BONUS_BLOCK,
                                            guy.get_num_blocks_base() ) - guy.get_num_blocks_base() );

    if( emitter ) {
        get_map().emit_field( guy.pos_bub(), *emitter );
    }
    for( const std::pair<const efftype_id, int> &eff : ench_effects ) {
        guy.add_effect( eff.first, 1_seconds, false, eff.second );
    }
    for( const std::pair<const time_duration, std::vector<fake_spell>> &activation :
         intermittent_activation ) {
        // a random approximation!
        if( one_in( to_seconds<int>( activation.first ) ) ) {
            for( const fake_spell &fake : activation.second ) {
                fake.get_spell( guy, 0 ).cast_all_effects( guy, guy.pos_bub() );
            }
        }
    }
}

void enchant_cache::cast_hit_you( Creature &caster, const Creature &target ) const
{
    for( const fake_spell &sp : hit_you_effect ) {
        cast_enchantment_spell( caster, &target, sp );
    }
}

void enchant_cache::cast_hit_me( Creature &caster, const Creature *target ) const
{
    for( const fake_spell &sp : hit_me_effect ) {
        cast_enchantment_spell( caster, target, sp );
    }
}

void enchant_cache::cast_hit_you( Character &caster, const Creature &target ) const
{
    for( const fake_spell &sp : hit_you_effect ) {
        cast_enchantment_spell( caster, &target, sp );
    }
}

void enchant_cache::cast_hit_me( Character &caster, const Creature *target ) const
{
    for( const fake_spell &sp : hit_me_effect ) {
        cast_enchantment_spell( caster, target, sp );
    }
}

void enchant_cache::cast_enchantment_spell( Creature &caster, const Creature *target,
        const fake_spell &sp ) const
{
    // check the chances
    if( !one_in( sp.trigger_once_in ) ) {
        return;
    }

    if( sp.self ) {
        caster.add_msg_player_or_npc( m_good,
                                      sp.trigger_message,
                                      sp.npc_trigger_message,
                                      caster.get_name() );
        sp.get_spell( caster, sp.level ).cast_all_effects( caster, caster.pos_bub() );
    } else  if( target != nullptr ) {
        const Creature &trg_crtr = *target;
        const spell &spell_lvl = sp.get_spell( caster, sp.level );
        if( !spell_lvl.is_valid_target( caster, trg_crtr.pos_bub() ) ||
            !spell_lvl.is_target_in_range( caster, trg_crtr.pos_bub() ) ) {
            return;
        }

        caster.add_msg_player_or_npc( m_good,
                                      sp.trigger_message,
                                      sp.npc_trigger_message,
                                      caster.get_name(), trg_crtr.disp_name() );

        spell_lvl.cast_all_effects( caster, trg_crtr.pos_bub() );
    }
}

void enchant_cache::clear()
{
    //I'm trusting all of these vectors and maps to have clear functions that avoid memory leaks.
    //Fingers crossed!
    values_add.clear();
    values_multiply.clear();
    skill_values_add.clear();
    skill_values_multiply.clear();
    damage_values_add.clear();
    damage_values_multiply.clear();
    armor_values_add.clear();
    armor_values_multiply.clear();
    encumbrance_values_add.clear();
    encumbrance_values_multiply.clear();
    extra_damage_add.clear();
    extra_damage_multiply.clear();
    special_vision_vector.clear();
    hit_me_effect.clear();
    hit_you_effect.clear();
    ench_effects.clear();
    emitter.reset();
    mutations.clear();
    modified_bodyparts.clear();
    intermittent_activation.clear();
    details.clear();
}

bool enchant_cache::operator==( const enchant_cache &rhs ) const
{
    if( this->values_add.size() != rhs.values_add.size() ||
        this->values_multiply.size() != rhs.values_multiply.size() ) {
        return false;
    }
    auto iter_add = this->values_add.cbegin();
    auto iter_add2 = rhs.values_add.cbegin();
    while( iter_add != this->values_add.cend() && iter_add2 != rhs.values_add.cend() ) {
        if( iter_add->second != iter_add2->second || iter_add->first != iter_add2->first ) {
            return false;
        }
        iter_add++;
        iter_add2++;
    }
    auto iter_mult = this->values_multiply.cbegin();
    auto iter_mult2 = rhs.values_multiply.cbegin();
    while( iter_mult != this->values_multiply.cend() && iter_mult2 != rhs.values_multiply.cend() ) {
        if( iter_mult->second != iter_mult2->second || iter_mult->first != iter_mult2->first ) {
            return false;
        }
        iter_mult++;
        iter_mult2++;
    }
    return this->id == rhs.id &&
           this->get_mutations() == rhs.get_mutations();
}
