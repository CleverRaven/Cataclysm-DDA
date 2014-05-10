#include "game.h"
#include "trap.h"
#include "rng.h"
#include "monstergenerator.h"
#include "messages.h"

// A pit becomes less effective as it fills with corpses.
float pit_effectiveness(int x, int y)
{
    int corpse_volume = 0;
    for (size_t i = 0; i < g->m.i_at(x, y).size(); i++) {
        item &j = g->m.i_at(x, y)[i];
        if (j.type->id == "corpse") {
            corpse_volume += j.volume();
        }
    }

    int filled_volume = 75 * 10; // 10 zombies; see item::volume

    return std::max(0.0f, 1.0f - float(corpse_volume) / filled_volume);
}

void trapfunc::bubble(Creature *c, int x, int y)
{
    // tiny animals don't trigger bubble wrap
    if (c != NULL && c->get_size() == MS_TINY) {
        return;
    }
    if (c != NULL) {
        c->add_msg_player_or_npc(m_warning, _("You step on some bubble wrap!"),
                                 _("<npcname> steps on some bubble wrap!"));
        c->add_memorial_log(pgettext("memorial_male", "Stepped on bubble wrap."),
                            pgettext("memorial_female", "Stepped on bubble wrap."));
    }
    g->sound(x, y, 18, _("Pop!"));
    g->m.remove_trap(x, y);
}

void trapfunc::cot(Creature *c, int, int)
{
    monster *z = dynamic_cast<monster *>(c);
    if (z != NULL) {
        // Haha, only monsters stumble over a cot, humans are smart.
        add_msg(m_good, _("The %s stumbles over the cot"), z->name().c_str());
        c->moves -= 100;
    }
}

void trapfunc::beartrap(Creature *c, int x, int y)
{
    // tiny animals don't trigger bear traps
    if (c != NULL && c->get_size() == MS_TINY) {
        return;
    }
    g->sound(x, y, 8, _("SNAP!"));
    if (c != NULL) {
        c->add_memorial_log(pgettext("memorial_male", "Caught by a beartrap."),
                            pgettext("memorial_female", "Caught by a beartrap."));
        c->add_msg_player_or_npc(m_bad, _("A bear trap closes on your foot!"),
                                 _("A bear trap closes on <npcname>'s foot!"));
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (z != NULL) {
            z->moves = 0;
            z->add_effect("beartrap", rng(8, 15));
            item beartrap(itypes["beartrap"], 0);
            z->add_item(beartrap);
            z->hurt(35);
        } else if (n != NULL) {
            n->hit(NULL, bp_legs, random_side(bp_legs), 10, 16);
            n->add_disease("beartrap", 1, true);
            g->m.spawn_item(x, y, "beartrap");
        }
    } else {
        g->m.spawn_item(x, y, "beartrap");
    }
    g->m.remove_trap(x, y);
}

void trapfunc::board(Creature *c, int, int)
{
    // tiny animals don't trigger spiked boards, they can squeeze between the nails
    if (c != NULL && c->get_size() == MS_TINY) {
        return;
    }
    if (c != NULL) {
        c->add_memorial_log(pgettext("memorial_male", "Stepped on a spiked board."),
                            pgettext("memorial_female", "Stepped on a spiked board."));
        c->add_msg_player_or_npc(m_bad, _("You step on a spiked board!"),
                                 _("<npcname> steps on a spiked board!"));
        monster *z = dynamic_cast<monster *>(c);
        if (z != NULL) {
            z->moves -= 80;
            z->hurt(rng(6, 10));
        } else {
            c->hit(NULL, bp_feet, 0, 0, rng(6, 10));
            c->hit(NULL, bp_feet, 1, 0, rng(6, 10));
        }
    }
}

void trapfunc::caltrops(Creature *c, int, int)
{
    // tiny animals don't trigger caltrops, they can squeeze between them
    if (c != NULL && c->get_size() == MS_TINY) {
        return;
    }
    if (c != NULL) {
        c->add_memorial_log(pgettext("memorial_male", "Stepped on a caltrop."),
                            pgettext("memorial_female", "Stepped on a caltrop."));
        c->add_msg_player_or_npc(m_bad, _("You step on a sharp metal caltrop!"),
                                 _("<npcname> steps on a sharp metal caltrop!"));
        monster *z = dynamic_cast<monster *>(c);
        if (z != NULL) {
            z->moves -= 80;
            z->hurt(rng(18, 30));
        } else {
            c->hit(NULL, bp_feet, 0, 0, rng(9, 30));
            c->hit(NULL, bp_feet, 1, 0, rng(9, 30));
        }
    }
}

