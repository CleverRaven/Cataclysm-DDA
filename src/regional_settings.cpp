#include "regional_settings.h"

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "debug.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "mapdata.h"
#include "map_extras.h"
#include "omdata.h"
#include "rng.h"
#include "string_formatter.h"

class mapgendata;

static const oter_str_id oter_lake_bed( "lake_bed" );
static const oter_str_id oter_lake_shore( "lake_shore" );
static const oter_str_id oter_lake_surface( "lake_surface" );
static const oter_str_id oter_lake_water_cube( "lake_water_cube" );

static const weighted_string_id_reader<overmap_special_id, int> building_bin_reader( 1 );
static const weighted_string_id_reader<furn_id, int> furn_reader( 1 );
static const weighted_string_id_reader<ter_id, int> ter_reader( 1 );

/** SETTING FACTORY */
namespace
{
generic_factory<region_settings_river> region_settings_river_factory( "region_settings_river" );
generic_factory<region_settings_lake> region_settings_lake_factory( "region_settings_lake" );
generic_factory<region_settings_ocean> region_settings_ocean_factory( "region_settings_ocean" );
generic_factory<region_settings_ravine> region_settings_ravine_factory( "region_settings_ravine" );
generic_factory<region_settings_forest> region_settings_forest_factory( "region_settings_forest" );
generic_factory<region_settings_highway>
region_settings_highway_factory( "region_settings_highway" );
generic_factory<region_settings_forest_trail>
region_settings_forest_trail_factory( "region_settings_forest_trail" );
generic_factory<region_settings_city> region_settings_city_factory( "region_settings_city" );
generic_factory<region_settings_terrain_furniture>
region_settings_terrain_furniture_factory( "region_settings_terrain_furniture" );
generic_factory<region_terrain_furniture>
region_terrain_furniture_factory( "region_terrain_furniture" );
generic_factory<region_settings_forest_mapgen>
region_settings_forest_mapgen_factory( "region_settings_forest_mapgen" );
generic_factory<region_settings_map_extras>
region_settings_map_extras_factory( "region_settings_map_extras" );
generic_factory<forest_biome_component> forest_biome_feature_factory( "forest_biome_component" );
generic_factory<forest_biome_mapgen> forest_biome_mapgen_factory( "forest_biome_mapgen" );
generic_factory<map_extra_collection> map_extra_collection_factory( "map_extra_collection" );
generic_factory<region_settings> region_settings_factory( "region_settings_new" );
} // namespace

/** OBJ */
template<>
const region_settings_river &string_id<region_settings_river>::obj() const
{
    return region_settings_river_factory.obj( *this );
}
template<>
const region_settings_lake &string_id<region_settings_lake>::obj() const
{
    return region_settings_lake_factory.obj( *this );
}
template<>
const region_settings_ocean &string_id<region_settings_ocean>::obj() const
{
    return region_settings_ocean_factory.obj( *this );
}
template<>
const region_settings_ravine &string_id<region_settings_ravine>::obj() const
{
    return region_settings_ravine_factory.obj( *this );
}
template<>
const region_settings_forest &string_id<region_settings_forest>::obj() const
{
    return region_settings_forest_factory.obj( *this );
}
template<>
const region_settings_highway &string_id<region_settings_highway>::obj() const
{
    return region_settings_highway_factory.obj( *this );
}
template<>
const region_settings_forest_trail &string_id<region_settings_forest_trail>::obj() const
{
    return region_settings_forest_trail_factory.obj( *this );
}
template<>
const region_settings_city &string_id<region_settings_city>::obj() const
{
    return region_settings_city_factory.obj( *this );
}
template<>
const region_settings_terrain_furniture &string_id<region_settings_terrain_furniture>::obj() const
{
    return region_settings_terrain_furniture_factory.obj( *this );
}
template<>
const region_terrain_furniture &string_id<region_terrain_furniture>::obj() const
{
    return region_terrain_furniture_factory.obj( *this );
}
template<>
const region_settings_forest_mapgen &string_id<region_settings_forest_mapgen>::obj() const
{
    return region_settings_forest_mapgen_factory.obj( *this );
}
template<>
const region_settings_map_extras &string_id<region_settings_map_extras>::obj() const
{
    return region_settings_map_extras_factory.obj( *this );
}
template<>
const forest_biome_component &string_id<forest_biome_component>::obj() const
{
    return forest_biome_feature_factory.obj( *this );
}
template<>
const forest_biome_mapgen &string_id<forest_biome_mapgen>::obj() const
{
    return forest_biome_mapgen_factory.obj( *this );
}
template<>
const region_settings &string_id<region_settings>::obj() const
{
    return region_settings_factory.obj( *this );
}
template<>
const map_extra_collection &string_id<map_extra_collection>::obj() const
{
    return map_extra_collection_factory.obj( *this );
}

