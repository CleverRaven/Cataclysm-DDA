#include "save_transaction.h"

#include <exception>
#include <fstream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "debug.h"
#include "filesystem.h"
#include "json.h"
#include "output.h"
#include "translations.h"

#if !defined(_WIN32) && !defined(EMSCRIPTEN)
#include <unistd.h>
#endif
#if defined(_WIN32)
#include "platform_win.h"
#endif

static const std::string backup_dirname( ".save_backup" );
static const std::string backup_marker_name( ".backup_marker" );
static const std::string commit_marker_name( ".save_commit" );

// NOLINTNEXTLINE(cata-static-int_id-constants)
save_transaction *save_transaction::current_ = nullptr;

static bool should_skip_entry( const std::filesystem::path &filename )
{
    const std::string name = filename.generic_u8string();
    return name == backup_dirname
           || name == commit_marker_name
           || ( name.size() >= 5 && name.substr( name.size() - 5 ) == ".temp" )
           || ( name.size() >= 4 && name.substr( name.size() - 4 ) == ".tmp" );
}

static int current_pid()
{
#if defined(_WIN32)
    return static_cast<int>( GetCurrentProcessId() );
#elif defined(EMSCRIPTEN)
    return 0;
#else
    return static_cast<int>( getpid() );
#endif
}

static bool write_marker( const std::filesystem::path &marker_path,
                          const std::optional<std::vector<std::string>> &manifest = std::nullopt )
{
    std::ofstream fout( marker_path, std::ios::binary );
    if( !fout.is_open() ) {
        DebugLog( D_ERROR, D_MAIN ) << "save_transaction: failed to open marker: "
                                    << marker_path.u8string();
        return false;
    }

    JsonOut jout( fout );
    jout.start_object();
    jout.member( "version", 1 );
    jout.member( "pid", current_pid() );
    if( manifest.has_value() ) {
        jout.member( "files" );
        jout.start_array();
        for( const std::string &f : *manifest ) {
            jout.write( f );
        }
        jout.end_array();
    }
    jout.end_object();

    fout.flush();
    if( fout.fail() ) {
        DebugLog( D_ERROR, D_MAIN ) << "save_transaction: failed to flush marker: "
                                    << marker_path.u8string();
        return false;
    }
    fout.close();
    return true;
}

// Returns the file manifest from the backup marker.  nullopt means the
// manifest is missing or unreadable (old-format marker or I/O error).
// An empty set means the backup was genuinely empty.
static std::optional<std::set<std::string>> read_manifest(
            const std::filesystem::path &marker_path )
{
    std::ifstream fin( marker_path, std::ios::binary );
    if( !fin.is_open() ) {
        return std::nullopt;
    }
    try {
        TextJsonIn jsin( fin, marker_path.u8string() );
        TextJsonObject jo = jsin.get_object();
        jo.allow_omitted_members();
        if( jo.has_member( "files" ) ) {
            std::set<std::string> files;
            for( const std::string &f : jo.get_string_array( "files" ) ) {
                files.insert( f );
            }
            return files;
        }
    } catch( const std::exception &e ) {
        DebugLog( D_WARNING, D_MAIN ) << "save_transaction: failed to read manifest: "
                                      << e.what();
    }
    return std::nullopt;
}

save_transaction::save_transaction( const std::filesystem::path &save_dir,
                                    fsync_level level )
    : save_dir_( save_dir )
    , backup_dir_( save_dir / std::filesystem::u8path( backup_dirname ) )
    , level_( level )
{
    create_backup();
    current_ = this;
}

save_transaction::~save_transaction()
{
    if( current_ == this ) {
        current_ = nullptr;
    }
    if( !committed_ && backup_created_ ) {
        try {
            restore_from_backup();
        } catch( const std::exception &e ) {
            DebugLog( D_ERROR, D_MAIN ) << "save_transaction: restore failed: " << e.what();
        }
    }
}

bool save_transaction::wants_full_fsync()
{
    return current_ != nullptr && current_->level_ == fsync_level::full;
}

void save_transaction::notify_sync_failure()
{
    if( current_ != nullptr && current_->level_ == fsync_level::full ) {
        current_->sync_failed_ = true;
    }
}

