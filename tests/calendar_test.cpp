#include <array>
#include <stddef.h>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "string_formatter.h"

TEST_CASE( "time_duration_to_string", "[calendar][nogame]" )
{
    calendar::set_season_length( 91 );
    CHECK( to_string( 10_seconds ) == "10 seconds" );
    CHECK( to_string( 60_seconds ) == "1 minute" );
    CHECK( to_string( 70_seconds ) == "1 minute and 10 seconds" );
    CHECK( to_string( 60_minutes ) == "1 hour" );
    CHECK( to_string( 70_minutes ) == "1 hour and 10 minutes" );
    CHECK( to_string( 24_hours ) == "1 day" );
    CHECK( to_string( 24_hours + 1_seconds ) == "1 day and 1 second" );
    CHECK( to_string( 25_hours ) == "1 day and 1 hour" );
    CHECK( to_string( 25_hours + 1_seconds ) == "1 day and 1 hour" );
    CHECK( to_string( 7_days ) == "1 week" );
    CHECK( to_string( 8_days ) == "1 week and 1 day" );
    CHECK( to_string( 91_days ) == "1 season" );
    CHECK( to_string( 92_days ) == "1 season and 1 day" );
    CHECK( to_string( 99_days ) == "1 season and 1 week" );
    CHECK( to_string( 364_days ) == "1 year" );
    CHECK( to_string( 365_days ) == "1 year and 1 day" );
    CHECK( to_string( 465_days ) == "1 year and 1 season" );
    CHECK( to_string( 3650_days ) == "10 years and 1 week" );
}

TEST_CASE( "time_duration_to_string_eternal_season", "[calendar][nogame]" )
{
    calendar::set_season_length( 91 );
    calendar::set_eternal_season( true );
    CHECK( to_string( 7_days ) == "1 week" );
    CHECK( to_string( 8_days ) == "1 week and 1 day" );
    CHECK( to_string( 91_days ) == "13 weeks" );
    CHECK( to_string( 92_days ) == "13 weeks and 1 day" );
    CHECK( to_string( 99_days ) == "14 weeks and 1 day" );
    CHECK( to_string( 364_days ) == "52 weeks" );
    CHECK( to_string( 365_days ) == "52 weeks and 1 day" );
    CHECK( to_string( 465_days ) == "66 weeks and 3 days" );
    CHECK( to_string( 3650_days ) == "521 weeks and 3 days" );
    calendar::set_eternal_season( false );
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );
}

TEST_CASE( "months_and_days_over_time", "[calendar]" )
{
    std::vector<std::pair<month, int>> date;
    date.reserve( 364 );
    for( int i = 0; i < 12; ++i ) {
        for( int j = 0; j < ( ( i % 3 == 0 ) ? 31 : 30 ); ++j ) {
            date.emplace_back( static_cast<month>( i ), j + 1 );
        }
    }
    std::array<weekdays, 7> days;
    for( int i = 0; i < 7; ++i ) {
        days[i] = static_cast<weekdays>( i );
    }

    // jan, feb, first 19 days of mar
    size_t date_idx = 31 + 30 + 20;
    std::pair<month, int> month_day = month_and_day( calendar::turn_zero );
    REQUIRE( month_day.first == month::MARCH );
    REQUIRE( month_day.second == 21 );
    CAPTURE( month_and_day( calendar::start_of_game ).first );
    CAPTURE( month_and_day( calendar::start_of_game ).second );
    REQUIRE( date[date_idx] == month_and_day( calendar::start_of_game ) );

    // game starts on a thursday
    restore_on_out_of_scope restore_game_start( calendar::start_of_game );
    calendar::start_of_game = calendar::turn_zero;
    size_t day_idx = static_cast<size_t>( weekdays::THURSDAY );
    REQUIRE( day_of_week( calendar::start_of_game ) == days[day_idx] );

    time_point turn = calendar::turn_zero;
    int years_since_cataclysm = 0;
    // ensure it's correct for the next ten years
    for( int i = 0; i <= 364 * 10; ++i ) {
        date_idx = ( date_idx + 1 ) % date.size();
        day_idx = ( day_idx + 1 ) % days.size();
        turn += 1_days;

        std::pair<month, int> month_day = month_and_day( turn );
        weekdays day = day_of_week( turn );
        INFO( string_format( "Month %d, day %d (weekday %d)", month_day.first, month_day.second, day ) );
        INFO( string_format( "expect month %d day %d (weekday %d)", date[date_idx].first,
                             date[date_idx].second, days[day_idx] ) );

        REQUIRE( month_day == date[date_idx] );
        REQUIRE( day == days[day_idx] );

        if( month_day == std::make_pair( month::JANUARY, 1 ) ) {
            years_since_cataclysm += 1;
        }
        CHECK( years_since_cataclysm == calendar::years_since_cataclysm( turn ) );
    }
    CHECK( calendar::years_since_cataclysm( turn ) == 10 );
}

