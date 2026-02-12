#include <cstddef>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include "cata_catch.h"
#include "save_transaction.h"

#if defined(_WIN32)
#include "platform_win.h"
#else
#if !defined(EMSCRIPTEN)
#include <sys/stat.h>
#endif
#include <unistd.h>
#endif

namespace
{

// RAII helper that creates a unique temp directory and removes it on destruction.
struct temp_dir {
    std::filesystem::path path;

    temp_dir() {
        // NOLINTNEXTLINE(cata-u8-path)
        path = std::filesystem::temp_directory_path() / ( "cata_txn_test_" +
                std::to_string( static_cast<int>(
#if defined(_WIN32)
                                    GetCurrentProcessId()
#else
                                    getpid()
#endif
                                ) ) );
        std::error_code ec;
        std::filesystem::remove_all( path, ec );
        std::filesystem::create_directories( path );
    }

    ~temp_dir() {
        // Restore permissions in case a test made the dir read-only
#if !defined(_WIN32) && !defined(EMSCRIPTEN)
        chmod( path.c_str(), 0755 );
#endif
        std::error_code ec;
        std::filesystem::remove_all( path, ec );
    }

    temp_dir( const temp_dir & ) = delete;
    temp_dir &operator=( const temp_dir & ) = delete;
};

// Write a file using temp+rename to simulate real save behavior.
// This avoids modifying hardlinked backup copies through shared inodes.
void write_file( const std::filesystem::path &p, const std::string &content )
{
    std::filesystem::create_directories( p.parent_path() );
    std::filesystem::path tmp = p;
    tmp += ".write_tmp"; // NOLINT(cata-u8-path)
    {
        std::ofstream f( tmp, std::ios::binary );
        f << content;
    }
    std::filesystem::rename( tmp, p );
}

std::string read_file( const std::filesystem::path &p )
{
    std::ifstream f( p, std::ios::binary );
    return { std::istreambuf_iterator<char>( f ), std::istreambuf_iterator<char>() };
}

bool has_file( const std::filesystem::path &dir, const std::string &name )
{
    return std::filesystem::exists( dir / std::filesystem::u8path( name ) );
}

// Write a minimal backup marker (JSON with file manifest).
void write_backup_marker( const std::filesystem::path &backup_dir,
                          const std::vector<std::string> &files )
{
    std::ofstream f( backup_dir / std::filesystem::u8path( ".backup_marker" ),
                     std::ios::binary );
    f << R"({"version":1,"pid":0,"files":[)";
    for( size_t i = 0; i < files.size(); ++i ) {
        if( i > 0 ) {
            f << ",";
        }
        f << "\"" << files[i] << "\"";
    }
    f << "]}";
}

void write_commit_marker( const std::filesystem::path &save_dir )
{
    std::ofstream f( save_dir / std::filesystem::u8path( ".save_commit" ),
                     std::ios::binary );
    f << R"({"version":1,"pid":0})";
}

} // namespace

TEST_CASE( "save_transaction_skip_temp_files", "[nogame][save_transaction]" )
{
    temp_dir td;
    const std::filesystem::path &dir = td.path;

    // Create files: one normal, one .temp (ofstream_wrapper convention),
    // one .tmp (zzip convention)
    write_file( dir / std::filesystem::u8path( "player.sav" ), "save data" );
    write_file( dir / std::filesystem::u8path( "player.sav.12345.temp" ), "temp ofstream" );
    write_file( dir / std::filesystem::u8path( "maps.zzip.tmp" ), "temp zzip" );

    {
        save_transaction txn( dir );
        // Don't commit -- destructor will restore, but we inspect backup first

        const std::filesystem::path backup = dir / std::filesystem::u8path( ".save_backup" );
        REQUIRE( std::filesystem::exists( backup ) );

        CHECK( has_file( backup, "player.sav" ) );
        CHECK_FALSE( has_file( backup, "player.sav.12345.temp" ) );
        CHECK_FALSE( has_file( backup, "maps.zzip.tmp" ) );

        // Commit so destructor doesn't restore
        CHECK( txn.commit() );
    }
}

TEST_CASE( "save_transaction_stale_marker_removal_failure", "[nogame][save_transaction]" )
{
#if defined(_WIN32) || defined(EMSCRIPTEN)
    WARN( "Skipping: permission-based deletion failure not portable to this platform" );
    return;
#else
    if( getuid() == 0 ) {
        WARN( "Skipping: running as root -- permission checks are bypassed" );
        return;
    }

    temp_dir td;
    const std::filesystem::path &dir = td.path;

    write_file( dir / std::filesystem::u8path( "data.sav" ), "content" );
    write_commit_marker( dir );

    // Make save directory read-only so remove() on the marker fails
    // (deletion is a directory operation on POSIX)
    chmod( dir.c_str(), 0555 );

    // Pre-check: verify that file deletion actually fails in this environment
    {
        std::error_code ec;
        std::filesystem::remove( dir / std::filesystem::u8path( ".save_commit" ), ec );
        if( !ec ) {
            // Marker was removed despite r/o dir (unusual ACL, etc.)
            chmod( dir.c_str(), 0755 );
            WARN( "Skipping: filesystem does not enforce directory permissions for deletion" );
            return;
        }
    }

    CHECK_THROWS_AS( save_transaction( dir ), std::runtime_error );

    // Verify backup was NOT created (no unsafe state)
    CHECK_FALSE( std::filesystem::exists( dir / std::filesystem::u8path( ".save_backup" ) ) );

    // Restore permissions for cleanup
    chmod( dir.c_str(), 0755 );
#endif
}

