#include "mission.h"

#include <algorithm>
#include <cstdlib>
#include <istream>
#include <iterator>
#include <list>
#include <memory>
#include <new>
#include <numeric>
#include <set>
#include <unordered_map>
#include <utility>

#include "avatar.h"
#include "colony.h"
#include "creature.h"
#include "debug.h"
#include "dialogue_chatbin.h"
#include "enum_conversions.h"
#include "game.h"
#include "inventory.h"
#include "item.h"
#include "item_group.h"
#include "item_stack.h"
#include "kill_tracker.h"
#include "map.h"
#include "map_iterator.h"
#include "monster.h"
#include "npc.h"
#include "npc_class.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "requirements.h"
#include "string_formatter.h"
#include "translations.h"
#include "vehicle.h"
#include "vpart_position.h"

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

static const efftype_id effect_pet( "pet" );
static const efftype_id effect_run( "run" );
static const itype_id itype_null( "null" );

static const mission_type_id mission_NULL( "NULL" );

mission mission_type::create( const character_id &npc_id ) const
{
    mission ret;
    ret.uid = g->assign_mission_id();
    ret.type = this;
    ret.npc_id = npc_id;
    ret.item_id = item_id;
    ret.item_count = item_count;
    ret.value = value;
    ret.follow_up = follow_up;
    ret.monster_species = monster_species;
    ret.monster_type = monster_type;
    ret.monster_kill_goal = monster_kill_goal;

    if( deadline_low != 0_turns || deadline_high != 0_turns ) {
        ret.deadline = calendar::turn + rng( deadline_low, deadline_high );
    } else {
        ret.deadline = calendar::turn_zero;
    }

    return ret;
}

std::string mission_type::tname() const
{
    return name.translated();
}

static std::unordered_map<int, mission> world_missions;

mission *mission::reserve_new( const mission_type_id &type, const character_id &npc_id )
{
    const mission tmp = mission_type::get( type )->create( npc_id );
    // TODO: Warn about overwrite?
    mission &miss = world_missions[tmp.uid] = tmp;
    return &miss;
}

mission *mission::find( int id, bool ok_missing )
{
    const auto iter = world_missions.find( id );
    if( iter != world_missions.end() ) {
        return &iter->second;
    }
    if( !ok_missing ) {
        dbg( D_ERROR ) << "requested mission with uid " << id << " does not exist";
        debugmsg( "requested mission with uid %d does not exist", id );
    }
    return nullptr;
}

std::vector<mission *> mission::get_all_active()
{
    std::vector<mission *> ret;
    ret.reserve( world_missions.size() );
    for( auto &pr : world_missions ) {
        ret.push_back( &pr.second );
    }

    return ret;
}

void mission::remove_active_world_mission( mission &cur_mission )
{
    for( auto iter = world_missions.begin(); iter != world_missions.end(); iter++ ) {
        if( iter->second.get_id() == cur_mission.get_id() && iter->second.in_progress() ) {
            world_missions.erase( iter );
            break;
        }
    }
}

void mission::add_existing( const mission &m )
{
    world_missions[ m.uid ] = m;
}

void mission::process_all()
{
    for( auto &e : world_missions ) {
        e.second.process();
    }
}

std::vector<mission *> mission::to_ptr_vector( const std::vector<int> &vec, bool ok_missing )
{
    std::vector<mission *> result;
    for( const int &id : vec ) {
        mission *miss = find( id, ok_missing );
        if( miss != nullptr ) {
            result.push_back( miss );
        }
    }
    return result;
}

std::vector<int> mission::to_uid_vector( const std::vector<mission *> &vec )
{
    std::vector<int> result;
    result.reserve( vec.size() );
    for( const mission *miss : vec ) {
        result.push_back( miss->uid );
    }
    return result;
}

void mission::clear_all()
{
    world_missions.clear();
}

