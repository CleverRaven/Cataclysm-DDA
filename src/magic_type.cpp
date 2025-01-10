#include "magic_type.h"

#include "debug.h"
#include "generic_factory.h"
#include "math_parser_jmath.h"

// LOADING
// magic_type

namespace
{
generic_factory<magic_type> magic_type_factory( "magic_type" );
} // namespace

template<>
const magic_type &string_id<magic_type>::obj() const
{
    return magic_type_factory.obj( *this );
}

template<>
bool string_id<magic_type>::is_valid() const
{
    return magic_type_factory.is_valid( *this );
}

void magic_type::load_magic_type( const JsonObject &jo, const std::string &src )
{
    magic_type_factory.load( jo, src );
}


void magic_type::load( const JsonObject &jo, const std::string_view src )
{
    src_mod = mod_id( src );

    optional( jo, was_loaded, "get_level_formula_id", get_level_formula_id );
    optional( jo, was_loaded, "exp_for_level_formula_id", exp_for_level_formula_id );
    if( ( get_level_formula_id.has_value() && !exp_for_level_formula_id.has_value() ) ||
        ( !get_level_formula_id.has_value() && exp_for_level_formula_id.has_value() ) ) {
        debugmsg( "magic_type id:%s has a get_level_formula_id or exp_for_level_formula_id but not the other!  This breaks the calculations for xp/level!",
                  id.c_str() );
    }
    optional( jo, was_loaded, "energy_source", energy_source );
    if( jo.has_array( "cannot_cast_flags" ) ) {
        for( auto &cannot_cast_flag : jo.get_string_array( "cannot_cast_flags" ) ) {
            cannot_cast_flags.insert( cannot_cast_flag );
        }
    } else if( jo.has_string( "cannot_cast_flags" ) ) {
        const std::string cannot_cast_flag = jo.get_string( "cannot_cast_flags" );
        cannot_cast_flags.insert( cannot_cast_flag );
    }
    optional( jo, was_loaded, "cannot_cast_message", cannot_cast_message );
}

void magic_type::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "type", "magic_type" );
    json.member( "id", id );
    json.member( "src_mod", src_mod );
    json.member( "get_level_formula_id", get_level_formula_id );
    json.member( "exp_for_level_formula_id", exp_for_level_formula_id );
    json.member( "energy_source", energy_source );
    json.member( "cannot_cast_flags", cannot_cast_flags, std::set<std::string> {} );
    json.member( "cannot_cast_message", cannot_cast_message );

    json.end_object();
}

void magic_type::check_consistency()
{
    for( const magic_type &m_t : get_all() ) {
        if( m_t.exp_for_level_formula_id.has_value() &&
            m_t.exp_for_level_formula_id.value()->num_params != 1 ) {
            debugmsg( "ERROR: %s exp_for_level_formula_id has params that != 1!", m_t.id.c_str() );
        }
        if( m_t.get_level_formula_id.has_value() && m_t.get_level_formula_id.value()->num_params != 1 ) {
            debugmsg( "ERROR: %s get_level_formula_id has params that != 1!", m_t.id.c_str() );
        }
    }
}

const std::vector<magic_type> &magic_type::get_all()
{
    return magic_type_factory.get_all();
}

void magic_type::reset_all()
{
    magic_type_factory.reset();
}
