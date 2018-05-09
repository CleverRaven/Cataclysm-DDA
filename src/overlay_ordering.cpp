#include "overlay_ordering.h"

#include "json.h"

std::map<trait_id, int> base_mutation_overlay_ordering;
std::map<trait_id, int> tileset_mutation_overlay_ordering;

void load_overlay_ordering( JsonObject &jsobj )
{
    load_overlay_ordering_into_array( jsobj, base_mutation_overlay_ordering );
}

void load_overlay_ordering_into_array( JsonObject &jsobj, std::map<trait_id, int> &orderarray )
{
    JsonArray jarr = jsobj.get_array( "overlay_ordering" );
    while( jarr.has_more() ) {
        JsonObject ordering = jarr.next_object();
        int order = ordering.get_int( "order" );
        for( auto &id : ordering.get_tags( "id" ) ) {
            orderarray[trait_id( id )] = order;
        }
    }
}

void reset_overlay_ordering()
{
    // tileset specific overlays are cleared on new tileset load
    base_mutation_overlay_ordering.clear();
}
