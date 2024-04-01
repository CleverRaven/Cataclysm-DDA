#include "mapgenformat.h"

#include <cctype>

#include "map.h"
#include "mapdata.h"
#include "point.h"

namespace mapf
{

void formatted_set_simple( map *m, const point &start, const char *cstr,
                           const format_effect<ter_id> &ter_b, const format_effect<furn_id> &furn_b )
{
    const char *p = cstr;
    point p2( start );
    while( *p != 0 ) {
        if( *p == '\n' ) {
            p2.y++;
            p2.x = start.x;
        } else {
            const ter_id ter = ter_b.translate( *p );
            const furn_id furn = furn_b.translate( *p );
            if( ter != t_null ) {
                m->ter_set( p2, ter );
            }
            if( furn != f_null ) {
                if( furn == f_toilet ) {
                    m->place_toilet( p2 );
                } else {
                    m->furn_set( p2, furn );
                }
            }
            p2.x++;
        }
        p++;
    }
}

template<typename ID>
format_effect<ID>::format_effect( const std::string &chars, std::vector<ID> dets )
    : characters( chars ), determiners( dets )
{
    characters.erase( std::remove_if( characters.begin(), characters.end(), isspace ),
                      characters.end() );
}

template<typename ID>
ID format_effect<ID>::translate( const char c ) const
{
    const size_t index = characters.find( c );
    if( index == std::string::npos ) {
        return ID( 0 );
    }
    return determiners[index];
}

template class format_effect<furn_id>;
template class format_effect<ter_id>;

}//END NAMESPACE mapf
