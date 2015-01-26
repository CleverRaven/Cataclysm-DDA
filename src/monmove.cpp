// Monster movement code; essentially, the AI

#include "monster.h"
#include "map.h"
#include "game.h"
#include "line.h"
#include "rng.h"
#include "pldata.h"
#include "messages.h"
#include <stdlib.h>
#include "cursesdef.h"

//Used for e^(x) functions
#include <stdio.h>
#include <math.h>

#define MONSTER_FOLLOW_DIST 8

bool monster::wander()
{
 return (plans.empty());
}

bool monster::can_move_to(int x, int y) const
{
    if (g->m.move_cost(x, y) == 0) {
        return false;
    }
    if (!can_submerge() && g->m.has_flag(TFLAG_DEEP_WATER, x, y)) {
        return false;
    }
    if (has_flag(MF_DIGS) && !g->m.has_flag("DIGGABLE", x, y)) {
        return false;
    }
    if (has_flag(MF_AQUATIC) && !g->m.has_flag("SWIMMABLE", x, y)) {
        return false;
    }

    if (has_flag(MF_SUNDEATH) && g->is_in_sunlight(x, y)) {
        return false;
    }

    // various animal behaviours
    if (has_flag(MF_ANIMAL))
    {
        // don't enter sharp terrain unless tiny, or attacking
        if (g->m.has_flag("SHARP", x, y) && !(attitude(&(g->u)) == MATT_ATTACK ||
                                              type->size == MS_TINY))
            return false;

        // don't enter open pits ever unless tiny or can fly
        if (!(type->size == MS_TINY || has_flag(MF_FLIES)) &&
            (g->m.ter(x, y) == t_pit || g->m.ter(x, y) == t_pit_spiked || g->m.ter(x, y) == t_pit_glass))
            return false;

        // don't enter lava ever
        if (g->m.ter(x, y) == t_lava)
            return false;

        // don't enter fire or electricity ever
        const field &local_field = g->m.field_at(x, y);
        if (local_field.findField(fd_fire) || local_field.findField(fd_electricity))
            return false;
    }
    return true;
}

// Resets plans (list of squares to visit) and builds it as a straight line
// to the destination (x,y). t is used to choose which eligible line to use.
// Currently, this assumes we can see (x,y), so shouldn't be used in any other
// circumstance (or else the monster will "phase" through solid terrain!)
void monster::set_dest(int x, int y, int &t)
{
 plans.clear();
// TODO: This causes a segfault, once in a blue moon!  Whyyyyy.
 plans = line_to(position, point(x, y), t);
}

// Move towards (x,y) for f more turns--generally if we hear a sound there
// "Stupid" movement; "if (wandx < posx) posx--;" etc.
void monster::wander_to(int x, int y, int f)
{
 wandx = x;
 wandy = y;
 wandf = f;
}

float monster::rate_target( Creature &c, int &bresenham_slope, bool smart ) const
{
    int d = rl_dist( pos(), c.pos() );
    // No targetting own tile. Creatures sharing tiles can happen.
    if( d <= 0 || !sees( c, bresenham_slope ) ) {
        return INT_MAX;
    }
    if( !smart ) {
        return d;
    }
    float power = c.power_rating();
    monster *mon = dynamic_cast< monster* >( &c );
    // Their attitude to us and not ours to them, so that bobcats won't get gunned down
    if( mon != nullptr && mon->attitude_to( *this ) == Attitude::A_HOSTILE ) {
        power += 2;
    }
    if( power > 0 ) {
        return d / power;
    }
    return INT_MAX;
}

