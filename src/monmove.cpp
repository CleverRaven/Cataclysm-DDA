// Monster movement code; essentially, the AI

#include "monster.h" // IWYU pragma: associated

#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <memory>
#include <ostream>

#include "avatar.h"
#include "bionics.h"
#include "debug.h"
#include "field.h"
#include "game.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "monfaction.h"
#include "mtype.h"
#include "creature_tracker.h"
#include "npc.h"
#include "rng.h"
#include "scent_map.h"
#include "sounds.h"
#include "translations.h"
#include "trap.h"
#include "vpart_position.h"
#include "tileray.h"
#include "vehicle.h"
#include "cata_utility.h"
#include "game_constants.h"
#include "mattack_common.h"
#include "pathfinding.h"
#include "player.h"
#include "int_id.h"
#include "string_id.h"
#include "pimpl.h"
#include "string_formatter.h"

#define MONSTER_FOLLOW_DIST 8

const species_id FUNGUS( "FUNGUS" );
const species_id INSECT( "INSECT" );
const species_id SPIDER( "SPIDER" );

const efftype_id effect_bouldering( "bouldering" );
const efftype_id effect_countdown( "countdown" );
const efftype_id effect_docile( "docile" );
const efftype_id effect_downed( "downed" );
const efftype_id effect_dragging( "dragging" );
const efftype_id effect_grabbed( "grabbed" );
const efftype_id effect_narcosis( "narcosis" );
const efftype_id effect_no_sight( "no_sight" );
const efftype_id effect_operating( "operating" );
const efftype_id effect_pacified( "pacified" );
const efftype_id effect_pushed( "pushed" );
const efftype_id effect_stunned( "stunned" );
const efftype_id effect_harnessed( "harnessed" );
const efftype_id effect_hack_detected( "hack_detected" );

const species_id ZOMBIE( "ZOMBIE" );
const species_id BLOB( "BLOB" );
const species_id ROBOT( "ROBOT" );
const species_id WORM( "WORM" );

bool monster::wander()
{
    return ( goal == pos() );
}

bool monster::is_immune_field( const field_type_id fid ) const
{
    if( fid == fd_fungal_haze ) {
        return has_flag( MF_NO_BREATHE ) || type->in_species( FUNGUS );
    }
    if( fid == fd_fungicidal_gas ) {
        return !type->in_species( FUNGUS );
    }
    if( fid == fd_insecticidal_gas ) {
        return !type->in_species( INSECT ) && !type->in_species( SPIDER );
    }
    const field_type &ft = fid.obj();
    if( ft.has_fume ) {
        return has_flag( MF_NO_BREATHE );
    }
    if( ft.has_acid ) {
        return has_flag( MF_ACIDPROOF ) || has_flag( MF_FLIES );
    }
    if( ft.has_fire ) {
        return has_flag( MF_FIREPROOF );
    }
    if( ft.has_elec ) {
        return has_flag( MF_ELECTRIC );
    }
    // No specific immunity was found, so fall upwards
    return Creature::is_immune_field( fid );
}

