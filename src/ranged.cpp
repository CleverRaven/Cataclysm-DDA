#include "ranged.h"

#include "ballistics.h"
#include "cata_utility.h"
#include "dispersion.h"
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
#include "projectile.h"
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
#include <vector>
#include <string>
#include <cmath>

const skill_id skill_throw( "throw" );
const skill_id skill_gun( "gun" );
const skill_id skill_driving( "driving" );
const skill_id skill_dodge( "dodge" );
const skill_id skill_launcher( "launcher" );

const efftype_id effect_on_roof( "on_roof" );
const efftype_id effect_hit_by_player( "hit_by_player" );

static const trait_id trait_TRIGGERHAPPY( "TRIGGERHAPPY" );
static const trait_id trait_HOLLOW_BONES( "HOLLOW_BONES" );
static const trait_id trait_LIGHT_BONES( "LIGHT_BONES" );

static projectile make_gun_projectile( const item &gun );
int time_to_fire( const Character &p, const itype &firing );
static void cycle_action( item& weap, const tripoint &pos );
void make_gun_sound_effect(player &p, bool burst, item *weapon);
extern bool is_valid_in_w_terrain(int, int);

struct aim_type {
    std::string name;
    std::string action;
    std::string help;
    bool has_threshold;
    int threshold;
};

static double occupied_tile_fraction( m_size target_size )
{
    switch( target_size ) {
        case MS_TINY:
            return 0.1;
        case MS_SMALL:
            return 0.25;
        case MS_MEDIUM:
            return 0.5;
        case MS_LARGE:
            return 0.75;
        case MS_HUGE:
            return 1.0;
    }

    return 0.5;
}

double Creature::ranged_target_size() const
{
    return occupied_tile_fraction( get_size() );
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
    /** @EFFECT_PISTOL delays effects of recoil during autoamtic fire */
    /** @EFFECT_SMG delays effects of recoil during automatic fire */
    /** @EFFECT_RIFLE delays effects of recoil during automatic fire */
    /** @EFFECT_SHOTGUN delays effects of recoil during automatic fire */
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

        dispersion_sources dispersion = get_weapon_dispersion( gun, rl_dist( pos(), aim ) );
        dispersion.add_range( recoil_total() );
        int range = rl_dist( pos(), aim );

        // If this is a vehicle mounted turret, which vehicle is it mounted on?
        const vehicle *in_veh = has_effect( effect_on_roof ) ? g->m.veh_at( pos() ) : nullptr;

        auto shot = projectile_attack( make_gun_projectile( gun ), pos(), aim, dispersion, this, in_veh );
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
            if( range > double( get_skill_level( skill_gun ) ) / MAX_SKILL * RANGE_SOFT_CAP ) {
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
                    return true; /** @EFFECT_GUN increases range of automatic retargeting during burst fire */

                } else if( z->is_dead_state() ) {
                    return true;

                } else if( has_trait( trait_id( "TRIGGERHAPPY" ) ) && one_in( 10 ) ) {
                    return false; // Trigger happy sometimes doesn't care who we shoot

                } else {
                    return attitude_to( *z ) != A_HOSTILE;
                }
            } ), hostiles.end() );

            if( hostiles.empty() || hostiles.front()->is_dead_state() ) {
                break; // We ran out of suitable targets

            } else if( !one_in( 7 - get_skill_level( skill_gun ) ) ) {
                break; /** @EFFECT_GUN increases chance of firing multiple times in a burst */
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

// @todo Method
int throw_cost( const player &c, const item &to_throw )
{
    // Very similar to player::attack_speed
    // @todo Extract into a function?
    // Differences:
    // Dex is more (2x) important for throwing speed
    // At 10 skill, the cost is down to 0.75%, not 0.66%
    const int base_move_cost = to_throw.attack_time() / 2;
    const int throw_skill = std::min<int>( MAX_SKILL, c.get_skill_level( skill_throw ) );
    ///\EFFECT_THROW increases throwing speed
    const int skill_cost = ( int )( base_move_cost * ( 20 - throw_skill ) / 20 );
    ///\EFFECT_DEX increases throwing speed
    const int dexbonus = c.get_dex();
    const int encumbrance_penalty = c.encumb( bp_torso ) +
                                    ( c.encumb( bp_hand_l ) + c.encumb( bp_hand_r ) ) / 2;
    const float stamina_ratio = ( float )c.stamina / c.get_stamina_max();
    const float stamina_penalty = 1.0 + std::max( ( 0.25f - stamina_ratio ) * 4.0f, 0.0f );

    int move_cost = base_move_cost;
    // Stamina penalty only affects base/2 and encumbrance parts of the cost
    move_cost += encumbrance_penalty;
    move_cost *= stamina_penalty;
    move_cost += skill_cost;
    move_cost -= dexbonus;

    if( c.has_trait( trait_LIGHT_BONES ) ) {
        move_cost *= .9;
    }
    if( c.has_trait( trait_HOLLOW_BONES ) ) {
        move_cost *= .8;
    }

    return std::max( 25, move_cost );
}

int Character::throw_dispersion_per_dodge( bool add_encumbrance ) const
{
    // +200 per dodge point at 0 dexterity
    // +100 at 8, +80 at 12, +66.6 at 16, +57 at 20, +50 at 24
    // Each 10 encumbrance on either hand is like -1 dex (can bring penalty to +400 per dodge)
    // Maybe @todo Only use one hand
    const int encumbrance = add_encumbrance ? encumb( bp_hand_l ) + encumb( bp_hand_r ) : 0;
    ///\EFFECT_DEX increases throwing accuracy against targets with good dodge stat
    float effective_dex = 2 + get_dex() / 4.0f - ( encumbrance ) / 40.0f;
    return static_cast<int>( 100.0f / std::max( 1.0f, effective_dex ) );
}

// Perfect situation gives us 1000 dispersion at lvl 0
// This goes down linearly to 250  dispersion at lvl 10
int Character::throwing_dispersion( const item &to_throw, Creature *critter ) const
{
    int weight = to_throw.weight();
    units::volume volume = to_throw.volume();
    if( to_throw.count_by_charges() && to_throw.charges > 1 ) {
        weight /= to_throw.charges;
        volume /= to_throw.charges;
    }

    int throw_difficulty = 1000;
    // 1000 penalty for every liter after the first
    // @todo Except javelin type items
    throw_difficulty += std::max<int>( 0, units::to_milliliter( volume - 1000_ml ) );
    // 1 penalty for gram above str*100 grams (at 0 skill)
    ///\EFFECT_STR decreases throwing dispersion when throwing heavy objects
    throw_difficulty += std::max( 0, weight - get_str() * 100 );

    // Dispersion from difficult throws goes from 100% at lvl 0 to 25% at lvl 10
    ///\EFFECT_THROW increases throwing accuracy
    const int throw_skill = std::min<int>( MAX_SKILL, get_skill_level( skill_throw ) );
    int dispersion = 10 * throw_difficulty / ( 3 * throw_skill + 10 );
    // If the target is a creature, it moves around and ruins aim
    // @todo Inform projectile functions if the attacker actually aims for the critter or just the tile
    if( critter != nullptr ) {
        // It's easier to dodge at close range (thrower needs to adjust more)
        // Dodge x10 at point blank, x5 at 1 dist, then flat
        float effective_dodge = critter->get_dodge() * std::max( 1, 10 - 5 * rl_dist( pos(), critter->pos() ) );
        dispersion += throw_dispersion_per_dodge( true ) * effective_dodge;
    }
    // 1 perception per 1 eye encumbrance
    ///\EFFECT_PER decreases throwing accuracy penalty from eye encumbrance
    dispersion += std::max( 0, ( encumb( bp_eyes ) - get_per() ) * 10 );
    return std::max( 0, dispersion );
}

dealt_projectile_attack player::throw_item( const tripoint &target, const item &to_throw )
{
    // Copy the item, we may alter it before throwing
    item thrown = to_throw;

    const int move_cost = throw_cost( *this, to_throw );
    mod_moves( -move_cost );

    units::volume volume = to_throw.volume();
    int weight = to_throw.weight();

    const int stamina_cost = ( ( weight / 100 ) + 20 ) * -1;
    mod_stat( "stamina", stamina_cost );

    const skill_id &skill_used = skill_throw;
    const int skill_level = std::min<int>( MAX_SKILL, get_skill_level( skill_throw ) );

    // We'll be constructing a projectile
    projectile proj;
    proj.impact = thrown.base_damage_thrown();
    proj.speed = 10 + skill_level;
    auto &impact = proj.impact;
    auto &proj_effects = proj.proj_effects;

    static const std::set<material_id> ferric = { material_id( "iron" ), material_id( "steel" ) };

    bool do_railgun = has_active_bionic( "bio_railgun" ) && thrown.made_of_any( ferric );

    // The damage dealt due to item's weight and player's strength
    // Up to str/2 or weight/100g (lower), so 10 str is 5 damage before multipliers
    // Railgun doubles the effective strength
    ///\EFFECT_STR increases throwing damage
    impact.add_damage( DT_BASH, std::min( weight / 100.0f, do_railgun ? get_str() : ( get_str() / 2.0f ) ) );

    if( thrown.has_flag( "ACT_ON_RANGED_HIT" ) ) {
        proj_effects.insert( "ACT_ON_RANGED_HIT" );
        thrown.active = true;
    }

    // Item will shatter upon landing, destroying the item, dealing damage, and making noise
    /** @EFFECT_STR increases chance of shattering thrown glass items (NEGATIVE) */
    const bool shatter = !thrown.active && thrown.made_of( material_id( "glass" ) ) &&
                         rng( 0, units::to_milliliter( 2000_ml - volume ) ) < get_str() * 100;

    // Add some flags to the projectile
    if( weight > 500 ) {
        proj_effects.insert( "HEAVY_HIT" );
    }

    proj_effects.insert( "NO_ITEM_DAMAGE" );

    if( thrown.active ) {
        // Can't have molotovs embed into mons
        // Mons don't have inventory processing
        proj_effects.insert( "NO_EMBED" );
    }

    if( do_railgun ) {
        proj_effects.insert( "LIGHTNING" );
    }

    if( volume > 500_ml ) {
        proj_effects.insert( "WIDE" );
    }

    // Deal extra cut damage if the item breaks
    if( shatter ) {
        impact.add_damage( DT_CUT, units::to_milliliter( volume ) / 500.0f );
        proj_effects.insert( "SHATTER_SELF" );
    }

    // Some minor (skill/2) armor piercing for skillfull throws
    // Not as much as in melee, though
    for( damage_unit &du : impact.damage_units ) {
        du.res_pen += skill_level / 2.0f;
    }

    // Put the item into the projectile
    proj.set_drop( std::move( thrown ) );
    if( thrown.has_flag( "CUSTOM_EXPLOSION" ) ) {
        proj.set_custom_explosion( thrown.type->explosion );
    }

    float range = rl_dist( pos(), target );
    proj.range = range;
    // Prevent light items from landing immediately
    proj.momentum_loss = std::min( impact.total_damage() / 10.0f, 1.0f );
    int skill_lvl = get_skill_level( skill_used );
    // Avoid awarding tons of xp for lucky throws against hard to hit targets
    const float range_factor = std::min<float>( range, skill_lvl + 3 );
    // We're aiming to get a damaging hit, not just an accurate one - reward proper weapons
    const float damage_factor = 5.0f * sqrt( proj.impact.total_damage() / 5.0f );
    // This should generally have values below ~20*sqrt(skill_lvl)
    const float final_xp_mult = range_factor * damage_factor;

    Creature *critter = g->critter_at( target, true );
    const dispersion_sources dispersion = throwing_dispersion( thrown, critter );
    auto dealt_attack = projectile_attack( proj, pos(), target, dispersion, this );

    const double missed_by = dealt_attack.missed_by;
    if( missed_by <= 0.1 && dealt_attack.hit_critter != nullptr ) {
        practice( skill_used, final_xp_mult, MAX_SKILL );
        // TODO: Check target for existence of head
        lifetime_stats()->headshots++;
    } else if( dealt_attack.hit_critter != nullptr && missed_by > 0.0f ) {
        practice( skill_used, final_xp_mult / ( 1.0f + missed_by ), MAX_SKILL );
    } else {
        // Pure grindy practice - cap gain at lvl 2
        practice( skill_used, 5, 2 );
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

struct confidence_rating {
    double aim_level;
    char symbol;
    std::string label;
};

static int print_ranged_chance( WINDOW *w, int line_number,
                                const std::vector<confidence_rating> &confidence_config,
                                dispersion_sources dispersion, double range, double target_size,
                                double steadiness = 10000.0 )
{
    const int window_width = getmaxx( w ) - 2; // Window width minus borders.
    // This is a rough estimate of accuracy based on a linear distribution across min and max
    // dispersion.  It is highly inaccurate probability-wise, but this is intentional, the player
    // is not doing gaussian integration in their head while aiming.  The result gives the player
    // correct relative measures of chance to hit, and corresponds with the actual distribution at
    // min, max, and mean.
    const double max_lateral_offset = iso_tangent( range, dispersion.max() );
    const double confidence = 1 / ( max_lateral_offset / target_size );

    bool print_steadiness = steadiness < 1000.0;
    if( get_option<std::string>( "ACCURACY_DISPLAY" ) == "numbers" ) {
        int last_chance = 0;
        std::string confidence_s = enumerate_as_string( confidence_config.begin(), confidence_config.end(),
            [&]( const confidence_rating &config ) {
                // @todo Consider not printing 0 chances, but only if you can print something (at least miss 100% or so)
                int chance = std::min<int>( 100, 100.0 * ( config.aim_level * confidence ) ) - last_chance;
                last_chance += chance;
                return string_format( "%s: %3d%%", config.label.c_str(), chance );
            }, false );
        line_number += fold_and_print_from( w, line_number, 1, window_width, 0,
                                            c_white, confidence_s );

        if( print_steadiness ) {
            std::string steadiness_s = string_format( "%s: %d%%", _( "Steadiness" ), (int)( 100.0 * steadiness ) );
            mvwprintw( w, line_number++, 1, "%s", steadiness_s.c_str() );
        }

    } else {
        // Extract pairs from tuples, because get_labeled_bar expects pairs
        std::vector<std::pair<double, char>> confidence_ratings;
        std::transform( confidence_config.begin(), confidence_config.end(), std::back_inserter( confidence_ratings ),
        [&]( const confidence_rating &config ) {
            return std::make_pair( config.aim_level, config.symbol );
        } );

        const std::string &confidence_bar = get_labeled_bar( confidence, window_width, _( "Confidence" ),
                                                             confidence_ratings.begin(),
                                                             confidence_ratings.end() );


        mvwprintw( w, line_number++, 1, _( "Symbols: * = Headshot + = Hit | = Graze" ) );
        mvwprintw( w, line_number++, 1, confidence_bar.c_str() );
        if( print_steadiness ) {
            const std::string &steadiness_bar = get_labeled_bar( steadiness, window_width,
                                                                 _( "Steadiness" ), '*' );
            mvwprintw( w, line_number++, 1, steadiness_bar.c_str() );
        }
    }

    return line_number;
}

static int print_aim( const player &p, WINDOW *w, int line_number, item *weapon,
                      Creature &target, double predicted_recoil ) {
    // This is absolute accuracy for the player.
    // TODO: push the calculations duplicated from Creature::deal_projectile_attack() and
    // Creature::projectile_attack() into shared methods.
    // Dodge doesn't affect gun attacks

    const double range = rl_dist( p.pos(), target.pos() );
    dispersion_sources dispersion = p.get_weapon_dispersion( *weapon, range );
    dispersion.add_range( predicted_recoil );
    dispersion.add_range( p.recoil_vehicle() );

    // This is a relative measure of how steady the player's aim is,
    // 0 it is the best the player can do.
    const double steady_score = predicted_recoil - p.effective_dispersion( p.weapon.sight_dispersion() );
    // Fairly arbitrary cap on steadiness...
    const double steadiness = 1.0 - steady_score / MIN_RECOIL;

    const double target_size = target.ranged_target_size();

    // This could be extracted, to allow more/less verbose displays
    static const std::vector<confidence_rating> confidence_config = {{
        { accuracy_headshot, '*', _( "Headshot" ) },
        { accuracy_goodhit, '+', _( "Hit" ) },
        { accuracy_grazing, '|', _( "Graze" ) }
    }};

    return print_ranged_chance( w, line_number, confidence_config, dispersion, range, target_size, steadiness );
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

static int draw_throw_aim( const player &p, WINDOW *w, int line_number,
                           const item *weapon, const tripoint &target_pos )
{
    Creature *target = g->critter_at( target_pos, true );
    if( target != nullptr && !p.sees( *target ) ) {
        target = nullptr;
    }

    const dispersion_sources dispersion = p.throwing_dispersion( *weapon, target );
    const double range = rl_dist( p.pos(), target_pos );

    const double target_size = target != nullptr ? target->ranged_target_size() : 1.0f;

    static const std::vector<confidence_rating> confidence_config_critter = {{
        { accuracy_headshot, '*', _( "Headshot" ) },
        { accuracy_goodhit, '+', _( "Hit" ) },
        { accuracy_grazing, '|', _( "Graze" ) }
    }};
    static const std::vector<confidence_rating> confidence_config_object = {{
        { accuracy_grazing, '*', _( "Hit" ) }
    }};
    const auto &confidence_config = target != nullptr ? confidence_config_critter : confidence_config_object;

    return print_ranged_chance( w, line_number, confidence_config, dispersion, range, target_size );
}

std::vector<tripoint> target_handler::target_ui( player &pc, const targeting_data &args ) {
    return target_ui( pc, args.mode, args.relevant, args.range,
                      args.ammo, args.on_mode_change, args.on_ammo_change );
}

// @todo: Shunt redundant drawing code elsewhere
std::vector<tripoint> target_handler::target_ui( player &pc, target_mode mode,
                                                 item *relevant, int range, const itype *ammo,
                                                 const target_callback &on_mode_change,
                                                 const target_callback &on_ammo_change )
{
    // @todo: this should return a reference to a static vector which is cleared on each call.
    static const std::vector<tripoint> empty_result{};
    std::vector<tripoint> ret;

    int sight_dispersion = 0;
    if ( !relevant ) {
        relevant = &pc.weapon;
    } else {
        sight_dispersion = pc.effective_dispersion( relevant->sight_dispersion() );
    }

    tripoint src = pc.pos();
    tripoint dst = pc.pos();

    std::vector<Creature *> t;
    int target;

    auto update_targets = [&]( int range, std::vector<Creature *>& targets, int &idx, tripoint &dst ) {
        targets = pc.get_targetable_creatures( range );

        targets.erase( std::remove_if( targets.begin(), targets.end(), [&]( const Creature *e ) {
            return pc.attitude_to( *e ) == Creature::Attitude::A_FRIENDLY;
        } ), targets.end() );

        if( targets.empty() ) {
            idx = -1;
            return;
        }

        std::sort( targets.begin(), targets.end(), [&]( const Creature *lhs, const Creature *rhs ) {
            return rl_dist( lhs->pos(), pc.pos() ) < rl_dist( rhs->pos(), pc.pos() );
        } );

        const Creature *last = nullptr;
        // @todo: last_target should be member of target_handler
        if( g->last_target >= 0 ) {
            if( g->last_target_was_npc ) {
                last = size_t( g->last_target ) < g->active_npc.size() ? g->active_npc[ g->last_target ] : nullptr;
            } else {
                last = size_t( g->last_target ) < g->num_zombies() ? &g->zombie( g->last_target ) : nullptr;
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
              ( use_narrow_sidebar() ? getbegy( g->w_messages ) : getbegy( g->w_minimap ) + getmaxy( g->w_minimap ) );

    WINDOW *w_target = newwin( height, getmaxx( g->w_messages ), top, getbegx( g->w_messages ) );

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

    int num_instruction_lines = draw_targeting_window( w_target, relevant ? relevant->tname() : "",
                                                       pc, mode, ctxt, aim_types,
                                                       bool( on_mode_change ),
                                                       bool( on_ammo_change ) );

    bool snap_to_target = get_option<bool>( "SNAP_TO_TARGET" );

    std::string enemiesmsg;
    if( t.empty() ) {
        enemiesmsg = _("No targets in range.");
    } else {
        enemiesmsg = string_format(ngettext("%d target in range.", "%d targets in range.",
                                            t.size()), t.size());
    }

    const auto set_last_target = [this]( const tripoint &dst ) {
        if( ( g->last_target = g->npc_at( dst ) ) >= 0 ) {
            g->last_target_was_npc = true;

        } else if( ( g->last_target = g->mon_at( dst, true ) ) >= 0 ) {
            g->last_target_was_npc = false;
        }
    };

    const auto confirm_non_enemy_target = [this, &pc]( const tripoint &dst ) {
        if( dst == pc.pos() ) {
            return true;
        }
        const int npc_index = g->npc_at( dst );
        if( npc_index >= 0 ) {
            const npc &who = *g->active_npc[ npc_index ];
            if( who.guaranteed_hostile() ) {
                return true;
            }
            return query_yn( _( "Really attack %s?" ), who.name.c_str() );
        }
        return true;
    };

    const tripoint old_offset = pc.view_offset;
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
            center = pc.pos() + pc.view_offset;
        }
        // Clear the target window.
        for( int i = 1; i <= getmaxy(w_target) - num_instruction_lines - 2; i++ ) {
            // Clear width excluding borders.
            for( int j = 1; j <= getmaxx(w_target) - 2; j++ ) {
                mvwputch( w_target, i, j, c_white, ' ' );
            }
        }
        g->draw_ter(center, true);
        int line_number = 1;
        Creature *critter = g->critter_at( dst, true );
        if( dst != src ) {
            // Only draw those tiles which are on current z-level
            auto ret_this_zlevel = ret;
            ret_this_zlevel.erase( std::remove_if( ret_this_zlevel.begin(), ret_this_zlevel.end(),
                [&center]( const tripoint &pt ) { return pt.z != center.z; } ), ret_this_zlevel.end() );
            // Only draw a highlighted trajectory if we can see the endpoint.
            // Provides feedback to the player, and avoids leaking information
            // about tiles they can't see.
            g->draw_line( dst, center, ret_this_zlevel );

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

        if( critter && critter != &pc && pc.sees( *critter ) ) {
            // The 6 is 2 for the border and 4 for aim bars.
            int available_lines = compact ? 1 : ( height - num_instruction_lines - line_number - 6 );
            line_number = critter->print_info( w_target, line_number, available_lines, 1);
        } else {
            mvwputch(g->w_terrain, POSY + dst.y - center.y, POSX + dst.x - center.x, c_red, '*');
        }

        if( mode == TARGET_MODE_FIRE && critter != nullptr && pc.sees( *critter ) ) {
            double predicted_recoil = pc.recoil;
            int predicted_delay = 0;
            if( aim_mode->has_threshold && aim_mode->threshold < pc.recoil ) {
                do{
                    const double aim_amount = pc.aim_per_move( *relevant, predicted_recoil );
                    if( aim_amount > 0 ) {
                        predicted_delay++;
                        predicted_recoil = std::max( predicted_recoil - aim_amount, 0.0 );
                    }
                } while( predicted_recoil > aim_mode->threshold &&
                          predicted_recoil - sight_dispersion > 0 );
            } else {
                predicted_recoil = pc.recoil;
            }

            line_number = print_aim( pc, w_target, line_number, &*relevant->gun_current_mode(), *critter, predicted_recoil );

            if( aim_mode->has_threshold ) {
                mvwprintw(w_target, line_number++, 1, _("%s Delay: %i"), aim_mode->name.c_str(), predicted_delay );
            }
        } else if( mode == TARGET_MODE_TURRET ) {
            line_number = draw_turret_aim( pc, w_target, line_number, dst );
        } else if( mode == TARGET_MODE_THROW ) {
            line_number = draw_throw_aim( pc, w_target, line_number, relevant, dst );
        }

        wrefresh(w_target);
        wrefresh(g->w_terrain);
        refresh();

        std::string action;
        if( pc.activity.id() == activity_id( "ACT_AIM" ) && pc.activity.str_values[0] != "AIM" ) {
            // If we're in 'aim and shoot' mode,
            // skip retrieving input and go straight to the action.
            action = pc.activity.str_values[0];
        } else {
            action = ctxt.handle_input();
        }
        // Clear the activity if any, we'll re-set it later if we need to.
        pc.cancel_activity();

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
            pc.view_offset.z++;
        } else if( g->m.has_zlevels() && action == "LEVEL_DOWN" ) {
            dst.z--;
            pc.view_offset.z--;
        }

        /* More drawing to terrain */
        if( targ != tripoint_zero ) {
            const Creature *critter = g->critter_at( dst, true );
            if( critter != nullptr ) {
                g->draw_critter( *critter, center );
            } else if( g->m.pl_sees( dst, -1 ) ) {
                g->m.drawsq( g->w_terrain, pc, dst, false, true, center );
            } else {
                mvwputch( g->w_terrain, POSY, POSX, c_black, 'X' );
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
                target = do_aim( pc, t, target, *relevant, dst );
            }
            if( pc.moves <= 0 ) {
                // We've run out of moves, clear target vector, but leave target selected.
                pc.assign_activity( activity_id( "ACT_AIM" ), 0, 0 );
                pc.activity.str_values.push_back( "AIM" );
                pc.view_offset = old_offset;
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
                target = do_aim( pc, t, target, *relevant, dst );
            } while( target != -1 && pc.moves > 0 && pc.recoil > aim_threshold &&
                     pc.recoil - sight_dispersion > 0 );
            if( target == -1 ) {
                // Bail out if there's no target.
                continue;
            }
            if( pc.recoil <= aim_threshold ||
                pc.recoil - sight_dispersion == 0) {
                // If we made it under the aim threshold, go ahead and fire.
                // Also fire if we're at our best aim level already.
                delwin( w_target );
                pc.view_offset = old_offset;
                set_last_target( dst );
                return ret;

            } else {
                // We've run out of moves, set the activity to aim so we'll
                // automatically re-enter the targeting menu next turn.
                // Set the string value of the aim action to the right thing
                // so we re-enter this loop.
                // Also clear target vector, but leave target selected.
                pc.assign_activity( activity_id( "ACT_AIM" ), 0, 0 );
                pc.activity.str_values.push_back( action );
                pc.view_offset = old_offset;
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
    pc.view_offset = old_offset;

    if( ret.empty() ) {
        return ret;
    }

    set_last_target( ret.back() );

    if( g->last_target >= 0 && g->last_target_was_npc ) {
        if( !g->active_npc[ g->last_target ]->guaranteed_hostile() ) {
            // TODO: get rid of this. Or combine it with effect_hit_by_player
            g->active_npc[ g->last_target ]->hit_by_player = true; // used for morale penalty
        }
        // TODO: should probably go into the on-hit code?
        g->active_npc[ g->last_target ]->make_angry();

    } else if( g->last_target >= 0 && !g->last_target_was_npc ) {
        // TODO: get rid of this. Or move into the on-hit code?
        g->zombie( g->last_target ).add_effect( effect_hit_by_player, 100 );
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
            weap.contents.push_back( item( weap.ammo_data()->ammo->casing ).set_flag( "CASING" ) );

        } else {
            if( cargo.empty() ) {
                g->m.add_item_or_charges( eject, item( weap.ammo_data()->ammo->casing ) );
            } else {
                veh->add_item( *cargo.front(), item( weap.ammo_data()->ammo->casing ) );
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
        // Grenade launchers
        return { 8, _( "Thunk!" ) };

    } else if( ammo_type() == ammotype( "12mm" ) || ammo_type() == ammotype( "metal_rail" ) ) {
        // Railguns
        return { 24, _( "tz-CRACKck!" ) };

    } else if( ammo_type() == ammotype( "flammable" ) || ammo_type() == ammotype( "66mm" ) ||
               ammo_type() == ammotype( "84x246mm" ) || ammo_type() == ammotype( "m235" ) ) {
        // Rocket launchers and flamethrowers
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
        } else if( noise < 175 ) {
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
dispersion_sources player::get_weapon_dispersion( const item &obj, float range ) const
{
    dispersion_sources dispersion = obj.gun_dispersion();

    /** @EFFECT_GUN improves usage of accurate weapons and sights */
    dispersion.add_range( 10 * ( MAX_SKILL - std::min( int( get_skill_level( skill_gun ) ), MAX_SKILL ) ) );

    if( is_driving( *this ) ) {
        // get volume of gun (or for auxiliary gunmods the parent gun)
        const item *parent = has_item( obj ) ? find_parent( obj ) : nullptr;
        const int vol = ( parent ? parent->volume() : obj.volume() ) / 250_ml;

        /** @EFFECT_DRIVING reduces the inaccuracy penalty when using guns whilst driving */
        dispersion.add_range( std::max( vol - get_skill_level( skill_driving ), 1 ) * 20 );
    }

    if( has_bionic( "bio_targeting" ) ) {
        dispersion.add_multiplier( 0.75 );
    }

    if( ( is_underwater() && !obj.has_flag( "UNDERWATER_GUN" ) ) ||
        // Range is effectively four times longer when shooting unflagged guns underwater.
        ( !is_underwater() && obj.has_flag( "UNDERWATER_GUN" ) ) ) {
        // Range is effectively four times longer when shooting flagged guns out of water.
        dispersion.add_multiplier( 4 );
    }

    // @todo Scale the range penalty with something (but don't allow it to get below 2.0*range)
    if( range > RANGE_SOFT_CAP ) {
        dispersion.add_range( ( range - RANGE_SOFT_CAP ) * 3.0f );
    }

    return dispersion;
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

    float damage_factor = weap.gun_damage( false );
    damage_factor += weap.gun_pierce( false ) / 2.0;

    item tmp = weap;
    tmp.ammo_set( weap.ammo_default() );
    int total_dispersion = get_weapon_dispersion( tmp, RANGE_SOFT_CAP ).max() +
      effective_dispersion( tmp.sight_dispersion() );

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
