#include <iostream>

#include <string>

#include <cassert>
#include <stdarg.h>
#include <algorithm>

#include "output.h"
#include "mapdata.h"
#include "map.h"
#include "mapgenformat.h"

namespace mapf
{

void formatted_set_simple( map *m, const int startx, const int starty, const char *cstr,
                           format_effect<ter_id> ter_b, format_effect<furn_id> furn_b )
{
    const char *p = cstr;
    int x = startx;
    int y = starty;
    while( *p != 0 ) {
        if( *p == '\n' ) {
            y++;
            x = startx;
        } else {
            const ter_id ter = ter_b.translate( *p );
            const furn_id furn = furn_b.translate( *p );
            if( ter != t_null ) {
                m->ter_set( x, y, ter );
            }
            if( furn != f_null ) {
                if( furn == f_toilet ) {
                    m->place_toilet( x, y );
                } else {
                    m->furn_set( x, y, furn );
                }
            }
            x++;
        }
        p++;
    }
}

template<typename ID>
format_effect<ID>::format_effect( std::string chars, std::vector<ID> dets )
    : characters( chars ), determiners( dets )
{
    characters.erase( std::remove_if( characters.begin(), characters.end(), isspace ),
                      characters.end() );
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

}//END NAMESPACE mapf
