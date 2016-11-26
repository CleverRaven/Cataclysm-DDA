#include "ammo.h"
#include "debug.h"
#include "json.h"
#include "item.h"

static std::unordered_map<ammotype, ammunition_type> ammunition_type_all;

void ammunition_type::load_ammunition_type( JsonObject &jsobj )
{
    ammunition_type &res = ammunition_type_all[ ammotype( jsobj.get_string( "id" ) ) ];
    res.name_             = jsobj.get_string( "name" );
    res.default_ammotype_ = jsobj.get_string( "default" );
}

template<>
const string_id<ammunition_type> string_id<ammunition_type>::NULL_ID( "NULL" );

template<>
bool string_id<ammunition_type>::is_valid() const
{
    return ammunition_type_all.count( *this );
}

template<>
ammunition_type const &string_id<ammunition_type>::obj() const
{
    const auto found = ammunition_type_all.find( *this );
    if( found == ammunition_type_all.end() ) {
        debugmsg( "Tried to get invalid ammunition type: %s", c_str() );
        static const ammunition_type null_ammo{};
        return null_ammo;
    }
    return found->second;
}

void ammunition_type::reset()
{
    ammunition_type_all.clear();
}

void ammunition_type::check_consistency()
{
    for( const auto &ammo : ammunition_type_all ) {
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

const std::unordered_map<ammotype, ammunition_type> &ammunition_type::all()
{
    return ammunition_type_all;
}

void ammunition_type::delete_if( const std::function<bool( const ammunition_type & )> &pred )
{
    for( auto iter = ammunition_type_all.begin(); iter != ammunition_type_all.end(); ) {
        pred( iter->second ) ? ammunition_type_all.erase( iter++ ) : ++iter;
    }
}
