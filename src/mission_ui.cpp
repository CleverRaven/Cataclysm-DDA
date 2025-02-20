#include "game.h" // IWYU pragma: associated

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_imgui.h"
#include "color.h"
#include "dialogue.h"
#include "dialogue_helpers.h"
#include "game_constants.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "mission.h"
#include "npc.h"
#include "output.h"
#include "string_formatter.h"
#include "talker.h"
#include "translation.h"
#include "translations.h"
#include "ui_manager.h"
#include "units.h"
#include "units_utility.h"

enum class mission_ui_tab_enum : int {
    ACTIVE = 0,
    COMPLETED,
    FAILED,
    num_tabs
};

static mission_ui_tab_enum &operator++( mission_ui_tab_enum &c )
{
    c = static_cast<mission_ui_tab_enum>( static_cast<int>( c ) + 1 );
    if( c == mission_ui_tab_enum::num_tabs ) {
        c = static_cast<mission_ui_tab_enum>( 0 );
    }
    return c;
}

static mission_ui_tab_enum &operator--( mission_ui_tab_enum &c )
{
    if( c == static_cast<mission_ui_tab_enum>( 0 ) ) {
        c = mission_ui_tab_enum::num_tabs;
    }
    c = static_cast<mission_ui_tab_enum>( static_cast<int>( c ) - 1 );
    return c;
}

class mission_ui
{
        friend class mission_ui_impl;
    public:
        void draw_mission_ui();
};

class mission_ui_impl : public cataimgui::window
{
    public:
        std::string last_action;
        explicit mission_ui_impl() : cataimgui::window( _( "Your missions" ),
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav ) {
        }

    private:
        void draw_mission_names( std::vector<mission *> missions, int &selected_mission,
                                 bool &need_adjust ) const;
        void draw_selected_description( std::vector<mission *> missions, int &selected_mission );

        mission_ui_tab_enum selected_tab = mission_ui_tab_enum::ACTIVE;
        mission_ui_tab_enum switch_tab = mission_ui_tab_enum::num_tabs;

        float window_width = std::clamp( float( str_width_to_pixels( EVEN_MINIMUM_TERM_WIDTH ) ),
                                         ImGui::GetMainViewport()->Size.x / 2,
                                         ImGui::GetMainViewport()->Size.x );
        float window_height = std::clamp( float( str_height_to_pixels( EVEN_MINIMUM_TERM_HEIGHT ) ),
                                          ImGui::GetMainViewport()->Size.y / 2,
                                          ImGui::GetMainViewport()->Size.y );
        float table_column_width = window_width / 2;

        cataimgui::scroll s = cataimgui::scroll::none;

    protected:
        void draw_controls() override;
};

void mission_ui::draw_mission_ui()
{
    input_context ctxt;
    mission_ui_impl p_impl;

    ctxt.register_navigate_ui_list();
    ctxt.register_leftright();
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "CONFIRM", to_translation( "Set selected mission as current objective" ) );
    // We don't actually have a way to remap this right now
    //ctxt.register_action( "DOUBLE_CLICK", to_translation( "Set selected mission as current objective" ) );
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

