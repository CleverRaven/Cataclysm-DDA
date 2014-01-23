#include "item_factory.h"
#include "mondeath.h"
#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "monstergenerator.h"
#include <math.h>  // rounding
#include <sstream>

void mdeath::normal(monster *z) {
    if (g->u_see(z)) {
        g->add_msg(_("The %s dies!"), z->name().c_str());
    }
    if(z->type->difficulty >= 30) {
        // TODO: might not be killed by the player (g->u)!
        g->u.add_memorial_log(pgettext("memorial_male","Killed a %s."),
            pgettext("memorial_female", "Killed a %s."),
            z->name().c_str());
    }

    m_size monSize = (z->type->size);
    bool isFleshy = (z->made_of("flesh") || z->made_of("hflesh"));
    bool leaveCorpse = !(z->type->has_flag(MF_VERMIN));

    // leave some blood if we have to
    if (isFleshy && z->has_flag(MF_WARM) && !z->has_flag(MF_VERMIN)) {
        g->m.add_field(z->posx(), z->posy(), fd_blood, 1);
    }

    int maxHP = z->type->hp;
    if (!maxHP) {
        maxHP = 1;
    }

    float overflowDamage = -(z->hp);
    float corpseDamage = 5 * (overflowDamage / (maxHP * 2));

    if (leaveCorpse) {
        int gibAmount = int(floor(corpseDamage)) - 1;
        // allow one extra gib per 5 HP
        int gibLimit = 1 + (maxHP / 5.0);
        if (gibAmount > gibLimit) {
            gibAmount = gibLimit;
        }
        bool pulverized = (corpseDamage > 5 && overflowDamage > 150);
        if (!pulverized) {
            make_mon_corpse(z, int(floor(corpseDamage)));
        } else if (monSize >= MS_MEDIUM) {
            gibAmount += rng(1,6);
        }
        // Limit chunking to flesh and veggy creatures until other kinds are supported.
        bool leaveGibs = (isFleshy || z->made_of("veggy"));
        if (leaveGibs) {
            make_gibs( z, gibAmount);
        }
    }
}

void mdeath::acid(monster *z) {
    if (g->u_see(z)) {
        g->add_msg(_("The %s's body dissolves into acid."), z->name().c_str());
    }
    g->m.add_field(z->posx(), z->posy(), fd_acid, 3);
}

void mdeath::boomer(monster *z) {
    std::string tmp;
    g->sound(z->posx(), z->posy(), 24, _("a boomer explode!"));
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            g->m.bash(z->posx() + i, z->posy() + j, 10, tmp);
            g->m.add_field(z->posx() + i, z->posy() + j, fd_bile, 1);
            int mondex = g->mon_at(z->posx() + i, z->posy() +j);
            if (mondex != -1) {
                g->zombie(mondex).stumble(false);
                g->zombie(mondex).moves -= 250;
            }
        }
    }
    if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) == 1) {
        g->u.infect("boomered", bp_eyes, 2, 24, false, 1, 1);
    }
}

void mdeath::kill_vines(monster *z) {
    std::vector<int> vines;
    std::vector<int> hubs;
    for (int i = 0; i < g->num_zombies(); i++) {
        bool isHub = g->zombie(i).type->id == "mon_creeper_hub";
        if (isHub && (g->zombie(i).posx() != z->posx() || g->zombie(i).posy() != z->posy())) {
            hubs.push_back(i);
        }
        if (g->zombie(i).type->id == "mon_creeper_vine") {
            vines.push_back(i);
        }
    }

    int curX, curY;
    for (int i = 0; i < vines.size(); i++) {
        monster *vine = &(g->zombie(vines[i]));
        int dist = rl_dist(vine->posx(), vine->posy(), z->posx(), z->posy());
        bool closer = false;
        for (int j = 0; j < hubs.size() && !closer; j++) {
            curX = g->zombie(hubs[j]).posx();
            curY = g->zombie(hubs[j]).posy();
            if (rl_dist(vine->posx(), vine->posy(), curX, curY) < dist) {
                closer = true;
            }
        }
        if (!closer) {
            vine->hp = 0;
        }
    }
}

