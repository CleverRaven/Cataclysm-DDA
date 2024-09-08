#include "item_search.h"

#include <map>
#include <utility>

#include "avatar.h"
#include "cata_utility.h"
#include "item.h"
#include "item_category.h"
#include "itype.h"
#include "material.h"
#include "requirements.h"
#include "type_id.h"

static std::pair<std::string, std::string> get_both( std::string_view a );

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
        // category
        case 'c':
            return [filter]( const item & i ) {
                return lcmatch( i.get_category_of_contents().name_header(), filter );
            };
        // material
        case 'm':
            return [filter]( const item & i ) {
                return std::any_of( i.made_of().begin(), i.made_of().end(),
                [&filter]( const std::pair<material_id, int> &mat ) {
                    return lcmatch( mat.first->name(), filter );
                } );
            };
        // qualities
        case 'q':
            return [filter]( const item & i ) {
                return i.type->has_any_quality( filter );
            };
        // both
        case 'b':
            return [filter]( const item & i ) {
                auto pair = get_both( filter );
                return item_filter_from_string( pair.first )( i )
                       && item_filter_from_string( pair.second )( i );
            };
        // disassembled components
        case 'd':
            return [filter]( const item & i ) {
                const auto &components = i.get_uncraft_components();
                for( const item_comp &component : components ) {
                    if( lcmatch( component.to_string(), filter ) ) {
                        return true;
                    }
                }
                return false;
            };
        // item notes
        case 'n':
            return [filter]( const item & i ) {
                const std::string note = i.get_var( "item_note" );
                return !note.empty() && lcmatch( note, filter );
            };
        // item flags, must type in whole flag string name(case insensitive) so as to avoid revealing hidden flags.
        case 'f':
            return [filter]( const item & i ) {
                std::string flag_filter = filter;
                transform( flag_filter.begin(), flag_filter.end(), flag_filter.begin(), ::toupper );
                const flag_id fsearch( flag_filter );
                if( fsearch.is_valid() ) {
                    return i.has_flag( fsearch );
                } else {
                    return false;
                }
            };
        // by book skill
        case 's':
            return [filter]( const item & i ) {
                if( get_avatar().has_identified( i.typeId() ) ) {
                    return lcmatch( i.get_book_skill(), filter );
                }
                return false;
            };
        // covers bodypart
        case 'v': {
            std::unordered_set<bodypart_id> filtered_bodyparts;
            std::unordered_set<sub_bodypart_id> filtered_sub_bodyparts;
            for( const body_part &bp : all_body_parts ) {
                const bodypart_str_id &bp_str_id = convert_bp( bp );
                if( lcmatch( body_part_name( bp_str_id, 1 ), filter )
                    || lcmatch( body_part_name( bp_str_id, 2 ), filter ) ) {
                    filtered_bodyparts.insert( bp_str_id->id );
                }
                for( const sub_bodypart_str_id &sbp : bp_str_id->sub_parts ) {
                    if( lcmatch( sbp->name.translated(), filter )
                        || lcmatch( sbp->name_multiple.translated(), filter ) ) {
                        filtered_sub_bodyparts.insert( sbp->id );
                    }
                }
            }
            return [filter, filtered_bodyparts, filtered_sub_bodyparts]( const item & i ) {
                return std::any_of( filtered_bodyparts.begin(), filtered_bodyparts.end(),
                [&i]( const bodypart_id & bp ) {
                    return i.covers( bp );
                } )
                || std::any_of( filtered_sub_bodyparts.begin(), filtered_sub_bodyparts.end(),
                [&i]( const sub_bodypart_id & sbp ) {
                    return i.covers( sbp );
                } );
            };
        }
        // by name
        default:
            return [filter]( const item & a ) {
                return lcmatch( remove_color_tags( a.tname() ), filter );
            };
    }
}

std::function<bool( const item & )> item_filter_from_string( const std::string &filter )
{
    return filter_from_string<item>( filter, basic_item_filter );
}

std::pair<std::string, std::string> get_both( const std::string_view a )
{
    size_t split_mark = a.find( ';' );
    return std::pair( std::string( a.substr( 0, split_mark ) ),
                      std::string( a.substr( split_mark + 1 ) ) );
}
