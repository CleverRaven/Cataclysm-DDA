#pragma once

#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "weighted_list.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class JsonObject;
template <typename T> struct enum_traits;

using Region_map = std::unordered_map<tripoint_abs_om, region_settings_id>;

enum class regions_generation_mode : int {
    UNIFORM = 0, // DYNAMIC; single-region
    MANUAL_VORONOI = 1, // STATIC; plot region points with JSON, where a voronoi diagram is generated from the points
    RANDOM = 2, // DYNAMIC; each overmap generated picks a random region from the provided set of regions
    ANGLES = 3, // DYNAMIC; divides world map into equal pie-chart-like sectors
    last = 4
};

template<>
struct enum_traits<regions_generation_mode> {
    static constexpr regions_generation_mode last = regions_generation_mode::last;
};

// an origin point with a region type for voronoi diagram generation
struct region_voronoi_point {
    tripoint_abs_om region_point;
    region_settings_id region;

    void deserialize( const JsonObject &jo );
};

// a set of points accompanied by a set of regions
// different generation modes have different usages; not all use `region_points`
struct region_point_set {
    // to-generate region points read from JSON
    std::unordered_set<tripoint_abs_om> region_points;
    // providing weights is optional
    weighted_int_list<region_settings_id> regions_weighted;
    // if true, removes a region from the weighted list when selected
    // if `regions` is empty, defaults to `default_region`
    bool remove_region = false;
    // fallback for region placement
    region_settings_id default_region;
    // maximum distance from specified point
    int region_point_variance = 0;

    // resolves the point set, mapping regions to points
    std::vector<region_voronoi_point> resolve() const;
    void deserialize( const JsonObject &jo );
};

// defines properties of an in-game dimension
// its name is solely to distinguish from a generic `dimension`
class dimension_world
{

        dimension_region_layout_id region_layout;
    public:
        dimension_id id;
        bool was_loaded = false;

        dimension_region_layout_id get_region_layout() const {
            return region_layout;
        }
        void load( const JsonObject &jo, const std::string_view &src );
        void finalize();
        static void finalize_all();
        static void load_dimensions( const JsonObject &jo, const std::string &src );
};

class dimension_region_layout_generator
{

        // forward to child deserialize function based on `generation_mode`
        virtual void deserialize( const JsonObject &jo ) = 0;
    public:
        virtual ~dimension_region_layout_generator() = default;
        // return the region for the given overmap
        virtual region_settings_id get_overmap_region( const tripoint_abs_om &om_point ) = 0;
        virtual bool is_static_generation() const = 0;
        // generate for one overmap, which will generate the entire layout for static layouts
        void init() {
            get_overmap_region( tripoint_abs_om( 0, 0, 0 ) );
        }
};

// generate one or more regions when first getting an overmap's region
class dimension_region_layout_dynamic : public dimension_region_layout_generator
{

        // forward to child deserialize function based on `generation_mode`
        void deserialize( const JsonObject &jo ) override = 0;
    public:
        // do dynamic region generation, called from get_overmap_region
        virtual void generate_dynamic( Region_map &placed_regions, const tripoint_abs_om &current_om ) = 0;
        // return the region for the given overmap
        region_settings_id get_overmap_region( const tripoint_abs_om &om_point ) override;
        bool is_static_generation() const override {
            return false;
        }
};

// generate a bounded static region layout at world start,
// and then use a dynamic generator for out-of-bounds
class dimension_region_layout_static : public dimension_region_layout_generator
{

        dimension_region_layout_id layout_out_of_bounds;
    protected:
        inclusive_cuboid<tripoint_abs_om> generated_bounds;

    public:
        void deserialize( const JsonObject &jo ) override;
        virtual void generate_entire_layout( Region_map &placed_regions,
                                             const tripoint_abs_om &current_om ) = 0;
        region_settings_id get_overmap_region( const tripoint_abs_om &om_point ) override;
        bool overmap_in_bounds( const tripoint_abs_om &om_point ) {
            return generated_bounds.contains( om_point );
        };
        bool is_static_generation() const override {
            return true;
        }
};

class dimension_region_layout_generator_uniform : public dimension_region_layout_dynamic
{
        region_settings_id uniform_region;
    public:
        void generate_dynamic( Region_map &placed_regions, const tripoint_abs_om &current_om ) override;
        void deserialize( const JsonObject &jo ) override;
};

class dimension_region_layout_generator_manual : public dimension_region_layout_static
{
        std::vector<region_point_set> region_point_sets;
    public:
        void generate_entire_layout( Region_map &placed_regions,
                                     const tripoint_abs_om &current_om ) override;
        void deserialize( const JsonObject &jo ) override;
};

class dimension_region_layout_generator_random : public dimension_region_layout_dynamic
{
        weighted_int_list<region_settings_id> random_regions;
    public:
        void generate_dynamic( Region_map &placed_regions, const tripoint_abs_om &current_om ) override;
        void deserialize( const JsonObject &jo ) override;
};

class dimension_region_layout_generator_angles : public dimension_region_layout_dynamic
{
        // these should be loaded in order
        std::vector<region_settings_id> angles_regions;
        units::angle angles_offset = 0_degrees;
    public:
        void generate_dynamic( Region_map &placed_regions, const tripoint_abs_om &current_om ) override;
        void deserialize( const JsonObject &jo ) override;
        int get_sector_count() const {
            return static_cast<int>( angles_regions.size() );
        }
};

/*
* reads JSON data used to generate a dimension's region map
*/
class dimension_region_layout
{

        // what type of region layout generator to use (see `regions_generation_mode`)
        regions_generation_mode generation_mode;
        std::shared_ptr<dimension_region_layout_generator> layout_generator;

    public:
        dimension_region_layout_id id;
        bool was_loaded = false;

        dimension_region_layout() = default;

        std::shared_ptr<dimension_region_layout_generator> get_generator() const {
            return layout_generator;
        }

        void load( const JsonObject &jo, const std::string_view &src );
        static void load_dimension_regions( const JsonObject &jo, const std::string &src );
};
