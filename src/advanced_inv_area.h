#pragma once
#ifndef ADVANCED_INV_AREA_H
#define ADVANCED_INV_AREA_H

//#include "advanced_inv.h"
#include "cursesdef.h"
#include "point.h"
#include "units.h"
#include "game.h"
#include "itype.h"
#include "item_stack.h"
#include "vehicle_selector.h"

#include <array>
#include <list>
#include <map>
#include <string>
#include <vector>

enum aim_location {
    AIM_INVENTORY = 0,
    AIM_SOUTHWEST,
    AIM_SOUTH,
    AIM_SOUTHEAST,
    AIM_WEST,
    AIM_CENTER,
    AIM_EAST,
    AIM_NORTHWEST,
    AIM_NORTH,
    AIM_NORTHEAST,
    AIM_DRAGGED,
    AIM_ALL,
    AIM_CONTAINER,
    AIM_WORN,
    NUM_AIM_LOCATIONS,
    // only useful for AIM_ALL
    AIM_AROUND_BEGIN = AIM_SOUTHWEST,
    AIM_AROUND_END = AIM_NORTHEAST
};

struct advanced_inv_listitem;

/**
 * Defines the source of item stacks.
 */
class advanced_inv_area
{
    public:
        // roll our own, to handle moving stacks better
        using itemstack = std::vector<std::list<item *> >;

        const aim_location id;
        // Used for the small overview 3x3 grid
        point hscreen = point_zero;
        // relative (to the player) position of the map point
        tripoint off;
        /** Long name, displayed, translated */
        const std::string name = "fake";
        /** Shorter name (2 letters) */
        const std::string shortname = "FK"; // FK in my coffee
        // absolute position of the map point.
        tripoint pos;
        /** Can we put items there? Only checks if location is valid, not if
            selected container in pane is. For full check use canputitems() **/
        bool canputitemsloc;
        // vehicle pointer and cargo part index
        vehicle *veh;
        int vstor;
        // description, e.g. vehicle name, label, or terrain
        std::array<std::string, 2> desc;
        // flags, e.g. FIRE, TRAP, WATER
        std::string flags;
        // total volume and weight of items currently there
        units::volume volume;
        units::mass weight;
        // maximal count / volume of items there.
        int max_size;

        advanced_inv_area( aim_location id ) : id( id ) {}
        advanced_inv_area( aim_location id, int hscreenx, int hscreeny, tripoint off,
                           const std::string &name, const std::string &shortname ) : id( id ),
            hscreen( hscreenx, hscreeny ), off( off ), name( name ), shortname( shortname ),
            canputitemsloc( false ), veh( nullptr ), vstor( -1 ), volume( 0_ml ),
            weight( 0_gram ), max_size( 0 ) {
        }

        void init();

        template <typename T>
        advanced_inv_area::itemstack i_stacked( T items );
        // if you want vehicle cargo, specify so via `in_vehicle'
        units::volume free_volume( bool in_vehicle = false ) const;
        int get_item_count() const;
        // Other area is actually the same item source, e.g. dragged vehicle to the south and AIM_SOUTH
        bool is_same( const advanced_inv_area &other ) const;
        // does _not_ check vehicle storage, do that with `can_store_in_vehicle()' below
        bool canputitems( const advanced_inv_listitem *advitem = nullptr );
        // if you want vehicle cargo, specify so via `in_vehicle'
        item *get_container( bool in_vehicle = false );
        void set_container( const advanced_inv_listitem *advitem );
        bool is_container_valid( const item *it ) const;
        void set_container_position();
        aim_location offset_to_location() const;
        bool can_store_in_vehicle() const {
            // disallow for non-valid vehicle locations
            if( id > AIM_DRAGGED || id < AIM_SOUTHWEST ) {
                return false;
            }
            return veh != nullptr && vstor >= 0;
        }
};
#endif
