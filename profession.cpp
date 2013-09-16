#include <iostream>
#include <fstream>
#include <sstream>

#include "profession.h"

#include "output.h" //debugmsg
#include "json.h"

profession::profession()
   : _ident(""), _name("null"), _description("null"), _point_cost(0)
{
}

profession::profession(std::string ident, std::string name, std::string description, signed int points)
{
    _ident = ident;
    _name = name;
    _description = description;
    _point_cost = points;
}

profmap profession::_all_profs;

void profession::load_profession(JsonObject &jsobj)
{
    profession prof;
    JsonArray jsarr;

    prof._ident = jsobj.get_string("ident");
    prof._name = _(jsobj.get_string("name").c_str());
    prof._description = _(jsobj.get_string("description").c_str());
    prof._point_cost = jsobj.get_int("points");

    jsarr = jsobj.get_array("items");
    while (jsarr.has_more()) {
        prof.add_item(jsarr.next_string());
    }
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

bool profession::has_initialized()
{
    return exists("unemployed");
}

void profession::add_item(std::string item)
{
    _starting_items.push_back(item);
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

std::string profession::name() const
{
    return _name;
}

std::string profession::description() const
{
    return _description;
}

signed int profession::point_cost() const
{
    return _point_cost;
}

std::vector<std::string> profession::items() const
{
    return _starting_items;
}

std::vector<addiction> profession::addictions() const
{
    return _starting_addictions;
}
const profession::StartingSkillList profession::skills() const
{
    return _starting_skills;
}

bool profession::has_flag(std::string flag) const {
    return flags.count(flag) != 0;
}

std::string profession::can_pick(player* u, int points) const {
    std::string rval = "YES";
    if(point_cost() - u->prof->point_cost() > points) rval = "INSUFFICIENT_POINTS";
    return rval;
}
// vim:ts=4:sw=4:et:tw=0:fdm=marker:fdl=0:
