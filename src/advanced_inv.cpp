#include "game.h"
#include "output.h"
#include "map.h"
#include "debug.h"
#include "catacharset.h"
#include "translations.h"
#include "uistate.h"
#include "auto_pickup.h"
#include "messages.h"
#include "player_activity.h"
#include "advanced_inv.h"
#include "compatibility.h"
#include "enums.h"
#include "input.h"
#include "options.h"
#include "ui.h"
#include "trap.h"
#include "itype.h"
#include "vehicle.h"

#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <sstream>
#include <cmath>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <cstring>

advanced_inventory::advanced_inventory()
    : head_height( 5 )
    , min_w_height( 10 )
    , min_w_width( FULL_SCREEN_WIDTH )
    , max_w_width( 120 )
    , inCategoryMode( false )
    , recalc( true )
    , redraw( true )
    , src( left )
    , dest( right )
    , filter_edit( false )
      // panes don't need initialization, they are recalculated immediately
    , squares(
{ {                //    y  x    x    y   z
        { AIM_INVENTORY, 2, 25, {0,   0,  0}, _( "Inventory" ),          _( "IN" ) },
        { AIM_SOUTHWEST, 3, 30, {-1,  1,  0}, _( "South West" ),         _( "SW" ) },
        { AIM_SOUTH,     3, 33, {0,   1,  0}, _( "South" ),              _( "S" )  },
        { AIM_SOUTHEAST, 3, 36, {1,   1,  0}, _( "South East" ),         _( "SE" ) },
        { AIM_WEST,      2, 30, {-1,  0,  0}, _( "West" ),               _( "W" )  },
        { AIM_CENTER,    2, 33, {0,   0,  0}, _( "Directly below you" ), _( "DN" ) },
        { AIM_EAST,      2, 36, {1,   0,  0}, _( "East" ),               _( "E" )  },
        { AIM_NORTHWEST, 1, 30, {-1, -1,  0}, _( "North West" ),         _( "NW" ) },
        { AIM_NORTH,     1, 33, {0,  -1,  0}, _( "North" ),              _( "N" )  },
        { AIM_NORTHEAST, 1, 36, {1,  -1,  0}, _( "North East" ),         _( "NE" ) },
        { AIM_DRAGGED,   1, 25, {0,   0,  0}, _( "Grabbed Vehicle" ),    _( "GR" ) },
        { AIM_ALL,       3, 22, {0,   0,  0}, _( "Surrounding area" ),   _( "AL" ) },
        { AIM_CONTAINER, 1, 22, {0,   0,  0}, _( "Container" ),          _( "CN" ) },
        { AIM_WORN,      3, 25, {0,   0,  0}, _( "Worn Items" ),         _( "WR" ) }
    }
} )
, head( nullptr )
, left_window( nullptr )
, right_window( nullptr )
{
}

advanced_inventory::~advanced_inventory()
{
    save_settings(false);
    // Only refresh if we exited manually, otherwise we're going to be right back
    if( exit ) {
        werase( head );
        werase( minimap );
        werase( mm_border );
        werase( left_window );
        werase( right_window );
    }
    delwin( head );
    delwin( minimap );
    delwin( mm_border );
    delwin( left_window );
    delwin( right_window );
    if( exit ) {
        g->refresh_all();
    }
}

void advanced_inventory::save_settings(bool only_panes)
{
    if(only_panes == false) {
        uistate.adv_inv_last_coords = g->u.pos();
        uistate.adv_inv_src = src;
        uistate.adv_inv_dest = dest;
    }
    for(int i = 0; i < NUM_PANES; ++i) {
        uistate.adv_inv_in_vehicle[i] = panes[i].in_vehicle();
        uistate.adv_inv_area[i] = panes[i].get_area();
        uistate.adv_inv_index[i] = panes[i].index;
        uistate.adv_inv_filter[i] = panes[i].filter;
    }
}

void advanced_inventory::load_settings()
{
    bool moved = uistate.adv_inv_last_coords != g->u.pos();
    for(int i = 0; i < NUM_PANES; ++i) {
        auto location = static_cast<aim_location>(uistate.adv_inv_area[i]);
        panes[i].sortby = static_cast<advanced_inv_sortby>(uistate.adv_inv_sort[i]);
        bool is_in_vehicle = (moved == false && uistate.adv_inv_in_vehicle[i]);
        panes[i].set_area(squares[location], is_in_vehicle || location == AIM_DRAGGED);
        panes[i].index = uistate.adv_inv_index[i];
        panes[i].filter = uistate.adv_inv_filter[i];
    }
}

std::string advanced_inventory::get_sortname( advanced_inv_sortby sortby )
{
    switch( sortby ) {
        case SORTBY_NONE:
            return _( "none" );
        case SORTBY_NAME:
            return _( "name" );
        case SORTBY_WEIGHT:
            return _( "weight" );
        case SORTBY_VOLUME:
            return _( "volume" );
        case SORTBY_CHARGES:
            return _( "charges" );
        case SORTBY_CATEGORY:
            return _( "category" );
        case SORTBY_DAMAGE:
            return _( "damage" );
    }
    return "!BUG!";
}

bool advanced_inventory::get_square( const std::string action, aim_location &ret )
{
    if( action == "ITEMS_INVENTORY" ) {
        ret = AIM_INVENTORY;
    } else if( action == "ITEMS_WORN" ) {
        ret = AIM_WORN;
    } else if( action == "ITEMS_NW" ) {
        ret = AIM_NORTHWEST;
    } else if( action == "ITEMS_N" ) {
        ret = AIM_NORTH;
    } else if( action == "ITEMS_NE" ) {
        ret = AIM_NORTHEAST;
    } else if( action == "ITEMS_W" ) {
        ret = AIM_WEST;
    } else if( action == "ITEMS_CE" ) {
        ret = AIM_CENTER;
    } else if( action == "ITEMS_E" ) {
        ret = AIM_EAST;
    } else if( action == "ITEMS_SW" ) {
        ret = AIM_SOUTHWEST;
    } else if( action == "ITEMS_S" ) {
        ret = AIM_SOUTH;
    } else if( action == "ITEMS_SE" ) {
        ret = AIM_SOUTHEAST;
    } else if( action == "ITEMS_AROUND" ) {
        ret = AIM_ALL;
    } else if( action == "ITEMS_DRAGGED_CONTAINER" ) {
        ret = AIM_DRAGGED;
    } else if( action == "ITEMS_CONTAINER" ) {
        ret = AIM_CONTAINER;
    } else {
        return false;
    }
    return true;
}


void advanced_inventory::print_items( advanced_inventory_pane &pane, bool active )
{
    const auto &items = pane.items;
    WINDOW *window = pane.window;
    const auto index = pane.index;
    const int page = index / itemsPerPage;
    bool compact = ( TERMX <= 100 );

    int columns = getmaxx( window );
    std::string spaces( columns - 4, ' ' );

    nc_color norm = active ? c_white : c_dkgray;

    //print inventory's current and total weight + volume
    if( pane.get_area() == AIM_INVENTORY || pane.get_area() == AIM_WORN ) {
        //right align
        int hrightcol = columns -
                        to_string( g->u.convert_weight( g->u.weight_carried() ) ).length() - 3 - //"xxx.y/"
                        to_string( g->u.convert_weight( g->u.weight_capacity() ) ).length() - 3 - //"xxx.y_"
                        to_string( g->u.volume_carried() ).length() - 1 - //"xxx/"
                        to_string( g->u.volume_capacity() - 2 ).length() - 1; //"xxx|"
        nc_color color = c_ltgreen;//red color if overload
        if( g->u.weight_carried() > g->u.weight_capacity() ) {
            color = c_red;
        }
        mvwprintz( window, 4, hrightcol, color, "%.1f", g->u.convert_weight( g->u.weight_carried() ) );
        wprintz( window, c_ltgray, "/%.1f ", g->u.convert_weight( g->u.weight_capacity() ) );
        if( g->u.volume_carried() > g->u.volume_capacity() - 2 ) {
            color = c_red;
        } else {
            color = c_ltgreen;
        }
        wprintz( window, color, "%d", g->u.volume_carried() );
        wprintz( window, c_ltgray, "/%d ", g->u.volume_capacity() - 2 );
    } else { //print square's current and total weight + volume
        std::string head;
        if( pane.get_area() == AIM_ALL ) {
            head = string_format( "%3.1f %3d",
                                  g->u.convert_weight( squares[pane.get_area()].weight ),
                                  squares[pane.get_area()].volume );
        } else {
            int maxvolume = 0;
            auto &s = squares[pane.get_area()];
            if( pane.get_area() == AIM_CONTAINER && s.get_container(pane.in_vehicle()) != nullptr ) {
                maxvolume = s.get_container(pane.in_vehicle())->type->container->contains;
            } else if( pane.in_vehicle() ) {
                maxvolume = s.veh->max_volume( s.vstor );
            } else {
                maxvolume = g->m.max_volume( s.pos );
            }
            head = string_format( "%3.1f %3d/%3d", g->u.convert_weight( s.weight ), s.volume, maxvolume );
        }
        mvwprintz( window, 4, columns - 1 - head.length(), norm, "%s", head.c_str() );
    }

    //print header row and determine max item name length
    const int lastcol = columns - 2; // Last printable column
    const size_t name_startpos = ( compact ? 1 : 4 );
    const size_t src_startpos = lastcol - 17;
    const size_t amt_startpos = lastcol - 14;
    const size_t weight_startpos = lastcol - 9;
    const size_t vol_startpos = lastcol - 3;
    int max_name_length = amt_startpos - name_startpos - 1; // Default name length

    //~ Items list header. Table fields length without spaces: amt - 4, weight - 5, vol - 4.
    const int table_hdr_len1 = utf8_width( _( "amt weight vol" ) ); // Header length type 1
    //~ Items list header. Table fields length without spaces: src - 2, amt - 4, weight - 5, vol - 4.
    const int table_hdr_len2 = utf8_width( _( "src amt weight vol" ) ); // Header length type 2

    mvwprintz( window, 5, ( compact ? 1 : 4 ), c_ltgray, _( "Name (charges)" ) );
    if( pane.get_area() == AIM_ALL && !compact ) {
        mvwprintz( window, 5, lastcol - table_hdr_len2 + 1, c_ltgray, _( "src amt weight vol" ) );
        max_name_length = src_startpos - name_startpos - 1; // 1 for space
    } else {
        mvwprintz( window, 5, lastcol - table_hdr_len1 + 1, c_ltgray, _( "amt weight vol" ) );
    }

    for( int i = page * itemsPerPage , x = 0 ; i < ( int )items.size() &&
         x < itemsPerPage ; i++ , x++ ) {
        const auto &sitem = items[i];
        if( sitem.is_category_header() ) {
            mvwprintz( window, 6 + x, ( columns - utf8_width( sitem.name.c_str() ) - 6 ) / 2, c_cyan, "[%s]",
                       sitem.name.c_str() );
            continue;
        }
        if( !sitem.is_item_entry() ) {
            // Empty entry at the bottom of a page.
            continue;
        }
        const auto &it = *sitem.it;
        const bool selected = active && index == i;

        nc_color thiscolor = active ? it.color( &g->u ) : norm;
        nc_color thiscolordark = c_dkgray;
        nc_color print_color;

        if( selected ) {
            thiscolor = ( inCategoryMode &&
                          pane.sortby == SORTBY_CATEGORY ) ? c_white_red : hilite( thiscolor );
            thiscolordark = hilite( thiscolordark );
            if( compact ) {
                mvwprintz( window, 6 + x, 1, thiscolor, "  %s", spaces.c_str() );
            } else {
                mvwprintz( window, 6 + x, 1, thiscolor, ">>%s", spaces.c_str() );
            }
        }

        std::string item_name = it.display_name();
        if( OPTIONS["ITEM_SYMBOLS"] ) {
            item_name = string_format( "%c %s", it.symbol(), item_name.c_str() );
        }

        //print item name
        trim_and_print( window, 6 + x, compact ? 1 : 4, max_name_length, thiscolor, "%s",
                        item_name.c_str() );

        //print src column
        // TODO: specify this is coming from a vehicle!
        if( pane.get_area() == AIM_ALL && !compact ) {
            mvwprintz( window, 6 + x, src_startpos, thiscolor, "%s", squares[sitem.area].shortname.c_str() );
        }

        //print "amount" column
        int it_amt = sitem.stacks;
        if( it_amt > 1 ) {
            print_color = thiscolor;
            if( it_amt > 9999 ) {
                it_amt = 9999;
                print_color = selected ? hilite( c_red ) : c_red;
            }
            mvwprintz( window, 6 + x, amt_startpos, print_color, "%4d", it_amt );
        }

        //print weight column
        double it_weight = g->u.convert_weight( sitem.weight );
        size_t w_precision;
        print_color = ( it_weight > 0 ) ? thiscolor : thiscolordark;

        if( it_weight >= 1000.0 ) {
            if( it_weight >= 10000.0 ) {
                print_color = selected ? hilite( c_red ) : c_red;
                it_weight = 9999.0;
            }
            w_precision = 0;
        } else if( it_weight >= 100.0 ) {
            w_precision = 1;
        } else {
            w_precision = 2;
        }
        mvwprintz( window, 6 + x, weight_startpos, print_color, "%5.*f", w_precision, it_weight );

        //print volume column
        int it_vol = sitem.volume;
        print_color = ( it_vol > 0 ) ? thiscolor : thiscolordark;
        if( it_vol > 9999 ) {
            it_vol = 9999;
            print_color = selected ? hilite( c_red ) : c_red;
        }
        mvwprintz( window, 6 + x, vol_startpos, print_color, "%4d", it_vol );

        if( active && sitem.autopickup ) {
            mvwprintz( window, 6 + x, 1, magenta_background( it.color( &g->u ) ), "%s",
                       ( compact ? it.tname().substr( 0, 1 ) : ">" ).c_str() );
        }
    }
}

