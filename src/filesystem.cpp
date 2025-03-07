#include "filesystem.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <functional>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

#if defined(EMSCRIPTEN)
#include <emscripten.h>
#endif

#include "cata_utility.h"
#include "debug.h"

#if defined(_WIN32)
#   include "platform_win.h"

static const std::array invalid_names = {
    std::string_view( "CON" ),
    std::string_view( "PRN" ),
    std::string_view( "AUX" ),
    std::string_view( "NUL" ),
    std::string_view( "COM0" ),
    std::string_view( "COM1" ),
    std::string_view( "COM2" ),
    std::string_view( "COM3" ),
    std::string_view( "COM4" ),
    std::string_view( "COM5" ),
    std::string_view( "COM6" ),
    std::string_view( "COM7" ),
    std::string_view( "COM8" ),
    std::string_view( "COM9" ),
    std::string_view( "COM¹" ),
    std::string_view( "COM²" ),
    std::string_view( "COM³" ),
    std::string_view( "LPT0" ),
    std::string_view( "LPT1" ),
    std::string_view( "LPT2" ),
    std::string_view( "LPT3" ),
    std::string_view( "LPT4" ),
    std::string_view( "LPT5" ),
    std::string_view( "LPT6" ),
    std::string_view( "LPT7" ),
    std::string_view( "LPT8" ),
    std::string_view( "LPT9" ),
    std::string_view( "LPT¹" ),
    std::string_view( "LPT²" ),
    std::string_view( "LPT³" )
};
#endif

static void setFsNeedsSync()
{
#if defined(EMSCRIPTEN)
    EM_ASM( window.setFsNeedsSync(); );
#endif
}

static const std::string invalid_chars = "\\/:?\"<>|";

bool assure_dir_exist( const std::string &path )
{
    return assure_dir_exist( std::filesystem::u8path( path ) );
}

bool assure_dir_exist( const cata_path &path )
{
    return assure_dir_exist( path.get_unrelative_path() );
}

bool assure_dir_exist( const std::filesystem::path &path )
{
    setFsNeedsSync();
    std::error_code ec;
    bool exists{false};
    bool created{false};
    const std::filesystem::path p = std::filesystem::u8path( as_norm_dir( path ) );
    bool is_dir{ std::filesystem::is_directory( p, ec ) };
    if( !is_dir ) {
        exists = std::filesystem::exists( p, ec );
        if( !exists ) {
            if( !is_lexically_valid( p ) ) {
                return false;
            }
            created = std::filesystem::create_directories( p, ec );
            if( !created ) {
                // TEMPORARY until we drop VS2019 support
                // VS2019 std::filesystem doesn't handle trailing slashes well in this
                // situation.  We think this is a bug.  Work around it by checking again
                created = std::filesystem::exists( p, ec );
            }
        }
    }
    bool ret = is_dir || ( !exists && created );
    return !ec && ret;
}

bool dir_exist( const std::string &path )
{
    return dir_exist( std::filesystem::u8path( path ) );
}

bool dir_exist( const std::filesystem::path &path )
{
    return std::filesystem::is_directory( path );
}

bool file_exist( const std::string &path )
{
    return file_exist( std::filesystem::u8path( path ) );
}

bool file_exist( const std::filesystem::path &path )
{
    return std::filesystem::exists( path ) && !std::filesystem::is_directory( path );
}

bool file_exist( const cata_path &path )
{
    const std::filesystem::path unrelative_path = path.get_unrelative_path();
    return std::filesystem::exists( unrelative_path ) &&
           !std::filesystem::is_directory( unrelative_path );
}

std::string as_norm_dir( const std::string &path )
{
    std::filesystem::path dir = std::filesystem::u8path( path ) / std::filesystem::path{};
    std::filesystem::path norm = dir.lexically_normal();
    std::string ret = norm.generic_u8string();
    if( "." == ret ) {
        ret = "./"; // TODO Change the many places that use strings instead of paths
    }
    return ret;
}

std::string as_norm_dir( const std::filesystem::path &path )
{
    return as_norm_dir( path.u8string() );
}

bool remove_file( const std::string &path )
{
    return remove_file( std::filesystem::u8path( path ) );
}

bool remove_file( const cata_path &path )
{
    return remove_file( path.get_unrelative_path() );
}

