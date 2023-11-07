#include "vitamin.h"

#include <cstdlib>
#include <map>

#include "assign.h"
#include "calendar.h"
#include "debug.h"
#include "enum_conversions.h"
#include "json.h"
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
    vit.rate_ = read_from_json_string<time_duration>( jo.get_member( "rate" ), time_duration::units );

    if( jo.has_string( "weight_per_unit" ) ) {
        vit.weight_per_unit = read_from_json_string( jo.get_member( "weight_per_unit" ),
                              vitamin_units::mass_units );
    }

    if( !jo.has_string( "vit_type" ) ) {
        jo.throw_error_at( "vit_type", "vitamin must have a vitamin type" );
    }
    vit.type_ = jo.get_enum_value<vitamin_type>( "vit_type" );

    for( JsonArray e : jo.get_array( "disease" ) ) {
        vit.disease_.emplace_back( e.get_int( 0 ), e.get_int( 1 ) );
    }

    for( JsonArray e : jo.get_array( "disease_excess" ) ) {
        vit.disease_excess_.emplace_back( e.get_int( 0 ), e.get_int( 1 ) );
    }

    for( JsonArray e : jo.get_array( "decays_into" ) ) {
        vit.decays_into_.emplace_back( vitamin_id( e.get_string( 0 ) ), e.get_int( 1 ) );
    }

    for( std::string e : jo.get_array( "flags" ) ) {
        vit.flags_.insert( e );
    }

    if( vitamins_all.find( vit.id_ ) != vitamins_all.end() ) {
        jo.throw_error_at( "id", "parsed vitamin overwrites existing definition" );
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

float vitamin::RDA_to_default( int percent ) const
{
    // if not a vitamin it's in Units and doesn't need conversion
    if( type_ != vitamin_type::VITAMIN ) {
        return percent;
    }
    return ( 24_hours / rate_ ) * ( static_cast<float>( percent ) / 100.0f );
}

int vitamin::units_from_mass( vitamin_units::mass val ) const
{
    if( !weight_per_unit.has_value() ) {
        debugmsg( "Tried to convert vitamin in mass to units, but %s doesn't support mass for vitamins",
                  id_.str() );
        return 1;
    }
    return val / *weight_per_unit;
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
    cata_fatal( "Invalid vitamin_type" );
}
} // namespace io
