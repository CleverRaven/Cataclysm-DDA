#include "ime.h"

#include "options.h"
#include "platform_win.h"
#include "sdltiles.h"

#ifdef _WIN32
class imm_wrapper
{
    private:
        HMODULE hImm;
        typedef HIMC( WINAPI *pImmGetContext_t )( HWND );
        typedef BOOL( WINAPI *pImmGetOpenStatus_t )( HIMC );
        typedef BOOL( WINAPI *pImmSetOpenStatus_t )( HIMC, BOOL );
        typedef BOOL( WINAPI *pImmReleaseContext_t )( HWND, HIMC );
        pImmGetContext_t pImmGetContext;
        pImmGetOpenStatus_t pImmGetOpenStatus;
        pImmSetOpenStatus_t pImmSetOpenStatus;
        pImmReleaseContext_t pImmReleaseContext;

        template<typename T>
        static T fun_ptr_cast( FARPROC p ) {
            // workaround function cast warning
            return reinterpret_cast<T>( reinterpret_cast<void *>( p ) );
        }
    public:
        imm_wrapper() {
            // Check if East Asian support is available
            hImm = LoadLibrary( "imm32.dll" );
            if( hImm ) {
                pImmGetContext = fun_ptr_cast<pImmGetContext_t>(
                                     GetProcAddress( hImm, "ImmGetContext" ) );
                pImmGetOpenStatus = fun_ptr_cast<pImmGetOpenStatus_t>(
                                        GetProcAddress( hImm, "ImmGetOpenStatus" ) );
                pImmSetOpenStatus = fun_ptr_cast<pImmSetOpenStatus_t>(
                                        GetProcAddress( hImm, "ImmSetOpenStatus" ) );
                pImmReleaseContext = fun_ptr_cast<pImmReleaseContext_t>(
                                         GetProcAddress( hImm, "ImmReleaseContext" ) );
                if( !pImmGetContext || !pImmGetOpenStatus ||
                    !pImmSetOpenStatus || !pImmReleaseContext ) {

                    FreeLibrary( hImm );
                    hImm = nullptr;
                    pImmGetContext = nullptr;
                    pImmGetOpenStatus = nullptr;
                    pImmSetOpenStatus = nullptr;
                    pImmReleaseContext = nullptr;
                }
            }
        }

        ~imm_wrapper() {
            if( hImm ) {
                FreeLibrary( hImm );
            }
        }

        bool ime_enabled() {
            if( hImm ) {
                const HWND hwnd = getWindowHandle();
                const HIMC himc = pImmGetContext( hwnd );
                bool enabled = pImmGetOpenStatus( himc );
                pImmReleaseContext( hwnd, himc );
                return enabled;
            }
            return false;
        }

        void enable_ime() {
            if( hImm ) {
                const HWND hwnd = getWindowHandle();
                const HIMC himc = pImmGetContext( hwnd );
                pImmSetOpenStatus( himc, TRUE );
                pImmReleaseContext( hwnd, himc );
            }
        }

        void disable_ime() {
            if( hImm ) {
                const HWND hwnd = getWindowHandle();
                const HIMC himc = pImmGetContext( hwnd );
                pImmSetOpenStatus( himc, FALSE );
                pImmReleaseContext( hwnd, himc );
            }
        }
};

static imm_wrapper imm;
#endif

static bool ime_enabled()
{
#if defined( __ANDROID__ )
    return false; // always call disable_ime() (i.e. do nothing) on return
#elif defined( _WIN32 )
    return imm.ime_enabled();
#endif
    return false;
    // TODO: other platforms?
}

void enable_ime()
{
#if defined( __ANDROID__ )
    if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
        SDL_StartTextInput();
    }
#elif defined( _WIN32 )
    imm.enable_ime();
#endif
    // TODO: other platforms?
}

void disable_ime()
{
#if defined( __ANDROID__ )
    // the original android code did nothing, so don't change it
#elif defined( _WIN32 )
    imm.disable_ime();
#endif
    // TODO: other platforms?
}

ime_sentry::ime_sentry( ime_sentry::mode m ) : previously_enabled( ime_enabled() )
{
    switch( m ) {
        case enable:
            enable_ime();
            break;
        case disable:
            disable_ime();
            break;
        case keep:
            // do nothing
            break;
    }
}

ime_sentry::~ime_sentry()
{
    if( previously_enabled ) {
        enable_ime();
    } else {
        disable_ime();
    }
}
