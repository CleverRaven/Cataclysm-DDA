#include "loading_ui.h"

#include "cached_options.h"
#include "input.h"
#include "options.h"
#include "output.h"
#include "text_snippets.h"
#include "ui_manager.h"

#if defined(TILES)
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#undef IMGUI_DEFINE_MATH_OPERATORS
#include "mod_manager.h"
#include "path_info.h"
#include "sdltiles.h"
#include "sdl_wrappers.h"
#include "cata_imgui.h"
#include "text_snippets.h"
#include "worldfactory.h"
#else
#include "cursesdef.h"
#endif // TILES

struct ui_state {
    ui_adaptor *ui;
    background_pane *bg;
#ifdef TILES
    ImVec2 window_size;
    ImVec2 splash_size;
    SDL_Texture_Ptr splash;
    cata_path chosen_load_img;
#else
    size_t splash_width = 0;
    std::vector<std::string> splash;
    std::string blanks;
#endif
    float hint_height;
    float loading_msg_height;
    // height of all the text that will be shown below the splash screen, tip + loading msg
    float text_height;
    // if screen resolution is large enough, we will increase the size of snippet text by 1.5x
    bool large_hint_size;
    // actual tip we generated from the tip list
    std::string tip;
    std::string context;
    std::string step;
};

static ui_state *gLUI = nullptr;

static void redraw()
{
#ifdef TILES
    ImGui::SetNextWindowSize( ImGui::GetMainViewport()->Size );
    ImGui::SetNextWindowPos( {0, 0}, ImGuiCond_Always );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
    if( ImGui::Begin( "Loading…", nullptr,
                      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings ) ) {

        const float width = ImGui::GetContentRegionAvail().x;

        // image
        const float center_x = ImGui::GetMainViewport()->Size.x / 2;
        const float image_start_pos_x = center_x - ( gLUI->splash_size.x / 2 );
        ImGui::SetCursorPosX( image_start_pos_x );
        ImGui::Image( reinterpret_cast<ImTextureID>( gLUI->splash.get() ), gLUI->splash_size );

        // hint
        ImGui::SetCursorPosY( ImGui::GetWindowHeight() - gLUI->text_height );
        if( gLUI->large_hint_size ) {
            cataimgui::PushGuiFont1_5x();
        }
        const float tip_width = cataimgui::get_string_width( gLUI->tip );
        // center align
        ImGui::SetCursorPosX( std::max( ( width / 2.f ) - ( tip_width / 2.f ),
                                        ImGui::GetStyle().WindowPadding.x ) );
        const std::string hint = string_format( "%s", colorize( gLUI->tip, c_cyan ) );

        cataimgui::draw_colored_text( hint, width );
        if( gLUI->large_hint_size ) {
            cataimgui::PopGuiFont1_5x();
        }

        // loading msg
        ImGui::SetCursorPosY( ImGui::GetWindowHeight() - gLUI->loading_msg_height );
        // somewhat arbitrary centering of loading msg
        // hardcoded so it will not jump around depending on the step
        const float loading_msg_pos_x = center_x - cataimgui::get_string_width( "Loading files: " );
        ImGui::SetCursorPosX( loading_msg_pos_x );
        const std::string loading_msg = string_format( "%s %s", gLUI->context, gLUI->step );
        cataimgui::draw_colored_text( loading_msg, ImGui::GetContentRegionAvail().x );
    }
    ImGui::End();
    ImGui::PopStyleVar();

#else
    int x = ( TERMX - static_cast<int>( gLUI->splash_width ) ) / 2;
    int y = 0;
    nc_color white = c_white;
    for( const std::string &line : gLUI->splash ) {
        if( !line.empty() && line[0] == '#' ) {
            continue;
        }
        print_colored_text( catacurses::stdscr, point( x, y++ ), white,
                            white, line );
    }
    mvwprintz( catacurses::stdscr, point( 0, TERMY - 1 ), c_black, gLUI->blanks );
    center_print( catacurses::stdscr, TERMY - 1, c_white, string_format( "%s %s",
                  gLUI->context.c_str(), gLUI->step.c_str() ) );
#endif
}

static void resize()
{
}

