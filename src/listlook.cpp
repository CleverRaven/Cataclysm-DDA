#include "game.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "avatar.h"
#include "avatar_action.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "color.h"
#include "coordinates.h"
#include "creature.h"
#include "cursesdef.h"
#include "debug.h"
#include "enums.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "game_ui.h"
#include "input_context.h"
#include "input_enums.h"
#include "input_popup.h"
#include "item.h"
#include "item_category.h"
#include "item_contents.h"
#include "item_location.h"
#include "line.h"
#include "map.h"
#include "map_item_stack.h"
#include "map_scale_constants.h"
#include "memory_fast.h"
#include "messages.h"
#include "monster.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "panels.h"
#include "point.h"
#include "safemode_ui.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "ui_manager.h"
#include "uilist.h"
#include "uistate.h"

static const trait_id trait_INATTENTIVE( "INATTENTIVE" );

static void draw_trail( const tripoint_bub_ms &start, const tripoint_bub_ms &end, bool bDrawX );

static shared_ptr_fast<game::draw_callback_t> create_trail_callback(
    const std::optional<tripoint_bub_ms> &trail_start,
    const std::optional<tripoint_bub_ms> &trail_end,
    const bool &trail_end_x
)
{
    return make_shared_fast<game::draw_callback_t>(
    [&]() {
        if( trail_start && trail_end ) {
            draw_trail( trail_start.value(), trail_end.value(), trail_end_x );
        }
    } );
}

void draw_trail( const tripoint_bub_ms &start, const tripoint_bub_ms &end, const bool bDrawX )
{
    map &here = get_map();

    std::vector<tripoint_bub_ms> pts;
    avatar &player_character = get_avatar();
    const tripoint_bub_ms pos = player_character.pos_bub( here );
    tripoint_bub_ms center = pos + player_character.view_offset;
    if( start != end ) {
        //Draw trail
        pts = line_to( start, end, 0, 0 );
    } else {
        //Draw point
        pts.emplace_back( start );
    }

    g->draw_line( end, center, pts );
    if( bDrawX ) {
        char sym = 'X';
        if( end.z() > center.z() ) {
            sym = '^';
        } else if( end.z() < center.z() ) {
            sym = 'v';
        }
        if( pts.empty() ) {
            mvwputch( g->w_terrain, point( POSX, POSY ), c_white, sym );
        } else {
            mvwputch( g->w_terrain, pts.back().raw().xy() - player_character.view_offset.xy().raw() +
                      point( POSX - pos.x(), POSY - pos.y() ),
                      c_white, sym );
        }
    }
}

void game::draw_trail_to_square( const tripoint_rel_ms &t, bool bDrawX )
{
    ::draw_trail( u.pos_bub(), u.pos_bub() + t, bDrawX );
}

static void centerlistview( const tripoint &active_item_position, int ui_width )
{
    Character &u = get_avatar();
    if( get_option<std::string>( "SHIFT_LIST_ITEM_VIEW" ) != "false" ) {
        u.view_offset.z() = active_item_position.z;
        if( get_option<std::string>( "SHIFT_LIST_ITEM_VIEW" ) == "centered" ) {
            u.view_offset.x() = active_item_position.x;
            u.view_offset.y() = active_item_position.y;
        } else {
            point pos( active_item_position.xy() + point( POSX, POSY ) );

            // item/monster list UI is on the right, so get the difference between its width
            // and the width of the sidebar on the right (if any)
            int sidebar_right_adjusted = ui_width - panel_manager::get_manager().get_width_right();
            // if and only if that difference is greater than zero, use that as offset
            int right_offset = sidebar_right_adjusted > 0 ? sidebar_right_adjusted : 0;

            // Convert offset to tile counts, calculate adjusted terrain window width
            // This lets us account for possible differences in terrain width between
            // the normal sidebar and the list-all-whatever display.
            to_map_font_dim_width( right_offset );
            int terrain_width = TERRAIN_WINDOW_WIDTH - right_offset;

            if( pos.x < 0 ) {
                u.view_offset.x() = pos.x;
            } else if( pos.x >= terrain_width ) {
                u.view_offset.x() = pos.x - ( terrain_width - 1 );
            } else {
                u.view_offset.x() = 0;
            }

            if( pos.y < 0 ) {
                u.view_offset.y() = pos.y;
            } else if( pos.y >= TERRAIN_WINDOW_HEIGHT ) {
                u.view_offset.y() = pos.y - ( TERRAIN_WINDOW_HEIGHT - 1 );
            } else {
                u.view_offset.y() = 0;
            }
        }
    }

}

