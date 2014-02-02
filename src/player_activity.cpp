#include "player_activity.h"
#include "translations.h"

player_activity::player_activity(activity_type t, int turns, int Index, int pos, std::string name_in)
: JsonSerializer(), JsonDeserializer()
, type(t)
, moves_left(turns)
, index(Index)
, position(pos)
, name(name_in)
, ignore_trivial(false)
, values()
, str_values()
, placement(-1,-1)
, warned_of_proximity(false)
{
}

player_activity::player_activity(const player_activity &copy)
: JsonSerializer(), JsonDeserializer()
, type(copy.type)
, moves_left(copy.moves_left)
, index(copy.index)
, position(copy.position)
, name(copy.name)
, ignore_trivial(copy.ignore_trivial)
, values(copy.values)
, str_values(copy.str_values)
, placement(copy.placement)
, warned_of_proximity(false) // Don't copy this?
{
}

const std::string &player_activity::get_stop_phrase() const {
    static const std::string stop_phrase[NUM_ACTIVITIES] = {
        _(" Stop?"), _(" Stop reloading?"),
        _(" Stop reading?"), _(" Stop playing?"),
        _(" Stop waiting?"), _(" Stop crafting?"),
        _(" Stop crafting?"), _(" Stop disassembly?"),
        _(" Stop butchering?"), _(" Stop foraging?"),
        _(" Stop construction?"), _(" Stop construction?"),
        _(" Stop pumping gas?"), _(" Stop training?"),
        _(" Stop waiting?"), _(" Stop using first aid?"),
        _(" Stop fishing?"), _(" Stop mining?"),
        _(" Stop smashing?")
    };
    return stop_phrase[type];
}

bool player_activity::is_abortable() const {
    switch(type) {
        case ACT_READ:
        case ACT_BUILD:
        case ACT_LONGCRAFT:
        case ACT_REFILL_VEHICLE:
        case ACT_WAIT:
        case ACT_WAIT_WEATHER:
        case ACT_FIRSTAID:
        case ACT_PICKAXE:
        case ACT_PULP:
            return true;
        default:
            return false;
    }
}

bool player_activity::is_suspendable() const {
    switch(type) {
        case ACT_NULL:
        case ACT_RELOAD:
        case ACT_DISASSEMBLE:
            return false;
        default:
            return true;
    }
}


int player_activity::get_value(int index, int def) const {
    return (index < values.size()) ? values[index] : def;
}

std::string player_activity::get_str_value(int index, std::string def) const {
    return (index < str_values.size()) ? str_values[index] : def;
}
