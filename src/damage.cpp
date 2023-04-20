#include "damage.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <numeric>
#include <string>
#include <utility>

#include "bodypart.h"
#include "cata_utility.h"
#include "debug.h"
#include "generic_factory.h"
#include "item.h"
#include "json.h"
#include "monster.h"
#include "mtype.h"
#include "translations.h"
#include "units.h"

namespace io
{

template<>
std::string enum_to_string<damage_type>( damage_type data )
{
    switch( data ) {
            // *INDENT-OFF*
        case damage_type::NONE: return "none";
        case damage_type::PURE: return "pure";
        case damage_type::BIOLOGICAL: return "biological";
        case damage_type::BASH: return "bash";
        case damage_type::CUT: return "cut";
        case damage_type::ACID: return "acid";
        case damage_type::STAB: return "stab";
        case damage_type::HEAT: return "heat";
        case damage_type::COLD: return "cold";
        case damage_type::ELECTRIC: return "electric";
        case damage_type::BULLET: return "bullet";
            // *INDENT-ON*
        case damage_type::NUM:
            break;
    }
    cata_fatal( "Invalid damage_type" );
}

} // namespace io

bool damage_unit::operator==( const damage_unit &other ) const
{
    return type == other.type &&
           amount == other.amount &&
           res_pen == other.res_pen &&
           res_mult == other.res_mult &&
           damage_multiplier == other.damage_multiplier &&
           unconditional_res_mult == other.unconditional_res_mult &&
           unconditional_damage_mult == other.unconditional_res_mult;
}

damage_instance::damage_instance() = default;
damage_instance damage_instance::physical( float bash, float cut, float stab, float arpen )
{
    damage_instance d;
    d.add_damage( damage_type::BASH, bash, arpen );
    d.add_damage( damage_type::CUT, cut, arpen );
    d.add_damage( damage_type::STAB, stab, arpen );
    return d;
}
damage_instance::damage_instance( damage_type dt, float amt, float arpen, float arpen_mult,
                                  float dmg_mult, float unc_arpen_mult, float unc_dmg_mult )
{
    add_damage( dt, amt, arpen, arpen_mult, dmg_mult, unc_arpen_mult, unc_dmg_mult );
}

void damage_instance::add_damage( damage_type dt, float amt, float arpen, float arpen_mult,
                                  float dmg_mult, float unc_arpen_mult, float unc_dmg_mult )
{
    damage_unit du( dt, amt, arpen, arpen_mult, dmg_mult, unc_arpen_mult, unc_dmg_mult );
    add( du );
}

void damage_instance::mult_damage( double multiplier, bool pre_armor )
{
    if( multiplier <= 0.0 ) {
        clear();
    }

    if( pre_armor ) {
        for( damage_unit &elem : damage_units ) {
            elem.amount *= multiplier;
        }
    } else {
        for( damage_unit &elem : damage_units ) {
            elem.damage_multiplier *= multiplier;
        }
    }
}

void damage_instance::mult_type_damage( double multiplier, damage_type dt )
{
    for( damage_unit &elem : damage_units ) {
        if( elem.type == dt ) {
            elem.damage_multiplier *= multiplier;
        }
    }
}

float damage_instance::type_damage( damage_type dt ) const
{
    float ret = 0.0f;
    for( const damage_unit &elem : damage_units ) {
        if( elem.type == dt ) {
            ret += elem.amount * elem.damage_multiplier * elem.unconditional_damage_mult;
        }
    }
    return ret;
}

float damage_instance::type_arpen( damage_type dt ) const
{
    float ret = 0.0f;
    for( const damage_unit &elem : damage_units ) {
        if( elem.type == dt ) {
            ret += elem.res_pen;
        }
    }
    return ret;
}

//This returns the damage from this damage_instance. The damage done to the target will be reduced by their armor.
float damage_instance::total_damage() const
{
    float ret = 0.0f;
    for( const damage_unit &elem : damage_units ) {
        ret += elem.amount * elem.damage_multiplier * elem.unconditional_damage_mult;
    }
    return ret;
}

