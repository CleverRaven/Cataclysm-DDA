#pragma once
#ifndef CATA_SRC_DEBUG_CAPTURE_H
#define CATA_SRC_DEBUG_CAPTURE_H

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

class JsonObject;
class JsonOut;

namespace cata
{
class event;
} // namespace cata

class event_bus;

namespace debugmode
{
enum debug_filter : int;
} // namespace debugmode

namespace debug_menu
{

struct debug_log_entry {
    int turn_num;
    debugmode::debug_filter category;
    std::string text;
};

struct event_log_entry {
    int turn_num;
    int event_type_idx;
    std::string fields_oneline;
};

struct eoc_trace_entry {
    // Field order packs the 8/4/4/1-byte members after the string to minimize
    // padding.
    std::string eoc_id;
    int64_t us;
    int turn_num;
    int depth;
    bool success;
};

// JSON-escape a string for safe inclusion as a JSON string body (no surrounding quotes).
std::string capture_json_escape( std::string_view s );

enum class monitor_mode {
    every_turn,
    on_change,
    manual,
};

struct monitor_entry {
    int id;
    std::string label;
    // Snapshot returns a short human-readable string for the watched target.
    std::function<std::string()> snapshot;
    monitor_mode mode = monitor_mode::on_change;
    std::string last_snapshot;
    bool enabled = true;
    // Turn of last snapshot. UI flags stale monitors via this vs current turn.
    int last_snapshot_turn = -1;
};

enum class capture_format : int {
    jsonl = 0,
    csv = 1,
};

struct trace_file_settings {
    bool enabled = false;
    bool auto_flush = true;
    capture_format format = capture_format::jsonl;
    std::string path = "config/debug_trace.jsonl";
    size_t rotate_mib = 50;
    std::set<std::string> sources = { "events", "eoc", "monitors", "log" };

    // Whether this source should currently be written to the trace file.
    bool wants( const std::string &src ) const {
        return enabled && sources.count( src ) != 0;
    }
};

struct capture_settings {
    size_t log_ring_size = 2000;
    size_t event_ring_size = 5000;
    size_t eoc_trace_ring_size = 5000;

    // Main gate. When false, push_* and tick_monitors short-circuit
    // (freeze capture without flipping every sub-toggle).
    bool main_enabled = true;
    bool add_msg_debug_capture = true;
    bool event_bus_capture = false;
    bool eoc_trace_capture = false;

    trace_file_settings trace_file;
};

class debug_capture
{
    public:
        static debug_capture &instance();
        static bool is_initialized();

        capture_settings &settings();
        const capture_settings &settings() const;

        void on_game_load( ::event_bus &bus );
        void on_game_shutdown();
        void flush_trace_file();

        int add_monitor( std::string label, std::function<std::string()> snap,
                         monitor_mode mode );
        void remove_monitor( int id );
        void snapshot_monitor( int id );
        std::vector<monitor_entry> &monitors();
        void tick_monitors();

        void push_debug_log( debugmode::debug_filter type, const std::string &msg );
        void push_event( const cata::event &e );
        void push_eoc_trace( const std::string &eoc_id, bool success,
                             std::chrono::microseconds us, int depth );

        const std::deque<debug_log_entry> &logs() const;
        const std::deque<event_log_entry> &events() const;
        const std::deque<eoc_trace_entry> &eoc_traces() const;

        void clear_logs();
        void clear_events();
        void clear_eoc_traces();

        void resize_log_ring( size_t n );
        void resize_event_ring( size_t n );
        void resize_eoc_trace_ring( size_t n );

        // Serialize / restore the user-facing capture toggles, ring sizes,
        // and JSONL writer config into a flat object the caller wraps in
        // its own "capture" member. Ring sizes route through the
        // resize_*_ring helpers so the active rings re-trim on load.
        void save_settings( JsonOut &jo ) const;
        void load_settings( const JsonObject &jo );

        // Monotonic counter bumped on any push/clear/resize. Safe under
        // ring rotation: size() can stay identical while content
        // changes, so consumers must compare this counter, not sizes.
        uint64_t feed_generation() const noexcept;

        static void tick_if_active();

        // Fast `eoc_trace_capture` flag check without exposing the
        // settings struct to the call site.
        static bool is_eoc_tracing();

        debug_capture( const debug_capture & ) = delete;
        debug_capture &operator=( const debug_capture & ) = delete;
        debug_capture( debug_capture && ) = delete;
        debug_capture &operator=( debug_capture && ) = delete;

    private:
        debug_capture();
        ~debug_capture();

        struct impl;
        std::unique_ptr<impl> p;
};

} // namespace debug_menu

#endif // CATA_SRC_DEBUG_CAPTURE_H
