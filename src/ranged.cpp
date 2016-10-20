#include <vector>
#include <string>
#include <cmath>
#include "cata_utility.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "output.h"
#include "line.h"
#include "skill.h"
#include "rng.h"
#include "projectile.h"
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
#include <algorithm>

using namespace units::literals;

const skill_id skill_throw( "throw" );
const skill_id skill_gun( "gun" );
const skill_id skill_driving( "driving" );
const skill_id skill_dodge( "dodge" );
const skill_id skill_launcher( "launcher" );

const efftype_id effect_on_roof( "on_roof" );
const efftype_id effect_bounced( "bounced" );
const efftype_id effect_hit_by_player( "hit_by_player" );

static projectile make_gun_projectile( const item &gun );
int time_to_fire( const Character &p, const itype &firing );
static void cycle_action( item& weap, const tripoint &pos );
void make_gun_sound_effect(player &p, bool burst, item *weapon);
extern bool is_valid_in_w_terrain(int, int);
void drop_or_embed_projectile( const dealt_projectile_attack &attack );

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

size_t blood_trail_len( int damage )
{
    if( damage > 50 ) {
        return 3;
    } else if( damage > 20 ) {
        return 2;
    } else if ( damage > 0 ) {
        return 1;
    }
    return 0;
}

dealt_projectile_attack Creature::projectile_attack( const projectile &proj_arg, const tripoint &source,
                                                     const tripoint &target_arg, double dispersion )
{
    const bool do_animation = get_option<bool>( "ANIMATIONS" );

    double range = rl_dist( source, target_arg );

    // an isosceles triangle is formed by the intended and actual targets
    double missed_by = iso_tangent( range, dispersion );

    // TODO: move to-hit roll back in here

    dealt_projectile_attack attack {
        proj_arg, nullptr, dealt_damage_instance(), source, missed_by
    };

    if( source == target_arg ) { // No suicidal shots
        debugmsg( "%s tried to commit suicide.", get_name().c_str() );
        return attack;
    }

    projectile &proj = attack.proj;
    const auto &proj_effects = proj.proj_effects;

    const bool stream = proj_effects.count("STREAM") > 0 ||
                        proj_effects.count("STREAM_BIG") > 0 ||
                        proj_effects.count("JET") > 0;
    const bool no_item_damage = proj_effects.count( "NO_ITEM_DAMAGE" ) > 0;
    const bool do_draw_line = proj_effects.count( "DRAW_AS_LINE" ) > 0;
    const bool null_source = proj_effects.count( "NULL_SOURCE" ) > 0;
    // Determines whether it can penetrate obstacles
    const bool is_bullet = proj_arg.speed >= 200 && std::any_of( proj_arg.impact.damage_units.begin(),
                           proj_arg.impact.damage_units.end(),
                           []( const damage_unit &dam )
    {
        return dam.type == DT_CUT;
    } );

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
        double spread = std::min( ARCMIN( dispersion ), DEGREES( 30 ) );
        rad += rng_float( -spread, spread );

        // @todo This should also represent the miss on z axis
        const int offset = std::min<int>( range, sqrtf( missed_by ) );
        int new_range = no_overshoot ?
                            range + rng( -offset, offset ) :
                            rng( range - offset, proj_arg.range );
        new_range = std::max( new_range, 1 );

        target.x = source.x + roll_remainder( new_range * cos( rad ) );
        target.y = source.y + roll_remainder( new_range * sin( rad ) );

        if( target == source ) {
            target.x = source.x + sgn( dx );
            target.y = source.y + sgn( dy );
        }

        // Don't extend range further, miss here can mean hitting the ground near the target
        range = rl_dist( source, target );
        extend_to_range = range;

        // Cap missed_by at 1.0
        missed_by = 1.0;
        sfx::play_variant_sound( "bullet_hit", "hit_wall", sfx::get_heard_volume( target ), sfx::get_heard_angle( target ));
        // TODO: Z dispersion
        // If we missed, just draw a straight line.
        trajectory = line_to( source, target );
    } else {
        // Go around obstacles a little if we're on target.
        trajectory = g->m.find_clear_path( source, target );
    }

    add_msg( m_debug, "%s proj_atk: shot_dispersion: %.2f", disp_name().c_str(), dispersion );

    add_msg( m_debug, "missed_by: %.2f target (orig/hit): %d,%d,%d/%d,%d,%d", missed_by,
             target_arg.x, target_arg.y, target_arg.z,
             target.x, target.y, target.z );

    // Trace the trajectory, doing damage in order
    tripoint &tp = attack.end_point;
    tripoint prev_point = source;

    trajectory.insert( trajectory.begin(), source ); // Add the first point to the trajectory

    static emit_id muzzle_smoke( "emit_smoke_plume" );
    if( proj_effects.count( "MUZZLE_SMOKE" ) ) {
        g->m.emit_field( trajectory.front(), muzzle_smoke );
    }

    if( !no_overshoot && range < extend_to_range ) {
        // Continue line is very "stiff" when the original range is short
        // @todo Make it use a more distant point for more realistic extended lines
        std::vector<tripoint> trajectory_extension = continue_line( trajectory,
                                                                    extend_to_range - range );
        trajectory.reserve( trajectory.size() + trajectory_extension.size() );
        trajectory.insert( trajectory.end(), trajectory_extension.begin(), trajectory_extension.end() );
    }
    // Range can be 0
    size_t traj_len = trajectory.size();
    while( traj_len > 0 && rl_dist( source, trajectory[traj_len-1] ) > proj_arg.range ) {
        --traj_len;
    }

    // If this is a vehicle mounted turret, which vehicle is it mounted on?
    const vehicle *in_veh = has_effect( effect_on_roof ) ?
                            g->m.veh_at( pos() ) : nullptr;

    const float projectile_skip_multiplier = 0.1;
    // Randomize the skip so that bursts look nicer
    int projectile_skip_calculation = range * projectile_skip_multiplier;
    int projectile_skip_current_frame = rng( 0, projectile_skip_calculation );
    bool has_momentum = true;

    for( size_t i = 1; i < traj_len && ( has_momentum || stream ); ++i ) {
        prev_point = tp;
        tp = trajectory[i];

        if( ( tp.z > prev_point.z && g->m.has_floor( tp ) ) ||
            ( tp.z < prev_point.z && g->m.has_floor( prev_point ) ) ) {
            // Currently strictly no shooting through floor
            // TODO: Bash the floor
            tp = prev_point;
            traj_len = --i;
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
        if( critter == this ) {
            // No hitting self with "weird" attacks
            critter = nullptr;
        }

        monster *mon = dynamic_cast<monster *>(critter);
        // ignore non-point-blank digging targets (since they are underground)
        if( mon != nullptr && mon->digging() &&
            rl_dist( pos(), tp ) > 1) {
            critter = nullptr;
            mon = nullptr;
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
            attack.missed_by = cur_missed_by;
            critter->deal_projectile_attack( null_source ? nullptr : this, attack );
            // Critter can still dodge the projectile
            // In this case hit_critter won't be set
            if( attack.hit_critter != nullptr ) {
                const size_t bt_len = blood_trail_len( attack.dealt_dam.total_damage() );
                if( bt_len > 0 ) {
                    const tripoint &dest = move_along_line( tp, trajectory, bt_len );
                    g->m.add_splatter_trail( critter->bloodType(), tp, dest );
                }
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

        if( ( !has_momentum || !is_bullet ) && g->m.impassable( tp ) ) {
            // Don't let flamethrowers go through walls
            // TODO: Let them go through bars
            traj_len = i;
            break;
        }
    } // Done with the trajectory!

    if( do_animation && do_draw_line && traj_len > 2 ) {
        trajectory.erase( trajectory.begin() );
        trajectory.resize( traj_len-- );
        g->draw_line( tp, trajectory );
        g->draw_bullet( g->u, tp, int( traj_len-- ), trajectory, stream ? '#' : '*' );
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
                    projectile_attack( proj, tp, z.pos(), dispersion );
                    sfx::play_variant_sound( "fire_gun", "bio_lightning_tail", sfx::get_heard_volume(z.pos()), sfx::get_heard_angle(z.pos()));
                    break;
                }
            }
        }
    }

    return attack;
}

