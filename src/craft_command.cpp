#include "craft_command.h"

#include <limits.h>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <limits>
#include <list>
#include <map>
#include <utility>

#include "debug.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "json.h"
#include "output.h"
#include "player.h"
#include "recipe.h"
#include "requirements.h"
#include "translations.h"
#include "uistate.h"
#include "type_id.h"

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
        case num_usages:
            break;
    }

    return item::nname( comp.type, comp.count );
}

namespace io
{

template<>
std::string enum_to_string<usage>( usage data )
{
    switch( data ) {
        // *INDENT-OFF*
        case usage::use_from_map: return "map";
        case usage::use_from_player: return "player";
        case usage::use_from_both: return "both";
        case usage::use_from_none: return "none";
        case usage::cancel: return "cancel";
        // *INDENT-ON*
        case usage::num_usages:
            break;
    }
    debugmsg( "Invalid usage" );
    abort();
}

} // namespace io

template<typename CompType>
void comp_selection<CompType>::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "use_from", io::enum_to_string( use_from ) );
    jsout.member( "type", comp.type );
    jsout.member( "count", comp.count );

    jsout.end_object();
}

template<typename CompType>
void comp_selection<CompType>::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();

    std::string use_from_str;
    data.read( "use_from", use_from_str );
    use_from = io::string_to_enum<usage>( use_from_str );
    data.read( "type", comp.type );
    data.read( "count", comp.count );
}

template void comp_selection<tool_comp>::serialize( JsonOut &jsout ) const;
template void comp_selection<item_comp>::serialize( JsonOut &jsout ) const;
template void comp_selection<tool_comp>::deserialize( JsonIn &jsin );
template void comp_selection<item_comp>::deserialize( JsonIn &jsin );

void craft_command::execute( const tripoint &new_loc )
{
    if( empty() ) {
        return;
    }

    if( new_loc != tripoint_zero ) {
        loc = new_loc;
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
        if( !crafter->can_make( rec, batch_size ) ) {
            if( crafter->can_start_craft( rec, batch_size ) ) {
                if( !query_yn( _( "You don't have enough charges to complete the %s.\n"
                                  "Start crafting anyway?" ), rec->result_name() ) ) {
                    return;
                }
            } else {
                debugmsg( "Tried to start craft without sufficient charges" );
                return;
            }
        }

        item_selections.clear();
        const auto needs = rec->requirements();
        const auto filter = rec->get_component_filter();

        for( const auto &it : needs.get_components() ) {
            comp_selection<item_comp> is = crafter->select_item_component( it, batch_size, map_inv, true,
                                           filter );
            if( is.use_from == cancel ) {
                return;
            }
            item_selections.push_back( is );
        }

        tool_selections.clear();
        for( const auto &it : needs.get_tools() ) {
            comp_selection<tool_comp> ts = crafter->select_tool_component(
            it, batch_size, map_inv, DEFAULT_HOTKEYS, true, true, []( int charges ) {
                return charges / 20 + charges % 20;
            } );
            if( ts.use_from == cancel ) {
                return;
            }
            tool_selections.push_back( ts );
        }
    }

    crafter->start_craft( *this, loc );
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

item craft_command::create_in_progress_craft()
{
    // Use up the components and tools
    std::list<item> used;
    std::vector<item_comp> comps_used;
    if( crafter->has_trait( trait_id( "DEBUG_HS" ) ) ) {
        return item( rec, batch_size, used, comps_used );
    }

    if( empty() ) {
        debugmsg( "Warning: attempted to consume items from an empty craft_command" );
        return item();
    }

    inventory map_inv;
    map_inv.form_from_map( crafter->pos(), PICKUP_RANGE );

    if( !check_item_components_missing( map_inv ).empty() ) {
        debugmsg( "Aborting crafting: couldn't find cached components" );
        return item();
    }

    const auto filter = rec->get_component_filter();

    for( const auto &it : item_selections ) {
        std::list<item> tmp = crafter->consume_items( it, batch_size, filter );
        used.splice( used.end(), tmp );
    }

    for( const comp_selection<item_comp> &selection : item_selections ) {
        item_comp comp_used = selection.comp;
        comp_used.count *= batch_size;
        comps_used.emplace_back( comp_used );
    }

    item new_craft( rec, batch_size, used, comps_used );

    new_craft.set_cached_tool_selections( tool_selections );
    new_craft.set_tools_to_continue( true );
    // Pass true to indicate that we are starting the craft and the remainder should be consumed as well
    crafter->craft_consume_tools( new_craft, 1, true );
    new_craft.set_next_failure_point( *crafter );

    return new_craft;
}

std::vector<comp_selection<item_comp>> craft_command::check_item_components_missing(
                                        const inventory &map_inv ) const
{
    std::vector<comp_selection<item_comp>> missing;

    const auto filter = rec->get_component_filter();

    for( const auto &item_sel : item_selections ) {
        itype_id type = item_sel.comp.type;
        const item_comp component = item_sel.comp;
        const int count = component.count > 0 ? component.count * batch_size : abs( component.count );

        if( item::count_by_charges( type ) && count > 0 ) {
            switch( item_sel.use_from ) {
                case use_from_player:
                    if( !crafter->has_charges( type, count, filter ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case use_from_map:
                    if( !map_inv.has_charges( type, count, filter ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case use_from_both:
                    if( !( crafter->charges_of( type, INT_MAX, filter ) +
                           map_inv.charges_of( type, INT_MAX, filter ) >= count ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case use_from_none:
                case cancel:
                case num_usages:
                    break;
            }
        } else {
            // Counting by units, not charges.
            switch( item_sel.use_from ) {
                case use_from_player:
                    if( !crafter->has_amount( type, count, false, filter ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case use_from_map:
                    if( !map_inv.has_components( type, count, filter ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case use_from_both:
                    if( !( crafter->amount_of( type, false, std::numeric_limits<int>::max(), filter ) +
                           map_inv.amount_of( type, false, std::numeric_limits<int>::max(), filter ) >= count ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case use_from_none:
                case cancel:
                case num_usages:
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
            const int count = tool_sel.comp.count * batch_size;
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
                case num_usages:
                    break;
            }
        } else if( !crafter->has_amount( type, 1 ) && !map_inv.has_tools( type, 1 ) ) {
            missing.push_back( tool_sel );
        }
    }

    return missing;
}
