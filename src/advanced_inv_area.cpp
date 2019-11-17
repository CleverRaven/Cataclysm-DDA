#include "auto_pickup.h"
#include "avatar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "item_category.h"
#include "item_search.h"
#include "item_stack.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "player.h"
#include "player_activity.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "trap.h"
#include "ui.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"
#include "inventory.h"
#include "item.h"
#include "enums.h"
#include "item_location.h"
#include "map_selector.h"
#include "pimpl.h"
#include "field.h"
#include "advanced_inv_area.h"

#include "advanced_inv.h"
#include "uistate.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <numeric>


int advanced_inv_area::get_item_count() const
{
    if( id == AIM_INVENTORY ) {
        return g->u.inv.size();
    } else if( id == AIM_WORN ) {
        return g->u.worn.size();
    } else if( id == AIM_ALL ) {
        return 0;
    } else if( id == AIM_DRAGGED ) {
        return can_store_in_vehicle() ? veh->get_items( vstor ).size() : 0;
    } else {
        return g->m.i_at( pos ).size();
    }
}

void advanced_inv_area::init()
{
    pos = g->u.pos() + off;
    veh = nullptr;
    vstor = -1;
    volume = 0_ml;   // must update in main function
    weight = 0_gram; // must update in main function
    switch( id ) {
        case AIM_INVENTORY:
        case AIM_WORN:
            canputitemsloc = true;
            break;
        case AIM_DRAGGED:
            if( g->u.get_grab_type() != OBJECT_VEHICLE ) {
                canputitemsloc = false;
                desc[0] = _( "Not dragging any vehicle!" );
                break;
            }
            // offset for dragged vehicles is not statically initialized, so get it
            off = g->u.grab_point;
            // Reset position because offset changed
            pos = g->u.pos() + off;
            if( const cata::optional<vpart_reference> vp = g->m.veh_at( pos ).part_with_feature( "CARGO",
                    false ) ) {
                veh = &vp->vehicle();
                vstor = vp->part_index();
            } else {
                veh = nullptr;
                vstor = -1;
            }
            if( vstor >= 0 ) {
                desc[0] = veh->name;
                canputitemsloc = true;
                max_size = MAX_ITEM_IN_VEHICLE_STORAGE;
            } else {
                veh = nullptr;
                canputitemsloc = false;
                desc[0] = _( "No dragged vehicle!" );
            }
            break;
        case AIM_CONTAINER:
            // set container position based on location
            set_container_position();
            // location always valid, actual check is done in canputitems()
            // and depends on selected item in pane (if it is valid container)
            canputitemsloc = true;
            if( get_container() == nullptr ) {
                desc[0] = _( "Invalid container!" );
            }
            break;
        case AIM_ALL:
            desc[0] = _( "All 9 squares" );
            canputitemsloc = true;
            break;
        case AIM_SOUTHWEST:
        case AIM_SOUTH:
        case AIM_SOUTHEAST:
        case AIM_WEST:
        case AIM_CENTER:
        case AIM_EAST:
        case AIM_NORTHWEST:
        case AIM_NORTH:
        case AIM_NORTHEAST:
            if( const cata::optional<vpart_reference> vp = g->m.veh_at( pos ).part_with_feature( "CARGO",
                    false ) ) {
                veh = &vp->vehicle();
                vstor = vp->part_index();
            } else {
                veh = nullptr;
                vstor = -1;
            }
            canputitemsloc = can_store_in_vehicle() || g->m.can_put_items_ter_furn( pos );
            max_size = MAX_ITEM_IN_SQUARE;
            if( can_store_in_vehicle() ) {
                desc[1] = vpart_position( *veh, vstor ).get_label().value_or( "" );
            }
            // get graffiti or terrain name
            desc[0] = g->m.has_graffiti_at( pos ) ?
                      g->m.graffiti_at( pos ) : g->m.name( pos );
        default:
            break;
    }

    /* assemble a list of interesting traits of the target square */
    // fields? with a special case for fire
    bool danger_field = false;
    const field &tmpfld = g->m.field_at( pos );
    for( auto &fld : tmpfld ) {
        const field_entry &cur = fld.second;
        if( fld.first.obj().has_fire ) {
            flags.append( _( " <color_white_red>FIRE</color>" ) );
        } else {
            if( cur.is_dangerous() ) {
                danger_field = true;
            }
        }
    }
    if( danger_field ) {
        flags.append( _( " DANGER" ) );
    }

    // trap?
    const trap &tr = g->m.tr_at( pos );
    if( tr.can_see( pos, g->u ) && !tr.is_benign() ) {
        flags.append( _( " TRAP" ) );
    }

    // water?
    static const std::array<ter_id, 8> ter_water = {
        {t_water_dp, t_water_pool, t_swater_dp, t_water_sh, t_swater_sh, t_sewage, t_water_moving_dp, t_water_moving_sh }
    };
    auto ter_check = [this]
    ( const ter_id & id ) {
        return g->m.ter( this->pos ) == id;
    };
    if( std::any_of( ter_water.begin(), ter_water.end(), ter_check ) ) {
        flags.append( _( " WATER" ) );
    }

    // remove leading space
    if( !flags.empty() && flags[0] == ' ' ) {
        flags.erase( 0, 1 );
    }
}


