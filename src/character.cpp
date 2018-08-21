#include "character.h"
#include "game.h"
#include "map.h"
#include "bionics.h"
#include "map_selector.h"
#include "effect.h"
#include "vehicle_selector.h"
#include "debug.h"
#include "mission.h"
#include "translations.h"
#include "itype.h"
#include "options.h"
#include "map_iterator.h"
#include "field.h"
#include "messages.h"
#include "input.h"
#include "monster.h"
#include "mtype.h"
#include "player.h"
#include "mutation.h"
#include "skill.h"
#include "vehicle.h"
#include "output.h"
#include "veh_interact.h"
#include "cata_utility.h"
#include "pathfinding.h"
#include "string_formatter.h"

#include <algorithm>
#include <sstream>
#include <numeric>

const efftype_id effect_bandaged( "bandaged" );
const efftype_id effect_beartrap( "beartrap" );
const efftype_id effect_bite( "bite" );
const efftype_id effect_bleed( "bleed" );
const efftype_id effect_blind( "blind" );
const efftype_id effect_boomered( "boomered" );
const efftype_id effect_contacts( "contacts" );
const efftype_id effect_crushed( "crushed" );
const efftype_id effect_darkness( "darkness" );
const efftype_id effect_disinfected( "disinfected" );
const efftype_id effect_downed( "downed" );
const efftype_id effect_grabbed( "grabbed" );
const efftype_id effect_heavysnare( "heavysnare" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_in_pit( "in_pit" );
const efftype_id effect_lightsnare( "lightsnare" );
const efftype_id effect_narcosis( "narcosis" );
const efftype_id effect_sleep( "sleep" );
const efftype_id effect_webbed( "webbed" );

const skill_id skill_dodge( "dodge" );
const skill_id skill_throw( "throw" );

static const trait_id trait_ACIDBLOOD( "ACIDBLOOD" );
static const trait_id trait_ARACHNID_ARMS( "ARACHNID_ARMS" );
static const trait_id trait_ARM_TENTACLES_4( "ARM_TENTACLES_4" );
static const trait_id trait_ARM_TENTACLES_8( "ARM_TENTACLES_8" );
static const trait_id trait_ARM_TENTACLES( "ARM_TENTACLES" );
static const trait_id trait_BADBACK( "BADBACK" );
static const trait_id trait_BENDY2( "BENDY2" );
static const trait_id trait_BENDY3( "BENDY3" );
static const trait_id trait_BIRD_EYE( "BIRD_EYE" );
static const trait_id trait_CEPH_EYES( "CEPH_EYES" );
static const trait_id trait_CEPH_VISION( "CEPH_VISION" );
static const trait_id trait_CHITIN2( "CHITIN2" );
static const trait_id trait_CHITIN3( "CHITIN3" );
static const trait_id trait_CHITIN_FUR3( "CHITIN_FUR3" );
static const trait_id trait_DEBUG_NIGHTVISION( "DEBUG_NIGHTVISION" );
static const trait_id trait_DISORGANIZED( "DISORGANIZED" );
static const trait_id trait_ELFA_FNV( "ELFA_FNV" );
static const trait_id trait_ELFA_NV( "ELFA_NV" );
static const trait_id trait_FEL_NV( "FEL_NV" );
static const trait_id trait_FLIMSY2( "FLIMSY2" );
static const trait_id trait_FLIMSY3( "FLIMSY3" );
static const trait_id trait_FLIMSY( "FLIMSY" );
static const trait_id trait_GLASSJAW( "GLASSJAW" );
static const trait_id trait_HOLLOW_BONES( "HOLLOW_BONES" );
static const trait_id trait_HUGE( "HUGE" );
static const trait_id trait_INSECT_ARMS( "INSECT_ARMS" );
static const trait_id trait_LIGHT_BONES( "LIGHT_BONES" );
static const trait_id trait_MEMBRANE( "MEMBRANE" );
static const trait_id trait_MUT_TOUGH2( "MUT_TOUGH2" );
static const trait_id trait_MUT_TOUGH3( "MUT_TOUGH3" );
static const trait_id trait_MUT_TOUGH( "MUT_TOUGH" );
static const trait_id trait_MYOPIC( "MYOPIC" );
static const trait_id trait_NIGHTVISION2( "NIGHTVISION2" );
static const trait_id trait_NIGHTVISION3( "NIGHTVISION3" );
static const trait_id trait_NIGHTVISION( "NIGHTVISION" );
static const trait_id trait_PACKMULE( "PACKMULE" );
static const trait_id trait_PER_SLIME_OK( "PER_SLIME_OK" );
static const trait_id trait_PER_SLIME( "PER_SLIME" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_SHELL( "SHELL" );
static const trait_id trait_STRONGBACK( "STRONGBACK" );
static const trait_id trait_TAIL_CATTLE( "TAIL_CATTLE" );
static const trait_id trait_TAIL_FLUFFY( "TAIL_FLUFFY" );
static const trait_id trait_TAIL_LONG( "TAIL_LONG" );
static const trait_id trait_TAIL_RAPTOR( "TAIL_RAPTOR" );
static const trait_id trait_TAIL_RAT( "TAIL_RAT" );
static const trait_id trait_TAIL_THICK( "TAIL_THICK" );
static const trait_id trait_THICK_SCALES( "THICK_SCALES" );
static const trait_id trait_THRESH_CEPHALOPOD( "THRESH_CEPHALOPOD" );
static const trait_id trait_THRESH_INSECT( "THRESH_INSECT" );
static const trait_id trait_THRESH_PLANT( "THRESH_PLANT" );
static const trait_id trait_THRESH_SPIDER( "THRESH_SPIDER" );
static const trait_id trait_TOUGH2( "TOUGH2" );
static const trait_id trait_TOUGH3( "TOUGH3" );
static const trait_id trait_TOUGH( "TOUGH" );
static const trait_id trait_URSINE_EYE( "URSINE_EYE" );
static const trait_id trait_WEBBED( "WEBBED" );
static const trait_id trait_WINGS_BAT( "WINGS_BAT" );
static const trait_id trait_WINGS_BUTTERFLY( "WINGS_BUTTERFLY" );
static const trait_id debug_nodmg( "DEBUG_NODMG" );

Character::Character() : Creature(), visitable<Character>(), hp_cur(
{{
        0
    }
} ), hp_max( {{0}} )
{
    str_max = 0;
    dex_max = 0;
    per_max = 0;
    int_max = 0;
    str_cur = 0;
    dex_cur = 0;
    per_cur = 0;
    int_cur = 0;
    str_bonus = 0;
    dex_bonus = 0;
    per_bonus = 0;
    int_bonus = 0;
    healthy = 0;
    healthy_mod = 0;
    hunger = 0;
    starvation = 0;
    thirst = 0;
    fatigue = 0;
    stomach_food = 0;
    stomach_water = 0;

    name.clear();

    *path_settings = pathfinding_settings{ 0, 1000, 1000, 0, true, false, true };
}

Character::~Character() = default;
Character::Character( const Character & ) = default;
Character::Character( Character && ) = default;
Character &Character::operator=( const Character & ) = default;
Character &Character::operator=( Character && ) = default;

field_id Character::bloodType() const
{
    if (has_trait( trait_ACIDBLOOD ))
        return fd_acid;
    if (has_trait( trait_THRESH_PLANT ))
        return fd_blood_veggy;
    if (has_trait( trait_THRESH_INSECT ) || has_trait( trait_THRESH_SPIDER ))
        return fd_blood_insect;
    if (has_trait( trait_THRESH_CEPHALOPOD ))
        return fd_blood_invertebrate;
    return fd_blood;
}
field_id Character::gibType() const
{
    return fd_gibs_flesh;
}

bool Character::is_warm() const
{
    return true; // TODO: is there a mutation (plant?) that makes a npc not warm blooded?
}

const std::string &Character::symbol() const
{
    static const std::string character_symbol("@");
    return character_symbol;
}

void Character::mod_stat( const std::string &stat, float modifier )
{
    if( stat == "str" ) {
        mod_str_bonus( modifier );
    } else if( stat == "dex" ) {
        mod_dex_bonus( modifier );
    } else if( stat == "per" ) {
        mod_per_bonus( modifier );
    } else if( stat == "int" ) {
        mod_int_bonus( modifier );
    } else if( stat == "healthy" ) {
        mod_healthy( modifier );
    } else if( stat == "hunger" ) {
        mod_hunger( modifier );
    } else if( stat == "starvation" ) {
        mod_starvation( modifier );
    } else {
        Creature::mod_stat( stat, modifier );
    }
}

int Character::effective_dispersion( int dispersion ) const
{
    /** @EFFECT_PER penalizes sight dispersion when low. */
    dispersion += ranged_per_mod();

    dispersion += encumb( bp_eyes ) / 2;

    return std::max( dispersion, 0 );
}

std::pair<int, int> Character::get_fastest_sight( const item &gun, double recoil ) const
{
    // Get fastest sight that can be used to improve aim further below @ref recoil.
    int sight_speed_modifier = INT_MIN;
    int limit = 0;
    if( effective_dispersion( gun.type->gun->sight_dispersion ) < recoil ) {
        sight_speed_modifier = gun.has_flag( "DISABLE_SIGHTS" ) ? 0 : 6;
        limit = effective_dispersion( gun.type->gun->sight_dispersion );
    }

    for( const auto e : gun.gunmods() ) {
        const islot_gunmod &mod = *e->type->gunmod;
        if( mod.sight_dispersion < 0 || mod.aim_speed < 0 ) {
            continue; // skip gunmods which don't provide a sight
        }
        if( effective_dispersion( mod.sight_dispersion ) < recoil && mod.aim_speed > sight_speed_modifier ) {
            sight_speed_modifier = mod.aim_speed;
            limit = effective_dispersion( mod.sight_dispersion );
        }
    }
    return std::make_pair( sight_speed_modifier, limit );
}

int Character::get_most_accurate_sight( const item &gun ) const
{
    if( !gun.is_gun() ) {
        return 0;
    }

    int limit = effective_dispersion( gun.type->gun->sight_dispersion );
    for( const auto e : gun.gunmods() ) {
        const islot_gunmod &mod = *e->type->gunmod;
        if( mod.aim_speed >= 0 ) {
            limit = std::min( limit, effective_dispersion( mod.sight_dispersion ) );
        }
    }

    return limit;
}

double Character::aim_speed_skill_modifier( const skill_id &gun_skill ) const
{
    double skill_mult = 1.0;
    if( gun_skill == "pistol" ) {
        skill_mult = 2.0;
    } else if( gun_skill == "rifle" ) {
        skill_mult = 0.9;
    }
    /** @EFFECT_PISTOL increases aiming speed for pistols */
    /** @EFFECT_SMG increases aiming speed for SMGs */
    /** @EFFECT_RIFLE increases aiming speed for rifles */
    /** @EFFECT_SHOTGUN increases aiming speed for shotguns */
    /** @EFFECT_LAUNCHER increases aiming speed for launchers */
    return skill_mult * std::min( MAX_SKILL, get_skill_level( gun_skill ) );
}

double Character::aim_speed_dex_modifier() const
{
    return get_dex() - 8;
}

double Character::aim_speed_encumbrance_modifier() const
{
    return ( encumb( bp_hand_l ) + encumb( bp_hand_r ) ) / 10.0;
}

double Character::aim_cap_from_volume( const item &gun ) const
{
    skill_id gun_skill = gun.gun_skill();
    double aim_cap = std::min( 49.0, 49.0 - static_cast<float>( gun.volume() / 75_ml ) );
    // TODO: also scale with skill level.
    if( gun_skill == "smg" ) {
        aim_cap = std::max( 12.0, aim_cap );
    } else if( gun_skill == "shotgun" ) {
        aim_cap = std::max( 12.0, aim_cap );
    } else if( gun_skill == "pistol" ) {
        aim_cap = std::max( 15.0, aim_cap * 1.25 );
    } else if( gun_skill == "rifle" ) {
        aim_cap = std::max( 7.0, aim_cap - 5.0 );
    } else { // Launchers, etc.
        aim_cap = std::max( 10.0, aim_cap );
    }
    return aim_cap;
}

double Character::aim_per_move( const item &gun, double recoil ) const
{
    if( !gun.is_gun() ) {
        return 0.0;
    }

    std::pair<int, int> best_sight = get_fastest_sight( gun, recoil );
    int sight_speed_modifier = best_sight.first;
    int limit = best_sight.second;
    if( sight_speed_modifier == INT_MIN ) {
        // No suitable sights (already at maximum aim).
        return 0;
    }

    // Overall strategy for determining aim speed is to sum the factors that contribute to it,
    // then scale that speed by current recoil level.
    // Player capabilities make aiming faster, and aim speed slows down as it approaches 0.
    // Base speed is non-zero to prevent extreme rate changes as aim speed approaches 0.
    double aim_speed = 10.0;

    skill_id gun_skill = gun.gun_skill();
    // Ranges [0 - 10]
    aim_speed += aim_speed_skill_modifier( gun_skill );

    // Range [0 - 12]
    /** @EFFECT_DEX increases aiming speed */
    aim_speed += aim_speed_dex_modifier();

    // Range [0 - 10]
    aim_speed += sight_speed_modifier;

    // Each 5 points (combined) of hand encumbrance decreases aim speed by one unit.
    aim_speed -= aim_speed_encumbrance_modifier();

    aim_speed = std::min( aim_speed, aim_cap_from_volume( gun ) );

    // Just a raw scaling factor.
    aim_speed *= 6.5;

    // Scale rate logistically as recoil goes from MAX_RECOIL to 0.
    aim_speed *= 1.0 - logarithmic_range( 0, MAX_RECOIL, recoil );

    // Minimum improvement is 5MoA.  This mostly puts a cap on how long aiming for sniping takes.
    aim_speed = std::max( aim_speed, 5.0 );

    // Never improve by more than the currently used sights permit.
    return std::min( aim_speed, recoil - limit );
}

bool Character::move_effects(bool attacking)
{
    if (has_effect( effect_downed )) {
        /** @EFFECT_DEX increases chance to stand up when knocked down */

        /** @EFFECT_STR increases chance to stand up when knocked down, slightly */
        if (rng(0, 40) > get_dex() + get_str() / 2) {
            add_msg_if_player(_("You struggle to stand."));
        } else {
            add_msg_player_or_npc(m_good, _("You stand up."),
                                    _("<npcname> stands up."));
            remove_effect( effect_downed);
        }
        return false;
    }
    if (has_effect( effect_webbed )) {
        /** @EFFECT_STR increases chance to escape webs */
        if (x_in_y(get_str(), 6 * get_effect_int( effect_webbed ))) {
            add_msg_player_or_npc(m_good, _("You free yourself from the webs!"),
                                    _("<npcname> frees themselves from the webs!"));
            remove_effect( effect_webbed);
        } else {
            add_msg_if_player(_("You try to free yourself from the webs, but can't get loose!"));
        }
        return false;
    }
    if (has_effect( effect_lightsnare )) {
        /** @EFFECT_STR increases chance to escape light snare */

        /** @EFFECT_DEX increases chance to escape light snare */
        if(x_in_y(get_str(), 12) || x_in_y(get_dex(), 8)) {
            remove_effect( effect_lightsnare);
            add_msg_player_or_npc(m_good, _("You free yourself from the light snare!"),
                                    _("<npcname> frees themselves from the light snare!"));
            item string("string_36", calendar::turn);
            item snare("snare_trigger", calendar::turn);
            g->m.add_item_or_charges(pos(), string);
            g->m.add_item_or_charges(pos(), snare);
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the light snare, but can't get loose!"));
        }
        return false;
    }
    if (has_effect( effect_heavysnare )) {
        /** @EFFECT_STR increases chance to escape heavy snare, slightly */

        /** @EFFECT_DEX increases chance to escape light snare */
        if(x_in_y(get_str(), 32) || x_in_y(get_dex(), 16)) {
            remove_effect( effect_heavysnare);
            add_msg_player_or_npc(m_good, _("You free yourself from the heavy snare!"),
                                    _("<npcname> frees themselves from the heavy snare!"));
            item rope("rope_6", calendar::turn);
            item snare("snare_trigger", calendar::turn);
            g->m.add_item_or_charges(pos(), rope);
            g->m.add_item_or_charges(pos(), snare);
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the heavy snare, but can't get loose!"));
        }
        return false;
    }
    if (has_effect( effect_beartrap )) {
        /* Real bear traps can't be removed without the proper tools or immense strength; eventually this should
           allow normal players two options: removal of the limb or removal of the trap from the ground
           (at which point the player could later remove it from the leg with the right tools).
           As such we are currently making it a bit easier for players and NPC's to get out of bear traps.
        */
        /** @EFFECT_STR increases chance to escape bear trap */
        if(x_in_y(get_str(), 100)) {
            remove_effect( effect_beartrap);
            add_msg_player_or_npc(m_good, _("You free yourself from the bear trap!"),
                                    _("<npcname> frees themselves from the bear trap!"));
            item beartrap("beartrap", calendar::turn);
            g->m.add_item_or_charges(pos(), beartrap);
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the bear trap, but can't get loose!"));
        }
        return false;
    }
    if (has_effect( effect_crushed )) {
        /** @EFFECT_STR increases chance to escape crushing rubble */

        /** @EFFECT_DEX increases chance to escape crushing rubble, slightly */
        if(x_in_y(get_str() + get_dex() / 4, 100)) {
            remove_effect( effect_crushed);
            add_msg_player_or_npc(m_good, _("You free yourself from the rubble!"),
                                    _("<npcname> frees themselves from the rubble!"));
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the rubble, but can't get loose!"));
        }
        return false;
    }
    // Below this point are things that allow for movement if they succeed

    // Currently we only have one thing that forces movement if you succeed, should we get more
    // than this will need to be reworked to only have success effects if /all/ checks succeed
    if (has_effect( effect_in_pit )) {
        /** @EFFECT_STR increases chance to escape pit */

        /** @EFFECT_DEX increases chance to escape pit, slightly */
        if (rng(0, 40) > get_str() + get_dex() / 2) {
            add_msg_if_player(m_bad, _("You try to escape the pit, but slip back in."));
            return false;
        } else {
            add_msg_player_or_npc(m_good, _("You escape the pit!"),
                                    _("<npcname> escapes the pit!"));
            remove_effect( effect_in_pit);
        }
    }
    if( has_effect( effect_grabbed ) && !attacking ) {
        int zed_number = 0;
        for( auto &&dest : g->m.points_in_radius( pos(), 1, 0 ) ){
            const monster *const mon = g->critter_at<monster>( dest );
            if( mon &&
                ( mon->has_flag( MF_GRABS ) ||
                  mon->type->has_special_attack( "GRAB" ) ) ) {
                zed_number ++;
            }
        }
        if( zed_number == 0 ) {
            add_msg_player_or_npc( m_good, _( "You find yourself no longer grabbed." ),
                                   _( "<npcname> finds themselves no longer grabbed." ) );
            remove_effect( effect_grabbed );
        /** @EFFECT_DEX increases chance to escape grab, if >STR */

        /** @EFFECT_STR increases chance to escape grab, if >DEX */
        } else if( rng( 0, std::max( get_dex(), get_str() ) ) < rng( get_effect_int( effect_grabbed ), 8 ) ) {
            // Randomly compare higher of dex or str to grab intensity.
            add_msg_player_or_npc( m_bad, _( "You try break out of the grab, but fail!" ),
                                   _( "<npcname> tries to break out of the grab, but fails!" ) );
            return false;
        } else {
            add_msg_player_or_npc( m_good, _( "You break out of the grab!" ),
                                   _( "<npcname> breaks out of the grab!" ) );
            remove_effect( effect_grabbed );
        }
    }
    return true;
}

void Character::add_effect( const efftype_id &eff_id, const time_duration dur, body_part bp,
                            bool permanent, int intensity, bool force, bool deferred )
{
    Creature::add_effect( eff_id, dur, bp, permanent, intensity, force, deferred );
}

void Character::process_turn()
{
    Creature::process_turn();
    drop_inventory_overflow();
}

void Character::recalc_hp()
{
    int new_max_hp[num_hp_parts];
    // Mutated toughness stacks with starting, by design.
    float hp_mod = 1.0f + mutation_value( "hp_modifier" ) + mutation_value( "hp_modifier_secondary" );
    float hp_adjustment = mutation_value( "hp_adjustment" );
    for( auto &elem : new_max_hp ) {
        /** @EFFECT_STR_MAX increases base hp */
        elem = 60 + str_max * 3 + hp_adjustment;
        elem *= hp_mod;
    }
    if( has_trait( trait_GLASSJAW ) ) {
        new_max_hp[hp_head] *= 0.8;
    }
    for( int i = 0; i < num_hp_parts; i++ ) {
        hp_cur[i] *= (float)new_max_hp[i] / (float)hp_max[i];
        hp_max[i] = new_max_hp[i];
    }
}


// This must be called when any of the following change:
// - effects
// - bionics
// - traits
// - underwater
// - clothes
// With the exception of clothes, all changes to these character attributes must
// occur through a function in this class which calls this function. Clothes are
// typically added/removed with wear() and takeoff(), but direct access to the
// 'wears' vector is still allowed due to refactor exhaustion.
void Character::recalc_sight_limits()
{
    sight_max = 9999;
    vision_mode_cache.reset();

    // Set sight_max.
    if( is_blind() || in_sleep_state() || has_effect( effect_narcosis ) ) {
        sight_max = 0;
    } else if( has_effect( effect_boomered ) && (!(has_trait( trait_PER_SLIME_OK ))) ) {
        sight_max = 1;
        vision_mode_cache.set( BOOMERED );
    } else if (has_effect( effect_in_pit ) ||
            (underwater && !has_bionic( bionic_id( "bio_membrane" ) ) &&
                !has_trait( trait_MEMBRANE ) && !worn_with_flag("SWIM_GOGGLES") &&
                !has_trait( trait_CEPH_EYES ) && !has_trait( trait_PER_SLIME_OK ) ) ) {
        sight_max = 1;
    } else if (has_active_mutation( trait_SHELL2 )) {
        // You can kinda see out a bit.
        sight_max = 2;
    } else if ( (has_trait( trait_MYOPIC ) || has_trait( trait_URSINE_EYE )) &&
            !worn_with_flag( "FIX_NEARSIGHT" ) && !has_effect( effect_contacts )) {
        sight_max = 4;
    } else if (has_trait( trait_PER_SLIME )) {
        sight_max = 6;
    } else if( has_effect( effect_darkness ) ) {
        vision_mode_cache.set( DARKNESS );
        sight_max = 10;
    }

    // Debug-only NV, by vache's request
    if( has_trait( trait_DEBUG_NIGHTVISION ) ) {
        vision_mode_cache.set( DEBUG_NIGHTVISION );
    }
    if( has_nv() ) {
        vision_mode_cache.set( NV_GOGGLES );
    }
    if( has_active_mutation( trait_NIGHTVISION3 ) || is_wearing("rm13_armor_on") ) {
        vision_mode_cache.set( NIGHTVISION_3 );
    }
    if( has_active_mutation( trait_ELFA_FNV ) ) {
        vision_mode_cache.set( FULL_ELFA_VISION );
    }
    if( has_active_mutation( trait_CEPH_VISION ) ) {
        vision_mode_cache.set( CEPH_VISION );
    }
    if (has_active_mutation( trait_ELFA_NV )) {
        vision_mode_cache.set( ELFA_VISION );
    }
    if( has_active_mutation( trait_NIGHTVISION2 ) ) {
        vision_mode_cache.set( NIGHTVISION_2 );
    }
    if( has_active_mutation( trait_FEL_NV ) ) {
        vision_mode_cache.set( FELINE_VISION );
    }
    if( has_active_mutation( trait_URSINE_EYE ) ) {
        vision_mode_cache.set( URSINE_VISION );
    }
    if (has_active_mutation( trait_NIGHTVISION )) {
        vision_mode_cache.set(NIGHTVISION_1);
    }
    if( has_trait( trait_BIRD_EYE ) ) {
        vision_mode_cache.set( BIRD_EYE);
    }

    // Not exactly a sight limit thing, but related enough
    if( has_active_bionic( bionic_id( "bio_infrared" ) ) ||
        has_trait( trait_id( "INFRARED" ) ) ||
        has_trait( trait_id( "LIZ_IR" ) ) ||
        worn_with_flag( "IR_EFFECT" ) ) {
        vision_mode_cache.set( IR_VISION );
    }

    if( has_artifact_with( AEP_SUPER_CLAIRVOYANCE ) ) {
        vision_mode_cache.set( VISION_CLAIRVOYANCE_SUPER );
    }

    if( has_artifact_with( AEP_CLAIRVOYANCE_PLUS ) ) {
        vision_mode_cache.set( VISION_CLAIRVOYANCE_PLUS );
    }

    if( has_artifact_with( AEP_CLAIRVOYANCE ) ) {
        vision_mode_cache.set( VISION_CLAIRVOYANCE );
    }
}

static float threshold_for_range( float range )
{
    constexpr float epsilon = 0.01f;
    return LIGHT_AMBIENT_MINIMAL / exp( range * LIGHT_TRANSPARENCY_OPEN_AIR ) - epsilon;
}

float Character::get_vision_threshold( float light_level ) const {
    if( vision_mode_cache[DEBUG_NIGHTVISION] ) {
        // Debug vision always works with absurdly little light.
        return 0.01;
    }

    // As light_level goes from LIGHT_AMBIENT_MINIMAL to LIGHT_AMBIENT_LIT,
    // dimming goes from 1.0 to 2.0.
    const float dimming_from_light = 1.0 + (((float)light_level - LIGHT_AMBIENT_MINIMAL) /
                                            (LIGHT_AMBIENT_LIT - LIGHT_AMBIENT_MINIMAL));

    float range = get_per() / 3.0f - encumb( bp_eyes ) / 10.0f;
    if( vision_mode_cache[NV_GOGGLES] || vision_mode_cache[NIGHTVISION_3] ||
        vision_mode_cache[FULL_ELFA_VISION] || vision_mode_cache[CEPH_VISION] ) {
        range += 10;
    } else if( vision_mode_cache[NIGHTVISION_2] || vision_mode_cache[FELINE_VISION] ||
               vision_mode_cache[URSINE_VISION] || vision_mode_cache[ELFA_VISION] ) {
        range += 4.5;
    } else if( vision_mode_cache[NIGHTVISION_1] ) {
        range += 2;
    }

    if( vision_mode_cache[BIRD_EYE] ) {
        range++;
    }

    return std::min( (float)LIGHT_AMBIENT_LOW, threshold_for_range( range ) * dimming_from_light );
}

bool Character::has_bionic(const bionic_id &b) const
{
    for (auto &i : *my_bionics) {
        if (i.id == b) {
            return true;
        }
    }
    return false;
}

bool Character::has_active_bionic(const bionic_id &b) const
{
    for (auto &i : *my_bionics) {
        if (i.id == b) {
            return (i.powered);
        }
    }
    return false;
}

std::vector<item_location> Character::nearby( const std::function<bool(const item *, const item *)>& func, int radius ) const
{
    std::vector<item_location> res;

    visit_items( [&]( const item *e, const item *parent ) {
        if( func( e, parent ) ) {
            res.emplace_back( const_cast<Character &>( *this ), const_cast<item *>( e ) );
        }
        return VisitResponse::NEXT;
    } );

    for( const auto &cur : map_selector( pos(), radius ) ) {
        cur.visit_items( [&]( const item *e, const item *parent  ) {
            if( func( e, parent ) ) {
                res.emplace_back( cur, const_cast<item *>( e ) );
            }
            return VisitResponse::NEXT;
        } );
    }

    for( const auto &cur : vehicle_selector( pos(), radius ) ) {
        cur.visit_items( [&]( const item *e, const item *parent  ) {
            if( func( e, parent ) ) {
                res.emplace_back( cur, const_cast<item *>( e ) );
            }
            return VisitResponse::NEXT;
        } );
    }

    return res;
}

long int Character::i_add_to_container(const item &it, const bool unloading)
{
    long int charges = it.charges;
    if( !it.is_ammo() || unloading ) {
        return charges;
    }

    const itype_id item_type = it.typeId();
    auto add_to_container = [&it, &charges](item &container) {
        auto &contained_ammo = container.contents.front();
        if( contained_ammo.charges < container.ammo_capacity() ) {
            const long int diff = container.ammo_capacity() - contained_ammo.charges;
            add_msg( _( "You put the %s in your %s." ), it.tname().c_str(), container.tname().c_str() );
            if( diff > charges ) {
                contained_ammo.charges += charges;
                return 0L;
            } else {
                contained_ammo.charges = container.ammo_capacity();
                return charges - diff;
            }
        }
        return charges;
    };

    visit_items( [ & ]( item *item ) {
        if( charges > 0 && item->is_ammo_container() && item_type == item->contents.front().typeId() ) {
            charges = add_to_container(*item);
        }
        return VisitResponse::NEXT;
    } );

    return charges;
}

item& Character::i_add(item it)
{
    itype_id item_type_id = it.typeId();
    last_item = item_type_id;

    if( it.is_food() || it.is_ammo() || it.is_gun()  || it.is_armor() ||
        it.is_book() || it.is_tool() || it.is_melee() || it.is_food_container() ) {
        inv.unsort();
    }

    // if there's a desired invlet for this item type, try to use it
    bool keep_invlet = false;
    const std::set<char> cur_inv = allocated_invlets();
    for (auto iter : inv.assigned_invlet) {
        if (iter.second == item_type_id && !cur_inv.count(iter.first)) {
            it.invlet = iter.first;
            keep_invlet = true;
            break;
        }
    }

    auto &item_in_inv = inv.add_item(it, keep_invlet);
    item_in_inv.on_pickup( *this );
    return item_in_inv;
}

std::list<item> Character::remove_worn_items_with( std::function<bool(item &)> filter )
{
    std::list<item> result;
    for( auto iter = worn.begin(); iter != worn.end(); ) {
        if( filter( *iter ) ) {
            iter->on_takeoff( *this );
            result.splice( result.begin(), worn, iter++ );
        } else {
            ++iter;
        }
    }
    return result;
}

// Negative positions indicate weapon/clothing, 0 & positive indicate inventory
const item& Character::i_at(int position) const
{
    if( position == -1 ) {
        return weapon;
    }
    if( position < -1 ) {
        int worn_index = worn_position_to_index(position);
        if (size_t(worn_index) < worn.size()) {
            auto iter = worn.begin();
            std::advance( iter, worn_index );
            return *iter;
        }
    }

    return inv.find_item(position);
}

item& Character::i_at(int position)
{
    return const_cast<item&>( const_cast<const Character*>(this)->i_at( position ) );
}

int Character::get_item_position( const item *it ) const
{
    if( weapon.has_item( *it ) ) {
        return -1;
    }

    int p = 0;
    for( const auto &e : worn ) {
        if( e.has_item( *it ) ) {
            return worn_position_to_index( p );
        }
        p++;
    }

    return inv.position_by_item( it );
}

item Character::i_rem(int pos)
{
 item tmp;
 if (pos == -1) {
     tmp = weapon;
     weapon = item();
     return tmp;
 } else if (pos < -1 && pos > worn_position_to_index(worn.size())) {
     auto iter = worn.begin();
     std::advance( iter, worn_position_to_index( pos ) );
     tmp = *iter;
     tmp.on_takeoff( *this );
     worn.erase( iter );
     return tmp;
 }
 return inv.remove_item(pos);
}

item Character::i_rem(const item *it)
{
    auto tmp = remove_items_with( [&it] (const item &i) { return &i == it; }, 1 );
    if( tmp.empty() ) {
        debugmsg( "did not found item %s to remove it!", it->tname().c_str() );
        return item();
    }
    return tmp.front();
}

void Character::i_rem_keep_contents( const int pos )
{
    for( auto &content : i_rem( pos ).contents ) {
        i_add_or_drop( content );
    }
}

bool Character::i_add_or_drop( item& it, int qty ) {
    bool retval = true;
    bool drop = it.made_of( LIQUID );
    bool add = it.is_gun() || !it.is_irremovable();
    inv.assign_empty_invlet( it, *this );
    for( int i = 0; i < qty; ++i ) {
        drop |= !can_pickWeight( it, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) || !can_pickVolume( it );
        if( drop ) {
            retval &= !g->m.add_item_or_charges( pos(), it ).is_null();
        } else if ( add ) {
            i_add( it );
        }
    }

    return retval;
}

std::set<char> Character::allocated_invlets() const {
    std::set<char> invlets = inv.allocated_invlets();

    if (weapon.invlet != 0) {
        invlets.insert(weapon.invlet);
    }
    for( const auto &w : worn ) {
        if( w.invlet != 0 ) {
            invlets.insert( w.invlet );
        }
    }

    return invlets;
}

bool Character::has_active_item(const itype_id & id) const
{
    return has_item_with( [id]( const item & it ) {
        return it.active && it.typeId() == id;
    } );
}

item Character::remove_weapon()
{
 item tmp = weapon;
 weapon = item();
 return tmp;
}

void Character::remove_mission_items( int mission_id )
{
    if( mission_id == -1 ) {
        return;
    }
    remove_items_with( has_mission_item_filter { mission_id } );
}

std::vector<const item *> Character::get_ammo( const ammotype &at ) const
{
    return items_with( [at]( const item & it ) {
        return it.is_ammo() && it.type->ammo->type.count( at );
    } );
}

template <typename T, typename Output>
void find_ammo_helper( T& src, const item& obj, bool empty, Output out, bool nested ) {
    if( obj.is_watertight_container() ) {
        if (!obj.is_container_empty()) {
            auto contents_id = obj.contents.front().typeId();

            // Look for containers with the same type of liquid as that already in our container
            src.visit_items( [&src, &nested, &out, &contents_id, &obj]( item *node ) {
                if ( node == &obj ) {
                    // This stops containers and magazines counting *themselves* as ammo sources.
                    return VisitResponse::SKIP;
                }

                if ( node->is_container() && !node->is_container_empty() &&
                        node->contents.front().typeId() == contents_id ) {
                    out = item_location( src, node );
                }
                return nested ? VisitResponse::NEXT : VisitResponse::SKIP;
            } );
        } else {
            // Look for containers with any liquid
            src.visit_items( [&src, &nested, &out]( item *node ) {
                if ( node->is_container() && !node->is_container_empty() &&
                        node->contents.front().made_of( LIQUID ) ) {
                    out = item_location( src, node );
                }
                return nested ? VisitResponse::NEXT : VisitResponse::SKIP;
            } );
        }
    }
    if( obj.magazine_integral() ) {
        // find suitable ammo excluding that already loaded in magazines
        ammotype ammo = obj.ammo_type();
        const auto mags = obj.magazine_compatible();

        src.visit_items( [&src,&nested,&out,&mags,ammo]( item *node ) {
            if( node->is_gun() || node->is_tool() ) {
                // guns/tools never contain usable ammo so most efficient to skip them now
                return VisitResponse::SKIP;
            }
            if( !node->made_of( SOLID ) ) {
                // some liquids are ammo but we can't reload with them unless within a container
                return VisitResponse::SKIP;
            }
            if( node->is_ammo_container() && !node->contents.front().made_of( SOLID ) ) {
                if( node->contents.front().type->ammo->type.count( ammo ) ) {
                    out = item_location( src, node );
                }
                return VisitResponse::SKIP;
            }
            if( node->is_ammo() && node->type->ammo->type.count( ammo ) ) {
                out = item_location( src, node );
            }
            if( node->is_magazine() && node->has_flag( "SPEEDLOADER" ) ) {
                if( mags.count( node->typeId() ) && node->ammo_remaining() ) {
                    out = item_location( src, node );
                }
            }
            return nested ? VisitResponse::NEXT : VisitResponse::SKIP;
        } );
    } else {
        // find compatible magazines excluding those already loaded in tools/guns
        const auto mags = obj.magazine_compatible();

        src.visit_items( [&src,&nested,&out,mags,empty]( item *node ) {
            if( node->is_gun() || node->is_tool() ) {
                return VisitResponse::SKIP;
            }
            if( node->is_magazine() ) {
                if ( mags.count( node->typeId() ) && ( node->ammo_remaining() || empty ) ) {
                    out = item_location( src, node );
                }
                return VisitResponse::SKIP;
            }
            return nested ? VisitResponse::NEXT : VisitResponse::SKIP;
        } );
    }
}

std::vector<item_location> Character::find_ammo( const item& obj, bool empty, int radius ) const
{
    std::vector<item_location> res;

    find_ammo_helper( const_cast<Character &>( *this ), obj, empty, std::back_inserter( res ), true );

    if( radius >= 0 ) {
        for( auto& cursor : map_selector( pos(), radius ) ) {
            find_ammo_helper( cursor, obj, empty, std::back_inserter( res ), false );
        }
        for( auto& cursor : vehicle_selector( pos(), radius ) ) {
            find_ammo_helper( cursor, obj, empty, std::back_inserter( res ), false );
        }
    }

    return res;
}

units::mass Character::weight_carried() const
{
    units::mass ret = 0;
    ret += weapon.weight();
    for (auto &i : worn) {
        ret += i.weight();
    }
    ret += inv.weight();
    return ret;
}

units::volume Character::volume_carried() const
{
    return inv.volume();
}

units::mass Character::weight_capacity() const
{
    if( has_trait( trait_id( "DEBUG_STORAGE" ) ) ) {
        // Infinite enough
        return units::mass_max;
    }
    // Get base capacity from creature,
    // then apply player-only mutation and trait effects.
    units::mass ret = Creature::weight_capacity();
    /** @EFFECT_STR increases carrying capacity */
    ret += get_str() * 4_kilogram;
    if( has_trait( trait_id( "BADBACK" ) ) ) {
        ret = ret * .65;
    }
    if( has_trait( trait_id( "STRONGBACK" ) ) ) {
        ret = ret * 1.35;
    }
    if( has_trait( trait_id( "LIGHT_BONES" ) ) ) {
        ret = ret * .80;
    }
    if( has_trait( trait_id( "HOLLOW_BONES" ) ) ) {
        ret = ret * .60;
    }
    if (has_artifact_with(AEP_CARRY_MORE)) {
        ret += 22500_gram;
    }
    if (ret < 0) {
        ret = 0;
    }
    return ret;
}

units::volume Character::volume_capacity() const
{
    return volume_capacity_reduced_by( 0 );
}

units::volume Character::volume_capacity_reduced_by( const units::volume &mod ) const
{
    if( has_trait( trait_id( "DEBUG_STORAGE" ) ) ) {
        return units::volume_max;
    }

    units::volume ret = -mod;
    for (auto &i : worn) {
        ret += i.get_storage();
    }
    if( has_bionic( bionic_id( "bio_storage" ) ) ) {
        ret += 2000_ml;
    }
    if( has_trait( trait_SHELL ) ) {
        ret += 4000_ml;
    }
    if( has_trait( trait_SHELL2 ) && !has_active_mutation( trait_SHELL2 ) ) {
        ret += 6000_ml;
    }
    if( has_trait( trait_PACKMULE ) ) {
        ret = ret * 1.4;
    }
    if( has_trait( trait_DISORGANIZED ) ) {
        ret = ret * 0.6;
    }
    return std::max( ret, 0_ml );
}

bool Character::can_pickVolume( const item &it, bool ) const
{
    inventory projected = inv;
    projected.add_item( it, true );
    return projected.volume() <= volume_capacity();
}

bool Character::can_pickWeight( const item &it, bool safe ) const
{
    if (!safe)
    {
        // Character can carry up to four times their maximum weight
        return ( weight_carried() + it.weight() <= ( has_trait( trait_id( "DEBUG_STORAGE" ) ) ? units::mass_max : weight_capacity() * 4 ) );
    }
    else
    {
        return ( weight_carried() + it.weight() <= weight_capacity() );
    }
}

bool Character::can_use( const item& it, const item& context ) const {
    const auto &ctx = !context.is_null() ? context : it;

    if( !meets_requirements( it, ctx ) ) {
        const std::string unmet( enumerate_unmet_requirements( it, ctx ) );

        if( &it == &ctx ) {
            //~ %1$s - list of unmet requirements, %2$s - item name.
            add_msg_player_or_npc( m_bad, _( "You need at least %1$s to use this %2$s." ),
                                          _( "<npcname> needs at least %1$s to use this %2$s." ),
                                          unmet.c_str(), it.tname().c_str() );
        } else {
            //~ %1$s - list of unmet requirements, %2$s - item name, %3$s - indirect item name.
            add_msg_player_or_npc( m_bad, _( "You need at least %1$s to use this %2$s with your %3$s." ),
                                          _( "<npcname> needs at least %1$s to use this %2$s with their %3$s." ),
                                          unmet.c_str(), it.tname().c_str(), ctx.tname().c_str() );
        }

        return false;
    }

    return true;
}

void Character::drop_inventory_overflow() {
    if( volume_carried() > volume_capacity() ) {
        for( auto &item_to_drop :
               inv.remove_randomly_by_volume( volume_carried() - volume_capacity() ) ) {
            g->m.add_item_or_charges( pos(), item_to_drop );
        }
        add_msg_if_player( m_bad, _("Some items tumble to the ground.") );
    }
}

bool Character::has_artifact_with(const art_effect_passive effect) const
{
    if( weapon.has_effect_when_wielded( effect ) ) {
        return true;
    }
    for( auto & i : worn ) {
        if( i.has_effect_when_worn( effect ) ) {
            return true;
        }
    }
    return has_item_with( [effect]( const item & it ) {
        return it.has_effect_when_carried( effect );
    } );
}

bool Character::is_wearing(const itype_id & it) const
{
    for (auto &i : worn) {
        if (i.typeId() == it) {
            return true;
        }
    }
    return false;
}

bool Character::is_wearing_on_bp(const itype_id & it, body_part bp) const
{
    for (auto &i : worn) {
        if (i.typeId() == it && i.covers(bp)) {
            return true;
        }
    }
    return false;
}

bool Character::worn_with_flag( const std::string &flag, body_part bp ) const
{
    return std::any_of( worn.begin(), worn.end(), [&flag, bp]( const item &it ) {
        return it.has_flag( flag ) && ( bp == num_bp || it.covers( bp ) );
    } );
}

SkillLevel &Character::get_skill_level_object( const skill_id &ident )
{
    static SkillLevel null_skill;

    if( ident && ident->is_contextual_skill() ) {
        debugmsg( "Skill \"%s\" is context-dependent. It cannot be assigned.", ident.str() );
    } else {
        return (*_skills)[ident];
    }

    null_skill.level( 0 );
    return null_skill;
}

int Character::get_skill_level( const skill_id &ident ) const
{
    return get_skill_level_object( ident ).level();
}

const SkillLevel &Character::get_skill_level_object( const skill_id &ident ) const
{
    static const SkillLevel null_skill;

    if( ident && ident->is_contextual_skill() ) {
        debugmsg( "Skill \"%s\" is context-dependent. It cannot be assigned.", ident.str() );
        return null_skill;
    }

    const auto iter = _skills->find( ident );

    if( iter != _skills->end() ) {
        return iter->second;
    }

    return null_skill;
}

int Character::get_skill_level( const skill_id &ident, const item &context ) const
{
    return get_skill_level_object( context.is_null() ? ident : context.contextualize_skill( ident ) ).level();
}

void Character::set_skill_level( const skill_id &ident, const int level )
{
    get_skill_level_object( ident ).level( level );
}

void Character::mod_skill_level( const skill_id &ident, const int delta )
{
    SkillLevel &obj = get_skill_level_object( ident );
    obj.level( obj.level() + delta );
}

std::map<skill_id, int> Character::compare_skill_requirements( const std::map<skill_id, int> &req, const item &context ) const
{
    std::map<skill_id, int> res;

    for( const auto &elem : req ) {
        const int diff = get_skill_level( elem.first, context ) - elem.second;
        if( diff != 0 ) {
            res[elem.first] = diff;
        }
    }

    return res;
}

std::string Character::enumerate_unmet_requirements( const item &it, const item &context ) const
{
    std::vector<std::string> unmet_reqs;

    const auto check_req = [ &unmet_reqs ]( const std::string &name, int cur, int req ) {
        if( cur < req ) {
            unmet_reqs.push_back( string_format( "%s %d", name.c_str(), req ) );
        }
    };

    check_req( _( "strength" ),     get_str(), it.type->min_str );
    check_req( _( "dexterity" ),    get_dex(), it.type->min_dex );
    check_req( _( "intelligence" ), get_int(), it.type->min_int );
    check_req( _( "perception" ),   get_per(), it.type->min_per );

    for( const auto &elem : it.type->min_skills ) {
        check_req( context.contextualize_skill( elem.first )->name().c_str(),
                   get_skill_level( elem.first, context ),
                   elem.second );
    }

    return enumerate_as_string( unmet_reqs );
}

bool Character::meets_skill_requirements( const std::map<skill_id, int> &req, const item &context ) const
{
    return std::all_of( req.begin(), req.end(), [this, &context]( const std::pair<skill_id, int> &pr ) {
        return get_skill_level( pr.first, context ) >= pr.second;
    });
}

bool Character::meets_stat_requirements( const item &it ) const
{
    return get_str() >= it.type->min_str &&
           get_dex() >= it.type->min_dex &&
           get_int() >= it.type->min_int &&
           get_per() >= it.type->min_per;
}

bool Character::meets_requirements( const item &it, const item &context ) const
{
    const auto &ctx = !context.is_null() ? context : it;
    return meets_stat_requirements( it ) && meets_skill_requirements( it.type->min_skills, ctx );
}

void Character::normalize()
{
    Creature::normalize();

    weapon   = item("null", 0);

    recalc_hp();
}

// Actual player death is mostly handled in game::is_game_over
void Character::die(Creature* nkiller)
{
    g->set_critter_died();
    set_killer( nkiller );
    set_time_died( calendar::turn );
    if( has_effect( effect_lightsnare ) ) {
        inv.add_item( item( "string_36", 0 ) );
        inv.add_item( item( "snare_trigger", 0 ) );
    }
    if( has_effect( effect_heavysnare ) ) {
        inv.add_item( item( "rope_6", 0 ) );
        inv.add_item( item( "snare_trigger", 0 ) );
    }
    if( has_effect( effect_beartrap ) ) {
        inv.add_item( item( "beartrap", 0 ) );
    }
    mission::on_creature_death( *this );
}

void Character::reset_stats()
{
    // Bionic buffs
    if (has_active_bionic( bionic_id( "bio_hydraulics" ) ) )
        mod_str_bonus(20);
    if (has_bionic( bionic_id( "bio_eye_enhancer" ) ) )
        mod_per_bonus(2);
    if (has_bionic( bionic_id( "bio_str_enhancer" ) ) )
        mod_str_bonus(2);
    if (has_bionic( bionic_id( "bio_int_enhancer" ) ) )
        mod_int_bonus(2);
    if (has_bionic( bionic_id( "bio_dex_enhancer" ) ) )
        mod_dex_bonus(2);

    // Trait / mutation buffs
    if (has_trait( trait_THICK_SCALES )) {
        mod_dex_bonus(-2);
    }
    if (has_trait( trait_CHITIN2 ) || has_trait( trait_CHITIN3 ) || has_trait( trait_CHITIN_FUR3 )) {
        mod_dex_bonus(-1);
    }
    if (has_trait( trait_BIRD_EYE )) {
        mod_per_bonus(4);
    }
    if (has_trait( trait_INSECT_ARMS )) {
        mod_dex_bonus(-2);
    }
    if (has_trait( trait_WEBBED )) {
        mod_dex_bonus(-1);
    }
    if (has_trait( trait_ARACHNID_ARMS )) {
        mod_dex_bonus(-4);
    }
    if (has_trait( trait_ARM_TENTACLES ) || has_trait( trait_ARM_TENTACLES_4 ) ||
            has_trait( trait_ARM_TENTACLES_8 )) {
        mod_dex_bonus(1);
    }

    // Dodge-related effects
    if (has_trait( trait_TAIL_LONG )) {
        mod_dodge_bonus(2);
    }
    if (has_trait( trait_TAIL_CATTLE )) {
        mod_dodge_bonus(1);
    }
    if (has_trait( trait_TAIL_RAT )) {
        mod_dodge_bonus(2);
    }
    if (has_trait( trait_TAIL_THICK ) && !(has_active_mutation( trait_TAIL_THICK )) ) {
        mod_dodge_bonus(1);
    }
    if (has_trait( trait_TAIL_RAPTOR )) {
        mod_dodge_bonus(3);
    }
    if (has_trait( trait_TAIL_FLUFFY )) {
        mod_dodge_bonus(4);
    }
    if (has_trait( trait_WINGS_BAT )) {
        mod_dodge_bonus(-3);
    }
    if (has_trait( trait_WINGS_BUTTERFLY )) {
        mod_dodge_bonus(-4);
    }

    /** @EFFECT_STR_MAX above 15 decreases Dodge bonus by 1 (NEGATIVE) */
    if (str_max >= 16) {mod_dodge_bonus(-1);} // Penalty if we're huge
    /** @EFFECT_STR_MAX below 6 increases Dodge bonus by 1 */
    else if (str_max <= 5) {mod_dodge_bonus(1);} // Bonus if we're small

    nv_cached = false;

    // Reset our stats to normal levels
    // Any persistent buffs/debuffs will take place in effects,
    // player::suffer(), etc.

    // repopulate the stat fields
    str_cur = str_max + get_str_bonus();
    dex_cur = dex_max + get_dex_bonus();
    per_cur = per_max + get_per_bonus();
    int_cur = int_max + get_int_bonus();

    // Floor for our stats.  No stat changes should occur after this!
    if( dex_cur < 0 ) {
        dex_cur = 0;
    }
    if( str_cur < 0 ) {
        str_cur = 0;
    }
    if( per_cur < 0 ) {
        per_cur = 0;
    }
    if( int_cur < 0 ) {
        int_cur = 0;
    }
}

void Character::reset()
{
    // TODO: Move reset_stats here, remove it from Creature
    Creature::reset();
}

bool Character::has_nv()
{
    static bool nv = false;

    if( !nv_cached ) {
        nv_cached = true;
        nv = (worn_with_flag("GNV_EFFECT") ||
              has_active_bionic( bionic_id( "bio_night_vision" ) ) );
    }

    return nv;
}

void Character::reset_encumbrance()
{
    encumbrance_cache = calc_encumbrance();
}

std::array<encumbrance_data, num_bp> Character::calc_encumbrance() const
{
    return calc_encumbrance( item() );
}

std::array<encumbrance_data, num_bp> Character::calc_encumbrance( const item &new_item ) const
{

    std::array<encumbrance_data, num_bp> enc;

    item_encumb( enc, new_item );
    mut_cbm_encumb( enc );

    return enc;
}

units::mass Character::get_weight() const
{
    units::mass ret = 0;
    units::mass wornWeight = std::accumulate( worn.begin(), worn.end(), units::mass( 0 ),
                     []( units::mass sum, const item &itm ) {
                        return sum + itm.weight();
                     } );

    ret += Creature::get_weight(); // The base weight of the player's body
    ret += inv.weight();           // Weight of the stored inventory
    ret += wornWeight;             // Weight of worn items
    ret += weapon.weight();        // Weight of wielded item
    return ret;
}

std::array<encumbrance_data, num_bp> Character::get_encumbrance() const
{
    return encumbrance_cache;
}

std::array<encumbrance_data, num_bp> Character::get_encumbrance( const item &new_item ) const
{
    return calc_encumbrance( new_item );
}

int Character::extraEncumbrance( const layer_level level, const int bp ) const
{
    return encumbrance_cache[bp].layer_penalty_details[static_cast<int>( level )].total;
}

void layer_item( std::array<encumbrance_data, num_bp> &vals,
                 const item &it, bool power_armor )
{
    const auto item_layer = it.get_layer();
    int encumber_val = it.get_encumber();
    // For the purposes of layering penalty, set a min of 2 and a max of 10 per item.
    int layering_encumbrance = std::min( 10, std::max( 2, encumber_val ) );

    const int armorenc = !power_armor || !it.is_power_armor() ?
        encumber_val : std::max( 0, encumber_val - 40 );

    for( const body_part bp : all_body_parts ) {
        if( !it.covers( bp ) ) {
            continue;
        }

        vals[bp].layer( static_cast<layer_level>( item_layer ), layering_encumbrance );

        vals[bp].armor_encumbrance += armorenc;
    }
}

bool Character::is_wearing_active_power_armor() const
{
    for( const auto &w : worn ) {
        if( w.is_power_armor() && w.active ) {
            return true;
        }
    }
    return false;
}

void layer_details::reset() {
    *this = layer_details();
}

// The stacking penalty applies by doubling the encumbrance of
// each item except the highest encumbrance one.
// So we add them together and then subtract out the highest.
int layer_details::layer( const int encumbrance ) {
    pieces.push_back( encumbrance );

    int current = total;
    if( encumbrance > max ) {
        total += max;   // *now* the old max is counted, just ignore the new max
        max = encumbrance;
    } else {
        total += encumbrance;
    }
    return total - current;
}

/*
 * Encumbrance logic:
 * Some clothing is intrinsically encumbering, such as heavy jackets, backpacks, body armor, etc.
 * These simply add their encumbrance value to each body part they cover.
 * In addition, each article of clothing after the first in a layer imposes an additional penalty.
 * e.g. one shirt will not encumber you, but two is tight and starts to restrict movement.
 * Clothes on separate layers don't interact, so if you wear e.g. a light jacket over a shirt,
 * they're intended to be worn that way, and don't impose a penalty.
 * The default is to assume that clothes do not fit, clothes that are "fitted" either
 * reduce the encumbrance penalty by ten, or if that is already 0, they reduce the layering effect.
 *
 * Use cases:
 * What would typically be considered normal "street clothes" should not be considered encumbering.
 * T-shirt, shirt, jacket on torso/arms, underwear and pants on legs, socks and shoes on feet.
 * This is currently handled by each of these articles of clothing
 * being on a different layer and/or body part, therefore accumulating no encumbrance.
 */
void Character::item_encumb( std::array<encumbrance_data, num_bp> &vals,
                             const item &new_item ) const
{

    // reset all layer data
    vals = std::array<encumbrance_data, num_bp>();

    const bool power_armored = is_wearing_active_power_armor();
    for( auto& w : worn ) {
        layer_item( vals, w, power_armored );
    }

    if( !new_item.is_null() ) {
        layer_item( vals, new_item, power_armored );
    }

    // make sure values are sane
    for( const body_part bp : all_body_parts ) {
        encumbrance_data &elem = vals[bp];

        elem.armor_encumbrance = std::max( 0, elem.armor_encumbrance );

        // Add armor and layering penalties for the final values
        elem.encumbrance += elem.armor_encumbrance + elem.layer_penalty;
    }
}

int Character::encumb( body_part bp ) const
{
    return encumbrance_cache[bp].encumbrance;
}

void apply_mut_encumbrance( std::array<encumbrance_data, num_bp> &vals,
                            const mutation_branch &mut,
                            const body_part_set &oversize )
{
    for( const auto &enc : mut.encumbrance_always ) {
        vals[enc.first].encumbrance += enc.second;
    }

    for( const auto &enc : mut.encumbrance_covered ) {
        if( !oversize.test( enc.first ) ) {
            vals[enc.first].encumbrance += enc.second;
        }
    }
}

void Character::mut_cbm_encumb( std::array<encumbrance_data, num_bp> &vals ) const
{
    if( has_bionic( bionic_id( "bio_stiff" ) ) ) {
        // All but head, mouth and eyes
        for( auto &val : vals ) {
            val.encumbrance += 10;
        }

        vals[bp_head].encumbrance -= 10;
        vals[bp_mouth].encumbrance -= 10;
        vals[bp_eyes].encumbrance -= 10;
    }

    if( has_bionic( bionic_id( "bio_nostril" ) ) ) {
        vals[bp_mouth].encumbrance += 10;
    }
    if( has_bionic( bionic_id( "bio_thumbs" ) ) ) {
        vals[bp_hand_l].encumbrance += 10;
        vals[bp_hand_r].encumbrance += 10;
    }
    if( has_bionic( bionic_id( "bio_pokedeye" ) ) ) {
        vals[bp_eyes].encumbrance += 10;
    }

    // Lower penalty for bps covered only by XL armor
    const auto oversize = exclusive_flag_coverage( "OVERSIZE" );
    for( const auto &mut_pair : my_mutations ) {
        apply_mut_encumbrance( vals, *mut_pair.first, oversize );
    }
}

body_part_set Character::exclusive_flag_coverage( const std::string &flag ) const
{
    body_part_set ret = body_part_set::all();

    for( const auto &elem : worn ) {
        if( !elem.has_flag( flag ) ) {
            // Unset the parts covered by this item
            ret &= ~elem.get_covered_body_parts();
        }
    }

    return ret;
}

/*
 * Innate stats getters
 */

// get_stat() always gets total (current) value, NEVER just the base
// get_stat_bonus() is always just the bonus amount
int Character::get_str() const
{
    return std::max(0, str_max + str_bonus);
}
int Character::get_dex() const
{
    return std::max(0, dex_max + dex_bonus);
}
int Character::get_per() const
{
    return std::max(0, per_max + per_bonus);
}
int Character::get_int() const
{
    return std::max(0, int_max + int_bonus);
}

int Character::get_str_base() const
{
    return str_max;
}
int Character::get_dex_base() const
{
    return dex_max;
}
int Character::get_per_base() const
{
    return per_max;
}
int Character::get_int_base() const
{
    return int_max;
}

int Character::get_str_bonus() const
{
    return str_bonus;
}
int Character::get_dex_bonus() const
{
    return dex_bonus;
}
int Character::get_per_bonus() const
{
    return per_bonus;
}
int Character::get_int_bonus() const
{
    return int_bonus;
}

int Character::ranged_dex_mod() const
{
    ///\EFFECT_DEX <20 increases ranged penalty
    return std::max( ( 20.0 - get_dex() ) * 0.5, 0.0 );
}

int Character::ranged_per_mod() const
{
    ///\EFFECT_PER <20 increases ranged aiming penalty.
    return std::max( ( 20.0 - get_per() ) * 1.0, 0.0 );
}

int Character::get_healthy() const
{
    return healthy;
}
int Character::get_healthy_mod() const
{
    return healthy_mod;
}

/*
 * Innate stats setters
 */

void Character::set_str_bonus(int nstr)
{
    str_bonus = nstr;
}
void Character::set_dex_bonus(int ndex)
{
    dex_bonus = ndex;
}
void Character::set_per_bonus(int nper)
{
    per_bonus = nper;
}
void Character::set_int_bonus(int nint)
{
    int_bonus = nint;
}
void Character::mod_str_bonus(int nstr)
{
    str_bonus += nstr;
}
void Character::mod_dex_bonus(int ndex)
{
    dex_bonus += ndex;
}
void Character::mod_per_bonus(int nper)
{
    per_bonus += nper;
}
void Character::mod_int_bonus(int nint)
{
    int_bonus += nint;
}

void Character::set_healthy(int nhealthy)
{
    healthy = nhealthy;
}
void Character::mod_healthy(int nhealthy)
{
    healthy += nhealthy;
}
void Character::set_healthy_mod(int nhealthy_mod)
{
    healthy_mod = nhealthy_mod;
}
void Character::mod_healthy_mod(int nhealthy_mod, int cap)
{
    // TODO: This really should be a full morale-like system, with per-effect caps
    //       and durations.  This version prevents any single effect from exceeding its
    //       intended ceiling, but multiple effects will overlap instead of adding.

    // Cap indicates how far the mod is allowed to shift in this direction.
    // It can have a different sign to the mod, e.g. for items that treat
    // extremely low health, but can't make you healthy.
    if( nhealthy_mod == 0 || cap == 0 ) {
        return;
    }
    int low_cap;
    int high_cap;
    if( nhealthy_mod < 0 ) {
        low_cap = cap;
        high_cap = 200;
    } else {
        low_cap = -200;
        high_cap = cap;
    }

    // If we're already out-of-bounds, we don't need to do anything.
    if( (healthy_mod <= low_cap && nhealthy_mod < 0) ||
        (healthy_mod >= high_cap && nhealthy_mod > 0) ) {
        return;
    }

    healthy_mod += nhealthy_mod;

    // Since we already bailed out if we were out-of-bounds, we can
    // just clamp to the boundaries here.
    healthy_mod = std::min( healthy_mod, high_cap );
    healthy_mod = std::max( healthy_mod, low_cap );
}

int Character::get_hunger() const
{
    return hunger;
}

void Character::mod_hunger(int nhunger)
{
    set_hunger( hunger + nhunger );
}

void Character::set_hunger(int nhunger)
{
    if( hunger != nhunger ) {
        // cap hunger at 300, just below famished
        hunger = std::min(300, nhunger);
        on_stat_change( "hunger", hunger );
    }
}

int Character::get_starvation() const
{
    return starvation;
}

void Character::mod_starvation(int nstarvation)
{
    set_starvation( starvation + nstarvation );
}

void Character::set_starvation(int nstarvation)
{
    if( starvation != nstarvation ) {
        starvation = std::max(0, nstarvation);
        on_stat_change( "starvation", starvation );
    }
}

int Character::get_thirst() const
{
    return thirst;
}

void Character::mod_thirst(int nthirst)
{
    set_thirst( thirst + nthirst );
}

void Character::set_thirst(int nthirst)
{
    if( thirst != nthirst ) {
        thirst = nthirst;
        on_stat_change( "thirst", thirst );
    }
}

int Character::get_stomach_food() const
{
    return stomach_food;
}
void Character::mod_stomach_food(int n_stomach_food)
{
    stomach_food = std::max(0, stomach_food + n_stomach_food);
}
void Character::set_stomach_food(int n_stomach_food)
{
    stomach_food = std::max(0, n_stomach_food);
}
int Character::get_stomach_water() const
{
    return stomach_water;
}
void Character::mod_stomach_water(int n_stomach_water)
{
    stomach_water = std::max(0, stomach_water + n_stomach_water);
}
void Character::set_stomach_water(int n_stomach_water)
{
    stomach_water = std::max(0, n_stomach_water);
}

void Character::mod_fatigue(int nfatigue)
{
    set_fatigue(fatigue + nfatigue);
}

void Character::set_fatigue(int nfatigue)
{
    nfatigue = std::max( nfatigue, -1000 );
    if( fatigue != nfatigue ) {
        fatigue = nfatigue;
        on_stat_change( "fatigue", fatigue );
    }
}

int Character::get_fatigue() const
{
    return fatigue;
}

void Character::reset_bonuses()
{
    // Reset all bonuses to 0 and multipliers to 1.0
    str_bonus = 0;
    dex_bonus = 0;
    per_bonus = 0;
    int_bonus = 0;

    reset_encumbrance();

    Creature::reset_bonuses();
}

void Character::update_health(int external_modifiers)
{
    if( has_artifact_with( AEP_SICK ) ) {
        // Carrying a sickness artifact makes your health 50 points worse on average
        external_modifiers -= 50;
    }
    // Limit healthy_mod to [-200, 200].
    // This also sets approximate bounds for the character's health.
    if( get_healthy_mod() > 200 ) {
        set_healthy_mod( 200 );
    } else if( get_healthy_mod() < -200 ) {
        set_healthy_mod( -200 );
    }

    // Active leukocyte breeder will keep your health near 100
    int effective_healthy_mod = get_healthy_mod();
    if( has_active_bionic( bionic_id( "bio_leukocyte" ) ) ) {
        // Side effect: dependency
        mod_healthy_mod( -50, -200 );
        effective_healthy_mod = 100;
    }

    // Health tends toward healthy_mod.
    // For small differences, it changes 4 points per day
    // For large ones, up to ~40% of the difference per day
    int health_change = effective_healthy_mod - get_healthy() + external_modifiers;
    mod_healthy( sgn( health_change ) * std::max( 1, abs( health_change ) / 10 ) );

    // And healthy_mod decays over time.
    // Slowly near 0, but it's hard to overpower it near +/-100
    set_healthy_mod( round( get_healthy_mod() * 0.95f ) );

    add_msg( m_debug, "Health: %d, Health mod: %d", get_healthy(), get_healthy_mod() );
}

float Character::get_dodge_base() const
{
    /** @EFFECT_DEX increases dodge base */
    /** @EFFECT_DODGE increases dodge_base */
    return get_dex() / 2.0f + get_skill_level( skill_dodge );
}
float Character::get_hit_base() const
{
    /** @EFFECT_DEX increases hit base, slightly */
    return get_dex() / 4.0f;
}

hp_part Character::body_window( bool precise ) const
{
    return body_window( disp_name(), true, precise, 0, 0, 0, 0, 0, 0, 0, 0 );
}

hp_part Character::body_window( const std::string &menu_header,
                                bool show_all, bool precise,
                                int normal_bonus, int head_bonus, int torso_bonus,
                                bool bleed, bool bite, bool infect, bool is_bandage, bool is_disinfectant ) const
{
    catacurses::window hp_window = catacurses::newwin( 10, 31, ( TERMY - 10 ) / 2, ( TERMX - 31 ) / 2 );
    draw_border(hp_window);

    trim_and_print( hp_window, 1, 1, getmaxx(hp_window) - 2, c_light_red, menu_header.c_str() );
    const int y_off = 2; // 1 for border, 1 for header

    /* This struct establishes some kind of connection between the hp_part (which can be healed and
     * have HP) and the body_part. Note that there are more body_parts than hp_parts. For example:
     * Damage to bp_head, bp_eyes and bp_mouth is all applied on the HP of hp_head. */
    struct healable_bp {
        mutable bool allowed;
        body_part bp;
        hp_part hp;
        std::string name; // Translated name as it appears in the menu.
        int bonus;
    };
    /* The array of the menu entries show to the player. The entries are displayed in this order,
     * it may be changed here. */
    std::array<healable_bp, num_hp_parts> parts = { {
        { false, bp_head, hp_head, _("Head"), head_bonus },
        { false, bp_torso, hp_torso, _("Torso"), torso_bonus },
        { false, bp_arm_l, hp_arm_l, _("Left Arm"), normal_bonus },
        { false, bp_arm_r, hp_arm_r, _("Right Arm"), normal_bonus },
        { false, bp_leg_l, hp_leg_l, _("Left Leg"), normal_bonus },
        { false, bp_leg_r, hp_leg_r, _("Right Leg"), normal_bonus },
    } };

    for( size_t i = 0; i < parts.size(); i++ ) {
        const auto &e = parts[i];
        const body_part bp = e.bp;
        const hp_part hp = e.hp;
        const int maximal_hp = hp_max[hp];
        const int current_hp = hp_cur[hp];
        const int bonus = e.bonus;
        // This will c_light_gray if the part does not have any effects cured by the item
        // (e.g. it cures only bites, but the part does not have a bite effect)
        const nc_color state_col = limb_color( bp, bleed, bite, infect );
        const bool has_curable_effect = state_col != c_light_gray;
        // The same as in the main UI sidebar. Independent of the capability of the healing item!
        const nc_color all_state_col = limb_color( bp, true, true, true );
        const bool has_any_effect = all_state_col != c_light_gray;
        // Broken means no HP can be restored, it requires surgical attention.
        const bool limb_is_broken = current_hp == 0;
        // This considers only the effects that can *not* be removed.
        const nc_color new_state_col = limb_color( bp, !bleed, !bite, !infect );

        if( show_all ) {
            e.allowed = true;
        } else if( has_curable_effect ) {
            e.allowed = true;
        } else if( limb_is_broken ) {
            continue;
        } else if( current_hp < maximal_hp && ( e.bonus != 0 || is_bandage || is_disinfectant ) ) {
            e.allowed = true;
        } else {
            continue;
        }

        const int line = i + y_off;

        mvwprintz( hp_window, line, 1, all_state_col, "%d: %s", i + 1, e.name.c_str() );

        const auto print_hp = [&]( const int x, const nc_color col, const int hp ) {
            const auto bar = get_hp_bar( hp, maximal_hp, false );
            if( hp == 0 ) {
                mvwprintz( hp_window, line, x, col, "-----" );
            } else if( precise ) {
                mvwprintz( hp_window, line, x, col, "%5d", hp );
            } else {
                mvwprintz( hp_window, line, x, col, bar.first.c_str() );
            }
        };

        if( !limb_is_broken ) {
            // Drop the bar color, use the state color instead
            const nc_color color = has_any_effect ? all_state_col : c_green;
            print_hp( 15, color, current_hp );
        } else {
            // But still could be infected or bleeding
            const nc_color color = has_any_effect ? all_state_col : c_dark_gray;
            print_hp( 15, color, 0 );
        }

        if( !limb_is_broken ) {
            const int new_hp = std::max( 0, std::min( maximal_hp, current_hp + bonus ) );

            if( new_hp == current_hp && !has_curable_effect ) {
                // Nothing would change
                continue;
            }

            mvwprintz( hp_window, line, 20, c_dark_gray, " -> " );

            const nc_color color = has_any_effect ? new_state_col : c_green;
            print_hp( 24, color, new_hp );
        } else {
            const nc_color color = has_any_effect ? new_state_col : c_dark_gray;
            mvwprintz( hp_window, line, 20, c_dark_gray, " -> " );
            print_hp( 24, color, 0 );
        }
    }
    mvwprintz( hp_window, parts.size() + y_off, 1, c_light_gray, _("%d: Exit"), parts.size() + 1 );

    wrefresh(hp_window);
    char ch;
    hp_part healed_part = num_hp_parts;
    do {
        // TODO: use input context
        ch = inp_mngr.get_input_event().get_first_input();
        const size_t index = ch - '1';
        if( index < parts.size() && parts[index].allowed ) {
            healed_part = parts[index].hp;
            break;
        } else if( index == parts.size() || ch == KEY_ESCAPE) {
            healed_part = num_hp_parts;
            break;
        }
    } while (ch < '1' || ch > '7');
    catacurses::refresh();

    return healed_part;
}

nc_color Character::limb_color( body_part bp, bool bleed, bool bite, bool infect ) const
{
    if( bp == num_bp ) {
        return c_light_gray;
    }

    int color_bit = 0;
    nc_color i_color = c_light_gray;
    if( bleed && has_effect( effect_bleed, bp ) ) {
        color_bit += 1;
    }
    if( bite && has_effect( effect_bite, bp ) ) {
        color_bit += 10;
    }
    if( infect && has_effect( effect_infected, bp ) ) {
        color_bit += 100;
    }
    switch( color_bit ) {
    case 1:
        i_color = c_red;
        break;
    case 10:
        i_color = c_blue;
        break;
    case 100:
        i_color = c_green;
        break;
    case 11:
        i_color = c_magenta;
        break;
    case 101:
        i_color = c_yellow;
        break;
    }

    return i_color;
}

std::string Character::get_name() const
{
    return name;
}

nc_color Character::symbol_color() const
{
    nc_color basic = basic_symbol_color();

    if( has_effect( effect_downed ) ) {
        return hilite( basic );
    } else if( has_effect( effect_grabbed ) ) {
        return cyan_background( basic );
    }

    const auto &fields = g->m.field_at( pos() );

    bool has_fire = false;
    bool has_acid = false;
    bool has_elec = false;
    bool has_fume = false;
    for( const auto &field : fields ) {
        switch( field.first ) {
            case fd_incendiary:
            case fd_fire:
                has_fire = true;
                break;
            case fd_electricity:
                has_elec = true;
                break;
            case fd_acid:
                has_acid = true;
                break;
            case fd_relax_gas:
            case fd_fungal_haze:
            case fd_fungicidal_gas:
            case fd_toxic_gas:
            case fd_tear_gas:
            case fd_nuke_gas:
            case fd_smoke:
                has_fume = true;
                break;
            default:
                continue;
        }
    }

    // Priority: electricity, fire, acid, gases
    // Can't just return in the switch, because field order is alphabetic
    if( has_elec ) {
        return hilite( basic );
    } else if( has_fire ) {
        return red_background( basic );
    } else if( has_acid ) {
        return green_background( basic );
    } else if( has_fume ) {
        return white_background( basic );
    }

    if( in_sleep_state() ) {
        return hilite( basic );
    }

    return basic;
}

bool Character::is_immune_field( const field_id fid ) const
{
    // Obviously this makes us invincible
    if( has_trait( debug_nodmg ) ) {
        return true;
    }

    // Check to see if we are immune
    switch( fid ) {
        case fd_smoke:
            return get_env_resist( bp_mouth ) >= 12;
        case fd_tear_gas:
        case fd_toxic_gas:
        case fd_gas_vent:
        case fd_relax_gas:
            return get_env_resist( bp_mouth ) >= 15;
        case fd_fungal_haze:
            return has_trait( trait_id( "M_IMMUNE" ) ) || ( get_env_resist( bp_mouth ) >= 15 &&
                   get_env_resist( bp_eyes ) >= 15);
        case fd_electricity:
            return is_elec_immune();
        case fd_acid:
            return has_trait( trait_id( "ACIDPROOF" ) ) ||
                   (!is_on_ground() && get_env_resist( bp_foot_l ) >= 15 &&
                   get_env_resist( bp_foot_r ) >= 15 &&
                   get_env_resist( bp_leg_l ) >= 15 &&
                   get_env_resist( bp_leg_r ) >= 15 &&
                   get_armor_type( DT_ACID, bp_foot_l ) >= 5 &&
                   get_armor_type( DT_ACID, bp_foot_r ) >= 5 &&
                   get_armor_type( DT_ACID, bp_leg_l ) >= 5 &&
                   get_armor_type( DT_ACID, bp_leg_r ) >= 5 );
        case fd_web:
            return has_trait( trait_id( "WEB_WALKER" ) );
        case fd_fire:
        case fd_flame_burst:
            return has_trait( trait_id( "M_SKIN2" ) ) || has_active_bionic( bionic_id( "bio_heatsink" ) ) ||
                   is_wearing( "rm13_armor_on" );
        default:
            // Suppress warning
            break;
    }
    // If we haven't found immunity yet fall up to the next level
    return Creature::is_immune_field( fid );
}

int Character::throw_range( const item &it ) const
{
    if( it.is_null() ) {
        return -1;
    }

    item tmp = it;

    if( tmp.count_by_charges() && tmp.charges > 1 ) {
        tmp.charges = 1;
    }

    /** @EFFECT_STR determines maximum weight that can be thrown */
    if( ( tmp.weight() / 113_gram ) > int( str_cur * 15 ) ) {
        return 0;
    }
    // Increases as weight decreases until 150 g, then decreases again
    /** @EFFECT_STR increases throwing range, vs item weight (high or low) */
    int ret = ( str_cur * 10 ) / ( tmp.weight() >= 150_gram ? tmp.weight() / 113_gram : 10 - int( tmp.weight() / 15_gram ) );
    ret -= tmp.volume() / 1000_ml;
    static const std::set<material_id> affected_materials = { material_id( "iron" ), material_id( "steel" ) };
    if( has_active_bionic( bionic_id( "bio_railgun" ) ) && tmp.made_of_any( affected_materials ) ) {
        ret *= 2;
    }
    if( ret < 1 ) {
        return 1;
    }
    // Cap at double our strength + skill
    /** @EFFECT_STR caps throwing range */

    /** @EFFECT_THROW caps throwing range */
    if( ret > str_cur * 3 + get_skill_level( skill_throw ) ) {
        return str_cur * 3 + get_skill_level( skill_throw );
    }

    return ret;
}

bool Character::made_of( const material_id &m ) const {
    // TODO: check for mutations that change this.
    static const std::vector<material_id> fleshy = { material_id( "flesh" ), material_id( "hflesh" ) };
    return std::find( fleshy.begin(), fleshy.end(), m ) != fleshy.end();
}

bool Character::is_blind() const
{
    return ( worn_with_flag( "BLIND" ) ||
             has_effect( effect_blind ) ||
             has_active_bionic( bionic_id( "bio_blindfold" ) ) );
}

bool Character::pour_into( item &container, item &liquid )
{
    std::string err;
    const long amount = container.get_remaining_capacity_for_liquid( liquid, *this, &err );

    if( !err.empty() ) {
        add_msg_if_player( m_bad, err.c_str() );
        return false;
    }

    add_msg_if_player( _( "You pour %1$s into the %2$s." ), liquid.tname().c_str(),
                       container.tname().c_str() );

    container.fill_with( liquid, amount );
    inv.unsort();

    if( liquid.charges > 0 ) {
        add_msg_if_player( _( "There's some left over!" ) );
    }

    return true;
}

bool Character::pour_into( vehicle &veh, item &liquid )
{
    auto sel = [&]( const vehicle_part &pt ) {
        return pt.is_tank() && pt.can_reload( liquid.typeId() );
    };

    auto stack = units::legacy_volume_factor / liquid.type->stack_size;
    auto title = string_format( _( "Select target tank for <color_%s>%.1fL %s</color>" ),
                                get_all_colors().get_name( liquid.color() ).c_str(),
                                round_up( to_liter( liquid.charges * stack ), 1 ),
                                liquid.tname().c_str() );

    auto &tank = veh_interact::select_part( veh, sel, title );
    if( !tank ) {
        return false;
    }

    tank.fill_with( liquid );

    //~ $1 - vehicle name, $2 - part name, $3 - liquid type
    add_msg_if_player( _( "You refill the %1$s's %2$s with %3$s." ),
                       veh.name.c_str(), tank.name().c_str(), liquid.type_name().c_str() );

    if( liquid.charges > 0 ) {
        add_msg_if_player( _( "There's some left over!" ) );
    }
    return true;
}

resistances Character::mutation_armor( body_part bp ) const
{
    resistances res;
    for( auto &iter : my_mutations ) {
        res += iter.first->damage_resistance( bp );
    }

    return res;
}

float Character::mutation_armor( body_part bp, damage_type dt ) const
{
    return mutation_armor( bp ).type_resist( dt );
}

float Character::mutation_armor( body_part bp, const damage_unit &du ) const
{
    return mutation_armor( bp ).get_effective_resist( du );
}

long Character::ammo_count_for( const item &gun )
{
    long ret = item::INFINITE_CHARGES;
    if( !gun.is_gun() ) {
        return ret;
    }

    long required = gun.ammo_required();

    if( required > 0 ) {
        long total_ammo = 0;
        total_ammo += gun.ammo_remaining();

        bool has_mag = gun.magazine_integral();

        const auto found_ammo = find_ammo( gun, true, -1 );
        long loose_ammo = 0;
        for( const auto &ammo : found_ammo ) {
            if( ammo->is_magazine() ) {
                has_mag = true;
                total_ammo += ammo->ammo_remaining();
            } else if( ammo->is_ammo() ) {
                loose_ammo += ammo->charges;
            }
        }

        if( has_mag ) {
            total_ammo += loose_ammo;
        }

        ret = std::min<long>( ret, total_ammo / required );
    }

    long ups_drain = gun.get_gun_ups_drain();
    if( ups_drain > 0 ) {
        ret = std::min<long>( ret, charges_of( "UPS" ) / ups_drain );
    }

    return ret;
}

float Character::rest_quality() const
{
    // Just a placeholder for now.
    // @todo: Waiting/reading/being unconscious on bed/sofa/grass
    return has_effect( effect_sleep ) ? 1.0f : 0.0f;
}

hp_part Character::bp_to_hp( const body_part bp )
{
    switch(bp) {
        case bp_head:
        case bp_eyes:
        case bp_mouth:
            return hp_head;
        case bp_torso:
            return hp_torso;
        case bp_arm_l:
        case bp_hand_l:
            return hp_arm_l;
        case bp_arm_r:
        case bp_hand_r:
            return hp_arm_r;
        case bp_leg_l:
        case bp_foot_l:
            return hp_leg_l;
        case bp_leg_r:
        case bp_foot_r:
            return hp_leg_r;
        default:
            return num_hp_parts;
    }
}

body_part Character::hp_to_bp( const hp_part hpart )
{
    switch(hpart) {
        case hp_head:
            return bp_head;
        case hp_torso:
            return bp_torso;
        case hp_arm_l:
            return bp_arm_l;
        case hp_arm_r:
            return bp_arm_r;
        case hp_leg_l:
            return bp_leg_l;
        case hp_leg_r:
            return bp_leg_r;
        default:
            return num_bp;
    }
}

body_part Character::get_random_body_part( bool main ) const
{
    // TODO: Refuse broken limbs, adjust for mutations
    return random_body_part( main );
}

std::vector<body_part> Character::get_all_body_parts( bool main ) const
{
    // @todo: Remove broken parts, parts removed by mutations etc.
    static const std::vector<body_part> all_bps = {{
        bp_head,
        bp_eyes,
        bp_mouth,
        bp_torso,
        bp_arm_l,
        bp_arm_r,
        bp_hand_l,
        bp_hand_r,
        bp_leg_l,
        bp_leg_r,
        bp_foot_l,
        bp_foot_r,
    }};

    static const std::vector<body_part> main_bps = {{
        bp_head,
        bp_torso,
        bp_arm_l,
        bp_arm_r,
        bp_leg_l,
        bp_leg_r,
    }};

    return main ? main_bps : all_bps;
}

// @todo: Better place for it?
std::string tag_colored_string( const std::string &s, nc_color color )
{
    // @todo: Make this tag generation a function, put it in good place
    std::string color_tag_open = "<color_" + string_from_color( color ) + ">";
    return color_tag_open + s;
}

std::string Character::extended_description() const
{
    std::ostringstream ss;
    if( is_player() ) {
        // <bad>This is me, <player_name>.</bad>
        ss << string_format( _( "This is you - %s." ), name.c_str() );
    } else {
        ss << string_format( _( "This is %s." ), name.c_str() );
    }

    ss << std::endl << "--" << std::endl;

    const auto &bps = get_all_body_parts( true );
    // Find length of bp names, to align
    // accumulate looks weird here, any better function?
    size_t longest = std::accumulate( bps.begin(), bps.end(), ( size_t )0, []( size_t m, body_part bp ) {
        return std::max( m, body_part_name_as_heading( bp, 1 ).size() );
    } );

    // This is a stripped-down version of the body_window function
    // This should be extracted into a separate function later on
    for( body_part bp : bps ) {
        const std::string &bp_heading = body_part_name_as_heading( bp, 1 );
        hp_part hp = bp_to_hp( bp );

        const int maximal_hp = hp_max[hp];
        const int current_hp = hp_cur[hp];
        const nc_color state_col = limb_color( bp, true, true, true );
        nc_color name_color = state_col;
        auto hp_bar = get_hp_bar( current_hp, maximal_hp, false );

        ss << tag_colored_string( bp_heading, name_color );
        // Align them. There is probably a less ugly way to do it
        ss << std::string( longest - bp_heading.size() + 1, ' ' );
        ss << tag_colored_string( hp_bar.first, hp_bar.second );
        // Trailing bars. UGLY!
        // @todo: Integrate into get_hp_bar somehow
        ss << tag_colored_string( std::string( 5 - hp_bar.first.size(), '.' ), c_white );
        ss << std::endl;
    }

    ss << "--" << std::endl;
    ss << _( "Wielding:" ) << " ";
    if( weapon.is_null() ) {
        ss << _( "Nothing" );
    } else {
        ss << weapon.tname();
    }

    ss << std::endl;
    ss << _( "Wearing:" ) << " ";
    ss << enumerate_as_string( worn.begin(), worn.end(), []( const item &it ) {
        return it.tname();
    } );

    return replace_colors( ss.str() );
}

const social_modifiers Character::get_mutation_social_mods() const
{
    social_modifiers mods;
    for( const mutation_branch *mut : cached_mutations ) {
        mods += mut->social_mods;
    }

    return mods;
}

template <float mutation_branch::*member>
float calc_mutation_value( const std::vector<const mutation_branch *> &mutations )
{
    float lowest = 0.0f;
    float highest = 0.0f;
    for( const mutation_branch *mut : mutations ) {
        float val = mut->*member;
        lowest = std::min( lowest, val );
        highest = std::max( highest, val );
    }

    return std::min( 0.0f, lowest ) + std::max( 0.0f, highest );
}

float Character::mutation_value( const std::string &val ) const
{
    // Syntax similar to tuple get<n>()
    // @todo: Get rid of if/else ladder
    if( val == "healing_awake" ) {
        return calc_mutation_value<&mutation_branch::healing_awake>( cached_mutations );
    } else if( val == "healing_resting" ) {
        return calc_mutation_value<&mutation_branch::healing_resting>( cached_mutations );
    } else if( val == "hp_modifier" ) {
        return calc_mutation_value<&mutation_branch::hp_modifier>( cached_mutations );
    } else if( val == "hp_modifier_secondary" ) {
        return calc_mutation_value<&mutation_branch::hp_modifier_secondary>( cached_mutations );
    } else if( val == "hp_adjustment" ) {
        return calc_mutation_value<&mutation_branch::hp_adjustment>( cached_mutations );
    } else if( val == "metabolism_modifier" ) {
        return calc_mutation_value<&mutation_branch::metabolism_modifier>( cached_mutations );
    } else if( val == "thirst_modifier" ) {
        return calc_mutation_value<&mutation_branch::thirst_modifier>( cached_mutations );
    } else if( val == "fatigue_regen_modifier" ) {
        return calc_mutation_value<&mutation_branch::fatigue_regen_modifier>( cached_mutations );
    } else if( val == "fatigue_modifier" ) {
        return calc_mutation_value<&mutation_branch::fatigue_modifier>( cached_mutations );
    } else if( val == "stamina_regen_modifier" ) {
        return calc_mutation_value<&mutation_branch::stamina_regen_modifier>( cached_mutations );
    }

    debugmsg( "Invalid mutation value name %s", val.c_str() );
    return 0.0f;
}

float Character::healing_rate( float at_rest_quality ) const
{
    // @todo: Cache
    float heal_rate;
    if( !is_npc() ){
        heal_rate = get_option< float >( "PLAYER_HEALING_RATE" );
    } else {
        heal_rate = get_option< float >( "NPC_HEALING_RATE" );
    }
    float awake_rate = heal_rate * mutation_value( "healing_awake" );
    float final_rate = 0.0f;
    if( awake_rate > 0.0f ) {
        final_rate += awake_rate;
    } else if( at_rest_quality < 1.0f ) {
        // Resting protects from rot
        final_rate += ( 1.0f - at_rest_quality ) * awake_rate;
    }
    float asleep_rate = 0.0f;
    if( at_rest_quality > 0.0f ) {
        asleep_rate = at_rest_quality * heal_rate * ( 1.0f + mutation_value( "healing_resting" ) );
    }
    if( asleep_rate > 0.0f ) {
        final_rate += asleep_rate * ( 1.0f + get_healthy() / 200.0f );
    }

    // Most common case: awake player with no regenerative abilities
    // ~7e-5 is 1 hp per day, anything less than that is totally negligible
    static constexpr float eps = 0.000007f;
    add_msg( m_debug, "%s healing: %.6f", name.c_str(), final_rate );
    if( std::abs( final_rate ) < eps ) {
        return 0.0f;
    }

    float primary_hp_mod = mutation_value( "hp_modifier" );
    if( primary_hp_mod < 0.0f ) {
        // HP mod can't get below -1.0
        final_rate *= 1.0f + primary_hp_mod;
    }

    return final_rate;
}

float Character::healing_rate_medicine( float at_rest_quality, const body_part bp ) const
{
    float rate_medicine = 0.0f;
    float bandaged_rate = 0.0f;
    float disinfected_rate = 0.0f;

    const effect &e_bandaged = get_effect( effect_bandaged, bp );
    const effect &e_disinfected = get_effect( effect_disinfected, bp );

    if( !e_bandaged.is_null() ) {
        bandaged_rate += static_cast<float>( e_bandaged.get_amount( "HEAL_RATE", 0 ) ) / HOURS( 24 );
        if( bp == bp_head ) {
            bandaged_rate *= e_bandaged.get_amount( "HEAL_HEAD", 0 ) / 100.0f;
        }
        if( bp == bp_torso ) {
            bandaged_rate *= e_bandaged.get_amount( "HEAL_TORSO", 0 ) / 100.0f;
        }
    }

    if( !e_disinfected.is_null() ) {
        disinfected_rate += static_cast<float>( e_disinfected.get_amount( "HEAL_RATE", 0 ) ) / HOURS( 24 );
        if( bp == bp_head ) {
            disinfected_rate *= e_disinfected.get_amount( "HEAL_HEAD", 0 ) / 100.0f;
        }
        if( bp == bp_torso ) {
            disinfected_rate *= e_disinfected.get_amount( "HEAL_TORSO", 0 ) / 100.0f;
        }
    }

    rate_medicine += bandaged_rate + disinfected_rate;
    rate_medicine *= 1.0f + mutation_value( "healing_resting" );
    rate_medicine *= 1.0f + at_rest_quality;

    // increase healing if character has both effects
    if( !e_bandaged.is_null() && !e_disinfected.is_null() ){
        rate_medicine *= 2;
    }

    if( get_healthy() > 0.0f ) {
        rate_medicine *= 1.0f + get_healthy() / 200.0f;
    } else {
        rate_medicine *= 1.0f + get_healthy() / 400.0f;
    }
    float primary_hp_mod = mutation_value( "hp_modifier" );
    if( primary_hp_mod < 0.0f ) {
        // HP mod can't get below -1.0
        rate_medicine *= 1.0f + primary_hp_mod;
    }
    return rate_medicine;
}
