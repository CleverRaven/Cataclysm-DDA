#include "ai_world_system.h"
#include "ai_quest_system.h"
#include "llm_bridge.h"
#include "game.h"
#include "npc.h"
#include "messages.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "faction.h"
#include <fstream>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <random>

namespace cata_ai {

static void log_to_file(const std::string& message) {
    std::ofstream log("ai_world_system.log", std::ios::app);
    if (log.is_open()) {
        std::time_t now = std::time(nullptr);
        char timestamp[64];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        log << "[" << timestamp << "] " << message << "\n";
        log.flush();
    }
}

AIWorldSystem& AIWorldSystem::instance() {
    static AIWorldSystem instance;
    return instance;
}

AIWorldSystem::AIWorldSystem() 
    : last_interaction_turn_(0), last_event_turn_(0), logging_enabled_(true) {
    
    // Initialize default faction relations
    faction_relations_["Free Merchants"]["Tacoma Commune"] = {
        "Free Merchants", "Tacoma Commune", 20, "friendly", {}
    };
    faction_relations_["Free Merchants"]["Hell's Raiders"] = {
        "Free Merchants", "Hell's Raiders", -60, "hostile", {}
    };
    faction_relations_["Tacoma Commune"]["Hell's Raiders"] = {
        "Tacoma Commune", "Hell's Raiders", -40, "tense", {}
    };
}

void AIWorldSystem::initialize(std::shared_ptr<LLMBridge> bridge) {
    bridge_ = bridge;
    log_to_file("=== AI WORLD SYSTEM INITIALIZED ===");

    // Initialize quest system with same bridge
    AIQuestSystem::instance().initialize(bridge);
}

void AIWorldSystem::enable_logging(bool enable) {
    logging_enabled_ = enable;
}

void AIWorldSystem::log_event(const std::string& message) {
    if (logging_enabled_) {
        log_to_file(message);
    }
}

std::string AIWorldSystem::build_world_context() {
    std::stringstream ctx;
    ctx << "CURRENT WORLD STATE:\n";
    
    // Active quests
    ctx << "Active Quests: " << active_quests_.size() << "\n";
    for (const auto& quest : active_quests_) {
        if (quest.is_active && !quest.is_completed) {
            ctx << "  - " << quest.title << " (from " << quest.giver_npc << ")\n";
        }
    }
    
    // Faction relations
    ctx << "Faction Relations:\n";
    for (const auto& [faction, relations] : faction_relations_) {
        for (const auto& [other, rel] : relations) {
            ctx << "  - " << faction << " <-> " << other << ": " << rel.status 
                << " (" << rel.relationship << ")\n";
        }
    }
    
    // Active world events
    ctx << "World Events: " << world_events_.size() << " active\n";
    for (const auto& event : world_events_) {
        if (event.is_active) {
            ctx << "  - " << event.type << ": " << event.description << "\n";
        }
    }
    
    // Recent storyline
    ctx << "Recent Events:\n";
    int count = 0;
    for (auto it = storyline_events_.rbegin(); it != storyline_events_.rend() && count < 5; ++it, ++count) {
        ctx << "  - " << *it << "\n";
    }
    
    return ctx.str();
}

void AIWorldSystem::on_turn_update(int turn_number) {
    // Process NPC interactions every 100 turns
    if (turn_number - last_interaction_turn_ >= 100) {
        process_npc_interactions();
        last_interaction_turn_ = turn_number;
    }

    // Generate world events every 500 turns
    if (turn_number - last_event_turn_ >= 500) {
        generate_world_event();
        last_event_turn_ = turn_number;
    }

    // NPC proactively approaches player every 300 turns
    static int last_npc_approach_turn = 0;
    if (turn_number - last_npc_approach_turn >= 300) {
        process_npc_approaching_player();
        last_npc_approach_turn = turn_number;
    }

    // Update quest system
    AIQuestSystem::instance().on_turn_update(turn_number);

    // Process ongoing world events
    process_world_events();
}

void AIWorldSystem::process_npc_approaching_player() {
    // Get nearby NPCs
    std::vector<npc*> nearby_npcs;
    for (npc &guy : g->all_npcs()) {
        if (!guy.is_dead() && !guy.is_hallucination()) {
            nearby_npcs.push_back(&guy);
        }
    }

    if (nearby_npcs.empty()) return;

    // 30% chance an NPC approaches player
    if (!one_in(3)) return;

    npc* approaching = nearby_npcs[rng(0, nearby_npcs.size() - 1)];

    // Generate approach based on world context
    std::string world_ctx = build_world_context();
    auto& quest_system = AIQuestSystem::instance();

    NPCApproach approach = quest_system.generate_npc_approach(*approaching, world_ctx);

    // Display the approach
    add_msg(m_warning, _("%s is approaching you..."), approaching->get_name());
    add_msg(m_neutral, _("%s: \"%s\""), approaching->get_name(), approach.dialogue);

    // Handle based on intent
    switch (approach.intent) {
        case NPCIntent::OFFER_QUEST:
        case NPCIntent::REQUEST_HELP:
            if (approach.quest.has_value()) {
                add_msg(m_good, _("=== QUEST OFFER FROM %s ==="), approaching->get_name());
                add_msg(m_info, _("Quest: %s"), approach.quest->title);
                add_msg(m_neutral, _("Objective: %s"), approach.quest->objective_text);
                add_msg(m_neutral, _("Reward: $%d + %d %s reputation"),
                        approach.quest->reward.cash_value,
                        approach.quest->reward.faction_reputation,
                        approach.quest->giver_faction);
                add_msg(m_info, _("(This quest is now available in your quest log)"));

                // Add to quest system
                quest_system.add_quest(approach.quest.value());

                add_storyline_event(approaching->get_name() + " offered quest: " +
                                    approach.quest->title);
            }
            break;

        case NPCIntent::WARN_DANGER:
            add_msg(m_warning, _("%s has warned you about danger!"), approaching->get_name());
            add_storyline_event(approaching->get_name() + " warned player about danger");
            break;

        case NPCIntent::TRADE_OPPORTUNITY:
            add_msg(m_info, _("%s has a special trade opportunity!"), approaching->get_name());
            add_storyline_event(approaching->get_name() + " offered special trade");
            break;

        case NPCIntent::FACTION_MESSAGE:
            add_msg(m_info, _("%s brings a message from their faction."), approaching->get_name());
            add_storyline_event("Faction message delivered by " + approaching->get_name());
            break;

        case NPCIntent::SEEK_ALLIANCE:
            add_msg(m_good, _("%s wants to form an alliance!"), approaching->get_name());
            add_storyline_event(approaching->get_name() + " sought alliance with player");
            break;

        case NPCIntent::BETRAYAL:
            add_msg(m_bad, _("%s seems to have hostile intentions..."), approaching->get_name());
            add_storyline_event("WARNING: " + approaching->get_name() + " may be planning betrayal");
            break;
    }

    log_event("NPC approached player: " + approaching->get_name() +
              " with intent " + std::to_string(static_cast<int>(approach.intent)));
}

NPCInteraction AIWorldSystem::generate_npc_interaction(const std::string& npc_a, 
                                                        const std::string& npc_b,
                                                        const std::string& location) {
    if (!bridge_) {
        return {"", "", "", "", "", 0, ""};
    }
    
    log_event("=== NPC INTERACTION START ===");
    log_event("NPCs: " + npc_a + " <-> " + npc_b + " at " + location);
    
    std::string world_ctx = build_world_context();
    
    std::string prompt = "You are simulating an interaction between two NPCs in a post-apocalyptic world.\n\n";
    prompt += "WORLD CONTEXT:\n" + world_ctx + "\n\n";
    prompt += "NPC_A: " + npc_a + "\n";
    prompt += "NPC_B: " + npc_b + "\n";
    prompt += "LOCATION: " + location + "\n\n";
    prompt += "Generate a realistic interaction between these NPCs.\n\n";
    prompt += "Output ONLY valid JSON:\n";
    prompt += "{\n";
    prompt += "  \"interaction_type\": \"conversation\",\n";
    prompt += "  \"npc_a_dialogue\": \"What NPC_A says\",\n";
    prompt += "  \"npc_b_dialogue\": \"What NPC_B responds\",\n";
    prompt += "  \"outcome\": \"Brief description of what happens\",\n";
    prompt += "  \"relationship_change\": 0,\n";
    prompt += "  \"world_event\": \"\"\n";
    prompt += "}";

    std::string response = bridge_->query_llm_raw(prompt,
        "You simulate post-apocalyptic NPC interactions. Output ONLY valid JSON.");
    log_event("LLM Response: " + response);

    NPCInteraction interaction;
    interaction.npc_a = npc_a;
    interaction.npc_b = npc_b;
    
    try {
        auto j = nlohmann::json::parse(response);
        interaction.interaction_type = j.value("interaction_type", "conversation");
        interaction.dialogue = j.value("npc_a_dialogue", "") + " | " + j.value("npc_b_dialogue", "");
        interaction.outcome = j.value("outcome", "");
        interaction.relationship_change = j.value("relationship_change", 0);
        interaction.world_event = j.value("world_event", "");
        
        // Apply relationship change
        // Update storyline
        if (!interaction.outcome.empty()) {
            add_storyline_event(npc_a + " and " + npc_b + ": " + interaction.outcome);
        }
        
        if (!interaction.world_event.empty()) {
            add_storyline_event("WORLD: " + interaction.world_event);
        }
        
    } catch (...) {
        interaction.interaction_type = "conversation";
        interaction.dialogue = "They exchanged brief words.";
        interaction.outcome = "Nothing significant happened.";
    }
    
    recent_interactions_.push_back(interaction);
    log_event("Outcome: " + interaction.outcome);
    log_event("=== NPC INTERACTION END ===");
    
    return interaction;
}

void AIWorldSystem::process_npc_interactions() {
    if (!bridge_) {
        // Lazy initialization of bridge
        bridge_ = std::make_shared<LLMBridge>();
        if (!bridge_->check_ollama_connection()) {
            log_event("AI World System: Ollama not available, skipping NPC interactions");
            return;
        }
        log_event("=== AI WORLD SYSTEM INITIALIZED ===");
    }

    log_event("Processing scheduled NPC interactions...");

    // Get all NPCs from the game
    std::vector<npc*> nearby_npcs;
    for (npc &guy : g->all_npcs()) {
        if (!guy.is_dead() && !guy.is_hallucination()) {
            nearby_npcs.push_back(&guy);
        }
    }

    log_event("NPCs loaded in world: " + std::to_string(nearby_npcs.size()));

    // Need at least 2 NPCs for interaction
    if (nearby_npcs.size() < 2) {
        log_event("Not enough NPCs for interaction (need 2+)");
        return;
    }

    // Randomly select two NPCs to interact (50% chance per turn cycle)
    if (one_in(2)) {
        size_t idx_a = rng(0, nearby_npcs.size() - 1);
        size_t idx_b = rng(0, nearby_npcs.size() - 1);
        while (idx_b == idx_a && nearby_npcs.size() > 1) {
            idx_b = rng(0, nearby_npcs.size() - 1);
        }

        npc* npc_a = nearby_npcs[idx_a];
        npc* npc_b = nearby_npcs[idx_b];

        // Generate interaction
        NPCInteraction interaction = generate_npc_interaction(
            npc_a->get_name(),
            npc_b->get_name(),
            "Refugee Center"
        );

        // Display to player if significant
        if (!interaction.outcome.empty()) {
            add_msg(m_info, _("You notice %s and %s talking nearby..."),
                    npc_a->get_name(), npc_b->get_name());
            add_msg(m_neutral, _("%s: \"%s\""), npc_a->get_name(),
                    interaction.dialogue.substr(0, interaction.dialogue.find(" | ")));

            // Check if there's a world event from the interaction
            if (!interaction.world_event.empty()) {
                add_msg(m_warning, _("WORLD: %s"), interaction.world_event);
                add_storyline_event("NPC interaction triggered: " + interaction.world_event);
            }
        }
    }
}

WorldSystemQuest AIWorldSystem::generate_quest_for_player(const std::string& npc_name,
                                                  const std::string& player_context) {
    if (!bridge_) {
        return {};
    }

    log_event("=== QUEST GENERATION START ===");
    log_event("Quest giver: " + npc_name);

    std::string world_ctx = build_world_context();

    std::string prompt = "You are generating a dynamic quest for a post-apocalyptic survival game.\n\n";
    prompt += "WORLD CONTEXT:\n" + world_ctx + "\n\n";
    prompt += "QUEST GIVER: " + npc_name + "\n";
    prompt += "PLAYER CONTEXT: " + player_context + "\n\n";
    prompt += "Create an engaging quest that fits the current world state.\n\n";
    prompt += "Output ONLY valid JSON:\n";
    prompt += "{\n";
    prompt += "  \"title\": \"Quest title\",\n";
    prompt += "  \"description\": \"Detailed quest description\",\n";
    prompt += "  \"target_type\": \"fetch\",\n";
    prompt += "  \"target\": \"What the player needs to deal with\",\n";
    prompt += "  \"reward\": \"What the player gets\",\n";
    prompt += "  \"difficulty\": 5,\n";
    prompt += "  \"related_npcs\": [],\n";
    prompt += "  \"faction_impact\": \"Which faction this affects\",\n";
    prompt += "  \"dialogue\": \"What the NPC says\"\n";
    prompt += "}";

    std::string response = bridge_->query_llm_raw(prompt,
        "You generate dynamic quests for a survival game. Output ONLY valid JSON.");
    log_event("LLM Response: " + response);

    WorldSystemQuest quest;
    quest.giver_npc = npc_name;
    quest.is_active = true;
    quest.is_completed = false;
    
    try {
        auto j = nlohmann::json::parse(response);
        quest.id = "quest_" + std::to_string(std::time(nullptr));
        quest.title = j.value("title", "Unknown Quest");
        quest.description = j.value("description", "");
        quest.target_type = j.value("target_type", "fetch");
        quest.target = j.value("target", "");
        quest.reward = j.value("reward", "Gratitude");
        quest.difficulty = j.value("difficulty", 5);
        quest.faction_impact = j.value("faction_impact", "");
        
        if (j.contains("related_npcs") && j["related_npcs"].is_array()) {
            for (const auto& npc : j["related_npcs"]) {
                quest.related_npcs.push_back(npc.get<std::string>());
            }
        }
        
        active_quests_.push_back(quest);
        add_storyline_event("NEW QUEST: " + quest.title + " from " + npc_name);
        
    } catch (...) {
        quest.title = "Help " + npc_name;
        quest.description = "Assist this survivor with their needs.";
        quest.difficulty = 3;
    }
    
    log_event("Generated quest: " + quest.title);
    log_event("=== QUEST GENERATION END ===");
    
    return quest;
}

std::vector<WorldSystemQuest> AIWorldSystem::get_active_quests() const {
    std::vector<WorldSystemQuest> active;
    for (const auto& quest : active_quests_) {
        if (quest.is_active && !quest.is_completed) {
            active.push_back(quest);
        }
    }
    return active;
}

void AIWorldSystem::complete_quest(const std::string& quest_id) {
    for (auto& quest : active_quests_) {
        if (quest.id == quest_id) {
            quest.is_completed = true;
            add_storyline_event("QUEST COMPLETE: " + quest.title);
            
            // Apply faction impact
            if (!quest.faction_impact.empty()) {
                add_storyline_event("FACTION IMPACT: " + quest.faction_impact);
            }
            
            log_event("Quest completed: " + quest.title);
            break;
        }
    }
}

void AIWorldSystem::update_faction_relations(const std::string& faction, const std::string& event) {
    log_event("Faction update: " + faction + " - " + event);
    
    // This would use LLM to determine impact on all related factions
    if (bridge_) {
        std::string prompt = "Given this faction event, determine the relationship changes:\n";
        prompt += "FACTION: " + faction + "\n";
        prompt += "EVENT: " + event + "\n\n";
        prompt += "Output JSON with relationship changes to other factions.\n";
        // Would parse response and update relations
    }
    
    add_storyline_event("FACTION: " + faction + " - " + event);
}

FactionRelation AIWorldSystem::get_faction_relation(const std::string& faction_a, 
                                                     const std::string& faction_b) {
    if (faction_relations_.count(faction_a) && faction_relations_[faction_a].count(faction_b)) {
        return faction_relations_[faction_a][faction_b];
    }
    return {faction_a, faction_b, 0, "neutral", {}};
}

std::string AIWorldSystem::get_faction_status(const std::string& faction) {
    std::stringstream status;
    status << "Faction: " << faction << "\nRelations:\n";
    
    if (faction_relations_.count(faction)) {
        for (const auto& [other, rel] : faction_relations_[faction]) {
            status << "  " << other << ": " << rel.status << " (" << rel.relationship << ")\n";
        }
    }
    
    return status.str();
}

void AIWorldSystem::generate_world_event() {
    if (!bridge_) return;
    
    log_event("=== WORLD EVENT GENERATION ===");
    
    std::string world_ctx = build_world_context();
    
    std::string prompt = "Generate a world event for a post-apocalyptic survival game.\n\n";
    prompt += "CURRENT WORLD STATE:\n" + world_ctx + "\n\n";
    prompt += "Create an event that creates tension or opportunity.\n\n";
    prompt += "Output ONLY valid JSON:\n";
    prompt += "{\n";
    prompt += "  \"type\": \"discovery\",\n";
    prompt += "  \"description\": \"What is happening\",\n";
    prompt += "  \"affected_npcs\": [],\n";
    prompt += "  \"affected_factions\": [],\n";
    prompt += "  \"severity\": 5,\n";
    prompt += "  \"player_hook\": \"How the player can get involved\"\n";
    prompt += "}";

    std::string response = bridge_->query_llm_raw(prompt,
        "You generate world events for a post-apocalyptic survival game. Output ONLY valid JSON.");
    log_event("LLM Response: " + response);

    WorldEvent event;
    event.id = "event_" + std::to_string(std::time(nullptr));
    event.is_active = true;
    
    try {
        auto j = nlohmann::json::parse(response);
        event.type = j.value("type", "discovery");
        event.description = j.value("description", "Something happened in the wasteland.");
        event.severity = j.value("severity", 5);
        event.player_hook = j.value("player_hook", "Investigate to learn more.");
        
        if (j.contains("affected_npcs") && j["affected_npcs"].is_array()) {
            for (const auto& npc : j["affected_npcs"]) {
                event.affected_npcs.push_back(npc.get<std::string>());
            }
        }
        
        if (j.contains("affected_factions") && j["affected_factions"].is_array()) {
            for (const auto& fac : j["affected_factions"]) {
                event.affected_factions.push_back(fac.get<std::string>());
            }
        }
        
        world_events_.push_back(event);
        add_storyline_event("WORLD EVENT: " + event.description);

        // Display to player based on severity
        if (event.severity >= 7) {
            add_msg(m_warning, _(">>> MAJOR WORLD EVENT: %s <<<"), event.description);
        } else if (event.severity >= 4) {
            add_msg(m_info, _("WORLD EVENT: %s"), event.description);
        } else {
            add_msg(m_neutral, _("You hear rumors: %s"), event.description);
        }

        // Show player opportunity
        if (!event.player_hook.empty()) {
            add_msg(m_good, _("[OPPORTUNITY]: %s"), event.player_hook);
        }

        // Show affected factions
        for (const auto& fac : event.affected_factions) {
            add_msg(m_neutral, _("This affects the %s faction."), fac);
        }

    } catch (...) {
        event.type = "discovery";
        event.description = "Rumors spread through the wasteland.";
        event.severity = 3;
        add_msg(m_neutral, _("You hear distant rumors about something happening..."));
    }

    log_event("Generated event: " + event.type + " - " + event.description);

    // Generate a quest from this world event
    auto& quest_system = AIQuestSystem::instance();
    if (!quest_system.get_available_quests().empty() || one_in(2)) {
        // Generate quest from event
        auto quest = quest_system.generate_quest_from_event(
            event.description,
            event.type,
            event.affected_factions
        );

        if (!quest.title.empty()) {
            quest_system.add_quest(quest);

            // Display quest offer to player
            add_msg(m_good, _("=== NEW QUEST AVAILABLE ==="));
            add_msg(m_info, _("Quest: %s"), quest.title);
            add_msg(m_neutral, _("Objective: %s"), quest.objective_text);
            add_msg(m_neutral, _("Difficulty: %d/10 | Reward: $%d + %d reputation"),
                    quest.difficulty, quest.reward.cash_value, quest.reward.faction_reputation);

            // Try to delegate to NPCs if quest allows
            if (quest.target_type != AIQuestTarget::PLAYER_ONLY) {
                std::vector<npc*> nearby_npcs;
                for (npc &guy : g->all_npcs()) {
                    if (!guy.is_dead() && !guy.is_hallucination()) {
                        nearby_npcs.push_back(&guy);
                    }
                }

                if (!nearby_npcs.empty() && one_in(3)) {
                    // Random NPC considers the quest
                    npc* candidate = nearby_npcs[rng(0, nearby_npcs.size() - 1)];
                    add_msg(m_info, _("%s is considering this quest..."), candidate->get_name());

                    if (quest_system.delegate_quest_to_npc(quest, *candidate)) {
                        add_msg(m_good, _("%s accepted the quest!"), candidate->get_name());
                    }
                }
            }
        }
    }

    // NPC-to-NPC quest delegation
    process_npc_quest_offers();

    log_event("=== WORLD EVENT END ===");
}

void AIWorldSystem::process_npc_quest_offers() {
    // Get all NPCs
    std::vector<npc*> npcs;
    for (npc &guy : g->all_npcs()) {
        if (!guy.is_dead() && !guy.is_hallucination()) {
            npcs.push_back(&guy);
        }
    }

    if (npcs.size() < 2) return;

    // Random chance for NPC-to-NPC quest creation
    if (!one_in(3)) return;

    // Select quest giver and potential receiver
    npc* giver = npcs[rng(0, npcs.size() - 1)];
    npc* receiver = npcs[rng(0, npcs.size() - 1)];

    // Don't self-assign
    if (giver == receiver) return;

    std::string giver_faction = giver->get_faction() ?
                                 giver->get_faction()->get_name() : "Unknown";

    // Generate NPC-to-NPC quest
    if (bridge_) {
        std::string prompt = "Generate a quest that one NPC gives to another NPC.\n\n";
        prompt += "QUEST GIVER: " + giver->get_name() + " (" + giver_faction + ")\n";
        prompt += "POTENTIAL HELPER: " + receiver->get_name() + "\n\n";
        prompt += "Generate a small task one survivor might ask another for. Output ONLY valid JSON:\n";
        prompt += "{\n";
        prompt += "  \"title\": \"Quest title (3-5 words)\",\n";
        prompt += "  \"description\": \"What needs to be done\",\n";
        prompt += "  \"dialogue_offer\": \"What the giver says when offering\",\n";
        prompt += "  \"dialogue_accept\": \"What the receiver says if accepting\",\n";
        prompt += "  \"dialogue_reject\": \"What the receiver says if rejecting\",\n";
        prompt += "  \"reward\": \"What the giver offers\",\n";
        prompt += "  \"difficulty\": 1-10,\n";
        prompt += "  \"accept_chance\": 1-100\n";
        prompt += "}";

        std::string response = bridge_->query_llm_raw(prompt,
            "You generate NPC interactions in a post-apocalyptic game. Output ONLY valid JSON.");

        log_event("NPC-to-NPC quest response: " + response);

        try {
            auto j = nlohmann::json::parse(response);

            std::string title = j.value("title", "Help Needed");
            std::string desc = j.value("description", "A task");
            std::string offer_dialogue = j.value("dialogue_offer", "Can you help me?");
            std::string accept_dialogue = j.value("dialogue_accept", "Sure, I'll help.");
            std::string reject_dialogue = j.value("dialogue_reject", "Sorry, can't right now.");
            int accept_chance = j.value("accept_chance", 50);

            // Display the interaction
            add_msg(m_info, _("You notice %s approaching %s..."),
                    giver->get_name(), receiver->get_name());
            add_msg(m_neutral, _("%s: \"%s\""), giver->get_name(), offer_dialogue);

            // Decide if receiver accepts
            if (rng(1, 100) <= accept_chance) {
                add_msg(m_neutral, _("%s: \"%s\""), receiver->get_name(), accept_dialogue);
                add_msg(m_good, _("%s accepted %s's quest: %s"),
                        receiver->get_name(), giver->get_name(), title);

                // Create and track the quest
                auto& quest_system = AIQuestSystem::instance();
                AIQuest npc_quest;
                npc_quest.id = "npc_quest_" + std::to_string(time(nullptr));
                npc_quest.title = title;
                npc_quest.description = desc;
                npc_quest.giver_name = giver->get_name();
                npc_quest.giver_faction = giver_faction;
                npc_quest.target_type = AIQuestTarget::NPC_ONLY;
                npc_quest.status = AIQuestStatus::ACTIVE;
                npc_quest.accepted_by = receiver->get_name();
                npc_quest.accepted_by_player = false;
                npc_quest.difficulty = j.value("difficulty", 3);

                quest_system.add_quest(npc_quest);
                add_storyline_event("NPC Quest: " + giver->get_name() + " asked " +
                                    receiver->get_name() + " to " + desc);
            } else {
                add_msg(m_neutral, _("%s: \"%s\""), receiver->get_name(), reject_dialogue);
                add_msg(m_info, _("%s declined %s's request."),
                        receiver->get_name(), giver->get_name());
            }

        } catch (const nlohmann::json::exception& e) {
            log_event("NPC quest JSON error: " + std::string(e.what()));
        }
    }
}

void AIWorldSystem::process_world_events() {
    // Process ongoing events - could trigger NPC reactions, etc.
    for (auto& event : world_events_) {
        if (event.is_active) {
            // Events could have time-based progression
        }
    }
}

std::vector<WorldEvent> AIWorldSystem::get_active_events() const {
    std::vector<WorldEvent> active;
    for (const auto& event : world_events_) {
        if (event.is_active) {
            active.push_back(event);
        }
    }
    return active;
}

std::string AIWorldSystem::get_current_storyline_summary() {
    std::stringstream summary;
    summary << "=== STORYLINE SUMMARY ===\n\n";
    
    summary << "Recent Events:\n";
    int count = 0;
    for (auto it = storyline_events_.rbegin(); it != storyline_events_.rend() && count < 10; ++it, ++count) {
        summary << "  " << (count + 1) << ". " << *it << "\n";
    }
    
    summary << "\nActive Quests:\n";
    for (const auto& quest : active_quests_) {
        if (quest.is_active && !quest.is_completed) {
            summary << "  - " << quest.title << " (from " << quest.giver_npc << ")\n";
            summary << "    " << quest.description << "\n";
        }
    }
    
    summary << "\nWorld Events:\n";
    for (const auto& event : world_events_) {
        if (event.is_active) {
            summary << "  - [" << event.type << "] " << event.description << "\n";
            summary << "    Player hook: " << event.player_hook << "\n";
        }
    }
    
    return summary.str();
}

void AIWorldSystem::add_storyline_event(const std::string& event) {
    storyline_events_.push_back(event);
    log_event("STORYLINE: " + event);
    
    // Store in Qdrant for long-term memory
    if (bridge_) {
        bridge_->store_memory(event, "world_storyline");
    }
}

} // namespace cata_ai
