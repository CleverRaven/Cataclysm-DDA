#include "map_selector.h"

#include "map.h"

map_selector::map_selector( map& m, const tripoint& pos, int radius, bool accessible ) {
	for( const auto& e : closest_tripoints_first( radius, pos ) ) {
		if( !accessible || m.clear_path( pos, e, radius, 1, 100 ) ) {
			data.emplace_back( pos );
		}
	}
}
