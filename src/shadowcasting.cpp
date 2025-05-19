#include "shadowcasting.h"

#include <cstdint>
#include <cstdlib>
#include <iterator>

#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "fragment_cloud.h" // IWYU pragma: keep
#include "line.h"
#include "list.h"
#include "point.h"

// historically 8 bits is enough for rise and run, as a shadowcasting radius of 60
// readily fits within that space. larger shadowcasting volumes may require larger
// storage units; a radius of 120 definitely will not fit.
struct slope {
    slope( int_least8_t rise, int_least8_t run ) {
        // Ensure run is always positive for the inequality operators
        this->run = std::abs( run );
        if( run < 0 ) {
            this->rise = -rise;
        } else {
            this->rise = rise;
        }
    }

    // see above for commentary on types.
    int_least8_t rise;
    int_least8_t run;
};

static bool operator<( const slope &lhs, const slope &rhs )
{
    // a/b < c/d <=> a*d < c*b if b and d have the same sign.
    return lhs.rise * rhs.run < rhs.rise * lhs.run;
}
static bool operator>( const slope &lhs, const slope &rhs )
{
    return rhs < lhs;
}
static bool operator==( const slope &lhs, const slope &rhs )
{
    // a/b == c/d <=> a*d == c*b
    return lhs.rise * rhs.run == rhs.rise * lhs.run;
}

template<typename T>
struct span {
    span( const slope &s_major, const slope &e_major,
          const slope &s_minor, const slope &e_minor,
          const T &value, bool skip_first_row = false, bool skip_first_column = false ) :
        start_major( s_major ), end_major( e_major ), start_minor( s_minor ), end_minor( e_minor ),
        cumulative_value( value ), skip_first_row( skip_first_row ),
        skip_first_column( skip_first_column ) {}
    slope start_major;
    slope end_major;
    slope start_minor;
    slope end_minor;
    T cumulative_value;
    bool skip_first_row;
    bool skip_first_column;
};

/**
 * Handle splitting the current span in cast_horizontal_zlight_segment and
 * cast_vertical_zlight_segment to avoid as much code duplication as possible
 */
template<typename T, bool( *is_transparent )( const T &, const T & ), T( *accumulate )( const T &, const T &, const int & )>
static void split_span( cata::list<span<T>> &spans,
                        typename cata::list<span<T>>::iterator &this_span,
                        T &current_transparency, const T &new_transparency, const T &last_intensity,
                        const int distance, slope &new_start_minor,
                        const slope &trailing_edge_major, const slope &leading_edge_major,
                        const slope &trailing_edge_minor, const slope &leading_edge_minor )
{
    const T next_cumulative_transparency = accumulate( this_span->cumulative_value,
                                           current_transparency, distance );
    // We split the span into up to 4 sub-blocks (sub-frustums actually,
    // this is the view from the origin looking out):
    // +-------+ <- end major
    // |   D   |
    // +---+---+ <- leading edge major
    // | B | C |
    // +---+---+ <- trailing edge major
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
    if( is_transparent( current_transparency, last_intensity ) ) {
        // Emit the A span if present, placing it before the current span in the list
        // If the parent span is to skip the first column, inherit it.
        if( trailing_edge_major > this_span->start_major ) {
            spans.emplace( this_span,
                           this_span->start_major, trailing_edge_major,
                           this_span->start_minor, this_span->end_minor,
                           next_cumulative_transparency, false, this_span->skip_first_column );
        }

        // Emit the B span if present, placing it before the current span in the list
        // If the parent span is to skip the first column, inherit it.
        if( trailing_edge_minor > this_span->start_minor ) {
            spans.emplace( this_span,
                           std::max( this_span->start_major, trailing_edge_major ),
                           std::min( this_span->end_major, leading_edge_major ),
                           this_span->start_minor, trailing_edge_minor,
                           next_cumulative_transparency, false, this_span->skip_first_column );
        }

        // Overwrite new_start_minor since previous tile is transparent.
        new_start_minor = trailing_edge_minor;
    }

    // Emit the D span if present, placing it after the current span in the list
    // If the parent span is to skip the first column, inherit it.
    if( leading_edge_major < this_span->end_major ) {
        // Pass true to the span constructor to set skip_first_row to true
        // This prevents the same row we are currently checking being checked by the
        // new D span
        spans.emplace( std::next( this_span ),
                       leading_edge_major, this_span->end_major,
                       this_span->start_minor, this_span->end_minor,
                       this_span->cumulative_value, true, this_span->skip_first_column );
    }
    // If the split is due to two transparent squares with different transparency, set skip_first_column to true
    // This prevents the last column of B span being checked by the new C span
    if( is_transparent( current_transparency, last_intensity ) &&
        is_transparent( new_transparency, last_intensity ) ) {
        this_span->skip_first_column = true;
    } else {
        this_span->skip_first_column = false;

    }

    // Truncate this_span to the current block.
    this_span->start_major = std::max( this_span->start_major, trailing_edge_major );
    this_span->end_major = std::min( this_span->end_major, leading_edge_major );

    // The new span starts at the leading edge of the previous square if it is opaque,
    // and at the trailing edge of the current square if it is transparent.
    this_span->start_minor = new_start_minor;

    new_start_minor = leading_edge_minor;
    current_transparency = new_transparency;
}

