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
        npc &who = *g->active_npc[i];
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
const static std::array<float, 629> trig_adjustment_values = {{
        // -314 to -301
        119.000f, 119.000f, 119.000f, 119.000f,
        114.264f, 113.216f, 109.664f, 109.055f, 109.758f, 109.907f, 109.371f, 109.956f, 109.727f, 109.265f,
        // -300 to -200
        110.270f, 110.096f, 109.655f, 109.798f, 110.414f, 110.194f, 109.756f, 109.786f, 109.718f, 109.378f,
        109.327f, 108.457f, 109.151f, 109.271f, 108.766f, 108.539f, 108.875f, 108.861f, 108.700f, 107.394f,
        108.205f, 108.321f, 108.430f, 108.260f, 107.254f, 108.278f, 107.903f, 107.872f, 106.955f, 107.996f,
        106.929f, 107.747f, 108.030f, 106.880f, 108.056f, 106.777f, 107.599f, 108.087f, 106.864f, 106.680f,
        106.784f, 106.940f, 106.903f, 106.827f, 107.201f, 108.054f, 108.212f, 108.125f, 108.234f, 108.004f,
        108.350f, 108.415f, 108.293f, 108.572f, 108.499f, 108.669f, 109.633f, 108.641f, 108.843f, 109.087f,
        109.157f, 109.233f, 109.497f, 109.566f, 109.623f, 109.482f, 109.557f, 109.727f, 109.406f, 109.305f,
        109.012f, 108.971f, 108.893f, 108.486f, 108.515f, 108.601f, 108.723f, 108.431f, 108.292f, 108.338f,
        108.318f, 108.246f, 108.115f, 107.951f, 108.137f, 107.820f, 107.885f, 107.965f, 107.831f, 107.804f,
        107.811f, 107.844f, 107.767f, 107.833f, 107.791f, 107.956f, 107.727f, 107.596f, 108.075f, 107.844f,

        108.043f, 107.934f, 107.912f, 107.737f, 108.563f, 108.062f, 107.928f, 108.108f, 107.983f, 108.402f,
        108.503f, 108.447f, 108.848f, 108.788f, 108.796f, 108.356f, 109.016f, 109.036f, 109.194f, 109.229f,
        109.171f, 109.445f, 109.823f, 109.924f, 109.749f, 110.375f, 110.353f, 107.211f, 107.663f, 107.884f,
        108.474f, 107.771f, 107.845f, 108.340f, 109.524f, 109.711f, 110.975f, 112.360f, 113.647f, 114.344f,
        119.862f, 119.949f, 119.500f, 129.465f, 119.000f, 119.680f, 119.738f, 119.246f, 114.990f, 114.544f,
        114.409f, 114.347f, 113.881f, 115.480f, 113.425f, 115.432f, 114.077f, 115.551f, 114.089f, 112.532f,
        112.379f, 111.875f, 111.987f, 112.301f, 112.277f, 111.921f, 112.854f, 114.760f, 113.285f, 113.253f,
        113.271f, 113.193f, 113.172f, 112.775f, 112.991f, 111.537f, 111.433f, 111.492f, 111.469f, 111.269f,
        111.207f, 111.335f, 111.389f, 111.194f, 111.105f, 110.646f, 110.979f, 110.748f, 110.601f, 109.800f,
        109.303f, 109.369f, 109.607f, 109.400f, 109.132f, 109.284f, 109.276f, 109.017f, 108.920f, 108.887f,

        108.747f, 108.952f, 108.924f, 108.764f, 109.030f, 108.833f, 108.867f, 108.925f, 108.712f, 108.914f,
        108.842f, 108.789f, 109.090f, 108.746f, 109.054f, 108.905f, 109.206f, 109.234f, 109.354f, 109.251f,
        109.418f, 109.720f, 109.669f, 109.778f, 109.582f, 109.123f, 109.181f, 109.192f, 106.998f, 106.996f,
        107.789f, 107.703f, 107.676f, 107.589f, 108.420f, 108.452f, 108.391f, 108.264f, 109.143f, 108.145f,
        109.949f, 109.489f, 109.029f, 108.832f, 108.846f, 108.988f, 109.002f, 108.575f, 107.883f, 108.711f,
        108.932f, 108.754f, 108.610f, 107.963f, 107.662f, 107.972f, 108.060f, 108.091f, 107.876f, 107.978f,
        108.201f, 107.893f, 107.751f, 108.268f, 108.270f, 107.933f, 108.225f, 108.180f, 108.545f, 108.665f,
        109.040f, 108.886f, 109.083f, 108.817f, 109.243f, 109.131f, 109.040f, 109.378f, 108.989f, 109.799f,
        109.805f, 109.841f, 110.185f, 110.411f, 107.340f, 107.545f, 107.941f, 107.584f, 108.411f, 108.836f,
        108.754f, 109.328f, 109.715f, 109.822f, 118.360f, 118.868f, 117.019f, 117.522f, 117.929f, 117.580f,
        // index 0.
        128.003f,
        // 1 to 100
        119.660f, 119.251f, 119.016f, 119.617f, 106.696f, 106.251f, 113.173f, 113.848f, 113.973f, 113.670f,
        113.550f, 113.069f, 112.887f, 112.819f, 112.829f, 111.826f, 112.234f, 112.490f, 111.839f, 111.950f,
        114.706f, 114.726f, 115.186f, 113.834f, 114.209f, 114.093f, 114.013f, 114.093f, 112.761f, 112.756f,
        114.125f, 111.494f, 111.582f, 111.140f, 111.355f, 111.307f, 111.014f, 110.825f, 110.368f, 110.096f,
        109.770f, 109.720f, 109.525f, 110.111f, 109.824f, 109.133f, 109.640f, 109.436f, 109.370f, 109.100f,
        109.244f, 109.013f, 109.340f, 109.175f, 109.048f, 108.847f, 109.015f, 108.992f, 108.941f, 108.889f,
        108.820f, 108.948f, 108.650f, 108.734f, 109.015f, 108.818f, 108.592f, 109.090f, 109.046f, 109.115f,
        109.023f, 109.261f, 109.349f, 109.316f, 109.452f, 109.716f, 110.787f, 110.807f, 110.775f, 110.577f,
        110.422f, 109.318f, 109.197f, 109.305f, 109.181f, 109.023f, 109.073f, 108.852f, 108.919f, 108.742f,
        108.758f, 108.841f, 108.909f, 109.098f, 108.942f, 108.736f, 108.882f, 108.707f, 109.200f, 108.958f,

        108.757f, 108.862f, 108.973f, 109.140f, 108.953f, 109.078f, 109.430f, 109.519f, 109.491f, 109.173f,
        110.640f, 110.808f, 110.771f, 111.001f, 110.815f, 110.817f, 111.286f, 111.155f, 111.239f, 111.189f,
        112.466f, 112.437f, 112.852f, 112.573f, 112.158f, 111.937f, 112.544f, 114.218f, 113.815f, 114.220f,
        114.072f, 114.954f, 115.520f, 115.720f, 115.883f, 116.275f, 111.767f, 112.197f, 112.032f, 112.789f,
        112.663f, 112.927f, 112.898f, 112.868f, 113.086f, 116.417f, 117.425f, 117.820f, 117.832f, 119.526f,
        119.773f, 117.940f, 117.458f, 117.506f, 117.280f, 117.350f, 127.393f, 117.500f, 116.979f, 116.682f,
        116.980f, 115.553f, 111.384f, 111.940f, 111.544f, 111.296f, 110.921f, 111.330f, 112.587f, 110.931f,
        110.987f, 110.597f, 110.311f, 110.495f, 109.891f, 109.980f, 110.022f, 109.731f, 109.329f, 109.162f,
        109.224f, 109.085f, 108.854f, 108.944f, 109.024f, 108.500f, 108.454f, 108.594f, 108.737f, 108.402f,
        108.253f, 108.300f, 108.262f, 108.417f, 108.123f, 108.144f, 108.197f, 107.974f, 107.914f, 108.143f,

        107.681f, 108.105f, 107.961f, 108.048f, 107.934f, 107.819f, 106.002f, 107.790f, 107.919f, 108.056f,
        107.972f, 107.876f, 107.623f, 108.078f, 108.027f, 108.210f, 107.043f, 108.025f, 108.207f, 108.411f,
        107.375f, 108.361f, 108.488f, 108.443f, 108.662f, 108.572f, 108.802f, 108.613f, 109.195f, 108.972f,
        109.152f, 109.325f, 109.565f, 110.765f, 110.698f, 110.803f, 110.683f, 110.210f, 109.398f, 109.151f,
        109.117f, 109.295f, 109.056f, 108.990f, 109.010f, 108.635f, 108.792f, 108.865f, 108.785f, 109.033f,
        108.748f, 108.719f, 108.574f, 108.826f, 108.495f, 109.049f, 108.897f, 108.967f, 109.051f, 108.687f,
        109.147f, 109.073f, 109.165f, 109.314f, 109.489f, 109.559f, 109.217f, 110.715f, 110.868f, 110.749f,
        110.515f, 110.826f, 110.860f, 111.073f, 111.233f, 111.100f, 111.148f, 111.211f, 111.209f, 111.525f,
        111.421f, 111.963f, 111.906f, 112.111f, 112.220f, 113.857f, 114.443f, 114.306f, 114.300f, 114.695f,
        114.720f, 114.505f, 115.029f, 111.803f, 112.182f, 112.334f, 112.212f, 112.834f, 112.730f, 113.060f,
        // 300 to 314
        112.842f, 113.253f, 113.563f, 113.575f, 114.159f, 114.176f, 114.867f, 114.342f, 114.999f, 119.033f,
        119.559f, 119.645f, 119.500f, 129.657f
    }
};

// Follows the same pattern as trig_adjustment_valuesf, but empirically derived for square distance.
const static std::array<float, 629> square_adjustment_values = {{
        98.400f, 98.400f, 98.050f, 98.932f,
        98.054f, 98.976f, 98.085f, 98.682f, 98.218f, 98.276f, 98.386f, 98.411f, 99.293f, 99.602f,

        101.540f, 101.700f, 101.883f, 102.004f, 101.546f, 102.147f, 102.380f, 102.400f, 102.673f, 102.712f,
        102.828f, 102.944f, 103.210f, 103.276f, 103.423f, 103.458f, 103.599f, 103.829f, 103.698f, 104.096f,
        104.257f, 104.416f, 104.894f, 104.809f, 105.009f, 104.997f, 105.143f, 105.354f, 105.392f, 105.786f,
        106.004f, 106.174f, 106.518f, 106.396f, 106.702f, 106.860f, 107.042f, 107.222f, 107.894f, 110.742f,
        108.533f, 108.649f, 108.698f, 108.786f, 109.229f, 109.168f, 109.719f, 112.547f, 110.131f, 110.146f,
        109.823f, 110.630f, 111.261f, 111.106f, 111.100f, 111.839f, 110.941f, 111.883f, 111.791f, 112.103f,
        124.079f, 124.866f, 125.336f, 126.109f, 127.114f, 130.262f, 126.477f, 125.670f, 124.262f, 123.688f,
        111.408f, 111.301f, 111.506f, 111.193f, 111.464f, 110.801f, 110.339f, 110.536f, 110.669f, 110.279f,
        109.753f, 109.477f, 109.238f, 109.057f, 109.012f, 108.701f, 108.507f, 108.261f, 108.207f, 107.937f,
        107.650f, 107.593f, 107.140f, 107.175f, 106.762f, 106.779f, 106.491f, 106.314f, 106.231f, 106.251f,

        106.049f, 105.785f, 105.701f, 105.152f, 105.103f, 105.185f, 105.070f, 104.757f, 104.648f, 104.499f,
        104.360f, 103.948f, 103.946f, 103.785f, 103.696f, 103.261f, 103.313f, 103.025f, 103.108f, 102.913f,
        102.630f, 102.500f, 102.272f, 102.268f, 100.950f, 101.511f, 100.553f, 101.258f, 101.052f, 101.054f,
        100.479f, 100.429f, 100.088f, 100.103f, 099.850f, 099.569f, 099.148f, 099.205f, 099.051f, 099.044f,
        099.010f, 098.890f, 099.100f, 099.204f, 099.695f, 100.000f, 099.025f, 099.150f, 099.353f, 099.358f,
        100.453f, 100.704f, 100.608f, 100.800f, 100.782f, 100.953f, 101.597f, 101.096f, 101.309f, 101.501f,
        102.079f, 102.484f, 102.536f, 102.902f, 102.951f, 103.075f, 103.379f, 103.451f, 103.288f, 103.619f,
        103.822f, 104.047f, 103.922f, 104.278f, 104.645f, 104.575f, 104.680f, 104.895f, 105.108f, 105.353f,
        105.525f, 105.600f, 105.609f, 105.892f, 105.934f, 106.307f, 106.541f, 106.127f, 106.474f, 106.942f,
        106.630f, 106.977f, 107.179f, 107.463f, 107.699f, 107.570f, 107.815f, 108.047f, 108.347f, 108.514f,

        109.001f, 109.809f, 109.342f, 110.231f, 110.311f, 109.924f, 110.079f, 110.745f, 111.188f, 111.257f,
        110.741f, 111.135f, 111.267f, 111.413f, 111.883f, 112.002f, 112.040f, 124.579f, 124.866f, 125.336f,
        126.109f, 127.114f, 130.562f, 127.477f, 127.170f, 125.162f, 124.088f, 112.138f, 111.836f, 111.430f,
        110.354f, 110.892f, 110.695f, 110.452f, 110.362f, 110.101f, 109.981f, 109.561f, 109.492f, 109.284f,
        108.731f, 108.971f, 108.517f, 108.458f, 108.535f, 108.129f, 107.985f, 107.683f, 107.596f, 107.369f,
        106.775f, 107.116f, 106.848f, 106.645f, 106.341f, 106.407f, 106.140f, 105.858f, 105.785f, 105.635f,
        105.503f, 105.126f, 104.966f, 105.075f, 104.728f, 104.420f, 104.410f, 104.298f, 104.112f, 103.835f,
        103.886f, 103.518f, 103.461f, 103.279f, 103.201f, 102.949f, 102.933f, 102.794f, 102.826f, 102.478f,
        102.086f, 101.461f, 102.011f, 101.985f, 101.917f, 101.607f, 101.516f, 101.566f, 101.452f, 101.204f,
        100.787f, 100.004f, 100.051f, 100.103f, 100.050f, 099.773f, 100.024f, 099.537f, 099.647f, 099.820f,

        98.078f,

        100.080f, 100.028f, 100.074f, 100.110f, 100.213f, 100.156f, 100.510f, 100.512f, 100.716f, 100.653f,
        100.686f, 100.867f, 101.255f, 101.133f, 101.629f, 101.600f, 101.777f, 101.812f, 101.969f, 101.927f,
        102.281f, 102.240f, 102.299f, 102.517f, 103.204f, 102.593f, 103.654f, 103.852f, 103.076f, 104.038f,
        104.403f, 104.636f, 104.888f, 105.078f, 105.092f, 105.482f, 105.199f, 105.334f, 105.794f, 105.963f,
        106.036f, 106.180f, 106.415f, 106.536f, 106.719f, 106.577f, 107.243f, 107.119f, 107.323f, 107.475f,
        107.561f, 107.846f, 108.132f, 108.078f, 108.251f, 108.773f, 108.874f, 108.834f, 109.219f, 109.374f,
        109.491f, 109.734f, 109.782f, 110.113f, 110.149f, 110.281f, 110.655f, 110.707f, 110.882f, 111.467f,
        111.669f, 111.618f, 112.041f, 112.154f, 112.435f, 112.713f, 122.335f, 133.120f, 121.277f, 112.750f,
        112.742f, 112.412f, 111.876f, 111.682f, 111.385f, 111.632f, 111.168f, 110.928f, 110.596f, 110.386f,
        110.103f, 109.973f, 109.697f, 109.720f, 109.422f, 109.286f, 109.218f, 108.886f, 108.850f, 108.559f,

        108.272f, 108.043f, 108.151f, 108.066f, 107.867f, 107.684f, 107.410f, 107.326f, 107.054f, 106.862f,
        106.872f, 106.689f, 106.160f, 106.339f, 106.095f, 105.704f, 105.829f, 105.580f, 105.460f, 105.422f,
        104.785f, 105.056f, 104.912f, 103.480f, 104.075f, 103.452f, 103.434f, 103.191f, 102.774f, 102.446f,
        102.846f, 102.701f, 102.581f, 102.322f, 102.188f, 102.230f, 101.924f, 101.774f, 101.617f, 101.553f,
        101.470f, 101.273f, 101.135f, 101.047f, 101.123f, 100.968f, 100.902f, 100.783f, 100.499f, 100.654f,
        100.422f, 100.239f, 100.197f, 100.080f, 100.200f, 100.175f, 100.034f, 100.000f, 099.856f, 099.904f,
        100.117f, 100.065f, 100.090f, 100.203f, 100.185f, 100.366f, 100.379f, 100.466f, 100.644f, 100.719f,
        101.568f, 101.980f, 102.027f, 101.891f, 102.273f, 102.340f, 102.297f, 102.466f, 102.611f, 102.954f,
        102.864f, 102.889f, 103.183f, 103.412f, 103.430f, 103.660f, 103.786f, 103.979f, 103.967f, 104.420f,
        104.371f, 104.569f, 104.850f, 105.005f, 105.264f, 105.136f, 105.466f, 105.447f, 105.748f, 106.090f,

        106.186f, 106.291f, 106.257f, 106.585f, 106.783f, 106.920f, 107.064f, 107.718f, 108.084f, 107.669f,
        108.381f, 108.967f, 108.746f, 109.578f, 109.565f, 108.856f, 109.469f, 110.386f, 110.038f, 110.684f,
        110.953f, 110.120f, 110.831f, 111.337f, 110.772f, 110.963f, 111.291f, 111.442f, 112.293f, 111.965f,
        124.579f, 124.866f, 125.836f, 126.609f, 128.614f, 126.862f, 126.577f, 125.670f, 124.662f, 124.188f,
        112.388f, 111.667f, 111.181f, 111.195f, 110.930f, 110.604f, 110.216f, 110.708f, 109.840f, 109.910f,
        109.804f, 109.523f, 109.336f, 109.253f, 108.867f, 109.053f, 108.757f, 108.505f, 108.382f, 108.249f,
        108.015f, 107.802f, 107.775f, 107.607f, 107.256f, 107.218f, 106.922f, 106.881f, 106.781f, 106.258f,
        106.272f, 106.125f, 105.894f, 105.716f, 105.816f, 105.688f, 105.306f, 105.144f, 105.114f, 104.959f,
        104.659f, 104.572f, 104.514f, 104.386f, 104.248f, 104.096f, 103.847f, 103.600f, 103.531f, 103.618f,
        103.266f, 103.301f, 103.239f, 102.850f, 103.025f, 101.286f, 101.666f, 101.356f, 101.386f, 101.173f,

        101.021f, 101.025f, 099.883f, 100.601f, 100.757f, 100.428f, 100.459f, 099.404f, 100.377f, 100.411f,
        100.271f, 100.235f, 100.200f, 100.565f
    }
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
        g->m.has_items( pos() ) ) {
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

        if( local_attack_data.cooldown > 0 ) {
            local_attack_data.cooldown--;
        }

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
    // @todo Remove when scentmap is 3D
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