void save_transaction::create_backup()
{
    // If a previous backup exists, handle it
    if( std::filesystem::exists( backup_dir_ ) ) {
        // If the commit marker exists, the previous save completed but cleanup
        // was interrupted.  Just clean up.
        if( std::filesystem::exists( save_dir_ / std::filesystem::u8path( commit_marker_name ) ) ) {
            std::error_code ec;
            std::filesystem::remove_all( backup_dir_, ec );
            if( !ec && fsync_directory( save_dir_ ) ) {
                // Backup removal is durable -- safe to remove commit marker
                std::filesystem::remove( save_dir_ / std::filesystem::u8path( commit_marker_name ), ec );
                if( ec ) {
                    DebugLog( D_WARNING, D_MAIN )
                            << "save_transaction: failed to remove commit marker during "
                            << "stale backup cleanup: " << ec.message();
                }
            } else if( ec ) {
                throw std::runtime_error( "failed to remove stale backup: " + ec.message() );
            } else {
                throw std::runtime_error( "failed to sync after stale backup removal" );
            }
        } else {
            // Previous save was interrupted -- recovery should have run at load.
            // If we got here, something is wrong.  Try recovery now.
            DebugLog( D_WARNING, D_MAIN ) << "save_transaction: stale backup found, recovering";
            restore_from_backup();
            if( std::filesystem::exists( backup_dir_ ) ) {
                throw std::runtime_error( "backup recovery was partial; cannot start new save" );
            }
        }
    }

    // Remove stale commit marker if present (from a fully completed previous save).
    // Only safe when no backup dir exists; if the backup dir is still around
    // (e.g. because removal above failed), the marker must stay so a later
    // run still sees "save completed, cleanup pending."
    //
    // The removal MUST be synced to disk before the backup dir is created.
    // Otherwise, power loss after backup creation could leave both the
    // backup dir and the stale commit marker on disk, causing recovery to
    // misclassify the interrupted save as "completed" and discard the backup.
    if( !std::filesystem::exists( backup_dir_ ) ) {
        const std::filesystem::path commit_path = save_dir_ / std::filesystem::u8path( commit_marker_name );
        if( std::filesystem::exists( commit_path ) ) {
            std::error_code ec;
            std::filesystem::remove( commit_path, ec );
            if( ec ) {
                throw std::runtime_error(
                    "failed to remove stale commit marker: " + ec.message() );
            }
        }
        if( !fsync_directory( save_dir_ ) ) {
            throw std::runtime_error( "failed to sync directory before backup creation" );
        }
    }

    std::error_code ec;
    std::filesystem::create_directory( backup_dir_, ec );
    if( ec ) {
        throw std::runtime_error( "failed to create backup dir: " + ec.message() );
    }

    // Recursively hardlink (or copy) all files.
    //
    // Most save writers use temp+rename (ofstream_wrapper / write_to_file),
    // which is safe with hardlinks: rename() replaces the directory entry,
    // and the backup hardlink retains the old inode with pre-save data.
    //
    // However, zzip archives are modified in place via mmap (MAP_SHARED)
    // before being compacted to a new file.  Hardlinking a .zzip would let
    // add_file() mutate the backup copy through the shared inode.  These
    // files must always be copied, not hardlinked.
    std::vector<std::string> manifest;

    auto dir_iter = std::filesystem::recursive_directory_iterator(
                        save_dir_, std::filesystem::directory_options::skip_permission_denied, ec );
    if( ec ) {
        throw std::runtime_error( "failed to iterate save dir: " + ec.message() );
    }

    for( auto it = dir_iter; it != std::filesystem::recursive_directory_iterator(); ++it ) {
        const std::filesystem::directory_entry &entry = *it;
        const std::filesystem::path rel = entry.path().lexically_relative( save_dir_ );

        // Skip the backup dir itself, commit markers, and temp files.
        // Check the first component of the relative path.
        if( !rel.empty() && should_skip_entry( *rel.begin() ) ) {
            if( entry.is_directory() ) {
                it.disable_recursion_pending();
            }
            continue;
        }

        const std::filesystem::path dest = backup_dir_ / rel;

        if( entry.is_directory() ) {
            std::filesystem::create_directories( dest, ec );
            if( ec ) {
                throw std::runtime_error( "backup mkdir failed: " + ec.message() );
            }
        } else if( entry.is_regular_file() ) {
            // Skip temp files by filename
            if( should_skip_entry( entry.path().filename() ) ) {
                continue;
            }

            const std::string ext = entry.path().extension().generic_u8string();
            const bool is_mmap_mutable = ext == ".zzip";

            if( is_mmap_mutable ) {
                // Zzip archives are modified in place via mmap; must copy
                std::error_code copy_ec;
                std::filesystem::copy_file( entry.path(), dest,
                                            std::filesystem::copy_options::overwrite_existing, copy_ec );
                if( copy_ec ) {
                    throw std::runtime_error( "backup copy failed for " +
                                              entry.path().u8string() + ": " + copy_ec.message() );
                }
            } else {
                // Safe to hardlink; fall back to copy if unsupported
                std::error_code link_ec;
                std::filesystem::create_hard_link( entry.path(), dest, link_ec );
                if( link_ec ) {
                    std::filesystem::copy_file( entry.path(), dest,
                                                std::filesystem::copy_options::overwrite_existing, link_ec );
                    if( link_ec ) {
                        throw std::runtime_error( "backup copy failed for " +
                                                  entry.path().u8string() + ": " + link_ec.message() );
                    }
                }
            }

            manifest.emplace_back( rel.generic_u8string() );
        }
    }

    // Write backup marker with file manifest for idempotent recovery
    if( !write_marker( backup_dir_ / std::filesystem::u8path( backup_marker_name ),
                       std::make_optional( manifest ) ) ) {
        throw std::runtime_error( "failed to write backup marker" );
    }
    if( !fsync_file( backup_dir_ / std::filesystem::u8path( backup_marker_name ) ) ||
        !fsync_directory( backup_dir_ ) ||
        !fsync_directory( save_dir_ ) ) {
        throw std::runtime_error( "failed to sync backup to disk" );
    }

    backup_created_ = true;
}

