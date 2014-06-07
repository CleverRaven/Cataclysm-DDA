#include "item.h"
#include "player.h"
#include "output.h"
#include "skill.h"
#include "game.h"
#include "cursesdef.h"
#include "text_snippets.h"
#include "material.h"
#include "item_factory.h"
#include "options.h"
#include "uistate.h"
#include "helper.h" //to_string_int
#include "messages.h"

#include <cmath> // floor
#include <sstream>
#include <algorithm>

// mfb(n) converts a flag to its appropriate position in covers's bitfield
#ifndef mfb
#define mfb(n) static_cast <unsigned long> (1 << (n))
#endif

light_emission nolight = {0, 0, 0};

item::item()
{
    init();
}

item::item(const std::string new_type, unsigned int turn, bool rand)
{
    init();
    type = item_controller->find_template( new_type );
    bday = turn;
    corpse = type->corpse;
    name = type->name;
    if (type->is_gun()) {
        charges = 0;
    } else if (type->is_ammo()) {
        it_ammo* ammo = dynamic_cast<it_ammo*>(type);
        charges = ammo->count;
    } else if (type->is_food()) {
        it_comest* comest = dynamic_cast<it_comest*>(type);
        if (comest->charges == 1 && !made_of(LIQUID)) {
            charges = -1;
        } else {
            if (rand && comest->rand_charges.size() > 1) {
                int charge_roll = rng(1, comest->rand_charges.size() - 1);
                charges = rng(comest->rand_charges[charge_roll - 1], comest->rand_charges[charge_roll]);
            } else {
                charges = comest->charges;
            }
        }
    } else if (type->is_tool()) {
        it_tool* tool = dynamic_cast<it_tool*>(type);
        if (tool->max_charges == 0) {
            charges = -1;
        } else {
            if (rand && tool->rand_charges.size() > 1) {
                int charge_roll = rng(1, tool->rand_charges.size() - 1);
                charges = rng(tool->rand_charges[charge_roll - 1], tool->rand_charges[charge_roll]);
            } else {
                charges = tool->def_charges;
            }
            if (tool->ammo != "NULL") {
                curammo = dynamic_cast<it_ammo*>(item_controller->find_template(default_ammo(tool->ammo)));
            }
        }
    } else if (type->is_book()) {
        it_book* book = dynamic_cast<it_book*>(type);
        charges = book->chapters;
    } else if ((type->is_gunmod() && type->id == "spare_mag") || type->item_tags.count("MODE_AUX")) {
        charges = 0;
    } else
        charges = -1;
    if(type->is_var_veh_part()) {
        it_var_veh_part* varcarpart = dynamic_cast<it_var_veh_part*>(type);
        bigness= rng( varcarpart->min_bigness, varcarpart->max_bigness);
    }
    // Should be a flag, but we're out at the moment
    if( type->is_stationary() ) {
        note = SNIPPET.assign( (dynamic_cast<it_stationary*>(type))->category );
    }
}

void item::make_corpse(const std::string new_type, mtype* mt, unsigned int turn)
{
    init();
    active = mt->has_flag(MF_REVIVES)? true : false;
    type = item_controller->find_template( new_type );
    corpse = mt;
    bday = turn;
}

itype * item::nullitem_m = new itype();
itype * item::nullitem()
{
    return nullitem_m;
}

item::item(std::string itemdata)
{
    load_info(itemdata);
}

item::item(JsonObject &jo)
{
    deserialize(jo);
}

item::~item()
{
}

void item::init() {
    name = "";
    charges = -1;
    bday = 0;
    invlet = 0;
    damage = 0;
    burnt = 0;
    poison = 0;
    mode = "NULL";
    item_counter = 0;
    type = nullitem();
    curammo = NULL;
    corpse = NULL;
    active = false;
    owned = -1;
    mission_id = -1;
    player_id = -1;
    light = nolight;
    fridge = 0;
    rot = 0;
    last_rot_check = 0;
}

void item::make( const std::string new_type )
{
    type = item_controller->find_template( new_type );
    contents.clear();
}

void item::clear()
{
    // should we be clearing contents, as well?
    // Seems risky to - there aren't any reported content-clearing bugs
    // init(); // this should not go here either, or make() should not use it...
    item_tags.clear();
    item_vars.clear();
}

bool item::is_null() const
{
    static const std::string s_null("null"); // used alot, no need to repeat
    return (this == NULL || type == NULL || type->id == s_null);
}

item item::in_its_container()
{
    if (is_software()) {
        item ret( "usb_drive", 0);
        ret.contents.push_back(*this);
        ret.invlet = invlet;
        return ret;
    }
    if (is_food() && (dynamic_cast<it_comest*>(type))->container != "null") {
        it_comest *food = dynamic_cast<it_comest*>(type);
        item ret(food->container, bday);

        if (made_of(LIQUID)) {
            it_container* container = dynamic_cast<it_container*>(ret.type);
            charges = container->contains * food->charges;
        }
        ret.contents.push_back(*this);
        ret.invlet = invlet;
        return ret;
    } else if (is_ammo() && (dynamic_cast<it_ammo*>(type))->container != "null") {
        it_ammo *ammo = dynamic_cast<it_ammo*>(type);
        item ret(ammo->container, bday);

        if (made_of(LIQUID)) {
            it_container* container = dynamic_cast<it_container*>(ret.type);
            charges = container->contains * ammo->count;
        }
        ret.contents.push_back(*this);
        ret.invlet = invlet;
        return ret;
    } else {
        return *this;
    }
}

bool item::invlet_is_okay()
{
    return (inv_chars.find(invlet) != std::string::npos);
}

bool item::stacks_with(item rhs)
{
    bool stacks = (type   == rhs.type   && damage  == rhs.damage  &&
    active == rhs.active && charges == rhs.charges &&
    item_tags == rhs.item_tags &&
    item_vars == rhs.item_vars &&
    contents.size() == rhs.contents.size() &&
    (!goes_bad() || bday == rhs.bday));

    if ((corpse == NULL && rhs.corpse != NULL) ||
    (corpse != NULL && rhs.corpse == NULL)   )
        return false;

    if (corpse != NULL && rhs.corpse != NULL &&
    corpse->id != rhs.corpse->id)
        return false;

    if (contents.size() != rhs.contents.size())
        return false;

    if(is_var_veh_part())
        if(bigness != rhs.bigness)
            return false;

    for (size_t i = 0; i < contents.size() && stacks; i++) {
        stacks &= contents[i].stacks_with(rhs.contents[i]);
    }

    return stacks;
}

void item::put_in(item payload)
{
    contents.push_back(payload);
}
const char ivaresc=001;


/*
 * Old 1 line string output retained for mapbuffer
 */
std::string item::save_info() const
{
    // doing this manually so as not to recurse
    std::stringstream s;
    JsonOut jsout(s);
    serialize(jsout, false);
    return s.str();
}

bool itag2ivar( std::string &item_tag, std::map<std::string, std::string> &item_vars ) {
    size_t pos = item_tag.find('=');
    if(item_tag.at(0) == ivaresc && pos != std::string::npos && pos >= 2 ) {
        std::string var_name, val_decoded;
        int svarlen, svarsep;
        svarsep = item_tag.find('=');
        svarlen = item_tag.size();
        val_decoded = "";
        var_name = item_tag.substr(1, svarsep - 1); // will assume sanity here for now
        for(int s = svarsep + 1; s < svarlen; s++ ) { // cheap and temporary, afaik stringstream IFS = [\r\n\t ];
            if(item_tag[s] == ivaresc && s < svarlen - 2 ) {
                if ( item_tag[s + 1] == '0' && item_tag[s + 2] == 'A' ) {
                    s += 2;
                    val_decoded.append(1, '\n');
                } else if ( item_tag[s + 1] == '0' && item_tag[s + 2] == 'D' ) {
                    s += 2;
                    val_decoded.append(1, '\r');
                } else if ( item_tag[s + 1] == '0' && item_tag[s + 2] == '6' ) {
                    s += 2;
                    val_decoded.append(1, '\t');
                } else if ( item_tag[s + 1] == '2' && item_tag[s + 2] == '0' ) {
                    s += 2;
                    val_decoded.append(1, ' ');
                } else {
                    val_decoded.append(1, item_tag[s]); // hhrrrmmmmm should be passing \a?
                }
            } else {
                val_decoded.append(1, item_tag[s]);
            }
        }
        item_vars[var_name]=val_decoded;
        return true;
    } else {
        return false;
    }
}


void item::load_info(std::string data)
{
    std::stringstream dump;
    dump << data;
    char check=dump.peek();
    if ( check == ' ' ) {
        // sigh..
        check=data[1];
    }
    if ( check == '{' ) {
        JsonIn jsin(dump);
        try {
            deserialize(jsin);
        } catch (std::string jsonerr) {
            debugmsg("Bad item json\n%s", jsonerr.c_str() );
        }
        return;
    } else {
        load_legacy(dump);
    }
}


std::string item::info(bool showtext)
{
    std::vector<iteminfo> dummy;
    return info(showtext, &dummy);
}

