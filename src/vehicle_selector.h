#ifndef VEHICLE_SELECTOR_H
#define VEHICLE_SELECTOR_H

#include <vector>

#include "visitable.h"

class vehicle;

class vehicle_cursor : public visitable<vehicle_cursor> {
    public:
        vehicle_cursor( vehicle& veh, int part ) : veh( veh ), part( part ) {};
        vehicle& veh;
        int part;
};

class vehicle_selector : public visitable<vehicle_selector> {
    friend visitable<vehicle_selector>;

    public:
        typedef vehicle_cursor value_type;
        typedef std::vector<tripoint>::size_type size_type;
        typedef value_type* iterator;
        typedef const value_type* const_iterator;
        typedef value_type& reference;
        typedef const value_type& const_reference;

        /**
         *  Constructs vehicle_selector used for querying items located on vehicle tiles
         *  @param pos map position at which to start each query which may or may not contain vehicle
         *  @param radius number of adjacent tiles to include (searching from pos outwards)
         */
        vehicle_selector( const tripoint& pos, int radius = 0 );

        // similar to item_location you are not supposed to store this class between turns
        vehicle_selector( const vehicle_selector& that ) = delete;
        vehicle_selector& operator=( const vehicle_selector& ) = delete;
        vehicle_selector( vehicle_selector && ) = default;

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
