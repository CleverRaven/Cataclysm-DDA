#include <iostream>
#include <fstream>
#include <sstream>

#include "skill.h"
#include "rng.h"

#include "picojson.h"

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

std::vector<Skill*> Skill::skills(Skill::loadSkills());

std::vector<Skill*> Skill::loadSkills() {
  std::vector<Skill*> allSkills;

  picojson::value skillsRaw;

  std::ifstream skillsFile;

  skillsFile.open("data/raw/skills.json");

  skillsFile >> skillsRaw;

  if (skillsRaw.is<picojson::array>()) {
    const picojson::array& skills = skillsRaw.get<picojson::array>();
    for (picojson::array::const_iterator aSkill = skills.begin(); aSkill != skills.end(); ++aSkill) {
      const picojson::array& fields = aSkill->get<picojson::array>();
      picojson::array::const_iterator aField = fields.begin();

      std::string ident, name, description;

      ident = aField++->get<std::string>();
      name = aField++->get<std::string>();
      description = aField++->get<std::string>();

      Skill *newSkill = new Skill(allSkills.size(), ident, name, description);
      allSkills.push_back(newSkill);
    }
  } else {
    std::cout << skillsRaw << std::endl;
    exit(1);
  }

  return allSkills;
}

Skill* Skill::skill(std::string ident) {
 for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin()++;
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

size_t Skill::skill_count() {
  return Skill::skills.size();
}


SkillLevel::SkillLevel(int level, int exercise, bool isTraining) {
  _level = level;
  _exercise = exercise;
  _isTraining = isTraining;
}

SkillLevel::SkillLevel(int minLevel, int maxLevel, int minExercise, int maxExercise, bool isTraining) {
  _level = rng(minLevel, maxLevel);
  _exercise = rng(minExercise, maxExercise);
  _isTraining = isTraining;
}

int SkillLevel::comprehension(int intellect, bool fastLearner) {
  if (intellect == 0)
    return 0;

  int base_comprehension;

  if (intellect <= 8) {
    base_comprehension = intellect * 10;
  } else {
    base_comprehension = 80 + (intellect - 8) * 8;
  }

  if (fastLearner) {
    base_comprehension = base_comprehension / 2 * 3;
  }

  int skill_penalty;

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

int SkillLevel::train(int &level) {
  ++_exercise;

  if (_exercise == 100) {
    _exercise = 0;
    ++_level;
  }

  level = _level;

  return _exercise;
}

int SkillLevel::rust(int &level) {
  --_exercise;

  if (_exercise == 100) {
    _exercise = 0;
    --_level;
  }

  level = _level;

  return _exercise;
}

int SkillLevel::readBook(int minimumGain, int maximumGain, int maximumLevel) {
  int gain = rng(minimumGain, maximumGain);

  int level;

  for (int i = 0; i < gain; ++i) {
    train(level);

    if (level >= maximumLevel)
      break;
  }

  return _exercise;
}


std::istream& operator>>(std::istream& is, SkillLevel& obj) {
  int level; int exercise; bool isTraining;

  is >> level >> exercise >> isTraining;

  obj = SkillLevel(level, exercise, isTraining);

  return is;
}

std::ostream& operator<<(std::ostream& os, const SkillLevel& obj) {
  os << obj.level() << " " << obj.exercise() << " " << obj.isTraining() << " ";

  return os;
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
