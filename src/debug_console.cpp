#include "debug_console.h"

// IWYU pragma: no_include "flat_set.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <deque>
#include <exception>
#include <functional>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

#include "action.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_imgui.h"
#include "cata_path.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "character.h"
#include "character_id.h"
#include "color.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug_actions_table.h"
#include "debug_capture.h"
#include "debug_console_snap.h"
#include "debug_menu.h"
#include "debug_monitor_targets.h"
#include "dialogue.h"
#include "effect.h"
#include "effect_on_condition.h"
#include "event.h"
#include "faction.h"
#include "field.h"
#include "filesystem.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "global_vars.h"
#include "input_context.h"
#include "input_enums.h"
#include "item.h"
#include "item_category.h"
#include "item_location.h"
#include "item_wakeup.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "mapdata.h"
#include "material.h"
#include "math_parser.h"
#include "math_parser_diag.h"
#include "math_parser_diag_value.h"
#include "math_parser_func.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "npc_opinion.h"
#include "output.h"
#include "path_info.h"
#include "pimpl.h"
#include "player_activity.h"
#include "point.h"
#include "proficiency.h"
#include "ret_val.h"
#include "rng.h"
#include "skill.h"
#include "stomach.h"
#include "string_formatter.h"
#include "talker.h"
#include "timed_event.h"
#include "translations.h"
#include "trap.h"
#include "try_parse_integer.h"
#include "ui_manager.h"
#include "units.h"
#include "vehicle.h"
#include "visitable.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"

namespace debug_menu
{

namespace
{
// Vehicle at the avatar's tile, or nullopt.
struct current_vehicle_ref {
    vehicle *v;
    map *m;
};
std::optional<current_vehicle_ref> current_vehicle_for_avatar()
{
    map &m = get_map();
    const optional_vpart_position vp = m.veh_at( get_avatar().pos_bub( m ) );
    vehicle *v = veh_pointer_or_null( vp );
    if( v == nullptr ) {
        return std::nullopt;
    }
    return current_vehicle_ref{ v, &m };
}

// Discrete play-speed positions in turns-per-second.
struct play_speed {
    float per_sec;
    const char *label;
};
constexpr std::array<play_speed, 5> speeds = { {
        { 0.25f, "0.25/s" },
        { 0.5f,  "0.5/s"  },
        { 1.0f,  "1/s"    },
        { 2.0f,  "2/s"    },
        { 4.0f,  "4/s"    },
    }
};

// One row in the canonical tab table. Its order defines tab-bar order,
// persistence id, Ctrl+1..0 mapping, NEXT/PREV ring, and category routing.
// id_string is the stable on-disk key for selected_tab and the per-tab
// nested object; changing it invalidates saved console state.
struct tab_descriptor {
    std::string_view id_string;
    std::function<std::unique_ptr<console_tab_view>()> factory;
    std::vector<std::string_view> categories;
};

const std::vector<tab_descriptor> &tab_registry()
{
    static const std::vector<tab_descriptor> reg = [] {
        std::vector<tab_descriptor> r;
        r.push_back( { "trace",     [] { return std::make_unique<tab_trace_view>(); },     {} } );
        r.push_back( { "player",    [] { return std::make_unique<tab_player_view>(); },    { "Player" } } );
        r.push_back( { "spawn",     [] { return std::make_unique<tab_spawn_view>(); },     { "Spawn" } } );
        r.push_back( { "map",       [] { return std::make_unique<tab_map_view>(); },       { "Map", "Overlays" } } );
        r.push_back( { "data",      [] { return std::make_unique<tab_data_view>(); },      { "Data" } } );
        r.push_back( { "vehicle",   [] { return std::make_unique<tab_vehicle_view>(); },   { "Vehicle" } } );
        r.push_back( { "game",      [] { return std::make_unique<tab_game_view>(); },      { "Game" } } );
        r.push_back( { "eoc",       [] { return std::make_unique<tab_eoc_view>(); },       {} } );
        r.push_back( { "creatures", [] { return std::make_unique<tab_creatures_view>(); }, {} } );
        r.push_back( { "items",     [] { return std::make_unique<tab_items_view>(); },     {} } );
        r.push_back( { "tiles",     [] { return std::make_unique<tab_tiles_view>(); },     {} } );
        return r;
    }();
    return reg;
}

// Linear lookup by stable id string. Returns nullopt for unknown ids.
std::optional<std::size_t> tab_idx_for_id( std::string_view id )
{
    const std::vector<tab_descriptor> &reg = tab_registry();
    for( std::size_t i = 0; i < reg.size(); i++ ) {
        if( reg[i].id_string == id ) {
            return i;
        }
    }
    return std::nullopt;
}

// Linear lookup by debug_action_entry category. Returns the index of the
// owning tab, or the "game" tab as defensive fallback for null/unknown
// input. Steady-state coverage is enforced by tab_registry_invariants.
std::size_t tab_idx_for_category( const char *cat )
{
    const std::vector<tab_descriptor> &reg = tab_registry();
    if( cat != nullptr ) {
        for( std::size_t i = 0; i < reg.size(); i++ ) {
            for( std::string_view c : reg[i].categories ) {
                if( c == cat ) {
                    return i;
                }
            }
        }
    }
    return tab_idx_for_id( "game" ).value_or( 0 );
}

// Dimmed-label + plain-value row inside an already-open 2-column table.
// Caller is responsible for ImGui::BeginTable / EndTable around batches of
// calls; this routine only advances the row and writes the two cells.
void kv_row( const char *k, const std::string &v )
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextDisabled( "%s", k );
    ImGui::TableNextColumn();
    ImGui::TextUnformatted( v.c_str() );
}

// Opens a 2-column key/value panel: a 160-px-wide dimmed label column and
// a stretching value column. Body runs only when BeginTable succeeded;
// EndTable is paired automatically.
void kv_panel( const char *id, const std::function<void()> &body )
{
    if( ImGui::BeginTable( id, 2, ImGuiTableFlags_SizingStretchProp ) ) {
        ImGui::TableSetupColumn( "k", ImGuiTableColumnFlags_WidthFixed, 160.0f );
        ImGui::TableSetupColumn( "v" );
        body();
        ImGui::EndTable();
    }
}

// Per-column sizing policy for the managed (clipped) tables. Widths are
// measured by us, not by ImGui, so a row clipper can never starve the
// measurement and column widths stay stable frame to frame.
enum class colw { fit, widget, stretch };

template<typename Row>
struct colspec {
    const char *header;
    colw policy = colw::fit;
    // fit: produces the display string measured per row.
    std::function<std::string( const Row & )> text = {};
    float px = 0.0f;          // widget: width; fit: minimum floor
    float weight = 1.0f;      // stretch: weight
    float max_frac = 0.0f;    // fit: cap as fraction of table width (0 = none)
    ImGuiTableColumnFlags flags = 0;
};

struct colw_cache {
    uint64_t key = 0;
    bool valid = false;
    std::vector<float> widths;   // natural (uncapped, unfloored) measured widths
};

std::unordered_map<std::string, colw_cache> &colw_caches()
{
    static std::unordered_map<std::string, colw_cache> m;
    return m;
}

// Natural width of one fit column over rows [first,last): header (plus a sort
// arrow allowance for sortable, non-NoSort columns) and the widest cell, padded.
template<typename Row>
float measure_fit( const colspec<Row> &s, const std::vector<Row> &rows,
                   size_t first, size_t last, bool sortable )
{
    float w = ImGui::CalcTextSize( s.header ).x;
    if( sortable && !( s.flags & ImGuiTableColumnFlags_NoSort ) ) {
        w += ImGui::GetFontSize();
    }
    if( s.text ) {
        for( size_t r = first; r < last; r++ ) {
            w = std::max( w, ImGui::CalcTextSize( s.text( rows[r] ).c_str() ).x );
        }
    }
    return w + ImGui::GetStyle().CellPadding.x * 2.0f + 6.0f;
}

// Emit TableSetupColumn for every column with an explicit width. fit columns
// cache a natural width measured against `key` (recomputed only when the
// row-set key changes, so widths do not drift or jitter on scroll), then
// grow_columns sticky-grows it for visible values. The px floor and max_frac
// cap are applied here each frame so a window resize re-caps without a rebuild.
// All managed columns are NoResize so the width passed each frame is honored
// (ImGui treats init width as init-only for resizable columns). Call right
// after BeginTable, before headers and rows.
template<typename Row>
void setup_columns( const char *cache_id, uint64_t key,
                    const std::vector<colspec<Row>> &specs,
                    const std::vector<Row> &rows, bool sortable = false )
{
    colw_cache &c = colw_caches()[cache_id];
    if( !c.valid || c.key != key ) {
        c.widths.assign( specs.size(), 0.0f );
        // Bounded initial measure; rows past the cap are picked up by
        // grow_columns once scrolled into view.
        const size_t n = std::min<size_t>( rows.size(), 4096 );
        for( size_t i = 0; i < specs.size(); i++ ) {
            if( specs[i].policy == colw::fit ) {
                c.widths[i] = measure_fit( specs[i], rows, 0, n, sortable );
            }
        }
        c.key = key;
        c.valid = true;
    }
    const float avail = ImGui::GetContentRegionAvail().x;
    for( size_t i = 0; i < specs.size(); i++ ) {
        const colspec<Row> &s = specs[i];
        const ImGuiTableColumnFlags extra = ImGuiTableColumnFlags_NoResize | s.flags;
        if( s.policy == colw::stretch ) {
            ImGui::TableSetupColumn( s.header,
                                     ImGuiTableColumnFlags_WidthStretch | extra, s.weight );
            continue;
        }
        float w = s.policy == colw::widget ? s.px : std::max( c.widths[i], s.px );
        if( s.policy == colw::fit && s.max_frac > 0.0f && avail > 0.0f ) {
            w = std::min( w, s.max_frac * avail );
        }
        ImGui::TableSetupColumn( s.header, ImGuiTableColumnFlags_WidthFixed | extra, w );
    }
}

// Sticky-grow visible fit columns. Call inside the clipper loop with the
// visible [first,last) range; cached widths only grow and apply next frame.
template<typename Row>
void grow_columns( const char *cache_id, const std::vector<colspec<Row>> &specs,
                   const std::vector<Row> &rows, int first, int last, bool sortable = false )
{
    auto it = colw_caches().find( cache_id );
    if( it == colw_caches().end() || !it->second.valid ) {
        return;
    }
    colw_cache &c = it->second;
    const size_t lo = static_cast<size_t>( std::max( 0, first ) );
    const size_t hi = std::min<size_t>( static_cast<size_t>( std::max( 0, last ) ), rows.size() );
    for( size_t i = 0; i < specs.size() && i < c.widths.size(); i++ ) {
        if( specs[i].policy == colw::fit && specs[i].text ) {
            c.widths[i] = std::max( c.widths[i], measure_fit( specs[i], rows, lo, hi, sortable ) );
        }
    }
}

// Cheap combiner for building a table layout key from row-set identity.
uint64_t lk_mix( uint64_t h, uint64_t v )
{
    return h ^ ( v + 0x9e3779b97f4a7c15ULL + ( h << 6 ) + ( h >> 2 ) );
}
uint64_t lk_str( uint64_t h, const std::string &s )
{
    return lk_mix( h, std::hash<std::string> {}( s ) );
}

// Show `full` as a hover tooltip when it is too wide for `cell_w` (the column
// content width, captured right after TableNextColumn). Call immediately after
// submitting the cell's text widget so IsItemHovered refers to it.
void clip_tooltip( const std::string &full, float cell_w )
{
    if( ImGui::IsItemHovered() && ImGui::CalcTextSize( full.c_str() ).x > cell_w ) {
        ImGui::SetTooltip( "%s", full.c_str() );
    }
}

// Column spec for export_rows. `header` is the markdown column heading;
// `json_key` is the matching JSON property name (kept independent so the
// JSON schema stays stable when display headers are tweaked). Each row
// produces one markdown cell and one already-encoded JSON value (callers
// supply quoted strings and bare numbers themselves).
template<typename Row>
struct export_column {
    const char *header;
    const char *json_key;
    std::function<std::string( const Row & )> md_cell;
    std::function<std::string( const Row & )> json_value;
};

// Drives an export_bar with a uniform markdown table + json array built
// from `rows` and `cols`. `name` is the export_bar label; `md_title` is
// the H2 line in the markdown body (pass the same string when no extra
// caption is needed). Builders run only on Copy click.
template<typename Row>
void export_rows( debug_console &host, const char *name,
                  const std::string &md_title,
                  const std::vector<Row> &rows,
                  const std::vector<export_column<Row>> &cols )
{
    host.export_bar( name,
    [&rows, &cols, &md_title]() {
        std::string md = string_format( "## %s (%d)\n\n", md_title.c_str(),
                                        static_cast<int>( rows.size() ) );
        md += "|";
        for( const export_column<Row> &c : cols ) {
            md += " ";
            md += c.header;
            md += " |";
        }
        md += "\n|";
        for( std::size_t i = 0; i < cols.size(); i++ ) {
            md += "---|";
        }
        md += "\n";
        for( const Row &r : rows ) {
            md += "|";
            for( const export_column<Row> &c : cols ) {
                md += " ";
                md += c.md_cell( r );
                md += " |";
            }
            md += "\n";
        }
        return md;
    },
    [&rows, &cols]() {
        std::string s = "[";
        bool first_row = true;
        for( const Row &r : rows ) {
            if( !first_row ) {
                s += ",";
            }
            first_row = false;
            s += "{";
            bool first_col = true;
            for( const export_column<Row> &c : cols ) {
                if( !first_col ) {
                    s += ",";
                }
                first_col = false;
                s += "\"";
                s += c.json_key;
                s += "\":";
                s += c.json_value( r );
            }
            s += "}";
        }
        s += "]";
        return s;
    } );
}

// Typed selection keys for the creatures + items tabs. Encoded as
// "npc:<id>" or "mon:<type>[@<pos>]"; to_string() and parse() round-trip
// the on-disk format byte-for-byte. For monsters, an absent "@pos"
// matches the first monster of the given type.
struct creature_ref {
    enum class kind { npc, monster };

    kind k = kind::npc;
    int npc_id = 0;                          // when k == npc
    std::string mon_type;                    // when k == monster
    std::optional<std::string> mon_pos_str;  // when k == monster; nullopt = wildcard

    std::string to_string() const {
        if( k == kind::npc ) {
            return "npc:" + std::to_string( npc_id );
        }
        if( mon_pos_str.has_value() ) {
            return "mon:" + mon_type + "@" + *mon_pos_str;
        }
        return "mon:" + mon_type;
    }

    static std::optional<creature_ref> parse( std::string_view s ) {
        if( s.rfind( "npc:", 0 ) == 0 ) {
            const std::string id_part( s.substr( 4 ) );
            try {
                return creature_ref{ kind::npc, std::stoi( id_part ), {}, std::nullopt };
            } catch( const std::exception & ) {
                return std::nullopt;
            }
        }
        if( s.rfind( "mon:", 0 ) == 0 ) {
            const std::string body( s.substr( 4 ) );
            const std::size_t at = body.find( '@' );
            creature_ref r;
            r.k = kind::monster;
            if( at == std::string::npos ) {
                r.mon_type = body;
            } else {
                r.mon_type = body.substr( 0, at );
                r.mon_pos_str = body.substr( at + 1 );
            }
            return r;
        }
        return std::nullopt;
    }
};

struct item_source_ref {
    enum class kind { avatar, tile_current, npc, monster };

    kind k = kind::avatar;
    int npc_id = 0;
    std::string mon_type;
    std::optional<std::string> mon_pos_str;

    std::string to_string() const {
        switch( k ) {
            case kind::avatar:
                return "avatar";
            case kind::tile_current:
                return "tile:current";
            case kind::npc:
                return "npc:" + std::to_string( npc_id );
            case kind::monster:
                if( mon_pos_str.has_value() ) {
                    return "mon:" + mon_type + "@" + *mon_pos_str;
                }
                return "mon:" + mon_type;
        }
        return "avatar";
    }

    static std::optional<item_source_ref> parse( std::string_view s ) {
        if( s == "avatar" ) {
            return item_source_ref{ kind::avatar, 0, {}, std::nullopt };
        }
        if( s == "tile:current" ) {
            return item_source_ref{ kind::tile_current, 0, {}, std::nullopt };
        }
        if( const std::optional<creature_ref> cr = creature_ref::parse( s );
            cr.has_value() ) {
            item_source_ref r;
            r.k = ( cr->k == creature_ref::kind::npc ) ? kind::npc : kind::monster;
            r.npc_id = cr->npc_id;
            r.mon_type = cr->mon_type;
            r.mon_pos_str = cr->mon_pos_str;
            return r;
        }
        return std::nullopt;
    }
};
} // namespace

debug_console::debug_console() : cataimgui::window( _( "Debug console" ) )
{
    // cataimgui::window::draw() calls BringWindowToDisplayFront each frame
    // when is_on_top, which sorts combo popups behind the console and
    // makes dropdowns unclickable. force_to_back lets popups render above.
    force_to_back = true;
    ctxt = input_context( "DEBUG_CONSOLE", keyboard_mode::keychar );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "WAIT" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    // Consume mouse so clicks/moves don't fall through to the game viewport.
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );

    // Tab views are instantiated from tab_registry() so the vector stays
    // index-aligned with the registry. Default to the registry's first
    // entry as the active tab.
    const std::vector<tab_descriptor> &reg = tab_registry();
    tab_views.reserve( reg.size() );
    for( const tab_descriptor &d : reg ) {
        tab_views.emplace_back( d.factory() );
    }
    // Cache the items_view pointer so cross-tab show_items_for_* verbs can
    // write through without walking the tab_views vector.
    if( auto idx = tab_idx_for_id( "items" ); idx.has_value() ) {
        items_view_ = dynamic_cast<tab_items_view *>( tab_views[*idx].get() );
    }
    selected_tab_idx_ = 0;
}

void debug_console::show_items_for_npc( character_id who )
{
    if( items_view_ != nullptr ) {
        item_source_ref r;
        r.k = item_source_ref::kind::npc;
        r.npc_id = who.get_value();
        items_view_->set_source( r.to_string() );
        request_tab_switch( "items" );
    }
}

void debug_console::show_items_for_monster( const std::string &type_id,
        const tripoint_bub_ms &pos )
{
    if( items_view_ != nullptr ) {
        item_source_ref r;
        r.k = item_source_ref::kind::monster;
        r.mon_type = type_id;
        r.mon_pos_str = pos.to_string();
        items_view_->set_source( r.to_string() );
        request_tab_switch( "items" );
    }
}

cataimgui::bounds debug_console::get_bounds()
{
    return { persisted_x, persisted_y, persisted_w, persisted_h };
}

// Step / play / fast-forward state machine. Owns the footer UI and the
// per-frame turn-drain logic; host calls tick() once per outer-loop frame
// and draw_footer() once per ImGui frame.
class step_controller
{
    public:
        enum class outcome { idle, busy, quit };

        outcome tick( input_context &ctxt );
        void request_step();
        void draw_footer();
        void load( const JsonObject &jo );
        void save( JsonOut &jo ) const;

    private:
        int pending_steps = 0;
        bool playing = false;
        int speed_idx = 2;
        std::chrono::steady_clock::time_point last_step_at;
        bool ff_stop_on_dbg_msg = false;
        bool ff_stop_on_dmg = true;
        int ff_hp_baseline = 0;
};

step_controller::outcome step_controller::tick( input_context &ctxt )
{
    if( !playing && pending_steps <= 0 ) {
        return outcome::idle;
    }
    using clock = std::chrono::steady_clock;
    const int sz = static_cast<int>( speeds.size() );
    const float speed = speeds[std::clamp( speed_idx, 0, sz - 1 )].per_sec;
    const clock::duration period = std::chrono::duration_cast<clock::duration>(
                                       std::chrono::duration<float>( 1.0f / speed ) );
    const clock::time_point now = clock::now();
    if( now - last_step_at < period ) {
        ctxt.handle_input( 5 );
        return outcome::busy;
    }
    last_step_at = now;
    if( !playing ) {
        pending_steps--;
    }
    // do_turn() blocks for player input when the avatar has moves and no
    // activity; drain moves to skip that. With an activity queued, let it
    // consume them normally.
    avatar &av = get_avatar();
    if( !av.activity ) {
        av.set_moves( 0 );
    }
    if( g->do_turn() ) {
        return outcome::quit;
    }
    if( ff_stop_on_dbg_msg && debug_capture::is_initialized()
        && !debug_capture::instance().logs().empty()
        && debug_capture::instance().logs().back().turn_num
        == to_turns<int>( calendar::turn - calendar::turn_zero ) ) {
        pending_steps = 0;
        playing = false;
    }
    if( ff_stop_on_dmg && get_avatar().get_hp() < ff_hp_baseline ) {
        pending_steps = 0;
        playing = false;
    }
    return outcome::busy;
}

void step_controller::request_step()
{
    ff_hp_baseline = get_avatar().get_hp();
    pending_steps = 1;
    // Sentinel: force the next tick() to fire immediately regardless of
    // the speed throttle.
    last_step_at = {};
}

void step_controller::load( const JsonObject &jo )
{
    jo.read( "speed_idx", speed_idx );
    jo.read( "ff_stop_on_dbg_msg", ff_stop_on_dbg_msg );
    jo.read( "ff_stop_on_dmg", ff_stop_on_dmg );
}

void step_controller::save( JsonOut &jo ) const
{
    jo.member( "speed_idx", speed_idx );
    jo.member( "ff_stop_on_dbg_msg", ff_stop_on_dbg_msg );
    jo.member( "ff_stop_on_dmg", ff_stop_on_dmg );
}

// Fuzzy-search omnibox state. Owns the input/cache plus the post-select
// highlight pulse fed back to debug_button / highlight_checkbox.
class omnibox_state
{
    public:
        void draw( debug_console &host );
        void request_focus();
        bool is_highlight( debug_menu_index action ) const;
        bool take_focus( debug_menu_index action );
        void load( const JsonObject &jo );
        void save( JsonOut &jo ) const;

    private:
        std::string query;
        // '/' poll sets; draw consumes via SetKeyboardFocusHere.
        bool focus_request = false;

        // Query-result cache.
        std::string cached_query;
        std::vector<std::pair<int, const debug_action_entry *>> cached_hits;
        bool activate_on_select = true;
        debug_menu_index highlight_action = debug_menu_index::last;
        int highlight_until_frame = 0;
        debug_menu_index focus_action = debug_menu_index::last;
        std::string jump_only_label;
};

void omnibox_state::request_focus()
{
    focus_request = true;
}

bool omnibox_state::is_highlight( debug_menu_index action ) const
{
    return action == highlight_action
           && ImGui::GetFrameCount() < highlight_until_frame;
}

bool omnibox_state::take_focus( debug_menu_index action )
{
    if( action != focus_action ) {
        return false;
    }
    focus_action = debug_menu_index::last;
    return true;
}

void omnibox_state::load( const JsonObject &jo )
{
    jo.read( "omnibox_activate_on_select", activate_on_select );
}

void omnibox_state::save( JsonOut &jo ) const
{
    jo.member( "omnibox_activate_on_select", activate_on_select );
}

void debug_console::execute()
{
    if( !persisted_state_loaded ) {
        load_persisted_state();
        persisted_state_loaded = true;
    }

    on_out_of_scope flush_guard{ [this]()
    {
        if( debug_capture::is_initialized() ) {
            debug_capture::instance().flush_trace_file();
        }
        save_persisted_state();
    } };

    // Suspend ImGui keyboard nav while the console is open so TAB drives only
    // the input_context tab switch instead of also cycling widget focus.
    ImGuiIO &io = ImGui::GetIO();
    const bool had_kbd_nav = io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
    on_out_of_scope restore_kbd_nav{ [&io, had_kbd_nav]()
    {
        if( had_kbd_nav ) {
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        }
    } };

    while( is_open ) {
        // Force a full viewport repaint each frame: otherwise ImGui tooltips
        // smear the tiles layer and play/step state never updates live.
        g->invalidate_main_ui_adaptor();
        ui_manager::redraw();

        // Drain one deferred op per frame (must run outside ImGui frame).
        // FIFO interleaves actions and EOC fires by enqueue order.
        if( !pending.empty() ) {
            deferred_op op = pending.front();
            pending.pop();
            // Action/EOC may open a blocking sub-UI; hide the console behind it.
            suspend_draw_ = true;
            on_out_of_scope restore_draw{ [this]()
            {
                suspend_draw_ = false;
            } };
            std::visit( [&]( auto &&payload ) {
                using T = std::decay_t<decltype( payload )>;
                if constexpr( std::is_same_v<T, deferred_action> ) {
                    execute_action( payload.id );
                } else if constexpr( std::is_same_v<T, deferred_eoc> ) {
                    for( const effect_on_condition &eoc :
                         effect_on_conditions::get_all() ) {
                        if( eoc.id == payload.id ) {
                            dialogue d( get_talker_for( get_avatar() ), nullptr );
                            eoc.activate( d );
                            break;
                        }
                    }
                }
            }, op );
            continue;
        }

        if( eval_pending ) {
            eval_pending = false;
            pending_eval_ok = true;
            // eval can raise a debugmsg popup; hide the console behind it.
            suspend_draw_ = true;
            on_out_of_scope restore_draw{ [this]()
            {
                suspend_draw_ = false;
            } };
            try {
                math_exp m;
                if( m.parse( pending_eval_expr, false ) ) {
                    dialogue d( get_talker_for( get_avatar() ), nullptr );
                    const double v = m.eval( d );
                    pending_eval_result = string_format( "%g", v );
                } else {
                    pending_eval_result = _( "parse error" );
                    pending_eval_ok = false;
                }
            } catch( const std::exception &e ) {
                // math_exp::error() frames the message with a leading newline
                // plus a caret diagram on later lines; trim the outer blank
                // lines but keep the body so the error stays visible.
                std::string what = e.what();
                const size_t start = what.find_first_not_of( '\n' );
                const size_t end = what.find_last_not_of( '\n' );
                if( start == std::string::npos ) {
                    what.clear();
                } else {
                    what = what.substr( start, end - start + 1 );
                }
                pending_eval_result = std::move( what );
                pending_eval_ok = false;
            }
            pending_eval_result_ready = true;
            continue;
        }

        switch( stepper_->tick( ctxt ) ) {
            case step_controller::outcome::quit:
                return;
            case step_controller::outcome::busy:
                continue;
            case step_controller::outcome::idle:
                break;
        }

        std::string action;
        if( has_button_action() ) {
            action = get_button_action();
        } else {
            action = ctxt.handle_input( 5 );
        }

        // Only swallow keybinds while typing / dragging / in a popup, so plain
        // TAB still reaches the input_context tab switch.
        const bool imgui_owns_keys = io.WantTextInput || ImGui::IsAnyItemActive() ||
                                     ImGui::IsPopupOpen( nullptr,
                                             ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel );
        if( imgui_owns_keys ) {
            continue;
        }

        if( action == "QUIT" ) {
            break;
        } else if( action == "WAIT" ) {
            // Route through pending_steps so '.' and the Step button share
            // one path; avoids racing with the polled step handler.
            stepper_->request_step();
        } else if( action == "NEXT_TAB" ) {
            const std::size_t n = tab_views.size();
            if( n > 0 ) {
                switch_tab_idx_ = ( selected_tab_idx_ + 1 ) % n;
            }
        } else if( action == "PREV_TAB" ) {
            const std::size_t n = tab_views.size();
            if( n > 0 ) {
                switch_tab_idx_ = ( selected_tab_idx_ + n - 1 ) % n;
            }
        }
    }
}

