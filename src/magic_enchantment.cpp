#include "magic_enchantment.h"

#include <cstdlib>
#include <set>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "condition.h"
#include "creature.h"
#include "debug.h"
#include "dialogue_helpers.h"
#include "enum_conversions.h"
#include "enums.h"
#include "generic_factory.h"
#include "item.h"
#include "json.h"
#include "map.h"
#include "point.h"
#include "rng.h"
#include "skill.h"
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
            case enchant_vals::mod::MAX_STAMINA: return "MAX_STAMINA";
            case enchant_vals::mod::REGEN_STAMINA: return "REGEN_STAMINA";
            case enchant_vals::mod::MAX_HP: return "MAX_HP";
            case enchant_vals::mod::REGEN_HP: return "REGEN_HP";
            case enchant_vals::mod::HUNGER: return "HUNGER";
            case enchant_vals::mod::THIRST: return "THIRST";
            case enchant_vals::mod::FATIGUE: return "FATIGUE";
            case enchant_vals::mod::PAIN: return "PAIN";
            case enchant_vals::mod::PAIN_REMOVE: return "PAIN_REMOVE";
            case enchant_vals::mod::MELEE_DAMAGE: return "MELEE_DAMAGE";
            case enchant_vals::mod::BONUS_BLOCK: return "BONUS_BLOCK";
            case enchant_vals::mod::BONUS_DODGE: return "BONUS_DODGE";
            case enchant_vals::mod::ATTACK_NOISE: return "ATTACK_NOISE";
            case enchant_vals::mod::SHOUT_NOISE: return "SHOUT_NOISE";
            case enchant_vals::mod::FOOTSTEP_NOISE: return "FOOTSTEP_NOISE";
            case enchant_vals::mod::SIGHT_RANGE_ELECTRIC: return "SIGHT_RANGE_ELECTRIC";
            case enchant_vals::mod::MOTION_VISION_RANGE: return "MOTION_VISION_RANGE";
            case enchant_vals::mod::CARRY_WEIGHT: return "CARRY_WEIGHT";
            case enchant_vals::mod::WEAPON_DISPERSION: return "WEAPON_DISPERSION";
            case enchant_vals::mod::SOCIAL_LIE: return "SOCIAL_LIE";
            case enchant_vals::mod::SOCIAL_PERSUADE: return "SOCIAL_PERSUADE";
            case enchant_vals::mod::SOCIAL_INTIMIDATE: return "SOCIAL_INTIMIDATE";
            case enchant_vals::mod::SLEEPY: return "SLEEPY";
            case enchant_vals::mod::LUMINATION: return "LUMINATION";
            case enchant_vals::mod::EFFECTIVE_HEALTH_MOD: return "EFFECTIVE_HEALTH_MOD";
            case enchant_vals::mod::MOD_HEALTH: return "MOD_HEALTH";
            case enchant_vals::mod::MOD_HEALTH_CAP: return "MOD_HEALTH_CAP";
            case enchant_vals::mod::READING_EXP: return "READING_EXP";
            case enchant_vals::mod::SKILL_RUST_RESIST: return "SKILL_RUST_RESIST";
            case enchant_vals::mod::LEARNING_FOCUS: return "LEARNING_FOCUS";
            case enchant_vals::mod::RECOIL_MODIFIER: return "RECOIL_MODIFIER";
            case enchant_vals::mod::ARMOR_ACID: return "ARMOR_ACID";
            case enchant_vals::mod::ARMOR_BASH: return "ARMOR_BASH";
            case enchant_vals::mod::ARMOR_BIO: return "ARMOR_BIO";
            case enchant_vals::mod::ARMOR_COLD: return "ARMOR_COLD";
            case enchant_vals::mod::ARMOR_CUT: return "ARMOR_CUT";
            case enchant_vals::mod::ARMOR_ELEC: return "ARMOR_ELEC";
            case enchant_vals::mod::ARMOR_HEAT: return "ARMOR_HEAT";
            case enchant_vals::mod::ARMOR_STAB: return "ARMOR_STAB";
            case enchant_vals::mod::ARMOR_BULLET: return "ARMOR_BULLET";
            case enchant_vals::mod::EXTRA_BASH: return "EXTRA_BASH";
            case enchant_vals::mod::EXTRA_CUT: return "EXTRA_CUT";
            case enchant_vals::mod::EXTRA_STAB: return "EXTRA_STAB";
            case enchant_vals::mod::EXTRA_BULLET: return "EXTRA_BULLET";
            case enchant_vals::mod::EXTRA_HEAT: return "EXTRA_HEAT";
            case enchant_vals::mod::EXTRA_COLD: return "EXTRA_COLD";
            case enchant_vals::mod::EXTRA_ELEC: return "EXTRA_ELEC";
            case enchant_vals::mod::EXTRA_ACID: return "EXTRA_ACID";
            case enchant_vals::mod::EXTRA_BIO: return "EXTRA_BIO";
            case enchant_vals::mod::EXTRA_ELEC_PAIN: return "EXTRA_ELEC_PAIN";
            case enchant_vals::mod::ITEM_DAMAGE_PURE: return "ITEM_DAMAGE_PURE";
            case enchant_vals::mod::ITEM_DAMAGE_BASH: return "ITEM_DAMAGE_BASH";
            case enchant_vals::mod::ITEM_DAMAGE_CUT: return "ITEM_DAMAGE_CUT";
            case enchant_vals::mod::ITEM_DAMAGE_STAB: return "ITEM_DAMAGE_STAB";
            case enchant_vals::mod::ITEM_DAMAGE_BULLET: return "ITEM_DAMAGE_BULLET";
            case enchant_vals::mod::ITEM_DAMAGE_HEAT: return "ITEM_DAMAGE_HEAT";
            case enchant_vals::mod::ITEM_DAMAGE_COLD: return "ITEM_DAMAGE_COLD";
            case enchant_vals::mod::ITEM_DAMAGE_ELEC: return "ITEM_DAMAGE_ELEC";
            case enchant_vals::mod::ITEM_DAMAGE_ACID: return "ITEM_DAMAGE_ACID";
            case enchant_vals::mod::ITEM_DAMAGE_BIO: return "ITEM_DAMAGE_BIO";
            case enchant_vals::mod::ITEM_ARMOR_BASH: return "ITEM_ARMOR_BASH";
            case enchant_vals::mod::ITEM_ARMOR_CUT: return "ITEM_ARMOR_CUT";
            case enchant_vals::mod::ITEM_ARMOR_STAB: return "ITEM_ARMOR_STAB";
            case enchant_vals::mod::ITEM_ARMOR_BULLET: return "ITEM_ARMOR_BULLET";
            case enchant_vals::mod::ITEM_ARMOR_HEAT: return "ITEM_ARMOR_HEAT";
            case enchant_vals::mod::ITEM_ARMOR_COLD: return "ITEM_ARMOR_COLD";
            case enchant_vals::mod::ITEM_ARMOR_ELEC: return "ITEM_ARMOR_ELEC";
            case enchant_vals::mod::ITEM_ARMOR_ACID: return "ITEM_ARMOR_ACID";
            case enchant_vals::mod::ITEM_ARMOR_BIO: return "ITEM_ARMOR_BIO";
            case enchant_vals::mod::ITEM_ATTACK_SPEED: return "ITEM_ATTACK_SPEED";
            case enchant_vals::mod::CLIMATE_CONTROL_HEAT: return "CLIMATE_CONTROL_HEAT";
            case enchant_vals::mod::CLIMATE_CONTROL_CHILL: return "CLIMATE_CONTROL_CHILL";
            case enchant_vals::mod::FALL_DAMAGE: return "FALL_DAMAGE";
            case enchant_vals::mod::OVERKILL_DAMAGE: return "OVERKILL_DAMAGE";
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

