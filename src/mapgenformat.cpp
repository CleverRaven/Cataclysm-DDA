#include <iostream>
#include <map>

#include <string>

#include <cassert>
#include <stdarg.h>
#include <algorithm>

#include "output.h"
#include "mapdata.h"
#include "mapgenformat.h"

namespace mapf
{

void formatted_set_simple(map* m, const int startx, const int starty, const char* cstr,
                       internal::format_effect<ter_id> ter_b, internal::format_effect<furn_id> furn_b,
                       const bool empty_toilets)
{
    const char* p = cstr;
    int x = startx;
    int y = starty;
    while (*p != 0) {
        if (*p == '\n') {
            y++;
            x = startx;
        } else {
            const ter_id ter = ter_b.translate( *p );
            const furn_id furn = furn_b.translate( *p );
            if (ter != t_null) {
                m->ter_set(x, y, ter);
            }
            if (furn != f_null) {
                if (furn == f_toilet && !empty_toilets) {
                    m->place_toilet(x, y);
                } else {
                    m->furn_set(x, y, furn);
                }
            }
            x++;
        }
        p++;
    }
}

template<typename ID>
internal::format_effect<ID> basic_bind( std::string &characters, va_list args )
{
    characters.erase( std::remove_if(characters.begin(), characters.end(), isspace), characters.end());
    std::vector<ID> determiners;
    va_list vl;
    va_copy( vl, args );
    determiners.resize(characters.size());
    for( size_t i = 0; i < characters.size(); ++i ) {
        determiners[i] = ID( va_arg(vl,int) );
    }
    va_end(vl);
    return internal::format_effect<ID>( characters, determiners );
}

internal::format_effect<ter_id> ter_bind( std::string characters, ... )
{
    va_list ap;
    va_start( ap, characters );
    auto result = basic_bind<ter_id>( characters, ap );
    va_end( ap );
    return result;
}

internal::format_effect<furn_id> furn_bind(std::string characters, ...)
{
    va_list ap;
    va_start( ap, characters );
    auto result = basic_bind<furn_id>( characters, ap );
    va_end( ap );
    return result;
}

namespace internal
{
    template<typename ID>
    format_effect<ID>::format_effect(std::string characters, std::vector<ID> &determiners)
        : characters( characters ), determiners( determiners )
    {
    }

    template<typename ID>
    ID format_effect<ID>::translate( const char c ) const
    {
        const auto index = characters.find( c );
        if( index == std::string::npos ) {
            return ID( 0 );
        }
        return determiners[index];
    }

    template class format_effect<furn_id>;
    template class format_effect<ter_id>;
}

}//END NAMESPACE mapf