bool remove_file( const std::filesystem::path &path )
{
    setFsNeedsSync();
    std::error_code ec;
    return std::filesystem::remove( path, ec );
}

bool rename_file( const std::string &old_path, const std::string &new_path )
{
    return rename_file( std::filesystem::u8path( old_path ), std::filesystem::u8path( new_path ) );
}

bool rename_file( const cata_path &old_path, const cata_path &new_path )
{
    return rename_file( old_path.get_unrelative_path(), new_path.get_unrelative_path() );
}

bool rename_file( const std::filesystem::path &old_path, const std::filesystem::path &new_path )
{
    setFsNeedsSync();
    std::error_code ec;
    std::filesystem::rename( old_path, new_path, ec );
    return !ec;
}

std::string abs_path( const std::string &path )
{
    return abs_path( std::filesystem::u8path( path ) ).generic_u8string();
}

std::filesystem::path abs_path( const std::filesystem::path &path )
{
    return std::filesystem::absolute( path );
}

bool remove_directory( const std::string &path )
{
    return remove_directory( std::filesystem::u8path( path ) );
}

bool remove_directory( const std::filesystem::path &path )
{
    std::error_code ec;
    setFsNeedsSync();
    return std::filesystem::remove( path, ec );
}

const char *cata_files::eol()
{
#if defined(_WIN32)
    // carriage return is necessary here
    // NOLINTNEXTLINE(cata-text-style, modernize-avoid-c-arrays)
    static const char local_eol[] = "\r\n";
#else
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static const char local_eol[] = "\n";
#endif
    return local_eol;
}

std::string read_entire_file( const std::string &path )
{
    std::ifstream infile( std::filesystem::u8path( path ), std::ifstream::in | std::ifstream::binary );
    return std::string( std::istreambuf_iterator<char>( infile ),
                        std::istreambuf_iterator<char>() );
}

std::string read_entire_file( const std::filesystem::path &path )
{
    std::ifstream infile( path, std::ifstream::in | std::ifstream::binary );
    return std::string( std::istreambuf_iterator<char>( infile ),
                        std::istreambuf_iterator<char>() );
}

