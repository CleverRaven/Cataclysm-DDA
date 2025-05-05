#include <algorithm>
#include <array>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "active_item_cache.h"
#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "avatar.h"
#include "basecamp.h"
#include "bionics.h"
#include "body_part_set.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_algo.h"
#include "character.h"
#include "character_attire.h"
#include "character_id.h"
#include "clzones.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "dialogue_chatbin.h"
#include "dispersion.h"
#include "effect.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "explosion.h"
#include "faction.h"
#include "field.h"
#include "field_type.h"
#include "flag.h"
#include "flat_set.h"  // IWYU pragma: keep // iwyu is being silly here
#include "game.h"
#include "game_constants.h"
#include "gates.h"
#include "gun_mode.h"
#include "inventory.h"
#include "item.h"
#include "item_factory.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "line.h"
#include "lru_cache.h"
#include "magic.h"
#include "map.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "map_selector.h"
#include "mapdata.h"
#include "memory_fast.h"
#include "messages.h"
#include "mission.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "npc_attack.h"
#include "npc_opinion.h"
#include "npctalk.h"
#include "omdata.h"
#include "options.h"
#include "overmap_location.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "pimpl.h"
#include "player_activity.h"
#include "point.h"
#include "projectile.h"
#include "ranged.h"
#include "ret_val.h"
#include "rng.h"
#include "simple_pathfinding.h"
#include "sleep.h"
#include "sounds.h"
#include "stomach.h"
#include "string_formatter.h"
#include "talker.h"  // IWYU pragma: keep
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "viewer.h"
#include "visitable.h"
#include "vpart_position.h"
#include "vpart_range.h"

enum class side : int;

static const activity_id ACT_CRAFT( "ACT_CRAFT" );
static const activity_id ACT_FIRSTAID( "ACT_FIRSTAID" );
static const activity_id ACT_MOVE_LOOT( "ACT_MOVE_LOOT" );
static const activity_id ACT_OPERATION( "ACT_OPERATION" );
static const activity_id ACT_SPELLCASTING( "ACT_SPELLCASTING" );
static const activity_id ACT_TIDY_UP( "ACT_TIDY_UP" );

static const bionic_id bio_ads( "bio_ads" );
static const bionic_id bio_blade( "bio_blade" );
static const bionic_id bio_chain_lightning( "bio_chain_lightning" );
static const bionic_id bio_claws( "bio_claws" );
static const bionic_id bio_faraday( "bio_faraday" );
static const bionic_id bio_heat_absorb( "bio_heat_absorb" );
static const bionic_id bio_heatsink( "bio_heatsink" );
static const bionic_id bio_hydraulics( "bio_hydraulics" );
static const bionic_id bio_laser( "bio_laser" );
static const bionic_id bio_leukocyte( "bio_leukocyte" );
static const bionic_id bio_nanobots( "bio_nanobots" );
static const bionic_id bio_ods( "bio_ods" );
static const bionic_id bio_painkiller( "bio_painkiller" );
static const bionic_id bio_plutfilter( "bio_plutfilter" );
static const bionic_id bio_radscrubber( "bio_radscrubber" );
static const bionic_id bio_shock( "bio_shock" );
static const bionic_id bio_soporific( "bio_soporific" );

static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_bullet( "bullet" );
static const damage_type_id damage_cut( "cut" );
static const damage_type_id damage_stab( "stab" );

static const efftype_id effect_asthma( "asthma" );
static const efftype_id effect_bandaged( "bandaged" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_bouldering( "bouldering" );
static const efftype_id effect_catch_up( "catch_up" );
static const efftype_id effect_cramped_space( "cramped_space" );
static const efftype_id effect_disinfected( "disinfected" );
static const efftype_id effect_hit_by_player( "hit_by_player" );
static const efftype_id effect_hypovolemia( "hypovolemia" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_no_sight( "no_sight" );
static const efftype_id effect_npc_fire_bad( "npc_fire_bad" );
static const efftype_id effect_npc_flee_player( "npc_flee_player" );
static const efftype_id effect_npc_player_still_looking( "npc_player_still_looking" );
static const efftype_id effect_npc_run_away( "npc_run_away" );
static const efftype_id effect_psi_stunned( "psi_stunned" );
static const efftype_id effect_stumbled_into_invisible( "stumbled_into_invisible" );
static const efftype_id effect_stunned( "stunned" );

static const field_type_str_id field_fd_last_known( "fd_last_known" );

static const itype_id itype_inhaler( "inhaler" );
static const itype_id itype_lsd( "lsd" );
static const itype_id itype_oxygen_tank( "oxygen_tank" );
static const itype_id itype_smoxygen_tank( "smoxygen_tank" );
static const itype_id itype_thorazine( "thorazine" );

static const json_character_flag json_flag_CANNOT_ATTACK( "CANNOT_ATTACK" );
static const json_character_flag json_flag_CANNOT_MOVE( "CANNOT_MOVE" );

static const npc_class_id NC_EVAC_SHOPKEEP( "NC_EVAC_SHOPKEEP" );

static const skill_id skill_firstaid( "firstaid" );

static const trait_id trait_IGNORE_SOUND( "IGNORE_SOUND" );
static const trait_id trait_RETURN_TO_START_POS( "RETURN_TO_START_POS" );

static const zone_type_id zone_type_NO_NPC_PICKUP( "NO_NPC_PICKUP" );
static const zone_type_id zone_type_NPC_RETREAT( "NPC_RETREAT" );

static constexpr float MAX_FLOAT = 5000000000.0f;

// TODO: These would be much better using common code or constants from character.cpp,
// which handles the player formatting of thirst/hunger levels. Right now we
// have magic numbers all over the place. ;(

static constexpr int NPC_THIRST_CONSUME  = 40;  // "Thirsty"
static constexpr int NPC_THIRST_COMPLAIN = 80;  // "Very thirsty"
static constexpr int NPC_HUNGER_CONSUME  = 80;  // 50% of what's needed to refuse training.
static constexpr int NPC_HUNGER_COMPLAIN = 160; // The level at which we refuse to do some tasks.

enum npc_action : int {
    npc_undecided = 0,
    npc_pause,
    npc_reload, npc_sleep,
    npc_pickup,
    npc_heal, npc_use_painkiller, npc_drop_items,
    npc_flee, npc_melee, npc_shoot,
    npc_look_for_player, npc_heal_player, npc_follow_player, npc_follow_embarked,
    npc_talk_to_player, npc_mug_player,
    npc_goto_to_this_pos,
    npc_goto_destination,
    npc_avoid_friendly_fire,
    npc_escape_explosion,
    npc_noop,
    npc_reach_attack,
    npc_do_attack,
    npc_aim,
    npc_investigate_sound,
    npc_return_to_guard_pos,
    npc_player_activity,
    npc_worker_downtime,
    num_npc_actions
};

namespace
{
const std::vector<bionic_id> defense_cbms = { {
        bio_ads,
        bio_faraday,
        bio_heat_absorb,
        bio_heatsink,
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
        bio_chain_lightning,
        bio_laser,
        bio_blade,
        bio_claws
    }
};

const int avoidance_vehicles_radius = 5;

bool good_for_pickup( const item &it, npc &who, const tripoint_bub_ms &there )
{
    return who.can_take_that( it ) &&
           who.wants_take_that( it ) &&
           who.would_take_that( it, there );
}

} // namespace

static std::string npc_action_name( npc_action action );

static void print_action( const char *prepend, npc_action action );

static bool compare_sound_alert( const dangerous_sound &sound_a, const dangerous_sound &sound_b );

bool compare_sound_alert( const dangerous_sound &sound_a, const dangerous_sound &sound_b )
{
    if( sound_a.type != sound_b.type ) {
        return sound_a.type < sound_b.type;
    }
    return sound_a.volume < sound_b.volume;
}

static bool clear_shot_reach( const tripoint_bub_ms &from, const tripoint_bub_ms &to,
                              bool check_ally = true )
{
    std::vector<tripoint_bub_ms> path = line_to( from, to );
    path.pop_back();
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint_bub_ms &p : path ) {
        Creature *inter = creatures.creature_at( p );
        if( check_ally && inter != nullptr ) {
            return false;
        }
        if( get_map().impassable( p ) ) {
            return false;
        }
    }

    return true;
}

tripoint_bub_ms npc::good_escape_direction( bool include_pos )
{
    map &here = get_map();
    // if NPC is repositioning rather than fleeing, they do smarter things
    add_msg_debug( debugmode::DF_NPC_MOVEAI,
                   "<color_brown>good_escape_direction</color> activated by %s", name );

    // To do: Eventually careful_retreat should determine if NPC will go to previously identified
    // safe locations.  For now it just sends them to a retreat zone if one exists.
    bool careful_retreat = mem_combat.repositioning || mem_combat.panic == 0;
    if( !careful_retreat ) {
        careful_retreat = mem_combat.panic < 10 + personality.bravery + get_int();
        if( !careful_retreat ) {
            add_msg_debug( debugmode::DF_NPC_MOVEAI, "%s is panicking too much to use retreat zones and stuff.",
                           name );
        } else {
            add_msg_debug( debugmode::DF_NPC_MOVEAI, "%s is running away but still being smart about it.",
                           name );
        }
    }
    //if not, consider regrouping on the player if they're getting far away.
    //in the future this should run to the strongest nearby ally, remembered in mem_combat or cached.
    bool run_to_friend = mem_combat.repositioning || mem_combat.panic == 0;
    if( !run_to_friend ) {
        run_to_friend = mem_combat.panic < 10 + personality.bravery + op_of_u.trust;
    }
    if( path.empty() && careful_retreat ) {
        add_msg_debug( debugmode::DF_NPC_MOVEAI,
                       "%s doesn't already have an escape path.  Checking for retreat zone.", name );
        zone_type_id retreat_zone = zone_type_NPC_RETREAT;
        const tripoint_abs_ms abs_pos = pos_abs();
        const zone_manager &mgr = zone_manager::get_manager();
        std::optional<tripoint_abs_ms> retreat_target = mgr.get_nearest( retreat_zone, abs_pos,
                MAX_VIEW_DISTANCE, fac_id );
        // if there is a retreat zone in range, go there


        if( retreat_target && *retreat_target != abs_pos ) {
            add_msg_debug( debugmode::DF_NPC_MOVEAI,
                           "<color_light_gray>%s is </color><color_brown>repositioning</color> to %s", name,
                           here.get_bub( *retreat_target ).to_string_writable() );
            update_path( here.get_bub( *retreat_target ) );
        }
        if( !path.empty() ) {
            return path[0];
        } else {
            add_msg_debug( debugmode::DF_NPC_MOVEAI,
                           "<color_light_gray>%s did not detect a zone to reposition to, or is too panicked.  Checking to see if there's an ally.</color>",
                           name );
        }
    } else if( path.empty() && run_to_friend && is_player_ally() ) {
        Character &player_character = get_player_character();
        int dist = rl_dist( pos_bub(), player_character.pos_bub() );
        int def_radius = rules.has_flag( ally_rule::follow_close ) ? follow_distance() : 6;
        if( dist > def_radius ) {
            add_msg_debug( debugmode::DF_NPC_MOVEAI,
                           "<color_light_gray>%s is repositioning closer to</color> you", name );
            tripoint_bub_ms destination = get_player_character().pos_bub();
            int loop_avoider = 0;
            while( !can_move_to( destination ) && loop_avoider < 10 ) {
                destination.x() += rng( -2, 2 );
                destination.y() += rng( -2, 2 );
                loop_avoider += 1;
            }
            update_path( destination );
            if( loop_avoider == 10 ) {
                add_msg_debug( debugmode::DF_NPC_MOVEAI,
                               "<color_red>%s had to break out of an infinite loop when looking for a good escape destination</color>.  This might not be that big a deal but if you're seeing this a lot, there might be something wrong in good_escape_direction().",
                               name );
            }
        }
    } else {
        add_msg_debug( debugmode::DF_NPC_MOVEAI,
                       "<color_light_gray>%s couldn't find anywhere preset to reposition to.  Looking for a random location.</color>",
                       name );
    }


    std::vector<tripoint_bub_ms> candidates;

    const auto rate_pt = [&]( const tripoint_bub_ms & pt, const float threat_val ) {
        if( !can_move_to( pt, !rules.has_flag( ally_rule::allow_bash ) ) ) {
            add_msg_debug( debugmode::DF_NPC_MOVEAI,
                           "<color_dark_gray>%s can't move to %s. Rate_pt returning absurdly high rating.</color>",
                           name, pt.to_string_writable() );
            return MAX_FLOAT;
        }
        float rating = threat_val;
        for( const auto &e : here.field_at( pt ) ) {
            if( is_dangerous_field( e.second ) ) {
                // Note, field danger should be rated more specifically than this,
                // to distinguish eg fire vs smoke. Probably needs to be handled by field code, not here.
                add_msg_debug( debugmode::DF_NPC_MOVEAI,
                               "<color_dark_gray>%s spotted field %s at %s; adding %f to rating</color>", name,
                               e.second.name(), pt.to_string_writable(), e.second.get_field_intensity() );
                rating += e.second.get_field_intensity();
            }
        }
        return rating;
    };

    float best_rating = include_pos ? rate_pt( pos_bub(), 0.0f ) : FLT_MAX;
    candidates.emplace_back( pos_bub() );

    std::map<direction, float> adj_map;
    int num_points_searched = 1;
    for( direction pt_dir : npc_threat_dir ) {
        const tripoint_bub_ms pt = pos_bub() + displace_XY( pt_dir );
        float cur_rating = rate_pt( pt, ai_cache.threat_map[ pt_dir ] );
        adj_map[pt_dir] = cur_rating;
        if( cur_rating == best_rating ) {
            add_msg_debug( debugmode::DF_NPC_MOVEAI,
                           "<color_light_gray>%s thinks </color><color_light_blue>%s</color><color_light_gray> is the best retreat spot they've seen so far</color><color_light_blue> rated %1.2f</color><color_light_gray>after checking %i candidates</color>",
                           name, pt.to_string_writable(), best_rating, num_points_searched );
            candidates.emplace_back( pos_bub() + displace_XY( pt_dir ) );
        } else if( cur_rating < best_rating ) {
            if( one_in( 5 ) ) {
                add_msg_debug( debugmode::DF_NPC_MOVEAI,
                               "<color_dark_gray>%s just wants to let you know, esteemed debugger, that they're still checking escape points but haven't found any new good ones.  Checked %i.</color>",
                               name, num_points_searched );
            }
            candidates.clear();
            candidates.emplace_back( pos_bub() + displace_XY( pt_dir ) );
            best_rating = cur_rating;
        }
        num_points_searched += 1;
    }
    ( void )num_points_searched;
    tripoint_bub_ms redirect_goal = random_entry( candidates );
    add_msg_debug( debugmode::DF_NPC_MOVEAI, "%s is repositioning to %s", name,
                   redirect_goal.to_string_writable() );
    return redirect_goal;
}

bool npc::sees_dangerous_field( const tripoint_bub_ms &p ) const
{
    return is_dangerous_fields( get_map().field_at( p ) );
}

bool npc::could_move_onto( const tripoint_bub_ms &p ) const
{
    map &here = get_map();
    if( !here.passable_through( p ) ) {
        return false;
    }

    if( !sees_dangerous_field( p ) ) {
        return true;
    }

    const field fields_here = here.field_at( pos_bub() );
    for( const auto &e : here.field_at( p ) ) {
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
    map &here = get_map();

    std::vector<sphere> result;

    const auto active_items = here.get_active_items_in_radius( pos_bub(), MAX_VIEW_DISTANCE,
                              special_item_type::explosive );

    for( const item_location &elem : active_items ) {
        const use_function *use = elem->type->get_use( "explosion" );

        if( !use ) {
            continue;
        }

        const explosion_iuse *actor = dynamic_cast<const explosion_iuse *>( use->get_actor_ptr() );
        const int safe_range = actor->explosion.safe_range();

        if( rl_dist( pos_abs(), elem.pos_abs() ) >= safe_range ) {
            continue;   // Far enough.
        }

        const int turns_to_evacuate = 2 * safe_range / speed_rating();

        if( elem->charges > turns_to_evacuate ) {
            continue;   // Consider only imminent dangers.
        }

        result.emplace_back( elem.pos_bub( here ).raw(), safe_range );
    }

    return result;
}

float npc::evaluate_monster( const monster &target, int dist ) const
{
    float speed = target.speed_rating();
    float scaled_distance = std::max( 1.0f, dist * dist / ( speed * 250.0f ) );
    float hp_percent = static_cast<float>( target.get_hp() ) / target.get_hp_max();
    float diff = std::max( static_cast<float>( target.type->difficulty ), NPC_DANGER_VERY_LOW );
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "<color_yellow>evaluate_monster </color><color_dark_gray>%s thinks %s threat level is <color_light_gray>%1.2f</color><color_dark_gray> before considering situation.  Speed rating: %1.2f; dist: %i; scaled_distance: %1.0f; HP: %1.0f%%</color>",
                   name, target.type->nname(), diff, speed, dist, scaled_distance, hp_percent * 100 );
    // Note that the danger can pass below "very low" if the monster is weak and far away.
    diff *= ( hp_percent * 0.5f + 0.5f ) / scaled_distance;
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "<color_light_gray>%s puts final %s threat level at </color>%1.2f<color_light_gray> after counting speed, distance, hp</color>",
                   name, target.type->nname(), diff );
    return std::min( diff, NPC_MONSTER_DANGER_MAX );
}

float npc::evaluate_character( const Character &candidate, bool my_gun, bool enemy = true )
{
    float threat = 0.0f;
    bool candidate_gun = candidate.get_wielded_item() && candidate.get_wielded_item()->is_gun();
    const item &candidate_weap = candidate.get_wielded_item() ? *candidate.get_wielded_item() :
                                 null_item_reference();
    double candidate_weap_val = candidate.weapon_value( candidate_weap );
    float candidate_health =  candidate.hp_percentage() / 100.0f;
    float armour = estimate_armour( candidate );
    float speed = std::max( 0.25f, candidate.get_speed() / 100.0f );
    bool is_fleeing = candidate.has_effect( effect_npc_run_away );
    int perception_inverted = std::max( ( 20 - get_per() ), 0 );
    if( has_effect( effect_bleed ) ) {
        int bleed_intensity = 0;
        for( const bodypart_id &bp : candidate.get_all_body_parts() ) {
            const effect &bleediness = candidate.get_effect( effect_bleed, bp );
            if( !bleediness.is_null() && bleediness.get_intensity() > perception_inverted / 2 ) {
                // unlike in evaluate self, NPCs can't notice bleeding in others unless it's pretty high.
                bleed_intensity += bleediness.get_intensity();
            }
        }
        candidate_health *= std::max( 1.0f - bleed_intensity / 10.0f, 0.25f );
        add_msg_debug( debugmode::DF_NPC_COMBATAI,
                       "<color_red>%s is bleeeeeeding…</color>, intensity %i", candidate.disp_name(), bleed_intensity );
    }
    if( !enemy ) {
        if( candidate_gun || ( is_player_ally() && candidate.is_avatar() ) ) {
            // later we should evaluate if the NPC trusts the player enough to stick to them so reliably
            int dist = rl_dist( pos_bub(), candidate.pos_bub() );
            if( dist > mem_combat.formation_distance ) {
                mem_combat.formation_distance = std::max( dist, mem_combat.engagement_distance );
            }
        }
    }

    if( !my_gun ) {
        speed = std::max( speed, 0.5f );
    };

    threat += my_gun && enemy ? candidate.get_dodge() / 2.0f : candidate.get_dodge();
    threat += armour;
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "<color_cyan>evaluate_character </color><color_light_gray>%s assesses %s defense value as %1.2f.</color>",
                   name, candidate.disp_name( true ), threat );

    if( enemy && candidate_gun && !my_gun ) {
        add_msg_debug( debugmode::DF_NPC_COMBATAI,
                       "<color_light_gray>%s has a gun and %s doesn't; %s adjusts threat accordingly.</color>",
                       candidate.disp_name(), name, name );
        candidate_weap_val *= 1.5f;
    }
    threat += candidate_weap_val;
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "<color_light_gray>%s assesses %s weapon value as %1.2f.</color>",
                   name, candidate.disp_name( true ), candidate_weap_val );
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "<color_light_gray>%s assesses</color> %s threat: %1.2f <color_light_gray>before personality and situation changes.</color>",
                   name,
                   candidate.disp_name( true ), threat );
    if( enemy ) {
        threat -= static_cast<float>( personality.aggression );
    } else {
        threat +=  static_cast<float>( personality.bravery );
    }

    threat *= speed;
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "<color_light_gray>%s scales %s threat by %1.0f%% based on speed.</color>",
                   name, candidate.disp_name( true ), speed * 100.0f );
    threat *= candidate_health;
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "<color_light_gray>%s scales %s threat by %1.0f%% based on remaining health.</color>", name,
                   candidate.disp_name( true ), candidate_health * 100.0f );

    if( is_fleeing ) {
        threat *= 0.5f;
        add_msg_debug( debugmode::DF_NPC_COMBATAI,
                       "<color_light_gray>%s scales %s threat by 50%% because they're running away.</color>", name,
                       candidate.disp_name( true ) );
    }
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "<color_light_gray>%s sets </color>%s threat: %1.2f <color_light_gray>before perception randomization.</color>",
                   name,
                   candidate.disp_name( true ), threat );
    // the math for perception fuzz is this way to make it more human readable because I kept making silly errors.
    // I hope this helps you too. If not, well, sorry bud.
    // Anyway the higher your perception gets the more accurate and predictable your rating is.
    // this will become more valuable the more skilled we make NPCs at assessing enemies.
    // At time of writing they're bad at it so this is mostly just me patting myself on the back for adding a cool looking feature.
    int perception_factor = rng( -10, 10 ) * perception_inverted;

    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "<color_light_gray>%s randomizes %s threat by %1.1f%% based on perception factor %i.</color>  Final threat %1.2f",
                   name, candidate.disp_name( true ), threat * perception_factor / 1000.0f, perception_factor,
                   threat +  threat * perception_factor / 1000.0f );
    threat += threat * perception_factor / 1000.0f;

    add_msg_debug( debugmode::DF_NPC, "<color_light_gray>%s assesses </color>%s final threat: %1.2f",
                   name,
                   candidate.disp_name( true ),
                   threat );
    return std::min( threat, NPC_CHARACTER_DANGER_MAX );
}

