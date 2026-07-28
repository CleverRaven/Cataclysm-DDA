#include "cata_utility.h"
#include "coordinates.h"
#include "debug.h"
#include "enum_conversions.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "generic_factory.h"
#include "imgui/imgui.h"

// IWYU pragma: begin_keep
#define JC_VORONOI_IMPLEMENTATION // NOLINT(clang-diagnostic-unused-macros)
// IWYU pragma: end_keep
#include "jc_voronoi/jc_voronoi.h"
#include "line.h"
#include "map_iterator.h"
#include "output.h"
#include "overmap_worldgen.h"
#include "overmapbuffer.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "units.h"

#include <cstring>
#include <unordered_set>
#include <utility>
#include <vector>

struct region_settings;

static const dimension_region_layout_id dimension_region_layout_default( "default" );

void overmapbuffer::init_region_layout()
{
    const dimension_id current_dimension = g->get_dimension_prefix();
    const dimension_region_layout_id region_layout = current_dimension->get_region_layout();
    std::shared_ptr<dimension_region_layout_generator> layout_generator =
        region_layout->get_generator();
    if( !layout_generator ) {
        debugmsg( string_format( "no region layout generator defined/loaded for dimension %s",
                                 current_dimension.str() ) );
    } else {
        region_layout->get_generator()->init();
    }
}

void region_voronoi_point::deserialize( const JsonObject &jo )
{
    jo.read( "point", region_point );
    jo.read( "region", region );
}

// converts a JCV point to a CATA point
// generalize with template if needed beyond regions
static tripoint_abs_om jcv_to_cata_point( const jcv_point &loc )
{
    return tripoint_abs_om( static_cast<int>( loc.x ), -static_cast<int>( loc.y ), 0 );
}

static void generate_cata_voronoi_map( Region_map &placed_regions,
                                       const std::vector<region_voronoi_point> &region_points,
                                       const inclusive_cuboid<tripoint_abs_om> &generated_bounds )
{
    jcv_diagram diagram;
    memset( &diagram, 0, sizeof( jcv_diagram ) );
    std::vector<jcv_point> jcv_points;

    // temporary map for `region_points`
    std::unordered_map<tripoint_abs_om, region_settings_id> site_regions;
    // y is negated because jcvoronoi flips y-axis
    for( const region_voronoi_point &rv_point : region_points ) {
        const jcv_point temp_point = { static_cast<float>( rv_point.region_point.x() ),
                                       -static_cast<float>( rv_point.region_point.y() )
                                     };
        jcv_points.emplace_back( temp_point );
        site_regions.emplace( rv_point.region_point, rv_point.region );
    }

    const jcv_point bounds_min = { static_cast<float>( generated_bounds.p_min.x() ), static_cast<float>( generated_bounds.p_min.y() ) };
    const jcv_point bounds_max = { static_cast<float>( generated_bounds.p_max.x() ), static_cast<float>( generated_bounds.p_max.y() ) };
    const jcv_rect jcv_bounds = { bounds_min, bounds_max };

    jcv_diagram_generate( region_points.size(), &*jcv_points.begin(), &jcv_bounds, nullptr, &diagram );

    const jcv_site *sites = jcv_diagram_get_sites( &diagram );
    for( int i = 0; i < diagram.numsites; ++i ) {
        const jcv_site *site = &sites[i];
        const tripoint_abs_om site_point = jcv_to_cata_point( site->p );
        region_settings_id site_region = site_regions.at( site_point );

        // add all points contained in a cell polygon by drawing triangles
        // from the center point to the cell edges
        const jcv_graphedge *e = site->edges;
        while( e ) {
            std::vector<tripoint_abs_om> cell_points = points_in_triangle_2d(
                        site_point, jcv_to_cata_point( e->pos[0] ), jcv_to_cata_point( e->pos[1] ) );
            for( const tripoint_abs_om &cell_point : cell_points ) {
                if( !placed_regions.count( cell_point ) ) {
                    placed_regions[cell_point] = site_region;
                }
            }

            e = e->next;
        }
    }
}

void dimension_region_layout_generator_uniform::generate_dynamic(
    Region_map &placed_regions, const tripoint_abs_om &current_om )
{
    placed_regions.emplace( current_om, uniform_region );
}

void dimension_region_layout_generator_manual::generate_entire_layout(
    Region_map &placed_regions, const tripoint_abs_om & )
{
    std::vector<region_voronoi_point> region_points;
    for( const region_point_set &rps : region_point_sets ) {
        std::vector<region_voronoi_point> resolved_points = rps.resolve();
        for( const region_voronoi_point &rvp : resolved_points ) {
            region_points.emplace_back( rvp );
        }
    }
    generate_cata_voronoi_map( placed_regions, region_points, generated_bounds );
}

