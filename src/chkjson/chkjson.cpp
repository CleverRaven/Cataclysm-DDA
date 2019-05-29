/* Main Loop for cataclysm
 * Linux only I guess
 * But maybe not
 * Who knows
 */
#include <sys/stat.h>
#include <dirent.h>
#include <clocale>
#include <cstdio>
#include <cstring>  // for strcmp
#include <stack>    // for stack (obviously)
#include <string>
#include <vector>
#include <fstream>
#include <sstream> // for throwing errors
#include <exception>
#include <iterator>
#include <stdexcept>

#include "json.h"

// copypasta: file_finder.cpp
static std::vector<std::string> get_files_from_path( std::string extension, std::string root_path,
        bool recursive_search, bool match_extension )
{
    std::vector<std::string> files;
    const size_t extsz = extension.size();
    const char *c_extension = extension.c_str();
    // test for empty root path
    if( root_path.empty() ) {
        root_path = ".";
    }

    std::stack<std::string> directories, tempstack;
    directories.push( root_path );
    std::string path;

    while( !directories.empty() ) {
        path = directories.top();
        directories.pop();

        DIR *root = opendir( path.c_str() );

        if( root ) {
            struct dirent *root_file;
            struct stat _buff;
            DIR *subdir;

            while( ( root_file = readdir( root ) ) ) {
                // check to see if it is a folder!
                if( stat( root_file->d_name, &_buff ) != 0x4 ) {
                    // ignore '.' and '..' folder names, which are current and parent folder relative paths
                    if( ( strcmp( root_file->d_name, "." ) != 0 ) && ( strcmp( root_file->d_name, ".." ) != 0 ) ) {
                        std::string subpath = path + "/" + root_file->d_name;

                        if( recursive_search ) {
                            subdir = opendir( subpath.c_str() );
                            if( subdir ) {
                                tempstack.push( subpath );
                                closedir( subdir );
                            }
                        }
                    }
                }
                // check to see if it is a file with the appropriate extension
                std::string tmp = root_file->d_name;
                if( tmp.find( c_extension, match_extension ? tmp.size() - extsz : 0 ) != std::string::npos ) {
                    std::string fullpath = path + "/" + tmp;
                    files.push_back( fullpath );
                }
            }
            closedir( root );
        }
        // Directories are added to tempstack in A->Z order, which makes them pop from Z->A. This Makes sure that directories are
        // searched in the proper order and that the final output is in the proper order.
        while( !tempstack.empty() ) {
            directories.push( tempstack.top() );
            tempstack.pop();
        }
    }
    return files;
}

// copypasta: init.cpp
static void load_object( JsonObject &jo )
{
    std::string type = jo.get_string( "type" );
    if( ! jo.has_string( "type" ) ) {
        jo.throw_error( "JSON object has no type" );
    }
}
static void load_all_from_json( JsonIn &jsin )
{
    char ch;
    jsin.eat_whitespace();
    // examine first non-whitespace char
    ch = jsin.peek();
    if( ch == '{' ) {
        // find type and dispatch single object
        JsonObject jo = jsin.get_object();
        load_object( jo );
        jo.finish();
        // if there's anything else in the file, it's an error.
        jsin.eat_whitespace();
        if( jsin.good() ) {
            std::stringstream err;
            err << "expected single-object file but found '";
            err << jsin.peek() << "'";
            jsin.error( err.str() );
        }
    } else if( ch == '[' ) {
        jsin.start_array();
        // find type and dispatch each object until array close
        while( !jsin.end_array() ) {
            jsin.eat_whitespace();
            ch = jsin.peek();
            if( ch != '{' ) {
                std::stringstream err;
                err << "expected array of objects but found '";
                err << ch << "', not '{'";
                jsin.error( err.str() );
            }
            JsonObject jo = jsin.get_object();
            load_object( jo );
            jo.finish();
        }
    } else {
        // not an object or an array?
        std::stringstream err;
        err << "expected object or array, but found '" << ch << "'";
        jsin.error( err.str() );
    }
}

static void load_json_dir( const std::string &dirname )
{
    // get a list of all files in the directory
    std::vector<std::string> dir =
        get_files_from_path( ".json", dirname, true, true );
    // iterate over each file
    std::vector<std::string>::iterator it;
    for( it = dir.begin(); it != dir.end(); it++ ) {
        // open the file as a stream
        std::ifstream infile( it->c_str(), std::ifstream::in | std::ifstream::binary );
        // and stuff it into ram
        std::istringstream iss(
            std::string(
                ( std::istreambuf_iterator<char>( infile ) ),
                std::istreambuf_iterator<char>()
            )
        );
        infile.close();
        // parse it
        try {
            JsonIn jsin( iss );
            load_all_from_json( jsin );
        } catch( const JsonError &err ) {
            throw std::runtime_error( *( it ) + ": " + err.what() );
        }
    }
}

int main( int, char ** )
{
    setlocale( LC_ALL, "" );
    try {
        load_json_dir( "data/json" );
    } catch( const std::exception &err ) {
        printf( "Error: %s\n", err.what() );
        return 1;
    }
    return 0;
}
