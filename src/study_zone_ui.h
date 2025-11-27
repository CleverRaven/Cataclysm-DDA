#pragma once
#ifndef CATA_SRC_STUDY_ZONE_UI_H
#define CATA_SRC_STUDY_ZONE_UI_H

#include <map>
#include <set>
#include <string>

#include "cata_imgui.h"
#include "type_id.h"

enum class study_zone_ui_result {
    canceled,
    successful,
    changed,
};

/**
 * UI for selecting skills for study zones per NPC.
 * npc_skill_preferences: map NPC name -> set of skills they should study.
 */
study_zone_ui_result query_study_zone_skills( std::map<std::string, std::set<skill_id>>
        &npc_skill_preferences );

#endif // CATA_SRC_STUDY_ZONE_UI_H
