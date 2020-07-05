#include <algorithm>
#include <cstdlib>
#include <memory>
#include <vector>

#include "calendar.h"
#include "catch/catch.hpp"
#include "point.h"
#include "weather.h"
#include "weather_gen.h"

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

    const weather_generator &wgen = get_weather().get_cur_weather_gen();
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

TEST_CASE( "local wind chill calculation", "[weather][wind_chill]" )
{
    // `get_local_windchill` returns degrees F offset from current temperature,
    // representing the amount of temperature difference from wind chill alone.
    //
    // It uses one of two formulas or models depending on the current temperature.
    // Below 50F, the North American / UK "wind chill index" is used. At 50F or above,
    // the Australian "apparent temperature" is used.
    //
    // All "quoted text" below is paraphrased from the Wikipedia article:
    // https://en.wikipedia.org/wiki/Wind_chill

    // CHECK expressions have the expected result on the left for readability.

    double temp_f;
    double humidity;
    double wind_mph;

    SECTION( "below 50F - North American and UK wind chill index" ) {

        // "Windchill temperature is defined only for temperatures at or below 10C (50F)
        // and wind speeds above 4.8 kilometres per hour (3.0 mph)."

        WHEN( "wind speed is less than 3 mph" ) {
            THEN( "windchill is undefined and effectively 0" ) {
                CHECK( 0 == get_local_windchill( 30.0f, 0.0f, 2.9f ) );
                CHECK( 0 == get_local_windchill( 30.0f, 0.0f, 2.0f ) );
                CHECK( 0 == get_local_windchill( 30.0f, 0.0f, 1.0f ) );
                CHECK( 0 == get_local_windchill( 30.0f, 0.0f, 0.0f ) );
            }
        }

        // "As the air temperature falls, the chilling effect of any wind that is present increases.
        // For example, a 16 km/h (9.9 mph) wind will lower the apparent temperature by a wider
        // margin at an air temperature of −20C (−4F), than a wind of the same speed would if
        // the air temperature were −10C (14F)."

        GIVEN( "constant wind of 10mph" ) {
            wind_mph = 10.0f;

            WHEN( "temperature is -10C (14F)" ) {
                temp_f = 14.0f;

                THEN( "the wind chill effect is -12F" ) {
                    CHECK( -12 == get_local_windchill( temp_f, 0.0f, wind_mph ) );
                }
            }

            WHEN( "temperature is -20C (-4F)" ) {
                temp_f = -4.0f;

                THEN( "there is more wind chill, an effect of -16F" ) {
                    CHECK( -16 == get_local_windchill( temp_f, 0.0f, wind_mph ) );
                }
            }
        }

        // More generally:

        WHEN( "wind speed is at least 3 mph" ) {
            THEN( "windchill gets colder as temperature decreases" ) {
                CHECK( -8 == get_local_windchill( 40.0f, 0.0f, 15.0f ) );
                CHECK( -10 == get_local_windchill( 30.0f, 0.0f, 15.0f ) );
                CHECK( -13 == get_local_windchill( 20.0f, 0.0f, 15.0f ) );
                CHECK( -16 == get_local_windchill( 10.0f, 0.0f, 15.0f ) );
                CHECK( -19 == get_local_windchill( 0.0f, 0.0f, 15.0f ) );
                CHECK( -22 == get_local_windchill( -10.0f, 0.0f, 15.0f ) );
                CHECK( -25 == get_local_windchill( -20.0f, 0.0f, 15.0f ) );
                CHECK( -27 == get_local_windchill( -30.0f, 0.0f, 15.0f ) );
                CHECK( -30 == get_local_windchill( -40.0f, 0.0f, 15.0f ) );
            }
        }

        // "When the temperature is −20C (−4F) and the wind speed is 5 km/h (3.1 mph),
        // the wind chill index is −24C. If the temperature remains at −20C and the wind
        // speed increases to 30 km/h (19 mph), the wind chill index falls to −33C."

        GIVEN( "constant temperature of -20C (-4F)" ) {
            temp_f = -4.0f;

            WHEN( "wind speed is 3.1mph" ) {
                wind_mph = 3.1f;

                THEN( "wind chill is -24C (-11.2F)" ) {
                    // -4C offset from -20C =~ -7.2F offset from -4F
                    CHECK( -7 == get_local_windchill( temp_f, 0.0f, wind_mph ) );
                }
            }
            WHEN( "wind speed increases to 19mph" ) {
                wind_mph = 19.0f;

                THEN( "wind chill is -33C (-27.4F)" ) {
                    // -13C offset from -20C =~ -23.4F offset from -4F
                    // NOTE: This should be -23, but we can forgive an off-by-one
                    CHECK( -22 == get_local_windchill( temp_f, 0.0f, wind_mph ) );
                }
            }
        }

        // Or more generally:

        WHEN( "wind speed is at least 3 mph" ) {
            THEN( "windchill gets colder as wind increases" ) {
                // Just below freezing
                CHECK( -2 == get_local_windchill( 30.0f, 0.0f, 3.0f ) );
                CHECK( -4 == get_local_windchill( 30.0f, 0.0f, 4.0f ) );
                CHECK( -5 == get_local_windchill( 30.0f, 0.0f, 5.0f ) );
                CHECK( -8 == get_local_windchill( 30.0f, 0.0f, 10.0f ) );
                CHECK( -12 == get_local_windchill( 30.0f, 0.0f, 20.0f ) );
                CHECK( -15 == get_local_windchill( 30.0f, 0.0f, 30.0f ) );
                CHECK( -16 == get_local_windchill( 30.0f, 0.0f, 40.0f ) );
                CHECK( -18 == get_local_windchill( 30.0f, 0.0f, 50.0f ) );

                // Well below zero
                CHECK( -10 == get_local_windchill( -30.0f, 0.0f, 3.0f ) );
                CHECK( -13 == get_local_windchill( -30.0f, 0.0f, 4.0f ) );
                CHECK( -15 == get_local_windchill( -30.0f, 0.0f, 5.0f ) );
                CHECK( -23 == get_local_windchill( -30.0f, 0.0f, 10.0f ) );
                CHECK( -31 == get_local_windchill( -30.0f, 0.0f, 20.0f ) );
                CHECK( -36 == get_local_windchill( -30.0f, 0.0f, 30.0f ) );
                CHECK( -40 == get_local_windchill( -30.0f, 0.0f, 40.0f ) );
                CHECK( -43 == get_local_windchill( -30.0f, 0.0f, 50.0f ) );
            }
        }

        // The function accepts a humidity argument, but this model does not use it.
        //
        // "The North American formula was designed to be applied at low temperatures
        // (as low as −46C or −50F) when humidity levels are also low."

        THEN( "wind chill index is unaffected by humidity" ) {
            CHECK( -6 == get_local_windchill( 40.0f, 0.0f, 10.0f ) );
            CHECK( -6 == get_local_windchill( 40.0f, 50.0f, 10.0f ) );
            CHECK( -6 == get_local_windchill( 40.0f, 100.0f, 10.0f ) );

            CHECK( -22 == get_local_windchill( 10.0f, 0.0f, 30.0f ) );
            CHECK( -22 == get_local_windchill( 10.0f, 50.0f, 30.0f ) );
            CHECK( -22 == get_local_windchill( 10.0f, 100.0f, 30.0f ) );

            CHECK( -33 == get_local_windchill( -20.0f, 0.0f, 30.0f ) );
            CHECK( -33 == get_local_windchill( -20.0f, 50.0f, 30.0f ) );
            CHECK( -33 == get_local_windchill( -20.0f, 100.0f, 30.0f ) );
        }
    }

    SECTION( "50F and above - Australian apparent temperature" ) {
        GIVEN( "constant temperature of 50F" ) {
            temp_f = 50.0f;

            WHEN( "wind is steady at 10mph" ) {
                wind_mph = 10.0f;

                THEN( "apparent temp increases as humidity increases" ) {
                    CHECK( -12 == get_local_windchill( temp_f, 0.0f, wind_mph ) );
                    CHECK( -11 == get_local_windchill( temp_f, 20.0f, wind_mph ) );
                    CHECK( -9 == get_local_windchill( temp_f, 40.0f, wind_mph ) );
                    CHECK( -8 == get_local_windchill( temp_f, 60.0f, wind_mph ) );
                    CHECK( -7 == get_local_windchill( temp_f, 80.0f, wind_mph ) );
                    CHECK( -5 == get_local_windchill( temp_f, 100.0f, wind_mph ) );
                }
            }

            WHEN( "humidity is steady at 90%%" ) {
                humidity = 90.0f;

                THEN( "apparent temp decreases as wind increases" ) {
                    CHECK( 0 == get_local_windchill( temp_f, humidity, 0.0f ) );
                    CHECK( -6 == get_local_windchill( temp_f, humidity, 10.0f ) );
                    CHECK( -11 == get_local_windchill( temp_f, humidity, 20.0f ) );
                    CHECK( -17 == get_local_windchill( temp_f, humidity, 30.0f ) );
                    CHECK( -23 == get_local_windchill( temp_f, humidity, 40.0f ) );
                    CHECK( -28 == get_local_windchill( temp_f, humidity, 50.0f ) );
                }
            }
        }

        GIVEN( "humidity is zero" ) {
            humidity = 0.0f;

            THEN( "apparent temp offset is only influenced by wind speed" ) {
                // No wind still gets -7 for some reason
                CHECK( -7 == get_local_windchill( 50.0f, humidity, 0.0f ) );
                CHECK( -7 == get_local_windchill( 70.0f, humidity, 0.0f ) );
                CHECK( -7 == get_local_windchill( 90.0f, humidity, 0.0f ) );

                // 25mph wind == -21F to temperature
                CHECK( -21 == get_local_windchill( 50.0f, humidity, 25.0f ) );
                CHECK( -21 == get_local_windchill( 70.0f, humidity, 25.0f ) );
                CHECK( -21 == get_local_windchill( 90.0f, humidity, 25.0f ) );
            }
        }

        GIVEN( "humidity is 50 percent" ) {
            humidity = 50.0f;

            WHEN( "there is no wind" ) {
                wind_mph = 0.0f;

                THEN( "apparent temp increases more as it gets hotter" ) {
                    CHECK( -3 == get_local_windchill( 50.0f, humidity, wind_mph ) );
                    CHECK( -1 == get_local_windchill( 60.0f, humidity, wind_mph ) );
                    CHECK( 1 == get_local_windchill( 70.0f, humidity, wind_mph ) );
                    CHECK( 4 == get_local_windchill( 80.0f, humidity, wind_mph ) );
                    CHECK( 8 == get_local_windchill( 90.0f, humidity, wind_mph ) );
                    CHECK( 13 == get_local_windchill( 100.0f, humidity, wind_mph ) );
                }
            }

            WHEN( "wind is steady at 10mph" ) {
                wind_mph = 10.0f;

                THEN( "apparent temp is less but still increases more as it gets hotter" ) {
                    CHECK( -9 == get_local_windchill( 50.0f, humidity, wind_mph ) );
                    CHECK( -7 == get_local_windchill( 60.0f, humidity, wind_mph ) );
                    CHECK( -5 == get_local_windchill( 70.0f, humidity, wind_mph ) );
                    CHECK( -2 == get_local_windchill( 80.0f, humidity, wind_mph ) );
                    CHECK( 2 == get_local_windchill( 90.0f, humidity, wind_mph ) );
                    CHECK( 7 == get_local_windchill( 100.0f, humidity, wind_mph ) );
                }
            }
        }
    }
}

