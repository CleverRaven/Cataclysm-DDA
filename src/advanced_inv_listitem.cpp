#include "auto_pickup.h"
#include "avatar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "input.h"
#include "item_category.h"
#include "item_search.h"
#include "item_stack.h"
#include "options.h"
#include "output.h"
#include "player.h"
#include "player_activity.h"
#include "string_formatter.h"
#include "color.h"
#include "int_id.h"
#include "item.h"
#include "ret_val.h"
#include "type_id.h"
#include "enums.h"
#include "faction.h"
#include "material.h"
#include "advanced_inv_listitem.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <numeric>

void advanced_inv_listitem::print_columns( std::vector<advanced_inv_columns> columns,
        advanced_inv_select_state selected_state, catacurses::window window, int right_bound,
        int cur_print_y )
{
    is_selected = selected_state;
    for( std::vector<advanced_inv_columns>::reverse_iterator col_iter = columns.rbegin();
         col_iter != columns.rend(); ++col_iter ) {
        std::string print_str;
        nc_color print_col;
        const int w = get_col_data( *col_iter ).width;

        if( w <= 0 ) { // internal column, not meant for display
            continue;
        }
        // if (true) - we found the column, and set data for it; if (false) - try to find in another type
        if( print_string_column( *col_iter, print_str, print_col ) ) {
        } else if( print_int_column( *col_iter, print_str, print_col ) ) {
        } else if( print_default_columns( *col_iter, print_str, print_col ) ) {
        } else {
            //we encountered a column that had no printing code
            assert( false );
        }

        right_bound -= w;
        mvwprintz( window, point( right_bound, cur_print_y ), print_col, "%*s", w, print_str );
    }
    print_name( window, right_bound, cur_print_y );
}

bool advanced_inv_listitem::print_string_column( advanced_inv_columns,
        std::string &, nc_color & )
{
    return false;
}

bool advanced_inv_listitem::print_int_column( advanced_inv_columns, std::string &,
        nc_color & )
{
    return false;
}

bool advanced_inv_listitem::print_default_columns( advanced_inv_columns col,
        std::string &print_str, nc_color &print_col )
{
    switch( col ) {
        case COLUMN_VOLUME: {
            bool it_vol_truncated = false;
            double it_vol_value = 0.0;
            print_str = format_volume( volume, 5, &it_vol_truncated, &it_vol_value );
            if( it_vol_truncated && it_vol_value > 0.0 ) {
                print_col = c_red;
            } else {
                print_col = volume.value() > 0 ? menu_color : menu_color_dark;
            }
            break;
        }

        case COLUMN_WEIGHT: {
            double it_weight = convert_weight( weight );
            size_t w_precision;
            print_col = it_weight > 0 ? menu_color : menu_color_dark;

            if( it_weight >= 1000.0 ) {
                if( it_weight >= 10000.0 ) {
                    print_col = c_red;
                    it_weight = 9999.0;
                }
                w_precision = 0;
            } else if( it_weight >= 100.0 ) {
                w_precision = 1;
            } else {
                w_precision = 2;
            }
            print_str = string_format( "%5.*f", w_precision, it_weight );
            break;
        }
        case COLUMN_AMOUNT: {
            print_col = menu_color;
            if( stacks > 1 ) {
                if( stacks > 9999 ) {
                    stacks = 9999;
                    print_col = c_red;
                }
                print_str = string_format( "%4d", stacks );
            } else {
                print_str.clear();
            }
            break;
        }
        case COLUMN_SRC:
            print_col = menu_color;
            print_str = area.info.get_shortname();
            break;

        default:
            return false;
            break;
    }
    set_print_color( print_col, print_col );
    return true;
}

void advanced_inv_listitem::print_name( catacurses::window window,
                                        int right_bound, int cur_print_y )
{
    std::string item_name;

    item *it = items.front();
    if( it->is_money() ) {
        //Count charges
        unsigned int charges_total = 0;
        for( const auto item : items ) {
            charges_total += item->charges;
        }
        item_name = it->display_money( items.size(), charges_total );
    } else {
        item_name = it->display_name();
    }
    //check stolen
    if( it->has_owner() ) {
        const faction *item_fac = it->get_owner();
        if( item_fac != g->faction_manager_ptr->get( faction_id( "your_followers" ) ) ) {
            item_name = colorize( "!", c_light_red ) + item_name;
        }
    }
    if( get_option<bool>( "ITEM_SYMBOLS" ) ) {
        item_name = string_format( "%s %s", it->symbol(), item_name );
    }

    int name_startpos = cond_size + 1;
    nc_color print_color;
    for( size_t i = 0; i < cond.size(); i++ ) {
        auto c = cond[i];
        //set_print_color(print_color, c.second);
        mvwprintz( window, point( i + 1, cur_print_y ), c.second, "%c", c.first );
    }
    set_print_color( print_color, menu_color );
    if( is_selected ) { //fill whole line with color
        mvwprintz( window, point( name_startpos, cur_print_y ), print_color, "%*s",
                   right_bound - name_startpos, "" );
    }
    int max_name_length = right_bound - name_startpos - 1;
    trim_and_print( window, point( name_startpos, cur_print_y ), max_name_length, print_color, "%s",
                    item_name );
}

void advanced_inv_listitem::set_print_color( nc_color &retval, nc_color default_col )
{
    switch( is_selected ) {
        case SELECTSTATE_NONE:
            retval = default_col;
            break;
        case SELECTSTATE_SELECTED:
            retval = hilite( c_white );
            break;
        case SELECTSTATE_CATEGORY:
            retval = c_white_red;
            break;
        default:
            break;
    }
}

std::function<bool( const advanced_inv_listitem *d1, const advanced_inv_listitem *d2 )>
advanced_inv_listitem::get_sort_function( advanced_inv_columns sortby,
        advanced_inv_columns default_sortby )
{
    using itm = advanced_inv_listitem;
    return [ = ]( const itm * d1, const itm * d2 ) {
        auto f1 = compare_function( sortby );
        auto f2 = compare_function( default_sortby );

        bool retval = false;
        if( !f1( d1, d2, retval ) ) {
            f2( d1, d2, retval );
        }
        return retval;
    };
}

std::function<bool( const advanced_inv_listitem *d1, const advanced_inv_listitem *d2, bool &retval )>
advanced_inv_listitem::compare_function( advanced_inv_columns sortby )
{
    using itm = advanced_inv_listitem;
    switch( sortby ) {
        case COLUMN_NAME:
            return []( const itm * d1, const itm * d2, bool & retval ) {
                const std::string *n1;
                const std::string *n2;
                if( d1->name_without_prefix == d2->name_without_prefix ) {
                    if( d1->name == d2->name ) {
                        retval = false;
                        return false;
                    } else {
                        //if names without prefix equal, compare full name
                        n1 = &d1->name;
                        n2 = &d2->name;
                    }
                } else {
                    //else compare name without prefix
                    n1 = &d1->name_without_prefix;
                    n2 = &d2->name_without_prefix;
                }

                retval = std::lexicographical_compare( n1->begin(), n1->end(), n2->begin(), n2->end(),
                                                       sort_case_insensitive_less() );
                return true;
            };
        case COLUMN_WEIGHT:
            return []( const itm * d1, const itm * d2, bool & retval ) {
                retval = d1->weight > d2->weight;
                return d1->weight != d2->weight;
            };
        case COLUMN_VOLUME:
            return []( const itm * d1, const itm * d2, bool & retval ) {
                retval = d1->volume > d2->volume;
                return d1->volume != d2->volume;
            };
        case COLUMN_SORTBY_NONE:
            return []( const itm * d1, const itm * d2, bool & retval ) {
                retval = d1->idx < d2->idx;
                return d1->idx != d2->idx;
            };
            break;
        case COLUMN_SORTBY_CATEGORY:
            return []( const itm * d1, const itm * d2, bool & retval ) {
                assert( d1->cat != nullptr );
                assert( d2->cat != nullptr );
                if( d1->cat != d2->cat ) {
                    retval = *d1->cat < *d2->cat;
                    return true;
                } else if( d1->is_category_header() ) {
                    retval = true;
                    return true;
                } else if( d2->is_category_header() ) {
                    retval = false;
                    return true;
                }
                retval = false;
                return false;
            };
        default:
            return nullptr;
    }
}

advanced_inv_listitem::advanced_inv_listitem( const std::list<item *> &list, int index,
        advanced_inv_area &area, bool from_vehicle ) :
    idx( index ),
    stacks( list.size() ),
    volume( list.front()->volume() * stacks ),
    weight( list.front()->weight() * stacks ),
    items( list ),
    name( list.front()->tname( list.size() ) ),
    name_without_prefix( list.front()->tname( 1, false ) ),
    autopickup( get_auto_pickup().has_rule( list.front() ) ),
    cat( &list.front()->get_category() ),
    from_vehicle( from_vehicle ),
    id( list.front()->typeId() ),
    area( area )
{
    init( *list.front() );
}

void advanced_inv_listitem::init( const item &it )
{
    menu_color = it.color_in_inventory();
    menu_color_dark = c_dark_gray;
    assert( stacks >= 1 );
}

// area is not used when listitem is a category, so AIM_SOUTHWEST is fine
static advanced_inv_area DUMMY_AREA = advanced_inv_area( advanced_inv_area::get_info(
        advanced_inv_area_info::AIM_SOUTHWEST ) );
advanced_inv_listitem::advanced_inv_listitem( const item_category *cat )
    : idx()
    , stacks()
    , autopickup()
    , cat( cat )
    , id( "null" )
    , area( DUMMY_AREA )
{
    if( cat != nullptr ) {
        name = cat->name();
    }
    menu_color_dark = c_dark_gray;
}

bool advanced_inv_listitem::is_category_header() const
{
    return items.empty() && cat != nullptr;
}

bool advanced_inv_listitem::is_item_entry() const
{
    return !items.empty();
}