void monster::plan(const mfactions &factions)
{
    // Bots are more intelligent than most living stuff
    bool electronic = has_flag( MF_ELECTRONIC );
    Creature *target = nullptr;
    // 8.6f is rating for tank drone 60 tiles away, moose 16 or boomer 33
    float dist = !electronic ? 1000 : 8.6f;
    int bresenham_slope = 0;
    int selected_slope = 0;
    bool fleeing = false;
    bool docile = has_flag( MF_VERMIN ) || ( friendly != 0 && has_effect( "docile" ) );
    bool angers_hostile_near = type->anger.find( MTRIG_HOSTILE_CLOSE ) != type->anger.end();
    bool angers_hostile_weak = type->anger.find( MTRIG_HOSTILE_WEAK ) != type->anger.end();
    bool group_morale = has_flag( MF_GROUP_MORALE ) && morale < type->morale;
    bool swarms = has_flag( MF_SWARMS );
    auto mood = attitude();

    if( friendly != 0 && !docile ) { // Target unfriendly monsters
        for( int i = 0, numz = g->num_zombies(); i < numz; i++ ) {
            monster &tmp = g->zombie( i );
            if( tmp.friendly == 0 ) {
                float rating = rate_target( tmp, bresenham_slope, electronic );
                if( rating < dist ) {
                    target = &tmp;
                    dist = rating;
                    selected_slope = bresenham_slope;
                }
            }
        }
    }

    // If we can see the player, move toward them or flee.
    if( friendly == 0 && sees( g->u, bresenham_slope ) ) {
        dist = rate_target( g->u, bresenham_slope, electronic );
        fleeing = fleeing || is_fleeing( g->u );
        target = &g->u;
        selected_slope = bresenham_slope;
        if( angers_hostile_near && dist <= 5 ) {
            anger += 5;
        }
    }

    if( !docile ) {
        for( size_t i = 0; i < g->active_npc.size(); i++ ) {
            npc *me = g->active_npc[i];
            float rating = rate_target( *me, bresenham_slope, electronic );
            bool fleeing_from = is_fleeing( *me );
            // Switch targets if closer and hostile or scarier than current target
            if( ( rating < dist && fleeing ) ||
                ( rating < dist && attitude( me ) == MATT_ATTACK ) ||
                ( !fleeing && fleeing_from ) ) {
                    target = me;
                    dist = rating;
                    selected_slope = bresenham_slope;
            }
            fleeing = fleeing || fleeing_from;
            if( angers_hostile_near && rating <= 5 ) {
                anger += 5;
            }
        }
    }

    fleeing = fleeing || ( mood == MATT_FLEE );
    if( friendly == 0 && !docile ) {
        for( const auto &fac : factions ) {
            auto faction_att = faction->attitude( fac.first );
            if( faction_att == MFA_NEUTRAL || faction_att == MFA_FRIENDLY ) {
                continue;
            }

            for( int i : fac.second ) { // mon indices
                monster &mon = g->zombie( i );
                float rating = rate_target( mon, bresenham_slope, electronic );
                if( rating < dist ) {
                    target = &mon;
                    dist = rating;
                    selected_slope = bresenham_slope;
                }
                if( angers_hostile_near && rating <= 5 ) {
                    anger += 5;
                }
            }
        }
    }

    // Friendly monsters here
    // Avoid for hordes of same-faction stuff or it could get expensive
    const monfaction *actual_faction = friendly == 0 ? faction : GetMFact( "player" );
    auto myfaction = factions.find( actual_faction )->second;
    swarms = swarms && target == nullptr; // Only swarm if we have no target
    if( group_morale || swarms ) {
        for( int i : myfaction ) {
            monster &mon = g->zombie( i );
            float rating = rate_target( mon, bresenham_slope, electronic );
            if( group_morale && rating <= 10 ) {
                morale += 10 - rating;
            }
            if( swarms ) {
                if( rating < 5 ) { // Too crowded here
                    wandx = posx() * rng( 1, 3 ) - mon.posx();
                    wandy = posy() * rng( 1, 3 ) - mon.posy();
                    wandf = 2;
                    target = nullptr;
                    // Swarm to the furthest ally you can see
                } else if( rating < INT_MAX && rating > dist && wandf <= 0 ) {
                    target = &mon;
                    dist = rating;
                    selected_slope = bresenham_slope;
                }
            }
        }
    }

    if( target != nullptr ) {
        if( one_in( 2 ) ) { // Random for the diversity of the trajectory
            ++selected_slope;
        } else {
            --selected_slope;
        }

        point dest = target->pos();
        auto att_to_target = attitude_to( *target );
        if( att_to_target == Attitude::A_HOSTILE && !fleeing ) {
            set_dest( dest.x, dest.y, selected_slope );
        } else if( fleeing ) {
            set_dest( posx() * 2 - dest.x, posy() * 2 - dest.y, selected_slope );
        }
        if( angers_hostile_weak && att_to_target != Attitude::A_FRIENDLY ) {
            int hp_per = target->hp_percentage();
            if( hp_per <= 70 ) {
                anger += 10 - int( hp_per / 10 );
            }
        }
    } else if( friendly > 0 && one_in(3)) {
            // Grow restless with no targets
            friendly--;
    } else if( friendly < 0 && sees( g->u, bresenham_slope ) ) {
        if( rl_dist( pos(), g->u.pos() ) > 2 ) {
            set_dest(g->u.posx(), g->u.posy(), bresenham_slope);
        } else {
            plans.clear();
        }
    }
    // If we're not adjacent to the start of our plan path, don't act on it.
    // This is to catch when we had pre-existing invalid plans and
    // made it through the function without changing them.
    if( !plans.empty() && square_dist(pos().x, pos().y,
                                      plans.front().x, plans.front().y ) > 1 ) {
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
    if (wandf > 0) {
        wandf--;
    }

    //Hallucinations have a chance of disappearing each turn
    if (is_hallucination() && one_in(25)) {
        die( nullptr );
        return;
    }

    //The monster can consume objects it stands on. Check if there are any.
    //If there are. Consume them.
    if( !is_hallucination() && has_flag( MF_ABSORBS ) ) {
        if(!g->m.i_at(posx(), posy()).empty()) {
            add_msg(_("The %s flows around the objects on the floor and they are quickly dissolved!"),
                    name().c_str());
            for( auto &elem : g->m.i_at(posx(), posy()) ) {
                hp += elem.volume(); // Yeah this means it can get more HP than normal.
            }
            g->m.i_clear(posx(), posy());
        }
    }

    // First, use the special attack, if we can!
    for (size_t i = 0; i < sp_timeout.size(); ++i) {
        if (sp_timeout[i] > 0) {
            sp_timeout[i]--;
        }

        if( sp_timeout[i] == 0 && !has_effect("pacified") && !is_hallucination() ) {
            mattack ma;
            (ma.*type->sp_attack[i])(this, i);
        }
    }
    if (moves < 0) {
        return;
    }
    if (!move_effects()) {
        moves = 0;
        return;
    }
    if (has_flag(MF_IMMOBILE)) {
        moves = 0;
        return;
    }
    if (has_effect("stunned")) {
        stumble(false);
        moves = 0;
        return;
    }
    if (friendly != 0) {
        if (friendly > 0) {
            friendly--;
        }
        friendly_move();
        return;
    }

    bool moved = false;
    point next;

    // Set attitude to attitude to our current target
    monster_attitude current_attitude = attitude( nullptr );
    if( !plans.empty() ) {
        if (plans.back().x == g->u.posx() && plans.back().y == g->u.posy()) {
            current_attitude = attitude( &(g->u) );
        } else {
            for( auto &i : g->active_npc ) {
                if( plans.back().x == i->posx() && plans.back().y == i->posy() ) {
                    current_attitude = attitude( i );
                }
            }
        }
    }

    if( current_attitude == MATT_IGNORE ||
        (current_attitude == MATT_FOLLOW && plans.size() <= MONSTER_FOLLOW_DIST) ) {
        moves -= 100;
        stumble(false);
        return;
    }

    int mondex = !plans.empty() ? g->mon_at( plans[0].x, plans[0].y ) : -1;
    auto mon_att = mondex != -1 ? attitude_to( g->zombie( mondex ) ) : A_HOSTILE;

    if( !plans.empty() &&
        ( mon_att == A_HOSTILE || has_flag(MF_ATTACKMON) ) &&
        ( can_move_to( plans[0].x, plans[0].y ) ||
          ( plans[0].x == g->u.posx() && plans[0].y == g->u.posy() ) ||
          ( ( has_flag( MF_BASHES ) || has_flag( MF_BORES ) ) &&
          g->m.bash_rating( bash_estimate(), plans[0].x, plans[0].y) >= 0 ) ) ) {
        // CONCRETE PLANS - Most likely based on sight
        next = plans[0];
        moved = true;
    } else if (has_flag(MF_SMELLS)) {
        // No sight... or our plans are invalid (e.g. moving through a transparent, but
        //  solid, square of terrain).  Fall back to smell if we have it.
        plans.clear();
        point tmp = scent_move();
        if (tmp.x != -1) {
            next = tmp;
            moved = true;
        }
    }
    if (wandf > 0 && !moved) { // No LOS, no scent, so as a fall-back follow sound
        plans.clear();
        point tmp = wander_next();
        if (tmp.x != posx() || tmp.y != posy()) {
            next = tmp;
            moved = true;
        }
    }

    // Finished logic section.  By this point, we should have chosen a square to
    //  move to (moved = true).
    if (moved) { // Actual effects of moving to the square we've chosen
        // Note: The below works because C++ in A() || B() won't call B() if A() is true
        int& x = next.x; int& y = next.y; // Define alias for x and y
        bool did_something = attack_at(x, y) || bash_at(x, y) || move_to(x, y);
        if(!did_something) {
            moves -= 100; // If we don't do this, we'll get infinite loops.
        }
    } else {
        moves -= 100;
    }

    // If we're close to our target, we get focused and don't stumble
    if ((has_flag(MF_STUMBLES) && (plans.size() > 3 || plans.empty())) ||
          !moved) {
        stumble(moved);
    }
}

// footsteps will determine how loud a monster's normal movement is
// and create a sound in the monsters location when they move
void monster::footsteps(int x, int y)
{
 if (made_footstep)
  return;
 if (has_flag(MF_FLIES))
  return; // Flying monsters don't have footsteps!
 made_footstep = true;
 int volume = 6; // same as player's footsteps
 if (digging())
  volume = 10;
 switch (type->size) {
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
  default: break;
 }
 int dist = rl_dist(x, y, g->u.posx(), g->u.posy());
 g->add_footstep(x, y, volume, dist, this);
 return;
}

/*
Function: friendly_move
The per-turn movement and action calculation of any friendly monsters.
*/
void monster::friendly_move()
{
    point next;
    bool moved = false;
    //If we successfully calculated a plan in the generic monster movement function, begin executing it.
    if (!plans.empty() && (plans[0].x != g->u.posx() || plans[0].y != g->u.posy()) &&
        (can_move_to(plans[0].x, plans[0].y) ||
         ((has_flag(MF_BASHES) || has_flag(MF_BORES)) &&
          g->m.bash_rating(bash_estimate(), plans[0].x, plans[0].y) >= 0))) {
        next = plans[0];
        plans.erase(plans.begin());
        moved = true;
    } else {
        //Otherwise just stumble around randomly until we formulate a plan.
        moves -= 100;
        stumble(moved);
    }
    if (moved) {
        int& x = next.x; int& y = next.y; // Define alias for x and y
        bool did_something = attack_at(x, y) || bash_at(x, y) || move_to(x, y);

        //If all else fails in our plan (an issue with pathfinding maybe) stumble around instead.
        if(!did_something) {
            stumble(moved);
            moves -= 100;
        }
    }
}

point monster::scent_move()
{
    std::vector<point> smoves;

    int maxsmell = 10; // Squares with smell 0 are not eligible targets.
    int smell_threshold = 200; // Squares at or above this level are ineligible.
    if (has_flag(MF_KEENNOSE)) {
        maxsmell = 1;
        smell_threshold = 400;
    }
    int minsmell = 9999;
    point pbuff, next(-1, -1);
    int smell;
    const bool fleeing = is_fleeing(g->u);
    if( !fleeing && g->scent( posx(), posy() ) > smell_threshold ) {
        return next;
    }
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            const int nx = posx() + x;
            const int ny = posy() + y;
            smell = g->scent(nx, ny);
            int mon = g->mon_at(nx, ny);
            if( (mon == -1 || g->zombie(mon).friendly != 0 || has_flag(MF_ATTACKMON)) &&
                (can_move_to(nx, ny) || (nx == g->u.posx() && ny == g->u.posy()) ||
                 ((has_flag(MF_BASHES) || has_flag(MF_BORES)) &&
                 g->m.bash_rating(bash_estimate(), nx, ny) >= 0))) {
                if ((!fleeing && smell > maxsmell) || (fleeing && smell < minsmell)) {
                    smoves.clear();
                    pbuff.x = nx;
                    pbuff.y = ny;
                    smoves.push_back(pbuff);
                    maxsmell = smell;
                    minsmell = smell;
                } else if ((!fleeing && smell == maxsmell) || (fleeing && smell == minsmell)) {
                    pbuff.x = nx;
                    pbuff.y = ny;
                    smoves.push_back(pbuff);
                }
            }
        }
    }
    if (!smoves.empty()) {
        int nextsq = rng(0, smoves.size() - 1);
        next = smoves[nextsq];
    }
    return next;
}