float npc::evaluate_self( bool my_gun )
{
    float threat = 0.0f;
    const double &my_weap_val = ai_cache.my_weapon_value;
    // the worse pain the NPC is in, the more likely they are to overestimate the severity of their injuries.
    // the more perceptive they are, the more they're able to see how bad it really is.
    // Randomize it such that it becomes more swingy as their emotions and pain grow higher.
    float pain_factor = rng( 0.0f,
                             static_cast<float>( get_pain() ) / static_cast<float>( get_per() ) );
    mem_combat.my_health = ( hp_percentage() - pain_factor ) / 100.0f;
    float armour = estimate_armour( dynamic_cast<const Character &>( *this ) );
    float speed = std::max( 0.5f, get_speed() / 100.0f );
    if( my_gun ) {
        speed = std::max( speed, 0.75f );
    }
    if( has_effect( effect_bleed ) ) {
        int bleed_intensity = 0;
        for( const bodypart_id &bp : get_all_body_parts() ) {
            const effect &bleediness = get_effect( effect_bleed, bp );
            if( !bleediness.is_null() ) {
                bleed_intensity += bleediness.get_intensity();
            }
        }
        mem_combat.my_health *= std::max( 1.0f - bleed_intensity / 10.0f, 0.25f );
        add_msg_debug( debugmode::DF_NPC_COMBATAI,
                       "<color_red>%s is bleeeeeeding…</color>, intensity %i", name, bleed_intensity );
        if( mem_combat.my_health < 0.25f ) {
            mem_combat.panic += 1;
        }
    }


    threat += get_dodge();
    threat += armour;
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "<color_light_green>evaluate_self </color><color_light_gray>%s assesses own defense value as %1.2f.</color>",
                   name, threat );

    threat += my_weap_val;
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "<color_light_gray>%s assesses own weapon value as %1.2f.",
                   name, my_weap_val );

    threat += static_cast<float>( personality.bravery + personality.aggression );
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "<color_light_gray>%s updates own threat by %i based on personality.",
                   name, personality.bravery + personality.aggression );

    threat *= speed;
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "<color_light_gray>%s scales own threat by %1.0f%% based on speed.",
                   name, speed * 100.0f );
    threat *= mem_combat.my_health;
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "<color_light_gray>%s scales own threat by %1.0f%% based on remaining health (reduced by %1.2f%% due to pain).</color>  Final value: %1.2f",
                   name,
                   mem_combat.my_health * 100.0f, pain_factor, threat );
    add_msg_debug( debugmode::DF_NPC, "%s assesses own threat as %1.2f", name, threat );
    return std::min( threat, NPC_CHARACTER_DANGER_MAX );
}

float npc::estimate_armour( const Character &candidate ) const
{
    float armour = 0.0f;
    int armour_step;
    int number_of_parts = 0;

    for( bodypart_id part_id : candidate.get_all_body_parts( get_body_part_flags::only_main ) ) {
        armour_step = 0;
        number_of_parts += 1;
        armour_step += candidate.get_armor_type( damage_bash, part_id );
        armour_step += candidate.get_armor_type( damage_cut, part_id );
        armour_step += candidate.get_armor_type( damage_stab, part_id );
        armour_step += candidate.get_armor_type( damage_bullet, part_id );
        add_msg_debug( debugmode::DF_NPC_ITEMAI,
                       "<color_light_gray>%s: %s armour value for %s rated as %i.</color>", name,
                       candidate.disp_name( true ), body_part_name( part_id ), armour_step );
        if( part_id == bodypart_id( "head" ) || part_id == bodypart_id( "torso" ) ) {
            armour_step *= 4;
            number_of_parts += 3;
        }
        // obtain an average value of the 4 armour types we checked.
        armour += static_cast<float>( armour_step ) / 4.0f;
    }
    armour /= number_of_parts;

    add_msg_debug( debugmode::DF_NPC_ITEMAI,
                   "<color_light_gray>%s rates </color>%s total armour value: %1.2f.", name,
                   candidate.disp_name( true ), armour );
    // this is a value we could easily cache.
    // I don't know how to do that, I'm supposed to be a writer.
    return armour;
}

static bool too_close( const tripoint_bub_ms &critter_pos, const tripoint_bub_ms &ally_pos,
                       const int def_radius )
{
    return rl_dist( critter_pos, ally_pos ) <= def_radius;
}

std::optional<int> npc_short_term_cache::closest_enemy_to_friendly_distance() const
{
    int distance = INT_MAX;
    for( const weak_ptr_fast<Creature> &buddy : friends ) {
        if( buddy.expired() ) {
            continue;
        }
        for( const weak_ptr_fast<Creature> &enemy : hostile_guys ) {
            if( enemy.expired() ) {
                continue;
            }
            distance = std::min( distance, rl_dist( buddy.lock()->pos_bub(), enemy.lock()->pos_bub() ) );
        }
    }
    if( distance == INT_MAX ) {
        return std::nullopt;
    }
    return distance;
}

void npc::assess_danger()
{
    const map &here = get_map();

    float highest_priority = 1.0f;
    int hostile_count = 0; // for tallying nearby threatening enemies
    int friendly_count = 1; // count yourself as a friendly
    int def_radius = rules.has_flag( ally_rule::follow_close ) ? follow_distance() : 6;
    bool npc_ranged = get_wielded_item() && get_wielded_item()->is_gun();

    if( !confident_range_cache ) {
        invalidate_range_cache();
    }
    // Radius we can attack without moving
    int max_range = *confident_range_cache;
    // Radius in which enemy threats are multiplied to avoid surrounding
    int preferred_medium_range = std::max( max_range, 8 );
    preferred_medium_range = std::min( preferred_medium_range, 15 );
    // Radius in which enemy threats are hugely multiplied to encourage repositioning
    int preferred_close_range = std::max( max_range, 1 );
    preferred_close_range = std::min( preferred_close_range, preferred_medium_range / 2 );

    Character &player_character = get_player_character();
    bool sees_player = sees( here, player_character.pos_bub( here ) );
    const bool self_defense_only = rules.engagement == combat_engagement::NO_MOVE ||
                                   rules.engagement == combat_engagement::NONE;
    const bool no_fighting = rules.has_flag( ally_rule::forbid_engage );
    const bool must_retreat = rules.has_flag( ally_rule::follow_close ) &&
                              !too_close( pos_bub(), player_character.pos_bub(), follow_distance() ) &&
                              !is_guarding();

    if( is_player_ally() ) {
        if( rules.engagement == combat_engagement::FREE_FIRE ) {
            def_radius = std::max( 6, max_range );
        } else if( self_defense_only ) {
            def_radius = max_range;
        } else if( no_fighting ) {
            def_radius = 1;
        }
    }
    mem_combat.engagement_distance = def_radius;

    const auto ok_by_rules = [max_range, def_radius, this, &player_character]( const Creature & c,
    int dist, int scaled_dist ) {
        // If we're forbidden to attack, no need to check engagement rules
        if( rules.has_flag( ally_rule::forbid_engage ) ) {
            return false;
        }
        switch( rules.engagement ) {
            case combat_engagement::NONE:
                return false;
            case combat_engagement::CLOSE:
                // Either close to player or close enough that we can reach it and close to us
                return ( dist <= max_range && scaled_dist <= def_radius * 0.5 ) ||
                       too_close( c.pos_bub(), player_character.pos_bub(), def_radius );
            case combat_engagement::WEAK:
                return c.get_hp() <= average_damage_dealt();
            case combat_engagement::HIT:
                return c.has_effect( effect_hit_by_player );
            case combat_engagement::NO_MOVE:
            case combat_engagement::FREE_FIRE:
                return dist <= max_range;
            case combat_engagement::ALL:
                return true;
        }

        return true;
    };
    std::map<direction, float> cur_threat_map;
    // start with a decayed version of last turn's map
    for( direction threat_dir : npc_threat_dir ) {
        cur_threat_map[ threat_dir ] = 0.25f * ai_cache.threat_map[ threat_dir ];
    }
    // cache string_id -> int_id conversion before hot loop
    const field_type_id fd_fire = ::fd_fire;
    // first, check if we're about to be consumed by fire
    // `map::get_field` uses `field_cache`, so in general case (no fire) it provides an early exit
    for( const tripoint_bub_ms &pt : here.points_in_radius( pos_bub(), 6 ) ) {
        if( pt == pos_bub() || !here.get_field( pt, fd_fire ) ||
            here.has_flag( ter_furn_flag::TFLAG_FIRE_CONTAINER,  pt ) ) {
            continue;
        }
        int dist = rl_dist( pos_bub(), pt );
        cur_threat_map[direction_from( pos_bub(), pt )] += 2.0f * ( NPC_MONSTER_DANGER_MAX - dist );
        if( dist < 3 && !has_effect( effect_npc_fire_bad ) ) {
            warn_about( "fire_bad", 1_minutes );
            add_effect( effect_npc_fire_bad, 5_turns );
            path.clear();
        }
    }

    // find our Character friends and enemies
    const bool clairvoyant = clairvoyance();
    for( const npc &guy : g->all_npcs() ) {
        if( &guy == this ) {
            continue;
        }
        if( !clairvoyant && !here.has_potential_los( pos_bub(), guy.pos_bub() ) ) {
            continue;
        }

        if( has_faction_relationship( guy, npc_factions::relationship::watch_your_back ) ) {
            ai_cache.friends.emplace_back( g->shared_from( guy ) );
        } else if( attitude_to( guy ) != Attitude::NEUTRAL && sees( here, guy.pos_bub( here ) ) ) {
            ai_cache.hostile_guys.emplace_back( g->shared_from( guy ) );
        }
    }
    if( is_friendly( player_character ) && sees_player ) {
        ai_cache.friends.emplace_back( g->shared_from( player_character ) );
    } else if( sees_player && is_enemy() && sees( here, player_character ) ) {
        // Unlike allies, hostile npcs should not see invisible players
        ai_cache.hostile_guys.emplace_back( g->shared_from( player_character ) );
    }

    for( const monster &critter : g->all_monsters() ) {
        if( !clairvoyant && !here.has_potential_los( pos_bub(), critter.pos_bub() ) ) {
            continue;
        }
        Creature::Attitude att = critter.attitude_to( *this );
        if( att == Attitude::FRIENDLY ) {
            ai_cache.friends.emplace_back( g->shared_from( critter ) );
            friendly_count += 1;
            continue;
        }
        if( att != Attitude::HOSTILE && ( critter.friendly || !is_enemy() ) ) {
            ai_cache.neutral_guys.emplace_back( g->shared_from( critter ) );
            continue;
        }
        if( !sees( here, critter ) ) {
            continue;
        }

        ai_cache.hostile_guys.emplace_back( g->shared_from( critter ) );
        // warn and consider the odds for distant enemies
        int dist = rl_dist( pos_bub(), critter.pos_bub() );
        float critter_threat = evaluate_monster( critter, dist );

        // ignore targets behind glass even if we can see them
        if( !clear_shot_reach( pos_bub(), critter.pos_bub(), false ) ) {
            if( is_enemy() || !critter.friendly ) {
                // still warn about enemies behind impassable glass walls, but not as often.
                add_msg_debug( debugmode::DF_NPC_COMBATAI,
                               "%s ignored %s because there's an obstacle in between.  Might warn about it.",
                               name, critter.type->nname() );
                if( critter_threat > 2 * ( 8.0f + personality.bravery + rng( 0, 5 ) ) ) {
                    warn_about( "monster", 10_minutes, critter.type->nname(), dist, critter.pos_bub() );
                }
            } else {
                add_msg_debug( debugmode::DF_NPC_COMBATAI,
                               "%s ignored %s because there's an obstacle in between, and it's not worth warning about.",
                               name, critter.type->nname() );
            }
            continue;
        }

        if( is_enemy() || !critter.friendly ) {
            mem_combat.assess_enemy += critter_threat;
            if( critter_threat > ( 8.0f + personality.bravery + rng( 0, 5 ) ) ) {
                warn_about( "monster", 10_minutes, critter.type->nname(), dist, critter.pos_bub() );
            }
            if( dist < preferred_medium_range ) {
                hostile_count += 1;
                add_msg_debug( debugmode::DF_NPC_COMBATAI,
                               "<color_light_gray>%s added %s to nearby hostile count.  Total: %i</color>", name,
                               critter.type->nname(), hostile_count );
            }
            if( dist <= preferred_close_range ) {
                mem_combat.swarm_count += 1;
                add_msg_debug( debugmode::DF_NPC_COMBATAI,
                               "<color_light_gray>%s added %s to swarm count.  Total: %i</color>",
                               name, critter.type->nname(), mem_combat.swarm_count );
            }
        }
        if( must_retreat || no_fighting ) {
            continue;
        }

        add_msg_debug( debugmode::DF_NPC,
                       "%s assessed threat of critter %s as %1.2f.",
                       name, critter.type->nname(), critter_threat );
        ai_cache.total_danger += critter_threat;
        float scaled_distance = std::max( 1.0f, dist / critter.speed_rating() );

        // don't ignore monsters that are too close or too close to an ally if we can move
        bool is_too_close = dist <= def_radius;
        for( const weak_ptr_fast<Creature> &guy : ai_cache.friends ) {
            if( is_too_close || self_defense_only ) {
                break;
            }
            // HACK: Bit of a dirty hack - sometimes shared_from, returns nullptr or bad weak_ptr for
            // friendly NPC when the NPC is riding a creature - I don't know why.
            // so this skips the bad weak_ptrs, but this doesn't functionally change the AI Priority
            // because the horse the NPC is riding is still in the ai_cache.friends vector,
            // so either one would count as a friendly for this purpose.
            if( guy.lock() ) {
                is_too_close |= too_close( critter.pos_bub(), guy.lock()->pos_bub(), def_radius );
            }
        }
        // ignore distant monsters that our rules prevent us from attacking
        if( !is_too_close && is_player_ally() && !ok_by_rules( critter, dist, scaled_distance ) ) {
            continue;
        }
        // prioritize the biggest, nearest threats, or the biggest threats that are threatening
        // us or an ally
        float priority = std::max( critter_threat - 2.0f * ( scaled_distance - 1.0f ),
                                   is_too_close ? critter_threat : 0.0f );
        cur_threat_map[direction_from( pos_bub(), critter.pos_bub() )] += priority;
        if( priority > highest_priority ) {
            highest_priority = priority;
            ai_cache.target = g->shared_from( critter );
            ai_cache.danger = critter_threat;
        }
    }

    if( mem_combat.assess_enemy == 0.0 && ai_cache.hostile_guys.empty() ) {
        if( mem_combat.panic > 0 ) {
            mem_combat.panic -= 1;
        }
        ai_cache.danger_assessment = mem_combat.assess_enemy;
        return;
    }

    // Warn about sufficiently risky nearby hostiles
    const auto handle_hostile = [&]( const Character & foe, float foe_threat,
    const std::string & bogey, const std::string & warning ) {
        int dist = rl_dist( pos_bub(), foe.pos_bub() );
        // ignore targets behind glass even if we can see them
        if( !clear_shot_reach( pos_bub(), foe.pos_bub(), false ) ) {
            // still warn about enemies behind impassable glass walls, but not as often.
            // since NPC threats have a higher chance of ignoring soft obstacles, we'll ignore them here.
            if( foe_threat > 2 * ( 8.0f + personality.bravery + rng( 0, 5 ) ) ) {
                warn_about( "monster", 10_minutes, bogey, dist, foe.pos_bub() );
            }
            return 0.0f;
        } else {
            add_msg_debug( debugmode::DF_NPC_COMBATAI, "%s ignored %s because there's an obstacle in between.",
                           name, bogey );
        }
        if( foe_threat > ( 8.0f + personality.bravery + rng( 0, 5 ) ) ) {
            warn_about( "monster", 10_minutes, bogey, dist, foe.pos_bub() );
        }

        int scaled_distance = std::max( 1, ( 100 * dist ) / foe.get_speed() );
        ai_cache.total_danger += foe_threat / scaled_distance;
        if( must_retreat || no_fighting ) {
            return 0.0f;
        }
        bool is_too_close = dist <= def_radius;
        for( const weak_ptr_fast<Creature> &guy : ai_cache.friends ) {
            if( self_defense_only ) {
                break;
            }
            is_too_close |= too_close( foe.pos_bub(), guy.lock()->pos_bub(), def_radius );
            if( is_too_close ) {
                break;
            }
        }

        if( !is_player_ally() || is_too_close || ok_by_rules( foe, dist, scaled_distance ) ) {
            float priority = std::max( foe_threat - 2.0f * ( scaled_distance - 1 ),
                                       is_too_close ? std::max( foe_threat, NPC_DANGER_VERY_LOW ) :
                                       0.0f );
            cur_threat_map[direction_from( pos_bub(), foe.pos_bub() )] += priority;
            if( priority > highest_priority ) {
                warn_about( warning, 1_minutes );
                highest_priority = priority;
                ai_cache.danger = foe_threat;
                ai_cache.target = g->shared_from( foe );
            }
        }
        add_msg_debug( debugmode::DF_NPC_COMBATAI,
                       "<color_light_gray>%s assessed threat of enemy %s as %1.2f.  With distance vs speed ratio %i, final relative threat is </color><color_red>%1.2f</color>",
                       name, bogey, foe_threat, scaled_distance, foe_threat / scaled_distance );
        return foe_threat;
    };


    for( const weak_ptr_fast<Creature> &guy : ai_cache.hostile_guys ) {
        Character *foe = dynamic_cast<Character *>( guy.lock().get() );
        if( foe && foe->is_npc() ) {
            mem_combat.assess_enemy += handle_hostile( *foe, evaluate_character( *foe, npc_ranged ),
                                       translate_marker( "bandit" ),
                                       "kill_npc" );
        }
    }
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "Before checking allies+player, %s assesses danger level as <color_light_red>%1.2f</color>.", name,
                   mem_combat.assess_enemy );
    for( const weak_ptr_fast<Creature> &guy : ai_cache.friends ) {
        if( !( guy.lock() && guy.lock()->is_npc() ) ) {
            continue;
        }
        float guy_threat = std::max( evaluate_character( dynamic_cast<const Character &>( *guy.lock() ),
                                     npc_ranged, false ), NPC_DANGER_VERY_LOW );
        mem_combat.assess_ally += guy_threat * 0.5f;
        add_msg_debug( debugmode::DF_NPC_COMBATAI,
                       "<color_light_gray>%s assessed friendly %s at threat level </color><color_light_blue>%1.2f.</color>",
                       name, guy.lock()->disp_name(), guy_threat );
    }
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "Total of <color_green>%s NPC ally threat</color>: <color_light_green>%1.2f</color>.",
                   name, mem_combat.assess_ally );

    if( sees_player ) {
        // Mod for the player's danger level, weight it higher if player is very close
        // When the player is almost adjacent, it can exceed max danger ratings, so the
        // NPC will try hard not to break and run while in formation.
        // This code should eventually remove the 'player' special case and be applied to
        // whoever the NPC perceives as their closest leader.
        float player_diff = std::max( evaluate_character( player_character, npc_ranged, is_enemy() ),
                                      NPC_DANGER_VERY_LOW );
        int dist = rl_dist( pos_bub(), player_character.pos_bub() );
        if( is_enemy() ) {
            add_msg_debug( debugmode::DF_NPC_COMBATAI,
                           "<color_light_gray>%s identified player as an</color> <color_red>enemy</color> <color_light_gray>of threat level %1.2f</color>",
                           name, player_diff );
            mem_combat.assess_enemy += handle_hostile( player_character, player_diff,
                                       translate_marker( "maniac" ),
                                       "kill_player" );
        } else if( is_friendly( player_character ) ) {
            add_msg_debug( debugmode::DF_NPC_COMBATAI,
                           "<color_light_gray>%s identified player as a </color><color_green>friend</color><color_light_gray> of threat level %1.2f (ily babe)",
                           name, player_diff );
            if( dist <= 3 ) {
                player_diff = player_diff * ( 4 - dist ) / 2;
                mem_combat.swarm_count /= ( 4 - dist );
                mem_combat.assess_ally += player_diff;
                add_msg_debug( debugmode::DF_NPC_COMBATAI,
                               "<color_green>Player is %i tiles from %s.</color><color_light_gray>  Adding </color><color_light_green>%1.2f to ally strength</color><color_light_gray> and bolstering morale.</color>",
                               dist, name,
                               player_diff );
                // don't try to fall back with your ranged weapon if you're in formation with the player.
                if( mem_combat.panic > 0 && one_in( dist ) ) {
                    mem_combat.panic -= 1;
                }
                friendly_count += 4 - dist; // when close to the player, weight enemy groups less.
            } else {
                add_msg_debug( debugmode::DF_NPC_COMBATAI,
                               "<color_light_gray>%s sees friendly player,</color> <color_light_green>adding %1.2f</color><color_light_gray> to ally strength.</color>",
                               name, player_diff * 0.5f );
                mem_combat.assess_ally += player_diff * 0.5f;
            }
            ai_cache.friends.emplace_back( g->shared_from( player_character ) );
        }
    }
    add_msg_debug( debugmode::DF_NPC_COMBATAI,
                   "<color_light_blue>After checking player</color><color_light_gray>, %s assesses enemy level as </color><color_yellow>%1.2f</color><color_light_gray>, ally level at </color><color_light_green>%1.2f</color>",
                   name, mem_combat.assess_enemy, mem_combat.assess_ally );


    // gotta rename cowardice modifier now.
    // This bit scales the assessments of enemies and allies so that the NPC weights their own skills a little higher.
    // It's likely to get deprecated in a while?
    mem_combat.assess_enemy *= NPC_COWARDICE_MODIFIER;
    //Figure our own health more heavily here, because it doens't matter how tough our friends are if we're dying.
    mem_combat.assess_ally *= mem_combat.my_health * NPC_COWARDICE_MODIFIER;

    // Swarm assessment.  Do a flat scale up your assessment if you're outnumbered.
    // Hostile_count counts enemies within a range of 8 who exceed the NPC's bravery, mitigated
    // how much pain they're currently experiencing. This means a very brave NPC might ignore
    // large crowds of minor creatures, until they start getting hurt.
    if( hostile_count > friendly_count ) {
        mem_combat.assess_enemy *= std::max( hostile_count / static_cast<float>( friendly_count ), 1.0f );
        add_msg_debug( debugmode::DF_NPC_COMBATAI,
                       "Crowd adjustment: <color_light_gray>%s set danger level to </color>%1.2f<color_light_gray> after counting </color><color_yellow>%i major hostiles</color><color_light_gray> vs </color><color_light_green>%i friendlies.</color>",
                       name, mem_combat.assess_enemy, hostile_count, friendly_count );
    }

    if( !has_effect( effect_npc_run_away ) && !has_effect( effect_npc_fire_bad ) ) {
        float my_diff = evaluate_self( npc_ranged ) * 0.5f;
        add_msg_debug( debugmode::DF_NPC_COMBATAI,
                       "%s assesses own final strength as %1.2f.", name, my_diff );
        mem_combat.assess_ally += my_diff;
        add_msg_debug( debugmode::DF_NPC_COMBATAI,
                       "%s rates total <color_yellow>enemy strength %1.2f</color>, <color_light_green>ally strength %1.2f</color>.",
                       name, mem_combat.assess_enemy, mem_combat.assess_ally );
        add_msg_debug( debugmode::DF_NPC, "Enemy Danger: %1f, Ally Strength: %2f.", mem_combat.assess_enemy,
                       mem_combat.assess_ally );
    }
    // update the threat cache
    for( size_t i = 0; i < 8; i++ ) {
        direction threat_dir = npc_threat_dir[i];
        direction dir_right = npc_threat_dir[( i + 1 ) % 8];
        direction dir_left = npc_threat_dir[( i + 7 ) % 8 ];
        ai_cache.threat_map[threat_dir] = cur_threat_map[threat_dir] + 0.1f *
                                          ( cur_threat_map[dir_right] + cur_threat_map[dir_left] );
    }
    if( mem_combat.assess_enemy <= 2.0f ) {
        ai_cache.danger_assessment = -10.0f + 5.0f *
                                     mem_combat.assess_enemy; // Low danger if no monsters around
    } else {
        ai_cache.danger_assessment = mem_combat.assess_enemy;
    }
}

