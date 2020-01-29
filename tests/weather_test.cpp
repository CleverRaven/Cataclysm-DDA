#include "catch/catch.hpp"

#include <algorithm>

#include "calendar.h"
#include "point.h"
#include "weather_gen.h"
#include "game.h"

static double mean_abs_running_diff( std::vector<double> const &v )
{
    double x = 0;
    int n = v.size() - 1;
    for( int i = 0 ; i < n ; ++i ) {
        x += std::abs( v[i + 1] - v[i] );
    }
    return x / n;
}

static double mean_pairwise_diffs( std::vector<double> const &a, std::vector<double> const &b )
{
    double x = 0;
    int n = a.size();
    for( int i = 0 ; i < n ; ++i ) {
        x += a[i] - b[i];
    }
    return x / n;
}

static double proportion_gteq_x( std::vector<double> const &v, double x )
{
    int count = 0;
    for( auto i : v ) {
        count += ( i >= x );
    }
    return static_cast<double>( count ) / v.size();
}

TEST_CASE( "weather realism" )
// Check our simulated weather against numbers from real data
// from a few years in a few locations in New England. The numbers
// are based on NOAA's Local Climatological Data (LCD). Analysis code
// can be found at:
// https://gist.github.com/Kodiologist/e2f1e6685e8fd865650f97bb6a67ad07
{
    // Try a few randomly selected seeds.
    const std::vector<unsigned> seeds = {317'024'741, 870'078'684, 1'192'447'748};

    const weather_generator &wgen = g->weather.get_cur_weather_gen();
    const time_point begin = 0;
    const time_point end = begin + calendar::year_length();
    const int n_days = to_days<int>( end - begin );
    const int n_hours = to_hours<int>( 1_days );
    const int n_minutes = to_minutes<int>( 1_days );

    for( auto seed : seeds ) {
        std::vector<std::vector<double>> temperature;
        temperature.resize( n_days, std::vector<double>( n_minutes, 0 ) );
        std::vector<double> hourly_precip;
        hourly_precip.resize( n_days * n_hours, 0 );

        // Collect generated weather data for a single year.
        for( time_point i = begin ; i < end ; i += 1_minutes ) {
            w_point w = wgen.get_weather( tripoint_zero, to_turn<int>( i ), seed );
            int day = to_days<int>( time_past_new_year( i ) );
            int minute = to_minutes<int>( time_past_midnight( i ) );
            temperature[day][minute] = w.temperature;
            int hour = to_hours<int>( time_past_new_year( i ) );
            hourly_precip[hour] +=
                precip_mm_per_hour(
                    weather::precip( wgen.get_weather_conditions( w ) ) )
                / 60;
        }

        // Collect daily highs and lows.
        std::vector<double> highs( n_days );
        std::vector<double> lows( n_days );
        for( int do_highs = 0 ; do_highs < 2 ; ++do_highs ) {
            std::vector<double> &t = do_highs ? highs : lows;
            std::transform( temperature.begin(), temperature.end(), t.begin(),
            [&]( std::vector<double> const & day ) {
                return do_highs
                       ? *std::max_element( day.begin(), day.end() )
                       : *std::min_element( day.begin(), day.end() );
            } );

            // Check the mean absolute difference between the highs or lows
            // of adjacent days (Fahrenheit).
            const double d = mean_abs_running_diff( t );
            CHECK( d >= ( do_highs ? 5.5 : 4 ) );
            CHECK( d <= ( do_highs ? 7.5 : 7 ) );
        }

        // Check the daily mean of the range in temperatures (Fahrenheit).
        const double mean_of_ranges = mean_pairwise_diffs( highs, lows );
        CHECK( mean_of_ranges >= 14 );
        CHECK( mean_of_ranges <= 25 );

        // Check the proportion of hours with light precipitation
        // or more, counting snow (mm of rain equivalent per hour).
        const double at_least_light_precip = proportion_gteq_x(
                hourly_precip, 1 );
        CHECK( at_least_light_precip >= .025 );
        CHECK( at_least_light_precip <= .05 );

        // Likewise for heavy precipitation.
        const double heavy_precip = proportion_gteq_x(
                                        hourly_precip, 2.5 );
        CHECK( heavy_precip >= .005 );
        CHECK( heavy_precip <= .02 );
    }
}
