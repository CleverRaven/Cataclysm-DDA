// Monster movement code; essentially, the AI

#include "monster.h"
#include "map.h"
#include "map_iterator.h"
#include "debug.h"
#include "game.h"
#include "output.h"
#include "line.h"
#include "rng.h"
#include "pldata.h"
#include "messages.h"
#include "cursesdef.h"
#include "trap.h"
#include "sounds.h"
#include "monattack.h"
#include "monfaction.h"
#include "translations.h"
#include "npc.h"
#include "mapdata.h"
#include "mtype.h"
#include "field.h"
#include "scent_map.h"

#include <stdlib.h>
//Used for e^(x) functions
#include <stdio.h>
#include <math.h>

#define MONSTER_FOLLOW_DIST 8

const species_id FUNGUS( "FUNGUS" );

const efftype_id effect_bouldering( "bouldering" );
const efftype_id effect_docile( "docile" );
const efftype_id effect_downed( "downed" );
const efftype_id effect_pacified( "pacified" );
const efftype_id effect_pushed( "pushed" );
const efftype_id effect_stunned( "stunned" );

bool monster::wander()
{
    return ( goal == pos() );
}

bool monster::is_immune_field( const field_id fid ) const
{
    switch( fid ) {
        case fd_smoke:
        case fd_tear_gas:
        case fd_toxic_gas:
        case fd_relax_gas:
        case fd_nuke_gas:
            return has_flag( MF_NO_BREATHE );
        case fd_acid:
            return has_flag( MF_ACIDPROOF ) || has_flag( MF_FLIES );
        case fd_fire:
            return has_flag( MF_FIREPROOF );
        case fd_electricity:
            return has_flag( MF_ELECTRIC );
        case fd_fungal_haze:
            return has_flag( MF_NO_BREATHE ) || type->in_species( FUNGUS );
        case fd_fungicidal_gas:
            return !type->in_species( FUNGUS );
        default:
            // Suppress warning
            break;
    }
    // No specific immunity was found, so fall upwards
    return Creature::is_immune_field( fid );
}

bool monster::can_move_to( const tripoint &p ) const
{
    const bool can_climb = has_flag( MF_CLIMBS ) || has_flag( MF_FLIES );
    if( g->m.impassable( p ) && !( can_climb && g->m.has_flag( "CLIMBABLE", p ) ) ) {
        return false;
    }

    if( !can_submerge() && g->m.has_flag( TFLAG_DEEP_WATER, p ) ) {
        return false;
    }
    if( has_flag( MF_DIGS ) && !g->m.has_flag( "DIGGABLE", p ) ) {
        return false;
    }
    if( has_flag( MF_AQUATIC ) && !g->m.has_flag( "SWIMMABLE", p ) ) {
        return false;
    }

    if( has_flag( MF_SUNDEATH ) && g->is_in_sunlight( p ) ) {
        return false;
    }

    // Various avoiding behaviors
    if( has_flag( MF_AVOID_DANGER_1 ) || has_flag( MF_AVOID_DANGER_2 ) ) {
        const ter_id target = g->m.ter( p );
        // Don't enter lava ever
        if( target == t_lava ) {
            return false;
        }
        // Don't ever throw ourselves off cliffs
        if( !g->m.has_floor( p ) && !has_flag( MF_FLIES ) ) {
            return false;
        }

        // Don't enter open pits ever unless tiny, can fly or climb well
        if( !( type->size == MS_TINY || can_climb ) &&
            ( target == t_pit || target == t_pit_spiked || target == t_pit_glass ) ) {
            return false;
        }

        // The following behaviors are overridden when attacking
        if( attitude( &( g->u ) ) != MATT_ATTACK ) {
            if( g->m.has_flag( "SHARP", p ) &&
                !( type->size == MS_TINY || has_flag( MF_FLIES ) ) ) {
                return false;
            }
        }

        const field &target_field = g->m.field_at( p );

        // Differently handled behaviors
        if( has_flag( MF_AVOID_DANGER_2 ) ) {
            const trap &target_trap = g->m.tr_at( p );
            // Don't enter any dangerous fields
            if( is_dangerous_fields( target_field ) ) {
                return false;
            }
            // Don't step on any traps (if we can see)
            if( has_flag( MF_SEES ) && !target_trap.is_benign() && g->m.has_floor( p ) ) {
                return false;
            }
        } else if( has_flag( MF_AVOID_DANGER_1 ) ) {
            // Don't enter fire or electricity ever (other dangerous fields are fine though)
            if( target_field.findField( fd_fire ) || target_field.findField( fd_electricity ) ) {
                return false;
            }
        }
    }

    return true;
}

void monster::set_dest( const tripoint &p )
{
    goal = p;
}

void monster::unset_dest()
{
    goal = pos();
    path.clear();
}

// Move towards p for f more turns--generally if we hear a sound there
// "Stupid" movement; "if (wander_pos.x < posx) posx--;" etc.
void monster::wander_to( const tripoint &p, int f )
{
    wander_pos = p;
    wandf = f;
}

float monster::rate_target( Creature &c, float best, bool smart ) const
{
    const int d = rl_dist( pos(), c.pos() );
    if( d <= 0 ) {
        return INT_MAX;
    }

    // Check a very common and cheap case first
    if( !smart && d >= best ) {
        return INT_MAX;
    }

    if( !sees( c ) ) {
        return INT_MAX;
    }

    if( !smart ) {
        return d;
    }

    float power = c.power_rating();
    monster *mon = dynamic_cast< monster * >( &c );
    // Their attitude to us and not ours to them, so that bobcats won't get gunned down
    if( mon != nullptr && mon->attitude_to( *this ) == Attitude::A_HOSTILE ) {
        power += 2;
    }

    if( power > 0 ) {
        return d / power;
    }

    return INT_MAX;
}

void monster::plan( const mfactions &factions )
{
    // Bots are more intelligent than most living stuff
    bool smart_planning = has_flag( MF_PRIORITIZE_TARGETS );
    Creature *target = nullptr;
    // 8.6f is rating for tank drone 60 tiles away, moose 16 or boomer 33
    float dist = !smart_planning ? 1000 : 8.6f;
    bool fleeing = false;
    bool docile = friendly != 0 && has_effect( effect_docile );
    bool angers_hostile_weak = type->anger.find( MTRIG_HOSTILE_WEAK ) != type->anger.end();
    int angers_hostile_near =
        ( type->anger.find( MTRIG_HOSTILE_CLOSE ) != type->anger.end() ) ? 5 : 0;
    int fears_hostile_near = ( type->fear.find( MTRIG_HOSTILE_CLOSE ) != type->fear.end() ) ? 5 : 0;
    bool group_morale = has_flag( MF_GROUP_MORALE ) && morale < type->morale;
    bool swarms = has_flag( MF_SWARMS );
    auto mood = attitude();

    // If we can see the player, move toward them or flee, simpleminded animals are too dumb to follow the player.
    if( friendly == 0 && sees( g->u ) && !has_flag( MF_PET_WONT_FOLLOW ) ) {
        dist = rate_target( g->u, dist, smart_planning );
        fleeing = fleeing || is_fleeing( g->u );
        target = &g->u;
        if( dist <= 5 ) {
            anger += angers_hostile_near;
            morale -= fears_hostile_near;
        }
    } else if( friendly != 0 && !docile ) {
        // Target unfriendly monsters, only if we aren't interacting with the player.
        for( monster &tmp : g->all_monsters() ) {
            if( tmp.friendly == 0 ) {
                float rating = rate_target( tmp, dist, smart_planning );
                if( rating < dist ) {
                    target = &tmp;
                    dist = rating;
                }
            }
        }
    }

    if( docile ) {
        if( friendly != 0 && target != nullptr ) {
            set_dest( target->pos() );
        }

        return;
    }

    for( npc &who : g->all_npcs() ) {
        auto faction_att = faction.obj().attitude( who.get_monster_faction() );
        if( faction_att == MFA_NEUTRAL || faction_att == MFA_FRIENDLY ) {
            continue;
        }

        float rating = rate_target( who, dist, smart_planning );
        bool fleeing_from = is_fleeing( who );
        // Switch targets if closer and hostile or scarier than current target
        if( ( rating < dist && fleeing ) ||
            ( rating < dist && attitude( &who ) == MATT_ATTACK ) ||
            ( !fleeing && fleeing_from ) ) {
            target = &who;
            dist = rating;
        }
        fleeing = fleeing || fleeing_from;
        if( rating <= 5 ) {
            anger += angers_hostile_near;
            morale -= fears_hostile_near;
        }
    }

    fleeing = fleeing || ( mood == MATT_FLEE );
    if( friendly == 0 ) {
        for( const auto &fac : factions ) {
            auto faction_att = faction.obj().attitude( fac.first );
            if( faction_att == MFA_NEUTRAL || faction_att == MFA_FRIENDLY ) {
                continue;
            }

            for( monster *const mon_ptr : fac.second ) {
                monster &mon = *mon_ptr;
                float rating = rate_target( mon, dist, smart_planning );
                if( rating < dist ) {
                    target = &mon;
                    dist = rating;
                }
                if( rating <= 5 ) {
                    anger += angers_hostile_near;
                    morale -= fears_hostile_near;
                }
            }
        }
    }

    // Friendly monsters here
    // Avoid for hordes of same-faction stuff or it could get expensive
    const auto actual_faction = friendly == 0 ? faction : mfaction_str_id( "player" );
    auto const &myfaction_iter = factions.find( actual_faction );
    if( myfaction_iter == factions.end() ) {
        DebugLog( D_ERROR, D_GAME ) << disp_name() << " tried to find faction "
                                    << actual_faction.id().str()
                                    << " which wasn't loaded in game::monmove";
        swarms = false;
        group_morale = false;
    }
    swarms = swarms && target == nullptr; // Only swarm if we have no target
    if( group_morale || swarms ) {
        for( monster *const mon_ptr : myfaction_iter->second ) {
            monster &mon = *mon_ptr;
            float rating = rate_target( mon, dist, smart_planning );
            if( group_morale && rating <= 10 ) {
                morale += 10 - rating;
            }
            if( swarms ) {
                if( rating < 5 ) { // Too crowded here
                    wander_pos.x = posx() * rng( 1, 3 ) - mon.posx();
                    wander_pos.y = posy() * rng( 1, 3 ) - mon.posy();
                    wandf = 2;
                    target = nullptr;
                    // Swarm to the furthest ally you can see
                } else if( rating < INT_MAX && rating > dist && wandf <= 0 ) {
                    target = &mon;
                    dist = rating;
                }
            }
        }
    }

    if( target != nullptr ) {

        tripoint dest = target->pos();
        auto att_to_target = attitude_to( *target );
        if( att_to_target == Attitude::A_HOSTILE && !fleeing ) {
            set_dest( dest );
        } else if( fleeing ) {
            set_dest( tripoint( posx() * 2 - dest.x, posy() * 2 - dest.y, posz() ) );
        }
        if( angers_hostile_weak && att_to_target != Attitude::A_FRIENDLY ) {
            int hp_per = target->hp_percentage();
            if( hp_per <= 70 ) {
                anger += 10 - int( hp_per / 10 );
            }
        }
    } else if( friendly > 0 && one_in( 3 ) ) {
        // Grow restless with no targets
        friendly--;
    } else if( friendly < 0 && sees( g->u ) ) {
        if( rl_dist( pos(), g->u.pos() ) > 2 ) {
            set_dest( g->u.pos() );
        } else {
            unset_dest();
        }
    }
}

/**
 * Method to make monster movement speed consistent in the face of staggering behavior and
 * differing distance metrics.
 * It works by scaling the cost to take a step by
 * how much that step reduces the distance to your goal.
 * Since it incorporates the current distance metric,
 * it also scales for diagonal vs orthogonal movement.
 **/
static float get_stagger_adjust( const tripoint &source, const tripoint &destination,
                                 const tripoint &next_step )
{
    // TODO: push this down into rl_dist
    const float initial_dist =
        trigdist ? trig_dist( source, destination ) : rl_dist( source, destination );
    const float new_dist =
        trigdist ? trig_dist( next_step, destination ) : rl_dist( next_step, destination );
    // If we return 0, it wil cancel the action.
    return std::max( 0.01f, initial_dist - new_dist );
}

