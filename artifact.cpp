#include <sstream>
#include <vector>
#include "game.h"
#include "artifact.h"
#include "artifactdata.h"

std::vector<art_effect_passive> fill_good_passive();
std::vector<art_effect_passive> fill_bad_passive();
std::vector<art_effect_active> fill_good_active();
std::vector<art_effect_active> fill_bad_active();

std::string artifact_name(std::string type);

itype* game::new_artifact()
{
 if (one_in(2)) { // Generate a "tool" artifact

  it_artifact_tool *art = new it_artifact_tool();
  int form = rng(ARTTOOLFORM_NULL + 1, NUM_ARTTOOLFORMS - 1);

  artifact_tool_form_datum *info = &(artifact_tool_form_data[form]);
  art->name = artifact_name(info->name);
  art->color = info->color;
  art->sym = info->sym;
  art->m1 = info->m1;
  art->m2 = info->m2;
  art->volume = rng(info->volume_min, info->volume_max);
  art->weight = rng(info->weight_min, info->weight_max);
// Set up the basic weapon type
  artifact_weapon_datum *weapon = &(artifact_weapon_data[info->base_weapon]);
  art->melee_dam = rng(weapon->bash_min, weapon->bash_max);
  art->melee_cut = rng(weapon->cut_min, weapon->cut_max);
  art->m_to_hit = rng(weapon->to_hit_min, weapon->to_hit_max);
  art->item_flags = weapon->flags;
// Add an extra weapon perhaps?
  if (one_in(2)) {
   int select = rng(0, 2);
   if (info->extra_weapons[select] != ARTWEAP_NULL) {
    weapon = &(artifact_weapon_data[ info->extra_weapons[select] ]);
    art->volume += weapon->volume;
    art->weight += weapon->weight;
    art->melee_dam += rng(weapon->bash_min, weapon->bash_max);
    art->melee_cut += rng(weapon->bash_min, weapon->bash_max);
    art->m_to_hit += rng(weapon->to_hit_min, weapon->to_hit_max);
    art->item_flags |= weapon->flags;
    std::stringstream newname;
    newname << weapon->adjective << " " << info->name;
    art->name = artifact_name(newname.str());
   }
  }

  std::stringstream description;
  description << "This is the " << art->name << ".\n\
It is the only one of its kind.\n\
It may have unknown powers; use 'a' to activate them.";
  art->description = description.str();

// Finally, pick some powers
  art_effect_passive passive_tmp = AEP_NULL;
  art_effect_active active_tmp = AEA_NULL;
  int num_good = 0, num_bad = 0, value = 0;
  std::vector<art_effect_passive> good_effects = fill_good_passive();
  std::vector<art_effect_passive> bad_effects = fill_bad_passive();
  
// Wielded effects first
  while (!good_effects.empty() && !bad_effects.empty() &&
         (num_good < 1 || num_bad < 1 || one_in(num_good + 1) ||
          one_in(num_bad + 1) || value > 1)) {
   if (one_in(2)) { // Good
    int index = rng(0, good_effects.size() - 1);
    passive_tmp = good_effects[index];
    good_effects.erase(good_effects.begin() + index);
    num_good++;
   } else if (!bad_effects.empty()) { // Bad effect
    int index = rng(0, bad_effects.size() - 1);
    passive_tmp = bad_effects[index];
    bad_effects.erase(bad_effects.begin() + index);
    num_bad++;
   }
   value += passive_effect_cost[passive_tmp];
   art->effects_wielded.push_back(passive_tmp);
  }
// Next, carried effects; more likely to be just bad
  num_good = 0;
  num_bad = 0;
  value = 0;
  good_effects = fill_good_passive();
  bad_effects = fill_bad_passive();
  while (one_in(2) && !good_effects.empty() && !bad_effects.empty() &&
         ((num_good > 2 && one_in(num_good + 1)) || num_bad < 1 ||
          one_in(num_bad + 1) || value > 1)) {
   if (one_in(3)) { // Good
    int index = rng(0, good_effects.size() - 1);
    passive_tmp = good_effects[index];
    good_effects.erase(good_effects.begin() + index);
    num_good++;
   } else { // Bad effect
    int index = rng(0, bad_effects.size() - 1);
    passive_tmp = bad_effects[index];
    bad_effects.erase(bad_effects.begin() + index);
    num_bad++;
   }
   value += passive_effect_cost[passive_tmp];
   art->effects_carried.push_back(passive_tmp);
  }
// Finally, activated effects; not necessarily good or bad
  num_good = 0;
  num_bad = 0;
  art->def_charges = 0;
  art->max_charges = 0;
  std::vector<art_effect_active> good_a_effects = fill_good_active();
  std::vector<art_effect_active> bad_a_effects = fill_bad_active();
  while (!good_a_effects.empty() && !bad_a_effects.empty() &&
         ((num_bad > 0 && num_good == 0) || !one_in(3 - num_good) ||
          !one_in(3 - num_bad))) {
   if (!one_in(3)) { // Good effect
    int index = rng(0, good_a_effects.size() - 1);
    active_tmp = good_a_effects[index];
    good_a_effects.erase(good_a_effects.begin() + index);
    num_good++;
   } else { // Bad effect
    int index = rng(0, bad_a_effects.size() - 1);
    active_tmp = bad_a_effects[index];
    bad_a_effects.erase(bad_a_effects.begin() + index);
    num_bad++;
   }
   art->effects_activated.push_back(active_tmp);
   art->max_charges += rng(1, 3);
  }
  art->def_charges = art->max_charges;
// If we have charges, pick a recharge mechanism
  if (art->max_charges > 0)
   art->charge_type = art_charge( rng(ARTC_NULL + 1, NUM_ARTCS - 1) );

  art->id = itypes.size();
  itypes.push_back(art);
  return art;

 } else { // Generate an armor artifact

  it_artifact_armor *art = new it_artifact_armor();
  int form = rng(ARTARMFORM_NULL + 1, NUM_ARTARMFORMS - 1);
  artifact_armor_form_datum *info = &(artifact_armor_form_data[form]);

  art->name = artifact_name(info->name);
  art->sym = '['; // Armor is always [
  art->color = info->color;
  art->m1 = info->m1;
  art->m2 = info->m2;
  art->volume = info->volume;
  art->weight = info->weight;
  art->melee_dam = info->melee_bash;
  art->melee_cut = info->melee_cut;
  art->m_to_hit = info->melee_hit;
  art->item_flags = 0;
  art->covers = info->covers;
  art->encumber = info->encumb;
  art->dmg_resist = info->dmg_resist;
  art->cut_resist = info->cut_resist;
  art->env_resist = info->env_resist;
  art->warmth = info->warmth;
  art->storage = info->storage; 
  std::stringstream description;
  description << "This is the " << art->name << ".\n" <<
                 (info->plural ? "They are the only ones of their kind." :
                                 "It is the only one of its kind.");

// Modify the armor further
  if (!one_in(4)) {
   int index = rng(0, 4);
   if (info->available_mods[index] != ARMORMOD_NULL) {
    artifact_armor_mod mod = info->available_mods[index];
    artifact_armor_form_datum *modinfo = &(artifact_armor_mod_data[mod]);
    if (modinfo->volume >= 0 || art->volume > abs(modinfo->volume))
     art->volume += modinfo->volume;
    else
     art->volume = 1;

    if (modinfo->weight >= 0 || art->weight > abs(modinfo->weight))
     art->weight += modinfo->weight;
    else
     art->weight = 1;
 
    art->encumber += modinfo->encumb;

    if (modinfo->dmg_resist > 0 || art->dmg_resist > abs(modinfo->dmg_resist))
     art->dmg_resist += modinfo->dmg_resist;
    else
     art->dmg_resist = 0;

    if (modinfo->cut_resist > 0 || art->cut_resist > abs(modinfo->cut_resist))
     art->cut_resist += modinfo->cut_resist;
    else
     art->cut_resist = 0;

    if (modinfo->env_resist > 0 || art->env_resist > abs(modinfo->env_resist))
     art->env_resist += modinfo->env_resist;
    else
     art->env_resist = 0;
    art->warmth += modinfo->warmth;

    if (modinfo->storage > 0 || art->storage > abs(modinfo->storage))
     art->storage += modinfo->storage;
    else
     art->storage = 0;

    description << "\n" << (info->plural ? "They are " : "It is ") <<
                   modinfo->name;
   }
  }

  art->description = description.str();

// Finally, pick some effects
  int num_good = 0, num_bad = 0, value = 0;
  art_effect_passive passive_tmp = AEP_NULL;
  std::vector<art_effect_passive> good_effects = fill_good_passive();
  std::vector<art_effect_passive> bad_effects = fill_bad_passive();

  while (!good_effects.empty() && !bad_effects.empty() &&
         (num_good < 1 || one_in(num_good) || !one_in(3 - num_bad) ||
          value > 1)) {
   if (one_in(2)) { // Good effect
    int index = rng(0, good_effects.size() - 1);
    passive_tmp = good_effects[index];
    good_effects.erase(good_effects.begin() + index);
    num_good++;
   } else { // Bad effect
    int index = rng(0, bad_effects.size() - 1);
    passive_tmp = bad_effects[index];
    bad_effects.erase(bad_effects.begin() + index);
    num_bad++;
   }
   value += passive_effect_cost[passive_tmp];
   art->effects_worn.push_back(passive_tmp);
  }

  art->id = itypes.size();
  itypes.push_back(art);
  return art;
 }
}

