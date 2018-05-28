#include "item_search.h"
#include "item.h"
#include "material.h"
#include "cata_utility.h"
#include "output.h"
#include "item_category.h"

#include <algorithm>

std::pair<std::string, std::string> get_both( const std::string &a );
std::function<bool( const item & )>
item_filter_from_string( std::string filter )
{
    if( filter.empty() ) {
        // Variable without name prevents unused parameter warning
        return []( const item & ) {
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
        std::vector<std::function<bool( const item & )> > functions;
        // Functions that must all return true
        std::vector<std::function<bool( const item & )> > inv_functions;
        size_t comma = filter.find( ',' );
        while( !filter.empty() ) {
            const auto &current_filter = trim( filter.substr( 0, comma ) );
            if( !current_filter.empty() ) {
                auto current_func = item_filter_from_string( current_filter );
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

        return [functions, inv_functions]( const item & it ) {
            auto apply = [&]( const std::function<bool( const item & )> &func ) {
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
        return [filter]( const item & i ) {
            return !item_filter_from_string( filter.substr( 1 ) )( i );
        };
    }
    size_t colon;
    char flag = '\0';
    if( ( colon = filter.find( ':' ) ) != std::string::npos ) {
        if( colon >= 1 ) {
            flag = filter[colon - 1];
            filter = filter.substr( colon + 1 );
        }
    }
    switch( flag ) {
        case 'c'://category
            return [filter]( const item & i ) {
                return lcmatch( i.get_category().name(), filter );
            };
            break;
        case 'm'://material
            return [filter]( const item & i ) {
                return std::any_of( i.made_of().begin(), i.made_of().end(),
                [&filter]( const material_id & mat ) {
                    return lcmatch( mat->name(), filter );
                } );
            };
            break;
        case 'b'://both
            return [filter]( const item & i ) {
                auto pair = get_both( filter );
                return item_filter_from_string( pair.first )( i )
                       && item_filter_from_string( pair.second )( i );
            };
            break;
        default://by name
            return [filter]( const item & a ) {
                return lcmatch( a.tname(), filter );
            };
            break;
    }
}
std::pair<std::string, std::string> get_both( const std::string &a )
{
    size_t split_mark = a.find( ';' );
    return std::make_pair( a.substr( 0, split_mark - 1 ),
                           a.substr( split_mark + 1 ) );
}