point monster::wander_next()
{
    point next;
    bool xbest = true;
    if (abs(wandy - posy()) > abs(wandx - posx())) {// which is more important
        xbest = false;
    }
    next.x = posx();
    next.y = posy();
    int x = posx(), x2 = posx() - 1, x3 = posx() + 1;
    int y = posy(), y2 = posy() - 1, y3 = posy() + 1;
    if (wandx < posx()) {
        x--; x2++;
    }
    if (wandx > posx()) {
        x++; x2++; x3 -= 2;
    }
    if (wandy < posy()) {
        y--; y2++;
    }
    if (wandy > posy()) {
        y++; y2++; y3 -= 2;
    }

    if (xbest) {
        if (can_move_to(x, y) || (x == g->u.posx() && y == g->u.posy()) ||
            ((has_flag(MF_BASHES) || has_flag(MF_BORES)) &&
             g->m.bash_rating(bash_estimate(), x, y) >= 0)) {
            next.x = x;
            next.y = y;
        } else if (can_move_to(x, y2) || (x == g->u.posx() && y == g->u.posy()) ||
                   ((has_flag(MF_BASHES) || has_flag(MF_BORES)) &&
                    g->m.bash_rating(bash_estimate(), x, y2) >= 0)) {
            next.x = x;
            next.y = y2;
        } else if (can_move_to(x2, y) || (x == g->u.posx() && y == g->u.posy()) ||
                   ((has_flag(MF_BASHES) || has_flag(MF_BORES)) &&
                    g->m.bash_rating(bash_estimate(), x2, y) >= 0)) {
            next.x = x2;
            next.y = y;
        } else if (can_move_to(x, y3) || (x == g->u.posx() && y == g->u.posy()) ||
                   ((has_flag(MF_BASHES) || has_flag(MF_BORES)) &&
                    g->m.bash_rating(bash_estimate(), x, y3) >= 0)) {
            next.x = x;
            next.y = y3;
        } else if (can_move_to(x3, y) || (x == g->u.posx() && y == g->u.posy()) ||
                   ((has_flag(MF_BASHES) || has_flag(MF_BORES)) &&
                    g->m.bash_rating(bash_estimate(), x3, y) >= 0)) {
            next.x = x3;
            next.y = y;
        }
    } else {
        if (can_move_to(x, y) || (x == g->u.posx() && y == g->u.posy()) ||
            ((has_flag(MF_BASHES) || has_flag(MF_BORES)) &&
             g->m.bash_rating(bash_estimate(), x, y) >= 0)) {
            next.x = x;
            next.y = y;
        } else if (can_move_to(x2, y) || (x == g->u.posx() && y == g->u.posy()) ||
                   ((has_flag(MF_BASHES) || has_flag(MF_BORES)) &&
                    g->m.bash_rating(bash_estimate(), x2, y) >= 0)) {
            next.x = x2;
            next.y = y;
        } else if (can_move_to(x, y2) || (x == g->u.posx() && y == g->u.posy()) ||
                   ((has_flag(MF_BASHES) || has_flag(MF_BORES)) &&
                    g->m.bash_rating(bash_estimate(), x, y2) >= 0)) {
            next.x = x;
            next.y = y2;
        } else if (can_move_to(x3, y) || (x == g->u.posx() && y == g->u.posy()) ||
                   ((has_flag(MF_BASHES) || has_flag(MF_BORES)) &&
                    g->m.bash_rating(bash_estimate(), x3, y) >= 0)) {
            next.x = x3;
            next.y = y;
        } else if (can_move_to(x, y3) || (x == g->u.posx() && y == g->u.posy()) ||
                   ((has_flag(MF_BASHES) || has_flag(MF_BORES)) &&
                    g->m.bash_rating(bash_estimate(), x, y3) >= 0)) {
            next.x = x;
            next.y = y3;
        }
    }
    return next;
}

