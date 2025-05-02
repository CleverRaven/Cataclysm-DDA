#include "cata_imgui.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>
#undef IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_freetype.h>

#include "catacharset.h"
#include "cached_options.h"
#include "color.h"
#include "input.h"
#include "output.h"
#include "path_info.h"
#include "system_locale.h"
#include "ui_manager.h"
#include "input_context.h"

static ImGuiKey cata_key_to_imgui( int cata_key );

#ifdef TUI
#include "wcwidth.h"
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

ImVec4 impalette[256] = {};
std::array<RGBTuple, color_loader<RGBTuple>::COLOR_NAMES_COUNT> rgbPalette;
std::array<pairs, 100> colorpairs;   //storage for paired colors

static ImVec4 compute_color( uint8_t index )
{
    if( index < 16 ) {
        RGBTuple &rgbCol = rgbPalette[index];
        return { static_cast<float>( rgbCol.Red ) / 255.0f,
                 static_cast<float>( rgbCol.Green ) / 255.0f,
                 static_cast<float>( rgbCol.Blue ) / 255.0f,
                 1.0f };
    } else if( index < 232 ) {
        static uint8_t colors[6] = {0, 95, 135, 175, 215, 255};
        int i = index - 16;
        int r = i / 36;
        i -= 36 * r;
        int g = i / 6;
        i -= 6 * g;
        int b = i;
        return { static_cast<float>( colors[r] ) / 255.0f,
                 static_cast<float>( colors[g] ) / 255.0f,
                 static_cast<float>( colors[b] ) / 255.0f,
                 1.0f };
    } else {
        static uint8_t gray[24] = {8, 18, 28, 38, 48, 58, 68, 78, 88, 98, 108, 118,
                                   128, 138, 148, 158, 168, 178, 188, 198, 208, 218,
                                   228, 238
                                  };
        float level = static_cast<float>( gray[index - 232] ) / 255.0f;
        return { level, level, level, 1.0f };
    }
}

ImVec4 cataimgui::imvec4_from_color( nc_color &color )
{
    int pair_id = color.get_index();
    pairs &pair = colorpairs[pair_id];

    int palette_index = pair.FG != 0 ? pair.FG : pair.BG;
    if( color.is_bold() ) {
        palette_index += color_loader<RGBTuple>::COLOR_NAMES_COUNT / 2;
    }
    return impalette[palette_index];
}

std::vector<std::pair<int, ImTui::mouse_event>> imtui_events;

static int GetFallbackStrWidth( const char *s_begin, const char *s_end,
                                const float scale )
{
    return utf8_width( std::string( s_begin, s_end ) ) * int( scale );
}

static int GetFallbackCharWidth( ImWchar c, const float scale )
{
    return mk_wcwidth( c ) * scale;
}

cataimgui::client::client()
{
    load_colors();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImTui_ImplNcurses_Init();
    ImTui_ImplText_Init();
    ImGuiIO &io = ImGui::GetIO();

    io.Fonts->Fonts[0]->SetFallbackCharSizeCallback( GetFallbackCharWidth );
    io.Fonts->Fonts[0]->SetFallbackStrSizeCallback( GetFallbackStrWidth );

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().LogFilename = nullptr;
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

    ImTui_ImplText_RenderDrawData( ImGui::GetDrawData() );
    ImTui_ImplNcurses_DrawScreen();
    ImGuiIO &io = ImGui::GetIO();
    for( const int &code : cata_input_trail ) {
        io.AddKeyEvent( cata_key_to_imgui( code ), false );
    }
    cata_input_trail.clear();
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
    if( !any_window_shown() ) {
        return;
    }
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
#if defined(BUTTON5_PRESSED) /* If curses version is prepared for a 5-button mouse, enable mousewheel */
                    case MouseInput::ScrollWheelUp:
                        new_mouse_event.bstate |= BUTTON4_PRESSED;
                        break;
                    case MouseInput::ScrollWheelDown:
                        new_mouse_event.bstate |= BUTTON5_PRESSED;
                        break;
#endif
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
    for( int i = 0; i < 256; i++ ) {
        impalette[i] = compute_color( i );
    }
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
#include "sdl_font.h"
#include "sdltiles.h"
#include "font_loader.h"
#include "wcwidth.h"
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_impl_sdlrenderer2.h>

ImVec4 cataimgui::imvec4_from_color( nc_color &color )
{
    SDL_Color c = curses_color_to_SDL( color );
    return { static_cast<float>( c.r / 255. ),
             static_cast<float>( c.g / 255. ),
             static_cast<float>( c.b / 255. ),
             static_cast<float>( c.a / 255. ) };
}

cataimgui::client::client( const SDL_Renderer_Ptr &sdl_renderer, const SDL_Window_Ptr &sdl_window,
                           const GeometryRenderer_Ptr &sdl_geometry ) :
    sdl_renderer( sdl_renderer ),
    sdl_window( sdl_window ),
    sdl_geometry( sdl_geometry )
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigInputTrickleEventQueue = false;

    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGuiStyle &style = ImGui::GetStyle();
    // Default cellPadding is {4, 2}. We reduce this to {3, 2}.
    ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 3, style.CellPadding.y ) );

    ImGui_ImplSDL2_InitForSDLRenderer( sdl_window.get(), sdl_renderer.get() );
    ImGui_ImplSDLRenderer2_Init( sdl_renderer.get() );
}