bool save_transaction::commit()
{
    if( sync_failed_ ) {
        DebugLog( D_WARNING, D_MAIN )
                << "save_transaction: refusing to commit -- fsync failed during save";
        return false;
    }

    // Write commit marker
    const std::filesystem::path marker = save_dir_ / std::filesystem::u8path( commit_marker_name );
    if( !write_marker( marker ) ) {
        DebugLog( D_ERROR, D_MAIN ) << "save_transaction: failed to write commit marker";
        return false;
    }
    if( !fsync_file( marker ) || !fsync_directory( save_dir_ ) ) {
        DebugLog( D_ERROR, D_MAIN ) << "save_transaction: failed to sync commit marker";
        return false;
    }

    // Remove backup.  The commit marker stays on disk so that if power is
    // lost during or after backup removal, recovery sees the marker and
    // knows the live directory is consistent.  The marker is cleaned up at
    // the start of the next save (create_backup) or on the next load
    // (recover_if_needed).
    remove_backup();

    committed_ = true;
    return true;
}

// Move files from backup_dir to save_dir, overwriting live files.
// Directories are created as needed.  The backup marker is skipped.
// Each rename is atomic per-file.  If interrupted, files already moved
// are in save_dir and no longer in backup_dir, so repeating is safe.
// Returns true if all files were moved, false if any file failed.
static bool restore_phase_move( const std::filesystem::path &backup_dir,
                                const std::filesystem::path &save_dir )
{
    bool all_ok = true;
    std::error_code ec;
    // Use recursive_directory_iterator to handle subdirectories.
    // Collect entries first to avoid modifying the tree during iteration.
    std::vector<std::pair<std::filesystem::path, bool>> entries;
    for( const std::filesystem::directory_entry &entry : std::filesystem::recursive_directory_iterator(
             backup_dir, std::filesystem::directory_options::skip_permission_denied, ec ) ) {
        const std::string fname = entry.path().filename().generic_u8string();
        if( fname == backup_marker_name ) {
            continue;
        }
        entries.emplace_back( entry.path(), entry.is_directory() );
    }

    for( const auto &[path, is_dir] : entries ) {
        const std::filesystem::path rel = path.lexically_relative( backup_dir );
        const std::filesystem::path dest = save_dir / rel;

        if( is_dir ) {
            std::filesystem::create_directories( dest, ec );
        } else {
            // Ensure parent directory exists
            std::filesystem::create_directories( dest.parent_path(), ec );
            // Remove existing live file so rename can succeed
            std::filesystem::remove( dest, ec );
            std::filesystem::rename( path, dest, ec );
            if( ec ) {
                // rename may fail across filesystems; fall back to copy+remove
                std::filesystem::copy_file( path, dest,
                                            std::filesystem::copy_options::overwrite_existing, ec );
                if( !ec ) {
                    std::filesystem::remove( path, ec );
                }
                if( ec ) {
                    DebugLog( D_ERROR, D_MAIN ) << "save recovery: failed to restore "
                                                << rel.u8string() << ": " << ec.message();
                    all_ok = false;
                }
            }
        }
    }
    return all_ok;
}