int monster::calc_movecost(int x1, int y1, int x2, int y2) const
{
    int movecost = 0;
    float diag_mult = (trigdist && x1 != x2 && y1 != y2) ? 1.41 : 1;

    // Digging and flying monsters ignore terrain cost
    if (has_flag(MF_FLIES) || (digging() && g->m.has_flag("DIGGABLE", x2, y2))) {
        movecost = 100 * diag_mult;
    // Swimming monsters move super fast in water
    } else if (has_flag(MF_SWIMS)) {
        if (g->m.has_flag("SWIMMABLE", x1, y1)) {
            movecost += 25;
        } else {
            movecost += 50 * g->m.move_cost(x1, y1);
        }
        if (g->m.has_flag("SWIMMABLE", x2, y2)) {
            movecost += 25;
        } else {
            movecost += 50 * g->m.move_cost(x2, y2);
        }
        movecost *= diag_mult;
    // No-breathe monsters have to walk underwater slowly
    } else if (can_submerge()) {
        if (g->m.has_flag("SWIMMABLE", x1, y1)) {
            movecost += 150;
        } else {
            movecost += 50 * g->m.move_cost(x1, y1);
        }
        if (g->m.has_flag("SWIMMABLE", x2, y2)) {
            movecost += 150;
        } else {
            movecost += 50 * g->m.move_cost(x2, y2);
        }
        movecost *= diag_mult / 2;
    // All others use the same calculation as the player
    } else {
        movecost = (g->m.combined_movecost(x1, y1, x2, y2));
    }

    return movecost;
}

