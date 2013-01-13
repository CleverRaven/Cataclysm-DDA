#ifndef _SKILL_H_
#define _SKILL_H_

#include <string>
#include <vector>

enum skill {
 sk_null = 0,
// Melee
 sk_dodge, sk_melee, sk_unarmed, sk_bashing, sk_cutting, sk_stabbing,
// Combat
 sk_throw, sk_gun, sk_pistol, sk_shotgun, sk_smg, sk_rifle, sk_archery,
  sk_launcher,
// Crafting
 sk_mechanics, sk_electronics, sk_cooking, sk_tailor, sk_carpentry,
// Medical
 sk_firstaid,
// Social
 sk_speech, sk_barter,
// Other
 sk_computer, sk_survival, sk_traps, sk_swimming, sk_driving,
 num_skill_types	// MUST be last!
};

#define EACH_SKILL std::vector<Skill>::iterator aSkill = Skill::skills.begin(); aSkill != Skill::skills.end(); ++aSkill

class Skill {
  size_t _id;
  std::string _ident;

  std::string _name;
  std::string _description;

  static size_t skill_id(std::string ident);

 public:
  static std::vector<Skill> skills;
  static std::vector<Skill> loadSkills();
  static Skill skill(std::string ident);
  static Skill skill(size_t id);

  static size_t skill_count();

  Skill();
  Skill(size_t id, std::string ident, std::string name, std::string description);

  std::string name() { return _name; }
  std::string description() { return _description; }

  bool operator==(const Skill& b) const { return this->_ident == b._ident; }
  bool operator< (const Skill& b) const { return this->_ident <  b._ident; } // Only here for the benefit of std::map<Skill,T>
};

class SkillLevel {
  uint32_t _level;
  int32_t _exercise;
  bool _isTraining;

 public:
  SkillLevel(uint32_t level = 0, int32_t exercise = 0, bool isTraining = true);
  SkillLevel(uint32_t minLevel, uint32_t maxLevel, int32_t minExercise, int32_t maxExercise, bool isTraining = true);

  bool isTraining() { return _isTraining; }
  bool toggleTraining() { _isTraining = !_isTraining; return _isTraining; }

  uint32_t level() { return _level; }
  uint32_t level(uint32_t level) { _level = level; return level; }

  int32_t train(uint32_t amount, uint32_t &level);
  int32_t rust(uint32_t &level);

  bool operator==(const SkillLevel& b) const { return this->_level == b._level; }
  bool operator< (const SkillLevel& b) const { return this->_level <  b._level; }
  bool operator> (const SkillLevel& b) const { return this->_level >  b._level; }

  bool operator==(const uint32_t& b) const { return this->_level == b; }
  bool operator< (const uint32_t& b) const { return this->_level <  b; }
  bool operator> (const uint32_t& b) const { return this->_level >  b; }

  bool operator!=(const SkillLevel& b) const { return !(*this == b); }
  bool operator<=(const SkillLevel& b) const { return !(*this >  b); }
  bool operator>=(const SkillLevel& b) const { return !(*this <  b); }

  bool operator!=(const uint32_t& b) const { return !(*this == b); }
  bool operator<=(const uint32_t& b) const { return !(*this >  b); }
  bool operator>=(const uint32_t& b) const { return !(*this <  b); }
};

std::string skill_name(int);
std::string skill_description(int);

double price_adjustment(int);

#endif
