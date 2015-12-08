#include "cata_ui.h"

#include "catacharset.h"
#include "output.h"
#include "input.h"

#include <cmath>
#include <array>
#include <unordered_set>

ui_rect::ui_rect( size_t size_x, size_t size_y, int x, int y ) : size_x( size_x ), size_y( size_y ), x( x ), y( y )
{
}

ui_window::ui_window( const ui_rect &rect, ui_anchor anchor ) : ui_element( rect, anchor ),
                    global_x( get_ax() ), global_y( get_ay() ), win( newwin( rect.size_y, rect.size_x, global_y, global_x ) )
{
    ctxt.register_directions();
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("NEXT_TAB");

    ctxt.register_action("QUIT");
    ctxt.register_action("CONFIRM");
}

ui_window::ui_window( size_t size_x, size_t size_y, int x, int y, ui_anchor anchor ) : ui_window( ui_rect( size_x, size_y, x, y ), anchor )
{
}

ui_window::~ui_window()
{
    for( auto c : children ){
        delete c;
    }

    delwin( win );
}

void ui_window::draw( std::vector<WINDOW *> &render_batch )
{
    werase( win );
    draw( win );
    render_batch.push_back( win );
    draw_children( render_batch );
}

void ui_window::draw()
{
    std::vector<WINDOW *> batch;
    draw( batch );

    std::unordered_set<WINDOW *> had;
    for( auto win : batch ) {
        if( had.count( win ) == 0 ) { // O(1) average case for hash-set lookups
            wrefresh( win );
            had.insert( win );
        }
    }
}

void ui_window::draw_children( std::vector<WINDOW *> &render_batch )
{
    for( auto child : children ) {
        if( child->is_visible() ) {
            child->draw( render_batch );
        }
    }
}

input_context &ui_window::get_input_context()
{
    return ctxt;
}

std::string ui_window::handle_input()
{
    std::string action = ctxt.handle_input();

    for( auto child : children ) {
        if( child->is_visible() ) {
            child->send_action( action );
        }
    }

    return action;
}

void ui_window::calc_anchored_values() {
    ui_element::calc_anchored_values();

    global_x = get_ax();
    global_y = get_ay();

    if( parent != nullptr ) {
        global_x += parent->global_x;
        global_y += parent->global_y;
    }

    delwin( win );
    win = newwin( get_rect().size_y, get_rect().size_x, global_y, global_x );

    for( auto child : children ) {
        child->calc_anchored_values();
    }
}

WINDOW *ui_window::get_win() const
{
    return win;
}

ui_element::ui_element( const ui_rect &rect, ui_anchor anchor ) : anchor( anchor ), anchored_x( rect.x ), anchored_y( rect.y ), rect( rect )
{
}

ui_element::ui_element( size_t size_x, size_t size_y, int x, int y, ui_anchor anchor ) : ui_element( ui_rect( size_x, size_y, x, y ), anchor )
{
}

void ui_element::draw( std::vector<WINDOW *> &render_batch )
{
    auto win = get_win();
    if( win == nullptr ) {
        return;
    }

    draw( win );
    render_batch.push_back( win );
}

void ui_element::set_visible( bool visible )
{
    show = visible;
}

bool ui_element::is_visible() const
{
    return show;
}

void ui_element::above( const ui_element &other, int x, int y )
{
    auto o_rect = other.get_rect();
    rect.x = o_rect.x + x;
    rect.y = o_rect.y - o_rect.size_y + y;
    set_anchor( other.get_anchor() );
}

void ui_element::below( const ui_element &other, int x, int y )
{
    auto o_rect = other.get_rect();
    rect.x = o_rect.x + x;
    rect.y = o_rect.y + o_rect.size_y + y;
    set_anchor( other.get_anchor() );
}

void ui_element::after( const ui_element &other, int x, int y )
{
    auto o_rect = other.get_rect();
    rect.x = o_rect.x + o_rect.size_x + x;
    rect.y = o_rect.y + y;
    set_anchor( other.get_anchor() );
}

void ui_element::before( const ui_element &other, int x, int y )
{
    auto o_rect = other.get_rect();
    rect.x = o_rect.x - o_rect.size_x + x;
    rect.y = o_rect.y + y;
    set_anchor( other.get_anchor() );
}

