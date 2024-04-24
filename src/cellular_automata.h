#pragma once
#ifndef CATA_SRC_CELLULAR_AUTOMATA_H
#define CATA_SRC_CELLULAR_AUTOMATA_H

#include <vector>

struct point;

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
int neighbor_count( const std::vector<std::vector<int>> &cells, int width,
                    int height, const point &p );

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
std::vector<std::vector<int>> generate_cellular_automaton(
                               int width, int height, int alive, int iterations,
                               int birth_limit, int stasis_limit );

} // namespace CellularAutomata

#endif // CATA_SRC_CELLULAR_AUTOMATA_H
