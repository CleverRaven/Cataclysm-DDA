#include "cata_imgui.h"

#include <stack>
#include <type_traits>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "color.h"
#include "input.h"
#include "output.h"
#include "ui_manager.h"

#if !(defined(TILES) || defined(WIN32))
#include <curses.h>
#include <imtui/imtui-impl-ncurses.h>
#include <imtui/imtui-impl-text.h>

#include "color_loader.h"

struct RGBTuple {
    uint8_t Blue;
    uint8_t Green;
    uint8_t Red;
};

struct pairs {
    short FG;
    short BG;
};

std::array<RGBTuple, color_loader<RGBTuple>::COLOR_NAMES_COUNT> rgbPalette;
std::array<pairs, 100> colorpairs;   //storage for pair'ed colored

ImTui::TScreen *imtui_screen = nullptr;
std::vector<std::pair<int, ImTui::mouse_event>> imtui_events;

cataimgui::client::client()
{
    load_colors();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    imtui_screen = ImTui_ImplNcurses_Init();
    ImTui_ImplText_Init();

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
}

cataimgui::client::~client()
{
    ImTui_ImplNcurses_Shutdown();
    ImTui_ImplText_Shutdown();
    ImGui::Shutdown();
}

void cataimgui::client::new_frame()
{
    ImTui_ImplNcurses_NewFrame( imtui_events );
    imtui_events.clear();
    ImTui_ImplText_NewFrame();

    ImGui::NewFrame();
}

void cataimgui::client::end_frame()
{
    ImGui::Render();

    ImTui_ImplText_RenderDrawData( ImGui::GetDrawData(), imtui_screen );
    ImTui_ImplNcurses_DrawScreen();
}

void cataimgui::client::upload_color_pair( int p, int f, int b )
{
    ImTui_ImplNcurses_UploadColorPair( p, static_cast<short>( f ), static_cast<short>( b ) );
    cataimgui::init_pair( p, f, b );
}

void cataimgui::client::set_alloced_pair_count( short count )
{
    ImTui_ImplNcurses_SetAllocedPairCount( count );
}

void cataimgui::client::process_input( void *input )
{
    if( input ) {
        input_event *curses_input = static_cast<input_event *>( input );
        ImTui::mouse_event new_mouse_event = ImTui::mouse_event();
        if( curses_input->type == input_event_t::mouse ) {
            new_mouse_event.x = curses_input->mouse_pos.x;
            new_mouse_event.y = curses_input->mouse_pos.y;
            new_mouse_event.bstate = 0;
            for( int input_raw_key : curses_input->sequence ) {
                switch( static_cast<MouseInput>( input_raw_key ) ) {
                    case MouseInput::LeftButtonPressed:
                        new_mouse_event.bstate |= BUTTON1_PRESSED;
                        break;
                    case MouseInput::LeftButtonReleased:
                        new_mouse_event.bstate |= BUTTON1_RELEASED;
                        break;
                    case MouseInput::RightButtonPressed:
                        new_mouse_event.bstate |= BUTTON3_PRESSED;
                        break;
                    case MouseInput::RightButtonReleased:
                        new_mouse_event.bstate |= BUTTON3_RELEASED;
                        break;
                    case MouseInput::ScrollWheelUp:
                        new_mouse_event.bstate |= BUTTON4_PRESSED;
                        break;
                    case MouseInput::ScrollWheelDown:
                        new_mouse_event.bstate |= BUTTON5_PRESSED;
                        break;
                    default:
                        break;
                }
            }
            imtui_events.push_back( std::pair<int, ImTui::mouse_event>( KEY_MOUSE, new_mouse_event ) );
        } else {
            int ch = curses_input->get_first_input();
            if( ch != UNKNOWN_UNICODE ) {
                imtui_events.push_back( std::pair<int, ImTui::mouse_event>( ch, new_mouse_event ) );
            }
        }
    }
}

void cataimgui::load_colors()
{

    color_loader<RGBTuple>().load( rgbPalette );
}

void cataimgui::init_pair( int p, int f, int b )
{
    colorpairs[p].FG = f;
    colorpairs[p].BG = b;
}

template<>
RGBTuple color_loader<RGBTuple>::from_rgb( const int r, const int g, const int b )
{
    RGBTuple result;
    // Blue
    result.Blue = b;
    // Green
    result.Green = g;
    // Red
    result.Red = r;
    return result;
}
#else
#include "sdl_utils.h"
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_impl_sdlrenderer2.h>

SDL_Renderer *cataimgui::client::sdl_renderer = nullptr;
SDL_Window *cataimgui::client::sdl_window = nullptr;

