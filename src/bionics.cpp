#include "player.h"
#include "game.h"
#include "rng.h"
#include "input.h"
#include "item.h"
#include "bionics.h"
#include "line.h"
#include "json.h"
#include "messages.h"
#include "item_factory.h"
#include <math.h>    //sqrt
#include <algorithm> //std::min
#include <sstream>

#define BATTERY_AMOUNT 100 // How much batteries increase your power

std::map<bionic_id, bionic_data *> bionics;
std::vector<bionic_id> faulty_bionics;
std::vector<bionic_id> power_source_bionics;
std::vector<bionic_id> unpowered_bionics;

void bionics_install_failure(player *u, it_bionic *type, int success);

bionic_data::bionic_data(std::string new_name, bool new_power_source, bool new_activated,
                         int new_power_cost, int new_charge_time, std::string new_description,
                         bool new_faulty) : description(new_description)
{
    name = new_name;
    power_source = new_power_source;
    activated = new_activated;
    power_cost = new_power_cost;
    charge_time = new_charge_time;
    faulty = new_faulty;
}

bionic_id game::random_good_bionic() const
{
    std::map<std::string, bionic_data *>::const_iterator random_bionic;
    do {
        random_bionic = bionics.begin();
        std::advance(random_bionic, rng(0, bionics.size() - 1));
    } while (random_bionic->second->faulty);
    return random_bionic->first;
}

