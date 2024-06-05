#include "advanced_inv_area.h"

#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <utility>

#include "avatar.h"
#include "character.h"
#include "character_attire.h"
#include "debug.h"
#include "enums.h"
#include "field.h"
#include "field_type.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "map.h"
#include "mapdata.h"
#include "mdarray.h"
#include "pimpl.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "uistate.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
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
        return can_store_in_vehicle() ? get_vehicle_stack().size() : 0;
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
    weight( 0_gram ), minimapname( std::move( minimapname ) ), actionname( std::move( actionname ) ),
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
            if( const std::optional<vpart_reference> vp = here.veh_at( pos ).cargo() ) {
                veh = &vp->vehicle();
                vstor = vp->part_index();
                desc[0] = veh->name;
                canputitemsloc = true;
                max_size = MAX_ITEM_IN_VEHICLE_STORAGE;
            } else {
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
            // write "invalid container" by default, in case it
            // doesn't get overwritten in advanced_inv_pane.add_items_from_area()
            desc[0] = _( "Invalid container!" );
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
            const std::optional<vpart_reference> vp = here.veh_at( pos ).cargo();
            if( vp ) {
                veh = &vp->vehicle();
                vstor = vp->part_index();
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

bool advanced_inv_area::canputitems( const item_location &container ) const
{
    return id != AIM_CONTAINER ? canputitemsloc : container && container.get_item()->is_container();
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
    if( const std::optional<vpart_reference> vp = get_map().veh_at( pos ).cargo() ) {
        veh = &vp->vehicle();
        vstor = vp->part_index();
    } else {
        veh = nullptr;
        vstor = -1;
    }
}

aim_location advanced_inv_area::offset_to_location() const
{
    static constexpr cata::mdarray<aim_location, point, 3, 3> loc_array = {
        {AIM_NORTHWEST,     AIM_NORTH,      AIM_NORTHEAST},
        {AIM_WEST,          AIM_CENTER,     AIM_EAST},
        {AIM_SOUTHWEST,     AIM_SOUTH,      AIM_SOUTHEAST}
    };
    return loc_array[off.xy() + point_south_east];
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

vehicle_stack advanced_inv_area::get_vehicle_stack() const
{
    if( !can_store_in_vehicle() ) {
        debugmsg( "advanced_inv_area::get_vehicle_stack when can_store_in_vehicle is false" );
    }
    return veh->get_items( veh->part( vstor ) );
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
        const itype_id id = elem.typeId();
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
