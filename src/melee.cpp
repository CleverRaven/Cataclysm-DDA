#include "player.h"
#include "bionics.h"
#include "debug.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "rng.h"
#include "martialarts.h"
#include "messages.h"
#include "sounds.h"
#include "translations.h"
#include "monster.h"
#include "npc.h"
#include "itype.h"
#include "line.h"
#include "mtype.h"
#include "field.h"
#include "cata_utility.h"

#include <sstream>
#include <stdlib.h>
#include <algorithm>

#include "cursesdef.h"

static const matec_id tec_none( "tec_none" );
static const matec_id WBLOCK_1( "WBLOCK_1" );
static const matec_id WBLOCK_2( "WBLOCK_2" );
static const matec_id WBLOCK_3( "WBLOCK_3" );

static const skill_id skill_stabbing( "stabbing" );
static const skill_id skill_cutting( "cutting" );
static const skill_id skill_unarmed( "unarmed" );
static const skill_id skill_bashing( "bashing" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_dodge( "dodge" );

const efftype_id effect_badpoison( "badpoison" );
const efftype_id effect_beartrap( "beartrap" );
const efftype_id effect_bouldering( "bouldering" );
const efftype_id effect_contacts( "contacts" );
const efftype_id effect_downed( "downed" );
const efftype_id effect_drunk( "drunk" );
const efftype_id effect_heavysnare( "heavysnare" );
const efftype_id effect_hit_by_player( "hit_by_player" );
const efftype_id effect_lightsnare( "lightsnare" );
const efftype_id effect_poison( "poison" );
const efftype_id effect_stunned( "stunned" );

void player_hit_message(player* attacker, std::string message,
                        Creature &t, int dam, bool crit = false);
void melee_practice( player &u, bool hit, bool unarmed, bool bashing, bool cutting, bool stabbing);
int  stumble(player &u);
std::string melee_message( const ma_technique &tech, player &p, const dealt_damage_instance &ddi );

/* Melee Functions!
 * These all belong to class player.
 *
 * STATE QUERIES
 * bool is_armed() - True if we are armed with any weapon.
 * bool unarmed_attack() - True if we are NOT armed with any weapon, but still
 *  true if we're wielding a "fist" weapon (cestus, bionic claws etc.).
 *
 * HIT DETERMINATION
 * int hit_roll() - The player's hit roll, to be compared to a monster's or
 *   player's dodge_roll().  This handles weapon bonuses, weapon-specific
 *   skills, torso encumbrance penalties and drunken master bonuses.
 */

bool player::is_armed() const
{
    return !weapon.is_null();
}

bool player::unarmed_attack() const
{
    return weapon.has_flag("UNARMED_WEAPON");
}

bool player::handle_melee_wear( float wear_multiplier )
{
    return handle_melee_wear( weapon, wear_multiplier );
}

bool player::handle_melee_wear( item &shield, float wear_multiplier )
{
    if( wear_multiplier <= 0.0f ) {
        return false;
    }
    // Here is where we handle wear and tear on things we use as melee weapons or shields.
    if( shield.is_null() ) {
        return false;
    }

    // UNBREAKABLE_MELEE items can't be damaged through melee combat usage.
    if( shield.has_flag("UNBREAKABLE_MELEE") ) {
        return false;
    }

    ///\EFFECT_DEX reduces chance of damaging your melee weapon

    ///\EFFECT_STR increases chance of damaging your melee weapon (NEGATIVE)

    ///\EFFECT_MELEE reduces chance of damaging your melee weapon
    const float stat_factor = dex_cur / 2.0f
        + get_skill_level( skill_melee )
        + ( 64.0f / std::max( str_cur, 4 ) );
    const float material_factor = shield.chip_resistance();
    int damage_chance = static_cast<int>( stat_factor * material_factor / wear_multiplier );
    // DURABLE_MELEE items are made to hit stuff and they do it well, so they're considered to be a lot tougher
    // than other weapons made of the same materials.
    if( shield.has_flag( "DURABLE_MELEE" ) ) {
        damage_chance *= 4;
    }

    if( damage_chance > 0 && !one_in(damage_chance) ) {
        return false;
    }

    if( shield.damage < MAX_ITEM_DAMAGE ){
        add_msg_player_or_npc( m_bad, _("Your %s is damaged by the force of the blow!"),
                                _("<npcname>'s %s is damaged by the force of the blow!"),
                                shield.tname().c_str());
        //Don't increment until after the message is displayed
        shield.damage++;
        return false;
    }

    add_msg_player_or_npc( m_bad, _("Your %s is destroyed by the blow!"),
                            _("<npcname>'s %s is destroyed by the blow!"),
                            shield.tname().c_str());
    // Dump its contents on the ground
    for( auto &elem : shield.contents ) {
        g->m.add_item_or_charges( pos(), elem );
    }

    if( &shield == &weapon ) {
        // TODO: Make this work with non-wielded items
        remove_weapon();
    }

    return true;
}

int player::get_hit_weapon( const item &weap ) const
{
    // TODO: there is a listing of the same skills in player.cpp
    int unarmed_skill = get_skill_level( skill_unarmed );
    int bashing_skill = get_skill_level( skill_bashing );
    int cutting_skill = get_skill_level( skill_cutting );
    int stabbing_skill = get_skill_level( skill_stabbing );
    int melee_skill = get_skill_level( skill_melee );

    if( has_active_bionic( "bio_cqb" ) ) {
        unarmed_skill = 5;
        bashing_skill = 5;
        cutting_skill = 5;
        stabbing_skill = 5;
        melee_skill = 5;
    }

    float best_bonus = 0.0;

    // Are we unarmed?
    if( weap.has_flag("UNARMED_WEAPON") ) {
        ///\EFFECT_UNARMED provides weapon bonus for UNARMED_WEAPON
        best_bonus = unarmed_skill / 2.0f;
    }

    // Using a bashing weapon?
    if( weap.is_bashing_weapon() ) {
        ///\EFFECT_BASHING provides weapon bonus for bashing weapons
        best_bonus = std::max( best_bonus, bashing_skill / 3.0f );
    }

    // Using a cutting weapon?
    if( weap.is_cutting_weapon() ) {
        ///\EFFECT_CUTTING provides weapon bonus for cutting weapons
        best_bonus = std::max( best_bonus, cutting_skill / 3.0f );
    }

    // Using a spear?
    if( weap.has_flag("SPEAR") || weap.has_flag("STAB") ) {
        ///\EFFECT_STABBING provides weapon bonus for SPEAR or STAB weapons
        best_bonus = std::max( best_bonus, stabbing_skill / 3.0f );
    }

    ///\EFFECT_MELEE adds to other weapon bonuses
    return int(melee_skill / 2.0f + best_bonus);
}

int player::get_hit_base() const
{
    // Character::get_hit_base includes stat calculations already
    return Character::get_hit_base() + get_hit_weapon( weapon );
}

int player::hit_roll() const
{
    //Unstable ground chance of failure
    if( has_effect( effect_bouldering) && one_in(8) ) {
        add_msg_if_player(m_bad, _("The ground shifts beneath your feet!"));
        return 0;
    }

    // Dexterity, skills, weapon and martial arts
    int numdice = get_hit();
    // Drunken master makes us hit better
    if (has_trait("DRUNKEN")) {
        if (unarmed_attack()) {
            numdice += int(get_effect_dur( effect_drunk ) / 300);
        } else {
            numdice += int(get_effect_dur( effect_drunk ) / 400);
        }
    }

    // Farsightedness makes us hit worse
    if( has_trait("HYPEROPIC") &&
        !is_wearing("glasses_reading") && !is_wearing("glasses_bifocal") ) {
        numdice -= 2;
    }

    if( numdice < 1 ) {
        numdice = 1;
    }

    int sides = 10 - divide_roll_remainder( encumb( bp_torso ), 10.0f );
    sides = std::max( sides, 2 );
    return dice( numdice, sides );
}

void player::add_miss_reason(const char *reason, unsigned int weight)
{
    melee_miss_reasons.add(reason, weight);

}

void player::clear_miss_reasons()
{
    melee_miss_reasons.clear();
}

const char *player::get_miss_reason()
{
    // everything that lowers accuracy in player::hit_roll()
    // adding it in hit_roll() might not be safe if it's called multiple times
    // in one turn
    add_miss_reason(
        _("Your torso encumbrance throws you off-balance."),
        divide_roll_remainder( encumb( bp_torso ), 10.0f ) );
    const int farsightedness = 2 * ( has_trait("HYPEROPIC") &&
                               !is_wearing("glasses_reading") &&
                               !is_wearing("glasses_bifocal") &&
                               !has_effect( effect_contacts) );
    add_miss_reason(
        _("You can't hit reliably due to your farsightedness."),
        farsightedness);

    const char** reason = melee_miss_reasons.pick();
    if(reason == NULL) {
        return NULL;
    }
    return *reason;
}

void player::roll_all_damage( bool crit, damage_instance &di ) const
{
    roll_all_damage( crit, di, false, weapon );
}

void player::roll_all_damage( bool crit, damage_instance &di, bool average, const item &weap ) const
{
    roll_bash_damage( crit, di, average, weap );
    roll_cut_damage ( crit, di, average, weap );
    roll_stab_damage( crit, di, average, weap );
}

// Melee calculation is in parts. This sets up the attack, then in deal_melee_attack,
// we calculate if we would hit. In Creature::deal_melee_hit, we calculate if the target dodges.
void player::melee_attack(Creature &t, bool allow_special, const matec_id &force_technique)
{
    if (!t.is_player()) {
        // @todo Per-NPC tracking? Right now monster hit by either npc or player will draw aggro...
        t.add_effect( effect_hit_by_player, 100); // Flag as attacked by us for AI
    }

    const bool critical_hit = scored_crit( t.dodge_roll() );

    int move_cost = attack_speed( weapon );

    const int hit_spread = t.deal_melee_attack(this, hit_roll());
    if( hit_spread < 0 ) {
        int stumble_pen = stumble(*this);
        sfx::generate_melee_sound( pos(), t.pos(), 0, 0);
        if( is_player() ) { // Only display messages if this is the player

            if( one_in(2) ) {
                const char* reason_for_miss = get_miss_reason();
                if (reason_for_miss != NULL)
                    add_msg(reason_for_miss);
            }

            if (has_miss_recovery_tec())
                add_msg(_("You feint."));
            else if (stumble_pen >= 60)
                add_msg(m_bad, _("You miss and stumble with the momentum."));
            else if (stumble_pen >= 10)
                add_msg(_("You swing wildly and miss."));
            else
                add_msg(_("You miss."));
        } else if( g->u.sees( *this ) ) {
            if (stumble_pen >= 60)
                add_msg( _("%s misses and stumbles with the momentum."),name.c_str());
            else if (stumble_pen >= 10)
                add_msg(_("%s swings wildly and misses."),name.c_str());
            else
                add_msg(_("%s misses."),name.c_str());
        }

        t.on_dodge( this, get_melee() );

        if( !has_active_bionic("bio_cqb") ) {
            // No practice if you're relying on bio_cqb to fight for you
            melee_practice( *this, false, unarmed_attack(),
                            weapon.is_bashing_weapon(), weapon.is_cutting_weapon(),
                            (weapon.has_flag("SPEAR") || weapon.has_flag("STAB")));
        }

        // Cap stumble penalty, heavy weapons are quite weak already
        move_cost += std::min( 60, stumble_pen );
        if( has_miss_recovery_tec() ) {
            move_cost = rng(move_cost / 3, move_cost);
        }
    } else {
        // Remember if we see the monster at start - it may change
        const bool seen = g->u.sees( t );
        // Start of attacks.
        damage_instance d;
        roll_all_damage( critical_hit, d );

        const bool has_force_technique = !force_technique.str().empty();

        // Pick one or more special attacks
        matec_id technique_id;
        if( allow_special && !has_force_technique ) {
            technique_id = pick_technique(t, critical_hit, false, false);
        } else if ( has_force_technique ) {
            technique_id = force_technique;
        } else {
            technique_id = tec_none;
        }

        const ma_technique &technique = technique_id.obj();

        // Handles effects as well; not done in melee_affect_*
        if( technique.id != tec_none ) {
            perform_technique(technique, t, d, move_cost);
        }

        if( allow_special && !t.is_dead_state() ) {
            perform_special_attacks(t);
        }

        // Proceed with melee attack.
        if( !t.is_dead_state() ) {
            // Handles speed penalties to monster & us, etc
            std::string specialmsg = melee_special_effects(t, d, technique);

            dealt_damage_instance dealt_dam; // gets overwritten with the dealt damage values
            t.deal_melee_hit(this, hit_spread, critical_hit, d, dealt_dam);

            // Make a rather quiet sound, to alert any nearby monsters
            if (!is_quiet()) { // check martial arts silence
                sounds::sound( pos(), 8, "" );
            }
            std::string material = "flesh";
            if( t.is_monster() ) {
                const monster *m = dynamic_cast<const monster*>( &t );
                if ( m->made_of("steel")) {
                    material = "steel";
                }
            }
            sfx::generate_melee_sound( pos(), t.pos(), 1, t.is_monster(), material);
            int dam = dealt_dam.total_damage();

            bool bashing = (d.type_damage(DT_BASH) >= 10 && !unarmed_attack());
            bool cutting = (d.type_damage(DT_CUT) >= 10);
            bool stabbing = (d.type_damage(DT_STAB) >= 10);

            // Set the highest damage type to true.
            if( !unarmed_attack() ) {
                if( d.type_damage(DT_BASH) > d.type_damage(DT_CUT) ) {
                    if( d.type_damage(DT_BASH) > d.type_damage(DT_STAB) ) {
                        bashing = true;
                    } else {
                        stabbing = true;
                    }
                } else {
                    if( d.type_damage(DT_CUT) > d.type_damage(DT_STAB) ) {
                        cutting = true;
                    } else {
                        stabbing = true;
                    }
                }
            }

            if (!has_active_bionic("bio_cqb")) {
                //no practice if you're relying on bio_cqb to fight for you
                melee_practice( *this, true, unarmed_attack(), bashing, cutting, stabbing );
            }

            if (dam >= 5 && has_artifact_with(AEP_SAP_LIFE)) {
                healall( rng(dam / 10, dam / 5) );
            }

            // Treat monster as seen if we see it before or after the attack
            if( seen || g->u.sees( t ) ) {
                std::string message = melee_message( technique, *this, dealt_dam );
                player_hit_message( this, message, t, dam, critical_hit );
            } else {
                add_msg_player_or_npc( m_good, _("You hit something."), _("<npcname> hits something.") );
            }

            if (!specialmsg.empty()) {
                add_msg_if_player(specialmsg.c_str());
            }
        }

        t.check_dead_state();
    }

    const int melee = get_skill_level( skill_melee );
    ///\EFFECT_STR reduces stamina cost for melee attack with heavier weapons
    const int weight_cost = weapon.weight() / ( 20 * std::max( 1, str_cur ) );
    const int encumbrance_cost =
        divide_roll_remainder( encumb( bp_arm_l ) + encumb( bp_arm_r ), 5.0f );
    ///\EFFECT_MELEE reduces stamina cost of melee attacks
    const int mod_sta = ( weight_cost + encumbrance_cost - melee + 20 ) * -1;
    mod_stat("stamina", mod_sta);

    mod_moves(-move_cost);

    ma_onattack_effects(); // trigger martial arts on-attack effects
    // some things (shattering weapons) can harm the attacking creature.
    check_dead_state();
    return;
}

void player::reach_attack( const tripoint &p )
{
    matec_id force_technique = tec_none;
    ///\EFFECT_MELEE >5 allows WHIP_DISARM technique
    if( weapon.has_flag( "WHIP" ) && ( skillLevel( skill_melee ) > 5) && one_in( 3 ) ) {
        force_technique = matec_id( "WHIP_DISARM" );
    }

    Creature *critter = g->critter_at( p );
    // Original target size, used when there are monsters in front of our target
    int target_size = critter != nullptr ? critter->get_size() : 2;

    int move_cost = attack_speed( weapon );
    int skill = std::min( 10, (int)get_skill_level( skill_stabbing ) );
    int t = 0;
    std::vector<tripoint> path = line_to( pos(), p, t, 0 );
    path.pop_back(); // Last point is our critter
    for( const tripoint &p : path ) {
        // Possibly hit some unintended target instead
        Creature *inter = g->critter_at( p );
        ///\EFFECT_STABBING decreases chance of hitting intervening target on reach attack
        if( inter != nullptr &&
              !x_in_y( ( target_size * target_size + 1 ) * skill,
                       ( inter->get_size() * inter->get_size() + 1 ) * 10 ) ) {
            // Even if we miss here, low roll means weapon is pushed away or something like that
            critter = inter;
            break;
        ///\EFFECT_STABBING increases ability to reach attack through fences
        } else if( g->m.impassable( p ) &&
                   // Fences etc. Spears can stab through those
                     !( weapon.has_flag( "SPEAR" ) &&
                        g->m.has_flag( "THIN_OBSTACLE", p ) &&
                        x_in_y( skill, 10 ) ) ) {
            ///\EFFECT_STR increases bash effects when reach attacking past something
            g->m.bash( p, str_cur + weapon.type->melee_dam );
            handle_melee_wear();
            mod_moves( -move_cost );
            return;
        }
    }

    if( critter == nullptr ) {
        add_msg_if_player( _("You swing at the air.") );
        if( has_miss_recovery_tec() ) {
            move_cost /= 3; // "Probing" is faster than a regular miss
        }

        mod_moves( -move_cost );
        return;
    }

    melee_attack( *critter, false, force_technique );
}

int stumble(player &u)
{
    if( u.has_trait("DEFT") ) {
        return 0;
    }

    // Examples:
    // 10 str with a hatchet: 4 + 8 = 12
    // 5 str with a battle axe: 26 + 49 = 75
    // Fist: 0

    ///\EFFECT_STR reduces chance of stumbling with heavier weapons
    return ( 2 * u.weapon.volume() ) +
           ( u.weapon.weight() / ( u.str_cur * 10 + 13.0f ) );
}

bool player::scored_crit(int target_dodge) const
{
    return rng_float( 0, 1.0 ) < crit_chance( hit_roll(), target_dodge, weapon );
}

double player::crit_chance( int roll_hit, int target_dodge, const item &weap ) const
{
    // TODO: see player.cpp ther eis the same listing of those skill!
    int unarmed_skill = get_skill_level( skill_unarmed );
    int bashing_skill = get_skill_level( skill_bashing );
    int cutting_skill = get_skill_level( skill_cutting );
    int stabbing_skill = get_skill_level( skill_stabbing );
    int melee_skill = get_skill_level( skill_melee );

    if( has_active_bionic("bio_cqb") ) {
        unarmed_skill = 5;
        bashing_skill = 5;
        cutting_skill = 5;
        stabbing_skill = 5;
        melee_skill = 5;
    }
    // Weapon to-hit roll
    double weapon_crit_chance = 0.5;
    if( weap.has_flag("UNARMED_WEAPON") ) {
        // Unarmed attack: 1/2 of unarmed skill is to-hit
        ///\EFFECT_UNARMED increases crit chance with UNARMED_WEAPON
        weapon_crit_chance = 0.5 + 0.05 * unarmed_skill;
    }

    if( weap.type->m_to_hit > 0 ) {
        weapon_crit_chance = std::max(
            weapon_crit_chance, 0.5 + 0.1 * weap.type->m_to_hit );
    } else if( weap.type->m_to_hit < 0 ) {
        weapon_crit_chance += 0.1 * weap.type->m_to_hit;
    }

    // Dexterity and perception
    ///\EFFECT_DEX increases chance for critical hits

    ///\EFFECT_PER increases chance for critical hits
    const double stat_crit_chance = 0.25 + 0.01 * dex_cur + ( 0.02 * per_cur );

    // Skill level roll
    int best_skill = 0;

    if( weap.is_bashing_weapon() && bashing_skill > best_skill ) {
        ///\EFFECT_BASHING inreases crit chance with bashing weapons
        best_skill = bashing_skill;
    }

    if( weap.is_cutting_weapon() && cutting_skill > best_skill ) {
        ///\EFFECT_CUTTING increases crit chance with cutting weapons
        best_skill = cutting_skill;
    }

    if( (weap.has_flag("SPEAR") || weap.has_flag("STAB")) &&
        stabbing_skill > best_skill ) {
        ///\EFFECT_STABBING increases crit chance with SPEAR or STAB weapons
        best_skill = stabbing_skill;
    }

    if( weap.has_flag("UNARMED_WEAPON") && unarmed_skill > best_skill ) {
        ///\EFFECT_UNARMED increases crit chance with UNARMED_WEAPON
        best_skill = unarmed_skill;
    }

    ///\EFFECT_MELEE slightly increases crit chance with any melee weapon
    best_skill += melee_skill / 2.5;

    const double skill_crit_chance = 0.25 + best_skill * 0.025;

    // Examples (survivor stats/chances of each crit):
    // Fresh (skill-less) 8/8/8/8, unarmed:
    //  50%, 49%, 25%; ~1/16 guaranteed crit + ~1/8 if roll>dodge*1.5
    // Expert (skills 10) 10/10/10/10, unarmed:
    //  100%, 55%, 60%; ~1/3 guaranteed crit + ~4/10 if roll>dodge*1.5
    // Godlike with combat CBM 20/20/20/20, pipe (+1 accuracy):
    //  60%, 100%, 42%; ~1/4 guaranteed crit + ~3/8 if roll>dodge*1.5

    // Note: the formulas below are only valid if none of the 3 crit chance values go above 1.0
    // But that would require >10 skills/+6 to-hit/75 dex, so it's OK to have minor weirdness there
    // Things like crit values dropping a bit instead of rising

    // Chance to get all 3 crits (a guaranteed crit regardless of hit/dodge)
    const double chance_triple = weapon_crit_chance * stat_crit_chance * skill_crit_chance;
    // Only check double crit (one that requries hit/dodge comparison) if we have good hit vs dodge
    if( roll_hit > target_dodge * 3 / 2 ) {
        const double chance_double = 0.5 * (
            weapon_crit_chance * stat_crit_chance +
            stat_crit_chance * skill_crit_chance +
            weapon_crit_chance * skill_crit_chance -
            ( 3 * chance_triple ) );
        return chance_triple + chance_double - ( chance_triple * chance_double );
    }

    return chance_triple;
}

int player::get_dodge_base() const
{
    // Creature::get_dodge_base includes stat calculations already
    ///\EFFECT_DODGE increases dodge_base
    return Character::get_dodge_base() + get_skill_level( skill_dodge );
}

//Returns 1/2*DEX + dodge skill level + static bonuses from mutations
//Return numbers range from around 4 (starting player, no boosts) to 29 (20 DEX, 10 dodge, +9 mutations)
int player::get_dodge() const
{
    //If we're asleep or busy we can't dodge
    if( in_sleep_state() ) {
        return 0;
    }

    int ret = Creature::get_dodge();
    // Chop in half if we are unable to move
    if( has_effect( effect_beartrap ) || has_effect( effect_lightsnare ) || has_effect( effect_heavysnare ) ) {
        ret /= 2;
    }
    return ret;
}

int player::dodge_roll()
{
    ///\EFFECT_DEX decreases chance of falling over when dodging on roller blades

    ///\EFFECT_DODGE decreases chances of falling over when dodging on roller blades
    if( is_wearing("roller_blades") &&
        one_in( (get_dex() + get_skill_level( skill_dodge )) / 3 ) &&
        !has_effect( effect_downed) ) {
        // Skaters have a 67% chance to avoid knockdown, and get up a turn quicker.
        if (has_trait("PROF_SKATER")) {
            if (one_in(3)) {
                add_msg_if_player(m_bad, _("You overbalance and stumble!"));
                add_effect( effect_downed, 2);
            } else {
                add_msg_if_player(m_good, _("You nearly fall, but recover thanks to your skating experience."));
            }
        } else {
            add_msg_if_player(_("Fighting on wheels is hard!"));
            add_effect( effect_downed, 3);
        }
    }

    if (has_effect( effect_bouldering)) {
        ///\EFFECT_DEX decreases chance of falling when dodging while bouldering
        if(one_in(get_dex())) {
            add_msg_if_player(m_bad, _("You slip as the ground shifts beneath your feet!"));
            add_effect( effect_downed, 3);
            return 0;
        }
    }
    int dodge_stat = get_dodge();

    if (dodges_left <= 0) { // We already dodged this turn
        ///\EFFECT_DEX increases chance of being allowed to dodge multiple times per turn

        ///\EFFECT_DODGE increases chance of being allowed to dodge multiple times per turn
        const int dodge_dex = get_skill_level( skill_dodge ) + dex_cur;
        if( rng(0, dodge_dex + 15) <= dodge_dex ) {
            dodge_stat = rng( dodge_stat/2, dodge_stat ); //Penalize multiple dodges per turn
        } else {
            dodge_stat = 0;
        }
    }

    const int roll = dice(dodge_stat, 10);
    // Speed below 100 linearly decreases dodge effectiveness
    int speed_stat = get_speed();
    if( speed_stat < 100 ) {
        return roll * speed_stat / 100;
    }

    return roll;
}

float player::bonus_damage( bool random ) const
{
    ///\EFFECT_STR increases bashing damage
    if( random ) {
        return rng_float( get_str() / 2.0f, get_str() );
    }

    return get_str() * 0.75f;
}

void player::roll_bash_damage( bool crit, damage_instance &di, bool average, const item &weap ) const
{
    float bash_dam = 0.0f;

    const bool unarmed = weap.has_flag("UNARMED_WEAPON");
    int bashing_skill = get_skill_level( skill_bashing );
    int unarmed_skill = get_skill_level( skill_unarmed );

    if( has_active_bionic("bio_cqb") ) {
        bashing_skill = 5;
        unarmed_skill = 5;
    }

    const int stat = get_str();
    ///\EFFECT_STR increases bashing damage
    float stat_bonus = bonus_damage( !average );
    stat_bonus += mabuff_bash_bonus();

    const int skill = unarmed ? unarmed_skill : bashing_skill;

    // Drunken Master damage bonuses
    if( has_trait("DRUNKEN") && has_effect( effect_drunk) ) {
        // Remember, a single drink gives 600 levels of "drunk"
        int mindrunk = 0;
        int maxdrunk = 0;
        const int drunk_dur = get_effect_dur( effect_drunk );
        if( unarmed ) {
            mindrunk = drunk_dur / 600;
            maxdrunk = drunk_dur / 250;
        } else {
            mindrunk = drunk_dur / 900;
            maxdrunk = drunk_dur / 400;
        }

        bash_dam += average ? (mindrunk + maxdrunk) * 0.5f : rng(mindrunk, maxdrunk);
    }

    ///\EFFECT_STR increases bashing damage
    float weap_dam = weap.damage_bash() + stat_bonus;
    ///\EFFECT_UNARMED caps bash damage with unarmed weapons

    ///\EFFECT_BASHING caps bash damage with bashing weapons
    float bash_cap = 2 * stat + 2 * skill;
    float bash_mul = 1.0f;

    if( unarmed ) {
        ///\EFFECT_UNARMED increases bashing damage with unarmed weapons
        weap_dam += unarmed_skill;
    }

    // 80%, 88%, 96%, 104%, 112%, 116%, 120%, 124%, 128%, 132%
    if( skill < 5 ) {
        bash_mul = 0.8 + 0.08 * skill;
    } else {
        bash_mul = 0.96 + 0.04 * skill;
    }

    if( bash_cap < weap_dam ) {
        // If damage goes over cap due to low stats/skills,
        // scale the post-armor damage down halfway between damage and cap
        bash_mul *= (1.0f + (bash_cap / weap_dam)) / 2.0f;
    }

    ///\EFFECT_STR boosts low cap on bashing damage
    const float low_cap = std::min( 1.0f, stat / 20.0f );
    const float bash_min = low_cap * weap_dam;
    weap_dam = average ? (bash_min + weap_dam) * 0.5f : rng_float(bash_min, weap_dam);

    bash_dam += weap_dam;
    bash_mul *= mabuff_bash_mult();

    float armor_mult = 1.0f;
    // Finally, extra crit effects
    if( crit ) {
        bash_mul *= 1.5f;
        // 50% arpen
        armor_mult = 0.5f;
    }

    di.add_damage( DT_BASH, bash_dam, 0, armor_mult, bash_mul );
}

void player::roll_cut_damage( bool crit, damage_instance &di, bool average, const item &weap ) const
{
    const bool stabs = weap.has_flag("SPEAR") || weap.has_flag("STAB");
    float cut_dam = stabs ? 0.0f : mabuff_cut_bonus() + weap.damage_cut();
    float cut_mul = 1.0f;

    int cutting_skill = get_skill_level( skill_cutting );
    int unarmed_skill = get_skill_level( skill_unarmed );

    if( has_active_bionic("bio_cqb") ) {
        cutting_skill = 5;
    }

    if( weap.has_flag("UNARMED_WEAPON") ) {
        // TODO: 1-handed weapons that aren't unarmed attacks
        const bool left_empty = !natural_attack_restricted_on(bp_hand_l);
        const bool right_empty = !natural_attack_restricted_on(bp_hand_r) &&
            weap.is_null();
        if( left_empty || right_empty ) {
            float per_hand = 0.0f;
            if (has_trait("CLAWS") || (has_active_mutation("CLAWS_RETRACT")) ) {
                per_hand += 3;
            }
            if (has_bionic("bio_razors")) {
                per_hand += 2;
            }
            if (has_trait("TALONS")) {
                ///\EFFECT_UNARMED increases cutting damage with TALONS
                per_hand += 3 + (unarmed_skill > 8 ? 4 : unarmed_skill / 2);
            }
            // Stainless Steel Claws do stabbing damage, too.
            if (has_trait("CLAWS_RAT") || has_trait("CLAWS_ST")) {
                ///\EFFECT_UNARMED increases cutting damage with CLAWS_RAT and CLAWS_ST
                per_hand += 1 + (unarmed_skill > 8 ? 4 : unarmed_skill / 2);
            }
            //TODO: add acidproof check back to slime hands (probably move it elsewhere)
            if (has_trait("SLIME_HANDS")) {
                ///\EFFECT_UNARMED increases cutting damage with SLIME_HANDS
                per_hand += average ? 2.5f : rng(2, 3);
            }

            cut_dam += per_hand; // First hand
            if( left_empty && right_empty ) {
                // Second hand
                cut_dam += per_hand;
            }
        }
    }

    if( cut_dam <= 0.0f ) {
        return; // No negative damage!
    }

    int arpen = 0;
    float armor_mult = 1.0f;

    // 80%, 88%, 96%, 104%, 112%, 116%, 120%, 124%, 128%, 132%
    ///\EFFECT_CUTTING increases cutting damage multiplier
    if( cutting_skill < 5 ) {
        cut_mul *= 0.8 + 0.08 * cutting_skill;
    } else {
        cut_mul *= 0.96 + 0.04 * cutting_skill;
    }

    cut_mul *= mabuff_cut_mult();
    if( crit ) {
        cut_mul *= 1.25f;
        arpen += 5;
        armor_mult = 0.75f; //25% arpen
    }

    di.add_damage( DT_CUT, cut_dam, arpen, armor_mult, cut_mul );
}

void player::roll_stab_damage( bool crit, damage_instance &di, bool average, const item &weap ) const
{
    (void)average; // No random rolls in stab damage
    const bool stabs = weap.has_flag("SPEAR") || weap.has_flag("STAB");
    float cut_dam = stabs ? mabuff_cut_bonus() + weap.damage_cut() : 0.0f;

    int unarmed_skill = get_skill_level( skill_unarmed );
    int stabbing_skill = get_skill_level( skill_stabbing );

    if( has_active_bionic( "bio_cqb" ) ) {
        stabbing_skill = 5;
    }

    if( weap.has_flag("UNARMED_WEAPON") ) {
        const bool left_empty = !natural_attack_restricted_on(bp_hand_l);
        const bool right_empty = !natural_attack_restricted_on(bp_hand_r) &&
            weap.is_null();
        if( left_empty || right_empty ) {
            float per_hand = 0.0f;
            if( has_trait("CLAWS") || has_active_mutation("CLAWS_RETRACT") ) {
                per_hand += 3;
            }

            if( has_trait("NAILS") ) {
                per_hand += .5;
            }

            if( has_bionic("bio_razors") ) {
                per_hand += 2;
            }

            if( has_trait("THORNS") ) {
                per_hand += 2;
            }

            if( has_trait("CLAWS_ST") ) {
                ///\EFFECT_UNARMED increases stabbing damage with CLAWS_ST
                per_hand += 3 + (unarmed_skill / 2);
            }

            cut_dam += per_hand; // First hand
            if( left_empty && right_empty ) {
                // Second hand
                cut_dam += per_hand;
            }
        }
    }

    if( cut_dam <= 0 ) {
        return; // No negative stabbing!
    }

    float stab_mul = 1.0f;
    // 66%, 76%, 86%, 96%, 106%, 116%, 122%, 128%, 134%, 140%
    ///\EFFECT_STABBING increases stabbing damage multiplier
    if( stabbing_skill <= 5 ) {
        stab_mul = 0.66 + 0.1 * stabbing_skill;
    } else {
        stab_mul = 0.86 + 0.06 * stabbing_skill;
    }

    stab_mul *= mabuff_cut_mult();
    float armor_mult = 1.0f;

    if( crit ) {
        // Crit damage bonus for stabbing scales with skill
        stab_mul *= 1.0 + (stabbing_skill / 10.0);
        // Stab criticals have extra extra %arpen
        armor_mult = 0.66f;
    }

    di.add_damage( DT_STAB, cut_dam, 0, armor_mult, stab_mul );
}

// Chance of a weapon sticking is based on weapon attack type.
// Only an issue for cutting and piercing weapons.
// Attack modes are "CHOP", "STAB", and "SLICE".
// "SPEAR" is synonymous with "STAB".
// Weapons can have a "low_stick" flag indicating they
// Have a feature to prevent sticking, such as a spear with a crossbar,
// Or a stabbing blade designed to resist sticking.
int player::roll_stuck_penalty(bool stabbing, const ma_technique &tec) const
{
    (void)tec;
    // The cost of the weapon getting stuck, in units of move points.
    const int weapon_speed = attack_speed( weapon );
    int stuck_cost = weapon_speed;
    int attack_skill = stabbing ? get_skill_level( skill_stabbing ) : get_skill_level( skill_cutting );

    if( has_active_bionic("bio_cqb") ) {
        attack_skill = 5;
    }

    const float cut_damage = weapon.damage_cut();
    const float bash_damage = weapon.damage_bash();
    float cut_bash_ratio = 0.0;

    // Scale cost along with the ratio between cutting and bashing damage of the weapon.
    if( cut_damage > 0.0f || bash_damage > 0.0f ) {
        cut_bash_ratio = cut_damage / ( cut_damage + bash_damage );
    }
    stuck_cost *= cut_bash_ratio;

    if( weapon.has_flag("SLICE") ) {
        // Slicing weapons assumed to have a very low chance of sticking.
        stuck_cost *= 0.25;
    } else if( weapon.has_flag("STAB") ) {
        // Stabbing has a moderate change of sticking.
        stuck_cost *= 0.50;
    } else if( weapon.has_flag("SPEAR") ) {
        // Spears should be a bit easier to manage
        stuck_cost *= 0.25;
    } else if( weapon.has_flag("CHOP") ) {
        // Chopping has a high chance of sticking.
        stuck_cost *= 1.00;
    } else {
        // Items with no attack type are assumed to be improvised weapons,
        // and get a very high stick cost.
        stuck_cost *= 2.00;
    }

    if( weapon.has_flag("NON_STUCK") ) {
        // Greatly reduce sticking frequency/severity if the weapon has an anti-sticking feature.
        stuck_cost /= 4;
    }

    // Reduce cost based on player skill, by 10.5 move/level on average.
    ///\EFFECT_STABBING reduces duration of stuck stabbing weapon

    ///\EFFECT_CUTTING reduces duration of stuck cutting weapon
    stuck_cost -= dice( attack_skill, 20 );
    // And also strength. This time totally reliable 5 moves per point
    ///\EFFECT_STR reduce duration of weapon getting stuck
    stuck_cost -= 5 * str_cur;
    // Make sure cost doesn't go negative.
    stuck_cost = std::max( stuck_cost, 0 );

    // Cap stuck penalty at 2x weapon speed
    stuck_cost = std::min( stuck_cost, 2 * weapon_speed );

    return stuck_cost;
}

matec_id player::pick_technique(Creature &t,
                                bool crit, bool dodge_counter, bool block_counter)
{

    std::vector<matec_id> all = get_all_techniques();

    std::vector<matec_id> possible;

    bool downed = t.has_effect( effect_downed);

    // first add non-aoe tecs
    for( auto & tec_id : all ) {
        const ma_technique &tec = tec_id.obj();

        // ignore "dummy" techniques like WBLOCK_1
        if (tec.dummy) {
            continue;
        }

        // skip defensive techniques
        if (tec.defensive) {
            continue;
        }

        // skip normal techniques if looking for a dodge counter
        if (dodge_counter && !tec.dodge_counter) {
            continue;
        }

        // skip normal techniques if looking for a block counter
        if (block_counter && !tec.block_counter) {
            continue;
        }

        // if crit then select only from crit tecs
        // dodge and blocks roll again for their attack, so ignore crit state
        if (!dodge_counter && !block_counter && ((crit && !tec.crit_tec) || (!crit && tec.crit_tec)) ) {
            continue;
        }

        // don't apply downing techniques to someone who's already downed
        if (downed && tec.down_dur > 0) {
            continue;
        }

        // don't apply disarming techniques to someone without a weapon
        // TODO: these are the stat reqs for tec_disarm
        // dice(   dex_cur +    get_skill_level("unarmed"),  8) >
        // dice(p->dex_cur + p->get_skill_level("melee"),   10))
        if (tec.disarms && !t.has_weapon()) {
            continue;
        }

        // if aoe, check if there are valid targets
        if( !tec.aoe.empty() && !valid_aoe_technique(t, tec) ) {
            continue;
        }

        // If we have negative weighting then roll to see if it's valid this time
        if (tec.weighting < 0 && !one_in(abs(tec.weighting))) {
            continue;
        }

        if (tec.is_valid_player(*this)) {
            possible.push_back(tec.id);

            //add weighted options into the list extra times, to increase their chance of being selected
            if (tec.weighting > 1) {
                for (int i = 1; i < tec.weighting; i++) {
                    possible.push_back(tec.id);
                }
            }
        }
    }

    return random_entry( possible, tec_none );
}

bool player::valid_aoe_technique( Creature &t, const ma_technique &technique )
{
    std::vector<int> dummy_mon_targets;
    std::vector<int> dummy_npc_targets;
    return valid_aoe_technique( t, technique, dummy_mon_targets, dummy_npc_targets );
}

bool player::valid_aoe_technique( Creature &t, const ma_technique &technique,
                                  std::vector<int> &mon_targets, std::vector<int> &npc_targets )
{
    if( technique.aoe.empty() ) {
        return false;
    }

    //wide hits all targets adjacent to the attacker and the target
    if( technique.aoe == "wide" ) {
        //check if either (or both) of the squares next to our target contain a possible victim
        //offsets are a pre-computed matrix allowing us to quickly lookup adjacent squares
        int offset_a[] = {0 ,-1,-1,1 ,0 ,-1,1 ,1 ,0 };
        int offset_b[] = {-1,-1,0 ,-1,0 ,1 ,0 ,1 ,1 };

        int lookup = t.posy() - posy() + 1 + (3 * (t.posx() - posx() + 1));

        tripoint left = pos() + tripoint( offset_a[lookup], offset_b[lookup], 0 );
        tripoint right = pos() + tripoint( offset_b[lookup], -offset_a[lookup], 0 );

        int mondex_l = g->mon_at( left );
        int mondex_r = g->mon_at( right );
        if (mondex_l != -1 && g->zombie(mondex_l).friendly == 0) {
            mon_targets.push_back(mondex_l);
        }
        if (mondex_r != -1 && g->zombie(mondex_r).friendly == 0) {
            mon_targets.push_back(mondex_r);
        }

        int npcdex_l = g->npc_at( left );
        int npcdex_r = g->npc_at( right );
        if (npcdex_l != -1 && g->active_npc[npcdex_l]->attitude == NPCATT_KILL) {
            npc_targets.push_back(npcdex_l);
        }
        if (npcdex_r != -1 && g->active_npc[npcdex_r]->attitude == NPCATT_KILL) {
            npc_targets.push_back(npcdex_r);
        }
        if(!(npc_targets.empty() && mon_targets.empty())) {
            return true;
        }
    }

    if( technique.aoe == "impale" ) {
        // Impale hits the target and a single target behind them
        // Check if the square cardinally behind our target, or to the left / right,
        // contains a possible target.
        // Offsets are a pre-computed matrix allowing us to quickly lookup adjacent squares.
        int offset_a[] = {0 ,-1,-1,1 ,0 ,-1,1 ,1 ,0 };
        int offset_b[] = {-1,-1,0 ,-1,0 ,1 ,0 ,1 ,1 };

        int lookup = t.posy() - posy() + 1 + (3 * (t.posx() - posx() + 1));

        tripoint left = t.pos() + tripoint( offset_a[lookup], offset_b[lookup], 0 );
        tripoint target_pos = t.pos() + (t.pos() - pos());
        tripoint right = t.pos() + tripoint( offset_b[lookup], -offset_b[lookup], 0 );

        int mondex_l = g->mon_at( left );
        int mondex_t = g->mon_at( target_pos );
        int mondex_r = g->mon_at( right );
        if (mondex_l != -1 && g->zombie(mondex_l).friendly == 0) {
            mon_targets.push_back(mondex_l);
        }
        if (mondex_t != -1 && g->zombie(mondex_t).friendly == 0) {
            mon_targets.push_back(mondex_t);
        }
        if (mondex_r != -1 && g->zombie(mondex_r).friendly == 0) {
            mon_targets.push_back(mondex_r);
        }

        int npcdex_l = g->npc_at( left );
        int npcdex_t = g->npc_at( target_pos );
        int npcdex_r = g->npc_at( right );
        if (npcdex_l != -1 && g->active_npc[npcdex_l]->attitude == NPCATT_KILL) {
            npc_targets.push_back(npcdex_l);
        }
        if (npcdex_t != -1 && g->active_npc[npcdex_t]->attitude == NPCATT_KILL) {
            npc_targets.push_back(npcdex_t);
        }
        if (npcdex_r != -1 && g->active_npc[npcdex_r]->attitude == NPCATT_KILL) {
            npc_targets.push_back(npcdex_r);
        }
        if(!(npc_targets.empty() && mon_targets.empty())) {
            return true;
        }
    }

    if( npc_targets.empty() && mon_targets.empty() && technique.aoe == "spin" ) {
        tripoint tmp;
        tmp.z = posz();
        for ( tmp.x = posx() - 1; tmp.x <= posx() + 1; tmp.x++) {
            for ( tmp.y = posy() - 1; tmp.y <= posy() + 1; tmp.y++) {
                if( tmp == t.pos() ) {
                    continue;
                }
                int mondex = g->mon_at( tmp );
                if (mondex != -1 && g->zombie(mondex).friendly == 0) {
                    mon_targets.push_back(mondex);
                }
                int npcdex = g->npc_at( tmp );
                if (npcdex != -1 && g->active_npc[npcdex]->attitude == NPCATT_KILL) {
                    npc_targets.push_back(npcdex);
                }
            }
        }
        //don't trigger circle for fewer than 2 targets
        if( npc_targets.size() + mon_targets.size() < 2 ) {
            npc_targets.clear();
            mon_targets.clear();
        } else {
            return true;
        }
    }
    return false;
}

bool player::has_technique( const matec_id &id ) const
{
    return weapon.has_technique( id ) ||
           style_selected.obj().has_technique( *this, id );
}

damage_unit &get_damage_unit( std::vector<damage_unit> &di, const damage_type dt )
{
    static damage_unit nullunit( DT_NULL, 0, 0, 0, 0 );
    for( auto &du : di ) {
        if( du.type == dt && du.amount > 0 ) {
            return du;
        }
    }

    return nullunit;
}

void player::perform_technique(const ma_technique &technique, Creature &t, damage_instance &di, int &move_cost)
{
    auto &bash = get_damage_unit( di.damage_units, DT_BASH );
    auto &cut  = get_damage_unit( di.damage_units, DT_CUT );
    auto &stab = get_damage_unit( di.damage_units, DT_STAB );

    add_msg( m_debug, _("damage before tec mult:%dbash,%dcut,%dstab,%dtotal"), (int)di.type_damage(DT_BASH), (int)di.type_damage(DT_CUT), (int)di.type_damage(DT_STAB), (int)di.total_damage());
    if( bash.amount > 0 ) {
        bash.amount += technique.bash;
        bash.damage_multiplier *= technique.bash_mult;
    }

    // Cut affects stab damage too since only one of cut/stab is used
    if( cut.amount > 0 && cut.amount > stab.amount ) {
        cut.amount += technique.cut;
        cut.damage_multiplier *= technique.cut_mult;
    } else if( stab.amount > 0 ) {
        stab.amount += technique.cut;
        stab.damage_multiplier *= technique.cut_mult;
    }
    add_msg( m_debug, _("damage after tec mult:%dbash,%dcut,%dstab,%dtotal"), (int)di.type_damage(DT_BASH), (int)di.type_damage(DT_CUT), (int)di.type_damage(DT_STAB), (int)di.total_damage());

    move_cost *= technique.speed_mult;

    if( technique.down_dur > 0 ) {
        if( t.get_throw_resist() == 0 ) {
            t.add_effect( effect_downed, rng(1, technique.down_dur));
            if( bash.amount > 0 ) {
                bash.amount += 3;
            }
        }
    }

    if (technique.stun_dur > 0) {
        t.add_effect( effect_stunned, rng(1, technique.stun_dur));
    }

    if (technique.knockback_dist > 0) {
        const int kb_offset_x = rng( -technique.knockback_spread,
                                     technique.knockback_spread );
        const int kb_offset_y = rng( -technique.knockback_spread,
                                     technique.knockback_spread );
        tripoint kb_point( posx() + kb_offset_x, posy() + kb_offset_y, posz() );
        t.knock_back_from( kb_point );
    }

    if (technique.pain > 0) {
        t.pain += rng(technique.pain / 2, technique.pain);
    }

    player *p = dynamic_cast<player*>( &t );
    if( technique.disarms && p != nullptr && p->is_armed() ) {
        g->m.add_item_or_charges( p->pos(), p->remove_weapon() );
        if( p->is_player() ) {
            add_msg_if_npc( _("<npcname> disarms you!") );
        } else {
            add_msg_player_or_npc( _("You disarm %s!"),
                                   _("<npcname> disarms %s!"),
                                   p->name.c_str() );
        }
    }

    //AOE attacks, feel free to skip over this lump
    if (technique.aoe.length() > 0) {
        // Remember out moves and stamina
        // We don't want to consume them for every attack!
        const int temp_moves = moves;
        const int temp_stamina = stamina;

        std::vector<int> mon_targets = std::vector<int>();
        std::vector<int> npc_targets = std::vector<int>();

        valid_aoe_technique( t, technique, mon_targets, npc_targets );

        //hit only one valid target (pierce through doesn't spread out)
        if (technique.aoe == "impale") {
            size_t victim = rng(0, mon_targets.size() + npc_targets.size() - 1);
            if (victim >= mon_targets.size()) {
                victim -= mon_targets.size();
                mon_targets.clear();
                int npc_id = npc_targets[victim];
                npc_targets.clear();
                npc_targets.push_back(npc_id);
            } else {
                npc_targets.clear();
                int mon_id = mon_targets[victim];
                mon_targets.clear();
                mon_targets.push_back(mon_id);
            }
        }

        //hit the targets in the lists (all candidates if wide or burst, or just the unlucky sod if deep)
        int count_hit = 0;
        for (auto &i : mon_targets) {
            if (hit_roll() >= rng(0, 5) + g->zombie(i).dodge_roll()) {
                count_hit++;
                melee_attack(g->zombie(i), false);

                std::string temp_target = string_format(_("the %s"), g->zombie(i).name().c_str());
                add_msg_player_or_npc( m_good, _("You hit %s!"), _("<npcname> hits %s!"), temp_target.c_str() );
            }
        }
        for (auto &i : npc_targets) {
            if (hit_roll() >= rng(0, 5) + g->active_npc[i]->dodge_roll()) {
                count_hit++;
                melee_attack(*g->active_npc[i], false);

                add_msg_player_or_npc( m_good, _("You hit %s!"), _("<npcname> hits %s!"),
                                          g->active_npc[i]->name.c_str() );
            }
        }

        t.add_msg_if_player(m_good, ngettext("%d enemy hit!", "%d enemies hit!", count_hit), count_hit);
        // Extra attacks are free of charge (otherwise AoE attacks would SUCK)
        moves = temp_moves;
        stamina = temp_stamina;
    }

    //player has a very small chance, based on their intelligence, to learn a style whilst using the cqb bionic
    if (has_active_bionic("bio_cqb") && !has_martialart(style_selected)) {
        ///\EFFECT_INT slightly increases chance to learn techniques when using CQB bionic
        if (one_in(1400 - (get_int() * 50))) {
            ma_styles.push_back(style_selected);
            add_msg_if_player(m_good, _("You have learnt %s from extensive practice with the CQB Bionic."),
                       style_selected.obj().name.c_str());
        }
    }
}

// this would be i2amroy's fix, but it's kinda handy
bool player::can_weapon_block() const
{
    return (weapon.has_technique( WBLOCK_1 ) ||
            weapon.has_technique( WBLOCK_2 ) ||
            weapon.has_technique( WBLOCK_3 ));
}

bool player::block_hit(Creature *source, body_part &bp_hit, damage_instance &dam) {

    // Shouldn't block if player is asleep; this only seems to be used by player.
    // TODO: It should probably be moved to the section that regenerates blocks
    // and to effects that disallow blocking
    if( blocks_left < 1 || in_sleep_state() ) {
        return false;
    }
    blocks_left--;

    ma_ongethit_effects(); // fire martial arts on-getting-hit-triggered effects
    // these fire even if the attack is blocked (you still got hit)

    // This bonus absorbs damage from incoming attacks before they land,
    // but it still counts as a block even if it absorbs all the damage.
    float total_phys_block = mabuff_block_bonus();
    // Extract this to make it easier to implement shields/multiwield later
    item &shield = weapon;
    bool conductive_shield = shield.conductive();
    bool unarmed = unarmed_attack();

    // This gets us a number between:
    // str ~0 + skill 0 = 0
    // str ~20 + skill 10 + 10(unarmed skill or weapon bonus) = 40
    int block_score = 1;
    // Remember if we're using a weapon or a limb to block.
    // So that we don't suddenly switch that for any reason.
    const bool weapon_blocking = can_weapon_block();
    if( weapon_blocking ) {
        int block_bonus = 2;
        if (shield.has_technique( WBLOCK_3 )) {
            block_bonus = 10;
        } else if (shield.has_technique( WBLOCK_2 )) {
            block_bonus = 6;
        } else if (shield.has_technique( WBLOCK_1 )) {
            block_bonus = 4;
        }
        ///\EFFECT_STR increases attack blocking effectiveness with a weapon

        ///\EFFECT_MELEE increases attack blocking effectiveness with a weapon
        block_score = str_cur + block_bonus + (int)get_skill_level( skill_melee );
    } else if( can_limb_block() ) {
        ///\EFFECT_STR increases attack blocking effectiveness with a limb

        ///\EFFECT_UNARMED increases attack blocking effectivness with a limb
        block_score = str_cur + (int)get_skill_level( skill_melee ) + (int)get_skill_level( skill_unarmed );
    } else {
        // Can't block with items, can't block with limbs
        // What were we supposed to be blocking with then?
        return false;
    }

    // Map block_score to the logistic curve for a number between 1 and 0.
    // Basic beginner character (str 8, skill 0, basic weapon)
    // Will have a score around 10 and block about %15 of incoming damage.
    // More proficient melee character (str 10, skill 4, wbock_2 weapon)
    // will have a score of 20 and block about 45% of damage.
    // A highly expert character (str 14, skill 8 wblock_2)
    // will have a score in the high 20s and will block about 80% of damage.
    // As the block score approaches 40, damage making it through will dwindle
    // to nothing, at which point we're relying on attackers hitting enough to drain blocks.
    const float physical_block_multiplier = logarithmic_range( 0, 40, block_score );

    float total_damage = 0.0;
    float damage_blocked = 0.0;
    for( auto &elem : dam.damage_units ) {
        total_damage += elem.amount;
        // block physical damage "normally"
        if( elem.type == DT_BASH || elem.type == DT_CUT || elem.type == DT_STAB ) {
            // use up our flat block bonus first
            float block_amount = std::min( total_phys_block, elem.amount );
            total_phys_block -= block_amount;
            elem.amount -= block_amount;
            damage_blocked += block_amount;
            if( elem.amount <= std::numeric_limits<float>::epsilon() ) {
                continue;
            }

            float previous_amount = elem.amount;
            elem.amount *= physical_block_multiplier;
            damage_blocked += previous_amount - elem.amount;
        // non-electrical "elemental" damage types do their full damage if unarmed,
        // but severely mitigated damage if not
        } else if( elem.type == DT_HEAT || elem.type == DT_ACID || elem.type == DT_COLD ) {
            // Unarmed weapons won't block those
            if( weapon_blocking && !unarmed ) {
                float previous_amount = elem.amount;
                elem.amount /= 5;
                damage_blocked += previous_amount - elem.amount;
            }
        // electrical damage deals full damage if unarmed OR wielding a
        // conductive weapon
        } else if( elem.type == DT_ELECTRIC ) {
            // Unarmed weapons and conductive weapons won't block this
            if( weapon_blocking && !unarmed && !conductive_shield ) {
                float previous_amount = elem.amount;
                elem.amount /= 5;
                damage_blocked += previous_amount - elem.amount;
            }
        }
    }

    ma_onblock_effects(); // fire martial arts block-triggered effects

    // weapon blocks are preferred to limb blocks
    std::string thing_blocked_with;
    if( weapon_blocking ) {
        thing_blocked_with = shield.tname();
        // TODO: Change this depending on damage blocked
        float wear_modifier = 1.0f;
        handle_melee_wear( shield, wear_modifier );
    } else {
        //Choose which body part to block with, assume left side first
        if (can_leg_block() && can_arm_block()) {
            bp_hit = one_in(2) ? bp_leg_l : bp_arm_l;
        } else if (can_leg_block()) {
            bp_hit = bp_leg_l;
        } else {
            bp_hit = bp_arm_l;
        }

        // Check if we should actually use the right side to block
        if (bp_hit == bp_leg_l) {
            if (hp_cur[hp_leg_r] > hp_cur[hp_leg_l]) {
                bp_hit = bp_leg_r;
            }
        } else {
            if (hp_cur[hp_arm_r] > hp_cur[hp_arm_l]) {
                bp_hit = bp_arm_r;
            }
        }

        thing_blocked_with = body_part_name(bp_hit);
    }

    std::string damage_blocked_description;
    // good/bad/ugly add_msg color code?
    // none, hardly any, a little, some, most, all
    float blocked_ratio = (total_damage - damage_blocked) / total_damage;
    if( blocked_ratio < std::numeric_limits<float>::epsilon() ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _("all");
    } else if( blocked_ratio < 0.2 ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _("nearly all");
    } else if( blocked_ratio < 0.4 ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _("most");
    } else if( blocked_ratio < 0.6 ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _("a lot");
    } else if( blocked_ratio < 0.8 ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _("some");
    } else if( blocked_ratio > std::numeric_limits<float>::epsilon() ){
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _("a little");
    } else {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _("none");
    }
    add_msg_player_or_npc( _("You block %1$s of the damage with your %2$s!"),
                           _("<npcname> blocks %1$s of the damage with their %2$s!"),
                           damage_blocked_description.c_str(), thing_blocked_with.c_str() );

    // check if we have any dodge counters
    matec_id tec = pick_technique( *source, false, false, true );

    if( tec != tec_none ) {
        melee_attack(*source, false, tec);
    }

    return true;
}

void player::perform_special_attacks(Creature &t)
{
    bool can_poison = false;

    std::vector<special_attack> special_attacks = mutation_attacks(t);

    std::string target = t.disp_name();

    bool practiced = false;
    for( auto &special_attack : special_attacks ) {
        if( t.is_dead_state() ) {
            break;
        }

        dealt_damage_instance dealt_dam;

        int hit_spread = t.deal_melee_attack(this, hit_roll() * 0.8);
        if (hit_spread >= 0) {
            t.deal_melee_hit(
                this, hit_spread, false,
                damage_instance::physical( special_attack.bash,
                    special_attack.cut, special_attack.stab ),
                dealt_dam );
            if( !practiced ) {
                // Practice unarmed, at most once per combo
                practiced = true;
                practice( skill_unarmed, rng( 0, 10 ) );
            }
        }
        int dam = dealt_dam.total_damage();
        if( dam > 0 ) {
            player_hit_message( this, special_attack.text, t, dam );
        }

        can_poison = can_poison ||
            dealt_dam.type_damage( DT_CUT ) > 0 ||
            dealt_dam.type_damage( DT_STAB ) > 0;
    }

    if( can_poison && (has_trait("POISONOUS") || has_trait("POISONOUS2")) ) {
        if( has_trait("POISONOUS") ) {
            t.add_msg_if_player(m_good, _("You poison %s!"), target.c_str());
            t.add_effect( effect_poison, 6);
        } else if( has_trait("POISONOUS2") ) {
            t.add_msg_if_player(m_good, _("You inject your venom into %s!"), target.c_str());
            t.add_effect( effect_badpoison, 6);
        }
    }
}

std::string player::melee_special_effects(Creature &t, damage_instance &d, const ma_technique &tec)
{
    std::stringstream dump;

    std::string target;

    target = t.disp_name();

    tripoint tarpos = t.pos();

    // Bonus attacks!
    bool shock_them = (has_active_bionic("bio_shock") && power_level >= 2 &&
                       (unarmed_attack() || weapon.made_of("iron") ||
                        weapon.made_of("steel") || weapon.made_of("silver") ||
                        weapon.made_of("gold") || weapon.made_of("superalloy")) && one_in(3));

    bool drain_them = (has_active_bionic("bio_heat_absorb") && power_level >= 1 &&
                       !is_armed() && t.is_warm());
    drain_them &= one_in(2); // Only works half the time

    bool burn_them = weapon.has_flag("FLAMING");


    if (shock_them) { // bionics only
        charge_power(-2);
        int shock = rng(2, 5);
        d.add_damage(DT_ELECTRIC, shock * rng(1, 3));

        if (is_player()) {
            dump << string_format(_("You shock %s."), target.c_str()) << std::endl;
        } else
            add_msg_player_or_npc(m_good, _("You shock %s."),
                                          _("<npcname> shocks %s."),
                                          target.c_str());
    }

    if (drain_them) { // bionics only
        power_level--;
        charge_power(rng(0, 2));
        d.add_damage(DT_COLD, 1);
        if (t.is_player()) {
            add_msg_if_npc(m_bad, _("<npcname> drains your body heat!"));
        } else {
            if (is_player()) {
                dump << string_format(_("You drain %s's body heat."), target.c_str()) << std::endl;
            } else
                add_msg_player_or_npc(m_good, _("You drain %s's body heat!"),
                                              _("<npcname> drains %s's body heat!"),
                                              target.c_str());
        }
    }

    if (burn_them) { // for flaming weapons
        d.add_damage(DT_HEAT, rng(1, 8));

        if (is_player()) {
            dump << string_format(_("You burn %s."), target.c_str()) << std::endl;
        } else
            add_msg_player_or_npc(m_good, _("You burn %s."),
                                     _("<npcname> burns %s."),
                                     target.c_str());
    }

    //Hurting the wielder from poorly-chosen weapons
    if(weapon.has_flag("HURT_WHEN_WIELDED") && x_in_y(2, 3)) {
        add_msg_if_player(m_bad, _("The %s cuts your hand!"), weapon.tname().c_str());
        deal_damage( nullptr, bp_hand_r, damage_instance::physical(0, weapon.damage_cut(), 0) );
        if( weapon.is_two_handed(*this) ) { // Hurt left hand too, if it was big
            deal_damage( nullptr, bp_hand_l, damage_instance::physical(0, weapon.damage_cut(), 0) );
        }
    }

    // Glass weapons shatter sometimes
    if (weapon.made_of("glass") &&
        ///\EFFECT_STR increases chance of breaking glass weapons (NEGATIVE)
        rng(0, weapon.volume() + 8) < weapon.volume() + str_cur) {
        if (is_player()) {
            dump << string_format(_("Your %s shatters!"), weapon.tname().c_str()) << std::endl;
        } else {
            add_msg_player_or_npc(m_bad, _("Your %s shatters!"),
                                     _("<npcname>'s %s shatters!"),
                                     weapon.tname().c_str());
        }

        sounds::sound( pos(), 16, "" );
        // Dump its contents on the ground
        for( auto &elem : weapon.contents ) {
            g->m.add_item_or_charges( pos(), elem );
        }
        // Take damage
        deal_damage( nullptr, bp_arm_r, damage_instance::physical(0, rng(0, weapon.volume() * 2), 0) );
        if( weapon.is_two_handed(*this) ) {// Hurt left arm too, if it was big
            //redeclare shatter_dam because deal_damage mutates it
            deal_damage( nullptr, bp_arm_l, damage_instance::physical(0, rng(0, weapon.volume() * 2), 0) );
        }
        d.add_damage(DT_CUT, rng(0, 5 + int(weapon.volume() * 1.5)));// Hurt the monster extra
        remove_weapon();
    }

    handle_melee_wear();

    bool is_hallucination = false; //Check if the target is an hallucination.
    if(monster *m = dynamic_cast<monster *>(&t)) { //Cast fails if the t is an NPC or the player.
        is_hallucination = m->is_hallucination();
    }

    // The skill used to counter stuck penalty
    const bool stab = d.type_damage(DT_STAB) > d.type_damage(DT_CUT);
    int used_skill = stab ? get_skill_level( skill_stabbing ) : get_skill_level( skill_cutting );
    if( has_active_bionic("bio_cqb") ) {
        used_skill = 5;
    }

    int cutting_penalty = roll_stuck_penalty( stab, tec );
    // Some weapons splatter a lot of blood around.
    // TODO: this function shows total damage done by this attack, not final damage inflicted.
    if (d.total_damage() > 10 && weapon.has_flag("MESSY") ) { //Check if you do non minor damage
        cutting_penalty /= 6; // Harder to get stuck
        //Get blood type.
        field_id type_blood = fd_blood;
        if(!is_hallucination || t.has_flag(MF_VERMIN)) {
            type_blood = t.bloodType();
        } else {
            type_blood = fd_null;
        }
        if(type_blood != fd_null) {
            tripoint tmp;
            tmp.z = tarpos.z;
            for( tmp.x = tarpos.x - 1; tmp.x <= tarpos.x + 1; tmp.x++ ) {
                for( tmp.y = tarpos.y - 1; tmp.y <= tarpos.y + 1; tmp.y++ ) {
                    if( !one_in(3) ) {
                        g->m.add_field( tmp, type_blood, 1, 0 );
                    }
                }
            }
        }
    }
    // Getting your weapon stuck
    ///\EFFECT_STR decreases chance of getting weapon stuck
    if (!unarmed_attack() && cutting_penalty > dice(str_cur * 2, 20) && !is_hallucination) {
        dump << string_format(_("Your %1$s gets stuck in %2$s, pulling it out of your hands!"),
                              weapon.tname().c_str(), target.c_str());
        // TODO: better speed debuffs for target, possibly through effects
        if (monster *m = dynamic_cast<monster *>(&t)) {
            m->add_item(remove_weapon());
        } else {
            // Happens if 't' is not of 'monster' origin (attacking an NPC)
            g->m.add_item_or_charges( tarpos, remove_weapon(), 1 );
        }

        t.mod_moves(-30);
        if (weapon.has_flag("HURT_WHEN_PULLED") && one_in(3)) {
            //Sharp objects that injure wielder when pulled from hands (so cutting damage only)
            dump << std::endl << string_format(_("You are hurt by the %s being pulled from your hands!"),
                                               weapon.tname().c_str());
            auto hand_hurt = one_in(2) ? bp_hand_l : bp_hand_r;
            deal_damage( this, hand_hurt, damage_instance::physical( 0, weapon.damage_cut() / 2, 0) );
        }
    } else {
        // Approximately "this would kill the target". Unless the target has limb-based hp.
        if( d.total_damage() > t.get_hp() ) {
            cutting_penalty /= 2;
            ///\EFFECT_STABBING reduces chance of weapon getting stuck, if STABBING>CUTTING

            ///\EFFECT_CUTTING reduces chance of weapon getting stuck, if CUTTING>STABBING
            cutting_penalty -= rng( used_skill, used_skill * 2 + 2 );
        }
        if( cutting_penalty >= 50 && !is_hallucination ) {
            dump << string_format(_("Your %1$s gets stuck in %2$s but you yank it free!"), weapon.tname().c_str(),
                                  target.c_str());
        }

        if( weapon.has_flag("SPEAR") ) {
            ///\EFFECT_STABBING increases damage when pulling out a stuck spear
            const int stabbing_skill = get_skill_level( skill_stabbing );
            d.add_damage( DT_CUT, rng( 1, stabbing_skill ) ); //add some extra damage for pulling out a spear
            t.mod_moves(-30);
        }
    }
    if( cutting_penalty > 0 && !is_hallucination ) {
        // Don't charge more than 1 turn of stuck penalty
        // It scales with attack time and low attack time is bad enough on its own
        mod_moves( std::max( -100, -cutting_penalty ) );
    }

    // on-hit effects for martial arts
    ma_onhit_effects();

    return dump.str();
}

std::vector<special_attack> player::mutation_attacks(Creature &t) const
{
    std::vector<special_attack> ret;

    std::string target = t.disp_name();

    ///\EFFECT_DEX increases chance of attacking with SABER_TEETH

    ///\EFFECT_UNARMED increases chance of attacking with SABER_TEETH
    if ( (has_trait("SABER_TEETH")) && !natural_attack_restricted_on(bp_mouth) &&
         one_in(20 - dex_cur - get_skill_level( skill_unarmed )) ) {
        special_attack tmp;
        ///\EFFECT_STR increases damage with SABER_TEETH attack
        tmp.stab = (25 + str_cur);
        if (is_player()) {
            tmp.text = string_format(_("You tear into %s with your saber teeth"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s tears into %2$s with his saber teeth"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s tears into %2$s with her saber teeth"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    // Having lupine or croc jaws makes it much easier to sink your fangs into people;
    // Ursine/Feline, not so much.  Rat is marginally better.
    ///\EFFECT_DEX increases chance of attacking with FANGS or FANGS + (MUZZLE, MUZZLE_RAT, MUZZLE_LONG)

    ///\EFFECT_UNARMED increases chance of attacking with FANGS or FANGS + (MUZZLE, MUZZLE_RAT, MUZZLE_LONG)
    if (has_trait("FANGS") && (!natural_attack_restricted_on(bp_mouth)) &&
        ((!has_trait("MUZZLE") && !has_trait("MUZZLE_LONG") && !has_trait("MUZZLE_RAT") &&
          one_in(20 - dex_cur - get_skill_level( skill_unarmed ))) ||
         (has_trait("MUZZLE_RAT") && one_in(19 - dex_cur - get_skill_level( skill_unarmed ))) ||
         (has_trait("MUZZLE") && one_in(18 - dex_cur - get_skill_level( skill_unarmed ))) ||
         (has_trait("MUZZLE_LONG") && one_in(15 - dex_cur - get_skill_level( skill_unarmed ))))) {
        special_attack tmp;
        tmp.stab = 20;
        if (is_player()) {
            tmp.text = string_format(_("You sink your fangs into %s"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s sinks his fangs into %2$s"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s sinks her fangs into %2$s"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with INCISORS

    ///\EFFECT_UNARMED increases chance of attacking with INCISORS
    if (has_trait("INCISORS") && one_in(18 - dex_cur - get_skill_level( skill_unarmed )) &&
        (!natural_attack_restricted_on(bp_mouth))) {
        special_attack tmp;
        tmp.cut = 3;
        tmp.bash = 3;
        if (is_player()) {
            tmp.text = string_format(_("You bite into %s with your ratlike incisors"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s bites %2$s with his ratlike incisors"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s bites %2$s with her ratlike incisors"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with FANGS + MUZZLE

    ///\EFFECT_UNARMED increases chance of attacking with FANGS + MUZZLE
    if (!has_trait("FANGS") && has_trait("MUZZLE") &&
        one_in(18 - dex_cur - get_skill_level( skill_unarmed )) &&
        (!natural_attack_restricted_on(bp_mouth))) {
        special_attack tmp;
        tmp.cut = 4;
        if (is_player()) {
            tmp.text = string_format(_("You nip at %s"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s nips and harries %2$s"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s nips and harries %2$s"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with MUZZLE_BEAR

    ///\EFFECT_UNARMED increases chance of attacking with MUZZLE_BEAR
    if (!has_trait("FANGS") && has_trait("MUZZLE_BEAR") &&
        one_in(20 - dex_cur - get_skill_level( skill_unarmed )) &&
        (!natural_attack_restricted_on(bp_mouth))) {
        special_attack tmp;
        tmp.cut = 5;
        if (is_player()) {
            tmp.text = string_format(_("You bite %s"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s bites %2$s"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s bites %2$s"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with MUZZLE_LONG

    ///\EFFECT_UNARMED increases chance of attacking with MUZZLE_LONG
    if (!has_trait("FANGS") && has_trait("MUZZLE_LONG") &&
        one_in(18 - dex_cur - get_skill_level( skill_unarmed )) &&
        (!natural_attack_restricted_on(bp_mouth))) {
        special_attack tmp;
        tmp.stab = 18;
        if (is_player()) {
            tmp.text = string_format(_("You bite a chunk out of %s"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s bites a chunk out of %2$s"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s bites a chunk out of %2$s"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with MANDIBLES, trait FANGS_SPIDER

    ///\EFFECT_UNARMED increases chance of attacking with MANDIBLES, trait FANGS_SPIDER
    if ((has_trait("MANDIBLES") || (has_trait("FANGS_SPIDER") && !has_active_mutation("FANGS_SPIDER"))) &&
        one_in(22 - dex_cur - get_skill_level( skill_unarmed )) && (!natural_attack_restricted_on(bp_mouth))) {
        special_attack tmp;
        tmp.cut = 12;
        if (is_player()) {
            tmp.text = string_format(_("You slice %s with your mandibles"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s slices %2$s with his mandibles"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s slices %2$s with her mandibles"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with mutation FANGS_SPIDER

    ///\EFFECT_UNARMED increases chance of attacking with mutation FANGS_SPIDER
    if (has_active_mutation("FANGS_SPIDER") && one_in(24 - dex_cur - get_skill_level( skill_unarmed )) &&
        (!natural_attack_restricted_on(bp_mouth)) ) {
        special_attack tmp;
        tmp.stab = 15;
        if (is_player()) {
            tmp.text = string_format(_("You bite %s with your fangs"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s bites %2$s with his fangs"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s bites %2$s with her fangs"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with BEAK

    ///\EFFECT_UNARMED increases chance of attacking with BEAK
    if (has_trait("BEAK") && one_in(15 - dex_cur - get_skill_level( skill_unarmed )) &&
        (!natural_attack_restricted_on(bp_mouth))) {
        special_attack tmp;
        tmp.stab = 15;
        if (is_player()) {
            tmp.text = string_format(_("You peck %s"),
                                     target.c_str());
        } else {
            tmp.text = string_format(_("%1$s pecks %2$s"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with BEAK_PECK

    ///\EFFECT_UNARMED increases chance of attacking with BEAK_PECK
    if (has_trait("BEAK_PECK") && one_in(15 - dex_cur - get_skill_level( skill_unarmed )) &&
        (!natural_attack_restricted_on(bp_mouth))) {
        // method open to improvement, please feel free to suggest
        // a better way to simulate target's anti-peck efforts
        ///\EFFECT_DEX increases number of hits with BEAK_PECK

        ///\EFFECT_UNARMED increases number of hits with BEAK_PECK
        int num_hits = (dex_cur + get_skill_level( skill_unarmed ) - rng(4, 10));
        if (num_hits <= 0) {
            num_hits = 1;
        }
        // Yeah, arbitrary balance cap of Unfunness. :-(
        // Though this is a 6-second turn, so only so much
        // time to peck your target.
        if (num_hits >= 5) {
            num_hits = 5;
        }
        special_attack tmp;
        tmp.stab = (num_hits *= 10 );
        if (num_hits == 1) {
            if (is_player()) {
                tmp.text = string_format(_("You peck %s"),
                                         target.c_str());
            } else {
                tmp.text = string_format(_("%1$s pecks %2$s"),
                                         name.c_str(), target.c_str());
            }
        }
        //~"jackhammering" with the beak is metaphor for the rapid-peck
        //~commonly employed by a woodpecker drilling into wood
        else {
            if (is_player()) {
                tmp.text = string_format(_("You jackhammer into %s with your beak"),
                                         target.c_str());
            } else if (male) {
                tmp.text = string_format(_("%1$s jackhammers into %2$s with his beak"),
                                         name.c_str(), target.c_str());
            } else {
                tmp.text = string_format(_("%1$s jackhammers into %2$s with her beak"),
                                         name.c_str(), target.c_str());
            }
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with HOOVES

    ///\EFFECT_UNARMED increases chance of attacking with HOOVES
    if (has_trait("HOOVES") && one_in(25 - dex_cur - 2 * get_skill_level( skill_unarmed ))) {
        special_attack tmp;
        ///\EFFECT_STR increases damage with HOOVES
        tmp.bash = str_cur * 3;
        if (tmp.bash > 40) {
            tmp.bash = 40;
        }
        if (is_player()) {
            tmp.text = string_format(_("You kick %s with your hooves"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s kicks %2$s with his hooves"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s kicks %2$s with her hooves"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with RAP_TALONS

    ///\EFFECT_UNARMED increases chance of attacking with RAP_TALONS
    if (has_trait("RAP_TALONS") && one_in(30 - dex_cur - 2 * get_skill_level( skill_unarmed ))) {
        special_attack tmp;
        ///\EFFECT_STR increases damage with RAP_TALONS
        tmp.cut = str_cur * 4;
        if (tmp.cut > 60) {
            tmp.cut = 60;
        }
        if (is_player()) {
            tmp.text = string_format(_("You slash %s with a talon"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s slashes %2$s with a talon"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s slashes %2$s with a talon"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with HORNS

    ///\EFFECT_UNARMED increases chance of attacking with HORNS
    if (has_trait("HORNS") && one_in(20 - dex_cur - get_skill_level( skill_unarmed ))) {
        special_attack tmp;
        tmp.bash = 3;
        tmp.stab = 3;
        if (is_player()) {
            tmp.text = string_format(_("You headbutt %s with your horns"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s headbutts %2$s with his horns"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s headbutts %2$s with her horns"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with HORNS_CURLED

    ///\EFFECT_UNARMED increases chance of attacking with HORNS_CURLED
    if (has_trait("HORNS_CURLED") && one_in(20 - dex_cur - get_skill_level( skill_unarmed ))) {
        special_attack tmp;
        tmp.bash = 14;
        if (is_player()) {
            tmp.text = string_format(_("You headbutt %s with your curled horns"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s headbutts %2$s with his curled horns"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s headbutts %2$s with her curled horns"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with HORNS_POINTED

    ///\EFFECT_UNARMED increases chance of attacking with HORNS_POINTED
    if (has_trait("HORNS_POINTED") && one_in(22 - dex_cur - get_skill_level( skill_unarmed ))) {
        special_attack tmp;
        tmp.stab = 24;
        if (is_player()) {
            tmp.text = string_format(_("You stab %s with your pointed horns"),
                                     target.c_str());
        } else {
            tmp.text = string_format(_("%1$s stabs %2$s with their pointed horns"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with ANTLERS

    ///\EFFECT_UNARMED increases chance of attacking with ANTLERS
    if (has_trait("ANTLERS") && one_in(20 - dex_cur - get_skill_level( skill_unarmed ))) {
        special_attack tmp;
        tmp.bash = 4;
        if (is_player()) {
            tmp.text = string_format(_("You butt %s with your antlers"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s butts %2$s with his antlers"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s butts %2$s with her antlers"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with TAIL_STING
    if ( ((has_trait("TAIL_STING") && one_in(3)) || has_active_mutation("TAIL_STING")) &&
      one_in(10 - dex_cur)) {
        special_attack tmp;
        tmp.stab = 20;
        if (is_player()) {
            tmp.text = string_format(_("You sting %s with your tail"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s stings %2$s with his tail"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s stings %2$s with her tail"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with TAIL_CLUB
    if ( ((has_trait("TAIL_CLUB") && one_in(3)) || has_active_mutation("TAIL_CLUB")) &&
      one_in(10 - dex_cur)) {
        special_attack tmp;
        tmp.bash = 18;
        if (is_player()) {
            tmp.text = string_format(_("You club %s with your tail"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s clubs %2$s with his tail"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s clubs %2$s with her tail"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    ///\EFFECT_DEX increases chance of attacking with TAIL_THICK
    if (((has_trait("TAIL_THICK") && one_in(3)) || has_active_mutation("TAIL_THICK")) &&
      one_in(10 - dex_cur)) {
        special_attack tmp;
        tmp.bash = 8;
        if (is_player()) {
            tmp.text = string_format(_("You whap %s with your tail"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%1$s whaps %2$s with his tail"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%1$s whaps %2$s with her tail"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (has_trait("ARM_TENTACLES") || has_trait("ARM_TENTACLES_4") ||
        has_trait("ARM_TENTACLES_8")) {
        int num_attacks = 1;
        if (has_trait("ARM_TENTACLES_4")) {
            num_attacks = 3;
        }
        if (has_trait("ARM_TENTACLES_8")) {
            num_attacks = 7;
        }
        if( weapon.is_two_handed( *this ) ) {
            num_attacks--;
        }

        for (int i = 0; i < num_attacks; i++) {
            special_attack tmp;
            // Tentacle Rakes add additional cutting damage
            if (is_player()) {
                if (has_trait("CLAWS_TENTACLE")) {
                    tmp.text = string_format(_("You rake %s with your tentacle"),
                                             target.c_str());
                } else tmp.text = string_format(_("You slap %s with your tentacle"),
                                                    target.c_str());
            } else if (male) {
                if (has_trait("CLAWS_TENTACLE")) {
                    tmp.text = string_format(_("%1$s rakes %2$s with his tentacle"),
                                             name.c_str(), target.c_str());
                } else tmp.text = string_format(_("%1$s slaps %2$s with his tentacle"),
                                                    name.c_str(), target.c_str());
            } else {
                if (has_trait("CLAWS_TENTACLE")) {
                    tmp.text = string_format(_("%1$s rakes %2$s with her tentacle"),
                                             name.c_str(), target.c_str());
                } else tmp.text = string_format(_("%1$s slaps %2$s with her tentacle"),
                                                    name.c_str(), target.c_str());
            }
            ///\EFFECT_STR increases damage with ARM_TENTACLES*
            if (has_trait("CLAWS_TENTACLE")) {
                tmp.cut = str_cur / 2 + 1;
            } else {
                tmp.bash = str_cur / 3 + 1;
            }
            ret.push_back(tmp);
        }
    }

    if (has_trait("VINES2") || has_trait("VINES3")) {
        int num_attacks = 2;
        if (has_trait("VINES3")) {
            num_attacks = 3;
        }
        for (int i = 0; i < num_attacks; i++) {
            special_attack tmp;
            if (is_player()) {
                tmp.text = string_format(_("You lash %s with a vine"),
                                         target.c_str());
            } else if (male) {
                tmp.text = string_format(_("%1$s lashes %2$s with his vines"),
                                         name.c_str(), target.c_str());
            } else {
                tmp.text = string_format(_("%1$s lashes %2$s with her vines"),
                                         name.c_str(), target.c_str());
            }
            ///\EFFECT_STR increases damage with VINES*
            tmp.bash = str_cur / 2;
            ret.push_back(tmp);
        }
    }
    return ret;
}

std::string melee_message( const ma_technique &tec, player &p, const dealt_damage_instance &ddi )
{
    // Those could be extracted to a json

    // Three last values are for low damage
    static const std::array<std::string, 6> player_stab = {{
        _("You impale %s"), _("You gouge %s"), _("You run %s through"),
        _("You puncture %s"), _("You pierce %s"), _("You poke %s")
    }};
    static const std::array<std::string, 6> npc_stab = {{
        _("<npcname> impales %s"), _("<npcname> gouges %s"), _("<npcname> runs %s through"),
        _("<npcname> punctures %s"), _("<npcname> pierces %s"), _("<npcname> pokes %s")
    }};
    // First 5 are for high damage, next 2 for medium, then for low and then for v. low
    static const std::array<std::string, 9> player_cut = {{
        _("You gut %s"), _("You chop %s"), _("You slash %s"),
        _("You mutilate %s"), _("You maim %s"), _("You stab %s"),
        _("You slice %s"), _("You cut %s"), _("You nick %s")
    }};
    static const std::array<std::string, 9> npc_cut = {{
        _("<npcname> guts %s"), _("<npcname> chops %s"), _("<npcname> slashes %s"),
        _("<npcname> mutilates %s"), _("<npcname> maims %s"), _("<npcname> stabs %s"),
        _("<npcname> slices %s"), _("<npcname> cuts %s"), _("<npcname> nicks %s")
    }};

    // Three last values are for low damage
    static const std::array<std::string, 6> player_bash = {{
        _("You clobber %s"), _("You smash %s"), _("You thrash %s"),
        _("You batter %s"), _("You whack %s"), _("You hit %s")
    }};
    static const std::array<std::string, 6> npc_bash = {{
        _("<npcname> clobbers %s"), _("<npcname> smashes %s"), _("<npcname> thrashes %s"),
        _("<npcname> batters %s"), _("<npcname> whacks %s"), _("<npcname> hits %s")
    }};

    const int bash_dam = ddi.type_damage( DT_BASH );
    const int cut_dam  = ddi.type_damage( DT_CUT );
    const int stab_dam = ddi.type_damage( DT_STAB );

    if( tec.id != tec_none ) {
        std::string message;
        if (p.is_npc()) {
            message = tec.npc_message;
        } else {
            message = tec.player_message;
        }
        if( !message.empty() ) {
            return message;
        }
    }

    damage_type dominant_type = DT_BASH;
    if( cut_dam + stab_dam > bash_dam ) {
        dominant_type = cut_dam >= stab_dam ? DT_CUT : DT_STAB;
    }

    const bool npc = p.is_npc();

    // Cutting has more messages and so needs different handling
    const bool cutting = dominant_type == DT_CUT;
    size_t index;
    const int total_dam = bash_dam + stab_dam + cut_dam;
    if( total_dam > 30 ) {
        index = cutting ? rng( 0, 4 ) : rng( 0, 2 );
    } else if( total_dam > 20 ) {
        index = cutting ? rng( 5, 6 ) : 3;
    } else if( total_dam > 10 ) {
        index = cutting ? 7 : 4;
    } else {
        index = cutting ? 8 : 5;
    }

    if( dominant_type == DT_STAB ) {
        return (npc ? npc_stab[index] : player_stab[index]).c_str();
    } else if( dominant_type == DT_CUT ) {
        return (npc ? npc_cut[index] : player_cut[index]).c_str();
    } else if( dominant_type == DT_BASH ) {
        return (npc ? npc_bash[index] : player_bash[index]).c_str();
    }

    return _("The bugs attack %s");
}

// display the hit message for an attack
void player_hit_message(player* attacker, std::string message,
                        Creature &t, int dam, bool crit)
{
    std::string msg;
    game_message_type msgtype;
    msgtype = m_good;
    std::string sSCTmod = "";
    game_message_type gmtSCTcolor = m_good;

    if (dam <= 0) {
        if (attacker->is_npc()) {
            //~ NPC hits something but does no damage
            msg = string_format(_("%s but does no damage."), message.c_str());
        } else {
            //~ you hit something but do no damage
            msg = string_format(_("%s but do no damage."), message.c_str());
        }
        msgtype = m_neutral;
    } else if (crit) {
        //~ someone hits something for %d damage (critical)
        msg = string_format(_("%s for %d damage. Critical!"),
                            message.c_str(), dam);
        sSCTmod = _("Critical!");
        gmtSCTcolor = m_critical;
    } else {
        //~ someone hits something for %d damage
        msg = string_format(_("%s for %d damage."), message.c_str(), dam);
    }

    if (dam > 0 && attacker->is_player()) {
        //player hits monster melee
        SCT.add(t.posx(),
                t.posy(),
                direction_from(0, 0, t.posx() - attacker->posx(), t.posy() - attacker->posy()),
                get_hp_bar(dam, t.get_hp_max(), true).first, m_good,
                sSCTmod, gmtSCTcolor);

        if (t.get_hp() > 0) {
            SCT.add(t.posx(),
                    t.posy(),
                    direction_from(0, 0, t.posx() - attacker->posx(), t.posy() - attacker->posy()),
                    get_hp_bar(t.get_hp(), t.get_hp_max(), true).first, m_good,
                    //~ "hit points", used in scrolling combat text
                    _("hp"), m_neutral,
                    "hp");
        } else {
            SCT.removeCreatureHP();
        }
    }

    // same message is used for player and npc,
    // just using this for the <npcname> substitution.
    attacker->add_msg_player_or_npc(msgtype, msg.c_str(), msg.c_str(),
                                    t.disp_name().c_str());
}

void melee_practice( player &u, bool hit, bool unarmed,
                     bool bashing, bool cutting, bool stabbing )
{
    int min = 2;
    int max = 2;
    skill_id first = NULL_ID;
    skill_id second = NULL_ID;
    skill_id third = NULL_ID;

    if (hit) {
        min = 5;
        max = 10;
        u.practice( skill_melee, rng(5, 10) );
    } else {
        u.practice( skill_melee, rng(2, 5) );
    }

    // type of weapon used determines order of practice
    if (u.weapon.has_flag("SPEAR")) {
        if (stabbing) first  = skill_stabbing;
        if (bashing)  second = skill_bashing;
        if (cutting)  third  = skill_cutting;
    } else if (u.weapon.has_flag("STAB")) {
        // stabbity weapons have a 50-50 chance of raising either stabbing or cutting first
        if (one_in(2)) {
            if (stabbing) first  = skill_stabbing;
            if (cutting)  second = skill_cutting;
            if (bashing)  third  = skill_bashing;
        } else {
            if (cutting)  first  = skill_cutting;
            if (stabbing) second = skill_stabbing;
            if (bashing)  third  = skill_bashing;
        }
    } else if (u.weapon.is_cutting_weapon()) {
        if (cutting)  first  = skill_cutting;
        if (bashing)  second = skill_bashing;
        if (stabbing) third  = skill_stabbing;
    } else {
        if (bashing)  first  = skill_bashing;
        if (cutting)  second = skill_cutting;
        if (stabbing) third  = skill_stabbing;
    }

    if (unarmed) u.practice( skill_unarmed, rng(min, max) );
    if( first )  u.practice( first, rng(min, max) );
    if( second ) u.practice( second, rng(min, max) );
    if( third )  u.practice( third, rng(min, max) );
}

int player::attack_speed( const item &weap, const bool average ) const
{
    const int base_move_cost = weap.attack_time() / 2;
    const int melee_skill = has_active_bionic("bio_cqb") ? 5 : (int)get_skill_level( skill_melee );
    ///\EFFECT_MELEE increases melee attack speed
    const int skill_cost = (int)( base_move_cost / (std::pow(melee_skill, 3.0f)/400.0 + 1.0));
    ///\EFFECT_DEX increases attack speed
    const int dexbonus = average ? dex_cur / 2 : rng( 0, dex_cur );
    const int encumbrance_penalty = encumb( bp_torso ) +
                                    ( encumb( bp_hand_l ) + encumb( bp_hand_r ) ) / 2;
    const float stamina_ratio = (float)stamina / (float)get_stamina_max();
    // Increase cost multiplier linearly from 1.0 to 2.0 as stamina goes from 25% to 0%.
    const float stamina_penalty = 1.0 + ( (stamina_ratio < 0.25) ?
                                          ((0.25 - stamina_ratio) * 4.0) : 0.0 );

    int move_cost = base_move_cost;
    move_cost += skill_cost;
    move_cost += encumbrance_penalty;
    move_cost -= dexbonus;
    move_cost *= stamina_penalty;

    if( has_trait("HOLLOW_BONES") ) {
        move_cost *= .8;
    } else if( has_trait("LIGHT_BONES") ) {
        move_cost *= .9;
    }

    if( move_cost < 25 ) {
        return 25;
    }

    return move_cost;
}

double player::weapon_value( const item &weap, long ammo ) const
{
    const double val_gun = gun_value( weap, ammo );
    const double val_melee = melee_value( weap );
    const double more = std::max( val_gun, val_melee );
    const double less = std::min( val_gun, val_melee );

    // A small bonus for guns you can also use to hit stuff with (bayonets etc.)
    return more + (less / 2.0);
}

double player::gun_value( const item &weap, long ammo ) const
{
    // TODO: Mods
    // TODO: Allow using a specified type of ammo rather than default
    if( weap.type->gun.get() == nullptr ) {
        return 0.0;
    }

    if( ammo == 0 && !weap.has_flag("NO_AMMO") ) {
        return 0.0;
    }

    const islot_gun& gun = *weap.type->gun.get();
    const itype *def_ammo_i = item::find_type( default_ammo( weap.ammo_type() ) );
    if( def_ammo_i == nullptr || def_ammo_i->ammo == nullptr ) {
        //debugmsg( "player::gun_value couldn't find ammo for %s", weap.tname().c_str() );
        return 0.0;
    }

    const islot_ammo &def_ammo = *def_ammo_i->ammo;
    double damage_bonus = weap.gun_damage( false ) + def_ammo.damage;
    damage_bonus += ( weap.gun_pierce( false ) + def_ammo.pierce ) / 2.0;
    double gun_value = 0.0;
    gun_value += std::min<double>( gun.clip, ammo ) / 5.0;
    gun_value -= weap.gun_dispersion( false ) / 60.0 + def_ammo.dispersion / 100.0;
    gun_value -= weap.gun_recoil( false ) / 150.0 + def_ammo.recoil / 200.0;

    ///\EFFECT_GUN increases apparent value of guns presented to NPCs

    ///\EFFECT_PISTOL increases apparent value of pistols presented to NPCs

    ///\EFFECT_RIFLE increases apparent value of rifles presented to NPCs

    ///\EFFECT_SHOTGUN increases apparent value of shotguns presented to NPCs

    ///\EFFECT_SMG increases apparent value of smgs presented to NPCs
    const double skill_scaling =
        (.5 + (.3 * get_skill_level( skill_id( "gun" ) ))) *
        (.3 + (.7 * get_skill_level(gun.skill_used)));
    gun_value *= skill_scaling;

    // Don't penalize high-damage weapons for no skill
    // Point blank shotgun blasts don't require much skill
    gun_value += damage_bonus * std::max( 1.0, skill_scaling );

    // Bonus that only applies when we have ammo to spare
    double multi_ammo_bonus = gun.burst / 2.0;
    multi_ammo_bonus += gun.clip / 2.5;
    if( ammo <= 10 && !weap.has_flag("NO_AMMO") ) {
        gun_value = gun_value * ammo / 10;
    } else if( ammo < 30 ) {
        gun_value += multi_ammo_bonus * (30.0 / ammo);
    } else {
        gun_value += multi_ammo_bonus;
    }

    add_msg( m_debug, "%s as gun: %.1f total, %.1f for damage+pierce, %.1f skill scaling",
             weap.tname().c_str(), gun_value, damage_bonus, skill_scaling );
    return std::max( 0.0, gun_value );
}

double player::melee_value( const item &weap ) const
{
    double my_value = 0;

    damage_instance non_crit;
    roll_all_damage( false, non_crit, true, weap );
    float avg_dmg = non_crit.total_damage();

    const int accuracy = weap.type->m_to_hit + get_hit_weapon( weap );
    if( accuracy < 0 ) {
        // Heavy penalty
        my_value += accuracy * 5;
    } else if( accuracy <= 5 ) {
        // Big bonus
        my_value += accuracy * 3;
    } else {
        // Small bonus above that
        my_value += 15 + (accuracy - 5);
    }

    int move_cost = attack_speed( weap, true );
    static const matec_id rapid_strike( "RAPID" );
    if( weap.has_technique( rapid_strike ) ) {
        move_cost /= 2;
        avg_dmg *= 0.66;
    }

    const int arbitrary_dodge_target = 5;
    double crit_ch = crit_chance( accuracy, arbitrary_dodge_target, weap );
    my_value += crit_ch * 10; // Crits are worth more than just extra damage
    if( crit_ch > 0.1 ) {
        damage_instance crit;
        roll_all_damage( true, crit, true, weap );
        // Note: intentionally doesn't include rapid attack bonus in crits
        avg_dmg = (1.0 - crit_ch) * avg_dmg + crit.total_damage() * crit_ch;
    }

    my_value += avg_dmg * 100 / move_cost;

    return std::max( 0.0, my_value );
}

double player::unarmed_value() const
{
    // TODO: Martial arts
    return melee_value( ret_null );
}
