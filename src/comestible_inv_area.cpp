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
#include "comestible_inv_area.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <numeric>

void comestible_inv_area::init()
{
    offset = info.default_offset;
    tripoint pos = g->u.pos() + offset;
    veh = nullptr;
    veh_part = -1;

    using ai = comestible_inv_area_info;
    switch( info.id ) {
        case ai::AIM_INVENTORY:
        case ai::AIM_WORN:
            is_valid_location = true;
            break;
        case ai::AIM_DRAGGED:
            if( g->u.get_grab_type() != OBJECT_VEHICLE ) {
                is_valid_location = false;
                desc[0] = _( "Not dragging any vehicle!" );
                break;
            }
            // Reset position because offset changed
            pos = g->u.pos() + g->u.grab_point;
            if( const cata::optional<vpart_reference> vp = g->m.veh_at( pos ).part_with_feature( "CARGO",
                    false ) ) {
                veh = &vp->vehicle();
                veh_part = vp->part_index();
            } else {
                veh = nullptr;
                veh_part = -1;
            }
            if( veh_part >= 0 ) {
                desc[0] = veh->name;
                is_valid_location = true;
            } else {
                veh = nullptr;
                is_valid_location = false;
                desc[0] = _( "No dragged vehicle!" );
            }
            break;
        case ai::AIM_ALL:
            desc[0] = _( "All 9 squares" );
            is_valid_location = true;
            break;
        case ai::AIM_ALL_I_W:
            desc[0] = _( "Around,Inv,Worn" );
            is_valid_location = true;
            break;
        case ai::AIM_SOUTHWEST:
        case ai::AIM_SOUTH:
        case ai::AIM_SOUTHEAST:
        case ai::AIM_WEST:
        case ai::AIM_CENTER:
        case ai::AIM_EAST:
        case ai::AIM_NORTHWEST:
        case ai::AIM_NORTH:
        case ai::AIM_NORTHEAST:
            if( const cata::optional<vpart_reference> vp = g->m.veh_at( pos ).part_with_feature( "CARGO",
                    false ) ) {
                veh = &vp->vehicle();
                veh_part = vp->part_index();
            } else {
                veh = nullptr;
                veh_part = -1;
            }
            is_valid_location = has_vehicle() || g->m.can_put_items_ter_furn( pos );
            if( has_vehicle() ) {
                desc[1] = vpart_position( *veh, veh_part ).get_label().value_or( "" );
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
    auto ter_check = [pos]
    ( const ter_id & id ) {
        return g->m.ter( pos ) == id;
    };
    if( std::any_of( ter_water.begin(), ter_water.end(), ter_check ) ) {
        flags.append( _( " WATER" ) );
    }

    // remove leading space
    if( flags.length() && flags[0] == ' ' ) {
        flags.erase( 0, 1 );
    }
}

bool comestible_inv_area::is_valid()
{
    return is_valid_location;
}

inline std::string comestible_inv_area::get_location_key()
{
    return comestible_inv_area::get_info( get_relative_location() ).minimapname;
}

template <typename T>
static comestible_inv_area::area_items mi_stacked( T items )
{
    //create a new container for our stacked items
    comestible_inv_area::area_items stacks;
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

comestible_inv_area::area_items comestible_inv_area::get_items( bool from_cargo )
{
    player &u = g->u;
    comestible_inv_area::area_items retval;

    if( info.id == comestible_inv_area_info::AIM_INVENTORY ) {
        const invslice &stacks = u.inv.slice();

        retval.reserve( stacks.size() );
        for( size_t x = 0; x < stacks.size(); ++x ) {
            std::list<item *> item_pointers;
            for( item &i : *stacks[x] ) {
                item_pointers.push_back( &i );
            }

            retval.push_back( item_pointers );
        }
    } else if( info.id == comestible_inv_area_info::AIM_WORN ) {
        auto iter = u.worn.begin();

        retval.reserve( u.worn.size() );
        for( size_t i = 0; i < u.worn.size(); ++i, ++iter ) {
            std::list<item *> item_pointers = { & *iter };
            retval.push_back( item_pointers );
        }
    } else {
        if( from_cargo ) {
            assert( has_vehicle() );
            retval = mi_stacked( veh->get_items( veh_part ) );
        } else {
            retval = mi_stacked( g->m.i_at( g->u.pos() + offset ) );
        }
    }

    return retval;
}

units::volume comestible_inv_area::get_max_volume( bool use_vehicle )
{
    if( use_vehicle ) {
        assert( has_vehicle() );
        return veh->max_volume( veh_part );
    } else {
        map &m = g->m;
        return m.max_volume( g->u.pos() + offset );
    }
}

std::string comestible_inv_area::get_name( bool use_vehicle ) const
{
    if( use_vehicle ) {
        assert( has_vehicle() );
        return veh->name;
    } else {
        return info.get_name();
    }
}

bool comestible_inv_area::is_vehicle_default() const
{
    if( has_vehicle() ) {
        if( get_relative_location() == comestible_inv_area_info::AIM_DRAGGED ) {
            return true;
        } else {
            // check item stacks in vehicle and map at said square
            auto map_stack = g->m.i_at( g->u.pos() + offset );
            auto veh_stack = veh->get_items( veh_part );
            // auto switch to vehicle storage if vehicle items are there, or neither are there
            return ( !veh_stack.empty() || map_stack.empty() );
        }
    }
    return false;
}
