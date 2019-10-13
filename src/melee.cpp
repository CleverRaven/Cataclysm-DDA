#include "melee.h"

#include <climits>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <array>
#include <limits>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cmath>

#include "avatar.h"
#include "cata_utility.h"
#include "debug.h"
#include "game.h"
#include "game_inventory.h"
#include "itype.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "martialarts.h"
#include "messages.h"
#include "monattack.h"
#include "monster.h"
#include "mutation.h"
#include "npc.h"
#include "output.h"
#include "player.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "creature.h"
#include "damage.h"
#include "enums.h"
#include "game_constants.h"
#include "item.h"
#include "item_location.h"
#include "optional.h"
#include "pldata.h"
#include "string_id.h"
#include "units.h"
#include "weighted_list.h"
#include "material.h"
#include "type_id.h"
#include "point.h"
#include "projectile.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "mapdata.h"

static const bionic_id bio_cqb( "bio_cqb" );
static const bionic_id bio_memory( "bio_memory" );

static const matec_id tec_none( "tec_none" );
static const matec_id WBLOCK_1( "WBLOCK_1" );
static const matec_id WBLOCK_2( "WBLOCK_2" );
static const matec_id WBLOCK_3( "WBLOCK_3" );

static const skill_id skill_stabbing( "stabbing" );
static const skill_id skill_cutting( "cutting" );
static const skill_id skill_unarmed( "unarmed" );
static const skill_id skill_bashing( "bashing" );
static const skill_id skill_melee( "melee" );

const efftype_id effect_badpoison( "badpoison" );
const efftype_id effect_beartrap( "beartrap" );
const efftype_id effect_bouldering( "bouldering" );
const efftype_id effect_contacts( "contacts" );
const efftype_id effect_downed( "downed" );
const efftype_id effect_drunk( "drunk" );
const efftype_id effect_grabbed( "grabbed" );
const efftype_id effect_grabbing( "grabbing" );
const efftype_id effect_heavysnare( "heavysnare" );
const efftype_id effect_hit_by_player( "hit_by_player" );
const efftype_id effect_lightsnare( "lightsnare" );
const efftype_id effect_narcosis( "narcosis" );
const efftype_id effect_poison( "poison" );
const efftype_id effect_riding( "riding" );
const efftype_id effect_stunned( "stunned" );

static const trait_id trait_CLAWS( "CLAWS" );
static const trait_id trait_CLAWS_RAT( "CLAWS_RAT" );
static const trait_id trait_CLAWS_RETRACT( "CLAWS_RETRACT" );
static const trait_id trait_CLAWS_ST( "CLAWS_ST" );
static const trait_id trait_CLAWS_TENTACLE( "CLAWS_TENTACLE" );
static const trait_id trait_CLUMSY( "CLUMSY" );
static const trait_id trait_DEBUG_NIGHTVISION( "DEBUG_NIGHTVISION" );
static const trait_id trait_DEFT( "DEFT" );
static const trait_id trait_DRUNKEN( "DRUNKEN" );
static const trait_id trait_HYPEROPIC( "HYPEROPIC" );
static const trait_id trait_NAILS( "NAILS" );
static const trait_id trait_POISONOUS2( "POISONOUS2" );
static const trait_id trait_POISONOUS( "POISONOUS" );
static const trait_id trait_PROF_SKATER( "PROF_SKATER" );
static const trait_id trait_SLIME_HANDS( "SLIME_HANDS" );
static const trait_id trait_TALONS( "TALONS" );
static const trait_id trait_THORNS( "THORNS" );

static const efftype_id effect_amigara( "amigara" );

const species_id HUMAN( "HUMAN" );

void player_hit_message( player *attacker, const std::string &message,
                         Creature &t, int dam, bool crit = false );
int  stumble( player &u, const item &weap );
std::string melee_message( const ma_technique &tec, player &p, const dealt_damage_instance &ddi );

/* Melee Functions!
 * These all belong to class player.
 *
 * STATE QUERIES
 * bool is_armed() - True if we are armed with any item.
 * bool unarmed_attack() - True if we are attacking with a "fist" weapon
 * (cestus, bionic claws etc.) or no weapon.
 *
 * HIT DETERMINATION
 * int hit_roll() - The player's hit roll, to be compared to a monster's or
 *   player's dodge_roll().  This handles weapon bonuses, weapon-specific
 *   skills, torso encumbrance penalties and drunken master bonuses.
 */

const item &Character::used_weapon() const
{
    return dynamic_cast<const player &>( *this ).get_combat_style().force_unarmed ?
           null_item_reference() : weapon;
}

item &Character::used_weapon()
{
    return const_cast<item &>( const_cast<const Character *>( this )->used_weapon() );
}

bool Character::is_armed() const
{
    return !weapon.is_null();
}

bool player::unarmed_attack() const
{
    const item &weap = used_weapon();
    return weap.is_null() || weap.has_flag( "UNARMED_WEAPON" );
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
    if( shield.has_flag( "UNBREAKABLE_MELEE" ) ) {
        return false;
    }

    /** @EFFECT_DEX reduces chance of damaging your melee weapon */

    /** @EFFECT_STR increases chance of damaging your melee weapon (NEGATIVE) */

    /** @EFFECT_MELEE reduces chance of damaging your melee weapon */
    const float stat_factor = dex_cur / 2.0f
                              + get_skill_level( skill_melee )
                              + ( 64.0f / std::max( str_cur, 4 ) );

    float material_factor;

    itype_id weak_comp;
    itype_id big_comp = "null";
    // Fragile items that fall apart easily when used as a weapon due to poor construction quality
    if( shield.has_flag( "FRAGILE_MELEE" ) ) {
        const float fragile_factor = 6;
        int weak_chip = INT_MAX;
        units::volume big_vol = 0_ml;

        // Items that should have no bearing on durability
        const std::set<itype_id> blacklist = { "rag",
                                               "leather",
                                               "fur"
                                             };

        for( auto &comp : shield.components ) {
            if( blacklist.count( comp.typeId() ) <= 0 ) {
                if( weak_chip > comp.chip_resistance() ) {
                    weak_chip = comp.chip_resistance();
                    weak_comp = comp.typeId();
                }
            }
            if( comp.volume() > big_vol ) {
                big_vol = comp.volume();
                big_comp = comp.typeId();
            }
        }
        material_factor = ( weak_chip < INT_MAX ? weak_chip : shield.chip_resistance() ) / fragile_factor;
    } else {
        material_factor = shield.chip_resistance();
    }
    int damage_chance = static_cast<int>( stat_factor * material_factor / wear_multiplier );
    // DURABLE_MELEE items are made to hit stuff and they do it well, so they're considered to be a lot tougher
    // than other weapons made of the same materials.
    if( shield.has_flag( "DURABLE_MELEE" ) ) {
        damage_chance *= 4;
    }

    if( damage_chance > 0 && !one_in( damage_chance ) ) {
        return false;
    }

    auto str = shield.tname(); // save name before we apply damage

    if( !shield.inc_damage() ) {
        add_msg_player_or_npc( m_bad, _( "Your %s is damaged by the force of the blow!" ),
                               _( "<npcname>'s %s is damaged by the force of the blow!" ),
                               str );
        return false;
    }

    // Dump its contents on the ground
    // Destroy irremovable mods, if any

    for( auto mod : shield.gunmods() ) {
        if( mod->is_irremovable() ) {
            remove_item( *mod );
        }
    }

    for( auto &elem : shield.contents ) {
        g->m.add_item_or_charges( pos(), elem );
    }

    // Preserve item temporarily for component breakdown
    item temp = shield;

    remove_item( shield );

    // Breakdown fragile weapons into components
    if( temp.has_flag( "FRAGILE_MELEE" ) && !temp.components.empty() ) {
        add_msg_player_or_npc( m_bad, _( "Your %s breaks apart!" ),
                               _( "<npcname>'s %s breaks apart!" ),
                               str );

        for( auto &comp : temp.components ) {
            int break_chance = comp.typeId() == weak_comp ? 2 : 8;

            if( one_in( break_chance ) ) {
                add_msg_if_player( m_bad, _( "The %s is destroyed!" ), comp.tname() );
                continue;
            }

            if( comp.typeId() == big_comp && !is_armed() ) {
                wield( comp );
            } else {
                g->m.add_item_or_charges( pos(), comp );
            }
        }
    } else {
        add_msg_player_or_npc( m_bad, _( "Your %s is destroyed by the blow!" ),
                               _( "<npcname>'s %s is destroyed by the blow!" ),
                               str );
    }

    return true;
}

float player::get_hit_weapon( const item &weap ) const
{
    /** @EFFECT_UNARMED improves hit chance for unarmed weapons */
    /** @EFFECT_BASHING improves hit chance for bashing weapons */
    /** @EFFECT_CUTTING improves hit chance for cutting weapons */
    /** @EFFECT_STABBING improves hit chance for piercing weapons */
    auto skill = get_skill_level( weap.melee_skill() );

    // CQB bionic acts as a lower bound providing item uses a weapon skill
    if( skill < BIO_CQB_LEVEL && has_active_bionic( bio_cqb ) ) {
        skill = BIO_CQB_LEVEL;
    }

    /** @EFFECT_MELEE improves hit chance for all items (including non-weapons) */
    return ( skill / 3.0f ) + ( get_skill_level( skill_melee ) / 2.0f );
}

float player::get_hit_base() const
{
    // Character::get_hit_base includes stat calculations already
    return Character::get_hit_base() + get_hit_weapon( used_weapon() );
}

float player::hit_roll() const
{
    // Dexterity, skills, weapon and martial arts
    float hit = get_hit();
    // Drunken master makes us hit better
    if( has_trait( trait_DRUNKEN ) ) {
        hit += to_turns<float>( get_effect_dur( effect_drunk ) ) / ( used_weapon().is_null() ? 300.0f :
                400.0f );
    }

    // Farsightedness makes us hit worse
    if( has_trait( trait_HYPEROPIC ) && !worn_with_flag( "FIX_FARSIGHT" ) &&
        !has_effect( effect_contacts ) ) {
        hit -= 2.0f;
    }

    //Unstable ground chance of failure
    if( has_effect( effect_bouldering ) ) {
        hit *= 0.75f;
    }

    hit *= std::max( 0.25f, 1.0f - encumb( bp_torso ) / 100.0f );

    return melee::melee_hit_range( hit );
}

