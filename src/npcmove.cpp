#include "npc.h" // IWYU pragma: associated

#include <climits>
#include <cstdlib>
#include <algorithm>
#include <memory>
#include <numeric>
#include <sstream>
#include <iterator>
#include <tuple>
#include <cmath>
#include <type_traits>

#include "avatar.h"
#include "bionics.h"
#include "cata_algo.h"
#include "clzones.h"
#include "coordinate_conversions.h"
#include "debug.h"
#include "dispersion.h"
#include "effect.h"
#include "field.h"
#include "game.h"
#include "gates.h"
#include "gun_mode.h"
#include "itype.h"
#include "iuse_actor.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "mission.h"
#include "monster.h"
#include "mtype.h"
#include "npctalk.h"
#include "options.h"
#include "overmap_location.h"
#include "overmapbuffer.h"
#include "projectile.h"
#include "ranged.h"
#include "rng.h"
#include "sounds.h"
#include "translations.h"
#include "veh_type.h"
#include "vehicle.h"
#include "visitable.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "bodypart.h"
#include "character.h"
#include "damage.h"
#include "explosion.h"
#include "game_constants.h"
#include "item.h"
#include "iuse.h"
#include "mapdata.h"
#include "player_activity.h"
#include "ret_val.h"
#include "units.h"
#include "pldata.h"
#include "enums.h"
#include "overmap.h"
#include "stomach.h"

static constexpr float NPC_DANGER_VERY_LOW = 5.0f;
static constexpr float NPC_DANGER_MAX = 150.0f;
static constexpr float MAX_FLOAT = 5000000000.0f;

const skill_id skill_firstaid( "firstaid" );
const skill_id skill_gun( "gun" );
const skill_id skill_throw( "throw" );

