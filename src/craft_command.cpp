#include "craft_command.h"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <functional>
#include <limits>
#include <list>
#include <string>
#include <utility>

#include "activity_actor_definitions.h"
#include "character.h"
#include "crafting.h"
#include "debug.h"
#include "enum_conversions.h"
#include "enum_traits.h"
#include "flag.h"
#include "flexbuffer_json.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "item_components.h"
#include "item_contents.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "output.h"
#include "pocket_type.h"
#include "recipe.h"
#include "requirements.h"
#include "translations.h"
#include "type_id.h"
#include "uistate.h"
#include "vehicle.h"
#include "visitable.h"
#include "vpart_position.h"

static const itype_id itype_candle( "candle" );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );

template<typename CompType>
std::string comp_selection<CompType>::nname() const
{
    switch( use_from ) {
        case usage_from::map:
            return item::nname( comp.type, comp.count ) + _( " (nearby)" );
        case usage_from::both:
            return item::nname( comp.type, comp.count ) + _( " (person & nearby)" );
        case usage_from::player:
        // Is the same as the default return;
        case usage_from::none:
        case usage_from::cancel:
        case usage_from::num_usages_from:
            break;
    }

    return item::nname( comp.type, comp.count );
}

namespace io
{

template<>
std::string enum_to_string<usage_from>( usage_from data )
{
    switch( data ) {
        // *INDENT-OFF*
        case usage_from::map: return "map";
        case usage_from::player: return "player";
        case usage_from::both: return "both";
        case usage_from::none: return "none";
        case usage_from::cancel: return "cancel";
        // *INDENT-ON*
        case usage_from::num_usages_from:
            break;
    }
    cata_fatal( "Invalid usage" );
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
void comp_selection<CompType>::deserialize( const JsonObject &data )
{
    std::string use_from_str;
    data.read( "use_from", use_from_str );
    use_from = io::string_to_enum<usage_from>( use_from_str );
    data.read( "type", comp.type );
    data.read( "count", comp.count );
}

template void comp_selection<tool_comp>::serialize( JsonOut &jsout ) const;
template void comp_selection<item_comp>::serialize( JsonOut &jsout ) const;
template void comp_selection<tool_comp>::deserialize( const JsonObject &data );
template void comp_selection<item_comp>::deserialize( const JsonObject &data );

void craft_command::execute( const std::optional<tripoint_bub_ms> &new_loc )
{
    loc = new_loc;
    execute();
}

void craft_command::execute( bool only_cache_comps )
{
    if( empty() ) {
        return;
    }

    bool need_selections = true;
    inventory map_inv;
    map_inv.form_from_map( crafter->pos_bub(), PICKUP_RANGE, crafter );

    if( has_cached_selections() ) {
        std::vector<comp_selection<item_comp>> missing_items = check_item_components_missing( map_inv );
        std::vector<comp_selection<tool_comp>> missing_tools = check_tool_components_missing( map_inv );

        if( missing_items.empty() && missing_tools.empty() ) {
            // All items we used previously are still there, so we don't need to do selection.
            need_selections = false;
        } else if( !only_cache_comps && !query_continue( missing_items, missing_tools ) ) {
            return;
        }
    }

    if( need_selections ) {
        if( !crafter->can_make( rec, batch_size ) ) {
            if( crafter->can_start_craft( rec, recipe_filter_flags::none, batch_size ) ) {
                if( !query_yn( _( "You don't have enough charges to complete the %s.\n"
                                  "Start crafting anyway?" ), rec->result_name() ) ) {
                    return;
                }
            } else if( !rec->character_has_required_proficiencies( *crafter ) ) {
                popup( _( "You don't have the required proficiencies to craft this!" ) );
                return;
            } else {
                debugmsg( "Tried to start craft without sufficient charges" );
                return;
            }
        }

        flags = recipe_filter_flags::no_rotten;

        if( !crafter->can_start_craft( rec, flags, batch_size ) ) {
            if( !query_yn( _( "This craft will use rotten components.\n"
                              "Start crafting anyway?" ) ) ) {
                return;
            }
            flags = recipe_filter_flags::none;
        }

        flags |= recipe_filter_flags::no_favorite;
        if( !crafter->can_start_craft( rec, flags, batch_size ) ) {
            if( !query_yn( _( "This craft will use favorited components.\n"
                              "Start crafting anyway?" ) ) ) {
                return;
            }
            flags = flags & recipe_filter_flags::no_rotten ? recipe_filter_flags::no_rotten :
                    recipe_filter_flags::none;
        }

        item_selections.clear();
        const auto filter = rec->get_component_filter( flags );
        const requirement_data *needs = rec->deduped_requirements().select_alternative(
                                            *crafter, filter, batch_size, craft_flags::start_only );
        if( !needs ) {
            return;
        }

        for( const auto &it : needs->get_components() ) {
            comp_selection<item_comp> is =
                crafter->select_item_component( it, batch_size, map_inv, true, filter, true, true, rec );
            if( is.use_from == usage_from::cancel ) {
                return;
            }
            item_selections.push_back( is );
        }

        tool_selections.clear();
        for( const auto &it : needs->get_tools() ) {
            comp_selection<tool_comp> ts = crafter->select_tool_component(
            it, batch_size, map_inv, true, true, true, []( int charges ) {
                return charges / 20 + charges % 20;
            } );
            if( ts.use_from == usage_from::cancel ) {
                return;
            }
            tool_selections.push_back( ts );
        }
    }

    if( only_cache_comps ) {
        return;
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
static std::string component_list_string( const std::vector<comp_selection<T>> &components )
{
    return enumerate_as_string( components.begin(), components.end(),
    []( const comp_selection<T> &comp ) {
        return comp.nname();
    } );
}

bool craft_command::query_continue( const std::vector<comp_selection<item_comp>> &missing_items,
                                    const std::vector<comp_selection<tool_comp>> &missing_tools )
{
    std::string ss = _( "Some components used previously are missing.  Continue?" );

    if( !missing_items.empty() ) {
        ss += "\n";
        ss += _( "Item(s): " );
        ss += component_list_string( missing_items );
    }

    if( !missing_tools.empty() ) {
        ss += "\n";
        ss += _( "Tool(s): " );
        ss += component_list_string( missing_tools );
    }

    return query_yn( ss );
}

bool craft_command::continue_prompt_liquids( const std::function<bool( const item & )> &filter,
        bool no_prompt )
{
    map &m = get_map();
    for( const auto &it : item_selections ) {
        const std::vector<pocket_data> it_pkt = it.comp.type->pockets;
        if( ( item::count_by_charges( it.comp.type ) && it.comp.count > 0 ) ||
        !std::any_of( it_pkt.begin(), it_pkt.end(), []( const pocket_data & p ) {
        return p.type == pocket_type::CONTAINER && p.watertight;
    } ) ) {
            continue;
        }

        // Everything below only occurs for item components that are liquid containers
        std::function<bool( const item & )> empty_filter = [&filter]( const item & it ) {
            return it.empty_container() && filter( it );
        };

        const char *liq_cont_msg = _( "%1$s is not empty.  Continue anyway?" );
        std::vector<std::pair<const tripoint_bub_ms, item>> map_items;
        std::vector<std::pair<const vpart_reference, item>> veh_items;
        std::vector<item> inv_items;

        auto reset_items = [&]() {
            for( auto &mit : map_items ) {
                m.add_item_or_charges( mit.first, mit.second );
            }
            for( item &iit : inv_items ) {
                crafter->i_add_or_drop( iit );
            }
            for( auto &vit : veh_items ) {
                vit.first.vehicle().add_item( m, vit.first.part(), vit.second );
            }
        };

        int real_count = ( it.comp.count > 0 ) ? it.comp.count * batch_size : std::abs( it.comp.count );
        for( int i = 0; i < 2 && real_count > 0; i++ ) {
            if( it.use_from & usage_from::map ) {
                const tripoint_bub_ms &loc = crafter->pos_bub();
                for( int radius = 0; radius <= PICKUP_RANGE && real_count > 0; radius++ ) {
                    for( const tripoint_bub_ms &p : m.points_in_radius( loc, radius ) ) {
                        if( rl_dist( loc, p ) >= radius ) {
                            // "Simulate" consuming items and put them back
                            // not very efficient but should be rare enough not to matter
                            std::list<item> tmp = m.use_amount_square( p, it.comp.type, real_count,
                                                  i == 0 ? empty_filter : filter );
                            bool cont_not_empty = false;
                            std::string iname;
                            for( item &tmp_i : tmp ) {
                                if( !tmp_i.empty_container() ) {
                                    cont_not_empty = true;
                                    iname = tmp_i.tname( 1U, true );
                                }
                                if( const std::optional<vpart_reference> vp = m.veh_at( p ).cargo() ) {
                                    veh_items.emplace_back( vp.value(), tmp_i );
                                } else {
                                    map_items.emplace_back( p, tmp_i );
                                }
                            }
                            if( cont_not_empty && ( no_prompt || !query_yn( liq_cont_msg, iname ) ) ) {
                                reset_items();
                                return false;
                            }
                        }
                    }
                }
            }
            if( it.use_from & usage_from::player && real_count > 0 ) {
                // "Simulate" consuming items and put them back
                // not very efficient but should be rare enough not to matter
                std::list<item> tmp = crafter->use_amount( it.comp.type, real_count,
                                      i == 0 ? empty_filter : filter );
                real_count -= tmp.size();
                bool cont_not_empty = false;
                std::string iname;
                for( item &tmp_i : tmp ) {
                    if( !tmp_i.empty_container() ) {
                        cont_not_empty = true;
                        iname = tmp_i.tname( 1U, true );
                    }
                    inv_items.emplace_back( tmp_i );
                }
                if( cont_not_empty && ( no_prompt || !query_yn( liq_cont_msg, iname ) ) ) {
                    reset_items();
                    return false;
                }
            }
        }
        reset_items();
    }
    return true;
}

static std::list<item> sane_consume_items( const comp_selection<item_comp> &it, Character *crafter,
        int batch, const std::function<bool( const item & )> &filter )
{
    map &m = get_map();
    const std::vector<pocket_data> it_pkt = it.comp.type->pockets;
    if( ( item::count_by_charges( it.comp.type ) && it.comp.count > 0 ) ||
    !std::any_of( it_pkt.begin(), it_pkt.end(), []( const pocket_data & p ) {
    return p.type == pocket_type::CONTAINER && p.watertight;
} ) ) {
        std::list<item> consumed = crafter->consume_items( it, batch, filter );
        return consumed;
    }

    // Everything below only occurs for item components that are liquid containers
    std::function<bool( const item & )> empty_filter = [&filter]( const item & it ) {
        return it.empty_container() && filter( it );
    };

    int real_count = ( it.comp.count > 0 ) ? it.comp.count * batch : std::abs( it.comp.count );
    std::list<item> ret;
    for( int i = 0; i < 2 && real_count > 0; i++ ) {
        if( it.use_from & usage_from::map ) {
            const tripoint_bub_ms &loc = crafter->pos_bub();
            for( int radius = 0; radius <= PICKUP_RANGE && real_count > 0; radius++ ) {
                for( const tripoint_bub_ms &p : m.points_in_radius( loc, radius ) ) {
                    if( rl_dist( loc, p ) >= radius ) {
                        std::list<item> tmp = m.use_amount_square( p, it.comp.type, real_count,
                                              i == 0 ? empty_filter : filter );
                        ret.insert( ret.end(), tmp.begin(), tmp.end() );
                    }
                }
            }
        }
        if( it.use_from & usage_from::player && real_count > 0 ) {
            std::list<item> tmp = crafter->use_amount( it.comp.type, real_count,
                                  i == 0 ? empty_filter : filter );
            real_count -= tmp.size();
            ret.insert( ret.end(), tmp.begin(), tmp.end() );
        }
    }
    return ret;
}

bool craft_command::safe_to_unload_comp( const item &it )
{
    // Candle wax from candles should be consumed with the candle
    if( it.is_container_empty() || it.typeId() == itype_candle ) {
        return false;
    }

    // Return true if item contains anything "real"
    const std::function<bool( const item &i )> filter = []( const item & i ) {
        return !i.has_flag( flag_ZERO_WEIGHT ) && !i.has_flag( flag_NO_DROP );
    };
    const bool valid = it.get_contents().has_any_with( filter, pocket_type::CONTAINER ) ||
                       it.get_contents().has_any_with( filter, pocket_type::MAGAZINE ) ||
                       it.get_contents().has_any_with( filter, pocket_type::MAGAZINE_WELL );
    if( valid ) {
        return true;
    }

    // Don't try to unload items that shouldn't be outside their container
    itype_id ammo = it.loaded_ammo().typeId();
    if( ammo.is_null() ) {
        return !it.empty_container();
    } else if( ammo->has_flag( flag_ZERO_WEIGHT ) ||
               ammo->has_flag( flag_NO_DROP ) ) {
        return false;
    }

    return true;
}

item craft_command::create_in_progress_craft()
{
    // Use up the components and tools
    item_components used;
    std::vector<item_comp> comps_used;
    if( crafter->has_trait( trait_DEBUG_HS ) ) {
        return item( rec, batch_size, used, comps_used );
    }

    if( empty() ) {
        debugmsg( "Warning: attempted to consume items from an empty craft_command" );
        return item();
    }

    inventory map_inv;
    map_inv.form_from_map( crafter->pos_bub(), PICKUP_RANGE, crafter );

    if( !check_item_components_missing( map_inv ).empty() ) {
        debugmsg( "Aborting crafting: couldn't find cached components" );
        return item();
    }

    const auto filter = rec->get_component_filter( flags );

    if( crafter->is_avatar() &&
        !continue_prompt_liquids( filter ) ) {
        // player cancelled when prompted to unload liquid
        return item();
    }

    for( const auto &it : item_selections ) {
        std::list<item> tmp = sane_consume_items( it, crafter, batch_size, filter );
        for( item &tmp_it : tmp ) {
            if( safe_to_unload_comp( tmp_it ) ) {
                item_location tmp_loc( *crafter, &tmp_it );
                unload_activity_actor::unload( *crafter, tmp_loc );
            }
        }
        for( item &it : tmp ) {
            used.add( it );
        }
    }

    for( const comp_selection<item_comp> &selection : item_selections ) {
        item_comp comp_used = selection.comp;
        comp_used.count *= batch_size;

        //Handle duplicate component requirement
        auto found_it = std::find_if( comps_used.begin(),
        comps_used.end(), [&comp_used]( const item_comp & c ) {
            return c.type == comp_used.type;
        } );
        if( found_it != comps_used.end() ) {
            item_comp &found_comp = *found_it;
            found_comp.count += comp_used.count;
        } else {
            comps_used.emplace_back( comp_used );
        }
    }

    item new_craft( rec, batch_size, used, comps_used );

    new_craft.set_cached_tool_selections( tool_selections );
    new_craft.set_tools_to_continue( true );
    // Pass true to indicate that we are starting the craft and the remainder should be consumed as well
    crafter->craft_consume_tools( new_craft, 1, true );
    new_craft.set_next_failure_point( *crafter );
    new_craft.set_owner( *crafter );

    return new_craft;
}

skill_id craft_command::get_skill_id()
{
    return rec->skill_used;
}

std::vector<comp_selection<item_comp>> craft_command::check_item_components_missing(
                                        const read_only_visitable &map_inv ) const
{
    std::vector<comp_selection<item_comp>> missing;

    const auto filter = rec->get_component_filter( flags );

    for( const auto &item_sel : item_selections ) {
        itype_id type = item_sel.comp.type;
        const item_comp component = item_sel.comp;
        const int count = component.count > 0 ? component.count * batch_size : std::abs( component.count );

        if( item::count_by_charges( type ) && count > 0 ) {
            switch( item_sel.use_from ) {
                case usage_from::player:
                    if( !crafter->has_charges( type, count, filter ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case usage_from::map:
                    if( !map_inv.has_charges( type, count, filter ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case usage_from::both:
                    if( !( crafter->charges_of( type, INT_MAX, filter ) +
                           map_inv.charges_of( type, INT_MAX, filter ) >= count ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case usage_from::none:
                case usage_from::cancel:
                case usage_from::num_usages_from:
                    break;
            }
        } else {
            // Counting by units, not charges.
            switch( item_sel.use_from ) {
                case usage_from::player:
                    if( !crafter->has_amount( type, count, false, filter ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case usage_from::map:
                    if( !map_inv.has_components( type, count, filter ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case usage_from::both:
                    if( !( crafter->amount_of( type, false, std::numeric_limits<int>::max(), filter ) +
                           map_inv.amount_of( type, false, std::numeric_limits<int>::max(), filter ) >= count ) ) {
                        missing.push_back( item_sel );
                    }
                    break;
                case usage_from::none:
                case usage_from::cancel:
                case usage_from::num_usages_from:
                    break;
            }
        }
    }

    return missing;
}

std::vector<comp_selection<tool_comp>> craft_command::check_tool_components_missing(
                                        const read_only_visitable &map_inv ) const
{
    std::vector<comp_selection<tool_comp>> missing;

    for( const auto &tool_sel : tool_selections ) {
        itype_id type = tool_sel.comp.type;
        if( tool_sel.comp.count > 0 ) {
            const int count = tool_sel.comp.count * batch_size;
            switch( tool_sel.use_from ) {
                case usage_from::player:
                    if( !crafter->has_charges( type, count ) ) {
                        missing.push_back( tool_sel );
                    }
                    break;
                case usage_from::map:
                    if( !map_inv.has_charges( type, count ) ) {
                        missing.push_back( tool_sel );
                    }
                    break;
                case usage_from::both:
                case usage_from::none:
                case usage_from::cancel:
                case usage_from::num_usages_from:
                    break;
            }
        } else if( !crafter->has_amount( type, 1 ) && !map_inv.has_tools( type, 1 ) ) {
            missing.push_back( tool_sel );
        }
    }

    return missing;
}
