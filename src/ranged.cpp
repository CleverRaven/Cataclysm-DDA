#include <vector>
#include <string>
#include "game.h"
#include "keypress.h"
#include "output.h"
#include "line.h"
#include "skill.h"
#include "rng.h"
#include "item.h"
#include "options.h"
#include "action.h"
#include "input.h"

int time_to_fire(player &p, it_gun* firing);
int recoil_add(player &p);
void make_gun_sound_effect(player &p, bool burst, item* weapon);
double calculate_missed_by(player &p, int trange, item* weapon);
void shoot_player(player &p, player *h, int &dam, double goodhit);

void splatter(std::vector<point> trajectory, int dam, Creature *target = NULL);

double Creature::projectile_attack(const projectile &proj, int targetx, int targety,
        double shot_dispersion) {
    return projectile_attack(proj, xpos(), ypos(), targetx, targety, shot_dispersion);
}

double Creature::projectile_attack(const projectile &proj, int sourcex, int sourcey,
                                   int targetx, int targety, double shot_dispersion) {
    int range = rl_dist(sourcex,sourcey,targetx,targety);
    // .013 * trange is a computationally cheap version of finding the tangent.
    // (note that .00325 * 4 = .013; .00325 is used because deviation is a number
    //  of quarter-degrees)
    // It's also generous; missed_by will be rather short.
    double missed_by = shot_dispersion * .00325 * range;
    // TODO: move to-hit roll back in here

    if (missed_by >= 1.) {
        // We missed D:
        // Shoot a random nearby space?
        targetx += rng(0 - int(sqrt(double(missed_by))), int(sqrt(double(missed_by))));
        targety += rng(0 - int(sqrt(double(missed_by))), int(sqrt(double(missed_by))));
    }

    std::vector<point> trajectory;
    int tart = 0;
    if (g->m.sees(sourcex, sourcey, targetx, targety, -1, tart)) {
        trajectory = line_to(sourcex,sourcey,targetx,targety,tart);
    } else {
        trajectory = line_to(sourcex,sourcey,targetx,targety,0);
    }

    // Set up a timespec for use in the nanosleep function below
    timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = BULLET_SPEED;

    int dam = proj.impact.total_damage() + proj.payload.total_damage();
    it_ammo *curammo = proj.ammo;

    // Trace the trajectory, doing damage in order
    int tx = trajectory[0].x;
    int ty = trajectory[0].y;
    int px = trajectory[0].x;
    int py = trajectory[0].y;

    // if this is a vehicle mounted turret, which vehicle is it mounted on?
    const vehicle* in_veh = is_fake() ? g->m.veh_at(xpos(), ypos()) : NULL;

    for (int i = 0; i < trajectory.size() && (dam > 0 || (proj.proj_effects.count("FLAME"))); i++) {
        px = tx;
        py = ty;
        (void) px;
        (void) py;
        tx = trajectory[i].x;
        ty = trajectory[i].y;
        // Drawing the bullet uses player u, and not player p, because it's drawn
        // relative to YOUR position, which may not be the gunman's position.
        g->draw_bullet(g->u, tx, ty, i, trajectory, proj.proj_effects.count("FLAME")? '#':'*', ts);

        /* TODO: add running out of momentum back in
        if (dam <= 0 && !(proj.proj_effects.count("FLAME"))) { // Ran out of momentum.
            break;
        }
        */

        Creature *critter = g->critter_at(tx, ty);
        monster *mon = dynamic_cast<monster*>(critter);
        // ignore non-point-blank digging targets (since they are underground)
        if (mon != NULL && mon->digging() &&
                            rl_dist(xpos(), ypos(), tx, ty) > 1) {
            critter = mon = NULL;
        }
        // If we shot us a monster...
        // TODO: add size effects to accuracy
        // If there's a monster in the path of our bullet, and either our aim was true,
        //  OR it's not the monster we were aiming at and we were lucky enough to hit it
        double cur_missed_by;
        if (i < trajectory.size() - 1) { // Unintentional hit
            cur_missed_by = std::max(rng_float(0,1.5)+(1-missed_by),0.2);
        } else {
            cur_missed_by = missed_by;
        }
        if (critter != NULL && cur_missed_by <= 1.0) {
            dealt_damage_instance dealt_dam;
            critter->deal_projectile_attack(this, missed_by, proj, dealt_dam);
            std::vector<point> blood_traj = trajectory;
            blood_traj.insert(blood_traj.begin(), point(xpos(), ypos()));
            splatter( blood_traj, dam, critter );
            dam = 0;
        } else if(in_veh != NULL && g->m.veh_at(tx, ty) == in_veh) {
            // Don't do anything, especially don't call map::shoot as this would damage the vehicle
        } else {
            g->m.shoot(tx, ty, dam, i == trajectory.size() - 1, proj.proj_effects);
        }
    } // Done with the trajectory!

    if (g->m.move_cost(tx, ty) == 0) {
        tx = px;
        ty = py;
    }
    // we can only drop something if curammo exists
    if (curammo != NULL && proj.drops &&
            !(proj.proj_effects.count("IGNITE")) &&
            !(proj.proj_effects.count("EXPLOSIVE")) &&
            ((curammo->m1 == "wood" && !one_in(5)) ||
            (curammo->m1 != "wood" && !one_in(15))  )) {
        item ammotmp = item(curammo, 0);
        ammotmp.charges = 1;
        g->m.add_item_or_charges(tx, ty, ammotmp);
    }

    ammo_effects(tx, ty, proj.proj_effects);

    if (proj.proj_effects.count("BOUNCE")) {
        for (unsigned long int i = 0; i < g->num_zombies(); i++) {
            monster &z = g->zombie(i);
            // search for monsters in radius 4 around impact site
            if (rl_dist(z.posx(), z.posy(), tx, ty) <= 4) {
                // don't hit targets that have already been hit
                if (!z.has_effect("bounced") && !z.dead) {
                    g->add_msg(_("The attack bounced to %s!"), z.name().c_str());
                    projectile_attack(proj, tx, ty, z.posx(), z.posy(), shot_dispersion);
                    break;
                }
            }
        }
    }

    return missed_by;
}

