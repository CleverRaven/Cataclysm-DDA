#include <vector>
#include <string>
#include <cmath>
#include "game.h"
#include "map.h"
#include "debug.h"
#include "output.h"
#include "line.h"
#include "skill.h"
#include "rng.h"
#include "item.h"
#include "options.h"
#include "action.h"
#include "input.h"
#include "messages.h"
#include "sounds.h"
#include "translations.h"
#include "monster.h"
#include "npc.h"
#include "trap.h"
#include "itype.h"
#include "vehicle.h"
#include "field.h"
#include "mtype.h"

const skill_id skill_pistol( "pistol" );
const skill_id skill_rifle( "rifle" );
const skill_id skill_smg( "smg" );
const skill_id skill_shotgun( "shotgun" );
const skill_id skill_launcher( "launcher" );
const skill_id skill_archery( "archery" );
const skill_id skill_throw( "throw" );
const skill_id skill_gun( "gun" );
const skill_id skill_melee( "melee" );
const skill_id skill_driving( "driving" );

const efftype_id effect_on_roof( "on_roof" );
const efftype_id effect_bounced( "bounced" );

static projectile make_gun_projectile( const item &gun );
int time_to_fire(player &p, const itype &firing);
static inline void eject_casing( player& p, item& weap );
int recoil_add( player& p, const item& gun, int shot );
void make_gun_sound_effect(player &p, bool burst, item *weapon);
extern bool is_valid_in_w_terrain(int, int);
void drop_or_embed_projectile( const dealt_projectile_attack &attack );

void splatter( const std::vector<tripoint> &trajectory, int dam, const Creature *target = nullptr );

struct aim_type {
    std::string name;
    std::string action;
    std::string help;
    bool has_threshold;
    int threshold;
};

