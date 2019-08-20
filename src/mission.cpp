#include "mission.h"

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <numeric>
#include <istream>
#include <iterator>
#include <list>
#include <utility>

#include "avatar.h"
#include "debug.h"
#include "game.h"
#include "line.h"
#include "npc.h"
#include "npc_class.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "requirements.h"
#include "skill.h"
#include "string_formatter.h"
#include "translations.h"
#include "item_group.h"
#include "creature.h"
#include "inventory.h"
#include "item.h"
#include "json.h"
#include "monster.h"
#include "material.h"

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

mission mission_type::create( const character_id npc_id ) const
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
        ret.deadline = 0;
    }

    return ret;
}

std::string mission_type::tname() const
{
    return _( name );
}

static std::unordered_map<int, mission> world_missions;

mission *mission::reserve_new( const mission_type_id &type, const character_id npc_id )
{
    const auto tmp = mission_type::get( type )->create( npc_id );
    // TODO: Warn about overwrite?
    mission &miss = world_missions[tmp.uid] = tmp;
    return &miss;
}

mission *mission::find( int id )
{
    const auto iter = world_missions.find( id );
    if( iter != world_missions.end() ) {
        return &iter->second;
    }
    dbg( D_ERROR ) << "requested mission with uid " << id << " does not exist";
    debugmsg( "requested mission with uid %d does not exist", id );
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

std::vector<mission *> mission::to_ptr_vector( const std::vector<int> &vec )
{
    std::vector<mission *> result;
    for( auto &id : vec ) {
        const auto miss = find( id );
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
    for( auto &miss : vec ) {
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
        if( mon->mission_id == -1 ) {
            return;
        }
        const auto mission = mission::find( mon->mission_id );
        const auto type = mission->type;
        if( type->goal == MGOAL_FIND_MONSTER ) {
            mission->fail();
        }
        if( type->goal == MGOAL_KILL_MONSTER ) {
            mission->step_complete( 1 );
        }
        return;
    }
    npc *p = dynamic_cast<npc *>( &poor_dead_dude );
    if( p == nullptr ) {
        // Must be the player
        for( auto &miss : g->u.get_active_missions() ) {
            // mission is free and can be reused
            miss->player_id = character_id();
        }
        // The missions remains assigned to the (dead) character. This should not cause any problems
        // as the character is dismissed anyway.
        // Technically, the active missions could be moved to the failed mission section.
        return;
    }
    const auto dead_guys_id = p->getID();
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

void mission::on_talk_with_npc( const character_id npc_id )
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

mission *mission::reserve_random( const mission_origin origin, const tripoint &p,
                                  const character_id npc_id )
{
    const auto type = mission_type::get_random_id( origin, p );
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
        if( type->goal == MGOAL_KILL_MONSTER_TYPE && monster_type != mtype_id::NULL_ID() ) {
            kill_count_to_reach = g->kill_count( monster_type ) + monster_kill_goal;
        } else if( type->goal == MGOAL_KILL_MONSTER_SPEC ) {
            kill_count_to_reach = g->kill_count( monster_species ) + monster_kill_goal;
        }
        type->start( this );
        status = mission_status::in_progress;
    }
}

void mission::fail()
{
    status = mission_status::failure;
    if( g->u.getID() == player_id ) {
        g->u.on_mission_finished( *this );
    }

    type->fail( this );
}

void mission::set_target_to_mission_giver()
{
    const auto giver = g->find_npc( npc_id );
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
    auto &u = g->u;
    if( u.getID() != player_id ) {
        // This is called from npctalk.cpp, the npc should only offer the option to wrap up mission
        // that have been assigned to the current player.
        debugmsg( "mission::wrap_up called, player %d was assigned, but current player is %d",
                  player_id.get_value(), u.getID().get_value() );
    }

    status = mission_status::success;
    u.on_mission_finished( *this );
    std::vector<item_comp> comps;
    switch( type->goal ) {
        case MGOAL_FIND_ITEM_GROUP: {
            inventory tmp_inv = u.crafting_inventory();
            std::vector<item *> items = std::vector<item *>();
            tmp_inv.dump( items );
            Group_tag grp_type = type->group_id;
            itype_id container = type->container_id;
            bool specific_container_required = container != "null";
            bool remove_container = type->remove_container;
            itype_id empty_container = type->empty_container;

            std::map<itype_id, int> matches = std::map<itype_id, int>();
            get_all_item_group_matches(
                items, grp_type, matches,
                container, itype_id( "null" ), specific_container_required );

            std::map<std::string, int>::iterator cnt_it;
            for( cnt_it = matches.begin(); cnt_it != matches.end(); cnt_it++ ) {
                comps.push_back( item_comp( cnt_it->first, cnt_it->second ) );

            }

            u.consume_items( comps );

            if( remove_container ) {
                std::vector<item_comp> container_comp = std::vector<item_comp>();
                if( empty_container != "null" ) {
                    container_comp.push_back( item_comp( empty_container, type->item_count ) );
                    u.consume_items( container_comp );
                } else {
                    container_comp.push_back( item_comp( container, type->item_count ) );
                    u.consume_items( container_comp );
                }
            }
        }
        break;

        case MGOAL_FIND_ITEM:
            comps.push_back( item_comp( type->item_id, item_count ) );
            u.consume_items( comps );
            break;
        case MGOAL_FIND_ANY_ITEM:
            u.remove_mission_items( uid );
            break;
        default:
            //Suppress warnings
            break;
    }

    type->end( this );
}

bool mission::is_complete( const character_id _npc_id ) const
{
    if( status == mission_status::success ) {
        return true;
    }

    auto &u = g->u;
    switch( type->goal ) {
        case MGOAL_GO_TO: {
            const tripoint cur_pos = g->u.global_omt_location();
            return ( rl_dist( cur_pos, target ) <= 1 );
        }

        case MGOAL_GO_TO_TYPE: {
            const auto cur_ter = overmap_buffer.ter( g->u.global_omt_location() );
            return is_ot_match( type->target_id.str(), cur_ter, ot_match_type::type );
        }

        case MGOAL_FIND_ITEM_GROUP: {
            inventory tmp_inv = u.crafting_inventory();
            std::vector<item *> items = std::vector<item *>();
            tmp_inv.dump( items );
            Group_tag grp_type = type->group_id;
            itype_id container = type->container_id;
            bool specific_container_required = container != "null";

            std::map<itype_id, int> matches = std::map<itype_id, int>();
            get_all_item_group_matches(
                items, grp_type, matches,
                container, itype_id( "null" ), specific_container_required );

            int total_match = std::accumulate( matches.begin(), matches.end(), 0,
            []( const std::size_t previous, const std::pair<const std::string, std::size_t> &p ) {
                return previous + p.second;
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
            const inventory &tmp_inv = u.crafting_inventory();
            // TODO: check for count_by_charges and use appropriate player::has_* function
            if( !tmp_inv.has_amount( type->item_id, item_count ) ) {
                return tmp_inv.has_amount( type->item_id, 1 ) && tmp_inv.has_charges( type->item_id, item_count );
            }
        }
        return true;

        case MGOAL_FIND_ANY_ITEM:
            return u.has_mission_item( uid ) && ( !npc_id.is_valid() || npc_id == _npc_id );

        case MGOAL_FIND_MONSTER:
            if( npc_id.is_valid() && npc_id != _npc_id ) {
                return false;
            }
            return g->get_creature_if( [&]( const Creature & critter ) {
                const monster *const mon_ptr = dynamic_cast<const monster *>( &critter );
                return mon_ptr && mon_ptr->mission_id == uid;
            } );

        case MGOAL_RECRUIT_NPC: {
            npc *p = g->find_npc( target_npc_id );
            return p != nullptr && p->get_attitude() == NPCATT_FOLLOW;
        }

        case MGOAL_RECRUIT_NPC_CLASS: {
            const auto npcs = overmap_buffer.get_npcs_near_player( 100 );
            for( auto &npc : npcs ) {
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
        case MGOAL_COMPUTER_TOGGLE:
            return step >= 1;

        case MGOAL_KILL_MONSTER_TYPE:
            return g->kill_count( monster_type ) >= kill_count_to_reach;

        case MGOAL_KILL_MONSTER_SPEC:
            return g->kill_count( monster_species ) >= kill_count_to_reach;

        case MGOAL_CONDITION: {
            // For now, we only allow completing when talking to the mission originator.
            if( npc_id != _npc_id ) {
                return false;
            }

            npc *n = g->find_npc( _npc_id );
            if( n == nullptr ) {
                return false;
            }

            mission_goal_condition_context cc;
            cc.alpha = &u;
            cc.beta = n;

            for( auto &mission : n->chatbin.missions_assigned ) {
                if( mission->get_assigned_player_id() == g->u.getID() ) {
                    cc.missions_assigned.push_back( mission );
                }
            }

            return type->test_goal_condition( cc );
        }

        default:
            return false;
    }
}

void mission::get_all_item_group_matches( std::vector<item *> &items,
        Group_tag &grp_type, std::map<itype_id, int> &matches,
        const itype_id &required_container, const itype_id &actual_container,
        bool &specific_container_required )
{
    for( item *itm : items ) {
        bool correct_container = ( required_container == actual_container ) ||
                                 !specific_container_required;

        bool item_in_group = item_group::group_contains_item( grp_type, itm->typeId() );

        //check whether item itself is target
        if( item_in_group && correct_container ) {
            std::map<std::string, int>::iterator it = matches.find( itm->typeId() );
            if( it != matches.end() ) {
                it->second = ( it->second ) + 1;
            } else {
                matches.insert( std::make_pair( itm->typeId(), 1 ) );
            }
        }

        //recursivly check item contents for target
        if( itm->is_container() && !itm->is_container_empty() ) {
            std::list<item> content_list = itm->contents;

            std::vector<item *> content = std::vector<item *>();

            //list of item into list item*
            std::transform(
                content_list.begin(), content_list.end(),
                std::back_inserter( content ),
            []( item & p ) -> item* {
                return &p;
            } );

            get_all_item_group_matches(
                content, grp_type, matches,
                required_container, ( itm->typeId() ), specific_container_required );
        }
    }
}

bool mission::has_deadline() const
{
    return deadline != 0;
}

time_point mission::get_deadline() const
{
    return deadline;
}

std::string mission::get_description() const
{
    return _( type->description );
}

bool mission::has_target() const
{
    return target != overmap::invalid_tripoint;
}

const tripoint &mission::get_target() const
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

const std::string &mission::get_item_id() const
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

    if( deadline > 0 && calendar::turn > deadline ) {
        fail();
    } else if( !npc_id.is_valid() && is_complete( npc_id ) ) { // No quest giver.
        wrap_up();
    }
}

character_id mission::get_npc_id() const
{
    return npc_id;
}

std::vector<std::pair<int, std::string>> mission::get_likely_rewards()
{
    return type->likely_rewards;
}

void mission::set_target( const tripoint &p )
{
    target = p;
}

void mission::set_target_npc_id( const character_id npc_id )
{
    target_npc_id = npc_id;
}

bool mission::is_assigned() const
{
    return player_id.is_valid() || legacy_no_player_id;
}

character_id mission::get_assigned_player_id() const
{
    return player_id;
}

void mission::set_player_id_legacy_0c( character_id id )
{
    if( !legacy_no_player_id || player_id.is_valid() ) {
        debugmsg( "Not a legacy mission, tried to set id %d", id.get_value() );
    } else {
        player_id = id;
        legacy_no_player_id = false;
    }
}

std::string mission::name()
{
    if( type == nullptr ) {
        return "NULL";
    }
    return type->tname();
}

mission_type_id mission::mission_id()
{
    if( type == nullptr ) {
        return mission_type_id( "NULL" );
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
        return _( response->second );
    }

    return string_format( "Someone forgot to code this message id is %s, topic is %s!",
                          type->id.c_str(), topic.c_str() );
}

mission::mission()
    : deadline( 0 )
{
    type = nullptr;
    status = mission_status::yet_to_start;
    value = 0;
    uid = -1;
    target = tripoint_min;
    item_id = "null";
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

mission_type::mission_type( mission_type_id ID, const std::string &NAME, mission_goal GOAL, int DIF,
                            int VAL,
                            bool URGENT,
                            std::function<bool( const tripoint & )> PLACE,
                            std::function<void( mission * )> START,
                            std::function<void( mission * )> END,
                            std::function<void( mission * )> FAIL ) :
    id( ID ), name( NAME ), goal( GOAL ), difficulty( DIF ), value( VAL ),
    urgent( URGENT ), place( PLACE ), start( START ), end( END ), fail( FAIL )
{
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
    debugmsg( "Invalid mission_status" );
    abort();
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
