#include "map_selector.h"

#include "map.h"

map_selector::map_selector( map& m, const tripoint& pos, int radius, bool accessible ) {
	data = closest_tripoints_first( radius, pos );
	if( accessible ) {
		data.erase( std::remove_if( data.begin(), data.end(), [&m,&pos,&radius,&accessible]( const tripoint& e ) {
			return !m.clear_path( pos, e, radius, 1, 100 );
		} ), data.end() );
	}
}
