#include "scores_ui.h"

#include <imgui/imgui.h>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "achievement.h"
#include "avatar.h"
#include "cata_imgui.h"
#include "color.h"
#include "enum_traits.h"
#include "event_statistics.h"
#include "game.h"
#include "input_context.h"
#include "kill_tracker.h"
#include "localized_comparator.h"
#include "mtype.h"
#include "options.h"
#include "past_games_info.h"
#include "stats_tracker.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translation.h"
#include "translations.h"
#include "ui_manager.h"

template <typename E> struct enum_traits;

enum class scores_ui_tab : int {
    achievements = 0,
    conducts,
    scores,
    kills,
    num_tabs
};

template<>
struct enum_traits<scores_ui_tab> {
    static constexpr scores_ui_tab first = scores_ui_tab::achievements;
    static constexpr scores_ui_tab last = scores_ui_tab::num_tabs;
};

class scores_ui
{
        friend class scores_ui_impl;
    public:
        void draw_scores_ui();
};

class scores_ui_impl : public cataimgui::window
{
    public:
        std::string last_action;
        explicit scores_ui_impl() : cataimgui::window( _( "Your scores" ),
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav ) {}
        void init_data();

    private:
        void draw_achievements_text( bool use_conducts = false ) const;
        void draw_scores_text() const;
        void draw_kills_text() const;

        scores_ui_tab selected_tab = enum_traits<scores_ui_tab>::first;
        scores_ui_tab switch_tab = enum_traits<scores_ui_tab>::last;

        size_t window_width = ImGui::GetMainViewport()->Size.x * 8 / 9;
        size_t window_height = ImGui::GetMainViewport()->Size.y * 8 / 9;

        bool monster_group_collapsed = false;
        bool npc_group_collapsed = false;

        std::vector<std::string> achievements_text;
        std::vector<std::string> conducts_text;
        std::vector<std::string> scores_text;
        std::vector<std::tuple<int, std::string, nc_color, std::string>> monster_kills_data;
        std::vector<std::string> npc_kills_data;

        int monster_kills = 0;
        int npc_kills = 0;
        int total_kills = 0;

        cataimgui::scroll s = cataimgui::scroll::none;

    protected:
        void draw_controls() override;
};

