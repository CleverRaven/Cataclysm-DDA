#include "mutation.h"
#include "json.h"
#include "pldata.h" // traits
#include "enums.h" // tripoint
#include "bodypart.h"

#include <vector>
#include <map>

// mfb(n) converts a flag to its appropriate position in covers's bitfield
#ifndef mfb
#define mfb(n) static_cast <unsigned long> (1 << (n))
#endif

std::vector<dream> dreams;
std::map<std::string, std::vector<std::string> > mutations_category;
std::map<std::string, mutation_branch> mutation_data;
std::map<std::string, unsigned> bodyparts_list;

void load_mutation(JsonObject &jsobj)
{
    trait new_trait;
    JsonArray jsarr;
    std::string id = jsobj.get_string("id");

    new_trait.name = _(jsobj.get_string("name").c_str());
    new_trait.description = _(jsobj.get_string("description").c_str());
    new_trait.points = jsobj.get_int("points");
    new_trait.visibility = jsobj.get_int("visibility", 0);
    new_trait.ugliness = jsobj.get_int("ugliness", 0);
    new_trait.startingtrait = jsobj.get_bool("starting_trait", false);

    traits[id] = new_trait;

    mutation_data[id].valid = jsobj.get_bool("valid", true);

    jsarr = jsobj.get_array("prereqs");
    while (jsarr.has_more()) {
        mutation_data[id].prereqs.push_back(jsarr.next_string());
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
        mutation_data[id].protection.push_back(
            mutation_wet(bodyparts_list[part_id], protect));
    }
}

void init_mutation_parts()
{
    bodyparts_list["TORSO"] = mfb(bp_torso);
    bodyparts_list["HEAD"] = mfb(bp_head);
    bodyparts_list["EYES"] = mfb(bp_eyes);
    bodyparts_list["MOUTH"] = mfb(bp_mouth);
    bodyparts_list["ARMS"] = mfb(bp_arms);
    bodyparts_list["HANDS"] = mfb(bp_hands);
    bodyparts_list["LEGS"] = mfb(bp_legs);
    bodyparts_list["FEET"] = mfb(bp_feet);
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

