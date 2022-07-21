#include <cstddef>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <unordered_map>
#include <utility>

#include "advanced_inv_area.h"
#include "advanced_inv_listitem.h"
#include "avatar.h"
#include "cata_assert.h"
#include "character.h"
#include "enums.h"
#include "field.h"
#include "field_type.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "map.h"
#include "map_selector.h"
#include "mapdata.h"
#include "optional.h"
#include "pimpl.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "uistate.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"

int advanced_inv_area::get_item_count() const
{
    Character &player_character = get_player_character();
    if( id == AIM_INVENTORY ) {
        return player_character.inv->size();
    } else if( id == AIM_WORN ) {
        return player_character.worn.size();
    } else if( id == AIM_ALL ) {
        return 0;
    } else if( id == AIM_DRAGGED ) {
        return can_store_in_vehicle() ? veh->get_items( vstor ).size() : 0;
    } else {
        return get_map().i_at( pos ).size();
    }
}

advanced_inv_area::advanced_inv_area( aim_location id, const point &h, tripoint off,
                                      const std::string &name, const std::string &shortname,
                                      std::string minimapname, std::string actionname,
                                      aim_location relative_location ) :
    id( id ), hscreen( h ),
    off( off ), name( name ), shortname( shortname ),
    vstor( -1 ), volume( 0_ml ),
    weight( 0_gram ), minimapname( minimapname ), actionname( actionname ),
    relative_location( relative_location )
{
}