cataimgui::client::client()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ( void )io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer( sdl_window, sdl_renderer );
    ImGui_ImplSDLRenderer2_Init( sdl_renderer );
}

cataimgui::client::~client()
{
    ImGui_ImplSDL2_Shutdown();
}

void cataimgui::client::new_frame()
{
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();

    ImGui::NewFrame();
}

void cataimgui::client::end_frame()
{
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData( ImGui::GetDrawData() );
}

void cataimgui::client::process_input( void *input )
{
    ImGui_ImplSDL2_ProcessEvent( static_cast<const SDL_Event *>( input ) );
}

#endif

void cataimgui::point_to_imvec2( point *src, ImVec2 *dest )
{
    if( src != nullptr && dest != nullptr ) {
        dest->x = src->x;
        dest->y = src->y;
    }
}

void cataimgui::imvec2_to_point( ImVec2 *src, point *dest )
{
    if( src != nullptr && dest != nullptr ) {
        dest->x = src->x;
        dest->y = src->y;
    }
}

void cataimgui::window::draw_colored_text( std::string const &text, const nc_color &color,
        float wrap_width, bool *is_selected, bool *is_focused, bool *is_hovered )
{
    nc_color color_cpy = color;
    draw_colored_text( text, color_cpy, wrap_width, is_selected, is_focused, is_hovered );
}

void cataimgui::window::draw_colored_text( std::string const &text, nc_color &color,
        float wrap_width, bool *is_selected, bool *is_focused, bool *is_hovered )
{
    ImGui::PushID( text.c_str() );
    ImGuiID itemId = GImGui->CurrentWindow->IDStack.back();
    std::stack<nc_color> color_stack;
    color_stack.push( color );
    size_t chars_per_line = size_t( wrap_width );
    if( chars_per_line == 0 ) {
        chars_per_line = SIZE_MAX;
    }
#if defined(WIN32) || defined(TILES)
    size_t char_width = size_t( ImGui::CalcTextSize( " " ).x );
    chars_per_line /= char_width;
#endif
    std::vector<std::string> folded_msg = foldstring( text, chars_per_line );

    for( const std::string &line : folded_msg ) {

        const auto color_segments = split_by_color( line );
        if( is_selected != nullptr ) {
            ImGui::Selectable( "", is_selected );
            ImGui::SameLine( 0, 0 );
        }
        int i = 0;
        for( auto seg : color_segments ) {
            if( seg.empty() ) {
                continue;
            }

            if( seg[0] == '<' ) {
                const color_tag_parse_result::tag_type type =
                    update_color_stack( color_stack, seg, report_color_error::yes );
                if( type != color_tag_parse_result::non_color_tag ) {
                    seg = rm_prefix( seg );
                }
            }

            color = color_stack.empty() ? color : color_stack.top();
            if( i++ != 0 ) {
                ImGui::SameLine( 0, 0 );
            }
#if !(defined(TILES) || defined(WIN32))
            int pair_id = color.get_index();
            pairs &pair = colorpairs[pair_id];

            int palette_index = pair.FG != 0 ? pair.FG : pair.BG;
            if( color.is_bold() ) {
                palette_index += color_loader<RGBTuple>::COLOR_NAMES_COUNT / 2;
            }
            RGBTuple &rgbCol = rgbPalette[palette_index];
            ImGui::TextColored( { static_cast<float>( rgbCol.Red / 255. ), static_cast<float>( rgbCol.Green / 255. ),
                                  static_cast<float>( rgbCol.Blue / 255. ), static_cast<float>( 255. ) },
                                "%s", seg.c_str() );
            GImGui->LastItemData.ID = itemId;
#else
            SDL_Color c = curses_color_to_SDL( color );
            ImGui::TextColored( { static_cast<float>( c.r / 255. ), static_cast<float>( c.g / 255. ),
                                  static_cast<float>( c.b / 255. ), static_cast<float>( c.a / 255. ) },
                                "%s", seg.c_str() );
            GImGui->LastItemData.ID = itemId;
#endif
            if( is_focused && !*is_focused ) {
                *is_focused = ImGui::IsItemFocused();
            }
            if( is_hovered && !*is_hovered ) {
#if defined(TILES) || defined(WIN32)
                *is_hovered = ImGui::IsItemHovered( ImGuiHoveredFlags_NoNavOverride );
#else
                *is_hovered = ImGui::IsItemHovered();
#endif
            }

        }
    }

    ImGui::PopID();
}

bool cataimgui::window::get_is_open() const
{
    return is_open;
}