const efftype_id effect_bandaged( "bandaged" );
const efftype_id effect_bite( "bite" );
const efftype_id effect_bleed( "bleed" );
const efftype_id effect_bouldering( "bouldering" );
const efftype_id effect_catch_up( "catch_up" );
const efftype_id effect_hallu( "hallu" );
const efftype_id effect_hit_by_player( "hit_by_player" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_infection( "infection" );
const efftype_id effect_lying_down( "lying_down" );
const efftype_id effect_under_op( "under_operation" );
const efftype_id effect_no_sight( "no_sight" );
const efftype_id effect_stunned( "stunned" );
const efftype_id effect_onfire( "onfire" );
const efftype_id effect_npc_run_away( "npc_run_away" );
const efftype_id effect_npc_fire_bad( "npc_fire_bad" );
const efftype_id effect_npc_flee_player( "npc_flee_player" );
const efftype_id effect_npc_player_looking( "npc_player_still_looking" );

// power source CBMs
const bionic_id bio_advreactor( "bio_advreactor" );
const bionic_id bio_batteries( "bio_batteries" );
const bionic_id bio_ethanol( "bio_ethanol" );
const bionic_id bio_furnace( "bio_furnace" );
const bionic_id bio_metabolics( "bio_metabolics" );
const bionic_id bio_reactor( "bio_reactor" );
const bionic_id bio_torsionratchet( "bio_torsionratchet" );

// active defense CBMs - activate when in danger
const bionic_id bio_ads( "bio_ads" );
const bionic_id bio_faraday( "bio_faraday" );
const bionic_id bio_heat_absorb( "bio_heat_absorb" );
const bionic_id bio_heat_sink( "bio_heatsink" );
const bionic_id bio_ods( "bio_ods" );
const bionic_id bio_shock( "bio_shock" );

// special health CBMs - activate as needed
const bionic_id bio_painkiller( "bio_painkiller" );
const bionic_id bio_nanobots( "bio_nanobots" );
const bionic_id bio_radscrubber( "bio_radscrubber" );
const bionic_id bio_soporific( "bio_soporific" );

// health CBMs - always activate
const bionic_id bio_leukocyte( "bio_leukocyte" );
const bionic_id bio_plutfilter( "bio_plutfilter" );

// melee CBMs - activate for melee combat
const bionic_id bio_hydraulics( "bio_hydraulics" );

// weapon CBMs - activate in combat if they're better than what we have
const bionic_id bio_lightning( "bio_chain_lightning" );
const bionic_id bio_laser( "bio_laser" );
const bionic_id bio_blade( "bio_blade" );
const bionic_id bio_claws( "bio_claws" );

const ammotype reactor_slurry( "reactor_slurry" );
const ammotype plutonium( "plutonium" );

enum npc_action : int {
    npc_undecided = 0,
    npc_pause,
    npc_reload, npc_sleep,
    npc_pickup,
    npc_heal, npc_use_painkiller, npc_drop_items,
    npc_flee, npc_melee, npc_shoot,
    npc_look_for_player, npc_heal_player, npc_follow_player, npc_follow_embarked,
    npc_talk_to_player, npc_mug_player,
    npc_goto_destination,
    npc_avoid_friendly_fire,
    npc_escape_explosion,
    npc_noop,
    npc_reach_attack,
    npc_aim,
    npc_investigate_sound,
    npc_return_to_guard_pos,
    npc_player_activity,
    num_npc_actions
};

namespace
{
const std::vector<bionic_id> power_cbms = { {
        bio_advreactor,
        bio_batteries,
        bio_ethanol,
        bio_furnace,
        bio_metabolics,
        bio_reactor,
        bio_torsionratchet
    }
};
const std::vector<bionic_id> defense_cbms = { {
        bio_ads,
        bio_faraday,
        bio_heat_absorb,
        bio_heat_sink,
        bio_ods,
        bio_shock
    }
};

const std::vector<bionic_id> health_cbms = { {
        bio_leukocyte,
        bio_plutfilter
    }
};

// lightning, laser, blade, claws in order of use priority
const std::vector<bionic_id> weapon_cbms = { {
        bio_lightning,
        bio_laser,
        bio_blade,
        bio_claws
    }
};

const int avoidance_vehicles_radius = 5;

} // namespace

std::string npc_action_name( npc_action action );

void print_action( const char *prepend, npc_action action );

bool compare_sound_alert( const dangerous_sound &sound_a, const dangerous_sound &sound_b );

hp_part most_damaged_hp_part( const Character &c );

// Used in npc::drop_items()
struct ratio_index {
    double ratio;
    int index;
    ratio_index( double R, int I ) : ratio( R ), index( I ) {}
};

bool compare_sound_alert( const dangerous_sound &sound_a, const dangerous_sound &sound_b )
{
    if( sound_a.type != sound_b.type ) {
        return sound_a.type < sound_b.type;
    }
    return sound_a.volume < sound_b.volume;
}

static bool clear_shot_reach( const tripoint &from, const tripoint &to )
{
    std::vector<tripoint> path = line_to( from, to );
    path.pop_back();
    for( const tripoint &p : path ) {
        Creature *inter = g->critter_at( p );
        if( inter != nullptr ) {
            return false;
        } else if( g->m.impassable( p ) ) {
            return false;
        }
    }

    return true;
}

tripoint npc::good_escape_direction( bool include_pos )
{
    if( path.empty() ) {
        zone_type_id retreat_zone = zone_type_id( "NPC_RETREAT" );
        const tripoint &abs_pos = global_square_location();
        const zone_manager &mgr = zone_manager::get_manager();
        cata::optional<tripoint> retreat_target = mgr.get_nearest( retreat_zone, abs_pos, 60,
                fac_id );
        if( retreat_target && *retreat_target != abs_pos ) {
            update_path( g->m.getlocal( *retreat_target ) );
            if( !path.empty() ) {
                return path[0];
            }
        }
    }

    std::vector<tripoint> candidates;

    const auto rate_pt = [&]( const tripoint & pt, const float threat_val ) {
        if( !can_move_to( pt, !rules.has_flag( ally_rule::allow_bash ) ) ) {
            return MAX_FLOAT;
        }
        float rating = threat_val;
        for( const auto &e : g->m.field_at( pt ) ) {
            if( is_dangerous_field( e.second ) ) {
                // @todo: Rate fire higher than smoke
                rating += e.second.get_field_intensity();
            }
        }
        return rating;
    };

    float best_rating = include_pos ? rate_pt( pos(), 0.0f ) :  INT_MAX;
    candidates.emplace_back( pos() );

    std::map<direction, float> adj_map;
    for( direction pt_dir : npc_threat_dir ) {
        const tripoint &pt = pos() + direction_XY( pt_dir );
        float cur_rating = rate_pt( pt, ai_cache.threat_map[ pt_dir ] );
        adj_map[pt_dir] = cur_rating;
        if( cur_rating == best_rating ) {
            candidates.emplace_back( pos() + direction_XY( pt_dir ) );
        } else if( cur_rating < best_rating ) {
            candidates.clear();
            candidates.emplace_back( pos() + direction_XY( pt_dir ) );
            best_rating = cur_rating;
        }
    }
    return random_entry( candidates );
}

bool npc::sees_dangerous_field( const tripoint &p ) const
{
    return is_dangerous_fields( g->m.field_at( p ) );
}

bool npc::could_move_onto( const tripoint &p ) const
{
    if( !g->m.passable( p ) ) {
        return false;
    }

    if( !sees_dangerous_field( p ) ) {
        return true;
    }

    const auto fields_here = g->m.field_at( pos() );
    for( const auto &e : g->m.field_at( p ) ) {
        if( !is_dangerous_field( e.second ) ) {
            continue;
        }

        const auto *entry_here = fields_here.find_field( e.first );
        if( entry_here == nullptr || entry_here->get_field_intensity() < e.second.get_field_intensity() ) {
            return false;
        }
    }

    return true;
}

std::vector<sphere> npc::find_dangerous_explosives() const
{
    std::vector<sphere> result;

    const auto active_items = g->m.get_active_items_in_radius( pos(), MAX_VIEW_DISTANCE,
                              special_item_type::explosive );

    for( const auto &elem : active_items ) {
        const auto use = elem->type->get_use( "explosion" );

        if( !use ) {
            continue;
        }

        const explosion_iuse *actor = dynamic_cast<const explosion_iuse *>( use->get_actor_ptr() );
        const int safe_range = actor->explosion.safe_range();

        if( rl_dist( pos(), elem.position() ) >= safe_range ) {
            continue;   // Far enough.
        }

        const int turns_to_evacuate = 2 * safe_range / speed_rating();

        if( elem->charges > turns_to_evacuate ) {
            continue;   // Consider only imminent dangers.
        }

        result.emplace_back( elem.position(), safe_range );
    }

    return result;
}

float npc::evaluate_enemy( const Creature &target ) const
{
    if( target.is_monster() ) {
        const monster &mon = dynamic_cast<const monster &>( target );
        float diff = static_cast<float>( mon.type->difficulty );
        return std::min( diff, NPC_DANGER_MAX );
    } else if( target.is_npc() || target.is_player() ) {
        return std::min( character_danger( dynamic_cast<const player &>( target ) ),
                         NPC_DANGER_MAX );
    } else {
        return 0.0f;
    }
}

static constexpr int def_radius = 6;
static bool too_close( const tripoint &critter_pos, const tripoint &ally_pos )
{
    return rl_dist( critter_pos, ally_pos ) <= def_radius;
}

void npc::assess_danger()
{
    float assessment = 0.0f;
    float highest_priority = 1.0f;

    // Radius we can attack without moving
    const int max_range = std::max( weapon.reach_range( *this ),
                                    confident_shoot_range( weapon,
                                            get_most_accurate_sight( weapon ) ) );

    const auto ok_by_rules = [max_range, this]( const Creature & c, int dist, int scaled_dist ) {
        // If we're forbidden to attack, no need to check engagement rules
        if( rules.has_flag( ally_rule::forbid_engage ) ) {
            return false;
        }
        switch( rules.engagement ) {
            case ENGAGE_NONE:
                return false;
            case ENGAGE_CLOSE:
                // Either close to player or close enough that we can reach it and close to us
                return ( dist <= max_range && scaled_dist <= def_radius * 0.5 ) ||
                       too_close( c.pos(), g->u.pos() );
            case ENGAGE_WEAK:
                return c.get_hp() <= average_damage_dealt();
            case ENGAGE_HIT:
                return c.has_effect( effect_hit_by_player );
            case ENGAGE_NO_MOVE:
                return dist <= max_range;
            case ENGAGE_ALL:
                return true;
        }

        return true;
    };
    std::map<direction, float> cur_threat_map;
    // start with a decayed version of last turn's map
    for( direction threat_dir : npc_threat_dir ) {
        cur_threat_map[ threat_dir ] = 0.25f * ai_cache.threat_map[ threat_dir ];
    }
    // first, check if we're about to be consumed by fire
    // TODO: Use the field cache
    for( const tripoint &pt : g->m.points_in_radius( pos(), 6 ) ) {
        if( pt == pos() || g->m.has_flag( TFLAG_FIRE_CONTAINER,  pt ) ) {
            continue;
        }
        if( g->m.get_field( pt, fd_fire ) != nullptr ) {
            int dist = rl_dist( pos(), pt );
            cur_threat_map[direction_from( pos(), pt )] += 2.0f * ( NPC_DANGER_MAX - dist );
            if( dist < 3 && !has_effect( effect_npc_fire_bad ) ) {
                warn_about( "fire_bad", 1_minutes );
                add_effect( effect_npc_fire_bad, 5_turns );
                path.clear();
            }
        }
    }
    // find our Character friends and enemies
    std::vector<std::weak_ptr<Creature>> hostile_guys;
    for( const npc &guy : g->all_npcs() ) {
        if( &guy == this || !guy.is_active() ) {
            continue;
        }

        if( has_faction_relationship( guy, npc_factions::watch_your_back ) ) {
            ai_cache.friends.emplace_back( g->shared_from( guy ) );
        } else if( attitude_to( guy ) != A_NEUTRAL && sees( guy.pos() ) ) {
            hostile_guys.emplace_back( g->shared_from( guy ) );
        }
    }
    if( sees( g->u.pos() ) ) {
        if( is_enemy() ) {
            hostile_guys.emplace_back( g->shared_from( g->u ) );
        } else if( is_friendly( g->u ) ) {
            ai_cache.friends.emplace_back( g->shared_from( g->u ) );
        }
    }

    for( const monster &critter : g->all_monsters() ) {
        auto att = critter.attitude_to( *this );
        if( att == A_FRIENDLY ) {
            ai_cache.friends.emplace_back( g->shared_from( critter ) );
            continue;
        }
        if( att != A_HOSTILE && ( critter.friendly || !is_enemy() ) ) {
            continue;
        }
        if( !sees( critter ) ) {
            continue;
        }
        float critter_threat = evaluate_enemy( critter );
        // warn and consider the odds for distant enemies
        if( ( is_enemy() || !critter.friendly ) ) {
            assessment += critter_threat;
            if( critter_threat > ( 8.0f + personality.bravery + rng( 0, 5 ) ) ) {
                warn_about( "monster", 10_minutes, critter.type->nname() );
            }
        }

        int dist = rl_dist( pos(), critter.pos() );
        float scaled_distance = std::max( 1.0f, dist / critter.speed_rating() );
        float hp_percent = 1.0f - static_cast<float>( critter.get_hp() ) / critter.get_hp_max();
        float critter_danger = std::max( critter_threat * ( hp_percent * 0.5f + 0.5f ),
                                         NPC_DANGER_VERY_LOW );
        ai_cache.total_danger += critter_danger / scaled_distance;
        if( is_player_ally() && !ok_by_rules( critter, dist, scaled_distance ) ) {
            continue;
        }

        // don't ignore monsters that are too close or too close to an ally
        bool is_too_close = dist <= def_radius;
        const auto test_too_close = [critter, &is_too_close]( const std::weak_ptr<Creature> &guy ) {
            is_too_close |= too_close( critter.pos(), guy.lock().get()->pos() );
            return is_too_close;
        };
        std::any_of( ai_cache.friends.begin(), ai_cache.friends.end(), test_too_close );
        float priority = std::max( critter_danger - 2.0f * ( scaled_distance - 1.0f ),
                                   is_too_close ?  NPC_DANGER_VERY_LOW : 0.0f );
        cur_threat_map[direction_from( pos(), critter.pos() )] += priority;
        if( priority > highest_priority ) {
            highest_priority = priority;
            ai_cache.target = g->shared_from( critter );
            ai_cache.danger = critter_danger;
        }
    }

    if( assessment == 0.0 && hostile_guys.empty() ) {
        ai_cache.danger_assessment = assessment;
        return;
    }
    const auto handle_hostile = [&]( const player & foe, float foe_threat,
    const std::string & bogey, const std::string & warning ) {
        if( foe_threat > ( 8.0f + personality.bravery + rng( 0, 5 ) ) ) {
            warn_about( "monster", 10_minutes, bogey );
        }

        int dist = rl_dist( pos(), foe.pos() );
        int scaled_distance = std::max( 1, ( 100 * dist ) / foe.get_speed() );
        ai_cache.total_danger += foe_threat / scaled_distance;
        if( !is_player_ally() || ok_by_rules( foe, dist, scaled_distance ) ) {
            bool is_too_close = dist <= def_radius;
            for( const std::weak_ptr<Creature> guy : ai_cache.friends ) {
                is_too_close |= too_close( foe.pos(), guy.lock().get()->pos() );
                if( is_too_close ) {
                    break;
                }
            }
            float priority = std::max( foe_threat - 2.0f * ( scaled_distance - 1 ),
                                       is_too_close ?  NPC_DANGER_VERY_LOW : 0.0f );
            cur_threat_map[direction_from( pos(), foe.pos() )] += priority;
            if( priority > highest_priority ) {
                warn_about( warning, 1_minutes );
                highest_priority = priority;
                ai_cache.danger = foe_threat;
                ai_cache.target = g->shared_from( foe );
            }
        }
        return foe_threat;
    };

    for( const std::weak_ptr<Creature> &guy : hostile_guys ) {
        player *foe = dynamic_cast<player *>( guy.lock().get() );
        if( foe && foe->is_npc() ) {
            assessment += handle_hostile( *foe, evaluate_enemy( *foe ), "bandit", "kill_npc" );
        }
    }

    for( const std::weak_ptr<Creature> &guy : ai_cache.friends ) {
        player *ally = dynamic_cast<player *>( guy.lock().get() );
        if( !( ally && ally->is_npc() ) ) {
            continue;
        }
        float guy_threat = evaluate_enemy( *ally );
        float min_danger = assessment >= NPC_DANGER_VERY_LOW ? NPC_DANGER_VERY_LOW : -10.0f;
        assessment = std::max( min_danger, assessment - guy_threat * 0.5f );
    }

    if( sees( g->u.pos() ) ) {
        // Mod for the player
        // cap player difficulty at 150
        float player_diff = evaluate_enemy( g->u );
        if( is_enemy() ) {
            assessment += handle_hostile( g->u, player_diff, "maniac", "kill_player" );
        } else if( is_friendly( g->u ) ) {
            float min_danger = assessment >= NPC_DANGER_VERY_LOW ? NPC_DANGER_VERY_LOW : -10.0f;
            assessment = std::max( min_danger, assessment - player_diff * 0.5f );
            ai_cache.friends.emplace_back( g->shared_from( g->u ) );
        }
    }
    assessment *= 0.1f;
    if( !has_effect( effect_npc_run_away ) && !has_effect( effect_npc_fire_bad ) ) {
        float my_diff = evaluate_enemy( *this );
        if( ( my_diff * 0.5f + personality.bravery + rng( 0, 10 ) ) < assessment ) {
            time_duration run_away_for = 5_turns + 1_turns * rng( 0, 5 );
            warn_about( "run_away", run_away_for );
            add_effect( effect_npc_run_away, run_away_for );
            path.clear();
        }
    }
    // update the threat cache
    for( size_t i = 0; i < 8; i++ ) {
        direction threat_dir = npc_threat_dir[i];
        direction dir_right = npc_threat_dir[( i + 1 ) % 8];
        direction dir_left = npc_threat_dir[( i + 7 ) % 8 ];
        ai_cache.threat_map[threat_dir] = cur_threat_map[threat_dir] + 0.1f *
                                          ( cur_threat_map[dir_right] + cur_threat_map[dir_left] );
    }
    if( assessment <= 2.0f ) {
        assessment = -10.0f + 5.0f * assessment; // Low danger if no monsters around
    }
    ai_cache.danger_assessment = assessment;
}

float npc::character_danger( const Character &uc ) const
{
    // TODO: Remove this when possible
    const player &u = dynamic_cast<const player &>( uc );
    float ret = 0.0;
    bool u_gun = u.weapon.is_gun();
    bool my_gun = weapon.is_gun();
    double u_weap_val = u.weapon_value( u.weapon );
    const double &my_weap_val = ai_cache.my_weapon_value;
    if( u_gun && !my_gun ) {
        u_weap_val *= 1.5f;
    }
    ret += u_weap_val;

    ret += hp_percentage() * get_hp_max( hp_torso ) / 100.0 / my_weap_val;

    ret += my_gun ? u.get_dodge() / 2 : u.get_dodge();

    ret *= std::max( 0.5, u.get_speed() / 100.0 );

    add_msg( m_debug, "%s danger: %1f", u.disp_name(), ret );
    return ret;
}

void npc::regen_ai_cache()
{
    auto i = std::begin( ai_cache.sound_alerts );
    while( i != std::end( ai_cache.sound_alerts ) ) {
        if( sees( g->m.getlocal( i->abs_pos ) ) ) {
            // if they were responding to a call for guards because of thievery
            npc *const sound_source = g->critter_at<npc>( g->m.getlocal( i->abs_pos ) );
            if( sound_source ) {
                if( my_fac == sound_source->my_fac && sound_source->known_stolen_item ) {
                    sound_source->known_stolen_item = nullptr;
                    set_attitude( NPCATT_RECOVER_GOODS );
                }
            }
            i = ai_cache.sound_alerts.erase( i );
            if( ai_cache.sound_alerts.size() == 1 ) {
                path.clear();
            }
        } else {
            ++i;
        }
    }
    float old_assessment = ai_cache.danger_assessment;
    ai_cache.friends.clear();
    ai_cache.target = std::shared_ptr<Creature>();
    ai_cache.ally = std::shared_ptr<Creature>();
    ai_cache.can_heal.clear_all();
    ai_cache.danger = 0.0f;
    ai_cache.total_danger = 0.0f;
    ai_cache.my_weapon_value = weapon_value( weapon );
    ai_cache.dangerous_explosives = find_dangerous_explosives();

    assess_danger();
    if( old_assessment > NPC_DANGER_VERY_LOW && ai_cache.danger_assessment <= 0 ) {
        warn_about( "relax", 30_minutes );
    } else if( old_assessment <= 0.0f && ai_cache.danger_assessment > NPC_DANGER_VERY_LOW ) {
        warn_about( "general_danger" );
    }
    // Non-allied NPCs with a completed mission should move to the player
    if( !is_player_ally() && !is_stationary( true ) ) {
        for( auto &miss : chatbin.missions_assigned ) {
            if( miss->is_complete( getID() ) ) {
                // unless the player found an item and already told the NPC he wanted to keep it
                const mission_goal &mgoal = miss->get_type().goal;
                if( ( mgoal == MGOAL_FIND_ITEM || mgoal == MGOAL_FIND_ANY_ITEM ||
                      mgoal == MGOAL_FIND_ITEM_GROUP ) &&
                    has_effect( effect_npc_player_looking ) ) {
                    continue;
                }
                if( global_omt_location() != g->u.global_omt_location() ) {
                    goal = g->u.global_omt_location();
                }
                set_attitude( NPCATT_TALK );
                break;
            }
        }
    }
}

void npc::move()
{
    if( attitude == NPCATT_FLEE ) {
        set_attitude( NPCATT_FLEE_TEMP );  // Only run for so many hours
    } else if( attitude == NPCATT_FLEE_TEMP && !has_effect( effect_npc_flee_player ) ) {
        set_attitude( NPCATT_NULL );
    }
    regen_ai_cache();
    adjust_power_cbms();

    if( activity.id() == "ACT_OPERATION" ) {
        execute_action( npc_player_activity );
        return;// NPCs under operation should just stay still
    }

    npc_action action = npc_undecided;

    static const std::string no_target_str = "none";
    const Creature *target = current_target();
    const std::string &target_name = target != nullptr ? target->disp_name() : no_target_str;
    add_msg( m_debug, "NPC %s: target = %s, danger = %.1f, range = %d",
             name, target_name, ai_cache.danger, confident_shoot_range( weapon, recoil_total() ) );

    //faction opinion determines if it should consider you hostile
    if( !is_enemy() && guaranteed_hostile() && sees( g->u ) ) {
        add_msg( m_debug, "NPC %s turning hostile because is guaranteed_hostile()", name );
        if( op_of_u.fear > 10 + personality.aggression + personality.bravery ) {
            set_attitude( NPCATT_FLEE_TEMP );    // We don't want to take u on!
        } else {
            set_attitude( NPCATT_KILL );    // Yeah, we think we could take you!
        }
    }

    /* This bypasses the logic to determine the npc action, but this all needs to be rewritten
     * anyway.
     * NPC won't avoid dangerous terrain while accompanying the player inside a vehicle to keep
     * them from inadvertantly getting themselves run over and/or cause vehicle related errors.
     * NPCs flee from uncontained fires within 3 tiles
     */
    if( !in_vehicle && ( sees_dangerous_field( pos() ) || has_effect( effect_npc_fire_bad ) ) ) {
        if( sees_dangerous_field( pos() ) ) {
            path.clear();
        }
        const tripoint escape_dir = good_escape_direction( sees_dangerous_field( pos() ) );
        if( escape_dir != pos() ) {
            move_to( escape_dir );
            return;
        }
    }

    // TODO: Place player-aiding actions here, with a weight

    /* NPCs are fairly suicidal so at this point we will do a quick check to see if
     * something nasty is going to happen.
     */

    if( is_enemy() && vehicle_danger( avoidance_vehicles_radius ) > 0 ) {
        // TODO: Think about how this actually needs to work, for now assume flee from player
        ai_cache.target = g->shared_from( g->u );
    }

    if( !ai_cache.dangerous_explosives.empty() ) {
        action = npc_escape_explosion;
    } else if( target == &g->u && attitude == NPCATT_FLEE_TEMP ) {
        action = method_of_fleeing();
    } else if( has_effect( effect_npc_run_away ) ) {
        action = method_of_fleeing();
    } else if( target != nullptr && ai_cache.danger > 0 ) {
        action = method_of_attack();
    } else if( !ai_cache.sound_alerts.empty() && !is_walking_with() ) {
        tripoint cur_s_abs_pos = ai_cache.s_abs_pos;
        if( !ai_cache.guard_pos ) {
            ai_cache.guard_pos = g->m.getabs( pos() );
        }
        if( ai_cache.sound_alerts.size() > 1 ) {
            std::sort( ai_cache.sound_alerts.begin(), ai_cache.sound_alerts.end(),
                       compare_sound_alert );
            if( ai_cache.sound_alerts.size() > 10 ) {
                ai_cache.sound_alerts.resize( 10 );
            }
        }
        action = npc_investigate_sound;
        if( ai_cache.sound_alerts.front().abs_pos != cur_s_abs_pos ) {
            ai_cache.stuck = 0;
            ai_cache.s_abs_pos = ai_cache.sound_alerts.front().abs_pos;
        } else if( ai_cache.stuck > 10 ) {
            ai_cache.stuck = 0;
            if( ai_cache.sound_alerts.size() == 1 ) {
                ai_cache.sound_alerts.clear();
                action = npc_return_to_guard_pos;
            } else {
                ai_cache.s_abs_pos = ai_cache.sound_alerts.at( 1 ).abs_pos;
            }
        }
        if( action == npc_investigate_sound ) {
            add_msg( m_debug, "NPC %s: investigating sound at x(%d) y(%d)", name,
                     ai_cache.s_abs_pos.x, ai_cache.s_abs_pos.y );
        }
    } else if( ai_cache.sound_alerts.empty() && ai_cache.guard_pos ) {
        tripoint return_guard_pos = *ai_cache.guard_pos;
        add_msg( m_debug, "NPC %s: returning to guard spot at x(%d) y(%d)", name,
                 return_guard_pos.x, return_guard_pos.y );
        action = npc_return_to_guard_pos;
    } else {
        // No present danger
        deactivate_combat_cbms();

        action = address_needs();
        print_action( "address_needs %s", action );

        if( action == npc_undecided ) {
            action = address_player();
            print_action( "address_player %s", action );
        }
    }

    if( action == npc_undecided && attitude == NPCATT_ACTIVITY ) {
        std::vector<tripoint> activity_route = get_auto_move_route();
        if( !activity_route.empty() && !has_destination_activity() ) {
            tripoint final_destination;
            if( destination_point ) {
                final_destination = g->m.getlocal( *destination_point );
            } else {
                final_destination = activity_route.back();
            }
            update_path( final_destination );
            if( !path.empty() ) {
                move_to_next();
                return;
            }
        }
        if( has_destination_activity() ) {
            start_destination_activity();
            action = npc_player_activity;
        } else if( has_player_activity() ) {
            action = npc_player_activity;
        }
    }

    if( action == npc_undecided ) {
        // an interrupted activity can cause this situation. stops allied NPCs zooming off like random NPCs
        if( attitude == NPCATT_ACTIVITY && !activity ) {
            revert_after_activity();
            if( is_ally( g->u ) ) {
                attitude = NPCATT_FOLLOW;
                mission = NPC_MISSION_NULL;
            }
        }
        if( is_stationary( true ) ) {
            // if we're in a vehicle, stay in the vehicle
            if( in_vehicle ) {
                action = npc_pause;
                goal = global_omt_location();
            } else {
                action = goal == global_omt_location() ?  npc_pause : npc_goto_destination;
            }
        } else if( has_new_items && scan_new_items() ) {
            return;
        } else if( !fetching_item ) {
            find_item();
            print_action( "find_item %s", action );
        }

        // check if in vehicle before rushing off to fetch things
        if( is_walking_with() && g->u.in_vehicle ) {
            action = npc_follow_embarked;
        } else if( fetching_item ) {
            // Set to true if find_item() found something
            action = npc_pickup;
        } else if( is_following() ) {
            // No items, so follow the player?
            action = npc_follow_player;
        }

        if( action == npc_undecided ) {
            // Do our long-term action
            action = long_term_goal_action();
            print_action( "long_term_goal_action %s", action );
        }
    }

    /* Sometimes we'll be following the player at this point, but close enough that
     * "following" means standing still.  If that's the case, if there are any
     * monsters around, we should attack them after all!
     *
     * If we are following a embarked player and we are in a vehicle then shoot anyway
     * as we are most likely riding shotgun
     */
    if( ai_cache.danger > 0 && target != nullptr &&
        (
            ( action == npc_follow_embarked && in_vehicle ) ||
            ( action == npc_follow_player &&
              ( rl_dist( pos(), g->u.pos() ) <= follow_distance() || posz() != g->u.posz() ) )
        ) ) {
        action = method_of_attack();
    }

    add_msg( m_debug, "%s chose action %s.", name, npc_action_name( action ) );

    execute_action( action );
}

void npc::execute_action( npc_action action )
{
    int oldmoves = moves;
    tripoint tar = pos();
    Creature *cur = current_target();
    player *patient = dynamic_cast<player *>( current_ally() );
    if( action == npc_flee ) {
        tar = good_escape_direction( false );
    } else if( cur != nullptr ) {
        tar = cur->pos();
    }
    /*
      debugmsg("%s ran execute_action() with target = %d! Action %s",
               name, target, npc_action_name(action));
    */

    switch( action ) {

        case npc_pause:
            move_pause();
            break;

        case npc_reload: {
            do_reload( weapon );
        }
        break;

        case npc_investigate_sound: {
            tripoint cur_pos = pos();
            update_path( g->m.getlocal( ai_cache.s_abs_pos ) );
            move_to_next();
            if( pos() == cur_pos ) {
                ai_cache.stuck += 1;
            }
        }
        break;

        case npc_return_to_guard_pos: {
            const tripoint local_guard_pos = g->m.getlocal( *ai_cache.guard_pos );
            update_path( local_guard_pos );
            if( pos() == local_guard_pos || path.empty() ) {
                move_pause();
                ai_cache.guard_pos = cata::nullopt;
                path.clear();
            } else {
                move_to_next();
            }
        }
        break;

        case npc_sleep: {
            // TODO: Allow stims when not too tired
            // Find a nice spot to sleep
            int best_sleepy = sleep_spot( pos() );
            tripoint best_spot = pos();
            const auto points = closest_tripoints_first( 6, pos() );
            for( const tripoint &p : points ) {
                if( !could_move_onto( p ) || !g->is_empty( p ) ) {
                    continue;
                }

                // TODO: Blankets when it's cold
                const int sleepy = sleep_spot( p );
                if( sleepy > best_sleepy ) {
                    best_sleepy = sleepy;
                    best_spot = p;
                }
            }
            if( is_walking_with() ) {
                complain_about( "napping", 30_minutes, _( "<warn_sleep>" ) );
            }
            update_path( best_spot );
            // TODO: Handle empty path better
            if( best_spot == pos() || path.empty() ) {
                move_pause();
                if( !has_effect( effect_lying_down ) ) {
                    activate_bionic_by_id( bio_soporific );
                    add_effect( effect_lying_down, 30_minutes, num_bp, false, 1 );
                    if( g->u.sees( *this ) && !g->u.in_sleep_state() ) {
                        add_msg( _( "%s lies down to sleep." ), name );
                    }
                }
            } else {
                move_to_next();
            }
        }
        break;

        case npc_pickup:
            pick_up_item();
            break;

        case npc_heal:
            heal_self();
            break;

        case npc_use_painkiller:
            use_painkiller();
            break;

        case npc_drop_items:
            /*
              drop_items(weight_carried() - weight_capacity(),
                            volume_carried() - volume_capacity());
            */
            move_pause();
            break;

        case npc_flee:
            if( path.empty() ) {
                move_to( tar );
            } else {
                move_to_next();
            }
            break;

        case npc_reach_attack:
            if( weapon.reach_range( *this ) >= rl_dist( pos(), tar ) &&
                clear_shot_reach( pos(), tar ) ) {
                if( can_use_offensive_cbm() ) {
                    activate_bionic_by_id( bio_hydraulics );
                }
                reach_attack( tar );
                break;
            }
        /* fallthrough */
        case npc_melee:
            update_path( tar );
            if( path.size() > 1 ) {
                move_to_next();
            } else if( path.size() == 1 ) {
                if( cur != nullptr ) {
                    if( can_use_offensive_cbm() ) {
                        activate_bionic_by_id( bio_hydraulics );
                    }
                    melee_attack( *cur, true );
                }
            } else {
                look_for_player( g->u );
            }
            break;

        case npc_aim:
            aim();
            break;

        case npc_shoot: {
            auto mode = weapon.gun_current_mode();
            if( !mode ) {
                debugmsg( "NPC tried to shoot without valid mode" );
                break;
            }
            aim();
            if( is_hallucination() ) {
                pretend_fire( this, mode.qty, *mode );
            } else {
                fire_gun( tar, mode.qty, *mode );
                // "discard" the fake bio weapon after shooting it
                if( cbm_weapon_index >= 0 ) {
                    discharge_cbm_weapon();
                }
            }
            break;
        }

        case npc_look_for_player:
            if( saw_player_recently() && last_player_seen_pos && sees( *last_player_seen_pos ) ) {
                update_path( *last_player_seen_pos );
                move_to_next();
            } else {
                look_for_player( g->u );
            }
            break;

        case npc_heal_player:
            update_path( patient->pos() );
            if( path.size() == 1 ) { // We're adjacent to u, and thus can heal u
                heal_player( *patient );
            } else if( !path.empty() ) {
                say( string_format( _( "Hold still %s, I'm coming to help you." ),
                                    patient->disp_name() ) );
                move_to_next();
            } else {
                move_pause();
            }
            break;

        case npc_follow_player:
            update_path( g->u.pos() );
            if( static_cast<int>( path.size() ) <= follow_distance() &&
                g->u.posz() == posz() ) { // We're close enough to u.
                move_pause();
            } else if( !path.empty() ) {
                move_to_next();
            } else {
                move_pause();
            }
            // TODO: Make it only happen when it's safe
            complain();
            break;

        case npc_follow_embarked: {
            const optional_vpart_position vp = g->m.veh_at( g->u.pos() );

            if( !vp ) {
                debugmsg( "Following an embarked player with no vehicle at their location?" );
                // TODO: change to wait? - for now pause
                move_pause();
                break;
            }
            vehicle *const veh = &vp->vehicle();

            // Try to find the last destination
            // This is mount point, not actual position
            point last_dest( INT_MIN, INT_MIN );
            if( !path.empty() && veh_pointer_or_null( g->m.veh_at( path[path.size() - 1] ) ) == veh ) {
                last_dest = vp->mount();
            }

            // Prioritize last found path, then seats
            // Don't change spots if ours is nice
            int my_spot = -1;
            std::vector<std::pair<int, int> > seats;
            for( const vpart_reference &vp : veh->get_avail_parts( VPFLAG_BOARDABLE ) ) {
                const player *passenger = veh->get_passenger( vp.part_index() );
                if( passenger != this && passenger != nullptr ) {
                    continue;
                }

                // a seat is available if either unassigned or assigned to us
                auto available_seat = [&]( const vehicle_part & pt ) {
                    if( !pt.is_seat() ) {
                        return false;
                    }
                    const npc *who = pt.crew();
                    return !who || who->getID() == getID();
                };

                const vehicle_part &pt = vp.part();

                int priority = 0;

                if( vp.mount() == last_dest ) {
                    // Shares mount point with last known path
                    // We probably wanted to go there in the last turn
                    priority = 4;

                } else if( available_seat( pt ) ) {
                    // Assuming player "owns" a sensible vehicle seats should be in good spots to occupy
                    // Prefer our assigned seat if we have one
                    const npc *who = pt.crew();
                    priority = who && who->getID() == getID() ? 3 : 2;

                } else if( vp.is_inside() ) {
                    priority = 1;
                }

                if( passenger == this ) {
                    my_spot = priority;
                }

                seats.push_back( std::make_pair( priority, static_cast<int>( vp.part_index() ) ) );
            }

            if( my_spot >= 3 ) {
                // We won't get any better, so don't try
                move_pause();
                break;
            }

            std::sort( seats.begin(), seats.end(),
            []( const std::pair<int, int> &l, const std::pair<int, int> &r ) {
                return l.first > r.first;
            } );

            if( seats.empty() ) {
                // TODO: be angry at player, switch to wait or leave - for now pause
                move_pause();
                break;
            }

            // Only check few best seats - pathfinding can get expensive
            const size_t try_max = std::min<size_t>( 4, seats.size() );
            for( size_t i = 0; i < try_max; i++ ) {
                if( seats[i].first <= my_spot ) {
                    // We have a nicer spot than this
                    // Note: this will make NPCs steal player's seat...
                    break;
                }

                const int cur_part = seats[i].second;

                tripoint pp = veh->global_part_pos3( cur_part );
                update_path( pp, true );
                if( !path.empty() ) {
                    // All is fine
                    move_to_next();
                    break;
                }
            }

            // TODO: Check the rest
            move_pause();
        }

        break;
        case npc_talk_to_player:
            talk_to_u();
            moves = 0;
            break;

        case npc_mug_player:
            mug_player( g->u );
            break;

        case npc_goto_destination:
            go_to_omt_destination();
            break;

        case npc_avoid_friendly_fire:
            avoid_friendly_fire();
            break;

        case npc_escape_explosion:
            escape_explosion();
            break;

        case npc_player_activity:
            do_player_activity();
            break;

        case npc_undecided:
            complain();
            move_pause();
            break;

        case npc_noop:
            add_msg( m_debug, "%s skips turn (noop)", disp_name() );
            return;

        default:
            debugmsg( "Unknown NPC action (%d)", action );
    }

    if( oldmoves == moves ) {
        add_msg( m_debug, "NPC didn't use its moves.  Action %s (%d).",
                 npc_action_name( action ), action );
    }
}

void npc::witness_thievery( item *it )
{
    // Shopkeep is behind glass
    if( myclass == npc_class_id( "NC_EVAC_SHOPKEEP" ) ) {
        known_stolen_item = it;
        return;
    }
    set_attitude( NPCATT_RECOVER_GOODS );
    known_stolen_item = it;
    for( auto &elem : g->u.inv_dump() ) {
        if( elem->get_old_owner() ) {
        }
    }
}

npc_action npc::method_of_fleeing()
{
    if( in_vehicle ) {
        return npc_undecided;
    }
    return npc_flee;
}

npc_action npc::method_of_attack()
{
    Creature *critter = current_target();
    if( critter == nullptr ) {
        // This function shouldn't be called...
        debugmsg( "Ran npc::method_of_attack without a target!" );
        return npc_pause;
    }

    tripoint tar = critter->pos();
    int dist = rl_dist( pos(), tar );
    double danger = evaluate_enemy( *critter );

    // TODO: Change the in_vehicle check to actual "are we driving" check
    const bool dont_move = in_vehicle || rules.engagement == ENGAGE_NO_MOVE;

    // if there's enough of a threat to be here, power up the combat CBMs
    activate_combat_cbms();

    int ups_charges = charges_of( "UPS" );

    // get any suitable modes excluding melee, any forbidden to NPCs and those without ammo
    // if we require a silent weapon inappropriate modes are also removed
    // except in emergency only fire bursts if danger > 0.5 and don't shoot at all at harmless targets
    std::vector<std::pair<gun_mode_id, gun_mode>> modes;
    if( rules.has_flag( ally_rule::use_guns ) || !is_player_ally() ) {
        for( const auto &e : weapon.gun_all_modes() ) {
            modes.push_back( e );
        }

        modes.erase( std::remove_if( modes.begin(), modes.end(),
        [&]( const std::pair<gun_mode_id, gun_mode> &e ) {

            const auto &m = e.second;
            return m.melee() || m.flags.count( "NPC_AVOID" ) ||
                   !m->ammo_sufficient( m.qty ) || !can_use( *m.target ) ||
                   m->get_gun_ups_drain() > ups_charges ||
                   ( danger <= ( ( m.qty == 1 ) ? 0.0 : 0.5 ) && !emergency() ) ||
                   ( rules.has_flag( ally_rule::use_silent ) && is_player_ally() &&
                     !m.target->is_silent() );

        } ), modes.end() );
    }

    // prefer modes that result in more total damage
    std::stable_sort( modes.begin(),
                      modes.end(), [&]( const std::pair<gun_mode_id, gun_mode> &lhs,
    const std::pair<gun_mode_id, gun_mode> &rhs ) {
        return ( lhs.second->gun_damage().total_damage() * lhs.second.qty ) >
               ( rhs.second->gun_damage().total_damage() * rhs.second.qty );
    } );

    const int cur_recoil = recoil_total();
    // modes outside confident range should always be the last option(s)
    std::stable_sort( modes.begin(),
                      modes.end(), [&]( const std::pair<gun_mode_id, gun_mode> &lhs,
    const std::pair<gun_mode_id, gun_mode> &rhs ) {
        return ( confident_gun_mode_range( lhs.second, cur_recoil ) >= dist ) >
               ( confident_gun_mode_range( rhs.second, cur_recoil ) >= dist );
    } );

    if( emergency() && alt_attack() ) {
        add_msg( m_debug, "%s is trying an alternate attack", disp_name() );
        return npc_noop;
    }

    // reach attacks are silent and consume no ammo so prefer these if available
    int reach_range = weapon.reach_range( *this );
    if( reach_range > 1 && reach_range >= dist && clear_shot_reach( pos(), tar ) ) {
        add_msg( m_debug, "%s is trying a reach attack", disp_name() );
        return npc_reach_attack;
    }

    // if the best mode is within the confident range try for a shot
    if( !modes.empty() && sees( *critter ) &&
        confident_gun_mode_range( modes[ 0 ].second, cur_recoil ) >= dist ) {

        // TODO: Make NPCs understand reinforced glass and vehicles blocking line of fire

        if( wont_hit_friend( tar, weapon, false ) ) {
            weapon.gun_set_mode( modes[ 0 ].first );
            add_msg( m_debug, "%s is trying to shoot someone", disp_name() );
            return npc_shoot;

        } else {
            if( !dont_move ) {
                add_msg( m_debug, "%s is trying to avoid friendly fire", disp_name() );
                return npc_avoid_friendly_fire;
            }
        }
    }

    if( dist == 1 ) {
        add_msg( m_debug, "%s is trying a melle attack", disp_name() );
        return npc_melee;
    }

    // don't mess with CBM weapons
    if( cbm_weapon_index < 0 ) {
        // TODO: Add a time check now that wielding takes a lot of time
        if( wield_better_weapon() ) {
            add_msg( m_debug, "%s is changing weapons", disp_name() );
            return npc_noop;
        }

        if( !weapon.ammo_sufficient() && can_reload_current() ) {
            add_msg( m_debug, "%s is reloading", disp_name() );
            return npc_reload;
        }
    }

    // TODO: Needs a check for transparent but non-passable tiles on the way
    if( !modes.empty() && sees( *critter ) &&
        aim_per_move( weapon, recoil ) > 0 &&
        confident_shoot_range( weapon, get_most_accurate_sight( weapon ) ) >= dist ) {
        add_msg( m_debug, "%s is aiming" );
        return npc_aim;
    }
    add_msg( m_debug, "%s can't figure out what to do", disp_name() );
    return dont_move ? npc_undecided : npc_melee;
}

bool npc::need_heal( const player &n )
{
    // if there are no healing items, there's nothing to do
    if( !( ai_cache.can_heal.bandage || ai_cache.can_heal.bleed || ai_cache.can_heal.bite ||
           ai_cache.can_heal.infect ) ) {
        return false;
    }
    // NPCs heal at 50% remaining HP, minus their bravery and skill in first aid
    // brave NPCs are less worried, skilled NPCs conserve bandages
    int threshold = 50 + personality.bravery - get_skill_level( skill_firstaid );
    // altruist NPCs hold off on healing themselves and heal allies earlier
    if( n.getID() == getID() ) {
        threshold -= personality.altruism;
    } else {
        threshold += personality.altruism;
    }

    for( int i = 0; i < num_hp_parts; i++ ) {
        const hp_part part = static_cast<hp_part>( i );
        const body_part bp_wounded = hp_to_bp( part );

        if( ai_cache.can_heal.bleed && n.has_effect( effect_bleed, bp_wounded ) ) {
            ai_cache.can_heal.clear_all();
            ai_cache.can_heal.bleed = true;
            return true;
        }

        // NPCs don't reapply bandages
        if( ai_cache.can_heal.bandage && !n.has_effect( effect_bandaged, bp_wounded ) ) {
            int part_threshold = threshold;
            if( part == hp_head ) {
                part_threshold += 20;
            } else if( part == hp_torso ) {
                part_threshold += 10;
            }
            part_threshold = std::min( 80, part_threshold );
            part_threshold = part_threshold * n.hp_max[i] / 100;

            if( n.hp_cur[i] <= part_threshold ) {
                ai_cache.can_heal.clear_all();
                ai_cache.can_heal.bandage = true;
                return true;
            }
        }

        if( ai_cache.can_heal.bite && n.has_effect( effect_bite, bp_wounded ) ) {
            ai_cache.can_heal.clear_all();
            ai_cache.can_heal.bite = true;
            return true;

        }

        if( ai_cache.can_heal.infect && n.has_effect( effect_infected, bp_wounded ) ) {
            ai_cache.can_heal.clear_all();
            ai_cache.can_heal.infect = true;
            return true;
        }
    }

    return false;
}

npc_action npc::address_needs()
{
    return address_needs( ai_cache.danger );
}

static bool wants_to_reload( const npc &who, const item &it )
{
    if( !who.can_reload( it ) ) {
        return false;
    }

    const int required = it.ammo_required();
    // TODO: Add bandolier check here, once they can be reloaded
    if( required < 1 && !it.is_magazine() ) {
        return false;
    }

    const int remaining = it.ammo_remaining();
    return remaining < required || remaining < it.ammo_capacity();
}

static bool wants_to_reload_with( const item &weap, const item &ammo )
{
    return !ammo.is_magazine() || ammo.ammo_remaining() > weap.ammo_remaining();
}

item &npc::find_reloadable()
{
    auto cached_value = cached_info.find( "reloadables" );
    if( cached_value != cached_info.end() ) {
        return null_item_reference();
    }
    // Check wielded gun, non-wielded guns, mags and tools
    // TODO: Build a proper gun->mag->ammo DAG (Directed Acyclic Graph)
    // to avoid checking same properties over and over
    // TODO: Make this understand bandoliers, pouches etc.
    // TODO: Cache items checked for reloading to avoid re-checking same items every turn
    // TODO: Make it understand smaller and bigger magazines
    item *reloadable = nullptr;
    visit_items( [this, &reloadable]( item * node ) {
        if( !wants_to_reload( *this, *node ) ) {
            return VisitResponse::NEXT;
        }
        const auto it_loc = select_ammo( *node ).ammo;
        if( it_loc && wants_to_reload_with( *node, *it_loc ) ) {
            reloadable = node;
            return VisitResponse::ABORT;
        }

        return VisitResponse::NEXT;
    } );

    if( reloadable != nullptr ) {
        return *reloadable;
    }

    cached_info.emplace( "reloadables", 0.0 );
    return null_item_reference();
}

const item &npc::find_reloadable() const
{
    return const_cast<const item &>( const_cast<npc *>( this )->find_reloadable() );
}

bool npc::can_reload_current()
{
    if( !weapon.is_gun() || !wants_to_reload( *this, weapon ) ) {
        return false;
    }

    return static_cast<bool>( find_usable_ammo( weapon ) );
}

item_location npc::find_usable_ammo( const item &weap )
{
    if( !can_reload( weap ) ) {
        return item_location();
    }

    auto loc = select_ammo( weap ).ammo;
    if( !loc || !wants_to_reload_with( weap, *loc ) ) {
        return item_location();
    }

    return loc;
}

item_location npc::find_usable_ammo( const item &weap ) const
{
    return const_cast<npc *>( this )->find_usable_ammo( weap );
}

void npc::adjust_power_cbms()
{
    if( !is_player_ally() || wants_to_recharge_cbm() ) {
        return;
    }
    for( const bionic_id &cbm_id : power_cbms ) {
        deactivate_bionic_by_id( cbm_id );
    }
}

void npc::activate_combat_cbms()
{
    for( const bionic_id &cbm_id : defense_cbms ) {
        activate_bionic_by_id( cbm_id );
    }
    if( can_use_offensive_cbm() ) {
        for( const bionic_id &cbm_id : weapon_cbms ) {
            check_or_use_weapon_cbm( cbm_id );
        }
    }
}

void npc::deactivate_combat_cbms()
{
    for( const bionic_id &cbm_id : defense_cbms ) {
        deactivate_bionic_by_id( cbm_id );
    }
    deactivate_bionic_by_id( bio_hydraulics );
    for( const bionic_id &cbm_id : weapon_cbms ) {
        deactivate_bionic_by_id( cbm_id );
    }
    cbm_weapon_index = -1;
}

bool npc::activate_bionic_by_id( const bionic_id &cbm_id, bool eff_only )
{
    int index = 0;
    for( auto &i : *my_bionics ) {
        if( i.id == cbm_id ) {
            if( !i.powered ) {
                return activate_bionic( index, eff_only );
            } else {
                return false;
            }
        }
        index += 1;
    }
    return false;
}

bool npc::use_bionic_by_id( const bionic_id &cbm_id, bool eff_only )
{
    int index = 0;
    for( auto &i : *my_bionics ) {
        if( i.id == cbm_id ) {
            if( !i.powered ) {
                return activate_bionic( index, eff_only );
            } else {
                return true;
            }
        }
        index += 1;
    }
    return false;
}

bool npc::deactivate_bionic_by_id( const bionic_id &cbm_id, bool eff_only )
{
    int index = 0;
    for( auto &i : *my_bionics ) {
        if( i.id == cbm_id ) {
            if( i.powered ) {
                return deactivate_bionic( index, eff_only );
            } else {
                return false;
            }
        }
        index += 1;
    }
    return false;
}

bool npc::wants_to_recharge_cbm()
{
    return power_level < ( max_power_level * static_cast<int>( rules.cbm_recharge ) / 100 );
}

bool npc::can_use_offensive_cbm() const
{
    return power_level > ( max_power_level * static_cast<int>( rules.cbm_reserve ) / 100 );
}

bool npc::consume_cbm_items( const std::function<bool( const item & )> &filter )
{
    invslice slice = inv.slice();
    int index = -1;
    for( size_t i = 0; i < slice.size(); i++ ) {
        const item &it = slice[i]->front();
        const item &real_item = it.is_container() ?  it.contents.front() : it;
        if( filter( real_item ) ) {
            index = i;
            break;
        }
    }
    if( index < 0 ) {
        return false;
    }
    int old_moves = moves;
    return consume( index ) && old_moves != moves;
}

bool npc::recharge_cbm()
{
    // non-allied NPCs don't consume resources to recharge
    if( !is_player_ally() ) {
        charge_power( max_power_level );
        return true;
    }

    use_bionic_by_id( bio_torsionratchet );
    use_bionic_by_id( bio_metabolics );

    if( use_bionic_by_id( bio_furnace ) ) {
        const std::function<bool( const item & )> furnace_filter = []( const item & it ) {
            return it.typeId() == itype_id( "withered" ) || it.typeId() == itype_id( "file" ) ||
                   it.has_flag( "FIREWOOD" );
        };
        if( consume_cbm_items( furnace_filter ) ) {
            return true;
        } else {
            complain_about( "need_junk", 3_hours, "<need_junk>", false );
        }
    }

    if( use_bionic_by_id( bio_batteries ) ) {
        const std::function<bool( const item & )> battery_filter = []( const item & it ) {
            return it.typeId() == itype_id( "battery" );
        };
        if( consume_cbm_items( battery_filter ) ) {
            return true;
        } else {
            complain_about( "need_batteries", 3_hours, "<need_batteries>", false );
        }
    }

    if( use_bionic_by_id( bio_ethanol ) ) {
        const std::function<bool( const item & )> ethanol_filter = []( const item & it ) {
            return it.type->can_use( "WEAK_ALCOHOL" ) || it.type->can_use( "ALCOHOL" ) ||
                   it.type->can_use( "STRONG_ALOCHOL" );
        };
        if( consume_cbm_items( ethanol_filter ) ) {
            return true;
        } else {
            complain_about( "need_booze", 3_hours, "<need_booze>", false );
        }
    }

    if( use_bionic_by_id( bio_reactor ) || use_bionic_by_id( bio_advreactor ) ) {
        const std::function<bool( const item & )> reactor_filter = []( const item & it ) {
            return it.is_ammo() && ( it.ammo_type() == plutonium ||
                                     it.ammo_type() == reactor_slurry );
        };
        if( consume_cbm_items( reactor_filter ) ) {
            return true;
        } else {
            complain_about( "need_radioactives", 3_hours, "<need_radioactives>", false );
        }
    }

    return false;
}

npc_action npc::address_needs( float danger )
{
    ai_cache.can_heal = has_healing_options();

    if( need_heal( *this ) ) {
        if( !use_bionic_by_id( bio_nanobots ) ) {
            return npc_heal;
        }
    } else {
        deactivate_bionic_by_id( bio_nanobots );
    }

    if( get_perceived_pain() >= 15 ) {
        if( !activate_bionic_by_id( bio_painkiller ) && has_painkiller() && !took_painkiller() ) {
            return npc_use_painkiller;
        }
    } else {
        deactivate_bionic_by_id( bio_nanobots );
    }

    // If there are no healing items, there's nothing to do
    if( ai_cache.can_heal.bandage || ai_cache.can_heal.bleed || ai_cache.can_heal.bite ||
        ai_cache.can_heal.infect ) {
        if( is_player_ally() && need_heal( g->u ) ) {
            ai_cache.ally = g->shared_from( g->u );
            return npc_heal_player;
        }

        const std::vector<npc *> hurt_allies = g->get_npcs_if( [&]( const npc & guy ) {
            return guy.getID() != getID() && guy.is_ally( *this ) && posz() == guy.posz() &&
                   need_heal( guy ) && rl_dist( pos(), guy.pos() ) <= SEEX * 2 && sees( guy.pos() );
        } );

        for( npc *guy : hurt_allies ) {
            ai_cache.ally = g->shared_from( *guy );
            return npc_heal_player;
        }
    }

    if( can_reload_current() ) {
        return npc_reload;
    }

    item &reloadable = find_reloadable();
    if( !reloadable.is_null() ) {
        do_reload( reloadable );
        return npc_noop;
    }

    // Extreme thirst or hunger, bypass safety check.
    if( get_thirst() > 80 ||
        get_stored_kcal() + stomach.get_calories() < get_healthy_kcal() * 0.75 ) {
        if( consume_food() ) {
            return npc_noop;
        }
    }
    //Does the hallucination needs to disappear ?
    if( g->u.sees( *this ) && is_hallucination() ) {
        if( !g->u.has_effect( effect_hallu ) ) {
            die( nullptr );
        }
    }

    if( danger > NPC_DANGER_VERY_LOW ) {
        return npc_undecided;
    }

    if( get_thirst() > 40 ||
        get_stored_kcal() + stomach.get_calories() < get_healthy_kcal() * 0.95 ) {
        if( consume_food() ) {
            return npc_noop;
        }
    }

    if( wants_to_recharge_cbm() && recharge_cbm() ) {
        return npc_noop;
    }

    if( find_corpse_to_pulp() ) {
        if( !do_pulp() ) {
            move_to_next();
        }
        return npc_noop;
    }

    if( adjust_worn() ) {
        return npc_noop;
    }

    const auto could_sleep = [&]() {
        if( danger <= 0.01 ) {
            if( get_fatigue() >= TIRED ) {
                return true;
            } else if( is_walking_with() && g->u.in_sleep_state() &&
                       get_fatigue() > ( TIRED / 2 ) ) {
                return true;
            }
        }
        return false;
    };
    // TODO: More risky attempts at sleep when exhausted
    if( could_sleep() ) {
        if( !is_player_ally() ) {
            set_fatigue( 0 ); // TODO: Make tired NPCs handle sleep offscreen
            return npc_undecided;
        }

        if( rules.has_flag( ally_rule::allow_sleep ) || get_fatigue() > MASSIVE_FATIGUE ) {
            return npc_sleep;
        } else if( g->u.in_sleep_state() ) {
            // TODO: "Guard me while I sleep" command
            return npc_sleep;
        }
    }
    // TODO: Mutation & trait related needs
    // e.g. finding glasses; getting out of sunlight if we're an albino; etc.

    return npc_undecided;
}

npc_action npc::address_player()
{
    if( ( attitude == NPCATT_TALK || attitude == NPCATT_RECOVER_GOODS ) && sees( g->u ) ) {
        if( g->u.in_sleep_state() ) {
            // Leave sleeping characters alone.
            return npc_undecided;
        }
        if( rl_dist( pos(), g->u.pos() ) <= 6 ) {
            return npc_talk_to_player;    // Close enough to talk to you
        } else {
            if( one_in( 10 ) ) {
                say( "<lets_talk>" );
            }
            return npc_follow_player;
        }
    }

    if( attitude == NPCATT_MUG && sees( g->u ) ) {
        if( one_in( 3 ) ) {
            say( _( "Don't move a <swear> muscle..." ) );
        }
        return npc_mug_player;
    }

    if( attitude == NPCATT_WAIT_FOR_LEAVE ) {
        patience--;
        if( patience <= 0 ) {
            patience = 0;
            set_attitude( NPCATT_KILL );
            return npc_noop;
        }
        return npc_undecided;
    }

    if( attitude == NPCATT_FLEE_TEMP ) {
        return npc_flee;
    }

    if( attitude == NPCATT_LEAD ) {
        if( rl_dist( pos(), g->u.pos() ) >= 12 || !sees( g->u ) ) {
            int intense = get_effect_int( effect_catch_up );
            if( intense < 10 ) {
                say( "<keep_up>" );
                add_effect( effect_catch_up, 5_turns );
                return npc_pause;
            } else {
                say( "<im_leaving_you>" );
                set_attitude( NPCATT_NULL );
                return npc_pause;
            }
        } else if( has_omt_destination() ) {
            return npc_goto_destination;
        } else { // At goal. Now, waiting on nearby player
            return npc_pause;
        }
    }
    return npc_undecided;
}

npc_action npc::long_term_goal_action()
{
    add_msg( m_debug, "long_term_goal_action()" );

    if( mission == NPC_MISSION_SHOPKEEP || mission == NPC_MISSION_SHELTER ) {
        return npc_pause;    // Shopkeepers just stay put.
    }

    if( !has_omt_destination() ) {
        set_omt_destination();
    }

    if( has_omt_destination() ) {
        if( mission != NPC_MISSION_TRAVELLING ) {
            set_mission( NPC_MISSION_TRAVELLING );
            set_attitude( attitude );
        }
        return npc_goto_destination;
    }

    return npc_undecided;
}

double npc::confidence_mult() const
{
    if( !is_player_ally() ) {
        return 1.0f;
    }

    switch( rules.aim ) {
        case AIM_WHEN_CONVENIENT:
            return emergency() ? 1.5f : 1.0f;
        case AIM_SPRAY:
            return 2.0f;
        case AIM_PRECISE:
            return emergency() ? 1.0f : 0.75f;
        case AIM_STRICTLY_PRECISE:
            return 0.5f;
    }

    return 1.0f;
}

int npc::confident_shoot_range( const item &it, int recoil ) const
{
    int res = 0;
    for( const auto &m : it.gun_all_modes() ) {
        res = std::max( res, confident_gun_mode_range( m.second, recoil ) );
    }
    return res;
}

int npc::confident_gun_mode_range( const gun_mode &gun, int at_recoil ) const
{
    if( !gun || gun.melee() ) {
        return 0;
    }

    // Same calculation as in @ref item::info
    // TODO: Extract into common method
    double max_dispersion = get_weapon_dispersion( *( gun.target ) ).max() + at_recoil;
    double even_chance_range = range_with_even_chance_of_good_hit( max_dispersion );
    double confident_range = even_chance_range * confidence_mult();

    add_msg( m_debug, "confident_gun (%s<=%.2f) at %.1f", gun.name(), confident_range,
             max_dispersion );
    return std::max<int>( confident_range, 1 );
}

int npc::confident_throw_range( const item &thrown, Creature *target ) const
{
    double average_dispersion = throwing_dispersion( thrown, target ) / 2.0;
    double even_chance_range = ( target == nullptr ? 0.5 : target->ranged_target_size() ) /
                               average_dispersion;
    double confident_range = even_chance_range * confidence_mult();
    add_msg( m_debug, "confident_throw_range == %d", static_cast<int>( confident_range ) );
    return static_cast<int>( confident_range );
}

// Index defaults to -1, i.e., wielded weapon
bool npc::wont_hit_friend( const tripoint &tar, const item &it, bool throwing ) const
{
    // TODO: Get actual dispersion instead of extracting it (badly) from confident range
    int confident = throwing ?
                    confident_throw_range( it, nullptr ) :
                    confident_shoot_range( it, recoil_total() );
    // if there is no confidence at using weapon, it's not used at range
    // zero confidence leads to divide by zero otherwise
    if( confident < 1 ) {
        return true;
    }

    if( rl_dist( pos(), tar ) == 1 ) {
        return true;    // If we're *really* sure that our aim is dead-on
    }

    int target_angle = coord_to_angle( pos(), tar );

    // TODO: Base on dispersion
    int safe_angle = 30;

    for( const auto &fr : ai_cache.friends ) {
        const std::shared_ptr<Creature> ally_p = fr.lock();
        if( !ally_p ) {
            continue;
        }
        const Creature &ally = *ally_p;

        // TODO: Extract common functions with turret target selection
        int safe_angle_ally = safe_angle;
        int ally_dist = rl_dist( pos(), ally.pos() );
        if( ally_dist < 3 ) {
            safe_angle_ally += ( 3 - ally_dist ) * 30;
        }

        int ally_angle = coord_to_angle( pos(), ally.pos() );
        int angle_diff = abs( ally_angle - target_angle );
        angle_diff = std::min( 360 - angle_diff, angle_diff );
        if( angle_diff < safe_angle_ally ) {
            // TODO: Disable NPC whining is it's other NPC who prevents aiming
            return false;
        }
    }

    return true;
}

bool npc::enough_time_to_reload( const item &gun ) const
{
    int rltime = item_reload_cost( gun, item( gun.ammo_default() ),
                                   gun.ammo_capacity() );
    const float turns_til_reloaded = static_cast<float>( rltime ) / get_speed();

    const Creature *target = current_target();
    if( target == nullptr ) {
        // No target, plenty of time to reload
        return true;
    }

    const auto distance = rl_dist( pos(), target->pos() );
    const float target_speed = target->speed_rating();
    const float turns_til_reached = distance / target_speed;
    if( target->is_player() || target->is_npc() ) {
        auto &c = dynamic_cast<const Character &>( *target );
        // TODO: Allow reloading if the player has a low accuracy gun
        if( sees( c ) && c.weapon.is_gun() && rltime > 200 &&
            c.weapon.gun_range( true ) > distance + turns_til_reloaded / target_speed ) {
            // Don't take longer than 2 turns if player has a gun
            return false;
        }
    }

    // TODO: Handle monsters with ranged attacks and players with CBMs
    return turns_til_reloaded < turns_til_reached;
}

void npc::aim()
{
    double aim_amount = aim_per_move( weapon, recoil );
    while( aim_amount > 0 && recoil > 0 && moves > 0 ) {
        moves--;
        recoil -= aim_amount;
        recoil = std::max( 0.0, recoil );
        aim_amount = aim_per_move( weapon, recoil );
    }
}

bool npc::update_path( const tripoint &p, const bool no_bashing, bool force )
{
    if( p == pos() ) {
        path.clear();
        return true;
    }

    while( !path.empty() && path[0] == pos() ) {
        path.erase( path.begin() );
    }

    if( !path.empty() ) {
        const tripoint &last = path[path.size() - 1];
        if( last == p && ( path[0].z != posz() || rl_dist( path[0], pos() ) <= 1 ) ) {
            // Our path already leads to that point, no need to recalculate
            return true;
        }
    }

    auto new_path = g->m.route( pos(), p, get_pathfinding_settings( no_bashing ), get_path_avoid() );
    if( new_path.empty() ) {
        if( !ai_cache.sound_alerts.empty() ) {
            ai_cache.sound_alerts.erase( ai_cache.sound_alerts.begin() );
            add_msg( m_debug, "failed to path to sound alert %d,%d,%d->%d,%d,%d",
                     posx(), posy(), posz(), p.x, p.y, p.z );
        }
        add_msg( m_debug, "Failed to path %d,%d,%d->%d,%d,%d",
                 posx(), posy(), posz(), p.x, p.y, p.z );
    }

    while( !new_path.empty() && new_path[0] == pos() ) {
        new_path.erase( new_path.begin() );
    }

    if( !new_path.empty() || force ) {
        path = std::move( new_path );
        return true;
    }

    return false;
}

bool npc::can_open_door( const tripoint &p, const bool inside ) const
{
    return !rules.has_flag( ally_rule::avoid_doors ) && g->m.open_door( p, inside, true );
}

bool npc::can_move_to( const tripoint &p, bool no_bashing ) const
{
    // Allow moving into any bashable spots, but penalize them during pathing
    // Doors are not passable for hallucinations
    return( rl_dist( pos(), p ) <= 1 &&
            ( g->m.passable( p ) || ( can_open_door( p, !g->m.is_outside( pos() ) ) && !is_hallucination() ) ||
              ( !no_bashing && g->m.bash_rating( smash_ability(), p ) > 0 ) )
          );
}

void npc::move_to( const tripoint &pt, bool no_bashing, std::set<tripoint> *nomove )
{
    tripoint p = pt;
    if( sees_dangerous_field( p )
        || ( nomove != nullptr && nomove->find( p ) != nomove->end() ) ) {
        // Move to a neighbor field instead, if possible.
        // Maybe this code already exists somewhere?
        auto other_points = g->m.get_dir_circle( pos(), p );
        for( const tripoint &ot : other_points ) {
            if( could_move_onto( ot )
                && ( nomove == nullptr || nomove->find( ot ) == nomove->end() ) ) {

                p = ot;
                break;
            }
        }
    }

    recoil = MAX_RECOIL;

    if( has_effect( effect_stunned ) ) {
        p.x = rng( posx() - 1, posx() + 1 );
        p.y = rng( posy() - 1, posy() + 1 );
        p.z = posz();
    }

    // nomove is used to resolve recursive invocation, so reset destination no
    // matter it was changed by stunned effect or not.
    if( nomove != nullptr && nomove->find( p ) != nomove->end() ) {
        p = pos();
    }

    // "Long steps" are allowed when crossing z-levels
    // Stairs teleport the player too
    if( rl_dist( pos(), p ) > 1 && p.z == posz() ) {
        // On the same level? Not so much. Something weird happened
        path.clear();
        move_pause();
    }
    bool attacking = false;
    if( g->critter_at<monster>( p ) ) {
        attacking = true;
    }
    if( !move_effects( attacking ) ) {
        mod_moves( -100 );
        return;
    }

    Creature *critter = g->critter_at( p );
    if( critter != nullptr ) {
        if( critter == this ) { // We're just pausing!
            move_pause();
            return;
        }
        const auto att = attitude_to( *critter );
        if( att == A_HOSTILE ) {
            if( !no_bashing ) {
                warn_about( "cant_flee", 5_turns + rng( 0, 5 ) * 1_turns );
                melee_attack( *critter, true );
            } else {
                move_pause();
            }

            return;
        }

        if( critter == &g->u ) {
            say( "<let_me_pass>" );
        }

        // Let NPCs push each other when non-hostile
        // TODO: Have them attack each other when hostile
        npc *np = dynamic_cast<npc *>( critter );
        if( np != nullptr && !np->in_sleep_state() ) {
            std::unique_ptr<std::set<tripoint>> newnomove;
            std::set<tripoint> *realnomove;
            if( nomove != nullptr ) {
                realnomove = nomove;
            } else {
                // create the no-move list
                newnomove = std::make_unique<std::set<tripoint>>();
                realnomove = newnomove.get();
            }
            // other npcs should not try to move into this npc anymore,
            // so infinite loop can be avoided.
            realnomove->insert( pos() );
            say( "<let_me_pass>" );
            np->move_away_from( pos(), true, realnomove );
        }

        if( critter->pos() == p ) {
            move_pause();
            return;
        }
    }

    // Boarding moving vehicles is fine, unboarding isn't
    bool moved = false;
    if( const optional_vpart_position vp = g->m.veh_at( pos() ) ) {
        const optional_vpart_position ovp = g->m.veh_at( p );
        if( vp->vehicle().is_moving() &&
            ( veh_pointer_or_null( ovp ) != veh_pointer_or_null( vp ) ||
              !ovp.part_with_feature( VPFLAG_BOARDABLE, true ) ) ) {
            move_pause();
            return;
        }
    }

    if( p.z != posz() ) {
        // Z-level move
        // For now just teleport to the destination
        // TODO: Make it properly find the tile to move to
        moves -= 100;
        moved = true;
    } else if( g->m.passable( p ) ) {
        bool diag = trigdist && posx() != p.x && posy() != p.y;
        moves -= run_cost( g->m.combined_movecost( pos(), p ), diag );
        moved = true;
    } else if( g->m.open_door( p, !g->m.is_outside( pos() ), true ) ) {
        if( !is_hallucination() ) { // hallucinations don't open doors
            g->m.open_door( p, !g->m.is_outside( pos() ) );
            moves -= 100;
        } else { // hallucinations teleport through doors
            moves -= 100;
            moved = true;
        }
    } else if( get_dex() > 1 && g->m.has_flag_ter_or_furn( "CLIMBABLE", p ) ) {
        ///\EFFECT_DEX_NPC increases chance to climb CLIMBABLE furniture or terrain
        int climb = get_dex();
        if( one_in( climb ) ) {
            add_msg_if_npc( m_neutral, _( "%1$s tries to climb the %2$s but slips." ), name,
                            g->m.tername( p ) );
            moves -= 400;
        } else {
            add_msg_if_npc( m_neutral, _( "%1$s climbs over the %2$s." ), name, g->m.tername( p ) );
            moves -= ( 500 - ( rng( 0, climb ) * 20 ) );
            moved = true;
        }
    } else if( !no_bashing && smash_ability() > 0 && g->m.is_bashable( p ) &&
               g->m.bash_rating( smash_ability(), p ) > 0 ) {
        moves -= !is_armed() ? 80 : weapon.attack_time() * 0.8;
        g->m.bash( p, smash_ability() );
    } else {
        if( attitude == NPCATT_MUG ||
            attitude == NPCATT_KILL ||
            attitude == NPCATT_WAIT_FOR_LEAVE ) {
            set_attitude( NPCATT_FLEE_TEMP );
        }

        moves = 0;
    }

    if( moved ) {
        const tripoint old_pos = pos();
        setpos( p );

        if( g->m.has_flag( "UNSTABLE", pos() ) ) {
            add_effect( effect_bouldering, 1_turns, num_bp, true );
        } else if( has_effect( effect_bouldering ) ) {
            remove_effect( effect_bouldering );
        }

        if( g->m.has_flag_ter_or_furn( TFLAG_NO_SIGHT, pos() ) ) {
            add_effect( effect_no_sight, 1_turns, num_bp, true );
        } else if( has_effect( effect_no_sight ) ) {
            remove_effect( effect_no_sight );
        }

        if( in_vehicle ) {
            g->m.unboard_vehicle( old_pos );
        }

        // Close doors behind self (if you can)
        if( ( rules.has_flag( ally_rule::close_doors ) && is_player_ally() ) && !is_hallucination() ) {
            doors::close_door( g->m, *this, old_pos );
        }

        if( g->m.veh_at( p ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
            g->m.board_vehicle( p, this );
        }

        g->m.creature_on_trap( *this );
        g->m.creature_in_field( *this );
    }
}

void npc::move_to_next()
{
    while( !path.empty() && pos() == path[0] ) {
        path.erase( path.begin() );
    }

    if( path.empty() ) {
        add_msg( m_debug, "npc::move_to_next() called with an empty path or path "
                 "containing only current position" );
        move_pause();
        return;
    }

    move_to( path[0] );
    if( !path.empty() && pos() == path[0] ) { // Move was successful
        path.erase( path.begin() );
    }
}

void npc::avoid_friendly_fire()
{
    // TODO: To parameter
    const tripoint &tar = current_target()->pos();
    // Calculate center of weight of friends and move away from that
    tripoint center;
    for( const auto &fr : ai_cache.friends ) {
        if( std::shared_ptr<Creature> fr_p = fr.lock() ) {
            center += fr_p->pos();
        }
    }

    float friend_count = ai_cache.friends.size();
    center.x = round( center.x / friend_count );
    center.y = round( center.y / friend_count );
    center.z = round( center.z / friend_count );

    auto candidates = closest_tripoints_first( 1, pos() );
    candidates.erase( candidates.begin() );
    std::sort( candidates.begin(), candidates.end(),
    [&tar, &center]( const tripoint & l, const tripoint & r ) {
        return ( rl_dist( l, tar ) - rl_dist( l, center ) ) <
               ( rl_dist( r, tar ) - rl_dist( r, center ) );
    } );

    for( const auto &pt : candidates ) {
        if( can_move_to( pt ) ) {
            move_to( pt );
            return;
        }
    }

    /* If we're still in the function at this point, maneuvering can't help us. So,
     * might as well address some needs.
     * We pass a <danger> value of NPC_DANGER_VERY_LOW + 1 so that we won't start
     * eating food (or, god help us, sleeping).
     */
    npc_action action = address_needs( NPC_DANGER_VERY_LOW + 1 );
    if( action == npc_undecided ) {
        move_pause();
    }
    execute_action( action );
}

void npc::escape_explosion()
{
    if( ai_cache.dangerous_explosives.empty() ) {
        return;
    }

    warn_about( "explosion", 1_minutes );

    move_away_from( ai_cache.dangerous_explosives, true );
}

void npc::move_away_from( const tripoint &pt, bool no_bash_atk, std::set<tripoint> *nomove )
{
    tripoint best_pos = pos();
    int best = -1;
    int chance = 2;
    for( const tripoint &p : g->m.points_in_radius( pos(), 1 ) ) {
        if( nomove != nullptr && nomove->find( p ) != nomove->end() ) {
            continue;
        }

        if( p == pos() ) {
            continue;
        }

        if( p == g->u.pos() ) {
            continue;
        }

        const int cost = g->m.combined_movecost( pos(), p );
        if( cost <= 0 ) {
            continue;
        }

        const int dst = abs( p.x - pt.x ) + abs( p.y - pt.y ) + abs( p.z - pt.z );
        const int val = dst * 1000 / cost;
        if( val > best && can_move_to( p, no_bash_atk ) ) {
            best_pos = p;
            best = val;
            chance = 2;
        } else if( ( val == best && one_in( chance ) ) && can_move_to( p, no_bash_atk ) ) {
            best_pos = p;
            best = val;
            chance++;
        }
    }

    move_to( best_pos, no_bash_atk, nomove );
}

void npc::move_pause()

{
    // make sure we're using the best weapon
    if( calendar::once_every( 1_hours ) ) {
        deactivate_bionic_by_id( bio_soporific );
        for( const bionic_id &bio_id : health_cbms ) {
            activate_bionic_by_id( bio_id );
        }
        if( wield_better_weapon() ) {
            return;
        }
    }
    // NPCs currently always aim when using a gun, even with no target
    // This simulates them aiming at stuff just at the edge of their range
    if( !weapon.is_gun() ) {
        pause();
        return;
    }

    // Stop, drop, and roll
    if( has_effect( effect_onfire ) ) {
        pause();
    } else {
        aim();
        moves = std::min( moves, 0 );
    }
}

static cata::optional<tripoint> nearest_passable( const tripoint &p, const tripoint &closest_to )
{
    if( g->m.passable( p ) ) {
        return p;
    }

    // We need to path to adjacent tile, not the exact one
    // Let's pick the closest one to us that is passable
    auto candidates = closest_tripoints_first( 1, p );
    std::sort( candidates.begin(), candidates.end(), [ closest_to ]( const tripoint & l,
    const tripoint & r ) {
        return rl_dist( closest_to, l ) < rl_dist( closest_to, r );
    } );
    auto iter = std::find_if( candidates.begin(), candidates.end(), []( const tripoint & pt ) {
        return g->m.passable( pt );
    } );
    if( iter != candidates.end() ) {
        return *iter;
    }

    return cata::nullopt;
}

void npc::move_away_from( const std::vector<sphere> &spheres, bool no_bashing )
{
    if( spheres.empty() ) {
        return;
    }

    tripoint minp( pos() );
    tripoint maxp( pos() );

    for( const auto &elem : spheres ) {
        minp.x = std::min( minp.x, elem.center.x - elem.radius );
        minp.y = std::min( minp.y, elem.center.y - elem.radius );
        maxp.x = std::max( maxp.x, elem.center.x + elem.radius );
        maxp.y = std::max( maxp.y, elem.center.y + elem.radius );
    }

    const tripoint_range range( minp, maxp );

    std::vector<tripoint> escape_points;

    std::copy_if( range.begin(), range.end(), std::back_inserter( escape_points ),
    [&]( const tripoint & elem ) {
        return g->m.passable( elem );
    } );

    algo::sort_by_rating( escape_points.begin(), escape_points.end(), [&]( const tripoint & elem ) {
        const int danger = std::accumulate( spheres.begin(), spheres.end(), 0,
        [&]( const int sum, const sphere & s ) {
            return sum + std::max( s.radius - rl_dist( elem, s.center ), 0 );
        } );

        const int distance = rl_dist( pos(), elem );
        const int move_cost = g->m.move_cost( elem );

        return std::make_tuple( danger, distance, move_cost );
    } );

    for( const auto &elem : escape_points ) {
        update_path( elem, no_bashing );

        if( elem == pos() || !path.empty() ) {
            break;
        }
    }

    if( !path.empty() ) {
        move_to_next();
    } else {
        move_pause();
    }
}

void npc::see_item_say_smth( const itype_id &object, const std::string &smth )
{
    for( const tripoint &p : closest_tripoints_first( 6, pos() ) ) {
        if( g->m.sees_some_items( p, *this ) && sees( p ) ) {
            for( const item &it : g->m.i_at( p ) ) {
                if( one_in( 100 ) && ( it.typeId() == object ) ) {
                    say( smth );
                }
            }
        }
    }
}

void npc::find_item()
{
    if( is_hallucination() ) {
        see_item_say_smth( "thorazine", "<no_to_thorazine>" );
        see_item_say_smth( "lsd", "<yes_to_lsd>" );
        return;
    }

    if( is_player_ally() && !rules.has_flag( ally_rule::allow_pick_up ) ) {

        // Grabbing stuff not allowed by our "owner"
        return;
    }

    fetching_item = false;
    int best_value = minimum_item_value();
    // Not perfect, but has to mirror pickup code
    units::volume volume_allowed = volume_capacity() - volume_carried();
    units::mass   weight_allowed = weight_capacity() - weight_carried();
    // For some reason range limiting by vision doesn't work properly
    const int range = 6;
    //int range = sight_range( g->light_level( posz() ) );
    //range = std::max( 1, std::min( 12, range ) );

    static const zone_type_id no_pickup( "NO_NPC_PICKUP" );

    const item *wanted = nullptr;

    const bool whitelisting = has_item_whitelist();

    if( volume_allowed <= 0_ml || weight_allowed <= 0_gram ) {
        return;
    }

    const auto consider_item =
        [&wanted, &best_value, whitelisting, volume_allowed, weight_allowed, this]
    ( const item & it, const tripoint & p ) {
        if( it.made_of_from_type( LIQUID ) ) {
            // Don't even consider liquids.
            return;
        }
        std::vector<npc *> followers;
        for( auto &elem : g->get_follower_list() ) {
            std::shared_ptr<npc> npc_to_get = overmap_buffer.find_npc( elem );
            if( !npc_to_get ) {
                continue;
            }
            npc *npc_to_add = npc_to_get.get();
            followers.push_back( npc_to_add );
        }
        for( auto &elem : followers ) {
            if( it.has_owner() && it.get_owner() != my_fac && ( elem->sees( this->pos() ) ||
                    elem->sees( wanted_item_pos ) ) ) {
                return;
            }
        }
        if( it.has_owner() && it.get_owner() != my_fac && ( g->u.sees( this->pos() ) ||
                g->u.sees( wanted_item_pos ) ) ) {
            return;
        }
        if( whitelisting && !item_whitelisted( it ) ) {
            return;
        }

        // When using a whitelist, skip the value check
        // TODO: Whitelist hierarchy?
        int itval = whitelisting ? 1000 : value( it );

        if( itval > best_value &&
            ( it.volume() <= volume_allowed && it.weight() <= weight_allowed ) ) {
            wanted_item_pos = p;
            wanted = &( it );
            best_value = itval;
        }
    };

    // Harvest item doesn't exist, so we'll be checking by its name
    std::string wanted_name;
    const auto consider_terrain =
    [ this, whitelisting, volume_allowed, &wanted, &wanted_name ]( const tripoint & p ) {
        // We only want to pick plants when there are no items to pick
        if( !whitelisting || wanted != nullptr || !wanted_name.empty() || volume_allowed < 250_ml ) {
            return;
        }

        const auto harvest = g->m.get_harvest_names( p );
        for( const auto &entry : harvest ) {
            if( item_name_whitelisted( entry ) ) {
                wanted_name = entry;
                wanted_item_pos = p;
                break;
            }
        }
    };

    for( const tripoint &p : closest_tripoints_first( range, pos() ) ) {
        // TODO: Make this sight check not overdraw nearby tiles
        // TODO: Optimize that zone check
        if( is_player_ally() && g->check_zone( no_pickup, p ) ) {
            continue;
        }

        if( g->m.sees_some_items( p, *this ) && sees( p ) ) {
            for( const item &it : g->m.i_at( p ) ) {
                consider_item( it, p );
            }
        }

        // Allow terrain check without sight, because it would cost more CPU than it is worth
        consider_terrain( p );

        const optional_vpart_position vp = g->m.veh_at( p );
        if( !vp || vp->vehicle().is_moving() || !sees( p ) ) {
            continue;
        }
        const cata::optional<vpart_reference> cargo = vp.part_with_feature( VPFLAG_CARGO, true );
        static const std::string locked_string( "LOCKED" );
        // TODO: Let player know what parts are safe from NPC thieves
        if( !cargo || cargo->has_feature( locked_string ) ) {
            continue;
        }

        static const std::string cargo_locking_string( "CARGO_LOCKING" );
        if( vp.part_with_feature( cargo_locking_string, true ) ) {
            continue;
        }

        for( const item &it : cargo->vehicle().get_items( cargo->part_index() ) ) {
            consider_item( it, p );
        }
    }

    if( wanted != nullptr ) {
        wanted_name = wanted->tname();
    }

    if( wanted_name.empty() ) {
        return;
    }

    fetching_item = true;

    // TODO: Move that check above, make it multi-target pathing and use it
    // to limit tiles available for choice of items
    const int dist_to_item = rl_dist( wanted_item_pos, pos() );
    if( const cata::optional<tripoint> dest = nearest_passable( wanted_item_pos, pos() ) ) {
        update_path( *dest );
    }

    if( path.empty() && dist_to_item > 1 ) {
        // Item not reachable, let's just totally give up for now
        fetching_item = false;
    }

    if( fetching_item && rl_dist( wanted_item_pos, pos() ) > 1 && is_walking_with() ) {
        say( _( "Hold on, I want to pick up that %s." ), wanted_name );
    }
}

void npc::pick_up_item()
{
    if( is_hallucination() ) {
        return;
    }

    if( !rules.has_flag( ally_rule::allow_pick_up ) && is_player_ally() ) {
        add_msg( m_debug, "%s::pick_up_item(); Cancelling on player's request", name );
        fetching_item = false;
        moves -= 1;
        return;
    }

    const cata::optional<vpart_reference> vp = g->m.veh_at( wanted_item_pos ).part_with_feature(
                VPFLAG_CARGO, false );
    const bool has_cargo = vp && !vp->has_feature( "LOCKED" );

    if( ( !g->m.has_items( wanted_item_pos ) && !has_cargo &&
          !g->m.is_harvestable( wanted_item_pos ) && sees( wanted_item_pos ) ) ||
        ( is_player_ally() && g->check_zone( zone_type_id( "NO_NPC_PICKUP" ), wanted_item_pos ) ) ) {
        // Items we wanted no longer exist and we can see it
        // Or player who is leading us doesn't want us to pick it up
        fetching_item = false;
        move_pause();
        add_msg( m_debug, "Canceling pickup - no items or new zone" );
        return;
    }

    add_msg( m_debug, "%s::pick_up_item(); [%d, %d, %d] => [%d, %d, %d]", name,
             posx(), posy(), posz(), wanted_item_pos.x, wanted_item_pos.y, wanted_item_pos.z );
    if( const cata::optional<tripoint> dest = nearest_passable( wanted_item_pos, pos() ) ) {
        update_path( *dest );
    }

    const int dist_to_pickup = rl_dist( pos(), wanted_item_pos );
    if( dist_to_pickup > 1 && !path.empty() ) {
        add_msg( m_debug, "Moving; [%d, %d, %d] => [%d, %d, %d]",
                 posx(), posy(), posz(), path[0].x, path[0].y, path[0].z );

        move_to_next();
        return;
    } else if( dist_to_pickup > 1 && path.empty() ) {
        add_msg( m_debug, "Can't find path" );
        // This can happen, always do something
        fetching_item = false;
        move_pause();
        return;
    }

    // We're adjacent to the item; grab it!

    auto picked_up = pick_up_item_map( wanted_item_pos );
    if( picked_up.empty() && has_cargo ) {
        picked_up = pick_up_item_vehicle( vp->vehicle(), vp->part_index() );
    }

    if( picked_up.empty() ) {
        // Last chance: plant harvest
        if( g->m.is_harvestable( wanted_item_pos ) ) {
            g->m.examine( *this, wanted_item_pos );
            // Note: we didn't actually pick up anything, just spawned items
            // but we want the item picker to find new items
            fetching_item = false;
            return;
        }
    }
    // Describe the pickup to the player
    bool u_see = g->u.sees( *this ) || g->u.sees( wanted_item_pos );
    if( u_see ) {
        if( picked_up.size() == 1 ) {
            add_msg( _( "%1$s picks up a %2$s." ), name, picked_up.front().tname() );
        } else if( picked_up.size() == 2 ) {
            add_msg( _( "%1$s picks up a %2$s and a %3$s." ), name, picked_up.front().tname(),
                     picked_up.back().tname() );
        } else if( picked_up.size() > 2 ) {
            add_msg( _( "%s picks up several items." ), name );
        } else {
            add_msg( _( "%s looks around nervously, as if searching for something." ), name );
        }
    }

    for( auto &it : picked_up ) {
        int itval = value( it );
        if( itval < worst_item_value ) {
            worst_item_value = itval;
        }
        i_add( it );
    }

    moves -= 100;
    fetching_item = false;
    has_new_items = true;
}

template <typename T>
std::list<item> npc_pickup_from_stack( npc &who, T &items )
{
    const bool whitelisting = who.has_item_whitelist();
    auto volume_allowed = who.volume_capacity() - who.volume_carried();
    auto weight_allowed = who.weight_capacity() - who.weight_carried();
    auto min_value = whitelisting ? 0 : who.minimum_item_value();
    std::list<item> picked_up;

    for( auto iter = items.begin(); iter != items.end(); ) {
        const item &it = *iter;
        if( it.made_of_from_type( LIQUID ) ) {
            iter++;
            continue;
        }

        if( whitelisting && !who.item_whitelisted( it ) ) {
            iter++;
            continue;
        }

        auto volume = it.volume();
        if( volume > volume_allowed ) {
            iter++;
            continue;
        }

        auto weight = it.weight();
        if( weight > weight_allowed ) {
            iter++;
            continue;
        }

        int itval = whitelisting ? 1000 : who.value( it );
        if( itval < min_value ) {
            iter++;
            continue;
        }

        volume_allowed -= volume;
        weight_allowed -= weight;
        picked_up.push_back( it );
        iter = items.erase( iter );
    }

    return picked_up;
}

std::list<item> npc::pick_up_item_map( const tripoint &where )
{
    auto stack = g->m.i_at( where );
    return npc_pickup_from_stack( *this, stack );
}

std::list<item> npc::pick_up_item_vehicle( vehicle &veh, int part_index )
{
    auto stack = veh.get_items( part_index );
    return npc_pickup_from_stack( *this, stack );
}

void npc::drop_items( int weight, int volume )
{
    add_msg( m_debug, "%s is dropping items-%d,%d (%d items, wgt %d/%d, vol %d/%d)",
             name, weight, volume, inv.size(), to_gram( weight_carried() ),
             to_gram( weight_capacity() ), volume_carried() / units::legacy_volume_factor,
             volume_capacity() / units::legacy_volume_factor );

    int weight_dropped = 0;
    int volume_dropped = 0;
    std::vector<ratio_index> rWgt, rVol; // Weight/Volume to value ratios

    // First fill our ratio vectors, so we know which things to drop first
    invslice slice = inv.slice();
    for( size_t i = 0; i < slice.size(); i++ ) {
        item &it = slice[i]->front();
        double wgt_ratio = 0.0;
        double vol_ratio = 0.0;
        if( value( it ) == 0 ) {
            wgt_ratio = 99999;
            vol_ratio = 99999;
        } else {
            wgt_ratio = units::to_gram<double>( it.weight() ) / value( it );
            vol_ratio = it.volume() * 1.0 / units::legacy_volume_factor / value( it );
        }
        bool added_wgt = false;
        bool added_vol = false;
        for( size_t j = 0; j < rWgt.size() && !added_wgt; j++ ) {
            if( wgt_ratio > rWgt[j].ratio ) {
                added_wgt = true;
                rWgt.insert( rWgt.begin() + j, ratio_index( wgt_ratio, i ) );
            }
        }
        if( !added_wgt ) {
            rWgt.push_back( ratio_index( wgt_ratio, i ) );
        }
        for( size_t j = 0; j < rVol.size() && !added_vol; j++ ) {
            if( vol_ratio > rVol[j].ratio ) {
                added_vol = true;
                rVol.insert( rVol.begin() + j, ratio_index( vol_ratio, i ) );
            }
        }
        if( !added_vol ) {
            rVol.push_back( ratio_index( vol_ratio, i ) );
        }
    }

    std::stringstream item_name; // For description below
    int num_items_dropped = 0; // For description below
    // Now, drop items, starting from the top of each list
    while( weight_dropped < weight || volume_dropped < volume ) {
        // weight and volume may be passed as 0 or a negative value, to indicate that
        // decreasing that variable is not important.
        int dWeight = ( weight <= 0 ? -1 : weight - weight_dropped );
        int dVolume = ( volume <= 0 ? -1 : volume - volume_dropped );
        int index;
        // Which is more important, weight or volume?
        if( dWeight > dVolume ) {
            index = rWgt[0].index;
            rWgt.erase( rWgt.begin() );
            // Fix the rest of those indices.
            for( auto &elem : rWgt ) {
                if( elem.index > index ) {
                    elem.index--;
                }
            }
        } else {
            index = rVol[0].index;
            rVol.erase( rVol.begin() );
            // Fix the rest of those indices.
            for( size_t i = 0; i < rVol.size(); i++ ) {
                if( i > rVol.size() ) {
                    debugmsg( "npc::drop_items() - looping through rVol - Size is %d, i is %d",
                              rVol.size(), i );
                }
                if( rVol[i].index > index ) {
                    rVol[i].index--;
                }
            }
        }
        weight_dropped += slice[index]->front().weight() / 1_gram;
        volume_dropped += slice[index]->front().volume() / units::legacy_volume_factor;
        item dropped = i_rem( index );
        num_items_dropped++;
        if( num_items_dropped == 1 ) {
            item_name << dropped.tname();
        } else if( num_items_dropped == 2 ) {
            item_name << _( " and " ) << dropped.tname();
        }
        if( !is_hallucination() ) { // hallucinations can't drop real items
            g->m.add_item_or_charges( pos(), dropped );
        }
    }
    // Finally, describe the action if u can see it
    if( g->u.sees( *this ) ) {
        if( num_items_dropped >= 3 ) {
            add_msg( ngettext( "%s drops %d item.", "%s drops %d items.", num_items_dropped ), name,
                     num_items_dropped );
        } else {
            add_msg( _( "%1$s drops a %2$s." ), name, item_name.str() );
        }
    }
    update_worst_item_value();
}

bool npc::find_corpse_to_pulp()
{
    if( ( is_player_ally() && ( !rules.has_flag( ally_rule::allow_pulp ) || g->u.in_vehicle ) ) ||
        is_hallucination() ) {
        return false;
    }

    // Pathing with overdraw can get expensive, limit it
    int path_counter = 4;
    const auto check_tile = [this, &path_counter]( const tripoint & p ) -> const item * {
        if( !g->m.sees_some_items( p, *this ) || !sees( p ) )
        {
            return nullptr;
        }

        const auto items = g->m.i_at( p );
        const item *found = nullptr;
        for( const item &it : items )
        {
            // Pulp only stuff that revives, but don't pulp acid stuff
            // That is, if you aren't protected from this stuff!
            if( it.can_revive() ) {
                // If the first encountered corpse bleeds something dangerous then
                // it is not safe to bash.
                if( is_dangerous_field( field_entry( it.get_mtype()->bloodType(), 1, 0_turns ) ) ) {
                    return nullptr;
                }

                found = &it;
                break;
            }
        }

        if( found != nullptr )
        {
            path_counter--;
            // Only return corpses we can path to
            return update_path( p, false, false ) ? found : nullptr;
        }

        return nullptr;
    };

    const int range = 6;

    const item *corpse = nullptr;
    if( pulp_location && square_dist( pos(), *pulp_location ) <= range ) {
        corpse = check_tile( *pulp_location );
    }

    // Find the old target to avoid spamming
    const item *old_target = corpse;

    if( corpse == nullptr ) {
        // If we're following the player, don't wander off to pulp corpses
        const tripoint &around = is_walking_with() ? g->u.pos() : pos();
        for( const item_location &location : g->m.get_active_items_in_radius( around, range,
                special_item_type::corpse ) ) {
            corpse = check_tile( location.position() );

            if( corpse != nullptr ) {
                pulp_location.emplace( location.position() );
                break;
            }

            if( path_counter <= 0 ) {
                break;
            }
        }
    }

    if( corpse != nullptr && corpse != old_target && is_walking_with() ) {
        say( _( "Hold on, I want to pulp that %s." ), corpse->tname() );
    }

    return corpse != nullptr;
}

bool npc::do_pulp()
{
    if( !pulp_location ) {
        return false;
    }

    if( rl_dist( *pulp_location, pos() ) > 1 || pulp_location->z != posz() ) {
        return false;
    }

    // TODO: Don't recreate the activity every time
    int old_moves = moves;
    assign_activity( activity_id( "ACT_PULP" ), calendar::INDEFINITELY_LONG, 0 );
    activity.placement = *pulp_location;
    activity.do_turn( *this );
    return moves != old_moves;
}

bool npc::do_player_activity()
{
    int old_moves = moves;
    while( moves > 0 && activity ) {
        activity.do_turn( *this );
        if( !is_active() ) {
            return true;
        }
    }
    /* if the activity is finished, grab any backlog or change the mission */
    if( !has_destination() && !activity ) {
        add_msg( m_info, string_format( "%s completed the assigned task.", disp_name() ) );
        if( !backlog.empty() ) {
            activity = backlog.front();
            backlog.pop_front();
            current_activity_id = activity.id();
        } else {
            current_activity_id = activity_id::NULL_ID();
            revert_after_activity();
            // if we loaded after being out of the bubble for a while, we might have more
            // moves than we need, so clear them
            set_moves( 0 );
        }
    }
    return moves != old_moves;
}

bool npc::wield_better_weapon()
{
    // TODO: Allow wielding weaker weapons against weaker targets
    bool can_use_gun = ( !is_player_ally() || rules.has_flag( ally_rule::use_guns ) );
    bool use_silent = ( is_player_ally() && rules.has_flag( ally_rule::use_silent ) );
    invslice slice = inv.slice();

    // Check if there's something better to wield
    item *best = &weapon;
    double best_value = -100.0;

    const int ups_charges = charges_of( "UPS" );

    const auto compare_weapon =
    [this, &best, &best_value, ups_charges, can_use_gun, use_silent]( const item & it ) {
        bool allowed = can_use_gun && it.is_gun() && ( !use_silent || it.is_silent() );
        double val;
        if( !allowed ) {
            val = weapon_value( it, 0 );
        } else {
            int ammo_count = it.ammo_remaining();
            int ups_drain = it.get_gun_ups_drain();
            if( ups_drain > 0 ) {
                ammo_count = std::min( ammo_count, ups_charges / ups_drain );
            }

            val = weapon_value( it, ammo_count );
        }

        if( val > best_value ) {
            best = const_cast<item *>( &it );
            best_value = val;
        }
    };

    compare_weapon( weapon );
    // To prevent changing to barely better stuff
    best_value *= std::max<float>( 1.0f, ai_cache.danger_assessment / 10.0f );

    // Fists aren't checked below
    compare_weapon( null_item_reference() );

    visit_items( [&compare_weapon]( item * node ) {
        // Only compare melee weapons, guns, or holstered items
        if( node->is_melee() || node->is_gun() ) {
            compare_weapon( *node );
        } else if( node->get_use( "holster" ) && !node->contents.empty() ) {
            const item &holstered = node->get_contained();
            if( holstered.is_melee() || holstered.is_gun() ) {
                compare_weapon( holstered );
            }
        }
        return VisitResponse::SKIP;
    } );

    // TODO: Reimplement switching to empty guns
    // Needs to check reload speed, RELOAD_ONE etc.
    // Until then, the NPCs should reload the guns as a last resort

    if( best == &weapon ) {
        add_msg( m_debug, "Wielded %s is best at %.1f, not switching", best->type->get_id(),
                 best_value );
        return false;
    }

    add_msg( m_debug, "Wielding %s at value %.1f", best->type->get_id(), best_value );

    wield( *best );
    return true;
}

bool npc::scan_new_items()
{
    add_msg( m_debug, "%s scanning new items", name );
    if( wield_better_weapon() ) {
        return true;
    } else {
        // Stop "having new items" when you no longer do anything with them
        has_new_items = false;
    }

    return false;
    // TODO: Armor?
}

static void npc_throw( npc &np, item &it, int index, const tripoint &pos )
{
    if( g->u.sees( np ) ) {
        add_msg( _( "%1$s throws a %2$s." ), np.name, it.tname() );
    }

    int stack_size = -1;
    if( it.count_by_charges() ) {
        stack_size = it.charges;
        it.charges = 1;
    }
    if( !np.is_hallucination() ) { // hallucinations only pretend to throw
        np.throw_item( pos, it );
    }
    // Throw a single charge of a stacking object.
    if( stack_size == -1 || stack_size == 1 ) {
        np.i_rem( index );
    } else {
        it.charges = stack_size - 1;
    }
}

bool npc::alt_attack()
{
    if( ( is_player_ally() && !rules.has_flag( ally_rule::use_grenades ) ) || is_hallucination() ) {
        return false;
    }

    Creature *critter = current_target();
    if( critter == nullptr ) {
        // This function shouldn't be called...
        debugmsg( "npc::alt_attack() called with no target" );
        move_pause();
        return false;
    }

    tripoint tar = critter->pos();

    const int dist = rl_dist( pos(), tar );
    item *used = nullptr;
    // Remember if we have an item that is dangerous to hold
    bool used_dangerous = false;

    static const std::string danger_string( "NPC_THROW_NOW" );
    static const std::string alt_string( "NPC_ALT_ATTACK" );
    // TODO: The active bomb with shortest fuse should be thrown first
    const auto check_alt_item = [&used, &used_dangerous, dist, this]( item & it ) {
        const bool dangerous = it.has_flag( danger_string );
        if( !dangerous && used_dangerous ) {
            return;
        }

        // Not alt attack
        if( !dangerous && !it.has_flag( alt_string ) ) {
            return;
        }

        // TODO: Non-thrown alt items
        if( !dangerous && throw_range( it ) < dist ) {
            return;
        }

        // Low priority items
        if( !dangerous && used != nullptr ) {
            return;
        }

        used = &it;
        used_dangerous = used_dangerous || dangerous;
    };

    check_alt_item( weapon );
    for( auto &sl : inv.slice() ) {
        // TODO: Cached values - an itype slot maybe?
        check_alt_item( sl->front() );
    }

    if( used == nullptr ) {
        return false;
    }

    int weapon_index = get_item_position( used );
    if( weapon_index == INT_MIN ) {
        debugmsg( "npc::alt_attack() couldn't find expected item %s", used->tname() );
        return false;
    }

    // Are we going to throw this item?
    if( !used->active && used->has_flag( "NPC_ACTIVATE" ) ) {
        activate_item( weapon_index );
        // Note: intentional lack of return here
        // We want to ignore player-centric rules to avoid carrying live explosives
        // TODO: Non-grenades
    }

    // We are throwing it!
    int conf = confident_throw_range( *used, critter );
    const bool wont_hit = wont_hit_friend( tar, *used, true );
    if( dist <= conf && wont_hit ) {
        npc_throw( *this, *used, weapon_index, tar );
        return true;
    }

    if( wont_hit ) {
        // Within this block, our chosen target is outside of our range
        update_path( tar );
        move_to_next(); // Move towards the target
    }

    // Danger of friendly fire
    if( !wont_hit && !used_dangerous ) {
        // Safe to hold on to, for now
        // Maneuver around player
        avoid_friendly_fire();
        return true;
    }

    // We need to throw this live (grenade, etc) NOW! Pick another target?
    for( int dist = 2; dist <= conf; dist++ ) {
        for( const tripoint &pt : g->m.points_in_radius( pos(), dist ) ) {
            const monster *const target_ptr = g->critter_at<monster>( pt );
            int newdist = rl_dist( pos(), pt );
            // TODO: Change "newdist >= 2" to "newdist >= safe_distance(used)"
            if( newdist <= conf && newdist >= 2 && target_ptr &&
                wont_hit_friend( pt, *used, true ) ) {
                // Friendlyfire-safe!
                ai_cache.target = g->shared_from( *target_ptr );
                if( !one_in( 100 ) ) {
                    // Just to prevent infinite loops...
                    if( alt_attack() ) {
                        return true;
                    }
                }
                return false;
            }
        }
    }
    /* If we have reached THIS point, there's no acceptable monster to throw our
     * grenade or whatever at.  Since it's about to go off in our hands, better to
     * just chuck it as far away as possible--while being friendly-safe.
     */
    int best_dist = 0;
    for( int dist = 2; dist <= conf; dist++ ) {
        for( const tripoint &pt : g->m.points_in_radius( pos(), dist ) ) {
            int new_dist = rl_dist( pos(), pt );
            if( new_dist > best_dist && wont_hit_friend( pt, *used, true ) ) {
                best_dist = new_dist;
                tar = pt;
            }
        }
    }
    /* Even if tar.x/tar.y didn't get set by the above loop, throw it anyway.  They
     * should be equal to the original location of our target, and risking friendly
     * fire is better than holding on to a live grenade / whatever.
     */
    npc_throw( *this, *used, weapon_index, tar );
    return true;
}

void npc::activate_item( int item_index )
{
    const int oldmoves = moves;
    item &it = i_at( item_index );
    if( it.is_tool() || it.is_food() ) {
        it.type->invoke( *this, it, pos() );
    }

    if( moves == oldmoves ) {
        // A hack to prevent debugmsgs when NPCs activate 0 move items
        // while not removing the debugmsgs for other 0 move actions
        moves--;
    }
}

void npc::heal_player( player &patient )
{
    int dist = rl_dist( pos(), patient.pos() );

    if( dist > 1 ) {
        // We need to move to the player
        update_path( patient.pos() );
        move_to_next();
        return;
    }

    // Close enough to heal!
    bool u_see = g->u.sees( *this ) || g->u.sees( patient );
    if( u_see ) {
        add_msg( _( "%1$s heals %2$s." ), disp_name(), patient.disp_name() );
    }

    item &used = get_healing_item( ai_cache.can_heal );
    if( used.is_null() ) {
        debugmsg( "%s tried to heal you but has no healing item", disp_name() );
        return;
    }
    if( !is_hallucination() ) {
        int charges_used = used.type->invoke( *this, used, patient.pos(), "heal" );
        consume_charges( used, charges_used );
    } else {
        pretend_heal( patient, used );
    }

    if( !patient.is_npc() ) {
        // Test if we want to heal the player further
        if( op_of_u.value * 4 + op_of_u.trust + personality.altruism * 3 -
            op_of_u.fear * 3 < 25 ) {
            say( _( "That's all the healing I can do." ) );
        } else {
            say( _( "Hold still, I can heal you more." ) );
        }
    }
}

void npc:: pretend_heal( player &patient, item used )
{
    if( g->u.sees( *this ) ) {
        add_msg( _( "%1$s heals %2$s." ), disp_name(),
                 patient.disp_name() ); // you can tell that it's not real by looking at your HP though
    }
    consume_charges( used, 1 ); // empty hallucination's inventory to avoid spammming
    moves -= 100; // consumes moves to avoid infinite loop
}

void npc::heal_self()
{
    item &used = get_healing_item( ai_cache.can_heal );
    if( used.is_null() ) {
        debugmsg( "%s tried to heal self but has no healing item", disp_name() );
        return;
    }

    if( g->u.sees( *this ) ) {
        add_msg( _( "%s applies a %s" ), disp_name(), used.tname() );
    }
    warn_about( "heal_self", 1_turns );

    int charges_used = used.type->invoke( *this, used, pos(), "heal" );
    if( used.is_medication() ) {
        consume_charges( used, charges_used );
    }
}

void npc::use_painkiller()
{
    // First, find the best painkiller for our pain level
    item *it = inv.most_appropriate_painkiller( get_pain() );

    if( it->is_null() ) {
        debugmsg( "NPC tried to use painkillers, but has none!" );
        move_pause();
    } else {
        if( g->u.sees( *this ) ) {
            add_msg( _( "%1$s takes some %2$s." ), disp_name(), it->tname() );
        }
        consume( inv.position_by_item( it ) );
        moves = 0;
    }
}

// We want our food to:
// Provide enough nutrition and quench
// Not provide too much of either (don't waste food)
// Not be unhealthy
// Not have side effects
// Be eaten before it rots (favor soon-to-rot perishables)
static float rate_food( const item &it, int want_nutr, int want_quench )
{
    const auto &food = it.get_comestible();
    if( !food ) {
        return 0.0f;
    }

    if( food->parasites && !it.has_flag( "NO_PARASITES" ) ) {
        return 0.0;
    }

    int nutr = food->get_nutr();
    int quench = food->quench;

    if( nutr <= 0 && quench <= 0 ) {
        // Not food - may be salt, drugs etc.
        return 0.0f;
    }

    if( !it.type->use_methods.empty() ) {
        // TODO: Get a good method of telling apart:
        // raw meat (parasites - don't eat unless mutant)
        // zed meat (poison - don't eat unless mutant)
        // alcohol (debuffs, health drop - supplement diet but don't bulk-consume)
        // caffeine (fine to consume, but expensive and prevents sleep)
        // hallucination mushrooms (NPCs don't hallucinate, so don't eat those)
        // honeycomb (harmless iuse)
        // royal jelly (way too expensive to eat as food)
        // mutagenic crap (don't eat, we want player to micromanage muties)
        // marloss (NPCs don't turn fungal)
        // weed brownies (small debuff)
        // seeds (too expensive)

        // For now skip all of those
        return 0.0f;
    }

    double relative_rot = it.get_relative_rot();
    if( relative_rot >= 1.0f ) {
        // TODO: Allow sapro mutants to eat it anyway and make them prefer it
        return 0.0f;
    }

    float weight = std::max( 1.0, 10.0 * relative_rot );
    if( food->fun < 0 ) {
        // This helps to avoid eating stuff like flour
        weight /= ( -food->fun ) + 1;
    }

    if( food->healthy < 0 ) {
        weight /= ( -food->healthy ) + 1;
    }

    // Avoid wasting quench values unless it's about to rot away
    if( relative_rot < 0.9f && quench > want_quench ) {
        weight -= ( 1.0f - relative_rot ) * ( quench - want_quench );
    }

    if( quench < 0 && want_quench > 0 && want_nutr < want_quench ) {
        // Avoid stuff that makes us thirsty when we're more thirsty than hungry
        weight = weight * want_nutr / want_quench;
    }

    if( nutr > want_nutr ) {
        // TODO: Allow overeating in some cases
        if( nutr >= 5 ) {
            return 0.0f;
        }

        if( relative_rot < 0.9f ) {
            weight /= nutr - want_nutr;
        }
    }

    if( it.poison > 0 ) {
        weight -= it.poison;
    }

    return weight;
}

bool npc::consume_food()
{
    float best_weight = 0.0f;
    int index = -1;
    int want_hunger = get_hunger();
    int want_quench = get_thirst();
    invslice slice = inv.slice();
    for( size_t i = 0; i < slice.size(); i++ ) {
        const item &it = slice[i]->front();
        const item &food_item = it.is_food_container() ?
                                it.contents.front() : it;
        float cur_weight = rate_food( food_item, want_hunger, want_quench );
        // Note: will_eat is expensive, avoid calling it if possible
        if( cur_weight > best_weight && will_eat( food_item ).success() ) {
            best_weight = cur_weight;
            index = i;
        }
    }

    if( index == -1 ) {
        if( !is_player_ally() ) {
            // TODO: Remove this and let player "exploit" hungry NPCs
            set_hunger( 0 );
            set_thirst( 0 );
        }
        return false;
    }

    // consume doesn't return a meaningful answer, we need to compare moves
    // TODO: Make player::consume return false if it fails to consume
    int old_moves = moves;
    bool consumed = consume( index ) && old_moves != moves;
    if( !consumed ) {
        debugmsg( "%s failed to consume %s", name, i_at( index ).tname() );
    }

    return consumed;
}

void npc::mug_player( player &mark )
{
    if( mark.is_armed() ) {
        make_angry();
    }

    if( rl_dist( pos(), mark.pos() ) > 1 ) { // We have to travel
        update_path( mark.pos() );
        move_to_next();
        return;
    }

    const bool u_see = g->u.sees( *this ) || g->u.sees( mark );
    if( mark.cash > 0 ) {
        if( !is_hallucination() ) { // hallucinations can't take items
            cash += mark.cash;
            mark.cash = 0;
        }
        moves = 0;
        // Describe the action
        if( mark.is_npc() ) {
            if( u_see ) {
                add_msg( _( "%1$s takes %2$s's money!" ), name, mark.name );
            }
        } else {
            add_msg( m_bad, _( "%s takes your money!" ), name );
        }
        return;
    }

    // We already have their money; take some goodies!
    // value_mod affects at what point we "take the money and run"
    // A lower value means we'll take more stuff
    double value_mod = 1 - ( ( 10 - personality.bravery ) * .05 ) -
                       ( ( 10 - personality.aggression ) * .04 ) -
                       ( ( 10 - personality.collector ) * .06 );
    if( !mark.is_npc() ) {
        value_mod += ( op_of_u.fear * .08 );
        value_mod -= ( ( 8 - op_of_u.value ) * .07 );
    }
    double best_value = minimum_item_value() * value_mod;
    int item_index = INT_MIN;
    invslice slice = mark.inv.slice();
    for( size_t i = 0; i < slice.size(); i++ ) {
        if( value( slice[i]->front() ) >= best_value &&
            can_pickVolume( slice[i]->front(), true ) &&
            can_pickWeight( slice[i]->front(), true ) ) {
            best_value = value( slice[i]->front() );
            item_index = i;
        }
    }
    if( item_index == INT_MIN ) { // Didn't find anything worthwhile!
        set_attitude( NPCATT_FLEE_TEMP );
        if( !one_in( 3 ) ) {
            say( "<done_mugging>" );
        }
        moves -= 100;
        return;
    }
    item stolen;
    if( !is_hallucination() ) {
        stolen = mark.i_rem( item_index );
        i_add( stolen );
    }
    if( mark.is_npc() ) {
        if( u_see ) {
            add_msg( _( "%1$s takes %2$s's %3$s." ), name, mark.name, stolen.tname() );
        }
    } else {
        add_msg( m_bad, _( "%1$s takes your %2$s." ), name, stolen.tname() );
    }
    moves -= 100;
    if( !mark.is_npc() ) {
        op_of_u.value -= rng( 0, 1 );  // Decrease the value of the player
    }
}

void npc::look_for_player( const player &sought )
{
    complain_about( "look_for_player", 5_minutes, "<wait>", false );
    update_path( sought.pos() );
    move_to_next();
    return;
    // The part below is not implemented properly
    /*
    if( sees( sought ) ) {
        move_pause();
        return;
    }

    if (!path.empty()) {
        const tripoint &dest = path[path.size() - 1];
        if( !sees( dest ) ) {
            move_to_next();
            return;
        }
        path.clear();
    }
    std::vector<point> possibilities;
    for (int x = 1; x < MAPSIZE_X; x += 11) { // 1, 12, 23, 34
        for (int y = 1; y < MAPSIZE_Y; y += 11) {
            if( sees( x, y ) ) {
                possibilities.push_back(point(x, y));
            }
        }
    }
    if (possibilities.empty()) { // We see all the spots we'd like to check!
        say("<wait>");
        move_pause();
    } else {
        if (one_in(6)) {
            say("<wait>");
        }
        update_path( tripoint( random_entry( possibilities ), posz() ) );
        move_to_next();
    }
    */
}

bool npc::saw_player_recently() const
{
    return last_player_seen_pos && g->m.inbounds( *last_player_seen_pos ) && last_seen_player_turn > 0;
}

bool npc::has_omt_destination() const
{
    return goal != no_goal_point;
}

void npc::reach_omt_destination()
{
    if( !omt_path.empty() ) {
        omt_path.clear();
    }
    if( is_travelling() ) {
        guard_pos = g->m.getabs( pos() );
        goal = no_goal_point;
        if( is_player_ally() ) {
            talk_function::assign_guard( *this );
            if( rl_dist( g->u.pos(), pos() ) > SEEX * 2 || !g->u.sees( pos() ) ) {
                if( g->u.has_item_with_flag( "TWO_WAY_RADIO", true ) &&
                    has_item_with_flag( "TWO_WAY_RADIO", true ) ) {
                    add_msg( m_info, _( "From your two-way radio you hear %s reporting in, "
                                        " 'I've arrived, boss!'" ), disp_name() );
                }
            }
        } else {
            revert_after_activity();
        }
        return;
    }
    // Guarding NPCs want a specific point, not just an overmap tile
    // Rest stops having a goal after reaching it
    if( !( is_guarding() || is_patrolling() ) ) {
        goal = no_goal_point;
        return;
    }
    // If we are guarding, remember our position in case we get forcibly moved
    goal = global_omt_location();
    if( guard_pos == g->m.getabs( pos() ) ) {
        // This is the specific point
        return;
    }

    if( path.size() > 1 ) {
        // No point recalculating the path to get home
        move_to_next();
    } else if( guard_pos != no_goal_point ) {
        update_path( g->m.getlocal( guard_pos ) );
        move_to_next();
    } else {
        guard_pos = g->m.getabs( pos() );
    }
}

void npc::set_omt_destination()
{
    /* TODO: Make NPCs' movement more intelligent.
     * Right now this function just makes them attempt to address their needs:
     *  if we need ammo, go to a gun store, if we need food, go to a grocery store,
     *  and if we don't have any needs, pick a random spot.
     * What it SHOULD do is that, if there's time; but also look at our mission and
     *  our faction to determine more meaningful actions, such as attacking a rival
     *  faction's base, or meeting up with someone friendly.  NPCs should also
     *  attempt to reach safety before nightfall, and possibly similar goals.
     * Also, NPCs should be able to assign themselves missions like "break into that
     *  lab" or "map that river bank."
     */
    if( is_stationary( true ) ) {
        guard_current_pos();
        return;
    }

    // all of the following luxuries are at ground level.
    // so please wallow in hunger & fear if below ground.
    if( posz() != 0 && !g->m.has_zlevels() ) {
        goal = no_goal_point;
        return;
    }

    tripoint surface_omt_loc = global_omt_location();
    // We need that, otherwise find_closest won't work properly
    surface_omt_loc.z = 0;

    // also, don't bother looking if the CITY_SIZE is 0, just go somewhere at random
    const int city_size = get_option<int>( "CITY_SIZE" );
    if( city_size == 0 ) {
        goal = surface_omt_loc + point( rng( -90, 90 ), rng( -90, 90 ) );
        return;
    }

    decide_needs();
    if( needs.empty() ) { // We don't need anything in particular.
        needs.push_back( need_none );
    }

    std::string dest_type;
    for( const auto &fulfill : needs ) {
        dest_type = get_location_for( fulfill )->get_random_terrain().id().str();
        goal = overmap_buffer.find_closest( surface_omt_loc, dest_type, 100, false );
        omt_path = overmap_buffer.get_npc_path( surface_omt_loc, goal );
        if( goal != overmap::invalid_tripoint && !omt_path.empty() ) {
            break;
        }
    }

    // couldn't find any places to go, so go somewhere.
    if( goal == overmap::invalid_tripoint || omt_path.empty() ) {
        goal = surface_omt_loc + point( rng( -90, 90 ), rng( -90, 90 ) );
        omt_path = overmap_buffer.get_npc_path( surface_omt_loc, goal );
        // try one more time
        if( omt_path.empty() ) {
            goal = surface_omt_loc + point( rng( -90, 90 ), rng( -90, 90 ) );
            omt_path = overmap_buffer.get_npc_path( surface_omt_loc, goal );
        }
        if( omt_path.empty() ) {
            goal = no_goal_point;
        }
        return;
    }

    DebugLog( D_INFO, DC_ALL ) << "npc::set_omt_destination - new goal for NPC [" << get_name() <<
                               "] with [" << get_need_str_id( needs.front() ) <<
                               "] is [" << dest_type <<
                               "] in [" << goal.x << "," << goal.y << "," << goal.z << "].";
}

void npc::go_to_omt_destination()
{
    if( ai_cache.guard_pos ) {
        if( g->m.getabs( pos() ) == *ai_cache.guard_pos ) {
            path.clear();
            ai_cache.guard_pos = cata::nullopt;
            move_pause();
            return;
        }
    }
    if( goal == no_goal_point || omt_path.empty() ) {
        add_msg( m_debug, "npc::go_to_destination with no goal" );
        move_pause();
        reach_omt_destination();
        return;
    }
    const tripoint omt_pos = global_omt_location();
    if( goal == omt_pos ) {
        // We're at our desired map square!  Pause to keep the NPC infinite loop counter happy
        move_pause();
        reach_omt_destination();
        return;
    }
    if( !path.empty() ) {
        // we already have a path, just use that until we cant.
        move_to_next();
        return;
    }
    // get the next path point
    if( !omt_path.empty() && omt_path.back() == omt_pos ) {
        // this should be the square we are at.
        omt_path.pop_back();
    }
    if( !omt_path.empty() ) {
        point omt_diff = omt_path.back().xy() - omt_pos.xy();
        if( omt_diff.x > 3 || omt_diff.x < -3 || omt_diff.y > 3 || omt_diff.y < -3 ) {
            // we've gone wandering somehow, reset destination.
            if( !is_player_ally() ) {
                set_omt_destination();
            } else {
                talk_function::assign_guard( *this );
            }
            return;
        }
    }
    tripoint sm_tri = g->m.getlocal( sm_to_ms_copy( omt_to_sm_copy( omt_path.back() ) ) );
    tripoint centre_sub = sm_tri + point( SEEX, SEEY );
    if( !g->m.passable( centre_sub ) ) {
        auto candidates = g->m.points_in_radius( centre_sub, 2 );
        for( const auto &elem : candidates ) {
            if( g->m.passable( elem ) ) {
                centre_sub = elem;
                break;
            }
        }
    }
    path = g->m.route( pos(), centre_sub, get_pathfinding_settings(), get_path_avoid() );
    add_msg( m_debug, "%s going (%d,%d,%d)->(%d,%d,%d)", name,
             omt_pos.x, omt_pos.y, omt_pos.z, goal.x, goal.y, goal.z );

    if( !path.empty() ) {
        move_to_next();
        return;
    }
    move_pause();
}

void npc::guard_current_pos()
{
    goal = global_omt_location();
    guard_pos = g->m.getabs( pos() );
}

std::string npc_action_name( npc_action action )
{
    switch( action ) {
        case npc_undecided:
            return "Undecided";
        case npc_pause:
            return "Pause";
        case npc_reload:
            return "Reload";
        case npc_investigate_sound:
            return "Investigate sound";
        case npc_return_to_guard_pos:
            return "Returning to guard position";
        case npc_sleep:
            return "Sleep";
        case npc_pickup:
            return "Pick up items";
        case npc_heal:
            return "Heal self";
        case npc_use_painkiller:
            return "Use painkillers";
        case npc_drop_items:
            return "Drop items";
        case npc_flee:
            return "Flee";
        case npc_melee:
            return "Melee";
        case npc_reach_attack:
            return "Reach attack";
        case npc_aim:
            return "Aim";
        case npc_shoot:
            return "Shoot";
        case npc_look_for_player:
            return "Look for player";
        case npc_heal_player:
            return "Heal player or ally";
        case npc_follow_player:
            return "Follow player";
        case npc_follow_embarked:
            return "Follow player (embarked)";
        case npc_talk_to_player:
            return "Talk to player";
        case npc_mug_player:
            return "Mug player";
        case npc_goto_destination:
            return "Go to destination";
        case npc_avoid_friendly_fire:
            return "Avoid friendly fire";
        case npc_escape_explosion:
            return "Escape explosion";
        case npc_player_activity:
            return "Performing activity";
        default:
            return "Unnamed action";
    }
}

void print_action( const char *prepend, npc_action action )
{
    if( action != npc_undecided ) {
        add_msg( m_debug, prepend, npc_action_name( action ) );
    }
}

const Creature *npc::current_target() const
{
    // TODO: Arguably we should return a shared_ptr to ensure that the returned
    // object stays alive while the caller uses it.  Not doing that for now.
    return ai_cache.target.lock().get();
}

Creature *npc::current_target()
{
    // TODO: As above.
    return ai_cache.target.lock().get();
}

const Creature *npc::current_ally() const
{
    // TODO: Arguably we should return a shared_ptr to ensure that the returned
    // object stays alive while the caller uses it.  Not doing that for now.
    return ai_cache.ally.lock().get();
}

Creature *npc::current_ally()
{
    // TODO: As above.
    return ai_cache.ally.lock().get();
}

// Maybe TODO: Move to Character method and use map methods
static body_part bp_affected( npc &who, const efftype_id &effect_type )
{
    body_part ret = num_bp;
    int highest_intensity = INT_MIN;
    for( const body_part bp : all_body_parts ) {
        const auto &eff = who.get_effect( effect_type, bp );
        if( !eff.is_null() && eff.get_intensity() > highest_intensity ) {
            ret = bp;
            highest_intensity = eff.get_intensity();
        }
    }

    return ret;
}

void npc::warn_about( const std::string &type, const time_duration &d, const std::string &name )
{
    std::string snip;
    sounds::sound_t spriority = sounds::sound_t::alert;
    if( type == "monster" ) {
        snip = is_enemy() ? "<monster_warning_h>" : "<monster_warning>";
    } else if( type == "explosion" ) {
        snip = is_enemy() ? "<fire_in_the_hole_h>" : "<fire_in_the_hole>";
    } else if( type == "general_danger" ) {
        snip = is_enemy() ? "<general_danger_h>" : "<general_danger>";
        spriority = sounds::sound_t::speech;
    } else if( type == "relax" ) {
        snip = is_enemy() ? "<its_safe_h>" : "<its_safe>";
        spriority = sounds::sound_t::speech;
    } else if( type == "kill_npc" ) {
        snip = is_enemy() ? "<kill_npc_h>" : "<kill_npc>";
    } else if( type == "kill_player" ) {
        snip = is_enemy() ? "<kill_player_h>" : "";
    } else if( type == "run_away" ) {
        snip = "<run_away>";
    } else if( type == "cant_flee" ) {
        snip = "<cant_flee>";
    } else if( type == "fire_bad" ) {
        snip = "<fire_bad>";
    } else if( type == "speech_noise" ) {
        snip = "<speech_warning>";
        spriority = sounds::sound_t::speech;
    } else if( type == "combat_noise" ) {
        snip = "<combat_noise_warning>";
        spriority = sounds::sound_t::speech;
    } else if( type == "movement_noise" ) {
        snip = "<movement_noise_warning>";
        spriority = sounds::sound_t::speech;
    } else if( type == "heal_self" ) {
        snip = "<heal_self>";
        spriority = sounds::sound_t::speech;
    } else {
        return;
    }
    const std::string warning_name = "warning_" + type + name;
    const std::string speech = name.empty() ? snip :
                               string_format( _( "%s %s<punc>" ), snip, name );
    complain_about( warning_name, d, speech, is_enemy(), static_cast<int>( spriority ) );
}

bool npc::complain_about( const std::string &issue, const time_duration &dur,
                          const std::string &speech, const bool force, const int priority )
{
    // Don't have a default constructor for time_point, so accessing it in the
    // complaints map is a bit difficult, those lambdas should cover it.
    const auto complain_since = [this]( const std::string & key, const time_duration & d ) {
        const auto iter = complaints.find( key );
        return iter == complaints.end() || iter->second < calendar::turn - d;
    };
    const auto set_complain_since = [this]( const std::string & key ) {
        const auto iter = complaints.find( key );
        if( iter == complaints.end() ) {
            complaints.emplace( key, calendar::turn );
        } else {
            iter->second = calendar::turn;
        }
    };

    // Don't wake player up with non-serious complaints
    // Stop complaining while asleep
    const bool do_complain = force || ( rules.has_flag( ally_rule::allow_complain ) &&
                                        !g->u.in_sleep_state() && !in_sleep_state() );

    if( complain_since( issue, dur ) && do_complain ) {
        say( speech, priority );
        set_complain_since( issue );
        return true;
    }
    return false;
}

bool npc::complain()
{
    static const std::string infected_string = "infected";
    static const std::string fatigue_string = "fatigue";
    static const std::string bite_string = "bite";
    static const std::string bleed_string = "bleed";
    static const std::string radiation_string = "radiation";
    static const std::string hunger_string = "hunger";
    static const std::string thirst_string = "thirst";

    if( !is_player_ally() || !g->u.sees( *this ) ) {
        return false;
    }

    // When infected, complain every (4-intensity) hours
    // At intensity 3, ignore player wanting us to shut up
    if( has_effect( effect_infected ) ) {
        body_part bp = bp_affected( *this, effect_infected );
        const auto &eff = get_effect( effect_infected, bp );
        int intensity = eff.get_intensity();
        const std::string speech = string_format( _( "My %s wound is infected..." ),
                                   body_part_name( bp ) );
        if( complain_about( infected_string, time_duration::from_hours( 4 - intensity ), speech,
                            intensity >= 3 ) ) {
            // Only one complaint per turn
            return true;
        }
    }

    // When bitten, complain every hour, but respect restrictions
    if( has_effect( effect_bite ) ) {
        body_part bp = bp_affected( *this, effect_bite );
        const std::string speech = string_format( _( "The bite wound on my %s looks bad." ),
                                   body_part_name( bp ) );
        if( complain_about( bite_string, 1_hours, speech ) ) {
            return true;
        }
    }

    // When tired, complain every 30 minutes
    // If massively tired, ignore restrictions
    if( get_fatigue() > TIRED && complain_about( fatigue_string, 30_minutes, _( "<yawn>" ),
            get_fatigue() > MASSIVE_FATIGUE - 100 ) )  {
        return true;
    }

    // Radiation every 10 minutes
    if( radiation > 90 ) {
        activate_bionic_by_id( bio_radscrubber );
        std::string speech = _( "I'm suffering from radiation sickness..." );
        if( complain_about( radiation_string, 10_minutes, speech, radiation > 150 ) ) {
            return true;
        }
    } else if( !radiation ) {
        deactivate_bionic_by_id( bio_radscrubber );
    }

    // Hunger every 3-6 hours
    // Since NPCs can't starve to death, respect the rules
    if( get_hunger() > 160 &&
        complain_about( hunger_string, std::max( 3_hours,
                        time_duration::from_minutes( 60 * 8 - get_hunger() ) ), _( "<hungry>" ) ) ) {
        return true;
    }

    // Thirst every 2 hours
    // Since NPCs can't dry to death, respect the rules
    if( get_thirst() > 80 && complain_about( thirst_string, 2_hours, _( "<thirsty>" ) ) ) {
        return true;
    }

    //Bleeding every 5 minutes
    if( has_effect( effect_bleed ) ) {
        body_part bp = bp_affected( *this, effect_bleed );
        std::string speech = string_format( _( "My %s is bleeding!" ), body_part_name( bp ) );
        if( complain_about( bleed_string, 5_minutes, speech ) ) {
            return true;
        }
    }

    return false;
}

void npc::do_reload( const item &it )
{
    item::reload_option reload_opt = select_ammo( it );

    if( !reload_opt ) {
        debugmsg( "do_reload failed: no usable ammo for %s", it.tname() );
        return;
    }

    // Note: we may be reloading the magazine inside, not the gun itself
    // Maybe TODO: allow reload functions to understand such reloads instead of const casts
    item &target = const_cast<item &>( *reload_opt.target );
    item_location &usable_ammo = reload_opt.ammo;

    int qty = std::max( 1, std::min( usable_ammo->charges,
                                     it.ammo_capacity() - it.ammo_remaining() ) );
    int reload_time = item_reload_cost( it, *usable_ammo, qty );
    // TODO: Consider printing this info to player too
    const std::string ammo_name = usable_ammo->tname();
    if( !target.reload( *this, std::move( usable_ammo ), qty ) ) {
        debugmsg( "do_reload failed: item %s could not be reloaded with %ld charge(s) of %s",
                  it.tname(), qty, ammo_name );
        return;
    }

    moves -= reload_time;
    recoil = MAX_RECOIL;

    if( g->u.sees( *this ) ) {
        add_msg( _( "%1$s reloads their %2$s." ), name, it.tname() );
        sfx::play_variant_sound( "reload", it.typeId(), sfx::get_heard_volume( pos() ),
                                 sfx::get_heard_angle( pos() ) );
    }

    // Otherwise the NPC may not equip the weapon until they see danger
    has_new_items = true;
}

bool npc::adjust_worn()
{
    const auto covers_broken = [this]( const item & it, side s ) {
        const auto covered = it.get_covered_body_parts( s );
        for( size_t i = 0; i < num_hp_parts; i++ ) {
            if( hp_cur[ i ] <= 0 && covered.test( hp_to_bp( static_cast<hp_part>( i ) ) ) ) {
                return true;
            }
        }
        return false;
    };

    for( auto &elem : worn ) {
        if( !elem.has_flag( "SPLINT" ) ) {
            continue;
        }

        if( !covers_broken( elem, elem.get_side() ) ) {
            const bool needs_change = covers_broken( elem, opposite_side( elem.get_side() ) );
            // Try to change side (if it makes sense), or take off.
            if( ( needs_change && change_side( elem ) ) || takeoff( elem ) ) {
                return true;
            }
        }
    }

    return false;
}
