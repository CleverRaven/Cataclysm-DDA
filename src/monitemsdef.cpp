#include "game.h"
#include "json.h"

void game::load_monitem(JsonObject &jo) {
    std::vector<std::string> tmp_keys;
    std::vector<items_location_and_chance> tmp_items;
    std::string mkey="";

    if ( jo.has_string("id") ) {
        tmp_keys.push_back( jo.get_string("id") );
    } else if ( jo.has_array("id") ) {
        jo.read("id", tmp_keys);
    } else {
        jo.throw_error("monitems: requires \"id\": \"monster_id\" or \"id\": [ \"multiple\", \"monster_ids\" ]");
    }

    if ( ! jo.has_array("item_groups") ) {
        jo.throw_error("monitems: requires \"item_groups\": [ [ \"group_one\", (chance) ], ... ]");
    }

    JsonArray ja = jo.get_array("item_groups");
    JsonArray ga;
    while ( ja.has_more() ) {
        ga = ja.next_array();
        if ( ! ga.has_string(0) || ! ga.has_number(1) ) {
             jo.throw_error("monitems: item_groups must contain arrays of [ \"string\", number ]");
        }
        tmp_items.push_back( items_location_and_chance( ga.get_string(0), ga.get_int(1) ) );
    }

    for( size_t i = 0; i < tmp_keys.size(); ++i ) {
        std::vector <items_location_and_chance> &itvec = monitems[tmp_keys[i]];
        itvec.insert( itvec.end(), tmp_items.begin(), tmp_items.end() );
    }
}

void game::reset_monitems()
{
    monitems.clear();
}
