#pragma once
#ifndef CATA_SRC_MAP_SELECTOR_H
#define CATA_SRC_MAP_SELECTOR_H

#include <climits>
#include <vector>

#include "coordinates.h"
#include "point.h"
#include "visitable.h"

class map;

class map_cursor : public visitable
{
    private:
        tripoint_abs_ms pos_abs_;
        tripoint_bub_ms pos_bub_;

    public:
        explicit map_cursor( const tripoint_bub_ms &pos );
        explicit map_cursor( map *here, const tripoint_bub_ms &pos );
        // Marginally faster than the previous operation if you have the absolute coordinates at hand.
        explicit map_cursor( const tripoint_abs_ms &pos );
        tripoint_bub_ms pos_bub() const;
        tripoint_bub_ms pos_bub( const map &here ) const;
        // Will return tripoint_abs_ms::invalid if g hasn't been defined.
        tripoint_abs_ms pos_abs() const;

        // inherited from visitable
        VisitResponse visit_items( const std::function<VisitResponse( item *, item * )> &func ) const
        override;
        std::list<item> remove_items_with( const std::function<bool( const item & )> &filter,
                                           int count = INT_MAX ) override;
};

class map_selector : public visitable
{
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
        explicit map_selector( const tripoint_bub_ms &pos, int radius = 0, bool accessible = true );

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

        // inherited from visitable
        VisitResponse visit_items( const std::function<VisitResponse( item *, item * )> &func ) const
        override;
        std::list<item> remove_items_with( const std::function<bool( const item & )> &filter,
                                           int count = INT_MAX ) override;

    private:
        std::vector<value_type> data;
};

#endif // CATA_SRC_MAP_SELECTOR_H
