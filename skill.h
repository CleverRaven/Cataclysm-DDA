#ifndef _SKILL_H_
#define _SKILL_H_

#include <string>

enum skill {
 sk_null = 0,
// Melee
 sk_dodge, sk_melee, sk_unarmed, sk_bashing, sk_cutting, sk_stabbing,
// Combat
 sk_throw, sk_gun, sk_pistol, sk_shotgun, sk_smg, sk_rifle,
// Crafting
 sk_mechanics, sk_electronics, sk_cooking, sk_tailor,
// Medical
 sk_firstaid,
// Social
 sk_speech, sk_barter,
// Other
 sk_computer, sk_butcher, sk_traps, sk_swimming,
 num_skill_types	// MUST be last!
};

std::string skill_name(int);
std::string skill_description(int);
double price_adjustment(int);
#endif
