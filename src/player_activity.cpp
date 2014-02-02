#include "player_activity.h"
#include "translations.h"

player_activity::player_activity(activity_type t, int turns, int Index, int pos, std::string name_in)
: JsonSerializer(), JsonDeserializer()
, type(t)
, moves_left(turns)
, index(Index)
, position(pos)
, name(name_in)
, continuous(false)
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
, continuous(copy.continuous)
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
        _(" Stop smasing?")
    };
    return stop_phrase[type];
}
