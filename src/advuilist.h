#ifndef CATA_SRC_ADVUILIST_H
#define CATA_SRC_ADVUILIST_H

#include <algorithm>   // for max, find_if, stable_sort
#include <cmath>       // for ceil
#include <cstddef>     // for size_t
#include <functional>  // for function
#include <iterator>    // for distance
#include <memory>      // for shared_ptr, unique_ptr, __shared...
#include <string>      // for string, operator==, basic_string
#include <tuple>       // for tuple
#include <type_traits> // for declval
#include <utility>     // for pair
#include <vector>      // for vector

#include "color.h"                  // for color_manager, hilite, c_dark_gray
#include "cursesdef.h"              // for mvwprintw, newwin, werase, window
#include "input.h"                  // for input_context
#include "output.h"                 // for trim_and_print, draw_border, rig...
#include "point.h"                  // for point
#include "src/cata_utility.h"       // for lcmatch
#include "src/string_formatter.h"   // for string_format
#include "src/string_input_popup.h" // for string_input_popup
#include "translations.h"           // for _, localized_compare
#include "ui.h"                     // for uilist
#include "ui_manager.h"             // for redraw, ui_adaptor

// TODO:
//     grouping
//     select all
//
//     advuilist_sourced wrapper
//     rewrite AIM to use advuilist_sourced

template <class Container, typename T = typename Container::value_type>
class advuilist
{
  public:
    /// column printer function
    using fcol_t = std::function<std::string( T const & )>;
    using cwidth_t = float;
    /// name, column printer function, width weight
    using col_t = std::tuple<std::string, fcol_t, cwidth_t>;
    /// sorting function
    using fsort_t = std::function<bool( T const &, T const & )>;
    /// name, sorting function
    using sorter_t = std::pair<std::string, fsort_t>;
    /// filter function. params are element, filter string
    using ffilter_t = std::function<bool( T const &, std::string const & )>;
    /// ctxt handler function for adding extra functionality
    using fctxt_t = std::function<void( std::string const & )>;

    // using fgid_t = std::function<std::size_t( T const & )>;
    // using fglabel_t = std::function<std::string( std::size_t )>;
    // using grouper_t = std::tuple<std::string, fgid_t, fglabel_t>;

    using ptr_t = typename Container::iterator;
    using select_t = std::vector<ptr_t>;

    advuilist( Container *list, point size = { -1, -1 }, point origin = { -1, -1 },
               std::string const &ctxtname = "default" );

    /// sets up columns and replaces implicit column sorters (does not affect additional sorters
    /// added with addSorter())
    ///@param columns
    void setColumns( std::vector<col_t> const &columns );
    /// adds a new sorter. replaces existing sorter with same name (including implicit column
    /// sorters)
    ///@param sorter
    void addSorter( sorter_t const &sorter );
    /// sets a handler for input_context actions. this is executed before any internal actions are
    /// handled
    void setctxthandler( fctxt_t const &func );
    /// sets the filtering function
    void setfilterf( ffilter_t const &func );
    select_t select();
    void sort( std::string const &name );
    void rebuild( Container *list );
    /// returns the currently hilighted element. meant to be called from the external ctxt handler
    /// added by setctxthandler()
    select_t peek();
    /// breaks internal loop in select(). meant to be called from the external ctxt handler added by
    /// setctxthandler()
    void suspend();

    // addGroup

    input_context *get_ctxt();
    catacurses::window *get_window();
    std::shared_ptr<ui_adaptor> get_ui();

  private:
    /// pair of index, pointer. index is used for "none" sorting mode and is not meant to represent
    /// index in the original Container (which may not even be indexable)
    using entry_t = std::pair<std::size_t, ptr_t>;
    using list_t = std::vector<entry_t>;
    using colscont_t = std::vector<col_t>;
    using sortcont_t = std::vector<sorter_t>;

    point _size;
    point _origin;
    std::size_t _pagesize = 0;
    list_t _list;
    colscont_t _columns;
    sortcont_t _sorters;
    ffilter_t _ffilter;
    fctxt_t _fctxt;
    std::string _filter;
    typename sortcont_t::size_type _csort = 0;
    typename list_t::size_type _cidx = 0;
    /// total column width weights
    cwidth_t _tweight = 0;
    bool _exit = false;

    // groups

