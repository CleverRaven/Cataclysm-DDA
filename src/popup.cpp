#include "popup.h"

#include <algorithm>
#include <array>
#include <memory>

#include "options.h"
#include "cached_options.h"
#include "catacharset.h"
#include "input_context.h"
#include "output.h"
#include "ui_manager.h"
#include "ui.h"
#include "cata_imgui.h"
#include "imgui/imgui.h"
#include "color.h"

class query_popup_impl : public cataimgui::window
{
        short mouse_selected_option;
        size_t msg_width;
        nc_color default_text_color;
        query_popup *parent;
        short last_keyboard_selected_option;
    public:
        short keyboard_selected_option;

        explicit query_popup_impl( query_popup *parent ) : cataimgui::window( "QUERY_POPUP",
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize ),
            default_text_color( c_white ) {
            msg_width = 400;
            this->parent = parent;
            keyboard_selected_option = 0;
            last_keyboard_selected_option = -1;
            mouse_selected_option = -1;
        }

        void on_resized() override;

        int get_mouse_selected_option() const {
            return mouse_selected_option;
        }
    protected:
        void draw_controls() override;
        cataimgui::bounds get_bounds() override {
            return { -1.f, parent->ontop ? 0 : -1.f, -1.f, -1.f};
        }
};

void query_popup_impl::draw_controls()
{
    mouse_selected_option = -1;

    for( const std::string &line : parent->folded_msg ) {
        nc_color col = parent->default_text_color;
        draw_colored_text( line, col, float( msg_width ) );
    }

    if( !parent->buttons.empty() ) {
        int current_line = 0;
        for( size_t ind = 0; ind < parent->buttons.size(); ++ind ) {
            ImGui::SetCursorPosX( float( parent->buttons[ind].pos.x ) );
            ImGui::Button( remove_color_tags( parent->buttons[ind].text ).c_str() );
            if( ImGui::IsItemHovered() ) {
                mouse_selected_option = ind;
            }
            if( keyboard_selected_option != last_keyboard_selected_option &&
                keyboard_selected_option == short( ind ) && ImGui::IsWindowFocused() ) {
                ImGui::SetKeyboardFocusHere( -1 );
            }
            if( current_line == parent->buttons[ind].pos.y ) {
                ImGui::SameLine();
            }
            current_line = parent->buttons[ind].pos.y;
        }
    }
}

void query_popup_impl::on_resized()
{
    size_t frame_padding = size_t( ImGui::GetStyle().FramePadding.x * 2 );
    size_t item_padding = size_t( ImGui::GetStyle().ItemSpacing.x );
    // constexpr size_t vert_padding = 1;
    size_t max_line_width = str_width_to_pixels( FULL_SCREEN_WIDTH - 1 * 2 );

    // Fold message text
    parent->folded_msg = foldstring( parent->text, max_line_width );

    // Fold query buttons
    const auto &folded_query = query_popup::fold_query( parent->category, parent->pref_kbd_mode,
                               parent->options, max_line_width,
                               frame_padding + item_padding );

    // Calculate size of message part
    msg_width = 0;
    for( const auto &line : parent->folded_msg ) {
        msg_width = std::max( msg_width,
                              get_text_width( remove_color_tags( line ) ) ); //utf8_width( line, true ) );
    }
    auto btn_padding = [&frame_padding, &item_padding]( size_t num_buttons ) {
        return ( frame_padding * ( num_buttons - 1 ) ) + ( item_padding * ( num_buttons - 1 ) );
    };
    // Calculate width with query buttons
    for( const auto &line : folded_query ) {
        if( !line.empty() ) {
            int button_width = 0;
            for( const auto &opt : line ) {
                button_width += get_text_width( remove_color_tags( opt ) );
            }
            // extra item padding needed here to account for space left at the beginning of the window by ImGui
            msg_width = std::max( msg_width, button_width + btn_padding( line.size() ) + item_padding );
        }
    }
    msg_width = std::min( msg_width, max_line_width );

    // Calculate height with query buttons & button positions
    parent->buttons.clear();
    size_t line_idx = 0;
    if( !folded_query.empty() ) {
        for( const auto &line : folded_query ) {
            if( !line.empty() ) {
                int button_width = 0;
                for( const auto &opt : line ) {
                    button_width += get_text_width( remove_color_tags( opt ) );
                }
                // Right align.
                // TODO: multi-line buttons
                size_t button_x = std::max( size_t( 0 ),
                                            size_t( msg_width - button_width - btn_padding( line.size() ) ) );
                for( const auto &opt : line ) {
                    parent->buttons.emplace_back( opt, point( button_x, line_idx ) );
                    button_x += get_text_width( remove_color_tags( opt ) ) + frame_padding + item_padding;
                }
            }
            line_idx++;
        }
    }
}