void trapfunc::tripwire(Creature *c, int x, int y)
{
    // tiny animals don't trigger tripwires, they just squeeze under it
    if (c != NULL && c->get_size() == MS_TINY) {
        return;
    }
    if (c != NULL) {
        c->add_memorial_log(pgettext("memorial_male", "Tripped on a tripwire."),
                            pgettext("memorial_female", "Tripped on a tripwire."));
        c->add_msg_player_or_npc(m_bad, _("You trip over a tripwire!"),
                                 _("<npcname> trips over a tripwire!"));
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (z != NULL) {
            z->stumble(false);
            if (rng(0, 10) > z->get_dodge()) {
                z->hurt(rng(1, 4));
            }
        } else if (n != NULL) {
            std::vector<point> valid;
            point jk;
            for (jk.x = x - 1; jk.x <= x + 1; jk.x++) {
                for (jk.y = y - 1; jk.y <= y + 1; jk.y++) {
                    if (g->is_empty(jk.x, jk.y)) {
                        // No monster, NPC, or player, plus valid for movement
                        valid.push_back(jk);
                    }
                }
            }
            if (!valid.empty()) {
                jk = valid[rng(0, valid.size() - 1)];
                n->posx = jk.x;
                n->posy = jk.y;
            }
            n->moves -= 150;
            if (rng(5, 20) > n->dex_cur) {
                n->hurtall(rng(1, 4));
            }
            if (c == &g->u) {
                g->update_map(g->u.posx, g->u.posy);
            }
        }
    }
}

void trapfunc::crossbow(Creature *c, int x, int y)
{
    bool add_bolt = true;
    if (c != NULL) {
        c->add_msg_player_or_npc(m_neutral, _("You trigger a crossbow trap!"),
                                 _("<npcname> triggers a crossbow trap!"));
        c->add_memorial_log(pgettext("memorial_male", "Triggered a crossbow trap."),
                            pgettext("memorial_female", "Triggered a crossbow trap."));
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (n != NULL) {
            if (!one_in(4) && rng(8, 20) > n->get_dodge()) {
                body_part hit = num_bp;
                switch (rng(1, 10)) {
                    case  1:
                        hit = bp_feet;
                        break;
                    case  2:
                    case  3:
                    case  4:
                        hit = bp_legs;
                        break;
                    case  5:
                    case  6:
                    case  7:
                    case  8:
                    case  9:
                        hit = bp_torso;
                        break;
                    case 10:
                        hit = bp_head;
                        break;
                }
                int side = random_side(hit);
                n->add_msg_if_player(m_bad, _("Your %s is hit!"), body_part_name(hit, side).c_str());
                n->hit(NULL, hit, side, 0, rng(20, 30));
                add_bolt = !one_in(10);
            } else {
                n->add_msg_player_or_npc(m_neutral, _("You dodge the shot!"),
                                         _("<npcname> dodges the shot!"));
            }
        } else if (z != NULL) {
            bool seen = g->u_see(z);
            int chance = 0;
            // adapted from shotgun code - chance of getting hit depends on size
            switch (z->type->size) {
                case MS_TINY:
                    chance = 50;
                    break;
                case MS_SMALL:
                    chance =  8;
                    break;
                case MS_MEDIUM:
                    chance =  6;
                    break;
                case MS_LARGE:
                    chance =  4;
                    break;
                case MS_HUGE:
                    chance =  1;
                    break;
            }
            if (one_in(chance)) {
                if (seen) {
                    add_msg(m_bad, _("A bolt shoots out and hits the %s!"), z->name().c_str());
                }
                z->hurt(rng(20, 30));
                add_bolt = !one_in(10);
            } else if (seen) {
                add_msg(m_neutral, _("A bolt shoots out, but misses the %s."), z->name().c_str());
            }
        }
    }
    g->m.remove_trap(x, y);
    g->m.spawn_item(x, y, "crossbow");
    g->m.spawn_item(x, y, "string_6");
    if (add_bolt) {
        g->m.spawn_item(x, y, "bolt_steel", 1, 1);
    }
}

void trapfunc::shotgun(Creature *c, int x, int y)
{
    g->sound(x, y, 60, _("Kerblam!"));
    int shots = 1;
    if (c != NULL) {
        c->add_msg_player_or_npc(m_neutral, _("You trigger a shotgun trap!"),
                                 _("<npcname> triggers a shotgun trap!"));
        c->add_memorial_log(pgettext("memorial_male", "Triggered a shotgun trap."),
                            pgettext("memorial_female", "Triggered a shotgun trap."));
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (n != NULL) {
            shots = (one_in(8) || one_in(20 - n->str_max) ? 2 : 1);
            if (g->m.tr_at(x, y) == tr_shotgun_1) {
                shots = 1;
            }
            if (rng(5, 50) > n->get_dodge()) {
                body_part hit = num_bp;
                switch (rng(1, 10)) {
                    case  1:
                        hit = bp_feet;
                        break;
                    case  2:
                    case  3:
                    case  4:
                        hit = bp_legs;
                        break;
                    case  5:
                    case  6:
                    case  7:
                    case  8:
                    case  9:
                        hit = bp_torso;
                        break;
                    case 10:
                        hit = bp_head;
                        break;
                }
                int side = random_side(hit);
                n->add_msg_if_player(m_bad, _("Your %s is hit!"), body_part_name(hit, side).c_str());
                n->hit(NULL, hit, side, 0, rng(40 * shots, 60 * shots));
            } else {
                n->add_msg_player_or_npc(m_neutral, _("You dodge the shot!"),
                                         _("<npcname> dodges the shot!"));
            }
        } else if (z != NULL) {
            bool seen = g->u_see(z);
            int chance = 0;
            switch (z->type->size) {
                case MS_TINY:
                    chance = 100;
                    break;
                case MS_SMALL:
                    chance =  16;
                    break;
                case MS_MEDIUM:
                    chance =  12;
                    break;
                case MS_LARGE:
                    chance =   8;
                    break;
                case MS_HUGE:
                    chance =   2;
                    break;
            }
            shots = (one_in(8) || one_in(chance) ? 2 : 1);
            if (g->m.tr_at(x, y) == tr_shotgun_1) {
                shots = 1;
            }
            if (seen) {
                add_msg(m_bad, _("A shotgun fires and hits the %s!"), z->name().c_str());
            }
            z->hurt(rng(40 * shots, 60 * shots));
        }
    }
    if (shots == 2 || g->m.tr_at(x, y) == tr_shotgun_1) {
        g->m.remove_trap(x, y);
        g->m.spawn_item(x, y, "shotgun_sawn");
        g->m.spawn_item(x, y, "string_6");
    } else {
        g->m.add_trap(x, y, tr_shotgun_1);
    }
}


