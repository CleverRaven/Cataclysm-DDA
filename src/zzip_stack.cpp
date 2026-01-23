#include "zzip_stack.h"

#include <algorithm>
#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "filesystem.h"
#include "zzip.h"

static constexpr std::string_view kColdSuffix = ".cold.zzip";
static constexpr std::string_view kWarmSuffix = ".warm.zzip";
static constexpr std::string_view kHotSuffix = ".hot.zzip";

zzip_stack::zzip_stack( std::filesystem::path const &path,
                        std::filesystem::path const &dictionary ) noexcept :
    path_{ path },
       dictionary_{ dictionary }
{}

zzip_stack::~zzip_stack() noexcept = default;

std::shared_ptr<zzip_stack> zzip_stack::load( std::filesystem::path const &path,
        std::filesystem::path const &dictionary_path )
{
    if( !assure_dir_exist( path ) ) {
        return nullptr;
    }
    // We lazily load and populate entries to avoid unnecessary overhead on short reads.
    return std::shared_ptr<zzip_stack>( new zzip_stack{ path, dictionary_path } );
}

bool zzip_stack::add_file( std::filesystem::path const &zzip_relative_path,
                           std::string_view content )
{
    bool ret = hot().add_file( zzip_relative_path, content );
    if( ret ) {
        path_temp_map_[zzip_relative_path] = file_temp::hot;
    }
    return ret;
}

bool zzip_stack::copy_files_to( std::vector<std::filesystem::path> const &zzip_relative_paths,
                                zzip &to )
{
    std::unordered_map<file_temp, std::vector<std::filesystem::path>> temp_to_paths;
    for( std::filesystem::path const &zzip_relative_path : zzip_relative_paths ) {
        file_temp temp = temp_of_file( zzip_relative_path );
        if( temp == file_temp::unknown ) {
            continue;
        }
        temp_to_paths[temp].emplace_back( zzip_relative_path );
    }
    return to.copy_files( temp_to_paths[file_temp::cold], cold() ) &&
           to.copy_files( temp_to_paths[file_temp::warm], warm() ) &&
           to.copy_files( temp_to_paths[file_temp::hot], hot() );
}

bool zzip_stack::has_file( std::filesystem::path const &zzip_relative_path ) const
{
    return temp_of_file( zzip_relative_path ) != file_temp::unknown;
}

size_t zzip_stack::get_file_size( std::filesystem::path const &zzip_relative_path ) const
{
    file_temp temp = temp_of_file( zzip_relative_path );
    if( temp == file_temp::unknown ) {
        return 0;
    }
    return zzip_of_temp( temp ).get_file_size( zzip_relative_path );
}

std::vector<std::filesystem::path> zzip_stack::get_entries() const
{
    std::vector<std::filesystem::path> entries;
    cold();
    warm();
    hot();
    entries.reserve( path_temp_map_.size() );
    for( const auto& [entry_path, temp] : path_temp_map_ ) {
        entries.emplace_back( entry_path );
    }
    return entries;
}

std::vector<std::byte> zzip_stack::get_file( std::filesystem::path const &zzip_relative_path ) const
{
    file_temp temp = temp_of_file( zzip_relative_path );
    if( temp == file_temp::unknown ) {
        return std::vector<std::byte> {};
    }
    return zzip_of_temp( temp ).get_file( zzip_relative_path );
}

size_t zzip_stack::get_file_to( std::filesystem::path const &zzip_relative_path, std::byte *dest,
                                size_t dest_len ) const
{
    file_temp temp = temp_of_file( zzip_relative_path );
    if( temp == file_temp::unknown ) {
        return 0;
    }
    return zzip_of_temp( temp ).get_file_to( zzip_relative_path, dest, dest_len );
}

std::shared_ptr<zzip_stack> zzip_stack::create_from_folder( std::filesystem::path const &path,
        std::filesystem::path const &folder,
        std::filesystem::path const &dictionary )
{
    if( !assure_dir_exist( path ) ) {
        return nullptr;
    }
    // Default everything to cold.
    std::filesystem::path zzip_filename = filename_of_temp( path, file_temp::cold );
    std::optional<zzip> z = zzip::create_from_folder( path / zzip_filename, folder, dictionary );
    if( !z ) {
        return nullptr;
    }
    return load( path, dictionary );
}

std::shared_ptr<zzip_stack> zzip_stack::create_from_folder_with_files(
    std::filesystem::path const &path,
    std::filesystem::path const &folder,
    std::vector<std::filesystem::path> const &files,
    uint64_t total_file_size,
    std::filesystem::path const &dictionary )
{
    if( !assure_dir_exist( path ) ) {
        return nullptr;
    }
    // Default everything to cold.
    std::filesystem::path zzip_filename = filename_of_temp( path, file_temp::cold );
    std::optional<zzip> z = zzip::create_from_folder_with_files( path / zzip_filename, folder, files,
                            total_file_size, dictionary );
    if( !z ) {
        return nullptr;
    }
    return load( path, dictionary );
}

bool zzip_stack::extract_to_folder( std::filesystem::path const &path,
                                    std::filesystem::path const &folder,
                                    std::filesystem::path const &dictionary )
{
    std::shared_ptr<zzip_stack> stack = zzip_stack::load( path, dictionary );
    if( !stack->cold().extract_to_folder( folder ) ) {
        return false;
    }
    if( !stack->warm().extract_to_folder( folder ) ) {
        return false;
    }
    if( !stack->hot().extract_to_folder( folder ) ) {
        return false;
    }
    return true;
}

bool zzip_stack::delete_files( std::unordered_set<std::filesystem::path, std_fs_path_hash> const
                               &zzip_relative_paths )
{
    std::unordered_set<std::filesystem::path, std_fs_path_hash> cold_deletions;
    std::unordered_set<std::filesystem::path, std_fs_path_hash> warm_deletions;
    std::unordered_set<std::filesystem::path, std_fs_path_hash> hot_deletions;
    std::array< std::unordered_set<std::filesystem::path, std_fs_path_hash>*, 3> deletions_list{ &cold_deletions, &warm_deletions, &hot_deletions };
    for( std::filesystem::path const &zzip_relative_path : zzip_relative_paths ) {
        file_temp temp = temp_of_file( zzip_relative_path );
        if( temp == file_temp::unknown ) {
            continue;
        }
        deletions_list[static_cast<int>( temp ) - 1]->emplace( zzip_relative_path );
    }
    if( cold_deletions.empty() && warm_deletions.empty() && hot_deletions.empty() ) {
        return false;
    }
    bool success = true;
    if( !cold_deletions.empty() ) {
        success = cold_->delete_files( cold_deletions ) && success;
    }
    if( !warm_deletions.empty() ) {
        success = warm_->delete_files( warm_deletions ) && success;
    }
    if( !hot_deletions.empty() ) {
        success = hot_->delete_files( hot_deletions ) && success;
    }
    return success;
}