void player::fire_gun(int tarx, int tary, bool burst) {
    item ammotmp;
    item* gunmod = weapon.active_gunmod();
    it_ammo *curammo = NULL;
    item *used_weapon = NULL;

    if (weapon.has_flag("CHARGE")) { // It's a charger gun, so make up a type
        // Charges maxes out at 8.
        int charges = weapon.num_charges();
        it_ammo *tmpammo = dynamic_cast<it_ammo*>(itypes["charge_shot"]);

        tmpammo->damage = charges * charges;
        tmpammo->pierce = (charges >= 4 ? (charges - 3) * 2.5 : 0);
        if (charges <= 4)
            tmpammo->dispersion = 14 - charges * 2;
        else // 5, 12, 21, 32
            tmpammo->dispersion = charges * (charges - 4);
        tmpammo->recoil = tmpammo->dispersion * .8;
        tmpammo->ammo_effects.clear(); // Reset effects.
        if (charges == 8) { tmpammo->ammo_effects.insert("EXPLOSIVE_BIG"); }
        else if (charges >= 6) { tmpammo->ammo_effects.insert("EXPLOSIVE"); }

        if (charges >= 5){ tmpammo->ammo_effects.insert("FLAME"); }
        else if (charges >= 4) { tmpammo->ammo_effects.insert("INCENDIARY"); }

        if (gunmod != NULL) { // TODO: range calculation in case of active gunmod.
            used_weapon = gunmod;
        } else {
            used_weapon = &weapon;
        }

        curammo = tmpammo;
        used_weapon->curammo = tmpammo;
    } else if (gunmod != NULL) {
        used_weapon = gunmod;
        curammo = used_weapon->curammo;
    } else {// Just a normal gun. If we're here, we know curammo is valid.
        curammo = weapon.curammo;
        used_weapon = &weapon;
    }

    ammotmp = item(curammo, 0);
    ammotmp.charges = 1;

    if (!used_weapon->is_gun() && !used_weapon->is_gunmod()) {
        debugmsg("%s tried to fire a non-gun (%s).", name.c_str(),
                                                    used_weapon->tname().c_str());
        return;
    }

    projectile proj; // damage will be set later
    proj.aoe_size = 0;
    proj.ammo = curammo;
    proj.speed = 1000;

    std::set<std::string> *curammo_effects = &curammo->ammo_effects;
    if(gunmod == NULL){
        std::set<std::string> *gun_effects = &dynamic_cast<it_gun*>(used_weapon->type)->ammo_effects;
        proj.proj_effects.insert(gun_effects->begin(),gun_effects->end());
    }
    proj.proj_effects.insert(curammo_effects->begin(),curammo_effects->end());

    proj.wide = (curammo->phase == LIQUID ||
            proj.proj_effects.count("SHOT") || proj.proj_effects.count("BOUNCE"));
    proj.drops = (curammo->type == "bolt" || curammo->type == "arrow");

    //int x = xpos(), y = ypos();
    // Have to use the gun, gunmods don't have a type
    it_gun* firing = dynamic_cast<it_gun*>(weapon.type);
    if (has_trait("TRIGGERHAPPY") && one_in(30))
        burst = true;
    if (burst && used_weapon->burst_size() < 2)
        burst = false; // Can't burst fire a semi-auto

    // Use different amounts of time depending on the type of gun and our skill
    moves -= time_to_fire(*this, firing);

    // Decide how many shots to fire
    int num_shots = 1;
    if (burst)
        num_shots = used_weapon->burst_size();
    if (num_shots > used_weapon->num_charges() && !used_weapon->has_flag("CHARGE") && !used_weapon->has_flag("NO_AMMO"))
        num_shots = used_weapon->num_charges();

    if (num_shots == 0)
        debugmsg("game::fire() - num_shots = 0!");

    int ups_drain = 0;
    int adv_ups_drain = 0;
    if (weapon.has_flag("USE_UPS")) {
        ups_drain = 5;
        adv_ups_drain = 3;
    } else if (weapon.has_flag("USE_UPS_20")) {
        ups_drain = 20;
        adv_ups_drain = 12;
    } else if (weapon.has_flag("USE_UPS_40")) {
        ups_drain = 40;
        adv_ups_drain = 24;
    }

    // cap our maximum burst size by the amount of UPS power left
    if (ups_drain > 0 || adv_ups_drain > 0)
    while (!(has_charges("UPS_off", ups_drain*num_shots) ||
                has_charges("UPS_on", ups_drain*num_shots) ||
                has_charges("adv_UPS_off", adv_ups_drain*num_shots) ||
                has_charges("adv_UPS_on", adv_ups_drain*num_shots))) {
        num_shots--;
    }

    const bool debug_retarget = false;  // this will inevitably be needed
    //const bool wildly_spraying = false; // stub for now. later, rng based on stress/skill/etc at the start,
    int weaponrange = weapon.range(); // this is expensive, let's cache. todo: figure out if we need weapon.range(&p);

    for (int curshot = 0; curshot < num_shots; curshot++) {
        // Burst-fire weapons allow us to pick a new target after killing the first
        int zid = g->mon_at(tarx, tary);
        if ( curshot > 0 && (zid == -1 || g->zombie(zid).hp <= 0) ) {
            std::vector<point> new_targets;
            new_targets.clear();

            if ( debug_retarget == true ) {
                mvprintz(curshot,5,c_red,"[%d] %s: retarget: mon_at(%d,%d)",curshot,name.c_str(),tarx,tary);
                if(zid == -1) {
                    printz(c_red, " = -1");
                } else {
                    printz(c_red, ".hp=%d", g->zombie(zid).hp);
                }
            }

            for (unsigned long int i = 0; i < g->num_zombies(); i++) {
                monster &z = g->zombie(i);
                int dummy;
                // search for monsters in radius
                if (rl_dist(z.posx(), z.posy(), tarx, tary) <= std::min(2 + skillLevel("gun"), weaponrange) &&
                        rl_dist(xpos(),ypos(),z.xpos(),z.ypos()) <= weaponrange &&
                        sees(&z, dummy) ) {
                    if (!z.is_dead_state())
                        new_targets.push_back(point(z.xpos(), z.ypos())); // oh you're not dead and I don't like you. Hello!
                }
            }

            if ( new_targets.empty() == false ) {    /* new victim! or last victim moved */
                int target_picked = rng(0, new_targets.size() - 1); /* 1 victim list unless wildly spraying */
                tarx = new_targets[target_picked].x;
                tary = new_targets[target_picked].y;
                zid = g->mon_at(tarx, tary);

                /* debug */ if (debug_retarget) printz(c_ltgreen, " NEW:(%d:%d,%d) %d,%d (%s)[%d] hp: %d",
                    target_picked, new_targets[target_picked].x, new_targets[target_picked].y,
                    tarx, tary, g->zombie(zid).name().c_str(), zid, g->zombie(zid).hp);

            } else if (
                (
                    !has_trait("TRIGGERHAPPY") ||   /* double ta TRIPLE TAP! wait, no... */
                    one_in(3)                          /* on second though...everyone double-taps at times. */
                ) && (
                    skillLevel("gun") >= 7 ||        /* unless trained */
                    one_in(7 - skillLevel("gun"))    /* ...sometimes */
                ) ) {
                return;                               // No targets, so return
            } else if (debug_retarget) {
                printz(c_red, " new targets.empty()!");
            }
        } else if (debug_retarget) {
            const int zid = g->mon_at(tarx, tary);
            mvprintz(curshot,5,c_red,"[%d] %s: target == mon_at(%d,%d)[%d] %s hp %d",curshot, name.c_str(), tarx ,tary,
            zid,
            g->zombie(zid).name().c_str(),
            g->zombie(zid).hp);
        }

        // Drop a shell casing if appropriate.
        itype_id casing_type = curammo->casing;
        if (casing_type != "NULL" && !casing_type.empty()) {
            item casing;
            casing.make(itypes[casing_type]);
            // Casing needs a charges of 1 to stack properly with other casings.
            casing.charges = 1;
            if( used_weapon->has_gunmod("brass_catcher") != -1 ) {
                i_add( casing );
            } else {
                int x = 0;
                int y = 0;
                int count = 0;
                do {
                    x = xpos() - 1 + rng(0, 2);
                    y = ypos() - 1 + rng(0, 2);
                    count++;
                    // Try not to drop the casing on a wall if at all possible.
                } while( g->m.move_cost( x, y ) == 0 && count < 10 );
                g->m.add_item_or_charges(x, y, casing);
            }
        }

        // Use up a round (or 100)
        if (used_weapon->has_flag("FIRE_100")) {
            used_weapon->charges -= 100;
        } else if (used_weapon->has_flag("FIRE_50")) {
            used_weapon->charges -= 50;
        } else if (used_weapon->has_flag("CHARGE")) {
            used_weapon->active = false;
            used_weapon->charges = 0;
        } else if (used_weapon->has_flag("BIO_WEAPON")) {
            //The weapon used is a bio weapon.
            //It should consume a charge to let the game (specific: bionics.cpp:player::activate_bionic)
            //know the weapon has been fired.
            //It should ignore the NO_AMMO tag for charges, and still use one.
            //the charges are virtual anyway.
            used_weapon->charges--;
        } else if (!used_weapon->has_flag("NO_AMMO")) {
            used_weapon->charges--;
        }

        // Drain UPS power
        if (has_charges("adv_UPS_off", adv_ups_drain)) {
            use_charges("adv_UPS_off", adv_ups_drain);
        } else if (has_charges("adv_UPS_on", adv_ups_drain)) {
            use_charges("adv_UPS_on", adv_ups_drain);
        } else if (has_charges("UPS_off", ups_drain)) {
            use_charges("UPS_off", ups_drain);
        } else if (has_charges("UPS_on", ups_drain)) {
            use_charges("UPS_on", ups_drain);
        }
// here we check if we're underwater and whether we should misfire as a result this causes no damage to the firearm,
// note that some guns are waterproof and so are immune to this effect, note also that WATERPROOF_GUN status does not
// mean the gun will actually be accurate underwater
        if (firing->skill_used != Skill::skill("archery") &&
            firing->skill_used != Skill::skill("throw")) {
            if (is_underwater() && !weapon.has_flag("WATERPROOF_GUN") && one_in(firing->durability)) {
                g->add_msg_player_or_npc(this, _("Your %s misfires with a wet click!"),
                                         _("<npcname>'s %s misfires with a wet click!"),
                              weapon.name.c_str());
                return;
// here we check for a chance for the weapon to suffer a mechanical malfunction note that some weapons never
// jam up 'NEVER_JAMS' and thus are immune to this effect as current guns have a durability between 5 and 9
// this results in a chance of mechanical failure between 1/64 and 1/1024 on any given shot the malfunction may cause
// damage, but never enough to push the weapon beyond 'shattered'
            } else if ((one_in(2 << firing->durability))&& !weapon.has_flag("NEVER_JAMS")) {
                g->add_msg_player_or_npc(this, _("Your %s malfunctions!"),
                                         _("<npcname>'s %s malfunctions!"),
                              weapon.name.c_str());
                   if ((weapon.damage < 4) && one_in(8 * firing->durability)){  
                   weapon.damage++;
                g->add_msg_player_or_npc(this, _("Your %s is damaged by the mechanical malfunction!"),
                                         _("<npcname>'s %s is damaged by the mechanical malfunction!"),
                              weapon.name.c_str());
                   }
                return;
// here we check for a chance for the weapon to suffer a misfire due to using OEM bullets note that these misfires 
// cause no damage to the weapon and some types of ammunition are immune to this effect via the NEVER_MISFIRES effect
            } else if (!curammo_effects->count("NEVER_MISFIRES") && one_in(1728)) {
                g->add_msg_player_or_npc(this, _("Your %s misfires with a dry click!"),
                                         _("<npcname>'s %s misfires with a dry click!"),
                              weapon.name.c_str());
                return;
// here we check for a chance for the weapon to suffer a misfire due to using player-made 'RECYCLED' bullets,
// note that not all forms of player-made ammunition have this effect the misfire may cause damage, but never
// enough to push the weapon beyond 'shattered'
            } else if (curammo_effects->count("RECYCLED") && one_in(512)) {
                g->add_msg_player_or_npc(this, _("Your %s misfires with a muffled click!"),
                                         _("<npcname>'s %s misfires with a muffled click!"),
                              weapon.name.c_str());
                   if ((weapon.damage < 4) && one_in(2 * firing->durability)){
                   weapon.damage++;
                g->add_msg_player_or_npc(this, _("Your %s is damaged by the misfired round!"),
                                         _("<npcname>'s %s is damaged by the misfired round!"),
                              weapon.name.c_str());
                   }
                return;
            }
        }

        make_gun_sound_effect(*this, burst, used_weapon);

        double total_dispersion = get_weapon_dispersion(used_weapon);
        //debugmsg("%f",total_dispersion);
        int range = rl_dist(xpos(), ypos(), tarx, tary);
        // penalties for point-blank
        if (range < (firing->volume/3) && firing->ammo != "shot")
            total_dispersion *= double(firing->volume/3) / double(range);

        // rifle has less range penalty past LONG_RANGE
        if (firing->skill_used == Skill::skill("rifle") && range > LONG_RANGE)
            total_dispersion *= 1 - 0.4*double(range - LONG_RANGE) / double(range);

        if (curshot > 0) {
            if (recoil_add(*this) % 2 == 1) {
                recoil++;
            }
            recoil += recoil_add(*this) / 2;
        } else {
            recoil += recoil_add(*this);
        }

        int mtarx = tarx;
        int mtary = tary;

        int adjusted_damage = used_weapon->gun_damage();

        proj.impact = damage_instance::physical(0,adjusted_damage,0);

        double missed_by = projectile_attack(proj, mtarx, mtary, total_dispersion);
        if (missed_by <= .1) { // TODO: check head existence for headshot
            practice(g->turn, firing->skill_used, 5);
            lifetime_stats()->headshots++;
        } else if (missed_by <= .2) {
            practice(g->turn, firing->skill_used, 3);
        } else if (missed_by <= .4) {
            practice(g->turn, firing->skill_used, 2);
        } else if (missed_by <= .6) {
            practice(g->turn, firing->skill_used, 1);
        }

    }

    if (used_weapon->num_charges() == 0) {
        used_weapon->curammo = NULL;
    }

}

