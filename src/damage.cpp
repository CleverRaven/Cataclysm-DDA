#include "damage.h"

#include <algorithm>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <utility>

#include "bodypart.h"
#include "cata_utility.h"
#include "creature.h"
#include "debug.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "flexbuffer_json-inl.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "init.h"
#include "item.h"
#include "json.h"
#include "json_error.h"
#include "make_static.h"
#include "monster.h"
#include "mtype.h"
#include "subbodypart.h"
#include "talker.h"
#include "units.h"

static std::map<damage_info_order::info_type, std::vector<damage_info_order>> sorted_order_lists;

namespace
{

generic_factory<damage_type> damage_type_factory( "damage type" );
generic_factory<damage_info_order> damage_info_order_factory( "damage info order" );

} // namespace

/** @relates string_id */
template<>
const damage_type &string_id<damage_type>::obj() const
{
    return damage_type_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<damage_type>::is_valid() const
{
    return damage_type_factory.is_valid( *this );
}

/** @relates string_id */
template<>
const damage_info_order &string_id<damage_info_order>::obj() const
{
    return damage_info_order_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<damage_info_order>::is_valid() const
{
    return damage_info_order_factory.is_valid( *this );
}

void damage_type::load_damage_types( const JsonObject &jo, const std::string &src )
{
    damage_type_factory.load( jo, src );
}

void damage_type::reset()
{
    damage_type_factory.reset();
}

const std::vector<damage_type> &damage_type::get_all()
{
    return damage_type_factory.get_all();
}

void damage_info_order::load_damage_info_orders( const JsonObject &jo, const std::string &src )
{
    damage_info_order_factory.load( jo, src );
}

void damage_info_order::reset()
{
    damage_info_order_factory.reset();
    sorted_order_lists.clear();
}

const std::vector<damage_info_order> &damage_info_order::get_all()
{
    return damage_info_order_factory.get_all();
}

const std::vector<damage_info_order> &damage_info_order::get_all( info_type sort_by )
{
    return sorted_order_lists.at( sort_by );
}

static damage_info_order::info_disp read_info_disp( const std::string &s )
{
    if( s == "detailed" ) {
        return damage_info_order::info_disp::DETAILED;
    } else if( s == "basic" ) {
        return damage_info_order::info_disp::BASIC;
    } else {
        return damage_info_order::info_disp::NONE;
    }
}

void damage_type::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "name", name );
    optional( jo, was_loaded, "skill", skill, skill_id::NULL_ID() );
    optional( jo, was_loaded, "physical", physical );
    optional( jo, was_loaded, "melee_only", melee_only );
    optional( jo, was_loaded, "edged", edged );
    optional( jo, was_loaded, "environmental", env );
    optional( jo, was_loaded, "material_required", material_required );
    optional( jo, was_loaded, "mon_difficulty", mon_difficulty );
    optional( jo, was_loaded, "no_resist", no_resist );
    if( jo.has_object( "immune_flags" ) ) {
        JsonObject jsobj = jo.get_object( "immune_flags" );
        if( jsobj.has_array( "monster" ) ) {
            mon_immune_flags.clear();
            for( const std::string flg : jsobj.get_array( "monster" ) ) {
                mon_immune_flags.insert( flg );
            }
        }
        if( jsobj.has_array( "character" ) ) {
            immune_flags.clear();
            for( const std::string flg : jsobj.get_array( "character" ) ) {
                immune_flags.insert( flg );
            }
        }
    }

    if( !was_loaded || jo.has_string( "magic_color" ) ) {
        std::string color_str;
        optional( jo, was_loaded, "magic_color", color_str, "black" );
        magic_color = color_from_string( color_str );
    }

    if( jo.has_array( "derived_from" ) ) {
        JsonArray ja = jo.get_array( "derived_from" );
        derived_from = { damage_type_id( ja.get_string( 0 ) ), static_cast<float>( ja.get_float( 1 ) ) };
    }

    for( JsonValue jv : jo.get_array( "onhit_eocs" ) ) {
        onhit_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, "" ) );
    }

    for( JsonValue jv : jo.get_array( "ondamage_eocs" ) ) {
        ondamage_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, "" ) );
    }
}

