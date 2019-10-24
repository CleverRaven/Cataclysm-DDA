#include "item_action.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <memory>
#include <set>
#include <tuple>
#include <unordered_set>
#include <utility>

#include "avatar.h"
#include "debug.h"
#include "game.h"
#include "input.h"
#include "inventory.h"
#include "item.h"
#include "item_factory.h"
#include "itype.h"
#include "json.h"
#include "output.h"
#include "player.h"
#include "ret_val.h"
#include "translations.h"
#include "ui.h"
#include "calendar.h"
#include "catacharset.h"
#include "cursesdef.h"
#include "iuse.h"
#include "type_id.h"

struct tripoint;

static item_action nullaction;
static const std::string errstring( "ERROR" );

static char key_bound_to( const input_context &ctxt, const item_action_id &act )
{
    auto keys = ctxt.keys_bound_to( act );
    return keys.empty() ? '\0' : keys[0];
}

class actmenu_cb : public uilist_callback
{
    private:
        const action_map am;
    public:
        actmenu_cb( const action_map &acm ) : am( acm ) { }
        ~actmenu_cb() override = default;

        bool key( const input_context &ctxt, const input_event &event, int /*idx*/,
                  uilist * /*menu*/ ) override {
            const std::string &action = ctxt.input_to_action( event );
            // Don't write a message if unknown command was sent
            // Only when an inexistent tool was selected
            const auto itemless_action = am.find( action );
            if( itemless_action != am.end() ) {
                popup( _( "You do not have an item that can perform this action." ) );
                return true;
            }
            return false;
        }
};

item_action_generator::item_action_generator() = default;

item_action_generator::~item_action_generator() = default;

// Get use methods of this item and its contents
static bool item_has_uses_recursive( const item &it )
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
                   func->get_actor_ptr()->can_use( p, *actual_item, false, p.pos() ).success() ) ) {
                continue;
            }
            if( !actual_item->ammo_sufficient() ) {
                continue;
            }

            // Don't try to remove 'irremovable' toolmods
            if( actual_item->is_toolmod() && use == item_action_id( "TOOLMOD_ATTACH" ) &&
                actual_item->has_flag( "IRREMOVABLE" ) ) {
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
        return act.name.translated();
    }

    return id;
}

bool item_action_generator::action_exists( const item_action_id &id ) const
{
    return item_actions.find( id ) != item_actions.end();
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
    if( !jo.read( "name", ia.name ) || ia.name.empty() ) {
        ia.name = no_translation( ia.id );
    }

    item_actions[ia.id] = ia;
}

void item_action_generator::check_consistency() const
{
    for( const auto &elem : item_actions ) {
        const auto &action = elem.second;
        if( !item_controller->has_iuse( action.id ) ) {
            debugmsg( "Item action \"%s\" isn't known to the game. Check item action definitions in JSON.",
                      action.id.c_str() );
        }
    }
}

void game::item_action_menu()
{
    const auto &gen = item_action_generator::generator();
    const action_map &item_actions = gen.get_item_action_map();

    // A bit of a hack for now. If more pseudos get implemented, this should be un-hacked
    std::vector<item *> pseudos;
    item toolset( "toolset", calendar::turn );
    if( u.has_active_bionic( bionic_id( "bio_tools" ) ) ) {
        pseudos.push_back( &toolset );
    }
    item bio_claws( "bio_claws_weapon", calendar::turn );
    if( u.has_active_bionic( bionic_id( "bio_claws" ) ) ) {
        pseudos.push_back( &bio_claws );
    }

    item_action_map iactions = gen.map_actions_to_items( u, pseudos );
    if( iactions.empty() ) {
        popup( _( "You don't have any items with registered uses" ) );
    }

    uilist kmenu;
    kmenu.text = _( "Execute which action?" );
    kmenu.input_category = "ITEM_ACTIONS";
    input_context ctxt( "ITEM_ACTIONS" );
    for( const auto &id : item_actions ) {
        ctxt.register_action( id.first, id.second.name.translated() );
        kmenu.additional_actions.emplace_back( id.first, id.second.name.translated() );
    }
    actmenu_cb callback( item_actions );
    kmenu.callback = &callback;
    int num = 0;

    const auto assigned_action = [&iactions]( const item_action_id & action ) {
        return iactions.find( action ) != iactions.end();
    };

    std::vector<std::tuple<item_action_id, std::string, std::string>> menu_items;
    // Sorts menu items by action.
    using Iter = decltype( menu_items )::iterator;
    const auto sort_menu = []( Iter from, Iter to ) {
        std::sort( from, to, []( const std::tuple<item_action_id, std::string, std::string> &lhs,
        const std::tuple<item_action_id, std::string, std::string> &rhs ) {
            return std::get<1>( lhs ).compare( std::get<1>( rhs ) ) < 0;
        } );
    };
    // Add mapped actions to the menu vector.
    std::transform( iactions.begin(), iactions.end(), std::back_inserter( menu_items ),
    []( const std::pair<item_action_id, item *> &elem ) {
        std::stringstream ss;
        ss << elem.second->display_name();
        if( elem.second->ammo_required() ) {
            ss << " (" << elem.second->ammo_required() << '/'
               << elem.second->ammo_remaining() << ')';
        }

        const auto method = elem.second->get_use( elem.first );
        return std::make_tuple( method->get_type(), method->get_name(), ss.str() );
    } );
    // Sort mapped actions.
    sort_menu( menu_items.begin(), menu_items.end() );
    // Add unmapped but binded actions to the menu vector.
    for( const auto &elem : item_actions ) {
        if( key_bound_to( ctxt, elem.first ) != '\0' && !assigned_action( elem.first ) ) {
            menu_items.emplace_back( elem.first, gen.get_action_name( elem.first ), "-" );
        }
    }
    // Sort unmapped actions.
    auto iter = menu_items.begin();
    std::advance( iter, iactions.size() );
    sort_menu( iter, menu_items.end() );
    // Determine max lengths, to print the menu nicely.
    std::pair<int, int> max_len;
    for( const auto &elem : menu_items ) {
        max_len.first = std::max( max_len.first, utf8_width( std::get<1>( elem ), true ) );
        max_len.second = std::max( max_len.second, utf8_width( std::get<2>( elem ), true ) );
    }
    // Fill the menu.
    for( const auto &elem : menu_items ) {
        std::stringstream ss;
        ss << std::get<1>( elem )
           << std::string( max_len.first - utf8_width( std::get<1>( elem ), true ), ' ' )
           << std::string( 4, ' ' );

        ss << std::get<2>( elem )
           << std::string( max_len.second - utf8_width( std::get<2>( elem ), true ), ' ' );

        const char bind = key_bound_to( ctxt, std::get<0>( elem ) );
        const bool enabled = assigned_action( std::get<0>( elem ) );

        kmenu.addentry( num, enabled, bind, ss.str() );
        num++;
    }

    kmenu.query();
    if( kmenu.ret < 0 || kmenu.ret >= static_cast<int>( iactions.size() ) ) {
        return;
    }

    draw_ter();
    wrefresh( w_terrain );
    draw_panels( true );

    const item_action_id action = std::get<0>( menu_items[kmenu.ret] );
    item *it = iactions[action];

    u.invoke_item( it, action );

    u.inv.restack( u );
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

ret_val<bool> iuse_actor::can_use( const player &, const item &, bool, const tripoint & ) const
{
    return ret_val<bool>::make_success();
}

bool iuse_actor::is_valid() const
{
    return item_action_generator::generator().action_exists( type );
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