TEST_CASE( "save_transaction_commit_leaves_marker", "[nogame][save_transaction]" )
{
    temp_dir td;
    const std::filesystem::path &dir = td.path;

    write_file( dir / std::filesystem::u8path( "data.sav" ), "original" );

    // First transaction: commit should leave commit marker, remove backup
    {
        save_transaction txn( dir );
        CHECK( txn.commit() );
    }

    CHECK_FALSE( std::filesystem::exists( dir / std::filesystem::u8path( ".save_backup" ) ) );
    CHECK( std::filesystem::exists( dir / std::filesystem::u8path( ".save_commit" ) ) );

    // Second transaction: create_backup should clean up the stale commit marker
    {
        save_transaction txn( dir );

        CHECK_FALSE( std::filesystem::exists( dir / std::filesystem::u8path( ".save_commit" ) ) );
        CHECK( std::filesystem::exists( dir / std::filesystem::u8path( ".save_backup" ) ) );

        CHECK( txn.commit() );
    }
}

TEST_CASE( "save_transaction_recover_backup_plus_commit", "[nogame][save_transaction]" )
{
    temp_dir td;
    const std::filesystem::path &dir = td.path;

    // Set up the state: backup with marker + commit marker = "completed but cleanup interrupted"
    write_file( dir / std::filesystem::u8path( "data.sav" ), "live data" );

    const std::filesystem::path backup = dir / std::filesystem::u8path( ".save_backup" );
    std::filesystem::create_directory( backup );
    write_file( backup / std::filesystem::u8path( "data.sav" ), "backup data" );
    write_backup_marker( backup, { "data.sav" } );
    write_commit_marker( dir );

    save_transaction::recover_if_needed( dir );

    // Live data should be unchanged (save completed successfully)
    CHECK( read_file( dir / std::filesystem::u8path( "data.sav" ) ) == "live data" );
    // Both backup dir and commit marker should be cleaned up
    CHECK_FALSE( std::filesystem::exists( backup ) );
    CHECK_FALSE( std::filesystem::exists( dir / std::filesystem::u8path( ".save_commit" ) ) );
}

TEST_CASE( "save_transaction_destructor_restores_on_no_commit", "[nogame][save_transaction]" )
{
    temp_dir td;
    const std::filesystem::path &dir = td.path;

    write_file( dir / std::filesystem::u8path( "data.sav" ), "original content" );

    {
        save_transaction txn( dir );

        // Simulate save writing: modify the live file and add a stale file
        write_file( dir / std::filesystem::u8path( "data.sav" ), "modified during save" );
        write_file( dir / std::filesystem::u8path( "stale_new_file.sav" ), "stale" );

        // Don't call commit() -- destructor should restore from backup
    }

    // Original content should be restored
    CHECK( read_file( dir / std::filesystem::u8path( "data.sav" ) ) == "original content" );
    // Stale file should be cleaned up (not in original manifest)
    CHECK_FALSE( std::filesystem::exists(
                     dir / std::filesystem::u8path( "stale_new_file.sav" ) ) );
    // Backup dir should be gone after successful restore
    CHECK_FALSE( std::filesystem::exists(
                     dir / std::filesystem::u8path( ".save_backup" ) ) );

    // TODO: Add a follow-up test that exercises recover_if_needed() directly
    // for the backup-without-commit branch once popup suppression in the test
    // harness is resolved (or popup is made optional in recovery).
}

TEST_CASE( "save_transaction_commit_refuses_on_sync_failure", "[nogame][save_transaction]" )
{
    temp_dir td;
    const std::filesystem::path &dir = td.path;

    write_file( dir / std::filesystem::u8path( "data.sav" ), "original" );

    {
        // Must use fsync_level::full -- notify_sync_failure() only records
        // failure when level is full (save_transaction.cpp)
        save_transaction txn( dir, save_transaction::fsync_level::full );

        // Simulate an fsync failure during save
        save_transaction::notify_sync_failure();

        CHECK_FALSE( txn.commit() );

        // Don't commit -- destructor will restore
    }

    // After destructor runs, original state should be restored
    CHECK( read_file( dir / std::filesystem::u8path( "data.sav" ) ) == "original" );
    CHECK_FALSE( std::filesystem::exists(
                     dir / std::filesystem::u8path( ".save_backup" ) ) );
}