void game::fire(player &p, int tarx, int tary, std::vector<point> &, bool burst) {
    p.fire_gun(tarx, tary, burst);
}

void game::throw_item(player &p, int tarx, int tary, item &thrown,
                      std::vector<point> &trajectory)
{
    int deviation = 0;
    int trange = 1.5 * rl_dist(p.posx, p.posy, tarx, tary);
    std::set<std::string> no_effects;

    // Throwing attempts below "Basic Competency" level are extra-bad
    int skillLevel = p.skillLevel("throw");

    if (skillLevel < 3) {
        deviation += rng(0, 8 - skillLevel);
    }

    if (skillLevel < 8) {
        deviation += rng(0, 8 - skillLevel);
    } else {
        deviation -= skillLevel - 6;
    }

    deviation += p.throw_dex_mod();

    if (p.per_cur < 6) {
        deviation += rng(0, 8 - p.per_cur);
    } else if (p.per_cur > 8) {
        deviation -= p.per_cur - 8;
    }

    deviation += rng(0, p.encumb(bp_hands) * 2 + p.encumb(bp_eyes) + 1);
    if (thrown.volume() > 5) {
        deviation += rng(0, 1 + (thrown.volume() - 5) / 4);
    }
    if (thrown.volume() == 0) {
        deviation += rng(0, 3);
    }

    deviation += rng(0, std::max( 0, p.str_cur - thrown.weight() / 113 ) );

    double missed_by = .01 * deviation * trange;
    bool missed = false;
    int tart;

    if (missed_by >= 1) {
        // We missed D:
        // Shoot a random nearby space?
        if (missed_by > 9) {
            missed_by = 9;
        }

        tarx += rng(0 - int(sqrt(double(missed_by))), int(sqrt(double(missed_by))));
        tary += rng(0 - int(sqrt(double(missed_by))), int(sqrt(double(missed_by))));
        if (m.sees(p.posx, p.posy, tarx, tary, -1, tart)) {
            trajectory = line_to(p.posx, p.posy, tarx, tary, tart);
        } else {
            trajectory = line_to(p.posx, p.posy, tarx, tary, 0);
        }
        missed = true;
        add_msg_if_player(&p,_("You miss!"));
    } else if (missed_by >= .6) {
        // Hit the space, but not necessarily the monster there
        missed = true;
        add_msg_if_player(&p,_("You barely miss!"));
    }

    std::string message;
    int real_dam = (thrown.weight() / 452 + thrown.type->melee_dam / 2 + p.str_cur / 2) /
               double(2 + double(thrown.volume() / 4));
    if (real_dam > thrown.weight() / 40) {
        real_dam = thrown.weight() / 40;
    }
    if (p.has_active_bionic("bio_railgun") && (thrown.made_of("iron") || thrown.made_of("steel"))) {
        real_dam *= 2;
    }
    int dam = real_dam;

    int i = 0, tx = 0, ty = 0;
    for (i = 0; i < trajectory.size() && dam >= 0; i++) {
        message = "";
        double goodhit = missed_by;
        tx = trajectory[i].x;
        ty = trajectory[i].y;

        const int zid = mon_at(tx, ty);

        // If there's a monster in the path of our item, and either our aim was true,
        //  OR it's not the monster we were aiming at and we were lucky enough to hit it
        if (zid != -1 && (!missed || one_in(7 - int(zombie(zid).type->size)))) {
            monster &z = zombie(zid);
            if (rng(0, 100) < 20 + skillLevel * 12 && thrown.type->melee_cut > 0) {
                if (!p.is_npc()) {
                    message += string_format(_(" You cut the %s!"), z.name().c_str());
                }
                if (thrown.type->melee_cut > z.get_armor_cut(bp_torso)) {
                    dam += (thrown.type->melee_cut - z.get_armor_cut(bp_torso));
                }
            }
            if (thrown.made_of("glass") && !thrown.active && // active = molotov, etc.
                rng(0, thrown.volume() + 8) - rng(0, p.str_cur) < thrown.volume()) {
                if (u_see(tx, ty)) {
                    add_msg(_("The %s shatters!"), thrown.tname().c_str());
                }

                for (int i = 0; i < thrown.contents.size(); i++) {
                    m.add_item_or_charges(tx, ty, thrown.contents[i]);
                }

                sound(tx, ty, 16, _("glass breaking!"));
                int glassdam = rng(0, thrown.volume() * 2);
                if (glassdam > z.get_armor_cut(bp_torso)) {
                    dam += (glassdam - z.get_armor_cut(bp_torso));
                }
            } else {
                m.add_item_or_charges(tx, ty, thrown);
            }

            if (i < trajectory.size() - 1) {
                goodhit = double(double(rand() / RAND_MAX) / 2);
            }

            if (goodhit < .1 && !z.has_flag(MF_NOHEAD)) {
                message = _("Headshot!");
                dam = rng(dam, dam * 3);
                p.practice(turn, "throw", 5);
                p.lifetime_stats()->headshots++;
            } else if (goodhit < .2) {
                message = _("Critical!");
                dam = rng(dam, dam * 2);
                p.practice(turn, "throw", 2);
            } else if (goodhit < .4) {
                dam = rng(int(dam / 2), int(dam * 1.5));
            } else if (goodhit < .5) {
                message = _("Grazing hit.");
                dam = rng(0, dam);
            }
            if (u_see(tx, ty)) {
                g->add_msg_player_or_npc(&p,
                    _("%s You hit the %s for %d damage."),
                    _("%s <npcname> hits the %s for %d damage."),
                    message.c_str(), z.name().c_str(), dam);
            }
            if (z.hurt(dam, real_dam)) {
                z.die(&p);
            }
            return;
        } else { // No monster hit, but the terrain might be.
            m.shoot(tx, ty, dam, false, no_effects);
        }
        // Collide with impassable terrain
        if (m.move_cost(tx, ty) == 0) {
            if (i > 0) {
                tx = trajectory[i - 1].x;
                ty = trajectory[i - 1].y;
            } else {
                tx = u.posx;
                ty = u.posy;
            }
            i = trajectory.size();
        }
        if (p.has_active_bionic("bio_railgun") &&
            (thrown.made_of("iron") || thrown.made_of("steel"))) {
            m.add_field(tx, ty, fd_electricity, rng(2,3));
        }
    }
    if (thrown.made_of("glass") && !thrown.active && // active means molotov, etc
        rng(0, thrown.volume() + 8) - rng(0, p.str_cur) < thrown.volume()) {
        if (u_see(tx, ty)) {
            add_msg(_("The %s shatters!"), thrown.tname().c_str());
        }

        for (int i = 0; i < thrown.contents.size(); i++) {
            m.add_item_or_charges(tx, ty, thrown.contents[i]);
        }
        sound(tx, ty, 16, _("glass breaking!"));
    } else {
        if(m.has_flag("LIQUID", tx, ty)) {
            sound(tx, ty, 10, _("splash!"));
        } else {
            sound(tx, ty, 8, _("thud."));
        }
        m.add_item_or_charges(tx, ty, thrown);
    }
}

