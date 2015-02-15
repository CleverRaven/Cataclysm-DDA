#include "mutation.h"
#include "json.h"
#include "pldata.h" // traits
#include "enums.h" // tripoint
#include "bodypart.h"

#include <vector>
#include <map>

std::vector<dream> dreams;
std::map<std::string, std::vector<std::string> > mutations_category;
std::map<std::string, mutation_branch> mutation_data;

static void extract_mod(JsonObject &j, std::unordered_map<std::pair<bool, std::string>, int> &data,
                        std::string mod_type, bool active, std::string type_key)
{
    int val = j.get_int(mod_type, 0);
    if (val != 0) {
        data[std::make_pair(active, type_key)] = val;
    }
}

static void load_mutation_mods(JsonObject &jsobj, std::string member, std::unordered_map<std::pair<bool, std::string>, int> &mods)
{
    if (jsobj.has_object(member)) {
        JsonObject j = jsobj.get_object(member);
        bool active = false;
        if (member == "active_mods") {
            active = true;
        }
        //                   json field             type key
        extract_mod(j, mods, "str_mod",     active, "STR");
        extract_mod(j, mods, "dex_mod",     active, "DEX");
        extract_mod(j, mods, "per_mod",     active, "PER");
        extract_mod(j, mods, "int_mod",     active, "INT");
    }   
}

void load_mutation(JsonObject &jsobj)
{
    trait new_trait;
    JsonArray jsarr;
    std::string id = jsobj.get_string("id");
    new_trait.id = id;
    new_trait.name = _(jsobj.get_string("name").c_str());
    new_trait.description = _(jsobj.get_string("description").c_str());
    new_trait.points = jsobj.get_int("points");
    new_trait.visibility = jsobj.get_int("visibility", 0);
    new_trait.ugliness = jsobj.get_int("ugliness", 0);
    new_trait.startingtrait = jsobj.get_bool("starting_trait", false);
    new_trait.mixed_effect = jsobj.get_bool("mixed_effect", false);
    new_trait.activated = jsobj.get_bool("active", false);
    new_trait.cost = jsobj.get_int("cost", 0);
    new_trait.cooldown = jsobj.get_int("time",0);
    new_trait.hunger = jsobj.get_bool("hunger",false);
    new_trait.thirst = jsobj.get_bool("thirst",false);
    new_trait.fatigue = jsobj.get_bool("fatigue",false);
    new_trait.charge = 0;


    traits[id] = new_trait;

    mutation_data[id].valid = jsobj.get_bool("valid", true);
    mutation_data[id].purifiable = jsobj.get_bool("purifiable", true);
    mutation_data[id].initial_ma_styles = jsobj.get_string_array( "initial_ma_styles" );
    mutation_data[id].threshold = jsobj.get_bool("threshold", false);
    mutation_data[id].profession = jsobj.get_bool("profession", false);
    
    load_mutation_mods(jsobj, "passive_mods", mutation_data[id].mods);
    /* Not currently supported due to inability to save active mutation state
    load_mutation_mods(jsobj, "active_mods", mutation_data[id].mods); */

    jsarr = jsobj.get_array("prereqs");
    while (jsarr.has_more()) {
        mutation_data[id].prereqs.push_back(jsarr.next_string());
    }
    // Helps to be able to have a trait require more than one other trait
    // (Individual prereq-lists are "OR", not "AND".)
    // Traits shoud NOT appear in both lists for a given mutation, unless
    // you want that trait to satisfy both requirements.
    // These are additional to the first list.
    jsarr = jsobj.get_array("prereqs2");
    while (jsarr.has_more()) {
        mutation_data[id].prereqs2.push_back(jsarr.next_string());
    }
    // Dedicated-purpose prereq slot for Threshold mutations
    jsarr = jsobj.get_array("threshreq");
    // Stuff like Huge might fit in more than one mutcat post-threshold, so yeah
    while (jsarr.has_more()) {
        mutation_data[id].threshreq.push_back(jsarr.next_string());
    }
    jsarr = jsobj.get_array("cancels");
    while (jsarr.has_more()) {
        mutation_data[id].cancels.push_back(jsarr.next_string());
    }
    jsarr = jsobj.get_array("changes_to");
    while (jsarr.has_more()) {
        mutation_data[id].replacements.push_back(jsarr.next_string());
    }
    jsarr = jsobj.get_array("leads_to");
    while (jsarr.has_more()) {
        mutation_data[id].additions.push_back(jsarr.next_string());
    }
    jsarr = jsobj.get_array("category");
    while (jsarr.has_more()) {
        std::string s = jsarr.next_string();
        mutation_data[id].category.push_back(s);
        mutations_category[s].push_back(id);
    }
    jsarr = jsobj.get_array("wet_protection");
    while (jsarr.has_more()) {
        JsonObject jo = jsarr.next_object();
        std::string part_id = jo.get_string("part");
        int ignored = jo.get_int("ignored", 0);
        int neutral = jo.get_int("neutral", 0);
        int good = jo.get_int("good", 0);
        tripoint protect = tripoint(ignored, neutral, good);
        mutation_data[id].protection[part_id] =
            mutation_wet(body_parts[part_id], protect);
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

