#include "debug.h"

#include <sys/stat.h>
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iosfwd>
#include <streambuf>

#include "cursesdef.h"
#include "filesystem.h"
#include "get_version.h"
#include "input.h"
#include "output.h"
#include "path_info.h"

#ifndef _MSC_VER
#include <sys/time.h>
#endif

#if (defined _WIN32 || defined _WIN64)
#if 1 // Hack to prevent reordering of #include "platform_win.h" by IWYU
#include "platform_win.h"
#endif
#endif

#ifdef BACKTRACE
#if (defined _WIN32 || defined _WIN64)
#include <dbghelp.h>
#else
#include <execinfo.h>
#include <unistd.h>
#include <cstdlib>
#endif
#endif

#ifdef TILES
#   if defined(_MSC_VER) && defined(USE_VCPKG)
#       include <SDL2/SDL.h>
#   else
#       include <SDL.h>
#   endif
#endif // TILES

// Static defines                                                   {{{1
// ---------------------------------------------------------------------

#if (defined(DEBUG) || defined(_DEBUG)) && !defined(NDEBUG)
static int debugLevel = DL_ALL;
static int debugClass = DC_ALL;
#else
static int debugLevel = D_ERROR;
static int debugClass = D_MAIN;
#endif

extern bool test_mode;

/** When in @ref test_mode will be set if any debugmsg are emitted */
bool test_dirty = false;

bool debug_mode = false;

namespace
{

std::set<std::string> ignored_messages;

}

void realDebugmsg( const char *filename, const char *line, const char *funcname,
                   const std::string &text )
{
    assert( filename != nullptr );
    assert( line != nullptr );
    assert( funcname != nullptr );

    if( test_mode ) {
        test_dirty = true;
        std::cerr << filename << ":" << line << " [" << funcname << "] " << text << std::endl;
        return;
    }

    DebugLog( D_ERROR, D_MAIN ) << filename << ":" << line << " [" << funcname << "] "
                                << text << std::flush;

    std::string msg_key( filename );
    msg_key += line;

    if( ignored_messages.count( msg_key ) > 0 ) {
        return;
    }

    if( !catacurses::stdscr ) {
        std::cerr << text << std::endl;
        abort();
    }

    std::string formatted_report =
        string_format( // developer-facing error report. INTENTIONALLY UNTRANSLATED!
            " DEBUG    : %s\n \n"
            " FUNCTION : %s\n"
            " FILE     : %s\n"
            " LINE     : %s\n",
            text.c_str(), funcname, filename, line
        );

#ifdef BACKTRACE
    std::string backtrace_instructions =
        string_format(
            _( "See %s for a full stack backtrace" ),
            FILENAMES["debug"]
        );
#endif

    fold_and_print( catacurses::stdscr, 0, 0, getmaxx( catacurses::stdscr ), c_light_red,
                    "\n \n" // Looks nicer with some space
                    " %s\n" // translated user string: error notification
                    " -----------------------------------------------------------\n"
                    "%s"
                    " -----------------------------------------------------------\n"
#ifdef BACKTRACE
                    " %s\n" // translated user string: where to find backtrace
#endif
                    " %s\n" // translated user string: space to continue
                    " %s\n" // translated user string: ignore key
#ifdef TILES
                    " %s\n" // translated user string: copy
#endif // TILES
                    , _( "An error has occurred! Written below is the error report:" ),
                    formatted_report,
#ifdef BACKTRACE
                    backtrace_instructions,
#endif
                    _( "Press <color_white>space bar</color> to continue the game." ),
                    _( "Press <color_white>I</color> (or <color_white>i</color>) to also ignore this particular message in the future." )
#ifdef TILES
                    , _( "Press <color_white>C</color> (or <color_white>c</color>) to copy this message to the clipboard." )
#endif // TILES
                  );

#ifdef __ANDROID__
    input_context ctxt( "DEBUG_MSG" );
    ctxt.register_manual_key( 'C' );
    ctxt.register_manual_key( 'I' );
    ctxt.register_manual_key( ' ' );
#endif
    for( bool stop = false; !stop; ) {
        switch( inp_mngr.get_input_event().get_first_input() ) {
#ifdef TILES
            case 'c':
            case 'C':
                SDL_SetClipboardText( formatted_report.c_str() );
                break;
#endif // TILES
            case 'i':
            case 'I':
                ignored_messages.insert( msg_key );
            /* fallthrough */
            case ' ':
                stop = true;
                break;
        }
    }

    werase( catacurses::stdscr );
    catacurses::refresh();
}

