#include "cata_utility.h"

#include "options.h"
#include "material.h"
#include "enums.h"
#include "item.h"
#include "creature.h"
#include "translations.h"
#include "debug.h"
#include "mapsharing.h"
#include "output.h"
#include "json.h"
#include "filesystem.h"

#include <algorithm>
#include <cmath>

double round_up( double val, unsigned int dp )
{
    const double denominator = std::pow( 10.0, double( dp ) );
    return std::ceil( denominator * val ) / denominator;
}

bool isBetween( int test, int down, int up )
{
    return test > down && test < up;
}

bool lcmatch( const std::string &str, const std::string &qry )
{
    std::string needle;
    needle.reserve( qry.size() );
    std::transform( qry.begin(), qry.end(), std::back_inserter( needle ), tolower );

    std::string haystack;
    haystack.reserve( str.size() );
    std::transform( str.begin(), str.end(), std::back_inserter( haystack ), tolower );

    return haystack.find( needle ) != std::string::npos;
}

bool list_items_match( const item *item, std::string sPattern )
{
    size_t iPos;
    bool hasExclude = false;

    if( sPattern.find( "-" ) != std::string::npos ) {
        hasExclude = true;
    }

    do {
        iPos = sPattern.find( "," );
        std::string pat = ( iPos == std::string::npos ) ? sPattern : sPattern.substr( 0, iPos );
        bool exclude = false;
        if( pat.substr( 0, 1 ) == "-" ) {
            exclude = true;
            pat = pat.substr( 1, pat.size() - 1 );
        } else if( hasExclude ) {
            hasExclude = false; //If there are non exclusive items to filter, we flip this back to false.
        }

        std::string namepat = pat;
        std::transform( namepat.begin(), namepat.end(), namepat.begin(), tolower );
        if( lcmatch( remove_color_tags( item->tname() ), namepat ) ) {
            return !exclude;
        }

        if( pat.find( "{", 0 ) != std::string::npos ) {
            std::string adv_pat_type = pat.substr( 1, pat.find( ":" ) - 1 );
            std::string adv_pat_search = pat.substr( pat.find( ":" ) + 1,
                                         ( pat.find( "}" ) - pat.find( ":" ) ) - 1 );
            std::transform( adv_pat_search.begin(), adv_pat_search.end(), adv_pat_search.begin(), tolower );
            if( adv_pat_type == "c" && lcmatch( item->get_category().name, adv_pat_search ) ) {
                return !exclude;
            } else if( adv_pat_type == "m" ) {
                for( auto material : item->made_of_types() ) {
                    if( lcmatch( material->name(), adv_pat_search ) ) {
                        return !exclude;
                    }
                }
            } else if( adv_pat_type == "dgt" && item->damage() > atoi( adv_pat_search.c_str() ) ) {
                return !exclude;
            } else if( adv_pat_type == "dlt" && item->damage() < atoi( adv_pat_search.c_str() ) ) {
                return !exclude;
            }
        }

        if( iPos != std::string::npos ) {
            sPattern = sPattern.substr( iPos + 1, sPattern.size() );
        }

    } while( iPos != std::string::npos );

    return hasExclude;
}

std::vector<map_item_stack> filter_item_stacks( std::vector<map_item_stack> stack,
        std::string filter )
{
    std::vector<map_item_stack> ret;

    std::string sFilterTemp = filter;

    for( auto &elem : stack ) {
        if( sFilterTemp == "" || list_items_match( elem.example, sFilterTemp ) ) {
            ret.push_back( elem );
        }
    }
    return ret;
}

//returns the first non priority items.
int list_filter_high_priority( std::vector<map_item_stack> &stack, std::string priorities )
{
    //TODO:optimize if necessary
    std::vector<map_item_stack> tempstack; // temp
    for( auto it = stack.begin(); it != stack.end(); ) {
        if( priorities == "" || !list_items_match( it->example, priorities ) ) {
            tempstack.push_back( *it );
            it = stack.erase( it );
        } else {
            it++;
        }
    }

    int id = stack.size();
    for( auto &elem : tempstack ) {
        stack.push_back( elem );
    }
    return id;
}