/** IS_VALID */
template<>
bool string_id<region_settings_river>::is_valid() const
{
    return region_settings_river_factory.is_valid( *this );
}
template<>
bool string_id<region_settings_lake>::is_valid() const
{
    return region_settings_lake_factory.is_valid( *this );
}
template<>
bool string_id<region_settings_ocean>::is_valid() const
{
    return region_settings_ocean_factory.is_valid( *this );
}
template<>
bool string_id<region_settings_ravine>::is_valid() const
{
    return region_settings_ravine_factory.is_valid( *this );
}
template<>
bool string_id<region_settings_forest>::is_valid() const
{
    return region_settings_forest_factory.is_valid( *this );
}
template<>
bool string_id<region_settings_highway>::is_valid() const
{
    return region_settings_highway_factory.is_valid( *this );
}
template<>
bool string_id<region_settings_forest_trail>::is_valid() const
{
    return region_settings_forest_trail_factory.is_valid( *this );
}
template<>
bool string_id<region_settings_city>::is_valid() const
{
    return region_settings_city_factory.is_valid( *this );
}
template<>
bool string_id<region_settings_terrain_furniture>::is_valid() const
{
    return region_settings_terrain_furniture_factory.is_valid( *this );
}
template<>
bool string_id<region_terrain_furniture>::is_valid() const
{
    return region_terrain_furniture_factory.is_valid( *this );
}
template<>
bool string_id<region_settings_forest_mapgen>::is_valid() const
{
    return region_settings_forest_mapgen_factory.is_valid( *this );
}
template<>
bool string_id<region_settings_map_extras>::is_valid() const
{
    return region_settings_map_extras_factory.is_valid( *this );
}
template<>
bool string_id<forest_biome_component>::is_valid() const
{
    return forest_biome_feature_factory.is_valid( *this );
}
template<>
bool string_id<forest_biome_mapgen>::is_valid() const
{
    return forest_biome_mapgen_factory.is_valid( *this );
}
template<>
bool string_id<map_extra_collection>::is_valid() const
{
    return map_extra_collection_factory.is_valid( *this );
}
template<>
bool string_id<region_settings>::is_valid() const
{
    return region_settings_factory.is_valid( *this );
}

