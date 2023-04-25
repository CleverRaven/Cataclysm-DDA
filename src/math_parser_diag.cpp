#include "math_parser_diag.h"

#include <functional>
#include <string>
#include <vector>

#include "condition.h"
#include "dialogue.h"
#include "math_parser_shim.h"
#include "mission.h"

namespace
{
bool is_beta( char scope )
{
    switch( scope ) {
        case 'n':
            return true;
        case 'u':
        default:
            return false;
    }
}
} // namespace

std::function<double( dialogue const & )> u_val( char scope,
        std::vector<std::string> const &params )
{
    kwargs_shim const shim( params, scope );
    try {
        return conditional_t::get_get_dbl( shim );
    } catch( std::exception const &e ) {
        debugmsg( "shim failed: %s", e.what() );
        return []( dialogue const & ) {
            return 0;
        };
    }
}

std::function<void( dialogue const &, double )> u_val_ass( char scope,
        std::vector<std::string> const &params )
{
    kwargs_shim const shim( params, scope );
    try {
        return conditional_t::get_set_dbl( shim, {}, {}, false );
    } catch( std::exception const &e ) {
        debugmsg( "shim failed: %s", e.what() );
        return []( dialogue const &, double ) {};
    }
}

std::function<double( dialogue const & )> pain_eval( char scope,
        std::vector<std::string> const &/* params */ )
{
    return [beta = is_beta( scope )]( dialogue const & d ) {
        return d.actor( beta )->pain_cur();
    };
}

std::function<void( dialogue const &, double )> pain_ass( char scope,
        std::vector<std::string> const &/* params */ )
{
    return [beta = is_beta( scope )]( dialogue const & d, double val ) {
        d.actor( beta )->set_pain( val );
    };
}
