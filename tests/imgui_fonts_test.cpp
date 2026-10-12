#if defined(TILES)
#include <functional>
#include <initializer_list>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "cata_imgui.h"
#include "cata_scope_helpers.h"
#include "font_loader.h"
#include "imgui/imgui.h"
#include "sdltiles.h"

TEST_CASE( "font_config_bitmap_sheets", "[imgui_fonts]" )
{
    CHECK( is_bitmap_typeface( "data/font/map_font_LARWICK.png" ) );
    CHECK( is_bitmap_typeface( "fixedsys.bmp" ) );
    CHECK_FALSE( is_bitmap_typeface( "data/font/Terminus.ttf" ) );
    CHECK_FALSE( is_bitmap_typeface( "unifont" ) );
    CHECK_FALSE( is_bitmap_typeface( "" ) );
}

TEST_CASE( "imgui_fonts_keep_roles", "[imgui_fonts]" )
{
    restore_on_out_of_scope restore_height( fontheight );
    fontheight = 16;
    ImGuiContext *ctx = ImGui::CreateContext();
    on_out_of_scope destroy_ctx( [ctx]() {
        ImGui::DestroyContext( ctx );
    } );
    ImGuiIO &io = ImGui::GetIO();
    const std::vector<font_config> gui = { font_config( "data/font/Roboto-Medium.ttf" ) };
    GIVEN( "mono list starting with a bitmap sheet" ) {
        const std::vector<font_config> mono = {
            font_config( "data/font/map_font_LARWICK.png" ), font_config( "data/font/Terminus.ttf" )
        };
        WHEN( "fonts load without CJK" ) {
            cataimgui::add_cata_fonts( io, gui, mono, false );
            THEN( "gui, mono and 1.5x gui fonts are separate fonts" ) {
                REQUIRE( io.Fonts->Fonts.Size == 3 );
                CHECK( io.Fonts->Fonts[0]->LegacySize == 16.0f );
                CHECK( io.Fonts->Fonts[1]->LegacySize == 16.0f );
                CHECK( io.Fonts->Fonts[2]->LegacySize == 24.0f );
            }
        }
        WHEN( "fonts load with CJK" ) {
            cataimgui::add_cata_fonts( io, gui, mono, true );
            THEN( "gui and mono are separate fonts" ) {
                CHECK( io.Fonts->Fonts.Size == 2 );
            }
        }
    }
    GIVEN( "a list whose first face is missing" ) {
        const std::vector<font_config> mono = {
            font_config( "data/font/no_such_face.ttf" ), font_config( "data/font/Terminus.ttf" )
        };
        THEN( "next face starts the font instead of merging into the previous one" ) {
            cataimgui::add_cata_fonts( io, gui, mono, false );
            CHECK( io.Fonts->Fonts.Size == 3 );
        }
    }
}
TEST_CASE( "imgui_font_reload_lands_at_next_frame", "[imgui_fonts]" )
{
    restore_on_out_of_scope restore_height( fontheight );
    fontheight = 16;
    ImGuiContext *ctx = ImGui::CreateContext();
    on_out_of_scope destroy_ctx( [ctx]() {
        ImGui::DestroyContext( ctx );
    } );
    ImGuiIO &io = ImGui::GetIO();
    const std::vector<font_config> gui = { font_config( "data/font/Roboto-Medium.ttf" ) };
    const std::vector<font_config> mono = { font_config( "data/font/Terminus.ttf" ) };
    cataimgui::add_cata_fonts( io, gui, mono, false );
    cataimgui::font_reload reload;
    GIVEN( "no request" ) {
        THEN( "applying changes nothing" ) {
            CHECK_FALSE( reload.apply( io, false ) );
            CHECK( io.Fonts->Fonts[0]->LegacySize == 16.0f );
        }
    }
    GIVEN( "request at a new cell height" ) {
        fontheight = 24;
        reload.request( gui, mono );
        THEN( "nothing changes until applied" ) {
            CHECK( reload.pending() );
            CHECK( io.Fonts->Fonts[0]->LegacySize == 16.0f );
        }
        WHEN( "applied at the frame boundary" ) {
            REQUIRE( reload.apply( io, false ) );
            THEN( "the three roles come back at the new size, in order" ) {
                REQUIRE( io.Fonts->Fonts.Size == 3 );
                CHECK( io.Fonts->Fonts[0]->LegacySize == 24.0f );
                CHECK( io.Fonts->Fonts[1]->LegacySize == 24.0f );
                CHECK( io.Fonts->Fonts[2]->LegacySize == 36.0f );
                CHECK( ImGui::GetStyle().FontSizeBase == 0.0f );
                CHECK_FALSE( reload.pending() );
            }
        }
    }
    GIVEN( "several requests before a frame" ) {
        fontheight = 20;
        reload.request( gui, mono );
        fontheight = 32;
        reload.request( gui, mono );
        THEN( "one apply lands the last size" ) {
            REQUIRE( reload.apply( io, false ) );
            CHECK( io.Fonts->Fonts.Size == 3 );
            CHECK( io.Fonts->Fonts[0]->LegacySize == 32.0f );
            CHECK_FALSE( reload.apply( io, false ) );
        }
    }
    GIVEN( "repeated reloads" ) {
        THEN( "font count stays three" ) {
            for( int h : {
                     12, 22, 14, 28
                 } ) {
                CAPTURE( h );
                fontheight = h;
                reload.request( gui, mono );
                REQUIRE( reload.apply( io, false ) );
                CHECK( io.Fonts->Fonts.Size == 3 );
                CHECK( io.Fonts->Fonts[1]->LegacySize == static_cast<float>( h ) );
            }
        }
    }
}

TEST_CASE( "imgui_font_reload_crosses_a_frame", "[imgui_fonts]" )
{
    restore_on_out_of_scope restore_height( fontheight );
    fontheight = 16;
    ImGuiContext *ctx = ImGui::CreateContext();
    on_out_of_scope destroy_ctx( [ctx]() {
        ImGui::DestroyContext( ctx );
    } );
    ImGuiIO &io = ImGui::GetIO();
    // as the SDL renderer backend does: the atlas may change between frames
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    io.DisplaySize = ImVec2( 1080.0f, 1920.0f );
    io.DeltaTime = 1.0f / 60.0f;
    const std::vector<font_config> gui = { font_config( "data/font/Roboto-Medium.ttf" ) };
    const std::vector<font_config> mono = { font_config( "data/font/Terminus.ttf" ) };
    cataimgui::add_cata_fonts( io, gui, mono, false );
    cataimgui::font_reload reload;

    ImGui::NewFrame();
    const float old_size = ImGui::GetFontSize();
    const float old_width = ImGui::CalcTextSize( "MMMMMMMMMM" ).x;
    // a size change lands while this frame is open
    fontheight = 24;
    reload.request( gui, mono );
    CHECK( ImGui::GetFontSize() == old_size );
    ImGui::Render();

    REQUIRE( reload.apply( io, false ) );
    ImGui::NewFrame();
    CHECK( ImGui::GetFontSize() == 24.0f );
    CHECK( ImGui::CalcTextSize( "MMMMMMMMMM" ).x > old_width );
    ImGui::Render();
}
#endif