template<int xx_transform, int xy_transform, int yx_transform, int yy_transform, int z_transform, typename T,
         T( *calc )( const T &, const T &, const int & ),
         bool( *is_transparent )( const T &, const T & ),
         T( *accumulate )( const T &, const T &, const int & )>
void cast_horizontal_zlight_segment(
    const array_of_grids_of<T> &output_caches,
    const array_of_grids_of<const T> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const tripoint_bub_ms &offset, const int offset_distance,
    const T numerator )
{
    const int radius = MAX_VIEW_DISTANCE - offset_distance;

    constexpr int min_z = -OVERMAP_DEPTH;
    constexpr int max_z = OVERMAP_HEIGHT;
    static half_open_rectangle<point_bub_ms> bounds( point_bub_ms::zero, point_bub_ms( MAPSIZE_X,
            MAPSIZE_Y ) );

    slope new_start_minor( 1, 1 );

    T last_intensity( 0.0 );
    tripoint_rel_ms delta;
    tripoint_bub_ms current;

    // We start out with one span covering the entire horizontal and vertical space
    // we are interested in.  Then as changes in transparency are encountered, we truncate
    // that initial span and insert new spans before/after it in the list, removing any that
    // are no longer needed as we go.
    cata::list<span<T>> spans = { {
            slope( 0, 1 ), slope( 1, 1 ),
            slope( 0, 1 ), slope( 1, 1 ),
            T( LIGHT_TRANSPARENCY_OPEN_AIR )
        }
    };
    // At each "depth", a.k.a. distance from the origin, we iterate once over the list of spans,
    // possibly splitting them.
    for( int distance = 1; distance <= radius; distance++ ) {
        delta.y() = distance;
        T current_transparency( 0.0f );

        for( auto this_span = spans.begin(); this_span != spans.end(); ) {
            bool started_block = false;
            // TODO: Precalculate min/max delta.z based on start/end and distance
            for( delta.z() = 0; delta.z() <= distance; delta.z()++ ) {
                // Shadowcasting sweeps from the cardinal to the most extreme edge of the octant
                // XXXX
                // --->
                // XXX
                // -->
                // XX
                // ->
                // X
                // @
                //
                // Trailing edge -> +-
                //                  |+| <- Center of tile
                //                   -+ <- Leading Edge
                //  Direction of sweep --->
                // Use corners of given tile as above to determine angles of
                // leading and trailing edges being considered.
                const slope trailing_edge_major( delta.z() * 2 - 1, delta.y() * 2 + 1 );
                const slope leading_edge_major( delta.z() * 2 + 1, delta.y() * 2 - 1 );
                current.z() = offset.z() + delta.z() * z_transform;
                if( current.z() > max_z || current.z() < min_z ) {
                    // Current tile is out of bounds, advance to the next tile.
                    continue;
                }
                if( this_span->start_major > leading_edge_major ) {
                    // Current span has a higher z-value,
                    // jump to next iteration to catch up.
                    continue;
                }
                if( this_span->skip_first_row && this_span->start_major == leading_edge_major ) {
                    // Prevents an infinite loop in some cases after splitting off the D span.
                    // We don't want to recheck the row that just caused the D span to be split off,
                    // since that can lead to an identical span being split off again, hence the
                    // infinite loop.
                    //
                    // This could also be accomplished by adding a small epsilon to the start_major
                    // of the D span but that causes artifacts.
                    continue;
                }
                if( this_span->end_major < trailing_edge_major ) {
                    // We've escaped the bounds of the current span we're considering,
                    // So continue to the next span.
                    break;
                }

                bool started_span = false;
                const int z_index = current.z() + OVERMAP_DEPTH;
                for( delta.x() = 0; delta.x() <= distance; delta.x()++ ) {
                    current.x() = offset.x() + delta.x() * xx_transform + delta.y() * xy_transform;
                    current.y() = offset.y() + delta.x() * yx_transform + delta.y() * yy_transform;
                    // See definition of trailing_edge_major and leading_edge_major for clarification.
                    const slope trailing_edge_minor( delta.x() * 2 - 1, delta.y() * 2 + 1 );
                    const slope leading_edge_minor( delta.x() * 2 + 1, delta.y() * 2 - 1 );

                    if( !bounds.contains( current.xy() ) ) {
                        // Current tile is out of bounds, advance to the next tile.
                        continue;
                    }
                    if( this_span->start_minor > leading_edge_minor ) {
                        // Current tile comes before the span we're considering, advance to the next tile.
                        continue;
                    }
                    if( this_span->skip_first_column && this_span->start_minor == leading_edge_minor ) {
                        // If the split is due to two transparent squares with different transparency,
                        // We want to check the blocks that are likely to cause split only in B,
                        // rather than in B & C, which can lead to performance hit.
                        continue;
                    }

                    if( this_span->end_minor < trailing_edge_minor ) {
                        // Current tile is after the span we're considering, continue to next row.
                        break;
                    }

                    T new_transparency = ( *input_arrays[z_index] )[current.x()][current.y()];

                    // If we're looking at a tile with floor or roof from the floor/roof side,
                    // that tile is actually invisible to us.
                    // TODO: Revisit this logic and differentiate between "can see bottom of tile"
                    // and "can see majority of tile".
                    bool floor_block = false;
                    if( current.z() < offset.z() ) {
                        if( ( *floor_caches[z_index + 1] )[current.x()][current.y()] ) {
                            floor_block = true;
                            new_transparency = LIGHT_TRANSPARENCY_SOLID;
                        }
                    } else if( current.z() > offset.z() ) {
                        if( ( *floor_caches[z_index] )[current.x()][current.y()] ) {
                            floor_block = true;
                            new_transparency = LIGHT_TRANSPARENCY_SOLID;
                        }
                    }

                    if( !started_block ) {
                        started_block = true;
                        current_transparency = new_transparency;
                    }

                    const int dist = rl_dist( tripoint_rel_ms::zero, delta ) + offset_distance;
                    last_intensity = calc( numerator, this_span->cumulative_value, dist );

                    if( !floor_block ) {
                        ( *output_caches[z_index] )[current.x()][current.y()] =
                            std::max( ( *output_caches[z_index] )[current.x()][current.y()], last_intensity );
                    }

                    if( !started_span ) {
                        // Need to reset minor slope, because we're starting a new line
                        new_start_minor = leading_edge_minor;
                        started_span = true;
                    }

                    if( new_transparency == current_transparency ) {
                        // All in order, no need to split the span.
                        new_start_minor = leading_edge_minor;
                        continue;
                    }

                    // Handle splitting the span into up to 4 separate spans
                    split_span<T, is_transparent, accumulate>( spans, this_span, current_transparency,
                            new_transparency, last_intensity,
                            distance, new_start_minor,
                            trailing_edge_major, leading_edge_major,
                            trailing_edge_minor, leading_edge_minor );
                }

                // If we end the row with an opaque tile, set the span to start at the next row
                // since we don't need to process the current one any more.
                if( !is_transparent( current_transparency, last_intensity ) ) {
                    this_span->start_major = leading_edge_major;
                }
            }

            if( // If we didn't scan at least 1 z-level, don't iterate further
                // Otherwise we may "phase" through tiles without checking them or waste time
                // checking spans that are out of bounds.
                !started_block ||
                // If we reach the end of the span with terrain being opaque, we don't iterate
                // further.
                // This means that any encountered transparent tiles from the current span have been
                // split off into new spans
                !is_transparent( current_transparency, last_intensity )
            ) {
                this_span = spans.erase( this_span );
            } else {
                // Cumulative average of the values encountered.
                this_span->cumulative_value = accumulate( this_span->cumulative_value,
                                              current_transparency, distance );
                ++this_span;
            }
        }
    }
}

