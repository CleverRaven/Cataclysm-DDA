#include "player_activity.h"
#include "translations.h"

player_activity::player_activity(activity_type t, int turns, int Index, int pos,
                                 std::string name_in) :
    JsonSerializer(), JsonDeserializer(), type(t), moves_left(turns), index(Index),
    position(pos), name(name_in), ignore_trivial(false), values(), str_values(),
    placement(-1, -1), warned_of_proximity(false), auto_resume(false)
{
}

const std::string &player_activity::get_stop_phrase() const
{
    static const std::string stop_phrase[NUM_ACTIVITIES] = {
        _(" Stop?"), _(" Stop reloading?"),
        _(" Stop reading?"), _(" Stop playing?"),
        _(" Stop waiting?"), _(" Stop crafting?"),
        _(" Stop crafting?"), _(" Stop disassembly?"),
        _(" Stop butchering?"), _(" Stop salvaging?"), _(" Stop foraging?"),
        _(" Stop construction?"), _(" Stop construction?"),
        _(" Stop pumping gas?"), _(" Stop training?"),
        _(" Stop waiting?"), _(" Stop using first aid?"),
        _(" Stop fishing?"), _(" Stop mining?"), _(" Stop burrowing?"),
        _(" Stop smashing?"), _(" Stop de-stressing?"),
        _(" Stop cutting tissues?"), _(" Stop dropping?"),
        _(" Stop stashing?"), _(" Stop picking up?"),
        _(" Stop moving items?"),
        _(" Stop interacting with inventory?"),
        _(" Stop lighting the fire?"),_(" Stop filling the container?"),
         _(" Stop hotwiring the vehicle?"),
        _(" Stop aiming?")
    };
    return stop_phrase[type];
}

bool player_activity::is_abortable() const
{
    switch(type) {
    case ACT_READ:
    case ACT_BUILD:
    case ACT_CRAFT:
    case ACT_LONGCRAFT:
    case ACT_REFILL_VEHICLE:
    case ACT_WAIT:
    case ACT_WAIT_WEATHER:
    case ACT_FIRSTAID:
    case ACT_PICKAXE:
    case ACT_BURROW:
    case ACT_PULP:
    case ACT_MAKE_ZLAVE:
    case ACT_DROP:
    case ACT_STASH:
    case ACT_PICKUP:
    case ACT_HOTWIRE_CAR:
    case ACT_MOVE_ITEMS:
    case ACT_ADV_INVENTORY:
    case ACT_START_FIRE:
    case ACT_FILL_LIQUID:
        return true;
    default:
        return false;
    }
}

bool player_activity::is_suspendable() const
{
    switch(type) {
    case ACT_NULL:
    case ACT_RELOAD:
    case ACT_DISASSEMBLE:
    case ACT_MAKE_ZLAVE:
    case ACT_DROP:
    case ACT_STASH:
    case ACT_PICKUP:
    case ACT_MOVE_ITEMS:
    case ACT_ADV_INVENTORY:
    case ACT_AIM:
        return false;
    default:
        return true;
    }
}


int player_activity::get_value(int index, int def) const
{
    return ((size_t)index < values.size()) ? values[index] : def;
}

std::string player_activity::get_str_value(int index, std::string def) const
{
    return ((size_t)index < str_values.size()) ? str_values[index] : def;
}
