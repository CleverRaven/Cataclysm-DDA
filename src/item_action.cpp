#include "item_action.h"

#include <algorithm>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <set>
#include <tuple>
#include <unordered_set>
#include <utility>

#include "avatar.h"
#include "catacharset.h"
#include "character.h"
#include "clone_ptr.h"
#include "debug.h"
#include "flag.h"
#include "game.h"
#include "input_context.h"
#include "inventory.h"
#include "item.h"
#include "item_contents.h"
#include "item_factory.h"
#include "item_pocket.h"
#include "itype.h"
#include "iuse.h"
#include "make_static.h"
#include "output.h"
#include "pimpl.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"

static const std::string errstring( "ERROR" );

static item_action nullaction;

static std::optional<input_event> key_bound_to( const input_context &ctxt,
        const item_action_id &act )
{
    const std::vector<input_event> keys = ctxt.keys_bound_to( act, /*maximum_modifier_count=*/1 );
    if( keys.empty() ) {
        return std::nullopt;
    } else {
        return keys.front();
    }
}

class actmenu_cb : public uilist_callback
{
    private:
        const action_map am;
    public:
        explicit actmenu_cb( const action_map &acm ) : am( acm ) { }
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

bool item::item_has_uses_recursive( bool contents_only ) const
{
    if( !contents_only && !type->use_methods.empty() ) {
        return true;
    }

    return contents.item_has_uses_recursive();
}

bool item_contents::item_has_uses_recursive() const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( pocket_type::CONTAINER ) &&
            pocket.item_has_uses_recursive() ) {
            return true;
        }
    }

    return false;
}

bool item_pocket::item_has_uses_recursive() const
{
    for( const item &it : contents ) {
        if( it.item_has_uses_recursive() ) {
            return true;
        }
    }

    return false;
}

item_action_map item_action_generator::map_actions_to_items( Character &you ) const
{
    return map_actions_to_items( you, std::vector<item *>() );
}

item_action_map item_action_generator::map_actions_to_items( Character &you,
        const std::vector<item *> &pseudos, const bool use_player_inventory ) const
{
    std::set< item_action_id > unmapped_actions;
    for( const auto &ia_ptr : item_actions ) { // Get ids of wanted actions
        unmapped_actions.insert( ia_ptr.first );
    }

    item_action_map candidates;
    std::vector< item * > items;
    // Default behavior
    if( use_player_inventory ) {
        items = you.inv_dump();
        items.reserve( items.size() + pseudos.size() );
        items.insert( items.end(), pseudos.begin(), pseudos.end() );
    } else {
        items = pseudos;
    }

    std::unordered_set< item_action_id > to_remove;
    for( item *i : items ) {
        if( !i->item_has_uses_recursive() ) {
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
                   func->get_actor_ptr()->can_use( you, *actual_item, you.pos() ).success() ) ) {
                continue;
            }

            if( !actual_item->ammo_sufficient( &you, use ) ) {
                continue;
            }

            // Don't try to remove 'irremovable' toolmods
            if( actual_item->is_toolmod() && use == STATIC( item_action_id( "TOOLMOD_ATTACH" ) ) &&
                actual_item->has_flag( STATIC( flag_id( "IRREMOVABLE" ) ) ) ) {
                continue;
            }

            // Prevent drinking frozen liquids that have specific use actions (ex: ALCOHOL)
            if( actual_item->is_frozen_liquid() ) {
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
                candidates[use] = actual_item;
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
    const item_action &act = get_action( id );
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

void item_action_generator::load_item_action( const JsonObject &jo )
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
        const item_action &action = elem.second;
        if( !item_controller->has_iuse( action.id ) ) {
            debugmsg( "Item action \"%s\" isn't known to the game.  Check item action definitions in JSON.",
                      action.id.c_str() );
        }
    }
}

