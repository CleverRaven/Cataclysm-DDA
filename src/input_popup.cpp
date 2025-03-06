#include "input_popup.h"

#include <cstddef>

#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "input_enums.h"
#include "text.h"
#include "translations.h"
#include "uilist.h"
#include "ui_manager.h"
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
    return {
        pos.x < 0 ? -1.f : str_width_to_pixels( pos.x ),
        pos.y < 0 ? -1.f : str_height_to_pixels( pos.y ),
        width <= 0 ? -1.f : str_width_to_pixels( width ),
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
            ImGui::NewLine();
            offset = pos + 1;
        }
        if( description_monofont ) {
            ImGui::PopFont();
        }
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

    return 0;
}

void string_input_popup_imgui::draw_input_control()
{
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackEdit;

    if( !is_uilist_history ) {
        flags |= ImGuiInputTextFlags_CallbackHistory;
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
            session_input = text;

            //avoid showing the same result twice (after reopen filter window without reset)
            if( hist.size() > 1 && session_input == hist.back() ) {
                history_index += 1;
            }
        }
    } else {
        if( history_index == 1 ) {
            text = session_input;
            ::set_text( data, text );
            //show initial string entered and 'return'
            history_index = 0;
            return;
        } else if( history_index == 0 ) {
            return;
        }
    }

    history_index += up ? 1 : -1;
    text = hist[hist.size() - history_index];
    ::set_text( data, text );
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
            text.clear();
        }

        // mouse click on x to close leads here
        if( !get_is_open() ) {
            break;
        }
    }

    is_cancelled = true;
    return old_input;
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

template<>
void number_input_popup<int>::draw_input_control()
{
    // todo: maybe set width of input field, default is fairly long
    // step size default values are imgui defaults
    ImGui::InputInt( "##number_input", &value, step_size.value_or( 1 ),
                     fast_step_size.value_or( 100 ) );
}

template<>
void number_input_popup<float>::draw_input_control()
{
    // todo: maybe set width of input field, default is fairly long
    // step size default values are imgui defaults
    cataimgui::InputFloat( "##number_input", &value, step_size.value_or( 0.f ),
                           fast_step_size.value_or( 0.f ) );
}

template<typename T>
T number_input_popup<T>::query()
{

    while( true ) {
        ui_manager::redraw_invalidated();
        std::string action = ctxt.handle_input();

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
