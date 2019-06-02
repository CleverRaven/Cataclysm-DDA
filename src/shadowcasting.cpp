#include "shadowcasting.h"

#include <list>

#include "enums.h"
#include "fragment_cloud.h" // IWYU pragma: keep
#include "line.h"

extern int fov_3d_z_range;

template<typename T>
struct span {
    span( const float &s_major, const float &e_major,
          const float &s_minor, const float &e_minor,
          const T &value ) :
        start_major( s_major ), end_major( e_major ), start_minor( s_minor ), end_minor( e_minor ),
        cumulative_value( value ) {}
    // TODO: Make these fixed-point or byte/byte pairs
    float start_major;
    float end_major;
    float start_minor;
    float end_minor;
    T cumulative_value;
};

// Add defaults for when method is invoked for the first time.
template<int xx, int xy, int xz, int yx, int yy, int yz, int zz, typename T,
         T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         T( *accumulate )( const T &, const T &, const int & )>
void cast_zlight_segment(
    const array_of_grids_of<T> &output_caches,
    const array_of_grids_of<const T> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const tripoint &offset, const int offset_distance,
    const T numerator = 1.0f );

template<int xx, int xy, int xz, int yx, int yy, int yz, int zz, typename T,
         T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         T( *accumulate )( const T &, const T &, const int & )>