void scores_ui::draw_scores_ui()
{
    input_context ctxt( "SCORES_UI" );
    scores_ui_impl p_impl;

    p_impl.init_data();

    ctxt.register_navigate_ui_list();
    ctxt.register_leftright();
    ctxt.register_action( "TOGGLE_MONSTER_GROUP" );
    ctxt.register_action( "TOGGLE_NPC_GROUP" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    // Smooths out our handling, makes tabs load immediately after input instead of waiting for next.
    ctxt.set_timeout( 10 );

    while( true ) {
        ui_manager::redraw_invalidated();

        p_impl.last_action = ctxt.handle_input();

        if( p_impl.last_action == "QUIT" || !p_impl.get_is_open() ) {
            break;
        }
    }
}

void scores_ui_impl::init_data()
{
    const achievements_tracker &achievements = g->achievements();
    // Load past game info beforehand because otherwise it may erase an `achievement_tracker`
    // within a call to its method when lazy-loaded, causing dangling pointer.
    get_past_games();
    std::vector<const achievement *> valid_achievements = achievements.valid_achievements();
    valid_achievements.erase(
        std::remove_if( valid_achievements.begin(), valid_achievements.end(),
    [&]( const achievement * a ) {
        return achievements.is_hidden( a );
    } ), valid_achievements.end() );

    using sortable_achievement =
        std::tuple<achievement_completion, std::string, const achievement *>;
    std::vector<sortable_achievement> sortable_achievements;
    std::transform( valid_achievements.begin(), valid_achievements.end(),
                    std::back_inserter( sortable_achievements ),
    [&]( const achievement * ach ) {
        achievement_completion comp = achievements.is_completed( ach->id );
        return std::make_tuple( comp, ach->name().translated(), ach );
    } );
    std::sort( sortable_achievements.begin(), sortable_achievements.end(), localized_compare );
    for( const sortable_achievement &entry : sortable_achievements ) {
        const achievement *ach = std::get<const achievement *>( entry );
        if( ach->is_conduct() ) {
            conducts_text.emplace_back( achievements.ui_text_for( ach ) );
        } else {
            achievements_text.emplace_back( achievements.ui_text_for( ach ) );
        }
    }

    stats_tracker &stats = g->stats();
    std::vector<const score *> valid_scores = stats.valid_scores();
    scores_text.reserve( stats.valid_scores().size() );
    for( const score *scr : valid_scores ) {
        scores_text.emplace_back( scr->description( stats ).c_str() );
    }

    const kill_tracker &kills_data = g->get_kill_tracker();
    monster_kills = kills_data.monster_kill_count();
    npc_kills = kills_data.npc_kill_count();
    total_kills = kills_data.total_kill_count();

    for( const auto &entry : kills_data.kills ) {
        const int num_kills = entry.second;
        const mtype &m = entry.first.obj();
        const std::string &symbol = m.sym;
        nc_color color = m.color;
        const std::string &name = m.nname();
        monster_kills_data.emplace_back( num_kills, symbol, color, name );
    }
    auto sort_by_count_then_name = []( auto & a, auto & b ) {
        return
            std::tie( std::get<0>( a ), std::get<3>( b ) )
            >
            std::tie( std::get<0>( b ), std::get<3>( a ) );
    };
    std::sort( monster_kills_data.begin(), monster_kills_data.end(), sort_by_count_then_name );
    for( const auto &entry : kills_data.npc_kills ) {
        npc_kills_data.emplace_back( entry );
    }
    std::sort( npc_kills_data.begin(), npc_kills_data.end(), localized_compare );
}

void scores_ui_impl::draw_achievements_text( bool use_conducts ) const
{
    if( !g->achievements().is_enabled() ) {
        ImGui::TextWrapped( "%s", _( "Achievements and conducts are disabled for debug characters." ) );
        return;
    }
    if( use_conducts && conducts_text.empty() ) {
        ImGui::TextWrapped( "%s", _( "This game has no valid conducts." ) );
        return;
    }
    if( !use_conducts && achievements_text.empty() ) {
        ImGui::TextWrapped( "%s", _( "This game has no valid achievements." ) );
        return;
    }
    for( const std::string &entry : use_conducts ? conducts_text : achievements_text ) {
        cataimgui::draw_colored_text( entry );
        ImGui::Separator();
    }
    ImGui::NewLine();
    ImGui::TextWrapped( "%s",
                        use_conducts
                        ? _( "Note that only conducts that existed when you started this game and still exist now will appear here." )
                        : _( "Note that only achievements that existed when you started this game and still exist now will appear here." )
                      );
}

void scores_ui_impl::draw_scores_text() const
{
    if( scores_text.empty() ) {
        ImGui::TextWrapped( "%s", _( "This game has no valid scores." ) );
        return;
    }
    for( const std::string &entry : scores_text ) {
        ImGui::TextWrapped( "%s", entry.c_str() );
    }
    ImGui::NewLine();
    ImGui::TextWrapped( "%s",
                        _( "Note that only scores that existed when you started this game and still exist now will appear here." )
                      );
}

void scores_ui_impl::draw_kills_text() const
{
    if( get_option<bool>( "STATS_THROUGH_KILLS" ) &&
        ImGui::CollapsingHeader( _( "Stats through kills:" ), ImGuiTreeNodeFlags_DefaultOpen ) ) {
        ImGui::TextWrapped( _( "Total kills: %d" ), total_kills );
        ImGui::NewLine();
        ImGui::TextWrapped( _( "Experience: %d (%d points available)" ),
                            get_avatar().kill_xp,
                            get_avatar().free_upgrade_points() );
    }

    if( ImGui::CollapsingHeader( string_format( _( "Monster kills (%d):" ), monster_kills ).c_str(),
                                 monster_group_collapsed ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_DefaultOpen ) ) {
        if( monster_kills == 0 ) {
            ImGui::TextWrapped( "%s", _( "You haven't killed any monsters yet!" ) );
        } else {
            for( const auto &entry : monster_kills_data ) {
                const int num_kills = std::get<0>( entry );
                const std::string &symbol = std::get<1>( entry );
                nc_color color = std::get<2>( entry );
                const std::string &name = std::get<3>( entry );

                cataimgui::PushMonoFont();
                ImGui::TextColored( c_light_gray, "%5d ", num_kills );
                ImGui::SameLine( 0, 0 );
                ImGui::TextColored( color, "%s", symbol.c_str() );
                ImGui::PopFont();
                ImGui::SameLine( 0, 0 );
                ImGui::TextColored( c_light_gray, " %s", name.c_str() );
            }
        }
    }
    if( ImGui::CollapsingHeader( string_format( _( "NPC kills (%d):" ), npc_kills ).c_str(),
                                 npc_group_collapsed ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_DefaultOpen ) ) {
        if( npc_kills == 0 ) {
            ImGui::TextWrapped( "%s", _( "You haven't killed any NPCs yet!" ) );
        } else {
            for( const std::string &npc_name : npc_kills_data ) {
                cataimgui::PushMonoFont();
                ImGui::TextColored( c_light_gray, "%5d ", 1 );
                ImGui::SameLine( 0, 0 );
                ImGui::TextColored( c_magenta, "%s", "@" );
                ImGui::PopFont();
                ImGui::SameLine( 0, 0 );
                ImGui::TextColored( c_light_gray, " %s", npc_name.c_str() );
            }
        }
    }
}

void scores_ui_impl::draw_controls()
{
    ImGui::SetWindowSize( ImVec2( window_width, window_height ), ImGuiCond_Once );

    if( last_action == "QUIT" ) {
        return;
    } else if( last_action == "TOGGLE_MONSTER_GROUP" ) {
        monster_group_collapsed = !monster_group_collapsed;
    } else if( last_action == "TOGGLE_NPC_GROUP" ) {
        npc_group_collapsed = !npc_group_collapsed;
    } else if( last_action == "UP" ) {
        s = cataimgui::scroll::line_up;
    } else if( last_action == "DOWN" ) {
        s = cataimgui::scroll::line_down;
    } else if( last_action == "NEXT_TAB" || last_action == "RIGHT" ) {
        s = cataimgui::scroll::begin;
        switch_tab = selected_tab;
        ++switch_tab;
    } else if( last_action == "PREV_TAB" || last_action == "LEFT" ) {
        s = cataimgui::scroll::begin;
        switch_tab = selected_tab;
        --switch_tab;
    } else if( last_action == "PAGE_UP" ) {
        s = cataimgui::scroll::page_up;
    } else if( last_action == "PAGE_DOWN" ) {
        s = cataimgui::scroll::page_down;
    } else if( last_action == "HOME" ) {
        s = cataimgui::scroll::begin;
    } else if( last_action == "END" ) {
        s = cataimgui::scroll::end;
    }

    ImGuiTabItemFlags_ flags = ImGuiTabItemFlags_None;

    if( ImGui::BeginTabBar( "##TAB_BAR" ) ) {
        flags = ImGuiTabItemFlags_None;
        if( switch_tab == scores_ui_tab::achievements ) {
            flags = ImGuiTabItemFlags_SetSelected;
            switch_tab = enum_traits<scores_ui_tab>::last;
        }
        if( ImGui::BeginTabItem( _( "ACHIEVEMENTS" ), nullptr, flags ) ) {
            selected_tab = scores_ui_tab::achievements;
            ImGui::EndTabItem();
        }
        flags = ImGuiTabItemFlags_None;
        if( switch_tab == scores_ui_tab::conducts ) {
            flags = ImGuiTabItemFlags_SetSelected;
            switch_tab = enum_traits<scores_ui_tab>::last;
        }
        if( ImGui::BeginTabItem( _( "CONDUCTS" ), nullptr, flags ) ) {
            selected_tab = scores_ui_tab::conducts;
            ImGui::EndTabItem();
        }
        flags = ImGuiTabItemFlags_None;
        if( switch_tab == scores_ui_tab::scores ) {
            flags = ImGuiTabItemFlags_SetSelected;
            switch_tab = enum_traits<scores_ui_tab>::last;
        }
        if( ImGui::BeginTabItem( _( "SCORES" ), nullptr, flags ) ) {
            selected_tab = scores_ui_tab::scores;
            ImGui::EndTabItem();
        }
        flags = ImGuiTabItemFlags_None;
        if( switch_tab == scores_ui_tab::kills ) {
            flags = ImGuiTabItemFlags_SetSelected;
            switch_tab = enum_traits<scores_ui_tab>::last;
        }
        if( ImGui::BeginTabItem( _( "KILLS" ), nullptr, flags ) ) {
            selected_tab = scores_ui_tab::kills;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    if( selected_tab == scores_ui_tab::achievements ) {
        draw_achievements_text();
    }
    if( selected_tab == scores_ui_tab::conducts ) {
        draw_achievements_text( true );
    }
    if( selected_tab == scores_ui_tab::scores ) {
        draw_scores_text();
    }
    if( selected_tab == scores_ui_tab::kills ) {
        draw_kills_text();
    }

    cataimgui::set_scroll( s );
}

void show_scores_ui()
{
    scores_ui new_instance;
    new_instance.draw_scores_ui();
}
