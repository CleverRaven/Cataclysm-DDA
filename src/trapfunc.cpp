#include "game.h"
#include "trap.h"
#include "rng.h"
#include "monstergenerator.h"
#include "messages.h"
#include "sounds.h"

// A pit becomes less effective as it fills with corpses.
float pit_effectiveness(int x, int y)
{
    int corpse_volume = 0;
    for( auto &pit_content : g->m.i_at( x, y ) ) {
        if( pit_content.is_corpse() ) {
            corpse_volume += pit_content.volume();
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
    sounds::sound(x, y, 18, _("Pop!"));
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
    sounds::sound(x, y, 8, _("SNAP!"));
    g->m.remove_trap(x, y);
    if (c != NULL) {
        // What got hit?
        body_part hit = num_bp;
        if (one_in(2)) {
            hit = bp_leg_l;
        } else {
            hit = bp_leg_r;
        }

        // Messages
        c->add_memorial_log(pgettext("memorial_male", "Caught by a beartrap."),
                            pgettext("memorial_female", "Caught by a beartrap."));
        c->add_msg_player_or_npc(m_bad, _("A bear trap closes on your foot!"),
                                 _("A bear trap closes on <npcname>'s foot!"));

        // Actual effects
        c->add_effect("beartrap", 1, hit, true);
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (z != NULL) {
            z->apply_damage( nullptr, hit, 30);
        } else if (n != NULL) {
            damage_instance d;
            d.add_damage( DT_BASH, 12 );
            d.add_damage( DT_CUT, 18 );
            n->deal_damage( nullptr, hit, d );
            
            if ((n->has_trait("INFRESIST")) && (one_in(512))) {
                n->add_effect("tetanus", 1, num_bp, true);
            }
            else if ((!n->has_trait("INFIMMUNE") || !n->has_trait("INFRESIST")) && (one_in(128))) {
                    n->add_effect("tetanus", 1, num_bp, true);
            }
        }
    }
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
        player *n = dynamic_cast<player *>(c);
        if (z != NULL) {
            z->moves -= 80;
            z->apply_damage( nullptr, bp_foot_l, rng( 3, 5 ) );
            z->apply_damage( nullptr, bp_foot_r, rng( 3, 5 ) );
        } else {
            c->deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, rng( 6, 10 ) ) );
            c->deal_damage( nullptr, bp_foot_r, damage_instance( DT_CUT, rng( 6, 10 ) ) );
              if ((n->has_trait("INFRESIST")) && (one_in(256))) {
                  n->add_effect("tetanus", 1, num_bp, true);
              }
              else if ((!n->has_trait("INFIMMUNE") || !n->has_trait("INFRESIST")) && (one_in(35))) {
                      n->add_effect("tetanus", 1, num_bp, true);
              }
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
            c->apply_damage( nullptr, bp_foot_l, rng( 9, 15 ) );
            c->apply_damage( nullptr, bp_foot_r, rng( 9, 15 ) );
        } else {
            c->deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, rng( 9, 30 ) ) );
            c->deal_damage( nullptr, bp_foot_r, damage_instance( DT_CUT, rng( 9, 30 ) ) );
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
                z->apply_damage( nullptr, bp_torso, rng(1, 4));
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
                n->setx( jk.x );
                n->sety( jk.y );
            }
            n->moves -= 150;
            if (rng(5, 20) > n->dex_cur) {
                n->hurtall(rng(1, 4));
            }
            if (c == &g->u) {
                g->update_map(&g->u);
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
                        if(one_in(2)) {
                            hit = bp_foot_l;
                        } else {
                            hit = bp_foot_r;
                        }
                        break;
                    case  2:
                    case  3:
                    case  4:
                        if(one_in(2)) {
                            hit = bp_leg_l;
                        } else {
                            hit = bp_leg_r;
                        }
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
                //~ %s is bodypart
                n->add_msg_if_player(m_bad, _("Your %s is hit!"), body_part_name(hit).c_str());
                c->deal_damage( nullptr, hit, damage_instance( DT_CUT, rng( 20, 30 ) ) );
                add_bolt = !one_in(10);
            } else {
                n->add_msg_player_or_npc(m_neutral, _("You dodge the shot!"),
                                         _("<npcname> dodges the shot!"));
            }
        } else if (z != NULL) {
            bool seen = g->u.sees(*z);
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
                z->apply_damage( nullptr, bp_torso, rng(20, 30));
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
    sounds::sound(x, y, 60, _("Kerblam!"));
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
                        if(one_in(2)) {
                            hit = bp_foot_l;
                        } else {
                            hit = bp_foot_r;
                        }
                        break;
                    case  2:
                    case  3:
                    case  4:
                        if(one_in(2)) {
                            hit = bp_leg_l;
                        } else {
                            hit = bp_leg_r;
                        }
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
                //~ %s is bodypart
                n->add_msg_if_player(m_bad, _("Your %s is hit!"), body_part_name(hit).c_str());
                c->deal_damage( nullptr, hit, damage_instance( DT_CUT, rng( 40 * shots, 60 * shots ) ) );
            } else {
                n->add_msg_player_or_npc(m_neutral, _("You dodge the shot!"),
                                         _("<npcname> dodges the shot!"));
            }
        } else if (z != NULL) {
            bool seen = g->u.sees(*z);
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
            z->apply_damage( nullptr, bp_torso, rng(40 * shots, 60 * shots));
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
            damage_instance d;
            d.add_damage( DT_BASH, 12 );
            d.add_damage( DT_CUT, 30 );
            n->deal_damage( nullptr, bp_torso, d );
        } else if (z != NULL) {
            int cutdam = std::max(0, 30 - z->get_armor_cut(bp_torso));
            int bashdam = std::max(0, 12 - z->get_armor_bash(bp_torso));
            // TODO: move the armor stuff above into monster::deal_damage_handle_type and call
            // Creature::hit for player *and* monster
            z->apply_damage( nullptr, bp_torso, bashdam + cutdam);
        }
    }
}