// General movement.
// Currently, priority goes:
// 1) Special Attack
// 2) Sight-based tracking
// 3) Scent-based tracking
// 4) Sound-based tracking
void monster::move()
{
    // We decrement wandf no matter what.  We'll save our wander_to plans until
    // after we finish out set_dest plans, UNLESS they time out first.
    if( wandf > 0 ) {
        wandf--;
    }

    //Hallucinations have a chance of disappearing each turn
    if( is_hallucination() && one_in( 25 ) ) {
        die( nullptr );
        return;
    }

    //The monster can consume objects it stands on. Check if there are any.
    //If there are. Consume them.
    if( !is_hallucination() && ( has_flag( MF_ABSORBS ) || has_flag( MF_ABSORBS_SPLITS ) ) &&
        !g->m.has_flag( TFLAG_SEALED, pos() ) && g->m.has_items( pos() ) ) {
        if( g->u.sees( *this ) ) {
            add_msg( _( "The %s flows around the objects on the floor and they are quickly dissolved!" ),
                     name().c_str() );
        }
        static const auto volume_per_hp = units::from_milliliter( 250 );
        for( auto &elem : g->m.i_at( pos() ) ) {
            hp += elem.volume() / volume_per_hp; // Yeah this means it can get more HP than normal.
            if( has_flag( MF_ABSORBS_SPLITS ) && hp * 2 > type->hp ) {
                for( const tripoint &dest : g->m.points_in_radius( pos(), 1 ) ) {
                    if( g->is_empty( dest ) && hp * 2 > type->hp ) {
                        if( monster *const  spawn = g->summon_mon( type->id, dest ) ) {
                            hp -= type->hp;
                            //this is a new copy of the monster. Ideally we should copy the stats/effects that affect the parent
                            spawn->make_ally( *this );
                            if( g->u.sees( *this ) ) {
                                add_msg( _( "The %s splits in two!" ),
                                         name().c_str() );
                            }
                        }
                    }
                }
            }
        }
        g->m.i_clear( pos() );
    }

    const bool pacified = has_effect( effect_pacified );

    // First, use the special attack, if we can!
    // The attack may change `monster::special_attacks` (e.g. by transforming
    // this into another monster type). Therefore we can not iterate over it
    // directly and instead iterate over the map from the monster type
    // (properties of monster types should never change).
    for( const auto &sp_type : type->special_attacks ) {
        const std::string &special_name = sp_type.first;
        const auto local_iter = special_attacks.find( special_name );
        if( local_iter == special_attacks.end() ) {
            continue;
        }
        mon_special_attack &local_attack_data = local_iter->second;
        if( !local_attack_data.enabled ) {
            continue;
        }

        // Cooldowns are decremented in monster::process_turn

        if( local_attack_data.cooldown == 0 && !pacified && !is_hallucination() ) {
            if( !sp_type.second->call( *this ) ) {
                continue;
            }

            // `special_attacks` might have changed at this point. Sadly `reset_special`
            // doesn't check the attack name, so we need to do it here.
            if( special_attacks.count( special_name ) == 0 ) {
                continue;
            }
            reset_special( special_name );
        }
    }

    // The monster can sometimes hang in air due to last fall being blocked
    const bool can_fly = has_flag( MF_FLIES );
    if( !can_fly && g->m.has_flag( TFLAG_NO_FLOOR, pos() ) ) {
        g->m.creature_on_trap( *this, false );
    }

    // The monster is in a deep water tile and has a chance to drown
    if( g->m.has_flag_ter( TFLAG_DEEP_WATER, pos() ) ) {
        if( g->m.has_flag( "LIQUID", pos() ) && can_drown() && one_in( 10 ) ) {
            die( nullptr );
            if( g->u.sees( pos() ) ) {
                add_msg( _( "The %s drowns!" ), name().c_str() );
            }
            return;
        }
    }

    if( moves < 0 ) {
        return;
    }

    // TODO: Move this to attack_at/move_to/etc. functions
    bool attacking = false;
    if( !move_effects( attacking ) ) {
        moves = 0;
        return;
    }
    if( has_flag( MF_IMMOBILE ) ) {
        moves = 0;
        return;
    }
    if( has_effect( effect_stunned ) ) {
        stumble();
        moves = 0;
        return;
    }
    if( friendly > 0 ) {
        --friendly;
    }

    // Set attitude to attitude to our current target
    monster_attitude current_attitude = attitude( nullptr );
    if( !wander() ) {
        if( goal == g->u.pos() ) {
            current_attitude = attitude( &( g->u ) );
        } else {
            for( const npc &guy : g->all_npcs() ) {
                if( goal == guy.pos() ) {
                    current_attitude = attitude( &guy );
                }
            }
        }
    }

    if( current_attitude == MATT_IGNORE ||
        ( current_attitude == MATT_FOLLOW && rl_dist( pos(), goal ) <= MONSTER_FOLLOW_DIST ) ) {
        moves -= 100;
        stumble();
        return;
    }

    bool moved = false;
    tripoint destination;

    // If true, don't try to greedily avoid locally bad paths
    bool pathed = false;
    if( !wander() ) {
        while( !path.empty() && path.front() == pos() ) {
            path.erase( path.begin() );
        }

        const auto &pf_settings = get_pathfinding_settings();
        if( pf_settings.max_dist >= rl_dist( pos(), goal ) &&
            ( path.empty() || rl_dist( pos(), path.front() ) >= 2 || path.back() != goal ) ) {
            // We need a new path
            path = g->m.route( pos(), goal, pf_settings, get_path_avoid() );
        }

        // Try to respect old paths, even if we can't pathfind at the moment
        if( !path.empty() && path.back() == goal ) {
            destination = path.front();
            moved = true;
            pathed = true;
        } else {
            // Straight line forward, probably because we can't pathfind (well enough)
            destination = goal;
            moved = true;
        }
    }
    if( !moved && has_flag( MF_SMELLS ) ) {
        // No sight... or our plans are invalid (e.g. moving through a transparent, but
        //  solid, square of terrain).  Fall back to smell if we have it.
        unset_dest();
        tripoint tmp = scent_move();
        if( tmp.x != -1 ) {
            destination = tmp;
            moved = true;
        }
    }
    if( wandf > 0 && !moved && friendly == 0 ) { // No LOS, no scent, so as a fall-back follow sound
        unset_dest();
        if( wander_pos != pos() ) {
            destination = wander_pos;
            moved = true;
        }
    }

    if( !g->m.has_zlevels() ) {
        // Otherwise weird things happen
        destination.z = posz();
    }

    tripoint next_step;
    const bool staggers = has_flag( MF_STUMBLES );
    const bool can_climb = has_flag( MF_CLIMBS );
    if( moved ) {
        // Implement both avoiding obstacles and staggering.
        moved = false;
        float switch_chance = 0.0;
        const bool can_bash = bash_skill() > 0;
        // This is a float and using trig_dist() because that Does the Right Thing(tm)
        // in both circular and roguelike distance modes.
        const float distance_to_target = trig_dist( pos(), destination );
        for( const tripoint &candidate : squares_closer_to( pos(), destination ) ) {
            if( candidate.z != posz() ) {
                bool can_z_move = true;
                if( !g->m.valid_move( pos(), candidate, false, true ) ) {
                    // Can't phase through floor
                    can_z_move = false;
                }

                // If we're trying to go up but can't fly, check if we can climb. If we can't, then don't
                // This prevents non-climb/fly enemies running up walls
                if( candidate.z > posz() && !can_fly ) {
                    if( !can_climb || !g->m.has_floor_or_support( candidate ) ) {
                        // Can't "jump" up a whole z-level
                        can_z_move = false;
                    }
                }

                // Last chance - we can still do the z-level stair teleport bullshit that isn't removed yet
                // @todo: Remove z-level stair bullshit teleport after aligning all stairs
                if( !can_z_move &&
                    posx() / ( SEEX * 2 ) == candidate.x / ( SEEX * 2 ) &&
                    posy() / ( SEEY * 2 ) == candidate.y / ( SEEY * 2 ) ) {
                    const tripoint &upper = candidate.z > posz() ? candidate : pos();
                    const tripoint &lower = candidate.z > posz() ? pos() : candidate;
                    if( g->m.has_flag( TFLAG_GOES_DOWN, upper ) && g->m.has_flag( TFLAG_GOES_UP, lower ) ) {
                        can_z_move = true;
                    }
                }

                if( !can_z_move ) {
                    continue;
                }
            }

            // A flag to allow non-stumbling critters to stumble when the most direct choice is bad.
            bool bad_choice = false;

            const Creature *target = g->critter_at( candidate, is_hallucination() );
            if( target != nullptr ) {
                const Creature::Attitude att = attitude_to( *target );
                if( att == A_HOSTILE ) {
                    // When attacking an adjacent enemy, we're direct.
                    moved = true;
                    next_step = candidate;
                    break;
                } else if( att == A_FRIENDLY && ( target->is_player() || target->is_npc() ) ) {
                    continue; // Friendly firing the player or an NPC is illegal for gameplay reasons
                } else if( !has_flag( MF_ATTACKMON ) && !has_flag( MF_PUSH_MON ) ) {
                    // Bail out if there's a non-hostile monster in the way and we're not pushy.
                    continue;
                }
                // Friendly fire and pushing are always bad choices - they take a lot of time
                bad_choice = true;
            }

            // Bail out if we can't move there and we can't bash.
            if( !pathed && !can_move_to( candidate ) ) {
                if( !can_bash ) {
                    continue;
                }

                const int estimate = g->m.bash_rating( bash_estimate(), candidate );
                if( estimate <= 0 ) {
                    continue;
                }

                if( estimate < 5 ) {
                    bad_choice = true;
                }
            }

            const float progress = distance_to_target - trig_dist( candidate, destination );
            // The x2 makes the first (and most direct) path twice as likely,
            // since the chance of switching is 1/1, 1/4, 1/6, 1/8
            switch_chance += progress * 2;
            // Randomly pick one of the viable squares to move to weighted by distance.
            if( !moved || x_in_y( progress, switch_chance ) ) {
                moved = true;
                next_step = candidate;
                // If we stumble, pick a random square, otherwise take the first one,
                // which is the most direct path.
                // Except if the direct path is bad, then check others
                // Or if the path is given by pathfinder
                if( !staggers && ( !bad_choice || pathed ) ) {
                    break;
                }
            }
        }
    }
    // Finished logic section.  By this point, we should have chosen a square to
    //  move to (moved = true).
    if( moved ) { // Actual effects of moving to the square we've chosen
        const bool did_something =
            ( !pacified && attack_at( next_step ) ) ||
            ( !pacified && bash_at( next_step ) ) ||
            ( !pacified && push_to( next_step, 0, 0 ) ) ||
            move_to( next_step, false, get_stagger_adjust( pos(), destination, next_step ) );

        if( !did_something ) {
            moves -= 100; // If we don't do this, we'll get infinite loops.
        }
    } else {
        moves -= 100;
        stumble();
        path.clear();
    }
}