//helper method so we can keep list_items shorter
void game::reset_item_list_state( const catacurses::window &window, int height,
                                  list_item_sort_mode sortMode )
{
    const int width = getmaxx( window );
    wattron( window, c_light_gray );

    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwhline( window, point( 1,                  0 ), LINE_OXOX, width - 1 ); // -
    mvwhline( window, point( 1, TERMY - height - 1 ), LINE_OXOX, width - 1 ); // -
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwvline( window, point( 0,         1 ), LINE_XOXO, TERMY - height - 1 ); // |
    mvwvline( window, point( width - 1, 1 ), LINE_XOXO, TERMY - height - 1 ); // |

    mvwaddch( window, point::zero,           LINE_OXXO ); // |^
    mvwaddch( window, point( width - 1, 0 ), LINE_OOXX ); // ^|

    mvwaddch( window, point( 0, TERMY - height - 1 ), LINE_XXXO ); // |-
    mvwaddch( window, point( width - 1, TERMY - height - 1 ), LINE_XOXX ); // -|
    wattroff( window, c_light_gray );

    mvwprintz( window, point( 2, 0 ), c_light_green, "<Tab> " );
    wprintz( window, c_white, _( "Items" ) );

    std::string sSort;
    switch( sortMode ) {
        // Sort by distance only
        case list_item_sort_mode::count:
        case list_item_sort_mode::DISTANCE:
            sSort = _( "<s>ort: dist" );
            break;
        // Sort by name only
        case list_item_sort_mode::NAME:
            sSort = _( "<s>ort: name" );
            break;
        // Group by category, sort by distance
        case list_item_sort_mode::CATEGORY_DISTANCE:
            sSort = _( "<s>ort: cat-dist" );
            break;
        // Group by category, sort by item name
        case list_item_sort_mode::CATEGORY_NAME:
            sSort = _( "<s>ort: cat-name" );
            break;
    }

    int letters = utf8_width( sSort );

    shortcut_print( window, point( getmaxx( window ) - letters, 0 ), c_white, c_light_green, sSort );

    std::vector<std::string> tokens;
    tokens.reserve( 5 + ( !sFilter.empty() ? 1 : 0 ) );
    if( !sFilter.empty() ) {
        tokens.emplace_back( _( "<R>eset" ) );
    }

    tokens.emplace_back( _( "<E>xamine" ) );
    tokens.emplace_back( _( "<C>ompare" ) );
    tokens.emplace_back( _( "<F>ilter" ) );
    tokens.emplace_back( _( "<+/->Priority" ) );
    tokens.emplace_back( _( "<T>ravel to" ) );

    int gaps = tokens.size() + 1;
    letters = 0;
    int n = tokens.size();
    for( int i = 0; i < n; i++ ) {
        letters += utf8_width( tokens[i] ) - 2; //length ignores < >
    }

    int usedwidth = letters;
    const int gap_spaces = ( width - usedwidth ) / gaps;
    usedwidth += gap_spaces * gaps;
    point pos( gap_spaces + ( width - usedwidth ) / 2, TERMY - height - 1 );

    for( int i = 0; i < n; i++ ) {
        pos.x += shortcut_print( window, pos, c_white, c_light_green,
                                 tokens[i] ) + gap_spaces;
    }
}

template<>
std::string io::enum_to_string<list_item_sort_mode>( list_item_sort_mode data )
{
    switch( data ) {
        case list_item_sort_mode::count:
        case list_item_sort_mode::DISTANCE:
            return "DISTANCE";
        case list_item_sort_mode::NAME:
            return "NAME";
        case list_item_sort_mode::CATEGORY_DISTANCE:
            return "CATEGORY_DISTANCE";
        case list_item_sort_mode::CATEGORY_NAME:
            return "CATEGORY_NAME";
    }
    cata_fatal( "Invalid list item sort mode" );
}

void game::list_items_monsters()
{
    // Search whole reality bubble because each function internally verifies
    // the visibility of the items / monsters in question.
    std::vector<Creature *> mons = u.get_visible_creatures( MAX_VIEW_DISTANCE );
    const std::vector<map_item_stack> items = find_nearby_items( MAX_VIEW_DISTANCE );

    if( mons.empty() && items.empty() ) {
        add_msg( m_info, _( "You don't see any items or monsters around you!" ) );
        return;
    }

    std::sort( mons.begin(), mons.end(), [&]( const Creature * lhs, const Creature * rhs ) {
        if( !u.has_trait( trait_INATTENTIVE ) ) {
            const Creature::Attitude att_lhs = lhs->attitude_to( u );
            const Creature::Attitude att_rhs = rhs->attitude_to( u );

            return att_lhs < att_rhs || ( att_lhs == att_rhs
                                          && rl_dist( u.pos_bub(), lhs->pos_bub() ) < rl_dist( u.pos_bub(), rhs->pos_bub() ) );
        } else { // Sort just by ditance if player has inattentive trait
            return ( rl_dist( u.pos_bub(), lhs->pos_bub() ) < rl_dist( u.pos_bub(), rhs->pos_bub() ) );
        }

    } );

    // If the current list is empty, switch to the non-empty list
    if( uistate.vmenu_show_items ) {
        if( items.empty() ) {
            uistate.vmenu_show_items = false;
        }
    } else if( mons.empty() ) {
        uistate.vmenu_show_items = true;
    }

    temp_exit_fullscreen();
    game::vmenu_ret ret;
    while( true ) {
        ret = uistate.vmenu_show_items ? list_items( items ) : list_monsters( mons );
        if( ret == game::vmenu_ret::CHANGE_TAB ) {
            uistate.vmenu_show_items = !uistate.vmenu_show_items;
        } else {
            break;
        }
    }

    if( ret == game::vmenu_ret::FIRE ) {
        avatar_action::fire_wielded_weapon( u );
    }
    reenter_fullscreen();
}

static std::string list_items_filter_history_help()
{
    input_context ctxt( "STRING_INPUT" );
    std::vector<std::string> act_descs;
    const auto add_action_desc = [&]( const std::string & act, const std::string & txt ) {
        act_descs.emplace_back( ctxt.get_desc( act, txt, input_context::allow_all_keys ) );
    };
    add_action_desc( "HISTORY_UP", pgettext( "string input", "History" ) );
    add_action_desc( "TEXT.CLEAR", pgettext( "string input", "Clear text" ) );
    add_action_desc( "TEXT.QUIT", pgettext( "string input", "Abort" ) );
    add_action_desc( "TEXT.CONFIRM", pgettext( "string input", "Save" ) );
    return colorize( enumerate_as_string( act_descs, enumeration_conjunction::none ), c_green );
}

/// return content_newness based on if item is known and nested items are known
static content_newness check_items_newness( const item *itm )
{
    if( !uistate.read_items.count( itm->typeId() ) ) {
        return content_newness::NEW;
    }
    return itm->get_contents().get_content_newness( uistate.read_items );
}

/// add item and all visible nested items inside to known items
static void mark_items_read_rec( const item *itm )
{
    uistate.read_items.insert( itm->typeId() );
    for( const item *child : itm->all_known_contents() ) {
        mark_items_read_rec( child );
    }
}

