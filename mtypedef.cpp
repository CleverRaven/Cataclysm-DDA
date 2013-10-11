#include "game.h"
#include "mondeath.h"
#include "monattack.h"
#include "itype.h"
#include "setvector.h"


 // Default constructor
 mtype::mtype () {
  id = "mon_null";
  name = _("human");
  description = "";
  //species = species_none;
  sym = ' ';
  color = c_white;
  size = MS_MEDIUM;
  mat = "hflesh";
  phase = SOLID;
  difficulty = 0;
  agro = 0;
  morale = 0;
  speed = 0;
  melee_skill = 0;
  melee_dice = 0;
  melee_sides = 0;
  melee_cut = 0;
  sk_dodge = 0;
  armor_bash = 0;
  armor_cut = 0;
  hp = 0;
  sp_freq = 0;
  item_chance = 0;
  dies = NULL;
  sp_attack = NULL;
  luminance = 0;
  flags.insert(MF_HUMAN);
 }
 // Non-default (messy)
 mtype::mtype (int pid, std::string pname, monster_species pspecies, char psym,
        nc_color pcolor, m_size psize, std::string pmat,
        unsigned int pdiff, signed char pagro,
        signed char pmorale, unsigned int pspeed, unsigned char pml_skill,
        unsigned char pml_dice, unsigned char pml_sides, unsigned char pml_cut,
        unsigned char pdodge, unsigned char parmor_bash,
        unsigned char parmor_cut, signed char pitem_chance, int php,
        unsigned char psp_freq,
        void (mdeath::*pdies)      (game *, monster *),
        void (mattack::*psp_attack)(game *, monster *),
        std::string pdescription ) {
  /*
  id = pid;
  name = pname;
  species = pspecies;
  sym = psym;
  color = pcolor;
  size = psize;
  mat = pmat;
  difficulty = pdiff;
  agro = pagro;
  morale = pmorale;
  speed = pspeed;
  melee_skill = pml_skill;
  melee_dice = pml_dice;
  melee_sides = pml_sides;
  melee_cut = pml_cut;
  sk_dodge = pdodge;
  armor_bash = parmor_bash;
  armor_cut = parmor_cut;
  item_chance = pitem_chance;
  hp = php;
  sp_freq = psp_freq;
  dies = pdies;
  sp_attack = psp_attack;
  description = pdescription;

  anger = default_anger(species);
  fear = default_fears(species);
  */
 }

 bool mtype::has_flag(m_flag flag) const
 {
  return bitflags[flag];
 }
 bool mtype::has_anger_trigger(monster_trigger trig) const
 {
     return bitanger[trig];
 }
 bool mtype::has_fear_trigger(monster_trigger trig) const
 {
     return bitfear[trig];
 }
 bool mtype::has_placate_trigger(monster_trigger trig) const
 {
     return bitplacate[trig];
 }

bool mtype::in_category(std::string category) const
{
    if (categories.find(category) != categories.end()){
        return true;
    }
    return false;
}
bool mtype::in_species(std::string spec) const
{
    if (species.find(spec) != species.end()){
        return true;
    }
    return false;
}
