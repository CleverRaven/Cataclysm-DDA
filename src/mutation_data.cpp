#include "mutation.h"
#include "json.h"
#include "pldata.h" // traits
#include "enums.h" // tripoint
#include "bodypart.h"

#include <vector>
#include <map>

std::map<std::string, mutation_type> mut_types;
std::vector<dream> dreams;

static void extract_mod(JsonObject &j, std::unordered_map<std::pair<bool, std::string>, int> &data,
                        std::string mod_type, bool active, std::string type_key)
{
    int val = j.get_int(mod_type, 0);
    if (val != 0) {
        data[std::make_pair(active, type_key)] = val;
    }
}

bool mutation_type::load_mod_data(JsonObject &jsobj, std::string member)
{
    if (jsobj.has_object(member)) {
        JsonObject j = jsobj.get_object(member);
        bool active = false;
        if (member == "active_mods") {
            active = true;
        }
        //                   json field            type key
        extract_mod(j, mods, "str_mod",     active, "STR");
        extract_mod(j, mods, "dex_mod",     active, "DEX");
        extract_mod(j, mods, "per_mod",     active, "PER");
        extract_mod(j, mods, "int_mod",     active, "INT");
        
        return true;
    } else {
        return false;
    }
}

static void extract_cost(JsonObject &j, std::unordered_map<std::string, int> &data,
                         std::string member)
{
    int tmp = j.get_int(member, 0);
    if (tmp != 0) {
        data[member] = tmp;
    }
}

bool mutation_type::load_cost_data(JsonObject &jsobj, std::string member)
{
    if (jsobj.has_object(member)) {
        JsonObject j = jsobj.get_object(member);
        extract_cost(j, costs, "fatigue");
        extract_cost(j, costs, "hunger");
        extract_cost(j, costs, "thirst");
        extract_cost(j, costs, "pain");
        extract_cost(j, costs, "radiation");
        
        return true;
    } else {
        return false;
    }
}

void load_mutation_type(JsonObject &js)
{
    mutation_type new_mut;
    new_mut.id = jo.get_string("id");
    new_mut.name = _(jsobj.get_string("name").c_str());
    new_mut.description = _(jsobj.get_string("description").c_str());
    
    // Mutation rating is based on point cost if it doesn't exist
    new_mut.points = jo.get_int("points");
    if(jo.has_member("rating")) {
        std::string r = jo.get_string("rating");
        if(r == "good") {
            new_mut.rating = mut_good;
        } else if(r == "neutral" ) {
            new_mut.rating = mut_neutral;
        } else if(r == "bad" ) {
            new_mut.rating = mut_bad;
        } else if(r == "mixed" ) {
            new_mut.rating = mut_mixed;
        } else {
            new_mut.rating = mut_neutral;
        }
    } else {
        if (new_mut.points > 0) {
            new_mut.rating = mut_good;
        } else if (new_mut.points < 0) {
            new_mut.rating = mut_bad;
        } else {
            new_mut.rating = mut_neutral;
        }
    }
    
    new_mut.visibility = jo.get_int("visibility", 0);
    new_mut.ugliness = jo.get_int("ugliness", 0);
    new_mut.valid = jo.get_bool("valid", true);
    new_mut.purifiable = jo.get_bool("purifiable", true);
    new_mut.threshold = jo.get_bool("threshold", false);
    new_mut.profession = jo.get_bool("profession", false);
    new_mut.starting_trait = jo.get_bool("starting_trait", false);
    new_mut.initial_ma_styles = jo.get_string_array( "initial_ma_styles" );
    
    // New mutations default to passive. Active muts have two types of activation
    std::string act_type = jo.get_string("activated", "passive");
    if (act_type == "repeating") {
        new_mut.activatable = true;
        new_mut.repeating = true;
    } else if (act_type == "once") {
        new_mut.activatable = true;
        new_mut.repeating = false;
    } else {
        new_mut.activatable = false;
        new_mut.repeating = false;
    }
    new_mut.duration = jo.get_int("duration", 0);

    jsarr = jo.get_array("prereqs");
    while (jsarr.has_more()) {
        new_mut.prereqs.push_back(jsarr.next_string());
    }
    // Helps to be able to have a trait require more than one other trait
    // (Individual prereq-lists are "OR", not "AND".)
    // Traits should NOT appear in both lists for a given mutation, unless
    // you want that trait to satisfy both requirements.
    // These are additional to the first list.
    jsarr = jo.get_array("prereqs2");
    while (jsarr.has_more()) {
        new_mut.prereqs2.push_back(jsarr.next_string());
    }
    // Dedicated-purpose prereq slot for Threshold mutations
    jsarr = jo.get_array("threshreq");
    // Stuff like Huge might fit in more than one mutcat post-threshold, so yeah
    while (jsarr.has_more()) {
        new_mut.threshreq.push_back(jsarr.next_string());
    }
    jsarr = jo.get_array("cancels");
    while (jsarr.has_more()) {
        new_mut.cancels.push_back(jsarr.next_string());
    }
    jsarr = jo.get_array("changes_to");
    while (jsarr.has_more()) {
        new_mut.replacements.push_back(jsarr.next_string());
    }
    jsarr = jo.get_array("leads_to");
    while (jsarr.has_more()) {
        new_mut.additions.push_back(jsarr.next_string());
    }
    jsarr = jo.get_array("category");
    while (jsarr.has_more()) {
        std::string s = jsarr.next_string();
        new_mut.category.push_back(s);
    }
    jsarr = jo.get_array("wet_protection");
    while (jsarr.has_more()) {
        JsonObject jsobj = jsarr.next_object();
        std::string part_id = jsobj.get_string("part");
        int ignored = jsobj.get_int("ignored", 0);
        int neutral = jsobj.get_int("neutral", 0);
        int good = jsobj.get_int("good", 0);
        tripoint protect = tripoint(ignored, neutral, good);
        new_mut.protection[part_id] =
            mutation_wet(body_parts[part_id], protect);
    }
    
    new_mut.load_mod_data(jo, "passive_mods");
    new_mut.load_mod_data(jo, "active_mods");
    new_mut.load_cost_data(jo, "costs");
    
    mut_types[new_mut.id] = new_mut;
}

void finalize_mut_types()
{
    for (auto &i : mut_types) {
        for (auto &j : i.second.replacements) {
            mut_types[j].replaces.push_back(i.first);
        }
    }
}

void load_dream(JsonObject &jsobj)
{
    dream newdream;

    newdream.strength = jsobj.get_int("strength");
    newdream.category = jsobj.get_string("category");

    JsonArray jsarr = jsobj.get_array("messages");
    while (jsarr.has_more()) {
        newdream.messages.push_back(_(jsarr.next_string().c_str()));
    }

    dreams.push_back(newdream);
}

