#include "action.h"
#include "output.h"
#include "options.h"
#include "path_info.h"
#include "file_wrapper.h"
#include "debug.h"
#include "game.h"
#include "options.h"
#include "messages.h"
#include "inventory.h"
#include <istream>
#include <sstream>
#include <fstream>
#include <iterator>

std::unordered_map< item_action_id, item_action > item_actions;

item_action_map map_actions_to_items( player &p )
{
    // Item index (in inventory) and pointers not to look them up every time
    item_action_map candidates;
    inventory inv = p.inv;
    inv += p.weapon;
    inv += p.worn;
    std::vector< item* > items;
    p.inv.dump( items );

    // Goes through all items and all uses - probably too slow for crafting_inventory
    for( item *i : items ) {
        for( auto ia : item_actions ) {
            auto use = ia.second;
            it_tool *tool = dynamic_cast<it_tool*>(i->type);
            // Can't just test for charges_per_use > charges, because charges can be -1
            if( !i->type->can_use( use.id )  || 
                (tool != nullptr && tool->charges_per_use > 0 && tool->charges_per_use > i->charges) ) {
                continue;
            }

            // Add to usable items only if it needs less charges per use
            auto found = candidates.find( use.id );
            int would_use_charges = tool == nullptr ? 0 : tool->charges_per_use;
            if( found == candidates.end() ) {
                candidates[use.id] = i;
            } else {
                it_tool *other = dynamic_cast<it_tool*>(found->second->type);
                if ( other != nullptr && would_use_charges < other->charges_per_use ) {
                    candidates[use.id] = i;
                }
            }
        }
    }

    return candidates;
}

void game::item_action_menu()
{
    WINDOW *w_item_actions = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                    (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                                    (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);

    size_t selection = 0;
    int invpos = INT_MIN;
    item *selected_item = nullptr;
    input_context ctxt("ITEM_ACTIONS");
    ctxt.register_action("UP");
    ctxt.register_action("DOWN");
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("HELP_KEYBINDINGS");
    item_action_map iactions = map_actions_to_items( u );
    while (true) {
        werase(w_item_actions);

        for (int i = 1; i < FULL_SCREEN_WIDTH - 1; i++) {
            mvwputch(w_item_actions, 2, i, BORDER_COLOR, LINE_OXOX);
            mvwputch(w_item_actions, FULL_SCREEN_HEIGHT - 1, i, BORDER_COLOR, LINE_OXOX);

            if (i > 2 && i < FULL_SCREEN_HEIGHT - 1) {
                mvwputch(w_item_actions, i, 0, BORDER_COLOR, LINE_XOXO);
                //mvwputch(w_item_actions, i, 30, BORDER_COLOR, LINE_XOXO);
                mvwputch(w_item_actions, i, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_XOXO);
            }
        }

        mvwputch(w_item_actions, 2, 0, BORDER_COLOR, LINE_OXXO); // |^
        mvwputch(w_item_actions, 2, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_OOXX); // ^|

        mvwputch(w_item_actions, FULL_SCREEN_HEIGHT - 1, 0, BORDER_COLOR, LINE_XXOO); // |
        mvwputch(w_item_actions, FULL_SCREEN_HEIGHT - 1, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_XOOX); // _|

        //mvwputch(w_item_actions, 2, 30, BORDER_COLOR, (tab == 1) ? LINE_XOXX : LINE_XXXX); // + || -|
        //mvwputch(w_item_actions, FULL_SCREEN_HEIGHT - 1, 30, BORDER_COLOR, LINE_XXOX); // _|_

        size_t i = 0;
        for( auto p : iactions ) {
            it_tool *tool = dynamic_cast<it_tool*>( p.second->type );
            int would_use_charges = tool == nullptr ? 0 : tool->charges_per_use;
            std::stringstream ss;
            ss << item_actions[p.first].name << " [" << p.second->type_name( 1 );
            if( would_use_charges > 0 ) {
                ss << " (" << would_use_charges << "/" << p.second->charges << ")";
            }
            ss << "]";

            nc_color col = c_white;
            if( selection == i ) {
                col = hilite( col );
                selected_item = p.second;
            }
            mvwprintz(w_item_actions, 3 + i, 1, col, "%s", ss.str().c_str());
            i++;
        }

        wrefresh(w_item_actions);
        const std::string action = ctxt.handle_input();
        if (action == "DOWN") {
            selection++;
            if( selection >= iactions.size() ) {
                selection = 0;
            }
        } else if( action == "UP" ) {
            if( selection == 0 ) {
                selection = iactions.size() - 1;
            } else {
                selection--;
            }
        } else if( action == "CONFIRM" ) {
            invpos = u.inv.position_by_item( selected_item );
            break;
        } else if( action == "QUIT" ) {
            break;
        }
    }

    werase(w_item_actions);
    delwin(w_item_actions);
    refresh_all();
    if( invpos != INT_MIN ) {
        u.use( invpos );
    }
}

void item_action::load_item_action(JsonObject &jo)
{
    item_action ia;

    ia.id = jo.get_string( "id" );
    ia.name = jo.get_string( "name", "" );
    if( !ia.name.empty() ) {
        ia.name = _( ia.name.c_str() );
    } else {
        ia.name = ia.id;
    }

    ia.keybind = inp_mngr.get_keycode( jo.get_string( "keybind", "" ) );
    item_actions[ia.id] = ia;
}