/*
 * Return points of an area extending 1 tile to either side and
 * (maxdepth) tiles behind basher.
 */
std::vector<point> get_bashing_zone( point bashee, point basher, int maxdepth ) {
    std::vector<point> direction;
    direction.push_back(bashee);
    direction.push_back(basher);
    // Draw a line from the target through the attacker.
    std::vector<point> path = continue_line( direction, maxdepth );
    // Remove the target.
    path.insert( path.begin(), basher );
    std::vector<point> zone;
    // Go ahead and reserve enough room for all the points since
    // we know how many it will be.
    zone.reserve( 3 * maxdepth );
    point previous = bashee;
    for( point p : path ) {
        std::vector<point> swath = squares_in_direction( previous.x, previous.y, p.x, p.y );
        for( point q : swath ) {
            zone.push_back( q );
        }
        previous = p;
    }
    return zone;
}

int monster::bash_at(int x, int y) {

    if (has_effect("pacified")) return 0;

    //Hallucinations can't bash stuff.
    if(is_hallucination()) {
       return 0;
    }
    bool try_bash = !can_move_to(x, y) || one_in(3);
    bool can_bash = g->m.is_bashable(x, y) && (has_flag(MF_BASHES) || has_flag(MF_BORES));
    if( try_bash && can_bash ) {
        int bashskill = group_bash_skill( point(x, y) );

        g->m.bash( x, y, bashskill );
        moves -= 100;
        return 1;
    }
    return 0;
}

