#ifdef CATA_CATCH_PCH
#undef TWOBLUECUBES_SINGLE_INCLUDE_CATCH_HPP_INCLUDED
#define CATCH_CONFIG_IMPL_ONLY
#endif
#define CATCH_CONFIG_RUNNER
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(_MSC_VER)
#include <io.h>
#else
#include <unistd.h>
#endif

#include "cata_catch.h"

#include "avatar.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "color.h"
#include "compatibility.h"
#include "coordinates.h"
#include "debug.h"
#include "enums.h"
#include "filesystem.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "json.h"
#include "map.h"
#include "messages.h"
#include "options.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "path_info.h"
#include "point.h"
#include "rng.h"
#include "string_formatter.h"
#include "type_id.h"
#include "weather.h"
#include "worldfactory.h"

static const mod_id MOD_INFORMATION_dda( "dda" );

using name_value_pair_t = std::pair<std::string, std::string>;
using option_overrides_t = std::vector<name_value_pair_t>;

static std::vector<mod_id> mods;
static std::string user_dir;
static bool dont_save{ false };
static option_overrides_t option_overrides_for_test_suite;
static std::string error_fmt = "human-readable";

static std::chrono::system_clock::time_point start_time;
static std::chrono::system_clock::time_point end_time;
static bool error_during_initialization{ false };
static bool fail_to_init_game_state{ false };

static bool needs_game{ false };

