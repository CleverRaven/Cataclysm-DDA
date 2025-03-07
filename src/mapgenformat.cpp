#include "mapgenformat.h"

#include <algorithm>
#include <cctype>

#include "coordinates.h"
#include "map.h"
#include "point.h"

static const furn_str_id furn_f_toilet( "f_toilet" );

namespace mapf
{

void formatted_set_simple( map *m, const point_bub_ms &start, const char *cstr,
                           const format_effect<ter_id> &ter_b, const format_effect<furn_id> &furn_b )
{
    const char *p = cstr;
    point_bub_ms p2( start );
    while( *p != 0 ) {
        if( *p == '\n' ) {
            p2.y()++;
            p2.x() = start.x();
        } else {
            const ter_id &ter = ter_b.translate( *p );
            const furn_id &furn = furn_b.translate( *p );
            if( ter != ter_str_id::NULL_ID() ) {
                m->ter_set( p2, ter );
            }
            if( furn != furn_str_id::NULL_ID() ) {
                if( furn == furn_f_toilet ) {
                    m->place_toilet( { p2, m->get_abs_sub().z() } );
                } else {
                    m->furn_set( p2, furn );
                }
            }
            p2.x()++;
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
