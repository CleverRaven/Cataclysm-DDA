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
#include <windows.h>

static void erase_char( std::string &s, const char &c )
{
    s.erase( std::remove( s.begin(), s.end(), c ), s.end() );
}
#endif

static bool enable_stdout_ansi_colors()
{
#if defined(_WIN32)
    // enable ANSI colors on windows consoles https://superuser.com/a/1529908
    DWORD dwMode;
    GetConsoleMode( GetStdHandle( STD_OUTPUT_HANDLE ), &dwMode );
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    // may fail on Windows 10 earlier than 1511
    return SetConsoleMode( GetStdHandle( STD_OUTPUT_HANDLE ), dwMode );
#else
    return true;
#endif
}

int main( int argc, char *argv[] )
{
#if defined(_MSC_VER)
    bool supports_color = _isatty( _fileno( stdout ) );
#else
    bool supports_color = isatty( STDOUT_FILENO );
#endif
    // formatter stdout in github actions is redirected but still able to handle ANSI colors
    supports_color |= std::getenv( "CI" ) != nullptr;

    // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
    json_error_output_colors = supports_color
                               ? json_error_output_colors_t::ansi_escapes
                               : json_error_output_colors_t::no_colors;

    if( !enable_stdout_ansi_colors() ) {
        json_error_output_colors = json_error_output_colors_t::no_colors;
    }

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
    TextJsonIn jsin( in, filename.empty() ? "<STDIN>" : filename );

    try {
        formatter::format( jsin, jsout );
    } catch( const JsonError &e ) {
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

        std::string color_bad = supports_color ? "\x1b[31m" : std::string();
        std::string color_end = supports_color ? "\x1b[0m" : std::string();
        if( in_str == out.str() ) {
            exit( EXIT_SUCCESS );
        } else {
            std::ofstream fout( filename, std::ios::binary | std::ios::trunc );
            fout << out.str();
            fout.close();
            std::cout << color_bad << "Has been linted : " << color_end << filename << std::endl;
            std::cout << "Please read doc/JSON_STYLE.md" << std::endl;
            exit( EXIT_FAILURE );
        }
    }
}
