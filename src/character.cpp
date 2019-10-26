#include "character.h"

#include <ctype.h>
#include <climits>
#include <cstdlib>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <cmath>
#include <iterator>
#include <memory>

#include "activity_handlers.h"
#include "avatar.h"
#include "bionics.h"
#include "cata_utility.h"
#include "construction.h"
#include "coordinate_conversions.h"
#include "debug.h"
#include "effect.h"
#include "event_bus.h"
#include "field.h"
#include "fungal_effects.h"
#include "game.h"
#include "itype.h"
#include "npc.h"
#include "material.h"
#include "map.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "memorial_logger.h"
#include "messages.h"
#include "mission.h"
#include "monster.h"
#include "morale.h"
#include "morale_types.h"
#include "mtype.h"
#include "mutation.h"
#include "options.h"
#include "output.h"
#include "overlay_ordering.h"
#include "pathfinding.h"
#include "player.h"
#include "skill.h"
#include "skill_boost.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "trap.h"
#include "veh_interact.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "catacharset.h"
#include "game_constants.h"
#include "item_location.h"
#include "lightmap.h"
#include "rng.h"
#include "stomach.h"
#include "ui.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vitamin.h"
#include "vpart_position.h"

static const bionic_id bio_ads( "bio_ads" );
static const bionic_id bio_armor_arms( "bio_armor_arms" );
static const bionic_id bio_armor_eyes( "bio_armor_eyes" );
static const bionic_id bio_armor_head( "bio_armor_head" );
static const bionic_id bio_armor_legs( "bio_armor_legs" );
static const bionic_id bio_armor_torso( "bio_armor_torso" );
static const bionic_id bio_carbon( "bio_carbon" );
static const bionic_id bio_gills( "bio_gills" );
static const bionic_id bio_laser( "bio_laser" );
static const bionic_id bio_lighter( "bio_lighter" );
static const bionic_id bio_tools( "bio_tools" );

const efftype_id effect_adrenaline( "adrenaline" );
const efftype_id effect_alarm_clock( "alarm_clock" );
const efftype_id effect_bandaged( "bandaged" );
const efftype_id effect_beartrap( "beartrap" );
const efftype_id effect_bite( "bite" );
const efftype_id effect_bleed( "bleed" );
const efftype_id effect_blind( "blind" );
const efftype_id effect_boomered( "boomered" );
const efftype_id effect_contacts( "contacts" );
const efftype_id effect_controlled( "controlled" );
const efftype_id effect_crushed( "crushed" );
const efftype_id effect_darkness( "darkness" );
const efftype_id effect_disinfected( "disinfected" );
const efftype_id effect_downed( "downed" );
const efftype_id effect_drunk( "drunk" );
const efftype_id effect_foodpoison( "foodpoison" );
const efftype_id effect_grabbed( "grabbed" );
const efftype_id effect_grabbing( "grabbing" );
const efftype_id effect_harnessed( "harnessed" );
const efftype_id effect_heavysnare( "heavysnare" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_in_pit( "in_pit" );
const efftype_id effect_lightsnare( "lightsnare" );
const efftype_id effect_lying_down( "lying_down" );
const efftype_id effect_narcosis( "narcosis" );
const efftype_id effect_nausea( "nausea" );
const efftype_id effect_no_sight( "no_sight" );
const efftype_id effect_onfire( "onfire" );
const efftype_id effect_pkill1( "pkill1" );
const efftype_id effect_pkill2( "pkill2" );
const efftype_id effect_pkill3( "pkill3" );
const efftype_id effect_ridden( "ridden" );
const efftype_id effect_riding( "riding" );
const efftype_id effect_sleep( "sleep" );
const efftype_id effect_slept_through_alarm( "slept_through_alarm" );
const efftype_id effect_tied( "tied" );
const efftype_id effect_took_prozac( "took_prozac" );
const efftype_id effect_took_xanax( "took_xanax" );
const efftype_id effect_webbed( "webbed" );
const efftype_id effect_winded( "winded" );

const skill_id skill_dodge( "dodge" );
const skill_id skill_throw( "throw" );

static const trait_id trait_ACIDBLOOD( "ACIDBLOOD" );
static const trait_id trait_ADRENALINE( "ADRENALINE" );
static const trait_id trait_BADBACK( "BADBACK" );
static const trait_id trait_BIRD_EYE( "BIRD_EYE" );
static const trait_id trait_CEPH_EYES( "CEPH_EYES" );
static const trait_id trait_CEPH_VISION( "CEPH_VISION" );
static const trait_id trait_DEBUG_CLOAK( "DEBUG_CLOAK" );
static const trait_id trait_DEBUG_NIGHTVISION( "DEBUG_NIGHTVISION" );
static const trait_id trait_DEBUG_NODMG( "DEBUG_NODMG" );
static const trait_id trait_DISORGANIZED( "DISORGANIZED" );
static const trait_id trait_ELFA_FNV( "ELFA_FNV" );
static const trait_id trait_ELFA_NV( "ELFA_NV" );
static const trait_id trait_FEL_NV( "FEL_NV" );
static const trait_id trait_PROF_FOODP( "PROF_FOODP" );
static const trait_id trait_GILLS( "GILLS" );
static const trait_id trait_GILLS_CEPH( "GILLS_CEPH" );
static const trait_id trait_GLASSJAW( "GLASSJAW" );
static const trait_id trait_HOARDER( "HOARDER" );
static const trait_id trait_HOLLOW_BONES( "HOLLOW_BONES" );
static const trait_id trait_LIGHT_BONES( "LIGHT_BONES" );
static const trait_id trait_MEMBRANE( "MEMBRANE" );
static const trait_id trait_MYOPIC( "MYOPIC" );
static const trait_id trait_NIGHTVISION2( "NIGHTVISION2" );
static const trait_id trait_NIGHTVISION3( "NIGHTVISION3" );
static const trait_id trait_NIGHTVISION( "NIGHTVISION" );
static const trait_id trait_NOMAD( "NOMAD" );
static const trait_id trait_NOMAD2( "NOMAD2" );
static const trait_id trait_NOMAD3( "NOMAD3" );
static const trait_id trait_PACKMULE( "PACKMULE" );
static const trait_id trait_PER_SLIME_OK( "PER_SLIME_OK" );
static const trait_id trait_PER_SLIME( "PER_SLIME" );
static const trait_id trait_ROOTS2( "ROOTS2" );
static const trait_id trait_ROOTS3( "ROOTS3" );
static const trait_id trait_SEESLEEP( "SEESLEEP" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_SHELL( "SHELL" );
static const trait_id trait_SHOUT2( "SHOUT2" );
static const trait_id trait_SHOUT3( "SHOUT3" );
static const trait_id trait_THRESH_CEPHALOPOD( "THRESH_CEPHALOPOD" );
static const trait_id trait_THRESH_INSECT( "THRESH_INSECT" );
static const trait_id trait_THRESH_PLANT( "THRESH_PLANT" );
static const trait_id trait_THRESH_SPIDER( "THRESH_SPIDER" );
static const trait_id trait_URSINE_EYE( "URSINE_EYE" );
static const trait_id debug_nodmg( "DEBUG_NODMG" );

const species_id HUMAN( "HUMAN" );

// *INDENT-OFF*
Character::Character() :

    visitable<Character>(),
    hp_cur( {{ 0 }} ),
    hp_max( {{ 0 }} ),
    damage_bandaged( {{ 0 }} ),
    damage_disinfected( {{ 0 }} ),
    id( -1 )
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
    thirst = 0;
    fatigue = 0;
    sleep_deprivation = 0;
    set_stim( 0 );
    set_stamina( 10000 ); //Temporary value for stamina. It will be reset later from external json option.
    pkill = 0;
    // 45 days to starve to death
    healthy_calories = 55000;
    stored_calories = healthy_calories;
    initialize_stomach_contents();
    healed_total = { { 0, 0, 0, 0, 0, 0 } };

    name.clear();

    *path_settings = pathfinding_settings{ 0, 1000, 1000, 0, true, false, true, false };

    move_mode = CMM_WALK;
}
// *INDENT-ON*

Character::~Character() = default;
Character::Character( Character && ) = default;
Character &Character::operator=( Character && ) = default;

void Character::setID( character_id i )
{
    if( id.is_valid() ) {
        debugmsg( "tried to set id of a npc/player, but has already a id: %d", id.get_value() );
    } else if( !i.is_valid() ) {
        debugmsg( "tried to set invalid id of a npc/player: %d", i.get_value() );
    } else {
        id = i;
    }
}

character_id Character::getID() const
{
    return this->id;
}

field_type_id Character::bloodType() const
{
    if( has_trait( trait_ACIDBLOOD ) ) {
        return fd_acid;
    }
    if( has_trait( trait_THRESH_PLANT ) ) {
        return fd_blood_veggy;
    }
    if( has_trait( trait_THRESH_INSECT ) || has_trait( trait_THRESH_SPIDER ) ) {
        return fd_blood_insect;
    }
    if( has_trait( trait_THRESH_CEPHALOPOD ) ) {
        return fd_blood_invertebrate;
    }
    return fd_blood;
}
field_type_id Character::gibType() const
{
    return fd_gibs_flesh;
}

bool Character::in_species( const species_id &spec ) const
{
    return spec == HUMAN;
}

bool Character::is_warm() const
{
    return true; // TODO: is there a mutation (plant?) that makes a npc not warm blooded?
}

const std::string &Character::symbol() const
{
    static const std::string character_symbol( "@" );
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
    } else {
        Creature::mod_stat( stat, modifier );
    }
}

int Character::get_fat_to_hp() const
{
    float mut_fat_hp = 0;
    for( const trait_id &mut : get_mutations() ) {
        mut_fat_hp += mut.obj().fat_to_max_hp;
    }

    return mut_fat_hp * ( get_bmi() - character_weight_category::normal );
}

std::string Character::disp_name( bool possessive ) const
{
    if( !possessive ) {
        if( is_player() ) {
            return pgettext( "not possessive", "you" );
        }
        return name;
    } else {
        if( is_player() ) {
            return _( "your" );
        }
        return string_format( _( "%s's" ), name );
    }
}

