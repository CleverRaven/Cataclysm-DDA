#include <iostream>
#include <fstream>
#include <sstream>

#include "profession.h"

#include "output.h" //debugmsg
#include "json.h"
#include "player.h"
#include "item_factory.h"
#include "bionics.h"

profession::profession()
   : _ident(""), _name_male("null"), _name_female("null"),
     _description_male("null"), _description_female("null"), _point_cost(0)
{
}

profession::profession(std::string ident, std::string name, std::string description, signed int points)
{
    _ident = ident;
    _name_male = name;
    _name_female = name;
    _description_male = description;
    _description_female = description;
    _point_cost = points;
}

profmap profession::_all_profs;

void profession::load_profession(JsonObject &jsobj)
{
    profession prof;
    JsonArray jsarr;

    prof._ident = jsobj.get_string("ident");
    //If the "name" is an object then we have to deal with gender-specific titles,
    if(jsobj.has_object("name")) {
        JsonObject name_obj=jsobj.get_object("name");
        prof._name_male = pgettext("profession_male", name_obj.get_string("male").c_str());
        prof._name_female = pgettext("profession_female", name_obj.get_string("female").c_str());
    }
    else {
        // Same profession names for male and female in English.
        // Still need to different names in other languages.
        const std::string name = jsobj.get_string("name");
        prof._name_female = pgettext("profession_female", name.c_str());
        prof._name_male = pgettext("profession_male", name.c_str());
    }

    const std::string desc = jsobj.get_string("description").c_str();
    prof._description_male = pgettext("prof_desc_male", desc.c_str());
    prof._description_female = pgettext("prof_desc_female", desc.c_str());

    prof._point_cost = jsobj.get_int("points");

    JsonObject items_obj=jsobj.get_object("items");
    prof.add_items_from_jsonarray(items_obj.get_array("both"), "both");
    prof.add_items_from_jsonarray(items_obj.get_array("male"), "male");
    prof.add_items_from_jsonarray(items_obj.get_array("female"), "female");

    jsarr = jsobj.get_array("skills");
    while (jsarr.has_more()) {
        JsonObject jo = jsarr.next_object();
        prof.add_skill(jo.get_string("name"),
                       jo.get_int("level"));
    }
    jsarr = jsobj.get_array("addictions");
    while (jsarr.has_more()) {
        JsonObject jo = jsarr.next_object();
        prof.add_addiction(addiction_type(jo.get_string("type")),
                           jo.get_int("intensity"));
    }
    jsarr = jsobj.get_array("CBMs");
    while (jsarr.has_more()) {
        prof.add_CBM(jsarr.next_string());
    }
    jsarr = jsobj.get_array("flags");
    while (jsarr.has_more()) {
        prof.flags.insert(jsarr.next_string());
    }

    _all_profs[prof._ident] = prof;
    //dout(D_INFO) << "Loaded profession: " << prof._name;
}

profession* profession::prof(std::string ident)
{
    profmap::iterator prof = _all_profs.find(ident);
    if (prof != _all_profs.end())
    {
        return &(prof->second);
    }
    else
    {
        debugmsg("Tried to get invalid profession: %s", ident.c_str());
        return NULL;
    }
}

profession* profession::generic()
{
    return profession::prof("unemployed");
}

// Strategy: a third of the time, return the generic profession.  Otherwise, return a profession,
// weighting 0 cost professions more likely--the weight of a profession with cost n is 2/(|n|+2),
// e.g., cost 1 is 2/3rds as likely, cost -2 is 1/2 as likely.
profession* profession::weighted_random()
{
    if (one_in(3)) {
        return generic();
    } else {
        profession* retval = 0;
        while(retval == 0) {
            profmap::iterator iter = _all_profs.begin();
            for (int i = rng(0, _all_profs.size() - 1); i > 0; --i) {
                ++iter;
            }
            if (x_in_y(2, abs(iter->second.point_cost()) + 2)) {
                retval = &(iter->second);
            }  // else reroll in the while loop.
        }
        return retval;
    }
}