query_popup::query_popup()
    : cur( 0 ), default_text_color( c_white ), anykey( false ), cancel( false ),
      ontop( false ), fullscr( false ), pref_kbd_mode( keyboard_mode::keycode )
{
}

query_popup &query_popup::context( const std::string &cat )
{
    invalidate_ui();
    category = cat;
    return *this;
}

query_popup &query_popup::option( const std::string &opt )
{
    invalidate_ui();
    options.emplace_back( opt, []( const input_event & ) {
        return true;
    } );
    return *this;
}

query_popup &query_popup::option( const std::string &opt,
                                  const std::function<bool( const input_event & )> &filter )
{
    invalidate_ui();
    options.emplace_back( opt, filter );
    return *this;
}

query_popup &query_popup::allow_anykey( bool allow )
{
    // Change does not affect cache, do not invalidate the window
    anykey = allow;
    return *this;
}

query_popup &query_popup::allow_cancel( bool allow )
{
    // Change does not affect cache, do not invalidate the window
    cancel = allow;
    return *this;
}

query_popup &query_popup::on_top( bool top )
{
    invalidate_ui();
    ontop = top;
    return *this;
}

query_popup &query_popup::full_screen( bool full )
{
    invalidate_ui();
    fullscr = full;
    return *this;
}

query_popup &query_popup::cursor( size_t pos )
{
    // Change does not affect cache, do not invalidate window
    cur = pos;
    std::shared_ptr<query_popup_impl> impl = p_impl.lock();
    if( impl ) {
        impl->keyboard_selected_option = short( cur );
    }
    return *this;
}

query_popup &query_popup::default_color( const nc_color &d_color )
{
    default_text_color = d_color;
    return *this;
}

query_popup &query_popup::preferred_keyboard_mode( const keyboard_mode mode )
{
    invalidate_ui();
    pref_kbd_mode = mode;
    return *this;
}

std::vector<std::vector<std::string>> query_popup::fold_query(
                                       const std::string &category,
                                       const keyboard_mode pref_kbd_mode,
                                       const std::vector<query_option> &options,
                                       const int max_width, const int horz_padding )
{
    input_context ctxt( category, pref_kbd_mode );

    std::vector<std::vector<std::string>> folded_query;
    folded_query.emplace_back();

    int query_cnt = 0;
    int query_width = 0;
    for( const query_popup::query_option &opt : options ) {
        const std::string &name = ctxt.get_action_name( opt.action );
        const std::string &desc = ctxt.get_desc( opt.action, name, opt.filter );
        const int this_query_width = utf8_width( desc, true ) + horz_padding;
        ++query_cnt;
        query_width += this_query_width;
        if( query_width > max_width + horz_padding ) {
            if( query_cnt == 1 ) {
                // Each line has at least one query, so keep this query in the current line
                folded_query.back().emplace_back( desc );
                folded_query.emplace_back();
                query_cnt = 0;
                query_width = 0;
            } else {
                // Wrap this query to the next line
                folded_query.emplace_back();
                folded_query.back().emplace_back( desc );
                query_cnt = 1;
                query_width = this_query_width;
            }
        } else {
            folded_query.back().emplace_back( desc );
        }
    }

    if( folded_query.back().empty() ) {
        folded_query.pop_back();
    }

    return folded_query;
}

void query_popup::invalidate_ui() const
{
    std::shared_ptr<query_popup_impl> ui = p_impl.lock();
    if( ui ) {
        ui->mark_resized();
    }
}