std::string item::info(bool showtext, std::vector<iteminfo> *dump, bool debug)
{
    std::stringstream temp1, temp2;
    std::string space="   ";
    if( g != NULL && debug == false &&
        ( g->debugmon == true || g->u.has_artifact_with(AEP_SUPER_CLAIRVOYANCE) ) ) {
        debug = true;
    }
    if( !is_null() ) {
        dump->push_back(iteminfo("BASE", _("Volume: "), "", volume(), true, "", false, true));
        dump->push_back(iteminfo("BASE", space + _("Weight: "),
                                 string_format(_("<num> %s"),
                                               OPTIONS["USE_METRIC_WEIGHTS"].getValue() == "lbs" ?
                                               _("lbs") : _("kg")),
                                 g->u.convert_weight(weight()), false, "", true, true));
        dump->push_back(iteminfo("BASE", _("Bash: "), "", damage_bash(), true, "", false));
        if( has_flag("SPEAR") ) {
            dump->push_back(iteminfo("BASE", _(" Pierce: "), "", damage_cut(), true, "", false));
        } else if (has_flag("STAB")) {
            dump->push_back(iteminfo("BASE", _(" Stab: "), "", damage_cut(), true, "", false));
        } else {
            dump->push_back(iteminfo("BASE", _(" Cut: "), "", damage_cut(), true, "", false));
        }
        dump->push_back(iteminfo("BASE", _(" To-hit bonus: "), ((type->m_to_hit > 0) ? "+" : ""),
                                 type->m_to_hit, true, ""));
        dump->push_back(iteminfo("BASE", _("Moves per attack: "), "",
                                 attack_time(), true, "", true, true));
        dump->push_back(iteminfo("BASE", _("Price: "), "<num>",
                                 (double)price() / 100, false, "$", true, true));

        if (!get_material(1)->is_null()) {
            std::string text;
            if (get_material(2)->is_null()) {
                text = string_format(_("Material: %s"), get_material(1)->name().c_str());
            } else {
                text = string_format(_("Material: %s, %s"), get_material(1)->name().c_str(), get_material(2)->name().c_str());
            }
            dump->push_back(iteminfo("BASE", text));
        }

        if ( debug == true ) {
            if( g != NULL ) {
                dump->push_back(iteminfo("BASE", _("age: "), "",
                                         (int(calendar::turn) - bday) / (10 * 60), true, "", true, true));
                int maxrot = 0;
                item * food = NULL;
                if( goes_bad() ) {
                    food = this;
                    maxrot = dynamic_cast<it_comest*>(type)->spoils * 600;
                } else if(is_food_container()) {
                    food = &contents[0];
                    if ( food->goes_bad() ) {
                        maxrot =dynamic_cast<it_comest*>(food->type)->spoils * 600;
                    }
                }
                if ( food != NULL && maxrot != 0 ) {
                    dump->push_back(iteminfo("BASE", _("bday rot: "), "",
                                             (int(calendar::turn) - food->bday), true, "", true, true));
                    dump->push_back(iteminfo("BASE", _("temp rot: "), "",
                                             (int)food->rot, true, "", true, true));
                    dump->push_back(iteminfo("BASE", _(" max rot: "), "",
                                             (int)maxrot, true, "", true, true));
                    dump->push_back(iteminfo("BASE", _("  fridge: "), "",
                                             (int)food->fridge, true, "", true, true));
                    dump->push_back(iteminfo("BASE", _("last rot: "), "",
                                             (int)food->last_rot_check, true, "", true, true));
                }
            }
            dump->push_back(iteminfo("BASE", _("burn: "), "",  burnt, true, "", true, true));
        }
    }

    if (is_food()) {
        it_comest* food = dynamic_cast<it_comest*>(type);

        dump->push_back(iteminfo("FOOD", _("Nutrition: "), "", food->nutr, true, "", false, true));
        dump->push_back(iteminfo("FOOD", space + _("Quench: "), "", food->quench));
        dump->push_back(iteminfo("FOOD", _("Enjoyability: "), "", food->fun));
        dump->push_back(iteminfo("FOOD", _("Portions: "), "", abs(int(charges))));
        if (corpse != NULL && ( debug == true || ( g != NULL &&
             ( g->u.has_bionic("bio_scent_vision") || g->u.has_trait("CARNIVORE") ||
               g->u.has_artifact_with(AEP_SUPER_CLAIRVOYANCE) ) ) ) ) {
            dump->push_back(iteminfo("FOOD", _("Smells like: ") + corpse->name));
        }
    } else if (is_food_container()) {
        // added charge display for debugging
        it_comest* food = dynamic_cast<it_comest*>(contents[0].type);

        dump->push_back(iteminfo("FOOD", _("Nutrition: "), "", food->nutr, true, "", false, true));
        dump->push_back(iteminfo("FOOD", space + _("Quench: "), "", food->quench));
        dump->push_back(iteminfo("FOOD", _("Enjoyability: "), "", food->fun));
        dump->push_back(iteminfo("FOOD", _("Portions: "), "", abs(int(contents[0].charges))));

    } else if (is_ammo()) {
        // added charge display for debugging
        it_ammo* ammo = dynamic_cast<it_ammo*>(type);

        if (ammo->type != "NULL") {
            dump->push_back(iteminfo("AMMO", _("Type: "), ammo_name(ammo->type)));
        }
        dump->push_back(iteminfo("AMMO", _("Damage: "), "", ammo->damage, true, "", false, true));
        dump->push_back(iteminfo("AMMO", space + _("Armor-pierce: "), "",
                                 ammo->pierce, true, "", true, true));
        dump->push_back(iteminfo("AMMO", _("Range: "), "",
                                 ammo->range, true, "", false, true));
        dump->push_back(iteminfo("AMMO", space + _("Dispersion: "), "",
                                 ammo->dispersion, true, "", true, true));
        dump->push_back(iteminfo("AMMO", _("Recoil: "), "", ammo->recoil, true, "", true, true));
        dump->push_back(iteminfo("AMMO", _("Count: "), "", ammo->count));
    } else if (is_ammo_container()) {
        it_ammo* ammo = dynamic_cast<it_ammo*>(contents[0].type);

        dump->push_back(iteminfo("AMMO", _("Type: "), ammo_name(ammo->type)));
        dump->push_back(iteminfo("AMMO", _("Damage: "), "", ammo->damage, true, "", false, true));
        dump->push_back(iteminfo("AMMO", space + _("Armor-pierce: "), "",
                                 ammo->pierce, true, "", true, true));
        dump->push_back(iteminfo("AMMO", _("Range: "), "", ammo->range, true, "", false, true));
        dump->push_back(iteminfo("AMMO", space + _("Dispersion: "), "",
                                 ammo->dispersion, true, "", false, true));
        dump->push_back(iteminfo("AMMO", _("Recoil: "), "", ammo->recoil, true, "", true, true));
        dump->push_back(iteminfo("AMMO", _("Count: "), "", contents[0].charges));

    } else if (is_gun()) {
        it_gun* gun = dynamic_cast<it_gun*>(type);
        int ammo_dam = 0;
        int ammo_range = 0;
        int ammo_recoil = 0;
        int ammo_pierce = 0;
        int ammo_dispersion = 0;
        bool has_ammo = (curammo != NULL && charges > 0);
        if (has_ammo) {
            ammo_dam = curammo->damage;
            ammo_range = curammo->range;
            ammo_recoil = curammo->recoil;
            ammo_pierce = curammo->pierce;
            ammo_dispersion = curammo->dispersion;
        }

        dump->push_back(iteminfo("GUN", _("Skill used: "), gun->skill_used->name()));
        dump->push_back(iteminfo("GUN", _("Ammunition: "), string_format(ngettext("<num> round of %s", "<num> rounds of %s", clip_size()),
                                 ammo_name(ammo_type()).c_str()), clip_size(), true));

        //damage of gun
        dump->push_back(iteminfo("GUN", _("Damage: "), "", gun_damage(false), true, "", false, false));
        if (has_ammo) {
            temp1.str("");
            temp1 << (ammo_dam >= 0 ? "+" : "" );
            // ammo_damage and sum_of_damage don't need to translate.
            dump->push_back(iteminfo("GUN", "ammo_damage", "",
                                     ammo_dam, true, temp1.str(), false, false, false));
            dump->push_back(iteminfo("GUN", "sum_of_damage", _(" = <num>"),
                                     gun_damage(), true, "", false, false, false));
        }

        //armor-pierce of gun
        dump->push_back(iteminfo("GUN", space + _("Armor-pierce: "), "",
                                 gun_pierce(false), true, "", !has_ammo, false));
        if (has_ammo) {
            temp1.str("");
            temp1 << (ammo_pierce >= 0 ? "+" : "" );
            // ammo_armor_pierce and sum_of_armor_pierce don't need to translate.
            dump->push_back(iteminfo("GUN", "ammo_armor_pierce", "",
                                     ammo_pierce, true, temp1.str(), false, false, false));
            dump->push_back(iteminfo("GUN", "sum_of_armor_pierce", _(" = <num>"),
                                     gun_pierce(), true, "", true, false, false));
        }

        //range of gun
        dump->push_back(iteminfo("GUN", _("Range: "), "", gun->range, true, "", false, false));
        if (has_ammo) {
            temp1.str("");
            temp1 << (ammo_range >= 0 ? "+" : "" );
            // ammo_range and sum_of_range don't need to translate.
            dump->push_back(iteminfo("GUN", "ammo_range", "",
                                     ammo_range, true, temp1.str(), false, false, false));
            dump->push_back(iteminfo("GUN", "sum_of_range", _(" = <num>"),
                                     range(NULL), true, "", false, false, false));
        }

        dump->push_back(iteminfo("GUN", space + _("Dispersion: "), "",
                                 dispersion(), true, "", !has_ammo, true));
        if (has_ammo) {
            temp1.str("");
            temp1 << (ammo_range >= 0 ? "+" : "" );
            // ammo_dispersion and sum_of_dispersion don't need to translate.
            dump->push_back(iteminfo("GUN", "ammo_dispersion", "",
                                     ammo_dispersion, true, temp1.str(), false, false, false));
            dump->push_back(iteminfo("GUN", "sum_of_dispersion", _(" = <num>"),
                                     dispersion() + ammo_dispersion, true, "", true, false, false));
        }

        //recoil of gun
        dump->push_back(iteminfo("GUN", _("Recoil: "), "", recoil(false), true, "", false, true));
        if (has_ammo) {
            temp1.str("");
            temp1 << (ammo_recoil >= 0 ? "+" : "" );
            // ammo_recoil and sum_of_recoil don't need to translate.
            dump->push_back(iteminfo("GUN", "ammo_recoil", "",
                                     ammo_recoil, true, temp1.str(), false, true, false));
            dump->push_back(iteminfo("GUN", "sum_of_recoil", _(" = <num>"),
                                     recoil(), true, "", false, true, false));
        }

        dump->push_back(iteminfo("GUN", space + _("Reload time: "),
                                 ((has_flag("RELOAD_ONE")) ? _("<num> per round") : ""),
                                 gun->reload_time, true, "", true, true));

        if (burst_size() == 0) {
            if (gun->skill_used == Skill::skill("pistol") && has_flag("RELOAD_ONE")) {
                dump->push_back(iteminfo("GUN", _("Revolver.")));
            } else {
                dump->push_back(iteminfo("GUN", _("Semi-automatic.")));
            }
        } else {
            dump->push_back(iteminfo("GUN", _("Burst size: "), "", burst_size()));
        }

        if (!gun->valid_mod_locations.empty()) {
            temp1.str("");
            temp1 << _("Mod Locations:") << "\n";
            int iternum = 0;
            for( std::map<std::string,int>::iterator i = gun->valid_mod_locations.begin();
                 i != gun->valid_mod_locations.end(); i++) {
                if (iternum != 0) {
                    temp1 << "; ";
                }
                const int free_slots = (*i).second - get_free_mod_locations((*i).first);
                temp1 << free_slots << "/" << (*i).second << " " << _((*i).first.c_str());
                bool first_mods = true;
                for (size_t mn = 0; mn < contents.size(); mn++) {
                    it_gunmod* mod = dynamic_cast<it_gunmod*>(contents[mn].type);
                    if (mod->location == (*i).first) {//if mod for this location
                        if (first_mods) {
                            temp1 << ": ";
                            first_mods = false;
                        }else{
                            temp1 << ", ";
                        }
                        temp1 << contents[mn].tname();
                    }
                }
                iternum++;
            }
            temp1 << ".";
            dump->push_back(iteminfo("DESCRIPTION", temp1.str()));
        }

    } else if (is_gunmod()) {
        it_gunmod* mod = dynamic_cast<it_gunmod*>(type);

        if (mod->dispersion != 0) {
            dump->push_back(iteminfo("GUNMOD", _("Dispersion: "), "",
                                     mod->dispersion, true, ((mod->dispersion > 0) ? "+" : "")));
        }
        if (mod->damage != 0) {
            dump->push_back(iteminfo("GUNMOD", _("Damage: "), "", mod->damage, true,
                                     ((mod->damage > 0) ? "+" : "")));
        }
        if (mod->clip != 0)
            dump->push_back(iteminfo("GUNMOD", _("Magazine: "), "<num>%", mod->clip, true,
                                     ((mod->clip > 0) ? "+" : "")));
        if (mod->recoil != 0)
            dump->push_back(iteminfo("GUNMOD", _("Recoil: "), "", mod->recoil, true,
                                     ((mod->recoil > 0) ? "+" : ""), true, true));
        if (mod->burst != 0)
            dump->push_back(iteminfo("GUNMOD", _("Burst: "), "", mod->burst, true,
                                     (mod->burst > 0 ? "+" : "")));

        if (mod->newtype != "NULL") {
            dump->push_back(iteminfo("GUNMOD", _("Ammo: ") + ammo_name(mod->newtype)));
        }

        temp1.str("");
        temp1 << _("Used on: ");
        bool first = true;
        if (mod->used_on_pistol){
            temp1 << _("Pistols");
            first = false;
        }
        if (mod->used_on_shotgun) {
            if (!first) temp1 << ", ";
            temp1 << _("Shotguns");
            first = false;
        }
        if (mod->used_on_smg){
            if (!first) temp1 << ", ";
            temp1 << _("SMGs");
            first = false;
        }
        if (mod->used_on_rifle){
            if (!first) temp1 << ", ";
            temp1 << _("Rifles");
            first = false;
        }

        temp2.str("");
        temp2 << _("Location: ");
        temp2 << _(mod->location.c_str());

        dump->push_back(iteminfo("GUNMOD", temp1.str()));
        dump->push_back(iteminfo("GUNMOD", temp2.str()));

    } else if (is_armor()) {
        it_armor* armor = dynamic_cast<it_armor*>(type);

        temp1.str("");
        temp1 << _("Covers: ");
        if (armor->covers & mfb(bp_head)) {
            temp1 << _("The head. ");
        }
        if (armor->covers & mfb(bp_eyes)) {
            temp1 << _("The eyes. ");
        }
        if (armor->covers & mfb(bp_mouth)) {
            temp1 << _("The mouth. ");
        }
        if (armor->covers & mfb(bp_torso)) {
            temp1 << _("The torso. ");
        }
        if (armor->covers & mfb(bp_arms)) {
            temp1 << _("The arms. ");
        }
        if (armor->covers & mfb(bp_hands)) {
            temp1 << _("The hands. ");
        }
        if (armor->covers & mfb(bp_legs)) {
            temp1 << _("The legs. ");
        }
        if (armor->covers & mfb(bp_feet)) {
            temp1 << _("The feet. ");
        }

        dump->push_back(iteminfo("ARMOR", temp1.str()));
        dump->push_back(iteminfo("ARMOR", _("Coverage: "), "<num>%  ", armor->coverage, true, "", false));
        dump->push_back(iteminfo("ARMOR", _("Warmth: "), "", armor->warmth));
        if (has_flag("FIT")) {
            dump->push_back(iteminfo("ARMOR", _("Encumberment: "), _("<num> (fits)"),
                                     std::max(0, armor->encumber - 1), true, "", true, true));
        } else {
            dump->push_back(iteminfo("ARMOR", _("Encumberment: "), "",
                                     armor->encumber, true, "", true, true));
        }
        dump->push_back(iteminfo("ARMOR", _("Protection: Bash: "), "", bash_resist(), true, "", false));
        dump->push_back(iteminfo("ARMOR", space + _("Cut: "), "", cut_resist(), true, "", true));
        dump->push_back(iteminfo("ARMOR", _("Environmental protection: "), "",
                                 armor->env_resist, true, "", false));
        dump->push_back(iteminfo("ARMOR", space + _("Storage: "), "", armor->storage));

    } else if (is_book()) {

        dump->push_back(iteminfo("DESCRIPTION", "--"));
        it_book* book = dynamic_cast<it_book*>(type);
        if (!book->type) {
            dump->push_back(iteminfo("BOOK", _("Just for fun.")));
        } else {
            dump->push_back(iteminfo("BOOK", "", string_format(_("Can bring your %s skill to <num>"),
                                                              book->type->name().c_str()), book->level));

            if (book->req == 0) {
                dump->push_back(iteminfo("BOOK", _("It can be understood by beginners.")));
            } else {
                dump->push_back(iteminfo("BOOK", "",
                                         string_format(_("Requires %s level <num> to understand."),
                                         book->type->name().c_str()), book->req, true, "", true, true));
            }
        }

        dump->push_back(iteminfo("BOOK", "", _("Requires intelligence of <num> to easily read."),
                                 book->intel, true, "", true, true));
        if (book->fun != 0) {
            dump->push_back(iteminfo("BOOK", "", _("Reading this book affects your morale by <num>"),
                                     book->fun, true, (book->fun > 0 ? "+" : "")));
        }
        dump->push_back(iteminfo("BOOK", "", ngettext("This book takes <num> minute to read.", "This book takes <num> minutes to read.", book->time),
                                 book->time, true, "", true, true));

        if (!(book->recipes.empty())) {
            std::string recipes = "";
            size_t index = 1;
            for (std::map<recipe*, int>::iterator iter = book->recipes.begin();
                 iter != book->recipes.end(); ++iter, ++index) {
                if(g->u.knows_recipe(iter->first)) {
                    recipes += "<color_ltgray>";
                }
                recipes += item_controller->find_template( iter->first->result )->name;
                if(g->u.knows_recipe(iter->first)) {
                    recipes += "</color>";
                }
                if(index == book->recipes.size() - 1) {
                    recipes += _(", and "); // oxford comma 4 lyfe
                } else if(index != book->recipes.size()) {
                    recipes += _(", ");
                }
            }
            std::string recipe_line = string_format(ngettext("This book contains %1$d crafting recipe: %2$s", "This book contains %1$d crafting recipes: %2$s", book->recipes.size()),
                                                    book->recipes.size(), recipes.c_str());
            dump->push_back(iteminfo("DESCRIPTION", "--"));
            dump->push_back(iteminfo("DESCRIPTION", recipe_line.c_str()));
        }

    } else if (is_tool()) {
        it_tool* tool = dynamic_cast<it_tool*>(type);

        if ((tool->max_charges)!=0) {
            std::string charges_line = _("Charges"); //;
            dump->push_back(iteminfo("TOOL",charges_line+ ": " + helper::to_string_int(charges)));

            if (has_flag("DOUBLE_AMMO")) {
                dump->push_back(iteminfo("TOOL", "", ((tool->ammo == "NULL") ?
                    ngettext("Maximum <num> charge (doubled).", "Maximum <num> charges (doubled)", tool->max_charges * 2) :
                    string_format(ngettext("Maximum <num> charge (doubled) of %s.", "Maximum <num> charges (doubled) of %s.", tool->max_charges * 2),
                                  ammo_name(tool->ammo).c_str())), tool->max_charges * 2));
            } else if (has_flag("RECHARGE")) {
                dump->push_back(iteminfo("TOOL", "", ((tool->ammo == "NULL") ?
                    ngettext("Maximum <num> charge (rechargeable).", "Maximum <num> charges (rechargeable).", tool->max_charges) :
                    string_format(ngettext("Maximum <num> charge (rechargeable) of %s", "Maximum <num> charges (rechargeable) of %s.", tool->max_charges),
                    ammo_name(tool->ammo).c_str())), tool->max_charges));
            } else if (has_flag("DOUBLE_AMMO") && has_flag("RECHARGE")) {
                dump->push_back(iteminfo("TOOL", "", ((tool->ammo == "NULL") ?
                    ngettext("Maximum <num> charge (rechargeable) (doubled).", "Maximum <num> charges (rechargeable) (doubled).", tool->max_charges * 2) :
                    string_format(ngettext("Maximum <num> charge (rechargeable) (doubled) of %s.", "Maximum <num> charges (rechargeable) (doubled) of %s.", tool->max_charges * 2),
                                  ammo_name(tool->ammo).c_str())), tool->max_charges * 2));
            } else if (has_flag("ATOMIC_AMMO")) {
                dump->push_back(iteminfo("TOOL", "",
                                         ((tool->ammo == "NULL") ? ngettext("Maximum <num> charge.", "Maximum <num> charges.", tool->max_charges * 100) :
                                          string_format(ngettext("Maximum <num> charge of %s.", "Maximum <num> charges of %s.", tool->max_charges * 100),
                                          ammo_name("plutonium").c_str())), tool->max_charges * 100));
            } else {
                dump->push_back(iteminfo("TOOL", "",
                    ((tool->ammo == "NULL") ? ngettext("Maximum <num> charge.", "Maximum <num> charges.", tool->max_charges) :
                     string_format(ngettext("Maximum <num> charge of %s.", "Maximum <num> charges of %s.", tool->max_charges),
                                   ammo_name(tool->ammo).c_str())), tool->max_charges));
            }
        }
    }

    if (!components.empty()) {
        typedef std::map<std::string, int> t_count_map;
        t_count_map counts;
        for(t_item_vector::const_iterator a = components.begin(); a != components.end(); ++a) {
            const std::string name = const_cast<item&>(*a).display_name();
            if (counts.count(name) > 0) {
                counts[name]++;
            } else {
                counts[name] = 1;
            }
        }
        std::ostringstream buffer;
        buffer << _("Made from: ");
        for(t_count_map::const_iterator a = counts.begin(); a != counts.end(); ++a) {
            if (a != counts.begin()) {
                buffer << _(", ");
            }
            if (a->second != 1) {
                buffer << string_format(_("%d x %s"), a->second, a->first.c_str());
            } else {
                buffer << a->first;
            }
        }
        dump->push_back(iteminfo("DESCRIPTION", buffer.str()));
    }

    if ( !type->qualities.empty()){
        for(std::map<std::string, int>::const_iterator quality = type->qualities.begin();
            quality != type->qualities.end(); ++quality){
            dump->push_back(iteminfo("QUALITIES", "", string_format(_("Has level %1$d %2$s quality."),
                            quality->second, qualities[quality->first].name.c_str())));
        }
    }

    if ( showtext && !is_null() ) {
        const std::map<std::string, std::string>::const_iterator idescription = item_vars.find("description");
        dump->push_back(iteminfo("DESCRIPTION", "--"));
        if (is_stationary()) {
            // Just use the dynamic description
            dump->push_back( iteminfo("DESCRIPTION", SNIPPET.get(note)) );
        } else if (idescription != item_vars.end()) {
            dump->push_back( iteminfo("DESCRIPTION", idescription->second) );
        } else {
            dump->push_back(iteminfo("DESCRIPTION", type->description));
        }

        std::ostringstream tec_buffer;
        for(std::set<std::string>::const_iterator a = type->techniques.begin(); a != type->techniques.end(); ++a) {
            const ma_technique &tec = ma_techniques[*a];
            if (tec.name.empty()) {
                continue;
            }
            if (!tec_buffer.str().empty()) {
                tec_buffer << _(", ");
            }
            tec_buffer << tec.name;
        }
        if (!tec_buffer.str().empty()) {
            dump->push_back(iteminfo("DESCRIPTION", std::string(_("Techniques: ")) + tec_buffer.str()));
        }

        //See shorten version of this in armor_layers.cpp::clothing_flags_description
        if (is_armor() && has_flag("FIT")) {
            dump->push_back(iteminfo("DESCRIPTION", "--"));
            dump->push_back(iteminfo("DESCRIPTION", _("This piece of clothing fits you perfectly.")));
        }
        if (is_armor() && has_flag("SKINTIGHT")) {
            dump->push_back(iteminfo("DESCRIPTION", "--"));
            dump->push_back(iteminfo("DESCRIPTION",
                _("This piece of clothing lies close to the skin and layers easily.")));
        }
        if (is_armor() && has_flag("POCKETS")) {
            dump->push_back(iteminfo("DESCRIPTION", "--"));
            dump->push_back(iteminfo("DESCRIPTION",
                _("This piece of clothing has pockets to warm your hands.")));
        }
        if (is_armor() && has_flag("HOOD")) {
            dump->push_back(iteminfo("DESCRIPTION", "--"));
            dump->push_back(iteminfo("DESCRIPTION",
                _("This piece of clothing has a hood to keep your head warm.")));
        }
        if (is_armor() && has_flag("RAINPROOF")) {
            dump->push_back(iteminfo("DESCRIPTION", "--"));
            dump->push_back(iteminfo("DESCRIPTION",
                _("This piece of clothing is designed to keep you dry in the rain.")));
        }
        if (is_armor() && has_flag("WATER_FRIENDLY")) {
            dump->push_back(iteminfo("DESCRIPTION", "--"));
            dump->push_back(iteminfo("DESCRIPTION",
                _("This piece of clothing performs well even when soaking wet. This can feel good.")));
        }
        if (is_armor() && has_flag("WATERPROOF")) {
            dump->push_back(iteminfo("DESCRIPTION", "--"));
            dump->push_back(iteminfo("DESCRIPTION",
                _("This piece of clothing won't let water through.  Unless you jump in the river or something like that.")));
        }
        if (is_armor() && has_flag("STURDY")) {
            dump->push_back(iteminfo("DESCRIPTION", "--"));
            dump->push_back(iteminfo("DESCRIPTION",
                _("This piece of clothing is designed to protect you from harm and withstand a lot of abuse.")));
        }
        if (is_armor() && has_flag("SWIM_GOGGLES")) {
            dump->push_back(iteminfo("DESCRIPTION", "--"));
            dump->push_back(iteminfo("DESCRIPTION",
                _("This piece of clothing allows you to see much further under water.")));
        }
        if (is_armor() && type->id == "rad_badge") {
            size_t i;
            for( i = 1; i < sizeof(rad_dosage_thresholds) / sizeof(rad_dosage_thresholds[0]); i++ ) {
                if( irridation < rad_dosage_thresholds[i] ) {
                    break;
                }
            }
            dump->push_back(iteminfo("DESCRIPTION",
                string_format(_("The film strip on the badge is %s."),
                              rad_threshold_colors[i - 1].c_str())));
        }
        if (is_tool() && has_flag("DOUBLE_AMMO")) {
            dump->push_back(iteminfo("DESCRIPTION",
                                     _("This tool has double the normal maximum charges.")));
        }
        if (is_tool() && has_flag("ATOMIC_AMMO")) {
            dump->push_back(iteminfo("DESCRIPTION",
                _("This tool has been modified to run off plutonium cells instead of batteries.")));
        }
        if (is_tool() && has_flag("RECHARGE")) {
            dump->push_back(iteminfo("DESCRIPTION",
                _("This tool has been modified to use a rechargeable power cell and is not compatible with standard batteries.")));
        }

        if (has_flag("LEAK_DAM") && has_flag("RADIOACTIVE") && damage > 0) {
            dump->push_back(iteminfo("DESCRIPTION",
                _("The casing of this item has cracked, revealing an ominous green glow.")));
        }

        if (has_flag("LEAK_ALWAYS") && has_flag("RADIOACTIVE")) {
            dump->push_back(iteminfo("DESCRIPTION",
                                     _("This object is surrounded by a sickly green glow.")));
        }

        if (is_food() && has_flag("HIDDEN_POISON") && g->u.skillLevel("survival").level() >= 3) {
            dump->push_back(iteminfo("DESCRIPTION",
                                     _("On closer inspection, this appears to be poisonous.")));
        }

        if (is_food() && has_flag("HIDDEN_HALLU") && g->u.skillLevel("survival").level() >= 5) {
            dump->push_back(iteminfo("DESCRIPTION",
                _("On closer inspection, this appears to be hallucinogenic.")));
        }

        if ((is_food() && has_flag("BREW")) || (is_food_container() && contents[0].has_flag("BREW"))) {
            int btime = ( is_food_container() ) ? contents[0].brewing_time() : brewing_time();
            if (btime <= 28800)
                dump->push_back(iteminfo("DESCRIPTION",
                    string_format(ngettext("Once set in a vat, this will ferment in around %d hour.", "Once set in a vat, this will ferment in around %d hours.", btime / 100),
                                  btime / 600)));
            else {
                btime = 0.5 + btime / 7200; //Round down to 12-hour intervals
                if (btime % 2 == 1) {
                    dump->push_back(iteminfo("DESCRIPTION",
                        string_format(_("Once set in a vat, this will ferment in around %d and a half days."),
                                      btime / 2)));
                } else {
                    dump->push_back(iteminfo("DESCRIPTION",
                        string_format(ngettext("Once set in a vat, this will ferment in around %d day.", "Once set in a vat, this will ferment in around %d days.", btime / 2),
                                      btime / 2)));
                }
            }
        }

        if (typeId() == "flask_yeast") {
            int cult_time = brewing_time();
            dump->push_back(iteminfo("DESCRIPTION",
                string_format(ngettext("It will take %d hour to culture after it's sealed.", "It will take %d hours to culture after it's sealed.", cult_time / 600),
                              cult_time / 600)));
        }

        if ((is_food() && goes_bad()) || (is_food_container() && contents[0].goes_bad())) {
            if(rotten() || (is_food_container() && contents[0].rotten())) {
                if(g->u.has_bionic("bio_digestion")) {
                    dump->push_back(iteminfo("DESCRIPTION",
                        _("This food has started to rot, but your bionic digestion can tolerate it.")));
                } else if(g->u.has_trait("SAPROVORE")) {
                    dump->push_back(iteminfo("DESCRIPTION",
                        _("This food has started to rot, but you can tolerate it.")));
                } else {
                    dump->push_back(iteminfo("DESCRIPTION",
                        _("This food has started to rot. Eating it would be a very bad idea.")));
                }
            } else {
                dump->push_back(iteminfo("DESCRIPTION",
                        _("This food is perishable, and will eventually rot.")));
            }

        }
        std::map<std::string, std::string>::const_iterator item_note = item_vars.find("item_note");
        std::map<std::string, std::string>::const_iterator item_note_type = item_vars.find("item_note_type");

        if ( item_note != item_vars.end() ) {
            dump->push_back(iteminfo("DESCRIPTION", "\n" ));
            std::string ntext = "";
            if ( item_note_type != item_vars.end() ) {
                ntext += string_format(_("%1$s on the %2$s is: "),
                                       item_note_type->second.c_str(), tname().c_str() );
            } else {
                ntext += _("Note: ");
            }
            dump->push_back(iteminfo("DESCRIPTION", ntext + item_note->second ));
        }
        if (!contents.empty()) {
            if (is_gun()) {//Mods description
                for (size_t i = 0; i < contents.size(); i++) {
                    it_gunmod* mod = dynamic_cast<it_gunmod*>(contents[i].type);
                    temp1.str("");
                    temp1 << " " << contents[i].tname() << " (" << _(mod->location.c_str()) << ")";
                    dump->push_back(iteminfo("DESCRIPTION", temp1.str()));
                    dump->push_back(iteminfo("DESCRIPTION", contents[i].type->description));
                }
            } else {
                dump->push_back(iteminfo("DESCRIPTION", contents[0].type->description));
            }
        }
    }

    temp1.str("");
    std::vector<iteminfo>& vecData = *dump; // vector is not copied here
    for (size_t i = 0; i < vecData.size(); i++) {
        if (vecData[i].sType == "DESCRIPTION") {
            temp1 << "\n";
        }

        if (vecData[i].bDrawName) {
            temp1 << vecData[i].sName;
        }
        size_t pos = vecData[i].sFmt.find("<num>");
        std::string sPost = "";
        if(pos != std::string::npos) {
            temp1 << vecData[i].sFmt.substr(0, pos);
            sPost = vecData[i].sFmt.substr(pos+5);
        } else {
            temp1 << vecData[i].sFmt.c_str();
        }
        if (vecData[i].sValue != "-999") {
            temp1 << vecData[i].sPlus << vecData[i].sValue;
        }
        temp1 << sPost;
        temp1 << ((vecData[i].bNewLine) ? "\n" : "");
    }

    return temp1.str();
}

