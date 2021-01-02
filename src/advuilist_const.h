#ifndef CATA_SRC_ADVUILIST_CONST_H
#define CATA_SRC_ADVUILIST_CONST_H

#include <cstddef>

// constants and literals used by advuilist and friends
namespace advuilist_literals
{

constexpr char const *ACTION_CYCLE_SOURCES = "CYCLE_SOURCES";
constexpr char const *ACTION_DOWN = "DOWN";
constexpr char const *ACTION_EXAMINE = "EXAMINE";
constexpr char const *ACTION_FILTER = "FILTER";
constexpr char const *ACTION_HELP_KEYBINDINGS = "HELP_KEYBINDINGS";
constexpr char const *ACTION_NEXT_SLOT = "ACTION_NEXT_SLOT";
constexpr char const *ACTION_PAGE_DOWN = "PAGE_DOWN";
constexpr char const *ACTION_PAGE_UP = "PAGE_UP";
constexpr char const *ACTION_PREV_SLOT = "ACTION_PREV_SLOT";
constexpr char const *ACTION_QUIT = "QUIT";
constexpr char const *ACTION_RESET_FILTER = "RESET_FILTER";
constexpr char const *ACTION_SELECT = "CONFIRM";
constexpr char const *ACTION_SELECT_ALL = "CONFIRM_ALL";
constexpr char const *ACTION_SELECT_PARTIAL = "CONFIRM_PARTIAL";
constexpr char const *ACTION_SELECT_WHOLE = "CONFIRM_WHOLE";
constexpr char const *ACTION_SORT = "SORT";
constexpr char const *ACTION_SOURCE_PRFX = "SOURCE_";
constexpr char const *ACTION_SWITCH_PANES = "SWITCH_PANES";
constexpr char const *ACTION_UP = "UP";
constexpr char const *CTXT_DEFAULT = "UILIST";
constexpr char const *ITEMS_DEFAULT = "ITEMS_DEFAULT";
constexpr char const *PANE_LEFT = "LEFT";
constexpr char const *PANE_RIGHT = "RIGHT";
constexpr char const *SAVE_DEFAULT = "SAVE_DEFAULT";
constexpr char const *TOGGLE_AUTO_PICKUP = "TOGGLE_AUTO_PICKUP";
constexpr char const *TOGGLE_FAVORITE = "TOGGLE_FAVORITE";
constexpr std::size_t const ACTION_SOURCE_PRFX_len = 7; // where is c++20 when you need it?

} // namespace advuilist_literals

#endif // CATA_SRC_ADVUILIST_CONST_H