query_popup::result query_popup::query_once()
{

    if( !anykey && !cancel && options.empty() ) {
        return { false, "ERROR", {} };
    }

    if( test_mode ) {
        return { false, "ERROR", {} };
    }

    std::shared_ptr<query_popup_impl> impl = create_or_get_impl();

    // ImGui doesn't like to show the window on the first drawn frame, so we do this
    ui_manager::redraw();

    input_context ctxt( category, pref_kbd_mode );
    if( cancel || !options.empty() ) {
        ctxt.register_action( "HELP_KEYBINDINGS" );
    }
    if( !options.empty() ) {
        ctxt.register_action( "CONFIRM" );
        for( const query_popup::query_option &opt : options ) {
            ctxt.register_action( opt.action );
        }
        // Mouse movement and button
        ctxt.register_action( "SELECT" );
        ctxt.register_action( "MOUSE_MOVE" );
        ctxt.register_action( "LEFT" );
        ctxt.register_action( "RIGHT" );
    }
    if( anykey ) {
        ctxt.register_action( "ANY_INPUT" );
        // Mouse movement, button, and wheel
        ctxt.register_action( "COORDINATE" );
    }
    if( cancel ) {
        ctxt.register_action( "QUIT" );
    }
#if defined(WIN32) || defined(TILES)
    ctxt.set_timeout( 50 );
#endif

    result res;
    // Assign outside construction of `res` to ensure execution order
    res.wait_input = !anykey;
    do {
        ui_manager::redraw();
        res.action = ctxt.handle_input();
        res.evt = ctxt.get_raw_input();

        // If we're tracking mouse movement
        if( !options.empty() && res.action == "SELECT" && impl->get_mouse_selected_option() != -1 ) {
            // Left-click to confirm selection
            res.action = "CONFIRM";
            cur = size_t( impl->get_mouse_selected_option() );
        } else if( res.action == "CONFIRM" && impl->keyboard_selected_option != -1 ) {
            cur = size_t( impl->keyboard_selected_option );
        }
    } while(
        // Always ignore mouse movement
        ( res.evt.type == input_event_t::mouse &&
          res.evt.get_first_input() == static_cast<int>( MouseInput::Move ) ) ||
        // Ignore window losing focus in SDL
        ( res.evt.type == input_event_t::keyboard_char && res.evt.sequence.empty() ) ||
        res.evt.type == input_event_t::timeout
    );

    if( cancel && res.action == "QUIT" ) {
        res.wait_input = false;
    } else if( res.action == "CONFIRM" ) {
        if( cur < options.size() ) {
            res.wait_input = false;
            res.action = options[cur].action;
        }
    } else if( res.action == "LEFT" ) {
        if( impl->keyboard_selected_option > 0 ) {
            impl->keyboard_selected_option--;
        } else {
            impl->keyboard_selected_option = short( buttons.size() - 1 );
        }
    } else if( res.action == "RIGHT" ) {
        if( impl->keyboard_selected_option < short( buttons.size() - 1 ) ) {
            impl->keyboard_selected_option++;
        } else {
            impl->keyboard_selected_option = 0;
        }
    } else if( res.action == "HELP_KEYBINDINGS" ) {
        // Keybindings may have changed, regenerate the UI
        std::shared_ptr<query_popup_impl> impl = p_impl.lock();
        if( impl ) {
            impl->on_resized();
        }
        //init();
    } else {
        for( size_t ind = 0; ind < options.size(); ++ind ) {
            if( res.action == options[ind].action ) {
                cur = ind;
                if( options[ind].filter( res.evt ) ) {
                    res.wait_input = false;
                    break;
                }
            }
        }
    }

    return res;
}

std::shared_ptr<query_popup_impl> query_popup::create_or_get_impl()
{
    std::shared_ptr<query_popup_impl> impl = p_impl.lock();
    if( !impl ) {
        p_impl = impl = std::make_shared<query_popup_impl>( this );
        if( impl ) {
            impl->mark_resized();
            impl->keyboard_selected_option = short( cur );
        }
    }
    return impl;
}
query_popup::result query_popup::query()
{
    std::shared_ptr<query_popup_impl> ui = create_or_get_impl();

    result res;
    do {
        res = query_once();
    } while( res.wait_input );
    return res;
}

std::string query_popup::wait_text( const std::string &text, const nc_color &bar_color )
{
    static const std::array<std::string, 4> phase_icons = {{ "|", "/", "-", "\\" }};
    static size_t phase = phase_icons.size() - 1;
    phase = ( phase + 1 ) % phase_icons.size();
    return string_format( " %s %s", colorize( phase_icons[phase], bar_color ), text );
}

std::string query_popup::wait_text( const std::string &text )
{
    return wait_text( text, c_light_green );
}

query_popup::result::result()
    : wait_input( false ), action( "ERROR" )
{
}

query_popup::result::result( bool wait_input, const std::string &action, const input_event &evt )
    : wait_input( wait_input ), action( action ), evt( evt )
{
}

query_popup::query_option::query_option(
    const std::string &action,
    const std::function<bool( const input_event & )> &filter )
    : action( action ), filter( filter )
{
}

query_popup::button::button( const std::string &text, const point &p )
    : text( text ), pos( p )
{
    width = utf8_width( text, true );
}

bool query_popup::button::contains( const point &p ) const
{
    return p.x >= pos.x + 1 &&
           p.x < pos.x + width + 1 &&
           p.y == pos.y + 1;
}

static_popup::static_popup()
{
    ui = create_or_get_impl();
}
