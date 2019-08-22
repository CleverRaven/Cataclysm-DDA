#pragma once
#ifndef MAP_SELECTOR_H
#define MAP_SELECTOR_H

#include <vector>

#include "point.h"
#include "visitable.h"

class map_cursor : public tripoint, public visitable<map_cursor>
{
    public:
        map_cursor( const tripoint &pos ) : tripoint( pos ) {}
};

class map_selector : public visitable<map_selector>
{
        friend visitable<map_selector>;

    public:
        using value_type = map_cursor;
        using size_type = std::vector<value_type>::size_type;
        using iterator = std::vector<value_type>::iterator;
        using const_iterator = std::vector<value_type>::const_iterator;
        using reference = std::vector<value_type>::reference;
        using const_reference = std::vector<value_type>::const_reference;

        /**
         *  Constructs map_selector used for querying items located on map tiles
         *  @param pos position on map at which to start each query
         *  @param radius number of adjacent tiles to include (searching from pos outwards)
         *  @param accessible whether found items must be accessible from pos to be considered
         */
        map_selector( const tripoint &pos, int radius = 0, bool accessible = true );

        // similar to item_location you are not supposed to store this class between turns
        map_selector( const map_selector &that ) = delete;
        map_selector &operator=( const map_selector & ) = delete;
        map_selector( map_selector && ) = default;

        size_type size() const {
            return data.size();
        }
        iterator begin() {
            return data.begin();
        }
        iterator end() {
            return data.end();
        }
        const_iterator cbegin() const {
            return data.cbegin();
        }
        const_iterator cend() const {
            return data.cend();
        }
        reference front() {
            return data.front();
        }
        const_reference front() const {
            return data.front();
        }
        reference back() {
            return data.back();
        }
        const_reference back() const {
            return data.back();
        }

    private:
        std::vector<value_type> data;
};

#endif