units::volume advanced_inv_area::free_volume( bool in_vehicle ) const
{
    assert( id != AIM_ALL ); // should be a specific location instead
    if( id == AIM_INVENTORY || id == AIM_WORN ) {
        return g->u.volume_capacity() - g->u.volume_carried();
    }
    return in_vehicle ? veh->free_volume( vstor ) : g->m.free_volume( pos );
}

bool advanced_inv_area::is_same( const advanced_inv_area &other ) const
{
    // All locations (sans the below) are compared by the coordinates,
    // e.g. dragged vehicle (to the south) and AIM_SOUTH are the same.
    if( id != AIM_INVENTORY && other.id != AIM_INVENTORY &&
        id != AIM_WORN && other.id != AIM_WORN &&
        id != AIM_CONTAINER && other.id != AIM_CONTAINER ) {
        //     have a vehicle?...     ...do the cargo index and pos match?...    ...at least pos?
        return veh == other.veh ? pos == other.pos && vstor == other.vstor : pos == other.pos;
    }
    //      ...is the id?
    return id == other.id;
}

bool advanced_inv_area::canputitems( const advanced_inv_listitem *advitem )
{
    bool canputitems = false;
    bool from_vehicle = false;
    item *it = nullptr;
    switch( id ) {
        case AIM_CONTAINER:
            if( advitem != nullptr && advitem->is_item_entry() ) {
                it = advitem->items.front();
                from_vehicle = advitem->from_vehicle;
            }
            if( get_container( from_vehicle ) != nullptr ) {
                it = get_container( from_vehicle );
            }
            if( it != nullptr ) {
                canputitems = it->is_watertight_container();
            }
            break;
        default:
            canputitems = canputitemsloc;
            break;
    }
    return canputitems;
}

item *advanced_inv_area::get_container( bool in_vehicle )
{
    item *container = nullptr;

    if( uistate.adv_inv_container_location != -1 ) {
        // try to find valid container in the area
        if( uistate.adv_inv_container_location == AIM_INVENTORY ) {
            const invslice &stacks = g->u.inv.slice();

            // check index first
            if( stacks.size() > static_cast<size_t>( uistate.adv_inv_container_index ) ) {
                auto &it = stacks[uistate.adv_inv_container_index]->front();
                if( is_container_valid( &it ) ) {
                    container = &it;
                }
            }

            // try entire area
            if( container == nullptr ) {
                for( size_t x = 0; x < stacks.size(); ++x ) {
                    auto &it = stacks[x]->front();
                    if( is_container_valid( &it ) ) {
                        container = &it;
                        uistate.adv_inv_container_index = x;
                        break;
                    }
                }
            }
        } else if( uistate.adv_inv_container_location == AIM_WORN ) {
            auto &worn = g->u.worn;
            size_t idx = static_cast<size_t>( uistate.adv_inv_container_index );
            if( worn.size() > idx ) {
                auto iter = worn.begin();
                std::advance( iter, idx );
                if( is_container_valid( &*iter ) ) {
                    container = &*iter;
                }
            }

            // no need to reinvent the wheel
            if( container == nullptr ) {
                auto iter = worn.begin();
                for( size_t i = 0; i < worn.size(); ++i, ++iter ) {
                    if( is_container_valid( &*iter ) ) {
                        container = &*iter;
                        uistate.adv_inv_container_index = i;
                        break;
                    }
                }
            }
        } else {
            map &m = g->m;
            bool is_in_vehicle = veh &&
                                 ( uistate.adv_inv_container_in_vehicle || ( can_store_in_vehicle() && in_vehicle ) );

            const itemstack &stacks = is_in_vehicle ?
                                      i_stacked( veh->get_items( vstor ) ) :
                                      i_stacked( m.i_at( pos ) );

            // check index first
            if( stacks.size() > static_cast<size_t>( uistate.adv_inv_container_index ) ) {
                auto it = stacks[uistate.adv_inv_container_index].front();
                if( is_container_valid( it ) ) {
                    container = it;
                }
            }

            // try entire area
            if( container == nullptr ) {
                for( size_t x = 0; x < stacks.size(); ++x ) {
                    auto it = stacks[x].front();
                    if( is_container_valid( it ) ) {
                        container = it;
                        uistate.adv_inv_container_index = x;
                        break;
                    }
                }
            }
        }

        // no valid container in the area, resetting container
        if( container == nullptr ) {
            set_container( nullptr );
            desc[0] = _( "Invalid container" );
        }
    }

    return container;
}