void npc::act_on_danger_assessment()
{
    const map &here = get_map();

    bool npc_ranged = get_wielded_item() && get_wielded_item()->is_gun();
    bool failed_reposition = false;
    Character &player_character = get_player_character();
    if( has_effect( effect_npc_run_away ) ) {
        // this check runs each turn that the NPC is repositioning and assesses if the situation is getting any better.
        // The longer they try to move without improving, the more likely they become to stop and stand their ground.
        const bool melee_reposition_fail = !npc_ranged &&
                                           mem_combat.assessment_before_repos + rng( 0, 5 ) <= mem_combat.assess_enemy;
        const bool range_reposition_fail = npc_ranged &&
                                           mem_combat.assessment_before_repos * mem_combat.swarm_count + rng( 0,
                                                   5 ) <= mem_combat.assess_enemy * mem_combat.swarm_count;
        if( melee_reposition_fail || range_reposition_fail ) {
            add_msg_debug( debugmode::DF_NPC_COMBATAI,
                           "<color_light_red>%s tried to reposition last turn, and the situation has not improved.</color>",
                           name );
            failed_reposition = true;
            mem_combat.failing_to_reposition += 1;
        } else {
            add_msg_debug( debugmode::DF_NPC_COMBATAI,
                           "<color_light_green>%s tried to reposition last turn, and it worked out!</color>",
                           name );
            mem_combat.failing_to_reposition = 0;
        }
    }
    if( !has_effect( effect_npc_run_away ) && !has_effect( effect_npc_fire_bad ) ) {
        mem_combat.assessment_before_repos = std::round( mem_combat.assess_enemy );
        if( mem_combat.assess_ally < mem_combat.assess_enemy ) {
            // Each time NPC decides to run away, their panic increases, which increases likelihood
            // and duration of running away.
            // if they run to a more advantageous position, they'll reassess and rally.
            time_duration run_away_for = std::max( 2_turns + 1_turns * mem_combat.panic, 20_turns );

            if( mem_combat.reposition_countdown <= 0 ) {
                add_msg_debug( debugmode::DF_NPC_COMBATAI,
                               "%s decides to reposition.  Has not yet decided to flee.", name );
                mem_combat.repositioning = true;
                add_effect( effect_npc_run_away, run_away_for );
                path.clear();
            } else {
                add_msg_debug( debugmode::DF_NPC_COMBATAI, "%s still wants to reposition, but they just tried.",
                               name );
            }
            mem_combat.panic *= ( mem_combat.assess_enemy / ( mem_combat.assess_ally + 0.5f ) );
            mem_combat.panic += std::min(
                                    rng( 1, 3 ) + ( get_pain() / 5 ) - personality.bravery, 1 );

            if( mem_combat.panic - personality.bravery >= mem_combat.failing_to_reposition ) {
                // NPC hasn't yet failed to get away
                add_msg_debug( debugmode::DF_NPC_COMBATAI, "%s upgrades reposition to flat out retreat.", name );
                mem_combat.repositioning = false; // we're not just moving, we're running.
                warn_about( "run_away", run_away_for );
                if( !is_player_ally() ) {
                    set_attitude( NPCATT_FLEE_TEMP );
                }
                if( mem_combat.panic > 5 && is_player_ally() && sees( here, player_character.pos_bub( here ) ) ) {
                    // consider warning player about panic
                    int panic_alert = rl_dist( pos_bub(), player_character.pos_bub() ) - player_character.get_per();
                    if( mem_combat.panic - personality.bravery > panic_alert ) {
                        if( one_in( 4 ) && mem_combat.panic < 10 + personality.bravery ) {
                            add_msg( m_bad, _( "%s is starting to panic a bit." ), name );
                        } else if( mem_combat.panic >= 10 + personality.bravery ) {
                            add_msg( m_bad, _( "%s is panicking!" ), name );
                        }
                    }
                }
            }
        } else if( failed_reposition ||
                   ( mem_combat.assess_ally < mem_combat.assess_enemy * mem_combat.swarm_count ) ) {
            add_msg_debug( debugmode::DF_NPC_COMBATAI,
                           "<color_light_gray>%s considers </color>repositioning<color_light_gray> from swarming enemies.</color>",
                           name );
            if( failed_reposition ) {
                add_msg_debug( debugmode::DF_NPC_COMBATAI, "%s failed repositioning, trying again.", name );
                mem_combat.failing_to_reposition++;
            } else {
                add_msg_debug( debugmode::DF_NPC_COMBATAI,
                               "%s decided to reposition/kite due to %i nearby enemies.",
                               name, mem_combat.swarm_count );
            }
            mem_combat.repositioning = true;
            // you chose not to run, but there are enough of them coming in that you should probably back away a bit.

            time_duration run_away_for = 2_turns;
            add_effect( effect_npc_run_away, run_away_for );
            path.clear();
        } else {
            // Things seem to be going okay, reset/reduce "worry" memories.
            if( mem_combat.panic > 0 ) {
                mem_combat.panic -= 1;
            }
            mem_combat.failing_to_reposition = 0;
        }
    }
    mem_combat.panic = std::max( 0, mem_combat.panic );
    add_msg_debug( debugmode::DF_NPC_COMBATAI, "%s <color_magenta>panic level</color> is up to %i.",
                   name, mem_combat.panic );
    if( mem_combat.failing_to_reposition > 2 && has_effect( effect_npc_run_away ) &&
        !has_effect( effect_npc_fire_bad ) ) {
        // NPC is fleeing, but hasn't been able to reposition safely.
        // Consider cancelling fleeing.
        // Note that panic will still increment prior to this, and a truly panicked NPC will not stand and fight for any reason.
        if( mem_combat.panic - personality.bravery < mem_combat.failing_to_reposition ) {
            add_msg_debug( debugmode::DF_NPC_COMBATAI, "%s decided running away was futile.", name );
            mem_combat.reposition_countdown = 4;
            remove_effect( effect_npc_run_away );
            path.clear();
        }
    }
}

bool npc::is_safe() const
{
    return ai_cache.total_danger <= 0;
}

