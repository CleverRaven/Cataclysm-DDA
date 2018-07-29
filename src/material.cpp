#include "material.h"

#include "debug.h"
#include "damage.h" // damage_type
#include "json.h"
#include "translations.h"
#include "generic_factory.h"
#include "item.h"

#include <string>
#include <map>

namespace
{

generic_factory<material_type> material_data( "material", "ident" );

} // namespace

/** @relates string_id */
template<>
bool string_id<material_type>::is_valid() const
{
    return material_data.is_valid( *this );
}

/** @relates string_id */
template<>
const material_type &string_id<material_type>::obj() const
{
    return material_data.obj( *this );
}

material_type::material_type() :
    id( material_id::NULL_ID() ),
    _bash_dmg_verb( translate_marker( "damages" ) ),
    _cut_dmg_verb( translate_marker( "damages" ) )
{
    _dmg_adj = { translate_marker( "lightly damaged" ), translate_marker( "damaged" ), translate_marker( "very damaged" ), translate_marker( "thoroughly damaged" ) };
}

mat_burn_data load_mat_burn_data( JsonObject &jsobj )
{
    mat_burn_data bd;
    assign( jsobj, "immune", bd.immune );
    assign( jsobj, "volume_penalty", bd.volume_per_turn );
    jsobj.read( "fuel", bd.fuel );
    jsobj.read( "smoke", bd.smoke );
    jsobj.read( "burn", bd.burn );
    return bd;
}

void material_type::load( JsonObject &jsobj, const std::string & )
{
    mandatory( jsobj, was_loaded, "name", _name );

    mandatory( jsobj, was_loaded, "bash_resist", _bash_resist );
    mandatory( jsobj, was_loaded, "cut_resist", _cut_resist );
    mandatory( jsobj, was_loaded, "acid_resist", _acid_resist );
    mandatory( jsobj, was_loaded, "elec_resist", _elec_resist );
    mandatory( jsobj, was_loaded, "fire_resist", _fire_resist );
    mandatory( jsobj, was_loaded, "chip_resist", _chip_resist );
    mandatory( jsobj, was_loaded, "density", _density );

    optional( jsobj, was_loaded, "salvaged_into", _salvaged_into, "null" );
    optional( jsobj, was_loaded, "repaired_with", _repaired_with, "null" );
    optional( jsobj, was_loaded, "edible", _edible, false );
    optional( jsobj, was_loaded, "soft", _soft, false );

    auto arr = jsobj.get_array( "vitamins" );
    while( arr.has_more() ) {
        auto pair = arr.next_array();
        _vitamins.emplace( vitamin_id( pair.get_string( 0 ) ), pair.get_float( 1 ) );
    }

    mandatory( jsobj, was_loaded, "bash_dmg_verb", _bash_dmg_verb );
    mandatory( jsobj, was_loaded, "cut_dmg_verb", _cut_dmg_verb );

    JsonArray jsarr = jsobj.get_array( "dmg_adj" );
    while( jsarr.has_more() ) {
        _dmg_adj.push_back( jsarr.next_string() );
    }

    JsonArray burn_data_array = jsobj.get_array( "burn_data" );
    for( size_t intensity = 0; intensity < MAX_FIELD_DENSITY; intensity++ ) {
        if( burn_data_array.has_more() ) {
            JsonObject brn = burn_data_array.next_object();
            _burn_data[ intensity ] = load_mat_burn_data( brn );
        } else {
            // If not specified, supply default
            bool flammable = _fire_resist <= ( int )intensity;
            mat_burn_data mbd;
            if( flammable ) {
                mbd.burn = 1;
            }

            _burn_data[ intensity ] = mbd;
        }
    }
    auto bp_array = jsobj.get_array( "burn_products" );
    while( bp_array.has_more( ) ) {
        auto pair = bp_array.next_array();
        _burn_products.emplace_back( pair.get_string( 0 ), pair.get_float( 1 ) );
    }
}

void material_type::check() const
{
    if( name().empty() ) {
        debugmsg( "material %s has no name.", id.c_str() );
    }
    if( _dmg_adj.size() < 4 ) {
        debugmsg( "material %s specifies insufficient damaged adjectives.", id.c_str() );
    }
    if( !item::type_is_defined( _salvaged_into ) ) {
        debugmsg( "invalid \"salvaged_into\" %s for %s.", _salvaged_into.c_str(), id.c_str() );
    }
    if( !item::type_is_defined( _repaired_with ) ) {
        debugmsg( "invalid \"repaired_with\" %s for %s.", _repaired_with.c_str(), id.c_str() );
    }
}

int material_type::dam_resist( damage_type damtype ) const
{
    switch( damtype ) {
        case DT_BASH:
            return _bash_resist;
            break;
        case DT_CUT:
            return _cut_resist;
            break;
        case DT_ACID:
            return _acid_resist;
            break;
        case DT_ELECTRIC:
            return _elec_resist;
            break;
        case DT_HEAT:
            return _fire_resist;
            break;
        default:
            return 0;
            break;
    }
}

material_id material_type::ident() const
{
    return id;
}

std::string material_type::name() const
{
    return _( _name.c_str() );
}

itype_id material_type::salvaged_into() const
{
    return _salvaged_into;
}

itype_id material_type::repaired_with() const
{
    return _repaired_with;
}

int material_type::bash_resist() const
{
    return _bash_resist;
}

int material_type::cut_resist() const
{
    return _cut_resist;
}

std::string material_type::bash_dmg_verb() const
{
    return _( _bash_dmg_verb.c_str() );
}

std::string material_type::cut_dmg_verb() const
{
    return _( _cut_dmg_verb.c_str() );
}

std::string material_type::dmg_adj( int damage ) const
{
    if( damage <= 0 ) {
        // not damaged (+/- reinforced)
        return std::string();
    }

    // apply bounds checking
    return _( _dmg_adj[std::min( size_t( damage ), _dmg_adj.size() ) - 1].c_str() );
}

int material_type::acid_resist() const
{
    return _acid_resist;
}

int material_type::elec_resist() const
{
    return _elec_resist;
}

int material_type::fire_resist() const
{
    return _fire_resist;
}

int material_type::chip_resist() const
{
    return _chip_resist;
}

int material_type::density() const
{
    return _density;
}

bool material_type::edible() const
{
    return _edible;
}

bool material_type::soft() const
{
    return _soft;
}

const mat_burn_data &material_type::burn_data( size_t intensity ) const
{
    return _burn_data[ std::min<size_t>( intensity, MAX_FIELD_DENSITY ) - 1 ];
}

const mat_burn_products &material_type::burn_products() const
{
    return _burn_products;
}

void materials::load( JsonObject &jo, const std::string &src )
{
    material_data.load( jo, src );
}

void materials::check()
{
    material_data.check();
}

void materials::reset()
{
    material_data.reset();
}
