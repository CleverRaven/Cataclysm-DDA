#include "current_map.h"
#include "debug.h"
#include "game.h"
#include "map.h"

map& current_map::get_map() {
    return *current;
}

swap_map::swap_map(map& new_map, std::string context) {
    previous = g->current_map.current;
    previous_context = g->current_map.context_;

    g->current_map.set(&new_map, context);

    DebugLog(D_INFO, D_MAIN) << "swap_map swapped from '" << previous_context << "' to '" << context << "'";
    return;
}

swap_map::~swap_map() {
    DebugLog(D_INFO, D_MAIN) << "swap_map swapped back from '" << previous_context << "'";
    g->current_map.set(previous, previous_context);
}