int list_filter_low_priority( std::vector<map_item_stack> &stack, int start,
                              std::string priorities )
{
    //TODO:optimize if necessary
    std::vector<map_item_stack> tempstack; // temp
    for( auto it = stack.begin() + start; it != stack.end(); ) {
        if( priorities != "" && list_items_match( it->example, priorities ) ) {
            tempstack.push_back( *it );
            it = stack.erase( it );
        } else {
            it++;
        }
    }

    int id = stack.size();
    for( auto &elem : tempstack ) {
        stack.push_back( elem );
    }
    return id;
}

// Operator overload required by sort interface.
bool pair_greater_cmp::operator()( const std::pair<int, tripoint> &a,
                                   const std::pair<int, tripoint> &b )
{
    return a.first > b.first;
}

// --- Library functions ---
// This stuff could be moved elsewhere, but there
// doesn't seem to be a good place to put it right now.

// Basic logistic function.
double logarithmic( double t )
{
    return 1 / ( 1 + exp( -t ) );
}

// Logistic curve [-6,6], flipped and scaled to
// range from 1 to 0 as pos goes from min to max.
double logarithmic_range( int min, int max, int pos )
{
    const double LOGI_CUTOFF = 4;
    const double LOGI_MIN = logarithmic( -LOGI_CUTOFF );
    const double LOGI_MAX = logarithmic( +LOGI_CUTOFF );
    const double LOGI_RANGE = LOGI_MAX - LOGI_MIN;

    if( min >= max ) {
        debugmsg( "Invalid interval (%d, %d).", min, max );
        return 0.0;
    }

    // Anything beyond (min,max) gets clamped.
    if( pos <= min ) {
        return 1.0;
    } else if( pos >= max ) {
        return 0.0;
    }

    // Normalize the pos to [0,1]
    double range = max - min;
    double unit_pos = ( pos - min ) / range;

    // Scale and flip it to [+LOGI_CUTOFF,-LOGI_CUTOFF]
    double scaled_pos = LOGI_CUTOFF - 2 * LOGI_CUTOFF * unit_pos;

    // Get the raw logistic value.
    double raw_logistic = logarithmic( scaled_pos );

    // Scale the output to [0,1]
    return ( raw_logistic - LOGI_MIN ) / LOGI_RANGE;
}

int bound_mod_to_vals( int val, int mod, int max, int min )
{
    if( val + mod > max && max != 0 ) {
        mod = std::max( max - val, 0 );
    }
    if( val + mod < min && min != 0 ) {
        mod = std::min( min - val, 0 );
    }
    return mod;
}

const char *velocity_units( const units_type vel_units )
{
    if( get_option<std::string>( "USE_METRIC_SPEEDS" ) == "mph" ) {
        return _( "mph" );
    } else {
        switch( vel_units ) {
            case VU_VEHICLE:
                return _( "km/h" );
            case VU_WIND:
                return _( "m/s" );
        }
    }
    return "error: unknown units!";
}

const char *weight_units()
{
    return get_option<std::string>( "USE_METRIC_WEIGHTS" ) == "lbs" ? _( "lbs" ) : _( "kg" );
}

/**
* Convert internal velocity units to units defined by user
*/
double convert_velocity( int velocity, const units_type vel_units )
{
    // internal units to mph conversion
    double ret = double( velocity ) / 100;

    if( get_option<std::string>( "USE_METRIC_SPEEDS" ) == "km/h" ) {
        switch( vel_units ) {
            case VU_VEHICLE:
                // mph to km/h conversion
                ret *= 1.609f;
                break;
            case VU_WIND:
                // mph to m/s conversion
                ret *= 0.447f;
                break;
        }
    }
    return ret;
}

/**
* Convert weight in grams to units defined by user (kg or lbs)
*/
double convert_weight( int weight )
{
    double ret;
    ret = double( weight );
    if( get_option<std::string>( "USE_METRIC_WEIGHTS" ) == "kg" ) {
        ret /= 1000;
    } else {
        ret /= 453.6;
    }
    return ret;
}

double temp_to_celsius( double fahrenheit )
{
    return ( ( fahrenheit - 32.0 ) * 5.0 / 9.0 );
}

float multi_lerp( const std::vector<std::pair<float, float>> &points, float x )
{
    size_t i = 0;
    while( i < points.size() && points[i].first <= x ) {
        i++;
    }

    if( i == 0 ) {
        return points.front().second;
    } else if( i >= points.size() ) {
        return points.back().second;
    }

    // How far are we along the way from last threshold to current one
    const float t = ( x - points[i - 1].first ) /
                    ( points[i].first - points[i - 1].first );

    // Linear interpolation of values at relevant thresholds
    return ( t * points[i].second ) + ( ( 1 - t ) * points[i - 1].second );
}

ofstream_wrapper::ofstream_wrapper( const std::string &path )
{
    file_stream.open( path.c_str(), std::ios::binary );
    if( !file_stream.is_open() ) {
        throw std::runtime_error( "opening file failed" );
    }
}

ofstream_wrapper::~ofstream_wrapper() = default;

void ofstream_wrapper::close()
{
    file_stream.close();
    if( file_stream.fail() ) {
        throw std::runtime_error( "writing to file failed" );
    }
}

bool write_to_file( const std::string &path, const std::function<void( std::ostream & )> &writer,
                    const char *const fail_message )
{
    try {
        ofstream_wrapper fout( path );
        writer( fout.stream() );
        fout.close();
        return true;

    } catch( const std::exception &err ) {
        if( fail_message ) {
            popup( _( "Failed to write %1$s to \"%2$s\": %3$s" ), fail_message, path.c_str(), err.what() );
        }
        return false;
    }
}

ofstream_wrapper_exclusive::ofstream_wrapper_exclusive( const std::string &path )
    : path( path )
{
    fopen_exclusive( file_stream, path.c_str(), std::ios::binary );
    if( !file_stream.is_open() ) {
        throw std::runtime_error( _( "opening file failed" ) );
    }
}

ofstream_wrapper_exclusive::~ofstream_wrapper_exclusive()
{
    if( file_stream.is_open() ) {
        fclose_exclusive( file_stream, path.c_str() );
    }
}

void ofstream_wrapper_exclusive::close()
{
    fclose_exclusive( file_stream, path.c_str() );
    if( file_stream.fail() ) {
        throw std::runtime_error( _( "writing to file failed" ) );
    }
}

bool write_to_file_exclusive( const std::string &path,
                              const std::function<void( std::ostream & )> &writer, const char *const fail_message )
{
    try {
        ofstream_wrapper_exclusive fout( path );
        writer( fout.stream() );
        fout.close();
        return true;

    } catch( const std::exception &err ) {
        if( fail_message ) {
            popup( _( "Failed to write %1$s to \"%2$s\": %3$s" ), fail_message, path.c_str(), err.what() );
        }
        return false;
    }
}

bool read_from_file( const std::string &path, const std::function<void( std::istream & )> &reader )
{
    try {
        std::ifstream fin( path, std::ios::binary );
        if( !fin ) {
            throw std::runtime_error( "opening file failed" );
        }
        reader( fin );
        if( fin.bad() ) {
            throw std::runtime_error( "reading file failed" );
        }
        return true;

    } catch( const std::exception &err ) {
        popup( _( "Failed to read from \"%1$s\": %2$s" ), path.c_str(), err.what() );
        return false;
    }
}

bool read_from_file( const std::string &path, const std::function<void( JsonIn & )> &reader )
{
    return read_from_file( path, [&reader]( std::istream & fin ) {
        JsonIn jsin( fin );
        reader( jsin );
    } );
}

bool read_from_file( const std::string &path, JsonDeserializer &reader )
{
    return read_from_file( path, [&reader]( JsonIn & jsin ) {
        reader.deserialize( jsin );
    } );
}

bool read_from_file_optional( const std::string &path,
                              const std::function<void( std::istream & )> &reader )
{
    // Note: slight race condition here, but we'll ignore it. Worst case: the file
    // exists and got removed before reading it -> reading fails with a message
    // Or file does not exists, than everything works fine because it's optional anyway.
    return file_exist( path ) && read_from_file( path, reader );
}

bool read_from_file_optional( const std::string &path,
                              const std::function<void( JsonIn & )> &reader )
{
    return read_from_file_optional( path, [&reader]( std::istream & fin ) {
        JsonIn jsin( fin );
        reader( jsin );
    } );
}

bool read_from_file_optional( const std::string &path, JsonDeserializer &reader )
{
    return read_from_file_optional( path, [&reader]( JsonIn & jsin ) {
        reader.deserialize( jsin );
    } );
}
