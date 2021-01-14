#include "creature.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <map>
#include <memory>

#include "anatomy.h"
#include "cached_options.h"
#include "calendar.h"
#include "character.h"
#include "color.h"
#include "cursesdef.h"
#include "damage.h"
#include "debug.h"
#include "effect.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "field.h"
#include "game.h"
#include "game_constants.h"
#include "int_id.h"
#include "item.h"
#include "json.h"
#include "lightmap.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "optional.h"
#include "output.h"
#include "point.h"
#include "projectile.h"
#include "rng.h"
#include "string_id.h"
#include "translations.h"
#include "units.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "options.h"

struct mutation_branch;

static const anatomy_id anatomy_human_anatomy( "human_anatomy" );

static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_blind( "blind" );
static const efftype_id effect_bounced( "bounced" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_dripping_mechanical_fluid( "dripping_mechanical_fluid" );
static const efftype_id effect_foamcrete_slow( "foamcrete_slow" );
static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_no_sight( "no_sight" );
static const efftype_id effect_npc_suspend( "npc_suspend" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_paralyzepoison( "paralyzepoison" );
static const efftype_id effect_ridden( "ridden" );
static const efftype_id effect_riding( "riding" );
static const efftype_id effect_sap( "sap" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_sensor_stun( "sensor_stun" );
static const efftype_id effect_tied( "tied" );
static const efftype_id effect_zapped( "zapped" );

static const species_id species_ROBOT( "ROBOT" );

const std::map<std::string, creature_size> Creature::size_map = {
    {"TINY",   creature_size::tiny},
    {"SMALL",  creature_size::small},
    {"MEDIUM", creature_size::medium},
    {"LARGE",  creature_size::large},
    {"HUGE",   creature_size::huge}
};

const std::set<material_id> Creature::cmat_flesh{
    material_id( "flesh" ), material_id( "iflesh" )
};
const std::set<material_id> Creature::cmat_fleshnveg{
    material_id( "flesh" ),  material_id( "iflesh" ), material_id( "veggy" )
};
const std::set<material_id> Creature::cmat_flammable{
    material_id( "paper" ), material_id( "powder" ), material_id( "wood" ),
    material_id( "cotton" ), material_id( "wool" )
};
const std::set<material_id> Creature::cmat_flameres{
    material_id( "stone" ), material_id( "kevlar" ), material_id( "steel" )
};

Creature::Creature()
{
    moves = 0;
    pain = 0;
    killer = nullptr;
    speed_base = 100;
    underwater = false;

    Creature::reset_bonuses();

    fake = false;
}

Creature::~Creature() = default;

std::vector<std::string> Creature::get_grammatical_genders() const
{
    // Returning empty list means we use the language-specified default
    return {};
}

void Creature::normalize()
{
}

void Creature::reset()
{
    reset_bonuses();
    reset_stats();
}

void Creature::bleed() const
{
    get_map().add_splatter( bloodType(), pos() );
}

void Creature::reset_bonuses()
{
    num_blocks = 1;
    num_dodges = 1;
    num_blocks_bonus = 0;
    num_dodges_bonus = 0;

    armor_bash_bonus = 0;
    armor_cut_bonus = 0;
    armor_bullet_bonus = 0;

    speed_bonus = 0;
    dodge_bonus = 0;
    block_bonus = 0;
    hit_bonus = 0;
    bash_bonus = 0;
    cut_bonus = 0;
    size_bonus = 0;

    bash_mult = 1.0f;
    cut_mult = 1.0f;

    melee_quiet = false;
    throw_resist = 0;
}

void Creature::process_turn()
{
    if( is_dead_state() ) {
        return;
    }
    reset_bonuses();

    process_effects();

    process_damage_over_time();

    // Call this in case any effects have changed our stats
    reset_stats();

    // add an appropriate number of moves
    if( !has_effect( effect_ridden ) ) {
        moves += get_speed();
    }
}

bool Creature::is_underwater() const
{
    return underwater;
}

bool Creature::is_ranged_attacker() const
{
    if( has_flag( MF_RANGED_ATTACKER ) ) {
        return true;
    }

    const monster *mon = as_monster();
    if( mon ) {
        for( const std::pair<const std::string, mtype_special_attack> &attack :
             mon->type->special_attacks ) {
            if( attack.second->id == "gun" ) {
                return true;
            }
        }
    }
    //TODO Potentially add check for this as npc wielding ranged weapon

    return false;
}

bool Creature::digging() const
{
    return false;
}

bool Creature::is_dangerous_fields( const field &fld ) const
{
    // Else check each field to see if it's dangerous to us
    for( const auto &dfield : fld ) {
        if( is_dangerous_field( dfield.second ) ) {
            return true;
        }
    }
    // No fields were found to be dangerous, so the field set isn't dangerous
    return false;
}

bool Creature::is_dangerous_field( const field_entry &entry ) const
{
    // If it's dangerous and we're not immune return true, else return false
    return entry.is_dangerous() && !is_immune_field( entry.get_field_type() );
}

bool Creature::sees( const Creature &critter ) const
{
    // Creatures always see themselves (simplifies drawing).
    if( &critter == this ) {
        return true;
    }

    map &here = get_map();
    // player can use mirrors, so `has_potential_los` cannot be used
    if( !is_player() && !here.has_potential_los( pos(), critter.pos() ) ) {
        return false;
    }

    if( critter.is_hallucination() ) {
        // hallucinations are imaginations of the player character, npcs or monsters don't hallucinate.
        // Invisible hallucinations would be pretty useless (nobody would see them at all), therefor
        // the player will see them always.
        return is_player();
    }

    if( !fov_3d && !debug_mode && posz() != critter.posz() ) {
        return false;
    }

    // This check is ridiculously expensive so defer it to after everything else.
    auto visible = []( const Character * ch ) {
        return ch == nullptr || !ch->is_invisible();
    };

    const Character *ch = critter.as_character();
    const int wanted_range = rl_dist( pos(), critter.pos() );

    // Can always see adjacent monsters on the same level.
    // We also bypass lighting for vertically adjacent monsters, but still check for floors.
    if( wanted_range <= 1 && ( posz() == critter.posz() || here.sees( pos(), critter.pos(), 1 ) ) ) {
        return visible( ch );
    } else if( ( wanted_range > 1 && critter.digging() ) ||
               ( critter.has_flag( MF_NIGHT_INVISIBILITY ) && here.light_at( critter.pos() ) <= lit_level::LOW ) ||
               ( critter.is_underwater() && !is_underwater() && here.is_divable( critter.pos() ) ) ||
               ( here.has_flag_ter_or_furn( TFLAG_HIDE_PLACE, critter.pos() ) &&
                 !( std::abs( posx() - critter.posx() ) <= 1 && std::abs( posy() - critter.posy() ) <= 1 &&
                    std::abs( posz() - critter.posz() ) <= 1 ) ) ) {
        return false;
    }
    if( ch != nullptr ) {
        if( ch->is_crouching() ) {
            const int coverage = here.obstacle_coverage( pos(), critter.pos() );
            if( coverage < 30 ) {
                return sees( critter.pos(), critter.is_avatar() ) && visible( ch );
            }
            float size_modifier = 1.0f;
            switch( ch->get_size() ) {
                case creature_size::tiny:
                    size_modifier = 2.0f;
                    break;
                case creature_size::small:
                    size_modifier = 1.4f;
                    break;
                case creature_size::medium:
                    break;
                case creature_size::large:
                    size_modifier = 0.6f;
                    break;
                case creature_size::huge:
                    size_modifier = 0.15f;
                    break;
                case creature_size::num_sizes:
                    debugmsg( "ERROR: Creature has invalid size class." );
                    break;
            }
            const int vision_modifier = 30 - 0.5 * coverage * size_modifier;
            if( vision_modifier > 1 ) {
                return sees( critter.pos(), critter.is_avatar(), vision_modifier ) && visible( ch );
            }
            return false;
        }
    }
    return sees( critter.pos(), critter.is_avatar() ) && visible( ch );
}

bool Creature::sees( const tripoint &t, bool is_avatar, int range_mod ) const
{
    if( !fov_3d && posz() != t.z ) {
        return false;
    }

    map &here = get_map();
    const int range_cur = sight_range( here.ambient_light_at( t ) );
    const int range_day = sight_range( default_daylight_level() );
    const int range_night = sight_range( 0 );
    const int range_max = std::max( range_day, range_night );
    const int range_min = std::min( range_cur, range_max );
    const int wanted_range = rl_dist( pos(), t );
    if( wanted_range <= range_min ||
        ( wanted_range <= range_max &&
          here.ambient_light_at( t ) > g->natural_light_level( t.z ) ) ) {
        int range = 0;
        if( here.ambient_light_at( t ) > g->natural_light_level( t.z ) ) {
            range = MAX_VIEW_DISTANCE;
        } else {
            range = range_min;
        }
        if( has_effect( effect_no_sight ) ) {
            range = 1;
        }
        if( range_mod > 0 ) {
            range = std::min( range, range_mod );
        }
        if( is_avatar ) {
            // Special case monster -> player visibility, forcing it to be symmetric with player vision.
            const float player_visibility_factor = get_player_character().visibility() / 100.0f;
            int adj_range = std::floor( range * player_visibility_factor );
            return adj_range >= wanted_range &&
                   here.get_cache_ref( pos().z ).seen_cache[pos().x][pos().y] > LIGHT_TRANSPARENCY_SOLID;
        } else {
            return here.sees( pos(), t, range );
        }
    } else {
        return false;
    }
}

// Helper function to check if potential area of effect of a weapon overlaps vehicle
// Maybe TODO: If this is too slow, precalculate a bounding box and clip the tested area to it
static bool overlaps_vehicle( const std::set<tripoint> &veh_area, const tripoint &pos,
                              const int area )
{
    for( const tripoint &tmp : tripoint_range<tripoint>( pos - tripoint( area, area, 0 ),
            pos + tripoint( area - 1, area - 1, 0 ) ) ) {
        if( veh_area.count( tmp ) > 0 ) {
            return true;
        }
    }

    return false;
}

Creature *Creature::auto_find_hostile_target( int range, int &boo_hoo, int area )
{
    Creature *target = nullptr;
    Character &player_character = get_player_character();
    tripoint player_pos = player_character.pos();
    constexpr int hostile_adj = 2; // Priority bonus for hostile targets
    const int iff_dist = ( range + area ) * 3 / 2 + 6; // iff check triggers at this distance
    // iff safety margin (degrees). less accuracy, more paranoia
    units::angle iff_hangle = units::from_degrees( 15 + area );
    float best_target_rating = -1.0f; // bigger is better
    units::angle u_angle = {};         // player angle relative to turret
    boo_hoo = 0;         // how many targets were passed due to IFF. Tragically.
    bool self_area_iff = false; // Need to check if the target is near the vehicle we're a part of
    bool area_iff = false;      // Need to check distance from target to player
    bool angle_iff = true;      // Need to check if player is in a cone between us and target
    int pldist = rl_dist( pos(), player_pos );
    map &here = get_map();
    vehicle *in_veh = is_fake() ? veh_pointer_or_null( here.veh_at( pos() ) ) : nullptr;
    if( pldist < iff_dist && sees( player_character ) ) {
        area_iff = area > 0;
        // Player inside vehicle won't be hit by shots from the roof,
        // so we can fire "through" them just fine.
        const optional_vpart_position vp = here.veh_at( player_pos );
        if( in_veh && veh_pointer_or_null( vp ) == in_veh && vp->is_inside() ) {
            angle_iff = false; // No angle IFF, but possibly area IFF
        } else if( pldist < 3 ) {
            // granularity increases with proximity
            iff_hangle = ( pldist == 2 ? 30_degrees : 60_degrees );
        }
        u_angle = coord_to_angle( pos(), player_pos );
    }

    if( area > 0 && in_veh != nullptr ) {
        self_area_iff = true;
    }

    std::vector<Creature *> targets = g->get_creatures_if( [&]( const Creature & critter ) {
        if( critter.is_monster() ) {
            // friendly to the player, not a target for us
            return static_cast<const monster *>( &critter )->friendly == 0;
        }
        if( critter.is_npc() ) {
            // friendly to the player, not a target for us
            return static_cast<const npc *>( &critter )->get_attitude() == NPCATT_KILL;
        }
        // TODO: what about g->u?
        return false;
    } );
    for( auto &m : targets ) {
        if( !sees( *m ) ) {
            // can't see nor sense it
            if( is_fake() && in_veh ) {
                // If turret in the vehicle then
                // Hack: trying yo avoid turret LOS blocking by frames bug by trying to see target from vehicle boundary
                // Or turret wallhack for turret's car
                // TODO: to visibility checking another way, probably using 3D FOV
                std::vector<tripoint> path_to_target = line_to( pos(), m->pos() );
                path_to_target.insert( path_to_target.begin(), pos() );

                // Getting point on vehicle boundaries and on line between target and turret
                bool continueFlag = true;
                do {
                    const optional_vpart_position vp = here.veh_at( path_to_target.back() );
                    vehicle *const veh = vp ? &vp->vehicle() : nullptr;
                    if( in_veh == veh ) {
                        continueFlag = false;
                    } else {
                        path_to_target.pop_back();
                    }
                } while( continueFlag );

                tripoint oldPos = pos();
                setpos( path_to_target.back() ); //Temporary moving targeting npc on vehicle boundary postion
                bool seesFromVehBound = sees( *m ); // And look from there
                setpos( oldPos );
                if( !seesFromVehBound ) {
                    continue;
                }
            } else {
                continue;
            }
        }
        int dist = rl_dist( pos(), m->pos() ) + 1; // rl_dist can be 0
        if( dist > range + 1 || dist < area ) {
            // Too near or too far
            continue;
        }
        // Prioritize big, armed and hostile stuff
        float mon_rating = m->power_rating();
        float target_rating = mon_rating / dist;
        if( mon_rating + hostile_adj <= 0 ) {
            // We wouldn't attack it even if it was hostile
            continue;
        }

        if( in_veh != nullptr && veh_pointer_or_null( here.veh_at( m->pos() ) ) == in_veh ) {
            // No shooting stuff on vehicle we're a part of
            continue;
        }
        if( area_iff && rl_dist( player_pos, m->pos() ) <= area ) {
            // Player in AoE
            boo_hoo++;
            continue;
        }
        // Hostility check can be expensive, but we need to inform the player of boo_hoo
        // only when the target is actually "hostile enough"
        bool maybe_boo = false;
        if( angle_iff ) {
            units::angle tangle = coord_to_angle( pos(), m->pos() );
            units::angle diff = units::fabs( u_angle - tangle );
            // Player is in the angle and not too far behind the target
            if( ( diff + iff_hangle > 360_degrees || diff < iff_hangle ) &&
                ( dist * 3 / 2 + 6 > pldist ) ) {
                maybe_boo = true;
            }
        }
        if( !maybe_boo && ( ( mon_rating + hostile_adj ) / dist <= best_target_rating ) ) {
            // "Would we skip the target even if it was hostile?"
            // Helps avoid (possibly expensive) attitude calculation
            continue;
        }
        if( m->attitude_to( player_character ) == Attitude::HOSTILE ) {
            target_rating = ( mon_rating + hostile_adj ) / dist;
            if( maybe_boo ) {
                boo_hoo++;
                continue;
            }
        }
        if( target_rating <= best_target_rating || target_rating <= 0 ) {
            continue; // Handle this late so that boo_hoo++ can happen
        }
        // Expensive check for proximity to vehicle
        if( self_area_iff && overlaps_vehicle( in_veh->get_points(), m->pos(), area ) ) {
            continue;
        }

        target = m;
        best_target_rating = target_rating;
    }
    return target;
}

/*
 * Damage-related functions
 */

int Creature::size_melee_penalty() const
{
    switch( get_size() ) {
        case creature_size::tiny:
            return 30;
        case creature_size::small:
            return 15;
        case creature_size::medium:
            return 0;
        case creature_size::large:
            return -10;
        case creature_size::huge:
            return -20;
        case creature_size::num_sizes:
            debugmsg( "ERROR: Creature has invalid size class." );
            return 0;
    }

    debugmsg( "Invalid target size %d", get_size() );
    return 0;
}

bool Creature::is_adjacent( const Creature *target, const bool allow_z_levels ) const
{
    if( target == nullptr ) {
        return false;
    }

    if( rl_dist( pos(), target->pos() ) != 1 ) {
        return false;
    }

    map &here = get_map();
    if( posz() == target->posz() ) {
        return
            /* either target or attacker are underwater and separated by vehicle tiles */
            !( underwater != target->underwater &&
               here.veh_at( pos() ) && here.veh_at( target->pos() ) );
    }

    if( !allow_z_levels ) {
        return false;
    }

    // The square above must have no floor.
    // The square below must have no ceiling (i.e. no floor on the tile above it).
    const bool target_above = target->posz() > posz();
    const tripoint &up   = target_above ? target->pos() : pos();
    const tripoint &down = target_above ? pos() : target->pos();
    const tripoint above{ down.xy(), up.z };
    return ( !here.has_floor( up ) || here.ter( up )->has_flag( TFLAG_GOES_DOWN ) ) &&
           ( !here.has_floor( above ) || here.ter( above )->has_flag( TFLAG_GOES_DOWN ) );
}

int Creature::deal_melee_attack( Creature *source, int hitroll )
{
    const float dodge = dodge_roll();
    int hit_spread = hitroll - dodge - size_melee_penalty();
    if( has_flag( MF_IMMOBILE ) ) {
        // Under normal circumstances, even a clumsy person would
        // not miss a turret.  It should, however, be possible to
        // miss a smaller target, especially when wielding a
        // clumsy weapon or when severely encumbered.
        hit_spread += 40;
    }

    // If attacker missed call targets on_dodge event
    if( dodge > 0.0 && hit_spread <= 0 && source != nullptr && !source->is_hallucination() ) {
        on_dodge( source, source->get_melee() );
    }

    return hit_spread;
}

void Creature::deal_melee_hit( Creature *source, int hit_spread, bool critical_hit,
                               damage_instance dam, dealt_damage_instance &dealt_dam )
{
    if( source == nullptr || source->is_hallucination() ) {
        dealt_dam.bp_hit = anatomy_id( "human_anatomy" )->random_body_part();
        return;
    }

    dam = source->modify_damage_dealt_with_enchantments( dam );

    // If carrying a rider, there is a chance the hits may hit rider instead.
    // melee attack will start off as targeted at mount
    if( has_effect( effect_ridden ) ) {
        monster *mons = dynamic_cast<monster *>( this );
        if( mons && mons->mounted_player ) {
            if( !mons->has_flag( MF_MECH_DEFENSIVE ) &&
                one_in( std::max( 2, mons->get_size() - mons->mounted_player->get_size() ) ) ) {
                mons->mounted_player->deal_melee_hit( source, hit_spread, critical_hit, dam, dealt_dam );
                return;
            }
        }
    }
    damage_instance d = dam; // copy, since we will mutate in block_hit
    bodypart_id bp_hit =  select_body_part( source, hit_spread );
    block_hit( source, bp_hit, d );

    // Stabbing effects
    int stab_moves = rng( d.type_damage( damage_type::STAB ) / 2,
                          d.type_damage( damage_type::STAB ) * 1.5 );
    if( critical_hit ) {
        stab_moves *= 1.5;
    }
    if( stab_moves >= 150 && !is_immune_effect( effect_downed ) ) {
        if( is_player() ) {
            source->add_msg_if_npc( m_bad, _( "<npcname> forces you to the ground!" ) );
        } else {
            source->add_msg_player_or_npc( m_good, _( "You force %s to the ground!" ),
                                           _( "<npcname> forces %s to the ground!" ),
                                           disp_name() );
        }

        add_effect( source, effect_downed, 1_turns );
        mod_moves( -stab_moves / 2 );
    } else {
        mod_moves( -stab_moves );
    }

    on_hit( source, bp_hit ); // trigger on-gethit events
    dealt_dam = deal_damage( source, bp_hit, d );
    dealt_dam.bp_hit = bp_hit;

    // Bashing critical
    if( critical_hit && !is_immune_effect( effect_stunned ) &&
        dealt_dam.type_damage( damage_type::BASH ) > 0 ) {
        // check if raw bash damage is enough to stun
        if( d.type_damage( damage_type::BASH ) * hit_spread > get_hp_max() ) {
            add_effect( source, effect_stunned, 1_turns ); // 1 turn is enough
            if( source->is_avatar() ) {
                add_msg( m_good, _( "You stun %s with your blow." ), disp_name() );
            }
        }
    }
}

/**
 * Attempts to harm a creature with a projectile.
 *
 * @param source Pointer to the creature who shot the projectile.
 * @param attack A structure describing the attack and its results.
 * @param print_messages enables message printing by default.
 */
void Creature::deal_projectile_attack( Creature *source, dealt_projectile_attack &attack,
                                       bool print_messages )
{
    const bool magic = attack.proj.proj_effects.count( "magic" ) > 0;
    const double missed_by = attack.missed_by;
    if( missed_by >= 1.0 && !magic ) {
        // Total miss
        return;
    }
    // If carrying a rider, there is a chance the hits may hit rider instead.
    if( has_effect( effect_ridden ) ) {
        monster *mons = dynamic_cast<monster *>( this );
        if( mons && mons->mounted_player ) {
            if( !mons->has_flag( MF_MECH_DEFENSIVE ) &&
                one_in( std::max( 2, mons->get_size() - mons->mounted_player->get_size() ) ) ) {
                mons->mounted_player->deal_projectile_attack( source, attack, print_messages );
                return;
            }
        }
    }
    const projectile &proj = attack.proj;
    dealt_damage_instance &dealt_dam = attack.dealt_dam;
    const auto &proj_effects = proj.proj_effects;

    viewer &player_view = get_player_view();
    const bool u_see_this = player_view.sees( *this );

    const int avoid_roll = dodge_roll();
    // Do dice(10, speed) instead of dice(speed, 10) because speed could potentially be > 10000
    const int diff_roll = dice( 10, proj.speed );
    // Partial dodge, capped at [0.0, 1.0], added to missed_by
    const double dodge_rescaled = avoid_roll / static_cast<double>( diff_roll );
    const double goodhit = missed_by + std::max( 0.0, std::min( 1.0, dodge_rescaled ) );

    if( goodhit >= 1.0 && !magic ) {
        attack.missed_by = 1.0; // Arbitrary value
        if( !print_messages ) {
            return;
        }
        // "Avoid" rather than "dodge", because it includes removing self from the line of fire
        //  rather than just Matrix-style bullet dodging
        if( source != nullptr && player_view.sees( *source ) ) {
            add_msg_player_or_npc(
                m_warning,
                _( "You avoid %s projectile!" ),
                _( "<npcname> avoids %s projectile." ),
                source->disp_name( true ) );
        } else {
            add_msg_player_or_npc(
                m_warning,
                _( "You avoid an incoming projectile!" ),
                _( "<npcname> avoids an incoming projectile." ) );
        }
        return;
    }

    // Bounce applies whether it does damage or not.
    if( proj.proj_effects.count( "BOUNCE" ) ) {
        add_effect( source, effect_bounced, 1_turns );
    }

    bodypart_id bp_hit;
    double hit_value = missed_by + rng_float( -0.5, 0.5 );
    // Headshots considered elsewhere
    if( hit_value <= 0.4 || magic ) {
        bp_hit = bodypart_id( "torso" );
    } else if( one_in( 4 ) ) {
        if( one_in( 2 ) ) {
            bp_hit = bodypart_id( "leg_l" );
        } else {
            bp_hit = bodypart_id( "leg_r" );
        }
    } else {
        if( one_in( 2 ) ) {
            bp_hit = bodypart_id( "arm_l" );
        } else {
            bp_hit = bodypart_id( "arm_r" );
        }
    }

    double damage_mult = 1.0;

    const float crit_multiplier = proj.critical_multiplier;
    const int max_damage = proj.impact.total_damage();
    std::string message;
    game_message_type gmtSCTcolor = m_neutral;
    float world_multiplier = get_option<int>( "MONSTER_RESILIENCE" ) / 100.0f;
    if( magic ) {
        damage_mult *= rng_float( 0.9, 1.1 );
    } else if( goodhit < accuracy_headshot &&
               max_damage * crit_multiplier > get_hp_max( bodypart_id( "head" ) ) / world_multiplier ) {
        message = _( "Headshot!" );
        gmtSCTcolor = m_headshot;
        damage_mult *= rng_float( 0.95, 1.05 );
        damage_mult *= crit_multiplier;
        bp_hit = bodypart_id( "head" ); // headshot hits the head, of course
    } else if( goodhit < accuracy_critical &&
               max_damage * crit_multiplier > get_hp_max( bodypart_id( "torso" ) ) / world_multiplier ) {
        message = _( "Critical!" );
        gmtSCTcolor = m_critical;
        damage_mult *= rng_float( 0.75, 1.0 );
        damage_mult *= crit_multiplier;
    } else if( goodhit < accuracy_goodhit ) {
        message = _( "Good hit!" );
        gmtSCTcolor = m_good;
        damage_mult *= rng_float( 0.5, 0.75 );
        damage_mult *= std::sqrt( 2.0 * crit_multiplier );
    } else if( goodhit < accuracy_standard ) {
        damage_mult *= rng_float( 0.5, 1 );

    } else if( goodhit < accuracy_grazing ) {
        message = _( "Grazing hit." );
        gmtSCTcolor = m_grazing;
        damage_mult *= rng_float( 0, .25 );
    }

    if( print_messages && source != nullptr && !message.empty() && u_see_this ) {
        source->add_msg_if_player( m_good, message );
    }

    attack.missed_by = goodhit;

    // copy it, since we're mutating
    damage_instance impact = proj.impact;
    if( damage_mult > 0.0f && proj_effects.count( "NO_DAMAGE_SCALING" ) ) {
        damage_mult = 1.0f;
    }

    impact.mult_damage( damage_mult );

    if( proj_effects.count( "NOGIB" ) > 0 ) {
        float dmg_ratio = static_cast<float>( impact.total_damage() ) / get_hp_max( bp_hit );
        if( dmg_ratio > 1.25f ) {
            impact.mult_damage( 1.0f / dmg_ratio );
        }
    }

    dealt_dam = deal_damage( source, bp_hit, impact );
    dealt_dam.bp_hit = bp_hit;

    // Apply ammo effects to target.
    if( proj.proj_effects.count( "TANGLE" ) ) {
        monster *z = dynamic_cast<monster *>( this );
        Character *n = dynamic_cast<Character *>( this );
        // if its a tameable animal, its a good way to catch them if they are running away, like them ranchers do!
        // we assume immediate success, then certain monster types immediately break free in monster.cpp move_effects()
        if( z ) {
            const item &drop_item = proj.get_drop();
            if( !drop_item.is_null() ) {
                z->add_effect( source, effect_tied, 1_turns, true );
                z->tied_item = cata::make_value<item>( drop_item );
            } else {
                add_msg_debug( "projectile with TANGLE effect, but no drop item specified" );
            }
        } else if( n && !is_immune_effect( effect_downed ) ) {
            // no tied up effect for people yet, just down them and stun them, its close enough to the desired effect.
            // we can assume a person knows how to untangle their legs eventually and not panic like an animal.
            add_effect( source, effect_downed, 1_turns );
            // stunned to simulate staggering around and stumbling trying to get the entangled thing off of them.
            add_effect( source, effect_stunned, rng( 3_turns, 8_turns ) );
        }
    }
    if( proj.proj_effects.count( "INCENDIARY" ) ) {
        if( made_of( material_id( "veggy" ) ) || made_of_any( cmat_flammable ) ) {
            add_effect( source, effect_onfire, rng( 2_turns, 6_turns ), bp_hit );
        } else if( made_of_any( cmat_flesh ) && one_in( 4 ) ) {
            add_effect( source, effect_onfire, rng( 1_turns, 4_turns ), bp_hit );
        }
    } else if( proj.proj_effects.count( "IGNITE" ) ) {
        if( made_of( material_id( "veggy" ) ) || made_of_any( cmat_flammable ) ) {
            add_effect( source, effect_onfire, 6_turns, bp_hit );
        } else if( made_of_any( cmat_flesh ) ) {
            add_effect( source, effect_onfire, 10_turns, bp_hit );
        }
    }

    if( proj.proj_effects.count( "ROBOT_DAZZLE" ) ) {
        if( goodhit < accuracy_critical && in_species( species_ROBOT ) ) {
            time_duration duration = rng( 6_turns, 8_turns );
            add_effect( source, effect_stunned, duration );
            add_effect( source, effect_sensor_stun, duration );
            add_msg( source->is_player() ?
                     _( "You land a clean shot on the %1$s sensors, stunning it." ) :
                     _( "The %1$s is stunned!" ),
                     disp_name( true ) );
        }
    }

    if( bp_hit == bodypart_id( "head" ) && proj_effects.count( "BLINDS_EYES" ) ) {
        // TODO: Change this to require bp_eyes
        add_env_effect( effect_blind, bodypart_id( "eyes" ), 5, rng( 3_turns, 10_turns ) );
    }

    if( proj_effects.count( "APPLY_SAP" ) ) {
        add_effect( source, effect_sap, 1_turns * dealt_dam.total_damage() );
    }
    if( proj_effects.count( "PARALYZEPOISON" ) && dealt_dam.total_damage() > 0 ) {
        add_msg_if_player( m_bad, _( "You feel poison coursing through your body!" ) );
        add_effect( source, effect_paralyzepoison, 5_minutes );
    }

    if( proj_effects.count( "FOAMCRETE" ) &&  effect_foamcrete_slow.is_valid() ) {
        add_msg_if_player( m_bad, _( "The foamcrete stiffens around you!" ) );
        add_effect( source, effect_foamcrete_slow, 5_minutes );
    }

    int stun_strength = 0;
    if( proj.proj_effects.count( "BEANBAG" ) ) {
        stun_strength = 4;
    }
    if( proj.proj_effects.count( "LARGE_BEANBAG" ) ) {
        stun_strength = 16;
    }
    if( stun_strength > 0 ) {
        switch( get_size() ) {
            case creature_size::tiny:
                stun_strength *= 4;
                break;
            case creature_size::small:
                stun_strength *= 2;
                break;
            case creature_size::medium:
            default:
                break;
            case creature_size::large:
                stun_strength /= 2;
                break;
            case creature_size::huge:
                stun_strength /= 4;
                break;
        }
        add_effect( source, effect_stunned, 1_turns * rng( stun_strength / 2, stun_strength ) );
    }

    if( u_see_this && print_messages ) {
        if( damage_mult == 0 ) {
            if( source != nullptr ) {
                add_msg( source->is_player() ? _( "You miss!" ) : _( "The shot misses!" ) );
            }
        } else if( dealt_dam.total_damage() == 0 ) {
            //~ 1$ - monster name, 2$ - character's bodypart or monster's skin/armor
            add_msg( _( "The shot reflects off %1$s %2$s!" ), disp_name( true ),
                     is_monster() ?
                     skin_name() :
                     body_part_name_accusative( bp_hit ) );
        } else if( is_player() ) {
            //monster hits player ranged
            //~ Hit message. 1$s is bodypart name in accusative. 2$d is damage value.
            add_msg_if_player( m_bad, _( "You were hit in the %1$s for %2$d damage." ),
                               body_part_name_accusative( bp_hit ),
                               dealt_dam.total_damage() );
        } else if( source != nullptr ) {
            if( source->is_player() ) {
                //player hits monster ranged
                SCT.add( point( posx(), posy() ),
                         direction_from( point_zero, point( posx() - source->posx(), posy() - source->posy() ) ),
                         get_hp_bar( dealt_dam.total_damage(), get_hp_max(), true ).first,
                         m_good, message, gmtSCTcolor );

                if( get_hp() > 0 ) {
                    SCT.add( point( posx(), posy() ),
                             direction_from( point_zero, point( posx() - source->posx(), posy() - source->posy() ) ),
                             get_hp_bar( get_hp(), get_hp_max(), true ).first, m_good,
                             //~ "hit points", used in scrolling combat text
                             _( "hp" ), m_neutral, "hp" );
                } else {
                    SCT.removeCreatureHP();
                }

                //~ %1$s: creature name, %2$d: damage value
                add_msg( m_good, _( "You hit %1$s for %2$d damage." ),
                         disp_name(), dealt_dam.total_damage() );
            } else if( u_see_this ) {
                //~ 1$ - shooter, 2$ - target
                add_msg( _( "%1$s shoots %2$s." ),
                         source->disp_name(), disp_name() );
            }
        }
    }

    check_dead_state();
    attack.hit_critter = this;
    attack.missed_by = goodhit;
}

dealt_damage_instance Creature::deal_damage( Creature *source, bodypart_id bp,
        const damage_instance &dam )
{
    if( is_dead_state() ) {
        return dealt_damage_instance();
    }
    int total_damage = 0;
    int total_pain = 0;
    damage_instance d = dam; // copy, since we will mutate in absorb_hit

    dealt_damage_instance dealt_dams;
    absorb_hit( bp, d );

    // Add up all the damage units dealt
    for( const auto &it : d.damage_units ) {
        int cur_damage = 0;
        deal_damage_handle_type( source, it, bp, cur_damage, total_pain );
        if( cur_damage > 0 ) {
            dealt_dams.dealt_dams[ static_cast<int>( it.type ) ] += cur_damage;
            total_damage += cur_damage;
        }
    }

    mod_pain( total_pain );

    apply_damage( source, bp, total_damage );
    return dealt_dams;
}
void Creature::deal_damage_handle_type( const effect_source &source, const damage_unit &du,
                                        bodypart_id bp, int &damage, int &pain )
{
    // Handles ACIDPROOF, electric immunity etc.
    if( is_immune_damage( du.type ) ) {
        return;
    }

    // Apply damage multiplier from skill, critical hits or grazes after all other modifications.
    const int adjusted_damage = du.amount * du.damage_multiplier * du.unconditional_damage_mult;
    if( adjusted_damage <= 0 ) {
        return;
    }

    float div = 4.0f;

    switch( du.type ) {
        case damage_type::BASH:
            // Bashing damage is less painful
            div = 5.0f;
            break;

        case damage_type::HEAT:
            // heat damage sets us on fire sometimes
            if( rng( 0, 100 ) < adjusted_damage ) {
                add_effect( source, effect_onfire, rng( 1_turns, 3_turns ), bp );
            }
            break;

        case damage_type::ELECTRIC:
            // Electrical damage adds a major speed/dex debuff
            add_effect( source, effect_zapped, 1_turns * std::max( adjusted_damage, 2 ) );
            break;

        case damage_type::ACID:
            // Acid damage and acid burns are more painful
            div = 3.0f;
            break;

        case damage_type::CUT:
        case damage_type::STAB:
        case damage_type::BULLET:
            // these are bleed inducing damage types
            if( is_avatar() || is_npc() ) {
                as_character()->make_bleed( source, bp, 1_minutes * rng( 1, adjusted_damage ) );
            } else if( in_species( species_ROBOT ) ) {
                add_effect( source, effect_dripping_mechanical_fluid, 1_seconds * rng( 1, adjusted_damage ), bp );
            } else {
                add_effect( source, effect_bleed, 1_minutes * rng( 1, adjusted_damage ), bp );
            }

        default:
            break;
    }

    on_damage_of_type( adjusted_damage, du.type, bp );

    damage += adjusted_damage;
    pain += roll_remainder( adjusted_damage / div );
}

void Creature::heal_bp( bodypart_id /* bp */, int /* dam */ )
{
}

/*
 * State check functions
 */

bool Creature::is_warm() const
{
    return true;
}

bool Creature::in_species( const species_id & ) const
{
    return false;
}

bool Creature::is_fake() const
{
    return fake;
}

void Creature::set_fake( const bool fake_value )
{
    fake = fake_value;
}

void Creature::add_effect( const effect_source &source, const effect &eff, bool force,
                           bool deferred )
{
    add_effect( source, eff.get_id(), eff.get_duration(), eff.get_bp(), eff.is_permanent(),
                eff.get_intensity(), force, deferred );
}

void Creature::add_effect( const effect_source &source, const efftype_id &eff_id,
                           const time_duration &dur, bodypart_id bp, bool permanent, int intensity, bool force, bool deferred )
{
    // Check our innate immunity
    if( !force && is_immune_effect( eff_id ) ) {
        return;
    }
    if( eff_id == efftype_id( "knockdown" ) && ( has_effect( effect_ridden ) ||
            has_effect( effect_riding ) ) ) {
        monster *mons = dynamic_cast<monster *>( this );
        if( mons && mons->mounted_player ) {
            mons->mounted_player->forced_dismount();
        }
    }

    if( !eff_id.is_valid() ) {
        debugmsg( "Invalid effect, ID: %s", eff_id.c_str() );
        return;
    }
    const effect_type &type = eff_id.obj();

    // Mutate to a main (HP'd) body_part if necessary.
    if( type.get_main_parts() ) {
        bp = bp->main_part;
    }

    bool found = false;
    // Check if we already have it
    auto matching_map = effects->find( eff_id );
    if( matching_map != effects->end() ) {
        auto &bodyparts = matching_map->second;
        auto found_effect = bodyparts.find( bp );
        if( found_effect != bodyparts.end() ) {
            found = true;
            effect &e = found_effect->second;
            const int prev_int = e.get_intensity();
            // If we do, mod the duration, factoring in the mod value
            e.mod_duration( dur * e.get_dur_add_perc() / 100 );
            // Limit to max duration
            if( e.get_max_duration() > 0_turns && e.get_duration() > e.get_max_duration() ) {
                e.set_duration( e.get_max_duration() );
            }
            // Adding a permanent effect makes it permanent
            if( e.is_permanent() ) {
                e.pause_effect();
            }
            // int_dur_factor overrides all other intensity settings
            // ...but it's handled in set_duration, so explicitly do nothing here
            if( e.get_int_dur_factor() > 0_turns ) {
                // Set intensity if value is given
            } else if( intensity > 0 ) {
                e.set_intensity( intensity );
                // Else intensity uses the type'd step size if it already exists
            } else if( e.get_int_add_val() != 0 ) {
                e.mod_intensity( e.get_int_add_val() );
            }

            // Bound intensity by [1, max intensity]
            if( e.get_intensity() < 1 ) {
                add_msg_debug( "Bad intensity, ID: %s", e.get_id().c_str() );
                e.set_intensity( 1 );
            } else if( e.get_intensity() > e.get_max_intensity() ) {
                e.set_intensity( e.get_max_intensity() );
            }
            if( e.get_intensity() != prev_int ) {
                on_effect_int_change( eff_id, e.get_intensity(), bp );
            }
        }
    }

    if( !found ) {
        // If we don't already have it then add a new one

        // Then check if the effect is blocked by another
        for( auto &elem : *effects ) {
            for( auto &_effect_it : elem.second ) {
                for( const auto &blocked_effect : _effect_it.second.get_blocks_effects() ) {
                    if( blocked_effect == eff_id ) {
                        // The effect is blocked by another, return
                        return;
                    }
                }
            }
        }

        // Now we can make the new effect for application
        effect e( effect_source( source ), &type, dur, bp.id(), permanent, intensity, calendar::turn );
        // Bound to max duration
        if( e.get_max_duration() > 0_turns && e.get_duration() > e.get_max_duration() ) {
            e.set_duration( e.get_max_duration() );
        }

        // Force intensity if it is duration based
        if( e.get_int_dur_factor() != 0_turns ) {
            // + 1 here so that the lowest is intensity 1, not 0
            e.set_intensity( e.get_duration() / e.get_int_dur_factor() + 1 );
        }
        // Bound new effect intensity by [1, max intensity]
        if( e.get_intensity() < 1 ) {
            add_msg_debug( "Bad intensity, ID: %s", e.get_id().c_str() );
            e.set_intensity( 1 );
        } else if( e.get_intensity() > e.get_max_intensity() ) {
            e.set_intensity( e.get_max_intensity() );
        }
        ( *effects )[eff_id][bp] = e;
        if( Character *ch = as_character() ) {
            get_event_bus().send<event_type::character_gains_effect>( ch->getID(), eff_id );
            if( is_player() && !type.get_apply_message().empty() ) {
                add_msg( type.gain_game_message_type(), type.get_apply_message() );
            }
        }
        on_effect_int_change( eff_id, e.get_intensity(), bp );
        // Perform any effect addition effects.
        // only when not deferred
        if( !deferred ) {
            process_one_effect( e, true );
        }
    }
}
void Creature::add_effect( const effect_source &source, const efftype_id &eff_id,
                           const time_duration &dur, bool permanent, int intensity, bool force, bool deferred )
{
    add_effect( source, eff_id, dur, bodypart_str_id::NULL_ID(), permanent, intensity, force,
                deferred );
}
bool Creature::add_env_effect( const efftype_id &eff_id, const bodypart_id &vector, int strength,
                               const time_duration &dur, const bodypart_id &bp, bool permanent, int intensity, bool force )
{
    if( !force && is_immune_effect( eff_id ) ) {
        return false;
    }

    if( dice( strength, 3 ) > dice( get_env_resist( vector ), 3 ) ) {
        // Only add the effect if we fail the resist roll
        // Don't check immunity (force == true), because we did check above
        add_effect( effect_source::empty(), eff_id, dur, bp, permanent, intensity, true );
        return true;
    } else {
        return false;
    }
}
bool Creature::add_env_effect( const efftype_id &eff_id, const bodypart_id &vector, int strength,
                               const time_duration &dur, bool permanent, int intensity, bool force )
{
    return add_env_effect( eff_id, vector, strength, dur, bodypart_str_id::NULL_ID(), permanent,
                           intensity, force );
}
void Creature::clear_effects()
{
    for( auto &elem : *effects ) {
        for( auto &_effect_it : elem.second ) {
            const effect &e = _effect_it.second;
            on_effect_int_change( e.get_id(), 0, e.get_bp() );
        }
    }
    effects->clear();
}
bool Creature::remove_effect( const efftype_id &eff_id, const bodypart_id &bp )
{
    if( !has_effect( eff_id, bp.id() ) ) {
        //Effect doesn't exist, so do nothing
        return false;
    }
    const effect_type &type = eff_id.obj();

    if( Character *ch = as_character() ) {
        if( is_player() ) {
            if( !type.get_remove_message().empty() ) {
                add_msg( type.lose_game_message_type(), type.get_remove_message() );
            }
        }
        get_event_bus().send<event_type::character_loses_effect>( ch->getID(), eff_id );
    }

    // bp_null means remove all of a given effect id
    if( bp == bodypart_str_id::NULL_ID() ) {
        for( auto &it : ( *effects )[eff_id] ) {
            on_effect_int_change( eff_id, 0, it.first );
        }
        effects->erase( eff_id );
    } else {
        ( *effects )[eff_id].erase( bp.id() );
        on_effect_int_change( eff_id, 0, bp );
        // If there are no more effects of a given type remove the type map
        if( ( *effects )[eff_id].empty() ) {
            effects->erase( eff_id );
        }
    }
    return true;
}
bool Creature::remove_effect( const efftype_id &eff_id )
{
    return remove_effect( eff_id, bodypart_str_id::NULL_ID() );
}

bool Creature::has_effect( const efftype_id &eff_id, const bodypart_id &bp ) const
{
    // bp_null means anything targeted or not
    if( bp.id() == bodypart_str_id::NULL_ID() ) {
        return effects->count( eff_id );
    } else {
        auto got_outer = effects->find( eff_id );
        if( got_outer != effects->end() ) {
            auto got_inner = got_outer->second.find( bp.id() );
            if( got_inner != got_outer->second.end() ) {
                return true;
            }
        }
        return false;
    }
}

bool Creature::has_effect( const efftype_id &eff_id ) const
{
    return has_effect( eff_id, bodypart_str_id::NULL_ID() );
}

bool Creature::has_effect_with_flag( const flag_id &flag, const bodypart_id &bp ) const
{
    return std::any_of( effects->begin(), effects->end(), [&]( const auto & elem ) {
        // effect::has_flag currently delegates to effect_type::has_flag
        return elem.first->has_flag( flag ) && elem.second.count( bp );
    } );
}

bool Creature::has_effect_with_flag( const flag_id &flag ) const
{
    return std::any_of( effects->begin(), effects->end(), [&]( const auto & elem ) {
        // effect::has_flag currently delegates to effect_type::has_flag
        return elem.first->has_flag( flag );
    } );
}

std::vector<effect> Creature::get_effects_with_flag( const flag_id &flag ) const
{
    std::vector<effect> effs;
    for( auto &elem : *effects ) {
        if( !elem.first->has_flag( flag ) ) {
            continue;
        }
        for( const std::pair<const bodypart_id, effect> &_it : elem.second ) {
            effs.push_back( _it.second );
        }
    }
    return effs;
}

effect &Creature::get_effect( const efftype_id &eff_id, const bodypart_id &bp )
{
    return const_cast<effect &>( const_cast<const Creature *>( this )->get_effect( eff_id, bp ) );
}

const effect &Creature::get_effect( const efftype_id &eff_id, const bodypart_id &bp ) const
{
    auto got_outer = effects->find( eff_id );
    if( got_outer != effects->end() ) {
        auto got_inner = got_outer->second.find( bp );
        if( got_inner != got_outer->second.end() ) {
            return got_inner->second;
        }
    }
    return effect::null_effect;
}
time_duration Creature::get_effect_dur( const efftype_id &eff_id, const bodypart_id &bp ) const
{
    const effect &eff = get_effect( eff_id, bp );
    if( !eff.is_null() ) {
        return eff.get_duration();
    }

    return 0_turns;
}
int Creature::get_effect_int( const efftype_id &eff_id, const bodypart_id &bp ) const
{
    const effect &eff = get_effect( eff_id, bp );
    if( !eff.is_null() ) {
        return eff.get_intensity();
    }

    return 0;
}
void Creature::process_effects()
{
    // id's and body_part's of all effects to be removed. If we ever get player or
    // monster specific removals these will need to be moved down to that level and then
    // passed in to this function.
    std::vector<efftype_id> rem_ids;
    std::vector<bodypart_id> rem_bps;

    // Decay/removal of effects
    for( auto &elem : *effects ) {
        for( auto &_it : elem.second ) {
            // Add any effects that others remove to the removal list
            for( const auto &removed_effect : _it.second.get_removes_effects() ) {
                rem_ids.push_back( removed_effect );
                rem_bps.push_back( bodypart_str_id::NULL_ID() );
            }
            effect &e = _it.second;
            const int prev_int = e.get_intensity();
            // Run decay effects, marking effects for removal as necessary.
            e.decay( rem_ids, rem_bps, calendar::turn, is_player() );

            if( e.get_intensity() != prev_int && e.get_duration() > 0_turns ) {
                on_effect_int_change( e.get_id(), e.get_intensity(), e.get_bp() );
            }
        }
    }

    // Actually remove effects. This should be the last thing done in process_effects().
    for( size_t i = 0; i < rem_ids.size(); ++i ) {
        remove_effect( rem_ids[i], rem_bps[i] );
    }
}

bool Creature::resists_effect( const effect &e )
{
    for( const efftype_id &i : e.get_resist_effects() ) {
        if( has_effect( i ) ) {
            return true;
        }
    }
    for( const string_id<mutation_branch> &i : e.get_resist_traits() ) {
        if( has_trait( i ) ) {
            return true;
        }
    }
    return false;
}

bool Creature::has_trait( const trait_id &/*flag*/ ) const
{
    return false;
}

// Methods for setting/getting misc key/value pairs.
void Creature::set_value( const std::string &key, const std::string &value )
{
    values[ key ] = value;
}

void Creature::remove_value( const std::string &key )
{
    values.erase( key );
}

std::string Creature::get_value( const std::string &key ) const
{
    auto it = values.find( key );
    return ( it == values.end() ) ? "" : it->second;
}

void Creature::mod_pain( int npain )
{
    mod_pain_noresist( npain );
}

void Creature::mod_pain_noresist( int npain )
{
    set_pain( pain + npain );
}

void Creature::set_pain( int npain )
{
    npain = std::max( npain, 0 );
    if( pain != npain ) {
        pain = npain;
        on_stat_change( "pain", pain );
    }
}

int Creature::get_pain() const
{
    return pain;
}

int Creature::get_perceived_pain() const
{
    return get_pain();
}

std::pair<std::string, nc_color> Creature::get_pain_description() const
{
    float scale = get_perceived_pain() / 10.f;
    std::string pain_string;
    nc_color pain_color = c_yellow;
    if( scale > 7 ) {
        pain_string = _( "Severe pain" );
    } else if( scale > 6 ) {
        pain_string = _( "Intense pain" );
    } else if( scale > 5 ) {
        pain_string = _( "Unmanageable pain" );
    } else if( scale > 4 ) {
        pain_string = _( "Distressing pain" );
    } else if( scale > 3 ) {
        pain_string = _( "Distracting pain" );
    } else if( scale > 2 ) {
        pain_string = _( "Moderate pain" );
    } else if( scale > 1 ) {
        pain_string = _( "Mild pain" );
    } else if( scale > 0 ) {
        pain_string = _( "Minimal pain" );
    } else {
        pain_string = _( "No pain" );
        pain_color = c_white;
    }
    return std::make_pair( pain_string, pain_color );
}

int Creature::get_moves() const
{
    return moves;
}
void Creature::mod_moves( int nmoves )
{
    moves += nmoves;
}
void Creature::set_moves( int nmoves )
{
    moves = nmoves;
}

bool Creature::in_sleep_state() const
{
    return has_effect( effect_sleep ) || has_effect( effect_lying_down ) ||
           has_effect( effect_npc_suspend );
}

/*
 * Killer-related things
 */
Creature *Creature::get_killer() const
{
    return killer;
}

void Creature::set_killer( Creature *const killer )
{
    // Only the first killer will be stored, calling set_killer again with a different
    // killer would mean it's called on a dead creature and therefore ignored.
    if( killer != nullptr && !killer->is_fake() && this->killer == nullptr ) {
        this->killer = killer;
    }
}

int Creature::get_num_blocks() const
{
    return num_blocks + num_blocks_bonus;
}
int Creature::get_num_dodges() const
{
    return num_dodges + num_dodges_bonus;
}
int Creature::get_num_blocks_bonus() const
{
    return num_blocks_bonus;
}
int Creature::get_num_dodges_bonus() const
{
    return num_dodges_bonus;
}
int Creature::get_num_dodges_base() const
{
    return num_dodges;
}

// currently this is expected to be overridden to actually have use
int Creature::get_env_resist( bodypart_id ) const
{
    return 0;
}
int Creature::get_armor_bash( bodypart_id ) const
{
    return armor_bash_bonus;
}
int Creature::get_armor_cut( bodypart_id ) const
{
    return armor_cut_bonus;
}
int Creature::get_armor_bullet( bodypart_id ) const
{
    return armor_bullet_bonus;
}
int Creature::get_armor_bash_base( bodypart_id ) const
{
    return armor_bash_bonus;
}
int Creature::get_armor_cut_base( bodypart_id ) const
{
    return armor_cut_bonus;
}
int Creature::get_armor_bullet_base( bodypart_id ) const
{
    return armor_bullet_bonus;
}
int Creature::get_armor_bash_bonus() const
{
    return armor_bash_bonus;
}
int Creature::get_armor_cut_bonus() const
{
    return armor_cut_bonus;
}
int Creature::get_armor_bullet_bonus() const
{
    return armor_bullet_bonus;
}
int Creature::get_speed() const
{
    return get_speed_base() + get_speed_bonus();
}
float Creature::get_dodge() const
{
    return get_dodge_base() + get_dodge_bonus();
}
float Creature::get_hit() const
{
    return get_hit_base() + get_hit_bonus();
}

anatomy_id Creature::get_anatomy() const
{
    return creature_anatomy;
}

void Creature::set_anatomy( const anatomy_id &anat )
{
    creature_anatomy = anat;
}

const std::map<bodypart_str_id, bodypart> &Creature::get_body() const
{
    return body;
}

void Creature::set_body()
{
    body.clear();
    for( const bodypart_id &bp : get_anatomy()->get_bodyparts() ) {
        body.emplace( bp.id(), bodypart( bp.id() ) );
    }
}

void Creature::calc_all_parts_hp( float hp_mod, float hp_adjustment, int str_max, int dex_max,
                                  int per_max,  int int_max, int healthy_mod,  int fat_to_max_hp )
{
    for( std::pair<const bodypart_str_id, bodypart> &part : body ) {
        int new_max = ( part.first->base_hp + str_max * part.first->hp_mods.str_mod + dex_max *
                        part.first->hp_mods.dex_mod + int_max * part.first->hp_mods.int_mod + per_max *
                        part.first->hp_mods.per_mod + part.first->hp_mods.health_mod * healthy_mod + fat_to_max_hp +
                        hp_adjustment ) * hp_mod;

        if( has_trait( trait_id( "GLASSJAW" ) ) && part.first == body_part_head ) {
            new_max *= 0.8;
        }

        float max_hp_ratio = static_cast<float>( new_max ) /
                             static_cast<float>( part.second.get_hp_max() );

        int new_cur = std::ceil( static_cast<float>( part.second.get_hp_cur() ) * max_hp_ratio );

        part.second.set_hp_max( std::max( new_max, 1 ) );
        part.second.set_hp_cur( std::max( std::min( new_cur, new_max ), 0 ) );
    }
}

bool Creature::has_part( const bodypart_id &id ) const
{
    return body.count( id.id() );
}

bodypart *Creature::get_part( const bodypart_id &id )
{
    auto found = body.find( id.id() );
    if( found == body.end() ) {
        debugmsg( "Could not find bodypart %s in %s's body", id.id().c_str(), get_name() );
        return nullptr;
    }
    return &found->second;
}

const bodypart *Creature::get_part( const bodypart_id &id ) const
{
    auto found = body.find( id.id() );
    if( found == body.end() ) {
        debugmsg( "Could not find bodypart %s in %s's body", id.id().c_str(), get_name() );
        return nullptr;
    }
    return &found->second;
}

template<typename T>
static T get_part_helper( const Creature &c, const bodypart_id &id,
                          T( bodypart::* get )() const )
{
    const bodypart *const part = c.get_part( id );
    if( part ) {
        return ( part->*get )();
    } else {
        // If the bodypart doesn't exist, return the appropriate accessor on the null bodypart.
        // Static null bodypart variable, otherwise the compiler notes the possible return of local variable address (#42798).
        static const bodypart bp_null;
        return ( bp_null.*get )();
    }
}

namespace
{
template<typename T>
class type_identity
{
    public:
        using type = T;
};
} // namespace

template<typename T>
static void set_part_helper( Creature &c, const bodypart_id &id,
                             void( bodypart::* set )( T ), typename type_identity<T>::type val )
{
    bodypart *const part = c.get_part( id );
    if( part ) {
        ( part->*set )( val );
    }
}

int Creature::get_part_hp_cur( const bodypart_id &id ) const
{
    return get_part_helper( *this, id, &bodypart::get_hp_cur );
}

int Creature::get_part_hp_max( const bodypart_id &id ) const
{
    return get_part_helper( *this, id, &bodypart::get_hp_max );
}

int Creature::get_part_healed_total( const bodypart_id &id ) const
{
    return get_part_helper( *this, id, &bodypart::get_healed_total );
}

int Creature::get_part_damage_disinfected( const bodypart_id &id ) const
{
    return get_part_helper( *this, id, &bodypart::get_damage_disinfected );
}

int Creature::get_part_damage_bandaged( const bodypart_id &id ) const
{
    return get_part_helper( *this, id, &bodypart::get_damage_bandaged );
}

const encumbrance_data &Creature::get_part_encumbrance_data( const bodypart_id &id ) const
{
    return get_part_helper( *this, id, &bodypart::get_encumbrance_data );
}

int Creature::get_part_drench_capacity( const bodypart_id &id ) const
{
    return get_part_helper( *this, id, &bodypart::get_drench_capacity );
}

int Creature::get_part_wetness( const bodypart_id &id ) const
{
    return get_part_helper( *this, id, &bodypart::get_wetness );
}

int Creature::get_part_temp_cur( const bodypart_id &id ) const
{
    return get_part_helper( *this, id, &bodypart::get_temp_cur );
}

int Creature::get_part_temp_conv( const bodypart_id &id ) const
{
    return get_part_helper( *this, id, &bodypart::get_temp_conv );
}

int Creature::get_part_frostbite_timer( const bodypart_id &id ) const
{
    return get_part_helper( *this, id, &bodypart::get_frostbite_timer );
}

std::array<int, NUM_WATER_TOLERANCE> Creature::get_part_mut_drench( const bodypart_id &id ) const
{
    return get_part_helper( *this, id, &bodypart::get_mut_drench );
}

float Creature::get_part_wetness_percentage( const bodypart_id &id ) const
{
    return get_part_helper( *this, id, &bodypart::get_wetness_percentage );
}

void Creature::set_part_hp_cur( const bodypart_id &id, int set )
{
    set_part_helper( *this, id, &bodypart::set_hp_cur, set );
    if( is_avatar() && as_character()->is_limb_broken( id ) ) {
        get_event_bus().send<event_type::broken_bone>( as_character()->getID(), id );
    }
}

void Creature::set_part_hp_max( const bodypart_id &id, int set )
{
    set_part_helper( *this, id, &bodypart::set_hp_max, set );
}

void Creature::set_part_healed_total( const bodypart_id &id, int set )
{
    set_part_helper( *this, id, &bodypart::set_healed_total, set );
}

void Creature::set_part_damage_disinfected( const bodypart_id &id, int set )
{
    set_part_helper( *this, id, &bodypart::set_damage_disinfected, set );
}

void Creature::set_part_damage_bandaged( const bodypart_id &id, int set )
{
    set_part_helper( *this, id, &bodypart::set_damage_bandaged, set );
}

void Creature::set_part_encumbrance_data( const bodypart_id &id, const encumbrance_data &set )
{
    set_part_helper( *this, id, &bodypart::set_encumbrance_data, set );
}

void Creature::set_part_wetness( const bodypart_id &id, int set )
{
    set_part_helper( *this, id, &bodypart::set_wetness, set );
}

void Creature::set_part_temp_cur( const bodypart_id &id, int set )
{
    set_part_helper( *this, id, &bodypart::set_temp_cur, set );
}

void Creature::set_part_temp_conv( const bodypart_id &id, int set )
{
    set_part_helper( *this, id, &bodypart::set_temp_conv, set );
}

void Creature::set_part_frostbite_timer( const bodypart_id &id, int set )
{
    set_part_helper( *this, id, &bodypart::set_frostbite_timer, set );
}

void Creature::set_part_mut_drench( const bodypart_id &id, std::pair<water_tolerance, int> set )
{
    set_part_helper( *this, id, &bodypart::set_mut_drench, set );
}

void Creature::mod_part_hp_cur( const bodypart_id &id, int mod )
{
    set_part_helper( *this, id, &bodypart::mod_hp_cur, mod );
    if( is_avatar() && as_character()->is_limb_broken( id ) ) {
        get_event_bus().send<event_type::broken_bone>( as_character()->getID(), id );
    }
}

void Creature::mod_part_hp_max( const bodypart_id &id, int mod )
{
    set_part_helper( *this, id, &bodypart::mod_hp_max, mod );
}

void Creature::mod_part_healed_total( const bodypart_id &id, int mod )
{
    set_part_helper( *this, id, &bodypart::mod_healed_total, mod );
}

void Creature::mod_part_damage_disinfected( const bodypart_id &id, int mod )
{
    set_part_helper( *this, id, &bodypart::mod_damage_disinfected, mod );
}

void Creature::mod_part_damage_bandaged( const bodypart_id &id, int mod )
{
    set_part_helper( *this, id, &bodypart::mod_damage_bandaged, mod );
}

void Creature::mod_part_wetness( const bodypart_id &id, int mod )
{
    set_part_helper( *this, id, &bodypart::mod_wetness, mod );
}

void Creature::mod_part_temp_cur( const bodypart_id &id, int mod )
{
    set_part_helper( *this, id, &bodypart::mod_temp_cur, mod );
}

void Creature::mod_part_temp_conv( const bodypart_id &id, int mod )
{
    set_part_helper( *this, id, &bodypart::mod_temp_conv, mod );
}

void Creature::mod_part_frostbite_timer( const bodypart_id &id, int mod )
{
    set_part_helper( *this, id, &bodypart::mod_frostbite_timer, mod );
}

void Creature::set_all_parts_temp_cur( int set )
{
    for( std::pair<const bodypart_str_id, bodypart> &elem : body ) {
        if( elem.first->has_flag( "IGNORE_TEMP" ) ) {
            continue;
        }
        elem.second.set_temp_cur( set );
    }
}

void Creature::set_all_parts_temp_conv( int set )
{
    for( std::pair<const bodypart_str_id, bodypart> &elem : body ) {
        if( elem.first->has_flag( "IGNORE_TEMP" ) ) {
            continue;
        }
        elem.second.set_temp_conv( set );
    }
}

void Creature::set_all_parts_wetness( int set )
{
    for( std::pair<const bodypart_str_id, bodypart> &elem : body ) {
        elem.second.set_wetness( set );
    }
}

void Creature::set_all_parts_hp_cur( const int set )
{
    for( std::pair<const bodypart_str_id, bodypart> &elem : body ) {
        elem.second.set_hp_cur( set );
    }
}

void Creature::set_all_parts_hp_to_max()
{
    for( std::pair<const bodypart_str_id, bodypart> &elem : body ) {
        elem.second.set_hp_to_max();
    }
}

bool Creature::has_atleast_one_wet_part() const
{
    for( const std::pair<const bodypart_str_id, bodypart> &elem : body ) {
        if( elem.second.get_wetness() > 0 ) {
            return true;
        }
    }
    return false;
}

bool Creature::is_part_at_max_hp( const bodypart_id &id ) const
{
    return get_part_helper( *this, id, &bodypart::is_at_max_hp );
}

bodypart_id Creature::get_random_body_part( bool main ) const
{
    // TODO: Refuse broken limbs, adjust for mutations
    const bodypart_id &part = get_anatomy()->random_body_part();
    return main ? part->main_part.id() : part;
}

static void sort_body_parts( std::vector<bodypart_id> &bps )
{
    // We want to dynamically sort the parts based on their connections as
    // defined in json.
    // The goal is to performa a pre-order depth-first traversal starting
    // with the root part (the head) and prioritising children with fewest
    // descendants.

    // First build a map with the reverse connections
    std::unordered_map<bodypart_id, cata::flat_set<bodypart_id>> parts_connected_to;
    bodypart_id root_part;
    for( const bodypart_id &bp : bps ) {
        bodypart_id conn = bp->connected_to;
        if( conn == bp ) {
            root_part = bp;
        } else {
            parts_connected_to[conn].insert( bp );
        }
    }

    if( root_part == bodypart_id() ) {
        debugmsg( "No root part in body" );
        return;
    }

    // Topo-sort the parts from the extremities towards the head
    std::unordered_map<bodypart_id, cata::flat_set<bodypart_id>> unaccounted_parts =
                parts_connected_to;
    cata::flat_set<bodypart_id> parts_with_no_connections;

    for( const bodypart_id &bp : bps ) {
        if( unaccounted_parts[bp].empty() ) {
            parts_with_no_connections.insert( bp );
        }
    }

    std::vector<bodypart_id> topo_sorted_parts;
    while( !parts_with_no_connections.empty() ) {
        auto last = parts_with_no_connections.end();
        --last;

        bodypart_id bp = *last;
        parts_with_no_connections.erase( last );
        unaccounted_parts.erase( bp );
        topo_sorted_parts.push_back( bp );
        bodypart_id conn = bp->connected_to;
        if( conn == bp ) {
            break;
        }
        auto conn_it = unaccounted_parts.find( conn );
        cata_assert( conn_it != unaccounted_parts.end() );
        conn_it->second.erase( bp );
        if( conn_it->second.empty() ) {
            parts_with_no_connections.insert( conn );
        }
    }

    if( !unaccounted_parts.empty() || !parts_with_no_connections.empty() ) {
        debugmsg( "Error in topo-sorting bodyparts: unaccounted_parts.size() == %d; "
                  "parts_with_no_connections.size() == %d", unaccounted_parts.size(),
                  parts_with_no_connections.size() );
        return;
    }

    // Using the topo-sorted parts, we can count the descendants of each
    // part
    std::unordered_map<bodypart_id, int> num_descendants;
    for( const bodypart_id &bp : topo_sorted_parts ) {
        int this_num_descendants = 1;
        for( const bodypart_id &child : parts_connected_to[bp] ) {
            this_num_descendants += num_descendants[child];
        }
        num_descendants[bp] = this_num_descendants;
    }

    // Finally, we can do the depth-first traversal:
    std::vector<bodypart_id> result;
    std::stack<bodypart_id> pending;
    pending.push( root_part );

    const auto compare_children = [&]( const bodypart_id & l, const bodypart_id & r ) {
        std::string l_name = l->name.translated();
        std::string r_name = r->name.translated();
        bodypart_id l_opp = l->opposite_part;
        bodypart_id r_opp = r->opposite_part;
        // Sorting first on the min of the name and opposite's name ensures
        // that we put pairs together in the list.
        std::string l_min_name = std::min( l_name, l_opp->name.translated(), localized_compare );
        std::string r_min_name = std::min( r_name, r_opp->name.translated(), localized_compare );
        // We delibarately reverse the comparison because the elements get
        // reversed below when they are transferred from the vector to the
        // stack.
        return localized_compare(
                   std::make_tuple( num_descendants[r], r_min_name, r_name ),
                   std::make_tuple( num_descendants[l], l_min_name, l_name ) );
    };

    while( !pending.empty() ) {
        bodypart_id next = pending.top();
        pending.pop();
        result.push_back( next );

        const cata::flat_set<bodypart_id> children_set = parts_connected_to.at( next );
        std::vector<bodypart_id> children( children_set.begin(), children_set.end() );
        std::sort( children.begin(), children.end(), compare_children );
        for( const bodypart_id &child : children ) {
            pending.push( child );
        }
    }

    cata_assert( bps.size() == result.size() );
    bps = result;
}

std::vector<bodypart_id> Creature::get_all_body_parts( get_body_part_flags flags ) const
{
    bool only_main( flags & get_body_part_flags::only_main );

    std::vector<bodypart_id> all_bps;
    all_bps.reserve( body.size() );
    for( const std::pair<const bodypart_str_id, bodypart> &elem : body ) {
        if( only_main && elem.first->main_part != elem.first ) {
            continue;
        }
        all_bps.push_back( elem.first );
    }

    if( flags & get_body_part_flags::sorted ) {
        sort_body_parts( all_bps );
    }

    return  all_bps;
}

int Creature::get_hp( const bodypart_id &bp ) const
{
    if( bp != bodypart_str_id::NULL_ID() ) {
        return get_part_hp_cur( bp );
    }
    int hp_total = 0;
    for( const std::pair<const bodypart_str_id, bodypart> &elem : get_body() ) {
        hp_total += elem.second.get_hp_cur();
    }
    return hp_total;
}

int Creature::get_hp() const
{
    return get_hp( bodypart_str_id::NULL_ID() );
}

int Creature::get_hp_max( const bodypart_id &bp ) const
{
    if( bp != bodypart_str_id::NULL_ID() ) {
        return get_part_hp_max( bp );
    }
    int hp_total = 0;
    for( const std::pair<const bodypart_str_id, bodypart> &elem : get_body() ) {
        hp_total += elem.second.get_hp_max();
    }
    return hp_total;
}

int Creature::get_hp_max() const
{
    return get_hp_max( bodypart_str_id::NULL_ID() );
}

int Creature::get_speed_base() const
{
    return speed_base;
}
int Creature::get_speed_bonus() const
{
    return speed_bonus;
}
float Creature::get_dodge_bonus() const
{
    return dodge_bonus;
}
int Creature::get_block_bonus() const
{
    return block_bonus; //base is 0
}
float Creature::get_hit_bonus() const
{
    return hit_bonus; //base is 0
}
int Creature::get_bash_bonus() const
{
    return bash_bonus;
}
int Creature::get_cut_bonus() const
{
    return cut_bonus;
}

float Creature::get_bash_mult() const
{
    return bash_mult;
}
float Creature::get_cut_mult() const
{
    return cut_mult;
}

bool Creature::get_melee_quiet() const
{
    return melee_quiet;
}

int Creature::get_throw_resist() const
{
    return throw_resist;
}

void Creature::mod_stat( const std::string &stat, float modifier )
{
    if( stat == "speed" ) {
        mod_speed_bonus( modifier );
    } else if( stat == "dodge" ) {
        mod_dodge_bonus( modifier );
    } else if( stat == "block" ) {
        mod_block_bonus( modifier );
    } else if( stat == "hit" ) {
        mod_hit_bonus( modifier );
    } else if( stat == "bash" ) {
        mod_bash_bonus( modifier );
    } else if( stat == "cut" ) {
        mod_cut_bonus( modifier );
    } else if( stat == "pain" ) {
        mod_pain( modifier );
    } else if( stat == "moves" ) {
        mod_moves( modifier );
    } else {
        add_msg( "Tried to modify a nonexistent stat %s.", stat.c_str() );
    }
}

void Creature::set_num_blocks_bonus( int nblocks )
{
    num_blocks_bonus = nblocks;
}
void Creature::mod_num_dodges_bonus( int ndodges )
{
    num_dodges_bonus += ndodges;
}

void Creature::set_armor_bash_bonus( int nbasharm )
{
    armor_bash_bonus = nbasharm;
}
void Creature::set_armor_cut_bonus( int ncutarm )
{
    armor_cut_bonus = ncutarm;
}
void Creature::set_armor_bullet_bonus( int nbulletarm )
{
    armor_bullet_bonus = nbulletarm;
}

void Creature::set_speed_base( int nspeed )
{
    speed_base = nspeed;
}
void Creature::set_speed_bonus( int nspeed )
{
    speed_bonus = nspeed;
}
void Creature::set_dodge_bonus( float ndodge )
{
    dodge_bonus = ndodge;
}

void Creature::set_block_bonus( int nblock )
{
    block_bonus = nblock;
}
void Creature::set_hit_bonus( float nhit )
{
    hit_bonus = nhit;
}
void Creature::set_bash_bonus( int nbash )
{
    bash_bonus = nbash;
}
void Creature::set_cut_bonus( int ncut )
{
    cut_bonus = ncut;
}
void Creature::mod_speed_bonus( int nspeed )
{
    speed_bonus += nspeed;
}
void Creature::mod_dodge_bonus( float ndodge )
{
    dodge_bonus += ndodge;
}
void Creature::mod_block_bonus( int nblock )
{
    block_bonus += nblock;
}
void Creature::mod_hit_bonus( float nhit )
{
    hit_bonus += nhit;
}
void Creature::mod_bash_bonus( int nbash )
{
    bash_bonus += nbash;
}
void Creature::mod_cut_bonus( int ncut )
{
    cut_bonus += ncut;
}
void Creature::mod_size_bonus( int nsize )
{
    size_bonus += nsize;
}

void Creature::set_bash_mult( float nbashmult )
{
    bash_mult = nbashmult;
}
void Creature::set_cut_mult( float ncutmult )
{
    cut_mult = ncutmult;
}

void Creature::set_melee_quiet( bool nquiet )
{
    melee_quiet = nquiet;
}
void Creature::set_throw_resist( int nthrowres )
{
    throw_resist = nthrowres;
}

units::mass Creature::weight_capacity() const
{
    units::mass base_carry = 13_kilogram;
    switch( get_size() ) {
        case creature_size::tiny:
            base_carry /= 4;
            break;
        case creature_size::small:
            base_carry /= 2;
            break;
        case creature_size::medium:
        default:
            break;
        case creature_size::large:
            base_carry *= 2;
            break;
        case creature_size::huge:
            base_carry *= 4;
            break;
    }

    return base_carry;
}

/*
 * Drawing-related functions
 */
void Creature::draw( const catacurses::window &w, const point &origin, bool inverted ) const
{
    draw( w, tripoint( origin, posz() ), inverted );
}

void Creature::draw( const catacurses::window &w, const tripoint &origin, bool inverted ) const
{
    if( is_draw_tiles_mode() ) {
        return;
    }

    point draw( -origin.xy() + point( getmaxx( w ) / 2 + posx(), getmaxy( w ) / 2 + posy() ) );
    if( inverted ) {
        mvwputch_inv( w, draw, basic_symbol_color(), symbol() );
    } else if( is_symbol_highlighted() ) {
        mvwputch_hi( w, draw, basic_symbol_color(), symbol() );
    } else {
        mvwputch( w, draw, symbol_color(), symbol() );
    }
}

bool Creature::is_symbol_highlighted() const
{
    return false;
}

bodypart_id Creature::select_body_part( Creature *source, int hit_roll ) const
{
    int szdif = source->get_size() - get_size();

    add_msg_debug( "hit roll = %d", hit_roll );
    add_msg_debug( "source size = %d", source->get_size() );
    add_msg_debug( "target size = %d", get_size() );
    add_msg_debug( "difference = %d", szdif );

    return anatomy_human_anatomy->select_body_part( szdif, hit_roll );
}

bodypart_id Creature::random_body_part( bool main_parts_only ) const
{
    const bodypart_id &bp = get_anatomy()->random_body_part();
    return  main_parts_only ? bp->main_part : bp ;
}

void Creature::add_damage_over_time( const damage_over_time_data &DoT )
{
    damage_over_time_map.emplace_back( DoT );
}

void Creature::process_damage_over_time()
{
    for( auto DoT = damage_over_time_map.begin(); DoT != damage_over_time_map.end(); ) {
        if( DoT->duration > 0_turns ) {
            for( const bodypart_str_id &bp : DoT->bps ) {
                const int dmg_amount = DoT->amount;
                if( dmg_amount < 0 ) {
                    heal_bp( bp.id(), -dmg_amount );
                } else if( dmg_amount > 0 ) {
                    deal_damage( nullptr, bp.id(), damage_instance( DoT->type, dmg_amount ) );
                }
            }
            DoT->duration -= 1_turns;
            ++DoT;
        } else {
            damage_over_time_map.erase( DoT );
        }
    }
}

void Creature::check_dead_state()
{
    if( is_dead_state() ) {
        die( nullptr );
    }
}

std::string Creature::attitude_raw_string( Attitude att )
{
    switch( att ) {
        case Attitude::HOSTILE:
            return "hostile";
        case Attitude::NEUTRAL:
            return "neutral";
        case Attitude::FRIENDLY:
            return "friendly";
        default:
            return "other";
    }
}

const std::pair<translation, nc_color> &Creature::get_attitude_ui_data( Attitude att )
{
    using pair_t = std::pair<translation, nc_color>;
    static const std::array<pair_t, 5> strings {
        {
            pair_t {to_translation( "Hostile" ), c_red},
            pair_t {to_translation( "Neutral" ), h_white},
            pair_t {to_translation( "Friendly" ), c_green},
            pair_t {to_translation( "Any" ), c_yellow},
            pair_t {to_translation( "BUG: Behavior unnamed.  (Creature::get_attitude_ui_data)" ), h_red}
        }
    };

    if( static_cast<int>( att ) < 0 || static_cast<int>( att ) >= static_cast<int>( strings.size() ) ) {
        return strings.back();
    }

    return strings[static_cast<int>( att )];
}

std::string Creature::replace_with_npc_name( std::string input ) const
{
    replace_substring( input, "<npcname>", disp_name(), true );
    return input;
}

void Creature::knock_back_from( const tripoint &p )
{
    if( p == pos() ) {
        return; // No effect
    }
    if( is_hallucination() ) {
        die( nullptr );
        return;
    }
    tripoint to = pos();
    if( p.x < posx() ) {
        to.x++;
    }
    if( p.x > posx() ) {
        to.x--;
    }
    if( p.y < posy() ) {
        to.y++;
    }
    if( p.y > posy() ) {
        to.y--;
    }

    knock_back_to( to );
}

void Creature::add_msg_if_player( const translation &msg ) const
{
    return add_msg_if_player( msg.translated() );
}

void Creature::add_msg_if_player( const game_message_params &params, const translation &msg ) const
{
    return add_msg_if_player( params, msg.translated() );
}

void Creature::add_msg_if_npc( const translation &msg ) const
{
    return add_msg_if_npc( msg.translated() );
}

void Creature::add_msg_if_npc( const game_message_params &params, const translation &msg ) const
{
    return add_msg_if_npc( params, msg.translated() );
}

void Creature::add_msg_player_or_npc( const translation &pc, const translation &npc ) const
{
    return add_msg_player_or_npc( pc.translated(), npc.translated() );
}

void Creature::add_msg_player_or_npc( const game_message_params &params, const translation &pc,
                                      const translation &npc ) const
{
    return add_msg_player_or_npc( params, pc.translated(), npc.translated() );
}

void Creature::add_msg_player_or_say( const translation &pc, const translation &npc ) const
{
    return add_msg_player_or_say( pc.translated(), npc.translated() );
}

void Creature::add_msg_player_or_say( const game_message_params &params, const translation &pc,
                                      const translation &npc ) const
{
    return add_msg_player_or_say( params, pc.translated(), npc.translated() );
}

std::vector <int> Creature::dispersion_for_even_chance_of_good_hit = { {
        1731, 859, 573, 421, 341, 286, 245, 214, 191, 175,
        151, 143, 129, 118, 114, 107, 101, 94, 90, 78,
        78, 78, 74, 71, 68, 66, 62, 61, 59, 57,
        46, 46, 46, 46, 46, 46, 45, 45, 44, 42,
        41, 41, 39, 39, 38, 37, 36, 35, 34, 34,
        33, 33, 32, 30, 30, 30, 30, 29, 28
    }
};

void Creature::load_hit_range( const JsonObject &jo )
{
    if( jo.has_array( "even_good" ) ) {
        jo.read( "even_good", dispersion_for_even_chance_of_good_hit );
    }
}

void Creature::describe_infrared( std::vector<std::string> &buf ) const
{
    std::string size_str;
    switch( get_size() ) {
        case creature_size::tiny:
            size_str = pgettext( "infrared size", "tiny" );
            break;
        case creature_size::small:
            size_str = pgettext( "infrared size", "small" );
            break;
        case creature_size::medium:
            size_str = pgettext( "infrared size", "medium" );
            break;
        case creature_size::large:
            size_str = pgettext( "infrared size", "large" );
            break;
        case creature_size::huge:
            size_str = pgettext( "infrared size", "huge" );
            break;
        case creature_size::num_sizes:
            debugmsg( "Creature has invalid size class." );
            size_str = "invalid";
            break;
    }
    buf.push_back( _( "You see a figure radiating heat." ) );
    buf.push_back( string_format( _( "It is %s in size." ), size_str ) );
}

void Creature::describe_specials( std::vector<std::string> &buf ) const
{
    buf.push_back( _( "You sense a creature here." ) );
}