void debug_console::defer_action( debug_menu_index action )
{
    // Bound the queue: an unbounded depth lets rapid clicks pile up arbitrary
    // work that the user can't see or cancel. 256 leaves room for legitimate
    // batches (e.g. "click 50 spawn buttons") without runaway growth.
    if( pending.size() >= 256 ) {
        return;
    }
    pending.emplace( deferred_action{ action } );
}

void debug_console::defer_eoc( effect_on_condition_id eoc_id )
{
    if( pending.size() >= 256 ) {
        return;
    }
    pending.emplace( deferred_eoc{ eoc_id } );
}

void debug_console::request_tab_switch( std::string_view tab_id )
{
    if( auto idx = tab_idx_for_id( tab_id ); idx.has_value() ) {
        switch_tab_idx_ = idx;
    }
}

void debug_console::request_eval( const std::string &expr )
{
    pending_eval_expr = expr;
    eval_pending = true;
}

std::optional<debug_console::eval_result_view> debug_console::consume_eval_result()
{
    if( !pending_eval_result_ready ) {
        return std::nullopt;
    }
    eval_result_view rv{ std::move( pending_eval_result ), pending_eval_ok };
    pending_eval_result.clear();
    pending_eval_result_ready = false;
    return rv;
}

void debug_console::set_eoc_trace_visible( bool v )
{
    trace_show_eoc = v;
}

bool debug_console::eoc_trace_visible() const
{
    return trace_show_eoc;
}

void debug_console::draw_tab_views()
{
    // tab_views is built index-aligned with tab_registry() so iteration here
    // gives the canonical order without a second lookup pass.
    for( std::size_t i = 0; i < tab_views.size(); i++ ) {
        console_tab_view &view = *tab_views[i];
        const bool sel = switch_tab_idx_.has_value() && *switch_tab_idx_ == i;
        if( sel ) {
            switch_tab_idx_.reset();
        }
        if( cataimgui::BeginTabItem( view.label(), sel ) ) {
            selected_tab_idx_ = i;
            view.draw_body( *this );
            ImGui::EndTabItem();
        }
    }
}

void debug_console::draw()
{
    // Skip the whole base draw() (not just draw_controls) so ImGui::Begin is
    // never called and the window isn't drawn over a blocking sub-UI.
    if( suspend_draw_ ) {
        button_action.clear();
        return;
    }
    cataimgui::window::draw();
}

void debug_console::draw_controls()
{
    // Ctrl+1..9 / Ctrl+0 jump directly to a tab. ImGui input must be queried
    // inside a frame, so check here rather than in execute()'s outer loop.
    if( ImGui::GetIO().KeyCtrl ) {
        static constexpr ImGuiKey hotkeys[] = {
            ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4, ImGuiKey_5,
            ImGuiKey_6, ImGuiKey_7, ImGuiKey_8, ImGuiKey_9, ImGuiKey_0,
        };
        const std::size_t n_keys = sizeof( hotkeys ) / sizeof( hotkeys[0] );
        const std::size_t n = std::min( n_keys, tab_views.size() );
        for( std::size_t i = 0; i < n; i++ ) {
            if( ImGui::IsKeyPressed( hotkeys[i], false ) ) {
                switch_tab_idx_ = i;
                break;
            }
        }
    }

    // '/' focuses the omnibox; '.' steps one turn. Both via ImGui so
    // they work without user keybinds; guarded against text/slider/
    // popup focus. WAIT input_context path also handled in execute()
    // for users who have it bound; both set pending_steps idempotently.
    const bool ui_busy = ImGui::GetIO().WantTextInput
                         || ImGui::IsAnyItemActive();
    if( !ui_busy ) {
        if( ImGui::IsKeyPressed( ImGuiKey_Slash, false ) ) {
            omni_->request_focus();
        }
        if( ImGui::IsKeyPressed( ImGuiKey_Period, false ) ) {
            stepper_->request_step();
        }
    }

    // Reserve vertical space for shared footer (two rows + separator).
    // Zero when debug_mode is off (footer hidden).
    const float footer_h = debug_mode
                           ? ImGui::GetFrameHeightWithSpacing() * 2.0f
                           + ImGui::GetStyle().ItemSpacing.y
                           + ImGui::GetStyle().FramePadding.y * 2.0f
                           : 0.0f;

    ImGui::BeginChild( "console_body", ImVec2( 0, -footer_h ), 0 );
    if( !debug_mode ) {
        // Picked once per off-state visit so it doesn't reroll every frame.
        // Reset to -1 by the Enable button so the next off-visit rerolls.
        static const char *const flavors[] = {
            translate_marker( "Turn back.  This door was sealed for cause." ),
            translate_marker( "Beyond this threshold lies neither help nor "
                              "heroism, only the marks thou leavest on a world "
                              "that was working." ),
            translate_marker( "Here thy save is mortal.  Here thy hours may "
                              "rot.  Here be no kindness." ),
            translate_marker( "Pass only if thou hast no further use for this "
                              "character." ),
            translate_marker( "Open this door and the game shall know thee for "
                              "what thou art." ),
            translate_marker( "Press on if thou must, but no bard shall sing "
                              "of what follows." ),
            translate_marker( "Down this hall lie corrupted saves, halted "
                              "turns, and silences where there should be sound." ),
            translate_marker( "Few who tread here remember why they came.  "
                              "Fewer still remember to back up first." ),
        };
        constexpr int n_flavors = static_cast<int>( sizeof( flavors ) /
                                  sizeof( flavors[0] ) );
        if( disclaimer_idx < 0 || disclaimer_idx >= n_flavors ) {
            disclaimer_idx = rng( 0, n_flavors - 1 );
        }

        const ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::Dummy( ImVec2( 0.0f, avail.y * 0.30f ) );
        const char *msg = _( "Debug mode is off." );
        const ImVec2 sz = ImGui::CalcTextSize( msg );
        ImGui::SetCursorPosX( ( avail.x - sz.x ) * 0.5f );
        ImGui::TextColored( ImVec4( 0.95f, 0.7f, 0.4f, 1.0f ), "%s", msg );

        ImGui::Dummy( ImVec2( 0.0f, ImGui::GetStyle().ItemSpacing.y ) );
        const std::string flavor = _( flavors[disclaimer_idx] );
        const float wrap_w = avail.x * 0.7f;
        // CalcTextSize with the wrap width returns the wrapped block size;
        // using that to position centers the block as a whole.
        const ImVec2 flavor_sz = ImGui::CalcTextSize( flavor.c_str(), nullptr,
                                 false, wrap_w );
        ImGui::SetCursorPosX( ( avail.x - flavor_sz.x ) * 0.5f );
        ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + flavor_sz.x );
        ImGui::TextDisabled( "%s", flavor.c_str() );
        ImGui::PopTextWrapPos();

        ImGui::Dummy( ImVec2( 0.0f, ImGui::GetStyle().ItemSpacing.y * 2.0f ) );
        const char *btn = _( "Enable debug mode" );
        const ImVec2 btn_sz = ImGui::CalcTextSize( btn );
        const float btn_w = btn_sz.x + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SetCursorPosX( ( avail.x - btn_w ) * 0.5f );
        if( ImGui::Button( btn, ImVec2( btn_w, 0.0f ) ) ) {
            debug_mode = true;
            disclaimer_idx = -1;
        }
    } else {
        // Right-aligned debug-mode toggle, kept away from the omnibox.
        omni_->draw( *this );
        ImGui::SameLine();
        const char *off_btn = _( "Debug: ON" );
        const float bw = ImGui::CalcTextSize( off_btn ).x
                         + ImGui::GetStyle().FramePadding.x * 4.0f;
        ImGui::SameLine( ImGui::GetContentRegionAvail().x
                         + ImGui::GetCursorPosX() - bw );
        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.25f, 0.55f, 0.25f, 1.0f ) );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.30f, 0.65f, 0.30f, 1.0f ) );
        if( ImGui::SmallButton( off_btn ) ) {
            debug_mode = false;
        }
        ImGui::PopStyleColor( 2 );
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s", _( "Turn debug mode off" ) );
        }
        if( ImGui::BeginTabBar( "debug_console_tabs" ) ) {
            draw_tab_views();
            ImGui::EndTabBar();
        }
    }
    ImGui::EndChild();

    // Shared footer: step / fast-forward / pause indicator. Anchored at
    // window bottom; hidden when debug_mode is off.
    if( debug_mode ) {
        ImGui::Separator();
        stepper_->draw_footer();
    }

    // Capture pos/size each frame for save_persisted_state. cata_imgui's
    // Cond_Always SetNextWindowPos defeats ImGui's own .ini round-trip,
    // so track here. Size normalized to viewport fraction.
    const ImVec2 wpos = ImGui::GetWindowPos();
    const ImVec2 wsz = ImGui::GetWindowSize();
    const ImVec2 vp = ImGui::GetMainViewport()->Size;
    persisted_x = wpos.x;
    persisted_y = wpos.y;
    if( vp.x > 0 && vp.y > 0 ) {
        persisted_w = wsz.x / vp.x;
        persisted_h = wsz.y / vp.y;
    }
}

// Shared transport row: Step / Play-Pause / speed slider, plus
// safety stops on a second row.
void step_controller::draw_footer()
{
    avatar &you = get_avatar();
    speed_idx = std::clamp( speed_idx, 0, static_cast<int>( speeds.size() ) - 1 );

    if( ImGui::Button( _( "Step" ) ) ) {
        ff_hp_baseline = you.get_hp();
        pending_steps = 1;
        // Force immediate tick on next loop iteration regardless of slider.
        last_step_at = {};
    }
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Advance one turn at current speed" ) );
    }
    ImGui::SameLine();
    const char *play_label = playing ? _( "Pause" ) : _( "Play" );
    if( ImGui::Button( play_label ) ) {
        playing = !playing;
        if( playing ) {
            ff_hp_baseline = you.get_hp();
            last_step_at = {};
        }
    }
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Run turns continuously at current speed" ) );
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth( 220.0f );
    ImGui::SliderInt( "##speed", &speed_idx, 0,
                      static_cast<int>( speeds.size() ) - 1,
                      speeds[speed_idx].label );
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Turns per real second for Play and Step" ) );
    }

    ImGui::SameLine();
    if( playing ) {
        ImGui::TextColored( ImVec4( 1.0f, 0.7f, 0.4f, 1.0f ), "%s", _( "PLAYING" ) );
    } else if( pending_steps > 0 ) {
        ImGui::TextDisabled( "%d", pending_steps );
    } else {
        ImGui::TextColored( ImVec4( 0.6f, 0.9f, 0.6f, 1.0f ), "%s", _( "PAUSED" ) );
    }

    ImGui::Checkbox( _( "Stop on new log entry" ), &ff_stop_on_dbg_msg );
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Halts Play/Step whenever a captured trace line lands this turn" ) );
    }
    ImGui::SameLine();
    ImGui::Checkbox( _( "Stop on player damage" ), &ff_stop_on_dmg );
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Stop Play/Step when player HP drops" ) );
    }
}

void omnibox_state::draw( debug_console &host )
{
    ImGui::SetNextItemWidth( 360.0f );
    if( focus_request ) {
        ImGui::SetKeyboardFocusHere();
        focus_request = false;
    }
    ImGui::InputTextWithHint( "##omnibox", _( "Search actions" ), &query );
    ImGui::SameLine();
    if( ImGui::SmallButton( _( "Clear##omnibox_clear" ) ) ) {
        query.clear();
    }
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s", _( "Clear the search query" ) );
    }
    ImGui::SameLine();
    ImGui::Checkbox( _( "Activate" ), &activate_on_select );
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "On: click runs action.  Off: jumps to its tab" ) );
    }
    if( query.empty() ) {
        cached_query.clear();
        cached_hits.clear();
        return;
    }
    // Recompute fuzzy scores only when the query string changes.
    if( query != cached_query ) {
        cached_hits.clear();
        for( const debug_action_entry &e : all_actions() ) {
            std::string hay = std::string( e.label ) + " " + e.search_keys + " " + e.category;
            int s = fuzzy_score( hay, query );
            if( s > 0 ) {
                cached_hits.emplace_back( s, &e );
            }
        }
        std::sort( cached_hits.begin(), cached_hits.end(),
        []( const auto & a, const auto & b ) {
            return a.first > b.first;
        } );
        cached_query = query;
    }
    const int total_hits = static_cast<int>( cached_hits.size() );
    int limit = total_hits < 10 ? total_hits : 10;
    for( int i = 0; i < limit; i++ ) {
        const int score = cached_hits[i].first;
        const debug_action_entry &e = *cached_hits[i].second;
        std::string label = std::string( _( e.label ) ) + "##om" + std::to_string( i );
        if( ImGui::Selectable( label.c_str() ) ) {
            if( activate_on_select ) {
                host.defer_action( e.id );
            } else {
                host.request_tab_switch(
                    tab_registry()[tab_idx_for_category( e.category )].id_string );
                highlight_action = e.id;
                focus_action = e.id;
                highlight_until_frame = ImGui::GetFrameCount() + 180;
                // jump_only actions immediately open a uilist and have no
                // inline control to pulse. Surface that explicitly so the
                // user isn't waiting for a highlight that will never appear.
                if( e.jump_only ) {
                    jump_only_label = _( e.label );
                } else {
                    jump_only_label.clear();
                }
            }
            query.clear();
            cached_query.clear();
            cached_hits.clear();
        }
        ImGui::SameLine();
        ImGui::TextDisabled( "[%s] %d", e.category, score );
    }
    if( total_hits > limit ) {
        ImGui::TextDisabled( _( "(%d more matches)" ), total_hits - limit );
    }
    // jump_only follow-up message stays visible for the same window as the
    // highlight pulse. After the window expires, clear it.
    if( !jump_only_label.empty() ) {
        if( ImGui::GetFrameCount() < highlight_until_frame ) {
            ImGui::TextColored( ImVec4( 0.95f, 0.85f, 0.45f, 1.0f ), "%s",
                                string_format(
                                    _( "Jumped to tab; \"%s\" opens a dialog and has no inline control to highlight." ),
                                    jump_only_label ).c_str() );
        } else {
            jump_only_label.clear();
        }
    }
    ImGui::Separator();
}

bool debug_console::is_omnibox_highlight( debug_menu_index action ) const
{
    return omni_->is_highlight( action );
}

bool debug_console::take_omnibox_focus( debug_menu_index action )
{
    return omni_->take_focus( action );
}

static void draw_action_tooltip( const debug_action_entry &e )
{
    ImGui::BeginTooltip();
    if( e.tooltip != nullptr && e.tooltip[0] != '\0' ) {
        ImGui::TextUnformatted( _( e.tooltip ) );
    } else {
        ImGui::TextColored( ImVec4( 1.0f, 0.85f, 0.35f, 1.0f ), "%s",
                            _( e.label ) );
        ImGui::TextColored( ImVec4( 0.7f, 0.7f, 1.0f, 1.0f ),
                            _( "Category: %s" ), e.category );
    }
    ImGui::EndTooltip();
}

bool debug_console::debug_button( debug_menu_index action )
{
    const debug_action_entry *e = find_action( action );
    const char *label = e != nullptr ? _( e->label ) : "?";
    return debug_button_with_label( label, action );
}

bool debug_console::debug_button_with_label( const char *label,
        debug_menu_index action )
{
    if( take_omnibox_focus( action ) ) {
        ImGui::SetKeyboardFocusHere();
    }
    const bool clicked = ImGui::Button( label );
    const debug_action_entry *e = find_action( action );
    if( e && ( ImGui::IsItemHovered() || ImGui::IsItemFocused() ) ) {
        draw_action_tooltip( *e );
    }
    if( clicked ) {
        defer_action( action );
        return true;
    }
    return false;
}

void debug_console::confirm_button( const char *label, const char *popup_id,
                                    const char *prompt, debug_menu_index action )
{
    if( take_omnibox_focus( action ) ) {
        ImGui::SetKeyboardFocusHere();
    }
    const bool open = ImGui::Button( label );
    if( const debug_action_entry *e = find_action( action ) ) {
        if( ImGui::IsItemHovered() || ImGui::IsItemFocused() ) {
            draw_action_tooltip( *e );
        }
    }
    if( open ) {
        ImGui::OpenPopup( popup_id );
    }
    if( ImGui::BeginPopupModal( popup_id, nullptr, ImGuiWindowFlags_AlwaysAutoResize ) ) {
        ImGui::TextUnformatted( prompt );
        if( ImGui::Button( _( "Confirm" ) ) ) {
            defer_action( action );
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if( ImGui::Button( _( "Cancel" ) ) ) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}








bool debug_console::highlight_checkbox( const char *label, bool *value,
                                        debug_menu_index pulse_target )
{
    if( pulse_target != debug_menu_index::last
        && take_omnibox_focus( pulse_target ) ) {
        ImGui::SetKeyboardFocusHere();
    }
    return ImGui::Checkbox( label, value );
}

void debug_console::export_bar( const std::string &name,
                                const std::function<std::string()> &build_md,
                                const std::function<std::string()> &build_json )
{
    ImGui::PushID( name.c_str() );
    if( ImGui::SmallButton( _( "Copy MD" ) ) ) {
        const std::string md = build_md();
        ImGui::SetClipboardText( md.c_str() );
    }
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Copy table as Markdown" ) );
    }
    ImGui::SameLine();
    if( ImGui::SmallButton( _( "Copy JSON" ) ) ) {
        const std::string js = build_json();
        ImGui::SetClipboardText( js.c_str() );
    }
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Copy table as JSON" ) );
    }
    ImGui::SameLine();
    ImGui::TextDisabled( "%s", name.c_str() );
    ImGui::PopID();
}




static cata_path persist_state_path()
{
    return PATH_INFO::config_dir_path() / "debug_console.json";
}

static cata_path persist_imgui_path()
{
    return PATH_INFO::config_dir_path() / "debug_console.imgui.ini";
}

void debug_console::load_persisted_state()
{
    // ImGui ini blob applied first: must precede any window creation, else
    // cata_imgui's first SetNextWindowPos fights the restored entries.
    const cata_path ini_path = persist_imgui_path();
    if( file_exist( ini_path ) ) {
        const std::string blob = read_entire_file( ini_path.get_unrelative_path() );
        if( !blob.empty() ) {
            ImGui::LoadIniSettingsFromMemory( blob.c_str(), blob.size() );
        }
    }

    read_from_file_optional_json( persist_state_path(), [&]( const JsonValue & jv ) {
        const JsonObject jo = jv.get_object();
        jo.allow_omitted_members();

        // Window bounds
        jo.read( "win_x", persisted_x );
        jo.read( "win_y", persisted_y );
        jo.read( "win_w", persisted_w );
        jo.read( "win_h", persisted_h );

        // Selected tab. Persisted as the registry's id_string so saved state
        // survives reordering or removal of unrelated tabs. Unknown ids fall
        // through to the default (index 0).
        std::string tab_str;
        if( jo.read( "selected_tab", tab_str ) ) {
            std::optional<std::size_t> idx = tab_idx_for_id( tab_str );
            if( idx.has_value() ) {
                selected_tab_idx_ = *idx;
                switch_tab_idx_ = idx;
            }
        }

        // Cross-tab toggle: trace tab reads, EOC tab writes via the
        // visibility verb pair.
        jo.read( "trace_show_eoc", trace_show_eoc );
        omni_->load( jo );

        // Per-tab nested state.
        JsonObject tabs_obj = jo.has_object( "tabs" )
                              ? jo.get_object( "tabs" )
                              : JsonObject();
        tabs_obj.allow_omitted_members();
        const std::vector<tab_descriptor> &reg = tab_registry();
        for( std::size_t i = 0; i < tab_views.size(); i++ ) {
            const std::string key( reg[i].id_string );
            JsonObject per_tab = tabs_obj.has_object( key )
                                 ? tabs_obj.get_object( key )
                                 : JsonObject();
            per_tab.allow_omitted_members();
            tab_views[i]->load_state( per_tab );
        }

        stepper_->load( jo );

        if( debug_capture::is_initialized() && jo.has_object( "capture" ) ) {
            JsonObject co = jo.get_object( "capture" );
            co.allow_omitted_members();
            debug_capture::instance().load_settings( co );
        }
    } );
}

void debug_console::save_persisted_state() const
{
    assure_dir_exist( PATH_INFO::config_dir() );

    write_to_file( persist_state_path(), [&]( std::ostream & fout ) {
        JsonOut jo( fout, true );
        jo.start_object();

        jo.member( "win_x", persisted_x );
        jo.member( "win_y", persisted_y );
        jo.member( "win_w", persisted_w );
        jo.member( "win_h", persisted_h );

        jo.member( "selected_tab",
                   std::string( tab_registry()[selected_tab_idx_].id_string ) );

        // Cross-tab visibility flag. Trace/eval state is written under
        // "tabs":{...} by per-tab save_state.
        jo.member( "trace_show_eoc", trace_show_eoc );
        omni_->save( jo );

        jo.member( "tabs" );
        jo.start_object();
        const std::vector<tab_descriptor> &reg = tab_registry();
        for( std::size_t i = 0; i < tab_views.size(); i++ ) {
            jo.member( std::string( reg[i].id_string ) );
            jo.start_object();
            tab_views[i]->save_state( jo );
            jo.end_object();
        }
        jo.end_object();

        stepper_->save( jo );

        if( debug_capture::is_initialized() ) {
            jo.member( "capture" );
            jo.start_object();
            debug_capture::instance().save_settings( jo );
            jo.end_object();
        }

        jo.end_object();
    }, _( "debug console state" ) );

    // ImGui ini blob: column widths/sort, splitter sizes, popup positions,
    // per-window pos/size for non-console surfaces. Plain text.
    size_t ini_size = 0;
    const char *ini_data = ImGui::SaveIniSettingsToMemory( &ini_size );
    if( ini_data != nullptr && ini_size > 0 ) {
        write_to_file( persist_imgui_path(), [&]( std::ostream & fout ) {
            fout.write( ini_data, ini_size );
        }, _( "debug console imgui state" ) );
    }
}

void open_console()
{
    debug_console console;
    console.execute();
}


const char *tab_spawn_view::label() const
{
    return _( "Spawn" );
}

void tab_spawn_view::draw_body( debug_console &host )
{
    framed_section( "s_items", _( "Items" ), [&]() {
        host.debug_button( debug_menu_index::WISH );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::SPAWN_ITEM_GROUP );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::SPAWN_ARTIFACT );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::SPAWN_CLAIRVOYANCE );
    } );

    framed_section( "s_creatures", _( "Creatures" ), [&]() {
        host.debug_button( debug_menu_index::SPAWN_MON );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::SPAWN_HORDE );

        host.debug_button( debug_menu_index::SPAWN_NPC );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::SPAWN_NPC_FOLLOWER );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::SPAWN_NAMED_NPC );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::SPAWN_OM_NPC );
    } );

    framed_section( "s_vehicles", _( "Vehicles" ), [&]() {
        host.debug_button( debug_menu_index::SPAWN_VEHICLE );
    } );
}


const char *tab_game_view::label() const
{
    return _( "Game" );
}

void tab_game_view::draw_body( debug_console &host )
{
    framed_section( "g_unlock", _( "Unlock" ), [&]() {
        host.debug_button( debug_menu_index::ENABLE_ACHIEVEMENTS );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::UNLOCK_ALL );
    } );

    framed_section( "g_quick", _( "Quick setup" ), [&]() {
        host.debug_button( debug_menu_index::QUICK_SETUP );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::QUICK_SETUP_FLAG_DIRTY );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::TOGGLE_SETUP_MUTATION );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::NORMALIZE_BODY_STAT );
    } );

    framed_section( "g_save", _( "Save / load" ), [&]() {
        host.debug_button( debug_menu_index::SAVE_SCREENSHOT );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::GAME_REPORT );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::GAME_MIN_ARCHIVE );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::QUICKLOAD );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::QUIT_NOSAVE );
    } );

    framed_section( "g_debug", _( "Debug" ), [&]() {
        host.debug_button( debug_menu_index::SHOW_MSG );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::CRASH_GAME );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::TEST_END_SCREEN );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::IMGUI_DEMO );
    } );
}


const char *tab_map_view::label() const
{
    return _( "Map" );
}