// TODO: Shunt redundant drawing code elsewhere
std::vector<point> game::target(int &x, int &y, int lowx, int lowy, int hix,
                                int hiy, std::vector <Creature*> t, int &target,
                                item *relevent)
{
 std::vector<point> ret;
 int tarx, tary, junk, tart;
 int range=(hix-u.posx);
// First, decide on a target among the monsters, if there are any in range
 if (!t.empty()) {
// Check for previous target
  if (target == -1) {
// If no previous target, target the closest there is
   double closest = -1;
   double dist;
   for (int i = 0; i < t.size(); i++) {
    dist = rl_dist(t[i]->xpos(), t[i]->ypos(), u.posx, u.posy);
    if (closest < 0 || dist < closest) {
     closest = dist;
     target = i;
    }
   }
  }
  x = t[target]->xpos();
  y = t[target]->ypos();
 } else
  target = -1; // No monsters in range, don't use target, reset to -1

 bool sideStyle = use_narrow_sidebar();
 int height = 13;
 int width  = getmaxx(w_messages);
 int top    = sideStyle ? getbegy(w_messages) : (getbegy(w_minimap) + getmaxy(w_minimap));
 int left   = getbegx(w_messages);
 WINDOW* w_target = newwin(height, width, top, left);
 draw_border(w_target);
 mvwprintz(w_target, 0, 2, c_white, "< ");
 if (!relevent) { // currently targetting vehicle to refill with fuel
   wprintz(w_target, c_red, _("Select a vehicle"));
 } else {
   if (relevent == &u.weapon && relevent->is_gun()) {
     wprintz(w_target, c_red, _("Firing %s (%d)"), // - %s (%d)",
            u.weapon.tname().c_str(),// u.weapon.curammo->name.c_str(),
            u.weapon.charges);
   } else {
     wprintz(w_target, c_red, _("Throwing %s"), relevent->tname().c_str());
   }
 }
 wprintz(w_target, c_white, " >");
/* Annoying clutter @ 2 3 4. */
 int text_y = getmaxy(w_target) - 4;
 if (is_mouse_enabled()) {
     --text_y;
 }
 mvwprintz(w_target, text_y++, 1, c_white,
           _("Move cursor to target with directional keys"));
 if (relevent) {
  mvwprintz(w_target, text_y++, 1, c_white,
            _("'<' '>' Cycle targets; 'f' or '.' to fire"));
  mvwprintz(w_target, text_y++, 1, c_white,
            _("'0' target self; '*' toggle snap-to-target"));
 }

 if (is_mouse_enabled()) {
     mvwprintz(w_target, text_y++, 1, c_white,
         _("Mouse: LMB: Target, Wheel: Cycle, RMB: Fire"));
 }

 wrefresh(w_target);
 bool snap_to_target = OPTIONS["SNAP_TO_TARGET"];
 do {
  if (m.sees(u.posx, u.posy, x, y, -1, tart))
    ret = line_to(u.posx, u.posy, x, y, tart);
  else
    ret = line_to(u.posx, u.posy, x, y, 0);

  if(trigdist && trig_dist(u.posx,u.posy, x,y) > range) {
    bool cont=true;
    int cx=x;
    int cy=y;
    for (int i = 0; i < ret.size() && cont; i++) {
      if(trig_dist(u.posx,u.posy, ret[i].x, ret[i].y) > range) {
        ret.resize(i);
        cont=false;
      } else {
        cx=0+ret[i].x; cy=0+ret[i].y;
      }
    }
    x=cx;y=cy;
  }
  point center;
  if (snap_to_target)
   center = point(x, y);
  else
   center = point(u.posx + u.view_offset_x, u.posy + u.view_offset_y);
  // Clear the target window.
  for (int i = 1; i < getmaxy(w_target) - 5; i++) {
   for (int j = 1; j < getmaxx(w_target) - 2; j++)
    mvwputch(w_target, i, j, c_white, ' ');
  }
  /* Start drawing w_terrain things -- possibly move out to centralized draw_terrain_window function as they all should be roughly similar*/
  m.build_map_cache(); // part of the SDLTILES drawing code
  m.draw(w_terrain, center); // embedded in SDL drawing code
  // Draw the Monsters
  for (int i = 0; i < num_zombies(); i++) {
   if (u_see(&(zombie(i)))) {
    zombie(i).draw(w_terrain, center.x, center.y, false);
   }
  }
  // Draw the NPCs
  for (int i = 0; i < active_npc.size(); i++) {
   if (u_see(active_npc[i]->posx, active_npc[i]->posy))
    active_npc[i]->draw(w_terrain, center.x, center.y, false);
  }
  if (x != u.posx || y != u.posy) {

   // Draw the player
   int atx = POSX + u.posx - center.x, aty = POSY + u.posy - center.y;
   if (atx >= 0 && atx < TERRAIN_WINDOW_WIDTH && aty >= 0 && aty < TERRAIN_WINDOW_HEIGHT)
    mvwputch(w_terrain, aty, atx, u.color(), '@');

   // Only draw a highlighted trajectory if we can see the endpoint.
   // Provides feedback to the player, and avoids leaking information about tiles they can't see.
   draw_line(x, y, center, ret);
/*
   if (u_see( x, y)) {
    for (int i = 0; i < ret.size(); i++) {
      int mondex = mon_at(ret[i].x, ret[i].y),
          npcdex = npc_at(ret[i].x, ret[i].y);
      // NPCs and monsters get drawn with inverted colors
      if (mondex != -1 && u_see(&(zombie(mondex))))
       zombie(mondex).draw(w_terrain, center.x, center.y, true);
      else if (npcdex != -1)
       active_npc[npcdex]->draw(w_terrain, center.x, center.y, true);
      else
       m.drawsq(w_terrain, u, ret[i].x, ret[i].y, true,true,center.x, center.y);
    }
   }
//*/
            // Print to target window
            if (!relevent) {
                // currently targetting vehicle to refill with fuel
                vehicle *veh = m.veh_at(x, y);
                if (veh) {
                    mvwprintw(w_target, 1, 1, _("There is a %s"),
                              veh->name.c_str());
                }
            } else if (relevent == &u.weapon && relevent->is_gun()) {
                // firing a gun
                mvwprintw(w_target, 1, 1, _("Range: %d"),
                          rl_dist(u.posx, u.posy, x, y));
                // get the current weapon mode or mods
                std::string mode = "";
                if (u.weapon.mode == "MODE_BURST") {
                    mode = _("Burst");
                } else {
                    item* gunmod = u.weapon.active_gunmod();
                    if (gunmod != NULL) {
                        mode = gunmod->type->name;
                    }
                }
                if (mode != "") {
                    mvwprintw(w_target, 1, 14, _("Firing mode: %s"),
                              mode.c_str());
                }
            } else {
                // throwing something
                mvwprintw(w_target, 1, 1, _("Range: %d"),
                          rl_dist(u.posx, u.posy, x, y));
            }

   const int zid = mon_at(x, y);
   if (zid == -1) {
    if (snap_to_target)
     mvwputch(w_terrain, POSY, POSX, c_red, '*');
    else
     mvwputch(w_terrain, POSY + y - center.y, POSX + x - center.x, c_red, '*');
   } else {
    if (u_see(&(zombie(zid)))) {
     zombie(zid).print_info(w_target,2);
    }
   }
  }
  wrefresh(w_target);
  wrefresh(w_terrain);
  wrefresh(w_status);
  refresh();

  input_context ctxt("TARGET");
  // "ANY_INPUT" should be added before any real help strings
  // Or strings will be writen on window border.
  ctxt.register_action("ANY_INPUT");
  ctxt.register_directions();
  ctxt.register_action("COORDINATE");
  ctxt.register_action("SELECT");
  ctxt.register_action("FIRE");
  ctxt.register_action("NEXT_TARGET");
  ctxt.register_action("PREV_TARGET");
  ctxt.register_action("WAIT");
  ctxt.register_action("CENTER");
  ctxt.register_action("TOGGLE_SNAP_TO_TARGET");
  ctxt.register_action("HELP_KEYBINDINGS");
  ctxt.register_action("QUIT");

  const std::string& action = ctxt.handle_input();


  tarx = 0; tary = 0;
  // Our coordinates will either be determined by coordinate input(mouse),
  // by a direction key, or by the previous value.
  if (action == "SELECT" && ctxt.get_coordinates(g->w_terrain, tarx, tary)) {
      if (!OPTIONS["USE_TILES"] && snap_to_target) {
          // Snap to target doesn't currently work with tiles.
          tarx += x - u.posx;
          tary += y - u.posy;
      }
      tarx -= x;
      tary -= y;


  } else {
    ctxt.get_direction(tarx, tary, action);
    if(tarx == -2) {
        tarx = 0;
        tary = 0;
    }
  }

  /* More drawing to terrain */
  if (tarx != 0 || tary != 0) {
   int mondex = mon_at(x, y), npcdex = npc_at(x, y);
   if (mondex != -1 && u_see(&(zombie(mondex))))
    zombie(mondex).draw(w_terrain, center.x, center.y, false);
   else if (npcdex != -1)
    active_npc[npcdex]->draw(w_terrain, center.x, center.y, false);
   else if (m.sees(u.posx, u.posy, x, y, -1, junk))
    m.drawsq(w_terrain, u, x, y, false, true, center.x, center.y);
   else
    mvwputch(w_terrain, POSY, POSX, c_black, 'X');
   x += tarx;
   y += tary;
   if (x < lowx)
    x = lowx;
   else if (x > hix)
    x = hix;
   if (y < lowy)
    y = lowy;
   else if (y > hiy)
    y = hiy;
  } else if ((action == "PREV_TARGET") && (target != -1)) {
   target--;
   if (target == -1) target = t.size() - 1;
   x = t[target]->xpos();
   y = t[target]->ypos();
  } else if ((action == "NEXT_TARGET") && (target != -1)) {
   target++;
   if (target == t.size()) target = 0;
   x = t[target]->xpos();
   y = t[target]->ypos();
  } else if (action == "WAIT" || action == "FIRE") {
   for (int i = 0; i < t.size(); i++) {
    if (t[i]->xpos() == x && t[i]->ypos() == y)
     target = i;
   }
   if (u.posx == x && u.posy == y)
       ret.clear();
   break;
  } else if (action == "CENTER") {
   x = u.posx;
   y = u.posy;
   ret.clear();
  } else if (action == "TOGGLE_SNAP_TO_TARGET")
   snap_to_target = !snap_to_target;
  else if (action == "QUIT") { // return empty vector (cancel)
   ret.clear();
   break;
  }
 } while (true);

 return ret;
}

