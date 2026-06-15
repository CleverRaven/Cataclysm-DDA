#pragma once
#ifndef CATA_SRC_POWER_NETWORK_H
#define CATA_SRC_POWER_NETWORK_H

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "calendar.h"
#include "coordinates.h"
#include "point.h"
#include "units.h"

class JsonObject;
class JsonOut;
class vehicle;

// Snapshot of a single vehicle's rated renewable output and battery capacity
// within a power network. Write-only infrastructure for phase 3 --
// not used for runtime generation decisions yet.
struct power_network_node {
    tripoint_abs_ms position;
    units::power rated_solar = 0_W;
    units::power rated_wind = 0_W;
    units::power rated_water = 0_W;
    units::power rated_consumption = 0_W;
    int battery_capacity_kJ = 0;

    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &data );
};

// A group of cable-connected vehicles forming one power grid.
// Rebuilt from live BFS every turn; persisted for phase 3 use.
struct power_network {
    int id = 0;
    time_point last_resolved = calendar::before_time_starts;
    tripoint_abs_ms root_position;
    std::vector<power_network_node> nodes;

    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &data );
};

// Owns all power networks. Rebuilt every turn from search_connected_vehicles(),
// carrying forward id from predecessor networks matched by position overlap.
// last_resolved is set to calendar::turn on each rebuild.
class power_network_manager
{
    public:
        // Call begin_rebuild() before adding grids each turn.
        // Call add_grid() for each BFS-discovered grid.
        // Call finish_rebuild() after all grids have been added.
        void begin_rebuild();
        void add_grid( vehicle *root, const std::map<vehicle *, float> &grid );
        void finish_rebuild();

        const power_network *find_network_at( const tripoint_abs_ms &pos ) const;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );

        void clear();

        const std::map<int, power_network> &all_networks() const {
            return networks_;
        }

    private:
        std::map<int, power_network> networks_;
        int next_id_ = 1;

        // Temporary state for the begin/add/finish rebuild cycle.
        std::unordered_map<tripoint_abs_ms, int> old_pos_to_id_; // NOLINT(cata-serialize)
        std::unordered_set<int> claimed_ids_; // NOLINT(cata-serialize)
        std::map<int, power_network> pending_networks_; // NOLINT(cata-serialize)
        int rebuild_next_id_ = 1; // NOLINT(cata-serialize)
};

#endif // CATA_SRC_POWER_NETWORK_H
