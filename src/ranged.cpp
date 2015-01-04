#include <vector>
#include <string>
#include "game.h"
#include "output.h"
#include "line.h"
#include "skill.h"
#include "rng.h"
#include "item.h"
#include "options.h"
#include "action.h"
#include "input.h"
#include "messages.h"

int time_to_fire(player &p, const itype &firing);
int recoil_add(player &p, const item &gun);
void make_gun_sound_effect(player &p, bool burst, item *weapon);
extern bool is_valid_in_w_terrain(int, int);

void splatter(std::vector<point> trajectory, int dam, Creature *target = NULL);

double Creature::projectile_attack(const projectile &proj, int targetx, int targety,
                                   double shot_dispersion)
{
    return projectile_attack(proj, xpos(), ypos(), targetx, targety, shot_dispersion);
}

/* Adjust dispersion cutoff thresholds per skill type.
 * If these drift significantly might need to adjust the values here.
 * Keep in mind these include factoring in the best ammo and the best mods.
 * The target is being able to skill up to lvl 10/10 guns/guntype with average (8) perception.
 * That means the adjustment should be dispersion of best-in-class weapon - 8.
 *
 * pistol 0 (.22 is 8, S&W 22A can get down to 0 with significant modding.)
 * rifle 0 (There are any number of rifles you can get down to 0/0.)
 * smg 0 (H&K MP5 can get dropped to 0, leaving 9mm +P+ as the limiting factor at 8.)
 * shotgun 0 (no comment.)
 * launcher 0 (no comment.)
 * archery 6 (best craftable bow is composite at 10, and best arrow is wood at 4)
 * throwing 13 (sling)
 * As a simple tweak, we're shifting the ranges so they match,
 * so if you acquire the best of a weapon type you can reach max skill with it.
 */
int ranged_skill_offset( std::string skill )
{
    if( skill == "pistol" ) {
        return 0;
    } else if( skill == "rifle" ) {
        return 0;
    } else if( skill == "smg" ) {
        return 0;
    } else if( skill == "shotgun" ) {
        return 0;
    } else if( skill == "launcher" ) {
        return 0;
    } else if( skill == "archery" ) {
        return 135;
    } else if( skill == "throw" ) {
        return 195;
    }
    return 0;
}

double Creature::projectile_attack(const projectile &proj, int sourcex, int sourcey,
                                   int targetx, int targety, double shot_dispersion)
{
    double range = rl_dist(sourcex, sourcey, targetx, targety);
    // .013 * trange is a computationally cheap version of finding the tangent in degrees.
    // 0.0002166... is used because the unit of dispersion is MOA (1/60 degree).
    // It's also generous; missed_by will be rather short.
    double missed_by = shot_dispersion * 0.00021666666666666666 * range;
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
        trajectory = line_to(sourcex, sourcey, targetx, targety, tart);
    } else {
        trajectory = line_to(sourcex, sourcey, targetx, targety, 0);
    }

    // Set up a timespec for use in the nanosleep function below
    timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000 * OPTIONS["ANIMATION_DELAY"];

    int dam = proj.impact.total_damage() + proj.payload.total_damage();
    itype *curammo = proj.ammo;

    // Trace the trajectory, doing damage in order
    int tx = sourcex;
    int ty = sourcey;
    int px = sourcex;
    int py = sourcey;

    // if this is a vehicle mounted turret, which vehicle is it mounted on?
    const vehicle *in_veh = is_fake() ? g->m.veh_at(xpos(), ypos()) : NULL;

    //Start this now in case we hit something early
    std::vector<point> blood_traj = std::vector<point>();
    for (size_t i = 0; i < trajectory.size() && (dam > 0 || (proj.proj_effects.count("FLAME"))); i++) {
        blood_traj.push_back(trajectory[i]);
        px = tx;
        py = ty;
        (void) px;
        (void) py;
        tx = trajectory[i].x;
        ty = trajectory[i].y;
        // Drawing the bullet uses player u, and not player p, because it's drawn
        // relative to YOUR position, which may not be the gunman's position.
        g->draw_bullet(g->u, tx, ty, (int)i, trajectory, proj.proj_effects.count("FLAME") ? '#' : '*', ts);

        if( in_veh != nullptr ) {
            int part;
            vehicle *other = g->m.veh_at( tx, ty, part );
            if( in_veh == other && other->is_inside( part ) ) {
                continue; // Turret is on the roof and can't hit anything inside
            }
        }
        /* TODO: add running out of momentum back in
        if (dam <= 0 && !(proj.proj_effects.count("FLAME"))) { // Ran out of momentum.
            break;
        }
        */

        Creature *critter = g->critter_at(tx, ty);
        monster *mon = dynamic_cast<monster *>(critter);
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
            cur_missed_by = std::max(rng_float(0, 1.5) + (1 - missed_by), 0.2);
        } else {
            cur_missed_by = missed_by;
        }
        if (critter != NULL && cur_missed_by <= 1.0) {
            if( in_veh != nullptr && g->m.veh_at( tx, ty ) == in_veh && critter->is_player() ) {
                // Turret either was aimed by the player (who is now ducking) and shoots from above
                // Or was just IFFing, giving lots of warnings and time to get out of the line of fire
                continue;
            }
            dealt_damage_instance dealt_dam;
            bool passed_through = critter->deal_projectile_attack(this, cur_missed_by, proj, dealt_dam) == 1;
            if (dealt_dam.total_damage() > 0) {
                splatter( blood_traj, dam, critter );
            }
            if (!passed_through) {
                dam = 0;
            }
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
        (
            (proj.proj_effects.count("RECOVER_3") && !one_in(3)) ||
            (proj.proj_effects.count("RECOVER_5") && !one_in(5)) ||
            (proj.proj_effects.count("RECOVER_10") && !one_in(10)) ||
            (proj.proj_effects.count("RECOVER_15") && !one_in(15)) ||
            (proj.proj_effects.count("RECOVER_25") && !one_in(25))
        )
       ) {
        item ammotmp = item(curammo->id, 0);
        ammotmp.charges = 1;
        g->m.add_item_or_charges(tx, ty, ammotmp);
    }

    ammo_effects(tx, ty, proj.proj_effects);

    if (proj.proj_effects.count("BOUNCE")) {
        for (unsigned long int i = 0; i < g->num_zombies(); i++) {
            monster &z = g->zombie(i);
            if( z.is_dead() ) {
                continue;
            }
            // search for monsters in radius 4 around impact site
            if( rl_dist( z.posx(), z.posy(), tx, ty ) <= 4 &&
                g->m.sees( z.posx(), z.posy(), tx, ty, -1, tart ) ) {
                // don't hit targets that have already been hit
                if (!z.has_effect("bounced")) {
                    add_msg(_("The attack bounced to %s!"), z.name().c_str());
                    z.add_effect("bounced", 1);
                    projectile_attack(proj, tx, ty, z.posx(), z.posy(), shot_dispersion);
                    break;
                }
            }
        }
    }

    return missed_by;
}

