#include "study_zone_ui.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "avatar.h"
#include "cata_imgui.h"
#include "game.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "localized_comparator.h"
#include "npc.h"
#include "overmapbuffer.h"
#include "skill.h"
#include "translations.h"
#include "ui_manager.h"


static int filter_skill_input_callback( ImGuiInputTextCallbackData *data )
{
    if( data->EventChar < 256 ) {
        char c = static_cast<char>( data->EventChar );
        if( !std::islower( c ) && c != ' ' ) {
            return 1;
        }
    }
    return 0;
}

class study_zone_window : public cataimgui::window
{
    public:
        study_zone_window( std::map<std::string, std::set<skill_id>> &npc_skill_preferences ) :
            cataimgui::window( _( "Study Zone Skill Preferences" ), ImGuiWindowFlags_NoNavInputs ),
            npc_skill_preferences( npc_skill_preferences ) {
            ctxt = input_context( "STUDY_ZONE_UI" );
            ctxt.register_action( "CONFIRM" );
            ctxt.register_action( "FILTER" );
            ctxt.set_timeout( 10 );

            // Get all skills
            for( const Skill &s : Skill::skills ) {
                all_skills.push_back( s.ident() );
            }
            std::sort( all_skills.begin(), all_skills.end(),
            []( const skill_id & a, const skill_id & b ) {
                return localized_compare( a->name(), b->name() );
            } );

            // get followers
            std::set<character_id> follower_ids = g->get_follower_list();
            for( const character_id &id : follower_ids ) {
                shared_ptr_fast<npc> npc_ptr = overmap_buffer.find_npc( id );
                if( npc_ptr && !npc_ptr->is_hallucination() && npc_ptr->is_player_ally() ) {
                    npc_names.push_back( npc_ptr->name );
                }
            }
            // include NPCs that are already in preferences but not in follower list
            for( const auto &pair : npc_skill_preferences ) {
                if( std::find( npc_names.begin(), npc_names.end(), pair.first ) == npc_names.end() ) {
                    npc_names.push_back( pair.first );
                }
            }
            std::sort( npc_names.begin(), npc_names.end(), localized_compare );

            // for new NPCs set all checkboxes
            for( const std::string &npc_name : npc_names ) {
                if( npc_skill_preferences.find( npc_name ) == npc_skill_preferences.end() ) {
                    std::set<skill_id> &npc_skills = npc_skill_preferences[npc_name];
                    for( const skill_id &skill : all_skills ) {
                        npc_skills.insert( skill );
                    }
                    changed = true;
                }
            }
        }

        study_zone_ui_result execute() {
            bool canceled_result = false;
            while( get_is_open() ) {
                ui_manager::redraw();
                std::string action = ctxt.handle_input();

                if( action == "CONFIRM" ) {
                    break;
                }

                if( action == "QUIT" ) {
                    canceled_result = true;
                    break;
                }
                if( action == "FILTER" ) {
                    filter_just_focused = true;
                }

            }

            if( canceled_result ) {
                return study_zone_ui_result::canceled;
            }
            return changed ? study_zone_ui_result::changed : study_zone_ui_result::successful;
        }

    protected:
        void draw_skill_row( const skill_id &skill ) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text( "%s", skill->name().c_str() );