// Remove files from save_dir that are not in the manifest (stale files
// written during an interrupted save).  Also removes now-empty directories.
static void restore_phase_cleanup( const std::filesystem::path &save_dir,
                                   const std::set<std::string> &manifest )
{
    std::error_code ec;
    // Collect entries first, then remove (don't modify tree during iteration).
    // Use manual loop so we can skip recursion into the backup directory.
    std::vector<std::pair<std::filesystem::path, bool>> entries;
    auto dir_iter = std::filesystem::recursive_directory_iterator(
                        save_dir, std::filesystem::directory_options::skip_permission_denied, ec );
    for( auto it = dir_iter; it != std::filesystem::recursive_directory_iterator(); ++it ) {
        const std::filesystem::directory_entry &entry = *it;
        const std::string fname = entry.path().filename().generic_u8string();
        // Skip the backup dir (and don't recurse into it) and commit marker
        if( fname == backup_dirname || fname == commit_marker_name ) {
            if( entry.is_directory() ) {
                it.disable_recursion_pending();
            }
            continue;
        }
        entries.emplace_back( entry.path(), entry.is_directory() );
    }

    // Remove stale files (not in manifest).
    for( const auto &[path, is_dir] : entries ) {
        if( is_dir ) {
            continue;
        }
        const std::string rel = path.lexically_relative( save_dir ).generic_u8string();
        if( manifest.find( rel ) == manifest.end() ) {
            std::filesystem::remove( path, ec );
        }
    }

    // Remove now-empty directories (iterate in reverse so children come first).
    for( auto it = entries.rbegin(); it != entries.rend(); ++it ) {
        if( !it->second ) {
            continue;
        }
        if( std::filesystem::is_empty( it->first, ec ) && !ec ) {
            std::filesystem::remove( it->first, ec );
        }
    }
}

// Run recovery phases 1-2: read manifest, move backup files to save_dir,
// clean up stale files.  Returns true if all files were moved successfully.
// The backup directory still exists after this call (caller handles phase 3).
static bool restore_phases_1_and_2( const std::filesystem::path &backup_dir,
                                    const std::filesystem::path &save_dir )
{
    // Read the file manifest so we know which files belong in the save.
    const std::optional<std::set<std::string>> manifest =
            read_manifest( backup_dir / std::filesystem::u8path( backup_marker_name ) );

    // Phase 1: Move each backup file to save_dir (overwriting live files).
    // Using rename is atomic per-file.  If recovery is interrupted, files
    // already moved are in save_dir correctly and no longer in backup_dir.
    // The next recovery attempt moves the remaining files.
    const bool move_ok = restore_phase_move( backup_dir, save_dir );

    // Phase 2: Remove stale files from save_dir that are not in the manifest.
    // These are files written during the interrupted save that should not exist
    // in the pre-save snapshot.  The manifest is still readable (it was in the
    // backup marker, which we haven't deleted yet).
    // Skipped only if the manifest is missing (old-format backup); an empty
    // manifest still triggers cleanup to remove any stale files.
    if( manifest.has_value() ) {
        restore_phase_cleanup( save_dir, *manifest );
    }

    return move_ok;
}

void save_transaction::remove_backup()
{
    if( std::filesystem::exists( backup_dir_ ) ) {
        std::error_code ec;
        std::filesystem::remove_all( backup_dir_, ec );
        if( ec ) {
            DebugLog( D_WARNING, D_MAIN ) << "save_transaction: failed to remove backup: "
                                          << ec.message();
        }
    }
}

