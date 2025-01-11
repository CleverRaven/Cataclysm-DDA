#include "loading_ui.h"

#include "cached_options.h"
#include "options.h"
#include "input.h"
#include "output.h"
#include "ui_manager.h"

#if defined(TILES)
#if defined(IMGUI)
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#undef IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "mod_manager.h"
#include "path_info.h"
#include "sdltiles.h"
#include "sdl_wrappers.h"
#include "worldfactory.h"
#else
#include "cursesdef.h"
#endif // TILES

struct ui_state {
    ui_adaptor *ui;
    background_pane *bg;
#ifdef TILES
#if defined(IMGUI)
    ImVec2 window_size;
    ImVec2 splash_size;
#endif
    SDL_Texture_Ptr splash;
    cata_path chosen_load_img;
#else
    size_t splash_width = 0;
    std::vector<std::string> splash;
    std::string blanks;
#endif
    std::string context;
    std::string step;
};

static ui_state *gLUI = nullptr;

static void redraw()
{
#ifdef TILES
#if defined(IMGUI)
    ImVec2 pos = { 0.5f, 0.5f };
    ImGui::SetNextWindowPos( ImGui::GetMainViewport()->Size * pos, ImGuiCond_Always, { 0.5f, 0.5f } );
    ImGui::SetNextWindowSize( gLUI->window_size );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
    ImGui::PushStyleColor( ImGuiCol_WindowBg, { 0.0f, 0.0f, 0.0f, 1.0f } );
    if( ImGui::Begin( "Loading…", nullptr,
                      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings ) ) {
        ImGui::Image( reinterpret_cast<ImTextureID>( gLUI->splash.get() ), gLUI->splash_size );
        float w = ImGui::CalcTextSize( gLUI->context.c_str() ).x + ImGui::CalcTextSize( " " ).x +
                  ImGui::CalcTextSize( gLUI->step.c_str() ).x;
        ImGui::SetCursorPosX( ( ( ImGui::GetWindowWidth() - w ) * 0.5f ) );
        ImGui::Text( "%s %s", gLUI->context.c_str(), gLUI->step.c_str() );
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
#endif
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
#if defined(IMGUI)
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
        if( gLUI->chosen_load_img == cata_path() ) {
            if( imgs.empty() ) {
                gLUI->chosen_load_img = PATH_INFO::gfxdir() / "cdda.png"; //default load screen
            } else {
                gLUI->chosen_load_img = random_entry( imgs );
            }
        }
        SDL_Surface_Ptr surf = load_image( gLUI->chosen_load_img.get_unrelative_path().u8string().c_str() );
        // leave some space for the loading text...
        ImVec2 max_size = ImGui::GetMainViewport()->Size * 0.9;
        // preserve aspect ratio by finding the longest **relative** side and scaling both sides by its ratio to max_size
        // scales both "up" and "down"
        float width_ratio = static_cast<float>( surf->w ) / max_size.x;
        float height_ratio = static_cast<float>( surf->h ) / max_size.y;
        float longest_side_ratio = width_ratio > height_ratio ? width_ratio : height_ratio;
        gLUI->splash_size = { static_cast<float>( surf->w ) / longest_side_ratio,
                              static_cast<float>( surf->h ) / longest_side_ratio
                            };
        gLUI->splash = CreateTextureFromSurface( get_sdl_renderer(), surf );
        gLUI->window_size = gLUI->splash_size + ImVec2{ 0.0f, 2.0f * ImGui::GetTextLineHeightWithSpacing() };
#endif
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
        gLUI->chosen_load_img = cata_path();
#endif
        delete gLUI->ui;
        delete gLUI->bg;
        delete gLUI;
        gLUI = nullptr;
    }
}
