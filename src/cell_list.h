#pragma once
#ifndef CELL_LIST_H
#define CELL_LIST_H

#include <algorithm>
#include <list>
#include <vector>

#include "cata_utility.h"
#include "line.h"
#include "point.h"

template< typename T, int CELL_SIZE, int MAXX, int MAXY >
class cell_list_iterator;
template< typename T, int CELL_SIZE, int MAXX, int MAXY >
class cell_list_range;

/*
 * A Cell List operates by subdividing an area into a regular grid and populating each cell
 * with a list of the entities in that area.
 * It is used to optimize lookup of nearby entities.
 */
template< typename T, int CELL_SIZE, int MAXX, int MAXY >
class cell_list
{
    public:
        cell_list() = default;
        void add( const point &p, T value ) {
            cells[p.x / CELL_SIZE][p.y / CELL_SIZE].emplace_back( p, value );
        }
        cell_list_range< T, CELL_SIZE, MAXX, MAXY > get( const point &p, int distance ) const {
            return cell_list_range<T, CELL_SIZE, MAXX, MAXY>( *this, p, distance );
        }
        static constexpr int num_cells_x() {
            return divide_round_up( MAXX, CELL_SIZE );
        }
        static constexpr int num_cells_y() {
            return divide_round_up( MAXY, CELL_SIZE );
        }
        friend class cell_list_range< T, CELL_SIZE, MAXX, MAXY >;
    private:
        std::vector<std::pair<point, T>> cells[num_cells_x()][num_cells_y()];
};
/*
 * A cell list range handles iteration over elements in the Cell List near some origin point in
 * nearest-first order and terminates at some specified threshold.
 * It does this by visiting one "rank" of cells at a time.
 * First it visits the cell the origin is in, determining the distance to the nearest cell wall,
 * and iterating over every entity closer than that distance. This guarantees that there is no
 * closer entity than one found via this process. Once that list is exhausted, it loads one row of
 * cells around the origin cell and repeating the above process.
 */
template< typename T, int CELL_SIZE, int MAXX, int MAXY >
class cell_list_range
{
    public:
        cell_list_range( const cell_list< T, CELL_SIZE, MAXX, MAXY > &list, const point &p,
                         int distance ) : list( &list ), origin( p ), origin_cell( p / CELL_SIZE ),
            max_distance( distance ), sorted_elements( distance + 1,
                    std::vector<const std::pair<point, T >*>() ) {
            min_bounds = origin_cell * CELL_SIZE;
            max_bounds = min_bounds + point( CELL_SIZE - 1, CELL_SIZE - 1 );
            distance_to_nearest_cell_wall = calc_distance();
            // Special case for first cell.
            add( list.cells[origin_cell.x][origin_cell.y] );
        }
        using iterator = cell_list_iterator<T, CELL_SIZE, MAXX, MAXY>;
        iterator begin() {
            return iterator( this );
        }
        iterator end() {
            return iterator();
        }
        point get_origin() const {
            return origin;
        }
        int get_distance() const {
            return max_distance;
        }
        friend class cell_list_iterator< T, CELL_SIZE, MAXX, MAXY >;
    private:
        int calc_distance() const {
            int distance = max_distance;
            if( min_bounds.x >= 0 ) {
                distance = std::min( distance, origin.x - min_bounds.x );
            }
            if( min_bounds.y >= 0 ) {
                distance = std::min( distance, origin.y - min_bounds.y );
            }
            if( max_bounds.x < MAXX ) {
                distance = std::min( distance, max_bounds.x - origin.x );
            }
            if( max_bounds.y < MAXY ) {
                distance = std::min( distance, max_bounds.y - origin.y );
            }
            return distance;
        }
        void add( const std::vector<std::pair<point, T>> &cells ) {
            for( const std::pair<point, T> &candidate : cells ) {
                add( candidate );
            }
        }
        void add( const std::pair<point, T> &candidate ) {
            const int distance = rl_dist( candidate.first, origin );
            if( distance > max_distance ) {
                return;
            }
            sorted_elements[distance].push_back( &candidate );
        }
        void load() {
            constexpr point rank_offset{ CELL_SIZE, CELL_SIZE };
            min_bounds -= rank_offset;
            max_bounds += rank_offset;
            distance_to_nearest_cell_wall = calc_distance();
            cell_ranks_loaded++;
            const point load_offset( cell_ranks_loaded, cell_ranks_loaded );
            const point min_cell = origin_cell - load_offset;
            const point max_cell = origin_cell + load_offset;
            const bool min_x_inbounds = min_cell.x >= 0;
            const bool min_y_inbounds = min_cell.y >= 0;
            const bool max_x_inbounds = max_cell.x < list->num_cells_x();
            const bool max_y_inbounds = max_cell.y < list->num_cells_x();
            // Draw a box...
            for( int x = min_cell.x; x <= max_cell.x; ++x ) {
                if( x < 0 || x >= list->num_cells_x() ) {
                    continue;
                }
                if( min_y_inbounds ) {
                    add( list->cells[x][min_cell.y] );
                }
                if( max_y_inbounds ) {
                    add( list->cells[x][max_cell.y] );
                }
            }
            for( int y = min_cell.y + 1; y <= max_cell.y - 1; ++y ) {
                if( y < 0 || y >= list->num_cells_y() ) {
                    continue;
                }
                if( min_x_inbounds ) {
                    add( list->cells[min_cell.x][y] );
                }
                if( max_x_inbounds ) {
                    add( list->cells[max_cell.x][y] );
                }
            }
        }
        const cell_list<T, CELL_SIZE, MAXX, MAXY> *list;
        const point origin;
        const point origin_cell; // = origin / CELL_SIZE
        const int max_distance;
        std::vector<std::vector<const std::pair<point, T>*>> sorted_elements;
        // Upper left and lower right corners of area, used to determine loaded distance to scan.
        point min_bounds;
        point max_bounds;
        int distance_to_nearest_cell_wall = 0;
        int cell_ranks_loaded = 0;
};

/*
 * cell_list_iterator is a forward iterator over the underlying cell_list_range adaptor.
 * It iterates from nearest entries to most distant entries, and when it reaches
 * the specified range limit it halts.
 * The intended use case is to be invoked in a iterator-based for or while loop with
 * early termination when a valid match is found.
 */
template< typename T, int CELL_SIZE, int MAXX, int MAXY>
class cell_list_iterator
{
    public:
        // The default constructor creates an end iterator.
        cell_list_iterator() : range( nullptr ) {};
        cell_list_iterator( cell_list_range< T, CELL_SIZE, MAXX, MAXY > *range ) : range( range ) {
            current = range->sorted_elements[current_distance].begin();
            if( range->sorted_elements[current_distance].empty() ) {
                ++( *this );
            }
        }
        const std::pair<point, T> &operator*() const {
            assert( range );
            return **current;
        }
        const std::pair<point, T> *operator->() const {
            assert( range );
            return & **current;
        }

        cell_list_iterator &operator++() {
            assert( range );
            if( current != range->sorted_elements[current_distance].end() ) {
                ++current;
            }
            // Effectively, "while current does not point to a valid entry".
            while( current == range->sorted_elements[current_distance].end() ) {
                ++current_distance;
                if( current_distance > range->max_distance ) {
                    range = nullptr;
                    break;
                }
                if( current_distance > range->distance_to_nearest_cell_wall ) {
                    range->load();
                }
                current = range->sorted_elements[current_distance].begin();
            }
            return *this;
        }

        bool operator==( const cell_list_iterator &rhs ) const {
            if( range == nullptr && rhs.range == nullptr ) {
                return true;
            }
            if( range != rhs.range ) {
                return false;
            }
            return current == rhs.current;
        }
        bool operator!=( const cell_list_iterator &rhs ) const {
            return !( *this == rhs );
        }

    private:
        cell_list_range<T, CELL_SIZE, MAXX, MAXY> *range;
        int current_distance = 1;
        typename std::vector<const std::pair<point, T>*>::iterator current;
};

#endif
