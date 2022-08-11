#include "try_parse_integer.h"

#include <sstream>

#include "translations.h"

template<typename T>
ret_val<T> try_parse_integer( const std::string &s, bool use_locale )
{
    // Using stringstream-based parsing because that's the only one in the
    // standard library for which it's possible to turn off the
    // locale-dependency.  Once we're using C++17 we could use a combination of
    // std::from_chars and std::strto* functions, but this should be fine so
    // long as the code is not performance-critical.
    std::istringstream buffer( s );
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

template ret_val<int> try_parse_integer<int>( const std::string &, bool use_locale );
template ret_val<long> try_parse_integer<long>( const std::string &, bool use_locale );
template ret_val<long long> try_parse_integer<long long>( const std::string &, bool use_locale );
