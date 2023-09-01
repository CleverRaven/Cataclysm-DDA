#ifdef CATA_CATCH_PCH
#undef TWOBLUECUBES_SINGLE_INCLUDE_CATCH_HPP_INCLUDED
#define CATCH_CONFIG_IMPL_ONLY
#endif
#define CATCH_CONFIG_RUNNER
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#if defined(_MSC_VER)
#include <io.h>
#else
#include <unistd.h>
#endif

#include "avatar.h"
#include "cached_options.h"
#include "cata_assert.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "color.h"
#include "compatibility.h"
#include "debug.h"
#include "filesystem.h"
#include "game.h"
#include "help.h"
#include "json.h"
#include "loading_ui.h"
#include "map.h"
#include "messages.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "path_info.h"
#include "rng.h"
#include "type_id.h"
#include "weather.h"
#include "worldfactory.h"

static const mod_id MOD_INFORMATION_dda( "dda" );

using name_value_pair_t = std::pair<std::string, std::string>;
using option_overrides_t = std::vector<name_value_pair_t>;

// If tag is found as a prefix of any argument in arg_vec, the argument is
// removed from arg_vec and the argument suffix after tag is returned.
// Otherwise, an empty string is returned and arg_vec is unchanged.
static std::string extract_argument( std::vector<const char *> &arg_vec, const std::string &tag )
{
    std::string arg_rest;
    for( auto iter = arg_vec.begin(); iter != arg_vec.end(); iter++ ) {
        if( strncmp( *iter, tag.c_str(), tag.length() ) == 0 ) {
            arg_rest = std::string( &( *iter )[tag.length()] );
            arg_vec.erase( iter );
            break;
        }
    }
    return arg_rest;
}

static std::vector<mod_id> extract_mod_selection( std::vector<const char *> &arg_vec )
{
    std::string mod_string = extract_argument( arg_vec, "--mods=" );

    std::vector<std::string> mod_names = string_split( mod_string, ',' );
    std::vector<mod_id> ret;
    for( const std::string &mod_name : mod_names ) {
        if( !mod_name.empty() ) {
            ret.emplace_back( mod_name );
        }
    }
    // Always load test data mod
    ret.emplace_back( "test_data" );

    return ret;
}

static void init_global_game_state( const std::vector<mod_id> &mods,
                                    option_overrides_t &option_overrides,
                                    const std::string &user_dir )
{
    if( !assure_dir_exist( user_dir ) ) {
        // NOLINTNEXTLINE(misc-static-assert,cert-dcl03-c)
        cata_fatal( "Unable to make user_dir directory '%s'.  Check permissions.", user_dir );
    }

    PATH_INFO::init_base_path( "" );
    PATH_INFO::init_user_dir( user_dir );
    PATH_INFO::set_standard_filenames();

    if( !assure_dir_exist( PATH_INFO::config_dir() ) ) {
        // NOLINTNEXTLINE(misc-static-assert,cert-dcl03-c)
        cata_fatal( "Unable to make config directory.  Check permissions." );
    }

    if( !assure_dir_exist( PATH_INFO::savedir() ) ) {
        // NOLINTNEXTLINE(misc-static-assert,cert-dcl03-c)
        cata_fatal( "Unable to make save directory.  Check permissions." );
    }

    if( !assure_dir_exist( PATH_INFO::templatedir() ) ) {
        // NOLINTNEXTLINE(misc-static-assert,cert-dcl03-c)
        cata_fatal( "Unable to make templates directory.  Check permissions." );
    }

    get_options().init();
    get_options().load();

    // Apply command-line option overrides for test suite execution.
    if( !option_overrides.empty() ) {
        for( const name_value_pair_t &option : option_overrides ) {
            if( get_options().has_option( option.first ) ) {
                options_manager::cOpt &opt = get_options().get_option( option.first );
                opt.setValue( option.second );
            }
        }
    }
    init_colors();

    g = std::make_unique<game>( );
    g->new_game = true;
    g->load_static_data();

    get_help().load();

    world_generator->set_active_world( nullptr );
    world_generator->init();
    // Using unicode characters in the world name to test path encoding
#ifndef _WIN32
    const std::string test_world_name = "Test World 测试世界 " + std::to_string( getpid() );
#else
    const std::string test_world_name = "Test World 测试世界";
#endif
    WORLD *test_world = world_generator->make_new_world( test_world_name, mods );
    cata_assert( test_world != nullptr );
    world_generator->set_active_world( test_world );
    cata_assert( world_generator->active_world != nullptr );

    calendar::set_eternal_season( get_option<bool>( "ETERNAL_SEASON" ) );
    calendar::set_season_length( get_option<int>( "SEASON_LENGTH" ) );

    loading_ui ui( false );
    g->load_core_data( ui );
    g->load_world_modfiles( ui );

    get_avatar() = avatar();
    get_avatar().create( character_type::NOW );
    get_avatar().setID( g->assign_npc_id(), false );

    get_map() = map();

    overmap_special_batch empty_specials( point_abs_om{} );
    overmap_buffer.create_custom_overmap( point_abs_om{}, empty_specials );

    map &here = get_map();
    // TODO: fix point types
    here.load( tripoint_abs_sm( here.get_abs_sub() ), false );
    get_avatar().move_to( tripoint_abs_ms( tripoint_zero ) );

    get_weather().update_weather();
}