// footsteps will determine how loud a monster's normal movement is
// and create a sound in the monsters location when they move
void monster::footsteps( const tripoint &p )
{
    if( made_footstep ) {
        return;
    }
    if( has_flag( MF_FLIES ) ) {
        return;    // Flying monsters don't have footsteps!
    }
    made_footstep = true;
    int volume = 6; // same as player's footsteps
    if( digging() ) {
        volume = 10;
    }
    switch( type->size ) {
        case MS_TINY:
            return; // No sound for the tinies
        case MS_SMALL:
            volume /= 3;
            break;
        case MS_MEDIUM:
            break;
        case MS_LARGE:
            volume *= 1.5;
            break;
        case MS_HUGE:
            volume *= 2;
            break;
        default:
            break;
    }
    int dist = rl_dist( p, g->u.pos() );
    sounds::add_footstep( p, volume, dist, this );
    return;
}

tripoint monster::scent_move()
{
    // @todo: Remove when scentmap is 3D
    if( abs( posz() - g->get_levz() ) > 1 ) {
        return { -1, -1, INT_MIN };
    }

    std::vector<tripoint> smoves;

    int bestsmell = 10; // Squares with smell 0 are not eligible targets.
    int smell_threshold = 200; // Squares at or above this level are ineligible.
    if( has_flag( MF_KEENNOSE ) ) {
        bestsmell = 1;
        smell_threshold = 400;
    }

    const bool fleeing = is_fleeing( g->u );
    if( fleeing ) {
        bestsmell = g->scent.get( pos() );
    }

    tripoint next( -1, -1, posz() );
    if( ( !fleeing && g->scent.get( pos() ) > smell_threshold ) ||
        ( fleeing && bestsmell == 0 ) ) {
        return next;
    }
    const bool can_bash = bash_skill() > 0;
    for( const auto &dest : g->m.points_in_radius( pos(), 1, 1 ) ) {
        int smell = g->scent.get( dest );
        if( g->m.valid_move( pos(), dest, can_bash, true ) &&
            ( can_move_to( dest ) || ( dest == g->u.pos() ) ||
              ( can_bash && g->m.bash_rating( bash_estimate(), dest ) > 0 ) ) ) {
            if( ( !fleeing && smell > bestsmell ) || ( fleeing && smell < bestsmell ) ) {
                smoves.clear();
                smoves.push_back( dest );
                bestsmell = smell;
            } else if( ( !fleeing && smell == bestsmell ) || ( fleeing && smell == bestsmell ) ) {
                smoves.push_back( dest );
            }
        }
    }

    return random_entry( smoves, next );
}

