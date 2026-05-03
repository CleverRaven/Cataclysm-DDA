#include "input_popup.h"

#include <algorithm>
#include <cstddef>

#include "coordinates.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "input_enums.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "text.h"
#include "translations.h"
#include "try_parse_integer.h"
#include "ui_manager.h"
#include "uilist.h"
#include "uistate.h"

input_popup::input_popup( int width, const std::string &title, const point &pos,
                          ImGuiWindowFlags flags ) :
    cataimgui::window( title, ImGuiWindowFlags_NoNavInputs | flags | ( title.empty() ?
                       ImGuiWindowFlags_NoTitleBar : 0 ) | ImGuiWindowFlags_AlwaysAutoResize ),
    pos( pos ),
    width( width )
{
    ctxt = input_context( "STRING_INPUT", keyboard_mode::keychar );
    ctxt.register_action( "TEXT.CONFIRM" );
    ctxt.register_action( "TEXT.QUIT" );
    ctxt.register_action( "TEXT.CLEAR" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.set_timeout( 10 );
}

void input_popup::set_description( const std::string &desc, const nc_color &default_color,
                                   bool monofont )
{
    description = desc;
    description_default_color = default_color;
    description_monofont = monofont;
}

void input_popup::set_label( const std::string &label, const nc_color &default_color )
{
    this->label = label;
    label_default_color = default_color;
}

void input_popup::set_max_input_length( int length )
{
    max_input_length = length;
}

int input_popup::get_max_input_length() const
{
    return max_input_length;
}

static void register_input( input_context &ctxt, const callback_input &input )
{
    if( input.description ) {
        ctxt.register_action( input.action, *input.description );
    } else {
        ctxt.register_action( input.action );
    }
}

void input_popup::add_callback( const callback_input &input,
                                const std::function<bool()> &callback_func )
{
    if( !input.action.empty() ) {
        register_input( ctxt, input );
    }
    callbacks.emplace_back( input, callback_func );
}

bool input_popup::cancelled() const
{
    return is_cancelled;
}

cataimgui::bounds input_popup::get_bounds()
{
    // AlwaysAutoResize fits to inner widgets only, so neither the title bar
    // nor a multi-line description gets a chance to widen the window.
    // When the caller hasn't pinned an explicit width, request one wide
    // enough for the longer of the title text or the widest description
    // line, plus the close button.
    float bounds_w = -1.f;
    if( width > 0 ) {
        bounds_w = str_width_to_pixels( width );
    } else {
        const ImGuiStyle &style = ImGui::GetStyle();
        const float padding = style.WindowPadding.x * 2.f;
        const float close_w = ImGui::GetFrameHeight() + style.ItemInnerSpacing.x;
        float title_needs = 0.f;
        if( !id.empty() ) {
            title_needs = ImGui::CalcTextSize( id.c_str(), nullptr, true ).x + close_w;
        }
        float description_needs = 0.f;
        size_t off = 0;
        while( off <= description.size() ) {
            const size_t nl = description.find( '\n', off );
            const std::string line = description.substr( off, nl - off );
            description_needs = std::max( description_needs,
                                          ImGui::CalcTextSize( line.c_str() ).x );
            if( nl == std::string::npos ) {
                break;
            }
            off = nl + 1;
        }
        if( title_needs > 0.f || description_needs > 0.f ) {
            bounds_w = std::max( title_needs, description_needs ) + padding;
        }
        // Floor at a comfortable minimum so popups with very short titles
        // and descriptions still have room for the input + step buttons.
        const float min_w = str_width_to_pixels( 30 );
        bounds_w = std::max( bounds_w, min_w );
    }
    return {
        pos.x < 0 ? -1.f : str_width_to_pixels( pos.x ),
        pos.y < 0 ? -1.f : str_height_to_pixels( pos.y ),
        bounds_w,
        -1.f
    };
}

void input_popup::draw_controls()
{
    if( !description.empty() ) {
        if( description_monofont ) {
            cataimgui::PushMonoFont();
        }
        // todo: you should not have to manually split text to print it as paragraphs
        uint64_t offset = 0;
        uint64_t pos = 0;
        while( pos != std::string::npos ) {
            pos = description.find( '\n', offset );
            std::string str = description.substr( offset, pos - offset );
            cataimgui::TextColoredParagraph( description_default_color, str );
            offset = pos + 1;
        }
        if( description_monofont ) {
            ImGui::PopFont();
        }
        ImGui::SameLine();
    }

    if( !label.empty() ) {
        ImGui::AlignTextToFramePadding();
        cataimgui::draw_colored_text( label, c_light_red );
        ImGui::SameLine();
    }

    // make sure the cursor is in the input field when we regain focus
    if( !ImGui::IsWindowFocused() ) {
        set_focus = true;
    }

    if( set_focus && ImGui::IsWindowFocused() ) {
        ImGui::SetKeyboardFocusHere();
        set_focus = false;
    }
    draw_input_control();

    // Save / Cancel row, right-aligned.  Mirrors TEXT.CONFIRM / TEXT.QUIT keys.
    const ImGuiStyle &style = ImGui::GetStyle();
    const std::string save_label = _( "Save" );
    const std::string cancel_label = _( "Cancel" );
    const float buttons_w = ImGui::CalcTextSize( save_label.c_str() ).x
                            + ImGui::CalcTextSize( cancel_label.c_str() ).x
                            + 4.f * style.FramePadding.x + style.ItemSpacing.x;
    const float row_avail = ImGui::GetContentRegionAvail().x;
    if( buttons_w < row_avail ) {
        ImGui::SetCursorPosX( ImGui::GetCursorPosX() + ( row_avail - buttons_w ) );
    }
    action_button( "TEXT.QUIT", cancel_label );
    ImGui::SameLine();
    action_button( "TEXT.CONFIRM", save_label );
}

bool input_popup::handle_custom_callbacks( const std::string &action )
{

    input_event ev = ctxt.get_raw_input();
    int key = ev.get_first_input();

    bool next_loop = false;
    for( const auto &cb : callbacks ) {
        if( cb.first == callback_input( action, key ) && cb.second() ) {
            next_loop = true;
            break;
        }
    }

    return next_loop;
}

string_input_popup_imgui::string_input_popup_imgui( int width, const std::string &old_input,
        const std::string &title, const point &pos, ImGuiWindowFlags flags ) :
    input_popup( width, title, pos, flags ),
    text( old_input ),
    old_input( old_input )
{
    // for opening uilist history
    // non-uilist history is handled by imgui itself and not customizable (yet?)
    ctxt.register_action( "HISTORY_UP" );
}

static void set_text( ImGuiInputTextCallbackData *data, const std::string &text )
{
    data->DeleteChars( 0, data->BufTextLen );
    data->InsertChars( 0, text.c_str() );
}

static int input_callback( ImGuiInputTextCallbackData *data )
{
    string_input_popup_imgui *popup = static_cast<string_input_popup_imgui *>( data->UserData );

    if( data->EventFlag == ImGuiInputTextFlags_CallbackHistory ) {
        popup->update_input_history( data );
    }

    if( data->EventFlag == ImGuiInputTextFlags_CallbackEdit ) {
        int max = popup->get_max_input_length();
        if( max > 0 && data->BufTextLen > max ) {
            data->BufTextLen = max;
            data->SelectionStart = data->BufTextLen;
            data->SelectionEnd = data->BufTextLen;
            data->CursorPos = data->BufTextLen;
            data->BufDirty = true;
        }
    }

    if( data->EventFlag == ImGuiInputTextFlags_CallbackAlways && popup->want_clear_text() ) {
        // Only called when popup->text is empty
        data->BufTextLen = 0;
        data->SelectionStart = 0;
        data->SelectionEnd = 0;
        data->CursorPos = 0;
        data->Buf[0] = '\000';
        data->BufDirty = true;
    }

    return 0;
}

bool string_input_popup_imgui::want_clear_text()
{
    if( do_clear_text ) {
        do_clear_text = false;
        return true;
    }
    return false;
}

void string_input_popup_imgui::draw_input_control()
{
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackEdit;

    if( !is_uilist_history ) {
        flags |= ImGuiInputTextFlags_CallbackHistory;
    }

    if( do_clear_text ) {
        flags |= ImGuiInputTextFlags_CallbackAlways;
    }

    // shrink width of input field if we only allow short inputs
    // todo: use full available width for unrestricted inputs in wider windows
    float input_width = str_width_to_pixels( max_input_length + 1 );
    if( max_input_length > 0 && input_width < ImGui::CalcItemWidth() ) {
        ImGui::SetNextItemWidth( input_width );
    }

    std::string input_label = "##string_input_" + label;
    ImGui::InputText( input_label.c_str(), &text, flags, input_callback, this );
}

void string_input_popup_imgui::set_identifier( const std::string &ident )
{
    identifier = ident;
}

void string_input_popup_imgui::set_text( const std::string &txt )
{
    text = txt;
}

void string_input_popup_imgui::show_history()
{
    if( identifier.empty() ) {
        return;
    }
    std::vector<std::string> &hist = uistate.gethistory( identifier );
    uilist hmenu;
    hmenu.title = _( "d: delete history" );
    hmenu.allow_anykey = true;
    for( size_t h = 0; h < hist.size(); h++ ) {
        hmenu.addentry( h, true, -2, hist[h] );
    }
    if( !text.empty() && ( hmenu.entries.empty() ||
                           hmenu.entries[hist.size() - 1].txt != text ) ) {
        hmenu.addentry( hist.size(), true, -2, text );
    }

    if( !hmenu.entries.empty() ) {
        hmenu.selected = hmenu.entries.size() - 1;

        bool finished = false;
        do {
            hmenu.query();
            if( hmenu.ret >= 0 && hmenu.entries[hmenu.ret].txt != text ) {
                set_text( hmenu.entries[hmenu.ret].txt );
                if( static_cast<size_t>( hmenu.ret ) < hist.size() ) {
                    hist.erase( hist.begin() + hmenu.ret );
                    add_to_history( text );
                }
                finished = true;
            } else if( hmenu.ret == UILIST_UNBOUND && hmenu.ret_evt.get_first_input() == 'd' ) {
                hist.clear();
                finished = true;
            } else if( hmenu.ret != UILIST_UNBOUND ) {
                finished = true;
            }
        } while( !finished );
    }
}

void string_input_popup_imgui::update_input_history( ImGuiInputTextCallbackData *data )
{
    if( data->EventKey != ImGuiKey_UpArrow && data->EventKey != ImGuiKey_DownArrow ) {
        return;
    }

    bool up = data->EventKey == ImGuiKey_UpArrow;

    if( identifier.empty() ) {
        return;
    }

    std::vector<std::string> &hist = uistate.gethistory( identifier );

    if( hist.empty() ) {
        return;
    }

    if( hist.size() >= max_history_size ) {
        hist.erase( hist.begin(), hist.begin() + ( hist.size() - max_history_size ) );
    }

    if( up ) {
        if( history_index >= static_cast<int>( hist.size() ) ) {
            return;
        } else if( history_index == 0 ) {
            session_input.assign( data->Buf, data->BufTextLen );

            //avoid showing the same result twice (after reopen filter window without reset)
            if( hist.size() > 1 && session_input == hist.back() ) {
                history_index += 1;
            }
        }
    } else {
        if( history_index == 1 ) {
            ::set_text( data, session_input );
            //show initial string entered and 'return'
            history_index = 0;
            return;
        } else if( history_index == 0 ) {
            return;
        }
    }

    history_index += up ? 1 : -1;
    const std::string &new_text = hist[hist.size() - history_index];
    ::set_text( data, new_text );
}

void string_input_popup_imgui::add_to_history( const std::string &value ) const
{
    if( !identifier.empty() && !value.empty() ) {
        std::vector<std::string> &hist = uistate.gethistory( identifier );
        if( hist.empty() || hist[hist.size() - 1] != value ) {
            hist.push_back( value );
        }
    }
}

std::string string_input_popup_imgui::query()
{
    is_cancelled = false;

    while( true ) {
        ui_manager::redraw_invalidated();

        std::string action = ctxt.handle_input();
        if( has_button_action() ) {
            action = get_button_action();
        }
        if( handle_custom_callbacks( action ) ) {
            continue;
        }

        if( action == "TEXT.CONFIRM" ) {
            add_to_history( text );
            return text;
        } else if( action == "TEXT.QUIT" ) {
            break;
        } else if( action == "HISTORY_UP" && is_uilist_history ) {
            // non-uilist history is handled inside input callback
            show_history();
        } else if( action == "TEXT.CLEAR" ) {
            do_clear_text = true;
        }

        // mouse click on x to close leads here
        if( !get_is_open() ) {
            break;
        }
    }

    is_cancelled = true;
    return old_input;
}

std::optional<tripoint_abs_omt> string_input_popup_imgui::query_coordinate( bool loop )
{
    do {
        const std::string &queried_string = query();
        ret_val<tripoint_abs_omt> result = try_parse_coordinate_abs( queried_string );
        if( cancelled() || queried_string.empty() ) {
            return std::nullopt;
        }
        if( result.success() ) {
            return result.value();
        }
    } while( loop );

    return tripoint_abs_omt::zero;
}

void string_input_popup_imgui::use_uilist_history( bool use_uilist )
{
    is_uilist_history = use_uilist;
}

template<typename T>
number_input_popup<T>::number_input_popup( int width, T old_value, const std::string &title,
        const point &pos, ImGuiWindowFlags flags ) :
    input_popup( width, title, pos, flags ),
    value( old_value ),
    old_value( old_value )
{
    // potentially register context keys
}

// SetNextItemWidth on InputScalar (used by InputInt/InputFloat) sizes the
// whole group; ImGui subtracts the step buttons internally for the text
// field.  When max_input_length is set we use it as the digit budget and
// right-align the input + buttons against the row.
static void number_input_popup_apply_width( cataimgui::window &win, int max_input_length )
{
    if( max_input_length <= 0 ) {
        return;
    }
    const ImGuiStyle &style = ImGui::GetStyle();
    const float button_w = ImGui::GetFrameHeight();
    const float spacing = style.ItemInnerSpacing.x;
    const float text_w = win.str_width_to_pixels( max_input_length + 1 );
    const float group_w = text_w + 2.f * ( button_w + spacing );
    const float avail = ImGui::GetContentRegionAvail().x;
    if( group_w < avail ) {
        ImGui::SetCursorPosX( ImGui::GetCursorPosX() + ( avail - group_w ) );
    }
    ImGui::SetNextItemWidth( group_w );
}

template<>
void number_input_popup<int>::draw_input_control()
{
    number_input_popup_apply_width( *this, max_input_length );
    ImGui::InputInt( "##number_input", &value, step_size.value_or( 1 ),
                     fast_step_size.value_or( 100 ) );
}

template<>
void number_input_popup<float>::draw_input_control()
{
    number_input_popup_apply_width( *this, max_input_length );
    cataimgui::InputFloat( "##number_input", &value, step_size.value_or( 0.f ),
                           fast_step_size.value_or( 0.f ) );
}

template<typename T>
void number_input_popup<T>::set_value_range( T min, T max )
{
    range_min = min;
    range_max = max;
    set_description( string_format( _( "Range: %s - %s" ),
                                    std::to_string( min ), std::to_string( max ) ) );
}

template<typename T>
T number_input_popup<T>::query()
{

    while( true ) {
        ui_manager::redraw_invalidated();
        std::string action = ctxt.handle_input();
        if( has_button_action() ) {
            action = get_button_action();
        }

        if( handle_custom_callbacks( action ) ) {
            continue;
        }

        if( action == "TEXT.CONFIRM" ) {
            return value;
        } else if( action == "TEXT.QUIT" ) {
            break;
        }

        // mouse click on x to close leads here
        if( !get_is_open() ) {
            break;
        }
    }

    return old_value;
}

template<typename T>
void number_input_popup<T>::set_step_size( std::optional<T> step, std::optional<T> fast_step )
{
    step_size = step;
    fast_step_size = fast_step;
}

template class number_input_popup<int>;
template class number_input_popup<float>;