TEST_CASE( "seasons_over_time", "[calendar]" )
{
    int season_length_days = GENERATE( 91, 73 );
    on_out_of_scope restore_season_length( []() {
        calendar::set_season_length( 91 );
    } );
    CAPTURE( season_length_days );
    calendar::set_season_length( season_length_days );
    // to match the other test
    restore_on_out_of_scope restore_game_start( calendar::start_of_game );
    calendar::start_of_game = calendar::turn_zero;

    time_point turn = calendar::turn_zero;

    REQUIRE( day_of_season<int>( turn ) == 0 );
    REQUIRE( season_of_year( turn ) == SPRING );

    int years_since_cataclysm = 0;
    // next ten years
    for( int i = 0; i <= season_length_days * 4 * 10; ++i ) {
        turn += 1_days;
        int day = day_of_season<int>( turn );
        season_type season = season_of_year( turn );

        INFO( string_format( "%s %d", calendar::name_season( season ), day ) );
        REQUIRE( day == to_days<int>( turn - calendar::turn_zero ) % season_length_days );
        REQUIRE( season == static_cast<season_type>( ( to_days<int>( turn - calendar::turn_zero ) /
                 season_length_days ) % 4 ) );

        if( season == WINTER && day == season_length_days - to_days<int>( calendar::turn_zero_offset() ) ) {
            years_since_cataclysm += 1;
        }
        INFO( string_format( "year offset: %s", to_string( calendar::turn_zero_offset() ) ) );
        CHECK( years_since_cataclysm == calendar::years_since_cataclysm( turn ) );
    }
    CHECK( calendar::years_since_cataclysm( turn ) == 10 );
}

TEST_CASE( "time_point_to_string", "[calendar]" )
{
    REQUIRE( calendar::year_length() == 364_days );

    CHECK( to_string( calendar::turn_zero + 7_days ) == "Year 1, Mar 28 12:00:00AM" );
    CHECK( to_string( calendar::turn_zero + 9_days ) == "Year 1, Mar 30 12:00:00AM" );
    CHECK( to_string( calendar::turn_zero + 10_days ) == "Year 1, Apr 1 12:00:00AM" );
    CHECK( to_string( calendar::turn_zero + 30_days + 4_days ) == "Year 1, Apr 25 12:00:00AM" );
    CHECK( to_string( calendar::turn_zero + 91_days ) == "Year 1, Jun 21 12:00:00AM" );
    CHECK( to_string( calendar::turn_zero + 91_days + 30_days ) == "Year 1, Jul 21 12:00:00AM" );
    CHECK( to_string( calendar::turn_zero + 91_days * 2 + 27_days ) == "Year 1, Oct 18 12:00:00AM" );
    CHECK( to_string( calendar::turn_zero + 91_days * 3 ) == "Year 1, Dec 21 12:00:00AM" );
    CHECK( to_string( calendar::turn_zero + 364_days - calendar::turn_zero_offset() - 1_days ) ==
           "Year 1, Dec 30 12:00:00AM" );
    CHECK( to_string( calendar::turn_zero + 364_days - calendar::turn_zero_offset() ) ==
           "Year 2, Jan 1 12:00:00AM" );
    CHECK( to_string( calendar::turn_zero + 364_days ) == "Year 2, Mar 21 12:00:00AM" );
}
