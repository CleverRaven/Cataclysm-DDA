#include "ranged.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "ballistics.h"
#include "cata_utility.h"
#include "debug.h"
#include "dispersion.h"
#include "game.h"
#include "gun_mode.h"
#include "input.h"
#include "item.h"
#include "itype.h"
#include "line.h"
#include "map.h"
#include "messages.h"
#include "monster.h"
#include "morale_types.h"
#include "mtype.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "projectile.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "trap.h"

const skill_id skill_throw( "throw" );
const skill_id skill_gun( "gun" );
const skill_id skill_driving( "driving" );
const skill_id skill_dodge( "dodge" );
const skill_id skill_launcher( "launcher" );

const efftype_id effect_on_roof( "on_roof" );
const efftype_id effect_hit_by_player( "hit_by_player" );

static const trait_id trait_PYROMANIA( "PYROMANIA" );

const trap_str_id tr_practice_target( "tr_practice_target" );

static projectile make_gun_projectile( const item &gun );
int time_to_fire( const Character &p, const itype &firing );
static void cycle_action( item &weap, const tripoint &pos );
void make_gun_sound_effect( const player &p, bool burst, item *weapon );
extern bool is_valid_in_w_terrain( int, int );

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
    if( has_flag( MF_HARDTOSHOOT ) ) {
        switch( get_size() ) {
            case MS_TINY:
                return occupied_tile_fraction( MS_TINY );
            case MS_SMALL:
                return occupied_tile_fraction( MS_TINY );
            case MS_MEDIUM:
                return occupied_tile_fraction( MS_SMALL );
            case MS_LARGE:
                return occupied_tile_fraction( MS_MEDIUM );
            case MS_HUGE:
                return occupied_tile_fraction( MS_LARGE );
        }
    }
    return occupied_tile_fraction( get_size() );
}

int range_with_even_chance_of_good_hit( int dispersion )
{
    // Empirically determined by "synthetic_range_test" in tests/ranged_balance.cpp.
    static const std::array<int, 59> dispersion_for_even_chance_of_good_hit = {{
            1731, 859, 573, 421, 341, 286, 245, 214, 191, 175,
            151, 143, 129, 118, 114, 107, 101, 94, 90, 78,
            78, 78, 74, 71, 68, 66, 62, 61, 59, 57,
            46, 46, 46, 46, 46, 46, 45, 45, 44, 42,
            41, 41, 39, 39, 38, 37, 36, 35, 34, 34,
            33, 33, 32, 30, 30, 30, 30, 29, 28
        }
    };
    int even_chance_range = 0;
    while( static_cast<unsigned>( even_chance_range ) <
           dispersion_for_even_chance_of_good_hit.size() &&
           dispersion < dispersion_for_even_chance_of_good_hit[ even_chance_range ] ) {
        even_chance_range++;
    }
    return even_chance_range;
}

int player::gun_engagement_moves( const item &gun, int target, int start ) const
{
    int mv = 0;
    double penalty = start;

    while( penalty > target ) {
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
    const cata::optional<islot_gun> &firing = it.type->gun;
    // Here we check if we're underwater and whether we should misfire.
    // As a result this causes no damage to the firearm, note that some guns are waterproof
    // and so are immune to this effect, note also that WATERPROOF_GUN status does not
    // mean the gun will actually be accurate underwater.
    if( is_underwater() && !it.has_flag( "WATERPROOF_GUN" ) && one_in( firing->durability ) ) {
        add_msg_player_or_npc( _( "Your %s misfires with a wet click!" ),
                               _( "<npcname>'s %s misfires with a wet click!" ),
                               it.tname().c_str() );
        return false;
        // Here we check for a chance for the weapon to suffer a mechanical malfunction.
        // Note that some weapons never jam up 'NEVER_JAMS' and thus are immune to this
        // effect as current guns have a durability between 5 and 9 this results in
        // a chance of mechanical failure between 1/64 and 1/1024 on any given shot.
        // the malfunction may cause damage, but never enough to push the weapon beyond 'shattered'
    } else if( ( one_in( 2 << firing->durability ) ) && !it.has_flag( "NEVER_JAMS" ) ) {
        add_msg_player_or_npc( _( "Your %s malfunctions!" ),
                               _( "<npcname>'s %s malfunctions!" ),
                               it.tname().c_str() );
        if( it.damage() < it.max_damage() && one_in( 4 * firing->durability ) ) {
            add_msg_player_or_npc( m_bad, _( "Your %s is damaged by the mechanical malfunction!" ),
                                   _( "<npcname>'s %s is damaged by the mechanical malfunction!" ),
                                   it.tname().c_str() );
            // Don't increment until after the message
            it.inc_damage();
        }
        return false;
        // Here we check for a chance for the weapon to suffer a misfire due to
        // using OEM bullets. Note that these misfires cause no damage to the weapon and
        // some types of ammunition are immune to this effect via the NEVER_MISFIRES effect.
    } else if( !curammo_effects.count( "NEVER_MISFIRES" ) && one_in( 1728 ) ) {
        add_msg_player_or_npc( _( "Your %s misfires with a dry click!" ),
                               _( "<npcname>'s %s misfires with a dry click!" ),
                               it.tname().c_str() );
        return false;
        // Here we check for a chance for the weapon to suffer a misfire due to
        // using player-made 'RECYCLED' bullets. Note that not all forms of
        // player-made ammunition have this effect the misfire may cause damage, but never
        // enough to push the weapon beyond 'shattered'.
    } else if( curammo_effects.count( "RECYCLED" ) && one_in( 256 ) ) {
        add_msg_player_or_npc( _( "Your %s misfires with a muffled click!" ),
                               _( "<npcname>'s %s misfires with a muffled click!" ),
                               it.tname().c_str() );
        if( it.damage() < it.max_damage() && one_in( firing->durability ) ) {
            add_msg_player_or_npc( m_bad, _( "Your %s is damaged by the misfired round!" ),
                                   _( "<npcname>'s %s is damaged by the misfired round!" ),
                                   it.tname().c_str() );
            // Don't increment until after the message
            it.inc_damage();
        }
        return false;
        // Here we check for a chance for attached mods to get damaged if they are flagged as 'CONSUMABLE'.
        // This is mostly for crappy handmade expedient stuff  or things that rarely receive damage during normal usage.
        // Default chance is 1/10000 unless set via json, damage is proportional to caliber(see below).
        // Can be toned down with 'consume_divisor.'

    } else if( it.has_flag( "CONSUMABLE" )  && !curammo_effects.count( "LASER" ) &&
               !curammo_effects.count( "PLASMA" ) ) {
        int uncork = ( ( 10 * it.ammo_data()->ammo->loudness )
                       + ( it.ammo_data()->ammo->recoil / 2 ) ) / 100;
        uncork = std::pow( uncork, 3 ) * 6.5;
        for( auto mod : it.gunmods() ) {
            if( mod->has_flag( "CONSUMABLE" ) ) {
                int dmgamt = uncork / mod->type->gunmod->consume_divisor;
                int modconsume = mod->type->gunmod->consume_chance;
                int initstate = it.damage();
                // fuzz damage if it's small
                if( dmgamt < 1000 ) {
                    dmgamt = rng( dmgamt, dmgamt + 200 );
                    // ignore damage if inconsequential.
                }
                if( dmgamt < 800 ) {
                    dmgamt = 0;
                }
                if( one_in( modconsume ) ) {
                    if( mod->mod_damage( dmgamt ) ) {
                        add_msg_player_or_npc( m_bad,  _( "Your attached %s is destroyed by your shot!" ),
                                               _( "<npcname>'s attached %s is destroyed by their shot!" ),
                                               mod->tname().c_str() );
                        i_rem( mod );
                    } else if( it.damage() > initstate ) {
                        add_msg_player_or_npc( m_bad,  _( "Your attached %s is damaged by your shot!" ),
                                               _( "<npcname>'s %s is damaged by their shot!" ),
                                               mod->tname().c_str() );
                    }
                }
            }
        }
    }
    return true;
}

int player::fire_gun( const tripoint &target, int shots )
{
    return fire_gun( target, shots, weapon );
}

int player::fire_gun( const tripoint &target, int shots, item &gun )
{
    if( !gun.is_gun() ) {
        debugmsg( "%s tried to fire non-gun (%s).", name.c_str(), gun.tname().c_str() );
        return 0;
    }

    // Number of shots to fire is limited by the amount of remaining ammo
    if( gun.ammo_required() ) {
        shots = std::min( shots, static_cast<int>( gun.ammo_remaining() / gun.ammo_required() ) );
    }

    // cap our maximum burst size by the amount of UPS power left
    if( !gun.has_flag( "VEHICLE" ) && gun.get_gun_ups_drain() > 0 ) {
        shots = std::min( shots, static_cast<int>( charges_of( "UPS" ) / gun.get_gun_ups_drain() ) );
    }

    if( shots <= 0 ) {
        debugmsg( "Attempted to fire zero or negative shots using %s", gun.tname().c_str() );
    }

    // usage of any attached bipod is dependent upon terrain
    bool bipod = g->m.has_flag_ter_or_furn( "MOUNTABLE", pos() );
    if( !bipod ) {
        if( const optional_vpart_position vp = g->m.veh_at( pos() ) ) {
            bipod = vp->vehicle().has_part( pos(), "MOUNTABLE" );
        }
    }

    // Up to 50% of recoil can be delayed until end of burst dependent upon relevant skill
    /** @EFFECT_PISTOL delays effects of recoil during automatic fire */
    /** @EFFECT_SMG delays effects of recoil during automatic fire */
    /** @EFFECT_RIFLE delays effects of recoil during automatic fire */
    /** @EFFECT_SHOTGUN delays effects of recoil during automatic fire */
    double absorb = std::min( get_skill_level( gun.gun_skill() ), MAX_SKILL ) / double( MAX_SKILL * 2 );

    tripoint aim = target;
    int curshot = 0;
    int hits = 0; // total shots on target
    int delay = 0; // delayed recoil that has yet to be applied
    while( curshot != shots ) {
        if( !handle_gun_damage( gun ) ) {
            break;
        }

        dispersion_sources dispersion = get_weapon_dispersion( gun );
        dispersion.add_range( recoil_total() );

        // If this is a vehicle mounted turret, which vehicle is it mounted on?
        const vehicle *in_veh = has_effect( effect_on_roof ) ? veh_pointer_or_null( g->m.veh_at(
                                    pos() ) ) : nullptr;

        auto shot = projectile_attack( make_gun_projectile( gun ), pos(), aim, dispersion, this, in_veh );
        curshot++;

        int qty = gun.gun_recoil( *this, bipod );
        delay  += qty * absorb;
        // Temporarily scale by 5x as we adjust MAX_RECOIL.
        recoil += 5.0 * ( qty * ( 1.0 - absorb ) );

        make_gun_sound_effect( *this, shots > 1, &gun );
        sfx::generate_gun_sound( *this, gun );

        cycle_action( gun, pos() );

        if( has_trait( trait_PYROMANIA ) && !has_morale( MORALE_PYROMANIA_STARTFIRE ) ) {
            if( gun.ammo_type() == ammotype( "flammable" ) || gun.ammo_type() == ammotype( "66mm" ) ||
                gun.ammo_type() == ammotype( "84x246mm" ) || gun.ammo_type() == ammotype( "m235" ) ) {
                add_msg_if_player( m_good, _( "You feel a surge of euphoria as flames roar out of the %s!" ),
                                   gun.tname() );
                add_morale( MORALE_PYROMANIA_STARTFIRE, 15, 15, 8_hours, 6_hours );
                rem_morale( MORALE_PYROMANIA_NOFIRE );
            }
        }

        if( gun.ammo_consume( gun.ammo_required(), pos() ) != gun.ammo_required() ) {
            debugmsg( "Unexpected shortage of ammo whilst firing %s", gun.tname().c_str() );
            break;
        }

        if( !gun.has_flag( "VEHICLE" ) ) {
            use_charges( "UPS", gun.get_gun_ups_drain() );
        }

        if( shot.missed_by <= .1 ) {
            lifetime_stats.headshots++; // TODO: check head existence for headshot
        }

        if( shot.hit_critter ) {
            hits++;
        }

        if( gun.gun_skill() == skill_launcher ) {
            continue; // skip retargeting for launchers
        }
    }

    // apply delayed recoil
    recoil += delay;
    // Reset aim for bows and other reload-and-shoot weapons.
    if( gun.has_flag( "RELOAD_AND_SHOOT" ) ) {
        recoil = MAX_RECOIL;
    }
    // Cap
    recoil = std::min( MAX_RECOIL, recoil );

    // Reset last target pos
    last_target_pos = cata::nullopt;

    // Use different amounts of time depending on the type of gun and our skill
    moves -= time_to_fire( *this, *gun.type );

    // Practice the base gun skill proportionally to number of hits, but always by one.
    practice( skill_gun, ( hits + 1 ) * 5 );
    // launchers train weapon skill for both hits and misses.
    int practice_units = gun.gun_skill() == skill_launcher ? curshot : hits;
    practice( gun.gun_skill(), ( practice_units + 1 ) * 5 );

    return curshot;
}

// TODO: Method
int throw_cost( const player &c, const item &to_throw )
{
    // Very similar to player::attack_speed
    // TODO: Extract into a function?
    // Differences:
    // Dex is more (2x) important for throwing speed
    // At 10 skill, the cost is down to 0.75%, not 0.66%
    const int base_move_cost = to_throw.attack_time() / 2;
    const int throw_skill = std::min( MAX_SKILL, c.get_skill_level( skill_throw ) );
    ///\EFFECT_THROW increases throwing speed
    const int skill_cost = static_cast<int>( ( base_move_cost * ( 20 - throw_skill ) / 20 ) );
    ///\EFFECT_DEX increases throwing speed
    const int dexbonus = c.get_dex();
    const int encumbrance_penalty = c.encumb( bp_torso ) +
                                    ( c.encumb( bp_hand_l ) + c.encumb( bp_hand_r ) ) / 2;
    const float stamina_ratio = static_cast<float>( c.stamina ) / c.get_stamina_max();
    const float stamina_penalty = 1.0 + std::max( ( 0.25f - stamina_ratio ) * 4.0f, 0.0f );

    int move_cost = base_move_cost;
    // Stamina penalty only affects base/2 and encumbrance parts of the cost
    move_cost += encumbrance_penalty;
    move_cost *= stamina_penalty;
    move_cost += skill_cost;
    move_cost -= dexbonus;
    move_cost *= c.mutation_value( "attackcost_modifier" );

    return std::max( 25, move_cost );
}

int Character::throw_dispersion_per_dodge( bool add_encumbrance ) const
{
    // +200 per dodge point at 0 dexterity
    // +100 at 8, +80 at 12, +66.6 at 16, +57 at 20, +50 at 24
    // Each 10 encumbrance on either hand is like -1 dex (can bring penalty to +400 per dodge)
    // Maybe TODO: Only use one hand
    const int encumbrance = add_encumbrance ? encumb( bp_hand_l ) + encumb( bp_hand_r ) : 0;
    ///\EFFECT_DEX increases throwing accuracy against targets with good dodge stat
    float effective_dex = 2 + get_dex() / 4.0f - ( encumbrance ) / 40.0f;
    return static_cast<int>( 100.0f / std::max( 1.0f, effective_dex ) );
}

// Perfect situation gives us 1000 dispersion at lvl 0
// This goes down linearly to 250  dispersion at lvl 10
int Character::throwing_dispersion( const item &to_throw, Creature *critter,
                                    bool is_blind_throw ) const
{
    units::mass weight = to_throw.weight();
    units::volume volume = to_throw.volume();
    if( to_throw.count_by_charges() && to_throw.charges > 1 ) {
        weight /= to_throw.charges;
        volume /= to_throw.charges;
    }

    int throw_difficulty = 1000;
    // 1000 penalty for every liter after the first
    // TODO: Except javelin type items
    throw_difficulty += std::max<int>( 0, units::to_milliliter( volume - 1000_ml ) );
    // 1 penalty for gram above str*100 grams (at 0 skill)
    ///\EFFECT_STR decreases throwing dispersion when throwing heavy objects
    throw_difficulty += std::max( 0, weight / 1_gram - get_str() * 100 );

    // Dispersion from difficult throws goes from 100% at lvl 0 to 25% at lvl 10
    ///\EFFECT_THROW increases throwing accuracy
    const int throw_skill = std::min( MAX_SKILL, get_skill_level( skill_throw ) );
    int dispersion = 10 * throw_difficulty / ( 8 * throw_skill + 4 );
    // If the target is a creature, it moves around and ruins aim
    // TODO: Inform projectile functions if the attacker actually aims for the critter or just the tile
    if( critter != nullptr ) {
        // It's easier to dodge at close range (thrower needs to adjust more)
        // Dodge x10 at point blank, x5 at 1 dist, then flat
        float effective_dodge = critter->get_dodge() * std::max( 1, 10 - 5 * rl_dist( pos(),
                                critter->pos() ) );
        dispersion += throw_dispersion_per_dodge( true ) * effective_dodge;
    }
    // 1 perception per 1 eye encumbrance
    ///\EFFECT_PER decreases throwing accuracy penalty from eye encumbrance
    dispersion += std::max( 0, ( encumb( bp_eyes ) - get_per() ) * 10 );

    // If throwing blind, we're assuming they mechanically can't achieve the
    // accuracy of a normal throw.
    if( is_blind_throw ) {
        dispersion *= 4;
    }

    return std::max( 0, dispersion );
}

dealt_projectile_attack player::throw_item( const tripoint &target, const item &to_throw,
        const cata::optional<tripoint> &blind_throw_from_pos )
{
    // Copy the item, we may alter it before throwing
    item thrown = to_throw;

    const int move_cost = throw_cost( *this, to_throw );
    mod_moves( -move_cost );

    units::volume volume = to_throw.volume();
    units::mass weight = to_throw.weight();

    const int stamina_cost = ( ( weight / 100_gram ) + 20 ) * -1;
    mod_stat( "stamina", stamina_cost );

    const skill_id &skill_used = skill_throw;
    const int skill_level = std::min( MAX_SKILL, get_skill_level( skill_throw ) );

    // We'll be constructing a projectile
    projectile proj;
    proj.impact = thrown.base_damage_thrown();
    proj.speed = 10 + skill_level;
    auto &impact = proj.impact;
    auto &proj_effects = proj.proj_effects;

    static const std::set<material_id> ferric = { material_id( "iron" ), material_id( "steel" ) };

    bool do_railgun = has_active_bionic( bionic_id( "bio_railgun" ) ) && thrown.made_of_any( ferric );

    // The damage dealt due to item's weight, player's strength, and skill level
    // Up to str/2 or weight/100g (lower), so 10 str is 5 damage before multipliers
    // Railgun doubles the effective strength
    ///\EFFECT_STR increases throwing damage
    double stats_mod = do_railgun ? get_str() : ( get_str() / 2.0 );
    // modify strength impact based on skill level, clamped to [0.15 - 1]
    // mod = mod * [ ( ( skill / max_skill ) * 0.85 ) + 0.15 ]
    stats_mod *= ( std::min( MAX_SKILL,
                             get_skill_level( skill_throw ) ) /
                   static_cast<double>( MAX_SKILL ) ) * 0.85 + 0.15;
    impact.add_damage( DT_BASH, std::min( weight / 100.0_gram, stats_mod ) );

    if( thrown.has_flag( "ACT_ON_RANGED_HIT" ) ) {
        proj_effects.insert( "ACT_ON_RANGED_HIT" );
        thrown.active = true;
    }

    // Item will shatter upon landing, destroying the item, dealing damage, and making noise
    /** @EFFECT_STR increases chance of shattering thrown glass items (NEGATIVE) */
    const bool shatter = !thrown.active && thrown.made_of( material_id( "glass" ) ) &&
                         rng( 0, units::to_milliliter( 2000_ml - volume ) ) < get_str() * 100;

    // Add some flags to the projectile
    if( weight > 500_gram ) {
        proj_effects.insert( "HEAVY_HIT" );
    }

    proj_effects.insert( "NO_ITEM_DAMAGE" );

    if( thrown.active ) {
        // Can't have Molotovs embed into monsters
        // Monsters don't have inventory processing
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

    // Some minor (skill/2) armor piercing for skillful throws
    // Not as much as in melee, though
    for( damage_unit &du : impact.damage_units ) {
        du.res_pen += skill_level / 2.0f;
    }

    // Put the item into the projectile
    proj.set_drop( std::move( thrown ) );
    if( thrown.has_flag( "CUSTOM_EXPLOSION" ) ) {
        proj.set_custom_explosion( thrown.type->explosion );
    }

    // Throw from the player's position, unless we're blind throwing, in which case
    // throw from the the blind throw position instead.
    const tripoint throw_from = blind_throw_from_pos ? *blind_throw_from_pos : pos();

    float range = rl_dist( throw_from, target );
    proj.range = range;
    int skill_lvl = get_skill_level( skill_used );
    // Avoid awarding tons of xp for lucky throws against hard to hit targets
    const float range_factor = std::min<float>( range, skill_lvl + 3 );
    // We're aiming to get a damaging hit, not just an accurate one - reward proper weapons
    const float damage_factor = 5.0f * sqrt( proj.impact.total_damage() / 5.0f );
    // This should generally have values below ~20*sqrt(skill_lvl)
    const float final_xp_mult = range_factor * damage_factor;

    Creature *critter = g->critter_at( target, true );
    const dispersion_sources dispersion = throwing_dispersion( thrown, critter,
                                          blind_throw_from_pos.has_value() );
    auto dealt_attack = projectile_attack( proj, throw_from, target, dispersion, this );

    const double missed_by = dealt_attack.missed_by;
    if( missed_by <= 0.1 && dealt_attack.hit_critter != nullptr ) {
        practice( skill_used, final_xp_mult, MAX_SKILL );
        // TODO: Check target for existence of head
        lifetime_stats.headshots++;
    } else if( dealt_attack.hit_critter != nullptr && missed_by > 0.0f ) {
        practice( skill_used, final_xp_mult / ( 1.0f + missed_by ), MAX_SKILL );
    } else {
        // Pure grindy practice - cap gain at lvl 2
        practice( skill_used, 5, 2 );
    }
    // Reset last target pos
    last_target_pos = cata::nullopt;

    return dealt_attack;
}

static std::string print_recoil( const player &p )
{
    if( p.weapon.is_gun() ) {
        const int val = p.recoil_total();
        const int min_recoil = p.effective_dispersion( p.weapon.sight_dispersion() );
        const int recoil_range = MAX_RECOIL - min_recoil;
        std::string level;
        if( val >= min_recoil + ( recoil_range * 2 / 3 ) ) {
            level = pgettext( "amount of backward momentum", "High" );
        } else if( val >= min_recoil + ( recoil_range / 2 ) ) {
            level = pgettext( "amount of backward momentum", "Medium" );
        } else if( val >= min_recoil + ( recoil_range / 4 ) ) {
            level = pgettext( "amount of backward momentum", "Low" );
        } else {
            level = pgettext( "amount of backward momentum", "None" );
        }
        return string_format( _( "Recoil: %s" ), level );
    }
    return std::string();
}

// Draws the static portions of the targeting menu,
// returns the number of lines used to draw instructions.
static int draw_targeting_window( const catacurses::window &w_target, const std::string &name,
                                  target_mode mode, input_context &ctxt,
                                  const std::vector<aim_type> &aim_types, bool switch_mode,
                                  bool switch_ammo, bool tiny )
{
    draw_border( w_target );
    // Draw the "title" of the window.
    mvwprintz( w_target, 0, 2, c_white, "< " );
    std::string title;

    switch( mode ) {
        case TARGET_MODE_FIRE:
        case TARGET_MODE_TURRET_MANUAL:
            title = string_format( _( "Firing %s" ), name.c_str() );
            break;

        case TARGET_MODE_THROW:
            title = string_format( _( "Throwing %s" ), name.c_str() );
            break;

        case TARGET_MODE_THROW_BLIND:
            title = string_format( _( "Blind throwing %s" ), name.c_str() );
            break;

        default:
            title = _( "Set target" );
    }

    trim_and_print( w_target, 0, 4, getmaxx( w_target ) - 7, c_red, title );
    wprintz( w_target, c_white, " >" );

    // Draw the help contents at the bottom of the window, leaving room for monster description
    // and aiming status to be drawn dynamically.
    // The - 2 accounts for the window border.
    // If tiny is set we're critically low on space, let the final line overwrite the border.
    int text_y = getmaxy( w_target ) - ( tiny ? 1 : 2 );
    if( is_mouse_enabled() ) {
        // Reserve a line for mouse instructions.
        --text_y;
    }

    // Reserve lines for aiming and firing instructions.
    if( mode == TARGET_MODE_FIRE ) {
        text_y -= ( 1 + aim_types.size() );
    } else if( mode == TARGET_MODE_TURRET_MANUAL || mode == TARGET_MODE_TURRET ) {
        text_y -= 2;
    }

    text_y -= switch_mode ? 1 : 0;
    text_y -= switch_ammo ? 1 : 0;

    // The -1 is the -2 from above, but adjusted since this is a total, not an index.
    int lines_used = getmaxy( w_target ) - 1 - text_y;
    mvwprintz( w_target, text_y++, 1, c_white, _( "Move cursor to target with directional keys" ) );

    const auto front_or = [&]( const std::string & s, const char fallback ) {
        const auto keys = ctxt.keys_bound_to( s );
        return keys.empty() ? fallback : keys.front();
    };

    if( mode == TARGET_MODE_FIRE || mode == TARGET_MODE_TURRET_MANUAL || mode == TARGET_MODE_TURRET ) {
        mvwprintz( w_target, text_y++, 1, c_white, _( "[%s] Cycle targets; [%c] to fire." ),
                   ctxt.get_desc( "NEXT_TARGET", 1 ).c_str(), front_or( "FIRE", ' ' ) );
        mvwprintz( w_target, text_y++, 1, c_white, _( "[%c] target self; [%c] toggle snap-to-target" ),
                   front_or( "CENTER", ' ' ), front_or( "TOGGLE_SNAP_TO_TARGET", ' ' ) );
    }

    if( mode == TARGET_MODE_FIRE ) {
        mvwprintz( w_target, text_y++, 1, c_white, _( "[%c] to steady your aim. (10 moves)" ),
                   front_or( "AIM", ' ' ) );
        std::string aim_and_fire;
        for( const auto &e : aim_types ) {
            if( e.has_threshold ) {
                aim_and_fire += string_format( "[%s] ", front_or( e.action, ' ' ) );
            }
        }
        mvwprintz( w_target, text_y++, 1, c_white, "%sto aim and fire", aim_and_fire );
        mvwprintz( w_target, text_y++, 1, c_white, _( "[%c] to switch aiming modes." ),
                   front_or( "SWITCH_AIM", ' ' ) );
    }

    if( switch_mode ) {
        mvwprintz( w_target, text_y++, 1, c_white, _( "[%c] to switch firing modes." ),
                   front_or( "SWITCH_MODE", ' ' ) );
    }
    if( switch_ammo ) {
        mvwprintz( w_target, text_y++, 1, c_white, _( "[%c] to switch ammo." ), front_or( "SWITCH_AMMO",
                   ' ' ) );
    }

    if( is_mouse_enabled() ) {
        mvwprintz( w_target, text_y++, 1, c_white,
                   _( "Mouse: LMB: Target, Wheel: Cycle, RMB: Fire" ) );
    }
    return lines_used;
}

static int find_target( const std::vector<Creature *> &t, const tripoint &tpos )
{
    for( size_t i = 0; i < t.size(); ++i ) {
        if( t[i]->pos() == tpos ) {
            return static_cast<int>( i );
        }
    }
    return -1;
}

static void do_aim( player &p, const item &relevant )
{
    const double aim_amount = p.aim_per_move( relevant, p.recoil );
    if( aim_amount > 0 ) {
        // Increase aim at the cost of moves
        p.mod_moves( -1 );
        p.recoil = std::max( 0.0, p.recoil - aim_amount );
    } else {
        // If aim is already maxed, we're just waiting, so pass the turn.
        p.set_moves( 0 );
    }
}

struct confidence_rating {
    double aim_level;
    char symbol;
    std::string color;
    std::string label;
};

static int print_steadiness( const catacurses::window &w, int line_number, double steadiness )
{
    const int window_width = getmaxx( w ) - 2; // Window width minus borders.

    if( get_option<std::string>( "ACCURACY_DISPLAY" ) == "numbers" ) {
        std::string steadiness_s = string_format( "%s: %d%%", _( "Steadiness" ),
                                   static_cast<int>( 100.0 * steadiness ) );
        mvwprintw( w, line_number++, 1, steadiness_s );
    } else {
        const std::string &steadiness_bar = get_labeled_bar( steadiness, window_width,
                                            _( "Steadiness" ), '*' );
        mvwprintw( w, line_number++, 1, steadiness_bar.c_str() );
    }

    return line_number;
}

static double confidence_estimate( int range, double target_size,
                                   const dispersion_sources &dispersion )
{
    // This is a rough estimate of accuracy based on a linear distribution across min and max
    // dispersion.  It is highly inaccurate probability-wise, but this is intentional, the player
    // is not doing Gaussian integration in their head while aiming.  The result gives the player
    // correct relative measures of chance to hit, and corresponds with the actual distribution at
    // min, max, and mean.
    if( range == 0 ) {
        return 2 * target_size;
    }
    const double max_lateral_offset = iso_tangent( range, dispersion.max() );
    return 1 / ( max_lateral_offset / target_size );
}

static std::vector<aim_type> get_default_aim_type()
{
    std::vector<aim_type> aim_types;
    aim_types.push_back( aim_type { "", "", "", false, 0 } ); // dummy aim type for unaimed shots
    return aim_types;
}

typedef std::vector<std::tuple<double, char, std::string>> RatingVector;
std::string get_colored_bar( const double val, const int width, const std::string &label,
                             RatingVector::iterator begin, RatingVector::iterator end )
{
    std::string result;

    result.reserve( width );
    if( !label.empty() ) {
        result += label;
        result += ' ';
    }
    const int bar_width = width - utf8_width( result ) - 2; // - 2 for the brackets

    result += '[';
    if( bar_width > 0 ) {
        int used_width = 0;
        for( auto it( begin ); it != end; ++it ) {
            const double factor = std::min( 1.0, std::max( 0.0, std::get<0>( *it ) * val ) );
            const int seg_width = static_cast<int>( factor * bar_width ) - used_width;

            if( seg_width <= 0 ) {
                continue;
            }
            used_width += seg_width;
            result += string_format( "<color_%s>", std::get<2>( *it ) );
            result.insert( result.end(), seg_width, std::get<1>( *it ) );
            result += "</color>";
        }
        result.insert( result.end(), bar_width - used_width, ' ' );
    }
    result += ']';

    return result;
}

static int print_ranged_chance( const player &p, const catacurses::window &w, int line_number,
                                target_mode mode, input_context &ctxt, const item &ranged_weapon,
                                const dispersion_sources &dispersion, const std::vector<confidence_rating> &confidence_config,
                                double range, double target_size, int recoil = 0 )
{
    const int window_width = getmaxx( w ) - 2; // Window width minus borders.
    std::string display_type = get_option<std::string>( "ACCURACY_DISPLAY" );

    nc_color col = c_dark_gray;

    std::vector<aim_type> aim_types;
    if( mode == TARGET_MODE_THROW || mode == TARGET_MODE_THROW_BLIND ) {
        aim_types = get_default_aim_type();
    } else {
        aim_types = p.get_aim_types( ranged_weapon );
    }

    if( display_type != "numbers" ) {
        std::string symbols;
        for( const confidence_rating &cr : confidence_config ) {
            symbols += string_format( " <color_%s>%s</color> = %s", cr.color, cr.symbol, cr.label );
        }
        print_colored_text( w, line_number++, 1, col, col, string_format(
                                _( "Symbols:%s" ), symbols ) );
    }

    const auto front_or = [&]( const std::string & s, const char fallback ) {
        const auto keys = ctxt.keys_bound_to( s );
        return keys.empty() ? fallback : keys.front();
    };

    for( const aim_type &type : aim_types ) {
        dispersion_sources current_dispersion = dispersion;
        int threshold = MAX_RECOIL;
        std::string label = _( "Current Aim" );
        if( type.has_threshold ) {
            label = type.name;
            threshold = type.threshold;
            current_dispersion.add_range( threshold );
        } else {
            current_dispersion.add_range( recoil );
        }

        int moves_to_fire;
        if( mode == TARGET_MODE_THROW || mode == TARGET_MODE_THROW_BLIND ) {
            moves_to_fire = throw_cost( p, ranged_weapon );
        } else {
            moves_to_fire = p.gun_engagement_moves( ranged_weapon, threshold, recoil ) + time_to_fire( p,
                            *ranged_weapon.type );
        }

        auto hotkey = front_or( type.action.empty() ? "FIRE" : type.action, ' ' );
        print_colored_text( w, line_number++, 1, col, col,
                            string_format( _( "<color_white>[%s]</color> %s: Moves to fire: <color_light_blue>%d</color>" ),
                                           hotkey, label, moves_to_fire ) );

        double confidence = confidence_estimate( range, target_size, current_dispersion );

        if( display_type == "numbers" ) {
            int last_chance = 0;
            std::string confidence_s = enumerate_as_string( confidence_config.begin(), confidence_config.end(),
            [&]( const confidence_rating & config ) {
                // TODO: Consider not printing 0 chances, but only if you can print something (at least miss 100% or so)
                int chance = std::min<int>( 100, 100.0 * ( config.aim_level * confidence ) ) - last_chance;
                last_chance += chance;
                return string_format( "%s: <color_%s>%3d%%</color>", _( config.label/*.c_str()*/ ), config.color,
                                      chance );
            }, enumeration_conjunction::none );
            line_number += fold_and_print_from( w, line_number, 1, window_width, 0,
                                                c_dark_gray, confidence_s );
        } else {
            std::vector<std::tuple<double, char, std::string>> confidence_ratings;
            std::transform( confidence_config.begin(), confidence_config.end(),
                            std::back_inserter( confidence_ratings ),
            [&]( const confidence_rating & config ) {
                return std::make_tuple( config.aim_level, config.symbol, config.color );
            }
                          );
            const std::string &confidence_bar = get_colored_bar( confidence, window_width, "",
                                                confidence_ratings.begin(),
                                                confidence_ratings.end() );

            print_colored_text( w, line_number++, 1, col, col, confidence_bar );
        }
    }

    return line_number;
}

static int print_aim( const player &p, const catacurses::window &w, int line_number,
                      input_context &ctxt, item *weapon,
                      const double target_size, const tripoint &pos, double predicted_recoil )
{
    // This is absolute accuracy for the player.
    // TODO: push the calculations duplicated from Creature::deal_projectile_attack() and
    // Creature::projectile_attack() into shared methods.
    // Dodge doesn't affect gun attacks

    dispersion_sources dispersion = p.get_weapon_dispersion( *weapon );
    dispersion.add_range( p.recoil_vehicle() );

    const double min_dispersion = p.effective_dispersion( p.weapon.sight_dispersion() );
    const double steadiness_range = MAX_RECOIL - min_dispersion;
    // This is a relative measure of how steady the player's aim is,
    // 0 is the best the player can do.
    const double steady_score = std::max( 0.0, predicted_recoil - min_dispersion );
    // Fairly arbitrary cap on steadiness...
    const double steadiness = 1.0 - ( steady_score / steadiness_range );

    // This could be extracted, to allow more/less verbose displays
    static const std::vector<confidence_rating> confidence_config = {{
            { accuracy_critical, '*', "green", translate_marker_context( "aim_confidence", "Great" ) },
            { accuracy_standard, '+', "light_gray", translate_marker_context( "aim_confidence", "Normal" ) },
            { accuracy_grazing, '|', "magenta", translate_marker_context( "aim_confidence", "Graze" ) }
        }
    };

    const double range = rl_dist( p.pos(), pos );
    line_number = print_steadiness( w, line_number, steadiness );
    return print_ranged_chance( p, w, line_number, TARGET_MODE_FIRE, ctxt, *weapon, dispersion,
                                confidence_config,
                                range, target_size, predicted_recoil );
}

static int draw_turret_aim( const player &p, const catacurses::window &w, int line_number,
                            const tripoint &targ )
{
    const optional_vpart_position vp = g->m.veh_at( p.pos() );
    if( !vp ) {
        debugmsg( "Tried to aim turret while outside vehicle" );
        return line_number;
    }

    // fetch and display list of turrets that are ready to fire at the target
    auto turrets = vp->vehicle().turrets( targ );

    mvwprintw( w, line_number++, 1, _( "Turrets in range: %d" ), turrets.size() );
    for( const auto e : turrets ) {
        mvwprintw( w, line_number++, 1, "*  %s", e->name().c_str() );
    }

    return line_number;
}

static int draw_throw_aim( const player &p, const catacurses::window &w, int line_number,
                           input_context &ctxt,
                           const item *weapon, const tripoint &target_pos, bool is_blind_throw )
{
    Creature *target = g->critter_at( target_pos, true );
    if( target != nullptr && !p.sees( *target ) ) {
        target = nullptr;
    }

    const dispersion_sources dispersion = p.throwing_dispersion( *weapon, target, is_blind_throw );
    const double range = rl_dist( p.pos(), target_pos );

    const double target_size = target != nullptr ? target->ranged_target_size() : 1.0f;

    static const std::vector<confidence_rating> confidence_config_critter = {{
            { accuracy_critical, '*', "green", translate_marker_context( "aim_confidence", "Great" ) },
            { accuracy_standard, '+', "light_gray", translate_marker_context( "aim_confidence", "Normal" ) },
            { accuracy_grazing, '|', "magenta", translate_marker_context( "aim_confidence", "Graze" ) }
        }
    };
    static const std::vector<confidence_rating> confidence_config_object = {{
            { accuracy_grazing, '*', "white", translate_marker_context( "aim_confidence", "Hit" ) }
        }
    };
    const auto &confidence_config = target != nullptr ?
                                    confidence_config_critter : confidence_config_object;

    const target_mode throwing_target_mode = is_blind_throw ? TARGET_MODE_THROW_BLIND :
            TARGET_MODE_THROW;
    return print_ranged_chance( p, w, line_number, throwing_target_mode, ctxt, *weapon, dispersion,
                                confidence_config,
                                range, target_size );
}

std::vector<tripoint> target_handler::target_ui( player &pc, const targeting_data &args )
{
    return target_ui( pc, args.mode, args.relevant, args.range,
                      args.ammo, args.on_mode_change, args.on_ammo_change );
}

std::vector<aim_type> Character::get_aim_types( const item &gun ) const
{
    std::vector<aim_type> aim_types = get_default_aim_type();
    if( !gun.is_gun() ) {
        return aim_types;
    }
    int sight_dispersion = effective_dispersion( gun.sight_dispersion() );
    // Aiming thresholds are dependent on weapon sight dispersion, attempting to place thresholds
    // at 10%, 5% and 0% of the difference between MAX_RECOIL and sight dispersion.
    std::vector<int> thresholds = {
        static_cast<int>( ( ( MAX_RECOIL - sight_dispersion ) / 10.0 ) + sight_dispersion ),
        static_cast<int>( ( ( MAX_RECOIL - sight_dispersion ) / 20.0 ) + sight_dispersion ),
        static_cast<int>( sight_dispersion )
    };
    std::vector<int>::iterator thresholds_it;
    // Remove duplicate thresholds.
    thresholds_it = std::adjacent_find( thresholds.begin(), thresholds.end() );
    while( thresholds_it != thresholds.end() ) {
        thresholds.erase( thresholds_it );
        thresholds_it = std::adjacent_find( thresholds.begin(), thresholds.end() );
    }
    thresholds_it = thresholds.begin();
    aim_types.push_back( aim_type { _( "Regular Aim" ), "AIMED_SHOT", _( "[%c] to aim and fire." ),
                                    true, *thresholds_it } );
    thresholds_it++;
    if( thresholds_it != thresholds.end() ) {
        aim_types.push_back( aim_type { _( "Careful Aim" ), "CAREFUL_SHOT",
                                        _( "[%c] to take careful aim and fire." ), true,
                                        *thresholds_it } );
        thresholds_it++;
    }
    if( thresholds_it != thresholds.end() ) {
        aim_types.push_back( aim_type { _( "Precise Aim" ), "PRECISE_SHOT",
                                        _( "[%c] to take precise aim and fire." ), true,
                                        *thresholds_it } );
    }
    return aim_types;
}

// TODO: Shunt redundant drawing code elsewhere
std::vector<tripoint> target_handler::target_ui( player &pc, target_mode mode,
        item *relevant, int range, const itype *ammo,
        const target_callback &on_mode_change,
        const target_callback &on_ammo_change )
{
    // TODO: this should return a reference to a static vector which is cleared on each call.
    static const std::vector<tripoint> empty_result{};
    std::vector<tripoint> ret;

    int sight_dispersion = 0;
    if( !relevant ) {
        relevant = &pc.weapon;
    } else {
        sight_dispersion = pc.effective_dispersion( relevant->sight_dispersion() );
    }

    tripoint src = pc.pos();
    tripoint dst = pc.pos();

    std::vector<Creature *> t;
    int target = 0;

    auto update_targets = [&]( int range, std::vector<Creature *> &targets, int &idx, tripoint & dst ) {
        targets = pc.get_targetable_creatures( range );

        // Convert and check last_target_pos is a valid aim point
        cata::optional<tripoint> local_last_tgt_pos = cata::nullopt;
        if( pc.last_target_pos ) {
            local_last_tgt_pos = g->m.getlocal( *pc.last_target_pos );
            if( rl_dist( src, *local_last_tgt_pos ) > range ) {
                local_last_tgt_pos = cata::nullopt;
            }
        }

        targets.erase( std::remove_if( targets.begin(), targets.end(), [&]( const Creature * e ) {
            return pc.attitude_to( *e ) == Creature::Attitude::A_FRIENDLY;
        } ), targets.end() );

        if( targets.empty() ) {
            idx = -1;

            if( pc.last_target_pos ) {

                if( local_last_tgt_pos ) {
                    dst = *local_last_tgt_pos;
                }
                if( ( pc.last_target.expired() || !pc.sees( *pc.last_target.lock().get() ) ) &&
                    pc.has_activity( activity_id( "ACT_AIM" ) ) ) {
                    //We lost our target. Stop auto aiming.
                    pc.cancel_activity();
                }

            } else {
                auto adjacent = closest_tripoints_first( range, dst );
                const auto target_spot = std::find_if( adjacent.begin(), adjacent.end(),
                [&pc]( const tripoint & pt ) {
                    return g->m.tr_at( pt ).id == tr_practice_target && pc.sees( pt );
                } );

                if( target_spot != adjacent.end() ) {
                    dst = *target_spot;
                }
            }
            return;
        }

        std::sort( targets.begin(), targets.end(), [&]( const Creature * lhs, const Creature * rhs ) {
            return rl_dist( lhs->pos(), pc.pos() ) < rl_dist( rhs->pos(), pc.pos() );
        } );

        // TODO: last_target should be member of target_handler
        const auto iter = std::find( targets.begin(), targets.end(), pc.last_target.lock().get() );

        if( iter != targets.end() ) {
            idx = std::distance( targets.begin(), iter );
            dst = targets[ idx ]->pos();
            pc.last_target_pos = cata::nullopt;
        } else {
            idx = 0;
            dst = local_last_tgt_pos ? *local_last_tgt_pos : targets[ 0 ]->pos();
            pc.last_target.reset();
        }
    };

    update_targets( range, t, target, dst );

    double recoil_pc = pc.recoil;
    tripoint recoil_pos = dst;

    bool compact = TERMY < 41;
    bool tiny = TERMY < 31;

    // Default to the maximum window size we can use.
    int height = 31;
    int top = 0;
    if( tiny ) {
        // If we're extremely short on space, use the whole sidebar.
        top = 0;
        height = TERMY;
    } else if( compact ) {
        // Cover up more low-value ui elements if we're tight on space.
        top -= 4;
        height = 25;
    }
    top = 0;
    catacurses::window w_target = catacurses::newwin( height, 45, top, TERMX - 45 );

    input_context ctxt( "TARGET" );
    ctxt.set_iso( true );
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
        aim_types = pc.get_aim_types( *relevant );
        for( aim_type &type : aim_types ) {
            if( type.has_threshold ) {
                ctxt.register_action( type.action );
            }
        }
        aim_mode = aim_types.begin();
    }

    int num_instruction_lines = draw_targeting_window( w_target, relevant->tname(),
                                mode, ctxt, aim_types,
                                static_cast<bool>( on_mode_change ),
                                static_cast<bool>( on_ammo_change ), tiny );

    bool snap_to_target = get_option<bool>( "SNAP_TO_TARGET" );

    const auto set_last_target = [&pc]( const tripoint & dst ) {
        pc.last_target_pos = g->m.getabs( dst );
        if( const Creature *const critter_ptr = g->critter_at( dst, true ) ) {
            pc.last_target = g->shared_from( *critter_ptr );
        } else {
            pc.last_target.reset();
        }
    };

    const auto confirm_non_enemy_target = [&pc]( const tripoint & dst ) {
        if( dst == pc.pos() ) {
            return true;
        }
        if( npc *const who_ = g->critter_at<npc>( dst ) ) {
            const npc &who = *who_;
            if( who.guaranteed_hostile() ) {
                return true;
            }
            return query_yn( _( "Really attack %s?" ), who.name.c_str() );
        }
        return true;
    };

    auto recalc_recoil = [&recoil_pc, &recoil_pos, &pc]( tripoint & dst ) {
        static const double recoil_per_deg = MAX_RECOIL / 180;

        const double phi = fmod( std::abs( coord_to_angle( pc.pos(), dst ) -
                                           coord_to_angle( pc.pos(), recoil_pos ) ),
                                 360.0 );
        const double angle = phi > 180.0 ? 360.0 - phi : phi;

        return std::min( recoil_pc + angle * recoil_per_deg, MAX_RECOIL );
    };

    const tripoint old_offset = pc.view_offset;
    do {
        ret = g->m.find_clear_path( src, dst );

        // This chunk of code handles shifting the aim point around
        // at maximum range when using circular distance.
        // The range > 1 check ensures that you can always at least hit adjacent squares.
        if( trigdist && range > 1 && round( trig_dist( src, dst ) ) > range ) {
            bool cont = true;
            tripoint cp = dst;
            for( size_t i = 0; i < ret.size() && cont; i++ ) {
                if( round( trig_dist( src, ret[i] ) ) > range ) {
                    ret.resize( i );
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
        for( int i = 1; i <= getmaxy( w_target ) - num_instruction_lines - 2; i++ ) {
            // Clear width excluding borders.
            for( int j = 1; j <= getmaxx( w_target ) - 2; j++ ) {
                mvwputch( w_target, i, j, c_white, ' ' );
            }
        }
        g->draw_ter( center, true );
        int line_number = 1;
        Creature *critter = g->critter_at( dst, true );
        const int relative_elevation = dst.z - pc.pos().z;
        if( dst != src ) {
            // Only draw those tiles which are on current z-level
            auto ret_this_zlevel = ret;
            ret_this_zlevel.erase( std::remove_if( ret_this_zlevel.begin(), ret_this_zlevel.end(),
            [&center]( const tripoint & pt ) {
                return pt.z != center.z;
            } ), ret_this_zlevel.end() );
            // Only draw a highlighted trajectory if we can see the endpoint.
            // Provides feedback to the player, and avoids leaking information
            // about tiles they can't see.
            g->draw_line( dst, center, ret_this_zlevel );

            // Print to target window
            mvwprintw( w_target, line_number++, 1, _( "Range: %d/%d Elevation: %d Targets: %d" ),
                       rl_dist( src, dst ), range, relative_elevation, t.size() );

        } else {
            mvwprintw( w_target, line_number++, 1, _( "Range: %d Elevation: %d Targets: %d" ), range,
                       relative_elevation, t.size() );
        }

        // Skip blank lines if we're short on space.
        if( !compact ) {
            line_number++;
        }
        if( mode == TARGET_MODE_FIRE || mode == TARGET_MODE_TURRET_MANUAL ) {
            auto m = relevant->gun_current_mode();

            if( relevant != m.target ) {
                mvwprintw( w_target, line_number++, 1, _( "Firing mode: %s %s (%d)" ),
                           m->tname().c_str(), m.name(), m.qty );
            } else {
                mvwprintw( w_target, line_number++, 1, _( "Firing mode: %s (%d)" ),
                           m.name(), m.qty );
            }

            const itype *cur = ammo ? ammo : m->ammo_data();
            if( cur ) {
                auto str = string_format( m->ammo_remaining() ?
                                          _( "Ammo: <color_%s>%s</color> (%d/%d)" ) :
                                          _( "Ammo: <color_%s>%s</color>" ),
                                          get_all_colors().get_name( cur->color ).c_str(),
                                          cur->nname( std::max( m->ammo_remaining(), 1L ) ).c_str(),
                                          m->ammo_remaining(), m->ammo_capacity() );

                nc_color col = c_light_gray;
                print_colored_text( w_target, line_number++, 1, col, col, str );
            }

            mvwprintw( w_target, line_number++, 1, _( "%s" ), print_recoil( g->u ).c_str() );

            // Skip blank lines if we're short on space.
            if( !compact ) {
                line_number++;
            }
        }

        if( critter && critter != &pc && pc.sees( *critter ) ) {
            // The 12 is 2 for the border and 10 for aim bars.
            // Just print the monster name if we're short on space.
            int available_lines = compact ? 1 : ( height - num_instruction_lines - line_number - 12 );
            line_number = critter->print_info( w_target, line_number, available_lines, 1 );
        } else {
            mvwputch( g->w_terrain, POSY + dst.y - center.y, POSX + dst.x - center.x, c_red, '*' );
        }

        if( mode == TARGET_MODE_FIRE ) {
            double predicted_recoil = pc.recoil;
            int predicted_delay = 0;
            if( aim_mode->has_threshold && aim_mode->threshold < pc.recoil ) {
                do {
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

            const double target_size = critter != nullptr ? critter->ranged_target_size() :
                                       occupied_tile_fraction( MS_MEDIUM );

            line_number = print_aim( pc, w_target, line_number, ctxt, &*relevant->gun_current_mode(),
                                     target_size, dst, predicted_recoil );

            if( aim_mode->has_threshold ) {
                mvwprintw( w_target, line_number++, 1, _( "%s Delay: %i" ), aim_mode->name.c_str(),
                           predicted_delay );
            }
        } else if( mode == TARGET_MODE_TURRET ) {
            line_number = draw_turret_aim( pc, w_target, line_number, dst );
        } else if( mode == TARGET_MODE_THROW ) {
            line_number = draw_throw_aim( pc, w_target, line_number, ctxt, relevant, dst, false );
        } else if( mode == TARGET_MODE_THROW_BLIND ) {
            line_number = draw_throw_aim( pc, w_target, line_number, ctxt, relevant, dst, true );
        }

        wrefresh( g->w_terrain );
        draw_targeting_window( w_target, relevant->tname(),
                               mode, ctxt, aim_types,
                               static_cast<bool>( on_mode_change ),
                               static_cast<bool>( on_ammo_change ), tiny );
        wrefresh( w_target );

        catacurses::refresh();

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
        cata::optional<tripoint> mouse_pos;
        // Our coordinates will either be determined by coordinate input(mouse),
        // by a direction key, or by the previous value.
        if( action == "SELECT" && ( mouse_pos = ctxt.get_coordinates( g->w_terrain ) ) ) {
            targ = *mouse_pos;
            targ.x -= dst.x;
            targ.y -= dst.y;
            targ.z -= dst.z;
        } else if( const cata::optional<tripoint> vec = ctxt.get_direction( action ) ) {
            targ.x = vec->x;
            targ.y = vec->y;
        } else {
            targ.x = 0;
            targ.y = 0;
        }
        if( action == "FIRE" && mode == TARGET_MODE_FIRE && aim_mode->has_threshold ) {
            action = aim_mode->action;
        }

        if( g->m.has_zlevels() && ( action == "LEVEL_UP" || action == "LEVEL_DOWN" ) ) {
            // Just determine our delta-z.
            const int dz = action == "LEVEL_UP" ? 1 : -1;

            // Shift the view up or down accordingly.
            // We need to clamp the offset, but it needs to be clamped such that
            // the player position plus the offset is still in range, since the player
            // might be at Z+10 and looking down to Z-10, which is an offset greater than
            // OVERMAP_DEPTH or OVERMAP_HEIGHT
            const int potential_result = pc.pos().z + pc.view_offset.z + dz;
            if( potential_result <= OVERMAP_HEIGHT && potential_result >= -OVERMAP_DEPTH ) {
                pc.view_offset.z += dz;
            }

            // Set our cursor z to our view z. This accounts for cases where
            // our view and our target are on different z-levels (e.g. when
            // we cycle targets on different z-levels but do not have SNAP_TO_TARGET
            // enabled). This will ensure that we don't just chase the cursor up or
            // down, never catching up.
            dst.z = clamp( pc.pos().z + pc.view_offset.z, -OVERMAP_DEPTH, OVERMAP_HEIGHT );

            // We need to do a bunch of redrawing and cache updates since we're
            // looking at a different z-level.
            g->refresh_all();
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

            pc.recoil = recalc_recoil( dst );

        } else if( ( action == "PREV_TARGET" ) && ( target != -1 ) ) {
            int newtarget = find_target( t, dst ) - 1;
            if( newtarget < 0 ) {
                newtarget = t.size() - 1;
            }
            dst = t[newtarget]->pos();
            pc.recoil = recalc_recoil( dst );
        } else if( ( action == "NEXT_TARGET" ) && ( target != -1 ) ) {
            int newtarget = find_target( t, dst ) + 1;
            if( newtarget == static_cast<int>( t.size() ) ) {
                newtarget = 0;
            }
            dst = t[newtarget]->pos();
            pc.recoil = recalc_recoil( dst );
        } else if( ( action == "AIM" ) ) {
            // No confirm_non_enemy_target here because we have not initiated the firing.
            // Aiming can be stopped / aborted at any time.
            recoil_pc = pc.recoil;
            recoil_pos = dst;

            set_last_target( dst );
            for( int i = 0; i < 10; ++i ) {
                do_aim( pc, *relevant );
            }
            if( pc.moves <= 0 ) {
                // We've run out of moves, clear target vector, but leave target selected.
                pc.assign_activity( activity_id( "ACT_AIM" ), 0, 0 );
                pc.activity.str_values.push_back( "AIM" );
                pc.view_offset = old_offset;
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
        } else if( action == "AIMED_SHOT" || action == "CAREFUL_SHOT" || action == "PRECISE_SHOT" ) {
            // This action basically means "FIRE" as well, the actual firing may be delayed
            // through aiming, but there is usually no means to stop it. Therefore we query here.
            if( !confirm_non_enemy_target( dst ) || dst == src ) {
                continue;
            }

            recoil_pc = pc.recoil;
            recoil_pos = dst;
            std::vector<aim_type>::iterator it;
            for( it = aim_types.begin(); it != aim_types.end(); it++ ) {
                if( action == it->action ) {
                    break;
                }
            }
            if( it == aim_types.end() ) {
                debugmsg( "Could not find a valid aim_type for %s", action.c_str() );
                aim_mode = aim_types.begin();
            }
            int aim_threshold = it->threshold;
            set_last_target( dst );
            do {
                do_aim( pc, *relevant );
            } while( pc.moves > 0 && pc.recoil > aim_threshold && pc.recoil - sight_dispersion > 0 );

            if( pc.recoil <= aim_threshold ||
                pc.recoil - sight_dispersion == 0 ) {
                // If we made it under the aim threshold, go ahead and fire.
                // Also fire if we're at our best aim level already.
                pc.view_offset = old_offset;
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
                return empty_result;
            }
        } else if( action == "FIRE" ) {
            if( !confirm_non_enemy_target( dst ) || dst == src ) {
                continue;
            }
            target = find_target( t, dst );
            if( src == dst ) {
                ret.clear();
            }
            break;
        } else if( action == "CENTER" ) {
            dst = src;
            set_last_target( dst );
            ret.clear();
        } else if( action == "TOGGLE_SNAP_TO_TARGET" ) {
            snap_to_target = !snap_to_target;
        } else if( action == "QUIT" ) { // return empty vector (cancel)
            ret.clear();
            pc.last_target_pos = cata::nullopt;
            target = -1;
            break;
        }

        // Make player's sprite flip to face the current target
        if( dst.x > src.x ) {
            g->u.facing = FD_RIGHT;
        } else if( dst.x < src.x ) {
            g->u.facing = FD_LEFT;
        }

    } while( true );

    pc.view_offset = old_offset;

    if( ret.empty() ) {
        return ret;
    }

    set_last_target( ret.back() );

    const auto lt_ptr = pc.last_target.lock();
    if( npc *const guy = dynamic_cast<npc *>( lt_ptr.get() ) ) {
        if( !guy->guaranteed_hostile() ) {
            // TODO: get rid of this. Or combine it with effect_hit_by_player
            guy->hit_by_player = true; // used for morale penalty
        }
        // TODO: should probably go into the on-hit code?
        guy->make_angry();

    } else if( monster *const mon = dynamic_cast<monster *>( lt_ptr.get() ) ) {
        // TODO: get rid of this. Or move into the on-hit code?
        mon->add_effect( effect_hit_by_player, 10_minutes );
    }
    wrefresh( w_target );
    return ret;
}

static projectile make_gun_projectile( const item &gun )
{
    projectile proj;
    proj.speed  = 1000;
    proj.impact = gun.gun_damage();
    proj.range = gun.gun_range();
    proj.proj_effects = gun.ammo_effects();

    auto &fx = proj.proj_effects;

    if( ( gun.ammo_data() && gun.ammo_data()->phase == LIQUID ) ||
        fx.count( "SHOT" ) || fx.count( "BOUNCE" ) ) {
        fx.insert( "WIDE" );
    }

    if( gun.ammo_data() ) {
        // Some projectiles have a chance of being recoverable
        bool recover = std::any_of( fx.begin(), fx.end(), []( const std::string & e ) {
            int n;
            return sscanf( e.c_str(), "RECOVER_%i", &n ) == 1 && !one_in( n );
        } );

        if( recover && !fx.count( "IGNITE" ) && !fx.count( "EXPLOSIVE" ) ) {
            item drop( gun.ammo_current(), calendar::turn, 1 );
            drop.active = fx.count( "ACT_ON_RANGED_HIT" );
            proj.set_drop( drop );
        }

        const auto &ammo = gun.ammo_data()->ammo;
        if( ammo->drop != "null" && x_in_y( ammo->drop_chance, 1.0 ) ) {
            item drop( ammo->drop );
            if( ammo->drop_active ) {
                drop.activate();
            }
            proj.set_drop( drop );
        }

        if( fx.count( "CUSTOM_EXPLOSION" ) > 0 ) {
            proj.set_custom_explosion( gun.ammo_data()->explosion );
        }
    }

    return proj;
}

int time_to_fire( const Character &p, const itype &firing )
{
    struct time_info_t {
        int min_time;  // absolute floor on the time taken to fire.
        int base;      // the base or max time taken to fire.
        int reduction; // the reduction in time given per skill level.
    };

    static const std::map<skill_id, time_info_t> map {
        {skill_id {"pistol"},   {10, 80,  10}},
        {skill_id {"shotgun"},  {70, 150, 25}},
        {skill_id {"smg"},      {20, 80,  10}},
        {skill_id {"rifle"},    {30, 150, 15}},
        {skill_id {"archery"},  {20, 220, 25}},
        {skill_id {"throw"},    {50, 220, 25}},
        {skill_id {"launcher"}, {30, 200, 20}},
        {skill_id {"melee"},    {50, 200, 20}}
    };

    const skill_id &skill_used = firing.gun->skill_used;
    const auto it = map.find( skill_used );
    // TODO: maybe JSON-ize this in some way? Probably as part of the skill class.
    static const time_info_t default_info{ 50, 220, 25 };

    const time_info_t &info = ( it == map.end() ) ? default_info : it->second;
    return std::max( info.min_time, info.base - info.reduction * p.get_skill_level( skill_used ) );
}

static void cycle_action( item &weap, const tripoint &pos )
{
    // eject casings and linkages in random direction avoiding walls using player position as fallback
    auto tiles = closest_tripoints_first( 1, pos );
    tiles.erase( tiles.begin() );
    tiles.erase( std::remove_if( tiles.begin(), tiles.end(), [&]( const tripoint & e ) {
        return !g->m.passable( e );
    } ), tiles.end() );
    tripoint eject = tiles.empty() ? pos : random_entry( tiles );

    // for turrets try and drop casings or linkages directly to any CARGO part on the same tile
    const optional_vpart_position vp = g->m.veh_at( pos );
    std::vector<vehicle_part *> cargo;
    if( vp && weap.has_flag( "VEHICLE" ) ) {
        cargo = vp->vehicle().get_parts_at( pos, "CARGO", part_status_flag::any );
    }

    if( weap.ammo_data() && weap.ammo_data()->ammo->casing ) {
        const itype_id casing = *weap.ammo_data()->ammo->casing;
        if( weap.has_flag( "RELOAD_EJECT" ) || weap.gunmod_find( "brass_catcher" ) ) {
            weap.contents.push_back( item( casing ).set_flag( "CASING" ) );
        } else {
            if( cargo.empty() ) {
                g->m.add_item_or_charges( eject, item( casing ) );
            } else {
                vp->vehicle().add_item( *cargo.front(), item( casing ) );
            }

            sfx::play_variant_sound( "fire_gun", "brass_eject", sfx::get_heard_volume( eject ),
                                     sfx::get_heard_angle( eject ) );
        }
    }

    // some magazines also eject disintegrating linkages
    const auto mag = weap.magazine_current();
    if( mag && mag->type->magazine->linkage ) {
        item linkage( *mag->type->magazine->linkage, calendar::turn, 1 );
        if( weap.gunmod_find( "brass_catcher" ) ) {
            linkage.set_flag( "CASING" );
            weap.contents.push_back( linkage );
        } else if( cargo.empty() ) {
            g->m.add_item_or_charges( eject, linkage );
        } else {
            vp->vehicle().add_item( *cargo.front(), linkage );
        }
    }
}

void make_gun_sound_effect( const player &p, bool burst, item *weapon )
{
    const auto data = weapon->gun_noise( burst );
    if( data.volume > 0 ) {
        sounds::sound( p.pos(), data.volume, sounds::sound_t::combat,
                       data.sound.empty() ? _( "Bang!" ) : data.sound );
    }
}

item::sound_data item::gun_noise( const bool burst ) const
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
    } else if( ammo_type() == ammotype( "arrow" ) ) {
        return { noise, _( "whizz!" ) };
    } else if( ammo_type() == ammotype( "bolt" ) ) {
        return { noise, _( "thonk!" ) };
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
            return { noise, _( "Kra-kow!" ) };
        }

    } else if( fx.count( "LIGHTNING" ) ) {
        if( noise < 20 ) {
            return { noise, _( "Bzzt!" ) };
        } else if( noise < 40 ) {
            return { noise, _( "Bzap!" ) };
        } else if( noise < 60 ) {
            return { noise, _( "Bzaapp!" ) };
        } else {
            return { noise, _( "Kra-koom!" ) };
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
            return { noise, burst ? _( "Kaboom!" ) : _( "kerblam!" ) };
        }
    }

    return { 0, "" }; // silent weapons
}

static bool is_driving( const player &p )
{
    const optional_vpart_position vp = g->m.veh_at( p.pos() );
    return vp && vp->vehicle().is_moving() && vp->vehicle().player_in_control( p );
}

static double dispersion_from_skill( double skill, double weapon_dispersion )
{
    if( skill >= MAX_SKILL ) {
        return 0.0;
    }
    double skill_shortfall = double( MAX_SKILL ) - skill;
    // Flat penalty dispersion per point of skill under max.
    double flat_penalty = get_option< float >( "GUN_DISPERSION_FLAT_PENALTY_PER_SKILL" );
    double dispersion_penalty = flat_penalty * skill_shortfall;
    double skill_threshold = get_option< float >( "GUN_DISPERSION_SKILL_THRESHOLD" );
    double mult_post_threshold = get_option< float >( "GUN_DISPERSION_MULT_POST_SKILL_THRESHOLD" );
    if( skill >= skill_threshold ) {
        double post_threshold_skill_shortfall = double( MAX_SKILL ) - skill;
        // Lack of mastery multiplies the dispersion of the weapon.
        return dispersion_penalty + weapon_dispersion * post_threshold_skill_shortfall *
               mult_post_threshold / ( double( MAX_SKILL ) - skill_threshold );
    }
    // Unskilled shooters suffer greater penalties, still scaling with weapon penalties.
    double pre_threshold_skill_shortfall = skill_threshold - skill;
    double mult_pre_thershold = get_option< float >( "GUN_DISPERSION_MULT_PRE_SKILL_THRESHOLD" );
    dispersion_penalty += weapon_dispersion * ( mult_post_threshold + pre_threshold_skill_shortfall *
                          mult_pre_thershold / skill_threshold );

    return dispersion_penalty;
}

// utility functions for projectile_attack
dispersion_sources player::get_weapon_dispersion( const item &obj ) const
{
    int weapon_dispersion = obj.gun_dispersion();
    dispersion_sources dispersion( weapon_dispersion );
    dispersion.add_range( ranged_dex_mod() );

    dispersion.add_range( ( encumb( bp_arm_l ) + encumb( bp_arm_r ) ) / 5 );

    if( is_driving( *this ) ) {
        // get volume of gun (or for auxiliary gunmods the parent gun)
        const item *parent = has_item( obj ) ? find_parent( obj ) : nullptr;
        const int vol = ( parent ? parent->volume() : obj.volume() ) / 250_ml;

        /** @EFFECT_DRIVING reduces the inaccuracy penalty when using guns whilst driving */
        dispersion.add_range( std::max( vol - get_skill_level( skill_driving ), 1 ) * 20 );
    }

    /** @EFFECT_GUN improves usage of accurate weapons and sights */
    double avgSkill = static_cast<double>( get_skill_level( skill_gun ) +
                                           get_skill_level( obj.gun_skill() ) ) / 2.0;
    avgSkill = std::min( avgSkill, static_cast<double>( MAX_SKILL ) );

    dispersion.add_range( dispersion_from_skill( avgSkill, weapon_dispersion ) );

    if( has_bionic( bionic_id( "bio_targeting" ) ) ) {
        dispersion.add_multiplier( 0.75 );
    }

    if( ( is_underwater() && !obj.has_flag( "UNDERWATER_GUN" ) ) ||
        // Range is effectively four times longer when shooting unflagged guns underwater.
        ( !is_underwater() && obj.has_flag( "UNDERWATER_GUN" ) ) ) {
        // Range is effectively four times longer when shooting flagged guns out of water.
        dispersion.add_range( 150 ); //Adding dispersion for additonal debuff
        dispersion.add_multiplier( 4 );
    }

    return dispersion;
}

double player::gun_value( const item &weap, long ammo ) const
{
    // TODO: Mods
    // TODO: Allow using a specified type of ammo rather than default
    if( !weap.type->gun ) {
        return 0.0;
    }

    if( ammo <= 0 ) {
        return 0.0;
    }

    const islot_gun &gun = *weap.type->gun;
    const itype_id ammo_type = weap.ammo_default( true );
    const itype *def_ammo_i = ammo_type != "NULL" ?
                              item::find_type( ammo_type ) :
                              nullptr;

    damage_instance gun_damage = weap.gun_damage();
    item tmp = weap;
    tmp.ammo_set( weap.ammo_default() );
    int total_dispersion = get_weapon_dispersion( tmp ).max() +
                           effective_dispersion( tmp.sight_dispersion() );

    if( def_ammo_i != nullptr && def_ammo_i->ammo ) {
        const islot_ammo &def_ammo = *def_ammo_i->ammo;
        gun_damage.add( def_ammo.damage );
        total_dispersion += def_ammo.dispersion;
    }

    float damage_factor = gun_damage.total_damage();
    if( damage_factor > 0 ) {
        // TODO: Multiple damage types
        damage_factor += 0.5f * gun_damage.damage_units.front().res_pen;
    }

    int move_cost = time_to_fire( *this, *weap.type );
    if( gun.clip != 0 && gun.clip < 10 ) {
        // TODO: RELOAD_ONE should get a penalty here
        int reload_cost = gun.reload_time + encumb( bp_hand_l ) + encumb( bp_hand_r );
        reload_cost /= gun.clip;
        move_cost += reload_cost;
    }

    // "Medium range" below means 9 tiles, "short range" means 4
    // Those are guarantees (assuming maximum time spent aiming)
    static const std::vector<std::pair<float, float>> dispersion_thresholds = {{
            // Headshots all the time
            { 0.0f, 5.0f },
            // Critical at medium range
            { 100.0f, 4.5f },
            // Critical at short range or good hit at medium
            { 200.0f, 3.5f },
            // OK hits at medium
            { 300.0f, 3.0f },
            // Point blank headshots
            { 450.0f, 2.5f },
            // OK hits at short
            { 700.0f, 1.5f },
            // Glances at medium, criticals at point blank
            { 1000.0f, 1.0f },
            // Nothing guaranteed, pure gamble
            { 2000.0f, 0.1f },
        }
    };

    static const std::vector<std::pair<float, float>> move_cost_thresholds = {{
            { 10.0f, 4.0f },
            { 25.0f, 3.0f },
            { 100.0f, 1.0f },
            { 500.0f, 5.0f },
        }
    };

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

    // TODO: Some better approximation of the ability to keep on shooting
    static const std::vector<std::pair<float, float>> capacity_thresholds = {{
            { 1.0f, 0.5f },
            { 5.0f, 1.0f },
            { 10.0f, 1.5f },
            { 20.0f, 2.0f },
            { 50.0f, 3.0f },
        }
    };

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
