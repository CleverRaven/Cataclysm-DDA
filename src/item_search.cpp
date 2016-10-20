#include "item.h"
#include "material.h"
#include "item_search.h"
#include "cata_utility.h"
#include "output.h"

#include <algorithm>

std::pair<std::string, std::string> get_both( const std::string &a );
std::function<bool( const item & )>
item_filter_from_string(  std::string filter )
{
    if( filter.empty() ) {
        // Variable without name prevents unused parameter warning
        return [](const item&) {
            return true;
        };
    }
    if( filter.find( "," ) != std::string::npos ){
        std::vector<std::function<bool( const item& )> > functions;
        size_t comma;
        while( ( comma = filter.find( "," ) ) != std::string::npos ) {
            const auto& current_filter = filter.substr( 0, comma );
            if( current_filter.empty() ){
                continue;
            }
            auto current_func = item_filter_from_string( current_filter );
            functions.push_back( current_func );
            filter = trim( filter.substr( comma + 1 ) );
        }
        if( !filter.empty() ){
            functions.push_back( item_filter_from_string( filter ) );
        }
        return [functions](const item& i) {
            return std::any_of( functions.begin(), functions.end(),
            [i](std::function<bool(const item&)> f){
                return f( i );
            } );
        };
    }
    size_t colon;
    char flag = '\0';
    if( ( colon = filter.find( ":" ) ) != std::string::npos ) {
        if( colon >= 1 ) {
            flag = filter[colon - 1];
            filter = filter.substr( colon + 1 );
        }
    }
    bool exclude = filter[0] == '-';
    if( exclude ) {
        return [filter](const item& i){
            return !item_filter_from_string( filter.substr( 1 ) )( i );
        };
    }
    switch( flag ) {
        case 'c'://category
            return [filter]( const item & i ) {
                return lcmatch( i.get_category().name, filter );
            };
            break;
        case 'm'://material
            return [filter]( const item & i ) {
                return lcmatch( i.get_base_material().name(), filter );
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
