#pragma once
#ifndef CATA_SRC_DEBUG_CONSOLE_H
#define CATA_SRC_DEBUG_CONSOLE_H

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <imgui/imgui.h>

#include "cata_imgui.h"
#include "coordinates.h"
#include "debug.h"
#include "debug_capture.h"
#include "input_context.h"
#include "pimpl.h"
#include "type_id.h"

class Character;
class JsonObject;
class JsonOut;
class character_id;

namespace debug_menu
{
enum class debug_menu_index : int;

class debug_console;
class omnibox_state;
class step_controller;

// Base type for a top-level tab in the debug console window. Tabs implement
// draw_body only -- the host owns BeginTabItem/EndTabItem and selection
// state, so tab subclasses do not see the tab-bar protocol. Tab identity is
// owned by the tab_registry table in debug_console.cpp; subclasses do not
// carry it themselves.
class console_tab_view
{
    public:
        virtual ~console_tab_view() = default;

        virtual const char *label() const = 0;

        // Called by the host only when this tab is the active tab in the
        // tab bar. Tab is expected to draw its body contents only -- no
        // BeginTabItem / EndTabItem wrapping.
        virtual void draw_body( debug_console &host ) = 0;

        // Persistence hooks. Host writes per-tab state under a nested
        // "tabs":{<id>:{...}} block via save_state. On load, the nested
        // object is passed in `nested` (empty if absent).
        virtual void load_state( const JsonObject &nested ) {
            ( void )nested;
        }
        virtual void save_state( JsonOut & ) const {}
};

// Scratch for the editable Character sections (skill bulk-set sliders and
// proficiency filter), held per tab that renders them.
struct character_edit_state {
    int bulk_theory = 0;
    int bulk_practice = 0;
    std::string prof_filter;
};

class tab_spawn_view : public console_tab_view
{
    public:
        const char *label() const override;
        void draw_body( debug_console &host ) override;
};

class tab_game_view : public console_tab_view
{
    public:
        const char *label() const override;
        void draw_body( debug_console &host ) override;
};

class tab_map_view : public console_tab_view
{
    public:
        const char *label() const override;
        void draw_body( debug_console &host ) override;
};

class tab_vehicle_view : public console_tab_view
{
    public:
        const char *label() const override;
        void draw_body( debug_console &host ) override;
};

class tab_tiles_view : public console_tab_view
{
    public:
        const char *label() const override;
        void draw_body( debug_console &host ) override;
        void load_state( const JsonObject &nested ) override;
        void save_state( JsonOut &jo ) const override;

    private:
        std::string tile_coord_input;
};

class tab_player_view : public console_tab_view
{
    public:
        const char *label() const override;
        void draw_body( debug_console &host ) override;

    private:
        character_edit_state char_edit;
};

class tab_eoc_view : public console_tab_view
{
    public:
        const char *label() const override;
        void draw_body( debug_console &host ) override;
        void load_state( const JsonObject &nested ) override;
        void save_state( JsonOut &jo ) const override;

    private:
        std::string func_filter;
        std::string eoc_filter;
        int eoc_type_filter = -1;
        std::string eoc_selected_id;

        // Eval state owned here. Host holds only the deferred-execution
        // flag + transit buffer (see request_eval / consume_eval_result).
        std::string eval_expr;
        std::string eval_result;
        bool eval_ok = true;
        bool eval_auto = false;
        int eval_last_turn = -1;

        void draw_eval_panel( debug_console &host );
        void draw_eoc_browser( debug_console &host );
};

enum class dv_types : std::uint8_t {
    DOUBLE = 0,
    STRING,
    TRIPOINT,
    last
};

// Edit-popup target and add-row inputs are shared across all var tables, so
// only one add or edit is in flight at a time.
struct var_browser_state {
    std::string filter;
    std::string edit_table;
    std::string edit_key;
    std::string edit_buf;
    dv_types edit_type{};
    std::string new_key;
    std::string new_val;
    dv_types new_var_type{};
};

class tab_data_view : public console_tab_view
{
    public:
        const char *label() const override;
        void draw_body( debug_console &host ) override;
        void load_state( const JsonObject &nested ) override;
        void save_state( JsonOut &jo ) const override;

