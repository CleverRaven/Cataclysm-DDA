#include "math_parser_shim.h"
#include "math_parser_func.h"

#include <map>
#include <string_view>

#include "condition.h"
#include "json_loader.h"
#include "string_formatter.h"

kwargs_shim::kwargs_shim( std::vector<std::string> const &tokens, char scope )
{
    bool positional = false;
    for( std::string_view const token : tokens ) {
        std::vector<std::string_view> parts = tokenize( token, ":", false );
        if( parts.size() == 1 && !positional ) {
            kwargs.emplace(
                // NOLINTNEXTLINE(cata-translate-string-literal): not a user-visible string
                string_format( "%s_val", scope == 'g' ? "global" : scope == 'n' ? "npc" : std::string( 1, scope ) ),
                parts[0] );
            positional = true;
        } else if( parts.size() == 2 ) {
            kwargs.emplace( parts[0], parts[1] );
        } else {
            debugmsg( "Too many parts in token %.*s", token.size(), token.data() );
        }
    }
}

std::string kwargs_shim::str() const
{
    std::string ret;
    for( decltype( kwargs )::value_type const &e : kwargs ) {
        // NOLINTNEXTLINE(cata-translate-string-literal): not a user-visible string
        ret += string_format( "'%.*s: %.*s', ", e.first.size(), e.first.data(), e.second.size(),
                              e.second.data() );
    }
    return ret;
}

JsonValue kwargs_shim::get_member( std::string_view key ) const
{
    auto const it = kwargs.find( key );
    bool const valid = it != kwargs.end();
    std::string const temp =
        valid ? string_format( R"("%.*s")", it->second.size(), it->second.data() )
        : R"("dummy")";
    return json_loader::from_string( temp );
}

std::string kwargs_shim::get_string( std::string_view key ) const
{
    std::optional<std::string> get = _get_string( key );
    if( get ) {
        return *get;
    }
    return {};
}

namespace
{
template<typename T>
T _ret_t( std::optional<std::string> const &get, T def )
{
    if( !get || get->empty() ) {
        return def;
    }
    try {
        return std::stod( *get );
    } catch( std::invalid_argument const &/* ex */ ) {
        return def;
    }
}
} // namespace

double kwargs_shim::get_float( std::string_view key, double def ) const
{
    return _ret_t( _get_string( key ), def );
}

int kwargs_shim::get_int( std::string_view key, int def ) const
{
    return _ret_t( _get_string( key ), def );
}

std::optional<std::string> kwargs_shim::_get_string( std::string_view key ) const
{
    auto itkw = kwargs.find( key );
    if( itkw != kwargs.end() ) {
        return { std::string{ itkw->second } };
    }

    return std::nullopt;
}