bool zzip_stack::compact( double bloat_factor )
{
    // preload entries from the stack.
    cold();
    warm();
    hot();

    size_t content_size = 0;
    for( const auto& [file, temp] : path_temp_map_ ) {
        content_size += zzip_of_temp( temp ).get_entry_size( file );
    }

    // Allocate approximately 1/2 of the bloat to the cold file.
    // The other 1/2 divided evenly between hot and warm.
    // So for a bloat factor of 3, the main file may double
    // and each other by 50%.
    size_t cold_max_size = content_size * bloat_factor / 2.0;
    size_t warm_hot_max_size = cold_max_size / 2.0;

    // First, copy unchanged files from warm to cold if warm is too full.
    std::vector<std::filesystem::path> files_to_demote;
    if( warm_->get_content_size() > warm_hot_max_size ) {
        // We don't compact warm, we write cold entries down. Hot entries
        // are kept in hot. We just delete warm entirely.
        for( std::filesystem::path const &entry : warm_->get_entries() ) {
            if( path_temp_map_[entry] == file_temp::warm ) {
                files_to_demote.emplace_back( entry );
            }
        }
        if( !( cold_->copy_files( files_to_demote, *warm_ ) && warm_->clear() ) ) {
            return false;
        }
        for( std::filesystem::path const &entry : files_to_demote ) {
            path_temp_map_[entry] = file_temp::cold;
        }
    }

    // Then copy hot to warm.
    if( hot_->get_content_size() > warm_hot_max_size ) {
        // Definitionally, all hot files are the most recent versions.
        files_to_demote = hot_->get_entries();
        if( !( warm_->copy_files( files_to_demote, *hot_ ) && hot_->clear() ) ) {
            return false;
        }
        for( std::filesystem::path const &entry : files_to_demote ) {
            path_temp_map_[entry] = file_temp::warm;
        }
    }

    // Finally normal compact on cold.
    std::filesystem::path cold_path = path_ / filename_of_temp( path_, file_temp::cold );
    std::filesystem::path tmp_path = cold_path;
    tmp_path.concat( ".tmp" ); // NOLINT(cata-u8-path)
    if( cold_->compact_to( tmp_path, std::max( bloat_factor / 2.0, 1.0 ) ) ) {
        cold_.reset();
        return rename_file( tmp_path, cold_path );
    }
    return true;
}

zzip &zzip_stack::cold() const
{
    if( !cold_ ) {
        cold_ = load_temp( file_temp::cold );
    }
    return *cold_;
}
zzip &zzip_stack::warm() const
{
    if( !warm_ ) {
        warm_ = load_temp( file_temp::warm );
    }
    return *warm_;
}
zzip &zzip_stack::hot() const
{
    if( !hot_ ) {
        hot_ = load_temp( file_temp::hot );
    }
    return *hot_;
}

zzip_stack::file_temp zzip_stack::temp_of_file( std::filesystem::path const &zzip_relative_path )
const
{
    // Don't open all the zzips if we don't have to.
    auto end = path_temp_map_.end();
    decltype( end ) it;
    if( !cold_ ) {
        if( !warm_ ) {
            hot();
            if( ( it = path_temp_map_.find( zzip_relative_path ) ) != end ) {
                return it->second;
            }
            warm();
        }
        if( ( it = path_temp_map_.find( zzip_relative_path ) ) != end ) {
            return it->second;
        }
        cold();
    }
    if( ( it = path_temp_map_.find( zzip_relative_path ) ) != end ) {
        return it->second;
    };
    return file_temp::unknown;
}

zzip &zzip_stack::zzip_of_temp( zzip_stack::file_temp temp ) const
{
    switch( temp ) {
        case file_temp::cold: {
            return cold();
        }
        case file_temp::warm: {
            return warm();
        }
        default: {
            // default unknown to hot
            return hot();
        }
    }
}

std::filesystem::path zzip_stack::filename_of_temp( std::filesystem::path const &folder,
        file_temp temp )
{
    std::filesystem::path zzip_path = folder.filename();
    std::string_view file_suffix;
    switch( temp ) {
        case file_temp::cold:
            file_suffix = kColdSuffix;
            break;
        case file_temp::warm:
            file_suffix = kWarmSuffix;
            break;
        case file_temp::hot:
        default:
            file_suffix = kHotSuffix;
            break;

    }
    zzip_path += file_suffix; // NOLINT(cata-u8-path)
    return zzip_path;
}

std::optional<zzip> zzip_stack::load_temp( file_temp temp ) const
{
    if( temp == file_temp::unknown ) {
        // default unknown to hot
        temp = file_temp::hot;
    }

    std::filesystem::path zzip_path = filename_of_temp( path_, temp );

    std::optional<zzip> z = zzip::load( path_ / zzip_path, dictionary_ );
    if( !z ) {
        return z;
    }
    std::vector<std::filesystem::path> entries = z->get_entries();
    for( std::filesystem::path const &p : entries ) {
        file_temp current_temp = path_temp_map_[p];
        path_temp_map_[p] = static_cast<file_temp>( std::max( static_cast<int>( temp ),
                            static_cast<int>( current_temp ) ) );
    }
    return z;
}
