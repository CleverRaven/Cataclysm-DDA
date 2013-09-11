#include <iostream>
#include <fstream>
#include <sstream>

#include "profession.h"

#include "debug.h"
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

bool profession::load_profession(Jsin &jsin)
{
    profession prof;
    std::string s;
    jsin.start_object();
    while (!jsin.end_object()) {
        s = jsin.get_member_name();
        if (s == "ident") {
            prof._ident = jsin.get_string();
        } else if (s == "name") {
            prof._name = _(jsin.get_string().c_str());
        } else if (s == "description") {
            prof._description = _(jsin.get_string().c_str());
        } else if (s == "points") {
            prof._point_cost = jsin.get_int();
        } else if (s == "items") {
            jsin.start_array();
            while (!jsin.end_array()) {
                prof.add_item(jsin.get_string());
            }
        } else if (s == "skills") {
            jsin.start_array();
            while (!jsin.end_array()) {
                prof.add_skill(jsin.fetch_string("name"),
                               jsin.fetch_int("level"));
                jsin.skip_object();
            }
        } else if (s == "addictions") {
            jsin.start_array();
            while (!jsin.end_array()) {
                prof.add_addiction(addiction_type(jsin.fetch_string("type")),
                                   jsin.fetch_int("intensity"));
                jsin.skip_object();
            }
        } else if (s == "flags") {
            jsin.start_array();
            while (!jsin.end_array()) {
                prof.flags.insert(jsin.get_string());
            }
        } else if (s == "type") {
            jsin.skip_value();
        } else {
            dout(D_WARNING) << "Ignoring profession member: " << s << "\n";
            jsin.skip_value();
        }
    }
    if (prof._ident.empty()) {
        dout(D_ERROR) << "Failed to load profession, no ident found.\n";
        return false;
    }
    _all_profs[prof._ident] = prof;
    dout(D_INFO) << "Loaded profession: " << prof._name << "\n";
    return true;
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
        dout(D_ERROR) << "Tried to get invalid profession: " << ident << "\n";
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