void damage_type::check()
{
    for( const damage_type &dt : damage_type::get_all() ) {
        const std::vector<damage_info_order> &dio_list = damage_info_order::get_all();
        auto iter = std::find_if( dio_list.begin(), dio_list.end(),
        [&dt]( const damage_info_order & dio ) {
            return dt.id == dio.dmg_type;
        } );
        if( iter == dio_list.end() ) {
            debugmsg( "damage type %s has no associated damage_info_order type.", dt.id.c_str() );
        }
    }
}

void damage_info_order::damage_info_order_entry::load( const JsonObject &jo,
        std::string_view member )
{
    if( jo.has_object( member ) || jo.has_int( member ) ) {
        if( jo.has_int( member ) ) {
            show_type = true;
            order = jo.get_int( member );
        } else {
            JsonObject jsobj = jo.get_object( member );
            mandatory( jsobj, false, "order", order );
            optional( jsobj, false, "show_type", show_type, true );
        }
    }
}

void damage_info_order::load( const JsonObject &jo, std::string_view )
{
    dmg_type = damage_type_id( id.c_str() );
    bionic_info.load( jo, "bionic_info" );
    protection_info.load( jo, "protection_info" );
    pet_prot_info.load( jo, "pet_prot_info" );
    melee_combat_info.load( jo, "melee_combat_info" );
    ablative_info.load( jo, "ablative_info" );
    optional( jo, was_loaded, "verb", verb );

    if( !was_loaded || jo.has_string( "info_display" ) ) {
        std::string info_str = info_display == info_disp::DETAILED ? "detailed" :
                               info_display == info_disp::BASIC ? "basic" : "none";
        optional( jo, was_loaded, "info_display", info_str, "none" );
        info_display = read_info_disp( info_str );
    }
}

static void prepare_sorted_lists( std::vector<damage_info_order> &list,
                                  damage_info_order::info_type sb )
{
    for( auto iter = list.begin(); iter != list.end(); ) {
        bool erase = false;
        switch( sb ) {
            case damage_info_order::info_type::BIO:
                if( !iter->bionic_info.show_type ) {
                    erase = true;
                }
                break;
            case damage_info_order::info_type::PROT:
                if( !iter->protection_info.show_type ) {
                    erase = true;
                }
                break;
            case damage_info_order::info_type::PET:
                if( !iter->pet_prot_info.show_type ) {
                    erase = true;
                }
                break;
            case damage_info_order::info_type::MELEE:
                if( !iter->melee_combat_info.show_type ) {
                    erase = true;
                }
                break;
            case damage_info_order::info_type::ABLATE:
                if( !iter->ablative_info.show_type ) {
                    erase = true;
                }
                break;
            case damage_info_order::info_type::NONE:
            default:
                break;
        }
        if( erase ) {
            iter = list.erase( iter );
        } else {
            iter++;
        }
    }
    if( sb == damage_info_order::info_type::NONE ) {
        return;
    }
    std::sort( list.begin(), list.end(),
    [sb]( const damage_info_order & a, const damage_info_order & b ) {
        switch( sb ) {
            case damage_info_order::info_type::BIO:
                return a.bionic_info.order < b.bionic_info.order;
            case damage_info_order::info_type::PROT:
                return a.protection_info.order < b.protection_info.order;
            case damage_info_order::info_type::PET:
                return a.pet_prot_info.order < b.pet_prot_info.order;
            case damage_info_order::info_type::MELEE:
                return a.melee_combat_info.order < b.melee_combat_info.order;
            case damage_info_order::info_type::ABLATE:
                return a.ablative_info.order < b.ablative_info.order;
            case damage_info_order::info_type::NONE:
            default:
                return false;
        }
    } );
}