struct advanced_inv_sort_case_insensitive_less : public std::binary_function< char, char, bool > {
    bool operator()( char x, char y ) const
    {
        return toupper( static_cast< unsigned char >( x ) ) < toupper( static_cast< unsigned char >( y ) );
    }
};

struct advanced_inv_sorter {
    advanced_inv_sortby sortby;
    advanced_inv_sorter( advanced_inv_sortby sort )
    {
        sortby = sort;
    };
    bool operator()( const advanced_inv_listitem &d1, const advanced_inv_listitem &d2 )
    {
        // Note: the item pointer can only be null on sort by category, otherwise it is always valid.
        switch( sortby ) {
            case SORTBY_NONE:
                if( d1.idx != d2.idx ) {
                    return d1.idx < d2.idx;
                }
                break;
            case SORTBY_NAME:
                // Fall through to code below the switch
                break;
            case SORTBY_WEIGHT:
                if( d1.weight != d2.weight ) {
                    return d1.weight > d2.weight;
                }
                break;
            case SORTBY_VOLUME:
                if( d1.volume != d2.volume ) {
                    return d1.volume > d2.volume;
                }
                break;
            case SORTBY_CHARGES:
                if( d1.it->charges != d2.it->charges ) {
                    return d1.it->charges > d2.it->charges;
                }
                break;
            case SORTBY_CATEGORY:
                assert( d1.cat != nullptr );
                assert( d2.cat != nullptr );
                if( d1.cat != d2.cat ) {
                    return *d1.cat < *d2.cat;
                } else if( d1.is_category_header() ) {
                    return true;
                } else if( d2.is_category_header() ) {
                    return false;
                }
                break;
            case SORTBY_DAMAGE:
                if( d1.it->damage != d2.it->damage ) {
                    return d1.it->damage < d2.it->damage;
                }
                break;
        }
        // secondary sort by name
        const std::string *n1;
        const std::string *n2;
        if( d1.name_without_prefix == d2.name_without_prefix ) {
            //if names without prefix equal, compare full name
            n1 = &d1.name;
            n2 = &d2.name;
        } else {
            //else compare name without prefix
            n1 = &d1.name_without_prefix;
            n2 = &d2.name_without_prefix;
        }
        return std::lexicographical_compare( n1->begin(), n1->end(),
                                             n2->begin(), n2->end(), advanced_inv_sort_case_insensitive_less() );
    }
};

void advanced_inventory::menu_square( uimenu *menu )
{
    assert( menu != nullptr );
    assert( menu->entries.size() >= 9 );
    int ofs = -25 - 4;
    int sel = menu->selected + 1;
    for( int i = 1; i < 10; i++ ) {
        char key = ( char )( i + 48 );
        bool in_vehicle = squares[i].can_store_in_vehicle();
        const char *bracket = (in_vehicle == true) ? "<>" : "[]";
        // always show storage option for vehicle storage, if applicable
        bool canputitems = (menu->entries[i - 1].enabled && squares[i].canputitems());
        nc_color bcolor = ( canputitems ? ( sel == i ? h_white : c_ltgray ) : c_dkgray );
        nc_color kcolor = ( canputitems ? ( sel == i ? h_white : c_ltgray ) : c_dkgray );
        const int x = squares[i].hscreenx + 5;
        const int y = squares[i].hscreeny + ofs;
        mvwprintz( menu->window, x, y, bcolor, "%c", bracket[0] );
        wprintz( menu->window, kcolor, "%c", key );
        wprintz( menu->window, bcolor, "%c", bracket[1] );
    }
}

inline char advanced_inventory::get_location_key( aim_location area )
{
    switch( area ) {
        case AIM_INVENTORY:
            return 'I';
        case AIM_WORN:
            return 'W';
        case AIM_SOUTHWEST:
            return '1';
        case AIM_SOUTH:
            return '2';
        case AIM_SOUTHEAST:
            return '3';
        case AIM_WEST:
            return '4';
        case AIM_CENTER:
            return '5';
        case AIM_EAST:
            return '6';
        case AIM_NORTHWEST:
            return '7';
        case AIM_NORTH:
            return '8';
        case AIM_NORTHEAST:
            return '9';
        case AIM_ALL:
            return 'A';
        case AIM_DRAGGED:
            return 'D';
        case AIM_CONTAINER:
            return 'C';
        default:
            debugmsg( "invalid [aim_location] in get_location_key()!" );
            return ' ';
    }
}

int advanced_inventory::print_header( advanced_inventory_pane &pane, aim_location sel )
{
    WINDOW *window = pane.window;
    int area = pane.get_area();
    auto other = ( pane.window == left_window ) ? panes[left] : panes[right];
    int wwidth = getmaxx( window );
    int ofs = wwidth - 25 - 2 - 14;
    for( int i = 0; i < NUM_AIM_LOCATIONS; ++i ) {
        const char key = get_location_key( static_cast<aim_location>( i ) );
        const char *bracket = ( squares[i].can_store_in_vehicle() ) ? "<>" : "[]";
        bool in_vehicle = ( pane.in_vehicle() && squares[i].id == area && sel == area && area != AIM_ALL );
        bool all_brackets = ( area == AIM_ALL && ( i >= AIM_SOUTHWEST && i <= AIM_NORTHEAST ) );
        nc_color bcolor = c_red, kcolor = c_red;
        if( squares[i].canputitems( pane.get_cur_item_ptr() ) ) {
            bcolor = ( in_vehicle ) ? c_ltblue :
                     ( area == i || all_brackets ) ? c_ltgray : c_dkgray;
            kcolor = ( area == i ) ? c_white : ( sel == i ) ? c_ltgray : c_dkgray;
        }
        const int x = squares[i].hscreenx;
        const int y = squares[i].hscreeny + ofs;
        mvwprintz( window, x, y, bcolor, "%c", bracket[0] );
        wprintz( window, kcolor, "%c", ( in_vehicle && sel != AIM_DRAGGED ) ? 'V' : key );
        wprintz( window, bcolor, "%c", bracket[1] );
    }
    return squares[AIM_INVENTORY].hscreeny + ofs;
}

int advanced_inv_area::get_item_count() const
{
    if( id == AIM_INVENTORY ) {
        return g->u.inv.size();
    } else if( id == AIM_WORN ) {
        return g->u.worn.size();
    } else if( id == AIM_ALL ) {
        return 0;
    } else if( id == AIM_DRAGGED ) {
        return ( can_store_in_vehicle() == true ) ? veh->get_items( vstor ).size() : 0;
    } else {
        return g->m.i_at( pos ).size();
    }
}