void trapfunc::blade(Creature *c, int, int)
{
    if (c != NULL) {
        c->add_msg_player_or_npc(m_bad, _("A blade swings out and hacks your torso!"),
                                 _("A blade swings out and hacks <npcname>s torso!"));
        c->add_memorial_log(pgettext("memorial_male", "Triggered a blade trap."),
                            pgettext("memorial_female", "Triggered a blade trap."));
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (n != NULL) {
            n->hit(NULL, bp_torso, -1, 12, 30);
        } else if (z != NULL) {
            int cutdam = std::max(0, 30 - z->get_armor_cut(bp_torso));
            int bashdam = std::max(0, 12 - z->get_armor_bash(bp_torso));
            z->hurt(bashdam + cutdam);
        }
    }
}

void trapfunc::snare_light(Creature *c, int x, int y)
{
    g->sound(x, y, 2, _("Snap!"));
    g->m.remove_trap(x, y);
    g->m.spawn_item(x, y, "string_36");
    g->m.spawn_item(x, y, "snare_trigger");
    // large animals will trigger and destroy the trap, but not get harmed
    if (c != NULL && c->get_size() >= MS_LARGE) {
        c->add_msg_if_npc(m_neutral, _("The snare has no effect on <npcname>!"));
        return;
    }
    if (c == NULL) {
        return;
    }
    c->add_msg_player_or_npc(m_bad, _("A snare closes on your leg."),
                             _("A snare closes on <npcname>s leg."));
    c->add_memorial_log(pgettext("memorial_male", "Triggered a light snare."),
                        pgettext("memorial_female", "Triggered a light snare."));
    monster *z = dynamic_cast<monster *>(c);
    player *n = dynamic_cast<player *>(c);
    if (n != NULL) {
        n->add_disease("lightsnare", rng(10, 20));
    } else if (z != NULL) {
        switch (z->type->size) {
            case MS_TINY:
                z->add_effect("beartrap", 1, 1, true);
                z->hurt(10);
                break;
            case MS_SMALL:
                z->moves = 0;
                z->add_effect("beartrap", rng(100, 150));
                break;
            case MS_MEDIUM:
                z->moves = 0;
                z->add_effect("beartrap", rng(20, 30));
                break;
            default:
                break;
        }
    }
}

void trapfunc::snare_heavy(Creature *c, int x, int y)
{
    g->sound(x, y, 4, _("Snap!"));
    g->m.remove_trap(x, y);
    g->m.spawn_item(x, y, "rope_6");
    g->m.spawn_item(x, y, "snare_trigger");
    // large animals will trigger and destroy the trap, but not get harmed
    if (c != NULL && c->get_size() >= MS_HUGE) {
        c->add_msg_if_npc(m_neutral, _("The snare has no effect on <npcname>!"));
        return;
    }
    if (c == NULL) {
        return;
    }
    int side = one_in(2) ? 0 : 1;
    body_part hit = bp_legs;
    c->add_msg_player_or_npc(m_bad, _("A snare closes on your %s."),
                             _("A snare closes on <npcname>s %s."), body_part_name(hit, side).c_str());
    c->add_memorial_log(pgettext("memorial_male", "Triggered a heavy snare."),
                        pgettext("memorial_female", "Triggered a heavy snare."));
    monster *z = dynamic_cast<monster *>(c);
    player *n = dynamic_cast<player *>(c);
    if (n != NULL) {
        n->hit(NULL, bp_legs, side, 15, 20);
        n->add_disease("heavysnare", rng(20, 30));
    } else if (z != NULL) {
        int damage;
        switch (z->type->size) {
            case MS_TINY:
                damage = 20;
                z->add_effect("beartrap", 1, 1, true);
                break;
            case MS_SMALL:
                z->add_effect("beartrap", 1, 1, true);
                damage = 20;
                break;
            case MS_MEDIUM:
                z->add_effect("beartrap", rng(100, 150));
                damage = 10;
                break;
            case MS_LARGE:
                z->add_effect("beartrap", rng(20, 30));
                damage = 0;
                break;
            default:
                damage = 0;
        }
        z->moves = 0;
        z->hurt(damage);
    }
}