std::vector<art_effect_passive> fill_good_passive()
{
 std::vector<art_effect_passive> ret;
 for (int i = AEP_NULL + 1; i < AEP_SPLIT; i++)
  ret.push_back( art_effect_passive(i) );
 return ret;
}

std::vector<art_effect_passive> fill_bad_passive()
{
 std::vector<art_effect_passive> ret;
 for (int i = AEP_SPLIT + 1; i < NUM_AEPS; i++)
  ret.push_back( art_effect_passive(i) );
 return ret;
}

std::vector<art_effect_active> fill_good_active()
{
 std::vector<art_effect_active> ret;
 for (int i = AEA_NULL + 1; i < AEA_SPLIT; i++)
  ret.push_back( art_effect_active(i) );
 return ret;
}

std::vector<art_effect_active> fill_bad_active()
{
 std::vector<art_effect_active> ret;
 for (int i = AEA_SPLIT + 1; i < NUM_AEAS; i++)
  ret.push_back( art_effect_active(i) );
 return ret;
}

std::string artifact_name(std::string type)
{
 std::stringstream ret;
 ret << type << " of ";
 std::string noun = artifact_noun[rng(0, NUM_ART_NOUNS - 1)];
 if (noun[0] == '+') {
  ret << "the ";
  noun = noun.substr(1); // Chop off '+'
 }
 ret << artifact_adj[rng(0, NUM_ART_ADJS - 1)] << " " << noun;
 return ret.str();
}