void advanced_inv_area::init()
{
    pos = g->u.pos() + off;
    veh = nullptr;
    vstor = -1;
    volume = 0; // must update in main function
    weight = 0; // must update in main function
    switch( id ) {
        case AIM_INVENTORY:
        case AIM_WORN:
            canputitemsloc = true;
            break;
        case AIM_DRAGGED:
            if( g->u.grab_type != OBJECT_VEHICLE ) {
                canputitemsloc = false;
                desc[0] = _( "Not dragging any vehicle" );
                break;
            }
            // offset for dragged vehicles is not statically initialized, so get it
            off = g->u.grab_point;
            // Reset position because offset changed
            pos = g->u.pos() + off;
            veh = g->m.veh_at( pos, vstor );
            if( veh != nullptr ) {
                vstor = veh->part_with_feature( vstor, "CARGO", false );
            }
            if( vstor >= 0 ) {
                desc[0] = veh->name;
                canputitemsloc = true;
                max_size = MAX_ITEM_IN_VEHICLE_STORAGE;
                max_volume = veh->max_volume( vstor );
            } else {
                veh = nullptr;
                canputitemsloc = false;
                desc[0] = _( "No dragged vehicle" );
            }
            break;
        case AIM_CONTAINER:
            // set container position based on location
            set_container_position();
            // location always valid, actual check is done in canputitems()
            // and depends on selected item in pane (if it is valid container)
            canputitemsloc = true;
            if( get_container() == nullptr ) {
                desc[0] = _( "Invalid container" );
            }
            break;
        case AIM_ALL:
            desc[0] = _( "All 9 squares" );
            canputitemsloc = true;
            break;
        case AIM_SOUTHWEST:
        case AIM_SOUTH:
        case AIM_SOUTHEAST:
        case AIM_WEST:
        case AIM_CENTER:
        case AIM_EAST:
        case AIM_NORTHWEST:
        case AIM_NORTH:
        case AIM_NORTHEAST:
            veh = g->m.veh_at( pos, vstor );
            if( veh != nullptr ) {
                vstor = veh->part_with_feature( vstor, "CARGO", false );
            }
            canputitemsloc = can_store_in_vehicle() || g->m.can_put_items( pos );
            max_size = MAX_ITEM_IN_SQUARE;
            max_volume = g->m.max_volume( pos );
            if( can_store_in_vehicle() ) {
                // get storage label
                const auto part = veh->parts[veh->global_part_at(pos.x, pos.y)];
                desc[1] = veh->get_label(part.precalc[0].x, part.precalc[0].y);
            }
            // get graffiti or terrain name
            desc[0] = ( g->m.has_graffiti_at( pos ) == true ) ? 
                g->m.graffiti_at( pos ) : g->m.ter_at( pos ).name;
        default:
            break;
    }

    /* assemble a list of interesting traits of the target square */
    // fields? with a special case for fire
    bool danger_field = false;
    const field &tmpfld = g->m.field_at( pos );
    for( auto &fld : tmpfld ) {
        const field_entry &cur = fld.second;
        field_id curType = cur.getFieldType();
        switch( curType ) {
            case fd_fire:
                flags.append( _( " <color_white_red>FIRE</color>" ) );
                break;
            default:
                if( cur.is_dangerous() ) {
                    danger_field = true;
                }
                break;
        }
    }
    if( danger_field ) {
        flags.append( _( " DANGER" ) );
    }

    // trap?
    const trap &tr = g->m.tr_at( pos );
    if( tr.can_see( pos, g->u ) && !tr.is_benign() ) {
        flags.append( _( " TRAP" ) );
    }

    // water?
    static const std::array<ter_id, 6> ter_water = {
        {t_water_dp, t_water_pool, t_swater_dp, t_water_sh, t_swater_sh, t_sewage}
    };
    if(is_any_of(g->m.ter(pos), ter_water)) {
        flags.append( _( " WATER" ) );
    }

    // remove leading space
    if( flags.length() ) {
        flags.erase( 0, 1 );
    }

}

std::string center_text( const char *str, int width )
{
    return std::string( ( ( width - strlen( str ) ) / 2 ), ' ' ) + str;
}

void advanced_inventory::init()
{
    for( auto &square : squares ) {
        square.init();
    }

    load_settings();

    src  = static_cast<side>( uistate.adv_inv_src );
    dest = static_cast<side>( uistate.adv_inv_dest );

    w_height = ( TERMY < min_w_height + head_height ) ? min_w_height : TERMY - head_height;
    w_width = ( TERMX < min_w_width ) ? min_w_width : ( TERMX > max_w_width ) ? max_w_width :
              ( int )TERMX;

    headstart = 0; //(TERMY>w_height)?(TERMY-w_height)/2:0;
    colstart = ( TERMX > w_width ) ? ( TERMX - w_width ) / 2 : 0;

    head = newwin( head_height, w_width - minimap_width, headstart, colstart );
    mm_border = newwin( minimap_height + 2, minimap_width + 2, headstart,
                        colstart + ( w_width - ( minimap_width + 2 ) ) );
    minimap = newwin( minimap_height, minimap_width, headstart + 1,
                      colstart + ( w_width - ( minimap_width + 1 ) ) );
    left_window = newwin( w_height, w_width / 2, headstart + head_height, colstart );
    right_window = newwin( w_height, w_width / 2, headstart + head_height,
                           colstart + w_width / 2 );

    itemsPerPage = w_height - 2 - 5; // 2 for the borders, 5 for the header stuff

    panes[left].window = left_window;
    panes[right].window = right_window;
}

advanced_inv_listitem::advanced_inv_listitem( item *an_item, int index, int count,
        aim_location _area, bool from_veh )
    : idx( index )
    , area( _area )
    , it( an_item )
    , name( an_item->tname( count ) )
    , name_without_prefix( an_item->tname( 1, false ) )
    , autopickup( hasPickupRule( an_item->tname( 1, false ) ) )
    , stacks( count )
    , volume( an_item->volume() * stacks )
    , weight( an_item->weight() * stacks )
    , cat( &an_item->get_category() )
    , from_vehicle( from_veh )
{
    assert( stacks >= 1 );
    assert( stacks == 1 || !it->count_by_charges() );
}

advanced_inv_listitem::advanced_inv_listitem()
    : idx()
    , area()
    , it( nullptr )
    , name()
    , name_without_prefix()
    , autopickup()
    , stacks()
    , volume()
    , weight()
    , cat( nullptr )
{
}

advanced_inv_listitem::advanced_inv_listitem( const item_category *category )
    : idx()
    , area()
    , it( nullptr )
    , name( category->name )
    , name_without_prefix()
    , autopickup()
    , stacks()
    , volume()
    , weight()
    , cat( category )
{
}

bool advanced_inv_listitem::is_category_header() const
{
    return it == nullptr && cat != nullptr;
}

bool advanced_inv_listitem::is_item_entry() const
{
    return it != nullptr;
}

bool advanced_inventory_pane::is_filtered( const advanced_inv_listitem &it ) const
{
    return is_filtered( it.it );
}

bool advanced_inventory_pane::is_filtered( const item *it ) const
{
    if( filter.empty() ) {
        return false;
    }

    std::string str = it->tname();
    if( filtercache.find( str ) == filtercache.end() ) {
        bool match = !g->list_items_match( it, filter );
        filtercache[ str ] = match;

        return match;
    }

    return filtercache[ str ];
}

template <typename Container>
static itemslice i_stacked( Container items )
{
    //create a new container for our stacked items
    itemslice islice;

    //iterate through all items in the vector
    for( auto &items_it : items ) {
        if( items_it.count_by_charges() ) {
            // Those exists as a single item all the item anyway
            islice.push_back( std::make_pair( &items_it, 1 ) );
            continue;
        }
        bool list_exists = false;

        //iterate through stacked item lists
        for( auto &elem : islice ) {
            //check if the ID exists
            item *first_item = elem.first;
            if( first_item->type->id == items_it.type->id ) {
                //we've found the list of items with the same type ID

                if( first_item->stacks_with( items_it ) ) {
                    //add it to the existing list
                    elem.second++;
                    list_exists = true;
                    break;
                }
            }
        }

        if( !list_exists ) {
            //insert the list into islice
            islice.push_back( std::make_pair( &items_it, 1 ) );
        }

    } //end items loop

    return islice;
}

void advanced_inventory_pane::add_items_from_area( advanced_inv_area &square,
        bool vehicle_override )
{
    assert( square.id != AIM_ALL );
    square.volume = 0;
    square.weight = 0;
    if( !square.canputitems() ) {
        return;
    }
    map &m = g->m;
    // Existing items are *not* cleared on purpose, this might be called
    // several time in case all surrounding squares are to be shown.
    if( square.id == AIM_INVENTORY ) {
        const invslice &stacks = g->u.inv.slice();
        for( size_t x = 0; x < stacks.size(); ++x ) {
            auto &an_item = stacks[x]->front();
            advanced_inv_listitem it( &an_item, x, stacks[x]->size(), square.id, false );
            if( is_filtered( it.it ) ) {
                continue;
            }
            square.volume += it.volume;
            square.weight += it.weight;
            items.push_back( it );
        }
    } else if( square.id == AIM_WORN ) {
        for( size_t i = 0; i < g->u.worn.size(); ++i ) {
            advanced_inv_listitem it( &g->u.worn[i], i, 1, square.id, false );
            if( is_filtered( it.it ) ) {
                continue;
            }
            square.volume += it.volume;
            square.weight += it.weight;
            items.push_back( it );
        }
    } else if( square.id == AIM_CONTAINER ) {
        item *cont = square.get_container( in_vehicle() );
        if( cont != nullptr ) {
            if( !cont->is_container_empty() ) {
                // filtering does not make sense for liquid in container
                item *it = &( square.get_container( in_vehicle() )->contents[0] );
                advanced_inv_listitem ait( it, 0, 1, square.id, in_vehicle() );
                square.volume += ait.volume;
                square.weight += ait.weight;
                items.push_back( ait );
            }
            square.desc[0] = cont->tname( 1, false );
        }
    } else {
        bool is_in_vehicle = square.can_store_in_vehicle() && (in_vehicle() || vehicle_override);
        const itemslice &stacks = (is_in_vehicle == true) ? 
                                  i_stacked( square.veh->get_items( square.vstor ) ) : 
                                  i_stacked( m.i_at( square.pos ) );
        for( size_t x = 0; x < stacks.size(); ++x ) {
            advanced_inv_listitem it( stacks[x].first, x, stacks[x].second, square.id, is_in_vehicle );
            if( is_filtered( it.it ) ) {
                continue;
            }
            square.volume += it.volume;
            square.weight += it.weight;
            items.push_back( it );
        }
    }
}

