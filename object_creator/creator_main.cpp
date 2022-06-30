#include <vector>

#include "crash.h"
#include "creator_main_window.h"
#include "cursesdef.h"
#include "debug.h"
#include "game.h"
#include "game_ui.h"
#include "input.h"
#include "loading_ui.h"
#include "mapsharing.h"
#include "options.h"
#include "output.h"
#include "path_info.h"
#include "platform_win.h"
#include "rng.h"
#include "string_id.h"
#include "ui.h"
#include "worldfactory.h"

#include <QtWidgets/qapplication.h>
#include <QtCore/QSettings>
#ifdef QT_STATICPLUGIN
#include <QtCore/QtPlugin>
#ifdef _WIN32
Q_IMPORT_PLUGIN( QWindowsIntegrationPlugin );
#endif
#endif

struct MOD_INFORMATION;
using mod_id = string_id<MOD_INFORMATION>;

namespace
{

void exit_handler( int s )
{
    const int old_timeout = inp_mngr.get_timeout();
    inp_mngr.reset_timeout();
    if( s != 2 || query_yn( _( "Really Quit?  All unsaved changes will be lost." ) ) ) {
        deinitDebug();

        int exit_status = 0;
        g.reset();

        catacurses::endwin();

        exit( exit_status );
    }
    inp_mngr.set_timeout( old_timeout );
}
}

struct cli_opts {
    int seed = time( nullptr );
    bool verifyexit = false;
    bool check_mods = false;
    std::string dump;
    dump_mode dmode = dump_mode::TSV;
    std::vector<std::string> opts;
    std::string world; /** if set try to load first save in this world on startup */
};

#if defined(USE_WINMAIN)
int APIENTRY WinMain( HINSTANCE /* hInstance */, HINSTANCE /* hPrevInstance */,
                      LPSTR /* lpCmdLine */, int /* nCmdShow */ )
{
    int argc = __argc;
    char **argv = __argv;
#elif defined(__ANDROID__)
extern "C" int SDL_main( int argc, char **argv ) {
#else
int main( int argc, char *argv[] )
{
#endif
    init_crash_handlers();

#if defined(__ANDROID__)
    // Start the standard output logging redirector
    start_logger( "cdda" );

    // On Android first launch, we copy all data files from the APK into the app's writeable folder so std::io stuff works.
    // Use the external storage so it's publicly modifiable data (so users can mess with installed data, save games etc.)
    std::string external_storage_path( SDL_AndroidGetExternalStoragePath() );
    if( external_storage_path.back() != '/' ) {
        external_storage_path += '/';
    }

    PATH_INFO::init_base_path( external_storage_path );
#else
    // Set default file paths
#if defined(PREFIX)
#define Q(STR) #STR
#define QUOTE(STR) Q(STR)
    PATH_INFO::init_base_path( std::string( QUOTE( PREFIX ) ) );
#else
    PATH_INFO::init_base_path( "" );
#endif
#endif

#if defined(__ANDROID__)
    PATH_INFO::init_user_dir( external_storage_path );
#else
#   if defined(USE_HOME_DIR) || defined(USE_XDG_DIR)
    PATH_INFO::init_user_dir( "" );
#   else
    PATH_INFO::init_user_dir( "./" );
#   endif
#endif
    PATH_INFO::set_standard_filenames();

    MAP_SHARING::setDefaults();
    
    QSettings settings( QSettings::IniFormat, QSettings::UserScope,
        "CleverRaven", "Cataclysm - DDA" );
        

    cli_opts cli;

    rng_set_engine_seed( cli.seed );

    g = std::make_unique<game>();
    // First load and initialize everything that does not
    // depend on the mods.
    try {
        g->load_static_data();
        if( cli.verifyexit ) {
            exit_handler( 0 );
        }
        if( !cli.dump.empty() ) {
            init_colors();
            exit( g->dump_stats( cli.dump, cli.dmode, cli.opts ) ? 0 : 1 );
        }
    } catch( const std::exception &err ) {
        debugmsg( "%s", err.what() );
        exit_handler( -999 );
    }

    loading_ui ui( false );

    get_options().init();
    get_options().load();

    init_colors();

    world_generator = std::make_unique<worldfactory>();
    world_generator->init();
    std::vector<mod_id> mods;
    mods.push_back( mod_id( "dda" ) );
    if( settings.contains( "mods/include" ) ) {
        QStringList modlist = settings.value( "mods/include" ).value<QStringList>();
        for( const QString& i : modlist )
        {
            mods.push_back( mod_id( i.toStdString() ) );
        }
    }
    world_generator->active_world = world_generator->make_new_world( { mods } );

    g->load_core_data( ui );
    g->load_world_modfiles( ui );

    QApplication app( argc, argv );
    creator::main_window().execute( app );
}