class cataimgui::window_impl
{
        friend class cataimgui::window;
        cataimgui::window *win_base;
        bool is_resized;
        std::unique_ptr<ui_adaptor> window_adaptor;
    public:
        explicit window_impl( cataimgui::window *win ) {
            win_base = win;
            is_resized = true;
            window_adaptor = std::make_unique<ui_adaptor>();
            window_adaptor->is_imgui = true;
            window_adaptor->on_redraw( [this]( ui_adaptor & ) {
                win_base->draw();
                point catapos;
                point catasize;
                ImVec2 impos = ImGui::GetWindowPos();
                ImVec2 imsize = ImGui::GetWindowSize();
                imvec2_to_point( &impos, &catapos );
                imvec2_to_point( &imsize, &catasize );
                window_adaptor->position( catapos, catasize );
            } );
            window_adaptor->on_screen_resize( [this]( ui_adaptor & ) {
                is_resized = true;
                win_base->on_resized();
            } );
        }
};

cataimgui::window::window( int window_flags )
{
    p_impl = nullptr;

    this->window_flags = window_flags | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavFocus |
                         ImGuiWindowFlags_NoBringToFrontOnFocus;
}

cataimgui::window::window( const std::string &id_, int window_flags ) : window( window_flags )
{
    p_impl = std::make_unique<cataimgui::window_impl>( this );
    id = id_ + "##" + std::to_string( uint64_t( this ) );
    is_open = true;
}


cataimgui::window::~window() = default;

bool cataimgui::window::is_bounds_changed()
{
    return p_impl->is_resized;
}

size_t cataimgui::window::get_text_width( const std::string &text )
{
#if defined(WIN32) || defined(TILES)
    return ImGui::CalcTextSize( text.c_str() ).x;
#else
    return utf8_width( text );
#endif
}

size_t cataimgui::window::get_text_height( const char *text )
{
#if defined(WIN32) || defined(TILES)
    return ImGui::CalcTextSize( "0" ).y * strlen( text );
#else
    return utf8_width( text );
#endif
}

size_t cataimgui::window::str_width_to_pixels( size_t len )
{
#if defined(WIN32) || defined(TILES)
    return ImGui::CalcTextSize( "0" ).x * len;
#else
    return len;
#endif
}

size_t cataimgui::window::str_height_to_pixels( size_t len )
{
#if defined(WIN32) || defined(TILES)
    return ImGui::CalcTextSize( "0" ).y * len;
#else
    return len;
#endif
}

void cataimgui::window::mark_resized()
{
    if( p_impl ) {
        p_impl->is_resized = true;

        p_impl->window_adaptor->mark_resize();
    }
}

void cataimgui::window::draw()
{
    button_action.clear();
    if( !is_open ) {
        return;
    }
    bool handled_resize = false;
    if( is_bounds_changed() ) {
        cached_bounds = get_bounds();
        // we want to make sure is_resized is able to be handled for at least a full frame
        handled_resize = true;
    }
    if( cached_bounds.x == -1 || cached_bounds.y == -1 ) {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        if( cached_bounds.x != -1.f ) {
            center.x = cached_bounds.x;
        }
        if( cached_bounds.y != -1.f ) {
            center.y = cached_bounds.y;
        }
        ImGui::SetNextWindowPos( center, ImGuiCond_Appearing, { cached_bounds.x == -1.f ? 0.5f : 0.f,  cached_bounds.y == -1.f ? 0.5f : 0.f } );
    } else if( cached_bounds.x >= 0 && cached_bounds.y >= 0 ) {
        ImGui::SetNextWindowPos( { cached_bounds.x, cached_bounds.y } );
    }
    if( cached_bounds.h > 0 || cached_bounds.w > 0 ) {
        ImGui::SetNextWindowSize( { cached_bounds.w, cached_bounds.h } );
    }
    if( ImGui::Begin( id.c_str(), &is_open, window_flags ) ) {
        draw_controls();
        if( p_impl->window_adaptor->is_on_top ) {
            ImGui::BringWindowToDisplayFront( ImGui::GetCurrentWindow() );
        }
    }
    ImGui::End();
    if( handled_resize ) {
        p_impl->is_resized = false;
    }
}

/// <summary>
/// A button tied to a string action. When the button is clicked, push the desired action to input_context to be returned the next time
///  there is no input action
/// </summary>
/// <param name="action">The action id to be returned by the button</param>
/// <param name="text">The button's text</param>
/// <returns></returns>
bool cataimgui::window::action_button( const std::string &action, const std::string &text )
{
    if( ImGui::Button( text.c_str() ) ) {
        button_action = action;
        return true;
    }
    return false;
}

bool cataimgui::window::has_button_action()
{
    return !button_action.empty();
}

std::string cataimgui::window::get_button_action()
{
    return button_action;
}

cataimgui::bounds cataimgui::window::get_bounds()
{
    return { -1.f, -1.f, -1.f, -1.f };
}
