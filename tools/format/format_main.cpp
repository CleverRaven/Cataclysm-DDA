#include "format.h"
#include "json.h"

#include "getpost.h"

#if defined(_MSC_VER)
#include <io.h>
#else
#include <unistd.h>
#endif
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#if defined(_WIN32)
static void erase_char( std::string &s, const char &c )
{
    s.erase( std::remove( s.begin(), s.end(), c ), s.end() );
}
#endif

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

    if( in.str().empty() ) {
        std::cout << "Error, " << ( filename.empty() ? "input" : filename ) << " is empty." << std::endl;
        exit( EXIT_FAILURE );
    }
    JsonOut jsout( out, true );
    TextJsonIn jsin( in );

    try {
        formatter::format( jsin, jsout );
    } catch( const JsonError &e ) {
        std::cout << "JSON error in " << ( filename.empty() ? "input" : filename ) << std::endl;
        std::cout << e.what() << std::endl;
        exit( EXIT_FAILURE );
    }

    out << std::endl;

    if( filename.empty() ) {
        std::cout << header;
        std::cout << out.str();
    } else {
        std::string in_str = in.str();
#if defined(_WIN32)
        erase_char( in_str, '\r' );
#endif

#if defined(_MSC_VER)
        bool supports_color = _isatty( _fileno( stdout ) );
#else
        bool supports_color = isatty( STDOUT_FILENO );
#endif
        std::string color_bad = supports_color ? "\x1b[31m" : std::string();
        std::string color_end = supports_color ? "\x1b[0m" : std::string();
        if( in_str == out.str() ) {
            exit( EXIT_SUCCESS );
        } else {
            std::ofstream fout( filename, std::ios::binary | std::ios::trunc );
            fout << out.str();
            fout.close();
            std::cout << color_bad << "Needs linting : " << color_end << filename << std::endl;
            std::cout << "Please read doc/JSON_STYLE.md" << std::endl;
            exit( EXIT_FAILURE );
        }
    }
}