/** INIT LOAD */
void region_settings_river::load_region_settings_river( const JsonObject &jo,
        const std::string &src )
{
    region_settings_river_factory.load( jo, src );
}
void region_settings_lake::load_region_settings_lake( const JsonObject &jo, const std::string &src )
{
    region_settings_lake_factory.load( jo, src );
}
void region_settings_ocean::load_region_settings_ocean( const JsonObject &jo,
        const std::string &src )
{
    region_settings_ocean_factory.load( jo, src );
}
void region_settings_ravine::load_region_settings_ravine( const JsonObject &jo,
        const std::string &src )
{
    region_settings_ravine_factory.load( jo, src );
}
void region_settings_forest::load_region_settings_forest( const JsonObject &jo,
        const std::string &src )
{
    region_settings_forest_factory.load( jo, src );
}
void region_settings_highway::load_region_settings_highway( const JsonObject &jo,
        const std::string &src )
{
    region_settings_highway_factory.load( jo, src );
}
void region_settings_forest_trail::load_region_settings_forest_trail( const JsonObject &jo,
        const std::string &src )
{
    region_settings_forest_trail_factory.load( jo, src );
}
void region_settings_city::load_region_settings_city( const JsonObject &jo,
        const std::string &src )
{
    region_settings_city_factory.load( jo, src );
}
void region_settings_terrain_furniture::load_region_settings_terrain_furniture(
    const JsonObject &jo,
    const std::string &src )
{
    region_settings_terrain_furniture_factory.load( jo, src );
}
void region_terrain_furniture::load_region_terrain_furniture( const JsonObject &jo,
        const std::string &src )
{
    region_terrain_furniture_factory.load( jo, src );
}
void region_settings_forest_mapgen::load_region_settings_forest_mapgen( const JsonObject &jo,
        const std::string &src )
{
    region_settings_forest_mapgen_factory.load( jo, src );
}
void region_settings_map_extras::load_region_settings_map_extras( const JsonObject &jo,
        const std::string &src )
{
    region_settings_map_extras_factory.load( jo, src );
}
void forest_biome_component::load_forest_biome_feature( const JsonObject &jo,
        const std::string &src )
{
    forest_biome_feature_factory.load( jo, src );
}
void forest_biome_mapgen::load_forest_biome_mapgen( const JsonObject &jo,
        const std::string &src )
{
    forest_biome_mapgen_factory.load( jo, src );
}
void map_extra_collection::load_map_extra_collection( const JsonObject &jo,
        const std::string &src )
{
    map_extra_collection_factory.load( jo, src );
}
void region_settings::load_region_settings( const JsonObject &jo,
        const std::string &src )
{
    region_settings_factory.load( jo, src );
}

/** UNLOAD (RESET) */
void region_settings_river::reset()
{
    region_settings_river_factory.reset();
}
void region_settings_lake::reset()
{
    region_settings_lake_factory.reset();
}
void region_settings_ocean::reset()
{
    region_settings_ocean_factory.reset();
}
void region_settings_ravine::reset()
{
    region_settings_ravine_factory.reset();
}
void region_settings_forest::reset()
{
    region_settings_forest_factory.reset();
}
void region_settings_highway::reset()
{
    region_settings_highway_factory.reset();
}
void region_settings_forest_trail::reset()
{
    region_settings_forest_trail_factory.reset();
}
void region_settings_city::reset()
{
    region_settings_city_factory.reset();
}
void region_settings_terrain_furniture::reset()
{
    region_settings_terrain_furniture_factory.reset();
}
void region_terrain_furniture::reset()
{
    region_terrain_furniture_factory.reset();
}
void region_settings_forest_mapgen::reset()
{
    region_settings_forest_mapgen_factory.reset();
}
void region_settings_map_extras::reset()
{
    region_settings_map_extras_factory.reset();
}
void forest_biome_component::reset()
{
    forest_biome_feature_factory.reset();
}
void forest_biome_mapgen::reset()
{
    forest_biome_mapgen_factory.reset();
}
void map_extra_collection::reset()
{
    map_extra_collection_factory.reset();
}
void region_settings::reset()
{
    region_settings_factory.reset();
}

template<typename T>
void read_and_set_or_throw( const JsonObject &jo, const std::string &member, T &target,
                            bool required )
{
    T tmp;
    if( !jo.read( member, tmp ) ) {
        if( required ) {
            jo.throw_error( string_format( "%s required", member ) );
        }
    } else {
        target = tmp;
    }
}

void forest_biome_component::load( const JsonObject &jo, std::string_view )
{
    weighted_string_id_reader<ter_furn_id, int> ter_furn_reader( 1 );
    optional( jo, was_loaded, "chance", chance );
    optional( jo, was_loaded, "sequence", sequence );
    optional( jo, was_loaded, "types", types, ter_furn_reader );
}

void forest_biome_terrain_dependent_furniture_new::deserialize( const JsonObject &jo )
{
    optional( jo, was_loaded, "chance", chance );
    optional( jo, was_loaded, "furniture", furniture, furn_reader );
}

void forest_biome_mapgen::load( const JsonObject &jo, std::string_view )
{

    optional( jo, was_loaded, "terrains", terrains, string_id_reader<oter_type_t> {} );
    optional( jo, was_loaded, "sparseness_adjacency_factor", sparseness_adjacency_factor );
    optional( jo, was_loaded, "item_group", item_group );
    optional( jo, was_loaded, "item_group_chance", item_group_chance );
    optional( jo, was_loaded, "item_spawn_iterations", item_spawn_iterations );

    optional( jo, was_loaded, "components", biome_components, string_id_reader<forest_biome_component> {} );
    optional( jo, was_loaded, "groundcover", groundcover, ter_reader );
    optional( jo, was_loaded, "terrain_furniture", terrain_dependent_furniture );
}

