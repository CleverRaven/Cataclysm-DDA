#include <iostream>
#include <fstream>
#include <sstream>

#include "scenario.h"

#include "debug.h"
#include "json.h"
#include "player.h"
#include "item_factory.h"
#include "bionics.h"
#include "start_location.h"

scenario::scenario()
   : _ident(""), _name_male("null"), _name_female("null"),
     _description_male("null"), _description_female("null"), _start_location("null")
{
}
scenario::scenario(std::string scen)
{

    
}
scenario::scenario(std::string ident, std::string name, std::string description, std::string start_location, profession* prof)
{
    _ident = ident;
    _name_male = name;
    _name_female = name;
    _description_male = description;
    _description_female = description;
    _start_location = start_location;
    _profession = prof;

}

scenmap scenario::_all_scens;

void scenario::load_scenario(JsonObject &jsobj)
{
    scenario scen;
    JsonArray jsarr;

    scen._ident = jsobj.get_string("ident");
    //If the "name" is an object then we have to deal with gender-specific titles,
    if(jsobj.has_object("name")) {
        JsonObject name_obj=jsobj.get_object("name");
        scen._name_male = pgettext("scenario_male", name_obj.get_string("male").c_str());
        scen._name_female = pgettext("scenario_female", name_obj.get_string("female").c_str());
    }
    else {
        // Same profession names for male and female in English.
        // Still need to different names in other languages.
        const std::string name = jsobj.get_string("name");
        scen._name_female = pgettext("scenario_female", name.c_str());
        scen._name_male = pgettext("scenario_male", name.c_str());
    }

    const std::string desc = jsobj.get_string("description").c_str();
    scen._description_male = pgettext("scen_desc_male", desc.c_str());
    scen._description_female = pgettext("scen_desc_female", desc.c_str());

    const std::string start = jsobj.get_string("start_location").c_str();
    scen._start_location = pgettext("start_location", start.c_str());

    const std::string proffe = jsobj.get_string("profession").c_str();
    scen._profession = profession::prof(pgettext("profession",proffe.c_str()));
    

    JsonObject items_obj=jsobj.get_object("items");
    scen.add_items_from_jsonarray(items_obj.get_array("both"), "both");
    scen.add_items_from_jsonarray(items_obj.get_array("male"), "male");
    scen.add_items_from_jsonarray(items_obj.get_array("female"), "female");
       

    jsarr = jsobj.get_array("flags");
    while (jsarr.has_more()) {
        scen.flags.insert(jsarr.next_string());
    }

    _all_scens[scen._ident] = scen;
    DebugLog( D_INFO, DC_ALL ) << "Loaded scenario: " << scen._ident;
}

scenario* scenario::scen(std::string ident)
{
    scenmap::iterator scen = _all_scens.find(ident);
    if (scen != _all_scens.end())
    {
        return &(scen->second);
    }
    else
    {
        debugmsg("Tried to get invalid scenario: %s", ident.c_str());
        return NULL;
    }
}

scenario* scenario::generic()
{
    return scenario::scen("Refugee");
}

// Strategy: a third of the time, return the generic profession.  Otherwise, return a profession,
// weighting 0 cost professions more likely--the weight of a profession with cost n is 2/(|n|+2),
// e.g., cost 1 is 2/3rds as likely, cost -2 is 1/2 as likely.
scenario* scenario::weighted_random()
{
    if (one_in(3)) {
        return generic();
    } else {
        scenario* retval = 0;
        while(retval == 0) {
            scenmap::iterator iter = _all_scens.begin();
            for (int i = rng(0, _all_scens.size() - 1); i > 0; --i) {
                ++iter;
            }
            if (x_in_y(2, abs(iter->second.point_cost()) + 2)) {
                retval = &(iter->second);
            }  // else reroll in the while loop.
        }
        return retval;
    }
}

bool scenario::exists(std::string ident)
{
    return _all_scens.find(ident) != _all_scens.end();
}

scenmap::const_iterator scenario::begin()
{
    return _all_scens.begin();
}

scenmap::const_iterator scenario::end()
{
    return _all_scens.end();
}

int scenario::count()
{
    return _all_scens.size();
}

void scenario::reset()
{
    _all_scens.clear();
}

void scenario::check_definitions()
{
    for (scenmap::const_iterator a = _all_scens.begin(); a != _all_scens.end(); ++a) {
        a->second.check_definition();
    }
}

void scenario::check_definition() const
{
    for (std::vector<std::string>::const_iterator a = _starting_items.begin(); a != _starting_items.end(); ++a) {
        if (!item_controller->has_template(*a)) {
            debugmsg("item %s for scenario %s does not exist", a->c_str(), _ident.c_str());
        }
    }
    for (std::vector<std::string>::const_iterator a = _starting_items_female.begin(); a != _starting_items_female.end(); ++a) {
        if (!item_controller->has_template(*a)) {
            debugmsg("item %s for scenario %s does not exist", a->c_str(), _ident.c_str());
        }
    }
    for (std::vector<std::string>::const_iterator a = _starting_items_male.begin(); a != _starting_items_male.end(); ++a) {
        if (!item_controller->has_template(*a)) {
            debugmsg("item %s for scenario %s does not exist", a->c_str(), _ident.c_str());
        }
    }

}

bool scenario::has_initialized()
{
    return exists("Refugee");
}

void scenario::add_items_from_jsonarray(JsonArray jsarr, std::string gender)
{
    while (jsarr.has_more()) {
        add_item(jsarr.next_string(), gender);
    }
}

void scenario::add_item(std::string item, std::string gender)
{
    if(gender=="male") {
        _starting_items_male.push_back(item);
    }
    else if(gender=="female") {
        _starting_items_female.push_back(item);
    }
    else {
        _starting_items.push_back(item);
    }
}


std::string scenario::ident() const
{
    return _ident;
}

std::string scenario::gender_appropriate_name(bool male) const
{
    if(male) {
        return _name_male;
    }
    else {
        return _name_female;
    }
}

std::string scenario::description(bool male) const
{
    if(male) {
        return _description_male;
    }
    else {
        return _description_female;
    }
}

signed int scenario::point_cost() const
{
    return 0;
}

std::string scenario::start_location() const
{
    return _start_location;
}

profession* scenario::prof() const
{
    return _profession;
}
std::vector<std::string> scenario::items() const
{
    return _starting_items;
}

std::vector<std::string> scenario::items_male() const
{
    return _starting_items_male;
}

std::vector<std::string> scenario::items_female() const
{
    return _starting_items_female;
}

bool scenario::has_flag(std::string flag) const
{
    return flags.count(flag) != 0;
}

bool scenario::can_pick(player* u, int points) const
{
    //Not Going to Cost points, so leaving this here in case future restricions arise

    return true;
}
// vim:ts=4:sw=4:et:tw=0:fdm=marker:fdl=0:
