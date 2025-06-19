#include "try_parse_integer.h"

#include <locale>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "coordinates.h"
#include "map_scale_constants.h"
#include "point.h"
#include "string_formatter.h"
#include "translations.h"


template<typename T>
ret_val<T> try_parse_integer( std::string_view s, bool use_locale )
{
    // Using stringstream-based parsing because that's the only one in the
    // standard library for which it's possible to turn off the
    // locale-dependency.  Once we're using C++17 we could use a combination of
    // std::from_chars and std::strto* functions, but this should be fine so
    // long as the code is not performance-critical.
    std::istringstream buffer{ std::string( s ) };
#ifdef __APPLE__
    // On Apple platforms we always use the classic locale, because the other
    // locales seem to behave strangely.  See
    // https://github.com/CleverRaven/Cataclysm-DDA/pull/48431 for more
    // discussion.
    static_cast<void>( use_locale );
    buffer.imbue( std::locale::classic() );
#else
    if( !use_locale ) {
        buffer.imbue( std::locale::classic() );
    }
#endif
    T result;
    buffer >> result;
    if( !buffer ) {
        return ret_val<T>::make_failure(
                   0, string_format( _( "Could not convert '%s' to an integer" ), s ) );
    }
    char c;
    buffer >> c;
    if( buffer ) {
        return ret_val<T>::make_failure(
                   0, string_format( _( "Stray characters after integer in '%s'" ), s ) );
    }
    return ret_val<T>::make_success( result );
}

template ret_val<int> try_parse_integer<int>( std::string_view, bool use_locale );
template ret_val<long> try_parse_integer<long>( std::string_view, bool use_locale );
template ret_val<long long> try_parse_integer<long long>( std::string_view, bool use_locale );

ret_val<tripoint_abs_omt> try_parse_coordinate_abs( std::string_view s )
{
    const std::vector<std::string> coord_strings = string_split( s, ',' );
    if( coord_strings.size() < 2 || coord_strings.size() > 3 ) {
        return ret_val<tripoint_abs_omt>::make_failure( tripoint_abs_omt::zero,
                string_format( _( "Error interpreting coordinate: "
                                  "expected two or three comma-separated values; got %zu" ),
                               coord_strings.size() ) );
    }
    std::vector<std::pair<int, int>> coord_ints;
    for( const std::string &coord_string : coord_strings ) {
        const std::vector<std::string> coord_parts = string_split( coord_string, '\'' );
        if( coord_parts.empty() || coord_parts.size() > 2 ) {
            return ret_val<tripoint_abs_omt>::make_failure( tripoint_abs_omt::zero,
                    string_format( _( "Error interpreting coordinate: "
                                      "expected an integer or two integers separated by \'; got %s" ), coord_string ) );
        }
        ret_val<int> parsed_coord = try_parse_integer<int>( coord_parts[0], true );
        if( !parsed_coord.success() ) {
            return ret_val<tripoint_abs_omt>::make_failure( tripoint_abs_omt::zero,
                    string_format( _( "Error interpreting coordinate: %s" ), parsed_coord.str() ) );
        }
        int major_coord = parsed_coord.value();
        int minor_coord = 0;
        if( coord_parts.size() >= 2 ) {
            ret_val<int> parsed_coord2 = try_parse_integer<int>( coord_parts[1], true );
            if( !parsed_coord2.success() ) {
                return ret_val<tripoint_abs_omt>::make_failure( tripoint_abs_omt::zero,
                        string_format( _( "Error interpreting coordinate: %s" ), parsed_coord2.str() ) );
            }
            minor_coord = parsed_coord2.value();
        }
        coord_ints.emplace_back( major_coord, minor_coord );
    }
    cata_assert( coord_ints.size() >= 2 );
    return ret_val<tripoint_abs_omt>::make_success( tripoint_abs_omt( OMAPX * coord_ints[0].first +
            coord_ints[0].second,
            OMAPY * coord_ints[1].first + coord_ints[1].second,
            ( coord_ints.size() >= 3 ? coord_ints[2].first : 0 ) ) );
}