// Why put this in a Big Switch?  Why not let bionics have pointers to
// functions, much like monsters and items?
//
// Well, because like diseases, which are also in a Big Switch, bionics don't
// share functions....
void player::activate_bionic(int b)
{
    bionic bio = my_bionics[b];
    int power_cost = bionics[bio.id]->power_cost;
    if ((weapon.type->id == "bio_claws_weapon" && bio.id == "bio_claws_weapon") ||
        (weapon.type->id == "bio_blade_weapon" && bio.id == "bio_blade_weapon")) {
        power_cost = 0;
    }
    if (power_level < power_cost) {
        if (my_bionics[b].powered) {
            add_msg(m_neutral, _("Your %s powers down."), bionics[bio.id]->name.c_str());
            my_bionics[b].powered = false;
        } else {
            add_msg(m_info, _("You cannot power your %s"), bionics[bio.id]->name.c_str());
        }
        return;
    }

    if (my_bionics[b].powered && my_bionics[b].charge > 0) {
        // Already-on units just lose a bit of charge
        my_bionics[b].charge--;
    } else {
        // Not-on units, or those with zero charge, have to pay the power cost
        if (bionics[bio.id]->charge_time > 0) {
            my_bionics[b].powered = true;
            my_bionics[b].charge = bionics[bio.id]->charge_time - 1;
        }
        power_level -= power_cost;
    }

    std::vector<point> traj;
    std::vector<std::string> good;
    std::vector<std::string> bad;
    int dirx, diry;
    item tmp_item;

    if(bio.id == "bio_painkiller") {
        pkill += 6;
        pain -= 2;
        if (pkill > pain) {
            pkill = pain;
        }
    } else if (bio.id == "bio_nanobots") {
        rem_disease("bleed");
        healall(4);
    } else if (bio.id == "bio_night") {
        if (calendar::turn % 5) {
            add_msg(m_neutral, _("Artificial night generator active!"));
        }
    } else if (bio.id == "bio_resonator") {
        g->sound(posx, posy, 30, _("VRRRRMP!"));
        for (int i = posx - 1; i <= posx + 1; i++) {
            for (int j = posy - 1; j <= posy + 1; j++) {
                g->m.bash( i, j, 40 );
                g->m.bash( i, j, 40 ); // Multibash effect, so that doors &c will fall
                g->m.bash( i, j, 40 );
            }
        }
    } else if (bio.id == "bio_time_freeze") {
        moves += power_level;
        power_level = 0;
        add_msg(m_good, _("Your speed suddenly increases!"));
        if (one_in(3)) {
            add_msg(m_bad, _("Your muscles tear with the strain."));
            apply_damage( nullptr, bp_arm_l, rng( 5, 10 ) );
            apply_damage( nullptr, bp_arm_r, rng( 5, 10 ) );
            apply_damage( nullptr, bp_leg_l, rng( 7, 12 ) );
            apply_damage( nullptr, bp_leg_r, rng( 7, 12 ) );
            apply_damage( nullptr, bp_torso, rng( 5, 15 ) );
        }
        if (one_in(5)) {
            add_disease("teleglow", rng(50, 400));
        }
    } else if (bio.id == "bio_teleport") {
        g->teleport();
        add_disease("teleglow", 300);
    }
    // TODO: More stuff here (and bio_blood_filter)
    else if(bio.id == "bio_blood_anal") {
        WINDOW *w = newwin(20, 40, 3 + ((TERMY > 25) ? (TERMY - 25) / 2 : 0),
                           10 + ((TERMX > 80) ? (TERMX - 80) / 2 : 0));
        draw_border(w);
        if (has_disease("fungus")) {
            bad.push_back(_("Fungal Parasite"));
        }
        if (has_disease("dermatik")) {
            bad.push_back(_("Insect Parasite"));
        }
        if (has_effect("stung")) {
            bad.push_back(_("Stung"));
        }
        if (has_effect("poison")) {
            bad.push_back(_("Poison"));
        }
        if (radiation > 0) {
            bad.push_back(_("Irradiated"));
        }
        if (has_disease("pkill1")) {
            good.push_back(_("Minor Painkiller"));
        }
        if (has_disease("pkill2")) {
            good.push_back(_("Moderate Painkiller"));
        }
        if (has_disease("pkill3")) {
            good.push_back(_("Heavy Painkiller"));
        }
        if (has_disease("pkill_l")) {
            good.push_back(_("Slow-Release Painkiller"));
        }
        if (has_disease("drunk")) {
            good.push_back(_("Alcohol"));
        }
        if (has_disease("cig")) {
            good.push_back(_("Nicotine"));
        }
        if (has_disease("meth")) {
            good.push_back(_("Methamphetamines"));
        }
        if (has_disease("high")) {
            good.push_back(_("Intoxicant: Other"));
        }
        if (has_disease("weed_high")) {
            good.push_back(_("THC Intoxication"));
        }
        if (has_disease("hallu") || has_disease("visuals")) {
            bad.push_back(_("Magic Mushroom"));
        }
        if (has_disease("iodine")) {
            good.push_back(_("Iodine"));
        }
        if (has_disease("datura")) {
            good.push_back(_("Anticholinergic Tropane Alkaloids"));
        }
        if (has_disease("took_xanax")) {
            good.push_back(_("Xanax"));
        }
        if (has_disease("took_prozac")) {
            good.push_back(_("Prozac"));
        }
        if (has_disease("took_flumed")) {
            good.push_back(_("Antihistamines"));
        }
        if (has_disease("adrenaline")) {
            good.push_back(_("Adrenaline Spike"));
        }
        if (has_disease("tapeworm")) {  // This little guy is immune to the blood filter though, as he lives in your bowels.
            good.push_back(_("Intestinal Parasite"));
        }
        if (has_disease("bloodworms")) {
            good.push_back(_("Hemolytic Parasites"));
        }
        if (has_disease("brainworm")) {  // This little guy is immune to the blood filter too, as he lives in your brain.
            good.push_back(_("Intracranial Parasite"));
        }
        if (has_disease("paincysts")) {  // These little guys are immune to the blood filter too, as they live in your muscles.
            good.push_back(_("Intramuscular Parasites"));
        }
        if (has_disease("tetanus")) {  // Tetanus infection.
            good.push_back(_("Clostridium Tetani Infection"));
        }
        if (good.empty() && bad.empty()) {
            mvwprintz(w, 1, 1, c_white, _("No effects."));
        } else {
            for (unsigned line = 1; line < 39 && line <= good.size() + bad.size(); line++) {
                if (line <= bad.size()) {
                    mvwprintz(w, line, 1, c_red, "%s", bad[line - 1].c_str());
                } else {
                    mvwprintz(w, line, 1, c_green, "%s", good[line - 1 - bad.size()].c_str());
                }
            }
        }
        wrefresh(w);
        refresh();
        getch();
        delwin(w);
    } else if(bio.id == "bio_blood_filter") {
        add_msg(m_neutral, _("You activate your blood filtration system."));
        rem_disease("fungus");
        rem_disease("dermatik");
        rem_disease("bloodworms");
        rem_disease("tetanus");
        remove_effect("poison");
        remove_effect("stung");
        rem_disease("pkill1");
        rem_disease("pkill2");
        rem_disease("pkill3");
        rem_disease("pkill_l");
        rem_disease("drunk");
        rem_disease("cig");
        rem_disease("high");
        rem_disease("hallu");
        rem_disease("visuals");
        rem_disease("iodine");
        rem_disease("datura");
        rem_disease("took_xanax");
        rem_disease("took_prozac");
        rem_disease("took_flumed");
        rem_disease("adrenaline");
        rem_disease("meth");
        pkill = 0;
        stim = 0;
    } else if(bio.id == "bio_evap") {
        item water = item("water_clean", 0);
        int humidity = g->weatherGen.get_weather(pos(), calendar::turn).humidity;
        int water_charges = (humidity * 3.0) / 100.0 + 0.5;
        // At 50% relative humidity or more, the player will draw 2 units of water
        // At 16% relative humidity or less, the player will draw 0 units of water
        water.charges = water_charges;
        if (water_charges == 0) {
            add_msg_if_player(m_bad, _("There was not enough moisture in the air from which to draw water!"));
        }
        if (g->handle_liquid(water, true, false)) {
            moves -= 100;
        } else if (query_yn(_("Drink from your hands?"))) {
            inv.push_back(water);
            consume(inv.position_by_type(water.typeId()));
            moves -= 350;
        } else if (water.charges == water_charges && water_charges != 0) {
            power_level += bionics["bio_evap"]->power_cost;
        }
    } else if(bio.id == "bio_lighter") {
        if(!choose_adjacent(_("Start a fire where?"), dirx, diry) ||
           (!g->m.add_field(dirx, diry, fd_fire, 1))) {
            add_msg_if_player(m_info, _("You can't light a fire there."));
            power_level += bionics["bio_lighter"]->power_cost;
        }

    }
    if(bio.id == "bio_leukocyte") {
        add_msg(m_neutral, _("You activate your leukocyte breeder system."));
        g->u.set_healthy(std::min(100, g->u.get_healthy() + 2));
        g->u.mod_healthy_mod(20);
    }
    if(bio.id == "bio_geiger") {
        add_msg(m_info, _("Your radiation level: %d"), radiation);
    }
    if(bio.id == "bio_radscrubber") {
        add_msg(m_neutral, _("You activate your radiation scrubber system."));
        if (radiation > 4) {
            radiation -= 5;
        } else {
            radiation = 0;
        }
    }
    if(bio.id == "bio_adrenaline") {
        add_msg(m_neutral, _("You activate your adrenaline pump."));
        if (has_disease("adrenaline")) {
            add_disease("adrenaline", 50);
        } else {
            add_disease("adrenaline", 200);
        }
    } else if(bio.id == "bio_claws") {
        if (weapon.type->id == "bio_claws_weapon") {
            add_msg(m_neutral, _("You withdraw your claws."));
            weapon = ret_null;
        } else if (weapon.has_flag ("NO_UNWIELD")) {
            add_msg(m_info, _("Deactivate your %s first!"),
                    weapon.tname().c_str());
            power_level += bionics[bio.id]->power_cost;
            return;
        } else if(weapon.type->id != "null") {
            add_msg(m_warning, _("Your claws extend, forcing you to drop your %s."),
                    weapon.tname().c_str());
            g->m.add_item_or_charges(posx, posy, weapon);
            weapon = item("bio_claws_weapon", 0);
            weapon.invlet = '#';
        } else {
            add_msg(m_neutral, _("Your claws extend!"));
            weapon = item("bio_claws_weapon", 0);
            weapon.invlet = '#';
        }
    } else if(bio.id == "bio_blade") {
        if (weapon.type->id == "bio_blade_weapon") {
            add_msg(m_neutral, _("You retract your blade."));
            weapon = ret_null;
        } else if (weapon.has_flag ("NO_UNWIELD")) {
            add_msg(m_info, _("Deactivate your %s first!"),
                    weapon.tname().c_str());
            power_level += bionics[bio.id]->power_cost;
            return;
        } else if(weapon.type->id != "null") {
            add_msg(m_warning, _("Your blade extends, forcing you to drop your %s."),
                    weapon.tname().c_str());
            g->m.add_item_or_charges(posx, posy, weapon);
            weapon = item("bio_blade_weapon", 0);
            weapon.invlet = '#';
        } else {
            add_msg(m_neutral, _("You extend your blade!"));
            weapon = item("bio_blade_weapon", 0);
            weapon.invlet = '#';
        }
    } else if(bio.id == "bio_blaster") {
        tmp_item = weapon;
        weapon = item("bio_blaster_gun", 0);
        g->refresh_all();
        g->plfire(false);
        if(weapon.charges == 1) { // not fired
            power_level += bionics[bio.id]->power_cost;
        }
        weapon = tmp_item;
    } else if (bio.id == "bio_laser") {
        tmp_item = weapon;
        weapon = item("bio_laser_gun", 0);
        g->refresh_all();
        g->plfire(false);
        if(weapon.charges == 1) { // not fired
            power_level += bionics[bio.id]->power_cost;
        }
        weapon = tmp_item;
    } else if(bio.id == "bio_chain_lightning") {
        tmp_item = weapon;
        weapon = item("bio_lightning", 0);
        g->refresh_all();
        g->plfire(false);
        if(weapon.charges == 1) { // not fired
            power_level += bionics[bio.id]->power_cost;
        }
        weapon = tmp_item;
    } else if (bio.id == "bio_emp") {
        if(choose_adjacent(_("Create an EMP where?"), dirx, diry)) {
            g->emp_blast(dirx, diry);
        } else {
            power_level += bionics["bio_emp"]->power_cost;
        }
    } else if (bio.id == "bio_hydraulics") {
        add_msg(m_good, _("Your muscles hiss as hydraulic strength fills them!"));
        // Sound of hissing hydraulic muscle! (not quite as loud as a car horn)
        g->sound(posx, posy, 19, _("HISISSS!"));
    } else if (bio.id == "bio_water_extractor") {
        bool extracted = false;
        for (std::vector<item>::iterator it = g->m.i_at(posx, posy).begin();
             it != g->m.i_at(posx, posy).end(); ++it) {
            if (it->type->id == "corpse" ) {
                int avail = 0;
                if ( it->item_vars.find("remaining_water") != it->item_vars.end() ) {
                    avail = atoi ( it->item_vars["remaining_water"].c_str() );
                } else {
                    avail = it->volume() / 2;
                }
                if(avail > 0 && query_yn(_("Extract water from the %s"), it->tname().c_str())) {
                    item water = item("water_clean", 0);
                    if (g->handle_liquid(water, true, true)) {
                        moves -= 100;
                    } else if (query_yn(_("Drink directly from the condenser?"))) {
                        inv.push_back(water);
                        consume(inv.position_by_type(water.typeId()));
                        moves -= 350;
                    }
                    extracted = true;
                    avail--;
                    it->item_vars["remaining_water"] = string_format("%d", avail);
                    break;
                }
            }
        }
        if (!extracted) {
            power_level += bionics["bio_water_extractor"]->power_cost;
        }
    } else if(bio.id == "bio_magnet") {
        for (int i = posx - 10; i <= posx + 10; i++) {
            for (int j = posy - 10; j <= posy + 10; j++) {
                if (g->m.i_at(i, j).size() > 0) {
                    int t; //not sure why map:sees really needs this, but w/e
                    if (g->m.sees(i, j, posx, posy, -1, t)) {
                        traj = line_to(i, j, posx, posy, t);
                    } else {
                        traj = line_to(i, j, posx, posy, 0);
                    }
                }
                traj.insert(traj.begin(), point(i, j));
                if( g->m.has_flag( "SEALED", i, j ) ) {
                    continue;
                }
                for (unsigned k = 0; k < g->m.i_at(i, j).size(); k++) {
                    tmp_item = g->m.i_at(i, j)[k];
                    if( (tmp_item.made_of("iron") || tmp_item.made_of("steel")) &&
                        tmp_item.weight() < weight_capacity() ) {
                        g->m.i_rem(i, j, k);
                        std::vector<point>::iterator it;
                        for (it = traj.begin(); it != traj.end(); ++it) {
                            int index = g->mon_at(it->x, it->y);
                            if (index != -1) {
                                g->zombie(index).apply_damage( this, bp_torso, tmp_item.weight() / 225 );
                                g->m.add_item_or_charges(it->x, it->y, tmp_item);
                                break;
                            } else if (g->m.move_cost(it->x, it->y) == 0) {
                                if (it != traj.begin()) {
                                    g->m.bash( it->x, it->y, tmp_item.weight() / 225 );
                                    if (g->m.move_cost(it->x, it->y) == 0) {
                                        g->m.add_item_or_charges((it - 1)->x, (it - 1)->y, tmp_item);
                                        break;
                                    }
                                } else {
                                    g->m.bash( it->x, it->y, tmp_item.weight() / 225 );
                                    if (g->m.move_cost(it->x, it->y) == 0) {
                                        break;
                                    }
                                }
                            }
                        }
                        if (it == traj.end()) {
                            g->m.add_item_or_charges(posx, posy, tmp_item);
                        }
                    }
                }
            }
        }
        moves -= 100;
    } else if(bio.id == "bio_lockpick") {
        if(!choose_adjacent(_("Activate your bio lockpick where?"), dirx, diry)) {
            power_level += bionics["bio_lockpick"]->power_cost;
            return;
        }
        ter_id type = g->m.ter(dirx, diry);
        if (type  == t_door_locked || type == t_door_locked_alarm || type == t_door_locked_interior) {
            moves -= 40;
            std::string door_name = rm_prefix(_("<door_name>door"));
            add_msg_if_player(m_neutral, _("With a satisfying click, the lock on the %s opens."),
                              door_name.c_str());
            g->m.ter_set(dirx, diry, t_door_c);
            // Locked metal doors are the Lab and Bunker entries.  Those need to stay locked.
        } else if(type == t_door_bar_locked) {
            moves -= 40;
            std::string door_name = rm_prefix(_("<door_name>door"));
            add_msg_if_player(m_neutral, _("The bars swing open..."),
                              door_name.c_str()); //Could better copy the messages from lockpick....
            g->m.ter_set(dirx, diry, t_door_bar_o);
        } else if(type == t_chaingate_l) {
            moves -= 40;
            std::string gate_name = rm_prefix (_("<door_name>gate"));
            add_msg_if_player(m_neutral, _("With a satisfying click, the chain-link gate opens."),
                              gate_name.c_str());
            g->m.ter_set(dirx, diry, t_chaingate_c);
        } else if (type  == t_door_locked_peep) {
            moves -= 40;
            std::string door_name = rm_prefix(_("<door_name>door"));
            add_msg_if_player(m_neutral, _("With a satisfying click, the peephole-door's lock opens."),
                              door_name.c_str());
            g->m.ter_set(dirx, diry, t_door_c_peep);
        } else if(type == t_door_c) {
            add_msg(m_info, _("That door isn't locked."));
        } else {
            add_msg_if_player(m_neutral, _("You can't unlock that %s."),
                              g->m.tername(dirx, diry).c_str());
        }
    } else if(bio.id == "bio_flashbang") {
        add_msg_if_player(m_neutral, _("You activate your integrated flashbang generator!"));
        g->flashbang(posx, posy, true);
    } else if(bio.id == "bio_shockwave") {
        g->shockwave(posx, posy, 3, 4, 2, 8, true);
        add_msg_if_player(m_neutral, _("You unleash a powerful shockwave!"));
    }
}

