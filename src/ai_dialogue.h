#pragma once
#ifndef CATA_SRC_AI_DIALOGUE_H
#define CATA_SRC_AI_DIALOGUE_H

#include "llm_bridge.h"

#include <memory>
#include <set>
#include <string>

class npc;

namespace cata_ai
{

/**
 * Result of AI dialogue generation
 */
struct AIDialogueResult {
    std::string response_text;  // The NPC's dialogue
    std::string action;         // Action to execute (NONE, MAKE_ANGRY, RUN_AWAY, etc.)
    std::string target;         // Target of the action (player, self, etc.)
    bool success = false;       // Whether generation succeeded
    std::string error;          // Error message if failed
};

/**
 * AIDialogueSystem - Singleton managing AI-powered NPC dialogue
 *
 * Provides RAG-augmented dialogue generation for NPCs.
 * Uses Ollama for LLM and Qdrant for memory storage.
 */
class AIDialogueSystem
{
    public:
        /**
         * Get the singleton instance
         */
        static AIDialogueSystem &instance();

        // Delete copy/move constructors
        AIDialogueSystem( const AIDialogueSystem & ) = delete;
        AIDialogueSystem &operator=( const AIDialogueSystem & ) = delete;

        /**
         * Generate AI dialogue for an NPC
         * @param p The NPC speaking
         * @param player_input What the player said
         * @return AIDialogueResult with response and optional action
         */
        AIDialogueResult generate_dialogue( npc &p, const std::string &player_input );

        /**
         * Check if an NPC has AI dialogue enabled
         */
        bool is_ai_enabled_npc( const npc &p ) const;

        /**
         * Enable AI dialogue for a specific NPC by name
         */
        void enable_ai_for_npc( const std::string &npc_name );

        /**
         * Enable AI dialogue for all NPCs
         */
        void enable_ai_for_all();

        /**
         * Disable AI dialogue for a specific NPC
         */
        void disable_ai_for_npc( const std::string &npc_name );

        /**
         * Check if the LLM services are available
         */
        bool is_service_available() const;

        /**
         * Store a world event in memory for RAG retrieval
         */
        void log_world_event( const std::string &event_text, const std::string &metadata = "" );

        /**
         * Store an NPC interaction in memory
         */
        void log_interaction( const npc &p, const std::string &player_input,
                              const AIDialogueResult &result );

    private:
        AIDialogueSystem();

        // LLM Bridge for API calls
        std::unique_ptr<cata_llm::LLMBridge> bridge_;

        // Set of NPC names with AI enabled (empty = all NPCs when ai_all_npcs_ is true)
        std::set<std::string> ai_enabled_npcs_;

        // Flag to enable AI for all NPCs
        bool ai_all_npcs_ = false;

        // Service availability (cached)
        mutable bool service_checked_ = false;
        mutable bool service_available_ = false;

        /**
         * Build context string from NPC state
         */
        std::string build_npc_context( const npc &p );

        /**
         * Build the system prompt for NPC dialogue
         */
        std::string build_system_prompt( const npc &p );
};

} // namespace cata_ai

#endif // CATA_SRC_AI_DIALOGUE_H
