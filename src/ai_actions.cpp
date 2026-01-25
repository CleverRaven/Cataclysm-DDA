#include "ai_actions.h"

#include "npc.h"
#include "game.h"
#include "map.h"
#include "messages.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "npctrade.h"
#include "avatar.h"
#include "translations.h"

static const faction_id faction_your_followers( "your_followers" );

namespace cata_ai
{

bool AIActionExecutor::is_valid_action( const std::string &action )
{
    return action == "NONE" ||
           action == "MAKE_ANGRY" ||
           action == "RUN_AWAY" ||
           action == "GIVE_QUEST" ||
           action == "TRADE" ||
           action == "FOLLOW";
}

bool AIActionExecutor::execute( npc &p, const std::string &action, const std::string &/*target*/ )
{
    if( action == "NONE" ) {
        return true;  // No action needed
    }

    if( action == "MAKE_ANGRY" ) {
        action_make_angry( p );
        return true;
    }

    if( action == "RUN_AWAY" ) {
        action_run_away( p );
        return true;
    }

    if( action == "GIVE_QUEST" ) {
        action_give_quest( p );
        return true;
    }

    if( action == "TRADE" ) {
        action_trade( p );
        return true;
    }

    if( action == "FOLLOW" ) {
        action_follow( p );
        return true;
    }

    // Unknown action
    return false;
}

void AIActionExecutor::action_make_angry( npc &p )
{
    const map &here = get_map();

    if( p.get_attitude() == NPCATT_KILL ) {
        return;  // Already hostile
    }

    if( p.sees( here, get_player_character() ) ) {
        add_msg( _( "%s turns hostile!" ), p.get_name() );
    }

    get_event_bus().send<event_type::npc_becomes_hostile>( p.getID(), p.name );
    p.set_attitude( NPCATT_KILL );
}

void AIActionExecutor::action_run_away( npc &p )
{
    add_msg( _( "%s turns to flee!" ), p.get_name() );
    p.set_attitude( NPCATT_FLEE_TEMP );
}

void AIActionExecutor::action_give_quest( npc &p )
{
    // Check if NPC has available missions
    if( !p.chatbin.missions.empty() ) {
        // Select the first available mission
        p.chatbin.mission_selected = p.chatbin.missions.front();
        add_msg( _( "%s has a task for you." ), p.get_name() );
    } else {
        add_msg( _( "%s doesn't have any tasks right now." ), p.get_name() );
    }
}

void AIActionExecutor::action_trade( npc &p )
{
    // Initiate trading with the NPC
    npc_trading::trade( p, 0, _( "Trade" ) );
}

void AIActionExecutor::action_follow( npc &p )
{
    // Add NPC as follower
    g->add_npc_follower( p.getID() );
    p.set_attitude( NPCATT_FOLLOW );
    p.set_fac( faction_your_followers );

    // Transfer NPC's cash to player
    get_player_character().cash += p.cash;
    p.cash = 0;

    add_msg( _( "%s agrees to follow you." ), p.get_name() );
}

} // namespace cata_ai