void damage_info_order::finalize_all()
{
    damage_info_order_factory.finalize();
    sorted_order_lists.emplace( info_type::NONE, damage_info_order_factory.get_all() );
    sorted_order_lists.emplace( info_type::BIO, damage_info_order_factory.get_all() );
    sorted_order_lists.emplace( info_type::PROT, damage_info_order_factory.get_all() );
    sorted_order_lists.emplace( info_type::PET, damage_info_order_factory.get_all() );
    sorted_order_lists.emplace( info_type::MELEE, damage_info_order_factory.get_all() );
    sorted_order_lists.emplace( info_type::ABLATE, damage_info_order_factory.get_all() );
    prepare_sorted_lists( sorted_order_lists[info_type::NONE], info_type::NONE );
    prepare_sorted_lists( sorted_order_lists[info_type::BIO], info_type::BIO );
    prepare_sorted_lists( sorted_order_lists[info_type::PROT], info_type::PROT );
    prepare_sorted_lists( sorted_order_lists[info_type::PET], info_type::PET );
    prepare_sorted_lists( sorted_order_lists[info_type::MELEE], info_type::MELEE );
    prepare_sorted_lists( sorted_order_lists[info_type::ABLATE], info_type::ABLATE );
}

void damage_info_order::finalize()
{
    if( verb.empty() ) {
        verb = dmg_type->name;
    }
}

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

damage_instance::damage_instance( const damage_type_id &dt, float amt, float arpen,
                                  float arpen_mult, float dmg_mult, float unc_arpen_mult, float unc_dmg_mult )
{
    add_damage( dt, amt, arpen, arpen_mult, dmg_mult, unc_arpen_mult, unc_dmg_mult );
}

void damage_instance::add_damage( const damage_type_id &dt, float amt, float arpen,
                                  float arpen_mult, float dmg_mult, float unc_arpen_mult, float unc_dmg_mult )
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

void damage_instance::mult_type_damage( double multiplier, const damage_type_id &dt )
{
    for( damage_unit &elem : damage_units ) {
        if( elem.type == dt ) {
            elem.damage_multiplier *= multiplier;
        }
    }
}

float damage_instance::type_damage( const damage_type_id &dt ) const
{
    float ret = 0.0f;
    for( const damage_unit &elem : damage_units ) {
        if( elem.type == dt ) {
            ret += elem.amount * elem.damage_multiplier * elem.unconditional_damage_mult;
        }
    }
    return ret;
}

float damage_instance::type_arpen( const damage_type_id &dt ) const
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

void damage_instance::onhit_effects( Creature *source, Creature *target ) const
{
    std::set<damage_type_id> used_types;
    for( const damage_unit &du : damage_units ) {
        if( used_types.count( du.type ) > 0 ) {
            continue;
        }
        used_types.emplace( du.type );
        du.type->onhit_effects( source, target );
    }
}

void damage_type::onhit_effects( Creature *source, Creature *target ) const
{
    for( const effect_on_condition_id &eoc : onhit_eocs ) {
        dialogue d( source == nullptr ? nullptr : get_talker_for( source ),
                    target == nullptr ? nullptr : get_talker_for( target ) );

        if( eoc->type == eoc_type::ACTIVATION ) {
            eoc->activate( d );
        } else {
            debugmsg( "Must use an activation eoc for a damage type effect.  If you don't want the effect_on_condition to happen on its own (without the damage type effect being activated), remove the recurrence min and max.  Otherwise, create a non-recurring effect_on_condition for this damage type with its condition and effects, then have a recurring one queue it." );
        }
    }
}