//This returns the damage from this damage_instance. The damage done to the target will be reduced by their armor.
damage_instance damage_instance::di_considering_length( units::length barrel_length ) const
{

    if( barrel_length == 0_mm ) {
        return *this;
    }

    damage_instance di = damage_instance();
    di.damage_units = this->damage_units;
    for( damage_unit &du : di.damage_units ) {
        if( !du.barrels.empty() ) {
            std::vector<std::pair<float, float>> lerp_points;
            for( const barrel_desc &b : du.barrels ) {
                lerp_points.emplace_back( static_cast<float>( b.barrel_length.value() ), b.amount );
            }
            du.amount = multi_lerp( lerp_points, barrel_length.value() );
        }
    }
    return di;
}
void damage_instance::clear()
{
    damage_units.clear();
}

bool damage_instance::empty() const
{
    return damage_units.empty();
}

void damage_instance::add( const damage_instance &added_di )
{
    for( const damage_unit &added_du : added_di.damage_units ) {
        add( added_du );
    }
}

void damage_instance::add( const damage_unit &added_du )
{
    auto iter = std::find_if( damage_units.begin(), damage_units.end(),
    [&added_du]( const damage_unit & du ) {
        return du.type == added_du.type;
    } );
    if( iter == damage_units.end() ) {
        damage_units.emplace_back( added_du );
    } else {
        damage_unit &du = *iter;
        float mult = added_du.damage_multiplier / du.damage_multiplier;
        du.amount += added_du.amount * mult;
        du.res_pen += added_du.res_pen * mult;
        // Linearly interpolate armor multiplier based on damage proportion contributed
        float t = added_du.damage_multiplier / ( added_du.damage_multiplier + du.damage_multiplier );
        du.res_mult = lerp( du.res_mult, added_du.damage_multiplier, t );

        du.unconditional_res_mult *= added_du.unconditional_res_mult;
        du.unconditional_damage_mult *= added_du.unconditional_damage_mult;

        if( added_du.barrels.empty() ) {
            if( du.barrels.empty() ) {
                du.barrels = added_du.barrels;
            } else {
                debugmsg( "Tried to add two sets of barrel definitions for damage together, this is not currently supported." );
            }
        }
    }
}

std::vector<damage_unit>::iterator damage_instance::begin()
{
    return damage_units.begin();
}

std::vector<damage_unit>::const_iterator damage_instance::begin() const
{
    return damage_units.begin();
}

std::vector<damage_unit>::iterator damage_instance::end()
{
    return damage_units.end();
}

std::vector<damage_unit>::const_iterator damage_instance::end() const
{
    return damage_units.end();
}

bool damage_instance::operator==( const damage_instance &other ) const
{
    return damage_units == other.damage_units;
}

void damage_instance::deserialize( const JsonValue &jsin )
{
    // TODO: Clean up
    if( jsin.test_object() ) {
        JsonObject jo = jsin.get_object();
        damage_units = load_damage_instance( jo ).damage_units;
    } else if( jsin.test_array() ) {
        damage_units = load_damage_instance( jsin.get_array() ).damage_units;
    } else {
        jsin.throw_error( "Expected object or array for damage_instance" );
    }
}

dealt_damage_instance::dealt_damage_instance()
{
    dealt_dams.fill( 0 );
    bp_hit  = bodypart_id( "torso" );
}

void dealt_damage_instance::set_damage( damage_type dt, int amount )
{
    if( static_cast<int>( dt ) < 0 || dt >= damage_type::NUM ) {
        debugmsg( "Tried to set invalid damage type %d. damage_type::NUM is %d", dt, damage_type::NUM );
        return;
    }

    dealt_dams[static_cast<int>( dt )] = amount;
}
int dealt_damage_instance::type_damage( damage_type dt ) const
{
    if( static_cast<size_t>( dt ) < dealt_dams.size() ) {
        return dealt_dams[static_cast<int>( dt )];
    }

    return 0;
}
int dealt_damage_instance::total_damage() const
{
    return std::accumulate( dealt_dams.begin(), dealt_dams.end(), 0 );
}

