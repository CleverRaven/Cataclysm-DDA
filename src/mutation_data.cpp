#include "mutation.h"
#include "json.h"
#include "pldata.h" // traits
#include "enums.h" // tripoint
#include "bodypart.h"
#include "debug.h"
#include "martialarts.h"

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
    const std::string id = jsobj.get_string( "id" );
    mutation_branch &new_mut = mutation_data[id];
    trait &new_trait = traits[id];

    JsonArray jsarr;
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

    new_mut.valid = jsobj.get_bool("valid", true);
    new_mut.purifiable = jsobj.get_bool("purifiable", true);
    new_mut.initial_ma_styles = jsobj.get_string_array( "initial_ma_styles" );
    new_mut.threshold = jsobj.get_bool("threshold", false);
    new_mut.profession = jsobj.get_bool("profession", false);
    
    load_mutation_mods(jsobj, "passive_mods", new_mut.mods);
    /* Not currently supported due to inability to save active mutation state
    load_mutation_mods(jsobj, "active_mods", new_mut.mods); */

    new_mut.prereqs = jsobj.get_string_array( "prereqs" );
    // Helps to be able to have a trait require more than one other trait
    // (Individual prereq-lists are "OR", not "AND".)
    // Traits shoud NOT appear in both lists for a given mutation, unless
    // you want that trait to satisfy both requirements.
    // These are additional to the first list.
    new_mut.prereqs2 = jsobj.get_string_array( "prereqs2" );
    // Dedicated-purpose prereq slot for Threshold mutations
    // Stuff like Huge might fit in more than one mutcat post-threshold, so yeah
    new_mut.threshreq = jsobj.get_string_array( "threshreq" );
    new_mut.cancels = jsobj.get_string_array( "cancels" );
    new_mut.replacements = jsobj.get_string_array( "changes_to" );
    new_mut.additions = jsobj.get_string_array( "leads_to" );
    jsarr = jsobj.get_array("category");
    while (jsarr.has_more()) {
        std::string s = jsarr.next_string();
        new_mut.category.push_back(s);
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
        new_mut.protection[part_id] = mutation_wet(body_parts[part_id], protect);
    }
}

static void check_consistency( const std::vector<std::string> &mvec, const std::string &mid, const std::string &what )
{
    for( const auto &m : mvec ) {
        if( mutation_data.count( m ) == 0 ) {
            debugmsg( "mutation %s refers to undefined %s %s", mid.c_str(), what.c_str(), m.c_str() );
        }
    }
}

void mutation_branch::check_consistency()
{
    for( const auto & m : mutation_data ) {
        const auto &mid = m.first;
        const auto &mdata = m.second;
        for( const auto & style : mdata.initial_ma_styles ) {
            if( martialarts.count( style ) == 0 ) {
                debugmsg( "mutation %s refers to undefined martial art style %s", mid.c_str(), style.c_str() );
            }
        }
        ::check_consistency( mdata.prereqs, mid, "prereq" );
        ::check_consistency( mdata.prereqs2, mid, "prereqs2" );
        ::check_consistency( mdata.threshreq, mid, "threshreq" );
        ::check_consistency( mdata.cancels, mid, "cancels" );
        ::check_consistency( mdata.replacements, mid, "replacements" );
        ::check_consistency( mdata.additions, mid, "additions" );
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

