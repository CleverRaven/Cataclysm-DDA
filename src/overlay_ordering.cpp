#include "overlay_ordering.h"

std::map<std::string, int> base_mutation_overlay_ordering;
std::map<std::string, int> tileset_mutation_overlay_ordering;

void load_overlay_ordering( JsonObject &jsobj )
{
    JsonArray jarr = jsobj.get_array( "ordering" );
    while( jarr.has_more() ) {
        JsonObject ordering = jarr.next_object();

        JsonArray jsarr;
        int order = ordering.get_int( "order" );
        if( ordering.has_array( "id" ) ) {
            jsarr = ordering.get_array( "id" );
            while( jsarr.has_more() ) {
                std::string mut_id = jsarr.next_string();
                base_mutation_overlay_ordering[mut_id] = order;
            }
        } else {
            std::string mut_id = ordering.get_string( "id" );
            base_mutation_overlay_ordering[mut_id] = order;
        }
    }
}
