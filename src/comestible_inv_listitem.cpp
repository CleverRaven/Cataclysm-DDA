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
#include "comestible_inv_listitem.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <numeric>

void comestible_inv_listitem::print_columns( std::vector<comestible_inv_columns> columns,
        comestible_select_state selected_state, catacurses::window window, int right_bound,
        int cur_print_y )
{
    is_selected = selected_state;
    for( std::vector<comestible_inv_columns>::reverse_iterator col_iter = columns.rbegin();
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

bool comestible_inv_listitem::print_string_column( comestible_inv_columns col,
        std::string &print_str, nc_color &print_col )
{
    //Function is overriden in children. Code is to supress warnings/errors.
    if( col == 0 ) {
        print_str = "";
        print_col = c_white;
    }
    return false;
}

bool comestible_inv_listitem::print_int_column( comestible_inv_columns col, std::string &print_str,
        nc_color &print_col )
{
    //Function is overriden in children. Code is to supress warnings/errors.
    if( col == 0 ) {
        print_str = "";
        print_col = c_white;
    }
    return false;
}

bool comestible_inv_listitem::print_default_columns( comestible_inv_columns col,
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
            if( stacks > 1 ) {
                print_col = menu_color;
                if( stacks > 9999 ) {
                    stacks = 9999;
                    print_col = c_red;
                }
                print_str = string_format( "%4d", stacks );
            } else {
                print_str = "";
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

void comestible_inv_listitem::print_name( catacurses::window window,
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

    int name_startpos = 1;
    nc_color print_color;
    set_print_color( print_color, menu_color );
    if( is_selected ) { //fill whole line with color
        mvwprintz( window, point( 1, cur_print_y ), print_color, "%*s", right_bound, "" );
    }
    int max_name_length = right_bound - name_startpos - 1;
    trim_and_print( window, point( name_startpos, cur_print_y ), max_name_length, print_color, "%s",
                    item_name );
}


char const *comestible_inv_listitem::set_string_params( nc_color &print_color, int value,
        bool need_highlight )
{
    char const *string_format;
    if( value > 0 ) {
        print_color = need_highlight ? c_yellow_green : c_green;
        string_format = "+%d";
    } else if( value < 0 ) {
        print_color = need_highlight ? c_yellow_red : c_red;
        string_format = "%d";
    } else {
        print_color = need_highlight ? c_yellow : c_dark_gray;
        string_format = "";
    }

    set_print_color( print_color, print_color );
    return string_format;
}

void comestible_inv_listitem::set_print_color( nc_color &retval, nc_color default_col )
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

const islot_comestible &comestible_inv_listitem::get_edible_comestible( player &p,
        const item &it ) const
{
    if( it.is_comestible() && p.can_eat( it ).success() ) {
        // Ok since can_eat() returns false if is_craft() is true
        return *it.type->comestible;
    }
    static const islot_comestible dummy{};
    return dummy;
}

static std::string time_to_comestible_str( const time_duration &d )
{
    if( d >= calendar::INDEFINITELY_LONG_DURATION ) {
        return _( "forever" );
    }

    const float day = to_days<float>( d );
    std::string format;
    if( day < 1.00 ) {
        format = _( "%.2f days" );
    } else if( day > 1.00 ) {
        format = _( "%.0f days" );
    } else {
        format = _( "1 day" );
    }

    return string_format( format, day );
}

std::string comestible_inv_listitem::get_time_left_rounded( player &p, const item &it ) const
{
    if( it.is_going_bad() ) {
        return _( "soon!" );
    }

    time_duration time_left = 0_turns;
    const time_duration shelf_life = get_edible_comestible( p, it ).spoils;
    if( shelf_life > 0_turns ) {
        const double relative_rot = it.get_relative_rot();
        time_left = shelf_life - shelf_life * relative_rot;

        // Correct for an estimate that exceeds shelf life -- this happens especially with
        // fresh items.
        if( time_left > shelf_life ) {
            time_left = shelf_life;
        }
    }

    time_duration divider = 0_turns;
    time_duration vicinity = 0_turns;

    if( time_left > 1_days ) {
        divider = 1_days;
        vicinity = 2_hours;
    } else if( time_left > 1_hours ) {
        divider = 1_hours;
        vicinity = 5_minutes;
    } // Minutes and seconds can be estimated precisely.

    if( divider != 0_turns ) {
        const time_duration remainder = time_left % divider;

        if( remainder >= divider - vicinity ) {
            time_left += divider;
        } else if( remainder > vicinity ) {
            return time_to_comestible_str( time_left );
        }
    }
    return time_to_comestible_str( time_left );
}

struct sort_case_insensitive_less : public std::binary_function< char, char, bool > {
    bool operator()( char x, char y ) const {
        return toupper( static_cast<unsigned char>( x ) ) < toupper( static_cast<unsigned char>( y ) );
    }
};

std::function<bool( const comestible_inv_listitem *d1, const comestible_inv_listitem *d2 )>
comestible_inv_listitem::get_sort_function( comestible_inv_columns sortby,
        comestible_inv_columns default_sortby )
{
    using itm = comestible_inv_listitem;
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

std::function<bool( const comestible_inv_listitem *d1, const comestible_inv_listitem *d2, bool &retval )>
comestible_inv_listitem::compare_function( comestible_inv_columns sortby )
{
    using itm = comestible_inv_listitem;
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

comestible_inv_listitem::comestible_inv_listitem( const std::list<item *> &list, int index,
        comestible_inv_area &area, bool from_vehicle ) :
    idx( index ),
    area( area ),
    id( list.front()->typeId() ),
    items( list ),
    name( list.front()->tname( list.size() ) ),
    name_without_prefix( list.front()->tname( 1, false ) ),
    autopickup( get_auto_pickup().has_rule( list.front() ) ),
    stacks( list.size() ),
    volume( list.front()->volume() * stacks ),
    weight( list.front()->weight() * stacks ),
    cat( &list.front()->get_category() ),
    from_vehicle( from_vehicle )
{
    init( g->u.get_consumable_from( *list.front() ) );
}

void comestible_inv_listitem::init( const item &it )
{
    menu_color = it.color_in_inventory();
    menu_color_dark = c_dark_gray;
    assert( stacks >= 1 );
}

// area is not used when listitem is a category, so AIM_SOUTHWEST is fine
static comestible_inv_area DUMMY_AREA = comestible_inv_area( comestible_inv_area::get_info(
        comestible_inv_area_info::AIM_SOUTHWEST ) );
comestible_inv_listitem::comestible_inv_listitem( const item_category *cat )
    : idx()
    , area( DUMMY_AREA )
    , id( "null" )
    , autopickup()
    , stacks()
    , cat( cat )
{
    if( cat != nullptr ) {
        name = cat->name();
    }
    menu_color_dark = c_dark_gray;
}

bool comestible_inv_listitem::is_category_header() const
{
    return items.empty() && cat != nullptr;
}

bool comestible_inv_listitem::is_item_entry() const
{
    return !items.empty();
}


/*******    FOOD    *******/


comestible_inv_listitem_food::comestible_inv_listitem_food( const item_category *cat ):
    comestible_inv_listitem( cat )
{
}

comestible_inv_listitem_food::comestible_inv_listitem_food( const std::list<item *> &list,
        int index,
        comestible_inv_area &area, bool from_vehicle ): comestible_inv_listitem( list, index, area,
                    from_vehicle )
{
    init( g->u.get_consumable_from( *list.front() ) );
}

void comestible_inv_listitem_food::init( const item &it )
{
    player &p = g->u;
    islot_comestible edible = get_edible_comestible( p, it );



    menu_color = it.color_in_inventory();

    // statuses
    cond.reserve( cond_size );
    const bool eat_verb = it.has_flag( "USE_EAT_VERB" );
    const bool is_food = eat_verb || edible.comesttype == "FOOD";
    const bool is_drink = !eat_verb && edible.comesttype == "DRINK";

    bool craft_only = false;
    bool warm_only = false;

    if( it.is_craft() ) {
        craft_only = true;
    } else if( is_food || is_drink ) {
        for( const auto &elem : it.type->materials ) {
            if( !elem->edible() ) {
                craft_only = true;
                break;
            }
        }
    }
    if( !craft_only && it.item_tags.count( "FROZEN" ) && !it.has_flag( "EDIBLE_FROZEN" ) &&
        !it.has_flag( "MELTS" ) ) {
        warm_only = true;
    }
    input_context ctxt( "COMESTIBLE_INVENTORY" );
    if( craft_only ) {
        cond.emplace_back( ctxt.get_desc( "CRAFT_WITH" )[0], c_cyan );
    } else if( warm_only ) {
        cond.emplace_back( ctxt.get_desc( "HEAT_UP" )[0], c_cyan );
    } else {
        cond.emplace_back( ' ', c_cyan );
    }

    if( it.has_flag( "MUSHY" ) ) {
        cond.emplace_back( 'm', c_yellow );
    } else if( edible.stim > 0 ) {
        cond.emplace_back( 's', c_yellow );
    } else if( edible.stim < 0 ) {
        cond.emplace_back( 'd', c_yellow );
    } else {
        cond.emplace_back( ' ', c_cyan );
    }

    //These numbers are arbitrary.
    if( edible.addict > 7 ) {
        if( edible.addict < 20 ) {
            cond.emplace_back( 'a', c_yellow );
        } else {
            cond.emplace_back( 'a', c_red );
        }
    } else {
        cond.emplace_back( ' ', c_cyan );
    }

    eat_error = p.can_eat( it );

    exipres_in = "";
    shelf_life = "";
    if( edible.spoils > 0_turns ) {
        if( !it.rotten() ) {
            exipres_in = get_time_left_rounded( p, it );
        }
        shelf_life = to_string_clipped( edible.spoils );
    }

    calories = p.kcal_for( it );
    quench = edible.quench;
    joy = p.fun_for( it ).first;
}
void comestible_inv_listitem_food::print_columns( std::vector<comestible_inv_columns> columns,
        comestible_select_state selected_state, catacurses::window window, int right_bound,
        int cur_print_y )
{
    // if it's ineddible, but cookable or reheatable we want to show full data
    if( !eat_error.success() && eat_error.value() != edible_rating::INEDIBLE ) {
        is_selected = selected_state;
        const int max_x = right_bound;

        //basically same code as base, except we skip few columns and replace them with error message
        //we still want to print base columns and find position to print error from, so keep the loop
        for( std::vector<comestible_inv_columns>::reverse_iterator col_iter = columns.rbegin();
             col_iter != columns.rend(); ++col_iter ) {
            std::string print_str;
            nc_color print_col;

            const int w = get_col_data( *col_iter ).width;
            if( w <= 0 ) { // internal column, not meant for display
                continue;
            }
            right_bound -= w;

            //can't put this as return value from one of the columns, because we want to print in a non-standard way
            if( *col_iter == COLUMN_CALORIES ) {
                print_col = is_selected ? hilite( c_white ) : c_red;
                print_str = string_format( " %s", eat_error.str() );
                int width = max_x - right_bound;
                trim_and_print( window, point( right_bound, cur_print_y ), width, print_col, "%*s", width,
                                print_str );
                continue;
            }

            // if (true) - we found the column, and set data for it; if (false) - try to find in another type
            if( print_string_column( *col_iter, print_str, print_col ) ) {
                continue; //skip
            } else if( print_int_column( *col_iter, print_str, print_col ) ) {
                continue; //skip
            } else if( print_default_columns( *col_iter, print_str, print_col ) ) {
            } else {
                //we encountered a column that had no printing code
                assert( false );
            }

            mvwprintz( window, point( right_bound, cur_print_y ), print_col, "%*s", w, print_str );
        }
        print_name( window, right_bound, cur_print_y );
    } else {
        comestible_inv_listitem::print_columns( columns, selected_state, window, right_bound, cur_print_y );
    }
}

bool comestible_inv_listitem_food::print_string_column( comestible_inv_columns col,
        std::string &print_str, nc_color &print_col )
{
    switch( col ) {
        case COLUMN_SHELF_LIFE:
            print_str = shelf_life;
            break;
        case COLUMN_EXPIRES:
            print_str = exipres_in;
            break;
        default:
            return false;
    }

    set_print_color( print_col, c_cyan );
    return true;
}

bool comestible_inv_listitem_food::print_int_column( comestible_inv_columns col,
        std::string &print_str, nc_color &print_col )
{
    int print_val;
    switch( col ) {
        case COLUMN_CALORIES:
            print_val = calories;
            break;
        case COLUMN_QUENCH:
            print_val = quench;
            break;
        case COLUMN_JOY:
            print_val = joy;
            break;
        default:
            return false;
            break;
    }
    char const *print_format = set_string_params( print_col, print_val, false );
    print_str = string_format( print_format, print_val );
    return true;
}

void comestible_inv_listitem_food::print_name( catacurses::window window,
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

    if( get_option<bool>( "ITEM_SYMBOLS" ) ) {
        item_name = string_format( "%s %s", it->symbol(), item_name );
    }

    int name_startpos = cond_size + 1;
    nc_color print_color;
    for( size_t i = 0; i < cond.size(); i++ ) {
        auto c = cond[i];
        set_print_color( print_color, c.second );
        mvwprintz( window, point( i + 1, cur_print_y ), print_color, "%c", c.first );
    }
    set_print_color( print_color, menu_color );
    if( is_selected ) { //fill whole line with color
        mvwprintz( window, point( cond.size() + 1, cur_print_y ), print_color, "%*s",
                   right_bound - cond.size(), "" );
    }
    int max_name_length = right_bound - name_startpos - 1;
    trim_and_print( window, point( name_startpos, cur_print_y ), max_name_length, print_color, "%s",
                    item_name );
}

std::function<bool( const comestible_inv_listitem *d1, const comestible_inv_listitem *d2, bool &retval )>
comestible_inv_listitem_food::compare_function( comestible_inv_columns sortby )
{
    auto f = comestible_inv_listitem::compare_function( sortby );
    if( f != nullptr ) {
        return f;
    }

    using itm = comestible_inv_listitem;
    using itmf = comestible_inv_listitem_food;
    switch( sortby ) {
        case COLUMN_CALORIES:
            return []( const itm * d1, const itm * d2, bool & retval ) {
                const itmf *d1f = static_cast<const itmf *>( d1 );
                const itmf *d2f = static_cast<const itmf *>( d2 );
                retval = d1f->calories > d2f->calories;
                return d1f->calories != d2f->calories;
            };
        case COLUMN_QUENCH:
            return []( const itm * d1, const itm * d2, bool & retval ) {
                const itmf *d1f = static_cast<const itmf *>( d1 );
                const itmf *d2f = static_cast<const itmf *>( d2 );
                retval = d1f->quench > d2f->quench;
                return d1f->quench != d2f->quench;
            };
        case COLUMN_JOY:
            return []( const itm * d1, const itm * d2, bool & retval ) {
                const itmf *d1f = static_cast<const itmf *>( d1 );
                const itmf *d2f = static_cast<const itmf *>( d2 );
                retval = d1f->joy > d2f->joy;
                return d1f->joy != d2f->joy;
            };
        case COLUMN_EXPIRES:
        case COLUMN_SHELF_LIFE:
            return []( const itm * d1, const itm * d2, bool & retval ) {
                const itmf *d1f = static_cast<const itmf *>( d1 );
                const itmf *d2f = static_cast<const itmf *>( d2 );
                int s1 = d1f->items.front()->spoilage_sort_order();
                int s2 = d2f->items.front()->spoilage_sort_order();
                retval = s1 < s2;
                return s1 != s2;
            };
        default:
            return nullptr;
    }
}

/*******    BIO    *******/

comestible_inv_listitem_bio::comestible_inv_listitem_bio( const item_category *cat ) :
    comestible_inv_listitem( cat )
{
}

comestible_inv_listitem_bio::comestible_inv_listitem_bio( const std::list<item *> &list, int index,
        comestible_inv_area &area, bool from_vehicle ) : comestible_inv_listitem( list, index, area,
                    from_vehicle )
{
    energy = g->u.get_acquirable_energy( *list.front() );
}

bool comestible_inv_listitem_bio::print_int_column( comestible_inv_columns col,
        std::string &print_str, nc_color &print_col )
{
    int print_val;
    bool need_highlight = false;
    switch( col ) {
        case COLUMN_ENERGY:
            print_val = energy;
            break;
        default:
            return false;
            break;
    }
    char const *print_format = set_string_params( print_col, print_val, need_highlight );
    print_str = string_format( print_format, print_val );
    return true;
}

std::function<bool( const comestible_inv_listitem *d1, const comestible_inv_listitem *d2, bool &retval )>
comestible_inv_listitem_bio::compare_function( comestible_inv_columns sortby )
{
    auto f = comestible_inv_listitem::compare_function( sortby );
    if( f != nullptr ) {
        return f;
    }

    using itm = comestible_inv_listitem;
    using itmb = comestible_inv_listitem_bio;
    switch( sortby ) {
        case COLUMN_ENERGY:
            return []( const itm * d1, const itm * d2, bool & retval ) {
                const itmb *d1f = static_cast<const itmb *>( d1 );
                const itmb *d2f = static_cast<const itmb *>( d2 );
                retval = d1f->energy > d2f->energy;
                return d1f->energy != d2f->energy;
            };
        default:
            return nullptr;
    }
}