// Checks if any of the flags are in container, removes them all
static bool check_remove_flags( std::vector<const char *> &cont,
                                const std::vector<const char *> &flags )
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

// Split s on separator sep, returning parts as a pair. Returns empty string as
// second value if no separator found.
static name_value_pair_t split_pair( const std::string &s, const char sep )
{
    const size_t pos = s.find( sep );
    if( pos != std::string::npos ) {
        return name_value_pair_t( s.substr( 0, pos ), s.substr( pos + 1 ) );
    } else {
        return name_value_pair_t( s, "" );
    }
}

static option_overrides_t extract_option_overrides( std::vector<const char *> &arg_vec )
{
    option_overrides_t ret;
    std::string option_overrides_string = extract_argument( arg_vec, "--option_overrides=" );
    if( option_overrides_string.empty() ) {
        return ret;
    }
    const char delim = ',';
    const char sep = ':';
    size_t i = 0;
    size_t pos = option_overrides_string.find( delim );
    while( pos != std::string::npos ) {
        std::string part = option_overrides_string.substr( i, pos );
        ret.emplace_back( split_pair( part, sep ) );
        i = ++pos;
        pos = option_overrides_string.find( delim, pos );
    }
    // Handle last part
    const std::string part = option_overrides_string.substr( i );
    ret.emplace_back( split_pair( part, sep ) );
    return ret;
}

static std::string extract_user_dir( std::vector<const char *> &arg_vec )
{
    std::string option_user_dir = extract_argument( arg_vec, "--user-dir=" );
    if( option_user_dir.empty() ) {
        return "./test_user_dir/";
    }
    if( !string_ends_with( option_user_dir, "/" ) ) {
        option_user_dir += "/";
    }
    return option_user_dir;
}

struct CataListener : Catch::TestEventListenerBase {
    using TestEventListenerBase::TestEventListenerBase;

    void sectionStarting( Catch::SectionInfo const &sectionInfo ) override {
        TestEventListenerBase::sectionStarting( sectionInfo );
        // Initialize the cata RNG with the Catch seed for reproducible tests
        rng_set_engine_seed( m_config->rngSeed() );
        // Clear the message log so on test failures we see only messages from
        // during that test
        Messages::clear_messages();
    }

    void sectionEnded( Catch::SectionStats const &sectionStats ) override {
        TestEventListenerBase::sectionEnded( sectionStats );
        if( !sectionStats.assertions.allPassed() ||
            m_config->includeSuccessfulResults() ) {
            std::vector<std::pair<std::string, std::string>> messages =
                        Messages::recent_messages( 0 );
            if( !messages.empty() ) {
                if( !sectionStats.assertions.allPassed() ) {
                    stream << "Log messages during failed test:\n";
                } else {
                    stream << "Log messages during successful test:\n";
                }
            }
            for( const std::pair<std::string, std::string> &message : messages ) {
                stream << message.first << ": " << message.second << '\n';
            }
            Messages::clear_messages();
        }
    }

    bool assertionEnded( Catch::AssertionStats const &assertionStats ) override {
#ifdef BACKTRACE
        Catch::AssertionResult const &result = assertionStats.assertionResult;

        if( result.getResultType() == Catch::ResultWas::FatalErrorCondition ) {
            // We are in a signal handler for a fatal error condition, so print a
            // backtrace
            stream << "Stack trace at fatal error:\n";
            debug_write_backtrace( stream );
        }
#endif

        return TestEventListenerBase::assertionEnded( assertionStats );
    }
};

CATCH_REGISTER_LISTENER( CataListener )

