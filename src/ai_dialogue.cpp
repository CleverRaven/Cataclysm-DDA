#include "ai_dialogue.h"
#include "npc.h"
#include "game.h"
#include "map.h"
#include "messages.h"
#include "faction.h"

#include <sstream>
#include <fstream>
#include <ctime>

// File-based logging for AI dialogue interactions
static void log_to_file( const std::string &message )
{
    std::ofstream log( "ai_dialogue.log", std::ios::app );
    if( log.is_open() ) {
        std::time_t now = std::time( nullptr );
        char timestamp[64];
        std::strftime( timestamp, sizeof( timestamp ), "%Y-%m-%d %H:%M:%S", std::localtime( &now ) );
        log << "[" << timestamp << "] " << message << "\n";
        log.flush();
    }
}

namespace cata_ai
{

// Singleton instance
AIDialogueSystem &AIDialogueSystem::instance()
{
    static AIDialogueSystem inst;
    return inst;
}

AIDialogueSystem::AIDialogueSystem()
    : bridge_( std::make_unique<cata_llm::LLMBridge>() )
{
    // Ensure Qdrant collection exists on startup
    bridge_->ensure_collection_exists();

    // Enable AI dialogue for all NPCs by default for testing
    // TODO: Make this configurable via game options
    ai_all_npcs_ = true;
}

bool AIDialogueSystem::is_service_available() const
{
    if( !service_checked_ ) {
        service_available_ = bridge_->check_ollama_connection() &&
                             bridge_->check_qdrant_connection();
        service_checked_ = true;
    }
    return service_available_;
}

bool AIDialogueSystem::is_ai_enabled_npc( const npc &p ) const
{
    if( ai_all_npcs_ ) {
        return true;
    }
    return ai_enabled_npcs_.find( p.get_name() ) != ai_enabled_npcs_.end();
}

void AIDialogueSystem::enable_ai_for_npc( const std::string &npc_name )
{
    ai_enabled_npcs_.insert( npc_name );
}

void AIDialogueSystem::enable_ai_for_all()
{
    ai_all_npcs_ = true;
}

void AIDialogueSystem::disable_ai_for_npc( const std::string &npc_name )
{
    ai_enabled_npcs_.erase( npc_name );
}

std::string AIDialogueSystem::build_npc_context( const npc &p )
{
    std::stringstream ctx;

    // Basic NPC info
    ctx << "NPC_NAME: " << p.get_name() << "\n";
    ctx << "NPC_ATTITUDE: " << npc_attitude_name( p.get_attitude() ) << "\n";

    // NPC class/role if available
    if( p.myclass ) {
        ctx << "NPC_CLASS: " << p.myclass.str() << "\n";
    }

    // Current mission
    if( p.mission != NPC_MISSION_NULL ) {
        ctx << "NPC_MISSION: " << static_cast<int>( p.mission ) << "\n";
    }

    // Location context (simplified)
    tripoint_bub_ms pos = p.pos_bub();
    ctx << "LOCATION: " << pos.x() << "," << pos.y() << "," << pos.z() << "\n";

    // Health/condition hints (without exact numbers for immersion)
    if( p.hp_percentage() < 30 ) {
        ctx << "CONDITION: badly injured\n";
    } else if( p.hp_percentage() < 70 ) {
        ctx << "CONDITION: wounded\n";
    } else {
        ctx << "CONDITION: healthy\n";
    }

    // Hunger/thirst state
    if( p.get_hunger() > 100 ) {
        ctx << "HUNGER: starving\n";
    } else if( p.get_hunger() > 50 ) {
        ctx << "HUNGER: hungry\n";
    }

    if( p.get_thirst() > 100 ) {
        ctx << "THIRST: dehydrated\n";
    } else if( p.get_thirst() > 50 ) {
        ctx << "THIRST: thirsty\n";
    }

    // Faction info
    if( p.get_faction() ) {
        ctx << "FACTION: " << p.get_faction()->get_name() << "\n";
    }

    return ctx.str();
}

std::string AIDialogueSystem::build_system_prompt( const npc &p )
{
    std::stringstream prompt;

    prompt << "You are " << p.get_name() << ", an NPC in Cataclysm: Dark Days Ahead, ";
    prompt << "a post-apocalyptic survival roguelike game.\n\n";

    prompt << "Stay in character. Your current attitude is: " << npc_attitude_name( p.get_attitude() ) << ".\n";
    prompt << "React appropriately based on your attitude and the player's actions.\n\n";

    prompt << "You MUST respond with valid JSON in this exact format:\n";
    prompt << "{\n";
    prompt << "  \"response\": \"Your dialogue text here\",\n";
    prompt << "  \"action\": \"NONE\",\n";
    prompt << "  \"target\": \"player\"\n";
    prompt << "}\n\n";

    prompt << "Valid actions:\n";
    prompt << "- NONE: Just talk, no special action\n";
    prompt << "- MAKE_ANGRY: Become hostile (use when insulted, threatened, or betrayed)\n";
    prompt << "- RUN_AWAY: Flee (use when scared or outmatched)\n";
    prompt << "- GIVE_QUEST: Offer a task (use when player offers help and you need something)\n";
    prompt << "- TRADE: Start trading (use when player wants to trade)\n";
    prompt << "- FOLLOW: Follow the player (use when agreeing to join them)\n\n";

    prompt << "Keep responses concise (1-3 sentences). Match the game's gritty tone.";

    return prompt.str();
}

AIDialogueResult AIDialogueSystem::generate_dialogue( npc &p, const std::string &player_input )
{
    AIDialogueResult result;

    log_to_file( "=== AI DIALOGUE START ===" );
    log_to_file( "NPC: " + p.get_name() + " | Player input: " + player_input );

    // Check service availability
    if( !is_service_available() ) {
        result.error = "LLM services not available";
        log_to_file( "ERROR: " + result.error );
        return result;
    }

    // Build context
    std::string npc_context = build_npc_context( p );
    log_to_file( "NPC Context built" );

    // Retrieve relevant memories
    std::vector<float> embedding = bridge_->get_embedding( player_input );
    std::string memories;
    if( !embedding.empty() ) {
        memories = bridge_->retrieve_memory( embedding, 3 );
        log_to_file( "Retrieved " + std::to_string( memories.length() ) + " chars of memories" );
    }

    // Combine context
    std::stringstream full_context;
    full_context << npc_context;
    if( !memories.empty() ) {
        full_context << "\nRELEVANT_MEMORIES:\n" << memories;
    }

    // Build system prompt
    std::string system_prompt = build_system_prompt( p );

    // Query LLM
    log_to_file( "Querying LLM..." );
    cata_llm::LLMResult llm_result = bridge_->query_llm(
                                         full_context.str(),
                                         player_input,
                                         system_prompt
                                     );

    // Transfer results
    result.response_text = llm_result.response;
    result.action = llm_result.action.empty() ? "NONE" : llm_result.action;
    result.target = llm_result.target.empty() ? "player" : llm_result.target;
    result.success = llm_result.success;
    result.error = llm_result.error;

    log_to_file( "LLM Response: " + result.response_text );
    log_to_file( "Action: " + result.action + " | Target: " + result.target );

    // Log the interaction for future RAG retrieval
    if( result.success ) {
        log_interaction( p, player_input, result );
        log_to_file( "Interaction logged to Qdrant" );
    } else {
        log_to_file( "ERROR: " + result.error );
    }

    log_to_file( "=== AI DIALOGUE END ===" );
    return result;
}

void AIDialogueSystem::log_world_event( const std::string &event_text, const std::string &metadata )
{
    if( !is_service_available() ) {
        return;
    }
    bridge_->store_memory( event_text, metadata );
}

void AIDialogueSystem::log_interaction( const npc &p, const std::string &player_input,
                                        const AIDialogueResult &result )
{
    if( !is_service_available() ) {
        return;
    }

    std::stringstream memory;
    memory << "Interaction with " << p.get_name() << ": ";
    memory << "Player said: \"" << player_input << "\". ";
    memory << p.get_name() << " responded: \"" << result.response_text << "\". ";
    if( result.action != "NONE" ) {
        memory << "Action taken: " << result.action << ".";
    }

    std::stringstream metadata;
    metadata << "{\"npc\": \"" << p.get_name() << "\", ";
    metadata << "\"action\": \"" << result.action << "\", ";
    metadata << "\"type\": \"dialogue\"}";

    bridge_->store_memory( memory.str(), metadata.str() );
}

} // namespace cata_ai
