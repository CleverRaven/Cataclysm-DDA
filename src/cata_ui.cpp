#include "cata_ui.h"

#include "catacharset.h"
#include "output.h"
#include "input.h"

#include "debug.h"

#include <cmath>
#include <array>

ui_rect::ui_rect( size_t size_x, size_t size_y, int x, int y ) : size_x( size_x ), size_y( size_y ), x( x ), y( y )
{
}

ui_window::ui_window( const ui_rect &rect ) : rect( rect ), win( newwin( rect.size_y, rect.size_x, rect.y, rect.x ) )
{
    ctxt.register_directions();
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("NEXT_TAB");

    ctxt.register_action("QUIT");
    ctxt.register_action("CONFIRM");
}

ui_window::ui_window( size_t size_x, size_t size_y, int x, int y ) : ui_window( ui_rect( size_x, size_y, x, y ) )
{
}

ui_window::~ui_window()
{
    for( auto c : children ){
        delete c;
    }

    delwin( win );
}

void ui_window::draw()
{
    werase( win );
    local_draw();
    draw_children();
    wrefresh( win );
}

void ui_window::draw_children()
{
    for( auto &child : children ) {
        if( child->is_visible() ) {
            child->draw();
        }
    }
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

const ui_rect &ui_window::get_rect() const
{
    return rect;
}

void ui_window::set_rect(const ui_rect &new_rect)
{
    rect = new_rect;

    for( auto child : children ) {
        child->calc_anchored_values();
    }

    delwin( win );
    win = newwin( rect.size_y, rect.size_x, rect.y, rect.x );
}

void ui_window::add_child( ui_element *child )
{
    children.push_back( child );
    child->set_parent( this );
}

WINDOW *ui_window::get_win() const
{
    return win;
}

const std::vector<ui_element *> &ui_window::get_children() const
{
    return children;
}

ui_element::ui_element( size_t size_x, size_t size_y, int x, int y, ui_anchor anchor ) :
                       anchor( anchor ), anchored_x( x ), anchored_y( y ), rect( ui_rect( size_x, size_y, x, y ) )
{
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

void ui_label::draw()
{
    auto win = get_win();
    if( win == nullptr ) {
        return;
    }

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

void bordered_window::local_draw()
{
    ui_window::local_draw();
    auto win = get_win(); // never null

    wattron( win, border_color );
    wborder( win, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    wattroff( win, border_color );
}

health_bar::health_bar( size_t size_x, int x, int y, ui_anchor anchor ) : ui_element( size_x , 1, x, y, anchor ),
                       max_health( size_x * points_per_char ), current_health( max_health ), bar_str( std::string( size_x, '|' ) )
{
}

void health_bar::draw()
{
    auto win = get_win();
    if( win == nullptr ) {
        return;
    }

    mvwprintz( win, get_ay(), get_ax(), bar_color, bar_str.c_str() );
}

void health_bar::refresh_bar( bool overloaded, float percentage )
{
    bar_str = "";
    if( overloaded ) {
        bar_str = std::string( get_rect().size_x, '*' );
    } else {
        for( unsigned int i = 0; i < get_rect().size_x; i++ ) {
            int char_health = current_health - ( i * points_per_char );
            if( char_health <= 0 ) {
                bar_str += ".";
            } else if( char_health == 1 ) {
                bar_str += "\\";
            } else {
                bar_str += "|";
            }
        }
    }

    static const std::array<nc_color, 7> colors {{
        c_ltgray,
        c_red,
        c_ltred,
        c_magenta,
        c_yellow,
        c_ltgreen,
        c_green
    }};

    bar_color = colors[(int) roundf( (colors.size() - 1) * percentage )];
}

void health_bar::set_health_percentage( float percentage )
{
    bool overloaded = false;
    if( percentage > 1 ) {
        percentage = 1;
        current_health = max_health;
        overloaded = true;
    } else if( percentage < 0 ) {
        current_health = 0;
        percentage = 0;
    } else {
        current_health = percentage * max_health;
    }

    refresh_bar( overloaded, percentage ); // clamping percentage by 0 and 1;
}

smiley_indicator::smiley_indicator( int x, int y, ui_anchor anchor) : ui_element(2, 1, x, y, anchor )
{
    set_state( neutral );
}

void smiley_indicator::draw()
{
    auto win = get_win();
    if( win == nullptr ) {
        return;
    }

    mvwprintz( win, get_ay(), get_ax(), smiley_color, smiley_str.c_str() );
}

void smiley_indicator::set_state( smiley_state new_state )
{
    state = new_state;
    switch( new_state ) {
        case very_unhappy:
            smiley_color = c_red;
            smiley_str = "D:";
            break;
        case unhappy:
            smiley_color = c_ltred;
            smiley_str = ":(";
            break;
        case neutral:
            smiley_color = c_white;
            smiley_str = ":|";
            break;
        case happy:
            smiley_color = c_ltgreen;
            smiley_str = ":)";
            break;
        case very_happy:
            smiley_color = c_green;
            smiley_str = ":D";
            break;
    }
}

template<class T>
ui_tile_panel<T>::ui_tile_panel( size_t size_x, size_t size_y, int x, int y, ui_anchor anchor )
                       : ui_element( size_x, size_y, x, y, anchor ), tiles( array_2d<T>( size_x, size_y ) )
{
}

template<class T>
void ui_tile_panel<T>::set_rect( const ui_rect &new_rect )
{
    ui_element::set_rect( new_rect );
    tiles = array_2d<T>( new_rect.size_x, new_rect.size_y );
}

template<class T>
void ui_tile_panel<T>::draw()
{
    auto win = get_win();
    if( win == nullptr ) {
        return;
    }

    for( unsigned int x = 0; x < get_rect().size_x; x++ ) {
        for( unsigned int y = 0; y < get_rect().size_y; y++ ) {
            tiles.get_at(x, y).draw( win, x + get_ax(), y + get_ay() );
        }
    }
}

template<class T>
void ui_tile_panel<T>::set_tile( const T &tile, unsigned int x, unsigned int y )
{
    if( x >= get_rect().size_x || y >= get_rect().size_y ) {
        return;
    }

    tiles.set_at( x, y, tile );
}

void char_tile::draw( WINDOW *win, int x, int y ) const
{
    mvwputch( win, y, x, color, sym );
}

char_tile::char_tile( long sym, nc_color color ) : sym( sym ), color( color )
{
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

void tabbed_window::local_draw()
{
    bordered_window::local_draw();
    auto win = get_win(); // never null
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

ui_group *tabbed_window::add_tab( const std::string &tab )
{
    auto group = new ui_group();
    tabs.push_back({tab, group});
    group->set_visible( tabs.size() == 1 );
    return group;
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

ui_vertical_list::ui_vertical_list( size_t size_x, size_t size_y, int x, int y, ui_anchor anchor ) : ui_element( size_x, size_y, x, y, anchor )
{
}

void ui_vertical_list::draw()
{
    auto win = get_win();
    if( win == nullptr ) {
        return;
    }

    int start_line = 0;
    calcStartPos( start_line, scroll, get_rect().size_y, text.size() );
    unsigned int end_line = start_line + get_rect().size_y;

    unsigned int available_space = get_rect().size_x - 2; // 2 for scroll bar and spacer

    for( unsigned int line = start_line; line < end_line && line < text.size(); line++ ) {
        auto txt = text[line];
        if( txt.size() > available_space ) {
            txt = txt.substr( 0, available_space );
        }
        nc_color col = text_color;
        col = std::find(to_highlight.begin(), to_highlight.end(), line) != to_highlight.end() ? highlight_color : col;
        col = scroll == line ? hilite(col) : col;

        mvwprintz( win, get_ay() + line - start_line, get_ax() + 2, col, "%s", txt.c_str() );
    }

    draw_scrollbar( win, scroll, get_rect().size_y, text.size(), get_ay(), get_ax(), bar_color, false );
}

void ui_vertical_list::set_text( std::vector<std::string> text )
{
    this->text = text;
}

void ui_vertical_list::add_highlight( unsigned int i )
{
    to_highlight.push_back( i );
}

void ui_vertical_list::add_highlight( const std::string &line )
{
    add_highlight( std::distance( text.begin(), std::find( text.begin(), text.end(), line ) ) );
}

void ui_vertical_list::scroll_up()
{
    if( text.size() > 1 ) {
        scroll = ( scroll == 0 ? text.size() - 1 : scroll - 1 );

        if( scroll == text.size() - 1 ) {
            window_scroll = scroll - get_rect().size_y + 1;
        } else if( scroll < window_scroll ) {
            window_scroll--;
        }
    }
}

void ui_vertical_list::scroll_down()
{
    if( text.size() > 1 ) {
        scroll = ( scroll == text.size() - 1 ? 0 : scroll + 1 );

        if( scroll > get_rect().size_y + window_scroll - 1 ) {
            window_scroll++;
        } else if( scroll == 0 ) {
            window_scroll = 0;
        }
    }
}

std::string ui_vertical_list::current() const
{
    if( text.empty() ) {
        return "";
    }
    return text[scroll];
}

void ui_vertical_list::send_action( const std::string &action )
{
    if( action == "UP" ) {
        scroll_up();
    } else if( action == "DOWN" ) {
        scroll_down();
    }
}

ui_horizontal_list::ui_horizontal_list( int x, int y, ui_anchor anchor ) : ui_element( 0, 1, x, y, anchor )
{
}

void ui_horizontal_list::draw()
{
    auto win = get_win();
    if( win == nullptr ) {
        return;
    }

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
    scroll = ( scroll == 0 ? text.size() - 1 : scroll - 1 );
}

void ui_horizontal_list::scroll_right()
{
    scroll = ( scroll == text.size() - 1 ? 0 : scroll + 1 );
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

color_mapped_label::color_mapped_label( std::string text, int x, int y, ui_anchor anchor ) : ui_label( text, x, y, anchor )
{
}

void color_mapped_label::draw()
{
    auto win = get_win();
    if( win == nullptr ) {
        return;
    }

    mvwprintz( win, get_ay(), get_ax(), text_color, "%s", text.c_str() );

    for( auto kvp : color_map ) {
        for( unsigned int x = 0; x < text.size() && x < kvp.second.size(); x++ ) {
            if( kvp.second[x] != '_' ) {
                mvwputch( win, get_ay(), get_ax() + x, kvp.first, text[x] );
            }
        }
    }
}

std::string &color_mapped_label::operator[]( nc_color color )
{
    return color_map[color];
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

long get_border_char( bool up, bool down, bool left, bool right )
{
    long ret = ' ';

    if( left ) {
        ret = LINE_OXOX; // '-'
    }

    if( up ) {
        if( left ) {
            ret = LINE_XOOX; // '_|'
        } else {
            ret = LINE_XOXO; // '|'
        }
    }

    if( right ) {
        if( up ) {
            if( left ) {
                ret = LINE_XXOX; // '_|_'
            } else {
                ret = LINE_XXOO; // '|_'
            }
        } else {
            ret = LINE_OXOX; // '-'
        }
    }

    if( down ) {
        if( right ) {
            if( up ) {
                if( left ) {
                    ret = LINE_XXXX; // '-|-'
                } else {
                    ret = LINE_XXXO; // '|-'
                }
            } else {
                if( left ) {
                    ret = LINE_OXXX; // '^|^'
                } else {
                    ret = LINE_OXXO; // '|^'
                }
            }
        } else {
            if( left ) {
                if( up ) {
                    ret = LINE_XOXX; // '-|'
                } else {
                    ret = LINE_OOXX; // '^|'
                }
            } else {
                ret = LINE_XOXO; // '|'
            }
        }
    }

    return ret;
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

void ui_border::draw()
{
    auto win = get_win();
    if( win == nullptr ) {
        return;
    }

    const size_t size_x = get_rect().size_x;
    const size_t size_y = get_rect().size_y;

    for( unsigned int x = 0; x < size_x; x++ ){
        for( unsigned int y = 0; y < size_y; y++ ) {
            mvwputch( win, get_ay() + y, get_ax() + x, border_color, borders.get_at(x, y) );
        }
    }
}