int monster::calc_movecost( const tripoint &f, const tripoint &t ) const
{
    int movecost = 0;

    const int source_cost = g->m.move_cost( f );
    const int dest_cost = g->m.move_cost( t );
    // Digging and flying monsters ignore terrain cost
    if( has_flag( MF_FLIES ) || ( digging() && g->m.has_flag( "DIGGABLE", t ) ) ) {
        movecost = 100;
        // Swimming monsters move super fast in water
    } else if( has_flag( MF_SWIMS ) ) {
        if( g->m.has_flag( "SWIMMABLE", f ) ) {
            movecost += 25;
        } else {
            movecost += 50 * g->m.move_cost( f );
        }
        if( g->m.has_flag( "SWIMMABLE", t ) ) {
            movecost += 25;
        } else {
            movecost += 50 * g->m.move_cost( t );
        }
    } else if( can_submerge() ) {
        // No-breathe monsters have to walk underwater slowly
        if( g->m.has_flag( "SWIMMABLE", f ) ) {
            movecost += 250;
        } else {
            movecost += 50 * g->m.move_cost( f );
        }
        if( g->m.has_flag( "SWIMMABLE", t ) ) {
            movecost += 250;
        } else {
            movecost += 50 * g->m.move_cost( t );
        }
        movecost /= 2;
    } else if( has_flag( MF_CLIMBS ) ) {
        if( g->m.has_flag( "CLIMBABLE", f ) ) {
            movecost += 150;
        } else {
            movecost += 50 * g->m.move_cost( f );
        }
        if( g->m.has_flag( "CLIMBABLE", t ) ) {
            movecost += 150;
        } else {
            movecost += 50 * g->m.move_cost( t );
        }
        movecost /= 2;
    } else {
        movecost = ( ( 50 * source_cost ) + ( 50 * dest_cost ) ) / 2.0;
    }

    return movecost;
}

int monster::calc_climb_cost( const tripoint &f, const tripoint &t ) const
{
    if( has_flag( MF_FLIES ) ) {
        return 100;
    }

    if( has_flag( MF_CLIMBS ) && !g->m.has_flag( TFLAG_NO_FLOOR, t ) ) {
        const int diff = g->m.climb_difficulty( f );
        if( diff <= 10 ) {
            return 150;
        }
    }

    return 0;
}

/*
 * Return points of an area extending 1 tile to either side and
 * (maxdepth) tiles behind basher.
 */
std::vector<tripoint> get_bashing_zone( const tripoint &bashee, const tripoint &basher,
                                        int maxdepth )
{
    std::vector<tripoint> direction;
    direction.push_back( bashee );
    direction.push_back( basher );
    // Draw a line from the target through the attacker.
    std::vector<tripoint> path = continue_line( direction, maxdepth );
    // Remove the target.
    path.insert( path.begin(), basher );
    std::vector<tripoint> zone;
    // Go ahead and reserve enough room for all the points since
    // we know how many it will be.
    zone.reserve( 3 * maxdepth );
    tripoint previous = bashee;
    for( const tripoint &p : path ) {
        std::vector<point> swath = squares_in_direction( previous.x, previous.y, p.x, p.y );
        for( point q : swath ) {
            zone.push_back( tripoint( q, bashee.z ) );
        }

        previous = p;
    }
    return zone;
}

bool monster::bash_at( const tripoint &p )
{
    if( p.z != posz() ) {
        return false; // TODO: Remove this
    }

    //Hallucinations can't bash stuff.
    if( is_hallucination() ) {
        return false;
    }
    bool try_bash = !can_move_to( p ) || one_in( 3 );
    bool can_bash = g->m.is_bashable( p ) && bash_skill() > 0;

    if( try_bash && can_bash ) {
        int bashskill = group_bash_skill( p );
        g->m.bash( p, bashskill );
        moves -= 100;
        return true;
    }
    return false;
}

int monster::bash_estimate()
{
    int estimate = bash_skill();
    if( has_flag( MF_GROUP_BASH ) ) {
        // Right now just give them a boost so they try to bash a lot of stuff.
        // TODO: base it on number of nearby friendlies.
        estimate *= 2;
    }
    return estimate;
}

int monster::bash_skill()
{
    return type->bash_skill;
}

