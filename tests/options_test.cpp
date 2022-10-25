#include "cata_catch.h"
#include "options.h"

static const option_slider_id option_slider_test_world_difficulty( "test_world_difficulty" );

TEST_CASE( "option_slider_test", "[option]" )
{
    options_manager::options_container opts = get_options().get_world_defaults();
    options_manager::options_container old_opts = opts;

    const option_slider &slider = option_slider_test_world_difficulty.obj();
    REQUIRE( slider.count() == 7 );

    // check reordering
    for( int i = 0; i < 7; i++ ) {
        REQUIRE( slider.level_name( i ).translated() == string_format( "TEST %d", i ) );
    }

    const std::map<std::string, std::string> expected_3 {
        { "MONSTER_SPEED", "100%" },
        { "MONSTER_RESILIENCE", "100%" },
        { "SPAWN_DENSITY", "1.00" },
        { "MONSTER_UPGRADE_FACTOR", "4.00" },
        { "ITEM_SPAWNRATE", "1.00" },
        { "ETERNAL_SEASON", "false" },
        { "ETERNAL_TIME_OF_DAY", "normal" }
    };

    const std::map<std::string, std::string> expected_6 {
        { "MONSTER_SPEED", "120%" },
        { "MONSTER_RESILIENCE", "200%" },
        { "SPAWN_DENSITY", "3.00" },
        { "MONSTER_UPGRADE_FACTOR", "3.00" },
        { "ITEM_SPAWNRATE", "0.50" },
        { "ETERNAL_SEASON", "true" },
        { "ETERNAL_TIME_OF_DAY", "night" }
    };

    // check defaults
    int checked = 0;
    for( const auto &opt : opts ) {
        CHECK( opt.second == old_opts.at( opt.first ) );
        const auto iter = expected_3.find( opt.first );
        if( iter != expected_3.end() ) {
            CHECK( opt.second.getValue() == iter->second );
            checked++;
        }
    }
    CHECK( checked == 7 );

    // check modified
    slider.apply_opts( 6, opts );
    checked = 0;
    for( const auto &opt : opts ) {
        const auto iter = expected_6.find( opt.first );
        if( iter != expected_6.end() ) {
            CHECK( opt.second.getValue() == iter->second );
            checked++;
        }
    }
    CHECK( checked == 7 );

    // check defaults
    slider.apply_opts( 3, opts );
    checked = 0;
    for( const auto &opt : opts ) {
        CHECK( opt.second == old_opts.at( opt.first ) );
        const auto iter = expected_3.find( opt.first );
        if( iter != expected_3.end() ) {
            CHECK( opt.second.getValue() == iter->second );
            checked++;
        }
    }
    CHECK( checked == 7 );
}
