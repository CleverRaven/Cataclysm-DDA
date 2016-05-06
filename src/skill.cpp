#include "skill.h"
#include "rng.h"
#include "options.h"
#include "debug.h"
#include "translations.h"

#include <algorithm>
#include <iterator>

// TODO: a map, for Barry's sake make this a map.
std::vector<Skill> Skill::skills;

template<>
const skill_id string_id<Skill>::NULL_ID( "none" );

template<>
const Skill &string_id<Skill>::obj() const
{
    for( const Skill &skill : Skill::skills ) {
        if( skill.ident() == *this ) {
            return skill;
        }
    }

    debugmsg( "unknown skill %s", c_str() );
    static const Skill dummy{};
    return dummy;
}

template<>
bool string_id<Skill>::is_valid() const
{
    for( const Skill &skill : Skill::skills ) {
        if( skill.ident() == *this ) {
            return true;
        }
    }
    return false;
}

Skill::Skill()
  : Skill(NULL_ID, "nothing", "The zen-most skill there is.", std::set<std::string> {})
{
}

Skill::Skill(skill_id ident, std::string name, std::string description,
             std::set<std::string> tags)
  : _ident(std::move(ident)), _name(std::move(name)),
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
    skill_id ident = skill_id( jsobj.get_string("ident") );
    skills.erase(std::remove_if(begin(skills), end(skills), [&](Skill const &s) {
        return s._ident == ident; }), end(skills));

    std::string name           = _(jsobj.get_string("name").c_str());
    std::string description    = _(jsobj.get_string("description").c_str());
    std::set<std::string> tags = jsobj.get_tags("tags");

    DebugLog( D_INFO, DC_ALL ) << "Loaded skill: " << name;

    skills.emplace_back(std::move(ident), std::move(name), std::move(description),
                        std::move(tags));
}

skill_id Skill::from_legacy_int( const int legacy_id )
{
    static const std::array<skill_id, 28> legacy_skills = { {
        skill_id::NULL_ID, skill_id("dodge"), skill_id("melee"), skill_id("unarmed"),
        skill_id("bashing"), skill_id("cutting"), skill_id("stabbing"), skill_id("throw"),
        skill_id("gun"), skill_id("pistol"), skill_id("shotgun"), skill_id("smg"),
        skill_id("rifle"), skill_id("archery"), skill_id("launcher"), skill_id("mechanics"),
        skill_id("electronics"), skill_id("cooking"), skill_id("tailor"), skill_id("carpentry"),
        skill_id("firstaid"), skill_id("speech"), skill_id("barter"), skill_id("computer"),
        skill_id("survival"), skill_id("traps"), skill_id("swimming"), skill_id("driving"),
    } };
    if( static_cast<size_t>( legacy_id ) < legacy_skills.size() ) {
        return legacy_skills[legacy_id];
    }
    debugmsg( "legacy skill id %d is invalid", legacy_id );
    return skills.front().ident(); // return a non-null id because callers might not expect a null-id
}

skill_id Skill::random_skill_with_tag( const std::string &tag)
{
    std::vector<Skill const*> valid;
    for (auto const &s : skills) {
        if (s._tags.count(tag)) {
            valid.push_back(&s);
        }
    }
    if( valid.empty() ) {
        debugmsg( "could not find a skill with the %s tag", tag.c_str() );
        return skills.front().ident();
    }
    return random_entry( valid )->ident();
}

skill_id Skill::random_skill()
{
    return skills[rng( 0, skills.size() - 1 )].ident();
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

void SkillLevel::train(int amount, bool skip_scaling)
{
    if( skip_scaling ) {
        _exercise += amount;
    } else {
        const double scaling = OPTIONS["SKILL_TRAINING_SPEED"];
        if( scaling > 0.0 ) {
            _exercise += divide_roll_remainder( amount * scaling, 1.0 );
        }
    }

    if( _exercise >= 100 * (_level + 1) * (_level + 1) ) {
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
            _exercise = (100 * _level * _level) - 1;
            --_level;
        } else {
            _exercise = 0;
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
    if (_level < maximumLevel || maximumLevel < 0) {
        train((_level + 1) * rng(minimumGain, maximumGain));
    }

    practice();
}

bool SkillLevel::can_train() const
{
    return OPTIONS["SKILL_TRAINING_SPEED"] > 0.0;
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
