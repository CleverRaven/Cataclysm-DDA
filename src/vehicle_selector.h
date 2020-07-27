#pragma once
#ifndef CATA_SRC_VEHICLE_SELECTOR_H
#define CATA_SRC_VEHICLE_SELECTOR_H

#include <vector>
#include <iosfwd>

#include "visitable.h"

class vehicle;
struct tripoint;

class vehicle_cursor : public visitable<vehicle_cursor>
{
    public:
        vehicle_cursor( vehicle &veh, std::ptrdiff_t part ) : veh( veh ), part( part ) {}
        vehicle &veh;
        std::ptrdiff_t part;
};

class vehicle_selector : public visitable<vehicle_selector>
{
        friend visitable<vehicle_selector>;

    public:
        using value_type = vehicle_cursor;
        using size_type = std::vector<value_type>::size_type;
        using iterator = std::vector<value_type>::iterator;
        using const_iterator = std::vector<value_type>::const_iterator;
        using reference = std::vector<value_type>::reference;
        using const_reference = std::vector<value_type>::const_reference;

        /**
         *  Constructs vehicle_selector used for querying items located on vehicle tiles
         *  @param pos map position at which to start each query which may or may not contain vehicle
         *  @param radius number of adjacent tiles to include (searching from pos outwards)
         *  @param accessible whether found items must be accessible from pos to be considered
         *  @param visibility_only accessibility based on line of sight, not walkability
         */
        vehicle_selector( const tripoint &pos, int radius = 0, bool accessible = true,
                          bool visibility_only = false );

        /**
         *  Constructs vehicle_selector used for querying items located on vehicle tiles
         *  @param pos map position at which to start each query which may or may not contain vehicle
         *  @param radius number of adjacent tiles to include (searching from pos outwards)
         *  @param accessible whether found items must be accessible from pos to be considered
         *  @param ignore don't include this vehicle as part of the selection
         */
        vehicle_selector( const tripoint &pos, int radius, bool accessible, const vehicle &ignore );

        // similar to item_location you are not supposed to store this class between turns
        vehicle_selector( const vehicle_selector &that ) = delete;
        vehicle_selector &operator=( const vehicle_selector & ) = delete;
        vehicle_selector( vehicle_selector && ) = default;

        size_type size() const {
            return data.size();
        }
        iterator begin() {
            return data.begin();
        }
        iterator end() {
            return data.end();
        }
        const_iterator begin() const {
            return data.cbegin();
        }
        const_iterator end() const {
            return data.cend();
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

#endif // CATA_SRC_VEHICLE_SELECTOR_H
