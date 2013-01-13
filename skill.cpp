#include <iostream>
#include <fstream>
#include <sstream>

#include "skill.h"
#include "rng.h"

Skill::Skill() {
  _ident = std::string("null");

  _name = std::string("nothing");
  _description = std::string("The zen-most skill there is.");
}

Skill::Skill(size_t id, std::string ident, std::string name, std::string description) {
  _id = id;
  _ident = ident;

  _name = name;
  _description = description;
}

std::vector<Skill> Skill::skills(Skill::loadSkills());

std::vector<Skill> Skill::loadSkills() {
  std::vector<Skill> allSkills;

  std::ifstream skills_file;

  skills_file.open("data/raw/SKILLS");

  while (!skills_file.eof()) {
    std::string ident, name;
    std::ostringstream description;

    getline(skills_file, ident);
    getline(skills_file, name);

    std::string tmp;

    while (1) {
      getline(skills_file, tmp);

      if (tmp == "") {
        description << '\b';

        Skill aSkill(allSkills.size(), ident, name, description.str());
        allSkills.push_back(aSkill);
        break;
      } else {
        description << tmp << '\n';
      }
    }
  }

  return allSkills;
}

size_t Skill::skill_id(std::string ident) {
  for (EACH_SKILL) {
    if (aSkill->_ident == ident) {
      return aSkill->_id;
    }
  }
}

Skill Skill::skill(std::string ident) {
  size_t skillID = Skill::skill_id(ident);

  return Skill::skill(skillID);
}

Skill Skill::skill(size_t id) {
  return Skill::skills[id];
}

size_t Skill::skill_count() {
  return Skill::skills.size();
}


SkillLevel::SkillLevel(uint32_t level, int32_t exercise, bool isTraining) {
  _level = level;
  _exercise = exercise;
  _isTraining = isTraining;
}

SkillLevel::SkillLevel(uint32_t minLevel, uint32_t maxLevel, int32_t minExercise, int32_t maxExercise, bool isTraining) {
  _level = rng(minLevel, maxLevel);
  _exercise = rng(minExercise, maxExercise);
  _isTraining = isTraining;
}

uint32_t SkillLevel::comprehension(uint32_t intellect, bool fastLearner) {
  if (intellect == 0)
    return 0;

  uint32_t base_comprehension;

  if (intellect <= 8) {
    base_comprehension = intellect * 10;
  } else {
    base_comprehension = 80 + (intellect - 8) * 8;
  }

  if (fastLearner) {
    base_comprehension = base_comprehension / 2 * 3;
  }

  uint32_t skill_penalty;

  if (_level <= intellect / 2) {
    skill_penalty = 0;
  } else if (_level <= intellect) {
    skill_penalty = _level;
  } else {
    skill_penalty = _level * 2;
  }

  if (skill_penalty >= base_comprehension) {
    return 1;
  } else {
    return base_comprehension - skill_penalty;
  }
}

int32_t SkillLevel::train(uint32_t &level) {
  ++_exercise;

  if (_exercise == 100) {
    _exercise = 0;
    ++_level;
  }

  level = _level;

  return _exercise;
}

int32_t SkillLevel::rust(uint32_t &level) {
  --_exercise;

  if (_exercise == 100) {
    _exercise = 0;
    --_level;
  }

  level = _level;

  return _exercise;
}

int32_t SkillLevel::readBook(uint32_t minimumGain, uint32_t maximumGain, uint32_t maximumLevel) {
  uint32_t gain = rng(minimumGain, maximumGain);

  uint32_t level;

  for (uint32_t i = 0; i < gain; ++i) {
    train(level);

    if (level >= maximumLevel)
      break;
  }

  return _exercise;
}


std::istream& operator>>(std::istream& is, SkillLevel& obj) {
  uint32_t level; int32_t exercise; bool isTraining;

  is >> level >> exercise >> isTraining;

  obj = SkillLevel(level, exercise, isTraining);

  return is;
}

std::ostream& operator<<(std::ostream& os, const SkillLevel& obj) {
  os << obj.level() << " " << obj.exercise() << " " << obj.isTraining() << " ";

  return os;
}


std::string skill_name(int sk) {
  return Skill::skill(sk).name();
}

std::string skill_description(int sk) {
  return Skill::skill(sk).description();
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