void trapfunc::landmine(Creature *c, int x, int y)
{
    // tiny animals are too light to trigger land mines
    if (c != NULL && c->get_size() == MS_TINY) {
        return;
    }
    if (c != NULL) {
        c->add_msg_player_or_npc(m_bad, _("You trigger a land mine!"),
                                 _("<npcname> triggers a land mine!"));
        c->add_memorial_log(pgettext("memorial_male", "Stepped on a land mine."),
                            pgettext("memorial_female", "Stepped on a land mine."));
    }
    g->explosion(x, y, 10, 8, false);
    g->m.remove_trap(x, y);
}

void trapfunc::boobytrap(Creature *c, int x, int y)
{
    if (c != NULL) {
        c->add_msg_player_or_npc(m_bad, _("You trigger a booby trap!"),
                                 _("<npcname> triggers a booby trap!"));
        c->add_memorial_log(pgettext("memorial_male", "Triggered a booby trap."),
                            pgettext("memorial_female", "Triggered a booby trap."));
    }
    g->explosion(x, y, 18, 12, false);
    g->m.remove_trap(x, y);
}

void trapfunc::telepad(Creature *c, int x, int y)
{
    //~ the sound of a telepad functioning
    g->sound(x, y, 6, _("vvrrrRRMM*POP!*"));
    if (c != NULL) {
        monster *z = dynamic_cast<monster *>(c);
        // TODO: NPC don't teleport?
        if (c == &g->u) {
            c->add_msg_if_player(m_warning, _("The air shimmers around you..."));
            c->add_memorial_log(pgettext("memorial_male", "Triggered a teleport trap."),
                                pgettext("memorial_female", "Triggered a teleport trap."));
            g->teleport();
        } else if (z != NULL) {
            if (g->u_see(z)) {
                add_msg(_("The air shimmers around the %s..."), z->name().c_str());
            }

            int tries = 0;
            int newposx, newposy;
            do {
                newposx = rng(z->posx() - SEEX, z->posx() + SEEX);
                newposy = rng(z->posy() - SEEY, z->posy() + SEEY);
                tries++;
            } while (g->m.move_cost(newposx, newposy) == 0 && tries != 10);

            if (tries == 10) {
                g->explode_mon(g->mon_at(z->posx(), z->posy()));
            } else {
                int mon_hit = g->mon_at(newposx, newposy);
                if (mon_hit != -1) {
                    if (g->u_see(z)) {
                        add_msg(m_good, _("The %s teleports into a %s, killing them both!"),
                                z->name().c_str(), g->zombie(mon_hit).name().c_str());
                    }
                    g->explode_mon(mon_hit);
                } else {
                    z->setpos(newposx, newposy);
                }
            }
        }
    }
}

void trapfunc::goo(Creature *c, int x, int y)
{
    if (c != NULL) {
        c->add_msg_player_or_npc(m_bad, _("You step in a puddle of thick goo."),
                                 _("<npcname> steps in a puddle of thick goo."));
        c->add_memorial_log(pgettext("memorial_male", "Stepped into thick goo."),
                            pgettext("memorial_female", "Stepped into thick goo."));
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (n != NULL) {
            n->infect("slimed", bp_feet, 6, 20);
            if (one_in(3)) {
                n->add_msg_if_player(m_bad, _("The acidic goo eats away at your feet."));
                n->hit(NULL, bp_feet, 0, 0, 5);
                n->hit(NULL, bp_feet, 1, 0, 5);
            }
        } else if (z != NULL) {
            if (z->type->id == "mon_blob") {
                z->speed += 15;
                z->hp = z->speed;
            } else {
                z->poly(GetMType("mon_blob"));
                z->speed -= 15;
                z->hp = z->speed;
            }
        }
    }
    g->m.remove_trap(x, y);
}

void trapfunc::dissector(Creature *c, int x, int y)
{
    //~ the sound of a dissector dissecting
    g->sound(x, y, 10, _("BRZZZAP!"));
    if (c != NULL) {
        c->add_msg_player_or_npc(m_bad, _("Electrical beams emit from the floor and slice your flesh!"),
                                 _("Electrical beams emit from the floor and slice <npcname>s flesh!"));
        c->add_memorial_log(pgettext("memorial_male", "Stepped into a dissector."),
                            pgettext("memorial_female", "Stepped into a dissector."));
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (n != NULL) {
            n->hit(NULL, bp_head,  -1, 0, 15);
            n->hit(NULL, bp_torso, -1, 0, 20);
            n->hit(NULL, bp_arms,  0, 0, 12);
            n->hit(NULL, bp_arms,  1, 0, 12);
            n->hit(NULL, bp_hands, 0, 0, 10);
            n->hit(NULL, bp_hands, 1, 0, 10);
            n->hit(NULL, bp_legs,  0, 0, 12);
            n->hit(NULL, bp_legs,  1, 0, 12);
            n->hit(NULL, bp_feet,  0, 0, 10);
            n->hit(NULL, bp_feet,  1, 0, 10);
        } else if (z != NULL) {
            if (z->hurt(60)) {
                g->explode_mon(g->mon_at(x, y));
            }
        }
    }
}

