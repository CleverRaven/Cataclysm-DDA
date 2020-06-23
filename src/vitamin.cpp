#include "vitamin.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <memory>

#include "calendar.h"
#include "debug.h"
#include "enum_conversions.h"
#include "json.h"
#include "string_id.h"
#include "units.h"

static std::map<vitamin_id, vitamin> vitamins_all;

/** @relates string_id */
template<>
bool string_id<vitamin>::is_valid() const
{
    return vitamins_all.count( *this );
}

/** @relates string_id */
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
    for( int i = 0; i != static_cast<int>( disease_.size() ); ++i ) {
        if( ( qty >= disease_[ i ].first && qty <= disease_[ i ].second ) ||
            ( qty <= disease_[ i ].first && qty >= disease_[ i ].second ) ) {
            return i + 1;
        }
    }
    for( int i = 0; i != static_cast<int>( disease_excess_.size() ); ++i ) {
        if( ( qty >= disease_excess_[ i ].first && qty <= disease_excess_[ i ].second ) ||
            ( qty <= disease_excess_[ i ].first && qty >= disease_excess_[ i ].second ) ) {
            return -i - 1;
        }
    }
    return 0;
}

void vitamin::load_vitamin( const JsonObject &jo )
{
    vitamin vit;

    vit.id_ = vitamin_id( jo.get_string( "id" ) );
    jo.read( "name", vit.name_ );
    vit.deficiency_ = efftype_id::NULL_ID();
    jo.read( "deficiency", vit.deficiency_ );
    vit.excess_ = efftype_id::NULL_ID();
    jo.read( "excess", vit.excess_ );
    vit.min_ = jo.get_int( "min" );
    vit.max_ = jo.get_int( "max", 0 );
    vit.rate_ = read_from_json_string<time_duration>( *jo.get_raw( "rate" ), time_duration::units );

    if( !jo.has_string( "vit_type" ) ) {
        jo.throw_error( "vitamin must have a vitamin type", "vit_type" );
    }
    vit.type_ = jo.get_enum_value<vitamin_type>( "vit_type" );

    if( vit.rate_ < 0_turns ) {
        jo.throw_error( "vitamin consumption rate cannot be negative", "rate" );
    }

    for( JsonArray e : jo.get_array( "disease" ) ) {
        vit.disease_.emplace_back( e.get_int( 0 ), e.get_int( 1 ) );
    }

    for( JsonArray e : jo.get_array( "disease_excess" ) ) {
        vit.disease_excess_.emplace_back( e.get_int( 0 ), e.get_int( 1 ) );
    }

    for( std::string e : jo.get_array( "flags" ) ) {
        vit.flags_.insert( e );
    }

    if( vitamins_all.find( vit.id_ ) != vitamins_all.end() ) {
        jo.throw_error( "parsed vitamin overwrites existing definition", "id" );
    } else {
        vitamins_all[ vit.id_ ] = vit;
    }
}

const std::map<vitamin_id, vitamin> &vitamin::all()
{
    return vitamins_all;
}

void vitamin::check_consistency()
{
    for( const auto &v : vitamins_all ) {
        if( !( v.second.deficiency_.is_null() || v.second.deficiency_.is_valid() ) ) {
            debugmsg( "vitamin %s has unknown deficiency %s", v.second.id_.c_str(),
                      v.second.deficiency_.c_str() );
        }
        if( !( v.second.excess_.is_null() || v.second.excess_.is_valid() ) ) {
            debugmsg( "vitamin %s has unknown excess %s", v.second.id_.c_str(), v.second.excess_.c_str() );
        }
    }
}

void vitamin::reset()
{
    vitamins_all.clear();
}

namespace io
{
template<>
std::string enum_to_string<vitamin_type>( vitamin_type data )
{
    switch( data ) {
        case vitamin_type::VITAMIN:
            return "vitamin";
        case vitamin_type::TOXIN:
            return "toxin";
        case vitamin_type::DRUG:
            return "drug";
        case vitamin_type::COUNTER:
            return "counter";
        case vitamin_type::num_vitamin_types:
            break;
    }
    debugmsg( "Invalid vitamin_type" );
    abort();
}
} // namespace io
