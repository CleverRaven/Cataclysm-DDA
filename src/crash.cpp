#include "crash.h"

// IWYU pragma: no_include <sys/signal.h>

#if defined(BACKTRACE)

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>

#if !(defined(WIN32) || defined(TILES) || defined(CYGWIN))
#include <curses.h>
#endif

#if defined(TILES)
#include "sdl_wrappers.h"
#endif

#if defined(_WIN32)
#if 1 // HACK: Hack to prevent reordering of #include "platform_win.h" by IWYU
#include "platform_win.h"
#endif
#include <dbghelp.h>
#endif

#include "debug.h"
#include "get_version.h"
#include "path_info.h"

// signal handlers are expected to have C linkage, and only use the
// common subset of C & C++
extern "C" {

#if defined(_WIN32)
    static void dump_to( const char *file )
    {
        HANDLE handle = CreateFile( file, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL, nullptr );
        // TODO: call from a separate process as suggested by the documentation
        // TODO: capture stack trace and pass as parameter as suggested by the documentation
        MiniDumpWriteDump( GetCurrentProcess(),
                           GetCurrentProcessId(),
                           handle,
                           MiniDumpNormal,
                           nullptr, nullptr, nullptr );
        CloseHandle( handle );
    }
#endif

    static void log_crash( const char *type, const char *msg )
    {
        // This implementation is not technically async-signal-safe for many
        // reasons, including the memory allocations and the SDL message box.
        // But it should usually work in practice, unless for example the
        // program segfaults inside malloc.
#if defined(_WIN32)
        dump_to( ".core" );
#endif
        const std::string crash_log_file = PATH_INFO::crash();
        std::ostringstream log_text;
#if defined(__ANDROID__)
        // At this point, Android JVM is already doomed
        // No further UI interaction (including the SDL message box)
        // Show a dialogue at next launch
        log_text << "VERSION: " << getVersionString()
                 << '\n' << type << ' ' << msg;
#else
        log_text << "The program has crashed."
                 << "\nSee the log file for a stack trace."
                 << "\nCRASH LOG FILE: " << crash_log_file
                 << "\nVERSION: " << getVersionString()
                 << "\nTYPE: " << type
                 << "\nMESSAGE: " << msg;
#if defined(TILES)
        if( SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Error",
                                      log_text.str().c_str(), nullptr ) != 0 ) {
            log_text << "\nError creating SDL message box: " << SDL_GetError();
        }
#endif
#endif
        log_text << "\nSTACK TRACE:\n";
        debug_write_backtrace( log_text );
        std::cerr << log_text.str();
        FILE *file = fopen( crash_log_file.c_str(), "w" );
        if( file ) {
            size_t written = fwrite( log_text.str().data(), 1, log_text.str().size(), file );
            if( written < log_text.str().size() ) {
                std::cerr << "Error: writing to log file failed: " << strerror( errno ) << "\n";
            }
            if( fclose( file ) ) {
                std::cerr << "Error: closing log file failed: " << strerror( errno ) << "\n";
            }
        }
#if defined(__ANDROID__)
        // Create a placeholder dummy file "config/crash.log.prompt"
        // to let the app show a dialog box at next start
        file = fopen( ( crash_log_file + ".prompt" ).c_str(), "w" );
        if( file ) {
            fwrite( "0", 1, 1, file );
            fclose( file );
        }
#endif
    }

    static void signal_handler( int sig )
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
        static_cast<void>( signal( sig, SIG_DFL ) );
#pragma GCC diagnostic pop
        const char *msg;
        switch( sig ) {
            case SIGSEGV:
                msg = "SIGSEGV: Segmentation fault";
                break;
            case SIGILL:
                msg = "SIGILL: Illegal instruction";
                break;
            case SIGABRT:
                msg = "SIGABRT: Abnormal termination";
                break;
            case SIGFPE:
                msg = "SIGFPE: Arithmetical error";
                break;
#if defined(SIGBUS)
            case SIGBUS:
                msg = "SIGBUS: Bus error";
                break;
#endif
            default:
                return;
        }
#if !(defined(WIN32) || defined(TILES)) && !defined(CYGWIN)
        endwin();
#endif
        if( !isDebuggerActive() ) {
            log_crash( "Signal", msg );
        }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
        static_cast<void>( std::signal( SIGABRT, SIG_DFL ) );
#pragma GCC diagnostic pop
        abort(); // NOLINT(cata-assert)
    }
} // extern "C"

[[noreturn]] static void crash_terminate_handler()
{
    // TODO: thread-safety?
    const char *type;
    const char *msg;
    try {
        auto &&ex = std::current_exception(); // *NOPAD*
        if( ex ) {
            std::rethrow_exception( ex );
        } else {
            type = msg = "Unexpected termination";
        }
    } catch( const std::exception &e ) {
        if( !isDebuggerActive() ) {
            type = typeid( e ).name();
            msg = e.what();
            // call here to avoid `msg = e.what()` going out of scope
            log_crash( type, msg );
        }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
        static_cast<void>( std::signal( SIGABRT, SIG_DFL ) );
#pragma GCC diagnostic pop
        abort(); // NOLINT(cata-assert)
    } catch( ... ) {
        type = "Unknown exception";
        msg = "Not derived from std::exception";
    }
    if( !isDebuggerActive() ) {
        log_crash( type, msg );
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
    static_cast<void>( std::signal( SIGABRT, SIG_DFL ) );
#pragma GCC diagnostic pop
    abort(); // NOLINT(cata-assert)
}

void init_crash_handlers()
{
#if defined(__ANDROID__)
    // Clean dummy file crash.log.prompt
    remove( ( PATH_INFO::crash() + ".prompt" ).c_str() );
#endif
    for( int sig : {
             SIGSEGV, SIGILL, SIGABRT, SIGFPE
#if defined(SIGBUS)
             , SIGBUS
#endif
         } ) {

        void ( *previous_handler )( int sig ) = std::signal( sig, signal_handler );
        if( previous_handler == SIG_ERR ) {
            std::cerr << "Failed to set signal handler for signal " << sig << "\n";
        }
    }
    std::set_terminate( crash_terminate_handler );
}

#else // !BACKTRACE

void init_crash_handlers()
{
}

#endif