void ui_element::set_rect( const ui_rect &new_rect )
{
    rect = new_rect;
    calc_anchored_values();
}

ui_anchor ui_element::get_anchor() const
{
    return anchor;
}

void ui_element::set_anchor( ui_anchor new_anchor )
{
    anchor = new_anchor;
    calc_anchored_values();
}

void ui_element::calc_anchored_values()
{
    if( parent != nullptr ) {
        auto p_rect = parent->get_rect();
        switch( anchor ) {
            case ui_anchor::top_left:
                break;
            case ui_anchor::top_center:
                anchored_x = ( p_rect.size_x / 2 ) - ( rect.size_x / 2 ) + rect.x;
                anchored_y = rect.y;
                return;
            case ui_anchor::top_right:
                anchored_x = p_rect.size_x - rect.size_x + rect.x;
                anchored_y = rect.y;
                return;
            case ui_anchor::center_left:
                anchored_x = rect.x;
                anchored_y = ( p_rect.size_y / 2 ) - ( rect.size_y / 2 ) + rect.y;
                return;
            case ui_anchor::center_center:
                anchored_x = ( p_rect.size_x / 2 ) - ( rect.size_x / 2 ) + rect.x;
                anchored_y = ( p_rect.size_y / 2 ) - ( rect.size_y / 2 ) + rect.y;
                return;
            case ui_anchor::center_right:
                anchored_x = p_rect.size_x - rect.size_x + rect.x;
                anchored_y = ( p_rect.size_y / 2 ) - ( rect.size_y / 2 ) + rect.y;
                return;
            case ui_anchor::bottom_left:
                anchored_x = rect.x;
                anchored_y = p_rect.size_y - rect.size_y + rect.y;
                return;
            case ui_anchor::bottom_center:
                anchored_x = ( p_rect.size_x / 2 ) - ( rect.size_x / 2 ) + rect.x;
                anchored_y = p_rect.size_y - rect.size_y + rect.y;
                return;
            case ui_anchor::bottom_right:
                anchored_x = p_rect.size_x - rect.size_x + rect.x;
                anchored_y = p_rect.size_y - rect.size_y + rect.y;
                return;
        }
    }
    anchored_x = rect.x;
    anchored_y = rect.y;
}

unsigned int ui_element::get_ax() const
{
    return anchored_x;
}

unsigned int ui_element::get_ay() const
{
    return anchored_y;
}

void ui_element::set_parent( const ui_window *parent )
{
    this->parent = parent;
    calc_anchored_values();
}

const ui_rect &ui_element::get_rect() const
{
    return rect;
}

WINDOW *ui_element::get_win() const
{
    if( parent != nullptr ) {
        return parent->get_win();
    }
    return nullptr;
}

ui_label::ui_label( std::string text ,int x, int y, ui_anchor anchor ) : ui_element( utf8_width( text.c_str() ), 1, x, y, anchor ), text( text )
{
}

void ui_label::draw( WINDOW *win )
{
    mvwprintz( win, get_ay(), get_ax(), text_color, "%s", text.c_str() );
}

void ui_label::set_text( std::string new_text )
{
    text = new_text;
    set_rect( ui_rect( utf8_width( new_text.c_str() ), get_rect().size_y, get_rect().x, get_rect().y ) );
}

bordered_window::bordered_window( size_t size_x, size_t size_y, int x, int y ) : ui_window( size_x, size_y, x, y )
{
}

