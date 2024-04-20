#pragma once
#include <cstddef>
#include <memory>
#include <string>

class nc_color;
struct input_event;

#if defined(WIN32) || defined(TILES)
#include "sdl_geometry.h"
#include "sdl_wrappers.h"
#include "color_loader.h"
#endif

struct point;
class ImVec2;
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

class client
{
    public:
#if !(defined(TILES) || defined(WIN32))
        client();
#else
        client( const SDL_Renderer_Ptr &sdl_renderer, const SDL_Window_Ptr &sdl_window,
                const GeometryRenderer_Ptr &sdl_geometry );
        void load_fonts( const std::unique_ptr<Font> &cata_font,
                         const std::array<SDL_Color, color_loader<SDL_Color>::COLOR_NAMES_COUNT> &windowsPalette );
#endif
        ~client();

        void new_frame();
        void end_frame();
        void process_input( void *input );
        void process_cata_input( const input_event &event );
#if !(defined(TILES) || defined(WIN32))
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
        void draw_colored_text( std::string const &text, const nc_color &color,
                                float wrap_width = 0.0F, bool *is_selected = nullptr,
                                bool *is_focused = nullptr, bool *is_hovered = nullptr );
        void draw_colored_text( std::string const &text, nc_color &color,
                                float wrap_width = 0.0F, bool *is_selected = nullptr,
                                bool *is_focused = nullptr, bool *is_hovered = nullptr );
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

#if !(defined(TILES) || defined(WIN32))
void init_pair( int p, int f, int b );
void load_colors();
#endif

} // namespace cataimgui