namespace
{
//--------------------------------------------------------------------------------------------------
// For non-empty path, call function for each file at path.
//--------------------------------------------------------------------------------------------------
template <typename Function>
void for_each_dir_entry( const std::filesystem::path &path, Function &&function )
{
    if( path.empty() ) {
        return;
    }

    std::error_code ec;
    auto dir_iter = std::filesystem::directory_iterator( path, ec );
    if( ec ) {
        std::string e_str = ec.message();
        DebugLog( D_WARNING, D_MAIN ) << "opendir [" << path.generic_u8string() << "] failed with \"" <<
                                      e_str << "\".";
        return;
    }
    for( const std::filesystem::directory_entry &dir_entry : dir_iter ) {
        function( dir_entry );
    }
}

//--------------------------------------------------------------------------------------------------
// Returns true if entry is a directory, false otherwise.
//--------------------------------------------------------------------------------------------------
bool is_directory( const std::filesystem::directory_entry &entry, const std::string &full_path )
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

bool is_directory( const std::filesystem::directory_entry &entry )
{
    // We do an extra dance here because directory_entry might not have a cached valid stat result.
    std::error_code ec;
    bool result = entry.is_directory( ec );

    if( ec ) {
        std::string e_str = ec.message();
        DebugLog( D_WARNING, D_MAIN ) << "stat [" << entry.path().generic_u8string() << "] failed with \""
                                      << e_str << "\".";
        return false;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
// If at_end is true, returns whether entry's name ends in match.
// Otherwise, returns whether entry's name contains match.
//--------------------------------------------------------------------------------------------------
bool name_contains( const std::filesystem::directory_entry &entry, const std::string &match,
                    const bool at_end,
                    const bool match_path )
{
    std::string entry_name;
    if( match_path ) {
        entry_name = entry.path().u8string();
    } else {
        entry_name = entry.path().filename().u8string();
    }
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
        const std::filesystem::path path = std::filesystem::u8path( directories.front() );
        directories.pop_front();

        const std::ptrdiff_t n_dirs    = static_cast<std::ptrdiff_t>( directories.size() );
        const std::ptrdiff_t n_results = static_cast<std::ptrdiff_t>( results.size() );

        // We could use std::filesystem::recursive_directory_iterator maybe
        for_each_dir_entry( path, [&]( const std::filesystem::directory_entry & entry ) {
            const std::string full_path = entry.path().generic_u8string();

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

template <typename Predicate>
std::vector<cata_path> find_file_if_bfs( const cata_path &root_path,
        const bool recursive_search,
        Predicate predicate )
{
    cata_path norm_root_path = root_path.lexically_normal();
    std::deque<std::filesystem::path>  directories{ norm_root_path.get_unrelative_path() };
    std::vector<std::filesystem::path> results;

    while( !directories.empty() ) {
        const std::filesystem::path path = std::move( directories.front() );
        directories.pop_front();

        const std::ptrdiff_t n_dirs = static_cast<std::ptrdiff_t>( directories.size() );
        const std::ptrdiff_t n_results = static_cast<std::ptrdiff_t>( results.size() );

        // We could use std::filesystem::recursive_directory_iterator maybe
        for_each_dir_entry( path, [&]( const std::filesystem::directory_entry & entry ) {
            const std::filesystem::path &full_path = entry.path();

            // don't add files ending in '~'.
            std::filesystem::path filename = full_path.filename();
            if( !filename.empty() && filename.generic_u8string().back() == '~' ) {
                return;
            }

            // add sub directories to recursive_search if requested
            const bool is_dir = is_directory( entry );
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
        std::sort( std::begin( directories ) + n_dirs, std::end( directories ) );
        // NOLINTNEXTLINE(cata-use-localized-sorting)
        std::sort( std::begin( results ) + n_results, std::end( results ) );
    }

    std::vector<cata_path> cps;
    cps.reserve( results.size() );
    for( const std::filesystem::path &result_path : results ) {
        // Re-relativize paths against the root.
        cps.emplace_back( norm_root_path.get_logical_root(),
                          result_path.lexically_relative( norm_root_path.get_logical_root_path() ) );
    }
    return cps;
}

} //anonymous namespace

//--------------------------------------------------------------------------------------------------
std::vector<std::string> get_files_from_path( const std::string &pattern,
        const std::string &root_path, const bool recursive_search, const bool match_extension )
{
    return find_file_if_bfs( root_path,
                             recursive_search, [&]( const std::filesystem::directory_entry & entry,
    bool ) {
        return name_contains( entry, pattern, match_extension, false );
    } );
}
std::vector<cata_path> get_files_from_path( const std::string &pattern,
        const cata_path &root_path, const bool recursive_search, const bool match_extension )
{
    return find_file_if_bfs( root_path,
                             recursive_search, [&]( const std::filesystem::directory_entry & entry,
    bool ) {
        return name_contains( entry, pattern, match_extension, false );
    } );
}

std::vector<std::string> get_files_from_path_with_path_exclusion( const std::string &pattern,
        const std::string &pattern_clash,
        const std::string &root_path, const bool recursive_search, const bool match_extension )
{
    return find_file_if_bfs( root_path,
                             recursive_search, [&]( const std::filesystem::directory_entry & entry,
    bool ) {
        return name_contains( entry, pattern, match_extension, false ) &&
               !name_contains( entry, pattern_clash, false, true );
    } );
}
std::vector<cata_path> get_files_from_path_with_path_exclusion( const std::string &pattern,
        const std::string &pattern_clash,
        const cata_path &root_path, const bool recursive_search, const bool match_extension )
{
    return find_file_if_bfs( root_path,
                             recursive_search, [&]( const std::filesystem::directory_entry & entry,
    bool ) {
        return name_contains( entry, pattern, match_extension, false ) &&
               !name_contains( entry, pattern_clash, false, true );
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

    auto files = find_file_if_bfs( root_path,
                                   recursive_search, [&]( const std::filesystem::directory_entry & entry,
    bool ) {
        return name_contains( entry, pattern, true, false );
    } );

    // Chop off the file names. Dir path MUST be splitted by '/'
    for( auto &file : files ) {
        file.erase( file.rfind( '/' ), std::string::npos );
    }

    files.erase( std::unique( std::begin( files ), std::end( files ) ), std::end( files ) );

    return files;
}

std::vector<cata_path> get_directories_with( const std::string &pattern,
        const cata_path &root_path, const bool recursive_search )
{
    if( pattern.empty() ) {
        return {};
    }

    auto files = find_file_if_bfs( root_path,
                                   recursive_search, [&]( const std::filesystem::directory_entry & entry,
    bool ) {
        return name_contains( entry, pattern, true, false );
    } );

    // Chop off the file names. Dir path MUST be splitted by '/'
    for( cata_path &file : files ) {
        file = file.parent_path();
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

    auto files = find_file_if_bfs( root_path,
                                   recursive_search, [&]( const std::filesystem::directory_entry & entry,
    bool ) {
        return std::any_of( ext_beg, ext_end, [&]( const std::string & ext ) {
            return name_contains( entry, ext, true, false );
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

std::vector<cata_path> get_directories_with( const std::vector<std::string> &patterns,
        const cata_path &root_path, const bool recursive_search )
{
    if( patterns.empty() ) {
        return {};
    }

    const auto ext_beg = std::begin( patterns );
    const auto ext_end = std::end( patterns );

    auto files = find_file_if_bfs( root_path,
                                   recursive_search, [&]( const std::filesystem::directory_entry & entry,
    bool ) {
        return std::any_of( ext_beg, ext_end, [&]( const std::string & ext ) {
            return name_contains( entry, ext, true, false );
        } );
    } );

    //chop off the file names
    for( cata_path &file : files ) {
        file = cata_path{ file.get_logical_root(), file.get_relative_path().parent_path() };
    }

    //remove resulting duplicates
    files.erase( std::unique( std::begin( files ), std::end( files ) ), std::end( files ) );

    return files;
}

/**
 * Finds all directories within given path
 * @param root_path Search root.
 * @param recursive_search Be recurse or not.
 * @return vector or directories without pattern filename at end.
 */
std::vector<std::string> get_directories( const std::string &root_path,
        const bool recursive_search )
{
    auto files = find_file_if_bfs( root_path,
                                   recursive_search, [&]( const std::filesystem::directory_entry & entry,
    bool ) {
        return dir_exist( entry.path() );
    } );

    files.erase( std::unique( std::begin( files ), std::end( files ) ), std::end( files ) );

    return files;
}

/**
 * Finds all directories within given path
 * @param root_path Search root.
 * @param recursive_search Be recurse or not.
 * @return vector or directories without pattern filename at end.
 */
std::vector<cata_path> get_directories( const cata_path &root_path, const bool recursive_search )
{
    auto files = find_file_if_bfs( root_path,
                                   recursive_search, [&]( const std::filesystem::directory_entry & entry,
    bool ) {
        return dir_exist( entry.path() );
    } );

    files.erase( std::unique( std::begin( files ), std::end( files ) ), std::end( files ) );

    return files;
}

bool copy_file( const std::string &source_path, const std::string &dest_path )
{
    std::ifstream source_stream( std::filesystem::u8path( source_path ),
                                 std::ifstream::in | std::ifstream::binary );
    if( !source_stream ) {
        return false;
    }
    return write_to_file( dest_path, [&]( std::ostream & dest_stream ) {
        dest_stream << source_stream.rdbuf();
    }, nullptr ) &&source_stream;
}

bool copy_file( const cata_path &source_path, const cata_path &dest_path )
{
    std::ifstream source_stream( source_path.get_unrelative_path(),
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

#if defined(_WIN32)
bool is_lexically_valid( const std::filesystem::path &path )
{
    // Windows has strict rules for file naming
    std::filesystem::path rel = path.relative_path();
    // "Do not end a file or directory name with a space or a period."
    // https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file#naming-conventions
    for( auto &it : rel ) {
        std::string item = it.generic_u8string();
        if( item == "." || item == ".." ) {
            continue;
        }
        if( !item.empty() && ( item.back() == ' ' || item.back() == '.' ) ) {
            return false;
        }
        for( auto &it : item ) {
            if( invalid_chars.find( it ) != std::string::npos ) {
                return false;
            }
        }
        for( auto &inv : invalid_names ) {
            if( item == inv ) {
                return false;
            }
            if( item.rfind( inv, 0 ) == 0 &&
                item[ inv.size() ] == '.'
              ) {
                return false;
            }
        }
    }
    return true;
}
#endif
