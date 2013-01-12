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

  Skill();
  Skill(size_t id, std::string ident, std::string name, std::string description);

  std::string name() { return _name; };
  std::string description() { return _description; };
};

std::string skill_name(int);
std::string skill_description(int);

double price_adjustment(int);

#endif
