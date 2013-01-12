#include <iostream>
#include <fstream>
#include <sstream>

#include "skill.h"

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

  skills_file.open("data/SKILLS");

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
  for (std::vector<Skill>::iterator aSkill = Skill::skills.begin(); aSkill != Skill::skills.end(); ++aSkill) {
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