resistances::resistances()
{
    resist_vals.fill( 0 );
}

resistances::resistances( const item &armor, bool to_self, int roll, const bodypart_id &bp )
{
    // Armors protect, but all items can resist
    if( to_self || armor.is_armor() || armor.is_pet_armor() ) {
        for( int i = 0; i < static_cast<int>( damage_type::NUM ); i++ ) {
            damage_type dt = static_cast<damage_type>( i );
            set_resist( dt, armor.resist( dt, to_self, bp, roll ) );
        }
    }
}

resistances::resistances( const item &armor, bool to_self, int roll, const sub_bodypart_id &bp )
{
    // Armors protect, but all items can resist
    if( to_self || armor.is_armor() ) {
        for( int i = 0; i < static_cast<int>( damage_type::NUM ); i++ ) {
            damage_type dt = static_cast<damage_type>( i );
            set_resist( dt, armor.resist( dt, to_self, bp, roll ) );
        }
    }
}

resistances::resistances( monster &monster ) : resistances()
{
    set_resist( damage_type::BASH, monster.type->armor_bash );
    set_resist( damage_type::CUT,  monster.type->armor_cut );
    set_resist( damage_type::STAB, monster.type->armor_stab );
    set_resist( damage_type::BULLET, monster.type->armor_bullet );
    set_resist( damage_type::ACID, monster.type->armor_acid );
    set_resist( damage_type::HEAT, monster.type->armor_fire );
    set_resist( damage_type::COLD, monster.type->armor_cold );
    set_resist( damage_type::PURE, monster.type->armor_pure );
    set_resist( damage_type::BIOLOGICAL, monster.type->armor_biological );
    set_resist( damage_type::ELECTRIC, monster.type->armor_elec );
}
void resistances::set_resist( damage_type dt, float amount )
{
    resist_vals[static_cast<int>( dt )] = amount;
}
float resistances::type_resist( damage_type dt ) const
{
    return resist_vals[static_cast<int>( dt )];
}
float resistances::get_effective_resist( const damage_unit &du ) const
{
    return std::max( type_resist( du.type ) - du.res_pen,
                     0.0f ) * du.res_mult * du.unconditional_res_mult;
}

resistances &resistances::operator+=( const resistances &other )
{
    for( size_t i = 0; i < static_cast<int>( damage_type::NUM ); i++ ) {
        resist_vals[ i ] += other.resist_vals[ i ];
    }

    return *this;
}

bool resistances::operator==( const resistances &other )
{
    for( size_t i = 0; i < static_cast<int>( damage_type::NUM ); i++ ) {
        if( resist_vals[i] != other.resist_vals[i] ) {
            return false;
        }
    }

    return true;
}

resistances resistances::operator*( float mod ) const
{
    resistances ret;
    for( size_t i = 0; i < static_cast<int>( damage_type::NUM ); i++ ) {
        ret.resist_vals[ i ] = this->resist_vals[ i ] * mod;
    }
    return ret;
}

resistances resistances::operator/( float mod ) const
{
    resistances ret;
    for( size_t i = 0; i < static_cast<int>( damage_type::NUM ); i++ ) {
        ret.resist_vals[i] = this->resist_vals[i] / mod;
    }
    return ret;
}

static const std::map<std::string, damage_type> dt_map = {
    { translate_marker_context( "damage type", "pure" ), damage_type::PURE },
    { translate_marker_context( "damage type", "biological" ), damage_type::BIOLOGICAL },
    { translate_marker_context( "damage type", "bash" ), damage_type::BASH },
    { translate_marker_context( "damage type", "cut" ), damage_type::CUT },
    { translate_marker_context( "damage type", "acid" ), damage_type::ACID },
    { translate_marker_context( "damage type", "stab" ), damage_type::STAB },
    { translate_marker_context( "damage_type", "bullet" ), damage_type::BULLET },
    { translate_marker_context( "damage type", "heat" ), damage_type::HEAT },
    { translate_marker_context( "damage type", "cold" ), damage_type::COLD },
    { translate_marker_context( "damage type", "electric" ), damage_type::ELECTRIC }
};

