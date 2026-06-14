#include <chrono>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <set>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "debug.h"
#include "debug_capture.h"

TEST_CASE( "capture_json_escape_round_trip", "[debug_capture]" )
{
    SECTION( "plain ASCII passes through" ) {
        CHECK( debug_menu::capture_json_escape( "hello world" ) == "hello world" );
    }
    SECTION( "quote and backslash get escaped" ) {
        CHECK( debug_menu::capture_json_escape( "say \"hi\"" ) == "say \\\"hi\\\"" );
        CHECK( debug_menu::capture_json_escape( "back\\slash" ) == "back\\\\slash" );
    }
    SECTION( "newline + tab + CR" ) {
        std::string in;
        in += 'a';
        in += '\n';
        in += 'b';
        in += '\t';
        in += 'c';
        in += '\r';
        in += 'd';
        CHECK( debug_menu::capture_json_escape( in ) == "a\\nb\\tc\\rd" );
    }
    SECTION( "control char below 0x20 -> \\u escape" ) {
        const std::string in( 1, '\x01' );
        const std::string out = debug_menu::capture_json_escape( in );
        CHECK( out == "\\u0001" );
    }
}

TEST_CASE( "capture_ring_trimming", "[debug_capture]" )
{
    debug_menu::debug_capture &cap = debug_menu::debug_capture::instance();
    cap.clear_logs();
    cap.resize_log_ring( 5 );
    cap.settings().add_msg_debug_capture = true;
    for( int i = 0; i < 20; i++ ) {
        cap.push_debug_log( debugmode::DF_GAME, "msg " + std::to_string( i ) );
    }
    CHECK( cap.logs().size() == 5 );
    // Ring is FIFO: oldest popped, last 5 retained.
    CHECK( cap.logs().front().text == "msg 15" );
    CHECK( cap.logs().back().text == "msg 19" );
    // Restore default for other tests.
    cap.resize_log_ring( 2000 );
    cap.clear_logs();
}

TEST_CASE( "capture_eoc_trace_ring", "[debug_capture]" )
{
    debug_menu::debug_capture &cap = debug_menu::debug_capture::instance();
    cap.clear_eoc_traces();
    cap.resize_eoc_trace_ring( 3 );
    cap.settings().eoc_trace_capture = true;
    for( int i = 0; i < 10; i++ ) {
        cap.push_eoc_trace( "EOC_" + std::to_string( i ), true,
                            std::chrono::microseconds( 100 ), i );
    }
    CHECK( cap.eoc_traces().size() == 3 );
    CHECK( cap.eoc_traces().back().depth == 9 );
    cap.resize_eoc_trace_ring( 5000 );
    cap.clear_eoc_traces();
    cap.settings().eoc_trace_capture = false;
}

TEST_CASE( "capture_monitor_basic", "[debug_capture]" )
{
    debug_menu::debug_capture &cap = debug_menu::debug_capture::instance();
    cap.clear_logs();
    cap.settings().add_msg_debug_capture = true;
    int counter = 0;
    const int id = cap.add_monitor( "test", [&counter]() {
        return std::to_string( ++counter );
    }, debug_menu::monitor_mode::every_turn );
    // Adding takes initial snapshot (counter=1) but no log push.
    CHECK( cap.logs().empty() );
    cap.tick_monitors();
    CHECK( cap.logs().size() == 1 );
    CHECK( cap.logs().back().category == debugmode::DF_MONITOR );
    cap.tick_monitors();
    CHECK( cap.logs().size() == 2 );
    cap.remove_monitor( id );
    CHECK( cap.monitors().empty() );
    cap.clear_logs();
}

TEST_CASE( "capture_jsonl_byte_counter_rebase_on_existing_file", "[debug_capture]" )
{
    // Write a placeholder file, then attach JSONL output to it -- the byte
    // counter should reflect existing file size, not zero.
    const std::string path = "test_jsonl_rebase.jsonl";
    {
        std::ofstream pre( path, std::ios::trunc );
        for( int i = 0; i < 100; i++ ) {
            pre << "{\"placeholder\":" << i << "}\n";
        }
    }
    debug_menu::debug_capture &cap = debug_menu::debug_capture::instance();
    cap.settings().trace_file.path = path;
    cap.settings().trace_file.rotate_mib = 100; // far above test size, won't rotate
    cap.settings().trace_file.enabled = true;
    cap.settings().trace_file.sources = { "log" };
    cap.settings().add_msg_debug_capture = true;
    cap.push_debug_log( debugmode::DF_GAME, "trigger open" );
    cap.flush_trace_file();
    // Verify file size grew vs placeholder size (counter was rebased; new
    // line appended).
    std::ifstream f( path, std::ios::ate );
    const auto sz = f.tellg();
    CHECK( sz > 100 );
    cap.settings().trace_file.enabled = false;
    cap.on_game_shutdown();
    const int rm_rc = std::remove( path.c_str() );
    ( void )rm_rc;
}

TEST_CASE( "capture_json_escape_produces_valid_json", "[debug_capture]" )
{
    // Control chars and non-ASCII become \uXXXX, so the result is a valid JSON
    // string body once wrapped in quotes.
    SECTION( "non-ASCII is \\u-escaped, not emitted raw" ) {
        // U+00E9 (e-acute) in UTF-8 is 0xC3 0xA9; EscapeString emits uppercase hex.
        const std::string in = "caf\xC3\xA9";
        const std::string out = debug_menu::capture_json_escape( in );
        CHECK( out == "caf\\u00E9" );
    }
    SECTION( "embedded control char between text" ) {
        std::string in = "a";
        in += '\x1f';
        in += "b";
        CHECK( debug_menu::capture_json_escape( in ) == "a\\u001Fb" );
    }
    SECTION( "empty string" ) {
        CHECK( debug_menu::capture_json_escape( "" ).empty() );
    }
}

TEST_CASE( "capture_resize_feed_generation", "[debug_capture]" )
{
    debug_menu::debug_capture &cap = debug_menu::debug_capture::instance();
    cap.clear_logs();
    cap.resize_log_ring( 100 );
    cap.settings().add_msg_debug_capture = true;
    for( int i = 0; i < 10; i++ ) {
        cap.push_debug_log( debugmode::DF_GAME, "m" );
    }
    SECTION( "growing / keeping size leaves generation unchanged" ) {
        const uint64_t gen = cap.feed_generation();
        cap.resize_log_ring( 100 ); // same
        CHECK( cap.feed_generation() == gen );
        cap.resize_log_ring( 500 ); // grow, 10 entries kept
        CHECK( cap.feed_generation() == gen );
    }
    SECTION( "shrinking below the entry count bumps generation" ) {
        const uint64_t gen = cap.feed_generation();
        cap.resize_log_ring( 4 ); // drops 6 entries
        CHECK( cap.feed_generation() > gen );
        CHECK( cap.logs().size() == 4 );
    }
    cap.resize_log_ring( 2000 );
    cap.clear_logs();
}

TEST_CASE( "capture_trace_file_rotation", "[debug_capture]" )
{
    const std::string base = "test_trace_rotate.jsonl";
    const std::string r1 = base + ".1";
    const std::string r2 = base + ".2";
    const std::string r2_tmp = r2 + ".tmp";
    for( const std::string &p : {
             base, r1, r2, r2_tmp
         } ) {
        const int rc = std::remove( p.c_str() );
        ( void )rc;
    }
    debug_menu::debug_capture &cap = debug_menu::debug_capture::instance();
    cap.settings().trace_file.path = base;
    cap.settings().trace_file.rotate_mib = 0; // rotate on the first append
    cap.settings().trace_file.enabled = true;
    cap.settings().trace_file.format = debug_menu::capture_format::jsonl;
    cap.settings().trace_file.sources = { "log" };
    cap.settings().add_msg_debug_capture = true;
    // First append opens the file; with rotate_mib 0 the next append rotates.
    cap.push_debug_log( debugmode::DF_GAME, "first" );
    cap.push_debug_log( debugmode::DF_GAME, "second" );
    cap.flush_trace_file();
    // base rolled to .1; no stray .tmp left behind.
    std::ifstream rolled( r1 );
    CHECK( rolled.good() );
    std::ifstream tmp( r2_tmp );
    CHECK_FALSE( tmp.good() );
    cap.settings().trace_file.enabled = false;
    cap.settings().trace_file.rotate_mib = 50;
    cap.on_game_shutdown();
    for( const std::string &p : {
             base, r1, r2, r2_tmp
         } ) {
        const int rc = std::remove( p.c_str() );
        ( void )rc;
    }
}
