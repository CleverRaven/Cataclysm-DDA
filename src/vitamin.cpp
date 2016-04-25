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

const std::pair<efftype_id, int> &vitamin::effect( int level ) const
{
    for( const auto &e : deficiency_ ) {
        if( level <= e.first ) {
            return e.second;
        }
    }
    for( const auto &e : excess_ ) {
        if( level >= e.first ) {
            return e.second;
        }
    }
    static std::pair<efftype_id, int> null_effect = { NULL_ID, 1 };
    return null_effect;
}

void vitamin::load_vitamin( JsonObject &jo )
{
    vitamin vit;

    vit.id_ = vitamin_id( jo.get_string( "id" ) );
    vit.name_ = jo.get_string( "name" );
    vit.min_ = jo.get_int( "min" );
    vit.max_ = jo.get_int( "max", 0 );
    vit.rate_ = jo.get_int( "rate", 60 );

    if( vit.rate_ < 0 ) {
        jo.throw_error( "vitamin consumption rate cannot be negative", "rate" );
    }

    using disease = std::pair<int, std::pair<efftype_id, int>>;

    auto def = jo.get_array( "deficiency" );
    while( def.has_more() ) {
        auto e = def.next_array();
        vit.deficiency_.emplace_back( e.get_int( 0 ),
                                      std::make_pair( efftype_id( e.get_string( 1 ) ), e.get_int( 2 ) ) );
    }
    std::sort( vit.deficiency_.begin(), vit.deficiency_.end(),
               []( const disease& lhs, const disease& rhs ) { return lhs.first < rhs.first; } );

    auto exc = jo.get_array( "excess" );
    while( exc.has_more() ) {
        auto e = exc.next_array();
        vit.excess_.emplace_back( e.get_int( 0 ),
                                  std::make_pair( efftype_id( e.get_string( 1 ) ), e.get_int( 2 ) ) );
    }
    std::sort( vit.excess_.rbegin(), vit.excess_.rend(),
               []( const disease& lhs, const disease& rhs ) { return lhs.first < rhs.first; } );

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
