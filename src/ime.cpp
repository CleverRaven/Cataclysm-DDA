#include "ime.h"

#include "options.h"
#include "platform_win.h"
#include "sdltiles.h"

static bool ime_enabled()
{
#if defined( __ANDROID__ )
    return false; // always call disable_ime() (i.e. do nothing) on return
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
#endif
    // TODO: other platforms?
}

void disable_ime()
{
#if defined( __ANDROID__ )
    // the original android code did nothing, so don't change it
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