// Normal functions                                                 {{{1
// ---------------------------------------------------------------------

void limitDebugLevel( int level_bitmask )
{
    DebugLog( DL_ALL, DC_ALL ) << "Set debug level to: " << level_bitmask;
    debugLevel = level_bitmask;
}

void limitDebugClass( int class_bitmask )
{
    DebugLog( DL_ALL, DC_ALL ) << "Set debug class to: " << class_bitmask;
    debugClass = class_bitmask;
}

// Debug only                                                       {{{1
// ---------------------------------------------------------------------

#ifdef BACKTRACE
#if defined _WIN32 || defined _WIN64
constexpr int module_path_len = 512;
// on some systems the number of frames to capture have to be less than 63 according to the documentation
constexpr int bt_cnt = 62;
constexpr int max_name_len = 512;
// ( max_name_len - 1 ) because SYMBOL_INFO already contains a TCHAR
constexpr int sym_size = sizeof( SYMBOL_INFO ) + ( max_name_len - 1 ) * sizeof( TCHAR );
static char mod_path[module_path_len];
static PVOID bt[bt_cnt];
static struct {
    alignas( SYMBOL_INFO ) char storage[sym_size];
} sym_storage;
static SYMBOL_INFO &sym = reinterpret_cast<SYMBOL_INFO &>( sym_storage );
#else
#define TRACE_SIZE 20

void *tracePtrs[TRACE_SIZE];
#endif
#endif

// Debug Includes                                                   {{{2
// ---------------------------------------------------------------------

// Null OStream                                                     {{{2
// ---------------------------------------------------------------------

struct NullBuf : public std::streambuf {
    NullBuf() = default;
    int overflow( int c ) override {
        return c;
    }
};

// DebugFile OStream Wrapper                                        {{{2
// ---------------------------------------------------------------------

struct time_info {
    int hours;
    int minutes;
    int seconds;
    int mseconds;

    template <typename Stream>
    friend Stream &operator<<( Stream &out, const time_info &t ) {
        using char_t = typename Stream::char_type;
        using base   = std::basic_ostream<char_t>;

        static_assert( std::is_base_of<base, Stream>::value, "" );

        out << std::setfill( '0' );
        out << std::setw( 2 ) << t.hours << ':' << std::setw( 2 ) << t.minutes << ':' <<
            std::setw( 2 ) << t.seconds << '.' << std::setw( 3 ) << t.mseconds;

        return out;
    }
};

#ifdef _MSC_VER
time_info get_time() noexcept
{
    SYSTEMTIME time {};

    GetLocalTime( &time );

    return time_info { static_cast<int>( time.wHour ), static_cast<int>( time.wMinute ),
                       static_cast<int>( time.wSecond ), static_cast<int>( time.wMilliseconds )
                     };
}
#else
time_info get_time() noexcept
{
    timeval tv;
    gettimeofday( &tv, nullptr );

    const auto tt      = time_t {tv.tv_sec};
    const auto current = localtime( &tt );

    return time_info { current->tm_hour, current->tm_min, current->tm_sec,
                       static_cast<int>( tv.tv_usec / 1000.0 + 0.5 )
                     };
}
#endif

struct DebugFile {
    DebugFile();
    ~DebugFile();
    void init( DebugOutput, const std::string &filename );
    void deinit();

