#ifndef MAP_SELECTOR_H
#define MAP_SELECTOR_H

#include <vector>

#include "visitable.h"

class map;

class map_cursor : public tripoint, public visitable<map_cursor> {
    public:
        map_cursor( const tripoint &pos ) : tripoint( pos ) {};
};

class map_selector : public visitable<map_selector> {
    friend visitable<map_selector>;

    public:
        typedef map_cursor value_type;
        typedef std::vector<tripoint>::size_type size_type;
        typedef value_type* iterator;
        typedef const value_type* const_iterator;
        typedef value_type& reference;
        typedef const value_type& const_reference;

        /**
         *  Constructs map_selector used for querying items located on map tiles
         *  @param pos position on map at which to start each query
         *  @param radius number of adjacent tiles to include (searching from pos outwards)
         *  @param accessible whether found items must be accesible from pos to be considered
         */
        map_selector( map& m, const tripoint& pos, int radius = 0, bool accessible = true );

        // similar to item_location you are not supposed to store this class between turns
        map_selector( const map_selector& that ) = delete;
        map_selector& operator=( const map_selector& ) = delete;
        map_selector( map_selector && ) = default;

        size_type size() const { return data.size(); }
        iterator begin() { return &data[ 0 ]; }
        iterator end() { return &data[ data.size() ]; }
        const_iterator cbegin() { return &data[ 0 ]; }
        const_iterator cend() { return &data[ size() ]; }
        reference front() { return data[ 0 ]; }
        const_reference front() const { return data[ 0 ]; }
        reference back() { return data[ size() - 1 ]; }
        const_reference back() const { return data[ size() - 1 ]; }

    private:
        std::vector<value_type> data;
};

#endif