    private:
        // Per sub-panel scratch state.
        var_browser_state var_browser;
        struct timed_events_state {
            std::string filter;
        } timed_events;
        struct item_wakeups_state {
            std::string filter;
            int kind_filter = -1;
        } item_wakeups;
        struct faction_browser_state {
            std::string filter;
            faction_id edit_fid;
            int edit_likes = 0;
            int edit_respects = 0;
            int edit_trusts = 0;
        } faction_browser;

        void draw_game_state_panel();
        void draw_var_browser( debug_console &host );
        void draw_timed_events_table();
        void draw_item_wakeups_table();
        void draw_faction_browser();
};

class tab_creatures_view : public console_tab_view
{
    public:
        const char *label() const override;
        void draw_body( debug_console &host ) override;
        void load_state( const JsonObject &nested ) override;
        void save_state( JsonOut &jo ) const override;

    private:
        std::string creature_filter;
        std::string creature_selected_id;
        // Filters NPC + monster lists to the loaded reality bubble.
        bool filter_within_bubble = true;
        character_edit_state char_edit;
};

class tab_items_view : public console_tab_view
{
    public:
        const char *label() const override;
        void draw_body( debug_console &host ) override;
        void load_state( const JsonObject &nested ) override;
        void save_state( JsonOut &jo ) const override;

        // Cross-tab entry point called by host's show_items_for_* verbs.
        // Writes the new source string onto this tab's state.
        void set_source( const std::string &source_key );

    private:
        std::string item_filter;
        std::string items_source = "avatar";

        // Stable selection key. Recovers the row across filter changes and
        // source reshuffles by matching (type_id, location, ordinal among
        // peers with the same type+location) rather than by positional index.
        struct item_pick {
            std::string source_key;
            std::string type_id;
            std::string location;
            int instance_rank = 0;
        };
        std::optional<item_pick> selected;

        // Sticky one-line warning shown above the source combo whenever the
        // persisted source no longer maps to an in-bubble entity. Reset when
        // the user picks any source from the combo.
        std::optional<std::string> reverted_from_source;
};

class tab_trace_view : public console_tab_view
{
    public:
        tab_trace_view() {
            log_category_mask.set();
        }
        const char *label() const override;
        void draw_body( debug_console &host ) override;
        void load_state( const JsonObject &nested ) override;
        void save_state( JsonOut &jo ) const override;

    private:
        void draw_monitors_body();

        std::bitset<debugmode::DF_LAST> log_category_mask;
        bool trace_show_log = true;
        bool trace_show_event = true;
        bool trace_show_monitor = true;
        std::string trace_filter;
        bool trace_autoscroll = true;
        size_t trace_prev_row_count = 0;
        std::string monitor_target_input;
        int monitor_target_type = 0;
        int monitor_mode_idx = 1;
        std::string monitor_label;

