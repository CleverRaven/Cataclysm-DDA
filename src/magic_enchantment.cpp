#include "magic_enchantment.h"

#include "character.h"
#include "game.h"
#include "generic_factory.h"
#include "item.h"
#include "json.h"
#include "map.h"
#include "units.h"

static const std::map<std::string, enchantment::has> has_map{
    { "WIELD", enchantment::has::WIELD },
    { "WORN", enchantment::has::WORN },
    { "HELD", enchantment::has::HELD }
};

static const std::map<std::string, enchantment::condition> condition_map{
    { "ALWAYS", enchantment::condition::ALWAYS },
    { "UNDERGROUND", enchantment::condition::UNDERGROUND },
    { "UNDERWATER", enchantment::condition::UNDERWATER }
};

static const std::map<std::string, enchantment::mod> mod_map{
    { "STRENGTH", enchantment::mod::STRENGTH },
    { "DEXTERITY", enchantment::mod::DEXTERITY },
    { "PERCEPTION", enchantment::mod::PERCEPTION },
    { "INTELLIGENCE", enchantment::mod::INTELLIGENCE },
    { "SPEED", enchantment::mod::SPEED },
    { "ATTACK_COST", enchantment::mod::ATTACK_COST },
    { "MOVE_COST", enchantment::mod::MOVE_COST },
    { "METABOLISM", enchantment::mod::METABOLISM },
    { "MAX_MANA", enchantment::mod::MAX_MANA },
    { "REGEN_MANA", enchantment::mod::REGEN_MANA },
    { "BIONIC_POWER", enchantment::mod::BIONIC_POWER },
    { "MAX_STAMINA", enchantment::mod::MAX_STAMINA },
    { "REGEN_STAMINA", enchantment::mod::REGEN_STAMINA },
    { "MAX_HP", enchantment::mod::MAX_HP },
    { "REGEN_HP", enchantment::mod::REGEN_HP },
    { "THIRST", enchantment::mod::THIRST },
    { "FATIGUE", enchantment::mod::FATIGUE },
    { "PAIN", enchantment::mod::PAIN },
    { "BONUS_DAMAGE", enchantment::mod::BONUS_DODGE },
    { "BONUS_BLOCK", enchantment::mod::BONUS_BLOCK },
    { "BONUS_DODGE", enchantment::mod::BONUS_DODGE },
    { "ATTACK_NOISE", enchantment::mod::ATTACK_NOISE },
    { "SPELL_NOISE", enchantment::mod::SPELL_NOISE },
    { "SHOUT_NOISE", enchantment::mod::SHOUT_NOISE },
    { "FOOTSTEP_NOISE", enchantment::mod::FOOTSTEP_NOISE },
    { "SIGHT_RANGE", enchantment::mod::SIGHT_RANGE },
    { "CARRY_WEIGHT", enchantment::mod::CARRY_WEIGHT },
    { "CARRY_VOLUME", enchantment::mod::CARRY_VOLUME },
    { "SOCIAL_LIE", enchantment::mod::SOCIAL_LIE },
    { "SOCIAL_PERSUADE", enchantment::mod::SOCIAL_PERSUADE },
    { "SOCIAL_INTIMIDATE", enchantment::mod::SOCIAL_INTIMIDATE },
    { "ITEM_DAMAGE_BASH", enchantment::mod::ITEM_DAMAGE_BASH },
    { "ITEM_DAMAGE_CUT", enchantment::mod::ITEM_DAMAGE_CUT },
    { "ITEM_DAMAGE_STAB", enchantment::mod::ITEM_DAMAGE_STAB },
    { "ITEM_DAMAGE_HEAT", enchantment::mod::ITEM_DAMAGE_HEAT },
    { "ITEM_DAMAGE_COLD", enchantment::mod::ITEM_DAMAGE_COLD },
    { "ITEM_DAMAGE_ELEC", enchantment::mod::ITEM_DAMAGE_ELEC },
    { "ITEM_DAMAGE_ACID", enchantment::mod::ITEM_DAMAGE_ACID },
    { "ITEM_DAMAGE_BIO", enchantment::mod::ITEM_DAMAGE_BIO },
    { "ITEM_DAMAGE_AP", enchantment::mod::ITEM_DAMAGE_AP },
    { "ITEM_ARMOR_BASH", enchantment::mod::ITEM_ARMOR_BASH },
    { "ITEM_ARMOR_CUT", enchantment::mod::ITEM_ARMOR_CUT },
    { "ITEM_ARMOR_STAB", enchantment::mod::ITEM_ARMOR_STAB },
    { "ITEM_ARMOR_HEAT", enchantment::mod::ITEM_ARMOR_HEAT },
    { "ITEM_ARMOR_COLD", enchantment::mod::ITEM_ARMOR_COLD },
    { "ITEM_ARMOR_ELEC", enchantment::mod::ITEM_ARMOR_ELEC },
    { "ITEM_ARMOR_ACID", enchantment::mod::ITEM_ARMOR_ACID },
    { "ITEM_ARMOR_BIO", enchantment::mod::ITEM_ARMOR_BIO },
    { "ITEM_WEIGHT", enchantment::mod::ITEM_WEIGHT },
    { "ITEM_ENCUMBRANCE", enchantment::mod::ITEM_ENCUMBRANCE },
    { "ITEM_VOLUME", enchantment::mod::ITEM_VOLUME },
    { "ITEM_COVERAGE", enchantment::mod::ITEM_COVERAGE },
};

