#include "mtype.h"
#include "translations.h"

mtype::mtype () {
    id = "mon_null";
    name = _("human");
    description = "";
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
	def_chance =0;
    item_chance = 0;
    dies = NULL;
    sp_attack = NULL;
    sp_defense = NULL;
	luminance = 0;
    flags.insert(MF_HUMAN);
}

bool mtype::has_flag(m_flag flag) const {
    return bitflags[flag];
}

bool mtype::has_anger_trigger(monster_trigger trig) const {
    return bitanger[trig];
}

bool mtype::has_fear_trigger(monster_trigger trig) const {
    return bitfear[trig];
}

bool mtype::has_placate_trigger(monster_trigger trig) const {
    return bitplacate[trig];
}

bool mtype::in_category(std::string category) const {
    return (categories.find(category) != categories.end());
}

bool mtype::in_species(std::string spec) const {
    return (species.find(spec) != species.end());
}