void player::deactivate_bionic(int b)
{
    bionic bio = my_bionics[b];

    if (bio.id == "bio_cqb") {
        // check if player knows current style naturally, otherwise drop them back to style_none
        if (style_selected != "style_none") {
            bool has_style = false;
            for (std::vector<matype_id>::iterator it = ma_styles.begin();
                 it != ma_styles.end(); ++it) {
                if (*it == style_selected) {
                    has_style = true;
                }
            }
            if (!has_style) {
                style_selected = "style_none";
            }
        }
    }
}

void bionics_uninstall_failure(player *u)
{
    switch (rng(1, 5)) {
    case 1:
        add_msg(m_neutral, _("You flub the removal."));
        break;
    case 2:
        add_msg(m_neutral, _("You mess up the removal."));
        break;
    case 3:
        add_msg(m_neutral, _("The removal fails."));
        break;
    case 4:
        add_msg(m_neutral, _("The removal is a failure."));
        break;
    case 5:
        add_msg(m_neutral, _("You screw up the removal."));
        break;
    }
    add_msg(m_bad, _("Your body is severely damaged!"));
    u->hurtall(rng(30, 80));
}

// bionic manipulation chance of success
int bionic_manip_cos(int p_int, int s_electronics, int s_firstaid, int s_mechanics,
                     int bionic_difficulty)
{
    int pl_skill = p_int * 4 +
                   s_electronics * 4 +
                   s_firstaid    * 3 +
                   s_mechanics   * 1;

    // Medical residents have some idea what they're doing
    if (g->u.has_trait("PROF_MED")) {
        pl_skill += 3;
        add_msg(m_neutral, _("You prep yourself to begin surgery."));
    }

    // for chance_of_success calculation, shift skill down to a float between ~0.4 - 30
    float adjusted_skill = float (pl_skill) - std::min( float (40),
                           float (pl_skill) - float (pl_skill) / float (10.0));

    // we will base chance_of_success on a ratio of skill and difficulty
    // when skill=difficulty, this gives us 1.  skill < difficulty gives a fraction.
    float skill_difficulty_parameter = float(adjusted_skill / (4.0 * bionic_difficulty));

    // when skill == difficulty, chance_of_success is 50%. Chance of success drops quickly below that
    // to reserve bionics for characters with the appropriate skill.  For more difficult bionics, the
    // curve flattens out just above 80%
    int chance_of_success = int((100 * skill_difficulty_parameter) /
                                (skill_difficulty_parameter + sqrt( 1 / skill_difficulty_parameter)));

    return chance_of_success;
}

