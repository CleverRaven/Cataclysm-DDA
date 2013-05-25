#ifndef _SKILL_H_
#define _SKILL_H_

#include <string>
#include <vector>
#include <iostream>
#include <stdint.h>
#include "calendar.h"

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

class Skill {
  size_t _id;
  std::string _ident;

  std::string _name;
  std::string _description;

 public:
  static std::vector<Skill*> skills;
  static std::vector<Skill*> loadSkills();
  static Skill* skill(std::string ident);
  static Skill* skill(size_t id);

  static size_t skill_count();

  Skill();
  Skill(size_t id, std::string ident, std::string name, std::string description);

  //DEBUG
  size_t id() { return _id; }

  std::string ident() { return _ident; }
  std::string name() { return _name; }
  std::string description() { return _description; }

  bool operator==(const Skill& b) const { return this->_ident == b._ident; }
  bool operator< (const Skill& b) const { return this->_ident <  b._ident; } // Only here for the benefit of std::map<Skill,T>

  bool operator!=(const Skill& b) const { return !(*this == b); }
};

class SkillLevel {
  int _level;
  int _exercise;
  bool _isTraining;
  calendar _lastPracticed;

 public:
  SkillLevel(int level = 0, int exercise = 0, bool isTraining = true, int lastPracticed = 0);
  SkillLevel(int minLevel, int maxLevel, int minExercise, int maxExercise,
             bool isTraining = true, int lastPracticed = 0);

  bool isTraining() const { return _isTraining; }
  bool toggleTraining() { _isTraining = !_isTraining; return _isTraining; }

  int level() const { return _level; }
  int level(int plevel) { _level = plevel; return plevel; }

  int exercise(bool raw = false) const { return raw ? _exercise : _exercise / (_level + 1); }

  int lastPracticed() const { return _lastPracticed; }

  void train(int amount);
  bool isRusting(const calendar& turn) const;
  bool rust(const calendar& turn, bool forgetful, bool charged_bio_mem);
  void practice(const calendar& turn);

  void readBook(int minimumGain, int maximumGain, const calendar& turn, int maximumLevel = 0xFFFFFFFF);

  bool operator==(const SkillLevel& b) const { return this->_level == b._level && this->_exercise == b._exercise; }
  bool operator< (const SkillLevel& b) const { return this->_level <  b._level || (this->_level == b._level && this->_exercise < b._exercise); }
  bool operator> (const SkillLevel& b) const { return this->_level >  b._level || (this->_level == b._level && this->_exercise > b._exercise); }

  bool operator==(const int& b) const { return this->_level == b; }
  bool operator< (const int& b) const { return this->_level <  b; }
  bool operator> (const int& b) const { return this->_level >  b; }

  bool operator!=(const SkillLevel& b) const { return !(*this == b); }
  bool operator<=(const SkillLevel& b) const { return !(*this >  b); }
  bool operator>=(const SkillLevel& b) const { return !(*this <  b); }

  bool operator!=(const int& b) const { return !(*this == b); }
  bool operator<=(const int& b) const { return !(*this >  b); }
  bool operator>=(const int& b) const { return !(*this <  b); }

   SkillLevel& operator= (const SkillLevel &rhs);

  // Make skillLevel act like a raw level by default.
  operator int() const { return _level; }
};

std::istream& operator>>(std::istream& is, SkillLevel& obj);
std::ostream& operator<<(std::ostream& os, const SkillLevel& obj);

std::string skill_name(int);
std::string skill_description(int);

double price_adjustment(int);

#endif
