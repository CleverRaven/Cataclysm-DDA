#include <vector>

#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "monster.h"
#include "mattack_actors.h"
#include "messages.h"
#include "translations.h"

void leap_actor::load( JsonObject &obj )
{
    // Mandatory:
    max_range = obj.get_float( "max_range" );
    // Optional:
    min_range = obj.get_float( "min_range", 1.0f );
    allow_no_target = obj.get_bool( "allow_no_target", true );
    move_cost = obj.get_int( "move_cost", 150 );
    min_consider_range = obj.get_float( "min_consider_range", 0.0f );
    max_consider_range = obj.get_float( "max_consider_range", 200.0f );
}

mattack_actor *leap_actor::clone() const
{
    return new leap_actor( *this );
}

bool leap_actor::call( monster &z ) const
{
    if( !z.can_act() ) {
        return false;
    }

    std::vector<tripoint> options;
    tripoint target = z.move_target();
    float best_float = trig_dist( z.pos(), target );
    if( best_float < min_consider_range || best_float > max_consider_range ) {
        return false;
    }

    // We wanted the float for range check
    // int here will make the jumps more random
    int best = (int)best_float;
    if( !allow_no_target && z.attack_target() == nullptr ) {
        return false;
    }

    for( const tripoint &dest : g->m.points_in_radius( z.pos(), max_range ) ) {
        if( dest == z.pos() ) {
            continue;
        }
        if( !z.sees( dest ) ) {
            continue;
        }
        if( !g->is_empty( dest ) ) {
            continue;
        }
        int cur_dist = rl_dist( target, dest );
        if( cur_dist > best ) {
            continue;
        }
        if( trig_dist( z.pos(), dest ) < min_range ) {
            continue;
        }
        bool blocked_path = false;
        // check if monster has a clear path to the proposed point
        std::vector<tripoint> line = g->m.find_clear_path( z.pos(), dest );
        for( auto &i : line ) {
            if( g->m.impassable( i ) ) {
                blocked_path = true;
                break;
            }
        }
        if( blocked_path ) {
            continue;
        }

        if( cur_dist < best ) {
            // Better than any earlier one
            options.clear();
        }

        options.push_back( dest );
        best = cur_dist;
    }

    if( options.empty() ) {
        return false;    // Nowhere to leap!
    }

    z.moves -= move_cost;
    const tripoint chosen = random_entry( options );
    bool seen = g->u.sees( z ); // We can see them jump...
    z.setpos(chosen);
    seen |= g->u.sees( z ); // ... or we can see them land
    if( seen ) {
        add_msg( _("The %s leaps!"), z.name().c_str() );
    }

    return true;
}