void cast_zlight_segment(
    const array_of_grids_of<T> &output_caches,
    const array_of_grids_of<const T> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const tripoint &offset, const int offset_distance,
    const T numerator )
{
    const float radius = 60.0f - offset_distance;

    constexpr int min_z = -OVERMAP_DEPTH;
    constexpr int max_z = OVERMAP_HEIGHT;

    float new_start_minor = 1.0f;

    T last_intensity = 0.0;
    tripoint delta( 0, 0, 0 );
    tripoint current( 0, 0, 0 );
    // TODO: More optimal data structure.
    // We start out with one span covering the entire horizontal and vertical space
    // we are interested in.  Then as changes in transparency are encountered, we truncate
    // that initial span and insert new spans after it in the list.
    std::list<span<T>> spans = { { 0.0, 1.0, 0.0, 1.0, LIGHT_TRANSPARENCY_OPEN_AIR } };
    // At each "depth", a.k.a. distance from the origin, we iterate once over the list of spans,
    // possibly splitting them.
    for( int distance = 1; distance <= radius; distance++ ) {
        delta.y = distance;
        bool started_block = false;
        T current_transparency = 0.0f;

        for( auto it = spans.begin(); it != spans.end(); ++it ) {
            span<T> &this_span = *it;
            // TODO: Precalculate min/max delta.z based on start/end and distance
            for( delta.z = 0; delta.z <= distance; delta.z++ ) {
                float trailing_edge_major = ( delta.z - 0.5f ) / ( delta.y + 0.5f );
                float leading_edge_major = ( delta.z + 0.5f ) / ( delta.y - 0.5f );
                current.z = offset.z + delta.x * 00 + delta.y * 00 + delta.z * zz;
                if( current.z > max_z || current.z < min_z ) {
                    continue;
                } else if( this_span.start_major > leading_edge_major ) {
                    // Current span has a higher z-value,
                    // jump to next iteration to catch up.
                    continue;
                } else if( this_span.end_major < trailing_edge_major ) {
                    // We've escaped the bounds of the current span we're considering,
                    // So continue to the next span.
                    break;
                }

                bool started_span = false;
                const int z_index = current.z + OVERMAP_DEPTH;
                for( delta.x = 0; delta.x <= distance; delta.x++ ) {
                    current.x = offset.x + delta.x * xx + delta.y * xy + delta.z * xz;
                    current.y = offset.y + delta.x * yx + delta.y * yy + delta.z * yz;
                    // Shadowcasting sweeps from the most extreme edge of the octant to the cardinal
                    // XXXX
                    // <---
                    // XXX
                    // <--
                    // XX
                    // <-
                    // X
                    // @
                    //
                    // Leading edge -> +-
                    //                 |+| <- Center of tile
                    //                  -+ <- Trailing edge
                    //               <------ Direction of sweep
                    // Use corners of given tile as above to determine angles of
                    // leading and trailing edges being considered.
                    float trailing_edge_minor = ( delta.x - 0.5f ) / ( delta.y + 0.5f );
                    float leading_edge_minor = ( delta.x + 0.5f ) / ( delta.y - 0.5f );

                    if( !( current.x >= 0 && current.y >= 0 && current.x < MAPSIZE_X &&
                           current.y < MAPSIZE_Y ) || this_span.start_minor > leading_edge_minor ) {
                        // Current tile comes before span we're considering, advance to the next tile.
                        continue;
                    } else if( this_span.end_minor < trailing_edge_minor ) {
                        // Current tile is after the span we're considering, continue to next row.
                        break;
                    }

                    T new_transparency = ( *input_arrays[z_index] )[current.x][current.y];
                    // If we're looking at a tile with floor or roof from the floor/roof side,
                    //  that tile is actually invisible to us.
                    bool floor_block = false;
                    if( current.z < offset.z ) {
                        if( z_index < ( OVERMAP_LAYERS - 1 ) &&
                            ( *floor_caches[z_index + 1] )[current.x][current.y] ) {
                            floor_block = true;
                            new_transparency = LIGHT_TRANSPARENCY_SOLID;
                        }
                    } else if( current.z > offset.z ) {
                        if( ( *floor_caches[z_index] )[current.x][current.y] ) {
                            floor_block = true;
                            new_transparency = LIGHT_TRANSPARENCY_SOLID;
                        }
                    }

                    if( !started_block ) {
                        started_block = true;
                        current_transparency = new_transparency;
                    }

                    const int dist = rl_dist( tripoint_zero, delta ) + offset_distance;
                    last_intensity = calc( numerator, this_span.cumulative_value, dist );

                    if( !floor_block ) {
                        ( *output_caches[z_index] )[current.x][current.y] =
                            std::max( ( *output_caches[z_index] )[current.x][current.y], last_intensity );
                    }

                    if( !started_span ) {
                        // Need to reset minor slope, because we're starting a new line
                        new_start_minor = leading_edge_minor;
                        // Need more precision or artifacts happen
                        leading_edge_minor = this_span.start_minor;
                        started_span = true;
                    }

                    if( new_transparency == current_transparency ) {
                        // All in order, no need to split the span.
                        new_start_minor = leading_edge_minor;
                        continue;
                    }

                    T next_cumulative_transparency = accumulate( this_span.cumulative_value,
                                                     current_transparency, distance );
                    // We split the span into up to 4 sub-blocks (sub-frustums actually,
                    // this is the view from the origin looking out):
                    // +-------+ <- end major
                    // |   D   |
                    // +---+---+ <- ???
                    // | B | C |
                    // +---+---+ <- major mid
                    // |   A   |
                    // +-------+ <- start major
                    // ^       ^
                    // |       end minor
                    // start minor
                    // A is previously processed row(s). This might be empty.
                    // B is already-processed tiles from current row. This must exist.
                    // C is remainder of current row. This must exist.
                    // D is not yet processed row(s). Might be empty.
                    // A, B and D have the previous transparency, C has the new transparency,
                    //   which might be opaque.
                    // One we processed fully in 2D and only need to extend in last D
                    // Only emit a new span horizontally if previous span was not opaque.
                    // If end_minor is <= trailing_edge_minor, A, B, C remain one span and
                    //  D becomes a new span if present.
                    // If this is the first row processed in the current span, there is no A span.
                    // If this is the last row processed, there is no D span.
                    // If check returns false, A and B are opaque and have no spans.
                    if( check( current_transparency, last_intensity ) ) {
                        // Emit the A span if it's present.
                        if( trailing_edge_major > this_span.start_major ) {
                            // This is BCD
                            spans.emplace( std::next( it ),
                                           trailing_edge_major, this_span.end_major,
                                           this_span.start_minor, this_span.end_minor,
                                           this_span.cumulative_value );
                            // All we do to A is truncate it.
                            this_span.end_major = trailing_edge_major;
                            // Then make the current span the one we just inserted.
                            it++;
                        }
                        // One way or the other, the current span is now BCD.
                        // First handle B.
                        const float major_mid = std::max( this_span.start_major,
                                                          trailing_edge_major );
                        const float major_mid_end = std::min( this_span.end_major,
                                                              leading_edge_major );
                        spans.emplace( it, major_mid, major_mid_end,
                                       this_span.start_minor, leading_edge_minor,
                                       next_cumulative_transparency );
                        // Overwrite new_start_minor since previous tile is transparent.
                        new_start_minor = trailing_edge_minor;
                    }
                    // Current span is BCD, but B either doesn't exist or is handled.
                    // Split off D if it exists.
                    if( leading_edge_major < this_span.end_major ) {
                        spans.emplace( std::next( it ),
                                       leading_edge_major, this_span.end_major,
                                       this_span.start_minor, this_span.end_minor,
                                       this_span.cumulative_value );
                    }
                    // Truncate this_span to the current block.
                    this_span.end_major = std::min( this_span.end_major, leading_edge_major );
                    this_span.start_major = std::max( this_span.start_major, trailing_edge_major );
                    // The new span starts at the leading edge of the previous square if it is opaque,
                    // and at the trailing edge of the current square if it is transparent.
                    this_span.start_minor = new_start_minor;
                    this_span.cumulative_value = next_cumulative_transparency;

                    new_start_minor = leading_edge_minor;
                    current_transparency = new_transparency;
                }

                if( !check( current_transparency, last_intensity ) ) {
                    this_span.start_major = leading_edge_major;
                }
            }

            if( !started_block ) {
                // If we didn't scan at least 1 z-level, don't iterate further
                // Otherwise we may "phase" through tiles without checking them
                break;
            }

            if( !check( current_transparency, last_intensity ) ) {
                // If we reach the end of the span with terrain being opaque, we don't iterate further.
                break;
            }
            // Cumulative average of the values encountered.
            this_span.cumulative_value = accumulate( this_span.cumulative_value,
                                         current_transparency, distance );
        }
    }
}

