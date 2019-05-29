#include "shadowcasting.h"

#include <vector>

#include "enums.h"
#include "fragment_cloud.h" // IWYU pragma: keep
#include "line.h"

extern int fov_3d_z_range;

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
    const T numerator = 1.0f, const int row = 1,
    float start_major = 0.0f, const float end_major = 1.0f,
    float start_minor = 0.0f, const float end_minor = 1.0f,
    T cumulative_transparency = LIGHT_TRANSPARENCY_OPEN_AIR );

template<int xx, int xy, int xz, int yx, int yy, int yz, int zz, typename T,
         T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         T( *accumulate )( const T &, const T &, const int & )>
void cast_zlight_segment(
    const array_of_grids_of<T> &output_caches,
    const array_of_grids_of<const T> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const tripoint &offset, const int offset_distance,
    const T numerator, const int row,
    float start_major, const float end_major,
    float start_minor, const float end_minor,
    T cumulative_transparency )
{
    if( start_major >= end_major || start_minor >= end_minor ) {
        return;
    }

    float radius = 60.0f - offset_distance;

    constexpr int min_z = -OVERMAP_DEPTH;
    constexpr int max_z = OVERMAP_HEIGHT;

    float new_start_minor = 1.0f;

    T last_intensity = 0.0;
    tripoint delta( 0, 0, 0 );
    tripoint current( 0, 0, 0 );
    for( int distance = row; distance <= radius; distance++ ) {
        delta.y = distance;
        bool started_block = false;
        T current_transparency = 0.0f;

        // TODO: Precalculate min/max delta.z based on start/end and distance
        for( delta.z = 0; delta.z <= std::min( fov_3d_z_range, distance ); delta.z++ ) {
            float trailing_edge_major = ( delta.z - 0.5f ) / ( delta.y + 0.5f );
            float leading_edge_major = ( delta.z + 0.5f ) / ( delta.y - 0.5f );
            current.z = offset.z + delta.x * 00 + delta.y * 00 + delta.z * zz;
            if( current.z > max_z || current.z < min_z ) {
                continue;
            } else if( start_major > leading_edge_major ) {
                continue;
            } else if( end_major < trailing_edge_major ) {
                break;
            }

            bool started_span = false;
            const int z_index = current.z + OVERMAP_DEPTH;
            for( delta.x = 0; delta.x <= distance; delta.x++ ) {
                current.x = offset.x + delta.x * xx + delta.y * xy + delta.z * xz;
                current.y = offset.y + delta.x * yx + delta.y * yy + delta.z * yz;
                float trailing_edge_minor = ( delta.x - 0.5f ) / ( delta.y + 0.5f );
                float leading_edge_minor = ( delta.x + 0.5f ) / ( delta.y - 0.5f );

                if( !( current.x >= 0 && current.y >= 0 &&
                       current.x < MAPSIZE_X &&
                       current.y < MAPSIZE_Y ) || start_minor > leading_edge_minor ) {
                    continue;
                } else if( end_minor < trailing_edge_minor ) {
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
                last_intensity = calc( numerator, cumulative_transparency, dist );

                if( !floor_block ) {
                    ( *output_caches[z_index] )[current.x][current.y] =
                        std::max( ( *output_caches[z_index] )[current.x][current.y], last_intensity );
                }

                if( !started_span ) {
                    // Need to reset minor slope, because we're starting a new line
                    new_start_minor = leading_edge_minor;
                    // Need more precision or artifacts happen
                    leading_edge_minor = start_minor;
                    started_span = true;
                }

                if( new_transparency == current_transparency ) {
                    // All in order, no need to recurse
                    new_start_minor = leading_edge_minor;
                    continue;
                }

                // We split the block into 4 sub-blocks (sub-frustums actually, this is the view from the origin looking out):
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
                // A is previously processed row(s).
                // B is already-processed tiles from current row.
                // C is remainder of current row.
                // D is not yet processed row(s).
                // One we processed fully in 2D and only need to extend in last D
                // Only cast recursively horizontally if previous span was not opaque.
                if( check( current_transparency, last_intensity ) ) {
                    T next_cumulative_transparency = accumulate( cumulative_transparency, current_transparency,
                                                     distance );
                    // Blocks can be merged if they are actually a single rectangle
                    // rather than rectangle + line shorter than rectangle's width
                    const bool merge_blocks = end_minor <= trailing_edge_minor;
                    // trailing_edge_major can be less than start_major
                    const float trailing_clipped = std::max( trailing_edge_major, start_major );
                    const float major_mid = merge_blocks ? leading_edge_major : trailing_clipped;
                    cast_zlight_segment<xx, xy, xz, yx, yy, yz, zz, T, calc, check, accumulate>(
                        output_caches, input_arrays, floor_caches,
                        offset, offset_distance, numerator, distance + 1,
                        start_major, major_mid, start_minor, end_minor,
                        next_cumulative_transparency );
                    if( !merge_blocks ) {
                        // One line that is too short to be part of the rectangle above
                        cast_zlight_segment<xx, xy, xz, yx, yy, yz, zz, T, calc, check, accumulate>(
                            output_caches, input_arrays, floor_caches,
                            offset, offset_distance, numerator, distance + 1,
                            major_mid, leading_edge_major, start_minor, trailing_edge_minor,
                            next_cumulative_transparency );
                    }
                }

                // One from which we shaved one line ("processed in 1D")
                const float old_start_minor = start_minor;
                // The new span starts at the leading edge of the previous square if it is opaque,
                // and at the trailing edge of the current square if it is transparent.
                if( !check( current_transparency, last_intensity ) ) {
                    start_minor = new_start_minor;
                } else {
                    // Note this is the same slope as one of the recursive calls we just made.
                    start_minor = std::max( start_minor, trailing_edge_minor );
                    start_major = std::max( start_major, trailing_edge_major );
                }

                // leading_edge_major plus some epsilon
                float after_leading_edge_major = ( delta.z + 0.50001f ) / ( delta.y - 0.5f );
                cast_zlight_segment<xx, xy, xz, yx, yy, yz, zz, T, calc, check, accumulate>(
                    output_caches, input_arrays, floor_caches,
                    offset, offset_distance, numerator, distance,
                    after_leading_edge_major, end_major, old_start_minor, start_minor,
                    cumulative_transparency );

                // One we just entered ("processed in 0D" - the first point)
                // No need to recurse, we're processing it right now

                current_transparency = new_transparency;
                new_start_minor = leading_edge_minor;
            }

            if( !check( current_transparency, last_intensity ) ) {
                start_major = leading_edge_major;
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
        cumulative_transparency = accumulate( cumulative_transparency, current_transparency, distance );
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
