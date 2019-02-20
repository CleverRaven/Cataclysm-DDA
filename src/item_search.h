#pragma once
#ifndef ITEM_SEARCH_H
#define ITEM_SEARCH_H

#include <algorithm>
#include <functional>
#include <string>

#include "output.h"

/**
 * Get a function that returns true if the value matches the query.
 */
template<typename T>
std::function<bool( const T & )> filter_from_string( std::string filter,
        std::function<std::function<bool( const T & )>( const std::string & )> basic_filter )
{
    if( filter.empty() ) {
        // Variable without name prevents unused parameter warning
        return []( const T & ) {
            return true;
        };
    }

    // remove curly braces (they only get in the way)
    if( filter.find( '{' ) != std::string::npos ) {
        filter.erase( std::remove( filter.begin(), filter.end(), '{' ) );
    }
    if( filter.find( '}' ) != std::string::npos ) {
        filter.erase( std::remove( filter.begin(), filter.end(), '}' ) );
    }
    if( filter.find( ',' ) != std::string::npos ) {
        // functions which only one of which must return true
        std::vector<std::function<bool( const T & )> > functions;
        // Functions that must all return true
        std::vector<std::function<bool( const T & )> > inv_functions;
        size_t comma = filter.find( ',' );
        while( !filter.empty() ) {
            const auto &current_filter = trim( filter.substr( 0, comma ) );
            if( !current_filter.empty() ) {
                auto current_func = filter_from_string( current_filter, basic_filter );
                if( current_filter[0] == '-' ) {
                    inv_functions.push_back( current_func );
                } else {
                    functions.push_back( current_func );
                }
            }
            if( comma != std::string::npos ) {
                filter = trim( filter.substr( comma + 1 ) );
                comma = filter.find( ',' );
            } else {
                break;
            }
        }

        return [functions, inv_functions]( const T & it ) {
            auto apply = [&]( const std::function<bool( const T & )> &func ) {
                return func( it );
            };
            bool p_result = std::any_of( functions.begin(), functions.end(),
                                         apply );
            bool n_result = std::all_of(
                                inv_functions.begin(),
                                inv_functions.end(),
                                apply );
            if( !functions.empty() && inv_functions.empty() ) {
                return p_result;
            }
            if( functions.empty() && !inv_functions.empty() ) {
                return n_result;
            }
            return p_result && n_result;
        };
    }
    bool exclude = filter[0] == '-';
    if( exclude ) {
        return [filter, basic_filter]( const T & i ) {
            return !filter_from_string( filter.substr( 1 ), basic_filter )( i );
        };
    }

    return basic_filter( filter );
}

class item;

/**
 * Get a function that returns true if the item matches the query.
 */
std::function<bool( const item & )> item_filter_from_string( const std::string &filter );

/**
 * Get a function that returns true if the value matches the basic query (no commas or minuses).
 */
std::function<bool( const item & )> basic_item_filter( std::string filter );

#endif
