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

    const ter_id target = g->m.ter( p );
    const field &target_field = g->m.field_at( p );
    const trap &target_trap = g->m.tr_at( p );
    // Various avoiding behaviors
    if( has_flag( MF_AVOID_DANGER_1 ) || has_flag( MF_AVOID_DANGER_2 ) ) {
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

        // Differently handled behaviors
        if( has_flag( MF_AVOID_DANGER_2 ) ) {
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

    // If we can see the player, move toward them or flee.
    if( friendly == 0 && sees( g->u ) ) {
        dist = rate_target( g->u, dist, smart_planning );
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

    for( size_t i = 0; i < g->active_npc.size(); i++ ) {
        npc *me = g->active_npc[i];
        float rating = rate_target( *me, dist, smart_planning );
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
        for( const int i : myfaction_iter->second ) {
            monster &mon = g->zombie( i );
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

// This is a table of moves spent to stagger in different directions.
// It was empirically derived by spawning monsters and having them proceed
// to a destination and recording the moves required. See tests/monster_test.cpp for details.
// The indices range from -314 to +314, shifted by 314 of course, which is radians * 100.
// This is a fairly terrible solution, but it's the only approach I can think of that seems to work.
const static float trig_adjustment_values[] = {
    // -314 to -301
    119.000, 119.000, 119.000, 119.000,
    114.264, 113.216, 109.664, 109.055, 109.758, 109.907, 109.371, 109.956, 109.727, 109.265,
    // -300 to -200
    110.270, 110.096, 109.655, 109.798, 110.414, 110.194, 109.756, 109.786, 109.718, 109.378,
    109.327, 108.457, 109.151, 109.271, 108.766, 108.539, 108.875, 108.861, 108.700, 107.394,
    108.205, 108.321, 108.430, 108.260, 107.254, 108.278, 107.903, 107.872, 106.955, 107.996,
    106.929, 107.747, 108.030, 106.880, 108.056, 106.777, 107.599, 108.087, 106.864, 106.680,
    106.784, 106.940, 106.903, 106.827, 107.201, 108.054, 108.212, 108.125, 108.234, 108.004,
    108.350, 108.415, 108.293, 108.572, 108.499, 108.669, 109.633, 108.641, 108.843, 109.087,
    109.157, 109.233, 109.497, 109.566, 109.623, 109.482, 109.557, 109.727, 109.406, 109.305,
    109.012, 108.971, 108.893, 108.486, 108.515, 108.601, 108.723, 108.431, 108.292, 108.338,
    108.318, 108.246, 108.115, 107.951, 108.137, 107.820, 107.885, 107.965, 107.831, 107.804,
    107.811, 107.844, 107.767, 107.833, 107.791, 107.956, 107.727, 107.596, 108.075, 107.844,

    108.043, 107.934, 107.912, 107.737, 108.563, 108.062, 107.928, 108.108, 107.983, 108.402,
    108.503, 108.447, 108.848, 108.788, 108.796, 108.356, 109.016, 109.036, 109.194, 109.229,
    109.171, 109.445, 109.823, 109.924, 109.749, 110.375, 110.353, 107.211, 107.663, 107.884,
    108.474, 107.771, 107.845, 108.340, 109.524, 109.711, 110.975, 112.360, 113.647, 114.344,
    119.862, 119.949, 119.500, 129.465, 119.000, 119.680, 119.738, 119.246, 114.990, 114.544,
    114.409, 114.347, 113.881, 115.480, 113.425, 115.432, 114.077, 115.551, 114.089, 112.532,
    112.379, 111.875, 111.987, 112.301, 112.277, 111.921, 112.854, 114.760, 113.285, 113.253,
    113.271, 113.193, 113.172, 112.775, 112.991, 111.537, 111.433, 111.492, 111.469, 111.269,
    111.207, 111.335, 111.389, 111.194, 111.105, 110.646, 110.979, 110.748, 110.601, 109.800,
    109.303, 109.369, 109.607, 109.400, 109.132, 109.284, 109.276, 109.017, 108.920, 108.887,

    108.747, 108.952, 108.924, 108.764, 109.030, 108.833, 108.867, 108.925, 108.712, 108.914,
    108.842, 108.789, 109.090, 108.746, 109.054, 108.905, 109.206, 109.234, 109.354, 109.251,
    109.418, 109.720, 109.669, 109.778, 109.582, 109.123, 109.181, 109.192, 106.998, 106.996,
    107.789, 107.703, 107.676, 107.589, 108.420, 108.452, 108.391, 108.264, 109.143, 108.145,
    109.949, 109.489, 109.029, 108.832, 108.846, 108.988, 109.002, 108.575, 107.883, 108.711,
    108.932, 108.754, 108.610, 107.963, 107.662, 107.972, 108.060, 108.091, 107.876, 107.978,
    108.201, 107.893, 107.751, 108.268, 108.270, 107.933, 108.225, 108.180, 108.545, 108.665,
    109.040, 108.886, 109.083, 108.817, 109.243, 109.131, 109.040, 109.378, 108.989, 109.799,
    109.805, 109.841, 110.185, 110.411, 107.340, 107.545, 107.941, 107.584, 108.411, 108.836,
    108.754, 109.328, 109.715, 109.822, 118.360, 118.868, 117.019, 117.522, 117.929, 117.580,
    // index 0.
    128.003,
    // 1 to 100
    119.660, 119.251, 119.016, 119.617, 106.696, 106.251, 113.173, 113.848, 113.973, 113.670,
    113.550, 113.069, 112.887, 112.819, 112.829, 111.826, 112.234, 112.490, 111.839, 111.950,
    114.706, 114.726, 115.186, 113.834, 114.209, 114.093, 114.013, 114.093, 112.761, 112.756,
    114.125, 111.494, 111.582, 111.140, 111.355, 111.307, 111.014, 110.825, 110.368, 110.096,
    109.770, 109.720, 109.525, 110.111, 109.824, 109.133, 109.640, 109.436, 109.370, 109.100,
    109.244, 109.013, 109.340, 109.175, 109.048, 108.847, 109.015, 108.992, 108.941, 108.889,
    108.820, 108.948, 108.650, 108.734, 109.015, 108.818, 108.592, 109.090, 109.046, 109.115,
    109.023, 109.261, 109.349, 109.316, 109.452, 109.716, 110.787, 110.807, 110.775, 110.577,
    110.422, 109.318, 109.197, 109.305, 109.181, 109.023, 109.073, 108.852, 108.919, 108.742,
    108.758, 108.841, 108.909, 109.098, 108.942, 108.736, 108.882, 108.707, 109.200, 108.958,

    108.757, 108.862, 108.973, 109.140, 108.953, 109.078, 109.430, 109.519, 109.491, 109.173,
    110.640, 110.808, 110.771, 111.001, 110.815, 110.817, 111.286, 111.155, 111.239, 111.189,
    112.466, 112.437, 112.852, 112.573, 112.158, 111.937, 112.544, 114.218, 113.815, 114.220,
    114.072, 114.954, 115.520, 115.720, 115.883, 116.275, 111.767, 112.197, 112.032, 112.789,
    112.663, 112.927, 112.898, 112.868, 113.086, 116.417, 117.425, 117.820, 117.832, 119.526,
    119.773, 117.940, 117.458, 117.506, 117.280, 117.350, 127.393, 117.500, 116.979, 116.682,
    116.980, 115.553, 111.384, 111.940, 111.544, 111.296, 110.921, 111.330, 112.587, 110.931,
    110.987, 110.597, 110.311, 110.495, 109.891, 109.980, 110.022, 109.731, 109.329, 109.162,
    109.224, 109.085, 108.854, 108.944, 109.024, 108.500, 108.454, 108.594, 108.737, 108.402,
    108.253, 108.300, 108.262, 108.417, 108.123, 108.144, 108.197, 107.974, 107.914, 108.143,

    107.681, 108.105, 107.961, 108.048, 107.934, 107.819, 106.002, 107.790, 107.919, 108.056,
    107.972, 107.876, 107.623, 108.078, 108.027, 108.210, 107.043, 108.025, 108.207, 108.411,
    107.375, 108.361, 108.488, 108.443, 108.662, 108.572, 108.802, 108.613, 109.195, 108.972,
    109.152, 109.325, 109.565, 110.765, 110.698, 110.803, 110.683, 110.210, 109.398, 109.151,
    109.117, 109.295, 109.056, 108.990, 109.010, 108.635, 108.792, 108.865, 108.785, 109.033,
    108.748, 108.719, 108.574, 108.826, 108.495, 109.049, 108.897, 108.967, 109.051, 108.687,
    109.147, 109.073, 109.165, 109.314, 109.489, 109.559, 109.217, 110.715, 110.868, 110.749,
    110.515, 110.826, 110.860, 111.073, 111.233, 111.100, 111.148, 111.211, 111.209, 111.525,
    111.421, 111.963, 111.906, 112.111, 112.220, 113.857, 114.443, 114.306, 114.300, 114.695,
    114.720, 114.505, 115.029, 111.803, 112.182, 112.334, 112.212, 112.834, 112.730, 113.060,
    // 300 to 314
    112.842, 113.253, 113.563, 113.575, 114.159, 114.176, 114.867, 114.342, 114.999, 119.033,
    119.559, 119.645, 119.500, 129.657
};

// Follows the same pattern as trig_adjustment_values, but empirically derived for square distance.
const static float square_adjustment_values[] = {
    98.400, 98.400, 98.050, 98.932,
    98.054, 98.976, 98.085, 98.682, 98.218, 98.276, 98.386, 98.411, 99.293, 99.602,

    101.540, 101.700, 101.883, 102.004, 101.546, 102.147, 102.380, 102.400, 102.673, 102.712,
    102.828, 102.944, 103.210, 103.276, 103.423, 103.458, 103.599, 103.829, 103.698, 104.096,
    104.257, 104.416, 104.894, 104.809, 105.009, 104.997, 105.143, 105.354, 105.392, 105.786,
    106.004, 106.174, 106.518, 106.396, 106.702, 106.860, 107.042, 107.222, 107.894, 110.742,
    108.533, 108.649, 108.698, 108.786, 109.229, 109.168, 109.719, 112.547, 110.131, 110.146,
    109.823, 110.630, 111.261, 111.106, 111.100, 111.839, 110.941, 111.883, 111.791, 112.103,
    124.079, 124.866, 125.336, 126.109, 127.114, 130.262, 126.477, 125.670, 124.262, 123.688,
    111.408, 111.301, 111.506, 111.193, 111.464, 110.801, 110.339, 110.536, 110.669, 110.279,
    109.753, 109.477, 109.238, 109.057, 109.012, 108.701, 108.507, 108.261, 108.207, 107.937,
    107.650, 107.593, 107.140, 107.175, 106.762, 106.779, 106.491, 106.314, 106.231, 106.251,

    106.049, 105.785, 105.701, 105.152, 105.103, 105.185, 105.070, 104.757, 104.648, 104.499,
    104.360, 103.948, 103.946, 103.785, 103.696, 103.261, 103.313, 103.025, 103.108, 102.913,
    102.630, 102.500, 102.272, 102.268, 100.950, 101.511, 100.553, 101.258, 101.052, 101.054,
    100.479, 100.429, 100.088, 100.103, 099.850, 099.569, 099.148, 099.205, 099.051, 099.044,
    099.010, 098.890, 099.100, 099.204, 099.695, 100.000, 099.025, 099.150, 099.353, 099.358,
    100.453, 100.704, 100.608, 100.800, 100.782, 100.953, 101.597, 101.096, 101.309, 101.501,
    102.079, 102.484, 102.536, 102.902, 102.951, 103.075, 103.379, 103.451, 103.288, 103.619,
    103.822, 104.047, 103.922, 104.278, 104.645, 104.575, 104.680, 104.895, 105.108, 105.353,
    105.525, 105.600, 105.609, 105.892, 105.934, 106.307, 106.541, 106.127, 106.474, 106.942,
    106.630, 106.977, 107.179, 107.463, 107.699, 107.570, 107.815, 108.047, 108.347, 108.514,

    109.001, 109.809, 109.342, 110.231, 110.311, 109.924, 110.079, 110.745, 111.188, 111.257,
    110.741, 111.135, 111.267, 111.413, 111.883, 112.002, 112.040, 124.579, 124.866, 125.336,
    126.109, 127.114, 130.562, 127.477, 127.170, 125.162, 124.088, 112.138, 111.836, 111.430,
    110.354, 110.892, 110.695, 110.452, 110.362, 110.101, 109.981, 109.561, 109.492, 109.284,
    108.731, 108.971, 108.517, 108.458, 108.535, 108.129, 107.985, 107.683, 107.596, 107.369,
    106.775, 107.116, 106.848, 106.645, 106.341, 106.407, 106.140, 105.858, 105.785, 105.635,
    105.503, 105.126, 104.966, 105.075, 104.728, 104.420, 104.410, 104.298, 104.112, 103.835,
    103.886, 103.518, 103.461, 103.279, 103.201, 102.949, 102.933, 102.794, 102.826, 102.478,
    102.086, 101.461, 102.011, 101.985, 101.917, 101.607, 101.516, 101.566, 101.452, 101.204,
    100.787, 100.004, 100.051, 100.103, 100.050, 099.773, 100.024, 099.537, 099.647, 099.820,

    98.078,

    100.080, 100.028, 100.074, 100.110, 100.213, 100.156, 100.510, 100.512, 100.716, 100.653,
    100.686, 100.867, 101.255, 101.133, 101.629, 101.600, 101.777, 101.812, 101.969, 101.927,
    102.281, 102.240, 102.299, 102.517, 103.204, 102.593, 103.654, 103.852, 103.076, 104.038,
    104.403, 104.636, 104.888, 105.078, 105.092, 105.482, 105.199, 105.334, 105.794, 105.963,
    106.036, 106.180, 106.415, 106.536, 106.719, 106.577, 107.243, 107.119, 107.323, 107.475,
    107.561, 107.846, 108.132, 108.078, 108.251, 108.773, 108.874, 108.834, 109.219, 109.374,
    109.491, 109.734, 109.782, 110.113, 110.149, 110.281, 110.655, 110.707, 110.882, 111.467,
    111.669, 111.618, 112.041, 112.154, 112.435, 112.713, 122.335, 133.120, 121.277, 112.750,
    112.742, 112.412, 111.876, 111.682, 111.385, 111.632, 111.168, 110.928, 110.596, 110.386,
    110.103, 109.973, 109.697, 109.720, 109.422, 109.286, 109.218, 108.886, 108.850, 108.559,

    108.272, 108.043, 108.151, 108.066, 107.867, 107.684, 107.410, 107.326, 107.054, 106.862,
    106.872, 106.689, 106.160, 106.339, 106.095, 105.704, 105.829, 105.580, 105.460, 105.422,
    104.785, 105.056, 104.912, 103.480, 104.075, 103.452, 103.434, 103.191, 102.774, 102.446,
    102.846, 102.701, 102.581, 102.322, 102.188, 102.230, 101.924, 101.774, 101.617, 101.553,
    101.470, 101.273, 101.135, 101.047, 101.123, 100.968, 100.902, 100.783, 100.499, 100.654,
    100.422, 100.239, 100.197, 100.080, 100.200, 100.175, 100.034, 100.000, 099.856, 099.904,
    100.117, 100.065, 100.090, 100.203, 100.185, 100.366, 100.379, 100.466, 100.644, 100.719,
    101.568, 101.980, 102.027, 101.891, 102.273, 102.340, 102.297, 102.466, 102.611, 102.954,
    102.864, 102.889, 103.183, 103.412, 103.430, 103.660, 103.786, 103.979, 103.967, 104.420,
    104.371, 104.569, 104.850, 105.005, 105.264, 105.136, 105.466, 105.447, 105.748, 106.090,

    106.186, 106.291, 106.257, 106.585, 106.783, 106.920, 107.064, 107.718, 108.084, 107.669,
    108.381, 108.967, 108.746, 109.578, 109.565, 108.856, 109.469, 110.386, 110.038, 110.684,
    110.953, 110.120, 110.831, 111.337, 110.772, 110.963, 111.291, 111.442, 112.293, 111.965,
    124.579, 124.866, 125.836, 126.609, 128.614, 126.862, 126.577, 125.670, 124.662, 124.188,
    112.388, 111.667, 111.181, 111.195, 110.930, 110.604, 110.216, 110.708, 109.840, 109.910,
    109.804, 109.523, 109.336, 109.253, 108.867, 109.053, 108.757, 108.505, 108.382, 108.249,
    108.015, 107.802, 107.775, 107.607, 107.256, 107.218, 106.922, 106.881, 106.781, 106.258,
    106.272, 106.125, 105.894, 105.716, 105.816, 105.688, 105.306, 105.144, 105.114, 104.959,
    104.659, 104.572, 104.514, 104.386, 104.248, 104.096, 103.847, 103.600, 103.531, 103.618,
    103.266, 103.301, 103.239, 102.850, 103.025, 101.286, 101.666, 101.356, 101.386, 101.173,

    101.021, 101.025, 099.883, 100.601, 100.757, 100.428, 100.459, 099.404, 100.377, 100.411,
    100.271, 100.235, 100.200, 100.565
};

static float get_stagger_adjust( const tripoint &source, const tripoint &destination )
{
    const float angle = atan2( source.x - destination.x, source.y - destination.y );
    if( trigdist ) {
        return 100.0 / trig_adjustment_values[314 + ( int )( angle * 100 )];
    }
    return 100.0 / square_adjustment_values[( int )( 314 + ( angle * 100 ) )];
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
    if( !is_hallucination() && has_flag( MF_ABSORBS ) && !g->m.has_flag( TFLAG_SEALED, pos() ) &&
        !g->m.has_items( pos() ) ) {
        if( g->u.sees( *this ) ) {
            add_msg( _( "The %s flows around the objects on the floor and they are quickly dissolved!" ),
                     name().c_str() );
        }
        static const auto volume_per_hp = units::from_milliliter( 250 );
        for( auto &elem : g->m.i_at( pos() ) ) {
            hp += elem.volume() / volume_per_hp; // Yeah this means it can get more HP than normal.
        }
        g->m.i_clear( pos() );
    }

    const bool pacified = has_effect( effect_pacified );

    // First, use the special attack, if we can!
    // The attack may change `monster::special_attacks` (e.g. by transforming
    // this into another monster type). Therefor we can not iterate over it
    // directly and instead iterate over the map from the monster type
    // (properties of monster types should never change).
    for( auto &sp_type : type->special_attacks ) {
        const std::string &special_name = sp_type.first;
        const auto local_iter = special_attacks.find( special_name );
        if( local_iter == special_attacks.end() ) {
            continue;
        }
        mon_special_attack &local_attack_data = local_iter->second;
        if( !local_attack_data.enabled ) {
            continue;
        }

        if( local_attack_data.cooldown > 0 ) {
            local_attack_data.cooldown--;
        }
        if( local_attack_data.cooldown == 0 && !pacified && !is_hallucination() ) {
            if( !sp_type.second.call( *this ) ) {
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
            for( auto &i : g->active_npc ) {
                if( goal == i->pos() ) {
                    current_attitude = attitude( i );
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
    if( wandf > 0 && !moved ) { // No LOS, no scent, so as a fall-back follow sound
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

                if( can_z_move && !can_fly && candidate.z > posz() && !g->m.has_floor_or_support( candidate ) ) {
                    // Can't "jump" up a whole z-level
                    can_z_move = false;
                }

                // Last chance - we can still do the z-level stair teleport bullshit that isn't removed yet
                // @todo Remove z-level stair bullshit teleport after aligning all stairs
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

            const Creature *target = g->critter_at( candidate, is_hallucination() );
            // When attacking an adjacent enemy, we're direct.
            if( target != nullptr && attitude_to( *target ) == A_HOSTILE ) {
                moved = true;
                next_step = candidate;
                break;
            }

            // Allow non-stumbling critters to stumble when most direct choice is bad
            bool bad_choice = false;
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
            // Bail out if there's a non-hostile monster in the way and we're not pushy.
            if( target != nullptr && attitude_to( *target ) != A_HOSTILE ) {
                if( !has_flag( MF_ATTACKMON ) && !has_flag( MF_PUSH_MON ) ) {
                    continue;
                }

                // Friendly fire and pushing are always bad choices - they take a lot of time
                bad_choice = true;
            }
            const float progress = distance_to_target - trig_dist( candidate, destination );
            // The x2 makes the first (and most direct) path twice as likely,
            // since the chance of switching is 1/1, 1/4, 1/6, 1/8
            switch_chance += progress * 2;
            // Randomly pick one of the viable squares to move to weighted by distance.
            if( moved == false || x_in_y( progress, switch_chance ) ) {
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
            move_to( next_step, false,
                     ( staggers ? get_stagger_adjust( pos(), destination ) : 1.0 ) );
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
    for( const auto &dest : g->m.points_in_radius( pos(), 1 ) ) {
        int smell = g->scent.get( dest );
        if( ( can_move_to( dest ) || ( dest == g->u.pos() ) ||
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

    // pileup = more bashskill, but only help bashing mob directly infront of target
    const int max_helper_depth = 5;
    const std::vector<tripoint> bzone = get_bashing_zone( target, pos(), max_helper_depth );

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

    if( p == g->u.pos() ) {
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
            melee_attack( mon, true );
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
        const int cost = stagger_adjustment *
                         ( float )( climbs ? calc_climb_cost( pos(), p ) :
                                    calc_movecost( pos(), p ) );

        if( cost > 0 ) {
            moves -= cost;
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
        add_effect( effect_bouldering, 1, num_bp, true );
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
    add_effect( effect_pushed, 1 );

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
                if( g->mon_at( p ) == -1 ) {
                    move_to( p );
                }

                moves -= movecost_attacker;
                if( movecost_from > 100 ) {
                    critter->add_effect( effect_downed, movecost_from / 100 + 1 );
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
            critter->add_effect( effect_downed, movecost_from / 100 + 1 );
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
    critter->add_effect( effect_stunned, rng( 0, 2 ) );
    // Only print the message when near player or it can get spammy
    if( rl_dist( g->u.pos(), pos() ) < 4 && g->u.sees( *critter ) ) {
        add_msg( m_warning, _( "The %1$s tramples %2$s" ),
                 name().c_str(), critter->disp_name().c_str() );
    }

    moves -= movecost_attacker;
    if( movecost_from > 100 ) {
        critter->add_effect( effect_downed, movecost_from / 100 + 1 );
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
                   !g->m.has_flag( TFLAG_SWIMMABLE, pos() ) ) &&
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
    int mondex = g->mon_at( to );
    if( mondex != -1 ) {
        monster *z = &( g->zombie( mondex ) );
        apply_damage( z, bp_torso, z->type->size );
        add_effect( effect_stunned, 1 );
        if( type->size > 1 + z->type->size ) {
            z->knock_back_from( pos() ); // Chain reaction!
            z->apply_damage( this, bp_torso, type->size );
            z->add_effect( effect_stunned, 1 );
        } else if( type->size > z->type->size ) {
            z->apply_damage( this, bp_torso, type->size );
            z->add_effect( effect_stunned, 1 );
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
        add_effect( effect_stunned, 1 );
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
        add_effect( effect_stunned, 2 );
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
