#include "craft_command.h"

#include <sstream>
#include <algorithm>

#include "debug.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "itype.h"
#include "output.h"
#include "player.h"
#include "recipe.h"
#include "requirements.h"
#include "translations.h"
#include "uistate.h"

template<typename CompType>
std::string comp_selection<CompType>::nname() const
{
    switch( use_from ) {
        case use_from_map:
            return item::nname( comp.type, comp.count ) + _( " (nearby)" );
        case use_from_both:
            return item::nname( comp.type, comp.count ) + _( " (person & nearby)" );
        case use_from_player: // Is the same as the default return;
        case use_from_none:
        case cancel:
            break;
    }

    return item::nname( comp.type, comp.count );
}

void craft_command::execute()
{
    if( empty() ) {
        return;
    }

    bool need_selections = true;
    inventory map_inv;
    map_inv.form_from_map( crafter->pos(), PICKUP_RANGE );

    if( has_cached_selections() ) {
        std::vector<comp_selection<item_comp>> missing_items = check_item_components_missing( map_inv );
        std::vector<comp_selection<tool_comp>> missing_tools = check_tool_components_missing( map_inv );

        if( missing_items.empty() && missing_tools.empty() ) {
            // All items we used previously are still there, so we don't need to do selection.
            need_selections = false;
        } else if( !query_continue( missing_items, missing_tools ) ) {
            return;
        }
    }

    if( need_selections ) {
        item_selections.clear();
        const auto needs = rec->requirements();
        for( const auto &it : needs.get_components() ) {
            comp_selection<item_comp> is = crafter->select_item_component( it, batch_size, map_inv, true );
            if( is.use_from == cancel ) {
                return;
            }
            item_selections.push_back( is );
        }

        tool_selections.clear();
        for( const auto &it : needs.get_tools() ) {
            comp_selection<tool_comp> ts = crafter->select_tool_component(
                                               it, batch_size, map_inv, DEFAULT_HOTKEYS, true );
            if( ts.use_from == cancel ) {
                return;
            }
            tool_selections.push_back( ts );
        }
    }

    auto type = activity_id( is_long ? "ACT_LONGCRAFT" : "ACT_CRAFT" );
    auto activity = player_activity( type, crafter->base_time_to_craft( *rec, batch_size ), -1, INT_MIN,
                                     rec->ident().str() );
    activity.values.push_back( batch_size );
    activity.values.push_back( calendar::turn );
    activity.coords.push_back( crafter->pos() );

    crafter->assign_activity( activity );

    /* legacy support for lua bindings to last_batch and lastrecipe */
    crafter->last_batch = batch_size;
    crafter->lastrecipe = rec->ident();

    const auto iter = std::find( uistate.recent_recipes.begin(), uistate.recent_recipes.end(),
                                 rec->ident() );
    if( iter != uistate.recent_recipes.end() ) {
        uistate.recent_recipes.erase( iter );
    }

    uistate.recent_recipes.push_back( rec->ident() );

    if( uistate.recent_recipes.size() > 20 ) {
        uistate.recent_recipes.erase( uistate.recent_recipes.begin() );
    }
}

/** Does a string join with ', ' of the components in the passed vector and inserts into 'str' */
template<typename T>
static void component_list_string( std::stringstream &str,
                                   const std::vector<comp_selection<T>> &components )
{
    str << enumerate_as_string( components.begin(), components.end(),
    []( const comp_selection<T> &comp ) {
        return comp.nname();
    } );
}

bool craft_command::query_continue( const std::vector<comp_selection<item_comp>> &missing_items,
                                    const std::vector<comp_selection<tool_comp>> &missing_tools )
{
    std::stringstream ss;
    ss << _( "Some components used previously are missing. Continue?" );

    if( !missing_items.empty() ) {
        ss << std::endl << _( "Item(s): " );
        component_list_string( ss, missing_items );
    }

    if( !missing_tools.empty() ) {
        ss << std::endl << _( "Tool(s): " );
        component_list_string( ss, missing_tools );
    }

    return query_yn( ss.str() );
}

std::list<item> craft_command::consume_components()
{
    std::list<item> used;
    if( crafter->has_trait( trait_id( "DEBUG_HS" ) ) ) {
        return used;
    }

    if( empty() ) {
        debugmsg( "Warning: attempted to consume items from an empty craft_command" );
        return used;
    }

    inventory map_inv;
    map_inv.form_from_map( crafter->pos(), PICKUP_RANGE );

    if( !check_item_components_missing( map_inv ).empty() ) {
        debugmsg( "Aborting crafting: couldn't find cached components" );
        return used;
    }

    for( const auto &it : item_selections ) {
        std::list<item> tmp = crafter->consume_items( it, batch_size );
        used.splice( used.end(), tmp );
    }

    for( const auto &it : tool_selections ) {
        crafter->consume_tools( it, batch_size );
    }

    return used;
}

std::vector<comp_selection<item_comp>> craft_command::check_item_components_missing(
                                        const inventory &map_inv ) const
{
    std::vector<comp_selection<item_comp>> missing;

    for( const auto &item_sel : item_selections ) {
        itype_id type = item_sel.comp.type;
        item_comp component = item_sel.comp;
        long count = ( component.count > 0 ) ? component.count * batch_size : abs( component.count );

        if( item::count_by_charges( type ) && count > 0 ) {
            switch( item_sel.use_from ) {
                case use_from_player:
                    if( !crafter->has_charges( type, count ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case use_from_map:
                    if( !map_inv.has_charges( type, count ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case use_from_both:
                    if( !( crafter->charges_of( type ) + map_inv.charges_of( type ) >= count ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case use_from_none:
                case cancel:
                    break;
            }
        } else {
            // Counting by units, not charges.
            switch( item_sel.use_from ) {
                case use_from_player:
                    if( !crafter->has_amount( type, count, false, is_crafting_component ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case use_from_map:
                    if( !map_inv.has_components( type, count ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case use_from_both:
                    if( !( crafter->amount_of( type, false, std::numeric_limits<int>::max(),
                                               is_crafting_component ) +
                           map_inv.amount_of( type, false, std::numeric_limits<int>::max(),
                                              is_crafting_component ) >= count ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case use_from_none:
                case cancel:
                    break;
            }
        }
    }

    return missing;
}

std::vector<comp_selection<tool_comp>> craft_command::check_tool_components_missing(
                                        const inventory &map_inv ) const
{
    std::vector<comp_selection<tool_comp>> missing;

    for( const auto &tool_sel : tool_selections ) {
        itype_id type = tool_sel.comp.type;
        if( tool_sel.comp.count > 0 ) {
            long count = tool_sel.comp.count * batch_size;
            switch( tool_sel.use_from ) {
                case use_from_player:
                    if( !crafter->has_charges( type, count ) ) {
                        missing.push_back( tool_sel );
                    }
                    break;
                case use_from_map:
                    if( !map_inv.has_charges( type, count ) ) {
                        missing.push_back( tool_sel );
                    }
                    break;
                case use_from_both:
                case use_from_none:
                case cancel:
                    break;
            }
        } else if( !crafter->has_amount( type, 1 ) && !map_inv.has_tools( type, 1 ) ) {
            missing.push_back( tool_sel );
        }
    }

    return missing;
}
