#pragma once
#ifndef CATA_SRC_DEBUG_ACTIONS_TABLE_H
#define CATA_SRC_DEBUG_ACTIONS_TABLE_H

#include <vector>

#include "debug_menu_types.h"

namespace debug_menu
{

struct debug_action_entry {
    debug_menu_index id;
    const char *label;
    const char *search_keys;
    const char *category;
    // Opaque function pointer to the code for the specific debug menu entry,
    // like a case statement in a switch. Prevents accidentally lambda-
    // capturing state.
    void ( *execute )();
    // Button tooltip. Wrapped with _() at render time; store the
    // English source. nullptr falls back to label + category.
    const char *tooltip = nullptr;
    // True for actions that immediately open a uilist dialog with no inline
    // console control. Omnibox search-mode jump shows a "no inline control"
    // footer instead of pulsing nothing.
    bool jump_only = false;
};

const std::vector<debug_action_entry> &all_actions();

// O(1) lookup by id. Builds an index on first call.
const debug_action_entry *find_action( debug_menu_index id );

} // namespace debug_menu

#endif // CATA_SRC_DEBUG_ACTIONS_TABLE_H
