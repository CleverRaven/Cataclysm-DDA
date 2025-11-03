#pragma once
#ifndef CATA_SRC_REGIONAL_SETTINGS_H
#define CATA_SRC_REGIONAL_SETTINGS_H

#include <algorithm>
#include <array>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "debug.h"
#include "enums.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "memory_fast.h"
#include "type_id.h"
#include "weighted_list.h"

class weather_generator;
class JsonObject;
class JsonValue;
class mapgendata;

const region_settings_id DEFAULT_REGION( "default" );

class building_bin
{
    private:
        bool finalized = false; // NOLINT(cata-serialize)
    public:
        building_bin() = default;
        //TODO: remove add
        void add( const overmap_special_id &building, int weight );
        overmap_special_id pick() const;
        std::vector<std::string> all; // NOLINT(cata-serialize)
        void clear();
        void finalize();
        weighted_int_list<overmap_special_id> get_all_buildings() const {
            return buildings;
        }
        void deserialize( const JsonValue &jv ) {
            buildings.deserialize( jv );
        }
        weighted_int_list<overmap_special_id> buildings;
};

struct region_settings_city {
    region_settings_city_id id = region_settings_city_id::NULL_ID();

    // About the average US city non-residential, non-park land usage
    int shop_radius = 30;
    int shop_sigma = 20;

    // Set the same as shop radius, let parks bleed through via normal rolls
    int park_radius = shop_radius;
    // We'll spread this out to the rest of the town.
    int park_sigma = 100 - park_radius;

    std::string name_snippet = "<city_name>";

    building_bin houses;
    building_bin shops;
    building_bin parks;

    overmap_special_id pick_house() const {
        return houses.pick();
    }

    overmap_special_id pick_shop() const {
        return shops.pick();
    }

    overmap_special_id pick_park() const {
        return parks.pick();
    }

    weighted_int_list<overmap_special_id> get_all_houses() const {
        return houses.get_all_buildings();
    }

    weighted_int_list<overmap_special_id> get_all_shops() const {
        return shops.get_all_buildings();
    }

    weighted_int_list<overmap_special_id> get_all_parks() const {
        return parks.get_all_buildings();
    }

    bool was_loaded = false;
    void load( const JsonObject &jo, std::string_view );
    static void load_region_settings_city( const JsonObject &jo, const std::string &src );
    static void reset();
    void finalize();
};

/*
 * template for random bushes and such.
 * supports occasional boost to a single ter/furn type (clustered blueberry bushes for example)
 * json: double percentages (region statistics)and str ids, runtime int % * 1mil and int ids
 */
struct groundcover_extra {
    // TODO: make into something more generic for other stuff (maybe)
    std::string                   default_ter_str;
    std::map<std::string, double> percent_str;
    std::map<std::string, double> boosted_percent_str;
    std::map<int, ter_furn_id>    weightlist;
    std::map<int, ter_furn_id>    boosted_weightlist;
    ter_id default_ter               = t_null;
    int mpercent_coverage         = 0; // % coverage where this is applied (*10000)
    int boost_chance              = 0;
    int boosted_mpercent_coverage = 0;
    int boosted_other_mpercent    = 1;

    ter_furn_id pick( bool boosted = false ) const;
    void finalize();
    groundcover_extra() = default;
};

struct forest_biome_component {
    forest_biome_component_id id = forest_biome_component_id::NULL_ID();

    weighted_int_list<ter_furn_id> types;
    int sequence = 0;
    int chance = 0;

    bool was_loaded = false;
    void finalize();
    void load( const JsonObject &jo, std::string_view );
    forest_biome_component() = default;
    static void load_forest_biome_feature( const JsonObject &jo, const std::string &src );
    static void reset();
};

struct forest_biome_terrain_dependent_furniture_new {
    weighted_int_list<furn_id> furniture;
    int chance = 0;

    bool was_loaded = false;
    void deserialize( const JsonObject &jo );
    void finalize();
    forest_biome_terrain_dependent_furniture_new() = default;
};

/** Defines forest mapgen */
struct forest_biome_mapgen {
    forest_biome_mapgen_id id = forest_biome_mapgen_id::NULL_ID();

    std::set<oter_type_str_id> terrains;
    std::set<forest_biome_component_id> biome_components;
    weighted_int_list<ter_id> groundcover;
    std::map<ter_id, forest_biome_terrain_dependent_furniture_new> terrain_dependent_furniture;

    int sparseness_adjacency_factor = 0;
    int item_group_chance = 0;
    int item_spawn_iterations = 0;
    item_group_id item_group;


    ter_furn_id pick() const;
    bool was_loaded = false;
    void finalize();
    void load( const JsonObject &jo, std::string_view );
    static void load_forest_biome_mapgen( const JsonObject &jo, const std::string &src );
    static void reset();
    forest_biome_mapgen() = default;
};