// this function QUEUES a character to be drawn
static bool CanRenderFallbackChar( ImWchar wch )
{
    return wch != 0;
}

static int GetFallbackStrWidth( const char *s_begin, const char *s_end,
                                const float scale )
{
    return fontwidth * utf8_width( std::string( s_begin, s_end ) ) * int( scale );
}

static int GetFallbackCharWidth( ImWchar c, const float scale )
{
    return fontwidth * mk_wcwidth( c ) * scale;
}

// NOLINTNEXTLINE(bugprone-suspicious-include)
#include "cldr/imgui-glyph-ranges.cpp"

static void AddGlyphRangesFromCLDR( ImFontGlyphRangesBuilder *b, const std::string &lang )
{
    // NOLINTBEGIN(bugprone-branch-clone)
    if( lang == "en" ) {
        AddGlyphRangesFromCLDRForEN( b );
    } else if( lang == "ar" ) {
        AddGlyphRangesFromCLDRForAR( b );
    } else if( lang == "cs" ) {
        AddGlyphRangesFromCLDRForCS( b );
    } else if( lang == "da" ) {
        AddGlyphRangesFromCLDRForDA( b );
    } else if( lang == "de" ) {
        AddGlyphRangesFromCLDRForDE( b );
    } else if( lang == "el" ) {
        AddGlyphRangesFromCLDRForEL( b );
    } else if( lang == "es_AR" ) {
        AddGlyphRangesFromCLDRForES( b );
    } else if( lang == "es_ES" ) {
        AddGlyphRangesFromCLDRForES( b );
    } else if( lang == "fr" ) {
        AddGlyphRangesFromCLDRForFR( b );
    } else if( lang == "hu" ) {
        AddGlyphRangesFromCLDRForHU( b );
    } else if( lang == "id" ) {
        AddGlyphRangesFromCLDRForID( b );
    } else if( lang == "is" ) {
        AddGlyphRangesFromCLDRForIS( b );
    } else if( lang == "it_IT" ) {
        AddGlyphRangesFromCLDRForIT( b );
    } else if( lang == "ja" ) {
        AddGlyphRangesFromCLDRForJA( b );
    } else if( lang == "ko" ) {
        AddGlyphRangesFromCLDRForKO( b );
    } else if( lang == "nb" ) {
        AddGlyphRangesFromCLDRForNB( b );
    } else if( lang == "nl" ) {
        AddGlyphRangesFromCLDRForNL( b );
    } else if( lang == "pl" ) {
        AddGlyphRangesFromCLDRForPL( b );
    } else if( lang == "pt" ) {
        AddGlyphRangesFromCLDRForPT( b );
    } else if( lang == "pt_BR" ) {
        AddGlyphRangesFromCLDRForPT( b );
    } else if( lang == "ru" ) {
        AddGlyphRangesFromCLDRForRU( b );
    } else if( lang == "sr" ) {
        AddGlyphRangesFromCLDRForSR( b );
    } else if( lang == "tr" ) {
        AddGlyphRangesFromCLDRForTR( b );
    } else if( lang == "uk_UA" ) {
        AddGlyphRangesFromCLDRForUK_UA( b );
    } else if( lang == "zh_CN" ) {
        AddGlyphRangesFromCLDRForZH_HANS( b );
    } else if( lang == "zh_TW" ) {
        AddGlyphRangesFromCLDRForZH_HANT( b );
    }
    // NOLINTEND(bugprone-branch-clone)
}

#if defined(__clang__) || defined(__GNUC__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

static void AddGlyphRangesMisc( UNUSED ImFontGlyphRangesBuilder *b )
{
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static ImWchar superscripts[] = { 0x00B9, 0x00B9, 0x00B2, 0x00B3, 0x2070, 0x208E, 0x0000 };
    b->AddRanges( &superscripts[0] );
}


// Load all fonts that exist in typefaces list
// - typefaces is a list of paths.
static void load_font( ImGuiIO &io, const std::vector<font_config> &typefaces,
                       const ImWchar *ranges )
{
    std::vector<font_config> io_typefaces{ typefaces };
    ensure_unifont_loaded( io_typefaces );

    ImFontConfig config = ImFontConfig();

    bool first = true;
    auto it = std::begin( io_typefaces );
    for( ; it != std::end( io_typefaces ); ++it ) {
        if( !file_exist( it->path ) ) {
            printf( "Font file '%s' does not exist.\n", it->path.c_str() );
        } else {
            config.MergeMode = !first;
            config.FontBuilderFlags = it->imgui_config();
            io.Fonts->AddFontFromFileTTF( it->path.c_str(), fontheight, &config, ranges );
            first = false;
        }
    }
    if( first ) {
        debugmsg( "No fonts were found in the fontdata file." );
    }
}