void damage_instance::ondamage_effects( Creature *source, Creature *target,
                                        const damage_instance &premitigated, bodypart_str_id bp ) const
{
    std::set<damage_type_id> used_types;
    for( const damage_unit &du : damage_units ) {
        if( used_types.count( du.type ) > 0 ) {
            continue;
        }
        used_types.emplace( du.type );

        // get the original damage
        auto predamageunit = std::find_if( premitigated.begin(),
        premitigated.end(), [du]( const damage_unit & dut ) {
            return dut.type == du.type;
        } );

        double premit = 0.0;
        if( predamageunit != premitigated.end() ) {
            premit = predamageunit->amount;
        }
        if( !target->is_immune_damage( du.type ) ) {
            du.type->ondamage_effects( source, target, bp, premit, du.amount );
        }
    }
}

void damage_type::ondamage_effects( Creature *source, Creature *target, bodypart_str_id bp,
                                    double total_damage, double damage_taken ) const
{
    for( const effect_on_condition_id &eoc : ondamage_eocs ) {
        dialogue d( source == nullptr ? nullptr : get_talker_for( source ),
                    target == nullptr ? nullptr : get_talker_for( target ) );

        d.set_value( "npctalk_var_damage_taken", std::to_string( damage_taken ) );
        d.set_value( "npctalk_var_total_damage", std::to_string( total_damage ) );
        d.set_value( "npctalk_var_bp", bp.str() );

        if( eoc->type == eoc_type::ACTIVATION ) {
            eoc->activate( d );
        } else {
            debugmsg( "Must use an activation eoc for a damage type effect.  If you don't want the effect_on_condition to happen on its own (without the damage type effect being activated), remove the recurrence min and max.  Otherwise, create a non-recurring effect_on_condition for this damage type with its condition and effects, then have a recurring one queue it." );
        }
    }
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
            lerp_points.reserve( du.barrels.size() );
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
    dealt_dams.clear();
    for( const damage_type &dt : damage_type::get_all() ) {
        dealt_dams.emplace( dt.id, 0 );
    }
    bp_hit  = bodypart_id( "torso" );
}

void dealt_damage_instance::set_damage( const damage_type_id &dt, int amount )
{
    if( !dt.is_valid() ) {
        debugmsg( "Tried to set invalid damage type %s.", dt.c_str() );
        return;
    }

    dealt_dams[dt] = amount;
}
int dealt_damage_instance::type_damage( const damage_type_id &dt ) const
{
    auto iter = dealt_dams.find( dt );
    if( iter != dealt_dams.end() ) {
        return iter->second;
    }

    return 0;
}
int dealt_damage_instance::total_damage() const
{
    return std::accumulate( dealt_dams.begin(), dealt_dams.end(), 0,
    []( int acc, const std::pair<const damage_type_id, int> &dmg ) {
        return acc + dmg.second;
    } );
}

resistances::resistances( const item &armor, bool to_self, int roll, const bodypart_id &bp )
{
    // Armors protect, but all items can resist
    if( to_self || armor.is_armor() || armor.is_pet_armor() ) {
        for( const damage_type &dam : damage_type::get_all() ) {
            set_resist( dam.id, armor.resist( dam.id, to_self, bp, roll ) );
        }
    }
}

resistances::resistances( const item &armor, bool to_self, int roll, const sub_bodypart_id &bp )
{
    // Armors protect, but all items can resist
    if( to_self || armor.is_armor() ) {
        for( const damage_type &dam : damage_type::get_all() ) {
            set_resist( dam.id, armor.resist( dam.id, to_self, bp, roll ) );
        }
    }
}

resistances::resistances( monster &monster ) : resistances( monster.type->armor )
{

}
void resistances::set_resist( const damage_type_id &dt, float amount )
{
    resist_vals[dt] = amount;
}
float resistances::type_resist( const damage_type_id &dt ) const
{
    auto iter = resist_vals.find( dt );
    return ( iter == resist_vals.end() || iter->first->no_resist ) ? 0.0f : iter->second;
}
float resistances::get_effective_resist( const damage_unit &du ) const
{
    return std::max( type_resist( du.type ) - du.res_pen,
                     0.0f ) * du.res_mult * du.unconditional_res_mult;
}