int item::get_free_mod_locations(const std::string &location) const
{
    if(!is_gun()) {
        return 0;
    }
    const it_gun *gt = dynamic_cast<const it_gun*>(type);
    std::map<std::string, int>::const_iterator loc =
        gt->valid_mod_locations.find(location);
    if(loc == gt->valid_mod_locations.end()) {
        return 0;
    }
    int result = loc->second;
    for(std::vector<item>::const_iterator a = contents.begin(); a != contents.end(); ++a) {
        const it_gunmod *mod = dynamic_cast<const it_gunmod*>(a->type);
        if(mod != NULL && mod->location == location) {
            result--;
        }
    }
    return result;
}

char item::symbol() const
{
    if( is_null() )
        return ' ';
    return type->sym;
}

nc_color item::color(player *u) const
{
    nc_color ret = c_ltgray;

    if(has_flag("WET")) {
        ret = c_cyan;
    } else if(has_flag("LITCIG")) {
        ret = c_red;
    } else if (active && !is_food() && !is_food_container()) { // Active items show up as yellow
        ret = c_yellow;
    } else if (is_gun()) { // Guns are green if you are carrying ammo for them
        ammotype amtype = ammo_type();
        if (u->has_ammo(amtype).size() > 0)
            ret = c_green;
    } else if (is_ammo()) { // Likewise, ammo is green if you have guns that use it
        ammotype amtype = ammo_type();
        if (u->weapon.is_gun() && u->weapon.ammo_type() == amtype) {
            ret = c_green;
        } else {
            if (u->inv.has_gun_for_ammo(amtype)) {
                ret = c_green;
            }
        }
    } else if (is_book()) {
        it_book* tmp = dynamic_cast<it_book*>(type);
        if (tmp->type && tmp->intel <= u->int_cur + u->skillLevel(tmp->type) &&
                (tmp->intel == 0 || !u->has_trait("ILLITERATE")) &&
                (u->skillLevel(tmp->type) >= (int)tmp->req) &&
                (u->skillLevel(tmp->type) < (int)tmp->level)) {
            ret = c_ltblue;
        } else if (!u->studied_all_recipes(tmp) && !u->has_trait("ILLITERATE")) {
          ret = c_yellow;
        }
    }
    return ret;
}

