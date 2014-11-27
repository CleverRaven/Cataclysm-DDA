#ifndef PLAYER_ACTIVITY_H
#define PLAYER_ACTIVITY_H

#include "enums.h"
#include "json.h"
#include <vector>
#include <climits>

enum activity_type {    // expanded this enum for readability
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
    ACT_REFILL_VEHICLE,
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
    ACT_START_FIRE,
    ACT_FILL_LIQUID,
    ACT_HOTWIRE_CAR,
    ACT_AIM,
    NUM_ACTIVITIES
};

class player_activity : public JsonSerializer, public JsonDeserializer
{
    public:
        activity_type type;
        int moves_left;
        int index;
        int position;
        std::string name;
        bool ignore_trivial;
        std::vector<int> values;
        std::vector<std::string> str_values;
        point placement;
        bool warned_of_proximity; // True if player has been warned of dangerously close monsters
        // Property that makes the activity resume if the previous activity completes.
        bool auto_resume;

        player_activity(activity_type t = ACT_NULL, int turns = 0, int Index = -1, int pos = INT_MIN,
                        std::string name_in = "");
        player_activity(player_activity &&) = default;
        player_activity(const player_activity &) = default;
        player_activity &operator=(player_activity &&) = default;
        player_activity &operator=(const player_activity &) = default;

        // Question to ask when the activity is to be stoped,
        // e.g. " Stop doing something?", already translated.
        const std::string &get_stop_phrase() const;
        /**
         * If this returns true, the activity can be aborted with
         * the ACTION_PAUSE key (see game::handle_key_blocking_activity)
         */
        bool is_abortable() const;
        int get_value(int index, int def = 0) const;
        std::string get_str_value(int index, const std::string def = "") const;
        /**
         * If this returns true, the action can be continued without
         * starting from scratch again (see player::backlog). This is only
         * possible if the player start the very same activity (with the same
         * parameters) again.
         */
        bool is_suspendable() const;

        using JsonSerializer::serialize;
        void serialize(JsonOut &jsout) const;
        using JsonDeserializer::deserialize;
        void deserialize(JsonIn &jsin);

        void load_legacy(std::stringstream &dump);
};

#endif