void trapfunc::pit(Creature *c, int x, int y)
{
    // tiny animals aren't hurt by falling into pits
    if (c->get_size() == MS_TINY) {
        return;
    }
    if (c != NULL) {
        const float eff = pit_effectiveness(x, y);
        c->add_msg_player_or_npc(m_bad, _("You fall in a pit!"), _("<npcname> falls in a pit!"));
        c->add_memorial_log(pgettext("memorial_male", "Fell in a pit."),
                            pgettext("memorial_female", "Fell in a pit."));
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (n != NULL) {
            if ( (n->has_trait("WINGS_BIRD")) || ((one_in(2)) && (n->has_trait("WINGS_BUTTERFLY"))) ) {
                n->add_msg_if_player(_("You flap your wings and flutter down gracefully."));
            } else {
                int dodge = n->get_dodge();
                int damage = eff * rng(10, 20) - rng(dodge, dodge * 5);
                if (damage > 0) {
                    n->add_msg_if_player(m_bad, _("You hurt yourself!"));
                    n->hurtall(rng(int(damage / 2), damage));
                    n->hit(NULL, bp_legs, 0, damage, 0);
                    n->hit(NULL, bp_legs, 1, damage, 0);
                } else {
                    n->add_msg_if_player(_("You land nimbly."));
                }
            }
            n->add_disease("in_pit", 1, true);
        } else if (z != NULL) {
            z->moves = -1000;
            z->hurt(eff * rng(10, 20));
        }
    }
}

void trapfunc::pit_spikes(Creature *c, int x, int y)
{
    // tiny animals aren't hurt by falling into spiked pits
    if (c->get_size() == MS_TINY) {
        return;
    }
    if (c != NULL) {
        c->add_msg_player_or_npc(m_bad, _("You fall in a spiked pit!"),
                                 _("<npcname> falls in a spiked pit!"));
        c->add_memorial_log(pgettext("memorial_male", "Fell into a spiked pit."),
                            pgettext("memorial_female", "Fell into a spiked pit."));
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (n != NULL) {
            int dodge = n->get_dodge();
            int damage = pit_effectiveness(x, y) * rng(20, 50);
            if ( (n->has_trait("WINGS_BIRD")) || ((one_in(2)) && (n->has_trait("WINGS_BUTTERFLY"))) ) {
                n->add_msg_if_player(_("You flap your wings and flutter down gracefully."));
            } else if (0 == damage || rng(5, 30) < dodge) {
                n->add_msg_if_player(_("You avoid the spikes within."));
            } else {
                body_part hit = num_bp;
                switch (rng(1, 10)) {
                    case  1:
                    case  2:
                        hit = bp_legs;
                        break;
                    case  3:
                    case  4:
                        hit = bp_arms;
                        break;
                    case  5:
                    case  6:
                    case  7:
                    case  8:
                    case  9:
                    case 10:
                        hit = bp_torso;
                        break;
                }
                int side = random_side(hit);
                n->add_msg_if_player(m_bad, _("The spikes impale your %s!"), body_part_name(hit, side).c_str());
                n->hit(NULL, hit, side, 0, damage);
            }
            n->add_disease("in_pit", 1, true);
        } else if (z != NULL) {
            z->moves = -1000;
            z->hurt(rng(20, 50));
        }
    }
    if (one_in(4)) {
        if (g->u_see(x, y)) {
            add_msg(_("The spears break!"));
        }
        g->m.ter_set(x, y, t_pit);
        g->m.add_trap(x, y, tr_pit);
        for (int i = 0; i < 4; i++) { // 4 spears to a pit
            if (one_in(3)) {
                g->m.spawn_item(x, y, "pointy_stick");
            }
        }
    }
}

void trapfunc::lava(Creature *c, int x, int y)
{
    if (c != NULL) {
        c->add_msg_player_or_npc(m_bad, _("The %s burns you horribly!"), _("The %s burns <npcname>!"),
                                 g->m.tername(x, y).c_str());
        c->add_memorial_log(pgettext("memorial_male", "Stepped into lava."),
                            pgettext("memorial_female", "Stepped into lava."));
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (n != NULL) {
            n->hit(NULL, bp_feet, 0, 0, 20);
            n->hit(NULL, bp_feet, 1, 0, 20);
            n->hit(NULL, bp_legs, 0, 0, 20);
            n->hit(NULL, bp_legs, 1, 0, 20);
        } else if (z != NULL) {
            // MATERIALS-TODO: use fire resistance
            int dam = 30;
            if (z->made_of("flesh") || z->made_of("iflesh")) {
                dam = 50;
            }
            if (z->made_of("veggy")) {
                dam = 80;
            }
            if (z->made_of("paper") || z->made_of(LIQUID) || z->made_of("powder") ||
                z->made_of("wood")  || z->made_of("cotton") || z->made_of("wool")) {
                dam = 200;
            }
            if (z->made_of("stone")) {
                dam = 15;
            }
            if (z->made_of("kevlar") || z->made_of("steel")) {
                dam = 5;
            }
            z->hurt(dam);
        }
    }
}