double player::gun_current_range( const item& gun, double penalty, unsigned chance, double accuracy ) const
{
    if( !gun.is_gun() || !gun.ammo_sufficient() ) {
        return 0;
    }

    if( chance == 0 ) {
        return gun.gun_range( this );
    }

    // calculate maximum potential dispersion
    double dispersion = get_weapon_dispersion( gun ) + ( penalty < 0 ? recoil_total() : penalty );

    // dispersion is uniformly distributed at random so scale linearly with chance
    dispersion *= chance / 100.0;

    // cap at min 1MOA as at zero dispersion would result in an infinite effective range
    dispersion = std::max( dispersion, 1.0 );

    double res = accuracy / sin( ARCMIN( dispersion / 2 ) ) / 2;

    // effective range could be limited by othe factors (eg. STR_DRAW etc)
    return std::min( res, double( gun.gun_range( this ) ) );
}

double player::gun_engagement_range( const item &gun, engagement opt ) const
{
    switch( opt ) {
        case engagement::snapshot:
            return gun_current_range( gun, MIN_RECOIL, 50, accuracy_goodhit );

        case engagement::effective:
            return gun_current_range( gun, effective_dispersion( gun.sight_dispersion() ), 50, accuracy_goodhit );

        case engagement::maximum:
            return gun_current_range( gun, effective_dispersion( gun.sight_dispersion() ), 10, accuracy_grazing );
    }

    abort(); // never reached
}

int player::gun_engagement_moves( const item &gun ) const
{
    int mv = 0;
    double penalty = MIN_RECOIL;

    while( true ) {
        double adj = aim_per_move( gun, penalty );
        if( adj <= 0 ) {
            break;
        }
        penalty -= adj;
        mv++;
    }

    return mv;
}