void advanced_inventory_pane::paginate( size_t itemsPerPage )
{
    if( sortby != SORTBY_CATEGORY ) {
        return; // not needed as there are no category entries here.
    }
    // first, we insert all the items, then we sort the result
    for( size_t i = 0; i < items.size(); ++i ) {
        if( i % itemsPerPage == 0 ) {
            // first entry on the page, should be a category header
            if( items[i].is_item_entry() ) {
                items.insert( items.begin() + i, advanced_inv_listitem( items[i].cat ) );
            }
        }
        if( ( i + 1 ) % itemsPerPage == 0 && i + 1 < items.size() ) {
            // last entry of the page, but not the last entry at all!
            // Must *not* be a category header!
            if( items[i].is_category_header() ) {
                items.insert( items.begin() + i, advanced_inv_listitem() );
            }
        }
    }
}

void advanced_inventory::recalc_pane( side p )
{
    auto &pane = panes[p];
    pane.recalc = false;
    pane.items.clear();
    // Add items from the source location or in case of all 9 surrounding squares,
    // add items from several locations.
    if( pane.get_area() == AIM_ALL ) {
        auto &alls = squares[AIM_ALL];
        auto &there = panes[-p + 1];
        auto &other = squares[there.get_area()];
        alls.volume = 0;
        alls.weight = 0;
        for( auto &s : squares ) {
            // All the surrounding squares, nothing else
            if(s.id < AIM_SOUTHWEST || s.id > AIM_NORTHEAST) {
                continue;
            }

            // To allow the user to transfer all items from all surrounding squares to
            // a specific square, filter out items that are already on that square.
            // e.g. left pane AIM_ALL, right pane AIM_NORTH. The user holds the
            // enter key down in the left square and moves all items to the other side.
            const bool same = other.is_same( s );

            // Deal with squares with ground + vehicle storage
            // Also handle the case when the other tile covers vehicle
            // or the ground below the vehicle.
            if( s.can_store_in_vehicle() && !(same && there.in_vehicle()) ) {
                bool do_vehicle = ( there.get_area() == s.id ) ? !there.in_vehicle() : true;
                pane.add_items_from_area( s, do_vehicle );
                alls.volume += s.volume;
                alls.weight += s.weight;
            }

            // Add map items
            if( !same || there.in_vehicle() ) {
                pane.add_items_from_area( s );
                alls.volume += s.volume;
                alls.weight += s.weight;
            }
        }
    } else {
        pane.add_items_from_area( squares[pane.get_area()] );
    }
    // Insert category headers (only expected when sorting by category)
    if( pane.sortby == SORTBY_CATEGORY ) {
        std::set<const item_category *> categories;
        for( auto &it : pane.items ) {
            categories.insert( it.cat );
        }
        for( auto &cat : categories ) {
            pane.items.push_back( advanced_inv_listitem( cat ) );
        }
    }
    // Finally sort all items (category headers will now be moved to their proper position)
    std::stable_sort( pane.items.begin(), pane.items.end(), advanced_inv_sorter( pane.sortby ) );
    pane.paginate( itemsPerPage );
}

void advanced_inventory_pane::fix_index()
{
    if( items.empty() ) {
        index = 0;
        return;
    }
    if( index < 0 ) {
        index = 0;
    } else if( static_cast<size_t>( index ) >= items.size() ) {
        index = static_cast<int>( items.size() ) - 1;
    }
    skip_category_headers( +1 );
}

void advanced_inventory::redraw_pane( side p )
{
    auto &pane = panes[p];
    if( recalc || pane.recalc ) {
        recalc_pane( p );
    } else if( !( redraw || pane.redraw ) ) {
        return;
    }
    pane.redraw = false;
    pane.fix_index();

    const bool active = p == src;
    const advanced_inv_area &square = squares[pane.get_area()];
    auto w = pane.window;

    werase( w );
    print_items( pane, active );

    auto itm = pane.get_cur_item_ptr();
    int width = print_header(pane, (itm != nullptr) ? itm->area : pane.get_area());
    bool same_as_dragged = (square.id >= AIM_SOUTHWEST && square.id <= AIM_NORTHEAST) && // only cardinals
            square.id != AIM_CENTER && panes[p].in_vehicle() && // not where you stand, and pane is in vehicle
            square.off == squares[AIM_DRAGGED].off; // make sure the offsets are the same as the grab point
    auto sq = (same_as_dragged) ? squares[AIM_DRAGGED] : square;
    bool car = square.can_store_in_vehicle() && panes[p].in_vehicle() && sq.id != AIM_DRAGGED;
    auto name = utf8_truncate((car == true) ? sq.veh->name : sq.name, width);
    auto desc = utf8_truncate(sq.desc[car], width);
    width -= 2 + 1; // starts at offset 2, plus space between the header and the text
    mvwprintz( w, 1, 2, active ? c_green  : c_ltgray, "%s", name.c_str() );
    mvwprintz( w, 2, 2, active ? c_ltblue : c_dkgray, "%s", desc.c_str() );
    trim_and_print( w, 3, 2, width, active ? c_cyan : c_dkgray, square.flags.c_str() );

    const int max_page = ( pane.items.size() + itemsPerPage - 1 ) / itemsPerPage;
    if( active && max_page > 1 ) {
        const int page = pane.index / itemsPerPage;
        mvwprintz( w, 4, 2, c_ltblue, _( "[<] page %d of %d [>]" ), page + 1, max_page );
    }

    if( active ) {
        wattron( w, c_cyan );
    }
    // draw a darker border around the inactive pane
    draw_border( w, ( active == true ) ? BORDER_COLOR : c_dkgray );
    mvwprintw( w, 0, 3, _( "< [s]ort: %s >" ), get_sortname( pane.sortby ).c_str() );
    int max = MAX_ITEM_IN_SQUARE; //TODO: use the square, luke
    if( pane.get_area() == AIM_ALL ) {
        max *= 9;
    }
    int fmtw = 7 + ( pane.items.size() > 99 ? 3 : pane.items.size() > 9 ? 2 : 1 ) +
               ( max > 99 ? 3 : max > 9 ? 2 : 1 );
    mvwprintw( w, 0 , ( w_width / 2 ) - fmtw, "< %d/%d >", pane.items.size(), max );
    const char *fprefix = _( "[F]ilter" );
    const char *fsuffix = _( "[R]eset" );
    if( ! filter_edit ) {
        if( !pane.filter.empty() ) {
            mvwprintw( w, getmaxy( w ) - 1, 2, "< %s: %s >", fprefix,
                       pane.filter.c_str() );
        } else {
            mvwprintw( w, getmaxy( w ) - 1, 2, "< %s >", fprefix );
        }
    }
    if( active ) {
        wattroff( w, c_white );
    }
    if( ! filter_edit && !pane.filter.empty() ) {
        mvwprintz( w, getmaxy( w ) - 1, 6 + std::strlen( fprefix ), c_white, "%s",
                   pane.filter.c_str() );
        mvwprintz( w, getmaxy( w ) - 1,
                   getmaxx( w ) - std::strlen( fsuffix ) - 2, c_white, "%s", fsuffix );
    }
    wrefresh( w );
}