// STUB
void trapfunc::portal(Creature */*c*/, int /*x*/, int /*y*/)
{
    // TODO: make this do something?
}

void trapfunc::sinkhole(Creature *c, int /*x*/, int /*y*/)
{
    if (c != &g->u) {
        // TODO: make something exciting happen here
        return;
    }
    g->u.add_msg_if_player(m_bad, _("You step into a sinkhole, and start to sink down!"));
    g->u.add_memorial_log(pgettext("memorial_male", "Stepped into a sinkhole."),
                        pgettext("memorial_female", "Stepped into a sinkhole."));
    if (g->u.has_amount("grapnel", 1) &&
        query_yn(_("There is a sinkhole here. Throw your grappling hook out to try to catch something?"))) {
        int throwroll = rng(g->u.skillLevel("throw"),
                            g->u.skillLevel("throw") + g->u.str_cur + g->u.dex_cur);
        if (throwroll >= 6) {
            add_msg(m_good, _("The grappling hook catches something!"));
            if (rng(g->u.skillLevel("unarmed"),
                    g->u.skillLevel("unarmed") + g->u.str_cur) > 4) {
                // Determine safe places for the character to get pulled to
                std::vector<point> safe;
                for (int i = g->u.posx - 1; i <= g->u.posx + 1; i++) {
                    for (int j = g->u.posy - 1; j <= g->u.posy + 1; j++) {
                        if (g->m.move_cost(i, j) > 0 && g->m.tr_at(i, j) != tr_pit) {
                            safe.push_back(point(i, j));
                        }
                    }
                }
                if (safe.empty()) {
                    add_msg(m_bad, _("There's nowhere to pull yourself to, and you sink!"));
                    g->u.use_amount("grapnel", 1);
                    g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "grapnel");
                    g->m.ter_set(g->u.posx, g->u.posy, t_hole);
                    g->vertical_move(-1, true);
                } else {
                    add_msg(m_good, _("You pull yourself to safety!  The sinkhole collapses."));
                    int index = rng(0, safe.size() - 1);
                    g->m.ter_set(g->u.posx, g->u.posy, t_hole);
                    g->u.posx = safe[index].x;
                    g->u.posy = safe[index].y;
                    g->update_map(g->u.posx, g->u.posy);
                }
            } else {
                add_msg(m_bad, _("You're not strong enough to pull yourself out..."));
                g->u.moves -= 100;
                g->u.use_amount("grapnel", 1);
                g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "grapnel");
                g->m.ter_set(g->u.posx, g->u.posy, t_hole);
                g->vertical_move(-1, true);
            }
        } else {
            add_msg(m_bad, _("Your throw misses completely, and you sink!"));
            if (one_in((g->u.str_cur + g->u.dex_cur) / 3)) {
                g->u.use_amount("grapnel", 1);
                g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "grapnel");
            }
            g->m.ter_set(g->u.posx, g->u.posy, t_hole);
            g->vertical_move(-1, true);
        }
    } else if (g->u.has_amount("rope_30", 1) &&
               query_yn(_("There is a sinkhole here. Throw your rope out to try to catch something?"))) {
        int throwroll = rng(g->u.skillLevel("throw"),
                            g->u.skillLevel("throw") + g->u.str_cur + g->u.dex_cur);
        if (throwroll >= 12) {
            add_msg(m_good, _("The rope catches something!"));
            if (rng(g->u.skillLevel("unarmed"),
                    g->u.skillLevel("unarmed") + g->u.str_cur) > 6) {
                // Determine safe places for the character to get pulled to
                std::vector<point> safe;
                for (int i = g->u.posx - 1; i <= g->u.posx + 1; i++) {
                    for (int j = g->u.posy - 1; j <= g->u.posy + 1; j++) {
                        if (g->m.move_cost(i, j) > 0 && g->m.tr_at(i, j) != tr_pit) {
                            safe.push_back(point(i, j));
                        }
                    }
                }
                if (safe.empty()) {
                    add_msg(m_bad, _("There's nowhere to pull yourself to, and you sink!"));
                    g->u.use_amount("rope_30", 1);
                    g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "rope_30");
                    g->m.ter_set(g->u.posx, g->u.posy, t_hole);
                    g->vertical_move(-1, true);
                } else {
                    add_msg(m_good, _("You pull yourself to safety!  The sinkhole collapses."));
                    int index = rng(0, safe.size() - 1);
                    g->m.ter_set(g->u.posx, g->u.posy, t_hole);
                    g->u.posx = safe[index].x;
                    g->u.posy = safe[index].y;
                    g->update_map(g->u.posx, g->u.posy);
                }
            } else {
                add_msg(m_bad, _("You're not strong enough to pull yourself out..."));
                g->u.moves -= 100;
                g->u.use_amount("rope_30", 1);
                g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "rope_30");
                g->m.ter_set(g->u.posx, g->u.posy, t_hole);
                g->vertical_move(-1, true);
            }
        } else {
            add_msg(m_bad, _("Your throw misses completely, and you sink!"));
            if (one_in((g->u.str_cur + g->u.dex_cur) / 3)) {
                g->u.use_amount("rope_30", 1);
                g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "rope_30");
            }
            g->m.ter_set(g->u.posx, g->u.posy, t_hole);
            g->vertical_move(-1, true);
        }
    } else {
        add_msg(m_bad, _("You sink into the sinkhole!"));
        g->m.ter_set(g->u.posx, g->u.posy, t_hole);
        g->vertical_move(-1, true);
    }
}

