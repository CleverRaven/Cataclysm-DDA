#include "melee.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <iosfwd>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "anatomy.h"
#include "bodypart.h"
#include "bionics.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "character_martial_arts.h"
#include "creature.h"
#include "creature_tracker.h"
#include "damage.h"
#include "debug.h"
#include "effect_on_condition.h"
#include "enum_bitset.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "line.h"
#include "magic_enchantment.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "martialarts.h"
#include "memory_fast.h"
#include "messages.h"
#include "monattack.h"
#include "monster.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "output.h"
#include "pimpl.h"
#include "point.h"
#include "popup.h"
#include "projectile.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weakpoint.h"
#include "weighted_list.h"

static const anatomy_id anatomy_human_anatomy( "human_anatomy" );

static const bionic_id bio_cqb( "bio_cqb" );
static const bionic_id bio_heat_absorb( "bio_heat_absorb" );
static const bionic_id bio_razors( "bio_razors" );
static const bionic_id bio_shock( "bio_shock" );

static const character_modifier_id
character_modifier_limb_dodge_mod( "limb_dodge_mod" );
static const character_modifier_id
character_modifier_melee_attack_roll_mod( "melee_attack_roll_mod" );
static const character_modifier_id
character_modifier_melee_thrown_move_balance_mod( "melee_thrown_move_balance_mod" );
static const character_modifier_id
character_modifier_melee_thrown_move_lift_mod( "melee_thrown_move_lift_mod" );

static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_cut( "cut" );
static const damage_type_id damage_stab( "stab" );

