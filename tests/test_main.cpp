#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

#include "game.h"
#include "filesystem.h"
#include "init.h"
#include "map.h"
#include "morale.h"
#include "path_info.h"
#include "player.h"
#include "worldfactory.h"
#include "debug.h"
#include "mod_manager.h"

#include <algorithm>
#include <cstring>
#include <chrono>

std::vector<std::string> extract_mod_selection( std::vector<const char *> &arg_vec )
{
    std::vector<std::string> ret;
    static const char *mod_tag = "--mods=";
    std::string mod_string;
    for( auto iter = arg_vec.begin(); iter != arg_vec.end(); iter++ ) {
        if( strncmp( *iter, mod_tag, strlen( mod_tag ) ) == 0 ) {
            mod_string = std::string( &(*iter)[ strlen( mod_tag ) ] );
            arg_vec.erase( iter );
            break;
        }
    }

    const char delim = ',';
    size_t i = 0;
    size_t pos = mod_string.find( delim );
    if( pos == std::string::npos && !mod_string.empty() ) {
        ret.push_back( mod_string );
    }

    while( pos != std::string::npos ) {
        ret.push_back( mod_string.substr( i, pos - i ) );
        i = ++pos;
        pos = mod_string.find( delim, pos );

        if( pos == std::string::npos ) {
            ret.push_back( mod_string.substr( i, mod_string.length() ) );
        }
    }

    return ret;
}

void init_global_game_state( const std::vector<std::string> &mods )
{
    PATH_INFO::init_base_path("");
    PATH_INFO::init_user_dir("./");
    PATH_INFO::set_standard_filenames();

    if( !assure_dir_exist( FILENAMES["config_dir"] ) ) {
        assert( !"Unable to make config directory. Check permissions." );
    }

    if( !assure_dir_exist( FILENAMES["savedir"] ) ) {
        assert( !"Unable to make save directory. Check permissions." );
    }

    if( !assure_dir_exist( FILENAMES["templatedir"] ) ) {
        assert( !"Unable to make templates directory. Check permissions." );
    }

    get_options().init();
    get_options().load();
    init_colors();

    g = new game;

    g->load_static_data();

    world_generator->set_active_world(NULL);
    world_generator->get_all_worlds();
    WORLDPTR test_world = world_generator->make_new_world( mods );
    assert( test_world != NULL );
    world_generator->set_active_world(test_world);
    assert( world_generator->active_world != NULL );

    g->load_core_data();
    g->load_world_modfiles( world_generator->active_world );

    g->u = player();
    g->u.create(PLTYPE_NOW);

    g->m = map( get_option<bool>( "ZLEVELS" ) );

    g->m.load( g->get_levx(), g->get_levy(), g->get_levz(), false );
}

// Checks if any of the flags are in container, removes them all
bool check_remove_flags( std::vector<const char *> &cont, const std::vector<const char *> &flags )
{
    bool has_any = false;
    auto iter = flags.begin();
    while( iter != flags.end() ) {
        auto found = std::find_if( cont.begin(), cont.end(),
        [iter]( const char *c ) {
            return strcmp( c, *iter ) == 0;
        } );
        if( found == cont.end() ) {
            iter++;
        } else {
            cont.erase( found );
            has_any = true;
        }
    }

    return has_any;
}

int main( int argc, const char *argv[] )
{
    Catch::Session session;

    std::vector<const char *> arg_vec( argv, argv + argc );

    std::vector<std::string> mods = extract_mod_selection( arg_vec );
    if( std::find( mods.begin(), mods.end(), "dda" ) == mods.end() ) {
        mods.insert( mods.begin(), "dda" ); // @todo move unit test items to core
    }

    bool dont_save = check_remove_flags( arg_vec, { "-D", "--drop-world" } );

    // Note: this must not be invoked before all DDA-specific flags are stripped from arg_vec!
    int result = session.applyCommandLine( arg_vec.size(), &arg_vec[0] );
    if( result != 0 || session.configData().showHelp ) {
        printf( "CataclysmDDA specific options:\n" );
        printf( "  --mods=<mod1,mod2,...>       Loads the list of mods before executing tests.\n" );
        printf( "  -D, --drop-world             Don't save the world on test failure.\n" );
        return result;
    }

    test_mode = true;

    try {
        // TODO: Only init game if we're running tests that need it.
        init_global_game_state( mods );
    } catch( const std::exception &err ) {
        fprintf( stderr, "Terminated: %s\n", err.what() );
        fprintf( stderr, "Make sure that you're in the correct working directory and your data isn't corrupted.\n" );
        return EXIT_FAILURE;
    }

    const auto start = std::chrono::system_clock::now();
    std::time_t start_time = std::chrono::system_clock::to_time_t( start );
    printf( "Starting the actual test at %s", std::ctime( &start_time ) );
    result = session.run();
    const auto end = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t( end );

    auto world_name = world_generator->active_world->world_name;
    if( result == 0 || dont_save ) {
        g->delete_world(world_name, true);
    } else {
        printf("Test world \"%s\" left for inspection.\n", world_name.c_str());
    }

    std::chrono::duration<double> elapsed_seconds = end - start;
    printf( "Ended test at %sThe test took %.3f seconds\n", std::ctime( &end_time ), elapsed_seconds.count() );

    return result;
}