void tab_map_view::draw_body( debug_console &host )
{
    framed_section( "m_tp", _( "Teleport" ), [&]() {
        host.debug_button( debug_menu_index::SHORT_TELEPORT );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::LONG_TELEPORT );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::OM_TELEPORT );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::OM_TELEPORT_COORDINATES );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::OM_TELEPORT_CITY );
    } );

    framed_section( "m_edit", _( "Editing" ), [&]() {
        host.debug_button( debug_menu_index::MAP_EDITOR );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::OM_EDITOR );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::PALETTE_VIEWER );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::MAP_EXTRA );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::NESTED_MAPGEN );
    } );

    framed_section( "m_env", _( "Environment" ), [&]() {
        host.debug_button( debug_menu_index::CHANGE_WEATHER );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::CHANGE_TIME );

        {
            weather_manager &weather = get_weather();
            struct wind_dir {
                const char *label;
                int deg;
            };
            static constexpr std::array<wind_dir, 8> dirs = { {
                    { "N",  0   }, { "NE", 45  }, { "E",  90  }, { "SE", 135 },
                    { "S",  180 }, { "SW", 225 }, { "W",  270 }, { "NW", 315 },
                }
            };
            int cur_dir = weather.wind_direction_override.value_or( -1 );
            int combo_idx = 8; // "Auto" by default
            for( int i = 0; i < 8; i++ ) {
                if( cur_dir == dirs[i].deg ) {
                    combo_idx = i;
                    break;
                }
            }
            ImGui::SetNextItemWidth( 80 );
            if( ImGui::BeginCombo( _( "Wind dir" ),
                                   combo_idx < 8 ? dirs[combo_idx].label : _( "Auto" ) ) ) {
                if( ImGui::Selectable( _( "Auto" ), combo_idx == 8 ) ) {
                    weather.wind_direction_override.reset();
                    weather.set_nextweather( calendar::turn );
                }
                for( int i = 0; i < 8; i++ ) {
                    if( ImGui::Selectable( dirs[i].label, combo_idx == i ) ) {
                        weather.wind_direction_override = dirs[i].deg;
                        weather.set_nextweather( calendar::turn );
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();

            int wspeed = weather.windspeed_override.value_or( weather.windspeed );
            bool has_override = weather.windspeed_override.has_value();
            ImGui::SetNextItemWidth( 120 );
            if( ImGui::SliderInt( _( "Wind spd" ), &wspeed, 0, 100 ) ) {
                weather.windspeed_override = wspeed;
                weather.set_nextweather( calendar::turn );
            }
            if( has_override ) {
                ImGui::SameLine();
                if( ImGui::SmallButton( _( "Reset##wind" ) ) ) {
                    weather.windspeed_override.reset();
                    weather.set_nextweather( calendar::turn );
                }
                if( ImGui::IsItemHovered() ) {
                    ImGui::SetTooltip( "%s",
                                       _( "Restore generator-driven wind speed" ) );
                }
            }
        }

        {
            weather_manager &weather = get_weather();
            bool has_forced = weather.forced_temperature.has_value();
            if( ImGui::Checkbox( _( "Force temp" ), &has_forced ) ) {
                if( !has_forced ) {
                    weather.forced_temperature.reset();
                } else {
                    weather.forced_temperature = units::from_celsius( 20 );
                }
            }
            if( weather.forced_temperature.has_value() ) {
                ImGui::SameLine();
                int temp_c = static_cast<int>( units::to_celsius( *weather.forced_temperature ) );
                ImGui::SetNextItemWidth( 80 );
                if( ImGui::InputInt( _( "C##temp" ), &temp_c ) ) {
                    weather.forced_temperature = units::from_celsius( temp_c );
                }
            }
        }
    } );

    framed_section( "m_actions", _( "Actions" ), [&]() {
        host.confirm_button( _( "Kill in area" ), "confirm_kill_area",
                             _( "Kill every creature in selected area?  Cannot undo." ),
                             debug_menu_index::KILL_AREA );
        ImGui::SameLine();
        host.confirm_button( _( "Kill NPCs" ), "confirm_kill_npcs",
                             _( "Kill every NPC on the loaded map?  Cannot undo." ),
                             debug_menu_index::KILL_NPCS );
        ImGui::SameLine();
        host.confirm_button( _( "Kill monsters" ), "confirm_kill_mons",
                             _( "Kill every monster on the loaded map?  Cannot undo." ),
                             debug_menu_index::KILL_MONS );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::GEN_SOUND );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::PRINT_OVERMAPS );
    } );

    framed_section( "m_overlay_om", _( "Overmap overlays" ), [&]() {
        host.debug_button( debug_menu_index::DISPLAY_HORDES );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::DISPLAY_WEATHER );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::DISPLAY_SCENTS );
    } );

    framed_section( "m_overlay_local", _( "Local display overlays" ), [&]() {
        ImGui::TextDisabled( "%s",
                             _( "Mutually exclusive: enabling one disables the others." ) );
        game &g_ref = *g;
        // pulse_idx maps each ACTION_DISPLAY_* overlay to its matching
        // debug_menu_index, so an omnibox jump to e.g. "Scent type" can
        // pulse the checkbox here rather than landing on a tab with no
        // visible cue.
        auto overlay_checkbox = [&]( const char *label, action_id act,
        debug_menu_index pulse_idx ) {
            bool active = g_ref.display_overlay_state( act );
            if( host.highlight_checkbox( label, &active, pulse_idx ) ) {
                g_ref.display_toggle_overlay( act );
            }
            ImGui::SameLine();
        };
        overlay_checkbox( _( "Scent" ), ACTION_DISPLAY_SCENT,
                          debug_menu_index::DISPLAY_SCENTS_LOCAL );
        overlay_checkbox( _( "Scent type" ), ACTION_DISPLAY_SCENT_TYPE,
                          debug_menu_index::DISPLAY_SCENTS_TYPE_LOCAL );
        overlay_checkbox( _( "Temperature" ), ACTION_DISPLAY_TEMPERATURE,
                          debug_menu_index::DISPLAY_TEMP );
        overlay_checkbox( _( "Snow depth" ), ACTION_DISPLAY_SNOW_DEPTH,
                          debug_menu_index::DISPLAY_SNOW_DEPTH );
        overlay_checkbox( _( "Vehicle AI" ), ACTION_DISPLAY_VEHICLE_AI,
                          debug_menu_index::DISPLAY_VEHICLE_AI );
        overlay_checkbox( _( "Radiation" ), ACTION_DISPLAY_RADIATION,
                          debug_menu_index::DISPLAY_RADIATION );
        overlay_checkbox( _( "Transparency" ), ACTION_DISPLAY_TRANSPARENCY,
                          debug_menu_index::DISPLAY_TRANSPARENCY );
        overlay_checkbox( _( "NPC attack" ), ACTION_DISPLAY_NPC_ATTACK_POTENTIAL,
                          debug_menu_index::DISPLAY_NPC_ATTACK );
        ImGui::NewLine();

        host.debug_button( debug_menu_index::DISPLAY_VISIBILITY );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::DISPLAY_LIGHTING );
        ImGui::SameLine();
        bool npc_path = g_ref.debug_pathfinding;
        if( ImGui::Checkbox( _( "NPC paths" ), &npc_path ) ) {
            g_ref.debug_pathfinding = npc_path;
        }
        ImGui::SameLine();
        host.debug_button( debug_menu_index::SHOW_SOUND );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::HOUR_TIMER );
    } );
}


const char *tab_vehicle_view::label() const
{
    return _( "Vehicle" );
}

void tab_vehicle_view::draw_body( debug_console &host )
{
    framed_section( "v_actions", _( "Vehicle actions" ), [&]() {
        host.debug_button( debug_menu_index::VEHICLE_BATTERY_CHARGE );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::VEHICLE_DELETE );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::VEHICLE_EXPORT );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::VEHICLE_EFFECTS );
    } );

    framed_section( "v_current", _( "Current vehicle" ), [&]() {
        std::optional<current_vehicle_ref> cv = current_vehicle_for_avatar();
        if( !cv ) {
            ImGui::TextDisabled( "%s", _( "(not in vehicle)" ) );
            return;
        }
        vehicle *v = cv->v;
        map &m = *cv->m;

        label_value( _( "Name" ), "%s", v->name.c_str() );
        ImGui::SameLine( 0, 20.0f );
        label_value( _( "Velocity" ), "%d", v->velocity );
        ImGui::SameLine( 0, 20.0f );
        label_value( _( "Battery" ), "%d",
                     static_cast<int>( v->battery_left( m ) ) );
        ImGui::SameLine();
        add_monitor_button( "veh_mon_current", "veh:" + v->name,
                            snap::vehicle_under_avatar() );

        label_value( _( "Wheel area" ), "%d", v->wheel_area() );
        ImGui::SameLine( 0, 20.0f );
        label_value( _( "Wheel config" ), "%s",
                     v->valid_wheel_config( m ) ? _( "valid" ) : _( "invalid" ) );
    } );

    framed_section( "v_parts", _( "Parts" ), [&]() {
        std::optional<current_vehicle_ref> cv = current_vehicle_for_avatar();
        if( !cv ) {
            ImGui::TextDisabled( "%s", _( "(not in vehicle)" ) );
            return;
        }
        vehicle *v = cv->v;
        map &m = *cv->m;

        // get_all_parts() iterates real (non-fake, non-removed) parts only.
        // part_index() is stable across a session and disambiguates duplicate
        // names (wheels, doors, mirrors) better than the part type id alone.
        const vehicle_part_range parts = v->get_all_parts();
        const int visible = static_cast<int>( v->part_count_real_cached() );
        if( ImGui::BeginTable( "v_parts_table", 7,
                               ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                               ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp,
                               ImVec2( 0.0f, fit_table_height( visible, 280.0f ) ) ) ) {
            fit_col( _( "#" ) );
            fit_col( _( "Mount" ) );
            ImGui::TableSetupColumn( _( "Name" ) );
            fit_col( _( "Status" ) );
            fit_col( _( "Broken" ) );
            fit_col( _( "On" ) );
            fit_col( _( "Fuel" ) );
            ImGui::TableSetupScrollFreeze( 0, 1 );
            ImGui::TableHeadersRow();
            for( const vpart_reference &vp : parts ) {
                const vehicle_part &part = vp.part();
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text( "%d", static_cast<int>( vp.part_index() ) );
                ImGui::TableNextColumn();
                ImGui::Text( "%d,%d", part.mount.x(), part.mount.y() );
                ImGui::TableNextColumn();
                cataimgui::draw_colored_text( part.name(), c_white );
                ImGui::TableNextColumn();
                ImGui::Text( "%d%%",
                             static_cast<int>( part.damage_percent() * 100.0 ) );
                ImGui::TableNextColumn();
                if( part.is_broken() ) {
                    ImGui::TextColored( ImVec4( 0.95f, 0.5f, 0.5f, 1.0f ),
                                        "%s", _( "yes" ) );
                } else {
                    ImGui::TextDisabled( "%s", _( "no" ) );
                }
                ImGui::TableNextColumn();
                ImGui::TextUnformatted( part.enabled ? _( "on" ) : _( "off" ) );
                ImGui::TableNextColumn();
                const itype_id fuel = part.fuel_current();
                if( fuel.is_null() ) {
                    ImGui::TextDisabled( "-" );
                } else {
                    ImGui::Text( "%s: %d", fuel.str().c_str(),
                                 v->engine_fuel_left( m, part ) );
                }
            }
            ImGui::EndTable();
        }
    } );

    framed_section( "v_effects", _( "Active effects" ), [&]() {
        std::optional<current_vehicle_ref> cv = current_vehicle_for_avatar();
        if( !cv ) {
            ImGui::TextDisabled( "%s", _( "(not in vehicle)" ) );
            return;
        }
        vehicle *v = cv->v;
        const std::vector<std::reference_wrapper<const effect>> effs = v->get_effects();
        if( effs.empty() ) {
            ImGui::TextDisabled( "%s", _( "(none)" ) );
            return;
        }
        for( const effect &e : effs ) {
            ImGui::Bullet();
            ImGui::SameLine();
            cataimgui::draw_colored_text( e.disp_name(), c_white );
            ImGui::SameLine();
            ImGui::Text( "int=%d", e.get_intensity() );
        }
    } );
}


const char *tab_tiles_view::label() const
{
    return _( "Tiles" );
}

void tab_tiles_view::load_state( const JsonObject &nested )
{
    nested.read( "tile_coord_input", tile_coord_input );
}

void tab_tiles_view::save_state( JsonOut &jo ) const
{
    jo.member( "tile_coord_input", tile_coord_input );
}

void tab_tiles_view::draw_body( debug_console &host )
{
    avatar &you = get_avatar();
    map &here = get_map();

    ImGui::TextUnformatted( _( "Tile coords (x,y,z) -- empty = avatar position" ) );
    ImGui::SetNextItemWidth( 200 );
    ImGui::InputText( "##tile_coord", &tile_coord_input );

    tripoint_bub_ms target = you.pos_bub( here );
    bool parse_error = false;
    if( !tile_coord_input.empty() ) {
        // Parse "x,y,z" comma-separated triplet. Tolerate spaces around commas.
        std::string_view sv( tile_coord_input );
        std::array<int, 3> parts = { 0, 0, 0 };
        int parsed = 0;
        size_t pos = 0;
        for( int field = 0; field < 3 && pos <= sv.size(); field++ ) {
            const size_t comma = sv.find( ',', pos );
            const size_t end = comma == std::string_view::npos ? sv.size() : comma;
            std::string_view chunk = sv.substr( pos, end - pos );
            while( !chunk.empty() && chunk.front() == ' ' ) {
                chunk.remove_prefix( 1 );
            }
            while( !chunk.empty() && chunk.back() == ' ' ) {
                chunk.remove_suffix( 1 );
            }
            ret_val<int> v = try_parse_integer<int>( chunk, false );
            if( !v.success() ) {
                break;
            }
            parts[field] = v.value();
            parsed++;
            if( comma == std::string_view::npos ) {
                break;
            }
            pos = comma + 1;
        }
        if( parsed == 3 ) {
            target = tripoint_bub_ms( parts[0], parts[1], parts[2] );
        } else {
            parse_error = true;
        }
    }

    if( parse_error ) {
        ImGui::TextColored( ImVec4( 1.0f, 0.4f, 0.4f, 1.0f ),
                            "%s", _( "Invalid coords: expected x,y,z" ) );
    }
    ImGui::Text( "Target: %s", target.to_string().c_str() );
    ImGui::SameLine();
    add_monitor_button( "tile_mon_abs", "tile@" + target.to_string(),
                        snap::tile_at_abs( target ) );
    ImGui::SameLine();
    add_monitor_button( "field_mon_abs", "field@" + target.to_string(),
                        snap::field_at_abs( target ) );
    ImGui::SameLine();
    add_monitor_button( "light_mon_abs", "light@" + target.to_string(),
                        snap::light_at_abs( target ) );
    ImGui::Separator();

    const ter_id &ter = here.ter( target );
    const furn_id &furn = here.furn( target );
    const trap &tr = here.tr_at( target );
    const field &fld = here.field_at( target );
    map_stack tile_items = here.i_at( target );
    Creature *creature_here = get_creature_tracker().creature_at( target );

    // Per-item summary (name x count) -- small, no clipper needed.
    std::vector<std::pair<std::string, int>> item_summary;
    for( const item &it : tile_items ) {
        const std::string nm = remove_color_tags( it.tname() );
        bool merged = false;
        for( auto &p : item_summary ) {
            if( p.first == nm ) {
                p.second++;
                merged = true;
                break;
            }
        }
        if( !merged ) {
            item_summary.emplace_back( nm, 1 );
        }
    }

    // Terrain + furniture flag dumps. These come from the type's flag_set.
    std::vector<std::string> ter_flags;
    for( const std::string &f : ter->get_flags() ) {
        ter_flags.push_back( f );
    }
    std::vector<std::string> furn_flags;
    if( furn ) {
        for( const std::string &f : furn->get_flags() ) {
            furn_flags.push_back( f );
        }
    }

    // Field stack: each field entry with its current intensity.
    std::vector<std::pair<std::string, int>> field_rows;
    for( const auto &fe : fld ) {
        field_rows.emplace_back( fe.second.get_field_type().id().str(),
                                 fe.second.get_field_intensity() );
    }

    const int light_at = static_cast<int>( here.light_at( target ) );
    const int move_cost = here.move_cost( target );
    const bool inbounds = here.inbounds( target );

    auto join = []( const std::vector<std::string> &v, const char *sep ) {
        std::string out;
        for( size_t i = 0; i < v.size(); i++ ) {
            if( i ) {
                out += sep;
            }
            out += v[i];
        }
        return out;
    };

    auto build_md = [&]() {
        std::string md = "# Tile " + target.to_string() + "\n\n";
        md += string_format( "- In bounds: %s\n", inbounds ? "yes" : "no" );
        md += "- Terrain: `" + ter.id().str() + "`\n";
        if( !ter_flags.empty() ) {
            md += "  - Flags: " + join( ter_flags, ", " ) + "\n";
        }
        md += "- Furniture: `" + furn.id().str() + "`\n";
        if( !furn_flags.empty() ) {
            md += "  - Flags: " + join( furn_flags, ", " ) + "\n";
        }
        md += string_format( "- Move cost: %d\n", move_cost );
        md += string_format( "- Light: %d\n", light_at );
        md += string_format( "- Trap: %s\n", tr.id != trap_str_id::NULL_ID()
                             ? tr.id.str().c_str() : "none" );
        md += "- Fields:";
        if( field_rows.empty() ) {
            md += " none\n";
        } else {
            md += "\n";
            for( const auto &fp : field_rows ) {
                md += string_format( "  - %s (intensity %d)\n", fp.first, fp.second );
            }
        }
        md += string_format( "- Creature: %s\n",
                             creature_here ? creature_here->get_name() : std::string( "none" ) );
        md += string_format( "- Items: %d", static_cast<int>( tile_items.size() ) );
        if( item_summary.empty() ) {
            md += "\n";
        } else {
            md += "\n";
            for( const auto &is : item_summary ) {
                md += string_format( "  - %d x %s\n", is.second, is.first );
            }
        }
        return md;
    };

    auto build_json = [&]() {
        std::string js = "{";
        js += string_format( R"("pos":"%s","inbounds":%s)",
                             target.to_string(), inbounds ? "true" : "false" );
        js += string_format( R"(,"ter":"%s","furn":"%s","move_cost":%d,"light":%d)",
                             capture_json_escape( ter.id().str() ),
                             capture_json_escape( furn.id().str() ),
                             move_cost, light_at );
        js += string_format( R"(,"trap":"%s")",
                             capture_json_escape( tr.id != trap_str_id::NULL_ID() ?
                                     tr.id.str() : std::string() ) );
        js += string_format( R"(,"creature":"%s")",
                             capture_json_escape( creature_here ? creature_here->get_name() :
                                     std::string() ) );
        auto str_array = [&]( const std::vector<std::string> &v ) {
            std::string s = "[";
            for( size_t i = 0; i < v.size(); i++ ) {
                if( i ) {
                    s += ",";
                }
                s += "\"" + capture_json_escape( v[i] ) + "\"";
            }
            s += "]";
            return s;
        };
        js += ",\"ter_flags\":" + str_array( ter_flags );
        js += ",\"furn_flags\":" + str_array( furn_flags );
        js += ",\"fields\":[";
        for( size_t i = 0; i < field_rows.size(); i++ ) {
            if( i ) {
                js += ",";
            }
            js += string_format( R"({"id":"%s","intensity":%d})",
                                 capture_json_escape( field_rows[i].first ),
                                 field_rows[i].second );
        }
        js += "],\"items\":[";
        for( size_t i = 0; i < item_summary.size(); i++ ) {
            if( i ) {
                js += ",";
            }
            js += string_format( R"({"name":"%s","count":%d})",
                                 capture_json_escape( item_summary[i].first ),
                                 item_summary[i].second );
        }
        js += "]}";
        return js;
    };

    host.export_bar( "tile", build_md, build_json );

    // Inline summary: compact label / value rows.
    framed_section( "tile_basic", _( "Basics" ), [&]() {
        label_value( _( "Position" ), "%s", target.to_string().c_str() );
        ImGui::SameLine( 0, 20.0f );
        label_value( _( "Move cost" ), "%d", move_cost );
        ImGui::SameLine( 0, 20.0f );
        label_value( _( "Light" ), "%d", light_at );
        label_value( _( "Terrain" ), "%s", ter.id().str().c_str() );
        if( !ter_flags.empty() ) {
            ImGui::TextDisabled( "  flags: %s", join( ter_flags, ", " ).c_str() );
        }
        label_value( _( "Furniture" ), "%s", furn.id().str().c_str() );
        if( !furn_flags.empty() ) {
            ImGui::TextDisabled( "  flags: %s", join( furn_flags, ", " ).c_str() );
        }
        label_value( _( "Trap" ), "%s",
                     tr.id != trap_str_id::NULL_ID() ? tr.id.str().c_str() : "none" );
        label_value( _( "Creature" ), "%s",
                     creature_here ? creature_here->get_name().c_str() : "none" );
    } );

    framed_section( "tile_fields", _( "Fields" ), [&]() {
        if( field_rows.empty() ) {
            ImGui::TextDisabled( "%s", _( "(none)" ) );
        } else {
            for( const auto &fp : field_rows ) {
                ImGui::BulletText( "%s  (intensity %d)", fp.first.c_str(), fp.second );
            }
        }
    } );

    framed_section( "tile_items", _( "Items on tile" ), [&]() {
        if( item_summary.empty() ) {
            ImGui::TextDisabled( "%s", _( "(none)" ) );
        } else {
            for( const auto &is : item_summary ) {
                ImGui::BulletText( "%d x %s", is.second, is.first.c_str() );
            }
        }
    } );
}


const char *tab_player_view::label() const
{
    return _( "Player" );
}

void render_char_stats( Character &ch )
{
    framed_section( "p_stats", _( "Stats" ), [&]() {
        scalar_slider_table( "p_stats", {
            {
                "STR", 1, 30, [&] { return ch.get_str_base(); }, [&]( int v )
                {
                    ch.set_str_base( v );
                }
            },
            {
                "DEX", 1, 30, [&] { return ch.get_dex_base(); }, [&]( int v )
                {
                    ch.set_dex_base( v );
                }
            },
            {
                "INT", 1, 30, [&] { return ch.get_int_base(); }, [&]( int v )
                {
                    ch.set_int_base( v );
                }
            },
            {
                "PER", 1, 30, [&] { return ch.get_per_base(); }, [&]( int v )
                {
                    ch.set_per_base( v );
                }
            },
        } );
    } );
}

void render_char_vitals( Character &ch )
{
    framed_section( "p_vitals", _( "Vitals" ), [&]() {
        add_monitor_button( "vitals_mon", ch.get_name() + ":vitals",
                            snap::char_vitals( ch.getID() ) );
        scalar_slider_table( "p_vitals", {
            {
                "Hunger", -300, 600, [&] { return ch.get_hunger(); }, [&]( int v )
                {
                    ch.set_hunger( v );
                }
            },
            {
                "Thirst", -100, 1200, [&] { return ch.get_thirst(); }, [&]( int v )
                {
                    ch.set_thirst( v );
                }
            },
            {
                "Sleepiness", 0, 1500, [&] { return ch.get_sleepiness(); }, [&]( int v )
                {
                    ch.set_sleepiness( v );
                }
            },
            {
                "Pain", 0, 500, [&] { return ch.get_pain(); }, [&]( int v )
                {
                    ch.set_pain( v );
                }
            },
            {
                "Focus", 0, 200, [&] { return ch.get_focus(); }, [&]( int v )
                {
                    ch.set_focus( v );
                }
            },
            {
                "Painkiller", 0, 240, [&] { return ch.get_painkiller(); }, [&]( int v )
                {
                    ch.set_painkiller( v );
                }
            },
        } );
    } );
}

void render_char_metabolism( Character &ch )
{
    framed_section( "p_metab", _( "Metabolism" ), [&]() {
        ImGui::Text( "%s", string_format(
                         _( "Hunger: %d  Thirst: %d  kcal: %d/%d  BMI: %.1f  BMR: %d" ),
                         ch.get_hunger(), ch.get_thirst(),
                         ch.get_stored_kcal(), ch.get_healthy_kcal(),
                         ch.get_bmi_fat(), ch.get_bmr() ).c_str() );
        ImGui::SameLine();
        add_monitor_button( "p_metab_mon", ch.get_name() + ":metabolism",
                            snap::char_metabolism( ch.getID() ) );
        ImGui::Text( "%s", string_format(
                         _( "Stomach: %d/%d mL, %d kcal  |  Guts: %d/%d mL, %d kcal" ),
                         units::to_milliliter( ch.stomach.contains() ),
                         units::to_milliliter( ch.stomach.capacity( ch ) ),
                         ch.stomach.get_calories(),
                         units::to_milliliter( ch.guts.contains() ),
                         units::to_milliliter( ch.guts.capacity( ch ) ),
                         ch.guts.get_calories() ).c_str() );
        ImGui::SameLine();
        add_monitor_button( "p_gi_mon", ch.get_name() + ":gi",
                            snap::char_gi( ch.getID() ) );
    } );
}

void render_char_skills( Character &ch, character_edit_state &st )
{
    if( !ImGui::CollapsingHeader( _( "Skills##p_skills" ) ) ) {
        return;
    }
    ImGui::Indent();
    const float slider_w = 180.0f;
    ImGui::SetNextItemWidth( slider_w );
    ImGui::SliderInt( "Set all theory##bulk", &st.bulk_theory, 0, 10 );
    ImGui::SameLine();
    if( ImGui::SmallButton( _( "Apply##bulk_th" ) ) ) {
        for( const Skill &sk : Skill::skills ) {
            ch.set_knowledge_level( sk.ident(), st.bulk_theory );
        }
    }
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Set the theory (knowledge) level of every skill to the slider value" ) );
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth( slider_w );
    ImGui::SliderInt( "Set all practice##bulk", &st.bulk_practice, 0, 10 );
    ImGui::SameLine();
    if( ImGui::SmallButton( _( "Apply##bulk_pr" ) ) ) {
        for( const Skill &sk : Skill::skills ) {
            ch.set_skill_level( sk.ident(), st.bulk_practice );
        }
    }
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Set the practice level of every skill to the slider value" ) );
    }
    ImGui::Separator();
    if( ImGui::BeginTable( "p_skills_table", 4,
                           ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                           ImGuiTableFlags_SizingStretchProp ) ) {
        ImGui::TableSetupColumn( _( "Skill" ), ImGuiTableColumnFlags_WidthStretch );
        // Slider cells have no intrinsic width; pin a fixed column so they
        // don't collapse to the header.
        ImGui::TableSetupColumn( _( "Theory" ), ImGuiTableColumnFlags_WidthFixed, 200.0f );
        ImGui::TableSetupColumn( _( "Practice" ), ImGuiTableColumnFlags_WidthFixed, 200.0f );
        fit_col( _( "+M" ) );
        ImGui::TableHeadersRow();
        for( const Skill &sk : Skill::skills ) {
            const skill_id &sid = sk.ident();
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            cataimgui::draw_colored_text( sk.name(), c_white );
            ImGui::TableNextColumn();
            int theory = ch.get_knowledge_level( sid );
            ImGui::SetNextItemWidth( -1.0f );
            if( ImGui::SliderInt( ( "##th_" + sid.str() ).c_str(), &theory, 0, 10 ) ) {
                ch.set_knowledge_level( sid, theory );
            }
            ImGui::TableNextColumn();
            int practice = static_cast<int>( ch.get_skill_level( sid ) );
            ImGui::SetNextItemWidth( -1.0f );
            if( ImGui::SliderInt( ( "##pr_" + sid.str() ).c_str(), &practice, 0, 10 ) ) {
                ch.set_skill_level( sid, practice );
            }
            ImGui::TableNextColumn();
            add_monitor_button(
                ( "skill_mon_" + sid.str() ).c_str(),
                ch.get_name() + " skill:" + sid.str(),
                snap::char_skill( ch.getID(), sid ) );
        }
        ImGui::EndTable();
    }
    ImGui::Unindent();
}

