#include "relic.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <set>

#include "creature.h"
#include "debug.h"
#include "enums.h"
#include "generic_factory.h"
#include "json.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "rng.h"
#include "translations.h"
#include "type_id.h"

namespace io
{
    // *INDENT-OFF*
    template<>
    std::string enum_to_string<relic_procgen_data::type>( relic_procgen_data::type data )
    {
        switch( data ) {
        case relic_procgen_data::type::active_enchantment: return "active_enchantment";
        case relic_procgen_data::type::hit_me: return "hit_me";
        case relic_procgen_data::type::hit_you: return "hit_you";
        case relic_procgen_data::type::passive_enchantment_add: return "passive_enchantment_add";
        case relic_procgen_data::type::passive_enchantment_mult: return "passive_enchantment_mult";
        case relic_procgen_data::type::last: break;
        }
        debugmsg( "Invalid enchantment::has" );
        abort();
    }
    // *INDENT-ON*
template<>
std::string enum_to_string<relic_recharge>( relic_recharge type )
{
    // *INDENT-OFF*
    switch( type ) {
        case relic_recharge::none: return "none";
        case relic_recharge::periodic: return "periodic";
        case relic_recharge::num: break;
    }
    // *INDENT-ON*
    debugmsg( "Invalid relic recharge type" );
    abort();
}
} // namespace io

namespace
{
generic_factory<relic_procgen_data> relic_procgen_data_factory( "relic_procgen_data" );
} // namespace

template<>
const relic_procgen_data &string_id<relic_procgen_data>::obj() const
{
    return relic_procgen_data_factory.obj( *this );
}

template<>
bool string_id<relic_procgen_data>::is_valid() const
{
    return relic_procgen_data_factory.is_valid( *this );
}

void relic_procgen_data::load_relic_procgen_data( const JsonObject &jo, const std::string &src )
{
    relic_procgen_data_factory.load( jo, src );
}

void relic::add_active_effect( const fake_spell &sp )
{
    active_effects.emplace_back( sp );
}

void relic::add_passive_effect( const enchantment &nench )
{
    for( enchantment &ench : passive_effects ) {
        if( ench.add( nench ) ) {
            return;
        }
    }
    passive_effects.emplace_back( nench );
}

template<typename T>
void relic_procgen_data::enchantment_value_passive<T>::load( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "type", type );
    optional( jo, was_loaded, "power_per_increment", power_per_increment, 1 );
    optional( jo, was_loaded, "increment", increment, 1 );
    optional( jo, was_loaded, "min_value", min_value, 0 );
    optional( jo, was_loaded, "max_value", max_value, 0 );
}

template<typename T>
void relic_procgen_data::enchantment_value_passive<T>::deserialize( JsonIn &jsin )
{
    JsonObject jobj = jsin.get_object();
    load( jobj );
}

void relic_procgen_data::enchantment_active::load( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "spell_id", activated_spell );
    optional( jo, was_loaded, "base_power", base_power, 0 );
    optional( jo, was_loaded, "power_per_increment", power_per_increment, 1 );
    optional( jo, was_loaded, "increment", increment, 1 );
    optional( jo, was_loaded, "min_level", min_level, 0 );
    optional( jo, was_loaded, "max_level", max_level, 0 );
}

void relic_procgen_data::enchantment_active::deserialize( JsonIn &jsin )
{
    JsonObject jobj = jsin.get_object();
    load( jobj );
}

void relic_procgen_data::load( const JsonObject &jo, const std::string & )
{
    for( const JsonObject &jo_inner : jo.get_array( "passive_add_procgen_values" ) ) {
        int weight = 0;
        mandatory( jo_inner, was_loaded, "weight", weight );
        relic_procgen_data::enchantment_value_passive<int> val;
        val.load( jo_inner );

        passive_add_procgen_values.add( val, weight );
    }

    for( const JsonObject &jo_inner : jo.get_array( "passive_mult_procgen_values" ) ) {
        int weight = 0;
        mandatory( jo_inner, was_loaded, "weight", weight );
        relic_procgen_data::enchantment_value_passive<float> val;
        val.load( jo_inner );

        passive_mult_procgen_values.add( val, weight );
    }

    for( const JsonObject &jo_inner : jo.get_array( "type_weights" ) ) {
        int weight = 0;
        mandatory( jo_inner, was_loaded, "weight", weight );
        relic_procgen_data::type val = relic_procgen_data::type::last;
        mandatory( jo_inner, was_loaded, "value", val );

        type_weights.add( val, weight );
    }

    for( const JsonObject &jo_inner : jo.get_array( "items" ) ) {
        int weight = 0;
        mandatory( jo_inner, was_loaded, "weight", weight );
        itype_id it;
        mandatory( jo_inner, was_loaded, "item", it );

        item_weights.add( it, weight );
    }

    for( const JsonObject &jo_inner : jo.get_array( "active_procgen_values" ) ) {
        int weight = 0;
        mandatory( jo_inner, was_loaded, "weight", weight );
        relic_procgen_data::enchantment_active val;
        val.load( jo_inner );

        active_procgen_values.add( val, weight );
    }

    for( const JsonObject &jo_inner : jo.get_array( "charge_types" ) ) {
        int weight = 0;
        mandatory( jo_inner, was_loaded, "weight", weight );
        relic_charge_template charge;
        charge.load( jo_inner );

        charge_values.add( charge, weight );
    }
}