nc_color item::color_in_inventory()
{
    // This should be relevant only for the player,
    // npcs don't care about the color
    return color(&g->u);
}

/* @param with_prefix determines whether to return for more of its object, such as
* the extent of damage and burning (was created to sort by name without prefix
* in additional inventory)
* @return name of item
*/
std::string item::tname( unsigned int quantity, bool with_prefix )
{
    std::stringstream ret;

// MATERIALS-TODO: put this in json
    std::string damtext = "";
    if (damage != 0 && !is_null() && with_prefix) {
        if (damage == -1)  {
          if (is_gun())  {
            damtext = rm_prefix(_("<dam_adj>accurized "));
          } else {
              damtext = rm_prefix(_("<dam_adj>reinforced "));
            }
        } else {
            if (type->id == "corpse") {
                if (damage == 1) damtext = rm_prefix(_("<dam_adj>bruised "));
                if (damage == 2) damtext = rm_prefix(_("<dam_adj>damaged "));
                if (damage == 3) damtext = rm_prefix(_("<dam_adj>mangled "));
                if (damage == 4) damtext = rm_prefix(_("<dam_adj>pulped "));
            } else {
                damtext = rmp_format("%s ", type->dmg_adj(damage).c_str());
            }
            }
    }

    std::string vehtext = "";
    if (is_var_veh_part()) {
        if(type->bigness_aspect == BIGNESS_ENGINE_DISPLACEMENT) {
            ret.str("");
            ret.precision(4);
            ret << (float)bigness/100;
            //~ liters, e.g. 3.21-Liter V8 engine
            vehtext = rmp_format(_("<veh_adj>%s-Liter "), ret.str().c_str());
        }
        else if(type->bigness_aspect == BIGNESS_WHEEL_DIAMETER) {
            //~ inches, e.g. 20" wheel
            vehtext = rmp_format(_("<veh_adj>%d\" "), bigness);
        }
    }

    std::string burntext = "";
    if (with_prefix) {
        if (volume() >= 4 && burnt >= volume() * 2) {
            burntext = rm_prefix(_("<burnt_adj>badly burnt "));
        } else if (burnt > 0) {
            burntext = rm_prefix(_("<burnt_adj>burnt "));
        }
    }

    const std::map<std::string, std::string>::const_iterator iname = item_vars.find("name");
    std::string maintext = "";
    if (corpse != NULL && typeId() == "corpse" ) {
        if (name != "") {
            maintext = rmp_format(ngettext("<item_name>%s corpse of %s",
                                           "<item_name>%s corpses of %s",
                                           quantity), corpse->name.c_str(), name.c_str());
        } else {
            maintext = rmp_format(ngettext("<item_name>%s corpse",
                                           "<item_name>%s corpses",
                                           quantity), corpse->name.c_str());
        }
    } else if (typeId() == "blood") {
        if (corpse == NULL || corpse->id == "mon_null")
            maintext = rm_prefix(ngettext("<item_name>human blood",
                                          "<item_name>human blood",
                                          quantity));
        else
            maintext = rmp_format(ngettext("<item_name>%s blood",
                                           "<item_name>%s blood",
                                           quantity), corpse->name.c_str());
    }
    else if (iname != item_vars.end()) {
        maintext = iname->second;
    }
    else if (is_gun() && !contents.empty() ) {
        ret.str("");
        ret << type->nname(quantity);
        for (size_t i = 0; i < contents.size(); i++) {
            ret << "+";
        }
        maintext = ret.str();
    } else if (contents.size() == 1) {
        if(contents[0].made_of(LIQUID)) {
            maintext = rmp_format(_("<item_name>%s of %s"), type->nname(quantity).c_str(), contents[0].tname().c_str());
        } else if (contents[0].is_food()) {
            maintext = contents[0].charges > 1 ? rmp_format(_("<item_name>%s of %s"), type->nname(quantity).c_str(),
                                                            contents[0].tname(contents[0].charges).c_str()) :
                                                 rmp_format(_("<item_name>%s of %s"), type->nname(quantity).c_str(),
                                                            contents[0].tname().c_str());
        } else {
            maintext = rmp_format(_("<item_name>%s with %s"), type->nname(quantity).c_str(), contents[0].tname().c_str());
        }
    }
    else if (!contents.empty()) {
        maintext = rmp_format(_("<item_name>%s, full"), type->nname(quantity).c_str());
    } else {
        maintext = type->nname(quantity);
    }

    item* food = NULL;
    it_comest* food_type = NULL;
    std::string tagtext = "";
    std::string toolmodtext = "";
    ret.str("");
    if (is_food())
    {
        food = this;
        food_type = dynamic_cast<it_comest*>(type);

        if (food_type->spoils != 0)
        {
            if(food->rotten()) {
                ret << _(" (rotten)");
            } else if ( rot < 100 ) {
                ret << _(" (fresh)");
            }
        }
        if (food->has_flag("HOT"))
            ret << _(" (hot)");
    }

    if (has_flag("FIT")) {
        ret << _(" (fits)");
    }

    if (has_flag("RECHARGE")) {
        ret << _(" (rechargeable)");
    }

    if (has_flag("ATOMIC_AMMO")) {
        toolmodtext = _("atomic ");
    }

    if (owned > 0)
        ret << _(" (owned)");

    if(has_flag("WET"))
       ret << _(" (wet)");

    if(has_flag("LITCIG"))
        ret << _(" (lit)");

    tagtext = ret.str();

    ret.str("");

    //~ This is a string to construct the item name as it is displayed. This format string has been added for maximum flexibility. The strings are: %1$s: Damage text (eg. “bruised”. %2$s: burn adjectives (eg. “burnt”). %3$s: tool modifier text (eg. “atomic”). %4$s: vehicle part text (eg. “3.8-Liter”. $5$s: main item text (eg. “apple”), %6$s: tags (eg. “ (wet) (fits)”).
    ret << string_format(_("%1$s%2$s%3$s%4$s%5$s%6$s"), damtext.c_str(), burntext.c_str(),
                         toolmodtext.c_str(), vehtext.c_str(), maintext.c_str(), tagtext.c_str());

    static const std::string const_str_item_note("item_note");
    if( item_vars.find(const_str_item_note) != item_vars.end() ) {
        //~ %s is an item name. This style is used to denote items with notes.
        return string_format(_("*%s*"), ret.str().c_str());
    } else {
        return ret.str();
    }
}

std::string item::display_name(unsigned int quantity)
{
    // Show count of contents (e.g. amount of liquid in container)
    // or usages remaining, even if 0 (e.g. uses remaining in charcoal smoker).
    if (contents.size() == 1 && contents[0].charges > 0) {
        return string_format("%s (%d)", tname(quantity).c_str(), contents[0].charges);
    } else if (charges >= 0 && !has_flag("NO_AMMO")) {
        return string_format("%s (%d)", tname(quantity).c_str(), charges);
    }
    else {
        return tname(quantity);
    }
}

nc_color item::color() const
{
    if( is_null() )
        return c_black;
    if ( corpse != NULL && typeId() == "corpse" ) {
        return corpse->color;
    }
    return type->color;
}

int item::price() const
{
    if( is_null() ) {
        return 0;
    }

    int ret = type->price;
    for (size_t i = 0; i < contents.size(); i++) {
        ret += contents[i].price();
    }
    return ret;
}

// MATERIALS-TODO: add a density field to materials.json
int item::weight() const
{
    if (corpse != NULL && typeId() == "corpse" ) {
        int ret = 0;
        switch (corpse->size) {
            case MS_TINY:   ret =   1000;  break;
            case MS_SMALL:  ret =  40750;  break;
            case MS_MEDIUM: ret =  81500;  break;
            case MS_LARGE:  ret = 120000;  break;
            case MS_HUGE:   ret = 200000;  break;
        }
        if (made_of("veggy")) {
            ret /= 3;
        }
        if(corpse->in_species("FISH") || corpse->in_species("BIRD") || corpse->in_species("INSECT") || made_of("bone")) {
            ret /= 8;
        } else if (made_of("iron") || made_of("steel") || made_of("stone")) {
            ret *= 7;
        }
        return ret;
    }

    if( is_null() ) {
        return 0;
    }

    int ret = type->weight;

    const std::map<std::string, std::string>::const_iterator iweight = item_vars.find("weight");
    if (iweight != item_vars.end()) {
        char *dummy;
        ret = strtol(iweight->second.c_str(), &dummy, 10);
    }

    if (count_by_charges()) {
        ret *= charges;
    } else if (type->is_gun() && charges >= 1) {
        ret += curammo->weight * charges;
    } else if (type->is_tool() && charges >= 1 && ammo_type() != "NULL") {
        if (typeId() == "adv_UPS_off" || typeId() == "adv_UPS_on" || has_flag("ATOMIC_AMMO") ||
            typeId() == "rm13_armor" || typeId() == "rm13_armor_on") {
            ret += item_controller->find_template(default_ammo(this->ammo_type()))->weight * charges / 500;
        } else {
            ret += item_controller->find_template(default_ammo(this->ammo_type()))->weight * charges;
        }
    }
    for (size_t i = 0; i < contents.size(); i++) {
        ret += contents[i].weight();
        if (contents[i].is_gunmod() && contents[i].charges >= 1) {
            ret += contents[i].curammo->weight * contents[i].charges;
        }
    }

// tool mods also add about a pound of weight
    if (has_flag("ATOMIC_AMMO")) {
        ret += 250;
    }

    return ret;
}

/*
 * precise_unit_volume: Returns the volume, multiplied by 1000.
 * 1: -except- ammo, since the game treats the volume of count_by_charge items as 1/stack_size of the volume defined in .json
 * 2: Ammo is also not totalled.
 * 3: gun mods -are- added to the total, since a modded gun is not a splittable thing, in an inventory sense
 * This allows one to obtain the volume of something consistent with game rules, with a precision that is lost
 * when a 2 volume bullet is divided by ???? and returned as an int.
 */
int item::precise_unit_volume() const
{
   return volume(true, true);
}

/*
 * note, the game currently has an undefined number of different scales of volume: items that are count_by_charges, and
 * everything else:
 *    everything else: volume = type->volume
 *   count_by_charges: volume = type->volume / stack_size
 * Also, this function will multiply count_by_charges items by the amount of charges before dividing by ???.
 * If you need more precision, precise_value = true will return a multiple of 1000
 * If you want to handle counting up charges elsewhere, unit value = true will skip that part,
 *   except for guns.
 * Default values are unit_value=false, precise_value=false
 */