bool advanced_inventory::move_all_items()
{
    auto &spane = panes[src];
    auto &dpane = panes[dest];

    // AIM_ALL source area routine
    if(spane.get_area() == AIM_ALL) {
        // future feature :-)
        popup(_("You can't do that (yet!)"));
        return false;
        // make a copy of the current pane for below loop
        auto shadow = panes[src];
        // here we recursively call this function with each area in order to 
        // put all items in the proper destination area, with minimal fuss
        auto &loc = uistate.adv_inv_aim_all_location;
        // vehicle items for said square
        if(squares[loc].can_store_in_vehicle()) {
            // either do the inverse of the pane (if it is the one we are transferring to),
            // or just transfer the contents (if it is not the one we are transferring to)
            spane.set_area(squares[loc], (dpane.get_area() == loc) ? !dpane.in_vehicle() : true);
            // add items, calculate weights and volumes... the fun stuff
            recalc_pane(src);
            // then move the items to the destination area
            move_all_items();
        }
        // same as above, but for map items
        spane.set_area(squares[loc++], false);
        recalc_pane(src);
        move_all_items();
        if(loc > AIM_NORTHEAST) {
            loc = 0;
        }
        // restore the pane to its former glory
        panes[src] = shadow;
        // done! that was easy!
    }

    // Check some preconditions to quickly leave the function.
    if( spane.items.empty() ) {
        return false;
    }


    if( dpane.get_area() == AIM_ALL ) {
        auto loc = dpane.get_area();
        // ask where we want to store the item via the menu
        if(!query_destination(loc)) {
            return false;
        }
        // set the pane to the destination square, so items dumped there are 
        // viewed afterwards, then continue on with moving items. this also
        // sets it to look into vehicle cargo if there exists one (for convenience)
        dpane.set_area(squares[loc], squares[loc].can_store_in_vehicle());
    }
    if( spane.get_area() == AIM_INVENTORY &&
        !query_yn( _( "Really move everything from your inventory?" ) ) ) {
        return false;
    }
    if( spane.get_area() == AIM_WORN &&
        !query_yn( _( "Really remove all your clothes? (woo woo)" ) ) ) {
        return false;
    }
    auto &sarea = squares[spane.get_area()];
    auto &darea = squares[dpane.get_area()];

    if( !OPTIONS["CLOSE_ADV_INV"] ) {
        // Why is this here? It's because the activity backlog can act
        // like a stack instead of a single deferred activity in order to
        // accomplish some UI shenanigans. The inventory menu activity is
        // added, then an activity to drop is pushed on the stack, then
        // the drop activity is repeatedly popped and pushed on the stack
        // until all its items are processed. When the drop activity runs out,
        // the inventory menu activity is there waiting and seamlessly returns
        // the player to the menu. If the activity is interrupted instead of
        // completing, both activities are cancelled.
        // Thanks to kevingranade for the explanation.
        g->u.assign_activity( ACT_ADV_INVENTORY, 0 );
        g->u.activity.auto_resume = true;
    }

    if( spane.get_area() == AIM_INVENTORY || spane.get_area() == AIM_WORN ) {
        g->u.assign_activity( ACT_DROP, 0 );
        g->u.activity.placement = darea.off;
        g->u.activity.values.push_back(dpane.in_vehicle());
    }
    if( spane.get_area() == AIM_INVENTORY ) {
        for( size_t index = 0; index < g->u.inv.size(); ++index ) {
            const auto &stack = g->u.inv.const_stack( index );
            if( spane.is_filtered( &( stack.front() ) ) ) {
                continue;
            }
            g->u.activity.values.push_back( index );
            if( stack.front().count_by_charges() ) {
                g->u.activity.values.push_back( stack.front().charges );
            } else {
                g->u.activity.values.push_back( stack.size() );
            }
        }
    } else if( spane.get_area() == AIM_WORN ) {
        // do this in reverse, to account for vector item removal messing with future indices
        for( size_t idx = 0; idx < g->u.worn.size(); ++idx ) {
            size_t index = ( g->u.worn.size() - idx - 1 );
            auto &elem = g->u.worn[index];
            if( spane.is_filtered( &elem ) ) {
                continue;
            }
            g->u.activity.values.push_back( player::worn_position_to_index( index ) );
            int amount = (elem.count_by_charges() == true) ? elem.charges : 1;
            g->u.activity.values.push_back(amount);
        }
    } else {
        if( dpane.get_area() == AIM_INVENTORY || dpane.get_area() == AIM_WORN ) {
            g->u.assign_activity( ACT_PICKUP, 0 );
            g->u.activity.values.push_back( spane.in_vehicle() );
            if( dpane.get_area() == AIM_WORN ) {
                g->u.activity.str_values.push_back( "equip" );
            }
        } else { // Vehicle and map destinations are handled the same.
            g->u.assign_activity( ACT_MOVE_ITEMS, 0 );
            // store whether the source is from a vehicle (first entry)
            g->u.activity.values.push_back(spane.in_vehicle());
            // store whether the destination is a vehicle
            g->u.activity.values.push_back(dpane.in_vehicle());
            // Stash the destination
            g->u.activity.coords.push_back(darea.off);
        }
        g->u.activity.placement = sarea.off;

        std::list<item>::iterator begin, end;
        if( panes[src].in_vehicle() ) {
            begin = sarea.veh->get_items( sarea.vstor ).begin();
            end = sarea.veh->get_items( sarea.vstor ).end();
        } else {
            begin = g->m.i_at( sarea.pos ).begin();
            end = g->m.i_at( sarea.pos ).end();
        }
        // push back indices and item count[s] for [begin => end)
        int index = 0;
        for(auto item_it = begin; item_it != end; ++item_it, ++index) {
            if(spane.is_filtered(&(*item_it))) {
                continue;
            }
            int amount = (item_it->count_by_charges() == true) ? item_it->charges : 1;
            g->u.activity.values.push_back(index);
            g->u.activity.values.push_back(amount);
        }
    }
    return true;
}

bool advanced_inventory::show_sort_menu( advanced_inventory_pane &pane )
{
    uimenu sm;
    sm.text = _( "Sort by... " );
    sm.addentry( SORTBY_NONE,     true, 'u', _( "Unsorted (recently added first)" ) );
    sm.addentry( SORTBY_NAME,     true, 'n', get_sortname( SORTBY_NAME ) );
    sm.addentry( SORTBY_WEIGHT,   true, 'w', get_sortname( SORTBY_WEIGHT ) );
    sm.addentry( SORTBY_VOLUME,   true, 'v', get_sortname( SORTBY_VOLUME ) );
    sm.addentry( SORTBY_CHARGES,  true, 'x', get_sortname( SORTBY_CHARGES ) );
    sm.addentry( SORTBY_CATEGORY, true, 'c', get_sortname( SORTBY_CATEGORY ) );
    sm.addentry( SORTBY_DAMAGE,   true, 'd', get_sortname( SORTBY_DAMAGE ) );
    // Pre-select current sort.
    sm.selected = pane.sortby - SORTBY_NONE;
    // Calculate key and window variables, generate window,
    // and loop until we get a valid answer.
    sm.query();
    if( sm.ret < SORTBY_NONE ) {
        return false;
    }
    pane.sortby = static_cast<advanced_inv_sortby>( sm.ret );
    return true;
}

