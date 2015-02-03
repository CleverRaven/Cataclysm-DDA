#include "action.h"
#include "output.h"
#include "options.h"
#include "path_info.h"
#include "debug.h"
#include "game.h"
#include "options.h"
#include "messages.h"
#include "inventory.h"
#include "item_factory.h"
#include "item_action.h"
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
    return keys.empty() ? '\0' : keys[0];
}

class actmenu_cb : public uimenu_callback {
    private:
        input_context ctxt;
    public:
        actmenu_cb( ) : ctxt("ITEM_ACTIONS") {
            ctxt.register_action("HELP_KEYBINDINGS");
            ctxt.register_action("QUIT");
            for( auto id : item_actions ) {
                ctxt.register_action( id.first, id.second.name );
            }
        }
        ~actmenu_cb() { }
        
        bool key(int ch, int /*num*/, uimenu * /*menu*/) {
            input_event wrap = input_event( ch, CATA_INPUT_KEYBOARD );
            const std::string action = ctxt.input_to_action( wrap );
            if( action == "HELP_KEYBINDINGS" ) {
                ctxt.display_help();
                return true;
            }
            // Don't write a message if unknown command was sent
            // Only when an inexistent tool was selected
            auto itemless_action = item_actions.find( action );
            if( itemless_action != item_actions.end() ) {
                popup( _("You do not have an item that can perform this action.") );
            }
            return false;
        }
};

item_action_map map_actions_to_items( player &p )
{
    std::set< item_action_id > unmapped_actions;
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
            const use_function *ufunc = i->type->get_use( use );
            // Can't just test for charges_per_use > charges, because charges can be -1
            if( ufunc == nullptr ||
                ( ufunc->get_actor_ptr() != nullptr && !ufunc->get_actor_ptr()->can_use( &p, i, false, p.pos() ) ) ||
                ( tool != nullptr && tool->charges_per_use > 0 && tool->charges_per_use > i->charges ) ) {
                continue;
            }

            // Add to usable items if it needs less charges per use or has less charges
            auto found = candidates.find( use );
            int would_use_charges = tool == nullptr ? 0 : tool->charges_per_use;
            bool better = false;
            if( found == candidates.end() ) {
                better = true;
            } else {
                it_tool *other = dynamic_cast<it_tool*>(found->second->type);
                if( other == nullptr || would_use_charges > other->charges_per_use ) {
                    continue; // Other item consumes less charges
                }

                if( found->second->charges > i->charges ) {
                    better = true; // Items with less charges preferred
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

    uimenu kmenu;
    kmenu.text = _( "Execute which action?" );
    input_context ctxt("ITEM_ACTIONS");
    actmenu_cb callback;
    kmenu.callback = &callback;
    int num = 0;
    for( auto &p : iactions ) {
        it_tool *tool = dynamic_cast<it_tool*>( p.second->type );
        int would_use_charges = tool == nullptr ? 0 : tool->charges_per_use;
        
        std::stringstream ss;
        ss << _( item_actions[p.first].name.c_str() ) << " [" << p.second->type_name( 1 );
        if( would_use_charges > 0 ) {
            ss << " (" << would_use_charges << '/' << p.second->charges << ')';
        }
        ss << "]";
        
        char bind = key_bound_to( ctxt, p.first );
        kmenu.addentry( num, true, bind, ss.str() );
        num++;
    }

    std::set< item_action_id > itemless;
    for( auto &p : item_actions ) {
        if( iactions.find( p.first ) == iactions.end() ) {
            char bind = key_bound_to( ctxt, p.first );
            kmenu.addentry( num, false, bind, _( item_actions[p.first].name.c_str() ) );
            num++;
        }
    }
    
    kmenu.addentry( num, true, key_bound_to( ctxt, "QUIT" ), _("Cancel") );

    kmenu.query();
    if( kmenu.ret < 0 || kmenu.ret >= (int)iactions.size() ) {
        return;
    }
    
    auto iter = iactions.begin();
    for( int i = 0; i < kmenu.ret; i++) {
        iter++;
    }
    int invpos = u.inv.position_by_item( iter->second );
    draw_ter();
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