void mission::on_creature_death( Creature &poor_dead_dude )
{
    if( poor_dead_dude.is_hallucination() ) {
        return;
    }
    monster *mon = dynamic_cast<monster *>( &poor_dead_dude );
    if( mon != nullptr ) {

        if( mon->is_nemesis() ) {
            //the nemesis monster doesn't have a mission attached bc it's an overmap horde
            //so we loop to find the appropriate mission and complete it
            avatar &player_character = get_avatar();
            for( std::pair<const int, mission> &e : world_missions ) {
                mission &i = e.second;

                if( i.type->goal == MGOAL_KILL_NEMESIS && player_character.getID() == i.player_id ) {
                    i.step_complete( 1 );
                    return;
                }
            }
        }

        if( mon->mission_ids.empty() ) {
            return;
        }
        for( const int mission_id : mon->mission_ids ) {
            mission *found_mission = mission::find( mission_id );
            if( !found_mission ) {
                debugmsg( "invalid mission id %d", mission_id );
                continue;
            }
            const mission_type *type = found_mission->type;
            if( type->goal == MGOAL_FIND_MONSTER ) {
                found_mission->fail();
            }
            if( type->goal == MGOAL_KILL_MONSTER ) {
                found_mission->step_complete( 1 );
            }
            if( type->goal == MGOAL_KILL_MONSTERS ) {
                if( found_mission->monster_kill_goal-- == 0 ) {
                    found_mission->step_complete( 1 );
                }
            }
        }
        return;
    }
    npc *p = dynamic_cast<npc *>( &poor_dead_dude );
    if( p == nullptr ) {
        // Must be the player
        for( mission *&miss : get_avatar().get_active_missions() ) {
            // mission is free and can be reused
            miss->player_id = character_id();
        }
        // The missions remains assigned to the (dead) character. This should not cause any problems
        // as the character is dismissed anyway.
        // Technically, the active missions could be moved to the failed mission section.
        return;
    }
    const character_id dead_guys_id = p->getID();
    for( auto &e : world_missions ) {
        mission &i = e.second;
        if( !i.in_progress() ) {
            continue;
        }
        //complete the mission if you needed killing
        if( i.type->goal == MGOAL_ASSASSINATE && i.target_npc_id == dead_guys_id ) {
            i.step_complete( 1 );
        }
        //fail the mission if the mission giver dies
        if( i.npc_id == dead_guys_id ) {
            i.fail();
        }
        //fail the mission if recruit target dies
        if( i.type->goal == MGOAL_RECRUIT_NPC && i.target_npc_id == dead_guys_id ) {
            i.fail();
        }
    }
}

bool mission::on_creature_fusion( Creature &fuser, Creature &fused )
{
    if( fuser.is_hallucination() || fused.is_hallucination() ) {
        return false;
    }
    monster *mon_fuser = dynamic_cast<monster *>( &fuser );
    if( mon_fuser == nullptr ) {
        debugmsg( "Unimplemented: fuser is not a monster" );
        return false;
    }
    monster *mon_fused = dynamic_cast<monster *>( &fused );
    if( mon_fused == nullptr ) {
        debugmsg( "Unimplemented: fused is not a monster" );
        return false;
    }
    bool mission_transfered = false;
    for( const int mission_id : mon_fused->mission_ids ) {
        const mission *const found_mission = mission::find( mission_id );
        if( !found_mission ) {
            debugmsg( "invalid mission id %d", mission_id );
            continue;
        }
        const mission_type *const type = found_mission->type;
        if( type->goal == MGOAL_KILL_MONSTER || type->goal == MGOAL_KILL_MONSTERS ) {
            // the fuser has to be killed now!
            mon_fuser->mission_ids.emplace( mission_id );
            mon_fused->mission_ids.erase( mission_id );
            mission_transfered = true;
        }
    }
    return mission_transfered;
}

void mission::on_talk_with_npc( const character_id &npc_id )
{
    switch( type->goal ) {
        case MGOAL_TALK_TO_NPC:
            // If our goal is to talk to this npc, and we haven't yet completed a step for this
            // mission, then complete a step.
            if( npc_id == target_npc_id && step == 0 ) {
                step_complete( 1 );
            }
            break;
        default:
            break;
    }
}

mission *mission::reserve_random( const mission_origin origin, const tripoint_abs_omt &p,
                                  const character_id &npc_id )
{
    const mission_type_id type = mission_type::get_random_id( origin, p );
    if( type.is_null() ) {
        return nullptr;
    }
    return mission::reserve_new( type, npc_id );
}