bool player::handle_gun_damage( item &it )
{
    if( !it.is_gun() ) {
        debugmsg( "Tried to handle_gun_damage of a non-gun %s", it.tname().c_str() );
        return false;
    }

    const auto &curammo_effects = it.ammo_effects();
    const islot_gun *firing = it.type->gun.get();
    // Here we check if we're underwater and whether we should misfire.
    // As a result this causes no damage to the firearm, note that some guns are waterproof
    // and so are immune to this effect, note also that WATERPROOF_GUN status does not
    // mean the gun will actually be accurate underwater.
    if (is_underwater() && !it.has_flag("WATERPROOF_GUN") && one_in(firing->durability)) {
        add_msg_player_or_npc(_("Your %s misfires with a wet click!"),
                              _("<npcname>'s %s misfires with a wet click!"),
                              it.tname().c_str());
        return false;
        // Here we check for a chance for the weapon to suffer a mechanical malfunction.
        // Note that some weapons never jam up 'NEVER_JAMS' and thus are immune to this
        // effect as current guns have a durability between 5 and 9 this results in
        // a chance of mechanical failure between 1/64 and 1/1024 on any given shot.
        // the malfunction may cause damage, but never enough to push the weapon beyond 'shattered'
    } else if ((one_in(2 << firing->durability)) && !it.has_flag("NEVER_JAMS")) {
        add_msg_player_or_npc(_("Your %s malfunctions!"),
                              _("<npcname>'s %s malfunctions!"),
                              it.tname().c_str());
        if( it.damage() < it.max_damage() && one_in( 4 * firing->durability ) ) {
            add_msg_player_or_npc(m_bad, _("Your %s is damaged by the mechanical malfunction!"),
                                  _("<npcname>'s %s is damaged by the mechanical malfunction!"),
                                  it.tname().c_str());
            // Don't increment until after the message
            it.inc_damage();
        }
        return false;
        // Here we check for a chance for the weapon to suffer a misfire due to
        // using OEM bullets. Note that these misfires cause no damage to the weapon and
        // some types of ammunition are immune to this effect via the NEVER_MISFIRES effect.
    } else if (!curammo_effects.count("NEVER_MISFIRES") && one_in(1728)) {
        add_msg_player_or_npc(_("Your %s misfires with a dry click!"),
                              _("<npcname>'s %s misfires with a dry click!"),
                              it.tname().c_str());
        return false;
        // Here we check for a chance for the weapon to suffer a misfire due to
        // using player-made 'RECYCLED' bullets. Note that not all forms of
        // player-made ammunition have this effect the misfire may cause damage, but never
        // enough to push the weapon beyond 'shattered'.
    } else if (curammo_effects.count("RECYCLED") && one_in(256)) {
        add_msg_player_or_npc(_("Your %s misfires with a muffled click!"),
                              _("<npcname>'s %s misfires with a muffled click!"),
                              it.tname().c_str());
        if( it.damage() < it.max_damage() && one_in( firing->durability ) ) {
            add_msg_player_or_npc(m_bad, _("Your %s is damaged by the misfired round!"),
                                  _("<npcname>'s %s is damaged by the misfired round!"),
                                  it.tname().c_str());
            // Don't increment until after the message
            it.inc_damage();
        }
        return false;
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
    if( !gun.has_flag( "VEHICLE" ) && gun.get_gun_ups_drain() > 0 ) {
        shots = std::min( shots, int( charges_of( "UPS" ) / gun.get_gun_ups_drain() ) );
    }

    if( shots <= 0 ) {
        debugmsg( "Attempted to fire zero or negative shots using %s", gun.tname().c_str() );
    }

    // usage of any attached bipod is dependent upon terrain
    bool bipod = g->m.has_flag_ter_or_furn( "MOUNTABLE", pos() );
    if( !bipod ) {
        auto veh = g->m.veh_at( pos() );
        bipod = veh && veh->has_part( pos(), "MOUNTABLE" );
    }

    // Up to 50% of recoil can be delayed until end of burst dependent upon relevant skill
    ///\EFFECT_PISTOL delays effects of recoil during autoamtic fire
    ///\EFFECT_SMG delays effects of recoil during automatic fire
    ///\EFFECT_RIFLE delays effects of recoil during automatic fire
    ///\EFFECT_SHOTGUN delays effects of recoil during automatic fire
    double absorb = std::min( int( get_skill_level( gun.gun_skill() ) ), MAX_SKILL ) / double( MAX_SKILL * 2 );

    tripoint aim = target;
    int curshot = 0;
    int xp = 0; // experience gain for marksmanship skill
    int hits = 0; // total shots on target
    int delay = 0; // delayed recoil that has yet to be applied
    while( curshot != shots ) {
        if( !handle_gun_damage( gun ) ) {
            break;
        }

        double dispersion = rng_normal( get_weapon_dispersion( gun ) ) + rng_normal( recoil_total() );
        int range = rl_dist( pos(), aim );

        auto shot = projectile_attack( make_gun_projectile( gun ), aim, dispersion );
        curshot++;

        int qty = gun.gun_recoil( *this, bipod );
        delay  += qty * absorb;
        recoil += qty * ( 1.0 - absorb );

        make_gun_sound_effect( *this, shots > 1, &gun );
        sfx::generate_gun_sound( *this, gun );

        cycle_action( gun, pos() );

        if( gun.ammo_consume( gun.ammo_required(), pos() ) != gun.ammo_required() ) {
            debugmsg( "Unexpected shortage of ammo whilst firing %s", gun.tname().c_str() );
            break;
        }

        if( !gun.has_flag( "VEHICLE" ) ) {
            use_charges( "UPS", gun.get_gun_ups_drain() );
        }


        if( shot.missed_by <= .1 ) {
            lifetime_stats()->headshots++; // @todo check head existence for headshot
        }

        if( shot.hit_critter ) {
            hits++;
            if( range > double( get_skill_level( skill_gun ) ) / MAX_SKILL * MAX_RANGE ) {
                xp += range; // shots at sufficient distance train marksmanship
            }
        }

        if( gun.gun_skill() == skill_launcher ) {
            continue; // skip retargeting for launchers
        }

        // If burst firing and we killed the target then try to retarget
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
        }
    }

    // apply delayed recoil
    recoil += delay;

    // Use different amounts of time depending on the type of gun and our skill
    moves -= time_to_fire( *this, *gun.type );

    practice( skill_gun, xp * ( get_skill_level( skill_gun ) + 1 ) );
    if( hits && !xp && one_in( 10 ) ) {
        add_msg_if_player( m_info, _( "You'll need to aim at more distant targets to further improve your marksmanship." ) );
    }

    // launchers train weapon skill for both hits and misses
    practice( gun.gun_skill(), ( gun.gun_skill() == skill_launcher ? curshot : hits ) * 10 );

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

    const int vol = thrown.volume() / 250_ml;

    deviation += rng(0, ((encumb(bp_hand_l) + encumb(bp_hand_r)) + encumb(bp_eyes) + 1) / 10);
    if (vol > 5) {
        deviation += rng(0, 1 + (vol - 5) / 4);
    }
    if (vol == 0) {
        deviation += rng(0, 3);
    }

    ///\EFFECT_STR randomly decreases throwing deviation
    deviation += rng(0, std::max( 0, thrown.weight() / 113 - str_cur ) );
    deviation = std::max( 0, deviation );

    // Rescaling to use the same units as projectile_attack
    const double shot_dispersion = deviation * (.01 / 0.00021666666666666666);
    static const std::set<material_id> ferric = { material_id( "iron" ), material_id( "steel" ) };

    bool do_railgun = has_active_bionic("bio_railgun") &&
                      thrown.made_of_any( ferric );

    // The damage dealt due to item's weight and player's strength
    ///\EFFECT_STR increases throwing damage
    int real_dam = ( (thrown.weight() / 452)
                     + (thrown.damage_melee(DT_BASH) / 2)
                     + (str_cur / 2) )
                   / (2.0 + (vol / 4.0));
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
                         rng(0, vol + 8) - rng(0, str_cur) < vol;

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

    if( vol > 2 ) {
        proj_effects.insert( "WIDE" );
    }

    // Deal extra cut damage if the item breaks
    if( shatter ) {
        const int glassdam = rng( 0, vol * 2 );
        impact.add_damage( DT_CUT, glassdam );
        proj_effects.insert( "SHATTER_SELF" );
    }

    if( rng(0, 100) < 20 + skill_level * 12 ) {
        int cut = thrown.damage_melee( DT_CUT );
        int stab = thrown.damage_melee( DT_STAB );
        if( cut > 0 || stab > 0 ) {
            proj.impact.add_damage( cut > stab ? DT_CUT : DT_STAB, std::max( cut, stab ) );
        }
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
        const int val = p.recoil_total();
        if( val > MIN_RECOIL ) {
            const char *color_name = "c_ltgray";
            if( val >= 690 ) {
                color_name = "c_red";
            } else if( val >= 450 ) {
                color_name = "c_ltred";
            } else if( val >= 210 ) {
                color_name = "c_yellow";
            }
            return string_format("<color_%s>%s</color>", color_name, _("Recoil"));
        }
    }
    return std::string();
}

