#pragma once
#ifndef CATA_TESTS_MAP_HELPERS_H
#define CATA_TESTS_MAP_HELPERS_H

#include <string>
#include <utility>
#include <vector>

#include "coordinates.h"
#include "type_id.h"

class map;
class monster;
class submap;
class time_point;

monster &spawn_test_monster( const std::string &monster_type, const tripoint_bub_ms &start,
                             bool death_drops = true );
void build_test_map( const ter_id &terrain );
void build_water_test_map( const ter_id &surface, const ter_id &mid, const ter_id &bottom );
void player_add_headlamp();
void player_wear_blindfold();
void set_time_to_day();
void set_time( const time_point &time );

using los_pairs = std::vector<std::pair<tripoint_bub_ms, tripoint_bub_ms>>;

// center paired with each point of the square ring of given radius
los_pairs los_pairs_around( const tripoint_bub_ms &center, int radius );

// caches vision_cache_oracle compares
enum class vision_layers {
    // outside, floor, transparency, sight, vision transparency, seen and
    // camera caches, plus pairwise sight answers on both traces
    scene_and_fov,
    // also light on every level, as a reader asking for it would find it
    light,
    // and final visibility classification on the levels the request reads
    all,
};

// Compares vision caches an incremental build left with a rebuild of the same
// scene from scratch.
class vision_cache_oracle
{
    public:
        explicit vision_cache_oracle( los_pairs pairs );
        // asks every pair once, so pairwise caches hold answers a later mutation
        // can leave stale
        void prime() const;
        // call after the incremental build following a mutation; final
        // visibility is compared for a request at the avatar's level, or zlev
        void check_matches_rebuild( vision_layers layers = vision_layers::all ) const;
        void check_matches_rebuild( vision_layers layers, int zlev ) const;
    private:
        los_pairs pairs_;
};

// second incremental build in a row changes no cache and advances no generation
void check_stationary_build_is_noop();

class map_meddler
{
    public:
        static bool has_altered_submaps( map &m );
        static submap *unsafe_get_submap_at( tripoint_bub_ms &p, point_sm_ms &l );
};

#endif // CATA_TESTS_MAP_HELPERS_H