void player::add_miss_reason( const std::string &reason, const unsigned int weight )
{
    melee_miss_reasons.add( reason, weight );

}

void player::clear_miss_reasons()
{
    melee_miss_reasons.clear();
}

std::string player::get_miss_reason()
{
    // everything that lowers accuracy in player::hit_roll()
    // adding it in hit_roll() might not be safe if it's called multiple times
    // in one turn
    add_miss_reason(
        _( "Your torso encumbrance throws you off-balance." ),
        roll_remainder( encumb( bp_torso ) / 10.0 ) );
    const int farsightedness = 2 * ( has_trait( trait_HYPEROPIC ) &&
                                     !worn_with_flag( "FIX_FARSIGHT" ) &&
                                     !has_effect( effect_contacts ) );
    add_miss_reason(
        _( "You can't hit reliably due to your farsightedness." ),
        farsightedness );

    const std::string *const reason = melee_miss_reasons.pick();
    if( reason == nullptr ) {
        return std::string();
    }
    return *reason;
}

void player::roll_all_damage( bool crit, damage_instance &di, bool average, const item &weap ) const
{
    roll_bash_damage( crit, di, average, weap );
    roll_cut_damage( crit, di, average, weap );
    roll_stab_damage( crit, di, average, weap );
}

static void melee_train( player &p, int lo, int hi, const item &weap )
{
    p.practice( skill_melee, ceil( rng( lo, hi ) / 2.0 ), hi );

    // allocate XP proportional to damage stats
    // Pure unarmed needs a special case because it has 0 weapon damage
    int cut  = weap.damage_melee( DT_CUT );
    int stab = weap.damage_melee( DT_STAB );
    int bash = weap.damage_melee( DT_BASH ) + ( weap.is_null() ? 1 : 0 );

    float total = std::max( cut + stab + bash, 1 );
    p.practice( skill_cutting,  ceil( cut  / total * rng( lo, hi ) ), hi );
    p.practice( skill_stabbing, ceil( stab / total * rng( lo, hi ) ), hi );

    // Unarmed skill scaled bashing damage and so scales with bashing damage
    p.practice( weap.is_unarmed_weapon() ? skill_unarmed : skill_bashing,
                ceil( bash / total * rng( lo, hi ) ), hi );
}

void player::melee_attack( Creature &t, bool allow_special )
{
    static const matec_id no_technique_id( "" );
    melee_attack( t, allow_special, no_technique_id );
}

// Melee calculation is in parts. This sets up the attack, then in deal_melee_attack,
// we calculate if we would hit. In Creature::deal_melee_hit, we calculate if the target dodges.
void player::melee_attack( Creature &t, bool allow_special, const matec_id &force_technique,
                           bool allow_unarmed )
{
    int hit_spread = t.deal_melee_attack( this, hit_roll() );
    if( !t.is_player() ) {
        // TODO: Per-NPC tracking? Right now monster hit by either npc or player will draw aggro...
        t.add_effect( effect_hit_by_player, 10_minutes ); // Flag as attacked by us for AI
    }
    if( is_mounted() ) {
        auto mons = mounted_creature.get();
        if( mons->has_flag( MF_RIDEABLE_MECH ) ) {
            if( !mons->check_mech_powered() ) {
                add_msg( m_bad, _( "The %s has dead batteries and will not move its arms." ), mons->get_name() );
                return;
            }
            if( mons->type->has_special_attack( "SMASH" ) && one_in( 3 ) ) {
                add_msg( m_info, _( "The %s hisses as its hydraulic arm pumps forward!" ), mons->get_name() );
                mattack::smash_specific( mons, &t );
            } else {
                mons->use_mech_power( -2 );
                mons->melee_attack( t );
            }
            mod_moves( -mons->type->attack_cost );
            return;
        }
    }
    item &cur_weapon = allow_unarmed ? used_weapon() : weapon;
    const bool critical_hit = scored_crit( t.dodge_roll(), cur_weapon );
    int move_cost = attack_speed( cur_weapon );

    if( hit_spread < 0 ) {
        int stumble_pen = stumble( *this, cur_weapon );
        sfx::generate_melee_sound( pos(), t.pos(), false, false );
        if( is_player() ) { // Only display messages if this is the player

            if( one_in( 2 ) ) {
                const std::string reason_for_miss = get_miss_reason();
                if( !reason_for_miss.empty() ) {
                    add_msg( reason_for_miss );
                }
            }

            if( can_miss_recovery( cur_weapon ) ) {
                ma_technique tec = get_miss_recovery_tec( cur_weapon );
                add_msg( _( tec.player_message ), t.disp_name() );
            } else if( stumble_pen >= 60 ) {
                add_msg( m_bad, _( "You miss and stumble with the momentum." ) );
            } else if( stumble_pen >= 10 ) {
                add_msg( _( "You swing wildly and miss." ) );
            } else {
                add_msg( _( "You miss." ) );
            }
        } else if( g->u.sees( *this ) ) {
            if( stumble_pen >= 60 ) {
                add_msg( _( "%s misses and stumbles with the momentum." ), name );
            } else if( stumble_pen >= 10 ) {
                add_msg( _( "%s swings wildly and misses." ), name );
            } else {
                add_msg( _( "%s misses." ), name );
            }
        }

        // Practice melee and relevant weapon skill (if any) except when using CQB bionic
        if( !has_active_bionic( bio_cqb ) ) {
            melee_train( *this, 2, 5, cur_weapon );
        }

        // Cap stumble penalty, heavy weapons are quite weak already
        move_cost += std::min( 60, stumble_pen );
        if( has_miss_recovery_tec( cur_weapon ) ) {
            move_cost /= 2;
        }

        ma_onmiss_effects(); // trigger martial arts on-miss effects
    } else {
        // Remember if we see the monster at start - it may change
        const bool seen = g->u.sees( t );
        // Start of attacks.
        damage_instance d;
        roll_all_damage( critical_hit, d, false, cur_weapon );

        const bool has_force_technique = !force_technique.str().empty();

        // Pick one or more special attacks
        matec_id technique_id;
        if( allow_special && !has_force_technique ) {
            technique_id = pick_technique( t, cur_weapon, critical_hit, false, false );
        } else if( has_force_technique ) {
            technique_id = force_technique;
        } else {
            technique_id = tec_none;
        }

        const ma_technique &technique = technique_id.obj();

        // Handles effects as well; not done in melee_affect_*
        if( technique.id != tec_none ) {
            perform_technique( technique, t, d, move_cost );
        }

        if( allow_special && !t.is_dead_state() ) {
            perform_special_attacks( t );
        }

        // Proceed with melee attack.
        if( !t.is_dead_state() ) {
            // Handles speed penalties to monster & us, etc
            std::string specialmsg = melee_special_effects( t, d, cur_weapon );

            dealt_damage_instance dealt_dam; // gets overwritten with the dealt damage values
            t.deal_melee_hit( this, hit_spread, critical_hit, d, dealt_dam );

            // Make a rather quiet sound, to alert any nearby monsters
            if( !is_quiet() ) { // check martial arts silence
                sounds::sound( pos(), 8, sounds::sound_t::combat, "whack!" ); //sound generated later
            }
            std::string material = "flesh";
            if( t.is_monster() ) {
                const monster *m = dynamic_cast<const monster *>( &t );
                if( m->made_of( material_id( "steel" ) ) ) {
                    material = "steel";
                }
            }
            sfx::generate_melee_sound( pos(), t.pos(), true, t.is_monster(), material );
            int dam = dealt_dam.total_damage();

            // Practice melee and relevant weapon skill (if any) except when using CQB bionic
            if( !has_active_bionic( bio_cqb ) ) {
                melee_train( *this, 5, 10, cur_weapon );
            }

            if( dam >= 5 && has_artifact_with( AEP_SAP_LIFE ) ) {
                healall( rng( dam / 10, dam / 5 ) );
            }

            // Treat monster as seen if we see it before or after the attack
            if( seen || g->u.sees( t ) ) {
                std::string message = melee_message( technique, *this, dealt_dam );
                player_hit_message( this, message, t, dam, critical_hit );
            } else {
                add_msg_player_or_npc( m_good, _( "You hit something." ), _( "<npcname> hits something." ) );
            }

            if( !specialmsg.empty() ) {
                add_msg_if_player( m_neutral, specialmsg );
            }

            if( critical_hit ) {
                ma_oncrit_effects(); // trigger martial arts on-crit effects
            }

        }

        t.check_dead_state();

        if( t.is_dead_state() ) {
            ma_onkill_effects(); // trigger martial arts on-kill effects
        }
    }

    const int melee = get_skill_level( skill_melee );
    /** @EFFECT_STR reduces stamina cost for melee attack with heavier weapons */
    const int weight_cost = cur_weapon.weight() / ( 2_gram * std::max( 1, str_cur ) );
    const int encumbrance_cost = roll_remainder( ( encumb( bp_arm_l ) + encumb( bp_arm_r ) ) * 2.0f );
    const int deft_bonus = hit_spread < 0 && has_trait( trait_DEFT ) ? 50 : 0;
    /** @EFFECT_MELEE reduces stamina cost of melee attacks */
    const int mod_sta = ( weight_cost + encumbrance_cost - melee - deft_bonus + 50 ) * -1;
    mod_stat( "stamina", std::min( -50, mod_sta ) );
    add_msg( m_debug, "Stamina burn: %d", std::min( -50, mod_sta ) );
    mod_moves( -move_cost );

    ma_onattack_effects(); // trigger martial arts on-attack effects
    // some things (shattering weapons) can harm the attacking creature.
    check_dead_state();
    did_hit( t );
    if( t.as_character() ) {
        dealt_projectile_attack dp = dealt_projectile_attack();
        t.as_character()->on_hit( this, body_part::num_bp, 0.0f, &dp );
    }
    return;
}

