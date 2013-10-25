#include "item_factory.h"
#include "mondeath.h"
#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "monstergenerator.h"
#include <sstream>

void mdeath::normal(game *g, monster *z) {
    const int CORPSE_DAM_MAX = 4;
    int monSize = z->type->size;

    if (g->u_see(z)) {
        g->add_msg(_("The %s dies!"), z->name().c_str());
    }
    if(z->type->difficulty >= 30) {
        g->u.add_memorial_log(_("Killed a %s."), z->name().c_str());
    }

    bool warmBlooded = z->has_flag(MF_WARM);
    if (z->made_of("flesh") && warmBlooded) {
        g->m.add_field(g, z->posx(), z->posy(), fd_blood, 1);
    }

    bool isFleshy = (z->made_of("flesh") || z->made_of("veggy") || z->made_of("hflesh"));
    bool leaveCorpse = !(z->type->has_flag(MF_VERMIN));
    if (leaveCorpse) {
        int maxHP = z->type->hp;
        signed int overflowDamage = -(z->hp);

        // determine how much of a mess is left, for flesh and veggy creatures

        int gibAmount = -2;
        int corpseDamage = 0;

        for (int i = 5; i >= 1; i--) {
            if (overflowDamage > maxHP / i) {
                corpseDamage += 1;
                if (i > 5 && isFleshy) {
                    gibAmount += rng(1,3);
                }
            }
        }

        if (!isFleshy && (overflowDamage < maxHP * 2) && (monSize < (int)MS_MEDIUM)) {
            // the corpse still exists, let's place it
            item corpse;
            corpse.make_corpse(g->itypes["corpse"], z->type, g->turn);
            corpse.damage = corpseDamage > CORPSE_DAM_MAX ? CORPSE_DAM_MAX : corpseDamage;
            g->m.add_item_or_charges(z->posx(), z->posy(), corpse);
        }

        // leave gibs
        for (int i = 0; i < gibAmount; i++) {
            const int gibX = z->posx() + rng(1,6) - 3;
            const int gibY = z->posy() + rng(1,6) - 3;
            const int gibDensity = rng(1, 3);
            const field_id gibType = (z->made_of("veggy") ? fd_gibs_veggy : fd_gibs_flesh);
            g->m.add_field(g, gibX, gibY, gibType, gibDensity);
        }
    }
}

void mdeath::acid(game *g, monster *z) {
    if (g->u_see(z)) {
        g->add_msg(_("The %s's body dissolves into acid."), z->name().c_str());
    }
    g->m.add_field(g, z->posx(), z->posy(), fd_acid, 3);
}

void mdeath::boomer(game *g, monster *z) {
    std::string tmp;
    g->sound(z->posx(), z->posy(), 24, _("a boomer explodes!"));
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            g->m.bash(z->posx() + i, z->posy() + j, 10, tmp);
            g->m.add_field(g, z->posx() + i, z->posy() + j, fd_bile, 1);
            int mondex = g->mon_at(z->posx() + i, z->posy() +j);
            if (mondex != -1) {
                g->zombie(mondex).stumble(g, false);
                g->zombie(mondex).moves -= 250;
            }
        }
    }
    if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) == 1) {
        g->u.infect("boomered", bp_eyes, 2, 24, false, 1, 1);
    }
}