    input_context _ctxt;
    catacurses::window _w;
    std::shared_ptr<ui_adaptor> _ui;

    Container *_olist = nullptr;

    /// number of lines at the top of the window reserved for decorations
    static constexpr int const _headersize = 2;
    /// number of lines at the bottom of the window reserved for decorations
    static constexpr int const _footersize = 1;
    /// first usable column after decorations
    static constexpr int const _firstcol = 1;
    /// minimum whitespace between columns
    static constexpr int const _colspacing = 1;

    void _initui();
    void _initctxt();
    void _print();
    int _printcol( col_t const &col, std::string const &str, point const &p,
                   nc_color const &color );
    void _printheaders();
    void _printfooters();
    void _sort( typename sortcont_t::size_type idx );
    void _querysort();
    void _queryfilter();
    void _setfilter( std::string const &filter );
    bool _basicfilter( T const &it, std::string const &filter ) const;
};

// *INDENT-OFF*
template <class Container, typename T>
advuilist<Container, T>::advuilist( Container *list, point size, point origin,
                                    std::string const &ctxtname )
    : _size( size.x > 0 ? size.x : TERMX / 4, size.y > 0 ? size.y : TERMY / 4 ),
      _origin( origin.x >= 0 ? origin.x : TERMX / 2 - _size.x / 2,
               origin.y >= 0 ? origin.y : TERMY / 2 - _size.y / 2 ),
      // leave space for decorations and column headers
      _pagesize( std::min( list->size(), static_cast<std::size_t>( std::max(
                                             0, _size.y - ( _headersize + _footersize + 1 ) ) ) ) ),
      // insert dummy sorter for "none" sort mode
      _sorters{ { "none", []( T const & /**/, T const & /**/ ) { return false; } } },
      // ugly hack that lets us use our implicit basic filter if one isn't supplied
      _ffilter{ [this]( T const &it, std::string const &filter ) {
          return this->_basicfilter( it, filter );
      } },
      _ctxt( ctxtname ), _ui( std::make_shared<ui_adaptor>() ),
      // remember constructor list so we can rebuild internal list when filtering
      _olist( list )
// *INDENT-ON*
{
    rebuild( list );
    _initui();
    _initctxt();
}

template <class Container, typename T>
void advuilist<Container, T>::setColumns( std::vector<col_t> const &columns )
{
    typename colscont_t::size_type const ncols_old = _columns.size();
    _columns.clear();
    _tweight = 0;

    sortcont_t tmp;
    for( auto const &v : columns ) {
        _columns.emplace_back( v );
        _tweight += std::get<2>( v );
        // build new implicit column sorters
        std::size_t const idx = _columns.size() - 1;
        tmp.emplace_back( std::get<0>( v ), [this, idx]( T const &lhs, T const &rhs ) -> bool {
            return localized_compare( std::get<1>( _columns[idx] )( lhs ),
                                      std::get<1>( _columns[idx] )( rhs ) );
        } );
    }

    // replace old implicit column sorters (keep "none" and additional sorters added with addSorter)
    _sorters.erase( _sorters.begin() + 1,
                    _sorters.begin() + 1 +
                        static_cast<typename sortcont_t::difference_type>( ncols_old ) );
    _sorters.insert( _sorters.begin() + 1, std::make_move_iterator( tmp.begin() ),
                     std::make_move_iterator( tmp.end() ) );
}

template <class Container, typename T>
void advuilist<Container, T>::addSorter( sorter_t const &sorter )
{
    auto it = std::find_if( _sorters.begin(), _sorters.end(),
                            [&]( auto const &v ) { return v.first == sorter.first; } );
    // replace sorter with same name if it already exists (including implicit sorters)
    if( it != _sorters.end() ) {
        *it = sorter;
    } else {
        _sorters.emplace_back( sorter );
    }
}

template <class Container, typename T>
void advuilist<Container, T>::setctxthandler( fctxt_t const &func )
{
    _fctxt = func;
}

template <class Container, typename T>
void advuilist<Container, T>::setfilterf( ffilter_t const &func )
{
    _ffilter = func;
}