    // Using shared_ptr for the type-erased deleter support, not because
    // it needs to be shared.
    std::shared_ptr<std::ostream> file;
    std::string filename;
};

static NullBuf nullBuf;
static std::ostream nullStream( &nullBuf );

// DebugFile OStream Wrapper                                        {{{2
// ---------------------------------------------------------------------

static DebugFile debugFile;

DebugFile::DebugFile() = default;

DebugFile::~DebugFile()
{
    deinit();
}

void DebugFile::deinit()
{
    if( file && file.get() != &std::cerr ) {
        *file << "\n";
        *file << get_time() << " : Log shutdown.\n";
        *file << "-----------------------------------------\n\n";
    }
    file.reset();
}

void DebugFile::init( DebugOutput output_mode, const std::string &filename )
{
    switch( output_mode ) {
        case DebugOutput::std_err:
            file = std::shared_ptr<std::ostream>( &std::cerr, null_deleter() );
            return;
        case DebugOutput::file: {
            this->filename = filename;
            const std::string oldfile = filename + ".prev";
            bool rename_failed = false;
            struct stat buffer;
            if( stat( filename.c_str(), &buffer ) == 0 ) {
                // Continue with the old log file if it's smaller than 1 MiB
                if( buffer.st_size >= 1024 * 1024 ) {
                    rename_failed = !rename_file( filename, oldfile );
                }
            }
            file = std::make_shared<std::ofstream>(
                       filename.c_str(), std::ios::out | std::ios::app );
            *file << "\n\n-----------------------------------------\n";
            *file << get_time() << " : Starting log.";
            DebugLog( D_INFO, D_MAIN ) << "Cataclysm DDA version " << getVersionString();
            if( rename_failed ) {
                DebugLog( D_ERROR, DC_ALL ) << "Moving the previous log file to "
                                            << oldfile << " failed.\n"
                                            << "Check the file permissions. This "
                                            "program will continue to use the "
                                            "previous log file.";
            }
        }
        return;
        default:
            std::cerr << "Unexpected debug output mode " << static_cast<int>( output_mode )
                      << std::endl;
    }
}

void setupDebug( DebugOutput output_mode )
{
    int level = 0;

#ifdef DEBUG_INFO
    level |= D_INFO;
#endif

#ifdef DEBUG_WARNING
    level |= D_WARNING;
#endif

#ifdef DEBUG_ERROR
    level |= D_ERROR;
#endif

#ifdef DEBUG_PEDANTIC_INFO
    level |= D_PEDANTIC_INFO;
#endif

    if( level != 0 ) {
        limitDebugLevel( level );
    }

    int cl = 0;

#ifdef DEBUG_ENABLE_MAIN
    cl |= D_MAIN;
#endif

#ifdef DEBUG_ENABLE_MAP
    cl |= D_MAP;
#endif

#ifdef DEBUG_ENABLE_MAP_GEN
    cl |= D_MAP_GEN;
#endif

#ifdef DEBUG_ENABLE_GAME
    cl |= D_GAME;
#endif

    if( cl != 0 ) {
        limitDebugClass( cl );
    }

    debugFile.init( output_mode, FILENAMES["debug"] );
}

void deinitDebug()
{
    debugFile.deinit();
}

// OStream Operators                                                {{{2
// ---------------------------------------------------------------------

std::ostream &operator<<( std::ostream &out, DebugLevel lev )
{
    if( lev != DL_ALL ) {
        if( lev & D_INFO ) {
            out << "INFO ";
        }
        if( lev & D_WARNING ) {
            out << "WARNING ";
        }
        if( lev & D_ERROR ) {
            out << "ERROR ";
        }
        if( lev & D_PEDANTIC_INFO ) {
            out << "PEDANTIC ";
        }
    }
    return out;
}