void advanced_inv_area::init()
{
    avatar &player_character = get_avatar();
    pos = player_character.pos() + off;
    veh = nullptr;
    vstor = -1;
    // must update in main function
    volume = 0_ml;
    volume_veh = 0_ml;
    // must update in main function
    weight = 0_gram;
    weight_veh = 0_gram;
    map &here = get_map();
    switch( id ) {
        case AIM_INVENTORY:
        case AIM_WORN:
            canputitemsloc = true;
            break;
        case AIM_DRAGGED:
            if( player_character.get_grab_type() != object_type::VEHICLE ) {
                canputitemsloc = false;
                desc[0] = _( "Not dragging any vehicle!" );
                break;
            }
            // offset for dragged vehicles is not statically initialized, so get it
            off = player_character.grab_point;
            // Reset position because offset changed
            pos = player_character.pos() + off;
            if( const cata::optional<vpart_reference> vp = here.veh_at( pos ).part_with_feature( "CARGO",
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
            if( !get_container() ) {
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
        case AIM_NORTHEAST: {
            const cata::optional<vpart_reference> vp =
                here.veh_at( pos ).part_with_feature( "CARGO", false );
            if( vp ) {
                veh = &vp->vehicle();
                vstor = vp->part_index();
            } else {
                veh = nullptr;
                vstor = -1;
            }
            canputitemsloc = can_store_in_vehicle() || here.can_put_items_ter_furn( pos );
            max_size = MAX_ITEM_IN_SQUARE;
            if( can_store_in_vehicle() ) {
                std::string part_name = vp->info().name();
                desc[1] = vp->get_label().value_or( part_name );
            }
            // get graffiti or terrain name
            desc[0] = here.has_graffiti_at( pos ) ?
                      here.graffiti_at( pos ) : here.name( pos );
        }
        default:
            break;
    }

    /* assemble a list of interesting traits of the target square */
    // fields? with a special case for fire
    bool danger_field = false;
    const field &tmpfld = here.field_at( pos );
    for( const auto &fld : tmpfld ) {
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
    const trap &tr = here.tr_at( pos );
    if( tr.can_see( pos, player_character ) && !tr.is_benign() ) {
        flags.append( _( " TRAP" ) );
    }

    // water?
    if( here.has_flag_ter( ter_furn_flag::TFLAG_SHALLOW_WATER, pos ) ||
        here.has_flag_ter( ter_furn_flag::TFLAG_DEEP_WATER, pos ) ) {
        flags.append( _( " WATER" ) );
    }

    // remove leading space
    if( !flags.empty() && flags[0] == ' ' ) {
        flags.erase( 0, 1 );
    }
}

units::volume advanced_inv_area::free_volume( bool in_vehicle ) const
{
    // should be a specific location instead
    cata_assert( id != AIM_ALL );
    if( id == AIM_INVENTORY || id == AIM_WORN ) {
        return get_player_character().free_space();
    }
    return in_vehicle ? veh->free_volume( vstor ) : get_map().free_volume( pos );
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
    switch( id ) {
        case AIM_CONTAINER: {
            item_location it;
            if( advitem != nullptr ) {
                it = advitem->items.front();
                from_vehicle = advitem->from_vehicle;
            }
            if( get_container( from_vehicle ) ) {
                it = get_container( from_vehicle );
            }
            if( it ) {
                canputitems = true;
            }
            break;
        }
        default:
            canputitems = canputitemsloc;
            break;
    }
    return canputitems;
}

item_location outfit::adv_inv_get_container( item_location container, const advanced_inv_area &area,
        Character &guy )
{
    size_t idx = static_cast<size_t>( uistate.adv_inv_container_index );
    if( worn.size() > idx ) {
        auto iter = worn.begin();
        std::advance( iter, idx );
        if( area.is_container_valid( &*iter ) ) {
            container = item_location( guy, &*iter );
        }
    }

    // no need to reinvent the wheel
    if( !container ) {
        auto iter = worn.begin();
        for( size_t i = 0; i < worn.size(); ++i, ++iter ) {
            if( area.is_container_valid( &*iter ) ) {
                container = item_location( guy, &*iter );
                uistate.adv_inv_container_index = i;
                break;
            }
        }
    }
    return container;
}

item_location advanced_inv_area::get_container( bool in_vehicle )
{
    item_location container;

    Character &player_character = get_player_character();
    if( uistate.adv_inv_container_location != -1 ) {
        // try to find valid container in the area
        if( uistate.adv_inv_container_location == AIM_INVENTORY ) {
            const invslice &stacks = player_character.inv->slice();

            // check index first
            if( stacks.size() > static_cast<size_t>( uistate.adv_inv_container_index ) ) {
                auto &it = stacks[uistate.adv_inv_container_index]->front();
                if( is_container_valid( &it ) ) {
                    container = item_location( player_character, &it );
                }
            }

            // try entire area
            if( !container ) {
                for( size_t x = 0; x < stacks.size(); ++x ) {
                    item &it = stacks[x]->front();
                    if( is_container_valid( &it ) ) {
                        container = item_location( player_character, &it );
                        uistate.adv_inv_container_index = x;
                        break;
                    }
                }
            }
        } else if( uistate.adv_inv_container_location == AIM_WORN ) {
            container = player_character.worn.adv_inv_get_container( container, *this, player_character );
        } else {
            map &here = get_map();
            bool is_in_vehicle = veh &&
                                 ( uistate.adv_inv_container_in_vehicle || ( can_store_in_vehicle() && in_vehicle ) );

            const itemstack &stacks = is_in_vehicle ?
                                      i_stacked( veh->get_items( vstor ) ) :
                                      i_stacked( here.i_at( pos ) );

            // check index first
            if( stacks.size() > static_cast<size_t>( uistate.adv_inv_container_index ) ) {
                item *it = stacks[uistate.adv_inv_container_index].front();
                if( is_container_valid( it ) ) {
                    if( is_in_vehicle ) {
                        container = item_location( vehicle_cursor( *veh, vstor ), it );
                    } else {
                        container = item_location( map_cursor( pos ), it );
                    }
                }
            }

            // try entire area
            if( !container ) {
                for( size_t x = 0; x < stacks.size(); ++x ) {
                    item *it = stacks[x].front();
                    if( is_container_valid( it ) ) {
                        if( is_in_vehicle ) {
                            container = item_location( vehicle_cursor( *veh, vstor ), it );
                        } else {
                            container = item_location( map_cursor( pos ), it );
                        }
                        uistate.adv_inv_container_index = x;
                        break;
                    }
                }
            }
        }

        // no valid container in the area, resetting container
        if( !container ) {
            set_container( nullptr );
            desc[0] = _( "Invalid container" );
        }
    }

    return container;
}

void advanced_inv_area::set_container( const advanced_inv_listitem *advitem )
{
    if( advitem != nullptr ) {
        const item_location &it = advitem->items.front();
        uistate.adv_inv_container_location = advitem->area;
        uistate.adv_inv_container_in_vehicle = advitem->from_vehicle;
        uistate.adv_inv_container_index = advitem->idx;
        uistate.adv_inv_container_type = it->typeId();
        uistate.adv_inv_container_content_type = !it->is_container_empty() ? it->legacy_front().typeId() :
                itype_id::NULL_ID();
        set_container_position();
    } else {
        uistate.adv_inv_container_location = -1;
        uistate.adv_inv_container_index = 0;
        uistate.adv_inv_container_in_vehicle = false;
        uistate.adv_inv_container_type = itype_id::NULL_ID();
        uistate.adv_inv_container_content_type = itype_id::NULL_ID();
    }
}

bool advanced_inv_area::is_container_valid( const item *it ) const
{
    if( it != nullptr ) {
        if( it->typeId() == uistate.adv_inv_container_type ) {
            if( it->is_container_empty() ) {
                if( uistate.adv_inv_container_content_type.is_null() ) {
                    return true;
                }
            } else {
                if( it->legacy_front().typeId() == uistate.adv_inv_container_content_type ) {
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
    avatar &player_character = get_avatar();
    // update the offset of the container based on location
    if( uistate.adv_inv_container_location == AIM_DRAGGED ) {
        off = player_character.grab_point;
    } else {
        off = aim_vector( static_cast<aim_location>( uistate.adv_inv_container_location ) );
    }
    // update the absolute position
    pos = player_character.pos() + off;
    // update vehicle information
    if( const cata::optional<vpart_reference> vp = get_map().veh_at( pos ).part_with_feature( "CARGO",
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

bool advanced_inv_area::can_store_in_vehicle() const
{
    // disallow for non-valid vehicle locations
    if( id > AIM_DRAGGED || id < AIM_SOUTHWEST ) {
        return false;
    }
    // Prevent AIM access to activated dishwasher, washing machine, or autoclave.
    if( veh != nullptr && vstor >= 0 ) {
        return !veh->part( vstor ).is_cleaner_on();
    } else {
        return false;
    }
}

template <typename T>
advanced_inv_area::itemstack advanced_inv_area::i_stacked( T items )
{
    //create a new container for our stacked items
    advanced_inv_area::itemstack stacks;
    // used to recall indices we stored `itype_id' item at in itemstack
    std::unordered_map<itype_id, std::set<int>> cache;
    // iterate through and create stacks
    for( item &elem : items ) {
        const auto id = elem.typeId();
        auto iter = cache.find( id );
        bool got_stacked = false;
        // cache entry exists
        if( iter != cache.end() ) {
            // check to see if it stacks with each item in a stack, not just front()
            for( const int &idx : iter->second ) {
                for( item *&it : stacks[idx] ) {
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

// instantiate the template
template
advanced_inv_area::itemstack advanced_inv_area::i_stacked<vehicle_stack>( vehicle_stack items );

template
advanced_inv_area::itemstack advanced_inv_area::i_stacked<map_stack>( map_stack items );