bool monster::can_move_to( const tripoint &p ) const
{
    const bool can_climb = has_flag( MF_CLIMBS ) || has_flag( MF_FLIES );

    if( g->m.impassable( p ) ) {
        if( digging() ) {
            if( !g->m.has_flag( "BURROWABLE", p ) ) {
                return false;
            }
        } else if( !( can_climb && g->m.has_flag( "CLIMBABLE", p ) ) ) {
            return false;
        }
    }

    if( ( !can_submerge() && !has_flag( MF_FLIES ) ) && g->m.has_flag( TFLAG_DEEP_WATER, p ) ) {
        return false;
    }

    if( has_flag( MF_DIGS ) && !g->m.has_flag( "DIGGABLE", p ) && !g->m.has_flag( "BURROWABLE", p ) ) {
        return false;
    }

    if( has_flag( MF_AQUATIC ) && !g->m.has_flag( "SWIMMABLE", p ) ) {
        return false;
    }

    if( has_flag( MF_SUNDEATH ) && g->is_in_sunlight( p ) ) {
        return false;
    }

    // Various avoiding behaviors.

    bool avoid_fire = has_flag( MF_AVOID_FIRE );
    bool avoid_fall = has_flag( MF_AVOID_FALL );
    bool avoid_simple = has_flag( MF_AVOID_DANGER_1 );
    bool avoid_complex = has_flag( MF_AVOID_DANGER_2 );
    /*
     * Because some avoidance behaviors are supersets of others,
     * we can cascade through the implications. Complex implies simple,
     * and simple implies fire and fall.
     * unfortunately, fall does not necessarily imply fire, nor the converse.
     */
    if( avoid_complex ) {
        avoid_simple = true;
    }
    if( avoid_simple ) {
        avoid_fire = true;
        avoid_fall = true;
    }

    // technically this will shortcut in evaluation from fire or fall
    // before hitting simple or complex but this is more explicit
    if( avoid_fire || avoid_fall || avoid_simple || avoid_complex ) {
        const ter_id target = g->m.ter( p );

        // Don't enter lava if we have any concept of heat being bad
        if( avoid_fire && target == t_lava ) {
            return false;
        }

        if( avoid_fall ) {
            // Don't throw ourselves off cliffs if we have a concept of falling
            if( !g->m.has_floor( p ) && !has_flag( MF_FLIES ) ) {
                return false;
            }

            // Don't enter open pits ever unless tiny, can fly or climb well
            if( !( type->size == MS_TINY || can_climb ) &&
                ( target == t_pit || target == t_pit_spiked || target == t_pit_glass ) ) {
                return false;
            }
        }

        // Some things are only avoided if we're not attacking
        if( attitude( &g->u ) != MATT_ATTACK ) {
            // Sharp terrain is ignored while attacking
            if( avoid_simple && g->m.has_flag( "SHARP", p ) &&
                !( type->size == MS_TINY || has_flag( MF_FLIES ) ) ) {
                return false;
            }
        }

        const field &target_field = g->m.field_at( p );

        // Higher awareness is needed for identifying these as threats.
        if( avoid_complex ) {
            const trap &target_trap = g->m.tr_at( p );
            // Don't enter any dangerous fields
            if( is_dangerous_fields( target_field ) ) {
                return false;
            }
            // Don't step on any traps (if we can see)
            if( has_flag( MF_SEES ) && !target_trap.is_benign() && g->m.has_floor( p ) ) {
                return false;
            }
        }

        // Without avoid_complex, only fire and electricity are checked for field avoidance.
        if( avoid_fire && target_field.find_field( fd_fire ) ) {
            return false;
        }
        if( avoid_simple && target_field.find_field( fd_electricity ) ) {
            return false;
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

void monster::plan()
{
    const auto &factions = g->critter_tracker->factions();

    // Bots are more intelligent than most living stuff
    bool smart_planning = has_flag( MF_PRIORITIZE_TARGETS );
    Creature *target = nullptr;
    int max_sight_range = std::max( type->vision_day, type->vision_night );
    // 8.6f is rating for tank drone 60 tiles away, moose 16 or boomer 33
    float dist = !smart_planning ? max_sight_range : 8.6f;
    bool fleeing = false;
    bool docile = friendly != 0 && has_effect( effect_docile );

    const bool angers_hostile_weak = type->has_anger_trigger( mon_trigger::HOSTILE_WEAK );
    const int angers_hostile_near = type->has_anger_trigger( mon_trigger::HOSTILE_CLOSE ) ? 5 : 0;
    const int angers_mating_season = type->has_anger_trigger( mon_trigger::MATING_SEASON ) ? 3 : 0;
    const int angers_cub_threatened = type->has_anger_trigger( mon_trigger::PLAYER_NEAR_BABY ) ? 8 : 0;
    const int fears_hostile_near = type->has_fear_trigger( mon_trigger::HOSTILE_CLOSE ) ? 5 : 0;

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
            if( angers_mating_season > 0 ) {
                bool mating_angry = false;
                season_type season = season_of_year( calendar::turn );
                for( auto &elem : type->baby_flags ) {
                    if( ( season == SUMMER && elem == "SUMMER" ) ||
                        ( season == WINTER && elem == "WINTER" ) ||
                        ( season == SPRING && elem == "SPRING" ) ||
                        ( season == AUTUMN && elem == "AUTUMN" ) ) {
                        mating_angry = true;
                        break;
                    }
                }
                if( mating_angry ) {
                    anger += angers_mating_season;
                }
            }
        }
        if( angers_cub_threatened > 0 ) {
            for( monster &tmp : g->all_monsters() ) {
                if( type->baby_monster == tmp.type->id ) {
                    // baby nearby; is the player too close?
                    dist = tmp.rate_target( g->u, dist, smart_planning );
                    if( dist <= 3 ) {
                        //proximity to baby; monster gets furious and less likely to flee
                        anger += angers_cub_threatened;
                        morale += angers_cub_threatened / 2;
                    }
                }
            }
        }
    } else if( friendly != 0 && !docile ) {
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
            if( angers_mating_season > 0 ) {
                bool mating_angry = false;
                season_type season = season_of_year( calendar::turn );
                for( auto &elem : type->baby_flags ) {
                    if( ( season == SUMMER && elem == "SUMMER" ) ||
                        ( season == WINTER && elem == "WINTER" ) ||
                        ( season == SPRING && elem == "SPRING" ) ||
                        ( season == AUTUMN && elem == "AUTUMN" ) ) {
                        mating_angry = true;
                        break;
                    }
                }
                if( mating_angry ) {
                    anger += angers_mating_season;
                }
            }
        }
    }

    fleeing = fleeing || ( mood == MATT_FLEE );
    if( friendly == 0 ) {
        for( const auto &fac : factions ) {
            auto faction_att = faction.obj().attitude( fac.first );
            if( faction_att == MFA_NEUTRAL || faction_att == MFA_FRIENDLY ) {
                continue;
            }

            for( const std::weak_ptr<monster> &weak : fac.second ) {
                const std::shared_ptr<monster> shared = weak.lock();
                if( !shared ) {
                    continue;
                }
                monster &mon = *shared;
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
    const auto &myfaction_iter = factions.find( actual_faction );
    if( myfaction_iter == factions.end() ) {
        DebugLog( D_ERROR, D_GAME ) << disp_name() << " tried to find faction "
                                    << actual_faction.id().str()
                                    << " which wasn't loaded in game::monmove";
        swarms = false;
        group_morale = false;
    }
    swarms = swarms && target == nullptr; // Only swarm if we have no target
    if( group_morale || swarms ) {
        for( const std::weak_ptr<monster> &weak : myfaction_iter->second ) {
            const std::shared_ptr<monster> shared = weak.lock();
            if( !shared ) {
                continue;
            }
            monster &mon = *shared;
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

    // Operating monster keep you safe while they operate, how nice....
    if( type->has_special_attack( "OPERATE" ) ) {
        int prev_friendlyness = friendly;
        if( has_effect( effect_operating ) ) {
            friendly = 100;
            for( auto critter : g->m.get_creatures_in_radius( pos(), 6 ) ) {
                monster *mon = dynamic_cast<monster *>( critter );
                if( mon != nullptr && mon->type->in_species( ZOMBIE ) ) {
                    anger = 100;
                } else {
                    anger = 0;
                }
            }
        } else {
            friendly = prev_friendlyness;
        }
    }

    if( has_effect( effect_dragging ) ) {

        if( type->has_special_attack( "OPERATE" ) ) {

            bool found_path_to_couch = false;
            tripoint tmp( pos() + point( 12, 12 ) );
            tripoint couch_loc;
            for( const auto &couch_pos : g->m.find_furnitures_in_radius( pos(), 10,
                    furn_id( "f_autodoc_couch" ) ) ) {
                if( g->m.clear_path( pos(), couch_pos, 10, 0, 100 ) ) {
                    if( rl_dist( pos(), couch_pos ) < rl_dist( pos(), tmp ) ) {
                        tmp = couch_pos;
                        found_path_to_couch = true;
                        couch_loc = couch_pos;
                    }
                }
            }

            if( !found_path_to_couch ) {
                anger = 0;
                remove_effect( effect_dragging );
            } else {
                set_dest( couch_loc );
            }
        }

    } else if( target != nullptr ) {

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
                anger += 10 - static_cast<int>( hp_per / 10 );
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

bool monster::die_if_drowning( const tripoint &at_pos, const int chance )
{
    if( g->m.has_flag( "LIQUID", at_pos ) && can_drown() && one_in( chance ) ) {
        // if there's a vehicle here with a boardable part, the monster is on it
        // and not drowning
        if( g->m.veh_at( at_pos ).part_with_feature( "BOARDABLE", false ) ) {
            return false;
        }
        die( nullptr );
        if( g->u.sees( at_pos ) ) {
            add_msg( _( "The %s drowns!" ), name() );
        }
        return true;
    }
    return false;
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
                     name() );
        }
        static const auto volume_per_hp = 250_ml;
        for( auto &elem : g->m.i_at( pos() ) ) {
            hp += elem.volume() / volume_per_hp; // Yeah this means it can get more HP than normal.
            if( has_flag( MF_ABSORBS_SPLITS ) ) {
                while( hp / 2 > type->hp ) {
                    monster *const spawn = g->place_critter_around( type->id, pos(), 1 );
                    if( !spawn ) {
                        break;
                    }
                    hp -= type->hp;
                    //this is a new copy of the monster. Ideally we should copy the stats/effects that affect the parent
                    spawn->make_ally( *this );
                    if( g->u.sees( *this ) ) {
                        add_msg( _( "The %s splits in two!" ), name() );
                    }
                }
            }
        }
        g->m.i_clear( pos() );
    }
    // record position before moving to put the player there if we're dragging
    tripoint drag_to = pos();

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

    // Check if they're dragging a foe and find their hapless victim
    player *dragged_foe = find_dragged_foe();

    // Give nursebots a chance to do surgery.
    nursebot_operate( dragged_foe );

    // The monster can sometimes hang in air due to last fall being blocked
    const bool can_fly = has_flag( MF_FLIES );
    if( !can_fly && g->m.has_flag( TFLAG_NO_FLOOR, pos() ) ) {
        g->m.creature_on_trap( *this, false );
        if( is_dead() ) {
            return;
        }
    }

    // The monster is in a deep water tile and has a chance to drown
    if( g->m.has_flag_ter( TFLAG_DEEP_WATER, pos() ) ) {
        if( die_if_drowning( pos(), 10 ) ) {
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
    if( has_flag( MF_IMMOBILE ) || has_flag( MF_RIDEABLE_MECH ) ) {
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

    // don't move if a passenger in a moving vehicle
    auto vp = g->m.veh_at( pos() );
    bool harness_part = static_cast<bool>( g->m.veh_at( pos() ).part_with_feature( "ANIMAL_CTRL",
                                           true ) );
    if( vp && vp->vehicle().is_moving() && vp->vehicle().get_pet( vp->part_index() ) ) {
        moves = 0;
        return;
        // Don't move if harnessed, even if vehicle is stationary
    } else if( vp && has_effect( effect_harnessed ) ) {
        moves = 0;
        return;
        // If harnessed monster finds itself moved from the harness point, the harness probably broke!
    } else if( !harness_part && has_effect( effect_harnessed ) ) {
        remove_effect( effect_harnessed );
    }
    // Set attitude to attitude to our current target
    monster_attitude current_attitude = attitude( nullptr );
    if( !wander() ) {
        if( goal == g->u.pos() ) {
            current_attitude = attitude( &g->u );
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

    int new_dx = destination.x - pos().x;
    int new_dy = destination.y - pos().y;

    // toggle facing direction for sdl flip
    if( ! tile_iso ) {
        if( new_dx < 0 ) {
            facing = FD_LEFT;
        } else if( new_dx > 0 ) {
            facing = FD_RIGHT;
        }
    } else {
        if( new_dy <= 0 && new_dx <= 0 ) {
            facing = FD_LEFT;
        }
        if( new_dx >= 0 && new_dy >= 0 ) {
            facing = FD_RIGHT;
        }
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
                // TODO: Remove z-level stair bullshit teleport after aligning all stairs
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

            // Try to shove vehicle out of the way
            shove_vehicle( destination, candidate );

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
            if( progress > 0 && ( !moved || x_in_y( progress, switch_chance ) ) ) {
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
    const bool can_open_doors = has_flag( MF_CAN_OPEN_DOORS );
    // Finished logic section.  By this point, we should have chosen a square to
    //  move to (moved = true).
    if( moved ) { // Actual effects of moving to the square we've chosen
        const bool did_something =
            ( !pacified && attack_at( next_step ) ) ||
            ( !pacified && can_open_doors && g->m.open_door( next_step, !g->m.is_outside( pos() ) ) ) ||
            ( !pacified && bash_at( next_step ) ) ||
            ( !pacified && push_to( next_step, 0, 0 ) ) ||
            move_to( next_step, false, get_stagger_adjust( pos(), destination, next_step ) );

        if( !did_something ) {
            moves -= 100; // If we don't do this, we'll get infinite loops.
        }
        if( has_effect( effect_dragging ) && dragged_foe != nullptr ) {

            if( !dragged_foe->has_effect( effect_grabbed ) ) {
                dragged_foe = nullptr;
                remove_effect( effect_dragging );
            } else if( drag_to != pos() && g->critter_at( drag_to ) == nullptr ) {
                dragged_foe->setpos( drag_to );
            }
        }
    } else {
        moves -= 100;
        stumble();
        path.clear();
    }
}

player *monster::find_dragged_foe()
{
    // Make sure they're actually dragging someone.
    if( !dragged_foe_id.is_valid() || !has_effect( effect_dragging ) ) {
        dragged_foe_id = character_id();
        return nullptr;
    }

    // Dragged critters may die or otherwise become invalid, which is why we look
    // them up each time. Luckily, monsters dragging critters is relatively rare,
    // so this check should happen infrequently.
    player *dragged_foe = g->critter_by_id<player>( dragged_foe_id );

    if( dragged_foe == nullptr ) {
        // Target no longer valid.
        dragged_foe_id = character_id();
        remove_effect( effect_dragging );
    }

    return dragged_foe;
}

// Nursebot surgery code
void monster::nursebot_operate( player *dragged_foe )
{
    // No dragged foe, nothing to do.
    if( dragged_foe == nullptr ) {
        return;
    }

    // Nothing to do if they can't operate, or they don't think they're dragging.
    if( !( type->has_special_attack( "OPERATE" ) && has_effect( effect_dragging ) ) ) {
        return;
    }

    if( rl_dist( pos(), goal ) == 1 && g->m.furn( goal ) == furn_id( "f_autodoc_couch" ) &&
        !has_effect( effect_operating ) ) {
        if( dragged_foe->has_effect( effect_grabbed ) && !has_effect( effect_countdown ) &&
            ( g->critter_at( goal ) == nullptr || g->critter_at( goal ) == dragged_foe ) ) {
            add_msg( m_bad, _( "The %1$s slowly but firmly puts %2$s down onto the autodoc couch." ), name(),
                     dragged_foe->disp_name() );

            dragged_foe->setpos( goal );

            add_effect( effect_countdown, 2_turns );// there's still time to get away
            add_msg( m_bad, _( "The %s produces a syringe full of some translucent liquid." ), name() );
        } else if( g->critter_at( goal ) != nullptr && has_effect( effect_dragging ) ) {
            sounds::sound( pos(), 8, sounds::sound_t::speech,
                           string_format(
                               _( "a soft robotic voice say, \"Please step away from the autodoc, this patient needs immediate care.\"" ) ) );
            // TODO: Make it able to push NPC/player
            push_to( goal, 4, 0 );
        }
    }
    if( get_effect_dur( effect_countdown ) == 1_turns && !has_effect( effect_operating ) ) {
        if( dragged_foe->has_effect( effect_grabbed ) ) {

            bionic_collection collec = *dragged_foe->my_bionics;
            int index = rng( 0, collec.size() - 1 );
            bionic target_cbm = collec[index];

            //8 intelligence*4 + 8 first aid*4 + 3 computer *3 + 4 electronic*1 = 77
            float adjusted_skill = static_cast<float>( 77 ) - std::min( static_cast<float>( 40 ),
                                   static_cast<float>( 77 ) - static_cast<float>( 77 ) / static_cast<float>( 10.0 ) );

            g->u.uninstall_bionic( target_cbm, *this, *dragged_foe, adjusted_skill );

            dragged_foe->remove_effect( effect_grabbed );
            remove_effect( effect_dragging );
            dragged_foe_id = character_id();

        }
    }
}

// footsteps will determine how loud a monster's normal movement is
// and create a sound in the monsters location when they move
void monster::footsteps( const tripoint &p )
{
    if( made_footstep ) {
        return;
    }
    made_footstep = true;
    int volume = 6; // same as player's footsteps
    if( has_flag( MF_FLIES ) ) {
        volume = 0;    // Flying monsters don't have footsteps!
    }
    if( digging() ) {
        volume = 10;
    }
    switch( type->size ) {
        case MS_TINY:
            volume = 0; // No sound for the tinies
            break;
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
    if( has_flag( MF_LOUDMOVES ) ) {
        volume += 6;
    }
    if( volume == 0 ) {
        return;
    }
    int dist = rl_dist( p, g->u.pos() );
    sounds::add_footstep( p, volume, dist, this, type->get_footsteps() );
    return;
}

tripoint monster::scent_move()
{
    // TODO: Remove when scentmap is 3D
    if( abs( posz() - g->get_levz() ) > SCENT_MAP_Z_REACH ) {
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
    for( const auto &dest : g->m.points_in_radius( pos(), 1, SCENT_MAP_Z_REACH ) ) {
        int smell = g->scent.get( dest );
        if( ( !fleeing && smell < bestsmell ) || ( fleeing && smell > bestsmell ) ) {
            continue;
        }
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
static std::vector<tripoint> get_bashing_zone( const tripoint &bashee, const tripoint &basher,
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
        std::vector<point> swath = squares_in_direction( previous.xy(), p.xy() );
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

    // Don't bash if a friendly monster is standing there
    monster *target = g->critter_at<monster>( p );
    if( target != nullptr && attitude_to( *target ) == A_FRIENDLY ) {
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
    if( has_flag( MF_PACIFIST ) ) {
        return false;
    }
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
        guy->on_attacked( *this ); // allow NPC hallucination to be one shot by monsters
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
                    add_msg( _( "The %1$s flies over the %2$s." ), name(),
                             g->m.has_flag_furn( "CLIMBABLE", p ) ? g->m.furnname( p ) :
                             g->m.tername( p ) );
                }
            } else if( has_flag( MF_CLIMBS ) ) {
                moves -= 150;
                force = true;
                if( g->u.sees( *this ) ) {
                    add_msg( _( "The %1$s climbs over the %2$s." ), name(),
                             g->m.has_flag_furn( "CLIMBABLE", p ) ? g->m.furnname( p ) :
                             g->m.tername( p ) );
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
                           static_cast<float>( climbs ? calc_climb_cost( pos(), p ) : calc_movecost( pos(), p ) );
        if( cost > 0.0f ) {
            moves -= static_cast<int>( ceil( cost ) );
        } else {
            return false;
        }
    }

    //Check for moving into/out of water
    bool was_water = g->m.is_divable( pos() );
    bool will_be_water = on_ground && can_submerge() && g->m.is_divable( p );

    //Birds and other flying creatures flying over the deep water terrain
    if( was_water && flies && g->u.sees( p ) ) {
        if( one_in( 4 ) ) {
            add_msg( m_warning, _( "A %1$s flies over the %2$s!" ), name(),
                     g->m.tername( pos() ) );
        }
    } else if( was_water && !will_be_water && g->u.sees( p ) ) {
        // Use more dramatic messages for swimming monsters
        //~ Message when a monster emerges from water
        //~ %1$s: monster name, %2$s: leaps/emerges, %3$s: terrain name
        add_msg( m_warning, pgettext( "monster movement", "A %1$s %2$s from the %3$s!" ), name(),
                 has_flag( MF_SWIMS ) || has_flag( MF_AQUATIC ) ? _( "leaps" ) : _( "emerges" ),
                 g->m.tername( pos() ) );
    } else if( !was_water && will_be_water && g->u.sees( p ) ) {
        //~ Message when a monster enters water
        //~ %1$s: monster name, %2$s: dives/sinks, %3$s: terrain name
        add_msg( m_warning, pgettext( "monster movement", "A %1$s %2$s into the %3$s!" ), name(),
                 has_flag( MF_SWIMS ) || has_flag( MF_AQUATIC ) ? _( "dives" ) : _( "sinks" ),
                 g->m.tername( p ) );
    }

    setpos( p );
    footsteps( p );
    underwater = will_be_water;
    if( is_hallucination() ) {
        //Hallucinations don't do any of the stuff after this point
        return true;
    }

    if( type->size != MS_TINY && on_ground ) {
        const int sharp_damage = rng( 1, 10 );
        const int rough_damage = rng( 1, 2 );
        if( g->m.has_flag( "SHARP", pos() ) && !one_in( 4 ) && get_armor_cut( bp_torso ) < sharp_damage ) {
            apply_damage( nullptr, bp_torso, sharp_damage );
        }
        if( g->m.has_flag( "ROUGH", pos() ) && one_in( 6 ) && get_armor_cut( bp_torso ) < rough_damage ) {
            apply_damage( nullptr, bp_torso, rough_damage );
        }
    }

    if( g->m.has_flag( "UNSTABLE", p ) && on_ground ) {
        add_effect( effect_bouldering, 1_turns, num_bp, true );
    } else if( has_effect( effect_bouldering ) ) {
        remove_effect( effect_bouldering );
    }

    if( g->m.has_flag_ter_or_furn( TFLAG_NO_SIGHT, p ) && on_ground ) {
        add_effect( effect_no_sight, 1_turns, num_bp, true );
    } else if( has_effect( effect_no_sight ) ) {
        remove_effect( effect_no_sight );
    }

    g->m.creature_on_trap( *this );
    if( is_dead() ) {
        return true;
    }
    if( !will_be_water && ( has_flag( MF_DIGS ) || has_flag( MF_CAN_DIG ) ) ) {
        underwater = g->m.has_flag( "DIGGABLE", pos() );
    }
    // Diggers turn the dirt into dirtmound
    if( digging() && g->m.has_flag( "DIGGABLE", pos() ) ) {
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
        // TODO: make this take terrain type into account so diggers travelling under sand will create mounds of sand etc.
        if( one_in( factor ) ) {
            g->m.ter_set( pos(), t_dirtmound );
        }
    }
    // Acid trail monsters leave... a trail of acid
    if( has_flag( MF_ACIDTRAIL ) ) {
        g->m.add_field( pos(), fd_acid, 3 );
    }

    // Not all acid trail monsters leave as much acid. Every time this monster takes a step, there is a 1/5 chance it will drop a puddle.
    if( has_flag( MF_SHORTACIDTRAIL ) ) {
        if( one_in( 5 ) ) {
            g->m.add_field( pos(), fd_acid, 3 );
        }
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
            // if it has more napalm, drop some and reduce ammo in tank
            if( ammo["pressurized_tank"] > 0 ) {
                g->m.add_item_or_charges( pos(), item( "napalm", calendar::turn, 50 ) );
                ammo["pressurized_tank"] -= 50;
            } else {
                // TODO remove MF_DRIPS_NAPALM flag since no more napalm in tank
                // Not possible for now since flag check is done on type, not individual monster
            }
        }
    }
    if( has_flag( MF_DRIPS_GASOLINE ) ) {
        if( one_in( 5 ) ) {
            // TODO use same idea that limits napalm dripping
            g->m.add_item_or_charges( pos(), item( "gasoline" ) );
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
    if( critter == nullptr || critter == this ||
        p == pos() || critter->movement_impaired() ) {
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

        tripoint dest( p + point( dx, dy ) );
        const int dest_movecost_from = 50 * g->m.move_cost( dest );

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
        if( !( critter_recur == nullptr || critter_recur->is_hallucination() ) ) {
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

                // Don't knock down a creature that successfully
                // pushed another creature, just reduce moves
                critter->moves -= dest_movecost_from;
                return true;
            } else {
                return false;
            }
        }

        critter_recur = g->critter_at( dest );
        if( critter_recur != nullptr ) {
            if( critter_recur->is_hallucination() ) {
                critter_recur->die( nullptr );
            }
        } else if( !critter->has_flag( MF_IMMOBILE ) ) {
            critter->setpos( dest );
            move_to( p );
            moves -= movecost_attacker;
            critter->add_effect( effect_downed, time_duration::from_turns( movecost_from / 100 + 1 ) );
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
                 name(), critter->disp_name() );
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
    // Only move every 10 turns.
    if( !one_in( 10 ) ) {
        return;
    }

    std::vector<tripoint> valid_stumbles;
    valid_stumbles.reserve( 11 );
    const bool avoid_water = has_flag( MF_NO_BREATHE ) &&
                             !has_flag( MF_SWIMS ) && !has_flag( MF_AQUATIC );
    for( const tripoint &dest : g->m.points_in_radius( pos(), 1 ) ) {
        if( dest != pos() ) {
            valid_stumbles.push_back( dest );
        }
    }

    if( g->m.has_zlevels() ) {
        tripoint below( posx(), posy(), posz() - 1 );
        tripoint above( posx(), posy(), posz() + 1 );
        if( g->m.valid_move( pos(), below, false, true ) ) {
            valid_stumbles.push_back( below );
        }
        // More restrictions for moving up
        if( has_flag( MF_FLIES ) && one_in( 5 ) &&
            g->m.valid_move( pos(), above, false, true ) ) {
            valid_stumbles.push_back( above );
        }
    }
    while( !valid_stumbles.empty() && !is_dead() ) {
        const tripoint dest = random_entry_removed( valid_stumbles );
        if( can_move_to( dest ) &&
            //Stop zombies and other non-breathing monsters wandering INTO water
            //(Unless they can swim/are aquatic)
            //But let them wander OUT of water if they are there.
            !( avoid_water &&
               g->m.has_flag( TFLAG_SWIMMABLE, dest ) &&
               !g->m.has_flag( TFLAG_SWIMMABLE, pos() ) ) &&
            ( g->critter_at( dest, is_hallucination() ) == nullptr ) ) {
            move_to( dest, true );
        }
    }
}

void monster::knock_back_to( const tripoint &to )
{
    if( to == pos() ) {
        return; // No effect
    }

    if( is_hallucination() ) {
        die( nullptr );
        return;
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
            add_msg( _( "The %1$s bounces off a %2$s!" ), name(), z->name() );
        }

        return;
    }

    if( npc *const p = g->critter_at<npc>( to ) ) {
        apply_damage( p, bp_torso, 3 );
        add_effect( effect_stunned, 1_turns );
        p->deal_damage( this, bp_torso, damage_instance( DT_BASH, type->size ) );
        if( u_see ) {
            add_msg( _( "The %1$s bounces off %2$s!" ), name(), p->name );
        }

        p->check_dead_state();
        return;
    }

    // If we're still in the function at this point, we're actually moving a tile!
    if( g->m.has_flag_ter( TFLAG_DEEP_WATER, to ) ) {
        // die_if_drowning will kill the monster if necessary, but if the deep water
        // tile is on a vehicle, we should check for swimmers out of water
        if( !die_if_drowning( to ) && has_flag( MF_AQUATIC ) ) {
            die( nullptr );
            if( u_see ) {
                add_msg( _( "The %s flops around and dies!" ), name() );
            }
        }
    }

    if( g->m.impassable( to ) ) {

        // It's some kind of wall.
        apply_damage( nullptr, bp_torso, type->size );
        add_effect( effect_stunned, 2_turns );
        if( u_see ) {
            add_msg( _( "The %1$s bounces off a %2$s." ), name(),
                     g->m.obstacle_name( to ) );
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
bool monster::will_reach( const point &p )
{
    monster_attitude att = attitude( &g->u );
    if( att != MATT_FOLLOW && att != MATT_ATTACK && att != MATT_FRIEND && att != MATT_ZLAVE ) {
        return false;
    }

    if( has_flag( MF_DIGS ) || has_flag( MF_AQUATIC ) ) {
        return false;
    }

    if( ( has_flag( MF_IMMOBILE ) || has_flag( MF_RIDEABLE_MECH ) ) && ( pos().xy() != p ) ) {
        return false;
    }

    auto path = g->m.route( pos(), tripoint( p, posz() ), get_pathfinding_settings() );
    if( path.empty() ) {
        return false;
    }

    if( has_flag( MF_SMELLS ) && g->scent.get( pos() ) > 0 &&
        g->scent.get( { p, posz() } ) > g->scent.get( pos() ) ) {
        return true;
    }

    if( can_hear() && wandf > 0 && rl_dist( wander_pos.xy(), p ) <= 2 &&
        rl_dist( point( posx(), posy() ), wander_pos.xy() ) <= wandf ) {
        return true;
    }

    if( can_see() && sees( tripoint( p, posz() ) ) ) {
        return true;
    }

    return false;
}

int monster::turns_to_reach( const point &p )
{
    // This function is a(n old) temporary hack that should soon be removed
    auto path = g->m.route( pos(), tripoint( p, posz() ), get_pathfinding_settings() );
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
            turns += static_cast<double>( calc_movecost( pos(), next ) ) / get_speed();
        } else {
            turns += static_cast<double>( calc_movecost( path[i - 1], next ) ) / get_speed();
        }
    }

    return static_cast<int>( turns + .9 ); // Halve (to get turns) and round up
}

void monster::shove_vehicle( const tripoint &remote_destination,
                             const tripoint &nearby_destination )
{
    if( this->has_flag( MF_PUSH_VEH ) ) {
        auto vp = g->m.veh_at( nearby_destination );
        if( vp ) {
            vehicle &veh = vp->vehicle();
            const units::mass veh_mass = veh.total_mass();
            int shove_moves_minimal = 0;
            int shove_veh_mass_moves_factor = 0;
            int shove_velocity = 0;
            float shove_damage_min = 0.00F;
            float shove_damage_max = 0.00F;
            switch( this->get_size() ) {
                case MS_TINY:
                case MS_SMALL:
                    break;
                case MS_MEDIUM:
                    if( veh_mass < 500_kilogram ) {
                        shove_moves_minimal = 150;
                        shove_veh_mass_moves_factor = 20;
                        shove_velocity = 500;
                        shove_damage_min = 0.00F;
                        shove_damage_max = 0.01F;
                    }
                    break;
                case MS_LARGE:
                    if( veh_mass < 1000_kilogram ) {
                        shove_moves_minimal = 100;
                        shove_veh_mass_moves_factor = 8;
                        shove_velocity = 1000;
                        shove_damage_min = 0.00F;
                        shove_damage_max = 0.03F;
                    }
                    break;
                case MS_HUGE:
                    if( veh_mass < 2000_kilogram ) {
                        shove_moves_minimal = 50;
                        shove_veh_mass_moves_factor = 4;
                        shove_velocity = 1500;
                        shove_damage_min = 0.00F;
                        shove_damage_max = 0.05F;
                    }
                    break;
                default:
                    break;
            }
            if( shove_velocity > 0 ) {
                if( g->u.sees( this->pos() ) ) {
                    //~ %1$s - monster name, %2$s - vehicle name
                    g->u.add_msg_if_player( m_bad, _( "%1$s shoves %2$s out of their way!" ), this->disp_name(),
                                            veh.disp_name() );
                }
                int shove_moves = shove_veh_mass_moves_factor * veh_mass / 10_kilogram;
                shove_moves = std::max( shove_moves, shove_moves_minimal );
                this->mod_moves( -shove_moves );
                const int destination_delta_x = remote_destination.x - nearby_destination.x;
                const int destination_delta_y = remote_destination.y - nearby_destination.y;
                const int destination_delta_z = remote_destination.z - nearby_destination.z;
                const tripoint shove_destination( clamp( destination_delta_x, -1, 1 ),
                                                  clamp( destination_delta_y, -1, 1 ),
                                                  clamp( destination_delta_z, -1, 1 ) );
                veh.skidding = true;
                veh.velocity = shove_velocity;
                if( shove_destination != tripoint_zero ) {
                    if( shove_destination.z != 0 ) {
                        veh.vertical_velocity = shove_destination.z < 0 ? -shove_velocity : +shove_velocity;
                    }
                    g->m.move_vehicle( veh, shove_destination, veh.face );
                }
                veh.move = tileray( point( destination_delta_x, destination_delta_y ) );
                veh.smash( g->m, shove_damage_min, shove_damage_max, 0.10F );
            }
        }
    }
}
