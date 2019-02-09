#include "flag.h"

#include <algorithm>
#include <unordered_map>

#include "debug.h"
#include "json.h"

static std::unordered_map<std::string, json_flag> json_flags_all;

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
    for( const auto &e : json_flags_all ) {
        const auto &f = e.second;
        for( const std::string &conflicting : f.conflicts_ ) {
            if( !json_flags_all.count( conflicting ) ) {
                debugmsg( "flag definition %s specifies unknown conflicting field %s",
                          f.id(), conflicting );
            }
        }
    }
}

void json_flag::reset()
{
    json_flags_all.clear();
}