void trapfunc::ledge(Creature *c, int /*x*/, int /*y*/)
{
    if (c == &g->u) {
        add_msg(m_warning, _("You fall down a level!"));
        g->u.add_memorial_log(pgettext("memorial_male", "Fell down a ledge."),
                              pgettext("memorial_female", "Fell down a ledge."));
        g->vertical_move(-1, true);
        return;
    }
    // TODO; port to Z-levels
    if (c != NULL) {
        c->add_msg_if_npc(_("<npcname> falls down a level!"));
        c->die(NULL);
    }
}

void trapfunc::temple_flood(Creature *c, int /*x*/, int /*y*/)
{
    // Monsters and npcs are completely ignored here, should they?
    if (c == &g->u) {
        add_msg(m_warning, _("You step on a loose tile, and water starts to flood the room!"));
        g->u.add_memorial_log(pgettext("memorial_male", "Triggered a flood trap."),
                              pgettext("memorial_female", "Triggered a flood trap."));
        for (int i = 0; i < SEEX * MAPSIZE; i++) {
            for (int j = 0; j < SEEY * MAPSIZE; j++) {
                if (g->m.tr_at(i, j) == tr_temple_flood) {
                    g->m.remove_trap(i, j);
                }
            }
        }
        g->add_event(EVENT_TEMPLE_FLOOD, calendar::turn + 3);
    }
}

void trapfunc::temple_toggle(Creature *c, int x, int y)
{
    // Monsters and npcs are completely ignored here, should they?
    if (c == &g->u) {
        add_msg(_("You hear the grinding of shifting rock."));
        ter_id type = g->m.oldter(x, y);
        for (int i = 0; i < SEEX * MAPSIZE; i++) {
            for (int j = 0; j < SEEY * MAPSIZE; j++) {
                switch (type) {
                    case old_t_floor_red:
                        if (g->m.ter(i, j) == t_rock_green) {
                            g->m.ter_set(i, j, t_floor_green);
                        } else if (g->m.ter(i, j) == t_floor_green) {
                            g->m.ter_set(i, j, t_rock_green);
                        }
                        break;

                    case old_t_floor_green:
                        if (g->m.ter(i, j) == t_rock_blue) {
                            g->m.ter_set(i, j, t_floor_blue);
                        } else if (g->m.ter(i, j) == t_floor_blue) {
                            g->m.ter_set(i, j, t_rock_blue);
                        }
                        break;

                    case old_t_floor_blue:
                        if (g->m.ter(i, j) == t_rock_red) {
                            g->m.ter_set(i, j, t_floor_red);
                        } else if (g->m.ter(i, j) == t_floor_red) {
                            g->m.ter_set(i, j, t_rock_red);
                        }
                        break;

                }
            }
        }
    }
}

void trapfunc::glow(Creature *c, int x, int y)
{
    if (c != NULL) {
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (n != NULL) {
            if (one_in(3)) {
                n->add_msg_if_player(m_bad, _("You're bathed in radiation!"));
                n->radiation += rng(10, 30);
            } else if (one_in(4)) {
                n->add_msg_if_player(m_bad, _("A blinding flash strikes you!"));
                g->flashbang(x, y);
            } else {
                c->add_msg_if_player(_("Small flashes surround you."));
            }
        } else if (z != NULL && one_in(3)) {
            z->hurt( rng(5, 10) );
            z->speed *= .9;
        }
    }
}

void trapfunc::hum(Creature */*c*/, int x, int y)
{
    int volume = rng(1, 200);
    std::string sfx;
    if (volume <= 10) {
        //~ a quiet humming sound
        sfx = _("hrm");
    } else if (volume <= 50) {
        //~ a humming sound
        sfx = _("hrmmm");
    } else if (volume <= 100) {
        //~ a loud humming sound
        sfx = _("HRMMM");
    } else {
        //~ a very loud humming sound
        sfx = _("VRMMMMMM");
    }
    g->sound(x, y, volume, sfx);
}

void trapfunc::shadow(Creature *c, int x, int y)
{
    if (c != &g->u) {
        return;
    }
    // Monsters and npcs are completely ignored here, should they?
    g->u.add_memorial_log(pgettext("memorial_male", "Triggered a shadow trap."),
                          pgettext("memorial_female", "Triggered a shadow trap."));
    monster spawned(GetMType("mon_shadow"));
    int tries = 0, monx, mony, junk;
    do {
        if (one_in(2)) {
            monx = rng(g->u.posx - 5, g->u.posx + 5);
            mony = (one_in(2) ? g->u.posy - 5 : g->u.posy + 5);
        } else {
            monx = (one_in(2) ? g->u.posx - 5 : g->u.posx + 5);
            mony = rng(g->u.posy - 5, g->u.posy + 5);
        }
    } while (tries < 5 && !g->is_empty(monx, mony) &&
             !g->m.sees(monx, mony, g->u.posx, g->u.posy, 10, junk));

    if (tries < 5) {
        add_msg(m_warning, _("A shadow forms nearby."));
        spawned.sp_timeout = rng(2, 10);
        spawned.spawn(monx, mony);
        g->add_zombie(spawned);
        g->m.remove_trap(x, y);
    }
}

