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
#define NUM_ALT_ATTACK_ITEMS 18
itype_id ALT_ATTACK_ITEMS[NUM_ALT_ATTACK_ITEMS] = {
    "knife_combat", "spear_wood", "molotov", "pipebomb", "grenade",
    "gasbomb", "bot_manhack", "tazer", "dynamite", "granade", "mininuke",
    "molotov_lit", "pipebomb_act", "grenade_act", "gasbomb_act",
    "dynamite_act", "granade_act", "mininuke_act"
};
#endif

const int avoidance_vehicles_radius = 5;

std::string npc_action_name(npc_action action);
bool thrown_item(item *used);

// Used in npc::drop_items()
struct ratio_index {
    double ratio;
    int index;
    ratio_index(double R, int I) : ratio (R), index (I) {};
};

bool npc::is_dangerous_field( const field_entry &fld ) const
{
    switch( fld.getFieldType() ) {
        case fd_smoke:
            return get_env_resist( bp_mouth ) < 7;
        case fd_tear_gas:
        case fd_toxic_gas:
        case fd_gas_vent:
        case fd_relax_gas:
            return get_env_resist( bp_mouth ) < 15;
        case fd_fungal_haze:
            if( has_trait( "M_IMMUNE" ) ) {
                return false;
            }
            return get_env_resist( bp_mouth ) < 15 || get_env_resist( bp_eyes ) < 15;
        default:
            return fld.is_dangerous();
    }
}

bool npc::sees_dangerous_field( const tripoint &p ) const
{
    auto &fields = g->m.field_at( p );
    for( auto & fld : fields ) {
        if( is_dangerous_field( fld.second ) ) {
            return true;
        }
    }
    return false;
}

bool npc::could_move_onto( const tripoint &p ) const
{
    return g->m.passable( p ) && !sees_dangerous_field( p );
}

// class npc functions!