bool player::uninstall_bionic(bionic_id b_id)
{
    // malfunctioning bionics don't have associated items and get a difficulty of 12
    int difficulty = 12;
    if( item_controller->has_template(b_id) > 0) {
        const it_bionic *type = dynamic_cast<it_bionic *> (item_controller->find_template(b_id));
        difficulty = type->difficulty;
    }

    if (!has_bionic(b_id)) {
        popup(_("You don't have this bionic installed."));
        return false;
    }
    if (!(inv.has_items_with_quality("CUT", 1, 1) && has_amount("1st_aid", 1))) {
        popup(_("Removing bionics requires a cutting tool and a first aid kit."));
        return false;
    }

    if ( b_id == "bio_blaster" ) {
        popup(_("Removing your Fusion Blaster Arm would leave you with a useless stump."));
        return false;
    }

    // removal of bionics adds +2 difficulty over installation
    int chance_of_success = bionic_manip_cos(int_cur,
                            skillLevel("electronics"),
                            skillLevel("firstaid"),
                            skillLevel("mechanics"),
                            difficulty + 2);

    if (!query_yn(_("WARNING: %i percent chance of failure and SEVERE bodily damage! Remove anyway?"),
                  100 - chance_of_success)) {
        return false;
    }

    use_charges("1st_aid", 1);

    practice( "electronics", int((100 - chance_of_success) * 1.5) );
    practice( "firstaid", int((100 - chance_of_success) * 1.0) );
    practice( "mechanics", int((100 - chance_of_success) * 0.5) );

    int success = chance_of_success - rng(1, 100);

    if (success > 0) {
        add_memorial_log(pgettext("memorial_male", "Removed bionic: %s."),
                         pgettext("memorial_female", "Removed bionic: %s."),
                         bionics[b_id]->name.c_str());
        // until bionics can be flagged as non-removable
        add_msg(m_neutral, _("You jiggle your parts back into their familiar places."));
        add_msg(m_good, _("Successfully removed %s."), bionics[b_id]->name.c_str());
        remove_bionic(b_id);
        g->m.spawn_item(posx, posy, "burnt_out_bionic", 1);
    } else {
        add_memorial_log(pgettext("memorial_male", "Removed bionic: %s."),
                         pgettext("memorial_female", "Removed bionic: %s."),
                         bionics[b_id]->name.c_str());
        bionics_uninstall_failure(this);
    }
    g->refresh_all();
    return true;
}