// Draws the static portions of the targeting menu,
// returns the number of lines used to draw instructions.
static int draw_targeting_window( WINDOW *w_target, const std::string &name, player &p, target_mode mode,
                                  input_context &ctxt, const std::vector<aim_type> &aim_types,
                                  bool switch_mode, bool switch_ammo )
{
    draw_border(w_target);
    // Draw the "title" of the window.
    mvwprintz(w_target, 0, 2, c_white, "< ");
    std::string title;

    switch( mode ) {
        case TARGET_MODE_FIRE:
        case TARGET_MODE_TURRET_MANUAL:
            title = string_format( _( "Firing %s %s" ), name.c_str(), print_recoil( p ).c_str() );
            break;

        case TARGET_MODE_THROW:
            title = string_format( _( "Throwing %s" ), name.c_str() );
            break;

        default:
            title = _( "Set target" );
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

    // Reserve lines for aiming and firing instructions.
    if( mode == TARGET_MODE_FIRE ) {
        text_y -= ( 3 + aim_types.size() );
    } else {
        text_y -= 2;
    }

    text_y -= switch_mode ? 1 : 0;
    text_y -= switch_ammo ? 1 : 0;

    // The -1 is the -2 from above, but adjusted since this is a total, not an index.
    int lines_used = getmaxy(w_target) - 1 - text_y;
    mvwprintz(w_target, text_y++, 1, c_white, _("Move cursor to target with directional keys"));

    auto const front_or = [&](std::string const &s, char const fallback) {
        auto const keys = ctxt.keys_bound_to(s);
        return keys.empty() ? fallback : keys.front();
    };

    if( mode == TARGET_MODE_FIRE || mode == TARGET_MODE_TURRET_MANUAL ) {
        mvwprintz( w_target, text_y++, 1, c_white, _("%c %c Cycle targets; %c to fire."),
                   front_or("PREV_TARGET", ' '), front_or("NEXT_TARGET", ' '), front_or("FIRE", ' ') );
        mvwprintz( w_target, text_y++, 1, c_white, _("%c target self; %c toggle snap-to-target"),
                   front_or("CENTER", ' ' ), front_or("TOGGLE_SNAP_TO_TARGET", ' ') );
    }

    if( mode == TARGET_MODE_FIRE ) {
        mvwprintz( w_target, text_y++, 1, c_white, _( "%c to steady your aim. " ), front_or( "AIM", ' ' ) );
        for( const auto &e : aim_types ) {
            if( e.has_threshold){
                mvwprintz( w_target, text_y++, 1, c_white, e.help.c_str(), front_or( e.action, ' ') );
            }
        }
        mvwprintz( w_target, text_y++, 1, c_white, _( "%c to switch aiming modes." ), front_or( "SWITCH_AIM", ' ' ) );
    }

    if( switch_mode ) {
        mvwprintz( w_target, text_y++, 1, c_white, _( "%c to switch firing modes." ), front_or( "SWITCH_MODE", ' ' ) );
    }
    if( switch_ammo) {
        mvwprintz( w_target, text_y++, 1, c_white, _( "%c to switch ammo." ), front_or( "SWITCH_AMMO", ' ' ) );
    }

    if( is_mouse_enabled() ) {
        mvwprintz(w_target, text_y++, 1, c_white,
                  _("Mouse: LMB: Target, Wheel: Cycle, RMB: Fire"));
    }
    return lines_used;
}

static int find_target( const std::vector<Creature *> &t, const tripoint &tpos ) {
    for( size_t i = 0; i < t.size(); ++i ) {
        if( t[i]->pos() == tpos ) {
            return int( i );
        }
    }
    return -1;
}

static int do_aim( player &p, const std::vector<Creature *> &t, int cur_target,
                   const item &relevant, const tripoint &tpos )
{
    // If we've changed targets, reset aim, unless it's above the minimum.
    if( size_t( cur_target ) >= t.size() || t[cur_target]->pos() != tpos ) {
        cur_target = find_target( t, tpos );
        // TODO: find radial offset between targets and
        // spend move points swinging the gun around.
        p.recoil = std::max( MIN_RECOIL, p.recoil );
    }

    const double aim_amount = p.aim_per_move( relevant, p.recoil );
    if( aim_amount > 0 ) {
        // Increase aim at the cost of moves
        p.mod_moves( -1 );
        p.recoil = std::max( 0.0, p.recoil - aim_amount );
    } else {
        // If aim is already maxed, we're just waiting, so pass the turn.
        p.set_moves( 0 );
    }

    return cur_target;
}

static int print_aim_bars( const player &p, WINDOW *w, int line_number, item *weapon,
                           Creature *target, double predicted_recoil ) {
    // This is absolute accuracy for the player.
    // TODO: push the calculations duplicated from Creature::deal_projectile_attack() and
    // Creature::projectile_attack() into shared methods.
    // Dodge is intentionally not accounted for.

    // Confidence is chance of the actual shot being under the target threshold,
    // This simplifies the calculation greatly, that's intentional.
    const double aim_level = p.get_weapon_dispersion( *weapon ) + predicted_recoil + p.recoil_vehicle();
    const double range = rl_dist( p.pos(), target->pos() );
    const double missed_by = aim_level * 0.00021666666666666666 * range;
    const double hit_rating = missed_by;
    const double confidence = 1 / hit_rating;
    // This is a relative measure of how steady the player's aim is,
    // 0 it is the best the player can do.
    const double steady_score = predicted_recoil - p.effective_dispersion( p.weapon.sight_dispersion() );
    // Fairly arbitrary cap on steadiness...
    const double steadiness = 1.0 - steady_score / MIN_RECOIL;

    const std::array<std::pair<double, char>, 3> confidence_ratings = {{
        std::make_pair( accuracy_headshot, '*' ),
        std::make_pair( accuracy_goodhit, '+' ),
        std::make_pair( accuracy_grazing, '|' ) }};

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

    // fetch and display list of turrets that are ready to fire at the target
    auto turrets = veh->turrets( targ );

    mvwprintw( w, line_number++, 1, _("Turrets in range: %d"), turrets.size() );
    for( const auto e : turrets ) {
        mvwprintw( w, line_number++, 1, "*  %s", e->name().c_str() );
    }

    return line_number;
}

// TODO: Shunt redundant drawing code elsewhere
std::vector<tripoint> game::pl_target_ui( target_mode mode, item *relevant, int range, const itype *ammo,
                                          const target_callback &on_mode_change,
                                          const target_callback &on_ammo_change )
{
    static const std::vector<tripoint> empty_result{};
    std::vector<tripoint> ret;

    tripoint src = u.pos();
    tripoint dst = u.pos();

    std::vector<Creature *> t;
    int target;

    auto update_targets = [&]( int range, std::vector<Creature *>& targets, int &idx, tripoint &dst ) {
        targets = u.get_targetable_creatures( range );

        targets.erase( std::remove_if( targets.begin(), targets.end(), [&]( const Creature *e ) {
            return u.attitude_to( *e ) == Creature::Attitude::A_FRIENDLY;
        } ), targets.end() );

        if( targets.empty() ) {
            idx = -1;
            return;
        }

        std::sort( targets.begin(), targets.end(), [&]( const Creature *lhs, const Creature *rhs ) {
            return rl_dist( lhs->pos(), u.pos() ) < rl_dist( rhs->pos(), u.pos() );
        } );

        const Creature *last = nullptr;
        if( g->last_target >= 0 ) {
            if( g->last_target_was_npc ) {
                last = size_t( last_target ) < active_npc.size() ? active_npc[ last_target ] : nullptr;
            } else {
                last = size_t( last_target ) < num_zombies() ? &zombie( last_target ) : nullptr;
            }
        }

        auto found = std::find( targets.begin(), targets.end(), last );
        idx = found != targets.end() ? std::distance( targets.begin(), found ) : 0;
        dst = targets[ target ]->pos();
    };

    update_targets( range, t, target, dst );

    bool compact = TERMY < 34;
    int height = compact ? 18 : 25;
    int top = ( compact ? -4 : -1 ) +
              ( use_narrow_sidebar() ? getbegy( w_messages ) : getbegy( w_minimap ) + getmaxy( w_minimap ) );

    WINDOW *w_target = newwin( height, getmaxx( w_messages ), top, getbegx( w_messages ) );

    input_context ctxt("TARGET");
    ctxt.set_iso(true);
    // "ANY_INPUT" should be added before any real help strings
    // Or strings will be written on window border.
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_directions();
    ctxt.register_action( "COORDINATE" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "FIRE" );
    ctxt.register_action( "NEXT_TARGET" );
    ctxt.register_action( "PREV_TARGET" );
    ctxt.register_action( "LEVEL_UP" );
    ctxt.register_action( "LEVEL_DOWN" );
    ctxt.register_action( "CENTER" );
    ctxt.register_action( "TOGGLE_SNAP_TO_TARGET" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "SWITCH_MODE" );
    ctxt.register_action( "SWITCH_AMMO" );

    if( mode == TARGET_MODE_FIRE ) {
        ctxt.register_action( "AIM" );
        ctxt.register_action( "SWITCH_AIM" );
    }

    std::vector<aim_type> aim_types;
    std::vector<aim_type>::iterator aim_mode;
    int sight_dispersion = u.effective_dispersion( u.weapon.sight_dispersion() );

    if( mode == TARGET_MODE_FIRE ) {
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

    int num_instruction_lines = draw_targeting_window( w_target, relevant ? relevant->tname() : "", u,
                                                       mode, ctxt, aim_types,
                                                       bool( on_mode_change ), bool( on_ammo_change ) );

    bool snap_to_target = get_option<bool>( "SNAP_TO_TARGET" );

    std::string enemiesmsg;
    if( t.empty() ) {
        enemiesmsg = _("No targets in range.");
    } else {
        enemiesmsg = string_format(ngettext("%d target in range.", "%d targets in range.",
                                            t.size()), t.size());
    }

    const auto set_last_target = [this]( const tripoint &dst ) {
        if( ( last_target = npc_at( dst ) ) >= 0 ) {
            last_target_was_npc = true;

        } else if( ( last_target = mon_at( dst, true ) ) >= 0 ) {
            last_target_was_npc = false;
        }
    };

    const auto confirm_non_enemy_target = [this]( const tripoint &dst ) {
        if( dst == u.pos() ) {
            return true;
        }
        const int npc_index = npc_at( dst );
        if( npc_index >= 0 ) {
            const npc &who = *active_npc[ npc_index ];
            if( who.is_enemy() ) {
                return true;
            }
            return query_yn( _( "Really attack %s?" ), who.name.c_str() );
        }
        return true;
    };

    const tripoint old_offset = u.view_offset;
    do {
        ret = g->m.find_clear_path( src, dst );

        // This chunk of code handles shifting the aim point around
        // at maximum range when using circular distance.
        // The range > 1 check ensures that you can alweays at least hit adjacent squares.
        if( trigdist && range > 1 && round(trig_dist( src, dst )) > range ) {
            bool cont = true;
            tripoint cp = dst;
            for( size_t i = 0; i < ret.size() && cont; i++ ) {
                if( round(trig_dist( src, ret[i] )) > range ) {
                    ret.resize(i);
                    cont = false;
                } else {
                    cp = ret[i];
                }
            }
            dst = cp;
        }
        tripoint center;
        if( snap_to_target ) {
            center = dst;
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
        Creature *critter = critter_at( dst, true );
        if( dst != src ) {
            // Only draw those tiles which are on current z-level
            auto ret_this_zlevel = ret;
            ret_this_zlevel.erase( std::remove_if( ret_this_zlevel.begin(), ret_this_zlevel.end(),
                [&center]( const tripoint &pt ) { return pt.z != center.z; } ), ret_this_zlevel.end() );
            // Only draw a highlighted trajectory if we can see the endpoint.
            // Provides feedback to the player, and avoids leaking information
            // about tiles they can't see.
            draw_line( dst, center, ret_this_zlevel );

            // Print to target window
            mvwprintw( w_target, line_number++, 1, _( "Range: %d/%d, %s" ),
                      rl_dist( src, dst ), range, enemiesmsg.c_str() );

        } else {
            mvwprintw( w_target, line_number++, 1, _("Range: %d, %s"), range, enemiesmsg.c_str() );
        }

        line_number++;

        if( mode == TARGET_MODE_FIRE || mode == TARGET_MODE_TURRET_MANUAL ) {
            auto m = relevant->gun_current_mode();

            if( relevant != m.target ) {
                mvwprintw( w_target, line_number++, 1, _( "Firing mode: %s %s (%d)" ),
                           m->tname().c_str(), m.mode.c_str(), m.qty );
            } else {
                mvwprintw( w_target, line_number++, 1, _( "Firing mode: %s (%d)" ),
                           m.mode.c_str(), m.qty );
            }

            const itype *cur = ammo ? ammo : m->ammo_data();
            if( cur ) {
                auto str = string_format( m->ammo_remaining() ?
                                          _( "Ammo: <color_%s>%s</color> (%d/%d)" ) :
                                          _( "Ammo: <color_%s>%s</color>" ),
                                          get_all_colors().get_name( cur->color ).c_str(),
                                          cur->nname( std::max( m->ammo_remaining(), 1L ) ).c_str(),
                                          m->ammo_remaining(), m->ammo_capacity() );

                nc_color col = c_ltgray;
                print_colored_text( w_target, line_number++, 1, col, col, str );
            }
            line_number++;
        }

        if( critter && critter != &u && u.sees( *critter ) ) {
            // The 6 is 2 for the border and 4 for aim bars.
            int available_lines = compact ? 1 : ( height - num_instruction_lines - line_number - 6 );
            line_number = critter->print_info( w_target, line_number, available_lines, 1);
        } else {
            mvwputch(w_terrain, POSY + dst.y - center.y, POSX + dst.x - center.x, c_red, '*');
        }

        if( mode == TARGET_MODE_FIRE && critter != nullptr && u.sees( *critter ) ) {
            double predicted_recoil = u.recoil;
            int predicted_delay = 0;
            if( aim_mode->has_threshold && aim_mode->threshold < u.recoil ) {
                do{
                    const double aim_amount = u.aim_per_move( u.weapon, predicted_recoil );
                    if( aim_amount > 0 ) {
                        predicted_delay++;
                        predicted_recoil = std::max( predicted_recoil - aim_amount, 0.0 );
                    }
                } while( predicted_recoil > aim_mode->threshold &&
                          predicted_recoil - sight_dispersion > 0 );
            } else {
                predicted_recoil = u.recoil;
            }

            line_number = print_aim_bars( u, w_target, line_number, &*relevant->gun_current_mode(), critter, predicted_recoil );

            if( aim_mode->has_threshold ) {
                mvwprintw(w_target, line_number++, 1, _("%s Delay: %i"), aim_mode->name.c_str(), predicted_delay );
            }
        } else if( mode == TARGET_MODE_TURRET ) {
            line_number = draw_turret_aim( u, w_target, line_number, dst );
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
            if( !get_option<bool>( "USE_TILES" ) && snap_to_target ) {
                // Snap to target doesn't currently work with tiles.
                targ.x += dst.x - src.x;
                targ.y += dst.y - src.y;
            }
            targ.x -= dst.x;
            targ.y -= dst.y;
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
            dst.z++;
            u.view_offset.z++;
        } else if( g->m.has_zlevels() && action == "LEVEL_DOWN" ) {
            dst.z--;
            u.view_offset.z--;
        }

        /* More drawing to terrain */
        if( targ != tripoint_zero ) {
            const Creature *critter = critter_at( dst, true );
            if( critter != nullptr ) {
                draw_critter( *critter, center );
            } else if( m.pl_sees( dst, -1 ) ) {
                m.drawsq( w_terrain, u, dst, false, true, center );
            } else {
                mvwputch( w_terrain, POSY, POSX, c_black, 'X' );
            }

            // constrain by range
            dst.x = std::min( std::max( dst.x + targ.x, src.x - range ), src.x + range );
            dst.y = std::min( std::max( dst.y + targ.y, src.y - range ), src.y + range );
            dst.z = std::min( std::max( dst.z + targ.z, src.z - range ), src.z + range );

        } else if( (action == "PREV_TARGET") && (target != -1) ) {
            int newtarget = find_target( t, dst ) - 1;
            if( newtarget < 0 ) {
                newtarget = t.size() - 1;
            }
            dst = t[newtarget]->pos();
        } else if( (action == "NEXT_TARGET") && (target != -1) ) {
            int newtarget = find_target( t, dst ) + 1;
            if( newtarget == (int)t.size() ) {
                newtarget = 0;
            }
            dst = t[newtarget]->pos();
        } else if( (action == "AIM") && target != -1 ) {
            // No confirm_non_enemy_target here because we have not initiated the firing.
            // Aiming can be stopped / aborted at any time.
            for( int i = 0; i != 10; ++i ) {
                target = do_aim( u, t, target, *relevant, dst );
            }
            if( u.moves <= 0 ) {
                // We've run out of moves, clear target vector, but leave target selected.
                u.assign_activity( ACT_AIM, 0, 0 );
                u.activity.str_values.push_back( "AIM" );
                u.view_offset = old_offset;
                set_last_target( dst );
                return empty_result;
            }
        } else if( action == "SWITCH_MODE" ) {
            if( on_mode_change ) {
                ammo = on_mode_change( relevant );
            }
        } else if( action == "SWITCH_AMMO" ) {
            if( on_ammo_change ) {
                ammo = on_ammo_change( relevant );
            }
        } else if( action == "SWITCH_AIM" ) {
            aim_mode++;
            if( aim_mode == aim_types.end() ) {
                aim_mode = aim_types.begin();
            }
        } else if( (action == "AIMED_SHOT" || action == "CAREFUL_SHOT" || action == "PRECISE_SHOT") &&
                   target != -1 ) {
            // This action basically means "FIRE" as well, the actual firing may be delayed
            // through aiming, but there is usually no means to stop it. Therefor we query here.
            if( !confirm_non_enemy_target( dst ) ) {
                continue;
            }
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
                target = do_aim( u, t, target, *relevant, dst );
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
                delwin( w_target );
                u.view_offset = old_offset;
                set_last_target( dst );
                return ret;

            } else {
                // We've run out of moves, set the activity to aim so we'll
                // automatically re-enter the targeting menu next turn.
                // Set the string value of the aim action to the right thing
                // so we re-enter this loop.
                // Also clear target vector, but leave target selected.
                u.assign_activity( ACT_AIM, 0, 0 );
                u.activity.str_values.push_back( action );
                u.view_offset = old_offset;
                set_last_target( dst );
                return empty_result;
            }
        } else if( action == "FIRE" ) {
            if( !confirm_non_enemy_target( dst ) ) {
                continue;
            }
            target = find_target( t, dst );
            if( src == dst ) {
                ret.clear();
            }
            break;
        } else if( action == "CENTER" ) {
            dst = src;
            ret.clear();
        } else if( action == "TOGGLE_SNAP_TO_TARGET" ) {
            snap_to_target = !snap_to_target;
        } else if (action == "QUIT") { // return empty vector (cancel)
            ret.clear();
            target = -1;
            break;
        }
    } while (true);

    delwin( w_target );
    u.view_offset = old_offset;

    if( ret.empty() ) {
        return ret;
    }

    set_last_target( ret.back() );

    if( last_target >= 0 && last_target_was_npc ) {
        if( !active_npc[ last_target ]->is_enemy() ) {
            // TODO: get rid of this. Or combine it with effect_hit_by_player
            active_npc[ last_target ]->hit_by_player = true; // used for morale penalty
        }
        // TODO: should probably go into the on-hit code?
        active_npc[ last_target ]->make_angry();

    } else if( last_target >= 0 && !last_target_was_npc ) {
        // TODO: get rid of this. Or move into the on-hit code?
        zombie( last_target ).add_effect( effect_hit_by_player, 100 );
    }

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
        fx.count( "SHOT" ) || fx.count( "BOUNCE" ) ) {
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

        const auto ammo = gun.ammo_data()->ammo.get();
        if( ammo->drop != "null" && x_in_y( ammo->drop_chance, 1.0 ) ) {
            item drop( ammo->drop );
            if( ammo->drop_active ) {
                drop.activate();
            }
            proj.set_drop( drop );
        }

        if( fx.count( "CUSTOM_EXPLOSION" ) > 0  ) {
            proj.set_custom_explosion( gun.ammo_data()->explosion );
        }
    }

    return proj;
}

int time_to_fire( const Character &p, const itype &firingt )
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

static void cycle_action( item& weap, const tripoint &pos ) {
    // eject casings and linkages in random direction avoiding walls using player position as fallback
    auto tiles = closest_tripoints_first( 1, pos );
    tiles.erase( tiles.begin() );
    tiles.erase( std::remove_if( tiles.begin(), tiles.end(), [&]( const tripoint& e ) {
        return !g->m.passable( e );
    } ), tiles.end() );
    tripoint eject = tiles.empty() ? pos : random_entry( tiles );

    // for turrets try and drop casings or linkages directly to any CARGO part on the same tile
    auto veh = g->m.veh_at( pos );
    std::vector<vehicle_part *> cargo;
    if( veh && weap.has_flag( "VEHICLE" ) ) {
        cargo = veh->get_parts( pos, "CARGO" );
    }

    if( weap.ammo_data() && weap.ammo_data()->ammo->casing != "null" ) {
        if( weap.has_flag( "RELOAD_EJECT" ) || weap.gunmod_find( "brass_catcher" ) ) {
            weap.emplace_back( weap.ammo_data()->ammo->casing, calendar::turn, 1 );

        } else {
            item casing( weap.ammo_data()->ammo->casing, calendar::turn, 1 );
            if( cargo.empty() ) {
                g->m.add_item_or_charges( eject, casing );
            } else {
                veh->add_item( *cargo.front(), casing );
            }

            sfx::play_variant_sound( "fire_gun", "brass_eject", sfx::get_heard_volume( eject ),
                                     sfx::get_heard_angle( eject ) );
        }
    }

    // some magazines also eject disintegrating linkages
    const auto mag = weap.magazine_current();
    if( mag && mag->type->magazine->linkage != "NULL" ) {
        item linkage( mag->type->magazine->linkage, calendar::turn, 1 );
        if( cargo.empty() ) {
            g->m.add_item_or_charges( eject, linkage );
        } else {
            veh->add_item( *cargo.front(), linkage );
        }
    }
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

    if( ammo_type() == ammotype( "40mm" ) ) {
        return { 8, _( "Thunk!" ) };

    } else if( typeId() == "hk_g80") {
        return { 24, _( "tz-CRACKck!" ) };

    } else if( ammo_type() == ammotype( "gasoline" ) || ammo_type() == ammotype( "66mm" ) ||
               ammo_type() == ammotype( "84x246mm" ) || ammo_type() == ammotype( "m235" ) ) {
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

static bool is_driving( const player &p )
{
    const auto veh = g->m.veh_at( p.pos() );
    return veh && veh->velocity != 0 && veh->player_in_control( p );
}


// utility functions for projectile_attack
double player::get_weapon_dispersion( const item &obj ) const
{
    double dispersion = obj.gun_dispersion();

    ///\EFFECT_GUN improves usage of accurate weapons and sights
    dispersion += 10 * ( MAX_SKILL - std::min( int( get_skill_level( skill_gun ) ), MAX_SKILL ) );

    if( is_driving( *this ) ) {
        // get volume of gun (or for auxiliary gunmods the parent gun)
        const item *parent = has_item( obj ) ? find_parent( obj ) : nullptr;
        const int vol = ( parent ? parent->volume() : obj.volume() ) / 250_ml;

        ///\EFFECT_DRIVING reduces the inaccuracy penalty when using guns whilst driving
        dispersion += std::max( vol - get_skill_level( skill_driving ), 1 ) * 20;
    }

    if (has_bionic("bio_targeting")) {
        dispersion *= 0.75;
    }

    if ((is_underwater() && !obj.has_flag("UNDERWATER_GUN")) ||
        // Range is effectively four times longer when shooting unflagged guns underwater.
        (!is_underwater() && obj.has_flag("UNDERWATER_GUN"))) {
        // Range is effectively four times longer when shooting flagged guns out of water.
        dispersion *= 4;
    }

    return std::max( dispersion, 0.0 );
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
        const units::volume vol = dropped_item.volume();
        embed = embed && ( critter_size > MS_TINY || vol < 250_ml );
        embed = embed && ( critter_size > MS_SMALL || vol < 500_ml );
        // And if we deal enough damage
        // Item volume bumps up the required damage too
        embed = embed &&
                 ( attack.dealt_dam.type_damage( DT_CUT ) / 2 ) +
                   attack.dealt_dam.type_damage( DT_STAB ) >
                     attack.dealt_dam.type_damage( DT_BASH ) +
                     vol * 3 / 250_ml + rng( 0, 5 );
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

double player::gun_value( const item &weap, long ammo ) const
{
    // TODO: Mods
    // TODO: Allow using a specified type of ammo rather than default
    if( weap.type->gun.get() == nullptr ) {
        return 0.0;
    }

    if( ammo <= 0 ) {
        return 0.0;
    }

    const islot_gun& gun = *weap.type->gun.get();
    const itype_id ammo_type = weap.ammo_default( true );
    const itype *def_ammo_i = ammo_type != "NULL" ?
                              item::find_type( ammo_type ) :
                              nullptr;
    if( def_ammo_i != nullptr && def_ammo_i->ammo == nullptr ) {
        debugmsg( "%s is default ammo for gun %s, but lacks ammo data",
                  def_ammo_i->nname( ammo ).c_str(), weap.tname().c_str() );
    }

    float damage_factor = weap.gun_damage( false );
    damage_factor += weap.gun_pierce( false ) / 2.0;

    item tmp = weap;
    tmp.ammo_set( weap.ammo_default() );
    int total_dispersion = get_weapon_dispersion( tmp ) + effective_dispersion( tmp.sight_dispersion() );

    if( def_ammo_i != nullptr && def_ammo_i->ammo != nullptr ) {
        const islot_ammo &def_ammo = *def_ammo_i->ammo;
        damage_factor += def_ammo.damage;
        damage_factor += def_ammo.pierce / 2;
        total_dispersion += def_ammo.dispersion;
    }

    int move_cost = time_to_fire( *this, *weap.type );
    if( gun.clip != 0 && gun.clip < 10 ) {
        // @todo RELOAD_ONE should get a penalty here
        int reload_cost = gun.reload_time + encumb( bp_hand_l ) + encumb( bp_hand_r );
        reload_cost /= gun.clip;
        move_cost += reload_cost;
    }

    // "Medium range" below means 9 tiles, "short range" means 4
    // Those are guarantees (assuming maximum time spent aiming)
    static const std::vector<std::pair<float, float>> dispersion_thresholds = {{
        // Headshots all the time
        { 0.0f, 5.0f },
        // Crit at medium range
        { 100.0f, 4.5f },
        // Crit at short range or good hit at medium
        { 200.0f, 3.5f },
        // OK hits at medium
        { 300.0f, 3.0f },
        // Point blank headshots
        { 450.0f, 2.5f },
        // OK hits at short
        { 700.0f, 1.5f },
        // Glances at medium, crits at point blank
        { 1000.0f, 1.0f },
        // Nothing guaranteed, pure gamble
        { 2000.0f, 0.1f },
    }};

    static const std::vector<std::pair<float, float>> move_cost_thresholds = {{
        { 10.0f, 4.0f },
        { 25.0f, 3.0f },
        { 100.0f, 1.0f },
        { 500.0f, 5.0f },
    }};

    float move_cost_factor = multi_lerp( move_cost_thresholds, move_cost );

    // Penalty for dodging in melee makes the gun unusable in melee
    // Until NPCs get proper kiting, at least
    int melee_penalty = weapon.volume() / 250_ml - get_skill_level( skill_dodge );
    if( melee_penalty <= 0 ) {
        // Dispersion matters less if you can just use the gun in melee
        total_dispersion = std::min<int>( total_dispersion / move_cost_factor, total_dispersion );
    }

    float dispersion_factor = multi_lerp( dispersion_thresholds, total_dispersion );

    float damage_and_accuracy = damage_factor * dispersion_factor;

    // @todo Some better approximation of the ability to keep on shooting
    static const std::vector<std::pair<float, float>> capacity_thresholds = {{
        { 1.0f, 0.5f },
        { 5.0f, 1.0f },
        { 10.0f, 1.5f },
        { 20.0f, 2.0f },
        { 50.0f, 3.0f },
    }};

    // How much until reload
    float capacity = gun.clip > 0 ? std::min<float>( gun.clip, ammo ) : ammo;
    // How much until dry and a new weapon is needed
    capacity += std::min<float>( 1.0, ammo / 20 );
    float capacity_factor = multi_lerp( capacity_thresholds, capacity );

    double gun_value = damage_and_accuracy * capacity_factor;

    add_msg( m_debug, "%s as gun: %.1f total, %.1f dispersion, %.1f damage, %.1f capacity",
             weap.tname().c_str(), gun_value, dispersion_factor, damage_factor,
             capacity_factor );
    return std::max( 0.0, gun_value );
}