int item::volume(bool unit_value, bool precise_value ) const
{
    int ret = 0;
    if (corpse != NULL && typeId() == "corpse" ) {
        switch (corpse->size) {
            case MS_TINY:
                ret = 2;
                break;
            case MS_SMALL:
                ret = 40;
                break;
            case MS_MEDIUM:
                ret = 75;
                break;
            case MS_LARGE:
                ret = 160;
                break;
            case MS_HUGE:
                ret = 600;
                break;
        }
        if ( precise_value == true ) {
            ret *= 1000;
        }
        return ret;
    }

    if( is_null()) {
        return 0;
    }

    ret = type->volume;
    const std::map<std::string, std::string>::const_iterator ivolume = item_vars.find("volume");
    if (ivolume != item_vars.end()) {
        char *dummy;
        ret = strtol(ivolume->second.c_str(), &dummy, 10);
    }

    if ( precise_value == true ) {
        ret *= 1000;
    }

    static const std::string RIGID_FLAG("RIGID");
    if (is_container() && !has_flag(RIGID_FLAG)) {
        // non-rigid container add the volume of the content
        int tmpvol = 0;
        for (size_t i = 0; i < contents.size(); i++) {
            tmpvol += contents[i].volume(false, true);
        }
        if (!precise_value) {
            tmpvol /= 1000;
        }
        ret += tmpvol;
    }

    if (count_by_charges() || made_of(LIQUID)) {
        if ( unit_value == false ) {
            ret *= charges;
        }
        ret /= max_charges();
    }

    if (is_gun()) {
        for (size_t i = 0; i < contents.size(); i++) {
            ret += contents[i].volume( false, precise_value );
        }
    }

// tool mods also add volume
    if (has_flag("ATOMIC_AMMO")) {
        ret += 1;
    }

    return ret;
}

int item::volume_contained()
{
    int ret = 0;
    for (size_t i = 0; i < contents.size(); i++)
        ret += contents[i].volume();
    return ret;
}

int item::attack_time()
{
    int ret = 65 + 4 * volume() + weight() / 60;
    return ret;
}

int item::damage_bash()
{
    int total = type->melee_dam;
    if( is_null() )
        return 0;
      total -= total * (damage * 0.1);
      if (total > 0) {
      return total;
      } else {
         return 0;
        }
}

int item::damage_cut() const
{
    int total = type->melee_cut;
    if (is_gun()) {
        for (size_t i = 0; i < contents.size(); i++) {
            if (contents[i].typeId() == "bayonet" || "pistol_bayonet"|| "sword_bayonet")
                return contents[i].type->melee_cut;
        }
    }
    if( is_null() )
        return 0;
    total -= total * (damage * 0.1);
      if (total > 0) {
      return total;
      } else {
         return 0;
        }
}

bool item::has_flag(const std::string &f) const
{
    bool ret = false;

    // first check for flags specific to item type
    // gun flags
    if (is_gun()) {
        if (mode == "MODE_AUX") {
            item const* gunmod = inspect_active_gunmod();
            if( gunmod != NULL )
                ret = gunmod->has_flag(f);
            if (ret) return ret;
        } else {
            for (size_t i = 0; i < contents.size(); i++) {
                // Don't report flags from active gunmods for the gun.
                if (contents[i].has_flag(f) && !contents[i].has_flag("MODE_AUX")) {
                    ret = true;
                    return ret;
                }
            }
        }
    }
    // other item type flags
    ret = type->item_tags.count(f);
    if (ret) {
        return ret;
    }

    // now check for item specific flags
    ret = item_tags.count(f);
    return ret;
}

bool item::contains_with_flag(std::string f) const
{
    bool ret = false;
    for (int k = 0; k < contents.size(); k++) {
        ret = contents[k].has_flag(f);
        if (ret) return ret;
    }
    return ret;
}

bool item::has_quality(std::string quality_id) const {
    return has_quality(quality_id, 1);
}

bool item::has_quality(std::string quality_id, int quality_value) const {
    bool ret = false;
    if ( !type->qualities.empty()){
        for(std::map<std::string, int>::const_iterator quality = type->qualities.begin(); quality != type->qualities.end(); ++quality){
            if(quality->first == quality_id && quality->second >= quality_value) {
                ret = true;
                break;
            }
        }
    }
    return ret;
}

bool item::has_technique(matec_id tech)
{
    return type->techniques.count(tech);
}

int item::has_gunmod(itype_id mod_type)
{
    if (!is_gun())
        return -1;
    for (size_t i = 0; i < contents.size(); i++)
        if (contents[i].is_gunmod() && contents[i].typeId() == mod_type)
            return i;
    return -1;
}

bool item::rotten()
{
    if (!is_food() || g == NULL)
        return false;
    it_comest* food = dynamic_cast<it_comest*>(type);
    if (food->spoils != 0) {
      calc_rot();

      return (rot > (signed int)food->spoils * 600);
    } else {
      return false;
    }
}

void item::calc_rot()
{
    const int now = calendar::turn;
    if ( last_rot_check + 10 < now ) {
        const int since = ( last_rot_check == 0 ? bday : last_rot_check );
        const int until = ( fridge > 0 ? fridge : now );
        if ( since < until ) {
            // rot (outside of fridge) from bday/last_rot_check until fridge/now
            int old = rot;
            rot += get_rot_since( since, until );
            if (g->debugmon) {
                add_msg(m_info, "r: %s %d,%d %d->%d", type->id.c_str(), since, until, old, rot );
            }
        }
        last_rot_check = now;

        if (fridge > 0) {
            // Flat 20%, rot from time of putting it into fridge up to now
            rot += (now - fridge) * 0.2;
            fridge = 0;
        }
    }
}

int item::brewing_time()
{
    float season_mult = ( (float)ACTIVE_WORLD_OPTIONS["SEASON_LENGTH"] ) / 14;
    if (typeId() == "flask_yeast")
        return 7200 * season_mult;
    unsigned int b_time = dynamic_cast<it_comest*>(type)->brewtime;
    int ret = b_time * season_mult;
    return ret;
}

bool item::can_revive()
{
    if ( corpse != NULL && corpse->has_flag(MF_REVIVES) && damage < 4) {
        return true;
    }
    return false;
}

bool item::ready_to_revive()
{
    if(can_revive() == false) {
        return false;
    }
    int age_in_hours = (int(calendar::turn) - bday) / (10 * 60);
    age_in_hours -= int((float)burnt / volume() * 24);
    if (damage > 0)
    {
        age_in_hours /= (damage + 1);
    }
    int rez_factor = 48 - age_in_hours;
    if (age_in_hours > 6 && (rez_factor <= 0 || one_in(rez_factor)))
    {
        return true;
    }
    return false;
}

bool item::goes_bad()
{
    if (!is_food())
        return false;
    it_comest* food = dynamic_cast<it_comest*>(type);
    return (food->spoils != 0);
}

bool item::count_by_charges() const
{
    if (is_ammo())
        return true;
    if (is_food()) {
        it_comest* food = dynamic_cast<it_comest*>(type);
        return (food->charges > 1);
    }
    return false;
}

long item::max_charges() const
{
    if(count_by_charges()) {
        return type->stack_size;
    } else {
        return 1;
    }
}

bool item::craft_has_charges()
{
    if (count_by_charges())
        return true;
    else if (ammo_type() == "NULL")
        return true;

    return false;
}

long item::num_charges()
{
    if (is_gun()) {
        if (mode == "MODE_AUX") {
            item* gunmod = active_gunmod();
            if (gunmod != NULL)
                return gunmod->charges;
        } else {
            return charges;
        }
    }
    if (is_gunmod() && mode == "MODE_AUX")
        return charges;
    return 0;
}

int item::weapon_value(player *p) const
{
 if( is_null() )
  return 0;

 int my_value = 0;
 if (is_gun()) {
  int gun_value = 14;
  it_gun* gun = dynamic_cast<it_gun*>(type);
  gun_value += gun->dmg_bonus;
  gun_value += int(gun->burst / 2);
  gun_value += int(gun->clip / 3);
  gun_value -= int(gun->dispersion / 5);
  gun_value *= (.5 + (.3 * p->skillLevel("gun")));
  gun_value *= (.3 + (.7 * p->skillLevel(gun->skill_used)));
  my_value += gun_value;
 }

 my_value += int(type->melee_dam * (1   + .3 * p->skillLevel("bashing") +
                                          .1 * p->skillLevel("melee")    ));

 my_value += int(type->melee_cut * (1   + .4 * p->skillLevel("cutting") +
                                          .1 * p->skillLevel("melee")    ));

 my_value += int(type->m_to_hit  * (1.2 + .3 * p->skillLevel("melee")));

 return my_value;
}

int item::melee_value(player *p)
{
 if( is_null() )
  return 0;

 int my_value = 0;
 my_value += int(type->melee_dam * (1   + .3 * p->skillLevel("bashing") +
                                          .1 * p->skillLevel("melee")    ));

 my_value += int(type->melee_cut * (1   + .4 * p->skillLevel("cutting") +
                                          .1 * p->skillLevel("melee")    ));

 my_value += int(type->m_to_hit  * (1.2 + .3 * p->skillLevel("melee")));

 return my_value;
}

int item::bash_resist() const
{
    int ret = 0;

    if (is_null())
        return 0;

    const material_type* cur_mat1 = get_material(1);
    const material_type* cur_mat2 = get_material(2);
    if (is_armor())
    {
        // base resistance
        it_armor* tmp = dynamic_cast<it_armor*>(type);
        int eff_thickness = ((tmp->thickness - damage <= 0) ? 1 : (tmp->thickness - damage));

        // assumes weighted sum of materials for items with 2 materials, 66% material 1 and 33% material 2
        if (cur_mat2->is_null())
        {
            ret = eff_thickness * (3 * cur_mat1->bash_resist());
        }
        else
        {
            ret = eff_thickness * (cur_mat1->bash_resist() + cur_mat1->bash_resist() + cur_mat2->bash_resist());
        }
    }
    else // for non-armor, just bash_resist
    {
        if (cur_mat2->is_null())
        {
            ret = 3 * cur_mat1->bash_resist();
        }
        else
        {
            ret = cur_mat1->bash_resist() + cur_mat1->bash_resist() + cur_mat2->bash_resist();
        }
    }
    ret = ret * 0.5; //halve armour values for balance reasons
    return ret;
}

int item::cut_resist() const
{
    int ret = 0;

    if (is_null())
        return 0;

    const material_type* cur_mat1 = get_material(1);
    const material_type* cur_mat2 = get_material(2);
    if (is_armor())
        {
        it_armor* tmp = dynamic_cast<it_armor*>(type);
        int eff_thickness = ((tmp->thickness - damage <= 0) ? 1 : (tmp->thickness - damage));

        // assumes weighted sum of materials for items with 2 materials, 66% material 1 and 33% material 2
        if (cur_mat2->is_null())
        {
            ret = eff_thickness * (3 * cur_mat1->cut_resist());

        }
        else
        {
            ret = eff_thickness * (cur_mat1->cut_resist() + cur_mat1->cut_resist() + cur_mat2->cut_resist());
        }
    }
    else // for non-armor
    {
        if (cur_mat2->is_null())
        {
            ret = 3 * cur_mat1->cut_resist();
        }
        else
        {
            ret = cur_mat1->cut_resist() + cur_mat1->cut_resist() + cur_mat2->cut_resist();
        }
    }
    ret = ret * 0.5; //halve armour values for balance reasons
    return ret;
}

int item::acid_resist() const
{
    int ret = 0;

    if (is_null())
        return 0;

    // similar weighted sum of acid resistances
    const material_type* cur_mat1 = get_material(1);
    const material_type* cur_mat2 = get_material(2);
    if (cur_mat2->is_null())
    {
        ret = 3 * cur_mat1->acid_resist();
    }
    else
    {
        ret = cur_mat1->acid_resist() + cur_mat1->acid_resist() + cur_mat2->acid_resist();
    }

    return ret;
}

bool item::is_two_handed(player *u)
{
    if (has_flag("ALWAYS_TWOHAND"))
    {
        return true;
    }
    return ((weight() / 113) > u->str_cur * 4);
}

bool item::made_of(std::string mat_ident) const
{
    if( is_null() )
        return false;

    if (corpse != NULL && typeId() == "corpse" )
        return (corpse->mat == mat_ident);

    return (type->m1 == mat_ident || type->m2 == mat_ident);
}

const material_type *item::get_material(int m) const
{
    if (is_null() || m < 1 || m > 2) {
        return material_type::base_material();
    }

    if (corpse != NULL && typeId() == "corpse" ) {
        if (m == 1) {
            return material_type::find_material(corpse->mat);
        }
        // corpses have only one material
        return material_type::base_material();
    }

    return material_type::find_material(m == 1 ? type->m1 : type->m2);
}

bool item::made_of(phase_id phase) const
{
    if( is_null() )
        return false;

    return (type->phase == phase);
}

bool item::conductive() const
{
    if( is_null() )
        return false;

    const material_type* cur_mat1 = get_material(1);
    const material_type* cur_mat2 = get_material(2);

    //Looks ugly, but prevents "null" 2nd material from making weapon conductive
    //Unsure if there is a better way to determine 2nd material is "null"
    return (cur_mat1->elec_resist() <= 0 || (!cur_mat2->is_null() &&
                                             cur_mat2->elec_resist() <= 0));
}

bool item::destroyed_at_zero_charges()
{
    return (is_ammo() || is_food());
}

bool item::is_var_veh_part() const
{
    if( is_null() )
        return false;

    return type->is_var_veh_part();
}

bool item::is_gun() const
{
    if( is_null() )
        return false;

    return type->is_gun();
}

bool item::is_silent() const
{
 if ( is_null() )
  return false;

 // So far only gun code uses this check
 return type->is_gun() && (
   noise() < 5 ||              // almost silent
   curammo->type == "bolt" || // crossbows
   curammo->type == "arrow" ||// bows
   curammo->type == "pebble" ||// sling[shot]
   curammo->type == "fishspear" ||// speargun spears
   curammo->type == "dart"     // blowguns and such
 );
}

bool item::is_gunmod() const
{
    if( is_null() )
        return false;

    return type->is_gunmod();
}

bool item::is_bionic() const
{
    if( is_null() )
        return false;

    return type->is_bionic();
}

bool item::is_ammo() const
{
    if( is_null() )
        return false;

    return type->is_ammo();
}