void save_transaction::restore_from_backup()
{
    if( !std::filesystem::exists( backup_dir_ ) ) {
        return;
    }

    // Check that backup is complete (has marker)
    if( !std::filesystem::exists( backup_dir_ / std::filesystem::u8path( backup_marker_name ) ) ) {
        // Backup was incomplete -- the live directory should be untouched.
        // Just remove the partial backup.
        DebugLog( D_WARNING, D_MAIN ) << "save_transaction: incomplete backup, removing";
        std::error_code ec;
        std::filesystem::remove_all( backup_dir_, ec );
        return;
    }

    DebugLog( D_WARNING, D_MAIN ) << "save_transaction: restoring from backup";

    const bool move_ok = restore_phases_1_and_2( backup_dir_, save_dir_ );

    // Phase 3: Remove backup dir (now contains only the marker).
    // Only safe when all files were restored; partial failure keeps the
    // backup so a subsequent recovery attempt can finish the job.
    if( move_ok ) {
        std::error_code ec;
        std::filesystem::remove_all( backup_dir_, ec );
        if( !ec && fsync_directory( save_dir_ ) ) {
            // Backup removal is durable -- safe to remove commit marker
            std::filesystem::remove( save_dir_ / std::filesystem::u8path( commit_marker_name ), ec );
            if( ec ) {
                DebugLog( D_WARNING, D_MAIN )
                        << "save_transaction: failed to remove commit marker after restore: "
                        << ec.message();
            }
        } else {
            DebugLog( D_WARNING, D_MAIN )
                    << "save_transaction: cleanup sync failed after restore";
        }
    } else {
        DebugLog( D_ERROR, D_MAIN )
                << "save_transaction: partial restore -- backup kept for manual recovery";
    }
}

void save_transaction::recover_if_needed( const std::filesystem::path &save_dir )
{
    const std::filesystem::path backup = save_dir / std::filesystem::u8path( backup_dirname );
    const std::filesystem::path commit = save_dir / std::filesystem::u8path( commit_marker_name );

    if( !std::filesystem::exists( backup ) ) {
        // No backup directory -- check for stale commit marker
        if( std::filesystem::exists( commit ) ) {
            std::error_code ec;
            std::filesystem::remove( commit, ec );
        }
        return;
    }

    if( std::filesystem::exists( commit ) ) {
        // Save completed successfully but cleanup was interrupted.
        // The live directory is consistent.  Just clean up.
        // Only remove the commit marker after the backup is gone and synced;
        // if backup removal or sync fails, the marker must stay so a later
        // run still sees the "save completed" state.
        std::error_code ec;
        std::filesystem::remove_all( backup, ec );
        if( !ec && fsync_directory( save_dir ) ) {
            // Backup removal is durable -- safe to remove commit marker
            std::filesystem::remove( commit, ec );
            if( ec ) {
                DebugLog( D_WARNING, D_MAIN )
                        << "save recovery: failed to remove commit marker: " << ec.message();
            }
        } else {
            DebugLog( D_WARNING, D_MAIN )
                    << "save recovery: cleanup sync failed, will retry next load";
        }
        return;
    }

    // Backup exists without commit marker -- save was interrupted.
    if( !std::filesystem::exists( backup / std::filesystem::u8path( backup_marker_name ) ) ) {
        // Backup was incomplete -- live directory should be untouched.
        DebugLog( D_WARNING, D_MAIN ) << "save recovery: removing incomplete backup";
        std::error_code ec;
        std::filesystem::remove_all( backup, ec );
        return;
    }

    // Full recovery needed
    popup( _( "Recovering save from backup after interrupted save\u2026" ) );
    DebugLog( D_WARNING, D_MAIN ) << "save recovery: restoring from backup in "
                                  << save_dir.u8string();

    const bool move_ok = restore_phases_1_and_2( backup, save_dir );

    // Phase 3: Remove backup dir.
    // Only safe when all files were restored; partial failure keeps the
    // backup so a subsequent recovery attempt can finish the job.
    if( move_ok ) {
        std::error_code ec;
        std::filesystem::remove_all( backup, ec );
    } else {
        DebugLog( D_ERROR, D_MAIN )
                << "save recovery: partial restore -- backup kept at "
                << backup.u8string();
        popup( _( "Warning: Save recovery was partial.  Some files could not be restored.\n"
                  "The backup has been preserved for manual recovery." ) );
    }
}