void region_settings_forest_mapgen::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "biomes", biomes, auto_flags_reader<forest_biome_mapgen_id> {} );
}

void region_settings_forest_trail::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "chance", chance );
    optional( jo, was_loaded, "border_point_chance", border_point_chance );
    optional( jo, was_loaded, "minimum_forest_size", minimum_forest_size );
    optional( jo, was_loaded, "random_point_min", random_point_min );
    optional( jo, was_loaded, "random_point_max", random_point_max );
    optional( jo, was_loaded, "random_point_size_scalar", random_point_size_scalar );
    optional( jo, was_loaded, "trailhead_chance", trailhead_chance );
    optional( jo, was_loaded, "trailhead_road_distance", trailhead_road_distance );
    optional( jo, was_loaded, "trailheads", trailheads.buildings, building_bin_reader );
}

void region_settings_feature_flag::deserialize( const JsonObject &jo )
{
    optional( jo, was_loaded, "blacklist", blacklist, string_reader{} );
    optional( jo, was_loaded, "whitelist", whitelist, string_reader{} );
    was_loaded = true;
}

void region_settings_forest::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "noise_threshold_forest", noise_threshold_forest );
    optional( jo, was_loaded, "noise_threshold_forest_thick", noise_threshold_forest_thick );
    optional( jo, was_loaded, "noise_threshold_swamp_adjacent_water",
              noise_threshold_swamp_adjacent_water );
    optional( jo, was_loaded, "noise_threshold_swamp_isolated", noise_threshold_swamp_isolated );
    optional( jo, was_loaded, "river_floodplain_buffer_distance_min",
              river_floodplain_buffer_distance_min );
    optional( jo, was_loaded, "river_floodplain_buffer_distance_max",
              river_floodplain_buffer_distance_max );

    optional( jo, was_loaded, "forest_threshold_limit", max_forest, 0.395 );
    optional( jo, was_loaded, "forest_threshold_increase", forest_increase, { 0, 0, 0, 0 } );
}

void region_settings_ravine::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "num_ravines", num_ravines );
    optional( jo, was_loaded, "ravine_width", ravine_width );
    optional( jo, was_loaded, "ravine_range",
              ravine_range );
    optional( jo, was_loaded, "ravine_depth", ravine_depth );
}

void region_settings_overmap_connection::deserialize( const JsonObject &jo )
{
    optional( jo, was_loaded, "trail_connection", trail_connection );
    optional( jo, was_loaded, "sewer_connection", sewer_connection );
    optional( jo, was_loaded, "subway_connection", subway_connection );
    optional( jo, was_loaded, "rail_connection", rail_connection );
    optional( jo, was_loaded, "intra_city_road_connection", intra_city_road_connection );
    optional( jo, was_loaded, "inter_city_road_connection", inter_city_road_connection );
}

region_settings_lake::region_settings_lake() :
    surface( oter_lake_surface ),
    shore( oter_lake_shore ),
    interior( oter_lake_water_cube ),
    bed( oter_lake_bed )
{}

void region_settings_lake::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "noise_threshold_lake", noise_threshold_lake );
    optional( jo, was_loaded, "lake_size_min", lake_size_min );
    optional( jo, was_loaded, "lake_depth", lake_depth );
    auto_flags_reader<oter_str_id> sid_reader;
    optional( jo, was_loaded, "shore_extendable_overmap_terrain",
              shore_extendable_overmap_terrain, sid_reader );
    optional( jo, was_loaded, "shore_extendable_overmap_terrain_aliases",
              shore_extendable_overmap_terrain_aliases );
    optional( jo, was_loaded, "invert_lakes", invert_lakes, false );
    optional( jo, was_loaded, "surface_ter", surface, oter_lake_surface );
    optional( jo, was_loaded, "shore_ter", shore, oter_lake_shore );
    optional( jo, was_loaded, "interior_ter", interior, oter_lake_water_cube );
    optional( jo, was_loaded, "bed_ter", bed, oter_lake_bed );
}