void advanced_inventory::display()
{
    init();

    g->u.inv.sort();
    g->u.inv.restack( ( &g->u ) );

    input_context ctxt( "ADVANCED_INVENTORY" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "LEFT" );
    ctxt.register_action( "RIGHT" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "TOGGLE_TAB" );
    ctxt.register_action( "TOGGLE_VEH" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "EXAMINE" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "TOGGLE_AUTO_PICKUP" );
    ctxt.register_action( "MOVE_SINGLE_ITEM" );
    ctxt.register_action( "MOVE_ITEM_STACK" );
    ctxt.register_action( "MOVE_ALL_ITEMS" );
    ctxt.register_action( "CATEGORY_SELECTION" );
    ctxt.register_action( "ITEMS_NW" );
    ctxt.register_action( "ITEMS_N" );
    ctxt.register_action( "ITEMS_NE" );
    ctxt.register_action( "ITEMS_W" );
    ctxt.register_action( "ITEMS_CE" );
    ctxt.register_action( "ITEMS_E" );
    ctxt.register_action( "ITEMS_SW" );
    ctxt.register_action( "ITEMS_S" );
    ctxt.register_action( "ITEMS_SE" );
    ctxt.register_action( "ITEMS_INVENTORY" );
    ctxt.register_action( "ITEMS_WORN" );
    ctxt.register_action( "ITEMS_AROUND" );
    ctxt.register_action( "ITEMS_DRAGGED_CONTAINER" );
    ctxt.register_action( "ITEMS_CONTAINER" );

    exit = false;
    recalc = true;
    redraw = true;

    while( !exit ) {
        if( g->u.moves < 0 ) {
            do_return_entry();
            return;
        }
        dest = ( src == left ? right : left );

        redraw_pane( left );
        redraw_pane( right );

        if( redraw ) {
            werase( head );
            werase( minimap );
            werase( mm_border );
            draw_border( head );
            Messages::display_messages( head, 2, 1, w_width - 1, 4 );
            draw_minimap();
            const std::string msg = _( "< [?] show help >" );
            mvwprintz( head, 0,
                       w_width - ( minimap_width + 2 ) - utf8_width( msg.c_str() ) - 1,
                       c_white, msg.c_str() );
            if( g->u.has_watch() ) {
                const std::string time = calendar::turn.print_time();
                mvwprintz( head, 0, 2, c_white, time.c_str() );
            }
            wrefresh( head );
            refresh_minimap();
        }
        redraw = false;
        recalc = false;
        // source and destination pane
        advanced_inventory_pane &spane = panes[src];
        advanced_inventory_pane &dpane = panes[dest];
        // current item in source pane, might be null
        advanced_inv_listitem *sitem = spane.get_cur_item_ptr();
        aim_location changeSquare;

        const std::string action = (uistate.adv_inv_re_enter_move_all) ? 
            "MOVE_ALL_ITEMS" : ctxt.handle_input();

        if( action == "CATEGORY_SELECTION" ) {
            inCategoryMode = !inCategoryMode;
            spane.redraw = true; // We redraw to force the color change of the highlighted line and header text.
        } else if( action == "HELP_KEYBINDINGS" ) {
            redraw = true;
        } else if( get_square( action, changeSquare ) ) {
            if( panes[left].get_area() == changeSquare || panes[right].get_area() == changeSquare ) {
                if(squares[changeSquare].can_store_in_vehicle() && changeSquare != AIM_DRAGGED) {
                    // only deal with spane, as you can't _directly_ change dpane
                    if(squares[changeSquare].can_store_in_vehicle() && dpane.get_area() == changeSquare) {
                        spane.set_area(squares[changeSquare], !dpane.in_vehicle());
                        spane.recalc = true;
                    } else if(spane.get_area() == dpane.get_area()) { 
                        // swap the `in_vehicle` element of each pane if "one in, one out"
                        spane.set_area(squares[spane.get_area()], !spane.in_vehicle());
                        dpane.set_area(squares[dpane.get_area()], !dpane.in_vehicle());
                        recalc = true;
                    }
                } else {
                    swap_panes();
                }
                redraw = true;
            // we need to check the original area if we can place items in vehicle storage
            } else if( squares[changeSquare].canputitems( spane.get_cur_item_ptr() ) ) {
                bool in_vehicle_cargo = false;
                if( changeSquare == AIM_CONTAINER ) {
                    squares[changeSquare].set_container( spane.get_cur_item_ptr() );
                } else if( spane.get_area() == AIM_CONTAINER ) {
                    squares[changeSquare].set_container( nullptr );
                    // auto select vehicle if items exist at said square, or both are empty
                } else if( squares[changeSquare].can_store_in_vehicle() && spane.get_area() != changeSquare ) {
                    if(changeSquare == AIM_DRAGGED) {
                        in_vehicle_cargo = true;
                    } else {
                        // check item stacks in vehicle and map at said square
                        auto sq = squares[changeSquare];
                        auto map_stack = g->m.i_at( sq.pos );
                        auto veh_stack = sq.veh->get_items( sq.vstor );
                        // auto switch to vehicle storage if vehicle items are there, or neither are there
                        if( !veh_stack.empty() || ( map_stack.empty() && veh_stack.empty() ) ) {
                            in_vehicle_cargo = true;
                        }
                    }
                }
                spane.set_area(squares[changeSquare], in_vehicle_cargo);
                spane.index = 0;
                spane.recalc = true;
                if( dpane.get_area() == AIM_ALL ) {
                    dpane.recalc = true;
                }
                redraw = true;
            } else {
                popup( _( "You can't put items there" ) );
                redraw = true; // to clear the popup
            }
        } else if( action == "MOVE_SINGLE_ITEM" || action == "MOVE_ITEM_STACK" ) {
            if( sitem == nullptr || !sitem->is_item_entry() ) {
                continue;
            }
            aim_location destarea = dpane.get_area();
            aim_location srcarea = sitem->area;
            if( !query_destination( destarea ) ) {
                continue;
            }

            // AIM_ALL should disable same area check and handle it with proper filtering instead.
            // This is a workaround around the lack of vehicle location info in
            // either aim_location or advanced_inv_listitem.
            if( squares[srcarea].is_same( squares[destarea] ) && 
                spane.get_area() != AIM_ALL && spane.in_vehicle() == dpane.in_vehicle() ) {
                popup( _( "Source area is the same as destination (%s)." ), squares[destarea].name.c_str() );
                redraw = true; // popup has messed up the screen
                continue;
            }
            assert( sitem->it != nullptr );
            const bool by_charges = sitem->it->count_by_charges();
            long amount_to_move = 0;
            if( !query_charges( destarea, *sitem, action == "MOVE_SINGLE_ITEM", amount_to_move ) ) {
                continue;
            }
            // This makes sure that all item references in the advanced_inventory_pane::items vector
            // are recalculated, even when they might not have change, but they could (e.g. items
            // taken from inventory, but unable to put into the cargo trunk go back into the inventory,
            // but are potentially at a different place).
            recalc = true;
            assert( amount_to_move > 0 );
            if( destarea == AIM_CONTAINER ) {
                if( !move_content( *sitem->it, *squares[destarea].get_container( dpane.in_vehicle() ) ) ) {
                    redraw = true;
                    continue;
                }
            } else if( srcarea == AIM_INVENTORY || srcarea == AIM_WORN ) {
                // from inventory: remove all items first, than try to put them
                // onto the map/vehicle, if it fails, put them back into the inventory.
                // If no item has actually been moved, continue.

                // if worn, we need to fix with the worn index number (starts at -2, as -1 is weapon)
                int idx = ( srcarea == AIM_INVENTORY ) ? sitem->idx : player::worn_position_to_index( sitem->idx );
                if( by_charges ) {
                    item moving_item = g->u.reduce_charges( idx, amount_to_move );
                    assert( !moving_item.is_null() );
                    if( !add_item( destarea, moving_item ) ) {
                        if( srcarea == AIM_INVENTORY ) {
                            g->u.i_add( moving_item );
                        } else if( srcarea == AIM_WORN ) {
                            g->u.wear_item(&moving_item);
                        }
                        continue;
                    }
                } else {
                    if( srcarea == AIM_INVENTORY ) {
                        // pointer to item if this is going to AIM_WORN
                        std::list<item> moving_items = g->u.inv.reduce_stack( idx, amount_to_move );
                        int moved = 0;
                        bool chargeback = false;
                        for( auto &elem : moving_items ) {
                            assert( !elem.is_null() );
                            // if we are adding to worn from inv, need to remove duplicate item
                            item *thing = nullptr;
                            if( chargeback ) {
                                thing = &g->u.i_add( elem );
                            } else if( add_item( destarea, elem ) ) {
                                ++moved;
                            } else {
                                thing = &g->u.i_add( elem );
                                chargeback = true;
                            }
                            (void)thing;
                        }
                        if( moved == 0 ) {
                            continue;
                        }
                    } else if( srcarea == AIM_WORN ) {
                        std::vector<item> moving_items;
                        g->u.takeoff( idx, false, &moving_items );
                        int moved = 0;
                        bool chargeback = false;
                        for( auto &elem : moving_items ) {
                            assert( !elem.is_null() );
                            if( chargeback ) {
                                g->u.i_add( elem );
                            } else if( add_item( destarea, elem ) ) {
                                ++moved;
                            } else {
                                g->u.i_add( elem );
                                chargeback = true;
                            }
                        }
                        if( moved == 0 ) {
                            continue;
                        }
                    }
                }
            } else {
                // from map/vehicle: add the item to the destination, if that worked,
                // remove it from the source, else continue.
                // TODO: move partial stack with items that are not counted by charges
                item new_item( *sitem->it );
                if( by_charges ) {
                    new_item.charges = amount_to_move;
                }
                if( !add_item( destarea, new_item ) ) {
                    // make sure to redraw the info window, so 
                    // the user can see why it wasn't equipped
                    if(destarea == AIM_WORN) {
                        redraw = 1;
                    }
                    continue;
                }

                if( by_charges && amount_to_move < sitem->it->charges ) {
                    sitem->it->charges -= amount_to_move;
                } else {
                    remove_item( *sitem );
                }
            }
            // This is only reached when at least one item has been moved.
            g->u.moves -= 100; // In pickup/move functions this depends on item stats
            // Just in case the items have moved from/to the inventory
            g->u.inv.sort();
            g->u.inv.restack( &g->u );
        } else if( action == "MOVE_ALL_ITEMS" ) {
            exit = move_all_items();
            recalc = true;
        } else if( action == "SORT" ) {
            if( show_sort_menu( spane ) ) {
                redraw = true;
                recalc = true;
                uistate.adv_inv_sort[src] = spane.sortby;
            }
        } else if( action == "FILTER" ) {
            long key = 0;
            int spos = -1;
            std::string filter = spane.filter;
            filter_edit = true;

            g->draw_item_filter_rules( dpane.window, 12 );

            do {
                mvwprintz( spane.window, getmaxy( spane.window ) - 1, 2, c_cyan, "< " );
                mvwprintz( spane.window, getmaxy( spane.window ) - 1, ( w_width / 2 ) - 3, c_cyan, " >" );
                filter = string_input_win( spane.window, spane.filter, 256, 4,
                                           w_height - 1, ( w_width / 2 ) - 4, false, key, spos, "",
                                           4, getmaxy( spane.window ) - 1 );
                spane.set_filter( filter );
                redraw_pane( src );
            } while( key != '\n' && key != KEY_ESCAPE );
            filter_edit = false;
            spane.redraw = true;
            dpane.redraw = true;
        } else if( action == "RESET_FILTER" ) {
            spane.set_filter( "" );
        } else if( action == "TOGGLE_AUTO_PICKUP" ) {
            if( sitem == nullptr || !sitem->is_item_entry() ) {
                continue;
            }
            if( sitem->autopickup == true ) {
                removePickupRule( sitem->it->tname() );
                sitem->autopickup = false;
            } else {
                addPickupRule( sitem->it->tname() );
                sitem->autopickup = true;
            }
            recalc = true;
        } else if( action == "EXAMINE" ) {
            if( sitem == nullptr || !sitem->is_item_entry() ) {
                continue;
            }
            int ret = 0;
            if( spane.get_area() == AIM_INVENTORY || spane.get_area() == AIM_WORN ) {
                int idx = ( spane.get_area() == AIM_INVENTORY ) ?
                          sitem->idx : player::worn_position_to_index( sitem->idx );
                ret = g->inventory_item_menu( idx, colstart + ( src == left ? w_width / 2 : 0 ),
                                              w_width / 2, ( src == right ? 0 : -1 ) );
                // if player has started an activity, leave the screen and process it
                if( !g->u.has_activity( ACT_NULL ) ) {
                    exit = true;
                }
                // Might have changed a stack (activated an item, repaired an item, etc.)
                if( spane.get_area() == AIM_INVENTORY ) {
                    g->u.inv.restack( &g->u );
                }
                recalc = true;
            } else {
                item &it = *sitem->it;
                std::vector<iteminfo> vThisItem, vDummy;
                it.info( true, &vThisItem );
                int rightWidth = w_width / 2;
                ret = draw_item_info( colstart + ( src == left ? w_width / 2 : 0 ),
                                      rightWidth, 0, 0, it.tname(), vThisItem, vDummy );
            }
            if( ret == KEY_NPAGE || ret == KEY_DOWN ) {
                spane.scroll_by( +1 );
            } else if( ret == KEY_PPAGE || ret == KEY_UP ) {
                spane.scroll_by( -1 );
            }
            redraw = true; // item info window overwrote the other pane and the header
        } else if( action == "QUIT" ) {
            exit = true;
        } else if( action == "PAGE_DOWN" ) {
            spane.scroll_by( +itemsPerPage );
        } else if( action == "PAGE_UP" ) {
            spane.scroll_by( -itemsPerPage );
        } else if( action == "DOWN" ) {
            if( inCategoryMode ) {
                spane.scroll_category( +1 );
            } else {
                spane.scroll_by( +1 );
            }
        } else if( action == "UP" ) {
            if( inCategoryMode ) {
                spane.scroll_category( -1 );
            } else {
                spane.scroll_by( -1 );
            }
        } else if( action == "LEFT" ) {
            src = left;
            redraw = true;
        } else if( action == "RIGHT" ) {
            src = right;
            redraw = true;
        } else if( action == "TOGGLE_TAB" ) {
            src = dest;
            redraw = true;
        } else if( action == "TOGGLE_VEH" ){
            if( squares[spane.get_area()].can_store_in_vehicle() ) {
                // swap the panes if going vehicle will show the same tile
                if(spane.get_area() == dpane.get_area() && spane.in_vehicle() != dpane.in_vehicle()) {
                    swap_panes();
                // disallow for dragged vehicles
                } else if(spane.get_area() != AIM_DRAGGED) {
                    // Toggle between vehicle and ground
                    spane.set_area(squares[spane.get_area()], !spane.in_vehicle());
                    spane.index = 0;
                    spane.recalc = true;
                    if( dpane.get_area() == AIM_ALL ) {
                        dpane.recalc = true;
                    }
                    // make sure to update the minimap as well!
                    redraw = true;
                }
            } else {
                popup( _("No vehicle there!") );
            }
        }
    }
}

void advanced_inventory_pane::skip_category_headers( int offset )
{
    assert( offset != 0 ); // 0 would make no sense
    assert( static_cast<size_t>( index ) < items.size() ); // valid index is required
    assert( offset == -1 || offset == +1 ); // only those two offsets are allowed
    assert( !items.empty() ); // index would not be valid, and this would be an endless loop
    while( !items[index].is_item_entry() ) {
        mod_index( offset );
    }
}

void advanced_inventory_pane::mod_index( int offset )
{
    assert( offset != 0 ); // 0 would make no sense
    assert( !items.empty() );
    index += offset;
    if( index < 0 ) {
        index = static_cast<int>( items.size() ) - 1;
    } else if( static_cast<size_t>( index ) >= items.size() ) {
        index = 0;
    }
}

void advanced_inventory_pane::scroll_by( int offset )
{
    assert( offset != 0 ); // 0 would make no sense
    if( items.empty() ) {
        return;
    }
    mod_index( offset );
    skip_category_headers( offset > 0 ? +1 : -1 );
    redraw = true;
}

