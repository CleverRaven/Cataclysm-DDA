#ifndef _PROFESSION_H_
#define _PROFESSION_H_

#include <string>
#include <vector>
#include <map>
#include "pldata.h"
#include "addiction.h"
#include "skill.h"
#include "json.h"

class profession;

typedef std::map<std::string, profession> profmap;

class profession
{
public:
    typedef std::pair<std::string, int> StartingSkill;
    typedef std::vector<StartingSkill> StartingSkillList;
private:
    std::string _ident;
    std::string _name;
    std::string _description;
    signed int _point_cost;
    std::vector<std::string> _starting_items;
    std::vector<addiction> _starting_addictions;
    std::vector<std::string> _starting_CBMs;
    std::set<std::string> flags; // flags for some special properties of the profession
    StartingSkillList  _starting_skills;

    void add_item(std::string item);
    void add_addiction(add_type, int);
    void add_CBM(std::string CBM);
    // Starting skills will boost the players level in those skills by a 
    // given amount.
    void add_skill(const std::string& skill_name, const int level);
public:
    //these three aren't meant for external use, but had to be made public regardless
    profession();
    profession(std::string ident, std::string name, std::string description, signed int points);
    static profmap _all_profs;

    static void load_profession(JsonObject &jsobj);

    // these should be the only ways used to get at professions
    static profession* prof(std::string ident);
    static profession* generic(); // points to the generic, default profession
    static bool exists(std::string ident);
    static profmap::const_iterator begin();
    static profmap::const_iterator end();
    static int count();

    static bool has_initialized();

    std::string ident() const;
    std::string name() const;
    std::string description() const;
    signed int point_cost() const;
    std::vector<std::string> items() const;
    std::vector<addiction> addictions() const;
    std::vector<std::string> CBMs() const;
    const StartingSkillList skills() const;

    /**
     * Check if this type of profession has a certain flag set.
     *
     * Current flags: none
     */
    bool has_flag(std::string flag) const;

    /**
     * Check if the given player can pick this job with the given amount
     * of points.
     *
     * @return "YES", or otherwise the reason for failure, e.g. "INSUFFICIENT_POINTS"
     */
    std::string can_pick(player* u, int points) const;

};

#endif