game::vmenu_ret game::list_items( const std::vector<map_item_stack> &item_list )
{
    std::vector<map_item_stack> ground_items = item_list;
    int iInfoHeight = 0;
    int iMaxRows = 0;
    int width = 0;
    int width_nob = 0;  // width without borders
    int max_name_width = 0;

    const bool highlight_unread_items = get_option<bool>( "HIGHLIGHT_UNREAD_ITEMS" );
    const nc_color item_new_col = c_light_green;
    const nc_color item_maybe_new_col = c_light_gray;
    const std::string item_new_str = pgettext( "crafting gui", "NEW!" );
    const std::string item_maybe_new_str = pgettext( "crafting gui", "hides contents" );
    const int item_new_str_width = utf8_width( item_new_str );
    const int item_maybe_new_str_width = utf8_width( item_maybe_new_str );

    // Constants for content that is always displayed in w_items
    // on left: 1 space
    const int left_padding = 1;
    // on right: 2 digit distance 1 space 2 direction 1 space
    const int right_padding = 2 + 1 + 2 + 1;
    const int padding = left_padding + right_padding;

    //find max length of item name and resize window width
    for( const map_item_stack &cur_item : ground_items ) {
        max_name_width = std::max( max_name_width,
                                   utf8_width( remove_color_tags( cur_item.example->display_name() ) ) );
    }
    // + 6 as estimate for `iThisPage` I guess
    max_name_width = max_name_width + padding + 6 + ( highlight_unread_items ? std::max(
                         item_new_str_width, item_maybe_new_str_width ) : 0 );

    tripoint_rel_ms active_pos;
    map_item_stack *activeItem = nullptr;

    catacurses::window w_items;
    catacurses::window w_items_border;
    catacurses::window w_item_info;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        iInfoHeight = std::min( 25, TERMY / 2 );
        iMaxRows = TERMY - iInfoHeight - 2;

        width = clamp( max_name_width, 55, TERMX / 3 );
        width_nob = width - 2;

        const int offsetX = TERMX - width;

        w_items = catacurses::newwin( TERMY - 2 - iInfoHeight,
                                      width_nob, point( offsetX + 1, 1 ) );
        w_items_border = catacurses::newwin( TERMY - iInfoHeight,
                                             width, point( offsetX, 0 ) );
        w_item_info = catacurses::newwin( iInfoHeight, width,
                                          point( offsetX, TERMY - iInfoHeight ) );

        if( activeItem ) {
            centerlistview( active_pos.raw(), width );
        }

        ui.position( point( offsetX, 0 ), point( width, TERMY ) );
    } );
    ui.mark_resize();

    // reload filter/priority settings on the first invocation, if they were active
    if( !uistate.list_item_init ) {
        if( uistate.list_item_filter_active ) {
            sFilter = uistate.list_item_filter;
        }
        if( uistate.list_item_downvote_active ) {
            list_item_downvote = uistate.list_item_downvote;
        }
        if( uistate.list_item_priority_active ) {
            list_item_upvote = uistate.list_item_priority;
        }
        uistate.list_item_init = true;
    }

    //this stores only those items that match our filter
    std::vector<map_item_stack> filtered_items =
        !sFilter.empty() ? filter_item_stacks( ground_items, sFilter ) : ground_items;
    int highPEnd = list_filter_high_priority( filtered_items, list_item_upvote );
    int lowPStart = list_filter_low_priority( filtered_items, highPEnd, list_item_downvote );
    int iItemNum = ground_items.size();

    const tripoint_rel_ms stored_view_offset = u.view_offset;

    u.view_offset = tripoint_rel_ms::zero;

    int iActive = 0; // Item index that we're looking at
    int page_num = 0;
    int iCatSortNum = 0;
    int iScrollPos = 0;
    std::map<int, std::string> mSortCategory;

    ui.on_redraw( [&]( ui_adaptor & ui ) {
        reset_item_list_state( w_items_border, iInfoHeight, uistate.list_item_sort );

        int iStartPos = 0;
        if( ground_items.empty() ) {
            wnoutrefresh( w_items_border );
            mvwprintz( w_items, point( 2, 10 ), c_white, _( "You don't see any items around you!" ) );
        } else {
            werase( w_items );
            calcStartPos( iStartPos, iActive, iMaxRows, iItemNum );
            int iNum = 0;
            // ITEM LIST
            int index = 0;
            int iCatSortOffset = 0;

            for( int i = 0; i < iStartPos; i++ ) {
                if( !mSortCategory[i].empty() ) {
                    iNum++;
                }
            }
            for( auto iter = filtered_items.begin(); iter != filtered_items.end(); ++index, iNum++ ) {
                if( iNum < iStartPos || iNum >= iStartPos + std::min( iMaxRows, iItemNum ) ) {
                    ++iter;
                    continue;
                }
                int iThisPage = 0;
                if( !mSortCategory[iNum].empty() ) {
                    iCatSortOffset++;
                    mvwprintz( w_items, point( 1, iNum - iStartPos ), c_magenta, mSortCategory[iNum] );
                    continue;
                }
                if( iNum == iActive ) {
                    iThisPage = page_num;
                }
                std::string sText;
                if( iter->vIG.size() > 1 ) {
                    sText += string_format( "[%d/%d] (%d) ", iThisPage + 1, iter->vIG.size(), iter->totalcount );
                }
                sText += iter->example->tname();
                if( iter->vIG[iThisPage].count > 1 ) {
                    sText += string_format( "[%d]", iter->vIG[iThisPage].count );
                }

                nc_color col = c_light_gray;
                if( iNum == iActive ) {
                    col = hilite( c_white );
                } else if( highPEnd > 0 && index < highPEnd + iCatSortOffset ) {  // priority high
                    col = c_yellow;
                } else if( index >= lowPStart + iCatSortOffset ) {  // priority low
                    col = c_red;
                } else {  // priority medium
                    col = iter->example->color_in_inventory();
                }
                bool print_new = highlight_unread_items;
                const std::string *new_str = nullptr;
                // 1 make space between item description and right padding (distance)
                int new_width = 1;
                const nc_color *new_col = nullptr;
                if( print_new ) {
                    switch( check_items_newness( iter->example ) ) {
                        case content_newness::NEW:
                            new_str =  &item_new_str;
                            // +1 make space between item description and "new"
                            new_width += item_new_str_width + 1;
                            new_col =  &item_new_col;
                            break;
                        case content_newness::MIGHT_BE_HIDDEN:
                            new_str =  &item_maybe_new_str;
                            new_width += item_maybe_new_str_width + 1;
                            new_col =  &item_maybe_new_col;
                            break;
                        case content_newness::SEEN:
                            print_new = false;
                            break;
                    }
                }
                trim_and_print( w_items, point( 1, iNum - iStartPos ), width_nob - padding - new_width, col,
                                sText );
                const point_rel_ms p( iter->vIG[iThisPage].pos.xy() );
                if( print_new ) {
                    // +1 move space between item description and "new"
                    mvwprintz( w_items, point( width_nob - right_padding - new_width + 1, iNum - iStartPos ), *new_col,
                               *new_str );
                }
                mvwprintz( w_items, point( width_nob - right_padding, iNum - iStartPos ),
                           iNum == iActive ? c_light_green : c_light_gray,
                           "%2d %s", rl_dist( point_rel_ms::zero, p ),
                           direction_name_short( direction_from( point_rel_ms::zero, p ) ) );
                ++iter;
            }
            // ITEM DESCRIPTION
            iNum = 0;
            for( int i = 0; i < iActive; i++ ) {
                if( !mSortCategory[i].empty() ) {
                    iNum++;
                }
            }
            const int current_i = iItemNum > 0 ? iActive - iNum + 1 : 0;
            const int numd = current_i > 999 ? 4 :
                             current_i > 99 ? 3 :
                             current_i > 9 ? 2 : 1;
            mvwprintz( w_items_border, point( width / 2 - numd - 2, 0 ), c_light_green, " %*d", numd,
                       current_i );
            wprintz( w_items_border, c_white, " / %*d ", numd, iItemNum - iCatSortNum );
            werase( w_item_info );

            if( iItemNum > 0 && activeItem ) {
                std::vector<iteminfo> vThisItem;
                std::vector<iteminfo> vDummy;
                activeItem->vIG[page_num].it->info( true, vThisItem );

                item_info_data dummy( "", "", vThisItem, vDummy, iScrollPos );
                dummy.without_getch = true;
                dummy.without_border = true;

                draw_item_info( w_item_info, dummy );
            }
            draw_scrollbar( w_items_border, iActive, iMaxRows, iItemNum, point::south );
            wnoutrefresh( w_items_border );
        }

        const bool bDrawLeft = ground_items.empty() || filtered_items.empty() || !activeItem;
        draw_custom_border( w_item_info, bDrawLeft, 1, 1, 1, LINE_XXXO, LINE_XOXX, 1, 1 );

        if( iItemNum > 0 && activeItem ) {
            // print info window title: < item name >
            mvwprintw( w_item_info, point( 2, 0 ), "< " );
            trim_and_print( w_item_info, point( 4, 0 ), width_nob - padding,
                            activeItem->vIG[page_num].it->color_in_inventory(),
                            activeItem->vIG[page_num].it->display_name() );
            wprintw( w_item_info, " >" );
            // move the cursor to the selected item (for screen readers)
            ui.set_cursor( w_items, point( 1, iActive - iStartPos ) );
        }

        wnoutrefresh( w_items );
        wnoutrefresh( w_item_info );
    } );

    std::optional<tripoint_bub_ms> trail_start;
    std::optional<tripoint_bub_ms> trail_end;
    bool trail_end_x = false;
    shared_ptr_fast<draw_callback_t> trail_cb = create_trail_callback( trail_start, trail_end,
            trail_end_x );
    add_draw_callback( trail_cb );

    bool addCategory = uistate.list_item_sort == list_item_sort_mode::CATEGORY_DISTANCE ||
                       uistate.list_item_sort == list_item_sort_mode::CATEGORY_NAME;
    bool refilter = true;

    std::string action;
    input_context ctxt( "LIST_ITEMS" );
    ctxt.register_action( "UP", to_translation( "Move cursor up" ) );
    ctxt.register_action( "DOWN", to_translation( "Move cursor down" ) );
    ctxt.register_action( "LEFT", to_translation( "Previous item" ) );
    ctxt.register_action( "RIGHT", to_translation( "Next item" ) );
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "SCROLL_ITEM_INFO_DOWN" );
    ctxt.register_action( "SCROLL_ITEM_INFO_UP" );
    ctxt.register_action( "zoom_in" );
    ctxt.register_action( "zoom_out" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "EXAMINE" );
    ctxt.register_action( "COMPARE" );
    ctxt.register_action( "PRIORITY_INCREASE" );
    ctxt.register_action( "PRIORITY_DECREASE" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "TRAVEL_TO" );

    do {
        bool recalc_unread = false;
        if( action == "COMPARE" && activeItem ) {
            game_menus::inv::compare( active_pos );
            recalc_unread = highlight_unread_items;
        } else if( action == "FILTER" ) {
            ui.invalidate_ui();
            string_input_popup_imgui imgui_popup( 70, sFilter, _( "Filter items" ) );
            imgui_popup.set_label( _( "Filter:" ) );
            imgui_popup.set_description( item_filter_rule_string( item_filter_type::FILTER ) + "\n\n" +
                                         list_items_filter_history_help(), c_white, true );
            imgui_popup.set_identifier( "item_filter" );
            sFilter = imgui_popup.query();
            refilter = true;
            uistate.list_item_filter_active = !sFilter.empty();
        } else if( action == "RESET_FILTER" ) {
            sFilter.clear();
            filtered_items = ground_items;
            refilter = true;
            uistate.list_item_filter_active = false;
        } else if( action == "EXAMINE" && !filtered_items.empty() && activeItem ) {
            std::vector<iteminfo> vThisItem;
            std::vector<iteminfo> vDummy;
            activeItem->vIG[page_num].it->info( true, vThisItem );

            item_info_data info_data( activeItem->vIG[page_num].it->tname(),
                                      activeItem->vIG[page_num].it->type_name(), vThisItem,
                                      vDummy );
            info_data.handle_scrolling = true;

            draw_item_info( [&]() -> catacurses::window {
                return catacurses::newwin( TERMY, width - 5, point::zero );
            }, info_data );
            recalc_unread = highlight_unread_items;
        } else if( action == "PRIORITY_INCREASE" ) {
            ui.invalidate_ui();
            list_item_upvote = string_input_popup()
                               .title( _( "High Priority:" ) )
                               .width( 55 )
                               .text( list_item_upvote )
                               .description( item_filter_rule_string( item_filter_type::HIGH_PRIORITY ) + "\n\n"
                                             + list_items_filter_history_help() )
                               .desc_color( c_white )
                               .identifier( "list_item_priority" )
                               .max_length( 256 )
                               .query_string();
            refilter = true;
            uistate.list_item_priority_active = !list_item_upvote.empty();
        } else if( action == "PRIORITY_DECREASE" ) {
            ui.invalidate_ui();
            list_item_downvote = string_input_popup()
                                 .title( _( "Low Priority:" ) )
                                 .width( 55 )
                                 .text( list_item_downvote )
                                 .description( item_filter_rule_string( item_filter_type::LOW_PRIORITY ) + "\n\n"
                                               + list_items_filter_history_help() )
                                 .desc_color( c_white )
                                 .identifier( "list_item_downvote" )
                                 .max_length( 256 )
                                 .query_string();
            refilter = true;
            uistate.list_item_downvote_active = !list_item_downvote.empty();
        } else if( action == "SORT" ) {
            switch( uistate.list_item_sort ) {
                case list_item_sort_mode::count:
                case list_item_sort_mode::DISTANCE:
                    uistate.list_item_sort = list_item_sort_mode::NAME;
                    break;
                case list_item_sort_mode::NAME:
                    uistate.list_item_sort = list_item_sort_mode::CATEGORY_DISTANCE;
                    break;
                case list_item_sort_mode::CATEGORY_DISTANCE:
                    uistate.list_item_sort = list_item_sort_mode::CATEGORY_NAME;
                    break;
                case list_item_sort_mode::CATEGORY_NAME:
                    uistate.list_item_sort = list_item_sort_mode::DISTANCE;
                    break;
            }

            highPEnd = -1;
            lowPStart = -1;
            iCatSortNum = 0;

            mSortCategory.clear();
            refilter = true;
        } else if( action == "TRAVEL_TO" && activeItem ) {
            // try finding route to the tile, or one tile away from it
            const std::optional<std::vector<tripoint_bub_ms>> try_route =
            safe_route_to( u, u.pos_bub() + active_pos, 1, []( const std::string & msg ) {
                popup( msg );
            } );
            recalc_unread = highlight_unread_items;
            if( try_route.has_value() ) {
                u.set_destination( *try_route );
                break;
            }
        }

        if( refilter ) {
            switch( uistate.list_item_sort ) {
                case list_item_sort_mode::count:
                case list_item_sort_mode::DISTANCE:
                    ground_items = item_list;
                    break;
                case list_item_sort_mode::NAME:
                    std::sort( ground_items.begin(), ground_items.end(), map_item_stack::map_item_stack_sort_name );
                    break;
                case list_item_sort_mode::CATEGORY_DISTANCE:
                    std::sort( ground_items.begin(), ground_items.end(),
                               map_item_stack::map_item_stack_sort_category_distance );
                    break;
                case list_item_sort_mode::CATEGORY_NAME:
                    std::sort( ground_items.begin(), ground_items.end(),
                               map_item_stack::map_item_stack_sort_category_name );
                    break;
            }

            refilter = false;
            addCategory = uistate.list_item_sort == list_item_sort_mode::CATEGORY_DISTANCE ||
                          uistate.list_item_sort == list_item_sort_mode::CATEGORY_NAME;
            filtered_items = filter_item_stacks( ground_items, sFilter );
            highPEnd = list_filter_high_priority( filtered_items, list_item_upvote );
            lowPStart = list_filter_low_priority( filtered_items, highPEnd, list_item_downvote );
            iActive = 0;
            page_num = 0;
            iItemNum = filtered_items.size();
        }

        if( addCategory ) {
            addCategory = false;
            iCatSortNum = 0;
            mSortCategory.clear();
            if( highPEnd > 0 ) {
                mSortCategory[0] = _( "HIGH PRIORITY" );
                iCatSortNum++;
            }
            std::string last_cat_name;
            for( int i = std::max( 0, highPEnd );
                 i < std::min( lowPStart, static_cast<int>( filtered_items.size() ) ); i++ ) {
                const std::string &cat_name =
                    filtered_items[i].example->get_category_of_contents().name_header();
                if( cat_name != last_cat_name ) {
                    mSortCategory[i + iCatSortNum++] = cat_name;
                    last_cat_name = cat_name;
                }
            }
            if( lowPStart < static_cast<int>( filtered_items.size() ) ) {
                mSortCategory[lowPStart + iCatSortNum++] = _( "LOW PRIORITY" );
            }
            if( !mSortCategory[0].empty() ) {
                iActive++;
            }
            iItemNum = static_cast<int>( filtered_items.size() ) + iCatSortNum;
        }

        const int item_info_scroll_lines = catacurses::getmaxy( w_item_info ) - 4;
        const int scroll_rate = iItemNum > 20 ? 10 : 3;

        if( action == "UP" ) {
            do {
                iActive--;
            } while( !mSortCategory[iActive].empty() );
            iScrollPos = 0;
            page_num = 0;
            if( iActive < 0 ) {
                iActive = iItemNum - 1;
            }
            recalc_unread = highlight_unread_items;
        } else if( action == "DOWN" ) {
            do {
                iActive++;
            } while( !mSortCategory[iActive].empty() );
            iScrollPos = 0;
            page_num = 0;
            if( iActive >= iItemNum ) {
                iActive = mSortCategory[0].empty() ? 0 : 1;
            }
            recalc_unread = highlight_unread_items;
        } else if( action == "PAGE_DOWN" ) {
            iScrollPos = 0;
            page_num = 0;
            if( iActive == iItemNum - 1 ) {
                iActive = mSortCategory[0].empty() ? 0 : 1;
            } else if( iActive + scroll_rate >= iItemNum ) {
                iActive = iItemNum - 1;
            } else {
                iActive += +scroll_rate - 1;
                do {
                    iActive++;
                } while( !mSortCategory[iActive].empty() );
            }
        } else if( action == "PAGE_UP" ) {
            iScrollPos = 0;
            page_num = 0;
            if( mSortCategory[0].empty() ? iActive == 0 : iActive == 1 ) {
                iActive = iItemNum - 1;
            } else if( iActive <= scroll_rate ) {
                iActive = mSortCategory[0].empty() ? 0 : 1;
            } else {
                iActive += -scroll_rate + 1;
                do {
                    iActive--;
                } while( !mSortCategory[iActive].empty() );
            }
        } else if( action == "RIGHT" ) {
            if( !filtered_items.empty() && activeItem ) {
                if( ++page_num >= static_cast<int>( activeItem->vIG.size() ) ) {
                    page_num = activeItem->vIG.size() - 1;
                }
            }
            recalc_unread = highlight_unread_items;
        } else if( action == "LEFT" ) {
            page_num = std::max( 0, page_num - 1 );
            recalc_unread = highlight_unread_items;
        } else if( action == "SCROLL_ITEM_INFO_UP" ) {
            iScrollPos -= item_info_scroll_lines;
            recalc_unread = highlight_unread_items;
        } else if( action == "SCROLL_ITEM_INFO_DOWN" ) {
            iScrollPos += item_info_scroll_lines;
            recalc_unread = highlight_unread_items;
        } else if( action == "zoom_in" ) {
            zoom_in();
            mark_main_ui_adaptor_resize();
        } else if( action == "zoom_out" ) {
            zoom_out();
            mark_main_ui_adaptor_resize();
        } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
            u.view_offset = stored_view_offset;
            return game::vmenu_ret::CHANGE_TAB;
        }

        active_pos = tripoint_rel_ms::zero;
        activeItem = nullptr;

        if( mSortCategory[iActive].empty() ) {
            auto iter = filtered_items.begin();
            for( int iNum = 0; iter != filtered_items.end() && iNum < iActive; iNum++ ) {
                if( mSortCategory[iNum].empty() ) {
                    ++iter;
                }
            }
            if( iter != filtered_items.end() ) {
                active_pos = iter->vIG[page_num].pos;
                activeItem = &( *iter );
            }
        }

        if( activeItem ) {
            centerlistview( active_pos.raw(), width );
            trail_start = u.pos_bub();
            trail_end = u.pos_bub() + active_pos;
            // Actually accessed from the terrain overlay callback `trail_cb` in the
            // call to `ui_manager::redraw`.
            //NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
            trail_end_x = true;
            if( recalc_unread ) {
                mark_items_read_rec( activeItem->example );
            }
        } else {
            u.view_offset = stored_view_offset;
            trail_start = trail_end = std::nullopt;
        }
        invalidate_main_ui_adaptor();

        ui_manager::redraw();

        action = ctxt.handle_input();
    } while( action != "QUIT" );

    u.view_offset = stored_view_offset;
    return game::vmenu_ret::QUIT;
}

game::vmenu_ret game::list_monsters( const std::vector<Creature *> &monster_list )
{
    const map &here = get_map();

    const int iInfoHeight = 15;
    const int width = 55;
    int offsetX = 0;
    int iMaxRows = 0;

    catacurses::window w_monsters;
    catacurses::window w_monsters_border;
    catacurses::window w_monster_info;
    catacurses::window w_monster_info_border;

    Creature *cCurMon = nullptr;
    tripoint iActivePos;

    bool hide_ui = false;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        if( hide_ui ) {
            ui.position( point::zero, point::zero );
        } else {
            offsetX = TERMX - width;
            iMaxRows = TERMY - iInfoHeight - 1;

            w_monsters = catacurses::newwin( iMaxRows, width - 2, point( offsetX + 1,
                                             1 ) );
            w_monsters_border = catacurses::newwin( iMaxRows + 1, width, point( offsetX,
                                                    0 ) );
            w_monster_info = catacurses::newwin( iInfoHeight - 2, width - 2,
                                                 point( offsetX + 1, TERMY - iInfoHeight + 1 ) );
            w_monster_info_border = catacurses::newwin( iInfoHeight, width, point( offsetX,
                                    TERMY - iInfoHeight ) );

            if( cCurMon ) {
                centerlistview( iActivePos, width );
            }

            ui.position( point( offsetX, 0 ), point( width, TERMY ) );
        }
    } );
    ui.mark_resize();

    int max_gun_range = 0;
    if( u.get_wielded_item() ) {
        max_gun_range = u.get_wielded_item()->gun_range( &u );
    }

    const tripoint_rel_ms stored_view_offset = u.view_offset;
    u.view_offset = tripoint_rel_ms::zero;

    int iActive = 0; // monster index that we're looking at

    std::string action;
    input_context ctxt( "LIST_MONSTERS" );
    ctxt.register_navigate_ui_list();
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "zoom_in" );
    ctxt.register_action( "zoom_out" );
    ctxt.register_action( "SAFEMODE_BLACKLIST_ADD" );
    ctxt.register_action( "SAFEMODE_BLACKLIST_REMOVE" );
    ctxt.register_action( "QUIT" );
    if( bVMonsterLookFire ) {
        ctxt.register_action( "look" );
        ctxt.register_action( "fire" );
    }
    ctxt.register_action( "HELP_KEYBINDINGS" );

    // first integer is the row the attitude category string is printed in the menu
    std::map<int, Creature::Attitude> mSortCategory;
    const bool player_knows = !u.has_trait( trait_INATTENTIVE );
    if( player_knows ) {
        for( int i = 0, last_attitude = -1; i < static_cast<int>( monster_list.size() ); i++ ) {
            const Creature::Attitude attitude = monster_list[i]->attitude_to( u );
            if( static_cast<int>( attitude ) != last_attitude ) {
                mSortCategory[i + mSortCategory.size()] = attitude;
                last_attitude = static_cast<int>( attitude );
            }
        }
    }

    ui.on_redraw( [&]( const ui_adaptor & ) {
        if( !hide_ui ) {
            draw_custom_border( w_monsters_border, 1, 1, 1, 1, 1, 1, LINE_XOXO, LINE_XOXO );
            draw_custom_border( w_monster_info_border, 1, 1, 1, 1, LINE_XXXO, LINE_XOXX, 1, 1 );

            mvwprintz( w_monsters_border, point( 2, 0 ), c_light_green, "<Tab> " );
            wprintz( w_monsters_border, c_white, _( "Monsters" ) );

            if( monster_list.empty() ) {
                werase( w_monsters );
                mvwprintz( w_monsters, point( 2, iMaxRows / 3 ), c_white,
                           _( "You don't see any monsters around you!" ) );
            } else {
                werase( w_monsters );

                const int iNumMonster = monster_list.size();
                const int iMenuSize = monster_list.size() + mSortCategory.size();

                const int numw = iNumMonster > 999 ? 4 :
                                 iNumMonster > 99  ? 3 :
                                 iNumMonster > 9   ? 2 : 1;

                // given the currently selected monster iActive. get the selected row
                int iSelPos = iActive;
                for( auto &ia : mSortCategory ) {
                    int index = ia.first;
                    if( index <= iSelPos ) {
                        ++iSelPos;
                    } else {
                        break;
                    }
                }
                int iStartPos = 0;
                // use selected row get the start row
                calcStartPos( iStartPos, iSelPos, iMaxRows - 1, iMenuSize );

                // get first visible monster and category
                int iCurMon = iStartPos;
                auto CatSortIter = mSortCategory.cbegin();
                while( CatSortIter != mSortCategory.cend() && CatSortIter->first < iStartPos ) {
                    ++CatSortIter;
                    --iCurMon;
                }

                std::string monNameSelected;
                std::string sSafemode;
                const int endY = std::min<int>( iMaxRows - 1, iMenuSize );
                for( int y = 0; y < endY; ++y ) {

                    if( player_knows && CatSortIter != mSortCategory.cend() ) {
                        const int iCurPos = iStartPos + y;
                        const int iCatPos = CatSortIter->first;
                        if( iCurPos == iCatPos ) {
                            const std::string cat_name = Creature::get_attitude_ui_data(
                                                             CatSortIter->second ).first.translated();
                            mvwprintz( w_monsters, point( 1, y ), c_magenta, cat_name );
                            ++CatSortIter;
                            continue;
                        }
                    }

                    // select current monster
                    Creature *critter = monster_list[iCurMon];
                    const bool selected = iCurMon == iActive;
                    ++iCurMon;
                    if( critter->sees( here, u ) && player_knows ) {
                        mvwprintz( w_monsters, point( 0, y ), c_yellow, "!" );
                    }
                    bool is_npc = false;
                    const monster *m = dynamic_cast<monster *>( critter );
                    const npc     *p = dynamic_cast<npc *>( critter );
                    nc_color name_color = critter->basic_symbol_color();

                    if( selected ) {
                        name_color = hilite( name_color );
                    }

                    if( m != nullptr ) {
                        trim_and_print( w_monsters, point( 1, y ), width - 32, name_color, m->name() );
                    } else {
                        trim_and_print( w_monsters, point( 1, y ), width - 32, name_color, critter->disp_name() );
                        is_npc = true;
                    }

                    if( selected && !get_safemode().empty() ) {
                        monNameSelected = is_npc ? get_safemode().npc_type_name() : m->name();

                        if( get_safemode().has_rule( monNameSelected, Creature::Attitude::ANY ) ) {
                            sSafemode = _( "<R>emove from safe mode blacklist" );
                        } else {
                            sSafemode = _( "<A>dd to safe mode blacklist" );
                        }
                    }

                    nc_color color = c_white;
                    std::string sText;

                    if( m != nullptr ) {
                        m->get_HP_Bar( color, sText );
                    } else {
                        std::tie( sText, color ) =
                            ::get_hp_bar( critter->get_hp(), critter->get_hp_max(), false );
                    }
                    mvwprintz( w_monsters, point( width - 31, y ), color, sText );
                    const int bar_max_width = 5;
                    const int bar_width = utf8_width( sText );
                    wattron( w_monsters, c_white );
                    for( int i = 0; i < bar_max_width - bar_width; ++i ) {
                        mvwprintw( w_monsters, point( width - 27 - i, y ), "." );
                    }
                    wattron( w_monsters, c_white );

                    if( m != nullptr ) {
                        const auto att = m->get_attitude();
                        sText = att.first;
                        color = att.second;
                    } else if( p != nullptr ) {
                        sText = npc_attitude_name( p->get_attitude() );
                        color = p->symbol_color();
                    }
                    if( !player_knows ) {
                        sText = _( "Unknown" );
                        color = c_yellow;
                    }
                    mvwprintz( w_monsters, point( width - 19, y ), color, sText );

                    const int mon_dist = rl_dist( u.pos_bub(), critter->pos_bub() );
                    const int numd = mon_dist > 999 ? 4 :
                                     mon_dist > 99 ? 3 :
                                     mon_dist > 9 ? 2 : 1;

                    trim_and_print( w_monsters, point( width - ( 5 + numd ), y ), 6 + numd,
                                    selected ? c_light_green : c_light_gray,
                                    "%*d %s",
                                    numd, mon_dist,
                                    direction_name_short( direction_from( u.pos_bub(), critter->pos_bub() ) ) );
                }

                mvwprintz( w_monsters_border, point( ( width / 2 ) - numw - 2, 0 ), c_light_green, " %*d", numw,
                           iActive + 1 );
                wprintz( w_monsters_border, c_white, " / %*d ", numw, static_cast<int>( monster_list.size() ) );

                werase( w_monster_info );
                if( cCurMon ) {
                    cCurMon->print_info( w_monster_info, 1, iInfoHeight - 3, 1 );
                }

                draw_custom_border( w_monster_info_border, 1, 1, 1, 1, LINE_XXXO, LINE_XOXX, 1, 1 );

                if( bVMonsterLookFire ) {
                    mvwprintw( w_monster_info_border, point::east, "< " );
                    wprintz( w_monster_info_border, c_light_green, ctxt.press_x( "look" ) );
                    wprintz( w_monster_info_border, c_light_gray, " %s", _( "to look around" ) );

                    if( cCurMon && rl_dist( u.pos_bub(), cCurMon->pos_bub() ) <= max_gun_range ) {
                        std::string press_to_fire_text = string_format( _( "%s %s" ),
                                                         ctxt.press_x( "fire" ),
                                                         string_format( _( "<color_light_gray>to shoot</color>" ) ) );
                        right_print( w_monster_info_border, 0, 3, c_light_green, press_to_fire_text );
                    }
                    wprintw( w_monster_info_border, " >" );
                }

                if( !get_safemode().empty() ) {
                    if( get_safemode().has_rule( monNameSelected, Creature::Attitude::ANY ) ) {
                        sSafemode = _( "<R>emove from safe mode blacklist" );
                    } else {
                        sSafemode = _( "<A>dd to safe mode blacklist" );
                    }

                    shortcut_print( w_monsters, point( 2, getmaxy( w_monsters ) - 1 ),
                                    c_white, c_light_green, sSafemode );
                }

                draw_scrollbar( w_monsters_border, iActive, iMaxRows, static_cast<int>( monster_list.size() ),
                                point::south );
            }

            wnoutrefresh( w_monsters_border );
            wnoutrefresh( w_monster_info_border );
            wnoutrefresh( w_monsters );
            wnoutrefresh( w_monster_info );
        }
    } );

    std::optional<tripoint_bub_ms> trail_start;
    std::optional<tripoint_bub_ms> trail_end;
    bool trail_end_x = false;
    shared_ptr_fast<draw_callback_t> trail_cb = create_trail_callback( trail_start, trail_end,
            trail_end_x );
    add_draw_callback( trail_cb );
    const int recmax = static_cast<int>( monster_list.size() );
    const int scroll_rate = recmax > 20 ? 10 : 3;

    do {
        if( navigate_ui_list( action, iActive, scroll_rate, recmax, true ) ) {
        } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
            u.view_offset = stored_view_offset;
            return game::vmenu_ret::CHANGE_TAB;
        } else if( action == "zoom_in" ) {
            zoom_in();
            mark_main_ui_adaptor_resize();
        } else if( action == "zoom_out" ) {
            zoom_out();
            mark_main_ui_adaptor_resize();
        } else if( action == "SAFEMODE_BLACKLIST_REMOVE" ) {
            const monster *m = dynamic_cast<monster *>( cCurMon );
            const std::string monName = ( m != nullptr ) ? m->name() : "human";

            if( get_safemode().has_rule( monName, Creature::Attitude::ANY ) ) {
                get_safemode().remove_rule( monName, Creature::Attitude::ANY );
            }
        } else if( action == "SAFEMODE_BLACKLIST_ADD" ) {
            if( !get_safemode().empty() ) {
                const monster *m = dynamic_cast<monster *>( cCurMon );
                const std::string monName = ( m != nullptr ) ? m->name() : "human";

                get_safemode().add_rule( monName, Creature::Attitude::ANY, get_option<int>( "SAFEMODEPROXIMITY" ),
                                         rule_state::BLACKLISTED );
            }
        } else if( action == "look" ) {
            hide_ui = true;
            ui.mark_resize();
            look_around();
            hide_ui = false;
            ui.mark_resize();
        } else if( action == "fire" ) {
            if( cCurMon != nullptr && rl_dist( u.pos_bub(), cCurMon->pos_bub() ) <= max_gun_range ) {
                u.last_target = shared_from( *cCurMon );
                u.recoil = MAX_RECOIL;
                u.view_offset = stored_view_offset;
                return game::vmenu_ret::FIRE;
            }
        }

        if( iActive >= 0 && static_cast<size_t>( iActive ) < monster_list.size() ) {
            cCurMon = monster_list[iActive];
            iActivePos = ( cCurMon->pos_bub() - u.pos_bub() ).raw();
            centerlistview( iActivePos, width );
            trail_start = u.pos_bub();
            trail_end = cCurMon->pos_bub();
            // Actually accessed from the terrain overlay callback `trail_cb` in the
            // call to `ui_manager::redraw`.
            //NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
            trail_end_x = false;
        } else {
            cCurMon = nullptr;
            iActivePos = tripoint::zero;
            u.view_offset = stored_view_offset;
            trail_start = trail_end = std::nullopt;
        }
        invalidate_main_ui_adaptor();

        ui_manager::redraw();

        action = ctxt.handle_input();
    } while( action != "QUIT" );

    u.view_offset = stored_view_offset;

    return game::vmenu_ret::QUIT;
}
