#include "game.h"
#include "game_inventory.h"
#include "player.h"
#include "catacharset.h" // used for utf8_width()
#include "input.h"
#include "output.h"
#include "item.h"
#include "iteminfo_query.h"
#include "string_formatter.h"
#include "units.h"
#include "translations.h"
#include "npc.h"
#include "cata_utility.h"
#include "line.h"
#include "clothing_layer.h"

#include <vector>
#include <string>

namespace
{

	// 'condensed' version of @ref item.name() without charges, contained items, etc... 
	// just damage state icon + type name
	static std::string itemLabel( const item *item, const int maxWidth = -1 ) {
		std::stringstream tmp;
		
		tmp << get_tag_from_color( item->damage_color() ) << item->damage_symbol() << "</color> ";

		std::string tname = item->type_name( 1 );

		if (maxWidth != -1 && utf8_width( tname ) > maxWidth - 3) {			// symbol (2 chars) + ' '
			tmp << utf8_truncate( tname, maxWidth - 5 ) << "\u2026 ";		// U+2026 = &hellipses; 
		}
		else {
			tmp << tname;
		}

		return tmp.str();
	}

void draw_mid_pane( const catacurses::window &w_sort_middle, item const &worn_item, int &scrollPos )
{

    std::vector<iteminfo> infos, dummy;

    static const iteminfo_query parts = iteminfo_query( std::vector<iteminfo_parts> {
        // skip the body parts, they're highlighted below anyway
        iteminfo_parts::ARMOR_LAYER,
        iteminfo_parts::ARMOR_COVERAGE,
        iteminfo_parts::ARMOR_WARMTH,
        iteminfo_parts::ARMOR_ENCUMBRANCE,
        iteminfo_parts::ARMOR_STORAGE,
        iteminfo_parts::ARMOR_PROTECTION,
        iteminfo_parts::DESCRIPTION_FLAGS,
        iteminfo_parts::DESCRIPTION_FLAGS_FITS,
    } );

    static std::string empty_name;

    worn_item.info( infos, &parts );  

    draw_item_info( w_sort_middle, empty_name, itemLabel( &worn_item ),
                    infos, dummy, scrollPos, true, true, false, false, true, 0 );
}

} //namespace

static std::vector<const item*> items_cover_bp( const Character &c, int bp )
{
    std::vector<const item*> s;
    for( auto &elem : c.worn ) {
        if( elem.covers( static_cast<body_part>( bp ) ) ) {			
			s.push_back( &elem );
        }
    }
    return s;
}


void draw_grid( const catacurses::window &w, int left_pane_w, int mid_pane_w )
{
    const int win_w = getmaxx( w );
    const int win_h = getmaxy( w );

    draw_border( w );
    mvwhline( w, 2, 1, 0, win_w - 2 );
    mvwvline( w, 3, left_pane_w + 1, 0, win_h - 4 );
    mvwvline( w, 3, left_pane_w + mid_pane_w + 2, 0, win_h - 4 );

    // intersections
    mvwputch( w, 2, 0, BORDER_COLOR, LINE_XXXO );
    mvwputch( w, 2, win_w - 1, BORDER_COLOR, LINE_XOXX );
    mvwputch( w, 2, left_pane_w + 1, BORDER_COLOR, LINE_OXXX );
    mvwputch( w, win_h - 1, left_pane_w + 1, BORDER_COLOR, LINE_XXOX );
    mvwputch( w, 2, left_pane_w + mid_pane_w + 2, BORDER_COLOR, LINE_OXXX );
    mvwputch( w, win_h - 1, left_pane_w + mid_pane_w + 2, BORDER_COLOR, LINE_XXOX );

    wrefresh( w );
}

