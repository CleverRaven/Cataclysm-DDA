#pragma once
#ifndef CATA_SRC_PLAYER_ACTIVITY_H
#define CATA_SRC_PLAYER_ACTIVITY_H

#include <climits>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "activity_actor.h"
#include "calendar.h"
#include "clone_ptr.h"
#include "compatibility.h"
#include "coordinates.h"
#include "enums.h"
#include "item_location.h"
#include "memory_fast.h"
#include "point.h"
#include "type_id.h"

class Character;
class JsonObject;
class JsonOut;
class avatar;
class monster;
class translation;

class player_activity
{
    private:
        activity_id type;
        cata::clone_ptr<activity_actor> actor;

        std::set<distraction_type> ignored_distractions; // NOLINT(cata-serialize)

        bool ignoreQuery = false; // NOLINT(cata-serialize)

    public:
        /** Total number of moves required to complete the activity */
        int moves_total = 0;
        /** The number of moves remaining in this activity before it is complete. */
        int moves_left = calendar::INDEFINITELY_LONG;
        /** Controls whether this activity can be cancelled at all */
        bool interruptable = true;
        /** Controls whether this activity can be cancelled with 'pause' action */
        bool interruptable_with_kb = true;

        // The members in the following block are deprecated, prefer creating a new
        // activity_actor.
        int index = 0;
        /**
         *   An activity specific value.
         *   DO NOT USE FOR ITEM INDEX
        */
        int position = 0;
        std::string name;
        std::vector<item_location> targets;
        std::vector<int> values;
        std::vector<std::string> str_values;
        std::vector<tripoint_abs_ms> coords;
        std::unordered_set<tripoint_abs_ms> coord_set;
        std::vector<weak_ptr_fast<monster>> monsters;
        static constexpr const tripoint_abs_ms &invalid_place = tripoint_abs_ms::invalid;
        tripoint_abs_ms placement;
        // ACT_START_ENGINES needs a relative position because the engine might
        // be in a moving vehicle at the time.
        tripoint_rel_ms relative_placement;

        bool no_drink_nearby_for_auto_consume = false; // NOLINT(cata-serialize)
        bool no_food_nearby_for_auto_consume = false; // NOLINT(cata-serialize)
        /** If true, the activity will be auto-resumed next time the player attempts
         *  an identical activity. This value is set dynamically.
         */
        bool auto_resume = false;
        /** Flag that will suppress the relatively expensive fire refueling search process.
         *  Initially assume there is a fire unless the activity proves not to have one.
         */
        bool have_fire = true; // NOLINT(cata-serialize)

        player_activity();
        // This constructor does not work with activities using the new activity_actor system
        // TODO: delete this constructor once migration to the activity_actor system is complete
        explicit player_activity( activity_id, int turns = 0, int Index = -1, int pos = INT_MIN,
                                  const std::string &name_in = "" );
        /**
         * Create a new activity with the given actor
         */
        explicit player_activity( const activity_actor &actor );

        player_activity( player_activity && ) noexcept = default;
        player_activity( const player_activity & ) = default;
        player_activity &operator=( player_activity && ) noexcept( list_is_noexcept ) = default;
        player_activity &operator=( const player_activity & ) = default;

        explicit operator bool() const {
            return !type.is_null();
        }
        bool is_null() const {
            return type.is_null();
        }
        bool is_multi_type() const;
        /** This replaces the former usage `act.type = ACT_NULL` */
        void set_to_null();

        // This makes player_activity's activity type inherit activity_actor's activity type,
        // in order to synchronize both, due to possible variability of actor's activity type
        // allowed via override of activity_actor::get_type()
        void synchronize_type_with_actor();

        const activity_id &id() const {
            return type;
        }
        bool rooted() const;

        // Question to ask when the activity is to be stopped,
        // e.g. "Stop doing something?", already translated.
        std::string get_stop_phrase() const;

        const translation &get_verb() const;

        int get_value( size_t index, int def = 0 ) const;
        std::string get_str_value( size_t index, std::string_view def = {} ) const;

        /**
         * Helper that returns an activity specific progress message.
         */
        std::optional<std::string> get_progress_message( const avatar &u ) const;

        /**
         * If this returns true, the action can be continued without
         * starting from scratch again (see player::backlog). This is only
         * possible if the player start the very same activity (with the same
         * parameters) again.
         */
        bool can_resume() const;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );
        /** Convert from the old enumeration to the new string_id */
        void deserialize_legacy_type( int legacy_type, activity_id &dest );

        /**
         * Perform necessary initialization to start or resume the activity. Must be
         * called whenever a Character starts a new activity.
         * When resuming an activity, do not call activity_actor::start
         */
        void start_or_resume( Character &who, bool resuming );

        /**
         * Performs the activity for a single turn. If the activity is complete
         * at the end of the turn, do_turn also executes whatever actions, if
         * any, are needed to conclude the activity.
         */
        void do_turn( Character &you );

        /**
         * Performs activity-specific cleanup when Character::cancel_activity() is called
         */
        void canceled( Character &who );

        /**
         * Returns true if activities are similar enough that this activity
         * can be resumed instead of starting the other activity.
         */
        bool can_resume_with( const player_activity &other, const Character &who ) const;

        bool is_interruptible() const;
        bool is_interruptible_with_kb() const;
        bool is_distraction_ignored( distraction_type ) const;
        void ignore_distraction( distraction_type );
        void allow_distractions();
        void inherit_distractions( const player_activity & );

        float exertion_level() const;

        bool do_drop_invalid_inventory() const {
            return !actor || actor->do_drop_invalid_inventory();
        }

        std::map<distraction_type, std::string> get_distractions() const;
};

#endif // CATA_SRC_PLAYER_ACTIVITY_H