int time_to_fire(player &p, it_gun* firing)
{
 int time = 0;
 if (firing->skill_used == Skill::skill("pistol")) {
   if (p.skillLevel("pistol") > 6)
     time = 10;
   else
     time = (80 - 10 * p.skillLevel("pistol"));
 } else if (firing->skill_used == Skill::skill("shotgun")) {
   if (p.skillLevel("shotgun") > 3)
     time = 70;
   else
     time = (150 - 25 * p.skillLevel("shotgun"));
 } else if (firing->skill_used == Skill::skill("smg")) {
   if (p.skillLevel("smg") > 5)
     time = 20;
   else
     time = (80 - 10 * p.skillLevel("smg"));
 } else if (firing->skill_used == Skill::skill("rifle")) {
   if (p.skillLevel("rifle") > 8)
     time = 30;
   else
     time = (150 - 15 * p.skillLevel("rifle"));
 } else if (firing->skill_used == Skill::skill("archery")) {
   if (p.skillLevel("archery") > 8)
     time = 20;
   else
     time = (220 - 25 * p.skillLevel("archery"));
 } else if (firing->skill_used == Skill::skill("throw")) {
   if (p.skillLevel("throw") > 6){
     time = 50;
   }else{
     time = (220 - 25 * p.skillLevel("throw"));
   }
 } else if (firing->skill_used == Skill::skill("launcher")) {
   if (p.skillLevel("launcher") > 8)
     time = 30;
   else
     time = (200 - 20 * p.skillLevel("launcher"));
 }
  else {
   debugmsg("Why is shooting %s using %s skill?", (firing->name).c_str(), firing->skill_used->name().c_str());
   time =  0;
 }

 return time;
}