namespace io
{
template<>
enchantment::has string_to_enum<enchantment::has>( const std::string &trigger )
{
    return string_to_enum_look_up( has_map, trigger );
}
template<>
enchantment::condition string_to_enum<enchantment::condition>( const std::string &trigger )
{
    return string_to_enum_look_up( condition_map, trigger );
}
template<>
enchantment::mod string_to_enum<enchantment::mod>( const std::string &trigger )
{
    return string_to_enum_look_up( mod_map, trigger );
}
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

void enchantment::load_enchantment( JsonObject &jo, const std::string &src )
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
           ( active_conditions.first == has::WIELD && &guy.weapon == &parent ) ||
           ( active_conditions.first == has::WORN && guy.is_worn( parent ) ) ) ) {
        return false;
    }

    if( active_conditions.second == condition::ALWAYS ) {
        return true;
    }

    if( active_conditions.second == condition::UNDERGROUND ) {
        if( guy.pos().z < 0 ) {
            return true;
        } else {
            return false;
        }
    }

    if( active_conditions.second == condition::UNDERWATER ) {
        if( g->m.is_divable( guy.pos() ) ) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

void enchantment::add_activation( const time_duration &dur, const fake_spell &fake )
{
    intermittent_activation[dur].emplace_back( fake );
}

void enchantment::load( JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "id", id, enchantment_id( "" ) );
    if( jo.has_array( "hit_you_effect" ) ) {
        JsonArray jarray = jo.get_array( "hit_you_effect" );
        while( jarray.has_more() ) {
            fake_spell fake;
            JsonObject fake_spell_obj = jarray.next_object();
            fake.load( fake_spell_obj );
            hit_you_effect.emplace_back( fake );
        }
    }

    if( jo.has_array( "hit_me_effect" ) ) {
        JsonArray jarray = jo.get_array( "hit_me_effect" );
        while( jarray.has_more() ) {
            fake_spell fake;
            JsonObject fake_spell_obj = jarray.next_object();
            fake.load( fake_spell_obj );
            hit_me_effect.emplace_back( fake );
        }
    }

    if( jo.has_object( "intermittent_activation" ) ) {
        JsonObject jobj = jo.get_object( "intermittent_activation" );
        JsonArray jarray = jo.get_array( "effects" );
        while( jarray.has_more() ) {
            JsonObject effect_obj;
            time_duration dur = read_from_json_string<time_duration>( *effect_obj.get_raw( "frequency" ),
                                time_duration::units );
            if( effect_obj.has_array( "spell_effects" ) ) {
                JsonArray jarray = effect_obj.get_array( "spell_effects" );
                while( jarray.has_more() ) {
                    fake_spell fake;
                    JsonObject fake_spell_obj = jarray.next_object();
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

    if( jo.has_array( "values" ) ) {
        JsonArray jarray = jo.get_array( "values" );
        while( jarray.has_more() ) {
            JsonObject value_obj = jarray.next_object();
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

bool enchantment::stacks_with( const enchantment &rhs ) const
{
    return active_conditions == rhs.active_conditions;
}

bool enchantment::add( const enchantment &rhs )
{
    if( !stacks_with( rhs ) ) {
        return false;
    }
    for( const std::pair<mod, int> &pair_values : rhs.values_add ) {
        values_add[pair_values.first] += pair_values.second;
    }
    for( const std::pair<mod, double> &pair_values : rhs.values_multiply ) {
        // values do not multiply against each other, they add.
        // so +10% and -10% will add to 0%
        values_multiply[pair_values.first] += pair_values.second;
    }
    for( const fake_spell &fake : rhs.hit_me_effect ) {
        hit_me_effect.emplace_back( fake );
    }
    for( const fake_spell &fake : rhs.hit_you_effect ) {
        hit_you_effect.emplace_back( fake );
    }
    for( const std::pair<time_duration, std::vector<fake_spell>> &act_pair :
         rhs.intermittent_activation ) {
        for( const fake_spell &fake : act_pair.second ) {
            intermittent_activation[act_pair.first].emplace_back( fake );
        }
    }
    return true;
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
}
