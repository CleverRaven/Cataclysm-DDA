#pragma once
#ifndef CATA_SRC_DEBUG_MONITOR_TARGETS_H
#define CATA_SRC_DEBUG_MONITOR_TARGETS_H

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace debug_menu
{

// One entry per kind of monitor the user can register from the Trace
// tab's Monitors > Add row. Mirrors the action-table pattern next door in
// debug_actions_table.h: label, hint, optional validator, snap factory.
struct monitor_target_entry {
    // Combo label shown in the dropdown.
    const char *label;
    // Hint string for the input box. Empty means the kind takes no input
    // (the input box is read-only).
    const char *hint;
    // Optional pre-construction validator: nullptr means "always accept".
    // Returning false disables the Add button without trying to call
    // make_snap on bad input.
    bool ( *validate )( std::string_view );
    // Builds the snap closure for this kind. May return an empty
    // std::function when input is unusable; the caller treats empty as
    // "do not register". The closure itself runs every snapshot.
    std::function<std::string()> ( *make_snap )( std::string_view input );
};

const std::vector<monitor_target_entry> &all_monitor_targets();

} // namespace debug_menu

#endif // CATA_SRC_DEBUG_MONITOR_TARGETS_H