bool item::is_food(player const*u) const
{
    if (!u)
        return is_food();

    if( is_null() )
        return false;

    if (type->is_food())
        return true;

    if (u->has_active_bionic("bio_batteries") && is_ammo() &&
            (dynamic_cast<it_ammo*>(type))->type == "battery")
        return true;
    if (u->has_active_bionic("bio_furnace") && flammable() && typeId() != "corpse")
        return true;
    return false;
}

bool item::is_food_container(player const*u) const
{
    return (contents.size() >= 1 && contents[0].is_food(u));
}

bool is_edible(item i, player const*u)
{
    return (i.is_food(u) || i.is_food_container(u));
}

bool item::is_food() const
{
    if( is_null() )
        return false;

    if (type->is_food())
        return true;
    return false;
}

bool item::is_food_container() const
{
    return (contents.size() >= 1 && contents[0].is_food());
}

bool item::is_corpse() const
{
    if( is_null() ) {
        return false;
    }

    if (type->id == "corpse") {
        return true;
    }
    return false;
}

bool item::is_ammo_container() const
{
    return (contents.size() >= 1 && contents[0].is_ammo());
}

bool item::is_drink() const
{
    if( is_null() )
        return false;

    return type->is_food() && type->phase == LIQUID;
}

bool item::is_weap() const
{
    if( is_null() )
        return false;

    if (is_gun() || is_food() || is_ammo() || is_food_container() || is_armor() ||
            is_book() || is_tool() || is_bionic() || is_gunmod())
        return false;
    return (type->melee_dam > 7 || type->melee_cut > 5);
}

bool item::is_bashing_weapon() const
{
    if( is_null() )
        return false;

    return (type->melee_dam >= 8);
}

bool item::is_cutting_weapon() const
{
    if( is_null() )
        return false;

    return (type->melee_cut >= 8 && !has_flag("SPEAR"));
}

bool item::is_armor() const
{
    if( is_null() )
        return false;

    return type->is_armor();
}

bool item::is_book() const
{
    if( is_null() )
        return false;

    return type->is_book();
}

bool item::is_container() const
{
    if( is_null() )
        return false;

    return type->is_container();
}

bool item::is_watertight_container() const
{
    return ( is_container() != false && has_flag("WATERTIGHT") && has_flag("SEALS") );
}

bool item::is_funnel_container(unsigned int &bigger_than) const
{
    if ( ! is_watertight_container() ) {
        return false;
    }
    it_container *ct = dynamic_cast<it_container *>(type);
    // todo; consider linking funnel to item or -making- it an active item
    if ( ct->contains <= bigger_than ) {
        return false; // skip contents check, performance
    }
    if (
        contents.empty() ||
        contents[0].typeId() == "water" ||
        contents[0].typeId() == "water_acid" ||
        contents[0].typeId() == "water_acid_weak") {
        bigger_than = ct->contains;
        return true;
    }
    return false;
}

bool item::is_tool() const
{
    if( is_null() )
        return false;

    return type->is_tool();
}

bool item::is_software() const
{
    if( is_null() )
        return false;

    return type->is_software();
}

bool item::is_macguffin() const
{
    if( is_null() )
        return false;

    return type->is_macguffin();
}

bool item::is_stationary() const
{
    if( is_null() )
        return false;

    return type->is_stationary();
}

bool item::is_other() const
{
    if( is_null() )
        return false;

    return (!is_gun() && !is_ammo() && !is_armor() && !is_food() &&
            !is_food_container() && !is_tool() && !is_gunmod() && !is_bionic() &&
            !is_book() && !is_weap());
}

bool item::is_artifact() const
{
    if( is_null() )
        return false;

    return type->is_artifact();
}

bool item::operator<(const item& other) const
{
    const item_category &cat_a = get_category();
    const item_category &cat_b = other.get_category();
    if(cat_a != cat_b) {
        return cat_a < cat_b;
    } else {
        const item *me = is_container() && !contents.empty() ? &contents[0] : this;
        const item *rhs = other.is_container() && !other.contents.empty() ? &other.contents[0] : &other;

        if (me->type->id == rhs->type->id)
        {
            return me->charges < rhs->charges;
        }
        else
        {
            return me->type->id < rhs->type->id;
        }
    }
}

int item::reload_time(player &u)
{
    int ret = 0;

    if (is_gun()) {
        it_gun* reloading = dynamic_cast<it_gun*>(type);
        ret = reloading->reload_time;
        if (charges == 0) {
            int spare_mag = has_gunmod("spare_mag");
            if (spare_mag != -1 && contents[spare_mag].charges > 0)
                ret -= int(double(ret) * 0.9);
        }
        double skill_bonus = double(u.skillLevel(reloading->skill_used)) * .075;
        if (skill_bonus > .75)
            skill_bonus = .75;
        ret -= int(double(ret) * skill_bonus);
    } else if (is_tool())
        ret = 100 + volume() + (weight() / 113);

    if (has_flag("STR_RELOAD"))
        ret -= u.str_cur * 20;
    if (ret < 25)
        ret = 25;
    ret += u.encumb(bp_hands) * 30;
    return ret;
}

item* item::active_gunmod()
{
    if( mode == "MODE_AUX" )
        for (size_t i = 0; i < contents.size(); i++)
            if (contents[i].is_gunmod() && contents[i].mode == "MODE_AUX")
                return &contents[i];
    return NULL;
}

item const* item::inspect_active_gunmod() const
{
    if (mode == "MODE_AUX")
    {
        for (size_t i = 0; i < contents.size(); ++i)
        {
            if (contents[i].is_gunmod() && contents[i].mode == "MODE_AUX")
            {
                return &contents[i];
            }
        }
    }
    return NULL;
}

void item::next_mode()
{
    if( mode == "NULL" && has_flag("MODE_BURST") )
    {
        mode = "MODE_BURST";
    }
    else if( mode == "NULL" || mode == "MODE_BURST" )
    {
        // mode is MODE_BURST, or item has no MODE_BURST flag and mode is NULL
        // Enable the first mod with an AUX firing mode.
        for (size_t i = 0; i < contents.size(); i++)
        {
            if (contents[i].is_gunmod() && contents[i].has_flag("MODE_AUX"))
            {
                mode = "MODE_AUX";
                contents[i].mode = "MODE_AUX";
                return;
            }
        }
        mode = "NULL";
    }
    else if( mode == "MODE_AUX")
    {
        size_t i = 0;
        // Advance to next aux mode, or if there isn't one, normal mode
        for (; i < contents.size(); i++)
        {
            if (contents[i].is_gunmod() && contents[i].mode == "MODE_AUX")
            {
                contents[i].mode = "NULL";
                break;
            }
        }
        for (i++; i < contents.size(); i++)
        {
            if (contents[i].is_gunmod() && contents[i].has_flag("MODE_AUX"))
            {
                contents[i].mode = "MODE_AUX";
                break;
            }
        }
        if (i == contents.size())
        {
            mode = "NULL";
        }
    }
}

int item::clip_size()
{
    if(is_gunmod() && has_flag("MODE_AUX"))
        return (dynamic_cast<it_gunmod*>(type))->clip;
    if (!is_gun())
        return 0;

    it_gun* gun = dynamic_cast<it_gun*>(type);
    int ret = gun->clip;
    for (size_t i = 0; i < contents.size(); i++) {
        if (contents[i].is_gunmod() && !contents[i].has_flag("MODE_AUX")) {
            int bonus = (ret * (dynamic_cast<it_gunmod*>(contents[i].type))->clip) / 100;
            ret = int(ret + bonus);
        }
    }
    return ret;
}

int item::dispersion()
{
    if (!is_gun())
        return 0;
    it_gun* gun = dynamic_cast<it_gun*>(type);
    int ret = gun->dispersion;
    for (size_t i = 0; i < contents.size(); i++) {
        if (contents[i].is_gunmod())
            ret += (dynamic_cast<it_gunmod*>(contents[i].type))->dispersion;
    }
    ret += damage * 4;
    if (ret < 0) ret = 0;
    return ret;
}

int item::gun_damage(bool with_ammo)
{
    if (is_gunmod() && mode == "MODE_AUX")
        return curammo->damage;
    if (!is_gun())
        return 0;
    if(mode == "MODE_AUX") {
        item* gunmod = active_gunmod();
        if(gunmod != NULL && gunmod->curammo != NULL)
            return gunmod->curammo->damage;
        else
            return 0;
    }
    it_gun* gun = dynamic_cast<it_gun*>(type);
    int ret = gun->dmg_bonus;
    if (with_ammo && curammo != NULL)
        ret += curammo->damage;
    for (size_t i = 0; i < contents.size(); i++) {
        if (contents[i].is_gunmod())
            ret += (dynamic_cast<it_gunmod*>(contents[i].type))->damage;
    }
    ret -= damage * 2;
    return ret;
}

int item::gun_pierce(bool with_ammo)
{
    if (is_gunmod() && mode == "MODE_AUX")
        return curammo->pierce;
    if (!is_gun())
        return 0;
    if(mode == "MODE_AUX") {
        item* gunmod = active_gunmod();
        if(gunmod != NULL && gunmod->curammo != NULL)
            return gunmod->curammo->pierce;
        else
            return 0;
    }
    it_gun* gun = dynamic_cast<it_gun*>(type);
    int ret = gun->pierce;
    if (with_ammo && curammo != NULL)
        ret += curammo->pierce;
    return ret;
}

int item::noise() const
{
    if (!is_gun())
        return 0;
    int ret = 0;
    if(mode == "MODE_AUX") {
        item const* gunmod = inspect_active_gunmod();
        if (gunmod && gunmod->curammo)
            ret = gunmod->curammo->damage;
    } else if (curammo)
        ret = curammo->damage;
    ret *= .8;
    if (ret >= 5) {
        ret += 20;
    }
    if(mode == "MODE_AUX") {
        return ret;
    }
    for (size_t i = 0; i < contents.size(); i++) {
        if (contents[i].is_gunmod())
            ret += (dynamic_cast<it_gunmod*>(contents[i].type))->loudness;
    }
    return ret;
}

int item::burst_size()
{
    if (!is_gun())
        return 0;
// No burst fire for gunmods right now.
    if(mode == "MODE_AUX")
        return 1;
    it_gun* gun = dynamic_cast<it_gun*>(type);
    int ret = gun->burst;
    for (size_t i = 0; i < contents.size(); i++) {
        if (contents[i].is_gunmod())
            ret += (dynamic_cast<it_gunmod*>(contents[i].type))->burst;
    }
    if (ret < 0)
        return 0;
    return ret;
}

int item::recoil(bool with_ammo)
{
    if (!is_gun())
        return 0;
// Just use the raw ammo recoil for now.
    if(mode == "MODE_AUX") {
        item* gunmod = active_gunmod();
        if (gunmod && gunmod->curammo)
            return gunmod->curammo->recoil;
        else
            return 0;
    }
    it_gun* gun = dynamic_cast<it_gun*>(type);
    int ret = gun->recoil;
    if (with_ammo && curammo)
        ret += curammo->recoil;
    for (size_t i = 0; i < contents.size(); i++) {
        if (contents[i].is_gunmod())
            ret += (dynamic_cast<it_gunmod*>(contents[i].type))->recoil;
    }
    ret += damage;
    return ret;
}

int item::range(player *p)
{
    if (!is_gun())
        return 0;
    // Just use the raw ammo range for now.
    // we do NOT want to use the parent gun's range.
    if(mode == "MODE_AUX") {
        item* gunmod = active_gunmod();
        if(gunmod && gunmod->curammo)
            return gunmod->curammo->range;
        else
            return 0;
    }

    // Ammoless weapons use weapon's range only
    if (has_flag("NO_AMMO") && !curammo) {
        return dynamic_cast<it_gun*>(type)->range;
    }

    int ret = (curammo ? dynamic_cast<it_gun*>(type)->range + curammo->range : 0);

    if (has_flag("CHARGE")) {
        ret = dynamic_cast<it_gun*>(type)->range + 5 + charges * 5;
    }

    if (has_flag("STR8_DRAW") && p) {
        if (p->str_cur < 4) { return 0; }
        if (p->str_cur < 8) {
            ret -= 2 * (8 - p->str_cur);
        }
    } else if (has_flag("STR10_DRAW") && p) {
        if (p->str_cur < 5) { return 0; }
        if (p->str_cur < 10) {
            ret -= 2 * (10 - p->str_cur);
        }
    } else if (has_flag("STR12_DRAW") && p) {
        if (p->str_cur < 6) { return 0; }
        if (p->str_cur < 12) {
            ret -= 2 * (12 - p->str_cur);
        }
    }

    if(ret < 0) { ret = 0; }
    return ret;
}


ammotype item::ammo_type() const
{
    if (is_gun()) {
        it_gun* gun = dynamic_cast<it_gun*>(type);
        ammotype ret = gun->ammo;
        for (size_t i = 0; i < contents.size(); i++) {
            if (contents[i].is_gunmod() && !contents[i].has_flag("MODE_AUX")) {
                it_gunmod* mod = dynamic_cast<it_gunmod*>(contents[i].type);
                if (mod->newtype != "NULL")
                    ret = mod->newtype;
            }
        }
        return ret;
    } else if (is_tool()) {
        it_tool* tool = dynamic_cast<it_tool*>(type);
        if (has_flag("ATOMIC_AMMO")) {
            return "plutonium";
        }
        return tool->ammo;
    } else if (is_ammo()) {
        it_ammo* amm = dynamic_cast<it_ammo*>(type);
        return amm->type;
    } else if (is_gunmod()) {
        it_gunmod* mod = dynamic_cast<it_gunmod*>(type);
        return mod->newtype;
    }
    return "NULL";
}

