#ifndef _PLAYER_ACTIVITY_H_
#define _PLAYER_ACTIVITY_H_

#include "enums.h"
#include "json.h"
#include <vector>
#include <climits>

enum activity_type {
 ACT_NULL = 0,
 ACT_RELOAD, ACT_READ, ACT_GAME, ACT_WAIT, ACT_CRAFT, ACT_LONGCRAFT,
 ACT_DISASSEMBLE, ACT_BUTCHER, ACT_FORAGE, ACT_BUILD, ACT_VEHICLE, ACT_REFILL_VEHICLE,
 ACT_TRAIN, ACT_WAIT_WEATHER, ACT_FIRSTAID,
 ACT_FISH, ACT_PICKAXE, ACT_PULP,
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

    player_activity(activity_type t = ACT_NULL, int turns = 0, int Index = -1, int pos = INT_MIN, std::string name_in = "");
    player_activity(const player_activity &copy);

    // Question to ask when the activity is to be stoped,
    // e.g. " Stop doing something?", already translated.
    const std::string &get_stop_phrase() const;
    bool is_abortable() const;
    int get_value(int index, int def = 0) const;
    std::string get_str_value(int index, const std::string def = "") const;

    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin);

    void load_legacy(std::stringstream &dump);
};

#endif