int monster::group_bash_skill( const tripoint &target )
{
    if( !has_flag( MF_GROUP_BASH ) ) {
        return bash_skill();
    }
    int bashskill = 0;

    // pileup = more bash skill, but only help bashing mob directly in front of target
    const int max_helper_depth = 5;
    const std::vector<tripoint> bzone = get_bashing_zone( target, pos(), max_helper_depth );

    for( const tripoint &candidate : bzone ) {
        // Drawing this line backwards excludes the target and includes the candidate.
        std::vector<tripoint> path_to_target = line_to( target, candidate, 0, 0 );
        bool connected = true;
        monster *mon = nullptr;
        for( const tripoint &in_path : path_to_target ) {
            // If any point in the line from zombie to target is not a cooperating zombie,
            // it can't contribute.
            mon = g->critter_at<monster>( in_path );
            if( !mon ) {
                connected = false;
                break;
            }
            monster &helpermon = *mon;
            if( !helpermon.has_flag( MF_GROUP_BASH ) || helpermon.is_hallucination() ) {
                connected = false;
                break;
            }
        }
        if( !connected || !mon ) {
            continue;
        }
        // If we made it here, the last monster checked was the candidate.
        monster &helpermon = *mon;
        // Contribution falls off rapidly with distance from target.
        bashskill += helpermon.bash_skill() / rl_dist( candidate, target );
    }

    return bashskill;
}

bool monster::attack_at( const tripoint &p )
{
    if( p.z != posz() ) {
        return false; // TODO: Remove this
    }

    if( p == g->u.pos() ) {
        melee_attack( g->u );
        return true;
    }

    if( const auto mon_ = g->critter_at<monster>( p, is_hallucination() ) ) {
        monster &mon = *mon_;

        // Don't attack yourself.
        if( &mon == this ) {
            return false;
        }

        // With no melee dice, we can't attack, but we had to process until here
        // because hallucinations require no melee dice to destroy.
        if( type->melee_dice <= 0 ) {
            return false;
        }

        auto attitude = attitude_to( mon );
        // MF_ATTACKMON == hulk behavior, whack everything in your way
        if( attitude == A_HOSTILE || has_flag( MF_ATTACKMON ) ) {
            melee_attack( mon );
            return true;
        }

        return false;
    }

    npc *const guy = g->critter_at<npc>( p );
    if( guy && type->melee_dice > 0 ) {
        // For now we're always attacking NPCs that are getting into our
        // way. This is consistent with how it worked previously, but
        // later on not hitting allied NPCs would be cool.
        melee_attack( *guy );
        return true;
    }

    // Nothing to attack.
    return false;
}

bool monster::move_to( const tripoint &p, bool force, const float stagger_adjustment )
{
    const bool digs = digging();
    const bool flies = has_flag( MF_FLIES );
    const bool on_ground = !digs && !flies;
    const bool climbs = has_flag( MF_CLIMBS ) && g->m.has_flag( TFLAG_NO_FLOOR, p );
    // Allows climbing monsters to move on terrain with movecost <= 0
    Creature *critter = g->critter_at( p, is_hallucination() );
    if( g->m.has_flag( "CLIMBABLE", p ) ) {
        if( g->m.impassable( p ) && critter == nullptr ) {
            if( flies ) {
                moves -= 100;
                force = true;
                if( g->u.sees( *this ) ) {
                    add_msg( _( "The %1$s flies over the %2$s." ), name().c_str(),
                             g->m.has_flag_furn( "CLIMBABLE", p ) ? g->m.furnname( p ).c_str() :
                             g->m.tername( p ).c_str() );
                }
            } else if( has_flag( MF_CLIMBS ) ) {
                moves -= 150;
                force = true;
                if( g->u.sees( *this ) ) {
                    add_msg( _( "The %1$s climbs over the %2$s." ), name().c_str(),
                             g->m.has_flag_furn( "CLIMBABLE", p ) ? g->m.furnname( p ).c_str() :
                             g->m.tername( p ).c_str() );
                }
            }
        }
    }

    if( critter != nullptr && !force ) {
        return false;
    }

    // Make sure that we can move there, unless force is true.
    if( !force && !can_move_to( p ) ) {
        return false;
    }

    if( !force ) {
        // This adjustment is to make it so that monster movement speed relative to the player
        // is consistent even if the monster stumbles,
        // and the same regardless of the distance measurement mode.
        // Note: Keep this as float here or else it will cancel valid moves
        const float cost = stagger_adjustment *
                           ( float )( climbs ? calc_climb_cost( pos(), p ) :
                                      calc_movecost( pos(), p ) );
        if( cost > 0.0f ) {
            moves -= ( int )ceil( cost );
        } else {
            return false;
        }
    }

    //Check for moving into/out of water
    bool was_water = g->m.is_divable( pos() );
    bool will_be_water = on_ground && can_submerge() && g->m.is_divable( p );

    if( was_water && !will_be_water && g->u.sees( p ) ) {
        //Use more dramatic messages for swimming monsters
        add_msg( m_warning, _( "A %1$s %2$s from the %3$s!" ), name().c_str(),
                 has_flag( MF_SWIMS ) || has_flag( MF_AQUATIC ) ? _( "leaps" ) : _( "emerges" ),
                 g->m.tername( pos() ).c_str() );
    } else if( !was_water && will_be_water && g->u.sees( p ) ) {
        add_msg( m_warning, _( "A %1$s %2$s into the %3$s!" ), name().c_str(),
                 has_flag( MF_SWIMS ) || has_flag( MF_AQUATIC ) ? _( "dives" ) : _( "sinks" ),
                 g->m.tername( p ).c_str() );
    }

    setpos( p );
    footsteps( p );
    underwater = will_be_water;
    if( is_hallucination() ) {
        //Hallucinations don't do any of the stuff after this point
        return true;
    }
    // TODO: Make tanks stop taking damage from rubble, because it's just silly
    if( type->size != MS_TINY && on_ground ) {
        if( g->m.has_flag( "SHARP", pos() ) && !one_in( 4 ) ) {
            apply_damage( nullptr, bp_torso, rng( 1, 10 ) );
        }
        if( g->m.has_flag( "ROUGH", pos() ) && one_in( 6 ) ) {
            apply_damage( nullptr, bp_torso, rng( 1, 2 ) );
        }

    }

    if( g->m.has_flag( "UNSTABLE", p ) && on_ground ) {
        add_effect( effect_bouldering, 1_turns, num_bp, true );
    } else if( has_effect( effect_bouldering ) ) {
        remove_effect( effect_bouldering );
    }
    g->m.creature_on_trap( *this );
    if( !will_be_water && ( has_flag( MF_DIGS ) || has_flag( MF_CAN_DIG ) ) ) {
        underwater = g->m.has_flag( "DIGGABLE", pos() );
    }
    // Diggers turn the dirt into dirtmound
    if( digging() ) {
        int factor = 0;
        switch( type->size ) {
            case MS_TINY:
                factor = 100;
                break;
            case MS_SMALL:
                factor = 30;
                break;
            case MS_MEDIUM:
                factor = 6;
                break;
            case MS_LARGE:
                factor = 3;
                break;
            case MS_HUGE:
                factor = 1;
                break;
        }
        if( one_in( factor ) ) {
            g->m.ter_set( pos(), t_dirtmound );
        }
    }
    // Acid trail monsters leave... a trail of acid
    if( has_flag( MF_ACIDTRAIL ) ) {
        g->m.add_field( pos(), fd_acid, 3 );
    }

    if( has_flag( MF_SLUDGETRAIL ) ) {
        for( const tripoint &sludge_p : g->m.points_in_radius( pos(), 1 ) ) {
            const int fstr = 3 - ( abs( sludge_p.x - posx() ) + abs( sludge_p.y - posy() ) );
            if( fstr >= 2 ) {
                g->m.add_field( sludge_p, fd_sludge, fstr );
            }
        }
    }

    if( has_flag( MF_DRIPS_NAPALM ) ) {
        if( one_in( 10 ) ) {
            g->m.add_item_or_charges( pos(), item( "napalm" ) );
        }
    }

    return true;
}

