#pragma once
#ifndef ADVANCED_INV_AREA_H
#define ADVANCED_INV_AREA_H

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

/**
 * Defines the source of item stacks.
 */
struct advanced_inv_area_info {
        enum area_type {
            AREA_TYPE_PLAYER,
            AREA_TYPE_GROUND,
            AREA_TYPE_MULTI
        };

        enum aim_location {
            AIM_SOUTHWEST,
            AIM_SOUTH,
            AIM_SOUTHEAST,
            AIM_WEST,
            AIM_CENTER,
            AIM_EAST,
            AIM_NORTHWEST,
            AIM_NORTH,
            AIM_NORTHEAST,

            AIM_INVENTORY,
            AIM_WORN,

            AIM_ALL,
            AIM_ALL_I_W,

            NUM_AIM_LOCATIONS
        };

    public:

        const aim_location id;
        // Used for the small overview 3x3 grid
        point hscreen;
        // relative (to the player) position of the map point
        tripoint default_offset;
    private:
        /** Long name, displayed */
        const std::string name;
        /** Appears as part of the list */
        const std::string shortname;
    public:

        area_type type;

        std::string get_name() const {
            return _( name );
        }
        std::string get_shortname() const {
            return _( shortname );
        }

        /** Appears as part of the legend at the top right */
        const std::string minimapname;

        const aim_location relative_location;
        const std::string actionname;

        std::vector<aim_location> multi_locations = {};

        bool has_area( aim_location loc ) const {
            return ( std::find( multi_locations.begin(), multi_locations.end(),
                                loc ) != multi_locations.end() );
        }

        advanced_inv_area_info( aim_location id, int hscreenx, int hscreeny, tripoint off,
                                const std::string &name, const std::string &shortname,
                                const std::string &minimapname, const aim_location relative_location,
                                const std::string &actionname ) : id( id ),
            hscreen( hscreenx, hscreeny ), default_offset( off ), name( name ), shortname( shortname ),
            minimapname( minimapname ), relative_location( relative_location ), actionname( actionname ) {
            static const std::array< aim_location, 9> ground_locations = { {
                    AIM_SOUTHWEST,
                    AIM_SOUTH,
                    AIM_SOUTHEAST,
                    AIM_WEST,
                    AIM_CENTER,
                    AIM_EAST,
                    AIM_NORTHWEST,
                    AIM_NORTH,
                    AIM_NORTHEAST
                }
            };
            if( id == AIM_ALL ) {
                type = AREA_TYPE_MULTI;
                multi_locations = std::vector<aim_location>( ground_locations.begin(), ground_locations.end() );
            } else if( id == AIM_ALL_I_W ) {
                type = AREA_TYPE_MULTI;
                multi_locations = std::vector<aim_location>( ground_locations.begin(), ground_locations.end() );
                multi_locations.push_back( AIM_INVENTORY );
                multi_locations.push_back( AIM_WORN );
            } else {
                if( id == AIM_INVENTORY || id == AIM_WORN ) {
                    type = AREA_TYPE_PLAYER;
                } else {
                    type = AREA_TYPE_GROUND;
                }
                multi_locations = { id };
            }
        }
};

class advanced_inv_area
{
    public :

