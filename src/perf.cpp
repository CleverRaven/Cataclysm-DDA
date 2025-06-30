#include "perf.h"

cata_timer::timers_map &cata_timer::top_level_timer_map()
{
    static cata_timer::timers_map map;
    return map;
}

std::vector<cata_timer::timers_map::iterator> &cata_timer::timer_stack()
{
    static std::vector<cata_timer::timers_map::iterator> stack;
    return stack;
}