void player::sort_armor()
{
    /* Define required height of the right pane:
    * + 3 - horizontal lines;
    * + 1 - caption line;
    * + 2 - innermost/outermost string lines;
    * + 12 - sub-categories (torso, head, eyes, etc.);
    * + 1 - gap;
    * number of lines required for displaying all items is calculated dynamically,
    * because some items can have multiple entries (i.e. cover a few parts of body).
    */

    int req_right_h = 3 + 1 + 2 + 12 + 1;
    for( const body_part cover : all_body_parts ) {
        for( const item &elem : worn ) {
            if( elem.covers( cover ) ) {
                req_right_h++;
            }
        }
    }

    /* Define required height of the mid pane:
    * + 3 - horizontal lines;
    * + 1 - caption line;
    * + 8 - general properties
    * + 7 - ASSUMPTION: max possible number of flags @ item
    * + 13 - warmth & enc block
    */
    const int req_mid_h = 3 + 1 + 8 + 7 + 13;

    const int win_h = std::min( TERMY, std::max( FULL_SCREEN_HEIGHT,
                                std::max( req_right_h, req_mid_h ) ) );
    const int win_w = FULL_SCREEN_WIDTH + ( TERMX - FULL_SCREEN_WIDTH ) * 3 / 4;
    const int win_x = TERMX / 2 - win_w / 2;
    const int win_y = TERMY / 2 - win_h / 2;

    int cont_h   = win_h - 4;
    int left_w   = ( win_w - 4 ) / 3;
    int right_w  = left_w;
    int middle_w = ( win_w - 4 ) - left_w - right_w;

    int tabindex = num_bp;
    const int tabcount = num_bp + 1;

    int leftListSize;
    int leftListIndex  = 0;
    int leftListOffset = 0;
    int selected       = -1;

    int rightListSize;
    int rightListOffset = 0;

	int iteminfoScrollPos = 0;

    std::vector<item *> tmp_worn;
    std::array<std::string, 13> armor_cat = {{
            _( "Torso" ), _( "Head" ), _( "Eyes" ), _( "Mouth" ), _( "L. Arm" ), _( "R. Arm" ),
            _( "L. Hand" ), _( "R. Hand" ), _( "L. Leg" ), _( "R. Leg" ), _( "L. Foot" ),
            _( "R. Foot" ), _( "All" )
        }
    };

	static std::array<nc_color, 4> armorHighlightingScheme = {
        h_cyan, h_light_gray, c_cyan, c_light_gray
	};

    // Layout window
    catacurses::window w_sort_armor = catacurses::newwin( win_h, win_w, win_y, win_x );
    draw_grid( w_sort_armor, left_w, middle_w );
    // Subwindows (between lines)
    catacurses::window w_sort_cat = catacurses::newwin( 1, win_w - 4, win_y + 1, win_x + 2 );
    catacurses::window w_sort_left = catacurses::newwin( cont_h, left_w,   win_y + 3, win_x + 1 );
    catacurses::window w_sort_middle = catacurses::newwin( cont_h - num_bp - 1, middle_w, win_y + 3,
                                       win_x + left_w + 2 );
    catacurses::window w_sort_right = catacurses::newwin( cont_h, right_w,  win_y + 3,
                                      win_x + left_w + middle_w + 3 );
    catacurses::window w_encumb = catacurses::newwin( num_bp + 1, middle_w,
                                  win_y + 3 + cont_h - num_bp - 1, win_x + left_w + 2 );

    input_context ctxt( "SORT_ARMOR" );
    ctxt.register_cardinal();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "MOVE_ARMOR" );
    ctxt.register_action( "CHANGE_SIDE" );
    ctxt.register_action( "ASSIGN_INVLETS" );
    ctxt.register_action( "EQUIP_ARMOR" );
    ctxt.register_action( "REMOVE_ARMOR" );
    ctxt.register_action( "USAGE_HELP" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
	ctxt.register_action( "PAGE_UP" );
	ctxt.register_action( "PAGE_DOWN" );

    bool exit = false;
    while( !exit ) {
        if( is_player() ) {
            // Totally hoisted this from advanced_inv
            if( g->u.moves < 0 ) {
                g->u.assign_activity( activity_id( "ACT_ARMOR_LAYERS" ), 0 );
                g->u.activity.auto_resume = true;
                g->u.activity.moves_left = INT_MAX;
                return;
            }
        } else {
            // Player is sorting NPC's armor here
            if( rl_dist( g->u.pos(), pos() ) > 1 ) {
                add_msg_if_npc( m_bad, _( "%s is too far to sort armor." ), name.c_str() );
                return;
            }
            if( attitude_to( g->u ) != Creature::A_FRIENDLY ) {
                add_msg_if_npc( m_bad, _( "%s is not friendly!" ), name.c_str() );
                return;
            }
        }
        werase( w_sort_cat );
        werase( w_sort_left );
        werase( w_sort_middle );
        werase( w_sort_right );
        werase( w_encumb );

        // top bar
        wprintz( w_sort_cat, c_white, _( "Sort Armor" ) );
		wprintz( w_sort_cat, c_yellow, "  << " );
		wprintz( w_sort_cat, c_magenta, armor_cat[tabindex].c_str());		// see @ref replace_colors for "<header>"
		wprintz( w_sort_cat, c_yellow, " >>" );

        right_print( w_sort_cat, 0, 0, c_white, string_format(
                         _( "Press %s for help. Press %s to change keybindings." ),
                         ctxt.get_desc( "USAGE_HELP" ).c_str(),
                         ctxt.get_desc( "HELP_KEYBINDINGS" ).c_str() ) );

        // Create ptr list of items to display
        tmp_worn.clear();
        if( tabindex == 12 ) { // All
            for( auto &elem : worn ) {
                tmp_worn.push_back( &elem );
            }
        } else { // bp_*
            for( auto &elem : worn ) {
                if( elem.covers( static_cast<body_part>( tabindex ) ) ) {
                    tmp_worn.push_back( &elem );
                }
            }
        }
        leftListSize = ( ( int )tmp_worn.size() < cont_h - 2 ) ? ( int )tmp_worn.size() : cont_h - 2;

        // Left header
        mvwprintz( w_sort_left, 0, 0, c_light_gray, _( "(Innermost)" ) );
        right_print( w_sort_left, 0, 0, c_light_gray, string_format( _( "Storage (%s)" ),
                     volume_units_abbr() ) );


        // Left list
        for( int drawindex = 0; drawindex < leftListSize; drawindex++ ) {
			int itemindex = leftListOffset + drawindex;

            clothing_layer layer = clothing_layer::get( tmp_worn[itemindex]->layer() );
            nc_color color = layer.color();

			// layer symbol
			print_colored_text( w_sort_left, drawindex + 1, 0, color, color, std::string( 1, layer.symbol() ) );

			// determine line color/highlight
            color = c_light_gray;
            if( itemindex == leftListIndex ) {
                color = itemindex == selected
                     ? h_red
                     : h_white;
            }

			std::stringstream tmp;

			const int offset_x = 2;
			int availableWidth = left_w - offset_x - 5;
			if (itemindex == selected) {
				tmp << " ";     // 'fake' offset the selected item - keeps the highlighted background bar same size
				availableWidth--;
			}

            tmp << itemLabel( tmp_worn[itemindex], availableWidth );

            std::string text = tmp.str();

            std::string volume = format_volume( tmp_worn[itemindex]->get_storage() );

            int width = utf8_width( text, true )
                        + utf8_width( volume );

            // pad line interior with spaces to highlight entire line (if applicable)
            tmp << std::string( left_w - offset_x - width, ' ' ) << "<neutral>"
                << volume << "</neutral>";

            text = replace_colors( tmp.str() );

            // highlighted lines have no colorized icons/volume
            if( itemindex == leftListIndex ) {
                // @todo: first we add the tags, then we strip them.... this could be done better...
                text = remove_color_tags( text );
            }

            // [highlighted] line
            print_colored_text( w_sort_left, drawindex + 1, offset_x, color, color, text );

        }

        // Left footer
        mvwprintz( w_sort_left, cont_h - 1, 0, c_light_gray, _( "(Outermost)" ) );
        if( leftListSize > ( int )tmp_worn.size() ) {
            // @todo: replace it by right_print()
            mvwprintz( w_sort_left, cont_h - 1, left_w - utf8_width( _( "<more>" ) ),
                       c_light_blue, _( "<more>" ) );
        }
        if( leftListSize == 0 ) {
            // @todo: replace it by right_print()
            mvwprintz( w_sort_left, cont_h - 1, left_w - utf8_width( _( "<empty>" ) ),
                       c_light_blue, _( "<empty>" ) );
        }

        // Items stats
        if( leftListSize > 0 ) {
            draw_mid_pane( w_sort_middle, *tmp_worn[leftListIndex], iteminfoScrollPos );
        } else {
            fold_and_print( w_sort_middle, 0, 1, middle_w - 1, c_white,
                            _( "Nothing to see here!" ) );
        }

		item *pSelectedItem = ( leftListSize > 0 ) ? tmp_worn[leftListIndex] : nullptr;

        mvwprintz( w_encumb, 0, 1, c_white, _( "Encumbrance and Warmth" ) );
        print_encumbrance( w_encumb, -1, pSelectedItem, &armorHighlightingScheme );

        // Right header
        mvwprintz( w_sort_right, 0, 0, c_light_gray, _( "(Innermost)" ) );
        right_print( w_sort_right, 0, 0, c_light_gray, _( "Encumbrance" ) );



        // Right list
        rightListSize = 0;
        for( int cover = 0, pos = 1; cover < num_bp; cover++ ) {
            bool combined = false;

			auto items = items_cover_bp( *this, cover );

            if( cover > 3 && cover % 2 == 0 &&
				items == items_cover_bp( *this, cover + 1 ) ) {
                combined = true;
            }

            if( rightListSize >= rightListOffset && pos <= cont_h - 2 ) {
                auto header = body_part_name_as_heading( all_body_parts[cover], combined ? 2 : 1 );
                int width = utf8_width( header );                
                
                mvwprintz( w_sort_right, pos, 1, ( cover == tabindex ? c_white : c_magenta ), 
						   header );
                
                std::stringstream tmp;

                // detailed extra encumbrance per layer
                int maxWidth = right_w - 2 - width;
                int extraWidth = 0;

                for (int i = 0; i < static_cast<int>( layer_level::MAX_CLOTHING_LAYER ); i++) {
                    layer_level level = static_cast<layer_level>( i );

                    int extraEnc = this->extraEncumbrance( level, cover );

                    if (extraEnc != 0) {
                        clothing_layer layer = clothing_layer::get( level );
                        if (tmp.str().length() != 0) {
                            tmp << " ";
                        }

						tmp << get_tag_from_color( layer.color() ) << layer.symbol() << "</color>:";

						// highlight malus for the *selected* item's layer
						if (pSelectedItem != nullptr 
							&& pSelectedItem->layer() == level 
							&& std::find( items.begin(), items.end(), pSelectedItem) != items.end() ) {
							tmp << get_tag_from_color( c_white )
								<< string_format( "%d", extraEnc )
								<< "</color>";
						}
						else {
							tmp << string_format( "%d", extraEnc );
						}
                            
                    }
                }

                std::string extra = tmp.str();
                size_t len = utf8_width(remove_color_tags( extra ));
                if (len != 0) {
                    if (len < maxWidth)
                        extra = std::string( maxWidth - len, ' ' ) + extra;

                    trim_and_print( w_sort_right, pos, width + 1, maxWidth, c_light_gray, extra );
                }

                pos++;
            }
            rightListSize++;
            for( auto &elem : items) {
                if( rightListSize >= rightListOffset && pos <= cont_h - 2 ) {
                    clothing_layer layer = clothing_layer::get(elem->layer());

                    nc_color color = layer.color();

                    print_colored_text( w_sort_right, pos, 1, color, color, 
										std::string( 1, layer.symbol() ) );

                    trim_and_print( w_sort_right, pos, 3, right_w - 4, 
									pSelectedItem != nullptr && pSelectedItem == elem 
										? c_white : c_light_gray,
                                    itemLabel(elem, right_w - 4 - 2));
                    mvwprintz( w_sort_right, pos, right_w - 3, c_light_gray, "%3d",
                               elem->get_encumber() );
                    pos++;
                }
                rightListSize++;
            }
            if( combined ) {
                cover++;
            }
        }

        // Right footer
        mvwprintz( w_sort_right, cont_h - 1, 0, c_light_gray, _( "(Outermost)" ) );
        if( rightListSize > cont_h - 2 ) {
            // @todo: replace it by right_print()
            mvwprintz( w_sort_right, cont_h - 1, right_w - utf8_width( _( "<more>" ) ), c_light_blue,
                       _( "<more>" ) );
        }
        // F5
        wrefresh( w_sort_cat );
        wrefresh( w_sort_left );
        wrefresh( w_sort_middle );
        wrefresh( w_sort_right );
        wrefresh( w_encumb );

        const std::string action = ctxt.handle_input();
        if( is_npc() && action == "ASSIGN_INVLETS" ) {
            // It doesn't make sense to assign invlets to NPC items
            continue;
        }

        if( action == "UP" && leftListSize > 0 ) {
			iteminfoScrollPos = 0;
            leftListIndex--;
            if( leftListIndex < 0 ) {
                leftListIndex = tmp_worn.size() - 1;
            }

            // Scrolling logic
            leftListOffset = ( leftListIndex < leftListOffset ) ? leftListIndex : leftListOffset;
            if( !( ( leftListIndex >= leftListOffset ) &&
                   ( leftListIndex < leftListOffset + leftListSize ) ) ) {
                leftListOffset = leftListIndex - leftListSize + 1;
                leftListOffset = ( leftListOffset > 0 ) ? leftListOffset : 0;
            }

            // move selected item
            if( selected >= 0 ) {
                if( leftListIndex < selected ) {
                    std::swap( *tmp_worn[leftListIndex], *tmp_worn[selected] );
                } else {
                    for( std::size_t i = 0; i < tmp_worn.size() - 1; i++ ) {
                        std::swap( *tmp_worn[i + 1], *tmp_worn[i] );
                    }
                }

                selected = leftListIndex;
            }
        } else if( action == "DOWN" && leftListSize > 0 ) {
			iteminfoScrollPos = 0;
			leftListIndex = ( leftListIndex + 1 ) % tmp_worn.size();

            // Scrolling logic
            if( !( ( leftListIndex >= leftListOffset ) &&
                   ( leftListIndex < leftListOffset + leftListSize ) ) ) {
                leftListOffset = leftListIndex - leftListSize + 1;
                leftListOffset = ( leftListOffset > 0 ) ? leftListOffset : 0;
            }

            // move selected item
            if( selected >= 0 ) {
                if( leftListIndex > selected ) {
                    std::swap( *tmp_worn[leftListIndex], *tmp_worn[selected] );
                } else {
                    for( std::size_t i = tmp_worn.size() - 1; i > 0; i-- ) {
                        std::swap( *tmp_worn[i - 1], *tmp_worn[i] );
                    }
                }

                selected = leftListIndex;
            }
        } else if( action == "LEFT" ) {
			iteminfoScrollPos = 0;
            tabindex--;
            if( tabindex < 0 ) {
                tabindex = tabcount - 1;
            }
            leftListIndex = leftListOffset = 0;
            selected = -1;
        } else if( action == "RIGHT" ) {
			iteminfoScrollPos = 0;
            tabindex = ( tabindex + 1 ) % tabcount;
            leftListIndex = leftListOffset = 0;
            selected = -1;
        } else if( action == "NEXT_TAB" ) {
            rightListOffset++;
            if( rightListOffset + cont_h - 2 > rightListSize ) {
                rightListOffset = rightListSize - cont_h + 2;
            }
        } else if( action == "PREV_TAB" ) {
            rightListOffset--;
            if( rightListOffset < 0 ) {
                rightListOffset = 0;
            }
        } else if( action == "MOVE_ARMOR" ) {
            if( selected >= 0 ) {
                selected = -1;
            } else {
                selected = leftListIndex;
            }
        } else if( action == "CHANGE_SIDE" ) {
            if( leftListIndex < ( int ) tmp_worn.size() && tmp_worn[leftListIndex]->is_sided() ) {
                if( g->u.query_yn( _( "Swap side for %s?" ), tmp_worn[leftListIndex]->tname().c_str() ) ) {
                    change_side( *tmp_worn[leftListIndex] );
                    wrefresh( w_sort_armor );
                }
            }
        } else if( action == "EQUIP_ARMOR" ) {
            // filter inventory for all items that are armor/clothing
            item_location loc = game_menus::inv::wear( *this );

            // only equip if something valid selected!
            if( loc ) {
                // wear the item
                if( wear( this->i_at( loc.obtain( *this ) ) ) ) {
                    // reorder `worn` vector to place new item at cursor
                    auto iter = worn.end();
                    item new_equip  = *( --iter );
                    // remove the item
                    worn.erase( iter );
                    iter = worn.begin();
                    // advance the iterator to cursor's position
                    std::advance( iter, leftListIndex );
                    // inserts at position before iter (no b0f, phew)
                    worn.insert( iter, new_equip );
                } else if( is_npc() ) {
                    // @todo: Pass the reason here
                    popup( _( "Can't put this on!" ) );
                }
            }
            draw_grid( w_sort_armor, left_w, middle_w );
        } else if (action == "REMOVE_ARMOR") {
            // query (for now)
            if (leftListIndex < (int)tmp_worn.size()) {
                if (g->u.query_yn( _( "Remove selected armor?" ) )) {
                    // remove the item, asking to drop it if necessary
                    takeoff( *tmp_worn[leftListIndex] );
                    wrefresh( w_sort_armor );
                    // prevent out of bounds in subsequent tmp_worn[leftListIndex]
                    int new_index_upper_bound = std::max( 0, ( (int)tmp_worn.size() ) - 2 );
                    leftListIndex = std::min( leftListIndex, new_index_upper_bound );
                    selected = -1;
                }
            }
        } else if (action == "ASSIGN_INVLETS") {
			// prompt first before doing this (yes, yes, more popups...)
			if (query_yn( _( "Reassign invlets for armor?" ) )) {
				// Start with last armor (the most unimportant one?)
				auto iiter = inv_chars.rbegin();
				auto witer = worn.rbegin();
				while (witer != worn.rend() && iiter != inv_chars.rend()) {
					const char invlet = *iiter;
					item &w = *witer;
					if (invlet == w.invlet) {
						++witer;
					}
					else if (invlet_to_position( invlet ) != INT_MIN) {
						++iiter;
					}
					else {
						inv.reassign_item( w, invlet );
						++witer;
						++iiter;
					}
				}
			}
		} else if( action == "PAGE_UP" ) {
			iteminfoScrollPos = std::max( 0, iteminfoScrollPos - 1 );
		} else if (action == "PAGE_DOWN") {
			iteminfoScrollPos++;
		} else if( action == "USAGE_HELP" ) {
            popup_getkey( _( "\
Use the arrow- or keypad keys to navigate the left list.\n\
Press [%s] to select highlighted armor for reordering.\n\
Use   [%s] / [%s] to scroll the right list.\n\
Use   [%s] / [%s] to scroll the selected item details.\n\
Press [%s] to assign special inventory letters to clothing.\n\
Press [%s] to change the side on which item is worn.\n\
Use   [%s] to equip an armor item from the inventory.\n\
Press [%s] to remove selected armor from oneself.\n\
 \n\
[Encumbrance and Warmth] explanation:\n\
The first number is the summed encumbrance from all clothing on that bodypart.\n\
The second number is an additional encumbrance penalty caused by wearing multiple items on one of the bodypart's layers.\n\
The sum of these values is the effective encumbrance value your character has for that bodypart." ),
                          ctxt.get_desc( "MOVE_ARMOR" ).c_str(),
                          ctxt.get_desc( "PREV_TAB" ).c_str(),
                          ctxt.get_desc( "NEXT_TAB" ).c_str(),
						  ctxt.get_desc( "PAGE_UP" ).c_str(),
						  ctxt.get_desc( "PAGE_DOWN" ).c_str(),
                          ctxt.get_desc( "ASSIGN_INVLETS" ).c_str(),
                          ctxt.get_desc( "CHANGE_SIDE" ).c_str(),
                          ctxt.get_desc( "EQUIP_ARMOR" ).c_str(),
                          ctxt.get_desc( "REMOVE_ARMOR" ).c_str()
                        );
            draw_grid( w_sort_armor, left_w, middle_w );
        } else if( action == "HELP_KEYBINDINGS" ) {
            draw_grid( w_sort_armor, left_w, middle_w );
        } else if( action == "QUIT" ) {
            exit = true;
        }
    }
}
