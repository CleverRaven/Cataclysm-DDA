#include "cata_utility.h"

#include <algorithm>
#include <cmath>
#include <locale>
#include <string>

#include "debug.h"
#include "enums.h"
#include "filesystem.h"
#include "json.h"
#include "mapsharing.h"
#include "material.h"
#include "options.h"
#include "output.h"
#include "rng.h"
#include "translations.h"
#include "units.h"

static double pow10( unsigned int n )
{
    double ret = 1;
    double tmp = 10;
    while( n ) {
        if( n & 1 ) {
            ret *= tmp;
        }
        tmp *= tmp;
        n >>= 1;
    }
    return ret;
}

double round_up( double val, unsigned int dp )
{
    // Some implementations of std::pow does not return the accurate result even
    // for small powers of 10, so we use a specialized routine to calculate them.
    const double denominator = pow10( dp );
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

bool match_include_exclude( const std::string &text, std::string filter )
{
    size_t iPos;
    bool found = false;

    if( filter.empty() ) {
        return false;
    }

    do {
        iPos = filter.find( ',' );

        std::string term = iPos == std::string::npos ? filter : filter.substr( 0, iPos );
        const bool exclude = term.substr( 0, 1 ) == "-";
        if( exclude ) {
            term = term.substr( 1 );
        }

        if( ( !found || exclude ) && lcmatch( text, term ) ) {
            if( exclude ) {
                return false;
            }

            found = true;
        }

        if( iPos != std::string::npos ) {
            filter = filter.substr( iPos + 1, filter.size() );
        }
    } while( iPos != std::string::npos );

    return found;
}

// --- Library functions ---
// This stuff could be moved elsewhere, but there
// doesn't seem to be a good place to put it right now.

double logarithmic( double t )
{
    return 1 / ( 1 + exp( -t ) );
}

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
    } else if( get_option<std::string>( "USE_METRIC_SPEEDS" ) == "t/t" ) {
        //~ vehicle speed tiles per turn
        return _( "t/t" );
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

const char *volume_units_abbr()
{
    const std::string vol_units = get_option<std::string>( "VOLUME_UNITS" );
    if( vol_units == "c" ) {
        return pgettext( "Volume unit", "c" );
    } else if( vol_units == "l" ) {
        return pgettext( "Volume unit", "L" );
    } else {
        return pgettext( "Volume unit", "qt" );
    }
}

const char *volume_units_long()
{
    const std::string vol_units = get_option<std::string>( "VOLUME_UNITS" );
    if( vol_units == "c" ) {
        return _( "cup" );
    } else if( vol_units == "l" ) {
        return _( "liter" );
    } else {
        return _( "quart" );
    }
}

double convert_velocity( int velocity, const units_type vel_units )
{
    const std::string type = get_option<std::string>( "USE_METRIC_SPEEDS" );
    // internal units to mph conversion
    double ret = double( velocity ) / 100;

    if( type == "km/h" ) {
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
    } else if( type == "t/t" ) {
        ret /= 4;
    }

    return ret;
}

double convert_weight( const units::mass &weight )
{
    double ret = to_gram( weight );
    if( get_option<std::string>( "USE_METRIC_WEIGHTS" ) == "kg" ) {
        ret /= 1000;
    } else {
        ret /= 453.6;
    }
    return ret;
}

double convert_volume( int volume )
{
    return convert_volume( volume, nullptr );
}

double convert_volume( int volume, int *out_scale )
{
    double ret = volume;
    int scale = 0;
    const std::string vol_units = get_option<std::string>( "VOLUME_UNITS" );
    if( vol_units == "c" ) {
        ret *= 0.004;
        scale = 1;
    } else if( vol_units == "l" ) {
        ret *= 0.001;
        scale = 2;
    } else {
        ret *= 0.00105669;
        scale = 2;
    }
    if( out_scale != nullptr ) {
        *out_scale = scale;
    }
    return ret;
}

double temp_to_celsius( double fahrenheit )
{
    return ( ( fahrenheit - 32.0 ) * 5.0 / 9.0 );
}

double temp_to_kelvin( double fahrenheit )
{
    return temp_to_celsius( fahrenheit ) + 273.15;
}

double clamp_to_width( double value, int width, int &scale )
{
    return clamp_to_width( value, width, scale, nullptr );
}

double clamp_to_width( double value, int width, int &scale, bool *out_truncated )
{
    if( out_truncated != nullptr ) {
        *out_truncated = false;
    }
    if( value >= std::pow( 10.0, width ) ) {
        // above the maximum number we can fit in the width without decimal
        // show the biggest number we can without decimal
        // flag as truncated
        value = std::pow( 10.0, width ) - 1.0;
        scale = 0;
        if( out_truncated != nullptr ) {
            *out_truncated = true;
        }
    } else if( scale > 0 ) {
        for( int s = 1; s <= scale; s++ ) {
            int scale_width = 1 + s; // 1 decimal separator + "s"
            if( width > scale_width && value >= std::pow( 10.0, width - scale_width ) ) {
                // above the maximum number we can fit in the width with "s" decimals
                // show this number with one less decimal than "s"
                scale = s - 1;
                break;
            }
        }
    }
    return value;
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

std::istream &safe_getline( std::istream &ins, std::string &str )
{
    str.clear();
    std::istream::sentry se( ins, true );
    std::streambuf *sb = ins.rdbuf();

    while( true ) {
        int c = sb->sbumpc();
        switch( c ) {
            case '\n':
                return ins;
            case '\r':
                if( sb->sgetc() == '\n' ) {
                    sb->sbumpc();
                }
                return ins;
            case EOF:
                if( str.empty() ) {
                    ins.setstate( std::ios::eofbit );
                }
                return ins;
            default:
                str += static_cast<char>( c );
        }
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
        debugmsg( _( "Failed to read from \"%1$s\": %2$s" ), path.c_str(), err.what() );
        return false;
    }
}

bool read_from_file_json( const std::string &path, const std::function<void( JsonIn & )> &reader )
{
    return read_from_file( path, [&reader]( std::istream & fin ) {
        JsonIn jsin( fin );
        reader( jsin );
    } );
}

bool read_from_file( const std::string &path, JsonDeserializer &reader )
{
    return read_from_file_json( path, [&reader]( JsonIn & jsin ) {
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

bool read_from_file_optional_json( const std::string &path,
                                   const std::function<void( JsonIn & )> &reader )
{
    return read_from_file_optional( path, [&reader]( std::istream & fin ) {
        JsonIn jsin( fin );
        reader( jsin );
    } );
}

bool read_from_file_optional( const std::string &path, JsonDeserializer &reader )
{
    return read_from_file_optional_json( path, [&reader]( JsonIn & jsin ) {
        reader.deserialize( jsin );
    } );
}

std::string obscure_message( const std::string &str, std::function<char()> f )
{
    //~ translators: place some random 1-width characters here in your language if possible, or leave it as is
    std::string gibberish_narrow = _( "abcdefghijklmnopqrstuvwxyz" );
    std::string gibberish_wide =
        //~ translators: place some random 2-width characters here in your language if possible, or leave it as is
        _( "に坂索トし荷測のンおク妙免イロコヤ梅棋厚れ表幌" );
    std::wstring w_gibberish_narrow = utf8_to_wstr( gibberish_narrow );
    std::wstring w_gibberish_wide = utf8_to_wstr( gibberish_wide );
    std::wstring w_str = utf8_to_wstr( str );
    char transformation[2] = { 0 }; // a trailing NULL terminator is necessary for utf8_width function
    for( size_t i = 0; i < w_str.size(); ++i ) {
        transformation[0] = f();
        std::string this_char = wstr_to_utf8( std::wstring( 1, w_str[i] ) );
        if( transformation[0] == -1 ) {
            continue;
        } else if( transformation[0] == 0 ) {
            if( utf8_width( this_char ) == 1 ) {
                w_str[i] = random_entry( w_gibberish_narrow );
            } else {
                w_str[i] = random_entry( w_gibberish_wide );
            }
        } else {
            // Only support the case e.g. replace current character to symbols like # or ?
            if( utf8_width( transformation ) != 1 ) {
                debugmsg( "target character isn't narrow" );
            }
            // A 2-width wide character in the original string should be replace by two narrow characters
            w_str.replace( i, 1, utf8_to_wstr( std::string( utf8_width( this_char ), transformation[0] ) ) );
        }
    }
    std::string result = wstr_to_utf8( w_str );
    if( utf8_width( str ) != utf8_width( result ) ) {
        debugmsg( "utf8_width differ between original string and obscured string" );
    }
    return result;
}

std::string serialize_wrapper( const std::function<void( JsonOut & )> &callback )
{
    std::ostringstream buffer;
    JsonOut jsout( buffer );
    callback( jsout );
    return buffer.str();
}

void deserialize_wrapper( const std::function<void( JsonIn & )> &callback, const std::string &data )
{
    std::istringstream buffer( data );
    JsonIn jsin( buffer );
    callback( jsin );
}

bool string_starts_with( const std::string &s1, const std::string &s2 )
{
    return s1.compare( 0, s2.size(), s2 ) == 0;
}

bool string_ends_with( const std::string &s1, const std::string &s2 )
{
    return s1.size() >= s2.size() &&
           s1.compare( s1.size() - s2.size(), s2.size(), s2 ) == 0;
}
