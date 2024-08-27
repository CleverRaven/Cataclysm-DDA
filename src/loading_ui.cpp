#include "loading_ui.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#undef IMGUI_DEFINE_MATH_OPERATORS
#include "input.h"
#include "output.h"
#include "path_info.h"
#include "ui_manager.h"

#if defined(TILES)
#include "sdltiles.h"
#include "sdl_wrappers.h"
#endif // TILES

struct ui_state {
    ui_adaptor *ui;
    background_pane *bg;
    ImVec2 window_size;
    ImVec2 splash_size;
    SDL_Texture_Ptr splash;
    std::string context;
    std::string step;
};

ui_state *gLUI = nullptr;

void redraw()
{
    ImVec2 pos = { 0.5f, 0.5f };
    ImGui::SetNextWindowPos( ImGui::GetMainViewport()->Size * pos, ImGuiCond_Always, { 0.5f, 0.5f } );
    ImGui::SetNextWindowSize( gLUI->window_size );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
    ImGui::PushStyleColor( ImGuiCol_WindowBg, { 0.0f, 0.0f, 0.0f, 1.0f } );
    if( ImGui::Begin( "Loading…", NULL,
                      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings ) ) {
        ImGui::Image(static_cast<void*>(gLUI->splash.get()), gLUI->splash_size);
        ImGui::SetCursorPosX( ( gLUI->splash_size.x / 2.0f ) - 120.0f );
        ImGui::TextUnformatted( gLUI->context.c_str() );
        ImGui::SameLine();
        ImGui::TextUnformatted( gLUI->step.c_str() );
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void resize()
{
}

void update_state( const std::string &context, const std::string &step )
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

        cata_path path = PATH_INFO::gfxdir() / "cdda.avif";
        SDL_Surface_Ptr surf = load_image( path.get_unrelative_path().u8string().c_str() );
        gLUI->splash_size = { static_cast<float>(surf->w), static_cast<float>(surf->h) };
        gLUI->splash = CreateTextureFromSurface( get_sdl_renderer(), surf );
        gLUI->window_size = gLUI->splash_size + ImVec2{ 0.0f, 2.0f*ImGui::GetTextLineHeightWithSpacing() };
    }
    gLUI->context = std::string( context );
    gLUI->step = std::string( step );
}

void loading_ui::show( const std::string &context, const std::string &step )
{
    update_state( context, step );
    ui_manager::redraw();
    refresh_display();
    inp_mngr.pump_events();
}

void loading_ui::done()
{
    if( gLUI != nullptr ) {
        delete gLUI->ui;
        delete gLUI->bg;
        delete gLUI;
        gLUI = nullptr;
    }
}
