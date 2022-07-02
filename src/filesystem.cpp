#include "filesystem.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

#include "cata_utility.h"
#include "debug.h"

#if defined(_WIN32)
#   include "platform_win.h"
#endif

bool assure_dir_exist( const std::string &path )
{
    std::error_code ec;
    fs::path p( path );
    bool ret = fs::is_directory( p, ec ) || ( !fs::exists( p, ec ) && fs::create_directories( p, ec ) );
    return !ec && ret;
}

bool dir_exist( const std::string &path )
{
    return fs::is_directory( path );
}

bool file_exist( const std::string &path )
{
    return fs::exists( path ) && !fs::is_directory( path );
}

bool remove_file( const std::string &path )
{
    std::error_code ec;
    return fs::remove( path, ec );
}

bool rename_file( const std::string &old_path, const std::string &new_path )
{
    std::error_code ec;
    fs::rename( old_path, new_path, ec );
    return !ec;
}

bool remove_directory( const std::string &path )
{
    std::error_code ec;
    return fs::remove( path, ec );
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

std::string read_entire_file( const std::string &path )
{
    cata::ifstream infile( fs::u8path( path ), std::ifstream::in | std::ifstream::binary );
    return std::string( std::istreambuf_iterator<char>( infile ),
                        std::istreambuf_iterator<char>() );
}

namespace
{
//--------------------------------------------------------------------------------------------------
// For non-empty path, call function for each file at path.
//--------------------------------------------------------------------------------------------------
template <typename Function>
void for_each_dir_entry( const std::string &path, Function function )
{
    if( path.empty() ) {
        return;
    }

    std::error_code ec;
    auto dir_iter = fs::directory_iterator( path, ec );
    if( ec ) {
        std::string e_str = ec.message();
        DebugLog( D_WARNING, D_MAIN ) << "opendir [" << path << "] failed with \"" << e_str << "\".";
        return;
    }
    for( const ghc::filesystem::directory_entry &dir_entry : dir_iter ) {
        function( dir_entry );
    }
}

//--------------------------------------------------------------------------------------------------
// Returns true if entry is a directory, false otherwise.
//--------------------------------------------------------------------------------------------------
bool is_directory( const fs::directory_entry &entry, const std::string &full_path )
{
    // We do an extra dance here because directory_entry might not have a cached valid stat result.
    std::error_code ec;
    bool result = entry.is_directory( ec );

    if( ec ) {
        std::string e_str = ec.message();
        DebugLog( D_WARNING, D_MAIN ) << "stat [" << full_path << "] failed with \"" << e_str << "\".";
        return false;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
// If at_end is true, returns whether entry's name ends in match.
// Otherwise, returns whether entry's name contains match.
//--------------------------------------------------------------------------------------------------
bool name_contains( const fs::directory_entry &entry, const std::string &match, const bool at_end )
{
    std::string entry_name = entry.path().filename().u8string();
    const size_t len_fname = entry_name.length();
    const size_t len_match = match.length();

    if( len_match > len_fname ) {
        return false;
    }

    const size_t offset = at_end ? ( len_fname - len_match ) : 0;
    return strstr( entry_name.c_str() + offset, match.c_str() ) != nullptr;
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

        const std::ptrdiff_t n_dirs    = static_cast<std::ptrdiff_t>( directories.size() );
        const std::ptrdiff_t n_results = static_cast<std::ptrdiff_t>( results.size() );

        // We could use fs::recursive_directory_iterator maybe
        for_each_dir_entry( path, [&]( const fs::directory_entry & entry ) {
            const auto full_path = entry.path().generic_u8string();

            // don't add files ending in '~'.
            if( full_path.back() == '~' ) {
                return;
            }

            // add sub directories to recursive_search if requested
            const bool is_dir = is_directory( entry, full_path );
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
    return find_file_if_bfs( root_path, recursive_search, [&]( const fs::directory_entry & entry,
    bool ) {
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

    auto files = find_file_if_bfs( root_path, recursive_search, [&]( const fs::directory_entry & entry,
    bool ) {
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

    auto files = find_file_if_bfs( root_path, recursive_search, [&]( const fs::directory_entry & entry,
    bool ) {
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
    cata::ifstream source_stream( fs::u8path( source_path ),
                                  std::ifstream::in | std::ifstream::binary );
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

#if defined(_WIN32) && defined(__GLIBCXX__)
// GLIBCXX does not offer the wchar_t extension for fstream paths
std::string cata::_details::path_to_native( const fs::path &p )
{
    if( GetACP() == 65001 ) { // utf-8 code page
        return p.u8string();
    } else {
        return wstr_to_native( p.wstring() );
    }
}
#elif defined(_WIN32)
std::wstring cata::_details::path_to_native( const fs::path &p )
{
    return p.wstring();
}
#else
std::string cata::_details::path_to_native( const fs::path &p )
{
    return p.u8string();
}
#endif
