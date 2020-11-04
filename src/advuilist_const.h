#ifndef CATA_SRC_ADVUILIST_CONST_H
#define CATA_SRC_ADVUILIST_CONST_H

// constants and literals used by advuilist and friends
namespace advuilist_literals {

constexpr auto const ACTION_CYCLE_SOURCES = "CYCLE_SOURCES";
constexpr auto const ACTION_DOWN = "DOWN";
constexpr auto const ACTION_EXAMINE = "EXAMINE";
constexpr auto const ACTION_FILTER = "FILTER";
constexpr auto const ACTION_HELP_KEYBINDINGS = "HELP_KEYBINDINGS";
constexpr auto const ACTION_PAGE_DOWN = "PAGE_DOWN";
constexpr auto const ACTION_PAGE_UP = "PAGE_UP";
constexpr auto const ACTION_QUIT = "QUIT";
constexpr auto const ACTION_RESET_FILTER = "RESET_FILTER";
constexpr auto const ACTION_SELECT = "SELECT";
constexpr auto const ACTION_SELECT_PARTIAL = "SELECT_PARTIAL";
constexpr auto const ACTION_SORT = "SORT";
constexpr auto const ACTION_SOURCE_PRFX = "SOURCE_";
constexpr auto const ACTION_SOURCE_PRFX_len = 7; // where is c++20 when you need it?
constexpr auto const ACTION_SWITCH_PANES = "SWITCH_PANES";
constexpr auto const ACTION_UP = "UP";
constexpr auto const CTXT_DEFAULT = "default";

} // namespace advuilist_literals

#endif // CATA_SRC_ADVUILIST_CONST_H