bool profession::exists(std::string ident)
{
    return _all_profs.find(ident) != _all_profs.end();
}

profmap::const_iterator profession::begin()
{
    return _all_profs.begin();
}

profmap::const_iterator profession::end()
{
    return _all_profs.end();
}

int profession::count()
{
    return _all_profs.size();
}

void profession::reset()
{
    _all_profs.clear();
}

void profession::check_definitions()
{
    for (profmap::const_iterator a = _all_profs.begin(); a != _all_profs.end(); ++a) {
        a->second.check_definition();
    }
}

void profession::check_definition() const
{
    for (std::vector<std::string>::const_iterator a = _starting_items.begin(); a != _starting_items.end(); ++a) {
        if (!item_controller->has_template(*a)) {
            debugmsg("item %s for profession %s does not exist", a->c_str(), _ident.c_str());
        }
    }
    for (std::vector<std::string>::const_iterator a = _starting_items_female.begin(); a != _starting_items_female.end(); ++a) {
        if (!item_controller->has_template(*a)) {
            debugmsg("item %s for profession %s does not exist", a->c_str(), _ident.c_str());
        }
    }
    for (std::vector<std::string>::const_iterator a = _starting_items_male.begin(); a != _starting_items_male.end(); ++a) {
        if (!item_controller->has_template(*a)) {
            debugmsg("item %s for profession %s does not exist", a->c_str(), _ident.c_str());
        }
    }
    for (std::vector<std::string>::const_iterator a = _starting_CBMs.begin(); a != _starting_CBMs.end(); ++a) {
        if (bionics.count(*a) == 0) {
            debugmsg("bionic %s for profession %s does not exist", a->c_str(), _ident.c_str());
        }
    }
    for (StartingSkillList::const_iterator a = _starting_skills.begin(); a != _starting_skills.end(); ++a) {
        // Skill::skill shows a debug message if the skill is unknown
        Skill::skill(a->first);
    }
}

bool profession::has_initialized()
{
    return exists("unemployed");
}

void profession::add_items_from_jsonarray(JsonArray jsarr, std::string gender)
{
    while (jsarr.has_more()) {
        add_item(jsarr.next_string(), gender);
    }
}

void profession::add_item(std::string item, std::string gender)
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

void profession::add_CBM(std::string CBM)
{
    _starting_CBMs.push_back(CBM);
}

void profession::add_addiction(add_type type,int intensity)
{
    _starting_addictions.push_back(addiction(type,intensity));
}
void profession::add_skill(const std::string& skill_name, const int level)
{
    _starting_skills.push_back(StartingSkill(skill_name, level));
}

std::string profession::ident() const
{
    return _ident;
}

std::string profession::gender_appropriate_name(bool male) const
{
    if(male) {
        return _name_male;
    }
    else {
        return _name_female;
    }
}

std::string profession::description(bool male) const
{
    if(male) {
        return _description_male;
    }
    else {
        return _description_female;
    }
}

signed int profession::point_cost() const
{
    return _point_cost;
}

std::vector<std::string> profession::items() const
{
    return _starting_items;
}

std::vector<std::string> profession::items_male() const
{
    return _starting_items_male;
}

std::vector<std::string> profession::items_female() const
{
    return _starting_items_female;
}

std::vector<addiction> profession::addictions() const
{
    return _starting_addictions;
}

std::vector<std::string> profession::CBMs() const
{
    return _starting_CBMs;
}

const profession::StartingSkillList profession::skills() const
{
    return _starting_skills;
}

bool profession::has_flag(std::string flag) const
{
    return flags.count(flag) != 0;
}

bool profession::can_pick(player* u, int points) const
{
    if (point_cost() - u->prof->point_cost() > points) {
        return false;
    }

    return true;
}
// vim:ts=4:sw=4:et:tw=0:fdm=marker:fdl=0:
