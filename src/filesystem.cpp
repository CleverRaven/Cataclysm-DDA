#include "filesystem.h"

// FILE I/O
#include <sys/stat.h>
#include <cstdlib>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fstream>
#include <string>
#include <vector>
#include <iterator>
#include <utility>

#include "debug.h"
#include "cata_utility.h"

#if defined(_MSC_VER)
#   include <direct.h>

#   include "wdirent.h"
#else
#   include <dirent.h>
#   include <unistd.h>
#endif

#if defined(_WIN32)
#   include "platform_win.h"
#endif

namespace
{

#if defined(_WIN32)
bool do_mkdir( const std::string &path, const int /*mode*/ )
{
#if defined(_MSC_VER)
    return _mkdir( path.c_str() ) == 0;
#else
    return mkdir( path.c_str() ) == 0;
#endif
}
#else
bool do_mkdir( const std::string &path, const int mode )
{
    return mkdir( path.c_str(), mode ) == 0;
}
#endif

} //anonymous namespace

bool assure_dir_exist( const std::string &path )
{
    return do_mkdir( path, 0777 ) || ( errno == EEXIST && dir_exist( path ) );
}

bool dir_exist( const std::string &path )
{
    DIR *dir = opendir( path.c_str() );
    if( dir != nullptr ) {
        closedir( dir );
        return true;
    }
    return false;
}

bool file_exist( const std::string &path )
{
    struct stat buffer;
    return ( stat( path.c_str(), &buffer ) == 0 );
}

#if defined(_WIN32)
bool remove_file( const std::string &path )
{
    return DeleteFile( path.c_str() ) != 0;
}
#else
bool remove_file( const std::string &path )
{
    return unlink( path.c_str() ) == 0;
}
#endif

#if defined(_WIN32)
bool rename_file( const std::string &old_path, const std::string &new_path )
{
    // Windows rename function does not override existing targets, so we
    // have to remove the target to make it compatible with the Linux rename
    if( file_exist( new_path ) ) {
        if( !remove_file( new_path ) ) {
            return false;
        }
    }

    return rename( old_path.c_str(), new_path.c_str() ) == 0;
}
#else
bool rename_file( const std::string &old_path, const std::string &new_path )
{
    return rename( old_path.c_str(), new_path.c_str() ) == 0;
}
#endif

bool remove_directory( const std::string &path )
{
#if defined(_WIN32)
    return RemoveDirectory( path.c_str() );
#else
    return remove( path.c_str() ) == 0;
#endif
}

const char *cata_files::eol()
{
#if defined(_WIN32)
    // NOLINTNEXTLINE(cata-text-style): carriage return is necessary here
    static const char local_eol[] = "\r\n";
#else
    static const char local_eol[] = "\n";
#endif
    return local_eol;
}

namespace
{

// TODO: move elsewhere.
template <typename T, size_t N>
inline size_t sizeof_array( T const( & )[N] ) noexcept
{
    return N;
}

//--------------------------------------------------------------------------------------------------
// For non-empty path, call function for each file at path.
//--------------------------------------------------------------------------------------------------
template <typename Function>
void for_each_dir_entry( const std::string &path, Function function )
{
    using dir_ptr = DIR*;

    if( path.empty() ) {
        return;
    }

    const dir_ptr root = opendir( path.c_str() );
    if( !root ) {
        const auto e_str = strerror( errno );
        DebugLog( D_WARNING, D_MAIN ) << "opendir [" << path << "] failed with \"" << e_str << "\".";
        return;
    }

    while( const auto entry = readdir( root ) ) {
        function( *entry );
    }
    closedir( root );
}

//--------------------------------------------------------------------------------------------------
#if !defined(_WIN32)
std::string resolve_path( const std::string &full_path )
{
    const auto result_str = realpath( full_path.c_str(), nullptr );
    if( !result_str ) {
        const auto e_str = strerror( errno );
        DebugLog( D_WARNING, D_MAIN ) << "realpath [" << full_path << "] failed with \"" << e_str << "\".";
        return {};
    }

    std::string result( result_str );
    free( result_str );
    return result;
}
#endif

//--------------------------------------------------------------------------------------------------
bool is_directory_stat( const std::string &full_path )
{
    if( full_path.empty() ) {
        return false;
    }

    struct stat result;
    if( stat( full_path.c_str(), &result ) != 0 ) {
        const auto e_str = strerror( errno );
        DebugLog( D_WARNING, D_MAIN ) << "stat [" << full_path << "] failed with \"" << e_str << "\".";
        return false;
    }

    if( S_ISDIR( result.st_mode ) ) {
        // NOLINTNEXTLINE(readability-simplify-boolean-expr)
        return true;
    }

#if !defined(_WIN32)
    if( S_ISLNK( result.st_mode ) ) {
        return is_directory_stat( resolve_path( full_path ) );
    }
#endif

    return false;
}

//--------------------------------------------------------------------------------------------------
// Returns true if entry is a directory, false otherwise.
//--------------------------------------------------------------------------------------------------
#if defined(__MINGW32__)
bool is_directory( const dirent &/*entry*/, const std::string &full_path )
{
    // no dirent::d_type
    return is_directory_stat( full_path );
}
#else
bool is_directory( const dirent &entry, const std::string &full_path )
{
    if( entry.d_type == DT_DIR ) {
        return true;
    }

#if !defined(_WIN32)
    if( entry.d_type == DT_LNK ) {
        return is_directory_stat( resolve_path( full_path ) );
    }
#endif

    if( entry.d_type == DT_UNKNOWN ) {
        return is_directory_stat( full_path );
    }

    return false;
}
#endif

//--------------------------------------------------------------------------------------------------
// Returns true if the name of entry matches "." or "..".
//--------------------------------------------------------------------------------------------------
bool is_special_dir( const dirent &entry )
{
    return !strncmp( entry.d_name, ".",  sizeof( entry.d_name ) - 1 ) ||
           !strncmp( entry.d_name, "..", sizeof( entry.d_name ) - 1 );
}

//--------------------------------------------------------------------------------------------------
// If at_end is true, returns whether entry's name ends in match.
// Otherwise, returns whether entry's name contains match.
//--------------------------------------------------------------------------------------------------
bool name_contains( const dirent &entry, const std::string &match, const bool at_end )
{
    const size_t len_fname = strlen( entry.d_name );
    const size_t len_match = match.length();

    if( len_match > len_fname ) {
        return false;
    }

    const size_t offset = at_end ? ( len_fname - len_match ) : 0;
    return strstr( entry.d_name + offset, match.c_str() ) != nullptr;
}

//--------------------------------------------------------------------------------------------------
// Return every file at root_path matching predicate.
//
// If root_path is empty, search the current working directory.
// If recursive_search is true, search breadth-first into the directory hierarchy.
//
// Results are ordered depth-first with directories searched in lexically order. Furthermore,
// regular files at each level are also ordered lexically by file name.
//
// Files ending in ~ are excluded.
//--------------------------------------------------------------------------------------------------
template <typename Predicate>
std::vector<std::string> find_file_if_bfs( const std::string &root_path,
        const bool recursive_search,
        Predicate predicate )
{
    std::deque<std::string>  directories {!root_path.empty() ? root_path : "."};
    std::vector<std::string> results;

    while( !directories.empty() ) {
        const auto path = std::move( directories.front() );
        directories.pop_front();

        const auto n_dirs    = static_cast<std::ptrdiff_t>( directories.size() );
        const auto n_results = static_cast<std::ptrdiff_t>( results.size() );

        for_each_dir_entry( path, [&]( const dirent & entry ) {
            // exclude special directories.
            if( is_special_dir( entry ) ) {
                return;
            }

            const auto full_path = path + "/" + entry.d_name;

            // don't add files ending in '~'.
            if( full_path.back() == '~' ) {
                return;
            }

            // add sub directories to recursive_search if requested
            const auto is_dir = is_directory( entry, full_path );
            if( recursive_search && is_dir ) {
                directories.emplace_back( full_path );
            }

            // check the file
            if( !predicate( entry, is_dir ) ) {
                return;
            }

            results.emplace_back( full_path );
        } );

        // Keep files and directories to recurse ordered consistently
        // by sorting from the old end to the new end.
        // NOLINTNEXTLINE(cata-use-localized-sorting)
        std::sort( std::begin( directories ) + n_dirs,    std::end( directories ) );
        // NOLINTNEXTLINE(cata-use-localized-sorting)
        std::sort( std::begin( results )     + n_results, std::end( results ) );
    }

    return results;
}

} //anonymous namespace

