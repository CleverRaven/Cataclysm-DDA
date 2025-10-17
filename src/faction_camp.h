#pragma once
#ifndef CATA_SRC_FACTION_CAMP_H
#define CATA_SRC_FACTION_CAMP_H

#include <functional>
#include <stdint.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "coords_fwd.h"
#include "translation.h"
#include "type_id.h"

template <typename E> struct enum_traits;

namespace catacurses
{
class window;
} // namespace catacurses
class npc;
struct mission_entry;

namespace base_camps
{
enum tab_mode : int;
} // namespace base_camps

enum class farm_ops : int {
    plow = 1,
    plant = 2,
    harvest = 4
};

template<>
struct enum_traits<farm_ops> {
    static constexpr bool is_flag_enum = true;
};

class JsonObject;

namespace talk_function
{
void basecamp_mission( npc & );

/// Start a faction camp on the current OM tile
void start_camp( npc & );

void draw_camp_tabs( const catacurses::window &win, base_camps::tab_mode cur_tab,
                     const std::vector<std::vector<mission_entry>> &entries );
std::string name_mission_tabs( const tripoint_abs_omt &omt_pos, const std::string &role_id,
                               const std::string &cur_title, base_camps::tab_mode cur_tab );

/// Returns the OM tiles surrounding the camp, @ref purge removes all tiles that aren't expansions
std::vector<std::pair<std::string, tripoint_abs_omt>> om_building_region(
            const tripoint_abs_omt &omt_pos, int range, bool purge = false );
/// Returns the x and y coordinates of ( omt_tar - omt_pos ), clamped to [-1, 1]
point_rel_omt om_simple_dir( const tripoint_abs_omt &omt_pos, const tripoint_abs_omt &omt_tar );
} // namespace talk_function

enum class risk_diff_level {
    NONE = 0,
    VERY_LOW,
    LOW,
    MEDIUM,
    HIGH,
    VERY_HIGH,
    NUM_RISK_DIFF_LEVELS
};

template<>
struct enum_traits<risk_diff_level> {
    static constexpr risk_diff_level last = risk_diff_level::NUM_RISK_DIFF_LEVELS;
};

class faction_mission
{
    public:
        bool was_loaded = false;
        faction_mission_id id;
        translation name;
        translation description;
        skill_id skill_used;
        risk_diff_level difficulty;
        risk_diff_level risk;
        float activity_level;
        translation time;
        uint16_t positions;
        translation items_label;
        std::vector<translation> items_possibilities;
        std::vector<translation> effects;
        translation footer;
        static void load_faction_missions( const JsonObject &jo, const std::string &src );
        static void reset();
        static void check_consistency();
        void load( const JsonObject &jo, const std::string_view &src );
        void check() const;
        std::string describe( int npc_count = 0,
                              const std::function<std::vector<std::string>()> &get_items_possibilities = nullptr,
                              const std::function<std::vector<std::string>()> &get_description = nullptr ) const;
};

#endif // CATA_SRC_FACTION_CAMP_H
