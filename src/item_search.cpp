#include "item_search.h"
#include "item.h"
#include "material.h"
#include "cata_utility.h"
#include "item_category.h"
#include "recipe_dictionary.h"

std::pair<std::string, std::string> get_both( const std::string &a );

std::function<bool( const item & )> basic_item_filter( std::string filter )
{
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
        case 'm'://material
            return [filter]( const item & i ) {
                return std::any_of( i.made_of().begin(), i.made_of().end(),
                [&filter]( const material_id & mat ) {
                    return lcmatch( mat->name(), filter );
                } );
            };
		case 'q'://qualities
			return [filter](const item & i) {
				return std::any_of(i.quality_of().begin(), i.quality_of().end(),
					[&filter](const std::pair<quality_id, int> &e) {
					return lcmatch(e.first->name, filter);
				});
			};
        case 'b'://both
            return [filter]( const item & i ) {
                auto pair = get_both( filter );
                return item_filter_from_string( pair.first )( i )
                       && item_filter_from_string( pair.second )( i );
            };
        default://by name
            return [filter]( const item & a ) {
                return lcmatch( a.tname(), filter );
            };
    }
}

std::function<bool( const item & )> item_filter_from_string( const std::string &filter )
{
    return filter_from_string<item>( filter, basic_item_filter );
}

std::pair<std::string, std::string> get_both( const std::string &a )
{
    size_t split_mark = a.find( ';' );
    return std::make_pair( a.substr( 0, split_mark - 1 ),
                           a.substr( split_mark + 1 ) );
}
