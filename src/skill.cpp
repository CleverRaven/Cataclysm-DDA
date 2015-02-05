#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>    // std::min
#include <list>

#include "skill.h"
#include "rng.h"

#include "json.h"

#include "options.h"
#include "output.h"
#include "debug.h"

Skill::Skill()
{
    _ident = std::string("null");

    _name = std::string("nothing");
    _description = std::string("The zen-most skill there is.");
}

Skill::Skill(size_t id, std::string ident, std::string name, std::string description,
             std::set<std::string> tags)
{
    _id = id;
    _ident = ident;

    _name = name;
    _description = description;
    _tags = tags;
}

std::vector<const Skill*> Skill::skills;

void Skill::reset()
{
    for( auto &skill : skills ) {
        delete skill;
    }
    skills.clear();
}

void Skill::load_skill(JsonObject &jsobj)
{
    std::string ident = jsobj.get_string("ident");
    for(std::vector<const Skill*>::iterator a = skills.begin(); a != skills.end(); ++a) {
        if ((*a)->_ident == ident) {
            delete *a;
            skills.erase(a);
            break;
        }
    }
    std::string name = _(jsobj.get_string("name").c_str());
    std::string description = _(jsobj.get_string("description").c_str());

    std::set<std::string> tags;
    JsonArray jsarr = jsobj.get_array("tags");
    while (jsarr.has_more()) {
        tags.insert(jsarr.next_string());
    }

    const Skill* sk = new Skill(skills.size(), ident, name, description, tags);
    skills.push_back(sk);
    DebugLog( D_INFO, DC_ALL ) << "Loaded skill: " << name;
}

const Skill* Skill::skill(const std::string& ident)
{
    for( auto &skill : Skill::skills ) {
        if( ( skill )->_ident == ident ) {
            return skill;
        }
    }
    if(ident != "none") {
        debugmsg("unknown skill %s", ident.c_str());
    }
    return NULL;
}

const Skill* Skill::skill(size_t id)
{
    return Skill::skills[id];
}

const Skill* Skill::random_skill_with_tag(const std::string& tag)
{
    std::list<const Skill*> valid;
    for( auto &skill : Skill::skills ) {
        if( ( skill )->_tags.find( tag ) != ( skill )->_tags.end() ) {
            valid.push_back( skill );
        }
    }
    if (valid.empty()) {
        return NULL;
    }
    std::list<const Skill*>::iterator chosen = valid.begin();
    std::advance(chosen, rng(0, valid.size() - 1));
    return *chosen;
}

size_t Skill::skill_count()
{
    return Skill::skills.size();
}

// used for the pacifist trait
bool Skill::is_combat_skill() const
{
    return this->_tags.find("combat_skill") != this->_tags.end();
}

SkillLevel::SkillLevel(int level, int exercise, bool isTraining, int lastPracticed)
{
    _level = level;
    _exercise = exercise;
    _isTraining = isTraining;
    if(lastPracticed == 0) {
        _lastPracticed = HOURS(ACTIVE_WORLD_OPTIONS["INITIAL_TIME"]);
    } else {
        _lastPracticed = lastPracticed;
    }
}

SkillLevel::SkillLevel(int minLevel, int maxLevel, int minExercise, int maxExercise,
                       bool isTraining, int lastPracticed)
{
    _level = rng(minLevel, maxLevel);
    _exercise = rng(minExercise, maxExercise);
    _isTraining = isTraining;
    if(lastPracticed == 0) {
        _lastPracticed = HOURS(ACTIVE_WORLD_OPTIONS["INITIAL_TIME"]);
    } else {
        _lastPracticed = lastPracticed;
    }
}

void SkillLevel::train(int amount)
{
    _exercise += amount;

    if (_exercise >= 100 * (_level + 1)) {
        _exercise = 0;
        ++_level;
    }
}

static int rustRate(int level)
{
    int forgetCap = std::min(level, 7);
    return 32768 / int(std::pow(2.0, double(forgetCap - 1)));
}

bool SkillLevel::isRusting() const
{
    return OPTIONS["SKILL_RUST"] != "off" && (_level > 0) &&
           (calendar::turn - _lastPracticed) > rustRate(_level);
}

bool SkillLevel::rust( bool charged_bio_mem )
{
    if (_level > 0 && calendar::turn > _lastPracticed &&
        (calendar::turn - _lastPracticed) % rustRate(_level) == 0) {
        if (charged_bio_mem) {
            return one_in(5);
        }
        _exercise -= _level;

        if (_exercise < 0) {
            if (OPTIONS["SKILL_RUST"] == "vanilla" || OPTIONS["SKILL_RUST"] == "int") {
                _exercise = (100 * _level) - 1;
                --_level;
            } else {
                _exercise = 0;
            }
        }
    }
    return false;
}

void SkillLevel::practice()
{
    _lastPracticed = calendar::turn;
}

void SkillLevel::readBook(int minimumGain, int maximumGain, int maximumLevel)
{
    int gain = rng(minimumGain, maximumGain);

    if (_level < maximumLevel) {
        train(gain);
    }
    practice();
}

SkillLevel &SkillLevel::operator= (const SkillLevel &rhs)
{
    if (this == &rhs) {
        return *this;    // No self-assignment
    }

    _level = rhs._level;
    _exercise = rhs._exercise;
    _isTraining = rhs._isTraining;
    _lastPracticed = rhs._lastPracticed;

    return *this;
}

std::string skill_name(int sk)
{
    return Skill::skill(sk)->name();
}

std::string skill_description(int sk)
{
    return Skill::skill(sk)->description();
}

//Actually take the difference in barter skill between the two parties involved
//Caps at 200% when you are 5 levels ahead, int comparison is handled in npctalk.cpp
double price_adjustment(int barter_skill)
{
    if (barter_skill <= 0) {
        return 1.0;
    }
    if (barter_skill >= 5) {
        return 2.0;
    }
    switch (barter_skill) {
    case 1:
        return 1.05;
    case 2:
        return 1.15;
    case 3:
        return 1.30;
    case 4:
        return 1.65;
    default:
        return 1.0;//should never occur
    }
}
