#ifndef STRING_INPUT_POPUP_H
#define STRING_INPUT_POPUP_H

#include "cursesdef.h"

#include <string>
#include <map>
#include <set>
#include <functional>

class input_context;

/**
 * Shows a window querying the user for input.
 *
 * Returns the input that was entered. If the user cancels the input (e.g. by pressing escape),
 * an empty string is returned. An empty string may also be returned when the user does not enter
 * any text and confirms the input (by pressing ENTER). It's currently not possible these two
 * situations.
 *
 * @param title The displayed title, describing what to enter. @ref color_tags can be used.
 * @param width Width of the input area where the user input appears.
 * @param input The initially display input. The user can change this.
 * @param desc An optional text (e.h. help or formatting information) which is displayed
 * above the input. Color tags can be used.
 * @param identifier If not empty, this is used to store and retrieve previously entered
 * text. All calls with the same `identifier` share this history, the history is also stored
 * when saving the game (see @ref uistate).
 * @param max_length The maximal length of the text the user can input. More input is simply
 * ignored and the returned string is never longer than this.
 * @param only_digits Whether to only allow digits in the string.
 */
std::string string_input_popup( std::string title, int width = 0, std::string input = "",
                                std::string desc = "", std::string identifier = "",
                                int max_length = -1, bool only_digits = false );

std::string string_input_win( WINDOW *w, std::string input, int max_length, int startx,
                              int starty, int endx, bool loop, long &key, int &pos,
                              std::string identifier = "", int w_x = -1, int w_y = -1,
                              bool dorefresh = true, bool only_digits = false,
                              std::map<long, std::function<void()>> callbacks = std::map<long, std::function<void()>>(),
                              std::set<long> ch_code_blacklist = std::set<long>() );

std::string string_input_win_from_context(
    WINDOW *w, input_context &ctxt, std::string input, int max_length, int startx, int starty,
    int endx, bool loop, std::string &action, long &ch, int &pos, std::string identifier = "",
    int w_x = -1, int w_y = -1, bool dorefresh = true, bool only_digits = false, bool draw_only = false,
    std::map<long, std::function<void()>> callbacks = std::map<long, std::function<void()>>(),
    std::set<long> ch_code_blacklist = std::set<long>() );

#endif
