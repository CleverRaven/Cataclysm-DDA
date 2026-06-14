#include "cellular_automata.h"

#include <algorithm>

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
int neighbor_count( const std::vector<std::vector<int>> &cells, const int width,
                    const int height, const point &p )
{
    int neighbors = 0;
    for( int ni = -1; ni <= 1; ni++ ) {
        for( int nj = -1; nj <= 1; nj++ ) {
            const point n( p + point( ni, nj ) );

            // These neighbors are outside the bounds, so they can't contribute.
            if( n.x < 0 || n.x >= width || n.y < 0 || n.y >= height ) {
                continue;
            }

            neighbors += cells[n.x][n.y];
        }
    }
    // Because we included ourself in the loop above, subtract ourselves back out.
    neighbors -= cells[p.x][p.y];

    return neighbors;
}

std::vector<std::vector<int>> generate_cellular_automaton(
                               const int width, const int height, const int alive, const int iterations,
                               const int birth_limit, const int stasis_limit )
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

                // Count our neighbors.
                const int neighbors = neighbor_count( current, width, height, point( i, j ) );

                // Dead and > birth_limit neighbors, become alive.
                // Alive and > stasis_limit neighbors, stay alive.
                const int to_live = current[i][j] ? stasis_limit : birth_limit;
                if( neighbors > to_live ) {
                    next[i][j] = 1;
                    // Else, die.
                } else {
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