void advanced_inventory_pane::scroll_category( int offset )
{
    assert( offset != 0 ); // 0 would make no sense
    assert( offset == -1 || offset == +1 ); // only those two offsets are allowed
    if( items.empty() ) {
        return;
    }
    assert( get_cur_item_ptr() != nullptr ); // index must already be valid!
    auto cur_cat = items[index].cat;
    if( offset > 0 ) {
        while( items[index].cat == cur_cat ) {
            index++;
            if( static_cast<size_t>( index ) >= items.size() ) {
                index = 0; // wrap to begin, stop there.
                break;
            }
        }
    } else {
        while( items[index].cat == cur_cat ) {
            index--;
            if( index < 0 ) {
                index = static_cast<int>( items.size() ) - 1; // wrap to end, stop there.
                break;
            }
        }
    }
    // Make sure we land on an item entry.
    skip_category_headers( offset > 0 ? +1 : -1 );
    redraw = true;
}

advanced_inv_listitem *advanced_inventory_pane::get_cur_item_ptr()
{
    if( static_cast<size_t>( index ) >= items.size() ) {
        return nullptr;
    }
    return &items[index];
}

void advanced_inventory_pane::set_filter( const std::string &new_filter )
{
    if( filter == new_filter ) {
        return;
    }
    filter = new_filter;
    filtercache.clear();
    recalc = true;
}

bool advanced_inventory::query_destination( aim_location &def )
{
    if( def != AIM_ALL ) {
        if( squares[def].canputitems() ) {
            return true;
        }
        popup( _( "You can't put items there" ) );
        redraw = true; // the popup has messed the screen up.
        return false;
    }

    uimenu menu;
    menu.text = _( "Select destination" );
    menu.pad_left = 9; /* free space for advanced_inventory::menu_square */

    // the direction locations should be continues in the enum
    assert( AIM_NORTHEAST - AIM_SOUTHWEST == 8 );
    for( int i = AIM_SOUTHWEST; i <= AIM_NORTHEAST; i++ ) {
        auto &s = squares[i];
        const int size = s.get_item_count();
        std::string prefix = string_format( "%2d/%d", size, MAX_ITEM_IN_SQUARE );
        if( size >= MAX_ITEM_IN_SQUARE ) {
            prefix += _( " (FULL)" );
        }
        menu.addentry( i, ( s.canputitems() && s.id != panes[src].get_area() ),
                       i + '0', prefix + " " + s.name + " " +
                       ( s.veh != nullptr ? s.veh->name : "" ) );
    }

    // Selected keyed to uimenu.entries, which starts at 0.
    menu.selected = uistate.adv_inv_last_popup_dest - AIM_SOUTHWEST;
    menu.show(); // generate and show window.
    while( menu.ret == UIMENU_INVALID && menu.keypress != 'q' && menu.keypress != KEY_ESCAPE ) {
        // Render a fancy ascii grid at the left of the menu.
        menu_square( &menu );
        menu.query( false ); // query, but don't loop
    }
    redraw = true; // the menu has messed the screen up.
    if( menu.ret >= AIM_SOUTHWEST && menu.ret <= AIM_NORTHEAST ) {
        assert( squares[menu.ret].canputitems() );
        def = static_cast<aim_location>( menu.ret );
        uistate.adv_inv_last_popup_dest = menu.ret;
        return true;
    }
    return false;
}

void advanced_inventory::remove_item( advanced_inv_listitem &sitem )
{
    assert( sitem.area != AIM_ALL );        // should be a specific location instead
    assert( sitem.area != AIM_INVENTORY );  // does not work for inventory
    assert( sitem.area != AIM_WORN );       // yeah what he said
    assert( sitem.it != nullptr );
    bool rc = false;

    auto &s = squares[sitem.area];
    if( s.id == AIM_CONTAINER ) {
        const auto cont = s.get_container( panes[src].in_vehicle() );
        assert( cont != nullptr );
        assert( !cont->contents.empty() );
        assert( &cont->contents.front() == sitem.it );
        cont->contents.erase( cont->contents.begin() );
        rc = true;
    } else if( sitem.area == AIM_WORN ) {
        rc = g->u.takeoff( sitem.it );
    } else if( sitem.from_vehicle ) {
        rc = s.veh->remove_item( s.vstor, sitem.it );
    }

    if( !rc ) {
        // TODO: Make this print an error if it fails
        g->m.i_rem( s.pos, sitem.it );
    }
}

bool advanced_inventory::add_item( aim_location destarea, item &new_item )
{
    assert( destarea != AIM_ALL ); // should be a specific location instead
    bool rc = true;
    if( destarea == AIM_INVENTORY ) {
        g->u.i_add( new_item );
        g->u.moves -= 100;
        return rc;
    } else if( destarea == AIM_WORN ) {
        return g->u.wear_item(&new_item);
    } else {
        advanced_inv_area &p = squares[destarea];
        if( panes[dest].in_vehicle() ) {
            rc = p.veh->add_item( p.vstor, new_item );
        } else {
            rc = g->m.add_item_or_charges( p.pos, new_item, 0 );
        }
    }
    if( rc == false ) {
        popup( _( "Destination area is full.  Remove some items first" ) );
    }
    return rc;
}

bool advanced_inventory::move_content( item &src_container, item &dest_container )
{
    if( !src_container.is_watertight_container() ) {
        popup( _( "Source must be watertight container." ) );
        return false;
    }
    if( src_container.is_container_empty() ) {
        popup( _( "Source container is empty." ) );
        return false;
    }

    item &src = src_container.contents[0];

    if( !src.made_of( LIQUID ) ) {
        popup( _( "You can unload only liquids into target container." ) );
        return false;
    }

    std::string err;
    if( !dest_container.fill_with( src, err ) ) {
        popup( err.c_str() );
        return false;
    }

    uistate.adv_inv_container_content_type = dest_container.contents[0].typeId();
    if( src.charges <= 0 ) {
        src_container.contents.clear();
    }

    return true;
}

int advanced_inv_area::free_volume( bool in_vehicle ) const
{
    assert( id != AIM_ALL ); // should be a specific location instead
    if( id == AIM_INVENTORY || id == AIM_WORN ) {
        return ( g->u.volume_capacity() - g->u.volume_carried() );
    }
    return (in_vehicle) ? veh->free_volume( vstor ) : g->m.free_volume( pos );
}

bool advanced_inventory::query_charges( aim_location destarea, const advanced_inv_listitem &sitem,
                                        bool askamount, long &amount )
{
    assert( destarea != AIM_ALL ); // should be a specific location instead
    assert( sitem.it != nullptr ); // valid item is obviously required
    const item &it = *sitem.it;
    advanced_inv_area &p = squares[destarea];
    const bool by_charges = it.count_by_charges();
    const int unitvolume = it.precise_unit_volume();
    const int free_volume = 1000 * p.free_volume( panes[dest].in_vehicle() );
    // default to move all, unless if being equipped
    const long input_amount = by_charges ? it.charges : ( destarea == AIM_WORN ) ? 1 : sitem.stacks;
    assert( input_amount > 0 ); // there has to be something to begin with
    amount = input_amount;

    // Picking up stuff might not be possible at all
    if( ( destarea == AIM_INVENTORY || destarea == AIM_WORN ) && !g->u.can_pickup( true ) ) {
        return false;
    }
    // Includes moving from/to inventory and around on the map.
    if( it.made_of( LIQUID ) ) {
        popup( _( "You can't pick up a liquid." ) );
        redraw = true;
        return false;
    }
    // Check volume, this should work the same for inventory, map and vehicles, but not for worn
    if( squares[destarea].id != AIM_WORN && unitvolume > 0 && ( unitvolume * amount ) > free_volume ) {
        const long volmax = free_volume / unitvolume;
        if( volmax <= 0 ) {
            popup( _( "Destination area is full.  Remove some items first." ) );
            redraw = true;
            return false;
        }
        amount = std::min( volmax, amount );
    }
    // Map and vehicles have a maximal item count, check that. Inventory does not have this.
    if( destarea != AIM_INVENTORY && destarea != AIM_WORN && destarea != AIM_CONTAINER ) {
        const long cntmax = p.max_size - p.get_item_count();
        if( cntmax <= 0 ) {
            // TODO: items by charges might still be able to be add to an existing stack!
            popup( _( "Destination area has too many items.  Remove some first." ) );
            redraw = true;
            return false;
        }
        // Items by charge count as a single item, regardless of the charges. As long as the
        // destination can hold another item, one can move all charges.
        if( !by_charges ) {
            amount = std::min( cntmax, amount );
        }
    }
    // Inventory has a weight capacity, map and vehicle don't have that
    if( destarea == AIM_INVENTORY  || destarea == AIM_WORN ) {
        const int unitweight = it.weight() * 1000 / ( by_charges ? it.charges : 1 );
        const int max_weight = ( g->u.weight_capacity() * 4 - g->u.weight_carried() ) * 1000;
        if( unitweight > 0 && unitweight * amount > max_weight ) {
            const long weightmax = max_weight / unitweight;
            if( weightmax <= 0 ) {
                popup( _( "This is too heavy!." ) );
                redraw = true;
                return false;
            }
            amount = std::min( weightmax, amount );
        }
    }

    // Now we have the final amount. Query if needed (either requested, or when
    // the destination can not hold all items).
    if( askamount || amount < input_amount ) {
        // moving several items (not charges!) from ground it currently not implemented.
        // TODO: implement this properly, see the code where this is called from.
        if( !by_charges && sitem.area != AIM_INVENTORY && sitem.area != AIM_WORN ) {
            amount = 1;
            return true;
        }
        std::string popupmsg = _( "How many do you want to move? (0 to cancel)" );
        // At this point amount contains the maximal amount that the destination can hold.
        if( amount < input_amount ) {
            popupmsg = string_format( _( "Destination can only hold %d! Move how many? (0 to cancel) " ),
                                      amount );
        }
        // instead of prompting to move all, prompt to move all _but_ one
        const long possible_max = std::min( input_amount, amount );
        amount = std::atoi( string_input_popup( popupmsg, 20, 
                    to_string( possible_max ), "", "", -1, true ).c_str() );
        if( amount <= 0 ) {
            return false;
        }
        if( amount > possible_max ) {
            amount = possible_max;
        }
    }
    return true;
}