template<typename T, T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         T( *accumulate )( const T &, const T &, const int & )>
void cast_zlight(
    const array_of_grids_of<T> &output_caches,
    const array_of_grids_of<const T> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const tripoint &origin, const int offset_distance, const T numerator )
{
    // Down
    cast_zlight_segment < 0, 1, 0, 1, 0, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment < 1, 0, 0, 0, 1, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

    cast_zlight_segment < 0, -1, 0, 1, 0, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment < -1, 0, 0, 0, 1, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

    cast_zlight_segment < 0, 1, 0, -1, 0, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment < 1, 0, 0, 0, -1, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

    cast_zlight_segment < 0, -1, 0, -1, 0, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment < -1, 0, 0, 0, -1, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

    // Up
    cast_zlight_segment<0, 1, 0, 1, 0, 0, 1, T, calc, check, accumulate>(
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment<1, 0, 0, 0, 1, 0, 1, T, calc, check, accumulate>(
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

    cast_zlight_segment < 0, -1, 0, 1, 0, 0, 1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment < -1, 0, 0, 0, 1, 0, 1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

    cast_zlight_segment < 0, 1, 0, -1, 0, 0, 1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment < 1, 0, 0, 0, -1, 0, 1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

    cast_zlight_segment < 0, -1, 0, -1, 0, 0, 1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment < -1, 0, 0, 0, -1, 0, 1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
}

// I can't figure out how to make implicit instantiation work when the parameters of
// the template-supplied function pointers are involved, so I'm explicitly instantiating instead.
template void cast_zlight<float, sight_calc, sight_check, accumulate_transparency>(
    const std::array<float ( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &output_caches,
    const std::array<const float ( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &input_arrays,
    const std::array<const bool ( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &floor_caches,
    const tripoint &origin, const int offset_distance, const float numerator );

template void cast_zlight<fragment_cloud, shrapnel_calc, shrapnel_check, accumulate_fragment_cloud>(
    const std::array<fragment_cloud( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &output_caches,
    const std::array<const fragment_cloud( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS>
    &input_arrays,
    const std::array<const bool ( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &floor_caches,
    const tripoint &origin, const int offset_distance, const fragment_cloud numerator );