bool player::install_bionics(it_bionic *type)
{
    if (type == NULL) {
        debugmsg("Tried to install NULL bionic");
        return false;
    }
    if (bionics.count(type->id) == 0) {
        popup("invalid / unknown bionic id %s", type->id.c_str());
        return false;
    }
    if (has_bionic(type->id)) {
        if (!(type->id == "bio_power_storage" || type->id == "bio_power_storage_mkII")) {
            popup(_("You have already installed this bionic."));
            return false;
        }
    }
    int chance_of_success = bionic_manip_cos(int_cur,
                            skillLevel("electronics"),
                            skillLevel("firstaid"),
                            skillLevel("mechanics"),
                            type->difficulty);

    if (!query_yn(
            _("WARNING: %i percent chance of genetic damage, blood loss, or damage to existing bionics! Install anyway?"),
            100 - chance_of_success)) {
        return false;
    }
    int pow_up = 0;
    if (type->id == "bio_power_storage" || type->id == "bio_power_storage_mkII") {
        pow_up = BATTERY_AMOUNT;
        if (type->id == "bio_power_storage_mkII") {
            pow_up = 250;
        }
    }

    practice( "electronics", int((100 - chance_of_success) * 1.5) );
    practice( "firstaid", int((100 - chance_of_success) * 1.0) );
    practice( "mechanics", int((100 - chance_of_success) * 0.5) );
    int success = chance_of_success - rng(1, 100);
    if (success > 0) {
        add_memorial_log(pgettext("memorial_male", "Installed bionic: %s."),
                         pgettext("memorial_female", "Installed bionic: %s."),
                         bionics[type->id]->name.c_str());
        if (pow_up) {
            max_power_level += pow_up;
            add_msg_if_player(m_good, _("Increased storage capacity by %i"), pow_up);
        } else {
            add_msg(m_good, _("Successfully installed %s."), bionics[type->id]->name.c_str());
            add_bionic(type->id);
        }
    } else {
        add_memorial_log(pgettext("memorial_male", "Installed bionic: %s."),
                         pgettext("memorial_female", "Installed bionic: %s."),
                         bionics[type->id]->name.c_str());
        bionics_install_failure(this, type, success);
    }
    g->refresh_all();
    return true;
}

