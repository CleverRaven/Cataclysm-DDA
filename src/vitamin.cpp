#include "vitamin.h"

#include <map>

#include "debug.h"
#include "translations.h"
#include "calendar.h"

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

int vitamin::severity( int qty ) const
{
    for( int i = 0; i != int( disease_.size() ); ++i ) {
        if( ( qty >= disease_[ i ].first && qty <= disease_[ i ].second ) ||
            ( qty <= disease_[ i ].first && qty >= disease_[ i ].second ) ) {
            return i + 1;
        }
    }
    // @todo implement distinct severity levels for vitamin excesses
    if( qty > 96 ) {
        return -1;
    }
    return 0;
}

void vitamin::load_vitamin( JsonObject &jo )
{
    vitamin vit;

    vit.id_ = vitamin_id( jo.get_string( "id" ) );
    vit.name_ = _( jo.get_string( "name" ).c_str() );
    vit.deficiency_ = efftype_id( jo.get_string( "deficiency" ) );
    vit.excess_ = efftype_id( jo.get_string( "excess", "null" ) );
    vit.min_ = jo.get_int( "min" );
    vit.max_ = jo.get_int( "max", 0 );
    vit.rate_ = jo.get_int( "rate", MINUTES( 60 ) );

    if( vit.rate_ < 0 ) {
        jo.throw_error( "vitamin consumption rate cannot be negative", "rate" );
    }

    auto def = jo.get_array( "disease" );
    while( def.has_more() ) {
        auto e = def.next_array();
        vit.disease_.emplace_back( e.get_int( 0 ), e.get_int( 1 ) );
    }

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
