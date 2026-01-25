#pragma once
#ifndef CATA_SRC_AI_QUEST_SYSTEM_H
#define CATA_SRC_AI_QUEST_SYSTEM_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <optional>

#include "calendar.h"
#include "type_id.h"
#include "llm_bridge.h"

class npc;

namespace cata_ai {

// Use LLMBridge from cata_llm namespace
using LLMBridge = cata_llm::LLMBridge;

// Quest status
enum class AIQuestStatus {
    AVAILABLE,      // Can be accepted
    ACTIVE,         // Currently being worked on
    COMPLETED,      // Successfully completed
    FAILED,         // Failed
    EXPIRED         // Time ran out
};

// Who can accept this quest
enum class AIQuestTarget {
    PLAYER_ONLY,    // Only player can accept
    NPC_ONLY,       // Only NPCs can accept (NPC-to-NPC)
    ANYONE          // Player or NPC can accept
};

// Quest objective types - what the player/NPC must do
enum class QuestObjectiveType {
    FIND_ITEM,          // Find/obtain a specific item
    FIND_LOCATION,      // Reach a specific location
    KILL_MONSTER,       // Kill specific monster type
    TALK_TO_NPC,        // Talk to a specific NPC
    DELIVER_ITEM,       // Bring item to NPC (requires return)
    INVESTIGATE,        // Go to location and "investigate" (auto-complete on arrival)
    CRAFT_ITEM,         // Craft a specific item
    SURVIVE_TIME,       // Survive for X turns
    CUSTOM              // LLM-determined completion
};

// Quest reward types
struct AIQuestReward {
    int faction_reputation = 0;     // Change to faction reputation
    int cash_value = 0;             // Currency reward
    std::string item_id;            // Item reward (CDDA item ID)
    int item_count = 1;
    std::string skill_id;           // Skill XP reward
    int skill_xp = 0;
    std::string special_unlock;     // Special unlock (e.g., "trader_access")
    bool auto_reward = false;       // True = get reward immediately, False = return to NPC
};

// Quest objective - tracks what needs to be done
struct QuestObjective {
    QuestObjectiveType type = QuestObjectiveType::CUSTOM;
    std::string target;             // Item ID, monster type, NPC name, or location
    int target_count = 1;           // How many needed
    int current_count = 0;          // Progress
    bool completed = false;
    std::string description;        // Human readable
};

// A dynamically generated quest
struct AIQuest {
    std::string id;                 // Unique quest ID
    std::string title;              // Quest title
    std::string description;        // Full description
    std::string objective_text;     // Human-readable objective
    std::string location_hint;      // Where to go

    std::string giver_name;         // NPC who gave the quest
    std::string giver_faction;      // Faction of quest giver
    std::string target_npc;         // If quest is for specific NPC

    AIQuestStatus status = AIQuestStatus::AVAILABLE;
    AIQuestTarget target_type = AIQuestTarget::ANYONE;

    // Structured objective for tracking
    QuestObjective objective;

    int difficulty = 1;             // 1-10 scale
    int priority = 5;               // How urgent (affects NPC behavior)
    int npc_progress = 0;           // 0-100, NPC's progress toward completion

    time_point deadline;            // When quest expires
    time_point accepted_time;       // When quest was accepted

    std::string accepted_by;        // Name of who accepted (player or NPC)
    bool accepted_by_player = false;

    AIQuestReward reward;

    // Faction impacts
    std::map<std::string, int> faction_impacts;  // faction_name -> reputation change

    // World event this quest relates to
    std::string related_event_id;

    // For NPC-to-NPC quests
    std::string delegated_to;       // NPC name if delegated
    std::string delegation_reason;  // Why NPC accepted/rejected

    // Completion tracking
    bool requires_return = false;   // Must return to quest giver
    bool reward_given = false;      // Has reward been claimed
};

// NPC intent when approaching player
enum class NPCIntent {
    OFFER_QUEST,        // Want to give player a quest
    REQUEST_HELP,       // Desperate plea for help
    WARN_DANGER,        // Warning about world events
    TRADE_OPPORTUNITY,  // Special trade related to events
    FACTION_MESSAGE,    // Message from faction leadership
    SEEK_ALLIANCE,      // Want to ally against threat
    BETRAYAL            // Setting up player (hostile intent)
};

struct NPCApproach {
    std::string npc_name;
    NPCIntent intent;
    std::string dialogue;
    std::optional<AIQuest> quest;
    std::string faction;
    int urgency = 5;  // 1-10, affects how aggressively NPC seeks player
};

class AIQuestSystem {
public:
    static AIQuestSystem& instance();

    // Initialize with LLM bridge
    void initialize(std::shared_ptr<LLMBridge> bridge);

    // Generate quests from world events
    AIQuest generate_quest_from_event(const std::string& event_description,
                                       const std::string& event_type,
                                       const std::vector<std::string>& affected_factions);

    // NPC-to-Player quest offering
    NPCApproach generate_npc_approach(npc& approaching_npc,
                                       const std::string& world_context);

    // NPC-to-NPC quest delegation
    bool delegate_quest_to_npc(AIQuest& quest, npc& target_npc);

    // Quest management
    void add_quest(const AIQuest& quest);
    std::vector<AIQuest> get_available_quests() const;
    std::vector<AIQuest> get_active_quests() const;
    std::optional<AIQuest> get_quest_by_id(const std::string& id) const;

    // Quest actions
    bool accept_quest(const std::string& quest_id, bool by_player, const std::string& acceptor_name = "");
    bool complete_quest(const std::string& quest_id);
    bool fail_quest(const std::string& quest_id);
    bool turn_in_quest(const std::string& quest_id);  // Return to NPC to claim reward

    // Quest objective checking - call these to update quest progress
    void check_item_pickup(const std::string& item_id);      // Player picked up item
    void check_monster_kill(const std::string& monster_type); // Player killed monster
    void check_location_reached(const std::string& location); // Player reached location
    void check_npc_talked(const std::string& npc_name);       // Player talked to NPC

    // Manual quest completion (for custom objectives)
    bool try_complete_objective(const std::string& quest_id);

    // Apply rewards
    void apply_quest_rewards(const AIQuest& quest);
    void spawn_item_reward(const std::string& item_id, int count);  // Actually give items

    // NPC quest AI
    void process_npc_quest_ai();  // NPCs actively pursue their quests

    // Faction relationship management
    void modify_faction_reputation(const std::string& faction_name, int change);
    int get_faction_reputation(const std::string& faction_name) const;
    std::string get_faction_standing(const std::string& faction_name) const;

    // NPCs seeking player
    std::vector<NPCApproach> get_pending_approaches() const;
    void add_npc_approach(const NPCApproach& approach);
    void clear_approach(const std::string& npc_name);

    // Update system (call from game loop)
    void on_turn_update(int current_turn);

    // Display quest to player
    void display_quest_offer(const AIQuest& quest);
    void display_quest_log();

private:
    AIQuestSystem() = default;

    std::shared_ptr<LLMBridge> bridge_;
    std::vector<AIQuest> quests_;
    std::vector<NPCApproach> pending_approaches_;
    std::map<std::string, int> faction_reputations_;

    int last_quest_gen_turn_ = 0;
    int quest_counter_ = 0;

    std::string generate_quest_id();
    void check_quest_expirations();
    void process_npc_quest_progress();
    void log_event(const std::string& message);
};

} // namespace cata_ai

#endif // CATA_SRC_AI_QUEST_SYSTEM_H
