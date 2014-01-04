#include <fstream>
#include <sstream>
#include <algorithm>    // std::min
#include <list>

#include "skill.h"
#include "rng.h"

#include "json.h"

#include "options.h"
#include "output.h"

Skill::Skill() {
  _ident = std::string("null");

  _name = std::string("nothing");
  _description = std::string("The zen-most skill there is.");
}

Skill::Skill(size_t id, std::string ident, std::string name, std::string description, std::set<std::string> tags) {
  _id = id;
  _ident = ident;

  _name = name;
  _description = description;
  _tags = tags;
}

std::vector<Skill*> Skill::skills;

void Skill::reset()
{
    for(std::vector<Skill*>::iterator a = skills.begin(); a != skills.end(); ++a) {
        delete *a;
    }
    skills.clear();
}

void Skill::load_skill(JsonObject &jsobj)
{
    std::string ident = jsobj.get_string("ident");
    for(std::vector<Skill*>::iterator a = skills.begin(); a != skills.end(); ++a) {
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

    Skill *sk = new Skill(skills.size(), ident, name, description, tags);
    skills.push_back(sk);
    //dout(D_INFO) << "Loaded skill: " << name << "\n";
}

Skill* Skill::skill(std::string ident) {
 for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin();
      aSkill != Skill::skills.end(); ++aSkill) {
  if ((*aSkill)->_ident == ident) {
   return *aSkill;
  }
 }
 return NULL;
}

Skill* Skill::skill(size_t id) {
  return Skill::skills[id];
}

Skill* Skill::random_skill_with_tag(std::string tag) {
    std::list<Skill*> valid;
    for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin();
         aSkill != Skill::skills.end(); ++aSkill)
    {
        if ((*aSkill)->_tags.find(tag) != (*aSkill)->_tags.end())
        {
            valid.push_back(*aSkill);
        }
    }
    if (valid.size() == 0)
    {
        return NULL;
    }
    std::list<Skill*>::iterator chosen = valid.begin();
    std::advance(chosen, rng(0, valid.size() - 1));
    return *chosen;
}

size_t Skill::skill_count() {
  return Skill::skills.size();
}


SkillLevel::SkillLevel(int level, int exercise, bool isTraining, int lastPracticed)
{
    _level = level;
    _exercise = exercise;
    _isTraining = isTraining;
    if(lastPracticed == 0)
    {
        _lastPracticed = HOURS(ACTIVE_WORLD_OPTIONS["INITIAL_TIME"]);
    }
    else
    {
        _lastPracticed = lastPracticed;
    }
}

SkillLevel::SkillLevel(int minLevel, int maxLevel, int minExercise, int maxExercise,
                       bool isTraining, int lastPracticed)
{
    _level = rng(minLevel, maxLevel);
    _exercise = rng(minExercise, maxExercise);
    _isTraining = isTraining;
    if(lastPracticed == 0)
    {
        _lastPracticed = HOURS(ACTIVE_WORLD_OPTIONS["INITIAL_TIME"]);
    }
    else
    {
        _lastPracticed = lastPracticed;
    }
}

void SkillLevel::train(int amount) {
  _exercise += amount;

  if (_exercise >= 100 * (_level + 1)) {
    _exercise = 0;
    ++_level;
  }
}

static int rustRate(int level)
{
    int forgetCap = std::min(level, 7);
    return 32768 / int(pow(2.0, double(forgetCap - 1)));
}

bool SkillLevel::isRusting(const calendar& turn) const
{
    return OPTIONS["SKILL_RUST"] != "off" && (_level > 0) && (turn - _lastPracticed) > rustRate(_level);
}

bool SkillLevel::rust(const calendar& turn, bool charged_bio_mem)
{
    if (_level > 0 && turn > _lastPracticed &&
       (turn - _lastPracticed) % rustRate(_level) == 0)
    {
        if (charged_bio_mem) return one_in(5);
        _exercise -= _level;

        if (_exercise < 0)
        {
            if (OPTIONS["SKILL_RUST"] == "vanilla" || OPTIONS["SKILL_RUST"] == "int")
            {
                _exercise = (100 * _level) - 1;
                --_level;
            } else {
                _exercise = 0;
            }
        }
    }
    return false;
}

void SkillLevel::practice(const calendar& turn)
{
    _lastPracticed = turn;
}

void SkillLevel::readBook(int minimumGain, int maximumGain, const calendar &turn,
                          int maximumLevel)
{
    int gain = rng(minimumGain, maximumGain);

    if (_level < maximumLevel)
    {
        train(gain);
    }
    practice(turn);
}

SkillLevel& SkillLevel::operator= (const SkillLevel &rhs)
{
 if (this == &rhs)
  return *this; // No self-assignment

  _level = rhs._level;
  _exercise = rhs._exercise;
  _isTraining = rhs._isTraining;
  _lastPracticed = rhs._lastPracticed;

 return *this;
}

std::string skill_name(int sk) {
  return Skill::skill(sk)->name();
}

std::string skill_description(int sk) {
  return Skill::skill(sk)->description();
}

double price_adjustment(int barter_skill) {
 switch (barter_skill) {
  case 0:  return 1.5;
  case 1:  return 1.4;
  case 2:  return 1.2;
  case 3:  return 1.0;
  case 4:  return 0.8;
  case 5:  return 0.6;
  case 6:  return 0.5;
  default: return 0.3 + 1.0 / barter_skill;
 }
}
