#pragma once
#ifndef CATA_SRC_CELLULAR_AUTOMATA_H
#define CATA_SRC_CELLULAR_AUTOMATA_H

#include <vector>

#include "point.h"
#include "rng.h"

namespace CellularAutomata
{
/**
* Calculates the number of alive neighbors by looking at the Moore neighborhood (3x3 grid of cells).
* @param cells The cells to look at. Assumed to be of a consistent width/height equal to the specified
* values.
* @param width The width of the cells. Specified up front to avoid checked it each time.
* @param width The height of the cells. Specified up front to avoid checked it each time.
* @param p
* @returns The number of neighbors that are alive, a value between 0 and 8.
*/
inline int neighbor_count( const std::vector<std::vector<int>> &cells, const int width,
                           const int height, const point &p )
{
    int neighbors = 0;
    for( int ni = -1; ni <= 1; ni++ ) {
        for( int nj = -1; nj <= 1; nj++ ) {
            const int nx = p.x + ni;
            const int ny = p.y + nj;

            // These neighbors are outside the bounds, so they can't contribute.
            if( nx < 0 || nx >= width || ny < 0 || ny >= height ) {
                continue;
            }

            neighbors += cells[nx][ny];
        }
    }
    // Because we included ourself in the loop above, subtract ourselves back out.
    neighbors -= cells[p.x][p.y];

    return neighbors;
}

/**
* Generate a cellular automaton using the provided parameters.
* Basic rules are as follows:
* - alive% of cells start alive
* - Run for specified number of iterations
* - Dead cells with > birth_limit neighbors become alive
* - Alive cells with > statis_limit neighbors stay alive
* - The rest die
* @param width Dimensions of the grid.
* @param height Dimensions of the grid.
* @param alive Number between 0 and 100, used to roll an x_in_y check for the cell to start alive.
* @param iterations Number of iterations to run the cellular automaton.
* @param birth_limit Dead cells with > birth_limit neighbors become alive.
* @param stasis_limit Alive cells with > statis_limit neighbors stay alive
* @returns The width x height grid of cells. Each cell is a 0 if dead or a 1 if alive.
*/
inline std::vector<std::vector<int>> generate_cellular_automaton( const int width, const int height,
                                  const int alive, const int iterations, const int birth_limit, const int stasis_limit )
{
    std::vector<std::vector<int>> current( width, std::vector<int>( height, 0 ) );
    std::vector<std::vector<int>> next( width, std::vector<int>( height, 0 ) );

    // Initialize our initial set of cells.
    for( int i = 0; i < width; i++ ) {
        for( int j = 0; j < height; j++ ) {
            current[i][j] = x_in_y( alive, 100 );
        }
    }

    for( int iteration = 0; iteration < iterations; iteration++ ) {
        for( int i = 0; i < width; i++ ) {
            for( int j = 0; j < height; j++ ) {
                // Skip the edges--no need to complicate this with more complex neighbor
                // calculations, just keep them constant.
                if( i == 0 || i == width - 1 || j == 0 || j == height - 1 ) {
                    next[i][j] = 0;
                    continue;
                }

                // Count our neighors.
                const int neighbors = neighbor_count( current, width, height, point( i, j ) );

                // Dead and > birth_limit neighbors, so become alive.
                if( ( current[i][j] == 0 ) && ( neighbors > birth_limit ) ) {
                    next[i][j] = 1;
                }
                // Alive and > statis_limit neighbors, so stay alive.
                else if( ( current[i][j] == 1 ) && ( neighbors > stasis_limit ) ) {
                    next[i][j] = 1;
                }
                // Else, die.
                else {
                    next[i][j] = 0;
                }
            }
        }

        // Swap our current and next vectors and repeat.
        std::swap( current, next );
    }

    return current;
}
} // namespace CellularAutomata

#endif // CATA_SRC_CELLULAR_AUTOMATA_H
