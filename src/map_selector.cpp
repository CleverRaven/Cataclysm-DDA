#include "map_selector.h"

#include "game.h"
#include "map.h"

map_selector::map_selector( const tripoint &pos, int radius, bool accessible )
{
    for( const auto &e : closest_tripoints_first( radius, pos ) ) {
        if( !accessible || g->m.clear_path( pos, e, radius, 1, 100 ) ) {
            data.emplace_back( e );
        }
    }
}
