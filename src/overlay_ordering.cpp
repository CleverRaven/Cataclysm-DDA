#include "overlay_ordering.h"

std::map<std::string, int> base_mutation_overlay_ordering;
std::map<std::string, int> tileset_mutation_overlay_ordering;

void load_overlay_ordering( JsonObject &jsobj )
{
    load_overlay_ordering_into_array( jsobj, base_mutation_overlay_ordering );
}

void load_overlay_ordering_into_array( JsonObject &jsobj, std::map<std::string, int> &orderarray )
{
    JsonArray jarr = jsobj.get_array( "overlay_ordering" );
    while( jarr.has_more() ) {
        JsonObject ordering = jarr.next_object();
        int order = ordering.get_int( "order" );
        for( auto &id : ordering.get_tags( "id" ) ) {
            orderarray[id] = order;
        }
    }
}

void reset_overlay_ordering()
{
    // tileset specific overlays are cleared on new tileset load
    base_mutation_overlay_ordering.clear();
}