static void check_font( const ImFont *font )
{
    if( !font || !font->IsLoaded() ) {
        // we can’t use debugmsg or cata_fatal because they trigger a new ImGui frame
        // NOLINTNEXTLINE(cert-err33-c)
        fprintf( stderr,
                 "Failed to create font atlas!  Make sure that your chosen "
                 "font exists, can be read, and has glyphs for your chosen "
                 "language.\n" );
        // NOLINTNEXTLINE(cata-assert)
        std::abort();
    }
}

void cataimgui::client::load_fonts( UNUSED const Font_Ptr &gui_font,
                                    const Font_Ptr &mono_font,
                                    const std::array<SDL_Color, color_loader<SDL_Color>::COLOR_NAMES_COUNT> &windowsPalette,
                                    const std::vector<font_config> &gui_typefaces, const std::vector<font_config> &mono_typefaces )
{
    ImGuiIO &io = ImGui::GetIO();
    if( ImGui::GetIO().FontDefault == nullptr ) {
        for( size_t index = 0; index < color_loader<SDL_Color>::COLOR_NAMES_COUNT; index++ ) {
            SDL_Color sdlCol = windowsPalette[index];
            ImU32 rgb = sdlCol.b << 16 | sdlCol.g << 8 | sdlCol.r;
            sdlColorsToCata[rgb] = index;
        }

        std::string lang = get_option<std::string>( "USE_LANG" );
        if( lang.empty() ) {
            lang = SystemLocale::Language().value_or( "en" );
        }
        ImFontGlyphRangesBuilder b = {};
        b.AddRanges( io.Fonts->GetGlyphRangesDefault() );
        AddGlyphRangesFromCLDR( &b, lang );
        AddGlyphRangesMisc( &b );
        if( get_option<bool>( "IMGUI_LOAD_CHINESE" ) ) {
            b.AddRanges( io.Fonts->GetGlyphRangesChineseFull() );
        }
        ImVector<ImWchar> ranges;
        b.BuildRanges( &ranges );

        load_font( io, gui_typefaces, ranges.begin() );
        load_font( io, mono_typefaces, ranges.begin() );
        io.Fonts->Fonts[0]->SetFallbackStrSizeCallback( GetFallbackStrWidth );
        io.Fonts->Fonts[0]->SetFallbackCharSizeCallback( GetFallbackCharWidth );
        io.Fonts->Fonts[0]->SetRenderFallbackCharCallback( CanRenderFallbackChar );
        io.Fonts->Fonts[1]->SetFallbackStrSizeCallback( GetFallbackStrWidth );
        io.Fonts->Fonts[1]->SetFallbackCharSizeCallback( GetFallbackCharWidth );
        io.Fonts->Fonts[1]->SetRenderFallbackCharCallback( CanRenderFallbackChar );
        io.Fonts->Build();
        check_font( io.Fonts->Fonts[0] );
        check_font( io.Fonts->Fonts[1] );
        ImGui::SetCurrentFont( ImGui::GetDefaultFont() );
        ImGui_ImplSDLRenderer2_SetFallbackGlyphDrawCallback( [&]( const ImFontGlyphToDraw & glyph ) {
            std::string uni_string = std::string( glyph.uni_str );
            point p( int( glyph.pos.x ), int( glyph.pos.y - 3 ) );
            unsigned char col = 0;
            auto it = sdlColorsToCata.find( glyph.col & 0xFFFFFF );
            if( it != sdlColorsToCata.end() ) {
                col = it->second;
            }
            mono_font->OutputChar( sdl_renderer, sdl_geometry, glyph.uni_str, p, col );
        } );
    }
}

cataimgui::client::~client()
{
    ImGui_ImplSDL2_Shutdown();
}

#if 0 and not TUI
struct FreeTypeTest {
    enum FontBuildMode { FontBuildMode_FreeType, FontBuildMode_Stb };

    FontBuildMode   BuildMode = FontBuildMode_FreeType;
    bool            WantRebuild = true;
    float           RasterizerMultiply = 1.0f;
    unsigned int    FreeTypeBuilderFlags = 0;

    // Call _BEFORE_ NewFrame()
    bool PreNewFrame() {
        if( !WantRebuild ) {
            return false;
        }

        ImFontAtlas *atlas = ImGui::GetIO().Fonts;
        for( int n = 0; n < atlas->ConfigData.Size; n++ ) {
            ( static_cast<ImFontConfig *>( &atlas->ConfigData[n] ) )->RasterizerMultiply = RasterizerMultiply;
        }

        // Allow for dynamic selection of the builder.
        // In real code you are likely to just define IMGUI_ENABLE_FREETYPE and never assign to FontBuilderIO.
#ifdef IMGUI_ENABLE_FREETYPE
        if( BuildMode == FontBuildMode_FreeType ) {
            atlas->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
            atlas->FontBuilderFlags = FreeTypeBuilderFlags;
        }
#endif
#ifdef IMGUI_ENABLE_STB_TRUETYPE
        if( BuildMode == FontBuildMode_Stb ) {
            atlas->FontBuilderIO = ImFontAtlasGetBuilderForStbTruetype();
            atlas->FontBuilderFlags = 0;
        }
#endif
        atlas->Build();
        WantRebuild = false;
        return true;
    }

