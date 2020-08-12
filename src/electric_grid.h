#pragma once
#ifndef CATA_SRC_ELECTRIC_GRID_H
#define CATA_SRC_ELECTRIC_GRID_H

#include <set>
#include <vector>
#include "memory_fast.h"
#include "point.h"

class battery_tile;
class map;
class submap;

class electric_grid
{
    private:
        friend class map;
        std::set<submap *> submaps;
        std::vector<battery_tile *> batteries;

    public:
        // TODO: Probably shouldn't use submap directly
        electric_grid( std::set<submap *> sms );
        /**
         * Stores or drains energy from grid batteries and similar.
         * @param amt Joules of energy to add to grid
         * @return Excess Joules that couldn't be added or removed
         */
        int mod_energy( int amt );

};

#endif // CATA_SRC_ELECTRIC_GRID_H
