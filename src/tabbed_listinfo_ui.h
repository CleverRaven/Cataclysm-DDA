#include "input_context.h"
#include "uilist.h"

/*
* A general UI interface for displaying an arbitrary number of tabs containing the following:
* 1. a `uilist` that handles all list selections and inputs, displayed on left
* 2. anything else ("info", usually for the `uilist` selection), displayed on right 
*    under the uilist
* 
* TODO: drawing a window on top of another window, while functional, isn't great
* practice. This class was designed this way because `uilist` was originally implemented as
* a standalone prompt window (and then ported to ImGui like that) but also contained many 
* list features that shouldn't be re-implemented over and over in ImGui, particularly 
* because ImGui key inputs can't be processed by `input_context`.
*/

class tabbed_listinfo_ui {
public:
    virtual void setup_new_uilist() = 0;
    // update uilist_entries for current tab, but do not modify uilist_entry count
    virtual void update_uilist_entries() = 0;
    virtual void upon_switching_tab() = 0;
    virtual void update_uilist_position(ImVec2 new_position) = 0;
    virtual void setup_input_context(input_context& cc_ictxt) = 0;
    // @return action was handled
    virtual bool handle_action(const std::string& action) = 0;
    virtual std::shared_ptr<uilist> get_current_tab_uilist() = 0;
    virtual void set_current_tab_uilist(const std::shared_ptr<uilist>& new_uilist) = 0;
    virtual input_context& get_current_tab_input() = 0;
    virtual void set_current_tab_input(const input_context& new_input) = 0;

    static cataimgui::bounds uilist_reset_desired_bounds();
    // convenience function for constructing a new `uilist_entry`
    static uilist_entry get_uilist_entry(const std::string& name);
    // used to set scrolling for info window
    static void set_info_scroll(bool& scrolled_down, bool& scrolled_up);
};