void advanced_inv_area::set_container( const advanced_inv_listitem *advitem )
{
    if( advitem != nullptr ) {
        item *it( advitem->items.front() );
        uistate.adv_inv_container_location = advitem->area;
        uistate.adv_inv_container_in_vehicle = advitem->from_vehicle;
        uistate.adv_inv_container_index = advitem->idx;
        uistate.adv_inv_container_type = it->typeId();
        uistate.adv_inv_container_content_type = !it->is_container_empty() ?
                it->contents.front().typeId() : "null";
        set_container_position();
    } else {
        uistate.adv_inv_container_location = -1;
        uistate.adv_inv_container_index = 0;
        uistate.adv_inv_container_in_vehicle = false;
        uistate.adv_inv_container_type = "null";
        uistate.adv_inv_container_content_type = "null";
    }
}

bool advanced_inv_area::is_container_valid( const item *it ) const
{
    if( it != nullptr ) {
        if( it->typeId() == uistate.adv_inv_container_type ) {
            if( it->is_container_empty() ) {
                if( uistate.adv_inv_container_content_type == "null" ) {
                    return true;
                }
            } else {
                if( it->contents.front().typeId() == uistate.adv_inv_container_content_type ) {
                    return true;
                }
            }
        }
    }

    return false;
}

static tripoint aim_vector( aim_location id )
{
    switch( id ) {
        case AIM_SOUTHWEST:
            return tripoint_south_west;
        case AIM_SOUTH:
            return tripoint_south;
        case AIM_SOUTHEAST:
            return tripoint_south_east;
        case AIM_WEST:
            return tripoint_west;
        case AIM_EAST:
            return tripoint_east;
        case AIM_NORTHWEST:
            return tripoint_north_west;
        case AIM_NORTH:
            return tripoint_north;
        case AIM_NORTHEAST:
            return tripoint_north_east;
        default:
            return tripoint_zero;
    }
}
void advanced_inv_area::set_container_position()
{
    // update the offset of the container based on location
    if( uistate.adv_inv_container_location == AIM_DRAGGED ) {
        off = g->u.grab_point;
    } else {
        off = aim_vector( static_cast<aim_location>( uistate.adv_inv_container_location ) );
    }
    // update the absolute position
    pos = g->u.pos() + off;
    // update vehicle information
    if( const cata::optional<vpart_reference> vp = g->m.veh_at( pos ).part_with_feature( "CARGO",
            false ) ) {
        veh = &vp->vehicle();
        vstor = vp->part_index();
    } else {
        veh = nullptr;
        vstor = -1;
    }
}


aim_location advanced_inv_area::offset_to_location() const
{
    static aim_location loc_array[3][3] = {
        {AIM_NORTHWEST,     AIM_NORTH,      AIM_NORTHEAST},
        {AIM_WEST,          AIM_CENTER,     AIM_EAST},
        {AIM_SOUTHWEST,     AIM_SOUTH,      AIM_SOUTHEAST}
    };
    return loc_array[off.y + 1][off.x + 1];
}

template <typename T>
advanced_inv_area::itemstack advanced_inv_area::i_stacked( T items )
{
    //create a new container for our stacked items
    advanced_inv_area::itemstack stacks;
    //    // make a list of the items first, so we can add non stacked items back on
    //    std::list<item> items(things.begin(), things.end());
    // used to recall indices we stored `itype_id' item at in itemstack
    std::unordered_map<itype_id, std::set<int>> cache;
    // iterate through and create stacks
    for( auto &elem : items ) {
        const auto id = elem.typeId();
        auto iter = cache.find( id );
        bool got_stacked = false;
        // cache entry exists
        if( iter != cache.end() ) {
            // check to see if it stacks with each item in a stack, not just front()
            for( auto &idx : iter->second ) {
                for( auto &it : stacks[idx] ) {
                    if( ( got_stacked = it->display_stacked_with( elem ) ) ) {
                        stacks[idx].push_back( &elem );
                        break;
                    }
                }
                if( got_stacked ) {
                    break;
                }
            }
        }
        if( !got_stacked ) {
            cache[id].insert( stacks.size() );
            stacks.push_back( { &elem } );
        }
    }
    return stacks;
}