void make_gun_sound_effect(player &p, bool burst, item* weapon)
{
 std::string gunsound;
 // noise() doesn't suport gunmods, but it does return the right value
 int noise = p.weapon.noise();
 if(weapon->is_gunmod()){ //TODO make this produce the correct sound
  g->sound(p.posx, p.posy, noise, "Boom!");
  return;
 }

 it_gun* weapontype = dynamic_cast<it_gun*>(weapon->type);
 if (weapontype->ammo_effects.count("LASER") || weapontype->ammo_effects.count("PLASMA")) {
  if (noise < 20) {
    gunsound = _("Fzzt!");
  } else if (noise < 40) {
    gunsound = _("Pew!");
  } else if (noise < 60) {
    gunsound = _("Tsewww!");
  } else {
    gunsound = _("Kra-kow!!");
  }
 } else if (weapontype->ammo_effects.count("LIGHTNING")) {
  if (noise < 20) {
    gunsound = _("Bzzt!");
  } else if (noise < 40) {
    gunsound = _("Bzap!");
  } else if (noise < 60) {
    gunsound = _("Bzaapp!");
  } else {
    gunsound = _("Kra-koom!!");
  }
 } else {
  if (noise < 5) {
   if (burst)
   gunsound = _("Brrrip!");
   else
   gunsound = _("plink!");
  } else if (noise < 25) {
   if (burst)
   gunsound = _("Brrrap!");
   else
   gunsound = _("bang!");
  } else if (noise < 60) {
   if (burst)
   gunsound = _("P-p-p-pow!");
   else
   gunsound = _("blam!");
  } else {
   if (burst)
   gunsound = _("Kaboom!!");
   else
   gunsound = _("kerblam!");
  }
 }

 if (weapon->curammo->type == "40mm")
  g->sound(p.posx, p.posy, 8, _("Thunk!"));
 else if (weapon->curammo->type == "gasoline" || weapon->curammo->type == "66mm" ||
     weapon->curammo->type == "84x246mm" || weapon->curammo->type == "m235")
  g->sound(p.posx, p.posy, 4, _("Fwoosh!"));
 else if (weapon->curammo->type != "bolt" &&
          weapon->curammo->type != "arrow" &&
          weapon->curammo->type != "pebble" &&
          weapon->curammo->type != "fishspear" &&
          weapon->curammo->type != "dart")
  g->sound(p.posx, p.posy, noise, gunsound);
}

