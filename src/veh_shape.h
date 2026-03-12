#pragma once
#ifndef CATA_SRC_VEH_SHAPE_H
#define CATA_SRC_VEH_SHAPE_H

#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "coordinates.h"
#include "input_context.h"
#include "vpart_position.h"

class map;
class player_activity;
class vehicle;
template <typename T> class ret_val;

class veh_shape
{
    public:
        explicit veh_shape( map &here, vehicle &vehicle );

        /// Starts vehicle ui loop, runs until canceled or activity is selected and returned
        /// @param vehicle Vehicle to handle
        /// @return Selected activity or player_activity( activity_id::NULL_ID() ) if cancelled
        player_activity start( const tripoint_bub_ms &pos );

    private:
        input_context ctxt;

        // vehicle being worked on
        vehicle &veh;

        // cursor handling
        map &here;
        std::set<tripoint_bub_ms> cursor_allowed; // all points the cursor is allowed to be on
        tripoint_bub_ms cursor_pos;
        tripoint_bub_ms get_cursor_pos() const;
        bool set_cursor_pos( const tripoint_bub_ms &new_pos );
        bool handle_cursor_movement( const std::string &action );

        /// Returns all parts under cursor (no filtering)
        std::vector<vpart_reference> parts_under_cursor() const;

        // break glass^W^W delete this in case multi-level vehicles are a thing
        const bool allow_zlevel_shift = false;

        void init_input();

        // doing actual "maintenance"
        std::optional<vpart_reference> select_part_at_cursor(
            const std::string &title, const std::string &extra_description,
            const std::function<ret_val<void>( const vpart_reference & )> &predicate,
            const std::optional<vpart_reference> &preselect ) const;

        void change_part_shape( vpart_reference vpr ) const;
};

#endif // CATA_SRC_VEH_SHAPE_H
