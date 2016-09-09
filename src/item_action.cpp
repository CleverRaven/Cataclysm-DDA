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
#include "iuse_actor.h"
#include "translations.h"
#include "input.h"
#include "itype.h"
#include "ui.h"
#include "player.h"
#include <istream>
#include <sstream>
#include <iterator>

static item_action nullaction;
static const std::string errstring( "ERROR" );

int clamp( int value, int low, int high )
{
    return ( value < low ) ? low : ( ( value > high ) ? high : value );
}

char key_bound_to( const input_context &ctxt, const item_action_id &act )
{
    auto keys = ctxt.keys_bound_to( act );
    return keys.empty() ? '\0' : keys[0];
}

class actmenu_cb : public uimenu_callback
{
    private:
        input_context ctxt;
        const action_map am;
    public:
        actmenu_cb( const action_map &acm ) : ctxt( "ITEM_ACTIONS" ), am( acm ) {
            ctxt.register_action( "HELP_KEYBINDINGS" );
            ctxt.register_action( "QUIT" );
            for( const auto &id : am ) {
                ctxt.register_action( id.first, id.second.name );
            }
        }
        ~actmenu_cb() override { }

        bool key( int ch, int /*num*/, uimenu * /*menu*/ ) override {
            input_event wrap = input_event( ch, CATA_INPUT_KEYBOARD );
            const std::string action = ctxt.input_to_action( wrap );
            if( action == "HELP_KEYBINDINGS" ) {
                ctxt.display_help();
                return true;
            }
            // Don't write a message if unknown command was sent
            // Only when an inexistent tool was selected
            auto itemless_action = am.find( action );
            if( itemless_action != am.end() ) {
                popup( _( "You do not have an item that can perform this action." ) );
            }
            return false;
        }
};

item_action_generator::item_action_generator() = default;

item_action_generator::~item_action_generator() = default;

// Get use methods of this item and its contents
bool item_has_uses_recursive( const item &it )
{
    if( !it.type->use_methods.empty() ) {
        return true;
    }

    for( const auto &elem : it.contents ) {
        if( item_has_uses_recursive( elem ) ) {
            return true;
        }
    }

    return false;
}

item_action_map item_action_generator::map_actions_to_items( player &p ) const
{
    return map_actions_to_items( p, std::vector<item *>() );
}

item_action_map item_action_generator::map_actions_to_items( player &p,
        const std::vector<item *> &pseudos ) const
{
    std::set< item_action_id > unmapped_actions;
    for( auto &ia_ptr : item_actions ) { // Get ids of wanted actions
        unmapped_actions.insert( ia_ptr.first );
    }

    item_action_map candidates;
    std::vector< item * > items = p.inv_dump();
    items.reserve( items.size() + pseudos.size() );
    items.insert( items.end(), pseudos.begin(), pseudos.end() );

    std::unordered_set< item_action_id > to_remove;
    for( item *i : items ) {
        if( !item_has_uses_recursive( *i ) ) {
            continue;
        }

        for( const item_action_id &use : unmapped_actions ) {
            // Actually used item can be different from the "outside item"
            // For example, sheathed knife
            item *actual_item = i->get_usable_item( use );
            if( actual_item == nullptr ) {
                continue;
            }

            const use_function *func = actual_item->get_use( use );
            if( !( func && func->get_actor_ptr() &&
                   func->get_actor_ptr()->can_use( &p, actual_item, false, p.pos() ) ) ) {
                continue;
            }
            if( !actual_item->ammo_sufficient() ) {
                continue;
            }

            // Add to usable items if it needs less charges per use or has less charges
            auto found = candidates.find( use );
            bool better = false;
            if( found == candidates.end() ) {
                better = true;
            } else {
                if( actual_item->ammo_required() > found->second->ammo_required() ) {
                    continue; // Other item consumes less charges
                }

                if( found->second->ammo_remaining() > actual_item->ammo_remaining() ) {
                    better = true; // Items with less charges preferred
                }
            }

            if( better ) {
                candidates[use] = i;
                if( actual_item->ammo_required() == 0 ) {
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

std::string item_action_generator::get_action_name( const item_action_id &id ) const
{
    const auto &act = get_action( id );
    if( !act.name.empty() ) {
        return _( act.name.c_str() );
    }

    return id;
}

const item_action &item_action_generator::get_action( const item_action_id &id ) const
{
    const auto &iter = item_actions.find( id );
    if( iter != item_actions.end() ) {
        return iter->second;
    }

    debugmsg( "Couldn't find item action named %s", id.c_str() );
    return nullaction;
}

void item_action_generator::load_item_action( JsonObject &jo )
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

void game::item_action_menu()
{
    const auto &gen = item_action_generator::generator();
    const action_map &item_actions = gen.get_item_action_map();

    // A bit of a hack for now. If more pseudos get implemented, this should be un-hacked
    std::vector<item *> pseudos;
    item toolset( "toolset", calendar::turn );
    if( u.has_active_bionic( "bio_tools" ) ) {
        pseudos.push_back( &toolset );
    }

    item_action_map iactions = gen.map_actions_to_items( u, pseudos );
    if( iactions.empty() ) {
        popup( _( "You don't have any items with registered uses" ) );
    }

    uimenu kmenu;
    kmenu.text = _( "Execute which action?" );
    kmenu.return_invalid = true;
    input_context ctxt( "ITEM_ACTIONS" );
    actmenu_cb callback( item_actions );
    kmenu.callback = &callback;
    int num = 0;
    for( auto &p : iactions ) {
        std::stringstream ss;
        ss << gen.get_action_name( p.first ) << " [" << p.second->display_name();
        if( p.second->ammo_required() ) {
            ss << " (" << p.second->ammo_required() << '/' << p.second->ammo_remaining() << ')';
        }
        ss << "]";

        char bind = key_bound_to( ctxt, p.first );
        kmenu.addentry( num, true, bind, ss.str() );
        num++;
    }

    for( auto &p : item_actions ) {
        if( iactions.find( p.first ) == iactions.end() ) {
            char bind = key_bound_to( ctxt, p.first );
            kmenu.addentry( num, false, bind, gen.get_action_name( p.first ) );
            num++;
        }
    }

    kmenu.addentry( num, true, key_bound_to( ctxt, "QUIT" ), _( "Cancel" ) );

    kmenu.query();
    if( kmenu.ret < 0 || kmenu.ret >= ( int )iactions.size() ) {
        return;
    }

    draw_ter();

    auto iter = iactions.begin();
    std::advance( iter, kmenu.ret );

    if( u.invoke_item( iter->second, iter->first ) ) {
        // Need to remove item
        u.i_rem( iter->second );
    }

    u.inv.restack( &u );
    u.inv.unsort();
}

std::string use_function::get_type() const
{
    if( actor ) {
        return actor->type;
    } else {
        return errstring;
    }
}

std::string iuse_actor::get_name() const
{
    return item_action_generator::generator().get_action_name( type );
}

std::string use_function::get_name() const
{
    if( actor ) {
        return actor->get_name();
    } else {
        return errstring;
    }
}