void mdeath::kill_vines(game *g, monster *z) {
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

void mdeath::vine_cut(game *g, monster *z) {
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

void mdeath::triffid_heart(game *g, monster *z) {
    g->add_msg(_("The root walls begin to crumble around you."));
    g->add_event(EVENT_ROOTS_DIE, int(g->turn) + 100);
}

void mdeath::fungus(game *g, monster *z) {
    mdeath::normal(g, z);
    monster spore(GetMType("mon_spore"));
    bool fungal;
    int sporex, sporey;
    int mondex;
    //~ the sound of a fungus dying
    g->sound(z->posx(), z->posy(), 10, _("Pouf!"));
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            sporex = z->posx() + i;
            sporey = z->posy() + j;
            mondex = g->mon_at(sporex, sporey);
            fungal = g->zombie(mondex).type->in_species("FUNGUS");
            if (g->m.move_cost(sporex, sporey) > 0) {
                if (mondex != -1) {
                    // Spores hit a monster
                    if (g->u_see(sporex, sporey) && !fungal) {
                        g->add_msg(_("The %s is covered in tiny spores!"),
                                   g->zombie(mondex).name().c_str());
                    }
                    if (!g->zombie(mondex).make_fungus(g)) {
                        g->kill_mon(mondex, (z->friendly != 0));
                    }
                } else if (g->u.posx == sporex && g->u.posy == sporey) {
                    // Spores hit the player
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

void mdeath::disintegrate(game *g, monster *z) {
    if (g->u_see(z)) {
        g->add_msg(_("It disintegrates!"));
    }
}

void mdeath::worm(game *g, monster *z) {
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

void mdeath::disappear(game *g, monster *z) {
    g->add_msg(_("The %s disappears!  Was it in your head?"), z->name().c_str());
}

void mdeath::guilt(game *g, monster *z) {
    const int MAX_GUILT_DISTANCE = 5;

    /*  TODO:   Replace default cannibal checks with more elaborate conditions,
                 and add a "PSYCHOPATH" trait for terminally guilt-free folk.
                 Guilty cannibals could make for good drama!
    */
    if (g->u.has_trait("CANNIBAL")) {
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

    g->add_msg(_("Killing %s fills you with guilt."), z->name().c_str());

    int moraleMalus = 50;
    int maxMalus = -250;
    int duration = 300;
    int decayDelay = 30;
    if (z->type->in_species("ZOMBIE")) {
        moraleMalus /= 10;
    }
    g->u.add_morale(MORALE_KILLED_MONSTER, moraleMalus, maxMalus, duration, decayDelay);

}
void mdeath::blobsplit(game *g, monster *z) {
    int speed = z->speed - rng(30, 50);
    if (speed <= 0) {
        if (g->u_see(z)) {
            //  TODO:  Add vermin-tagged tiny versions of the splattered blob  :)
            g->add_msg(_("The %s splatters apart."), z->name().c_str());
            return;
        }
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

void mdeath::melt(game *g, monster *z) {
    if (g->u_see(z)) {
        g->add_msg(_("The %s melts away."), z->name().c_str());
    }
}

void mdeath::amigara(game *g, monster *z) {
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
        item art(g->new_artifact(), g->turn);
        g->m.add_item_or_charges(z->posx(), z->posy(), art);
    }
    normal(g, z);
}

void mdeath::thing(game *g, monster *z) {
    monster thing(GetMType("mon_thing"));
    thing.spawn(z->posx(), z->posy());
    g->add_zombie(thing);
}

void mdeath::explode(game *g, monster *z) {
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

void mdeath::ratking(game *g, monster *z) {
    g->u.rem_disease("rat");
    if (g->u_see(z)) {
        g->add_msg(_("Swarming rats converge on you."));
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

void mdeath::smokeburst(game *g, monster *z) {
    std::string tmp;
    g->sound(z->posx(), z->posy(), 24, _("a smoker explodes!"));
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            g->m.add_field(g, z->posx() + i, z->posy() + j, fd_smoke, 3);
            int mondex = g->mon_at(z->posx() + i, z->posy() +j);
            if (mondex != -1) {
                g->zombie(mondex).stumble(g, false);
                g->zombie(mondex).moves -= 250;
            }
        }
    }
}

// this function generates clothing for zombies
void mdeath::zombie(game *g, monster *z) {
    // normal death function first
    mdeath::normal(g, z);

    // skip clothing generation if the zombie was rezzed rather than spawned
    if (z->no_extra_death_drops) {
        return;
    }

    // now generate appropriate clothing
    char dropset = -1;
    std::string zid = z->type->id;
    if (zid == "mon_zombie_cop"){ dropset = 0;}
    else if (zid == "mon_zombie_swimmer"){ dropset = 1;}
    else if (zid == "mon_zombie_scientist"){ dropset = 2;}
    else if (zid == "mon_zombie_soldier"){ dropset = 3;}
    else if (zid == "mon_zombie_hulk"){ dropset = 4;}
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
            g->m.spawn_item(z->posx(), z->posy(), "rag", g->turn, 0, 0, rng(5,10));
            g->m.put_items_from("pants", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            break;

        default:
            g->m.put_items_from("pants", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("shirts", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            if (one_in(5)) {
                g->m.put_items_from("jackets", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            }
            if (one_in(15)) {
                g->m.put_items_from("bags", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            }
        break;
    }
}

void mdeath::gameover(game *g, monster *z) {
    g->add_msg(_("Your %s was destroyed!  GAME OVER!"), z->name().c_str());
    g->u.hp_cur[hp_torso] = 0;
}

void mdeath::kill_breathers(game *g, monster *z) {
    for (int i = 0; i < g->num_zombies(); i++) {
        std::string monID = g->zombie(i).type->id;
        if (monID == "mon_breather_hub " || monID == "mon_breather") {
            g->zombie(i).dead = true;
        }
    }
}