static void update_state( const std::string &context, const std::string &step )
{
    if( gLUI == nullptr ) {
        gLUI = new struct ui_state;
        gLUI->bg = new background_pane;
        gLUI->ui = new ui_adaptor;
        gLUI->ui->is_imgui = true;
        gLUI->ui->on_redraw( []( ui_adaptor & ) {
            redraw();
        } );
        gLUI->ui->on_screen_resize( []( ui_adaptor & ) {
            resize();
        } );

#ifdef TILES
        // padding will eat some of it, but we cannot access padding without imgui window
        // so eyeball a bit off the main viewport to compensate
        const ImVec2 screen_size = ImGui::GetMainViewport()->Size * 0.98f;

        // get snippet text and calculate it's size ahead of time
        gLUI->tip = SNIPPET.random_from_category( "tip" ).value_or(
                        translation() ).translated();

        // if our screen space is at least twice as large as minimal, we will use 1.5x font size
        gLUI->large_hint_size = cataimgui::min_screen_res_y * 2.f < screen_size.y;

        // calculate hint_height maybe with increased font
        // ideally it would use proper ImGui::PopFont(), but it requires imgui window, which we do not have here
        // so we just eyeball it to be 1.6x bigger than normal-sized string
        if( gLUI->large_hint_size ) {
            gLUI->hint_height = cataimgui::get_string_height( gLUI->tip, screen_size.x ) * 1.6f;
        } else {
            gLUI->hint_height = cataimgui::get_string_height( gLUI->tip, screen_size.x );
        }

        // no matter the size, loading message always fit in 1 line,
        // so just use dummy line to calculate it's height
        gLUI->loading_msg_height = cataimgui::get_string_height( " ", 0 );

        gLUI->text_height = gLUI->hint_height + gLUI->loading_msg_height;

        // get image
        std::vector<cata_path> imgs;
        std::vector<mod_id> &active_mod_list = world_generator->active_world->active_mod_order;
        for( mod_id &some_mod : active_mod_list ) {
            // We haven't migrated mods yet
            // TODO: Move mod migration before this function?
            if( !some_mod.is_valid() ) {
                continue;
            }
            const MOD_INFORMATION &mod = *some_mod;
            for( const std::string &img_name : mod.loading_images ) {
                // There may be more than one file matching the name, so we need to get all of them
                for( cata_path &img_path : get_files_from_path( img_name, mod.path, true ) ) {
                    imgs.emplace_back( img_path );
                }
            }
        }
        // We haven't selected a loading image yet, let's do so.
        if( gLUI->chosen_load_img == cata_path() ) {
            if( imgs.empty() ) { // No modded screens available, so let's consider the vanilla ones.
                const std::string img_name_pattern = "loading_img";
                const cata_path load_screens_directory = PATH_INFO::gfxdir() / "loading_screens";
                for( cata_path &img_path : get_files_from_path( img_name_pattern, load_screens_directory, true ) ) {
                    imgs.emplace_back( img_path );
                }
            }
            gLUI->chosen_load_img = random_entry( imgs );
        }
        SDL_Surface_Ptr surf = load_image( gLUI->chosen_load_img.get_unrelative_path().u8string().c_str() );
        // calculate max size of image, decreasing it by the size of text below
        const ImVec2 max_img_size = { screen_size.x, screen_size.y - gLUI->text_height };
        // preserve aspect ratio by finding the longest **relative** side and scaling both sides by its ratio to max_img_size
        // scales both "up" and "down"
        float width_ratio = static_cast<float>( surf->w ) / max_img_size.x;
        float height_ratio = static_cast<float>( surf->h ) / max_img_size.y;
        float longest_side_ratio = width_ratio > height_ratio ? width_ratio : height_ratio;
        gLUI->splash_size = { static_cast<float>( surf->w ) / longest_side_ratio,
                              static_cast<float>( surf->h ) / longest_side_ratio
                            };
        gLUI->splash = CreateTextureFromSurface( get_sdl_renderer(), surf );
        gLUI->window_size = gLUI->splash_size + ImVec2{ 0.0f, 2.0f * ImGui::GetTextLineHeightWithSpacing() };
#else
        std::string splash = PATH_INFO::title( get_holiday_from_time() );
        if( get_option<bool>( "ENABLE_ASCII_TITLE" ) ) {
            splash = read_whole_file( splash ).value_or( _( "Cataclysm: Dark Days Ahead" ) );
        }
        gLUI->splash = string_split( splash, '\n' );
        gLUI->blanks = std::string( TERMX, ' ' );
        for( const std::string &line : gLUI->splash ) {
            if( !line.empty() && line[0] == '#' ) {
                continue;
            }
            gLUI->splash_width = std::max( gLUI->splash_width, remove_color_tags( line ).length() );
        }
#endif
    }
    gLUI->context = std::string( context );
    gLUI->step = std::string( step );
}

void loading_ui::show( const std::string &context, const std::string &step )
{
    if( test_mode ) {
        return;
    }
    update_state( context, step );
    ui_manager::redraw();
    refresh_display();
    inp_mngr.pump_events();
}

void loading_ui::done()
{
    if( gLUI != nullptr ) {
#ifdef TILES
        // Need to manually clear the screen due to using direct ImGui calls
        clear_sdl_window();
#endif
        delete gLUI->ui;
        delete gLUI->bg;
        delete gLUI;
        gLUI = nullptr;
    }
}