void shore_extendable_overmap_terrain_alias::deserialize( const JsonObject &jo )
{
    optional( jo, false, "om_terrain", overmap_terrain );
    optional( jo, false, "alias", alias );
    optional( jo, false, "om_terrain_match_type", match_type );
}

void region_settings_river::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "river_scale", river_scale );
    optional( jo, was_loaded, "river_frequency", river_frequency );
    optional( jo, was_loaded, "river_branch_chance", river_branch_chance );
    optional( jo, was_loaded, "river_branch_remerge_chance", river_branch_remerge_chance );
    optional( jo, was_loaded, "river_branch_scale_decrease", river_branch_scale_decrease );
}

void region_settings_ocean::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "noise_threshold_ocean", noise_threshold_ocean );
    optional( jo, was_loaded, "ocean_size_min", ocean_size_min );
    optional( jo, was_loaded, "ocean_depth", ocean_depth );
    optional( jo, was_loaded, "ocean_start_north", ocean_start_north, std::nullopt );
    optional( jo, was_loaded, "ocean_start_east", ocean_start_east, std::nullopt );
    optional( jo, was_loaded, "ocean_start_west", ocean_start_west, std::nullopt );
    optional( jo, was_loaded, "ocean_start_south", ocean_start_south, std::nullopt );
    optional( jo, was_loaded, "sandy_beach_width", sandy_beach_width, std::nullopt );
}

void region_settings_highway::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "width_of_segments", width_of_segments );
    optional( jo, was_loaded, "straightness_chance", straightness_chance );
    optional( jo, was_loaded, "reserved_terrain_id", reserved_terrain_id );
    optional( jo, was_loaded, "reserved_terrain_water_id", reserved_terrain_water_id );
    optional( jo, was_loaded, "segment_flat_special", segment_flat );
    optional( jo, was_loaded, "segment_ramp_special", segment_ramp );
    optional( jo, was_loaded, "segment_road_bridge_special", segment_road_bridge );
    optional( jo, was_loaded, "segment_bridge_special", segment_bridge );
    optional( jo, was_loaded, "fallback_onramp_special", fallback_onramp );
    optional( jo, was_loaded, "segment_bridge_supports_special", segment_bridge_supports );
    optional( jo, was_loaded, "segment_overpass_special", segment_overpass );
    optional( jo, was_loaded, "fallback_bend_special", fallback_bend );
    optional( jo, was_loaded, "clockwise_slant_special", clockwise_slant );
    optional( jo, was_loaded, "counterclockwise_slant_special", counterclockwise_slant );
    optional( jo, was_loaded, "fallback_three_way_intersection_special",
              fallback_three_way_intersection );
    optional( jo, was_loaded, "fallback_four_way_intersection_special",
              fallback_four_way_intersection );
    optional( jo, was_loaded, "fallback_supports", fallback_supports );
    optional( jo, was_loaded, "reserved_terrain_water_id", reserved_terrain_water_id );

    optional( jo, was_loaded, "four_way_intersections", four_way_intersections.buildings,
              building_bin_reader );
    optional( jo, was_loaded, "three_way_intersections", three_way_intersections.buildings,
              building_bin_reader );
    optional( jo, was_loaded, "bends", bends.buildings, building_bin_reader );
    optional( jo, was_loaded, "road_connections", road_connections.buildings, building_bin_reader );
    optional( jo, was_loaded, "interchanges", interchanges.buildings, building_bin_reader );
}

void region_settings_terrain_furniture::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "ter_furn", ter_furn, auto_flags_reader<region_terrain_furniture_id> {} );
}

void region_terrain_furniture::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "ter_id", replaced_ter_id );
    optional( jo, was_loaded, "furn_id", replaced_furn_id );
    optional( jo, was_loaded, "replace_with_terrain", terrain, ter_reader );
    optional( jo, was_loaded, "replace_with_furniture", furniture, furn_reader );
}

