#pragma once
#ifndef CATA_SRC_NPC_DECISION_CATEGORY_H
#define CATA_SRC_NPC_DECISION_CATEGORY_H

#include <string>

// Decision categories for BT/cascade convergence diagnostic.
// Used by the top-level npc_decision behavior tree to compare
// BT goal selection against the legacy npc::move() cascade.
enum class decision_category : int {
    combat, investigate, needs, follow, order, duty, camp_work, camp_travel, free_time, idle, unmodeled
};

const char *category_name( decision_category cat );
decision_category bt_goal_to_category( const std::string &goal );
const char *classify_comparison( decision_category bt, decision_category cascade );

#endif // CATA_SRC_NPC_DECISION_CATEGORY_H