void relic_procgen_data::generation_rules::load( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "power_level", power_level );
    mandatory( jo, was_loaded, "max_attributes", max_attributes );
    optional( jo, was_loaded, "max_negative_power", max_negative_power, 0 );
}

void relic_procgen_data::generation_rules::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    load( jo );
}

void relic_procgen_data::deserialize( JsonIn &jsin )
{
    JsonObject jobj = jsin.get_object();
    load( jobj );
}

void relic_charge_template::deserialize( JsonIn &jsin )
{
    load( jsin.get_object() );
}

void relic_charge_template::load( const JsonObject &jo )
{
    int tmp_power = 0;

    const JsonObject max_charge = jo.get_object( "max_charges" );
    max_charge.read( "range", max_charges );
    max_charge.read( "power", tmp_power );
    power_level += tmp_power;

    const JsonObject charge = jo.get_object( "charges" );
    charge.read( "range", init_charges );
    charge.read( "power", tmp_power );
    power_level += tmp_power;

    const JsonObject init_charge = jo.get_object( "charges_per_use" );
    init_charge.read( "range", charges_per_use );
    init_charge.read( "power", tmp_power );
    power_level += tmp_power;

    jo.read( "recharge_type", type );
    jo.read( "time", time );
}

relic_charge_info relic_charge_template::generate() const
{
    relic_charge_info ret;
    ret.max_charges = rng( max_charges.first, max_charges.second );
    ret.charges_per_use = rng( charges_per_use.first, charges_per_use.second );
    ret.charges = std::min( rng( init_charges.first, init_charges.second ), ret.max_charges );
    ret.activation_time = rng( time.first, time.second );
    ret.type = type;
    ret.power = power_level;
    return ret;
}

void relic_charge_info::deserialize( JsonIn &jsin )
{
    load( jsin.get_object() );
}

void relic_charge_info::load( const JsonObject &jo )
{
    jo.read( "charges", charges );
    jo.read( "charges_per_use", charges_per_use );
    jo.read( "max_charges", max_charges );
    jo.read( "recharge_type", type );
    jo.read( "activation_accumulator", activation_accumulator );
    jo.read( "time", activation_time );
}

void relic_charge_info::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "charges", charges );
    jsout.member( "charges_per_use", charges_per_use );
    jsout.member( "max_charges", max_charges );
    jsout.member( "recharge_type", type );
    jsout.member( "activation_accumulator", activation_accumulator );
    jsout.member( "time", activation_time );
    jsout.end_object();
}

void relic_charge_info::accumulate_charge()
{
    if( charges >= max_charges || activation_time == 0_seconds ) {
        // return early, no accumulation required
        return;
    }
    activation_accumulator += 1_seconds;
    if( activation_accumulator >= activation_time ) {
        activation_accumulator -= activation_time;
        charges++;
    }
}

void relic::load( const JsonObject &jo )
{
    if( jo.has_array( "active_effects" ) ) {
        for( JsonObject jobj : jo.get_array( "active_effects" ) ) {
            fake_spell sp;
            sp.load( jobj );
            add_active_effect( sp );
        }
    }
    if( jo.has_array( "passive_effects" ) ) {
        for( JsonObject jobj : jo.get_array( "passive_effects" ) ) {
            enchantment ench;
            ench.load( jobj );
            if( !ench.id.is_empty() ) {
                ench = ench.id.obj();
            }
            add_passive_effect( ench );
        }
    }
    jo.read( "charge_info", charge );
    if( jo.has_member( "charges_per_activation" ) ) {
        charge.charges_per_use = jo.get_int( "charges_per_activation", 1 );
    }
    jo.read( "name", item_name_override );
    moves = jo.get_int( "moves", 100 );
}

void relic::deserialize( JsonIn &jsin )
{
    JsonObject jobj = jsin.get_object();
    load( jobj );
}