void bionics_install_failure(player *u, it_bionic *type, int success)
{
    // "success" should be passed in as a negative integer representing how far off we
    // were for a successful install.  We use this to determine consequences for failing.
    success = abs(success);

    // it would be better for code reuse just to pass in skill as an argument from install_bionic
    // pl_skill should be calculated the same as in install_bionics
    int pl_skill = u->int_cur * 4 +
                   u->skillLevel("electronics") * 4 +
                   u->skillLevel("firstaid")    * 3 +
                   u->skillLevel("mechanics")   * 1;
    // Medical resients get a substantial assist here
    if (u->has_trait("PROF_MED")) {
        pl_skill += 6;
    }

    // for failure_level calculation, shift skill down to a float between ~0.4 - 30
    float adjusted_skill = float (pl_skill) - std::min( float (40),
                           float (pl_skill) - float (pl_skill) / float (10.0));

    // failure level is decided by how far off the character was from a successful install, and
    // this is scaled up or down by the ratio of difficulty/skill.  At high skill levels (or low
    // difficulties), only minor consequences occur.  At low skill levels, severe consequences
    // are more likely.
    int failure_level = int(sqrt(success * 4.0 * type->difficulty / float (adjusted_skill)));
    int fail_type = (failure_level > 5 ? 5 : failure_level);

    if (fail_type <= 0) {
        add_msg(m_neutral, _("The installation fails without incident."));
        return;
    }

    switch (rng(1, 5)) {
    case 1:
        add_msg(m_neutral, _("You flub the installation."));
        break;
    case 2:
        add_msg(m_neutral, _("You mess up the installation."));
        break;
    case 3:
        add_msg(m_neutral, _("The installation fails."));
        break;
    case 4:
        add_msg(m_neutral, _("The installation is a failure."));
        break;
    case 5:
        add_msg(m_neutral, _("You screw up the installation."));
        break;
    }

    if (u->has_trait("PROF_MED")) {
    //~"Complications" is USian medical-speak for "unintended damage from a medical procedure".
        add_msg(m_neutral, _("Your training helps you minimize the complications."));
    // In addition to the bonus, medical residents know enough OR protocol to avoid botching.
    // Take MD and be immune to faulty bionics.
        if (fail_type == 5) {
            fail_type = rng(1,3);
        }
    }

    if (fail_type == 3 && u->num_bionics() == 0) {
        fail_type = 2;    // If we have no bionics, take damage instead of losing some
    }

    switch (fail_type) {

    case 1:
        if (!(u->has_trait("NOPAIN"))) {
            add_msg(m_bad, _("It really hurts!"));
            u->mod_pain( rng(failure_level * 3, failure_level * 6) );
        }
        break;

    case 2:
        add_msg(m_bad, _("Your body is damaged!"));
        u->hurtall(rng(failure_level, failure_level * 2));
        break;

    case 3:
        if (u->num_bionics() <= failure_level && u->max_power_level == 0) {
            add_msg(m_bad, _("All of your existing bionics are lost!"));
        } else {
            add_msg(m_bad, _("Some of your existing bionics are lost!"));
        }
        for (int i = 0; i < failure_level && u->remove_random_bionic(); i++) {
            ;
        }
        break;

    case 4:
        add_msg(m_mixed, _("You do damage to your genetics, causing mutation!"));
        while (failure_level > 0) {
            u->mutate();
            failure_level -= rng(1, failure_level + 2);
        }
        break;

    case 5: {
        add_msg(m_bad, _("The installation is faulty!"));
        std::vector<bionic_id> valid;
        for (std::vector<std::string>::iterator it = faulty_bionics.begin() ; it != faulty_bionics.end();
             ++it) {
            if (!u->has_bionic(*it)) {
                valid.push_back(*it);
            }
        }
        if (valid.empty()) { // We've got all the bad bionics!
            if (u->max_power_level > 0) {
                int old_power = u->max_power_level;
                add_msg(m_bad, _("You lose power capacity!"));
                u->max_power_level = rng(0, u->max_power_level - 25);
                g->u.add_memorial_log(pgettext("memorial_male", "Lost %d units of power capacity."),
                                      pgettext("memorial_female", "Lost %d units of power capacity."),
                                      old_power - u->max_power_level);
            }
            // TODO: What if we can't lose power capacity?  No penalty?
        } else {
            int index = rng(0, valid.size() - 1);
            u->add_bionic(valid[index]);
            u->add_memorial_log(pgettext("memorial_male", "Installed bad bionic: %s."),
                                pgettext("memorial_female", "Installed bad bionic: %s."),
                                bionics[valid[index]]->name.c_str());
        }
    }
    break;
    }
}

void reset_bionics()
{
    for (std::map<bionic_id, bionic_data *>::iterator bio = bionics.begin(); bio != bionics.end();
         ++bio) {
        delete bio->second;
    }
    bionics.clear();
    faulty_bionics.clear();
    power_source_bionics.clear();
    unpowered_bionics.clear();
}

void load_bionic(JsonObject &jsobj)
{
    std::string id = jsobj.get_string("id");
    std::string name = _(jsobj.get_string("name").c_str());
    std::string description = _(jsobj.get_string("description").c_str());
    int cost = jsobj.get_int("cost", 0);
    int time = jsobj.get_int("time", 0);
    bool faulty = jsobj.get_bool("faulty", false);
    bool power_source = jsobj.get_bool("power_source", false);
    bool active = jsobj.get_bool("active", false);

    if (faulty) {
        faulty_bionics.push_back(id);
    }
    if (power_source) {
        power_source_bionics.push_back(id);
    }
    if (!active) {
        unpowered_bionics.push_back(id);
    }

    bionics[id] = new bionic_data(name, power_source, active, cost, time, description, faulty);
}
