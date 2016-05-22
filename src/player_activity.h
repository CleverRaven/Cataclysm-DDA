#ifndef PLAYER_ACTIVITY_H
#define PLAYER_ACTIVITY_H

#include "enums.h"
#include "json.h"
#include "item_location.h"

#include <climits>
#include <vector>

enum activity_type : int {    // expanded this enum for readability
    ACT_NULL = 0,
    ACT_RELOAD,
    ACT_READ,
    ACT_GAME,
    ACT_WAIT,
    ACT_CRAFT,
    ACT_LONGCRAFT,
    ACT_DISASSEMBLE,
    ACT_BUTCHER,
    ACT_LONGSALVAGE,
    ACT_FORAGE,
    ACT_BUILD,
    ACT_VEHICLE,
    ACT_REFILL_VEHICLE, // not used anymore.
    ACT_TRAIN,
    ACT_WAIT_WEATHER,
    ACT_FIRSTAID,
    ACT_FISH,
    ACT_PICKAXE,
    ACT_BURROW,
    ACT_PULP,
    ACT_VIBE,
    ACT_MAKE_ZLAVE,
    ACT_DROP,
    ACT_STASH,
    ACT_PICKUP,
    ACT_MOVE_ITEMS,
    ACT_ADV_INVENTORY,
    ACT_ARMOR_LAYERS,
    ACT_START_FIRE,
    ACT_OPEN_GATE,
    ACT_FILL_LIQUID,
    ACT_HOTWIRE_CAR,
    ACT_AIM,
    ACT_ATM,
    ACT_START_ENGINES,
    ACT_OXYTORCH,
    ACT_CRACKING,
    ACT_REPAIR_ITEM,
    ACT_MEND_ITEM,
    ACT_GUNMOD_ADD,
    ACT_WAIT_NPC,
    NUM_ACTIVITIES
};

class player;

class player_activity : public JsonSerializer, public JsonDeserializer
{
    private:
        void finish( player *p );

    public:
        player_activity() = default;
        player_activity( const player_activity & ) = delete;
        player_activity &operator= ( const player_activity & ) = delete;
        player_activity( player_activity && ) = default;
        player_activity &operator=( player_activity && ) = default;

        player_activity( activity_type type, item_location &&target, int moves,
                         std::string key = std::string(), const std::vector<int> &vals = {} )
            : type( type ), moves_total( moves ), moves_left( moves ),
              targets( {
            {
                std::move( target ), item_location(), item_location()
            }
        } ),
        name( key ), values( vals ) {}

        bool operator==( const player_activity &rhs ) const {
            return type == rhs.type &&
                   targets == rhs.targets &&
                   values == rhs.values &&
                   str_values == rhs.str_values &&
                   coords == rhs.coords &&
                   index == rhs.index &&
                   position == rhs.position &&
                   placement == rhs.placement;
        }

        bool operator!=( const player_activity &rhs ) const {
            return !this->operator==( rhs );
        }

        operator bool() const {
            return type != ACT_NULL;
        }

        // Question to ask when the activity is to be stoped,
        // e.g. " Stop doing something?", already translated.
        const std::string &get_stop_phrase() const;
        /**
         * If this returns true, the activity can be aborted with
         * the ACTION_PAUSE key (see game::handle_key_blocking_activity)
         */
        bool is_abortable() const;
        /**
         * If this returns true, the activity does not finish. This is
         * the type of activities that use UI trickery, but must be cancelled
         * manually!
         */
        bool never_completes() const;
        int get_value( size_t index, int def = 0 ) const;
        std::string get_str_value( size_t index, const std::string def = "" ) const;
        /**
         * If this returns true, the action can be continued without
         * starting from scratch again (see player::backlog). This is only
         * possible if the player start the very same activity (with the same
         * parameters) again.
         */
        bool is_suspendable() const;

        using JsonSerializer::serialize;
        void serialize( JsonOut &jsout ) const override;
        using JsonDeserializer::deserialize;
        void deserialize( JsonIn &jsin ) override;

        /**
         * Performs the activity for a single turn. If the activity is complete
         * at the end of the turn, do_turn also executes whatever actions, if
         * any, are needed to conclude the activity.
         */
        void do_turn( player *p );

        /**
         * Returns true if the activity is complete.
         */
        bool is_complete() const;

        activity_type type = ACT_NULL; /** type of activity. */
        int moves_total = 0; /** total moves required to complete activity */
        int moves_left = 0; /** moves remaining before activity complete */

        /*@{*/
        /** activity specific values */
        std::array<item_location, 3> targets = { { item_location(), item_location(), item_location() } };
        std::string name;
        std::vector<int> values;
        std::vector<std::string> str_values;
        std::vector<tripoint> coords;
        /*@}*/

        /*@{*/
        /** deprecated activity specific values */
        int index = -1;
        int position = INT_MIN;
        tripoint placement = tripoint_min;
        /*@}*/

        bool ignore_trivial = false; /** @todo document field */

        /** If true, the player has been warned of dangerously close monsters with
         * respect to this activity. */
        bool warned_of_proximity = false;

        /** If true, the activity will be auto-resumed next time the player attempts
         *  an identical activity. This value is set dynamically */
        bool auto_resume = false;
};

#endif
