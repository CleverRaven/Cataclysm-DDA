#pragma once
#ifndef CATA_SRC_AI_WORLD_SYSTEM_H
#define CATA_SRC_AI_WORLD_SYSTEM_H

#include "llm_bridge.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace cata_ai {

// Use LLMBridge from cata_llm namespace
using LLMBridge = cata_llm::LLMBridge;

// Legacy simple quest structure for world system internal tracking
// (The new comprehensive AIQuest is in ai_quest_system.h)
struct WorldSystemQuest {
    std::string id;
    std::string title;
    std::string description;
    std::string giver_npc;
    std::string target_type;  // "fetch", "kill", "escort", "investigate", "negotiate"
    std::string target;
    std::string reward;
    int difficulty;  // 1-10
    bool is_active;
    bool is_completed;
    std::vector<std::string> related_npcs;
    std::string faction_impact;  // Which faction this affects
};

// Faction relationship
struct FactionRelation {
    std::string faction_a;
    std::string faction_b;
    int relationship;  // -100 to 100
    std::string status;  // "allied", "friendly", "neutral", "tense", "hostile", "war"
    std::vector<std::string> recent_events;
};

// NPC-NPC interaction result
struct NPCInteraction {
    std::string npc_a;
    std::string npc_b;
    std::string interaction_type;  // "conversation", "trade", "conflict", "cooperation"
    std::string dialogue;
    std::string outcome;
    int relationship_change;
    std::string world_event;  // Optional world state change
};

// World event
struct WorldEvent {
    std::string id;
    std::string type;  // "faction_conflict", "resource_shortage", "new_threat", "alliance", "betrayal"
    std::string description;
    std::vector<std::string> affected_npcs;
    std::vector<std::string> affected_factions;
    int severity;  // 1-10
    bool is_active;
    std::string player_hook;  // How player can engage
};

// Main AI World System
class AIWorldSystem {
public:
    static AIWorldSystem& instance();
    
    // Initialize with LLM bridge
    void initialize(std::shared_ptr<LLMBridge> bridge);
    
    // Main game loop hook - called each turn
    void on_turn_update(int turn_number);
    
    // NPC-NPC Interactions
    NPCInteraction generate_npc_interaction(const std::string& npc_a, const std::string& npc_b,
                                            const std::string& location);
    void process_npc_interactions();
    void process_npc_approaching_player();  // NPC proactively seeks player
    
    // Dynamic Quest Generation (legacy simple quests)
    WorldSystemQuest generate_quest_for_player(const std::string& npc_name, const std::string& player_context);
    std::vector<WorldSystemQuest> get_active_quests() const;
    void complete_quest(const std::string& quest_id);
    
    // Faction System
    void update_faction_relations(const std::string& faction, const std::string& event);
    FactionRelation get_faction_relation(const std::string& faction_a, const std::string& faction_b);
    std::string get_faction_status(const std::string& faction);
    
    // World Events
    void generate_world_event();
    void process_world_events();
    void process_npc_quest_offers();  // NPC-to-NPC quest delegation
    std::vector<WorldEvent> get_active_events() const;
    
    // Storyline tracking
    std::string get_current_storyline_summary();
    void add_storyline_event(const std::string& event);
    
    // Logging
    void enable_logging(bool enable);

private:
    AIWorldSystem();
    std::shared_ptr<LLMBridge> bridge_;

    std::vector<WorldSystemQuest> active_quests_;
    std::vector<WorldEvent> world_events_;
    std::map<std::string, std::map<std::string, FactionRelation>> faction_relations_;
    std::vector<std::string> storyline_events_;
    std::vector<NPCInteraction> recent_interactions_;
    
    int last_interaction_turn_;
    int last_event_turn_;
    bool logging_enabled_;
    
    // Internal helpers
    std::string build_world_context();
    void log_event(const std::string& message);
};

} // namespace cata_ai

#endif // CATA_SRC_AI_WORLD_SYSTEM_H