bool resistances::operator==( const resistances &other )
{
    for( const auto &dam : other.resist_vals ) {
        if( resist_vals.count( dam.first ) <= 0 || resist_vals[dam.first] != dam.second ) {
            return false;
        }
    }

    return true;
}

resistances resistances::operator*( float mod ) const
{
    resistances ret;
    for( const auto &dam : resist_vals ) {
        ret.resist_vals[dam.first] = dam.second * mod;
    }
    return ret;
}

resistances resistances::operator/( float mod ) const
{
    resistances ret;
    for( const auto &dam : resist_vals ) {
        ret.resist_vals[dam.first] = dam.second / mod;
    }
    return ret;
}

static damage_unit load_damage_unit( const JsonObject &curr )
{
    damage_type_id dt( curr.get_string( "damage_type" ) );

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

    for( const damage_type &dam : damage_type::get_all() ) {
        ret.add_damage( dam.id, 0.0f );
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

std::unordered_map<damage_type_id, float> load_damage_map( const JsonObject &jo,
        const std::set<std::string> &ignored_keys )
{
    std::unordered_map<damage_type_id, float> ret;
    for( const JsonMember &jmemb : jo ) {
        if( !ignored_keys.empty() && ignored_keys.count( jmemb.name() ) > 0 ) {
            continue;
        }
        ret[damage_type_id( jmemb.name() )] = jmemb.get_float();
    }
    return ret;
}

void finalize_damage_map( std::unordered_map<damage_type_id, float> &damage_map, bool force_derive,
                          float default_value )
{
    const std::vector<damage_type> &dams = damage_type::get_all();
    if( dams.empty() ) {
        debugmsg( "called before damage_types have been loaded." );
        return;
    }

    auto get_and_erase = [&damage_map]( const damage_type_id & did, float def_val ) {
        auto iter = damage_map.find( did );
        if( iter == damage_map.end() ) {
            return def_val;
        }
        float val = iter->second;
        damage_map.erase( iter );
        return val;
    };

    const float all = get_and_erase( STATIC( damage_type_id( "all" ) ), default_value );
    const float physical = get_and_erase( STATIC( damage_type_id( "physical" ) ), all );
    const float non_phys = get_and_erase( STATIC( damage_type_id( "non_physical" ) ), all );

    std::vector<damage_type_id> to_derive;
    for( const damage_type &dam : dams ) {
        float val = dam.physical ? physical : non_phys;
        if( damage_map.count( dam.id ) > 0 ) {
            val = damage_map.at( dam.id );
        } else if( force_derive && !dam.derived_from.first.is_null() ) {
            to_derive.emplace_back( dam.id );
            continue;
        }
        damage_map[dam.id] = val;
    }

    for( auto &td : to_derive ) {
        auto iter = damage_map.find( td->derived_from.first );
        damage_map[td] = iter == damage_map.end() ? ( td->physical ? physical : non_phys ) :
                         iter->second * td->derived_from.second;
    }
}

resistances extend_resistances_instance( resistances ret, const JsonObject &jo )
{
    resistances ext;
    ext.resist_vals = load_damage_map( jo );
    for( const std::pair<const damage_type_id, float> &damage_pair : ext.resist_vals ) {
        ret.resist_vals[damage_pair.first] += damage_pair.second;
    }
    return ret;
}

resistances load_resistances_instance( const JsonObject &jo,
                                       const std::set<std::string> &ignored_keys )
{
    resistances ret;
    ret.resist_vals = load_damage_map( jo, ignored_keys );
    return ret;
}

void damage_over_time_data::load( const JsonObject &obj )
{
    mandatory( obj, was_loaded, "damage_type", type );
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
    jsout.member( "damage_type", type );
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
    type = damage_type_id( tmp_string );
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