int monster::bash_estimate()
{
    int estimate = bash_skill();
    if( has_flag(MF_GROUP_BASH) ) {
        // Right now just give them a boost so they try to bash a lot of stuff.
        // TODO: base it on number of nearby friendlies.
        estimate += 20;
    }
    return estimate;
}

int monster::bash_skill()
{
    int ret = type->melee_dice * type->melee_sides; // IOW, the critter's max bashing damage
    if (has_flag(MF_BORES)) {
        ret *= 15; // This is for stuff that goes through solid rock: minerbots, dark wyrms, etc
    } else if (has_flag(MF_DESTROYS)) {
        ret *= 2.5;
    }
    return ret;
}

int monster::group_bash_skill( point target )
{
    if( !has_flag(MF_GROUP_BASH) ) {
        return bash_skill();
    }
    int bashskill = 0;

    // pileup = more bashskill, but only help bashing mob directly infront of target
    const int max_helper_depth = 5;
    const std::vector<point> bzone = get_bashing_zone( target, pos(), max_helper_depth );

    for( point candidate : bzone ) {
        // Drawing this line backwards excludes the target and includes the candidate.
        std::vector<point> path_to_target = line_to( target, candidate, 0 );
        bool connected = true;
        int mondex = -1;
        for( point in_path : path_to_target ) {
            // If any point in the line from zombie to target is not a cooperating zombie,
            // it can't contribute.
            mondex = g->mon_at( in_path );
            if( mondex == -1 ) {
                connected = false;
                break;
            }
            monster &helpermon = g->zombie( mondex );
            if( !helpermon.has_flag(MF_GROUP_BASH) || helpermon.is_hallucination() ) {
                connected = false;
                break;
            }
        }
        if( !connected ) {
            continue;
        }
        // If we made it here, the last monster checked was the candidate.
        monster &helpermon = g->zombie( mondex );
        // Contribution falls off rapidly with distance from target.
        bashskill += helpermon.bash_skill() / rl_dist( candidate, target );
    }

    return bashskill;
}

int monster::attack_at(int x, int y) {

    if (has_effect("pacified")) return 0;

    int mondex = g->mon_at(x, y);
    int npcdex = g->npc_at(x, y);

    if(x == g->u.posx() && y == g->u.posy()) {
        melee_attack(g->u);
        return 1;
    }

    if(mondex != -1) {
        monster& mon = g->zombie(mondex);

        // Don't attack yourself.
        if(&mon == this) {
            return 0;
        }

        // Special case: Target is hallucination
        if(mon.is_hallucination()) {
            mon.die( nullptr );

            // We haven't actually attacked anything, i.e. we can still do things.
            // Hallucinations(obviously) shouldn't affect the way real monsters act.
            return 0;
        }

        // With no melee dice, we can't attack, but we had to process until here
        // because hallucinations require no melee dice to destroy.
        if(type->melee_dice <= 0) {
            return 0;
        }

        auto attitude = attitude_to( mon );
        // MF_ATTACKMON == hulk behavior, whack everything in your way
        if( attitude == A_HOSTILE || has_flag( MF_ATTACKMON ) ) {
            hit_monster(mon);
            return 1;
        }
    } else if(npcdex != -1  && type->melee_dice > 0) {
        // For now we're always attacking NPCs that are getting into our
        // way. This is consistent with how it worked previously, but
        // later on not hitting allied NPCs would be cool.
        melee_attack(*g->active_npc[npcdex]);
        return 1;
    }

    // Nothing to attack.
    return 0;
}