void render_char_proficiencies( Character &ch, character_edit_state &st )
{
    if( !ImGui::CollapsingHeader( _( "Proficiencies##p_profs" ) ) ) {
        return;
    }
    ImGui::Indent();
    ImGui::SetNextItemWidth( 220.0f );
    ImGui::InputTextWithHint( "##prof_filter", _( "filter" ), &st.prof_filter );
    ImGui::SameLine();
    if( ImGui::SmallButton( _( "Learn all##profs" ) ) ) {
        for( const proficiency &pr : proficiency::get_all() ) {
            if( !ch.has_proficiency( pr.prof_id() ) ) {
                ch.add_proficiency( pr.prof_id(), true );
            }
        }
    }
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Mark every loaded proficiency as learned, bypassing prerequisite chains" ) );
    }
    ImGui::SameLine();
    if( ImGui::SmallButton( _( "Forget all##profs" ) ) ) {
        for( const proficiency_id &pid : ch.known_proficiencies() ) {
            ch.lose_proficiency( pid, true );
        }
    }
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Drop every proficiency this character currently knows" ) );
    }
    const std::vector<proficiency> &all_profs = proficiency::get_all();
    std::vector<const proficiency *> filtered;
    filtered.reserve( all_profs.size() );
    for( const proficiency &pr : all_profs ) {
        if( st.prof_filter.empty() ||
            pr.name().find( st.prof_filter ) != std::string::npos ) {
            filtered.push_back( &pr );
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled( "%d / %d", static_cast<int>( filtered.size() ),
                         static_cast<int>( all_profs.size() ) );
    if( ImGui::BeginTable( "p_profs_table", 4,
                           ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                           ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit |
                           ImGuiTableFlags_NoSavedSettings,
                           ImVec2( 0.0f, 320.0f ) ) ) {
        const uint64_t key = lk_mix( lk_str( lk_mix( 0u, filtered.size() ),
                                             st.prof_filter ),
                                     static_cast<uint64_t>( ch.getID().get_value() ) );
        setup_columns<const proficiency *>( "p_profs_table", key, {
            { _( "Name" ), colw::stretch },
            { _( "Known" ), colw::fit },
            { _( "Progress" ), colw::widget, {}, 220.0f },
            { _( "+M" ), colw::widget, {}, 40.0f },
        }, filtered );
        ImGui::TableSetupScrollFreeze( 0, 1 );
        ImGui::TableHeadersRow();
        ImGuiListClipper clipper;
        clipper.Begin( static_cast<int>( filtered.size() ) );
        while( clipper.Step() ) {
            for( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ ) {
                const proficiency &pr = *filtered[i];
                const proficiency_id pid = pr.prof_id();
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                cataimgui::draw_colored_text( pr.name(), c_white );
                ImGui::TableNextColumn();
                bool known = ch.has_proficiency( pid );
                if( ImGui::Checkbox( ( "##k_" + pid.str() ).c_str(), &known ) ) {
                    if( known ) {
                        ch.add_proficiency( pid, true );
                    } else {
                        ch.lose_proficiency( pid, true );
                    }
                }
                ImGui::TableNextColumn();
                if( known ) {
                    ImGui::TextDisabled( "%s", _( "learned" ) );
                } else {
                    const int max_turns = to_turns<int>( pr.time_to_learn() );
                    const int cur_turns = to_turns<int>(
                                              ch.get_proficiency_practiced_time( pid ) );
                    int pct = max_turns > 0 ? cur_turns * 100 / max_turns : 0;
                    ImGui::SetNextItemWidth( -1.0f );
                    if( ImGui::SliderInt( ( "##p_" + pid.str() ).c_str(),
                                          &pct, 0, 100, "%d%%" ) ) {
                        if( pct >= 100 ) {
                            ch.add_proficiency( pid, true );
                        } else {
                            ch.set_proficiency_practiced_time(
                                pid, pct * max_turns / 100 );
                        }
                    }
                }
                ImGui::TableNextColumn();
                add_monitor_button(
                    ( "prof_mon_" + pid.str() ).c_str(),
                    ch.get_name() + " prof:" + pid.str(),
                    snap::char_proficiency( ch.getID(), pid ) );
            }
        }
        ImGui::EndTable();
    }
    ImGui::Unindent();
}

void tab_player_view::draw_body( debug_console &host )
{
    avatar &you = get_avatar();

    render_char_stats( you );
    render_char_vitals( you );
    render_char_metabolism( you );

    framed_section( "p_char", _( "Character" ), [&]() {
        host.debug_button( debug_menu_index::EDIT_PLAYER );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::MUTATE );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::SIX_MILLION_DOLLAR_SURVIVOR );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::CHANGE_SPELLS );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::LEARN_MA );
    } );

    render_char_skills( you, char_edit );
    render_char_proficiencies( you, char_edit );

    framed_section( "p_recipes", _( "Recipes / items" ), [&]() {
        host.debug_button( debug_menu_index::UNLOCK_RECIPES );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::FORGET_ALL_RECIPES );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::FORGET_ALL_ITEMS );
    } );

    framed_section( "p_health", _( "Health" ), [&]() {
        host.debug_button( debug_menu_index::DAMAGE_SELF );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::BLEED_SELF );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::SET_AUTOMOVE );
    } );

    framed_section( "p_npcs", _( "NPCs" ), [&]() {
        host.debug_button( debug_menu_index::CONTROL_NPC );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::IMPORT_FOLLOWER );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::EXPORT_FOLLOWER );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::EXPORT_SELF );
    } );
}


const char *tab_creatures_view::label() const
{
    return _( "Creatures" );
}

void tab_creatures_view::load_state( const JsonObject &nested )
{
    nested.read( "filter", creature_filter );
    nested.read( "selected_id", creature_selected_id );
    nested.read( "filter_within_bubble", filter_within_bubble );
}

void tab_creatures_view::save_state( JsonOut &jo ) const
{
    jo.member( "filter", creature_filter );
    jo.member( "selected_id", creature_selected_id );
    jo.member( "filter_within_bubble", filter_within_bubble );
}

void tab_creatures_view::draw_body( debug_console &host )
{
    ImGui::SetNextItemWidth( 240 );
    ImGui::InputText( _( "Filter##creatures" ), &creature_filter );
    ImGui::SameLine();
    ImGui::Checkbox( _( "Bubble only" ), &filter_within_bubble );

    avatar &you = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms apos = you.pos_bub( here );

    struct row_data {
        std::string kind;
        std::string name;
        std::string type_id;
        std::string faction;
        int hp;
        int hp_max;
        int dist;
        std::string pos;
        std::string sel_key;
    };
    std::vector<row_data> rows;
    for( const monster &m : g->all_monsters() ) {
        const tripoint_bub_ms mpos = m.pos_bub( here );
        if( filter_within_bubble && !here.inbounds( mpos ) ) {
            continue;
        }
        if( !creature_filter.empty() &&
            m.get_name().find( creature_filter ) == std::string::npos &&
            m.type->id.str().find( creature_filter ) == std::string::npos ) {
            continue;
        }
        const std::string pos_str = mpos.to_string();
        creature_ref ref;
        ref.k = creature_ref::kind::monster;
        ref.mon_type = m.type->id.str();
        ref.mon_pos_str = pos_str;
        rows.push_back( {
            "mon", m.get_name(), m.type->id.str(),
            m.faction.id().str(), m.get_hp(), m.get_hp_max(),
            rl_dist( apos, mpos ), pos_str,
            ref.to_string()
        } );
    }
    for( const npc &n : g->all_npcs() ) {
        const tripoint_bub_ms npos = n.pos_bub( here );
        if( filter_within_bubble && !here.inbounds( npos ) ) {
            continue;
        }
        if( !creature_filter.empty() &&
            n.get_name().find( creature_filter ) == std::string::npos ) {
            continue;
        }
        creature_ref ref;
        ref.k = creature_ref::kind::npc;
        ref.npc_id = n.getID().get_value();
        rows.push_back( {
            "npc", n.get_name(), "npc",
            n.get_faction() ? n.get_faction()->id.str() : std::string( "?" ),
            n.get_hp(), n.get_hp_max(),
            rl_dist( apos, npos ), npos.to_string(),
            ref.to_string()
        } );
    }
    if( creature_filter.empty() ||
        you.get_name().find( creature_filter ) != std::string::npos ) {
        rows.push_back( {
            "you", you.get_name(), "avatar",
            you.get_faction() ? you.get_faction()->id.str() : std::string( "?" ),
            you.get_hp(), you.get_hp_max(),
            0, apos.to_string(),
            "avatar:player"
        } );
    }
    std::sort( rows.begin(), rows.end(),
    []( const row_data & a, const row_data & b ) {
        return a.dist < b.dist;
    } );

    ImGui::Text( "%d creatures", static_cast<int>( rows.size() ) );

    {
        std::vector<export_column<row_data>> cols = {
            {
                "Kind", "kind", []( const row_data & r ) { return r.kind; },
                []( const row_data & r )
                {
                    return "\"" + capture_json_escape( r.kind ) + "\"";
                }
            },
            {
                "Name", "name", []( const row_data & r ) { return r.name; },
                []( const row_data & r )
                {
                    return "\"" + capture_json_escape( r.name ) + "\"";
                }
            },
            {
                "Type", "type", []( const row_data & r ) { return r.type_id; },
                []( const row_data & r )
                {
                    return "\"" + capture_json_escape( r.type_id ) + "\"";
                }
            },
            {
                "Faction", "faction", []( const row_data & r ) { return r.faction; },
                []( const row_data & r )
                {
                    return "\"" + capture_json_escape( r.faction ) + "\"";
                }
            },
            {
                "HP", "hp",
                []( const row_data & r ) { return string_format( "%d/%d", r.hp, r.hp_max ); },
                []( const row_data & r )
                {
                    return std::to_string( r.hp );
                }
            },
            {
                "HP max", "hp_max",
                []( const row_data & r ) { return std::to_string( r.hp_max ); },
                []( const row_data & r )
                {
                    return std::to_string( r.hp_max );
                }
            },
            {
                "Dist", "dist", []( const row_data & r ) { return std::to_string( r.dist ); },
                []( const row_data & r )
                {
                    return std::to_string( r.dist );
                }
            },
            {
                "Pos", "pos", []( const row_data & r ) { return r.pos; },
                []( const row_data & r )
                {
                    return "\"" + capture_json_escape( r.pos ) + "\"";
                }
            },
        };
        export_rows( host, "creatures", "creatures", rows, cols );
    }

    const float details_h = ImGui::GetFrameHeightWithSpacing() * 8.0f;
    const float avail_y = ImGui::GetContentRegionAvail().y;
    const float table_h = avail_y - details_h - ImGui::GetStyle().ItemSpacing.y;

    if( ImGui::BeginTable( "creatures_table", 8,
                           ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                           ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit |
                           ImGuiTableFlags_NoSavedSettings,
                           ImVec2( 0.0f, fit_table_height(
                                       static_cast<int>( rows.size() ),
                                       std::max( table_h, 120.0f ) ) ) ) ) {
        const uint64_t key = lk_str( lk_mix( filter_within_bubble ? 1u : 0u,
                                             rows.size() ), creature_filter );
        const std::vector<colspec<row_data>> specs = {
            {
                _( "Kind" ), colw::fit, []( const row_data & r )
                {
                    return r.kind;
                }
            },
            { _( "Name" ), colw::stretch },
            { _( "Type" ), colw::fit, []( const row_data & r ) { return r.type_id; }, 0.0f, 1.0f, 0.25f },
            { _( "Faction" ), colw::fit, []( const row_data & r ) { return r.faction; }, 0.0f, 1.0f, 0.2f },
            {
                _( "HP" ), colw::fit, []( const row_data & r )
                {
                    return string_format( "%d/%d", r.hp, r.hp_max );
                }
            },
            {
                _( "Dist" ), colw::fit, []( const row_data & r )
                {
                    return std::to_string( r.dist );
                }
            },
            { _( "Pos" ), colw::fit, []( const row_data & r ) { return r.pos; }, 0.0f, 1.0f, 0.25f },
            { _( "+M" ), colw::widget, {}, 40.0f },
        };
        setup_columns<row_data>( "creatures_table", key, specs, rows );
        ImGui::TableSetupScrollFreeze( 0, 1 );
        ImGui::TableHeadersRow();
        ImGuiListClipper clipper;
        clipper.Begin( static_cast<int>( rows.size() ) );
        while( clipper.Step() ) {
            grow_columns<row_data>( "creatures_table", specs, rows, clipper.DisplayStart, clipper.DisplayEnd );
            for( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ ) {
                const row_data &r = rows[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                const bool is_selected = creature_selected_id == r.sel_key;
                if( ImGui::Selectable( ( r.kind + "##sel_" + r.sel_key ).c_str(),
                                       is_selected,
                                       ImGuiSelectableFlags_SpanAllColumns |
                                       ImGuiSelectableFlags_AllowOverlap ) ) {
                    creature_selected_id = r.sel_key;
                }
                ImGui::TableNextColumn();
                cataimgui::draw_colored_text( r.name, c_white );
                ImGui::TableNextColumn();
                ImGui::TextUnformatted( r.type_id.c_str() );
                ImGui::TableNextColumn();
                ImGui::TextUnformatted( r.faction.c_str() );
                ImGui::TableNextColumn();
                ImGui::Text( "%d/%d", r.hp, r.hp_max );
                ImGui::TableNextColumn();
                ImGui::Text( "%d", r.dist );
                ImGui::TableNextColumn();
                ImGui::TextUnformatted( r.pos.c_str() );
                ImGui::TableNextColumn();
                add_monitor_button(
                    ( "cre_mon_" + r.sel_key ).c_str(),
                    r.kind + ":" + r.name,
                    snap::creature_named( r.name ) );
            }
        }
        ImGui::EndTable();
    }

    ImGui::SeparatorText( creature_selected_id.empty()
                          ? _( "Select a creature to inspect" )
                          : creature_selected_id.c_str() );
    if( creature_selected_id.empty() ) {
        return;
    }

    Creature *sel = nullptr;
    monster *sel_mon = nullptr;
    npc *sel_npc = nullptr;
    // Set for the avatar and NPCs (any Character); drives the shared editable
    // sections. Stays null for monsters.
    Character *sel_char = nullptr;
    if( creature_selected_id == "avatar:player" ) {
        sel = &you;
        sel_char = &you;
    } else if( const std::optional<creature_ref> ref = creature_ref::parse( creature_selected_id );
               ref.has_value() ) {
        if( ref->k == creature_ref::kind::npc ) {
            for( npc &n : g->all_npcs() ) {
                if( n.getID().get_value() == ref->npc_id ) {
                    sel = &n;
                    sel_npc = &n;
                    sel_char = &n;
                    break;
                }
            }
        } else {
            for( monster &m : g->all_monsters() ) {
                if( m.type->id.str() != ref->mon_type ) {
                    continue;
                }
                if( !ref->mon_pos_str.has_value() ||
                    m.pos_bub( here ).to_string() == *ref->mon_pos_str ) {
                    sel = &m;
                    sel_mon = &m;
                    break;
                }
            }
        }
    }

    if( !sel ) {
        ImGui::TextDisabled( "%s", _( "(no longer present)" ) );
        return;
    }

    kv_panel( "creature_details", [&]() {
        kv_row( _( "name" ), sel->get_name() );
        kv_row( _( "pos" ), sel->pos_bub( here ).to_string() );
        kv_row( _( "hp" ), string_format( "%d / %d", sel->get_hp(),
                                          sel->get_hp_max() ) );
        if( sel_mon ) {
            kv_row( _( "type" ), sel_mon->type->id.str() );
            kv_row( _( "faction" ), sel_mon->faction.id().str() );
            kv_row( _( "speed" ), std::to_string( sel_mon->get_speed() ) );
            kv_row( _( "morale" ), std::to_string( sel_mon->morale ) );
            kv_row( _( "anger" ), std::to_string( sel_mon->anger ) );
        }
        if( sel_npc ) {
            kv_row( _( "char id" ),
                    std::to_string( sel_npc->getID().get_value() ) );
            kv_row( _( "faction" ),
                    sel_npc->get_faction() ? sel_npc->get_faction()->id.str()
                    : std::string( "?" ) );
            kv_row( _( "attitude" ),
                    std::to_string( static_cast<int>( sel_npc->get_attitude() ) ) );
            kv_row( _( "likes/respects/trusts" ),
                    string_format( "%d / %d / %d",
                                   sel_npc->op_of_u.trust,
                                   sel_npc->op_of_u.value,
                                   sel_npc->op_of_u.fear ) );
        }
    } );

    // Heal/Kill/Teleport act on a non-avatar target. The avatar uses the
    // Player tab's Health controls and must not be Kill-able from here.
    if( sel_mon || sel_npc ) {
        if( ImGui::Button( _( "Heal##cr" ) ) ) {
            if( sel_mon ) {
                sel_mon->set_hp( sel_mon->get_hp_max() );
            } else if( sel_npc ) {
                sel_npc->healall( 9999 );
            }
        }
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Restore the selected creature to full HP" ) );
        }
        ImGui::SameLine();
        if( ImGui::Button( _( "Kill##cr" ) ) ) {
            sel->die( &here, nullptr );
        }
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Kill the selected creature with no killer attribution" ) );
        }
        ImGui::SameLine();
        if( ImGui::Button( _( "Teleport here##cr" ) ) ) {
            sel->setpos( here, apos );
        }
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Move the selected creature to the avatar's tile" ) );
        }
        if( sel_npc ) {
            ImGui::SameLine();
            if( ImGui::Button( _( "Control NPC##cr" ) ) ) {
                host.defer_action( debug_menu_index::CONTROL_NPC );
            }
            if( ImGui::IsItemHovered() ) {
                ImGui::SetTooltip( "%s",
                                   _( "Launch the legacy Control NPC menu (this NPC may not be preselected)" ) );
            }
        }
    }

    if( ImGui::CollapsingHeader( _( "Description##cr" ) ) ) {
        const std::vector<std::string> desc = sel->extended_description();
        for( const std::string &line : desc ) {
            cataimgui::draw_colored_text( line, c_white );
        }
    }

    if( ImGui::CollapsingHeader( _( "Effects##cr" ) ) ) {
        const std::vector<std::reference_wrapper<const effect>> effs = sel->get_effects();
        if( effs.empty() ) {
            ImGui::TextDisabled( "%s", _( "(none)" ) );
        } else if( ImGui::BeginTable( "cr_effects", 3,
                                      ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                                      ImGuiTableFlags_SizingStretchProp ) ) {
            ImGui::TableSetupColumn( _( "Effect" ), ImGuiTableColumnFlags_WidthStretch );
            fit_col( _( "Intensity" ) );
            fit_col( _( "Bodypart" ) );
            ImGui::TableHeadersRow();
            for( const effect &e : effs ) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                cataimgui::draw_colored_text( e.disp_name(), c_white );
                ImGui::TableNextColumn();
                ImGui::Text( "%d", e.get_intensity() );
                ImGui::TableNextColumn();
                ImGui::TextUnformatted( e.get_bp().id().str().c_str() );
            }
            ImGui::EndTable();
        }
    }

    if( sel_mon && ImGui::CollapsingHeader( _( "Monster type##cr" ) ) ) {
        const mtype &mt = *sel_mon->type;
        kv_panel( "cr_mtype", [&]() {
            kv_row( _( "id" ), mt.id.str() );
            kv_row( _( "size" ), std::to_string( static_cast<int>( mt.size ) ) );
            kv_row( _( "difficulty" ),
                    std::to_string( mt.get_total_difficulty() ) );
            kv_row( _( "base_speed" ), std::to_string( mt.speed ) );
            kv_row( _( "hp_max" ), std::to_string( mt.hp ) );
            kv_row( _( "morale_default" ), std::to_string( mt.morale ) );
            kv_row( _( "agro_default" ), std::to_string( mt.agro ) );
            kv_row( _( "vision_day/night" ),
                    string_format( "%d / %d", mt.vision_day, mt.vision_night ) );
        } );
    }

    if( sel_mon && ImGui::CollapsingHeader( _( "Monster inventory##cr" ) ) ) {
        if( ImGui::SmallButton( _( "Show in Items tab##mon" ) ) ) {
            host.show_items_for_monster( sel_mon->type->id.str(),
                                         sel_mon->pos_bub( here ) );
        }
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Show this monster.s inventory in Items tab" ) );
        }
        if( sel_mon->inv.empty() ) {
            ImGui::TextDisabled( "%s", _( "(empty)" ) );
        } else {
            for( const item &it : sel_mon->inv ) {
                ImGui::Bullet();
                cataimgui::draw_colored_text( it.tname(), c_white );
            }
        }
    }

    // Avatar and NPCs share the editable Character sections; monsters are not
    // Character and keep only their monster-specific panels.
    if( sel_char ) {
        render_char_stats( *sel_char );
        render_char_vitals( *sel_char );
        render_char_metabolism( *sel_char );
        render_char_skills( *sel_char, char_edit );
        render_char_proficiencies( *sel_char, char_edit );
    }

    if( sel_npc && ImGui::CollapsingHeader( _( "NPC inventory##cr" ) ) ) {
        if( ImGui::SmallButton( _( "Show in Items tab##npc" ) ) ) {
            host.show_items_for_npc( sel_npc->getID() );
        }
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Show this NPC.s inventory in Items tab" ) );
        }
        const item_location wielded = sel_npc->get_wielded_item();
        if( wielded ) {
            ImGui::TextDisabled( "%s", _( "wielded:" ) );
            ImGui::SameLine();
            cataimgui::draw_colored_text( wielded->tname(), c_white );
        } else {
            ImGui::TextDisabled( "%s", _( "(empty hands)" ) );
        }
        const std::vector<item_location> all = sel_npc->all_items_loc();
        ImGui::TextDisabled( "%d carried", static_cast<int>( all.size() ) );
        for( const item_location &loc : all ) {
            if( loc ) {
                ImGui::Bullet();
                cataimgui::draw_colored_text( loc->tname(), c_white );
            }
        }
    }
}


const char *tab_items_view::label() const
{
    return _( "Items" );
}

void tab_items_view::load_state( const JsonObject &nested )
{
    nested.read( "filter", item_filter );
    nested.read( "source", items_source );

    selected.reset();

    std::string sel_source;
    std::string sel_type;
    std::string sel_location;
    int sel_rank = 0;
    bool have_new = nested.read( "selected_source", sel_source );
    have_new = nested.read( "selected_type", sel_type ) || have_new;
    have_new = nested.read( "selected_location", sel_location ) || have_new;
    have_new = nested.read( "selected_rank", sel_rank ) || have_new;
    if( have_new && !sel_type.empty() ) {
        selected = item_pick{ sel_source, sel_type, sel_location, sel_rank };
    }
}

void tab_items_view::save_state( JsonOut &jo ) const
{
    jo.member( "filter", item_filter );
    jo.member( "source", items_source );
    if( selected.has_value() ) {
        jo.member( "selected_source", selected->source_key );
        jo.member( "selected_type", selected->type_id );
        jo.member( "selected_location", selected->location );
        jo.member( "selected_rank", selected->instance_rank );
    }
}

void tab_items_view::set_source( const std::string &source_key )
{
    items_source = source_key;
    selected.reset();
    reverted_from_source.reset();
}

void tab_items_view::draw_body( debug_console &host )
{
    avatar &you = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms apos = you.pos_bub( here );

    // Source registry rebuilt each frame so creature movement / death is reflected.
    struct source_opt {
        std::string key;
        std::string label;
    };
    std::vector<source_opt> sources;
    {
        item_source_ref avatar_ref;
        avatar_ref.k = item_source_ref::kind::avatar;
        sources.push_back( { avatar_ref.to_string(), _( "Avatar (carried)" ) } );
    }
    {
        item_source_ref tile_ref;
        tile_ref.k = item_source_ref::kind::tile_current;
        sources.push_back( {
            tile_ref.to_string(),
            string_format( _( "Tile (current) @ %s" ), apos.to_string() )
        } );
    }
    for( npc &n : g->all_npcs() ) {
        if( here.inbounds( n.pos_bub( here ) ) ) {
            item_source_ref npc_ref;
            npc_ref.k = item_source_ref::kind::npc;
            npc_ref.npc_id = n.getID().get_value();
            sources.push_back( {
                npc_ref.to_string(),
                string_format( "NPC: %s", n.get_name() )
            } );
        }
    }
    for( monster &m : g->all_monsters() ) {
        if( here.inbounds( m.pos_bub( here ) ) ) {
            item_source_ref mon_ref;
            mon_ref.k = item_source_ref::kind::monster;
            mon_ref.mon_type = m.type->id.str();
            mon_ref.mon_pos_str = m.pos_bub( here ).to_string();
            sources.push_back( {
                mon_ref.to_string(),
                string_format( "Mon: %s @ %s", m.get_name(),
                               m.pos_bub( here ).to_string() )
            } );
        }
    }

    int cur_src_idx = -1;
    for( int i = 0; i < static_cast<int>( sources.size() ); i++ ) {
        if( sources[i].key == items_source ) {
            cur_src_idx = i;
            break;
        }
    }
    if( cur_src_idx < 0 ) {
        // Persisted source is gone (NPC moved out of bubble, monster died,
        // etc.). Snap to Avatar and remember the old key for the sticky
        // warning until the user picks a fresh source.
        reverted_from_source = items_source;
        items_source = sources[0].key;
        selected.reset();
        cur_src_idx = 0;
    }
    framed_section( "items_spawn", _( "Spawn" ), [&]() {
        host.debug_button( debug_menu_index::WISH );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::SPAWN_ITEM_GROUP );
    } );

    framed_section( "items_src", _( "Source" ), [&]() {
        if( reverted_from_source.has_value() ) {
            ImGui::TextColored( ImVec4( 0.95f, 0.7f, 0.4f, 1.0f ), "%s",
                                string_format(
                                    _( "Previous source \"%s\" no longer present; reverted to Avatar." ),
                                    *reverted_from_source ).c_str() );
        }
        ImGui::SetNextItemWidth( 360.0f );
        if( ImGui::BeginCombo( "##items_src_combo",
                               sources[cur_src_idx].label.c_str() ) ) {
            for( int i = 0; i < static_cast<int>( sources.size() ); i++ ) {
                const bool sel = i == cur_src_idx;
                // Same-named sources (e.g. two NPCs) would collide; scope by index.
                ImGui::PushID( i );
                if( ImGui::Selectable( sources[i].label.c_str(), sel ) ) {
                    items_source = sources[i].key;
                    selected.reset();
                    reverted_from_source.reset();
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth( -1.0f );
        ImGui::InputTextWithHint( _( "Filter##items" ),
                                  _( "name / type id / category" ), &item_filter );
    } );

    // Snapshot rows up front: keeps the table renderer iterating a stable
    // vector and lets the export builders share the same filtered data.
    struct item_row {
        std::string name;        // colored tname
        std::string plain;       // tname without color tags (for filter)
        std::string type_id;
        std::string category;
        std::string material;
        std::string location;    // "tile" or pocket / parent context
        int count;
        int charges;             // -1 if not a charged item
        int damage;
        int weight_g;
        int volume_ml;
        // Ordinal among rows sharing the same (type_id, location). Drives
        // stable selection identity across filter changes.
        int instance_rank = 0;
        std::vector<std::string> flags;
    };

    auto build_row = []( const item & it, const std::string & location )
    -> item_row {
        item_row r;
        r.name = it.tname();
        r.plain = remove_color_tags( r.name );
        r.type_id = it.typeId().str();
        r.category = it.get_category_shallow().id.str();
        r.material = it.get_base_material().id.str();
        r.location = location;
        r.count = it.count();
        r.charges = it.count_by_charges() ? it.charges : -1;
        r.damage = it.damage();
        r.weight_g = static_cast<int>( units::to_gram( it.weight() ) );
        r.volume_ml = static_cast<int>( units::to_milliliter( it.volume() ) );
        for( const flag_id &f : it.get_flags() )
        {
            r.flags.push_back( f.str() );
        }
        return r;
    };

    auto passes_filter = [this]( const item_row & r ) {
        if( item_filter.empty() ) {
            return true;
        }
        return r.plain.find( item_filter ) != std::string::npos ||
               r.type_id.find( item_filter ) != std::string::npos ||
               r.category.find( item_filter ) != std::string::npos;
    };

    std::vector<item_row> rows;
    // Rank is assigned over the unfiltered visit order so duplicates keep
    // a stable identity regardless of which filter terms are active.
    std::unordered_map<std::string, int> rank_counter;
    auto add = [&]( const item & it, const std::string & where ) {
        item_row r = build_row( it, where );
        const std::string key = r.type_id + '|' + r.location;
        r.instance_rank = rank_counter[key]++;
        if( passes_filter( r ) ) {
            rows.push_back( std::move( r ) );
        }
    };
    const item_source_ref src_ref =
        item_source_ref::parse( items_source ).value_or( item_source_ref{} );
    switch( src_ref.k ) {
        case item_source_ref::kind::tile_current:
            for( const item &it : here.i_at( apos ) ) {
                add( it, "tile" );
            }
            break;
        case item_source_ref::kind::npc: {
            npc *target = nullptr;
            for( npc &n : g->all_npcs() ) {
                if( n.getID().get_value() == src_ref.npc_id ) {
                    target = &n;
                    break;
                }
            }
            if( target ) {
                target->visit_items( [&]( const item * it, item * parent ) {
                    const std::string loc = parent ? parent->tname( 1, false )
                                            : std::string( "self" );
                    add( *it, remove_color_tags( loc ) );
                    return VisitResponse::NEXT;
                } );
            }
            break;
        }
        case item_source_ref::kind::monster:
            for( monster &m : g->all_monsters() ) {
                if( m.type->id.str() != src_ref.mon_type ) {
                    continue;
                }
                if( src_ref.mon_pos_str.has_value() &&
                    m.pos_bub( here ).to_string() != *src_ref.mon_pos_str ) {
                    continue;
                }
                for( const item &it : m.inv ) {
                    add( it, "mon_inv" );
                }
                break;
            }
            break;
        case item_source_ref::kind::avatar:
            you.visit_items( [&]( const item * it, item * parent ) {
                const std::string loc = parent ? parent->tname( 1, false )
                                        : std::string( "self" );
                add( *it, remove_color_tags( loc ) );
                return VisitResponse::NEXT;
            } );
            break;
    }

    {
        std::vector<export_column<item_row>> cols = {
            {
                "Name", "name", []( const item_row & r ) { return r.plain; },
                []( const item_row & r )
                {
                    return "\"" + capture_json_escape( r.plain ) + "\"";
                }
            },
            {
                "Type", "type", []( const item_row & r ) { return "`" + r.type_id + "`"; },
                []( const item_row & r )
                {
                    return "\"" + capture_json_escape( r.type_id ) + "\"";
                }
            },
            {
                "Cat", "cat", []( const item_row & r ) { return r.category; },
                []( const item_row & r )
                {
                    return "\"" + capture_json_escape( r.category ) + "\"";
                }
            },
            {
                "Where", "where", []( const item_row & r ) { return r.location; },
                []( const item_row & r )
                {
                    return "\"" + capture_json_escape( r.location ) + "\"";
                }
            },
            {
                "n", "n", []( const item_row & r ) { return std::to_string( r.count ); },
                []( const item_row & r )
                {
                    return std::to_string( r.count );
                }
            },
            {
                "Chg", "charges",
                []( const item_row & r ) { return r.charges >= 0 ? std::to_string( r.charges ) : std::string( "-" ); },
                []( const item_row & r )
                {
                    return std::to_string( r.charges );
                }
            },
            {
                "Dmg", "damage", []( const item_row & r ) { return std::to_string( r.damage ); },
                []( const item_row & r )
                {
                    return std::to_string( r.damage );
                }
            },
            {
                "Wt(g)", "weight_g", []( const item_row & r ) { return std::to_string( r.weight_g ); },
                []( const item_row & r )
                {
                    return std::to_string( r.weight_g );
                }
            },
            {
                "Vol(mL)", "volume_ml", []( const item_row & r ) { return std::to_string( r.volume_ml ); },
                []( const item_row & r )
                {
                    return std::to_string( r.volume_ml );
                }
            },
            {
                "Material", "material", []( const item_row & r ) { return r.material; },
                []( const item_row & r )
                {
                    return "\"" + capture_json_escape( r.material ) + "\"";
                }
            },
        };
        export_rows( host, "items", sources[cur_src_idx].label, rows, cols );
    }

    label_value( _( "Items" ), "%d", static_cast<int>( rows.size() ) );

    // Resolve the typed selection to a row index for this frame. Stays -1
    // when the pick exists but no matching row is currently visible (filtered
    // out or source switched); we keep `selected` so unfiltering recovers it.
    int selected_row_idx = -1;
    if( selected.has_value() && selected->source_key == items_source ) {
        for( int i = 0; i < static_cast<int>( rows.size() ); i++ ) {
            const item_row &r = rows[i];
            if( r.type_id == selected->type_id && r.location == selected->location
                && r.instance_rank == selected->instance_rank ) {
                selected_row_idx = i;
                break;
            }
        }
    }

    const float details_h = ImGui::GetFrameHeightWithSpacing() * 8.0f;
    const float avail_y = ImGui::GetContentRegionAvail().y;
    const float table_h = avail_y - details_h - ImGui::GetStyle().ItemSpacing.y;

    if( ImGui::BeginTable( "items_table", 10,
                           ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                           ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit |
                           ImGuiTableFlags_NoSavedSettings,
                           ImVec2( 0.0f, fit_table_height(
                                       static_cast<int>( rows.size() ),
                                       std::max( table_h, 120.0f ) ) ) ) ) {
        const uint64_t key = lk_str( lk_str( lk_mix( 0u, rows.size() ),
                                             items_source ), item_filter );
        const std::vector<colspec<item_row>> specs = {
            { _( "Name" ), colw::stretch },
            { _( "Type" ), colw::fit, []( const item_row & r ) { return r.type_id; }, 0.0f, 1.0f, 0.25f },
            {
                _( "Cat" ), colw::fit, []( const item_row & r )
                {
                    return r.category;
                }
            },
            { _( "Where" ), colw::fit, []( const item_row & r ) { return r.location; }, 0.0f, 1.0f, 0.25f },
            {
                _( "n" ), colw::fit, []( const item_row & r )
                {
                    return std::to_string( r.count );
                }
            },
            {
                _( "Chg" ), colw::fit, []( const item_row & r )
                {
                    return r.charges >= 0 ? std::to_string( r.charges ) : std::string( "-" );
                }
            },
            {
                _( "Dmg" ), colw::fit, []( const item_row & r )
                {
                    return r.damage > 0 ? std::to_string( r.damage ) : std::string( "0" );
                }
            },
            {
                _( "Wt(g)" ), colw::fit, []( const item_row & r )
                {
                    return std::to_string( r.weight_g );
                }
            },
            {
                _( "Vol(mL)" ), colw::fit, []( const item_row & r )
                {
                    return std::to_string( r.volume_ml );
                }
            },
            { _( "+M" ), colw::widget, {}, 40.0f },
        };
        setup_columns<item_row>( "items_table", key, specs, rows );
        ImGui::TableSetupScrollFreeze( 0, 1 );
        ImGui::TableHeadersRow();
        ImGuiListClipper clipper;
        clipper.Begin( static_cast<int>( rows.size() ) );
        while( clipper.Step() ) {
            grow_columns<item_row>( "items_table", specs, rows, clipper.DisplayStart, clipper.DisplayEnd );
            for( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ ) {
                const item_row &r = rows[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::PushID( i );
                const bool sel = selected_row_idx == i;
                if( ImGui::Selectable( "##sel", sel,
                                       ImGuiSelectableFlags_SpanAllColumns |
                                       ImGuiSelectableFlags_AllowOverlap ) ) {
                    selected = item_pick{ items_source, r.type_id, r.location, r.instance_rank };
                    selected_row_idx = i;
                }
                if( ImGui::BeginPopupContextItem( "item_ctx" ) ) {
                    if( ImGui::MenuItem( _( "Copy type id" ) ) ) {
                        ImGui::SetClipboardText( r.type_id.c_str() );
                    }
                    if( ImGui::MenuItem( _( "Copy full name" ) ) ) {
                        ImGui::SetClipboardText( r.plain.c_str() );
                    }
                    ImGui::EndPopup();
                }
                ImGui::SameLine();
                cataimgui::draw_colored_text( r.name, c_white );
                ImGui::PopID();
                ImGui::TableNextColumn();
                ImGui::TextDisabled( "%s", r.type_id.c_str() );
                ImGui::TableNextColumn();
                ImGui::TextUnformatted( r.category.c_str() );
                ImGui::TableNextColumn();
                ImGui::TextUnformatted( r.location.c_str() );
                ImGui::TableNextColumn();
                ImGui::Text( "%d", r.count );
                ImGui::TableNextColumn();
                if( r.charges >= 0 ) {
                    ImGui::Text( "%d", r.charges );
                } else {
                    ImGui::TextDisabled( "-" );
                }
                ImGui::TableNextColumn();
                if( r.damage > 0 ) {
                    ImGui::TextColored( ImVec4( 0.95f, 0.6f, 0.4f, 1.0f ),
                                        "%d", r.damage );
                } else {
                    ImGui::TextDisabled( "0" );
                }
                ImGui::TableNextColumn();
                ImGui::Text( "%d", r.weight_g );
                ImGui::TableNextColumn();
                ImGui::Text( "%d", r.volume_ml );
                ImGui::TableNextColumn();
                const bool by_charges = r.charges >= 0;
                const itype_id iid( r.type_id );
                add_monitor_button(
                    ( "it_mon_" + r.type_id + "_" + std::to_string( i ) ).c_str(),
                    ( by_charges ? "item_chg:" : "item_cnt:" ) + r.type_id,
                    by_charges ? snap::item_charges( iid ) : snap::item_count( iid ) );
            }
        }
        ImGui::EndTable();
    }

    ImGui::SeparatorText( selected_row_idx < 0
                          ? _( "Select an item to inspect" )
                          : rows[selected_row_idx].plain.c_str() );
    if( selected_row_idx < 0 ) {
        return;
    }
    const item_row &r = rows[selected_row_idx];
    if( ImGui::BeginTable( "item_details", 2,
                           ImGuiTableFlags_SizingStretchProp ) ) {
        ImGui::TableSetupColumn( "k", ImGuiTableColumnFlags_WidthFixed, 140.0f );
        ImGui::TableSetupColumn( "v" );
        kv_row( _( "name" ), r.plain );
        kv_row( _( "type" ), r.type_id );
        kv_row( _( "category" ), r.category );
        kv_row( _( "material" ), r.material );
        kv_row( _( "where" ), r.location );
        kv_row( _( "count" ), std::to_string( r.count ) );
        if( r.charges >= 0 ) {
            kv_row( _( "charges" ), std::to_string( r.charges ) );
        }
        kv_row( _( "damage" ), std::to_string( r.damage ) );
        kv_row( _( "weight (g)" ), std::to_string( r.weight_g ) );
        kv_row( _( "volume (mL)" ), std::to_string( r.volume_ml ) );
        if( !r.flags.empty() ) {
            std::string fl;
            for( size_t i = 0; i < r.flags.size(); i++ ) {
                if( i ) {
                    fl += ", ";
                }
                fl += r.flags[i];
            }
            kv_row( _( "flags" ), fl );
        }
        ImGui::EndTable();
    }
    if( ImGui::Button( _( "Copy type id##it" ) ) ) {
        ImGui::SetClipboardText( r.type_id.c_str() );
    }
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Copy this item's itype_id to the clipboard" ) );
    }
}


const char *tab_eoc_view::label() const
{
    return _( "EOC" );
}

void tab_eoc_view::load_state( const JsonObject &nested )
{
    nested.read( "filter", eoc_filter );
    nested.read( "type_filter", eoc_type_filter );
    nested.read( "selected_id", eoc_selected_id );
    nested.read( "eval_expr", eval_expr );
    nested.read( "eval_auto", eval_auto );
}

void tab_eoc_view::save_state( JsonOut &jo ) const
{
    jo.member( "filter", eoc_filter );
    jo.member( "type_filter", eoc_type_filter );
    jo.member( "selected_id", eoc_selected_id );
    jo.member( "eval_expr", eval_expr );
    jo.member( "eval_auto", eval_auto );
}

void tab_eoc_view::draw_body( debug_console &host )
{
    draw_eval_panel( host );
    draw_eoc_browser( host );
}

static const char *eoc_type_name( eoc_type type )
{
    switch( type ) {
        case ACTIVATION:
            return "activation";
        case RECURRING:
            return "recurring";
        case AVATAR_DEATH:
            return "avatar_death";
        case NPC_DEATH:
            return "npc_death";
        case PREVENT_DEATH:
            return "prevent_death";
        case EVENT:
            return "event";
        default:
            return "?";
    }
}

void tab_eoc_view::draw_eval_panel( debug_console &host )
{
    if( std::optional<debug_console::eval_result_view> rv =
            host.consume_eval_result() ) {
        eval_result = std::move( rv->result );
        eval_ok = rv->ok;
    }

    ImGui::SeparatorText( _( "Eval" ) );

    if( ImGui::SmallButton( "+##eval_fn_pop" ) ) {
        ImGui::OpenPopup( "eval_func_popup" );
    }
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Insert a function template at the end of the input" ) );
    }
    if( ImGui::BeginPopup( "eval_func_popup" ) ) {
        ImGui::SetNextItemWidth( 220.0f );
        ImGui::InputTextWithHint( "##eval_func_filter", _( "filter" ),
                                  &func_filter );
        ImGui::Separator();
        auto matches = [&]( std::string_view name ) {
            return func_filter.empty() ||
                   name.find( func_filter ) != std::string_view::npos;
        };
        auto insert = [&]( const std::string & tmpl ) {
            if( !eval_expr.empty() && eval_expr.back() != ' ' ) {
                eval_expr += ' ';
            }
            eval_expr += tmpl;
            ImGui::CloseCurrentPopup();
        };
        ImGui::BeginChild( "eval_func_list", ImVec2( 360.0f, 320.0f ),
                           false, ImGuiWindowFlags_HorizontalScrollbar );
        ImGui::SeparatorText( "math" );
        for( const math_func &f : functions ) {
            const std::string nm( f.symbol );
            if( !matches( nm ) ) {
                continue;
            }
            std::string tmpl;
            if( f.num_params < 0 ) {
                tmpl = nm + "(<args>)";
            } else {
                tmpl = nm + "(";
                for( int i = 0; i < f.num_params; i++ ) {
                    if( i ) {
                        tmpl += ", ";
                    }
                    tmpl += "<a" + std::to_string( i + 1 ) + ">";
                }
                tmpl += ")";
            }
            if( ImGui::Selectable( tmpl.c_str() ) ) {
                insert( tmpl );
            }
        }
        ImGui::SeparatorText( "dialogue" );
        for( const auto &kv : get_all_diag_funcs() ) {
            const std::string sym( kv.first );
            const dialogue_func &df = kv.second;
            for( char sc : df.scopes ) {
                const std::string prefix = ( sc == 'g' ) ? std::string() :
                                           std::string( 1, sc ) + "_";
                const std::string nm = prefix + sym;
                if( !matches( nm ) ) {
                    continue;
                }
                std::string tmpl = nm + "(";
                bool first = true;
                for( int i = 0; i < df.num_params; i++ ) {
                    if( !first ) {
                        tmpl += ", ";
                    }
                    first = false;
                    tmpl += "<p" + std::to_string( i + 1 ) + ">";
                }
                for( std::string_view kw : df.kwargs ) {
                    if( !first ) {
                        tmpl += ", ";
                    }
                    first = false;
                    tmpl += "'" + std::string( kw ) + "': <value>";
                }
                tmpl += ")";
                if( ImGui::Selectable( tmpl.c_str() ) ) {
                    insert( tmpl );
                }
            }
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }
    ImGui::SameLine();

    ImGui::SetNextItemWidth( -200.0f );
    const bool entered = ImGui::InputTextWithHint(
                             "##eval_expr",
                             _( "expression, e.g. 1 + u_skill('cooking')" ),
                             &eval_expr,
                             ImGuiInputTextFlags_EnterReturnsTrue );
    ImGui::SameLine();
    const bool clicked = ImGui::Button( _( "Eval##eoc_eval" ) );
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Evaluate expression in avatar dialogue" ) );
    }
    ImGui::SameLine();
    ImGui::Checkbox( _( "auto/turn" ), &eval_auto );
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Re-evaluate the expression every game turn" ) );
    }

    const int now_turn = to_turns<int>( calendar::turn - calendar::turn_zero );
    const bool auto_fire = eval_auto && !eval_expr.empty() &&
                           now_turn != eval_last_turn;
    if( ( entered || clicked || auto_fire ) && !eval_expr.empty() ) {
        // eval() can call debugmsg from the dialogue layer (e.g. when
        // const_actor is null), which opens a popup mid-ImGui-frame and
        // corrupts state. Defer the actual eval to the outer execute() loop.
        host.request_eval( eval_expr );
        eval_last_turn = now_turn;
    }
    if( !eval_result.empty() ) {
        const ImVec4 col = eval_ok ? ImVec4( 0.6f, 0.9f, 0.6f, 1.0f )
                           : ImVec4( 1.0f, 0.5f, 0.5f, 1.0f );
        const char *prefix = eval_ok ? "= " : "Error: ";
        ImGui::TextColored( col, "%s%s", prefix, eval_result.c_str() );
    }
}

void tab_eoc_view::draw_eoc_browser( debug_console &host )
{
    ImGui::SeparatorText( _( "Browser" ) );
    const std::vector<effect_on_condition> &all_eocs = effect_on_conditions::get_all();

    // Filters
    ImGui::SetNextItemWidth( 200 );
    ImGui::InputText( _( "Filter##eocs" ), &eoc_filter );
    ImGui::SameLine();

    static const char *const type_names[] = {
        "all", "activation", "recurring", "avatar_death",
        "npc_death", "prevent_death", "event"
    };
    ImGui::SetNextItemWidth( 120 );
    const char *cur_type = eoc_type_filter < 0 ? "all" : type_names[eoc_type_filter + 1];
    if( ImGui::BeginCombo( _( "Type##eoc" ), cur_type ) ) {
        if( ImGui::Selectable( "all", eoc_type_filter < 0 ) ) {
            eoc_type_filter = -1;
        }
        for( int i = 0; i < static_cast<int>( NUM_EOC_TYPES ); i++ ) {
            if( ImGui::Selectable( type_names[i + 1], eoc_type_filter == i ) ) {
                eoc_type_filter = i;
            }
        }
        ImGui::EndCombo();
    }
    // Merge global (game) + character-scoped queued/inactive EOC IDs.
    // Without merge, player-bound recurring EOCs misreport as "loaded".
    std::unordered_set<std::string> queued_ids;
    std::unordered_map<std::string, time_point> queued_times;
    auto collect_queued = [&]( const queued_eocs & q ) {
        for( const queued_eoc &qe : q.list ) {
            queued_ids.insert( qe.eoc.str() );
            queued_times[qe.eoc.str()] = qe.time;
        }
    };
    std::unordered_set<std::string> inactive_ids;
    auto collect_inactive = [&]( const std::vector<effect_on_condition_id> &v ) {
        for( const effect_on_condition_id &eid : v ) {
            inactive_ids.insert( eid.str() );
        }
    };
    collect_queued( g->queued_global_effect_on_conditions );
    collect_inactive( g->inactive_global_effect_on_condition_vector );
    collect_queued( get_avatar().queued_effect_on_conditions );
    collect_inactive( get_avatar().inactive_effect_on_condition_vector );

    auto status_rank = [&]( const std::string & id ) -> int {
        if( queued_ids.count( id ) )
        {
            return 0;
        }
        if( inactive_ids.count( id ) )
        {
            return 2;
        }
        return 1;
    };

    std::vector<int> filtered;
    filtered.reserve( all_eocs.size() );
    for( int i = 0; i < static_cast<int>( all_eocs.size() ); i++ ) {
        const effect_on_condition &eoc = all_eocs[i];
        if( eoc_type_filter >= 0 &&
            static_cast<int>( eoc.type ) != eoc_type_filter ) {
            continue;
        }
        if( !eoc_filter.empty() &&
            eoc.id.str().find( eoc_filter ) == std::string::npos ) {
            continue;
        }
        filtered.push_back( i );
    }

    ImGui::SameLine();
    ImGui::TextDisabled( "%d / %d", static_cast<int>( filtered.size() ),
                         static_cast<int>( all_eocs.size() ) );

    // Reserve a fixed strip for the per-EOC details panel so the table fills
    // the remaining height. ~9 lines + spacing covers the field list below.
    const float details_h = ImGui::GetFrameHeightWithSpacing() * 9.0f;
    const float avail_y = ImGui::GetContentRegionAvail().y;
    const float table_h = avail_y - details_h - ImGui::GetStyle().ItemSpacing.y;

    auto add_eoc_monitor = [&host]( const std::string & id_copy ) {
        const std::string label = "eoc:" + id_copy;
        debug_capture &cap = debug_capture::instance();
        cap.add_monitor( label, snap::eoc_state( effect_on_condition_id( id_copy ) ),
                         monitor_mode::on_change );
        cap.push_debug_log( debugmode::DF_MONITOR,
                            "added monitor " + label );
        // EOC monitors need fire trace capture; mute raw EOC feed as redundant.
        cap.settings().eoc_trace_capture = true;
        host.set_eoc_trace_visible( false );
    };

    if( ImGui::BeginTable( "eocs", 7,
                           ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                           ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit |
                           ImGuiTableFlags_NoSavedSettings |
                           ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti,
                           ImVec2( 0.0f, fit_table_height(
                                       static_cast<int>( filtered.size() ),
                                       std::max( table_h, 120.0f ) ) ) ) ) {
        const uint64_t key = lk_str( lk_str( lk_mix( 0u, filtered.size() ),
                                             eoc_filter ),
                                     std::to_string( eoc_type_filter ) );
        const std::vector<colspec<int>> specs = {
            { _( "ID" ), colw::stretch, {}, 0.0f, 1.0f, 0.0f, ImGuiTableColumnFlags_DefaultSort },
            {
                _( "Type" ), colw::fit, [&]( const int &i )
                {
                    return std::string( eoc_type_name( all_eocs[i].type ) );
                }
            },
            { _( "Src mod" ), colw::fit, [&]( const int &i ) { return all_eocs[i].src.empty() ? std::string() : all_eocs[i].src.front().second.str(); }, 0.0f, 1.0f, 0.25f },
            {
                _( "Status" ), colw::fit, [&]( const int &i )
                {
                    const std::string &id = all_eocs[i].id.str();
                    return queued_ids.count( id ) ? std::string( "queued" ) : inactive_ids.count(
                        id ) ? std::string( "inactive" ) : std::string( "loaded" );
                }
            },
            {
                _( "Next fire" ), colw::fit, [&]( const int &i )
                {
                    const auto it = queued_times.find( all_eocs[i].id.str() );
                    return it != queued_times.end() ? to_string( it->second - calendar::turn ) : std::string();
                }
            },
            { _( "Run" ), colw::widget, {}, 50.0f, 1.0f, 0.0f, ImGuiTableColumnFlags_NoSort },
            { _( "Mon" ), colw::widget, {}, 50.0f, 1.0f, 0.0f, ImGuiTableColumnFlags_NoSort },
        };
        setup_columns<int>( "eocs", key, specs, filtered, true );
        ImGui::TableSetupScrollFreeze( 0, 1 );
        ImGui::TableHeadersRow();

        if( ImGuiTableSortSpecs *sort_specs = ImGui::TableGetSortSpecs() ) {
            if( sort_specs->SpecsCount > 0 ) {
                std::sort( filtered.begin(), filtered.end(),
                [&]( int a, int b ) {
                    for( int i = 0; i < sort_specs->SpecsCount; i++ ) {
                        const ImGuiTableColumnSortSpecs &s = sort_specs->Specs[i];
                        const effect_on_condition &ea = all_eocs[a];
                        const effect_on_condition &eb = all_eocs[b];
                        int cmp = 0;
                        switch( s.ColumnIndex ) {
                            case 0:
                                cmp = ea.id.str().compare( eb.id.str() );
                                break;
                            case 1:
                                cmp = static_cast<int>( ea.type ) - static_cast<int>( eb.type );
                                break;
                            case 2: {
                                const std::string sa = ea.src.empty() ? "" :
                                                       ea.src.front().second.str();
                                const std::string sb = eb.src.empty() ? "" :
                                                       eb.src.front().second.str();
                                cmp = sa.compare( sb );
                                break;
                            }
                            case 3:
                                cmp = status_rank( ea.id.str() ) -
                                      status_rank( eb.id.str() );
                                break;
                            case 4: {
                                const auto ita = queued_times.find( ea.id.str() );
                                const auto itb = queued_times.find( eb.id.str() );
                                const bool ha = ita != queued_times.end();
                                const bool hb = itb != queued_times.end();
                                if( ha != hb ) {
                                    cmp = ha ? -1 : 1;
                                } else if( ha ) {
                                    cmp = ita->second < itb->second ? -1
                                          : itb->second < ita->second ? 1 : 0;
                                }
                                break;
                            }
                            default:
                                break;
                        }
                        if( cmp != 0 ) {
                            return s.SortDirection == ImGuiSortDirection_Ascending
                                   ? cmp < 0 : cmp > 0;
                        }
                    }
                    return a < b;
                } );
            }
        }

        ImGuiListClipper clipper;
        clipper.Begin( static_cast<int>( filtered.size() ) );
        while( clipper.Step() ) {
            grow_columns<int>( "eocs", specs, filtered, clipper.DisplayStart, clipper.DisplayEnd, true );
            for( int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++ ) {
                const effect_on_condition &eoc = all_eocs[filtered[row]];
                const std::string &id_str = eoc.id.str();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                const bool is_selected = eoc_selected_id == id_str;
                if( ImGui::Selectable( id_str.c_str(), is_selected,
                                       ImGuiSelectableFlags_SpanAllColumns |
                                       ImGuiSelectableFlags_AllowOverlap ) ) {
                    eoc_selected_id = id_str;
                }
                if( ImGui::BeginPopupContextItem( ( "eoc_ctx_" + id_str ).c_str() ) ) {
                    if( ImGui::MenuItem( _( "Copy id" ) ) ) {
                        ImGui::SetClipboardText( id_str.c_str() );
                    }
                    ImGui::EndPopup();
                }

                ImGui::TableNextColumn();
                ImGui::TextUnformatted( eoc_type_name( eoc.type ) );

                ImGui::TableNextColumn();
                if( !eoc.src.empty() ) {
                    ImGui::TextUnformatted( eoc.src.front().second.str().c_str() );
                }

                ImGui::TableNextColumn();
                if( queued_ids.count( id_str ) ) {
                    ImGui::TextColored( ImVec4( 0.4f, 1.0f, 0.4f, 1.0f ), "queued" );
                } else if( inactive_ids.count( id_str ) ) {
                    ImGui::TextColored( ImVec4( 0.6f, 0.6f, 0.6f, 1.0f ), "inactive" );
                } else {
                    ImGui::TextUnformatted( "loaded" );
                }

                ImGui::TableNextColumn();
                auto it = queued_times.find( id_str );
                if( it != queued_times.end() ) {
                    time_duration remaining = it->second - calendar::turn;
                    ImGui::TextUnformatted( to_string( remaining ).c_str() );
                }

                ImGui::TableNextColumn();
                ImGui::PushID( filtered[row] );
                if( ImGui::SmallButton( ">" ) ) {
                    host.defer_eoc( effect_on_condition_id( id_str ) );
                }
                if( ImGui::IsItemHovered() ) {
                    ImGui::SetTooltip( "%s",
                                       _( "Trigger this EOC immediately with the avatar as alpha talker" ) );
                }
                ImGui::PopID();

                ImGui::TableNextColumn();
                ImGui::PushID( filtered[row] + 100000 );
                const bool mon_clicked = ImGui::SmallButton( "+M" );
                if( ImGui::IsItemHovered() ) {
                    ImGui::SetTooltip( "%s",
                                       _( "Monitor this EOC (Trace > Monitors)" ) );
                }
                if( mon_clicked ) {
                    add_eoc_monitor( id_str );
                }
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }

    ImGui::SeparatorText( eoc_selected_id.empty()
                          ? _( "Select an EOC to inspect" )
                          : eoc_selected_id.c_str() );
    if( !eoc_selected_id.empty() ) {
        const effect_on_condition *sel = nullptr;
        for( const effect_on_condition &eoc : all_eocs ) {
            if( eoc.id.str() == eoc_selected_id ) {
                sel = &eoc;
                break;
            }
        }
        if( !sel ) {
            ImGui::TextDisabled( "%s", _( "(no longer loaded)" ) );
        } else {
            kv_panel( "eoc_details", [&]() {
                kv_row( _( "type" ), eoc_type_name( sel->type ) );
                kv_row( _( "global" ), sel->global ? "true" : "false" );
                kv_row( _( "run_for_npcs" ), sel->run_for_npcs ? "true" : "false" );
                kv_row( _( "has_condition" ), sel->has_condition ? "true" : "false" );
                kv_row( _( "has_deactivate_condition" ),
                        sel->has_deactivate_condition ? "true" : "false" );
                kv_row( _( "has_false_effect" ),
                        sel->has_false_effect ? "true" : "false" );
                if( sel->type == eoc_type::EVENT ) {
                    kv_row( _( "required_event" ),
                            io::enum_to_string<event_type>( sel->required_event ) );
                }
                std::string src_chain;
                for( const auto &pr : sel->src ) {
                    if( !src_chain.empty() ) {
                        src_chain += " -> ";
                    }
                    src_chain += pr.second.str();
                }
                kv_row( _( "src" ), src_chain );
                auto it = queued_times.find( eoc_selected_id );
                if( it != queued_times.end() ) {
                    kv_row( _( "next fire" ),
                            to_string( it->second - calendar::turn ) );
                }
            } );

            if( ImGui::Button( _( "Trigger now##eoc_det" ) ) ) {
                host.defer_eoc( effect_on_condition_id( eoc_selected_id ) );
            }
            if( ImGui::IsItemHovered() ) {
                ImGui::SetTooltip( "%s",
                                   _( "Trigger this EOC immediately with the avatar as alpha talker" ) );
            }
            ImGui::SameLine();
            if( ImGui::Button( _( "+Monitor##eoc_det" ) ) ) {
                add_eoc_monitor( eoc_selected_id );
            }
            if( ImGui::IsItemHovered() ) {
                ImGui::SetTooltip( "%s",
                                   _( "Monitor this EOC and mute the raw feed" ) );
            }
            ImGui::SameLine();
            if( ImGui::Button( _( "Copy id##eoc_det" ) ) ) {
                ImGui::SetClipboardText( eoc_selected_id.c_str() );
            }
            if( ImGui::IsItemHovered() ) {
                ImGui::SetTooltip( "%s", _( "Copy the EOC id to the clipboard" ) );
            }
        }
    }
}


const char *tab_data_view::label() const
{
    return _( "Data" );
}

namespace
{

struct var_table_writer {
    std::function<void( const std::string &, const diag_value & )> write;
    std::function<void( const std::string & )> erase;
};

// Colored relation int -> green/red/dim grey.
ImVec4 relation_color( int v )
{
    if( v > 0 ) {
        return ImVec4( 0.4f, 0.9f, 0.4f, 1.0f );
    }
    if( v < 0 ) {
        return ImVec4( 0.95f, 0.45f, 0.45f, 1.0f );
    }
    return ImVec4( 0.7f, 0.7f, 0.7f, 1.0f );
}

const char *timed_event_type_name( timed_event_type t )
{
    switch( t ) {
        case timed_event_type::NONE:
            return "none";
        case timed_event_type::HELP:
            return "help";
        case timed_event_type::WANTED:
            return "wanted";
        case timed_event_type::ROBOT_ATTACK:
            return "robot_attack";
        case timed_event_type::SPAWN_WYRMS:
            return "spawn_wyrms";
        case timed_event_type::AMIGARA:
            return "amigara";
        case timed_event_type::AMIGARA_WHISPERS:
            return "amigara_whispers";
        case timed_event_type::ROOTS_DIE:
            return "roots_die";
        case timed_event_type::TEMPLE_OPEN:
            return "temple_open";
        case timed_event_type::TEMPLE_FLOOD:
            return "temple_flood";
        case timed_event_type::TEMPLE_SPAWN:
            return "temple_spawn";
        case timed_event_type::DIM:
            return "dim";
        case timed_event_type::ARTIFACT_LIGHT:
            return "artifact_light";
        case timed_event_type::DSA_ALRP_SUMMON:
            return "dsa_alrp_summon";
        case timed_event_type::CUSTOM_LIGHT_LEVEL:
            return "custom_light";
        case timed_event_type::TRANSFORM_RADIUS:
            return "transform_radius";
        case timed_event_type::UPDATE_MAPGEN:
            return "update_mapgen";
        case timed_event_type::REVERT_SUBMAP:
            return "revert_submap";
        case timed_event_type::OVERRIDE_PLACE:
            return "override_place";
        case timed_event_type::EXPLOSION:
            return "explosion";
        case timed_event_type::NUM_TIMED_EVENT_TYPES:
            break;
    }
    return "?";
}

// Empty diag_value on parse failure, so callers can reject bad input.
diag_value parse_value( const std::string &value, dv_types type )
{
    switch( type ) {
        case dv_types::DOUBLE: {
            // Classic locale plus a whole-string check rejects a partial parse
            // ("1,2,3") instead of keeping just the leading number.
            std::istringstream is( value );
            is.imbue( std::locale::classic() );
            double d = 0.0;
            if( ( is >> d ) && ( is >> std::ws ).eof() ) {
                return diag_value{ d };
            }
            break;
        }
        case dv_types::STRING:
            return diag_value{ value };
        case dv_types::TRIPOINT: {
            // operator>>( tripoint ) leaves failbit unset on bad delimiters, so
            // read the punctuation explicitly and require the whole string.
            std::istringstream is( value );
            is.imbue( std::locale::classic() );
            tripoint result;
            char lp = 0;
            char c1 = 0;
            char c2 = 0;
            char rp = 0;
            if( ( is >> lp >> result.x >> c1 >> result.y >> c2 >> result.z >> rp ) &&
                lp == '(' && c1 == ',' && c2 == ',' && rp == ')' &&
                ( is >> std::ws ).eof() ) {
                return diag_value{ tripoint_abs_ms{ result } };
            }
            break;
        }
        case dv_types::last:
            break;
    }
    return {};
}

constexpr std::array<const char *, static_cast<size_t>( dv_types::last )> var_type_names{ {
        "double", "string", "tripoint",
    }
};

void draw_edit_val( std::string &val, dv_types &type )
{
    ImGui::SetNextItemWidth( 100.0f );
    if( ImGui::BeginCombo( "##var_type", var_type_names[static_cast<size_t>( type )] ) ) {
        for( size_t i = 0; i < var_type_names.size(); i++ ) {
            if( ImGui::Selectable( var_type_names[i], static_cast<size_t>( type ) == i ) ) {
                type = static_cast<dv_types>( i );
            }
        }
        ImGui::EndCombo();
    }
    std::string hint;
    switch( type ) {
        case dv_types::DOUBLE:
            hint = _( "value (number)" );
            break;
        case dv_types::STRING:
            hint = _( "value (text)" );
            break;
        case dv_types::TRIPOINT:
            hint = _( "(x,y,z)" );
            break;
        case dv_types::last:
            break;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth( 200.0f );
    ImGui::InputTextWithHint( "##var_edit_value", hint.c_str(), &val );
}

} // namespace

// Reusable var-key/value table. table_id discriminates the trailing
// var_edit_popup so multiple var_tables in one frame don't fight over a
// single popup.
static void draw_var_table( const char *table_id,
                            const global_variables::impl_t &vars,
                            float height,
                            const std::string &scope_prefix,
                            const std::function<std::string( const std::string & )>
                            &value_provider,
                            const var_table_writer &writer,
                            var_browser_state &state )
{
    const bool can_monitor = static_cast<bool>( value_provider );
    const bool can_edit = static_cast<bool>( writer.write );
    const bool can_delete = static_cast<bool>( writer.erase );
    const std::string &filter = state.filter;
    std::string &edit_table = state.edit_table;
    std::string &edit_key = state.edit_key;
    std::string &edit_buf = state.edit_buf;

    // Drawn before the empty-list check so a var can be added to an empty
    // scope. PushID keeps widget IDs distinct between tables in one frame.
    if( can_edit ) {
        ImGui::PushID( table_id );
        ImGui::SetNextItemWidth( 160.0f );
        ImGui::InputTextWithHint( "##new_var_key", _( "new key" ), &state.new_key );
        ImGui::SameLine();
        draw_edit_val( state.new_val, state.new_var_type );
        ImGui::SameLine();
        ImGui::BeginDisabled( state.new_key.empty() );
        if( ImGui::SmallButton( _( "Add##var" ) ) ) {
            diag_value parsed = parse_value( state.new_val, state.new_var_type );
            if( !parsed.is_empty() ) {
                writer.write( state.new_key, parsed );
                state.new_key.clear();
                state.new_val.clear();
            }
        }
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s", _( "Create variable from key + value" ) );
        }
        ImGui::EndDisabled();
        ImGui::PopID();
    }

    if( vars.empty() ) {
        ImGui::TextUnformatted( _( "No variables." ) );
        return;
    }

    bool editpopup = false;
    std::string to_erase;
    bool do_erase = false;

    const int n_cols = can_monitor ? 6 : 5;

    int filtered_count = 0;
    for( const auto &kv : vars ) {
        if( filter.empty() || kv.first.find( filter ) != std::string::npos ) {
            filtered_count++;
        }
    }

    if( ImGui::BeginTable( table_id, n_cols,
                           ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                           ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit |
                           ImGuiTableFlags_NoSavedSettings |
                           ImGuiTableFlags_Sortable,
                           ImVec2( 0.0f, fit_table_height( filtered_count, height ) ) ) ) {
        using var_row = const std::pair<const std::string, diag_value> *;
        std::vector<var_row> rows;
        rows.reserve( vars.size() );
        for( const auto &kv : vars ) {
            if( !filter.empty() && kv.first.find( filter ) == std::string::npos ) {
                continue;
            }
            rows.push_back( &kv );
        }

        std::vector<colspec<var_row>> specs = {
            { _( "Key" ), colw::stretch, {}, 0.0f, 1.0f, 0.0f, ImGuiTableColumnFlags_DefaultSort },
            {
                _( "Type" ), colw::fit, []( const var_row & r )
                {
                    const diag_value &v = r->second;
                    return std::string( v.is_dbl() ? "dbl" : v.is_str() ? "str" :
                                        v.is_array() ? "arr" : v.is_tripoint() ? "pos" : "?" );
                }
            },
            { _( "Value" ), colw::fit, []( const var_row & r ) { return r->second.to_string( true ); }, 0.0f, 1.0f, 0.25f },
            {
                _( "As Duration" ), colw::fit, []( const var_row & r )
                {
                    return r->second.is_dbl() ? to_string( time_duration::from_turns( r->second.dbl() ) ) :
                    std::string();
                }, 0.0f, 1.0f, 0.0f, ImGuiTableColumnFlags_NoSort
            },
            {
                _( "As Timepoint" ), colw::fit, []( const var_row & r )
                {
                    return r->second.is_dbl() ? to_string( calendar::turn_zero + time_duration::from_turns(
                            r->second.dbl() ) ) : std::string();
                }, 0.0f, 1.0f, 0.0f, ImGuiTableColumnFlags_NoSort
            },
        };
        if( can_monitor ) {
            specs.push_back( { _( "Mon" ), colw::widget, {}, 40.0f, 1.0f, 0.0f, ImGuiTableColumnFlags_NoSort } );
        }
        const uint64_t key = lk_mix( lk_str( lk_mix( 0u, rows.size() ), filter ),
                                     can_monitor ? 1u : 0u );
        setup_columns<var_row>( table_id, key, specs, rows, true );
        ImGui::TableSetupScrollFreeze( 0, 1 );
        ImGui::TableHeadersRow();

        if( ImGuiTableSortSpecs *sort_specs = ImGui::TableGetSortSpecs() ) {
            if( sort_specs->SpecsCount > 0 ) {
                std::sort( rows.begin(), rows.end(),
                           [&]( const std::pair<const std::string, diag_value> *a,
                const std::pair<const std::string, diag_value> *b ) {
                    for( int i = 0; i < sort_specs->SpecsCount; i++ ) {
                        const ImGuiTableColumnSortSpecs &s = sort_specs->Specs[i];
                        int cmp = 0;
                        switch( s.ColumnIndex ) {
                            case 0:
                                cmp = a->first.compare( b->first );
                                break;
                            case 1: {
                                auto rank = []( const diag_value & v ) {
                                    return v.is_dbl() ? 0 : v.is_str() ? 1
                                           : v.is_tripoint() ? 2 : v.is_array() ? 3 : 4;
                                };
                                cmp = rank( a->second ) - rank( b->second );
                                break;
                            }
                            case 2:
                                cmp = a->second.to_string( true )
                                      .compare( b->second.to_string( true ) );
                                break;
                            default:
                                break;
                        }
                        if( cmp != 0 ) {
                            return s.SortDirection == ImGuiSortDirection_Ascending
                                   ? cmp < 0 : cmp > 0;
                        }
                    }
                    return a->first.compare( b->first ) < 0;
                } );
            }
        }

        grow_columns<var_row>( table_id, specs, rows, 0, static_cast<int>( rows.size() ), true );
        int row_id = 0;
        for( const auto *row : rows ) {
            const std::string &key = row->first;
            const diag_value &val = row->second;

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushID( row_id );
            if( ImGui::Selectable( key.c_str(), false,
                                   ImGuiSelectableFlags_SpanAllColumns |
                                   ImGuiSelectableFlags_AllowOverlap ) ) {
                ImGui::SetClipboardText( ( key + "=" + val.to_string( true ) ).c_str() );
            }
            if( ImGui::BeginPopupContextItem( "var_ctx" ) ) {
                if( ImGui::MenuItem( _( "Copy key" ) ) ) {
                    ImGui::SetClipboardText( key.c_str() );
                }
                if( ImGui::MenuItem( _( "Copy value" ) ) ) {
                    ImGui::SetClipboardText( val.to_string( true ).c_str() );
                }
                if( ImGui::MenuItem( _( "Copy key=value" ) ) ) {
                    ImGui::SetClipboardText(
                        ( key + "=" + val.to_string( true ) ).c_str() );
                }
                if( can_edit ) {
                    if( ImGui::MenuItem( _( "Edit value" ) ) ) {
                        edit_table = table_id;
                        edit_key = key;
                        edit_buf = val.to_string( true );
                        state.edit_type = val.is_dbl() ? dv_types::DOUBLE
                                          : val.is_tripoint() ? dv_types::TRIPOINT
                                          : dv_types::STRING;
                        // OpenPopup must run from this function's ID stack,
                        // not the context menu popup's, or BeginPopupModal
                        // never matches it.
                        editpopup = true;
                    }
                }
                if( can_delete ) {
                    ImGui::Separator();
                    if( ImGui::MenuItem( _( "Delete" ) ) ) {
                        // Defer erase: key and val alias the map node that
                        // erase would free while this row still renders.
                        to_erase = key;
                        do_erase = true;
                    }
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();

            ImGui::TableNextColumn();
            const char *type_str = val.is_dbl() ? "dbl" :
                                   val.is_str() ? "str" :
                                   val.is_array() ? "arr" :
                                   val.is_tripoint() ? "pos" : "?";
            ImGui::TextUnformatted( type_str );

            ImGui::TableNextColumn();
            ImGui::TextUnformatted( val.to_string( true ).c_str() );

            ImGui::TableNextColumn();
            if( val.is_dbl() ) {
                ImGui::TextUnformatted(
                    to_string( time_duration::from_turns( val.dbl() ) ).c_str() );
            }

            ImGui::TableNextColumn();
            if( val.is_dbl() ) {
                ImGui::TextUnformatted(
                    to_string( calendar::turn_zero +
                               time_duration::from_turns( val.dbl() ) ).c_str() );
            }

            if( can_monitor ) {
                ImGui::TableNextColumn();
                ImGui::PushID( row_id );
                const bool var_mon_clicked = ImGui::SmallButton( "+M" );
                if( ImGui::IsItemHovered() ) {
                    ImGui::SetTooltip( "%s",
                                       _( "Monitor this variable (Trace > Monitors)" ) );
                }
                if( var_mon_clicked ) {
                    std::string key_copy = key;
                    auto provider_copy = value_provider;
                    auto snap = [provider_copy, key_copy]() {
                        return provider_copy( key_copy );
                    };
                    std::string label = scope_prefix;
                    label += ":";
                    label += key_copy;
                    debug_capture::instance().add_monitor(
                        label, std::move( snap ),
                        monitor_mode::on_change );
                    debug_capture::instance().push_debug_log(
                        debugmode::DF_MONITOR, "added monitor " + label );
                }
                ImGui::PopID();
            }
            row_id++;
        }
        ImGui::EndTable();
    }

    if( do_erase ) {
        writer.erase( to_erase );
    }

    if( can_edit && edit_table == table_id ) {
        if( editpopup ) {
            ImGui::OpenPopup( "var_edit_popup" );
        }
        ImGui::SetNextWindowSize( ImVec2( 360.0f, 0.0f ) );
        if( ImGui::BeginPopupModal( "var_edit_popup", nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize ) ) {
            ImGui::TextDisabled( "%s", edit_key.c_str() );
            draw_edit_val( edit_buf, state.edit_type );
            if( ImGui::Button( _( "Save##var_edit" ) ) ) {
                diag_value parsed = parse_value( edit_buf, state.edit_type );
                if( !parsed.is_empty() ) {
                    writer.write( edit_key, parsed );
                    edit_table.clear();
                    edit_key.clear();
                    edit_buf.clear();
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if( ImGui::Button( _( "Cancel##var_edit" ) ) ) {
                edit_table.clear();
                edit_key.clear();
                edit_buf.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

void tab_data_view::load_state( const JsonObject &nested )
{
    nested.read( "var_filter", var_browser.filter );
}

void tab_data_view::save_state( JsonOut &jo ) const
{
    jo.member( "var_filter", var_browser.filter );
}

void tab_data_view::draw_body( debug_console &host )
{
    framed_section( "data_edit", _( "Edit & inspect" ), [&]() {
        host.debug_button( debug_menu_index::EDIT_GLOBAL_VARS );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::ACTIVATE_EOC );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::EDIT_FACTION );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::TALK_TOPIC );
        ImGui::SameLine();
        host.debug_button_with_label( _( "Mut categories" ), debug_menu_index::SHOW_MUT_CAT );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::PRINT_NPC_MAGIC );
    } );

    framed_section( "data_tests", _( "Tests & benchmarks" ), [&]() {
        host.debug_button( debug_menu_index::TEST_IT_GROUP );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::TRAIT_GROUP );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::TEST_MAP_EXTRA_DISTRIBUTION );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::BENCHMARK );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::TEST_WEATHER );
    } );

    framed_section( "data_export", _( "Export & print" ), [&]() {
        host.debug_button( debug_menu_index::WRITE_GLOBAL_EOCS );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::WRITE_GLOBAL_VARS );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::WRITE_TIMED_EVENTS );
        ImGui::SameLine();
        host.debug_button( debug_menu_index::WRITE_CITY_LIST );
        ImGui::SameLine();
        host.debug_button_with_label( _( "Effect list" ), debug_menu_index::GENERATE_EFFECT_LIST );
        ImGui::SameLine();
        host.debug_button_with_label( _( "Faction info" ), debug_menu_index::PRINT_FACTION_INFO );
    } );

    draw_game_state_panel();
    draw_var_browser( host );
    draw_faction_browser();
    draw_timed_events_table();
    draw_item_wakeups_table();
}

void tab_data_view::draw_game_state_panel()
{
    if( !ImGui::CollapsingHeader( _( "Game state" ), ImGuiTreeNodeFlags_DefaultOpen ) ) {
        return;
    }

    const avatar &player_character = get_avatar();
    const map &here = get_map();

    label_value( _( "Turn" ), "%d",
                 to_turns<int>( calendar::turn - calendar::turn_zero ) );
    ImGui::SameLine( 0, 20.0f );
    label_value( _( "Position" ), "%s",
                 player_character.pos_bub( here ).to_string().c_str() );
    ImGui::SameLine( 0, 20.0f );
    label_value( _( "Submap" ), "%s",
                 here.get_abs_sub().to_string().c_str() );
    ImGui::SameLine();
    add_monitor_button( "gs_pos_mon", "avatar:pos", snap::avatar_pos() );

    {
        std::unordered_map<std::string, int> creature_counts;
        for( const Creature &critter : g->all_creatures() ) {
            creature_counts[critter.get_name()]++;
        }
        std::vector<std::pair<std::string, int>> sorted_creatures( creature_counts.begin(),
                                              creature_counts.end() );
        std::sort( sorted_creatures.begin(), sorted_creatures.end(),
        []( const std::pair<std::string, int> &a, const std::pair<std::string, int> &b ) {
            return a.second > b.second;
        } );
        ImGui::Text( "%s", string_format( _( "Creatures: %d total" ),
                                          g->num_creatures() ).c_str() );
        ImGui::SameLine();
        add_monitor_button( "gs_creatures_mon", "world:creature_count",
                            snap::world_creature_count() );
        if( !sorted_creatures.empty() && ImGui::TreeNode( _( "Breakdown" ) ) ) {
            for( const auto &[name, count] : sorted_creatures ) {
                ImGui::Text( "  %d x %s", count, name.c_str() );
            }
            ImGui::TreePop();
        }
    }

    {
        int npc_count = 0;
        std::vector<std::pair<std::string, std::string>> npc_info;
        for( const npc &guy : g->all_npcs() ) {
            npc_info.emplace_back( guy.get_name(),
                                   guy.pos_bub( here ).to_string() );
            npc_count++;
        }
        ImGui::Text( "%s", string_format( _( "NPCs: %d" ), npc_count ).c_str() );
        ImGui::SameLine();
        add_monitor_button( "gs_npcs_mon", "world:npc_count",
                            snap::world_npc_count() );
        if( npc_count > 0 && ImGui::TreeNode(
                string_format( _( "Roster##npcs_tree" ) ).c_str() ) ) {
            for( const auto &[name, pos] : npc_info ) {
                ImGui::Text( "  %s @ %s", name.c_str(), pos.c_str() );
            }
            ImGui::TreePop();
        }
    }
}