void region_settings_city::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "shop_radius", shop_radius );
    optional( jo, was_loaded, "shop_sigma", shop_sigma );
    optional( jo, was_loaded, "park_radius", park_radius );
    optional( jo, was_loaded, "park_sigma", park_sigma );
    optional( jo, was_loaded, "name_snippet", name_snippet, "<city_name>" );
    optional( jo, was_loaded, "houses", houses.buildings, building_bin_reader );
    optional( jo, was_loaded, "shops", shops.buildings, building_bin_reader );
    optional( jo, was_loaded, "parks", parks.buildings, building_bin_reader );
}

void region_settings_map_extras::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "extras", extras, auto_flags_reader<map_extra_collection_id> {} );
}

std::set<map_extra_id> region_settings_map_extras::get_all_map_extras() const
{
    std::set<map_extra_id> all_mx;
    for( const map_extra_collection_id &region_extra : extras ) {
        for( const std::pair<map_extra_id, int> &mx_entry : ( *region_extra ).values ) {
            all_mx.emplace( mx_entry.first );
        }
    }
    return all_mx;
}

void map_extra_collection::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "chance", chance );
    weighted_string_id_reader<map_extra_id, int> extras_reader( 1 );
    optional( jo, was_loaded, "extras", values, extras_reader );
}

void region_settings::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "default_oter", default_oter );

    optional( jo, was_loaded, "default_groundcover", default_groundcover, ter_reader );
    optional( jo, was_loaded, "forest_composition", forest_composition );
    optional( jo, was_loaded, "forest_trails", forest_trail );

    optional( jo, was_loaded, "map_extras", region_extras );
    optional( jo, was_loaded, "cities", city_spec );
    optional( jo, was_loaded, "weather", weather );
    optional( jo, was_loaded, "feature_flag_settings", overmap_feature_flag );
    optional( jo, was_loaded, "forests", overmap_forest );
    optional( jo, was_loaded, "rivers", overmap_river );
    optional( jo, was_loaded, "lakes", overmap_lake );
    optional( jo, was_loaded, "ocean", overmap_ocean );
    optional( jo, was_loaded, "highways", overmap_highway );
    optional( jo, was_loaded, "ravines", overmap_ravine );
    optional( jo, was_loaded, "connections", overmap_connection );
    optional( jo, was_loaded, "terrain_furniture", region_terrain_and_furniture );

    optional( jo, was_loaded, "place_swamps", place_swamps, true );
    optional( jo, was_loaded, "place_roads", place_roads, true );
    optional( jo, was_loaded, "place_railroads", place_railroads, false );
    optional( jo, was_loaded, "place_railroads_before_roads", place_railroads_before_roads, false );
    optional( jo, was_loaded, "place_specials", place_specials, true );
    optional( jo, was_loaded, "neighbor_connections", neighbor_connections, true );

    optional( jo, was_loaded, "max_urbanity", max_urban, 8 );
    optional( jo, was_loaded, "urbanity_increase", urban_increase, { 0, 0, 0, 0 } );
}

void region_settings::finalize()
{
    // So the data definition goes from z = OVERMAP_HEIGHT to z = OVERMAP_DEPTH
    std::reverse( default_oter.begin(), default_oter.end() );
}

void region_settings::finalize_all()
{
    forest_biome_feature_factory.finalize();

    forest_biome_mapgen_factory.finalize();
    region_terrain_furniture_factory.finalize();
    map_extra_collection_factory.finalize();

    region_settings_river_factory.finalize();
    region_settings_lake_factory.finalize();
    region_settings_ocean_factory.finalize();
    region_settings_ravine_factory.finalize();
    region_settings_forest_factory.finalize();
    region_settings_highway_factory.finalize();
    region_settings_forest_trail_factory.finalize();
    region_settings_forest_mapgen_factory.finalize();
    region_settings_city_factory.finalize();
    region_settings_map_extras_factory.finalize();
    region_settings_terrain_furniture_factory.finalize();

    region_settings_factory.finalize();

    if( !DEFAULT_REGION.is_valid() ) {
        debugmsg( "id: `default` region settings were not loaded or failed to load" );
    }
}