void mdeath::vine_cut(monster *z) {
    std::vector<int> vines;
    for (int x = z->posx() - 1; x <= z->posx() + 1; x++) {
        for (int y = z->posy() - 1; y <= z->posy() + 1; y++) {
            if (x == z->posx() && y == z->posy()) {
                y++; // Skip ourselves
            }
            int mondex = g->mon_at(x, y);
            if (mondex != -1 && g->zombie(mondex).type->id == "mon_creeper_vine") {
                vines.push_back(mondex);
            }
        }
    }

    for (int i = 0; i < vines.size(); i++) {
        bool found_neighbor = false;
        monster *vine = &(g->zombie( vines[i] ));
        for (int x = vine->posx() - 1; x <= vine->posx() + 1 && !found_neighbor; x++) {
            for (int y = vine->posy() - 1; y <= vine->posy() + 1 && !found_neighbor; y++) {
                if (x != z->posx() || y != z->posy()) {
                    // Not the dying vine
                    int mondex = g->mon_at(x, y);
                    if (mondex != -1 && (g->zombie(mondex).type->id == "mon_creeper_hub" ||
                                         g->zombie(mondex).type->id == "mon_creeper_vine")) {
                        found_neighbor = true;
                    }
                }
            }
        }
        if (!found_neighbor) {
            vine->hp = 0;
        }
    }
}

void mdeath::triffid_heart(monster *z) {
    if (g->u_see(z)) {
        g->add_msg(_("The surrounding roots begin to crack and crumble."));
    }
    g->add_event(EVENT_ROOTS_DIE, int(g->turn) + 100);
}

void mdeath::fungus(monster *z) {
    mdeath::normal( z);
    monster spore(GetMType("mon_spore"));
    bool fungal = false;
    int mondex = -1;
    int sporex, sporey;
    //~ the sound of a fungus dying
    g->sound(z->posx(), z->posy(), 10, _("Pouf!"));
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            sporex = z->posx() + i;
            sporey = z->posy() + j;
            mondex = g->mon_at(sporex, sporey);
            if (g->m.move_cost(sporex, sporey) > 0) {
                if (mondex != -1) {
                    // Spores hit a monster
                    fungal = g->zombie(mondex).type->in_species("FUNGUS");
                    if (g->u_see(sporex, sporey) && !fungal) {
                        g->add_msg(_("The %s is covered in tiny spores!"),
                                   g->zombie(mondex).name().c_str());
                    }
                    if (!g->zombie(mondex).make_fungus()) {
                        g->kill_mon(mondex, (z->friendly != 0));
                    }
                } else if (g->u.posx == sporex && g->u.posy == sporey) {
                    // Spores hit the player
                        if (g->u.has_trait("TAIL_CATTLE") && one_in(20 - g->u.dex_cur - g->u.skillLevel("melee"))) {
                        g->add_msg(_("The spores land on you, but you quickly swat them off with your tail!"));
                        return;
                        }
                    bool hit = false;
                    if (one_in(4) && g->u.infect("spores", bp_head, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (one_in(2) && g->u.infect("spores", bp_torso, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_arms, 3, 90, false, 1, 3, 120, 1, true, 1)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_arms, 3, 90, false, 1, 3, 120, 1, true, 0)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_legs, 3, 90, false, 1, 3, 120, 1, true, 1)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_legs, 3, 90, false, 1, 3, 120, 1, true, 0)) {
                        hit = true;
                    }
                    if (hit && (g->u.has_trait("TAIL_CATTLE") && one_in(20 - g->u.dex_cur - g->u.skillLevel("melee")))) {
                        g->add_msg(_("The spores land on you, but you quickly swat them off with your tail!"));
                        hit = false;
                    }
                    if (hit) {
                        g->add_msg(_("You're covered in tiny spores!"));
                    }
                } else if (one_in(2) && g->num_zombies() <= 1000) {
                    // Spawn a spore
                    spore.spawn(sporex, sporey);
                    g->add_zombie(spore);
                }
            }
        }
    }
}

void mdeath::disintegrate(monster *z) {
    if (g->u_see(z)) {
        g->add_msg(_("The %s disintegrates!"), z->name().c_str());
    }
}

void mdeath::worm(monster *z) {
    if (g->u_see(z))
        g->add_msg(_("The %s splits in two!"), z->name().c_str());

    std::vector <point> wormspots;
    int wormx, wormy;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            wormx = z->posx() + i;
            wormy = z->posy() + j;
            if (g->m.has_flag("DIGGABLE", wormx, wormy) &&
                    !(g->u.posx == wormx && g->u.posy == wormy)) {
                wormspots.push_back(point(wormx, wormy));
            }
        }
    }
    int worms = 0;
    while(worms < 2 && wormspots.size() > 0) {
        monster worm(GetMType("mon_halfworm"));
        int rn = rng(0, wormspots.size() - 1);
        if(-1 == g->mon_at(wormspots[rn])) {
            worm.spawn(wormspots[rn].x, wormspots[rn].y);
            g->add_zombie(worm);
            worms++;
        }
        wormspots.erase(wormspots.begin() + rn);
    }
}

void mdeath::disappear(monster *z) {
    g->add_msg(_("The %s disappears."), z->name().c_str());
}

void mdeath::guilt(monster *z) {
    const int MAX_GUILT_DISTANCE = 5;
    int kill_count = g->kill_count(z->type->id);
    int maxKills = 100; // this is when the player stop caring altogether.

    // different message as we kill more of the same monster
    std::string msg = "You feel guilty for killing %s."; // default guilt message
    std::map<int, std::string> guilt_tresholds;
    guilt_tresholds[75] = "You feel ashamed for killing %s.";
    guilt_tresholds[50] = "You regret killing %s.";
    guilt_tresholds[25] = "You feel remorse for killing %s.";

    if (g->u.has_trait("PSYCHOPATH") || g->u.has_trait("PRED3") || g->u.has_trait("PRED4") ) {
        return;
    }
    if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > MAX_GUILT_DISTANCE) {
        // Too far away, we can deal with it.
        return;
    }
    if (z->hp >= 0) {
        // We probably didn't kill it
        return;
    }
    if (kill_count >= maxKills) {
        // player no longer cares
        if (kill_count == maxKills) {
            g->add_msg(_("After killing so many bloody %ss you no longer care "
                          "about their deaths anymore."), z->name().c_str());
        }
        return;
    }
        else if ((g->u.has_trait("PRED1")) || (g->u.has_trait("PRED2"))) {
            msg = (_("Culling the weak is distasteful, but necessary."));
        }
        else {
        for (std::map<int, std::string>::iterator it = guilt_tresholds.begin();
                it != guilt_tresholds.end(); it++) {
            if (kill_count >= it->first) {
                msg = it->second;
                break;
            }
        }
    }

    g->add_msg(_(msg.c_str()), z->name().c_str());

    int moraleMalus = -50 * (1.0 - ((float) kill_count / maxKills));
    int maxMalus = -250 * (1.0 - ((float) kill_count / maxKills));
    int duration = 300 * (1.0 - ((float) kill_count / maxKills));
    int decayDelay = 30 * (1.0 - ((float) kill_count / maxKills));
    if (z->type->in_species("ZOMBIE")) {
        moraleMalus /= 10;
        if (g->u.has_trait("PACIFIST")) {
            moraleMalus *= 5;
        }
        else if (g->u.has_trait("PRED1")) {
            moraleMalus /= 4;
        }
        else if (g->u.has_trait("PRED2")) {
            moraleMalus /= 5;
        }
    }
    g->u.add_morale(MORALE_KILLED_MONSTER, moraleMalus, maxMalus, duration, decayDelay);

}
void mdeath::blobsplit(monster *z) {
    int speed = z->speed - rng(30, 50);
    g->m.spawn_item(z->posx(), z->posy(), "slime_scrap", 1, 0, g->turn, rng(1,4));
    if (speed <= 0) {
        if (g->u_see(z)) {
            //  TODO:  Add vermin-tagged tiny versions of the splattered blob  :)
            g->add_msg(_("The %s splatters apart."), z->name().c_str());
        }
        return;
    }
    monster blob(GetMType((speed < 50 ? "mon_blob_small" : "mon_blob")));
    blob.speed = speed;
    // If we're tame, our kids are too
    blob.friendly = z->friendly;
    if (g->u_see(z)) {
        g->add_msg(_("The %s splits in two!"), z->name().c_str());
    }
    blob.hp = blob.speed;
    std::vector <point> valid;

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            bool moveOK = (g->m.move_cost(z->posx()+i, z->posy()+j) > 0);
            bool monOK = g->mon_at(z->posx()+i, z->posy()+j) == -1;
            bool posOK = (g->u.posx != z->posx()+i || g->u.posy != z->posy() + j);
            if (moveOK && monOK && posOK) {
                valid.push_back(point(z->posx()+i, z->posy()+j));
            }
        }
    }

    int rn;
    for (int s = 0; s < 2 && valid.size() > 0; s++) {
        rn = rng(0, valid.size() - 1);
        blob.spawn(valid[rn].x, valid[rn].y);
        g->add_zombie(blob);
        valid.erase(valid.begin() + rn);
    }
}

void mdeath::melt(monster *z) {
    if (g->u_see(z)) {
        g->add_msg(_("The %s melts away."), z->name().c_str());
    }
}

void mdeath::amigara(monster *z) {
    if (!g->u.has_disease("amigara")) {
        return;
    }
    int count = 0;
    for (int i = 0; i < g->num_zombies(); i++) {
        if (g->zombie(i).type->id == "mon_amigara_horror") {
            count++;
        }
    }
    if (count <= 1) { // We're the last!
        g->u.rem_disease("amigara");
        g->add_msg(_("Your obsession with the fault fades away..."));
        item art(new_artifact(itypes), g->turn);
        g->m.add_item_or_charges(z->posx(), z->posy(), art);
    }
    normal( z);
}

