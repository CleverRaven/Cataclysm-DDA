#include "skill.h"
#include "rng.h"
#include "options.h"
#include "debug.h"
#include "translations.h"

#include <algorithm>
#include <iterator>

std::vector<Skill> Skill::skills;

Skill::Skill()
  : Skill(0, "null", "nothing", "The zen-most skill there is.", std::set<std::string> {})
{
}

Skill::Skill(size_t id, std::string ident, std::string name, std::string description,
             std::set<std::string> tags)
  : _id(std::move(id)), _ident(std::move(ident)), _name(std::move(name)),
    _description(std::move(description)), _tags(std::move(tags))
{
}

std::vector<Skill const*> Skill::get_skills_sorted_by(
    std::function<bool (Skill const&, Skill const&)> pred)
{
    std::vector<Skill const*> result;
    result.reserve(skills.size());

    std::transform(begin(skills), end(skills), back_inserter(result), [](Skill const& s) {
        return &s;
    });

    std::sort(begin(result), end(result), [&](Skill const* lhs, Skill const* rhs) {
        return pred(*lhs, *rhs);
    });

    return result;
}

void Skill::reset()
{
    skills.clear();
}

void Skill::load_skill(JsonObject &jsobj)
{
    std::string ident = jsobj.get_string("ident");
    skills.erase(std::remove_if(begin(skills), end(skills), [&](Skill const &s) {
        return s._ident == ident; }), end(skills));

    std::string name           = _(jsobj.get_string("name").c_str());
    std::string description    = _(jsobj.get_string("description").c_str());
    std::set<std::string> tags = jsobj.get_tags("tags");

    DebugLog( D_INFO, DC_ALL ) << "Loaded skill: " << name;

    skills.emplace_back(skills.size(), std::move(ident), std::move(name), std::move(description),
                        std::move(tags));
}

const Skill* Skill::skill(const std::string& ident)
{
    for( auto &skill : Skill::skills ) {
        if( skill._ident == ident ) {
            return &skill;
        }
    }
    if(ident != "none") {
        debugmsg("unknown skill %s", ident.c_str());
    }
    return nullptr;
}

const Skill* Skill::skill(size_t id)
{
    return &Skill::skills[id];
}

const Skill* Skill::random_skill_with_tag(const std::string& tag)
{
    std::vector<Skill const*> valid;
    for (auto const &s : skills) {
        if (s._tags.count(tag)) {
            valid.push_back(&s);
        }
    }

    auto const size = static_cast<long>(valid.size());
    return size ? valid[rng(0, size - 1)] : nullptr;
}

size_t Skill::skill_count()
{
    return Skill::skills.size();
}

// used for the pacifist trait
bool Skill::is_combat_skill() const
{
    return !!_tags.count("combat_skill");
}

SkillLevel::SkillLevel(int level, int exercise, bool isTraining, int lastPracticed)
  : _level(level), _exercise(exercise), _lastPracticed(lastPracticed), _isTraining(isTraining)
{
    if (lastPracticed <= 0) {
        _lastPracticed = HOURS(ACTIVE_WORLD_OPTIONS["INITIAL_TIME"]);
    }
}

SkillLevel::SkillLevel(int minLevel, int maxLevel, int minExercise, int maxExercise,
                       bool isTraining, int lastPracticed)
  : SkillLevel(rng(minLevel, maxLevel), rng(minExercise, maxExercise), isTraining, lastPracticed)

{
}

void SkillLevel::train(int amount)
{
    _exercise += amount;

    if (_exercise >= 100 * (_level + 1)) {
        _exercise = 0;
        ++_level;
    }
}

namespace {
int rustRate(int level)
{
    // for n = [0, 7]
    //
    // 2^15
    // -------
    // 2^(n-1)

    unsigned const n = level < 0 ? 0 : level > 7 ? 7 : level;
    return 1 << (15 - n + 1);
}
} //namespace

bool SkillLevel::isRusting() const
{
    return OPTIONS["SKILL_RUST"] != "off" && (_level > 0) &&
           (calendar::turn - _lastPracticed) > rustRate(_level);
}

bool SkillLevel::rust( bool charged_bio_mem )
{
    calendar const delta = calendar::turn - _lastPracticed;
    if (_level <= 0 || delta <= 0 || delta % rustRate(_level)) {
        return false;
    }

    if (charged_bio_mem) {
        return one_in(5);
    }

    _exercise -= _level;
    auto const &rust_type = OPTIONS["SKILL_RUST"];
    if (_exercise < 0) {
        if (rust_type == "vanilla" || rust_type == "int") {
            _exercise = (100 * _level) - 1;
            --_level;
        } else {
            _exercise = 0;
        }
    }

    return true;
}

void SkillLevel::practice()
{
    _lastPracticed = calendar::turn;
}

void SkillLevel::readBook(int minimumGain, int maximumGain, int maximumLevel)
{
    if (_level < maximumLevel || maximumLevel < 0) {
        train(rng(minimumGain, maximumGain));
    }

    practice();
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