void tab_data_view::draw_var_browser( debug_console &/*host*/ )
{
    if( !ImGui::CollapsingHeader( _( "Variables" ) ) ) {
        return;
    }


    ImGui::SetNextItemWidth( 200 );
    ImGui::InputText( _( "Filter##vars" ), &var_browser.filter );
    ImGui::SameLine();
    if( ImGui::SmallButton( _( "Copy all##vars" ) ) ) {
        std::string clipboard;
        for( const auto &[key, val] : get_globals().get_global_values() ) {
            clipboard += key + ";" + val.to_string( true ) + "\n";
        }
        ImGui::SetClipboardText( clipboard.c_str() );
    }
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Copy all global variables to clipboard" ) );
    }

    auto global_provider = []( const std::string & k ) -> std::string {
        const global_variables::impl_t &m = get_globals().get_global_values();
        const global_variables::impl_t::const_iterator it = m.find( k );
        return it == m.end() ? std::string( "<gone>" ) : it->second.to_string( true );
    };
    auto avatar_provider = []( const std::string & k ) -> std::string {
        const global_variables::impl_t &m = get_avatar().get_values();
        const global_variables::impl_t::const_iterator it = m.find( k );
        return it == m.end() ? std::string( "<gone>" ) : it->second.to_string( true );
    };

    var_table_writer global_writer{
        []( const std::string & k, const diag_value & v )
        {
            get_globals().set_global_value( k, v );
        },
        []( const std::string & k )
        {
            get_globals().remove_global_value( k );
        }
    };
    var_table_writer avatar_writer{
        []( const std::string & k, const diag_value & v )
        {
            get_avatar().set_value( k, v );
        },
        []( const std::string & k )
        {
            get_avatar().remove_value( k );
        }
    };

    if( ImGui::TreeNodeEx( _( "Global" ), ImGuiTreeNodeFlags_DefaultOpen ) ) {
        draw_var_table( "global_vars", get_globals().get_global_values(),
                        150.0f, "global", global_provider, global_writer, var_browser );
        ImGui::TreePop();
    }

    avatar &player_character = get_avatar();
    {
        std::string label = string_format( _( "Avatar (%s)" ),
                                           player_character.get_name() );
        if( ImGui::TreeNode( label.c_str() ) ) {
            draw_var_table( "avatar_vars", player_character.get_values(),
                            120.0f, "avatar", avatar_provider, avatar_writer, var_browser );
            ImGui::TreePop();
        }
    }

    {
        int npc_idx = 0;
        for( npc &guy : g->all_npcs() ) {
            const std::string nm = guy.get_name();
            const int nvars = static_cast<int>( guy.get_values().size() );
            const character_id npc_cid = guy.getID();
            std::string label = string_format( "%s (%d vars)##npc%d", nm, nvars, npc_idx );
            if( ImGui::TreeNode( label.c_str() ) ) {
                auto npc_provider = [npc_cid]( const std::string & k ) -> std::string {
                    npc *p = g->find_npc( npc_cid );
                    if( !p )
                    {
                        return "<gone>";
                    }
                    const global_variables::impl_t &m = p->get_values();
                    auto it = m.find( k );
                    return it == m.end() ? std::string( "<gone>" )
                    : it->second.to_string( true );
                };
                var_table_writer npc_writer{
                    [npc_cid]( const std::string & k, const diag_value & v )
                    {
                        if( npc *p = g->find_npc( npc_cid ) ) {
                            p->set_value( k, v );
                        }
                    },
                    [npc_cid]( const std::string & k )
                    {
                        if( npc *p = g->find_npc( npc_cid ) ) {
                            p->remove_value( k );
                        }
                    }
                };
                draw_var_table( string_format( "npc_vars_%d", npc_idx ).c_str(),
                                guy.get_values(), 120.0f,
                                string_format( "npc#%d", npc_cid.get_value() ),
                                npc_provider, npc_writer, var_browser );
                ImGui::TreePop();
            }
            npc_idx++;
        }
    }

    {
        int mon_idx = 0;
        for( monster &mon : g->all_monsters() ) {
            if( mon.get_values().empty() ) {
                continue;
            }
            const std::string nm = mon.get_name();
            const int nvars = static_cast<int>( mon.get_values().size() );
            std::string label = string_format( "%s (%d vars)##mon%d", nm, nvars, mon_idx );
            if( ImGui::TreeNode( label.c_str() ) ) {
                draw_var_table( string_format( "mon_vars_%d", mon_idx ).c_str(),
                                mon.get_values(), 120.0f,
                                std::string(), {}, {}, var_browser );
                ImGui::TreePop();
            }
            mon_idx++;
        }
    }
}

void tab_data_view::draw_timed_events_table()
{
    const std::list<timed_event> &events = get_timed_events().get_all();
    std::string header = string_format( _( "Timed Events (%d)##timed_events_header" ),
                                        static_cast<int>( events.size() ) );
    if( !ImGui::CollapsingHeader( header.c_str() ) ) {
        return;
    }

    ImGui::SetNextItemWidth( 200.0f );
    ImGui::InputTextWithHint( "##te_filter", _( "Filter (key or type)" ),
                              &timed_events.filter );
    ImGui::SameLine();
    add_monitor_button( "te_count_mon", "world:timed_event_count",
                        snap::world_timed_event_count() );

    if( events.empty() ) {
        ImGui::TextUnformatted( _( "No timed events." ) );
        return;
    }

    std::vector<const timed_event *> rows;
    rows.reserve( events.size() );
    for( const timed_event &te : events ) {
        if( !timed_events.filter.empty() ) {
            const std::string tn = timed_event_type_name( te.type );
            if( te.key.find( timed_events.filter ) == std::string::npos &&
                tn.find( timed_events.filter ) == std::string::npos ) {
                continue;
            }
        }
        rows.push_back( &te );
    }

    if( ImGui::BeginTable( "timed_events", 5,
                           ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                           ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit |
                           ImGuiTableFlags_NoSavedSettings |
                           ImGuiTableFlags_Sortable,
                           ImVec2( 0.0f, fit_table_height(
                                       static_cast<int>( rows.size() ), 240.0f ) ) ) ) {
        const uint64_t key = lk_str( lk_mix( 0u, rows.size() ), timed_events.filter );
        const std::vector<colspec<const timed_event *>> specs = {
            { _( "When" ), colw::fit, []( const timed_event *const & te ) { return to_string( te->when ); }, 0.0f, 1.0f, 0.0f, ImGuiTableColumnFlags_DefaultSort },
            {
                _( "Type" ), colw::fit, []( const timed_event *const & te )
                {
                    return std::string( timed_event_type_name( te->type ) );
                }
            },
            { _( "Key" ), colw::stretch },
            {
                _( "Strength" ), colw::fit, []( const timed_event *const & te )
                {
                    return std::to_string( te->strength );
                }
            },
            {
                _( "Faction" ), colw::fit, []( const timed_event *const & te )
                {
                    return std::to_string( te->faction_id );
                }
            },
        };
        setup_columns<const timed_event *>( "timed_events", key, specs, rows, true );
        ImGui::TableSetupScrollFreeze( 0, 1 );
        ImGui::TableHeadersRow();

        if( ImGuiTableSortSpecs *sort_specs = ImGui::TableGetSortSpecs() ) {
            if( sort_specs->SpecsCount > 0 ) {
                std::sort( rows.begin(), rows.end(),
                [&]( const timed_event * a, const timed_event * b ) {
                    for( int i = 0; i < sort_specs->SpecsCount; i++ ) {
                        const ImGuiTableColumnSortSpecs &s = sort_specs->Specs[i];
                        int cmp = 0;
                        switch( s.ColumnIndex ) {
                            case 0:
                                cmp = a->when < b->when ? -1
                                      : b->when < a->when ? 1 : 0;
                                break;
                            case 1:
                                cmp = std::strcmp( timed_event_type_name( a->type ),
                                                   timed_event_type_name( b->type ) );
                                break;
                            case 2:
                                cmp = a->key.compare( b->key );
                                break;
                            case 3:
                                cmp = a->strength - b->strength;
                                break;
                            case 4:
                                cmp = a->faction_id - b->faction_id;
                                break;
                            default:
                                break;
                        }
                        if( cmp != 0 ) {
                            return s.SortDirection == ImGuiSortDirection_Ascending
                                   ? cmp < 0 : cmp > 0;
                        }
                    }
                    return false;
                } );
            }
        }

        grow_columns<const timed_event *>( "timed_events", specs, rows, 0, static_cast<int>( rows.size() ),
                                           true );
        int row_id = 0;
        for( const timed_event *te : rows ) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushID( row_id++ );
            const std::string when_text = to_string( te->when );
            if( ImGui::Selectable( when_text.c_str(), false,
                                   ImGuiSelectableFlags_SpanAllColumns |
                                   ImGuiSelectableFlags_AllowOverlap ) ) {
                ImGui::SetClipboardText( te->key.c_str() );
            }
            if( ImGui::BeginPopupContextItem( "te_ctx" ) ) {
                if( ImGui::MenuItem( _( "Copy key" ) ) ) {
                    ImGui::SetClipboardText( te->key.c_str() );
                }
                if( ImGui::MenuItem( _( "Copy summary" ) ) ) {
                    ImGui::SetClipboardText( string_format(
                                                 "when=%s type=%s key=%s strength=%d faction=%d",
                                                 to_string( te->when ),
                                                 timed_event_type_name( te->type ),
                                                 te->key, te->strength, te->faction_id ).c_str() );
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted( timed_event_type_name( te->type ) );
            ImGui::TableNextColumn();
            ImGui::TextUnformatted( te->key.c_str() );
            ImGui::TableNextColumn();
            ImGui::Text( "%d", te->strength );
            ImGui::TableNextColumn();
            ImGui::Text( "%d", te->faction_id );
        }
        ImGui::EndTable();
    }
}

void tab_data_view::draw_item_wakeups_table()
{
    const std::vector<scheduled_wakeup_info> rows = get_item_wakeups().dump();
    std::string header = string_format(
                             _( "Item wakeups (%d)##item_wakeups_header" ),
                             static_cast<int>( rows.size() ) );
    if( !ImGui::CollapsingHeader( header.c_str() ) ) {
        return;
    }

    auto kind_name = []( item_wakeup_kind k ) {
        switch( k ) {
            case item_wakeup_kind::alarm:
                return "alarm";
            case item_wakeup_kind::env_check:
                return "env_check";
            case item_wakeup_kind::ready_check:
                return "ready_check";
            case item_wakeup_kind::fail_check:
                return "fail_check";
            case item_wakeup_kind::last:
                break;
        }
        return "?";
    };

    auto hint_text = []( const item_locator_hint & h ) -> std::string {
        switch( h.where )
        {
            case item_locator_hint::place::map:
                if( const auto *p = std::get_if<tripoint_abs_ms>( &h.location ) ) {
                    return string_format( "map@%s", p->to_string() );
                }
                return "map@?";
            case item_locator_hint::place::vehicle:
                if( const auto *v = std::get_if<vehicle_hint>( &h.location ) ) {
                    return string_format( "veh@%s p%d", v->cargo_square.to_string(),
                                          v->part_index );
                }
                return "veh@?";
            case item_locator_hint::place::character:
                if( const auto *c = std::get_if<character_id>( &h.location ) ) {
                    return string_format( "char#%d", c->get_value() );
                }
                return "char#?";
            case item_locator_hint::place::unknown:
                break;
        }
        return "?";
    };

    ImGui::SetNextItemWidth( 110.0f );
    static const char *const kind_choices[] = {
        "all", "alarm", "env_check", "ready_check", "fail_check"
    };
    const char *cur_kind = item_wakeups.kind_filter < 0 ? "all"
                           : kind_choices[item_wakeups.kind_filter + 1];
    if( ImGui::BeginCombo( _( "Kind##iw" ), cur_kind ) ) {
        if( ImGui::Selectable( "all", item_wakeups.kind_filter < 0 ) ) {
            item_wakeups.kind_filter = -1;
        }
        for( int i = 0; i < static_cast<int>( item_wakeup_kind::last ); i++ ) {
            if( ImGui::Selectable( kind_choices[i + 1],
                                   item_wakeups.kind_filter == i ) ) {
                item_wakeups.kind_filter = i;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth( 200.0f );
    ImGui::InputTextWithHint( "##iw_filter", _( "Filter (uid or hint)" ),
                              &item_wakeups.filter );
    ImGui::SameLine();
    add_monitor_button( "iw_count_mon", "world:item_wakeup_count",
                        snap::world_item_wakeup_count() );

    if( rows.empty() ) {
        ImGui::TextUnformatted( _( "No scheduled item wakeups." ) );
        const item_wakeup_manager::stats s0 = get_item_wakeups().get_stats();
        ImGui::TextDisabled( "pending=%d  stale=%d  dup=%d  dropped=%d",
                             s0.total_pending, s0.stale_resolution,
                             s0.duplicate_event, s0.dropped_on_load );
        return;
    }

    std::vector<const scheduled_wakeup_info *> filtered;
    filtered.reserve( rows.size() );
    for( const scheduled_wakeup_info &w : rows ) {
        if( item_wakeups.kind_filter >= 0
            && static_cast<int>( w.kind ) != item_wakeups.kind_filter ) {
            continue;
        }
        if( !item_wakeups.filter.empty() ) {
            const std::string uid_str = std::to_string( w.uid );
            const std::string where = hint_text( w.hint );
            if( uid_str.find( item_wakeups.filter ) == std::string::npos &&
                where.find( item_wakeups.filter ) == std::string::npos ) {
                continue;
            }
        }
        filtered.push_back( &w );
    }

    if( ImGui::BeginTable( "item_wakeups", 5,
                           ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                           ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit |
                           ImGuiTableFlags_NoSavedSettings,
                           ImVec2( 0.0f, fit_table_height(
                                       static_cast<int>( filtered.size() ), 240.0f ) ) ) ) {
        const uint64_t key = lk_mix( lk_str( lk_mix( 0u, filtered.size() ),
                                             item_wakeups.filter ),
                                     static_cast<uint64_t>( item_wakeups.kind_filter ) );
        const std::vector<colspec<const scheduled_wakeup_info *>> specs = {
            {
                _( "When" ), colw::fit, []( const scheduled_wakeup_info *const & w )
                {
                    return to_string( w->when );
                }
            },
            {
                _( "In" ), colw::fit, []( const scheduled_wakeup_info *const & w )
                {
                    return to_string( w->when - calendar::turn );
                }
            },
            {
                _( "UID" ), colw::fit, []( const scheduled_wakeup_info *const & w )
                {
                    return std::to_string( w->uid );
                }
            },
            {
                _( "Kind" ), colw::fit, [&]( const scheduled_wakeup_info *const & w )
                {
                    return std::string( kind_name( w->kind ) );
                }
            },
            { _( "Where" ), colw::stretch },
        };
        setup_columns<const scheduled_wakeup_info *>( "item_wakeups", key, specs, filtered );
        ImGui::TableSetupScrollFreeze( 0, 1 );
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin( static_cast<int>( filtered.size() ) );
        while( clipper.Step() ) {
            grow_columns<const scheduled_wakeup_info *>( "item_wakeups", specs, filtered, clipper.DisplayStart,
                    clipper.DisplayEnd );
            for( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ ) {
                const scheduled_wakeup_info &w = *filtered[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::PushID( i );
                const std::string when_text = to_string( w.when );
                if( ImGui::Selectable( when_text.c_str(), false,
                                       ImGuiSelectableFlags_SpanAllColumns |
                                       ImGuiSelectableFlags_AllowOverlap ) ) {
                    ImGui::SetClipboardText( std::to_string( w.uid ).c_str() );
                }
                if( ImGui::BeginPopupContextItem( "iw_ctx" ) ) {
                    if( ImGui::MenuItem( _( "Copy UID" ) ) ) {
                        ImGui::SetClipboardText( std::to_string( w.uid ).c_str() );
                    }
                    if( ImGui::MenuItem( _( "Copy where" ) ) ) {
                        ImGui::SetClipboardText( hint_text( w.hint ).c_str() );
                    }
                    ImGui::Separator();
                    if( ImGui::MenuItem( _( "Cancel wakeup" ) ) ) {
                        get_item_wakeups().cancel( w.uid, w.kind );
                    }
                    ImGui::EndPopup();
                }
                ImGui::PopID();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted( to_string( w.when - calendar::turn ).c_str() );
                ImGui::TableNextColumn();
                ImGui::Text( "%lld", static_cast<long long>( w.uid ) );
                ImGui::TableNextColumn();
                ImGui::TextUnformatted( kind_name( w.kind ) );
                ImGui::TableNextColumn();
                ImGui::TextUnformatted( hint_text( w.hint ).c_str() );
            }
        }
        ImGui::EndTable();
    }

    const item_wakeup_manager::stats s = get_item_wakeups().get_stats();
    ImGui::TextDisabled( "pending=%d  stale=%d  dup=%d  dropped=%d",
                         s.total_pending, s.stale_resolution,
                         s.duplicate_event, s.dropped_on_load );
}

void tab_data_view::draw_faction_browser()
{
    const auto &factions = g->faction_manager_ptr->all();
    std::string header = string_format( _( "Factions (%d)##factions_header" ),
                                        static_cast<int>( factions.size() ) );
    if( !ImGui::CollapsingHeader( header.c_str() ) ) {
        return;
    }

    ImGui::SetNextItemWidth( 200.0f );
    ImGui::InputTextWithHint( "##fac_filter", _( "Filter (id or name)" ),
                              &faction_browser.filter );
    ImGui::SameLine();
    ImGui::TextDisabled( "%s",
                         _( "Right-click row to copy or edit relations" ) );

    std::vector<std::pair<faction_id, const faction *>> rows;
    rows.reserve( factions.size() );
    for( const auto &[fid, fac] : factions ) {
        if( !faction_browser.filter.empty() ) {
            const std::string nm = fac.get_name();
            if( fid.str().find( faction_browser.filter ) == std::string::npos &&
                nm.find( faction_browser.filter ) == std::string::npos ) {
                continue;
            }
        }
        rows.emplace_back( fid, &fac );
    }

    using faction_pair = std::pair<faction_id, const faction *>;
    if( ImGui::BeginTable( "factions", 8,
                           ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                           ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit |
                           ImGuiTableFlags_NoSavedSettings |
                           ImGuiTableFlags_Sortable,
                           ImVec2( 0.0f, fit_table_height(
                                       static_cast<int>( rows.size() ), 240.0f ) ) ) ) {
        const uint64_t key = lk_str( lk_mix( 0u, rows.size() ), faction_browser.filter );
        const std::vector<colspec<faction_pair>> specs = {
            { _( "ID" ), colw::fit, []( const faction_pair & r ) { return r.first.str(); }, 0.0f, 1.0f, 0.25f, ImGuiTableColumnFlags_DefaultSort },
            { _( "Name" ), colw::stretch },
            {
                _( "Likes" ), colw::fit, []( const faction_pair & r )
                {
                    return std::to_string( r.second->likes_u );
                }
            },
            {
                _( "Respect" ), colw::fit, []( const faction_pair & r )
                {
                    return std::to_string( r.second->respects_u );
                }
            },
            {
                _( "Trust" ), colw::fit, []( const faction_pair & r )
                {
                    return std::to_string( r.second->trusts_u );
                }
            },
            {
                _( "Size" ), colw::fit, []( const faction_pair & r )
                {
                    return std::to_string( r.second->size );
                }
            },
            { _( "Food" ), colw::fit, []( const faction_pair & r ) { return r.second->food_supply_text(); }, 0.0f, 1.0f, 0.0f, ImGuiTableColumnFlags_NoSort },
            { _( "+M" ), colw::widget, {}, 40.0f, 1.0f, 0.0f, ImGuiTableColumnFlags_NoSort },
        };
        setup_columns<faction_pair>( "factions", key, specs, rows, true );
        ImGui::TableSetupScrollFreeze( 0, 1 );
        ImGui::TableHeadersRow();

        if( ImGuiTableSortSpecs *sort_specs = ImGui::TableGetSortSpecs() ) {
            if( sort_specs->SpecsCount > 0 ) {
                std::sort( rows.begin(), rows.end(),
                           [&]( const std::pair<faction_id, const faction *> &a,
                const std::pair<faction_id, const faction *> &b ) {
                    for( int i = 0; i < sort_specs->SpecsCount; i++ ) {
                        const ImGuiTableColumnSortSpecs &s = sort_specs->Specs[i];
                        int cmp = 0;
                        switch( s.ColumnIndex ) {
                            case 0:
                                cmp = a.first.str().compare( b.first.str() );
                                break;
                            case 1:
                                cmp = a.second->get_name().compare( b.second->get_name() );
                                break;
                            case 2:
                                cmp = a.second->likes_u - b.second->likes_u;
                                break;
                            case 3:
                                cmp = a.second->respects_u - b.second->respects_u;
                                break;
                            case 4:
                                cmp = a.second->trusts_u - b.second->trusts_u;
                                break;
                            case 5:
                                cmp = a.second->size - b.second->size;
                                break;
                            default:
                                break;
                        }
                        if( cmp != 0 ) {
                            return s.SortDirection == ImGuiSortDirection_Ascending
                                   ? cmp < 0 : cmp > 0;
                        }
                    }
                    return a.first.str().compare( b.first.str() ) < 0;
                } );
            }
        }

        grow_columns<faction_pair>( "factions", specs, rows, 0, static_cast<int>( rows.size() ), true );
        int row_id = 0;
        for( const auto &[fid, fac_ptr] : rows ) {
            const faction &fac = *fac_ptr;
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushID( row_id++ );
            if( ImGui::Selectable( fid.str().c_str(), false,
                                   ImGuiSelectableFlags_SpanAllColumns |
                                   ImGuiSelectableFlags_AllowOverlap ) ) {
                ImGui::SetClipboardText( fid.str().c_str() );
            }
            if( ImGui::BeginPopupContextItem( "fac_ctx" ) ) {
                if( ImGui::MenuItem( _( "Copy id" ) ) ) {
                    ImGui::SetClipboardText( fid.str().c_str() );
                }
                if( ImGui::MenuItem( _( "Copy name" ) ) ) {
                    ImGui::SetClipboardText( fac.get_name().c_str() );
                }
                ImGui::Separator();
                if( ImGui::MenuItem( _( "Edit relations" ) ) ) {
                    faction_browser.edit_fid = fid;
                    faction_browser.edit_likes = fac.likes_u;
                    faction_browser.edit_respects = fac.respects_u;
                    faction_browser.edit_trusts = fac.trusts_u;
                    ImGui::OpenPopup( "fac_edit_popup" );
                }
                if( ImGui::MenuItem( _( "Reset relations to 0" ) ) ) {
                    if( faction *f = g->faction_manager_ptr->get( fid, false ) ) {
                        f->likes_u = 0;
                        f->respects_u = 0;
                        f->trusts_u = 0;
                    }
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
            ImGui::TableNextColumn();
            cataimgui::draw_colored_text( fac.get_name(), c_white );
            ImGui::TableNextColumn();
            ImGui::TextColored( relation_color( fac.likes_u ), "%d", fac.likes_u );
            ImGui::TableNextColumn();
            ImGui::TextColored( relation_color( fac.respects_u ), "%d", fac.respects_u );
            ImGui::TableNextColumn();
            ImGui::TextColored( relation_color( fac.trusts_u ), "%d", fac.trusts_u );
            ImGui::TableNextColumn();
            ImGui::Text( "%d", fac.size );
            ImGui::TableNextColumn();
            ImGui::TextUnformatted( fac.food_supply_text().c_str() );
            ImGui::TableNextColumn();
            add_monitor_button(
                ( "fac_mon_" + fid.str() ).c_str(),
                "faction:" + fid.str(),
                snap::faction_stats( fid ) );
        }
        ImGui::EndTable();
    }

    if( !faction_browser.edit_fid.is_empty() ) {
        ImGui::SetNextWindowSize( ImVec2( 320.0f, 0.0f ) );
        if( ImGui::BeginPopupModal( "fac_edit_popup", nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize ) ) {
            ImGui::TextDisabled( "%s", faction_browser.edit_fid.str().c_str() );
            ImGui::SetNextItemWidth( 220.0f );
            ImGui::InputInt( _( "Likes##fac_edit" ), &faction_browser.edit_likes );
            ImGui::SetNextItemWidth( 220.0f );
            ImGui::InputInt( _( "Respect##fac_edit" ), &faction_browser.edit_respects );
            ImGui::SetNextItemWidth( 220.0f );
            ImGui::InputInt( _( "Trust##fac_edit" ), &faction_browser.edit_trusts );
            if( ImGui::Button( _( "Save##fac_edit" ) ) ) {
                if( faction *f = g->faction_manager_ptr->get( faction_browser.edit_fid,
                                 false ) ) {
                    f->likes_u = faction_browser.edit_likes;
                    f->respects_u = faction_browser.edit_respects;
                    f->trusts_u = faction_browser.edit_trusts;
                }
                faction_browser.edit_fid = faction_id::NULL_ID();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if( ImGui::Button( _( "Cancel##fac_edit" ) ) ) {
                faction_browser.edit_fid = faction_id::NULL_ID();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}


const char *tab_trace_view::label() const
{
    return _( "Trace" );
}

void tab_trace_view::load_state( const JsonObject &nested )
{
    nested.read( "filter", trace_filter );
    nested.read( "show_log", trace_show_log );
    nested.read( "show_event", trace_show_event );
    nested.read( "show_monitor", trace_show_monitor );
    nested.read( "autoscroll", trace_autoscroll );
    nested.read( "monitor_target_input", monitor_target_input );
    nested.read( "monitor_target_type", monitor_target_type );
    nested.read( "monitor_mode_idx", monitor_mode_idx );
    uint64_t mask_bits = 0;
    if( nested.read( "log_category_mask", mask_bits ) ) {
        for( size_t i = 0; i < log_category_mask.size() && i < 64; ++i ) {
            log_category_mask.set( i, ( mask_bits >> i ) & 1ULL );
        }
    }
}

void tab_trace_view::save_state( JsonOut &jo ) const
{
    jo.member( "filter", trace_filter );
    jo.member( "show_log", trace_show_log );
    jo.member( "show_event", trace_show_event );
    jo.member( "show_monitor", trace_show_monitor );
    jo.member( "autoscroll", trace_autoscroll );
    jo.member( "monitor_target_input", monitor_target_input );
    jo.member( "monitor_target_type", monitor_target_type );
    jo.member( "monitor_mode_idx", monitor_mode_idx );
    uint64_t mask_bits = 0;
    for( size_t i = 0; i < log_category_mask.size() && i < 64; ++i ) {
        if( log_category_mask.test( i ) ) {
            mask_bits |= ( 1ULL << i );
        }
    }
    jo.member( "log_category_mask", mask_bits );
}

void tab_trace_view::draw_body( debug_console &host )
{
    debug_capture &cap = debug_capture::instance();
    capture_settings &s = cap.settings();

    // Collapsed by default so the entries feed below gets most of the height.
    if( ImGui::CollapsingHeader( _( "Capture##trace" ) ) ) {
        ImGui::PushID( "trace_capture" );
        ImGui::Indent();
        ImGui::Checkbox( _( "Capture" ), &s.main_enabled );
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Main capture gate (off freezes everything)" ) );
        }
        ImGui::SameLine();
        ImGui::TextDisabled( "|" );
        ImGui::SameLine();
        ImGui::BeginDisabled( !s.main_enabled );
        ImGui::Checkbox( _( "add_msg_debug" ), &s.add_msg_debug_capture );
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Capture add_msg_debug lines (needs debug_mode + filters)" ) );
        }
        ImGui::SameLine();
        const std::string filt_preview = string_format(
                                             _( "filters: %d / %d" ),
                                             static_cast<int>( debugmode::enabled_filters.size() ),
                                             static_cast<int>( debugmode::DF_LAST ) );
        ImGui::SetNextItemWidth( 180.0f );
        if( ImGui::BeginCombo( "##dbgf_combo", filt_preview.c_str() ) ) {
            if( ImGui::SmallButton( _( "All##dbgf" ) ) ) {
                for( int i = 0; i < static_cast<int>( debugmode::DF_LAST ); i++ ) {
                    debugmode::enabled_filters.insert(
                        static_cast<debugmode::debug_filter>( i ) );
                }
            }
            ImGui::SameLine();
            if( ImGui::SmallButton( _( "None##dbgf" ) ) ) {
                debugmode::enabled_filters.clear();
            }
            ImGui::Separator();
            for( int i = 0; i < static_cast<int>( debugmode::DF_LAST ); i++ ) {
                const debugmode::debug_filter cat =
                    static_cast<debugmode::debug_filter>( i );
                bool on = debugmode::enabled_filters.count( cat ) > 0;
                if( ImGui::Checkbox( debugmode::filter_name( cat ).c_str(), &on ) ) {
                    if( on ) {
                        debugmode::enabled_filters.insert( cat );
                    } else {
                        debugmode::enabled_filters.erase( cat );
                    }
                }
            }
            ImGui::EndCombo();
        }
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Per-category gate for add_msg_debug emission" ) );
        }
        ImGui::SameLine();
        ImGui::Checkbox( _( "event_bus" ), &s.event_bus_capture );
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Capture every event_bus emission (no per-category gate)" ) );
        }
        ImGui::SameLine();
        ImGui::Checkbox( _( "EOC fire trace" ), &s.eoc_trace_capture );
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Capture EOC activations with success, depth and timing" ) );
        }
        if( !debug_mode ) {
            ImGui::TextColored( ImVec4( 0.95f, 0.7f, 0.4f, 1.0f ), "%s",
                                _( "Debug mode off; add_msg_debug capture inert" ) );
        }

        ImGui::Spacing();
        ImGui::Checkbox( _( "Store on disk" ), &s.trace_file.enabled );
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Mirror every captured line to a file on disk" ) );
        }
        ImGui::SameLine();
        const char *const fmt_items[] = { "JSONL", "CSV" };
        int fmt_idx = static_cast<int>( s.trace_file.format );
        ImGui::SetNextItemWidth( 80.0f );
        if( ImGui::Combo( "##disk_fmt", &fmt_idx, fmt_items, IM_ARRAYSIZE( fmt_items ) ) ) {
            const capture_format new_fmt = fmt_idx == 1
                                           ? capture_format::csv
                                           : capture_format::jsonl;
            if( new_fmt != s.trace_file.format ) {
                s.trace_file.format = new_fmt;
                const std::string desired = new_fmt == capture_format::csv
                                            ? ".csv"
                                            : ".jsonl";
                const std::string &p = s.trace_file.path;
                const size_t dot = p.find_last_of( '.' );
                const size_t slash = p.find_last_of( '/' );
                if( dot != std::string::npos
                    && ( slash == std::string::npos || dot > slash ) ) {
                    s.trace_file.path = p.substr( 0, dot ) + desired;
                } else if( !p.empty() ) {
                    s.trace_file.path = p + desired;
                }
            }
        }
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "On-disk format (auto-renames extension)" ) );
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth( 240.0f );
        ImGui::InputTextWithHint( "##jsonl_path", _( "path" ), &s.trace_file.path );
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Output path (relative to game working directory)" ) );
        }
        ImGui::SameLine();
        ImGui::Checkbox( _( "auto-flush" ), &s.trace_file.auto_flush );
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Auto-flush each batch (off keeps buffered)" ) );
        }
        ImGui::SameLine();
        if( ImGui::SmallButton( _( "Flush" ) ) ) {
            cap.flush_trace_file();
        }
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Flush pending lines to disk now" ) );
        }

        ImGui::Spacing();
        const float ring_w = 130.0f;
        int log_n = static_cast<int>( s.log_ring_size );
        ImGui::SetNextItemWidth( ring_w );
        if( ImGui::SliderInt( "##ring_log", &log_n, 100, 50000, "Log %d" ) ) {
            cap.resize_log_ring( static_cast<size_t>( log_n ) );
        }
        ImGui::SameLine();
        if( ImGui::SmallButton( _( "Clr##log" ) ) ) {
            cap.clear_logs();
        }
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s", _( "Drop every entry from the captured-log ring" ) );
        }
        ImGui::SameLine();
        int ev_n = static_cast<int>( s.event_ring_size );
        ImGui::SetNextItemWidth( ring_w );
        if( ImGui::SliderInt( "##ring_ev", &ev_n, 100, 50000, "Event %d" ) ) {
            cap.resize_event_ring( static_cast<size_t>( ev_n ) );
        }
        ImGui::SameLine();
        if( ImGui::SmallButton( _( "Clr##ev" ) ) ) {
            cap.clear_events();
        }
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s", _( "Drop every entry from the captured-event ring" ) );
        }
        ImGui::SameLine();
        int eoc_n = static_cast<int>( s.eoc_trace_ring_size );
        ImGui::SetNextItemWidth( ring_w );
        if( ImGui::SliderInt( "##ring_eoc", &eoc_n, 100, 50000, "EOC %d" ) ) {
            cap.resize_eoc_trace_ring( static_cast<size_t>( eoc_n ) );
        }
        ImGui::SameLine();
        if( ImGui::SmallButton( _( "Clr##eoc" ) ) ) {
            cap.clear_eoc_traces();
        }
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s", _( "Drop every entry from the captured-EOC-trace ring" ) );
        }

        ImGui::EndDisabled();

        const size_t mon_count = cap.monitors().size();
        const std::string mon_header = string_format(
                                           _( "Monitors (%d)##cap_mon_header" ),
                                           static_cast<int>( mon_count ) );
        ImGui::SetNextItemOpen( mon_count > 0, ImGuiCond_Once );
        if( ImGui::CollapsingHeader( mon_header.c_str() ) ) {
            draw_monitors_body();
        }
        ImGui::Unindent();
        ImGui::PopID();
    }

    framed_section( "trace_view", _( "View" ), [&]() {
        ImGui::Checkbox( _( "Log" ), &trace_show_log );
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s", _( "Show captured debug log lines in the feed" ) );
        }
        ImGui::SameLine();
        ImGui::Checkbox( _( "Event" ), &trace_show_event );
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s", _( "Show captured event_bus events in the feed" ) );
        }
        ImGui::SameLine();
        bool eoc_visible = host.eoc_trace_visible();
        if( ImGui::Checkbox( _( "EOC" ), &eoc_visible ) ) {
            host.set_eoc_trace_visible( eoc_visible );
        }
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s", _( "Show captured EOC fire traces in the feed" ) );
        }
        ImGui::SameLine();
        ImGui::Checkbox( _( "Monitor" ), &trace_show_monitor );
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Show monitor snapshots in the feed (independent from the Log toggle)" ) );
        }
        ImGui::SameLine();
        ImGui::Checkbox( _( "Auto-scroll" ), &trace_autoscroll );
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Snap to bottom on new rows (disable to read history)" ) );
        }
        ImGui::SameLine();
        int active_cats = 0;
        for( int i = 0; i < static_cast<int>( debugmode::DF_LAST ); i++ ) {
            if( log_category_mask.test( i ) ) {
                active_cats++;
            }
        }
        const std::string cat_preview = string_format(
                                            _( "cats: %d / %d" ), active_cats,
                                            static_cast<int>( debugmode::DF_LAST ) );
        ImGui::SetNextItemWidth( 160.0f );
        if( ImGui::BeginCombo( "##trace_cats_combo", cat_preview.c_str() ) ) {
            if( ImGui::SmallButton( _( "All##trace_cat_all" ) ) ) {
                log_category_mask.set();
            }
            ImGui::SameLine();
            if( ImGui::SmallButton( _( "None##trace_cat_none" ) ) ) {
                log_category_mask.reset();
            }
            ImGui::Separator();
            for( int i = 0; i < static_cast<int>( debugmode::DF_LAST ); i++ ) {
                const debugmode::debug_filter cat =
                    static_cast<debugmode::debug_filter>( i );
                bool on = log_category_mask.test( i );
                if( ImGui::Checkbox( debugmode::filter_name( cat ).c_str(), &on ) ) {
                    log_category_mask.set( i, on );
                }
            }
            ImGui::EndCombo();
        }
        if( ImGui::IsItemHovered() ) {
            ImGui::SetTooltip( "%s",
                               _( "Display-side category gate (capture is not affected)" ) );
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth( -1.0f );
        ImGui::InputTextWithHint( "##trace_filter", _( "filter (label or body)" ),
                                  &trace_filter );
    } );

    const bool eoc_visible = host.eoc_trace_visible();
    const uint8_t now_mask = ( trace_show_log ? 1 : 0 ) |
                             ( trace_show_event ? 2 : 0 ) |
                             ( trace_show_monitor ? 4 : 0 );
    const uint64_t now_gen = cap.feed_generation();
    const bool need_rebuild = now_gen != cached_feed_gen
                              || now_mask != cached_toggle_mask
                              || eoc_visible != cached_eoc_visible
                              || trace_filter != cached_filter
                              || log_category_mask != cached_log_mask;
    if( need_rebuild ) {
        cached_rows.clear();
        auto match_filter = [this]( std::string_view label, std::string_view body ) {
            if( trace_filter.empty() ) {
                return true;
            }
            return label.find( trace_filter ) != std::string_view::npos
                   || body.find( trace_filter ) != std::string_view::npos;
        };
        if( trace_show_log || trace_show_monitor ) {
            for( const debug_log_entry &e : cap.logs() ) {
                const bool is_monitor = e.category == debugmode::DF_MONITOR;
                if( is_monitor ? !trace_show_monitor : !trace_show_log ) {
                    continue;
                }
                if( !is_monitor && !log_category_mask.test( e.category ) ) {
                    continue;
                }
                trace_row r;
                r.turn = e.turn_num;
                r.source = 0;
                r.label = debugmode::filter_name( e.category );
                r.body = e.text;
                r.log_cat = e.category;
                if( match_filter( r.label, r.body ) ) {
                    cached_rows.push_back( std::move( r ) );
                }
            }
        }
        if( trace_show_event ) {
            for( const event_log_entry &e : cap.events() ) {
                trace_row r;
                r.turn = e.turn_num;
                r.source = 1;
                r.label = std::to_string( e.event_type_idx );
                r.body = e.fields_oneline;
                if( match_filter( r.label, r.body ) ) {
                    cached_rows.push_back( std::move( r ) );
                }
            }
        }
        if( eoc_visible ) {
            for( const eoc_trace_entry &t : cap.eoc_traces() ) {
                trace_row r;
                r.turn = t.turn_num;
                r.source = 2;
                r.label = t.eoc_id;
                r.body = string_format( "%s us=%ld depth=%d",
                                        t.success ? "ok" : "fail", t.us, t.depth );
                if( match_filter( r.label, r.body ) ) {
                    cached_rows.push_back( std::move( r ) );
                }
            }
        }
        std::stable_sort( cached_rows.begin(), cached_rows.end(),
        []( const trace_row & a, const trace_row & b ) {
            return a.turn < b.turn;
        } );
        cached_feed_gen = now_gen;
        cached_toggle_mask = now_mask;
        cached_eoc_visible = eoc_visible;
        cached_filter = trace_filter;
        cached_log_mask = log_category_mask;
    }
    const std::vector<trace_row> &rows = cached_rows;

    static const char *const source_names[] = { "Log", "Event", "EOC" };
    static const ImVec4 source_colors[] = {
        ImVec4( 0.6f, 0.9f, 1.0f, 1.0f ),
        ImVec4( 0.7f, 1.0f, 0.7f, 1.0f ),
        ImVec4( 1.0f, 0.8f, 0.5f, 1.0f ),
    };

    {
        std::vector<export_column<trace_row>> cols = {
            {
                "Turn", "turn", []( const trace_row & r ) { return std::to_string( r.turn ); },
                []( const trace_row & r )
                {
                    return std::to_string( r.turn );
                }
            },
            {
                "Source", "src",
                []( const trace_row & r ) { return std::string( source_names[r.source] ); },
                []( const trace_row & r )
                {
                    return "\"" + std::string( source_names[r.source] ) + "\"";
                }
            },
            {
                "Label", "label", []( const trace_row & r ) { return r.label; },
                []( const trace_row & r )
                {
                    return "\"" + capture_json_escape( r.label ) + "\"";
                }
            },
            {
                "Detail", "body", []( const trace_row & r ) { return r.body; },
                []( const trace_row & r )
                {
                    return "\"" + capture_json_escape( r.body ) + "\"";
                }
            },
        };
        export_rows( host, "trace", "trace", rows, cols );
    }

    label_value( _( "Entries" ), "%d", static_cast<int>( rows.size() ) );

    if( ImGui::BeginTable( "trace_feed", 4,
                           ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                           ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit |
                           ImGuiTableFlags_NoSavedSettings,
                           ImVec2( 0.0f, ImGui::GetContentRegionAvail().y ) ) ) {
        uint64_t key = lk_mix( now_gen, rows.size() );
        key = lk_mix( key, now_mask );
        key = lk_mix( key, eoc_visible ? 1u : 0u );
        key = lk_mix( key, log_category_mask.to_ullong() );
        key = lk_str( key, trace_filter );
        const std::vector<colspec<trace_row>> specs = {
            {
                _( "Turn" ), colw::fit, []( const trace_row & r )
                {
                    return std::to_string( r.turn );
                }
            },
            {
                _( "Src" ), colw::fit, []( const trace_row & r )
                {
                    return std::string( source_names[r.source] );
                }
            },
            { _( "Label" ), colw::fit, []( const trace_row & r ) { return r.label; }, 0.0f, 1.0f, 0.25f },
            { _( "Detail" ), colw::stretch },
        };
        setup_columns<trace_row>( "trace_feed", key, specs, rows );
        ImGui::TableSetupScrollFreeze( 0, 1 );
        ImGui::TableHeadersRow();
        ImGuiListClipper clipper;
        clipper.Begin( static_cast<int>( rows.size() ) );
        while( clipper.Step() ) {
            grow_columns<trace_row>( "trace_feed", specs, rows, clipper.DisplayStart, clipper.DisplayEnd );
            for( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ ) {
                const trace_row &r = rows[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text( "%d", r.turn );
                ImGui::TableNextColumn();
                ImGui::TextColored( source_colors[r.source], "%s",
                                    source_names[r.source] );
                ImGui::TableNextColumn();
                const float label_cw = ImGui::GetContentRegionAvail().x;
                if( r.source == 0 && r.log_cat >= 0 ) {
                    ImGui::TextColored( category_color( r.log_cat ), "%s", r.label.c_str() );
                } else {
                    ImGui::TextUnformatted( r.label.c_str() );
                }
                clip_tooltip( r.label, label_cw );
                ImGui::TableNextColumn();
                cataimgui::draw_colored_text( r.body, c_white );
            }
        }
        if( trace_autoscroll && rows.size() > trace_prev_row_count ) {
            ImGui::SetScrollHereY( 1.0f );
        }
        trace_prev_row_count = rows.size();
        ImGui::EndTable();
    }
}