const std::map<std::string, damage_type> &get_dt_map()
{
    return dt_map;
}

damage_type dt_by_name( const std::string &name )
{
    const auto &iter = dt_map.find( name );
    if( iter == dt_map.end() ) {
        return damage_type::NONE;
    }

    return iter->second;
}

std::string name_by_dt( const damage_type &dt )
{
    auto iter = dt_map.cbegin();
    while( iter != dt_map.cend() ) {
        if( iter->second == dt ) {
            return pgettext( "damage type", iter->first.c_str() );
        }
        iter++;
    }
    static const std::string err_msg( "dt_not_found" );
    return err_msg;
}

const skill_id &skill_by_dt( damage_type dt )
{
    static skill_id skill_bashing( "bashing" );
    static skill_id skill_cutting( "cutting" );
    static skill_id skill_stabbing( "stabbing" );

    switch( dt ) {
        case damage_type::BASH:
            return skill_bashing;

        case damage_type::CUT:
            return skill_cutting;

        case damage_type::STAB:
            return skill_stabbing;

        default:
            return skill_id::NULL_ID();
    }
}

static damage_unit load_damage_unit( const JsonObject &curr )
{
    damage_type dt = dt_by_name( curr.get_string( "damage_type" ) );
    if( dt == damage_type::NONE ) {
        curr.throw_error( "Invalid damage type" );
    }

    float amount = curr.get_float( "amount", 0 );
    float arpen = curr.get_float( "armor_penetration", 0 );
    float armor_mul = curr.get_float( "armor_multiplier", 1.0f );
    float damage_mul = curr.get_float( "damage_multiplier", 1.0f );

    float unc_armor_mul = curr.get_float( "constant_armor_multiplier", 1.0f );
    float unc_damage_mul = curr.get_float( "constant_damage_multiplier", 1.0f );

    damage_unit du( dt, amount, arpen, armor_mul, damage_mul, unc_armor_mul,
                    unc_damage_mul );

    optional( curr, false, "barrels", du.barrels );

    return du;
}

static damage_unit load_damage_unit_inherit( const JsonObject &curr, const damage_instance &parent )
{
    damage_unit ret = load_damage_unit( curr );

    const std::vector<damage_unit> &parent_damage = parent.damage_units;
    auto du_iter = std::find_if( parent_damage.begin(),
    parent_damage.end(), [&ret]( const damage_unit & dmg ) {
        return dmg.type == ret.type;
    } );

    if( du_iter == parent_damage.end() ) {
        return ret;
    }

    const damage_unit &parent_du = *du_iter;

    if( !curr.has_float( "amount" ) ) {
        ret.amount = parent_du.amount;
    }
    if( !curr.has_float( "armor_penetration" ) ) {
        ret.res_pen = parent_du.res_pen;
    }
    if( !curr.has_float( "armor_multiplier" ) ) {
        ret.res_mult = parent_du.res_mult;
    }
    if( !curr.has_float( "damage_multiplier" ) ) {
        ret.damage_multiplier = parent_du.damage_multiplier;
    }
    if( !curr.has_float( "constant_armor_multiplier" ) ) {
        ret.unconditional_res_mult = parent_du.unconditional_res_mult;
    }
    if( !curr.has_float( "constant_damage_multiplier" ) ) {
        ret.unconditional_damage_mult = parent_du.unconditional_damage_mult;
    }
    if( !curr.has_array( "barrels" ) ) {
        ret.barrels = parent_du.barrels;
    }

    return ret;
}

static damage_instance blank_damage_instance()
{
    damage_instance ret;

    for( int i = 0; i < static_cast<int>( damage_type::NUM ); ++i ) {
        ret.add_damage( static_cast<damage_type>( i ), 0.0f );
    }

    return ret;
}

damage_instance load_damage_instance( const JsonObject &jo )
{
    return load_damage_instance_inherit( jo, blank_damage_instance() );
}