int monster::move_to(int x, int y, bool force)
{
    // Make sure that we can move there, unless force is true.
    if(!force) if(!g->is_empty(x, y) || !can_move_to(x, y)) {
        return 0;
    }

    if (!plans.empty()) {
        plans.erase(plans.begin());
    }

    if (!force) {
        moves -= calc_movecost(posx(), posy(), x, y);
    }

    //Check for moving into/out of water
    bool was_water = g->m.is_divable(posx(), posy());
    bool will_be_water = g->m.is_divable(x, y);

    if(was_water && !will_be_water && g->u.sees(x, y)) {
        //Use more dramatic messages for swimming monsters
        add_msg(m_warning, _("A %s %s from the %s!"), name().c_str(),
                   has_flag(MF_SWIMS) || has_flag(MF_AQUATIC) ? _("leaps") : _("emerges"),
                   g->m.tername(posx(), posy()).c_str());
    } else if(!was_water && will_be_water && g->u.sees(x, y)) {
        add_msg(m_warning, _("A %s %s into the %s!"), name().c_str(),
                   has_flag(MF_SWIMS) || has_flag(MF_AQUATIC) ? _("dives") : _("sinks"),
                   g->m.tername(x, y).c_str());
    }

    setpos(x, y);
    footsteps(x, y);
    if(is_hallucination()) {
        //Hallucinations don't do any of the stuff after this point
        return 1;
    }
    if (type->size != MS_TINY && g->m.has_flag("SHARP", posx(), posy()) && !one_in(4)) {
        apply_damage( nullptr, bp_torso, rng( 2, 3 ) );
    }
    if (type->size != MS_TINY && g->m.has_flag("ROUGH", posx(), posy()) && one_in(6)) {
        apply_damage( nullptr, bp_torso, rng( 1, 2 ) );
    }
    if (g->m.has_flag("UNSTABLE", x, y)) {
        add_effect("bouldering", 1, num_bp, true);
    } else if (has_effect("bouldering")) {
        remove_effect("bouldering");
    }
    if (!digging() && !has_flag(MF_FLIES) &&
          g->m.tr_at(posx(), posy()) != tr_null) { // Monster stepped on a trap!
        trap* tr = traplist[g->m.tr_at(posx(), posy())];
        if (dice(3, type->sk_dodge + 1) < dice(3, tr->get_avoidance())) {
            tr->trigger(this, posx(), posy());
        }
    }
    // Diggers turn the dirt into dirtmound
    if (digging()){
        int factor = 0;
        switch (type->size) {
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
        if (has_flag(MF_VERMIN)) {
            factor *= 100;
        }
        if (one_in(factor)) {
            g->m.ter_set(posx(), posy(), t_dirtmound);
        }
    }
    // Acid trail monsters leave... a trail of acid
    if (has_flag(MF_ACIDTRAIL)){
        g->m.add_field(posx(), posy(), fd_acid, 3);
    }

    if (has_flag(MF_SLUDGETRAIL)) {
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                const int fstr = 3 - (abs(dx) + abs(dy));
                if (fstr >= 2) {
                    g->m.add_field(posx() + dx, posy() + dy, fd_sludge, fstr);
                }
            }
        }
    }
    if (has_flag(MF_LEAKSGAS)){
        if (one_in(6)){
        g->m.add_field(posx() + rng(-1,1), posy() + rng(-1, 1), fd_toxic_gas, 3);
        }
    }

    return 1;
}

/* Random walking even when we've moved
 * To simulate zombie stumbling and ineffective movement
 * Note that this is sub-optimal; stumbling may INCREASE a zombie's speed.
 * Most of the time (out in the open) this effect is insignificant compared to
 * the negative effects, but in a hallway it's perfectly even
 */
void monster::stumble(bool moved)
{
 // don't stumble every turn. every 3rd turn, or 8th when walking.
 if((moved && !one_in(8)) || !one_in(3))
 {
     return;
 }

 std::vector <point> valid_stumbles;
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   const int nx = posx() + i;
   const int ny = posy() + j;
   if ((i || j) && can_move_to(nx, ny) &&
       //Stop zombies and other non-breathing monsters wandering INTO water
       //(Unless they can swim/are aquatic)
       //But let them wander OUT of water if they are there.
       !(has_flag(MF_NO_BREATHE) && !has_flag(MF_SWIMS) && !has_flag(MF_AQUATIC)
           && g->m.has_flag("SWIMMABLE", nx, ny)
           && !g->m.has_flag("SWIMMABLE", posx(), posy())) &&
       (g->u.posx() != nx || g->u.posy() != ny) &&
       (g->mon_at(nx, ny) == -1) &&
       (g->npc_at(nx, ny) == -1) ) {
    point tmp(nx, ny);
    valid_stumbles.push_back(tmp);
   }
  }
 }
 if (valid_stumbles.empty()) //nowhere to stumble?
 {
     return;
 }

 int choice = rng(0, valid_stumbles.size() - 1);
 int cx = valid_stumbles[choice].x;
 int cy = valid_stumbles[choice].y;

 moves -= calc_movecost(posx(), posy(), cx, cy);
 setpos(cx, cy);

 // Here we have to fix our plans[] list,
 // acquiring a new path to the previous target.
 // target == either end of current plan, or the player.
 int bresenham_slope;
 if (!plans.empty()) {
  if (g->m.sees( pos(), plans.back(), -1, bresenham_slope))
   set_dest(plans.back().x, plans.back().y, bresenham_slope);
  else if (sees( g->u, bresenham_slope ))
   set_dest(g->u.posx(), g->u.posy(), bresenham_slope);
  else //durr, i'm suddenly calm. what was i doing?
   plans.clear();
 }
}

