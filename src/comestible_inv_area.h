#pragma once
#ifndef COMESTIBLE_INV_AREA_H
#define COMESTIBLE_INV_AREA_H

#include "cursesdef.h"
#include "point.h"
#include "units.h"
#include "game.h"
#include "itype.h"

#include <cctype>
#include <cstddef>
#include <array>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>

/**
 * Defines the source of item stacks.
 */
struct comestible_inv_area_info {
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

        AIM_DRAGGED,
        AIM_INVENTORY,
        AIM_WORN,

        AIM_ALL,
        AIM_ALL_I_W,

        NUM_AIM_LOCATIONS
    };



    const aim_location id;
    // Used for the small overview 3x3 grid
    point hscreen;
    // relative (to the player) position of the map point
    tripoint default_offset;

    area_type type;

    /** Long name, displayed, translated */
    const std::string name;
    /** Appears as part of the list */
    const std::string shortname;
    /** Appears as part of the legend at the top right */
    const std::string minimapname;

    const aim_location relative_location;
    const std::string actionname;

    std::vector<aim_location> multi_locations = {};

    bool has_area( aim_location loc ) const {
        return ( std::find( multi_locations.begin(), multi_locations.end(),
                            loc ) != multi_locations.end() );
    }

    comestible_inv_area_info( aim_location id, int hscreenx, int hscreeny, tripoint off,
                              const std::string &name, const std::string &shortname,
                              const std::string &minimapname, const aim_location relative_location,
                              const std::string &actionname ) : id( id ),
        hscreen( hscreenx, hscreeny ), default_offset( off ), name( name ), shortname( shortname ),
        minimapname( minimapname ), relative_location( relative_location ), actionname( actionname ) {
        static const std::array< aim_location, 9> ground_locations = { AIM_SOUTHWEST,
                                                                       AIM_SOUTH,
                                                                       AIM_SOUTHEAST,
                                                                       AIM_WEST,
                                                                       AIM_CENTER,
                                                                       AIM_EAST,
                                                                       AIM_NORTHWEST,
                                                                       AIM_NORTH,
                                                                       AIM_NORTHEAST
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



class comestible_inv_area
{
    public :

    // *INDENT-OFF*
    static comestible_inv_area_info get_info(comestible_inv_area_info::aim_location loc) {
        using i = comestible_inv_area_info;
        static const std::array< comestible_inv_area_info, comestible_inv_area_info::NUM_AIM_LOCATIONS> area_info = { {
            { i::AIM_SOUTHWEST,     30, 3, tripoint_south_west, _("South West"), _("SW"), "1", i::AIM_WEST, "ITEMS_SW" },
            { i::AIM_SOUTH,         33, 3, tripoint_south,      _("South"),      _("S"), "2", i::AIM_SOUTHWEST, "ITEMS_S"},
            { i::AIM_SOUTHEAST,     36, 3, tripoint_south_east, _("South East"), _("SE"), "3", i::AIM_SOUTH, "ITEMS_SE"},
            { i::AIM_WEST,          30, 2, tripoint_west,       _("West"),       _("W"), "4", i::AIM_NORTHWEST, "ITEMS_W" },
            { i::AIM_CENTER,        33, 2, tripoint_zero,       _("Directly below you"), _("DN"), "5", i::AIM_CENTER, "ITEMS_CE"},
            { i::AIM_EAST,          36, 2, tripoint_east,       _("East"),       _("E"), "6", i::AIM_SOUTHEAST, "ITEMS_E"  },
            { i::AIM_NORTHWEST,     30, 1, tripoint_north_west, _("North West"), _("NW"), "7", i::AIM_NORTH, "ITEMS_NW"},
            { i::AIM_NORTH,         33, 1, tripoint_north,      _("North"),      _("N"), "8", i::AIM_NORTHEAST, "ITEMS_N" },
            { i::AIM_NORTHEAST,     36, 1, tripoint_north_east, _("North East"), _("NE"), "9", i::AIM_EAST, "ITEMS_NE" },

            { i::AIM_DRAGGED,       25, 1, tripoint_zero,       _("Grabbed Vehicle"), _("GR"), "D", i::AIM_DRAGGED, "ITEMS_DRAGGED_CONTAINER" },
            { i::AIM_INVENTORY,     25, 2, tripoint_zero,       _("Inventory"),  _("I"), "I", i::AIM_INVENTORY, "ITEMS_INVENTORY" },
            { i::AIM_WORN,          22, 2, tripoint_zero,       _("Worn Items"), _("W"), "W", i::AIM_WORN, "ITEMS_WORN"},

            { i::AIM_ALL,           25, 3, tripoint_zero,       _("Surrounding area"), "", "A", i::AIM_ALL, "ITEMS_AROUND" },
            { i::AIM_ALL_I_W,       18, 3, tripoint_zero,        _("Everything"), "", "A+I+W", i::AIM_ALL_I_W, "ITEMS_AROUND_I_W" }
            }
        };
        return area_info[loc];
    }
    // *INDENT-ON*

        const comestible_inv_area_info info;

    private:

        /** Can we put items there? Only checks if location is valid, not if
            selected container in pane is. For full check use canputitems() **/
        bool is_valid;
        // vehicle pointer and cargo part index
        vehicle *veh;
        int veh_part;

    public:
        // roll our own, to handle moving stacks better
        using area_items = std::vector<std::list<item *> >;

        area_items get_items( bool use_vehicle );

        units::volume get_max_volume( bool use_vehicle );

        std::string get_name( bool use_vehicle ) const;

        bool is_vehicle_default() const;

        // absolute position of the map point.
        //tripoint pos;
        // description, e.g. vehicle name, label, or terrain
        std::array<std::string, 2> desc;
        // flags, e.g. FIRE, TRAP, WATER
        std::string flags;
        // total volume and weight of items currently there
        //units::volume volume;
        //units::mass weight;
        // maximal count / volume of items there.
        //int max_size;

        tripoint offset;


        //comestible_inv_area(aim_location id) : id(id), relative_location(id) {}
        //comestible_inv_area(aim_location id, int hscreenx, int hscreeny, tripoint off,
        //    const std::string& name, const std::string& shortname,
        //    const std::string& minimapname, const aim_location relative_location,
        //    const std::string& actionname) : id(id),
        //    hscreen(hscreenx, hscreeny), off(off), name(name), shortname(shortname),
        //    minimapname(minimapname), relative_location(relative_location), actionname(actionname),
        //    is_valid(false), veh(nullptr), veh_part(-1), volume(0_ml),
        //    weight(0_gram), max_size(0) {
        //}

        comestible_inv_area( comestible_inv_area_info area_info ) : info( area_info ),
            is_valid( false ), veh( nullptr ), veh_part( -1 ) {
        }

        void init();
        // Other area is actually the same item source, e.g. dragged vehicle to the south and AIM_SOUTH
        //bool is_same( const comestible_inv_area &other ) const;
        // does _not_ check vehicle storage, do that with `can_store_in_vehicle()' below
        bool canputitems();
        bool has_vehicle() const {
            //add_msg("XXX veh: %d %d", info.id, info.type);
            // disallow for non-valid vehicle locations
            if( info.type != comestible_inv_area_info::AREA_TYPE_GROUND ) {
                return false;
            }
            return veh != nullptr && veh_part >= 0;
        }

        std::string get_location_key();
        comestible_inv_area_info::aim_location get_relative_location() const {
            if( !( tile_iso && use_tiles ) ) {
                return info.id;
            } else {
                return info.relative_location;
            }
        }
};
#endif