damage_instance load_damage_instance( const JsonArray &jarr )
{
    return load_damage_instance_inherit( jarr, blank_damage_instance() );
}

damage_instance load_damage_instance_inherit( const JsonObject &jo, const damage_instance &parent )
{
    damage_instance di;
    if( jo.has_array( "values" ) ) {
        for( const JsonObject curr : jo.get_array( "values" ) ) {
            di.damage_units.push_back( load_damage_unit_inherit( curr, parent ) );
        }
    } else if( jo.has_string( "damage_type" ) ) {
        di.damage_units.push_back( load_damage_unit_inherit( jo, parent ) );
    }

    return di;
}

damage_instance load_damage_instance_inherit( const JsonArray &jarr, const damage_instance &parent )
{
    damage_instance di;
    for( const JsonObject curr : jarr ) {
        di.damage_units.push_back( load_damage_unit_inherit( curr, parent ) );
    }

    return di;
}

std::array<float, static_cast<int>( damage_type::NUM )> load_damage_array( const JsonObject &jo,
        float default_value )
{
    std::array<float, static_cast<int>( damage_type::NUM )> ret;
    float init_val = jo.get_float( "all", default_value );

    float phys = jo.get_float( "physical", init_val );
    ret[ static_cast<int>( damage_type::BASH ) ] = jo.get_float( "bash", phys );
    ret[ static_cast<int>( damage_type::CUT )] = jo.get_float( "cut", phys );
    ret[ static_cast<int>( damage_type::STAB ) ] = jo.get_float( "stab", phys );
    ret[ static_cast<int>( damage_type::BULLET ) ] = jo.get_float( "bullet", phys );

    float non_phys = jo.get_float( "non_physical", init_val );
    ret[ static_cast<int>( damage_type::BIOLOGICAL ) ] = jo.get_float( "biological", non_phys );
    ret[ static_cast<int>( damage_type::ACID ) ] = jo.get_float( "acid", non_phys );
    ret[ static_cast<int>( damage_type::HEAT ) ] = jo.get_float( "heat", non_phys );
    ret[ static_cast<int>( damage_type::COLD ) ] = jo.get_float( "cold", non_phys );
    ret[ static_cast<int>( damage_type::ELECTRIC ) ] = jo.get_float( "electric", non_phys );

    // damage_type::PURE should never be resisted
    ret[ static_cast<int>( damage_type::PURE ) ] = 0.0f;
    return ret;
}

resistances load_resistances_instance( const JsonObject &jo )
{
    resistances ret;
    ret.resist_vals = load_damage_array( jo );
    return ret;
}

void damage_over_time_data::load( const JsonObject &obj )
{
    std::string tmp_string;
    mandatory( obj, was_loaded, "damage_type", tmp_string );
    type = dt_by_name( tmp_string );
    mandatory( obj, was_loaded, "amount", amount );
    mandatory( obj, was_loaded, "bodyparts", bps );

    if( obj.has_string( "duration" ) ) {
        duration = read_from_json_string<time_duration>( obj.get_member( "duration" ),
                   time_duration::units );
    } else {
        duration = time_duration::from_turns( obj.get_int( "duration", 0 ) );
    }
}

void damage_over_time_data::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "damage_type", name_by_dt( type ) );
    jsout.member( "duration", duration );
    jsout.member( "amount", amount );
    jsout.member( "bodyparts", bps );
    jsout.end_object();
}

void damage_over_time_data::deserialize( const JsonObject &jo )
{
    std::string tmp_string = jo.get_string( "damage_type" );
    // Remove after 0.F, migrating DT_TRUE to DT_PURE
    if( tmp_string == "true" ) {
        tmp_string = "pure";
    }
    type = dt_by_name( tmp_string );
    jo.read( "amount", amount );
    jo.read( "duration", duration );
    jo.read( "bodyparts", bps );
}

barrel_desc::barrel_desc() = default;

void barrel_desc::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "barrel_length", barrel_length );
    mandatory( jo, false, "amount", amount );
}