void game::item_action_menu( item_location loc )
{
    const item_action_generator &gen = item_action_generator::generator();
    const action_map &item_actions = gen.get_item_action_map();
    std::vector<item *> pseudos;
    bool use_player_inventory = false;
    if( !loc ) {
        use_player_inventory = true;
        // Ugly const_cast because the menu needs non-const pointers
        std::vector<const item *> pseudo_items = get_player_character().get_pseudo_items();
        pseudos.reserve( pseudo_items.size() );
        for( const item *pseudo : pseudo_items ) {
            pseudos.push_back( const_cast<item *>( pseudo ) );
        }
    } else {
        if( loc.get_item()->type->has_use() ) {
            pseudos.push_back( const_cast< item * >( loc.get_item() ) );
        }
        loc.get_item()->visit_contents( [&pseudos]( item * node, item * ) {
            if( node->type->use_methods.empty() ) {
                return VisitResponse::NEXT;
            }
            pseudos.push_back( const_cast<item *>( node ) );
            return VisitResponse::NEXT;
        } );
    }
    item_action_map iactions = gen.map_actions_to_items( u, pseudos, use_player_inventory );
    if( iactions.empty() ) {
        popup( _( "You don't have any items with registered uses" ) );
    }

    uilist kmenu;
    kmenu.desc_enabled = true;
    kmenu.text = _( "Execute which action?" );
    kmenu.input_category = "ITEM_ACTIONS";
    input_context ctxt( "ITEM_ACTIONS", keyboard_mode::keycode );
    for( const std::pair<const item_action_id, item_action> &id : item_actions ) {
        ctxt.register_action( id.first, id.second.name );
        kmenu.additional_actions.emplace_back( id.first, id.second.name );
    }
    actmenu_cb callback( item_actions );
    kmenu.callback = &callback;
    int num = 0;

    const auto assigned_action = [&iactions]( const item_action_id & action ) {
        return iactions.find( action ) != iactions.end();
    };

    std::vector<std::tuple<item_action_id, std::string, std::string, std::string>> menu_items;
    // Sorts menu items by action.
    using Iter = decltype( menu_items )::iterator;
    const auto sort_menu = []( Iter from, Iter to ) {
        std::sort( from, to, []( const auto & lhs, const auto & rhs ) {
            return std::get<1>( lhs ).compare( std::get<1>( rhs ) ) < 0;
        } );
    };
    // Add mapped actions to the menu vector.
    std::transform( iactions.begin(), iactions.end(), std::back_inserter( menu_items ),
    []( const std::pair<item_action_id, item *> &elem ) {
        std::string ss = elem.second->display_name();
        if( elem.second->ammo_required() ) {

            if( elem.second->has_flag( flag_USES_BIONIC_POWER ) ) {
                ss += string_format( "(%d kJ)", elem.second->ammo_required() );
            } else {
                auto iter = elem.second->type->ammo_scale.find( elem.first );
                ss += string_format( "(-%d)", elem.second->ammo_required() * ( iter ==
                                     elem.second->type->ammo_scale.end() ? 1 : iter->second ) );
            }

        }

        const use_function *method = elem.second->get_use( elem.first );
        if( method ) {
            return std::make_tuple( method->get_type(), method->get_name(), ss, method->get_description() );
        } else {
            return std::make_tuple( errstring, std::string( "NO USE FUNCTION" ), ss, std::string() );
        }
    } );
    // Sort mapped actions.
    sort_menu( menu_items.begin(), menu_items.end() );
    if( !loc ) {
        // Add unmapped but binded actions to the menu vector.
        for( const auto &elem : item_actions ) {
            if( key_bound_to( ctxt, elem.first ).has_value() && !assigned_action( elem.first ) ) {
                menu_items.emplace_back( elem.first, gen.get_action_name( elem.first ), "-", std::string() );
            }
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
        std::string ss;
        ss += std::get<1>( elem );
        ss += std::string( max_len.first - utf8_width( std::get<1>( elem ), true ), ' ' );
        ss += std::string( 4, ' ' );

        ss += std::get<2>( elem );
        ss += std::string( max_len.second - utf8_width( std::get<2>( elem ), true ), ' ' );

        const std::optional<input_event> bind = key_bound_to( ctxt, std::get<0>( elem ) );
        const bool enabled = assigned_action( std::get<0>( elem ) );
        const std::string desc =  std::get<3>( elem ) ;

        kmenu.addentry_desc( num, enabled, bind, ss, desc );
        num++;
    }

    kmenu.footer_text = string_format( _( "[<color_yellow>%s</color>] keybindings" ),
                                       ctxt.get_desc( "HELP_KEYBINDINGS" ) );

    kmenu.query();
    if( kmenu.ret < 0 || kmenu.ret >= static_cast<int>( iactions.size() ) ) {
        return;
    }

    const item_action_id action = std::get<0>( menu_items[kmenu.ret] );
    item *it = iactions[action];

    u.invoke_item( it, action );

    u.inv->restack( u );
    u.inv->unsort();
}

std::string use_function::get_type() const
{
    if( actor ) {
        return actor->type;
    } else {
        return errstring;
    }
}

ret_val<void> iuse_actor::can_use( const Character &, const item &, const tripoint & ) const
{
    return ret_val<void>::make_success();
}

bool iuse_actor::is_valid() const
{
    return item_action_generator::generator().action_exists( type );
}

std::string iuse_actor::get_name() const
{
    return item_action_generator::generator().get_action_name( type );
}

std::string iuse_actor::get_description() const
{
    return std::string();
}

std::string use_function::get_name() const
{
    if( actor ) {
        return actor->get_name();
    } else {
        return errstring;
    }
}

std::string use_function::get_description() const
{
    if( actor ) {
        return actor->get_description();
    } else {
        return errstring;
    }
}
