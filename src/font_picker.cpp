#ifndef IMTUI
#include "cata_imgui.h"
#include "cursesport.h"
#include "font_loader.h"
#include "font_picker.h"
#include "input_context.h"
#include "options.h"
#include "sdltiles.h"
#include "translations.h"
#include "ui_manager.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>
#undef IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_freetype.h>

#include <CustomFont/CustomFont.h>

#include <map>
#include <vector>

namespace FontPicker
{
namespace fs = std::filesystem;

static bool WantRebuild = false;
static std::vector<const char *> hinting_types;
static std::map<const char *, std::string> localized_hinting_types;
static bool gui_font_detail_window_open;
static bool mono_font_detail_window_open;

static void ShowFontOptions( const char *label, const char *window_name, bool *detail_window_open,
                             ImFont *imfont, std::vector<font_config> &faces );
static void ShowFontDetailsWindow( const char *window_name, std::vector<font_config> &typefaces,
                                   bool *p_open );
static bool ShowFaceDetail( font_config &face, bool is_only_face );

class FontPickerWindow : public cataimgui::window
{
    public:
        explicit FontPickerWindow();
    protected:
        cataimgui::bounds get_bounds() override;
        void draw_controls() override;
};

FontPickerWindow::FontPickerWindow() : cataimgui::window(
        _( "Font Settings" ) )
{
    force_to_back = true;
    window_flags = ImGuiWindowFlags_None;
}

cataimgui::bounds FontPickerWindow::get_bounds()
{
    return {0.0f, -1.0f, -1.0f, -1.0f};
}

static void HelpMarker( const char *desc )
{
    ImGui::SameLine( 0.0, 0.0 );
    ImGui::TextDisabled( "(?)" );
    if( ImGui::BeginItemTooltip() ) {
        ImGui::PushTextWrapPos( ImGui::GetFontSize() * 35.0f );
        ImGui::TextUnformatted( desc );
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void FontPickerWindow::draw_controls()
{
    WantRebuild |= ImGui::SliderInt( _( "GUI Font Size" ), &font_loader.gui_fontsize, 6, 48,
                                     "%d px" );
    HelpMarker( _( "Font size used by new ImGui‐based windows." ) );
    WantRebuild |= ImGui::SliderInt( _( "Mono Font Width" ), &fontwidth, 6,
                                     get_maximum_font_width(), "%d px" );
    HelpMarker( _( "Width of the character cell for the primary monospaced font." ) );
    WantRebuild |= ImGui::SliderInt( _( "Mono Font Height" ), &fontheight, 6,
                                     get_maximum_font_height(), "%d px" );
    HelpMarker( _( "Height of the character cell for the primary monospaced font." ) );
    WantRebuild |= ImGui::SliderInt( _( "Mono Font Size" ), &font_loader.fontsize, 6, 48,
                                     "%d px" );
    HelpMarker(
        _( "Size of the font drawn inside the character cell of the primary monospaced font." ) );
    WantRebuild |= ImGui::SliderInt( _( "Map Font Width" ), &font_loader.map_fontwidth, 6, 48,
                                     "%d px" );
    HelpMarker( _( "Width of the character cell for the overlay text on the main map." ) );
    WantRebuild |= ImGui::SliderInt( _( "Map Font Height" ), &font_loader.map_fontheight, 6, 48,
                                     "%d px" );
    HelpMarker( _( "Height of the character cell for the overlay text on the main map." ) );
    WantRebuild |= ImGui::SliderInt( _( "Map Font Size" ), &font_loader.map_fontsize, 6, 48,
                                     "%d px" );
    HelpMarker(
        _( "Size of the font drawn inside the character cell of the overlay text on the main map." ) );
    WantRebuild |= ImGui::SliderInt( _( "Overmap Font Width" ), &font_loader.overmap_fontwidth,
                                     6, 48, "%d px" );
    HelpMarker(
        _( "Width of the character cell used to display the world map, unless you use a tileset for that." ) );
    WantRebuild |= ImGui::SliderInt( _( "Overmap Font Height" ),
                                     &font_loader.overmap_fontheight, 6, 48, "%d px" );
    HelpMarker(
        _( "Height of the character cell used to display the world map, unless you use a tileset for that." ) );
    WantRebuild |= ImGui::SliderInt( _( "Overmap Font Size" ), &font_loader.overmap_fontsize, 6,
                                     48, "%d px" );
    HelpMarker(
        _( "Size of the font drawn insidet used to display the world map, unless you use a tileset for that." ) );

    ImGui::Separator();
    auto imfonts = ImGui::GetIO().Fonts->Fonts;
    ShowFontOptions( _( "GUI Font" ), _( "GUI Font Fallback Order" ), &gui_font_detail_window_open,
                     imfonts[0], font_loader.gui_typeface );
    ImGui::Separator();
    ShowFontOptions( _( "Monospace Font" ), _( "Monospace Font Fallback Order" ),
                     &mono_font_detail_window_open, imfonts[1], font_loader.typeface );
}

bool PreNewFrame()
{
    if( !WantRebuild ) {
        // no changes were made, but we want to redraw everything
        // anyway so that our tooltips are erased
        ui_manager::invalidate_all_ui_adaptors();
        return false;
    }
    WantRebuild = false;
    get_options().get_option( "GUI_FONT_SIZE" ).setValue( font_loader.gui_fontsize );
    get_options().get_option( "FONT_WIDTH" ).setValue( font_loader.fontwidth = fontwidth );
    get_options().get_option( "FONT_HEIGHT" ).setValue( font_loader.fontheight = fontheight );
    get_options().get_option( "FONT_SIZE" ).setValue( font_loader.fontsize );
    get_options().get_option( "MAP_FONT_WIDTH" ).setValue( font_loader.map_fontwidth );
    get_options().get_option( "MAP_FONT_HEIGHT" ).setValue( font_loader.map_fontheight );
    get_options().get_option( "MAP_FONT_SIZE" ).setValue( font_loader.map_fontsize );
    get_options().get_option( "OVERMAP_FONT_WIDTH" ).setValue( font_loader.overmap_fontwidth );
    get_options().get_option( "OVERMAP_FONT_HEIGHT" ).setValue( font_loader.overmap_fontheight );
    get_options().get_option( "OVERMAP_FONT_SIZE" ).setValue( font_loader.overmap_fontsize );
    get_options().save();
    font_loader.save( PATH_INFO::fontdata() );
    handle_font_size_change();
    return true;
}

// Call to draw UI
void ShowFontsOptionsWindow()
{
    if( hinting_types.empty() ) {
        hinting_types = { "Default", "Auto", "NoAuto", "Light", "None", "Bitmap", };
        localized_hinting_types = { { "Default", _( "Default" ) },
            { "Auto", _( "Auto" ) },
            { "NoAuto", _( "NoAuto" ) },
            { "Light", _( "Light" ) },
            { "None", _( "None" ) },
            { "Bitmap", _( "Bitmap" ) },
        };
    }
    gui_font_detail_window_open = false;
    mono_font_detail_window_open = false;

    std::unique_ptr<FontPickerWindow> win = std::make_unique<FontPickerWindow>();
    input_context ctxt = input_context();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    bool loop = true;
    do {
        std::string act = ctxt.handle_input( 1 );
        if( act == "QUIT" ) {
            // TODO(db48x): this closes all the windows at once, which is a bit jarring.
            loop = false;
        } else if( act == "TIMEOUT" ) {
            loop = true;
        }
        ui_manager::redraw();
    } while( loop && win->get_is_open() );
}

static void ShowFontOptions( const char *label, const char *window_name,
                             bool *detail_window_open, ImFont *imfont,
                             std::vector<font_config> &faces )
{
    auto &face = faces[0];
    ImGui::PushID( &faces );
    ImGui::TextUnformatted( label );
    ImGui::BeginDisabled( true );
    if( ImGui::Button( ICON_IGFD_EDIT ) ) {
    }
    ImGui::SetItemTooltip(
        // NOLINTNEXTLINE(cata-text-style) not shown in a monospaced font
        _( "Not yet implemented. In the mean time you can edit ‘%s’ to pick any font you want." ),
        fs::path( PATH_INFO::fontdata() ).c_str() );
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    fs::path filename = fs::u8path( face.path ).filename();
    ImGui::TextUnformatted( filename );
    ImGui::SameLine();
    if( cataimgui::BeginRightAlign( "x" ) ) {
        if( ImGui::Button( _( "Details" ) ) ) {
            *detail_window_open = true;
        }
        ShowFontDetailsWindow( window_name, faces, detail_window_open );
        cataimgui::EndRightAlign();
    }
    ImGui::Text( "%s: ", _( "Sample" ) );
    ImGui::SameLine();
    ImGui::PushFont( imfont );
    ImGui::TextUnformatted( _( "The quick brown fox jumped over the lazy dog." ) );
    ImGui::PopFont();
    ImGui::PopID();
}

static void ShowFontDetailsWindow( const char *window_name,
                                   std::vector<font_config> &typefaces, bool *p_open )
{
    if( !*p_open ) {
        return;
    }
    ImGui::SetNextWindowSize( {640.0, 480.0} );
    if( ImGui::Begin( window_name, p_open ) ) {
        font_config *face_to_remove = nullptr;
        static font_config *drag_source = nullptr;
        static font_config *drop_target = nullptr;

        ImVec2 pos = ImGui::GetCursorPos();
        float button_width = ImGui::GetStyle().ItemSpacing.x;
        ImGui::BeginDisabled( true );
        if( cataimgui::BeginRightAlign( "+" ) ) {
            if( ImGui::Button( ICON_IGFD_ADD ) ) {
            }
            button_width += ImGui::GetColumnWidth( -1 );
            cataimgui::EndRightAlign();
        }
        ImGui::SetItemTooltip(
            // NOLINTNEXTLINE(cata-text-style) not shown in a monospaced font
            _( "Not yet implemented. In the mean time you can edit ‘%s’ to pick any font you want." ),
            fs::path( PATH_INFO::fontdata() ).c_str() );
        ImGui::EndDisabled();

        float wrap_width = ImGui::GetContentRegionAvail().x - button_width;
        ImGui::SetCursorPos( pos );
        ImGui::PushTextWrapPos( ImGui::GetCursorPos().x + wrap_width );
        ImGui::AlignTextToFramePadding();
        ImGui::TextWrapped( "%s", _( "Later fonts are used as fallbacks when a glyph "
                                     "cannot be found in an earlier font." ) );

        ImGui::Separator();
        ImGui::Indent();
        bool is_only_face = typefaces.size() == 1;
        for( font_config &face : typefaces ) {
            if( ShowFaceDetail( face, is_only_face ) ) {
                face_to_remove = &face;
            }
            if( int d = ImGui::ItemReorder( &typefaces, true ) ) {
                drag_source = &face;
                drop_target = &face + d;
            }
        }
        if( face_to_remove ) {
#if defined(_WIN32) or defined(__APPLE__)
            typefaces.erase( typefaces.begin() + ( face_to_remove - typefaces.data() ) );
#else
            typefaces.erase( std::vector<font_config>::iterator( face_to_remove ) );
#endif
            face_to_remove = nullptr;
            WantRebuild = true;
        }
        if( drag_source && drop_target ) {
            std::swap( *drag_source, *drop_target );
            drop_target = nullptr;
            drag_source = nullptr;
            WantRebuild = true;
        }
    }
    ImGui::End();
    if( !*p_open ) {
        // The window was open or we would have returned early,
        // therefore it must have been closed this frame.
        ui_manager::invalidate_all_ui_adaptors();
    }
}

static bool ShowFaceDetail( font_config &face, bool is_only_face )
{
    fs::path filename = fs::u8path( face.path ).filename();
    ImGui::PushID( face.path.c_str() );
    ImGui::BeginGroup();
    ImGui::AlignTextToFramePadding();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::SetNextItemAllowOverlap();
    ImGui::TextDisabled( "%s", ICON_IGFD_FILE_LIST );
    ImGui::SetCursorScreenPos( pos );
    ImVec2 grab_size = ImGui::CalcTextSize( ICON_IGFD_FILE_LIST );
    ImGui::InvisibleButton( "grab", grab_size );
    ImGui::SameLine();
    ImGui::BeginDisabled( is_only_face );
    bool clicked = ImGui::Button( ICON_IGFD_REMOVE );
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled( true );
    if( ImGui::Button( ICON_IGFD_EDIT ) ) {
    }
    ImGui::SetItemTooltip(
        // NOLINTNEXTLINE(cata-text-style) not shown in a monospaced font
        _( "Not yet implemented. In the mean time you can edit ‘%s’ to pick any font you want." ),
        fs::path( PATH_INFO::fontdata() ).c_str() );
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted( filename );
    ImGui::SameLine();
    if( cataimgui::BeginRightAlign( "right-controls" ) ) {
        if( ImGui::BeginCombo( "##hinting",
                               localized_hinting_types[fonthint_to_hint( face.hinting )].c_str(),
                               ImGuiComboFlags_WidthFitPreview ) ) {
            for( const char *h : hinting_types ) {
                ImGui::PushID( h );
                const bool is_selected = fonthint_to_hint( face.hinting ) == h;
                if( ImGui::Selectable( localized_hinting_types[h].c_str(), is_selected ) ) {
                    face.hinting = hint_to_fonthint( h );
                    WantRebuild = true;
                }
                if( is_selected ) {
                    ImGui::SetItemDefaultFocus();
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        cataimgui::EndRightAlign();
    }
    // TODO(db48x): other error messages here
    if( !file_exist( face.path ) ) {
        ImGui::Bullet();
        ImGui::TextUnformatted( _( "Warning: the specified file does not exist; ignoring it." ) );
    }
    ImGui::EndGroup();
    ImGui::PopID();
    return clicked;
}

} // FontPicker
#endif // IMTUI
