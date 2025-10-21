#include <imgui/imgui.h>
#include <algorithm>
#include <memory>

#include "avatar.h"
#include "ascii_art.h"
#include "cata_imgui.h"
#include "condition.h"
#include "dialogue.h"
#include "end_screen.h"
#include "event.h"
#include "event_bus.h"
#include "game.h"
#include "generic_factory.h"
#include "imgui/imgui_stdlib.h"
#include "input_context.h"
#include "npc.h" // for parse_tags()! Why!
#include "talker.h"
#include "ui_manager.h"

static const ascii_art_id ascii_art_ascii_tombstone( "ascii_tombstone" );

namespace
{
generic_factory<end_screen> end_screen_factory( "end_screen" );
} // namespace

template<>
const end_screen &string_id<end_screen>::obj()const
{
    return end_screen_factory.obj( *this );
}

template<>
bool string_id<end_screen>::is_valid() const
{
    return end_screen_factory.is_valid( *this );
}

void end_screen::load_end_screen( const JsonObject &jo, const std::string &src )
{
    end_screen_factory.load( jo, src );
}

void end_screen::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "picture_id", picture_id );
    mandatory( jo, was_loaded, "priority", priority );
    read_condition( jo, "condition", condition, false );

    optional( jo, was_loaded, "added_info", added_info );
    optional( jo, was_loaded, "last_words_label", last_words_label );
}

const std::vector<end_screen> &end_screen::get_all()
{
    return end_screen_factory.get_all();
}

void end_screen_data::draw_end_screen_ui( bool actually_dead )
{
    input_context ctxt;
    ctxt.register_action( "TEXT.CONFIRM" );
#if defined(WIN32) || defined(TILES)
    ctxt.set_timeout( 50 );
#endif
    end_screen_ui_impl p_impl;

    while( true ) {
        ui_manager::redraw_invalidated();
        std::string action = ctxt.handle_input();
        if( action == "TEXT.CONFIRM" || !p_impl.get_is_open() ) {
            break;
        }
    }
    if( actually_dead ) {
        avatar &u = get_avatar();
        const bool is_suicide = g->uquit == QUIT_SUICIDE;
        get_event_bus().send<event_type::game_avatar_death>( u.getID(), u.name, is_suicide, p_impl.text );
    }
}

void end_screen_ui_impl::draw_controls()
{
    avatar &u = get_avatar();
    ascii_art_id art = ascii_art_ascii_tombstone;
    dialogue d( get_talker_for( u ), nullptr );
    std::string input_label;
    std::vector<std::pair<std::pair<int, int>, std::string>> added_info;

    //Sort end_screens in order of decreasing priority
    std::vector<end_screen> sorted_screens = end_screen::get_all();
    std::sort( sorted_screens.begin(), sorted_screens.end(), []( end_screen const & a,
    end_screen const & b ) {
        return a.priority > b.priority;
    } );

    for( const end_screen &e_screen : sorted_screens ) {
        if( e_screen.condition( d ) ) {
            art = e_screen.picture_id;
            if( !e_screen.added_info.empty() ) {
                added_info = e_screen.added_info;
            }
            if( !e_screen.last_words_label.empty() ) {
                input_label = e_screen.last_words_label;
            }
            break;
        }
    }

    if( art.is_valid() ) {
        cataimgui::PushMonoFont();
        int row = 1;
        for( const std::string &line : art->picture ) {
            cataimgui::draw_colored_text( line );

            for( std::pair<std::pair<int, int>, std::string> info : added_info ) {
                if( row ==  info.first.second ) {
                    parse_tags( info.second, u, u );
                    ImGui::SameLine( str_width_to_pixels( info.first.first ), 0 );
                    cataimgui::draw_colored_text( info.second );
                }
            }
            row++;
        }
        ImGui::PopFont();
    }

    if( !input_label.empty() ) {
        ImGui::NewLine();
        ImGui::AlignTextToFramePadding();
        cataimgui::draw_colored_text( input_label );
        ImGui::SameLine( str_width_to_pixels( input_label.size() + 2 ), 0 );
        ImGui::InputText( "##LAST_WORD_BOX", &text );
        ImGui::SetKeyboardFocusHere( -1 );
    }

}