void dimension_region_layout_generator_random::generate_dynamic(
    Region_map &placed_regions, const tripoint_abs_om &current_om )
{
    placed_regions.emplace( current_om, *random_regions.pick() );
}

void dimension_region_layout_generator_angles::generate_dynamic(
    Region_map &placed_regions, const tripoint_abs_om &current_om )
{

    units::angle angle_diff = coord_to_angle( tripoint_abs_om::zero, current_om );
    // align sectors towards north
    angle_diff += angles_offset;

    const units::angle sector_size = units::from_degrees( 360.0f / static_cast<double>
                                     ( get_sector_count() ) );
    int sector_index = modulo( static_cast<int>( angle_diff / sector_size ),
                               get_sector_count() );
    placed_regions.emplace( current_om, angles_regions[sector_index] );
}

region_settings_id dimension_region_layout_static::get_overmap_region(
    const tripoint_abs_om &om_point )
{
    // in-bounds (static)
    if( overmap_in_bounds( om_point ) ) {
        if( overmap_buffer.global_state.placed_regions.empty() ) {
            generate_entire_layout( overmap_buffer.global_state.placed_regions, om_point );
        }
        return overmap_buffer.global_state.placed_regions[om_point];
    }
    // out-of-bounds (dynamic)
    else {
        return layout_out_of_bounds->get_generator()->get_overmap_region( om_point );
    }
}

region_settings_id dimension_region_layout_dynamic::get_overmap_region(
    const tripoint_abs_om &om_point )
{
    generate_dynamic( overmap_buffer.global_state.placed_regions, om_point );
    return overmap_buffer.global_state.placed_regions[om_point];
}

region_settings_id overmapbuffer::get_overmap_region( tripoint_abs_om om_point )
{
    dimension_region_layout_id region_layout = g->get_dimension_prefix()->get_region_layout();

    const std::shared_ptr<dimension_region_layout_generator> &layout_generator =
        region_layout->get_generator();
    return layout_generator->get_overmap_region( om_point );
}

namespace io
{
template<>
std::string enum_to_string<regions_generation_mode>( regions_generation_mode data )
{
    switch( data ) {
        case regions_generation_mode::UNIFORM:
            return "UNIFORM";
        case regions_generation_mode::MANUAL_VORONOI:
            return "MANUAL_VORONOI";
        case regions_generation_mode::RANDOM:
            return "RANDOM";
        case regions_generation_mode::ANGLES:
            return "ANGLES";
        default:
            return "last";
    }
    cata_fatal( "Invalid regions_generation_mode" );
}
} // namespace io

namespace
{
generic_factory<dimension_region_layout> dimension_regions_factory( "dimension_region_layout" );
generic_factory<dimension_world> dimension_factory( "dimension" );
} // namespace

template<>
const dimension_region_layout &string_id<dimension_region_layout>::obj() const
{
    return dimension_regions_factory.obj( *this );
}
template<>
const dimension_world &string_id<dimension_world>::obj() const
{
    return dimension_factory.obj( *this );
}
template<>
bool string_id<dimension_region_layout>::is_valid() const
{
    return dimension_regions_factory.is_valid( *this );
}
template<>
bool string_id<dimension_world>::is_valid() const
{
    return dimension_factory.is_valid( *this );
}
void dimension_region_layout::load_dimension_regions( const JsonObject &jo,
        const std::string &src )
{
    dimension_regions_factory.load( jo, src );
}
void dimension_world::load_dimensions( const JsonObject &jo,
                                       const std::string &src )
{
    dimension_factory.load( jo, src );
}

void dimension_world::load( const JsonObject &jo, const std::string_view & )
{
    jo.read( "region_layout", region_layout );
}

void dimension_world::finalize()
{
    if( !region_layout.is_valid() ) {
        debugmsg( string_format( "invalid region layout %s loaded for dimension %s; switching to default",
                                 region_layout.str(), id.str() ) );
        region_layout = dimension_region_layout_default;
    }
}

void dimension_world::finalize_all()
{
    dimension_factory.finalize();
}

template<typename T>
static void load_layout_generator( const JsonObject &jo,
                                   std::shared_ptr<dimension_region_layout_generator> &layout_generator )
{
    T generator;
    generator.deserialize( jo );
    layout_generator = std::make_unique<T>( generator );
}

