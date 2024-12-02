#include "scores_ui.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "achievement.h"
#include "color.h"
#include "cursesdef.h"
#include "event_statistics.h"
#include "game.h"
#include "input_context.h"
#include "localized_comparator.h"
#include "kill_tracker.h"
#include "mtype.h"
#include "options.h"
#include "output.h"
#include "past_games_info.h"
#include "point.h"
#include "stats_tracker.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"

enum class scores_ui_tab_enum : int {
    achievements = 0,
    conducts,
    scores,
    kills,
    num_tabs
};

static scores_ui_tab_enum &operator++( scores_ui_tab_enum &c )
{
    c = static_cast<scores_ui_tab_enum>( static_cast<int>( c ) + 1 );
    if( c == scores_ui_tab_enum::num_tabs ) {
        c = static_cast<scores_ui_tab_enum>( 0 );
    }
    return c;
}

static scores_ui_tab_enum &operator--( scores_ui_tab_enum &c )
{
    if( c == static_cast<scores_ui_tab_enum>( 0 ) ) {
        c = scores_ui_tab_enum::num_tabs;
    }
    c = static_cast<scores_ui_tab_enum>( static_cast<int>( c ) - 1 );
    return c;
}

class scores_ui;

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

    private:
        void draw_achievements_text( bool use_conducts = false );
        void draw_scores_text();
        void draw_kills_text();

        scores_ui_tab_enum selected_tab = scores_ui_tab_enum::achievements;
        scores_ui_tab_enum switch_tab = scores_ui_tab_enum::num_tabs;

        size_t window_width = ImGui::GetMainViewport()->Size.x / 3.0 * 2.0;
        size_t window_height = ImGui::GetMainViewport()->Size.y / 3.0 * 2.0;

    protected:
        void draw_controls() override;
};

