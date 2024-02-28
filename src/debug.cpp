#include "debug.h"

#include <cctype>
// IWYU pragma: no_include <sys/errno.h>
#include <sys/stat.h>
// IWYU pragma: no_include <sys/unistd.h>
#include <clocale>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iterator>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "cached_options.h"
#include "cata_assert.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "color.h"
#include "cursesdef.h"
#include "filesystem.h"
#include "get_version.h"
#include "input.h"
#include "mod_manager.h"
#include "options.h"
#include "output.h"
#include "path_info.h"
#include "point.h"
#include "translations.h"
#include "type_id.h"
#include "ui_manager.h"
#include "worldfactory.h"

#if !defined(_MSC_VER)
#include <sys/time.h>
#endif

#if defined(_WIN32)
#   if 1 // HACK: Hack to prevent reordering of #include "platform_win.h" by IWYU
#       include "platform_win.h"
#   endif
#endif

#if defined(BACKTRACE)
#   if defined(_WIN32)
#       include <dbghelp.h>
#       if defined(LIBBACKTRACE)
#           include <winnt.h>
#       endif
#   elif defined(__ANDROID__)
#       include <unwind.h>
#       include <dlfcn.h>
#   else
#       include <execinfo.h>
#       include <unistd.h>
#   endif
#endif

#if defined(LIBBACKTRACE)
#   include <backtrace.h>
#endif

#if defined(TILES)
#include "sdl_wrappers.h"
#endif // TILES

#if defined(__ANDROID__)
// used by android_version() function for __system_property_get().
#include <sys/system_properties.h>
#include "input_context.h"
#endif

#if (defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)) && !defined(CATA_IS_ON_BSD)
#define CATA_IS_ON_BSD
#endif

// Static defines                                                   {{{1
// ---------------------------------------------------------------------

#if (defined(DEBUG) || defined(_DEBUG)) && !defined(NDEBUG)
static int debugLevel = DL_ALL;
static int debugClass = DC_ALL;
#else
static int debugLevel = D_ERROR;
static int debugClass = D_MAIN;
#endif

/** Set to true when any error is logged. */
static bool error_observed = false;

/** If true, debug messages will be captured,
 * used to test debugmsg calls in the unit tests
 */
static bool capturing = false;
/** сaptured debug messages */
static std::string captured;

#if defined(_WIN32) and defined(LIBBACKTRACE)
// Get the image base of a module from its PE header
static uintptr_t get_image_base( const char *const path )
{
    HANDLE file = CreateFile( path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr );
    if( file == INVALID_HANDLE_VALUE ) {
        return 0;
    }
    on_out_of_scope close_file( [file]() {
        CloseHandle( file );
    } );

    HANDLE mapping = CreateFileMapping( file, nullptr, PAGE_READONLY, 0, 0, nullptr );
    if( mapping == nullptr ) {
        return 0;
    }
    on_out_of_scope close_mapping( [mapping]() {
        CloseHandle( mapping );
    } );

    LONG nt_header_offset = 0;
    {
        LPVOID dos_header_view = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, sizeof( IMAGE_DOS_HEADER ) );
        if( dos_header_view == nullptr ) {
            return 0;
        }
        on_out_of_scope close_dos_header_view( [dos_header_view]() {
            UnmapViewOfFile( dos_header_view );
        } );

        PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>( dos_header_view );
        if( dos_header->e_magic != IMAGE_DOS_SIGNATURE ) {
            return 0;
        }
        nt_header_offset = dos_header->e_lfanew;
    }

    LPVOID pe_header_view = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0,
                                           nt_header_offset + sizeof( IMAGE_NT_HEADERS ) );
    if( pe_header_view == nullptr ) {
        return 0;
    }
    on_out_of_scope close_pe_header_view( [pe_header_view]() {
        UnmapViewOfFile( pe_header_view );
    } );

    PIMAGE_NT_HEADERS nt_header = reinterpret_cast<PIMAGE_NT_HEADERS>(
                                      reinterpret_cast<uintptr_t>( pe_header_view ) + nt_header_offset );
    if( nt_header->Signature != IMAGE_NT_SIGNATURE
        || nt_header->FileHeader.SizeOfOptionalHeader != sizeof( IMAGE_OPTIONAL_HEADER ) ) {
        return 0;
    }
    return nt_header->OptionalHeader.ImageBase;
}
#endif

/**
 * Class for capturing debugmsg,
 * used by capture_debugmsg_during.
 */
class capture_debugmsg
{
    public:
        capture_debugmsg();
        std::string dmsg();
        ~capture_debugmsg();
};

std::string capture_debugmsg_during( const std::function<void()> &func )
{
    capture_debugmsg capture;
    func();
    return capture.dmsg();
}

capture_debugmsg::capture_debugmsg()
{
    capturing = true;
    captured.clear();
}

std::string capture_debugmsg::dmsg()
{
    capturing = false;
    return captured;
}

capture_debugmsg::~capture_debugmsg()
{
    capturing = false;
}

bool debug_has_error_been_observed()
{
    return error_observed;
}

bool debug_mode = false;

namespace debugmode
{
std::unordered_set<debug_filter> enabled_filters;
std::string filter_name( debug_filter value )
{
    // see debug.h for commentary
    switch( value ) {
        // *INDENT-OFF*
        case DF_ACT_BUTCHER: return "DF_ACT_BUTCHER";
        case DF_ACT_EBOOK: return "DF_ACT_EBOOK";
        case DF_ACT_HARVEST: return "DF_ACT_HARVEST";
        case DF_ACT_LOCKPICK: return "DF_ACT_LOCKPICK";
        case DF_ACT_READ: return "DF_ACT_READ";
        case DF_ACT_SAFECRACKING: return "DF_ACT_SAFECRACKING";
        case DF_ACT_SHEARING: return "DF_ACT_SHEARING";
        case DF_ACT_WORKOUT: return "DF_ACT_WORKOUT";
        case DF_ACTIVITY: return "DF_ACTIVITY";
        case DF_ANATOMY_BP: return "DF_ANATOMY_BP";
        case DF_AVATAR: return "DF_AVATAR";
        case DF_BALLISTIC: return "DF_BALLISTIC";
        case DF_CHARACTER: return "DF_CHARACTER";
        case DF_CHAR_CALORIES: return "DF_CHAR_CALORIES";
        case DF_CHAR_HEALTH: return "DF_CHAR_HEALTH";
        case DF_CRAFTING: return "DF_CRAFTING";
        case DF_CREATURE: return "DF_CREATURE";
        case DF_EFFECT: return "DF_EFFECT";
        case DF_EXPLOSION: return "DF_EXPLOSION";
        case DF_FOOD: return "DF_FOOD";
        case DF_GAME: return "DF_GAME";
        case DF_IEXAMINE: return "DF_IEXAMINE";
        case DF_IUSE: return "DF_IUSE";
        case DF_MAP: return "DF_MAP";
        case DF_MATTACK: return "DF_MATTACK";
        case DF_MELEE: return "DF_MELEE";
        case DF_MONMOVE: return "DF_MONMOVE";
        case DF_MONSTER: return "DF_MONSTER";
        case DF_MUTATION: return "DF_MUTATION";
        case DF_NPC: return "DF_NPC";
        case DF_NPC_COMBATAI: return "DF_NPC_COMBATAI";
        case DF_NPC_ITEMAI: return "DF_NPC_ITEMAI";
        case DF_NPC_MOVEAI: return "DF_NPC_MOVEAI";
        case DF_OVERMAP: return "DF_OVERMAP";
        case DF_RADIO: return "DF_RADIO";
        case DF_RANGED: return "DF_RANGED";
        case DF_REQUIREMENTS_MAP: return "DF_REQUIREMENTS_MAP";
        case DF_SOUND: return "DF_SOUND";
        case DF_TALKER: return "DF_TALKER";
        case DF_VEHICLE: return "DF_VEHICLE";
        case DF_VEHICLE_DRAG: return "DF_VEHICLE_DRAG";
        case DF_VEHICLE_MOVE: return "DF_VEHICLE_MOVE";
        // *INDENT-ON*
        case DF_LAST:
        default:
            debugmsg( "Invalid DF_FILTER : %d", value );
            return "DF_INVALID";
    }
}
} // namespace debugmode

