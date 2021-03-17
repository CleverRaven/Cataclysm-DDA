#include "material.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>

#include "assign.h"
#include "calendar.h"
#include "debug.h"
#include "generic_factory.h"
#include "item.h"
#include "json.h"

namespace
{

generic_factory<material_type> material_data( "material" );

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
    _bash_dmg_verb( to_translation( "damages" ) ),
    _cut_dmg_verb( to_translation( "damages" ) )
{
    _dmg_adj = { to_translation( "lightly damaged" ), to_translation( "damaged" ), to_translation( "very damaged" ), to_translation( "thoroughly damaged" ) };
}

static mat_burn_data load_mat_burn_data( const JsonObject &jsobj )
{
    mat_burn_data bd;
    assign( jsobj, "immune", bd.immune );
    assign( jsobj, "volume_per_turn", bd.volume_per_turn );
    jsobj.read( "fuel", bd.fuel );
    jsobj.read( "smoke", bd.smoke );
    jsobj.read( "burn", bd.burn );
    return bd;
}

void material_type::load( const JsonObject &jsobj, const std::string & )
{
    mandatory( jsobj, was_loaded, "name", _name );

    mandatory( jsobj, was_loaded, "bash_resist", _bash_resist );
    mandatory( jsobj, was_loaded, "cut_resist", _cut_resist );
    mandatory( jsobj, was_loaded, "acid_resist", _acid_resist );
    mandatory( jsobj, was_loaded, "elec_resist", _elec_resist );
    mandatory( jsobj, was_loaded, "fire_resist", _fire_resist );
    mandatory( jsobj, was_loaded, "bullet_resist", _bullet_resist );
    mandatory( jsobj, was_loaded, "chip_resist", _chip_resist );
    mandatory( jsobj, was_loaded, "density", _density );

    optional( jsobj, was_loaded, "specific_heat_liquid", _specific_heat_liquid );
    optional( jsobj, was_loaded, "specific_heat_solid", _specific_heat_solid );
    optional( jsobj, was_loaded, "latent_heat", _latent_heat );
    optional( jsobj, was_loaded, "freezing_point", _freeze_point );

    assign( jsobj, "salvaged_into", _salvaged_into );
    optional( jsobj, was_loaded, "repaired_with", _repaired_with, itype_id::NULL_ID() );
    optional( jsobj, was_loaded, "edible", _edible, false );
    optional( jsobj, was_loaded, "rotting", _rotting, false );
    optional( jsobj, was_loaded, "soft", _soft, false );
    optional( jsobj, was_loaded, "reinforces", _reinforces, false );

    for( JsonArray pair : jsobj.get_array( "vitamins" ) ) {
        _vitamins.emplace( vitamin_id( pair.get_string( 0 ) ), pair.get_float( 1 ) );
    }

    mandatory( jsobj, was_loaded, "bash_dmg_verb", _bash_dmg_verb );
    mandatory( jsobj, was_loaded, "cut_dmg_verb", _cut_dmg_verb );

    mandatory( jsobj, was_loaded, "dmg_adj", _dmg_adj );

    if( jsobj.has_array( "burn_data" ) ) {
        for( JsonObject brn : jsobj.get_array( "burn_data" ) ) {
            _burn_data.emplace_back( load_mat_burn_data( brn ) );
        }
    }
    if( _burn_data.empty() ) {
        // If not specified, supply default
        mat_burn_data mbd;
        if( _fire_resist <= 0 ) {
            mbd.burn = 1;
        }
        _burn_data.emplace_back( mbd );
    }

    optional( jsobj, was_loaded, "fuel_data", fuel );

    jsobj.read( "burn_products", _burn_products, true );
}

void material_type::check() const
{
    if( name().empty() ) {
        debugmsg( "material %s has no name.", id.c_str() );
    }
    if( _dmg_adj.size() < 4 ) {
        debugmsg( "material %s specifies insufficient damaged adjectives.", id.c_str() );
    }
    if( _salvaged_into && ( !item::type_is_defined( *_salvaged_into ) || _salvaged_into->is_null() ) ) {
        debugmsg( "invalid \"salvaged_into\" %s for %s.", _salvaged_into->c_str(), id.c_str() );
    }
    if( !item::type_is_defined( _repaired_with ) ) {
        debugmsg( "invalid \"repaired_with\" %s for %s.", _repaired_with.c_str(), id.c_str() );
    }
}

material_id material_type::ident() const
{
    return id;
}

std::string material_type::name() const
{
    return _name.translated();
}

cata::optional<itype_id> material_type::salvaged_into() const
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

int material_type::bullet_resist() const
{
    return _bullet_resist;
}

std::string material_type::bash_dmg_verb() const
{
    return _bash_dmg_verb.translated();
}

std::string material_type::cut_dmg_verb() const
{
    return _cut_dmg_verb.translated();
}

std::string material_type::dmg_adj( int damage ) const
{
    if( damage <= 0 ) {
        // not damaged (+/- reinforced)
        return std::string();
    }

    // apply bounds checking
    return _dmg_adj[std::min( static_cast<size_t>( damage ), _dmg_adj.size() ) - 1].translated();
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

float material_type::specific_heat_liquid() const
{
    return _specific_heat_liquid;
}

float material_type::specific_heat_solid() const
{
    return _specific_heat_solid;
}

float material_type::latent_heat() const
{
    return _latent_heat;
}

float material_type::freeze_point() const
{
    return _freeze_point;
}

int material_type::density() const
{
    return _density;
}

bool material_type::edible() const
{
    return _edible;
}

bool material_type::rotting() const
{
    return _rotting;
}

bool material_type::soft() const
{
    return _soft;
}

bool material_type::reinforces() const
{
    return _reinforces;
}

fuel_data material_type::get_fuel_data() const
{
    return fuel;
}

const mat_burn_data &material_type::burn_data( size_t intensity ) const
{
    return _burn_data[ std::min<size_t>( intensity, _burn_data.size() ) - 1 ];
}

const mat_burn_products &material_type::burn_products() const
{
    return _burn_products;
}

void materials::load( const JsonObject &jo, const std::string &src )
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

material_list materials::get_all()
{
    return material_data.get_all();
}

std::set<material_id> materials::get_rotting()
{
    static generic_factory<material_type>::Version version;
    static std::set<material_id> rotting;

    // freshly created version is guaranteed to be invalid
    if( !material_data.is_valid( version ) ) {
        material_list all = get_all();
        rotting.clear();
        for( const material_type &m : all ) {
            if( m.rotting() ) {
                rotting.emplace( m.ident() );
            }
        }
        version = material_data.get_version();
    }

    return rotting;
}

void fuel_data::load( const JsonObject &jsobj )
{
    mandatory( jsobj, was_loaded, "energy", energy );
    optional( jsobj, was_loaded, "pump_terrain", pump_terrain );
    optional( jsobj, was_loaded, "explosion_data", explosion_data );
    optional( jsobj, was_loaded, "perpetual", is_perpetual_fuel );
}

void fuel_data::deserialize( JsonIn &jsin )
{
    const JsonObject &jo = jsin.get_object();
    load( jo );
}

bool fuel_explosion_data::is_empty()
{
    return explosion_chance_cold == 0 && explosion_chance_hot == 0 && explosion_factor == 0.0f &&
           !fiery_explosion && fuel_size_factor == 0.0f;
}

void fuel_explosion_data::load( const JsonObject &jsobj )
{
    optional( jsobj, was_loaded, "chance_hot", explosion_chance_hot );
    optional( jsobj, was_loaded, "chance_cold", explosion_chance_cold );
    optional( jsobj, was_loaded, "factor", explosion_factor );
    optional( jsobj, was_loaded, "size_factor", fuel_size_factor );
    optional( jsobj, was_loaded, "fiery", fiery_explosion );
}

void fuel_explosion_data::deserialize( JsonIn &jsin )
{
    const JsonObject &jo = jsin.get_object();
    load( jo );
}
