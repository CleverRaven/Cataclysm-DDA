#include "crash.h"

#if defined(BACKTRACE)

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>

#if defined(TILES)
#   if defined(_MSC_VER) && defined(USE_VCPKG)
#       include <SDL2/SDL.h>
#   else
#       include <SDL.h>
#   endif
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
        log_text << "The program has crashed."
                 << "\nSee the log file for a stack trace."
                 << "\nCRASH LOG FILE: " << crash_log_file
                 << "\nVERSION: " << getVersionString()
                 << "\nTYPE: " << type
                 << "\nMESSAGE: " << msg;
#if defined(TILES)
        if( SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Error",
                                      log_text.str().c_str(), nullptr ) != 0 ) {
            log_text << "Error creating SDL message box: " << SDL_GetError() << '\n';
        }
#endif
        log_text << "\nSTACK TRACE:\n";
        debug_write_backtrace( log_text );
        std::cerr << log_text.str();
        FILE *file = fopen( crash_log_file.c_str(), "w" );
        if( file ) {
            fwrite( log_text.str().data(), 1, log_text.str().size(), file );
            fclose( file );
        }
    }

    static void signal_handler( int sig )
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
        signal( sig, SIG_DFL );
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
            default:
                return;
        }
        log_crash( "Signal", msg );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
        std::signal( SIGABRT, SIG_DFL );
#pragma GCC diagnostic pop
        abort();
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
        type = typeid( e ).name();
        msg = e.what();
        // call here to avoid `msg = e.what()` going out of scope
        log_crash( type, msg );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
        std::signal( SIGABRT, SIG_DFL );
#pragma GCC diagnostic pop
        abort();
    } catch( ... ) {
        type = "Unknown exception";
        msg = "Not derived from std::exception";
    }
    log_crash( type, msg );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
    std::signal( SIGABRT, SIG_DFL );
#pragma GCC diagnostic pop
    abort();
}

void init_crash_handlers()
{
    for( auto sig : {
             SIGSEGV, SIGILL, SIGABRT, SIGFPE
         } ) {

        std::signal( sig, signal_handler );
    }
    std::set_terminate( crash_terminate_handler );
}

#else // !BACKTRACE

void init_crash_handlers()
{
}

#endif
