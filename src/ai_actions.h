#pragma once
#ifndef CATA_SRC_AI_ACTIONS_H
#define CATA_SRC_AI_ACTIONS_H

#include <string>

class npc;

namespace cata_ai
{

/**
 * AIActionExecutor - Executes LLM-generated actions on NPCs
 *
 * Maps action strings from LLM JSON responses to actual game state changes.
 */
class AIActionExecutor
{
    public:
        /**
         * Execute an action on an NPC
         * @param p The NPC to act on
         * @param action The action string (MAKE_ANGRY, RUN_AWAY, etc.)
         * @param target The target of the action (usually "player")
         * @return true if action was executed successfully
         */
        static bool execute( npc &p, const std::string &action, const std::string &target );

        /**
         * Check if an action string is valid
         */
        static bool is_valid_action( const std::string &action );

    private:
        // Individual action implementations
        static void action_make_angry( npc &p );
        static void action_run_away( npc &p );
        static void action_give_quest( npc &p );
        static void action_trade( npc &p );
        static void action_follow( npc &p );
};

} // namespace cata_ai

#endif // CATA_SRC_AI_ACTIONS_H
