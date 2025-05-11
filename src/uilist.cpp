#include "uilist.h"

#include <cctype>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <set>

#include "avatar.h"
#include "cached_options.h" // IWYU pragma: keep
#include "cata_assert.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "game.h"
#include "input.h"
#include "memory_fast.h"
#include "output.h"
#include "sdltiles.h"
#include "input_popup.h"
#include "translations.h"
#include "ui_manager.h"
#include "cata_imgui.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#if defined(__ANDROID__)
#include <jni.h>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include "options.h"
#endif

#if defined(TILES) && !defined(__ANDROID__)
#include <SDL2/SDL_mouse.h>
#endif

class uilist_impl : cataimgui::window
{
        uilist &parent;
    public:
        explicit uilist_impl( uilist &parent ) : cataimgui::window( "UILIST",
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoNavInputs ),
            parent( parent ) {
        }

        uilist_impl( uilist &parent, const std::string &title ) : cataimgui::window( title,
                    ImGuiWindowFlags_None | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoNavInputs ),
            parent( parent ) {
        }

        cataimgui::bounds get_bounds() override {
            if( !parent.started ) {
                parent.setup();
            }

            return parent.desired_bounds.value_or( parent.calculated_bounds );
        }
        void draw_controls() override;
};

void uilist_impl::draw_controls()
{
#if defined(TILES)
    using cata::options::mouse;
    bool cursor_shown = SDL_ShowCursor( SDL_QUERY ) == SDL_ENABLE;
    if( mouse.hidekb && !cursor_shown ) {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
    } else {
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    }
#endif
    if( !parent.text.empty() ) {
        cataimgui::draw_colored_text( parent.text );
        ImGui::Separator();
    }

    if( !parent.categories.empty() ) {
        if( ImGui::BeginTabBar( "##categories",
                                ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_NoCloseWithMiddleMouseButton ) ) {
            for( size_t i = 0; i < parent.categories.size(); i++ ) {
                auto cat = parent.categories[ i ];
                bool selected = i == parent.switch_to_category;
                ImGuiTabItemFlags_ flags = ImGuiTabItemFlags_None;
                if( selected ) {
                    flags = ImGuiTabItemFlags_SetSelected;
                    parent.switch_to_category = -1;
                }
                if( ImGui::BeginTabItem( cat.second.c_str(), nullptr, flags ) ) {
                    if( parent.current_category != i ) {
                        parent.current_category = i;
                        parent.filterlist();
                    }
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }

    // An invisible table with three columns. Used to create a sidebar effect.
    // Ideally we would use a layout engine for this, but ImGui does not natively support any.
    // TODO: Investigate using Stack Layout (https://github.com/thedmd/imgui/tree/feature/layout-external)
    // Center column is for the menu, left and right are usually invisible. Caller may use
    // left/right column to add additional content to the
    // window. There should only ever be one row.
    if( ImGui::BeginTable( "table", 3,
                           ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX |
                           ImGuiTableFlags_Hideable ) ) {
        ImGui::TableSetupColumn( "left", ImGuiTableColumnFlags_WidthFixed, parent.extra_space_left );
        ImGui::TableSetupColumn( "menu", ImGuiTableColumnFlags_WidthFixed, parent.calculated_menu_size.x );
        ImGui::TableSetupColumn( "right", ImGuiTableColumnFlags_WidthFixed, parent.extra_space_right );

        ImGui::TableSetColumnEnabled( 0, parent.extra_space_left < 1.0f ? false : true );
        ImGui::TableSetColumnEnabled( 2, parent.extra_space_right < 1.0f ? false : true );

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 1 );

        float entry_height = ImGui::GetTextLineHeightWithSpacing();
        ImGuiStyle &style = ImGui::GetStyle();
        if( ImGui::BeginChild( "scroll", parent.calculated_menu_size ) ) {
            if( ImGui::BeginTable( "menu items", 3, ImGuiTableFlags_SizingFixedFit ) ) {
                ImGui::TableSetupColumn( "hotkey", ImGuiTableColumnFlags_WidthFixed,
                                         parent.calculated_hotkey_width );
                ImGui::TableSetupColumn( "primary", ImGuiTableColumnFlags_WidthFixed,
                                         parent.calculated_label_width );
                ImGui::TableSetupColumn( "secondary", ImGuiTableColumnFlags_WidthFixed,
                                         parent.calculated_secondary_width );

                ImGuiListClipper clipper;
                clipper.Begin( parent.fentries.size(), entry_height );
                clipper.IncludeRangeByIndices( parent.fselected, parent.fselected + 1 );
                while( clipper.Step() ) {
                    // It would be natural to make the entries into buttons, or
                    // combos, or other pre-built ui elements. For now I am mostly
                    // going to copy the style of the original textual ui elements.
                    for( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ ) {
                        auto entry = parent.entries[parent.fentries[i]];
                        bool is_selected = i == parent.fselected;
                        ImGui::PushID( i );
                        ImGui::TableNextRow( ImGuiTableRowFlags_None, entry_height );
                        ImGui::TableSetColumnIndex( 0 );

                        if( is_selected && parent.need_to_scroll ) {
                            // this is the selected row, and the user just changed the selection; scroll it into view
                            ImGui::SetScrollHereY();
                            parent.need_to_scroll = false;
                        }
                        ImGuiSelectableFlags flags = ImGuiSelectableFlags_SpanAllColumns |
                                                     ImGuiSelectableFlags_AllowItemOverlap;
                        if( !entry.enabled ) {
                            flags |= ImGuiSelectableFlags_Disabled;
                        }
                        if( ImGui::Selectable( "##s", is_selected, flags ) ) {
                            parent.fselected = i;
                            parent.selected = parent.previewing = parent.fentries[ parent.fselected ];
                            // We are going to return now that the user clicked on something, so scrolling seems
                            // unnecessary. However, the debug spawn item function reuses the same menu to let the
                            // user spawn multiple items and it’s weird if the correct item isn’t scrolled into view
                            // the next time around.
                            parent.need_to_scroll = true;
                            is_selected = parent.clicked = true;
                        }
                        bool is_hovered = ImGui::IsItemHovered( ImGuiHoveredFlags_NoNavOverride );
                        bool mouse_moved = ImGui::GetCurrentContext()->HoveredId !=
                                           ImGui::GetCurrentContext()->HoveredIdPreviousFrame;
                        if( is_hovered && mouse_moved ) {
                            // this row is hovered and the hover state just changed, show context for it
                            parent.previewing = parent.fentries[ i ];
                        }

                        // Force the spacing to be set to the padding value.
                        ImGui::SameLine( 0, style.CellPadding.x );
                        if( entry.hotkey.has_value() ) {
                            cataimgui::draw_colored_text( entry.hotkey.value().short_description(),
                                                          is_selected ? parent.hilight_color : parent.hotkey_color );
                        }

                        std::string &str1 = entry.txt;
                        std::string &str2 = entry.ctxt;
                        nc_color color = entry.enabled || entry.force_color ? entry.text_color : parent.disabled_color;
                        if( is_selected || is_hovered ) {
                            str1 = entry._txt_nocolor();
                            str2 = entry._ctxt_nocolor();
                            color = parent.hilight_color;
                        }

                        ImGui::TableSetColumnIndex( 1 );
                        cataimgui::draw_colored_text( str1, color );

                        ImGui::TableSetColumnIndex( 2 );
                        // Right-align text.
                        ImVec2 curPos = ImGui::GetCursorScreenPos();
                        // Remove the edge padding so that the last pixel just touches the border.
                        ImGui::SetCursorScreenPos( ImVec2( ImMax( 0.0f, curPos.x + style.CellPadding.x ), curPos.y ) );
                        cataimgui::draw_colored_text( str2, color );

                        ImGui::PopID();
                    }
                }
                ImGui::EndTable();
            }
        }
        ImGui::EndChild();

        if( parent.callback != nullptr ) {
            parent.callback->refresh( &parent );
        }
        ImGui::EndTable();
    }

    if( parent.desc_enabled ) {
        ImGui::Separator();
        std::string description;
        if( !parent.footer_text.empty() ) {
            description = parent.footer_text;
        } else {
            description = parent.entries[parent.previewing].desc;
        }
        cataimgui::draw_colored_text( description );
    }
}

catacurses::window new_centered_win( int nlines, int ncols )
{
    int height = std::min( nlines, TERMY );
    int width = std::min( ncols, TERMX );
    point pos( ( TERMX - width ) / 2, ( TERMY - height ) / 2 );
    return catacurses::newwin( height, width, pos );
}

/**
* \defgroup UI "The UI Menu."
* @{
*/

static std::optional<input_event> hotkey_from_char( const int ch )
{
    if( ch == MENU_AUTOASSIGN ) {
        return std::nullopt;
    } else if( ch <= 0 || ch == ' ' ) {
        return input_event();
    }
    switch( input_manager::actual_keyboard_mode( keyboard_mode::keycode ) ) {
        case keyboard_mode::keycode:
            if( ch >= 'A' && ch <= 'Z' ) {
                return input_event( std::set<keymod_t>( { keymod_t::shift } ),
                                    ch - 'A' + 'a', input_event_t::keyboard_code );
            } else {
                return input_event( ch, input_event_t::keyboard_code );
            }
        case keyboard_mode::keychar:
            return input_event( ch, input_event_t::keyboard_char );
    }
    return input_event();
}

uilist_entry::uilist_entry( const std::string &txt )
    : retval( -1 ), enabled( true ), hotkey( std::nullopt ), txt( txt ),
      text_color( c_red_red )
{

}

uilist_entry::uilist_entry( const std::string &txt, const std::string &desc )
    : retval( -1 ), enabled( true ), hotkey( std::nullopt ), txt( txt ),
      desc( desc ), text_color( c_red_red )
{

}

uilist_entry::uilist_entry( const std::string &txt, const int key )
    : retval( -1 ), enabled( true ), hotkey( hotkey_from_char( key ) ), txt( txt ),
      text_color( c_red_red )
{

}

uilist_entry::uilist_entry( const std::string &txt, const std::optional<input_event> &key )
    : retval( -1 ), enabled( true ), hotkey( key ), txt( txt ),
      text_color( c_red_red )
{

}

uilist_entry::uilist_entry( const int retval, const bool enabled, const int key,
                            const std::string &txt )
    : retval( retval ), enabled( enabled ), hotkey( hotkey_from_char( key ) ), txt( txt ),
      text_color( c_red_red )
{

}

uilist_entry::uilist_entry( const int retval, const bool enabled,
                            const std::optional<input_event> &key,
                            const std::string &txt )
    : retval( retval ), enabled( enabled ), hotkey( key ), txt( txt ),
      text_color( c_red_red )
{

}

uilist_entry::uilist_entry( const int retval, const bool enabled, const int key,
                            const std::string &txt, const std::string &desc )
    : retval( retval ), enabled( enabled ), hotkey( hotkey_from_char( key ) ), txt( txt ),
      desc( desc ), text_color( c_red_red )
{

}

uilist_entry::uilist_entry( const int retval, const bool enabled,
                            const std::optional<input_event> &key, const std::string &txt, const std::string &desc )
    : retval( retval ), enabled( enabled ), hotkey( key ), txt( txt ),
      desc( desc ), text_color( c_red_red )
{

}

uilist_entry::uilist_entry( const int retval, const bool enabled, const int key,
                            const std::string &txt, const std::string &desc,
                            const std::string &column )
    : retval( retval ), enabled( enabled ), hotkey( hotkey_from_char( key ) ), txt( txt ),
      desc( desc ), ctxt( column ), text_color( c_red_red )
{

}

uilist_entry::uilist_entry( const int retval, const bool enabled,
                            const std::optional<input_event> &key,
                            const std::string &txt, const std::string &desc,
                            const std::string &column )
    : retval( retval ), enabled( enabled ), hotkey( key ), txt( txt ),
      desc( desc ), ctxt( column ), text_color( c_red_red )
{

}

uilist_entry::uilist_entry( const int retval, const bool enabled, const int key,
                            const std::string &txt,
                            const nc_color &keycolor, const nc_color &txtcolor )
    : retval( retval ), enabled( enabled ), hotkey( hotkey_from_char( key ) ), txt( txt ),
      hotkey_color( keycolor ), text_color( txtcolor )
{

}

const std::string &uilist_entry::_txt_nocolor()
{
    if( _txt_nocolor_cached.empty() && !txt.empty() ) {
        _txt_nocolor_cached = remove_color_tags( txt );
    }
    return _txt_nocolor_cached;
}

const std::string &uilist_entry::_ctxt_nocolor()
{
    if( _ctxt_nocolor_cached.empty() && !ctxt.empty() ) {
        _ctxt_nocolor_cached = remove_color_tags( ctxt );
    }
    return _ctxt_nocolor_cached;
}

uilist::size_scalar &uilist::size_scalar::operator=( auto_assign )
{
    fun = nullptr;
    return *this;
}

uilist::size_scalar &uilist::size_scalar::operator=( const int val )
{
    fun = [val]() -> int {
        return val;
    };
    return *this;
}

uilist::size_scalar &uilist::size_scalar::operator=( const std::function<int()> &fun )
{
    this->fun = fun;
    return *this;
}

uilist::pos_scalar &uilist::pos_scalar::operator=( auto_assign )
{
    fun = nullptr;
    return *this;
}

uilist::pos_scalar &uilist::pos_scalar::operator=( const int val )
{
    fun = [val]( int ) -> int {
        return val;
    };
    return *this;
}

uilist::pos_scalar &uilist::pos_scalar::operator=( const std::function<int( int )> &fun )
{
    this->fun = fun;
    return *this;
}

uilist::uilist()
{
    init();
}

uilist::uilist( const std::string &msg, const std::vector<uilist_entry> &opts )
{
    init();
    text = msg;
    entries = opts;
    query();
}

uilist::uilist( const std::string &msg, const std::vector<std::string> &opts )
{
    init();
    text = msg;
    for( const std::string &opt : opts ) {
        entries.emplace_back( opt );
    }
    query();
}

uilist::uilist( const std::string &msg, std::initializer_list<const char *const> opts )
{
    init();
    text = msg;
    for( const char *const opt : opts ) {
        entries.emplace_back( opt );
    }
    query();
}

uilist::~uilist() = default;

void uilist::color_error( const bool report )
{
    if( report ) {
        _color_error = report_color_error::yes;
    } else {
        _color_error = report_color_error::no;
    }
}

/*
 * Enables oneshot construction -> running -> exit
 */
uilist::operator int() const
{
    return ret;
}

/**
 * Sane defaults on initialization
 */
void uilist::init()
{
    cata_assert( !test_mode ); // uilist should not be used in tests where there's no place for it
    desired_bounds = std::nullopt;
    calculated_bounds = { -1.f, -1.f, -1.f, -1.f };
    calculated_menu_size = { 0.0, 0.0 };
    calculated_hotkey_width = 0.0;
    calculated_label_width = 0.0;
    calculated_secondary_width = 0.0;
    extra_space_left = 0.0;
    extra_space_right = 0.0;
    ret = UILIST_WAIT_INPUT;
    text.clear();          // header text, after (maybe) folding, populates:
    title.clear();         // Makes use of the top border, no folding, sets min width if w_width is auto
    ret_evt = input_event(); // last input event
    keymap.clear();        // keymap[input_event] == index, for entries[index]
    selected = 0;          // current highlight, for entries[index]
    previewing = 0;           // current mouse highlight, for entries[index]
    entries.clear();       // uilist_entry(int returnval, bool enabled, int keycode, std::string text, ... TODO: submenu stuff)
    started = false;       // set to true when width and key calculations are done, and window is generated.
    desc_enabled = false;  // don't show option description by default
    footer_text.clear();   // takes precedence over per-entry descriptions.
    border_color = c_magenta; // border color
    text_color = c_light_gray;  // text color
    title_color = c_green;  // title color
    hilight_color = h_white; // highlight for up/down selection bar
    hotkey_color = c_light_green; // hotkey text to the right of menu entry's text
    disabled_color = c_dark_gray; // disabled menu entry
    allow_disabled = false;  // disallow selecting disabled options
    allow_anykey = false;    // do not return on unbound keys
    allow_cancel = true;     // allow canceling with "UILIST.QUIT" action
    allow_confirm = true;     // allow confirming with confirm action
    allow_additional = false; // do not return on unhandled additional actions
    hilight_disabled =
        false; // if false, hitting 'down' onto a disabled entry will advance downward to the first enabled entry
    vshift = 0;              // scrolling menu offset
    vmax = 0;                // max entries area rows
    callback = nullptr;         // * uilist_callback
    filter.clear();          // filter string. If "", show everything
    fentries.clear();        // fentries is the actual display after filtering, and maps displayed entry number to actual entry number
    fselected = 0;           // selected = fentries[fselected]
    filtering = true;        // enable list display filtering via '/' or '.'
    filtering_nocase = true; // ignore case when filtering
    max_entry_len = 0;
    max_column_len = 0;      // for calculating space for second column

    categories.clear();
    switch_to_category = current_category = 0;

    input_category = "UILIST";
    additional_actions.clear();
}

input_context uilist::create_main_input_context() const
{
    input_context ctxt( input_category, keyboard_mode::keycode );
    ctxt.register_action( "UILIST.UP" );
    ctxt.register_action( "UILIST.DOWN" );
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "HOME", to_translation( "Go to first entry" ) );
    ctxt.register_action( "END", to_translation( "Go to last entry" ) );
    if( allow_cancel ) {
        ctxt.register_action( "UILIST.QUIT" );
    }
    ctxt.register_action( "MOUSE_MOVE" );
    if( allow_confirm ) {
        ctxt.register_action( "CONFIRM" );
        ctxt.register_action( "SELECT" );
    }
    ctxt.register_action( "UILIST.FILTER" );
    if( !categories.empty() ) {
        ctxt.register_action( "UILIST.LEFT" );
        ctxt.register_action( "UILIST.RIGHT" );
    }
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    for( const auto &additional_action : additional_actions ) {
        ctxt.register_action( additional_action.first, additional_action.second );
    }
    return ctxt;
}

/**
 * repopulate filtered entries list (fentries) and set fselected accordingly
 */
void uilist::filterlist()
{
    // TODO: && is_all_lc( filter )
    fentries.clear();
    fselected = -1;
    int f = 0;
    for( size_t i = 0; i < entries.size(); i++ ) {
        bool visible = true;
        if( !categories.empty() && !category_filter( entries[i], categories[current_category].first ) ) {
            continue;
        }
        if( filtering && !filter.empty() ) {
            if( filtering_nocase ) {
                // case-insensitive match
                visible = lcmatch( entries[i].txt, filter );
            } else {
                // case-sensitive match
                visible = entries[i].txt.find( filter ) != std::string::npos;
            }
        }
        if( visible ) {
            fentries.push_back( static_cast<int>( i ) );
            if( hilight_disabled || entries[i].enabled ) {
                if( static_cast<int>( i ) == selected || ( static_cast<int>( i ) > selected && fselected == -1 ) ) {
                    // Either this is selected, or we are past the previously selected entry,
                    // which has been filtered out, so choose another nearby entry instead.
                    fselected = f;
                }
            }
            f++;
        }
    }
    if( fselected == -1 ) {
        fselected = 0;
        vshift = 0;
        if( fentries.empty() ) {
            selected = -1;
        } else {
            previewing = selected = fentries [ 0 ];
        }
    } else if( fselected < static_cast<int>( fentries.size() ) ) {
        previewing = selected = fentries[fselected];
    } else {
        previewing = fselected = selected = -1;
    }
    // scroll to top of screen if all remaining entries fit the screen.
    if( static_cast<int>( fentries.size() ) <= vmax ) {
        vshift = 0;
    }
    if( callback != nullptr ) {
        callback->select( this );
    }
}

void uilist::inputfilter()
{
    filter_popup = std::make_unique<string_input_popup_imgui>( 0, filter );
    filter_popup->set_max_input_length( 256 );
    filter = filter_popup->query();
    if( filter_popup->cancelled() ) {
        filter.clear();
    }
    filterlist();
    filter_popup.reset();
}

static ImVec2 calc_size( std::string_view line )
{
    return ImGui::CalcTextSize( remove_color_tags( line ).c_str() );
}

void uilist::calc_data()
{
    ImGuiStyle s = ImGui::GetStyle();

    std::vector<int> autoassign;
    for( size_t i = 0; i < entries.size(); i++ ) {
        if( entries[ i ].enabled ) {
            if( !entries[i].hotkey.has_value() ) {
                autoassign.emplace_back( static_cast<int>( i ) );
            } else if( entries[i].hotkey.value() != input_event() ) {
                keymap[entries[i].hotkey.value()] = i;
            }
            if( entries[ i ].retval == -1 ) {
                entries[ i ].retval = i;
            }
        }
        if( entries[ i ].text_color == c_red_red ) {
            entries[ i ].text_color = text_color;
        }
    }
    input_context ctxt = create_main_input_context();
    const hotkey_queue &hotkeys = hotkey_queue::alpha_digits();
    input_event hotkey = ctxt.first_unassigned_hotkey( hotkeys );
    for( auto it = autoassign.begin(); it != autoassign.end() &&
         hotkey != input_event(); ++it ) {
        bool assigned = false;
        do {
            if( keymap.count( hotkey ) == 0 ) {
                entries[*it].hotkey = hotkey;
                keymap[hotkey] = *it;
                assigned = true;
            }
            hotkey = ctxt.next_unassigned_hotkey( hotkeys, hotkey );
        } while( !assigned && hotkey != input_event() );
    }

    vmax = entries.size();

    ImVec2 title_size = ImVec2();
    bool has_titlebar = !title.empty();
    if( has_titlebar ) {
        title_size = calc_size( title );
        float expected_num_lines = title_size.y / ImGui::GetTextLineHeight();
        title_size.y += ( s.ItemSpacing.y * expected_num_lines ) + ( s.ItemSpacing.y * 2.0 );
    }

    ImVec2 text_size = ImVec2();
    if( !text.empty() ) {
        text_size = calc_size( text );
        float expected_num_lines = text_size.y / ImGui::GetTextLineHeight();
        text_size.y += ( s.ItemSpacing.y * expected_num_lines ) + ( s.ItemSpacing.y * 2.0 );
    }

    ImVec2 tabs_size = ImVec2();
    if( !categories.empty() ) {
        tabs_size.y = ImGui::GetTextLineHeightWithSpacing() + ( 2.0 * s.FramePadding.y );
    }

    ImVec2 desc_size = ImVec2();
    if( desc_enabled ) {
        desc_size = calc_size( footer_text );
        for( const uilist_entry &ent : entries ) {
            ImVec2 entry_size = calc_size( ent.desc );
            desc_size.x = std::max( desc_size.x, entry_size.x );
            desc_size.y = std::max( desc_size.y, entry_size.y );
        }
        if( desc_size.y <= 0.0 ) {
            desc_enabled = false;
        }
        float expected_num_lines = desc_size.y / ImGui::GetTextLineHeight();
        desc_size.y += ( s.ItemSpacing.y * expected_num_lines ) + ( s.ItemSpacing.y * 2.0 );
    }
    float additional_height = title_size.y + text_size.y + desc_size.y + tabs_size.y +
                              2.0 * ( s.FramePadding.y + s.WindowBorderSize );

    if( vmax * ImGui::GetTextLineHeightWithSpacing() + additional_height >
        0.9 * ImGui::GetMainViewport()->Size.y ) {
        vmax = floorf( ( 0.9 * ImGui::GetMainViewport()->Size.y - additional_height +
                         ( s.FramePadding.y * 2.0 ) ) /
                       ImGui::GetTextLineHeightWithSpacing() );
    }

    float padding = 2.0f * s.CellPadding.x;
    calculated_hotkey_width = ImGui::CalcTextSize( "M" ).x;
    calculated_label_width = 0.0;
    calculated_secondary_width = 0.0;
    for( const uilist_entry &entry : entries ) {
        calculated_label_width = std::max( calculated_label_width, calc_size( entry.txt ).x );
        calculated_secondary_width = std::max( calculated_secondary_width,
                                               calc_size( entry.ctxt ).x );
    }
    calculated_menu_size = { 0.0, 0.0 };
    calculated_menu_size.x += calculated_hotkey_width + padding;
    calculated_menu_size.x += calculated_label_width + padding;
    calculated_menu_size.x += calculated_secondary_width + padding;
    float max_avail_height = 0.9 * ImGui::GetMainViewport()->Size.y;
    if( desired_bounds.has_value() ) {
        float desired_height = desired_bounds.value().h;
        if( desired_height != -1 ) {
            max_avail_height = std::min( max_avail_height, desired_height );
        }
    }
    calculated_menu_size.y = std::min(
                                 max_avail_height - additional_height + ( s.FramePadding.y * 2.0 ),
                                 vmax * ImGui::GetTextLineHeightWithSpacing() + ( s.FramePadding.y * 2.0 ) );

    extra_space_left = 0.0;
    extra_space_right = 0.0;
    if( callback != nullptr ) {
        extra_space_left = callback->desired_extra_space_left( ) + s.FramePadding.x;
        extra_space_right = callback->desired_extra_space_right( ) + s.FramePadding.x;
    }

    float longest_line_width = std::max( { title_size.x, text_size.x,
                                           calculated_menu_size.x, desc_size.x } );
    calculated_bounds.w = extra_space_left + extra_space_right + longest_line_width
                          + 2 * ( s.WindowPadding.x + s.WindowBorderSize );
    calculated_bounds.h = calculated_menu_size.y + additional_height;

    if( desired_bounds.has_value() ) {
        cataimgui::bounds b = desired_bounds.value();
        bool h_neg = b.h < 0.0f;
        bool w_neg = b.w < 0.0f;
        bool both_neg = h_neg && w_neg;
        if( !both_neg ) {
            if( h_neg ) {
                desired_bounds->h = calculated_bounds.h;
            }
            if( w_neg ) {
                desired_bounds->w = calculated_bounds.w;
            }
        }
    }

    if( longest_line_width > calculated_menu_size.x ) {
        calculated_menu_size.x = longest_line_width;
        calculated_label_width = calculated_menu_size.x - calculated_hotkey_width - padding -
                                 calculated_secondary_width - padding - padding;
    }
}

void uilist::setup()
{
    if( !started ) {
        filterlist();
    }

    calc_data();

    started = true;
    recalc_start = true;
}

void uilist::reposition()
{
    setup();
}


int uilist::scroll_amount_from_action( const std::string &action )
{
    const int scroll_rate = vmax > 20 ? 10 : 3;
    if( action == "UILIST.UP" ) {
        return -1;
    } else if( action == "PAGE_UP" ) {
        return -scroll_rate;
    } else if( action == "HOME" ) {
        return -fselected;
    } else if( action == "END" ) {
        return fentries.size() - fselected - 1;
    } else if( action == "UILIST.DOWN" ) {
        return 1;
    } else if( action == "PAGE_DOWN" ) {
        return scroll_rate;
    } else {
        return 0;
    }
}

/**
 * check for valid scrolling keypress and handle. return false if invalid keypress
 */
bool uilist::scrollby( const int scrollby )
{
    if( scrollby == 0 ) {
        return false;
    }

    bool looparound = scrollby == -1 || scrollby == 1;
    bool backwards = scrollby < 0;
    int recmax = static_cast<int>( fentries.size() );

    fselected += scrollby;
    if( !looparound ) {
        if( backwards && fselected < 0 ) {
            fselected = 0;
        } else if( fselected >= recmax ) {
            fselected = fentries.size() - 1;
        }
    }

    if( backwards ) {
        if( fselected < 0 ) {
            fselected = fentries.size() - 1;
        }
        for( size_t i = 0; i < fentries.size(); ++i ) {
            if( hilight_disabled || entries[ fentries [ fselected ] ].enabled ) {
                break;
            }
            --fselected;
            if( fselected < 0 ) {
                fselected = fentries.size() - 1;
            }
        }
    } else {
        if( fselected >= recmax ) {
            fselected = 0;
        }
        for( size_t i = 0; i < fentries.size(); ++i ) {
            if( hilight_disabled || entries[ fentries [ fselected ] ].enabled ) {
                break;
            }
            ++fselected;
            if( fselected >= recmax ) {
                fselected = 0;
            }
        }
    }
    if( static_cast<size_t>( fselected ) < fentries.size() ) {
        selected = previewing = fentries [ fselected ];
        if( callback != nullptr ) {
            callback->select( this );
        }
    }
    return true;
}

shared_ptr_fast<uilist_impl> uilist::create_or_get_ui()
{
    shared_ptr_fast<uilist_impl> current_ui = ui.lock();
    if( !current_ui ) {
        if( title.empty() ) {
            ui = current_ui = make_shared_fast<uilist_impl>( *this );
        } else {
            ui = current_ui = make_shared_fast<uilist_impl>( *this, title );
        }
    }
    return current_ui;
}

/**
 * Handle input and update display
 *
 */
void uilist::query( bool loop, int timeout, bool allow_unfiltered_hotkeys )
{
#if defined(__ANDROID__)
    if( get_option<bool>( "ANDROID_NATIVE_UI" ) && !entries.empty() && !desired_bounds ) {
        if( !started ) {
            calc_data();
            started = true;
        }
        JNIEnv *env = ( JNIEnv * )SDL_AndroidGetJNIEnv();
        jobject activity = ( jobject )SDL_AndroidGetActivity();
        jclass clazz( env->GetObjectClass( activity ) );
        jmethodID get_nativeui_method_id = env->GetMethodID( clazz, "getNativeUI",
                                           "()Lcom/cleverraven/cataclysmdda/NativeUI;" );
        jobject native_ui_obj = env->CallObjectMethod( activity, get_nativeui_method_id );
        jclass native_ui_cls( env->GetObjectClass( native_ui_obj ) );
        jmethodID list_menu_method_id = env->GetMethodID( native_ui_cls, "singleChoiceList",
                                        "(Ljava/lang/String;[Ljava/lang/String;[Z)I" );
        jstring jstr_message = env->NewStringUTF( text.c_str() );
        jobjectArray j_options = env->NewObjectArray( entries.size(), env->FindClass( "java/lang/String" ),
                                 env->NewStringUTF( "" ) );
        jbooleanArray j_enabled = env->NewBooleanArray( entries.size() );
        jboolean *n_enabled = new jboolean[entries.size()];
        for( std::size_t i = 0; i < entries.size(); i++ ) {
            std::string entry = remove_color_tags( entries[i].txt );
            if( !entries[i].ctxt.empty() ) {
                std::string ctxt = remove_color_tags( entries[i].ctxt );
                while( !ctxt.empty() && ctxt.back() == '\n' ) {
                    ctxt.pop_back();
                }
                if( !ctxt.empty() ) {
                    str_append( entry, "\n", ctxt );
                }
            }
            if( desc_enabled ) {
                std::string desc = remove_color_tags( entries[i].desc );
                while( !desc.empty() && desc.back() == '\n' ) {
                    desc.pop_back();
                }
                if( !desc.empty() ) {
                    str_append( entry, "\n", desc );
                }
            }
            env->SetObjectArrayElement( j_options, i, env->NewStringUTF( entry.c_str() ) );
            n_enabled[i] = entries[i].enabled;
        }
        env->SetBooleanArrayRegion( j_enabled, 0, entries.size(), n_enabled );
        int j_ret = env->CallIntMethod( native_ui_obj, list_menu_method_id, jstr_message, j_options,
                                        j_enabled );
        env->DeleteLocalRef( j_enabled );
        env->DeleteLocalRef( j_options );
        env->DeleteLocalRef( jstr_message );
        env->DeleteLocalRef( native_ui_cls );
        env->DeleteLocalRef( native_ui_obj );
        env->DeleteLocalRef( clazz );
        env->DeleteLocalRef( activity );
        delete[] n_enabled;
        if( j_ret == -1 ) {
            ret = UILIST_CANCEL;
        } else if( 0 <= j_ret && j_ret < static_cast<int>( entries.size() ) ) {
            ret = entries[j_ret].retval;
        } else {
            ret = UILIST_ERROR;
        }
        return;
    }
#endif
    ret_evt = input_event();
    if( entries.empty() ) {
        ret = UILIST_ERROR;
        return;
    }
    ret = UILIST_WAIT_INPUT;

    input_context ctxt = create_main_input_context();

    shared_ptr_fast<uilist_impl> ui = create_or_get_ui();

#if defined(__ANDROID__)
    for( const auto &entry : entries ) {
        if( entry.enabled && entry.hotkey.has_value()
            && entry.hotkey.value() != input_event() ) {
            ctxt.register_manual_key( entry.hotkey.value().get_first_input(), entry.txt );
        }
    }
#endif

    do {
        ui_manager::redraw();

        ret_act = ctxt.handle_input( timeout );
        const input_event event = ctxt.get_raw_input();
        ret_evt = event;
        const auto iter = keymap.find( ret_evt );
        recalc_start = false;

        if( scrollby( scroll_amount_from_action( ret_act ) ) ) {
            need_to_scroll = true;
            recalc_start = true;
        } else if( filtering && ret_act == "UILIST.FILTER" ) {
            inputfilter();
        } else if( !categories.empty() && ( ret_act == "UILIST.LEFT" || ret_act == "UILIST.RIGHT" ) ) {
            int tmp = current_category + ( ret_act == "UILIST.LEFT" ? -1 : 1 );
            if( tmp < 0 ) {
                tmp = categories.size() - 1;
            } else if( tmp >= static_cast<int>( categories.size() ) ) {
                tmp = 0;
            }
            switch_to_category = static_cast<size_t>( tmp );
        } else if( iter != keymap.end() ) {
            if( allow_unfiltered_hotkeys ) {
                const bool enabled = entries[iter->second].enabled;
                if( enabled || allow_disabled ) {
                    ret = entries[iter->second].retval;
                }
            } else {
                const auto it = std::find( fentries.begin(), fentries.end(), iter->second );
                if( it != fentries.end() ) {
                    const bool enabled = entries[*it].enabled;
                    if( enabled || allow_disabled || hilight_disabled ) {
                        // Change the selection to display correctly when this function
                        // is called again.
                        fselected = std::distance( fentries.begin(), it );
                        selected = *it;
                        if( enabled || allow_disabled ) {
                            ret = entries[selected].retval;
                        }
                        if( callback != nullptr ) {
                            callback->select( this );
                        }
                    }
                }
            }
        } else if( allow_confirm && !fentries.empty() && ( clicked || ret_act == "CONFIRM" ) ) {
            clicked = false;
            if( entries[ selected ].enabled || allow_disabled ) {
                ret = entries[selected].retval;
            }
        } else if( allow_cancel && ret_act == "UILIST.QUIT" ) {
            ret = UILIST_CANCEL;
        } else if( ret_act == "TIMEOUT" ) {
            ret = UILIST_WAIT_INPUT;
        } else {
            // including HELP_KEYBINDINGS, in case the caller wants to refresh their contents
            bool unhandled = callback == nullptr || !callback->key( ctxt, event, selected, this );
            if( unhandled && allow_anykey ) {
                ret = UILIST_UNBOUND;
            } else if( unhandled && allow_additional ) {
                recalc_start = true;
                for( const auto &it : additional_actions ) {
                    if( it.first == ret_act ) {
                        ret = UILIST_ADDITIONAL;
                        break;
                    }
                }
            }
        }
    } while( loop && ret == UILIST_WAIT_INPUT );
}

///@}
/**
 * cleanup
 */
void uilist::reset()
{
    init();
}

void uilist::addentry( const std::string &txt )
{
    entries.emplace_back( txt );
}

void uilist::addentry( int retval, bool enabled, int key, const std::string &txt )
{
    entries.emplace_back( retval, enabled, key, txt );
}

void uilist::addentry( const int retval, const bool enabled,
                       const std::optional<input_event> &key,
                       const std::string &txt )
{
    entries.emplace_back( retval, enabled, key, txt );
}

void uilist::addentry_desc( const std::string &txt, const std::string &desc )
{
    entries.emplace_back( txt, desc );
}

void uilist::addentry_desc( int retval, bool enabled, int key, const std::string &txt,
                            const std::string &desc )
{
    entries.emplace_back( retval, enabled, key, txt, desc );
}

void uilist::addentry_desc( int retval, bool enabled, const std::optional<input_event> &key,
                            const std::string &txt, const std::string &desc )
{
    entries.emplace_back( retval, enabled, key, txt, desc );
}

void uilist::addentry_col( int retval, bool enabled, int key, const std::string &txt,
                           const std::string &column,
                           const std::string &desc )
{
    entries.emplace_back( retval, enabled, key, txt, desc, column );
}

void uilist::addentry_col( const int retval, const bool enabled,
                           const std::optional<input_event> &key,
                           const std::string &txt, const std::string &column,
                           const std::string &desc )
{
    entries.emplace_back( retval, enabled, key, txt, desc, column );
}

void uilist::settext( const std::string &str )
{
    text = str;
}

void uilist::set_selected( int index )
{
    selected = previewing = std::clamp( index, 0, static_cast<int>( entries.size() - 1 ) );
}

void uilist::add_category( const std::string &key, const std::string &name )
{
    categories.emplace_back( key, name );
}

void uilist::set_category( const std::string &key )
{
    const auto it = std::find_if( categories.begin(),
    categories.end(), [key]( std::pair<std::string, std::string> &pair ) {
        return pair.first == key;
    } );
    current_category = std::distance( categories.begin(), it );
}

void uilist::set_category_filter( const
                                  std::function<bool( const uilist_entry &, const std::string & )> &fun )
{
    category_filter = fun;
}

void uimenu::addentry( int retval, bool enabled, const std::vector<std::string> &col_content )
{
    cata_assert( static_cast<int>( col_content.size() ) == col_count );
    cols.emplace_back( retval, enabled, col_content );
}

void uimenu::finalize_addentries()
{
    menu.entries.clear();
    std::vector<int> maxes( col_count, 0 );
    // get max width of each column
    for( col &c : cols ) {
        int i = 0;
        for( const std::string &entry : c.col_content ) {
            maxes[i] = std::max( maxes[i], utf8_width( entry, true ) );
            ++i;
        }
    }
    // adding spacing between columns
    int free_width = suggest_width - std::reduce( maxes.begin(), maxes.end() );
    int spacing = std::min( 3, col_count > 1 ? free_width / ( col_count - 1 ) : 0 );
    if( spacing > 0 ) {
        for( int i = 0; i < col_count - 1; ++i ) {
            maxes[i] += spacing;
        }
    }

    for( col &c : cols ) {
        std::string row;
        int i = 0;
        for( const std::string &entry : c.col_content ) {
            // Pad with spaces
            // Add length of tags to number of spaces to pad with
            // That is length(entry_with_tags) - length(entry_without_tags)
            // Otherwise the entry padding will be shorter by number of chars in tags
            int entry_len_plus_tags = maxes[i] + utf8_width( entry, false ) - utf8_width( entry, true );
            row += string_format( "%-*s", entry_len_plus_tags, entry );
            ++i;
        }
        menu.addentry( c.retval, c.enabled, -1, row );
    }
}

void uimenu::set_selected( int index )
{
    menu.selected = menu.previewing = index;
}

void uimenu::set_title( const std::string &title )
{
    menu.title = title;
}

int uimenu::query()
{
    finalize_addentries();
    menu.query();
    return menu.ret;
}

struct pointmenu_cb::impl_t {
    const std::vector< tripoint_bub_ms > &points;
    int last; // to suppress redrawing
    tripoint_rel_ms last_view; // to reposition the view after selecting
    shared_ptr_fast<game::draw_callback_t> terrain_draw_cb;

    explicit impl_t( const std::vector<tripoint_bub_ms> &pts );
    ~impl_t();

    void select( uilist *menu );
};

pointmenu_cb::impl_t::impl_t( const std::vector<tripoint_bub_ms> &pts ) : points( pts )
{
    last = INT_MIN;
    avatar &player_character = get_avatar();
    last_view = player_character.view_offset;
    terrain_draw_cb = make_shared_fast<game::draw_callback_t>( [this, &player_character]() {
        if( last >= 0 && static_cast<size_t>( last ) < points.size() ) {
            g->draw_trail_to_square( player_character.view_offset, true );
        }
    } );
    g->add_draw_callback( terrain_draw_cb );
}

pointmenu_cb::impl_t::~impl_t()
{
    get_avatar().view_offset = last_view;
}

void pointmenu_cb::impl_t::select( uilist *const menu )
{
    if( last == menu->selected ) {
        return;
    }
    last = menu->selected;
    avatar &player_character = get_avatar();
    if( menu->selected < 0 || menu->selected >= static_cast<int>( points.size() ) ) {
        player_character.view_offset = tripoint_rel_ms::zero;
    } else {
        const tripoint_bub_ms &center = points[menu->selected];
        player_character.view_offset = center - player_character.pos_bub();
        // TODO: Remove this line when it's safe
        player_character.view_offset.z() = 0;
    }
    g->invalidate_main_ui_adaptor();
}


pointmenu_cb::pointmenu_cb( const std::vector<tripoint_bub_ms> &pts ) : impl( pts )
{
}

pointmenu_cb::~pointmenu_cb() = default;

void pointmenu_cb::select( uilist *const menu )
{
    impl->select( menu );
}

template bool navigate_ui_list<int, int>( const std::string &action, int &val, int page_delta,
        int size, bool wrap );
template bool navigate_ui_list<int, size_t>( const std::string &action, int &val, int page_delta,
        size_t size, bool wrap );
template bool navigate_ui_list<size_t, size_t>( const std::string &action, size_t &val,
        int page_delta, size_t size, bool wrap );
template bool navigate_ui_list<unsigned int, unsigned int>( const std::string &action,
        unsigned int &val,
        int page_delta, unsigned int size, bool wrap );

// Templating of existing `unsigned int` triggers linter rules against `unsigned long`
// NOLINTs below are to address
template<typename V, typename S>
bool navigate_ui_list( const std::string &action, V &val, int page_delta, S size, bool wrap )
{
    if( action == "UP" || action == "SCROLL_UP" || action == "DOWN" || action == "SCROLL_DOWN" ) {
        if( wrap ) {
            // NOLINTNEXTLINE(cata-no-long)
            val = inc_clamp_wrap( val, action == "DOWN" || action == "SCROLL_DOWN", static_cast<V>( size ) );
        } else {
            val = inc_clamp( val, action == "DOWN" || action == "SCROLL_DOWN",
                             // NOLINTNEXTLINE(cata-no-long)
                             static_cast<V>( size ? size - 1 : 0 ) );
        }
    } else if( ( action == "PAGE_UP" || action == "PAGE_DOWN" ) && page_delta ) {
        // page navigation never wraps
        val = inc_clamp( val, action == "PAGE_UP" ? -page_delta : page_delta,
                         // NOLINTNEXTLINE(cata-no-long)
                         static_cast<V>( size ? size - 1 : 0 ) );
    } else if( action == "HOME" ) {
        // NOLINTNEXTLINE(cata-no-long)
        val = static_cast<V>( 0 );
    } else if( action == "END" ) {
        // NOLINTNEXTLINE(cata-no-long)
        val = static_cast<V>( size ? size - 1 : 0 );
    } else {
        return false;
    }
    return true;
}

std::pair<int, int> subindex_around_cursor( int num_entries, int available_space, int cursor_pos,
        bool focused )
{
    if( !focused || num_entries <= available_space ) {
        return { 0, std::min( available_space, num_entries ) };
    }
    int slice_start = std::min( std::max( 0, cursor_pos - available_space / 2 ),
                                num_entries - available_space );
    return {slice_start, slice_start + available_space };
}
