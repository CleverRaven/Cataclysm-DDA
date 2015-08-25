// Monster movement code; essentially, the AI

#include "monster.h"
#include "map.h"
#include "map_iterator.h"
#include "debug.h"
#include "game.h"
#include "line.h"
#include "rng.h"
#include "pldata.h"
#include "messages.h"
#include "cursesdef.h"
#include "sounds.h"
#include "monattack.h"
#include "monfaction.h"
#include "translations.h"
#include "npc.h"
#include "mapdata.h"
#include "mtype.h"
#include "field.h"

#include <stdlib.h>
//Used for e^(x) functions
#include <stdio.h>
#include <math.h>

#define MONSTER_FOLLOW_DIST 8

bool monster::wander()
{
    return ( plans.empty() );
}

bool monster::can_move_to( const tripoint &p ) const
{
    const bool can_climb = has_flag( MF_CLIMBS ) || has_flag( MF_FLIES );
    if( g->m.move_cost( p ) == 0 && !( can_climb && g->m.has_flag( "CLIMBABLE", p ) ) ) {
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

    // various animal behaviours
    if( has_flag( MF_ANIMAL ) ) {
        // don't enter sharp terrain unless tiny, or attacking
        if( g->m.has_flag( "SHARP", p ) && !( attitude( &( g->u ) ) == MATT_ATTACK || type->size == MS_TINY || has_flag( MF_FLIES )) ) {
            return false;
        }

        // Don't enter open pits ever unless tiny, can fly or climb well
        if( !( type->size == MS_TINY || can_climb ) &&
            ( g->m.ter( p ) == t_pit || g->m.ter( p ) == t_pit_spiked || g->m.ter( p ) == t_pit_glass ) ) {
            return false;
        }

        // don't enter lava ever
        if( g->m.ter( p ) == t_lava ) {
            return false;
        }

        // don't enter fire or electricity ever
        const field &local_field = g->m.field_at( p );
        if( local_field.findField( fd_fire ) || local_field.findField( fd_electricity ) ) {
            return false;
        }

        if( g->m.has_flag( TFLAG_NO_FLOOR, p ) && !has_flag( MF_FLIES ) ) {
            return false;
        }
    }

    return true;
}

// Resets plans (list of squares to visit) and builds it as a straight line
// to the destination p. t is used to choose which eligible line to use.
// Currently, this assumes we can see p, so shouldn't be used in any other
// circumstance (or else the monster will "phase" through solid terrain!)
void monster::set_dest( const tripoint &p )
{
    plans.clear();
    plans = g->m.find_clear_path( pos(), p );
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
    const int d = rl_dist( pos3(), c.pos3() );
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
    bool electronic = has_flag( MF_ELECTRONIC );
    Creature *target = nullptr;
    // 8.6f is rating for tank drone 60 tiles away, moose 16 or boomer 33
    float dist = !electronic ? 1000 : 8.6f;
    bool fleeing = false;
    bool docile = has_flag( MF_VERMIN ) || ( friendly != 0 && has_effect( "docile" ) );
    bool angers_hostile_weak = type->anger.find( MTRIG_HOSTILE_WEAK ) != type->anger.end();
    int angers_hostile_near = ( type->anger.find( MTRIG_HOSTILE_CLOSE ) != type->anger.end() ) ? 5 : 0;
    int fears_hostile_near = ( type->fear.find( MTRIG_HOSTILE_CLOSE ) != type->fear.end() ) ? 5 : 0;
    bool group_morale = has_flag( MF_GROUP_MORALE ) && morale < type->morale;
    bool swarms = has_flag( MF_SWARMS );
    auto mood = attitude();

    // If we can see the player, move toward them or flee.
    if( friendly == 0 && sees( g->u ) ) {
        dist = rate_target( g->u, dist, electronic );
        fleeing = fleeing || is_fleeing( g->u );
        target = &g->u;
        if( dist <= 5 ) {
            anger += angers_hostile_near;
            morale -= fears_hostile_near;
        }
    } else if( friendly != 0 && !docile ) {
        // Target unfriendly monsters, only if we aren't interacting with the player.
        for( int i = 0, numz = g->num_zombies(); i < numz; i++ ) {
            monster &tmp = g->zombie( i );
            if( tmp.friendly == 0 ) {
                float rating = rate_target( tmp, dist, electronic );
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

    for( size_t i = 0; i < g->active_npc.size(); i++ ) {
        npc *me = g->active_npc[i];
        float rating = rate_target( *me, dist, electronic );
        bool fleeing_from = is_fleeing( *me );
        // Switch targets if closer and hostile or scarier than current target
        if( ( rating < dist && fleeing ) ||
            ( rating < dist && attitude( me ) == MATT_ATTACK ) ||
            ( !fleeing && fleeing_from ) ) {
            target = me;
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

            for( int i : fac.second ) { // mon indices
                monster &mon = g->zombie( i );
                float rating = rate_target( mon, dist, electronic );
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
                                    << actual_faction.id().str() << " which wasn't loaded in game::monmove";
        swarms = false;
        group_morale = false;
    }
    swarms = swarms && target == nullptr; // Only swarm if we have no target
    if( group_morale || swarms ) {
        for( const int i : myfaction_iter->second ) {
            monster &mon = g->zombie( i );
            float rating = rate_target( mon, dist, electronic );
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
        if( rl_dist( pos3(), g->u.pos3() ) > 2 ) {
            set_dest( g->u.pos3() );
        } else {
            plans.clear();
        }
    }
    // If we're not adjacent to the start of our plan path, don't act on it.
    // This is to catch when we had pre-existing invalid plans and
    // made it through the function without changing them.
    if( !plans.empty() && square_dist( pos3(), plans.front() ) > 1 ) {
        plans.clear();
    }
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
    if( !is_hallucination() && has_flag( MF_ABSORBS ) && !g->m.has_flag( TFLAG_SEALED, pos() ) ) {
        if( !g->m.i_at( pos3() ).empty() ) {
            add_msg( _( "The %s flows around the objects on the floor and they are quickly dissolved!" ),
                     name().c_str() );
            for( auto &elem : g->m.i_at( pos3() ) ) {
                hp += elem.volume(); // Yeah this means it can get more HP than normal.
            }
            g->m.i_clear( pos3() );
        }
    }

    static const std::string pacified_string = "pacified";
    const bool pacified = has_effect( pacified_string );

    // First, use the special attack, if we can!
    for( size_t i = 0; i < sp_timeout.size(); ++i ) {
        if( sp_timeout[i] > 0 ) {
            sp_timeout[i]--;
        }

        if( sp_timeout[i] == 0 && !pacified && !is_hallucination() ) {
            type->sp_attack[i]( this, i );
        }
    }

    // The monster can sometimes hang in air due to last fall being blocked
    const bool can_fly = has_flag( MF_FLIES );
    if( !can_fly && g->m.has_flag( TFLAG_NO_FLOOR, pos() ) ) {
        g->m.creature_on_trap( *this, false );
    }

    if( moves < 0 ) {
        return;
    }

    // TODO: Move this to attack_at/move_to/etc. functions
    bool attacking = false;
    if( !move_effects(attacking) ) {
        moves = 0;
        return;
    }
    if( has_flag( MF_IMMOBILE ) ) {
        moves = 0;
        return;
    }
    static const std::string stun_string = "stunned";
    if( has_effect( stun_string ) ) {
        stumble();
        moves = 0;
        return;
    }
    if( friendly != 0 ) {
        if( friendly > 0 ) {
            friendly--;
        }
        friendly_move();
        return;
    }

    bool moved = false;
    tripoint next;

    // Set attitude to attitude to our current target
    monster_attitude current_attitude = attitude( nullptr );
    if( !plans.empty() ) {
        if( plans.back() == g->u.pos3() ) {
            current_attitude = attitude( &( g->u ) );
        } else {
            for( auto &i : g->active_npc ) {
                if( plans.back() == i->pos3() ) {
                    current_attitude = attitude( i );
                }
            }
        }
    }

    if( current_attitude == MATT_IGNORE ||
        ( current_attitude == MATT_FOLLOW && plans.size() <= MONSTER_FOLLOW_DIST ) ) {
        moves -= 100;
        stumble();
        return;
    }

    // Fix possibly invalid plans
    // Also make sure the monster won't act across z-levels when it shouldn't.
    // Don't do it in plan(), because the mon can still use ranged special attacks using
    // the plans that are not valid for travel/melee.
    const bool can_bash = has_flag( MF_BASHES ) || has_flag( MF_BORES );
    if( !plans.empty() && !g->m.valid_move( pos(), plans[0], can_bash, can_fly ) ) {
        plans.clear();
    }

    // CONCRETE PLANS - Most likely based on sight
    if( !plans.empty() ) {
        next = plans[0];
        moved = true;
    }
    if( !moved && has_flag( MF_SMELLS ) ) {
        // No sight... or our plans are invalid (e.g. moving through a transparent, but
        //  solid, square of terrain).  Fall back to smell if we have it.
        plans.clear();
        tripoint tmp = scent_move();
        if( tmp.x != -1 ) {
            next = tmp;
            moved = true;
        }
    }
    if( wandf > 0 && !moved ) { // No LOS, no scent, so as a fall-back follow sound
        plans.clear();
        tripoint tmp = wander_next();
        if( tmp != pos() ) {
            next = tmp;
            moved = true;
        }
    }

    if( moved ) {
        const Creature *target = g->critter_at( next, is_hallucination() );
        // When attacking an adjacent enemy, we're direct.
        if( target != nullptr && attitude_to( *target ) == A_HOSTILE ) {
            // No change, stick with the plan.
        } else {
            // If the direct path is blocked, go around.
            int switch_chance = 1;
            std::vector<point> squares = squares_in_direction( pos().x, pos().y, next.x, next.y );
            for( point &square : squares ) {
                const tripoint candidate = { square.x, square.y, pos().z };
                // Bail out if the move doesn't get us closer to the target.
                if( !plans.empty() && rl_dist( candidate, plans.back() ) >=
                    rl_dist( pos(), plans.back() ) ) {
                    continue;
                }
                // Bail out if we can't move there and we can't bash.
                if( !can_move_to( candidate ) &&
                    !(can_bash && g->m.bash_rating( bash_estimate(), candidate ) >= 0 ) ) {
                    continue;
                }
                const Creature *target = g->critter_at( candidate, is_hallucination() );
                // Bail out if there's a non-hostile monster in the way and we're not pushy.
                if( target != nullptr && attitude_to( *target ) == A_HOSTILE &&
                    !has_flag( MF_ATTACKMON ) && !has_flag( MF_PUSH_MON ) ) {
                    continue;
                }
                // Randomly pick one of the viable squares to move to.
                if( one_in(switch_chance) ) {
                    next = candidate;
                    switch_chance++;
                    // If we stumble, pick a random square, otherwise take the first one,
                    // which is the most direct path.
                    if( !has_flag( MF_STUMBLES ) ) {
                        break;
                    }
                }
            }
        }
    }
    // Finished logic section.  By this point, we should have chosen a square to
    //  move to (moved = true).
    if( moved ) { // Actual effects of moving to the square we've chosen
        const bool did_something =
            ( !pacified && attack_at( next ) ) ||
            ( !pacified && bash_at( next ) ) ||
            ( !pacified && push_to( next, 0, 0 ) ) ||
            move_to( next );
        if( !did_something ) {
            moves -= 100; // If we don't do this, we'll get infinite loops.
        }
    } else {
        moves -= 100;
    }
    // Stumble around if we're not moving
    if( !moved ) {
        stumble();
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

/*
Function: friendly_move
The per-turn movement and action calculation of any friendly monsters.
*/
void monster::friendly_move()
{
    tripoint p;
    bool moved = false;
    //If we successfully calculated a plan in the generic monster movement function, begin executing it.
    if( !plans.empty() && ( plans[0] != g->u.pos3() ) &&
        ( can_move_to( plans[0] ) ||
          ( ( has_flag( MF_BASHES ) || has_flag( MF_BORES ) ) &&
            g->m.bash_rating( bash_estimate(), plans[0] ) >= 0 ) ) ) {
        p = plans[0];
        plans.erase( plans.begin() );
        moved = true;
    } else {
        //Otherwise just stumble around randomly until we formulate a plan.
        moves -= 100;
        stumble();
    }
    if( moved ) {
        const bool pacified = has_effect( "pacified" );
        const bool did_something =
            ( !pacified && attack_at( p ) ) ||
            ( !pacified && bash_at( p ) ) ||
            move_to( p );

        //If all else fails in our plan (an issue with pathfinding maybe) stumble around instead.
        if( !did_something ) {
            stumble();
            moves -= 100;
        }
    }
}

tripoint monster::scent_move()
{
    std::vector<tripoint> smoves;

    int bestsmell = 10; // Squares with smell 0 are not eligible targets.
    int smell_threshold = 200; // Squares at or above this level are ineligible.
    if( has_flag( MF_KEENNOSE ) ) {
        bestsmell = 1;
        smell_threshold = 400;
    }

    const bool fleeing = is_fleeing( g->u );
    if( fleeing ) {
        bestsmell = g->scent( pos() );
    }

    tripoint next( -1, -1, posz() );
    if( ( !fleeing && g->scent( pos() ) > smell_threshold ) ||
        ( fleeing && bestsmell == 0 ) ) {
        return next;
    }
    const bool can_bash = has_flag( MF_BASHES ) || has_flag( MF_BORES );
    for( const auto &dest : g->m.points_in_radius( pos(), 1 ) ) {
        int smell = g->scent( dest );
        if( ( can_move_to( dest ) || ( dest == g->u.pos3() ) ||
              ( can_bash && g->m.bash_rating( bash_estimate(), dest ) >= 0 ) ) ) {
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

tripoint monster::wander_next()
{
    tripoint next = pos();
    bool xbest = true;
    if( abs( wander_pos.y - posy() ) > abs( wander_pos.x - posx() ) ) {
        // Which is more important
        xbest = false;
    }

    int x = posx(), x2 = posx() - 1, x3 = posx() + 1;
    int y = posy(), y2 = posy() - 1, y3 = posy() + 1;
    int z = posz();
    // Used to avoid checking same points 3 times when moving in a straight line
    // *_move is true if pos*() != wander_pos.*
    bool x_move = true;
    bool y_move = true;
    bool z_move = true;
    if( wander_pos.x < posx() ) {
        x--;
        x2++;
    } else if( wander_pos.x > posx() ) {
        x++;
        x2++;
        x3 -= 2;
    } else {
        x_move = false;
    }

    if( wander_pos.y < posy() ) {
        y--;
        y2++;
    } else if( wander_pos.y > posy() ) {
        y++;
        y2++;
        y3 -= 2;
    } else {
        y_move = false;
    }

    if( wander_pos.z < posz() ) {
        z--;
    } else if( wander_pos.z > posz() ) {
        z++;
    } else {
        z_move = false;
    }

    if( !x_move && !y_move && !z_move ) {
        return next;
    }

    // Any creature can "fly" downwards
    const bool flies = z < posz() || has_flag( MF_FLIES );
    const bool climbs =  has_flag( MF_CLIMBS );
    const bool canbash = has_flag( MF_BASHES ) || has_flag( MF_BORES );
    const int bash_est = bash_estimate();
    // Check if we can move into position, attack player on position or bash position
    // If yes, set next to this position and return true, otherwise return false
    const auto try_pos = [&]( const int x, const int y, const int z ) {
        tripoint dest( x, y, z );
        if( ( canbash && g->m.bash_rating( bash_est, dest ) > 0 ) ||
            ( ( flies || !g->m.has_flag( TFLAG_NO_FLOOR, dest ) ) && can_move_to( dest ) ) ) {
            next = dest;
            return true;
        }

        return false;
    };

    bool found = false;
    if( z_move && g->m.valid_move( pos(), tripoint( posx(), posy(), z ), false, flies || climbs ) ) {
        found = true;
        if( ( x_move || y_move ) && try_pos( x, y, z ) ) {
        } else if( y_move && try_pos( x, y2, z ) ) {
        } else if( x_move && try_pos( x2, y, z ) ) {
        } else if( y_move && try_pos( x, y3, z ) ) {
        } else if( x_move && try_pos( x3, y, z ) ) {
        } else if( try_pos( posx(), posy(), z ) ) {
        } else {
            found = false;
        }
    }

    if( found ) {
        return next;
    }

    if( xbest ) {
        if( ( x_move || y_move ) && try_pos( x, y, posz() ) ) {
            // Do nothing in each of those ifs, the if-else is just for convenience
        } else if( y_move && try_pos( x, y2, posz() ) ) {
        } else if( x_move && try_pos( x2, y, posz() ) ) {
        } else if( y_move && try_pos( x, y3, posz() ) ) {
        } else if( x_move && try_pos( x3, y, posz() ) ) {
        }
    } else {
        if( ( x_move || y_move ) && try_pos( x, y, posz() ) ) {
        } else if( x_move && try_pos( x2, y, posz() ) ) {
        } else if( y_move && try_pos( x, y2, posz() ) ) {
        } else if( x_move && try_pos( x3, y, posz() ) ) {
        } else if( y_move && try_pos( x, y3, posz() ) ) {
        }
    }

    return next;
}

int monster::calc_movecost( const tripoint &f, const tripoint &t ) const
{
    int movecost = 0;
    float diag_mult = ( trigdist && f.x != t.x && f.y != t.y ) ? 1.41 : 1;

    // Digging and flying monsters ignore terrain cost
    if( has_flag( MF_FLIES ) || ( digging() && g->m.has_flag( "DIGGABLE", t ) ) ) {
        movecost = 100 * diag_mult;
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
        movecost *= diag_mult;
    } else if( can_submerge() ) {
        // No-breathe monsters have to walk underwater slowly
        if( g->m.has_flag( "SWIMMABLE", f ) ) {
            movecost += 150;
        } else {
            movecost += 50 * g->m.move_cost( f );
        }
        if( g->m.has_flag( "SWIMMABLE", t ) ) {
            movecost += 150;
        } else {
            movecost += 50 * g->m.move_cost( t );
        }
        movecost *= diag_mult / 2;
    } else if( has_flag(MF_CLIMBS) ) {
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
        movecost *= diag_mult / 2;
    } else {
        // All others use the same calculation as the player
        movecost = ( g->m.combined_movecost( f, t ) );
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
    bool can_bash = g->m.is_bashable( p ) && ( has_flag( MF_BASHES ) || has_flag( MF_BORES ) );

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
        estimate += 20;
    }
    return estimate;
}

int monster::bash_skill()
{
    int ret = type->melee_dice * type->melee_sides; // IOW, the critter's max bashing damage
    if( has_flag( MF_BORES ) ) {
        ret *= 15; // This is for stuff that goes through solid rock: minerbots, dark wyrms, etc
    } else if( has_flag( MF_DESTROYS ) ) {
        ret *= 2.5;
    }
    return ret;
}

int monster::group_bash_skill( const tripoint &target )
{
    if( !has_flag( MF_GROUP_BASH ) ) {
        return bash_skill();
    }
    int bashskill = 0;

    // pileup = more bashskill, but only help bashing mob directly infront of target
    const int max_helper_depth = 5;
    const std::vector<tripoint> bzone = get_bashing_zone( target, pos3(), max_helper_depth );

    for( const tripoint &candidate : bzone ) {
        // Drawing this line backwards excludes the target and includes the candidate.
        std::vector<tripoint> path_to_target = line_to( target, candidate, 0, 0 );
        bool connected = true;
        int mondex = -1;
        for( const tripoint &in_path : path_to_target ) {
            // If any point in the line from zombie to target is not a cooperating zombie,
            // it can't contribute.
            mondex = g->mon_at( in_path );
            if( mondex == -1 ) {
                connected = false;
                break;
            }
            monster &helpermon = g->zombie( mondex );
            if( !helpermon.has_flag( MF_GROUP_BASH ) || helpermon.is_hallucination() ) {
                connected = false;
                break;
            }
        }
        if( !connected || mondex == -1 ) {
            continue;
        }
        // If we made it here, the last monster checked was the candidate.
        monster &helpermon = g->zombie( mondex );
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

    if( p == g->u.pos3() ) {
        melee_attack( g->u, true );
        return true;
    }

    const int mondex = g->mon_at( p, is_hallucination() );
    if( mondex != -1 ) {
        monster &mon = g->zombie( mondex );

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
            hit_monster( mon );
            return true;
        }

        return false;
    }

    const int npcdex = g->npc_at( p );
    if( npcdex != -1 && type->melee_dice > 0 ) {
        // For now we're always attacking NPCs that are getting into our
        // way. This is consistent with how it worked previously, but
        // later on not hitting allied NPCs would be cool.
        melee_attack( *g->active_npc[npcdex], true );
        return true;
    }

    // Nothing to attack.
    return false;
}

bool monster::move_to( const tripoint &p, bool force )
{
    const bool digs = digging();
    const bool flies = has_flag( MF_FLIES );
    const bool on_ground = !digs && !flies;
    const bool climbs = has_flag( MF_CLIMBS ) && g->m.has_flag( TFLAG_NO_FLOOR, p );
    // Allows climbing monsters to move on terrain with movecost <= 0
    Creature *critter = g->critter_at( p, is_hallucination() );
    if( g->m.has_flag( "CLIMBABLE", p ) ) {
        if( g->m.move_cost( p ) == 0 && critter == nullptr ) {
            if( flies ) {
                moves -= 100;
                force = true;
                if (g->u.sees( *this )){
                    add_msg(_("The %1$s flies over the %2$s."), name().c_str(),
                    g->m.has_flag_furn("CLIMBABLE", p) ? g->m.furnname(p).c_str() : g->m.tername(p).c_str());
                }
            } else if (has_flag(MF_CLIMBS)) {
                moves -= 150;
                force = true;
                if (g->u.sees( *this )){
                    add_msg(_("The %1$s climbs over the %2$s."), name().c_str(),
                    g->m.has_flag_furn("CLIMBABLE", p) ? g->m.furnname(p).c_str() : g->m.tername(p).c_str());
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

    if( !plans.empty() ) {
        plans.erase( plans.begin() );
    }

    if( !force ) {
        const int cost = !climbs ? calc_movecost( pos(), p ) :
                                   calc_climb_cost( pos(), p );
        if( cost > 0 ) {
            moves -= cost;
        } else {
            return false;
        }
    }

    //Check for moving into/out of water
    bool was_water = g->m.is_divable( pos3() );
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
        add_effect( "bouldering", 1, num_bp, true );
    } else if( has_effect( "bouldering" ) ) {
        remove_effect( "bouldering" );
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
        if( has_flag( MF_VERMIN ) ) {
            factor *= 100;
        }
        if( one_in( factor ) ) {
            g->m.ter_set( pos(), t_dirtmound );
        }
    }
    // Acid trail monsters leave... a trail of acid
    if( has_flag( MF_ACIDTRAIL ) ) {
        g->m.add_field( pos(), fd_acid, 3, 0 );
    }

    if( has_flag( MF_SLUDGETRAIL ) ) {
        for( const tripoint &sludge_p : g->m.points_in_radius( pos(), 1 ) ) {
            const int fstr = 3 - ( abs( sludge_p.x - posx() ) + abs( sludge_p.y - posy() ) );
            if( fstr >= 2 ) {
                g->m.add_field( sludge_p, fd_sludge, fstr, 0 );
            }
        }
    }
    if( has_flag( MF_LEAKSGAS ) ) {
        if( one_in( 6 ) ) {
            tripoint dest( posx() + rng( -1, 1 ), posy() + rng( -1, 1 ), posz() );
            g->m.add_field( dest, fd_toxic_gas, 3, 0 );
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

    if( !has_flag( MF_PUSH_MON ) || depth > 2 || has_effect( "pushed" ) ) {
        return false;
    }

    // TODO: Generalize this to Creature
    const int mondex = g->mon_at( p );
    if( mondex < 0 ) {
        return false;
    }

    monster *critter = &g->zombie( mondex );
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
    add_effect( "pushed", 1 );

    for( size_t i = 0; i < 6; i++ ) {
        const int dx = rng( -1, 1 );
        const int dy = rng( -1, 1 );
        if( dx == 0 && dy == 0 ) {
            continue;
        }

        // Pushing forward is easier than pushing aside
        const int direction_penalty = abs( dx - dir.x ) + abs( dy + dir.y );
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
            monster *mon_recur = dynamic_cast< monster* >( critter_recur );
            if( mon_recur == nullptr ) {
                continue;
            }

            if( critter->push_to( dest, roll, depth + 1 ) ) {
                // The tile isn't necessarily free, need to check
                if( g->mon_at( p ) == -1 ) {
                    move_to( p );
                }

                moves -= movecost_attacker;
                if( movecost_from > 100 ) {
                    critter->add_effect( "downed", movecost_from / 100 + 1 );
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
            critter->add_effect( "downed", movecost_from / 100 + 1 );
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
    critter->add_effect( "stunned", rng( 0, 2 ) );
    // Only print the message when near player or it can get spammy
    if( rl_dist( g->u.pos(), pos() ) < 4 && g->u.sees( *critter ) ) {
        add_msg( m_warning, _("The %1$s tramples %2$s"),
                 name().c_str(), critter->disp_name().c_str() );
    }

    moves -= movecost_attacker;
    if( movecost_from > 100 ) {
        critter->add_effect( "downed", movecost_from / 100 + 1 );
    } else {
        critter->moves -= movecost_from;
    }
    
    return true;
}

/**
 * Stumble in a random direction, but with some caveats.
 */
void monster::stumble( )
{
    // Only move every 3rd turn.
    if( !one_in( 3 ) ) {
        return;
    }

    std::vector<tripoint> valid_stumbles;
    const bool avoid_water = has_flag( MF_NO_BREATHE ) &&
      !has_flag( MF_SWIMS ) && !has_flag( MF_AQUATIC );
    for( int i = -1; i <= 1; i++ ) {
        for( int j = -1; j <= 1; j++ ) {
            tripoint dest( posx() + i, posy() + j, posz() );
            if( ( i || j ) && can_move_to( dest ) &&
                //Stop zombies and other non-breathing monsters wandering INTO water
                //(Unless they can swim/are aquatic)
                //But let them wander OUT of water if they are there.
                !( avoid_water &&
                   g->m.has_flag( TFLAG_SWIMMABLE, dest ) &&
                   !g->m.has_flag( TFLAG_SWIMMABLE, pos3() ) ) &&
                ( g->critter_at( dest, is_hallucination() ) == nullptr ) ) {
                valid_stumbles.push_back( dest );
            }
        }
    }

    if( g->m.has_zlevels() ) {
        tripoint below( posx(), posy(), posz() - 1 );
        tripoint above( posx(), posy(), posz() + 1 );
        if( g->m.valid_move( pos(), below, false, true ) && can_move_to( below ) ) {
            valid_stumbles.push_back( below );
        }
        // More restrictions for moving up
        if( one_in( 5 ) && has_flag( MF_FLIES ) &&
            g->m.valid_move( pos(), above, false, true ) && can_move_to( above ) ) {
            valid_stumbles.push_back( above );
        }
    }

    if( valid_stumbles.empty() ) { //nowhere to stumble?
        return;
    }

    move_to( random_entry( valid_stumbles ), false );

    // Here we have to fix our plans[] list,
    // acquiring a new path to the previous target.
    // target == either end of current plan, or the player.
    if( !plans.empty() ) {
        if( g->m.sees( pos3(), plans.back(), -1 ) ) {
            set_dest( plans.back() );
        } else if( sees( g->u ) ) {
            set_dest( g->u.pos() );
        } else { //durr, i'm suddenly calm. what was i doing?
            plans.clear();
        }
    }
}

void monster::knock_back_from( const tripoint &p )
{
    if( p == pos3() ) {
        return; // No effect
    }
    if( is_hallucination() ) {
        die( nullptr );
        return;
    }
    tripoint to = pos3();;
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
    int mondex = g->mon_at( to );
    if( mondex != -1 ) {
        monster *z = &( g->zombie( mondex ) );
        apply_damage( z, bp_torso, z->type->size );
        add_effect( "stunned", 1 );
        if( type->size > 1 + z->type->size ) {
            z->knock_back_from( pos3() ); // Chain reaction!
            z->apply_damage( this, bp_torso, type->size );
            z->add_effect( "stunned", 1 );
        } else if( type->size > z->type->size ) {
            z->apply_damage( this, bp_torso, type->size );
            z->add_effect( "stunned", 1 );
        }
        z->check_dead_state();

        if( u_see ) {
            add_msg( _( "The %1$s bounces off a %2$s!" ), name().c_str(), z->name().c_str() );
        }

        return;
    }

    int npcdex = g->npc_at( to );
    if( npcdex != -1 ) {
        npc *p = g->active_npc[npcdex];
        apply_damage( p, bp_torso, 3 );
        add_effect( "stunned", 1 );
        p->deal_damage( this, bp_torso, damage_instance( DT_BASH, type->size ) );
        if( u_see ) {
            add_msg( _( "The %1$s bounces off %2$s!" ), name().c_str(), p->name.c_str() );
        }

        p->check_dead_state();
        return;
    }

    // If we're still in the function at this point, we're actually moving a tile!
    if( g->m.ter_at( to ).has_flag( TFLAG_DEEP_WATER ) ) {
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

    if( g->m.move_cost( to ) == 0 ) {

        // It's some kind of wall.
        apply_damage( nullptr, bp_torso, type->size );
        add_effect( "stunned", 2 );
        if( u_see ) {
            add_msg( _( "The %1$s bounces off a %2$s." ), name().c_str(),
                     g->m.tername( to ).c_str() );
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
         Injure monsters if they're gonna be walking through pits or whatevs
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

    std::vector<tripoint> path = g->m.route( pos(), tripoint(x, y, posz()), 0, 100 );
    if( path.empty() ) {
        return false;
    }

    if( has_flag( MF_SMELLS ) && g->scent( pos3() ) > 0 &&
        g->scent( { x, y, posz() } ) > g->scent( pos3() ) ) {
        return true;
    }

    if( can_hear() && wandf > 0 && rl_dist( wander_pos.x, wander_pos.y, x, y ) <= 2 &&
        rl_dist( posx(), posy(), wander_pos.x, wander_pos.y ) <= wandf ) {
        return true;
    }

    if( can_see() && g->m.sees( posx(), posy(), x, y, g->light_level() ) ) {
        return true;
    }

    return false;
}

int monster::turns_to_reach( int x, int y )
{
    // This function is a(n old) temporary hack that should soon be removed
    std::vector<tripoint> path = g->m.route( pos(), tripoint(x, y, posz()), 0, 100 );
    if( path.empty() ) {
        return 999;
    }

    double turns = 0.;
    for( size_t i = 0; i < path.size(); i++ ) {
        const tripoint &next = path[i];
        if( g->m.move_cost( next ) == 0 ) {
            // No bashing through, it looks stupid when you go back and find
            // the doors intact.
            return 999;
        } else if( i == 0 ) {
            turns += double( calc_movecost( pos3(), next ) ) / get_speed();
        } else {
            turns += double( calc_movecost( path[i - 1], next ) ) / get_speed();
        }
    }

    return int( turns + .9 ); // Halve (to get turns) and round up
}