void trapfunc::snare_light(Creature *c, int x, int y)
{
    sounds::sound(x, y, 2, _("Snap!"));
    g->m.remove_trap(x, y);
    if (c != NULL) {
        // Determine what gets hit
        body_part hit = num_bp;
        if (one_in(2)) {
            hit = bp_leg_l;
        } else {
            hit = bp_leg_r;
        }
        // Messages
        c->add_msg_player_or_npc(m_bad, _("A snare closes on your leg."),
                                 _("A snare closes on <npcname>s leg."));
        c->add_memorial_log(pgettext("memorial_male", "Triggered a light snare."),
                            pgettext("memorial_female", "Triggered a light snare."));

        // Actual effects
        c->add_effect("lightsnare", 1, hit, true);
        monster *z = dynamic_cast<monster *>(c);
        if (z != NULL && z->type->size == MS_TINY) {
            z->apply_damage( nullptr, one_in( 2 ) ? bp_leg_l : bp_leg_r, 10);
        }
    }
}

void trapfunc::snare_heavy(Creature *c, int x, int y)
{
    sounds::sound(x, y, 4, _("Snap!"));
    g->m.remove_trap(x, y);
    if (c != NULL) {
        // Determine waht got hit
        body_part hit = num_bp;
        if (one_in(2)) {
            hit = bp_leg_l;
        } else {
            hit = bp_leg_r;
        }
        //~ %s is bodypart name in accusative.
        c->add_msg_player_or_npc(m_bad, _("A snare closes on your %s."),
                                 _("A snare closes on <npcname>s %s."), body_part_name_accusative(hit).c_str());
        c->add_memorial_log(pgettext("memorial_male", "Triggered a heavy snare."),
                            pgettext("memorial_female", "Triggered a heavy snare."));

        // Actual effects
        c->add_effect("heavysnare", 1, hit, true);
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (n != NULL) {
            damage_instance d;
            d.add_damage( DT_BASH, 10 );
            n->deal_damage( nullptr, hit, d );
        } else if (z != NULL) {
            int damage;
            switch (z->type->size) {
                case MS_TINY:
                    damage = 20;
                    break;
                case MS_SMALL:
                    damage = 20;
                    break;
                case MS_MEDIUM:
                    damage = 10;
                    break;
                default:
                    damage = 0;
            }
            z->apply_damage( nullptr, hit, damage);
        }
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
    sounds::sound(x, y, 6, _("vvrrrRRMM*POP!*"));
    if (c != NULL) {
        monster *z = dynamic_cast<monster *>(c);
        // TODO: NPC don't teleport?
        if (c == &g->u) {
            c->add_msg_if_player(m_warning, _("The air shimmers around you..."));
            c->add_memorial_log(pgettext("memorial_male", "Triggered a teleport trap."),
                                pgettext("memorial_female", "Triggered a teleport trap."));
            g->teleport();
        } else if (z != NULL) {
            if (g->u.sees(*z)) {
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
                z->die_in_explosion( nullptr );
            } else {
                int mon_hit = g->mon_at(newposx, newposy);
                if (mon_hit != -1) {
                    if (g->u.sees(*z)) {
                        add_msg(m_good, _("The %s teleports into a %s, killing them both!"),
                                z->name().c_str(), g->zombie(mon_hit).name().c_str());
                    }
                    g->zombie( mon_hit ).die_in_explosion( z );
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
            n->add_env_effect("slimed", bp_foot_l, 6, 20);
            n->add_env_effect("slimed", bp_foot_r, 6, 20);
            if (one_in(3)) {
                n->add_msg_if_player(m_bad, _("The acidic goo eats away at your feet."));
                n->deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, 5 ) );
                n->deal_damage( nullptr, bp_foot_r, damage_instance( DT_CUT, 5 ) );
            }
        } else if (z != NULL) {
            if (z->type->id == "mon_blob") {
                z->set_speed_base( z->get_speed_base() + 15 );
                z->hp = z->get_speed();
            } else {
                z->poly(GetMType("mon_blob"));
                z->set_speed_base( z->get_speed_base() - 15 );
                z->hp = z->get_speed();
            }
        }
    }
    g->m.remove_trap(x, y);
}

void trapfunc::dissector(Creature *c, int x, int y)
{
    //~ the sound of a dissector dissecting
    sounds::sound(x, y, 10, _("BRZZZAP!"));
    if (c != NULL) {
        c->add_msg_player_or_npc(m_bad, _("Electrical beams emit from the floor and slice your flesh!"),
                                 _("Electrical beams emit from the floor and slice <npcname>s flesh!"));
        c->add_memorial_log(pgettext("memorial_male", "Stepped into a dissector."),
                            pgettext("memorial_female", "Stepped into a dissector."));
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (n != NULL) {
            n->deal_damage( nullptr, bp_head, damage_instance( DT_CUT, 15 ) );
            n->deal_damage( nullptr, bp_torso, damage_instance( DT_CUT, 20 ) );
            n->deal_damage( nullptr, bp_arm_r, damage_instance( DT_CUT, 12 ) );
            n->deal_damage( nullptr, bp_arm_l, damage_instance( DT_CUT, 12 ) );
            n->deal_damage( nullptr, bp_hand_r, damage_instance( DT_CUT, 10 ) );
            n->deal_damage( nullptr, bp_hand_l, damage_instance( DT_CUT, 10 ) );
            n->deal_damage( nullptr, bp_leg_r, damage_instance( DT_CUT, 12 ) );
            n->deal_damage( nullptr, bp_leg_r, damage_instance( DT_CUT, 12 ) );
            n->deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, 10 ) );
            n->deal_damage( nullptr, bp_foot_r, damage_instance( DT_CUT, 10 ) );
        } else if (z != NULL) {
            z->apply_damage( nullptr, bp_torso, 60 );
            if( z->is_dead() ) {
                z->explode();
            }
        }
    }
}