bool advanced_inv_area::is_same( const advanced_inv_area &other ) const
{
    // All locations (sans the below) are compared by the coordinates, 
    // e.g. dragged vehicle (to the south) and AIM_SOUTH are the same.
    if( id != AIM_INVENTORY && other.id != AIM_INVENTORY && 
        id != AIM_WORN      && other.id != AIM_WORN      &&
        id != AIM_CONTAINER && other.id != AIM_CONTAINER ) {
        //     have a vehicle?...     ...do the cargo index and pos match?...    ...at least pos?
        return (veh == other.veh) ? (pos == other.pos && vstor == other.vstor) : pos == other.pos;
    }
    //      ...is the id?
    return id == other.id;
}

bool advanced_inv_area::canputitems( const advanced_inv_listitem *advitem )
{
    bool canputitems = false;
    bool from_vehicle = false;
    item *it = nullptr;
    switch( id ) {
        case AIM_CONTAINER:
            if( advitem != nullptr && advitem->is_item_entry() ) {
                it = advitem->it;
                from_vehicle = advitem->from_vehicle;
            }
            if(get_container(from_vehicle) != nullptr) {
                it = get_container(from_vehicle);
            }
            if( it != nullptr ) {
                canputitems = it->is_watertight_container();
            }
            break;
        default:
            canputitems = canputitemsloc;
            break;
    }
    return canputitems;
}

item *advanced_inv_area::get_container( bool in_vehicle )
{
    item *container = nullptr;

    if( uistate.adv_inv_container_location != -1 ) {
        // try to find valid container in the area
        if( uistate.adv_inv_container_location == AIM_INVENTORY ) {
            const invslice &stacks = g->u.inv.slice();

            // check index first
            if( stacks.size() > ( size_t )uistate.adv_inv_container_index ) {
                auto &it = stacks[uistate.adv_inv_container_index]->front();
                if( is_container_valid( &it ) ) {
                    container = &it;
                }
            }

            // try entire area
            if( container == nullptr ) {
                for( size_t x = 0; x < stacks.size(); ++x ) {
                    auto &it = stacks[x]->front();
                    if( is_container_valid( &it ) ) {
                        container = &it;
                        uistate.adv_inv_container_index = x;
                        break;
                    }
                }
            }
        } else if( uistate.adv_inv_container_location == AIM_WORN ) {
            auto worn = g->u.worn;
            size_t idx = ( size_t )uistate.adv_inv_container_index;
            if( worn.size() > idx ) {
                if( is_container_valid( &worn[idx] ) ) {
                    container = &worn[idx];
                }
            }

            // no need to reinvent the wheel
            if( container == nullptr ) {
                for( size_t i = 0; i < worn.size(); ++i ) {
                    if( is_container_valid( &worn[i] ) ) {
                        container = &worn[i];
                        uistate.adv_inv_container_index = i;
                        break;
                    }
                }
            }
        } else {
            map &m = g->m;
            bool is_in_vehicle = uistate.adv_inv_container_in_vehicle || 
                (can_store_in_vehicle() && in_vehicle);
            const itemslice &stacks = (is_in_vehicle == true) ? 
                i_stacked( veh->get_items( vstor ) ) : 
                i_stacked( m.i_at( pos ) );

            // check index first
            if( stacks.size() > ( size_t )uistate.adv_inv_container_index ) {
                auto it = stacks[uistate.adv_inv_container_index].first;
                if( is_container_valid( it ) ) {
                    container = it;
                }
            }

            // try entire area
            if( container == nullptr ) {
                for( size_t x = 0; x < stacks.size(); ++x ) {
                    auto it = stacks[x].first;
                    if( is_container_valid( it ) ) {
                        container = it;
                        uistate.adv_inv_container_index = x;
                        break;
                    }
                }
            }
        }

        // no valid container in the area, resetting container
        if( container == nullptr ) {
            set_container( nullptr );
            desc[0] = _( "Invalid container" );
        }
    }

    return container;
}

void advanced_inv_area::set_container( const advanced_inv_listitem *advitem )
{
    if( advitem != nullptr ) {
        item *it( advitem->it );
        uistate.adv_inv_container_location = advitem->area;
        uistate.adv_inv_container_in_vehicle = advitem->from_vehicle;
        uistate.adv_inv_container_index = advitem->idx;
        uistate.adv_inv_container_type = it->typeId();
        uistate.adv_inv_container_content_type = ( !it->is_container_empty() ) ? 
            it->contents[0].typeId() : "null";
        set_container_position();
    } else {
        uistate.adv_inv_container_location = -1;
        uistate.adv_inv_container_index = 0;
        uistate.adv_inv_container_in_vehicle = false;
        uistate.adv_inv_container_type = "null";
        uistate.adv_inv_container_content_type = "null";
    }
}

bool advanced_inv_area::is_container_valid( const item *it ) const
{
    if( it != nullptr ) {
        if( it->typeId() == uistate.adv_inv_container_type ) {
            if( it->is_container_empty() ) {
                if( uistate.adv_inv_container_content_type == "null" ) {
                    return true;
                }
            } else {
                if( it->contents[0].typeId() == uistate.adv_inv_container_content_type ) {
                    return true;
                }
            }
        }
    }

    return false;
}

void advanced_inv_area::set_container_position()
{
    // update the offset of the container based on location
    switch( uistate.adv_inv_container_location ) {
        case AIM_DRAGGED:
            off = g->u.grab_point;
            break;
        case AIM_SOUTHWEST:
            off = tripoint( -1, 1, 0 );
            break;
        case AIM_SOUTH:
            off = tripoint( 0, 1, 0 );
            break;
        case AIM_SOUTHEAST:
            off = tripoint( 1, 1, 0 );
            break;
        case AIM_WEST:
            off = tripoint( -1, 0, 0 );
            break;
        case AIM_EAST:
            off = tripoint( 1, 0, 0 );
            break;
        case AIM_NORTHWEST:
            off = tripoint( -1, -1, 0 );
            break;
        case AIM_NORTH:
            off = tripoint( 0, -1, 0 );
            break;
        case AIM_NORTHEAST:
            off = tripoint( 1, -1, 0 );
            break;
        default:
            off = tripoint( 0, 0, 0 );
            break;
    }
    // update the absolute position
    pos = g->u.pos() + off;
    // update vehicle information
    vstor = -1;
    veh = g->m.veh_at( pos, vstor );
    if( veh != nullptr ) {
        vstor = veh->part_with_feature( vstor, "CARGO", false );
    }
    if( vstor < 0 ) {
        veh = nullptr;
    }
}


void advanced_inv()
{
    advanced_inventory advinv;
    advinv.display();
}

void advanced_inventory::refresh_minimap()
{
    // redraw border around minimap
    draw_border( mm_border );
    // minor addition to border for AIM_ALL, sorta hacky
    if( panes[src].get_area() == AIM_ALL || panes[dest].get_area() == AIM_ALL ) {
        mvwprintz( mm_border, 0, 1, c_ltgray,
                   utf8_truncate( _( "All" ), minimap_width ).c_str() );
    }
    // refresh border, then minimap
    wrefresh( mm_border );
    wrefresh( minimap );
}

void advanced_inventory::draw_minimap()
{
    static const std::array<aim_location, 3> places = {
        {AIM_CENTER, AIM_INVENTORY, AIM_WORN}
    };
    static const std::array<side, NUM_PANES> sides = {{left, right}};
    // get the center of the window
    tripoint pc = {getmaxx(minimap) / 2, getmaxy(minimap) / 2, 0};
    // draw the 3x3 tiles centered around player
    g->m.draw( minimap, g->u.pos() );
    for(auto s : sides) {
        char sym = get_minimap_sym(s);
        if(sym == '\0') { continue; }
        auto sq = squares[panes[s].get_area()];
        auto pt = pc + sq.off;
        // invert the color if pointing to the player's position
        auto cl = (sq.id == AIM_INVENTORY || sq.id == AIM_WORN) ? 
            invert_color(c_ltcyan) : c_ltcyan | A_BLINK;
        mvwputch(minimap, pt.y, pt.x, static_cast<nc_color>(cl), sym);
    }
    // draw the player iff neither pane's area isn't in `places'
    bool lip = is_any_of(panes[src].get_area(), places);
    bool rip = is_any_of(panes[dest].get_area(), places);
    if(!(lip && rip)) {
        // draw the player in the center of the map
        g->u.draw(minimap, g->u.pos(), (lip || rip));
    }
}

char advanced_inventory::get_minimap_sym(side p) const
{
    static const std::array<char, NUM_PANES> c_side = {{'L', 'R'}};
    static const std::array<char, NUM_PANES> d_side = {{'^', 'v'}};
    static const std::array<char, NUM_AIM_LOCATIONS> g_nome = {{
        '@', '#', '#', '#', '#', '@', '#', 
        '#', '#', '#', 'D', '^', 'C', '@'
    }};
    char ch = g_nome[panes[p].get_area()];
    switch(ch) {
        case '@': // '^' or 'v'
            ch = d_side[panes[-p+1].get_area() == AIM_CENTER];
            break;
        case '#': // 'L' or 'R'
            ch = (panes[p].in_vehicle()) ? 'V' : c_side[p];
            break;
        case '^': // do not show anything
            ch ^= ch;
            break;
    }
    return ch;
}

aim_location advanced_inv_area::offset_to_location() const
{
    static aim_location loc_array[3][3] = {
        {AIM_NORTHWEST,     AIM_NORTH,      AIM_NORTHEAST},
        {AIM_WEST,          AIM_CENTER,     AIM_EAST},
        {AIM_SOUTHWEST,     AIM_SOUTH,      AIM_SOUTHEAST}
    };
    return loc_array[off.y + 1][off.x + 1];
}

void advanced_inventory::swap_panes()
{
    // Switch left and right pane.
    std::swap( panes[left], panes[right] );
    // Window pointer must be unchanged!
    std::swap( panes[left].window, panes[right].window );
    // No recalculation needed, data has not changed
    redraw = true;
}

void advanced_inventory::do_return_entry()
{
    // only save pane settings
    save_settings(true);
    g->u.assign_activity( ACT_ADV_INVENTORY, 0 );
    g->u.activity.auto_resume = true;
}

