#pragma once
#ifndef CATA_SRC_START_LOCATION_H
#define CATA_SRC_START_LOCATION_H

#include <cstddef>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "enums.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
class player;
class tinymap;
struct tripoint;

class start_location
{
    public:
        start_location_id id;
        bool was_loaded = false;
        void load( const JsonObject &jo, const std::string &src );
        void finalize();
        void check() const;

        std::string name() const;
        int targets_count() const;
        std::pair<std::string, ot_match_type> random_target() const;
        const std::set<std::string> &flags() const;

        /**
         * Find a suitable start location on the overmap.
         * @return Global, absolute overmap terrain coordinates where the player should spawn.
         * It may return `overmap::invalid_tripoint` if no suitable starting location could be found
         * in the world.
         */
        tripoint find_player_initial_location() const;
        /**
         * Initialize the map at players start location using @ref prepare_map.
         * @param omtstart Global overmap terrain coordinates where the player is to be spawned.
         */
        void prepare_map( const tripoint &omtstart ) const;
        /**
         * Place the player somewhere in the reality bubble (g->m).
         */
        void place_player( player &u ) const;
        /**
         * Burn random terrain / furniture with FLAMMABLE or FLAMMABLE_ASH tag.
         * Doors and windows are excluded.
         * @param omtstart Global overmap terrain coordinates where the player is to be spawned.
         * @param rad safe radius area to prevent player spawn next to burning wall.
         * @param count number of fire on the map.
         */
        void burn( const tripoint &omtstart, size_t count, int rad ) const;
        /**
         * Adds a map extra, see map_extras.h and map_extras.cpp. Look at the namespace MapExtras and class map_extras.
         */
        void add_map_extra( const tripoint &omtstart, const std::string &map_extra ) const;

        void handle_heli_crash( player &u ) const;

        /**
         * Adds surround start monsters.
         * @param expected_points Expected value of "monster points" (map tiles times density from @ref map::place_spawns).
         */
        void surround_with_monsters( const tripoint &omtstart, const mongroup_id &type,
                                     float expected_points ) const;
    private:
        translation _name;
        std::vector<std::pair<std::string, ot_match_type>> _omt_types;
        std::set<std::string> _flags;

        void prepare_map( tinymap &m ) const;
};

namespace start_locations
{

void load( const JsonObject &jo, const std::string &src );
void finalize_all();
void check_consistency();
void reset();

const std::vector<start_location> &get_all();

} // namespace start_locations

#endif // CATA_SRC_START_LOCATION_H