void npc::move()
{
    npc_action action = npc_undecided;
    int danger = 0;
    int total_danger = 0;
    int target = INT_MIN;

    choose_monster_target(target, danger, total_danger);
    add_msg( m_debug, "NPC %s: target = %d, danger = %d, range = %d",
                 name.c_str(), target, danger, confident_range(-1));

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
    if( sees_dangerous_field( pos3() ) ) {
        auto targets = closest_tripoints_first( 1, pos3() );
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

    if( is_enemy() ) {
        int pl_danger = player_danger( &(g->u) );
        if( ( pl_danger > danger || rl_dist( pos3(), g->u.pos3() ) <= 1 ) || target == INT_MIN ) {
            target = TARGET_PLAYER;
            danger = pl_danger;
            add_msg( m_debug, "NPC %s: Set target to PLAYER, danger = %d", name.c_str(), danger );
        }
    }
    // TODO: Place player-aiding actions here, with a weight

    /* NPCs are fairly suicidal so at this point we will do a quick check to see if
     * something nasty is going to happen.
     */

    if( is_enemy() && vehicle_danger(avoidance_vehicles_radius) > 0 ) {
        // TODO: Think about how this actually needs to work, for now assume flee from player
        target = TARGET_PLAYER;
    }

    // TODO: morale breaking when surrounded by hostiles
    //if (!bravery_check(danger) || !bravery_check(total_danger) ||
    // TODO: near by active explosives spotted

    if( target == TARGET_PLAYER && attitude == NPCATT_FLEE ) {
        action = method_of_fleeing(target);
    } else if( ( target != INT_MIN && danger > 0 ) ||
               (target == TARGET_PLAYER && attitude == NPCATT_KILL) ) {
        action = method_of_attack(target, danger);
    }

    else { // No present danger
        action = address_needs(danger);
        if( action != npc_undecided ) {
            add_msg( m_debug, "address_needs %s", npc_action_name(action).c_str() );
        }
        if (action == npc_undecided) {
            action = address_player();
            if( action != npc_undecided ) {
                add_msg( m_debug, "address_player %s", npc_action_name(action).c_str() );
            }
        }

        if (action == npc_undecided) {
            if (mission == NPC_MISSION_SHELTER || mission == NPC_MISSION_BASE || mission == NPC_MISSION_SHOPKEEP
                || mission == NPC_MISSION_GUARD || has_effect("infection")) {
                action = npc_pause;
            } else if (has_new_items) {
                action = scan_new_items(target);
            } else if (!fetching_item) {
                find_item();
            }

            if( action != npc_undecided ) {
                add_msg( m_debug, "find_item %s", npc_action_name(action).c_str() );
            }
            // check if in vehicle before rushing off to fetch things
            if (is_following() && g->u.in_vehicle) {
                action = npc_follow_embarked;
            } else if (fetching_item) { // Set to true if find_item() found something
                action = npc_pickup;
            } else if (is_following()) { // No items, so follow the player?
                action = npc_follow_player;
            } else { // Do our long-term action
                action = long_term_goal_action();
            }

            if( action != npc_undecided ) {
                add_msg( m_debug, "long_term_goal_action %s", npc_action_name(action).c_str() );
            }
        }
    }

    /* Sometimes we'll be following the player at this point, but close enough that
     * "following" means standing still.  If that's the case, if there are any
     * monsters around, we should attack them after all!
     *
     * If we are following a embarked player and we are in a vehicle then shoot anyway
     * as we are most likely riding shotgun
     */
    if( danger > 0 && target != INT_MIN &&
        (
          ( action == npc_follow_embarked && in_vehicle) ||
          ( action == npc_follow_player &&
            ( rl_dist( pos(), g->u.pos() ) <= follow_distance() || posz() != g->u.posz() ) )
        ) ) {
        action = method_of_attack( target, danger );
    }

    add_msg( m_debug, "%s chose action %s.", name.c_str(), npc_action_name( action ).c_str() );

    execute_action( action, target );
}

void npc::execute_action(npc_action action, int target)
{
    int oldmoves = moves;
    tripoint tar = pos3();
    if( target == TARGET_PLAYER ) {
        tar = g->u.pos3();
    } else if( target >= 0 && g->num_zombies() > (size_t)target ) {
        tar = g->zombie(target).pos3();
    }
    /*
      debugmsg("%s ran execute_action() with target = %d! Action %s",
               name.c_str(), target, npc_action_name(action).c_str());
    */

    /*
    // Before doing anything else, if the player is friendly and we're
    // blocking them, get out of the way.
    if( attitude_to(g->u) == Creature::A_FRIENDLY && rl_dist(pos(), g->u.pos()) == 1 ) {
        // We are only blocking movement if we're one tile away from the player.
        if(is_blocking_position(g->u.pos())) {
            move_away_from(g->u.pos());
            return;
        }
    }
    // Disabled because it causes problems with vehicles
    // Just use push/swap manually
    */

    std::vector<tripoint> line;
    if( tar != pos3() ) {
        line = g->m.find_clear_path( pos3(), tar );
    }

    switch (action) {

    case npc_pause:
        move_pause();
        break;

    case npc_reload: {
        moves -= weapon.reload_time(*this);
        int ammo_index = weapon.pick_reload_ammo(*this, false);
        if (!weapon.reload(*this, ammo_index)) {
            debugmsg("NPC reload failed.");
        }
        recoil = MIN_RECOIL;
        if (g->u.sees( *this )) {
            add_msg(_("%1$s reloads their %2$s."), name.c_str(),
                    weapon.tname().c_str());
            sfx::play_variant_sound( "reload", weapon.typeId(), sfx::get_heard_volume(pos3()), sfx::get_heard_angle( pos3()));
        }
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
            if( !has_effect( "lying_down" ) ) {
                add_effect( "lying_down", 300, num_bp, false, 1 );
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
            wield(it);
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
            wield(&(slice[index]->front()));
        }
    }
    break;

    case npc_heal:
        heal_self();
        break;

    case npc_use_painkiller:
        use_painkiller();
        break;

    case npc_eat:
        pick_and_eat();
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

    case npc_melee:
        update_path( tar );
        if( path.size() > 1 ) {
            move_to_next();
        } else if( path.size() == 1 ) {
            Creature *critter = get_target( target );
            if( critter != nullptr ) {
                melee_attack( *critter, true );
            }
        } else {
            look_for_player(g->u);
        }
        break;

    case npc_shoot:
        fire_gun( tar, false );
        break;

    case npc_shoot_burst:
        fire_gun( tar, true );
        break;

    case npc_alt_attack:
        alt_attack(target);
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
        update_path( g->u.pos3() );
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
        update_path( g->u.pos3() );
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
        avoid_friendly_fire(target);
        break;

    case npc_base_idle:
        // TODO: patrol or sleep or something?
        move_pause();
        break;

    case npc_undecided:
        move_pause();
        break;

    default:
        debugmsg("Unknown NPC action (%d)", action);
    }

    if( oldmoves == moves ) {
        dbg(D_ERROR) << "map::execute_action: NPC didn't use its moves.";
        debugmsg("NPC didn't use its moves.  Action %d.  Turning on debug mode.", action);
        debug_mode = true;
    }
}

void npc::choose_monster_target(int &enemy, int &danger,
                                int &total_danger)
{
    bool defend_u = sees( g->u ) && is_defending();
    int highest_priority = 0;
    total_danger = 0;

    for (size_t i = 0; i < g->num_zombies(); i++) {
        monster *mon = &(g->zombie(i));
        if( !sees( *mon ) ) {
            continue;
        }

        int distance = (100 * rl_dist(pos(), mon->pos())) / mon->get_speed();
        double hp_percent = (mon->get_hp_max() - mon->get_hp()) / mon->get_hp_max();
        int priority = mon->type->difficulty * (1 + hp_percent) - distance;
        int monster_danger = (mon->type->difficulty * mon->get_hp()) / mon->get_hp_max();

        auto att = mon->attitude( this );
        if( att == MATT_FRIEND || att == MATT_FPASSIVE ) {
            priority = -999;
            monster_danger *= -1;
        } else if( att == MATT_ATTACK ) {
            monster_danger++;
        }

        total_danger += int(monster_danger / (distance == 0 ? 1 : distance));

        bool okay_by_rules = true;
        if (is_following()) {
            switch (rules.engagement) {
            case ENGAGE_NONE:
                okay_by_rules = false;
                break;
            case ENGAGE_CLOSE:
                okay_by_rules = (distance <= 6);
                break;
            case ENGAGE_WEAK:
                okay_by_rules = (mon->get_hp() <= average_damage_dealt());
                break;
            case ENGAGE_HIT:
                okay_by_rules = (mon->has_effect("hit_by_player"));
                break;
            case ENGAGE_ALL:
                okay_by_rules = true;
                break;
            }
        }

        if( !okay_by_rules ) {
            continue;
        }

        if( monster_danger > danger && priority > 0 ) {
            danger = monster_danger;
        }

        if( priority > highest_priority ) {
            highest_priority = priority;
            enemy = i;
        } else if( defend_u ) {
            priority = mon->type->difficulty * (1 + hp_percent);
            distance = (100 * rl_dist(g->u.pos(), mon->pos())) /
                mon->get_speed();
            priority -= distance;
            if( mon->get_speed() < get_speed() ) {
                priority -= 10;
            }
            priority *= (personality.bravery + personality.altruism + op_of_u.value) /
                        15;
            if (priority > highest_priority) {
                highest_priority = priority;
                enemy = i;
            }
        }
    }
}

npc_action npc::method_of_fleeing(int enemy)
{
    int speed = (enemy == TARGET_PLAYER ? g->u.get_speed() :
                 g->zombie(enemy).get_speed());
    tripoint enemy_loc = enemy == TARGET_PLAYER ?
        g->u.pos3() : g->zombie(enemy).pos3();
    int distance = rl_dist(pos(), enemy_loc);

    if (choose_escape_item() != INT_MIN) { // We have an escape item!
        return npc_escape_item;
    }

    if (speed > 0 && (100 * distance) / speed <= 4 && speed > get_speed()) {
        return method_of_attack(enemy, -1);    // Can't outrun, so attack
    }

    return npc_flee;
}

npc_action npc::method_of_attack(int target, int danger)
{
    tripoint tar;
    bool can_use_gun = (!is_following() || rules.use_guns);
    bool use_silent = (is_following() && rules.use_silent);

    Creature *critter = get_target( target );
    if( critter != nullptr ) {
        tar = critter->pos();
    } else { // This function shouldn't be called...
        debugmsg("Ran npc::method_of_attack without a target!");
        return npc_pause;
    }

    int dist = rl_dist( pos(), tar );
    int target_HP;
    if (target == TARGET_PLAYER) {
        target_HP = g->u.hp_percentage() * g->u.hp_max[hp_torso];
    } else {
        target_HP = g->zombie(target).get_hp();
    }

    if (can_use_gun) {
        if (need_to_reload() && can_reload()) {
            return npc_reload;
        }
        if (emergency(danger_assessment()) && alt_attack_available()) {
            return npc_alt_attack;
        }
        if (weapon.is_gun() && (!use_silent || weapon.is_silent()) && weapon.charges > 0) {
            if (dist > confident_range()) {
                if (can_reload() && (enough_time_to_reload(target, weapon) || in_vehicle)) {
                    return npc_reload;
                } else if (in_vehicle && dist > 1) {
                    return npc_pause;
                } else {
                    return npc_melee;
                }
            }
            if (!wont_hit_friend( tar )) {
                if (in_vehicle)
                    if (can_reload()) {
                        return npc_reload;
                    } else {
                        return npc_pause;    // wait for clear shot
                    }
                else {
                    return npc_avoid_friendly_fire;
                }
            } else if (target == TARGET_PLAYER && !sees( g->u )) {
                return npc_melee;//Can't see target
            } else if (rl_dist( pos3(), tar ) > weapon.gun_range( this ) &&
                       sees( tar )) {
                return npc_melee; // If out of range, move closer to the target
            } else if (dist <= confident_range() / 3 && weapon.charges >= weapon.type->gun->burst &&
                       weapon.type->gun->burst > 1 &&
                       ((weapon.has_curammo() && target_HP >= weapon.get_curammo()->ammo->damage * 3) ||
                        emergency(danger * 2))) {
                return npc_shoot_burst;
            } else {
                return npc_shoot;
            }
        }
    }

    // Check if there's something better to wield
    bool has_empty_gun = false, has_better_melee = false;
    std::vector<item *> empty_guns;
    invslice slice = inv.slice();
    for (auto &i : slice) {
        item &it = i->front();
        bool allowed = can_use_gun && it.is_gun() && (!use_silent || weapon.is_silent());
        if (allowed && it.charges > 0) {
            return npc_wield_loaded_gun;
        } else if (allowed && enough_time_to_reload(target, it)) {
            has_empty_gun = true;
            empty_guns.push_back(&it);
        } else if( melee_value( it ) > melee_value( weapon ) * 1.1 ) {
            has_better_melee = true;
        }
    }

    bool has_ammo_for_empty_gun = false;
    for (auto &i : empty_guns) {
        for (auto &j : slice) {
            item &it = j->front();
            if (it.is_ammo() &&
                it.ammo_type() == i->ammo_type()) {
                has_ammo_for_empty_gun = true;
            }
        }
    }

    if (has_empty_gun && has_ammo_for_empty_gun) {
        return npc_wield_empty_gun;
    } else if( has_better_melee ) {
        return npc_wield_melee;
    } else if( unarmed_value() > melee_value( weapon ) * 1.1 ) {
        return npc_wield_melee;
    }

    if (in_vehicle && dist > 1) {
        return npc_pause;
    }
    return npc_melee;
}

npc_action npc::address_needs(int danger)
{
    if (has_healing_item()) {
        for (int i = 0; i < num_hp_parts; i++) {
            hp_part part = hp_part(i);
            if ((part == hp_head  && hp_cur[i] <= 35) ||
                (part == hp_torso && hp_cur[i] <= 25) ||
                hp_cur[i] <= 15) {
                return npc_heal;
            }
        }
    }

    if (has_painkiller() && !took_painkiller() && pain - pkill >= 15) {
        return npc_use_painkiller;
    }

    if (can_reload()) {
        return npc_reload;
    }

    if ((danger <= NPC_DANGER_VERY_LOW && (get_hunger() > 40 || thirst > 40)) ||
        thirst > 80 || get_hunger() > 160) {
        //return npc_eat; // TODO: Make eating work when then NPC doesn't have enough food
        set_hunger(0);
        thirst = 0;
    }

    // TODO: More risky attempts at sleep when exhausted
    if( danger == 0 && fatigue > TIRED ) {
        if( !is_following() ) {
            fatigue = 0; // TODO: Make tired NPCs handle sleep offscreen
            return npc_undecided;
        }

        if( rules.allow_sleep || fatigue > MASSIVE_FATIGUE ) {
            return npc_sleep;
        } else if( g->u.sees( *this ) && !has_effect( "npc_said" ) &&
                   one_in( 10000 / ( fatigue + 1 ) ) ) {
            say( "<yawn>" );
            add_effect( "npc_said", 10 );
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
            return method_of_attack(TARGET_PLAYER, player_danger( &(g->u) ));
        }
        return npc_undecided;
    }

    if (attitude == NPCATT_FLEE) {
        return npc_flee;
    }

    if (attitude == NPCATT_LEAD) {
        if( rl_dist( pos(), g->u.pos() ) >= 12 || !sees( g->u ) ) {
            if(has_effect("catch_up")) {
                int intense = get_effect_int("catch_up");
                if (intense < 10) {
                    say("<keep_up>");
                    add_effect("catch_up", 5);
                    return npc_pause;
                } else if (intense == 10) {
                    say("<im_leaving_you>");
                    add_effect("catch_up", 5);
                    return npc_pause;
                } else {
                    return npc_goto_destination;
                }
            }
        } else {
            return npc_goto_destination;
        }
    }
    return npc_undecided;
}

npc_action npc::long_term_goal_action()
{
    add_msg( m_debug, "long_term_goal_action()" );
    path.clear();

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
        if( ( !is_following() || rules.use_grenades ||
              !( item::find_type( elem )->item_tags.count( "GRENADE" ) ) ) &&
            has_amount( elem, 1 ) ) {
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
            const it_comest *food = NULL;
            if (it.is_food()) {
                food = dynamic_cast<const it_comest *>(it.type);
            }
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

// Index defaults to 0, i.e., wielded weapon
int npc::confident_range(int position)
{

    if (position == -1 && (!weapon.is_gun() || weapon.charges <= 0)) {
        return 1;
    }

    double deviation = 0;
    int max = 0;
    if (position == -1) {
        deviation = get_weapon_dispersion( &weapon, true );
        deviation += recoil + driving_recoil;
        // Convert from MoA back to quarter-degrees.
        deviation /= 15;
    } else { // We aren't firing a gun, we're throwing something!

        item *thrown = &i_at(position);
        max = throw_range(position); // The max distance we can throw
        deviation = 0;
        ///\EFFECT_THROW_NPC increases throwing confidence
        if (skillLevel( skill_throw ) < 8) {
            deviation += 8 - skillLevel( skill_throw );
        } else {
            deviation -= skillLevel( skill_throw ) - 6;
        }

        deviation += throw_dex_mod();

        ///\EFFECT_PER_NPC increases throwing confidence
        if (per_cur < 6) {
            deviation += 8 - per_cur;
        } else if (per_cur > 8) {
            deviation -= per_cur - 8;
        }

        deviation += encumb(bp_hand_r) + encumb(bp_hand_l) + encumb(bp_eyes) + 1;
        if (thrown->volume() > 5) {
            deviation += 1 + (thrown->volume() - 5) / 4;
        }
        if (thrown->volume() == 0) {
            deviation += 3;
        }

        ///\EFFECT_STR_NPC decreases throwing confidence
        deviation += 1 + abs(str_cur - (thrown->weight() / 113));
    }
    //Account for rng's, *.5 for 50%
    deviation /= 2;

    // Using 180 for now for extra-confident NPCs.
    int ret = (max > int(180 / deviation) ? max : int(180 / deviation));
    if (weapon.has_curammo() && ret > weapon.gun_range(this)) {
        return weapon.gun_range(this);
    }
    return ret;
}

// Index defaults to -1, i.e., wielded weapon
bool npc::wont_hit_friend( const tripoint &tar, int weapon_index )
{
    int confident = confident_range(weapon_index);
    if( rl_dist( pos3(), tar ) == 1 ) {
        return true;    // If we're *really* sure that our aim is dead-on
    }

    std::vector<tripoint> traj = g->m.find_clear_path( pos3(), tar );

    for( auto &i : traj ) {
        int dist = rl_dist( pos3(), i );
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

bool npc::is_blocking_position( const tripoint &p ) {
    // TODO: consider 3d? not very important for now
    // TODO: there might be a more elegant way to do this
    int dx = posx() - p.x;
    int dy = posy() - p.y;

    // Check whether this NPC is blocking movement from the given tile.
    // Example:
    // W..
    // NP.
    // W..
    //
    // Here the two W's are blocking movement from P to the tile left
    // of N

    tripoint left = pos();
    tripoint right = pos();

    if(dx == 0) {
        // Vertical movement
        left.x--;
        right.x++;
    } else if(dy == 0) {
        // Horizontal movement
        left.y--;
        right.y++;
    } else if(dx * dy == 1) {
        // Diagonal movement
        left.x += dx;
        right.y += dy;
    }

    return (g->m.impassable(left) && g->m.impassable(right));
}

bool npc::need_to_reload()
{
    if (!weapon.is_gun()) {
        return false;
    }
    return (weapon.charges < weapon.type->gun->clip * .1);
}

bool npc::enough_time_to_reload(int target, item &gun)
{
    int rltime = gun.reload_time(*this);
    double turns_til_reloaded = rltime / get_speed();
    int dist, speed;

    if (target == TARGET_PLAYER) {
        if( sees( g->u ) && g->u.weapon.is_gun() && rltime > 200 ) {
            return false;    // Don't take longer than 2 turns if player has a gun
        }

        dist = rl_dist(pos(), g->u.pos());
        speed = speed_estimate( g->u );
    } else if (target >= 0) {
        const monster &mon = g->zombie( target );
        dist = rl_dist(pos(), mon.pos());
        speed = speed_estimate( mon );
    } else {
        return true;    // No target, plenty of time to reload
    }

    double turns_til_reached = (dist * 100) / speed;

    return (turns_til_reloaded < turns_til_reached);
}

void npc::update_path( const tripoint &p, const bool no_bashing )
{
    if( p == pos() ) {
        path.clear();
        return;
    }

    while( !path.empty() && path[0] == pos() ) {
        path.erase( path.begin() );
    }

    const int bash_power = no_bashing ? 0 : smash_ability();
    if( path.empty() ) {
        path = g->m.route( pos(), p, bash_power, 1000 );
        return;
    }
    const tripoint &last = path[path.size() - 1];
    if( last == p && ( path[0].z != posz() || rl_dist( path[0], pos() ) <= 1 ) ) {
        // Our path already leads to that point, no need to recalculate
        return;
    }

    path = g->m.route( pos(), p, bash_power, 1000 );
    if( !path.empty() && path[0] == pos() ) {
        path.erase( path.begin() );
    }
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
        add_effect("bouldering", 1, num_bp, true);
    } else if (has_effect("bouldering")) {
        remove_effect("bouldering");
    }

    tripoint p = pt;
    if( sees_dangerous_field( pt ) ) {
        // Move to a neighbor field instead, if possible.
        // Maybe this code already exists somewhere?
        auto other_points = g->m.get_dir_circle( pos3(), pt );
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
        if (int(str_cur / 2) + skillLevel( skill_gun ) >= (int)recoil) {
            recoil = MIN_RECOIL;
        } else {
            recoil -= int(str_cur / 2) + skillLevel( skill_gun );
            recoil = int(recoil / 2);
        }
    }

    if (has_effect("stunned")) {
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
    if( pos() == path[0] ) { // Move was successful
        path.erase( path.begin() );
    }
}

// TODO: Rewrite this.  It doesn't work well and is ugly.
void npc::avoid_friendly_fire(int target)
{
    tripoint tar;
    Creature *critter = get_target( target );
    if( critter != nullptr ) {
        tar = critter->pos();
        if( critter != &g->u && !one_in( 3 ) ) {
            say(_("<move> so I can shoot that %s!"), g->zombie(target).name().c_str());
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
    execute_action(action, target);
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
    pause();
}

void npc::find_item()
{
    if( is_following() && !rules.allow_pick_up ) {
        // Grabbing stuff not allowed by our "owner"
        return;
    }

    fetching_item = false;
    int best_value = minimum_item_value();
    int range = sight_range( g->light_level( posz() ) );
    if( range > 12 ) {
        range = 12;
    }

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
    bool u_see_me = g->u.sees( *this );
    bool u_see_items = g->u.sees( wanted_item_pos );
    if( u_see_me ) {
        if( pickup.size() == 1 ) {
            if (u_see_items) {
                add_msg(_("%1$s picks up a %2$s."), name.c_str(),
                        items[pickup[0]].tname().c_str());
            } else {
                add_msg(_("%s picks something up."), name.c_str());
            }
        } else if( pickup.size() == 2 ) {
            if (u_see_items) {
                add_msg(_("%1$s picks up a %2$s and a %3$s."), name.c_str(),
                        items[pickup[0]].tname().c_str(),
                        items[pickup[1]].tname().c_str());
            } else {
                add_msg(_("%s picks up a couple of items."), name.c_str());
            }
        } else if( pickup.size() > 2 ) {
            add_msg(_("%s picks up several items."), name.c_str());
        } else {
            add_msg(_("%s looks around nervously, as if searching for something."), name.c_str());
        }
    } else if (u_see_items) {
        if( pickup.size() == 1 ) {
            add_msg(_("Someone picks up a %s."),
                    items[pickup[0]].tname().c_str());
        } else if( pickup.size() == 2 ) {
            add_msg(_("Someone picks up a %1$s and a %2$s"),
                    items[pickup[0]].tname().c_str(),
                    items[pickup[1]].tname().c_str());
        } else if( pickup.size() > 2 ) {
            add_msg(_("Someone picks up several items."));
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
        g->m.add_item_or_charges(posx(), posy(), dropped);
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

npc_action npc::scan_new_items(int target)
{
    bool can_use_gun = (!is_following() || rules.use_guns);
    bool use_silent = (is_following() && rules.use_silent);
    invslice slice = inv.slice();

    // Check if there's something better to wield
    bool has_empty_gun = false, has_better_melee = false;
    std::vector<item *> empty_guns;
    for( auto &i : slice ) {
        item &it = i->front();
        bool allowed = can_use_gun && it.is_gun() && (!use_silent || it.is_silent());
        if( allowed && it.charges > 0 ) {
            return npc_wield_loaded_gun;
        } else if( allowed && enough_time_to_reload( target, it ) ) {
            has_empty_gun = true;
            empty_guns.push_back(&it);
        } else if( !has_better_melee && melee_value( it ) > melee_value( weapon ) * 1.1 ) {
            has_better_melee = true;
        }
    }

    bool has_ammo_for_empty_gun = false;
    for (auto &i : empty_guns) {
        for (auto &j : slice) {
            item &it = j->front();
            if (it.is_ammo() &&
                it.ammo_type() == i->ammo_type()) {
                has_ammo_for_empty_gun = true;
            }
        }
    }

    if (has_empty_gun && has_ammo_for_empty_gun) {
        return npc_wield_empty_gun;
    } else if (has_better_melee) {
        return npc_wield_melee;
    } if( unarmed_value() > melee_value( weapon ) * 1.1 ) {
        return npc_wield_melee;
    }

    return npc_pause;
}

void npc::wield_best_melee()
{
    double best_value = 0.0;
    item *it = inv.best_for_melee( *this, best_value );
    if( unarmed_value() >= best_value ) {
        // "I cast fist!"
        it = &ret_null;
    }

    wield( it );
}

void npc::alt_attack(int target)
{
    itype_id which = "null";
    tripoint tar;
    Creature *critter = get_target( target );
    if( critter != nullptr ) {
        tar = critter->pos();
    } else { // This function shouldn't be called...
        debugmsg("npc::alt_attack() called with target = %d", target);
        move_pause();
        return;
    }

    int dist = rl_dist( pos(), tar );
    /* ALT_ATTACK_ITEMS is an array which stores the itype_id of all alternate
     * items, from least to most important.
     * See npc.h for definition of ALT_ATTACK_ITEMS
     */
    for( auto &elem : ALT_ATTACK_ITEMS ) {
        if( ( !is_following() || rules.use_grenades ||
              !( item::find_type( elem )->item_tags.count( "GRENADE" ) ) ) &&
            has_amount( elem, 1 ) ) {
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
    item *used = NULL;
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

    // Are we going to throw this item?
    if (!thrown_item(used)) {
        activate_item(weapon_index);
    } else { // We are throwing it!
        if (dist <= confident_range(weapon_index) && wont_hit_friend( tar, weapon_index )) {
            if (g->u.sees( *this )) {
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
        } else if (!wont_hit_friend( tar, weapon_index )) {// Danger of friendly fire

            if (!used->active || used->charges > 2) { // Safe to hold on to, for now
                avoid_friendly_fire(target);    // Maneuver around player
            } else { // We need to throw this live (grenade, etc) NOW! Pick another target?
                int conf = confident_range(weapon_index);
                for (int dist = 2; dist <= conf; dist++) {
                    for (int x = posx() - dist; x <= posx() + dist; x++) {
                        for (int y = posy() - dist; y <= posy() + dist; y++) {
                            int newtarget = g->mon_at( { x, y, posz() } );
                            int newdist = rl_dist(posx(), posy(), x, y);
                            // TODO: Change "newdist >= 2" to "newdist >= safe_distance(used)"
                            // Molotovs are safe at 2 tiles, grenades at 4, mininukes at 8ish
                            if (newdist <= conf && newdist >= 2 && newtarget != -1 &&
                                wont_hit_friend( tripoint( x, y, posz() ), weapon_index)) { // Friendlyfire-safe!
                                alt_attack(newtarget);
                                return;
                            }
                        }
                    }
                }
                /* If we have reached THIS point, there's no acceptable monster to throw our
                 * grenade or whatever at.  Since it's about to go off in our hands, better to
                 * just chuck it as far away as possible--while being friendly-safe.
                 */
                int best_dist = 0;
                for (int dist = 2; dist <= conf; dist++) {
                    for (int x = posx() - dist; x <= posx() + dist; x++) {
                        for (int y = posy() - dist; y <= posy() + dist; y++) {
                            int new_dist = rl_dist(posx(), posy(), x, y);
                            if (new_dist > best_dist && wont_hit_friend( tripoint( x, y, posz() ), weapon_index)) {
                                best_dist = new_dist;
                                tar.x = x;
                                tar.y = y;
                            }
                        }
                    }
                }
                /* Even if tar.x/tar.y didn't get set by the above loop, throw it anyway.  They
                 * should be equal to the original location of our target, and risking friendly
                 * fire is better than holding on to a live grenade / whatever.
                 */
                if (g->u.sees( *this )) {
                    add_msg(_("%1$s throws a %2$s."), name.c_str(),
                            used->tname().c_str());
                }

                int stack_size = -1;
                if( used->count_by_charges() ) {
                    stack_size = used->charges;
                    used->charges = 1;
                }
                throw_item( tar, *used);

                // Throw a single charge of a stacking object.
                if( stack_size == -1 || stack_size == 1 ) {
                    i_rem(weapon_index);
                } else {
                    used->charges = stack_size - 1;
                }

                i_rem(weapon_index);
            }

        } else { // Within this block, our chosen target is outside of our range
            update_path( tar );
            move_to_next(); // Move towards the target
        }
    } // Done with throwing-item block
}

void npc::activate_item(int item_index)
{
    const int oldmoves = moves;
    item *it = &i_at(item_index);
    if (it->is_tool()) {
        const auto tool = dynamic_cast<const it_tool *>(it->type);
        tool->invoke( this, it, pos3() );
    } else if (it->is_food()) {
        const auto comest = dynamic_cast<const it_comest *>(it->type);
        comest->invoke( this, it, pos3() );
    }

    if( moves == oldmoves ) {
        // A hack to prevent debugmsgs when NPCs activate 0 move items
        // while not removing the debugmsgs for other 0 move actions
        moves--;
    }
}

bool thrown_item(item *used)
{
    if (used == NULL) {
        debugmsg("npcmove.cpp's thrown_item() called with NULL item");
        return false;
    }
    itype_id type = itype_id(used->type->id);
    return (used->active || type == "knife_combat" || type == "spear_wood");
}

void npc::heal_player(player &patient)
{
    int dist = rl_dist( pos3(), patient.pos3() );

    if (dist > 1) { // We need to move to the player
        update_path( patient.pos3() );
        move_to_next();
    } else { // Close enough to heal!
        int lowest_HP = 400;
        hp_part worst = hp_head;
        // Chose the worst-hurting body part
        for (int i = 0; i < num_hp_parts; i++) {
            int hp = patient.hp_cur[i];
            // The head and torso are weighted more heavily than other body parts
            if (i == hp_head) {
                hp = patient.hp_max[i] - 3 * (patient.hp_max[i] - hp);
            } else if (i == hp_torso) {
                hp = patient.hp_max[i] - 2 * (patient.hp_max[i] - hp);
            }
            if (hp < lowest_HP) {
                lowest_HP = hp;
                worst = hp_part(i);
            }
        }

        bool u_see_me      = g->u.sees( *this ),
             u_see_patient = g->u.sees( patient );
        if (patient.is_npc()) {
            if (u_see_me) {
                if (u_see_patient) {
                    add_msg(_("%1$s heals %2$s."),
                            name.c_str(), patient.name.c_str());
                } else {
                    add_msg(_("%s heals someone."), name.c_str());
                }
            } else if (u_see_patient) {
                add_msg(_("Someone heals %s."), patient.name.c_str());
            }
        } else if (u_see_me) {
            add_msg(m_good, _("%s heals you."), name.c_str());
        } else {
            add_msg(m_good, _("Someone heals you."));
        }

        int amount_healed = 0;
        ///\EFFECT_FIRSTAID_NPC increases healing effects of first aid kit or bandages for player
        if (has_amount("1st_aid", 1)) {
            switch (worst) {
            case hp_head:
                amount_healed = 10 + 1.6 * skillLevel( skill_firstaid );
                break;
            case hp_torso:
                amount_healed = 20 + 3   * skillLevel( skill_firstaid );
                break;
            default:
                amount_healed = 15 + 2   * skillLevel( skill_firstaid );
            }
            use_charges("1st_aid", 1);
        } else if (has_amount("bandages", 1)) {
            switch (worst) {
            case hp_head:
                amount_healed =  1 + 1.6 * skillLevel( skill_firstaid );
                break;
            case hp_torso:
                amount_healed =  4 + 3   * skillLevel( skill_firstaid );
                break;
            default:
                amount_healed =  3 + 2   * skillLevel( skill_firstaid );
            }
            use_charges("bandages", 1);
        }
        patient.heal(worst, amount_healed);
        moves -= 250;

        if (!patient.is_npc()) {
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
}

void npc::heal_self()
{
    int lowest_HP = 400;
    hp_part worst = hp_head;
    // Chose the worst-hurting body part
    for (int i = 0; i < num_hp_parts; i++) {
        int hp = hp_cur[i];
        // The head and torso are weighted more heavily than other body parts
        if (i == hp_head) {
            hp = hp_max[i] - 3 * (hp_max[i] - hp);
        } else if (i == hp_torso) {
            hp = hp_max[i] - 2 * (hp_max[i] - hp);
        }
        if (hp < lowest_HP) {
            lowest_HP = hp;
            worst = hp_part(i);
        }
    }

    ///\EFFECT_FIRSTAID_NPC increases healing effects of first aid kit or bandages for self
    int amount_healed = 0;
    if (has_amount("1st_aid", 1)) {
        switch (worst) {
        case hp_head:
            amount_healed = 10 + 1.6 * skillLevel( skill_firstaid );
            break;
        case hp_torso:
            amount_healed = 20 + 3   * skillLevel( skill_firstaid );
            break;
        default:
            amount_healed = 15 + 2   * skillLevel( skill_firstaid );
        }
        use_charges("1st_aid", 1);
    } else if (has_amount("bandages", 1)) {
        switch (worst) {
        case hp_head:
            amount_healed =  1 + 1.6 * skillLevel( skill_firstaid );
            break;
        case hp_torso:
            amount_healed =  4 + 3   * skillLevel( skill_firstaid );
            break;
        default:
            amount_healed =  3 + 2   * skillLevel( skill_firstaid );
        }
        use_charges("bandages", 1);
    } else {
        debugmsg("NPC tried to heal self, but has no bandages / first aid");
        move_pause();
    }
    if (g->u.sees( *this )) {
        add_msg(_("%s heals %s."), name.c_str(),
                (male ? _("himself") : _("herself")));
    }
    heal(worst, amount_healed);
    moves -= 250;
}

void npc::use_painkiller()
{
    // First, find the best painkiller for our pain level
    item *it = inv.most_appropriate_painkiller(pain);

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

void npc::pick_and_eat()
{
    int best_hunger = 999, best_thirst = 999, index = -1;
    bool thirst_more_important = (thirst > get_hunger() * 1.5);
    invslice slice = inv.slice();
    for (size_t i = 0; i < slice.size(); i++) {
        int eaten_hunger = -1, eaten_thirst = -1;
        const it_comest *food = NULL;
        item &it = slice[i]->front();
        if (it.is_food()) {
            food = dynamic_cast<const it_comest *>(it.type);
        } else if (it.is_food_container()) {
            food = dynamic_cast<const it_comest *>(it.contents[0].type);
        }
        if (food != NULL) {
            eaten_hunger = get_hunger() - food->nutr;
            eaten_thirst = thirst - food->quench;
        }
        if (eaten_hunger > 0) { // <0 means we have a chance of puking
            if ((thirst_more_important && eaten_thirst < best_thirst) ||
                (!thirst_more_important && eaten_hunger < best_hunger) ||
                (eaten_thirst == best_thirst && eaten_hunger < best_hunger) ||
                (eaten_hunger == best_hunger && eaten_thirst < best_thirst)   ) {
                if (eaten_hunger < best_hunger) {
                    best_hunger = eaten_hunger;
                }
                if (eaten_thirst < best_thirst) {
                    best_thirst = eaten_thirst;
                }
                index = i;
            }
        }
    }

    if (index == -1) {
        move_pause();
        return;
    }

    consume(index);
    moves = 0;
}

void npc::mug_player(player &mark)
{
    if( rl_dist( pos(), mark.pos() ) > 1 ) { // We have to travel
        update_path( mark.pos3() );
        move_to_next();
    } else {
        bool u_see_me   = g->u.sees( *this ),
             u_see_mark = g->u.sees( mark );
        if (mark.cash > 0) {
            cash += mark.cash;
            mark.cash = 0;
            moves = 0;
            // Describe the action
            if (mark.is_npc()) {
                if (u_see_me) {
                    if (u_see_mark) {
                        add_msg(_("%1$s takes %2$s's money!"),
                                name.c_str(), mark.name.c_str());
                    } else {
                        add_msg(_("%s takes someone's money!"),
                                name.c_str());
                    }
                } else if (u_see_mark) {
                    add_msg(_("Someone takes %s's money!"),
                            mark.name.c_str());
                }
            } else {
                if (u_see_me) {
                    add_msg(m_bad, _("%s takes your money!"), name.c_str());
                } else {
                    add_msg(m_bad, _("Someone takes your money!"));
                }
            }
        } else { // We already have their money; take some goodies!
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
            } else {
                bool u_see_me   = g->u.sees( *this ),
                     u_see_mark = g->u.sees( mark );
                item stolen = mark.i_rem(item_index);
                if (mark.is_npc()) {
                    if (u_see_me) {
                        if (u_see_mark)
                            add_msg(_("%1$s takes %2$s's %3$s."), name.c_str(),
                                    mark.name.c_str(),
                                    stolen.tname().c_str());
                        else {
                            add_msg(_("%s takes something from somebody."),
                                    name.c_str());
                        }
                    } else if (u_see_mark)
                        add_msg(_("Someone takes %1$s's %2$s."),
                                mark.name.c_str(), stolen.tname().c_str());
                } else {
                    if (u_see_me) {
                        add_msg(m_bad, _("%1$s takes your %2$s."),
                                name.c_str(), stolen.tname().c_str());
                    } else {
                        add_msg(m_bad, _("Someone takes your %s."),
                                stolen.tname().c_str());
                    }
                }
                i_add(stolen);
                moves -= 100;
                if (!mark.is_npc()) {
                    op_of_u.value -= rng(0, 1);    // Decrease the value of the player
                }
            }
        }
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
    //this entire clause is to preserve the guard's home coordinates and permit him/her to return
    if (mission == NPC_MISSION_GUARD || mission == NPC_MISSION_SHOPKEEP) {
        if( guard_pos == global_square_location() ) {
            return; //Our guard is already at his/her home tile
        } else {
            if (path.size() > 1) {
                move_to_next();    //No point recalculating the path to get home
            } else {
                const tripoint dest( guard_pos.x - mapx * SEEX,
                                     guard_pos.y - mapy * SEEY,
                                     guard_pos.z );
                update_path( dest );
                move_to_next();
            }
        }
        return;//don't delete our post if we are a guard
    }

    goal = no_goal_point;
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
    if (mission == NPC_MISSION_GUARD || mission == NPC_MISSION_SHOPKEEP) {
        goal.x = global_omt_location().x;
        goal.y = global_omt_location().y;
        goal.z = g->get_levz();
        guard_pos = global_square_location();
        return;
    }

    // all of the following luxuries are at ground level.
    // so please wallow in hunger & fear if below ground.
    if(g->get_levz() != 0) {
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

    goal = overmap_buffer.find_closest(global_omt_location(), dest_type, 0, false);
}

void npc::go_to_destination()
{
    const tripoint omt_pos = global_omt_location();
    const int sx = (goal.x == omt_pos.x) ? 0 : sgn( goal.x - omt_pos.x );
    const int sy = (goal.y == omt_pos.y) ? 0 : sgn( goal.y - omt_pos.y );
    add_msg( m_debug, "%s going (%d,%d,%d)->(%d,%d,%d)", name.c_str(),
             omt_pos.x, omt_pos.y, omt_pos.z, goal.x, goal.y, goal.z );
    if( goal.x == omt_pos.x && goal.y == omt_pos.y ) { // We're at our desired map square!
        move_pause();
        reach_destination();
        return;
    } 

    // sx and sy are now equal to the direction we need to move in
    tripoint dest( posx() + 8 * sx, posy() + 8 * sy, posz() );
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

        dest = tripoint( posx() + rng( 0, 16 ) * sx, posy() + rng( 0, 16 ) * sy, goal.z );
    }
    move_pause();
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
    case npc_eat:
        return _("Eat");
    case npc_drop_items:
        return _("Drop items");
    case npc_flee:
        return _("Flee");
    case npc_melee:
        return _("Melee");
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

Creature *npc::get_target( int target ) const
{
    if( target == TARGET_PLAYER && !is_following() ) {
        return &g->u;
    } else if( target >= 0 && g->num_zombies() > (size_t)target ) {
        return &g->zombie( target );
    }

    // Should actually return a NPC, but those aren't well supported yet
    return nullptr;
}