std::ostream &operator<<( std::ostream &out, DebugClass cl )
{
    if( cl != DC_ALL ) {
        if( cl & D_MAIN ) {
            out << "MAIN ";
        }
        if( cl & D_MAP ) {
            out << "MAP ";
        }
        if( cl & D_MAP_GEN ) {
            out << "MAP_GEN ";
        }
        if( cl & D_NPC ) {
            out << "NPC ";
        }
        if( cl & D_GAME ) {
            out << "GAME ";
        }
        if( cl & D_SDL ) {
            out << "SDL ";
        }
    }
    return out;
}

#ifdef BACKTRACE
// Verify that a string is safe for passing as an argument to addr2line.
// In particular, we want to avoid any characters of significance to the shell.
bool debug_is_safe_string( const char *start, const char *finish )
{
    static constexpr char safe_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                         "abcdefghijklmnopqrstuvwxyz"
                                         "01234567890_./-+";
    using std::begin;
    using std::end;
    const auto is_safe_char =
    [&]( char c ) {
        auto in_safe = std::find( begin( safe_chars ), end( safe_chars ), c );
        return c && in_safe != end( safe_chars );
    };
    return std::all_of( start, finish, is_safe_char );
}

std::string debug_resolve_binary( const std::string &binary, std::ostream &out )
{
    if( binary.find( '/' ) != std::string::npos ) {
        // The easy case, where we have a path to the binary
        return binary;
    }
    // If the binary name has no slashes then it was found via PATH
    // lookup, and we need to do the same to pass the correct name
    // to addr2line.  An alternative would be to use /proc/self/exe,
    // but that's Linux-specific.
    // Obviously this will not work in all situations, but it will
    // usually do the right thing.
    const char *path = std::getenv( "PATH" );
    if( !path ) {
        // Should be impossible, but I want to avoid segfaults
        // in the crash handler.
        out << "\tbacktrace: PATH not set\n";
        return binary;
    }

    for( const std::string &path_elem : string_split( path, ':' ) ) {
        if( path_elem.empty() ) {
            continue;
        }
        std::string candidate = path_elem + "/" + binary;
        if( 0 == access( candidate.c_str(), X_OK ) ) {
            return candidate;
        }
    }

    return binary;
}

cata::optional<uintptr_t> debug_compute_load_offset(
    const std::string &binary, const std::string &symbol,
    const std::string &offset_within_symbol_s, void *address, std::ostream &out )
{
    // I don't know a good way to compute this offset.  This
    // seems to work, but I'm not sure how portable it is.
    //
    // backtrace_symbols has provided the address of a symbol as loaded
    // in memory.  We use nm to compute the address of the same symbol
    // in the binary file, and take the difference of the two.
    //
    // There are platform-specific functions which can do similar
    // things (e.g. dladdr1 in GNU libdl) but this approach might
    // perhaps be more portable and adds no link-time dependencies.

    uintptr_t offset_within_symbol = std::stoull( offset_within_symbol_s, 0, 0 );
    std::string string_sought = " " + symbol;

    // We need to try calling nm in two different ways, because one
    // works for executables and the other for libraries.
    const char *nm_variants[] = { "nm", "nm -D" };
    for( const char *nm_variant : nm_variants ) {
        std::ostringstream cmd;
        cmd << nm_variant << ' ' << binary << " 2>&1";
        FILE *nm = popen( cmd.str().c_str(), "re" );
        if( !nm ) {
            out << "\tbacktrace: popen(nm) failed\n";
            return cata::nullopt;
        }

        char buf[1024];
        while( fgets( buf, sizeof( buf ), nm ) ) {
            std::string line( buf );
            while( !line.empty() && isspace( line.end()[-1] ) ) {
                line.erase( line.end() - 1 );
            }
            if( string_ends_with( line, string_sought ) ) {
                std::istringstream line_is( line );
                uintptr_t symbol_address;
                line_is >> std::hex >> symbol_address;
                if( line_is ) {
                    pclose( nm );
                    return reinterpret_cast<uintptr_t>( address ) -
                           ( symbol_address + offset_within_symbol );
                }
            }
        }

        pclose( nm );
    }

    return cata::nullopt;
}