template <class Container, typename T>
typename advuilist<Container, T>::select_t advuilist<Container, T>::select()
{
    while( !_exit ) {

        ui_manager::redraw();
        std::string const action = _ctxt.handle_input();

        if( _fctxt ) {
            _fctxt( action );
        }

        if( action == "UP" ) {
            _cidx = _cidx == 0 ? _list.size() - 1 : _cidx - 1;
        } else if( action == "DOWN" ) {
            _cidx = _cidx >= _list.size() - 1 ? 0 : _cidx + 1;
        } else if( action == "PAGE_UP" ) {
            _cidx = _cidx > _pagesize ? _cidx - _pagesize : 0;
        } else if( action == "PAGE_DOWN" ) {
            _cidx = _cidx < _list.size() - _pagesize ? _cidx + _pagesize : _list.size() - 1;
        } else if( action == "SORT" ) {
            _querysort();
        } else if( action == "FILTER" ) {
            _queryfilter();
        } else if( action == "RESET_FILTER" ) {
            _setfilter( std::string() );
        } else if( action == "SELECT" ) {
            return { _list[_cidx].second };
        } else if( action == "QUIT" ) {
            _exit = true;
        }
        // FIXME: add SELECT_ALL and maybe SELECT_MARKED (or expand SELECT)
    }

    return select_t();
}

template <class Container, typename T>
void advuilist<Container, T>::sort( std::string const &name )
{
    auto const it = std::find_if( _sorters.begin(), _sorters.end(),
                                  [&]( auto const &v ) { return v.first == name; } );
    if( it != _sorters.end() ) {
        _sort(
            static_cast<typename sortcont_t::size_type>( std::distance( _sorters.begin(), it ) ) );
    }
}

template <class Container, typename T>
void advuilist<Container, T>::rebuild( Container *list )
{
    _list.clear();
    std::size_t idx = 0;
    for( auto it = list->begin(); it != list->end(); ++it ) {
        if( !_ffilter or _filter.empty() or _ffilter( *it, _filter ) ) {
            _list.emplace_back( idx++, it );
        }
    }
    _cidx = _cidx > _list.size() - 1 ? _list.size() - 1 : _cidx;
    // FIXME?: use a set for _list
    _sort( _csort );
}

template <class Container, typename T>
typename advuilist<Container, T>::select_t advuilist<Container, T>::peek()
{
    return { _list[_cidx].second };
}

template <class Container, typename T>
void advuilist<Container, T>::suspend()
{
    _exit = true;
}

template <class Container, typename T>
input_context *advuilist<Container, T>::get_ctxt()
{
    return &_ctxt;
}

template <class Container, typename T>
catacurses::window *advuilist<Container, T>::get_window()
{
    return &_w;
}

template <class Container, typename T>
std::shared_ptr<ui_adaptor> advuilist<Container, T>::get_ui()
{
    return _ui;
}

template <class Container, typename T>
void advuilist<Container, T>::_initui()
{
    _ui->on_screen_resize( [&]( ui_adaptor &ui ) {
        _w = catacurses::newwin( _size.y, _size.x, _origin );
        ui.position_from_window( _w );
    } );
    _ui->mark_resize();

    _ui->on_redraw( [&]( const ui_adaptor & ) {
        werase( _w );
        draw_border( _w );
        _print();
        wnoutrefresh( _w );
    } );
}

template <class Container, typename T>
void advuilist<Container, T>::_initctxt()
{
    _ctxt.register_updown();
    _ctxt.register_action( "PAGE_UP" );
    _ctxt.register_action( "PAGE_DOWN" );
    _ctxt.register_action( "SORT" );
    _ctxt.register_action( "FILTER" );
    _ctxt.register_action( "RESET_FILTER" );
    _ctxt.register_action( "SELECT" );
    _ctxt.register_action( "QUIT" );
    _ctxt.register_action( "HELP_KEYBINDINGS" );
}

template <class Container, typename T>
void advuilist<Container, T>::_print()
{
    _printheaders();

    point lpos( _firstcol, _headersize );

    // print column headers
    for( auto const &col : _columns ) {
        lpos.x += _printcol( col, std::get<0>( col ), lpos, c_white );
    }
    lpos.y += 1;

    // print entries
    std::size_t const cpagebegin = ( _cidx / _pagesize ) * _pagesize;
    std::size_t const cpageend = std::min( cpagebegin + _pagesize, _list.size() );
    for( std::size_t idx = cpagebegin; idx < cpageend; idx++ ) {
        T const &it = *_list[idx].second;
        lpos.x = _firstcol;
        for( auto const &col : _columns ) {
            lpos.x += _printcol( col, std::get<1>( col )( it ), lpos,
                                 idx == _cidx ? hilite( c_dark_gray ) : c_dark_gray );
        }
        lpos.y += 1;
    }

    _printfooters();
}

