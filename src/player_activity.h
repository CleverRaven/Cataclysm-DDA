#pragma once
#ifndef PLAYER_ACTIVITY_H
#define PLAYER_ACTIVITY_H

#include <cstddef>
#include <climits>
#include <set>
#include <vector>
#include <memory>
#include <string>
#include <unordered_set>

#include "enums.h"
#include "item_location.h"
#include "point.h"
#include "string_id.h"

class player;
class Character;
class JsonIn;
class JsonOut;
class activity_type;
class monster;
class translation;

using activity_id = string_id<activity_type>;

class player_activity
{
    private:
        activity_id type;
        std::set<distraction_type> ignored_distractions;
    public:
        /** Total number of moves required to complete the activity */
        int moves_total;
        /** The number of moves remaining in this activity before it is complete. */
        int moves_left;
        /** An activity specific value. */
        int index;
        /** An activity specific value. */
        int position;
        /** An activity specific value. */
        std::string name;
        std::vector<item_location> targets;
        std::vector<int> values;
        std::vector<std::string> str_values;
        std::vector<tripoint> coords;
        std::unordered_set<tripoint> coord_set;
        std::vector<std::weak_ptr<monster>> monsters;
        tripoint placement;
        /** If true, the activity will be auto-resumed next time the player attempts
         *  an identical activity. This value is set dynamically.
         */
        bool auto_resume;

        player_activity();
        player_activity( activity_id, int turns = 0, int Index = -1, int pos = INT_MIN,
                         const std::string &name_in = "" );
        player_activity( player_activity && ) = default;
        player_activity( const player_activity & ) = default;
        player_activity &operator=( player_activity && ) = default;
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

        const activity_id &id() const {
            return type;
        }
        bool rooted() const;

        // Question to ask when the activity is to be stopped,
        // e.g. "Stop doing something?", already translated.
        std::string get_stop_phrase() const;

        const translation &get_verb() const;

        int get_value( size_t index, int def = 0 ) const;
        std::string get_str_value( size_t index, const std::string &def = "" ) const;
        /**
         * If this returns true, the action can be continued without
         * starting from scratch again (see player::backlog). This is only
         * possible if the player start the very same activity (with the same
         * parameters) again.
         */
        bool is_suspendable() const;

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
        /** Convert from the old enumeration to the new string_id */
        void deserialize_legacy_type( int legacy_type, activity_id &dest );

        /**
         * Performs the activity for a single turn. If the activity is complete
         * at the end of the turn, do_turn also executes whatever actions, if
         * any, are needed to conclude the activity.
         */
        void do_turn( player &p );

        /**
         * Returns true if activities are similar enough that this activity
         * can be resumed instead of starting the other activity.
         */
        bool can_resume_with( const player_activity &other, const Character &who ) const;

        bool is_distraction_ignored( distraction_type type ) const;
        void ignore_distraction( distraction_type type );
        void allow_distractions();
        void inherit_distractions( const player_activity & );
};

#endif