void debug_write_backtrace( std::ostream &out )
{
#if defined _WIN32 || defined _WIN64
    sym.SizeOfStruct = sizeof( SYMBOL_INFO );
    sym.MaxNameLen = max_name_len;
    USHORT num_bt = CaptureStackBackTrace( 0, bt_cnt, bt, NULL );
    HANDLE proc = GetCurrentProcess();
    for( USHORT i = 0; i < num_bt; ++i ) {
        DWORD64 off;
        out << "\n\t(";
        if( SymFromAddr( proc, ( DWORD64 ) bt[i], &off, &sym ) ) {
            out << sym.Name << "+0x" << std::hex << off << std::dec;
        }
        out << "@" << bt[i];
        DWORD64 mod_base = SymGetModuleBase64( proc, ( DWORD64 ) bt[i] );
        if( mod_base ) {
            out << "[";
            DWORD mod_len = GetModuleFileName( ( HMODULE ) mod_base, mod_path, module_path_len );
            // mod_len == module_path_len means insufficient buffer
            if( mod_len > 0 && mod_len < module_path_len ) {
                const char *mod_name = mod_path + mod_len;
                for( ; mod_name > mod_path && *( mod_name - 1 ) != '\\'; --mod_name ) {
                }
                out << mod_name;
            } else {
                out << "0x" << std::hex << mod_base << std::dec;
            }
            out << "+0x" << std::hex << ( uintptr_t ) bt[i] - mod_base << std::dec << "]";
        }
        out << "), ";
    }
    out << "\n";
#else
    int count = backtrace( tracePtrs, TRACE_SIZE );
    char **funcNames = backtrace_symbols( tracePtrs, count );
    for( int i = 0; i < count; ++i ) {
        out << "\n\t" << funcNames[i];
    }
    out << "\n\n\tAttempting to repeat stack trace using debug symbols...\n";
    // Try to print the backtrace again, but this time using addr2line
    // to extract debug info and thus get a more detailed / useful
    // version.  If addr2line is not available this will just fail,
    // which is fine.
    //
    // To make this fast, we need to call addr2line with as many
    // addresses as possible in each commandline.  To that end, we track
    // the binary of the frame and issue a command whenever that
    // changes.
    std::vector<uintptr_t> addresses;
    std::map<std::string, uintptr_t> load_offsets;
    std::string last_binary_name;

    auto call_addr2line = [&out, &load_offsets]( const std::string & binary,
    const std::vector<uintptr_t> &addresses ) {
        const auto load_offset_it = load_offsets.find( binary );
        const uintptr_t load_offset = ( load_offset_it == load_offsets.end() ) ? 0 :
                                      load_offset_it->second;

        std::ostringstream cmd;
        cmd.imbue( std::locale::classic() );
        cmd << "addr2line -i -e " << binary << " -f -C" << std::hex;
        for( uintptr_t address : addresses ) {
            cmd << " 0x" << ( address - load_offset );
        }
        cmd << " 2>&1";
        FILE *addr2line = popen( cmd.str().c_str(), "re" );
        if( addr2line == nullptr ) {
            out << "\tbacktrace: popen(addr2line) failed\n";
            return false;
        }
        char buf[1024];
        while( fgets( buf, sizeof( buf ), addr2line ) ) {
            out.write( "\t", 1 );
            // Strip leading directories for source file path
            char search_for[] = "/src/";
            auto buf_end = buf + strlen( buf );
            auto src = std::find_end( buf, buf_end,
                                      search_for, search_for + strlen( search_for ) );
            if( src == buf_end ) {
                src = buf;
            } else {
                out.write( "...", 3 );
            }
            out.write( src, strlen( src ) );
        }
        if( 0 != pclose( addr2line ) ) {
            // Most likely reason is that addr2line is not installed, so
            // in this case we give up and don't try any more frames.
            out << "\tbacktrace: addr2line failed\n";
            return false;
        }
        return true;
    };

    for( int i = 0; i < count; ++i ) {
        // An example string from backtrace_symbols is
        //   ./cataclysm-tiles(_Z21debug_write_backtraceRSo+0x3d) [0x55ddebfa313d]
        // From that we need to extract the binary name, the symbol
        // name, and the offset within the symbol.  We don't need to
        // extract the address (the last thing) because that's already
        // available in tracePtrs.

        auto funcName = funcNames[i];
        const auto funcNameEnd = funcName + std::strlen( funcName );
        const auto binaryEnd = std::find( funcName, funcNameEnd, '(' );
        if( binaryEnd == funcNameEnd ) {
            out << "\tbacktrace: Could not extract binary name from line\n";
            continue;
        }

        if( !debug_is_safe_string( funcName, binaryEnd ) ) {
            out << "\tbacktrace: Binary name not safe\n";
            continue;
        }

        std::string binary_name( funcName, binaryEnd );
        binary_name = debug_resolve_binary( binary_name, out );

        // For each binary we need to determine its offset relative to
        // its natural load address in order to undo ASLR and pass the
        // correct addresses to addr2line
        auto load_offset = load_offsets.find( binary_name );
        if( load_offset == load_offsets.end() ) {
            const auto symbolNameStart = binaryEnd + 1;
            const auto symbolNameEnd = std::find( symbolNameStart, funcNameEnd, '+' );
            const auto offsetEnd = std::find( symbolNameStart, funcNameEnd, ')' );

            if( symbolNameEnd < offsetEnd && offsetEnd < funcNameEnd ) {
                const auto offsetStart = symbolNameEnd + 1;
                std::string symbol_name( symbolNameStart, symbolNameEnd );
                std::string offset_within_symbol( offsetStart, offsetEnd );

                cata::optional<uintptr_t> offset =
                    debug_compute_load_offset( binary_name, symbol_name, offset_within_symbol,
                                               tracePtrs[i], out );
                if( offset ) {
                    load_offsets.emplace( binary_name, *offset );
                }
            }
        }

        if( !last_binary_name.empty() && binary_name != last_binary_name ) {
            // We have reached the end of the sequence of addresses
            // within this binary, so call addr2line before proceeding
            // to the next binary.
            if( !call_addr2line( last_binary_name, addresses ) ) {
                addresses.clear();
                break;
            }

            addresses.clear();
        }

        last_binary_name = binary_name;
        addresses.push_back( reinterpret_cast<uintptr_t>( tracePtrs[i] ) );
    }

    if( !addresses.empty() ) {
        call_addr2line( last_binary_name, addresses );
    }
    free( funcNames );
#endif
}
#endif

