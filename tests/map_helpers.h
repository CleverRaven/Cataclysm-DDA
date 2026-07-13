#pragma once
#ifndef CATA_TESTS_MAP_HELPERS_H
#define CATA_TESTS_MAP_HELPERS_H

#include <string>

#include "coords_fwd.h"
#include "type_id.h"

class map;
class monster;
class submap;
class time_point;

void wipe_map_terrain( map *target = nullptr );
void clear_creatures();
void clear_npcs();
void clear_fields( int zlevel );
void clear_items( int zlevel );
void clear_zones();
void clear_basecamps();
void clear_map( int zmin = -2, int zmax = 0 );
void clear_radiation();
void clear_map_and_put_player_underground();
monster &spawn_test_monster( const std::string &monster_type, const tripoint_bub_ms &start,
                             bool death_drops = true );
void clear_vehicles( map *target = nullptr );
void build_test_map( const ter_id &terrain );
void build_water_test_map( const ter_id &surface, const ter_id &mid, const ter_id &bottom );
void player_add_headlamp();
void player_wear_blindfold();
void set_time_to_day();
void set_time( const time_point &time );
void clear_overmaps();

class map_meddler
{
    public:
        static bool has_altered_submaps( map &m );
        static submap *unsafe_get_submap_at( tripoint_bub_ms &p, point_sm_ms &l );
};

#endif // CATA_TESTS_MAP_HELPERS_H
