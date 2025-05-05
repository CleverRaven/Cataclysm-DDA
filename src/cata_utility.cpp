#include "cata_utility.h"

#include <zconf.h>
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <exception>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "cached_options.h"
#include "cata_path.h"
#include "catacharset.h"
#include "debug.h"
#include "filesystem.h"
#include "flexbuffer_json.h"
#include "json.h"
#include "json_loader.h"
#include "ofstream_wrapper.h"
#include "options.h"
#include "output.h"
#include "pinyin.h"
#include "rng.h"
#include "string_formatter.h"
#include "translation.h"
#include "translations.h"
#include "unicode.h"
#include "zlib.h"

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

int divide_round_down( int a, int b )
{
    if( b < 0 ) {
        a = -a;
        b = -b;
    }
    if( a >= 0 ) {
        return a / b;
    } else {
        return -( ( -a + b - 1 ) / b );
    }
}

int modulo( int v, int m )
{
    // C++11: negative v and positive m result in negative v%m (or 0),
    // but this is supposed to be mathematical modulo: 0 <= v%m < m,
    const int r = v % m;
    // Adding m in that (and only that) case.
    return r >= 0 ? r : r + m;
}

bool isBetween( int test, int down, int up )
{
    return test > down && test < up;
}

bool lcmatch( std::string_view str, std::string_view qry )
{
    // It will be quite common for the query string to be empty.  Anything will
    // match in that case, so short-circuit and avoid the expensive
    // conversions.
    if( qry.empty() ) {
        return true;
    }

    std::u32string u32_str = utf8_to_utf32( str );
    std::u32string u32_qry = utf8_to_utf32( qry );
    std::for_each( u32_str.begin(), u32_str.end(), u32_to_lowercase );
    std::for_each( u32_qry.begin(), u32_qry.end(), u32_to_lowercase );
    // First try match their lowercase forms
    if( u32_str.find( u32_qry ) != std::u32string::npos ) {
        return true;
    }
    // Then try removing accents from str ONLY
    std::for_each( u32_str.begin(), u32_str.end(), remove_accent );
    if( u32_str.find( u32_qry ) != std::u32string::npos ) {
        return true;
    }
    if( use_pinyin_search ) {
        // Finally, try to convert the string to pinyin and compare
        return pinyin::pinyin_match( u32_str, u32_qry );
    }
    return false;
}

bool lcmatch( const translation &str, std::string_view qry )
{
    return lcmatch( str.translated(), qry );
}

bool match_include_exclude( std::string_view text, std::string filter )
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
    return 1 / ( 1 + std::exp( -t ) );
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

double kelvin_to_fahrenheit( double kelvin )
{
    return 1.8 * ( kelvin - 273.15 ) + 32;
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
            // 1 decimal separator + "s"
            int scale_width = 1 + s;
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

void write_to_file( const std::string &path, const std::function<void( std::ostream & )> &writer )
{
    // Any of the below may throw. ofstream_wrapper will clean up the temporary path on its own.
    ofstream_wrapper fout( std::filesystem::u8path( path ), std::ios::binary );
    writer( fout.stream() );
    fout.close();
}

bool write_to_file( const std::string &path, const std::function<void( std::ostream & )> &writer,
                    const char *const fail_message )
{
    try {
        write_to_file( path, writer );
        return true;

    } catch( const std::exception &err ) {
        if( fail_message ) {
            const std::string msg =
                string_format( _( "Failed to write %1$s to \"%2$s\": %3$s" ),
                               fail_message, path, err.what() );
            if( test_mode ) {
                DebugLog( D_ERROR, DC_ALL ) << msg;
            } else {
                popup( "%s", msg );
            }
        }
        return false;
    }
}

void write_to_file( const cata_path &path, const std::function<void( std::ostream & )> &writer )
{
    // Any of the below may throw. ofstream_wrapper will clean up the temporary path on its own.
    ofstream_wrapper fout( path.get_unrelative_path(), std::ios::binary );
    writer( fout.stream() );
    fout.close();
}

bool write_to_file( const cata_path &path, const std::function<void( std::ostream & )> &writer,
                    const char *const fail_message )
{
    try {
        write_to_file( path, writer );
        return true;

    } catch( const std::exception &err ) {
        if( fail_message ) {
            const std::string msg =
                string_format( _( "Failed to write %1$s to \"%2$s\": %3$s" ),
                               fail_message, path.generic_u8string(), err.what() );
            if( test_mode ) {
                DebugLog( D_ERROR, DC_ALL ) << msg;
            } else {
                popup( "%s", msg );
            }
        }
        return false;
    }
}

ofstream_wrapper::ofstream_wrapper( const std::filesystem::path &path,
                                    const std::ios::openmode mode )
    : path( path )

{
    open( mode );
}

ofstream_wrapper::~ofstream_wrapper()
{
    try {
        close();
    } catch( ... ) {
        // ignored in destructor
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

namespace
{

std::string read_compressed_file_to_string( std::istream &fin )
{
    std::string outstring;

    std::ostringstream deflated_contents_stream;
    std::string str;

    deflated_contents_stream << fin.rdbuf();
    str = deflated_contents_stream.str();

    z_stream zs;
    memset( &zs, 0, sizeof( zs ) );

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    if( inflateInit2( &zs, MAX_WBITS | 16 ) != Z_OK ) {
#pragma GCC diagnostic pop
        throw std::runtime_error( "inflateInit failed while decompressing." );
    }

    zs.next_in = reinterpret_cast<unsigned char *>( const_cast<char *>( str.data() ) );
    zs.avail_in = str.size();

    int ret;
    std::array<char, 32768> outbuffer;

    // get the decompressed bytes blockwise using repeated calls to inflate
    do {
        zs.next_out = reinterpret_cast<Bytef *>( outbuffer.data() );
        zs.avail_out = sizeof( outbuffer );

        ret = inflate( &zs, 0 );

        if( outstring.size() < static_cast<size_t>( zs.total_out ) ) {
            outstring.append( outbuffer.data(),
                              zs.total_out - outstring.size() );
        }

    } while( ret == Z_OK );

    inflateEnd( &zs );

    if( ret != Z_STREAM_END ) { // an error occurred that was not EOF
        std::ostringstream oss;
        oss << "Exception during zlib decompression: (" << ret << ") "
            << zs.msg;
        throw std::runtime_error( oss.str() );
    }
    return outstring;
}

} // namespace

bool read_from_file( const cata_path &path, const std::function<void( std::istream & )> &reader )
{
    return read_from_file( path.get_unrelative_path(), reader );
}

bool read_from_file( const std::filesystem::path &path,
                     const std::function<void( std::istream & )> &reader )
{
    std::unique_ptr<std::istream> finp = read_maybe_compressed_file( path );
    if( !finp ) {
        return false;
    }
    try {
        reader( *finp );
    } catch( const std::exception &err ) {
        debugmsg( _( "Failed to read from \"%1$s\": %2$s" ), path.generic_u8string().c_str(), err.what() );
        return false;
    }
    return true;
}

bool read_from_file( const std::string &path, const std::function<void( std::istream & )> &reader )
{
    return read_from_file( std::filesystem::u8path( path ), reader );
}

std::unique_ptr<std::istream> read_maybe_compressed_file( const std::string &path )
{
    return read_maybe_compressed_file( std::filesystem::u8path( path ) );
}

std::unique_ptr<std::istream> read_maybe_compressed_file( const std::filesystem::path &path )
{
    try {
        std::ifstream fin( path, std::ios::binary );
        if( !fin ) {
            throw std::runtime_error( "opening file failed" );
        }

        // check if file is gzipped
        // (byte1 == 0x1f) && (byte2 == 0x8b)
        std::array<char, 2> header;
        fin.read( header.data(), 2 );
        fin.clear();
        fin.seekg( 0, std::ios::beg ); // reset read position

        if( ( header[0] == '\x1f' ) && ( header[1] == '\x8b' ) ) {
            std::string outstring = read_compressed_file_to_string( fin );
            std::stringstream inflated_contents_stream;
            inflated_contents_stream.write( outstring.data(), outstring.size() );
            return std::make_unique<std::stringstream>( std::move( inflated_contents_stream ) );
        } else {
            return std::make_unique<std::ifstream>( std::move( fin ) );
        }
        if( fin.bad() ) {
            throw std::runtime_error( "reading file failed" );
        }

    } catch( const std::exception &err ) {
        debugmsg( _( "Failed to read from \"%1$s\": %2$s" ), path.generic_u8string().c_str(), err.what() );
    }
    return nullptr;
}

std::unique_ptr<std::istream> read_maybe_compressed_file( const cata_path &path )
{
    return read_maybe_compressed_file( path.get_unrelative_path() );
}

std::optional<std::string> read_whole_file( const std::string &path )
{
    return read_whole_file( std::filesystem::u8path( path ) );
}

std::optional<std::string> read_whole_file( const std::filesystem::path &path )
{
    std::string outstring;
    try {
        std::ifstream fin( path, std::ios::binary );
        if( !fin ) {
            throw std::runtime_error( "opening file failed" );
        }

        // check if file is gzipped
        // (byte1 == 0x1f) && (byte2 == 0x8b)
        std::array<char, 2> header;
        fin.read( header.data(), 2 );
        fin.clear();
        fin.seekg( 0, std::ios::beg ); // reset read position

        if( ( header[0] == '\x1f' ) && ( header[1] == '\x8b' ) ) {
            outstring = read_compressed_file_to_string( fin );
        } else {
            fin.seekg( 0, std::ios_base::end );
            std::streamoff size = fin.tellg();
            fin.seekg( 0 );

            outstring.resize( size );
            fin.read( outstring.data(), size );
        }
        if( fin.bad() ) {
            throw std::runtime_error( "reading file failed" );
        }

        return std::optional<std::string>( std::move( outstring ) );
    } catch( const std::exception &err ) {
        debugmsg( _( "Failed to read from \"%1$s\": %2$s" ), path.generic_u8string().c_str(), err.what() );
    }
    return std::nullopt;
}

std::optional<std::string> read_whole_file( const cata_path &path )
{
    return read_whole_file( path.get_unrelative_path() );
}

bool read_from_file_json( const cata_path &path,
                          const std::function<void( const JsonValue & )> &reader )
{
    try {
        JsonValue jo = json_loader::from_path( path );
        reader( jo );
        return true;
    } catch( const std::exception &err ) {
        debugmsg( _( "Failed to read from \"%1$s\": %2$s" ), path.generic_u8string().c_str(),
                  err.what() );
        return false;
    }
}

bool read_from_file_optional( const std::string &path,
                              const std::function<void( std::istream & )> &reader )
{
    // Note: slight race condition here, but we'll ignore it. Worst case: the file
    // exists and got removed before reading it -> reading fails with a message
    // Or file does not exists, than everything works fine because it's optional anyway.
    return file_exist( path ) && read_from_file( path, reader );
}

bool read_from_file_optional( const std::filesystem::path &path,
                              const std::function<void( std::istream & )> &reader )
{
    // Note: slight race condition here, but we'll ignore it. Worst case: the file
    // exists and got removed before reading it -> reading fails with a message
    // Or file does not exists, than everything works fine because it's optional anyway.
    return file_exist( path ) && read_from_file( path, reader );
}

bool read_from_file_optional( const cata_path &path,
                              const std::function<void( std::istream & )> &reader )
{
    return read_from_file_optional( path.get_unrelative_path(), reader );
}

bool read_from_file_optional_json( const cata_path &path,
                                   const std::function<void( const JsonValue & )> &reader )
{
    return file_exist( path.get_unrelative_path() ) && read_from_file_json( path, reader );
}

std::string obscure_message( const std::string &str, const std::function<char()> &f )
{
    //~ translators: place some random 1-width characters here in your language if possible, or leave it as is
    std::string gibberish_narrow = _( "abcdefghijklmnopqrstuvwxyz" );
    std::string gibberish_wide =
        //~ translators: place some random 2-width characters here in your language if possible, or leave it as is
        _( "に坂索トし荷測のンおク妙免イロコヤ梅棋厚れ表幌" );
    std::wstring w_gibberish_narrow = utf8_to_wstr( gibberish_narrow );
    std::wstring w_gibberish_wide = utf8_to_wstr( gibberish_wide );
    std::wstring w_str = utf8_to_wstr( str );
    // a trailing NULL terminator is necessary for utf8_width function
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    char transformation[2] = { 0 };
    for( size_t i = 0; i < w_str.size(); ++i ) {
        transformation[0] = f();
        std::string this_char = wstr_to_utf8( std::wstring( 1, w_str[i] ) );
        // mk_wcwidth, which is used by utf8_width, might return -1 for some values, such as newlines 0x0A
        if( transformation[0] == -1 || utf8_width( this_char ) == -1 ) {
            // Leave unchanged
            continue;
        } else if( transformation[0] == 0 ) {
            // Replace with random character
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

void deserialize_wrapper( const std::function<void( const JsonValue & )> &callback,
                          const std::string &data )
{
    JsonValue jsin = json_loader::from_string( data );
    callback( jsin );
}

bool string_empty_or_whitespace( const std::string &s )
{
    if( s.empty() ) {
        return true;
    }

    std::wstring ws = utf8_to_wstr( s );
    return std::all_of( ws.begin(), ws.end(), []( const wchar_t &c ) {
        return std::iswspace( c );
    } );
}

int string_view_cmp( std::string_view l, std::string_view r )
{
    size_t min_len = std::min( l.size(), r.size() );
    int result = memcmp( l.data(), r.data(), min_len );
    if( result ) {
        return result;
    }
    if( l.size() == r.size() ) {
        return 0;
    }
    return l.size() < r.size() ? -1 : 1;
}

template<typename Integer>
Integer svto( std::string_view s )
{
    Integer result = 0;
    const char *end = s.data() + s.size();
    std::from_chars_result r = std::from_chars( s.data(), end, result );
    if( r.ptr != end ) {
        cata_fatal( "could not parse string_view as an integer" );
    }
    return result;
}

template int svto<int>( std::string_view );

std::vector<std::string> string_split( std::string_view string, char delim )
{
    std::vector<std::string> elems;

    if( string.empty() ) {
        return elems; // Well, that was easy.
    }

    std::stringstream ss( std::string{ string } );
    std::string item;
    while( std::getline( ss, item, delim ) ) {
        elems.push_back( item );
    }

    if( string.back() == delim ) {
        elems.emplace_back( "" );
    }

    return elems;
}

template<>
std::string io::enum_to_string<holiday>( holiday data )
{
    switch( data ) {
        // *INDENT-OFF*
        case holiday::none:             return "none";
        case holiday::new_year:         return "new_year";
        case holiday::easter:           return "easter";
        case holiday::independence_day: return "independence_day";
        case holiday::halloween:        return "halloween";
        case holiday::thanksgiving:     return "thanksgiving";
        case holiday::christmas:        return "christmas";
            // *INDENT-ON*
        case holiday::num_holiday:
            break;
    }
    cata_fatal( "Invalid holiday." );
}

/* compare against table of easter dates */
static bool is_easter( int day, int month, int year )
{
    if( month == 3 ) {
        switch( year ) {
            // *INDENT-OFF*
            case 2024: return day == 31;
            case 2027: return day == 28;
            default: break;
            // *INDENT-ON*
        }
    } else if( month == 4 ) {
        switch( year ) {
            // *INDENT-OFF*
            case 2021: return day == 4;
            case 2022: return day == 17;
            case 2023: return day == 9;
            case 2025: return day == 20;
            case 2026: return day == 5;
            case 2028: return day == 16;
            case 2029: return day == 1;
            case 2030: return day == 21;
            default: break;
            // *INDENT-ON*
        }
    }
    return false;
}

holiday get_holiday_from_time( std::time_t time, bool force_refresh )
{
    static holiday cached_holiday = holiday::none;
    static bool is_cached = false;

    if( force_refresh ) {
        is_cached = false;
    }
    if( is_cached ) {
        return cached_holiday;
    }

    is_cached = true;

    bool success = false;

    std::tm local_time;
    std::time_t current_time = time == 0 ? std::time( nullptr ) : time;

    /* necessary to pass LGTM, as threadsafe version of localtime differs by platform */
#if defined(_WIN32)

    errno_t err = localtime_s( &local_time, &current_time );
    if( err == 0 ) {
        success = true;
    }

#else

    success = !!localtime_r( &current_time, &local_time );

#endif

    if( success ) {

        const int month = local_time.tm_mon + 1;
        const int day = local_time.tm_mday;
        const int wday = local_time.tm_wday;
        const int year = local_time.tm_year + 1900;

        /* check date against holidays */
        if( month == 1 && day == 1 ) {
            cached_holiday = holiday::new_year;
            return cached_holiday;
        }
        // only run easter date calculation if currently March or April
        else if( ( month == 3 || month == 4 ) && is_easter( day, month, year ) ) {
            cached_holiday = holiday::easter;
            return cached_holiday;
        } else if( month == 7 && day == 4 ) {
            cached_holiday = holiday::independence_day;
            return cached_holiday;
        }
        // 13 days seems appropriate for Halloween
        else if( month == 10 && day >= 19 ) {
            cached_holiday = holiday::halloween;
            return cached_holiday;
        } else if( month == 11 && ( day >= 22 && day <= 28 ) && wday == 4 ) {
            cached_holiday = holiday::thanksgiving;
            return cached_holiday;
        }
        // For the 12 days of Christmas, my true love gave to me...
        else if( month == 12 && ( day >= 14 && day <= 25 ) ) {
            cached_holiday = holiday::christmas;
            return cached_holiday;
        }
    }
    // fall through to here if localtime fails, or none of the day tests hit
    cached_holiday = holiday::none;
    return cached_holiday;
}

int bucket_index_from_weight_list( const std::vector<int> &weights )
{
    int total_weight = std::accumulate( weights.begin(), weights.end(), int( 0 ) );
    if( total_weight < 1 ) {
        return 0;
    }
    const int roll = rng( 0, total_weight - 1 );
    int index = 0;
    int accum = 0;
    for( int w : weights ) {
        accum += w;
        if( accum > roll ) {
            break;
        }
        index++;
    }
    return index;
}

template<>
std::string io::enum_to_string<aggregate_type>( aggregate_type agg )
{
    switch( agg ) {
        // *INDENT-OFF*
        case aggregate_type::FIRST:   return "first";
        case aggregate_type::LAST:    return "last";
        case aggregate_type::MIN:     return "min";
        case aggregate_type::MAX:     return "max";
        case aggregate_type::SUM:     return "sum";
        case aggregate_type::AVERAGE: return "average";
        // *INDENT-ON*
        case aggregate_type::num_aggregate_types:
            break;
    }
    cata_fatal( "Invalid aggregate type." );
}

std::optional<double> svtod( std::string_view token )
{
    char *pEnd = nullptr;
    double const val = std::strtod( token.data(), &pEnd );
    if( pEnd == token.data() + token.size() ) {
        return { val };
    }
    char block = *pEnd;
    if( block == ',' || block == '.' ) {
        // likely localized with a different locale
        std::string unlocalized( token );
        unlocalized[pEnd - token.data()] = block == ',' ? '.' : ',';
        return svtod( unlocalized );
    }
    debugmsg( R"(Failed to convert string value "%s" to double: %s)", token, std::strerror( errno ) );

    errno = 0;

    return std::nullopt;
}