void mission::assign( avatar &u )
{
    if( player_id == u.getID() ) {
        debugmsg( "strange: player is already assigned to mission %d", uid );
        return;
    }
    if( player_id.is_valid() ) {
        debugmsg( "tried to assign mission %d to player, but mission is already assigned to %d",
                  uid, player_id.get_value() );
        return;
    }
    player_id = u.getID();
    u.on_mission_assignment( *this );
    if( status == mission_status::yet_to_start ) {
        const kill_tracker &kills = g->get_kill_tracker();
        if( type->goal == MGOAL_KILL_MONSTER_TYPE && monster_type != mtype_id::NULL_ID() ) {
            kill_count_to_reach = kills.kill_count( monster_type ) + monster_kill_goal;
        } else if( type->goal == MGOAL_KILL_MONSTER_SPEC ) {
            kill_count_to_reach = kills.kill_count( monster_species ) + monster_kill_goal;
        }
        if( type->deadline_low != 0_turns || type->deadline_high != 0_turns ) {
            deadline = calendar::turn + rng( type->deadline_low, type->deadline_high );
        } else {
            deadline = calendar::turn_zero;
        }
        type->start( this );
        status = mission_status::in_progress;
    }
}

void mission::fail()
{
    status = mission_status::failure;
    avatar &player_character = get_avatar();
    if( player_character.getID() == player_id ) {
        player_character.on_mission_finished( *this );
    }

    type->fail( this );
}

void mission::set_target_to_mission_giver()
{
    const npc *giver = g->find_npc( npc_id );
    if( giver != nullptr ) {
        target = giver->global_omt_location();
    } else {
        target = overmap::invalid_tripoint;
    }
}

void mission::step_complete( const int _step )
{
    step = _step;
    switch( type->goal ) {
        case MGOAL_FIND_ITEM:
        case MGOAL_FIND_ITEM_GROUP:
        case MGOAL_FIND_MONSTER:
        case MGOAL_ASSASSINATE:
        case MGOAL_KILL_MONSTER:
        case MGOAL_KILL_MONSTERS:
        case MGOAL_COMPUTER_TOGGLE:
        case MGOAL_TALK_TO_NPC:
            // Go back and report.
            set_target_to_mission_giver();
            break;
        default:
            //Suppress warnings
            break;
    }
}

void mission::wrap_up()
{
    avatar &player_character = get_avatar();
    if( player_character.getID() != player_id ) {
        // This is called from npctalk.cpp, the npc should only offer the option to wrap up mission
        // that have been assigned to the current player.
        debugmsg( "mission::wrap_up called, player %d was assigned, but current player is %d",
                  player_id.get_value(), player_character.getID().get_value() );
    }

    status = mission_status::success;
    player_character.on_mission_finished( *this );
    std::vector<item_comp> comps;
    switch( type->goal ) {
        case MGOAL_FIND_ITEM_GROUP: {
            inventory tmp_inv = player_character.crafting_inventory();
            std::vector<item *> items = std::vector<item *>();
            tmp_inv.dump( items );
            item_group_id grp_type = type->group_id;
            itype_id container = type->container_id;
            bool specific_container_required = !container.is_null();
            bool remove_container = type->remove_container;
            itype_id empty_container = type->empty_container;

            std::map<itype_id, int> matches = std::map<itype_id, int>();
            get_all_item_group_matches(
                items, grp_type, matches,
                container, itype_null, specific_container_required );

            for( std::pair<const itype_id, int> &cnt : matches ) {
                comps.emplace_back( cnt.first, cnt.second );

            }

            player_character.consume_items( comps );

            if( remove_container ) {
                std::vector<item_comp> container_comp = std::vector<item_comp>();
                if( !empty_container.is_null() ) {
                    container_comp.emplace_back( empty_container, type->item_count );
                    player_character.consume_items( container_comp );
                } else {
                    container_comp.emplace_back( container, type->item_count );
                    player_character.consume_items( container_comp );
                }
            }
        }
        break;

        case MGOAL_FIND_ITEM: {
            const item item_sought( type->item_id );
            if( item_sought.is_software() ) {
                int consumed = 0;
                while( consumed < item_count ) {
                    if( player_character.consume_software_container( type->item_id ) ) {
                        consumed++;
                    } else {
                        debugmsg( "Tried to consume more software %s than available", type->item_id.c_str() );
                        break;
                    }
                }
            } else {
                comps.emplace_back( type->item_id, item_count );
                player_character.consume_items( comps );
            }
        }
        break;
        case MGOAL_FIND_ANY_ITEM:
            player_character.remove_mission_items( uid );
            break;
        case MGOAL_FIND_MONSTER: {
            Creature *found_creature = g->get_creature_if( [&]( const Creature & critter ) {
                const monster *const mon_ptr = dynamic_cast<const monster *>( &critter );
                return mon_ptr && mon_ptr->mission_ids.count( uid );
            } );
            if( found_creature != nullptr ) {
                monster *found_monster = found_creature->as_monster();
                if( found_monster != nullptr ) {
                    found_monster->mission_ids.erase( uid );
                    found_monster->remove_effect( effect_pet );
                    found_monster->remove_effect( effect_run );
                    found_monster->friendly = 0;
                    found_monster->morale = 100;
                }
            }
        }
        default:
            //Suppress warnings
            break;
    }

    type->end( this );
}

bool mission::is_complete( const character_id &_npc_id ) const
{
    if( status == mission_status::success ) {
        return true;
    }

    avatar &player_character = get_avatar();
    switch( type->goal ) {
        case MGOAL_GO_TO: {
            const tripoint_abs_omt cur_pos = player_character.global_omt_location();
            return rl_dist( cur_pos, target ) <= 1;
        }

        case MGOAL_GO_TO_TYPE: {
            const oter_id cur_ter = overmap_buffer.ter( player_character.global_omt_location() );
            return ( cur_ter->get_type_id() == oter_type_str_id( type->target_id.str() ) );
        }

        case MGOAL_FIND_ITEM_GROUP: {
            inventory tmp_inv = player_character.crafting_inventory();
            std::vector<item *> items = std::vector<item *>();
            tmp_inv.dump( items );
            item_group_id grp_type = type->group_id;
            itype_id container = type->container_id;
            bool specific_container_required = !container.is_null();

            std::map<itype_id, int> matches = std::map<itype_id, int>();
            get_all_item_group_matches(
                items, grp_type, matches,
                container, itype_null, specific_container_required );

            int total_match = std::accumulate( matches.begin(), matches.end(), 0,
            []( const std::size_t previous, const std::pair<const itype_id, std::size_t> &p ) {
                return static_cast<int>( previous + p.second );
            } );

            if( total_match >= ( type->item_count ) ) {
                return true;

            }
        }
        return false;

        case MGOAL_FIND_ITEM: {
            if( npc_id.is_valid() && npc_id != _npc_id ) {
                return false;
            }
            item item_sought( type->item_id );
            map &here = get_map();
            int found_quantity = 0;
            bool charges = item_sought.count_by_charges();
            bool software = item_sought.is_software();
            auto count_items = [this, &found_quantity, &player_character, charges, software]( item_stack &&
            items ) {
                for( const item &i : items ) {
                    if( !i.is_owned_by( player_character, true ) ) {
                        continue;
                    }
                    if( software ) {
                        for( const item *soft : i.softwares() ) {
                            if( soft->typeId() == type->item_id ) {
                                found_quantity ++;
                            }
                        }
                    }
                    if( charges ) {
                        found_quantity += i.charges_of( type->item_id, item_count - found_quantity );
                    } else {
                        found_quantity += i.amount_of( type->item_id, false, item_count - found_quantity );
                    }
                }
            };
            for( const tripoint &p : here.points_in_radius( player_character.pos(), 5 ) ) {
                if( player_character.sees( p ) ) {
                    if( here.has_items( p ) && here.accessible_items( p ) ) {
                        count_items( here.i_at( p ) );
                    }
                    if( const std::optional<vpart_reference> ovp = here.veh_at( p ).cargo() ) {
                        count_items( ovp->items() );
                    }
                    if( found_quantity >= item_count ) {
                        break;
                    }
                }
            }
            if( software ) {
                found_quantity += player_character.count_softwares( type->item_id );
            }
            if( charges ) {
                return player_character.charges_of( type->item_id ) + found_quantity >= item_count;
            } else {
                return player_character.amount_of( type->item_id ) + found_quantity >= item_count;
            }
        }
        return true;

        case MGOAL_FIND_ANY_ITEM:
            return player_character.has_mission_item( uid ) && ( !npc_id.is_valid() || npc_id == _npc_id );

        case MGOAL_FIND_MONSTER:
            if( npc_id.is_valid() && npc_id != _npc_id ) {
                return false;
            }
            return g->get_creature_if( [&]( const Creature & critter ) {
                const monster *const mon_ptr = dynamic_cast<const monster *>( &critter );
                return mon_ptr && mon_ptr->mission_ids.count( uid );
            } );

        case MGOAL_RECRUIT_NPC: {
            npc *p = g->find_npc( target_npc_id );
            return p != nullptr && p->get_attitude() == NPCATT_FOLLOW;
        }

        case MGOAL_RECRUIT_NPC_CLASS: {
            const auto npcs = overmap_buffer.get_npcs_near_player( 100 );
            for( const auto &npc : npcs ) {
                if( npc->myclass == recruit_class && npc->get_attitude() == NPCATT_FOLLOW ) {
                    return true;
                }
            }
            return false;
        }

        case MGOAL_FIND_NPC:
            return npc_id == _npc_id;

        case MGOAL_TALK_TO_NPC:
        case MGOAL_ASSASSINATE:
        case MGOAL_KILL_MONSTER:
        case MGOAL_KILL_MONSTERS:
        case MGOAL_KILL_NEMESIS:
        case MGOAL_COMPUTER_TOGGLE:
            return step >= 1;

        case MGOAL_KILL_MONSTER_TYPE:
            return g->get_kill_tracker().kill_count( monster_type ) >= kill_count_to_reach;

        case MGOAL_KILL_MONSTER_SPEC:
            return g->get_kill_tracker().kill_count( monster_species ) >= kill_count_to_reach;

        case MGOAL_CONDITION: {
            std::unique_ptr<talker> beta;
            std::vector<mission *> miss;
            // Skip the NPC check if the mission was obtained via a scenario/profession/hobby
            if( npc_id.is_valid() || type->origins.empty() || type->origins.size() != 1 ||
                type->origins.front() != mission_origin::ORIGIN_GAME_START ) {
                // Only allow completing when talking to the mission originator.
                if( npc_id != _npc_id ) {
                    return false;
                }
                npc *n = g->find_npc( _npc_id );
                if( n == nullptr ) {
                    return false;
                }
                beta = get_talker_for( *n );
                for( mission *&mission : n->chatbin.missions_assigned ) {
                    if( mission->get_assigned_player_id() == player_character.getID() ) {
                        miss.emplace_back( mission );
                    }
                }
            } else {
                for( mission *&mission : player_character.get_active_missions() ) {
                    if( mission->type->id == type->id ) {
                        miss.emplace_back( mission );
                    }
                }
            }
            dialogue cc( get_talker_for( player_character ), std::move( beta ) );
            cc.missions_assigned = std::move( miss );

            return type->test_goal_condition( cc );
        }

        default:
            return false;
    }
}