        // Cached merged feed. Rebuilt only when feed_generation,
        // filter, toggle mask, or log-category mask change.
        struct trace_row {
            int turn;
            int source;
            std::string label;
            std::string body;
            int log_cat = -1;
        };
        std::vector<trace_row> cached_rows;
        uint64_t cached_feed_gen = static_cast<uint64_t>( -1 );
        std::string cached_filter;
        uint8_t cached_toggle_mask = 0;
        std::bitset<debugmode::DF_LAST> cached_log_mask;
        bool cached_eoc_visible = true;
};


// Group controls under a label with indent. SeparatorText + Indent rather
// than BeginChild because the latter's scroll clip rect breaks combo popup
// click handling.
void framed_section( const char *id, const char *label,
                     const std::function<void()> &body );

// Vararg format applies to value only.
template<typename... Args>
inline void label_value( const char *label, const char *fmt, Args &&... args )
{
    ImGui::TextDisabled( "%s:", label );
    ImGui::SameLine( 0, 4.0f );
    ImGui::Text( fmt, std::forward<Args>( args )... );
}

// SliderInt that invokes `set` only when the user actually moves it.
template<typename Getter, typename Setter>
inline void stat_slider( const char *label, int lo, int hi,
                         Getter get, Setter set, float w = 180.0f )
{
    int v = get();
    ImGui::SetNextItemWidth( w );
    if( ImGui::SliderInt( label, &v, lo, hi ) ) {
        set( v );
    }
}

// +4 cushion accounts for borders/cell padding so the last row isn't clipped.
float fit_table_height( int n_rows, float cap_h );

// Auto-sized column: width tracks the wider of its header text and its visible
// cells, so neither clips. Use for columns with a visible header.
void fit_col( const char *label, ImGuiTableColumnFlags extra_flags = 0 );

struct slider_row {
    std::string label;
    int lo;
    int hi;
    std::function<int()> get;
    std::function<void( int )> set;
};

// Aligned grid of scalar int sliders: `pairs_per_row` label+slider pairs per
// row sharing one label column so the tracks line up.
void scalar_slider_table( const char *id, const std::vector<slider_row> &rows,
                          int pairs_per_row = 2 );

// Editable Character sections, reusable for the avatar or any Character (NPC).
// Monitor (+M) buttons target the given character via the char_* snapshots.
void render_char_stats( Character &ch );
void render_char_vitals( Character &ch );
void render_char_metabolism( Character &ch );
void render_char_skills( Character &ch, character_edit_state &st );
void render_char_proficiencies( Character &ch, character_edit_state &st );

// "+M" button registering a monitor with given label and snapshot.
void add_monitor_button( const char *id, const std::string &label,
                         std::function<std::string()> snap,
                         monitor_mode mode = monitor_mode::on_change );

// Stable distinct color per category index via golden-angle hue spread.
// Saturation/value tuned so text stays readable on the default ImGui dark theme.
ImVec4 category_color( int cat );

// Score how well `needle` matches `haystack`. Higher = better, 0 = no match.
// Case-insensitive subsequence match with bonuses for consecutive runs and
// matching at position 0.
int fuzzy_score( std::string_view haystack, std::string_view needle );

// Public flat view of the tab registry. Excludes the factory function so
// callers can introspect tab order/identity and category routing without
// pulling in tab subclasses. Used by tab_registry_invariants and any other
// caller that needs to reason about tab id strings.
struct tab_registry_view_entry {
    std::string_view id;
    std::vector<std::string_view> categories;
};
std::vector<tab_registry_view_entry> tab_registry_snapshot();


class debug_console : public cataimgui::window
{
    public:
        debug_console();
        void execute();

        void defer_action( debug_menu_index action );
        void defer_eoc( effect_on_condition_id eoc_id );
        void request_tab_switch( std::string_view tab_id );

        // Button label + tooltip read from the action table.
        bool debug_button( debug_menu_index action );
        // Caller-supplied label override; tooltip still from the table.
        bool debug_button_with_label( const char *label, debug_menu_index action );

        // Confirmation popup before deferring. For destructive actions.
        void confirm_button( const char *label, const char *popup_id,
                             const char *prompt, debug_menu_index action );

        // Timed window after an omnibox jump fired; drives the jump-only
        // follow-up text.
        bool is_omnibox_highlight( debug_menu_index action ) const;
        // One-shot focus claim. Returns true once per omnibox jump, then
        // clears so SetKeyboardFocusHere doesn't fight user navigation.
        bool take_omnibox_focus( debug_menu_index action );

        // Checkbox that participates in the omnibox highlight pulse. Pass
        // an inert pulse_target of debug_menu_index::last when no omnibox
        // entry maps to this checkbox. Returns true if the user toggled.
        bool highlight_checkbox( const char *label, bool *value,
                                 debug_menu_index pulse_target );

        // Export helpers -- builders run only when user clicks a Copy button,
        // avoiding per-frame string construction for large ring buffers.
        void export_bar( const std::string &name,
                         const std::function<std::string()> &build_md,
                         const std::function<std::string()> &build_json );

