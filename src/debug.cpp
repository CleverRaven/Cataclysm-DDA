#include "debug.h"
#include "path_info.h"
#include "output.h"
#include "filesystem.h"
#include "cursesdef.h"
#include "input.h"
#include <time.h>
#include <cassert>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <algorithm>
#include <iosfwd>
#include <iomanip>
#include <fstream>
#include <streambuf>
#include <sys/stat.h>
#include <exception>

#ifndef _MSC_VER
#include <sys/time.h>
#endif

#ifdef BACKTRACE
#if defined _WIN32 || defined _WIN64
#include "platform_win.h"
#include <dbghelp.h>
#else
#include <execinfo.h>
#include <stdlib.h>
#endif
#endif

#ifdef TILES
#include <SDL.h>
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

    DebugLog( D_ERROR, D_MAIN ) << filename << ":" << line << " [" << funcname << "] " << text;

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

    std::string backtrace_instructions =
        string_format(
            _( "See %s for a full stack backtrace" ),
            FILENAMES["debug"]
        );

    fold_and_print( catacurses::stdscr, 0, 0, getmaxx( catacurses::stdscr ), c_light_red,
                    "\n \n" // Looks nicer with some space
                    " %s\n" // translated user string: error notification
                    " -----------------------------------------------------------\n"
                    "%s"
                    " -----------------------------------------------------------\n"
                    " %s\n" // translated user string: space to continue
                    " %s\n" // translated user string: ignore key
#ifdef TILES
                    " %s\n" // translated user string: copy
#endif // TILES
#ifdef BACKTRACE
                    " %s\n" // translated user string: where to find backtrace
#endif
                    , _( "An error has occurred! Written below is the error report:" ),
                    formatted_report,
                    _( "Press <color_white>space bar</color> to continue the game." ),
                    _( "Press <color_white>I</color> (or <color_white>i</color>) to also ignore this particular message in the future." )
#ifdef TILES
                    , _( "Press <color_white>C</color> (or <color_white>c</color>) to copy this message to the clipboard." )
#endif // TILES
#ifdef BACKTRACE
                    , backtrace_instructions
#endif
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
int constexpr module_path_len = 512;
// on some systems the number of frames to capture have to be less than 63 according to the documentation
int constexpr bt_cnt = 62;
int constexpr max_name_len = 512;
// ( max_name_len - 1 ) because SYMBOL_INFO already contains a TCHAR
int constexpr sym_size = sizeof( SYMBOL_INFO ) + ( max_name_len - 1 ) * sizeof( TCHAR );
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
    NullBuf() {}
    int overflow( int c ) override {
        return c;
    }
};

// DebugFile OStream Wrapper                                        {{{2
// ---------------------------------------------------------------------

struct DebugFile {
    DebugFile();
    ~DebugFile();
    void init( const std::string &filename );
    void deinit();

    std::ofstream &currentTime();
    std::ofstream file;
    std::string filename;
};

static NullBuf nullBuf;
static std::ostream nullStream( &nullBuf );

// DebugFile OStream Wrapper                                        {{{2
// ---------------------------------------------------------------------

static DebugFile debugFile;

DebugFile::DebugFile()
{
}

DebugFile::~DebugFile()
{
    if( file.is_open() ) {
        deinit();
    }
}

void DebugFile::deinit()
{
    file << "\n";
    currentTime() << " : Log shutdown.\n";
    file << "-----------------------------------------\n\n";
    file.close();
}

void DebugFile::init( const std::string &filename )
{
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
    file.open( filename.c_str(), std::ios::out | std::ios::app );
    file << "\n\n-----------------------------------------\n";
    currentTime() << " : Starting log.";
    if( rename_failed ) {
        DebugLog( D_ERROR, DC_ALL ) << "Moving the previous log file to " << oldfile << " failed.\n" <<
                                    "Check the file permissions. This program will continue to use the previous log file.";
    }
}

void setupDebug()
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

    debugFile.init( FILENAMES["debug"] );
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

struct time_info {
    int hours;
    int minutes;
    int seconds;
    int mseconds;