/** Defines forest mapgen for a given OMT in a given region */
struct region_settings_forest_mapgen {
    region_settings_forest_mapgen_id id = region_settings_forest_mapgen_id::NULL_ID();

    std::set<forest_biome_mapgen_id> biomes;
    //use for convenience
    std::map<oter_type_id, forest_biome_mapgen_id> oter_to_biomes;

    bool was_loaded = false;
    void finalize();
    void load( const JsonObject &jo, std::string_view );
    static void load_region_settings_forest_mapgen( const JsonObject &jo, const std::string &src );
    static void reset();
    region_settings_forest_mapgen() = default;
};

struct region_settings_forest_trail {
    region_settings_forest_trail_id id = region_settings_forest_trail_id::NULL_ID();
    int chance = 1;
    int border_point_chance = 2;
    int minimum_forest_size = 50;
    int random_point_min = 4;
    int random_point_max = 50;
    int random_point_size_scalar = 100;
    int trailhead_chance = 1;
    int trailhead_road_distance = 6;
    building_bin trailheads;

    bool was_loaded = false;
    void load( const JsonObject &jo, std::string_view );
    void finalize();
    static void load_region_settings_forest_trail( const JsonObject &jo, const std::string &src );
    static void reset();
    region_settings_forest_trail() = default;
};

struct region_settings_feature_flag {
    std::set<std::string> blacklist;
    std::set<std::string> whitelist;

    bool was_loaded = false;
    void deserialize( const JsonObject &jo );
    region_settings_feature_flag() = default;
};

struct region_settings_forest {
    region_settings_forest_id id = region_settings_forest_id::NULL_ID();

    double noise_threshold_forest = 0.25;
    double noise_threshold_forest_thick = 0.3;
    double noise_threshold_swamp_adjacent_water = 0.3;
    double noise_threshold_swamp_isolated = 0.6;
    int river_floodplain_buffer_distance_min = 3;
    int river_floodplain_buffer_distance_max = 15;

    // hard to generate cities above 0.4
    float max_forest;
    // increase nesw
    std::array<float, 4> forest_increase;

    bool was_loaded = false;
    void load( const JsonObject &jo, std::string_view );
    void finalize();

    static void load_region_settings_forest( const JsonObject &jo, const std::string &src );
    static void reset();
    region_settings_forest() = default;
};

struct shore_extendable_overmap_terrain_alias {
    std::string overmap_terrain;
    ot_match_type match_type = ot_match_type::exact;
    oter_str_id alias;
    void deserialize( const JsonObject &jo );
};

struct region_settings_river {
    region_settings_river_id id = region_settings_river_id::NULL_ID();

    int river_scale = 1;
    double river_frequency = 1.5;
    double river_branch_chance = 64;
    double river_branch_remerge_chance = 4;
    double river_branch_scale_decrease = 1;

    bool was_loaded = false;
    void load( const JsonObject &jo, std::string_view );
    void finalize();

    static void load_region_settings_river( const JsonObject &jo, const std::string &src );
    static void reset();
    region_settings_river() = default;
};

struct region_settings_lake {
    region_settings_lake_id id = region_settings_lake_id::NULL_ID();

    double noise_threshold_lake = 0.25;
    int lake_size_min = 20;
    int lake_depth = -5;
    oter_str_id surface;
    oter_str_id shore;
    oter_str_id interior;
    oter_str_id bed;
    std::vector<oter_str_id> shore_extendable_overmap_terrain;
    std::vector<shore_extendable_overmap_terrain_alias> shore_extendable_overmap_terrain_aliases;
    bool invert_lakes = false;

    bool was_loaded = false;
    void load( const JsonObject &jo, std::string_view );
    void finalize();

    static void load_region_settings_lake( const JsonObject &jo, const std::string &src );
    static void reset();
    region_settings_lake();
};

struct region_settings_ocean {
    region_settings_ocean_id id = region_settings_ocean_id::NULL_ID();

    double noise_threshold_ocean = 0.25;
    int ocean_size_min = 100;
    int ocean_depth = -9;
    int ocean_start_north = 0;
    int ocean_start_east = 10;
    int ocean_start_west = 0;
    int ocean_start_south = 0;
    int sandy_beach_width = 2;

    bool was_loaded = false;
    void load( const JsonObject &jo, std::string_view );
    void finalize();

    static void load_region_settings_ocean( const JsonObject &jo, const std::string &src );
    static void reset();
    region_settings_ocean() = default;
};

struct region_settings_ravine {
    region_settings_ravine_id id = region_settings_ravine_id::NULL_ID();

    int num_ravines = 0;
    int ravine_range = 45;
    int ravine_width = 1;
    int ravine_depth = -3;

    bool was_loaded = false;
    void load( const JsonObject &jo, std::string_view );
    void finalize();

    static void load_region_settings_ravine( const JsonObject &jo, const std::string &src );
    static void reset();
    region_settings_ravine() = default;
};

struct region_settings_overmap_connection {
    overmap_connection_id trail_connection;
    overmap_connection_id sewer_connection;
    overmap_connection_id subway_connection;
    overmap_connection_id rail_connection;
    overmap_connection_id intra_city_road_connection;
    overmap_connection_id inter_city_road_connection;

    bool was_loaded = false;
    void deserialize( const JsonObject &jo );
    region_settings_overmap_connection() = default;
};

struct region_settings_highway {
    region_settings_highway_id id = region_settings_highway_id::NULL_ID();

    int grid_column_seperation = 10;
    int grid_row_seperation = 8;
    int width_of_segments = 2;
    int intersection_max_radius = 3;
    double straightness_chance = 0.6;
    oter_type_str_id reserved_terrain_id;
    oter_type_str_id reserved_terrain_water_id;
    oter_type_str_id symbolic_ramp_up_id;
    oter_type_str_id symbolic_ramp_down_id;
    oter_type_str_id symbolic_overpass_road_id;
    overmap_special_id segment_flat;
    overmap_special_id segment_ramp;
    overmap_special_id segment_road_bridge;
    overmap_special_id segment_bridge;
    overmap_special_id segment_bridge_supports;
    overmap_special_id segment_overpass;
    overmap_special_id clockwise_slant;
    overmap_special_id counterclockwise_slant;
    overmap_special_id fallback_onramp;
    overmap_special_id fallback_bend;
    overmap_special_id fallback_three_way_intersection;
    overmap_special_id fallback_four_way_intersection;
    oter_type_str_id fallback_supports;
    building_bin four_way_intersections;
    building_bin three_way_intersections;
    building_bin bends;
    building_bin interchanges;
    building_bin road_connections;

    int longest_slant_length = 0;
    int longest_bend_length = 0;
    int HIGHWAY_MAX_DEVIANCE = 0;

    bool was_loaded = false;
    void load( const JsonObject &jo, std::string_view );
    void finalize();

    static void load_region_settings_highway( const JsonObject &jo, const std::string &src );
    static void reset();
    region_settings_highway() = default;
};

struct map_extra_collection {
    map_extra_collection_id id = map_extra_collection_id::NULL_ID();

    unsigned int chance = 1;
    weighted_int_list<map_extra_id> values;

    map_extra_collection() : chance( 0 ) {}
    explicit map_extra_collection( const unsigned int embellished ) : chance( embellished ) {}
    map_extra_collection filtered_by( const mapgendata & ) const;

    bool was_loaded = false;
    void load( const JsonObject &jo, std::string_view );
    static void load_map_extra_collection( const JsonObject &jo, const std::string &src );
    static void reset();
};

struct region_settings_map_extras {
    region_settings_map_extras_id id = region_settings_map_extras_id::NULL_ID();
    std::set<map_extra_collection_id> extras;

    //returns every map extra in this collection regardless of weight
    std::set<map_extra_id> get_all_map_extras() const;

    bool was_loaded = false;
    void load( const JsonObject &jo, std::string_view );
    static void load_region_settings_map_extras( const JsonObject &jo, const std::string &src );
    static void reset();
};

/** Collection of `region_terrain_furniture` mappings */
struct region_settings_terrain_furniture {
    region_settings_terrain_furniture_id id = region_settings_terrain_furniture_id::NULL_ID();

    std::set<region_terrain_furniture_id> ter_furn;

    ter_id resolve( const ter_id & ) const;
    furn_id resolve( const furn_id & ) const;

    void finalize();
    bool was_loaded = false;
    void load( const JsonObject &jo, std::string_view );
    region_settings_terrain_furniture() = default;
    static void load_region_settings_terrain_furniture( const JsonObject &jo, const std::string &src );
    static void reset();
};

/**
* Maps abstract region terrain/furniture (e.g. `t_region_groundcover`) to
* actual region terrain/furniture (e.g. `t_grass`) with a weighted list
*/
struct region_terrain_furniture {
    region_terrain_furniture_id id = region_terrain_furniture_id::NULL_ID();

    ter_id replaced_ter_id;
    furn_id replaced_furn_id;
    weighted_int_list<ter_id> terrain;
    weighted_int_list<furn_id> furniture;

    bool was_loaded = false;
    void finalize();
    void load( const JsonObject &jo, std::string_view );
    ter_id resolve( const ter_id & ) const;
    furn_id resolve( const furn_id & ) const;
    region_terrain_furniture() = default;
    static void load_region_terrain_furniture( const JsonObject &jo, const std::string &src );
    static void reset();
};