void mdeath::thing(monster *z) {
    monster thing(GetMType("mon_thing"));
    thing.spawn(z->posx(), z->posy());
    g->add_zombie(thing);
}

void mdeath::explode(monster *z) {
    int size = 0;
    switch (z->type->size) {
        case MS_TINY:
            size = 4;  break;
        case MS_SMALL:
            size = 8;  break;
        case MS_MEDIUM:
            size = 14; break;
        case MS_LARGE:
            size = 20; break;
        case MS_HUGE:
            size = 26; break;
    }
    g->explosion(z->posx(), z->posy(), size, 0, false);
}

void mdeath::broken(monster *z) {
  if (z->type->id == "mon_manhack") {
    g->m.spawn_item(z->posx(), z->posy(), "broken_manhack", 1, 0, g->turn);
  }
  else {
    debugmsg("Tried to create a broken %s but it does not exist.", z->type->name.c_str());
  }
}

void mdeath::ratking(monster *z) {
    g->u.rem_disease("rat");
    if (g->u_see(z)) {
        g->add_msg(_("Rats suddenly swarm into view."));
    }

    std::vector <point> ratspots;
    int ratx, raty;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            ratx = z->posx() + i;
            raty = z->posy() + i;
            if (g->m.move_cost(ratx, raty) > 0 && g->mon_at(ratx, raty) == -1 &&
                  !(g->u.posx == ratx && g->u.posy == raty)) {
                ratspots.push_back(point(ratx, raty));
            }
        }
    }
    int rn;
    monster rat(GetMType("mon_sewer_rat"));
    for (int rats = 0; rats < 7 && ratspots.size() > 0; rats++) {
        rn = rng(0, ratspots.size() - 1);
        rat.spawn(ratspots[rn].x, ratspots[rn].y);
        g->add_zombie(rat);
        ratspots.erase(ratspots.begin() + rn);
    }
}

void mdeath::smokeburst(monster *z) {
    std::string tmp;
    g->sound(z->posx(), z->posy(), 24, _("a smoker explode!"));
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            g->m.add_field(z->posx() + i, z->posy() + j, fd_smoke, 3);
            int mondex = g->mon_at(z->posx() + i, z->posy() +j);
            if (mondex != -1) {
                g->zombie(mondex).stumble(false);
                g->zombie(mondex).moves -= 250;
            }
        }
    }
}

