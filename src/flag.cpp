#include "flag.h"

#include "debug.h"
#include "json.h"

#include <map>
#include <algorithm>

std::map<std::string, json_flag> json_flags_all;

const json_flag &json_flag::get( const std::string &id )
{
    static json_flag null_flag;
    auto iter = json_flags_all.find( id );
    return iter != json_flags_all.end() ? iter->second : null_flag;
}

void json_flag::load( JsonObject &jo )
{
    auto id = jo.get_string( "id" );
    auto &f = json_flags_all.emplace( id, json_flag( id ) ).first->second;

    jo.read( "info", f.info_ );
    jo.read( "conflicts", f.conflicts_ );
    jo.read( "inherit", f.inherit_ );
}

void json_flag::check_consistency()
{
    std::vector<std::string> flags;
    std::transform( json_flags_all.begin(), json_flags_all.end(), std::back_inserter( flags ),
    []( const std::pair<std::string, json_flag> &e ) {
        return e.first;
    } );

    for( const auto &e : json_flags_all ) {
        const auto &f = e.second;
        if( !std::includes( flags.begin(), flags.end(), f.conflicts_.begin(), f.conflicts_.end() ) ) {
            debugmsg( "flag definition %s specifies unknown conflicting field", f.id().c_str() );
        }
    }
}

void json_flag::reset()
{
    json_flags_all.clear();
}
