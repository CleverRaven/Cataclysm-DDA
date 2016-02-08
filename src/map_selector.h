#ifndef MAP_SELECTOR_H
#define MAP_SELECTOR_H

#include "visitable.h"
#include "enums.h"

class map;

class map_selector : public visitable<map_selector> {
    friend visitable<map_selector>;

    public:
        /**
         *  Constructs map_selector used for querying items located on map tiles
         *  @param pos position on map at which to start each query
         *  @param radius number of adjacent tiles to include (searching from pos outwards)
         *  @param accessible whether found items must be accesible from pos to be considered
         */
        map_selector( map& m, const tripoint& pos, int radius = 0, bool accessible = true )
            : m( m ), pos( pos ), radius( radius ), accessible( accessible ) {}

        // similar to item_location you are not supposed to store this class between turns
        map_selector( const map_selector& that ) = delete;
        map_selector& operator=( const map_selector& ) = delete;
        map_selector( map_selector && ) = default;

    private:
        map& m;
        const tripoint& pos;
        int radius;
        bool accessible;
};

#endif