// utility functions for projectile_attack
double player::get_weapon_dispersion(item *weapon) {
    int weapon_skill_level = 0;
    if(weapon->is_gunmod()) {
      it_gunmod* firing = dynamic_cast<it_gunmod *>(weapon->type);
      weapon_skill_level = skillLevel(firing->skill_used);
    } else {
      it_gun* firing = dynamic_cast<it_gun *>(weapon->type);
      weapon_skill_level = skillLevel(firing->skill_used);
    }
    double dispersion = 0.; // Measured in quarter-degrees.
    // Up to 0.75 degrees for each skill point < 8.
    if (weapon_skill_level < 8) {
        dispersion += rng(0, 3 * (8 - weapon_skill_level));
    }
    // Up to 0.25 deg per each skill point < 9.
    if (skillLevel("gun") < 9) {
        dispersion += rng(0, 9 - skillLevel("gun"));
    }

    dispersion += rng(0, ranged_dex_mod());
    dispersion += rng(0, ranged_per_mod());

    dispersion += rng(0, 2 * encumb(bp_arms)) + rng(0, 4 * encumb(bp_eyes));

    dispersion += rng(0, weapon->curammo->dispersion);
    // item::dispersion() doesn't support gunmods.
    dispersion += rng(0, weapon->dispersion());
    int adj_recoil = recoil + driving_recoil;
    dispersion += rng(int(adj_recoil / 4), adj_recoil);

    // this is what the total bonus USED to look like
    // rng(0,x) on each term in the sum
    // 3 * skill + skill + 2 * dex + 2 * per
    // - 2*p.encumb(bp_arms) - 4*p.encumb(bp_eyes) - 5/8 * recoil

    // old targeting bionic suddenly went from 0.8 to 0.65 when LONG_RANGE was
    // crossed, so increasing range by 1 would actually increase accuracy by a
    // lot. This is kind of a compromise
    if (has_bionic("bio_targeting"))
        dispersion *= 0.75;
    if ((is_underwater() && !weapon->has_flag("UNDERWATER_GUN")) || // Range is effectively four times longer when shooting unflagged guns underwater.
            (!is_underwater() && weapon->has_flag("UNDERWATER_GUN"))) { // Range is effectively four times longer when shooting flagged guns out of water.
        dispersion *= 4;
    }

    if (dispersion < 0) { return 0; }
    return dispersion;
}