std::ostream &DebugLog( DebugLevel lev, DebugClass cl )
{
    // If debugging has not been initialized then stop
    // (we could instead use std::cerr in this case?)
    if( !debugFile.file ) {
        return nullStream;
    }

    // Error are always logged, they are important,
    // Messages from D_MAIN come from debugmsg and are equally important.
    if( ( ( lev & debugLevel ) && ( cl & debugClass ) ) || lev & D_ERROR || cl & D_MAIN ) {
        std::ostream &out = *debugFile.file;
        out << std::endl;
        out << get_time() << " ";
        out << lev;
        if( cl != debugClass ) {
            out << cl;
        }
        out << ": ";

        // Backtrace on error.
#ifdef BACKTRACE
        // Push the first retrieved value back by a second so it won't match.
        static time_t next_backtrace = time( nullptr ) - 1;
        time_t now = time( nullptr );
        if( lev == D_ERROR && now >= next_backtrace ) {
            out << "(error message will follow backtrace)";
            debug_write_backtrace( out );
            time_t after = time( nullptr );
            // Cool down for 60s between backtrace emissions.
            next_backtrace = after + 60;
            out << "Backtrace emission took " << after - now << " seconds." << std::endl;
        }
#endif

        return out;
    }
    return nullStream;
}

// vim:tw=72:sw=4:fdm=marker:fdl=0:
