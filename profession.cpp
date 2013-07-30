#include <iostream>
#include <fstream>
#include <sstream>

#include "profession.h"
#include "output.h"

#include "catajson.h"

profession::profession()
   : _id(0), _ident(""), _name("null"), _description("null"), _point_cost(0)
{
}

profession::profession(unsigned int id, std::string ident, std::string name, std::string description, signed int points)
{
    _id = id;
    _ident = ident;
    _name = name;
    _description = description;
    _point_cost = points;
}

profmap profession::_all_profs;

void game::init_professions()
{
    profession::_all_profs = profession::load_professions();
}

profmap profession::load_professions()
{
    profmap allProfs;
    catajson profsRaw("data/raw/professions.json",true);

    unsigned int id = 0;
    for (profsRaw.set_begin(); profsRaw.has_curr() && json_good(); profsRaw.next())
    {
        ++id;
        catajson currProf = profsRaw.curr();
        std::string ident = currProf.get("ident").as_string();
        std::string name = currProf.get("name").as_string();
        std::string description = currProf.get("description").as_string();
        signed int points = currProf.get("points").as_int();

        name = _(name.c_str());
        description = _(description.c_str());

        profession newProfession(id, ident, name, description, points);

        catajson items = currProf.get("items");
        for (items.set_begin(); items.has_curr(); items.next())
        {
            newProfession.add_item(items.curr().as_string());
        }

        // Addictions are optional
        if (currProf.has("addictions"))
        {
            catajson addictions = currProf.get("addictions");
            for (addictions.set_begin(); addictions.has_curr(); addictions.next())
            {
                catajson currAdd = addictions.curr();
                std::string type_str = currAdd.get("type").as_string();
                add_type type = addiction_type(type_str);
                int intensity = currAdd.get("intensity").as_int();
                newProfession.add_addiction(type,intensity);
            }
        }
        // Skills are optional as well
        if (currProf.has("skills"))
        {
            catajson skills = currProf.get("skills");
            for (skills.set_begin(); skills.has_curr(); skills.next())
            {
                catajson currSkill = skills.curr();
                std::string skill_str = currSkill.get("name").as_string();
                // Verifying this skill exists MUST happen later since the
                // skills have not yet been loaded at this point.
                int level = currSkill.get("level").as_int();
                newProfession.add_skill(skill_str, level);
            }
        }
        // Optional flags
        if (currProf.has("flags"))
        {
            catajson cflags = currProf.get("flags");
            for (cflags.set_begin(); cflags.has_curr(); cflags.next())
            {
                newProfession.flags.insert(cflags.curr().as_string());
            }
        }
        allProfs[ident] = newProfession;
    }

    if(!json_good())
        exit(1);

    return allProfs;
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

unsigned int profession::id() const
{
    return _id;
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
