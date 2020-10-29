#include "magic_enchantment.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <set>

#include "bodypart.h"
#include "character.h"
#include "creature.h"
#include "debug.h"
#include "enum_conversions.h"
#include "enums.h"
#include "game.h"
#include "generic_factory.h"
#include "json.h"
#include "map.h"
#include "point.h"
#include "rng.h"
#include "string_id.h"
#include "units.h"

template <typename E> struct enum_traits;

template<>
struct enum_traits<enchantment::has> {
    static constexpr enchantment::has last = enchantment::has::NUM_HAS;
};

template<>
struct enum_traits<enchantment::condition> {
    static constexpr enchantment::condition last = enchantment::condition::NUM_CONDITION;
};

template<>
struct enum_traits<enchantment::mod> {
    static constexpr enchantment::mod last = enchantment::mod::NUM_MOD;
};

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
        debugmsg( "Invalid enchantment::has" );
        abort();
    }

    template<>
    std::string enum_to_string<enchantment::condition>( enchantment::condition data )
    {
        switch ( data ) {
        case enchantment::condition::ALWAYS: return "ALWAYS";
        case enchantment::condition::UNDERGROUND: return "UNDERGROUND";
        case enchantment::condition::UNDERWATER: return "UNDERWATER";
        case enchantment::condition::ACTIVE: return "ACTIVE";
        case enchantment::condition::NUM_CONDITION: break;
        }
        debugmsg( "Invalid enchantment::condition" );
        abort();
    }

    template<>
    std::string enum_to_string<enchantment::mod>( enchantment::mod data )
    {
        switch ( data ) {
            case enchantment::mod::STRENGTH: return "STRENGTH";
            case enchantment::mod::DEXTERITY: return "DEXTERITY";
            case enchantment::mod::PERCEPTION: return "PERCEPTION";
            case enchantment::mod::INTELLIGENCE: return "INTELLIGENCE";
            case enchantment::mod::SPEED: return "SPEED";
            case enchantment::mod::ATTACK_COST: return "ATTACK_COST";
            case enchantment::mod::ATTACK_SPEED: return "ATTACK_SPEED";
            case enchantment::mod::MOVE_COST: return "MOVE_COST";
            case enchantment::mod::METABOLISM: return "METABOLISM";
            case enchantment::mod::MAX_MANA: return "MAX_MANA";
            case enchantment::mod::REGEN_MANA: return "REGEN_MANA";
            case enchantment::mod::BIONIC_POWER: return "BIONIC_POWER";
            case enchantment::mod::MAX_STAMINA: return "MAX_STAMINA";
            case enchantment::mod::REGEN_STAMINA: return "REGEN_STAMINA";
            case enchantment::mod::MAX_HP: return "MAX_HP";
            case enchantment::mod::REGEN_HP: return "REGEN_HP";
            case enchantment::mod::THIRST: return "THIRST";
            case enchantment::mod::FATIGUE: return "FATIGUE";
            case enchantment::mod::PAIN: return "PAIN";
            case enchantment::mod::BONUS_DAMAGE: return "BONUS_DAMAGE";
            case enchantment::mod::BONUS_BLOCK: return "BONUS_BLOCK";
            case enchantment::mod::BONUS_DODGE: return "BONUS_DODGE";
            case enchantment::mod::ATTACK_NOISE: return "ATTACK_NOISE";
            case enchantment::mod::SPELL_NOISE: return "SPELL_NOISE";
            case enchantment::mod::SHOUT_NOISE: return "SHOUT_NOISE";
            case enchantment::mod::FOOTSTEP_NOISE: return "FOOTSTEP_NOISE";
            case enchantment::mod::SIGHT_RANGE: return "SIGHT_RANGE";
            case enchantment::mod::CARRY_WEIGHT: return "CARRY_WEIGHT";
            case enchantment::mod::CARRY_VOLUME: return "CARRY_VOLUME";
            case enchantment::mod::SOCIAL_LIE: return "SOCIAL_LIE";
            case enchantment::mod::SOCIAL_PERSUADE: return "SOCIAL_PERSUADE";
            case enchantment::mod::SOCIAL_INTIMIDATE: return "SOCIAL_INTIMIDATE";
            case enchantment::mod::ARMOR_ACID: return "ARMOR_ACID";
            case enchantment::mod::ARMOR_BASH: return "ARMOR_BASH";
            case enchantment::mod::ARMOR_BIO: return "ARMOR_BIO";
            case enchantment::mod::ARMOR_COLD: return "ARMOR_COLD";
            case enchantment::mod::ARMOR_CUT: return "ARMOR_CUT";
            case enchantment::mod::ARMOR_ELEC: return "ARMOR_ELEC";
            case enchantment::mod::ARMOR_HEAT: return "ARMOR_HEAT";
            case enchantment::mod::ARMOR_STAB: return "ARMOR_STAB";
            case enchantment::mod::ITEM_DAMAGE_BASH: return "ITEM_DAMAGE_BASH";
            case enchantment::mod::ITEM_DAMAGE_CUT: return "ITEM_DAMAGE_CUT";
            case enchantment::mod::ITEM_DAMAGE_STAB: return "ITEM_DAMAGE_STAB";
            case enchantment::mod::ITEM_DAMAGE_HEAT: return "ITEM_DAMAGE_HEAT";
            case enchantment::mod::ITEM_DAMAGE_COLD: return "ITEM_DAMAGE_COLD";
            case enchantment::mod::ITEM_DAMAGE_ELEC: return "ITEM_DAMAGE_ELEC";
            case enchantment::mod::ITEM_DAMAGE_ACID: return "ITEM_DAMAGE_ACID";
            case enchantment::mod::ITEM_DAMAGE_BIO: return "ITEM_DAMAGE_BIO";
            case enchantment::mod::ITEM_DAMAGE_AP: return "ITEM_DAMAGE_AP";
            case enchantment::mod::ITEM_ARMOR_BASH: return "ITEM_ARMOR_BASH";
            case enchantment::mod::ITEM_ARMOR_CUT: return "ITEM_ARMOR_CUT";
            case enchantment::mod::ITEM_ARMOR_STAB: return "ITEM_ARMOR_STAB";
            case enchantment::mod::ITEM_ARMOR_HEAT: return "ITEM_ARMOR_HEAT";
            case enchantment::mod::ITEM_ARMOR_COLD: return "ITEM_ARMOR_COLD";
            case enchantment::mod::ITEM_ARMOR_ELEC: return "ITEM_ARMOR_ELEC";
            case enchantment::mod::ITEM_ARMOR_ACID: return "ITEM_ARMOR_ACID";
            case enchantment::mod::ITEM_ARMOR_BIO: return "ITEM_ARMOR_BIO";
            case enchantment::mod::ITEM_WEIGHT: return "ITEM_WEIGHT";
            case enchantment::mod::ITEM_ENCUMBRANCE: return "ITEM_ENCUMBRANCE";
            case enchantment::mod::ITEM_VOLUME: return "ITEM_VOLUME";
            case enchantment::mod::ITEM_COVERAGE: return "ITEM_COVERAGE";
            case enchantment::mod::ITEM_ATTACK_SPEED: return "ITEM_ATTACK_SPEED";
            case enchantment::mod::ITEM_WET_PROTECTION: return "ITEM_WET_PROTECTION";
            case enchantment::mod::NUM_MOD: break;
        }
        debugmsg( "Invalid enchantment::mod" );
        abort();
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

    if( active_conditions.second == condition::ALWAYS ) {
        return true;
    }

    if( active_conditions.second == condition::UNDERGROUND ) {
        return guy.pos().z < 0;
    }

    if( active_conditions.second == condition::UNDERWATER ) {
        return g->m.is_divable( guy.pos() );
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

void enchantment::load( const JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "id", id, enchantment_id( "" ) );

    jo.read( "hit_you_effect", hit_you_effect );
    jo.read( "hit_me_effect", hit_me_effect );
    jo.read( "emitter", emitter );

    if( jo.has_object( "intermittent_activation" ) ) {
        JsonObject jobj = jo.get_object( "intermittent_activation" );
        for( const JsonObject effect_obj : jobj.get_array( "effects" ) ) {
            time_duration dur = read_from_json_string<time_duration>( *effect_obj.get_raw( "frequency" ),
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
    active_conditions.second = io::string_to_enum<condition>( jo.get_string( "condition",
                               "ALWAYS" ) );

    for( JsonObject jsobj : jo.get_array( "ench_effects" ) ) {
        ench_effects.emplace( efftype_id( jsobj.get_string( "effect" ) ), jsobj.get_int( "intensity" ) );
    }

    if( jo.has_array( "values" ) ) {
        for( const JsonObject value_obj : jo.get_array( "values" ) ) {
            const enchantment::mod value = io::string_to_enum<mod>( value_obj.get_string( "value" ) );
            const int add = value_obj.get_int( "add", 0 );
            const double mult = value_obj.get_float( "multiply", 0.0 );
            if( add != 0 ) {
                values_add.emplace( value, add );
            }
            if( mult != 0.0 ) {
                values_multiply.emplace( value, mult );
            }
        }
    }
}

void enchantment::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    if( !id.is_empty() ) {
        jsout.member( "id", id );
        jsout.end_object();
        // if the enchantment has an id then it is defined elsewhere and does not need to be serialized.
        return;
    }

    jsout.member( "has", io::enum_to_string<has>( active_conditions.first ) );
    jsout.member( "condition", io::enum_to_string<condition>( active_conditions.second ) );
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
        for( const std::pair<time_duration, std::vector<fake_spell>> pair : intermittent_activation ) {
            jsout.member( "duration", pair.first );
            jsout.start_array( "effects" );
            for( const fake_spell &sp : pair.second ) {
                sp.serialize( jsout );
            }
            jsout.end_array();
        }
        jsout.end_object();
    }

    jsout.member( "values" );
    jsout.start_array();
    for( int value = 0; value < mod::NUM_MOD; value++ ) {
        mod enum_value = static_cast<mod>( value );
        if( get_value_add( enum_value ) == 0 && get_value_multiply( enum_value ) == 0.0 ) {
            continue;
        }
        jsout.start_object();
        jsout.member( "value", io::enum_to_string<mod>( enum_value ) );
        if( get_value_add( enum_value ) != 0 ) {
            jsout.member( "add", get_value_add( enum_value ) );
        }
        if( get_value_multiply( enum_value ) != 0 ) {
            jsout.member( "multiply", get_value_multiply( enum_value ) );
        }
        jsout.end_object();
    }
    jsout.end_array();

    jsout.end_object();
}

bool enchantment::stacks_with( const enchantment &rhs ) const
{
    return active_conditions == rhs.active_conditions;
}

bool enchantment::add( const enchantment &rhs )
{
    if( !stacks_with( rhs ) ) {
        return false;
    }
    force_add( rhs );
    return true;
}

void enchantment::force_add( const enchantment &rhs )
{
    for( const std::pair<const mod, int> &pair_values : rhs.values_add ) {
        values_add[pair_values.first] += pair_values.second;
    }
    for( const std::pair<const mod, double> &pair_values : rhs.values_multiply ) {
        // values do not multiply against each other, they add.
        // so +10% and -10% will add to 0%
        values_multiply[pair_values.first] += pair_values.second;
    }

    hit_me_effect.insert( hit_me_effect.end(), rhs.hit_me_effect.begin(), rhs.hit_me_effect.end() );

    hit_you_effect.insert( hit_you_effect.end(), rhs.hit_you_effect.begin(), rhs.hit_you_effect.end() );

    ench_effects.insert( rhs.ench_effects.begin(), rhs.ench_effects.end() );

    if( rhs.emitter ) {
        emitter = rhs.emitter;
    }

    for( const std::pair<const time_duration, std::vector<fake_spell>> &act_pair :
         rhs.intermittent_activation ) {
        for( const fake_spell &fake : act_pair.second ) {
            intermittent_activation[act_pair.first].emplace_back( fake );
        }
    }
}

int enchantment::get_value_add( const mod value ) const
{
    const auto found = values_add.find( value );
    if( found == values_add.cend() ) {
        return 0;
    }
    return found->second;
}

double enchantment::get_value_multiply( const mod value ) const
{
    const auto found = values_multiply.find( value );
    if( found == values_multiply.cend() ) {
        return 0;
    }
    return found->second;
}

int enchantment::mult_bonus( enchantment::mod value_type, int base_value ) const
{
    return get_value_multiply( value_type ) * base_value;
}

void enchantment::activate_passive( Character &guy ) const
{
    guy.mod_str_bonus( get_value_add( mod::STRENGTH ) );
    guy.mod_str_bonus( mult_bonus( mod::STRENGTH, guy.get_str_base() ) );

    guy.mod_dex_bonus( get_value_add( mod::DEXTERITY ) );
    guy.mod_dex_bonus( mult_bonus( mod::DEXTERITY, guy.get_dex_base() ) );

    guy.mod_per_bonus( get_value_add( mod::PERCEPTION ) );
    guy.mod_per_bonus( mult_bonus( mod::PERCEPTION, guy.get_per_base() ) );

    guy.mod_int_bonus( get_value_add( mod::INTELLIGENCE ) );
    guy.mod_int_bonus( mult_bonus( mod::INTELLIGENCE, guy.get_int_base() ) );

    guy.mod_speed_bonus( get_value_add( mod::SPEED ) );
    guy.mod_speed_bonus( mult_bonus( mod::SPEED, guy.get_speed_base() ) );

    guy.mod_num_dodges_bonus( get_value_add( mod::BONUS_DODGE ) );
    guy.mod_num_dodges_bonus( mult_bonus( mod::BONUS_DODGE, guy.get_num_dodges_base() ) );

    if( emitter ) {
        g->m.emit_field( guy.pos(), *emitter );
    }
    for( const std::pair<efftype_id, int> eff : ench_effects ) {
        guy.add_effect( eff.first, 1_seconds, num_bp, false, eff.second );
    }
    for( const std::pair<const time_duration, std::vector<fake_spell>> &activation :
         intermittent_activation ) {
        // a random approximation!
        if( one_in( to_seconds<int>( activation.first ) ) ) {
            for( const fake_spell &fake : activation.second ) {
                fake.get_spell( 0 ).cast_all_effects( guy, guy.pos() );
            }
        }
    }
}

void enchantment::cast_hit_you( Character &caster, const Creature &target ) const
{
    for( const fake_spell &sp : hit_you_effect ) {
        cast_enchantment_spell( caster, &target, sp );
    }
}

void enchantment::cast_hit_me( Character &caster, const Creature *target ) const
{
    for( const fake_spell &sp : hit_me_effect ) {
        cast_enchantment_spell( caster, target, sp );
    }
}

void enchantment::cast_enchantment_spell( Character &caster, const Creature *target,
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
                                      caster.name );
        sp.get_spell( sp.level ).cast_all_effects( caster, caster.pos() );
    } else  if( target != nullptr ) {
        const Creature &trg_crtr = *target;
        const spell &spell_lvl = sp.get_spell( sp.level );
        if( !spell_lvl.is_valid_target( caster, trg_crtr.pos() ) ||
            !spell_lvl.is_target_in_range( caster, trg_crtr.pos() ) ) {
            return;
        }

        caster.add_msg_player_or_npc( m_good,
                                      sp.trigger_message,
                                      sp.npc_trigger_message,
                                      caster.name, trg_crtr.disp_name() );

        spell_lvl.cast_all_effects( caster, trg_crtr.pos() );
    }
}
