#include "math_parser_func.h"
#include "math_parser_shim.h"

#include <exception>
#include <string_view>
#include <vector>

#include "condition.h"
#include "dialogue.h"
#include "mission.h"

std::vector<std::string_view> tokenize( std::string_view str, std::string_view separators,
                                        bool include_seps )
{
    std::vector<std::string_view> ret;
    std::string_view::size_type start = 0;
    std::string_view::size_type pos = 0;

    while( pos != std::string_view::npos ) {
        pos = str.find_first_of( separators, start );
        if( pos != start ) {
            std::string_view const ts = str.substr( start, pos - start );
            if( std::string_view::size_type const unpad = ts.find_first_not_of( ' ' );
                unpad != std::string_view::npos ) {
                std::string_view::size_type const unpad_e = ts.find_last_not_of( ' ' );
                ret.emplace_back(
                    ts.substr( unpad, unpad_e - unpad + 1 ) );
            }
        }
        if( pos != std::string_view::npos && include_seps ) {
            ret.emplace_back( &str[pos], 1 );
        }
        start = pos + 1;
    }
    return ret;
}

template<class D>
std::function<double( D const & )> u_val( char scope,
        std::vector<std::string> const &params )
{
    kwargs_shim const shim( params, scope );
    try {
        return conditional_t<D>::get_get_dbl( shim );
    } catch( std::exception const &e ) {
        debugmsg( "shim failed: %s", e.what() );
        return []( D const & ) {
            return 0;
        };
    }
}

template<class D>
std::function<void( D const &, double )> u_val_ass( char scope,
        std::vector<std::string> const &params )
{
    kwargs_shim const shim( params, scope );
    try {
        return conditional_t<D>::get_set_dbl( shim, {}, {}, false );
    } catch( std::exception const &e ) {
        debugmsg( "shim failed: %s", e.what() );
        return []( D const &, double ) {};
    }
}

template std::function<double( dialogue const & )>
u_val<dialogue>( char scope,
                 std::vector<std::string> const &params );
template std::function<double( mission_goal_condition_context const & )>
u_val<mission_goal_condition_context>( char scope,
                                       std::vector<std::string> const &params );
template std::function<void( dialogue const &, double )>
u_val_ass( char, std::vector<std::string> const & );
template std::function<void( mission_goal_condition_context const &, double )>
u_val_ass( char, std::vector<std::string> const & );
