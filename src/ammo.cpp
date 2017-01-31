#include "ammo.h"
#include "debug.h"
#include "json.h"
#include "item.h"
#include "translations.h"

#include <unordered_map>

namespace
{
using ammo_map_t = std::unordered_map<ammotype, ammunition_type>;

ammo_map_t &all_ammunition_types()
{
    static ammo_map_t the_map;
    return the_map;
}
} //namespace

void ammunition_type::load_ammunition_type( JsonObject &jsobj )
{
    ammunition_type &res = all_ammunition_types()[ ammotype( jsobj.get_string( "id" ) ) ];
    res.name_             = _( jsobj.get_string( "name" ).c_str() );
    res.default_ammotype_ = jsobj.get_string( "default" );
}

template<>
const string_id<ammunition_type> string_id<ammunition_type>::NULL_ID( "NULL" );

template<>
bool string_id<ammunition_type>::is_valid() const
{
    return all_ammunition_types().count( *this ) > 0;
}

template<>
ammunition_type const &string_id<ammunition_type>::obj() const
{
    auto const &the_map = all_ammunition_types();

    auto const it = the_map.find( *this );
    if( it != the_map.end() ) {
        return it->second;
    }

    debugmsg( "Tried to get invalid ammunition: %s", c_str() );
    static ammunition_type const null_ammunition {
        "null"
    };
    return null_ammunition;
}

void ammunition_type::reset()
{
    all_ammunition_types().clear();
}

void ammunition_type::check_consistency()
{
    for( const auto &ammo : all_ammunition_types() ) {
        auto const &id = ammo.first;
        auto const &at = ammo.second.default_ammotype_;

        // TODO: these ammo types should probably not have default ammo at all.
        if( at == "UPS" || at == "components" || at == "thrown" ) {
            continue;
        }

        if( !at.empty() && !item::type_is_defined( at ) ) {
            debugmsg( "ammo type %s has invalid default ammo %s", id.c_str(), at.c_str() );
        }
    }
}
