#include "mtype.h"
#include "translations.h"
#include "monstergenerator.h"

mtype::mtype () {
    id = "mon_null";
    name = "human";
    name_plural = "humans";
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
    dies.push_back(&mdeath::normal);
    sp_attack = NULL;
    sp_defense = NULL;
    luminance = 0;
    flags.insert(MF_HUMAN);
}

std::string mtype::nname(unsigned int quantity) const {
    return ngettext(name.c_str(), name_plural.c_str(), quantity);
}

bool mtype::has_flag(m_flag flag) const {
    return bitflags[flag];
}

bool mtype::has_flag(std::string flag) const {
    return has_flag( MonsterGenerator::generator().m_flag_from_string( flag ) );
}

void mtype::set_flag(std::string flag, bool state) {
    if( state ) {
        flags.insert( MonsterGenerator::generator().m_flag_from_string( flag ) );
    } else {
        flags.erase( MonsterGenerator::generator().m_flag_from_string( flag ) );
    }
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

field_id mtype::bloodType() {
    if (has_flag(MF_ACID_BLOOD))
        //A monster that has the death effect "ACID" does not need to have acid blood.
        return fd_acid;
    if (has_flag(MF_BILE_BLOOD))
        return fd_bile;
    if (has_flag(MF_LARVA) || has_flag(MF_ARTHROPOD_BLOOD))
        return fd_blood_invertebrate;
    if (mat == "veggy")
        return fd_blood_veggy;
    if (mat == "iflesh")
        return fd_blood_insect;
    if (has_flag(MF_WARM))
        return fd_blood;
    return fd_null; //Please update the monster blood type code at bloodType() in monster.cpp when modifying these rules!
}
field_id mtype::gibType() {
    if (has_flag(MF_LARVA) || in_species("MOLLUSK"))
        return fd_gibs_invertebrate;
    if (mat == "veggy")
        return fd_gibs_veggy;
    if (mat == "iflesh")
        return fd_gibs_insect;
    return fd_gibs_flesh; //Please update the monster blood type code at gibType() in monster.cpp when modifying these rules!
}
