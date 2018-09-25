#pragma once
#ifndef REGIONAL_SETTINGS_H
#define REGIONAL_SETTINGS_H

#include "weighted_list.h"
#include "omdata.h"
#include "mapdata.h"
#include "weather_gen.h"

#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

class JsonObject;

class building_bin
{
    private:
        bool finalized = false;
        weighted_int_list<overmap_special_id> buildings;
        std::map<overmap_special_id, int> unfinalized_buildings;
    public:
        building_bin() {};
        void add( const overmap_special_id &building, int weight );
        overmap_special_id pick() const;
        std::vector<std::string> all;
        void clear();
        void finalize();
};

struct city_settings {
    int shop_radius =
        80;  // this is not a cut and dry % but rather an inverse voodoo number; rng(0,99) > VOODOO * distance / citysize;
    int park_radius =
        130; // in theory, adjusting these can make a town with a few shops and a lot of parks + houses......by increasing shop_radius
    int house_basement_chance = 5; // one_in(n) chance a house has a basement
    building_bin houses;
    building_bin basements;
    building_bin shops;
    building_bin parks;

    overmap_special_id pick_house() const {
        return houses.pick()->id;
    }

    overmap_special_id pick_basement() const {
        return basements.pick()->id;
    }

    overmap_special_id pick_shop() const {
        return shops.pick()->id;
    }

    overmap_special_id pick_park() const {
        return parks.pick()->id;
    }

    void finalize();
};

struct ter_furn_id {
    ter_id ter;
    furn_id furn;
    ter_furn_id();
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
    std::map<std::string, int> unfinalized_types;
    weighted_int_list<ter_furn_id> types;
    int sequence = 0;
    int chance = 0;
    bool clear_types = false;

    void finalize();
    forest_biome_component() = default;
};

struct forest_biome {
    std::map<std::string, forest_biome_component> unfinalized_biome_components;
    std::vector<forest_biome_component> biome_components;
    std::map<std::string, int> unfinalized_groundcover;
    weighted_int_list<ter_id> groundcover;
    int sparseness_adjacency_factor = 0;
    int item_group_chance = 0;
    int item_spawn_iterations = 0;
    std::string item_group;
    bool clear_components = false;
    bool clear_groundcover = false;

    ter_furn_id pick() const;
    void finalize();
    forest_biome() = default;
};

struct forest_mapgen_settings {
    std::map<std::string, forest_biome> unfinalized_biomes;
    std::map<oter_id, forest_biome> biomes;

    void finalize();
    forest_mapgen_settings() = default;
};

struct map_extras {
    unsigned int chance;
    weighted_int_list<std::string> values;

    map_extras() : chance( 0 ), values() {}
    map_extras( const unsigned int embellished ) : chance( embellished ), values() {}
};

struct sid_or_sid;
/*
 * Spationally relevant overmap and mapgen variables grouped into a set of suggested defaults;
 * eventually region mapping will modify as required and allow for transitions of biomes / demographics in a smoooth fashion
 */
struct regional_settings {
    std::string id;           //
    oter_str_id default_oter; // 'field'

    weighted_int_list<ter_id> default_groundcover; // ie, 'grass_or_dirt'
    std::shared_ptr<weighted_int_list<ter_str_id>> default_groundcover_str;

    int num_forests           = 250;  // amount of forest groupings per overmap
    int forest_size_min       = 15;   // size range of a forest group
    int forest_size_max       = 40;   // size range of a forest group
    int swamp_maxsize         = 4;    // SWAMPINESS: Affects the size of a swamp
    int swamp_river_influence = 5;    // voodoo number limiting spread of river through swamp
    int swamp_spread_chance   =
        8500; // SWAMPCHANCE: (one in, every forest*forest size) chance of swamp extending past forest

    city_settings     city_spec;      // put what where in a city of what kind
    groundcover_extra field_coverage;
    forest_mapgen_settings forest_composition;

    weather_generator weather;

    std::unordered_map<std::string, map_extras> region_extras;

    regional_settings() : id( "null" ), default_oter( "field" ) {
        default_groundcover.add( t_null, 0 );
    }
    void finalize();
};

typedef std::unordered_map<std::string, regional_settings> t_regional_settings_map;
typedef t_regional_settings_map::const_iterator t_regional_settings_map_citr;
extern t_regional_settings_map region_settings_map;

void load_region_settings( JsonObject &jo );
void reset_region_settings();
void load_region_overlay( JsonObject &jo );
void apply_region_overlay( JsonObject &jo, regional_settings &region );

#endif