void trapfunc::drain(Creature *c, int, int)
{
    if (c != NULL) {
        c->add_msg_if_player(m_bad, _("You feel your life force sapping away."));
        c->add_memorial_log(pgettext("memorial_male", "Triggered a life-draining trap."),
                            pgettext("memorial_female", "Triggered a life-draining trap."));
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (n != NULL) {
            n->hurtall(1);
        } else if (z != NULL) {
            z->hurt(1);
        }
    }
}

void trapfunc::snake(Creature *c, int x, int y)
{
    //~ the sound a snake makes
    g->sound(x, y, 10, _("ssssssss"));
    if (one_in(6)) {
        g->m.remove_trap(x, y);
    }
    if (c != NULL) {
        c->add_memorial_log(pgettext("memorial_male", "Triggered a shadow snake trap."),
                            pgettext("memorial_female", "Triggered a shadow snake trap."));
    }
    if (one_in(3)) {
        monster spawned(GetMType("mon_shadow_snake"));
        int tries = 0, monx, mony, junk;
        // This spawns snakes only when the player can see them, why?
        do {
            if (one_in(2)) {
                monx = rng(g->u.posx - 5, g->u.posx + 5);
                mony = (one_in(2) ? g->u.posy - 5 : g->u.posy + 5);
            } else {
                monx = (one_in(2) ? g->u.posx - 5 : g->u.posx + 5);
                mony = rng(g->u.posy - 5, g->u.posy + 5);
            }
        } while (tries < 5 && !g->is_empty(monx, mony) &&
                 !g->m.sees(monx, mony, g->u.posx, g->u.posy, 10, junk));

        if (tries < 5) {
            add_msg(m_warning, _("A shadowy snake forms nearby."));
            spawned.spawn(monx, mony);
            g->add_zombie(spawned);
            g->m.remove_trap(x, y);
        }
    }
}

/**
 * Takes the name of a trap function and returns a function pointer to it.
 * @param function_name The name of the trapfunc function to find.
 * @return A function pointer to the matched function, or to trapfunc::none if
 *         there is no match.
 */
trap_function trap_function_from_string(std::string function_name)
{
    if("none" == function_name) {
        return &trapfunc::none;
    }
    if("bubble" == function_name) {
        return &trapfunc::bubble;
    }
    if("cot" == function_name) {
        return &trapfunc::cot;
    }
    if("beartrap" == function_name) {
        return &trapfunc::beartrap;
    }
    if("board" == function_name) {
        return &trapfunc::board;
    }
    if("caltrops" == function_name) {
        return &trapfunc::caltrops;
    }
    if("tripwire" == function_name) {
        return &trapfunc::tripwire;
    }
    if("crossbow" == function_name) {
        return &trapfunc::crossbow;
    }
    if("shotgun" == function_name) {
        return &trapfunc::shotgun;
    }
    if("blade" == function_name) {
        return &trapfunc::blade;
    }
    if("snare_light" == function_name) {
        return &trapfunc::snare_light;
    }
    if("snare_heavy" == function_name) {
        return &trapfunc::snare_heavy;
    }
    if("landmine" == function_name) {
        return &trapfunc::landmine;
    }
    if("telepad" == function_name) {
        return &trapfunc::telepad;
    }
    if("goo" == function_name) {
        return &trapfunc::goo;
    }
    if("dissector" == function_name) {
        return &trapfunc::dissector;
    }
    if("sinkhole" == function_name) {
        return &trapfunc::sinkhole;
    }
    if("pit" == function_name) {
        return &trapfunc::pit;
    }
    if("pit_spikes" == function_name) {
        return &trapfunc::pit_spikes;
    }
    if("lava" == function_name) {
        return &trapfunc::lava;
    }
    if("portal" == function_name) {
        return &trapfunc::portal;
    }
    if("ledge" == function_name) {
        return &trapfunc::ledge;
    }
    if("boobytrap" == function_name) {
        return &trapfunc::boobytrap;
    }
    if("temple_flood" == function_name) {
        return &trapfunc::temple_flood;
    }
    if("temple_toggle" == function_name) {
        return &trapfunc::temple_toggle;
    }
    if("glow" == function_name) {
        return &trapfunc::glow;
    }
    if("hum" == function_name) {
        return &trapfunc::hum;
    }
    if("shadow" == function_name) {
        return &trapfunc::shadow;
    }
    if("drain" == function_name) {
        return &trapfunc::drain;
    }
    if("snake" == function_name) {
        return &trapfunc::snake;
    }

    //No match found
    debugmsg("Could not find a trapfunc function matching '%s'!", function_name.c_str());
    return &trapfunc::none;
}