        // *INDENT-OFF*
        static advanced_inv_area_info get_info(advanced_inv_area_info::aim_location loc) {
            using i = advanced_inv_area_info;
            static const std::array< advanced_inv_area_info, advanced_inv_area_info::NUM_AIM_LOCATIONS> area_info = { {
                { i::AIM_SOUTHWEST,     30, 3, tripoint_south_west, translate_marker("South West"), translate_marker("SW"), "1", i::AIM_WEST, "ITEMS_SW" },
                { i::AIM_SOUTH,         33, 3, tripoint_south,      translate_marker("South"),      translate_marker("S"), "2", i::AIM_SOUTHWEST, "ITEMS_S"},
                { i::AIM_SOUTHEAST,     36, 3, tripoint_south_east, translate_marker("South East"), translate_marker("SE"), "3", i::AIM_SOUTH, "ITEMS_SE"},
                { i::AIM_WEST,          30, 2, tripoint_west,       translate_marker("West"),       translate_marker("W"), "4", i::AIM_NORTHWEST, "ITEMS_W" },
                { i::AIM_CENTER,        33, 2, tripoint_zero,       translate_marker("Directly below you"), translate_marker("DN"), "5", i::AIM_CENTER, "ITEMS_CE"},
                { i::AIM_EAST,          36, 2, tripoint_east,       translate_marker("East"),       translate_marker("E"), "6", i::AIM_SOUTHEAST, "ITEMS_E"  },
                { i::AIM_NORTHWEST,     30, 1, tripoint_north_west, translate_marker("North West"), translate_marker("NW"), "7", i::AIM_NORTH, "ITEMS_NW"},
                { i::AIM_NORTH,         33, 1, tripoint_north,      translate_marker("North"),      translate_marker("N"), "8", i::AIM_NORTHEAST, "ITEMS_N" },
                { i::AIM_NORTHEAST,     36, 1, tripoint_north_east, translate_marker("North East"), translate_marker("NE"), "9", i::AIM_EAST, "ITEMS_NE" },

                { i::AIM_INVENTORY,     25, 2, tripoint_zero,       translate_marker("Inventory"),  translate_marker("I"), "I", i::AIM_INVENTORY, "ITEMS_INVENTORY" },
                { i::AIM_WORN,          22, 2, tripoint_zero,       translate_marker("Worn Items"), translate_marker("W"), "W", i::AIM_WORN, "ITEMS_WORN"},

                { i::AIM_ALL,           25, 3, tripoint_zero,       translate_marker("Surrounding area"), "", "A", i::AIM_ALL, "ITEMS_AROUND" },
                { i::AIM_ALL_I_W,       18, 3, tripoint_zero,       translate_marker("Everything"), "", "A+I+W", i::AIM_ALL_I_W, "ITEMS_AROUND_I_W" }
                }
            };
            return area_info[loc];
        }
        // *INDENT-ON*

        const advanced_inv_area_info info;

    private:

        /** false if blocked by somehting (like a wall)  **/
        bool is_valid_location;
        // vehicle pointer and cargo part index
        vehicle *veh;
        int veh_part;

    public:
        // description, e.g. vehicle name, label, or terrain
        std::array<std::string, 2> desc;
        // flags, e.g. FIRE, TRAP, WATER
        std::string flags;

        // use this instead of info.offset, because 'D'ragged containers change original
        tripoint offset;

        // roll our own, to handle moving stacks better
        using area_items = std::vector<std::list<item *> >;

        advanced_inv_area( advanced_inv_area_info area_info ) : info( area_info ),
            is_valid_location( false ), veh( nullptr ), veh_part( -1 ) {
        }

        void init();
        /** false if blocked by somehting (like a wall)  **/
        bool is_valid() const;

        std::string get_name( bool use_vehicle ) const;
        area_items get_items( bool from_cargo ) const;
        void get_item_iterators( bool from_cargo, item_stack::iterator &stack_begin,
                                 item_stack::iterator &stack_end );
        units::volume free_volume( bool in_vehicle ) const;
        units::volume get_max_volume( bool use_vehicle );
        int get_item_count( bool use_vehicle ) const;
        int get_max_items( bool use_vehicle ) const;

        // whether to show vehicle when we open location for the first time
        bool is_vehicle_default() const;
        // used py pane to display locations grid at the top right
        std::string get_location_key();

        item_location generate_item_location( bool use_vehicle, item *it );

        bool has_vehicle() const {
            // disallow for non-valid vehicle locations
            if( info.type != advanced_inv_area_info::AREA_TYPE_GROUND ) {
                return false;
            }
            return veh != nullptr && veh_part >= 0;
        }

        bool is_same( const advanced_inv_area &other ) const {
            return info.id == other.info.id;
        }

        advanced_inv_area_info::aim_location get_relative_location() const {
            if( !( tile_iso && use_tiles ) ) {
                return info.id;
            } else {
                return info.relative_location;
            }
        }
};
#endif