void trapfunc::pit(Creature *c, int x, int y)
{
    // tiny animals aren't hurt by falling into pits
    if (c != NULL && c->get_size() == MS_TINY) {
        return;
    }
    if (c != NULL) {
        const float eff = pit_effectiveness(x, y);
        c->add_msg_player_or_npc(m_bad, _("You fall in a pit!"), _("<npcname> falls in a pit!"));
        c->add_memorial_log(pgettext("memorial_male", "Fell in a pit."),
                            pgettext("memorial_female", "Fell in a pit."));
        c->add_effect("in_pit", 1, num_bp, true);
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
                    n->deal_damage( nullptr, bp_leg_l, damage_instance( DT_BASH, damage ) );
                    n->deal_damage( nullptr, bp_leg_r, damage_instance( DT_BASH, damage ) );
                } else {
                    n->add_msg_if_player(_("You land nimbly."));
                }
            }
        } else if (z != NULL) {
            z->apply_damage( nullptr, bp_torso, eff * rng(10, 20));
        }
    }
}

void trapfunc::pit_spikes(Creature *c, int x, int y)
{
    // tiny animals aren't hurt by falling into spiked pits
    if (c != NULL && c->get_size() == MS_TINY) {
        return;
    }
    if (c != NULL) {
        c->add_msg_player_or_npc(m_bad, _("You fall in a spiked pit!"),
                                 _("<npcname> falls in a spiked pit!"));
        c->add_memorial_log(pgettext("memorial_male", "Fell into a spiked pit."),
                            pgettext("memorial_female", "Fell into a spiked pit."));
        c->add_effect("in_pit", 1, num_bp, true);
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
                        hit = bp_leg_l;
                        break;
                    case  2:
                        hit = bp_leg_r;
                        break;
                    case  3:
                        hit = bp_arm_l;
                        break;
                    case  4:
                        hit = bp_arm_r;
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
                n->add_msg_if_player(m_bad, _("The spikes impale your %s!"), body_part_name_accusative(hit).c_str());
                n->deal_damage( nullptr, hit, damage_instance( DT_CUT, damage ) );
              if ((n->has_trait("INFRESIST")) && (one_in(256))) {
                  n->add_effect("tetanus", 1, num_bp, true);
              }
              else if ((!n->has_trait("INFIMMUNE") || !n->has_trait("INFRESIST")) && (one_in(35))) {
                      n->add_effect("tetanus", 1, num_bp, true);
              }
            }
        } else if (z != NULL) {
            z->apply_damage( nullptr, bp_torso, rng(20, 50));
        }
    }
    if (one_in(4)) {
        if (g->u.sees(x, y)) {
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

void trapfunc::pit_glass(Creature *c, int x, int y)
{
    // tiny animals aren't hurt by falling into glass pits
    if (c != NULL && c->get_size() == MS_TINY) {
        return;
    }
    if (c != NULL) {
        c->add_msg_player_or_npc(m_bad, _("You fall in a pit filled with glass shards!"),
                                 _("<npcname> falls in pit filled with glass shards!"));
        c->add_memorial_log(pgettext("memorial_male", "Fell into a pit filled with glass shards."),
                            pgettext("memorial_female", "Fell into a pit filled with glass shards."));
        c->add_effect("in_pit", 1, num_bp, true);
        monster *z = dynamic_cast<monster *>(c);
        player *n = dynamic_cast<player *>(c);
        if (n != NULL) {
            int dodge = n->get_dodge();
            int damage = pit_effectiveness(x, y) * rng(15, 35);
            if ( (n->has_trait("WINGS_BIRD")) || ((one_in(2)) && (n->has_trait("WINGS_BUTTERFLY"))) ) {
                n->add_msg_if_player(_("You flap your wings and flutter down gracefully."));
            } else if (0 == damage || rng(5, 30) < dodge) {
                n->add_msg_if_player(_("You avoid the glass shards within."));
            } else {
                body_part hit = num_bp;
                switch (rng(1, 10)) {
                    case  1:
                        hit = bp_leg_l;
                        break;
                    case  2:
                        hit = bp_leg_r;
                        break;
                    case  3:
                        hit = bp_arm_l;
                        break;
                    case  4:
                        hit = bp_arm_r;
                        break;
                    case  5:
                        hit = bp_foot_l;
                        break;
                    case  6:
                        hit = bp_foot_r;
                        break;
                    case  7:
                    case  8:
                    case  9:
                    case 10:
                        hit = bp_torso;
                        break;
                }
                n->add_msg_if_player(m_bad, _("The glass shards slash your %s!"), body_part_name_accusative(hit).c_str());
                n->deal_damage( nullptr, hit, damage_instance( DT_CUT, damage ) );
              if ((n->has_trait("INFRESIST")) && (one_in(256))) {
                  n->add_effect("tetanus", 1, num_bp, true);
              }
              else if ((!n->has_trait("INFIMMUNE") || !n->has_trait("INFRESIST")) && (one_in(35))) {
                      n->add_effect("tetanus", 1, num_bp, true);
              }
            }
        } else if (z != NULL) {
            z->apply_damage( nullptr, bp_torso, rng(20, 50));
        }
    }
    if (one_in(5)) {
        if (g->u.sees(x, y)) {
            add_msg(_("The shards shatter!"));
        }
        g->m.ter_set(x, y, t_pit);
        g->m.add_trap(x, y, tr_pit);
        for (int i = 0; i < 20; i++) { // 20 shards in a pit.
            if (one_in(3)) {
                g->m.spawn_item(x, y, "glass_shard");
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
            n->deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, 20 ) );
            n->deal_damage( nullptr, bp_foot_r, damage_instance( DT_CUT, 20 ) );
            n->deal_damage( nullptr, bp_leg_l, damage_instance( DT_CUT, 20 ) );
            n->deal_damage( nullptr, bp_leg_r, damage_instance( DT_CUT, 20 ) );
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
            z->apply_damage( nullptr, bp_torso, dam );
        }
    }
}

// STUB
void trapfunc::portal(Creature * /*c*/, int /*x*/, int /*y*/)
{
    // TODO: make this do something?
}

void trapfunc::sinkhole(Creature *c, int /*x*/, int /*y*/) {
    if (c != &g->u) {
        //TODO: make something exciting happen here
        return;
    }
    g->u.add_msg_if_player(m_bad, _("You step into a sinkhole, and start to sink down!"));
    g->u.add_memorial_log(pgettext("memorial_male", "Stepped into a sinkhole."),
                        pgettext("memorial_female", "Stepped into a sinkhole."));
    if (g->u.has_amount("grapnel", 1) &&
        query_yn(_("You step into a sinkhole! Throw your grappling hook out to try to catch something?"))) {
        int throwroll = rng(g->u.skillLevel("throw"),
                            g->u.skillLevel("throw") + g->u.str_cur + g->u.dex_cur);
        if (throwroll >= 6) {
            add_msg(m_good, _("The grappling hook catches something!"));
            if (rng(g->u.skillLevel("unarmed"),
                    g->u.skillLevel("unarmed") + g->u.str_cur) > 4) {
                // Determine safe places for the character to get pulled to
                std::vector<point> safe;
                for (int i = g->u.posx() - 1; i <= g->u.posx() + 1; i++) {
                    for (int j = g->u.posy() - 1; j <= g->u.posy() + 1; j++) {
                        if (g->m.move_cost(i, j) > 0 && g->m.tr_at(i, j) != tr_pit) {
                            safe.push_back(point(i, j));
                        }
                    }
                }
                if (safe.empty()) {
                    add_msg(m_bad, _("There's nowhere to pull yourself to, and you sink!"));
                    g->u.use_amount("grapnel", 1);
                    g->m.spawn_item(g->u.posx() + rng(-1, 1), g->u.posy() + rng(-1, 1), "grapnel");
                    g->m.ter_set(g->u.posx(), g->u.posy(), t_hole);
                    g->vertical_move(-1, true);
                } else {
                    add_msg(m_good, _("You pull yourself to safety!  The sinkhole collapses."));
                    int index = rng(0, safe.size() - 1);
                    g->m.ter_set(g->u.posx(), g->u.posy(), t_hole);
                    g->u.setx( safe[index].x );
                    g->u.sety( safe[index].y );
                    g->update_map(&g->u);
                }
            } else {
                add_msg(m_bad, _("You're not strong enough to pull yourself out..."));
                g->u.moves -= 100;
                g->u.use_amount("grapnel", 1);
                g->m.spawn_item(g->u.posx() + rng(-1, 1), g->u.posy() + rng(-1, 1), "grapnel");
                g->m.ter_set(g->u.posx(), g->u.posy(), t_hole);
                g->vertical_move(-1, true);
            }
        } else {
            add_msg(m_bad, _("Your throw misses completely, and you sink!"));
            if (one_in((g->u.str_cur + g->u.dex_cur) / 3)) {
                g->u.use_amount("grapnel", 1);
                g->m.spawn_item(g->u.posx() + rng(-1, 1), g->u.posy() + rng(-1, 1), "grapnel");
            }
            g->m.ter_set(g->u.posx(), g->u.posy(), t_hole);
            g->vertical_move(-1, true);
        }
    } else if(g->u.has_amount("bullwhip", 1) &&
              query_yn(_("You step into a sinkhole! Throw your whip out to try and snag something?"))) {
            int whiproll = rng(g->u.skillLevel("melee"),
                               g->u.skillLevel("melee") + g->u.dex_cur + g->u.str_cur);

            if(whiproll < 8) {
                add_msg(m_bad, _("Your whip flails uselessly through the air!"));
                g->m.ter_set(g->u.posx(), g->u.posy(), t_hole);
                g->vertical_move(-1, true);
            } else {
                add_msg(m_good, _("Your whip wraps around something!"));
                if(g->u.str_cur < 5) {
                    add_msg(m_bad, _("But you're too weak to pull yourself out..."));
                    g->u.moves -= 100;
                    g->u.use_amount("bullwhip", 1);
                    g->m.spawn_item(g->u.posx() + rng(-1, 1), g->u.posy() + rng(-1, 1), "bullwhip");
                    g->m.ter_set(g->u.posx(), g->u.posy(), t_hole);
                    g->vertical_move(-1, true);
                } else {
                    // Determine safe places for the character to get pulled to
                    std::vector<point> safe;
                    for (int i = g->u.posx() - 1; i <= g->u.posx() + 1; i++) {
                        for (int j = g->u.posy() - 1; j <= g->u.posy() + 1; j++) {
                            if (g->m.move_cost(i, j) > 0 && g->m.tr_at(i, j) != tr_pit) {
                                safe.push_back(point(i, j));
                            }
                        }
                    }
                    if (safe.empty()) {
                        add_msg(m_bad, _("There's nowhere to pull yourself to, and you sink!"));
                        g->u.use_amount("bullwhip", 1);
                        g->m.spawn_item(g->u.posx() + rng(-1, 1), g->u.posy() + rng(-1, 1), "bullwhip");
                        g->m.ter_set(g->u.posx(), g->u.posy(), t_hole);
                        g->vertical_move(-1, true);
                    } else {
                        add_msg(m_good, _("You pull yourself to safety!  The sinkhole collapses."));
                        int index = rng(0, safe.size() - 1);
                        g->m.ter_set(g->u.posx(), g->u.posy(), t_hole);
                        g->u.setx( safe[index].x );
                        g->u.sety( safe[index].y );
                        g->update_map(&(g->u));
                    }
                }
            }
    } else if (g->u.has_amount("rope_30", 1) &&
               query_yn(_("You step into a sinkhole! Throw your rope out to try to catch something?"))) {
        int throwroll = rng(g->u.skillLevel("throw"),
                            g->u.skillLevel("throw") + g->u.str_cur + g->u.dex_cur);
        if (throwroll >= 12) {
            add_msg(m_good, _("The rope catches something!"));
            if (rng(g->u.skillLevel("unarmed"),
                    g->u.skillLevel("unarmed") + g->u.str_cur) > 6) {
                // Determine safe places for the character to get pulled to
                std::vector<point> safe;
                for (int i = g->u.posx() - 1; i <= g->u.posx() + 1; i++) {
                    for (int j = g->u.posy() - 1; j <= g->u.posy() + 1; j++) {
                        if (g->m.move_cost(i, j) > 0 && g->m.tr_at(i, j) != tr_pit) {
                            safe.push_back(point(i, j));
                        }
                    }
                }
                if (safe.empty()) {
                    add_msg(m_bad, _("There's nowhere to pull yourself to, and you sink!"));
                    g->u.use_amount("rope_30", 1);
                    g->m.spawn_item(g->u.posx() + rng(-1, 1), g->u.posy() + rng(-1, 1), "rope_30");
                    g->m.ter_set(g->u.posx(), g->u.posy(), t_hole);
                    g->vertical_move(-1, true);
                } else {
                    add_msg(m_good, _("You pull yourself to safety!  The sinkhole collapses."));
                    int index = rng(0, safe.size() - 1);
                    g->m.ter_set(g->u.posx(), g->u.posy(), t_hole);
                    g->u.setx( safe[index].x );
                    g->u.sety( safe[index].y );
                    g->update_map(&(g->u));
                }
            } else {
                add_msg(m_bad, _("You're not strong enough to pull yourself out..."));
                g->u.moves -= 100;
                g->u.use_amount("rope_30", 1);
                g->m.spawn_item(g->u.posx() + rng(-1, 1), g->u.posy() + rng(-1, 1), "rope_30");
                g->m.ter_set(g->u.posx(), g->u.posy(), t_hole);
                g->vertical_move(-1, true);
            }
        } else {
            add_msg(m_bad, _("Your throw misses completely, and you sink!"));
            if (one_in((g->u.str_cur + g->u.dex_cur) / 3)) {
                g->u.use_amount("rope_30", 1);
                g->m.spawn_item(g->u.posx() + rng(-1, 1), g->u.posy() + rng(-1, 1), "rope_30");
            }
            g->m.ter_set(g->u.posx(), g->u.posy(), t_hole);
            g->vertical_move(-1, true);
        }
    } else {
        add_msg(m_bad, _("You sink into the sinkhole!"));
        g->m.ter_set(g->u.posx(), g->u.posy(), t_hole);
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
        const ter_id type = g->m.ter(x, y);
        for (int i = 0; i < SEEX * MAPSIZE; i++) {
            for (int j = 0; j < SEEY * MAPSIZE; j++) {
                if( type == t_floor_red ) {
                        if (g->m.ter(i, j) == t_rock_green) {
                            g->m.ter_set(i, j, t_floor_green);
                        } else if (g->m.ter(i, j) == t_floor_green) {
                            g->m.ter_set(i, j, t_rock_green);
                        }
                } else if( type == t_floor_green ) {
                        if (g->m.ter(i, j) == t_rock_blue) {
                            g->m.ter_set(i, j, t_floor_blue);
                        } else if (g->m.ter(i, j) == t_floor_blue) {
                            g->m.ter_set(i, j, t_rock_blue);
                        }
                } else if( type == t_floor_blue ) {
                        if (g->m.ter(i, j) == t_rock_red) {
                            g->m.ter_set(i, j, t_floor_red);
                        } else if (g->m.ter(i, j) == t_floor_red) {
                            g->m.ter_set(i, j, t_rock_red);
                        }
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
            z->apply_damage( nullptr, bp_torso, rng( 5, 10 ) );
            z->set_speed_base( z->get_speed_base() * 0.9 );
        }
    }
}

void trapfunc::hum(Creature * /*c*/, int const x, int const y)
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
    sounds::sound(x, y, volume, sfx);
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
            monx = rng(g->u.posx() - 5, g->u.posx() + 5);
            mony = (one_in(2) ? g->u.posy() - 5 : g->u.posy() + 5);
        } else {
            monx = (one_in(2) ? g->u.posx() - 5 : g->u.posx() + 5);
            mony = rng(g->u.posy() - 5, g->u.posy() + 5);
        }
    } while (tries < 5 && !g->is_empty(monx, mony) &&
             !g->m.sees(monx, mony, g->u.posx(), g->u.posy(), 10, junk));

    if (tries < 5) {
        add_msg(m_warning, _("A shadow forms nearby."));
        spawned.reset_special_rng(0);
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
            z->apply_damage( nullptr, bp_torso, 1 );
        }
    }
}

void trapfunc::snake(Creature *c, int x, int y)
{
    //~ the sound a snake makes
    sounds::sound(x, y, 10, _("ssssssss"));
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
                monx = rng(g->u.posx() - 5, g->u.posx() + 5);
                mony = (one_in(2) ? g->u.posy() - 5 : g->u.posy() + 5);
            } else {
                monx = (one_in(2) ? g->u.posx() - 5 : g->u.posx() + 5);
                mony = rng(g->u.posy() - 5, g->u.posy() + 5);
            }
        } while (tries < 5 && !g->is_empty(monx, mony) &&
                 !g->m.sees(monx, mony, g->u.posx(), g->u.posy(), 10, junk));

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
    if("pit_glass" == function_name) {
        return &trapfunc::pit_glass;
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