void enchantment::load_enchantment( const JsonObject &jo, const std::string &src )
{
    spell_factory.load( jo, src );
}

void enchantment::reset()
{
    spell_factory.reset();
}

enchantment_id enchantment::load_inline_enchantment( const JsonValue &jv, const std::string &src,
        std::string &inline_id )
{
    if( jv.test_string() ) {
        return enchantment_id( jv.get_string() );
    } else if( jv.test_object() ) {
        if( inline_id.empty() ) {
            jv.throw_error( "Inline enchantment cannot be created without an id." );
        }
        if( spell_factory.is_valid( enchantment_id( inline_id ) ) ) {
            jv.throw_error( "Inline enchantment " + inline_id +
                            " cannot be created as an enchantment already has this id." );
        }

        enchantment inline_enchant;
        inline_enchant.load( jv.get_object(), src, inline_id );
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
           ( active_conditions.first == has::WORN && guy.is_worn( parent ) ) ) ) {
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
        dialogue d( get_talker_for( guy ), nullptr );
        return dialog_condition( d );
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

void enchantment::load( const JsonObject &jo, const std::string &,
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

    if( !is_child && jo.has_array( "values" ) ) {
        for( const JsonObject value_obj : jo.get_array( "values" ) ) {
            const enchant_vals::mod value = io::string_to_enum<enchant_vals::mod>
                                            ( value_obj.get_string( "value" ) );
            dbl_or_var add = get_dbl_or_var( value_obj, "add", false );
            values_add.emplace( value, add );
            dbl_or_var mult = get_dbl_or_var( value_obj, "multiply", false );
            if( value_obj.has_member( "multiply" ) ) {
                if( value_obj.has_float( "multiply" ) ) {
                    mult.max.dbl_val = mult.min.dbl_val = value_obj.get_float( "multiply" );
                }
            }
            values_multiply.emplace( value, mult );
        }
    }

    // note: if we add another map to enchantment that looks exactly the same as this,
    // i believe it would then be the time to consider using a template to read this in
    if( !is_child && jo.has_array( "skills" ) ) {
        for( const JsonObject value_obj : jo.get_array( "skills" ) ) {
            const skill_id value = skill_id( value_obj.get_string( "value" ) );
            if( value_obj.has_member( "add" ) ) {
                dbl_or_var add = get_dbl_or_var( value_obj, "add", false );
                skill_values_add.emplace( value, add );
            }
            if( value_obj.has_member( "multiply" ) ) {
                dbl_or_var mult;
                if( value_obj.has_float( "multiply" ) ) {
                    mult.max.dbl_val = mult.min.dbl_val = value_obj.get_float( "multiply" );
                } else {
                    mult = get_dbl_or_var( value_obj, "multiply", false );

                }
                skill_values_multiply.emplace( value, mult );
            }
        }
    }
}

void enchant_cache::load( const JsonObject &jo, const std::string &,
                          const std::optional<std::string> &inline_id )
{
    enchantment::load( jo, "", inline_id, true );
    if( jo.has_array( "values" ) ) {
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
                debugmsg( "A relic attempted to load invalid enchantment %s.  If you updated versions this may be a removed enchantment and will fix itself.",
                          value_obj.get_string( "value", "" ) );
            }
        }
    }

    if( jo.has_array( "skills" ) ) {
        for( const JsonObject value_obj : jo.get_array( "skills" ) ) {
            const skill_id value = skill_id( value_obj.get_string( "value" ) );
            const int add = value_obj.get_int( "add", 0 );
            const double mult = value_obj.get_float( "multiply", 0.0 );
            if( add != 0 ) {
                skill_values_add.emplace( value, add );
            }
            if( mult != 0.0 ) {
                skill_values_multiply.emplace( value, static_cast<int>( mult ) );
            }
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

void enchant_cache::force_add( const enchant_cache &rhs )
{
    for( const std::pair<const enchant_vals::mod, int> &pair_values :
         rhs.values_add ) {
        values_add[pair_values.first] += pair_values.second;
    }
    for( const std::pair<const enchant_vals::mod, double> &pair_values : rhs.values_multiply ) {
        // values do not multiply against each other, they add.
        // so +10% and -10% will add to 0%
        values_multiply[pair_values.first] += pair_values.second;
    }

    for( const std::pair<const skill_id, int> &pair_values :
         rhs.skill_values_add ) {
        skill_values_add[pair_values.first] += pair_values.second;
    }
    for( const std::pair<const skill_id, int> &pair_values :
         rhs.skill_values_multiply ) {
        // values do not multiply against each other, they add.
        // so +10% and -10% will add to 0%
        skill_values_multiply[pair_values.first] += pair_values.second;
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
        mutations.emplace( branch );
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
    dialogue d( get_talker_for( guy ), nullptr );
    for( const std::pair<const enchant_vals::mod, dbl_or_var> &pair_values :
         rhs.values_add ) {
        values_add[pair_values.first] += pair_values.second.evaluate( d );
    }
    for( const std::pair<const enchant_vals::mod, dbl_or_var> &pair_values :
         rhs.values_multiply ) {
        // values do not multiply against each other, they add.
        // so +10% and -10% will add to 0%
        values_multiply[pair_values.first] += pair_values.second.evaluate( d );
    }

    for( const std::pair<const skill_id, dbl_or_var> &pair_values :
         rhs.skill_values_add ) {
        skill_values_add[pair_values.first] += pair_values.second.evaluate( d );
    }
    for( const std::pair<const skill_id, dbl_or_var> &pair_values :
         rhs.skill_values_multiply ) {
        // values do not multiply against each other, they add.
        // so +10% and -10% will add to 0%
        skill_values_multiply[pair_values.first] += pair_values.second.evaluate( d );
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
        mutations.emplace( branch );
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

int enchantment::get_value_add( const enchant_vals::mod value, const Character &guy ) const
{
    const auto found = values_add.find( value );
    if( found == values_add.cend() ) {
        return 0;
    }
    dialogue d( get_talker_for( guy ), nullptr );
    return found->second.evaluate( d );
}

double enchantment::get_value_multiply( const enchant_vals::mod value, const Character &guy ) const
{
    const auto found = values_multiply.find( value );
    if( found == values_multiply.cend() ) {
        return 0;
    }
    dialogue d( get_talker_for( guy ), nullptr );
    return found->second.evaluate( d );
}

int enchant_cache::get_value_add( const enchant_vals::mod value ) const
{
    const auto found = values_add.find( value );
    if( found == values_add.cend() ) {
        return 0;
    }
    return found->second;
}

int enchant_cache::get_skill_value_add( const skill_id &value ) const
{
    const auto found = skill_values_add.find( value );
    if( found == skill_values_add.cend() ) {
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

double enchant_cache::get_skill_value_multiply( const skill_id &value ) const
{
    const auto found = skill_values_multiply.find( value );
    if( found == skill_values_multiply.cend() ) {
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
    value += units::from_millijoule<int>( get_value_add( mod_val ) );
    value *= 1.0 + get_value_multiply( mod_val );
    return value;
}

units::mass enchant_cache::modify_value( const enchant_vals::mod mod_val,
        units::mass value ) const
{
    value += units::from_gram<int>( get_value_add( mod_val ) );
    value *= 1.0 + get_value_multiply( mod_val );
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
    guy.mod_str_bonus( get_value_add( enchant_vals::mod::STRENGTH ) );
    guy.mod_str_bonus( mult_bonus( enchant_vals::mod::STRENGTH, guy.get_str_base() ) );

    guy.mod_dex_bonus( get_value_add( enchant_vals::mod::DEXTERITY ) );
    guy.mod_dex_bonus( mult_bonus( enchant_vals::mod::DEXTERITY, guy.get_dex_base() ) );

    guy.mod_per_bonus( get_value_add( enchant_vals::mod::PERCEPTION ) );
    guy.mod_per_bonus( mult_bonus( enchant_vals::mod::PERCEPTION, guy.get_per_base() ) );

    guy.mod_int_bonus( get_value_add( enchant_vals::mod::INTELLIGENCE ) );
    guy.mod_int_bonus( mult_bonus( enchant_vals::mod::INTELLIGENCE, guy.get_int_base() ) );

    guy.mod_num_dodges_bonus( get_value_add( enchant_vals::mod::BONUS_DODGE ) );
    guy.mod_num_dodges_bonus( mult_bonus( enchant_vals::mod::BONUS_DODGE, guy.get_num_dodges_base() ) );

    guy.mod_num_blocks_bonus( get_value_add( enchant_vals::mod::BONUS_BLOCK ) );
    guy.mod_num_blocks_bonus( mult_bonus( enchant_vals::mod::BONUS_BLOCK, guy.get_num_blocks_base() ) );

    if( emitter ) {
        get_map().emit_field( guy.pos(), *emitter );
    }
    for( const std::pair<efftype_id, int> eff : ench_effects ) {
        guy.add_effect( eff.first, 1_seconds, false, eff.second );
    }
    for( const std::pair<const time_duration, std::vector<fake_spell>> &activation :
         intermittent_activation ) {
        // a random approximation!
        if( one_in( to_seconds<int>( activation.first ) ) ) {
            for( const fake_spell &fake : activation.second ) {
                fake.get_spell( guy, 0 ).cast_all_effects( guy, guy.pos() );
            }
        }
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

void enchant_cache::cast_enchantment_spell( Character &caster, const Creature *target,
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
        sp.get_spell( caster, sp.level ).cast_all_effects( caster, caster.pos() );
    } else  if( target != nullptr ) {
        const Creature &trg_crtr = *target;
        const spell &spell_lvl = sp.get_spell( caster, sp.level );
        if( !spell_lvl.is_valid_target( caster, trg_crtr.pos() ) ||
            !spell_lvl.is_target_in_range( caster, trg_crtr.pos() ) ) {
            return;
        }

        caster.add_msg_player_or_npc( m_good,
                                      sp.trigger_message,
                                      sp.npc_trigger_message,
                                      caster.get_name(), trg_crtr.disp_name() );

        spell_lvl.cast_all_effects( caster, trg_crtr.pos() );
    }
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