bool monster::push_to( const tripoint &p, const int boost, const size_t depth )
{
    if( is_hallucination() ) {
        // Don't let hallucinations push, not even other hallucinations
        return false;
    }

    if( !has_flag( MF_PUSH_MON ) || depth > 2 || has_effect( effect_pushed ) ) {
        return false;
    }

    // TODO: Generalize this to Creature
    monster *const critter = g->critter_at<monster>( p );
    if( critter == nullptr || critter == this || p == pos() ) {
        return false;
    }

    if( !can_move_to( p ) ) {
        return false;
    }

    if( critter->is_hallucination() ) {
        // Kill the hallu, but return false so that the regular move_to is uses instead
        critter->die( nullptr );
        return false;
    }

    // Stability roll of the pushed critter
    const int defend = critter->stability_roll();
    // Stability roll of the pushing zed
    const int attack = stability_roll() + boost;
    if( defend > attack ) {
        return false;
    }

    const int movecost_from = 50 * g->m.move_cost( p );
    const int movecost_attacker = std::max( movecost_from, 200 - 10 * ( attack - defend ) );
    const tripoint dir = p - pos();

    // Mark self as pushed to simplify recursive pushing
    add_effect( effect_pushed, 1_turns );

    for( size_t i = 0; i < 6; i++ ) {
        const int dx = rng( -1, 1 );
        const int dy = rng( -1, 1 );
        if( dx == 0 && dy == 0 ) {
            continue;
        }

        // Pushing forward is easier than pushing aside
        const int direction_penalty = abs( dx - dir.x ) + abs( dy - dir.y );
        if( direction_penalty > 2 ) {
            continue;
        }

        tripoint dest( p.x + dx, p.y + dy, p.z );

        // Pushing into cars/windows etc. is harder
        const int movecost_penalty = g->m.move_cost( dest ) - 2;
        if( movecost_penalty <= -2 ) {
            // Can't push into unpassable terrain
            continue;
        }

        int roll = attack - ( defend + direction_penalty + movecost_penalty );
        if( roll < 0 ) {
            continue;
        }

        Creature *critter_recur = g->critter_at( dest );
        if( critter_recur == nullptr || critter_recur->is_hallucination() ) {
            // Try to push recursively
            monster *mon_recur = dynamic_cast< monster * >( critter_recur );
            if( mon_recur == nullptr ) {
                continue;
            }

            if( critter->push_to( dest, roll, depth + 1 ) ) {
                // The tile isn't necessarily free, need to check
                if( !g->critter_at( p ) ) {
                    move_to( p );
                }

                moves -= movecost_attacker;
                if( movecost_from > 100 ) {
                    critter->add_effect( effect_downed, time_duration::from_turns( movecost_from / 100 + 1 ) );
                } else {
                    critter->moves -= movecost_from;
                }

                return true;
            } else {
                continue;
            }
        }

        critter_recur = g->critter_at( dest );
        if( critter_recur != nullptr ) {
            if( critter_recur->is_hallucination() ) {
                critter_recur->die( nullptr );
            } else {
                return false;
            }
        }

        critter->setpos( dest );
        move_to( p );
        moves -= movecost_attacker;
        if( movecost_from > 100 ) {
            critter->add_effect( effect_downed, time_duration::from_turns( movecost_from / 100 + 1 ) );
        } else {
            critter->moves -= movecost_from;
        }

        return true;
    }

    // Try to trample over a much weaker zed (or one with worse rolls)
    // Don't allow trampling with boost
    if( boost > 0 || attack < 2 * defend ) {
        return false;
    }

    g->swap_critters( *critter, *this );
    critter->add_effect( effect_stunned, rng( 0_turns, 2_turns ) );
    // Only print the message when near player or it can get spammy
    if( rl_dist( g->u.pos(), pos() ) < 4 && g->u.sees( *critter ) ) {
        add_msg( m_warning, _( "The %1$s tramples %2$s" ),
                 name().c_str(), critter->disp_name().c_str() );
    }

    moves -= movecost_attacker;
    if( movecost_from > 100 ) {
        critter->add_effect( effect_downed, time_duration::from_turns( movecost_from / 100 + 1 ) );
    } else {
        critter->moves -= movecost_from;
    }

    return true;
}

/**
 * Stumble in a random direction, but with some caveats.
 */
