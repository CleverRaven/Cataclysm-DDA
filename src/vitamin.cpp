#include "vitamin.h"

#include <map>

#include "debug.h"

static std::map<vitamin_id, vitamin> vitamins_all;

template<>
bool string_id<vitamin>::is_valid() const
{
    return vitamins_all.count( *this );
}

template<>
const vitamin &string_id<vitamin>::obj() const
{
    const auto found = vitamins_all.find( *this );
    if( found == vitamins_all.end() ) {
        debugmsg( "Tried to get invalid vitamin: %s", c_str() );
        static const vitamin null_vitamin{};
        return null_vitamin;
    }
    return found->second;
}

void vitamin::load_vitamin( JsonObject &jo )
{
    vitamin vit;

    vit.id_ = vitamin_id( jo.get_string( "id" ) );
    vit.name_ = jo.get_string( "name" );

    if( vitamins_all.find( vit.id_ ) != vitamins_all.end() ) {
        jo.throw_error( "parsed vitamin overwrites existing definition", "id" );
    } else {
        vitamins_all[ vit.id_ ] = vit;
        DebugLog( D_INFO, DC_ALL ) << "Loaded vitamin: " << vit.name_;
    }
}

const std::map<vitamin_id, vitamin> &vitamin::all()
{
    return vitamins_all;
}

void vitamin::reset()
{
    vitamins_all.clear();
}