    template <typename Stream>
    friend Stream &operator<<( Stream &out, time_info const &t ) {
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

    auto const tt      = time_t {tv.tv_sec};
    auto const current = localtime( &tt );

    return time_info { current->tm_hour, current->tm_min, current->tm_sec,
                       static_cast<int>( tv.tv_usec / 1000.0 + 0.5 )
                     };
}
#endif

std::ofstream &DebugFile::currentTime()
{
    return ( file << get_time() );
}

#ifdef BACKTRACE
// Verify that a string is safe for passing as an argument to addr2line.
// In particular, we want to avoid any characters of significance to the shell.
static bool is_safe_string( const char *start, const char *finish )
{
    static constexpr char safe_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                         "abcdefghijklmnopqrstuvwxyz"
                                         "01234567890_./-";
    using std::begin;
    using std::end;
    const auto is_safe_char =
    [&]( char c ) {
        auto in_safe = std::find( begin( safe_chars ), end( safe_chars ), c );
        return c && in_safe != end( safe_chars );
    };
    return std::all_of( start, finish, is_safe_char );
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
                char const *mod_name = mod_path + mod_len;
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
    out << "\n\t";
#else
    int count = backtrace( tracePtrs, TRACE_SIZE );
    char **funcNames = backtrace_symbols( tracePtrs, count );
    for( int i = 0; i < count; ++i ) {
        out << "\n(" << funcNames[i] << "), ";
    }
    out << "\n\nAttempting to repeat stack trace using debug symbols...\n";
    // Try to print the backtrace again, but this time using addr2line
    // to extract debug info and thus get a more detailed / useful
    // version.  If addr2line is not available this will just fail,
    // which is fine.
    //
    // To make this fast, we need to call addr2line with as many
    // addresses as possible in each commandline.  To that end, we track
    // the binary of the frame and issue a command whenever that
    // changes.
    std::string addresses;
    std::string last_binary_name;

    auto call_addr2line = [&out]( const std::string & binary, const std::string & addresses ) {
        std::string cmd = "addr2line -e " + binary + " -f -C " + addresses;
        FILE *addr2line = popen( cmd.c_str(), "re" );
        if( addr2line == nullptr ) {
            out << "backtrace: popen(addr2line) failed\n";
            // Most likely reason is that addr2line is not installed, so
            // in this case we give up and don't try any more frames.
            return false;
        }
        char buf[1024];
        while( size_t num_bytes = fread( buf, 1, sizeof( buf ), addr2line ) ) {
            out.write( buf, num_bytes );
        }
        if( 0 != pclose( addr2line ) ) {
            out << "backtrace: addr2line failed\n";
            return false;
        }
        return true;
    };

    for( int i = 0; i < count; ++i ) {
        // We want to call addr2Line to convert the address to a
        // useful format
        auto funcName = funcNames[i];
        const auto funcNameEnd = funcName + std::strlen( funcName );
        const auto binaryEnd = std::find( funcName, funcNameEnd, '(' );
        auto addressStart = std::find( funcName, funcNameEnd, '[' );
        auto addressEnd = std::find( addressStart, funcNameEnd, ']' );
        if( binaryEnd == funcNameEnd || addressEnd == funcNameEnd ) {
            out << "backtrace: Could not extract binary name and address from line\n";
            continue;
        }
        ++addressStart;

        if( !is_safe_string( addressStart, addressEnd ) ) {
            out << "backtrace: Address not safe\n";
            continue;
        }

        if( !is_safe_string( funcName, binaryEnd ) ) {
            out << "backtrace: Binary name not safe\n";
            continue;
        }

        std::string binary_name( funcName, binaryEnd );

        if( !last_binary_name.empty() && binary_name != last_binary_name ) {
            if( !call_addr2line( last_binary_name, addresses ) ) {
                addresses.clear();
                break;
            }

            addresses.clear();
        }

        last_binary_name = binary_name;
        addresses += " ";
        addresses.insert( addresses.end(), addressStart, addressEnd );
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
    // Error are always logged, they are important,
    // Messages from D_MAIN come from debugmsg and are equally important.
    if( ( ( lev & debugLevel ) && ( cl & debugClass ) ) || lev & D_ERROR || cl & D_MAIN ) {
        debugFile.file << std::endl;
        debugFile.currentTime() << " ";
        if( lev != debugLevel ) {
            debugFile.file << lev;
        }
        if( cl != debugClass ) {
            debugFile.file << cl;
        }
        debugFile.file << ": ";

        // Backtrace on error.
#ifdef BACKTRACE
        if( lev == D_ERROR ) {
            debug_write_backtrace( debugFile.file );
        }
#endif

        return debugFile.file;
    }
    return nullStream;
}

// vim:tw=72:sw=4:fdm=marker:fdl=0:
