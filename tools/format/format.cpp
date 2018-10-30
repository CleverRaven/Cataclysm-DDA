#include "json.h"

#include "getpost.h"

#include <cstdlib>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <string>

static void format( JsonIn &jsin, JsonOut &jsout, int depth = -1 );

#ifdef MSYS2
static void erase_char( std::string &s, const char &c )
{
    size_t pos = std::string::npos;
    while( ( pos  = s.find( c ) ) != std::string::npos )
    {
        s.erase( pos, 1 );
    }
}
#endif

static void write_array( JsonIn &jsin, JsonOut &jsout, int depth, bool force_wrap )
{
    jsout.start_array( force_wrap );
    jsin.start_array();
    while( !jsin.end_array() ) {
        format( jsin, jsout, depth );
    }
    jsout.end_array();
}

static void write_object( JsonIn &jsin, JsonOut &jsout, int depth, bool force_wrap )
{
    jsout.start_object( force_wrap );
    jsin.start_object();
    while( !jsin.end_object() ) {
        std::string name = jsin.get_member_name();
        jsout.member( name );
        format( jsin, jsout, depth );
    }
    jsout.end_object();
}

static void format_collection( JsonIn &jsin, JsonOut &jsout, int depth,
                               std::function<void(JsonIn &, JsonOut &, int, bool )>write_func )
{
    if( depth > 1 ) {
        // We're backtracking by storing jsin and jsout state before formatting
        // and restoring it afterwards if necessary.
        int in_start_pos = jsin.tell();
        bool ate_seperator = jsin.get_ate_separator();
        int out_start_pos = jsout.tell();
        bool need_separator = jsout.get_need_separator();
        write_func( jsin, jsout, depth, false );
        if( jsout.tell() - out_start_pos <= 120 ) {
            // Line is short enough, so we're done.
            return;
        } else {
            // Reset jsin and jsout to their initial state,
            // and we'll serialize while forcing wrapping.
            jsin.seek( in_start_pos );
            jsin.set_ate_separator( ate_seperator );
            jsout.seek( out_start_pos );
            if( need_separator ) {
                jsout.set_need_separator();
            }
        }
    }
    write_func( jsin, jsout, depth, true );
}

static void format( JsonIn &jsin, JsonOut &jsout, int depth )
{
    depth++;
    if( jsin.test_array() ) {
        format_collection( jsin, jsout, depth, write_array );
    } else if( jsin.test_object() ) {
        format_collection( jsin, jsout, depth, write_object );
    } else if( jsin.test_string() ) {
        std::string str = jsin.get_string();
        jsout.write( str );
    } else if( jsin.test_number() ) {
        // Have to introspect into the string to distinguish integers from floats.
        // Otherwise they won't serialize correctly.
        const int start_pos = jsin.tell();
        double num = jsin.get_float();
        const int end_pos = jsin.tell();
        std::string str_form = jsin.substr( start_pos, end_pos - start_pos );
        if( str_form.find( '.' ) == std::string::npos ) {
            jsout.write( static_cast<long>( num ) );
        } else {
            // This is QUITE insane, but as far as I can tell there is NO way to configure
            // an ostream to output a float/double meeting two constraints:
            // Always emit a decimal point (and single trailing 0 after a bare decimal point).
            // Don't emit trailing zeroes otherwise.
            std::string double_str = std::to_string( num );
            double_str.erase( double_str.find_last_not_of( '0' ) + 1, std::string::npos );
            if( double_str.back() == '.' ) {
                double_str += "0";
            }
            jsout.write_separator();
            *jsout.get_stream() << double_str;
            jsout.set_need_separator();
        }
        jsin.seek( end_pos );
    } else if( jsin.test_bool() ) {
        bool tf = jsin.get_bool();
        jsout.write( tf );
    } else if( jsin.test_null() ) {
        jsout.write_null();
    } else {
        std::cerr << "Encountered unrecognized json element \"";
        const int start_pos = jsin.tell();
        jsin.skip_value();
        const int end_pos = jsin.tell();
        for( int i = start_pos; i < end_pos; ++i ) {
            jsin.seek( i );
            std::cerr << jsin.peek();
        }
        std::cerr << "\"" << std::endl;
    }
}

int main( int argc, char *argv[] )
{
    std::stringstream in;
    std::stringstream out;
    std::string filename;
    std::string header;

    char *gateway_var = getenv( "GATEWAY_INTERFACE" );
    if( gateway_var == nullptr ) {
        // Expect a single filename for now.
        if( argc == 2 ) {
            filename = argv[1];
        } else if( argc != 1 ) {
            std::cout << "Supply a filename to style or no arguments." << std::endl;
            exit( EXIT_FAILURE );
        }

        if( filename.empty() ) {
            in << std::cin.rdbuf();
        } else {
            std::ifstream fin( filename, std::ios::binary );
            if( !fin.good() ) {
                std::cout << "Failed to open " << filename << std::endl;
                exit( EXIT_FAILURE );
            }
            in << fin.rdbuf();
            fin.close();
        }
    } else {
        std::map<std::string, std::string> params;
        initializePost( params );
        std::string data = params[ "data" ];
        if( data.empty() ) {
            exit( -255 );
        }
        in.str( data );
        header = "Content-type: application/json\n\n";
    }

    if( in.str().size() == 0 ) {
        std::cout << "Error, input empty." << std::endl;
        exit( EXIT_FAILURE );
    }
    JsonOut jsout( out, true );
    JsonIn jsin( in );

    format( jsin, jsout );

    out << std::endl;

    if( filename.empty() ) {
        std::cout << header;
        std::cout << out.str();
    } else {
        std::string in_str = in.str();
#ifdef MSYS2
        erase_char( in_str, '\r' );
#endif
        if( in_str == out.str() ) {
            std::cout << "Unformatted " << filename << std::endl;
            exit( EXIT_SUCCESS );
        } else {
            std::ofstream fout( filename, std::ios::binary | std::ios::trunc );
            fout << out.str();
            fout.close();
            std::cout << "Formatted " << filename << std::endl;
            exit( EXIT_FAILURE );
        }
    }
}
