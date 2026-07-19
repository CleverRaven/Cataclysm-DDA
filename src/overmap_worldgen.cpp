#include "coordinates.h"
#include "debug.h"
#include "enum_conversions.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "generic_factory.h"
#include "imgui/imgui.h"
#include "map_iterator.h"
#include "output.h"
#include "overmapbuffer.h"
#include "overmap_worldgen.h"
#include "string_formatter.h"
#include "translations.h"

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

void dimension_region_layout_generator_uniform::generate_dynamic(
    Region_map &placed_regions, const tripoint_abs_om &current_om )
{
    placed_regions.emplace( current_om, uniform_region );
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
        case regions_generation_mode::EIGHTHS:
            return "EIGHTHS";
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
        default:
            break;
    }
}

void dimension_region_layout_generator_uniform::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "uniform_region", uniform_region );
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