dealt_projectile_attack Creature::projectile_attack( const projectile &proj, const tripoint &target,
                                                     double shot_dispersion )
{
    return projectile_attack( proj, pos(), target, shot_dispersion );
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

int ranged_skill_offset( const skill_id &skill )
{
    if( skill == skill_pistol ) {
        return 0;
    } else if( skill == skill_rifle ) {
        return 0;
    } else if( skill == skill_smg ) {
        return 0;
    } else if( skill == skill_shotgun ) {
        return 0;
    } else if( skill == skill_launcher ) {
        return 0;
    } else if( skill == skill_archery ) {
        return 135;
    } else if( skill == skill_throw ) {
        return 195;
    }
    return 0;
}

dealt_projectile_attack Creature::projectile_attack( const projectile &proj_arg, const tripoint &source,
                                                     const tripoint &target_arg, double shot_dispersion )
{
    const bool do_animation = OPTIONS["ANIMATIONS"];

    // shot_dispersion is in MOA - 1/60 of a degree
    constexpr double moa = M_PI / 180 / 60;
    // We're approximating the tangent. Multiplying angle*range by ~0.745 does that (kinda).
    constexpr double to_tangent = 0.745;

    double range = rl_dist( source, target_arg );
    double missed_by = shot_dispersion * moa * to_tangent * range;
    // TODO: move to-hit roll back in here

    dealt_projectile_attack attack {
        proj_arg, nullptr, dealt_damage_instance(), source, missed_by
    };

    projectile &proj = attack.proj;
    const auto &proj_effects = proj.proj_effects;

    const bool stream = proj_effects.count("STREAM") > 0 ||
                        proj_effects.count("STREAM_BIG") > 0 ||
                        proj_effects.count("JET") > 0;
    const bool no_item_damage = proj_effects.count( "NO_ITEM_DAMAGE" ) > 0;
    const bool do_draw_line = proj_effects.count( "DRAW_AS_LINE" ) > 0;
    const bool null_source = proj_effects.count( "NULL_SOURCE" ) > 0;

    // If we were targetting a tile rather than a monster, don't overshoot
    // Unless the target was a wall, then we are aiming high enough to overshoot
    const bool no_overshoot = proj_effects.count( "NO_OVERSHOOT" ) ||
                              ( g->critter_at( target_arg ) == nullptr && g->m.passable( target_arg ) );

    double extend_to_range = no_overshoot ? range : proj_arg.range;

    tripoint target = target_arg;
    std::vector<tripoint> trajectory;
    if( missed_by >= 1.0 ) {
        // We missed enough to target a different tile
        double dx = target_arg.x - source.x;
        double dy = target_arg.y - source.y;
        double rad = atan2( dy, dx );
        // Cap spread at 30 degrees or it gets wild quickly
        double spread = std::min( shot_dispersion / moa, M_PI / 180 * 30 );
        rad += rng_float( -spread, spread );

        // @todo This should also represent the miss on z axis
        const int offset = std::min<int>( range, sqrtf( missed_by ) );
        int new_range = no_overshoot ?
                            range + rng( -offset, offset ) :
                            rng( range - offset, proj_arg.range );
        new_range = std::max( new_range, 1 );

        target.x = source.x + roll_remainder( new_range * cos( rad ) );
        target.y = source.y + roll_remainder( new_range * sin( rad ) );

        // Don't extend range further, miss here can mean hitting the ground near the target
        range = rl_dist( source, target );
        extend_to_range = range;

        // Cap missed_by at 1.0
        missed_by = 1.0;
        sfx::play_variant_sound( "bullet_hit", "hit_wall", sfx::get_heard_volume( target ), sfx::get_heard_angle( target ));
        // TODO: Z dispersion
        int junk = 0;
        // If we missed, just draw a straight line.
        trajectory = line_to( source, target, junk, junk );
    } else {
        // Go around obstacles a little if we're on target.
        trajectory = g->m.find_clear_path( source, target );
    }

    if( proj_effects.count( "MUZZLE_SMOKE" ) ) {
        for( const auto& e : closest_tripoints_first( 1, trajectory.empty() ? source : trajectory[ 0 ] ) ) {
            if( one_in( 2 ) ) {
                g->m.add_field( e, fd_smoke, 1, 0 );
            }
        }
    }

    add_msg( m_debug, "%s proj_atk: shot_dispersion: %.2f",
             disp_name().c_str(), shot_dispersion );
    add_msg( m_debug, "missed_by: %.2f target (orig/hit): %d,%d,%d/%d,%d,%d", missed_by,
             target_arg.x, target_arg.y, target_arg.z,
             target.x, target.y, target.z );

    // Trace the trajectory, doing damage in order
    tripoint &tp = attack.end_point;
    tripoint prev_point = source;

    if( !no_overshoot && range < extend_to_range ) {
        // Continue line is very "stiff" when the original range is short
        // @todo Make it use a more distant point for more realistic extended lines
        std::vector<tripoint> trajectory_extension = continue_line( trajectory,
                                                                    extend_to_range - range );
        trajectory.reserve( trajectory.size() + trajectory_extension.size() );
        trajectory.insert( trajectory.end(), trajectory_extension.begin(), trajectory_extension.end() );
    }
    // Range can be 0
    while( !trajectory.empty() && rl_dist( source, trajectory.back() ) > proj_arg.range ) {
        trajectory.pop_back();
    }

    // If this is a vehicle mounted turret, which vehicle is it mounted on?
    const vehicle *in_veh = has_effect( effect_on_roof ) ?
                            g->m.veh_at( pos() ) : nullptr;

    //Start this now in case we hit something early
    std::vector<tripoint> blood_traj = std::vector<tripoint>();
    const float projectile_skip_multiplier = 0.1;
    // Randomize the skip so that bursts look nicer
    int projectile_skip_calculation = range * projectile_skip_multiplier;
    int projectile_skip_current_frame = rng( 0, projectile_skip_calculation );
    bool has_momentum = true;
    size_t i = 0; // Outside loop, because we want it for line drawing
    for( ; i < trajectory.size() && ( has_momentum || stream ); i++ ) {
        blood_traj.push_back(trajectory[i]);
        prev_point = tp;
        tp = trajectory[i];

        if( ( tp.z > prev_point.z && g->m.has_floor( tp ) ) ||
            ( tp.z < prev_point.z && g->m.has_floor( prev_point ) ) ) {
            // Currently strictly no shooting through floor
            // TODO: Bash the floor
            tp = prev_point;
            i--;
            break;
        }
        // Drawing the bullet uses player u, and not player p, because it's drawn
        // relative to YOUR position, which may not be the gunman's position.
        if( do_animation && !do_draw_line ) {
            // TODO: Make this draw thrown item/launched grenade/arrow
            if( projectile_skip_current_frame >= projectile_skip_calculation ) {
                g->draw_bullet(g->u, tp, (int)i, trajectory, stream ? '#' : '*');
                projectile_skip_current_frame = 0;
                // If we missed recalculate the skip factor so they spread out.
                projectile_skip_calculation = std::max( (size_t)range, i ) * projectile_skip_multiplier;
            } else {
                projectile_skip_current_frame++;
            }
        }

        if( in_veh != nullptr ) {
            int part;
            vehicle *other = g->m.veh_at( tp, part );
            if( in_veh == other && other->is_inside( part ) ) {
                continue; // Turret is on the roof and can't hit anything inside
            }
        }

        Creature *critter = g->critter_at( tp );
        monster *mon = dynamic_cast<monster *>(critter);
        // ignore non-point-blank digging targets (since they are underground)
        if( mon != nullptr && mon->digging() &&
            rl_dist( pos(), tp ) > 1) {
            critter = mon = nullptr;
        }

        // Reset hit critter from the last iteration
        attack.hit_critter = nullptr;

        // If we shot us a monster...
        // TODO: add size effects to accuracy
        // If there's a monster in the path of our bullet, and either our aim was true,
        //  OR it's not the monster we were aiming at and we were lucky enough to hit it
        double cur_missed_by = missed_by;
        // If missed_by is 1.0, the end of the trajectory may not be the original target
        // We missed it too much for the original target to matter, just reroll as unintended
        if( missed_by >= 1.0 || tp != target_arg ) {
            // Unintentional hit
            cur_missed_by = std::max( rng_float( 0.2, 3.0 - missed_by ), 0.4 );
        }

        if( critter != nullptr && cur_missed_by < 1.0 ) {
            if( in_veh != nullptr && g->m.veh_at( tp ) == in_veh && critter->is_player() ) {
                // Turret either was aimed by the player (who is now ducking) and shoots from above
                // Or was just IFFing, giving lots of warnings and time to get out of the line of fire
                continue;
            }
            dealt_damage_instance dealt_dam;
            attack.missed_by = cur_missed_by;
            critter->deal_projectile_attack( null_source ? nullptr : this, attack );
            // Critter can still dodge the projectile
            // In this case hit_critter won't be set
            if( attack.hit_critter != nullptr ) {
                splatter( blood_traj, dealt_dam.total_damage(), critter );
                sfx::do_projectile_hit( *attack.hit_critter );
                has_momentum = false;
            } else {
                attack.missed_by = missed_by;
            }
        } else if( in_veh != nullptr && g->m.veh_at( tp ) == in_veh ) {
            // Don't do anything, especially don't call map::shoot as this would damage the vehicle
        } else {
            g->m.shoot( tp, proj, !no_item_damage && tp == target );
            has_momentum = proj.impact.total_damage() > 0;
        }

        if( !has_momentum && g->m.impassable( tp ) ) {
            // Don't let flamethrowers go through walls
            // TODO: Let them go through bars
            break;
        }
    } // Done with the trajectory!

    if( do_animation && do_draw_line && i > 0 ) {
        trajectory.resize( i );
        g->draw_line( tp, trajectory );
        g->draw_bullet( g->u, tp, (int)i, trajectory, stream ? '#' : '*' );
    }

    if( g->m.impassable(tp) ) {
        tp = prev_point;
    }

    drop_or_embed_projectile( attack );

    apply_ammo_effects( tp, proj.proj_effects );
    const auto &expl = proj.get_custom_explosion();
    if( expl.power > 0.0f ) {
        g->explosion( tp, proj.get_custom_explosion() );
    }

    // TODO: Move this outside now that we have hit point in return values?
    if( proj.proj_effects.count( "BOUNCE" ) ) {
        for( size_t i = 0; i < g->num_zombies(); i++ ) {
            monster &z = g->zombie(i);
            if( z.is_dead() ) {
                continue;
            }
            // search for monsters in radius 4 around impact site
            if( rl_dist( z.pos(), tp ) <= 4 &&
                g->m.sees( z.pos(), tp, -1 ) ) {
                // don't hit targets that have already been hit
                if( !z.has_effect( effect_bounced ) ) {
                    add_msg(_("The attack bounced to %s!"), z.name().c_str());
                    z.add_effect( effect_bounced, 1 );
                    projectile_attack( proj, tp, z.pos(), shot_dispersion );
                    sfx::play_variant_sound( "fire_gun", "bio_lightning_tail", sfx::get_heard_volume(z.pos()), sfx::get_heard_angle(z.pos()));
                    break;
                }
            }
        }
    }

    return attack;
}

bool player::handle_gun_damage( const itype &firingt, const std::set<std::string> &curammo_effects )
{
    const islot_gun *firing = firingt.gun.get();
    // Here we check if we're underwater and whether we should misfire.
    // As a result this causes no damage to the firearm, note that some guns are waterproof
    // and so are immune to this effect, note also that WATERPROOF_GUN status does not
    // mean the gun will actually be accurate underwater.
    if (firing->skill_used != skill_archery &&
        firing->skill_used != skill_throw ) {
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
            if( weapon.damage < MAX_ITEM_DAMAGE && one_in( 4 * firing->durability ) ) {
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
            if( weapon.damage < MAX_ITEM_DAMAGE && one_in( firing->durability ) ) {
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

int player::fire_gun( const tripoint &target, int shots )
{
    return fire_gun( target, shots, weapon );
}

int player::fire_gun( const tripoint &target, int shots, item& gun )
{
    if( !gun.is_gun() ) {
        debugmsg( "%s tried to fire non-gun (%s).", name.c_str(), gun.tname().c_str() );
        return 0;
    }

    // Number of shots to fire is limited by the ammount of remaining ammo
    if( gun.ammo_required() ) {
        shots = std::min( shots, int( gun.ammo_remaining() / gun.ammo_required() ) );
    }

    // cap our maximum burst size by the amount of UPS power left
    if( gun.get_gun_ups_drain() > 0 ) {
        if( !worn.empty() && worn.back().type->id == "fake_UPS" ) {
            shots = std::min( shots, int( worn.back().charges / gun.get_gun_ups_drain() ) );
        } else {
            shots = std::min( shots, int( charges_of( "UPS" ) / gun.get_gun_ups_drain() ) );
        }
    }

    if( shots <= 0 ) {
        debugmsg( "Attempted to fire zero or negative shots using %s", gun.tname().c_str() );
    }

    const skill_id skill_used = gun.gun_skill();

    const int player_dispersion = skill_dispersion( gun, false ) + ranged_skill_offset( skill_used );
    // If weapon dispersion exceeds skill dispersion you can't tell
    // if you need to correct or if the gun messed up, so you can't learn.
    ///\EFFECT_PER allows you to learn more often with less accurate weapons.
    const bool train_skill = gun.gun_dispersion() < player_dispersion + 15 * rng( 0, get_per() );
    if( train_skill ) {
        practice( skill_used, 8 + 2 * shots );
    } else if( one_in( 30 ) ) {
        add_msg_if_player(m_info, _("You'll need a more accurate gun to keep improving your aim."));
    }

    tripoint aim = target;
    int curshot = 0;
    int burst = 0; // count of shots against current target
    for( ; curshot != shots; ++curshot ) {


        if( !handle_gun_damage( *gun.type, gun.ammo_effects() ) ) {
            break;
        }

        double dispersion = get_weapon_dispersion( &gun, true );
        int range = rl_dist( pos(), aim );

        // Apply penalty when using bulky weapons at point-blank range (except when loaded with shot)
        // If we are firing an auxiliary gunmod we wan't to use the base guns volume (which includes the gunmod itself)
        if( gun.ammo_type() != "shot" ) {
            const item *parent = gun.is_auxiliary_gunmod() && has_item( gun ) ? find_parent( gun ) : nullptr;
            dispersion *= std::max( ( ( parent ? parent->volume() : gun.volume() ) / 3.0 ) / range, 1.0 );
        }

        auto shot = projectile_attack( make_gun_projectile( gun ), aim, dispersion );

        // if we are firing a turret don't apply that recoil to the player
        // @todo turrets need to accumulate recoil themselves
        recoil_add( *this, gun, ++burst );

        make_gun_sound_effect( *this, shots > 1, &gun );
        sfx::generate_gun_sound( *this, gun );

        eject_casing( *this, gun );

        if( gun.has_flag( "BIO_WEAPON" ) ) {
            // Consume a (virtual) charge to let player::activate_bionic know the weapon has been fired.
            gun.charges--;
        } else {
            if( gun.ammo_consume( gun.ammo_required(), pos() ) != gun.ammo_required() ) {
                debugmsg( "Unexpected shortage of ammo whilst firing %s", gun.tname().c_str() );
                break;
            }
        }

        if ( !worn.empty() && worn.back().type->id == "fake_UPS" ) {
            use_charges( "fake_UPS", gun.get_gun_ups_drain() );
        } else {
            use_charges( "UPS", gun.get_gun_ups_drain() );
        }

        // Experience gain is limited by range and penalised proportional to inaccuracy.
        int exp = std::min( range, 3 * ( get_skill_level( skill_used ) + 1 ) ) * 20;
        int penalty = std::max( int( sqrt( shot.missed_by * 36 ) ), 1 );

        // Even if we are not training we practice the skill to prevent rust.
        practice( skill_used, train_skill ? exp / penalty : 0 );

        if( shot.missed_by <= .1 ) {
            lifetime_stats()->headshots++; // @todo check head existence for headshot
        }

        // If burst firing and we killed the target (or were shooting into empty space) then try to retarget
        const auto critter = g->critter_at( aim, true );
        if( !critter || critter->is_dead_state() ) {

            // Find suitable targets that are in range, hostile and near any previous target
            auto hostiles = get_targetable_creatures( gun.gun_range( this ) );

            hostiles.erase( std::remove_if( hostiles.begin(), hostiles.end(), [&]( const Creature *z ) {
                if( rl_dist( z->pos(), aim ) > get_skill_level( skill_gun ) ) {
                    return true; ///\EFFECT_GUN increases range of automatic retargeting during burst fire

                } else if( z->is_dead_state() ) {
                    return true;

                } else if( has_trait( "TRIGGERHAPPY") && one_in( 10 ) ) {
                    return false; // Trigger happy sometimes doesn't care who we shoot

                } else {
                    return attitude_to( *z ) != A_HOSTILE;
                }
            } ), hostiles.end() );

            if( hostiles.empty() || hostiles.front()->is_dead_state() ) {
                break; // We ran out of suitable targets

            } else if( !one_in( 7 - get_skill_level( skill_gun ) ) ) {
                break; ///\EFFECT_GUN increases chance of firing multiple times in a burst
            }
            aim = random_entry( hostiles )->pos();
            burst = 0;
        }
    }

    practice( skill_gun, train_skill ? 15 : 0 );

    // Use different amounts of time depending on the type of gun and our skill
    moves -= time_to_fire( *this, *gun.type );

    return curshot;
}

dealt_projectile_attack player::throw_item( const tripoint &target, const item &to_throw )
{
    // Copy the item, we may alter it before throwing
    item thrown = to_throw;

    // Base move cost on moves per turn of the weapon
    // and our skill.
    int move_cost = thrown.attack_time() / 2;
    ///\EFFECT_THROW speeds up throwing items
    int skill_cost = (int)(move_cost / (std::pow(get_skill_level( skill_throw ), 3.0f) / 400.0 + 1.0));
    ///\EFFECT_DEX speeds up throwing items
    const int dexbonus = (int)(std::pow(std::max(dex_cur - 8, 0), 0.8) * 3.0);

    move_cost += skill_cost;
    move_cost += 2 * encumb(bp_torso);
    move_cost -= dexbonus;

    if( has_trait("LIGHT_BONES") ) {
        move_cost *= .9;
    }
    if( has_trait("HOLLOW_BONES") ) {
        move_cost *= .8;
    }

    if( move_cost < 25 ) {
        move_cost = 25;
    }

    moves -= move_cost;

    const int stamina_cost = ( (thrown.weight() / 100 ) + 20) * -1;
    mod_stat("stamina", stamina_cost);

    tripoint targ = target;
    int deviation = 0;

    const skill_id &skill_used = skill_throw;
    // Throwing attempts below "Basic Competency" level are extra-bad
    int skill_level = get_skill_level( skill_throw );

    ///\EFFECT_THROW <8 randomly increases throwing deviation
    if( skill_level < 3 ) {
        deviation += rng(0, 8 - skill_level);
    }

    if( skill_level < 8 ) {
        deviation += rng(0, 8 - skill_level);
    ///\EFFECT_THROW >7 decreases throwing deviation
    } else {
        deviation -= skill_level - 6;
    }

    deviation += throw_dex_mod();

    ///\EFFECT_PER <6 randomly increases throwing deviation
    if (per_cur < 6) {
        deviation += rng(0, 8 - per_cur);
    ///\EFFECT_PER >8 decreases throwing deviation
    } else if (per_cur > 8) {
        deviation -= per_cur - 8;
    }

    deviation += rng(0, ((encumb(bp_hand_l) + encumb(bp_hand_r)) + encumb(bp_eyes) + 1) / 10);
    if (thrown.volume() > 5) {
        deviation += rng(0, 1 + (thrown.volume() - 5) / 4);
    }
    if (thrown.volume() == 0) {
        deviation += rng(0, 3);
    }

    ///\EFFECT_STR randomly decreases throwing deviation
    deviation += rng(0, std::max( 0, thrown.weight() / 113 - str_cur ) );
    deviation = std::max( 0, deviation );

    // Rescaling to use the same units as projectile_attack
    const double shot_dispersion = deviation * (.01 / 0.00021666666666666666);
    static const std::vector<material_id> ferric = { material_id( "iron" ), material_id( "steel" ) };

    bool do_railgun = has_active_bionic("bio_railgun") &&
                      thrown.made_of_any( ferric );

    // The damage dealt due to item's weight and player's strength
    ///\EFFECT_STR increases throwing damage
    int real_dam = ( (thrown.weight() / 452)
                     + (thrown.type->melee_dam / 2)
                     + (str_cur / 2) )
                   / (2.0 + (thrown.volume() / 4.0));
    if( real_dam > thrown.weight() / 40 ) {
        real_dam = thrown.weight() / 40;
    }
    if( real_dam < 1 ) {
        // Need at least 1 dmg or projectile attack will stop due to no momentum
        real_dam = 1;
    }
    if( do_railgun ) {
        real_dam *= 2;
    }

    // We'll be constructing a projectile
    projectile proj;
    proj.speed = 10 + skill_level;
    auto &impact = proj.impact;
    auto &proj_effects = proj.proj_effects;

    impact.add_damage( DT_BASH, real_dam );

    if( thrown.has_flag( "ACT_ON_RANGED_HIT" ) ) {
        proj_effects.insert( "ACT_ON_RANGED_HIT" );
        thrown.active = true;
    }

    // Item will shatter upon landing, destroying the item, dealing damage, and making noise
    ///\EFFECT_STR increases chance of shattering thrown glass items (NEGATIVE)
    const bool shatter = !thrown.active && thrown.made_of( material_id( "glass" ) ) &&
                         rng(0, thrown.volume() + 8) - rng(0, str_cur) < thrown.volume();

    // Add some flags to the projectile
    // TODO: Add this flag only when the item is heavy
    proj_effects.insert( "HEAVY_HIT" );
    proj_effects.insert( "NO_ITEM_DAMAGE" );

    if( thrown.active ) {
        // Can't have molotovs embed into mons
        // Mons don't have inventory processing
        proj_effects.insert( "NO_EMBED" );
    }

    if( do_railgun ) {
        proj_effects.insert( "LIGHTNING" );
    }

    if( thrown.volume() > 2 ) {
        proj_effects.insert( "WIDE" );
    }

    // Deal extra cut damage if the item breaks
    if( shatter ) {
        const int glassdam = rng( 0, thrown.volume() * 2 );
        impact.add_damage( DT_CUT, glassdam );
        proj_effects.insert( "SHATTER_SELF" );
    }

    if( rng(0, 100) < 20 + skill_level * 12 && thrown.type->melee_cut > 0 ) {
        const auto type =
            ( thrown.has_flag("SPEAR") || thrown.has_flag("STAB") ) ?
            DT_STAB : DT_CUT;
        proj.impact.add_damage( type, thrown.type->melee_cut );
    }

    // Put the item into the projectile
    proj.set_drop( std::move( thrown ) );
    if( thrown.has_flag( "CUSTOM_EXPLOSION" ) ) {
        proj.set_custom_explosion( thrown.type->explosion );
    }
    const int range = rl_dist( pos(), target );
    proj.range = range;

    auto dealt_attack = projectile_attack( proj, target, shot_dispersion );

    const double missed_by = dealt_attack.missed_by;

    // Copied from the shooting function
    const int range_multiplier = std::min( range, 3 * ( get_skill_level( skill_used ) + 1 ) );
    constexpr int damage_factor = 21;

    if( missed_by <= .1 ) {
        practice( skill_used, damage_factor * range_multiplier );
        // TODO: Check target for existence of head
        if( dealt_attack.hit_critter != nullptr ) {
            lifetime_stats()->headshots++;
        }
    } else if( missed_by <= .2 ) {
        practice( skill_used, damage_factor * range_multiplier / 2 );
    } else if( missed_by <= .4 ) {
        practice( skill_used, damage_factor * range_multiplier / 3 );
    } else if( missed_by <= .6 ) {
        practice( skill_used, damage_factor * range_multiplier / 4 );
    } else if( missed_by <= 1.0 ) {
        practice( skill_used, damage_factor * range_multiplier / 5 );
    } else {
        practice( skill_used, 10 );
    }

    return dealt_attack;
}

static std::string print_recoil( const player &p)
{
    if( p.weapon.is_gun() ) {
        const int adj_recoil = p.recoil + p.driving_recoil;
        if( adj_recoil > MIN_RECOIL ) {
            // 150 is the minimum when not actively aiming
            const char *color_name = "c_ltgray";
            if( adj_recoil >= 690 ) {
                color_name = "c_red";
            } else if( adj_recoil >= 450 ) {
                color_name = "c_ltred";
            } else if( adj_recoil >= 210 ) {
                color_name = "c_yellow";
            }
            return string_format("<color_%s>%s</color>", color_name, _("Recoil"));
        }
    }
    return std::string();
}

// Draws the static portions of the targeting menu,
// returns the number of lines used to draw instructions.
static int draw_targeting_window( WINDOW *w_target, item *relevant, player &p, target_mode mode,
                                  input_context &ctxt, const std::vector<aim_type> &aim_types )
{
    draw_border(w_target);
    // Draw the "title" of the window.
    mvwprintz(w_target, 0, 2, c_white, "< ");
    std::string title;
    if (!relevant) { // currently targetting vehicle to refill with fuel
        title = _("Select a vehicle");
    } else {
        if( mode == TARGET_MODE_FIRE ) {
            if( relevant->has_flag( "RELOAD_AND_SHOOT" ) && relevant->ammo_data() ) {
                title = string_format( _( "Shooting %1$s from %2$s" ),
                        relevant->ammo_data()->nname( relevant->ammo_required() ).c_str(),
                        relevant->tname().c_str() );
            } else {
                title = string_format( _( "Firing %s" ), relevant->tname().c_str() );
            }
            title += " ";
            title += print_recoil( p );
        } else if( mode == TARGET_MODE_THROW ) {
            title = string_format( _("Throwing %s"), relevant->tname().c_str());
        } else {
            title = string_format( _("Setting target for %s"), relevant->tname().c_str());
        }
    }
    trim_and_print( w_target, 0, 4, getmaxx(w_target) - 7, c_red, "%s", title.c_str() );
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
            text_y -= ( 3 + aim_types.size() );
        } else {
            text_y -= 2;
        }
    }

    // The -1 is the -2 from above, but adjusted since this is a total, not an index.
    int lines_used = getmaxy(w_target) - 1 - text_y;
    mvwprintz(w_target, text_y++, 1, c_white, _("Move cursor to target with directional keys"));
    if( relevant ) {
        auto const front_or = [&](std::string const &s, char const fallback) {
            auto const keys = ctxt.keys_bound_to(s);
            return keys.empty() ? fallback : keys.front();
        };

        mvwprintz( w_target, text_y++, 1, c_white, _("%c %c Cycle targets; %c to fire."),
                   front_or("PREV_TARGET", ' '), front_or("NEXT_TARGET", ' '),
                   front_or("FIRE", ' ') );
        mvwprintz( w_target, text_y++, 1, c_white, _("%c target self; %c toggle snap-to-target"),
                   front_or("CENTER", ' ' ), front_or("TOGGLE_SNAP_TO_TARGET", ' ') );
        if( mode == TARGET_MODE_FIRE ) {
            mvwprintz( w_target, text_y++, 1, c_white, _("%c to steady your aim. "),
                       front_or("AIM", ' ') );
            for( std::vector<aim_type>::const_iterator it = aim_types.begin(); it != aim_types.end(); it++ ) {
                if(it->has_threshold){
                    mvwprintz( w_target, text_y++, 1, c_white, it->help.c_str(), front_or( it->action, ' ') );
                }
            }
            mvwprintz( w_target, text_y++, 1, c_white, _("%c to switch aiming modes."),
                       front_or("SWITCH_AIM", ' ') );
        }
    }

    if( is_mouse_enabled() ) {
        mvwprintz(w_target, text_y++, 1, c_white,
                  _("Mouse: LMB: Target, Wheel: Cycle, RMB: Fire"));
    }
    return lines_used;
}

static int find_target( std::vector <Creature *> &t, const tripoint &tpos ) {
    int target = -1;
    for( int i = 0; i < (int)t.size(); i++ ) {
        if( t[i]->pos() == tpos ) {
            target = i;
            break;
        }
    }
    return target;
}

static void do_aim( player *p, std::vector <Creature *> &t, int &target,
                    item *relevant, const tripoint &tpos )
{
    // If we've changed targets, reset aim, unless it's above the minimum.
    if( t[target]->pos() != tpos ) {
        target = find_target( t, tpos );
        // TODO: find radial offset between targets and
        // spend move points swinging the gun around.
        p->recoil = std::max(MIN_RECOIL, p->recoil);
    }

    const int aim_amount = p->aim_per_time( *relevant, p->recoil );
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

static int print_aim_bars( const player &p, WINDOW *w, int line_number, item *weapon,
                           Creature *target, int predicted_recoil ) {
    // This is absolute accuracy for the player.
    // TODO: push the calculations duplicated from Creature::deal_projectile_attack() and
    // Creature::projectile_attack() into shared methods.
    // Dodge is intentionally not accounted for.

    // Confidence is chance of the actual shot being under the target threshold,
    // This simplifies the calculation greatly, that's intentional.
    const double aim_level = predicted_recoil + p.driving_recoil +
        p.get_weapon_dispersion( weapon, false );
    const double range = rl_dist( p.pos(), target->pos() );
    const double missed_by = aim_level * 0.00021666666666666666 * range;
    const double hit_rating = missed_by;
    const double confidence = 1 / hit_rating;
    // This is a relative measure of how steady the player's aim is,
    // 0 it is the best the player can do.
    const double steady_score = predicted_recoil - p.weapon.sight_dispersion( -1 );
    // Fairly arbitrary cap on steadiness...
    const double steadiness = 1.0 - steady_score / 250;

    const std::array<std::pair<double, char>, 3> confidence_ratings = {{
        std::make_pair( 0.1, '*' ),
        std::make_pair( 0.4, '+' ),
        std::make_pair( 0.6, '|' ) }};

    const int window_width = getmaxx( w ) - 2; // Window width minus borders.
    const std::string &confidence_bar = get_labeled_bar( confidence, window_width, _( "Confidence" ),
                                                         confidence_ratings.begin(),
                                                         confidence_ratings.end() );
    const std::string &steadiness_bar = get_labeled_bar( steadiness, window_width,
                                                         _( "Steadiness" ), '*' );

    mvwprintw( w, line_number++, 1, _( "Symbols: * = Headshot + = Hit | = Graze" ) );
    mvwprintw( w, line_number++, 1, confidence_bar.c_str() );
    mvwprintw( w, line_number++, 1, steadiness_bar.c_str() );

    return line_number;
}

static int draw_turret_aim( const player &p, WINDOW *w, int line_number, const tripoint &targ )
{
    vehicle *veh = g->m.veh_at( p.pos() );
    if( veh == nullptr ) {
        debugmsg( "Tried to aim turret while outside vehicle" );
        return line_number;
    }

    const auto turret_state = veh->turrets_can_shoot( targ );
    int num_ok = 0;
    for( const auto &pr : turret_state ) {
        if( pr.second == turret_all_ok ) {
            num_ok++;
        }
    }

    mvwprintw( w, line_number++, 1, _("Turrets in range: %d"), num_ok );
    return line_number;
}

// TODO: Shunt redundant drawing code elsewhere
std::vector<tripoint> game::target( tripoint &p, const tripoint &low, const tripoint &high,
                                    std::vector<Creature *> t, int &target,
                                    item *relevant, target_mode mode,
                                    const tripoint &from_arg )
{
    std::vector<tripoint> ret;
    tripoint from = from_arg;
    if( from == tripoint_min ) {
        from = u.pos();
    }
    int range = ( high.x - from.x );
    // First, decide on a target among the monsters, if there are any in range
    if( !t.empty() ) {
        if( static_cast<size_t>( target ) >= t.size() ) {
            target = 0;
        }
        p = t[target]->pos();
    } else {
        target = -1; // No monsters in range, don't use target, reset to -1
    }

    bool sideStyle = use_narrow_sidebar();
    bool compact_window = TERMY < 34;
    int height = compact_window ? 18 : 25;
    int width  = getmaxx(w_messages);
    // Overlap the player info window.
    int top    = ( compact_window ? -4 : -1 ) + (sideStyle ? getbegy(w_messages) : (getbegy(w_minimap) + getmaxy(w_minimap)) );
    int left   = getbegx(w_messages);

    // Keeping the target menu window around between invocations,
    // it only gets reset if we actually exit the menu.
    static WINDOW *w_target = nullptr;
    if( w_target == nullptr ) {
        w_target = newwin(height, width, top, left);
    }

    std::vector<aim_type> aim_types;
    std::vector<aim_type>::iterator aim_mode;
    int sight_dispersion = u.weapon.sight_dispersion( -1 );

    input_context ctxt("TARGET");
    ctxt.set_iso(true);
    // "ANY_INPUT" should be added before any real help strings
    // Or strings will be written on window border.
    ctxt.register_action("ANY_INPUT");
    ctxt.register_directions();
    ctxt.register_action("COORDINATE");
    ctxt.register_action("SELECT");
    ctxt.register_action("FIRE");
    ctxt.register_action("NEXT_TARGET");
    ctxt.register_action("PREV_TARGET");
    ctxt.register_action( "LEVEL_UP" );
    ctxt.register_action( "LEVEL_DOWN" );
    if( mode == TARGET_MODE_FIRE ) {
        ctxt.register_action("AIM");
        ctxt.register_action("SWITCH_AIM");
        aim_types.push_back( aim_type { "", "", "", false, 0 } ); // dummy aim type for unaimed shots
        const int threshold_step = 30;
        // Aiming thresholds are dependent on weapon sight dispersion, attempting to place thresholds
        // at 66%, 33% and 0% of the difference between MIN_RECOIL and sight dispersion. The thresholds
        // are then floored to multiples of threshold_step.
        // With a MIN_RECOIL of 150 and threshold_step of 30, this means:-
        // Weapons with <90 s_d can be aimed 'precisely'
        // Weapons with <120 s_d can be aimed 'carefully'
        // All other weapons can only be 'aimed'
        std::vector<int> thresholds = {
            (int) floor( ( ( MIN_RECOIL - sight_dispersion ) * 2 / 3 + sight_dispersion ) /
                         threshold_step ) * threshold_step,
            (int) floor( ( ( MIN_RECOIL - sight_dispersion ) / 3 + sight_dispersion ) /
                         threshold_step ) * threshold_step,
            (int) floor( sight_dispersion / threshold_step ) * threshold_step };
        std::vector<int>::iterator thresholds_it;
        // Remove duplicate thresholds.
        thresholds_it = std::adjacent_find( thresholds.begin(), thresholds.end() );
        while( thresholds_it != thresholds.end() ) {
            thresholds.erase( thresholds_it );
            thresholds_it = std::adjacent_find( thresholds.begin(), thresholds.end() );
        }
        thresholds_it = thresholds.begin();
        aim_types.push_back( aim_type { _("Aim"), "AIMED_SHOT",
                             _("%c to aim and fire."), true,
                             *thresholds_it } );
        thresholds_it++;
        if( thresholds_it != thresholds.end() ) {
            aim_types.push_back( aim_type { _("Careful Aim"), "CAREFUL_SHOT",
                                 _("%c to take careful aim and fire."), true,
                                 *thresholds_it } );
            thresholds_it++;
            if( thresholds_it != thresholds.end() ) {
                aim_types.push_back( aim_type { _("Precise Aim"), "PRECISE_SHOT",
                                     _("%c to take precise aim and fire."), true,
                                     *thresholds_it } );
            }
        }
        for( std::vector<aim_type>::iterator it = aim_types.begin(); it != aim_types.end(); it++ ) {
            if( it->has_threshold ) {
                ctxt.register_action( it->action );
            }
        }
        aim_mode = aim_types.begin();
    }
    ctxt.register_action("CENTER");
    ctxt.register_action("TOGGLE_SNAP_TO_TARGET");
    ctxt.register_action("HELP_KEYBINDINGS");
    ctxt.register_action("QUIT");
    int num_instruction_lines = draw_targeting_window( w_target, relevant, u, mode, ctxt, aim_types );
    bool snap_to_target = OPTIONS["SNAP_TO_TARGET"];

    std::string enemiesmsg;
    if( t.empty() ) {
        enemiesmsg = _("No targets in range.");
    } else {
        enemiesmsg = string_format(ngettext("%d target in range.", "%d targets in range.",
                                            t.size()), t.size());
    }

    const tripoint old_offset = u.view_offset;
    do {
        ret = g->m.find_clear_path( from, p );

        // This chunk of code handles shifting the aim point around
        // at maximum range when using circular distance.
        // The range > 1 check ensures that you can alweays at least hit adjacent squares.
        if( trigdist && range > 1 && round(trig_dist( from, p )) > range ) {
            bool cont = true;
            tripoint cp = p;
            for( size_t i = 0; i < ret.size() && cont; i++ ) {
                if( round(trig_dist( from, ret[i] )) > range ) {
                    ret.resize(i);
                    cont = false;
                } else {
                    cp = ret[i];
                }
            }
            p = cp;
        }
        tripoint center;
        if( snap_to_target ) {
            center = p;
        } else {
            center = u.pos() + u.view_offset;
        }
        // Clear the target window.
        for( int i = 1; i <= getmaxy(w_target) - num_instruction_lines - 2; i++ ) {
            // Clear width excluding borders.
            for( int j = 1; j <= getmaxx(w_target) - 2; j++ ) {
                mvwputch( w_target, i, j, c_white, ' ' );
            }
        }
        draw_ter(center, true);
        int line_number = 1;
        Creature *critter = critter_at( p, true );
        if( p != from ) {
            // Only draw those tiles which are on current z-level
            auto ret_this_zlevel = ret;
            ret_this_zlevel.erase( std::remove_if( ret_this_zlevel.begin(), ret_this_zlevel.end(),
                [&center]( const tripoint &pt ) { return pt.z != center.z; } ), ret_this_zlevel.end() );
            // Only draw a highlighted trajectory if we can see the endpoint.
            // Provides feedback to the player, and avoids leaking information
            // about tiles they can't see.
            draw_line( p, center, ret_this_zlevel );

            // Print to target window
            if( !relevant ) {
                // currently targetting vehicle to refill with fuel
                vehicle *veh = m.veh_at(p);
                if( veh != nullptr && u.sees( p ) ) {
                    mvwprintw(w_target, line_number++, 1, _("There is a %s"),
                              veh->name.c_str());
                }
            } else if( relevant == &u.weapon && relevant->is_gun() ) {
                // firing a gun
                mvwprintw(w_target, line_number, 1, _("Range: %d/%d, %s"),
                          rl_dist(from, p), range, enemiesmsg.c_str());
                // get the current weapon mode or mods
                std::string mode = "";
                if( u.weapon.get_gun_mode() == "MODE_BURST" ) {
                    mode = _("Burst");
                } else {
                    item *gunmod = u.weapon.gunmod_current();
                    if( gunmod != NULL ) {
                        mode = gunmod->type_name();
                    }
                }
                if( mode != "" ) {
                    mvwprintw( w_target, line_number, 14, _("Firing mode: %s"),
                               mode.c_str() );
                }
                line_number++;
            } else {
                // throwing something or setting turret's target
                mvwprintw( w_target, line_number++, 1, _("Range: %d/%d, %s"),
                          rl_dist(from, p), range, enemiesmsg.c_str() );
            }

            if( critter != nullptr && u.sees( *critter ) ) {
                // The 6 is 2 for the border and 4 for aim bars.
                int available_lines = compact_window ? 1 :
                                      ( height - num_instruction_lines - line_number - 6 );
                line_number = critter->print_info( w_target, line_number, available_lines, 1);
            } else {
                mvwputch(w_terrain, POSY + p.y - center.y, POSX + p.x - center.x, c_red, '*');
            }
        } else {
            mvwprintw( w_target, line_number++, 1, _("Range: %d, %s"), range, enemiesmsg.c_str() );
        }

        if( mode == TARGET_MODE_FIRE && critter != nullptr && u.sees( *critter ) ) {
            int predicted_recoil = u.recoil;
            int predicted_delay = 0;
            if( aim_mode->has_threshold && aim_mode->threshold < u.recoil ) {
                do{
                    const int aim_amount = u.aim_per_time( u.weapon, predicted_recoil );
                    if( aim_amount > 0 ) {
                        predicted_delay += 10;
                        predicted_recoil = std::max( predicted_recoil - aim_amount , 0);
                    }
                } while( predicted_recoil > aim_mode->threshold &&
                          predicted_recoil - sight_dispersion > 0 );
            } else {
                predicted_recoil = u.recoil;
            }
            if( relevant->gunmod_current() ) {
                line_number = print_aim_bars( u, w_target, line_number, relevant->gunmod_current(), critter, predicted_recoil );
            } else {
                line_number = print_aim_bars( u, w_target, line_number, relevant, critter, predicted_recoil );
            }
            if( aim_mode->has_threshold ) {
                mvwprintw(w_target, line_number++, 1, _("%s Delay: %i"), aim_mode->name.c_str(), predicted_delay );
            }
        } else if( mode == TARGET_MODE_TURRET ) {
            line_number = draw_turret_aim( u, w_target, line_number, p );
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

        tripoint targ( 0, 0, 0 );
        // Our coordinates will either be determined by coordinate input(mouse),
        // by a direction key, or by the previous value.
        if( action == "SELECT" && ctxt.get_coordinates(g->w_terrain, targ.x, targ.y) ) {
            if( !OPTIONS["USE_TILES"] && snap_to_target ) {
                // Snap to target doesn't currently work with tiles.
                targ.x += p.x - from.x;
                targ.y += p.y - from.y;
            }
            targ.x -= p.x;
            targ.y -= p.y;
        } else {
            ctxt.get_direction(targ.x, targ.y, action);
            if( targ.x == -2 ) {
                targ.x = 0;
                targ.y = 0;
            }
        }
        if( action == "FIRE" && mode == TARGET_MODE_FIRE && aim_mode->has_threshold ) {
            action = aim_mode->action;
        }
        if( g->m.has_zlevels() && action == "LEVEL_UP" ) {
            p.z++;
            u.view_offset.z++;
        } else if( g->m.has_zlevels() && action == "LEVEL_DOWN" ) {
            p.z--;
            u.view_offset.z--;
        }

        /* More drawing to terrain */
        if( targ != tripoint_zero ) {
            const Creature *critter = critter_at( p, true );
            if( critter != nullptr ) {
                draw_critter( *critter, center );
            } else if( m.pl_sees( p, -1 ) ) {
                m.drawsq( w_terrain, u, p, false, true, center );
            } else {
                mvwputch( w_terrain, POSY, POSX, c_black, 'X' );
            }
            p.x += targ.x;
            p.y += targ.y;
            p.z += targ.z;
            if( p.x < low.x ) {
                p.x = low.x;
            } else if ( p.x > high.x ) {
                p.x = high.x;
            }
            if ( p.y < low.y ) {
                p.y = low.y;
            } else if ( p.y > high.y ) {
                p.y = high.y;
            }
            if ( p.z < low.z ) {
                p.z = low.z;
            } else if ( p.z > high.z ) {
                p.z = high.z;
            }
        } else if( (action == "PREV_TARGET") && (target != -1) ) {
            int newtarget = find_target( t, p ) - 1;
            if( newtarget < 0 ) {
                newtarget = t.size() - 1;
            }
            p = t[newtarget]->pos();
        } else if( (action == "NEXT_TARGET") && (target != -1) ) {
            int newtarget = find_target( t, p ) + 1;
            if( newtarget == (int)t.size() ) {
                newtarget = 0;
            }
            p = t[newtarget]->pos();
        } else if( (action == "AIM") && target != -1 ) {
            do_aim( &u, t, target, relevant, p );
            if( u.moves <= 0 ) {
                // We've run out of moves, clear target vector, but leave target selected.
                u.assign_activity( ACT_AIM, 0, 0 );
                u.activity.str_values.push_back( "AIM" );
                ret.clear();
                u.view_offset = old_offset;
                return ret;
            }
        } else if( action == "SWITCH_AIM" ) {
            aim_mode++;
            if( aim_mode == aim_types.end() ) {
                aim_mode = aim_types.begin();
            }
        } else if( (action == "AIMED_SHOT" || action == "CAREFUL_SHOT" || action == "PRECISE_SHOT") &&
                   target != -1 ) {
            int aim_threshold;
            std::vector<aim_type>::iterator it;
            for( it = aim_types.begin(); it != aim_types.end(); it++ ) {
                if ( action == it->action ) {
                    break;
                }
            }
            if( it == aim_types.end() ) {
                debugmsg( "Could not find a valid aim_type for %s", action.c_str() );
                aim_mode = aim_types.begin();
            }
            aim_threshold = it->threshold;
            do {
                do_aim( &u, t, target, relevant, p );
            } while( target != -1 && u.moves > 0 && u.recoil > aim_threshold &&
                     u.recoil - sight_dispersion > 0 );
            if( target == -1 ) {
                // Bail out if there's no target.
                continue;
            }
            if( u.recoil <= aim_threshold ||
                u.recoil - sight_dispersion == 0) {
                // If we made it under the aim threshold, go ahead and fire.
                // Also fire if we're at our best aim level already.
                werase( w_target );
                wrefresh( w_target );
                delwin( w_target );
                w_target = nullptr;
                u.view_offset = old_offset;
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
                u.view_offset = old_offset;
                return ret;
            }
        } else if( action == "FIRE" ) {
            target = find_target( t, p );
            if( from == p ) {
                ret.clear();
            }
            break;
        } else if( action == "CENTER" ) {
            p = from;
            ret.clear();
        } else if( action == "TOGGLE_SNAP_TO_TARGET" ) {
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
    u.view_offset = old_offset;
    return ret;
}

static projectile make_gun_projectile( const item &gun ) {
    projectile proj;
    proj.speed  = 1000;
    proj.impact = damage_instance::physical( 0, gun.gun_damage(), 0, gun.gun_pierce() );
    proj.range = gun.gun_range();
    proj.proj_effects = gun.ammo_effects();

    auto &fx = proj.proj_effects;

    if( ( gun.ammo_data() && gun.ammo_data()->phase == LIQUID ) ||
        fx.count( "SHOT" ) || fx.count("BOUNCE" ) ) {
        fx.insert( "WIDE" );
    }

    if( gun.ammo_data() ) {
        // Some projectiles have a chance of being recoverable
        bool recover = std::any_of(fx.begin(), fx.end(), []( const std::string& e ) {
            int n;
            return sscanf( e.c_str(), "RECOVER_%i", &n ) == 1 && !one_in( n );
        });

        if( recover && !fx.count( "IGNITE" ) && !fx.count( "EXPLOSIVE" ) ) {
            item drop( gun.ammo_current(), calendar::turn, 1 );
            drop.active = fx.count( "ACT_ON_RANGED_HIT" );
            proj.set_drop( drop );
        }

        if( fx.count( "CUSTOM_EXPLOSION" ) > 0  ) {
            proj.set_custom_explosion( gun.ammo_data()->explosion );
        }
    }

    return proj;
}

int time_to_fire(player &p, const itype &firingt)
{
    struct time_info_t {
        int min_time;  // absolute floor on the time taken to fire.
        int base;      // the base or max time taken to fire.
        int reduction; // the reduction in time given per skill level.
    };

    static std::map<skill_id, time_info_t> const map {
        {skill_id {"pistol"},   {10, 80,  10}},
        {skill_id {"shotgun"},  {70, 150, 25}},
        {skill_id {"smg"},      {20, 80,  10}},
        {skill_id {"rifle"},    {30, 150, 15}},
        {skill_id {"archery"},  {20, 220, 25}},
        {skill_id {"throw"},    {50, 220, 25}},
        {skill_id {"launcher"}, {30, 200, 20}},
        {skill_id {"melee"},    {50, 200, 20}}
    };

    const skill_id &skill_used = firingt.gun.get()->skill_used;
    auto const it = map.find( skill_used );
    // TODO: maybe JSON-ize this in some way? Probably as part of the skill class.
    static const time_info_t default_info{ 50, 220, 25 };

    time_info_t const &info = (it == map.end()) ? default_info : it->second;
    return std::max(info.min_time, info.base - info.reduction * p.get_skill_level( skill_used ));
}

static inline void eject_casing( player& p, item& weap ) {
    // eject casings and linkages in random direction avoiding walls using player position as fallback
    auto tiles = closest_tripoints_first( 1, p.pos() );
    tiles.erase( tiles.begin() );
    tiles.erase( std::remove_if( tiles.begin(), tiles.end(), [&p]( const tripoint& e ) {
        return !g->m.passable( e );
    } ), tiles.end() );
    tripoint eject = tiles.empty() ? p.pos() : random_entry( tiles );

    // some magazines also eject disintegrating linkages
    const auto mag = weap.magazine_current();
    if( mag && mag->type->magazine->linkage != "NULL" ) {
        g->m.add_item_or_charges( eject, item( mag->type->magazine->linkage, calendar::turn, 1 ) );
    }

    if( weap.ammo_casing() == "null" ) {
        return;
    }

    if( weap.has_flag( "RELOAD_EJECT" ) ) {
        const int num_casings = weap.get_var( "CASINGS", 0 );
        weap.set_var( "CASINGS", num_casings + 1 );
        return;
    }

    if( weap.gunmod_find( "brass_catcher" ) ) {
        p.i_add( item( weap.ammo_casing(), calendar::turn, 1 ) );
        return;
    }

    g->m.add_item_or_charges( eject, item( weap.ammo_casing(), calendar::turn, 1 ) );
    sfx::play_variant_sound( "fire_gun", "brass_eject", sfx::get_heard_volume( eject ), sfx::get_heard_angle( eject ) );
}

void make_gun_sound_effect(player &p, bool burst, item *weapon)
{
    const auto data = weapon->gun_noise( burst );
    if( data.volume > 0 ) {
        sounds::sound( p.pos(), data.volume, data.sound );
    }
}

item::sound_data item::gun_noise( bool const burst ) const
{
    if( !is_gun() ) {
        return { 0, "" };
    }

    int noise = type->gun->loudness;
    for( const auto mod : gunmods() ) {
        noise += mod->type->gunmod->loudness;
    }
    if( ammo_data() ) {
        noise += ammo_data()->ammo->loudness;
    }

    noise = std::max( noise, 0 );

    if( ammo_type() == "40mm") {
        return { 8, _( "Thunk!" ) };

    } else if( typeId() == "hk_g80") {
        return { 24, _( "tz-CRACKck!" ) };

    } else if( ammo_type() == "gasoline" || ammo_type() == "66mm" ||
               ammo_type() == "84x246mm" || ammo_type() == "m235" ) {
        return { 4, _( "Fwoosh!" ) };
    }

    auto fx = ammo_effects();

    if( fx.count( "LASER" ) || fx.count( "PLASMA" ) ) {
        if( noise < 20 ) {
            return { noise, _( "Fzzt!" ) };
        } else if( noise < 40 ) {
            return { noise, _( "Pew!" ) };
        } else if( noise < 60 ) {
            return { noise, _( "Tsewww!" ) };
        } else {
            return { noise, _( "Kra-kow!!" ) };
        }

    } else if( fx.count( "LIGHTNING" ) ) {
        if( noise < 20 ) {
            return { noise, _( "Bzzt!" ) };
        } else if( noise < 40 ) {
            return { noise, _( "Bzap!" ) };
        } else if( noise < 60 ) {
            return { noise, _( "Bzaapp!" ) };
        } else {
            return { noise, _( "Kra-koom!!" ) };
        }

    } else if( fx.count( "WHIP" ) ) {
        return { noise, _( "Crack!" ) };

    } else if( noise > 0 ) {
        if( noise < 10 ) {
            return { noise, burst ? _( "Brrrip!" ) : _( "plink!" ) };
        } else if( noise < 150 ) {
            return { noise, burst ? _( "Brrrap!" ) : _( "bang!" ) };
        } else if(noise < 175 ) {
            return { noise, burst ? _( "P-p-p-pow!" ) : _( "blam!" ) };
        } else {
            return { noise, burst ? _( "Kaboom!!" ) : _( "kerblam!" ) };
        }
    }

    return { 0, "" }; // silent weapons
}

// Little helper to clean up dispersion calculation methods.
static int rand_or_max( bool random, int max )
{
    return random ? rng(0, max) : max;
}

static bool is_driving( const player &p )
{
    const auto veh = g->m.veh_at( p.pos() );
    return veh && veh->velocity != 0 && veh->player_in_control( p );
}


// utility functions for projectile_attack
double player::get_weapon_dispersion( const item *weapon, bool random ) const
{
    double dispersion = 0.; // Measured in quarter-degrees.
    dispersion += skill_dispersion( *weapon, random );

    if( is_driving( *this ) ) {
        // get volume of gun (or for auxiliary gunmods the parent gun)
        const item *parent = has_item( *weapon ) ? find_parent( *weapon ) : nullptr;
        int vol = parent ? parent->volume() : weapon->volume();

        ///\EFFECT_DRIVING reduces the inaccuracy penalty when using guns whilst driving
        dispersion += std::max( vol - get_skill_level( skill_driving ), 1 ) * 20;
    }

    dispersion += rand_or_max( random, ranged_dex_mod() );
    dispersion += rand_or_max( random, ranged_per_mod() );

    dispersion += rand_or_max( random, 3 * (encumb(bp_arm_l) + encumb(bp_arm_r)));
    dispersion += rand_or_max( random, 6 * encumb(bp_eyes));

    if( weapon->ammo_data() ) {
        dispersion += rand_or_max( random, weapon->ammo_data()->ammo->dispersion );
    }

    dispersion += rand_or_max( random, weapon->gun_dispersion(false) );
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

    return std::max( dispersion, 0.0 );
}

int recoil_add( player& p, const item &gun, int shot )
{
    if( p.has_effect( effect_on_roof ) ) {
        // @todo fix handling of turret recoil
        return p.recoil;
    }

    int qty = gun.gun_recoil();

    ///\EFFECT_STR reduces recoil when using guns and tools
    qty -= rng( 7, 15 ) * p.get_str();

    ///\EFFECT_PISTOL randomly decreases recoil with appropriate guns
    ///\EFFECT_RIFLE randomly decreases recoil with appropriate guns
    ///\EFFECT_SHOTGUN randomly decreases recoil with appropriate guns
    ///\EFFECT_SMG randomly decreases recoil with appropriate guns
    qty -= rng( 0, p.get_skill_level( gun.gun_skill() ) * 7 );

    // when firing in bursts reduce the penalty from each sucessive shot
    double k = 1.6; // 5 round burst is equivalent to ~2 individually aimed shots
    qty *= pow( 1.0 / sqrt( shot ), k );

    return p.recoil += std::max( qty, 0 );
}

void splatter( const std::vector<tripoint> &trajectory, int dam, const Creature *target )
{
    if( dam <= 0 ) {
        return;
    }

    if( !target->is_npc() && !target->is_player() ) {
        //Check if the creature isn't an NPC or the player (so the cast works)
        const monster *mon = dynamic_cast<const monster *>( target );
        if( mon->is_hallucination() || !mon->made_of( material_id( "flesh" ) ) ) {
            // If it is a hallucination or not made of flesh don't splatter the blood.
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

    std::vector<tripoint> spurt = continue_line( trajectory, distance );

    for( auto &elem : spurt ) {
        g->m.adjust_field_strength( elem, blood, 1 );
        if( g->m.impassable( elem ) ) {
            // Blood splatters stop at walls.
            break;
        }
    }
}

void drop_or_embed_projectile( const dealt_projectile_attack &attack )
{
    const auto &proj = attack.proj;
    const auto &drop_item = proj.get_drop();
    const auto &effects = proj.proj_effects;
    if( drop_item.is_null() ) {
        return;
    }

    const tripoint &pt = attack.end_point;

    if( effects.count( "SHATTER_SELF" ) ) {
        // Drop the contents, not the thrown item
        if( g->u.sees( pt ) ) {
            add_msg( _("The %s shatters!"), drop_item.tname().c_str() );
        }

        for( const item &i : drop_item.contents ) {
            g->m.add_item_or_charges( pt, i );
        }
        // TODO: Non-glass breaking
        // TODO: Wine glass breaking vs. entire sheet of glass breaking
        sounds::sound(pt, 16, _("glass breaking!"));
        return;
    }

    // Copy the item
    item dropped_item = drop_item;

    monster *mon = dynamic_cast<monster *>( attack.hit_critter );

    // We can only embed in monsters
    bool embed = mon != nullptr && !mon->is_dead_state();
    // And if we actually want to embed
    embed = embed && effects.count( "NO_EMBED" ) == 0;
    // Don't embed in small creatures
    if( embed ) {
        const m_size critter_size = mon->get_size();
        const int vol = dropped_item.volume();
        embed = embed && ( critter_size > MS_TINY || vol < 1 );
        embed = embed && ( critter_size > MS_SMALL || vol < 2 );
        // And if we deal enough damage
        // Item volume bumps up the required damage too
        embed = embed &&
                 ( attack.dealt_dam.type_damage( DT_CUT ) / 2 ) +
                   attack.dealt_dam.type_damage( DT_STAB ) >
                     attack.dealt_dam.type_damage( DT_BASH ) +
                     vol * 3 + rng( 0, 5 );
    }

    if( embed ) {
        mon->add_item( dropped_item );
        if( g->u.sees( *mon ) ) {
            add_msg( _("The %1$s embeds in %2$s!"),
                     dropped_item.tname().c_str(),
                     mon->disp_name().c_str() );
        }
    } else {
        bool do_drop = true;
        if( effects.count( "ACT_ON_RANGED_HIT" ) ) {
            // Don't drop if it exploded
            do_drop = !dropped_item.process( nullptr, attack.end_point, true );
        }

        if( do_drop ) {
            g->m.add_item_or_charges( attack.end_point, dropped_item );
        }

        if( effects.count( "HEAVY_HIT" ) ) {
            if( g->m.has_flag( "LIQUID", pt ) ) {
                sounds::sound( pt, 10, _("splash!") );
            } else {
                sounds::sound( pt, 8, _("thud.") );
            }
            const trap &tr = g->m.tr_at( pt );
            if( tr.triggered_by_item( dropped_item ) ) {
                tr.trigger( pt, nullptr );
            }
        }
    }
}