        // Semantic cross-tab verbs. Each verb mutates the target tab's
        // state through a typed back-pointer cached at construction, then
        // switches the active tab.
        void show_items_for_npc( character_id who );
        void show_items_for_monster( const std::string &type_id,
                                     const tripoint_bub_ms &pos );

        // Eval handoff. request_eval() snapshots the expression; outer
        // execute() runs it between frames so parser debugmsg popups
        // don't corrupt the ImGui frame. consume_eval_result() returns
        // the latest result or nullopt if none since last consume.
        void request_eval( const std::string &expr );
        struct eval_result_view {
            std::string result;
            bool ok = true;
        };
        std::optional<eval_result_view> consume_eval_result();

        // Trace tab reads; EOC tab clears when the user pins a per-EOC
        // monitor (raw feed becomes redundant).
        void set_eoc_trace_visible( bool v );
        bool eoc_trace_visible() const;

    protected:
        void draw_controls() override;
        void draw() override;
        cataimgui::bounds get_bounds() override;

    private:
        // Indices into tab_registry() (in debug_console.cpp). The registry
        // is the single source of truth for tab order, identity, and category
        // routing. `switch_tab_idx_` is set when a hotkey, NEXT/PREV, or
        // semantic verb wants the tab bar to swap; the next draw consumes
        // it.
        std::size_t selected_tab_idx_ = 0;
        std::optional<std::size_t> switch_tab_idx_;
        input_context ctxt;

        // Per-frame iteration over registered tab_views. Host owns
        // BeginTabItem/EndTabItem; tab classes implement only draw_body.
        void draw_tab_views();
        std::vector<std::unique_ptr<console_tab_view>> tab_views;

        // Typed back-pointer cached at construction for the only tab that
        // currently receives cross-tab semantic verbs.
        tab_items_view *items_view_ = nullptr;

        // Deferred operations -- queued by buttons during draw, drained one
        // per outer-loop iteration so each runs outside the ImGui frame.
        // FIFO over action triggers and EOC fires interleaves them by
        // enqueue order; the queue is bounded at 256.
        struct deferred_action {
            debug_menu_index id;
        };
        struct deferred_eoc {
            effect_on_condition_id id;
        };
        using deferred_op = std::variant<deferred_action, deferred_eoc>;
        std::queue<deferred_op> pending;

        int disclaimer_idx = -1;

        // Cross-tab visibility flag (see {get,set}_eoc_trace_visible).
        bool trace_show_eoc = true;

        // While set, draw() skips the window so it isn't drawn over a blocking
        // sub-UI launched from execute().
        bool suspend_draw_ = false;

        // Eval-loop transit buffers (see request_eval / consume_eval_result).
        bool eval_pending = false;
        std::string pending_eval_expr;
        std::string pending_eval_result;
        bool pending_eval_ok = true;
        bool pending_eval_result_ready = false;

        // Owns the step / play / fast-forward state machine plus the footer
        // controls that drive it. Hidden behind pimpl so the host header does
        // not pull in <chrono> or capture/turn-loop internals.
        pimpl<step_controller> stepper_;

        // Owns the fuzzy-search box: query, hit cache, post-select highlight
        // pulse, and the jump-only follow-up. Hidden behind pimpl so the host
        // header has no visibility into the debug_action_entry table.
        pimpl<omnibox_state> omni_;

        // Window bounds tracked per-frame from ImGui's window pos/size;
        // w/h are stored as viewport fractions so the layout scales when
        // the game window is resized between sessions, while x/y stay
        // absolute.
        float persisted_x = -1.f;
        float persisted_y = -1.f;
        float persisted_w = 0.8f;
        float persisted_h = 0.8f;
        bool persisted_state_loaded = false;

        void load_persisted_state();
        void save_persisted_state() const;
};

} // namespace debug_menu

#endif // CATA_SRC_DEBUG_CONSOLE_H