std::string Character::skin_name() const
{
    // TODO: Return actual deflecting layer name
    return _( "armor" );
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
        if( effective_dispersion( mod.sight_dispersion ) < recoil &&
            mod.aim_speed > sight_speed_modifier ) {
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

void Character::set_destination_activity( const player_activity &new_destination_activity )
{
    destination_activity = new_destination_activity;
}

void Character::clear_destination_activity()
{
    destination_activity = player_activity();
}

player_activity Character::get_destination_activity() const
{
    return destination_activity;
}

void Character::mount_creature( monster &z )
{
    tripoint pnt = z.pos();
    std::shared_ptr<monster> mons = g->shared_from( z );
    if( mons == nullptr ) {
        add_msg( m_debug, "mount_creature(): monster not found in critter_tracker" );
        return;
    }
    add_effect( effect_riding, 1_turns, num_bp, true );
    z.add_effect( effect_ridden, 1_turns, num_bp, true );
    if( z.has_effect( effect_tied ) ) {
        z.remove_effect( effect_tied );
        if( z.tied_item ) {
            i_add( *z.tied_item );
            z.tied_item = cata::nullopt;
        }
    }
    z.mounted_player_id = getID();
    if( z.has_effect( effect_harnessed ) ) {
        z.remove_effect( effect_harnessed );
        add_msg_if_player( m_info, _( "You remove the %s's harness." ), z.get_name() );
    }
    mounted_creature = mons;
    mons->mounted_player = this;
    if( is_avatar() ) {
        if( g->u.is_hauling() ) {
            g->u.stop_hauling();
        }
        if( g->u.get_grab_type() != OBJECT_NONE ) {
            add_msg( m_warning, _( "You let go of the grabbed object." ) );
            g->u.grab( OBJECT_NONE );
        }
        g->place_player( pnt );
    } else {
        npc &guy = dynamic_cast<npc &>( *this );
        guy.setpos( pnt );
    }
    z.facing = facing;
    add_msg_if_player( m_good, _( "You climb on the %s." ), z.get_name() );
    if( z.has_flag( MF_RIDEABLE_MECH ) ) {
        if( !z.type->mech_weapon.empty() ) {
            item mechwep = item( z.type->mech_weapon );
            wield( mechwep );
        }
        add_msg_if_player( m_good, _( "You hear your %s whir to life." ), z.get_name() );
    }
    // some rideable mechs have night-vision
    recalc_sight_limits();
    mod_moves( -100 );
}

bool Character::is_mounted() const
{
    return has_effect( effect_riding ) && mounted_creature;
}

void Character::forced_dismount()
{
    remove_effect( effect_riding );
    bool mech = false;
    if( mounted_creature ) {
        auto mon = mounted_creature.get();
        if( mon->has_flag( MF_RIDEABLE_MECH ) && !mon->type->mech_weapon.empty() ) {
            mech = true;
            remove_item( weapon );
        }
        mon->mounted_player_id = character_id();
        mon->remove_effect( effect_ridden );
        mon->add_effect( effect_controlled, 5_turns );
        mounted_creature = nullptr;
        mon->mounted_player = nullptr;
    }
    std::vector<tripoint> valid;
    for( const tripoint &jk : g->m.points_in_radius( pos(), 1 ) ) {
        if( g->is_empty( jk ) ) {
            valid.push_back( jk );
        }
    }
    if( !valid.empty() ) {
        setpos( random_entry( valid ) );
        if( mech ) {
            add_msg_player_or_npc( m_bad, _( "You are ejected from your mech!" ),
                                   _( "<npcname> is ejected from their mech!" ) );
        } else {
            add_msg_player_or_npc( m_bad, _( "You fall off your mount!" ),
                                   _( "<npcname> falls off their mount!" ) );
        }
        const int dodge = get_dodge();
        const int damage = std::max( 0, rng( 1, 20 ) - rng( dodge, dodge * 2 ) );
        body_part hit = num_bp;
        switch( rng( 1, 10 ) ) {
            case  1:
                if( one_in( 2 ) ) {
                    hit = bp_foot_l;
                } else {
                    hit = bp_foot_r;
                }
                break;
            case  2:
            case  3:
            case  4:
                if( one_in( 2 ) ) {
                    hit = bp_leg_l;
                } else {
                    hit = bp_leg_r;
                }
                break;
            case  5:
            case  6:
            case  7:
                if( one_in( 2 ) ) {
                    hit = bp_arm_l;
                } else {
                    hit = bp_arm_r;
                }
                break;
            case  8:
            case  9:
                hit = bp_torso;
                break;
            case 10:
                hit = bp_head;
                break;
        }
        if( damage > 0 ) {
            add_msg_if_player( m_bad, _( "You hurt yourself!" ) );
            deal_damage( nullptr, hit, damage_instance( DT_BASH, damage ) );
            if( is_avatar() ) {
                g->memorial().add(
                    pgettext( "memorial_male", "Fell off a mount." ),
                    pgettext( "memorial_female", "Fell off a mount." ) );
            }
            check_dead_state();
        }
        add_effect( effect_downed, 5_turns, num_bp, true );
    } else {
        add_msg( m_debug, "Forced_dismount could not find a square to deposit player" );
    }
    if( is_avatar() ) {
        if( g->u.get_grab_type() != OBJECT_NONE ) {
            add_msg( m_warning, _( "You let go of the grabbed object." ) );
            g->u.grab( OBJECT_NONE );
        }
        set_movement_mode( CMM_WALK );
        g->update_map( g->u );
    }
    moves -= 150;
}

void Character::dismount()
{
    if( !is_mounted() ) {
        add_msg( m_debug, "dismount called when not riding" );
        return;
    }
    if( const cata::optional<tripoint> pnt = choose_adjacent( _( "Dismount where?" ) ) ) {
        if( !g->is_empty( *pnt ) ) {
            add_msg( m_warning, _( "You cannot dismount there!" ) );
            return;
        }
        remove_effect( effect_riding );
        monster *critter = mounted_creature.get();
        critter->mounted_player_id = character_id();
        if( critter->has_flag( MF_RIDEABLE_MECH ) && !critter->type->mech_weapon.empty() ) {
            remove_item( g->u.weapon );
        }
        if( is_avatar() && g->u.get_grab_type() != OBJECT_NONE ) {
            add_msg( m_warning, _( "You let go of the grabbed object." ) );
            g->u.grab( OBJECT_NONE );
        }
        critter->remove_effect( effect_ridden );
        critter->add_effect( effect_controlled, 5_turns );
        mounted_creature = nullptr;
        critter->mounted_player = nullptr;
        setpos( *pnt );
        g->refresh_all();
        mod_moves( -100 );
        set_movement_mode( CMM_WALK );
    }
}

/** Returns true if the character has two functioning arms */
bool Character::has_two_arms() const
{
    return get_working_arm_count() >= 2;
}

// working is defined here as not disabled which means arms can be not broken
// and still not count if they have low enough hitpoints
int Character::get_working_arm_count() const
{
    if( has_active_mutation( trait_id( "SHELL2" ) ) ) {
        return 0;
    }

    int limb_count = 0;
    if( !is_limb_disabled( hp_arm_l ) ) {
        limb_count++;
    }
    if( !is_limb_disabled( hp_arm_r ) ) {
        limb_count++;
    }
    if( has_bionic( bionic_id( "bio_blaster" ) ) && limb_count > 0 ) {
        limb_count--;
    }

    return limb_count;
}

// working is defined here as not broken
int Character::get_working_leg_count() const
{
    int limb_count = 0;
    if( !is_limb_broken( hp_leg_l ) ) {
        limb_count++;
    }
    if( !is_limb_broken( hp_leg_r ) ) {
        limb_count++;
    }
    return limb_count;
}

bool Character::is_limb_hindered( hp_part limb ) const
{
    return hp_cur[limb] < hp_max[limb] * .40;
}

bool Character::is_limb_disabled( hp_part limb ) const
{
    return hp_cur[limb] <= hp_max[limb] * .125;
}

// this is the source of truth on if a limb is broken so all code to determine
// if a limb is broken should point here to make any future changes to breaking easier
bool Character::is_limb_broken( hp_part limb ) const
{
    return hp_cur[limb] == 0;
}

bool Character::can_run()
{
    return get_stamina() > 0 && !has_effect( effect_winded ) && get_working_leg_count() >= 2;
}

bool Character::move_effects( bool attacking )
{
    if( has_effect( effect_downed ) ) {
        /** @EFFECT_DEX increases chance to stand up when knocked down */

        /** @EFFECT_STR increases chance to stand up when knocked down, slightly */
        if( rng( 0, 40 ) > get_dex() + get_str() / 2 ) {
            add_msg_if_player( _( "You struggle to stand." ) );
        } else {
            add_msg_player_or_npc( m_good, _( "You stand up." ),
                                   _( "<npcname> stands up." ) );
            remove_effect( effect_downed );
        }
        return false;
    }
    if( has_effect( effect_webbed ) ) {
        if( is_mounted() ) {
            auto mon = mounted_creature.get();
            if( x_in_y( mon->type->melee_dice * mon->type->melee_sides,
                        6 * get_effect_int( effect_webbed ) ) ) {
                add_msg( _( "The %s breaks free of the webs!" ), mon->get_name() );
                mon->remove_effect( effect_webbed );
                remove_effect( effect_webbed );
            }
            /** @EFFECT_STR increases chance to escape webs */
        } else if( x_in_y( get_str(), 6 * get_effect_int( effect_webbed ) ) ) {
            add_msg_player_or_npc( m_good, _( "You free yourself from the webs!" ),
                                   _( "<npcname> frees themselves from the webs!" ) );
            remove_effect( effect_webbed );
        } else {
            add_msg_if_player( _( "You try to free yourself from the webs, but can't get loose!" ) );
        }
        return false;
    }
    if( has_effect( effect_lightsnare ) ) {
        if( is_mounted() ) {
            auto mon = mounted_creature.get();
            if( x_in_y( mon->type->melee_dice * mon->type->melee_sides, 12 ) ) {
                mon->remove_effect( effect_lightsnare );
                remove_effect( effect_lightsnare );
                g->m.spawn_item( pos(), "string_36" );
                g->m.spawn_item( pos(), "snare_trigger" );
                add_msg( _( "The %s escapes the light snare!" ), mon->get_name() );
            }
        } else {
            /** @EFFECT_STR increases chance to escape light snare */

            /** @EFFECT_DEX increases chance to escape light snare */
            if( x_in_y( get_str(), 12 ) || x_in_y( get_dex(), 8 ) ) {
                remove_effect( effect_lightsnare );
                add_msg_player_or_npc( m_good, _( "You free yourself from the light snare!" ),
                                       _( "<npcname> frees themselves from the light snare!" ) );
                item string( "string_36", calendar::turn );
                item snare( "snare_trigger", calendar::turn );
                g->m.add_item_or_charges( pos(), string );
                g->m.add_item_or_charges( pos(), snare );
            } else {
                add_msg_if_player( m_bad,
                                   _( "You try to free yourself from the light snare, but can't get loose!" ) );
            }
            return false;
        }
    }
    if( has_effect( effect_heavysnare ) ) {
        if( is_mounted() ) {
            auto mon = mounted_creature.get();
            if( mon->type->melee_dice * mon->type->melee_sides >= 7 ) {
                if( x_in_y( mon->type->melee_dice * mon->type->melee_sides, 32 ) ) {
                    mon->remove_effect( effect_heavysnare );
                    remove_effect( effect_heavysnare );
                    g->m.spawn_item( pos(), "rope_6" );
                    g->m.spawn_item( pos(), "snare_trigger" );
                    add_msg( _( "The %s escapes the heavy snare!" ), mon->get_name() );
                }
            }
        } else {
            /** @EFFECT_STR increases chance to escape heavy snare, slightly */

            /** @EFFECT_DEX increases chance to escape light snare */
            if( x_in_y( get_str(), 32 ) || x_in_y( get_dex(), 16 ) ) {
                remove_effect( effect_heavysnare );
                add_msg_player_or_npc( m_good, _( "You free yourself from the heavy snare!" ),
                                       _( "<npcname> frees themselves from the heavy snare!" ) );
                item rope( "rope_6", calendar::turn );
                item snare( "snare_trigger", calendar::turn );
                g->m.add_item_or_charges( pos(), rope );
                g->m.add_item_or_charges( pos(), snare );
            } else {
                add_msg_if_player( m_bad,
                                   _( "You try to free yourself from the heavy snare, but can't get loose!" ) );
            }
        }
        return false;
    }
    if( has_effect( effect_beartrap ) ) {
        /* Real bear traps can't be removed without the proper tools or immense strength; eventually this should
           allow normal players two options: removal of the limb or removal of the trap from the ground
           (at which point the player could later remove it from the leg with the right tools).
           As such we are currently making it a bit easier for players and NPC's to get out of bear traps.
        */
        /** @EFFECT_STR increases chance to escape bear trap */
        // If is riding, then despite the character having the effect, it is the mounted creature that escapes.
        if( is_player() && is_mounted() ) {
            auto mon = mounted_creature.get();
            if( mon->type->melee_dice * mon->type->melee_sides >= 18 ) {
                if( x_in_y( mon->type->melee_dice * mon->type->melee_sides, 200 ) ) {
                    mon->remove_effect( effect_beartrap );
                    remove_effect( effect_beartrap );
                    g->m.spawn_item( pos(), "beartrap" );
                    add_msg( _( "The %s escapes the bear trap!" ), mon->get_name() );
                } else {
                    add_msg_if_player( m_bad,
                                       _( "Your %s tries to free itself from the bear trap, but can't get loose!" ), mon->get_name() );
                }
            }
        } else {
            if( x_in_y( get_str(), 100 ) ) {
                remove_effect( effect_beartrap );
                add_msg_player_or_npc( m_good, _( "You free yourself from the bear trap!" ),
                                       _( "<npcname> frees themselves from the bear trap!" ) );
                item beartrap( "beartrap", calendar::turn );
                g->m.add_item_or_charges( pos(), beartrap );
            } else {
                add_msg_if_player( m_bad,
                                   _( "You try to free yourself from the bear trap, but can't get loose!" ) );
            }
        }
        return false;
    }
    if( has_effect( effect_crushed ) ) {
        /** @EFFECT_STR increases chance to escape crushing rubble */

        /** @EFFECT_DEX increases chance to escape crushing rubble, slightly */
        if( x_in_y( get_str() + get_dex() / 4.0, 100 ) ) {
            remove_effect( effect_crushed );
            add_msg_player_or_npc( m_good, _( "You free yourself from the rubble!" ),
                                   _( "<npcname> frees themselves from the rubble!" ) );
        } else {
            add_msg_if_player( m_bad, _( "You try to free yourself from the rubble, but can't get loose!" ) );
        }
        return false;
    }
    // Below this point are things that allow for movement if they succeed

    // Currently we only have one thing that forces movement if you succeed, should we get more
    // than this will need to be reworked to only have success effects if /all/ checks succeed
    if( has_effect( effect_in_pit ) ) {
        /** @EFFECT_STR increases chance to escape pit */

        /** @EFFECT_DEX increases chance to escape pit, slightly */
        if( rng( 0, 40 ) > get_str() + get_dex() / 2 ) {
            add_msg_if_player( m_bad, _( "You try to escape the pit, but slip back in." ) );
            return false;
        } else {
            add_msg_player_or_npc( m_good, _( "You escape the pit!" ),
                                   _( "<npcname> escapes the pit!" ) );
            remove_effect( effect_in_pit );
        }
    }
    if( has_effect( effect_grabbed ) && !attacking ) {
        int zed_number = 0;
        if( is_mounted() ) {
            auto mon = mounted_creature.get();
            if( mon->has_effect( effect_grabbed ) ) {
                if( ( dice( mon->type->melee_dice + mon->type->melee_sides,
                            3 ) < get_effect_int( effect_grabbed ) ) ||
                    !one_in( 4 ) ) {
                    add_msg( m_bad, _( "Your %s tries to break free, but fails!" ), mon->get_name() );
                    return false;
                } else {
                    add_msg( m_good, _( "Your %s breaks free from the grab!" ), mon->get_name() );
                    remove_effect( effect_grabbed );
                    mon->remove_effect( effect_grabbed );
                }
            } else {
                if( one_in( 4 ) ) {
                    add_msg( m_bad, _( "You are pulled from your %s!" ), mon->get_name() );
                    remove_effect( effect_grabbed );
                    dynamic_cast<player &>( *this ).forced_dismount();
                }
            }
        } else {
            for( auto &&dest : g->m.points_in_radius( pos(), 1, 0 ) ) { // *NOPAD*
                const monster *const mon = g->critter_at<monster>( dest );
                if( mon && mon->has_effect( effect_grabbing ) ) {
                    zed_number += mon->get_grab_strength();
                }
            }
            if( zed_number == 0 ) {
                add_msg_player_or_npc( m_good, _( "You find yourself no longer grabbed." ),
                                       _( "<npcname> finds themselves no longer grabbed." ) );
                remove_effect( effect_grabbed );
                /** @EFFECT_DEX increases chance to escape grab, if >STR */

                /** @EFFECT_STR increases chance to escape grab, if >DEX */
            } else if( rng( 0, std::max( get_dex(), get_str() ) ) <
                       rng( get_effect_int( effect_grabbed, bp_torso ), 8 ) ) {
                // Randomly compare higher of dex or str to grab intensity.
                add_msg_player_or_npc( m_bad, _( "You try break out of the grab, but fail!" ),
                                       _( "<npcname> tries to break out of the grab, but fails!" ) );
                return false;
            } else {
                add_msg_player_or_npc( m_good, _( "You break out of the grab!" ),
                                       _( "<npcname> breaks out of the grab!" ) );
                remove_effect( effect_grabbed );
                for( auto &&dest : g->m.points_in_radius( pos(), 1, 0 ) ) { // *NOPAD*
                    monster *mon = g->critter_at<monster>( dest );
                    if( mon && mon->has_effect( effect_grabbing ) ) {
                        mon->remove_effect( effect_grabbing );
                    }
                }
            }
        }
    }
    return true;
}

bool Character::movement_mode_is( const character_movemode mode ) const
{
    return move_mode == mode;
}

void Character::add_effect( const efftype_id &eff_id, const time_duration dur, body_part bp,
                            bool permanent, int intensity, bool force, bool deferred )
{
    Creature::add_effect( eff_id, dur, bp, permanent, intensity, force, deferred );
}

void Character::process_turn()
{
    for( auto &i : *my_bionics ) {
        if( i.incapacitated_time > 0_turns ) {
            i.incapacitated_time -= 1_turns;
            if( i.incapacitated_time == 0_turns ) {
                add_msg_if_player( m_bad, _( "Your %s bionic comes back online." ), i.info().name );
            }
        }
    }

    Creature::process_turn();

    check_item_encumbrance_flag();
}

void Character::recalc_hp()
{
    int new_max_hp[num_hp_parts];
    int str_boost_val = 0;
    cata::optional<skill_boost> str_boost = skill_boost::get( "str" );
    if( str_boost ) {
        int skill_total = 0;
        for( const std::string &skill_str : str_boost->skills() ) {
            skill_total += get_skill_level( skill_id( skill_str ) );
        }
        str_boost_val = str_boost->calc_bonus( skill_total );
    }
    // Mutated toughness stacks with starting, by design.
    float hp_mod = 1.0f + mutation_value( "hp_modifier" ) + mutation_value( "hp_modifier_secondary" );
    float hp_adjustment = mutation_value( "hp_adjustment" ) + ( str_boost_val * 3 );
    for( auto &elem : new_max_hp ) {
        /** @EFFECT_STR_MAX increases base hp */
        elem = 60 + str_max * 3 + hp_adjustment + get_fat_to_hp();
        elem *= hp_mod;
    }
    if( has_trait( trait_GLASSJAW ) ) {
        new_max_hp[hp_head] *= 0.8;
    }
    for( int i = 0; i < num_hp_parts; i++ ) {
        // Only recalculate when max changes,
        // otherwise we end up walking all over due to rounding errors.
        if( new_max_hp[i] == hp_max[i] ) {
            continue;
        }
        float max_hp_ratio = static_cast<float>( new_max_hp[i] ) /
                             static_cast<float>( hp_max[i] );
        hp_cur[i] = std::ceil( static_cast<float>( hp_cur[i] ) * max_hp_ratio );
        hp_cur[i] = std::max( std::min( hp_cur[i], new_max_hp[i] ), 1 );
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
    if( is_blind() || ( in_sleep_state() && !has_trait( trait_SEESLEEP ) ) ||
        has_effect( effect_narcosis ) ) {
        sight_max = 0;
    } else if( has_effect( effect_boomered ) && ( !( has_trait( trait_PER_SLIME_OK ) ) ) ) {
        sight_max = 1;
        vision_mode_cache.set( BOOMERED );
    } else if( has_effect( effect_in_pit ) || has_effect( effect_no_sight ) ||
               ( underwater && !has_bionic( bionic_id( "bio_membrane" ) ) &&
                 !has_trait( trait_MEMBRANE ) && !worn_with_flag( "SWIM_GOGGLES" ) &&
                 !has_trait( trait_CEPH_EYES ) && !has_trait( trait_PER_SLIME_OK ) ) ) {
        sight_max = 1;
    } else if( has_active_mutation( trait_SHELL2 ) ) {
        // You can kinda see out a bit.
        sight_max = 2;
    } else if( ( has_trait( trait_MYOPIC ) || has_trait( trait_URSINE_EYE ) ) &&
               !worn_with_flag( "FIX_NEARSIGHT" ) && !has_effect( effect_contacts ) ) {
        sight_max = 4;
    } else if( has_trait( trait_PER_SLIME ) ) {
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
    if( has_active_mutation( trait_NIGHTVISION3 ) || is_wearing( "rm13_armor_on" ) ||
        ( is_mounted() && mounted_creature->has_flag( MF_MECH_RECON_VISION ) ) ) {
        vision_mode_cache.set( NIGHTVISION_3 );
    }
    if( has_active_mutation( trait_ELFA_FNV ) ) {
        vision_mode_cache.set( FULL_ELFA_VISION );
    }
    if( has_active_mutation( trait_CEPH_VISION ) ) {
        vision_mode_cache.set( CEPH_VISION );
    }
    if( has_active_mutation( trait_ELFA_NV ) ) {
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
    if( has_active_mutation( trait_NIGHTVISION ) ) {
        vision_mode_cache.set( NIGHTVISION_1 );
    }
    if( has_trait( trait_BIRD_EYE ) ) {
        vision_mode_cache.set( BIRD_EYE );
    }

    // Not exactly a sight limit thing, but related enough
    if( has_active_bionic( bionic_id( "bio_infrared" ) ) ||
        has_trait( trait_id( "INFRARED" ) ) ||
        has_trait( trait_id( "LIZ_IR" ) ) ||
        worn_with_flag( "IR_EFFECT" ) || ( is_mounted() &&
                                           mounted_creature->has_flag( MF_MECH_RECON_VISION ) ) ) {
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

float Character::get_vision_threshold( float light_level ) const
{
    if( vision_mode_cache[DEBUG_NIGHTVISION] ) {
        // Debug vision always works with absurdly little light.
        return 0.01;
    }

    // As light_level goes from LIGHT_AMBIENT_MINIMAL to LIGHT_AMBIENT_LIT,
    // dimming goes from 1.0 to 2.0.
    const float dimming_from_light = 1.0 + ( ( static_cast<float>( light_level ) -
                                     LIGHT_AMBIENT_MINIMAL ) /
                                     ( LIGHT_AMBIENT_LIT - LIGHT_AMBIENT_MINIMAL ) );

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

    return std::min( static_cast<float>( LIGHT_AMBIENT_LOW ),
                     threshold_for_range( range ) * dimming_from_light );
}

void Character::flag_encumbrance()
{
    check_encumbrance = true;
}

void Character::check_item_encumbrance_flag()
{
    bool update_required = check_encumbrance;
    for( auto &i : worn ) {
        if( !update_required && i.has_flag( "ENCUMBRANCE_UPDATE" ) ) {
            update_required = true;
        }
        i.unset_flag( "ENCUMBRANCE_UPDATE" );
    }

    if( update_required ) {
        reset_encumbrance();
    }
}

std::vector<bionic_id> Character::get_bionics() const
{
    std::vector<bionic_id> result;
    for( const bionic &b : *my_bionics ) {
        result.push_back( b.id );
    }
    return result;
}

bool Character::has_bionic( const bionic_id &b ) const
{
    for( auto &i : *my_bionics ) {
        if( i.id == b ) {
            return true;
        }
    }
    return false;
}

bool Character::has_active_bionic( const bionic_id &b ) const
{
    for( auto &i : *my_bionics ) {
        if( i.id == b ) {
            return ( i.powered && i.incapacitated_time == 0_turns );
        }
    }
    return false;
}

bool Character::has_any_bionic() const
{
    return !my_bionics->empty();
}

bool Character::can_fuel_bionic_with( const item &it ) const
{
    if( !it.is_fuel() ) {
        return false;
    }

    for( const auto bio : *my_bionics ) {
        for( const itype_id fuel : bio.info().fuel_opts ) {
            if( fuel == it.typeId() ) {
                return true;
            }
        }
    }
    return false;
}

std::vector<bionic_id> Character::get_bionic_fueled_with( const item &it ) const
{
    std::vector<bionic_id> bionics;

    for( const bionic bio : *my_bionics ) {
        for( const itype_id fuel : bio.info().fuel_opts ) {
            if( fuel == it.typeId() ) {
                bionics.emplace_back( bio.id );
            }
        }
    }

    return bionics;
}

std::vector<bionic_id> Character::get_fueled_bionics() const
{
    std::vector<bionic_id> bionics;
    for( const bionic bio : *my_bionics ) {
        if( !bio.info().fuel_opts.empty() ) {
            bionics.emplace_back( bio.id );
        }
    }
    return bionics;
}

bionic_id Character::get_most_efficient_bionic( const std::vector<bionic_id> &bids ) const
{
    float temp_eff = 0;
    bionic_id bio( "null" );
    for( const bionic_id &bid : bids ) {
        if( bid->fuel_efficiency > temp_eff ) {
            temp_eff = bid->fuel_efficiency;
            bio = bid;
        }
    }
    return bio;
}

units::energy Character::get_power_level() const
{
    return power_level;
}

units::energy Character::get_max_power_level() const
{
    return max_power_level;
}

void Character::set_power_level( units::energy npower )
{
    power_level = std::min( npower, max_power_level );
}

void Character::set_max_power_level( units::energy npower_max )
{
    max_power_level = npower_max;
}

void Character::mod_power_level( units::energy npower )
{
    int new_power = units::to_millijoule( power_level ) + units::to_millijoule( npower );
    // An integer overflow has occurred - the sum of two positive things (power_level is always positive)
    // cannot be negative, so if it is, we know integer overflow has occured
    if( npower >= 0_mJ && new_power < 0 ) {
        new_power = units::to_millijoule( max_power_level );
    }
    power_level = clamp( units::from_millijoule( new_power ), 0_kJ, max_power_level );
}

void Character::mod_max_power_level( units::energy npower_max )
{
    max_power_level += npower_max;
}

bool Character::is_max_power() const
{
    return power_level >= max_power_level;
}

bool Character::has_power() const
{
    return power_level > 0_kJ;
}

bool Character::has_max_power() const
{
    return max_power_level > 0_kJ;
}

bool Character::enough_power_for( const bionic_id &bid ) const
{
    return power_level >= bid->power_activate;
}

std::vector<itype_id> Character::get_fuel_available( const bionic_id &bio ) const
{
    std::vector<itype_id> stored_fuels;
    for( const itype_id fuel : bio->fuel_opts ) {
        if( !get_value( fuel ).empty() || fuel == itype_id( "muscle" ) ) {
            stored_fuels.emplace_back( fuel );
        }
    }
    return stored_fuels;
}

int Character::get_fuel_capacity( const itype_id fuel ) const
{
    int amount_stored = 0;
    if( !get_value( fuel ).empty() ) {
        amount_stored = std::stoi( get_value( fuel ) );
    }
    int capacity = 0;
    for( const bionic &bio : *my_bionics ) {
        for( const itype_id &fl : bio.info().fuel_opts ) {
            if( get_value( bio.id.c_str() ).empty() || get_value( bio.id.c_str() ) == fl ) {
                if( fl == fuel ) {
                    capacity += bio.info().fuel_capacity;
                }
            }
        }
    }
    return capacity - amount_stored;
}

int Character::get_total_fuel_capacity( const itype_id fuel ) const
{
    int capacity = 0;
    for( const bionic &bio : *my_bionics ) {
        for( const itype_id &fl : bio.info().fuel_opts ) {
            if( get_value( bio.id.c_str() ).empty() || get_value( bio.id.c_str() ) == fl ) {
                if( fl == fuel ) {
                    capacity += bio.info().fuel_capacity;
                }
            }
        }
    }
    return capacity;
}

void Character::update_fuel_storage( const itype_id &fuel )
{
    const item it( fuel );
    if( get_value( fuel ).empty() ) {
        for( const bionic_id &bid : get_bionic_fueled_with( it ) ) {
            remove_value( bid.c_str() );
        }
        return;
    }

    std::vector<bionic_id> bids = get_bionic_fueled_with( it );
    int amount_fuel_loaded = std::stoi( get_value( fuel ) );
    std::vector<bionic_id> loaded_bio;

    // Sort bionic in order of decreasing capacity
    // To fill the bigger ones firts.
    bool swap = true;
    while( swap ) {
        swap = false;
        for( size_t i = 0; i < bids.size() - 1; i++ ) {
            if( bids[i + 1]->fuel_capacity > bids[i]->fuel_capacity ) {
                std::swap( bids[i + 1], bids[i] );
                swap = true;
            }
        }
    }

    for( const bionic_id &bid : bids ) {
        remove_value( bid.c_str() );
        if( bid->fuel_capacity <= amount_fuel_loaded ) {
            amount_fuel_loaded -= bid->fuel_capacity;
            loaded_bio.emplace_back( bid );
        } else if( amount_fuel_loaded != 0 ) {
            loaded_bio.emplace_back( bid );
            break;
        }
    }

    for( const bionic_id &bd : loaded_bio ) {
        set_value( bd.c_str(), fuel );
    }

}

int Character::get_mod_stat_from_bionic( const Character::stat &Stat ) const
{
    int ret = 0;
    for( const bionic &bio : *my_bionics ) {
        const auto St_bn = bio.info().stat_bonus.find( Stat );
        if( St_bn != bio.info().stat_bonus.end() ) {
            ret += St_bn->second;
        }
    }
    return ret;
}

std::vector<item_location> Character::nearby( const
        std::function<bool( const item *, const item * )> &func, int radius ) const
{
    std::vector<item_location> res;

    visit_items( [&]( const item * e, const item * parent ) {
        if( func( e, parent ) ) {
            res.emplace_back( const_cast<Character &>( *this ), const_cast<item *>( e ) );
        }
        return VisitResponse::NEXT;
    } );

    for( const auto &cur : map_selector( pos(), radius ) ) {
        cur.visit_items( [&]( const item * e, const item * parent ) {
            if( func( e, parent ) ) {
                res.emplace_back( cur, const_cast<item *>( e ) );
            }
            return VisitResponse::NEXT;
        } );
    }

    for( const auto &cur : vehicle_selector( pos(), radius ) ) {
        cur.visit_items( [&]( const item * e, const item * parent ) {
            if( func( e, parent ) ) {
                res.emplace_back( cur, const_cast<item *>( e ) );
            }
            return VisitResponse::NEXT;
        } );
    }

    return res;
}

int Character::i_add_to_container( const item &it, const bool unloading )
{
    int charges = it.charges;
    if( !it.is_ammo() || unloading ) {
        return charges;
    }

    const itype_id item_type = it.typeId();
    auto add_to_container = [&it, &charges]( item & container ) {
        auto &contained_ammo = container.contents.front();
        if( contained_ammo.charges < container.ammo_capacity() ) {
            const int diff = container.ammo_capacity() - contained_ammo.charges;
            //~ %1$s: item name, %2$s: container name
            add_msg( pgettext( "container", "You put the %1$s in your %2$s." ), it.tname(), container.tname() );
            if( diff > charges ) {
                contained_ammo.charges += charges;
                return 0;
            } else {
                contained_ammo.charges = container.ammo_capacity();
                return charges - diff;
            }
        }
        return charges;
    };

    visit_items( [ & ]( item * item ) {
        if( charges > 0 && item->is_ammo_container() && item_type == item->contents.front().typeId() ) {
            charges = add_to_container( *item );
            item->handle_pickup_ownership( *this );
        }
        return VisitResponse::NEXT;
    } );

    return charges;
}

item &Character::i_add( item it, bool should_stack )
{
    itype_id item_type_id = it.typeId();
    last_item = item_type_id;

    if( it.is_food() || it.is_ammo() || it.is_gun() || it.is_armor() ||
        it.is_book() || it.is_tool() || it.is_melee() || it.is_food_container() ) {
        inv.unsort();
    }

    // if there's a desired invlet for this item type, try to use it
    bool keep_invlet = false;
    const invlets_bitset cur_inv = allocated_invlets();
    for( auto iter : inv.assigned_invlet ) {
        if( iter.second == item_type_id && !cur_inv[iter.first] ) {
            it.invlet = iter.first;
            keep_invlet = true;
            break;
        }
    }
    auto &item_in_inv = inv.add_item( it, keep_invlet, true, should_stack );
    item_in_inv.on_pickup( *this );
    cached_info.erase( "reloadables" );
    return item_in_inv;
}

std::list<item> Character::remove_worn_items_with( std::function<bool( item & )> filter )
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
const item &Character::i_at( int position ) const
{
    if( position == -1 ) {
        return weapon;
    }
    if( position < -1 ) {
        int worn_index = worn_position_to_index( position );
        if( static_cast<size_t>( worn_index ) < worn.size() ) {
            auto iter = worn.begin();
            std::advance( iter, worn_index );
            return *iter;
        }
    }

    return inv.find_item( position );
}

item &Character::i_at( int position )
{
    return const_cast<item &>( const_cast<const Character *>( this )->i_at( position ) );
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

item Character::i_rem( int pos )
{
    item tmp;
    if( pos == -1 ) {
        tmp = weapon;
        weapon = item();
        return tmp;
    } else if( pos < -1 && pos > worn_position_to_index( worn.size() ) ) {
        auto iter = worn.begin();
        std::advance( iter, worn_position_to_index( pos ) );
        tmp = *iter;
        tmp.on_takeoff( *this );
        worn.erase( iter );
        return tmp;
    }
    return inv.remove_item( pos );
}

item Character::i_rem( const item *it )
{
    auto tmp = remove_items_with( [&it]( const item & i ) {
        return &i == it;
    }, 1 );
    if( tmp.empty() ) {
        debugmsg( "did not found item %s to remove it!", it->tname() );
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

bool Character::i_add_or_drop( item &it, int qty )
{
    bool retval = true;
    bool drop = it.made_of( LIQUID );
    bool add = it.is_gun() || !it.is_irremovable();
    inv.assign_empty_invlet( it, *this );
    for( int i = 0; i < qty; ++i ) {
        drop |= !can_pickWeight( it, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) || !can_pickVolume( it );
        if( drop ) {
            retval &= !g->m.add_item_or_charges( pos(), it ).is_null();
        } else if( add ) {
            i_add( it );
        }
    }

    return retval;
}

invlets_bitset Character::allocated_invlets() const
{
    invlets_bitset invlets = inv.allocated_invlets();

    invlets.set( weapon.invlet );
    for( const auto &w : worn ) {
        invlets.set( w.invlet );
    }

    invlets[0] = false;

    return invlets;
}

bool Character::has_active_item( const itype_id &id ) const
{
    return has_item_with( [id]( const item & it ) {
        return it.active && it.typeId() == id;
    } );
}

item Character::remove_weapon()
{
    item tmp = weapon;
    weapon = item();
    cached_info.erase( "weapon_value" );
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
        return it.ammo_type() == at;
    } );
}

template <typename T, typename Output>
void find_ammo_helper( T &src, const item &obj, bool empty, Output out, bool nested )
{
    if( obj.is_watertight_container() ) {
        if( !obj.is_container_empty() ) {
            auto contents_id = obj.contents.front().typeId();

            // Look for containers with the same type of liquid as that already in our container
            src.visit_items( [&src, &nested, &out, &contents_id, &obj]( item * node ) {
                if( node == &obj || ( node->is_watertight_container() && node->contents_made_of( SOLID ) ) ) {
                    // This stops containers and magazines counting *themselves* as ammo sources and prevents reloading with items frozen in watertight containers.
                    return VisitResponse::SKIP;
                }

                if( node->is_watertight_container() && node->contents_made_of( SOLID ) ) {
                    return VisitResponse::SKIP;
                }

                if( node->is_container() && !node->is_container_empty() &&
                    node->contents.front().typeId() == contents_id ) {
                    out = item_location( src, node );
                }
                return nested ? VisitResponse::NEXT : VisitResponse::SKIP;
            } );
        } else {
            // Look for containers with any liquid and loose frozen liquids
            src.visit_items( [&src, &nested, &out]( item * node ) {
                if( node->is_watertight_container() && node->contents_made_of( SOLID ) ) {
                    return VisitResponse::SKIP;
                }

                if( ( node->is_container() && node->contents_made_of( LIQUID ) ) || node->is_frozen_liquid() ) {
                    out = item_location( src, node );
                }
                return nested ? VisitResponse::NEXT : VisitResponse::SKIP;
            } );
        }
    }
    if( obj.magazine_integral() ) {
        // find suitable ammo excluding that already loaded in magazines
        std::set<ammotype> ammo = obj.ammo_types();
        const auto mags = obj.magazine_compatible();

        src.visit_items( [&src, &nested, &out, &mags, ammo]( item * node ) {
            if( node->is_gun() || node->is_tool() ) {
                // guns/tools never contain usable ammo so most efficient to skip them now
                return VisitResponse::SKIP;
            }
            if( !node->made_of_from_type( SOLID ) ) {
                // some liquids are ammo but we can't reload with them unless within a container or frozen
                return VisitResponse::SKIP;
            }
            if( node->is_ammo_container() && !node->contents.empty() &&
                !node->contents_made_of( SOLID ) ) {
                for( const ammotype &at : ammo ) {
                    if( node->contents.front().ammo_type() == at ) {
                        out = item_location( src, node );
                    }
                }
                return VisitResponse::SKIP;
            }

            for( const ammotype &at : ammo ) {
                if( node->ammo_type() == at ) {
                    out = item_location( src, node );
                }
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

        src.visit_items( [&src, &nested, &out, mags, empty]( item * node ) {
            if( node->is_gun() || node->is_tool() ) {
                return VisitResponse::SKIP;
            }
            if( node->is_magazine() ) {
                if( mags.count( node->typeId() ) && ( node->ammo_remaining() || empty ) ) {
                    out = item_location( src, node );
                }
                return VisitResponse::SKIP;
            }
            return nested ? VisitResponse::NEXT : VisitResponse::SKIP;
        } );
    }
}

std::vector<item_location> Character::find_ammo( const item &obj, bool empty, int radius ) const
{
    std::vector<item_location> res;

    find_ammo_helper( const_cast<Character &>( *this ), obj, empty, std::back_inserter( res ), true );

    if( radius >= 0 ) {
        for( auto &cursor : map_selector( pos(), radius ) ) {
            find_ammo_helper( cursor, obj, empty, std::back_inserter( res ), false );
        }
        for( auto &cursor : vehicle_selector( pos(), radius ) ) {
            find_ammo_helper( cursor, obj, empty, std::back_inserter( res ), false );
        }
    }

    return res;
}

std::vector<item_location> Character::find_reloadables()
{
    std::vector<item_location> reloadables;

    visit_items( [this, &reloadables]( item * node ) {
        if( node->is_holster() ) {
            return VisitResponse::NEXT;
        }
        bool reloadable = false;
        if( node->is_gun() && !node->magazine_compatible().empty() ) {
            reloadable = node->magazine_current() == nullptr ||
                         node->ammo_remaining() < node->ammo_capacity();
        } else {
            reloadable = ( node->is_magazine() || node->is_bandolier() ||
                           ( node->is_gun() && node->magazine_integral() ) ) &&
                         node->ammo_remaining() < node->ammo_capacity();
        }
        if( reloadable ) {
            reloadables.push_back( item_location( *this, node ) );
        }
        return VisitResponse::SKIP;
    } );
    return reloadables;
}

units::mass Character::weight_carried() const
{
    return weight_carried_with_tweaks( {} );
}

units::volume Character::volume_carried() const
{
    return inv.volume();
}

int Character::best_nearby_lifting_assist() const
{
    return best_nearby_lifting_assist( this->pos() );
}

int Character::best_nearby_lifting_assist( const tripoint &world_pos ) const
{
    const quality_id LIFT( "LIFT" );
    int mech_lift = 0;
    if( is_mounted() ) {
        auto mons = mounted_creature.get();
        if( mons->has_flag( MF_RIDEABLE_MECH ) ) {
            mech_lift = mons->mech_str_addition() + 10;
        }
    }
    return std::max( { this->max_quality( LIFT ), mech_lift,
                       map_selector( this->pos(), PICKUP_RANGE ).max_quality( LIFT ),
                       vehicle_selector( world_pos, 4, true, true ).max_quality( LIFT )
                     } );
}

units::mass Character::weight_carried_with_tweaks( const item_tweaks &tweaks ) const
{
    const std::map<const item *, int> empty;
    const std::map<const item *, int> &without = tweaks.without_items ? tweaks.without_items->get() :
            empty;

    // Worn items
    units::mass ret = 0_gram;
    for( auto &i : worn ) {
        if( !without.count( &i ) ) {
            ret += i.weight();
        }
    }

    // Items in inventory
    const inventory &i = tweaks.replace_inv ? tweaks.replace_inv->get() : inv;
    ret += i.weight_without( without );

    // Wielded item
    units::mass weaponweight = 0_gram;
    auto weapon_it = without.find( &weapon );
    if( weapon_it == without.end() ) {
        weaponweight = weapon.weight();
    } else {
        int subtract_count = ( *weapon_it ).second;
        if( weapon.count_by_charges() ) {
            item copy = weapon;
            copy.charges -= subtract_count;
            if( copy.charges < 0 ) {
                debugmsg( "Trying to remove more charges than the wielded item has" );
                copy.charges = 0;
            }
            weaponweight = copy.weight();
        } else if( subtract_count > 1 ) {
            debugmsg( "Trying to remove more than one wielded item" );
        }
    }
    // Exclude wielded item if using lifting tool
    if( weaponweight + ret > weight_capacity() ) {
        const float liftrequirement = ceil( units::to_gram<float>( weaponweight ) / units::to_gram<float>
                                            ( TOOL_LIFT_FACTOR ) );
        if( g->new_game || best_nearby_lifting_assist() < liftrequirement ) {
            ret += weaponweight;
        }
    } else {
        ret += weaponweight;
    }

    return ret;
}

units::volume Character::volume_carried_with_tweaks( const item_tweaks &tweaks ) const
{
    const auto &i = tweaks.replace_inv ? tweaks.replace_inv->get() : inv;
    return tweaks.without_items ? i.volume_without( *tweaks.without_items ) : i.volume();
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
    ret *= mutation_value( "weight_capacity_modifier" );

    units::mass worn_weight_bonus = 0_gram;
    for( const item &it : worn ) {
        ret *= it.get_weight_capacity_modifier();
        worn_weight_bonus += it.get_weight_capacity_bonus();
    }

    units::mass bio_weight_bonus = 0_gram;
    for( const bionic &bio : *my_bionics ) {
        ret *= bio.info().weight_capacity_modifier;
        bio_weight_bonus +=  bio.info().weight_capacity_bonus;
    }

    ret += bio_weight_bonus + worn_weight_bonus ;

    if( has_artifact_with( AEP_CARRY_MORE ) ) {
        ret += 22500_gram;
    }

    if( ret < 0_gram ) {
        ret = 0_gram;
    }
    if( is_mounted() ) {
        auto *mons = mounted_creature.get();
        // the mech has an effective strength for other purposes, like hitting.
        // but for lifting, its effective strength is even higher, due to its sturdy construction, leverage,
        // and being built entirely for that purpose with hydraulics etc.
        ret = mons->mech_str_addition() == 0 ? ret : ( mons->mech_str_addition() + 10 ) * 4_kilogram;
    }
    return ret;
}

units::volume Character::volume_capacity() const
{
    return volume_capacity_reduced_by( 0_ml );
}

units::volume Character::volume_capacity_reduced_by(
    const units::volume &mod, const std::map<const item *, int> &without_items ) const
{
    if( has_trait( trait_id( "DEBUG_STORAGE" ) ) ) {
        return units::volume_max;
    }

    units::volume ret = -mod;
    for( auto &i : worn ) {
        if( !without_items.count( &i ) ) {
            ret += i.get_storage();
        }
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
    if( !safe ) {
        // Character can carry up to four times their maximum weight
        return ( weight_carried() + it.weight() <= ( has_trait( trait_id( "DEBUG_STORAGE" ) ) ?
                 units::mass_max : weight_capacity() * 4 ) );
    } else {
        return ( weight_carried() + it.weight() <= weight_capacity() );
    }
}

bool Character::can_use( const item &it, const item &context ) const
{
    const auto &ctx = !context.is_null() ? context : it;

    if( !meets_requirements( it, ctx ) ) {
        const std::string unmet( enumerate_unmet_requirements( it, ctx ) );

        if( &it == &ctx ) {
            //~ %1$s - list of unmet requirements, %2$s - item name.
            add_msg_player_or_npc( m_bad, _( "You need at least %1$s to use this %2$s." ),
                                   _( "<npcname> needs at least %1$s to use this %2$s." ),
                                   unmet, it.tname() );
        } else {
            //~ %1$s - list of unmet requirements, %2$s - item name, %3$s - indirect item name.
            add_msg_player_or_npc( m_bad, _( "You need at least %1$s to use this %2$s with your %3$s." ),
                                   _( "<npcname> needs at least %1$s to use this %2$s with their %3$s." ),
                                   unmet, it.tname(), ctx.tname() );
        }

        return false;
    }

    return true;
}

void Character::drop_invalid_inventory()
{
    bool dropped_liquid = false;
    for( const std::list<item> *stack : inv.const_slice() ) {
        const item &it = stack->front();
        if( it.made_of( LIQUID ) ) {
            dropped_liquid = true;
            g->m.add_item_or_charges( pos(), it );
            // must be last
            i_rem( &it );
        }
    }
    if( dropped_liquid ) {
        add_msg_if_player( m_bad, _( "Liquid from your inventory has leaked onto the ground." ) );
    }

    if( volume_carried() > volume_capacity() ) {
        auto items_to_drop = inv.remove_randomly_by_volume( volume_carried() - volume_capacity() );
        put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, items_to_drop );
    }
}

bool Character::has_artifact_with( const art_effect_passive effect ) const
{
    if( weapon.has_effect_when_wielded( effect ) ) {
        return true;
    }
    for( auto &i : worn ) {
        if( i.has_effect_when_worn( effect ) ) {
            return true;
        }
    }
    return has_item_with( [effect]( const item & it ) {
        return it.has_effect_when_carried( effect );
    } );
}

bool Character::is_wearing( const itype_id &it ) const
{
    for( auto &i : worn ) {
        if( i.typeId() == it ) {
            return true;
        }
    }
    return false;
}

bool Character::is_wearing_on_bp( const itype_id &it, body_part bp ) const
{
    for( auto &i : worn ) {
        if( i.typeId() == it && i.covers( bp ) ) {
            return true;
        }
    }
    return false;
}

bool Character::worn_with_flag( const std::string &flag, body_part bp ) const
{
    return std::any_of( worn.begin(), worn.end(), [&flag, bp]( const item & it ) {
        return it.has_flag( flag ) && ( bp == num_bp || it.covers( bp ) );
    } );
}

std::vector<std::string> Character::get_overlay_ids() const
{
    std::vector<std::string> rval;
    std::multimap<int, std::string> mutation_sorting;
    int order;
    std::string overlay_id;

    // first get effects
    for( const auto &eff_pr : *effects ) {
        rval.push_back( "effect_" + eff_pr.first.str() );
    }

    // then get mutations
    for( const auto &mut : my_mutations ) {
        overlay_id = ( mut.second.powered ? "active_" : "" ) + mut.first.str();
        order = get_overlay_order_of_mutation( overlay_id );
        mutation_sorting.insert( std::pair<int, std::string>( order, overlay_id ) );
    }

    // then get bionics
    for( const bionic &bio : *my_bionics ) {
        overlay_id = ( bio.powered ? "active_" : "" ) + bio.id.str();
        order = get_overlay_order_of_mutation( overlay_id );
        mutation_sorting.insert( std::pair<int, std::string>( order, overlay_id ) );
    }

    for( auto &mutorder : mutation_sorting ) {
        rval.push_back( "mutation_" + mutorder.second );
    }

    // next clothing
    // TODO: worry about correct order of clothing overlays
    for( const item &worn_item : worn ) {
        rval.push_back( "worn_" + worn_item.typeId() );
    }

    // last weapon
    // TODO: might there be clothing that covers the weapon?
    if( is_armed() ) {
        rval.push_back( "wielded_" + weapon.typeId() );
    }

    if( move_mode != CMM_WALK ) {
        rval.push_back( character_movemode_str[ move_mode ] );
    }
    return rval;
}

const SkillLevelMap &Character::get_all_skills() const
{
    return *_skills;
}

const SkillLevel &Character::get_skill_level_object( const skill_id &ident ) const
{
    return _skills->get_skill_level_object( ident );
}

SkillLevel &Character::get_skill_level_object( const skill_id &ident )
{
    return _skills->get_skill_level_object( ident );
}

int Character::get_skill_level( const skill_id &ident ) const
{
    return _skills->get_skill_level( ident );
}

int Character::get_skill_level( const skill_id &ident, const item &context ) const
{
    return _skills->get_skill_level( ident, context );
}

void Character::set_skill_level( const skill_id &ident, const int level )
{
    get_skill_level_object( ident ).level( level );
}

void Character::mod_skill_level( const skill_id &ident, const int delta )
{
    _skills->mod_skill_level( ident, delta );
}

std::string Character::enumerate_unmet_requirements( const item &it, const item &context ) const
{
    std::vector<std::string> unmet_reqs;

    const auto check_req = [ &unmet_reqs ]( const std::string & name, int cur, int req ) {
        if( cur < req ) {
            unmet_reqs.push_back( string_format( "%s %d", name, req ) );
        }
    };

    check_req( _( "strength" ),     get_str(), it.get_min_str() );
    check_req( _( "dexterity" ),    get_dex(), it.type->min_dex );
    check_req( _( "intelligence" ), get_int(), it.type->min_int );
    check_req( _( "perception" ),   get_per(), it.type->min_per );

    for( const auto &elem : it.type->min_skills ) {
        check_req( context.contextualize_skill( elem.first )->name(),
                   get_skill_level( elem.first, context ),
                   elem.second );
    }

    return enumerate_as_string( unmet_reqs );
}

bool Character::meets_skill_requirements( const std::map<skill_id, int> &req,
        const item &context ) const
{
    return _skills->meets_skill_requirements( req, context );
}

bool Character::meets_skill_requirements( const construction &con ) const
{
    return std::all_of( con.required_skills.begin(), con.required_skills.end(),
    [&]( const std::pair<skill_id, int> &pr ) {
        return get_skill_level( pr.first ) >= pr.second;
    } );
}

bool Character::meets_stat_requirements( const item &it ) const
{
    return get_str() >= it.get_min_str() &&
           get_dex() >= it.type->min_dex &&
           get_int() >= it.type->min_int &&
           get_per() >= it.type->min_per;
}

bool Character::meets_requirements( const item &it, const item &context ) const
{
    const auto &ctx = !context.is_null() ? context : it;
    return meets_stat_requirements( it ) && meets_skill_requirements( it.type->min_skills, ctx );
}

void Character::make_bleed( body_part bp, time_duration duration, int intensity,
                            bool permanent, bool force, bool defferred )
{
    int b_resist = 0;
    for( const trait_id &mut : get_mutations() ) {
        b_resist += mut.obj().bleed_resist;
    }

    if( b_resist > intensity ) {
        return;
    }

    add_effect( effect_bleed, duration, bp, permanent, intensity, force, defferred );
}

void Character::normalize()
{
    Creature::normalize();

    weapon   = item( "null", 0 );

    recalc_hp();
}

// Actual player death is mostly handled in game::is_game_over
void Character::die( Creature *nkiller )
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

void Character::apply_skill_boost()
{
    for( const skill_boost &boost : skill_boost::get_all() ) {
        // For migration, reset previously applied bonus.
        // Remove after 0.E or so.
        const std::string bonus_name = boost.stat() + std::string( "_bonus" );
        std::string previous_bonus = get_value( bonus_name );
        if( !previous_bonus.empty() ) {
            if( boost.stat() == "str" ) {
                str_max -= atoi( previous_bonus.c_str() );
            } else if( boost.stat() == "dex" ) {
                dex_max -= atoi( previous_bonus.c_str() );
            } else if( boost.stat() == "int" ) {
                int_max -= atoi( previous_bonus.c_str() );
            } else if( boost.stat() == "per" ) {
                per_max -= atoi( previous_bonus.c_str() );
            }
            remove_value( bonus_name );
        }
        // End migration code
        int skill_total = 0;
        for( const std::string &skill_str : boost.skills() ) {
            skill_total += get_skill_level( skill_id( skill_str ) );
        }
        mod_stat( boost.stat(), boost.calc_bonus( skill_total ) );
        if( boost.stat() == "str" ) {
            recalc_hp();
        }
    }
}

void Character::reset_stats()
{
    // Bionic buffs
    if( has_active_bionic( bionic_id( "bio_hydraulics" ) ) ) {
        mod_str_bonus( 20 );
    }

    mod_str_bonus( get_mod_stat_from_bionic( STRENGTH ) );
    mod_dex_bonus( get_mod_stat_from_bionic( DEXTERITY ) );
    mod_per_bonus( get_mod_stat_from_bionic( PERCEPTION ) );
    mod_int_bonus( get_mod_stat_from_bionic( INTELLIGENCE ) );

    // Trait / mutation buffs
    mod_str_bonus( std::floor( mutation_value( "str_modifier" ) ) );
    mod_dodge_bonus( std::floor( mutation_value( "dodge_modifier" ) ) );

    /** @EFFECT_STR_MAX above 15 decreases Dodge bonus by 1 (NEGATIVE) */
    if( str_max >= 16 ) {
        mod_dodge_bonus( -1 );   // Penalty if we're huge
    }
    /** @EFFECT_STR_MAX below 6 increases Dodge bonus by 1 */
    else if( str_max <= 5 ) {
        mod_dodge_bonus( 1 );   // Bonus if we're small
    }

    apply_skill_boost();

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
        nv = ( worn_with_flag( "GNV_EFFECT" ) ||
               has_active_bionic( bionic_id( "bio_night_vision" ) ) ||
               has_effect_with_flag( "EFFECT_NIGHT_VISION" ) );
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
    units::mass ret = 0_gram;
    units::mass wornWeight = std::accumulate( worn.begin(), worn.end(), 0_gram,
    []( units::mass sum, const item & itm ) {
        return sum + itm.weight();
    } );

    ret += bodyweight();       // The base weight of the player's body
    ret += inv.weight();           // Weight of the stored inventory
    ret += wornWeight;             // Weight of worn items
    ret += weapon.weight();        // Weight of wielded item
    ret += bionics_weight();       // Weight of installed bionics
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

static void layer_item( std::array<encumbrance_data, num_bp> &vals,
                        const item &it,
                        std::array<layer_level, num_bp> &highest_layer_so_far,
                        bool power_armor, const Character &c )
{
    const auto item_layer = it.get_layer();
    int encumber_val = it.get_encumber( c );
    // For the purposes of layering penalty, set a min of 2 and a max of 10 per item.
    int layering_encumbrance = std::min( 10, std::max( 2, encumber_val ) );

    /*
     * Setting layering_encumbrance to 0 at this point makes the item cease to exist
     * for the purposes of the layer penalty system. (normally an item has a minimum
     * layering_encumbrance of 2 )
     */
    if( it.has_flag( "SEMITANGIBLE" ) ) {
        encumber_val = 0;
        layering_encumbrance = 0;
    }

    const int armorenc = !power_armor || !it.is_power_armor() ?
                         encumber_val : std::max( 0, encumber_val - 40 );

    body_part_set covered_parts = it.get_covered_body_parts();
    for( const body_part bp : all_body_parts ) {
        if( !covered_parts.test( bp ) ) {
            continue;
        }
        highest_layer_so_far[bp] =
            std::max( highest_layer_so_far[bp], item_layer );

        // Apply layering penalty to this layer, as well as any layer worn
        // within it that would normally be worn outside of it.
        for( layer_level penalty_layer = item_layer;
             penalty_layer <= highest_layer_so_far[bp]; ++penalty_layer ) {
            vals[bp].layer( penalty_layer, layering_encumbrance );
        }

        vals[bp].armor_encumbrance += armorenc;
    }
}

bool Character::is_wearing_power_armor( bool *hasHelmet ) const
{
    bool result = false;
    for( auto &elem : worn ) {
        if( !elem.is_power_armor() ) {
            continue;
        }
        if( hasHelmet == nullptr ) {
            // found power armor, helmet not requested, cancel loop
            return true;
        }
        // found power armor, continue search for helmet
        result = true;
        if( elem.covers( bp_head ) ) {
            *hasHelmet = true;
            return true;
        }
    }
    return result;
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

bool Character::is_wearing_active_optcloak() const
{
    for( auto &w : worn ) {
        if( w.active && w.has_flag( "ACTIVE_CLOAKING" ) ) {
            return true;
        }
    }
    return false;
}

void layer_details::reset()
{
    *this = layer_details();
}

// The stacking penalty applies by doubling the encumbrance of
// each item except the highest encumbrance one.
// So we add them together and then subtract out the highest.
int layer_details::layer( const int encumbrance )
{
    /*
     * We should only get to this point with an encumbrance value of 0
     * if the item is 'semitangible'. A normal item has a minimum of
     * 2 encumbrance for layer penalty purposes.
     * ( even if normally its encumbrance is 0 )
     */
    if( encumbrance == 0 ) {
        return total; // skip over the other logic because this item doesn't count
    }

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

std::list<item>::iterator Character::position_to_wear_new_item( const item &new_item )
{
    // By default we put this item on after the last item on the same or any
    // lower layer.
    return std::find_if(
               worn.rbegin(), worn.rend(),
    [&]( const item & w ) {
        return w.get_layer() <= new_item.get_layer();
    }
           ).base();
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

    // Figure out where new_item would be worn
    std::list<item>::const_iterator new_item_position = worn.end();
    if( !new_item.is_null() ) {
        // const_cast required to work around g++-4.8 library bug
        // see the commit that added this comment to understand why
        new_item_position =
            const_cast<Character *>( this )->position_to_wear_new_item( new_item );
    }

    // Track highest layer observed so far so we can penalise out-of-order
    // items
    std::array<layer_level, num_bp> highest_layer_so_far;
    std::fill( highest_layer_so_far.begin(), highest_layer_so_far.end(),
               PERSONAL_LAYER );

    const bool power_armored = is_wearing_active_power_armor();
    for( auto w_it = worn.begin(); w_it != worn.end(); ++w_it ) {
        if( w_it == new_item_position ) {
            layer_item( vals, new_item, highest_layer_so_far, power_armored, *this );
        }
        layer_item( vals, *w_it, highest_layer_so_far, power_armored, *this );
    }

    if( worn.end() == new_item_position && !new_item.is_null() ) {
        layer_item( vals, new_item, highest_layer_so_far, power_armored, *this );
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

static void apply_mut_encumbrance( std::array<encumbrance_data, num_bp> &vals,
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

    for( const bionic &bio : *my_bionics ) {
        for( const auto &element : bio.info().encumbrance ) {
            vals[element.first].encumbrance += element.second;
        }
    }

    if( has_active_bionic( bionic_id( "bio_shock_absorber" ) ) ) {
        for( auto &val : vals ) {
            val.encumbrance += 3; // Slight encumbrance to all parts except eyes
        }
        vals[bp_eyes].encumbrance -= 3;
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
    return std::max( 0, get_str_base() + str_bonus );
}
int Character::get_dex() const
{
    return std::max( 0, get_dex_base() + dex_bonus );
}
int Character::get_per() const
{
    return std::max( 0, get_per_base() + per_bonus );
}
int Character::get_int() const
{
    return std::max( 0, get_int_base() + int_bonus );
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

static int get_speedydex_bonus( const int dex )
{
    // this is the number to be multiplied by the increment
    const int modified_dex = std::max( dex - get_option<int>( "SPEEDYDEX_MIN_DEX" ), 0 );
    return modified_dex * get_option<int>( "SPEEDYDEX_DEX_SPEED" );
}

int Character::get_speed() const
{
    return Creature::get_speed() + get_speedydex_bonus( get_dex() );
}

int Character::ranged_dex_mod() const
{
    ///\EFFECT_DEX <20 increases ranged penalty
    return std::max( ( 20.0 - get_dex() ) * 0.5, 0.0 );
}

int Character::ranged_per_mod() const
{
    ///\EFFECT_PER <20 increases ranged aiming penalty.
    return std::max( ( 20.0 - get_per() ) * 1.2, 0.0 );
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

void Character::set_str_bonus( int nstr )
{
    str_bonus = nstr;
    str_cur = std::max( 0, str_max + str_bonus );
}
void Character::set_dex_bonus( int ndex )
{
    dex_bonus = ndex;
    dex_cur = std::max( 0, dex_max + dex_bonus );
}
void Character::set_per_bonus( int nper )
{
    per_bonus = nper;
    per_cur = std::max( 0, per_max + per_bonus );
}
void Character::set_int_bonus( int nint )
{
    int_bonus = nint;
    int_cur = std::max( 0, int_max + int_bonus );
}
void Character::mod_str_bonus( int nstr )
{
    str_bonus += nstr;
    str_cur = std::max( 0, str_max + str_bonus );
}
void Character::mod_dex_bonus( int ndex )
{
    dex_bonus += ndex;
    dex_cur = std::max( 0, dex_max + dex_bonus );
}
void Character::mod_per_bonus( int nper )
{
    per_bonus += nper;
    per_cur = std::max( 0, per_max + per_bonus );
}
void Character::mod_int_bonus( int nint )
{
    int_bonus += nint;
    int_cur = std::max( 0, int_max + int_bonus );
}

namespace io
{
template<>
std::string enum_to_string<Character::stat>( Character::stat data )
{
    switch( data ) {
        // *INDENT-OFF*
    case Character::stat::STRENGTH:     return "STR";
    case Character::stat::DEXTERITY:    return "DEX";
    case Character::stat::INTELLIGENCE: return "INT";
    case Character::stat::PERCEPTION:   return "PER";

        // *INDENT-ON*
        case Character::stat::DUMMY_STAT:
            break;
    }
    abort();
}
} // namespace io

void Character::set_healthy( int nhealthy )
{
    healthy = nhealthy;
}
void Character::mod_healthy( int nhealthy )
{
    float mut_rate = 1.0f;
    for( const trait_id &mut : get_mutations() ) {
        mut_rate *= mut.obj().healthy_rate;
    }
    healthy += nhealthy * mut_rate;
}
void Character::set_healthy_mod( int nhealthy_mod )
{
    healthy_mod = nhealthy_mod;
}
void Character::mod_healthy_mod( int nhealthy_mod, int cap )
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
    if( ( healthy_mod <= low_cap && nhealthy_mod < 0 ) ||
        ( healthy_mod >= high_cap && nhealthy_mod > 0 ) ) {
        return;
    }

    healthy_mod += nhealthy_mod;

    // Since we already bailed out if we were out-of-bounds, we can
    // just clamp to the boundaries here.
    healthy_mod = std::min( healthy_mod, high_cap );
    healthy_mod = std::max( healthy_mod, low_cap );
}

int Character::get_stored_kcal() const
{
    return stored_calories;
}

void Character::mod_stored_kcal( int nkcal )
{
    set_stored_kcal( stored_calories + nkcal );
}

void Character::mod_stored_nutr( int nnutr )
{
    // nutr is legacy type code, this function simply converts old nutrition to new kcal
    mod_stored_kcal( -1 * round( nnutr * 2500.0f / ( 12 * 24 ) ) );
}

void Character::set_stored_kcal( int kcal )
{
    if( stored_calories != kcal ) {
        stored_calories = kcal;

        //some mutant change their max_hp according to their bmi
        recalc_hp();
    }
}

int Character::get_healthy_kcal() const
{
    return healthy_calories;
}

float Character::get_kcal_percent() const
{
    return static_cast<float>( get_stored_kcal() ) / static_cast<float>( get_healthy_kcal() );
}

int Character::get_hunger() const
{
    return hunger;
}

void Character::mod_hunger( int nhunger )
{
    set_hunger( hunger + nhunger );
}

void Character::set_hunger( int nhunger )
{
    if( hunger != nhunger ) {
        // cap hunger at 300, just below famished
        hunger = std::min( 300, nhunger );
        on_stat_change( "hunger", hunger );
    }
}

// this is a translation from a legacy value
int Character::get_starvation() const
{
    static const std::vector<std::pair<float, float>> starv_thresholds = { {
            std::make_pair( 0.0f, 6000.0f ),
            std::make_pair( 0.8f, 300.0f ),
            std::make_pair( 0.95f, 100.0f )
        }
    };
    if( get_kcal_percent() < 0.95f ) {
        return round( multi_lerp( starv_thresholds, get_kcal_percent() ) );
    }
    return 0;
}

int Character::get_thirst() const
{
    return thirst;
}

std::pair<std::string, nc_color> Character::get_thirst_description() const
{
    // some delay from water in stomach is desired, but there needs to be some visceral response
    int thirst = get_thirst() - ( std::max( units::to_milliliter<int>( g->u.stomach.get_water() ) / 10,
                                            0 ) );
    std::string hydration_string;
    nc_color hydration_color = c_white;
    if( thirst > 520 ) {
        hydration_color = c_light_red;
        hydration_string = _( "Parched" );
    } else if( thirst > 240 ) {
        hydration_color = c_light_red;
        hydration_string = _( "Dehydrated" );
    } else if( thirst > 80 ) {
        hydration_color = c_yellow;
        hydration_string = _( "Very thirsty" );
    } else if( thirst > 40 ) {
        hydration_color = c_yellow;
        hydration_string = _( "Thirsty" );
    } else if( thirst < -60 ) {
        hydration_color = c_green;
        hydration_string = _( "Turgid" );
    } else if( thirst < -20 ) {
        hydration_color = c_green;
        hydration_string = _( "Hydrated" );
    } else if( thirst < 0 ) {
        hydration_color = c_green;
        hydration_string = _( "Slaked" );
    }
    return std::make_pair( hydration_string, hydration_color );
}

std::pair<std::string, nc_color> Character::get_hunger_description() const
{
    int hunger = get_hunger();
    std::string hunger_string;
    nc_color hunger_color = c_white;
    if( hunger >= 300 && get_starvation() > 2500 ) {
        hunger_color = c_red;
        hunger_string = _( "Starving!" );
    } else if( hunger >= 300 && get_starvation() > 1100 ) {
        hunger_color = c_light_red;
        hunger_string = _( "Near starving" );
    } else if( hunger > 250 ) {
        hunger_color = c_light_red;
        hunger_string = _( "Famished" );
    } else if( hunger > 100 ) {
        hunger_color = c_yellow;
        hunger_string = _( "Very hungry" );
    } else if( hunger > 40 ) {
        hunger_color = c_yellow;
        hunger_string = _( "Hungry" );
    } else if( hunger < -60 ) {
        hunger_color = c_yellow;
        hunger_string = _( "Engorged" );
    } else if( hunger < -20 ) {
        hunger_color = c_green;
        hunger_string = _( "Sated" );
    } else if( hunger < 0 ) {
        hunger_color = c_green;
        hunger_string = _( "Full" );
    }
    return std::make_pair( hunger_string, hunger_color );
}

std::pair<std::string, nc_color> Character::get_fatigue_description() const
{
    int fatigue = get_fatigue();
    std::string fatigue_string;
    nc_color fatigue_color = c_white;
    if( fatigue > EXHAUSTED ) {
        fatigue_color = c_red;
        fatigue_string = _( "Exhausted" );
    } else if( fatigue > DEAD_TIRED ) {
        fatigue_color = c_light_red;
        fatigue_string = _( "Dead Tired" );
    } else if( fatigue > TIRED ) {
        fatigue_color = c_yellow;
        fatigue_string = _( "Tired" );
    }
    return std::make_pair( fatigue_string, fatigue_color );
}

void Character::mod_thirst( int nthirst )
{
    set_thirst( thirst + nthirst );
}

void Character::set_thirst( int nthirst )
{
    if( thirst != nthirst ) {
        thirst = nthirst;
        on_stat_change( "thirst", thirst );
    }
}

void Character::mod_fatigue( int nfatigue )
{
    set_fatigue( fatigue + nfatigue );
}

void Character::mod_sleep_deprivation( int nsleep_deprivation )
{
    set_sleep_deprivation( sleep_deprivation + nsleep_deprivation );
}

void Character::set_fatigue( int nfatigue )
{
    nfatigue = std::max( nfatigue, -1000 );
    if( fatigue != nfatigue ) {
        fatigue = nfatigue;
        on_stat_change( "fatigue", fatigue );
    }
}

void Character::set_sleep_deprivation( int nsleep_deprivation )
{
    sleep_deprivation = std::min( static_cast< int >( SLEEP_DEPRIVATION_MASSIVE ), std::max( 0,
                                  nsleep_deprivation ) );
}

int Character::get_fatigue() const
{
    return fatigue;
}

int Character::get_sleep_deprivation() const
{
    if( !get_option< bool >( "SLEEP_DEPRIVATION" ) ) {
        return 0;
    }

    return sleep_deprivation;
}

void Character::on_damage_of_type( int adjusted_damage, damage_type type, body_part bp )
{
    // Electrical damage has a chance to temporarily incapacitate bionics in the damaged body_part.
    if( type == DT_ELECTRIC ) {
        const time_duration min_disable_time = 10_turns * adjusted_damage;
        for( auto &i : *my_bionics ) {
            if( !i.powered ) {
                // Unpowered bionics are protected from power surges.
                continue;
            }
            const auto &info = i.info();
            if( info.shockproof || info.faulty ) {
                continue;
            }
            const auto &bodyparts = info.occupied_bodyparts;
            if( bodyparts.find( bp ) != bodyparts.end() ) {
                const int bp_hp = hp_cur[bp_to_hp( bp )];
                // The chance to incapacitate is as high as 50% if the attack deals damage equal to one third of the body part's current health.
                if( x_in_y( adjusted_damage * 3, bp_hp ) && one_in( 2 ) ) {
                    if( i.incapacitated_time == 0_turns ) {
                        add_msg_if_player( m_bad, _( "Your %s bionic shorts out!" ), info.name );
                    }
                    i.incapacitated_time += rng( min_disable_time, 10 * min_disable_time );
                }
            }
        }
    }
}

void Character::reset_bonuses()
{
    // Reset all bonuses to 0 and multipliers to 1.0
    str_bonus = 0;
    dex_bonus = 0;
    per_bonus = 0;
    int_bonus = 0;

    Creature::reset_bonuses();
}

int Character::get_max_healthy() const
{
    const float bmi = get_bmi();
    return clamp( static_cast<int>( round( -3 * ( bmi - character_weight_category::normal ) *
                                           ( bmi - character_weight_category::overweight ) + 200 ) ), -200, 200 );
}

void Character::update_health( int external_modifiers )
{
    if( has_artifact_with( AEP_SICK ) ) {
        // Carrying a sickness artifact makes your health 50 points worse on average
        external_modifiers -= 50;
    }
    // Limit healthy_mod to [-200, 200].
    // This also sets approximate bounds for the character's health.
    if( get_healthy_mod() > get_max_healthy() ) {
        set_healthy_mod( get_max_healthy() );
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

hp_part Character::body_window( const std::string &menu_header,
                                bool show_all, bool precise,
                                int normal_bonus, int head_bonus, int torso_bonus,
                                float bleed, float bite, float infect, float bandage_power, float disinfectant_power ) const
{
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
            { false, bp_head, hp_head, _( "Head" ), head_bonus },
            { false, bp_torso, hp_torso, _( "Torso" ), torso_bonus },
            { false, bp_arm_l, hp_arm_l, _( "Left Arm" ), normal_bonus },
            { false, bp_arm_r, hp_arm_r, _( "Right Arm" ), normal_bonus },
            { false, bp_leg_l, hp_leg_l, _( "Left Leg" ), normal_bonus },
            { false, bp_leg_r, hp_leg_r, _( "Right Leg" ), normal_bonus },
        }
    };

    int max_bp_name_len = 0;
    for( const auto &e : parts ) {
        max_bp_name_len = std::max( max_bp_name_len, utf8_width( e.name ) );
    }

    const auto hp_str = [precise]( const int hp, const int maximal_hp ) -> std::string {
        if( hp <= 0 )
        {
            return "==%==";
        } else if( precise )
        {
            return string_format( "%d", hp );
        } else
        {
            std::string h_bar = get_hp_bar( hp, maximal_hp, false ).first;
            nc_color h_bar_col = get_hp_bar( hp, maximal_hp, false ).second;

            return colorize( h_bar, h_bar_col ) + colorize( std::string( 5 - utf8_width( h_bar ),
                    '.' ), c_white );
        }
    };

    uilist bmenu;
    bmenu.desc_enabled = true;
    bmenu.text = menu_header;

    bmenu.hilight_disabled = true;
    bool is_valid_choice = false;

    for( size_t i = 0; i < parts.size(); i++ ) {
        const auto &e = parts[i];
        const body_part bp = e.bp;
        const hp_part hp = e.hp;
        const int maximal_hp = hp_max[hp];
        const int current_hp = hp_cur[hp];
        // This will c_light_gray if the part does not have any effects cured by the item/effect
        // (e.g. it cures only bites, but the part does not have a bite effect)
        const nc_color state_col = limb_color( bp, bleed > 0.0f, bite > 0.0f, infect > 0.0f );
        const bool has_curable_effect = state_col != c_light_gray;
        // The same as in the main UI sidebar. Independent of the capability of the healing item/effect!
        const nc_color all_state_col = limb_color( bp, true, true, true );
        // Broken means no HP can be restored, it requires surgical attention.
        const bool limb_is_broken = current_hp == 0;

        if( show_all ) {
            e.allowed = true;
        } else if( has_curable_effect ) {
            e.allowed = true;
        } else if( limb_is_broken ) {
            e.allowed = false;
        } else if( current_hp < maximal_hp && ( e.bonus != 0 || bandage_power > 0.0f  ||
                                                disinfectant_power > 0.0f ) ) {
            e.allowed = true;
        } else {
            e.allowed = false;
        }

        std::stringstream msg;
        std::stringstream desc;
        bool bleeding = has_effect( effect_bleed, e.bp );
        bool bitten = has_effect( effect_bite, e.bp );
        bool infected = has_effect( effect_infected, e.bp );
        bool bandaged = has_effect( effect_bandaged, e.bp );
        bool disinfected = has_effect( effect_disinfected, e.bp );
        const int b_power = get_effect_int( effect_bandaged, e.bp );
        const int d_power = get_effect_int( effect_disinfected, e.bp );
        int new_b_power = static_cast<int>( std::floor( bandage_power ) );
        if( bandaged ) {
            const effect &eff = get_effect( effect_bandaged, e.bp );
            if( new_b_power > eff.get_max_intensity() ) {
                new_b_power = eff.get_max_intensity();
            }

        }
        int new_d_power = static_cast<int>( std::floor( disinfectant_power ) );

        const auto &aligned_name = std::string( max_bp_name_len - utf8_width( e.name ), ' ' ) + e.name;
        msg << colorize( aligned_name, all_state_col ) << " " << hp_str( current_hp, maximal_hp );

        if( limb_is_broken ) {
            desc << colorize( _( "It is broken.  It needs a splint or surgical attention." ), c_red ) << "\n";
        }

        // BLEEDING block
        if( bleeding ) {
            desc << colorize( string_format( "%s: %s", get_effect( effect_bleed, e.bp ).get_speed_name(),
                                             get_effect( effect_bleed, e.bp ).disp_short_desc() ), c_red ) << "\n";
            if( bleed > 0.0f ) {
                desc << colorize( string_format( _( "Chance to stop: %d %%" ),
                                                 static_cast<int>( bleed * 100 ) ), c_light_green ) << "\n";
            } else {
                desc << colorize( _( "This will not stop the bleeding." ),
                                  c_yellow ) << "\n";
            }
        }
        // BANDAGE block
        if( bandaged ) {
            desc << string_format( _( "Bandaged [%s]" ), texitify_healing_power( b_power ) ) << "\n";
            if( new_b_power > b_power ) {
                desc << colorize( string_format( _( "Expected quality improvement: %s" ),
                                                 texitify_healing_power( new_b_power ) ), c_light_green ) << "\n";
            } else if( new_b_power > 0 ) {
                desc << colorize( _( "You don't expect any improvement from using this." ), c_yellow ) << "\n";
            }
        } else if( new_b_power > 0 && e.allowed ) {
            desc << colorize( string_format( _( "Expected bandage quality: %s" ),
                                             texitify_healing_power( new_b_power ) ), c_light_green ) << "\n";
        }
        // BITTEN block
        if( bitten ) {
            desc << colorize( string_format( "%s: ", get_effect( effect_bite,
                                             e.bp ).get_speed_name() ), c_red );
            desc << colorize( _( "It has a deep bite wound that needs cleaning." ), c_red ) << "\n";
            if( bite > 0 ) {
                desc << colorize( string_format( _( "Chance to clean and disinfect: %d %%" ),
                                                 static_cast<int>( bite * 100 ) ), c_light_green ) << "\n";
            } else {
                desc << colorize( _( "This will not help in cleaning this wound." ), c_yellow ) << "\n";
            }
        }
        // INFECTED block
        if( infected ) {
            desc << colorize( string_format( "%s: ", get_effect( effect_infected,
                                             e.bp ).get_speed_name() ), c_red );
            desc << colorize( _( "It has a deep wound that looks infected.  Antibiotics might be required." ),
                              c_red ) << "\n";
            if( infect > 0 ) {
                desc << colorize( string_format( _( "Chance to heal infection: %d %%" ),
                                                 static_cast<int>( infect * 100 ) ), c_light_green ) << "\n";
            } else {
                desc << colorize( _( "This will not help in healing infection." ), c_yellow ) << "\n";
            }
        }
        // DISINFECTANT (general) block
        if( disinfected ) {
            desc << string_format( _( "Disinfected [%s]" ),
                                   texitify_healing_power( d_power ) ) << "\n";
            if( new_d_power > d_power ) {
                desc << colorize( string_format( _( "Expected quality improvement: %s" ),
                                                 texitify_healing_power( new_d_power ) ), c_light_green ) << "\n";
            } else if( new_d_power > 0 ) {
                desc << colorize( _( "You don't expect any improvement from using this." ),
                                  c_yellow ) << "\n";
            }
        } else if( new_d_power > 0 && e.allowed ) {
            desc << colorize( string_format(
                                  _( "Expected disinfection quality: %s" ),
                                  texitify_healing_power( new_d_power ) ), c_light_green ) << "\n";
        }
        // END of blocks

        if( ( !e.allowed && !limb_is_broken ) || ( show_all && current_hp == maximal_hp &&
                !limb_is_broken && !bitten && !infected && !bleeding ) ) {
            desc << colorize( _( "Healthy." ), c_green ) << "\n";
        }
        if( !e.allowed ) {
            desc << colorize( _( "You don't expect any effect from using this." ), c_yellow );
        } else {
            is_valid_choice = true;
        }
        bmenu.addentry_desc( i, e.allowed, MENU_AUTOASSIGN, msg.str(), desc.str() );
    }

    if( !is_valid_choice ) { // no body part can be chosen for this item/effect
        bmenu.init();
        bmenu.desc_enabled = false;
        bmenu.text = _( "No limb would benefit from it." );
        bmenu.addentry( parts.size(), true, 'q', "%s", _( "Cancel" ) );
    }

    bmenu.query();
    if( bmenu.ret >= 0 && static_cast<size_t>( bmenu.ret ) < parts.size() &&
        parts[bmenu.ret].allowed ) {
        return parts[bmenu.ret].hp;
    } else {
        return num_hp_parts;
    }
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

std::vector<std::string> Character::get_grammatical_genders() const
{
    if( male ) {
        return { "m" };
    } else {
        return { "f" };
    }
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

    // Priority: electricity, fire, acid, gases
    bool has_elec = false;
    bool has_fire = false;
    bool has_acid = false;
    bool has_fume = false;
    for( const auto &field : fields ) {
        has_elec = field.first.obj().has_elec;
        if( has_elec ) {
            return hilite( basic );
        }
        has_fire = field.first.obj().has_fire;
        has_acid = field.first.obj().has_acid;
        has_fume = field.first.obj().has_fume;
    }
    if( has_fire ) {
        return red_background( basic );
    }
    if( has_acid ) {
        return green_background( basic );
    }
    if( has_fume ) {
        return white_background( basic );
    }
    if( in_sleep_state() ) {
        return hilite( basic );
    }
    return basic;
}

bool Character::is_immune_field( const field_type_id fid ) const
{
    // Obviously this makes us invincible
    if( has_trait( debug_nodmg ) ) {
        return true;
    }
    // Check to see if we are immune
    const field_type &ft = fid.obj();
    for( const trait_id &t : ft.immunity_data_traits ) {
        if( has_trait( t ) ) {
            return true;
        }
    }
    bool immune_by_body_part_resistance = !ft.immunity_data_body_part_env_resistance.empty();
    for( const std::pair<body_part, int> &fide : ft.immunity_data_body_part_env_resistance ) {
        immune_by_body_part_resistance = immune_by_body_part_resistance &&
                                         get_env_resist( fide.first ) >= fide.second;
    }
    if( immune_by_body_part_resistance ) {
        return true;
    }
    if( ft.has_elec ) {
        return is_elec_immune();
    }
    if( ft.has_fire ) {
        return has_active_bionic( bionic_id( "bio_heatsink" ) ) || is_wearing( "rm13_armor_on" );
    }
    if( ft.has_acid ) {
        return !is_on_ground() && get_env_resist( bp_foot_l ) >= 15 &&
               get_env_resist( bp_foot_r ) >= 15 &&
               get_env_resist( bp_leg_l ) >= 15 &&
               get_env_resist( bp_leg_r ) >= 15 &&
               get_armor_type( DT_ACID, bp_foot_l ) >= 5 &&
               get_armor_type( DT_ACID, bp_foot_r ) >= 5 &&
               get_armor_type( DT_ACID, bp_leg_l ) >= 5 &&
               get_armor_type( DT_ACID, bp_leg_r ) >= 5;
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
    if( ( tmp.weight() / 113_gram ) > static_cast<int>( str_cur * 15 ) ) {
        return 0;
    }
    // Increases as weight decreases until 150 g, then decreases again
    /** @EFFECT_STR increases throwing range, vs item weight (high or low) */
    int str_override = str_cur;
    if( is_mounted() ) {
        auto mons = mounted_creature.get();
        str_override = mons->mech_str_addition() != 0 ? mons->mech_str_addition() : str_cur;
    }
    int ret = ( str_override * 10 ) / ( tmp.weight() >= 150_gram ? tmp.weight() / 113_gram : 10 -
                                        static_cast<int>(
                                            tmp.weight() / 15_gram ) );
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
    if( ret > str_override * 3 + get_skill_level( skill_throw ) ) {
        return str_override * 3 + get_skill_level( skill_throw );
    }

    return ret;
}

const std::vector<material_id> Character::fleshy = { material_id( "flesh" ), material_id( "hflesh" ) };
bool Character::made_of( const material_id &m ) const
{
    // TODO: check for mutations that change this.
    return std::find( fleshy.begin(), fleshy.end(), m ) != fleshy.end();
}
bool Character::made_of_any( const std::set<material_id> &ms ) const
{
    // TODO: check for mutations that change this.
    return std::any_of( fleshy.begin(), fleshy.end(), [&ms]( const material_id & e ) {
        return ms.count( e );
    } );
}

tripoint Character::global_square_location() const
{
    return g->m.getabs( position );
}

tripoint Character::global_sm_location() const
{
    return ms_to_sm_copy( global_square_location() );
}

tripoint Character::global_omt_location() const
{
    return ms_to_omt_copy( global_square_location() );
}

bool Character::is_blind() const
{
    return ( worn_with_flag( "BLIND" ) ||
             has_effect( effect_blind ) ||
             has_active_bionic( bionic_id( "bio_blindfold" ) ) );
}

bool Character::is_invisible() const
{
    static const bionic_id str_bio_cloak( "bio_cloak" ); // This function used in monster::plan_moves
    static const bionic_id str_bio_night( "bio_night" );
    return (
               has_effect_with_flag( "EFFECT_INVISIBLE" ) ||
               has_active_bionic( str_bio_cloak ) ||
               has_active_bionic( str_bio_night ) ||
               is_wearing_active_optcloak() ||
               has_trait( trait_DEBUG_CLOAK ) ||
               has_artifact_with( AEP_INVISIBLE )
           );
}

int Character::visibility( bool, int ) const
{
    // 0-100 %
    if( is_invisible() ) {
        return 0;
    }
    // TODO:
    // if ( dark_clothing() && light check ...
    int stealth_modifier = std::floor( mutation_value( "stealth_modifier" ) );
    return clamp( 100 - stealth_modifier, 40, 160 );
}

bool Character::pour_into( item &container, item &liquid )
{
    std::string err;
    const int amount = container.get_remaining_capacity_for_liquid( liquid, *this, &err );

    if( !err.empty() ) {
        add_msg_if_player( m_bad, err );
        return false;
    }

    add_msg_if_player( _( "You pour %1$s into the %2$s." ), liquid.tname(), container.tname() );

    container.fill_with( liquid, amount );
    inv.unsort();

    if( liquid.charges > 0 ) {
        add_msg_if_player( _( "There's some left over!" ) );
    }

    return true;
}

bool Character::pour_into( vehicle &veh, item &liquid )
{
    auto sel = [&]( const vehicle_part & pt ) {
        return pt.is_tank() && pt.can_reload( liquid );
    };

    auto stack = units::legacy_volume_factor / liquid.type->stack_size;
    auto title = string_format( _( "Select target tank for <color_%s>%.1fL %s</color>" ),
                                get_all_colors().get_name( liquid.color() ),
                                round_up( to_liter( liquid.charges * stack ), 1 ),
                                liquid.tname() );

    auto &tank = veh_interact::select_part( veh, sel, title );
    if( !tank ) {
        return false;
    }

    tank.fill_with( liquid );

    //~ $1 - vehicle name, $2 - part name, $3 - liquid type
    add_msg_if_player( _( "You refill the %1$s's %2$s with %3$s." ),
                       veh.name, tank.name(), liquid.type_name() );

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

int Character::ammo_count_for( const item &gun )
{
    int ret = item::INFINITE_CHARGES;
    if( !gun.is_gun() ) {
        return ret;
    }

    int required = gun.ammo_required();

    if( required > 0 ) {
        int total_ammo = 0;
        total_ammo += gun.ammo_remaining();

        bool has_mag = gun.magazine_integral();

        const auto found_ammo = find_ammo( gun, true, -1 );
        int loose_ammo = 0;
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

        ret = std::min( ret, total_ammo / required );
    }

    int ups_drain = gun.get_gun_ups_drain();
    if( ups_drain > 0 ) {
        ret = std::min( ret, charges_of( "UPS" ) / ups_drain );
    }

    return ret;
}

float Character::rest_quality() const
{
    // Just a placeholder for now.
    // TODO: Waiting/reading/being unconscious on bed/sofa/grass
    return has_effect( effect_sleep ) ? 1.0f : 0.0f;
}

hp_part Character::bp_to_hp( const body_part bp )
{
    switch( bp ) {
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
    switch( hpart ) {
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

std::vector<body_part> Character::get_all_body_parts( bool only_main ) const
{
    // TODO: Remove broken parts, parts removed by mutations etc.
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
        }
    };

    static const std::vector<body_part> main_bps = {{
            bp_head,
            bp_torso,
            bp_arm_l,
            bp_arm_r,
            bp_leg_l,
            bp_leg_r,
        }
    };

    return only_main ? main_bps : all_bps;
}

std::string Character::extended_description() const
{
    std::ostringstream ss;
    if( is_player() ) {
        // <bad>This is me, <player_name>.</bad>
        ss << string_format( _( "This is you - %s." ), name );
    } else {
        ss << string_format( _( "This is %s." ), name );
    }

    ss << std::endl << "--" << std::endl;

    const auto &bps = get_all_body_parts( true );
    // Find length of bp names, to align
    // accumulate looks weird here, any better function?
    int longest = std::accumulate( bps.begin(), bps.end(), 0,
    []( int m, body_part bp ) {
        return std::max( m, utf8_width( body_part_name_as_heading( bp, 1 ) ) );
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

        ss << colorize( left_justify( bp_heading, longest ), name_color );
        ss << colorize( hp_bar.first, hp_bar.second );
        // Trailing bars. UGLY!
        // TODO: Integrate into get_hp_bar somehow
        ss << colorize( std::string( 5 - utf8_width( hp_bar.first ), '.' ), c_white );
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
    ss << enumerate_as_string( worn.begin(), worn.end(), []( const item & it ) {
        return it.tname();
    } );

    return replace_colors( ss.str() );
}

social_modifiers Character::get_mutation_social_mods() const
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

template <float mutation_branch::*member>
float calc_mutation_value_additive( const std::vector<const mutation_branch *> &mutations )
{
    float ret = 0.0f;
    for( const mutation_branch *mut : mutations ) {
        ret += mut->*member;
    }
    return ret;
}

template <float mutation_branch::*member>
float calc_mutation_value_multiplicative( const std::vector<const mutation_branch *> &mutations )
{
    float ret = 1.0f;
    for( const mutation_branch *mut : mutations ) {
        ret *= mut->*member;
    }
    return ret;
}

static const std::map<std::string, std::function <float( std::vector<const mutation_branch *> )>>
mutation_value_map = {
    { "healing_awake", calc_mutation_value<&mutation_branch::healing_awake> },
    { "healing_resting", calc_mutation_value<&mutation_branch::healing_resting> },
    { "hp_modifier", calc_mutation_value<&mutation_branch::hp_modifier> },
    { "hp_modifier_secondary", calc_mutation_value<&mutation_branch::hp_modifier_secondary> },
    { "hp_adjustment", calc_mutation_value<&mutation_branch::hp_adjustment> },
    { "temperature_speed_modifier", calc_mutation_value<&mutation_branch::temperature_speed_modifier> },
    { "metabolism_modifier", calc_mutation_value<&mutation_branch::metabolism_modifier> },
    { "thirst_modifier", calc_mutation_value<&mutation_branch::thirst_modifier> },
    { "fatigue_regen_modifier", calc_mutation_value<&mutation_branch::fatigue_regen_modifier> },
    { "fatigue_modifier", calc_mutation_value<&mutation_branch::fatigue_modifier> },
    { "stamina_regen_modifier", calc_mutation_value<&mutation_branch::stamina_regen_modifier> },
    { "stealth_modifier", calc_mutation_value<&mutation_branch::stealth_modifier> },
    { "str_modifier", calc_mutation_value<&mutation_branch::str_modifier> },
    { "dodge_modifier", calc_mutation_value_additive<&mutation_branch::dodge_modifier> },
    { "mana_modifier", calc_mutation_value_additive<&mutation_branch::mana_modifier> },
    { "mana_multiplier", calc_mutation_value_multiplicative<&mutation_branch::mana_multiplier> },
    { "mana_regen_multiplier", calc_mutation_value_multiplicative<&mutation_branch::mana_regen_multiplier> },
    { "speed_modifier", calc_mutation_value_multiplicative<&mutation_branch::speed_modifier> },
    { "movecost_modifier", calc_mutation_value_multiplicative<&mutation_branch::movecost_modifier> },
    { "movecost_flatground_modifier", calc_mutation_value_multiplicative<&mutation_branch::movecost_flatground_modifier> },
    { "movecost_obstacle_modifier", calc_mutation_value_multiplicative<&mutation_branch::movecost_obstacle_modifier> },
    { "attackcost_modifier", calc_mutation_value_multiplicative<&mutation_branch::attackcost_modifier> },
    { "max_stamina_modifier", calc_mutation_value_multiplicative<&mutation_branch::max_stamina_modifier> },
    { "weight_capacity_modifier", calc_mutation_value_multiplicative<&mutation_branch::weight_capacity_modifier> },
    { "hearing_modifier", calc_mutation_value_multiplicative<&mutation_branch::hearing_modifier> },
    { "noise_modifier", calc_mutation_value_multiplicative<&mutation_branch::noise_modifier> },
    { "overmap_sight", calc_mutation_value_multiplicative<&mutation_branch::overmap_sight> },
    { "overmap_multiplier", calc_mutation_value_multiplicative<&mutation_branch::overmap_multiplier> },
    { "map_memory_capacity_multiplier", calc_mutation_value_multiplicative<&mutation_branch::map_memory_capacity_multiplier> },
    { "skill_rust_multiplier", calc_mutation_value_multiplicative<&mutation_branch::skill_rust_multiplier> }
};

float Character::mutation_value( const std::string &val ) const
{
    // Syntax similar to tuple get<n>()
    const auto found = mutation_value_map.find( val );

    if( found == mutation_value_map.end() ) {
        debugmsg( "Invalid mutation value name %s", val );
        return 0.0f;
    } else {
        return found->second( cached_mutations );
    }
}

float Character::healing_rate( float at_rest_quality ) const
{
    // TODO: Cache
    float heal_rate;
    if( !is_npc() ) {
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
    add_msg( m_debug, "%s healing: %.6f", name, final_rate );
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
        bandaged_rate += static_cast<float>( e_bandaged.get_amount( "HEAL_RATE" ) ) / to_turns<int>
                         ( 24_hours );
        if( bp == bp_head ) {
            bandaged_rate *= e_bandaged.get_amount( "HEAL_HEAD" ) / 100.0f;
        }
        if( bp == bp_torso ) {
            bandaged_rate *= e_bandaged.get_amount( "HEAL_TORSO" ) / 100.0f;
        }
    }

    if( !e_disinfected.is_null() ) {
        disinfected_rate += static_cast<float>( e_disinfected.get_amount( "HEAL_RATE" ) ) / to_turns<int>
                            ( 24_hours );
        if( bp == bp_head ) {
            disinfected_rate *= e_disinfected.get_amount( "HEAL_HEAD" ) / 100.0f;
        }
        if( bp == bp_torso ) {
            disinfected_rate *= e_disinfected.get_amount( "HEAL_TORSO" ) / 100.0f;
        }
    }

    rate_medicine += bandaged_rate + disinfected_rate;
    rate_medicine *= 1.0f + mutation_value( "healing_resting" );
    rate_medicine *= 1.0f + at_rest_quality;

    // increase healing if character has both effects
    if( !e_bandaged.is_null() && !e_disinfected.is_null() ) {
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

float Character::get_bmi() const
{
    return 12 * get_kcal_percent() + 13;
}

std::string Character::get_weight_string() const
{
    const float bmi = get_bmi();
    if( get_option<bool>( "CRAZY" ) ) {
        if( bmi > character_weight_category::morbidly_obese + 10.0f ) {
            return _( "AW HELL NAH" );
        } else if( bmi > character_weight_category::morbidly_obese + 5.0f ) {
            return _( "DAYUM" );
        } else if( bmi > character_weight_category::morbidly_obese ) {
            return _( "Fluffy" );
        } else if( bmi > character_weight_category::very_obese ) {
            return _( "Husky" );
        } else if( bmi > character_weight_category::obese ) {
            return _( "Healthy" );
        } else if( bmi > character_weight_category::overweight ) {
            return _( "Big" );
        } else if( bmi > character_weight_category::normal ) {
            return _( "Normal" );
        } else if( bmi > character_weight_category::underweight ) {
            return _( "Bean Pole" );
        } else if( bmi > character_weight_category::emaciated ) {
            return _( "Emaciated" );
        } else {
            return _( "Spooky Scary Skeleton" );
        }
    } else {
        if( bmi > character_weight_category::morbidly_obese ) {
            return _( "Morbidly Obese" );
        } else if( bmi > character_weight_category::very_obese ) {
            return _( "Very Obese" );
        } else if( bmi > character_weight_category::obese ) {
            return _( "Obese" );
        } else if( bmi > character_weight_category::overweight ) {
            return _( "Overweight" );
        } else if( bmi > character_weight_category::normal ) {
            return _( "Normal" );
        } else if( bmi > character_weight_category::underweight ) {
            return _( "Underweight" );
        } else if( bmi > character_weight_category::emaciated ) {
            return _( "Emaciated" );
        } else {
            return _( "Skeletal" );
        }
    }
}

std::string Character::get_weight_description() const
{
    const float bmi = get_bmi();
    if( bmi > character_weight_category::morbidly_obese ) {
        return _( "You have far more fat than is healthy or useful.  It is causing you major problems." );
    } else if( bmi > character_weight_category::very_obese ) {
        return _( "You have too much fat.  It impacts your day to day health and wellness." );
    } else if( bmi > character_weight_category::obese ) {
        return _( "You've definitely put on a lot of extra weight.  Although it's helpful in times of famine, this is too much and is impacting your health." );
    } else if( bmi > character_weight_category::overweight ) {
        return _( "You've put on some extra pounds.  Nothing too excessive but it's starting to impact your health and waistline a bit." );
    } else if( bmi > character_weight_category::normal ) {
        return _( "You look to be a pretty healthy weight, with some fat to last you through the winter but nothing excessive." );
    } else if( bmi > character_weight_category::underweight ) {
        return _( "You are thin, thinner than is healthy.  You are less resilient to going without food." );
    } else if( bmi > character_weight_category::emaciated ) {
        return _( "You are very unhealthily underweight, nearing starvation." );
    } else {
        return _( "You have very little meat left on your bones.  You appear to be starving." );
    }
}

units::mass Character::bodyweight() const
{
    return units::from_kilogram( get_bmi() * pow( height() / 100.0f, 2 ) );
}

units::mass Character::bionics_weight() const
{
    units::mass bio_weight = 0_gram;
    for( const auto bio : *my_bionics ) {
        if( !bio.info().included ) {
            bio_weight += item::find_type( bio.id.c_str() )->weight;
        }
    }
    return bio_weight;
}

int Character::height() const
{
    int height = init_height;
    int height_pos = 15;

    const static std::array<int, 5> v = {{ 290, 240, 190, 140, 90 }};
    for( const int up_bound : v ) {
        if( up_bound >= init_height && up_bound - init_height < 40 ) {
            height_pos = up_bound - init_height;
        }
    }

    if( get_size() == MS_TINY ) {
        height = 90 - height_pos;
    } else if( get_size() == MS_SMALL ) {
        height = 140 - height_pos;
    } else if( get_size() == MS_LARGE ) {
        height = 240 - height_pos;
    } else if( get_size() == MS_HUGE ) {
        height = 290 - height_pos;
    }

    // TODO: Make this a player creation option
    return height;
}

int Character::get_bmr() const
{
    /**
    Values are for males, and average!
    */
    const int age = 25;
    const int equation_constant = 5;
    return ceil( metabolic_rate_base() * activity_level * ( units::to_gram<int>
                 ( bodyweight() / 100.0 ) +
                 ( 6.25 * height() ) - ( 5 * age ) + equation_constant ) );
}

void Character::increase_activity_level( float new_level )
{
    if( activity_level < new_level ) {
        activity_level = new_level;
    }
}

void Character::decrease_activity_level( float new_level )
{
    if( activity_level > new_level ) {
        activity_level = new_level;
    }
}
void Character::reset_activity_level()
{
    activity_level = NO_EXERCISE;
}

std::string Character::activity_level_str() const
{
    if( activity_level <= NO_EXERCISE ) {
        return _( "NO_EXERCISE" );
    } else if( activity_level <= LIGHT_EXERCISE ) {
        return _( "LIGHT_EXERCISE" );
    } else if( activity_level <= MODERATE_EXERCISE ) {
        return _( "MODERATE_EXERCISE" );
    } else if( activity_level <= ACTIVE_EXERCISE ) {
        return _( "ACTIVE_EXERCISE" );
    } else {
        return _( "EXTRA_EXERCISE" );
    }
}

int Character::get_stim() const
{
    return stim;
}

void Character::set_stim( int new_stim )
{
    stim = new_stim;
}

void Character::mod_stim( int mod )
{
    stim += mod;
}

int Character::get_stamina() const
{
    return stamina;
}

int Character::get_stamina_max() const
{
    int maxStamina = get_option< int >( "PLAYER_MAX_STAMINA" );
    maxStamina *= Character::mutation_value( "max_stamina_modifier" );
    return maxStamina;
}

void Character::set_stamina( int new_stamina )
{
    stamina = new_stamina;
}

void Character::mod_stamina( int mod )
{
    stamina += mod;
}

void Character::burn_move_stamina( int moves )
{
    int overburden_percentage = 0;
    units::mass current_weight = weight_carried();
    units::mass max_weight = weight_capacity();
    if( current_weight > max_weight ) {
        overburden_percentage = ( current_weight - max_weight ) * 100 / max_weight;
    }

    int burn_ratio = get_option<int>( "PLAYER_BASE_STAMINA_BURN_RATE" );
    for( const bionic_id &bid : get_bionic_fueled_with( item( "muscle" ) ) ) {
        if( has_active_bionic( bid ) ) {
            burn_ratio = burn_ratio * 2 - 3;
        }
    }
    burn_ratio += overburden_percentage;
    if( move_mode == CMM_RUN ) {
        burn_ratio = burn_ratio * 7;
    }
    mod_stat( "stamina", -( ( moves * burn_ratio ) / 100.0 ) );
    add_msg( m_debug, "Stamina burn: %d", -( ( moves * burn_ratio ) / 100 ) );
    // Chance to suffer pain if overburden and stamina runs out or has trait BADBACK
    // Starts at 1 in 25, goes down by 5 for every 50% more carried
    if( ( current_weight > max_weight ) && ( has_trait( trait_BADBACK ) || get_stamina() == 0 ) &&
        one_in( 35 - 5 * current_weight / ( max_weight / 2 ) ) ) {
        add_msg_if_player( m_bad, _( "Your body strains under the weight!" ) );
        // 1 more pain for every 800 grams more (5 per extra STR needed)
        if( ( ( current_weight - max_weight ) / 800_gram > get_pain() && get_pain() < 100 ) ) {
            mod_pain( 1 );
        }
    }
}

void Character::update_stamina( int turns )
{
    const int current_stim = get_stim();
    float stamina_recovery = 0.0f;
    // Recover some stamina every turn.
    // Mutated stamina works even when winded
    float stamina_multiplier = ( !has_effect( effect_winded ) ? 1.0f : 0.1f ) +
                               mutation_value( "stamina_regen_modifier" );
    // But mouth encumbrance interferes, even with mutated stamina.
    stamina_recovery += stamina_multiplier * std::max( 1.0f,
                        get_option<float>( "PLAYER_BASE_STAMINA_REGEN_RATE" ) - ( encumb( bp_mouth ) / 5.0f ) );
    // TODO: recovering stamina causes hunger/thirst/fatigue.
    // TODO: Tiredness slowing recovery

    // stim recovers stamina (or impairs recovery)
    if( current_stim > 0 ) {
        // TODO: Make stamina recovery with stims cost health
        stamina_recovery += std::min( 5.0f, current_stim / 15.0f );
    } else if( current_stim < 0 ) {
        // Affect it less near 0 and more near full
        // Negative stim kill at -200
        // At -100 stim it inflicts -20 malus to regen at 100%  stamina,
        // effectivly countering stamina gain of default 20,
        // at 50% stamina its -10 (50%), cuts by 25% at 25% stamina
        stamina_recovery += current_stim / 5.0f * get_stamina() / get_stamina_max();
    }

    const int max_stam = get_stamina_max();
    if( get_power_level() >= 3_kJ && has_active_bionic( bio_gills ) ) {
        int bonus = std::min<int>( units::to_kilojoule( get_power_level() ) / 3,
                                   max_stam - get_stamina() - stamina_recovery * turns );
        // so the effective recovery is up to 5x default
        bonus = std::min( bonus, 4 * static_cast<int>
                          ( get_option<float>( "PLAYER_BASE_STAMINA_REGEN_RATE" ) ) );
        if( bonus > 0 ) {
            stamina_recovery += bonus;
            bonus /= 10;
            bonus = std::max( bonus, 1 );
            mod_power_level( units::from_kilojoule( -bonus ) );
        }
    }

    mod_stamina( roll_remainder( stamina_recovery * turns ) );
    add_msg( m_debug, "Stamina recovery: %d", roll_remainder( stamina_recovery * turns ) );
    // Cap at max
    set_stamina( std::min( std::max( get_stamina(), 0 ), max_stam ) );
}

int Character::item_handling_cost( const item &it, bool penalties, int base_cost ) const
{
    int mv = base_cost;
    if( penalties ) {
        // 40 moves per liter, up to 200 at 5 liters
        mv += std::min( 200, it.volume() / 20_ml );
    }

    if( weapon.typeId() == "e_handcuffs" ) {
        mv *= 4;
    } else if( penalties && has_effect( effect_grabbed ) ) {
        mv *= 2;
    }

    // For single handed items use the least encumbered hand
    if( it.is_two_handed( *this ) ) {
        mv += encumb( bp_hand_l ) + encumb( bp_hand_r );
    } else {
        mv += std::min( encumb( bp_hand_l ), encumb( bp_hand_r ) );
    }

    return std::min( std::max( mv, 0 ), MAX_HANDLING_COST );
}

int Character::item_store_cost( const item &it, const item & /* container */, bool penalties,
                                int base_cost ) const
{
    /** @EFFECT_PISTOL decreases time taken to store a pistol */
    /** @EFFECT_SMG decreases time taken to store an SMG */
    /** @EFFECT_RIFLE decreases time taken to store a rifle */
    /** @EFFECT_SHOTGUN decreases time taken to store a shotgun */
    /** @EFFECT_LAUNCHER decreases time taken to store a launcher */
    /** @EFFECT_STABBING decreases time taken to store a stabbing weapon */
    /** @EFFECT_CUTTING decreases time taken to store a cutting weapon */
    /** @EFFECT_BASHING decreases time taken to store a bashing weapon */
    int lvl = get_skill_level( it.is_gun() ? it.gun_skill() : it.melee_skill() );
    return item_handling_cost( it, penalties, base_cost ) / ( ( lvl + 10.0f ) / 10.0f );
}

void Character::wake_up()
{
    remove_effect( effect_sleep );
    remove_effect( effect_slept_through_alarm );
    remove_effect( effect_lying_down );
    // Do not remove effect_alarm_clock now otherwise it invalidates an effect iterator in player::process_effects().
    // We just set it for later removal (also happening in player::process_effects(), so no side effects) with a duration of 0 turns.
    if( has_effect( effect_alarm_clock ) ) {
        get_effect( effect_alarm_clock ).set_duration( 0_turns );
    }
    recalc_sight_limits();
}

int Character::get_shout_volume() const
{
    int base = 10;
    int shout_multiplier = 2;

    // Mutations make shouting louder, they also define the default message
    if( has_trait( trait_SHOUT3 ) ) {
        shout_multiplier = 4;
        base = 20;
    } else if( has_trait( trait_SHOUT2 ) ) {
        base = 15;
        shout_multiplier = 3;
    }

    // You can't shout without your face
    if( has_trait( trait_PROF_FOODP ) && !( is_wearing( itype_id( "foodperson_mask" ) ) ||
                                            is_wearing( itype_id( "foodperson_mask_on" ) ) ) ) {
        base = 0;
        shout_multiplier = 0;
    }

    // Masks and such dampen the sound
    // Balanced around whisper for wearing bondage mask
    // and noise ~= 10 (door smashing) for wearing dust mask for character with strength = 8
    /** @EFFECT_STR increases shouting volume */
    const int penalty = encumb( bp_mouth ) * 3 / 2;
    int noise = base + str_cur * shout_multiplier - penalty;

    // Minimum noise volume possible after all reductions.
    // Volume 1 can't be heard even by player
    constexpr int minimum_noise = 2;

    if( noise <= base ) {
        noise = std::max( minimum_noise, noise );
    }

    // Screaming underwater is not good for oxygen and harder to do overall
    if( underwater ) {
        noise = std::max( minimum_noise, noise / 2 );
    }
    return noise;
}

void Character::shout( std::string msg, bool order )
{
    int base = 10;
    std::string shout;

    // You can't shout without your face
    if( has_trait( trait_PROF_FOODP ) && !( is_wearing( itype_id( "foodperson_mask" ) ) ||
                                            is_wearing( itype_id( "foodperson_mask_on" ) ) ) ) {
        add_msg_if_player( m_warning, _( "You try to shout but you have no face!" ) );
        return;
    }

    // Mutations make shouting louder, they also define the default message
    if( has_trait( trait_SHOUT3 ) ) {
        base = 20;
        if( msg.empty() ) {
            msg = is_player() ? _( "yourself let out a piercing howl!" ) : _( "a piercing howl!" );
            shout = "howl";
        }
    } else if( has_trait( trait_SHOUT2 ) ) {
        base = 15;
        if( msg.empty() ) {
            msg = is_player() ? _( "yourself scream loudly!" ) : _( "a loud scream!" );
            shout = "scream";
        }
    }

    if( msg.empty() ) {
        msg = is_player() ? _( "yourself shout loudly!" ) : _( "a loud shout!" );
        shout = "default";
    }
    int noise = get_shout_volume();

    // Minimum noise volume possible after all reductions.
    // Volume 1 can't be heard even by player
    constexpr int minimum_noise = 2;

    if( noise <= base ) {
        std::string dampened_shout;
        std::transform( msg.begin(), msg.end(), std::back_inserter( dampened_shout ), tolower );
        msg = std::move( dampened_shout );
    }

    // Screaming underwater is not good for oxygen and harder to do overall
    if( underwater ) {
        if( !has_trait( trait_GILLS ) && !has_trait( trait_GILLS_CEPH ) ) {
            mod_stat( "oxygen", -noise );
        }
    }

    const int penalty = encumb( bp_mouth ) * 3 / 2;
    // TODO: indistinct noise descriptions should be handled in the sounds code
    if( noise <= minimum_noise ) {
        add_msg_if_player( m_warning,
                           _( "The sound of your voice is almost completely muffled!" ) );
        msg = is_player() ? _( "your muffled shout" ) : _( "an indistinct voice" );
    } else if( noise * 2 <= noise + penalty ) {
        // The shout's volume is 1/2 or lower of what it would be without the penalty
        add_msg_if_player( m_warning, _( "The sound of your voice is significantly muffled!" ) );
    }

    sounds::sound( pos(), noise, order ? sounds::sound_t::order : sounds::sound_t::alert, msg, false,
                   "shout", shout );
}

void Character::vomit()
{
    g->events().send<event_type::throws_up>( getID() );

    if( stomach.contains() != 0_ml ) {
        // empty stomach contents
        stomach.bowel_movement();
        g->m.add_field( adjacent_tile(), fd_bile, 1 );
        add_msg_player_or_npc( m_bad, _( "You throw up heavily!" ), _( "<npcname> throws up heavily!" ) );
    }

    if( !has_effect( effect_nausea ) ) {  // Prevents never-ending nausea
        const effect dummy_nausea( &effect_nausea.obj(), 0_turns, num_bp, false, 1, calendar::turn );
        add_effect( effect_nausea, std::max( dummy_nausea.get_max_duration() * units::to_milliliter(
                stomach.contains() ) / 21, dummy_nausea.get_int_dur_factor() ) );
    }

    moves -= 100;
    for( auto &elem : *effects ) {
        for( auto &_effect_it : elem.second ) {
            auto &it = _effect_it.second;
            if( it.get_id() == effect_foodpoison ) {
                it.mod_duration( -30_minutes );
            } else if( it.get_id() == effect_drunk ) {
                it.mod_duration( rng( -10_minutes, -50_minutes ) );
            }
        }
    }
    remove_effect( effect_pkill1 );
    remove_effect( effect_pkill2 );
    remove_effect( effect_pkill3 );
    // Don't wake up when just retching
    if( stomach.contains() > 0_ml ) {
        wake_up();
    }
}

// adjacent_tile() returns a safe, unoccupied adjacent tile. If there are no such tiles, returns player position instead.
tripoint Character::adjacent_tile() const
{
    std::vector<tripoint> ret;
    int dangerous_fields = 0;
    for( const tripoint &p : g->m.points_in_radius( pos(), 1 ) ) {
        if( p == pos() ) {
            // Don't consider player position
            continue;
        }
        const trap &curtrap = g->m.tr_at( p );
        if( g->critter_at( p ) == nullptr && g->m.passable( p ) &&
            ( curtrap.is_null() || curtrap.is_benign() ) ) {
            // Only consider tile if unoccupied, passable and has no traps
            dangerous_fields = 0;
            auto &tmpfld = g->m.field_at( p );
            for( auto &fld : tmpfld ) {
                const field_entry &cur = fld.second;
                if( cur.is_dangerous() ) {
                    dangerous_fields++;
                }
            }

            if( dangerous_fields == 0 ) {
                ret.push_back( p );
            }
        }
    }

    return random_entry( ret, pos() ); // player position if no valid adjacent tiles
}

void Character::healed_bp( int bp, int amount )
{
    healed_total[bp] += amount;
}

void Character::set_fac_id( const std::string &my_fac_id )
{
    fac_id = faction_id( my_fac_id );
}

std::string get_stat_name( Character::stat Stat )
{
    switch( Stat ) {
        // *INDENT-OFF*
    case Character::stat::STRENGTH:     return pgettext( "strength stat", "STR" );
    case Character::stat::DEXTERITY:    return pgettext( "dexterity stat", "DEX" );
    case Character::stat::INTELLIGENCE: return pgettext( "intelligence stat", "INT" );
    case Character::stat::PERCEPTION:   return pgettext( "perception stat", "PER" );
        // *INDENT-ON*
        default:
            return pgettext( "fake stat there's an error", "ERR" );
            break;

    }
    return pgettext( "fake stat there's an error", "ERR" );
}

void Character::build_mut_dependency_map( const trait_id &mut,
        std::unordered_map<trait_id, int> &dependency_map, int distance )
{
    // Skip base traits and traits we've seen with a lower distance
    const auto lowest_distance = dependency_map.find( mut );
    if( !has_base_trait( mut ) && ( lowest_distance == dependency_map.end() ||
                                    distance < lowest_distance->second ) ) {
        dependency_map[mut] = distance;
        // Recurse over all prerequisite and replacement mutations
        const mutation_branch &mdata = mut.obj();
        for( const trait_id &i : mdata.prereqs ) {
            build_mut_dependency_map( i, dependency_map, distance + 1 );
        }
        for( const trait_id &i : mdata.prereqs2 ) {
            build_mut_dependency_map( i, dependency_map, distance + 1 );
        }
        for( const trait_id &i : mdata.replacements ) {
            build_mut_dependency_map( i, dependency_map, distance + 1 );
        }
    }
}

void Character::set_highest_cat_level()
{
    mutation_category_level.clear();

    // For each of our mutations...
    for( const std::pair<trait_id, Character::trait_data> &mut : my_mutations ) {
        // ...build up a map of all prerequisite/replacement mutations along the tree, along with their distance from the current mutation
        std::unordered_map<trait_id, int> dependency_map;
        build_mut_dependency_map( mut.first, dependency_map, 0 );

        // Then use the map to set the category levels
        for( const std::pair<trait_id, int> &i : dependency_map ) {
            const mutation_branch &mdata = i.first.obj();
            if( !mdata.flags.count( "NON_THRESH" ) ) {
                for( const std::string &cat : mdata.category ) {
                    // Decay category strength based on how far it is from the current mutation
                    mutation_category_level[cat] += 8 / static_cast<int>( std::pow( 2, i.second ) );
                }
            }
        }
    }
}

void Character::drench_mut_calc()
{
    for( const body_part bp : all_body_parts ) {
        int ignored = 0;
        int neutral = 0;
        int good = 0;

        for( const auto &iter : my_mutations ) {
            const mutation_branch &mdata = iter.first.obj();
            const auto wp_iter = mdata.protection.find( bp );
            if( wp_iter != mdata.protection.end() ) {
                ignored += wp_iter->second.x;
                neutral += wp_iter->second.y;
                good += wp_iter->second.z;
            }
        }

        mut_drench[bp][WT_GOOD] = good;
        mut_drench[bp][WT_NEUTRAL] = neutral;
        mut_drench[bp][WT_IGNORED] = ignored;
    }
}

/// Returns the mutation category with the highest strength
std::string Character::get_highest_category() const
{
    int iLevel = 0;
    std::string sMaxCat;

    for( const std::pair<std::string, int> &elem : mutation_category_level ) {
        if( elem.second > iLevel ) {
            sMaxCat = elem.first;
            iLevel = elem.second;
        } else if( elem.second == iLevel ) {
            sMaxCat.clear();  // no category on ties
        }
    }
    return sMaxCat;
}

void Character::recalculate_enchantment_cache()
{
    // start by resetting the cache to all inventory items
    enchantment_cache = inv.get_active_enchantment_cache( *this );

    for( const enchantment &ench : weapon.get_enchantments() ) {
        if( ench.is_active( *this, weapon ) ) {
            enchantment_cache.force_add( ench );
        }
    }

    for( const item &worn_it : worn ) {
        for( const enchantment &ench : worn_it.get_enchantments() ) {
            if( ench.is_active( *this, worn_it ) ) {
                enchantment_cache.force_add( ench );
            }
        }
    }
}

double Character::calculate_by_enchantment( double modify, enchantment::mod value,
        bool round_output ) const
{
    modify += enchantment_cache.get_value_add( value );
    modify *= 1.0 + enchantment_cache.get_value_multiply( value );
    if( round_output ) {
        modify = round( modify );
    }
    return modify;
}

void Character::passive_absorb_hit( body_part bp, damage_unit &du ) const
{
    // >0 check because some mutations provide negative armor
    // Thin skin check goes before subdermal armor plates because SUBdermal
    if( du.amount > 0.0f ) {
        // Horrible hack warning!
        // Get rid of this as soon as CUT and STAB are split
        if( du.type == DT_STAB ) {
            damage_unit du_copy = du;
            du_copy.type = DT_CUT;
            du.amount -= mutation_armor( bp, du_copy );
        } else {
            du.amount -= mutation_armor( bp, du );
        }
    }
    du.amount -= bionic_armor_bonus( bp, du.type ); //Check for passive armor bionics
    du.amount -= mabuff_armor_bonus( du.type );
    du.amount = std::max( 0.0f, du.amount );
}

static void destroyed_armor_msg( Character &who, const std::string &pre_damage_name )
{
    if( who.is_avatar() ) {
        g->memorial().add(
            //~ %s is armor name
            pgettext( "memorial_male", "Worn %s was completely destroyed." ),
            pgettext( "memorial_female", "Worn %s was completely destroyed." ),
            pre_damage_name );
    }
    who.add_msg_player_or_npc( m_bad, _( "Your %s is completely destroyed!" ),
                               _( "<npcname>'s %s is completely destroyed!" ),
                               pre_damage_name );
}

static void item_armor_enchantment_adjust( Character &guy, damage_unit &du, item &armor )
{
    switch( du.type ) {
        case DT_ACID:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchantment::mod::ITEM_ARMOR_ACID );
            break;
        case DT_BASH:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchantment::mod::ITEM_ARMOR_BASH );
            break;
        case DT_BIOLOGICAL:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchantment::mod::ITEM_ARMOR_BIO );
            break;
        case DT_COLD:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchantment::mod::ITEM_ARMOR_COLD );
            break;
        case DT_CUT:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchantment::mod::ITEM_ARMOR_CUT );
            break;
        case DT_ELECTRIC:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchantment::mod::ITEM_ARMOR_ELEC );
            break;
        case DT_HEAT:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchantment::mod::ITEM_ARMOR_HEAT );
            break;
        case DT_STAB:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchantment::mod::ITEM_ARMOR_STAB );
            break;
        default:
            return;
    }
    du.amount = std::max( 0.0f, du.amount );
}

// adjusts damage unit depending on type by enchantments.
// the ITEM_ enchantments only affect the damage resistance for that one item, while the others affect all of them
static void armor_enchantment_adjust( Character &guy, damage_unit &du )
{
    switch( du.type ) {
        case DT_ACID:
            du.amount = guy.calculate_by_enchantment( du.amount, enchantment::mod::ARMOR_ACID );
            break;
        case DT_BASH:
            du.amount = guy.calculate_by_enchantment( du.amount, enchantment::mod::ARMOR_BASH );
            break;
        case DT_BIOLOGICAL:
            du.amount = guy.calculate_by_enchantment( du.amount, enchantment::mod::ARMOR_BIO );
            break;
        case DT_COLD:
            du.amount = guy.calculate_by_enchantment( du.amount, enchantment::mod::ARMOR_COLD );
            break;
        case DT_CUT:
            du.amount = guy.calculate_by_enchantment( du.amount, enchantment::mod::ARMOR_CUT );
            break;
        case DT_ELECTRIC:
            du.amount = guy.calculate_by_enchantment( du.amount, enchantment::mod::ARMOR_ELEC );
            break;
        case DT_HEAT:
            du.amount = guy.calculate_by_enchantment( du.amount, enchantment::mod::ARMOR_HEAT );
            break;
        case DT_STAB:
            du.amount = guy.calculate_by_enchantment( du.amount, enchantment::mod::ARMOR_STAB );
            break;
        default:
            return;
    }
    du.amount = std::max( 0.0f, du.amount );
}

void Character::absorb_hit( body_part bp, damage_instance &dam )
{
    std::list<item> worn_remains;
    bool armor_destroyed = false;

    for( damage_unit &elem : dam.damage_units ) {
        if( elem.amount < 0 ) {
            // Prevents 0 damage hits (like from hallucinations) from ripping armor
            elem.amount = 0;
            continue;
        }

        // The bio_ads CBM absorbs damage before hitting armor
        if( has_active_bionic( bio_ads ) ) {
            if( elem.amount > 0 && get_power_level() > 24_kJ ) {
                if( elem.type == DT_BASH ) {
                    elem.amount -= rng( 1, 8 );
                } else if( elem.type == DT_CUT ) {
                    elem.amount -= rng( 1, 4 );
                } else if( elem.type == DT_STAB ) {
                    elem.amount -= rng( 1, 2 );
                }
                mod_power_level( -25_kJ );
            }
            if( elem.amount < 0 ) {
                elem.amount = 0;
            }
        }

        armor_enchantment_adjust( *this, elem );

        // Only the outermost armor can be set on fire
        bool outermost = true;
        // The worn vector has the innermost item first, so
        // iterate reverse to damage the outermost (last in worn vector) first.
        for( auto iter = worn.rbegin(); iter != worn.rend(); ) {
            item &armor = *iter;

            if( !armor.covers( bp ) ) {
                ++iter;
                continue;
            }

            const std::string pre_damage_name = armor.tname();
            bool destroy = false;

            item_armor_enchantment_adjust( *this, elem, armor );
            // Heat damage can set armor on fire
            // Even though it doesn't cause direct physical damage to it
            if( outermost && elem.type == DT_HEAT && elem.amount >= 1.0f ) {
                // TODO: Different fire intensity values based on damage
                fire_data frd{ 2 };
                destroy = armor.burn( frd );
                int fuel = roll_remainder( frd.fuel_produced );
                if( fuel > 0 ) {
                    add_effect( effect_onfire, time_duration::from_turns( fuel + 1 ), bp, false, 0, false, true );
                }
            }

            if( !destroy ) {
                destroy = armor_absorb( elem, armor );
            }

            if( destroy ) {
                if( g->u.sees( *this ) ) {
                    SCT.add( point( posx(), posy() ), NORTH, remove_color_tags( pre_damage_name ),
                             m_neutral, _( "destroyed" ), m_info );
                }
                destroyed_armor_msg( *this, pre_damage_name );
                armor_destroyed = true;
                armor.on_takeoff( *this );
                worn_remains.insert( worn_remains.end(), armor.contents.begin(), armor.contents.end() );
                // decltype is the type name of the iterator, note that reverse_iterator::base returns the
                // iterator to the next element, not the one the revers_iterator points to.
                // http://stackoverflow.com/questions/1830158/how-to-call-erase-with-a-reverse-iterator
                iter = decltype( iter )( worn.erase( --( iter.base() ) ) );
            } else {
                ++iter;
                outermost = false;
            }
        }

        passive_absorb_hit( bp, elem );

        if( elem.type == DT_BASH ) {
            if( has_trait( trait_LIGHT_BONES ) ) {
                elem.amount *= 1.4;
            }
            if( has_trait( trait_HOLLOW_BONES ) ) {
                elem.amount *= 1.8;
            }
        }

        elem.amount = std::max( elem.amount, 0.0f );
    }
    for( item &remain : worn_remains ) {
        g->m.add_item_or_charges( pos(), remain );
    }
    if( armor_destroyed ) {
        drop_invalid_inventory();
    }
}

bool Character::armor_absorb( damage_unit &du, item &armor )
{
    if( rng( 1, 100 ) > armor.get_coverage() ) {
        return false;
    }

    // TODO: add some check for power armor
    armor.mitigate_damage( du );

    // We want armor's own resistance to this type, not the resistance it grants
    const int armors_own_resist = armor.damage_resist( du.type, true );
    if( armors_own_resist > 1000 ) {
        // This is some weird type that doesn't damage armors
        return false;
    }

    // Scale chance of article taking damage based on the number of parts it covers.
    // This represents large articles being able to take more punishment
    // before becoming ineffective or being destroyed.
    const int num_parts_covered = armor.get_covered_body_parts().count();
    if( !one_in( num_parts_covered ) ) {
        return false;
    }

    // Don't damage armor as much when bypassed by armor piercing
    // Most armor piercing damage comes from bypassing armor, not forcing through
    const int raw_dmg = du.amount;
    if( raw_dmg > armors_own_resist ) {
        // If damage is above armor value, the chance to avoid armor damage is
        // 50% + 50% * 1/dmg
        if( one_in( raw_dmg ) || one_in( 2 ) ) {
            return false;
        }
    } else {
        // Sturdy items and power armors never take chip damage.
        // Other armors have 0.5% of getting damaged from hits below their armor value.
        if( armor.has_flag( "STURDY" ) || armor.is_power_armor() || !one_in( 200 ) ) {
            return false;
        }
    }

    const material_type &material = armor.get_random_material();
    std::string damage_verb = ( du.type == DT_BASH ) ? material.bash_dmg_verb() :
                              material.cut_dmg_verb();

    const std::string pre_damage_name = armor.tname();
    const std::string pre_damage_adj = armor.get_base_material().dmg_adj( armor.damage_level( 4 ) );

    // add "further" if the damage adjective and verb are the same
    std::string format_string = ( pre_damage_adj == damage_verb ) ?
                                _( "Your %1$s is %2$s further!" ) : _( "Your %1$s is %2$s!" );
    add_msg_if_player( m_bad, format_string, pre_damage_name, damage_verb );
    //item is damaged
    if( is_player() ) {
        SCT.add( point( posx(), posy() ), NORTH, remove_color_tags( pre_damage_name ), m_neutral,
                 damage_verb,
                 m_info );
    }

    return armor.mod_damage( armor.has_flag( "FRAGILE" ) ?
                             rng( 2 * itype::damage_scale, 3 * itype::damage_scale ) : itype::damage_scale, du.type );
}

float Character::bionic_armor_bonus( body_part bp, damage_type dt ) const
{
    float result = 0.0f;
    // We only check the passive bionics
    if( has_bionic( bio_carbon ) ) {
        if( dt == DT_BASH ) {
            result += 2;
        } else if( dt == DT_CUT || dt == DT_STAB ) {
            result += 4;
        }
    }
    // All the other bionic armors reduce bash/cut/stab by 3
    // Map body parts to a set of bionics that protect it
    // TODO: JSONize passive bionic armor instead of hardcoding it
    static const std::map< body_part, bionic_id > armor_bionics = {
        { bp_head, { bio_armor_head } },
        { bp_arm_l, { bio_armor_arms } },
        { bp_arm_r, { bio_armor_arms } },
        { bp_torso, { bio_armor_torso } },
        { bp_leg_l, { bio_armor_legs } },
        { bp_leg_r, { bio_armor_legs } },
        { bp_eyes, { bio_armor_eyes } }
    };
    auto iter = armor_bionics.find( bp );
    if( iter != armor_bionics.end() && has_bionic( iter->second ) &&
        ( dt == DT_BASH || dt == DT_CUT || dt == DT_STAB ) ) {
        result += 3;
    }
    return result;
}

void Character::did_hit( Creature &target )
{
    enchantment_cache.cast_hit_you( *this, target.pos() );
}

void Character::on_hit( Creature * /*source*/, body_part /*bp_hit*/,
                        float /*difficulty*/, dealt_projectile_attack const *const /*proj*/ )
{
    enchantment_cache.cast_hit_me( *this );
}

void Character::heal( body_part healed, int dam )
{
    hp_part healpart;
    switch( healed ) {
        case bp_eyes: // Fall through to head damage
        case bp_mouth: // Fall through to head damage
        case bp_head:
            healpart = hp_head;
            break;
        case bp_torso:
            healpart = hp_torso;
            break;
        case bp_hand_l:
            // Shouldn't happen, but fall through to arms
            debugmsg( "Heal against hands!" );
        /* fallthrough */
        case bp_arm_l:
            healpart = hp_arm_l;
            break;
        case bp_hand_r:
            // Shouldn't happen, but fall through to arms
            debugmsg( "Heal against hands!" );
        /* fallthrough */
        case bp_arm_r:
            healpart = hp_arm_r;
            break;
        case bp_foot_l:
            // Shouldn't happen, but fall through to legs
            debugmsg( "Heal against feet!" );
        /* fallthrough */
        case bp_leg_l:
            healpart = hp_leg_l;
            break;
        case bp_foot_r:
            // Shouldn't happen, but fall through to legs
            debugmsg( "Heal against feet!" );
        /* fallthrough */
        case bp_leg_r:
            healpart = hp_leg_r;
            break;
        default:
            debugmsg( "Wacky body part healed!" );
            healpart = hp_torso;
    }
    heal( healpart, dam );
}

void Character::heal( hp_part healed, int dam )
{
    if( hp_cur[healed] > 0 ) {
        int effective_heal = std::min( dam, hp_max[healed] - hp_cur[healed] );
        hp_cur[healed] += effective_heal;
        g->events().send<event_type::character_heals_damage>( getID(), effective_heal );
    }
}

void Character::healall( int dam )
{
    for( int healed_part = 0; healed_part < num_hp_parts; healed_part++ ) {
        heal( static_cast<hp_part>( healed_part ), dam );
        healed_bp( healed_part, dam );
    }
}

void Character::hurtall( int dam, Creature *source, bool disturb /*= true*/ )
{
    if( is_dead_state() || has_trait( trait_DEBUG_NODMG ) || dam <= 0 ) {
        return;
    }

    for( int i = 0; i < num_hp_parts; i++ ) {
        const hp_part bp = static_cast<hp_part>( i );
        // Don't use apply_damage here or it will annoy the player with 6 queries
        const int dam_to_bodypart = std::min( dam, hp_cur[bp] );
        hp_cur[bp] -= dam_to_bodypart;
        g->events().send<event_type::character_takes_damage>( getID(), dam_to_bodypart );
    }

    // Low pain: damage is spread all over the body, so not as painful as 6 hits in one part
    mod_pain( dam );
    on_hurt( source, disturb );
}

int Character::hitall( int dam, int vary, Creature *source )
{
    int damage_taken = 0;
    for( int i = 0; i < num_hp_parts; i++ ) {
        const body_part bp = hp_to_bp( static_cast<hp_part>( i ) );
        int ddam = vary ? dam * rng( 100 - vary, 100 ) / 100 : dam;
        int cut = 0;
        auto damage = damage_instance::physical( ddam, cut, 0 );
        damage_taken += deal_damage( source, bp, damage ).total_damage();
    }
    return damage_taken;
}

void Character::on_hurt( Creature *source, bool disturb /*= true*/ )
{
    if( has_trait( trait_ADRENALINE ) && !has_effect( effect_adrenaline ) &&
        ( hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15 ) ) {
        add_effect( effect_adrenaline, 20_minutes );
    }

    if( disturb ) {
        if( has_effect( effect_sleep ) && !has_effect( effect_narcosis ) ) {
            wake_up();
        }
        if( !is_npc() && !has_effect( effect_narcosis ) ) {
            if( source != nullptr ) {
                g->cancel_activity_or_ignore_query( distraction_type::attacked,
                                                    string_format( _( "You were attacked by %s!" ),
                                                            source->disp_name() ) );
            } else {
                g->cancel_activity_or_ignore_query( distraction_type::attacked, _( "You were hurt!" ) );
            }
        }
    }

    if( is_dead_state() ) {
        set_killer( source );
    }
}

bool Character::crossed_threshold() const
{
    for( const std::pair<trait_id, Character::trait_data> &mut : my_mutations ) {
        if( mut.first->threshold ) {
            return true;
        }
    }
    return false;
}

void Character::spores()
{
    fungal_effects fe( *g, g->m );
    //~spore-release sound
    sounds::sound( pos(), 10, sounds::sound_t::combat, _( "Pouf!" ), false, "misc", "puff" );
    for( const tripoint &sporep : g->m.points_in_radius( pos(), 1 ) ) {
        if( sporep == pos() ) {
            continue;
        }
        fe.fungalize( sporep, this, 0.25 );
    }
}

void Character::blossoms()
{
    // Player blossoms are shorter-ranged, but you can fire much more frequently if you like.
    sounds::sound( pos(), 10, sounds::sound_t::combat, _( "Pouf!" ), false, "misc", "puff" );
    for( const tripoint &tmp : g->m.points_in_radius( pos(), 2 ) ) {
        g->m.add_field( tmp, fd_fungal_haze, rng( 1, 2 ) );
    }
}

void Character::update_vitamins( const vitamin_id &vit )
{
    if( is_npc() ) {
        return; // NPCs cannot develop vitamin diseases
    }

    efftype_id def = vit.obj().deficiency();
    efftype_id exc = vit.obj().excess();

    int lvl = vit.obj().severity( vitamin_get( vit ) );
    if( lvl <= 0 ) {
        remove_effect( def );
    }
    if( lvl >= 0 ) {
        remove_effect( exc );
    }
    if( lvl > 0 ) {
        if( has_effect( def, num_bp ) ) {
            get_effect( def, num_bp ).set_intensity( lvl, true );
        } else {
            add_effect( def, 1_turns, num_bp, true, lvl );
        }
    }
    if( lvl < 0 ) {
        if( has_effect( exc, num_bp ) ) {
            get_effect( exc, num_bp ).set_intensity( -lvl, true );
        } else {
            add_effect( exc, 1_turns, num_bp, true, -lvl );
        }
    }
}

void Character::rooted_message() const
{
    bool wearing_shoes = is_wearing_shoes( side::LEFT ) || is_wearing_shoes( side::RIGHT );
    if( ( has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 ) ) &&
        g->m.has_flag( "PLOWABLE", pos() ) &&
        !wearing_shoes ) {
        add_msg( m_info, _( "You sink your roots into the soil." ) );
    }
}

void Character::rooted()
// Should average a point every two minutes or so; ground isn't uniformly fertile
{
    double shoe_factor = footwear_factor();
    if( ( has_trait( trait_ROOTS2 ) || has_trait( trait_ROOTS3 ) ) &&
        g->m.has_flag( "PLOWABLE", pos() ) && shoe_factor != 1.0 ) {
        if( one_in( 96 ) ) {
            vitamin_mod( vitamin_id( "iron" ), 1, true );
            vitamin_mod( vitamin_id( "calcium" ), 1, true );
        }
        if( get_thirst() <= -2000 && x_in_y( 75, 425 ) ) {
            mod_thirst( -1 );
        }
        mod_healthy_mod( 5, 50 );
    }
}

bool Character::wearing_something_on( body_part bp ) const
{
    for( auto &i : worn ) {
        if( i.covers( bp ) ) {
            return true;
        }
    }
    return false;
}

bool Character::is_wearing_shoes( const side &which_side ) const
{
    bool left = true;
    bool right = true;
    if( which_side == side::LEFT || which_side == side::BOTH ) {
        left = false;
        for( const item &worn_item : worn ) {
            if( worn_item.covers( bp_foot_l ) && !worn_item.has_flag( "BELTED" ) &&
                !worn_item.has_flag( "PERSONAL" ) && !worn_item.has_flag( "AURA" ) &&
                !worn_item.has_flag( "SEMITANGIBLE" ) && !worn_item.has_flag( "SKINTIGHT" ) ) {
                left = true;
                break;
            }
        }
    }
    if( which_side == side::RIGHT || which_side == side::BOTH ) {
        right = false;
        for( const item &worn_item : worn ) {
            if( worn_item.covers( bp_foot_r ) && !worn_item.has_flag( "BELTED" ) &&
                !worn_item.has_flag( "PERSONAL" ) && !worn_item.has_flag( "AURA" ) &&
                !worn_item.has_flag( "SEMITANGIBLE" ) && !worn_item.has_flag( "SKINTIGHT" ) ) {
                right = true;
                break;
            }
        }
    }
    return ( left && right );
}

double Character::footwear_factor() const
{
    double ret = 0;
    if( wearing_something_on( bp_foot_l ) ) {
        ret += .5;
    }
    if( wearing_something_on( bp_foot_r ) ) {
        ret += .5;
    }
    return ret;
}

void Character::update_morale()
{
    morale->decay( 1_minutes );
    apply_persistent_morale();
}

void Character::apply_persistent_morale()
{
    // Hoarders get a morale penalty if they're not carrying a full inventory.
    if( has_trait( trait_HOARDER ) ) {
        int pen = ( volume_capacity() - volume_carried() ) / 125_ml;
        if( pen > 70 ) {
            pen = 70;
        }
        if( pen <= 0 ) {
            pen = 0;
        }
        if( has_effect( effect_took_xanax ) ) {
            pen = pen / 7;
        } else if( has_effect( effect_took_prozac ) ) {
            pen = pen / 2;
        }
        if( pen > 0 ) {
            add_morale( MORALE_PERM_HOARDER, -pen, -pen, 1_minutes, 1_minutes, true );
        }
    }
    // Nomads get a morale penalty if they stay near the same overmap tiles too long.
    if( has_trait( trait_NOMAD ) || has_trait( trait_NOMAD2 ) || has_trait( trait_NOMAD3 ) ) {
        const tripoint ompos = global_omt_location();
        float total_time = 0;
        // Check how long we've stayed in any overmap tile within 5 of us.
        const int max_dist = 5;
        for( const tripoint &pos : points_in_radius( ompos, max_dist ) ) {
            const float dist = rl_dist( ompos, pos );
            if( dist > max_dist ) {
                continue;
            }
            const auto iter = overmap_time.find( pos.xy() );
            if( iter == overmap_time.end() ) {
                continue;
            }
            // Count time in own tile fully, tiles one away as 4/5, tiles two away as 3/5, etc.
            total_time += to_moves<float>( iter->second ) * ( max_dist - dist ) / max_dist;
        }
        // Characters with higher tiers of Nomad suffer worse morale penalties, faster.
        int max_unhappiness;
        float min_time, max_time;
        if( has_trait( trait_NOMAD ) ) {
            max_unhappiness = 20;
            min_time = to_moves<float>( 12_hours );
            max_time = to_moves<float>( 1_days );
        } else if( has_trait( trait_NOMAD2 ) ) {
            max_unhappiness = 40;
            min_time = to_moves<float>( 4_hours );
            max_time = to_moves<float>( 8_hours );
        } else { // traid_NOMAD3
            max_unhappiness = 60;
            min_time = to_moves<float>( 1_hours );
            max_time = to_moves<float>( 2_hours );
        }
        // The penalty starts at 1 at min_time and scales up to max_unhappiness at max_time.
        const float t = ( total_time - min_time ) / ( max_time - min_time );
        const int pen = ceil( lerp_clamped( 0, max_unhappiness, t ) );
        if( pen > 0 ) {
            add_morale( MORALE_PERM_NOMAD, -pen, -pen, 1_minutes, 1_minutes, true );
        }
    }

    if( has_trait( trait_PROF_FOODP ) ) {
        // Loosing your face is distressing
        if( !( is_wearing( itype_id( "foodperson_mask" ) ) ||
               is_wearing( itype_id( "foodperson_mask_on" ) ) ) ) {
            add_morale( MORALE_PERM_NOFACE, -20, -20, 1_minutes, 1_minutes, true );
        } else if( is_wearing( itype_id( "foodperson_mask" ) ) ||
                   is_wearing( itype_id( "foodperson_mask_on" ) ) ) {
            rem_morale( MORALE_PERM_NOFACE );
        }

        if( is_wearing( itype_id( "foodperson_mask_on" ) ) ) {
            add_morale( MORALE_PERM_FPMODE_ON, 10, 10, 1_minutes, 1_minutes, true );
        } else {
            rem_morale( MORALE_PERM_FPMODE_ON );
        }
    }
}

int Character::get_morale_level() const
{
    return morale->get_level();
}

void Character::add_morale( const morale_type &type, int bonus, int max_bonus,
                            const time_duration &duration, const time_duration &decay_start,
                            bool capped, const itype *item_type )
{
    morale->add( type, bonus, max_bonus, duration, decay_start, capped, item_type );
}

int Character::has_morale( const morale_type &type ) const
{
    return morale->has( type );
}

void Character::rem_morale( const morale_type &type, const itype *item_type )
{
    morale->remove( type, item_type );
}

void Character::clear_morale()
{
    morale->clear();
}

bool Character::has_morale_to_read() const
{
    return get_morale_level() >= -40;
}

void Character::check_and_recover_morale()
{
    player_morale test_morale;

    for( const auto &wit : worn ) {
        test_morale.on_item_wear( wit );
    }

    for( const auto &mut : my_mutations ) {
        test_morale.on_mutation_gain( mut.first );
    }

    for( auto &elem : *effects ) {
        for( auto &_effect_it : elem.second ) {
            const effect &e = _effect_it.second;
            test_morale.on_effect_int_change( e.get_id(), e.get_intensity(), e.get_bp() );
        }
    }

    test_morale.on_stat_change( "hunger", get_hunger() );
    test_morale.on_stat_change( "thirst", get_thirst() );
    test_morale.on_stat_change( "fatigue", get_fatigue() );
    test_morale.on_stat_change( "pain", get_pain() );
    test_morale.on_stat_change( "pkill", get_painkiller() );
    test_morale.on_stat_change( "perceived_pain", get_perceived_pain() );

    apply_persistent_morale();

    if( !morale->consistent_with( test_morale ) ) {
        *morale = player_morale( test_morale ); // Recover consistency
        add_msg( m_debug, "%s morale was recovered.", disp_name( true ) );
    }
}


void Character::start_hauling()
{
    add_msg( _( "You start hauling items along the ground." ) );
    if( is_armed() ) {
        add_msg( m_warning, _( "Your hands are not free, which makes hauling slower." ) );
    }
    hauling = true;
}

void Character::stop_hauling()
{
    add_msg( _( "You stop hauling items." ) );
    hauling = false;
    if( has_activity( activity_id( "ACT_MOVE_ITEMS" ) ) ) {
        cancel_activity();
    }
}

bool Character::is_hauling() const
{
    return hauling;
}

void Character::assign_activity( const activity_id &type, int moves, int index, int pos,
                                 const std::string &name )
{
    assign_activity( player_activity( type, moves, index, pos, name ) );
}

void Character::assign_activity( const player_activity &act, bool allow_resume )
{
    if( allow_resume && !backlog.empty() && backlog.front().can_resume_with( act, *this ) ) {
        add_msg_if_player( _( "You resume your task." ) );
        activity = backlog.front();
        backlog.pop_front();
    } else {
        if( activity ) {
            backlog.push_front( activity );
        }

        activity = act;
    }

    if( activity.rooted() ) {
        rooted_message();
    }
    if( is_npc() ) {
        npc *guy = dynamic_cast<npc *>( this );
        guy->set_attitude( NPCATT_ACTIVITY );
        guy->set_mission( NPC_MISSION_ACTIVITY );
        guy->current_activity_id = activity.id();
    }
}

bool Character::has_activity( const activity_id &type ) const
{
    return activity.id() == type;
}

bool Character::has_activity( const std::vector<activity_id> &types ) const
{
    return std::find( types.begin(), types.end(), activity.id() ) != types.end();
}

void Character::cancel_activity()
{
    if( has_activity( activity_id( "ACT_MOVE_ITEMS" ) ) && is_hauling() ) {
        stop_hauling();
    }
    if( has_activity( activity_id( "ACT_TRY_SLEEP" ) ) ) {
        remove_value( "sleep_query" );
    }
    // Clear any backlog items that aren't auto-resume.
    for( auto backlog_item = backlog.begin(); backlog_item != backlog.end(); ) {
        if( backlog_item->auto_resume ) {
            backlog_item++;
        } else {
            backlog_item = backlog.erase( backlog_item );
        }
    }
    if( activity && activity.is_suspendable() ) {
        backlog.push_front( activity );
    }
    sfx::end_activity_sounds(); // kill activity sounds when canceled
    activity = player_activity();
}

void Character::resume_backlog_activity()
{
    if( !backlog.empty() && backlog.front().auto_resume ) {
        activity = backlog.front();
        backlog.pop_front();
    }
}

void Character::fall_asleep()
{
    // Communicate to the player that he is using items on the floor
    std::string item_name = is_snuggling();
    if( item_name == "many" ) {
        if( one_in( 15 ) ) {
            add_msg_if_player( _( "You nestle your pile of clothes for warmth." ) );
        } else {
            add_msg_if_player( _( "You use your pile of clothes for warmth." ) );
        }
    } else if( item_name != "nothing" ) {
        if( one_in( 15 ) ) {
            add_msg_if_player( _( "You snuggle your %s to keep warm." ), item_name );
        } else {
            add_msg_if_player( _( "You use your %s to keep warm." ), item_name );
        }
    }
    if( has_active_mutation( trait_id( "HIBERNATE" ) ) &&
        get_kcal_percent() > 0.8f ) {
        if( is_avatar() ) {
            g->memorial().add( pgettext( "memorial_male", "Entered hibernation." ),
                               pgettext( "memorial_female", "Entered hibernation." ) );
        }
        // some days worth of round-the-clock Snooze.  Cata seasons default to 91 days.
        fall_asleep( 10_days );
        // If you're not fatigued enough for 10 days, you won't sleep the whole thing.
        // In practice, the fatigue from filling the tank from (no msg) to Time For Bed
        // will last about 8 days.
    }

    fall_asleep( 10_hours ); // default max sleep time.
}

void Character::fall_asleep( const time_duration &duration )
{
    if( activity ) {
        if( activity.id() == "ACT_TRY_SLEEP" ) {
            activity.set_to_null();
        } else {
            cancel_activity();
        }
    }
    add_effect( effect_sleep, duration );
}

std::string Character::is_snuggling() const
{
    auto begin = g->m.i_at( pos() ).begin();
    auto end = g->m.i_at( pos() ).end();

    if( in_vehicle ) {
        if( const cata::optional<vpart_reference> vp = g->m.veh_at( pos() ).part_with_feature( VPFLAG_CARGO,
                false ) ) {
            vehicle *const veh = &vp->vehicle();
            const int cargo = vp->part_index();
            if( !veh->get_items( cargo ).empty() ) {
                begin = veh->get_items( cargo ).begin();
                end = veh->get_items( cargo ).end();
            }
        }
    }
    const item *floor_armor = nullptr;
    int ticker = 0;

    // If there are no items on the floor, return nothing
    if( begin == end ) {
        return "nothing";
    }

    for( auto candidate = begin; candidate != end; ++candidate ) {
        if( !candidate->is_armor() ) {
            continue;
        } else if( candidate->volume() > 250_ml && candidate->get_warmth() > 0 &&
                   ( candidate->covers( bp_torso ) || candidate->covers( bp_leg_l ) ||
                     candidate->covers( bp_leg_r ) ) ) {
            floor_armor = &*candidate;
            ticker++;
        }
    }

    if( ticker == 0 ) {
        return "nothing";
    } else if( ticker == 1 ) {
        return floor_armor->type_name();
    } else if( ticker > 1 ) {
        return "many";
    }

    return "nothing";
}

bool Character::has_item_with_flag( const std::string &flag, bool need_charges ) const
{
    return has_item_with( [&flag, &need_charges]( const item & it ) {
        if( it.is_tool() && need_charges ) {
            return it.has_flag( flag ) && it.type->tool->max_charges ? it.charges > 0 : it.has_flag( flag );
        }
        return it.has_flag( flag );
    } );
}

std::vector<const item *> Character::all_items_with_flag( const std::string &flag ) const
{
    return items_with( [&flag]( const item & it ) {
        return it.has_flag( flag );
    } );
}

bool Character::has_charges( const itype_id &it, int quantity,
                             const std::function<bool( const item & )> &filter ) const
{
    if( it == "fire" || it == "apparatus" ) {
        return has_fire( quantity );
    }
    if( it == "UPS" && is_mounted() &&
        mounted_creature.get()->has_flag( MF_RIDEABLE_MECH ) ) {
        auto mons = mounted_creature.get();
        return quantity <= mons->battery_item->ammo_remaining();
    }
    return charges_of( it, quantity, filter ) == quantity;
}

bool Character::has_fire( const int quantity ) const
{
    // TODO: Replace this with a "tool produces fire" flag.

    if( g->m.has_nearby_fire( pos() ) ) {
        return true;
    } else if( has_item_with_flag( "FIRE" ) ) {
        return true;
    } else if( has_item_with_flag( "FIRESTARTER" ) ) {
        auto firestarters = all_items_with_flag( "FIRESTARTER" );
        for( auto &i : firestarters ) {
            if( has_charges( i->typeId(), quantity ) ) {
                return true;
            }
        }
    } else if( has_active_bionic( bio_tools ) && get_power_level() > quantity * 5_kJ ) {
        return true;
    } else if( has_bionic( bio_lighter ) && get_power_level() > quantity * 5_kJ ) {
        return true;
    } else if( has_bionic( bio_laser ) && get_power_level() > quantity * 5_kJ ) {
        return true;
    } else if( is_npc() ) {
        // A hack to make NPCs use their Molotovs
        return true;
    }
    return false;
}

void Character::mod_painkiller( int npkill )
{
    set_painkiller( pkill + npkill );
}

void Character::set_painkiller( int npkill )
{
    npkill = std::max( npkill, 0 );
    if( pkill != npkill ) {
        const int prev_pain = get_perceived_pain();
        pkill = npkill;
        on_stat_change( "pkill", pkill );
        const int cur_pain = get_perceived_pain();

        if( cur_pain != prev_pain ) {
            react_to_felt_pain( cur_pain - prev_pain );
            on_stat_change( "perceived_pain", cur_pain );
        }
    }
}

int Character::get_painkiller() const
{
    return pkill;
}

bool Character::is_wielding( const item &target ) const
{
    return &weapon == &target;
}
