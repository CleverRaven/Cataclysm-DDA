#include "ai_quest_system.h"
#include "llm_bridge.h"
#include "game.h"
#include "npc.h"
#include "messages.h"
#include "output.h"
#include "input_context.h"
#include "ui_manager.h"
#include "string_formatter.h"
#include "rng.h"
#include "faction.h"
#include "item.h"
#include "avatar.h"
#include "map.h"
#include "itype.h"

#include "third-party/nlohmann/json.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>

using json = nlohmann::json;

namespace cata_ai {

AIQuestSystem& AIQuestSystem::instance() {
    static AIQuestSystem inst;
    return inst;
}

void AIQuestSystem::initialize(std::shared_ptr<LLMBridge> bridge) {
    bridge_ = bridge;
    log_event("=== AI QUEST SYSTEM INITIALIZED ===");
}

void AIQuestSystem::log_event(const std::string& message) {
    std::ofstream log("ai_quest_system.log", std::ios::app);
    if (log.is_open()) {
        time_t now = time(nullptr);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", localtime(&now));
        log << timestamp << " " << message << std::endl;
    }
}

std::string AIQuestSystem::generate_quest_id() {
    return "AIQ_" + std::to_string(++quest_counter_) + "_" + std::to_string(time(nullptr));
}

AIQuest AIQuestSystem::generate_quest_from_event(const std::string& event_description,
                                                   const std::string& event_type,
                                                   const std::vector<std::string>& affected_factions) {
    AIQuest quest;
    quest.id = generate_quest_id();

    if (!bridge_) {
        quest.title = "Investigate the Disturbance";
        quest.description = event_description;
        quest.objective.type = QuestObjectiveType::INVESTIGATE;
        quest.objective.description = "Investigate the area and report back.";
        quest.objective.target = "disturbance_location";
        quest.objective.target_count = 1;
        quest.objective_text = quest.objective.description;
        quest.giver_name = "Unknown Survivor";
        quest.difficulty = 5;
        quest.requires_return = false;
        quest.reward.cash_value = 100;
        quest.reward.auto_reward = true;
        return quest;
    }

    std::string factions_str;
    for (const auto& f : affected_factions) {
        if (!factions_str.empty()) factions_str += ", ";
        factions_str += f;
    }

    std::string prompt = "Generate a quest based on this world event. Output ONLY valid JSON.\n\n";
    prompt += "WORLD EVENT: " + event_description + "\n";
    prompt += "EVENT TYPE: " + event_type + "\n";
    prompt += "FACTIONS INVOLVED: " + factions_str + "\n\n";
    prompt += "Generate a quest with this exact JSON structure:\n";
    prompt += "{\n";
    prompt += "  \"title\": \"Short quest title (3-6 words)\",\n";
    prompt += "  \"description\": \"Full quest description\",\n";
    prompt += "  \"objective_type\": \"FIND_ITEM or FIND_LOCATION or KILL_MONSTER or TALK_TO_NPC or INVESTIGATE\",\n";
    prompt += "  \"objective_target\": \"specific item/monster/location/npc name\",\n";
    prompt += "  \"objective_count\": 1,\n";
    prompt += "  \"objective_text\": \"Human readable objective description\",\n";
    prompt += "  \"location_hint\": \"Where to go or look\",\n";
    prompt += "  \"giver_name\": \"NPC name who gives quest\",\n";
    prompt += "  \"giver_faction\": \"Which faction is offering\",\n";
    prompt += "  \"difficulty\": 5,\n";
    prompt += "  \"requires_return\": true,\n";
    prompt += "  \"auto_reward\": false,\n";
    prompt += "  \"reward_cash\": 100,\n";
    prompt += "  \"reward_reputation\": 10,\n";
    prompt += "  \"reward_item\": \"bandages\",\n";
    prompt += "  \"reward_item_count\": 1,\n";
    prompt += "  \"target_type\": \"ANYONE\"\n";
    prompt += "}";

    std::string response = bridge_->query_llm_raw(prompt,
        "You generate quests for a post-apocalyptic survival game. Output ONLY valid JSON.");

    log_event("Quest generation response: " + response);

    try {
        auto j = json::parse(response);

        quest.title = j.value("title", "Unknown Quest");
        quest.description = j.value("description", event_description);
        quest.location_hint = j.value("location_hint", "Unknown location");
        quest.giver_name = j.value("giver_name", "Unknown Survivor");
        quest.giver_faction = j.value("giver_faction", "Unknown");
        quest.difficulty = j.value("difficulty", 5);
        quest.requires_return = j.value("requires_return", true);
        quest.target_type = AIQuestTarget::ANYONE;

        // Parse structured objective
        quest.objective.description = j.value("objective_text", "Complete the objective");
        quest.objective_text = quest.objective.description;
        quest.objective.target = j.value("objective_target", "");
        quest.objective.target_count = j.value("objective_count", 1);
        quest.objective.current_count = 0;
        quest.objective.completed = false;

        std::string obj_type = j.value("objective_type", "INVESTIGATE");
        if (obj_type == "FIND_ITEM") quest.objective.type = QuestObjectiveType::FIND_ITEM;
        else if (obj_type == "FIND_LOCATION") quest.objective.type = QuestObjectiveType::FIND_LOCATION;
        else if (obj_type == "KILL_MONSTER") quest.objective.type = QuestObjectiveType::KILL_MONSTER;
        else if (obj_type == "TALK_TO_NPC") quest.objective.type = QuestObjectiveType::TALK_TO_NPC;
        else if (obj_type == "DELIVER_ITEM") quest.objective.type = QuestObjectiveType::DELIVER_ITEM;
        else if (obj_type == "INVESTIGATE") quest.objective.type = QuestObjectiveType::INVESTIGATE;
        else quest.objective.type = QuestObjectiveType::CUSTOM;

        std::string target_str = j.value("target_type", "ANYONE");
        if (target_str == "PLAYER_ONLY") quest.target_type = AIQuestTarget::PLAYER_ONLY;
        else if (target_str == "NPC_ONLY") quest.target_type = AIQuestTarget::NPC_ONLY;

        // Parse rewards
        quest.reward.cash_value = j.value("reward_cash", 100);
        quest.reward.faction_reputation = j.value("reward_reputation", 10);
        quest.reward.item_id = j.value("reward_item", "");
        quest.reward.item_count = j.value("reward_item_count", 1);
        quest.reward.auto_reward = j.value("auto_reward", false);

        if (j.contains("faction_impacts")) {
            for (auto& [key, val] : j["faction_impacts"].items()) {
                quest.faction_impacts[key] = val.get<int>();
            }
        }

        quest.related_event_id = event_type;

    } catch (const json::exception& e) {
        log_event("Quest JSON parse error: " + std::string(e.what()));
        quest.title = "Investigate: " + event_type;
        quest.description = event_description;
        quest.objective.type = QuestObjectiveType::INVESTIGATE;
        quest.objective.description = "Investigate the area and report back.";
        quest.objective.target = "unknown_location";
        quest.objective.target_count = 1;
        quest.objective_text = quest.objective.description;
        quest.giver_name = "Unknown Survivor";
        quest.difficulty = 5;
        quest.requires_return = false;
        quest.reward.cash_value = 100;
        quest.reward.auto_reward = true;
    }

    log_event("Generated quest: " + quest.title);
    return quest;
}

NPCApproach AIQuestSystem::generate_npc_approach(npc& approaching_npc,
                                                   const std::string& world_context) {
    NPCApproach approach;
    approach.npc_name = approaching_npc.get_name();
    approach.faction = approaching_npc.get_faction() ?
                       approaching_npc.get_faction()->get_name() : "Unknown";

    if (!bridge_) {
        approach.intent = NPCIntent::OFFER_QUEST;
        approach.dialogue = "Hey, I need your help with something.";
        approach.urgency = 5;
        return approach;
    }

    std::string prompt = "An NPC wants to approach the player. Generate their intent and dialogue.\n\n";
    prompt += "NPC NAME: " + approach.npc_name + "\n";
    prompt += "NPC FACTION: " + approach.faction + "\n";
    prompt += "WORLD SITUATION: " + world_context + "\n\n";
    prompt += "Output ONLY valid JSON:\n";
    prompt += "{\n";
    prompt += "  \"intent\": \"OFFER_QUEST or REQUEST_HELP or WARN_DANGER or TRADE_OPPORTUNITY\",\n";
    prompt += "  \"dialogue\": \"What the NPC says when approaching\",\n";
    prompt += "  \"urgency\": 5,\n";
    prompt += "  \"quest_title\": \"Quest title if offering\",\n";
    prompt += "  \"quest_description\": \"Quest description if offering\",\n";
    prompt += "  \"quest_objective\": \"Quest objective if offering\",\n";
    prompt += "  \"quest_reward_cash\": 100,\n";
    prompt += "  \"quest_reward_reputation\": 10\n";
    prompt += "}";

    std::string response = bridge_->query_llm_raw(prompt,
        "You roleplay NPCs in a post-apocalyptic game. Output ONLY valid JSON.");

    log_event("NPC approach response: " + response);

    try {
        auto j = json::parse(response);

        std::string intent_str = j.value("intent", "OFFER_QUEST");
        if (intent_str == "OFFER_QUEST") approach.intent = NPCIntent::OFFER_QUEST;
        else if (intent_str == "REQUEST_HELP") approach.intent = NPCIntent::REQUEST_HELP;
        else if (intent_str == "WARN_DANGER") approach.intent = NPCIntent::WARN_DANGER;
        else if (intent_str == "TRADE_OPPORTUNITY") approach.intent = NPCIntent::TRADE_OPPORTUNITY;
        else if (intent_str == "FACTION_MESSAGE") approach.intent = NPCIntent::FACTION_MESSAGE;
        else if (intent_str == "SEEK_ALLIANCE") approach.intent = NPCIntent::SEEK_ALLIANCE;
        else if (intent_str == "BETRAYAL") approach.intent = NPCIntent::BETRAYAL;

        approach.dialogue = j.value("dialogue", "I need to talk to you.");
        approach.urgency = j.value("urgency", 5);

        // If offering quest, create one
        if (approach.intent == NPCIntent::OFFER_QUEST ||
            approach.intent == NPCIntent::REQUEST_HELP) {
            AIQuest quest;
            quest.id = generate_quest_id();
            quest.title = j.value("quest_title", "Help " + approach.npc_name);
            quest.description = j.value("quest_description", "Help this NPC.");
            quest.objective.description = j.value("quest_objective", "Complete the task.");
            quest.objective.type = QuestObjectiveType::CUSTOM;
            quest.objective_text = quest.objective.description;
            quest.giver_name = approach.npc_name;
            quest.giver_faction = approach.faction;
            quest.difficulty = approach.urgency;
            quest.reward.cash_value = j.value("quest_reward_cash", 100);
            quest.reward.faction_reputation = j.value("quest_reward_reputation", 10);
            quest.target_type = AIQuestTarget::PLAYER_ONLY;
            approach.quest = quest;
        }

    } catch (const json::exception& e) {
        log_event("NPC approach JSON error: " + std::string(e.what()));
        approach.intent = NPCIntent::OFFER_QUEST;
        approach.dialogue = "Survivor! I could use your help.";
        approach.urgency = 5;
    }

    return approach;
}

bool AIQuestSystem::delegate_quest_to_npc(AIQuest& quest, npc& target_npc) {
    if (quest.target_type == AIQuestTarget::PLAYER_ONLY) {
        return false;
    }

    if (!bridge_) {
        // Random chance without AI
        if (one_in(2)) {
            quest.delegated_to = target_npc.get_name();
            quest.delegation_reason = "I'll handle it.";
            quest.status = AIQuestStatus::ACTIVE;
            quest.accepted_by = target_npc.get_name();
            quest.accepted_by_player = false;
            return true;
        }
        return false;
    }

    std::string npc_faction = target_npc.get_faction() ?
                              target_npc.get_faction()->get_name() : "None";

    std::string prompt = "An NPC is considering whether to accept a quest from another NPC.\n\n";
    prompt += "QUEST: " + quest.title + "\n";
    prompt += "DESCRIPTION: " + quest.description + "\n";
    prompt += "DIFFICULTY: " + std::to_string(quest.difficulty) + "\n";
    prompt += "REWARD: " + std::to_string(quest.reward.cash_value) + " cash, ";
    prompt += std::to_string(quest.reward.faction_reputation) + " reputation\n";
    prompt += "QUEST GIVER FACTION: " + quest.giver_faction + "\n\n";
    prompt += "NPC CONSIDERING: " + target_npc.get_name() + "\n";
    prompt += "NPC FACTION: " + npc_faction + "\n\n";
    prompt += "Should this NPC accept the quest? Consider faction relationships, difficulty, and reward.\n\n";
    prompt += "Output ONLY valid JSON:\n";
    prompt += "{\n";
    prompt += "  \"accepts\": true,\n";
    prompt += "  \"reason\": \"Why the NPC accepts or rejects\",\n";
    prompt += "  \"negotiation\": \"Any counter-offer or condition\"\n";
    prompt += "}";

    std::string response = bridge_->query_llm_raw(prompt,
        "You simulate NPC decision-making in a post-apocalyptic game. Output ONLY valid JSON.");

    log_event("NPC delegation response for " + target_npc.get_name() + ": " + response);

    try {
        auto j = json::parse(response);

        bool accepts = j.value("accepts", false);
        std::string reason = j.value("reason", "I'll think about it.");

        if (accepts) {
            quest.delegated_to = target_npc.get_name();
            quest.delegation_reason = reason;
            quest.status = AIQuestStatus::ACTIVE;
            quest.accepted_by = target_npc.get_name();
            quest.accepted_by_player = false;
            quest.accepted_time = calendar::turn;

            // Display to player
            add_msg(m_info, _("%s accepted a quest: %s"),
                    target_npc.get_name(), quest.title);
            add_msg(m_neutral, _("%s: \"%s\""),
                    target_npc.get_name(), reason);

            log_event("Quest delegated to NPC: " + target_npc.get_name() + " - " + quest.title);
            return true;
        } else {
            add_msg(m_info, _("%s declined the quest: %s"),
                    target_npc.get_name(), quest.title);
            add_msg(m_neutral, _("%s: \"%s\""),
                    target_npc.get_name(), reason);
        }

    } catch (const json::exception& e) {
        log_event("Delegation JSON error: " + std::string(e.what()));
    }

    return false;
}

void AIQuestSystem::add_quest(const AIQuest& quest) {
    quests_.push_back(quest);
    log_event("Quest added: " + quest.title + " (ID: " + quest.id + ")");
}

std::vector<AIQuest> AIQuestSystem::get_available_quests() const {
    std::vector<AIQuest> available;
    for (const auto& q : quests_) {
        if (q.status == AIQuestStatus::AVAILABLE) {
            available.push_back(q);
        }
    }
    return available;
}

std::vector<AIQuest> AIQuestSystem::get_active_quests() const {
    std::vector<AIQuest> active;
    for (const auto& q : quests_) {
        if (q.status == AIQuestStatus::ACTIVE) {
            active.push_back(q);
        }
    }
    return active;
}

std::optional<AIQuest> AIQuestSystem::get_quest_by_id(const std::string& id) const {
    for (const auto& q : quests_) {
        if (q.id == id) {
            return q;
        }
    }
    return std::nullopt;
}

bool AIQuestSystem::accept_quest(const std::string& quest_id, bool by_player,
                                  const std::string& acceptor_name) {
    for (auto& q : quests_) {
        if (q.id == quest_id && q.status == AIQuestStatus::AVAILABLE) {
            q.status = AIQuestStatus::ACTIVE;
            q.accepted_by_player = by_player;
            q.accepted_by = by_player ? "Player" : acceptor_name;
            q.accepted_time = calendar::turn;

            if (by_player) {
                add_msg(m_good, _("Quest accepted: %s"), q.title);
                add_msg(m_neutral, _("Objective: %s"), q.objective.description);
                if (!q.location_hint.empty()) {
                    add_msg(m_info, _("Location: %s"), q.location_hint);
                }
                if (q.requires_return) {
                    add_msg(m_info, _("Return to %s when complete."), q.giver_name);
                }
            }

            log_event("Quest accepted: " + q.title + " by " + q.accepted_by);
            return true;
        }
    }
    return false;
}

bool AIQuestSystem::complete_quest(const std::string& quest_id) {
    for (auto& q : quests_) {
        if (q.id == quest_id && q.status == AIQuestStatus::ACTIVE) {
            q.status = AIQuestStatus::COMPLETED;

            add_msg(m_good, _("Quest completed: %s"), q.title);

            // Apply rewards
            apply_quest_rewards(q);

            // Apply faction impacts
            for (const auto& [faction, change] : q.faction_impacts) {
                modify_faction_reputation(faction, change);
                if (change > 0) {
                    add_msg(m_good, _("Your reputation with %s increased by %d"),
                            faction, change);
                } else if (change < 0) {
                    add_msg(m_bad, _("Your reputation with %s decreased by %d"),
                            faction, -change);
                }
            }

            log_event("Quest completed: " + q.title);
            return true;
        }
    }
    return false;
}

bool AIQuestSystem::fail_quest(const std::string& quest_id) {
    for (auto& q : quests_) {
        if (q.id == quest_id && q.status == AIQuestStatus::ACTIVE) {
            q.status = AIQuestStatus::FAILED;

            add_msg(m_bad, _("Quest failed: %s"), q.title);

            // Negative faction impact for failure
            if (!q.giver_faction.empty()) {
                modify_faction_reputation(q.giver_faction, -5);
                add_msg(m_bad, _("Your reputation with %s decreased."), q.giver_faction);
            }

            log_event("Quest failed: " + q.title);
            return true;
        }
    }
    return false;
}

void AIQuestSystem::apply_quest_rewards(const AIQuest& quest) {
    if (!quest.accepted_by_player) {
        // NPC completed the quest - different handling
        add_msg(m_info, _("%s completed the quest and earned rewards."), quest.accepted_by);
        return;
    }

    add_msg(m_good, _("=== QUEST REWARDS ==="));

    // Cash reward - spawn actual cash items
    if (quest.reward.cash_value > 0) {
        // Spawn cash as "cash_card" items with appropriate value
        // For now, display the reward (full implementation would add currency)
        add_msg(m_good, _("Received: $%d"), quest.reward.cash_value);
        // Could spawn: spawn_item_reward("cash_card", 1); with modified value
    }

    // Reputation reward
    if (quest.reward.faction_reputation > 0 && !quest.giver_faction.empty()) {
        modify_faction_reputation(quest.giver_faction, quest.reward.faction_reputation);
        add_msg(m_good, _("Reputation with %s increased by %d"),
                quest.giver_faction, quest.reward.faction_reputation);
    }

    // Item reward - actually spawn the items
    if (!quest.reward.item_id.empty()) {
        spawn_item_reward(quest.reward.item_id, quest.reward.item_count);
    }

    log_event("Rewards applied for quest: " + quest.title);
}

void AIQuestSystem::modify_faction_reputation(const std::string& faction_name, int change) {
    faction_reputations_[faction_name] += change;

    log_event("Faction reputation changed: " + faction_name + " " +
              (change >= 0 ? "+" : "") + std::to_string(change) +
              " (now: " + std::to_string(faction_reputations_[faction_name]) + ")");
}

int AIQuestSystem::get_faction_reputation(const std::string& faction_name) const {
    auto it = faction_reputations_.find(faction_name);
    if (it != faction_reputations_.end()) {
        return it->second;
    }
    return 0;
}

std::string AIQuestSystem::get_faction_standing(const std::string& faction_name) const {
    int rep = get_faction_reputation(faction_name);
    if (rep >= 100) return "Allied";
    if (rep >= 50) return "Friendly";
    if (rep >= 20) return "Favorable";
    if (rep >= 0) return "Neutral";
    if (rep >= -20) return "Unfavorable";
    if (rep >= -50) return "Hostile";
    return "Hated";
}

std::vector<NPCApproach> AIQuestSystem::get_pending_approaches() const {
    return pending_approaches_;
}

void AIQuestSystem::add_npc_approach(const NPCApproach& approach) {
    pending_approaches_.push_back(approach);
    log_event("NPC approach queued: " + approach.npc_name + " with intent " +
              std::to_string(static_cast<int>(approach.intent)));
}

void AIQuestSystem::clear_approach(const std::string& npc_name) {
    pending_approaches_.erase(
        std::remove_if(pending_approaches_.begin(), pending_approaches_.end(),
                       [&npc_name](const NPCApproach& a) {
                           return a.npc_name == npc_name;
                       }),
        pending_approaches_.end());
}

void AIQuestSystem::on_turn_update(int current_turn) {
    // Check quest expirations
    check_quest_expirations();

    // Process NPC quest AI - actively pursuing quests (every 100 turns)
    static int last_ai_turn = 0;
    if (current_turn - last_ai_turn >= 100) {
        process_npc_quest_ai();
        last_ai_turn = current_turn;
    }

    // Process NPC quest delegation (every 200 turns)
    if (current_turn - last_quest_gen_turn_ >= 200) {
        process_npc_quest_progress();
        last_quest_gen_turn_ = current_turn;
    }
}

void AIQuestSystem::check_quest_expirations() {
    for (auto& q : quests_) {
        if (q.status == AIQuestStatus::ACTIVE &&
            q.deadline != time_point() &&
            calendar::turn > q.deadline) {
            q.status = AIQuestStatus::EXPIRED;
            add_msg(m_bad, _("Quest expired: %s"), q.title);
            log_event("Quest expired: " + q.title);
        }
    }
}

void AIQuestSystem::process_npc_quest_progress() {
    // Simulate NPC progress on their quests
    for (auto& q : quests_) {
        if (q.status == AIQuestStatus::ACTIVE && !q.accepted_by_player) {
            // NPC is working on this quest
            // Random chance of completion based on difficulty
            int completion_chance = 100 - (q.difficulty * 8);
            if (rng(1, 100) <= completion_chance) {
                q.status = AIQuestStatus::COMPLETED;
                add_msg(m_info, _("%s completed their quest: %s"),
                        q.accepted_by, q.title);
                log_event("NPC completed quest: " + q.accepted_by + " - " + q.title);

                // Faction relationship changes for NPC completion
                for (const auto& [faction, change] : q.faction_impacts) {
                    // Reduced impact since player didn't do it
                    modify_faction_reputation(faction, change / 2);
                }
            }
        }
    }
}

void AIQuestSystem::display_quest_offer(const AIQuest& quest) {
    std::string title = "Quest Offer: " + quest.title;

    std::stringstream ss;
    ss << quest.description << "\n\n";
    ss << "Objective: " << quest.objective_text << "\n";
    if (!quest.location_hint.empty()) {
        ss << "Location: " << quest.location_hint << "\n";
    }
    ss << "\nDifficulty: " << quest.difficulty << "/10\n";
    ss << "From: " << quest.giver_faction << "\n\n";
    ss << "Rewards:\n";
    if (quest.reward.cash_value > 0) {
        ss << "  - $" << quest.reward.cash_value << "\n";
    }
    if (quest.reward.faction_reputation > 0) {
        ss << "  - +" << quest.reward.faction_reputation << " reputation with "
           << quest.giver_faction << "\n";
    }
    if (!quest.reward.item_id.empty()) {
        ss << "  - " << quest.reward.item_id << " x" << quest.reward.item_count << "\n";
    }

    // Display and get player choice
    add_msg(m_info, _("=== QUEST OFFER: %s ==="), quest.title);
    add_msg(m_neutral, "%s", quest.description);
    add_msg(m_neutral, _("Objective: %s"), quest.objective_text);
    add_msg(m_neutral, _("Reward: $%d, +%d %s reputation"),
            quest.reward.cash_value, quest.reward.faction_reputation, quest.giver_faction);
    add_msg(m_info, _("[Press 'Y' to accept, 'N' to decline]"));
}

void AIQuestSystem::display_quest_log() {
    add_msg(m_info, _("=== AI QUEST LOG ==="));

    auto active = get_active_quests();
    if (active.empty()) {
        add_msg(m_neutral, _("No active quests."));
    } else {
        add_msg(m_good, _("Active Quests:"));
        for (const auto& q : active) {
            std::string owner = q.accepted_by_player ? "You" : q.accepted_by;
            std::string progress = "";
            if (q.objective.type != QuestObjectiveType::CUSTOM && q.objective.target_count > 1) {
                progress = " [" + std::to_string(q.objective.current_count) + "/" +
                           std::to_string(q.objective.target_count) + "]";
            }
            if (q.objective.completed && q.requires_return) {
                progress = " [COMPLETE - Return to " + q.giver_name + "]";
            }
            add_msg(m_neutral, _("  - %s (%s) - %s%s"), q.title, owner,
                    q.objective.description, progress);
        }
    }

    auto available = get_available_quests();
    if (!available.empty()) {
        add_msg(m_info, _("Available Quests:"));
        for (const auto& q : available) {
            add_msg(m_neutral, _("  - %s (Difficulty: %d)"), q.title, q.difficulty);
        }
    }

    // Faction standings
    add_msg(m_info, _("Faction Standings:"));
    for (const auto& [faction, rep] : faction_reputations_) {
        add_msg(m_neutral, _("  - %s: %s (%d)"), faction, get_faction_standing(faction), rep);
    }
}

// ============ QUEST OBJECTIVE TRACKING ============

void AIQuestSystem::check_item_pickup(const std::string& item_id) {
    for (auto& q : quests_) {
        if (q.status == AIQuestStatus::ACTIVE && q.accepted_by_player &&
            q.objective.type == QuestObjectiveType::FIND_ITEM &&
            q.objective.target == item_id && !q.objective.completed) {

            q.objective.current_count++;
            add_msg(m_good, _("Quest progress: %s - Found %s (%d/%d)"),
                    q.title, item_id, q.objective.current_count, q.objective.target_count);

            if (q.objective.current_count >= q.objective.target_count) {
                q.objective.completed = true;
                add_msg(m_good, _("Quest objective complete: %s"), q.objective.description);

                if (q.reward.auto_reward || !q.requires_return) {
                    // Auto-complete and give reward
                    complete_quest(q.id);
                } else {
                    add_msg(m_info, _("Return to %s to claim your reward."), q.giver_name);
                }
            }
            log_event("Quest item progress: " + q.title + " - " + item_id);
        }
    }
}

void AIQuestSystem::check_monster_kill(const std::string& monster_type) {
    for (auto& q : quests_) {
        if (q.status == AIQuestStatus::ACTIVE && q.accepted_by_player &&
            q.objective.type == QuestObjectiveType::KILL_MONSTER &&
            q.objective.target == monster_type && !q.objective.completed) {

            q.objective.current_count++;
            add_msg(m_good, _("Quest progress: %s - Killed %s (%d/%d)"),
                    q.title, monster_type, q.objective.current_count, q.objective.target_count);

            if (q.objective.current_count >= q.objective.target_count) {
                q.objective.completed = true;
                add_msg(m_good, _("Quest objective complete: %s"), q.objective.description);

                if (q.reward.auto_reward || !q.requires_return) {
                    complete_quest(q.id);
                } else {
                    add_msg(m_info, _("Return to %s to claim your reward."), q.giver_name);
                }
            }
            log_event("Quest kill progress: " + q.title + " - " + monster_type);
        }
    }
}

void AIQuestSystem::check_location_reached(const std::string& location) {
    for (auto& q : quests_) {
        if (q.status == AIQuestStatus::ACTIVE && q.accepted_by_player &&
            (q.objective.type == QuestObjectiveType::FIND_LOCATION ||
             q.objective.type == QuestObjectiveType::INVESTIGATE) &&
            q.objective.target == location && !q.objective.completed) {

            q.objective.completed = true;
            add_msg(m_good, _("Quest objective complete: Reached %s"), location);

            if (q.objective.type == QuestObjectiveType::INVESTIGATE) {
                // Investigation quests auto-complete on arrival
                add_msg(m_good, _("You investigate the area and discover useful information."));
                complete_quest(q.id);
            } else if (q.reward.auto_reward || !q.requires_return) {
                complete_quest(q.id);
            } else {
                add_msg(m_info, _("Return to %s to report your findings."), q.giver_name);
            }
            log_event("Quest location reached: " + q.title + " - " + location);
        }
    }
}

void AIQuestSystem::check_npc_talked(const std::string& npc_name) {
    for (auto& q : quests_) {
        if (q.status == AIQuestStatus::ACTIVE && q.accepted_by_player) {
            // Check if this is the target NPC for a TALK_TO quest
            if (q.objective.type == QuestObjectiveType::TALK_TO_NPC &&
                q.objective.target == npc_name && !q.objective.completed) {

                q.objective.completed = true;
                add_msg(m_good, _("Quest objective complete: Talked to %s"), npc_name);
                complete_quest(q.id);
                log_event("Quest talk complete: " + q.title + " - " + npc_name);
            }
            // Check if returning to quest giver
            else if (q.objective.completed && q.requires_return &&
                     q.giver_name == npc_name && !q.reward_given) {
                turn_in_quest(q.id);
            }
        }
    }
}

bool AIQuestSystem::turn_in_quest(const std::string& quest_id) {
    for (auto& q : quests_) {
        if (q.id == quest_id && q.status == AIQuestStatus::ACTIVE &&
            q.objective.completed && !q.reward_given) {

            add_msg(m_good, _("%s: \"Well done! Here's your reward.\""), q.giver_name);
            q.reward_given = true;
            complete_quest(q.id);
            return true;
        }
    }
    return false;
}

bool AIQuestSystem::try_complete_objective(const std::string& quest_id) {
    for (auto& q : quests_) {
        if (q.id == quest_id && q.status == AIQuestStatus::ACTIVE &&
            q.accepted_by_player && !q.objective.completed) {

            // For CUSTOM objectives, allow manual completion
            if (q.objective.type == QuestObjectiveType::CUSTOM) {
                q.objective.completed = true;
                add_msg(m_good, _("Quest objective marked complete: %s"), q.objective.description);

                if (q.reward.auto_reward || !q.requires_return) {
                    complete_quest(q.id);
                } else {
                    add_msg(m_info, _("Return to %s to claim your reward."), q.giver_name);
                }
                return true;
            }
        }
    }
    return false;
}

void AIQuestSystem::spawn_item_reward(const std::string& item_id, int count) {
    if (item_id.empty() || count <= 0) return;

    // Get player's position and spawn items
    avatar& player_char = get_avatar();
    map& here = get_map();

    for (int i = 0; i < count; i++) {
        item reward(itype_id(item_id), calendar::turn);
        if (reward.is_null()) {
            add_msg(m_warning, _("Reward item '%s' not found in game data."), item_id);
            log_event("Failed to spawn reward item: " + item_id);
            return;
        }

        // Try to add to inventory, drop if full
        if (!player_char.can_pickVolume(reward) || !player_char.can_pickWeight(reward, false)) {
            here.add_item_or_charges(player_char.pos_bub(), reward);
            add_msg(m_info, _("Reward dropped at your feet (inventory full)."));
        } else {
            player_char.i_add(reward);
        }
    }

    add_msg(m_good, _("Received: %s x%d"), item_id, count);
    log_event("Spawned reward item: " + item_id + " x" + std::to_string(count));
}

// ============ NPC QUEST AI ============

void AIQuestSystem::process_npc_quest_ai() {
    for (auto& q : quests_) {
        if (q.status != AIQuestStatus::ACTIVE || q.accepted_by_player) {
            continue;
        }

        // NPC is actively pursuing this quest
        // Increase progress based on difficulty (harder = slower)
        int progress_rate = std::max(1, 10 - q.difficulty);
        q.npc_progress += rng(1, progress_rate);

        // Check for completion (100% progress)
        if (q.npc_progress >= 100) {
            q.objective.completed = true;
            q.status = AIQuestStatus::COMPLETED;

            add_msg(m_info, _("%s completed their quest: %s"), q.accepted_by, q.title);

            // Display what they did
            if (bridge_) {
                std::string prompt = "An NPC just completed a quest. Describe briefly what they did.\n\n";
                prompt += "NPC: " + q.accepted_by + "\n";
                prompt += "QUEST: " + q.title + "\n";
                prompt += "OBJECTIVE: " + q.objective.description + "\n\n";
                prompt += "Output a single sentence describing their success (in third person, past tense):";

                std::string response = bridge_->query_llm_raw(prompt,
                    "You narrate NPC actions. Output only the narrative sentence.");

                if (!response.empty() && response.length() < 200) {
                    add_msg(m_neutral, "%s", response);
                } else {
                    add_msg(m_neutral, _("%s successfully completed their task."), q.accepted_by);
                }
            }

            // Apply faction impacts (reduced for NPC completion)
            for (const auto& [faction, change] : q.faction_impacts) {
                modify_faction_reputation(faction, change / 2);
            }

            log_event("NPC AI completed quest: " + q.accepted_by + " - " + q.title);

        } else if (q.npc_progress % 25 == 0 && q.npc_progress > 0) {
            // Periodic progress updates (every 25%)
            add_msg(m_info, _("%s is making progress on their quest: %s (%d%%)"),
                    q.accepted_by, q.title, q.npc_progress);
        }
    }
}

} // namespace cata_ai