    // Call to draw UI
    void ShowFontsOptionsWindow() {
        ImFontAtlas *atlas = ImGui::GetIO().Fonts;

        ImGui::Begin( "FreeType Options" );
        ImGui::ShowFontSelector( "Fonts" );
        WantRebuild |= ImGui::RadioButton( "FreeType", reinterpret_cast<int *>( &BuildMode ),
                                           FontBuildMode_FreeType );
        ImGui::SameLine();
        WantRebuild |= ImGui::RadioButton( "Stb (Default)", reinterpret_cast<int *>( &BuildMode ),
                                           FontBuildMode_Stb );
        WantRebuild |= ImGui::DragInt( "TexGlyphPadding", &atlas->TexGlyphPadding, 0.1f, 1, 16 );
        WantRebuild |= ImGui::DragFloat( "RasterizerMultiply", &RasterizerMultiply, 0.001f, 0.0f, 2.0f );
        ImGui::Separator();

        if( BuildMode == FontBuildMode_FreeType ) {
#ifndef IMGUI_ENABLE_FREETYPE
            ImGui::TextColored( ImVec4( 1.0f, 0.5f, 0.5f, 1.0f ), "Error: FreeType builder not compiled!" );
#endif
            WantRebuild |= ImGui::CheckboxFlags( "NoHinting", &FreeTypeBuilderFlags,
                                                 ImGuiFreeTypeBuilderFlags_NoHinting );
            WantRebuild |= ImGui::CheckboxFlags( "NoAutoHint", &FreeTypeBuilderFlags,
                                                 ImGuiFreeTypeBuilderFlags_NoAutoHint );
            WantRebuild |= ImGui::CheckboxFlags( "ForceAutoHint", &FreeTypeBuilderFlags,
                                                 ImGuiFreeTypeBuilderFlags_ForceAutoHint );
            WantRebuild |= ImGui::CheckboxFlags( "LightHinting", &FreeTypeBuilderFlags,
                                                 ImGuiFreeTypeBuilderFlags_LightHinting );
            WantRebuild |= ImGui::CheckboxFlags( "MonoHinting", &FreeTypeBuilderFlags,
                                                 ImGuiFreeTypeBuilderFlags_MonoHinting );
            WantRebuild |= ImGui::CheckboxFlags( "Bold", &FreeTypeBuilderFlags,
                                                 ImGuiFreeTypeBuilderFlags_Bold );
            WantRebuild |= ImGui::CheckboxFlags( "Oblique", &FreeTypeBuilderFlags,
                                                 ImGuiFreeTypeBuilderFlags_Oblique );
            WantRebuild |= ImGui::CheckboxFlags( "Monochrome", &FreeTypeBuilderFlags,
                                                 ImGuiFreeTypeBuilderFlags_Monochrome );
        }

        if( BuildMode == FontBuildMode_Stb ) {
#ifndef IMGUI_ENABLE_STB_TRUETYPE
            ImGui::TextColored( ImVec4( 1.0f, 0.5f, 0.5f, 1.0f ), "Error: stb_truetype builder not compiled!" );
#endif
        }
        ImGui::End();
    }
};

FreeTypeTest freetype_test;
#endif

void cataimgui::client::new_frame()
{
#if 0 and not TUI
    if( freetype_test.PreNewFrame() ) {
        // REUPLOAD FONT TEXTURE TO GPU
        ImGui_ImplSDLRenderer2_DestroyDeviceObjects();
        ImGui_ImplSDLRenderer2_CreateDeviceObjects();
    }
#endif
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();

    ImGui::NewFrame();
#if 0 and not TUI
    freetype_test.ShowFontsOptionsWindow();
#endif
}

void cataimgui::client::end_frame()
{
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData( ImGui::GetDrawData(), sdl_renderer.get() );
    ImGuiIO &io = ImGui::GetIO();
    for( const int &code : cata_input_trail ) {
        io.AddKeyEvent( cata_key_to_imgui( code ), false );
    }
    cata_input_trail.clear();
}

void cataimgui::client::process_input( void *input )
{
    if( any_window_shown() ) {
        const SDL_Event *evt = static_cast<const SDL_Event *>( input );
        bool no_mouse = ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NoMouse;
        if( no_mouse ) {
            switch( evt->type ) {
                case SDL_MOUSEMOTION:
                case SDL_MOUSEWHEEL:
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    return;
            }
        }
        ImGui_ImplSDL2_ProcessEvent( evt );
    }
}

#endif

bool cataimgui::client::auto_size_frame_active()
{
    for( const ImGuiWindow *window : GImGui->Windows ) {
        if( ( window->ContentSize.x == 0 || window->ContentSize.y == 0 ) && ( window->AutoFitFramesX > 0 ||
                window->AutoFitFramesY > 0 ) ) {
            return true;
        }
    }
    return false;
}

bool cataimgui::client::any_window_shown()
{
    bool any_window_shown = false;
    for( const ImGuiWindow *window : GImGui->Windows ) {
        if( window->Active && !window->Hidden ) {
            any_window_shown = true;
            break;
        }
    }
    return any_window_shown;
}

