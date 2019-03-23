#pragma once
#ifndef START_LOCATION_H
#define START_LOCATION_H

#include <set>
#include <vector>

#include "string_id.h"

class overmap;
class tinymap;
class player;
class JsonObject;
struct tripoint;
class start_location;
template<typename T>
class generic_factory;
struct MonsterGroup;
using mongroup_id = string_id<MonsterGroup>;

class start_location
{
    public:
        start_location();

        const string_id<start_location> &ident() const;
        std::string name() const;
        std::string target() const;
        const std::set<std::string> &flags() const;

        static void load_location( JsonObject &jo, const std::string &src );
        static void reset();

        static const std::vector<start_location> &get_all();

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
        void burn( const tripoint &omtstart,
                   const size_t count, const int rad ) const;
        /**
         * Adds a map special, see mapgen.h and mapgen.cpp. Look at the namespace MapExtras.
         */
        void add_map_special( const tripoint &omtstart, const std::string &map_special ) const;

        void handle_heli_crash( player &u ) const;

        /**
         * Adds surround start monsters.
         * @param expected_points Expected value of "monster points" (map tiles times density from @ref map::place_spawns).
         */
        void surround_with_monsters( const tripoint &omtstart, const mongroup_id &type,
                                     float expected_points ) const;
    private:
        friend class generic_factory<start_location>;
        string_id<start_location> id;
        bool was_loaded = false;
        std::string _name;
        std::string _target;
        std::set<std::string> _flags;

        void load( JsonObject &jo, const std::string &src );

        void prepare_map( tinymap &m ) const;
};

#endif
