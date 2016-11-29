#ifndef PLAYER_ACTIVITY_H
#define PLAYER_ACTIVITY_H

#include "enums.h"
#include "json.h"
#include "item_location.h"
#include "string_id.h"

#include <climits>
#include <vector>

class player;
class Character;
class player_activity;
class activity_type;

using activity_id = string_id<activity_type>;

template<>
const activity_type &string_id<activity_type>::obj() const;

enum class based_on_type : int {
    TIME = 0,
    SPEED,
    NEITHER
};

/** A class that stores constant information that doesn't differ between activities of the same type */
class activity_type
{
    private:
        activity_id id_;
        bool rooted_ = false;
        std::string stop_phrase_ = "THIS IS A BUG";
        bool abortable_ = true;
        bool suspendable_ = true;
        based_on_type based_on_ = based_on_type::SPEED;
        bool no_resume_ = false;

    public:
        const activity_id &id() const {
            return id_;
        }
        bool rooted() const {
            return rooted_;
        }
        bool abortable() const {
            return abortable_;
        }
        bool suspendable() const {
            return suspendable_;
        }
        std::string stop_phrase() const {
            return stop_phrase_;
        }
        based_on_type based_on() const {
            return based_on_;
        }
        bool no_resume() const {
            return no_resume_;
        }

        void call_do_turn( player_activity *, player * ) const;
        /** Returns whether it had a finish function or not */
        bool call_finish( player_activity *, player * ) const;

        /** JSON stuff */
        static void load( JsonObject &jo );
        static void check_consistency();
        static void reset();
};

class player_activity : public JsonSerializer, public JsonDeserializer
{
    private:
        void finish( player *p );
        activity_id type;
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
        bool ignore_trivial;
        std::vector<int> values;
        std::vector<std::string> str_values;
        std::vector<tripoint> coords;
        tripoint placement;
        /** If true, the player has been warned of dangerously close monsters with
         * respect to this activity.
         */
        bool warned_of_proximity;
        /** If true, the activity will be auto-resumed next time the player attempts
         *  an identical activity. This value is set dynamically.
         */
        bool auto_resume;

        player_activity() : type( activity_id( NULL_ID ) ) { }
        player_activity( activity_id, int turns = 0, int Index = -1, int pos = INT_MIN,
                         std::string name_in = "" );
        player_activity( player_activity && ) = default;
        player_activity( const player_activity & );
        player_activity &operator=( player_activity && ) = default;
        player_activity &operator=( const player_activity & );

        explicit operator bool() const {
            return !type.is_null();
        }
        bool is_null() const {
            return type.is_null();
        }
        /** This replaces the former usage `act.type = ACT_NULL` */
        void set_to_null() {
            type = activity_id( NULL_ID );
        }
        const activity_id &id() const {
            return type;
        }
        bool rooted() const {
            return type->rooted();
        }

        // Question to ask when the activity is to be stoped,
        // e.g. "Stop doing something?", already translated.
        std::string get_stop_phrase() const {
            return type->stop_phrase();
        }
        /**
         * If this returns true, the activity can be aborted with
         * the ACTION_PAUSE key (see game::handle_key_blocking_activity)
         */
        bool is_abortable() const {
            return type->abortable();
        }
        int get_value( size_t index, int def = 0 ) const;
        std::string get_str_value( size_t index, const std::string def = "" ) const;
        /**
         * If this returns true, the action can be continued without
         * starting from scratch again (see player::backlog). This is only
         * possible if the player start the very same activity (with the same
         * parameters) again.
         */
        bool is_suspendable() const {
            return type->suspendable();
        }

        using JsonSerializer::serialize;
        void serialize( JsonOut &jsout ) const override;
        using JsonDeserializer::deserialize;
        void deserialize( JsonIn &jsin ) override;
        /** Convert from the old enumeration to the new string_id */
        void deserialize_legacy_type( int legacy_type, activity_id &dest );

        /**
         * Performs the activity for a single turn. If the activity is complete
         * at the end of the turn, do_turn also executes whatever actions, if
         * any, are needed to conclude the activity.
         */
        void do_turn( player *p );

        /**
         * Returns true if activities are similar enough that this activity
         * can be resumed instead of starting the other activity.
         */
        bool can_resume_with( const player_activity &other, const Character &who ) const;
};

#endif