bool item::is_of_type_or_contains_it(const std::string &type_id) const
{
    if (type != NULL && type->id == type_id) {
        return true;
    }
    for (size_t i = 0; i < contents.size(); i++) {
        if (contents[i].is_of_type_or_contains_it(type_id)) {
            return true;
        }
    }
    return false;
}

bool item::is_of_ammo_type_or_contains_it(const ammotype &ammo_type_id) const
{
    const it_ammo *amm = dynamic_cast<const it_ammo *>(type);
    if (amm != NULL && amm->type == ammo_type_id) {
        return true;
    }
    for (size_t i = 0; i < contents.size(); i++) {
        if (contents[i].is_of_ammo_type_or_contains_it(ammo_type_id)) {
            return true;
        }
    }
    return false;
}

void remove_non_matching_types(std::vector<item*> &vec, const std::string &type)
{
    for (std::vector<item*>::iterator it = vec.begin(); it != vec.end(); ) {
        if ((*it)->is_of_type_or_contains_it(type)) {
            ++it;
        } else {
            it = vec.erase(it);
        }
    }
}

int item::pick_reload_ammo(player &u, bool interactive)
{
    if( is_null() ) {
        return INT_MIN;
    }

    if (!type->is_gun() && !type->is_tool()) {
        debugmsg("RELOADING NON-GUN NON-TOOL");
        return INT_MIN;
    }
    int has_spare_mag = has_gunmod ("spare_mag");

    std::vector<item *> am; // List of valid ammo

    if (type->is_gun()) {
        if(charges <= 0 && has_spare_mag != -1 && contents[has_spare_mag].charges > 0) {
            // Special return to use magazine for reloading.
            return INT_MIN + 1;
        }
        it_gun *tmp = dynamic_cast<it_gun *>(type);

        // If there's room to load more ammo into the gun or a spare mag, stash the ammo.
        // If the gun is partially loaded make sure the ammo matches.
        // If the gun is empty, either the spare mag is empty too and anything goes,
        // or the spare mag is loaded and we're doing a tactical reload.
        if (charges < clip_size() ||
            (has_spare_mag != -1 && contents[has_spare_mag].charges < tmp->clip)) {
            std::vector<item *> tmpammo = u.has_ammo(ammo_type());
            if (charges > 0) {
                // partially loaded, accept only ammo of the exact same type
                remove_non_matching_types(tmpammo, curammo->id);
            }
            am.insert(am.end(), tmpammo.begin(), tmpammo.end());
        }

        // ammo for gun attachments (shotgun attachments, grenade attachments, etc.)
        // for each attachment, find its associated ammo & append it to the ammo vector
        for (size_t i = 0; i < contents.size(); i++) {
            item &cont = contents[i];
            const it_gunmod *mod = dynamic_cast<it_gunmod *>(cont.type);
            if (mod == NULL || !cont.has_flag("MODE_AUX")) {
                // not a gunmod, or has no separate firing mode and can not be load
                continue;
            }
            if (cont.charges >= mod->clip) {
                // already fully loaded
                continue;
            }
            std::vector<item *> tmpammo = u.has_ammo(mod->newtype);
            if (cont.charges > 0) {
                // partially loaded, accept only ammo of the exact same type
                remove_non_matching_types(tmpammo, cont.curammo->id);
            }
            am.insert(am.end(), tmpammo.begin(), tmpammo.end());
        }
    } else { //non-gun.
        // this is probably a tool.  Check if it uses atomic power instead of batteries
        if (has_flag("ATOMIC_AMMO")) {
            am = u.has_ammo("plutonium");
        } else {
            am = u.has_ammo(ammo_type());
        }
    }

    if (am.empty()) {
        return INT_MIN;
    }
    if (am.size() == 1 || !interactive) {
        // Either only one valid choice or chosing for a NPC, just return the first.
        return u.get_item_position(am[0]);
    }

    // More than one option; list 'em and pick
    uimenu amenu;
    amenu.return_invalid = true;
    amenu.w_y = 0;
    amenu.w_x = 0;
    amenu.w_width = TERMX;
    // 40: = 4 * ammo stats colum (10 chars each)
    // 2: prefix from uimenu: hotkey + space in front of name
    // 4: borders: 2 char each ("| " and " |")
    const int namelen = TERMX - 2 - 40 - 4;
    std::string lastreload = "";

    if ( uistate.lastreload.find( ammo_type() ) != uistate.lastreload.end() ) {
        lastreload = uistate.lastreload[ ammo_type() ];
    }

    amenu.text = std::string(_("Choose ammo type:"));
    if (amenu.text.length() < namelen) {
        amenu.text += std::string(namelen - amenu.text.length(), ' ');
    } else {
        amenu.text.erase(namelen, amenu.text.length() - namelen);
    }
    // To cover the space in the header that is used by the hotkeys created by uimenu
    amenu.text.insert(0, "  ");
    //~ header of table that appears when reloading, each colum must contain exactly 10 characters
    amenu.text += _("| Damage  | Pierce  | Range   | Accuracy");
    // Stores the ammo ids (=item type, not ammo type) for the uistate
    std::vector<std::string> ammo_ids;
    for (size_t i = 0; i < am.size(); i++) {
        item &it = *am[i];
        it_ammo *ammo_def = dynamic_cast<it_ammo *>(it.type);
        // ammo_def == NULL means the item is a container,
        // containing the ammo, go through its content to find the ammo
        for (size_t j = 0; ammo_def == NULL && j < it.contents.size(); j++) {
            ammo_def = dynamic_cast<it_ammo *>(it.contents[j].type);
        }
        if (ammo_def == NULL) {
            debugmsg("%s: contains no ammo & is no ammo", it.tname().c_str());
            ammo_ids.push_back("");
            continue;
        }
        ammo_ids.push_back(ammo_def->id);
        // still show the container name, display_name adds the contents
        // to the name: "bottle of gasoline"
        std::string row = it.display_name();
        if (row.length() < namelen) {
            row += std::string(namelen - row.length(), ' ');
        } else {
            row.erase(namelen, row.length() - namelen);
        }
        row += string_format("| %-7d | %-7d | %-7d | %-7d",
                                ammo_def->damage, ammo_def->pierce, ammo_def->range,
                                100 - ammo_def->dispersion);
        amenu.addentry(i, true, i + 'a', row);
        if ( lastreload == ammo_def->id ) {
            amenu.selected = i;
        }
    }
    amenu.query();
    if (amenu.ret < 0 || amenu.ret >= am.size()) {
        // invalid selection / escaped from the menu
        return INT_MIN;
    }
    uistate.lastreload[ ammo_type() ] = ammo_ids[ amenu.ret ];
    return u.get_item_position(am[ amenu.ret ]);
}

// Helper to handle ejecting casings from guns that require them to be manually extracted.
static void eject_casings( player &p, item *reload_target, itype_id casing_type ) {
    if( reload_target->has_flag("RELOAD_EJECT") && casing_type != "NULL" && !casing_type.empty() ) {
        if( reload_target->item_vars.count( "CASINGS" ) ) {
            int num_casings = atoi( reload_target->item_vars[ "CASINGS" ].c_str() );
            item casing( casing_type, 0);
            // Casings need a count of one to stack properly.
            casing.charges = 1;
            // Drop all the casings on the ground under the player.
            for( int i = 0; i < num_casings; ++i ) {
                g->m.add_item_or_charges(p.posx, p.posy, casing);
            }
            reload_target->item_vars.erase( "CASINGS" );
        }
    }
}

bool item::reload(player &u, int pos)
{
    bool single_load = false;
    int max_load = 1;
    item *reload_target = NULL;
    item *ammo_to_use = &u.i_at(pos);
    item *ammo_container = NULL;

    // Handle ammo in containers, currently only gasoline and quivers
    if (!ammo_to_use->contents.empty() && (ammo_to_use->is_container() ||
                                           ammo_to_use->type->can_use("QUIVER"))) {
        ammo_container = ammo_to_use;
        ammo_to_use = &ammo_to_use->contents[0];
    }

    if (is_gun()) {
        // Reload using a spare magazine
        int spare_mag = has_gunmod("spare_mag");
        if (charges <= 0 && spare_mag != -1 &&
            u.weapon.contents[spare_mag].charges > 0) {
            charges = u.weapon.contents[spare_mag].charges;
            curammo = u.weapon.contents[spare_mag].curammo;
            u.weapon.contents[spare_mag].charges = 0;
            u.weapon.contents[spare_mag].curammo = NULL;
            return true;
        }

        // Determine what we're reloading, the gun, a spare magazine, or another gunmod.
        // Prefer the active gunmod if there is one
        item* gunmod = active_gunmod();
        if (gunmod && gunmod->ammo_type() == ammo_to_use->ammo_type() &&
            (gunmod->charges <= 0 || gunmod->curammo->id == ammo_to_use->typeId())) {
            reload_target = gunmod;
            // Then prefer the gun itself
        } else if (charges < clip_size() &&
                   ammo_type() == ammo_to_use->ammo_type() &&
                   (charges <= 0 || curammo->id == ammo_to_use->typeId())) {
            reload_target = this;
            // Then prefer a spare mag if present
        } else if (spare_mag != -1 &&
                   ammo_type() == ammo_to_use->ammo_type() &&
                   contents[spare_mag].charges != (dynamic_cast<it_gun*>(type))->clip &&
                   (charges <= 0 || curammo->id == ammo_to_use->typeId())) {
            reload_target = &contents[spare_mag];
            // Finally consider other gunmods
        } else {
            for (size_t i = 0; i < contents.size(); i++) {
                if (&contents[i] != gunmod && (int)i != spare_mag && contents[i].is_gunmod() &&
                    contents[i].has_flag("MODE_AUX") &&
                    contents[i].ammo_type() == ammo_to_use->ammo_type() &&
                    (contents[i].charges <= (dynamic_cast<it_gunmod*>(contents[i].type))->clip ||
                     (contents[i].charges <= 0 || contents[i].curammo->id == ammo_to_use->typeId()))) {
                    reload_target = &contents[i];
                    break;
                }
            }
        }

        if (reload_target == NULL) {
            return false;
        }

        if (reload_target->is_gun() || reload_target->is_gunmod()) {
            if (reload_target->is_gunmod() && reload_target->typeId() == "spare_mag") {
                // Use gun numbers instead of the mod if it's a spare magazine
                max_load = (dynamic_cast<it_gun*>(type))->clip;
                single_load = has_flag("RELOAD_ONE");
            } else {
                single_load = reload_target->has_flag("RELOAD_ONE");
                max_load = reload_target->clip_size();
            }
        }
    } else if (is_tool()) {
        it_tool* tool = dynamic_cast<it_tool*>(type);
        reload_target = this;
        single_load = false;
        max_load = tool->max_charges;
    } else {
        return false;
    }

    if (has_flag("DOUBLE_AMMO")) {
        max_load *= 2;
    }

    if (has_flag("ATOMIC_AMMO")) {
        max_load *= 100;
    }

    if (pos != INT_MIN) {
        // If the gun is currently loaded with a different type of ammo, reloading fails
        if ((reload_target->is_gun() || reload_target->is_gunmod()) &&
            reload_target->charges > 0 && reload_target->curammo->id != ammo_to_use->typeId()) {
            return false;
        }
        if (reload_target->is_gun() || reload_target->is_gunmod()) {
            if (!ammo_to_use->is_ammo()) {
                debugmsg("Tried to reload %s with %s!", tname().c_str(),
                         ammo_to_use->tname().c_str());
                return false;
            }
            reload_target->curammo = dynamic_cast<it_ammo*>((ammo_to_use->type));
        }
        if (curammo != NULL) {
            eject_casings( u, reload_target, curammo->casing );
        }
        if (single_load || max_load == 1) { // Only insert one cartridge!
            reload_target->charges++;
            ammo_to_use->charges--;
        }
        else if (reload_target->typeId() == "adv_UPS_off" ||
                 reload_target->typeId() == "adv_UPS_on" || reload_target->has_flag("ATOMIC_AMMO") ||
                 reload_target->typeId() == "rm13_armor" || reload_target->typeId() == "rm13_armor_on") {
            int charges_per_plut = 500;
            long max_plut = floor( static_cast<float>((max_load - reload_target->charges) /
                                                      charges_per_plut) );
            int charges_used = std::min(ammo_to_use->charges, max_plut);
            reload_target->charges += (charges_used * charges_per_plut);
            ammo_to_use->charges -= charges_used;
        } else {
            reload_target->charges += ammo_to_use->charges;
            ammo_to_use->charges = 0;
            if (reload_target->charges > max_load) {
                // More rounds than the clip holds, put some back
                ammo_to_use->charges += reload_target->charges - max_load;
                reload_target->charges = max_load;
            }
        }
        if (ammo_to_use->charges == 0) {
            if (ammo_container != NULL) {
                ammo_container->contents.erase(ammo_container->contents.begin());
                // We just emptied a container, which might be part of stack,
                // but empty and non-empty containers should not stack, force
                // a re-stacking.
                u.inv.restack(&u);
            } else {
                u.i_rem(pos);
            }
        }
        return true;
    } else {
        return false;
    }
}

void item::use()
{
    if (charges > 0) {
        charges--;
    }
}

bool item::burn(int amount)
{
    burnt += amount;
    return (burnt >= volume() * 3);
}

bool item::flammable() const
{
    const material_type* cur_mat1 = get_material(1);
    const material_type* cur_mat2 = get_material(2);

    return ((cur_mat1->fire_resist() + cur_mat2->fire_resist()) <= 0);
}

std::ostream & operator<<(std::ostream & out, const item * it)
{
    out << "item(";
    if(!it)
    {
        out << "NULL)";
        return out;
    }
    out << it->name << ")";
    return out;
}

std::ostream & operator<<(std::ostream & out, const item & it)
{
    out << (&it);
    return out;
}


itype_id item::typeId() const
{
    if (!type) {
        return "null";
    }
    return type->id;
}