void mission_ui_impl::draw_controls()
{
    ImGui::SetWindowSize( ImVec2( window_width, window_height ), ImGuiCond_Once );
    std::vector<mission *> umissions;

    static int selected_mission = 0;
    bool adjust_selected = false;

    if( last_action == "QUIT" ) {
        return;
    } else if( last_action == "UP" ) {
        adjust_selected = true;
        ImGui::SetKeyboardFocusHere( -1 );
        selected_mission--;
    } else if( last_action == "DOWN" ) {
        adjust_selected = true;
        ImGui::SetKeyboardFocusHere( 1 );
        selected_mission++;
    } else if( last_action == "NEXT_TAB" || last_action == "RIGHT" ) {
        adjust_selected = true;
        selected_mission = 0;
        s = cataimgui::scroll::begin;
        switch_tab = selected_tab;
        ++switch_tab;
    } else if( last_action == "PREV_TAB" || last_action == "LEFT" ) {
        adjust_selected = true;
        selected_mission = 0;
        s = cataimgui::scroll::begin;
        switch_tab = selected_tab;
        --switch_tab;
    } else if( last_action == "PAGE_UP" ) {
        ImGui::SetWindowFocus(); // Dumb hack! Clear our focused item so listbox selection isn't nav highlighted.
        s = cataimgui::scroll::page_up;
    } else if( last_action == "PAGE_DOWN" ) {
        ImGui::SetWindowFocus(); // Dumb hack! Clear our focused item so listbox selection isn't nav highlighted.
        s = cataimgui::scroll::page_down;
    }

    ImGuiTabItemFlags_ flags = ImGuiTabItemFlags_None;

    if( ImGui::BeginTabBar( "##TAB_BAR" ) ) {
        flags = ImGuiTabItemFlags_None;
        if( switch_tab == mission_ui_tab_enum::ACTIVE ) {
            flags = ImGuiTabItemFlags_SetSelected;
            switch_tab = mission_ui_tab_enum::num_tabs;
        }
        if( ImGui::BeginTabItem( _( "ACTIVE" ), nullptr, flags ) ) {
            selected_tab = mission_ui_tab_enum::ACTIVE;
            umissions = get_avatar().get_active_missions();
            ImGui::EndTabItem();
        }
        flags = ImGuiTabItemFlags_None;
        if( switch_tab == mission_ui_tab_enum::COMPLETED ) {
            flags = ImGuiTabItemFlags_SetSelected;
            switch_tab = mission_ui_tab_enum::num_tabs;
        }
        if( ImGui::BeginTabItem( _( "COMPLETED" ), nullptr, flags ) ) {
            selected_tab = mission_ui_tab_enum::COMPLETED;
            umissions = get_avatar().get_completed_missions();
            ImGui::EndTabItem();
        }
        flags = ImGuiTabItemFlags_None;
        if( switch_tab == mission_ui_tab_enum::FAILED ) {
            flags = ImGuiTabItemFlags_SetSelected;
            switch_tab = mission_ui_tab_enum::num_tabs;
        }
        if( ImGui::BeginTabItem( _( "FAILED" ), nullptr, flags ) ) {
            selected_tab = mission_ui_tab_enum::FAILED;
            umissions = get_avatar().get_failed_missions();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    if( selected_mission < 0 ) {
        selected_mission = 0;
    }

    if( static_cast<size_t>( selected_mission ) > umissions.size() - 1 ) {
        selected_mission = umissions.size() - 1;
    }

    // This action needs to be after umissions is populated
    if( !umissions.empty() && last_action == "CONFIRM" &&
        selected_tab == mission_ui_tab_enum::ACTIVE ) {
        get_avatar().set_active_mission( *umissions[selected_mission] );
    }

    if( umissions.empty() ) {
        static const std::map< mission_ui_tab_enum, std::string > nope = {
            { mission_ui_tab_enum::ACTIVE, translate_marker( "You have no active missions!" ) },
            { mission_ui_tab_enum::COMPLETED, translate_marker( "You haven't completed any missions!" ) },
            { mission_ui_tab_enum::FAILED, translate_marker( "You haven't failed any missions!" ) }
        };
        ImGui::TextWrapped( "%s", nope.at( selected_tab ).c_str() );
        return;
    }

    if( get_avatar().get_active_mission() ) {
        ImGui::TextWrapped( _( "Current objective: %s" ),
                            get_avatar().get_active_mission()->name().c_str() );
    }

    if( ImGui::BeginTable( "##MISSION_TABLE", 2, ImGuiTableFlags_None,
                           ImVec2( window_width, window_height ) ) ) {
        // Missions selection is purposefully thinner than the description, it has less to convey.
        ImGui::TableSetupColumn( _( "Missions" ), ImGuiTableColumnFlags_WidthStretch,
                                 table_column_width * 0.8 );
        ImGui::TableSetupColumn( _( "Description" ), ImGuiTableColumnFlags_WidthStretch,
                                 table_column_width * 1.2 );
        ImGui::TableHeadersRow();
        ImGui::TableNextColumn();
        draw_mission_names( umissions, selected_mission, adjust_selected );
        ImGui::TableNextColumn();
        draw_selected_description( umissions, selected_mission );
        ImGui::EndTable();
    }

    cataimgui::set_scroll( s );
}

void mission_ui_impl::draw_mission_names( std::vector<mission *> missions, int &selected_mission,
        bool &need_adjust ) const
{
    const int num_missions = missions.size();

    // roughly 6 lines of header info. title+tab+objective+table headers+2 lines worth of padding between those four
    const float header_height = ImGui::GetTextLineHeight() * 6;
    if( ImGui::BeginListBox( "##LISTBOX", ImVec2( table_column_width * 0.75,
                             window_height - header_height ) ) ) {
        for( int i = 0; i < num_missions; i++ ) {
            const bool is_selected = selected_mission == i;
            ImGui::PushID( i );
            if( ImGui::Selectable( missions[i]->name().c_str(), is_selected,
                                   ImGuiSelectableFlags_AllowDoubleClick ) ) {
                selected_mission = i;
                if( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) ) {
                    get_avatar().set_active_mission( *missions[selected_mission] );
                }
            }

            if( is_selected && need_adjust ) {
                ImGui::SetScrollHereY();
                ImGui::SetItemDefaultFocus();
            }
            ImGui::PopID();
        }
        ImGui::EndListBox();
    }
}

void mission_ui_impl::draw_selected_description( std::vector<mission *> missions,
        int &selected_mission )
{
    mission *miss = missions[selected_mission];
    ImGui::TextWrapped( _( "Mission: %s" ), miss->name().c_str() );
    if( miss->get_npc_id().is_valid() ) {
        npc *guy = g->find_npc( miss->get_npc_id() );
        if( guy ) {
            ImGui::TextWrapped( _( "Given by: %s" ), guy->disp_name().c_str() );
        }
    }
    ImGui::Separator();
    static std::string raw_description;
    static std::string parsed_description;
    // Avoid replacing expanded snippets with other valid snippet text on redraw
    if( raw_description != miss->get_description() ) {
        raw_description = miss->get_description();
        parsed_description = raw_description;
        // Handles(example)  <reward_count:item>   in description
        // Not handled in parse_tags for some reason!
        dialogue d( get_talker_for( get_avatar() ), nullptr, {} );
        const talk_effect_fun_t::likely_rewards_t &rewards = miss->get_likely_rewards();
        for( const auto &reward : rewards ) {
            std::string token = "<reward_count:" + itype_id( reward.second.evaluate( d ) ).str() + ">";
            parsed_description = string_replace( parsed_description, token, string_format( "%g",
                                                 reward.first.evaluate( d ) ) );
        }
        parse_tags( parsed_description, get_player_character(), get_player_character() );
    }
    cataimgui::draw_colored_text( parsed_description, c_unset, table_column_width * 1.15 );
    if( miss->has_deadline() ) {
        const time_point deadline = miss->get_deadline();
        if( selected_tab == mission_ui_tab_enum::ACTIVE ) {
            ImGui::TextWrapped( _( "Deadline: %s" ), to_string( deadline ).c_str() );
            const time_duration remaining = deadline - calendar::turn;
            std::string remaining_time;
            if( remaining <= 0_turns ) {
                remaining_time = _( "None!" );
            } else if( get_player_character().has_watch() ) {
                remaining_time = to_string( remaining );
            } else {
                remaining_time = to_string_approx( remaining );
            }
            ImGui::TextWrapped( _( "Time remaining: %s" ), remaining_time.c_str() );
        } else {
            const time_duration time_in_past = calendar::turn - deadline;
            std::string time_in_past_string;
            if( get_player_character().has_watch() ) {
                time_in_past_string = to_string( time_in_past );
            } else {
                time_in_past_string = to_string_approx( time_in_past );
            }
            if( deadline != calendar::turn_zero ) {
                if( selected_tab == mission_ui_tab_enum::COMPLETED ) {
                    cataimgui::draw_colored_text( string_format( _( "Completed: %s" ),
                                                  to_string( deadline ) ), c_green );
                } else if( selected_tab == mission_ui_tab_enum::FAILED ) {
                    cataimgui::draw_colored_text( string_format( _( "Failed at: %s" ),
                                                  to_string( deadline ).c_str() ), c_red );
                }
                cataimgui::draw_colored_text( string_format( _( "%s ago" ), time_in_past_string ), c_unset );
            }
        }
    }
    if( miss->has_target() ) {
        // TODO: target does not contain a z-component, targets are assumed to be on z=0
        const tripoint_abs_omt pos = get_player_character().pos_abs_omt();
        cataimgui::draw_colored_text( string_format( _( "Target: %s" ), miss->get_target().to_string() ),
                                      c_white );
        // Below is done instead of a table for the benefit of right-to-left languages
        //~Extra padding spaces in the English text are so that the replaced string vertically aligns with the one above
        cataimgui::draw_colored_text( string_format( _( "You:    %s" ), pos.to_string() ), c_white );
        int omt_distance = rl_dist( pos, miss->get_target() );
        if( omt_distance > 0 ) {
            // One OMT is 24 tiles across, at 1x1 meters each, so we can simply do number of OMTs * 24
            units::length actual_distance = omt_distance * 24_meter;
            //~Parenthesis is a real-world value for distance. Example string: "Distance: 223 tiles (5352 m)"
            cataimgui::draw_colored_text( string_format( _( "Distance: %1$s tiles (%2$s)" ),
                                          omt_distance, length_to_string_approx( actual_distance ) ), c_white );
        }
    }
}

void game::list_missions()
{
    mission_ui new_instance;
    new_instance.draw_mission_ui();
}
