#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <iomanip>

#include "utf8helper.h"

bool utf8helper::is_supported( const std::string &lang ) const
{
    return supported_langs.count( lang ) > 0;
}

size_t utf8helper::char_width( const char &ch ) const
{
    if( !( ch & 128 ) ) { // < 0x7F 0xxxxxxx
        return 1;
    }
    if( !( ch & 32 ) ) { // < 0xE0 110xxxxx
        return 2;
    }
    if( !( ch & 16 ) ) { // < 0xF0 1110xxxx
        return 3;
    }
    if( !( ch & 8 ) ) { // < 0xF8 11110xxx
        return 4;
    }
    return 0; // error
}

std::vector<std::string> utf8helper::split( const std::string &s, bool exclude_ascii ) const
{
    std::vector<std::string> result;
    for( size_t i = 0, width = 0; i < s.length(); i += width ) {
        width = char_width( s[i] );
        if( exclude_ascii && width == 1 ) {
            continue;
        }
        if( !width ) {
            return result; // error
        }
        result.push_back( s.substr( i, width ) );
    }
    return result;
}

utf8helper::utf8helper()
{
    // for toupper, tolower
    std::for_each( langs.begin(), langs.end(), [this]( const std::string & lang ) {
        std::vector<std::string> upper =  split( alphabet[lang + "_upper"], lang != "en" );
        std::vector<std::string> lower =  split( alphabet[lang + "_lower"], lang != "en" );
        if( upper.size() != lower.size() ) {
            throw std::runtime_error( lang + " upper vector size not equal lower size" );
        }
        for( size_t i = 0; i < upper.size(); ++i ) {
            dictionary["toupper"][lower[i]] = upper[i];
            dictionary["tolower"][upper[i]] = lower[i];
            // dictionary[lang+"_toupper"][] = ... if need lang-specific transformation
        }
    } );

    // for case insensitive search
    const std::unordered_map<std::string, std::string> &tolower = dictionary.at( "tolower" );
    dictionary["forsearch"].insert( tolower.begin(), tolower.end() );

    // for sorting
    size_t j = 0, k = 0;
    auto fast_tolower = [&tolower]( const std::string & s ) {
        return tolower.count( s ) ? tolower.at( s ) : s;
    };
    std::for_each( asc.begin(), asc.end(), [this, &j, &k,
          &fast_tolower]( const std::pair<std::string, std::string> &asc_pair ) {
        std::vector<std::string> asc_letters =  split( asc_pair.second );
        const size_t last_index = asc_letters.size() - 1;
        for( size_t i = 0; i < last_index; ++i, ++j ) {
            ascorder[asc_letters[i]] = j;
            ascorder_case_insensitive[asc_letters[i]] = k;
            if( fast_tolower( asc_letters[i] ) != fast_tolower( asc_letters[i + 1] ) ) {
                ++k;
            }
        }
        ascorder[asc_letters[last_index]] = j++;
        ascorder_case_insensitive[asc_letters[last_index]] = k++;
    } );

    // --- SPECIAL CASES ---

    // de "ß" -> "SS" (deprecated since 2017)
    // dictionary["toupper"]["ß"] = "SS";

    // Cyrillic Ёё -> е search insensitive
    // Note: Latin Ëë not changed
    // Warning: MacOS (<=10.15.6) has bug: Latin Ë instead Cyrillic Ё in Russian keyboard layout
    dictionary["forsearch"]["Ё"] = "е";
    dictionary["forsearch"]["ё"] = "е";
}

std::string utf8helper::convert( const std::string &s, const std::string &rules ) const
{
    std::vector<std::string> letters = split( s );
    const std::unordered_map<std::string, std::string> &conv = dictionary.at( rules );
    std::string result;
    std::for_each( letters.begin(), letters.end(), [&conv, &result]( const std::string & letter ) {
        result += conv.count( letter ) ? conv.at( letter ) : letter;
    } );
    return result;
}

std::string utf8helper::to_upper_case( const std::string &s ) const
{
    return convert( s, "toupper" );
}

std::string utf8helper::to_lower_case( const std::string &s ) const
{
    return convert( s, "tolower" );
}

std::string utf8helper::for_search( const std::string &s ) const
{
    return convert( s, "forsearch" );
}

bool utf8helper::compare( const std::string &l, const std::string &r, bool case_sensitive ) const
{
    if( l == r ) {
        return false;
    }
    const std::unordered_map<std::string, size_t> &rules = case_sensitive ? ascorder :
            ascorder_case_insensitive;
    for( size_t i = 0, j = 0, width; i < r.size(); ) {
        if( j >= l.size() ) {
            return true;
        }
        width = char_width( r[i] );
        std::string rc =  r.substr( i, width );
        i += width;
        width = char_width( l[j] );
        std::string lc =  l.substr( j, width );
        j += width;
        if( rules.count( lc ) && rules.count( rc ) ) {
            int result = rules.at( lc ) - rules.at( rc );
            if( result < 0 ) {
                return true;
            }
            if( result > 0 ) {
                return false;
            }
        } else {
            if( std::locale()( lc, rc ) ) {
                return true;
            }
            if( std::locale()( rc, lc ) ) {
                return false;
            }
        }
    }
    return false;
}

bool utf8helper::operator()( const std::string &l, const std::string &r,
                             bool case_sensitive ) const
{
    return compare( l, r, case_sensitive );
}

void utf8helper::print_debug() const
{
    auto fast_compare = [this]( const std::string & l, const std::string & r ) {
        return ascorder.at( l ) < ascorder.at( r );
    };
    std::map<std::string, std::string, decltype( fast_compare )> dic_debug( fast_compare );
    std::for_each( dictionary.begin(),
                   dictionary.end(), [&dic_debug]( const
    std::pair<std::string, std::unordered_map<std::string, std::string>> &part ) {
        std::cout << "dictionary[" << part.first << "]:" << std::endl;
        dic_debug.clear();
        dic_debug.insert( part.second.begin(), part.second.end() );
        std::for_each( dic_debug.begin(),
        dic_debug.end(), []( const std::pair<std::string, std::string>  &letters ) {
            std::cout << letters.first << " -> " << letters.second << ", ";
        } );
        std::cout << std::endl;
    } );

    std::map<std::string, size_t, decltype( fast_compare )> asc_debug( fast_compare );
    auto asc_debug_print = []( const std::pair<std::string, size_t> &debug_pair ) {
        std::cout << "[" << debug_pair.first << ":" << std::setw( 3 ) << debug_pair.second << "] ";
    };
    std::cout << "ascorder:" << std::endl;
    asc_debug.insert( ascorder.begin(), ascorder.end() );
    std::for_each( asc_debug.begin(), asc_debug.end(), asc_debug_print );

    std::cout << std::endl << "ascorder_case_insensitive:" << std::endl;
    asc_debug.clear();
    asc_debug.insert( ascorder_case_insensitive.begin(), ascorder_case_insensitive.end() );
    std::for_each( asc_debug.begin(), asc_debug.end(), asc_debug_print );
    std::cout << std::endl << "---------------------" << std::endl;
}