template<int x_transform, int y_transform, int z_transform, typename T,
         T( *calc )( const T &, const T &, const int & ),
         bool( *is_transparent )( const T &, const T & ),
         T( *accumulate )( const T &, const T &, const int & )>
void cast_vertical_zlight_segment(
    const array_of_grids_of<T> &output_caches,
    const array_of_grids_of<const T> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const tripoint_bub_ms &offset, const int offset_distance,
    const T numerator )
{
    const int radius = MAX_VIEW_DISTANCE - offset_distance;

    constexpr int min_z = -OVERMAP_DEPTH;
    constexpr int max_z = OVERMAP_HEIGHT;

    slope new_start_minor( 1, 1 );

    T last_intensity( 0.0 );
    tripoint_rel_ms delta;
    tripoint_bub_ms current;

    // We start out with one span covering the entire horizontal and vertical space
    // we are interested in.  Then as changes in transparency are encountered, we truncate
    // that initial span and insert new spans before/after it in the list, removing any that
    // are no longer needed as we go.
    cata::list<span<T>> spans = { {
            slope( 0, 1 ), slope( 1, 1 ),
            slope( 0, 1 ), slope( 1, 1 ),
            T( LIGHT_TRANSPARENCY_OPEN_AIR )
        }
    };
    // At each "depth", a.k.a. distance from the origin, we iterate once over the list of spans,
    // possibly splitting them.
    for( int distance = 1; distance <= radius; distance++ ) {
        delta.z() = distance;
        T current_transparency( 0.0f );

        for( auto this_span = spans.begin(); this_span != spans.end(); ) {
            bool started_block = false;
            for( delta.y() = 0; delta.y() <= distance; delta.y()++ ) {
                // See comment above trailing_edge_major and leading_edge_major in above function.
                const slope trailing_edge_major( delta.y() * 2 - 1, delta.z() * 2 + 1 );
                const slope leading_edge_major( delta.y() * 2 + 1, delta.z() * 2 - 1 );
                current.y() = offset.y() + delta.y() * y_transform;
                if( current.y() < 0 || current.y() >= MAPSIZE_Y ) {
                    // Current tile is out of bounds, advance to the next tile.
                    continue;
                }
                if( this_span->start_major > leading_edge_major ) {
                    // Current span has a higher z-value,
                    // jump to next iteration to catch up.
                    continue;
                }
                if( this_span->skip_first_row && this_span->start_major == leading_edge_major ) {
                    // Prevents an infinite loop in some cases after splitting off the D span.
                    // We don't want to recheck the row that just caused the D span to be split off,
                    // since that can lead to an identical span being split off again, hence the
                    // infinite loop.
                    //
                    // This could also be accomplished by adding a small epsilon to the start_major
                    // of the D span but that causes artifacts.
                    continue;
                }
                if( this_span->end_major < trailing_edge_major ) {
                    // We've escaped the bounds of the current span we're considering,
                    // So continue to the next span.
                    break;
                }

                bool started_span = false;
                for( delta.x() = 0; delta.x() <= distance; delta.x()++ ) {
                    current.x() = offset.x() + delta.x() * x_transform;
                    current.z() = offset.z() + delta.z() * z_transform;
                    // See comment above trailing_edge_major and leading_edge_major in above function.
                    const slope trailing_edge_minor( delta.x() * 2 - 1, delta.z() * 2 + 1 );
                    const slope leading_edge_minor( delta.x() * 2 + 1, delta.z() * 2 - 1 );

                    if( current.x() < 0 || current.x() >= MAPSIZE_X ||
                        current.z() > max_z || current.z() < min_z ) {
                        // Current tile is out of bounds, advance to the next tile.
                        continue;
                    }
                    if( this_span->start_minor > leading_edge_minor ) {
                        // Current tile comes before the span we're considering, advance to the next tile.
                        continue;
                    }
                    if( this_span->end_minor < trailing_edge_minor ) {
                        // Current tile is after the span we're considering, continue to next row.
                        break;
                    }

                    const int z_index = current.z() + OVERMAP_DEPTH;

                    T new_transparency = ( *input_arrays[z_index] )[current.x()][current.y()];

                    // If we're looking at a tile with floor or roof from the floor/roof side,
                    // that tile is actually invisible to us.
                    bool floor_block = false;
                    if( current.z() < offset.z() ) {
                        if( ( *floor_caches[z_index + 1] )[current.x()][current.y()] ) {
                            floor_block = true;
                            new_transparency = LIGHT_TRANSPARENCY_SOLID;
                        }
                    } else if( current.z() > offset.z() ) {
                        if( ( *floor_caches[z_index] )[current.x()][current.y()] ) {
                            floor_block = true;
                            new_transparency = LIGHT_TRANSPARENCY_SOLID;
                        }
                    }

                    if( !started_block ) {
                        started_block = true;
                        current_transparency = new_transparency;
                    }

                    const int dist = rl_dist( tripoint_rel_ms::zero, delta ) + offset_distance;
                    last_intensity = calc( numerator, this_span->cumulative_value, dist );

                    if( !floor_block ) {
                        ( *output_caches[z_index] )[current.x()][current.y()] =
                            std::max( ( *output_caches[z_index] )[current.x()][current.y()], last_intensity );
                    }

                    if( !started_span ) {
                        // Need to reset minor slope, because we're starting a new line
                        new_start_minor = leading_edge_minor;
                        started_span = true;
                    }

                    if( new_transparency == current_transparency ) {
                        // All in order, no need to split the span.
                        new_start_minor = leading_edge_minor;
                        continue;
                    }

                    // Handle splitting the span into up to 4 separate spans
                    split_span<T, is_transparent, accumulate>( spans, this_span, current_transparency,
                            new_transparency, last_intensity,
                            distance, new_start_minor,
                            trailing_edge_major, leading_edge_major,
                            trailing_edge_minor, leading_edge_minor );
                }

                // If we end the row with an opaque tile, set the span to start at the next row
                // since we don't need to process the current one any more.
                if( !is_transparent( current_transparency, last_intensity ) ) {
                    this_span->start_major = leading_edge_major;
                }
            }

            if( // If we didn't scan at least 1 z-level, don't iterate further
                // Otherwise we may "phase" through tiles without checking them or waste time
                // checking spans that are out of bounds.
                !started_block ||
                // If we reach the end of the span with terrain being opaque, we don't iterate
                // further.
                // This means that any encountered transparent tiles from the current span have been
                // split off into new spans
                !is_transparent( current_transparency, last_intensity )
            ) {
                this_span = spans.erase( this_span );
            } else {
                // Cumulative average of the values encountered.
                this_span->cumulative_value = accumulate( this_span->cumulative_value,
                                              current_transparency, distance );
                ++this_span;
            }
        }
    }
}