void player::reach_attack( const tripoint &p )
{
    matec_id force_technique = tec_none;
    /** @EFFECT_MELEE >5 allows WHIP_DISARM technique */
    if( weapon.has_flag( "WHIP" ) && ( get_skill_level( skill_melee ) > 5 ) && one_in( 3 ) ) {
        force_technique = matec_id( "WHIP_DISARM" );
    }

    Creature *critter = g->critter_at( p );
    // Original target size, used when there are monsters in front of our target
    int target_size = critter != nullptr ? critter->get_size() : 2;
    // Reset last target pos
    last_target_pos = cata::nullopt;

    int move_cost = attack_speed( weapon );
    int skill = std::min( 10, get_skill_level( skill_stabbing ) );
    int t = 0;
    std::vector<tripoint> path = line_to( pos(), p, t, 0 );
    path.pop_back(); // Last point is our critter
    for( const tripoint &p : path ) {
        // Possibly hit some unintended target instead
        Creature *inter = g->critter_at( p );
        /** @EFFECT_STABBING decreases chance of hitting intervening target on reach attack */
        if( inter != nullptr &&
            !x_in_y( ( target_size * target_size + 1 ) * skill,
                     ( inter->get_size() * inter->get_size() + 1 ) * 10 ) ) {
            // Even if we miss here, low roll means weapon is pushed away or something like that
            critter = inter;
            break;
            /** @EFFECT_STABBING increases ability to reach attack through fences */
        } else if( g->m.impassable( p ) &&
                   // Fences etc. Spears can stab through those
                   !( weapon.has_flag( "SPEAR" ) &&
                      g->m.has_flag( "THIN_OBSTACLE", p ) &&
                      x_in_y( skill, 10 ) ) ) {
            /** @EFFECT_STR increases bash effects when reach attacking past something */
            g->m.bash( p, str_cur + weapon.damage_melee( DT_BASH ) );
            handle_melee_wear( weapon );
            mod_moves( -move_cost );
            return;
        }
    }

    if( critter == nullptr ) {
        add_msg_if_player( _( "You swing at the air." ) );
        if( has_miss_recovery_tec( weapon ) ) {
            move_cost /= 3; // "Probing" is faster than a regular miss
        }

        mod_moves( -move_cost );
        return;
    }

    reach_attacking = true;
    melee_attack( *critter, false, force_technique, false );
    reach_attacking = false;
}

int stumble( player &u, const item &weap )
{
    if( u.has_trait( trait_DEFT ) ) {
        return 0;
    }

    // Examples:
    // 10 str with a hatchet: 4 + 8 = 12
    // 5 str with a battle axe: 26 + 49 = 75
    // Fist: 0

    /** @EFFECT_STR reduces chance of stumbling with heavier weapons */
    return ( weap.volume() / 125_ml ) +
           ( weap.weight() / ( u.str_cur * 10_gram + 13.0_gram ) );
}

bool player::scored_crit( float target_dodge, const item &weap ) const
{
    return rng_float( 0, 1.0 ) < crit_chance( hit_roll(), target_dodge, weap );
}

/**
 * Limits a probability to be between 0.0 and 1.0
 */
inline double limit_probability( double unbounded_probability )
{
    return std::max( std::min( unbounded_probability, 1.0 ), 0.0 );
}

double player::crit_chance( float roll_hit, float target_dodge, const item &weap ) const
{
    // Weapon to-hit roll
    double weapon_crit_chance = 0.5;
    if( weap.is_unarmed_weapon() ) {
        // Unarmed attack: 1/2 of unarmed skill is to-hit
        /** @EFFECT_UNARMED increases critical chance with UNARMED_WEAPON */
        weapon_crit_chance = 0.5 + 0.05 * get_skill_level( skill_unarmed );
    }

    if( weap.type->m_to_hit > 0 ) {
        weapon_crit_chance = std::max( weapon_crit_chance, 0.5 + 0.1 * weap.type->m_to_hit );
    } else if( weap.type->m_to_hit < 0 ) {
        weapon_crit_chance += 0.1 * weap.type->m_to_hit;
    }
    weapon_crit_chance = limit_probability( weapon_crit_chance );

    // Dexterity and perception
    /** @EFFECT_DEX increases chance for critical hits */

    /** @EFFECT_PER increases chance for critical hits */
    const double stat_crit_chance = limit_probability( 0.25 + 0.01 * dex_cur + ( 0.02 * per_cur ) );

    /** @EFFECT_BASHING increases critical chance with bashing weapons */
    /** @EFFECT_CUTTING increases critical chance with cutting weapons */
    /** @EFFECT_STABBING increases critical chance with piercing weapons */
    /** @EFFECT_UNARMED increases critical chance with unarmed weapons */
    int sk = get_skill_level( weap.melee_skill() );
    if( has_active_bionic( bio_cqb ) ) {
        sk = std::max( sk, BIO_CQB_LEVEL );
    }

    /** @EFFECT_MELEE slightly increases critical chance with any item */
    sk += get_skill_level( skill_melee ) / 2.5;

    const double skill_crit_chance = limit_probability( 0.25 + sk * 0.025 );

    // Examples (survivor stats/chances of each critical):
    // Fresh (skill-less) 8/8/8/8, unarmed:
    //  50%, 49%, 25%; ~1/16 guaranteed critical + ~1/8 if roll>dodge*1.5
    // Expert (skills 10) 10/10/10/10, unarmed:
    //  100%, 55%, 60%; ~1/3 guaranteed critical + ~4/10 if roll>dodge*1.5
    // Godlike with combat CBM 20/20/20/20, pipe (+1 accuracy):
    //  60%, 100%, 42%; ~1/4 guaranteed critical + ~3/8 if roll>dodge*1.5

    // Note: the formulas below are only valid if none of the 3 critical chance values go above 1.0
    // It is therefore important to limit them to between 0.0 and 1.0

    // Chance to get all 3 criticals (a guaranteed critical regardless of hit/dodge)
    const double chance_triple = weapon_crit_chance * stat_crit_chance * skill_crit_chance;
    // Only check double critical (one that requires hit/dodge comparison) if we have good hit vs dodge
    if( roll_hit > target_dodge * 3 / 2 ) {
        const double chance_double = 0.5 * (
                                         weapon_crit_chance * stat_crit_chance +
                                         stat_crit_chance * skill_crit_chance +
                                         weapon_crit_chance * skill_crit_chance -
                                         ( 3 * chance_triple ) );
        // Because chance_double already removed the triples with -( 3 * chance_triple ), chance_triple
        // and chance_double are mutually exclusive probabilities and can just be added together.
        return chance_triple + chance_double;
    }

    return chance_triple;
}

float player::get_dodge_base() const
{
    // TODO: Remove this override?
    return Character::get_dodge_base();
}

float player::get_dodge() const
{
    //If we're asleep or busy we can't dodge
    if( in_sleep_state() || has_effect( effect_narcosis ) ||
        has_effect( efftype_id( "winded" ) ) ) {
        return 0.0f;
    }

    float ret = Creature::get_dodge();
    // Chop in half if we are unable to move
    if( has_effect( effect_beartrap ) || has_effect( effect_lightsnare ) ||
        has_effect( effect_heavysnare ) ) {
        ret /= 2;
    }

    if( has_effect( effect_grabbed ) ) {
        int zed_number = 0;
        for( auto &dest : g->m.points_in_radius( pos(), 1, 0 ) ) {
            const monster *const mon = g->critter_at<monster>( dest );
            if( mon && mon->has_effect( effect_grabbing ) ) {
                zed_number++;
            }
        }
        if( zed_number > 0 ) {
            ret /= zed_number + 1;
        }
    }

    if( worn_with_flag( "ROLLER_INLINE" ) ||
        worn_with_flag( "ROLLER_QUAD" ) ||
        worn_with_flag( "ROLLER_ONE" ) ) {
        ret /= has_trait( trait_PROF_SKATER ) ? 2 : 5;
    }

    if( has_effect( effect_bouldering ) ) {
        ret /= 4;
    }

    // Each dodge after the first subtracts equivalent of 2 points of dodge skill
    if( dodges_left <= 0 ) {
        ret += dodges_left * 2 - 2;
    }

    // Speed below 100 linearly decreases dodge effectiveness
    int speed_stat = get_speed();
    if( speed_stat < 100 ) {
        ret *= speed_stat / 100.0f;
    }

    return std::max( 0.0f, ret );
}

float player::dodge_roll()
{
    return get_dodge() * 5;
}

float player::bonus_damage( bool random ) const
{
    /** @EFFECT_STR increases bashing damage */
    if( random ) {
        return rng_float( get_str() / 2.0f, get_str() );
    }

    return get_str() * 0.75f;
}

void player::roll_bash_damage( bool crit, damage_instance &di, bool average,
                               const item &weap ) const
{
    float bash_dam = 0.0f;

    const bool unarmed = weap.is_unarmed_weapon();
    int skill = get_skill_level( unarmed ? skill_unarmed : skill_bashing );
    if( has_active_bionic( bio_cqb ) ) {
        skill = BIO_CQB_LEVEL;
    }

    const int stat = get_str();
    /** @EFFECT_STR increases bashing damage */
    float stat_bonus = bonus_damage( !average );
    stat_bonus += mabuff_damage_bonus( DT_BASH );

    // Drunken Master damage bonuses
    if( has_trait( trait_DRUNKEN ) && has_effect( effect_drunk ) ) {
        // Remember, a single drink gives 600 levels of "drunk"
        int mindrunk = 0;
        int maxdrunk = 0;
        const time_duration drunk_dur = get_effect_dur( effect_drunk );
        if( unarmed ) {
            mindrunk = drunk_dur / 600_turns;
            maxdrunk = drunk_dur / 250_turns;
        } else {
            mindrunk = drunk_dur / 900_turns;
            maxdrunk = drunk_dur / 400_turns;
        }

        bash_dam += average ? ( mindrunk + maxdrunk ) * 0.5f : rng( mindrunk, maxdrunk );
    }

    /** @EFFECT_STR increases bashing damage */
    float weap_dam = weap.damage_melee( DT_BASH ) + stat_bonus;
    /** @EFFECT_UNARMED caps bash damage with unarmed weapons */

    /** @EFFECT_BASHING caps bash damage with bashing weapons */
    float bash_cap = 2 * stat + 2 * skill;
    float bash_mul = 1.0f;

    // 80%, 88%, 96%, 104%, 112%, 116%, 120%, 124%, 128%, 132%
    if( skill < 5 ) {
        bash_mul = 0.8 + 0.08 * skill;
    } else {
        bash_mul = 0.96 + 0.04 * skill;
    }

    if( bash_cap < weap_dam && !weap.is_null() ) {
        // If damage goes over cap due to low stats/skills,
        // scale the post-armor damage down halfway between damage and cap
        bash_mul *= ( 1.0f + ( bash_cap / weap_dam ) ) / 2.0f;
    }

    /** @EFFECT_STR boosts low cap on bashing damage */
    const float low_cap = std::min( 1.0f, stat / 20.0f );
    const float bash_min = low_cap * weap_dam;
    weap_dam = average ? ( bash_min + weap_dam ) * 0.5f : rng_float( bash_min, weap_dam );

    bash_dam += weap_dam;
    bash_mul *= mabuff_damage_mult( DT_BASH );

    float armor_mult = 1.0f;
    // Finally, extra critical effects
    if( crit ) {
        bash_mul *= 1.5f;
        // 50% armor penetration
        armor_mult = 0.5f;
    }

    di.add_damage( DT_BASH, bash_dam, 0, armor_mult, bash_mul );
}

void player::roll_cut_damage( bool crit, damage_instance &di, bool average, const item &weap ) const
{
    float cut_dam = mabuff_damage_bonus( DT_CUT ) + weap.damage_melee( DT_CUT );
    float cut_mul = 1.0f;

    int cutting_skill = get_skill_level( skill_cutting );
    int unarmed_skill = get_skill_level( skill_unarmed );

    if( has_active_bionic( bio_cqb ) ) {
        cutting_skill = BIO_CQB_LEVEL;
    }

    if( weap.is_unarmed_weapon() ) {
        // TODO: 1-handed weapons that aren't unarmed attacks
        const bool left_empty = !natural_attack_restricted_on( bp_hand_l );
        const bool right_empty = !natural_attack_restricted_on( bp_hand_r ) &&
                                 weap.is_null();
        if( left_empty || right_empty ) {
            float per_hand = 0.0f;
            if( has_trait( trait_CLAWS ) || ( has_active_mutation( trait_CLAWS_RETRACT ) ) ) {
                per_hand += 3;
            }
            if( has_bionic( bionic_id( "bio_razors" ) ) ) {
                per_hand += 2;
            }
            if( has_trait( trait_TALONS ) ) {
                /** @EFFECT_UNARMED increases cutting damage with TALONS */
                per_hand += 3 + ( unarmed_skill > 8 ? 4 : unarmed_skill / 2 );
            }
            // Stainless Steel Claws do stabbing damage, too.
            if( has_trait( trait_CLAWS_RAT ) || has_trait( trait_CLAWS_ST ) ) {
                /** @EFFECT_UNARMED increases cutting damage with CLAWS_RAT and CLAWS_ST */
                per_hand += 1 + ( unarmed_skill > 8 ? 4 : unarmed_skill / 2 );
            }
            // TODO: add acidproof check back to slime hands (probably move it elsewhere)
            if( has_trait( trait_SLIME_HANDS ) ) {
                /** @EFFECT_UNARMED increases cutting damage with SLIME_HANDS */
                per_hand += average ? 2.5f : rng( 2, 3 );
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
    /** @EFFECT_CUTTING increases cutting damage multiplier */
    if( cutting_skill < 5 ) {
        cut_mul *= 0.8 + 0.08 * cutting_skill;
    } else {
        cut_mul *= 0.96 + 0.04 * cutting_skill;
    }

    cut_mul *= mabuff_damage_mult( DT_CUT );
    if( crit ) {
        cut_mul *= 1.25f;
        arpen += 5;
        armor_mult = 0.75f; //25% armor penetration
    }

    di.add_damage( DT_CUT, cut_dam, arpen, armor_mult, cut_mul );
}

void player::roll_stab_damage( bool crit, damage_instance &di, bool average,
                               const item &weap ) const
{
    ( void )average; // No random rolls in stab damage
    float cut_dam = mabuff_damage_bonus( DT_STAB ) + weap.damage_melee( DT_STAB );

    int unarmed_skill = get_skill_level( skill_unarmed );
    int stabbing_skill = get_skill_level( skill_stabbing );

    if( has_active_bionic( bio_cqb ) ) {
        stabbing_skill = BIO_CQB_LEVEL;
    }

    if( weap.is_unarmed_weapon() ) {
        const bool left_empty = !natural_attack_restricted_on( bp_hand_l );
        const bool right_empty = !natural_attack_restricted_on( bp_hand_r ) &&
                                 weap.is_null();
        if( left_empty || right_empty ) {
            float per_hand = 0.0f;
            if( has_trait( trait_CLAWS ) || has_active_mutation( trait_CLAWS_RETRACT ) ) {
                per_hand += 3;
            }

            if( has_trait( trait_NAILS ) ) {
                per_hand += .5;
            }

            if( has_bionic( bionic_id( "bio_razors" ) ) ) {
                per_hand += 2;
            }

            if( has_trait( trait_THORNS ) ) {
                per_hand += 2;
            }

            if( has_trait( trait_CLAWS_ST ) ) {
                /** @EFFECT_UNARMED increases stabbing damage with CLAWS_ST */
                per_hand += 3 + unarmed_skill / 2.0;
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
    /** @EFFECT_STABBING increases stabbing damage multiplier */
    if( stabbing_skill <= 5 ) {
        stab_mul = 0.66 + 0.1 * stabbing_skill;
    } else {
        stab_mul = 0.86 + 0.06 * stabbing_skill;
    }

    stab_mul *= mabuff_damage_mult( DT_STAB );
    float armor_mult = 1.0f;

    if( crit ) {
        // Critical damage bonus for stabbing scales with skill
        stab_mul *= 1.0 + ( stabbing_skill / 10.0 );
        // Stab criticals have extra %arpen
        armor_mult = 0.66f;
    }

    di.add_damage( DT_STAB, cut_dam, 0, armor_mult, stab_mul );
}

matec_id player::pick_technique( Creature &t, const item &weap,
                                 bool crit, bool dodge_counter, bool block_counter )
{

    std::vector<matec_id> all = get_all_techniques( weap );

    std::vector<matec_id> possible;

    bool downed = t.has_effect( effect_downed );
    bool stunned = t.has_effect( effect_stunned );
    bool wall_adjacent = g->m.is_wall_adjacent( pos() );

    // first add non-aoe tecs
    for( auto &tec_id : all ) {
        const ma_technique &tec = tec_id.obj();

        // ignore "dummy" techniques like WBLOCK_1
        if( tec.dummy ) {
            continue;
        }

        // skip defensive techniques
        if( tec.defensive ) {
            continue;
        }

        // skip wall adjacent techniques if not next to a wall
        if( tec.wall_adjacent && !wall_adjacent ) {
            continue;
        }

        // skip dodge counter techniques
        if( dodge_counter != tec.dodge_counter ) {
            continue;
        }

        // skip block counter techniques
        if( block_counter != tec.block_counter ) {
            continue;
        }

        // if critical then select only from critical tecs
        // but allow the technique if its crit ok
        if( !tec.crit_ok && ( crit != tec.crit_tec ) ) {
            continue;
        }

        // don't apply downing techniques to someone who's already downed
        if( downed && tec.down_dur > 0 ) {
            continue;
        }

        // don't apply "downed only" techniques to someone who's not downed
        if( !downed && tec.downed_target ) {
            continue;
        }

        // don't apply "stunned only" techniques to someone who's not stunned
        if( !stunned && tec.stunned_target ) {
            continue;
        }

        // don't apply disarming techniques to someone without a weapon
        // TODO: these are the stat requirements for tec_disarm
        // dice(   dex_cur +    get_skill_level("unarmed"),  8) >
        // dice(p->dex_cur + p->get_skill_level("melee"),   10))
        if( tec.disarms && !t.has_weapon() ) {
            continue;
        }

        if( ( tec.take_weapon && ( has_weapon() || !t.has_weapon() ) ) ) {
            continue;
        }

        // Don't apply humanoid-only techniques to non-humanoids
        if( tec.human_target && !t.in_species( HUMAN ) ) {
            continue;
        }
        // if aoe, check if there are valid targets
        if( !tec.aoe.empty() && !valid_aoe_technique( t, tec ) ) {
            continue;
        }

        // If we have negative weighting then roll to see if it's valid this time
        if( tec.weighting < 0 && !one_in( abs( tec.weighting ) ) ) {
            continue;
        }

        if( tec.is_valid_player( *this ) ) {
            possible.push_back( tec.id );

            //add weighted options into the list extra times, to increase their chance of being selected
            if( tec.weighting > 1 ) {
                for( int i = 1; i < tec.weighting; i++ ) {
                    possible.push_back( tec.id );
                }
            }
        }
    }

    return random_entry( possible, tec_none );
}

bool player::valid_aoe_technique( Creature &t, const ma_technique &technique )
{
    std::vector<Creature *> dummy_targets;
    return valid_aoe_technique( t, technique, dummy_targets );
}

bool player::valid_aoe_technique( Creature &t, const ma_technique &technique,
                                  std::vector<Creature *> &targets )
{
    if( technique.aoe.empty() ) {
        return false;
    }

    // pre-computed matrix of adjacent squares
    std::array<int, 9> offset_a = {{0, -1, -1, 1, 0, -1, 1, 1, 0 }};
    std::array<int, 9> offset_b = {{-1, -1, 0, -1, 0, 1, 0, 1, 1 }};

    // filter the values to be between -1 and 1 to avoid indexing the array out of bounds
    int dy = std::max( -1, std::min( 1, t.posy() - posy() ) );
    int dx = std::max( -1, std::min( 1, t.posx() - posx() ) );
    int lookup = dy + 1 + 3 * ( dx + 1 );

    //wide hits all targets adjacent to the attacker and the target
    if( technique.aoe == "wide" ) {
        //check if either (or both) of the squares next to our target contain a possible victim
        //offsets are a pre-computed matrix allowing us to quickly lookup adjacent squares
        tripoint left = pos() + tripoint( offset_a[lookup], offset_b[lookup], 0 );
        tripoint right = pos() + tripoint( offset_b[lookup], -offset_a[lookup], 0 );

        monster *const mon_l = g->critter_at<monster>( left );
        if( mon_l && mon_l->friendly == 0 ) {
            targets.push_back( mon_l );
        }
        monster *const mon_r = g->critter_at<monster>( right );
        if( mon_r && mon_r->friendly == 0 ) {
            targets.push_back( mon_r );
        }

        npc *const npc_l = g->critter_at<npc>( left );
        npc *const npc_r = g->critter_at<npc>( right );
        if( npc_l && npc_l->is_enemy() ) {
            targets.push_back( npc_l );
        }
        if( npc_r && npc_r->is_enemy() ) {
            targets.push_back( npc_r );
        }
        if( !targets.empty() ) {
            return true;
        }
    }

    if( technique.aoe == "impale" ) {
        // Impale hits the target and a single target behind them
        // Check if the square cardinally behind our target, or to the left / right,
        // contains a possible target.
        tripoint left = t.pos() + tripoint( offset_a[lookup], offset_b[lookup], 0 );
        tripoint target_pos = t.pos() + ( t.pos() - pos() );
        tripoint right = t.pos() + tripoint( offset_b[lookup], -offset_b[lookup], 0 );

        monster *const mon_l = g->critter_at<monster>( left );
        monster *const mon_t = g->critter_at<monster>( target_pos );
        monster *const mon_r = g->critter_at<monster>( right );
        if( mon_l && mon_l->friendly == 0 ) {
            targets.push_back( mon_l );
        }
        if( mon_t && mon_t->friendly == 0 ) {
            targets.push_back( mon_t );
        }
        if( mon_r && mon_r->friendly == 0 ) {
            targets.push_back( mon_r );
        }

        npc *const npc_l = g->critter_at<npc>( left );
        npc *const npc_t = g->critter_at<npc>( target_pos );
        npc *const npc_r = g->critter_at<npc>( right );
        if( npc_l && npc_l->is_enemy() ) {
            targets.push_back( npc_l );
        }
        if( npc_t && npc_t->is_enemy() ) {
            targets.push_back( npc_t );
        }
        if( npc_r && npc_r->is_enemy() ) {
            targets.push_back( npc_r );
        }
        if( !targets.empty() ) {
            return true;
        }
    }

    if( targets.empty() && technique.aoe == "spin" ) {
        for( const tripoint &tmp : g->m.points_in_radius( pos(), 1 ) ) {
            if( tmp == t.pos() ) {
                continue;
            }
            monster *const mon = g->critter_at<monster>( tmp );
            if( mon && mon->friendly == 0 ) {
                targets.push_back( mon );
            }
            npc *const np = g->critter_at<npc>( tmp );
            if( np && np->is_enemy() ) {
                targets.push_back( np );
            }
        }
        //don't trigger circle for fewer than 2 targets
        if( targets.size() < 2 ) {
            targets.clear();
        } else {
            return true;
        }
    }
    return false;
}

bool player::has_technique( const matec_id &id, const item &weap ) const
{
    return weap.has_technique( id ) ||
           style_selected.obj().has_technique( *this, id );
}

static damage_unit &get_damage_unit( std::vector<damage_unit> &di, const damage_type dt )
{
    static damage_unit nullunit( DT_NULL, 0, 0, 0, 0 );
    for( auto &du : di ) {
        if( du.type == dt && du.amount > 0 ) {
            return du;
        }
    }

    return nullunit;
}

static void print_damage_info( const damage_instance &di )
{
    if( !debug_mode ) {
        return;
    }

    int total = 0;
    std::ostringstream ss;
    for( auto &du : di.damage_units ) {
        int amount = di.type_damage( du.type );
        total += amount;
        ss << name_by_dt( du.type ) << ':' << amount << ',';
    }

    add_msg( m_debug, "%stotal: %d", ss.str(), total );
}

void player::perform_technique( const ma_technique &technique, Creature &t, damage_instance &di,
                                int &move_cost )
{
    add_msg( m_debug, "dmg before tec:" );
    print_damage_info( di );

    for( damage_unit &du : di.damage_units ) {
        // TODO: Allow techniques to add more damage types to attacks
        if( du.amount <= 0 ) {
            continue;
        }

        du.amount += technique.damage_bonus( *this, du.type );
        du.damage_multiplier *= technique.damage_multiplier( *this, du.type );
        du.res_pen += technique.armor_penetration( *this, du.type );
    }

    add_msg( m_debug, "dmg after tec:" );
    print_damage_info( di );

    move_cost *= technique.move_cost_multiplier( *this );
    move_cost += technique.move_cost_penalty( *this );

    if( technique.down_dur > 0 ) {
        if( t.get_throw_resist() == 0 ) {
            t.add_effect( effect_downed, rng( 1_turns, time_duration::from_turns( technique.down_dur ) ) );
            auto &bash = get_damage_unit( di.damage_units, DT_BASH );
            if( bash.amount > 0 ) {
                bash.amount += 3;
            }
        }
    }

    if( technique.side_switch ) {
        const tripoint b = t.pos();
        int newx;
        int newy;

        if( b.x > posx() ) {
            newx = posx() - 1;
        } else if( b.x < posx() ) {
            newx = posx() + 1;
        } else {
            newx = b.x;
        }

        if( b.y > posy() ) {
            newy = posy() - 1;
        } else if( b.y < posy() ) {
            newy = posy() + 1;
        } else {
            newy = b.y;
        }

        const tripoint &dest = tripoint( newx, newy, b.z );
        if( g->is_empty( dest ) ) {
            t.setpos( dest );
        }
    }

    if( technique.stun_dur > 0 && !technique.powerful_knockback ) {
        t.add_effect( effect_stunned, rng( 1_turns, time_duration::from_turns( technique.stun_dur ) ) );
    }

    if( technique.knockback_dist ) {
        const tripoint prev_pos = t.pos(); // track target startpoint for knockback_follow
        const int kb_offset_x = rng( -technique.knockback_spread, technique.knockback_spread );
        const int kb_offset_y = rng( -technique.knockback_spread, technique.knockback_spread );
        tripoint kb_point( posx() + kb_offset_x, posy() + kb_offset_y, posz() );
        for( int dist = rng( 1, technique.knockback_dist ); dist > 0; dist-- ) {
            t.knock_back_from( kb_point );
        }
        // This technique makes the player follow into the tile the target was knocked from
        if( technique.knockback_follow ) {
            const optional_vpart_position vp0 = g->m.veh_at( pos() );
            vehicle *const veh0 = veh_pointer_or_null( vp0 );
            bool to_swimmable = g->m.has_flag( "SWIMMABLE", prev_pos );
            bool to_deepwater = g->m.has_flag( TFLAG_DEEP_WATER, prev_pos );

            // Check if it's possible to move to the new tile
            bool move_issue =
                g->is_dangerous_tile( prev_pos ) || // Tile contains fire, etc
                ( to_swimmable && to_deepwater ) || // Dive into deep water
                is_mounted() ||
                ( veh0 != nullptr && abs( veh0->velocity ) > 100 ) || // Diving from moving vehicle
                ( veh0 != nullptr && veh0->player_in_control( g->u ) ) || // Player is driving
                has_effect( effect_amigara );

            if( !move_issue ) {
                if( t.pos() != prev_pos ) {
                    g->place_player( prev_pos );
                    g->on_move_effects();
                }
            }
        }
    }

    player *p = dynamic_cast<player *>( &t );

    if( technique.take_weapon && !has_weapon() && p != nullptr && p->is_armed() ) {
        if( p->is_player() ) {
            add_msg_if_npc( _( "<npcname> disarms you and takes your weapon!" ) );
        } else {
            add_msg_player_or_npc( _( "You disarm %s and take their weapon!" ),
                _( "<npcname> disarms %s and takes their weapon!" ),
                p->name );
        }
        item &it = p->remove_weapon();
        wield( it );
    }

    if( technique.disarms && p != nullptr && p->is_armed() ) {
        g->m.add_item_or_charges( p->pos(), p->remove_weapon() );
        if( p->is_player() ) {
            add_msg_if_npc( _( "<npcname> disarms you!" ) );
        } else {
            add_msg_player_or_npc( _( "You disarm %s!" ),
                                   _( "<npcname> disarms %s!" ),
                                   p->name );
        }
    }

    //AOE attacks, feel free to skip over this lump
    if( technique.aoe.length() > 0 ) {
        // Remember out moves and stamina
        // We don't want to consume them for every attack!
        const int temp_moves = moves;
        const int temp_stamina = stamina;

        std::vector<Creature *> targets;

        valid_aoe_technique( t, technique, targets );

        //hit only one valid target (pierce through doesn't spread out)
        if( technique.aoe == "impale" ) {
            // TODO: what if targets is empty
            Creature *const v = random_entry( targets );
            targets.clear();
            targets.push_back( v );
        }

        //hit the targets in the lists (all candidates if wide or burst, or just the unlucky sod if deep)
        int count_hit = 0;
        for( Creature *const c : targets ) {
            melee_attack( *c, false );
        }

        t.add_msg_if_player( m_good, ngettext( "%d enemy hit!", "%d enemies hit!", count_hit ), count_hit );
        // Extra attacks are free of charge (otherwise AoE attacks would SUCK)
        moves = temp_moves;
        stamina = temp_stamina;
    }

    //player has a very small chance, based on their intelligence, to learn a style whilst using the CQB bionic
    if( has_active_bionic( bio_cqb ) && !has_martialart( style_selected ) ) {
        /** @EFFECT_INT slightly increases chance to learn techniques when using CQB bionic */
        // Enhanced Memory Banks bionic doubles chance to learn martial art
        const int bionic_boost = has_active_bionic( bionic_id( bio_memory ) ) ? 2 : 1;
        if( one_in( ( 1400 - ( get_int() * 50 ) ) / bionic_boost ) ) {
            ma_styles.push_back( style_selected );
            add_msg_if_player( m_good, _( "You have learned %s from extensive practice with the CQB Bionic." ),
                               style_selected.obj().name );
        }
    }
}

static int blocking_ability( const item &shield )
{
    int block_bonus = 0;
    if( shield.has_technique( WBLOCK_3 ) ) {
        block_bonus = 10;
    } else if( shield.has_technique( WBLOCK_2 ) ) {
        block_bonus = 6;
    } else if( shield.has_technique( WBLOCK_1 ) ) {
        block_bonus = 4;
    } else if( shield.has_flag( "BLOCK_WHILE_WORN" ) ) {
        block_bonus = 2;
    }
    return block_bonus;
}

item &player::best_shield()
{
    // Note: wielded weapon, not one used for attacks
    int best_value = blocking_ability( weapon );
    // "BLOCK_WHILE_WORN" without a blocking tech need to be worn for the bonus
    best_value = best_value == 2 ? 0 : best_value;
    item *best = best_value > 0 ? &weapon : &null_item_reference();
    for( item &shield : worn ) {
        if( shield.has_flag( "BLOCK_WHILE_WORN" ) && blocking_ability( shield ) >= best_value ) {
            best = &shield;
        }
    }

    return *best;
}

bool player::block_hit( Creature *source, body_part &bp_hit, damage_instance &dam )
{

    // Shouldn't block if player is asleep or winded ; this only seems to be used by player.
    // TODO: It should probably be moved to the section that regenerates blocks
    // and to effects that disallow blocking
    if( blocks_left < 1 || in_sleep_state() || has_effect( effect_narcosis ) ||
        has_effect( efftype_id( "winded" ) ) ) {
        return false;
    }
    blocks_left--;

    ma_ongethit_effects(); // fire martial arts on-getting-hit-triggered effects
    // these fire even if the attack is blocked (you still got hit)

    // This bonus absorbs damage from incoming attacks before they land,
    // but it still counts as a block even if it absorbs all the damage.
    float total_phys_block = mabuff_block_bonus();
    // Extract this to make it easier to implement shields/multiwield later
    item &shield = best_shield();
    bool conductive_shield = shield.conductive();
    bool unarmed = shield.has_flag( "UNARMED_WEAPON" );

    // This gets us a number between:
    // str ~0 + skill 0 = 0
    // str ~20 + skill 10 + 10(unarmed skill or weapon bonus) = 40
    int block_score = 1;
    // Remember if we're using a weapon or a limb to block.
    // So that we don't suddenly switch that for any reason.
    const bool weapon_blocking = !shield.is_null();
    if( !is_force_unarmed() && weapon_blocking ) {
        /** @EFFECT_STR increases attack blocking effectiveness with a weapon */

        /** @EFFECT_MELEE increases attack blocking effectiveness with a weapon */
        block_bonus = blocking_ability( shield );
        block_score = str_cur + block_bonus + get_skill_level( skill_melee );
    } else if( can_limb_block() ) {
        /** @EFFECT_STR increases attack blocking effectiveness with a limb */

        /** @EFFECT_UNARMED increases attack blocking effectiveness with a limb */
        block_score = str_cur + get_skill_level( skill_melee ) + get_skill_level( skill_unarmed );
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
            if( !is_force_unarmed() && weapon_blocking && !unarmed ) {
                float previous_amount = elem.amount;
                elem.amount /= 5;
                damage_blocked += previous_amount - elem.amount;
            }
            // electrical damage deals full damage if unarmed OR wielding a
            // conductive weapon
        } else if( elem.type == DT_ELECTRIC ) {
            // Unarmed weapons and conductive weapons won't block this
            if( !is_force_unarmed() && weapon_blocking && !unarmed && !conductive_shield ) {
                float previous_amount = elem.amount;
                elem.amount /= 5;
                damage_blocked += previous_amount - elem.amount;
            }
        }
    }

    ma_onblock_effects(); // fire martial arts block-triggered effects

    // weapon blocks are preferred to limb blocks
    std::string thing_blocked_with;
    if( !is_force_unarmed() && weapon_blocking ) {
        thing_blocked_with = shield.tname();
        // TODO: Change this depending on damage blocked
        float wear_modifier = 1.0f;
        if( source != nullptr && source->is_hallucination() ) {
            wear_modifier = 0.0f;
        }

        handle_melee_wear( shield, wear_modifier );
    } else {
        //Choose which body part to block with, assume left side first
        if( can_leg_block() && can_arm_block() ) {
            bp_hit = one_in( 2 ) ? bp_leg_l : bp_arm_l;
        } else if( can_leg_block() ) {
            bp_hit = bp_leg_l;
        } else {
            bp_hit = bp_arm_l;
        }

        // Check if we should actually use the right side to block
        if( bp_hit == bp_leg_l ) {
            if( hp_cur[hp_leg_r] > hp_cur[hp_leg_l] ) {
                bp_hit = bp_leg_r;
            }
        } else {
            if( hp_cur[hp_arm_r] > hp_cur[hp_arm_l] ) {
                bp_hit = bp_arm_r;
            }
        }

        thing_blocked_with = body_part_name( bp_hit );
    }

    std::string damage_blocked_description;
    // good/bad/ugly add_msg color code?
    // none, hardly any, a little, some, most, all
    float blocked_ratio = 0.0f;
    if( total_damage > std::numeric_limits<float>::epsilon() ) {
        blocked_ratio = ( total_damage - damage_blocked ) / total_damage;
    }
    if( blocked_ratio < std::numeric_limits<float>::epsilon() ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _( "all" );
    } else if( blocked_ratio < 0.2 ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _( "nearly all" );
    } else if( blocked_ratio < 0.4 ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _( "most" );
    } else if( blocked_ratio < 0.6 ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _( "a lot" );
    } else if( blocked_ratio < 0.8 ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _( "some" );
    } else if( blocked_ratio > std::numeric_limits<float>::epsilon() ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _( "a little" );
    } else {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _( "none" );
    }
    add_msg_player_or_npc( _( "You block %1$s of the damage with your %2$s!" ),
                           _( "<npcname> blocks %1$s of the damage with their %2$s!" ),
                           damage_blocked_description, thing_blocked_with );

    // Check if we have any block counters
    matec_id tec = pick_technique( *source, shield, false, false, true );

    if( tec != tec_none ) {
        melee_attack( *source, false, tec );
    }

    return true;
}

void player::perform_special_attacks( Creature &t )
{
    bool can_poison = false;

    std::vector<special_attack> special_attacks = mutation_attacks( t );

    std::string target = t.disp_name();

    bool practiced = false;
    for( const auto &att : special_attacks ) {
        if( t.is_dead_state() ) {
            break;
        }

        dealt_damage_instance dealt_dam;

        // TODO: Make this hit roll use unarmed skill, not weapon skill + weapon to_hit
        int hit_spread = t.deal_melee_attack( this, hit_roll() * 0.8 );
        if( hit_spread >= 0 ) {
            t.deal_melee_hit( this, hit_spread, false, att.damage, dealt_dam );
            if( !practiced ) {
                // Practice unarmed, at most once per combo
                practiced = true;
                practice( skill_unarmed, rng( 0, 10 ) );
            }
        }
        int dam = dealt_dam.total_damage();
        if( dam > 0 ) {
            player_hit_message( this, att.text, t, dam );
        }

        can_poison = can_poison ||
                     dealt_dam.type_damage( DT_CUT ) > 0 ||
                     dealt_dam.type_damage( DT_STAB ) > 0;
    }

    if( can_poison && ( has_trait( trait_POISONOUS ) || has_trait( trait_POISONOUS2 ) ) ) {
        if( has_trait( trait_POISONOUS ) ) {
            add_msg_if_player( m_good, _( "You poison %s!" ), target );
            t.add_effect( effect_poison, 6_turns );
        } else if( has_trait( trait_POISONOUS2 ) ) {
            add_msg_if_player( m_good, _( "You inject your venom into %s!" ), target );
            t.add_effect( effect_badpoison, 6_turns );
        }
    }
}

std::string player::melee_special_effects( Creature &t, damage_instance &d, item &weap )
{
    std::stringstream dump;

    std::string target = t.disp_name();

    if( has_active_bionic( bionic_id( "bio_shock" ) ) && get_power_level() >= 2_kJ &&
        ( !is_armed() || weapon.conductive() ) ) {
        mod_power_level( -2_kJ );
        d.add_damage( DT_ELECTRIC, rng( 2, 10 ) );

        if( is_player() ) {
            dump << string_format( _( "You shock %s." ), target ) << std::endl;
        } else {
            add_msg_if_npc( _( "<npcname> shocks %s." ), target );
        }
    }

    if( has_active_bionic( bionic_id( "bio_heat_absorb" ) ) && !is_armed() && t.is_warm() ) {
        mod_power_level( 3_kJ );
        d.add_damage( DT_COLD, 3 );
        if( is_player() ) {
            dump << string_format( _( "You drain %s's body heat." ), target ) << std::endl;
        } else {
            add_msg_if_npc( _( "<npcname> drains %s's body heat!" ), target );
        }
    }

    if( weapon.has_flag( "FLAMING" ) ) {
        d.add_damage( DT_HEAT, rng( 1, 8 ) );

        if( is_player() ) {
            dump << string_format( _( "You burn %s." ), target ) << std::endl;
        } else {
            add_msg_player_or_npc( _( "<npcname> burns %s." ), target );
        }
    }

    //Hurting the wielder from poorly-chosen weapons
    if( weap.has_flag( "HURT_WHEN_WIELDED" ) && x_in_y( 2, 3 ) ) {
        add_msg_if_player( m_bad, _( "The %s cuts your hand!" ), weap.tname() );
        deal_damage( nullptr, bp_hand_r, damage_instance::physical( 0, weap.damage_melee( DT_CUT ), 0 ) );
        if( weap.is_two_handed( *this ) ) { // Hurt left hand too, if it was big
            deal_damage( nullptr, bp_hand_l, damage_instance::physical( 0, weap.damage_melee( DT_CUT ), 0 ) );
        }
    }

    const int vol = weap.volume() / 250_ml;
    // Glass weapons shatter sometimes
    if( weap.made_of( material_id( "glass" ) ) &&
        /** @EFFECT_STR increases chance of breaking glass weapons (NEGATIVE) */
        rng( 0, vol + 8 ) < vol + str_cur ) {
        if( is_player() ) {
            dump << string_format( _( "Your %s shatters!" ), weap.tname() ) << std::endl;
        } else {
            add_msg_player_or_npc( m_bad, _( "Your %s shatters!" ),
                                   _( "<npcname>'s %s shatters!" ),
                                   weap.tname() );
        }

        sounds::sound( pos(), 16, sounds::sound_t::combat, "Crack!", true, "smash_success",
                       "smash_glass_contents" );
        // Dump its contents on the ground
        for( auto &elem : weap.contents ) {
            g->m.add_item_or_charges( pos(), elem );
        }
        // Take damage
        deal_damage( nullptr, bp_arm_r, damage_instance::physical( 0, rng( 0, vol * 2 ), 0 ) );
        if( weap.is_two_handed( *this ) ) { // Hurt left arm too, if it was big
            //redeclare shatter_dam because deal_damage mutates it
            deal_damage( nullptr, bp_arm_l, damage_instance::physical( 0, rng( 0, vol * 2 ), 0 ) );
        }
        d.add_damage( DT_CUT, rng( 0, 5 + static_cast<int>( vol * 1.5 ) ) ); // Hurt the monster extra
        remove_weapon();
    }

    if( !t.is_hallucination() ) {
        handle_melee_wear( weap );
    }

    // on-hit effects for martial arts
    ma_onhit_effects();

    return dump.str();
}

static damage_instance hardcoded_mutation_attack( const player &u, const trait_id &id )
{
    if( id == "BEAK_PECK" ) {
        // method open to improvement, please feel free to suggest
        // a better way to simulate target's anti-peck efforts
        /** @EFFECT_DEX increases number of hits with BEAK_PECK */

        /** @EFFECT_UNARMED increases number of hits with BEAK_PECK */
        int num_hits = std::max( 1, std::min<int>( 6,
                                 u.get_dex() + u.get_skill_level( skill_unarmed ) - rng( 4, 10 ) ) );
        return damage_instance::physical( 0, 0, num_hits * 10 );
    }

    if( id == "ARM_TENTACLES" || id == "ARM_TENTACLES_4" || id == "ARM_TENTACLES_8" ) {
        int num_attacks = 1;
        if( id == "ARM_TENTACLES_4" ) {
            num_attacks = 3;
        } else if( id == "ARM_TENTACLES_8" ) {
            num_attacks = 7;
        }
        // Note: we're counting arms, so we want wielded item here, not weapon used for attack
        if( u.weapon.is_two_handed( u ) || !u.has_two_arms() || u.worn_with_flag( "RESTRICT_HANDS" ) ) {
            num_attacks--;
        }

        if( num_attacks <= 0 ) {
            return damage_instance();
        }

        const bool rake = u.has_trait( trait_CLAWS_TENTACLE );

        /** @EFFECT_STR increases damage with ARM_TENTACLES* */
        damage_instance ret;
        if( rake ) {
            ret.add_damage( DT_CUT, u.get_str() / 2.0f + 1.0f, 0, 1.0f, num_attacks );
        } else {
            ret.add_damage( DT_BASH, u.get_str() / 3.0f + 1.0f, 0, 1.0f, num_attacks );
        }

        return ret;
    }

    if( id == "VINES2" || id == "VINES3" ) {
        const int num_attacks = id == "VINES2" ? 2 : 3;
        /** @EFFECT_STR increases damage with VINES* */
        damage_instance ret;
        ret.add_damage( DT_BASH, u.get_str() / 2.0f, 0, 1.0f, num_attacks );
        return ret;
    }

    debugmsg( "Invalid hardcoded mutation id: %s", id.c_str() );
    return damage_instance();
}

std::vector<special_attack> player::mutation_attacks( Creature &t ) const
{
    std::vector<special_attack> ret;

    std::string target = t.disp_name();

    const auto usable_body_parts = exclusive_flag_coverage( "ALLOWS_NATURAL_ATTACKS" );
    const int unarmed = get_skill_level( skill_unarmed );

    for( const auto &pr : my_mutations ) {
        const auto &branch = pr.first.obj();
        for( const auto &mut_atk : branch.attacks_granted ) {
            // Covered body part
            if( mut_atk.bp != num_bp && !usable_body_parts.test( mut_atk.bp ) ) {
                continue;
            }

            /** @EFFECT_UNARMED increases chance of attacking with mutated body parts */
            /** @EFFECT_DEX increases chance of attacking with mutated body parts */

            // Calculate actor ability value to be compared against mutation attack difficulty and add debug message
            const int proc_value = get_dex() + unarmed;
            add_msg( m_debug, "%s proc chance: %d in %d", pr.first.c_str(), proc_value, mut_atk.chance );
            // If the mutation attack fails to proc, bail out
            if( !x_in_y( proc_value, mut_atk.chance ) ) {
                continue;
            }

            // If player has any blocker, bail out
            if( std::any_of( mut_atk.blocker_mutations.begin(), mut_atk.blocker_mutations.end(),
            [this]( const trait_id & blocker ) {
            return has_trait( blocker );
            } ) ) {
                add_msg( m_debug, "%s not procing: blocked", pr.first.c_str() );
                continue;
            }

            // Player must have all needed traits
            if( !std::all_of( mut_atk.required_mutations.begin(), mut_atk.required_mutations.end(),
            [this]( const trait_id & need ) {
            return has_trait( need );
            } ) ) {
                add_msg( m_debug, "%s not procing: unmet req", pr.first.c_str() );
                continue;
            }

            special_attack tmp;
            // Ugly special case: player's strings have only 1 variable, NPC have 2
            // Can't use <npcname> here
            // TODO: Fix
            if( is_player() ) {
                tmp.text = string_format( _( mut_atk.attack_text_u ), target );
            } else {
                tmp.text = string_format( _( mut_atk.attack_text_npc ), name, target );
            }

            // Attack starts here
            if( mut_atk.hardcoded_effect ) {
                tmp.damage = hardcoded_mutation_attack( *this, pr.first );
            } else {
                damage_instance dam = mut_atk.base_damage;
                damage_instance scaled = mut_atk.strength_damage;
                scaled.mult_damage( std::min<float>( 15.0f, get_str() ), true );
                dam.add( scaled );

                tmp.damage = dam;
            }

            if( tmp.damage.total_damage() > 0.0f ) {
                ret.emplace_back( tmp );
            } else {
                add_msg( m_debug, "%s not procing: zero damage", pr.first.c_str() );
            }
        }
    }

    return ret;
}

std::string melee_message( const ma_technique &tec, player &p, const dealt_damage_instance &ddi )
{
    // Those could be extracted to a json

    // Three last values are for low damage
    static const std::array<std::string, 6> player_stab = {{
            translate_marker( "You impale %s" ),
            translate_marker( "You gouge %s" ),
            translate_marker( "You run %s through" ),
            translate_marker( "You puncture %s" ),
            translate_marker( "You pierce %s" ),
            translate_marker( "You poke %s" )
        }
    };
    static const std::array<std::string, 6> npc_stab = {{
            translate_marker( "<npcname> impales %s" ),
            translate_marker( "<npcname> gouges %s" ),
            translate_marker( "<npcname> runs %s through" ),
            translate_marker( "<npcname> punctures %s" ),
            translate_marker( "<npcname> pierces %s" ),
            translate_marker( "<npcname> pokes %s" )
        }
    };
    // First 5 are for high damage, next 2 for medium, then for low and then for v. low
    static const std::array<std::string, 9> player_cut = {{
            translate_marker( "You gut %s" ),
            translate_marker( "You chop %s" ),
            translate_marker( "You slash %s" ),
            translate_marker( "You mutilate %s" ),
            translate_marker( "You maim %s" ),
            translate_marker( "You stab %s" ),
            translate_marker( "You slice %s" ),
            translate_marker( "You cut %s" ),
            translate_marker( "You nick %s" )
        }
    };
    static const std::array<std::string, 9> npc_cut = {{
            translate_marker( "<npcname> guts %s" ),
            translate_marker( "<npcname> chops %s" ),
            translate_marker( "<npcname> slashes %s" ),
            translate_marker( "<npcname> mutilates %s" ),
            translate_marker( "<npcname> maims %s" ),
            translate_marker( "<npcname> stabs %s" ),
            translate_marker( "<npcname> slices %s" ),
            translate_marker( "<npcname> cuts %s" ),
            translate_marker( "<npcname> nicks %s" )
        }
    };

    // Three last values are for low damage
    static const std::array<std::string, 6> player_bash = {{
            translate_marker( "You clobber %s" ),
            translate_marker( "You smash %s" ),
            translate_marker( "You thrash %s" ),
            translate_marker( "You batter %s" ),
            translate_marker( "You whack %s" ),
            translate_marker( "You hit %s" )
        }
    };
    static const std::array<std::string, 6> npc_bash = {{
            translate_marker( "<npcname> clobbers %s" ),
            translate_marker( "<npcname> smashes %s" ),
            translate_marker( "<npcname> thrashes %s" ),
            translate_marker( "<npcname> batters %s" ),
            translate_marker( "<npcname> whacks %s" ),
            translate_marker( "<npcname> hits %s" )
        }
    };

    const int bash_dam = ddi.type_damage( DT_BASH );
    const int cut_dam  = ddi.type_damage( DT_CUT );
    const int stab_dam = ddi.type_damage( DT_STAB );

    if( tec.id != tec_none ) {
        std::string message;
        if( p.is_npc() ) {
            message = _( tec.npc_message );
        } else {
            message = _( tec.player_message );
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
        return npc ? _( npc_stab[index] ) : _( player_stab[index] );
    } else if( dominant_type == DT_CUT ) {
        return npc ? _( npc_cut[index] ) : _( player_cut[index] );
    } else if( dominant_type == DT_BASH ) {
        return npc ? _( npc_bash[index] ) : _( player_bash[index] );
    }

    return _( "The bugs attack %s" );
}

// display the hit message for an attack
void player_hit_message( player *attacker, const std::string &message,
                         Creature &t, int dam, bool crit )
{
    std::string msg;
    game_message_type msgtype = m_good;
    std::string sSCTmod;
    game_message_type gmtSCTcolor = m_good;

    if( dam <= 0 ) {
        if( attacker->is_npc() ) {
            //~ NPC hits something but does no damage
            msg = string_format( _( "%s but does no damage." ), message );
        } else {
            //~ someone hits something but do no damage
            msg = string_format( _( "%s but do no damage." ), message );
        }
        msgtype = m_neutral;
    } else if(
        crit ) { //Player won't see exact numbers of damage dealt by NPC unless player has DEBUG_NIGHTVISION trait
        if( attacker->is_npc() && !g->u.has_trait( trait_DEBUG_NIGHTVISION ) ) {
            //~ NPC hits something (critical)
            msg = string_format( _( "%s. Critical!" ), message );
        } else {
            //~ someone hits something for %d damage (critical)
            msg = string_format( _( "%s for %d damage. Critical!" ), message, dam );
        }
        sSCTmod = _( "Critical!" );
        gmtSCTcolor = m_critical;
    } else {
        if( attacker->is_npc() && !g->u.has_trait( trait_DEBUG_NIGHTVISION ) ) {
            //~ NPC hits something
            msg = string_format( _( "%s." ), message );
        } else {
            //~ someone hits something for %d damage
            msg = string_format( _( "%s for %d damage." ), message, dam );
        }
    }

    if( dam > 0 && attacker->is_player() ) {
        //player hits monster melee
        SCT.add( point( t.posx(), t.posy() ),
                 direction_from( point_zero, point( t.posx() - attacker->posx(), t.posy() - attacker->posy() ) ),
                 get_hp_bar( dam, t.get_hp_max(), true ).first, m_good,
                 sSCTmod, gmtSCTcolor );

        if( t.get_hp() > 0 ) {
            SCT.add( point( t.posx(), t.posy() ),
                     direction_from( point_zero, point( t.posx() - attacker->posx(), t.posy() - attacker->posy() ) ),
                     get_hp_bar( t.get_hp(), t.get_hp_max(), true ).first, m_good,
                     //~ "hit points", used in scrolling combat text
                     _( "hp" ), m_neutral,
                     "hp" );
        } else {
            SCT.removeCreatureHP();
        }
    }

    // same message is used for player and npc,
    // just using this for the <npcname> substitution.
    attacker->add_msg_player_or_npc( msgtype, msg, msg, t.disp_name() );
}

int player::attack_speed( const item &weap ) const
{
    const int base_move_cost = weap.attack_time() / 2;
    const int melee_skill = has_active_bionic( bionic_id( bio_cqb ) ) ? BIO_CQB_LEVEL : get_skill_level(
                                skill_melee );
    /** @EFFECT_MELEE increases melee attack speed */
    const int skill_cost = static_cast<int>( ( base_move_cost * ( 15 - melee_skill ) / 15 ) );
    /** @EFFECT_DEX increases attack speed */
    const int dexbonus = dex_cur / 2;
    const int encumbrance_penalty = encumb( bp_torso ) +
                                    ( encumb( bp_hand_l ) + encumb( bp_hand_r ) ) / 2;
    const int ma_move_cost = mabuff_attack_cost_penalty();
    const float stamina_ratio = static_cast<float>( stamina ) / static_cast<float>( get_stamina_max() );
    // Increase cost multiplier linearly from 1.0 to 2.0 as stamina goes from 25% to 0%.
    const float stamina_penalty = 1.0 + std::max( ( 0.25f - stamina_ratio ) * 4.0f, 0.0f );
    const float ma_mult = mabuff_attack_cost_mult();

    int move_cost = base_move_cost;
    // Stamina penalty only affects base/2 and encumbrance parts of the cost
    move_cost += encumbrance_penalty;
    move_cost *= stamina_penalty;
    move_cost += skill_cost;
    move_cost -= dexbonus;

    move_cost = calculate_by_enchantment( move_cost, enchantment::mod::ATTACK_SPEED, true );
    // Martial arts last. Flat has to be after mult, because comments say so.
    move_cost *= ma_mult;
    move_cost += ma_move_cost;

    move_cost *= mutation_value( "attackcost_modifier" );

    if( move_cost < 25 ) {
        return 25;
    }

    return move_cost;
}

double player::weapon_value( const item &weap, int ammo ) const
{
    if( &weapon == &weap ) {
        auto cached_value = cached_info.find( "weapon_value" );
        if( cached_value != cached_info.end() ) {
            return cached_value->second;
        }
    }
    const double val_gun = gun_value( weap, ammo );
    const double val_melee = melee_value( weap );
    const double more = std::max( val_gun, val_melee );
    const double less = std::min( val_gun, val_melee );

    // A small bonus for guns you can also use to hit stuff with (bayonets etc.)
    const double my_val = more + ( less / 2.0 );
    add_msg( m_debug, "%s (%ld ammo) sum value: %.1f", weap.type->get_id(), ammo, my_val );
    if( &weapon == &weap ) {
        cached_info.emplace( "weapon_value", my_val );
    }
    return my_val;
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
        my_value += 15 + ( accuracy - 5 );
    }

    int move_cost = attack_speed( weap );
    static const matec_id rapid_strike( "RAPID" );
    if( weap.has_technique( rapid_strike ) ) {
        move_cost /= 2;
        avg_dmg *= 0.66;
    }

    const int arbitrary_dodge_target = 5;
    double crit_ch = crit_chance( accuracy, arbitrary_dodge_target, weap );
    my_value += crit_ch * 10; // Criticals are worth more than just extra damage
    if( crit_ch > 0.1 ) {
        damage_instance crit;
        roll_all_damage( true, crit, true, weap );
        // Note: intentionally doesn't include rapid attack bonus in criticals
        avg_dmg = ( 1.0 - crit_ch ) * avg_dmg + crit.total_damage() * crit_ch;
    }

    my_value += avg_dmg * 100 / move_cost;

    float reach = weap.reach_range( *this );
    if( reach > 1.0f ) {
        my_value *= 1.0f + 0.5f * ( sqrtf( reach ) - 1.0f );
    }

    add_msg( m_debug, "%s as melee: %.1f", weap.type->get_id(), my_value );

    return std::max( 0.0, my_value );
}

double player::unarmed_value() const
{
    // TODO: Martial arts
    return melee_value( item() );
}

void player::disarm( npc &target )
{
    if( !target.is_armed() ) {
        return;
    }

    if( target.is_hallucination() ) {
        target.on_attacked( *this );
        return;
    }

    /** @EFFECT_STR increases chance to disarm, primary stat */
    /** @EFFECT_DEX increases chance to disarm, secondary stat */
    int my_roll = dice( 3, 2 * get_str() + get_dex() );

    /** @EFFECT_MELEE increases chance to disarm */
    my_roll += dice( 3, get_skill_level( skill_melee ) );

    int their_roll = dice( 3, 2 * target.get_str() + target.get_dex() );
    their_roll += dice( 3, target.get_per() );
    their_roll += dice( 3, target.get_skill_level( skill_melee ) );

    item &it = target.weapon;

    // roll your melee and target's dodge skills to check if grab/smash attack succeeds
    int hitspread = target.deal_melee_attack( this, hit_roll() );
    if( hitspread < 0 ) {
        add_msg( _( "You lunge for the %s, but miss!" ), it.tname() );
        mod_moves( -100 - stumble( *this, weapon ) - attack_speed( weapon ) );
        target.on_attacked( *this );
        return;
    }

    // hitspread >= 0, which means we are going to disarm by grabbing target by their weapon
    if( !is_armed() ) {
        /** @EFFECT_UNARMED increases chance to disarm, bonus when nothing wielded */
        my_roll += dice( 3, get_skill_level( skill_unarmed ) );

        if( my_roll >= their_roll ) {
            //~ %s: weapon name
            add_msg( _( "You grab at %s and pull with all your force!" ), it.tname() );
            //~ %1$s: weapon name, %2$s: NPC name
            add_msg( _( "You forcefully take %1$s from %2$s!" ), it.tname(), target.name );
            // wield() will deduce our moves, consider to deduce more/less moves for balance
            item rem_it = target.i_rem( &it );
            wield( rem_it );
        } else if( my_roll >= their_roll / 2 ) {
            add_msg( _( "You grab at %s and pull with all your force, but it drops nearby!" ),
                     it.tname() );
            const tripoint tp = target.pos() + tripoint( rng( -1, 1 ), rng( -1, 1 ), 0 );
            g->m.add_item_or_charges( tp, target.i_rem( &it ) );
            mod_moves( -100 );
        } else {
            add_msg( _( "You grab at %s and pull with all your force, but in vain!" ), it.tname() );
            mod_moves( -100 );
        }

        target.on_attacked( *this );
        return;
    }

    // Make their weapon fall on floor if we've rolled enough.
    mod_moves( -100 - attack_speed( weapon ) );
    if( my_roll >= their_roll ) {
        add_msg( _( "You smash %s with all your might forcing their %s to drop down nearby!" ),
                 target.name, it.tname() );
        const tripoint tp = target.pos() + tripoint( rng( -1, 1 ), rng( -1, 1 ), 0 );
        g->m.add_item_or_charges( tp, target.i_rem( &it ) );
    } else {
        add_msg( _( "You smash %s with all your might but %s remains in their hands!" ),
                 target.name, it.tname() );
    }

    target.on_attacked( *this );
}

void avatar::steal( npc &target )
{
    if( target.is_enemy() ) {
        add_msg( _( "%s is hostile!" ), target.name );
        return;
    }

    item_location loc = game_menus::inv::steal( *this, target );
    if( !loc ) {
        return;
    }

    /** @EFFECT_DEX defines the chance to steal */
    int my_roll = dice( 3, get_dex() );

    /** @EFFECT_UNARMED adds bonus to stealing when wielding nothing */
    if( !is_armed() ) {
        my_roll += dice( 4, 3 );
    }
    if( has_trait( trait_DEFT ) ) {
        my_roll += dice( 2, 6 );
    }
    if( has_trait( trait_CLUMSY ) ) {
        my_roll -= dice( 4, 6 );
    }

    int their_roll = dice( 5, target.get_per() );

    const item *it = loc.get_item();
    if( my_roll >= their_roll ) {
        add_msg( _( "You sneakily steal %1$s from %2$s!" ),
                 it->tname(), target.name );
        i_add( target.i_rem( it ) );
    } else if( my_roll >= their_roll / 2 ) {
        add_msg( _( "You failed to steal %1$s from %2$s, but did not attract attention." ),
                 it->tname(), target.name );
    } else  {
        add_msg( _( "You failed to steal %1$s from %2$s." ),
                 it->tname(), target.name );
        target.on_attacked( *this );
    }

    // consider to deduce less/more moves for balance
    mod_moves( -200 );
}

namespace melee
{

/**
 * Once the accuracy (sum of modifiers) of an attack has been determined,
 * this is used to actually roll the "hit value" of the attack to be compared to dodge.
 */
float melee_hit_range( float accuracy )
{
    return normal_roll( accuracy * 5, 25.0f );
}

} // namespace melee
