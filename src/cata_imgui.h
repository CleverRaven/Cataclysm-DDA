#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#ifndef IMGUI_DISABLE

#include "font_loader.h"

class nc_color;
struct input_event;
using ImGuiInputTextFlags = int;

#if defined(IMTUI) || !(defined(WIN32) || defined(TILES))
#   define TUI
#endif

#ifndef TUI
#include "sdl_geometry.h"
#include "sdl_wrappers.h"
#include "color_loader.h"
#endif
#include "text.h"

struct point;
struct ImVec2;
struct ImVec4;
class Font;
class input_context;

namespace cataimgui
{
struct bounds {
    float x;
    float y;
    float w;
    float h;
};

enum class mbox_btn {
    BT_OK = 0,
    BT_OKCancel = 1,
    BT_YesNoCancel = 2,
    BT_YesNo = 3
};

enum class dialog_result {
    None = 0,
    OKClicked,
    CancelClicked,
    YesClicked,
    NoClicked
};

enum class scroll : int {
    none = 0,
    begin,
    end,
    line_up,
    line_down,
    page_up,
    page_down
};

class client
{
        std::vector<int> cata_input_trail;
#ifndef TUI
        std::unordered_map<uint32_t, unsigned char> sdlColorsToCata;
#endif
    public:
#ifdef TUI
        client();
#else
        client( const SDL_Renderer_Ptr &sdl_renderer, const SDL_Window_Ptr &sdl_window,
                const GeometryRenderer_Ptr &sdl_geometry );
        void load_fonts( const std::unique_ptr<Font> &gui_font, const std::unique_ptr<Font> &mono_font,
                         const std::array<SDL_Color, color_loader<SDL_Color>::COLOR_NAMES_COUNT> &windowsPalette,
                         const std::vector<font_config> &gui_typeface, const std::vector<font_config> &mono_typeface );
#endif
        ~client();

        void new_frame();
        void end_frame();
        void process_input( void *input );
        void process_cata_input( const input_event &event );
#ifdef TUI
        void upload_color_pair( int p, int f, int b );
        void set_alloced_pair_count( short count );
#else
        const SDL_Renderer_Ptr &sdl_renderer;
        const SDL_Window_Ptr &sdl_window;
        const GeometryRenderer_Ptr &sdl_geometry;
#endif
        bool auto_size_frame_active();
        bool any_window_shown();
};

void point_to_imvec2( point *src, ImVec2 *dest );
void imvec2_to_point( ImVec2 *src, point *dest );

ImVec4 imvec4_from_color( nc_color &color );

void set_scroll( scroll &s );

void draw_colored_text( const std::string &original_text, const nc_color &color,
                        float wrap_width = 0.0F, bool *is_selected = nullptr,
                        bool *is_focused = nullptr, bool *is_hovered = nullptr );
void draw_colored_text( const std::string &original_text, nc_color &color,
                        float wrap_width = 0.0F, bool *is_selected = nullptr,
                        bool *is_focused = nullptr, bool *is_hovered = nullptr );
void draw_colored_text( const std::string &original_text,
                        float wrap_width = 0.0F, bool *is_selected = nullptr,
                        bool *is_focused = nullptr, bool *is_hovered = nullptr );

class window
{
        std::unique_ptr<class window_impl> p_impl;
        std::unique_ptr<class filter_box_impl> filter_impl;
        bounds cached_bounds;
    protected:
        explicit window( int window_flags = 0 );
    public:
        explicit window( const std::string &id_, int window_flags = 0 );
        virtual ~window();
        bool action_button( const std::string &action, const std::string &text );
        bool has_button_action();
        std::string get_button_action();
        void draw_header( std::string const &text );
        bool get_is_open() const;
        virtual void draw();
        virtual void on_resized() {}
        bool is_bounds_changed();
        size_t get_text_width( const std::string &text );
        size_t get_text_height( const char *text );
        size_t str_width_to_pixels( size_t len );
        size_t str_height_to_pixels( size_t len );
        std::string get_filter();
        void clear_filter();
        void mark_resized();

    protected:
        bool force_to_back = false;
        bool is_open;
        std::string id;
        int window_flags;
        std::string button_action;
        virtual bounds get_bounds();
        virtual void draw_controls() = 0;
        void draw_filter( const input_context &ctxt, bool filtering_active );
};

#ifdef TUI
void init_pair( int p, int f, int b );
void load_colors();
#endif

// drops the ImGuiInputTextFlags_CharsScientific flag from regular imgui InputFloat because it doesn't allow commas
bool InputFloat( const char *label, float *v, float step = 0.0f, float step_fast = 0.0f,
                 const char *format = "%.3f", ImGuiInputTextFlags flags = 0 );

void PushGuiFont();
void PushMonoFont();

bool BeginRightAlign( const char *str_id );
void EndRightAlign();

// Set ImGui theme colors to those chosen by the player.
// This loads the settings from `config/imgui_style.json` and - optionally - falls back to base colors
// for elements not explicitly specified.
void init_colors();
} // namespace cataimgui
#else
#include "imgui_facade.h"
#endif // IMGUI_DISABLE