//--------------------------------------------------------------------------------------------------
std::vector<std::string> get_files_from_path( const std::string &pattern,
        const std::string &root_path, const bool recursive_search, const bool match_extension )
{
    return find_file_if_bfs( root_path, recursive_search, [&]( const dirent & entry, bool ) {
        return name_contains( entry, pattern, match_extension );
    } );
}

/**
 *  Find directories which containing pattern.
 *  @param pattern Search pattern.
 *  @param root_path Search root.
 *  @param recursive_search Be recurse or not.
 *  @return vector or directories without pattern filename at end.
 */
std::vector<std::string> get_directories_with( const std::string &pattern,
        const std::string &root_path, const bool recursive_search )
{
    if( pattern.empty() ) {
        return std::vector<std::string>();
    }

    auto files = find_file_if_bfs( root_path, recursive_search, [&]( const dirent & entry, bool ) {
        return name_contains( entry, pattern, true );
    } );

    // Chop off the file names. Dir path MUST be splitted by '/'
    for( auto &file : files ) {
        file.erase( file.rfind( '/' ), std::string::npos );
    }

    files.erase( std::unique( std::begin( files ), std::end( files ) ), std::end( files ) );

    return files;
}

/**
 *  Find directories which containing pattern.
 *  @param patterns Search patterns.
 *  @param root_path Search root.
 *  @param recursive_search Be recurse or not.
 *  @return vector or directories without pattern filename at end.
 */
std::vector<std::string> get_directories_with( const std::vector<std::string> &patterns,
        const std::string &root_path, const bool recursive_search )
{
    if( patterns.empty() ) {
        return std::vector<std::string>();
    }

    const auto ext_beg = std::begin( patterns );
    const auto ext_end = std::end( patterns );

    auto files = find_file_if_bfs( root_path, recursive_search, [&]( const dirent & entry, bool ) {
        return std::any_of( ext_beg, ext_end, [&]( const std::string & ext ) {
            return name_contains( entry, ext, true );
        } );
    } );

    //chop off the file names
    for( auto &file : files ) {
        file.erase( file.rfind( '/' ), std::string::npos );
    }

    //remove resulting duplicates
    files.erase( std::unique( std::begin( files ), std::end( files ) ), std::end( files ) );

    return files;
}

bool copy_file( const std::string &source_path, const std::string &dest_path )
{
    std::ifstream source_stream( source_path.c_str(), std::ifstream::in | std::ifstream::binary );
    if( !source_stream ) {
        return false;
    }
    return write_to_file( dest_path, [&]( std::ostream & dest_stream ) {
        dest_stream << source_stream.rdbuf();
    }, nullptr ) &&source_stream;
}

std::string ensure_valid_file_name( const std::string &file_name )
{
    const char replacement_char = ' ';
    const std::string invalid_chars = "\\/:?\"<>|";

    // do any replacement in the file name, if needed.
    std::string new_file_name = file_name;
    std::transform( new_file_name.begin(), new_file_name.end(),
    new_file_name.begin(), [&]( const char c ) {
        if( invalid_chars.find( c ) != std::string::npos ) {
            return replacement_char;
        }
        return c;
    } );

    return new_file_name;
}
