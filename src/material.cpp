#include "material.h"

#include "debug.h"
#include "damage.h" // damage_type
#include "json.h"
#include "translations.h"

#include <string>
#include <map>

namespace
{
std::map<material_id, material_type> _all_materials;
} // namespace

template<>
bool string_id<material_type>::is_valid() const
{
    return _all_materials.count( *this ) > 0;
}

template<>
const material_type &string_id<material_type>::obj() const
{
    const auto found = _all_materials.find( *this );
    if( found == _all_materials.end() ) {
        debugmsg( "Tried to get invalid material: %s", c_str() );
        static const material_type null_material{};
        return null_material;
    }
    return found->second;
}

material_type::material_type()
{
    _ident = material_id( "null" );
    _name = "null";
    _salvage_id = "null";
    _salvage_multiplier = 1.0;
    _bash_resist = 0;
    _cut_resist = 0;
    _bash_dmg_verb = _( "damages" );
    _cut_dmg_verb = _( "damages" );
    _dmg_adj[0] = _( "lightly damaged" );
    _dmg_adj[1] = _( "damaged" );
    _dmg_adj[2] = _( "very damaged" );
    _dmg_adj[3] = _( "thoroughly damaged" );
    _acid_resist = 0;
    _elec_resist = 0;
    _fire_resist = 0;
    _chip_resist = 0;
    _density = 1;
}

// load a material object from incoming JSON
void material_type::load_material( JsonObject &jsobj )
{
    material_type mat;

    mat._ident = material_id( jsobj.get_string( "ident" ) );
    mat._name = _( jsobj.get_string( "name" ).c_str() );
    mat._salvage_id = jsobj.get_string( "salvage_id", "null" );
    mat._salvage_multiplier = jsobj.get_float( "salvage_multiplier", 1.0 );
    mat._bash_resist = jsobj.get_int( "bash_resist" );
    mat._cut_resist = jsobj.get_int( "cut_resist" );
    mat._bash_dmg_verb = _( jsobj.get_string( "bash_dmg_verb" ).c_str() );
    mat._cut_dmg_verb = _( jsobj.get_string( "cut_dmg_verb" ).c_str() );
    mat._acid_resist = jsobj.get_int( "acid_resist" );
    mat._elec_resist = jsobj.get_int( "elec_resist" );
    mat._fire_resist = jsobj.get_int( "fire_resist" );
    mat._chip_resist = jsobj.get_int( "chip_resist" );
    mat._density = jsobj.get_int( "density" );
    mat._edible = jsobj.get_bool( "edible", false );

    auto arr = jsobj.get_array( "vitamins" );
    while( arr.has_more() ) {
        auto pair = arr.next_array();
        mat._vitamins.emplace( vitamin_id( pair.get_string( 0 ) ), pair.get_float( 1 ) );
    }

    JsonArray jsarr = jsobj.get_array( "dmg_adj" );
    mat._dmg_adj[0] = _( jsarr.next_string().c_str() );
    mat._dmg_adj[1] = _( jsarr.next_string().c_str() );
    mat._dmg_adj[2] = _( jsarr.next_string().c_str() );
    mat._dmg_adj[3] = _( jsarr.next_string().c_str() );

    _all_materials[mat._ident] = mat;
    DebugLog( D_INFO, DC_ALL ) << "Loaded material: " << mat._name;
}

void material_type::reset()
{
    _all_materials.clear();
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

bool material_type::is_null() const
{
    return ( _ident == material_id( "null" ) );
}

material_id material_type::ident() const
{
    return _ident;
}

std::string material_type::name() const
{
    return _name;
}

std::string material_type::salvage_id() const
{
    return _salvage_id;
}

float material_type::salvage_multiplier() const
{
    return _salvage_multiplier;
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
    return _bash_dmg_verb;
}

std::string material_type::cut_dmg_verb() const
{
    return _cut_dmg_verb;
}

std::string material_type::dmg_adj( int damage ) const
{
    if( damage <= 0 ) {
        // not damaged (+/- reinforced)
        return std::string();
    }

    // apply bounds checking
    return _dmg_adj[ std::min( damage, MAX_ITEM_DAMAGE ) - 1 ];
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