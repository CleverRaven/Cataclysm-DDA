#if defined BACKTRACE && ( defined _WIN32 || defined _WIN64 )

#include <csignal>
#include <cstdalign>
#include <cstdio>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <typeinfo>

#ifdef TILES
#include <SDL.h>
#endif

#include "crash.h"
#include "version.h"
#include "platform_win.h"

#include <dbghelp.h>

// signal handlers are expected to have C linkage, and only use the
// common subset of C & C++
extern "C" {

#define BUF_SIZE 4096
    static char buf[BUF_SIZE];

#define MODULE_PATH_LEN 512
    static char mod_path[MODULE_PATH_LEN];

    // on some systems the number of frames to capture have to be less than 63 according to the documentation
#define BT_CNT 62
#define MAX_NAME_LEN 512
    // ( MAX_NAME_LEN - 1 ) because SYMBOL_INFO already contains a TCHAR
#define SYM_SIZE ( sizeof( SYMBOL_INFO ) + ( MAX_NAME_LEN - 1 ) * sizeof( TCHAR ) )
    static PVOID bt[BT_CNT];
    static struct {
        alignas( SYMBOL_INFO ) char storage[SYM_SIZE];
    } sym_storage;
    static SYMBOL_INFO *const sym = ( SYMBOL_INFO * ) &sym_storage;

    // compose message ourselves to avoid potential dynamical allocation.
    static void append_str( FILE *file, char **beg, char *end, char const *from )
    {
        fputs( from, stderr );
        if( file ) {
            fputs( from, file );
        }
        for( ; *from && *beg + 1 < end; ++from, ++*beg ) {
            **beg = *from;
        }
    }

    static void append_ch( FILE *file, char **beg, char *end, char ch )
    {
        fputc( ch, stderr );
        if( file ) {
            fputc( ch, file );
        }
        if( *beg + 1 < end ) {
            **beg = ch;
            ++*beg;
        }
    }

    static void append_uint( FILE *file, char **beg, char *end, uintmax_t value )
    {
        if( value != 0 ) {
            int cnt = 0;
            for( uintmax_t tmp = value; tmp; tmp >>= 4, ++cnt ) {
            }
            for( ; cnt; --cnt ) {
                char ch = "0123456789ABCDEF"[( value >> ( cnt * 4 - 4 ) ) & 0xF];
                append_ch( file, beg, end, ch );
            }
        } else {
            append_ch( file, beg, end, '0' );
        }
    }

    static void append_ptr( FILE *file, char **beg, char *end, void *p )
    {
        append_uint( file, beg, end, uintptr_t( p ) );
    }

    static void dump_to( char const *file )
    {
        HANDLE handle = CreateFile( file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL, NULL );
        //@todo call from a separate process as suggested by the documentation
        //@todo capture stack trace and pass as parameter as suggested by the documentation
        MiniDumpWriteDump( GetCurrentProcess(),
                           GetCurrentProcessId(),
                           handle,
                           MiniDumpNormal,
                           NULL, NULL, NULL );
        CloseHandle( handle );
    }

    static void log_crash( char const *type, char const *msg )
    {
        dump_to( ".core" );
        char *beg = buf, *end = buf + BUF_SIZE;
        FILE *file = fopen( "config/crash.log", "w" );
        append_str( file, &beg, end, "VERSION: " );
        append_str( file, &beg, end, VERSION );
        append_str( file, &beg, end, "\nTYPE: " );
        append_str( file, &beg, end, type );
        append_str( file, &beg, end, "\nMESSAGE: " );
        append_str( file, &beg, end, msg );
        append_str( file, &beg, end, "\nSTACK TRACE:\n" );
        sym->SizeOfStruct = sizeof( SYMBOL_INFO );
        sym->MaxNameLen = MAX_NAME_LEN;
        USHORT num_bt = CaptureStackBackTrace( 0, BT_CNT, bt, NULL );
        HANDLE proc = GetCurrentProcess();
        for( USHORT i = 0; i < num_bt; ++i ) {
            DWORD64 off;
            append_str( file, &beg, end, "    " );
            if( SymFromAddr( proc, ( DWORD64 ) bt[i], &off, sym ) ) {
                append_str( file, &beg, end, sym->Name );
                append_str( file, &beg, end, "+0x" );
                append_uint( file, &beg, end, off );
            }
            append_str( file, &beg, end, "@0x" );
            append_ptr( file, &beg, end, bt[i] );
            DWORD64 mod_base = SymGetModuleBase64( proc, ( DWORD64 ) bt[i] );
            if( mod_base ) {
                append_ch( file, &beg, end, '[' );
                DWORD mod_len = GetModuleFileName( ( HMODULE ) mod_base, mod_path, MODULE_PATH_LEN );
                // mod_len == MODULE_NAME_LEN means insufficient buffer
                if( mod_len > 0 && mod_len < MODULE_PATH_LEN ) {
                    char const *mod_name = mod_path + mod_len;
                    for( ; mod_name > mod_path && *( mod_name - 1 ) != '\\'; --mod_name ) {
                    }
                    append_str( file, &beg, end, mod_name );
                } else {
                    append_str( file, &beg, end, "0x" );
                    append_uint( file, &beg, end, mod_base );
                }
                append_str( file, &beg, end, "+0x" );
                append_uint( file, &beg, end, ( uintptr_t ) bt[i] - mod_base );
                append_ch( file, &beg, end, ']' );
            }
            append_ch( file, &beg, end, '\n' );
        }
#ifdef TILES
        if( SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Error", buf, NULL ) != 0 ) {
            append_str( file, &beg, end, "Error creating SDL message box: " );
            append_str( file, &beg, end, SDL_GetError() );
            append_ch( file, &beg, end, '\n' );
        }
#endif
        *beg = '\0';
        if( file ) {
            fclose( file );
        }
    }

    static void signal_handler( int sig )
    {
        //@todo thread-safety?
        //@todo make string literals & static variables atomic?
        signal( sig, SIG_DFL );
        // undefined behavior according to the standard
        // but we can get nothing out of it without these
        char const *msg;
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
        // end of UB
        _Exit( EXIT_FAILURE );
    }

} // extern "C"

[[noreturn]] static void terminate_hanlder()
{
    //@todo thread-safety?
    char const *type;
    char const *msg;
    try {
        auto &&ex = std::current_exception();
        if( ex ) {
            std::rethrow_exception( ex );
        } else {
            type = msg = "Unexpected termination";
        }
    } catch( std::exception const &e ) {
        type = typeid( e ).name();
        msg = e.what();
    } catch( ... ) {
        type = "Unknown exception";
        msg = "Not derived from std::exception";
    }
    log_crash( type, msg );
    std::exit( EXIT_FAILURE );
}

void init_crash_handlers()
{
    SymInitialize( GetCurrentProcess(), NULL, TRUE );
    ULONG stacksize = 2048;
    SetThreadStackGuarantee( &stacksize );
    for( auto &&sig : {
             SIGSEGV, SIGILL, SIGABRT, SIGFPE
         } ) {

        std::signal( sig, signal_handler );
    }
    std::set_terminate( terminate_hanlder );
}

#else

void init_crash_handlers()
{
}

#endif