static ImGuiKey cata_key_to_imgui( int cata_key )
{
    switch( cata_key ) {
        case KEY_UP:
            return ImGuiKey_UpArrow;
        case KEY_DOWN:
            return ImGuiKey_DownArrow;
        case KEY_LEFT:
            return ImGuiKey_LeftArrow;
        case KEY_RIGHT:
            return ImGuiKey_RightArrow;
        case KEY_ENTER:
            return ImGuiKey_Enter;
        case KEY_ESCAPE:
            return ImGuiKey_Escape;
        default:
            if( cata_key >= 'a' && cata_key <= 'z' ) {
                return static_cast<ImGuiKey>( ImGuiKey_A + ( cata_key - 'a' ) );
            } else if( cata_key >= 'A' && cata_key <= 'Z' ) {
                return static_cast<ImGuiKey>( ImGuiKey_A + ( cata_key - 'A' ) );
            } else if( cata_key >= '0' && cata_key <= '9' ) {
                return static_cast<ImGuiKey>( ImGuiKey_A + ( cata_key - '0' ) );
            }
            return ImGuiKey_None;
    }
}

void cataimgui::client::process_cata_input( const input_event &event )
{
    if( event.type == input_event_t::keyboard_code || event.type == input_event_t::keyboard_char ) {
        int code = event.get_first_input();
        ImGuiIO &io = ImGui::GetIO();
        io.AddKeyEvent( cata_key_to_imgui( code ), true );
        cata_input_trail.push_back( code );
    }
}

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

static void PushOrPopColor( std::string_view seg, int minimumColorStackSize )
{
    color_tag_parse_result tag = get_color_from_tag( seg, report_color_error::yes );
    switch( tag.type ) {
        case color_tag_parse_result::open_color_tag:
            ImGui::PushStyleColor( ImGuiCol_Text, tag.color );
            break;
        case color_tag_parse_result::close_color_tag:
            if( GImGui->ColorStack.Size > minimumColorStackSize ) {
                ImGui::PopStyleColor();
            }
            break;
        case color_tag_parse_result::non_color_tag:
            // Do nothing
            break;
    }
}

/**
 * Scrolls the current ImGui window by a scroll action
 *
 * Setting scroll needs to happen before drawing contents for page scroll to work properly
 * @param s an enum for the currently pending scroll action
 */
void cataimgui::set_scroll( scroll &s )
{
    int scroll_px_begin = ImGui::GetScrollY();
    int scroll_px = 0;
    int line_height = ImGui::GetTextLineHeightWithSpacing();
    int page_height = ImGui::GetWindowSize().y;

    switch( s ) {
        case scroll::none:
            break;
        case scroll::begin:
            scroll_px_begin = 0;
            break;
        case scroll::end:
            scroll_px_begin = ImGui::GetScrollMaxY();
            break;
        case scroll::line_up:
            scroll_px = -line_height;
            break;
        case scroll::line_down:
            scroll_px = line_height;
            break;
        case scroll::page_up:
            scroll_px = -page_height;
            break;
        case scroll::page_down:
            scroll_px = page_height;
            break;
    }

    ImGui::SetScrollY( scroll_px_begin + scroll_px );

    s = scroll::none;
}

void cataimgui::draw_colored_text( const std::string &original_text, const nc_color &color,
                                   float wrap_width, bool *is_selected, bool *is_focused, bool *is_hovered )
{
    nc_color color_cpy = color;
    ImGui::PushStyleColor( ImGuiCol_Text, color_cpy );
    draw_colored_text( original_text, wrap_width, is_selected, is_focused, is_hovered );
    ImGui::PopStyleColor();
}

void cataimgui::draw_colored_text( const std::string &original_text, nc_color &color,
                                   float wrap_width, bool *is_selected, bool *is_focused, bool *is_hovered )
{
    ImGui::PushStyleColor( ImGuiCol_Text, color );
    draw_colored_text( original_text, wrap_width, is_selected, is_focused, is_hovered );
    ImGui::PopStyleColor();
}

void cataimgui::draw_colored_text( const std::string &original_text,
                                   float wrap_width, bool *is_selected, bool *is_focused, bool *is_hovered )
{
    if( original_text.empty() ) {
        ImGui::NewLine();
        return;
    }
    const std::string &text = replace_colors( original_text );

    ImGui::PushID( text.c_str() );
    int startColorStackCount = GImGui->ColorStack.Size;
    ImGuiID itemId = GImGui->CurrentWindow->IDStack.back();

    size_t chars_per_line = size_t( wrap_width );
    if( chars_per_line == 0 ) {
        chars_per_line = SIZE_MAX;
    }
#ifndef TUI
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
                PushOrPopColor( seg, startColorStackCount );
                seg = rm_prefix( seg );
            }

            if( i++ != 0 ) {
                ImGui::SameLine( 0, 0 );
            }
            ImGui::TextUnformatted( seg.c_str() );
            GImGui->LastItemData.ID = itemId;
            if( is_focused && !*is_focused ) {
                *is_focused = ImGui::IsItemFocused();
            }
            if( is_hovered && !*is_hovered ) {
                *is_hovered = GImGui->HoveredId == itemId;
            }

        }
    }
    for( int curColorStackCount = GImGui->ColorStack.Size; curColorStackCount > startColorStackCount;
         curColorStackCount-- ) {
        ImGui::PopStyleColor();
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
            } );
            window_adaptor->on_screen_resize( [this]( ui_adaptor & ) {
                is_resized = true;
                win_base->on_resized();
            } );
        }
};

