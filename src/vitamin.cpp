#include "vitamin.h"

#include <map>
#include <memory>

#include "calendar.h"
#include "debug.h"
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

std::string vitamin::get_string_level( int qty ) const
{
    float norm_level = 1.0f;
    if( type_ == vitamin_type::VITAMIN ) {
        if( id_ == "vitA" ) {
            norm_level = 2.0f;
        } else if( id_ == "vitB" ) {
            norm_level = 2.75f;
        } else if( id_ == "vitC" ) {
            norm_level = 2.2f;
        } else if( id_ == "iron" ) {
            norm_level = 1.23f;
        } else if( id_ == "calcium" ) {
            norm_level = 1.1f;
        }
        float v_level = 0.0f;
        if( ( disease_.size() > 0 ) && !disease_[0].first ) {
            v_level = norm_level * qty / disease_[0].first;
        } else if( ( disease_excess_.size() > 0 ) && !disease_excess_[0].first ) {
            v_level = norm_level * qty / disease_excess_[0].first;
        } else {
            return "";
        }
        if( v_level != 0 ) {
            std::string message = string_format( _( "%s level is %.2f times %s than normal." ), name(),
                                                 std::abs( v_level ), ( v_level < 0  ? _( "higher" ) : _( "lower" ) ) );
            const int sev = severity( qty );
            if( sev ) {
                const std::vector<std::string> s = { "", _( "Minor" ), _( "Severe" ), _( "Extreme" ) };
                message += string_format( " %s %s.", s[std::abs( sev )],
                                          ( sev < 0 ? _( "deficiency" ) : _( "excess" ) ) );
            }
            return message;
        } else {
            return string_format( _( "%s level is normal." ), name() );
        }
    }
    if( type_ == vitamin_type::TOXIN ) {
        if( id_ == "mutant_toxin" ) {
            if( qty > ( disease_excess_[2].first ) ) {
                return _( "Deadly levels of unknown toxins detected." );
            } else if( qty > ( disease_excess_[1].first ) ) {
                return _( "Extreme levels of unknown toxins detected." );
            } else if( qty > ( disease_excess_[0].first ) ) {
                return _( "Dangerous levels of unknown toxins detected." );
            } else if( qty > ( disease_excess_[0].first / 2 ) ) {
                return _( "Significant levels of unknown toxins detected." );
            } else if( qty > ( disease_excess_[0].first / 4 ) ) {
                return _( "Traces of unknown toxins detected." );
            } else {
                return "";
            }
        }
    }
    return "";
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