void mission::get_all_item_group_matches( std::vector<item *> &items,
        item_group_id &grp_type, std::map<itype_id, int> &matches,
        const itype_id &required_container, const itype_id &actual_container,
        bool &specific_container_required )
{
    for( item *itm : items ) {
        bool correct_container = ( required_container == actual_container ) ||
                                 !specific_container_required;

        bool item_in_group = item_group::group_contains_item( grp_type, itm->typeId() );

        //check whether item itself is target
        if( item_in_group && correct_container ) {
            std::map<itype_id, int>::iterator it = matches.find( itm->typeId() );
            if( it != matches.end() ) {
                it->second = ( it->second ) + 1;
            } else {
                matches.insert( std::make_pair( itm->typeId(), 1 ) );
            }
        }

        //recursively check item contents for target
        if( itm->is_container() && !itm->is_container_empty() ) {
            std::list<item *> content_list = itm->all_items_top();
            std::vector<item *> content = std::vector<item *>();

            //list of item into list item*
            std::transform(
                content_list.begin(), content_list.end(),
                std::back_inserter( content ),
            []( item * p ) {
                return p;
            } );

            get_all_item_group_matches(
                content, grp_type, matches,
                required_container, itm->typeId(), specific_container_required );
        }
    }
}

bool mission::has_deadline() const
{
    return deadline != calendar::turn_zero;
}

time_point mission::get_deadline() const
{
    return deadline;
}

std::string mission::get_description() const
{
    return type->description.translated();
}

bool mission::has_target() const
{
    return target != overmap::invalid_tripoint;
}

const tripoint_abs_omt &mission::get_target() const
{
    return target;
}

const mission_type &mission::get_type() const
{
    if( type == nullptr ) {
        debugmsg( "Null mission type" );
        return mission_type::get_all().front();
    }

    return *type;
}

bool mission::has_follow_up() const
{
    return !follow_up.is_null();
}

mission_type_id mission::get_follow_up() const
{
    return follow_up;
}

int mission::get_value() const
{
    return value;
}

int mission::get_id() const
{
    return uid;
}

const itype_id &mission::get_item_id() const
{
    return item_id;
}

bool mission::has_failed() const
{
    return status == mission_status::failure;
}

bool mission::in_progress() const
{
    return status == mission_status::in_progress;
}

void mission::process()
{
    if( !in_progress() ) {
        return;
    }

    if( has_deadline() && calendar::turn > deadline ) {
        fail();
    } else if( !npc_id.is_valid() && is_complete( npc_id ) ) { // No quest giver.
        wrap_up();
    }
}

character_id mission::get_npc_id() const
{
    return npc_id;
}

const std::vector<std::pair<int, itype_id>> &mission::get_likely_rewards() const
{
    return type->likely_rewards;
}

bool mission::has_generic_rewards() const
{
    return type->has_generic_rewards;
}

void mission::set_target( const tripoint_abs_omt &p )
{
    target = p;
}

void mission::set_target_npc_id( const character_id &npc_id )
{
    target_npc_id = npc_id;
}

void mission::set_assigned_player_id( const character_id &char_id )
{
    player_id = char_id;
}

void mission::update_world_missions_character( const character_id &old_char_id,
        const character_id &new_char_id )
{
    for( auto &world_mission : world_missions ) {
        if( world_mission.second.in_progress() &&
            ( world_mission.second.get_assigned_player_id()  == old_char_id ||
              world_mission.second.get_assigned_player_id() == character_id( - 1 ) ) ) {
            world_mission.second.set_assigned_player_id( new_char_id );
        }
    }
}

bool mission::is_assigned() const
{
    return player_id.is_valid();
}

character_id mission::get_assigned_player_id() const
{
    return player_id;
}

std::string mission::name() const
{
    if( type == nullptr ) {
        return "NULL";
    }
    return type->tname();
}

mission_type_id mission::mission_id() const
{
    if( type == nullptr ) {
        return mission_NULL;
    }
    return type->id;
}

std::string mission::dialogue_for_topic( const std::string &in_topic ) const
{
    // The internal keys are pretty ugly, it's better to translate them here than globally
    static const std::map<std::string, std::string> topic_translation = {{
            { "TALK_MISSION_DESCRIBE", "describe" },
            { "TALK_MISSION_DESCRIBE_URGENT", "describe" },
            { "TALK_MISSION_OFFER", "offer" },
            { "TALK_MISSION_ACCEPTED", "accepted" },
            { "TALK_MISSION_REJECTED", "rejected" },
            { "TALK_MISSION_ADVICE", "advice" },
            { "TALK_MISSION_INQUIRE", "inquire" },
            { "TALK_MISSION_SUCCESS", "success" },
            { "TALK_MISSION_SUCCESS_LIE", "success_lie" },
            { "TALK_MISSION_FAILURE", "failure" }
        }
    };

    const auto &replacement = topic_translation.find( in_topic );
    const std::string &topic = replacement != topic_translation.end() ? replacement->second : in_topic;

    const auto &response = type->dialogue.find( topic );
    if( response != type->dialogue.end() ) {
        return response->second.translated();
    }

    debugmsg( "Someone forgot to code this message id is %s, topic is %s!",
              type->id.c_str(), topic.c_str() );
    return "";
}

mission::mission()
    : deadline( 0 )
{
    type = nullptr;
    status = mission_status::yet_to_start;
    value = 0;
    uid = -1;
    target = tripoint_abs_omt( tripoint_min );
    item_id = itype_id::NULL_ID();
    item_count = 1;
    target_id = string_id<oter_type_t>::NULL_ID();
    recruit_class = NC_NONE;
    target_npc_id = character_id();
    monster_type = mtype_id::NULL_ID();
    monster_kill_goal = -1;
    npc_id = character_id();
    good_fac_id = -1;
    bad_fac_id = -1;
    step = 0;
    player_id = character_id();
}

namespace io
{
template<>
std::string enum_to_string<mission::mission_status>( mission::mission_status data )
{
    switch( data ) {
        // *INDENT-OFF*
        case mission::mission_status::yet_to_start: return "yet_to_start";
        case mission::mission_status::in_progress: return "in_progress";
        case mission::mission_status::success: return "success";
        case mission::mission_status::failure: return "failure";
        // *INDENT-ON*
        case mission::mission_status::num_mission_status:
            break;

    }
    cata_fatal( "Invalid mission_status" );
}

} // namespace io

mission::mission_status mission::status_from_string( const std::string &s )
{
    return io::string_to_enum<mission::mission_status>( s );
}

std::string mission::status_to_string( mission::mission_status st )
{
    return io::enum_to_string<mission::mission_status>( st );
}