void monster::stumble()
{
    // Only move every 3rd turn.
    if( !one_in( 3 ) ) {
        return;
    }

    std::vector<tripoint> valid_stumbles;
    valid_stumbles.reserve( 11 );
    const bool avoid_water = has_flag( MF_NO_BREATHE ) &&
                             !has_flag( MF_SWIMS ) && !has_flag( MF_AQUATIC );
    for( const tripoint &dest : g->m.points_in_radius( pos(), 1 ) ) {
        if( dest != pos() && can_move_to( dest ) &&
            //Stop zombies and other non-breathing monsters wandering INTO water
            //(Unless they can swim/are aquatic)
            //But let them wander OUT of water if they are there.
            !( avoid_water &&
               g->m.has_flag( TFLAG_SWIMMABLE, dest ) &&
               !g->m.has_flag( TFLAG_SWIMMABLE, pos() ) ) &&
            ( g->critter_at( dest, is_hallucination() ) == nullptr ) ) {
            valid_stumbles.push_back( dest );
        }
    }

    if( g->m.has_zlevels() ) {
        tripoint below( posx(), posy(), posz() - 1 );
        tripoint above( posx(), posy(), posz() + 1 );
        if( g->m.valid_move( pos(), below, false, true ) && can_move_to( below ) ) {
            valid_stumbles.push_back( below );
        }
        // More restrictions for moving up
        if( has_flag( MF_FLIES ) && one_in( 5 ) &&
            g->m.valid_move( pos(), above, false, true ) && can_move_to( above ) ) {
            valid_stumbles.push_back( above );
        }
    }

    if( valid_stumbles.empty() ) { //nowhere to stumble?
        return;
    }

    move_to( random_entry( valid_stumbles ), false );
}

void monster::knock_back_from( const tripoint &p )
{
    if( p == pos() ) {
        return; // No effect
    }
    if( is_hallucination() ) {
        die( nullptr );
        return;
    }
    tripoint to = pos();;
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

    bool u_see = g->u.sees( to );

    // First, see if we hit another monster
    if( monster *const z = g->critter_at<monster>( to ) ) {
        apply_damage( z, bp_torso, z->type->size );
        add_effect( effect_stunned, 1_turns );
        if( type->size > 1 + z->type->size ) {
            z->knock_back_from( pos() ); // Chain reaction!
            z->apply_damage( this, bp_torso, type->size );
            z->add_effect( effect_stunned, 1_turns );
        } else if( type->size > z->type->size ) {
            z->apply_damage( this, bp_torso, type->size );
            z->add_effect( effect_stunned, 1_turns );
        }
        z->check_dead_state();

        if( u_see ) {
            add_msg( _( "The %1$s bounces off a %2$s!" ), name().c_str(), z->name().c_str() );
        }

        return;
    }

    if( npc *const p = g->critter_at<npc>( to ) ) {
        apply_damage( p, bp_torso, 3 );
        add_effect( effect_stunned, 1_turns );
        p->deal_damage( this, bp_torso, damage_instance( DT_BASH, type->size ) );
        if( u_see ) {
            add_msg( _( "The %1$s bounces off %2$s!" ), name().c_str(), p->name.c_str() );
        }

        p->check_dead_state();
        return;
    }

    // If we're still in the function at this point, we're actually moving a tile!
    if( g->m.has_flag_ter( TFLAG_DEEP_WATER, to ) ) {
        if( g->m.has_flag( "LIQUID", to ) && can_drown() ) {
            die( nullptr );
            if( u_see ) {
                add_msg( _( "The %s drowns!" ), name().c_str() );
            }

        } else if( has_flag( MF_AQUATIC ) ) { // We swim but we're NOT in water
            die( nullptr );
            if( u_see ) {
                add_msg( _( "The %s flops around and dies!" ), name().c_str() );
            }
        }
    }

    if( g->m.impassable( to ) ) {

        // It's some kind of wall.
        apply_damage( nullptr, bp_torso, type->size );
        add_effect( effect_stunned, 2_turns );
        if( u_see ) {
            add_msg( _( "The %1$s bounces off a %2$s." ), name().c_str(),
                     g->m.obstacle_name( to ).c_str() );
        }

    } else { // It's no wall
        setpos( to );
    }
    check_dead_state();
}


/* will_reach() is used for determining whether we'll get to stairs (and
 * potentially other locations of interest).  It is generally permissive.
 * TODO: Pathfinding;
         Make sure that non-smashing monsters won't "teleport" through windows
         Injure monsters if they're gonna be walking through pits or whatever
 */
bool monster::will_reach( int x, int y )
{
    monster_attitude att = attitude( &( g->u ) );
    if( att != MATT_FOLLOW && att != MATT_ATTACK && att != MATT_FRIEND && att != MATT_ZLAVE ) {
        return false;
    }

    if( has_flag( MF_DIGS ) || has_flag( MF_AQUATIC ) ) {
        return false;
    }

    if( has_flag( MF_IMMOBILE ) && ( posx() != x || posy() != y ) ) {
        return false;
    }

    auto path = g->m.route( pos(), tripoint( x, y, posz() ), get_pathfinding_settings() );
    if( path.empty() ) {
        return false;
    }

    if( has_flag( MF_SMELLS ) && g->scent.get( pos() ) > 0 &&
        g->scent.get( { x, y, posz() } ) > g->scent.get( pos() ) ) {
        return true;
    }

    if( can_hear() && wandf > 0 && rl_dist( wander_pos.x, wander_pos.y, x, y ) <= 2 &&
        rl_dist( posx(), posy(), wander_pos.x, wander_pos.y ) <= wandf ) {
        return true;
    }

    if( can_see() && sees( tripoint( x, y, posz() ) ) ) {
        return true;
    }

    return false;
}

int monster::turns_to_reach( int x, int y )
{
    // This function is a(n old) temporary hack that should soon be removed
    auto path = g->m.route( pos(), tripoint( x, y, posz() ), get_pathfinding_settings() );
    if( path.empty() ) {
        return 999;
    }

    double turns = 0.;
    for( size_t i = 0; i < path.size(); i++ ) {
        const tripoint &next = path[i];
        if( g->m.impassable( next ) ) {
            // No bashing through, it looks stupid when you go back and find
            // the doors intact.
            return 999;
        } else if( i == 0 ) {
            turns += double( calc_movecost( pos(), next ) ) / get_speed();
        } else {
            turns += double( calc_movecost( path[i - 1], next ) ) / get_speed();
        }
    }

    return int( turns + .9 ); // Halve (to get turns) and round up
}
