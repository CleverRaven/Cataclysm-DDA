#include <sstream>
#include "npc.h"
#include "rng.h"
#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "line.h"
#include "debug.h"
#include "overmapbuffer.h"
#include "messages.h"
#include "translations.h"
#include "veh_type.h"
#include "monster.h"
#include "itype.h"
#include "vehicle.h"
#include "mtype.h"
#include "field.h"
#include "sounds.h"

#define dbg(x) DebugLog((DebugLevel)(x),D_NPC) << __FILE__ << ":" << __LINE__ << ": "
#define TARGET_NONE INT_MIN
#define TARGET_PLAYER -2

const skill_id skill_firstaid( "firstaid" );
const skill_id skill_gun( "gun" );
const skill_id skill_throw( "throw" );

const efftype_id effect_bleed( "bleed" );
const efftype_id effect_bite( "bite" );
const efftype_id effect_bouldering( "bouldering" );
const efftype_id effect_catch_up( "catch_up" );
const efftype_id effect_hit_by_player( "hit_by_player" );
const efftype_id effect_infection( "infection" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_lying_down( "lying_down" );
const efftype_id effect_stunned( "stunned" );

// A list of items used for escape, in order from least to most valuable
#ifndef NUM_ESCAPE_ITEMS
#define NUM_ESCAPE_ITEMS 11
itype_id ESCAPE_ITEMS[NUM_ESCAPE_ITEMS] = {
    "cola", "caffeine", "energy_drink", "canister_goo", "smokebomb",
    "smokebomb_act", "adderall", "coke", "meth", "teleporter",
    "pheromone"
};
#endif

// A list of alternate attack items (e.g. grenades), from least to most valuable
#ifndef NUM_ALT_ATTACK_ITEMS
#define NUM_ALT_ATTACK_ITEMS 16
itype_id ALT_ATTACK_ITEMS[NUM_ALT_ATTACK_ITEMS] = {
    "knife_combat", "spear_wood", "molotov", "pipebomb", "grenade",
    "gasbomb", "bot_manhack", "tazer", "dynamite", "mininuke",
    "molotov_lit", "pipebomb_act", "grenade_act", "gasbomb_act",
    "dynamite_act", "mininuke_act"
};
#endif

enum npc_action : int {
    npc_undecided = 0,
    npc_pause, //1
    npc_reload, npc_sleep, // 2, 3
    npc_pickup, // 4
    npc_escape_item, npc_wield_melee, npc_wield_loaded_gun, npc_wield_empty_gun,
    npc_heal, npc_use_painkiller, npc_drop_items, // 5 - 12
    npc_flee, npc_melee, npc_shoot, npc_shoot_burst, npc_alt_attack, // 13 - 17
    npc_look_for_player, npc_heal_player, npc_follow_player, npc_follow_embarked,
    npc_talk_to_player, npc_mug_player, // 18 - 23
    npc_goto_destination, npc_avoid_friendly_fire, // 24, 25
    npc_base_idle, // 26
    npc_noop,
    npc_reach_attack, npc_aim,
    num_npc_actions
};

const int avoidance_vehicles_radius = 5;

std::string npc_action_name(npc_action action);
bool thrown_item( item &used );

void print_action( const char *prepend, npc_action action );

hp_part most_damaged_hp_part( const Character &c );

// Used in npc::drop_items()
struct ratio_index {
    double ratio;
    int index;
    ratio_index(double R, int I) : ratio (R), index (I) {};
};

bool clear_shot_reach( const tripoint &from, const tripoint &to )
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

bool npc::sees_dangerous_field( const tripoint &p ) const
{
    return is_dangerous_field( g->m.field_at( p ) );
}

bool npc::could_move_onto( const tripoint &p ) const
{
    return g->m.passable( p ) && !sees_dangerous_field( p );
}

// class npc functions!

void npc::assess_danger()
{
    int assessment = 0;
    for (size_t i = 0; i < g->num_zombies(); i++) {
        if( sees( g->zombie( i ) ) ) {
            assessment += g->zombie(i).type->difficulty;
        }
    }
    assessment /= 10;
    if (assessment <= 2) {
        assessment = -10 + 5 * assessment; // Low danger if no monsters around
    }
    // Mod for the player
    if (is_enemy()) {
        if (rl_dist( pos(), g->u.pos() ) < 10) {
            if (g->u.weapon.is_gun()) {
                assessment += 10;
            } else {
                assessment += 10 - rl_dist( pos(), g->u.pos() );
            }
        }
    } else if (is_friend()) {
        if (rl_dist( pos(), g->u.pos() ) < 8) {
            if (g->u.weapon.is_gun()) {
                assessment -= 8;
            } else {
                assessment -= 8 - rl_dist( pos(), g->u.pos() );
            }
        }
    }
    for (int i = 0; i < num_hp_parts; i++) {
        if (i == hp_head || i == hp_torso) {
            if (hp_cur[i] < hp_max[i] / 4) {
                assessment += 5;
            } else if (hp_cur[i] < hp_max[i] / 2) {
                assessment += 3;
            } else if (hp_cur[i] < hp_max[i] * .9) {
                assessment += 1;
            }
        } else {
            if (hp_cur[i] < hp_max[i] / 4) {
                assessment += 2;
            } else if (hp_cur[i] < hp_max[i] / 2) {
                assessment += 1;
            }
        }
    }

    ai_cache.danger_assessment = assessment;
}

void npc::regen_ai_cache()
{
    assess_danger();
    ai_cache.target = TARGET_NONE;
    choose_monster_target();

    if( is_enemy() ) {
        int pl_danger = player_danger( g->u );
        if( ( pl_danger > ai_cache.danger || rl_dist( pos(), g->u.pos() ) <= 1 ) ||
              ai_cache.target == TARGET_NONE ) {
            ai_cache.target = TARGET_PLAYER;
            ai_cache.danger = pl_danger;
            add_msg( m_debug, "NPC %s: Set target to PLAYER, danger = %d", name.c_str(), ai_cache.danger );
        }
    }
}

void npc::move()
{
    regen_ai_cache();
    npc_action action = npc_undecided;

    add_msg( m_debug, "NPC %s: target = %d, danger = %d, range = %d",
                 name.c_str(), ai_cache.target, ai_cache.danger, confident_range(-1));

    //faction opinion determines if it should consider you hostile
    if( my_fac != nullptr && my_fac->likes_u < -10 && sees( g->u ) ) {
        add_msg( m_debug, "NPC %s turning hostile because my_fac->likes_u %d < -10",
                 name.c_str(), my_fac->likes_u );
        if (op_of_u.fear > 10 + personality.aggression + personality.bravery) {
            attitude = NPCATT_FLEE;    // We don't want to take u on!
        } else {
            attitude = NPCATT_KILL;    // Yeah, we think we could take you!
        }
    }

    // This bypasses the logic to determine the npc action, but this all needs to be rewritten anyway.
    if( sees_dangerous_field( pos() ) ) {
        auto targets = closest_tripoints_first( 1, pos() );
        targets.erase( targets.begin() ); // current location
        auto filter = [this](const tripoint &p) {
            return !could_move_onto( p );
        };
        targets.erase( std::remove_if( targets.begin(), targets.end(), filter ), targets.end() );
        if( !targets.empty() ) {
            move_to( random_entry( targets ) );
            return;
        }
    }

    // TODO: Place player-aiding actions here, with a weight

    /* NPCs are fairly suicidal so at this point we will do a quick check to see if
     * something nasty is going to happen.
     */

    if( is_enemy() && vehicle_danger(avoidance_vehicles_radius) > 0 ) {
        // TODO: Think about how this actually needs to work, for now assume flee from player
        ai_cache.target = TARGET_PLAYER;
    }

    // TODO: morale breaking when surrounded by hostiles
    //if (!bravery_check(danger) || !bravery_check(total_danger) ||
    // TODO: near by active explosives spotted

    if( ai_cache.target == TARGET_PLAYER && attitude == NPCATT_FLEE ) {
        action = method_of_fleeing();
    } else if( ( ai_cache.target != TARGET_NONE && ai_cache.danger > 0 ) ||
               ( ai_cache.target == TARGET_PLAYER && attitude == NPCATT_KILL) ) {
        action = method_of_attack();
    } else {
        // No present danger
        action = address_needs();
        print_action( "address_needs %s", action );

        if( action == npc_undecided ) {
            action = address_player();
            print_action( "address_player %s", action );
        }
    }

    if( action == npc_undecided ) {
        if( is_guarding() ) {
            action = goal == global_omt_location() ?
                npc_pause :
                npc_goto_destination;
        } else if( has_new_items && scan_new_items() ) {
            return;
        } else if( !fetching_item ) {
            find_item();
            print_action( "find_item %s", action );
        }

        // check if in vehicle before rushing off to fetch things
        if( is_following() && g->u.in_vehicle ) {
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
    if( ai_cache.danger > 0 && ai_cache.target != INT_MIN &&
        (
          ( action == npc_follow_embarked && in_vehicle) ||
          ( action == npc_follow_player &&
            ( rl_dist( pos(), g->u.pos() ) <= follow_distance() || posz() != g->u.posz() ) )
        ) ) {
        action = method_of_attack();
    }

    add_msg( m_debug, "%s chose action %s.", name.c_str(), npc_action_name( action ).c_str() );

    execute_action( action );
}

void npc::execute_action( npc_action action )
{
    int oldmoves = moves;
    tripoint tar = pos();
    Creature *cur = current_target();
    if( cur != nullptr ) {
        tar = cur->pos();
    }
    /*
      debugmsg("%s ran execute_action() with target = %d! Action %s",
               name.c_str(), target, npc_action_name(action).c_str());
    */

    switch (action) {

    case npc_pause:
        move_pause();
        break;

    case npc_reload: {
        do_reload( weapon );
    }
    break;

    case npc_sleep:
    {
        // TODO: Allow stims when not too tired
        // Find a nice spot to sleep
        int best_sleepy = sleep_spot( pos() );
        tripoint best_spot = pos();
        const auto points = closest_tripoints_first( 6, pos() );
        for( const tripoint &p : points )
        {
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

        update_path( best_spot );
        // TODO: Handle empty path better
        if( best_spot == pos() || path.empty() ) {
            move_pause();
            if( !has_effect( effect_lying_down ) ) {
                add_effect( effect_lying_down, 300, num_bp, false, 1 );
                if( g->u.sees( *this ) ) {
                    add_msg( _("%s lies down to sleep."), name.c_str() );
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

    case npc_escape_item:
        use_escape_item(choose_escape_item());
        break;

    case npc_wield_melee:
        wield_best_melee();
        break;

    case npc_wield_loaded_gun: {
        item *it = inv.most_loaded_gun();
        if (it->is_null()) {
            debugmsg("NPC tried to wield a loaded gun, but has none!");
            move_pause();
        } else {
            wield( *it );
        }
    }
    break;

    case npc_wield_empty_gun: {
        bool ammo_found = false;
        int index = -1;
        invslice slice = inv.slice();
        for (size_t i = 0; i < slice.size(); i++) {
            item &it = slice[i]->front();
            bool am = (it.is_gun() &&
                       get_ammo( it.type->gun->ammo ).size() > 0);
            if (it.is_gun() && (!ammo_found || am)) {
                index = i;
                ammo_found = (ammo_found || am);
            }
        }
        if (index == -1) {
            debugmsg("NPC tried to wield a gun, but has none!");
            move_pause();
        } else {
            wield( slice[index]->front() );
        }
    }
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
        // TODO: More intelligent fleeing
        move_away_from( tar );
        break;

    case npc_reach_attack:
        if( weapon.reach_range() >= rl_dist( pos(), tar ) &&
            clear_shot_reach( pos(), tar ) ) {
            reach_attack( tar );
            break;
        }
        // Otherwise fallthrough to npc_melee
    case npc_melee:
        update_path( tar );
        if( path.size() > 1 ) {
            move_to_next();
        } else if( path.size() == 1 ) {
            if( cur != nullptr ) {
                melee_attack( *cur, true );
            }
        } else {
            look_for_player(g->u);
        }
        break;

    case npc_aim:
        if( moves > 10 ) {
            aim();
        } else {
            move_pause();
        }

        break;

    case npc_shoot:
        aim();
        fire_gun( tar, 1 );
        break;

    case npc_shoot_burst:
        aim();
        fire_gun( tar, weapon.burst_size() );
        break;

    case npc_alt_attack:
        alt_attack();
        break;

    case npc_look_for_player:
        if( saw_player_recently() && sees( last_player_seen_pos ) ) {
            update_path( last_player_seen_pos );
            move_to_next();
        } else {
            look_for_player(g->u);
        }
        break;

    case npc_heal_player:
        update_path( g->u.pos() );
        if (path.size() == 1) { // We're adjacent to u, and thus can heal u
            heal_player(g->u);
        } else if (!path.empty()) {
            move_to_next();
        } else {
            move_pause();
        }
        break;

    case npc_follow_player:
        update_path( g->u.pos() );
        if( (int)path.size() <= follow_distance() && g->u.posz() == posz() ) { // We're close enough to u.
            move_pause();
        } else if( !path.empty() ) {
            move_to_next();
        } else {
            move_pause();
        }
        // TODO: Make it only happen when it's safe
        complain();
        break;

    case npc_follow_embarked:
    {
        int p1;
        vehicle *veh = g->m.veh_at( g->u.pos(), p1 );

        if( veh == nullptr ) {
            debugmsg("Following an embarked player with no vehicle at their location?");
            // TODO: change to wait? - for now pause
            move_pause();
            break;
        }

        // Try to find the last destination
        // This is mount point, not actual position
        point last_dest( INT_MIN, INT_MIN );
        if( !path.empty() && g->m.veh_at( path[path.size() - 1], p1 ) == veh && p1 >= 0 ) {
            last_dest = veh->parts[p1].mount;
        }

        // Prioritize last found path, then seats
        // Don't change spots if ours is nice
        int my_spot = -1;
        std::vector<std::pair<int, int> > seats;
        for( size_t p2 = 0; p2 < veh->parts.size(); p2++ ) {
            if( !veh->part_flag( p2, VPFLAG_BOARDABLE ) ) {
                continue;
            }

            const player *passenger = veh->get_passenger( p2 );
            if( passenger != this && passenger != nullptr ) {
                continue;
            }

            int priority = 0;
            if( veh->parts[p2].mount == last_dest ) {
                // Shares mount point with last known path
                // We probably wanted to go there in the last turn
                priority = 4;
            } else if( veh->part_flag( p2, "SEAT" ) ) {
                // Assuming the player "owns" a sensible vehicle,
                //  seats should be in good spots to occupy
                priority = veh->part_with_feature( p2, "SEATBELT" ) >= 0 ? 3 : 2;
            } else if( veh->is_inside( p2 ) ) {
                priority = 1;
            }

            if( passenger == this ) {
                my_spot = priority;
            }

            seats.push_back( std::make_pair( priority, p2 ) );
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

            tripoint pp = veh->global_pos3() + veh->parts[cur_part].precalc[0];
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
        update_path( g->u.pos() );
        if (path.size() == 1) { // We're adjacent to u, and thus can mug u
            mug_player(g->u);
        } else if (!path.empty()) {
            move_to_next();
        } else {
            move_pause();
        }
        break;

    case npc_goto_destination:
        go_to_destination();
        break;

    case npc_avoid_friendly_fire:
        avoid_friendly_fire();
        break;

    case npc_base_idle:
        // TODO: patrol or sleep or something?
        move_pause();
        break;

    case npc_undecided:
        complain();
        move_pause();
        break;

    case npc_noop:
        add_msg( m_debug, "%s skips turn (noop)", disp_name().c_str() );
        return;

    default:
        debugmsg("Unknown NPC action (%d)", action);
    }

    if( oldmoves == moves ) {
        add_msg( m_debug, "NPC didn't use its moves.  Action %d.", action);
    }
}

void npc::choose_monster_target()
{
    int &enemy = ai_cache.target;
    int &danger = ai_cache.danger;
    int &total_danger = ai_cache.total_danger;
    enemy = 0;
    danger = 0;
    total_danger = 0;

    bool defend_u = sees( g->u ) && is_defending();
    int highest_priority = 0;

    // Radius we can attack without moving
    const int cur_range = std::max( weapon.reach_range(), confident_range() );

    constexpr int def_radius = 6;

    for( size_t i = 0; i < g->num_zombies(); i++ ) {
        monster &mon = g->zombie( i );
        if( !sees( mon ) ) {
            continue;
        }

        int dist = rl_dist( pos(), mon.pos() );
        int scaled_distance = std::max( 1, (100 * dist) / mon.get_speed() );
        float hp_percent = (float)(mon.get_hp_max() - mon.get_hp()) / mon.get_hp_max();
        float priority = mon.type->difficulty * (1.0f + hp_percent) - (scaled_distance - 1);
        int monster_danger = mon.type->difficulty * hp_percent;

        auto att = mon.attitude( this );
        if( att == MATT_FRIEND || att == MATT_FPASSIVE ) {
            continue;
        }

        if( att == MATT_ATTACK ) {
            monster_danger++;
        }

        total_danger += monster_danger / scaled_distance;

        bool okay_by_rules = true;
        if( is_following() ) {
            switch( rules.engagement ) {
            case ENGAGE_NONE:
                okay_by_rules = false;
                break;
            case ENGAGE_CLOSE:
                // Either close to player or close enough that we can reach it and close to us
                okay_by_rules = rl_dist( mon.pos(), g->u.pos() ) <= def_radius ||
                    ( dist <= cur_range && scaled_distance <= def_radius / 2 );
                break;
            case ENGAGE_WEAK:
                okay_by_rules = mon.get_hp() <= average_damage_dealt();
                break;
            case ENGAGE_HIT:
                okay_by_rules = mon.has_effect( effect_hit_by_player );
                break;
            case ENGAGE_NO_MOVE:
                okay_by_rules = dist <= cur_range;
                break;
            case ENGAGE_ALL:
                okay_by_rules = true;
                break;
            }
        }

        if( !okay_by_rules ) {
            continue;
        }

        if( priority < 1.0f && is_following() && att == MATT_ATTACK &&
            rl_dist( mon.pos(), g->u.pos() ) <= def_radius ) {
            priority = 1.0f;
        }

        if( monster_danger > danger && priority > 0 ) {
            danger = monster_danger;
        }

        if( priority > highest_priority ) {
            highest_priority = priority;
            enemy = i;
        } else if( defend_u ) {
            priority = mon.type->difficulty * (1 + hp_percent);
            scaled_distance = (100 * rl_dist(g->u.pos(), mon.pos())) / mon.get_speed();
            priority -= scaled_distance;
            if( mon.get_speed() < get_speed() ) {
                priority -= 10;
            }
            priority *= (personality.bravery + personality.altruism + op_of_u.value) / 15;
            if( priority > highest_priority ) {
                highest_priority = priority;
                enemy = i;
            }
        }
    }
}

npc_action npc::method_of_fleeing()
{
    Creature *target = current_target();
    if( target == nullptr ) {
        // Shouldn't be called
        debugmsg("Ran npc::method_of_fleeing without a target!");
        return npc_pause;
    }
    const float enemy_speed = target->speed_rating();
    const tripoint &enemy_loc = target->pos();
    int distance = rl_dist( pos(), enemy_loc );

    if( choose_escape_item() != INT_MIN ) {
        // We have an escape item!
        return npc_escape_item;
    }

    if( distance / enemy_speed < 4 && enemy_speed > speed_rating() ) {
        // Can't outrun, so attack
        return method_of_attack();
    }

    return npc_flee;
}

npc_action npc::method_of_attack()
{
    bool can_use_gun = (!is_following() || rules.use_guns);
    bool use_silent = (is_following() && rules.use_silent);
    int reach_range = weapon.reach_range();

    Creature *critter = get_target( ai_cache.target );
    if( critter == nullptr ) {
        // This function shouldn't be called...
        debugmsg("Ran npc::method_of_attack without a target!");
        return npc_pause;
    }

    tripoint tar = critter->pos();
    int dist = rl_dist( pos(), tar );
    int target_HP;
    if( !critter->is_monster() ) {
        target_HP = critter->hp_percentage() * critter->get_hp( hp_torso );
    } else {
        target_HP = critter->get_hp();
    }

    const npc_action melee_action = reach_range > 1 ? npc_reach_attack : npc_melee;
    // TODO: Change the in_vehicle check to actual "are we driving" check
    const bool dont_move = in_vehicle || rules.engagement == ENGAGE_NO_MOVE;

    // TODO: Make NPCs understand reinforced glass and vehicles blocking line of fire
    if( can_use_gun ) {
        if( need_to_reload() && can_reload_current() ) {
            return npc_reload;
        }
        if( emergency() && alt_attack_available() ) {
            return npc_alt_attack;
        }
        if( weapon.is_gun() && (!use_silent || weapon.is_silent()) &&
            weapon.ammo_remaining() >= weapon.ammo_required() ) {
            const int confident = confident_gun_range( weapon );
            if( dist > confident ) {
                if( can_reload_current() && (enough_time_to_reload( weapon ) || in_vehicle) ) {
                    return npc_reload;
                } else if( dont_move && dist > reach_range ) {
                    return npc_pause;
                } else if( dist > confident_gun_range( weapon, weapon.sight_dispersion( -1 ) ) ) {
                    return melee_action;
                } else {
                    return npc_aim;
                }
            }
            if( !wont_hit_friend( tar ) ) {
                if( dont_move ) {
                    if( can_reload_current() ) {
                        return npc_reload;
                    } else {
                        // Wait for clear shot
                        return npc_pause;
                    }
                } else {
                    return npc_avoid_friendly_fire;
                }
            } else if( !sees( *critter ) ) {
                // Can't see target
                return melee_action;
            } else if( dist > confident && sees( tar ) ) {
                // If out of confident range, aim or move closer to the target
                if( dist > confident_gun_range( weapon, weapon.sight_dispersion( -1 ) ) ) {
                    return melee_action;
                } else {
                    return npc_aim;
                }
            } else if( dist <= confident / 3 &&
                       weapon.ammo_remaining() >= weapon.burst_size() &&
                       (target_HP >= weapon.gun_damage() * 3 ||
                        emergency( ai_cache.danger * 2 ) ) ) {
                return npc_shoot_burst;
            } else {
                return npc_shoot;
            }
        }
    }

    // TODO: Add a time check now that wielding takes a lot of time
    if( wield_better_weapon() ) {
        return npc_noop;
    }

    if( dont_move && (dist > reach_range || !clear_shot_reach( pos(), tar ) ) ) {
        return npc_pause;
    }

    return melee_action;
}

bool need_heal( const Character &n )
{
    for( int i = 0; i < num_hp_parts; i++ ) {
        hp_part part = hp_part(i);
        if( (part == hp_head  && n.hp_cur[i] <= 35) ||
            (part == hp_torso && n.hp_cur[i] <= 25) ||
            n.hp_cur[i] <= 15) {
            return true;
        }
    }

    return false;
}

npc_action npc::address_needs()
{
    return address_needs( ai_cache.danger );
}

bool wants_to_reload( const npc& who, const item &it )
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

bool wants_to_reload_with( const item &weap, const item &ammo )
{
    if( ammo.is_magazine() && ammo.ammo_remaining() <= weap.ammo_remaining() ) {
        return false;
    }

    return true;
}

item &npc::find_reloadable()
{
    // Check wielded gun, non-wielded guns, mags and tools
    // TODO: Build a proper gun->mag->ammo DAG (Directed Acyclic Graph)
    // to avoid checking same properties over and over
    // TODO: Make this understand bandoliers, pouches etc.
    // TODO: Cache items checked for reloading to avoid re-checking same items every turn
    // TODO: Make it understand smaller and bigger magazines
    item *reloadable = nullptr;
    visit_items( [this, &reloadable]( item *node ) {
        if( !wants_to_reload( *this, *node ) ) {
            return VisitResponse::SKIP;
        }
        const auto it_loc = node->pick_reload_ammo( *this ).ammo;
        if( it_loc && wants_to_reload_with( *node, *it_loc ) ) {
            reloadable = node;
            return VisitResponse::ABORT;
        }

        return VisitResponse::SKIP;
    } );

    if( reloadable != nullptr ) {
        return *reloadable;
    }

    return ret_null;
}

const item &npc::find_reloadable() const
{
    return const_cast<const item &>( const_cast<npc *>( this )->find_reloadable() );
}

bool npc::can_reload_current()
{
    if( !weapon.is_gun() ) {
        return false;
    }

    return find_usable_ammo( weapon );
}

item_location npc::find_usable_ammo( const item &weap )
{
    if( !can_reload( weap ) ) {
        return item_location();
    }

    auto loc = weap.pick_reload_ammo( *this ).ammo;
    if( !loc || !wants_to_reload_with( weap, *loc ) ) {
        return item_location();
    }

    return loc;
}

const item_location npc::find_usable_ammo( const item &weap ) const
{
    return const_cast<npc *>( this )->find_usable_ammo( weap );
}

npc_action npc::address_needs( int danger )
{
    if( need_heal( *this ) && has_healing_item() ) {
        return npc_heal;
    }

    if( get_perceived_pain() >= 15 && has_painkiller() && !took_painkiller() ) {
        return npc_use_painkiller;
    }

    if( can_reload_current() ) {
        return npc_reload;
    }

    item &reloadable = find_reloadable();
    if( !reloadable.is_null() ) {
        do_reload( reloadable );
        return npc_noop;
    }

    if ((danger <= NPC_DANGER_VERY_LOW && (get_hunger() > 40 || get_thirst() > 40)) ||
        get_thirst() > 80 || get_hunger() > 160) {
        if( consume_food() ) {
            return npc_noop;
        }
    }

    if( danger <= NPC_DANGER_VERY_LOW && find_corpse_to_pulp() ) {
        if( !do_pulp() ) {
            move_to_next();
        }

        return npc_noop;
    }

    // TODO: More risky attempts at sleep when exhausted
    if( danger == 0 && get_fatigue() > TIRED ) {
        if( !is_following() ) {
            set_fatigue(0); // TODO: Make tired NPCs handle sleep offscreen
            return npc_undecided;
        }

        if( rules.allow_sleep || get_fatigue() > MASSIVE_FATIGUE ) {
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
    if ((attitude == NPCATT_TALK || attitude == NPCATT_TRADE) && sees( g->u ) ) {
        if (g->u.in_sleep_state()) {
            // Leave sleeping characters alone.
            return npc_undecided;
        }
        if (rl_dist(pos(), g->u.pos()) <= 6) {
            return npc_talk_to_player;    // Close enough to talk to you
        } else {
            if (one_in(10)) {
                say("<lets_talk>");
            }
            return npc_follow_player;
        }
    }

    if (attitude == NPCATT_MUG && sees( g->u ) ) {
        if (one_in(3)) {
            say(_("Don't move a <swear> muscle..."));
        }
        return npc_mug_player;
    }

    if (attitude == NPCATT_WAIT_FOR_LEAVE) {
        patience--;
        if (patience <= 0) {
            patience = 0;
            attitude = NPCATT_KILL;
            return npc_noop;
        }
        return npc_undecided;
    }

    if (attitude == NPCATT_FLEE) {
        return npc_flee;
    }

    if (attitude == NPCATT_LEAD) {
        if( rl_dist( pos(), g->u.pos() ) >= 12 || !sees( g->u ) ) {
            int intense = get_effect_int( effect_catch_up );
            if (intense < 10) {
                say("<keep_up>");
                add_effect( effect_catch_up, 5);
                return npc_pause;
            } else {
                say("<im_leaving_you>");
                attitude = NPCATT_NULL;
                return npc_pause;
            }
        } else if( has_destination() ) {
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

    if (mission == NPC_MISSION_SHOPKEEP || mission == NPC_MISSION_SHELTER) {
        return npc_pause;    // Shopkeeps just stay put.
    }

    // TODO: Follow / look for player

    if (mission == NPC_MISSION_BASE) {
        return npc_base_idle;
    }

    if( !has_destination() ) {
        set_destination();
    }

    if( has_destination() ) {
        return npc_goto_destination;
    }

    return npc_undecided;
}


bool npc::alt_attack_available()
{
    for( auto &elem : ALT_ATTACK_ITEMS ) {
        if( ( !is_following() || rules.use_grenades ) && has_amount( elem, 1 ) ) {
            return true;
        }
    }
    return false;
}

int npc::choose_escape_item()
{
    int best = -1;
    int ret = INT_MIN;
    invslice slice = inv.slice();
    for (size_t i = 0; i < slice.size(); i++) {
        item &it = slice[i]->front();
        for (int j = 0; j < NUM_ESCAPE_ITEMS; j++) {
            const auto food = it.type->comestible.get();
            if (it.type->id == ESCAPE_ITEMS[j] &&
                (food == NULL || stim < food->stim ||            // Avoid guzzling down
                 (food->stim >= 10 && stim < food->stim * 2)) && //  Adderall etc.
                (j > best || (j == best && it.charges < slice[ret]->front().charges))) {
                ret = i;
                best = j;
                break;
            }
        }
    }
    return ret;
}

void npc::use_escape_item(int position)
{
    item *used = &i_at(position);
    if (used->is_null()) {
        debugmsg("%s tried to use null item (position: %d)", name.c_str(), position);
        move_pause();
        return;
    }

    /* There is a static list of items that NPCs consider to be "escape items," so
     * we can just use a switch here to decide what to do based on type.  See
     * ESCAPE_ITEMS, defined in npc.h
     */

    if (used->is_food() || used->is_food_container()) {
        consume(position);
        return;
    }

    if (used->is_tool()) {
        use(position);
        return;
    }

    debugmsg("NPC tried to use %s (%d) but it has no use?", used->tname().c_str(),
             position);
    move_pause();
}

double npc::confidence_mult() const
{
    if( !is_following() ) {
        return 1.0f;
    }

    switch( rules.aim ) {
        case AIM_WHEN_CONVENIENT:
            return emergency() ? 1.0f : 0.75f;
            break;
        case AIM_SPRAY:
            return 1.25f;
            break;
        case AIM_PRECISE:
            return emergency() ? 0.75f : 0.4f;
            break;
        case AIM_STRICTLY_PRECISE:
            return 0.25f;
            break;
    }

    return 1.0f;
}

int npc::confident_range( int position ) const
{
    if( position == -1 ) {
        return confident_gun_range( weapon );
    }

    return confident_throw_range( i_at( position ) );
}

int npc::confident_gun_range( const item &gun ) const
{
    return confident_gun_range( gun, recoil + driving_recoil );
}

int npc::confident_gun_range( const item &gun, int at_recoil ) const
{
    if( !gun.is_gun() || gun.ammo_remaining() < gun.ammo_required() ) {
        return 0;
    }

    double deviation = get_weapon_dispersion( &gun, false ) + at_recoil;
    // Halve to get expected values
    deviation /= 2;
    // Convert from MoA back to quarter-degrees.
    deviation /= 15;

    const int ret = std::min( int( confidence_mult() * 360 / deviation ), gun.gun_range( this ) );
    add_msg( m_debug, "confident_gun_range == %d", ret );
    return ret;
}

int npc::confident_throw_range( const item &thrown ) const
{
    ///\EFFECT_THROW_NPC increases throwing confidence of all items
    double deviation = 10 - get_skill_level( skill_throw );

    ///\EFFECT_PER_NPC increases throwing confidence of all items
    deviation += 10 - per_cur;

    ///\EFFECT_DEX_NPC increases throwing confidence of all items
    deviation += throw_dex_mod();

    ///\EFFECT_STR_NPC increases throwing confidence of heavy items
    deviation += std::min( ( thrown.weight() / 100 ) - str_cur, 0 );

    deviation += thrown.volume() / 4;

    deviation += encumb( bp_hand_r ) + encumb( bp_hand_l ) + encumb( bp_eyes );

    const int ret = std::min( int( confidence_mult() * 360 / deviation ), throw_range( thrown ) );
    add_msg( m_debug, "confident_throw_range == %d", ret );
    return ret;
}

// Index defaults to -1, i.e., wielded weapon
bool npc::wont_hit_friend( const tripoint &tar, int weapon_index ) const
{
    int confident = confident_range(weapon_index);
    // if there is no confidence at using weapon, it's not used at range
    // zero confidence leads to divide by zero otherwise
    if( confident < 1 ) {
        return true;
    }
    if( rl_dist( pos(), tar ) == 1 ) {
        return true;    // If we're *really* sure that our aim is dead-on
    }

    std::vector<tripoint> traj = g->m.find_clear_path( pos(), tar );

    for( auto &i : traj ) {
        int dist = rl_dist( pos(), i );
        int deviation = 1 + int(dist / confident);
        for (int x = i.x - deviation; x <= i.x + deviation; x++) {
            for (int y = i.y - deviation; y <= i.y + deviation; y++) {
                // Hit the player?
                if (is_friend() && g->u.posx() == x && g->u.posy() == y) {
                    return false;
                }
                // Hit a friendly monster?
                /*
                    for (int n = 0; n < g->num_zombies(); n++) {
                     if (g->zombie(n).friendly != 0 && g->zombie(n).posx == x && g->zombie(n).posyposition == y)
                      return false;
                    }
                */
                // Hit an NPC that's on our team?
                /*
                    for (int n = 0; n < g->active_npc.size(); n++) {
                     npc* guy = &(g->active_npc[n]);
                     if (guy != this && (is_friend() == guy->is_friend()) &&
                         guy->posx == x && guy->posy == y)
                      return false;
                    }
                */
            }
        }
    }
    return true;
}

bool npc::need_to_reload() const
{
    if( !can_reload( weapon ) ) {
        return false;
    }

    return ( weapon.ammo_remaining() < weapon.ammo_required() ||
             weapon.ammo_remaining() < weapon.ammo_capacity() * 0.1 );
}

bool npc::enough_time_to_reload( const item &gun ) const
{
    int rltime = item_reload_cost( gun, item( default_ammo( gun.ammo_type() ) ), gun.ammo_capacity() );
    const float turns_til_reloaded = (float)rltime / get_speed();

    Creature *target = current_target();
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
    int aim_amount = aim_per_time( weapon, recoil );
    while( aim_amount > 0 && recoil > 0 && moves > 10 ) {
        moves -= 10;
        recoil -= aim_amount;
        recoil = std::max( 0, recoil );
        aim_amount = aim_per_time( weapon, recoil );
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

    const int bash_power = no_bashing ? 0 : smash_ability();
    auto new_path = g->m.route( pos(), p, bash_power, 1000 );
    if( new_path.empty() ) {
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

bool npc::can_move_to( const tripoint &p, bool no_bashing ) const
{
    // Allow moving into any bashable spots, but penalize them during pathing
    return( rl_dist( pos(), p ) <= 1 &&
              (
                g->m.passable( p ) ||
                ( !no_bashing && g->m.bash_rating( smash_ability(), p ) > 0 ) ||
                g->m.open_door( p, !g->m.is_outside( pos() ), true )
              )
           );
}

void npc::move_to( const tripoint &pt, bool no_bashing )
{
    if( g->m.has_flag("UNSTABLE", pt ) ) {
        add_effect( effect_bouldering, 1, num_bp, true);
    } else if (has_effect( effect_bouldering)) {
        remove_effect( effect_bouldering);
    }

    tripoint p = pt;
    if( sees_dangerous_field( pt ) ) {
        // Move to a neighbor field instead, if possible.
        // Maybe this code already exists somewhere?
        auto other_points = g->m.get_dir_circle( pos(), pt );
        for( const tripoint &ot : other_points ) {
            if( could_move_onto( ot ) ) {
                p = ot;
                break;
            }
        }
    }

    if (recoil > 0) { // Start by dropping recoil a little
        ///\EFFECT_STR_NPC increases recoil recovery speed

        ///\EFFECT_GUN_NPC increases recoil recovery speed
        if (int(str_cur / 2) + get_skill_level( skill_gun ) >= (int)recoil) {
            recoil = MIN_RECOIL;
        } else {
            recoil -= int(str_cur / 2) + get_skill_level( skill_gun );
            recoil = int(recoil / 2);
        }
    }

    if (has_effect( effect_stunned)) {
        p.x = rng(posx() - 1, posx() + 1);
        p.y = rng(posy() - 1, posy() + 1);
        p.z = posz();
    }

    // "Long steps" are allowed when crossing z-levels
    // Stairs teleport the player too
    if( rl_dist( pos(), p ) > 1 && p.z == posz() ) {
        // On the same level? Not so much. Something weird happened
        path.clear();
        move_pause();
    }
    bool attacking = false;
    if( g->mon_at( p ) ) {
        attacking = true;
    }
    if( !move_effects(attacking) ) {
        mod_moves(-100);
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
                melee_attack( *critter, true );
            } else {
                move_pause();
            }

            return;
        }

        if( critter == &g->u ) {
            say("<let_me_pass>");
        }

        // Let NPCs push each other when non-hostile
        // TODO: Have them attack each other when hostile
        npc *np = dynamic_cast<npc*>( critter );
        if( np != nullptr && !np->in_sleep_state() ) {
            np->move_away_from( pos(), true );
        }

        if( critter->pos() == p ) {
            move_pause();
            return;
        }
    }

    // Boarding moving vehicles is fine, unboarding isn't
    bool moved = false;
    const vehicle *veh = g->m.veh_at( pos() );
    if( veh != nullptr ) {
        int other_part = -1;
        const vehicle *oveh = g->m.veh_at( p, other_part );
        if( abs(veh->velocity) > 0 &&
            ( oveh != veh ||
              veh->part_with_feature( other_part, VPFLAG_BOARDABLE ) < 0 ) ) {
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
    } else if( g->m.open_door( p, !g->m.is_outside( pos() ) ) ) {
        moves -= 100;
    } else if( g->m.has_flag_ter_or_furn( "CLIMBABLE", p ) ) {
        ///\EFFECT_DEX_NPC increases chance to climb CLIMBABLE furniture or terrain
        int climb = dex_cur;
        if( one_in( climb ) ) {
            add_msg_if_npc( m_neutral, _( "%1$s falls tries to climb the %2$s but slips." ),
                            name.c_str(), g->m.tername(p).c_str() );
            moves -= 400;
        } else {
            add_msg_if_npc( m_neutral, _( "%1$s climbs over the %2$s." ), name.c_str(),
                            g->m.tername( p ).c_str() );
            moves -= (500 - (rng(0,climb) * 20));
            moved = true;
        }
    } else if( !no_bashing && smash_ability() > 0 && g->m.is_bashable( p ) &&
               g->m.bash_rating( smash_ability(), p ) > 0 ) {
        moves -= int(weapon.is_null() ? 80 : weapon.attack_time() * 0.8);
        g->m.bash( p, smash_ability() );
    } else {
        if( attitude == NPCATT_MUG ||
            attitude == NPCATT_KILL ||
            attitude == NPCATT_WAIT_FOR_LEAVE ) {
            attitude = NPCATT_FLEE;
        }

        moves = 0;
    }

    if( moved ) {
        if( in_vehicle ) {
            g->m.unboard_vehicle( pos() );
        }

        setpos( p );
        int part;
        vehicle *veh = g->m.veh_at( p, part );
        if( veh != nullptr && veh->part_with_feature( part, VPFLAG_BOARDABLE ) >= 0 ) {
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
        add_msg( m_debug, "npc::move_to_next() called with an empty path or path containing only current position" );
        move_pause();
        return;
    }

    move_to( path[0] );
    if( !path.empty() && pos() == path[0] ) { // Move was successful
        path.erase( path.begin() );
    }
}

// TODO: Rewrite this.  It doesn't work well and is ugly.
void npc::avoid_friendly_fire()
{
    tripoint tar;
    Creature *critter = current_target();
    if( critter != nullptr ) {
        tar = critter->pos();
        if( critter != &g->u && !one_in( 3 ) ) {
            say( _("<move> so I can shoot that %s!"), critter->get_name().c_str() );
        }
    } else { // This function shouldn't be called...
        debugmsg("npc::avoid_friendly_fire() called with no target!");
        move_pause();
        return;
    }

    int xdir = (tar.x > posx() ? 1 : -1), ydir = (tar.y > posy() ? 1 : -1);
    direction dir_to_target = direction_from( posx(), posy(), tar.x, tar.y );
    std::vector<point> valid_moves;
    /* Ugh, big ugly switch.  This fills valid_moves with a list of moves from most
     * desirable to least; the only two moves excluded are those along the line of
     * sight.
     * TODO: Use some math instead of a big ugly switch.
     */
    switch (dir_to_target) {
    case NORTH:
        valid_moves.push_back(point(posx() + xdir, posy()));
        valid_moves.push_back(point(posx() - xdir, posy()));
        valid_moves.push_back(point(posx() + xdir, posy() + 1));
        valid_moves.push_back(point(posx() - xdir, posy() + 1));
        valid_moves.push_back(point(posx() + xdir, posy() - 1));
        valid_moves.push_back(point(posx() - xdir, posy() - 1));
        break;
    case NORTHEAST:
        valid_moves.push_back(point(posx() + 1, posy() + 1));
        valid_moves.push_back(point(posx() - 1, posy() - 1));
        valid_moves.push_back(point(posx() - 1, posy()    ));
        valid_moves.push_back(point(posx()    , posy() + 1));
        valid_moves.push_back(point(posx() + 1, posy()    ));
        valid_moves.push_back(point(posx()    , posy() - 1));
        break;
    case EAST:
        valid_moves.push_back(point(posx(), posy() - 1));
        valid_moves.push_back(point(posx(), posy() + 1));
        valid_moves.push_back(point(posx() - 1, posy() - 1));
        valid_moves.push_back(point(posx() - 1, posy() + 1));
        valid_moves.push_back(point(posx() + 1, posy() - 1));
        valid_moves.push_back(point(posx() + 1, posy() + 1));
        break;
    case SOUTHEAST:
        valid_moves.push_back(point(posx() + 1, posy() - 1));
        valid_moves.push_back(point(posx() - 1, posy() + 1));
        valid_moves.push_back(point(posx() + 1, posy()    ));
        valid_moves.push_back(point(posx()    , posy() + 1));
        valid_moves.push_back(point(posx() - 1, posy()    ));
        valid_moves.push_back(point(posx()    , posy() - 1));
        break;
    case SOUTH:
        valid_moves.push_back(point(posx() + xdir, posy()));
        valid_moves.push_back(point(posx() - xdir, posy()));
        valid_moves.push_back(point(posx() + xdir, posy() - 1));
        valid_moves.push_back(point(posx() - xdir, posy() - 1));
        valid_moves.push_back(point(posx() + xdir, posy() + 1));
        valid_moves.push_back(point(posx() - xdir, posy() + 1));
        break;
    case SOUTHWEST:
        valid_moves.push_back(point(posx() + 1, posy() + 1));
        valid_moves.push_back(point(posx() - 1, posy() - 1));
        valid_moves.push_back(point(posx() + 1, posy()    ));
        valid_moves.push_back(point(posx()    , posy() - 1));
        valid_moves.push_back(point(posx() - 1, posy()    ));
        valid_moves.push_back(point(posx()    , posy() + 1));
        break;
    case WEST:
        valid_moves.push_back(point(posx()    , posy() + ydir));
        valid_moves.push_back(point(posx()    , posy() - ydir));
        valid_moves.push_back(point(posx() + 1, posy() + ydir));
        valid_moves.push_back(point(posx() + 1, posy() - ydir));
        valid_moves.push_back(point(posx() - 1, posy() + ydir));
        valid_moves.push_back(point(posx() - 1, posy() - ydir));
        break;
    case NORTHWEST:
        valid_moves.push_back(point(posx() + 1, posy() - 1));
        valid_moves.push_back(point(posx() - 1, posy() + 1));
        valid_moves.push_back(point(posx() - 1, posy()    ));
        valid_moves.push_back(point(posx()    , posy() - 1));
        valid_moves.push_back(point(posx() + 1, posy()    ));
        valid_moves.push_back(point(posx()    , posy() + 1));
        break;
    default:
        // contains the case CENTER (pos==target, can not happen) and above/below (can not happen, function above is 2D only)
        dbg( D_ERROR ) << "avoid_friendly_fire has strange direction to target: " << dir_to_target;
        break;
    }

    for (auto &i : valid_moves) {
        const tripoint maybe_valid( i, posz() );
        if( can_move_to( maybe_valid ) ) {
            move_to( maybe_valid );
            return;
        }
    }

    /* If we're still in the function at this point, maneuvering can't help us. So,
     * might as well address some needs.
     * We pass a <danger> value of NPC_DANGER_VERY_LOW + 1 so that we won't start
     * eating food (or, god help us, sleeping).
     */
    npc_action action = address_needs(NPC_DANGER_VERY_LOW + 1);
    if (action == npc_undecided) {
        move_pause();
    }
    execute_action( action );
}

void npc::move_away_from( const tripoint &pt, bool no_bash_atk )
{
    tripoint best_pos = pos();
    int best = -1;
    int chance = 2;
    for( const tripoint &p : g->m.points_in_radius( pos(), 1 ) ) {
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

    move_to( best_pos, no_bash_atk );
}

void npc::move_pause()
{
    // NPCs currently always aim when using a gun, even with no target
    // This simulates them aiming at stuff just at the edge of their range
    if( !weapon.is_gun() ) {
        pause();
        return;
    }

    aim();

    // Player can cheese the pause recoil drop to speed up aiming, let npcs do it too
    int pause_recoil = recoil - str_cur + 2 * get_skill_level( skill_gun );
    pause_recoil = std::max( MIN_RECOIL * 2, pause_recoil );
    pause_recoil = pause_recoil / 2;
    if( pause_recoil < recoil ) {
        pause();
    } else {
        moves = 0;
    }
}

void npc::find_item()
{
    if( is_following() && !rules.allow_pick_up ) {
        // Grabbing stuff not allowed by our "owner"
        return;
    }

    fetching_item = false;
    int best_value = minimum_item_value();
    // For some reason range limiting by vision doesn't work properly
    const int range = 6;
    //int range = sight_range( g->light_level( posz() ) );
    //range = std::max( 1, std::min( 12, range ) );

    static const std::string no_pickup( "NO_NPC_PICKUP" );

    const item *wanted = nullptr;
    for( const tripoint &p : g->m.points_in_radius( pos(), range ) ) {
        // TODO: Make this sight check not overdraw nearby tiles
        // TODO: Optimize that zone check
        if( g->m.sees_some_items( p, *this ) && sees( p ) &&
            ( !is_following() || !g->check_zone( no_pickup, p ) ) ) {
            for( auto &elem : g->m.i_at( p ) ) {
                if( elem.made_of( LIQUID ) ) {
                    // Don't even consider liquids.
                    continue;
                }
                int itval = value( elem );
                int wgt = elem.weight(), vol = elem.volume();
                if( itval > best_value &&
                    ( can_pickWeight( wgt, true ) && can_pickVolume( vol, true ) ) ) {
                    wanted_item_pos = p;
                    wanted = &( elem );
                    best_value = itval;
                    fetching_item = true;
                }
            }
        }
    }

    if( !fetching_item ) {
        return;
    }

    // TODO: Move that check above, make it multi-target pathing and use it
    // to limit tiles available for choice of items
    const int dist_to_item = rl_dist( wanted_item_pos, pos() );
    update_path( wanted_item_pos );
    if( path.empty() && dist_to_item > 1 ) {
        // Item not reachable, let's just totally give up for now
        fetching_item = false;
    }

    if( fetching_item && rl_dist( wanted_item_pos, pos() ) > 1 && is_following() ) {
        say( _("Hold on, I want to pick up that %s."),
             wanted->tname().c_str() );
    }
}

void npc::pick_up_item()
{
    if( is_following() && !rules.allow_pick_up ) {
        add_msg( m_debug, "%s::pick_up_item(); Cancelling on player's request", name.c_str() );
        fetching_item = false;
        moves -= 1;
        return;
    }

    add_msg( m_debug, "%s::pick_up_item(); [%d, %d, %d] => [%d, %d, %d]", name.c_str(),
             posx(), posy(), posz(), wanted_item_pos.x, wanted_item_pos.y, wanted_item_pos.z );
    update_path( wanted_item_pos );

    auto items = g->m.i_at( wanted_item_pos );

    if( ( items.size() == 0 && sees( wanted_item_pos ) ) ||
        ( is_following() && g->check_zone( "NO_NPC_PICKUP", wanted_item_pos ) ) ) {
        // Items we wanted no longer exist and we can see it
        // Or player who is leading us doesn't want us to pick it up
        fetching_item = false;
        // Just to prevent debugmsgs
        moves -= 1;
        return;
    }

    if( path.size() > 1 ) {
        add_msg( m_debug, "Moving; [%d, %d, %d] => [%d, %d, %d]",
                 posx(), posy(), posz(), path[0].x, path[0].y, path[0].z );

        move_to_next();
        return;
    } else if( path.empty() && pos() != wanted_item_pos ) {
        // This can happen, always do something
        fetching_item = false;
        move_pause();
        return;
    }

    // We're adjacent to the item; grab it!
    moves -= 100;
    fetching_item = false;
    int total_volume = 0;
    int total_weight = 0; // How much the items will add
    std::vector<int> pickup; // Indices of items we want

    for( size_t i = 0; i < items.size(); i++ ) {
        const item &item = items[i];
        int itval = value( item );
        int vol = item.volume();
        int wgt = item.weight();
        if ( itval >= minimum_item_value() && // (itval >= worst_item_value ||
             ( can_pickVolume( total_volume + vol, true ) &&
               can_pickWeight( total_weight + wgt, true ) ) &&
             !item.made_of( LIQUID ) ) {
            pickup.push_back( i );
            total_volume += vol;
            total_weight += wgt;
        }
    }
    // Describe the pickup to the player
    bool u_see = g->u.sees( *this ) || g->u.sees( wanted_item_pos );
    if( u_see ) {
        if( pickup.size() == 1 ) {
            add_msg(_("%1$s picks up a %2$s."), name.c_str(),
                    items[pickup[0]].tname().c_str());
        } else if( pickup.size() == 2 ) {
            add_msg(_("%1$s picks up a %2$s and a %3$s."), name.c_str(),
                    items[pickup[0]].tname().c_str(),
                    items[pickup[1]].tname().c_str());
        } else if( pickup.size() > 2 ) {
            add_msg(_("%s picks up several items."), name.c_str());
        } else {
            add_msg(_("%s looks around nervously, as if searching for something."), name.c_str());
        }
    }

    for (auto &i : pickup) {
        int itval = value(items[i]);
        if (itval < worst_item_value) {
            worst_item_value = itval;
        }
        i_add(items[i]);
    }
    for (auto &i : pickup) {
        g->m.i_rem( wanted_item_pos, i );
        // Fix indices
        for (auto &j : pickup) {
            j--;
        }
    }

    has_new_items = true;
}

void npc::drop_items(int weight, int volume)
{
    add_msg( m_debug, "%s is dropping items-%d,%d (%d items, wgt %d/%d, vol %d/%d)",
                 name.c_str(), weight, volume, inv.size(), weight_carried(),
                 weight_capacity(), volume_carried(), volume_capacity());

    int weight_dropped = 0, volume_dropped = 0;
    std::vector<ratio_index> rWgt, rVol; // Weight/Volume to value ratios

    // First fill our ratio vectors, so we know which things to drop first
    invslice slice = inv.slice();
    for (size_t i = 0; i < slice.size(); i++) {
        item &it = slice[i]->front();
        double wgt_ratio, vol_ratio;
        if (value(it) == 0) {
            wgt_ratio = 99999;
            vol_ratio = 99999;
        } else {
            wgt_ratio = it.weight() / value(it);
            vol_ratio = it.volume() / value(it);
        }
        bool added_wgt = false, added_vol = false;
        for (size_t j = 0; j < rWgt.size() && !added_wgt; j++) {
            if (wgt_ratio > rWgt[j].ratio) {
                added_wgt = true;
                rWgt.insert(rWgt.begin() + j, ratio_index(wgt_ratio, i));
            }
        }
        if (!added_wgt) {
            rWgt.push_back(ratio_index(wgt_ratio, i));
        }
        for (size_t j = 0; j < rVol.size() && !added_vol; j++) {
            if (vol_ratio > rVol[j].ratio) {
                added_vol = true;
                rVol.insert(rVol.begin() + j, ratio_index(vol_ratio, i));
            }
        }
        if (!added_vol) {
            rVol.push_back(ratio_index(vol_ratio, i));
        }
    }

    std::stringstream item_name; // For description below
    int num_items_dropped = 0; // For description below
    // Now, drop items, starting from the top of each list
    while (weight_dropped < weight || volume_dropped < volume) {
        // weight and volume may be passed as 0 or a negative value, to indicate that
        // decreasing that variable is not important.
        int dWeight = (weight <= 0 ? -1 : weight - weight_dropped);
        int dVolume = (volume <= 0 ? -1 : volume - volume_dropped);
        int index;
        // Which is more important, weight or volume?
        if (dWeight > dVolume) {
            index = rWgt[0].index;
            rWgt.erase(rWgt.begin());
            // Fix the rest of those indices.
            for( auto &elem : rWgt ) {
                if( elem.index > index ) {
                    elem.index--;
                }
            }
        } else {
            index = rVol[0].index;
            rVol.erase(rVol.begin());
            // Fix the rest of those indices.
            for (size_t i = 0; i < rVol.size(); i++) {
                if (i > rVol.size())
                    debugmsg("npc::drop_items() - looping through rVol - Size is %d, i is %d",
                             rVol.size(), i);
                if (rVol[i].index > index) {
                    rVol[i].index--;
                }
            }
        }
        weight_dropped += slice[index]->front().weight();
        volume_dropped += slice[index]->front().volume();
        item dropped = i_rem(index);
        num_items_dropped++;
        if (num_items_dropped == 1) {
            item_name << dropped.tname();
        } else if (num_items_dropped == 2) {
            item_name << _(" and ") << dropped.tname();
        }
        g->m.add_item_or_charges(pos(), dropped);
    }
    // Finally, describe the action if u can see it
    std::string item_name_str = item_name.str();
    if (g->u.sees( *this )) {
        if (num_items_dropped >= 3) {
            add_msg(ngettext("%s drops %d item.", "%s drops %d items.",
                             num_items_dropped), name.c_str(),
                    num_items_dropped);
        } else {
            add_msg(_("%1$s drops a %2$s."), name.c_str(),
                    item_name_str.c_str());
        }
    }
    update_worst_item_value();
}

bool npc::find_corpse_to_pulp()
{
    if( is_following() && ( !rules.allow_pulp || g->u.in_vehicle ) ) {
        return false;
    }

    // Pathing with overdraw can get expensive, limit it
    int path_counter = 4;
    const auto check_tile = [this, &path_counter]( const tripoint &p ) -> const item * {
        if( !g->m.sees_some_items( p, *this ) || !sees( p ) ) {
            return nullptr;
        }

        const auto items = g->m.i_at( p );
        const item * found = nullptr;
        for( const item &it : items ) {
            // Pulp only stuff that revives, but don't pulp acid stuff
            // That is, if you aren't protected from this stuff!
            if( it.can_revive() ) {
                // If the first encountered corpse is acidic, it is not safe to bash
                if( is_dangerous_field( it.get_mtype()->bloodType() ) ) {
                    return nullptr;
                }

                found = &it;
                break;
            }
        }

        if( found != nullptr ) {
            path_counter--;
            // Only return corpses we can path to
            return update_path( p, false, false ) ? found : nullptr;
        }

        return nullptr;
    };

    const int range = 6;

    const bool had_pulp_target = pulp_location != tripoint_min;

    const item *corpse = nullptr;
    if( had_pulp_target && square_dist( pos(), pulp_location ) <= range ) {
        corpse = check_tile( pulp_location );
    }

    // Find the old target to avoid spamming
    const item *old_target = corpse;

    if( corpse == nullptr ) {
        // If we're following the player, don't wander off to pulp corpses
        const tripoint &around = is_following() ? g->u.pos() : pos();
        for( const tripoint &p : closest_tripoints_first( range, around ) ) {
            corpse = check_tile( p );

            if( corpse != nullptr ) {
                pulp_location = p;
                break;
            }

            if( path_counter <= 0 ) {
                break;
            }
        }
    }

    if( corpse != nullptr && corpse != old_target && is_following() ) {
        say( _("Hold on, I want to pulp that %s."),
             corpse->tname().c_str() );
    }

    return corpse != nullptr;
}

bool npc::do_pulp()
{
    if( pulp_location == tripoint_min ) {
        return false;
    }

    if( rl_dist( pulp_location, pos() ) > 1 || pulp_location.z != posz() ) {
        return false;
    }

    // TODO: Don't recreate the activity every time
    int old_moves = moves;
    assign_activity( ACT_PULP, INT_MAX, 0 );
    activity.placement = pulp_location;
    activity.do_turn( this );
    return moves != old_moves;
}

bool npc::wield_better_weapon()
{
    // TODO: Allow wielding weaker weapons against weaker targets
    bool can_use_gun = (!is_following() || rules.use_guns);
    bool use_silent = (is_following() && rules.use_silent);
    invslice slice = inv.slice();

    // Check if there's something better to wield
    item *best = &weapon;
    double best_value = -100.0;

    const auto compare_weapon =
        [this, &best, &best_value, can_use_gun, use_silent]( item &it, bool allow_ranged ) {
        double val = allow_ranged ? weapon_value( it ) : melee_value( it );
        if( val > best_value ) {
            best = &it;
            best_value = val;
        }
    };

    compare_weapon( weapon, can_use_gun );
    // To prevent changing to barely better stuff
    best_value *= 1.1;

    std::vector<item *> empty_guns;
    visit_items( [this, &compare_weapon, &empty_guns, can_use_gun, use_silent]( item *node ) {
        // Skip some bad items
        if( !node->is_gun() && node->type->melee_dam + node->type->melee_cut < 5 ) {
            return VisitResponse::SKIP;
        }

        bool allowed = can_use_gun && node->is_gun() && ( !use_silent || node->is_silent() );
        if( allowed && node->ammo_remaining() >= node->ammo_required() ) {
            compare_weapon( *node, true );
        } else if( allowed && enough_time_to_reload( *node ) ) {
            empty_guns.push_back( node );
        } else {
            compare_weapon( *node, false );
        }

        return VisitResponse::SKIP;
    } );

    for( auto &i : empty_guns ) {
        compare_weapon( *i, find_usable_ammo( *i ) );
    }

    if( best == &weapon ) {
        add_msg( m_debug, "Wielded %s is best at %.1f, not switching",
                 best->display_name().c_str(), best_value );
        return false;
    }

    add_msg( m_debug, "Wielding %s at value %.1f",
             best->display_name().c_str(), best_value );

    wield( *best );
    return true;
}

bool npc::scan_new_items()
{
    add_msg( m_debug, "%s scanning new items", name.c_str() );
    if( !wield_better_weapon() ) {
        // Stop "having new items" when you no longer do anything with them
        has_new_items = false;
        return true;
    }

    return false;
    // TODO: Armor?
}

void npc::wield_best_melee()
{
    double best_value = 0.0;
    item *it = inv.best_for_melee( *this, best_value );
    if( unarmed_value() >= best_value ) {
        // "I cast fist!"
        it = &ret_null;
    }

    wield( *it );
}

void npc::alt_attack()
{
    itype_id which = "null";
    Creature *critter = get_target( ai_cache.target );
    if( critter == nullptr ) {
        // This function shouldn't be called...
        debugmsg( "npc::alt_attack() called with target = %d", ai_cache.target );
        move_pause();
        return;
    }

    tripoint tar = critter->pos();

    int dist = rl_dist( pos(), tar );
    /* ALT_ATTACK_ITEMS is an array which stores the itype_id of all alternate
     * items, from least to most important.
     * See npc.h for definition of ALT_ATTACK_ITEMS
     */
    for( auto &elem : ALT_ATTACK_ITEMS ) {
        if( ( !is_following() || rules.use_grenades ) && has_amount( elem, 1 ) ) {
            which = elem;
        }
    }

    if (which == "null") { // We ain't got shit!
        // Not sure if this should ever occur.  For now, let's warn with a debug msg
        debugmsg("npc::alt_attack() couldn't find an alt attack item!");
        if (dist == 1) {
            melee_attack( *critter, true );
        } else {
            move_to( tar );
        }
    }

    int weapon_index = INT_MIN;
    item *used = nullptr;
    if (weapon.type->id == which) {
        used = &weapon;
        weapon_index = -1;
    } else {
        invslice slice = inv.slice();
        for (size_t i = 0; i < inv.size(); i++) {
            if (slice[i]->front().type->id == which) {
                used = &(slice[i]->front());
                weapon_index = i;
            }
        }
    }

    if( used == nullptr ) {
        debugmsg( "npc::alt_attack() couldn't find expected item of type %s",
                  which.c_str() );
        return;
    }

    // Are we going to throw this item?
    if( !thrown_item( *used ) ) {
        activate_item( weapon_index );
        return;
    }

    // We are throwing it!
    int conf = confident_throw_range( *used );
    const bool wont_hit = wont_hit_friend( tar, weapon_index );
    if( dist <= conf && wont_hit ) {
        if( g->u.sees( *this ) ) {
            add_msg(_("%1$s throws a %2$s."),
                    name.c_str(), used->tname().c_str());
        }

        int stack_size = -1;
        if( used->count_by_charges() ) {
            stack_size = used->charges;
            used->charges = 1;
        }
        throw_item( tar, *used );
        // Throw a single charge of a stacking object.
        if( stack_size == -1 || stack_size == 1 ) {
            i_rem(weapon_index);
        } else {
            used->charges = stack_size - 1;
        }

        return;
    }

    if( wont_hit ) {
        // Within this block, our chosen target is outside of our range
        update_path( tar );
        move_to_next(); // Move towards the target
    }

    // Danger of friendly fire
    if( !used->active || used->charges > 2 ) {
        // Safe to hold on to, for now
        // Maneuver around player
        avoid_friendly_fire();
        return;
    }

    // We need to throw this live (grenade, etc) NOW! Pick another target?
    for( int dist = 2; dist <= conf; dist++ ) {
        for( const tripoint &pt : g->m.points_in_radius( pos(), dist ) ) {
            int newtarget = g->mon_at( pt );
            int newdist = rl_dist( pos(), pt );
            // TODO: Change "newdist >= 2" to "newdist >= safe_distance(used)"
            if( newdist <= conf && newdist >= 2 && newtarget != -1 &&
                wont_hit_friend( pt, weapon_index ) ) {
                // Friendlyfire-safe!
                ai_cache.target = newtarget;
                if( !one_in( 100 ) ) {
                    // Just to prevent infinite loops...
                    alt_attack();
                }
                return;
            }
        }
    }
    /* If we have reached THIS point, there's no acceptable monster to throw our
     * grenade or whatever at.  Since it's about to go off in our hands, better to
     * just chuck it as far away as possible--while being friendly-safe.
     */
    int best_dist = 0;
    for (int dist = 2; dist <= conf; dist++) {
        for( const tripoint &pt : g->m.points_in_radius( pos(), dist ) ) {
            int new_dist = rl_dist( pos(), pt );
            if( new_dist > best_dist && wont_hit_friend( pt ), weapon_index ) {
                best_dist = new_dist;
                tar = pt;
            }
        }
    }
    /* Even if tar.x/tar.y didn't get set by the above loop, throw it anyway.  They
     * should be equal to the original location of our target, and risking friendly
     * fire is better than holding on to a live grenade / whatever.
     */
    if( g->u.sees( *this ) ) {
        add_msg(_("%1$s throws a %2$s."), name.c_str(),
                used->tname().c_str());
    }

    int stack_size = -1;
    if( used->count_by_charges() ) {
        stack_size = used->charges;
        used->charges = 1;
    }
    throw_item( tar, *used );

    // Throw a single charge of a stacking object.
    if( stack_size == -1 || stack_size == 1 ) {
        i_rem(weapon_index);
    } else {
        used->charges = stack_size - 1;
        if( used->charges == 0 ) {
            i_rem( weapon_index );
        }
    }
}

void npc::activate_item(int item_index)
{
    const int oldmoves = moves;
    item *it = &i_at(item_index);
    if( it->is_tool() || it->is_food() ) {
        it->type->invoke( this, it, pos() );
    }

    if( moves == oldmoves ) {
        // A hack to prevent debugmsgs when NPCs activate 0 move items
        // while not removing the debugmsgs for other 0 move actions
        moves--;
    }
}

bool thrown_item( item &used )
{
    const itype_id &type = used.type->id;
    // TODO: Remove the horrid hardcode
    return (used.active || type == "knife_combat" || type == "spear_wood");
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
        add_msg( _("%1$s heals %2$s."), name.c_str(), patient.name.c_str() );
    }

    item &used = get_healing_item();
    if( used.is_null() ) {
        debugmsg( "%s tried to heal you but has no healing item", disp_name().c_str() );
        return;
    }

    long charges_used = used.type->invoke( this, &used, patient.pos(), "heal" );
    consume_charges( used, charges_used );

    if( !patient.is_npc() ) {
        // Test if we want to heal the player further
        if (op_of_u.value * 4 + op_of_u.trust + personality.altruism * 3 +
            (fac_has_value(FACVAL_CHARITABLE) ?  5 : 0) +
            (fac_has_job  (FACJOB_DOCTORS)    ? 15 : 0) - op_of_u.fear * 3 <  25) {
            attitude = NPCATT_FOLLOW;
            say(_("That's all the healing I can do."));
        } else {
            say(_("Hold still, I can heal you more."));
        }
    }
}

void npc::heal_self()
{
    item &used = get_healing_item();
    if( used.is_null() ) {
        debugmsg( "%s tried to heal self but has no healing item", disp_name().c_str() );
        return;
    }

    if( g->u.sees( *this ) ) {
        add_msg( _("%s applies a %s"), name.c_str(), used.tname().c_str() );
    }

    long charges_used = used.type->invoke( this, &used, pos(), "heal" );
    consume_charges( used, charges_used );
}

void npc::use_painkiller()
{
    // First, find the best painkiller for our pain level
    item *it = inv.most_appropriate_painkiller( get_pain() );

    if (it->is_null()) {
        debugmsg("NPC tried to use painkillers, but has none!");
        move_pause();
    } else {
        if (g->u.sees( *this )) {
            add_msg(_("%1$s takes some %2$s."), name.c_str(), it->tname().c_str());
        }
        consume(inv.position_by_item(it));
        moves = 0;
    }
}

// We want our food to:
// Provide enough nutrition and quench
// Not provide too much of either (don't waste food)
// Not be unhealthy
// Not have side effects
// Be eaten before it rots (favor soon-to-rot perishables)
float rate_food( const item &it, int want_nutr, int want_quench )
{
    const auto food = it.type->comestible.get();
    if( food == nullptr ) {
        // Not food
        return 0.0f;
    }

    if( food->parasites ) {
        return 0.0;
    }

    int nutr = food->nutr;
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
        // hallu shrooms (NPCs don't hallucinate, so don't eat those)
        // honeycomb (harmless iuse)
        // royal jelly (way too expensive to eat as food)
        // mutagenic crap (don't eat, we want player to micromanage muties)
        // marloss (NPCs don't turn fungal)
        // weed brownies (small debuff)
        // seeds (too expensive)

        // For now skip all of those
        return 0.0f;
    }

    if( it.rotten() ) {
        // TODO: Allow sapro mutants to eat it anyway and make them prefer it
        return 0.0f;
    }

    float relative_rot = it.get_relative_rot();
    float weight = std::max( 1.0f, 10.0f * relative_rot );
    if( food->fun < 0 ) {
        // This helps to avoid eating stuff like flour
        weight /= (-food->fun) + 1;
    }

    if( food->healthy < 0 ) {
        weight /= (-food->healthy) + 1;
    }

    // Avoid wasting quench values unless it's about to rot away
    if( relative_rot < 0.9f && quench > want_quench ) {
        weight -= (1.0f - relative_rot) * (quench - want_quench);
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
        float cur_weight = it.is_food_container() ?
            rate_food( it.contents[0], want_hunger, want_quench ) :
            rate_food( it, want_hunger, want_quench );
        if( cur_weight > best_weight ) {
            best_weight = cur_weight;
            index = i;
        }
    }

    if( index == -1 ) {
        if( !is_friend() ) {
            // TODO: Remove this and let player "exploit" hungry NPCs
            set_hunger( 0 );
            set_thirst( 0 );
        }
        return false;
    }

    return consume( index );
}

void npc::mug_player(player &mark)
{
    if( rl_dist( pos(), mark.pos() ) > 1 ) { // We have to travel
        update_path( mark.pos() );
        move_to_next();
        return;
    }

    const bool u_see = g->u.sees( *this ) || g->u.sees( mark );
    if (mark.cash > 0) {
        cash += mark.cash;
        mark.cash = 0;
        moves = 0;
        // Describe the action
        if( mark.is_npc() ) {
            if( u_see ) {
                add_msg(_("%1$s takes %2$s's money!"),
                        name.c_str(), mark.name.c_str());
            }
        } else {
            add_msg(m_bad, _("%s takes your money!"), name.c_str());
        }
        return;
    }

    // We already have their money; take some goodies!
    // value_mod affects at what point we "take the money and run"
    // A lower value means we'll take more stuff
    double value_mod = 1 - double((10 - personality.bravery)    * .05) -
                       double((10 - personality.aggression) * .04) -
                       double((10 - personality.collector)  * .06);
    if (!mark.is_npc()) {
        value_mod += double(op_of_u.fear * .08);
        value_mod -= double((8 - op_of_u.value) * .07);
    }
    int best_value = minimum_item_value() * value_mod;
    int item_index = INT_MIN;
    invslice slice = mark.inv.slice();
    for (size_t i = 0; i < slice.size(); i++) {
        if( value(slice[i]->front()) >= best_value &&
            can_pickVolume( slice[i]->front().volume(), true ) &&
            can_pickWeight( slice[i]->front().weight(), true ) ) {
            best_value = value(slice[i]->front());
            item_index = i;
        }
    }
    if (item_index == INT_MIN) { // Didn't find anything worthwhile!
        attitude = NPCATT_FLEE;
        if (!one_in(3)) {
            say("<done_mugging>");
        }
        moves -= 100;
        return;
    }

    item stolen = mark.i_rem(item_index);
    if (mark.is_npc()) {
        if (u_see) {
            add_msg(_("%1$s takes %2$s's %3$s."), name.c_str(),
                    mark.name.c_str(),
                    stolen.tname().c_str());
        }
    } else {
        add_msg(m_bad, _("%1$s takes your %2$s."),
                name.c_str(), stolen.tname().c_str());
    }
    i_add(stolen);
    moves -= 100;
    if (!mark.is_npc()) {
        op_of_u.value -= rng(0, 1);    // Decrease the value of the player
    }
}

void npc::look_for_player(player &sought)
{
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
    for (int x = 1; x < SEEX * MAPSIZE; x += 11) { // 1, 12, 23, 34
        for (int y = 1; y < SEEY * MAPSIZE; y += 11) {
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
    return ( last_player_seen_pos.x >= 0 && last_player_seen_pos.x < SEEX * MAPSIZE &&
             last_player_seen_pos.y >= 0 && last_player_seen_pos.y < SEEY * MAPSIZE &&
             last_seen_player_turn > 0 );
}

bool npc::has_destination() const
{
    return goal != no_goal_point;
}

void npc::reach_destination()
{
    // Guarding NPCs want a specific point, not just an overmap tile
    // Rest stops having a goal after reaching it
    if( !is_guarding() ) {
        goal = no_goal_point;
        return;
    }

    // If we are guarding, remember our position in case we get forcibly moved
    goal = global_omt_location();
    if( guard_pos == global_square_location() ) {
        // This is the specific point
        return;
    }

    if( path.size() > 1 ) {
        // No point recalculating the path to get home
        move_to_next();
    } else if( guard_pos != no_goal_point ) {
        const tripoint dest( guard_pos.x - mapx * SEEX,
                             guard_pos.y - mapy * SEEY,
                             guard_pos.z );
        update_path( dest );
        move_to_next();
    } else {
        guard_pos = global_square_location();
    }
}

void npc::set_destination()
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
    if( is_guarding() ) {
        guard_current_pos();
        return;
    }

    // all of the following luxuries are at ground level.
    // so please wallow in hunger & fear if below ground.
    if( posz() != 0 && !g->m.has_zlevels() ) {
        goal = no_goal_point;
        return;
    }

    decide_needs();
    if (needs.empty()) { // We don't need anything in particular.
        needs.push_back(need_none);
    }
    std::vector<std::string> options;
    switch(needs[0]) {
    case need_ammo:
        options.push_back("house");
    case need_gun:
        options.push_back("s_gun");
        break;

    case need_weapon:
        options.push_back("s_gun");
        options.push_back("s_sports");
        options.push_back("s_hardware");
        break;

    case need_drink:
        options.push_back("s_gas");
        options.push_back("s_pharm");
        options.push_back("s_liquor");
    case need_food:
        options.push_back("s_grocery");
        break;

    default:
        options.push_back("house");
        options.push_back("s_gas");
        options.push_back("s_pharm");
        options.push_back("s_hardware");
        options.push_back("s_sports");
        options.push_back("s_liquor");
        options.push_back("s_gun");
        options.push_back("s_library");
    }

    const std::string dest_type = random_entry( options );

    // We need that, otherwise find_closest won't work properly
    // TODO: Allow finding sewers and stuff
    tripoint surface_omt_loc = global_omt_location();
    surface_omt_loc.z = 0;

    goal = overmap_buffer.find_closest( surface_omt_loc, dest_type, 0, false );
    add_msg( m_debug, "New goal: %s at %d,%d,%d", dest_type.c_str(), goal.x, goal.y, goal.z );
}

void npc::go_to_destination()
{
    if( goal == no_goal_point ) {
        add_msg( m_debug, "npc::go_to_destination with no goal" );
        move_pause();
        reach_destination();
        return;
    }

    const tripoint omt_pos = global_omt_location();
    int sx = sgn( goal.x - omt_pos.x );
    int sy = sgn( goal.y - omt_pos.y );
    const int minz = std::min( goal.z, posz() );
    const int maxz = std::max( goal.z, posz() );
    add_msg( m_debug, "%s going (%d,%d,%d)->(%d,%d,%d)", name.c_str(),
             omt_pos.x, omt_pos.y, omt_pos.z, goal.x, goal.y, goal.z );
    if( goal == omt_pos ) {
        // We're at our desired map square!
        reach_destination();
        return;
    }

    if( !path.empty() &&
        sgn( path.back().x - posx() ) == sx &&
        sgn( path.back().y - posy() ) == sy &&
        sgn( path.back().z - posz() ) == sgn( goal.z - posz() ) ) {
        // We're moving in the right direction, don't find a different path
        move_to_next();
        return;
    }

    if( sx == 0 && sy == 0 && goal.z != posz() ) {
        // Make sure we always have some points to check
        sx = rng( -1, 1 );
        sy = rng( -1, 1 );
    }

    // sx and sy are now equal to the direction we need to move in
    tripoint dest( posx() + 8 * sx, posy() + 8 * sy, goal.z );
    for( int i = 0; i < 8; i++ ) {
        if( ( g->m.passable( dest ) ||
             //Needs 20% chance of bashing success to be considered for pathing
             g->m.bash_rating( smash_ability(), dest ) >= 2 ||
             g->m.open_door( dest, true, true ) ) &&
            ( one_in( 4 ) || sees( dest ) ) ) {
            update_path( dest );
            if( !path.empty() && can_move_to( path[0] ) ) {
                move_to_next();
                return;
            } else {
                move_pause();
                return;
            }
        }

        dest = tripoint( posx() + rng( 0, 16 ) * sx, posy() + rng( 0, 16 ) * sy, rng( minz, maxz ) );
    }
    move_pause();
}

void npc::guard_current_pos()
{
    goal = global_omt_location();
    guard_pos = global_square_location();
}

std::string npc_action_name(npc_action action)
{
    switch (action) {
    case npc_undecided:
        return _("Undecided");
    case npc_pause:
        return _("Pause");
    case npc_reload:
        return _("Reload");
    case npc_sleep:
        return _("Sleep");
    case npc_pickup:
        return _("Pick up items");
    case npc_escape_item:
        return _("Use escape item");
    case npc_wield_melee:
        return _("Wield melee weapon");
    case npc_wield_loaded_gun:
        return _("Wield loaded gun");
    case npc_wield_empty_gun:
        return _("Wield empty gun");
    case npc_heal:
        return _("Heal self");
    case npc_use_painkiller:
        return _("Use painkillers");
    case npc_drop_items:
        return _("Drop items");
    case npc_flee:
        return _("Flee");
    case npc_melee:
        return _("Melee");
    case npc_reach_attack:
        return _("Reach attack");
    case npc_aim:
        return _("Aim");
    case npc_shoot:
        return _("Shoot");
    case npc_shoot_burst:
        return _("Fire a burst");
    case npc_alt_attack:
        return _("Use alternate attack");
    case npc_look_for_player:
        return _("Look for player");
    case npc_heal_player:
        return _("Heal player");
    case npc_follow_player:
        return _("Follow player");
    case npc_follow_embarked:
        return _("Follow player (embarked)");
    case npc_talk_to_player:
        return _("Talk to player");
    case npc_mug_player:
        return _("Mug player");
    case npc_goto_destination:
        return _("Go to destination");
    case npc_avoid_friendly_fire:
        return _("Avoid friendly fire");
    default:
        return "Unnamed action";
    }
}

void print_action( const char *prepend, npc_action action )
{
    if( action != npc_undecided ) {
        add_msg( m_debug, prepend, npc_action_name( action ).c_str() );
    }
}

Creature *npc::current_target() const
{
    return get_target( ai_cache.target );
}

Creature *npc::get_target( int target ) const
{
    if( target == TARGET_PLAYER && !is_following() ) {
        return &g->u;
    } else if( target >= 0 && g->num_zombies() > (size_t)target ) {
        return &g->zombie( target );
    } else if( target == TARGET_NONE ) {
        return nullptr;
    }

    // Should actually return a NPC, but those aren't well supported yet
    return nullptr;
}

// Maybe TODO: Move to Character method and use map methods
body_part bp_affected( npc &who, const efftype_id &effect_type )
{
    body_part ret = num_bp;
    int highest_intensity = INT_MIN;
    for( int i = 0; i < num_bp; i++ ) {
        body_part bp = body_part( i );
        const auto &eff = who.get_effect( effect_type, bp );
        if( !eff.is_null() && eff.get_intensity() > highest_intensity ) {
            ret = bp;
            highest_intensity = eff.get_intensity();
        }
    }

    return ret;
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
    // TODO: Allow calling for help when scared
    if( !is_following() || !g->u.sees( *this ) ) {
        return false;
    }

    // Don't wake player up with non-serious complaints
    const bool do_complain = rules.allow_complain && !g->u.in_sleep_state();

    // When infected, complain every (4-intensity) hours
    // At intensity 3, ignore player wanting us to shut up
    if( has_effect( effect_infected ) ) {
        body_part bp = bp_affected( *this, effect_infected );
        const auto &eff = get_effect( effect_infected, bp );
        if( complaints[infected_string] < calendar::turn - HOURS(4 - eff.get_intensity()) &&
            (do_complain || eff.get_intensity() >= 3) ) {
            say( _("My %s wound is infected..."), body_part_name( bp ).c_str() );
            complaints[infected_string] = calendar::turn;
            // Only one complaint per turn
            return true;
        }
    }

    // When bitten, complain every hour, but respect restrictions
    if( has_effect( effect_bite ) ) {
        body_part bp = bp_affected( *this, effect_bite );
        if( do_complain &&
            complaints[bite_string] < calendar::turn - HOURS(1) ) {
            say( _("The bite wound on my %s looks bad."), body_part_name( bp ).c_str() );
            complaints[bite_string] = calendar::turn;
            return true;
        }
    }

    // When tired, complain every 30 minutes
    // If massively tired, ignore restrictions
    if( get_fatigue() > TIRED &&
        complaints[fatigue_string] < calendar::turn - MINUTES(30) &&
        (do_complain || get_fatigue() > MASSIVE_FATIGUE - 100) ) {
        say( "<yawn>" );
        complaints[fatigue_string] = calendar::turn;
        return true;
    }

    // Radiation every 10 minutes
    if( radiation > 90 &&
        complaints[radiation_string] < calendar::turn - MINUTES(10) &&
        (do_complain || radiation > 150) ) {
        say( _("I'm suffering from radiation sickness...") );
        complaints[radiation_string] = calendar::turn;
        return true;
    }

    // Hunger every 3-6 hours
    // Since NPCs can't starve to death, respect the rules
    if( get_hunger() > 160 &&
        complaints[hunger_string] < calendar::turn - std::max( HOURS(3), MINUTES(60*8 - get_hunger()) ) &&
        do_complain ) {
        say( _("<hungry>") );
        complaints[hunger_string] = calendar::turn;
        return true;
    }

    // Thirst every 2 hours
    // Since NPCs can't dry to death, respect the rules
    if( get_thirst() > 80
        && complaints[thirst_string] < calendar::turn - HOURS(2) &&
        do_complain ) {
        say( _("<thirsty>") );
        complaints[thirst_string] = calendar::turn;
        return true;
    }

    return false;
}

void npc::do_reload( item &it )
{
    auto usable_ammo = find_usable_ammo( it );

    if( !usable_ammo ) {
        debugmsg( "do_reload failed: no usable ammo" );
        return;
    }

    long qty = std::max( 1l, std::min( usable_ammo->charges, it.ammo_capacity() - it.ammo_remaining() ) );
    int reload_time = item_reload_cost( it, *usable_ammo, qty );
    if( !it.reload( *this, std::move( usable_ammo ), qty ) ) {
        debugmsg( "do_reload failed: item could not be reloaded" );
        return;
    }

    moves -= reload_time;
    recoil = MIN_RECOIL;

    if( g->u.sees( *this ) ) {
        add_msg( _( "%1$s reloads their %2$s." ), name.c_str(), it.tname().c_str() );
        sfx::play_variant_sound( "reload", it.typeId(), sfx::get_heard_volume( pos() ),
                                 sfx::get_heard_angle( pos() ) );
    }
}