struct buffered_prompt_info {
    std::string filename;
    std::string line;
    std::string funcname;
    std::string text;
    bool forced;
};

namespace
{

std::set<std::string> ignored_messages;

} // namespace

// debugmsg prompts that could not be shown immediately are buffered and replayed when catacurses::stdscr is available
// need to use method here to ensure `buffered_prompts` vector is initialized single time
static std::vector<buffered_prompt_info> &buffered_prompts()
{
    static std::vector<buffered_prompt_info> buffered_prompts;
    return buffered_prompts;
}

static void debug_error_prompt(
    const char *filename,
    const char *line,
    const char *funcname,
    const char *text,
    bool force )
{
    cata_assert( catacurses::stdscr );
    cata_assert( filename != nullptr );
    cata_assert( line != nullptr );
    cata_assert( funcname != nullptr );
    cata_assert( text != nullptr );

    std::string msg_key( filename );
    msg_key += line;

    if( !force && ignored_messages.count( msg_key ) > 0 ) {
        return;
    }

    std::string formatted_report =
        string_format( // developer-facing error report. INTENTIONALLY UNTRANSLATED!
            " DEBUG    : %s\n\n"
            " FUNCTION : %s\n"
            " FILE     : %s\n"
            " LINE     : %s\n"
            " VERSION  : %s\n",
            text, funcname, filename, line, getVersionString()
        );

#if defined(BACKTRACE)
    std::string backtrace_instructions =
        string_format(
            _( "See %s for a full stack backtrace" ),
            PATH_INFO::debug()
        );
#endif

    // Create a special debug message UI that does various things to ensure
    // the graphics are correct when the debug message is displayed during a
    // redraw callback.
    ui_adaptor ui( ui_adaptor::debug_message_ui {} );
    const auto init_window = []( ui_adaptor & ui ) {
        ui.position_from_window( catacurses::stdscr );
    };
    init_window( ui );
    ui.on_screen_resize( init_window );
    const std::string message = string_format(
                                    "\n\n" // Looks nicer with some space
                                    " %s\n" // translated user string: error notification
                                    " -----------------------------------------------------------\n"
                                    "%s"
                                    " -----------------------------------------------------------\n"
#if defined(BACKTRACE)
                                    " %s\n" // translated user string: where to find backtrace
#endif
                                    " %s\n" // translated user string: space to continue
                                    " %s\n" // translated user string: ignore key
#if defined(TILES)
                                    " %s\n" // translated user string: copy
#endif // TILES
                                    , _( "An error has occurred!  Written below is the error report:" ),
                                    formatted_report,
#if defined(BACKTRACE)
                                    backtrace_instructions,
#endif
                                    _( "Press <color_white>space bar</color> to continue the game." ),
                                    _( "Press <color_white>I</color> (or <color_white>i</color>) to also ignore this particular message in the future." )
#if defined(TILES)
                                    , _( "Press <color_white>C</color> (or <color_white>c</color>) to copy this message to the clipboard." )
#endif // TILES
                                );
    ui.on_redraw( [&]( const ui_adaptor & ) {
        catacurses::erase();
        fold_and_print( catacurses::stdscr, point_zero, getmaxx( catacurses::stdscr ), c_light_red,
                        "%s", message );
        wnoutrefresh( catacurses::stdscr );
    } );

#if defined(__ANDROID__)
    input_context ctxt( "DEBUG_MSG", keyboard_mode::keycode );
    ctxt.register_manual_key( 'C' );
    ctxt.register_manual_key( 'I' );
    ctxt.register_manual_key( ' ' );
#endif
    for( bool stop = false; !stop; ) {
        ui_manager::redraw();
        switch( inp_mngr.get_input_event().get_first_input() ) {
#if defined(TILES)
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
}

void replay_buffered_debugmsg_prompts()
{
    if( buffered_prompts().empty() || !catacurses::stdscr ) {
        return;
    }
    for( const buffered_prompt_info &prompt : buffered_prompts() ) {
        debug_error_prompt(
            prompt.filename.c_str(),
            prompt.line.c_str(),
            prompt.funcname.c_str(),
            prompt.text.c_str(),
            prompt.forced
        );
    }
    buffered_prompts().clear();
}

struct time_info {
    int hours;
    int minutes;
    int seconds;
    int mseconds;

    template <typename Stream>
    friend Stream &operator<<( Stream &out, const time_info &t ) {
        using char_t = typename Stream::char_type;
        using base   = std::basic_ostream<char_t>;

        static_assert( std::is_base_of_v<base, Stream> );

        out << std::setfill( '0' );
        out << std::setw( 2 ) << t.hours << ':' << std::setw( 2 ) << t.minutes << ':' <<
            std::setw( 2 ) << t.seconds << '.' << std::setw( 3 ) << t.mseconds;

        return out;
    }
};

static time_info get_time() noexcept;

struct repetition_folder {
    const char *m_filename = nullptr;
    const char *m_line = nullptr;
    const char *m_funcname = nullptr;
    std::string m_text;
    time_info m_time;

    static constexpr time_info timeout = { 0, 0, 0, 100 }; // 100ms timeout
    static constexpr int repetition_threshold = 10000;

    int repeat_count = 0;

    bool test( const char *filename, const char *line, const char *funcname,
               const std::string &text ) const {
        return m_filename == filename &&
               m_line == line &&
               m_funcname == funcname &&
               m_text == text &&
               !timed_out();
    }
    void set_time() {
        m_time = get_time();
    }
    void set( const char *filename, const char *line, const char *funcname, const std::string &text ) {
        m_filename = filename;
        m_line = line;
        m_funcname = funcname;
        m_text = text;

        set_time();

        repeat_count = 0;
    }
    void increment_count() {
        ++repeat_count;
        set_time();
    }
    void reset() {
        m_filename = nullptr;
        m_line = nullptr;
        m_funcname = nullptr;

        m_time = time_info{0, 0, 0, 0};

        repeat_count = 0;
    }

    bool timed_out() const {
        const time_info now = get_time();

        const int now_raw = now.mseconds + 1000 * now.seconds + 60000 * now.minutes + 3600000 * now.hours;
        const int old_raw = m_time.mseconds + 1000 * m_time.seconds + 60000 * m_time.minutes + 3600000 *
                            m_time.hours;

        const int timeout_raw = timeout.mseconds + 1000 * timeout.seconds + 60000 * timeout.minutes +
                                3600000 * timeout.hours;

        return ( now_raw - old_raw ) > timeout_raw;
    }
};

static repetition_folder rep_folder;
static void output_repetitions( std::ostream &out );

void realDebugmsg( const char *filename, const char *line, const char *funcname,
                   const std::string &text )
{
    cata_assert( filename != nullptr );
    cata_assert( line != nullptr );
    cata_assert( funcname != nullptr );

    if( capturing ) {
        captured += text;
    } else {

        if( !rep_folder.test( filename, line, funcname, text ) ) {
            DebugLog( D_ERROR, D_MAIN ) << filename << ":" << line << " [" << funcname << "] " << text <<
                                        std::flush;
            rep_folder.set( filename, line, funcname, text );
        } else {
            rep_folder.increment_count();
        }
    }

    if( test_mode ) {
        return;
    }

    // Enable the following to step in debug messages with a debugger
#if 0
    if( isDebuggerActive() ) {
#if defined(_WIN32)
        DebugBreak();
        return;
#elif defined( __linux__ )
        raise( SIGTRAP );
        return;
#endif
    }
#endif //

    // Show excessive repetition prompt once per excessive set
    bool excess_repetition = rep_folder.repeat_count == repetition_folder::repetition_threshold;

    if( !catacurses::stdscr ) {
        buffered_prompts().push_back( {filename, line, funcname, text, false } );
        if( excess_repetition ) {
            // prepend excessive error repetition to original text then prompt
            std::string rep_err =
                "Excessive error repetition detected.  Please file a bug report at https://github.com/CleverRaven/Cataclysm-DDA/issues\n            "
                + text;
            buffered_prompts().push_back( {filename, line, funcname, rep_err, true } );
        }
        return;
    }

    debug_error_prompt( filename, line, funcname, text.c_str(), false );

    if( excess_repetition ) {
        // prepend excessive error repetition to original text then prompt
        std::string rep_err =
            "Excessive error repetition detected.  Please file a bug report at https://github.com/CleverRaven/Cataclysm-DDA/issues\n            "
            + text;
        debug_error_prompt( filename, line, funcname, rep_err.c_str(), true );
        // Do not count this prompt when considering repetition folding
        // Might look weird in the log if the repetitions end exactly after this prompt is displayed.
        rep_folder.set_time();

    }
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

// Debug Includes                                                   {{{2
// ---------------------------------------------------------------------

// Null OStream                                                     {{{2
// ---------------------------------------------------------------------

class NullStream : public std::ostream
{
    public:
        NullStream() : std::ostream( nullptr ) {}
        NullStream( const NullStream & ) = delete;
        NullStream( NullStream && ) = delete;
};

// DebugFile OStream Wrapper                                        {{{2
// ---------------------------------------------------------------------

#if defined(_WIN32)
static time_info get_time() noexcept
{
    SYSTEMTIME time {};

    GetLocalTime( &time );

    return time_info { static_cast<int>( time.wHour ), static_cast<int>( time.wMinute ),
                       static_cast<int>( time.wSecond ), static_cast<int>( time.wMilliseconds )
                     };
}
#else
static time_info get_time() noexcept
{
    timeval tv;
    gettimeofday( &tv, nullptr );

    const time_t tt      = time_t {tv.tv_sec};
    tm current;
    localtime_r( &tt, &current );

    return time_info { current.tm_hour, current.tm_min, current.tm_sec,
                       static_cast<int>( std::lround( tv.tv_usec / 1000.0 ) )
                     };
}
#endif

struct DebugFile {
    DebugFile();
    ~DebugFile();
    void init( DebugOutput, const std::string &filename );
    void deinit();
    std::ostream &get_file();

    // Using shared_ptr for the type-erased deleter support, not because
    // it needs to be shared.
    std::shared_ptr<std::ostream> file;
    std::string filename;
};

// DebugFile OStream Wrapper                                        {{{2
// ---------------------------------------------------------------------

// needs to be inside the method to ensure it's initialized (and only once)
// NOTE: using non-local static variables (defined at top level in cpp file) here is wrong,
// because DebugLog (that uses them) might be called from the constructor of some non-local static entity
// during dynamic initialization phase, when non-local static variables here are
// only zero-initialized
static DebugFile &debugFile()
{
    static DebugFile debugFile;
    return debugFile;
}

DebugFile::DebugFile() = default;

DebugFile::~DebugFile()
{
    deinit();
}

void DebugFile::deinit()
{
    if( file && file.get() != &std::cerr ) {
        output_repetitions( *file );
        *file << "\n";
        *file << get_time() << " : Log shutdown.\n";
        *file << "-----------------------------------------\n\n";
    }
    file.reset();
}

std::ostream &DebugFile::get_file()
{
    if( !file ) {
        file = std::make_shared<std::ostringstream>();
    }
    return *file;
}

void DebugFile::init( DebugOutput output_mode, const std::string &filename )
{
    std::shared_ptr<std::ostringstream> str_buffer = std::dynamic_pointer_cast<std::ostringstream>
            ( file );

    switch( output_mode ) {
        case DebugOutput::std_err:
            file = std::shared_ptr<std::ostream>( &std::cerr, null_deleter() );
            break;
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
                       fs::u8path( filename ), std::ios::out | std::ios::app );
            *file << "\n\n-----------------------------------------\n";
            *file << get_time() << " : Starting log.";
            DebugLog( D_INFO, D_MAIN ) << "Cataclysm DDA version " << getVersionString();
            if( rename_failed ) {
                DebugLog( D_ERROR, DC_ALL ) << "Moving the previous log file to "
                                            << oldfile << " failed.\n"
                                            << "Check the file permissions.  This "
                                            "program will continue to use the "
                                            "previous log file.";
            }
        }
        break;
        default:
            std::cerr << "Unexpected debug output mode " << static_cast<int>( output_mode )
                      << std::endl;
            return;
    }

    if( str_buffer && file ) {
        *file << str_buffer->str();
    }
}

void setupDebug( DebugOutput output_mode )
{
    int level = 0;

#if defined(DEBUG_INFO)
    level |= D_INFO;
#endif

#if defined(DEBUG_WARNING)
    level |= D_WARNING;
#endif

#if defined(DEBUG_ERROR)
    level |= D_ERROR;
#endif

#if defined(DEBUG_PEDANTIC_INFO)
    level |= D_PEDANTIC_INFO;
#endif

    if( level != 0 ) {
        limitDebugLevel( level );
    }

    int cl = 0;

#if defined(DEBUG_ENABLE_MAIN)
    cl |= D_MAIN;
#endif

#if defined(DEBUG_ENABLE_MAP)
    cl |= D_MAP;
#endif

#if defined(DEBUG_ENABLE_MAP_GEN)
    cl |= D_MAP_GEN;
#endif

#if defined(DEBUG_ENABLE_GAME)
    cl |= D_GAME;
#endif

    if( cl != 0 ) {
        limitDebugClass( cl );
    }

    debugFile().init( output_mode, PATH_INFO::debug() );
}

void deinitDebug()
{
    debugFile().deinit();
}

// OStream Operators                                                {{{2
// ---------------------------------------------------------------------

static std::ostream &operator<<( std::ostream &out, DebugLevel lev )
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

static std::ostream &operator<<( std::ostream &out, DebugClass cl )
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
        if( cl & D_MMAP ) {
            out << "MMAP ";
        }
    }
    return out;
}