static const efftype_id effect_amigara( "amigara" );
static const efftype_id effect_beartrap( "beartrap" );
static const efftype_id effect_contacts( "contacts" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_drunk( "drunk" );
static const efftype_id effect_fearparalyze( "fearparalyze" );
static const efftype_id effect_heavysnare( "heavysnare" );
static const efftype_id effect_hit_by_player( "hit_by_player" );
static const efftype_id effect_incorporeal( "incorporeal" );
static const efftype_id effect_lightsnare( "lightsnare" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_natural_stance( "natural_stance" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_transition_contacts( "transition_contacts" );
static const efftype_id effect_venom_dmg( "venom_dmg" );
static const efftype_id effect_venom_player1( "venom_player1" );
static const efftype_id effect_venom_player2( "venom_player2" );
static const efftype_id effect_venom_weaken( "venom_weaken" );
static const efftype_id effect_winded( "winded" );

static const itype_id itype_fur( "fur" );
static const itype_id itype_leather( "leather" );
static const itype_id itype_sheet_cotton( "sheet_cotton" );

static const json_character_flag json_flag_CBQ_LEARN_BONUS( "CBQ_LEARN_BONUS" );
static const json_character_flag json_flag_GRAB( "GRAB" );
static const json_character_flag json_flag_GRAB_FILTER( "GRAB_FILTER" );
static const json_character_flag json_flag_HARDTOHIT( "HARDTOHIT" );
static const json_character_flag json_flag_HYPEROPIC( "HYPEROPIC" );
static const json_character_flag json_flag_NEED_ACTIVE_TO_MELEE( "NEED_ACTIVE_TO_MELEE" );
static const json_character_flag json_flag_NULL( "NULL" );
static const json_character_flag json_flag_PSEUDOPOD_GRASP( "PSEUDOPOD_GRASP" );
static const json_character_flag json_flag_UNARMED_BONUS( "UNARMED_BONUS" );

static const limb_score_id limb_score_block( "block" );
static const limb_score_id limb_score_grip( "grip" );
static const limb_score_id limb_score_reaction( "reaction" );

static const matec_id WBLOCK_1( "WBLOCK_1" );
static const matec_id WBLOCK_2( "WBLOCK_2" );
static const matec_id WBLOCK_3( "WBLOCK_3" );
static const matec_id WHIP_DISARM( "WHIP_DISARM" );

static const material_id material_glass( "glass" );
static const material_id material_steel( "steel" );

static const move_mode_id move_mode_prone( "prone" );

static const skill_id skill_melee( "melee" );
static const skill_id skill_spellcraft( "spellcraft" );
static const skill_id skill_unarmed( "unarmed" );

static const trait_id trait_ARM_TENTACLES( "ARM_TENTACLES" );
static const trait_id trait_ARM_TENTACLES_4( "ARM_TENTACLES_4" );
static const trait_id trait_ARM_TENTACLES_8( "ARM_TENTACLES_8" );
static const trait_id trait_BEAK_PECK( "BEAK_PECK" );
static const trait_id trait_CLAWS_TENTACLE( "CLAWS_TENTACLE" );
static const trait_id trait_CLUMSY( "CLUMSY" );
static const trait_id trait_DEBUG_NIGHTVISION( "DEBUG_NIGHTVISION" );
static const trait_id trait_DEFT( "DEFT" );
static const trait_id trait_DRUNKEN( "DRUNKEN" );
static const trait_id trait_KI_STRIKE( "KI_STRIKE" );
static const trait_id trait_POISONOUS( "POISONOUS" );
static const trait_id trait_POISONOUS2( "POISONOUS2" );
static const trait_id trait_PROF_SKATER( "PROF_SKATER" );
static const trait_id trait_VINES2( "VINES2" );
static const trait_id trait_VINES3( "VINES3" );

static const weapon_category_id weapon_category_UNARMED( "UNARMED" );

static void player_hit_message( Character *attacker, const std::string &message,
                                Creature &t, int dam, bool crit = false, bool technique = false, const std::string &wp_hit = {} );
static int stumble( Character &u, const item_location &weap );
static std::string melee_message( const ma_technique &tec, Character &p,
                                  const dealt_damage_instance &ddi );

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

item_location Character::used_weapon() const
{
    return martial_arts_data->selected_force_unarmed() ? item_location() : get_wielded_item();
}

item_location Character::used_weapon()
{
    return const_cast<const Character *>( this )->used_weapon();
}

bool Character::is_armed() const
{
    return !weapon.is_null();
}

bool Character::unarmed_attack() const
{
    const item_location weap = used_weapon();
    return !weap;
}

bool Character::handle_melee_wear( item_location shield, float wear_multiplier )
{
    if( wear_multiplier <= 0.0f ) {
        return false;
    }
    // Here is where we handle wear and tear on things we use as melee weapons or shields.
    if( !shield ) {
        return false;
    }

    // UNBREAKABLE_MELEE and UNBREAKABLE items can't be damaged through melee combat usage.
    if( shield->has_flag( flag_UNBREAKABLE_MELEE ) || shield->has_flag( flag_UNBREAKABLE ) ) {
        return false;
    }

    /** @EFFECT_DEX reduces chance of damaging your melee weapon */

    /** @ARM_STR increases chance of damaging your melee weapon (NEGATIVE) */

    /** @EFFECT_MELEE reduces chance of damaging your melee weapon */
    const float stat_factor = dex_cur / 2.0f
                              + get_skill_level( skill_melee )
                              + ( 64.0f / std::max( get_arm_str(), 4 ) );

    float material_factor;

    itype_id weak_comp;
    itype_id big_comp = itype_id::NULL_ID();
    // Fragile items that fall apart easily when used as a weapon due to poor construction quality
    if( shield->has_flag( flag_FRAGILE_MELEE ) ) {
        const float fragile_factor = 6.0f;
        int weak_chip = INT_MAX;
        units::volume big_vol = 0_ml;

        // Items that should have no bearing on durability
        const std::set<itype_id> blacklist = { itype_sheet_cotton, itype_leather, itype_fur };

        for( item_components::type_vector_pair &tvp : shield->components ) {
            if( blacklist.count( tvp.first ) > 0 ) {
                continue;
            }
            for( item &comp : tvp.second ) {
                if( weak_chip > comp.chip_resistance() ) {
                    weak_chip = comp.chip_resistance();
                    weak_comp = comp.typeId();
                }
                if( comp.volume() > big_vol ) {
                    big_vol = comp.volume();
                    big_comp = comp.typeId();
                }
            }
        }
        material_factor = ( weak_chip < INT_MAX ? weak_chip : shield->chip_resistance() ) / fragile_factor;
    } else {
        material_factor = shield->chip_resistance();
    }
    int damage_chance = static_cast<int>( stat_factor * material_factor / wear_multiplier );
    // DURABLE_MELEE items are made to hit stuff and they do it well, so they're considered to be a lot tougher
    // than other weapons made of the same materials.
    if( shield->has_flag( flag_DURABLE_MELEE ) ) {
        damage_chance *= 4;
    }

    if( damage_chance > 0 && !one_in( damage_chance ) ) {
        return false;
    }

    std::string str = shield->tname(); // save name before we apply damage

    if( !shield->inc_damage() ) {
        add_msg_player_or_npc( m_bad, _( "Your %s is damaged by the force of the blow!" ),
                               _( "<npcname>'s %s is damaged by the force of the blow!" ),
                               str );
        return false;
    }

    // Dump its contents on the ground
    // Destroy irremovable mods, if any

    for( item *mod : shield->gunmods() ) {
        if( mod->is_irremovable() ) {
            remove_item( *mod );
        }
    }

    // Preserve item temporarily for component breakdown
    item temp = *shield;

    shield->get_contents().spill_contents( pos() );

    shield.remove_item();

    // Breakdown fragile weapons into components
    if( temp.has_flag( flag_FRAGILE_MELEE ) && !temp.components.empty() ) {
        add_msg_player_or_npc( m_bad, _( "Your %s breaks apart!" ),
                               _( "<npcname>'s %s breaks apart!" ),
                               str );

        for( item_components::type_vector_pair &tvp : temp.components ) {
            for( item &comp : tvp.second ) {
                int break_chance = comp.typeId() == weak_comp ? 2 : 8;

                if( one_in( break_chance ) ) {
                    add_msg_if_player( m_bad, _( "The %s is destroyed!" ), comp.tname() );
                    continue;
                }

                if( comp.typeId() == big_comp && !has_wield_conflicts( comp ) ) {
                    wield( comp );
                } else {
                    get_map().add_item_or_charges( pos(), comp );
                }
            }
        }
    } else {
        add_msg_player_or_npc( m_bad, _( "Your %s is destroyed by the blow!" ),
                               _( "<npcname>'s %s is destroyed by the blow!" ),
                               str );
    }

    if( is_using_bionic_weapon() && temp.has_flag( flag_NO_UNWIELD ) ) {
        if( std::optional<bionic *> bio_opt = find_bionic_by_uid( get_weapon_bionic_uid() ) ) {
            bionic &bio = **bio_opt;
            if( bio.get_weapon().typeId() == temp.typeId() ) {
                weapon_bionic_uid = 0;
                bio.set_weapon( item() );
                force_bionic_deactivation( bio );
            }
        }
    }

    return true;
}

float Character::get_hit_weapon( const item &weap ) const
{
    /** @EFFECT_UNARMED improves hit chance for unarmed weapons */
    /** @EFFECT_BASHING improves hit chance for bashing weapons */
    /** @EFFECT_CUTTING improves hit chance for cutting weapons */
    /** @EFFECT_STABBING improves hit chance for piercing weapons */
    float skill = get_skill_level( weap.melee_skill() );

    // CQB bionic acts as a lower bound providing item uses a weapon skill
    if( skill < BIO_CQB_LEVEL && has_active_bionic( bio_cqb ) ) {
        skill = BIO_CQB_LEVEL;
    }

    /** @EFFECT_MELEE improves hit chance for all items (including non-weapons) */
    return ( skill / 3.0f ) + ( get_skill_level( skill_melee ) / 2.0f ) + weap.type->m_to_hit;
}

float Character::get_melee_hit_base() const
{
    float hit_weapon = 0.0f;

    item_location cur_weapon = used_weapon();
    item cur_weap = cur_weapon ? *cur_weapon : null_item_reference();

    hit_weapon = get_hit_weapon( cur_weap );

    // Character::get_hit_base includes stat calculations already
    return Character::get_hit_base() + hit_weapon + mabuff_tohit_bonus();
}

float Character::hit_roll() const
{
    // Dexterity, skills, weapon and martial arts
    float hit = get_melee_hit_base();

    // Farsightedness makes us hit worse
    if( has_flag( json_flag_HYPEROPIC ) && !worn_with_flag( flag_FIX_FARSIGHT ) &&
        !has_effect( effect_contacts ) &&
        !has_effect( effect_transition_contacts ) ) {
        hit -= 2.0f;
    }

    // Difficult to land a hit while prone
    // Quadrupeds don't mind crouching as long as they're unarmed
    // Tentacles and goo-limbs care even less
    item_location cur_weapon = used_weapon();
    item cur_weap = cur_weapon ? *cur_weapon : null_item_reference();
    if( is_on_ground() ) {
        if( has_flag( json_flag_PSEUDOPOD_GRASP ) ) {
            hit -= 2.0f;
        } else {
            hit -= 8.0f;
        }
    } else if( is_crouching() && ( !has_flag( json_flag_PSEUDOPOD_GRASP ) &&
                                   ( !has_effect( effect_natural_stance ) ) ) ) {
        hit -= 2.0f;
    }

    hit *= get_modifier( character_modifier_melee_attack_roll_mod );

    return melee::melee_hit_range( hit );
}

bool Character::can_attack_high() const
{
    return !is_on_ground();
}

void Character::add_miss_reason( const std::string &reason, const unsigned int weight )
{
    melee_miss_reasons.add( reason, weight );

}

void Character::clear_miss_reasons()
{
    melee_miss_reasons.clear();
}

std::string Character::get_miss_reason()
{
    // everything that lowers accuracy in player::hit_roll()
    // adding it in hit_roll() might not be safe if it's called multiple times
    // in one turn
    add_miss_reason(
        _( "Your torso encumbrance throws you off-balance." ),
        roll_remainder( avg_encumb_of_limb_type( body_part_type::type::torso ) / 10.0 ) );
    const int farsightedness = 2 * ( has_flag( json_flag_HYPEROPIC ) &&
                                     !worn_with_flag( flag_FIX_FARSIGHT ) &&
                                     !has_effect( effect_contacts ) &&
                                     !has_effect( effect_transition_contacts ) );
    add_miss_reason(
        _( "You can't hit reliably due to your farsightedness." ),
        farsightedness );
    add_miss_reason(
        _( "You struggle to hit reliably while on the ground." ),
        3 * is_on_ground() );

    const std::string *const reason = melee_miss_reasons.pick();
    if( reason == nullptr ) {
        return std::string();
    }
    return *reason;
}

void Character::roll_all_damage( bool crit, damage_instance &di, bool average,
                                 const item &weap, const std::string &attack_vector, const Creature *target,
                                 const bodypart_id &bp ) const
{
    float crit_mod = 1.f;
    if( target != nullptr ) {
        crit_mod = target->get_crit_factor( bp );
    }
    for( const damage_type &dt : damage_type::get_all() ) {
        roll_damage( dt.id, crit, di, average, weap, attack_vector, crit_mod );
    }
}

static void melee_train( Character &you, int lo, int hi, const item &weap,
                         const std::string &attack_vector )
{
    you.practice( skill_melee, std::ceil( rng( lo, hi ) / 2.0 ), hi );

    float total = 0.f;

    // allocate XP proportional to damage stats
    // Pure unarmed needs a special case because it has 0 weapon damage
    std::map<damage_type_id, int> dmg_vals;
    for( const damage_type &dt : damage_type::get_all() ) {
        if( !dt.melee_only ) {
            continue;
        }
        int dmg = weap.damage_melee( dt.id );
        if( weap.is_null() && dt.id == damage_bash ) {
            dmg++;
        }
        dmg_vals[dt.id] = dmg;
        total += dmg;
    }

    total = std::max( total, 1.f );

    // Unarmed may deal cut, stab, and bash damage depending on the weapon
    if( attack_vector != "WEAPON" ) {
        you.practice( skill_unarmed, std::ceil( 1 * rng( lo, hi ) ), hi );
    } else {
        for( const std::pair<const damage_type_id, int> &dmg : dmg_vals ) {
            if( !dmg.first->skill.is_null() ) {
                you.practice( dmg.first->skill, std::ceil( dmg.second / total * rng( lo, hi ) ), hi );
            }
        }
    }
}

bool Character::melee_attack( Creature &t, bool allow_special )
{
    static const matec_id no_technique_id( "" );
    return melee_attack( t, allow_special, no_technique_id );
}

damage_instance Creature::modify_damage_dealt_with_enchantments( const damage_instance &dam ) const
{
    return dam;
}

damage_instance Character::modify_damage_dealt_with_enchantments( const damage_instance &dam ) const
{
    damage_instance modified;

    std::vector<damage_type_id> types_used;

    auto dt_to_ench_dt = []( const damage_type_id & dt ) {
        if( dt == STATIC( damage_type_id( "acid" ) ) ) {
            return enchant_vals::mod::ITEM_DAMAGE_ACID;
        } else if( dt == STATIC( damage_type_id( "bash" ) ) ) {
            return enchant_vals::mod::ITEM_DAMAGE_BASH;
        } else if( dt == STATIC( damage_type_id( "biological" ) ) ) {
            return enchant_vals::mod::ITEM_DAMAGE_BIO;
        } else if( dt == STATIC( damage_type_id( "bullet" ) ) ) {
            return enchant_vals::mod::ITEM_DAMAGE_BULLET;
        } else if( dt == STATIC( damage_type_id( "cold" ) ) ) {
            return enchant_vals::mod::ITEM_DAMAGE_COLD;
        } else if( dt == STATIC( damage_type_id( "cut" ) ) ) {
            return enchant_vals::mod::ITEM_DAMAGE_CUT;
        } else if( dt == STATIC( damage_type_id( "electric" ) ) ) {
            return enchant_vals::mod::ITEM_DAMAGE_ELEC;
        } else if( dt == STATIC( damage_type_id( "heat" ) ) ) {
            return enchant_vals::mod::ITEM_DAMAGE_HEAT;
        } else if( dt == STATIC( damage_type_id( "stab" ) ) ) {
            return enchant_vals::mod::ITEM_DAMAGE_STAB;
        } else if( dt == STATIC( damage_type_id( "pure" ) ) ) {
            return enchant_vals::mod::ITEM_DAMAGE_PURE;
        }
        return enchant_vals::mod::NUM_MOD;
    };

    auto modify_damage_type = [&]( const damage_type_id & dt, double val ) {
        const enchant_vals::mod mod_type = dt_to_ench_dt( dt );
        if( mod_type == enchant_vals::mod::NUM_MOD ) {
            return val;
        } else {
            val = enchantment_cache->modify_value( dt_to_ench_dt( dt ), val );
        }
        return enchantment_cache->modify_value( enchant_vals::mod::MELEE_DAMAGE, val );
    };

    for( damage_unit du : dam ) {
        du.amount = modify_damage_type( du.type, du.amount );
        modified.add( du );
        types_used.emplace_back( du.type );
    }

    for( const damage_type &dt : damage_type::get_all() ) {
        if( std::find( types_used.begin(), types_used.end(), dt.id ) != types_used.end() ) {
            continue;
        }
        modified.add_damage( dt.id, modify_damage_type( dt.id, 0.0f ) );
        modified.add_damage( dt.id, enchantment_cache->modify_value( enchant_vals::mod::MELEE_DAMAGE,
                             0.0f ) );
    }

    return modified;
}

// Melee calculation is in parts. This sets up the attack, then in deal_melee_attack,
// we calculate if we would hit. In Creature::deal_melee_hit, we calculate if the target dodges.
bool Character::melee_attack( Creature &t, bool allow_special, const matec_id &force_technique,
                              bool allow_unarmed, int forced_movecost )
{
    if( has_effect( effect_incorporeal ) ) {
        add_msg_if_player( m_info, _( "You lack the substance to affect anything." ) );
        return false;
    }
    if( !is_adjacent( &t, true ) ) {
        return false;
    }

    // Max out recoil & reset aim point
    recoil = MAX_RECOIL;
    last_target_pos = std::nullopt;

    return melee_attack_abstract( t, allow_special, force_technique, allow_unarmed, forced_movecost );
}

static const std::set<weapon_category_id> &wielded_weapon_categories( const Character &c )
{
    static const std::set<weapon_category_id> unarmed{ weapon_category_UNARMED };
    if( c.get_wielded_item() ) {
        return c.get_wielded_item()->typeId()->weapon_category;
    }
    return unarmed;
}

bool Character::melee_attack_abstract( Creature &t, bool allow_special,
                                       const matec_id &force_technique,
                                       bool allow_unarmed, int forced_movecost )
{
    if( !enough_working_legs() ) {
        if( !movement_mode_is( move_mode_prone ) ) {
            add_msg_if_player( m_bad, _( "Your broken legs cannot hold you and you fall down." ) );
            set_movement_mode( move_mode_prone );
            return false;
        }
    }

    melee::melee_stats.attack_count += 1;
    int hit_spread = t.deal_melee_attack( this, hit_roll() );
    if( !t.is_avatar() ) {
        // TODO: Per-NPC tracking? Right now monster hit by either npc or player will draw aggro...
        t.add_effect( effect_hit_by_player, 10_minutes ); // Flag as attacked by us for AI
    }
    if( is_mounted() ) {
        auto *mons = mounted_creature.get();
        if( mons->has_flag( mon_flag_RIDEABLE_MECH ) ) {
            if( !mons->check_mech_powered() ) {
                add_msg( m_bad, _( "The %s has dead batteries and will not move its arms." ),
                         mons->get_name() );
                return false;
            }
            if( mons->type->has_special_attack( "SMASH" ) && one_in( 3 ) ) {
                add_msg( m_info, _( "The %s hisses as its hydraulic arm pumps forward!" ),
                         mons->get_name() );
                mattack::smash_specific( mons, &t );
            } else {
                mons->use_mech_power( 2_kJ );
                mons->melee_attack( t );
            }
            mod_moves( forced_movecost >= 0 ? -forced_movecost : -mons->type->attack_cost );
            return true;
        }
    }

    // Fighting is hard work
    set_activity_level( EXTRA_EXERCISE );

    item_location cur_weapon = allow_unarmed ? used_weapon() : get_wielded_item();
    item cur_weap = cur_weapon ? *cur_weapon : null_item_reference();

    int move_cost = attack_speed( cur_weap );

    if( cur_weap.attack_time( *this ) > move_cost * 20 ) {
        add_msg( m_bad, _( "This weapon is too unwieldy to attack with!" ) );
        return false;
    }

    if( is_avatar() && move_cost > 1000 && calendar::turn > melee_warning_turn ) {
        const std::string &action = query_popup()
                                    .context( "CANCEL_ACTIVITY_OR_IGNORE_QUERY" )
                                    .message( _( "<color_light_red>Attacking with your %1$s will take a long time.  "
                                              "Are you sure you want to continue?</color>" ),
                                              cur_weap.display_name() )
                                    .option( "YES" )
                                    .option( "NO" )
                                    .option( "IGNORE" )
                                    .query()
                                    .action;

        if( action == "NO" ) {
            return false;
        }
        if( action == "IGNORE" ) {
            if( melee_warning_turn == calendar::turn_zero || melee_warning_turn <= calendar::turn ) {
                melee_warning_turn = calendar::turn + 50_turns;
            }
        }
    }

    const bool hits = hit_spread >= 0;

    if( monster *m = t.as_monster() ) {
        cata::event e = cata::event::make<event_type::character_melee_attacks_monster>( getID(),
                        cur_weap.typeId(), hits, m->type->id );
        get_event_bus().send_with_talker( this, m, e );
    } else if( Character *c = t.as_character() ) {
        cata::event e = cata::event::make<event_type::character_melee_attacks_character>( getID(),
                        cur_weap.typeId(), hits, c->getID(), c->get_name() );
        get_event_bus().send_with_talker( this, c, e );
    }

    const int skill_training_cap = t.is_monster() ? t.as_monster()->type->melee_training_cap :
                                   MAX_SKILL;
    Character &player_character = get_player_character();
    if( !hits ) {
        int stumble_pen = stumble( *this, cur_weapon );
        sfx::generate_melee_sound( pos(), t.pos(), false, false );

        const ma_technique miss_recovery = martial_arts_data->get_miss_recovery( *this );

        if( is_avatar() ) { // Only display messages if this is the player

            if( one_in( 2 ) ) {
                const std::string reason_for_miss = get_miss_reason();
                if( !reason_for_miss.empty() ) {
                    add_msg( reason_for_miss );
                }
            }

            if( miss_recovery.id != tec_none ) {
                add_msg( miss_recovery.avatar_message.translated(), t.disp_name() );
            } else if( stumble_pen >= 60 ) {
                add_msg( m_bad, _( "You miss and stumble with the momentum." ) );
            } else if( stumble_pen >= 10 ) {
                add_msg( _( "You swing wildly and miss." ) );
            } else {
                add_msg( _( "You miss." ) );
            }
        } else if( player_character.sees( *this ) ) {
            if( miss_recovery.id != tec_none ) {
                add_msg_if_npc( miss_recovery.npc_message.translated(), t.disp_name() );
            } else if( stumble_pen >= 60 ) {
                add_msg( _( "%s misses and stumbles with the momentum." ), get_name() );
            } else if( stumble_pen >= 10 ) {
                add_msg( _( "%s swings wildly and misses." ), get_name() );
            } else {
                add_msg( _( "%s misses." ), get_name() );
            }
        }

        // Practice melee and relevant weapon skill (if any) except when using CQB bionic
        if( !has_active_bionic( bio_cqb ) && !t.is_hallucination() ) {
            std::string attack_vector = cur_weapon ? "WEAPON" : "HAND";
            melee_train( *this, 2, std::min( 5, skill_training_cap ), cur_weap, attack_vector );
        }

        // Cap stumble penalty, heavy weapons are quite weak already
        move_cost += std::min( 60, stumble_pen );
        if( miss_recovery.id != tec_none ) {
            move_cost /= 2;
        }

        // trigger martial arts on-miss effects
        martial_arts_data->ma_onmiss_effects( *this );
    } else {
        melee::melee_stats.hit_count += 1;
        // Remember if we see the monster at start - it may change
        const bool seen = player_character.sees( t );
        // Start of attacks.
        const bool critical_hit = scored_crit( t.dodge_roll(), cur_weap );
        if( critical_hit ) {
            melee::melee_stats.actual_crit_count += 1;
        }
        // select target body part
        const bodypart_id &target_bp = t.select_body_part( -1, -1, can_attack_high(),
                                       hit_spread );

        const bool has_force_technique = !force_technique.str().empty();

        // Pick one or more special attacks
        matec_id technique_id;
        if( has_force_technique ) {
            technique_id = force_technique;
        } else if( allow_special ) {
            technique_id = pick_technique( t, cur_weapon, critical_hit, false, false );
            if( critical_hit && technique_id.obj().crit_tec_id != tec_none ) {
                technique_id = technique_id.obj().crit_tec_id;
            }
        } else {
            technique_id = tec_none;
        }

        std::string attack_vector;

        // Failsafe for tec_none
        if( technique_id == tec_none ) {
            attack_vector = cur_weapon ? "WEAPON" : "HAND";
        } else {
            attack_vector = martial_arts_data->get_valid_attack_vector( *this,
                            technique_id.obj().attack_vectors );

            if( attack_vector == "NONE" ) {
                std::vector<std::string> shuffled_attack_vectors = technique_id.obj().attack_vectors_random;
                std::shuffle( shuffled_attack_vectors.begin(), shuffled_attack_vectors.end(), rng_get_engine() );
                attack_vector = martial_arts_data->get_valid_attack_vector( *this, shuffled_attack_vectors );
            }
        }

        // If no weapon is selected, use highest layer of clothing for attack vector instead.
        if( attack_vector != "WEAPON" ) {
            // todo: simplify this by using item_location everywhere
            // so only cur_weapon = worn.current_unarmed_weapon remains
            item *worn_weap = worn.current_unarmed_weapon( attack_vector );
            cur_weapon = worn_weap ? item_location( *this, worn_weap ) : item_location();
            cur_weap = cur_weapon ? *cur_weapon : null_item_reference();
        }

        damage_instance d;
        roll_all_damage( critical_hit, d, false, cur_weap, attack_vector, &t, target_bp );

        // your hits are not going to hurt very much if you can't use martial arts due to broken limbs
        if( attack_vector == "HAND" && get_working_arm_count() < 1 ) {
            technique_id = tec_none;
            d.mult_damage( 0.1 );
            add_msg_if_player( m_bad, _( "Your arms are too damaged or encumbered to fight effectively!" ) );
        }
        // polearms and pikes (but not spears) do less damage to adjacent targets
        // In the case of a weapon like a glaive or a naginata, the wielder
        // lacks the room to build up momentum on a slash.
        // In the case of a pike, the mass of the pole behind the wielder
        // should they choose to employ it up close will unbalance them.
        if( cur_weap.reach_range( *this ) > 1 && !reach_attacking &&
            cur_weap.has_flag( flag_POLEARM ) ) {
            d.mult_damage( 0.7 );
        }
        // being prone affects how much leverage you can use to deal damage
        // quadrupeds don't mind as much, tentacles and goo-limbs even less
        if( is_on_ground() )  {
            if( has_flag( json_flag_PSEUDOPOD_GRASP ) ) {
                d.mult_damage( 0.8 );
            }
            d.mult_damage( 0.3 );
        } else if( is_crouching() && ( !has_effect( effect_natural_stance ) && !unarmed_attack() ) &&
                   !has_flag( json_flag_PSEUDOPOD_GRASP ) ) {
            d.mult_damage( 0.8 );
        }

        const ma_technique &technique = technique_id.obj();

        // Handles effects as well; not done in melee_affect_*
        if( technique.id != tec_none ) {
            perform_technique( technique, t, d, move_cost, cur_weapon );
        }

        //player has a very small chance, based on their intelligence, to learn a style whilst using the CQB bionic
        if( has_active_bionic( bio_cqb ) && !martial_arts_data->knows_selected_style() ) {
            /** @EFFECT_INT slightly increases chance to learn techniques when using CQB bionic */
            // Enhanced Memory Banks bionic doubles chance to learn martial art
            const int learn_boost = has_flag( json_flag_CBQ_LEARN_BONUS ) ? 2 : 1;
            if( one_in( ( 1400 - ( get_int() * 50 ) ) / learn_boost ) ) {
                martial_arts_data->learn_current_style_CQB( is_avatar() );
            }
        }

        // Proceed with melee attack.
        if( !t.is_dead_state() ) {

            std::string specialmsg;
            // Handles speed penalties to monster & us, etc
            if( !t.is_hallucination() ) {
                if( technique.attack_override ) {
                    specialmsg = melee_special_effects( t, d, null_item_reference() );
                } else {
                    specialmsg = melee_special_effects( t, d, cur_weap );
                }
            }

            // gets overwritten with the dealt damage values
            dealt_damage_instance dealt_dam;
            dealt_damage_instance dealt_special_dam;
            if( allow_special ) {
                perform_special_attacks( t, dealt_special_dam );
            }
            weakpoint_attack attack;
            attack.weapon = &cur_weap;
            t.deal_melee_hit( this, hit_spread, critical_hit, d, dealt_dam, attack, &target_bp );

            bool has_edged_damage = false;
            for( const damage_type &dt : damage_type::get_all() ) {
                if( dt.melee_only && dt.edged && dealt_special_dam.type_damage( dt.id ) > 0 ) {
                    has_edged_damage = true;
                    break;
                }
            }

            if( has_edged_damage || ( !cur_weapon && has_edged_damage ) ) {
                if( has_trait( trait_POISONOUS ) ) {
                    if( t.is_monster() ) {
                        t.add_effect( effect_venom_player1, 1_minutes );
                    } else {
                        t.add_effect( effect_venom_dmg, 10_minutes );
                    }
                    if( t.is_immune_effect( effect_venom_player1 ) ) {
                        add_msg_if_player( m_bad, _( "The %s is not affected by your venom" ), t.disp_name() );
                    } else {
                        add_msg_if_player( m_good, _( "You poison %s!" ), t.disp_name() );
                        if( x_in_y( 1, 10 ) ) {
                            t.add_effect( effect_stunned, 1_turns );
                        }
                    }
                } else if( has_trait( trait_POISONOUS2 ) ) {
                    if( t.is_monster() ) {
                        t.add_effect( effect_venom_player2, 1_minutes );
                    } else {
                        t.add_effect( effect_venom_dmg, 15_minutes );
                        t.add_effect( effect_venom_weaken, 5_minutes );
                    }
                    if( t.is_immune_effect( effect_venom_player2 ) ) {
                        add_msg_if_player( m_bad, _( "The %s is not affected by your venom" ), t.disp_name() );
                    } else {
                        add_msg_if_player( m_good, _( "You inject your venom into %s!" ), t.disp_name() );
                        if( x_in_y( 1, 4 ) ) {
                            t.add_effect( effect_stunned, 1_turns );
                        }
                    }
                }
            }
            // Make a rather quiet sound, to alert any nearby monsters
            if( !is_quiet() ) { // check martial arts silence
                //sound generated later
                int volume = enchantment_cache->modify_value( enchant_vals::mod::ATTACK_NOISE, 8 );
                sounds::sound( pos(), volume, sounds::sound_t::combat, _( "whack!" ) );
            }
            std::string material = "flesh";
            if( t.is_monster() ) {
                const monster *m = dynamic_cast<const monster *>( &t );
                if( m->made_of( material_steel ) ) {
                    material = "steel";
                }
            }
            sfx::generate_melee_sound( pos(), t.pos(), true, t.is_monster(), material );
            int dam = dealt_dam.total_damage();
            melee::melee_stats.damage_amount += dam;

            // Practice melee and relevant weapon skill (if any) except when using CQB bionic
            if( !has_active_bionic( bio_cqb ) && !t.is_hallucination() ) {
                melee_train( *this, 5, std::min( 10, skill_training_cap ), cur_weap, attack_vector );
            }

            // Treat monster as seen if we see it before or after the attack
            if( seen || player_character.sees( t ) ) {
                std::string message = melee_message( technique, *this, dealt_dam );
                player_hit_message( this, message, t, dam, critical_hit, technique.id != tec_none,
                                    dealt_dam.wp_hit );
            } else {
                add_msg_player_or_npc( m_good, _( "You hit something." ),
                                       _( "<npcname> hits something." ) );
            }

            if( !specialmsg.empty() ) {
                add_msg_if_player( m_neutral, specialmsg );
            }

            if( critical_hit ) {
                // trigger martial arts on-crit effects
                martial_arts_data->ma_oncrit_effects( *this );
            }

        }

        t.check_dead_state();

        if( t.is_dead_state() ) {
            // trigger martial arts on-kill effects
            martial_arts_data->ma_onkill_effects( *this );
        }
    }

    if( !t.is_hallucination() ) {
        handle_melee_wear( cur_weapon );
    }

    /** @EFFECT_MELEE reduces stamina cost of melee attacks */
    const int deft_bonus = !hits && has_trait( trait_DEFT ) ? 50 : 0;
    const int base_stam = get_base_melee_stamina_cost();
    const int total_stam = enchantment_cache->modify_value(
                               enchant_vals::mod::MELEE_STAMINA_CONSUMPTION,
                               get_total_melee_stamina_cost() );

    // Train weapon proficiencies
    for( const weapon_category_id &cat : wielded_weapon_categories( *this ) ) {
        for( const proficiency_id &prof : cat->category_proficiencies() ) {
            practice_proficiency( prof, 1_seconds );
        }
    }

    burn_energy_arms( std::min( -50, total_stam + deft_bonus ) );
    add_msg_debug( debugmode::DF_MELEE, "Stamina burn base/total (capped at -50): %d/%d", base_stam,
                   total_stam + deft_bonus );
    // Weariness handling - 1 / the value, because it returns what % of the normal speed
    const float weary_mult = exertion_adjusted_move_multiplier( EXTRA_EXERCISE );
    mod_moves( forced_movecost >= 0 ? -forced_movecost : -move_cost * ( 1 / weary_mult ) );
    // trigger martial arts on-attack effects
    martial_arts_data->ma_onattack_effects( *this );
    // some things (shattering weapons) can harm the attacking creature.
    check_dead_state();
    did_hit( t );
    if( t.as_character() ) {
        dealt_projectile_attack dp = dealt_projectile_attack();
        t.as_character()->on_hit( this, bodypart_id( "bp_null" ), 0.0f, &dp );
    }
    return true;
}

int Character::get_base_melee_stamina_cost( const item *weap ) const
{
    return std::min( -50, get_standard_stamina_cost( weap ) );
}

int Character::get_total_melee_stamina_cost( const item *weap ) const
{
    const int mod_sta = get_standard_stamina_cost( weap );
    const int melee = round( get_skill_level( skill_melee ) );
    // Quadrupeds don't mind crouching, squids and slimes hardly care about even being prone
    const int stance_malus = ( is_on_ground() &&
                               !has_flag( json_flag_PSEUDOPOD_GRASP ) ) ? 50 : ( !has_flag( json_flag_PSEUDOPOD_GRASP ) &&
                                       ( !has_effect( effect_natural_stance ) && ( !unarmed_attack() ) ) && is_crouching() ? 20 : 0 );

    float proficiency_multiplier = 1.f;
    for( const weapon_category_id &cat : wielded_weapon_categories( *this ) ) {
        float loss = 0.f;
        for( const proficiency_id &prof : cat->category_proficiencies() ) {
            if( !has_proficiency( prof ) ) {
                continue;
            }
            std::optional<float> bonus = prof->bonus_for( "melee_attack", proficiency_bonus_type::stamina );
            if( !bonus.has_value() ) {
                continue;
            }
            loss += bonus.value();
        }
        proficiency_multiplier = std::clamp( 1.f - loss, 0.f, proficiency_multiplier );
    }

    return std::min<int>( -50, proficiency_multiplier * ( mod_sta + melee - stance_malus ) );
}

void Character::reach_attack( const tripoint &p, int forced_movecost )
{
    static const matec_id no_technique_id( "" );
    matec_id force_technique = no_technique_id;
    /** @EFFECT_MELEE >5 allows WHIP_DISARM technique */
    if( weapon.has_flag( flag_WHIP ) && ( get_skill_level( skill_melee ) > 5 ) && one_in( 3 ) ) {
        force_technique = WHIP_DISARM;
    }

    // Fighting is hard work
    set_activity_level( EXTRA_EXERCISE );

    creature_tracker &creatures = get_creature_tracker();
    Creature *critter = creatures.creature_at( p );
    // Original target size, used when there are monsters in front of our target
    const int target_size = critter != nullptr ? static_cast<int>( critter->get_size() ) : 2;
    // Reset last target pos
    last_target_pos = std::nullopt;
    // Max out recoil
    recoil = MAX_RECOIL;

    // Weariness handling
    // 1 / mult because mult is the percent penalty, in the form 1.0 == 100%
    const float weary_mult = 1.0f / exertion_adjusted_move_multiplier( EXTRA_EXERCISE );
    int move_cost = attack_speed( weapon ) * weary_mult;
    float skill = std::min( 10.0f, get_skill_level( skill_melee ) );
    int t = 0;
    map &here = get_map();
    std::vector<tripoint> path = line_to( pos(), p, t, 0 );
    path.pop_back(); // Last point is our critter
    for( const tripoint &path_point : path ) {
        // Possibly hit some unintended target instead
        Creature *inter = creatures.creature_at( path_point );
        /** @EFFECT_MELEE decreases chance of hitting intervening target on reach attack */
        if( inter != nullptr &&
            !x_in_y( ( target_size * target_size + 1 ) * skill,
                     ( inter->get_size() * inter->get_size() + 1 ) * 10 ) ) {
            // Even if we miss here, low roll means weapon is pushed away or something like that
            if( inter->has_effect( effect_pet ) || ( inter->is_npc() &&
                    inter->as_npc()->is_friendly( get_player_character() ) ) ) {
                if( query_yn( _( "Your attack may cause accidental injury, continue?" ) ) ) {
                    critter = inter;
                    break;
                } else {
                    return;
                }
            }
            critter = inter;
            break;
        } else if( here.impassable( path_point ) &&
                   // Fences etc. Spears can stab through those
                   !( weapon.has_flag( flag_SPEAR ) &&
                      here.has_flag( ter_furn_flag::TFLAG_THIN_OBSTACLE, path_point ) ) ) {
            /** @ARM_STR increases bash effects when reach attacking past something */
            here.bash( path_point, get_arm_str() + weapon.damage_melee( damage_bash ) );
            handle_melee_wear( get_wielded_item() );
            mod_moves( forced_movecost >= 0 ? -forced_movecost : -move_cost );
            return;
        }
    }

    if( critter == nullptr ) {
        add_msg_if_player( _( "You swing at the air." ) );

        const ma_technique miss_recovery = martial_arts_data->get_miss_recovery( *this );

        if( miss_recovery.id != tec_none ) {
            move_cost /= 3; // "Probing" is faster than a regular miss
            // Communicate this with a different message?
        }

        mod_moves( forced_movecost >= 0 ? -forced_movecost : -move_cost );
        return;
    }

    reach_attacking = true;
    melee_attack_abstract( *critter, true, force_technique, false, forced_movecost );
    reach_attacking = false;
}

int stumble( Character &u, const item_location &weap )
{
    if( !weap || u.has_trait( trait_DEFT ) ) {
        return 0;
    }
    item cur_weap = weap ? *weap : null_item_reference();
    units::mass str_mod = u.get_arm_str() * 10_gram;
    // Ceph and Slime mutants still need good posture to prevent stumbling
    if( u.is_on_ground() ) {
        str_mod /= 4;
        // but quadrupeds fight naturally on all fours
    } else if( u.is_crouching() && ( !u.has_effect( effect_natural_stance ) && !u.unarmed_attack() ) ) {
        str_mod /= 2;
    }

    // Examples:
    // 10 str with a hatchet: 4 + 8 = 12
    // 5 str with a battle axe: 26 + 49 = 75
    // Fist: 0

    /** @EFFECT_STR reduces chance of stumbling with heavier weapons */
    return ( weap->volume() / 125_ml ) +
           ( weap->weight() / ( str_mod + 13.0_gram ) );
}

bool Character::scored_crit( float target_dodge, const item &weap ) const
{
    return rng_float( 0, 1.0 ) < crit_chance( hit_roll(), target_dodge, weap );
}

/**
 * Limits a probability to be between 0.0 and 1.0
 */
static double limit_probability( double unbounded_probability )
{
    return std::max( std::min( unbounded_probability, 1.0 ), 0.0 );
}

double Character::crit_chance( float roll_hit, float target_dodge, const item &weap ) const
{
    // Martial arts buff bonuses
    double ma_buff_crit_chance = mabuff_critical_hit_chance_bonus() / 100;

    // Weapon to-hit roll
    double weapon_crit_chance = 0.5;
    if( weap.is_null() ) {
        // Unarmed attack: 1/2 of unarmed skill is to-hit
        /** @EFFECT_UNARMED increases critical chance */
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
    float sk = get_skill_level( weap.melee_skill() );
    if( has_active_bionic( bio_cqb ) ) {
        sk = std::max( sk, static_cast<float>( BIO_CQB_LEVEL ) );
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
    // Only check double critical (one that requires hit/dodge comparison) if we have good
    // hit vs dodge
    if( roll_hit > target_dodge * 3 / 2 ) {
        const double chance_double = 0.5 * (
                                         weapon_crit_chance * stat_crit_chance +
                                         stat_crit_chance * skill_crit_chance +
                                         weapon_crit_chance * skill_crit_chance -
                                         ( 3 * chance_triple ) );
        // Because chance_double already removed the triples with -( 3 * chance_triple ),
        // chance_triple and chance_double are mutually exclusive probabilities and can just
        // be added together.
        melee::melee_stats.double_crit_count += 1;
        melee::melee_stats.double_crit_chance += chance_double + chance_triple + ma_buff_crit_chance;
        return chance_triple + chance_double + ma_buff_crit_chance;
    }
    melee::melee_stats.crit_count += 1;
    melee::melee_stats.crit_chance += chance_triple + ma_buff_crit_chance;
    return chance_triple + ma_buff_crit_chance;
}

int Character::get_spell_resist() const
{
    return round( get_skill_level( skill_spellcraft ) );
}

float Character::get_dodge() const
{
    if( !can_try_doge().success() ) {
        return 0.0f;
    }

    float ret = Creature::get_dodge();
    add_msg_debug( debugmode::DF_MELEE, "Base dodge %.1f", ret );

    // Chop in half if we are unable to move
    if( has_effect( effect_beartrap ) || has_effect( effect_lightsnare ) ||
        has_effect( effect_heavysnare ) ) {
        ret /= 2;
        add_msg_debug( debugmode::DF_MELEE, "Dodge after trapped penalty %.1f", ret );
    }

    if( worn_with_flag( flag_ROLLER_INLINE ) ||
        worn_with_flag( flag_ROLLER_QUAD ) ||
        worn_with_flag( flag_ROLLER_ONE ) ) {
        ret /= has_trait( trait_PROF_SKATER ) ? 2 : 5;
        add_msg_debug( debugmode::DF_MELEE, "Dodge after skate penalty %.1f", ret );
    }

    // Speed below 100 linearly decreases dodge effectiveness
    int speed_stat = get_speed();
    if( speed_stat < 100 ) {
        ret *= speed_stat / 100.0f;
        add_msg_debug( debugmode::DF_MELEE, "Dodge after speed penalty %.1f", ret );
    }

    //Dodge decreases logisticaly with stamina.
    const double stamina_logistic = get_stamina_dodge_modifier();
    ret *= stamina_logistic;

    add_msg_debug( debugmode::DF_MELEE, "Dodge after stamina penalty %.1f", ret );

    // Reaction score of limbs influences dodge chances
    ret *= get_limb_score( limb_score_reaction );
    add_msg_debug( debugmode::DF_MELEE, "Dodge after reaction score %.1f", ret );

    // Somatic limb dodge multiplier is applied after reaction score
    ret *= get_modifier( character_modifier_limb_dodge_mod );
    add_msg_debug( debugmode::DF_MELEE, "Dodge after limb score modifier %.1f", ret );

    // Modify by how much bigger/smaller we got from our limbs
    ret /= anatomy( get_all_body_parts() ).get_size_ratio( anatomy_human_anatomy );
    add_msg_debug( debugmode::DF_MELEE, "Dodge after bodysize modifier %.1f", ret );

    return std::max( 0.0f, ret );
}

float Character::dodge_roll() const
{
    // if your character has evasion then try rolling that first
    double evasion = enchantment_cache->modify_value( enchant_vals::mod::EVASION, 0.0 );
    if( rng( 0, 99 ) < evasion * 100.0 ) {
        // arbitrarily high number without being max float
        return 999999.0f;
    }

    if( has_flag( json_flag_HARDTOHIT ) ) {
        // two chances at rng!
        return std::max( get_dodge(), get_dodge() ) * 5;
    }
    return get_dodge() * 5;
}

float Character::bonus_damage( bool random ) const
{
    /** @ARM_STR increases bashing damage */
    if( random ) {
        return rng_float( get_arm_str() / 2.0f, get_arm_str() );
    }

    return get_arm_str() * 0.75f;
}

static void roll_melee_damage_internal( const Character &u, const damage_type_id &dt, bool crit,
                                        damage_instance &di, bool average, const item &weap,
                                        const std::string &attack_vector, float crit_mod )
{
    // FIXME: Hardcoded damage type
    float dmg = dt == damage_bash ? 0.f : u.mabuff_damage_bonus( dt ) + weap.damage_melee( dt );
    bool unarmed = attack_vector != "WEAPON";
    int arpen = 0;

    float skill = u.get_skill_level( unarmed ? skill_unarmed : dt->skill );

    if( u.has_active_bionic( bio_cqb ) ) {
        skill = BIO_CQB_LEVEL;
    }

    // FIXME: Hardcoded damage type effects (bash)
    if( dt == damage_bash ) {
        if( u.has_trait( trait_KI_STRIKE ) && unarmed ) {
            // Pure unarmed doubles the bonuses from unarmed skill
            skill *= 2;
        }

        // Drunken Master damage bonuses
        if( u.has_trait( trait_DRUNKEN ) && u.has_effect( effect_drunk ) ) {
            // Remember, a single drink gives 600 levels of "drunk"
            int mindrunk = 0;
            int maxdrunk = 0;
            const time_duration drunk_dur = u.get_effect_dur( effect_drunk );
            if( unarmed ) {
                mindrunk = drunk_dur / 1_hours;
                maxdrunk = drunk_dur / 25_minutes;
            } else {
                mindrunk = drunk_dur / 90_minutes;
                maxdrunk = drunk_dur / 40_minutes;
            }

            dmg += average ? ( mindrunk + maxdrunk ) * 0.5f : rng( mindrunk, maxdrunk );
        }
    }

    if( unarmed ) {
        bool bp_unrestricted;

        if( attack_vector == "ARM" ) {
            bp_unrestricted = !u.natural_attack_restricted_on( bodypart_id( "arm_l" ) ) ||
                              ( !u.natural_attack_restricted_on( bodypart_id( "arm_r" ) ) );
        } else if( attack_vector == "ELBOW" ) {
            bp_unrestricted = !u.natural_attack_restricted_on( sub_bodypart_id( "arm_elbow_l" ) ) ||
                              ( !u.natural_attack_restricted_on( sub_bodypart_id( "arm_elbow_r" ) ) );
        } else if( attack_vector == "WRIST" ) {
            bp_unrestricted = !u.natural_attack_restricted_on( sub_bodypart_id( "hand_wrist_l" ) ) ||
                              ( !u.natural_attack_restricted_on( sub_bodypart_id( "hand_wrist_r" ) ) );
        } else if( attack_vector == "SHOULDER" ) {
            bp_unrestricted = !u.natural_attack_restricted_on( sub_bodypart_id( "arm_shoulder_l" ) ) ||
                              ( !u.natural_attack_restricted_on( sub_bodypart_id( "arm_shoulder_r" ) ) );
        } else if( attack_vector == "FOOT" ) {
            bp_unrestricted = !u.natural_attack_restricted_on( bodypart_id( "foot_l" ) ) ||
                              ( !u.natural_attack_restricted_on( bodypart_id( "foot_r" ) ) );
        } else if( attack_vector == "LOWER_LEG" ) {
            bp_unrestricted = !u.natural_attack_restricted_on( sub_bodypart_id( "leg_lower_l" ) ) ||
                              ( !u.natural_attack_restricted_on( sub_bodypart_id( "leg_lower_r" ) ) );
        } else if( attack_vector == "KNEE" ) {
            bp_unrestricted = !u.natural_attack_restricted_on( sub_bodypart_id( "leg_knee_l" ) ) ||
                              ( !u.natural_attack_restricted_on( sub_bodypart_id( "leg_knee_r" ) ) );
        } else if( attack_vector == "HIP" ) {
            bp_unrestricted = !u.natural_attack_restricted_on( sub_bodypart_id( "leg_hip_l" ) ) ||
                              ( !u.natural_attack_restricted_on( sub_bodypart_id( "leg_hip_r" ) ) );
        } else if( attack_vector == "HEAD" ) {
            bp_unrestricted = !u.natural_attack_restricted_on( bodypart_id( "head" ) );
        } else if( attack_vector == "MOUTH" ) {
            bp_unrestricted = !u.natural_attack_restricted_on( bodypart_id( "mouth" ) );
        } else if( attack_vector == "TORSO" ) {
            bp_unrestricted = !u.natural_attack_restricted_on( bodypart_id( "torso" ) );
        } else {
            bp_unrestricted = !u.natural_attack_restricted_on( bodypart_id( "hand_l" ) ) ||
                              ( !u.natural_attack_restricted_on( bodypart_id( "hand_r" ) ) && weap.is_null() );
        }

        if( bp_unrestricted ) {
            float extra_damage = 0.0f;
            for( const trait_id &mut : u.get_mutations() ) {
                if( mut->flags.count( json_flag_NEED_ACTIVE_TO_MELEE ) > 0 && !u.has_active_mutation( mut ) ) {
                    continue;
                }
                float unarmed_bonus = 0.0f;
                int bonus_dmg = 0;
                std::pair<int, int> bonus_rand = { 0, 0 };
                if( mut->flags.count( json_flag_UNARMED_BONUS ) > 0 && bonus_dmg > 0 ) {
                    unarmed_bonus += std::min( u.get_skill_level( skill_unarmed ) / 2, 4.0f );
                }
                extra_damage += bonus_dmg + unarmed_bonus;
                extra_damage += average ? ( bonus_rand.first + bonus_rand.second ) / 2.0f :
                                rng( bonus_rand.first, bonus_rand.second );
            }

            // FIXME: Hardcoded damage type effect (stab)
            if( attack_vector == "HAND" && dt == damage_stab && u.has_bionic( bio_razors ) ) {
                extra_damage += 2;
            }

            dmg += extra_damage;
        }

        float dam = 0.0f;
        float ap = 0.0f;
        for( const bodypart_id &bp : u.get_all_body_parts() ) {
            if( bp->unarmed_bonus && !u.natural_attack_restricted_on( bp ) ) {
                dam += bp->unarmed_damage( dt );
                ap += bp->unarmed_arpen( dt );
            }
        }
        dmg += dam;
        arpen += ap;

    }

    /** @ARM_STR increases bashing damage */
    float stat_bonus = u.bonus_damage( !average );
    stat_bonus += u.mabuff_damage_bonus( dt );
    /** @EFFECT_STR increases bashing damage */
    float weap_dam = weap.damage_melee( dt ) + stat_bonus;
    /** @EFFECT_BASHING caps bash damage with bashing weapons */
    float bash_cap = 2 * u.get_arm_str() + 2 * skill;

    // FIXME: Hardcoded damage type effects (bash)
    if( dt != damage_bash && dmg <= 0 ) {
        return; // No negative damage!
    } else if( dt == damage_bash ) {
        float melee_bonus = u.get_skill_level( skill_melee );

        /** @EFFECT_UNARMED caps bash damage with unarmed weapons */
        if( u.is_melee_bash_damage_cap_bonus() ) {
            bash_cap += melee_bonus;
        }

        if( u.has_trait( trait_KI_STRIKE ) && unarmed ) {
            /** @EFFECT_UNARMED increases bashing damage with unarmed weapons when paired with the Ki Strike trait */
            weap_dam += skill;
        }
    }

    float dmg_mul = 1.0f;
    // FIXME: Hardcoded damage type effects (stab)
    if( dt == damage_stab ) {
        // 66%, 76%, 86%, 96%, 106%, 116%, 122%, 128%, 134%, 140%
        /** @EFFECT_STABBING increases stabbing damage multiplier */
        if( skill <= 5 ) {
            dmg_mul = 0.66 + 0.1 * skill;
        } else {
            dmg_mul = 0.86 + 0.06 * skill;
        }
    } else {
        // 80%, 88%, 96%, 104%, 112%, 116%, 120%, 124%, 128%, 132%
        if( skill < 5 ) {
            dmg_mul *= 0.8 + 0.08 * skill;
        } else {
            dmg_mul *= 0.96 + 0.04 * skill;
        }
    }

    // FIXME: Hardcoded damage type effects (bash)
    if( dt == damage_bash ) {
        if( bash_cap < weap_dam && !weap.is_null() ) {
            // If damage goes over cap due to low stats/skills,
            // scale the post-armor damage down halfway between damage and cap
            dmg_mul *= ( 1.0f + ( bash_cap / weap_dam ) ) / 2.0f;
        }

        /** @ARM_STR boosts low cap on bashing damage */
        const float low_cap = std::min( 1.0f, u.get_arm_str() / 20.0f );
        const float bash_min = low_cap * weap_dam;
        weap_dam = average ? ( bash_min + weap_dam ) * 0.5f : rng_float( bash_min, weap_dam );
        dmg += weap_dam;
    }

    dmg_mul *= u.mabuff_damage_mult( dt );
    arpen += u.mabuff_arpen_bonus( dt );

    float armor_mult = 1.0f;
    if( crit ) {
        // FIXME: Hardcoded damage type effects (stab, cut, bash)
        if( dt == damage_stab ) {
            // Critical damage bonus for stabbing scales with skill
            dmg_mul *= 1.0 + ( skill / 10.0 ) * crit_mod;
            // Stab criticals have extra %arpen
            armor_mult = 1.f - 0.34f * crit_mod;
        } else if( dt == damage_cut ) {
            dmg_mul *= 1.f + 0.25f * crit_mod;
            arpen += static_cast<int>( 5.f * crit_mod );
            // 25% armor penetration
            armor_mult = 1.f - 0.25f * crit_mod;
        } else if( dt == damage_bash ) {
            dmg_mul *= 1.f + 0.5f * crit_mod;
            // 50% armor penetration
            armor_mult = 0.5f * crit_mod;
        }
    }

    di.add_damage( dt, dmg, arpen, armor_mult, dmg_mul );
}

void Character::roll_damage( const damage_type_id &dt, bool crit, damage_instance &di, bool average,
                             const item &weap, const std::string &attack_vector, float crit_mod ) const
{
    // For handling typical melee damage types (bash, cut, stab)
    if( dt->melee_only ) {
        roll_melee_damage_internal( *this, dt, crit, di, average, weap, attack_vector, crit_mod );
        return;
    }

    bool unarmed = attack_vector != "WEAPON";

    float other_dam = mabuff_damage_bonus( dt ) + weap.damage_melee( dt );
    float arpen = 0.0f;
    if( unarmed ) {
        float dam = 0.0f;
        float ap = 0.0f;
        for( const bodypart_id &bp : get_all_body_parts() ) {
            if( bp->unarmed_bonus && !natural_attack_restricted_on( bp ) ) {
                dam += bp->unarmed_damage( dt );
                arpen += bp->unarmed_arpen( dt );
            }
        }
        other_dam += dam;
        arpen += ap;
    }

    // No negative damage!
    if( other_dam > 0 ) {
        float other_mul = 1.0f * mabuff_damage_mult( dt );
        float armor_mult = 1.0f;

        di.add_damage( dt, other_dam, arpen, armor_mult, other_mul );
    }
}
matec_id Character::pick_technique( Creature &t, const item_location &weap, bool crit,
                                    bool dodge_counter, bool block_counter, const std::vector<matec_id> &blacklist )
{
    std::vector<matec_id> possible = evaluate_techniques( t, weap, crit,
                                     dodge_counter,  block_counter, blacklist );
    return random_entry( possible, tec_none );
}
std::vector<matec_id> Character::evaluate_techniques( Creature &t, const item_location &weap,
        bool crit, bool dodge_counter, bool block_counter, const std::vector<matec_id> &blacklist )
{

    const std::vector<matec_id> all = martial_arts_data->get_all_techniques( weap, *this );

    std::vector<matec_id> possible;

    bool wall_adjacent = get_map().is_wall_adjacent( pos() );
    // this could be more robust but for now it should work fine
    bool is_loaded = weap && weap->is_magazine_full();

    // first add non-aoe tecs
    for( const matec_id &tec_id : all ) {
        const ma_technique &tec = tec_id.obj();
        add_msg_debug( debugmode::DF_MELEE, "Evaluating technique %s", tec.name );

        // skip techniques on the blacklist
        if( find( blacklist.begin(), blacklist.end(), tec_id ) != blacklist.end() ) {
            add_msg_debug( debugmode::DF_MELEE, "Technique is on the blacklist, discarded" );
            continue;
        }

        // ignore "dummy" techniques like WBLOCK_1
        if( tec.dummy ) {
            add_msg_debug( debugmode::DF_MELEE, "Dummy technique, attack discarded" );
            continue;
        }

        // skip defensive techniques
        if( tec.defensive ) {
            add_msg_debug( debugmode::DF_MELEE, "Defensive technique, attack discarded" );
            continue;
        }

        // Ignore this technique if we fail the doalog conditions
        if( tec.has_condition ) {
            dialogue d( get_talker_for( this ), get_talker_for( t ) );
            if( !tec.condition( d ) ) {
                add_msg_debug( debugmode::DF_MELEE, "Conditionas failed, attack discarded" );
                continue;
            }
        }

        // skip wall adjacent techniques if not next to a wall
        if( tec.wall_adjacent && !wall_adjacent ) {
            add_msg_debug( debugmode::DF_MELEE, "No adjacent walls found, attack discarded" );
            continue;
        }

        // skip non reach ok techniques if reach attacking
        if( !( tec.reach_ok || tec.reach_tec ) && reach_attacking ) {
            add_msg_debug( debugmode::DF_MELEE, "Not usable with reach attack, attack discarded" );
            continue;
        }

        // skip reach techniques if not reach attacking
        if( tec.reach_tec && !reach_attacking ) {
            add_msg_debug( debugmode::DF_MELEE, "Only usable with reach attack, attack discarded" );
            continue;
        }

        // skip dodge counter techniques if it's not a dodge count, and vice versa
        if( dodge_counter != tec.dodge_counter ) {
            add_msg_debug( debugmode::DF_MELEE, "Not a dodge counter, attack discarded" );
            continue;
        }
        // likewise for block counters
        if( block_counter != tec.block_counter ) {
            add_msg_debug( debugmode::DF_MELEE, "Not a block counter, attack discarded" );
            continue;
        }

        // Don't counter if it would exhaust moves.
        if( tec.block_counter || tec.dodge_counter ) {
            item &used_weap = used_weapon() ? *used_weapon() : null_item_reference();
            float move_cost = attack_speed( used_weap );
            move_cost *= tec.move_cost_multiplier( *this );
            move_cost += tec.move_cost_penalty( *this );
            float move_mult = exertion_adjusted_move_multiplier( EXTRA_EXERCISE );
            move_cost *= ( 1.0f / move_mult );
            if( get_moves() + get_speed() - move_cost < 0 ) {
                add_msg_debug( debugmode::DF_MELEE,
                               "Counter technique would exhaust remaining moves, attack discarded" );
                continue;
            }
        }

        // if critical then select only from critical tecs
        // but allow the technique if its crit ok
        if( !tec.crit_ok && ( crit != tec.crit_tec ) ) {
            add_msg_debug( debugmode::DF_MELEE, "Attack is%s critical, attack discarded", crit ? "" : "n't" );
            continue;
        }

        // if the technique needs a loaded weapon and it isn't loaded skip it
        if( tec.needs_ammo && !is_loaded ) {
            add_msg_debug( debugmode::DF_MELEE, "No ammo, attack discarded" );
            continue;
        }

        // don't apply disarming techniques to someone without a weapon
        // TODO: these are the stat requirements for tec_disarm
        // dice(   dex_cur +    get_skill_level("unarmed"),  8) >
        // dice(p->dex_cur + p->get_skill_level("melee"),   10))
        if( tec.disarms && !t.has_weapon() ) {
            add_msg_debug( debugmode::DF_MELEE,
                           "Disarming technique against unarmed opponent, attack discarded" );
            continue;
        }

        if( tec.take_weapon && ( has_weapon() || !t.has_weapon() ) ) {
            add_msg_debug( debugmode::DF_MELEE, "Weapon-taking technique %s, attack discarded",
                           has_weapon() ? "while armed" : "against an unarmed opponent" );
            continue;
        }

        // if aoe, check if there are valid targets
        if( !tec.aoe.empty() && !valid_aoe_technique( t, tec ) ) {
            add_msg_debug( debugmode::DF_MELEE, "AoE technique witout valid AoE targets, attack discarded" );
            continue;
        }

        // If we have negative weighting then roll to see if it's valid this time
        if( tec.weighting < 0 && !one_in( std::abs( tec.weighting ) ) ) {
            add_msg_debug( debugmode::DF_MELEE,
                           "Negative technique weighting failed weight roll, attack discarded" );
            continue;
        }

        // Does the player have a functional attack vector to deliver the technique?
        std::vector<std::string> shuffled_attack_vectors = tec.attack_vectors_random;
        std::shuffle( shuffled_attack_vectors.begin(), shuffled_attack_vectors.end(), rng_get_engine() );
        if( martial_arts_data->get_valid_attack_vector( *this, tec.attack_vectors ) == "NONE" &&
            martial_arts_data->get_valid_attack_vector( *this, shuffled_attack_vectors ) == "NONE" ) {
            add_msg_debug( debugmode::DF_MELEE, "No valid attack vector found, attack discarded" );
            continue;
        }

        if( tec.is_valid_character( *this ) ) {
            possible.push_back( tec.id );

            //add weighted options into the list extra times, to increase their chance of being selected
            if( tec.weighting > 1 ) {
                for( int i = 1; i < tec.weighting; i++ ) {
                    possible.push_back( tec.id );
                }
            }
        }
    }

    return possible;
}

bool Character::valid_aoe_technique( Creature &t, const ma_technique &technique )
{
    std::vector<Creature *> dummy_targets;
    return valid_aoe_technique( t, technique, dummy_targets );
}

bool Character::valid_aoe_technique( Creature &t, const ma_technique &technique,
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

    creature_tracker &creatures = get_creature_tracker();
    //wide hits all targets adjacent to the attacker and the target
    if( technique.aoe == "wide" ) {
        //check if either (or both) of the squares next to our target contain a possible victim
        //offsets are a pre-computed matrix allowing us to quickly lookup adjacent squares
        tripoint left = pos() + tripoint( offset_a[lookup], offset_b[lookup], 0 );
        tripoint right = pos() + tripoint( offset_b[lookup], -offset_a[lookup], 0 );

        monster *const mon_l = creatures.creature_at<monster>( left );
        if( mon_l && mon_l->friendly == 0 ) {
            targets.push_back( mon_l );
        }
        monster *const mon_r = creatures.creature_at<monster>( right );
        if( mon_r && mon_r->friendly == 0 ) {
            targets.push_back( mon_r );
        }

        npc *const npc_l = creatures.creature_at<npc>( left );
        npc *const npc_r = creatures.creature_at<npc>( right );
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

        monster *const mon_l = creatures.creature_at<monster>( left );
        monster *const mon_t = creatures.creature_at<monster>( target_pos );
        monster *const mon_r = creatures.creature_at<monster>( right );
        if( mon_l && mon_l->friendly == 0 ) {
            targets.push_back( mon_l );
        }
        if( mon_t && mon_t->friendly == 0 ) {
            targets.push_back( mon_t );
        }
        if( mon_r && mon_r->friendly == 0 ) {
            targets.push_back( mon_r );
        }

        npc *const npc_l = creatures.creature_at<npc>( left );
        npc *const npc_t = creatures.creature_at<npc>( target_pos );
        npc *const npc_r = creatures.creature_at<npc>( right );
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
        for( const tripoint &tmp : get_map().points_in_radius( pos(), 1 ) ) {
            if( tmp == t.pos() ) {
                continue;
            }
            monster *const mon = creatures.creature_at<monster>( tmp );
            if( mon && mon->friendly == 0 ) {
                targets.push_back( mon );
            }
            npc *const np = creatures.creature_at<npc>( tmp );
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

bool character_martial_arts::has_technique( const Character &guy, const matec_id &id,
        const item &weap ) const
{
    return weap.has_technique( id ) ||
           style_selected->has_technique( guy, id );
}

static damage_unit &get_damage_unit( std::vector<damage_unit> &di, const damage_type_id &dt )
{
    static damage_unit nullunit( damage_type_id::NULL_ID(), 0, 0, 0, 0 );
    for( damage_unit &du : di ) {
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
    std::string ss;
    for( const damage_unit &du : di.damage_units ) {
        int amount = di.type_damage( du.type );
        total += amount;
        ss += du.type->name.translated() + ":" + std::to_string( amount ) + ",";
    }

    add_msg_debug( debugmode::DF_MELEE, "%stotal: %d", ss, total );
}

void Character::perform_technique( const ma_technique &technique, Creature &t,
                                   damage_instance &di,
                                   int &move_cost, item_location &cur_weapon )
{
    add_msg_debug( debugmode::DF_MELEE, "Chose technique %s\ndmg before tec:", technique.name );
    print_damage_info( di );
    int rep = rng( technique.repeat_min, technique.repeat_max );
    add_msg_debug( debugmode::DF_MELEE, "Tech repeats %d times", rep );

    // Keep the technique definitions shorter
    if( technique.attack_override ) {
        move_cost = 0;
        di.clear();
    }

    for( const damage_type &dt : damage_type::get_all() ) {
        float dam = technique.damage_bonus( *this, dt.id );
        float arpen = technique.armor_penetration( *this, dt.id );
        float mult = technique.damage_multiplier( *this, dt.id );

        if( mult != 1 ) {
            di.mult_type_damage( technique.damage_multiplier( *this, dt.id ), dt.id );
        }
        if( dam != 0 || arpen != 0 ) {
            di.add_damage( dt.id, dam, arpen, 1.0f, rep );
        }
    }
    add_msg_debug( debugmode::DF_MELEE, "dmg after attack" );
    print_damage_info( di );

    move_cost *= technique.move_cost_multiplier( *this );
    // Flat movecosts scale with repeating techs
    move_cost += technique.move_cost_penalty( *this ) * rep;

    // Add effects for each repeat of the tech
    for( int i = 0; i < rep; i++ ) {
        for( const tech_effect_data &eff : technique.tech_effects ) {
            // Add the tech's effects if it rolls the chance and either did damage or ignores it
            if( x_in_y( eff.chance, 100 ) && ( di.total_damage() != 0 || !eff.on_damage ) ) {
                if( eff.req_flag == json_flag_NULL || has_flag( eff.req_flag ) ) {
                    t.add_effect( eff.id, time_duration::from_turns( eff.duration ), eff.permanent );
                    add_msg_if_player( m_good, _( eff.message ), t.disp_name() );
                }
            }
        }

        if( technique.down_dur > 0 ) {
            if( t.get_throw_resist() == 0 ) {
                t.add_effect( effect_downed, rng( 1_turns, time_duration::from_turns( technique.down_dur ) ) );
                damage_unit &bash = get_damage_unit( di.damage_units, damage_bash );
                if( bash.amount > 0 ) {
                    bash.amount += 3;
                }
            }
        }

        if( technique.stun_dur > 0 ) {
            t.add_effect( effect_stunned, rng( 1_turns, time_duration::from_turns( technique.stun_dur ) ) );
        }

        for( const effect_on_condition_id &eoc : technique.eocs ) {
            dialogue d( get_talker_for( *this ), get_talker_for( t ) );
            if( eoc->type == eoc_type::ACTIVATION ) {
                eoc->activate( d );
            } else {
                debugmsg( "Must use an activation eoc for a technique activation.  If you don't want the effect_on_condition to happen on its own (without the technique being activated), remove the recurrence min and max.  Otherwise, create a non-recurring effect_on_condition for this technique with its condition and effects, then have a recurring one queue it." );
            }
        }
    }

    if( technique.needs_ammo ) {
        const itype_id current_ammo = cur_weapon.get_item()->ammo_current();
        // if the weapon needs ammo we now expend it
        cur_weapon.get_item()->ammo_consume( 1, pos(), this );
        // thing going off should be as loud as the ammo
        sounds::sound( pos(), current_ammo->ammo->loudness, sounds::sound_t::combat, _( "Crack!" ), true );
        const itype_id casing = *current_ammo->ammo->casing;
        if( cur_weapon.get_item()->has_flag( flag_RELOAD_EJECT ) ) {
            cur_weapon.get_item()->force_insert_item( item( casing ).set_flag( flag_CASING ),
                    pocket_type::MAGAZINE );
            cur_weapon.get_item()->on_contents_changed();
        }
    }

    if( technique.side_switch && !t.has_flag( mon_flag_IMMOBILE ) ) {
        const tripoint b = t.pos();
        point new_;

        if( b.x > posx() ) {
            new_.x = posx() - 1;
        } else if( b.x < posx() ) {
            new_.x = posx() + 1;
        } else {
            new_.x = b.x;
        }

        if( b.y > posy() ) {
            new_.y = posy() - 1;
        } else if( b.y < posy() ) {
            new_.y = posy() + 1;
        } else {
            new_.y = b.y;
        }

        const tripoint &dest = tripoint( new_, b.z );
        if( g->is_empty( dest ) ) {
            t.setpos( dest );
        }
    }
    map &here = get_map();
    if( technique.knockback_dist && !t.has_flag( mon_flag_IMMOBILE ) ) {
        const tripoint prev_pos = t.pos(); // track target startpoint for knockback_follow
        const point kb_offset( rng( -technique.knockback_spread, technique.knockback_spread ),
                               rng( -technique.knockback_spread, technique.knockback_spread ) );
        tripoint kb_point( posx() + kb_offset.x, posy() + kb_offset.y, posz() );
        for( int dist = rng( 1, technique.knockback_dist ); dist > 0; dist-- ) {
            t.knock_back_from( kb_point );
        }

        Character *passenger = t.as_character();
        if( passenger && passenger->in_vehicle ) {
            here.unboard_vehicle( prev_pos );
        }

        // This technique makes the player follow into the tile the target was knocked from
        if( technique.knockback_follow ) {
            const optional_vpart_position vp0 = here.veh_at( pos() );
            vehicle *const veh0 = veh_pointer_or_null( vp0 );
            bool to_swimmable = here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, prev_pos );
            bool to_deepwater = here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, prev_pos );

            // Check if it's possible to move to the new tile
            bool move_issue =
                g->is_dangerous_tile( prev_pos ) || // Tile contains fire, etc
                ( to_swimmable && to_deepwater ) || // Dive into deep water
                is_mounted() ||
                ( veh0 != nullptr && std::abs( veh0->velocity ) > 100 ) || // Diving from moving vehicle
                ( veh0 != nullptr && veh0->player_in_control( get_avatar() ) ) || // Player is driving
                has_effect( effect_amigara ) ||
                has_flag( json_flag_GRAB );
            if( !move_issue ) {
                if( t.pos() != prev_pos ) {
                    g->place_player( prev_pos );
                    g->on_move_effects();
                }
            }
        }
        // Remove our grab if we knocked back our grabber (if we can do so is handled by tech conditions)
        if( has_flag( json_flag_GRAB ) && t.has_effect_with_flag( json_flag_GRAB_FILTER ) ) {
            for( const effect &eff : get_effects_with_flag( json_flag_GRAB ) ) {
                if( t.is_monster() ) {
                    monster *m = t.as_monster();
                    if( m->is_grabbing( eff.get_bp().id() ) ) {
                        m->remove_grab( eff.get_bp().id() );
                        remove_effect( eff.get_id(), eff.get_bp() );
                        add_msg_debug( debugmode::DF_MELEE, "Grabber %s knocked back, grab on %s removed", t.get_name(),
                                       eff.get_bp()->name );
                    }
                }
            }
        }
    }

    Character *you = dynamic_cast<Character *>( &t );

    if( technique.take_weapon && !has_weapon() && you != nullptr && you->is_armed() &&
        !you->is_hallucination() ) {
        if( you->is_avatar() ) {
            add_msg_if_npc( _( "<npcname> disarms you and takes your weapon!" ) );
        } else {
            add_msg_player_or_npc( _( "You disarm %s and take their weapon!" ),
                                   _( "<npcname> disarms %s and takes their weapon!" ),
                                   you->get_name() );
        }
        item it = you->remove_weapon();
        wield( it );
    }

    if( technique.disarms && you != nullptr && you->is_armed() && !you->is_hallucination() ) {
        item weap = you->remove_weapon();
        here.add_item_or_charges( you->pos(), weap );
        if( you->is_avatar() ) {
            add_msg_if_npc( _( "<npcname> disarms you!" ) );
        } else {
            add_msg_player_or_npc( _( "You disarm %s!" ),
                                   _( "<npcname> disarms %s!" ),
                                   you->get_name() );
        }
    }

    //AOE attacks, feel free to skip over this lump
    if( !technique.aoe.empty() ) {
        // Remember out moves and stamina
        // We don't want to consume them for every attack!
        const int temp_moves = moves;
        const int temp_stamina = get_stamina();

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

        t.add_msg_if_player( m_good, n_gettext( "%d enemy hit!", "%d enemies hit!", count_hit ),
                             count_hit );
        // Extra attacks are free of charge (otherwise AoE attacks would SUCK)
        moves = temp_moves;
        set_stamina( temp_stamina );
    }
}

int melee::blocking_ability( const item &shield )
{
    int block_bonus = 0;
    if( shield.has_technique( WBLOCK_3 ) ) {
        block_bonus = 10;
    } else if( shield.has_technique( WBLOCK_2 ) ) {
        block_bonus = 6;
    } else if( shield.has_technique( WBLOCK_1 ) ) {
        block_bonus = 4;
    } else if( shield.has_flag( flag_BLOCK_WHILE_WORN ) ) {
        block_bonus = 2;
    }
    return block_bonus;
}

item_location Character::best_shield()
{
    // Note: wielded weapon, not one used for attacks
    int best_value = melee::blocking_ability( weapon );
    // "BLOCK_WHILE_WORN" without a blocking tech need to be worn for the bonus
    best_value = best_value == 2 ? 0 : best_value;
    item_location best = best_value > 0 ? get_wielded_item() : item_location();
    item *best_worn = worn.best_shield();
    if( best_worn && melee::blocking_ability( *best_worn ) >= best_value ) {
        best = item_location( *this, best_worn );
    }

    return best;
}

bool Character::block_hit( Creature *source, bodypart_id &bp_hit, damage_instance &dam )
{

    // Shouldn't block if player is asleep or winded
    if( in_sleep_state() || has_effect( effect_narcosis ) ||
        has_effect( effect_winded ) || has_effect( effect_fearparalyze ) || is_driving() ) {
        return false;
    }

    // fire martial arts on-getting-hit-triggered effects
    // these fire even if the attack is blocked (you still got hit)
    martial_arts_data->ma_ongethit_effects( *this );

    if( blocks_left < 1 ) {
        return false;
    }

    // Melee skill and reaction score governs if you can react in time
    // Skill of 5 without relevant encumbrance guarantees a block attempt
    float melee_skill = has_active_bionic( bio_cqb ) ? 5 : get_skill_level( skill_melee );
    if( !x_in_y( melee_skill * 20.0 * get_limb_score( limb_score_reaction ), 100 ) ) {
        add_msg_debug( debugmode::DF_MELEE, "Block roll failed" );
        return false;
    }

    blocks_left--;

    // This bonus absorbs damage from incoming attacks before they land,
    // but it still counts as a block even if it absorbs all the damage.
    float total_phys_block = mabuff_block_bonus();

    // Extract this to make it easier to implement shields/multiwield later
    item_location shield = best_shield();

    // Check if we are going to block with an item. This could
    // be worn equipment with the BLOCK_WHILE_WORN flag.
    const bool has_shield = !!shield;
    bool worn_shield = has_shield && shield->has_flag( flag_BLOCK_WHILE_WORN );

    bool conductive_shield = false;
    bool unarmed = !is_armed();
    bool force_unarmed = martial_arts_data->is_force_unarmed();
    bool allow_weapon_blocking = martial_arts_data->can_weapon_block();
    bool armed_body_block = weapon.has_flag( flag_ALLOWS_BODY_BLOCK );

    // boolean check if blocking is being done with unarmed or not
    const bool item_blocking = allow_weapon_blocking && has_shield && !unarmed && !armed_body_block;

    bool arm_block = false;
    bool leg_block = false;
    bool nonstandard_block = false;

    int unarmed_skill = round( get_skill_level( skill_unarmed ) );

    int block_score = 1;

    if( has_shield ) {
        block_bonus = melee::blocking_ability( *shield );
        conductive_shield = shield->conductive();
    }

    /** @ARM_STR increases attack blocking effectiveness with a limb or worn/wielded item */
    /** @EFFECT_UNARMED increases attack blocking effectiveness with a limb or worn item */
    if( unarmed || force_unarmed || worn_shield || armed_body_block || ( has_shield &&
            !allow_weapon_blocking ) ) {
        arm_block = martial_arts_data->can_arm_block( *this );
        leg_block = martial_arts_data->can_leg_block( *this );
        nonstandard_block = martial_arts_data->can_nonstandard_block( *this );
        if( arm_block || leg_block || nonstandard_block ) {
            // block_bonus for limb blocks will be added when the limb is decided
            block_score = get_arm_str() + unarmed_skill;
        } else {
            // We don't have a shield or a technique. How are we blocking?
            return false;
        }
        // Do we block with a weapon? Worn shields are already filtered out
        // And weapon blocks are preferred by best_shield
    } else if( has_shield ) {
        block_score = get_arm_str() + block_bonus + melee_skill;
    } else {
        // Can't block with limbs or items (do not block)
        return false;
    }

    // add martial arts block effectiveness bonus
    block_score += mabuff_block_effectiveness_bonus();

    // weapon blocks are preferred to limb blocks
    std::string thing_blocked_with;
    // Do we block with a weapon? Handle melee wear but leave bp the same
    if( !( unarmed || force_unarmed || worn_shield || armed_body_block ) && allow_weapon_blocking ) {
        thing_blocked_with = shield->tname();
        // TODO: Change this depending on damage blocked
        float wear_modifier = 1.0f;
        if( source != nullptr && source->is_hallucination() ) {
            wear_modifier = 0.0f;
        }

        handle_melee_wear( shield, wear_modifier );
    }  else {
        // Select part to block with, preferring worn blocking armor if applicable
        bp_hit = select_blocking_part( arm_block, leg_block, nonstandard_block );
        block_score *= get_part( bp_hit )->get_limb_score( limb_score_block );
        add_msg_debug( debugmode::DF_MELEE, "Block score after multiplier %d", block_score );
        if( worn_shield && shield->covers( bp_hit ) ) {
            thing_blocked_with = shield->tname();

            if( source != nullptr && !source->is_hallucination() ) {
                for( damage_unit &du : dam.damage_units ) {
                    shield->damage_armor_durability( du, bp_hit );
                }
            }

            block_score += block_bonus;

        } else {
            thing_blocked_with = body_part_name( bp_hit );
        }
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

    add_msg_debug( debugmode::DF_MELEE, "Physical block multiplier %.1f", physical_block_multiplier );
    float total_damage = 0.0f;
    float damage_blocked = 0.0f;

    for( damage_unit &elem : dam.damage_units ) {
        total_damage += elem.amount;

        // block physical damage "normally"
        if( elem.type->physical && elem.type->melee_only ) {
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
        }

        else if( !elem.type->physical && !elem.type->no_resist ) {
            // electrical damage deals full damage if unarmed OR wielding a conductive weapon
            // non-electrical "elemental" damage types do their full damage if unarmed,
            // but severely mitigated damage if not
            bool can_block = elem.type == STATIC( damage_type_id( "electric" ) ) ? !conductive_shield : true;
            // Unarmed weapons won't block those
            if( item_blocking && can_block ) {
                float previous_amount = elem.amount;
                elem.amount /= 5;
                damage_blocked += previous_amount - elem.amount;
            }
        }
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
    add_msg_debug( debugmode::DF_MELEE, "Blocked damage %.1f / %.1f", total_damage, damage_blocked );

    // fire martial arts block-triggered effects
    martial_arts_data->ma_onblock_effects( *this );

    // Check if we have any block counters
    matec_id tec = pick_technique( *source, shield, false, false, true );

    if( tec != tec_none && !is_dead_state() ) {
        int twenty_percent = std::round( ( 20 * weapon.type->mat_portion_total ) / 100.0f );
        if( get_stamina() < get_stamina_max() / 3 ) {
            add_msg( m_bad, _( "You try to counterattack but you are too exhausted!" ) );
        } else if( weapon.made_of( material_glass ) > twenty_percent ) {
            add_msg( m_bad, _( "The item you are wielding is too fragile to counterattack with!" ) );
        } else {
            melee_attack( *source, false, tec );
        }
    }

    return true;
}

void Character::perform_special_attacks( Creature &t, dealt_damage_instance &dealt_dam )
{
    std::vector<special_attack> special_attacks = mutation_attacks( t );

    bool practiced = false;
    for( const special_attack &att : special_attacks ) {
        if( t.is_dead_state() ) {
            break;
        }
        //TODO: Add flags to distinct mutation attack that can be triggered by reach attack (or just use ranged_mutation to fire fake gun.)
        if( !is_adjacent( &t, true ) ) {
            break;
        }

        // TODO: Make this hit roll use unarmed skill, not weapon skill + weapon to_hit
        int hit_spread = t.deal_melee_attack( this, hit_roll() * 0.8 );
        if( hit_spread >= 0 ) {
            weakpoint_attack attack;
            t.deal_melee_hit( this, hit_spread, false, att.damage, dealt_dam, attack );
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
    }
}

std::string Character::melee_special_effects( Creature &t, damage_instance &d, item &weap )
{
    std::string dump;

    std::string target = t.disp_name();

    if( has_active_bionic( bio_shock ) && get_power_level() >= bio_shock->power_trigger &&
        ( weap.is_null() || weapon.conductive() ) ) {
        mod_power_level( -bio_shock->power_trigger );
        d.add_damage( STATIC( damage_type_id( "electric" ) ), rng( 2, 10 ) );

        if( is_avatar() ) {
            dump += string_format( _( "You shock %s." ), target ) + "\n";
        } else {
            add_msg_if_npc( _( "<npcname> shocks %s." ), target );
        }
    }

    if( has_active_bionic( bio_heat_absorb ) && weap.is_null() && t.is_warm() ) {
        mod_power_level( bio_heat_absorb->power_trigger );
        d.add_damage( STATIC( damage_type_id( "cold" ) ), 3 );
        if( is_avatar() ) {
            dump += string_format( _( "You drain %s's body heat." ), target ) + "\n";
        } else {
            add_msg_if_npc( _( "<npcname> drains %s's body heat!" ), target );
        }
    }

    if( weap.has_flag( flag_FLAMING ) ) {
        d.add_damage( STATIC( damage_type_id( "heat" ) ), rng( 1, 8 ) );

        if( is_avatar() ) {
            dump += string_format( _( "You burn %s." ), target ) + "\n";
        } else {
            add_msg_player_or_npc( _( "<npcname> burns %s." ), target );
        }
    }

    //Hurting the wielder from poorly-chosen weapons
    if( weap.has_flag( flag_HURT_WHEN_WIELDED ) && x_in_y( 2, 3 ) ) {
        add_msg_if_player( m_bad, _( "The %s cuts your hand!" ), weap.tname() );
        damage_instance di = damage_instance();
        di.add_damage( damage_cut, weap.damage_melee( damage_cut ) );
        deal_damage( nullptr, bodypart_id( "hand_r" ), di );
        if( weap.is_two_handed( *this ) ) { // Hurt left hand too, if it was big
            deal_damage( nullptr, bodypart_id( "hand_l" ), di );
        }
    }

    // Glass weapons shatter sometimes
    const int glass_portion = weap.made_of( material_glass );
    float glass_fraction = glass_portion / static_cast<float>( weap.type->mat_portion_total );
    if( std::isnan( glass_fraction ) || glass_fraction > 1.f ) {
        glass_fraction = 0.f;
    }
    // only consider portion of weapon made of glass
    const int vol = weap.volume() * glass_fraction / 250_ml;
    if( glass_portion &&
        /** @ARM_STR increases chance of breaking glass weapons (NEGATIVE) */
        rng_float( 0.0f, vol + 8 ) < vol + get_arm_str() ) {
        if( is_avatar() ) {
            dump += string_format( _( "Your %s shatters!" ), weap.tname() ) + "\n";
        } else {
            add_msg_player_or_npc( m_bad, _( "Your %s shatters!" ),
                                   _( "<npcname>'s %s shatters!" ),
                                   weap.tname() );
        }

        sounds::sound( pos(), 16, sounds::sound_t::combat, "Crack!", true, "smash_success",
                       "smash_glass_contents" );
        // Dump its contents on the ground
        weap.spill_contents( pos() );
        // Take damage
        damage_instance di = damage_instance();
        di.add_damage( damage_cut, std::clamp( rng( 0, vol * 2 ), 0, 7 ) );
        deal_damage( nullptr, bodypart_id( "arm_r" ), di );
        if( weap.is_two_handed( *this ) ) { // Hurt left arm too, if it was big
            //redeclare shatter_dam because deal_damage mutates it
            deal_damage( nullptr, bodypart_id( "arm_l" ), di );
        }
        d.add_damage( damage_cut, rng( 0, 5 + static_cast<int>( vol * 1.5 ) ) ); // Hurt the monster extra
        remove_weapon();
    }

    // on-hit effects for martial arts
    martial_arts_data->ma_onhit_effects( *this );

    return dump;
}

static damage_instance hardcoded_mutation_attack( const Character &u, const trait_id &id )
{
    if( id == trait_BEAK_PECK ) {
        // method open to improvement, please feel free to suggest
        // a better way to simulate target's anti-peck efforts
        /** @EFFECT_DEX increases number of hits with BEAK_PECK */

        /** @EFFECT_UNARMED increases number of hits with BEAK_PECK */
        int num_hits = std::max( 1, std::min<int>( 6,
                                 u.get_dex() + u.get_skill_level( skill_unarmed ) - rng( 4, 10 ) ) );
        damage_instance di = damage_instance();
        di.add_damage( damage_stab, num_hits * 10 );
        return di;
    }

    if( id == trait_ARM_TENTACLES || id == trait_ARM_TENTACLES_4 || id == trait_ARM_TENTACLES_8 ) {
        int num_attacks = 1;
        if( id == trait_ARM_TENTACLES_4 ) {
            num_attacks = 3;
        } else if( id == trait_ARM_TENTACLES_8 ) {
            num_attacks = 7;
        }
        // Note: we're counting arms, so we want wielded item here, not weapon used for attack
        if( ( u.get_wielded_item() && u.get_wielded_item()->is_two_handed( u ) ) ||
            !u.has_two_arms_lifting() || u.worn_with_flag( flag_RESTRICT_HANDS ) ) {
            num_attacks--;
        }

        if( num_attacks <= 0 ) {
            return damage_instance();
        }

        const bool rake = u.has_trait( trait_CLAWS_TENTACLE );

        /** @EFFECT_STR increases damage with ARM_TENTACLES* */
        damage_instance ret;
        if( rake ) {
            ret.add_damage( damage_cut, u.get_str() / 2.0f + 1.0f, 0, 1.0f, num_attacks );
        } else {
            ret.add_damage( damage_bash, u.get_str() / 3.0f + 1.0f, 0, 1.0f, num_attacks );
        }

        return ret;
    }

    if( id == trait_VINES2 || id == trait_VINES3 ) {
        const int num_attacks = id == trait_VINES2 ? 2 : 3;
        /** @EFFECT_STR increases damage with VINES* */
        damage_instance ret;
        ret.add_damage( damage_bash, u.get_str() / 2.0f, 0, 1.0f, num_attacks );
        return ret;
    }

    debugmsg( "Invalid hardcoded mutation id: %s", id.c_str() );
    return damage_instance();
}

std::vector<special_attack> Character::mutation_attacks( Creature &t ) const
{
    std::vector<special_attack> ret;

    std::string target = t.disp_name();

    const body_part_set usable_body_parts = exclusive_flag_coverage( flag_ALLOWS_NATURAL_ATTACKS );
    const int unarmed = get_skill_level( skill_unarmed );

    for( const trait_id &pr : get_mutations() ) {
        const mutation_branch &branch = pr.obj();
        for( const mut_attack &mut_atk : branch.attacks_granted ) {
            // Covered body part
            if( !mut_atk.bp.is_empty() && !usable_body_parts.test( mut_atk.bp ) ) {
                continue;
            }

            /** @EFFECT_UNARMED increases chance of attacking with mutated body parts */
            /** @EFFECT_DEX increases chance of attacking with mutated body parts */

            // Calculate actor ability value to be compared against mutation attack difficulty and add debug message
            const int proc_value = get_dex() + unarmed;
            add_msg_debug( debugmode::DF_MELEE, "%s proc chance: %d in %d", pr.c_str(), proc_value,
                           mut_atk.chance );
            // If the mutation attack fails to proc, bail out
            if( !x_in_y( proc_value, mut_atk.chance ) ) {
                continue;
            }

            // If player has any blocker, bail out
            if( std::any_of( mut_atk.blocker_mutations.begin(), mut_atk.blocker_mutations.end(),
            [this]( const trait_id & blocker ) {
            return has_trait( blocker );
            } ) ) {
                add_msg_debug( debugmode::DF_MELEE, "%s not procing: blocked", pr.c_str() );
                continue;
            }

            // Player must have all needed traits
            if( !std::all_of( mut_atk.required_mutations.begin(), mut_atk.required_mutations.end(),
            [this]( const trait_id & need ) {
            return has_trait( need );
            } ) ) {
                add_msg_debug( debugmode::DF_MELEE, "%s not procing: unmet req", pr.c_str() );
                continue;
            }

            special_attack tmp;
            // Ugly special case: player's strings have only 1 variable, NPC have 2
            // Can't use <npcname> here
            // TODO: Fix
            if( is_avatar() ) {
                tmp.text = string_format( mut_atk.attack_text_u.translated(), target );
            } else {
                tmp.text = string_format( mut_atk.attack_text_npc.translated(), get_name(), target );
            }

            // Attack starts here
            if( mut_atk.hardcoded_effect ) {
                tmp.damage = hardcoded_mutation_attack( *this, pr );
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
                add_msg_debug( debugmode::DF_MELEE, "%s not procing: zero damage", pr.c_str() );
            }
        }
    }

    return ret;
}

std::string melee_message( const ma_technique &tec, Character &p,
                           const dealt_damage_instance &ddi )
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

    int total_dam = 0;
    std::pair<damage_type_id, int> dominant_type = { damage_bash, 0 };
    for( const damage_type &dt : damage_type::get_all() ) {
        if( dt.melee_only ) {
            int dmg = ddi.type_damage( dt.id );
            total_dam += dmg;
            if( dominant_type.second < dmg ) {
                dominant_type = { dt.id, dmg };
            }
        }
    }

    if( tec.id != tec_none ) {
        std::string message;
        if( p.is_npc() ) {
            message = tec.npc_message.translated();
        } else {
            message = tec.avatar_message.translated();
        }
        if( !message.empty() ) {
            return message;
        }
    }

    const bool npc = p.is_npc();

    // Cutting has more messages and so needs different handling
    const bool cutting = dominant_type.first == damage_cut;
    size_t index;
    if( total_dam > 30 ) {
        index = cutting ? rng( 0, 4 ) : rng( 0, 2 );
    } else if( total_dam > 20 ) {
        index = cutting ? rng( 5, 6 ) : 3;
    } else if( total_dam > 10 ) {
        index = cutting ? 7 : 4;
    } else {
        index = cutting ? 8 : 5;
    }

    std::string message;
    // FIXME: Hardcoded damage type messages
    if( dominant_type.first == damage_stab ) {
        message = npc ? _( npc_stab[index] ) : _( player_stab[index] );
    } else if( dominant_type.first == damage_cut ) {
        message = npc ? _( npc_cut[index] ) : _( player_cut[index] );
    } else if( dominant_type.first == damage_bash ) {
        message = npc ? _( npc_bash[index] ) : _( player_bash[index] );
    }
    if( ddi.wp_hit.empty() ) {
        return message;
    } else {
        //~ %1$s: "Somone hit something", %2$s: the weakpoint that was hit
        return string_format( _( "%1$s in %2$s" ), message, ddi.wp_hit );
    }

    return _( "The bugs attack %s" );
}

// display the hit message for an attack
void player_hit_message( Character *attacker, const std::string &message,
                         Creature &t, int dam, bool crit, bool technique, const std::string &wp_hit )
{
    std::string msg;
    game_message_type msgtype = m_good;
    std::string sSCTmod;
    game_message_type gmtSCTcolor = m_good;

    Character &player_character = get_player_character();
    if( dam <= 0 ) {
        if( attacker->is_npc() ) {
            //~ NPC hits something but does no damage
            msg = string_format( _( "%s but does no damage." ), message );
        } else {
            //~ someone hits something but do no damage
            msg = string_format( _( "%s but do no damage." ), message );
        }
        msgtype = m_neutral;
    } else if( crit ) {
        //Player won't see exact numbers of damage dealt by NPC unless player has DEBUG_NIGHTVISION trait
        if( attacker->is_npc() && !player_character.has_trait( trait_DEBUG_NIGHTVISION ) ) {
            //~ NPC hits something (critical)
            msg = string_format( _( "%s.  Critical!" ), message );
        } else if( technique && !wp_hit.empty() ) {
            //~ %1$s: "someone hits something", %2$d: damage dealt, %3$s: the weakpoint hit
            msg = string_format( _( "%1$s for %2$d damage, and hit it in %3$s.  Critical!" ), message, dam,
                                 wp_hit );
        } else {
            //~ someone hits something for %d damage (critical)
            msg = string_format( _( "%s for %d damage.  Critical!" ), message, dam );
        }
        sSCTmod = _( "Critical!" );
        gmtSCTcolor = m_critical;
    } else {
        if( attacker->is_npc() && !player_character.has_trait( trait_DEBUG_NIGHTVISION ) ) {
            //~ NPC hits something
            msg = string_format( _( "%s." ), message );
        } else if( technique && !wp_hit.empty() ) {
            //~ %1$s: "someone hits something", %2$d: damage dealt, %3$s: the weakpoint hit
            msg = string_format( _( "%1$s for %2$d damage, and hit it in %3$s." ), message, dam, wp_hit );
        } else {
            //~ someone hits something for %d damage
            msg = string_format( _( "%s for %d damage." ), message, dam );
        }
    }

    if( dam > 0 && attacker->is_avatar() ) {
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
                     _( "HP" ), m_neutral,
                     "hp" );
        } else {
            SCT.removeCreatureHP();
        }
    }

    // same message is used for player and npc,
    // just using this for the <npcname> substitution.
    attacker->add_msg_player_or_npc( msgtype, msg, msg, t.disp_name() );
}

int Character::attack_speed( const item &weap ) const
{
    const int base_move_cost = weap.attack_time( *this ) / 2;
    const float melee_skill = has_active_bionic( bionic_id( bio_cqb ) ) ? BIO_CQB_LEVEL :
                              get_skill_level( skill_melee );
    /** @EFFECT_MELEE increases melee attack speed */
    const int skill_cost = static_cast<int>( ( base_move_cost * ( 15 - melee_skill ) / 15 ) );
    /** @EFFECT_DEX increases attack speed */
    const int dexbonus = dex_cur / 2;
    const int ma_move_cost = mabuff_attack_cost_penalty();
    const float stamina_ratio = static_cast<float>( get_stamina() ) / static_cast<float>
                                ( get_stamina_max() );
    // Increase cost multiplier linearly from 1.0 to 2.0 as stamina goes from 25% to 0%.
    const float stamina_penalty = 1.0 + std::max( ( 0.25f - stamina_ratio ) * 4.0f, 0.0f );
    const float ma_mult = mabuff_attack_cost_mult();

    double move_cost = base_move_cost;
    move_cost *= get_modifier( character_modifier_melee_thrown_move_lift_mod );
    move_cost *= get_modifier( character_modifier_melee_thrown_move_balance_mod );
    move_cost *= stamina_penalty;
    move_cost += skill_cost;
    move_cost -= dexbonus;

    move_cost = calculate_by_enchantment( move_cost, enchant_vals::mod::ATTACK_SPEED, true );
    // Martial arts last. Flat has to be after mult, because comments say so.
    move_cost *= ma_mult;
    move_cost += ma_move_cost;

    if( is_on_ground() ) {
        if( has_flag( json_flag_PSEUDOPOD_GRASP ) ) {
            move_cost *= 1.5;
        } else {
            move_cost *= 4.0;
        }
    } else if( is_crouching() && ( !has_flag( json_flag_PSEUDOPOD_GRASP ) &&
                                   ( !has_effect( effect_natural_stance ) && !unarmed_attack() ) ) ) {
        move_cost *= 1.5;
    }

    if( move_cost < 25.0 ) {
        return 25;
    }

    return std::round( move_cost );
}

double Character::weapon_value( const item &weap, int ammo ) const
{
    if( is_wielding( weap ) || ( !get_wielded_item() && weap.is_null() ) ) {
        auto cached_value = cached_info.find( "weapon_value" );
        if( cached_value != cached_info.end() ) {
            return cached_value->second;
        }
    }
    double val_gun = gun_value( weap, ammo );
    val_gun = val_gun /
              5.0f; // This is an emergency patch to get melee and ranged in approximate parity, if you're looking at it in 2025 or later and it's still here... I'm sorry.  Kill it with fire.  Tear it all down, and rebuild a glorious castle from the ashes.
    add_msg_debug( debugmode::DF_NPC_ITEMAI,
                   "<color_magenta>weapon_value</color>%s %s valued at <color_light_cyan>%1.2f as a ranged weapon</color>.",
                   disp_name( true ), weap.type->get_id().str(), val_gun );
    double val_melee = melee_value( weap );
    val_melee *=
        val_melee; // Same emergency patch.  Same purple prose descriptors, you already saw them above.
    add_msg_debug( debugmode::DF_NPC_ITEMAI,
                   "%s %s valued at <color_light_cyan>%1.2f as a melee weapon</color>.", disp_name( true ),
                   weap.type->get_id().str(), val_melee );
    const double more = std::max( val_gun, val_melee );
    const double less = std::min( val_gun, val_melee );

    // A small bonus for guns you can also use to hit stuff with (bayonets etc.)
    const double my_val = more + ( less / 2.0 );
    add_msg_debug( debugmode::DF_MELEE, "%s (%ld ammo) sum value: %.1f", weap.type->get_id().str(),
                   ammo, my_val );
    if( is_wielding( weap ) || ( !get_wielded_item() && weap.is_null() ) ) {
        cached_info.emplace( "weapon_value", my_val );
    }
    return my_val;
}

double Character::melee_value( const item &weap ) const
{
    // start with average effective dps against a range of enemies
    double my_value = weap.average_dps( *this );

    float reach = weap.reach_range( *this );
    // value reach weapons more
    if( reach > 1.0f ) {
        my_value *= 1.0f + 0.5f * ( std::sqrt( reach ) - 1.0f );
    }
    // value polearms less to account for the trickiness of keeping the right range
    if( weapon.has_flag( flag_POLEARM ) ) {
        my_value *= 0.8;
    }

    // value style weapons more
    if( !martial_arts_data->enumerate_known_styles( weap.type->get_id() ).empty() ) {
        my_value *= 1.5;
    }

    add_msg_debug( debugmode::DF_MELEE, "%s as melee: %.1f", weap.type->get_id().str(), my_value );

    return std::max( 0.0, my_value );
}

double Character::unarmed_value() const
{
    // TODO: Martial arts
    return melee_value( item() );
}

void avatar::disarm( npc &target )
{
    if( !target.is_armed() ) {
        return;
    }

    if( target.is_hallucination() ) {
        target.on_attacked( *this );
        return;
    }

    /** @ARM_STR increases chance to disarm, primary stat */
    /** @EFFECT_DEX increases chance to disarm, secondary stat */
    /** Grip strength modifies all disarm rolls */
    int my_roll = dice( 3, get_limb_score( limb_score_grip ) * get_arm_str() + get_dex() );

    /** @EFFECT_MELEE increases chance to disarm */
    my_roll += dice( 3, round( get_skill_level( skill_melee ) ) );

    int their_roll = dice( 3, target.get_limb_score( limb_score_grip ) * target.get_arm_str() +
                           target.get_dex() );
    their_roll *= target.get_limb_score( limb_score_reaction );
    their_roll += dice( 3, round( target.get_skill_level( skill_melee ) ) );

    item_location it = target.get_wielded_item();
    const item_location weapon = get_wielded_item();
    // roll your melee and target's dodge skills to check if grab/smash attack succeeds
    int hitspread = target.deal_melee_attack( this, hit_roll() );
    if( hitspread < 0 ) {
        add_msg( _( "You lunge for the %s, but miss!" ), it->tname() );
        item weap = get_wielded_item() ? *get_wielded_item() : null_item_reference();
        mod_moves( -100 - stumble( *this, weapon ) - attack_speed( weap ) );
        target.on_attacked( *this );
        return;
    }

    map &here = get_map();
    // hitspread >= 0, which means we are going to disarm by grabbing target by their weapon
    if( !is_armed() ) {
        /** @EFFECT_UNARMED increases chance to disarm, bonus when nothing wielded */
        my_roll += dice( 3, round( get_skill_level( skill_unarmed ) ) );

        if( my_roll >= their_roll ) {
            //~ %s: weapon name
            add_msg( _( "You grab at %s and pull with all your force!" ), it->tname() );
            //~ %1$s: weapon name, %2$s: NPC name
            add_msg( _( "You forcefully take %1$s from %2$s!" ), it->tname(), target.get_name() );
            // wield() will deduce our moves, consider to deduce more/less moves for balance
            item rem_it = target.i_rem( &*it );
            wield( rem_it );
        } else if( my_roll >= their_roll / 2 ) {
            add_msg( _( "You grab at %s and pull with all your force, but it drops nearby!" ),
                     it->tname() );
            const tripoint tp = target.pos() + tripoint( rng( -1, 1 ), rng( -1, 1 ), 0 );
            here.add_item_or_charges( tp, target.i_rem( &*it ) );
            mod_moves( -100 );
        } else {
            add_msg( _( "You grab at %s and pull with all your force, but in vain!" ), it->tname() );
            mod_moves( -100 );
        }

        target.on_attacked( *this );
    } else {
        // Make their weapon fall on floor if we've rolled enough.
        mod_moves( -100 - attack_speed( *get_wielded_item() ) );
        if( my_roll >= their_roll ) {
            add_msg( _( "You smash %s with all your might forcing their %s to drop down nearby!" ),
                     target.get_name(), it->tname() );
            const tripoint tp = target.pos() + tripoint( rng( -1, 1 ), rng( -1, 1 ), 0 );
            here.add_item_or_charges( tp, target.i_rem( &*it ) );
        } else {
            add_msg( _( "You smash %s with all your might but %s remains in their hands!" ),
                     target.get_name(), it->tname() );
        }

        target.on_attacked( *this );
    }
}

void avatar::steal( npc &target )
{
    if( target.is_enemy() ) {
        add_msg( _( "%s is hostile!" ), target.get_name() );
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
    if( my_roll >= their_roll && !target.is_hallucination() ) {
        add_msg( _( "You sneakily steal %1$s from %2$s!" ),
                 it->tname(), target.get_name() );
        i_add( target.i_rem( it ) );
    } else if( my_roll >= their_roll / 2 ) {
        add_msg( _( "You failed to steal %1$s from %2$s, but did not attract attention." ),
                 it->tname(), target.get_name() );
    } else  {
        add_msg( _( "You failed to steal %1$s from %2$s." ),
                 it->tname(), target.get_name() );
        target.on_attacked( *this );
    }

    // consider to deduce less/more moves for balance
    mod_moves( -200 );
}

/**
 * Once the accuracy (sum of modifiers) of an attack has been determined,
 * this is used to actually roll the "hit value" of the attack to be compared to dodge.
 */
float melee::melee_hit_range( float accuracy )
{
    return normal_roll( accuracy * 5, 25.0f );
}

melee_statistic_data melee::get_stats()
{
    return melee_stats;
}

void melee::clear_stats()
{
    melee_stats = melee_statistic_data{};
}

namespace melee
{
melee_statistic_data melee_stats;
} // namespace melee
