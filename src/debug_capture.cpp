#include "debug_capture.h"

#include <algorithm>
#include <atomic>
#include <fstream>
#include <string_view>
#include <utility>

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

std::string json_escape( std::string_view s )
{
    std::string out;
    out.reserve( s.size() + 2 );
    for( char c : s ) {
        switch( c ) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if( static_cast<unsigned char>( c ) < 0x20 ) {
                    static constexpr char hex[] = "0123456789abcdef";
                    const unsigned char uc = static_cast<unsigned char>( c );
                    out += R"(\u00)";
                    out += hex[( uc >> 4 ) & 0xF ];
                    out += hex[ uc & 0xF ];
                } else {
                    out += c;
                }
        }
    }
    return out;
}
} // namespace

std::string capture_json_escape( std::string_view s )
{
    return json_escape( s );
}

struct debug_capture::impl {
    capture_settings settings;
    std::deque<debug_log_entry> log_ring;
    std::deque<event_log_entry> event_ring;
    std::deque<eoc_trace_entry> eoc_trace_ring;

    event_capture_sub event_sub;
    bool event_sub_attached = false;

    std::ofstream jsonl_file;
    size_t jsonl_bytes = 0;
    int lines_since_flush = 0;
    static constexpr int jsonl_flush_batch = 64;

    std::vector<monitor_entry> monitors;
    int next_monitor_id = 1;

    // See debug_capture::feed_generation().
    uint64_t feed_gen = 0;

    void trim_to_max();
    void append_jsonl( const std::string &source, const std::string &json_payload );
    void ensure_jsonl_open();
    void rotate_jsonl_if_needed();
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

void debug_capture::impl::ensure_jsonl_open()
{
    if( jsonl_file.is_open() || !settings.jsonl.enabled ) {
        return;
    }
    jsonl_file.open( settings.jsonl.path, std::ios::app | std::ios::out );
    if( jsonl_file.is_open() ) {
        // Rebase byte counter to existing file size so rotation triggers
        // correctly when appending to a prior session's log.
        jsonl_file.seekp( 0, std::ios::end );
        const auto pos = jsonl_file.tellp();
        jsonl_bytes = pos > 0 ? static_cast<size_t>( pos ) : 0;
        lines_since_flush = 0;
        if( settings.jsonl.format == capture_format::csv && jsonl_bytes == 0 ) {
            const std::string header = "turn,src,data\n";
            jsonl_file.write( header.data(), header.size() );
            jsonl_bytes += header.size();
        }
    }
}

void debug_capture::impl::rotate_jsonl_if_needed()
{
    if( jsonl_bytes < settings.jsonl.rotate_mib * 1024 * 1024 ) {
        return;
    }
    jsonl_file.close();
    const std::string &base = settings.jsonl.path;
    const std::string r1 = base + ".1";
    const std::string r2 = base + ".2";
    // Errors here are recoverable (rotation just skipped this turn); we still
    // reopen the base file fresh.
    const int rm_rc = std::remove( r2.c_str() );
    const int rn1_rc = std::rename( r1.c_str(), r2.c_str() );
    const int rn2_rc = std::rename( base.c_str(), r1.c_str() );
    ( void )rm_rc;
    ( void )rn1_rc;
    ( void )rn2_rc;
    jsonl_file.open( base, std::ios::out | std::ios::trunc );
    jsonl_bytes = 0;
    lines_since_flush = 0;
    if( jsonl_file.is_open() && settings.jsonl.format == capture_format::csv ) {
        const std::string header = "turn,src,data\n";
        jsonl_file.write( header.data(), header.size() );
        jsonl_bytes += header.size();
    }
}

void debug_capture::impl::append_jsonl( const std::string &source,
                                        const std::string &json_payload )
{
    if( !settings.jsonl.enabled ) {
        return;
    }
    if( !settings.jsonl.sources.count( source ) ) {
        return;
    }
    ensure_jsonl_open();
    if( !jsonl_file.is_open() ) {
        return;
    }
    rotate_jsonl_if_needed();
    const int turn = to_turns<int>( calendar::turn - calendar::turn_zero );
    std::string line;
    if( settings.jsonl.format == capture_format::csv ) {
        line = std::to_string( turn ) + "," +
               csv_escape( source ) + "," +
               csv_escape( json_payload ) + "\n";
    } else {
        line = R"({"turn":)" + std::to_string( turn ) +
               R"(,"src":")" + json_escape( source ) +
               R"(","data":)" + json_payload + "}\n";
    }
    jsonl_file.write( line.data(), line.size() );
    jsonl_bytes += line.size();
    if( settings.jsonl.auto_flush
        && ++lines_since_flush >= jsonl_flush_batch ) {
        jsonl_file.flush();
        lines_since_flush = 0;
    }
}

void debug_capture::flush_jsonl()
{
    if( p->jsonl_file.is_open() ) {
        p->jsonl_file.flush();
        p->lines_since_flush = 0;
    }
}

debug_capture &debug_capture::instance()
{
    static debug_capture inst;
    return inst;
}

bool debug_capture::is_initialized()
{
    return g_initialized.load( std::memory_order_acquire );
}

debug_capture::debug_capture() : p( std::make_unique<impl>() )
{
    g_initialized.store( true, std::memory_order_release );
}

debug_capture::~debug_capture()
{
    // Mark uninitialized first so any code racing this dtor (e.g. ~game later
    // in the atexit chain, log_debug_msg from another static-dtor) skips the
    // singleton instead of touching destroyed state.
    g_initialized.store( false, std::memory_order_release );
    if( p && p->jsonl_file.is_open() ) {
        p->jsonl_file.flush();
        p->jsonl_file.close();
    }
}

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
    if( p->jsonl_file.is_open() ) {
        p->jsonl_file.flush();
        p->jsonl_file.close();
        p->lines_since_flush = 0;
    }
}

void debug_capture::push_debug_log( debugmode::debug_filter type, const std::string &msg )
{
    if( !p->settings.main_enabled ) {
        return;
    }
    const bool ring_on = p->settings.add_msg_debug_capture;
    const bool jsonl_on = p->settings.jsonl.enabled
                          && p->settings.jsonl.sources.count( "log" );
    if( !ring_on && !jsonl_on ) {
        return;
    }
    if( ring_on ) {
        const int turn = to_turns<int>( calendar::turn - calendar::turn_zero );
        p->log_ring.push_back( { type, msg, turn } );
        p->trim_to_max();
        ++p->feed_gen;
    }
    if( jsonl_on ) {
        std::string payload = R"({"category":")" + debugmode::filter_name( type ) +
                              R"(","text":")" + json_escape( msg ) + R"("})";
        p->append_jsonl( "log", payload );
    }
}

void debug_capture::push_event( const cata::event &e )
{
    if( !p->settings.main_enabled ) {
        return;
    }
    const bool ring_on = p->settings.event_bus_capture;
    const bool jsonl_on = p->settings.jsonl.enabled
                          && p->settings.jsonl.sources.count( "events" );
    if( !ring_on && !jsonl_on ) {
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
    if( jsonl_on ) {
        std::string payload = R"({"type":)" + std::to_string( static_cast<int>( e.type() ) ) +
                              R"(,"fields":")" + json_escape( oneline ) + R"("})";
        p->append_jsonl( "events", payload );
    }
}

void debug_capture::push_eoc_trace( const std::string &eoc_id, bool success,
                                    std::chrono::microseconds us, int depth )
{
    if( !p->settings.main_enabled ) {
        return;
    }
    const bool ring_on = p->settings.eoc_trace_capture;
    const bool jsonl_on = p->settings.jsonl.enabled
                          && p->settings.jsonl.sources.count( "eoc" );
    if( !ring_on && !jsonl_on ) {
        return;
    }
    if( ring_on ) {
        const int turn = to_turns<int>( calendar::turn - calendar::turn_zero );
        p->eoc_trace_ring.push_back( { turn, eoc_id, success,
                                       static_cast<int64_t>( us.count() ), depth } );
        p->trim_to_max();
        ++p->feed_gen;
    }
    if( jsonl_on ) {
        std::string payload = R"({"id":")" + json_escape( eoc_id ) + R"(","ok":)" +
                              ( success ? "true" : "false" ) +
                              R"(,"us":)" + std::to_string( us.count() ) +
                              R"(,"depth":)" + std::to_string( depth ) + "}";
        p->append_jsonl( "eoc", payload );
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
    p->settings.log_ring_size = n;
    p->trim_to_max();
    ++p->feed_gen;
}

void debug_capture::resize_event_ring( size_t n )
{
    p->settings.event_ring_size = n;
    p->trim_to_max();
    ++p->feed_gen;
}

void debug_capture::resize_eoc_trace_ring( size_t n )
{
    p->settings.eoc_trace_ring_size = n;
    p->trim_to_max();
    ++p->feed_gen;
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
    jo.member( "jsonl" );
    jo.start_object();
    jo.member( "enabled", cs.jsonl.enabled );
    jo.member( "auto_flush", cs.jsonl.auto_flush );
    jo.member( "format", static_cast<int>( cs.jsonl.format ) );
    jo.member( "path", cs.jsonl.path );
    jo.member( "rotate_mib", cs.jsonl.rotate_mib );
    jo.member( "sources" );
    jo.start_array();
    for( const std::string &s : cs.jsonl.sources ) {
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
    if( jo.has_object( "jsonl" ) ) {
        const JsonObject jl = jo.get_object( "jsonl" );
        jl.allow_omitted_members();
        jl.read( "enabled", cs.jsonl.enabled );
        jl.read( "auto_flush", cs.jsonl.auto_flush );
        int fmt_int = static_cast<int>( cs.jsonl.format );
        if( jl.read( "format", fmt_int ) ) {
            cs.jsonl.format = fmt_int == 1 ? capture_format::csv : capture_format::jsonl;
        }
        jl.read( "path", cs.jsonl.path );
        uint64_t rotate = cs.jsonl.rotate_mib;
        if( jl.read( "rotate_mib", rotate ) ) {
            cs.jsonl.rotate_mib = static_cast<size_t>( rotate );
        }
        if( jl.has_array( "sources" ) ) {
            cs.jsonl.sources.clear();
            for( const JsonValue &s : jl.get_array( "sources" ) ) {
                cs.jsonl.sources.insert( s.get_string() );
            }
        }
    }
}

} // namespace debug_menu