#if defined(BACKTRACE)
#if !defined(_WIN32) && !defined(__CYGWIN__) && !defined(__ANDROID__) && !defined(LIBBACKTRACE)
// Verify that a string is safe for passing as an argument to addr2line.
// In particular, we want to avoid any characters of significance to the shell.
static bool debug_is_safe_string( const char *start, const char *finish )
{
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static constexpr char safe_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                         "abcdefghijklmnopqrstuvwxyz"
                                         "01234567890_./-+";
    using std::begin;
    using std::end;
    const auto is_safe_char =
    [&]( char c ) {
        const char *in_safe = std::find( begin( safe_chars ), end( safe_chars ), c );
        return c && in_safe != end( safe_chars );
    };
    return std::all_of( start, finish, is_safe_char );
}

static std::string debug_resolve_binary( const std::string &binary, std::ostream &out )
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
        out << "    backtrace: PATH not set\n";
        return binary;
    }

    std::string suffix = "/" + binary;
    for( const std::string &path_elem : string_split( path, ':' ) ) {
        if( path_elem.empty() ) {
            continue;
        }
        std::string candidate = path_elem + suffix;
        if( 0 == access( candidate.c_str(), X_OK ) ) {
            return candidate;
        }
    }

    return binary;
}

static std::optional<uintptr_t> debug_compute_load_offset(
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

    uintptr_t offset_within_symbol = std::stoull( offset_within_symbol_s, nullptr, 0 );
    std::string string_sought = " " + symbol;

    // We need to try calling nm in two different ways, because one
    // works for executables and the other for libraries.
    std::array<const char *, 2> nm_variants = { "nm", "nm -D" };
    for( const char *nm_variant : nm_variants ) {
        std::ostringstream cmd;
        cmd << nm_variant << ' ' << binary << " 2>&1";
        FILE *nm = popen( cmd.str().c_str(), "re" );
        if( !nm ) {
            out << "    backtrace: popen(nm) failed: " << strerror( errno ) << "\n";
            return std::nullopt;
        }

        std::array<char, 1024> buf;
        while( fgets( buf.data(), buf.size(), nm ) ) {
            std::string line( buf.data() );
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

    return std::nullopt;
}
#endif

#if defined(LIBBACKTRACE)
// wrap libbacktrace to use std::function instead of function pointers
using bt_error_callback = std::function<void( const char *, int )>;
using bt_full_callback = std::function<int( uintptr_t, const char *, int, const char * )>;
using bt_syminfo_callback = std::function<void( uintptr_t, const char *, uintptr_t, uintptr_t )>;

static backtrace_state *bt_create_state( const char *const filename, const int threaded,
        const bt_error_callback &cb )
{
    return backtrace_create_state( filename, threaded,
    []( void *const data, const char *const msg, const int errnum ) {
        const bt_error_callback &cb = *reinterpret_cast<const bt_error_callback *>( data );
        cb( msg, errnum );
    },
    const_cast<bt_error_callback *>( &cb ) );
}

#if !defined(_WIN32)
static int bt_full( backtrace_state *const state, int skip, const bt_full_callback &cb_full,
                    const bt_error_callback &cb_error )
{
    using cb_pair = std::pair<const bt_full_callback &, const bt_error_callback &>;
    cb_pair cb { cb_full, cb_error };
    return backtrace_full( state, skip,
                           // backtrace callback
                           []( void *const data, const uintptr_t pc, const char *const filename,
    const int lineno, const char *const function ) -> int {
        cb_pair &cb = *reinterpret_cast<cb_pair *>( data );
        return cb.first( pc, filename, lineno, function );
    },
    // error callback
    []( void *const data, const char *const msg, const int errnum ) {
        cb_pair &cb = *reinterpret_cast<cb_pair *>( data );
        cb.second( msg, errnum );
    },
    &cb );
}
#else
static int bt_pcinfo( backtrace_state *const state, const uintptr_t pc,
                      const bt_full_callback &cb_full, const bt_error_callback &cb_error )
{
    using cb_pair = std::pair<const bt_full_callback &, const bt_error_callback &>;
    cb_pair cb { cb_full, cb_error };
    return backtrace_pcinfo( state, pc,
                             // backtrace callback
                             []( void *const data, const uintptr_t pc, const char *const filename,
    const int lineno, const char *const function ) -> int {
        cb_pair &cb = *reinterpret_cast<cb_pair *>( data );
        return cb.first( pc, filename, lineno, function );
    },
    // error callback
    []( void *const data, const char *const msg, const int errnum ) {
        cb_pair &cb = *reinterpret_cast<cb_pair *>( data );
        cb.second( msg, errnum );
    },
    &cb );
}

static int bt_syminfo( backtrace_state *const state, const uintptr_t addr,
                       const bt_syminfo_callback &cb_syminfo, const bt_error_callback cb_error )
{
    using cb_pair = std::pair<const bt_syminfo_callback &, const bt_error_callback &>;
    cb_pair cb { cb_syminfo, cb_error };
    return backtrace_syminfo( state, addr,
                              // syminfo callback
                              []( void *const data, const uintptr_t pc, const char *const symname,
    const uintptr_t symval, const uintptr_t symsize ) {
        cb_pair &cb = *reinterpret_cast<cb_pair *>( data );
        cb.first( pc, symname, symval, symsize );
    },
    // error callback
    []( void *const data, const char *const msg, const int errnum ) {
        cb_pair &cb = *reinterpret_cast<cb_pair *>( data );
        cb.second( msg, errnum );
    },
    &cb );
}
#endif
#endif

#if defined(_WIN32)
class sym_init
{
    public:
        sym_init() {
            SymInitialize( GetCurrentProcess(), nullptr, TRUE );
        }

        ~sym_init() {
            SymCleanup( GetCurrentProcess() );
        }
};
static std::unique_ptr<sym_init> sym_init_;

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
#if defined(LIBBACKTRACE)
struct backtrace_module_info_t {
    backtrace_state *state = nullptr;
    uintptr_t image_base = 0;
};
static std::map<DWORD64, backtrace_module_info_t> bt_module_info_map;
#endif
#elif !defined(__ANDROID__) && !defined(LIBBACKTRACE)
static constexpr int bt_cnt = 20;
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
static void *bt[bt_cnt];
#endif

#if !defined(_WIN32) && !defined(__ANDROID__) && !defined(LIBBACKTRACE)
static void write_demangled_frame( std::ostream &out, const char *frame )
{
#if defined(__linux__)
    // ./cataclysm(_ZN4game13handle_actionEv+0x47e8) [0xaaaae91e80fc]
    static const std::regex symbol_regex( R"(^(.*)\((.*)\+(0x?[a-f0-9]*)\)\s\[(0x[a-f0-9]+)\]$)" );
    std::cmatch match_result;
    if( std::regex_search( frame, match_result, symbol_regex ) && match_result.size() == 5 ) {
        std::csub_match file_name = match_result[1];
        std::csub_match raw_symbol_name = match_result[2];
        std::csub_match offset = match_result[3];
        std::csub_match address = match_result[4];
        out << "\n    " << file_name.str() << "(" << demangle( raw_symbol_name.str().c_str() ) << "+" <<
            offset.str() << ") [" << address.str() << "]";
    } else {
        out << "\n    " << frame;
    }
#elif defined(MACOSX)
    //1   cataclysm-tiles                     0x0000000102ba2244 _ZL9log_crashPKcS0_ + 608
    static const std::regex symbol_regex( R"(^(.*)(0x[a-f0-9]{16})\s(.*)\s\+\s([0-9]+)$)" );
    std::cmatch match_result;
    if( std::regex_search( frame, match_result, symbol_regex ) && match_result.size() == 5 ) {
        std::csub_match prefix = match_result[1];
        std::csub_match address = match_result[2];
        std::csub_match raw_symbol_name = match_result[3];
        std::csub_match offset = match_result[4];
        out << "\n    " << prefix.str() << address.str() << ' ' << demangle( raw_symbol_name.str().c_str() )
            << " + " << offset.str();
    } else {
        out << "\n    " << frame;
    }
#elif defined(CATA_IS_ON_BSD)
    static const std::regex symbol_regex( R"(^(0x[a-f0-9]+)\s<(.*)\+(0?x?[a-f0-9]*)>\sat\s(.*)$)" );
    std::cmatch match_result;
    if( std::regex_search( frame, match_result, symbol_regex ) && match_result.size() == 5 ) {
        std::csub_match address = match_result[1];
        std::csub_match raw_symbol_name = match_result[2];
        std::csub_match offset = match_result[3];
        std::csub_match file_name = match_result[4];
        out << "\n    " << address.str() << " <" << demangle( raw_symbol_name.str().c_str() ) << "+" <<
            offset.str() << "> at " << file_name.str();
    } else {
        out << "\n    " << frame;
    }
#else
    out << "\n    " << frame;
#endif
}
#endif // !defined(_WIN32) && !defined(__ANDROID__)

#if !defined(__ANDROID__)
void debug_write_backtrace( std::ostream &out )
{
#if defined(LIBBACKTRACE)
    auto bt_full_print = [&out]( const uintptr_t pc, const char *const filename,
    const int lineno, const char *const function ) -> int {
        std::string file = filename ? filename : "[unknown src]";
        size_t src = file.find( "/src/" );
        if( src != std::string::npos )
        {
            file.erase( 0, src );
            file = "…" + file;
        }
        out << "\n    0x" << std::hex << pc << std::dec
            << "    " << file << ":" << lineno
            << "    " << ( function ? demangle( function ) : "[unknown func]" );
        return 0;
    };
#endif

#if defined(_WIN32)
    if( !sym_init_ ) {
        sym_init_ = std::make_unique<sym_init>();
    }
    sym.SizeOfStruct = sizeof( SYMBOL_INFO );
    sym.MaxNameLen = max_name_len;
    // libbacktrace's own backtrace capturing doesn't seem to work on Windows
    const USHORT num_bt = CaptureStackBackTrace( 0, bt_cnt, bt, nullptr );
    const HANDLE proc = GetCurrentProcess();
    for( USHORT i = 0; i < num_bt; ++i ) {
        DWORD64 off;
        out << "\n  #" << i;
        out << "\n    (dbghelp: ";
        if( SymFromAddr( proc, reinterpret_cast<DWORD64>( bt[i] ), &off, &sym ) ) {
            out << demangle( sym.Name ) << "+0x" << std::hex << off << std::dec;
        }
        out << "@" << bt[i];
        const DWORD64 mod_base = SymGetModuleBase64( proc, reinterpret_cast<DWORD64>( bt[i] ) );
        if( mod_base ) {
            out << "[";
            const DWORD mod_len = GetModuleFileName( reinterpret_cast<HMODULE>( mod_base ), mod_path,
                                  module_path_len );
            // mod_len == module_path_len means insufficient buffer
            if( mod_len > 0 && mod_len < module_path_len ) {
                const char *mod_name = mod_path + mod_len;
                for( ; mod_name > mod_path && *( mod_name - 1 ) != '\\'; --mod_name ) {
                }
                out << mod_name;
            } else {
                out << "0x" << std::hex << mod_base << std::dec;
            }
            out << "+0x" << std::hex << reinterpret_cast<uintptr_t>( bt[i] ) - mod_base <<
                std::dec << "]";
        }
        out << "), ";
#if defined(LIBBACKTRACE)
        backtrace_module_info_t bt_module_info;
        if( mod_base ) {
            const auto it = bt_module_info_map.find( mod_base );
            if( it != bt_module_info_map.end() ) {
                bt_module_info = it->second;
            } else {
                const DWORD mod_len = GetModuleFileName( reinterpret_cast<HMODULE>( mod_base ), mod_path,
                                      module_path_len );
                if( mod_len > 0 && mod_len < module_path_len ) {
                    bt_module_info.state = bt_create_state( mod_path, 0,
                                                            // error callback
                    [&out]( const char *const msg, const int errnum ) {
                        out << "\n    (backtrace_create_state failed: errno = " << errnum
                            << ", msg = " << ( msg ? msg : "[no msg]" ) << "),";
                    } );
                    bt_module_info.image_base = get_image_base( mod_path );
                    if( bt_module_info.image_base == 0 ) {
                        out << "\n    (cannot locate image base),";
                    }
                } else {
                    out << "\n    (executable path exceeds " << module_path_len << " chars),";
                }
                bt_module_info_map.emplace( mod_base, bt_module_info );
            }
        } else {
            out << "\n    (unable to get module base address),";
        }
        if( bt_module_info.state && bt_module_info.image_base != 0 ) {
            const uintptr_t de_aslr_pc = reinterpret_cast<uintptr_t>( bt[i] ) - mod_base +
                                         bt_module_info.image_base;
            bt_syminfo( bt_module_info.state, de_aslr_pc,
                        // syminfo callback
                        [&out]( const uintptr_t pc, const char *const symname,
            const uintptr_t symval, const uintptr_t ) {
                out << "\n    (libbacktrace: " << ( symname ? demangle( symname ) : "[unknown symbol]" )
                    << "+0x" << std::hex << pc - symval << std::dec
                    << "@0x" << std::hex << pc << std::dec
                    << "),";
            },
            // error callback
            [&out]( const char *const msg, const int errnum ) {
                out << "\n    (backtrace_syminfo failed: errno = " << errnum
                    << ", msg = " << ( msg ? msg : "[no msg]" )
                    << "),";
            } );
            bt_pcinfo( bt_module_info.state, de_aslr_pc, bt_full_print,
                       // error callback
            [&out]( const char *const msg, const int errnum ) {
                out << "\n    (backtrace_pcinfo failed: errno = " << errnum
                    << ", msg = " << ( msg ? msg : "[no msg]" )
                    << "),";
            } );
        }
#endif
    }
    out << "\n";
#else
#   if defined(LIBBACKTRACE)
    auto bt_error = [&out]( const char *err_msg, int errnum ) {
        out << "\n    libbacktrace error " << errnum << ": " << err_msg;
    };
    static backtrace_state *bt_state = bt_create_state( nullptr, 0, bt_error );
    if( bt_state ) {
        bt_full( bt_state, 0, bt_full_print, bt_error );
        out << std::endl;
    } else {
        out << "\n\n    Failed to initialize libbacktrace\n";
    }
#   elif defined(__CYGWIN__)
    // BACKTRACE is not supported under CYGWIN!
    ( void ) out;
#   else
    int count = backtrace( bt, bt_cnt );
    char **funcNames = backtrace_symbols( bt, count );
    for( int i = 0; i < count; ++i ) {
        write_demangled_frame( out, funcNames[i] );
    }
    out << "\n\n    Attempting to repeat stack trace using debug symbols…\n";
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
            out << "    backtrace: popen(addr2line) failed\n";
            return false;
        }
        std::array<char, 1024> buf;
        while( fgets( buf.data(), buf.size(), addr2line ) ) {
            out.write( "    ", 4 );
            // Strip leading directories for source file path
            const char *search_for = "/src/";
            char *buf_end = buf.data() + strlen( buf.data() );
            char *src = std::find_end( buf.data(), buf_end,
                                       search_for, search_for + strlen( search_for ) );
            if( src == buf_end ) {
                src = buf.data();
            } else {
                out << "…";
            }
            out.write( src, strlen( src ) );
        }
        if( 0 != pclose( addr2line ) ) {
            // Most likely reason is that addr2line is not installed, so
            // in this case we give up and don't try any more frames.
            out << "    backtrace: addr2line failed\n";
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
        // available in bt.

        char *funcName = funcNames[i];
        cata_assert( funcName ); // To appease static analysis
        char *const funcNameEnd = funcName + std::strlen( funcName );
        char *const binaryEnd = std::find( funcName, funcNameEnd, '(' );
        if( binaryEnd == funcNameEnd ) {
            out << "    backtrace: Could not extract binary name from line\n";
            continue;
        }

        if( !debug_is_safe_string( funcName, binaryEnd ) ) {
            out << "    backtrace: Binary name not safe\n";
            continue;
        }

        std::string binary_name( funcName, binaryEnd );
        binary_name = debug_resolve_binary( binary_name, out );

        // For each binary we need to determine its offset relative to
        // its natural load address in order to undo ASLR and pass the
        // correct addresses to addr2line
        auto load_offset = load_offsets.find( binary_name );
        if( load_offset == load_offsets.end() ) {
            char *const symbolNameStart = binaryEnd + 1;
            char *const symbolNameEnd = std::find( symbolNameStart, funcNameEnd, '+' );
            char *const offsetEnd = std::find( symbolNameStart, funcNameEnd, ')' );

            if( symbolNameEnd < offsetEnd && offsetEnd < funcNameEnd ) {
                char *const offsetStart = symbolNameEnd + 1;
                std::string symbol_name( symbolNameStart, symbolNameEnd );
                std::string offset_within_symbol( offsetStart, offsetEnd );

                std::optional<uintptr_t> offset =
                    debug_compute_load_offset( binary_name, symbol_name, offset_within_symbol,
                                               bt[i], out );
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
        addresses.push_back( reinterpret_cast<uintptr_t>( bt[i] ) );
    }

    if( !addresses.empty() ) {
        call_addr2line( last_binary_name, addresses );
    }
    free( funcNames );
#   endif
#endif
}
#endif
#endif

// Probably because there are too many nested #if..#else..#endif in this file
// NDK compiler doesn't understand #if defined(__ANDROID__)..#else..#endif
// So write as two separate #if blocks
#if defined(__ANDROID__)

// The following Android backtrace code was originally written by Eugene Shapovalov
// on https://stackoverflow.com/questions/8115192/android-ndk-getting-the-backtrace
struct android_backtrace_state {
    void **current;
    void **end;
};

static _Unwind_Reason_Code unwindCallback( struct _Unwind_Context *context, void *arg )
{
    android_backtrace_state *state = static_cast<android_backtrace_state *>( arg );
    uintptr_t pc = _Unwind_GetIP( context );
    if( pc ) {
        if( state->current == state->end ) {
            return _URC_END_OF_STACK;
        } else {
            *state->current++ = reinterpret_cast<void *>( pc );
        }
    }
    return _URC_NO_REASON;
}

void debug_write_backtrace( std::ostream &out )
{
    const size_t max = 50;
    void *buffer[max];
    android_backtrace_state state = {buffer, buffer + max};
    _Unwind_Backtrace( unwindCallback, &state );
    const std::size_t count = state.current - buffer;
    // Start from 1: skip debug_write_backtrace ourselves
    for( size_t idx = 1; idx < count && idx < max; ++idx ) {
        const void *addr = buffer[idx];
        Dl_info info;
        if( dladdr( addr, &info ) && info.dli_sname ) {
            out << "#" << std::setw( 2 ) << idx << ": " << addr << " " << demangle( info.dli_sname ) << "\n";
        }
    }
}
#endif

void output_repetitions( std::ostream &out )
{
    // Need to complete the folding
    if( rep_folder.repeat_count > 0 ) {
        if( rep_folder.repeat_count > 1 ) {
            out << std::endl;
            out << "[ Previous repeated " << ( rep_folder.repeat_count - 1 ) << " times ]";
        }
        out << std::endl;
        out << rep_folder.m_time << " ";
        // repetition folding is only done through DebugLog( D_ERROR, D_MAIN )
        out << D_ERROR;
        out << ": ";
        out << rep_folder.m_filename << ":" << rep_folder.m_line << " [" << rep_folder.m_funcname << "] " <<
            rep_folder.m_text << std::flush;
        rep_folder.reset();
    }
}

std::ostream &DebugLog( DebugLevel lev, DebugClass cl )
{
    if( lev & D_ERROR ) {
        error_observed = true;
    }

    // Error are always logged, they are important,
    // Messages from D_MAIN come from debugmsg and are equally important.
    if( ( lev & debugLevel && cl & debugClass ) || lev & D_ERROR || cl & D_MAIN ) {
        std::ostream &out = debugFile().get_file();

        output_repetitions( out );

        out << std::endl;
        out << get_time() << " ";
        out << lev;
        if( cl != debugClass ) {
            out << cl;
        }
        out << ": ";

        // Backtrace on error.
#if defined(BACKTRACE)
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
            out << "(continued from above) " << lev << ": ";
        }
#endif

        return out;
    }

    static NullStream null_stream;
    return null_stream;
}

bool isDebuggerActive()
{
#if defined(_WIN32)
    // From catch.hpp: both _MSVC_VER and __MINGW32__
    return IsDebuggerPresent() != 0;
#elif defined(__linux__)
    // From catch.hpp:
    // The standard POSIX way of detecting a debugger is to attempt to
    // ptrace() the process, but this needs to be done from a child and not
    // this process itself to still allow attaching to this process later
    // if wanted, so is rather heavy. Under Linux we have the PID of the
    // "debugger" (which doesn't need to be gdb, of course, it could also
    // be strace, for example) in /proc/$PID/status, so just get it from
    // there instead.
    std::ifstream in( "/proc/self/status" );
    for( std::string line; std::getline( in, line ); ) {
        static const int PREFIX_LEN = 11;
        //NOLINTNEXTLINE(cata-text-style)
        if( line.compare( 0, PREFIX_LEN, "TracerPid:\t" ) == 0 ) {
            // We're traced if the PID is not 0 and no other PID starts
            // with 0 digit, so it's enough to check for just a single
            // character.
            return line.length() > PREFIX_LEN && line[PREFIX_LEN] != '0';
        }
    }

    return false;
#else
    return false;
#endif
}

std::string game_info::operating_system()
{
#if defined(__ANDROID__)
    return "Android";
#elif defined(_WIN32)
    return "Windows";
#elif defined(__linux__)
    return "Linux";
#elif defined(unix) || defined(__unix__) || defined(__unix) || ( defined(__APPLE__) && defined(__MACH__) ) // unix; BSD; MacOs
#if defined(__APPLE__) && defined(__MACH__)
    // The following include is **only** needed for the TARGET_xxx defines below and is only included if both of the above defines are true.
    // The whole function only relying on compiler defines, it is probably more meaningful to include it here and not mingle with the
    // headers at the top of the .cpp file.
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR == 1
    /* iOS in Xcode simulator */
    return "iOS Simulator";
#elif TARGET_OS_IPHONE == 1
    /* iOS on iPhone, iPad, etc. */
    return "iOS";
#elif TARGET_OS_MAC == 1
    /* OSX */
    return "MacOs";
#endif // TARGET_IPHONE_SIMULATOR
#elif defined(CATA_IS_ON_BSD)
    return "BSD";
#else
    return "Unix";
#endif // __APPLE__
#else
    return "Unknown";
#endif
}

#if !defined(EMSCRIPTEN) && !defined(__CYGWIN__) && !defined (__ANDROID__) && ( defined (__linux__) || defined(unix) || defined(__unix__) || defined(__unix) || ( defined(__APPLE__) && defined(__MACH__) ) || defined(CATA_IS_ON_BSD) ) // linux; unix; MacOs; BSD
class FILEDeleter
{
    public:
        void operator()( FILE *f ) const noexcept {
            pclose( f );
        }
};

/** Execute a command with the shell by using `popen()`.
 * @param command The full command to execute.
 * @note The output buffer is limited to 512 characters.
 * @returns The result of the command (only stdout) or an empty string if there was a problem.
 */
static std::string shell_exec( const std::string &command )
{
    std::vector<char> buffer( 512 );
    std::string output;
    try {
        std::unique_ptr<FILE, FILEDeleter> pipe( popen( command.c_str(), "r" ) );
        if( pipe ) {
            while( fgets( buffer.data(), buffer.size(), pipe.get() ) != nullptr ) {
                output += buffer.data();
            }
        }
    } catch( ... ) {
        output = "";
    }
    return output;
}
#endif

#if defined (__ANDROID__)
/** Get a precise version number for Android systems.
 * @note see:
 *    - https://stackoverflow.com/a/19777977/507028
 *    - https://github.com/pytorch/cpuinfo/blob/master/test/build.prop/galaxy-s7-us.log
 * @returns If successful, a string containing the Android system version, otherwise an empty string.
 */
static std::string android_version()
{
    std::string output;

    // buffer used for the __system_property_get() function.
    // note: according to android sources, it can't be greater than 92 chars (see 'PROP_VALUE_MAX' define in system_properties.h)
    std::vector<char> buffer( 255 );

    static const std::vector<std::pair<std::string, std::string>> system_properties = {
        // The manufacturer of the product/hardware; e.g. "Samsung", this is different than the carrier.
        { "ro.product.manufacturer", "Manufacturer" },
        // The end-user-visible name for the end product; .e.g. "SAMSUNG-SM-G930A" for a Samsung S7.
        { "ro.product.model", "Model" },
        // The Android system version; e.g. "6.0.1"
        { "ro.build.version.release", "Release" },
        // The internal value used by the underlying source control to represent this build; e.g "G930AUCS4APK1" for a Samsung S7 on 6.0.1.
        { "ro.build.version.incremental", "Incremental" },
    };

    for( const auto &entry : system_properties ) {
        int len = __system_property_get( entry.first.c_str(), &buffer[0] );
        std::string value;
        if( len <= 0 ) {
            // failed to get the property
            value = "<unknown>";
        } else {
            value = std::string( buffer.data() );
        }
        output.append( string_format( "%s: %s; ", entry.second, value ) );
    }
    return output;
}

#elif defined(CATA_IS_ON_BSD)

/** Get a precise version number for BSD systems.
 * @note The code shells-out to call `uname -a`.
 * @returns If successful, a string containing the Linux system version, otherwise an empty string.
 */
static std::string bsd_version()
{
    std::string output;
    output = shell_exec( "uname -a" );
    if( !output.empty() ) {
        // remove trailing '\n', if any.
        output.erase( std::remove( output.begin(), output.end(), '\n' ),
                      output.end() );
    }
    return output;
}

#elif defined(__linux__)

/** Get a precise version number for Linux systems.
 * @note The code shells-out to call `lsb_release -a`.
 * @returns If successful, a string containing the Linux system version, otherwise an empty string.
 */
static std::string linux_version()
{
    std::string output;
    output = shell_exec( "lsb_release -a" );
    if( !output.empty() ) {
        // replace '\n' and '\t' in output.
        static const std::vector<std::pair<std::string, std::string>> to_replace = {
            {"\n", "; "},
            {"\t", " "}, // NOLINT(cata-text-style)
        };
        for( const auto &e : to_replace ) {
            std::string::size_type pos;
            while( ( pos = output.find( e.first ) ) != std::string::npos ) {
                output.replace( pos, e.first.length(), e.second );
            }
        }
    }
    return output;
}

#elif defined(__APPLE__) && defined(__MACH__) && !defined(CATA_IS_ON_BSD)

/** Get a precise version number for MacOs systems.
 * @note The code shells-out to call `sw_vers` with various options.
 * @returns If successful, a string containing the MacOS system version, otherwise an empty string.
 */
static std::string mac_os_version()
{
    std::string output;
    static const std::vector<std::pair<std::string,  std::string>> commands = {
        { "sw_vers -productName", "Name" },
        { "sw_vers -productVersion", "Version" },
        { "sw_vers -buildVersion", "Build" },
    };

    for( const auto &entry : commands ) {
        std::string command_result = shell_exec( entry.first );
        if( command_result.empty() ) {
            command_result = "<unknown>";
        } else {
            // remove trailing '\n', if any.
            command_result.erase( std::remove( command_result.begin(), command_result.end(), '\n' ),
                                  command_result.end() );
        }
        output.append( string_format( "%s: %s; ", entry.second, command_result ) );
    }
    return output;
}

#elif defined (_WIN32)

/** Get a precise version number for Windows systems.
 * @note Since Windows 10 all version-related APIs lie about the underlying system if the application is not Manifested (see VerifyVersionInfoA
 *     or GetVersionEx documentation for further explanation). In this function we use the registry or the native RtlGetVersion which both
 *     report correct versions and are compatible down to XP.
 * @returns If successful, a string containing the Windows system version number, otherwise an empty string.
 */
static std::string windows_version()
{
    std::string output;
    HKEY handle_key;
    bool success = RegOpenKeyExA( HKEY_LOCAL_MACHINE, R"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)",
                                  0,
                                  KEY_QUERY_VALUE, &handle_key ) == ERROR_SUCCESS;
    if( success ) {
        DWORD value_type;
        constexpr DWORD c_buffer_size = 512;
        std::vector<BYTE> byte_buffer( c_buffer_size );
        DWORD buffer_size = c_buffer_size;
        DWORD major_version = 0;
        success = RegQueryValueExA( handle_key, "CurrentMajorVersionNumber", nullptr, &value_type,
                                    &byte_buffer[0], &buffer_size ) == ERROR_SUCCESS && value_type == REG_DWORD;
        if( success ) {
            major_version = *reinterpret_cast<const DWORD *>( &byte_buffer[0] );
            output.append( std::to_string( major_version ) );
        }
        if( success ) {
            buffer_size = c_buffer_size;
            success = RegQueryValueExA( handle_key, "CurrentMinorVersionNumber", nullptr, &value_type,
                                        &byte_buffer[0], &buffer_size ) == ERROR_SUCCESS && value_type == REG_DWORD;
            if( success ) {
                const DWORD minor_version = *reinterpret_cast<const DWORD *>( &byte_buffer[0] );
                output.append( "." );
                output.append( std::to_string( minor_version ) );
            }
        }
        if( success ) {
            buffer_size = c_buffer_size;
            success = RegQueryValueExA( handle_key, "CurrentBuildNumber", nullptr, &value_type, &byte_buffer[0],
                                        &buffer_size ) == ERROR_SUCCESS && value_type == REG_SZ;
            if( success ) {
                output.append( "." );
                output.append( std::string( reinterpret_cast<char *>( &byte_buffer[0] ) ) );
            }
            if( success ) {
                buffer_size = c_buffer_size;
                success = RegQueryValueExA( handle_key, "UBR", nullptr, &value_type, &byte_buffer[0],
                                            &buffer_size ) == ERROR_SUCCESS && value_type == REG_DWORD;
                if( success ) {
                    output.append( "." );
                    output.append( std::to_string( *reinterpret_cast<const DWORD *>( &byte_buffer[0] ) ) );
                }
            }

            // Applies to both Windows 10 and Windows 11
            if( major_version == 10 ) {
                buffer_size = c_buffer_size;
                // present in Windows 10 version >= 20H2 (aka 2009) and Windows 11
                success = RegQueryValueExA( handle_key, "DisplayVersion", nullptr, &value_type, &byte_buffer[0],
                                            &buffer_size ) == ERROR_SUCCESS && value_type == REG_SZ;

                if( !success ) {
                    // only accurate in Windows 10 version <= 2009
                    buffer_size = c_buffer_size;
                    success = RegQueryValueExA( handle_key, "ReleaseId", nullptr, &value_type, &byte_buffer[0],
                                                &buffer_size ) == ERROR_SUCCESS && value_type == REG_SZ;
                }

                if( success ) {
                    output.append( " (" );
                    output.append( std::string( reinterpret_cast<char *>( byte_buffer.data() ) ) );
                    output.append( ")" );
                }
            }
        }

        RegCloseKey( handle_key );
    }

    if( !success ) {
#if defined (__MINGW32__) || defined (__MINGW64__) || defined (__CYGWIN__) || defined (MSYS2)
        output = "MINGW/CYGWIN/MSYS2 on unknown Windows version";
#else
        output.clear();
        using RtlGetVersion = LONG( WINAPI * )( PRTL_OSVERSIONINFOW );
        const HMODULE handle_ntdll = GetModuleHandleA( "ntdll" );
        if( handle_ntdll != nullptr ) {
            // Use union-based type-punning to convert function pointer
            // type without gcc warnings.
            union {
                RtlGetVersion p;
                FARPROC q;
            } rtl_get_version_func;
            rtl_get_version_func.q = GetProcAddress( handle_ntdll, "RtlGetVersion" );
            if( rtl_get_version_func.p != nullptr ) {
                RTL_OSVERSIONINFOW os_version_info = RTL_OSVERSIONINFOW();
                os_version_info.dwOSVersionInfoSize = sizeof( RTL_OSVERSIONINFOW );
                if( rtl_get_version_func.p( &os_version_info ) == 0 ) { // NT_STATUS_SUCCESS = 0
                    output.append( string_format( "%i.%i %i", os_version_info.dwMajorVersion,
                                                  os_version_info.dwMinorVersion, os_version_info.dwBuildNumber ) );
                }
            }
        }
#endif
    }
    return output;
}
#endif // Various OS define tests

std::string game_info::operating_system_version()
{
#if defined(__ANDROID__)
    return android_version();
#elif defined(CATA_IS_ON_BSD)
    return bsd_version();
#elif defined(__linux__)
    return linux_version();
#elif defined(__APPLE__) && defined(__MACH__) && !defined(CATA_IS_ON_BSD)
    return mac_os_version();
#elif defined(_WIN32)
    return windows_version();
#else
    return "<unknown>";
#endif
}

std::string game_info::bitness()
{
    if( sizeof( void * ) == 8 ) {
        return "64-bit";
    }

    if( sizeof( void * ) == 4 ) {
        return "32-bit";
    }

    return "Unknown";
}

std::string game_info::game_version()
{
    return getVersionString();
}

std::string game_info::graphics_version()
{
#if defined(TILES)
    return "Tiles";
#else
    return "Curses";
#endif
}

std::string game_info::mods_loaded()
{
    if( world_generator->active_world == nullptr ) {
        return "No active world";
    }

    const std::vector<mod_id> &mod_ids = world_generator->active_world->active_mod_order;
    if( mod_ids.empty() ) {
        return "No loaded mods";
    }

    std::vector<std::string> mod_names;
    mod_names.reserve( mod_ids.size() );
    std::transform( mod_ids.begin(), mod_ids.end(),
    std::back_inserter( mod_names ), []( const mod_id & mod ) -> std::string {
        // e.g. "Dark Days Ahead [dda]".
        return string_format( "%s [%s]", mod->name(), mod->ident.str() );
    } );

    return string_join( mod_names, ",\n    " ); // note: 4 spaces for a slight offset.
}

std::string game_info::game_report()
{
    std::string os_version = operating_system_version();
    if( os_version.empty() ) {
        os_version = "<unknown>";
    }
    std::stringstream report;

    std::string lang = get_option<std::string>( "USE_LANG" );
    std::string lang_translated;
    for( const options_manager::id_and_option &vItem : options_manager::get_lang_options() ) {
        if( vItem.first == lang ) {
            lang_translated = vItem.second.translated();
            break;
        }
    }

    report <<
           "- OS: " << operating_system() << "\n" <<
           "    - OS Version: " << os_version << "\n" <<
           "- Game Version: " << game_version() << " [" << bitness() << "]\n" <<
           "- Graphics Version: " << graphics_version() << "\n" <<
           "- Game Language: " << lang_translated << " [" << lang << "]\n" <<
           "- Mods loaded: [\n    " << mods_loaded() << "\n]\n";

    return report.str();
}

// vim:tw=72:sw=4:fdm=marker:fdl=0:
