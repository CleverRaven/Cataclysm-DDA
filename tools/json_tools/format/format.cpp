#include "json.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

static void format( JsonIn &jsin, JsonOut &jsout )
{
    if( jsin.test_array() ) {
        jsout.start_array();
        jsin.start_array();
        while( !jsin.end_array() ) {
            format( jsin, jsout );
        }
        jsout.end_array();
    } else if( jsin.test_object() ) {
        jsout.start_object();
        jsin.start_object();
        while( !jsin.end_object() ) {
            std::string name = jsin.get_member_name();
            jsout.member( name );
            format( jsin, jsout );
        }
        jsout.end_object();
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
            jsout.need_seperator();
        }
        jsin.seek( end_pos );
    } else if( jsin.test_bool() ) {
        bool tf = jsin.get_bool();
        jsout.write( tf );
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
        in << fin.rdbuf();
    }

    JsonOut jsout( out, true );
    JsonIn jsin( in );

    format( jsin, jsout );

    out << std::endl;

    if( filename.empty() ) {
        std::cout << out.str() << std::endl;
    } else {
        if( in.str() == out.str() ) {
            std::cout << "Unformatted " << filename << std::endl;
        } else {
            std::ofstream fout( filename, std::ios::binary | std::ios::trunc );
            fout << out.str();
            std::cout << "Formatted " << filename << std::endl;
        }
    }
}
