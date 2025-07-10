#include "material.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <unordered_map>

#include "assign.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "item.h"
#include "make_static.h"

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

namespace io
{
template<>
std::string enum_to_string<breathability_rating>( breathability_rating data )
{
    switch( data ) {
        case breathability_rating::IMPERMEABLE:
            return "IMPERMEABLE";
        case breathability_rating::POOR:
            return "POOR";
        case breathability_rating::AVERAGE:
            return "AVERAGE";
        case breathability_rating::GOOD:
            return "GOOD";
        case breathability_rating::MOISTURE_WICKING:
            return "MOISTURE_WICKING";
        case breathability_rating::SECOND_SKIN:
            return "SECOND_SKIN";
        case breathability_rating::last:
            break;
    }
    cata_fatal( "Invalid breathability" );
}
} // namespace io

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

void material_type::load( const JsonObject &jsobj, std::string_view )
{
    mandatory( jsobj, was_loaded, "name", _name );

    if( jsobj.has_object( "resist" ) ) {
        _res_was_loaded.clear();
        JsonObject jo = jsobj.get_object( "resist" );
        _resistances = load_resistances_instance( jo );
        for( const JsonMember &jmemb : jo ) {
            _res_was_loaded.emplace_back( jmemb.name() );
        }
    }

    optional( jsobj, was_loaded, "conductive", _conductive );
    mandatory( jsobj, was_loaded, "chip_resist", _chip_resist );
    mandatory( jsobj, was_loaded, "density", _density );

    optional( jsobj, was_loaded, "sheet_thickness", _sheet_thickness );

    optional( jsobj, was_loaded, "repair_difficulty", _repair_difficulty );

    optional( jsobj, was_loaded, "wind_resist", _wind_resist );
    optional( jsobj, was_loaded, "specific_heat_liquid", _specific_heat_liquid );
    optional( jsobj, was_loaded, "specific_heat_solid", _specific_heat_solid );
    optional( jsobj, was_loaded, "latent_heat", _latent_heat );
    optional( jsobj, was_loaded, "freezing_point", _freeze_point );

    optional( jsobj, was_loaded, "breathability", _breathability, breathability_rating::IMPERMEABLE );

    assign( jsobj, "salvaged_into", _salvaged_into );
    optional( jsobj, was_loaded, "repaired_with", _repaired_with, itype_id::NULL_ID() );
    optional( jsobj, was_loaded, "rotting", _rotting, false );
    optional( jsobj, was_loaded, "soft", _soft, false );
    optional( jsobj, was_loaded, "uncomfortable", _uncomfortable, false );

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
        if( _resistances.type_resist( STATIC( damage_type_id( "heat" ) ) ) <= 0.f ) {
            mbd.burn = 1;
        }
        _burn_data.emplace_back( mbd );
    }

    optional( jsobj, was_loaded, "fuel_data", fuel );

    jsobj.read( "burn_products", _burn_products, true );
}

void material_type::finalize_all()
{
    material_data.finalize();
    for( const material_type &mtype : material_data.get_all() ) {
        material_type &mt = const_cast<material_type &>( mtype );
        finalize_damage_map( mt._resistances.resist_vals );
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
    if( _salvaged_into && ( !item::type_is_defined( *_salvaged_into ) || _salvaged_into->is_null() ) ) {
        debugmsg( "invalid \"salvaged_into\" %s for %s.", _salvaged_into->c_str(), id.c_str() );
    }
    if( !item::type_is_defined( _repaired_with ) ) {
        debugmsg( "invalid \"repaired_with\" %s for %s.", _repaired_with.c_str(), id.c_str() );
    }

    if( _wind_resist && ( *_wind_resist > 100 || *_wind_resist < 0 ) ) {
        debugmsg( "Wind resistance outside of range (100%% to 0%%, is %d%%) for %s.", *_wind_resist,
                  id.str() );
    }

    if( _repair_difficulty && ( _repair_difficulty > 10 || _repair_difficulty < 0 ) ) {
        debugmsg( "Repair difficulty out of skill range (0 to 10, is %d) for %s.", _repair_difficulty,
                  id.str() );
    }

    for( const auto &dt : _resistances.resist_vals ) {
        if( !dt.first.is_valid() ) {
            debugmsg( "Invalid resistance type \"%s\" for material %s", dt.first.c_str(), id.c_str() );
        }
    }

    for( const damage_type &dt : damage_type::get_all() ) {
        bool type_defined =
            std::find( _res_was_loaded.begin(), _res_was_loaded.end(),  dt.id ) != _res_was_loaded.end();
        if( dt.material_required && !type_defined ) {
            debugmsg( "material %s is missing required resistance for \"%s\"", id.c_str(), dt.id.c_str() );
        }
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

std::optional<itype_id> material_type::salvaged_into() const
{
    return _salvaged_into;
}

itype_id material_type::repaired_with() const
{
    return _repaired_with;
}

float material_type::resist( const damage_type_id &dmg_type ) const
{
    return _resistances.type_resist( dmg_type );
}

bool material_type::has_dedicated_resist( const damage_type_id &dmg_type ) const
{
    return std::find( _res_was_loaded.begin(), _res_was_loaded.end(),
                      dmg_type ) != _res_was_loaded.end();
}

std::string material_type::bash_dmg_verb() const
{
    return _bash_dmg_verb.translated();
}

std::string material_type::cut_dmg_verb() const
{
    return _cut_dmg_verb.translated();
}

std::string material_type::dmg_adj( int damage_level ) const
{
    if( damage_level <= 1 ) {
        return std::string(); // not damaged
    }
    const int idx = std::clamp( damage_level - 2, 0, static_cast<int>( _dmg_adj.size() ) );
    return _dmg_adj[idx].translated();
}

int material_type::chip_resist() const
{
    return _chip_resist;
}

int material_type::repair_difficulty() const
{
    return _repair_difficulty;
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

float material_type::density() const
{
    return _density;
}

bool material_type::is_conductive() const
{
    return _conductive;
}

bool material_type::is_valid_thickness( float thickness ) const
{
    // if this doesn't have an expected thickness return true
    if( _sheet_thickness == 0 ) {
        return true;
    }

    // float calcs so rounding need to be mindful of
    return std::fmod( thickness, _sheet_thickness ) < .01f;
}

float material_type::thickness_multiple() const
{
    return _sheet_thickness;
}

int material_type::breathability_to_rating( breathability_rating breathability )
{
    // this is where the values for each of these exist
    switch( breathability ) {
        case breathability_rating::IMPERMEABLE:
            return 0;
        case breathability_rating::POOR:
            return 30;
        case breathability_rating::AVERAGE:
            return 50;
        case breathability_rating::GOOD:
            return 80;
        case breathability_rating::MOISTURE_WICKING:
            return 110;
        case breathability_rating::SECOND_SKIN:
            return 140;
        case breathability_rating::last:
            break;
    }
    return 0;
}
int material_type::breathability() const
{
    return material_type::breathability_to_rating( _breathability );
}

std::optional<int> material_type::wind_resist() const
{
    return _wind_resist;
}

bool material_type::rotting() const
{
    return _rotting;
}

bool material_type::soft() const
{
    return _soft;
}

bool material_type::uncomfortable() const
{
    return _uncomfortable;
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

void fuel_data::deserialize( const JsonObject &jo )
{
    load( jo );
}

bool fuel_explosion_data::is_empty() const
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

void fuel_explosion_data::deserialize( const JsonObject &jo )
{
    load( jo );
}