item item::clone(bool rand) {
    return item(type->id, bday, rand);
}

bool item::getlight(float & luminance, int & width, int & direction, bool calculate_dimming ) const {
    luminance = 0;
    width = 0;
    direction = 0;
    if ( light.luminance > 0 ) {
        luminance = (float)light.luminance;
        if ( light.width > 0 ) { // width > 0 is a light arc
            width = light.width;
            direction = light.direction;
        }
        return true;
    } else {
        const int lumint = getlight_emit( calculate_dimming );
        if ( lumint > 0 ) {
            luminance = (float)lumint;
            return true;
        }
    }
    return false;
}

/*
 * Returns just the integer
 */
int item::getlight_emit(bool calculate_dimming) const {
    const int mult = 10; // woo intmath
    const int chargedrop = 5 * mult; // start dimming at 1/5th charge.

    int lumint = type->light_emission * mult;

    if ( lumint == 0 ) {
        return 0;
    }
    if ( calculate_dimming && has_flag("CHARGEDIM") && is_tool()) {
        it_tool * tool = dynamic_cast<it_tool *>(type);
        int maxcharge = tool->max_charges;
        if ( maxcharge > 0 ) {
            lumint = ( type->light_emission * chargedrop * charges ) / maxcharge;
        }
    }
    if ( lumint > 4 && lumint < 10 ) {
        lumint = 10;
    }
    return lumint / 10;
}

// How much more of this liquid can be put in this container
int item::get_remaining_capacity_for_liquid(const item &liquid, LIQUID_FILL_ERROR &error) const
{
    error = L_ERR_NONE;

    if (liquid.is_ammo() && (is_tool() || is_gun())) {
        // for filling up chainsaws, jackhammers and flamethrowers
        ammotype ammo = "NULL";
        int max = 0;

        if (is_tool()) {
            it_tool *tool = dynamic_cast<it_tool *>(type);
            ammo = tool->ammo;
            max = tool->max_charges;
        } else {
            it_gun *gun = dynamic_cast<it_gun *>(type);
            ammo = gun->ammo;
            max = gun->clip;
        }

        ammotype liquid_type = liquid.ammo_type();

        if (ammo != liquid_type) {
            error = L_ERR_NOT_CONTAINER;
            return 0;
        }

        if (max <= 0 || charges >= max) {
            error = L_ERR_FULL;
            return 0;
        }

        if (charges > 0 && curammo != NULL && curammo->id != liquid.type->id) {
            error = L_ERR_NO_MIX;
            return 0;
        }
        return max - charges;
    }

    if (!is_container()) {
        error = L_ERR_NOT_CONTAINER;
        return 0;
    }

    if (contents.empty()) {
        if (!has_flag("WATERTIGHT")) { // invalid container types
            error = L_ERR_NOT_WATERTIGHT;
            return 0;
        } else if (!has_flag("SEALS")) {
            error = L_ERR_NOT_SEALED;
            return 0;
        }
    } else { // Not empty
        if (contents[0].type->id != liquid.type->id) {
            error = L_ERR_NO_MIX;
            return 0;
        }
    }

    it_container *container = dynamic_cast<it_container *>(type);
    int total_capacity = container->contains;

    if (liquid.is_food()) {
        it_comest *tmp_comest = dynamic_cast<it_comest *>(liquid.type);
        total_capacity = container->contains * tmp_comest->charges;
    } else if (liquid.is_ammo()) {
        it_ammo *tmp_ammo = dynamic_cast<it_ammo *>(liquid.type);
        total_capacity = container->contains * tmp_ammo->count;
    }

    int remaining_capacity = total_capacity;
    if (!contents.empty()) {
        remaining_capacity -= contents[0].charges;
    }

    if (remaining_capacity <= 0) {
        error = L_ERR_FULL;
        return 0;
    }

    return remaining_capacity;
}

int item::amount_of(const itype_id &it, bool used_as_tool) const
{
    int count = 0;
    // Check that type matches, and (if not used as tool), it
    // is not a pseudo item.
    if (type->id == it && (used_as_tool || !has_flag("PSEUDO"))) {
        if (contents.empty()) {
            // Only use empty container
            count++;
        }
    }
    for (size_t k = 0; k < contents.size(); k++) {
        count += contents[k].amount_of(it, used_as_tool);
    }
    return count;
}

bool item::use_amount(const itype_id &it, int &quantity, bool use_container, std::list<item> &used)
{
    // First, check contents
    bool used_item_contents = false;
    for (std::vector<item>::iterator a = contents.begin(); a != contents.end() && quantity > 0; ) {
        if (a->use_amount(it, quantity, use_container, used)) {
            a = contents.erase(a);
            used_item_contents = true;
        } else {
            ++a;
        }
    }
    // Now check the item itself
    if (use_container && used_item_contents) {
        return true;
    } else if (type->id == it && quantity > 0 && contents.empty()) {
        used.push_back(*this);
        quantity--;
        return true;
    } else {
        return false;
    }
}

long item::charges_of(const itype_id &it) const
{
    long count = 0;
    if (type->id == it && contents.empty()) {
        // If we're specifically looking for a container, only say we have it if it's empty.
        if (charges < 0) {
            count++;
        } else {
            count += charges;
        }
    } else {
        for (std::vector<item>::const_iterator a = contents.begin(); a != contents.end(); ++a) {
            count += a->charges_of(it);
        }
    }
    return count;
}

bool item::use_charges(const itype_id &it, long &quantity, std::list<item> &used)
{
    // First, check contents
    for (std::vector<item>::iterator a = contents.begin(); a != contents.end() && quantity > 0; ) {
        if (a->use_charges(it, quantity, used)) {
            a = contents.erase(a);
        } else {
            ++a;
        }
    }
    // Now check the item itself
    if (type->id != it || quantity <= 0 || !contents.empty()) {
        return false;
    }
    if (charges <= quantity) {
        used.push_back(*this);
        if (charges < 0) {
            quantity--;
        } else {
            quantity -= charges;
        }
        charges = 0;
        return destroyed_at_zero_charges();
    }
    used.push_back(*this);
    used.back().charges = quantity;
    charges -= quantity;
    quantity = 0;
    return false;
}

const item_category &item::get_category() const
{
    if(is_container() && !contents.empty()) {
        return contents[0].get_category();
    }
    if(type != 0) {
        if(type->category == 0) {
            // Category not set? Set it now.
            itype *t = const_cast<itype *>(type);
            t->category = item_controller->get_category(item_controller->calc_category(t));
        }
        return *type->category;
    }
    // null-item -> null-category
    static item_category null_category;
    return null_category;
}

bool item_matches_locator(const item &it, const itype_id &id, int) {
    return it.typeId() == id;
}
bool item_matches_locator(const item &, int locator_pos, int item_pos) {
    return item_pos == locator_pos;
}
bool item_matches_locator(const item &it, char invlet, int) {
    return it.invlet == invlet;
}

iteminfo::iteminfo(std::string Type, std::string Name, std::string Fmt, double Value, bool _is_int, std::string Plus, bool NewLine, bool LowerIsBetter, bool DrawName) {
    sType = Type;
    sName = Name;
    sFmt = Fmt;
    is_int = _is_int;
    dValue = Value;
    std::stringstream convert;
    if (_is_int) {
        int dIn0i = int(Value);
        convert << dIn0i;
    } else {
        convert.precision(2);
        convert << std::fixed << Value;
    }
    sValue = convert.str();
    sPlus = Plus;
    bNewLine = NewLine;
    bLowerIsBetter = LowerIsBetter;
    bDrawName = DrawName;
}

void item::detonate(point p) const
{
    if (type == NULL || type->explosion_on_fire_data.power < 0) {
        return;
    }
    g->explosion(p.x, p.y, type->explosion_on_fire_data.power, type->explosion_on_fire_data.shrapnel, type->explosion_on_fire_data.fire, type->explosion_on_fire_data.blast);
}


//sort quivers by contents, such that empty quivers go last
struct sort_by_charges {
    bool operator()(const std::pair<item*,int> &left, const std::pair<item*,int> &right) {
        if(left.first->contents.empty()) {
            return false;
        } else if (right.first->contents.empty()){
            return true;
        } else {
            return right.first->contents[0].charges < left.first->contents[0].charges;
        }
    }
};

//return value is number of arrows/bolts quivered
//isAutoPickup is used to determine text output behavior
int item::add_ammo_to_quiver(player *u, bool isAutoPickup)
{
    std::vector<std::pair<item*, int> > quivers;
    for(std::vector<item>::iterator it = u->worn.begin(); it != u->worn.end(); it++) {
        item& worn = *it;

        //item is valid quiver to store items in if it satisfies these conditions:
        // a) is a quiver  b) contents are ammo w/ charges  c) quiver isn't full
        if(worn.type->can_use("QUIVER")) {
            int maxCharges = worn.max_charges_from_flag("QUIVER");
            if (worn.contents.empty() || (worn.contents[0].is_ammo() && worn.contents[0].charges > 0)) {
                quivers.push_back(std::make_pair(&worn, maxCharges));
            }
        }
    }

    // check if we have eligible quivers
    if(!quivers.empty()) {
        int movesPerArrow = 10;
        int arrowsQuivered = 0;

        //sort quivers by contents, such that empty quivers go last
        std::sort(quivers.begin(), quivers.end(), sort_by_charges());

        //loop over all eligible quivers
        for(std::vector<std::pair<item*, int> >::iterator it = quivers.begin(); it != quivers.end(); it++) {
            //only proceed if we still have item charges
            if(charges > 0) {
                item *worn = it->first;
                int maxArrows = it->second;
                int arrowsStored = 0;
                int toomany = 0;
                std::vector<std::pair<item*, int> >::iterator final_iter = quivers.end();
                --final_iter;

                if(maxArrows == 0) {
                    debugmsg("Tried storing arrows in quiver without a QUIVER_n tag (item::add_ammo_to_quiver)");
                    return 0;
                }

                // quiver not empty so adding more ammo
                if(!(worn->contents.empty()) && worn->contents[0].charges > 0) {
                    if(worn->contents[0].type->id != type->id) {
                        if(!isAutoPickup) {
                            u->add_msg_if_player(m_info, _("Those aren't the same arrows!"));
                        }

                        //only return false if this is last quiver in the loop
                        if (it != final_iter) {
                            continue;
                        } else {
                            return 0;
                        }
                    }
                    if(worn->contents[0].charges >= maxArrows) {
                        if(!isAutoPickup) {
                            u->add_msg_if_player(m_info, _("That %s is already full!"), worn->name.c_str());
                        }

                        //only return false if this is last quiver in the loop
                        if (it != final_iter) {
                            continue;
                        } else {
                            return 0;
                        }
                    }

                    arrowsStored = worn->contents[0].charges;
                    worn->contents[0].charges += charges;
                } else { // quiver empty, putting in new arrows
                    //add a clone so we can zero out charges on base item
                    item clone = this->clone();
                    clone.charges = charges;
                    worn->put_in(clone);
                }

                //get rid of charges from base item, since the ammo is now quivered
                charges = 0;

                // check for any extra ammo
                if(worn->contents[0].charges > maxArrows) {
                    //set quiver's charges to max
                    toomany = worn->contents[0].charges - maxArrows;
                    worn->contents[0].charges -= toomany;

                    //add any extra ammo back into base item charges
                    charges += toomany;
                }

                arrowsStored = worn->contents[0].charges - arrowsStored;
                u->add_msg_if_player(ngettext("You store %d %s in your %s.", "You store %d %s in your %s.", arrowsStored),
                                     arrowsStored, worn->contents[0].type->nname(arrowsStored).c_str(), worn->name.c_str());
                u->moves -= std::min(100, movesPerArrow * arrowsStored);
                arrowsQuivered += arrowsStored;
            }
        }

        // handle overflow after filling all quivers
        if(isAutoPickup && charges > 0 && u->can_pickVolume(volume())) {
            //add any extra ammo to inventory
            item clone = this->clone();
            clone.charges = charges;
            u->i_add(clone);

            u->add_msg_if_player(ngettext("You pick up %d %s.", "You pick up %d %s.", charges),
                             charges, clone.type->nname(charges).c_str());
            u->moves -= 100;

            charges = 0;
        }
        return arrowsQuivered;
    }
    return 0;
}

//used to implement charges for items that aren't tools (e.g. quivers)
//flagName arg is the flag's name before the underscore and integer on the end
//e.g. for "QUIVER_20" flag, flagName = "QUIVER"
int item::max_charges_from_flag(std::string flagName)
{
    item* it = this;
    int maxCharges = 0;

    //loop through item's flags, looking for flag that matches flagName
    for(std::set<std::string>::iterator iter = it->type->item_tags.begin(); iter != it->type->item_tags.end(); iter++) {
        std::string flag = *iter;
        if(flag.substr(0, flagName.size()) == flagName ) {
            //get the substring of the flag starting w/ digit after underscore
            std::stringstream ss(flag.substr(flagName.size() + 1, flag.size()));

            //attempt to store that stringstream into maxCharges and error if there's a problem
            if(!(ss >> maxCharges)) {
                debugmsg("Error parsing %s_n tag (item::max_charges_from_flag)"), flagName.c_str();
                maxCharges = -1;
            }
            break;
        }
    }

    return maxCharges;
}

int item::butcher_factor() const
{
    int butcher_factor = INT_MAX;
    if (has_quality("CUT") && !has_flag("SPEAR")) {
        int butcher_factor = volume() * 5 - weight() / 75 - damage_cut();
        if (damage_cut() <= 20) {
            butcher_factor *= 2;
        }
        return butcher_factor;
    } else {
        for(std::vector<item>::const_iterator a = contents.begin(); a != contents.end(); ++a) {
            butcher_factor = std::min(butcher_factor, a->butcher_factor());
        }
    }
    return butcher_factor;
}