static std::vector<mod_id> extract_mod_selection( const std::string_view mod_string )
{
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

    g->load_core_data();
    g->load_world_modfiles();

    get_avatar() = avatar();
    get_avatar().create( character_type::NOW );
    get_avatar().setID( g->assign_npc_id(), false );

    get_map() = map();

    overmap_special_batch empty_specials( point_abs_om{} );
    overmap_buffer.create_custom_overmap( point_abs_om{}, empty_specials );

    map &here = get_map();
    // TODO: fix point types
    here.load( tripoint_abs_sm( here.get_abs_sub() ), false );
    get_avatar().move_to( tripoint_abs_ms::zero );

    get_weather().update_weather();
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

static option_overrides_t extract_option_overrides( const std::string_view option_overrides_string )
{
    option_overrides_t ret;
    const char delim = ',';
    const char sep = ':';
    size_t i = 0;
    size_t pos = option_overrides_string.find( delim );
    while( pos != std::string::npos ) {
        std::string part = static_cast<std::string>( option_overrides_string.substr( i, pos ) );
        ret.emplace_back( split_pair( part, sep ) );
        i = ++pos;
        pos = option_overrides_string.find( delim, pos );
    }
    // Handle last part
    const std::string part = static_cast<std::string>( option_overrides_string.substr( i ) );
    ret.emplace_back( split_pair( part, sep ) );
    return ret;
}

struct CataListener : Catch::TestEventListenerBase {
    using TestEventListenerBase::TestEventListenerBase;

    void testRunStarting( Catch::TestRunInfo const & ) override {
        if( needs_game ) {
            try {
                init_global_game_state( mods, option_overrides_for_test_suite, user_dir );
            } catch( ... ) {
                DebugLog( D_INFO, DC_ALL ) << "Fail to initialize global game state" << std::endl;
                // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
                fail_to_init_game_state = true;
                throw;
            }
            // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
            error_during_initialization = debug_has_error_been_observed();

            DebugLog( D_INFO, DC_ALL ) << "Game data loaded, running Catch2 session:" << std::endl;
        } else {
            DebugLog( D_INFO, DC_ALL ) << "Running Catch2 session:" << std::endl;
        }
        end_time = start_time = std::chrono::system_clock::now();
    }

    void testRunEnded( Catch::TestRunStats const & ) override {
        end_time = std::chrono::system_clock::now();
    }

    void sectionStarting( Catch::SectionInfo const &sectionInfo ) override {
        TestEventListenerBase::sectionStarting( sectionInfo );
        // Initialize the cata RNG with the Catch seed for reproducible tests
        const unsigned int seed = m_config->rngSeed();
        if( seed ) {
            rng_set_engine_seed( seed );
        } else {
            rng_set_engine_seed( rng_get_first_seed() );
        }
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

struct CataCIReporter: Catch::ConsoleReporter {
    explicit CataCIReporter( Catch::ReporterConfig const &config ) : Catch::ConsoleReporter(
            config ) {};

    void testCaseStarting( Catch::TestCaseInfo const &testInfo ) override {
        Catch::ConsoleReporter::testCaseStarting( testInfo );
        std::string tag_string;
        for( const std::string &tag : testInfo.tags ) {
            tag_string += string_format( "[%s]", tag );
        }
        // NOLINTNEXTLINE(cata-text-style)
        DebugLog( D_INFO, DC_ALL ) << "  Testing " << testInfo.name << " " << tag_string << "...";
    }
};

CATCH_REGISTER_REPORTER( "cata-ci-reporter", CataCIReporter )

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

    using namespace Catch::clara;
    std::string option_overrides;
    std::string mods_string;
    std::string check_plural_str;
    int limit_debug_level = -1;
    Parser cli = session.cli()
                 | Opt( mods_string, "mod1,mod2,…" )
                 ["--mods"]
                 ( "[CataclysmDDA] Loads the list of mods before executing tests." )
                 | Opt( user_dir, "dirname" )
                 ["--user-dir"]
                 ( "[CataclysmDDA] Set user dir (where test world will be created)" )
                 | Opt( dont_save )
                 ["--drop-world"] // "-D" conflicts with Catch2 own "--min-duration"
                 ( "[CataclysmDDA] Don't save the world on test failure." )
                 | Opt( option_overrides, "n:v[,…]" )
                 ["--option_overrides"]
                 ( "[CataclysmDDA] Name-value pairs of game options for tests (overrides config/options.json values)." )
                 | Opt( error_fmt, "human-readable|github-action" )
                 ["--error-format"]
                 ( "[CataclysmDDA] Format of error messages (default: human-readable)" )
                 | Opt( check_plural_str, "none|certain|possbile" )
                 ["--check-plural"]
                 ( "[CataclysmDDA] (TBW)" )
                 | Opt( limit_debug_level, "number" )
                 ["--set-debug-level-mask"]
                 ( "[CataclysmDDA] Set debug level bitmask - see `enum DebugLevel` in src/debug.h for individual bits definition" )
                 ;
    session.cli( cli );

    // Note: this must not be invoked before all DDA-specific flags are stripped from arg_vec!
    int result = session.applyCommandLine( arg_vec.size(), &arg_vec[0] );
    if( result != 0 || session.configData().showHelp ) {
        return result;
    }

    // Validate CDDA arguments
    mods = extract_mod_selection( mods_string );
    if( std::find( mods.begin(), mods.end(), MOD_INFORMATION_dda ) == mods.end() ) {
        mods.insert( mods.begin(), MOD_INFORMATION_dda ); // @todo move unit test items to core
    }

    if( user_dir.empty() ) {
        user_dir = "./test_user_dir/";
    }
    if( !string_ends_with( user_dir, "/" ) ) {
        user_dir += "/";
    }

    option_overrides_for_test_suite = extract_option_overrides( option_overrides );

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

    // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
    test_mode = true;

    on_out_of_scope print_newline( []() {
        printf( "\n" );
    } );

    setupDebug( DebugOutput::std_err );
    if( limit_debug_level != -1 ) {
        limitDebugLevel( limit_debug_level );
    }

    // Set the seed for mapgen (the seed will also be reset before each test)
    const unsigned int seed = session.config().rngSeed();
    if( seed ) {
        rng_set_engine_seed( seed );

        // If the run is terminated due to a crash during initialization, we won't
        // see the seed unless it's printed out in advance, so do that here.
        DebugLog( D_INFO, DC_ALL ) << "Randomness seeded to: " << seed << std::endl;
    } else {
        DebugLog( D_INFO, DC_ALL ) << "Default randomness seeded to: " << rng_get_first_seed() << std::endl;
    }

    // Tests not requiring the global game initialized are tagged with [nogame]
    {
        using namespace Catch;
        Config const &config = session.config();
        std::vector<TestCase> const &tcs = filterTests(
                                               getAllTestCasesSorted( config ),
                                               config.testSpec(),
                                               config
                                           );
        for( TestCase const &tc : tcs ) {
            // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
            needs_game = true;
            for( std::string const &tag : tc.getTestCaseInfo().tags ) {
                if( tag == "nogame" ) {
                    // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
                    needs_game = false;
                    break;
                }
            }
            if( needs_game ) {
                break;
            }
        }
    }

    try {
        result = session.run();
    } catch( const std::exception &err ) {
        DebugLog( D_ERROR, DC_ALL ) << "Terminated:\n" << err.what();
        DebugLog( D_INFO, DC_ALL ) <<
                                   "Make sure that you're in the correct working directory and your data isn't corrupted.";
        return EXIT_FAILURE;
    }

    if( world_generator ) {
        std::string world_name = world_generator->active_world->world_name;
        if( result == 0 || dont_save || fail_to_init_game_state ) {
            world_generator->delete_world( world_name, true );
        } else {
            if( g->save() ) {
                DebugLog( D_INFO, DC_ALL ) << "Test world " << world_name << " left for inspection.";
            } else {
                DebugLog( D_ERROR, DC_ALL ) << "Test world " << world_name << " failed to save.";
                result = 1;
            }
        }
    }

    std::chrono::duration<double> elapsed_seconds = end_time - start_time;
    DebugLog( D_INFO, DC_ALL ) << "Finished in " << elapsed_seconds.count() << " seconds";

    if( seed ) {
        // Also print the seed at the end so it can be easily found
        DebugLog( D_INFO, DC_ALL ) << "Randomness seeded to: " << seed << std::endl;
    } else {
        DebugLog( D_INFO, DC_ALL ) << "Default randomness seeded to: " << rng_get_first_seed() << std::endl;
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