void relic::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves );
    // item_name_override is not saved, in case the original json text changes:
    // in such case names read back from a save wouold no longer be properly translated.

    if( !passive_effects.empty() ) {
        jsout.member( "passive_effects" );
        jsout.start_array();
        for( const enchantment &ench : passive_effects ) {
            ench.serialize( jsout );
        }
        jsout.end_array();
    }

    if( !active_effects.empty() ) {
        jsout.member( "active_effects" );
        jsout.start_array();
        for( const fake_spell &sp : active_effects ) {
            sp.serialize( jsout );
        }
        jsout.end_array();
    }

    jsout.member( "charge_info", charge );

    jsout.end_object();
}

int relic::activate( Creature &caster, const tripoint &target )
{
    if( charge.charges_per_use != 0 && charges() - charge.charges_per_use < 0 ) {
        caster.add_msg_if_player( m_bad, _( "This artifact lacks the charges to activate." ) );
        return 0;
    }
    caster.moves -= moves;
    for( const fake_spell &sp : active_effects ) {
        spell casting = sp.get_spell( sp.level );
        casting.cast_all_effects( caster, target );
        caster.add_msg_if_player( casting.message() );
    }
    charge.charges -= charge.charges_per_use;
    return charge.charges_per_use;
}

int relic::charges() const
{
    return charge.charges;
}

int relic::charges_per_use() const
{
    return charge.charges_per_use;
}

int relic::max_charges() const
{
    return charge.max_charges;
}

bool relic::has_recharge() const
{
    return charge.type != relic_recharge::none;
}

void relic::try_recharge()
{
    if( charge.charges == charge.max_charges ) {
        return;
    }
    switch( charge.type ) {
        case relic_recharge::none: {
            return;
        }
        case relic_recharge::periodic: {
            charge.accumulate_charge();
            return;
        }
        case relic_recharge::num: {
            debugmsg( "Attempted to recharge relic with invalid recharge type" );
            return;
        }
    }
}

void relic::overwrite_charge( const relic_charge_info &info )
{
    charge = info;
}

int relic::modify_value( const enchant_vals::mod value_type, const int value ) const
{
    int add_modifier = 0;
    double multiply_modifier = 0.0;
    for( const enchantment &ench : passive_effects ) {
        add_modifier += ench.get_value_add( value_type );
        multiply_modifier += ench.get_value_multiply( value_type );
    }
    multiply_modifier = std::max( multiply_modifier + 1.0, 0.0 );
    int modified_value;
    if( multiply_modifier < 1.0 ) {
        modified_value = std::floor( multiply_modifier * value );
    } else {
        modified_value = std::ceil( multiply_modifier * value );
    }
    return modified_value + add_modifier;
}

std::string relic::name() const
{
    return item_name_override.translated();
}

std::vector<enchantment> relic::get_enchantments() const
{
    return passive_effects;
}

int relic::power_level( const relic_procgen_id &ruleset ) const
{
    int total_power_level = 0;
    for( const enchantment &ench : passive_effects ) {
        total_power_level += ruleset->power_level( ench );
    }
    for( const fake_spell &sp : active_effects ) {
        total_power_level += ruleset->power_level( sp );
    }
    total_power_level += charge.power;
    return total_power_level;
}

bool relic::has_activation() const
{
    return !active_effects.empty();
}

int relic_procgen_data::power_level( const enchantment &ench ) const
{
    int power = 0;

    for( const weighted_object<int, relic_procgen_data::enchantment_value_passive<int>>
         &add_val_passive : passive_add_procgen_values ) {
        int val = ench.get_value_add( add_val_passive.obj.type );
        if( val != 0 ) {
            power += static_cast<float>( add_val_passive.obj.power_per_increment ) /
                     static_cast<float>( add_val_passive.obj.increment ) * val;
        }
    }

    for( const weighted_object<int, relic_procgen_data::enchantment_value_passive<float>>
         &mult_val_passive : passive_mult_procgen_values ) {
        float val = ench.get_value_multiply( mult_val_passive.obj.type );
        if( val != 0.0f ) {
            power += mult_val_passive.obj.power_per_increment / mult_val_passive.obj.increment * val;
        }
    }

    return power;
}

int relic_procgen_data::power_level( const fake_spell &sp ) const
{
    for( const weighted_object<int, relic_procgen_data::enchantment_active> &vals :
         active_procgen_values ) {
        if( vals.obj.activated_spell == sp.id ) {
            return vals.obj.calc_power( sp.level );
        }
    }
    return 0;
}

item relic_procgen_data::create_item( const relic_procgen_data::generation_rules &rules ) const
{
    const itype_id *it_id = item_weights.pick();
    if( it_id == nullptr ) {
        debugmsg( "ERROR: %s procgen data does not have items", id.c_str() );
        return null_item_reference();
    }

    item it( *it_id, calendar::turn );

    it.overwrite_relic( generate( rules, *it_id ) );

    return it;
}

relic relic_procgen_data::generate( const relic_procgen_data::generation_rules &rules,
                                    const itype_id &it_id ) const
{
    relic ret;
    int num_attributes = 0;
    int negative_attribute_power = 0;
    const bool is_armor = item( it_id ).is_armor();

    while( rules.max_attributes > num_attributes && rules.power_level > ret.power_level( id ) ) {
        switch( *type_weights.pick() ) {
            case relic_procgen_data::type::active_enchantment: {
                const relic_procgen_data::enchantment_active *active = active_procgen_values.pick();
                if( active != nullptr ) {
                    fake_spell active_sp;
                    active_sp.id = active->activated_spell;
                    active_sp.level = rng( active->min_level, active->max_level );
                    num_attributes++;
                    int power = power_level( active_sp );
                    if( power < 0 ) {
                        if( rules.max_negative_power > negative_attribute_power ) {
                            break;
                        }
                        negative_attribute_power += power;
                    }
                    ret.add_active_effect( active_sp );
                }
                break;
            }
            case relic_procgen_data::type::passive_enchantment_add: {
                const relic_procgen_data::enchantment_value_passive<int> *add = passive_add_procgen_values.pick();
                if( add != nullptr ) {
                    enchantment ench;
                    int value = rng( add->min_value, add->max_value );
                    if( value == 0 ) {
                        break;
                    }
                    ench.add_value_add( add->type, value );
                    num_attributes++;
                    int negative_ench_attribute = power_level( ench );
                    if( negative_ench_attribute < 0 ) {
                        if( rules.max_negative_power > negative_attribute_power ) {
                            break;
                        }
                        negative_attribute_power += negative_ench_attribute;
                    }
                    if( is_armor ) {
                        ench.set_has( enchantment::has::WORN );
                    } else {
                        ench.set_has( enchantment::has::WIELD );
                    }
                    ret.add_passive_effect( ench );
                }
                break;
            }
            case relic_procgen_data::type::passive_enchantment_mult: {
                const relic_procgen_data::enchantment_value_passive<float> *mult =
                    passive_mult_procgen_values.pick();
                if( mult != nullptr ) {
                    enchantment ench;
                    float value = rng( mult->min_value, mult->max_value );
                    ench.add_value_mult( mult->type, value );
                    num_attributes++;
                    int negative_ench_attribute = power_level( ench );
                    if( negative_ench_attribute < 0 ) {
                        if( rules.max_negative_power > negative_attribute_power ) {
                            break;
                        }
                        negative_attribute_power += negative_ench_attribute;
                    }
                    if( is_armor ) {
                        ench.set_has( enchantment::has::WORN );
                    } else {
                        ench.set_has( enchantment::has::WIELD );
                    }
                    ret.add_passive_effect( ench );
                }
                break;
            }
            case relic_procgen_data::type::hit_me: {
                const relic_procgen_data::enchantment_active *active = passive_hit_me.pick();
                if( active != nullptr ) {
                    fake_spell active_sp;
                    active_sp.id = active->activated_spell;
                    active_sp.level = rng( active->min_level, active->max_level );
                    num_attributes++;
                    enchantment ench;
                    ench.add_hit_me( active_sp );
                    int power = power_level( ench );
                    if( power < 0 ) {
                        if( rules.max_negative_power > negative_attribute_power ) {
                            break;
                        }
                        negative_attribute_power += power;
                    }
                    if( is_armor ) {
                        ench.set_has( enchantment::has::WORN );
                    } else {
                        ench.set_has( enchantment::has::WIELD );
                    }
                    ret.add_passive_effect( ench );
                }
                break;
            }
            case relic_procgen_data::type::hit_you: {
                const relic_procgen_data::enchantment_active *active = passive_hit_you.pick();
                if( active != nullptr ) {
                    fake_spell active_sp;
                    active_sp.id = active->activated_spell;
                    active_sp.level = rng( active->min_level, active->max_level );
                    num_attributes++;
                    enchantment ench;
                    ench.add_hit_you( active_sp );
                    int power = power_level( ench );
                    if( power < 0 ) {
                        if( rules.max_negative_power > negative_attribute_power ) {
                            break;
                        }
                        negative_attribute_power += power;
                    }
                    if( is_armor ) {
                        ench.set_has( enchantment::has::WORN );
                    } else {
                        ench.set_has( enchantment::has::WIELD );
                    }
                    ret.add_passive_effect( ench );
                }
                break;
            }
            case relic_procgen_data::type::last: {
                debugmsg( "ERROR: invalid relic procgen type" );
                break;
            }
        }
    }

    if( ret.has_activation() ) {
        const relic_charge_template *charge = charge_values.pick();
        if( charge != nullptr ) {
            ret.overwrite_charge( charge->generate() );
        }
    }

    return ret;
}