template <class Container, typename T>
int advuilist<Container, T>::_printcol( col_t const &col, std::string const &str, point const &p,
                                        nc_color const &color )
{
    constexpr int const borderwidth = _firstcol * 2;
    int const colwidth = std::get<2>( col ) * ( _size.x - borderwidth ) / _tweight;
    trim_and_print( _w, p, colwidth - _colspacing, color, str );
    return colwidth;
}

template <class Container, typename T>
void advuilist<Container, T>::_printheaders()
{
    // sort mode
    mvwprintw( _w, { _firstcol, 0 }, _( "< [%s] Sort: %s >" ), _ctxt.get_desc( "SORT" ),
               _sorters[_csort].first );

    // page index
    std::size_t const cpage = _cidx / _pagesize + 1;
    std::size_t const npages = std::ceil( _list.size() / static_cast<float>( _pagesize ) );
    std::string const msg2 = string_format( _( "[<] page %1$d of %2$d [>]" ), cpage, npages );
    trim_and_print( _w, { _firstcol, 1 }, _size.x, c_light_blue, msg2 );

    // keybinding hint
    std::string const msg3 = string_format( _( "< [<color_yellow>%s</color>] keybindings >" ),
                                            _ctxt.get_desc( "HELP_KEYBINDINGS" ) );
    right_print( _w, 0, 0, c_white, msg3 );
}

template <class Container, typename T>
void advuilist<Container, T>::_printfooters()
{
    // filter
    std::string fprefix = string_format( _( "[%s] Filter" ), _ctxt.get_desc( "FILTER" ) );
    if( !_filter.empty() ) {
        mvwprintw( _w, { _firstcol, _size.y - 1 }, "< %s: %s >", fprefix, _filter );
    } else {
        mvwprintw( _w, { _firstcol, _size.y - 1 }, "< %s >", fprefix );
    }
}

template <class Container, typename T>
void advuilist<Container, T>::_sort( typename sortcont_t::size_type idx )
{
    if( idx > 0 ) {
        struct cmp {
            fsort_t const &sorter;
            cmp( sorter_t const &_s ) : sorter( _s.second ) {}

            constexpr bool operator()( entry_t const &lhs, entry_t const &rhs ) const
            {
                return sorter( *lhs.second, *rhs.second );
            }
        };
        std::stable_sort( _list.begin(), _list.end(), cmp( _sorters[idx] ) );
    } else {
        // "none" sort mode needs special handling unfortunately
        struct cmp {
            constexpr bool operator()( entry_t const &lhs, entry_t const &rhs ) const
            {
                return lhs.first < rhs.first;
            }
        };
        std::stable_sort( _list.begin(), _list.end(), cmp() );
    }
    _csort = idx;
}
template <class Container, typename T>
void advuilist<Container, T>::_querysort()
{
    uilist menu;
    menu.text = _( "Sort by…" );
    for( int i = 0; i < static_cast<int>( _sorters.size() ); i++ ) {
        menu.addentry( i, true, _sorters[i].first.front(), _sorters[i].first );
    }
    menu.query();
    if( menu.ret >= 0 ) {
        _sort( menu.ret );
    }
}

template <class Container, typename T>
void advuilist<Container, T>::_queryfilter()
{
    auto spopup = std::make_unique<string_input_popup>();
    spopup->max_length( 256 ).text( _filter );

    do {
        ui_manager::redraw();
        std::string const nfilter = spopup->query_string( false );
        if( !spopup->canceled() && nfilter != _filter ) {
            _setfilter( nfilter );
        }
    } while( !spopup->canceled() && !spopup->confirmed() );
}

template <class Container, typename T>
void advuilist<Container, T>::_setfilter( std::string const &filter )
{
    _filter = filter;
    rebuild( _olist );
}

template <class Container, typename T>
bool advuilist<Container, T>::_basicfilter( T const &it, std::string const &filter ) const
{
    for( auto const &col : _columns ) {
        if( lcmatch( std::get<1>( col )( it ), filter ) ) {
            return true;
        }
    }

    return false;
}

#endif // CATA_SRC_ADVUILIST_H