void groundcover_extra::finalize()   // FIXME: return bool for failure
{
    default_ter = ter_id( default_ter_str );

    ter_furn_id tf_id;
    int wtotal = 0;
    int btotal = 0;

    for( std::map<std::string, double>::const_iterator it = percent_str.begin();
         it != percent_str.end(); ++it ) {
        tf_id = ter_furn_id();
        if( it->second < 0.0001 ) {
            continue;
        }

        bool resolved = tf_id.resolve( it->first );

        if( !resolved ) {
            debugmsg( "No clue what '%s' is!  No such terrain or furniture", it->first.c_str() );
            continue;
        }
        wtotal += static_cast<int>( it->second * 10000.0 );
        weightlist[ wtotal ] = tf_id;
    }

    for( std::map<std::string, double>::const_iterator it = boosted_percent_str.begin();
         it != boosted_percent_str.end(); ++it ) {
        tf_id = ter_furn_id();
        if( it->second < 0.0001 ) {
            continue;
        }
        const ter_str_id tid( it->first );
        const furn_str_id fid( it->first );

        bool resolved = tf_id.resolve( it->first );
        if( !resolved ) {
            debugmsg( "No clue what '%s' is!  No such terrain or furniture", it->first.c_str() );
            continue;
        }
        btotal += static_cast<int>( it->second * 10000.0 );
        boosted_weightlist[ btotal ] = tf_id;
    }

    if( wtotal > 1000000 ) {
        std::stringstream ss;
        for( auto it = percent_str.begin(); it != percent_str.end(); ++it ) {
            if( it != percent_str.begin() ) {
                ss << '+';
            }
            ss << it->second;
        }
        debugmsg( "plant coverage total (%s=%de-4) exceeds 100%%", ss.str(), wtotal );
    }
    if( btotal > 1000000 ) {
        std::stringstream ss;
        for( auto it = boosted_percent_str.begin(); it != boosted_percent_str.end(); ++it ) {
            if( it != boosted_percent_str.begin() ) {
                ss << '+';
            }
            ss << it->second;
        }
        debugmsg( "boosted plant coverage total (%s=%de-4) exceeds 100%%", ss.str(), btotal );
    }

    tf_id.ter_furn = default_ter;
    weightlist[ 1000000 ] = tf_id;
    boosted_weightlist[ 1000000 ] = tf_id;

    percent_str.clear();
    boosted_percent_str.clear();
}

ter_furn_id groundcover_extra::pick( bool boosted ) const
{
    if( boosted ) {
        return boosted_weightlist.lower_bound( rng( 0, 1000000 ) )->second;
    }
    return weightlist.lower_bound( rng( 0, 1000000 ) )->second;
}

ter_furn_id forest_biome_mapgen::pick() const
{
    // Iterate through the biome components (which have already been put into sequence), roll for the
    // one_in chance that component contributes a feature, and if so pick that feature and return it.
    // If a given component does not roll as success, proceed to the next feature in sequence until
    // a feature is picked or none are picked, in which case an empty feature is returned.
    const ter_furn_id *result = nullptr;
    for( const forest_biome_component_id &pr : biome_components ) {
        if( one_in( pr->chance ) ) {
            result = pr->types.pick();
            break;
        }
    }

    if( result == nullptr ) {
        return ter_furn_id();
    }

    return *result;
}

void forest_biome_mapgen::finalize()
{
    for( auto &pr : terrain_dependent_furniture ) {
        pr.second.finalize();
    }
}

void region_settings_forest_mapgen::finalize()
{
    for( const forest_biome_mapgen_id &biome_id : biomes ) {
        for( const oter_type_id ott_id : biome_id->terrains ) {
            oter_to_biomes[ott_id] = biome_id;
        }
    }
}

void region_settings_forest_trail::finalize()
{
    trailheads.finalize();
}

void region_settings_city::finalize()
{
    houses.finalize();
    shops.finalize();
    parks.finalize();
}

//these could be defined in the future
void forest_biome_component::finalize() {}
void forest_biome_terrain_dependent_furniture_new::finalize() {}
void region_settings_river::finalize() {}
void region_settings_lake::finalize() {}
void region_settings_ocean::finalize() {}
void region_settings_forest::finalize() {}
void region_settings_ravine::finalize() {}
void region_settings_terrain_furniture::finalize() {}
void region_terrain_furniture::finalize() {}

