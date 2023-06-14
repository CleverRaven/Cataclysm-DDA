#include "math_parser_func.h"

#include <exception>
#include <string_view>
#include <vector>

std::vector<std::string_view> tokenize( std::string_view str, std::string_view separators,
                                        bool include_seps )
{
    std::vector<std::string_view> ret;
    std::string_view::size_type start = 0;
    std::string_view::size_type pos = 0;

    while( pos != std::string_view::npos ) {
        pos = str.find_first_of( separators, start );
        if( pos != start ) {
            std::string_view const ts = str.substr( start, pos - start );
            if( std::string_view::size_type const unpad = ts.find_first_not_of( ' ' );
                unpad != std::string_view::npos ) {
                std::string_view::size_type const unpad_e = ts.find_last_not_of( ' ' );
                ret.emplace_back(
                    ts.substr( unpad, unpad_e - unpad + 1 ) );
            }
        }
        if( pos != std::string_view::npos && include_seps ) {
            ret.emplace_back( &str[pos], 1 );
        }
        start = pos + 1;
    }
    return ret;
}