void npc::regen_ai_cache()
{
    map &here = get_map();
    auto i = std::begin( ai_cache.sound_alerts );
    creature_tracker &creatures = get_creature_tracker();
    if( has_trait( trait_RETURN_TO_START_POS ) ) {
        if( !ai_cache.guard_pos ) {
            ai_cache.guard_pos = pos_abs();
        }
    }
    while( i != std::end( ai_cache.sound_alerts ) ) {
        if( sees( here,  here.get_bub( tripoint_abs_ms( i->abs_pos ) ) ) ) {
            // if they were responding to a call for guards because of thievery
            npc *const sound_source = creatures.creature_at<npc>( here.get_bub( tripoint_abs_ms(
                                          i->abs_pos ) ) );
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
    ai_cache.hostile_guys.clear();
    ai_cache.neutral_guys.clear();
    ai_cache.target = shared_ptr_fast<Creature>();
    ai_cache.ally = shared_ptr_fast<Creature>();
    ai_cache.can_heal.clear_all();
    ai_cache.danger = 0.0f;
    ai_cache.total_danger = 0.0f;
    item &weapon = get_wielded_item() ? *get_wielded_item() : null_item_reference();
    ai_cache.my_weapon_value = weapon_value( weapon );
    ai_cache.dangerous_explosives = find_dangerous_explosives();
    mem_combat.formation_distance = -1;

    mem_combat.assess_enemy = 0.0f;
    mem_combat.assess_ally = 0.0f;
    mem_combat.swarm_count = 0;
    if( mem_combat.reposition_countdown > 0 ) {
        mem_combat.reposition_countdown --;
    }

    if( mem_combat.repositioning && !has_effect( effect_npc_run_away ) &&
        !has_effect( effect_npc_fire_bad ) ) {
        // if NPC no longer has the run away effect and isn't fleeing in panic,
        // they can stop moving away.
        mem_combat.repositioning = false;
        mem_combat.reposition_countdown = 1;
        path.clear();
    }

    assess_danger();
    if( old_assessment > NPC_DANGER_VERY_LOW && ai_cache.danger_assessment <= 0 ) {
        warn_about( "relax", 30_minutes );
    } else if( old_assessment <= 0.0f && ai_cache.danger_assessment > NPC_DANGER_VERY_LOW ) {
        warn_about( "general_danger" );
    }
    // Non-allied NPCs with a completed mission should move to the player
    if( !is_player_ally() && !is_stationary( true ) ) {
        Character &player_character = get_player_character();
        for( ::mission *miss : chatbin.missions_assigned ) {
            if( miss->is_complete( getID() ) ) {
                // unless the player found an item and already told the NPC he wanted to keep it
                const mission_goal &mgoal = miss->get_type().goal;
                if( ( mgoal == MGOAL_FIND_ITEM || mgoal == MGOAL_FIND_ANY_ITEM ||
                      mgoal == MGOAL_FIND_ITEM_GROUP ) &&
                    has_effect( effect_npc_player_still_looking ) ) {
                    continue;
                }
                if( pos_abs_omt() != player_character.pos_abs_omt() ) {
                    goal = player_character.pos_abs_omt();
                }
                set_attitude( NPCATT_TALK );
                break;
            }
        }
    }
}

void npc::move()
{
    const map &here = get_map();

    // don't just return from this function without doing something
    // that will eventually subtract moves, or change the NPC to a different type of action.
    // because this will result in an infinite loop
    if( attitude == NPCATT_FLEE ) {
        set_attitude( NPCATT_FLEE_TEMP );  // Only run for so many hours
    } else if( attitude == NPCATT_FLEE_TEMP && !has_effect( effect_npc_flee_player ) ) {
        set_attitude( NPCATT_NULL );
    }
    regen_ai_cache();
    // NPCs under operation should just stay still
    if( activity.id() == ACT_OPERATION || activity.id() == ACT_SPELLCASTING ) {
        execute_action( npc_player_activity );
        return;
    }
    act_on_danger_assessment();
    npc_action action = npc_undecided;

    const item_location weapon = get_wielded_item();
    static const std::string no_target_str = "none";
    const Creature *target = current_target();
    const std::string &target_name = target != nullptr ? target->disp_name() : no_target_str;
    if( !confident_range_cache ) {
        invalidate_range_cache();
    }
    add_msg_debug( debugmode::DF_NPC, "NPC %s: target = %s, danger = %.1f, range = %d",
                   get_name(), target_name, ai_cache.danger, *confident_range_cache );

    Character &player_character = get_player_character();
    //faction opinion determines if it should consider you hostile
    if( !is_enemy() && guaranteed_hostile() && sees( here, player_character ) ) {
        if( is_player_ally() ) {
            mutiny();
        }
        add_msg_debug( debugmode::DF_NPC, "NPC %s turning hostile because is guaranteed_hostile()",
                       get_name() );
        if( op_of_u.fear > 10 + personality.aggression + personality.bravery ) {
            set_attitude( NPCATT_FLEE_TEMP );    // We don't want to take u on!
        } else {
            set_attitude( NPCATT_KILL );    // Yeah, we think we could take you!
        }
    }

    /* This bypasses the logic to determine the npc action, but this all needs to be rewritten
     * anyway.
     * NPC won't avoid dangerous terrain while accompanying the player inside a vehicle to keep
     * them from inadvertently getting themselves run over and/or cause vehicle related errors.
     * NPCs flee from uncontained fires within 3 tiles
     */
    if( !in_vehicle && ( sees_dangerous_field( pos_bub() ) || has_effect( effect_npc_fire_bad ) ) &&
        !has_flag( json_flag_CANNOT_MOVE ) ) {
        if( sees_dangerous_field( pos_bub() ) ) {
            path.clear();
        }
        const tripoint_bub_ms escape_dir = good_escape_direction( sees_dangerous_field( pos_bub() ) );
        if( escape_dir != pos_bub() ) {
            move_to( escape_dir );
            return;
        }
    }

    // TODO: Place player-aiding actions here, with a weight

    /* NPCs are fairly suicidal so at this point we will do a quick check to see if
     * something nasty is going to happen.
     */

    if( !ai_cache.dangerous_explosives.empty() && !has_flag( json_flag_CANNOT_MOVE ) ) {
        action = npc_escape_explosion;
    } else if( is_enemy() && vehicle_danger( avoidance_vehicles_radius ) >= 0 &&
               !has_flag( json_flag_CANNOT_MOVE ) ) {
        // TODO: Think about how this actually needs to work, for now assume flee from player
        ai_cache.target = g->shared_from( player_character );
        action = method_of_fleeing();
    } else if( ( target == &player_character && attitude == NPCATT_FLEE_TEMP ) ||
               has_effect( effect_npc_run_away ) ) {
        if( hp_percentage() > 30 && target && rl_dist( pos_bub(), target->pos_bub() ) <= 1 &&
            !has_flag( json_flag_CANNOT_ATTACK ) ) {
            action = method_of_attack();
        } else if( !has_flag( json_flag_CANNOT_MOVE ) ) {
            action = method_of_fleeing();
        }
    } else if( has_effect( effect_asthma ) && ( has_charges( itype_inhaler, 1 ) ||
               has_charges( itype_oxygen_tank, 1 ) ||
               has_charges( itype_smoxygen_tank, 1 ) ) ) {
        action = npc_heal;
    } else if( target != nullptr && ai_cache.danger > 0 && !has_flag( json_flag_CANNOT_ATTACK ) ) {
        action = method_of_attack();
    } else if( !ai_cache.sound_alerts.empty() && !is_walking_with() &&
               !has_flag( json_flag_CANNOT_MOVE ) ) {
        tripoint_abs_ms cur_s_abs_pos = ai_cache.s_abs_pos;
        if( !ai_cache.guard_pos ) {
            ai_cache.guard_pos = pos_abs();
        }
        if( ai_cache.sound_alerts.size() > 1 ) {
            std::sort( ai_cache.sound_alerts.begin(), ai_cache.sound_alerts.end(),
                       compare_sound_alert );
            if( ai_cache.sound_alerts.size() > 10 ) {
                ai_cache.sound_alerts.resize( 10 );
            }
        }
        if( has_trait( trait_IGNORE_SOUND ) ) { //Do not investigate sounds - clear sound alerts as below
            ai_cache.sound_alerts.clear();
            action = npc_return_to_guard_pos;
        } else {
            action = npc_investigate_sound;
        }
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
            add_msg_debug( debugmode::DF_NPC, "NPC %s: investigating sound at x(%d) y(%d)", get_name(),
                           ai_cache.s_abs_pos.x(), ai_cache.s_abs_pos.y() );
        }
    } else {
        // No present danger
        cleanup_on_no_danger();

        action = address_needs();
        print_action( "address_needs %s", action );

        if( action == npc_undecided ) {
            action = address_player();
            print_action( "address_player %s", action );
        }
        if( action == npc_undecided && ai_cache.sound_alerts.empty() && ai_cache.guard_pos &&
            !has_flag( json_flag_CANNOT_MOVE ) ) {
            tripoint_abs_ms return_guard_pos = *ai_cache.guard_pos;
            add_msg_debug( debugmode::DF_NPC, "NPC %s: returning to guard spot at x(%d) y(%d)", get_name(),
                           return_guard_pos.x(), return_guard_pos.y() );
            action = npc_return_to_guard_pos;
        }
    }

    if( action == npc_undecided && is_walking_with() && goto_to_this_pos &&
        !has_flag( json_flag_CANNOT_MOVE ) ) {
        action = npc_goto_to_this_pos;
    }

    // check if in vehicle before doing any other follow activities
    if( action == npc_undecided && is_walking_with() && player_character.in_vehicle &&
        !in_vehicle ) {
        action = npc_follow_embarked;
        path.clear();
    }

    if( action == npc_undecided && is_walking_with() && rules.has_flag( ally_rule::follow_close ) &&
        rl_dist( pos_bub(), player_character.pos_bub() ) > follow_distance() &&
        !( player_character.in_vehicle &&
           in_vehicle ) && !has_flag( json_flag_CANNOT_MOVE ) ) {
        action = npc_follow_player;
    }

    if( action == npc_undecided && attitude == NPCATT_ACTIVITY && !has_flag( json_flag_CANNOT_MOVE ) ) {
        if( has_stashed_activity() ) {
            if( !check_outbounds_activity( get_stashed_activity(), true ) ) {
                assign_stashed_activity();
            } else {
                // wait a turn, because next turn, the object of our activity
                // may have been loaded in.
                set_moves( 0 );
            }
            return;
        }
        std::vector<tripoint_bub_ms> activity_route = get_auto_move_route();
        if( !activity_route.empty() && !has_destination_activity() ) {
            tripoint_bub_ms final_destination;
            if( destination_point ) {
                final_destination = here.get_bub( *destination_point );
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
        // an interrupted activity can cause this situation. stops allied NPCs zooming off
        // like random NPCs
        if( attitude == NPCATT_ACTIVITY && !activity ) {
            revert_after_activity();
            if( is_ally( player_character ) && !assigned_camp ) {
                attitude = NPCATT_FOLLOW;
                mission = NPC_MISSION_NULL;
            }
        }
        if( assigned_camp && attitude != NPCATT_ACTIVITY ) {
            if( has_job() && calendar::once_every( 10_minutes ) && find_job_to_perform() ) {
                action = npc_player_activity;
            } else {
                action = npc_worker_downtime;
                goal = pos_abs_omt();
            }
        }
        if( is_stationary( true ) && !assigned_camp && !has_flag( json_flag_CANNOT_MOVE ) ) {
            // if we're in a vehicle, stay in the vehicle
            if( in_vehicle ) {
                action = npc_pause;
                goal = pos_abs_omt();
            } else {
                action = goal == pos_abs_omt() ?  npc_pause : npc_goto_destination;
            }
        } else if( has_new_items && scan_new_items() ) {
            return;
        } else if( !fetching_item ) {
            find_item();
            print_action( "find_item %s", action );
        } else if( assigned_camp ) {
            // this should be covered above, but justincase to stop them zooming away.
            action = npc_pause;
        }

        // check if in vehicle before rushing off to fetch things
        if( is_walking_with() && player_character.in_vehicle && !has_flag( json_flag_CANNOT_MOVE ) ) {
            action = npc_follow_embarked;
            path.clear();
        } else if( fetching_item ) {
            // Set to true if find_item() found something
            action = npc_pickup;
        } else if( is_following() && !has_flag( json_flag_CANNOT_MOVE ) ) {
            // No items, so follow the player?
            action = npc_follow_player;
        }
        // Friendly NPCs who are followers/ doing tasks for the player should never get here.
        // This will revert them to a dynamic NPC state.
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
              ( rl_dist( pos_bub(), player_character.pos_bub() ) <= follow_distance() ||
                posz() != player_character.posz() ) )
        ) && !has_flag( json_flag_CANNOT_ATTACK ) ) {
        action = method_of_attack();
    }

    add_msg_debug( debugmode::DF_NPC, "%s chose action %s.", get_name(), npc_action_name( action ) );
    execute_action( action );
}

void npc::execute_action( npc_action action )
{
    int oldmoves = moves;
    tripoint_bub_ms tar = pos_bub();
    Creature *cur = current_target();
    if( action == npc_flee ) {
        tar = good_escape_direction( false );
    } else if( cur != nullptr ) {
        tar = cur->pos_bub();
    }
    /*
      debugmsg("%s ran execute_action() with target = %d! Action %s",
               name, target, npc_action_name(action));
    */

    Character &player_character = get_player_character();
    map &here = get_map();
    switch( action ) {
        case npc_do_attack:
            if( has_flag( json_flag_CANNOT_ATTACK ) ) {
                move_pause();
                break;
            }
            ai_cache.current_attack->use( *this, ai_cache.current_attack_evaluation.target() );
            ai_cache.current_attack.reset();
            ai_cache.current_attack_evaluation = npc_attack_rating{};
            break;
        case npc_pause:
            move_pause();
            break;
        case npc_worker_downtime:
            worker_downtime();
            break;
        case npc_reload: {
            if( !get_wielded_item() ) {
                debugmsg( "NPC tried to reload without weapon" );
            }
            do_reload( get_wielded_item() );
        }
        break;

        case npc_investigate_sound: {
            tripoint_bub_ms cur_pos = pos_bub();
            update_path( here.get_bub( tripoint_abs_ms( ai_cache.s_abs_pos ) ) );
            move_to_next();
            if( pos_bub() == cur_pos ) {
                ai_cache.stuck += 1;
            }
        }
        break;

        case npc_return_to_guard_pos: {
            const tripoint_bub_ms local_guard_pos = here.get_bub( *ai_cache.guard_pos );
            update_path( local_guard_pos );
            if( pos_bub() == local_guard_pos || path.empty() ) {
                move_pause();
                ai_cache.guard_pos = std::nullopt;
                path.clear();
            } else {
                move_to_next();
            }
        }
        break;

        case npc_sleep: {
            // TODO: Allow stims when not too tired
            // Find a nice spot to sleep
            tripoint_bub_ms best_spot = pos_bub();
            int best_sleepy = evaluate_sleep_spot( best_spot );
            for( const tripoint_bub_ms &p : closest_points_first( pos_bub(), MAX_VIEW_DISTANCE ) ) {
                if( !could_move_onto( p ) || !g->is_empty( p ) ) {
                    continue;
                }

                // For non-mutants, very_comfortable-1 is the expected value of an ideal normal bed.
                if( best_sleepy < comfort_data::COMFORT_VERY_COMFORTABLE - 1 ) {
                    const int sleepy = evaluate_sleep_spot( p );
                    if( sleepy > best_sleepy ) {
                        best_sleepy = sleepy;
                        best_spot = p;
                    }
                }
            }
            if( is_walking_with() ) {
                complain_about( "napping", 30_minutes, chat_snippets().snip_warn_sleep.translated() );
            }
            update_path( best_spot );
            // TODO: Handle empty path better
            if( best_spot == pos_bub() || path.empty() ) {
                move_pause();
                if( !has_effect( effect_lying_down ) ) {
                    activate_bionic_by_id( bio_soporific );
                    add_effect( effect_lying_down, 30_minutes, false, 1 );
                    if( !player_character.in_sleep_state() ) {
                        add_msg_if_player_sees( *this, _( "%s lies down to sleep." ), get_name() );
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
            /* NPCs can't choose this action anymore, but at least it works */
            drop_invalid_inventory();
            /* drop_items is still broken
             * drop_items( weight_carried() - weight_capacity(),
             *             volume_carried() - volume_capacity() );
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
            if( can_use_offensive_cbm() ) {
                activate_bionic_by_id( bio_hydraulics );
            }
            reach_attack( tar );
            break;
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
                look_for_player( player_character );
            }
            break;

        case npc_aim:
            aim( Target_attributes( pos_bub(), tar ) );
            break;

        case npc_shoot: {
            if( !get_wielded_item() ) {
                debugmsg( "NPC tried to shoot without weapon" );
            }
            gun_mode mode = get_wielded_item()->gun_current_mode();
            if( !mode ) {
                debugmsg( "NPC tried to shoot without valid mode" );
                break;
            }
            aim( Target_attributes( pos_bub(), tar ) );
            if( is_hallucination() ) {
                pretend_fire( this, mode.qty, *mode );
            } else {
                fire_gun( here, tar, mode.qty, *mode );
                // "discard" the fake bio weapon after shooting it
                if( is_using_bionic_weapon() ) {
                    discharge_cbm_weapon();
                }
            }
            break;
        }

        case npc_look_for_player:
            if( saw_player_recently() && last_player_seen_pos &&
                sees( here, here.get_bub( *last_player_seen_pos ) ) ) {
                update_path( here.get_bub( *last_player_seen_pos ) );
                move_to_next();
            } else {
                look_for_player( player_character );
            }
            break;

        case npc_heal_player: {
            Character *patient = dynamic_cast<Character *>( current_ally() );
            if( patient ) {
                update_path( patient->pos_bub() );
                if( path.size() == 1 ) { // We're adjacent to u, and thus can heal u
                    heal_player( *patient );
                } else if( !path.empty() ) {
                    std::string talktag = chat_snippets().snip_heal_player.translated();
                    parse_tags( talktag, get_player_character(), *this );
                    say( string_format( talktag, patient->disp_name() ) );
                    move_to_next();
                } else {
                    move_pause();
                }
            }
            break;
        }
        case npc_follow_player:
            update_path( player_character.pos_bub() );
            if( path.empty() ||
                ( static_cast<int>( path.size() ) <= follow_distance() &&
                  player_character.posz() == posz() ) ) {
                // We're close enough to u.
                move_pause();
            } else {
                move_to_next();
            }
            // TODO: Make it only happen when it's safe
            complain();
            break;

        case npc_follow_embarked: {
            const optional_vpart_position vp = here.veh_at( player_character.pos_bub() );
            if( !vp ) {
                debugmsg( "Following an embarked player with no vehicle at their location?" );
                // TODO: change to wait? - for now pause
                move_pause();
                break;
            }
            vehicle *const veh = &vp->vehicle();

            // Try to find the last destination
            // This is mount point, not actual position
            point_rel_ms last_dest( INT_MIN, INT_MIN );
            if( !path.empty() && veh_pointer_or_null( here.veh_at( path[path.size() - 1] ) ) == veh ) {
                last_dest = vp->mount_pos();
            }

            // Prioritize last found path, then seats
            // Don't change spots if ours is nice
            int my_spot = -1;
            std::vector<std::pair<int, int> > seats;
            for( const vpart_reference &vp : veh->get_avail_parts( VPFLAG_BOARDABLE ) ) {
                const Character *passenger = veh->get_passenger( vp.part_index() );
                if( passenger != this && passenger != nullptr ) {
                    continue;
                }
                // A seat is available if we can move there and it's either unassigned or assigned to us
                auto available_seat = [&]( const vehicle_part & pt ) {
                    tripoint_bub_ms target = veh->bub_part_pos( here, pt );
                    if( !pt.is_seat() ) {
                        return false;
                    }
                    if( !could_move_onto( target ) ) {
                        return false;
                    }
                    const npc *who = pt.crew();
                    return !who || who->getID() == getID();
                };

                const vehicle_part &pt = vp.part();

                int priority = 0;

                if( vp.mount_pos() == last_dest ) {
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

                seats.emplace_back( priority, static_cast<int>( vp.part_index() ) );
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

                tripoint_bub_ms pp = veh->bub_part_pos( here, cur_part );
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
            get_avatar().talk_to( get_talker_for( this ) );
            set_moves( 0 );
            break;

        case npc_mug_player:
            mug_player( player_character );
            break;

        case npc_goto_to_this_pos: {
            update_path( here.get_bub( *goto_to_this_pos ) );
            move_to_next();

            if( pos_abs() == *goto_to_this_pos ) {
                goto_to_this_pos = std::nullopt;
            }
            break;
        }

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
            add_msg_debug( debugmode::DF_NPC, "%s skips turn (noop)", disp_name() );
            return;

        default:
            debugmsg( "Unknown NPC action (%d)", action );
    }

    if( oldmoves == moves ) {
        add_msg_debug( debugmode::DF_NPC, "NPC didn't use its moves.  Action %s (%d).",
                       npc_action_name( action ), action );
    }
}

void npc::witness_thievery( item *it )
{
    known_stolen_item = it;
    // Shopkeep is behind glass
    if( myclass == NC_EVAC_SHOPKEEP ) {
        return;
    }
    set_attitude( NPCATT_RECOVER_GOODS );
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

    // if there's enough of a threat to be here, power up the combat CBMs and any combat items.
    prepare_for_combat();

    evaluate_best_attack( critter );

    std::optional<int> potential = ai_cache.current_attack_evaluation.value();
    if( potential && *potential > 0 ) {
        return npc_do_attack;
    } else {
        add_msg_debug( debugmode::debug_filter::DF_NPC, "%s can't figure out what to do", disp_name() );
        return npc_undecided;
    }
}

void npc::evaluate_best_attack( const Creature *target )
{
    // Required because evaluation includes electricity via linked cables.
    const map &here = get_map();

    std::shared_ptr<npc_attack> best_attack;
    npc_attack_rating best_evaluated_attack;
    const auto compare = [&best_attack, &best_evaluated_attack, this, &target]
    ( const std::shared_ptr<npc_attack> &potential_attack, const std::string & disp ) {
        const npc_attack_rating evaluated = potential_attack->evaluate( *this, target );
        if( evaluated.value() ) {
            add_msg_debug( debugmode::DF_NPC_ITEMAI, "%s: Item %s has effectiveness %d",
                           this->get_name(), disp, *evaluated.value() );
            if( evaluated > best_evaluated_attack ) {
                best_attack = potential_attack;
                best_evaluated_attack = evaluated;
            }
        }
    };

    // punching things is always available
    compare( std::make_shared<npc_attack_melee>( null_item_reference() ), "barehanded" );
    visit_items( [&compare, this, &here]( item * it, item * ) {
        if( can_wield( *it ).success() ) {
            // you can theoretically melee with anything.
            compare( std::make_shared<npc_attack_melee>( *it ), "(as MELEE) " + it->display_name() );
            if( !is_wielding( *it ) || !it->has_flag( flag_NO_UNWIELD ) ) {
                compare( std::make_shared<npc_attack_throw>( *it ), "(as THROWN) " + it->display_name() );
            }
            if( !it->type->use_methods.empty() ) {
                compare( std::make_shared<npc_attack_activate_item>( *it ),
                         "(as ACTIVATED) " + it->display_name() );
            }
            if( rules.has_flag( ally_rule::use_guns ) ) {
                for( const std::pair<const gun_mode_id, gun_mode> &mode : it->gun_all_modes() ) {
                    if( !( mode.second.melee() || mode.second.flags.count( "NPC_AVOID" ) ||
                           !can_use( *mode.second.target ) ||
                           ( rules.has_flag( ally_rule::use_silent ) && is_player_ally() &&
                             !mode.second->is_silent() ) ) ) {
                        if( it->shots_remaining( here, this ) > 0 || can_reload_current() ) {
                            compare( std::make_shared<npc_attack_gun>( *it, mode.second ), "(as FIRED) " + it->display_name() );
                        } else {
                            compare( std::make_shared<npc_attack_melee>( *it ), "(as MELEE) " + it->display_name() );
                        }
                    }
                }
            }
        }
        return VisitResponse::NEXT;
    } );
    if( magic->spells().empty() ) {
        magic->clear_opens_spellbook_data();
        get_event_bus().send<event_type::opens_spellbook>( getID() );
        magic->evaluate_opens_spellbook_data();
    }
    for( const spell_id &sp : magic->spells() ) {
        compare( std::make_shared<npc_attack_spell>( sp ), sp.c_str() );
    }

    ai_cache.current_attack = best_attack;
    ai_cache.current_attack_evaluation = best_evaluated_attack;
}

npc_action npc::address_needs()
{
    return address_needs( ai_cache.danger );
}

int npc::evaluate_sleep_spot( tripoint_bub_ms p )
{
    // Base evaluation is based on ability to actually fall sleep there
    int sleep_eval = get_comfort_at( p ).comfort;
    // Only evaluate further if the possible bed isn't already considered very comfortable.
    // This opt-out is necessary to allow mutant NPCs to find desired non-bed sleeping spaces
    if( sleep_eval < comfort_data::COMFORT_VERY_COMFORTABLE - 1 ) {
        const units::temperature_delta ideal_bed_value = 2_C_delta;
        const units::temperature_delta sleep_spot_value = floor_bedding_warmth( p );
        if( sleep_spot_value < ideal_bed_value ) {
            double bed_similarity = sleep_spot_value / ideal_bed_value;
            // bed_similarity^2, exponentially diminishing the value of non-bed sleeping spots the more not-bed-like they are
            sleep_eval *= pow( bed_similarity, 2 );
        }
    }
    return sleep_eval;
}

static bool wants_to_reload( const npc &guy, const item &candidate )
{
    if( !candidate.is_reloadable() ) {
        if( !candidate.is_magazine() || !candidate.is_gun() ) {
            add_msg_debug( debugmode::DF_NPC_ITEMAI, "%s considered reloading %s, but decided it was silly.",
                           guy.name, candidate.tname() );
        } else {
            add_msg_debug( debugmode::DF_NPC_ITEMAI,
                           "%s considered reloading %s, but feels it is inappropriate.", guy.name, candidate.tname() );
        }

        return false;
    }

    if( !guy.can_reload( candidate ) ) {
        add_msg_debug( debugmode::DF_NPC_ITEMAI, "%s doesn't think they can reload %s.", guy.name,
                       candidate.tname() );
        return false;
    }

    const int required = candidate.ammo_required();
    // TODO: Add bandolier check here, once they can be reloaded
    if( required < 1 && !candidate.is_magazine() ) {
        add_msg_debug( debugmode::DF_NPC_ITEMAI, "%s couldn't find requirements to reload %s.", guy.name,
                       candidate.tname() );
        return false;
    }
    add_msg_debug( debugmode::DF_NPC_ITEMAI, "%s might try reloading %s.", guy.name,
                   candidate.tname() );
    return true;
}

static bool wants_to_reload_with( const item &weap, const item &ammo )
{
    return !ammo.is_magazine() || ammo.ammo_remaining( ) > weap.ammo_remaining( );
}

// todo: make visit_items use item_locations and remove this
static item_location form_loc_recursive( Character *npc, item *node, item *parent )
{
    if( parent ) {
        return item_location( form_loc_recursive( npc, parent, npc->find_parent( *parent ) ), node );
    }

    return item_location( *npc, node );
}

item_location npc::find_reloadable()
{
    auto cached_value = cached_info.find( "reloadables" );
    if( cached_value != cached_info.end() ) {
        return item_location();
    }
    // Check wielded gun, non-wielded guns, mags and tools
    // TODO: Build a proper gun->mag->ammo DAG (Directed Acyclic Graph)
    // to avoid checking same properties over and over
    // TODO: Make this understand bandoliers, pouches etc.
    // TODO: Cache items checked for reloading to avoid re-checking same items every turn
    // TODO: Make it understand smaller and bigger magazines
    item_location reloadable;
    visit_items( [this, &reloadable]( item * node, item * parent ) {
        if( !wants_to_reload( *this, *node ) ) {
            return VisitResponse::NEXT;
        }

        item_location node_loc = form_loc_recursive( this, node, parent );

        const item_location it_loc = select_ammo( node_loc ).ammo;
        if( it_loc && wants_to_reload_with( *node, *it_loc ) ) {
            reloadable = node_loc;
            add_msg_debug( debugmode::DF_NPC_ITEMAI, "%s has decided to reload %s!", name, node->tname() );
            return VisitResponse::ABORT;
        }

        return VisitResponse::NEXT;
    } );

    if( reloadable ) {
        return reloadable;
    }

    cached_info.emplace( "reloadables", 0.0 );
    return item_location();
}

bool npc::can_reload_current()
{
    const item_location weapon = get_wielded_item();
    if( !weapon || !weapon->is_gun() || !wants_to_reload( *this, *weapon ) ) {
        return false;
    }

    return static_cast<bool>( find_usable_ammo( weapon ) );
}

item_location npc::find_usable_ammo( const item_location &weap )
{
    if( !can_reload( *weap ) ) {
        return item_location();
    }

    item_location loc = select_ammo( weap ).ammo;
    if( !loc || !wants_to_reload_with( *weap, *loc ) ) {
        return item_location();
    }

    return loc;
}

item_location npc::find_usable_ammo( const item_location &weap ) const
{
    return const_cast<npc *>( this )->find_usable_ammo( weap );
}

item::reload_option npc::select_ammo( const item_location &base, bool, bool empty )
{
    if( !base ) {
        return item::reload_option();
    }

    std::vector<item::reload_option> ammo_list;
    list_ammo( base, ammo_list, empty );

    if( ammo_list.empty() ) {
        return item::reload_option();
    }

    // sort in order of move cost (ascending), then remaining ammo (descending) with empty magazines always last
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const item::reload_option & lhs,
    const item::reload_option & rhs ) {
        if( lhs.ammo->ammo_remaining( ) == 0 || rhs.ammo->ammo_remaining( ) == 0 ) {
            return ( lhs.ammo->ammo_remaining( ) != 0 ) > ( rhs.ammo->ammo_remaining( ) != 0 );
        }

        if( lhs.moves() != rhs.moves() ) {
            return lhs.moves() < rhs.moves();
        }

        return lhs.ammo->ammo_remaining( ) > rhs.ammo->ammo_remaining( );
    } );

    if( ammo_list[0].ammo.get_item()->ammo_remaining( ) > 0 ) {
        return ammo_list[0];
    } else {
        return item::reload_option();
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
    deactivate_or_discharge_bionic_weapon();
    weapon_bionic_uid = 0;
}

bool npc::activate_bionic_by_id( const bionic_id &cbm_id, bool eff_only )
{
    for( bionic &i : *my_bionics ) {
        if( i.id == cbm_id ) {
            if( !i.powered ) {
                return activate_bionic( i, eff_only );
            } else {
                return false;
            }
        }
    }
    return false;
}

bool npc::use_bionic_by_id( const bionic_id &cbm_id, bool eff_only )
{
    for( bionic &i : *my_bionics ) {
        if( i.id == cbm_id ) {
            if( !i.powered ) {
                return activate_bionic( i, eff_only );
            } else {
                return true;
            }
        }
    }
    return false;
}

bool npc::deactivate_bionic_by_id( const bionic_id &cbm_id, bool eff_only )
{
    for( bionic &i : *my_bionics ) {
        if( i.id == cbm_id ) {
            if( i.powered ) {
                return deactivate_bionic( i, eff_only );
            } else {
                return false;
            }
        }
    }
    return false;
}

bool npc::wants_to_recharge_cbm()
{
    const units::energy curr_power =  get_power_level();
    const float allowed_ratio = static_cast<int>( rules.cbm_recharge ) / 100.0f;
    const units::energy max_pow_allowed = get_max_power_level() * allowed_ratio;

    if( curr_power < max_pow_allowed ) {
        for( const bionic_id &bid : get_fueled_bionics() ) {
            if( !has_active_bionic( bid ) ) {
                return true;
            }
        }
        return get_fueled_bionics().empty(); //NPC might have power CBM that doesn't use the json fuel_opts entry
    }
    return false;
}

bool npc::can_use_offensive_cbm() const
{
    const float allowed_ratio = static_cast<int>( rules.cbm_reserve ) / 100.0f;
    return get_power_level() > get_max_power_level() * allowed_ratio;
}

bool npc::recharge_cbm()
{
    // non-allied NPCs don't consume resources to recharge
    if( !is_player_ally() ) {
        mod_power_level( get_max_power_level() );
        return true;
    }

    for( bionic_id &bid : get_fueled_bionics() ) {
        if( has_active_bionic( bid ) ) {
            continue;
        }

        if( !get_bionic_fuels( bid ).empty() ) {
            use_bionic_by_id( bid );
            return true;
        }
    }

    return false;
}

void outfit::activate_combat_items( npc &guy )
{
    for( item &candidate : worn ) {
        if( candidate.has_flag( flag_COMBAT_TOGGLEABLE ) && candidate.is_transformable() &&
            !candidate.active ) {

            const iuse_transform *transform = dynamic_cast<const iuse_transform *>
                                              ( candidate.type->get_use( "transform" )->get_actor_ptr() );

            // Due to how UPS works, there can be no charges_needed for UPS items.
            // Energy consumption is thus not checked at activation.
            // To prevent "flickering", this is a hard check for UPS charges > 0.
            if( transform->target->has_flag( flag_USE_UPS ) && guy.available_ups() == 0_kJ ) {
                continue;
            }
            if( transform->can_use( guy, candidate, tripoint_bub_ms::zero ).success() ) {
                transform->use( &guy, candidate, tripoint_bub_ms::zero );
                guy.add_msg_if_npc( _( "<npcname> activates their %s." ), candidate.display_name() );
            }
        }
    }
}

void npc::activate_combat_items()
{
    worn.activate_combat_items( *this );
}

void outfit::deactivate_combat_items( npc &guy )
{
    for( item &candidate : worn ) {
        if( candidate.has_flag( flag_COMBAT_TOGGLEABLE ) && candidate.is_transformable() &&
            candidate.active ) {
            const iuse_transform *transform = dynamic_cast<const iuse_transform *>
                                              ( candidate.type->get_use( "transform" )->get_actor_ptr() );
            if( transform->can_use( guy, candidate, tripoint_bub_ms::zero ).success() ) {
                transform->use( &guy, candidate, tripoint_bub_ms::zero );
                guy.add_msg_if_npc( _( "<npcname> deactivates their %s." ), candidate.display_name() );
            }
        }
    }
}

void npc::deactivate_combat_items()
{
    worn.deactivate_combat_items( *this );
}

void npc::prepare_for_combat()
{
    activate_combat_cbms();
    activate_combat_items();
}

void npc::cleanup_on_no_danger()
{
    deactivate_combat_cbms();
    deactivate_combat_items();
}

healing_options npc::patient_assessment( const Character &c )
{
    healing_options try_to_fix;
    try_to_fix.clear_all();

    for( bodypart_id part_id : c.get_all_body_parts( get_body_part_flags::only_main ) ) {
        if( c.has_effect( effect_bleed, part_id ) ) {
            try_to_fix.bleed = true;
        }
        if( c.has_effect( effect_bite, part_id ) ) {
            try_to_fix.bite = true;
        }
        if( c.has_effect( effect_infected, part_id ) ) {
            try_to_fix.infect = true;
        }

        int part_threshold = 75;
        if( part_id.id() == body_part_head ) {
            part_threshold += 20;
        } else if( part_id.id() == body_part_torso ) {
            part_threshold += 10;
        }
        part_threshold = std::min( 80, part_threshold );
        part_threshold = part_threshold * c.get_part_hp_max( part_id ) / 100;

        if( c.get_part_hp_cur( part_id ) <= part_threshold ) {
            if( !c.has_effect( effect_bandaged, part_id ) ) {
                try_to_fix.bandage = true;
            }
            if( !c.has_effect( effect_disinfected, part_id ) ) {
                try_to_fix.disinfect = true;
            }
        }
    }
    return try_to_fix;
}

npc_action npc::address_needs( float danger )
{
    map &here = get_map();

    Character &player_character = get_player_character();
    // rng because NPCs are not meant to be hypervigilant hawks that notice everything
    // and swing into action with alarming alacrity.
    // no sometimes they are just looking the other way, sometimes they hestitate.
    // ( also we can get huge performance boosts )
    if( one_in( 3 ) ) {
        healing_options try_to_fix_me = patient_assessment( *this );
        if( try_to_fix_me.any_true() ) {
            if( !use_bionic_by_id( bio_nanobots ) ) {
                ai_cache.can_heal = has_healing_options( try_to_fix_me );
                if( ai_cache.can_heal.any_true() ) {
                    return npc_heal;
                }
            }
        } else {
            deactivate_bionic_by_id( bio_nanobots );
        }
        if( static_cast<int>( get_skill_level( skill_firstaid ) ) > 0 ) {
            if( is_player_ally() ) {
                healing_options try_to_fix_other = patient_assessment( player_character );
                if( try_to_fix_other.any_true() ) {
                    ai_cache.can_heal = has_healing_options( try_to_fix_other );
                    if( ai_cache.can_heal.any_true() ) {
                        ai_cache.ally = g->shared_from( player_character );
                        return npc_heal_player;
                    }
                }
            }
            for( const npc &guy : g->all_npcs() ) {
                if( &guy == this || !guy.is_ally( *this ) || guy.posz() != posz() || !sees( here, guy ) ) {
                    continue;
                }
                healing_options try_to_fix_other = patient_assessment( guy );
                if( try_to_fix_other.any_true() ) {
                    ai_cache.can_heal = has_healing_options( try_to_fix_other );
                    if( ai_cache.can_heal.any_true() ) {
                        ai_cache.ally = g->shared_from( guy );
                        return npc_heal_player;
                    }
                }
            }
        }
    }

    if( one_in( 3 ) ) {
        if( get_perceived_pain() >= 15 ) {
            if( !activate_bionic_by_id( bio_painkiller ) && has_painkiller() && !took_painkiller() ) {
                return npc_use_painkiller;
            }
        } else {
            deactivate_bionic_by_id( bio_painkiller );
        }
    }

    if( one_in( 3 ) && can_reload_current() ) {
        return npc_reload;
    }

    if( one_in( 3 ) ) {
        add_msg_debug( debugmode::DF_NPC_ITEMAI, "%s decided to look into reloading items.", name );
        item_location reloadable = find_reloadable();
        if( reloadable ) {
            do_reload( reloadable );
            return npc_noop;
        }
    }

    // Extreme thirst or hunger, bypass safety check.
    if( get_thirst() > 80 ||
        get_stored_kcal() + stomach.get_calories() < get_healthy_kcal() * 0.75 ) {
        if( consume_food_from_camp() ) {
            return npc_noop;
        }
        if( consume_food() ) {
            return npc_noop;
        }
    }
    //Hallucinations have a chance of disappearing each turn
    if( is_hallucination() && one_in( 25 ) ) {
        die( &here, nullptr );
    }

    if( danger > NPC_DANGER_VERY_LOW ) {
        return npc_undecided;
    }

    if( one_in( 3 ) && ( get_thirst() > NPC_THIRST_CONSUME ||
                         get_hunger() > NPC_HUNGER_CONSUME ) ) {
        if( consume_food_from_camp() ) {
            return npc_noop;
        }
        if( consume_food() ) {
            return npc_noop;
        }
    }

    if( one_in( 3 ) && wants_to_recharge_cbm() && recharge_cbm() ) {
        return npc_noop;
    }

    if( can_do_pulp() ) {
        if( !activity ) {
            assign_activity( pulp_activity_actor( *pulp_location ) );
        }
        return npc_player_activity;
    } else if( find_corpse_to_pulp() ) {
        move_to_next();
        return npc_noop;
    }

    if( one_in( 3 ) && adjust_worn() ) {
        return npc_noop;
    }

    const auto could_sleep = [&]() {
        if( danger <= 0.01 ) {
            if( get_sleepiness() >= sleepiness_levels::TIRED ) {
                return true;
            }
            if( is_walking_with() && player_character.in_sleep_state() &&
                get_sleepiness() > ( sleepiness_levels::TIRED / 2 ) ) {
                return true;
            }
        }
        return false;
    };
    // TODO: More risky attempts at sleep when exhausted
    if( could_sleep() ) {
        if( !is_player_ally() ) {
            // TODO: Make tired NPCs handle sleep offscreen
            set_sleepiness( 0 );
            return npc_undecided;
        }

        if( rules.has_flag( ally_rule::allow_sleep ) ||
            get_sleepiness() > sleepiness_levels::MASSIVE_SLEEPINESS ) {
            return npc_sleep;
        }
        if( player_character.in_sleep_state() ) {
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
    const map &here = get_map();

    Character &player_character = get_player_character();
    if( ( attitude == NPCATT_TALK || attitude == NPCATT_RECOVER_GOODS ) &&
        sees( here, player_character ) ) {
        if( player_character.in_sleep_state() ) {
            // Leave sleeping characters alone.
            return npc_undecided;
        }
        if( rl_dist( pos_abs(), player_character.pos_abs() ) <= 6 ) {
            return npc_talk_to_player;    // Close enough to talk to you
        } else {
            if( one_in( 10 ) ) {
                say( chat_snippets().snip_lets_talk.translated() );
            }
            return npc_follow_player;
        }
    }

    if( attitude == NPCATT_MUG && sees( here, player_character ) ) {
        if( one_in( 3 ) ) {
            say( chat_snippets().snip_mug_dontmove.translated() );
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
        if( rl_dist( pos_abs(), player_character.pos_abs() ) >= 12 || !sees( here, player_character ) ) {
            int intense = get_effect_int( effect_catch_up );
            if( intense < 10 ) {
                say( chat_snippets().snip_keep_up.translated() );
                add_effect( effect_catch_up, 5_turns );
                return npc_pause;
            } else {
                say( chat_snippets().snip_im_leaving_you.translated() );
                set_attitude( NPCATT_NULL );
                return npc_pause;
            }
        } else if( has_omt_destination() && !has_flag( json_flag_CANNOT_MOVE ) ) {
            return npc_goto_destination;
        } else { // At goal. Now, waiting on nearby player
            return npc_pause;
        }
    }
    return npc_undecided;
}

npc_action npc::long_term_goal_action()
{
    add_msg_debug( debugmode::DF_NPC, "long_term_goal_action()" );

    if( mission == NPC_MISSION_SHOPKEEP || mission == NPC_MISSION_SHELTER || ( is_player_ally() &&
            mission != NPC_MISSION_TRAVELLING ) ) {
        return npc_pause;    // Shopkeepers just stay put.
    }

    if( !has_omt_destination() ) {
        set_omt_destination();
    }

    if( has_omt_destination() && !has_flag( json_flag_CANNOT_MOVE ) ) {
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
        case aim_rule::WHEN_CONVENIENT:
            return emergency() ? 1.5f : 1.0f;
        case aim_rule::SPRAY:
            return 2.0f;
        case aim_rule::PRECISE:
            return emergency() ? 1.0f : 0.75f;
        case aim_rule::STRICTLY_PRECISE:
            return 0.5f;
    }

    return 1.0f;
}

int npc::confident_shoot_range( const item &it, int recoil ) const
{
    int res = 0;
    if( !it.is_gun() ) {
        return res;
    }

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
    add_msg_debug( debugmode::DF_NPC, "%s: Even Chance Dist / Max Dispersion: %.1f / %.1f",
                   gun.tname(), even_chance_range, max_dispersion );
    return std::max<int>( confident_range, 1 );
}

int npc::confident_throw_range( const item &thrown, Creature *target ) const
{
    double average_dispersion = throwing_dispersion( thrown, target ) / 2.0;
    double even_chance_range = ( target == nullptr ? 0.5 : target->ranged_target_size() ) /
                               average_dispersion;
    double confident_range = even_chance_range * confidence_mult();
    add_msg_debug( debugmode::DF_NPC, "confident_throw_range == %d",
                   static_cast<int>( confident_range ) );
    return static_cast<int>( confident_range );
}

// Index defaults to -1, i.e., wielded weapon
bool npc::wont_hit_friend( const tripoint_bub_ms &tar, const item &it, bool throwing ) const
{
    if( !throwing && it.is_gun() && it.empty() ) {
        return true;    // Prevent calling nullptr ammo_data
    }

    if( throwing && rl_dist( pos_bub(), tar ) == 1 ) {
        return true;    // If we're *really* sure that our aim is dead-on
    }

    map &here = get_map();
    std::vector<tripoint_bub_ms> trajectory = here.find_clear_path( pos_bub(), tar );

    units::angle target_angle = coord_to_angle( pos_bub(), tar );
    double dispersion = throwing ? throwing_dispersion( it, nullptr ) : total_gun_dispersion( it,
                        recoil_total(), it.ammo_data()->ammo->shot_spread ).max();
    units::angle safe_angle = units::from_arcmin( dispersion );

    for( const auto &fr : ai_cache.friends ) {
        const shared_ptr_fast<Creature> ally_p = fr.lock();
        if( !ally_p || !sees( here, *ally_p ) ) {
            continue;
        }
        const Creature &ally = *ally_p;

        // TODO: When lines are straight again, optimize for small distances
        for( tripoint_bub_ms &p : trajectory ) {
            if( ally.pos_bub() == p ) {
                return false;
            }
        }

        // TODO: Extract common functions with turret target selection
        units::angle safe_angle_ally = safe_angle;
        units::angle ally_angle = coord_to_angle( pos_bub(), ally.pos_bub() );
        units::angle angle_diff = units::fabs( ally_angle - target_angle );
        angle_diff = std::min( 360_degrees - angle_diff, angle_diff );
        if( angle_diff < safe_angle_ally ) {
            // TODO: Disable NPC whining is it's other NPC who prevents aiming
            add_msg_debug( debugmode::DF_NPC_COMBATAI, "%s was in %s line of fire", ally.get_name(),
                           get_name() );
            return false;
        }
    }

    return true;
}

bool npc::enough_time_to_reload( const item &gun ) const
{
    const map &here = get_map();

    int rltime = item_reload_cost( gun, item( gun.ammo_default() ),
                                   gun.ammo_capacity(
                                       item_controller->find_template( gun.ammo_default() )->ammo->type ) );
    const float turns_til_reloaded = static_cast<float>( rltime ) / get_speed();

    const Creature *target = current_target();
    if( target == nullptr ) {
        add_msg_debug( debugmode::DF_NPC_ITEMAI, "%s can't see anyone around: great time to reload.",
                       name );
        return true;
    }

    const int distance = rl_dist( pos_bub(), target->pos_bub() );
    const float target_speed = target->speed_rating();
    const float turns_til_reached = distance / target_speed;
    if( target->is_avatar() || target->is_npc() ) {
        const Character &foe = dynamic_cast<const Character &>( *target );
        const item_location weapon = foe.get_wielded_item();
        // TODO: Allow reloading if the player has a low accuracy gun
        if( sees( here, foe ) && weapon && weapon->is_gun() && rltime > 200 &&
            weapon->gun_range( true ) > distance + turns_til_reloaded / target_speed ) {
            // Don't take longer than 2 turns if player has a gun
            add_msg_debug( debugmode::DF_NPC_ITEMAI, "%s is shy about reloading with &s standing right there.",
                           name, foe.name );
            return false;
        }
    }

    // TODO: Handle monsters with ranged attacks and players with CBMs
    add_msg_debug( debugmode::DF_NPC_ITEMAI, "%s turns to reload: %i./nTurns til reached: %i.", name,
                   static_cast<int>( turns_til_reloaded ), static_cast<int>( turns_til_reached ) );
    return turns_til_reloaded < turns_til_reached;
}

void npc::aim( const Target_attributes &target_attributes )
{
    const item_location weapon = get_wielded_item();
    double aim_amount = weapon ? aim_per_move( *weapon, recoil ) : 0.0;
    const aim_mods_cache aim_cache = gen_aim_mods_cache( *weapon );
    int hold_moves = moves;
    double hold_recoil = recoil;
    while( aim_amount > 0 && recoil > 0 && moves > 0 ) {
        moves--;
        recoil -= aim_amount;
        recoil = std::max( 0.0, recoil );
        aim_amount = aim_per_move( *weapon, recoil, target_attributes, { std::ref( aim_cache ) } );
    }
    add_msg_debug( debugmode::debug_filter::DF_NPC_COMBATAI,
                   "%s reduced recoil from %f to %f in %d moves",
                   this->get_name(), hold_recoil, recoil, hold_moves );
}

bool npc::update_path( const tripoint_bub_ms &p, const bool no_bashing, bool force )
{
    if( p == pos_bub() ) {
        path.clear();
        return true;
    }

    while( !path.empty() && path[0] == pos_bub() ) {
        path.erase( path.begin() );
    }

    if( !path.empty() ) {
        const tripoint_bub_ms &last = path[path.size() - 1];
        if( last == p && ( path[0].z() != posz() || rl_dist( path[0], pos_bub() ) <= 1 ) ) {
            // Our path already leads to that point, no need to recalculate
            return true;
        }
    }

    std::vector<tripoint_bub_ms> new_path = get_map().route( pos_bub(), pathfinding_target::point( p ),
                                            get_pathfinding_settings( no_bashing ),
                                            get_path_avoid() );
    if( new_path.empty() ) {
        if( !ai_cache.sound_alerts.empty() ) {
            ai_cache.sound_alerts.erase( ai_cache.sound_alerts.begin() );
            add_msg_debug( debugmode::DF_NPC, "failed to path to sound alert %s->%s",
                           pos_bub().to_string_writable(), p.to_string_writable() );
        }
        add_msg_debug( debugmode::DF_NPC, "Failed to path %s->%s",
                       pos_bub().to_string_writable(), p.to_string_writable() );
    }

    while( !new_path.empty() && new_path[0] == pos_bub() ) {
        new_path.erase( new_path.begin() );
    }

    if( !new_path.empty() || force ) {
        path = std::move( new_path );
        return true;
    }

    return false;
}

void npc::set_guard_pos( const tripoint_abs_ms &p )
{
    ai_cache.guard_pos = p;
}

bool npc::can_open_door( const tripoint_bub_ms &p, const bool inside ) const
{
    return !is_hallucination() && !rules.has_flag( ally_rule::avoid_doors ) &&
           get_map().open_door( *this, p, inside, true );
}

bool npc::can_move_to( const tripoint_bub_ms &p, bool no_bashing ) const
{
    map &here = get_map();

    // Allow moving into any bashable spots, but penalize them during pathing
    // Doors are not passable for hallucinations
    return( rl_dist( pos_bub(), p ) <= 1 && here.has_floor_or_water( p ) &&
            !g->is_dangerous_tile( p ) &&
            ( here.passable_through( p ) || ( can_open_door( p, !here.is_outside( pos_bub() ) ) &&
                    !is_hallucination() ) ||
              ( !no_bashing && here.bash_rating( smash_ability(), p ) > 0 ) )
          );
}

void npc::move_to( const tripoint_bub_ms &pt, bool no_bashing, std::set<tripoint_bub_ms> *nomove )
{
    if( has_flag( json_flag_CANNOT_MOVE ) ) {
        move_pause();
        return;
    }
    tripoint_bub_ms p = pt;
    map &here = get_map();
    const tripoint_bub_ms pos = pos_bub( here );

    if( sees_dangerous_field( p )
        || ( nomove != nullptr && nomove->find( p ) != nomove->end() ) ) {
        // Move to a neighbor field instead, if possible.
        // Maybe this code already exists somewhere?
        std::vector<tripoint_bub_ms> other_points = here.get_dir_circle( pos, p );
        for( const tripoint_bub_ms &ot : other_points ) {
            if( could_move_onto( ot )
                && ( nomove == nullptr || nomove->find( ot ) == nomove->end() ) ) {

                p = ot;
                break;
            }
        }
    }

    recoil = MAX_RECOIL;

    if( has_effect( effect_stunned ) || has_effect( effect_psi_stunned ) ) {
        p.x() = rng( pos.x() - 1, pos.x() + 1 );
        p.y() = rng( pos.y() - 1, pos.y() + 1 );
        p.z() = pos.z();
    }

    // nomove is used to resolve recursive invocation, so reset destination no
    // matter it was changed by stunned effect or not.
    if( nomove != nullptr && nomove->find( p ) != nomove->end() ) {
        p = pos;
    }

    // "Long steps" are allowed when crossing z-levels
    // Stairs teleport the player too
    if( rl_dist( pos, p ) > 1 && p.z() == pos.z() ) {
        // On the same level? Not so much. Something weird happened
        path.clear();
        move_pause();
    }
    creature_tracker &creatures = get_creature_tracker();
    bool attacking = false;
    if( creatures.creature_at<monster>( p ) ) {
        attacking = true;
    }
    if( !move_effects( attacking ) ) {
        mod_moves( -get_speed() );
        return;
    }

    Creature *critter = creatures.creature_at( p );
    if( critter != nullptr ) {
        if( critter == this || has_flag( json_flag_CANNOT_ATTACK ) ) { // We're just pausing!
            move_pause();
            return;
        }
        const Creature::Attitude att = attitude_to( *critter );
        if( att == Attitude::HOSTILE ) {
            if( !no_bashing ) {
                warn_about( "cant_flee", 5_turns + rng( 0, 5 ) * 1_turns );
                melee_attack( *critter, true );
            } else {
                move_pause();
            }

            return;
        }

        if( critter->is_avatar() ) {
            if( sees( here, *critter ) ) {
                say( chat_snippets().snip_let_me_pass.translated() );
            } else {
                stumble_invis( *critter );
            }
        }

        // Let NPCs push each other when non-hostile
        // TODO: Have them attack each other when hostile
        npc *np = dynamic_cast<npc *>( critter );
        if( np != nullptr && !np->in_sleep_state() ) {
            std::unique_ptr<std::set<tripoint_bub_ms>> newnomove;
            std::set<tripoint_bub_ms> *realnomove;
            if( nomove != nullptr ) {
                realnomove = nomove;
            } else {
                // create the no-move list
                newnomove = std::make_unique<std::set<tripoint_bub_ms>>();
                realnomove = newnomove.get();
            }
            // other npcs should not try to move into this npc anymore,
            // so infinite loop can be avoided.
            realnomove->insert( pos );
            say( chat_snippets().snip_let_me_pass.translated() );
            np->move_away_from( pos, true, realnomove );
            // if we moved NPC, readjust their path, so NPCs don't jostle each other out of their activity paths.
            if( np->attitude == NPCATT_ACTIVITY ) {
                std::vector<tripoint_bub_ms> activity_route = np->get_auto_move_route();
                if( !activity_route.empty() && !np->has_destination_activity() ) {
                    tripoint_bub_ms final_destination;
                    if( destination_point ) {
                        final_destination = here.get_bub( *destination_point );
                    } else {
                        final_destination = activity_route.back();
                    }
                    np->update_path( final_destination );
                }
            }
        }

        if( critter->pos_bub( here ) == p ) {
            move_pause();
            return;
        }
    }

    // Boarding moving vehicles is fine, unboarding isn't
    bool moved = false;
    if( const optional_vpart_position vp = here.veh_at( pos ) ) {
        const optional_vpart_position ovp = here.veh_at( p );
        if( vp->vehicle().is_moving() &&
            ( veh_pointer_or_null( ovp ) != veh_pointer_or_null( vp ) ||
              !ovp.part_with_feature( VPFLAG_BOARDABLE, true ) ) ) {
            move_pause();
            return;
        }
    }

    Character &player_character = get_player_character();
    if( p.z() != pos.z() ) {
        // Z-level move
        // For now just teleport to the destination
        // TODO: Make it properly find the tile to move to
        if( is_mounted() ) {
            move_pause();
            return;
        }
        mod_moves( -get_speed() );
        moved = true;
    } else if( has_effect( effect_stumbled_into_invisible ) &&
               here.has_field_at( p, field_fd_last_known ) && !sees( here, player_character ) &&
               attitude_to( player_character ) == Attitude::HOSTILE ) {
        attack_air( p );
        move_pause();
    } else if( here.passable_through( p ) && !here.has_flag( ter_furn_flag::TFLAG_DOOR, p ) ) {
        bool diag = trigdist && pos.x() != p.x() && pos.y() != p.y();
        if( is_mounted() ) {
            const double base_moves = run_cost( here.combined_movecost( pos, p ),
                                                diag ) * 100.0 / mounted_creature->get_speed();
            const double encumb_moves = get_weight() / 4800.0_gram;
            mod_moves( -static_cast<int>( std::ceil( base_moves + encumb_moves ) ) );
            if( mounted_creature->has_flag( mon_flag_RIDEABLE_MECH ) ) {
                mounted_creature->use_mech_power( 1_kJ );
            }
        } else {
            mod_moves( -run_cost( here.combined_movecost( pos, p ), diag ) );
        }
        moved = true;
    } else if( here.open_door( *this, p, !here.is_outside( pos ), true ) ) {
        if( !is_hallucination() ) { // hallucinations don't open doors
            here.open_door( *this, p, !here.is_outside( pos ) );
            mod_moves( -get_speed() );
        } else { // hallucinations teleport through doors
            mod_moves( -get_speed() );
            moved = true;
        }
    } else if( doors::can_unlock_door( here, *this, tripoint_bub_ms( pt ) ) ) {
        if( !is_hallucination() ) {
            doors::unlock_door( here, *this, tripoint_bub_ms( pt ) );
        } else {
            mod_moves( -get_speed() );
            moved = true;
        }
    } else if( get_dex() > 1 && here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_CLIMBABLE, p ) ) {
        ///\EFFECT_DEX_NPC increases chance to climb CLIMBABLE furniture or terrain
        int climb = get_dex();
        if( one_in( climb ) ) {
            add_msg_if_npc( m_neutral, _( "%1$s tries to climb the %2$s but slips." ), get_name(),
                            here.tername( p ) );
            mod_moves( -get_speed() * 4 );
        } else {
            add_msg_if_npc( m_neutral, _( "%1$s climbs over the %2$s." ), get_name(), here.tername( p ) );
            mod_moves( ( -get_speed() * 5 ) - ( rng( 0, climb ) * 20 ) );
            moved = true;
        }
    } else if( !no_bashing && smash_ability() > 0 && here.is_bashable( p ) &&
               here.bash_rating( smash_ability(), p ) > 0 ) {
        mod_moves( -get_speed() * 0.8 );
        here.bash( p, smash_ability() );
    } else {
        if( attitude == NPCATT_MUG ||
            attitude == NPCATT_KILL ||
            attitude == NPCATT_WAIT_FOR_LEAVE ) {
            set_attitude( NPCATT_FLEE_TEMP );
        }

        set_moves( 0 );
    }

    if( moved ) {
        make_footstep_noise();
        const tripoint_bub_ms old_pos = pos;
        setpos( here, p );
        if( old_pos.x() - p.x() < 0 ) {
            facing = FacingDirection::RIGHT;
        } else {
            facing = FacingDirection::LEFT;
        }
        if( is_mounted() ) {
            if( mounted_creature->pos_abs() != pos_abs() ) {
                mounted_creature->setpos( pos_abs() );
                mounted_creature->facing = facing;
                mounted_creature->process_triggers();
                here.creature_in_field( *mounted_creature );
                here.creature_on_trap( *mounted_creature );
            }
        }
        if( here.has_flag( ter_furn_flag::TFLAG_UNSTABLE, pos ) &&
            !here.has_vehicle_floor( pos ) ) {
            add_effect( effect_bouldering, 1_turns, true );
        } else if( has_effect( effect_bouldering ) ) {
            remove_effect( effect_bouldering );
        }

        if( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NO_SIGHT, pos ) ) {
            add_effect( effect_no_sight, 1_turns, true );
        } else if( has_effect( effect_no_sight ) ) {
            remove_effect( effect_no_sight );
        }

        if( in_vehicle ) {
            here.unboard_vehicle( old_pos );
        }

        // Close doors behind self (if you can)
        if( ( rules.has_flag( ally_rule::close_doors ) && is_player_ally() ) && !is_hallucination() ) {
            doors::close_door( here, *this, old_pos );
        }
        // Lock doors as well
        if( ( rules.has_flag( ally_rule::lock_doors ) && is_player_ally() ) && !is_hallucination() ) {
            doors::lock_door( here, *this, old_pos );
        }

        if( here.veh_at( p ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
            here.board_vehicle( p, this );
        }
        here.creature_on_trap( *this );
        here.creature_in_field( *this );

        if( will_be_cramped_in_vehicle_tile( here, here.get_abs( p ) ) ) {
            if( !has_effect( effect_cramped_space ) ) {
                add_msg_if_player_sees( *this, m_warning,
                                        string_format( _( "%s has to really cram their huge body to fit." ), disp_name() ) );
            }
            add_effect( effect_cramped_space, 2_turns, true );
        }
    }
}


void npc::move_to_next()
{
    while( !path.empty() && pos_bub() == path[0] ) {
        path.erase( path.begin() );
    }

    if( path.empty() ) {
        add_msg_debug( debugmode::DF_NPC, "npc::move_to_next() called with an empty path or path "
                       "containing only current position" );
        move_pause();
        return;
    }

    move_to( path[0] );
    if( !path.empty() && pos_bub() == path[0] ) { // Move was successful
        path.erase( path.begin() );
    }
}

void npc::avoid_friendly_fire()
{
    // TODO: To parameter
    const tripoint_bub_ms tar = current_target()->pos_bub();
    // Calculate center of weight of friends and move away from that
    std::vector<tripoint_bub_ms> fr_pts;
    fr_pts.reserve( ai_cache.friends.size() );
    for( const auto &fr : ai_cache.friends ) {
        if( shared_ptr_fast<Creature> fr_p = fr.lock() ) {
            fr_pts.push_back( fr_p->pos_bub() );
        }
    }

    tripoint_bub_ms center = midpoint_round_to_nearest( fr_pts );

    std::vector<tripoint_bub_ms> candidates = closest_points_first( pos_bub(), 1, 1 );
    std::sort( candidates.begin(), candidates.end(),
    [&tar, &center]( const tripoint_bub_ms & l, const tripoint_bub_ms & r ) {
        return ( rl_dist( l, tar ) - rl_dist( l, center ) ) <
               ( rl_dist( r, tar ) - rl_dist( r, center ) );
    } );

    for( const tripoint_bub_ms &pt : candidates ) {
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

void npc::move_away_from( const tripoint_bub_ms &pt, bool no_bash_atk,
                          std::set<tripoint_bub_ms> *nomove )
{
    tripoint_bub_ms best_pos = pos_bub();
    int best = -1;
    int chance = 2;
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_in_radius( pos_bub(), 1 ) ) {
        if( nomove != nullptr && nomove->find( p ) != nomove->end() ) {
            continue;
        }

        if( p == pos_bub() ) {
            continue;
        }

        if( p == get_player_character().pos_bub() ) {
            continue;
        }

        const int cost = here.combined_movecost( pos_bub(), p );
        if( cost <= 0 ) {
            continue;
        }

        const int dst = std::abs( p.x() - pt.x() ) + std::abs( p.y() - pt.y() ) + std::abs(
                            p.z() - pt.z() );
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

bool npc::find_job_to_perform()
{
    // cleanup history
    auto fetch_itr = job.fetch_history.begin();
    while( fetch_itr != job.fetch_history.end() ) {
        if( fetch_itr->second != calendar::turn ) {
            fetch_itr = job.fetch_history.erase( fetch_itr );
        } else {
            fetch_itr++;
        }
    }

    for( activity_id &elem : job.get_prioritised_vector() ) {
        if( job.get_priority_of_job( elem ) == 0 ) {
            continue;
        }
        player_activity scan_act = player_activity( elem );
        if( elem == ACT_MOVE_LOOT ) {
            assign_activity( elem );
        } else if( generic_multi_activity_handler( scan_act, *this->as_character(), true ) ) {
            assign_activity( elem );
            return true;
        }
    }
    return false;
}

void npc::worker_downtime()
{
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    // are we already in a chair
    if( here.has_flag_furn( ter_furn_flag::TFLAG_CAN_SIT, pos_bub() ) ) {
        // just chill here
        move_pause();
        return;
    }
    //  already know of a chair, go there - if there isn't already another creature there.
    //  this is a bit of behind the scene omniscience for the npc, since ideally the npc
    //  should walk to the chair and then change their destination due to the seat being taken.
    tripoint_bub_ms local_chair_pos = chair_pos ? here.get_bub( *chair_pos ) :
                                      tripoint_bub_ms::zero;
    if( chair_pos && !creatures.creature_at( local_chair_pos ) ) {
        if( here.has_flag_furn( ter_furn_flag::TFLAG_CAN_SIT, local_chair_pos ) ) {
            update_path( local_chair_pos );
            if( pos_abs() == *chair_pos || path.empty() ) {
                move_pause();
                path.clear();
            } else {
                move_to_next();
            }
            wander_pos = std::nullopt;
            return;
        } else {
            chair_pos = std::nullopt;
        }
    } else {
        // find a chair
        if( !is_mounted() ) {
            for( const tripoint_bub_ms &elem : here.points_in_radius( pos_bub(), 30 ) ) {
                if( here.has_flag_furn( ter_furn_flag::TFLAG_CAN_SIT, elem ) && !creatures.creature_at( elem ) &&
                    could_move_onto( elem ) &&
                    here.point_within_camp( here.get_abs( elem ) ) ) {
                    // this one will do
                    chair_pos = here.get_abs( elem );
                    return;
                }
            }
        }
    }
    // we got here if there are no chairs available.
    // wander back to near the bulletin board of the camp.
    if( wander_pos ) {
        update_path( here.get_bub( *wander_pos ) );
        if( pos_abs() == *wander_pos || path.empty() ) {
            move_pause();
            path.clear();
            if( one_in( 30 ) ) {
                wander_pos = std::nullopt;
            }
        } else {
            move_to_next();
        }
        return;
    }
    if( assigned_camp ) {
        std::optional<basecamp *> bcp = overmap_buffer.find_camp( ( *assigned_camp ).xy() );
        if( !bcp ) {
            assigned_camp = std::nullopt;
            move_pause();
            return;
        }
        basecamp *temp_camp = *bcp;
        std::vector<tripoint_bub_ms> pts;
        for( const tripoint_bub_ms &elem : here.points_in_radius( here.get_bub(
                    tripoint_abs_ms( temp_camp->get_bb_pos() ) ), 10 ) ) {
            if( creatures.creature_at( elem ) || !could_move_onto( elem ) ||
                here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, elem ) ||
                !here.has_floor_or_water( elem ) || g->is_dangerous_tile( elem ) ) {
                continue;
            }
            pts.push_back( elem );
        }
        if( !pts.empty() ) {
            wander_pos = here.get_abs( random_entry( pts ) );
            return;
        }
    }
    move_pause();
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
    pause();
}

static std::optional<tripoint_bub_ms> nearest_passable( const tripoint_bub_ms &p,
        const tripoint_bub_ms &closest_to )
{
    map &here = get_map();
    if( here.passable_through( p ) ) {
        return p;
    }

    // We need to path to adjacent tile, not the exact one
    // Let's pick the closest one to us that is passable
    std::vector<tripoint_bub_ms> candidates = closest_points_first( p, 1 );
    std::sort( candidates.begin(), candidates.end(), [ closest_to ]( const tripoint_bub_ms & l,
    const tripoint_bub_ms & r ) {
        return rl_dist( closest_to, l ) < rl_dist( closest_to, r );
    } );
    auto iter = std::find_if( candidates.begin(), candidates.end(),
    [&here]( const tripoint_bub_ms & pt ) {
        return here.passable_through( pt );
    } );
    if( iter != candidates.end() ) {
        return *iter;
    }

    return std::nullopt;
}

void npc::move_away_from( const std::vector<sphere> &spheres, bool no_bashing )
{
    if( spheres.empty() ) {
        return;
    }

    tripoint_bub_ms minp( pos_bub() );
    tripoint_bub_ms maxp( pos_bub() );

    for( const sphere &elem : spheres ) {
        minp.x() = std::min( minp.x(), elem.center.x - elem.radius );
        minp.y() = std::min( minp.y(), elem.center.y - elem.radius );
        maxp.x() = std::max( maxp.x(), elem.center.x + elem.radius );
        maxp.y() = std::max( maxp.y(), elem.center.y + elem.radius );
    }

    const tripoint_range<tripoint_bub_ms> range( minp, maxp );

    std::vector<tripoint_bub_ms> escape_points;

    map &here = get_map();
    std::copy_if( range.begin(), range.end(), std::back_inserter( escape_points ),
    [&here]( const tripoint_bub_ms & elem ) {
        return here.passable( elem ) && here.has_floor_or_water( elem );
    } );

    cata::sort_by_rating( escape_points.begin(),
    escape_points.end(), [&]( const tripoint_bub_ms & elem ) {
        const int danger = std::accumulate( spheres.begin(), spheres.end(), 0,
        [&]( const int sum, const sphere & s ) {
            return sum + std::max( s.radius - rl_dist( elem.raw(), s.center ), 0 );
        } );

        const int distance = rl_dist( pos_bub(), elem );
        const int move_cost = here.move_cost( elem );

        return std::make_tuple( danger, distance, move_cost );
    } );

    for( const tripoint_bub_ms &elem : escape_points ) {
        update_path( elem, no_bashing );

        if( elem == pos_bub() || !path.empty() ) {
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
    map &here = get_map();
    for( const tripoint_bub_ms &p : closest_points_first( pos_bub(), 6 ) ) {
        if( here.sees_some_items( p, *this ) && sees( here, p ) ) {
            for( const item &it : here.i_at( p ) ) {
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
        see_item_say_smth( itype_thorazine, chat_snippets().snip_no_to_thorazine.translated() );
        see_item_say_smth( itype_lsd, chat_snippets().snip_yes_to_lsd.translated() );
        return;
    }

    if( is_player_ally() && !rules.has_flag( ally_rule::allow_pick_up ) ) {
        // Grabbing stuff not allowed by our "owner"
        add_msg_debug( debugmode::DF_NPC_ITEMAI,
                       "%s considered picking something up but player said not to.", name );
        return;
    }

    fetching_item = false;
    wanted_item = {};
    int best_value = minimum_item_value();
    // Not perfect, but has to mirror pickup code
    units::volume volume_allowed = volume_capacity() - volume_carried();
    units::mass   weight_allowed = weight_capacity() - weight_carried();
    // For some reason range limiting by vision doesn't work properly
    const int range = 6;
    //int range = sight_range( g->light_level( posz() ) );
    //range = std::max( 1, std::min( 12, range ) );

    if( volume_allowed <= 0_ml || weight_allowed <= 0_gram ) {
        add_msg_debug( debugmode::DF_NPC_ITEMAI, "%s considered picking something up, but no storage left.",
                       name );
        return;
    }

    const auto consider_item =
        [&best_value, this]
    ( const item & it, const tripoint_bub_ms & p ) {
        if( ::good_for_pickup( it, *this, p ) ) {
            wanted_item_pos = p;
            best_value = has_item_whitelist() ? 1000 : value( it );
            return true;
        } else {
            return false;
        }
    };

    map &here = get_map();
    // Harvest item doesn't exist, so we'll be checking by its name
    std::string wanted_name;
    const auto consider_terrain =
    [ this, volume_allowed, &wanted_name, &here ]( const tripoint_bub_ms & p ) {
        // We only want to pick plants when there are no items to pick
        if( !has_item_whitelist() || wanted_item.get_item() != nullptr || !wanted_name.empty() ||
            volume_allowed < 250_ml ) {
            return;
        }

        const auto &harvest = here.get_harvest_names( p );
        for( const auto &entry : harvest ) {
            if( item_name_whitelisted( entry ) ) {
                wanted_name = entry;
                wanted_item_pos = p;
                break;
            }
        }
    };

    for( const tripoint_bub_ms &p : closest_points_first( pos_bub(), range ) ) {
        // TODO: Make this sight check not overdraw nearby tiles
        // TODO: Optimize that zone check
        if( is_player_ally() && g->check_zone( zone_type_NO_NPC_PICKUP, p ) ) {
            add_msg_debug( debugmode::DF_NPC_ITEMAI,
                           "%s didn't pick up an item because it's in a no-pickup zone.", name );
            continue;
        }

        const tripoint_abs_ms abs_p = pos_abs() + ( p - pos_bub() );
        const int prev_num_items = ai_cache.searched_tiles.get( abs_p, -1 );
        // Prefetch the number of items present so we can bail out if we already checked here.
        map_stack m_stack = here.i_at( p );
        int num_items = m_stack.size();
        const optional_vpart_position vp = here.veh_at( p );
        if( vp ) {
            if( const std::optional<vpart_reference> vp_cargo = vp.cargo() ) {
                num_items += vp_cargo->items().size();
            }
        }
        if( prev_num_items == num_items ) {
            continue;
        }
        auto cache_tile = [this, &abs_p, num_items]() {
            if( wanted_item.get_item() == nullptr ) {
                ai_cache.searched_tiles.insert( 1000, abs_p, num_items );
            }
        };
        bool can_see = false;
        if( here.sees_some_items( p, *this ) && sees( here, p ) ) {
            can_see = true;
            for( item &it : m_stack ) {
                if( consider_item( it, p ) ) {
                    wanted_item = item_location{ map_cursor{tripoint_bub_ms( p )}, &it };
                }
            }
        }

        // Not cached because it gets checked once and isn't expected to change.
        if( can_see || sees( here, p ) ) {
            can_see = true;
            consider_terrain( p );
        }

        if( !vp || vp->vehicle().is_moving() || !( can_see || sees( here, p ) ) ) {
            cache_tile();
            continue;
        }
        const std::optional<vpart_reference> cargo = vp.cargo();
        static const std::string locked_string( "LOCKED" );
        // TODO: Let player know what parts are safe from NPC thieves
        if( !cargo || cargo->has_feature( locked_string ) ) {
            cache_tile();
            continue;
        }

        static const std::string cargo_locking_string( "CARGO_LOCKING" );
        if( vp.part_with_feature( cargo_locking_string, true ) ) {
            cache_tile();
            continue;
        }

        for( item &it : cargo->items() ) {
            if( consider_item( it, p ) ) {
                wanted_item = {  vehicle_cursor{ cargo->vehicle(), static_cast<ptrdiff_t>( cargo->part_index() ) }, &it };
            }
        }
        cache_tile();
    }

    if( wanted_item.get_item() != nullptr ) {
        wanted_name = wanted_item->tname();
    }

    if( wanted_name.empty() ) {
        return;
    }

    fetching_item = true;

    // TODO: Move that check above, make it multi-target pathing and use it
    // to limit tiles available for choice of items
    const int dist_to_item = rl_dist( wanted_item_pos, pos_bub() );
    if( const std::optional<tripoint_bub_ms> dest = nearest_passable( wanted_item_pos, pos_bub() ) ) {
        update_path( *dest );
    }

    if( path.empty() && dist_to_item > 1 ) {
        // Item not reachable, let's just totally give up for now
        fetching_item = false;
        wanted_item = {};
    }

    if( fetching_item && rl_dist( wanted_item_pos, pos_bub() ) > 1 && is_walking_with() ) {
        say( _( "Hold on, I want to pick up that %s." ), wanted_name );
    }
}

void npc::pick_up_item()
{
    if( is_hallucination() ) {
        return;
    }

    if( !rules.has_flag( ally_rule::allow_pick_up ) && is_player_ally() ) {
        add_msg_debug( debugmode::DF_NPC, "%s::pick_up_item(); Canceling on player's request", get_name() );
        fetching_item = false;
        wanted_item = {};
        mod_moves( -1 );
        return;
    }

    map &here = get_map();
    const std::optional<vpart_reference> vp = here.veh_at( wanted_item_pos ).part_with_feature(
                VPFLAG_CARGO, false );
    const bool has_cargo = vp && !vp->has_feature( "LOCKED" );

    if( ( !here.has_items( wanted_item_pos ) && !has_cargo &&
          !here.is_harvestable( wanted_item_pos ) && sees( here, wanted_item_pos ) ) ||
        ( is_player_ally() && g->check_zone( zone_type_NO_NPC_PICKUP, wanted_item_pos ) ) ) {
        // Items we wanted no longer exist and we can see it
        // Or player who is leading us doesn't want us to pick it up
        fetching_item = false;
        wanted_item = {};
        move_pause();
        add_msg_debug( debugmode::DF_NPC, "Canceling pickup - no items or new zone" );
        return;
    }

    // Check: Is the item owned? Has the situation changed since we last moved? Am 'I' now
    // standing in front of the shopkeeper/player that I am about to steal from?
    if( wanted_item ) {
        if( !::good_for_pickup( *wanted_item, *this, wanted_item_pos ) ) {
            add_msg_debug( debugmode::DF_NPC_ITEMAI,
                           "%s canceling pickup - situation changed since they decided to take item", get_name() );
            fetching_item = false;
            wanted_item = {};
            move_pause();
            return;
        }
    }

    add_msg_debug( debugmode::DF_NPC, "%s::pick_up_item(); [%s] => [%s]",
                   get_name(),
                   pos_bub().to_string_writable(), wanted_item_pos.to_string_writable() );
    if( const std::optional<tripoint_bub_ms> dest = nearest_passable( wanted_item_pos, pos_bub() ) ) {
        update_path( *dest );
    }

    const int dist_to_pickup = rl_dist( pos_bub(), wanted_item_pos );
    if( dist_to_pickup > 1 && !path.empty() ) {
        add_msg_debug( debugmode::DF_NPC, "Moving; [%s] => [%s]",
                       pos_bub().to_string_writable(), path[0].to_string_writable() );

        move_to_next();
        return;
    } else if( dist_to_pickup > 1 && path.empty() ) {
        add_msg_debug( debugmode::DF_NPC, "Can't find path" );
        // This can happen, always do something
        fetching_item = false;
        wanted_item = {};
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
        if( here.is_harvestable( wanted_item_pos ) ) {
            here.examine( *this, wanted_item_pos );
            // Note: we didn't actually pick up anything, just spawned items
            // but we want the item picker to find new items
            fetching_item = false;
            wanted_item = {};
            return;
        }
    }
    viewer &player_view = get_player_view();
    // Describe the pickup to the player
    bool u_see = player_view.sees( here, *this ) || player_view.sees( here, wanted_item_pos );
    if( u_see ) {
        if( picked_up.size() == 1 ) {
            add_msg( _( "%1$s picks up a %2$s." ), get_name(), picked_up.front().tname() );
        } else if( picked_up.size() == 2 ) {
            add_msg( _( "%1$s picks up a %2$s and a %3$s." ), get_name(), picked_up.front().tname(),
                     picked_up.back().tname() );
        } else if( picked_up.size() > 2 ) {
            add_msg( _( "%s picks up several items." ), get_name() );
        } else {
            add_msg( _( "%s looks around nervously, as if searching for something." ), get_name() );
        }
    }

    for( item &it : picked_up ) {
        int itval = value( it );
        if( itval < worst_item_value ) {
            worst_item_value = itval;
        }
        i_add( it );
        mod_moves( -get_speed() );
    }

    fetching_item = false;
    wanted_item = {};
    has_new_items = true;
}

template <typename T>
std::list<item> npc_pickup_from_stack( npc &who, T &items )
{
    std::list<item> picked_up;

    for( auto iter = items.begin(); iter != items.end(); ) {
        const item &it = *iter;
        if( who.can_take_that( it ) && who.wants_take_that( it ) ) {
            picked_up.push_back( it );
            iter = items.erase( iter );
        } else {
            ++iter;
        }
    }

    return picked_up;
}

bool npc::can_take_that( const item &it )
{
    bool good = false;

    auto weight_allowed = weight_capacity() - weight_carried();

    if( !it.made_of_from_type( phase_id::LIQUID ) && ( it.weight() <= weight_allowed ) &&
        can_stash( it ) ) {
        good = true;
    }

    return good;
}

bool npc::wants_take_that( const item &it )
{
    bool good = false;
    int min_value = minimum_item_value();
    const bool whitelisting = has_item_whitelist();

    item &weap = get_wielded_item() ? *get_wielded_item() : null_item_reference();
    if( ( ( !whitelisting && value( it ) > min_value ) || item_whitelisted( it ) ) ||
        weapon_value( it ) > weapon_value( weap ) ) {
        good = true;
    }

    return good;
}

bool npc::would_take_that( const item &it, const tripoint_bub_ms &p )
{
    const map &here = get_map();

    const bool is_stealing = !it.is_owned_by( *this, true );
    if( !is_stealing ) {
        return true;
    }
    Character &player = get_player_character();
    // Actual numeric relations are only relative to player faction
    if( it.is_owned_by( player ) ) {
        bool would_always_steal = false;
        int stealing_threshold = 10;
        // Trust = less likely to steal. Distrust? more likely!
        stealing_threshold += ( get_faction()->trusts_u / 5 );
        // We've already decided we want the item. So the primary motivator for stealing is aggression, not hoarding.
        stealing_threshold -= personality.aggression;
        stealing_threshold -= static_cast<int>( personality.collector / 3 );
        if( stealing_threshold < 0 ) {
            would_always_steal = true;
        }
        // Anyone willing to kill you no longer cares for your property rights
        if( has_faction_relationship( player, npc_factions::relationship::kill_on_sight ) ) {
            would_always_steal = true;
        }
        if( would_always_steal ) {
            add_msg_debug( debugmode::DF_NPC_ITEMAI, "%s attempting to steal %s (owned by player).", get_name(),
                           it.tname() );
            return true;
        }

        /*Handle player and follower vision*/
        viewer &player_view = get_player_view();
        if( player_view.sees( here, this->pos_bub( here ) ) || player_view.sees( here,  p ) ) {
            return false;
        }
        std::vector<npc *> followers;
        for( const character_id &elem : g->get_follower_list() ) {
            shared_ptr_fast<npc> npc_to_get = overmap_buffer.find_npc( elem );
            if( !npc_to_get ) {
                continue;
            }
            npc *npc_to_add = npc_to_get.get();
            followers.push_back( npc_to_add );
        }
        for( npc *&elem : followers ) {
            if( elem->sees( here, this->pos_bub( here ) ) || elem->sees( here,  p ) ) {
                return false;
            }
        }
        //Fallthrough, no consequences if you won't be caught!
        add_msg_debug( debugmode::DF_NPC_ITEMAI,
                       "%s attempting to steal %s (owned by player) because it isn't guarded.",
                       get_name(), it.tname() );
        return true;
    }


    // Currently always willing to steal from other NPCs
    return true;
}

std::list<item> npc::pick_up_item_map( const tripoint_bub_ms &where )
{
    map_stack stack = get_map().i_at( where );
    return npc_pickup_from_stack( *this, stack );
}

std::list<item> npc::pick_up_item_vehicle( vehicle &veh, int part_index )
{
    vehicle_stack stack = veh.get_items( veh.part( part_index ) );
    return npc_pickup_from_stack( *this, stack );
}

bool npc::find_corpse_to_pulp()
{
    map &here = get_map();

    Character &player_character = get_player_character();
    if( is_player_ally() ) {
        if( !rules.has_flag( ally_rule::allow_pulp ) ||
            player_character.in_vehicle || is_hallucination() ) {
            return false;
        }
        if( rl_dist( pos_bub(), player_character.pos_bub() ) >= mem_combat.engagement_distance ) {
            // don't start to pulp corpses if you're already far from the player.
            return false;
        }
    }

    // Pathing with overdraw can get expensive, limit it
    int path_counter = 4;
    const auto check_tile = [this, &path_counter, &here]( const tripoint_bub_ms & p ) -> const item * {
        if( !here.sees_some_items( p, *this ) || !sees( here, p ) )
        {
            return nullptr;
        }

        const map_stack items = here.i_at( p );
        const item *found = nullptr;
        for( const item &it : items )
        {
            if( it.can_revive() ) {
                const mtype &corpse = *it.get_corpse_mon();
                if( g->can_pulp_corpse( *this, corpse ) ) {
                    continue;
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

    const int range = mem_combat.engagement_distance;

    const item *corpse = nullptr;
    if( pulp_location && square_dist( pos_abs(), *pulp_location ) <= range ) {
        corpse = check_tile( here.get_bub( *pulp_location ) );
    }

    // Find the old target to avoid spamming
    const item *old_target = corpse;

    if( corpse == nullptr ) {
        // If we're following the player, don't wander off to pulp corpses
        const tripoint_bub_ms around = is_walking_with() ? player_character.pos_bub( here ) : pos_bub(
                                           here );
        for( const item_location &location : here.get_active_items_in_radius( around, range,
                special_item_type::corpse ) ) {
            corpse = check_tile( location.pos_bub( here ) );

            if( corpse != nullptr ) {
                pulp_location = location.pos_abs();
                break;
            }

            if( path_counter <= 0 ) {
                break;
            }
        }
    }

    if( corpse != nullptr && corpse != old_target && is_walking_with() ) {
        std::string talktag = chat_snippets().snip_pulp_zombie.translated();
        parse_tags( talktag, get_player_character(), *this );
        say( string_format( talktag, corpse->tname() ) );
    }

    return corpse != nullptr;
}

bool npc::can_do_pulp()
{
    if( !pulp_location ) {
        return false;
    }

    if( rl_dist( *pulp_location, pos_abs() ) > 1 || pulp_location->z() != posz() ) {
        return false;
    }
    return true;
}

bool npc::do_player_activity()
{
    int old_moves = moves;
    if( moves > 200 && activity && ( activity.is_multi_type() ||
                                     activity.id() == ACT_TIDY_UP ) ) {
        // a huge backlog of a multi-activity type can forever loop
        // instead; just scan the map ONCE for a task to do, and if it returns false
        // then stop scanning, abandon the activity, and kill the backlog of moves.
        if( !generic_multi_activity_handler( activity, *this->as_character(), true ) ) {
            revert_after_activity();
            set_moves( 0 );
            return true;
        }
    }
    // the multi-activity types can sometimes cancel the activity, and return without using up any moves.
    // ( when they are setting a destination etc. )
    // normally this isn't a problem, but in the main game loop, if the NPC has a huge backlog of moves;
    // then each of these occurrences will nudge the infinite loop counter up by one.
    // ( even if other move-using things occur inbetween )
    // so here - if no moves are used in a multi-type activity do_turn(), then subtract a nominal amount
    // to satisfy the infinite loop counter.
    const bool multi_type = activity ? activity.is_multi_type() : false;
    const int moves_before = moves;
    while( moves > 0 && activity ) {
        activity.do_turn( *this );
        if( !is_active() ) {
            return true;
        }
    }
    if( multi_type && moves == moves_before ) {
        mod_moves( -1 );
    }
    /* if the activity is finished, grab any backlog or change the mission */
    if( !has_destination() && !activity ) {
        // workaround: auto resuming craft activity may cause infinite loop
        while( !backlog.empty() && backlog.front().id() == ACT_CRAFT ) {
            backlog.pop_front();
        }
        if( !backlog.empty() ) {
            activity = backlog.front();
            backlog.pop_front();
            current_activity_id = activity.id();
        } else {
            if( is_player_ally() ) {
                add_msg( m_info, string_format( _( "%s completed the assigned task." ), disp_name() ) );
            }
            current_activity_id = activity_id::NULL_ID();
            revert_after_activity();
            // if we loaded after being out of the bubble for a while, we might have more
            // moves than we need, so clear them
            set_moves( 0 );
        }
    }
    return moves != old_moves;
}

double npc::evaluate_weapon( item &maybe_weapon, bool can_use_gun, bool use_silent ) const
{
    // Needed because evaluation includes electricity via linked cables.
    const map &here = get_map();

    bool allowed = can_use_gun && maybe_weapon.is_gun() && ( !use_silent || maybe_weapon.is_silent() );
    // According to unmodified evaluation score, NPCs almost always prioritize wielding guns if they have one.
    // This is relatively reasonable, as players can issue commands to NPCs when we do not want them to use ranged weapons.
    // Conversely, we cannot directly issue commands when we want NPCs to prioritize ranged weapons.
    // Note that the scoring method here is different from the 'weapon_value' used elsewhere.
    double val_gun = allowed ? gun_value( maybe_weapon, maybe_weapon.shots_remaining( here,
                                          this ) ) : 0;
    add_msg_debug( debugmode::DF_NPC_ITEMAI,
                   "%s %s valued at <color_light_cyan>%1.2f as a ranged weapon to wield</color>.",
                   disp_name( true ), maybe_weapon.type->get_id().str(), val_gun );
    double val_melee = melee_value( maybe_weapon );
    add_msg_debug( debugmode::DF_NPC_ITEMAI,
                   "%s %s valued at <color_light_cyan>%1.2f as a melee weapon to wield</color>.", disp_name( true ),
                   maybe_weapon.type->get_id().str(), val_melee );
    double val = std::max( val_gun, val_melee );
    return val;
}

item *npc::evaluate_best_weapon() const
{
    bool can_use_gun = !is_player_ally() || rules.has_flag( ally_rule::use_guns );
    bool use_silent = is_player_ally() && rules.has_flag( ally_rule::use_silent );

    item_location weapon = get_wielded_item();
    item &weap = weapon ? *weapon : null_item_reference();

    // Check if there's something better to wield
    item *best = &weap;
    double best_value = evaluate_weapon( weap, can_use_gun, use_silent );

    // To prevent changing to barely better stuff
    best_value *= std::max<float>( 1.0f, ai_cache.danger_assessment / 10.0f );

    // Fists aren't checked below
    double fist_value = evaluate_weapon( null_item_reference(), can_use_gun, use_silent );

    if( fist_value > best_value ) {
        best = &null_item_reference();
        best_value = fist_value;
    }

    //Now check through the NPC's inventory for melee weapons, guns, or holstered items
    visit_items( [this, can_use_gun, use_silent, &weap, &best_value, &best]( item * node, item * ) {
        if( can_wield( *node ).success() ) {
            double weapon_value = 0.0;
            bool using_same_type_bionic_weapon = is_using_bionic_weapon()
                                                 && node != &weap
                                                 && node->type->get_id() == weap.type->get_id();

            if( node->is_melee() || node->is_gun() ) {
                weapon_value = evaluate_weapon( *node, can_use_gun, use_silent );
                if( weapon_value > best_value && !using_same_type_bionic_weapon ) {
                    best = const_cast<item *>( node );
                    best_value = weapon_value;
                }
                return VisitResponse::SKIP;
            } else if( node->get_use( "holster" ) && !node->empty() ) {
                // we just recur to the next farther down
                return VisitResponse::NEXT;
            }
        }
        return VisitResponse::NEXT;
    } );

    return best;
}

bool npc::wield_better_weapon()
{
    // These are also assigned here so npc::evaluate_best_weapon() can be called by itself
    bool can_use_gun = !is_player_ally() || rules.has_flag( ally_rule::use_guns );
    bool use_silent = is_player_ally() && rules.has_flag( ally_rule::use_silent );

    item_location weapon = get_wielded_item();
    item &weap = weapon ? *weapon : null_item_reference();
    item *best = &weap;

    item *better_weapon = evaluate_best_weapon();

    if( best == better_weapon ) {
        add_msg_debug( debugmode::DF_NPC, "Wielded %s is best at %.1f, not switching",
                       best->type->get_id().str(),
                       evaluate_weapon( *better_weapon, can_use_gun, use_silent ) );
        return false;
    }

    add_msg_debug( debugmode::DF_NPC, "Wielding %s at value %.1f", better_weapon->type->get_id().str(),
                   evaluate_weapon( *better_weapon, can_use_gun, use_silent ) );

    // Always returns true, but future proof
    bool wield_success = wield( *better_weapon );
    if( !wield_success ) {
        debugmsg( "NPC failed to wield better weapon %s", better_weapon->tname() );
        return false;
    }
    return true;
}

bool npc::scan_new_items()
{
    add_msg_debug( debugmode::DF_NPC, "%s scanning new items", get_name() );
    if( wield_better_weapon() ) {
        return true;
    } else {
        // Stop "having new items" when you no longer do anything with them
        has_new_items = false;
    }

    return false;
    // TODO: Armor?
}

static void npc_throw( npc &np, item &it, const tripoint_bub_ms &pos )
{
    add_msg_if_player_sees( np, _( "%1$s throws a %2$s." ), np.get_name(), it.tname() );

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
        np.i_rem( &it );
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

    tripoint_bub_ms tar = critter->pos_bub();

    const int dist = rl_dist( pos_bub(), tar );
    item *used = nullptr;
    // Remember if we have an item that is dangerous to hold
    bool used_dangerous = false;

    // TODO: The active bomb with shortest fuse should be thrown first
    const auto check_alt_item = [&used, &used_dangerous, dist, this]( item * it ) {
        if( !it ) {
            return;
        }

        const bool dangerous = it->has_flag( flag_NPC_THROW_NOW );
        if( !dangerous && used_dangerous ) {
            return;
        }

        // Not alt attack
        if( !dangerous && !it->has_flag( flag_NPC_ALT_ATTACK ) ) {
            return;
        }

        // TODO: Non-thrown alt items
        if( !dangerous && throw_range( *it ) < dist ) {
            return;
        }

        // Low priority items
        if( !dangerous && used != nullptr ) {
            return;
        }

        used = it;
        used_dangerous = used_dangerous || dangerous;
    };

    check_alt_item( &*get_wielded_item() );
    const auto inv_all = items_with( []( const item & ) {
        return true;
    } );
    for( item *it : inv_all ) {
        // TODO: Cached values - an itype slot maybe?
        check_alt_item( it );
    }

    if( used == nullptr ) {
        return false;
    }

    // Are we going to throw this item?
    if( !used->active && used->has_flag( flag_NPC_ACTIVATE ) ) {
        activate_item( *used );
        // Note: intentional lack of return here
        // We want to ignore player-centric rules to avoid carrying live explosives
        // TODO: Non-grenades
    }

    // We are throwing it!
    int conf = confident_throw_range( *used, critter );
    const bool wont_hit = wont_hit_friend( tar, *used, true );
    if( dist <= conf && wont_hit ) {
        npc_throw( *this, *used, tar );
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

    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    // We need to throw this live (grenade, etc) NOW! Pick another target?
    for( int dist = 2; dist <= conf; dist++ ) {
        for( const tripoint_bub_ms &pt : here.points_in_radius( pos_bub(), dist ) ) {
            const monster *const target_ptr = creatures.creature_at<monster>( pt );
            int newdist = rl_dist( pos_bub(), pt );
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
        for( const tripoint_bub_ms &pt : here.points_in_radius( pos_bub(), dist ) ) {
            int new_dist = rl_dist( pos_bub(), pt );
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
    npc_throw( *this, *used, tar );
    return true;
}

void npc::activate_item( item &it )
{
    const int oldmoves = moves;
    if( it.is_tool() || it.is_food() ) {
        it.type->invoke( this, it, pos_bub() );
    }

    if( moves == oldmoves ) {
        // HACK: A hack to prevent debugmsgs when NPCs activate 0 move items
        // while not removing the debugmsgs for other 0 move actions
        moves--;
    }
}

void npc::heal_player( Character &patient )
{
    const map &here = get_map();

    // Avoid more than one first aid activity at a time.
    if( Character::has_activity( ACT_FIRSTAID ) ) {
        return;
    }

    int dist = rl_dist( pos_abs(), patient.pos_abs() );

    if( dist > 1 ) {
        // We need to move to the player
        update_path( patient.pos_bub() );
        move_to_next();
        return;
    }

    viewer &player_view = get_player_view();
    // Close enough to heal!
    bool u_see = player_view.sees( here, *this ) || player_view.sees( here, patient );
    if( u_see ) {
        add_msg( _( "%1$s starts healing %2$s." ), disp_name(), patient.disp_name() );
    }

    item &used = get_healing_item( ai_cache.can_heal );
    if( used.is_null() ) {
        debugmsg( "%s tried to heal you but has no healing item", disp_name() );
        return;
    }
    if( !is_hallucination() ) {
        int charges_used = used.type->invoke( this, used, patient.pos_bub(), "heal" ).value_or( 0 );
        consume_charges( used, charges_used );
    } else {
        pretend_heal( patient, used );
    }

}

void npc::pretend_heal( Character &patient, item used )
{
    // you can tell that it's not real by looking at your HP though
    add_msg_if_player_sees( *this, _( "%1$s heals %2$s." ), disp_name(),
                            patient.disp_name() );
    consume_charges( used, 1 ); // empty hallucination's inventory to avoid spammming
    mod_moves( -get_speed() ); // consumes moves to avoid infinite loop
}

void npc::heal_self()
{
    if( has_effect( effect_asthma ) ) {
        item *treatment = nullptr;
        std::string iusage = "INHALER";

        const auto filter_use = [this]( const std::string & filter ) -> std::vector<item *> {
            std::vector<item *> inv_filtered = items_with( [&filter]( const item & itm )
            {
                return ( itm.type->get_use( filter ) != nullptr ) && itm.ammo_sufficient( nullptr );
            } );
            return inv_filtered;
        };

        const std::vector<item *> inv_inhalers = filter_use( iusage );

        for( item *inhaler : inv_inhalers ) {
            if( treatment == nullptr || treatment->ammo_remaining( ) > inhaler->ammo_remaining( ) ) {
                treatment = inhaler;
            }
        }

        if( treatment == nullptr ) {
            iusage = "OXYGEN_BOTTLE";
            const std::vector<item *> inv_oxybottles = filter_use( iusage );

            for( item *oxy_bottle : inv_oxybottles ) {
                if( treatment == nullptr ||
                    treatment->ammo_remaining( ) > oxy_bottle->ammo_remaining( ) ) {
                    treatment = oxy_bottle;
                }
            }
        }

        if( treatment != nullptr ) {
            treatment->get_use( iusage )->call( this, *treatment, pos_bub() );
            treatment->ammo_consume( treatment->ammo_required(), pos_bub(), this );
            return;
        }
    }

    // Avoid more than one first aid activity at a time.
    if( Character::has_activity( ACT_FIRSTAID ) ) {
        return;
    }

    item &used = get_healing_item( ai_cache.can_heal );
    if( used.is_null() ) {
        debugmsg( "%s tried to heal self but has no healing item", disp_name() );
        return;
    }

    add_msg_if_player_sees( *this, _( "%1$s starts applying a %2$s." ), disp_name(), used.tname() );
    warn_about( "heal_self", 1_turns );

    int charges_used = used.type->invoke( this, used, pos_bub(), "heal" ).value_or( 0 );
    if( used.is_medication() && charges_used > 0 ) {
        consume_charges( used, charges_used );
    }
}

void npc::use_painkiller()
{
    // First, find the best painkiller for our pain level
    item *it = inv->most_appropriate_painkiller( get_pain() );

    if( it->is_null() ) {
        debugmsg( "NPC tried to use painkillers, but has none!" );
        move_pause();
    } else {
        add_msg_if_player_sees( *this, _( "%1$s takes some %2$s." ), disp_name(), it->tname() );
        item_location loc = item_location( *this, it );
        const time_duration &consume_time = get_consume_time( *loc );
        mod_moves( -to_moves<int>( consume_time ) );
        consume( loc );
    }
}

// We want our food to:
// Provide enough nutrition and quench
// Not provide too much of either (don't waste food)
// Not be unhealthy
// Not have side effects
// Be eaten before it rots (favor soon-to-rot perishables)
//
// TODO: Cache the results of this, *especially* if there's nothing we want to eat.
static float rate_food( const item &it, int want_nutr, int want_quench )
{
    const auto &food = it.get_comestible();
    if( !food ) {
        return 0.0f;
    }

    // Don't eat it if it's filled with parasites
    if( food->parasites && !it.has_flag( flag_NO_PARASITES ) ) {
        return 0.0;
    }

    // TODO: Use the actual nutrition for this food, rather than the default?
    int nutr = food->get_default_nutr();
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

    // Don't eat rotten food.
    if( relative_rot >= 1.0f ) {
        // TODO: Allow sapro mutants to eat it anyway and make them prefer it
        return 0.0f;
    }

    // For non-rotten food, we have a starting weight in the range 1-10
    // The closer it is to expiring, the more we should aim to eat it.
    float weight = std::max( 1.0, 10.0 * relative_rot );

    // TODO: I feel like we should exclude *really* un-fun foods (flour, hot sauce, etc)
    //       rather than discount them. Eating cooked liver is fine, eating raw flour... :/
    //       Likewise, *fun* foods should be boosted in attractiveness.
    if( it.get_comestible_fun() < 0 ) {
        // This helps to avoid eating stuff like flour
        weight /= ( -it.get_comestible_fun() ) + 1;
    }

    // NPCs will avoid unhealthy foods.
    if( food->healthy < 0 ) {
        weight /= ( -food->healthy ) + 1;
    }

    // Avoid wasting quench values unless it's about to rot away
    if( relative_rot < 0.9f && quench > want_quench ) {
        // TODO: This can remove a food as a candidate entirely, which means
        //       an NPC could avoid eating a slightly hydrating but calorie-dense
        //       food because they're "not thirsty".
        weight -= ( 1.0f - relative_rot ) * ( quench - want_quench );
    }

    if( quench < 0 && want_quench > 0 && want_nutr < want_quench ) {
        // Avoid stuff that makes us thirsty when we're more thirsty than hungry
        weight = weight * want_nutr / want_quench;
    }

    if( nutr > want_nutr ) {

        // Discount fresh foods that are bigger than our hunger.
        if( relative_rot < 0.9f ) {
            weight /= nutr - want_nutr;
        }
    }

    // NPCs won't eat poison food unless it's only a little poisoned
    if( it.poison > 0 ) {
        weight -= it.poison;
    }

    return weight;
}

bool npc::consume_food_from_camp()
{
    Character &player_character = get_player_character();
    std::optional<basecamp *> potential_bc;
    for( const tripoint_abs_omt &camp_pos : player_character.camps ) {
        if( rl_dist( camp_pos.xy(), pos_abs_omt().xy() ) < 3 ) {
            potential_bc = overmap_buffer.find_camp( camp_pos.xy() );
            if( potential_bc ) {
                break;
            }
        }
    }
    if( !potential_bc ) {
        return false;
    }
    basecamp *bcp = *potential_bc;

    // Handle water
    if( get_thirst() > 40 && bcp->has_water() && bcp->allowed_access_by( *this, true ) ) {
        complain_about( "camp_water_thanks", 1_hours,
                        chat_snippets().snip_camp_water_thanks.translated(), false );
        // TODO: Stop skipping the stomach for this, actually put the water in there.
        set_thirst( 0 );
        return true;
    }

    // Handle food
    int current_kcals = get_stored_kcal() + stomach.get_calories() + guts.get_calories();
    int kcal_threshold = get_healthy_kcal() * 19 / 20;
    if( get_hunger() > 0 && current_kcals < kcal_threshold && bcp->allowed_access_by( *this ) ) {
        // Try to eat a bit more than the bare minimum so that we're not eating every 5 minutes
        // but also don't try to eat a week's worth of food in one sitting
        int desired_kcals = std::min( static_cast<int>( base_metabolic_rate ), std::max( 0,
                                      kcal_threshold + 100 - current_kcals ) );
        int kcals_to_eat = std::min( desired_kcals, bcp->get_owner()->food_supply.kcal() );

        if( kcals_to_eat > 0 ) {
            bcp->feed_workers( *this, bcp->camp_food_supply( -kcals_to_eat ) );

            return true;
        } else {
            // We need food but there's none to eat :(
            complain_about( "camp_larder_empty", 1_hours,
                            chat_snippets().snip_camp_larder_empty.translated(), false );
            return false;
        }
    }

    return false;
}

bool npc::consume_food()
{
    float best_weight = 0.0f;
    item *best_food = nullptr;
    bool consumed = false;
    int want_hunger = std::max( 0, get_hunger() );
    int want_quench = std::max( 0, get_thirst() );

    const std::vector<item *> inv_food = cache_get_items_with( "is_food", &item::is_food );

    if( inv_food.empty() ) {
        if( !needs_food() ) {
            // TODO: Remove this and let player "exploit" hungry NPCs
            set_hunger( 0 );
            set_thirst( 0 );
        }
    } else {
        for( item * const &food_item : inv_food ) {
            float cur_weight = rate_food( *food_item, want_hunger, want_quench );
            // Note: will_eat is expensive, avoid calling it if possible
            if( cur_weight > best_weight && will_eat( *food_item ).success() ) {
                best_weight = cur_weight;
                best_food = food_item;
            }
        }

        // consume doesn't return a meaningful answer, we need to compare moves
        if( best_food != nullptr ) {
            const time_duration &consume_time = get_consume_time( *best_food );
            consumed = consume( item_location( *this, best_food ) ) != trinary::NONE;
            if( consumed ) {
                // TODO: Message that "X begins eating Y?" Right now it appears to the player
                //       that "Urist eats a carp roast" and then stands still doing nothing
                //       for a while.
                mod_moves( -to_moves<int>( consume_time ) );
            } else {
                debugmsg( "%s failed to consume %s", get_name(), best_food->tname() );
            }
        }

    }

    return consumed;
}

void npc::mug_player( Character &mark )
{
    const map &here = get_map();

    if( mark.is_armed() ) {
        make_angry();
    }

    if( rl_dist( pos_bub(), mark.pos_bub() ) > 1 ) { // We have to travel
        update_path( mark.pos_bub() );
        move_to_next();
        return;
    }

    Character &player_character = get_player_character();
    const bool u_see = player_character.sees( here, *this ) || player_character.sees( here, mark );
    if( mark.cash > 0 ) {
        if( !is_hallucination() ) { // hallucinations can't take items
            cash += mark.cash;
            mark.cash = 0;
        }
        set_moves( 0 );
        // Describe the action
        if( mark.is_npc() ) {
            if( u_see ) {
                add_msg( _( "%1$s takes %2$s's money!" ), get_name(), mark.get_name() );
            }
        } else {
            add_msg( m_bad, _( "%s takes your money!" ), get_name() );
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
    item *to_steal = nullptr;
    std::vector<const item *> pseudo_items = mark.get_pseudo_items();
    const auto inv_valuables = mark.items_with( [this, pseudo_items]( const item & itm ) {
        return std::find( pseudo_items.begin(), pseudo_items.end(), &itm ) == pseudo_items.end() &&
               !itm.has_flag( flag_INTEGRATED ) && !itm.has_flag( flag_NO_TAKEOFF ) && value( itm ) > 0;
    } );
    for( item *it : inv_valuables ) {
        item &front_stack = *it; // is this safe?
        if( value( front_stack ) >= best_value &&
            can_pickVolume( front_stack, true ) &&
            can_pickWeight( front_stack, true ) ) {
            best_value = value( front_stack );
            to_steal = &front_stack;
        }
    }
    if( to_steal == nullptr ) { // Didn't find anything worthwhile!
        set_attitude( NPCATT_FLEE_TEMP );
        if( !one_in( 3 ) ) {
            say( chat_snippets().snip_done_mugging.translated() );
        }
        mod_moves( -get_speed() );
        return;
    }
    item stolen;
    if( !is_hallucination() ) {
        stolen = mark.i_rem( to_steal );
        i_add( stolen );
    }
    if( mark.is_npc() ) {
        if( u_see ) {
            add_msg( _( "%1$s takes %2$s's %3$s." ), get_name(), mark.get_name(), stolen.tname() );
        }
    } else {
        add_msg( m_bad, _( "%1$s takes your %2$s." ), get_name(), stolen.tname() );
    }
    mod_moves( -get_speed() );
    if( !mark.is_npc() ) {
        op_of_u.value -= rng( 0, 1 );  // Decrease the value of the player
    }
}

void npc::look_for_player( const Character &sought )
{
    complain_about( "look_for_player", 5_minutes, chat_snippets().snip_wait.translated(), false );
    update_path( sought.pos_bub() );
    move_to_next();
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
    return last_player_seen_pos && get_map().inbounds( *last_player_seen_pos ) &&
           last_seen_player_turn > 0;
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
    map &here = get_map();
    if( is_travelling() ) {
        guard_pos = pos_abs();
        goal = no_goal_point;
        if( is_player_ally() ) {
            Character &player_character = get_player_character();
            talk_function::assign_guard( *this );
            if( rl_dist( player_character.pos_bub(), pos_bub() ) > SEEX * 2 ) {
                if( player_character.cache_has_item_with_flag( flag_TWO_WAY_RADIO, true ) &&
                    cache_has_item_with_flag( flag_TWO_WAY_RADIO, true ) ) {
                    add_msg_if_player_sees( pos_bub(), m_info, _( "From your two-way radio you hear %s reporting in, "
                                            "'I've arrived, boss!'" ), disp_name() );
                }
            }
        } else {
            // for now - they just travel to a nearby place they want as a base
            // and chill there indefinitely, the plan is to add systems for them to build
            // up their base, then go out on looting missions,
            // then return to base afterwards.
            set_mission( NPC_MISSION_GUARD );
            if( !needs.empty() && needs[0] == need_safety ) {
                // we found our base.
                base_location = pos_abs_omt();
            }
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
    goal = pos_abs_omt();
    if( guard_pos && *guard_pos == pos_abs() ) {
        // This is the specific point
        return;
    }

    if( path.size() > 1 ) {
        // No point recalculating the path to get home
        move_to_next();
    } else if( guard_pos ) {
        update_path( here.get_bub( *guard_pos ) );
        move_to_next();
    } else {
        guard_pos = pos_abs();
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

    tripoint_abs_omt surface_omt_loc = pos_abs_omt();
    // We need that, otherwise find_closest won't work properly
    surface_omt_loc.z() = 0;

    decide_needs();
    if( needs.empty() ) { // We don't need anything in particular.
        needs.push_back( need_none );

        // also, don't bother looking if the CITY_SIZE is 0, just go somewhere at random
        const int city_size = get_option<int>( "CITY_SIZE" );
        if( city_size == 0 ) {
            goal = surface_omt_loc + point( rng( -90, 90 ), rng( -90, 90 ) );
            return;
        }
    }

    std::string dest_type;
    for( const npc_need &fulfill : needs ) {
        auto cache_iter = goal_cache.find( fulfill );
        if( cache_iter != goal_cache.end() && cache_iter->second.omt_loc == surface_omt_loc ) {
            goal = cache_iter->second.goal;
        } else {
            // look for the closest occurrence of any of that locations terrain types
            omt_find_params find_params;
            for( const oter_type_str_id &elem : get_location_for( fulfill )->get_all_terrains() ) {
                std::pair<std::string, ot_match_type> temp_pair;
                temp_pair.first = elem.str();
                temp_pair.second = ot_match_type::type;
                find_params.types.push_back( temp_pair );
            }
            // note: no shuffle of `find_params.types` is needed, because `find_closest`
            // disregards `types` order anyway, and already returns random result among
            // those having equal minimal distance
            find_params.search_range = 75;
            find_params.existing_only = false;
            // force finding goal on the same zlevel (for random spawned NPCs that's z=0), otherwise
            // we may target unreachable overmap tiles (no ramp up/down) which makes
            // overmap_buffer.get_travel_path waste a lot of time
            find_params.min_z = surface_omt_loc.z();
            find_params.max_z = surface_omt_loc.z();
            goal = overmap_buffer.find_closest( surface_omt_loc, find_params );
            npc_need_goal_cache &cache = goal_cache[fulfill];
            cache.goal = goal;
            cache.omt_loc = surface_omt_loc;
        }
        omt_path.clear();
        if( !goal.is_invalid() ) {
            omt_path = overmap_buffer.get_travel_path( surface_omt_loc, goal,
                       overmap_path_params::for_npc() ).points;
        }
        if( !omt_path.empty() ) {
            dest_type = overmap_buffer.ter( goal )->get_type_id().str();
            break;
        }
    }

    // couldn't find any places to go, so go somewhere.
    if( goal.is_invalid() || omt_path.empty() ) {
        goal = surface_omt_loc + point( rng( -90, 90 ), rng( -90, 90 ) );
        omt_path = overmap_buffer.get_travel_path( surface_omt_loc, goal,
                   overmap_path_params::for_npc() ).points;
        // try one more time
        if( omt_path.empty() ) {
            goal = surface_omt_loc + point( rng( -90, 90 ), rng( -90, 90 ) );
            omt_path = overmap_buffer.get_travel_path( surface_omt_loc, goal,
                       overmap_path_params::for_npc() ).points;
        }
        if( omt_path.empty() ) {
            goal = no_goal_point;
        }
        return;
    }

    DebugLog( D_INFO, DC_ALL ) << "npc::set_omt_destination - new goal for NPC [" << get_name() <<
                               "] with [" << get_need_str_id( needs.front() ) <<
                               "] is [" << dest_type <<
                               "] in " << goal.to_string() << ".";
}

void npc::go_to_omt_destination()
{
    map &here = get_map();
    if( ai_cache.guard_pos ) {
        if( pos_abs() == *ai_cache.guard_pos ) {
            path.clear();
            ai_cache.guard_pos = std::nullopt;
            move_pause();
            return;
        }
    }
    if( goal == no_goal_point || omt_path.empty() ) {
        add_msg_debug( debugmode::DF_NPC, "npc::go_to_destination with no goal" );
        move_pause();
        reach_omt_destination();
        return;
    }
    const tripoint_abs_omt omt_pos = pos_abs_omt();
    if( goal == omt_pos ) {
        // We're at our desired map square!  Pause to keep the NPC infinite loop counter happy
        move_pause();
        reach_omt_destination();
        return;
    }
    if( !path.empty() ) {
        // we already have a path, just use that until we can't.
        move_to_next();
        return;
    }
    // get the next path point
    if( omt_path.back() == omt_pos ) {
        // this should be the square we are at.
        omt_path.pop_back();
    }
    if( !omt_path.empty() ) {
        point_rel_omt omt_diff = omt_path.back().xy() - omt_pos.xy();
        if( omt_diff.x() > 3 || omt_diff.x() < -3 || omt_diff.y() > 3 || omt_diff.y() < -3 ) {
            // we've gone wandering somehow, reset destination.
            if( !is_player_ally() ) {
                set_omt_destination();
            } else {
                talk_function::assign_guard( *this );
            }
            return;
        }
    }
    tripoint_bub_ms sm_tri = here.get_bub( project_to<coords::ms>( omt_path.back() ) );
    tripoint_bub_ms centre_sub = sm_tri + point( SEEX, SEEY );
    path = here.route( *this, pathfinding_target::radius( centre_sub, 2 ) );
    add_msg_debug( debugmode::DF_NPC, "%s going %s->%s", get_name(), omt_pos.to_string_writable(),
                   goal.to_string_writable() );

    if( !path.empty() ) {
        move_to_next();
        return;
    }
    move_pause();
}

void npc::guard_current_pos()
{
    goal = pos_abs_omt();
    guard_pos = pos_abs();
}

std::string npc_action_name( npc_action action )
{
    switch( action ) {
        case npc_undecided:
            return "Undecided";
        case npc_pause:
            return "Pause";
        case npc_worker_downtime:
            return "Relaxing";
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
        case npc_noop:
            return "Do nothing";
        case npc_do_attack:
            return "Attack";
        case npc_goto_to_this_pos:
            return "Go to position";
        case num_npc_actions:
            return "Unnamed action";
    }

    return "Unnamed action";
}

void print_action( const char *prepend, npc_action action )
{
    if( action != npc_undecided ) {
        add_msg_debug( debugmode::DF_NPC, prepend, npc_action_name( action ) );
    }
}

const Creature *npc::current_target() const
{
    // TODO: Arguably we should return a shared_ptr to ensure that the returned
    // object stays alive while the caller uses it.  Not doing that for now.
    return ai_cache.target.lock().get();
}

// NOLINTNEXTLINE(readability-make-member-function-const)
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

// NOLINTNEXTLINE(readability-make-member-function-const)
Creature *npc::current_ally()
{
    // TODO: As above.
    return ai_cache.ally.lock().get();
}

// Maybe TODO: Move to Character method and use map methods
static bodypart_id bp_affected( npc &who, const efftype_id &effect_type )
{
    bodypart_id ret;
    int highest_intensity = INT_MIN;
    for( const bodypart_id &bp : who.get_all_body_parts() ) {
        const effect &eff = who.get_effect( effect_type, bp );
        if( !eff.is_null() && eff.get_intensity() > highest_intensity ) {
            ret = bp;
            highest_intensity = eff.get_intensity();
        }
    }

    return ret;
}

std::string npc::distance_string( int range ) const
{
    if( range < 6 ) {
        return chat_snippets().snip_danger_close_distance.translated();
    } else if( range < 11 ) {
        return chat_snippets().snip_close_distance.translated();
    } else if( range < 26 ) {
        return chat_snippets().snip_medium_distance.translated();
    } else {
        return chat_snippets().snip_far_distance.translated();
    }
}

void npc::warn_about( const std::string &type, const time_duration &d, const std::string &name,
                      int range, const tripoint_bub_ms &danger_pos )
{
    std::string snip;
    sounds::sound_t spriority = sounds::sound_t::alert;
    if( type == "monster" ) {
        snip = is_enemy() ? chat_snippets().snip_monster_warning_h.translated()
               : chat_snippets().snip_monster_warning.translated();
    } else if( type == "explosion" ) {
        snip = is_enemy() ? chat_snippets().snip_fire_in_the_hole_h.translated()
               : chat_snippets().snip_fire_in_the_hole.translated();
    } else if( type == "general_danger" ) {
        snip = is_enemy() ? chat_snippets().snip_general_danger_h.translated()
               : chat_snippets().snip_general_danger.translated();
        spriority = sounds::sound_t::speech;
    } else if( type == "relax" ) {
        snip = is_enemy() ? chat_snippets().snip_its_safe_h.translated()
               : chat_snippets().snip_its_safe.translated();
        spriority = sounds::sound_t::speech;
    } else if( type == "kill_npc" ) {
        snip = is_enemy() ? chat_snippets().snip_kill_npc_h.translated()
               : chat_snippets().snip_kill_npc.translated();
    } else if( type == "kill_player" ) {
        snip = is_enemy() ? chat_snippets().snip_kill_player_h.translated() : "";
    } else if( type == "run_away" ) {
        snip = chat_snippets().snip_run_away.translated();
    } else if( type == "cant_flee" ) {
        snip = chat_snippets().snip_cant_flee.translated();
    } else if( type == "fire_bad" ) {
        snip = chat_snippets().snip_fire_bad.translated();
    } else if( type == "speech_noise" && !has_trait( trait_IGNORE_SOUND ) ) {
        snip = chat_snippets().snip_speech_warning.translated();
        spriority = sounds::sound_t::speech;
    } else if( type == "combat_noise" && !has_trait( trait_IGNORE_SOUND ) ) {
        snip = chat_snippets().snip_combat_noise_warning.translated();
        spriority = sounds::sound_t::speech;
    } else if( type == "movement_noise" && !has_trait( trait_IGNORE_SOUND ) ) {
        snip = chat_snippets().snip_movement_noise_warning.translated();
        spriority = sounds::sound_t::speech;
    } else if( type == "heal_self" ) {
        snip = chat_snippets().snip_heal_self.translated();
        spriority = sounds::sound_t::speech;
    } else {
        return;
    }
    const std::string warning_name = "warning_" + type + name;
    if( name.empty() ) {
        complain_about( warning_name, d, snip, is_enemy(), spriority );
    } else {
        const std::string range_str = range < 1 ? "<punc>" :
                                      string_format( _( " %s, %s" ),
                                              direction_name( direction_from( pos_bub(), danger_pos ) ),
                                              distance_string( range ) );
        const std::string speech = string_format( _( "%s %s%s" ), snip, _( name ), range_str );
        complain_about( warning_name, d, speech, is_enemy(), spriority );
    }
}

bool npc::complain_about( const std::string &issue, const time_duration &dur,
                          const std::string &speech, const bool force, const sounds::sound_t priority )
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
                                        !get_player_character().in_sleep_state() && !in_sleep_state() );

    if( complain_since( issue, dur ) && do_complain ) {
        say( speech, priority );
        set_complain_since( issue );
        return true;
    }
    return false;
}

bool npc::complain()
{
    const map &here = get_map();

    static const std::string infected_string = "infected";
    static const std::string sleepiness_string = "sleepiness";
    static const std::string bite_string = "bite";
    static const std::string bleed_string = "bleed";
    static const std::string hypovolemia_string = "hypovolemia";
    static const std::string radiation_string = "radiation";
    static const std::string hunger_string = "hunger";
    static const std::string thirst_string = "thirst";

    if( !is_player_ally() || !get_player_view().sees( here, *this ) ) {
        return false;
    }

    // When infected, complain every (4-intensity) hours
    // At intensity 3, ignore player wanting us to shut up
    if( has_effect( effect_infected ) ) {
        const bodypart_id &bp =  bp_affected( *this, effect_infected );
        const effect &eff = get_effect( effect_infected, bp );
        int intensity = eff.get_intensity();
        std::string talktag = chat_snippets().snip_wound_infected.translated();
        parse_tags( talktag, get_player_character(), *this );
        const std::string speech = string_format( talktag, body_part_name( bp ) );
        if( complain_about( infected_string, time_duration::from_hours( 4 - intensity ), speech,
                            intensity >= 3 ) ) {
            // Only one complaint per turn
            return true;
        }
    }

    // When bitten, complain every hour, but respect restrictions
    if( has_effect( effect_bite ) ) {
        const bodypart_id &bp =  bp_affected( *this, effect_bite );
        std::string talktag = chat_snippets().snip_wound_bite.translated();
        parse_tags( talktag, get_player_character(), *this );
        const std::string speech = string_format( talktag, body_part_name( bp ) );
        if( complain_about( bite_string, 1_hours, speech ) ) {
            return true;
        }
    }

    // When tired, complain every 30 minutes
    // If massively tired, ignore restrictions
    if( get_sleepiness() > sleepiness_levels::TIRED &&
        complain_about( sleepiness_string, 30_minutes, chat_snippets().snip_yawn.translated(),
                        get_sleepiness() > sleepiness_levels::MASSIVE_SLEEPINESS - 100 ) )  {
        return true;
    }

    // Radiation every 10 minutes
    if( get_rad() > 90 ) {
        activate_bionic_by_id( bio_radscrubber );
        std::string speech = chat_snippets().snip_radiation_sickness.translated();
        if( complain_about( radiation_string, 10_minutes, speech, get_rad() > 150 ) ) {
            return true;
        }
    } else if( !get_rad() ) {
        deactivate_bionic_by_id( bio_radscrubber );
    }

    // Hunger every 3-6 hours
    // Since NPCs can't starve to death, respect the rules
    if( get_hunger() > NPC_HUNGER_COMPLAIN &&
        complain_about( hunger_string,
                        std::max( 3_hours, time_duration::from_minutes( 60 * 8 - get_hunger() ) ),
                        chat_snippets().snip_hungry.translated() ) ) {
        return true;
    }

    // Thirst every 2 hours
    // Since NPCs can't dry to death, respect the rules
    if( get_thirst() > NPC_THIRST_COMPLAIN &&
        complain_about( thirst_string, 2_hours, chat_snippets().snip_thirsty.translated() ) ) {
        return true;
    }

    //Bleeding every 5 minutes
    if( has_effect( effect_bleed ) ) {
        const bodypart_id &bp =  bp_affected( *this, effect_bleed );
        std::string speech;
        time_duration often;
        if( get_effect( effect_bleed, bp ).get_intensity() < 10 ) {
            std::string talktag = chat_snippets().snip_bleeding.translated();
            parse_tags( talktag, get_player_character(), *this );
            speech = string_format( talktag, body_part_name( bp ) );
            often = 5_minutes;
        } else {
            std::string talktag = chat_snippets().snip_bleeding_badly.translated();
            parse_tags( talktag, get_player_character(), *this );
            speech = string_format( talktag, body_part_name( bp ) );
            often = 1_minutes;
        }
        if( complain_about( bleed_string, often, speech ) ) {
            return true;
        }
    }

    if( has_effect( effect_hypovolemia ) ) {
        std::string speech = chat_snippets().snip_lost_blood.translated();
        if( complain_about( hypovolemia_string, 10_minutes, speech ) ) {
            return true;
        }
    }

    return false;
}

void npc::do_reload( const item_location &it )
{
    const map &here = get_map();

    if( !it ) {
        debugmsg( "do_reload failed: %s tried to reload a none", name );
        return;
    }

    item::reload_option reload_opt = select_ammo( it );

    if( !reload_opt ) {
        debugmsg( "do_reload failed: no usable ammo for %s", it->tname() );
        return;
    }

    // Note: we may be reloading the magazine inside, not the gun itself
    // Maybe TODO: allow reload functions to understand such reloads instead of const casts
    item &target = const_cast<item &>( *reload_opt.target );
    item_location &usable_ammo = reload_opt.ammo;

    int qty = std::max( 1, std::min( usable_ammo->charges,
                                     it->ammo_capacity( usable_ammo->ammo_data()->ammo->type ) - it->ammo_remaining( ) ) );
    int reload_time = item_reload_cost( *it, *usable_ammo, qty );
    // TODO: Consider printing this info to player too
    const std::string ammo_name = usable_ammo->tname();
    if( !target.reload( *this, std::move( usable_ammo ), qty ) ) {
        debugmsg( "do_reload failed: item %s could not be reloaded with %ld charge(s) of %s",
                  it->tname(), qty, ammo_name );
        return;
    }

    mod_moves( -reload_time );
    recoil = MAX_RECOIL;

    if( get_player_view().sees( here, *this ) ) {
        add_msg( _( "%1$s reloads their %2$s." ), get_name(), it->tname() );
        sfx::play_variant_sound( "reload", it->typeId().str(), sfx::get_heard_volume( pos_bub() ),
                                 sfx::get_heard_angle( pos_bub() ) );
    }

    // Otherwise the NPC may not equip the weapon until they see danger
    has_new_items = true;
}

bool npc::adjust_worn()
{
    bool any_broken = false;
    for( const bodypart_id &bp : get_all_body_parts() ) {
        if( is_limb_broken( bp ) ) {
            any_broken = true;
            break;
        }
    }

    if( !any_broken ) {
        return false;
    }

    return worn.adjust_worn( *this );
}

bool outfit::adjust_worn( npc &guy )
{
    const auto covers_broken = [&guy]( const item & it, side s ) {
        const body_part_set covered = it.get_covered_body_parts( s );
        for( const std::pair<const bodypart_str_id, bodypart> &elem : guy.get_body() ) {
            if( elem.second.get_hp_cur() <= 0 && covered.test( elem.first ) ) {
                return true;
            }
        }
        return false;
    };

    for( item &elem : worn ) {
        if( !elem.has_flag( flag_SPLINT ) ) {
            continue;
        }

        if( !covers_broken( elem, elem.get_side() ) ) {
            const bool needs_change = covers_broken( elem, opposite_side( elem.get_side() ) );
            //create an item_location for takeoff() to handle.
            item_location loc_for_takeoff = item_location( guy, &elem );
            // Try to change side (if it makes sense), or take off.
            std::list<item> temp_list;
            if( ( needs_change && guy.change_side( elem ) ) || takeoff( loc_for_takeoff, &temp_list, guy ) ) {
                return true;
            }
        }
    }
    return false;
}

void npc::set_movement_mode( const move_mode_id &new_mode )
{
    // Enchantments based on move modes can stack inappropriately without a recalc here
    recalculate_enchantment_cache();
    move_mode = new_mode;
}