/*
 * Spatially relevant overmap and mapgen variables grouped into a set of suggested defaults;
 * eventually region mapping will modify as required and allow for transitions of biomes / demographics in a smooth fashion
 */
struct region_settings {
    region_settings_id id = region_settings_id::NULL_ID();
    std::array<oter_str_id, OVERMAP_LAYERS> default_oter;
    weighted_int_list<ter_id> default_groundcover; // i.e., 'grass_or_dirt'
    shared_ptr_fast<weighted_int_list<ter_str_id>> default_groundcover_str;

    std::optional<region_settings_city_id> city_spec;
    region_settings_forest_mapgen_id forest_composition;
    std::optional<region_settings_forest_trail_id> forest_trail;
    weather_generator_id weather;
    region_settings_feature_flag overmap_feature_flag;
    std::optional<region_settings_forest_id> overmap_forest;
    std::optional<region_settings_river_id> overmap_river;
    std::optional<region_settings_lake_id> overmap_lake;
    std::optional<region_settings_ocean_id> overmap_ocean;
    std::optional<region_settings_highway_id> overmap_highway;
    std::optional<region_settings_ravine_id> overmap_ravine;
    region_settings_overmap_connection overmap_connection;
    region_settings_terrain_furniture_id region_terrain_and_furniture;

    bool place_swamps;
    bool place_roads;
    bool place_railroads;
    bool place_railroads_before_roads;
    bool place_specials;
    bool neighbor_connections;

    float max_urban;
    // increase nesw
    std::array<float, 4> urban_increase;


    region_settings_map_extras_id region_extras;

    region_settings() : id( "null" ) {
        default_groundcover.add( t_null, 0 );
    }

    const region_settings_city &get_settings_city() const {
        if( !city_spec.has_value() ) {
            debugmsg( "No city settings defined for %s, but requesting them", id.str() );
            static region_settings_city ret;
            return ret;
        }
        return *city_spec.value();
    }
    const region_settings_forest_mapgen &get_settings_forest_composition() const {
        return *forest_composition;
    }
    const region_settings_forest_trail &get_settings_forest_trail() const {
        if( !forest_trail.has_value() ) {
            debugmsg( "No forest trail settings defined for %s, but requesting them", id.str() );
            static region_settings_forest_trail ret;
            return ret;
        }
        return *forest_trail.value();
    }
    const weather_generator &get_settings_weather() const {
        return *weather;
    }
    const region_settings_forest &get_settings_forest() const {
        if( !overmap_forest.has_value() ) {
            debugmsg( "No forest settings defined for %s, but requesting them", id.str() );
            static region_settings_forest ret;
            return ret;
        }
        return *overmap_forest.value();
    }
    const region_settings_river &get_settings_river() const {
        if( !overmap_river.has_value() ) {
            debugmsg( "No river settings defined for %s, but requesting them", id.str() );
            static region_settings_river ret;
            return ret;
        }
        return *overmap_river.value();
    }
    const region_settings_lake &get_settings_lake() const {
        if( !overmap_lake.has_value() ) {
            debugmsg( "No lake settings defined for %s, but requesting them", id.str() );
            static region_settings_lake ret;
            return ret;
        }
        return *overmap_lake.value();
    }
    const region_settings_ocean &get_settings_ocean() const {
        if( !overmap_ocean.has_value() ) {
            debugmsg( "No ocean settings defined for %s, but requesting them", id.str() );
            static region_settings_ocean ret;
            return ret;
        }
        return *overmap_ocean.value();
    }
    const region_settings_highway &get_settings_highway() const {
        if( !overmap_highway.has_value() ) {
            debugmsg( "No highway settings defined for %s, but requesting them", id.str() );
            static region_settings_highway ret;
            return ret;
        }
        return *overmap_highway.value();
    }
    const region_settings_ravine &get_settings_ravine() const {
        if( !overmap_ravine.has_value() ) {
            debugmsg( "No ravine settings defined for %s, but requesting them", id.str() );
            static region_settings_ravine ret;
            return ret;
        }
        return *overmap_ravine.value();
    }
    const region_settings_terrain_furniture &get_settings_terrain_furniture() const {
        return *region_terrain_and_furniture;
    }
    const region_settings_map_extras &get_settings_map_extras() const {
        return *region_extras;
    }

    bool was_loaded = false;
    void load( const JsonObject &jo, std::string_view );
    void finalize();
    static void finalize_all();
    static void load_region_settings( const JsonObject &jo, const std::string &src );
    static void reset();
};

#endif // CATA_SRC_REGIONAL_SETTINGS_H