int main( int argc, const char *argv[] )
{
#if defined(_MSC_VER)
    bool supports_color = _isatty( _fileno( stdout ) );
#else
    bool supports_color = isatty( STDOUT_FILENO );
#endif
    // formatter stdout in github actions is redirected but still able to handle ANSI colors
    supports_color |= std::getenv( "CI" ) != nullptr;

    // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
    json_error_output_colors = supports_color
                               ? json_error_output_colors_t::ansi_escapes
                               : json_error_output_colors_t::no_colors;

    reset_floating_point_mode();
    on_out_of_scope json_member_reporting_guard{ [] {
            // Disable reporting unvisited members if stack unwinding leaves main early.
            Json::globally_report_unvisited_members( false );
        } };
    Catch::Session session;

    std::vector<const char *> arg_vec( argv, argv + argc );

    std::vector<mod_id> mods = extract_mod_selection( arg_vec );
    if( std::find( mods.begin(), mods.end(), MOD_INFORMATION_dda ) == mods.end() ) {
        mods.insert( mods.begin(), MOD_INFORMATION_dda ); // @todo move unit test items to core
    }

    option_overrides_t option_overrides_for_test_suite = extract_option_overrides( arg_vec );

    const bool dont_save = check_remove_flags( arg_vec, { "-D", "--drop-world" } );

    std::string user_dir = extract_user_dir( arg_vec );

    std::string error_fmt = extract_argument( arg_vec, "--error-format=" );
    if( error_fmt == "github-action" ) {
        // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
        error_log_format = error_log_format_t::github_action;
    } else if( error_fmt == "human-readable" || error_fmt.empty() ) {
        // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
        error_log_format = error_log_format_t::human_readable;
    } else {
        printf( "Unknown format %s", error_fmt.c_str() );
        return EXIT_FAILURE;
    }

    std::string check_plural_str = extract_argument( arg_vec, "--check-plural=" );
    if( check_plural_str == "none" ) {
        // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
        check_plural = check_plural_t::none;
    } else if( check_plural_str == "certain" || check_plural_str.empty() ) {
        // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
        check_plural = check_plural_t::certain;
    } else if( check_plural_str == "possible" ) {
        // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
        check_plural = check_plural_t::possible;
    } else {
        printf( "Unknown check_plural value %s", check_plural_str.c_str() );
        return EXIT_FAILURE;
    }

    // Note: this must not be invoked before all DDA-specific flags are stripped from arg_vec!
    int result = session.applyCommandLine( arg_vec.size(), &arg_vec[0] );
    if( result != 0 || session.configData().showHelp ) {
        printf( "CataclysmDDA specific options:\n" );
        printf( "  --mods=<mod1,mod2,…>         Loads the list of mods before executing tests.\n" );
        printf( "  --user-dir=<dir>             Set user dir (where test world will be created).\n" );
        printf( "  -D, --drop-world             Don't save the world on test failure.\n" );
        printf( "  --option_overrides=n:v[,…]   Name-value pairs of game options for tests.\n" );
        printf( "                               (overrides config/options.json values)\n" );
        printf( "  --error-format=<value>       Format of error messages.  Possible values are:\n" );
        printf( "                                   human-readable (default)\n" );
        printf( "                                   github-action\n" );
        return result;
    }

    // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
    test_mode = true;

    on_out_of_scope print_newline( []() {
        printf( "\n" );
    } );

    setupDebug( DebugOutput::std_err );

    // Set the seed for mapgen (the seed will also be reset before each test)
    const unsigned int seed = session.config().rngSeed();
    if( seed ) {
        rng_set_engine_seed( seed );

        // If the run is terminated due to a crash during initialization, we won't
        // see the seed unless it's printed out in advance, so do that here.
        DebugLog( D_INFO, DC_ALL ) << "Randomness seeded to: " << seed;
    }

    try {
        // TODO: Only init game if we're running tests that need it.
        init_global_game_state( mods, option_overrides_for_test_suite, user_dir );
    } catch( const std::exception &err ) {
        DebugLog( D_ERROR, DC_ALL ) << "Terminated:\n" << err.what();
        DebugLog( D_INFO, DC_ALL ) <<
                                   "Make sure that you're in the correct working directory and your data isn't corrupted.";
        return EXIT_FAILURE;
    }

    bool error_during_initialization = debug_has_error_been_observed();

    DebugLog( D_INFO, DC_ALL ) << "Game data loaded, running Catch2 session:" << std::endl;
    const std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
    result = session.run();
    const std::chrono::system_clock::time_point end = std::chrono::system_clock::now();

    std::string world_name = world_generator->active_world->world_name;
    if( result == 0 || dont_save ) {
        world_generator->delete_world( world_name, true );
    } else {
        if( g->save() ) {
            DebugLog( D_INFO, DC_ALL ) << "Test world " << world_name << " left for inspection.";
        } else {
            DebugLog( D_ERROR, DC_ALL ) << "Test world " << world_name << " failed to save.";
            result = 1;
        }
    }

    std::chrono::duration<double> elapsed_seconds = end - start;
    DebugLog( D_INFO, DC_ALL ) << "Finished in " << elapsed_seconds.count() << " seconds";

    if( seed ) {
        // Also print the seed at the end so it can be easily found
        DebugLog( D_INFO, DC_ALL ) << "Randomness seeded to: " << seed << std::endl;
    }

    if( error_during_initialization ) {
        DebugLog( D_INFO, DC_ALL ) <<
                                   "Treating result as failure due to error logged during initialization.";
        return 1;
    }

    if( debug_has_error_been_observed() ) {
        DebugLog( D_INFO, DC_ALL ) << "Treating result as failure due to error logged during tests.";
        return 1;
    }

    return result;
}