            for( const std::string &npc_name : npc_names ) {
                ImGui::TableNextColumn();

                std::set<skill_id> &npc_skills = npc_skill_preferences[npc_name];

                bool is_selected = npc_skills.count( skill ) > 0;
                bool was_selected = is_selected;
                std::string checkbox_id = "##" + skill.str() + "_" + npc_name;
                if( ImGui::Checkbox( checkbox_id.c_str(), &is_selected ) ) {
                    changed = true;
                }
                if( is_selected != was_selected ) {
                    if( is_selected ) {
                        npc_skills.insert( skill );
                    } else {
                        npc_skills.erase( skill );
                    }
                    if( npc_skills.empty() ) {
                        npc_skill_preferences.erase( npc_name );
                    }
                }
            }
        }

        void draw_footer( const std::vector<skill_id> &filtered_skills ) {
            // filter input, buttons, and Done button
            ImGui::Separator();

            std::string filter_label = _( "Filter skills: " );
            ImGui::Text( "%s", filter_label.c_str() );
            ImGui::SameLine();

            std::string filter_input_id = "##skill_filter";
            if( filter_just_focused ) {
                ImGui::SetKeyboardFocusHere();
                filter_just_focused = false;
            }

            ImGui::SetNextItemWidth( 300.0f );
            char filter_buffer[256] = {0};
            strncpy( filter_buffer, skill_filter.c_str(), sizeof( filter_buffer ) - 1 );
            ImGui::InputText( filter_input_id.c_str(), filter_buffer, sizeof( filter_buffer ),
                              ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCharFilter,
                              filter_skill_input_callback );
            skill_filter = filter_buffer;

            ImGui::SameLine();
            if( ImGui::Button( _( "Check All" ) ) ) {
                for( const std::string &npc_name : npc_names ) {
                    std::set<skill_id> &npc_skills = npc_skill_preferences[npc_name];
                    for( const skill_id &skill : filtered_skills ) {
                        npc_skills.insert( skill );
                    }
                }
                changed = true;
            }

            ImGui::SameLine();
            if( ImGui::Button( _( "Clear All" ) ) ) {
                for( const std::string &npc_name : npc_names ) {
                    std::set<skill_id> &npc_skills = npc_skill_preferences[npc_name];
                    for( const skill_id &skill : filtered_skills ) {
                        npc_skills.erase( skill );
                    }
                    if( npc_skills.empty() ) {
                        npc_skill_preferences.erase( npc_name );
                    }
                }
                changed = true;
            }

            ImGui::SameLine();
            if( ImGui::Button( _( "Done" ) ) ) {
                is_open = false;
            }
        }

        cataimgui::bounds get_bounds() override {
            ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
            float width = std::min( 800.0f, viewport_size.x * 0.8f );
            float height = viewport_size.y;
            return { -1.f, -1.f, width, height };
        }

        void draw_controls() override {
            std::vector<skill_id> filtered_skills;
            if( skill_filter.empty() ) {
                filtered_skills = all_skills;
            } else {
                for( const skill_id &skill : all_skills ) {
                    if( skill->name().find( skill_filter ) != std::string::npos ) {
                        filtered_skills.push_back( skill );
                    }
                }
            }

            const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y * 2 +
                                                   ImGui::GetFrameHeightWithSpacing() * 3;

            // inner scroll
            if( ImGui::BeginChild( "table_scroll_region", ImVec2( 0, -footer_height_to_reserve ), false,
                                   ImGuiWindowFlags_HorizontalScrollbar ) ) {
                // Create a table with Skills as rows and npc names as columns
                if( ImGui::BeginTable( "skill_npc_table", static_cast<int>( npc_names.size() ) + 1,
                                       ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
                                       ImGuiTableFlags_RowBg ) ) {
                    // table header row
                    ImGui::TableSetupColumn( "Skill", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide,
                                             150.0f );
                    for( const std::string &npc_name : npc_names ) {
                        ImGui::TableSetupColumn( npc_name.c_str(), ImGuiTableColumnFlags_WidthFixed, 100.0f );
                    }
                    // freeze skill column horizontally and header vertically
                    ImGui::TableSetupScrollFreeze( 1, 1 );
                    ImGui::TableHeadersRow();

                    // skill rows
                    ImGuiListClipper clipper;
                    clipper.Begin( filtered_skills.size() );
                    while( clipper.Step() ) {
                        for( int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++ ) {
                            draw_skill_row( filtered_skills[row] );
                        }
                    }

                    ImGui::EndTable();
                }
            }
            ImGui::EndChild();

            draw_footer( filtered_skills );
        }

    private:
        std::map<std::string, std::set<skill_id>> &npc_skill_preferences;
        std::vector<skill_id> all_skills;
        std::vector<std::string> npc_names;
        std::string skill_filter;
        bool filter_just_focused = false;
        bool changed = false;
        input_context ctxt;
};

study_zone_ui_result query_study_zone_skills( std::map<std::string, std::set<skill_id>>
        &npc_skill_preferences )
{
    study_zone_window window( npc_skill_preferences );
    return window.execute();
}

