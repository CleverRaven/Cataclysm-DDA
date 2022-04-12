#pragma once
#ifndef CATA_SRC_START_LOCATION_H
#define CATA_SRC_START_LOCATION_H

#include <cstddef>
#include <iosfwd>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "assign.h"
#include "common_types.h"
#include "coordinates.h"
#include "enums.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
class avatar;
struct city;
class tinymap;

struct start_location_placement_constraints {
    numeric_interval<int> city_size{ 0, INT_MAX };
    numeric_interval<int> city_distance{ 0, INT_MAX };
    numeric_interval<int> allowed_z_levels{ -OVERMAP_DEPTH, OVERMAP_HEIGHT };
};

class start_location
{
    public:
        start_location_id id;
        std::vector<std::pair<start_location_id, mod_id>> src;
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
        tripoint_abs_omt find_player_initial_location( const point_abs_om &origin ) const;
        /**
         * Find a suitable start location on the overmap in specific city.
         * @return Global, absolute overmap terrain coordinates where the player should spawn.
         * It may return `overmap::invalid_tripoint` if no suitable starting location could be found
         * in the world.
         */
        tripoint_abs_omt find_player_initial_location( const city &origin ) const;
        /**
         * Initialize the map at players start location using @ref prepare_map.
         * @param omtstart Global overmap terrain coordinates where the player is to be spawned.
         */
        void prepare_map( const tripoint_abs_omt &omtstart ) const;
        /**
         * Place the player somewhere in the reality bubble (g->m).
         */
        void place_player( avatar &you, const tripoint_abs_omt &omtstart ) const;
        /**
         * Burn random terrain / furniture with FLAMMABLE or FLAMMABLE_ASH tag.
         * Doors and windows are excluded.
         * @param omtstart Global overmap terrain coordinates where the player is to be spawned.
         * @param rad safe radius area to prevent player spawn next to burning wall.
         * @param count number of fire on the map.
         */
        void burn( const tripoint_abs_omt &omtstart, size_t count, int rad ) const;
        /**
         * Adds a map extra, see map_extras.h and map_extras.cpp. Look at the namespace MapExtras and class map_extras.
         */
        void add_map_extra( const tripoint_abs_omt &omtstart, const map_extra_id &map_extra ) const;

        void handle_heli_crash( avatar &you ) const;

        /**
         * Adds surround start monsters.
         * @param expected_points Expected value of "monster points" (map tiles times density from @ref map::place_spawns).
         */
        void surround_with_monsters( const tripoint_abs_omt &omtstart, const mongroup_id &type,
                                     float expected_points ) const;
        const start_location_placement_constraints &get_constraints() const {
            return constraints_;
        }
        /** @returns true if this start location requires a city */
        bool requires_city() const;
        /** @returns whether the start location at specified tripoint can belong to the specified city. */
        bool can_belong_to_city( const tripoint_om_omt &p, const city &cit ) const;
    private:
        translation _name;
        std::vector<std::pair<std::string, ot_match_type>> _omt_types;
        std::set<std::string> _flags;
        start_location_placement_constraints constraints_;

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