// this function generates clothing for zombies
void mdeath::zombie(monster *z) {
    // normal death function first
    mdeath::normal( z);

    // skip clothing generation if the zombie was rezzed rather than spawned
    if (z->no_extra_death_drops) {
        return;
    }

    // now generate appropriate clothing
    char dropset = -1;
    std::string zid = z->type->id;
    bool underwear = true;
    if (zid == "mon_zombie_cop"){ dropset = 0;}
    else if (zid == "mon_zombie_swimmer"){ dropset = 1;}
    else if (zid == "mon_zombie_scientist"){ dropset = 2;}
    else if (zid == "mon_zombie_soldier"){ dropset = 3;}
    else if (zid == "mon_zombie_hulk"){ dropset = 4;}
    else if (zid == "mon_zombie_hazmat"){ dropset = 5;}
    else if (zid == "mon_zombie_fireman"){ dropset = 6;}
    else if (zid == "mon_zombie_survivor"){ dropset = 7;}
    switch(dropset) {
        case 0: // mon_zombie_cop
            g->m.put_items_from("cop_shoes", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("cop_torso", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("cop_pants", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
        break;

        case 1: // mon_zombie_swimmer
            if (one_in(10)) {
                //Wetsuit zombie
                g->m.put_items_from("swimmer_wetsuit", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1, 4));
            } else {
                if (!one_in(4)) {
                    g->m.put_items_from("swimmer_head", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1, 4));
                }
                if (one_in(3)) {
                    g->m.put_items_from("swimmer_torso", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1, 4));
                }
                g->m.put_items_from("swimmer_pants", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1, 4));
                if (one_in(4)) {
                    g->m.put_items_from("swimmer_shoes", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1, 4));
                }
            }
            underwear = false;
        break;

        case 2: // mon_zombie_scientist
            g->m.put_items_from("lab_shoes", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("lab_torso", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("lab_pants", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
        break;

        case 3: // mon_zombie_soldier
            g->m.put_items_from("cop_shoes", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("mil_armor_torso", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("mil_armor_pants", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            if (one_in(3))
            {
                g->m.put_items_from("mil_armor_helmet", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            }
        break;

        case 4: // mon_zombie_hulk
            g->m.spawn_item(z->posx(), z->posy(), "rag", 1, 0, g->turn, rng(1,4));
            g->m.put_items_from("pants", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            underwear = false;
        break;

        case 5: // mon_zombie_hazmat
            if (one_in(5)) {
            g->m.put_items_from("hazmat_full", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            } else {
                g->m.put_items_from("hazmat_torso", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
                g->m.put_items_from("hazmat_gloves", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
                g->m.put_items_from("hazmat_boots", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
                g->m.put_items_from("hazmat_mask", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));

                if (one_in(3)) {
                    g->m.put_items_from("hazmat_eyes", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1, 4));
                }
            }
        break;

        case 6: // mon_zombie_fireman
            g->m.put_items_from("fireman_torso", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("fireman_pants", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("fireman_gloves", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("fireman_boots", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("fireman_mask", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("fireman_head", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));

            if (one_in(3)) {
                g->m.put_items_from("hazmat_eyes", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1, 4));
            }
        break;

        case 7: // mon_zombie_survivor
            g->m.put_items_from("survivorzed_gloves", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("survivorzed_boots", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("survivorzed_head", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("survivorzed_extra", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            if (one_in(4)) {
                g->m.put_items_from("survivorzed_suits", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            } else {
                g->m.put_items_from("survivorzed_tops", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
                g->m.put_items_from("survivorzed_bottoms", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            }
            if (one_in(3)) {
                underwear = false;
                g->m.put_items_from("loincloth", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            }
        break;


        default:
            g->m.put_items_from("pants", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("shirts", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("shoes", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            if (one_in(5)) {
                g->m.put_items_from("jackets", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            }
            if (one_in(15)) {
                g->m.put_items_from("bags", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            }
        break;
    }

    // Give underwear if needed
    if (underwear) {
        if (one_in(2)) {
            g->m.put_items_from("female_underwear_bottom", 1, z->posx(), z->posy(),
                                    g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("female_underwear_top", 1, z->posx(), z->posy(),
                                    g->turn, 0, 0, rng(1,4));
        }
        else {
            g->m.put_items_from("male_underwear_bottom", 1, z->posx(), z->posy(),
                                    g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("male_underwear_top", 1, z->posx(), z->posy(),
                                    g->turn, 0, 0, rng(1,4));
        }
    }
}

void mdeath::gameover(monster *z) {
    g->add_msg(_("The %s was destroyed!  GAME OVER!"), z->name().c_str());
    g->u.hp_cur[hp_torso] = 0;
}

void mdeath::kill_breathers(monster *z)
{
    (void)z; //unused
    for (int i = 0; i < g->num_zombies(); i++) {
        const std::string monID = g->zombie(i).type->id;
        if (monID == "mon_breather_hub " || monID == "mon_breather") {
            g->zombie(i).dead = true;
        }
    }
}

void make_gibs(monster* z, int amount) {
    if (amount <= 0) {
        return;
    }
    const field_id gibType = (z->made_of("veggy") ? fd_gibs_veggy : fd_gibs_flesh);
    const int zposx = z->posx();
    const int zposy = z->posy();
    const bool warm = z->has_flag(MF_WARM);
    for (int i = 0; i < amount; i++) {
        // leave gibs, if there are any
        const int gibX = zposx + rng(0,6) - 3;
        const int gibY = zposy + rng(0,6) - 3;
        const int gibDensity = rng(1, i+1);
        int junk;
        if( g->m.clear_path( zposx, zposy, gibX, gibY, 3, 1, 100, junk ) ) {
            // Only place gib if there's a clear path for it to get there.
            g->m.add_field(gibX, gibY, gibType, gibDensity);
        }
        if( warm ) {
            const int bloodX = zposx + (rng(0,2) - 1);
            const int bloodY = zposy + (rng(0,2) - 1);
            if( g->m.clear_path( zposx, zposy, bloodX, bloodY, 2, 1, 100, junk ) ) {
                // Only place blood if there's a clear path for it to get there.
                g->m.add_field(bloodX, bloodY, fd_blood, 1);
            }
        }
    }
}

void make_mon_corpse(monster* z, int damageLvl) {
    const int MAX_DAM = 4;
    item corpse;
    corpse.make_corpse(itypes["corpse"], z->type, g->turn);
    corpse.damage = damageLvl > MAX_DAM ? MAX_DAM : damageLvl;
    g->m.add_item_or_charges(z->posx(), z->posy(), corpse);
}