void dimension_region_layout::load( const JsonObject &jo, const std::string_view & )
{

    jo.read( "generation_mode", generation_mode );
    switch( generation_mode ) {
        case regions_generation_mode::UNIFORM: {
            load_layout_generator<dimension_region_layout_generator_uniform>( jo, layout_generator );
            break;
        }
        case regions_generation_mode::MANUAL_VORONOI: {
            load_layout_generator<dimension_region_layout_generator_manual>( jo, layout_generator );
            break;
        }
        case regions_generation_mode::RANDOM: {
            load_layout_generator<dimension_region_layout_generator_random>( jo, layout_generator );
            break;
        }
        case regions_generation_mode::ANGLES: {
            load_layout_generator<dimension_region_layout_generator_angles>( jo, layout_generator );
            break;
        }
        default:
            break;
    }
}

void dimension_region_layout_generator_uniform::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "uniform_region", uniform_region );
}

void dimension_region_layout_generator_manual::deserialize( const JsonObject &jo )
{
    dimension_region_layout_static::deserialize( jo );
    mandatory( jo, false, "region_point_sets", region_point_sets );
}

void dimension_region_layout_generator_random::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "random_regions", random_regions );
}

void dimension_region_layout_generator_angles::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "angles_regions", angles_regions );
    optional( jo, false, "angles_offset", angles_offset );
    if( angles_regions.size() > 16 || angles_regions.size() < 2 ) {
        debugmsg( "ANGLES layout must have between 2 and 16 entries" );
    }
    if( angles_offset < 0_degrees || angles_offset >= 180_degrees ) {
        debugmsg( "ANGLES offset must be in range [0, 180)" );
    }
}

void dimension_region_layout_static::deserialize( const JsonObject &jo )
{
    tripoint_abs_om bounds_min;
    tripoint_abs_om bounds_max;
    mandatory( jo, false, "generated_bounds_min", bounds_min );
    mandatory( jo, false, "generated_bounds_max", bounds_max );
    generated_bounds = inclusive_cuboid<tripoint_abs_om>( bounds_min, bounds_max );
    mandatory( jo, false, "layout_out_of_bounds", layout_out_of_bounds );
}

void overmapbuffer::print_region_layout()
{
    const Region_map &placed_regions = overmap_buffer.global_state.placed_regions;
    // calculate bounds from placed regions
    std::vector<tripoint_abs_om> all_points;
    for( const auto &p : placed_regions ) {
        all_points.emplace_back( p.first );
    }

    tripoint_range<tripoint_abs_om> bounds( construct_min( all_points ), construct_max( all_points ) );

    std::unordered_set<region_settings_id> printed_regions;
    std::unordered_map<region_settings_id, char> printed_region_char;

    int x_delta = bounds.max().x() - bounds.min().x() + 1;

    std::string total_output;
    std::string line_output;
    for( const tripoint_abs_om &p : bounds ) {
        if( placed_regions.count( p ) ) {
            region_settings_id p_region = placed_regions.at( p );
            if( !printed_regions.count( p_region ) ) {
                printed_regions.insert( p_region );
                printed_region_char.emplace( p_region, static_cast<char>( 'A' + printed_region_char.size() ) );
            }

            line_output += printed_region_char[p_region];
        } else {
            line_output += ' ';
        }
        if( line_output.size() % x_delta == 0 ) {
            total_output += line_output + '\n';
            line_output.clear();
        }
    }

    for( const std::pair< const string_id<region_settings>, char> &region_char : printed_region_char ) {
        total_output += string_format( "%c = %s", region_char.second, region_char.first.str() ) + '\n';
    }

    //copy to clipboard
    ImGui::SetClipboardText( total_output.c_str() );
    popup( _( "Copied world region map to clipboard!" ) );
}

std::vector<region_voronoi_point> region_point_set::resolve() const
{
    std::vector<region_voronoi_point> returned_points;
    // make a copy of the weighted list to remove entries
    weighted_int_list<region_settings_id> regions_weighted_mod = regions_weighted;
    const int variance = region_point_variance;

    for( const tripoint_abs_om &original_point : region_points ) {
        region_voronoi_point new_rv_point;
        region_settings_id *selected_region_ptr = regions_weighted_mod.pick();
        region_settings_id selected_region = selected_region_ptr == nullptr ? default_region :
                                             *selected_region_ptr;
        if( remove_region ) {
            regions_weighted_mod.remove( selected_region );
        }
        new_rv_point.region_point = original_point + point_rel_om( rng( -variance, variance ),
                                    rng( -variance, variance ) );
        new_rv_point.region = selected_region;
        returned_points.emplace_back( new_rv_point );
    }
    return returned_points;
}

void region_point_set::deserialize( const JsonObject &jo )
{
    optional( jo, false, "region_points", region_points );
    optional( jo, false, "regions_weighted", regions_weighted );
    optional( jo, false, "remove_region", remove_region );
    optional( jo, false, "default_region", default_region );
    optional( jo, false, "region_point_variance", region_point_variance );
}
