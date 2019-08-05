#include "damage.h"

#include <cstddef>
#include <algorithm>
#include <map>
#include <numeric>
#include <utility>

#include "debug.h"
#include "item.h"
#include "json.h"
#include "monster.h"
#include "mtype.h"
#include "translations.h"
#include "cata_utility.h"

bool damage_unit::operator==( const damage_unit &other ) const
{
    return type == other.type &&
           amount == other.amount &&
           res_pen == other.res_pen &&
           res_mult == other.res_mult &&
           damage_multiplier == other.damage_multiplier;
}

damage_instance::damage_instance() = default;
damage_instance damage_instance::physical( float bash, float cut, float stab, float arpen )
{
    damage_instance d;
    d.add_damage( DT_BASH, bash, arpen );
    d.add_damage( DT_CUT, cut, arpen );
    d.add_damage( DT_STAB, stab, arpen );
    return d;
}
damage_instance::damage_instance( damage_type dt, float a, float rp, float rm, float mul )
{
    add_damage( dt, a, rp, rm, mul );
}

void damage_instance::add_damage( damage_type dt, float a, float rp, float rm, float mul )
{
    damage_unit du( dt, a, rp, rm, mul );
    add( du );
}

void damage_instance::mult_damage( double multiplier, bool pre_armor )
{
    if( multiplier <= 0.0 ) {
        clear();
    }

    if( pre_armor ) {
        for( auto &elem : damage_units ) {
            elem.amount *= multiplier;
        }
    } else {
        for( auto &elem : damage_units ) {
            elem.damage_multiplier *= multiplier;
        }
    }
}
float damage_instance::type_damage( damage_type dt ) const
{
    float ret = 0;
    for( const auto &elem : damage_units ) {
        if( elem.type == dt ) {
            ret += elem.amount * elem.damage_multiplier;
        }
    }
    return ret;
}
//This returns the damage from this damage_instance. The damage done to the target will be reduced by their armor.
float damage_instance::total_damage() const
{
    float ret = 0;
    for( const auto &elem : damage_units ) {
        ret += elem.amount * elem.damage_multiplier;
    }
    return ret;
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
    for( auto &added_du : added_di.damage_units ) {
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

void damage_instance::deserialize( JsonIn &jsin )
{
    JsonObject jo( jsin );
    // TODO: Clean up
    damage_units = load_damage_instance( jo ).damage_units;
}

dealt_damage_instance::dealt_damage_instance()
{
    dealt_dams.fill( 0 );
}

void dealt_damage_instance::set_damage( damage_type dt, int amount )
{
    if( dt < 0 || dt >= NUM_DT ) {
        debugmsg( "Tried to set invalid damage type %d. NUM_DT is %d", dt, NUM_DT );
        return;
    }

    dealt_dams[dt] = amount;
}
int dealt_damage_instance::type_damage( damage_type dt ) const
{
    if( static_cast<size_t>( dt ) < dealt_dams.size() ) {
        return dealt_dams[dt];
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

resistances::resistances( const item &armor, bool to_self )
{
    // Armors protect, but all items can resist
    if( to_self || armor.is_armor() ) {
        for( int i = 0; i < NUM_DT; i++ ) {
            damage_type dt = static_cast<damage_type>( i );
            set_resist( dt, armor.damage_resist( dt, to_self ) );
        }
    }
}
resistances::resistances( monster &monster )
{
    set_resist( DT_BASH, monster.type->armor_bash );
    set_resist( DT_CUT,  monster.type->armor_cut );
    set_resist( DT_STAB, monster.type->armor_stab );
    set_resist( DT_ACID, monster.type->armor_acid );
    set_resist( DT_HEAT, monster.type->armor_fire );
}
void resistances::set_resist( damage_type dt, float amount )
{
    resist_vals[dt] = amount;
}
float resistances::type_resist( damage_type dt ) const
{
    return resist_vals[dt];
}
float resistances::get_effective_resist( const damage_unit &du ) const
{
    return std::max( type_resist( du.type ) - du.res_pen, 0.0f ) * du.res_mult;
}

resistances &resistances::operator+=( const resistances &other )
{
    for( size_t i = 0; i < NUM_DT; i++ ) {
        resist_vals[ i ] += other.resist_vals[ i ];
    }

    return *this;
}

static const std::map<std::string, damage_type> dt_map = {
    { translate_marker_context( "damage type", "true" ), DT_TRUE },
    { translate_marker_context( "damage type", "biological" ), DT_BIOLOGICAL },
    { translate_marker_context( "damage type", "bash" ), DT_BASH },
    { translate_marker_context( "damage type", "cut" ), DT_CUT },
    { translate_marker_context( "damage type", "acid" ), DT_ACID },
    { translate_marker_context( "damage type", "stab" ), DT_STAB },
    { translate_marker_context( "damage type", "heat" ), DT_HEAT },
    { translate_marker_context( "damage type", "cold" ), DT_COLD },
    { translate_marker_context( "damage type", "electric" ), DT_ELECTRIC }
};

damage_type dt_by_name( const std::string &name )
{
    const auto &iter = dt_map.find( name );
    if( iter == dt_map.end() ) {
        return DT_NULL;
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
        case DT_BASH:
            return skill_bashing;

        case DT_CUT:
            return skill_cutting;

        case DT_STAB:
            return skill_stabbing;

        default:
            return skill_id::NULL_ID();
    }
}

static damage_unit load_damage_unit( JsonObject &curr )
{
    damage_type dt = dt_by_name( curr.get_string( "damage_type" ) );
    if( dt == DT_NULL ) {
        curr.throw_error( "Invalid damage type" );
    }

    float amount = curr.get_float( "amount" );
    int arpen = curr.get_int( "armor_penetration", 0 );
    float armor_mul = curr.get_float( "armor_multiplier", 1.0f );
    float damage_mul = curr.get_float( "damage_multiplier", 1.0f );
    return damage_unit( dt, amount, arpen, armor_mul, damage_mul );
}

damage_instance load_damage_instance( JsonObject &jo )
{
    damage_instance di;
    if( jo.has_array( "values" ) ) {
        JsonArray jarr = jo.get_array( "values" );
        while( jarr.has_more() ) {
            JsonObject curr = jarr.next_object();
            di.damage_units.push_back( load_damage_unit( curr ) );
        }
    } else if( jo.has_string( "damage_type" ) ) {
        di.damage_units.push_back( load_damage_unit( jo ) );
    }

    return di;
}

damage_instance load_damage_instance( JsonArray &jarr )
{
    damage_instance di;
    while( jarr.has_more() ) {
        JsonObject curr = jarr.next_object();
        di.damage_units.push_back( load_damage_unit( curr ) );
    }

    return di;
}

std::array<float, NUM_DT> load_damage_array( JsonObject &jo )
{
    std::array<float, NUM_DT> ret;
    float init_val = jo.get_float( "all", 0.0f );

    float phys = jo.get_float( "physical", init_val );
    ret[ DT_BASH ] = jo.get_float( "bash", phys );
    ret[ DT_CUT ] = jo.get_float( "cut", phys );
    ret[ DT_STAB ] = jo.get_float( "stab", phys );

    float non_phys = jo.get_float( "non_physical", init_val );
    ret[ DT_BIOLOGICAL ] = jo.get_float( "biological", non_phys );
    ret[ DT_ACID ] = jo.get_float( "acid", non_phys );
    ret[ DT_HEAT ] = jo.get_float( "heat", non_phys );
    ret[ DT_COLD ] = jo.get_float( "cold", non_phys );
    ret[ DT_ELECTRIC ] = jo.get_float( "electric", non_phys );

    // DT_TRUE should never be resisted
    ret[ DT_TRUE ] = 0.0f;
    return ret;
}

resistances load_resistances_instance( JsonObject &jo )
{
    resistances ret;
    ret.resist_vals = load_damage_array( jo );
    return ret;
}
