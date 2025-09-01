#include "assign.h"

#include "debug.h"

void report_strict_violation( const JsonObject &jo, const std::string &message,
                              std::string_view name )
{
    try {
        // Let the json class do the formatting, it includes the context of the JSON data.
        jo.throw_error_at( name, message );
    } catch( const JsonError &err ) {
        // And catch the exception so the loading continues like normal.
        debugmsg( "(json-error)\n%s", err.what() );
    }
}

bool assign( const JsonObject &jo, std::string_view name, bool &val, bool strict )
{
    bool out;

    if( !jo.read( name, out ) ) {
        return false;
    }

    if( strict && out == val ) {
        report_strict_violation( jo, "cannot assign explicit value the same as default or inherited value",
                                 name );
    }

    val = out;

    return true;
}