void bordered_window::draw( WINDOW *win )
{
    ui_window::draw( win );

    wattron( win, border_color );
    wborder( win, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    wattroff( win, border_color );
}

tabbed_window::tabbed_window( size_t size_x, size_t size_y, int x, int y ) : bordered_window( size_x, size_y, x, y )
{
}

tabbed_window::~tabbed_window()
{
    for( auto t : tabs ) {
        delete t.second;
    }
}

void tabbed_window::draw( WINDOW *win )
{
    bordered_window::draw( win );

    //erase the top 3 rows
    for( unsigned int y = 0; y < 3; y++ ) {
        for( unsigned int x = 0; x < get_rect().size_x; x++ ){
            mvwputch( win, y, x, border_color, ' ' );
        }
    }

    for ( unsigned int i = 0; i < get_rect().size_x; i++ ) {
        mvwputch( win, 2, i, border_color, LINE_OXOX );
    }

    mvwputch( win, 2,  0, border_color, LINE_OXXO ); // |^
    mvwputch( win, 2, get_rect().size_x - 1, border_color, LINE_OOXX ); // ^|

    int x_offset = 1; // leave space for selection bracket
    for( unsigned int i = 0; i < tabs.size(); i++ ) {
        x_offset += draw_tab( win, x_offset, tabs[i].first, tab_index == i ) + 2;
    }
}

void tabbed_window::next_tab()
{
    if( tabs.size() > 1 ) {
        tabs[tab_index].second->set_visible( false );
        if( tab_index == tabs.size() - 1 ){
            tab_index = 0;
        } else {
            tab_index++;
        }
        tabs[tab_index].second->set_visible( true );

        on_scroll();
    }
}

void tabbed_window::previous_tab()
{
    if( tabs.size() > 1 ) {
        tabs[tab_index].second->set_visible( false );
        if( tab_index == 0 ){
            tab_index = tabs.size() - 1;
        } else {
            tab_index--;
        }
        tabs[tab_index].second->set_visible( true );

        on_scroll();
    }
}

std::string tabbed_window::current_tab() const
{
    if( tabs.empty() ) {
        return "";
    }
    return tabs[tab_index].first;
}

std::string tabbed_window::handle_input()
{
    std::string action = ui_window::handle_input();

    if( action == "PREV_TAB" ) {
        previous_tab();
    } else if( action == "NEXT_TAB" ) {
        next_tab();
    }

    return action;
}

ui_horizontal_list::ui_horizontal_list( int x, int y, ui_anchor anchor ) : ui_element( 0, 1, x, y, anchor )
{
}

void ui_horizontal_list::draw( WINDOW *win )
{
    unsigned int x_offset = 1 + get_ax();

    for( unsigned int i = 0; i < text.size(); i++ ) {
        x_offset += draw_subtab( win, get_ax() + x_offset, get_ay(), text[i], i == scroll ) + 2;
    }
}

void ui_horizontal_list::set_text( std::vector<std::string> text )
{
    this->text = text;
    size_t text_length = 1;

    for( const auto &str : text ) {
        text_length += utf8_width( str ) + 1;
    }

    set_rect( ui_rect( text_length, get_rect().size_y, get_rect().x, get_rect().y ) );
}

void ui_horizontal_list::scroll_left()
{
    if( text.size() > 1) {
        scroll = ( scroll == 0 ? text.size() - 1 : scroll - 1 );
        on_scroll();
    }
}

void ui_horizontal_list::scroll_right()
{
    if( text.size() > 1 ) {
        scroll = ( scroll == text.size() - 1 ? 0 : scroll + 1 );
        on_scroll();
    }
}

const std::string &ui_horizontal_list::current() const
{
    return text[scroll];
}

void ui_horizontal_list::send_action( const std::string &action )
{
    if( action == "LEFT" ) {
        scroll_left();
    } else if( action == "RIGHT" ) {
        scroll_right();
    }
}

ui_border::ui_border( size_t size_x, size_t size_y, int x, int y, ui_anchor anchor ) : ui_element( size_x, size_y, x, y, anchor ), borders( array_2d<long>( size_x, size_y ) )
{
    calc_borders();
}

void ui_border::set_rect( const ui_rect &rect )
{
    ui_element::set_rect( rect );
    borders = array_2d<long>(rect.size_x, rect.size_y);
    calc_borders();
}

void ui_border::calc_borders()
{
    for( unsigned int x = 0; x < get_rect().size_x; x++ ){
        for( unsigned int y = 0; y < get_rect().size_y; y++ ) {
            long border = get_border_char(y != 0, y != get_rect().size_y - 1, x != 0, x != get_rect().size_x - 1);
            borders.set_at(x, y, border);
        }
    }
}

void ui_border::draw( WINDOW *win )
{
    const size_t size_x = get_rect().size_x;
    const size_t size_y = get_rect().size_y;

    for( unsigned int x = 0; x < size_x; x++ ){
        for( unsigned int y = 0; y < size_y; y++ ) {
            mvwputch( win, get_ay() + y, get_ax() + x, border_color, borders.get_at(x, y) );
        }
    }
}