void scores_ui::draw_scores_ui()
{
    input_context ctxt;
    scores_ui_impl p_impl;

    ctxt.register_navigate_ui_list();
    ctxt.register_leftright();
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

void scores_ui_impl::draw_achievements_text( bool use_conducts )
{
    const achievements_tracker &achievements = g->achievements();
    if( !achievements.is_enabled() ) {
        ImGui::TextWrapped( "%s",
                            use_conducts
                            ? _( "Conducts are disabled, probably due to use of the debug menu.  If you only used "
                                 "the debug menu to work around a game bug, then you can re-enable conducts via the "
                                 "debug menu (\"Enable achievements\" under the \"Game\" submenu)." )
                            : _( "Achievements are disabled, probably due to use of the debug menu.  If you only used "
                                 "the debug menu to work around a game bug, then you can re-enable achievements via the "
                                 "debug menu (\"Enable achievements\" under the \"Game\" submenu)." ) );
        return;
    }
    // Load past game info beforehand because otherwise it may erase an `achievement_tracker`
    // within a call to its method when lazy-loaded, causing dangling pointer.
    get_past_games( false );
    std::vector<const achievement *> valid_achievements = achievements.valid_achievements();
    valid_achievements.erase(
        std::remove_if( valid_achievements.begin(), valid_achievements.end(),
    [&]( const achievement * a ) {
        return achievements.is_hidden( a ) || a->is_conduct() != use_conducts;
    } ), valid_achievements.end() );
    if( valid_achievements.empty() ) {
        ImGui::TextWrapped( "%s",
                            use_conducts
                            ? _( "This game has no valid conducts." )
                            : _( "This game has no valid achievements." ) );
        return;
    }
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
    for( const sortable_achievement &ach : sortable_achievements ) {
        cataimgui::draw_colored_text( achievements.ui_text_for( std::get<const achievement *>( ach ) ) );
        ImGui::Separator();
    }
    ImGui::NewLine();
    ImGui::TextWrapped( "%s",
                        use_conducts
                        ? _( "Note that only conducts that existed when you started this game and still exist now will appear here." )
                        : _( "Note that only achievements that existed when you started this game and still exist now will appear here." )
                      );
}

void scores_ui_impl::draw_scores_text()
{
    stats_tracker &stats = g->stats();
    std::vector<const score *> valid_scores = stats.valid_scores();
    if( valid_scores.empty() ) {
        ImGui::TextWrapped( "%s", _( "This game has no valid scores." ) );
        return;
    }
    for( const score *scr : valid_scores ) {
        ImGui::TextWrapped( "%s", scr->description( stats ).c_str() );
    }
    ImGui::NewLine();
    ImGui::TextWrapped( "%s",
                        _( "Note that only scores that existed when you started this game and still exist now will appear here." )
                      );
}

void scores_ui_impl::draw_kills_text()
{
    const kill_tracker &kills_data = g->get_kill_tracker();

    const int monster_kills = kills_data.monster_kill_count();
    const int npc_kills = kills_data.npc_kill_count();

    if( get_option<bool>( "STATS_THROUGH_KILLS" ) &&
        ImGui::CollapsingHeader( _( "Stats through kills:" ), ImGuiTreeNodeFlags_DefaultOpen ) ) {
        const int total_kills = kills_data.total_kill_count();
        ImGui::TextWrapped( _( "Total kills: %d" ), total_kills );
        ImGui::NewLine();
        ImGui::TextWrapped( _( "Experience: %d (%d points available)" ),
                            get_avatar().kill_xp,
                            get_avatar().free_upgrade_points() );
    }

    if( ImGui::CollapsingHeader( string_format( _( "Monster kills (%d):" ), monster_kills ).c_str(),
                                 ImGuiTreeNodeFlags_DefaultOpen ) ) {
        if( monster_kills == 0 ) {
            ImGui::TextWrapped( "%s", _( "You haven't killed any monsters yet!" ) );
        } else {
            for( const auto &entry : kills_data.kills ) {
                const int num_kills = entry.second;
                const mtype &m = entry.first.obj();
                const std::string &name = m.nname();
                const std::string &symbol = m.sym;
                nc_color color = m.color;

                ImGui::TextColored( c_light_gray, "%d ", num_kills );
                ImGui::SameLine( 0, 0 );
                cataimgui::PushMonoFont();
                ImGui::TextColored( color, "%s", symbol.c_str() );
                ImGui::PopFont();
                ImGui::SameLine( 0, 0 );
                ImGui::TextColored( c_light_gray, " %s", name.c_str() );
            }
        }
    }
    if( ImGui::CollapsingHeader( string_format( _( "NPC kills (%d):" ), npc_kills ).c_str(),
                                 ImGuiTreeNodeFlags_DefaultOpen ) ) {
        if( npc_kills == 0 ) {
            ImGui::TextWrapped( "%s", _( "You haven't killed any NPCs yet!" ) );
        } else {
            for( const auto &npc_name : kills_data.npc_kills ) {
                ImGui::TextColored( c_light_gray, "%d ", 1 );
                ImGui::SameLine( 0, 0 );
                cataimgui::PushMonoFont();
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
    } else if( last_action == "UP" ) {
        ImGui::SetKeyboardFocusHere( -1 );
    } else if( last_action == "DOWN" ) {
        ImGui::SetKeyboardFocusHere( 1 );
    } else if( last_action == "NEXT_TAB" || last_action == "RIGHT" ) {
        switch_tab = selected_tab;
        ++switch_tab;
    } else if( last_action == "PREV_TAB" || last_action == "LEFT" ) {
        switch_tab = selected_tab;
        --switch_tab;
    } else if( last_action == "PAGE_UP" ) {
        ImGui::SetWindowFocus(); // Dumb hack! Clear our focused item so listbox selection isn't nav highlighted.
        ImGui::SetScrollY( ImGui::GetScrollY() - ( window_height / 5.0f ) );
    } else if( last_action == "PAGE_DOWN" ) {
        ImGui::SetWindowFocus(); // Dumb hack! Clear our focused item so listbox selection isn't nav highlighted.
        ImGui::SetScrollY( ImGui::GetScrollY() + ( window_height / 5.0f ) );
    }

    ImGuiTabItemFlags_ flags = ImGuiTabItemFlags_None;

    if( ImGui::BeginTabBar( "##TAB_BAR" ) ) {
        flags = ImGuiTabItemFlags_None;
        if( switch_tab == scores_ui_tab_enum::achievements ) {
            flags = ImGuiTabItemFlags_SetSelected;
            switch_tab = scores_ui_tab_enum::num_tabs;
        }
        if( ImGui::BeginTabItem( _( "ACHIEVEMENTS" ), nullptr, flags ) ) {
            selected_tab = scores_ui_tab_enum::achievements;
            ImGui::EndTabItem();
        }
        flags = ImGuiTabItemFlags_None;
        if( switch_tab == scores_ui_tab_enum::conducts ) {
            flags = ImGuiTabItemFlags_SetSelected;
            switch_tab = scores_ui_tab_enum::num_tabs;
        }
        if( ImGui::BeginTabItem( _( "CONDUCTS" ), nullptr, flags ) ) {
            selected_tab = scores_ui_tab_enum::conducts;
            ImGui::EndTabItem();
        }
        flags = ImGuiTabItemFlags_None;
        if( switch_tab == scores_ui_tab_enum::scores ) {
            flags = ImGuiTabItemFlags_SetSelected;
            switch_tab = scores_ui_tab_enum::num_tabs;
        }
        if( ImGui::BeginTabItem( _( "SCORES" ), nullptr, flags ) ) {
            selected_tab = scores_ui_tab_enum::scores;
            ImGui::EndTabItem();
        }
        flags = ImGuiTabItemFlags_None;
        if( switch_tab == scores_ui_tab_enum::kills ) {
            flags = ImGuiTabItemFlags_SetSelected;
            switch_tab = scores_ui_tab_enum::num_tabs;
        }
        if( ImGui::BeginTabItem( _( "KILLS" ), nullptr, flags ) ) {
            selected_tab = scores_ui_tab_enum::kills;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    if( selected_tab == scores_ui_tab_enum::achievements ) {
        draw_achievements_text( false );
    }
    if( selected_tab == scores_ui_tab_enum::conducts ) {
        draw_achievements_text( true );
    }
    if( selected_tab == scores_ui_tab_enum::scores ) {
        draw_scores_text();
    }
    if( selected_tab == scores_ui_tab_enum::kills ) {
        draw_kills_text();
    }

}

void show_scores_ui()
{
    scores_ui new_instance;
    new_instance.draw_scores_ui();
}