template<typename T, T( *calc )( const T &, const T &, const int & ),
         bool( *is_transparent )( const T &, const T & ),
         T( *accumulate )( const T &, const T &, const int & )>
void cast_zlight(
    const array_of_grids_of<T> &output_caches,
    const array_of_grids_of<const T> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const tripoint_bub_ms &origin, const int offset_distance, const T numerator,
    vertical_direction dir )
{
    if( dir == vertical_direction::DOWN || dir == vertical_direction::BOTH ) {
        // Down lateral
        // @..
        //  ..
        //   .
        cast_horizontal_zlight_segment < 0, 1, 1, 0, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // @
        // ..
        // ...
        cast_horizontal_zlight_segment < 1, 0, 0, 1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        //   .
        //  ..
        // @..
        cast_horizontal_zlight_segment < 0, -1, 1, 0, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // ...
        // ..
        // @
        cast_horizontal_zlight_segment < -1, 0, 0, 1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // ..@
        // ..
        // .
        cast_horizontal_zlight_segment < 0, 1, -1, 0, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        //   @
        //  ..
        // ...
        cast_horizontal_zlight_segment < 1, 0, 0, -1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // .
        // ..
        // ..@
        cast_horizontal_zlight_segment < 0, -1, -1, 0, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // ...
        //  ..
        //   @
        cast_horizontal_zlight_segment < -1, 0, 0, -1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

        // Straight down
        // @.
        // ..
        cast_vertical_zlight_segment < 1, 1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // ..
        // @.
        cast_vertical_zlight_segment < 1, -1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // .@
        // ..
        cast_vertical_zlight_segment < -1, 1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // ..
        // .@
        cast_vertical_zlight_segment < -1, -1, -1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    }

    if( dir == vertical_direction::UP || dir == vertical_direction::BOTH ) {
        // Up lateral
        // @..
        //  ..
        //   .
        cast_horizontal_zlight_segment < 0, 1, 1, 0, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // @
        // ..
        // ...
        cast_horizontal_zlight_segment < 1, 0, 0, 1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // ..@
        // ..
        // .
        cast_horizontal_zlight_segment < 0, -1, 1, 0, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        //   @
        //  ..
        // ...
        cast_horizontal_zlight_segment < -1, 0, 0, 1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        //   .
        //  ..
        // @..
        cast_horizontal_zlight_segment < 0, 1, -1, 0, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // ...
        // ..
        // @
        cast_horizontal_zlight_segment < 1, 0, 0, -1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // .
        // ..
        // ..@
        cast_horizontal_zlight_segment < 0, -1, -1, 0, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // ...
        //  ..
        //   @
        cast_horizontal_zlight_segment < -1, 0, 0, -1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

        // Straight up
        // @.
        // ..
        cast_vertical_zlight_segment < 1, 1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // ..
        // @.
        cast_vertical_zlight_segment < 1, -1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // .@
        // ..
        cast_vertical_zlight_segment < -1, 1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
        // ..
        // .@
        cast_vertical_zlight_segment < -1, -1, 1, T, calc, is_transparent, accumulate > (
            output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    }
}

// I can't figure out how to make implicit instantiation work when the parameters of
// the template-supplied function pointers are involved, so I'm explicitly instantiating instead.
template void cast_zlight<float, sight_calc, sight_check, accumulate_transparency>(
    const array_of_grids_of<float> &output_caches,
    const array_of_grids_of<const float> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const tripoint_bub_ms &origin, int offset_distance, float numerator,
    vertical_direction dir );

template void cast_zlight<fragment_cloud, shrapnel_calc, shrapnel_check, accumulate_fragment_cloud>(
    const array_of_grids_of<fragment_cloud> &output_caches,
    const array_of_grids_of<const fragment_cloud> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const tripoint_bub_ms &origin, int offset_distance, fragment_cloud numerator,
    vertical_direction dir );