void monster::knock_back_from(int x, int y)
{
 if (x == posx() && y == posy())
  return; // No effect
    if( is_hallucination() ) {
        die( nullptr );
        return;
    }
 point to(posx(), posy());
 if (x < posx())
  to.x++;
 if (x > posx())
  to.x--;
 if (y < posy())
  to.y++;
 if (y > posy())
  to.y--;

 bool u_see = g->u.sees( to );

// First, see if we hit another monster
 int mondex = g->mon_at(to.x, to.y);
    if( mondex != -1 && g->zombie( mondex ).is_hallucination() ) {
        // hallucinations should not affect real monsters. If they interfer, just remove them.
        g->zombie( mondex ).die( this );
        mondex = -1;
    }
 if (mondex != -1) {
  monster *z = &(g->zombie(mondex));
  apply_damage( z, bp_torso, z->type->size );
  add_effect("stunned", 1);
  if (type->size > 1 + z->type->size) {
   z->knock_back_from(posx(), posy()); // Chain reaction!
   z->apply_damage( this, bp_torso, type->size );
   z->add_effect("stunned", 1);
  } else if (type->size > z->type->size) {
   z->apply_damage( this, bp_torso, type->size );
   z->add_effect("stunned", 1);
  }

  if (u_see)
   add_msg(_("The %s bounces off a %s!"), name().c_str(), z->name().c_str());

  return;
 }

 int npcdex = g->npc_at(to.x, to.y);
 if (npcdex != -1) {
  npc *p = g->active_npc[npcdex];
  apply_damage( p, bp_torso, 3 );
  add_effect("stunned", 1);
  p->deal_damage( this, bp_torso, damage_instance( DT_BASH, type->size ) );
  if (u_see)
   add_msg(_("The %s bounces off %s!"), name().c_str(), p->name.c_str());

  return;
 }

// If we're still in the function at this point, we're actually moving a tile!
 if (g->m.ter_at(to.x, to.y).has_flag(TFLAG_DEEP_WATER)) {
  if (g->m.has_flag("LIQUID", to.x, to.y) && can_drown()) {
   die( nullptr );
   if (u_see) {
    add_msg(_("The %s drowns!"), name().c_str());
   }

  } else if (has_flag(MF_AQUATIC)) { // We swim but we're NOT in water
   die( nullptr );
   if (u_see) {
    add_msg(_("The %s flops around and dies!"), name().c_str());
   }
  }
 }

 if (g->m.move_cost(to.x, to.y) == 0) {

   // It's some kind of wall.
   apply_damage( nullptr, bp_torso, type->size );
   add_effect("stunned", 2);
   if (u_see) {
    add_msg(_("The %s bounces off a %s."), name().c_str(),
               g->m.tername(to.x, to.y).c_str());
   }

 } else { // It's no wall
  setpos(to);
 }
}


/* will_reach() is used for determining whether we'll get to stairs (and
 * potentially other locations of interest).  It is generally permissive.
 * TODO: Pathfinding;
         Make sure that non-smashing monsters won't "teleport" through windows
         Injure monsters if they're gonna be walking through pits or whatevs
 */
bool monster::will_reach(int x, int y)
{
 monster_attitude att = attitude(&(g->u));
 if (att != MATT_FOLLOW && att != MATT_ATTACK && att != MATT_FRIEND && att != MATT_ZLAVE)
  return false;

 if (has_flag(MF_DIGS))
  return false;

 if (has_flag(MF_IMMOBILE) && (posx() != x || posy() != y))
  return false;

 std::vector<point> path = g->m.route(posx(), posy(), x, y, false);
 if (path.empty())
   return false;

 if (has_flag(MF_SMELLS) && g->scent(posx(), posy()) > 0 &&
     g->scent(x, y) > g->scent(posx(), posy()))
  return true;

 if (can_hear() && wandf > 0 && rl_dist(wandx, wandy, x, y) <= 2 &&
     rl_dist(posx(), posy(), wandx, wandy) <= wandf)
  return true;

 int t;
 if (can_see() && g->m.sees(posx(), posy(), x, y, g->light_level(), t))
  return true;

 return false;
}

int monster::turns_to_reach(int x, int y)
{
    std::vector<point> path = g->m.route(posx(), posy(), x, y, false);
    if (path.empty())
        return 999;

    double turns = 0.;
    for (size_t i = 0; i < path.size(); i++) {
        if (g->m.move_cost(path[i].x, path[i].y) == 0) {
            // We have to bash through
            turns += 5;
        } else if (i == 0) {
            turns += double(calc_movecost(posx(), posy(), path[i].x, path[i].y)) / get_speed();
        } else {
            turns += double(calc_movecost(path[i-1].x, path[i-1].y, path[i].x, path[i].y)) / get_speed();
        }
    }
    return int(turns + .9); // Round up
}