void region_settings_highway::finalize()
{
    //finds longest special in a building collection
    auto find_longest_special = []( const building_bin & b ) {
        int longest_length = 0;
        for( const auto &weighted_pair : b.get_all_buildings() ) {
            const overmap_special_id &special = weighted_pair.first;
            int spec_length = special.obj().longest_side();
            if( spec_length > longest_length ) {
                longest_length = spec_length;
            }
        }
        return longest_length;
    };

    four_way_intersections.finalize();
    three_way_intersections.finalize();
    bends.finalize();
    road_connections.finalize();
    interchanges.finalize();

    longest_bend_length = find_longest_special( bends );
    longest_slant_length = std::max( clockwise_slant->longest_side(),
                                     counterclockwise_slant->longest_side() );
    HIGHWAY_MAX_DEVIANCE = ( longest_bend_length + 1 ) * 2;
}

map_extra_collection map_extra_collection::filtered_by( const mapgendata &dat ) const
{
    map_extra_collection result( chance );
    for( const std::pair<map_extra_id, int> &obj : values ) {
        const map_extra_id &extra_id = obj.first;
        if( extra_id->is_valid_for( dat ) ) {
            result.values.add( extra_id, obj.second );
        }
    }
    if( !values.empty() && result.values.empty() ) {
        // OMT is too tall / too deep for all map extras. Skip map extra generation.
        result.chance = 0;
    }
    return result;
}

ter_id region_settings_terrain_furniture::resolve( const ter_id &tid ) const
{
    if( tid.id().is_null() ) {
        return tid;
    }
    ter_id result = tid;
    auto predicate = [&result]( const region_terrain_furniture_id & tid_r ) {
        return tid_r->replaced_ter_id == result;
    };
    auto found_tfid = std::find_if( ter_furn.begin(), ter_furn.end(), predicate );
    while( found_tfid != ter_furn.end() ) {
        const region_terrain_furniture_id &rtf_id = *found_tfid;
        result = *rtf_id->terrain.pick();
        found_tfid = std::find_if( ter_furn.begin(), ter_furn.end(), predicate );
    }
    return result;
}

furn_id region_settings_terrain_furniture::resolve( const furn_id &fid ) const
{
    if( fid.id().is_null() ) {
        return fid;
    }
    furn_id result = fid;
    auto predicate = [&result]( const region_terrain_furniture_id & fid_r ) {
        return fid_r->replaced_furn_id == result;
    };
    auto found_tfid = std::find_if( ter_furn.begin(), ter_furn.end(), predicate );
    while( found_tfid != ter_furn.end() ) {
        const region_terrain_furniture_id &rtf_id = *found_tfid;
        result = *rtf_id->furniture.pick();
        found_tfid = std::find_if( ter_furn.begin(), ter_furn.end(), predicate );
    }
    return result;
}

void building_bin::add( const overmap_special_id &building, int weight )
{
    if( finalized ) {
        debugmsg( "Tried to add special %s to a finalized building bin", building.c_str() );
        return;
    }

    buildings.add( building, weight );
}

overmap_special_id building_bin::pick() const
{
    if( !finalized ) {
        debugmsg( "Tried to pick a special out of a non-finalized bin" );
        overmap_special_id null_special( "null" );
        return null_special;
    }

    return *buildings.pick();
}

void building_bin::clear()
{
    finalized = false;
    buildings.clear();
}

void building_bin::finalize()
{
    if( finalized ) {
        debugmsg( "Tried to finalize a finalized bin (that's a code-side error which can't be fixed with jsons)" );
        return;
    }

    weighted_int_list< overmap_special_id> new_buildings;
    for( const std::pair<const overmap_special_id, int> pr : buildings ) {
        overmap_special_id current_id = pr.first;
        if( !current_id.is_valid() ) {
            // First, try to convert oter to special
            string_id<oter_type_t> converted_id( pr.first.str() );
            if( !converted_id.is_valid() ) {
                debugmsg( "Tried to add city building %s, but it is neither a special nor a terrain type",
                          pr.first.c_str() );
                continue;
            }
            current_id = overmap_specials::create_building_from( converted_id );
        }
        new_buildings.add( current_id, pr.second );
    }

    buildings = new_buildings;
    finalized = true;
}
