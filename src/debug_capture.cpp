#include "debug_capture.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <string_view>
#include <utility>

#include <flatbuffers/util.h>

#include "calendar.h"
#include "cata_variant.h"
#include "debug.h"
#include "event.h"
#include "event_bus.h"
#include "event_subscriber.h"
#include "flexbuffer_json.h"
#include "json.h"
#include "messages.h"

namespace debug_menu
{

namespace
{
std::atomic<bool> g_initialized{ false };

class event_capture_sub : public event_subscriber
{
    public:
        using event_subscriber::notify;
        void notify( const cata::event &e ) override;
};

// Escape a string into a complete JSON string literal, including the
// surrounding double quotes. natural_utf8=false so control characters and
// non-ASCII bytes become \uXXXX (valid JSON) rather than being emitted raw.
std::string json_quoted( std::string_view s )
{
    std::string out;
    flatbuffers::EscapeString( s.data(), s.size(), &out,
                               /*allow_non_utf8=*/true, /*natural_utf8=*/false );
    return out;
}
} // namespace

std::string capture_json_escape( std::string_view s )
{
    // Same escaping as json_quoted, minus the surrounding quotes it always
    // adds; callers that need a bare body supply their own quoting.
    const std::string quoted = json_quoted( s );
    return quoted.substr( 1, quoted.size() - 2 );
}

struct debug_capture::impl {
    capture_settings settings;
    std::deque<debug_log_entry> log_ring;
    std::deque<event_log_entry> event_ring;
    std::deque<eoc_trace_entry> eoc_trace_ring;

    event_capture_sub event_sub;
    bool event_sub_attached = false;

    std::ofstream trace_file_stream;
    size_t trace_file_bytes = 0;
    int lines_since_flush = 0;
    static constexpr int trace_file_flush_batch = 64;

    std::vector<monitor_entry> monitors;
    int next_monitor_id = 1;

    // See debug_capture::feed_generation().
    uint64_t feed_gen = 0;

    void trim_to_max();
    void append_trace_file( const std::string &source, const std::string &json_payload );
    void ensure_trace_file_open();
    void rotate_trace_file_if_needed();
};

void debug_capture::impl::trim_to_max()
{
    while( log_ring.size() > settings.log_ring_size ) {
        log_ring.pop_front();
    }
    while( event_ring.size() > settings.event_ring_size ) {
        event_ring.pop_front();
    }
    while( eoc_trace_ring.size() > settings.eoc_trace_ring_size ) {
        eoc_trace_ring.pop_front();
    }
}

static std::string csv_escape( std::string_view s )
{
    std::string out;
    out.reserve( s.size() + 2 );
    out.push_back( '"' );
    for( char c : s ) {
        if( c == '"' ) {
            out.append( "\"\"" );
        } else {
            out.push_back( c );
        }
    }
    out.push_back( '"' );
    return out;
}

void debug_capture::impl::ensure_trace_file_open()
{
    if( trace_file_stream.is_open() || !settings.trace_file.enabled ) {
        return;
    }
    trace_file_stream.open( settings.trace_file.path, std::ios::app | std::ios::out );
    if( trace_file_stream.is_open() ) {
        // Rebase byte counter to existing file size so rotation triggers
        // correctly when appending to a prior session's log.
        trace_file_stream.seekp( 0, std::ios::end );
        const auto pos = trace_file_stream.tellp();
        trace_file_bytes = pos > 0 ? static_cast<size_t>( pos ) : 0;
        lines_since_flush = 0;
        if( settings.trace_file.format == capture_format::csv && trace_file_bytes == 0 ) {
            const std::string header = "turn,src,data\n";
            trace_file_stream.write( header.data(), header.size() );
            trace_file_bytes += header.size();
        }
    }
}

void debug_capture::impl::rotate_trace_file_if_needed()
{
    if( trace_file_bytes < settings.trace_file.rotate_mib * 1024 * 1024 ) {
        return;
    }
    trace_file_stream.close();
    const std::string &base = settings.trace_file.path;
    const std::string r1 = base + ".1";
    const std::string r2 = base + ".2";
    const std::string r2_tmp = r2 + ".tmp";
    // Errors here are recoverable (rotation just skipped this turn); we still
    // reopen the base file fresh. Move the stale .2 aside with a rename rather
    // than deleting it before the .1 -> .2 rename: on Windows a delete can
    // leave the file briefly locked, which would make that rename fail.
    const int rn0_rc = std::rename( r2.c_str(), r2_tmp.c_str() );
    const int rn1_rc = std::rename( r1.c_str(), r2.c_str() );
    const int rn2_rc = std::rename( base.c_str(), r1.c_str() );
    const int rm_rc = std::remove( r2_tmp.c_str() );
    ( void )rn0_rc;
    ( void )rn1_rc;
    ( void )rn2_rc;
    ( void )rm_rc;
    trace_file_stream.open( base, std::ios::out | std::ios::trunc );
    trace_file_bytes = 0;
    lines_since_flush = 0;
    if( trace_file_stream.is_open() && settings.trace_file.format == capture_format::csv ) {
        const std::string header = "turn,src,data\n";
        trace_file_stream.write( header.data(), header.size() );
        trace_file_bytes += header.size();
    }
}

void debug_capture::impl::append_trace_file( const std::string &source,
        const std::string &json_payload )
{
    if( !settings.trace_file.wants( source ) ) {
        return;
    }
    ensure_trace_file_open();
    if( !trace_file_stream.is_open() ) {
        return;
    }
    rotate_trace_file_if_needed();
    const int turn = to_turns<int>( calendar::turn - calendar::turn_zero );
    std::string line;
    if( settings.trace_file.format == capture_format::csv ) {
        line = std::to_string( turn ) + "," +
               csv_escape( source ) + "," +
               csv_escape( json_payload ) + "\n";
    } else {
        line = R"({"turn":)" + std::to_string( turn ) +
               R"(,"src":)" + json_quoted( source ) +
               R"(,"data":)" + json_payload + "}\n";
    }
    trace_file_stream.write( line.data(), line.size() );
    trace_file_bytes += line.size();
    if( settings.trace_file.auto_flush
        && ++lines_since_flush >= trace_file_flush_batch ) {
        trace_file_stream.flush();
        lines_since_flush = 0;
    }
}

void debug_capture::flush_trace_file()
{
    if( p->trace_file_stream.is_open() ) {
        p->trace_file_stream.flush();
        p->lines_since_flush = 0;
    }
}

debug_capture &debug_capture::instance()
{
    // Deliberately leaked, process-lifetime singleton. A function-local static
    // would run its destructor during the atexit chain, racing ~game and other
    // static destructors that may still touch capture (e.g. a log from another
    // static dtor). Leaking sidesteps the teardown-order hazard entirely; the
    // OS reclaims at exit, and buffered output is flushed by on_game_shutdown()
    // and the auto-flush batch.
    static debug_capture *const inst = new debug_capture();
    return *inst;
}

bool debug_capture::is_initialized()
{
    return g_initialized.load( std::memory_order_acquire );
}

debug_capture::debug_capture() : p( std::make_unique<impl>() )
{
    g_initialized.store( true, std::memory_order_release );
}

// Defined out-of-line so unique_ptr<impl> sees a complete impl. The instance()
// singleton is leaked, so this is never invoked at runtime.
debug_capture::~debug_capture() = default;

capture_settings &debug_capture::settings()
{
    return p->settings;
}

const capture_settings &debug_capture::settings() const
{
    return p->settings;
}

void debug_capture::on_game_load( event_bus &bus )
{
    if( !p->event_sub_attached ) {
        bus.subscribe( &p->event_sub );
        p->event_sub_attached = true;
    }
}

void debug_capture::on_game_shutdown()
{
    // event_subscriber dtor auto-unsubscribes from its bus when bus still exists.
    p->event_sub_attached = false;
    if( p->trace_file_stream.is_open() ) {
        p->trace_file_stream.flush();
        p->trace_file_stream.close();
        p->lines_since_flush = 0;
    }
}

void debug_capture::push_debug_log( debugmode::debug_filter type, const std::string &msg )
{
    if( !p->settings.main_enabled ) {
        return;
    }
    const bool ring_on = p->settings.add_msg_debug_capture;
    const bool file_on = p->settings.trace_file.wants( "log" );
    if( ring_on ) {
        const int turn = to_turns<int>( calendar::turn - calendar::turn_zero );
        p->log_ring.push_back( { turn, type, msg } );
        p->trim_to_max();
        ++p->feed_gen;
    }
    if( file_on ) {
        const std::string payload = R"({"category":)" + json_quoted( debugmode::filter_name( type ) ) +
                                    R"(,"text":)" + json_quoted( msg ) + "}";
        p->append_trace_file( "log", payload );
    }
}

void debug_capture::push_event( const cata::event &e )
{
    if( !p->settings.main_enabled ) {
        return;
    }
    const bool ring_on = p->settings.event_bus_capture;
    const bool file_on = p->settings.trace_file.wants( "events" );
    // Skip building the field string when no sink will consume it.
    if( !ring_on && !file_on ) {
        return;
    }
    std::string oneline;
    for( const auto &kv : e.data() ) {
        if( !oneline.empty() ) {
            oneline += ", ";
        }
        oneline += kv.first + "=" + kv.second.get_string();
    }
    if( ring_on ) {
        const int turn = to_turns<int>( calendar::turn - calendar::turn_zero );
        p->event_ring.push_back( { turn, static_cast<int>( e.type() ), oneline } );
        p->trim_to_max();
        ++p->feed_gen;
    }
    if( file_on ) {
        const std::string payload = R"({"type":)" + std::to_string( static_cast<int>( e.type() ) ) +
                                    R"(,"fields":)" + json_quoted( oneline ) + "}";
        p->append_trace_file( "events", payload );
    }
}

void debug_capture::push_eoc_trace( const std::string &eoc_id, bool success,
                                    std::chrono::microseconds us, int depth )
{
    if( !p->settings.main_enabled ) {
        return;
    }
    const bool ring_on = p->settings.eoc_trace_capture;
    const bool file_on = p->settings.trace_file.wants( "eoc" );
    if( ring_on ) {
        const int turn = to_turns<int>( calendar::turn - calendar::turn_zero );
        p->eoc_trace_ring.push_back( { eoc_id, static_cast<int64_t>( us.count() ),
                                       turn, depth, success } );
        p->trim_to_max();
        ++p->feed_gen;
    }
    if( file_on ) {
        const std::string payload = R"({"id":)" + json_quoted( eoc_id ) + R"(,"ok":)" +
                                    ( success ? "true" : "false" ) +
                                    R"(,"us":)" + std::to_string( us.count() ) +
                                    R"(,"depth":)" + std::to_string( depth ) + "}";
        p->append_trace_file( "eoc", payload );
    }
}

const std::deque<debug_log_entry> &debug_capture::logs() const
{
    return p->log_ring;
}

const std::deque<event_log_entry> &debug_capture::events() const
{
    return p->event_ring;
}

const std::deque<eoc_trace_entry> &debug_capture::eoc_traces() const
{
    return p->eoc_trace_ring;
}

void debug_capture::clear_logs()
{
    p->log_ring.clear();
    ++p->feed_gen;
}

void debug_capture::clear_events()
{
    p->event_ring.clear();
    ++p->feed_gen;
}

void debug_capture::clear_eoc_traces()
{
    p->eoc_trace_ring.clear();
    ++p->feed_gen;
}

void debug_capture::resize_log_ring( size_t n )
{
    const size_t before = p->log_ring.size();
    p->settings.log_ring_size = n;
    p->trim_to_max();
    // Only bump the feed generation when entries were actually dropped; a grow
    // or no-op resize leaves content unchanged.
    if( p->log_ring.size() != before ) {
        ++p->feed_gen;
    }
}

void debug_capture::resize_event_ring( size_t n )
{
    const size_t before = p->event_ring.size();
    p->settings.event_ring_size = n;
    p->trim_to_max();
    if( p->event_ring.size() != before ) {
        ++p->feed_gen;
    }
}

void debug_capture::resize_eoc_trace_ring( size_t n )
{
    const size_t before = p->eoc_trace_ring.size();
    p->settings.eoc_trace_ring_size = n;
    p->trim_to_max();
    if( p->eoc_trace_ring.size() != before ) {
        ++p->feed_gen;
    }
}

uint64_t debug_capture::feed_generation() const noexcept
{
    return p->feed_gen;
}

int debug_capture::add_monitor( std::string label,
                                std::function<std::string()> snap, monitor_mode mode )
{
    monitor_entry e;
    e.id = p->next_monitor_id++;
    e.label = std::move( label );
    e.snapshot = std::move( snap );
    e.mode = mode;
    e.enabled = true;
    if( e.snapshot ) {
        e.last_snapshot = e.snapshot();
        e.last_snapshot_turn = to_turns<int>( calendar::turn - calendar::turn_zero );
    }
    p->monitors.push_back( std::move( e ) );
    return p->monitors.back().id;
}

void debug_capture::remove_monitor( int id )
{
    p->monitors.erase(
        std::remove_if( p->monitors.begin(), p->monitors.end(),
    [id]( const monitor_entry & e ) {
        return e.id == id;
    } ),
    p->monitors.end() );
}

void debug_capture::snapshot_monitor( int id )
{
    for( monitor_entry &e : p->monitors ) {
        if( e.id == id && e.snapshot ) {
            std::string s = e.snapshot();
            push_debug_log( debugmode::DF_MONITOR,
                            "[" + e.label + "] " + s );
            e.last_snapshot = std::move( s );
            e.last_snapshot_turn = to_turns<int>( calendar::turn - calendar::turn_zero );
            return;
        }
    }
}

std::vector<monitor_entry> &debug_capture::monitors()
{
    return p->monitors;
}

void debug_capture::tick_monitors()
{
    if( !p->settings.main_enabled ) {
        return;
    }
    const int now = to_turns<int>( calendar::turn - calendar::turn_zero );
    for( monitor_entry &e : p->monitors ) {
        if( !e.enabled || !e.snapshot || e.mode == monitor_mode::manual ) {
            continue;
        }
        std::string s = e.snapshot();
        const bool changed = s != e.last_snapshot;
        if( e.mode == monitor_mode::every_turn || changed ) {
            push_debug_log( debugmode::DF_MONITOR,
                            "[" + e.label + "] " + s );
            e.last_snapshot = std::move( s );
            e.last_snapshot_turn = now;
        }
    }
}

void debug_capture::tick_if_active()
{
    if( !g_initialized.load( std::memory_order_acquire ) ) {
        return;
    }
    // Skip the per-turn scan when nothing watches it. Eliminates work for
    // sessions that never add a monitor (the common case).
    debug_capture &inst = instance();
    if( inst.p->monitors.empty() ) {
        return;
    }
    inst.tick_monitors();
}

bool debug_capture::is_eoc_tracing()
{
    if( !g_initialized.load( std::memory_order_acquire ) ) {
        return false;
    }
    return instance().p->settings.eoc_trace_capture;
}

void log_debug_msg( debugmode::debug_filter type, const std::string &msg )
{
    // Skip after the singleton destructor ran (process exit) so we never
    // touch a destroyed instance via the macro hook in messages.h.
    if( !debug_capture::is_initialized() ) {
        return;
    }
    debug_capture::instance().push_debug_log( type, msg );
}

void event_capture_sub::notify( const cata::event &e )
{
    if( !debug_capture::is_initialized() ) {
        return;
    }
    debug_capture::instance().push_event( e );
}

void debug_capture::save_settings( JsonOut &jo ) const
{
    const capture_settings &cs = settings();
    jo.member( "main_enabled", cs.main_enabled );
    jo.member( "add_msg_debug_capture", cs.add_msg_debug_capture );
    jo.member( "event_bus_capture", cs.event_bus_capture );
    jo.member( "eoc_trace_capture", cs.eoc_trace_capture );
    jo.member( "log_ring_size", cs.log_ring_size );
    jo.member( "event_ring_size", cs.event_ring_size );
    jo.member( "eoc_trace_ring_size", cs.eoc_trace_ring_size );
    // On-disk key is "jsonl" (not trace_file) for back-compat with existing
    // config files.
    jo.member( "jsonl" );
    jo.start_object();
    jo.member( "enabled", cs.trace_file.enabled );
    jo.member( "auto_flush", cs.trace_file.auto_flush );
    jo.member( "format", static_cast<int>( cs.trace_file.format ) );
    jo.member( "path", cs.trace_file.path );
    jo.member( "rotate_mib", cs.trace_file.rotate_mib );
    jo.member( "sources" );
    jo.start_array();
    for( const std::string &s : cs.trace_file.sources ) {
        jo.write( s );
    }
    jo.end_array();
    jo.end_object();
}

void debug_capture::load_settings( const JsonObject &jo )
{
    capture_settings &cs = settings();
    jo.read( "main_enabled", cs.main_enabled );
    jo.read( "add_msg_debug_capture", cs.add_msg_debug_capture );
    jo.read( "event_bus_capture", cs.event_bus_capture );
    jo.read( "eoc_trace_capture", cs.eoc_trace_capture );
    uint64_t ring = 0;
    if( jo.read( "log_ring_size", ring ) ) {
        resize_log_ring( static_cast<size_t>( ring ) );
    }
    if( jo.read( "event_ring_size", ring ) ) {
        resize_event_ring( static_cast<size_t>( ring ) );
    }
    if( jo.read( "eoc_trace_ring_size", ring ) ) {
        resize_eoc_trace_ring( static_cast<size_t>( ring ) );
    }
    // On-disk key is "jsonl" for back-compat (see save_settings).
    if( jo.has_object( "jsonl" ) ) {
        const JsonObject jl = jo.get_object( "jsonl" );
        jl.allow_omitted_members();
        jl.read( "enabled", cs.trace_file.enabled );
        jl.read( "auto_flush", cs.trace_file.auto_flush );
        int fmt_int = static_cast<int>( cs.trace_file.format );
        if( jl.read( "format", fmt_int ) ) {
            cs.trace_file.format = fmt_int == 1 ? capture_format::csv : capture_format::jsonl;
        }
        jl.read( "path", cs.trace_file.path );
        uint64_t rotate = cs.trace_file.rotate_mib;
        if( jl.read( "rotate_mib", rotate ) ) {
            cs.trace_file.rotate_mib = static_cast<size_t>( rotate );
        }
        if( jl.has_array( "sources" ) ) {
            cs.trace_file.sources.clear();
            for( const JsonValue &s : jl.get_array( "sources" ) ) {
                cs.trace_file.sources.insert( s.get_string() );
            }
        }
    }
}

} // namespace debug_menu