void game::process_artifact(item *it, player *p, bool wielded)
{
 std::vector<art_effect_passive> effects;
 if (it->is_armor()) {
  it_artifact_armor* armor = dynamic_cast<it_artifact_armor*>(it->type);
  effects = armor->effects_worn;
 } else if (it->is_tool()) {
  it_artifact_tool* tool = dynamic_cast<it_artifact_tool*>(it->type);
  effects = tool->effects_carried;
  if (wielded) {
   for (int i = 0; i < tool->effects_wielded.size(); i++)
    effects.push_back(tool->effects_wielded[i]);
  }
// Recharge it if necessary
  if (it->charges < tool->max_charges) {
   switch (tool->charge_type) {
    case ARTC_TIME:
     if (int(turn) % 300 == 0)
      it->charges++;
     break;
    case ARTC_SOLAR:
     if (int(turn) % 100 == 0 && is_in_sunlight(p->posx, p->posy))
      it->charges++;
     break;
    case ARTC_PAIN:
     if (int(turn) % 50 == 0) {
      add_msg("You suddenly feel sharp pain for no reason.");
      p->pain += 3 * rng(1, 3);
      it->charges++;
     }
     break;
    case ARTC_HP:
     if (int(turn) % 50 == 0) {
      add_msg("You feel your body decaying.");
      p->hurtall(1);
     }
     it->charges++;
     break;
   }
  }
 }

 for (int i = 0; i < effects.size(); i++) {
  switch (effects[i]) {
  case AEP_STR_UP:
   p->str_cur += 4;
   break;
  case AEP_DEX_UP:
   p->dex_cur += 4;
   break;
  case AEP_PER_UP:
   p->per_cur += 4;
   break;
  case AEP_INT_UP:
   p->int_cur += 4;
   break;
  case AEP_ALL_UP:
   p->str_cur += 2;
   p->dex_cur += 2;
   p->per_cur += 2;
   p->int_cur += 2;
   break;
  case AEP_SPEED_UP: // Handled in player::current_speed()
   break;

  case AEP_IODINE:
   if (p->radiation > 0)
    p->radiation--;
   break;

  case AEP_SMOKE:
   if (one_in(10)) {
    int x = p->posx + rng(-1, 1), y = p->posy + rng(-1, 1);
    if (m.add_field(this, x, y, fd_smoke, rng(1, 3)))
     add_msg("The %s emits some smoke.", it->tname().c_str());
   }
   break;

  case AEP_SNAKES:
   break; // Handled in player::hit()

  case AEP_EVIL:
   if (one_in(150)) { // Once every 15 minutes, on average
    p->add_disease(DI_EVIL, 300, this);
    if (!wielded && !it->is_armor())
     add_msg("You have an urge to %s the %s.",
             (it->is_armor() ? "wear" : "wield"), it->tname().c_str());
   }
   break;
  
  case AEP_SCHIZO:
   break; // Handled in player::suffer()

  case AEP_RADIOACTIVE:
   if (one_in(4))
    p->radiation++;
   break;

  case AEP_STR_DOWN:
   p->str_cur -= 3;
   break;

  case AEP_DEX_DOWN:
   p->dex_cur -= 3;
   break;

  case AEP_PER_DOWN:
   p->per_cur -= 3;
   break;

  case AEP_INT_DOWN:
   p->int_cur -= 3;
   break;

  case AEP_ALL_DOWN:
   p->str_cur -= 2;
   p->dex_cur -= 2;
   p->per_cur -= 2;
   p->int_cur -= 2;
   break;

  case AEP_SPEED_DOWN:
   break; // Handled in player::current_speed()
  }
 }
}