class cataimgui::filter_box_impl
{
    public:
        std::string text;
        ImGuiID id;
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

cataimgui::window::~window()
{
    p_impl.reset();
    if( GImGui ) {
        ImGui::ClearWindowSettings( id.c_str() );
        if( !ui_adaptor::has_imgui() ) {
            ImGui::GetIO().ClearInputKeys();
            GImGui->InputEventsQueue.resize( 0 );
        }
    }
}

bool cataimgui::window::is_bounds_changed()
{
    return p_impl->is_resized;
}

size_t cataimgui::window::get_text_width( const std::string &text )
{
#ifndef TUI
    return ImGui::CalcTextSize( text.c_str() ).x;
#else
    return utf8_width( text );
#endif
}

size_t cataimgui::window::get_text_height( const char *text )
{
#ifndef TUI
    return ImGui::CalcTextSize( "0" ).y * strlen( text );
#else
    return utf8_width( text );
#endif
}

size_t cataimgui::window::str_width_to_pixels( size_t len )
{
#ifndef TUI
    return ImGui::CalcTextSize( "0" ).x * len;
#else
    return len;
#endif
}

size_t cataimgui::window::str_height_to_pixels( size_t len )
{
#ifndef TUI
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
        ImGui::SetNextWindowPos( center, ImGuiCond_Always, { cached_bounds.x == -1.f ? 0.5f : 0.f,  cached_bounds.y == -1.f ? 0.5f : 0.f } );
    } else if( cached_bounds.x >= 0 && cached_bounds.y >= 0 ) {
        ImGui::SetNextWindowPos( { cached_bounds.x, cached_bounds.y } );
    }
    if( cached_bounds.h > 1.0 || cached_bounds.w > 1.0 ) {
        ImGui::SetNextWindowSize( { cached_bounds.w, cached_bounds.h } );
    } else if( cached_bounds.h > 0.0 && cached_bounds.w > 0.0 && cached_bounds.h <= 1.0 &&
               cached_bounds.w <= 1.0 ) {
        ImGui::SetNextWindowSize( ImGui::GetMainViewport()->Size * ImVec2 { cached_bounds.w, cached_bounds.h } );
    }
    if( ImGui::Begin( id.c_str(), &is_open, window_flags ) ) {
        draw_controls();
        if( p_impl->window_adaptor->is_on_top && !force_to_back ) {
            ImGui::BringWindowToDisplayFront( ImGui::GetCurrentWindow() );
        }
        if( handled_resize ) {
            point catapos;
            point catasize;
            ImVec2 impos = ImGui::GetWindowPos();
            ImVec2 imsize = ImGui::GetWindowSize();
            imvec2_to_point( &impos, &catapos );
            imvec2_to_point( &imsize, &catasize );
            p_impl->window_adaptor->position_absolute( catapos, catasize );
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

void cataimgui::window::draw_filter( const input_context &ctxt, bool filtering_active )
{
    if( !filter_impl ) {
        filter_impl = std::make_unique<cataimgui::filter_box_impl>();
        filter_impl->id = 0;
    }

    if( !filtering_active ) {
        action_button( "FILTER", ctxt.get_button_text( "FILTER" ) );
        ImGui::SameLine();
        action_button( "RESET_FILTER", ctxt.get_button_text( "RESET_FILTER" ) );
        ImGui::SameLine();
    } else {
        action_button( "QUIT", ctxt.get_button_text( "QUIT", _( "Cancel" ) ) );
        ImGui::SameLine();
        action_button( "TEXT.CONFIRM", ctxt.get_button_text( "TEXT.CONFIRM", _( "OK" ) ) );
        ImGui::SameLine();
    }
    ImGui::BeginDisabled( !filtering_active );
    ImGui::InputText( "##FILTERBOX", &filter_impl->text );
    ImGui::EndDisabled();
    if( !filter_impl->id ) {
        filter_impl->id = GImGui->LastItemData.ID;
    }
}

std::string cataimgui::window::get_filter()
{
    if( filter_impl ) {
        return filter_impl->text;
    } else {
        return std::string();
    }
}

void cataimgui::window::clear_filter()
{
    if( filter_impl && filter_impl->id != 0 ) {
        ImGuiInputTextState *input_state = ImGui::GetInputTextState( filter_impl->id );
        if( input_state ) {
            input_state->ClearText();
            filter_impl->text.clear();
        }
    }
}

bool cataimgui::InputFloat( const char *label, float *v, float step, float step_fast,
                            const char *format, ImGuiInputTextFlags flags )
{
    return ImGui::InputScalar( label, ImGuiDataType_Float, static_cast<void *>( v ),
                               static_cast<void *>( step > 0.0f ? &step : nullptr ),
                               static_cast<void *>( step_fast > 0.0f ? &step_fast : nullptr ), format, flags );
}

void cataimgui::PushGuiFont()
{
#ifdef TILES
    ImGui::PushFont( ImGui::GetIO().Fonts->Fonts[0] );
#endif
}

void cataimgui::PushMonoFont()
{
#ifdef TILES
    ImGui::PushFont( ImGui::GetIO().Fonts->Fonts[1] );
#endif
}

bool cataimgui::BeginRightAlign( const char *str_id )
{
    if( ImGui::BeginTable( str_id, 2, ImGuiTableFlags_SizingFixedFit, ImVec2( -1, 0 ) ) ) {
        ImGui::TableSetupColumn( "a", ImGuiTableColumnFlags_WidthStretch );

        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        return true;
    }
    return false;
}

void cataimgui::EndRightAlign()
{
    ImGui::EndTable();
}

// Use the base terminal palette to reasonably color ImGui elements.
// This might be useful to easily apply the color theme (chosen via
// Color Manager, likely) to ImGui with minimal effort.
static void inherit_base_colors()
{
    ImGuiStyle &style = ImGui::GetStyle();

    style.Colors[ImGuiCol_Text] = c_white;
    style.Colors[ImGuiCol_TextDisabled] = c_unset;
    style.Colors[ImGuiCol_WindowBg] = c_black;
    style.Colors[ImGuiCol_ChildBg] = c_black;
    style.Colors[ImGuiCol_PopupBg] = c_black;
    style.Colors[ImGuiCol_Border] = c_white;
    style.Colors[ImGuiCol_BorderShadow] = c_blue;
    style.Colors[ImGuiCol_FrameBg] = c_dark_gray;
    style.Colors[ImGuiCol_FrameBgHovered] = c_black;
    style.Colors[ImGuiCol_FrameBgActive] = c_dark_gray;
    style.Colors[ImGuiCol_TitleBg] = c_dark_gray;
    style.Colors[ImGuiCol_TitleBgActive] = c_black;
    style.Colors[ImGuiCol_TitleBgCollapsed] = c_dark_gray;
    style.Colors[ImGuiCol_MenuBarBg] = c_black;
    style.Colors[ImGuiCol_ScrollbarBg] = c_black;
    style.Colors[ImGuiCol_ScrollbarGrab] = c_dark_gray;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = c_light_gray;
    style.Colors[ImGuiCol_ScrollbarGrabActive] = c_white;
    style.Colors[ImGuiCol_CheckMark] = c_white;
    style.Colors[ImGuiCol_SliderGrab] = c_white;
    style.Colors[ImGuiCol_SliderGrabActive] = c_white;
    style.Colors[ImGuiCol_Button] = c_dark_gray;
    style.Colors[ImGuiCol_ButtonHovered] = c_dark_gray;
    style.Colors[ImGuiCol_ButtonActive] = c_blue;
    style.Colors[ImGuiCol_Header] = h_blue;
    style.Colors[ImGuiCol_HeaderHovered] = c_black;
    style.Colors[ImGuiCol_HeaderActive] = c_dark_gray;
    style.Colors[ImGuiCol_Separator] = c_dark_gray;
    style.Colors[ImGuiCol_SeparatorHovered] = c_white;
    style.Colors[ImGuiCol_SeparatorActive] = c_white;
    style.Colors[ImGuiCol_ResizeGrip] = c_light_gray;
    style.Colors[ImGuiCol_ResizeGripHovered] = c_white;
    style.Colors[ImGuiCol_ResizeGripActive] = c_white;
    style.Colors[ImGuiCol_Tab] = c_black;
    style.Colors[ImGuiCol_TabHovered] = c_blue;
    style.Colors[ImGuiCol_TabActive] = c_blue;
    style.Colors[ImGuiCol_TabUnfocused] = c_black;
    style.Colors[ImGuiCol_TabUnfocusedActive] = c_black;
    style.Colors[ImGuiCol_TextSelectedBg] = c_blue;
    style.Colors[ImGuiCol_NavHighlight] = c_blue;
}

static void load_imgui_style_file( const cata_path &style_path )
{
    ImGuiStyle &style = ImGui::GetStyle();

    JsonValue jsin = json_loader::from_path( style_path );


    JsonObject jo = jsin.get_object();


    if( jo.has_bool( "inherit_base_colors" ) && jo.get_bool( "inherit_base_colors" ) ) {
        inherit_base_colors();
    }
    JsonObject joc = jo.get_object( "colors" );

    std::unordered_map<std::string, int> key_options = {
        {"ImGuiCol_Text", ImGuiCol_Text},
        {"ImGuiCol_TextDisabled", ImGuiCol_TextDisabled},
        {"ImGuiCol_WindowBg", ImGuiCol_WindowBg},
        {"ImGuiCol_ChildBg", ImGuiCol_ChildBg},
        {"ImGuiCol_PopupBg", ImGuiCol_PopupBg},
        {"ImGuiCol_Border", ImGuiCol_Border},
        {"ImGuiCol_BorderShadow", ImGuiCol_BorderShadow},
        {"ImGuiCol_FrameBg", ImGuiCol_FrameBg},
        {"ImGuiCol_FrameBgHovered", ImGuiCol_FrameBgHovered},
        {"ImGuiCol_FrameBgActive", ImGuiCol_FrameBgActive},
        {"ImGuiCol_TitleBg", ImGuiCol_TitleBg},
        {"ImGuiCol_TitleBgActive", ImGuiCol_TitleBgActive},
        {"ImGuiCol_TitleBgCollapsed", ImGuiCol_TitleBgCollapsed},
        {"ImGuiCol_MenuBarBg", ImGuiCol_MenuBarBg},
        {"ImGuiCol_ScrollbarBg", ImGuiCol_ScrollbarBg},
        {"ImGuiCol_ScrollbarGrab", ImGuiCol_ScrollbarGrab},
        {"ImGuiCol_ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered},
        {"ImGuiCol_ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive},
        {"ImGuiCol_CheckMark", ImGuiCol_CheckMark},
        {"ImGuiCol_SliderGrab", ImGuiCol_SliderGrab},
        {"ImGuiCol_SliderGrabActive", ImGuiCol_SliderGrabActive},
        {"ImGuiCol_Button", ImGuiCol_Button},
        {"ImGuiCol_ButtonHovered", ImGuiCol_ButtonHovered},
        {"ImGuiCol_ButtonActive", ImGuiCol_ButtonActive},
        {"ImGuiCol_Header", ImGuiCol_Header},
        {"ImGuiCol_HeaderHovered", ImGuiCol_HeaderHovered},
        {"ImGuiCol_HeaderActive", ImGuiCol_HeaderActive},
        {"ImGuiCol_Separator", ImGuiCol_Separator},
        {"ImGuiCol_SeparatorHovered", ImGuiCol_SeparatorHovered},
        {"ImGuiCol_SeparatorActive", ImGuiCol_SeparatorActive},
        {"ImGuiCol_ResizeGrip", ImGuiCol_ResizeGrip},
        {"ImGuiCol_ResizeGripHovered", ImGuiCol_ResizeGripHovered},
        {"ImGuiCol_ResizeGripActive", ImGuiCol_ResizeGripActive},
        {"ImGuiCol_Tab", ImGuiCol_Tab},
        {"ImGuiCol_TabHovered", ImGuiCol_TabHovered},
        {"ImGuiCol_TabActive", ImGuiCol_TabActive},
        {"ImGuiCol_TabUnfocused", ImGuiCol_TabUnfocused},
        {"ImGuiCol_TabUnfocusedActive", ImGuiCol_TabUnfocusedActive},
        {"ImGuiCol_PlotLines", ImGuiCol_PlotLines},
        {"ImGuiCol_PlotLinesHovered", ImGuiCol_PlotLinesHovered},
        {"ImGuiCol_PlotHistogram", ImGuiCol_PlotHistogram},
        {"ImGuiCol_PlotHistogramHovered", ImGuiCol_PlotHistogramHovered},
        {"ImGuiCol_TableHeaderBg", ImGuiCol_TableHeaderBg},
        {"ImGuiCol_TableBorderStrong", ImGuiCol_TableBorderStrong},
        {"ImGuiCol_TableBorderLight", ImGuiCol_TableBorderLight},
        {"ImGuiCol_TableRowBg", ImGuiCol_TableRowBg},
        {"ImGuiCol_TableRowBgAlt", ImGuiCol_TableRowBgAlt},
        {"ImGuiCol_TextSelectedBg", ImGuiCol_TextSelectedBg},
        {"ImGuiCol_DragDropTarget", ImGuiCol_DragDropTarget},
        {"ImGuiCol_NavHighlight", ImGuiCol_NavHighlight},
        {"ImGuiCol_NavWindowingHighlight", ImGuiCol_NavWindowingHighlight},
        {"ImGuiCol_NavWindowingDimBg", ImGuiCol_NavWindowingDimBg},
        {"ImGuiCol_ModalWindowDimBg", ImGuiCol_ModalWindowDimBg},
    };
    for( const auto& [text_key, imgui_key] : key_options ) {
        if( joc.has_array( text_key ) ) {
            JsonArray jsarr = joc.get_array( text_key );
            float alpha = 1.0; // default to full opacity if not specified explicitly
            if( jsarr.has_float( 3 ) ) {
                alpha = jsarr.get_float( 3 );
            }
            ImVec4 color = ImVec4(
                               jsarr.get_float( 0 ),
                               jsarr.get_float( 1 ),
                               jsarr.get_float( 2 ),
                               alpha
                           );
            style.Colors[imgui_key] = color;
        }
    }

}

void cataimgui::init_colors()
{
    const cata_path default_style_path = PATH_INFO::datadir_path() / "raw" / "imgui_style.json";
    const cata_path style_path = PATH_INFO::config_dir_path() / "imgui_style.json";
    if( !file_exist( style_path ) ) {
        assure_dir_exist( PATH_INFO::config_dir() );
        copy_file( default_style_path, style_path );
    }

    try {
        load_imgui_style_file( style_path );
    } catch( const JsonError &err ) {
        debugmsg( "Failed to load imgui color data from \"%s\": %s",
                  style_path.generic_u8string(), err.what() );
    }
}

