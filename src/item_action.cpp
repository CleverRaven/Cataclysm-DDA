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
#include "item_factory.h"
#include <istream>
#include <sstream>
#include <fstream>
#include <iterator>

std::unordered_map< item_action_id, item_action > item_actions;

int clamp(int value, int low, int high)
{
    return (value < low) ? low : ( (value > high) ? high : value );
}

char key_bound_to( const input_context &ctxt, const item_action_id &act )
{
    auto keys = ctxt.keys_bound_to( act );
    return keys.empty() ? ' ' : keys[0];
}

item_action_map map_actions_to_items( player &p )
{
    std::unordered_set< item_action_id > unmapped_actions;
    for( auto &p : item_actions ) { // Get ids of wanted actions
        unmapped_actions.insert( p.first );
    }

    item_action_map candidates;
    inventory inv = p.inv;
    inv += p.weapon;
    inv += p.worn;
    std::vector< item* > items;
    p.inv.dump( items );

    std::unordered_set< item_action_id > to_remove;
    for( item *i : items ) {
        if( i->type->use_methods.empty() ) {
            continue;
        }

        for( const item_action_id &use : unmapped_actions ) {
            it_tool *tool = dynamic_cast<it_tool*>(i->type);
            // Can't just test for charges_per_use > charges, because charges can be -1
            if( !i->type->can_use( use ) ||
                (tool != nullptr && tool->charges_per_use > 0 && tool->charges_per_use > i->charges) ) {
                continue;
            }

            // Add to usable items only if it needs less charges per use
            auto found = candidates.find( use );
            int would_use_charges = tool == nullptr ? 0 : tool->charges_per_use;
            bool better = false;
            if( found == candidates.end() ) {
                better = true;
            } else {
                it_tool *other = dynamic_cast<it_tool*>(found->second->type);
                if ( other != nullptr && would_use_charges < other->charges_per_use ) {
                    better = true;
                }
            }
            
            if( better ) {
                candidates[use] = i;
                if( would_use_charges == 0 ) {
                    to_remove.insert( use );
                }
            }
        }

        for( const item_action_id &r : to_remove ) {
            unmapped_actions.erase( r );
        }
    }

    return candidates;
}

void game::item_action_menu()
{
    item_action_map iactions = map_actions_to_items( u );
    if( iactions.empty() ) {
        popup( _("You don't have any items with registered uses") );
    }

    int height = FULL_SCREEN_HEIGHT;
    int width = FULL_SCREEN_WIDTH;
    int begin_y = (TERMY > height) ? (TERMY - height) / 2 : 0;
    int begin_x = (TERMX > width) ? (TERMX - width) / 2 : 0;
    WINDOW *w_item_actions = newwin(height, width, begin_y, begin_x);
    int invpos = INT_MIN;
    size_t selection = 0;
    item *selected_item = nullptr;
    input_context ctxt("ITEM_ACTIONS");
    ctxt.register_action("UP");
    ctxt.register_action("DOWN");
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("HELP_KEYBINDINGS");
    for( auto id : item_actions ) {
        ctxt.register_action( id.first, id.second.name );
    }

    while (true) {
        werase(w_item_actions);

        for (int i = 1; i < width - 1; i++) {
            mvwputch(w_item_actions, 1, i, BORDER_COLOR, LINE_OXOX);
            mvwputch(w_item_actions, height - 1, i, BORDER_COLOR, LINE_OXOX);

            if (i > 1 && i < height - 1) {
                mvwputch(w_item_actions, i, 0, BORDER_COLOR, LINE_XOXO);
                mvwputch(w_item_actions, i, width - 1, BORDER_COLOR, LINE_XOXO);
            }
        }

        mvwputch(w_item_actions, 1, 0, BORDER_COLOR, LINE_OXXO); // |^
        mvwputch(w_item_actions, 1, width - 1, BORDER_COLOR, LINE_OOXX); // ^|

        mvwputch(w_item_actions, height - 1, 0, BORDER_COLOR, LINE_XXOO); // |
        mvwputch(w_item_actions, height - 1, width - 1, BORDER_COLOR, LINE_XOOX); // _|
        
        mvwprintz(w_item_actions, 0, 1, c_white, "%s", _("Select an action to perform") );

        size_t line = 0;
        const size_t lineoffset = clamp( (int)selection - height / 2, 0, (int)iactions.size() - 2 - height / 2 );
        for( auto p : iactions ) {
            line++;
            if( lineoffset >= line ) {
                continue;
            }
            nc_color col = c_white;
            if( selection == line - 1 ) {
                col = hilite( col );
                selected_item = p.second;
            }
            
            if( 2 + line - lineoffset >= (size_t)height ) {
                break;
            }
            
            it_tool *tool = dynamic_cast<it_tool*>( p.second->type );
            int would_use_charges = tool == nullptr ? 0 : tool->charges_per_use;
            char bind = key_bound_to( ctxt, p.first );
            std::stringstream ss;
            ss << bind << ' ' << _( item_actions[p.first].name.c_str() ) << " [" << p.second->type_name( 1 );
            if( would_use_charges > 0 ) {
                ss << " (" << would_use_charges << '/' << p.second->charges << ')';
            }
            ss << "]";

            mvwprintz(w_item_actions, 1 + line - lineoffset, 1, col, "%s", ss.str().c_str());
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
        } else {
            auto ac = iactions.find( action );
            if( ac != iactions.end() ) {
                invpos = u.inv.position_by_item( ac->second );
                break;
            }
            // Don't write a message if unknown command was sent
            // Only when an inexistent tool was selected
            auto itemless_action = item_actions.find( action );
            if( itemless_action != item_actions.end() ) {
                popup( _("You do not have an item that can perform this action.") );
            }
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

    item_actions[ia.id] = ia;
}