bool player::handle_gun_damage( const itype &firingt, const std::set<std::string> &curammo_effects )
{
    const islot_gun *firing = firingt.gun.get();
    // Here we check if we're underwater and whether we should misfire.
    // As a result this causes no damage to the firearm, note that some guns are waterproof
    // and so are immune to this effect, note also that WATERPROOF_GUN status does not
    // mean the gun will actually be accurate underwater.
    if (firing->skill_used != Skill::skill("archery") &&
        firing->skill_used != Skill::skill("throw")) {
        if (is_underwater() && !weapon.has_flag("WATERPROOF_GUN") && one_in(firing->durability)) {
            add_msg_player_or_npc(_("Your %s misfires with a wet click!"),
                                  _("<npcname>'s %s misfires with a wet click!"),
                                  weapon.tname().c_str());
            return false;
            // Here we check for a chance for the weapon to suffer a mechanical malfunction.
            // Note that some weapons never jam up 'NEVER_JAMS' and thus are immune to this
            // effect as current guns have a durability between 5 and 9 this results in
            // a chance of mechanical failure between 1/64 and 1/1024 on any given shot.
            // the malfunction may cause damage, but never enough to push the weapon beyond 'shattered'
        } else if ((one_in(2 << firing->durability)) && !weapon.has_flag("NEVER_JAMS")) {
            add_msg_player_or_npc(_("Your %s malfunctions!"),
                                  _("<npcname>'s %s malfunctions!"),
                                  weapon.tname().c_str());
            if ((weapon.damage < 4) && one_in(4 * firing->durability)) {
                add_msg_player_or_npc(m_bad, _("Your %s is damaged by the mechanical malfunction!"),
                                      _("<npcname>'s %s is damaged by the mechanical malfunction!"),
                                      weapon.tname().c_str());
                // Don't increment until after the message
                weapon.damage++;
            }
            return false;
            // Here we check for a chance for the weapon to suffer a misfire due to
            // using OEM bullets. Note that these misfires cause no damage to the weapon and
            // some types of ammunition are immune to this effect via the NEVER_MISFIRES effect.
        } else if (!curammo_effects.count("NEVER_MISFIRES") && one_in(1728)) {
            add_msg_player_or_npc(_("Your %s misfires with a dry click!"),
                                  _("<npcname>'s %s misfires with a dry click!"),
                                  weapon.tname().c_str());
            return false;
            // Here we check for a chance for the weapon to suffer a misfire due to
            // using player-made 'RECYCLED' bullets. Note that not all forms of
            // player-made ammunition have this effect the misfire may cause damage, but never
            // enough to push the weapon beyond 'shattered'.
        } else if (curammo_effects.count("RECYCLED") && one_in(256)) {
            add_msg_player_or_npc(_("Your %s misfires with a muffled click!"),
                                  _("<npcname>'s %s misfires with a muffled click!"),
                                  weapon.tname().c_str());
            if ((weapon.damage < 4) && one_in(firing->durability)) {
                add_msg_player_or_npc(m_bad, _("Your %s is damaged by the misfired round!"),
                                      _("<npcname>'s %s is damaged by the misfired round!"),
                                      weapon.tname().c_str());
                // Don't increment until after the message
                weapon.damage++;
            }
            return false;
        }
    }
    return true;
}

void player::fire_gun(int tarx, int tary, bool burst)
{
    item *gunmod = weapon.active_gunmod();
    itype *curammo = NULL;
    item *used_weapon = NULL;

    if( gunmod != nullptr ) {
        used_weapon = gunmod;
    } else if( weapon.is_auxiliary_gunmod() ) {
        add_msg( m_info, _( "The %s must be attached to a gun, it can not be fired separately." ), weapon.tname().c_str() );
        return;
    } else {
        used_weapon = &weapon;
    }
    const bool is_charger_gun = used_weapon->update_charger_gun_ammo();
    curammo = used_weapon->get_curammo();

    if( curammo == nullptr ) {
        debugmsg( "%s tried to fire an empty gun (%s).", name.c_str(),
                  used_weapon->tname().c_str() );
        return;
    }
    if( !used_weapon->is_gun() ) {
        debugmsg("%s tried to fire a non-gun (%s).", name.c_str(),
                 used_weapon->tname().c_str());
        return;
    }
    const Skill* skill_used = Skill::skill( used_weapon->gun_skill() );

    projectile proj; // damage will be set later
    proj.aoe_size = 0;
    proj.ammo = curammo;
    proj.speed = 1000;

    const auto &curammo_effects = curammo->ammo->ammo_effects;
    const auto &gun_effects = used_weapon->type->gun->ammo_effects;
    proj.proj_effects.insert(gun_effects.begin(), gun_effects.end());
    proj.proj_effects.insert(curammo_effects.begin(), curammo_effects.end());

    proj.wide = (curammo->phase == LIQUID ||
                 proj.proj_effects.count("SHOT") || proj.proj_effects.count("BOUNCE"));

    proj.drops = (proj.proj_effects.count("RECOVER_3") ||
                  proj.proj_effects.count("RECOVER_5") ||
                  proj.proj_effects.count("RECOVER_10") ||
                  proj.proj_effects.count("RECOVER_15") ||
                  proj.proj_effects.count("RECOVER_25") );

    //int x = xpos(), y = ypos();
    if (has_trait("TRIGGERHAPPY") && one_in(30)) {
        burst = true;
    }
    if (burst && used_weapon->burst_size() < 2) {
        burst = false; // Can't burst fire a semi-auto
    }

    // Use different amounts of time depending on the type of gun and our skill
    moves -= time_to_fire(*this, *used_weapon->type);

    // Decide how many shots to fire
    long num_shots = 1;
    if (burst) {
        num_shots = used_weapon->burst_size();
    }
    if (num_shots > used_weapon->num_charges() &&
        !is_charger_gun && !used_weapon->has_flag("NO_AMMO")) {
        num_shots = used_weapon->num_charges();
    }

    if (num_shots == 0) {
        debugmsg("game::fire() - num_shots = 0!");
    }

    int ups_drain = 0;
    int adv_ups_drain = 0;
    int bio_power_drain = 0;
    if( used_weapon->type->gun->ups_charges > 0 ) {
        ups_drain = used_weapon->type->gun->ups_charges;
        adv_ups_drain = std::min( 1, ups_drain * 3 / 5 );
        bio_power_drain = std::min( 1, ups_drain / 5 );
    }

    // cap our maximum burst size by the amount of UPS power left
    if (ups_drain > 0 || adv_ups_drain > 0 || bio_power_drain > 0)
        while (!(has_charges("UPS_off", ups_drain * num_shots) ||
                 has_charges("adv_UPS_off", adv_ups_drain * num_shots) ||
                 (has_bionic("bio_ups") && power_level >= (bio_power_drain * num_shots)))) {
            num_shots--;
        }

    // This is expensive, let's cache. todo: figure out if we need weapon.range(&p);
    const int weaponrange = used_weapon->gun_range( this );

    // If the dispersion from the weapon is greater than the dispersion from your skill,
    // you can't tell if you need to correct or the gun messed you up, so you can't learn.
    const int weapon_dispersion = used_weapon->get_curammo()->ammo->dispersion + used_weapon->gun_dispersion();
    const int player_dispersion = skill_dispersion( used_weapon, false ) +
        ranged_skill_offset( used_weapon->gun_skill() );
    // High perception allows you to pick out details better, low perception interferes.
    const bool train_skill = weapon_dispersion < player_dispersion + rng(0, get_per());
    if( train_skill ) {
        practice( skill_used, 4 + (num_shots / 2));
    } else if( one_in(30) ) {
        add_msg_if_player(m_info, _("You'll need a more accurate gun to keep improving your aim."));
    }

    // chance to disarm an NPC with a whip if skill is high enough
    if(proj.proj_effects.count("WHIP") && (this->skillLevel("melee") > 5) && one_in(3)) {
        int npcdex = g->npc_at(tarx, tary);
        if(npcdex != -1) {
            npc *p = g->active_npc[npcdex];
            if(!p->weapon.is_null()) {
                item weap = p->remove_weapon();
                add_msg_if_player(m_good, _("You disarm %s's %s using your whip!"), p->name.c_str(),
                                  weap.tname().c_str());
                g->m.add_item_or_charges(tarx + rng(-1, 1), tary + rng(-1, 1), weap);
            }
        }
    }

    const bool trigger_happy = has_trait( "TRIGGERHAPPY" );
    for (int curshot = 0; curshot < num_shots; curshot++) {
        // Burst-fire weapons allow us to pick a new target after killing the first
        const auto critter = g->critter_at( tarx, tary );
        if ( curshot > 0 && ( critter == nullptr || critter->is_dead_state() ) ) {
            auto const near_range = std::min( 2 + skillLevel( "gun" ), weaponrange );
            auto new_targets = get_visible_creatures( weaponrange );
            for( auto it = new_targets.begin(); it != new_targets.end(); ) {
                auto &z = **it;
                if( attitude_to( z ) == A_FRIENDLY ) {
                    if( !trigger_happy ) {
                        it = new_targets.erase( it );
                        continue;
                    } else if( !one_in( 10 ) ) {
                        // Trigger happy sometimes doesn't care whom to shoot.
                        it = new_targets.erase( it );
                        continue;
                    }
                }
                // search for monsters in radius
                if( rl_dist( z.xpos(), z.ypos(), tarx, tary) <= near_range ) {
                    // oh you're not dead and I don't like you. Hello!
                    ++it;
                } else {
                    it = new_targets.erase( it );
                }
            }

            if ( new_targets.empty() == false ) {    /* new victim! or last victim moved */
                /* 1 victim list unless wildly spraying */
                int target_picked = rng(0, new_targets.size() - 1);
                tarx = new_targets[target_picked]->xpos();
                tary = new_targets[target_picked]->ypos();
            } else if( ( !trigger_happy || one_in(3) ) &&
                       ( skillLevel("gun") >= 7 || one_in(7 - skillLevel("gun")) ) ) {
                // Triggerhappy has a higher chance of firing repeatedly.
                // Otherwise it's dominated by how much practice you've had.
                return;
            }
        }

        // Drop a shell casing if appropriate.
        itype_id casing_type = curammo->ammo->casing;
        if( casing_type != "NULL" && !casing_type.empty() ) {
            if( used_weapon->has_flag("RELOAD_EJECT") ) {
                const int num_casings = used_weapon->get_var( "CASINGS", 0 );
                used_weapon->set_var( "CASINGS", num_casings + 1 );
            } else {
                item casing;
                casing.make(casing_type);
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
        }

        // Use up a round (or 100)
        if (used_weapon->has_flag("FIRE_100")) {
            used_weapon->charges -= 100;
        } else if (used_weapon->has_flag("FIRE_50")) {
            used_weapon->charges -= 50;
        } else if (used_weapon->has_flag("FIRE_20")) {
            used_weapon->charges -= 20;
        } else if( used_weapon->deactivate_charger_gun() ) {
            // Done, charger gun is deactivated.
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
        } else if (has_charges("UPS_off", ups_drain)) {
            use_charges("UPS_off", ups_drain);
        } else if (has_bionic("bio_ups")) {
            charge_power(-1 * bio_power_drain);
        }

        if( !handle_gun_damage( *used_weapon->type, curammo_effects ) ) {
            return;
        }

        make_gun_sound_effect(*this, burst, used_weapon);

        double total_dispersion = get_weapon_dispersion(used_weapon, true);
        //debugmsg("%f",total_dispersion);
        int range = rl_dist(xpos(), ypos(), tarx, tary);
        // penalties for point-blank
        // TODO: why is this using the weapon item, is this correct (may use the fired gun instead?)
        if (range < int(weapon.type->volume / 3) && curammo->ammo->type != "shot") {
            total_dispersion *= double(weapon.type->volume / 3) / double(range);
        }

        // rifle has less range penalty past LONG_RANGE
        if (skill_used == Skill::skill("rifle") && range > LONG_RANGE) {
            total_dispersion *= 1 - 0.4 * double(range - LONG_RANGE) / double(range);
        }

        if (curshot > 0) {
            // TODO: or should use the recoil of the whole gun, not just the auxiliary gunmod?
            if (recoil_add(*this, *used_weapon) % 2 == 1) {
                recoil++;
            }
            recoil += recoil_add(*this, *used_weapon) / 2;
        } else {
            recoil += recoil_add(*this, *used_weapon);
        }

        int mtarx = tarx;
        int mtary = tary;

        int adjusted_damage = used_weapon->gun_damage();
        int armor_penetration = used_weapon->gun_pierce();

        proj.impact = damage_instance::physical(0, adjusted_damage, 0, armor_penetration);

        double missed_by = projectile_attack(proj, mtarx, mtary, total_dispersion);
        if (missed_by <= .1) { // TODO: check head existence for headshot
            lifetime_stats()->headshots++;
        }

        if (!train_skill) {
            practice( skill_used, 0 ); // practice, but do not train
        } else if (missed_by <= .1) {
            practice( skill_used, 5 );
        } else if (missed_by <= .2) {
            practice( skill_used, 3 );
        } else if (missed_by <= .4) {
            practice( skill_used, 2 );
        } else if (missed_by <= .6) {
            practice( skill_used, 1 );
        }

    }

    if (used_weapon->num_charges() == 0) {
        used_weapon->unset_curammo();
    }

    if( train_skill ) {
        practice( "gun", 5 );
    } else {
        practice( "gun", 0 );
    }
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

    deviation += rng(0, (p.encumb(bp_hand_l) + p.encumb(bp_hand_r)) + p.encumb(bp_eyes) + 1);
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
    bool do_railgun = (p.has_active_bionic("bio_railgun") &&
            (thrown.made_of("iron") || thrown.made_of("steel")));

    if (missed_by >= 1) {
        // We missed D:
        // Shoot a random nearby space?
        if (missed_by > 9.0) {
            missed_by = 9.0;
        }

        tarx += rng(0 - int(sqrt(missed_by)), int(sqrt(missed_by)));
        tary += rng(0 - int(sqrt(missed_by)), int(sqrt(missed_by)));
        if (m.sees(p.posx, p.posy, tarx, tary, -1, tart)) {
            trajectory = line_to(p.posx, p.posy, tarx, tary, tart);
        } else {
            trajectory = line_to(p.posx, p.posy, tarx, tary, 0);
        }
        missed = true;
        p.add_msg_if_player(_("You miss!"));
    } else if (missed_by >= .6) {
        // Hit the space, but not the monster there
        missed = true;
        p.add_msg_if_player(_("You barely miss!"));
    }

    // The damage dealt due to item's weight and player's strength
    int real_dam = ( (thrown.weight() / 452)
                     + (thrown.type->melee_dam / 2)
                     + (p.str_cur / 2) )
                   / (2.0 + (thrown.volume() / 4.0));
    if (real_dam > thrown.weight() / 40) {
        real_dam = thrown.weight() / 40;
    }
    if (do_railgun) {
        real_dam *= 2;
    }

    // Item will shatter upon landing, destroying the item, dealing damage, and making noise
    bool shatter = ( thrown.made_of("glass") && !thrown.active && // active = molotov, etc.
            rng(0, thrown.volume() + 8) - rng(0, p.str_cur) < thrown.volume() );

    int dam = real_dam;
    int tx = 0, ty = 0;

    // Loop through all squares of the trajectory, stop if we hit anything on the way
    size_t i = 0;
    for (i = 0; i < trajectory.size() && dam >= 0; i++) {
        std::string message = "";
        double goodhit = missed_by;
        tx = trajectory[i].x;
        ty = trajectory[i].y;

        bool hit_something = false;
        const int zid = mon_at(tx, ty);
        const int npcID = npc_at(tx, ty);

        monster *z = nullptr;
        npc *guy = nullptr;

        // Make railgun sparks
        if (do_railgun) {
            m.add_field(tx, ty, fd_electricity, rng(2, 3));
        }

        // Check if we hit a zombie or NPC
        // Can be either the one we aimed for, or one that was in the way
        if (zid != -1 && (!missed || one_in(7 - int(zombie(zid).type->size)))) {
            z = &zombie(zid);
            hit_something = true;
        } else if (npcID != -1 && (!missed || one_in(4))) {
            guy = g->active_npc[npcID];
            hit_something = true;
        }

        if (hit_something) {
            // Check if we manage to do cutting damage
            if (rng(0, 100) < 20 + skillLevel * 12 && thrown.type->melee_cut > 0) {
                if (!p.is_npc()) {
                    if (zid != -1) {
                        message += string_format(_(" You cut the %s!"), z->name().c_str());
                    } else if (npcID != -1) {
                        message += string_format(_(" You cut %s!"), guy->name.c_str());
                    }
                }
                if (zid != -1 && thrown.type->melee_cut > z->get_armor_cut(bp_torso)) {
                    dam += (thrown.type->melee_cut - z->get_armor_cut(bp_torso));
                } else if (npcID != -1 && thrown.type->melee_cut > guy->get_armor_cut(bp_torso)) {
                    dam += (thrown.type->melee_cut - guy->get_armor_cut(bp_torso));
                }
            }

            // Deal extra cut damage if the item breaks
            if (shatter) {
                int glassdam = rng(0, thrown.volume() * 2);
                if (zid != -1 && glassdam > z->get_armor_cut(bp_torso)) {
                    dam += (glassdam - z->get_armor_cut(bp_torso));
                } else if (npcID != -1 && glassdam > guy->get_armor_cut(bp_torso)) {
                    dam += (glassdam - guy->get_armor_cut(bp_torso));
                }
            }

            if (i < trajectory.size() - 1) {
                goodhit = double(rand() / RAND_MAX) / 2.0;
            }
            game_message_type gmtSCTcolor = m_good;
            body_part bp = bp_torso; // for NPCs
            if (goodhit < .1) {
                message = _("Headshot!");
                gmtSCTcolor = m_headshot;
                bp = bp_head;
                dam = rng(dam, dam * 3);
                p.practice( "throw", 5 );
                p.lifetime_stats()->headshots++;
            } else if (goodhit < .2) {
                message = _("Critical!");
                gmtSCTcolor = m_critical;
                dam = rng(dam, dam * 2);
                p.practice( "throw", 2 );
            } else if (goodhit < .4) {
                dam = rng(dam / 2, int(dam * 1.5));
            } else if (goodhit < .5) {
                message = _("Grazing hit.");
                gmtSCTcolor = m_grazing;
                dam = rng(0, dam);
            }

            // Combat text and message
            if (u_see(tx, ty)) {
                nc_color color;
                std::string health_bar = "";
                if (zid != -1) {
                    get_HP_Bar(dam, z->get_hp_max(), color, health_bar, true);
                    SCT.add(z->xpos(),
                            z->ypos(),
                            direction_from(0, 0, z->xpos() - p.posx, z->ypos() - p.posy),
                            health_bar.c_str(), m_good,
                            message, gmtSCTcolor);
                    p.add_msg_player_or_npc(m_good, _("%s You hit the %s for %d damage."),
                                            _("%s <npcname> hits the %s for %d damage."),
                                            message.c_str(), z->name().c_str(), dam);
                } else if (npcID != -1) {
                    get_HP_Bar(dam, guy->get_hp_max(bodypart_to_hp_part(bp)), color, health_bar, true);
                    SCT.add(guy->xpos(),
                            guy->ypos(),
                            direction_from(0, 0, guy->xpos() - p.posx, guy->ypos() - p.posy),
                            health_bar.c_str(), m_good,
                            message, gmtSCTcolor);
                    p.add_msg_player_or_npc(m_good, _("%s You hit %s for %d damage."),
                                            _("%s <npcname> hits %s for %d damage."),
                                            message.c_str(), guy->name.c_str(), dam);
                }
            }

            // actually deal damage now
            if (zid != -1) {
                z->apply_damage( &p, bp_torso, dam );
            } else if (npcID != -1) {
                guy->apply_damage( &p, bp, dam );
                if (guy->is_dead_state()) {
                    guy->die(&p);
                }
            }
            break; // trajectory stops at this square
            // end if (hit_something)
        } else { // No monster hit, but the terrain might be. (e.g. window)
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
            break;
        }
    }

    // Add the thrown item to the map at the place it stopped (tx, ty)
    if (shatter) {
        if (u_see(tx, ty)) {
            add_msg(_("The %s shatters!"), thrown.tname().c_str());
        }
        for (item &i : thrown.contents) {
            m.add_item_or_charges(tx, ty, i);
        }
        sound(tx, ty, 16, _("glass breaking!"));
    } else {
        if(m.has_flag("LIQUID", tx, ty)) {
            sound(tx, ty, 10, _("splash!"));
        } else {
            sound(tx, ty, 8, _("thud."));
        }
        m.add_item_or_charges(tx, ty, thrown);
        const trap_id trid = m.tr_at(tx, ty);
        if (trid != tr_null) {
            const struct trap *tr = traplist[trid];
            if (thrown.weight() >= tr->trigger_weight) {
                tr->trigger(NULL, tx, ty);
            }
        }
    }
}

template<typename C>
static char front_or( C container, char default_char  ) {
    if( container.empty() ) {
        return default_char;
    }
    return container.front();
}

// Draws the static portions of the targeting menu,
// returns the number of lines used to draw instructions.
static int draw_targeting_window( WINDOW *w_target, item *relevant, player &p, target_mode mode,
                                  input_context &ctxt )
{
    draw_border(w_target);
    // Draw the "title" of the window.
    mvwprintz(w_target, 0, 2, c_white, "< ");
    if (!relevant) { // currently targetting vehicle to refill with fuel
        wprintz(w_target, c_red, _("Select a vehicle"));
    } else {
        if( mode == TARGET_MODE_FIRE ) {
            if(relevant->has_flag("RELOAD_AND_SHOOT")) {
                wprintz(w_target, c_red, _("Shooting %s from %s"),
                        p.weapon.get_curammo()->nname(1).c_str(), p.weapon.tname().c_str());
            } else if( relevant->has_flag("NO_AMMO") ) {
                wprintz(w_target, c_red, _("Firing %s"), p.weapon.tname().c_str());
            } else {
                wprintz(w_target, c_red, _("Firing ") );
                p.print_gun_mode( w_target, c_red );
                wprintz(w_target, c_red, "%s", " ");
                p.print_recoil( w_target );
            }
        } else if( mode == TARGET_MODE_THROW ) {
            wprintz(w_target, c_red, _("Throwing %s"), relevant->tname().c_str());
        } else {
            wprintz(w_target, c_red, _("Setting target for %s"), relevant->tname().c_str());
        }
    }
    wprintz(w_target, c_white, " >");

    // Draw the help contents at the bottom of the window, leaving room for monster description
    // and aiming status to be drawn dynamically.
    // The - 2 accounts for the window border.
    int text_y = getmaxy(w_target) - 2;
    if (is_mouse_enabled()) {
        // Reserve a line for mouse instructions.
        --text_y;
    }
    if( relevant ) {
        if( mode == TARGET_MODE_FIRE ) {
            // Reserve lines for aiming and firing instructions.
            text_y -= 6;
        } else {
            text_y -= 2;
        }
    }
    // The -1 is the -2 from above, but adjustted since this is a total, not an index.
    int lines_used = getmaxy(w_target) - 1 - text_y;
    mvwprintz(w_target, text_y++, 1, c_white, _("Move cursor to target with directional keys"));
    if( relevant ) {
        mvwprintz( w_target, text_y++, 1, c_white, _("%c %c Cycle targets; %c to fire."),
                   front_or( ctxt.keys_bound_to("PREV_TARGET"), ' ' ),
                   front_or( ctxt.keys_bound_to("NEXT_TARGET"), ' ' ),
                   front_or( ctxt.keys_bound_to("FIRE"), ' ' ) );
        mvwprintz( w_target, text_y++, 1, c_white, _("%c target self; %c toggle snap-to-target"),
                   front_or( ctxt.keys_bound_to("CENTER"), ' ' ),
                   front_or( ctxt.keys_bound_to("TOGGLE_SNAP_TO_TARGET"), ' ' ) );
        if( mode == TARGET_MODE_FIRE ) {
            mvwprintz( w_target, text_y++, 1, c_white, _("%c to steady your aim."),
                       front_or( ctxt.keys_bound_to("AIM"), ' ' ) );
            mvwprintz( w_target, text_y++, 1, c_white, _("%c to aim and fire."),
                       front_or( ctxt.keys_bound_to("AIMED_SHOT"), ' ' ) );
            mvwprintz( w_target, text_y++, 1, c_white, _("%c to take careful aim and fire."),
                       front_or( ctxt.keys_bound_to("CAREFUL_SHOT"), ' ' ) );
            mvwprintz( w_target, text_y++, 1, c_white, _("%c to take precise aim and fire."),
                       front_or( ctxt.keys_bound_to("PRECISE_SHOT"), ' ' ) );
        }
    }

    if( is_mouse_enabled() ) {
        mvwprintz(w_target, text_y++, 1, c_white,
                  _("Mouse: LMB: Target, Wheel: Cycle, RMB: Fire"));
    }
    return lines_used;
}

static int find_target( std::vector <Creature *> &t, int x, int y ) {
    int target = -1;
    for( int i = 0; i < (int)t.size(); i++ ) {
        if( t[i]->xpos() == x && t[i]->ypos() == y ) {
            target = i;
            break;
        }
    }
    return target;
}

static void do_aim( player *p, std::vector <Creature *> &t, int &target,
                    item *relevant, const int x, const int y )
{
    // If we've changed targets, reset aim, unless it's above the minimum.
    if( t[target]->xpos() != x || t[target]->ypos() != y ) {
        target = find_target( t, x, y );
        // TODO: find radial offset between targets and
        // spend move points swinging the gun around.
        p->recoil = std::max(MIN_RECOIL, p->recoil);
    }
    const int aim_amount = p->aim_per_time( relevant );
    if( aim_amount > 0 ) {
        // Increase aim at the cost of moves
        p->moves -= 10;
        p->recoil -= aim_amount;
        p->recoil = std::max( 0, p->recoil );
    } else {
        // If aim is already maxed, we're just waiting, so pass the turn.
        p->moves = 0;
    }
}

// TODO: Shunt redundant drawing code elsewhere
std::vector<point> game::target(int &x, int &y, int lowx, int lowy, int hix,
                                int hiy, std::vector <Creature *> t, int &target,
                                item *relevant, target_mode mode, point from)
{
    std::vector<point> ret;
    int tarx, tary, junk, tart;
    if( from.x == -1 && from.y == -1 ) {
        from = u.pos();
    }
    int range = ( hix - from.x );
    // First, decide on a target among the monsters, if there are any in range
    if (!t.empty()) {
        if( static_cast<size_t>( target ) >= t.size() ) {
            target = 0;
        }
        x = t[target]->xpos();
        y = t[target]->ypos();
    } else {
        target = -1; // No monsters in range, don't use target, reset to -1
    }

    bool sideStyle = use_narrow_sidebar();
    int height = 25;
    int width  = getmaxx(w_messages);
    // Overlap the player info window.
    int top    = -1 + (sideStyle ? getbegy(w_messages) : (getbegy(w_minimap) + getmaxy(w_minimap)) );
    int left   = getbegx(w_messages);

    // Keeping the target menu window around between invocations,
    // it only gets reset if we actually exit the menu.
    static WINDOW *w_target = nullptr;
    if( w_target == nullptr ) {
        w_target = newwin(height, width, top, left);
    }

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
    if( mode == TARGET_MODE_FIRE ) {
        ctxt.register_action("AIM");
        ctxt.register_action("AIMED_SHOT");
        ctxt.register_action("CAREFUL_SHOT");
        ctxt.register_action("PRECISE_SHOT");
    }
    ctxt.register_action("CENTER");
    ctxt.register_action("TOGGLE_SNAP_TO_TARGET");
    ctxt.register_action("HELP_KEYBINDINGS");
    ctxt.register_action("QUIT");

    int num_instruction_lines = draw_targeting_window( w_target, relevant, u, mode, ctxt );

    bool snap_to_target = OPTIONS["SNAP_TO_TARGET"];

    std::string enemiesmsg;
    if (t.empty()) {
        enemiesmsg = _("No targets in range.");
    } else {
        enemiesmsg = string_format(ngettext("%d target in range.", "%d targets in range.",
                                            t.size()), t.size());
    }

    do {
        if (m.sees(from.x, from.y, x, y, -1, tart)) {
            ret = line_to(from.x, from.y, x, y, tart);
        } else {
            ret = line_to(from.x, from.y, x, y, 0);
        }

        // This chunk of code handles shifting the aim point around
        // at maximum range when using circular distance.
        if(trigdist && trig_dist(from.x, from.y, x, y) > range) {
            bool cont = true;
            int cx = x;
            int cy = y;
            for (size_t i = 0; i < ret.size() && cont; i++) {
                if(trig_dist(from.x, from.y, ret[i].x, ret[i].y) > range) {
                    ret.resize(i);
                    cont = false;
                } else {
                    cx = 0 + ret[i].x;
                    cy = 0 + ret[i].y;
                }
            }
            x = cx;
            y = cy;
        }
        point center;
        if (snap_to_target) {
            center = point(x, y);
        } else {
            center = point(u.posx + u.view_offset_x, u.posy + u.view_offset_y);
        }
        // Clear the target window.
        for (int i = 1; i <= getmaxy(w_target) - num_instruction_lines - 2; i++) {
            // Clear width excluding borders.
            for (int j = 1; j <= getmaxx(w_target) - 2; j++) {
                mvwputch(w_target, i, j, c_white, ' ');
            }
        }
        /* Start drawing w_terrain things -- possibly move out to centralized
           draw_terrain_window function as they all should be roughly similar */
        m.build_map_cache(); // part of the SDLTILES drawing code
        m.draw(w_terrain, center); // embedded in SDL drawing code
        // Draw the Monsters
        for (size_t i = 0; i < num_zombies(); i++) {
            draw_critter( zombie( i ), center );
        }
        // Draw the NPCs
        for (auto &i : active_npc) {
            draw_critter( *i, center );
        }
        // Draw the player
        draw_critter( g->u, center );
        int line_number = 1;
        if (x != from.x || y != from.y) {
            // Only draw a highlighted trajectory if we can see the endpoint.
            // Provides feedback to the player, and avoids leaking information
            // about tiles they can't see.
            draw_line(x, y, center, ret);

            // Print to target window
            if (!relevant) {
                // currently targetting vehicle to refill with fuel
                vehicle *veh = m.veh_at(x, y);
                if( veh != nullptr && u.sees( x, y ) ) {
                    mvwprintw(w_target, line_number++, 1, _("There is a %s"),
                              veh->name.c_str());
                }
            } else if (relevant == &u.weapon && relevant->is_gun()) {
                // firing a gun
                mvwprintw(w_target, line_number, 1, _("Range: %d/%d, %s"),
                          rl_dist(from.x, from.y, x, y), range, enemiesmsg.c_str());
                // get the current weapon mode or mods
                std::string mode = "";
                if (u.weapon.get_gun_mode() == "MODE_BURST") {
                    mode = _("Burst");
                } else {
                    item *gunmod = u.weapon.active_gunmod();
                    if (gunmod != NULL) {
                        mode = gunmod->type_name();
                    }
                }
                if (mode != "") {
                    mvwprintw(w_target, line_number, 14, _("Firing mode: %s"),
                              mode.c_str());
                }
                line_number++;
            } else {
                // throwing something or setting turret's target
                mvwprintw(w_target, line_number++, 1, _("Range: %d/%d, %s"),
                          rl_dist(from.x, from.y, x, y), range, enemiesmsg.c_str());
            }

            const Creature *critter = critter_at( x, y );
            if( critter != nullptr && u.sees( critter ) ) {
                // The 4 is 2 for the border and 2 for aim bars.
                int available_lines = height - num_instruction_lines - line_number - 4;
                line_number = critter->print_info( w_target, line_number, available_lines, 1);
            } else {
                mvwputch(w_terrain, POSY + y - center.y, POSX + x - center.x, c_red, '*');
            }
        } else {
            mvwprintw(w_target, line_number++, 1, _("Range: %d, %s"), range, enemiesmsg.c_str());
        }

        if( mode == TARGET_MODE_FIRE && critter_at( x, y ) ) {
            line_number = u.print_aim_bars( w_target, line_number, relevant, critter_at( x, y ) );
        }

        wrefresh(w_target);
        wrefresh(w_terrain);
        refresh();

        std::string action;
        if( u.activity.type == ACT_AIM && u.activity.str_values[0] != "AIM" ) {
            // If we're in 'aim and shoot' mode,
            // skip retrieving input and go straight to the action.
            action = u.activity.str_values[0];
        } else {
            action = ctxt.handle_input();
        }
        // Clear the activity if any, we'll re-set it later if we need to.
        u.cancel_activity();

        tarx = 0;
        tary = 0;
        // Our coordinates will either be determined by coordinate input(mouse),
        // by a direction key, or by the previous value.
        if (action == "SELECT" && ctxt.get_coordinates(g->w_terrain, tarx, tary)) {
            if (!OPTIONS["USE_TILES"] && snap_to_target) {
                // Snap to target doesn't currently work with tiles.
                tarx += x - from.x;
                tary += y - from.y;
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
            const Creature *critter = critter_at( x, y );
            if( critter != nullptr ) {
                draw_critter( *critter, center );
            } else if (m.sees(u.posx, u.posy, x, y, -1, junk)) {
                m.drawsq(w_terrain, u, x, y, false, true, center.x, center.y);
            } else {
                mvwputch(w_terrain, POSY, POSX, c_black, 'X');
            }
            x += tarx;
            y += tary;
            if (x < lowx) {
                x = lowx;
            } else if (x > hix) {
                x = hix;
            }
            if (y < lowy) {
                y = lowy;
            } else if (y > hiy) {
                y = hiy;
            }
        } else if ((action == "PREV_TARGET") && (target != -1)) {
            int newtarget = find_target( t, x, y ) - 1;
            if( newtarget == -1 ) {
                newtarget = t.size() - 1;
            }
            x = t[newtarget]->xpos();
            y = t[newtarget]->ypos();
        } else if ((action == "NEXT_TARGET") && (target != -1)) {
            int newtarget = find_target( t, x, y ) + 1;
            if( newtarget == (int)t.size() ) {
                newtarget = 0;
            }
            x = t[newtarget]->xpos();
            y = t[newtarget]->ypos();
        } else if ((action == "AIM") && target != -1) {
            do_aim( &u, t, target, relevant, x, y );
            if(u.moves <= 0) {
                // We've run out of moves, clear target vector, but leave target selected.
                u.assign_activity( ACT_AIM, 0, 0 );
                u.activity.str_values.push_back( "AIM" );
                ret.clear();
                return ret;
            }
        } else if( (action == "AIMED_SHOT" || action == "CAREFUL_SHOT" || action == "PRECISE_SHOT") &&
                   target != -1 ) {
            int aim_threshold = 20;
            if( action == "CAREFUL_SHOT" ) {
                aim_threshold = 10;
            } else if( action == "PRECISE_SHOT" ) {
                aim_threshold = 0;
            }
            do {
                do_aim( &u, t, target, relevant, x, y );
            } while( u.moves > 0 && u.recoil > aim_threshold &&
                     u.recoil - u.weapon.sight_dispersion( -1 ) > 0 );
            if( u.recoil <= aim_threshold ||
                u.recoil - u.weapon.sight_dispersion( -1 ) == 0) {
                // If we made it under the aim threshold, go ahead and fire.
                // Also fire if we're at our best aim level already.
                werase( w_target );
                wrefresh( w_target );
                delwin( w_target );
                w_target = nullptr;
                return ret;
            } else {
                // We've run out of moves, set the activity to aim so we'll
                // automatically re-enter the targeting menu next turn.
                // Set the string value of the aim action to the right thing
                // so we re-enter this loop.
                // Also clear target vector, but leave target selected.
                u.assign_activity( ACT_AIM, 0, 0 );
                u.activity.str_values.push_back( action );
                ret.clear();
                return ret;
            }
        } else if (action == "FIRE") {
            target = find_target( t, x, y );
            if (from.x == x && from.y == y) {
                ret.clear();
            }
            break;
        } else if (action == "CENTER") {
            x = from.x;
            y = from.y;
            ret.clear();
        } else if (action == "TOGGLE_SNAP_TO_TARGET") {
            snap_to_target = !snap_to_target;
        } else if (action == "QUIT") { // return empty vector (cancel)
            ret.clear();
            target = -1;
            break;
        }
    } while (true);

    werase( w_target );
    wrefresh( w_target );
    delwin( w_target );
    w_target = nullptr;
    return ret;
}

int time_to_fire(player &p, const itype &firingt)
{
    const islot_gun *firing = firingt.gun.get();
    int time = 0;
    if (firing->skill_used == Skill::skill("pistol")) {
        if (p.skillLevel("pistol") > 6) {
            time = 10;
        } else {
            time = (80 - 10 * p.skillLevel("pistol"));
        }
    } else if (firing->skill_used == Skill::skill("shotgun")) {
        if (p.skillLevel("shotgun") > 3) {
            time = 70;
        } else {
            time = (150 - 25 * p.skillLevel("shotgun"));
        }
    } else if (firing->skill_used == Skill::skill("smg")) {
        if (p.skillLevel("smg") > 5) {
            time = 20;
        } else {
            time = (80 - 10 * p.skillLevel("smg"));
        }
    } else if (firing->skill_used == Skill::skill("rifle")) {
        if (p.skillLevel("rifle") > 8) {
            time = 30;
        } else {
            time = (150 - 15 * p.skillLevel("rifle"));
        }
    } else if (firing->skill_used == Skill::skill("archery")) {
        if (p.skillLevel("archery") > 8) {
            time = 20;
        } else {
            time = (220 - 25 * p.skillLevel("archery"));
        }
    } else if (firing->skill_used == Skill::skill("throw")) {
        if (p.skillLevel("throw") > 6) {
            time = 50;
        } else {
            time = (220 - 25 * p.skillLevel("throw"));
        }
    } else if (firing->skill_used == Skill::skill("launcher")) {
        if (p.skillLevel("launcher") > 8) {
            time = 30;
        } else {
            time = (200 - 20 * p.skillLevel("launcher"));
        }
    } else if(firing->skill_used == Skill::skill("melee")) { // right now, just whips
        if (p.skillLevel("melee") > 8) {
            time = 50;
        } else {
            time = (200 - (20 * p.skillLevel("melee")));
        }
    } else {
        debugmsg("Why is shooting %s using %s skill?", firingt.nname(1).c_str(),
                 firing->skill_used->name().c_str());
        time =  0;
    }

    return time;
}

void make_gun_sound_effect(player &p, bool burst, item *weapon)
{
    std::string gunsound;
    int noise = p.weapon.noise();
    const auto &ammo_used = weapon->type->gun->ammo;
    const auto &ammo_effects = weapon->type->gun->ammo_effects;
    const auto &weapon_id = weapon->typeId();

    if( ammo_effects.count("LASER") || ammo_effects.count("PLASMA") ) {
        if (noise < 20) {
            gunsound = _("Fzzt!");
        } else if (noise < 40) {
            gunsound = _("Pew!");
        } else if (noise < 60) {
            gunsound = _("Tsewww!");
        } else {
            gunsound = _("Kra-kow!!");
        }
    } else if( ammo_effects.count("LIGHTNING") ) {
        if (noise < 20) {
            gunsound = _("Bzzt!");
        } else if (noise < 40) {
            gunsound = _("Bzap!");
        } else if (noise < 60) {
            gunsound = _("Bzaapp!");
        } else {
            gunsound = _("Kra-koom!!");
        }
    } else if( ammo_effects.count("WHIP") ) {
        noise = 20;
        gunsound = _("Crack!");
    } else {
        if (noise < 5) {
            if (burst) {
                gunsound = _("Brrrip!");
            } else {
                gunsound = _("plink!");
            }
        } else if (noise < 25) {
            if (burst) {
                gunsound = _("Brrrap!");
            } else {
                gunsound = _("bang!");
            }
        } else if (noise < 60) {
            if (burst) {
                gunsound = _("P-p-p-pow!");
            } else {
                gunsound = _("blam!");
            }
        } else {
            if (burst) {
                gunsound = _("Kaboom!!");
            } else {
                gunsound = _("kerblam!");
            }
        }
    }

    if( ammo_used == "40mm") {
        g->sound(p.posx, p.posy, 8, _("Thunk!"));
    } else if( weapon_id == "hk_g80") {
        g->sound(p.posx, p.posy, 24, _("tz-CRACKck!"));
    } else if( ammo_used == "gasoline" || ammo_used == "66mm" ||
               ammo_used == "84x246mm" || ammo_used == "m235" ) {
        g->sound(p.posx, p.posy, 4, _("Fwoosh!"));
    } else if( ammo_used != "bolt" && ammo_used != "arrow" && ammo_used != "pebble" &&
               ammo_used != "fishspear" && ammo_used != "dart" ) {
        g->sound(p.posx, p.posy, noise, gunsound);
    }
}

// Little helper to clean up dispersion calculation methods.
static int rand_or_max( bool random, int max )
{
    return random ? rng(0, max) : max;
}

int player::skill_dispersion( item *weapon, bool random ) const
{
    const std::string skill_used = weapon->gun_skill();
    const int weapon_skill_level = get_skill_level(skill_used);
    int dispersion = 0; // Measured in Minutes of Arc.
    // Up to 0.75 degrees for each skill point < 10.
    if (weapon_skill_level < 10) {
        dispersion += rand_or_max( random, 45 * (10 - weapon_skill_level) );
    }
    // Up to 0.25 deg per each skill point < 10.
    if( get_skill_level("gun") < 10) {
        dispersion += rand_or_max( random, 15 * (10 - get_skill_level("gun")) );
    }
    return dispersion;
}
// utility functions for projectile_attack
double player::get_weapon_dispersion(item *weapon, bool random) const
{
    if( weapon->is_gun() && weapon->is_in_auxiliary_mode() ) {
        const auto gunmod = weapon->active_gunmod();
        if( gunmod != nullptr ) {
            return get_weapon_dispersion( gunmod, random );
        }
    }

    double dispersion = 0.; // Measured in quarter-degrees.
    dispersion += skill_dispersion( weapon, random );

    dispersion += rand_or_max( random, ranged_dex_mod() );
    dispersion += rand_or_max( random, ranged_per_mod() );

    dispersion += rand_or_max( random, 30 * (encumb(bp_arm_l) + encumb(bp_arm_r)) );
    dispersion += rand_or_max( random, 60 * encumb(bp_eyes) );

    if( weapon->has_curammo() ) {
        dispersion += rand_or_max( random, weapon->get_curammo()->ammo->dispersion);
    }

    dispersion += rand_or_max( random, weapon->gun_dispersion() );
    if( random ) {
        int adj_recoil = recoil + driving_recoil;
        dispersion += rng( int(adj_recoil / 4), adj_recoil );
    }

    if (has_bionic("bio_targeting")) {
        dispersion *= 0.75;
    }
    if ((is_underwater() && !weapon->has_flag("UNDERWATER_GUN")) ||
        // Range is effectively four times longer when shooting unflagged guns underwater.
        (!is_underwater() && weapon->has_flag("UNDERWATER_GUN"))) {
        // Range is effectively four times longer when shooting flagged guns out of water.
        dispersion *= 4;
    }

    if (dispersion < 0) {
        return 0;
    }
    return dispersion;
}

int recoil_add(player &p, const item &gun)
{
    int ret = gun.gun_recoil();
    ret -= rng(p.str_cur * 7, p.str_cur * 15);
    ret -= rng(0, p.get_skill_level(gun.gun_skill()) * 7);
    if (ret > 0) {
        return ret;
    }
    return 0;
}

void splatter( std::vector<point> trajectory, int dam, Creature *target )
{
    if( dam <= 0) {
        return;
    }
    if( !target->is_npc() && !target->is_player() ) {
        //Check if the creature isn't an NPC or the player (so the cast works)
        monster *mon = dynamic_cast<monster *>(target);
        if (mon->is_hallucination() || mon->get_material() != "flesh" ||
            mon->has_flag( MF_VERMIN)) {
            // If it is a hallucanation, not made of flesh, or a vermin creature,
            // don't splatter the blood.
            return;
        }
    }
    field_id blood = fd_blood;
    if( target != NULL ) {
        blood = target->bloodType();
    }
    if (blood == fd_null) { //If there is no blood to splatter, return.
        return;
    }

    int distance = 1;
    if( dam > 50 ) {
        distance = 3;
    } else if( dam > 20 ) {
        distance = 2;
    }

    std::vector<point> spurt = continue_line( trajectory, distance );

    for( auto &elem : spurt ) {
        int tarx = elem.x;
        int tary = elem.y;
        g->m.adjust_field_strength( point(tarx, tary), blood, 1 );
        if( g->m.move_cost(tarx, tary) == 0 ) {
            // Blood splatters stop at walls.
            break;
        }
    }
}