void tab_trace_view::draw_monitors_body()
{
    ImGui::TextWrapped( "%s", _(
                            "Snapshots fire every turn / on change / manually and are written to the "
                            "Trace tab under the DF_MONITOR category (and JSONL when enabled)." ) );

    ImGui::SeparatorText( _( "Add monitor" ) );

    // Table-driven dispatch: each entry carries the combo label, input
    // hint, optional validator, and snap-closure factory.
    const std::vector<monitor_target_entry> &targets = all_monitor_targets();
    const int n_targets = static_cast<int>( targets.size() );
    monitor_target_type = std::clamp( monitor_target_type, 0, n_targets - 1 );

    // ImGui::Combo wants a raw const char ** of options. Build a per-frame
    // label vector from the table; the table itself is rebuilt only once.
    std::vector<const char *> combo_labels;
    combo_labels.reserve( targets.size() );
    for( const monitor_target_entry &e : targets ) {
        combo_labels.push_back( e.label );
    }
    ImGui::SetNextItemWidth( 320 );
    ImGui::Combo( _( "Target##mon_target" ), &monitor_target_type,
                  combo_labels.data(), n_targets );

    const monitor_target_entry &target = targets[monitor_target_type];
    const bool needs_input = target.hint[0] != '\0';

    ImGui::SetNextItemWidth( 240 );
    ImGui::InputTextWithHint( "##mon_input",
                              needs_input ? target.hint : _( "(no input)" ),
                              &monitor_target_input,
                              needs_input ? 0 : ImGuiInputTextFlags_ReadOnly );
    ImGui::SameLine();
    ImGui::SetNextItemWidth( 200 );
    ImGui::InputTextWithHint( "##mon_label", _( "label…" ), &monitor_label );
    ImGui::SameLine();
    static const char *const mode_labels[] = { "every_turn", "on_change", "manual" };
    ImGui::SetNextItemWidth( 100 );
    ImGui::Combo( "##mon_mode", &monitor_mode_idx, mode_labels,
                  IM_ARRAYSIZE( mode_labels ) );
    ImGui::SameLine();
    const bool input_present = !needs_input || !monitor_target_input.empty();
    const bool input_valid = target.validate == nullptr ||
                             target.validate( monitor_target_input );
    ImGui::BeginDisabled( monitor_label.empty() || !input_present ||
                          !input_valid );
    const bool add_clicked = ImGui::Button( _( "Add" ) );
    ImGui::EndDisabled();
    if( add_clicked ) {
        const monitor_mode mode = static_cast<monitor_mode>( monitor_mode_idx );
        std::function<std::string()> snap = target.make_snap( monitor_target_input );
        if( snap ) {
            debug_capture::instance().add_monitor(
                monitor_label, std::move( snap ), mode );
            monitor_label.clear();
            monitor_target_input.clear();
        }
    }

    ImGui::SeparatorText( _( "Active monitors" ) );
    std::vector<monitor_entry> &mons = debug_capture::instance().monitors();
    if( mons.empty() ) {
        ImGui::TextDisabled( "%s", _( "(none)" ) );
    } else if( ImGui::BeginTable( "monitors_table", 6,
                                  ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                                  ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_SizingStretchProp ) ) {
        fit_col( _( "On" ) );
        ImGui::TableSetupColumn( _( "Label" ) );
        fit_col( _( "Mode" ) );
        ImGui::TableSetupColumn( _( "Last snapshot" ) );
        fit_col( _( "Snap" ) );
        fit_col( _( "Remove" ) );
        ImGui::TableHeadersRow();
        int remove_id = -1;
        int snap_id = -1;
        const int now_turn = to_turns<int>( calendar::turn - calendar::turn_zero );
        for( monitor_entry &m : mons ) {
            ImGui::PushID( m.id );
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Checkbox( "##en", &m.enabled );
            ImGui::TableNextColumn();
            ImGui::TextUnformatted( m.label.c_str() );
            ImGui::TableNextColumn();
            ImGui::TextUnformatted( mode_labels[static_cast<int>( m.mode )] );
            ImGui::TableNextColumn();
            ImGui::TextUnformatted( m.last_snapshot.c_str() );
            if( m.mode != monitor_mode::manual && m.last_snapshot_turn >= 0 ) {
                const int stale = now_turn - m.last_snapshot_turn;
                if( stale > 10 ) {
                    ImGui::SameLine();
                    ImGui::TextColored( ImVec4( 0.95f, 0.7f, 0.4f, 1.0f ),
                                        " (stale %d)", stale );
                }
            }
            ImGui::TableNextColumn();
            if( ImGui::SmallButton( "Snap" ) ) {
                snap_id = m.id;
            }
            if( ImGui::IsItemHovered() ) {
                ImGui::SetTooltip( "%s",
                                   _( "Take a snapshot of this monitor right now and log it" ) );
            }
            ImGui::TableNextColumn();
            if( ImGui::SmallButton( "Remove" ) ) {
                remove_id = m.id;
            }
            if( ImGui::IsItemHovered() ) {
                ImGui::SetTooltip( "%s",
                                   _( "Delete this monitor permanently from the registry" ) );
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
        if( snap_id >= 0 ) {
            debug_capture::instance().snapshot_monitor( snap_id );
        }
        if( remove_id >= 0 ) {
            debug_capture::instance().remove_monitor( remove_id );
        }
    }
}


std::vector<tab_registry_view_entry> tab_registry_snapshot()
{
    const std::vector<tab_descriptor> &reg = tab_registry();
    std::vector<tab_registry_view_entry> out;
    out.reserve( reg.size() );
    for( const tab_descriptor &d : reg ) {
        out.push_back( { d.id_string, d.categories } );
    }
    return out;
}

void framed_section( const char *id, const char *label,
                     const std::function<void()> &body )
{
    ImGui::SeparatorText( label );
    ImGui::PushID( id );
    ImGui::Indent();
    body();
    ImGui::Unindent();
    ImGui::PopID();
}

float fit_table_height( int n_rows, float cap_h )
{
    const float row_h = ImGui::GetTextLineHeightWithSpacing();
    const float header_h = ImGui::GetFrameHeightWithSpacing();
    const float content_h = header_h
                            + row_h * static_cast<float>( std::max( 1, n_rows ) )
                            + 4.0f;
    return std::min( content_h, cap_h );
}

void fit_col( const char *label, ImGuiTableColumnFlags extra_flags )
{
    // init width 0 => ImGui auto-sizes the fixed column to the wider of its
    // header and its submitted (visible, under a clipper) cells.
    ImGui::TableSetupColumn( label, ImGuiTableColumnFlags_WidthFixed | extra_flags, 0.0f );
}

void scalar_slider_table( const char *id, const std::vector<slider_row> &rows,
                          int pairs_per_row )
{
    if( rows.empty() || pairs_per_row < 1 ) {
        return;
    }
    // Shared label-column width so all slider tracks start at the same x.
    float label_w = 0.0f;
    for( const slider_row &r : rows ) {
        label_w = std::max( label_w, ImGui::CalcTextSize( r.label.c_str() ).x );
    }
    label_w += 8.0f;
    ImGui::PushID( id );
    if( ImGui::BeginTable( "scalar_sliders", pairs_per_row * 2,
                           ImGuiTableFlags_SizingStretchProp ) ) {
        for( int p = 0; p < pairs_per_row; p++ ) {
            ImGui::TableSetupColumn( "l", ImGuiTableColumnFlags_WidthFixed, label_w );
            ImGui::TableSetupColumn( "s", ImGuiTableColumnFlags_WidthStretch );
        }
        for( size_t i = 0; i < rows.size(); i++ ) {
            if( i % static_cast<size_t>( pairs_per_row ) == 0 ) {
                ImGui::TableNextRow();
            }
            const slider_row &r = rows[i];
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted( r.label.c_str() );
            ImGui::TableNextColumn();
            int v = r.get();
            ImGui::PushID( static_cast<int>( i ) );
            ImGui::SetNextItemWidth( -1.0f );
            if( ImGui::SliderInt( "##v", &v, r.lo, r.hi ) ) {
                r.set( v );
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::PopID();
}

void add_monitor_button( const char *id, const std::string &label,
                         std::function<std::string()> snap,
                         monitor_mode mode )
{
    ImGui::PushID( id );
    const bool clicked = ImGui::SmallButton( "+M" );
    if( ImGui::IsItemHovered() ) {
        ImGui::SetTooltip( "%s",
                           _( "Add monitor (visible in Trace > Monitors)" ) );
    }
    if( clicked ) {
        debug_capture::instance().add_monitor( label, std::move( snap ), mode );
        debug_capture::instance().push_debug_log(
            debugmode::DF_MONITOR, "added monitor " + label );
    }
    ImGui::PopID();
}

ImVec4 category_color( int cat )
{
    const float golden = 137.508f;
    float hue = std::fmod( static_cast<float>( cat ) * golden, 360.0f ) / 360.0f;
    float r = 0.0f;
    float gv = 0.0f;
    float b = 0.0f;
    ImGui::ColorConvertHSVtoRGB( hue, 0.55f, 0.95f, r, gv, b );
    return ImVec4( r, gv, b, 1.0f );
}

int fuzzy_score( std::string_view haystack, std::string_view needle )
{
    if( needle.empty() ) {
        return 1;
    }
    std::string h;
    h.reserve( haystack.size() );
    for( char c : haystack ) {
        h.push_back( static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) ) );
    }
    std::string n;
    n.reserve( needle.size() );
    for( char c : needle ) {
        n.push_back( static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) ) );
    }

    size_t ni = 0;
    int score = 0;
    int run = 0;
    int last_idx = -2;
    for( size_t i = 0; i < h.size() && ni < n.size(); i++ ) {
        if( h[i] == n[ni] ) {
            ni++;
            if( static_cast<int>( i ) == last_idx + 1 ) {
                run++;
                score += 3 + run;
            } else {
                run = 0;
                score += 1;
            }
            if( i == 0 ) {
                score += 2;
            }
            last_idx = static_cast<int>( i );
        }
    }
    if( ni < n.size() ) {
        return 0;
    }
    return score;
}

} // namespace debug_menu