int recoil_add(player &p)
{
 // Gunmods don't have atype,so use guns.
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 // item::recoil() doesn't suport gunmods, so call it on player gun.
 int ret = p.weapon.recoil();
 ret -= rng(p.str_cur / 2, p.str_cur);
 ret -= rng(0, p.skillLevel(firing->skill_used) / 2);
 if (ret > 0)
  return ret;
 return 0;
}

void shoot_player(player &p, player *h, int &dam, double goodhit)
{
    int npcdex = g->npc_at(h->posx, h->posy);
    // Gunmods don't have a type, so use the player gun type.
    it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
    body_part hit = bp_torso;
    if (goodhit < .003) {
        hit = bp_eyes;
        dam = rng(3 * dam, 5 * dam);
        p.practice(g->turn, firing->skill_used, 5);
    } else if (goodhit < .066) {
        if (one_in(25)) {
            hit = bp_eyes;
        } else if (one_in(15)) {
            hit = bp_mouth;
        } else {
            hit = bp_head;
        }
        dam = rng(2 * dam, 5 * dam);
        p.practice(g->turn, firing->skill_used, 5);
    } else if (goodhit < .2) {
        hit = bp_torso;
        dam = rng(dam, 2 * dam);
        p.practice(g->turn, firing->skill_used, 2);
    } else if (goodhit < .4) {
        if (one_in(3)) {
            hit = bp_torso;
        } else if (one_in(2)) {
            hit = bp_arms;
        } else {
            hit = bp_legs;
        }
        dam = rng(int(dam * .9), int(dam * 1.5));
        p.practice(g->turn, firing->skill_used, rng(0, 1));
    } else if (goodhit < .5) {
        if (one_in(2)) {
            hit = bp_arms;
        } else {
            hit = bp_legs;
        }
        dam = rng(dam / 2, dam);
    } else {
        dam = 0;
    }
    if (dam > 0) {
        int side = random_side(hit);
        h->moves -= rng(0, dam);
        if (h == &(g->u)) {
            g->add_msg(_("%s shoots your %s for %d damage!"), p.name.c_str(),
                          body_part_name(hit, side).c_str(), dam);
        } else {
            if (&p == &(g->u)) {
                g->add_msg(_("You shoot %s's %s."), h->name.c_str(),
                               body_part_name(hit, side).c_str());
                g->active_npc[npcdex]->make_angry();
            } else if (g->u_see(h->posx, h->posy)) {
                g->add_msg(_("%s shoots %s's %s."),
                   (g->u_see(p.posx, p.posy) ? p.name.c_str() : _("Someone")),
                    h->name.c_str(), body_part_name(hit, side).c_str());
            }
        }
        h->hit(&p, hit, side, 0, dam);
    }
}

void splatter( std::vector<point> trajectory, int dam, Creature* target )
{
    if( dam <= 0 ) {
        return;
    }
    field_id blood = fd_blood;
    if( target != NULL ) {
        if( target->get_material() != "flesh" || target->has_flag(MF_VERMIN) ) {
            return;
        }
        if( target->has_flag(MF_BILE_BLOOD) ) {
            blood = fd_bile;
        } else if( target->has_flag(MF_ACID_BLOOD) ) {
            blood = fd_acid;
        }
    }

    int distance = 1;
    if( dam > 50 ) {
        distance = 3;
    } else if( dam > 20 ) {
        distance = 2;
    }

    std::vector<point> spurt = continue_line( trajectory, distance );

    for( int i = 0; i < spurt.size(); i++ ) {
        int tarx = spurt[i].x;
        int tary = spurt[i].y;
        g->m.adjust_field_strength( point(tarx, tary), blood, 1 );
        if( g->m.move_cost(tarx, tary) == 0 ) {
            // Blood splatters stop at walls.
            break;
        }
    }
}


