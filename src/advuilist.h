#ifndef CATA_SRC_ADVUILIST_H
#define CATA_SRC_ADVUILIST_H

#include <algorithm>   // for max, find_if, stable_sort
#include <cmath>       // for ceil
#include <cstddef>     // for size_t
#include <cstdint>     // for uint64_t
#include <functional>  // for function
#include <iterator>    // for distance
#include <memory>      // for shared_ptr, unique_ptr, __shared...
#include <string>      // for string, operator==, basic_string
#include <tuple>       // for tuple
#include <type_traits> // for declval
#include <utility>     // for pair
#include <vector>      // for vector

#include "advuilist_const.h"    // for ACTION_
#include "cata_utility.h"       // for lcmatch
#include "color.h"              // for color_manager, hilite, c_dark_gray
#include "cursesdef.h"          // for mvwprintw, newwin, werase, window
#include "input.h"              // for input_context
#include "output.h"             // for trim_and_print, draw_border, rig...
#include "point.h"              // for point
#include "string_formatter.h"   // for string_format
#include "string_input_popup.h" // for string_input_popup
#include "translations.h"       // for _, localized_compare
#include "ui.h"                 // for uilist
#include "ui_manager.h"         // for redraw, ui_adaptor
#include "uistate.h"            // for advuilist_save_state

// TODO:
//     mark element & expand SELECT

template <class Container, typename T = typename Container::value_type>
class advuilist
{
    public:
        /// column printer function. entry, colwidth. colwidth = 0 means unlimited
        using fcol_t = std::function<std::string( T const &, int )>;
        using cwidth_t = float;
        /// name, column printer function, width weight
        using col_t = std::tuple<std::string, fcol_t, cwidth_t>;
        using count_t = std::size_t;
        /// counting function. used only for partial/whole selection
        using fcounter_t = std::function<count_t( T const & )>;
        /// on_rebuild function. params are first_item, element
        using frebuild_t = std::function<void( bool, T const & )>;
        using fdraw_t = std::function<void( advuilist<Container, T> * )>;
        /// sorting function
        using fsort_t = std::function<bool( T const &, T const & )>;
        /// name, sorting function
        using sorter_t = std::pair<std::string, fsort_t>;
        /// filter function. params are element, filter string
        using ffilter_t = std::function<bool( T const &, std::string const & )>;
        /// filter description, filter function
        using filter_t = std::pair<std::string, ffilter_t>;
        /// ctxt handler function for adding extra functionality
        using fctxt_t = std::function<void( advuilist<Container, T> *, std::string const & )>;
        /// group sorter function
        using fgsort_t = std::function<bool( T const &, T const & )>;
        /// group label printer
        using fglabel_t = std::function<std::string( T const & )>;
        /// group name, ID function, label printer
        using grouper_t = std::tuple<std::string, fgsort_t, fglabel_t>;

        using ptr_t = typename Container::iterator;
        /// count, pointer. count is always > 0
        using selection_t = std::pair<count_t, ptr_t>;
        using select_t = std::vector<selection_t>;

        explicit advuilist( Container *list, point size = { -9, -9 }, point origin = { -9, -9 },
                            std::string const &ctxtname = advuilist_literals::CTXT_DEFAULT,
                            point reserved_rows = { 2, 1 } );

        /// sets up columns and optionally sets up implict sorters
        ///@param columns
        ///@param implicit if true, implicit sorters will be created for each column. will replace sorters
        ///                with same name (see addSorter )
        void setColumns( std::vector<col_t> const &columns, bool implicit = true );
        /// adds a new sorter. replaces existing sorter with same name (including implicit column
        /// sorters)
        ///@param sorter
        void addSorter( sorter_t const &sorter );
        /// adds a new grouper. replaces existing grouper with same name
        ///@param grouper
        void addGrouper( grouper_t const &grouper );
        /// sets a handler for input_context actions. this is executed after all internal actions are
        /// handled
        void setctxthandler( fctxt_t const &func );
        /// sets the element counting function. enables partial and whole selection
        void setcountingf( fcounter_t const &func );
        /// if set, func gets called for every element that gets added to the internal list. This is
        /// meant to be used for collecting stats
        void on_rebuild( frebuild_t const &func );
        /// if set, func gets called after all internal printing calls and before wnoutrefresh(). This
        /// is meant to be used for drawing extra decorations or stats
        void on_redraw( fdraw_t const &func );
        /// if set, func will get called on screen resize instead of running resize() directly. Use
        /// this if window size/pos depends on external variables such as TERMX/Y
        void on_resize( fdraw_t const &func );
        /// sets the filtering function
        void setfilterf( filter_t const &func );
        select_t select();
        void sort( std::string const &name );
        void rebuild( Container *list = nullptr );
        /// returns the currently hilighted element. meant to be called from the external ctxt handler
        /// added by setctxthandler()
        select_t peek();
        /// breaks internal loop in select(). meant to be called from the external ctxt handler added by
        /// setctxthandler()
        void suspend();
        /// pre-initialize or reset the internal ui_adaptor;
        void initui();
        void hide();
        void resize( point size, point origin, point reserved_rows = {-9, -9} );

        input_context *get_ctxt();
        catacurses::window *get_window();
        std::shared_ptr<ui_adaptor> get_ui();
        std::pair<point, point> get_size();

        void savestate( advuilist_save_state *state );
        void loadstate( advuilist_save_state *state, bool reb = true );

    private:
        /// pair of index, pointer. index is used for "none" sorting mode and is not meant to represent
        /// index in the original Container (which may not even be indexable)
        using nidx_t = typename Container::size_type;
        using entry_t = std::pair<nidx_t, ptr_t>;
        using list_t = std::vector<entry_t>;
        using colscont_t = std::vector<col_t>;
        using sortcont_t = std::vector<sorter_t>;
        /// groups are pairs of begin and end iterators of the internal list
        /// NOTE: this only works well with RandomAccessIterator
        using group_t = std::pair<typename list_t::iterator, typename list_t::iterator>;
        /// pages are pairs of direct indices of the internal list
        using page_t = std::pair<typename list_t::size_type, typename list_t::size_type>;
        using groupcont_t = std::vector<group_t>;
        using groupercont_t = std::vector<grouper_t>;
        using pagecont_t = std::vector<page_t>;

        point _size, _osize;
        point _origin, _oorigin;
        typename list_t::size_type _pagesize = 0;
        list_t _list;
        colscont_t _columns;
        sortcont_t _sorters;
        groupercont_t _groupers;
        groupcont_t _groups;
        pagecont_t _pages;
        ffilter_t _ffilter;
        fcounter_t _fcounter;
        frebuild_t _frebuild;
        fdraw_t _fdraw, _fresize;
        fctxt_t _fctxt;
        std::string _filter;
        std::string _filterdsc;
        typename sortcont_t::size_type _csort = 0;
        typename groupercont_t::size_type _cgroup = 0;
        typename list_t::size_type _cidx = 0;
        typename pagecont_t::size_type _cpage = 0;
        /// total column width weights
        cwidth_t _tweight = 0;
        int _innerw = 0;
        /// number of lines at the top of the window reserved for decorations
        int _headersize = 2;
        /// number of lines at the bottom of the window reserved for decorations
        int _footersize = 1;
        bool _exit = true;
        bool _needsinit = true;

        input_context _ctxt;
        catacurses::window _w;
        std::shared_ptr<ui_adaptor> _ui;

        Container *_olist = nullptr;

        /// first usable column after decorations
        static constexpr int const _firstcol = 1;
        /// minimum whitespace between columns
        static constexpr int const _colspacing = 1;

        select_t _peek( count_t amount );
        select_t _peekall();
        count_t _count( typename list_t::size_type idx );
        count_t _peekcount();

        void _initctxt();
        void _print();
        int _colwidth( col_t const &col, point const &p );
        int _printcol( col_t const &col, std::string const &str, point const &p, nc_color const &color );
        int _printcol( col_t const &col, T const &it, point const &p, nc_color const &color, bool hilited );
        void _printheaders();
        void _printfooters();
        void _sort( typename sortcont_t::size_type idx );
        void _group( typename groupercont_t::size_type idx );
        void _querysort();
        void _queryfilter();
        count_t _querypartial( count_t max );
        void _setfilter( std::string const &filter );
        bool _basicfilter( T const &it, std::string const &filter ) const;
        typename pagecont_t::size_type _idxtopage( typename list_t::size_type idx );
        void _incidx( typename list_t::size_type amount );
        void _decidx( typename list_t::size_type amount );
        void _setidx( typename list_t::size_type idx );
};

// *INDENT-OFF*

#ifdef __clang__
#pragma clang diagnostic push
// travis' old clang wants a change that breaks compilation with newer versions
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

template <class Container, typename T>
advuilist<Container, T>::advuilist( Container *list, point size, point origin,
                                    std::string const &ctxtname, point reserved_rows )
    : _osize( size ),
      _oorigin( origin ),
      // insert dummy sorter for "none" sort mode
      _sorters{ sorter_t{ "none", fsort_t() } },
      // insert dummy grouper for "none" grouping mode
      _groupers{ grouper_t{ "none", fgsort_t(), fglabel_t() } },
      // ugly hack that lets us use our implicit basic filter if one isn't supplied
      _ffilter{ [this]( T const &it, std::string const &filter ) {
          return this->_basicfilter( it, filter );
      } },
      _innerw( _size.x - _firstcol * 2 ),
      _headersize( reserved_rows.x ),
      _footersize( reserved_rows.y ),
      _ctxt( ctxtname ),
      // remember constructor list so we can rebuild internal list when filtering
      _olist( list )

#ifdef __clang__
#pragma clang diagnostic pop
#endif

// *INDENT-ON*

{
    _initctxt();
}

template <class Container, typename T>
void advuilist<Container, T>::setColumns( std::vector<col_t> const &columns, bool implicit )
{
    _columns.clear();
    _tweight = 0;

    sortcont_t tmp;
    for( col_t const &v : columns ) {
        _columns.emplace_back( v );
        _tweight += std::get<cwidth_t>( v );

        // build new implicit column sorters
        if( implicit ) {
            typename colscont_t::size_type const idx = _columns.size() - 1;
            tmp.emplace_back( std::get<std::string>( v ),
            [this, idx]( T const & lhs, T const & rhs ) -> bool {
                return localized_compare( std::get<fcol_t>( _columns[idx] )( lhs, 0 ),
                                          std::get<fcol_t>( _columns[idx] )( rhs, 0 ) );
            } );
        }
    }

    for( sorter_t const &s : tmp ) {
        addSorter( s );
    }
}

template <class Container, typename T>
void advuilist<Container, T>::addSorter( sorter_t const &sorter )
{
    auto it = std::find_if( _sorters.begin(), _sorters.end(), [&]( sorter_t const & v ) {
        return std::get<std::string>( v ) == std::get<std::string>( sorter );
    } );
    // replace sorter with same name if it already exists (including implicit sorters)
    if( it != _sorters.end() ) {
        *it = sorter;
    } else {
        _sorters.emplace_back( sorter );
    }
}

// FIXME: can I deduplicate this with addSorter?
template <class Container, typename T>
void advuilist<Container, T>::addGrouper( grouper_t const &grouper )
{
    auto it = std::find_if( _groupers.begin(), _groupers.end(), [&]( grouper_t const & v ) {
        return std::get<std::string>( v ) == std::get<std::string>( grouper );
    } );
    // replace grouper with same name
    if( it != _groupers.end() ) {
        *it = grouper;
    } else {
        _groupers.emplace_back( grouper );
    }
}

template <class Container, typename T>
void advuilist<Container, T>::setctxthandler( fctxt_t const &func )
{
    _fctxt = func;
}

template <class Container, typename T>
void advuilist<Container, T>::setcountingf( fcounter_t const &func )
{
    _fcounter = func;
}

template <class Container, typename T>
void advuilist<Container, T>::on_rebuild( frebuild_t const &func )
{
    _frebuild = func;
}

template <class Container, typename T>
void advuilist<Container, T>::on_redraw( fdraw_t const &func )
{
    _fdraw = func;
}

template <class Container, typename T>
void advuilist<Container, T>::on_resize( fdraw_t const &func )
{
    _fresize = func;
}

template <class Container, typename T>
void advuilist<Container, T>::setfilterf( filter_t const &func )
{
    _filterdsc = std::get<std::string>( func );
    _ffilter = std::get<ffilter_t>( func );
}

template <class Container, typename T>
typename advuilist<Container, T>::select_t advuilist<Container, T>::select()
{
    using namespace advuilist_literals;
    // reset exit variable in case suspend() was called earlier
    _exit = false;

    if( !_ui ) {
        initui();
    }
    if( _needsinit ) {
        rebuild();
    }

    while( !_exit ) {

        _ui->invalidate_ui();
        ui_manager::redraw_invalidated();
        std::string const action = _ctxt.handle_input();

        if( action == ACTION_UP ) {
            _decidx( 1 );
        } else if( action == ACTION_DOWN ) {
            _incidx( 1 );
        } else if( action == ACTION_PAGE_UP ) {
            _decidx( _pagesize );
        } else if( action == ACTION_PAGE_DOWN ) {
            _incidx( _pagesize );
        } else if( action == ACTION_SORT ) {
            _querysort();
        } else if( action == ACTION_FILTER ) {
            _queryfilter();
        } else if( action == ACTION_RESET_FILTER ) {
            _setfilter( std::string() );
        } else if( action == ACTION_SELECT ) {
            return peek();
        } else if( action == ACTION_SELECT_PARTIAL ) {
            if( !_list.empty() ) {
                count_t const count = _peekcount();
                count_t const input = _querypartial( count );
                if( input > 0 ) {
                    return _peek( input );
                }
            }
        } else if( action == ACTION_SELECT_WHOLE ) {
            count_t const count = _peekcount();
            return _peek( count );
        } else if( action == ACTION_SELECT_ALL ) {
            return _peekall();
        } else if( action == ACTION_QUIT ) {
            _exit = true;
        }

        if( _fctxt ) {
            _fctxt( this, action );
        }
    }

    return select_t();
}

template <class Container, typename T>
void advuilist<Container, T>::sort( std::string const &name )
{
    auto const it = std::find_if( _sorters.begin(), _sorters.end(), [&]( sorter_t const & v ) {
        return std::get<std::string>( v ) == name;
    } );
    if( it != _sorters.end() ) {
        _sort(
            static_cast<typename sortcont_t::size_type>( std::distance( _sorters.begin(), it ) ) );
    }
}

template <class Container, typename T>
void advuilist<Container, T>::rebuild( Container *list )
{
    _list.clear();
    Container *rlist = list == nullptr ? _olist : list;
    nidx_t idx = 0;
    for( auto it = rlist->begin(); it != rlist->end(); ++it ) {
        if( !_ffilter or _filter.empty() or _ffilter( *it, _filter ) ) {
            if( _frebuild ) {
                _frebuild( _list.empty(), *it );
            }
            _list.emplace_back( idx++, it );
        }
    }
    _group( _cgroup );
    _sort( _csort );
    _setidx( _cidx );
    _cpage = _idxtopage( _cidx );
    _needsinit = false;
}

template <class Container, typename T>
typename advuilist<Container, T>::select_t advuilist<Container, T>::peek()
{
    return _peek( 1 );
}

template <class Container, typename T>
void advuilist<Container, T>::suspend()
{
    _exit = true;
    // redraw darker borders, etc
    _ui->invalidate_ui();
}

template <class Container, typename T>
void advuilist<Container, T>::initui()
{
    _ui = std::make_shared<ui_adaptor>();
    _ui->on_screen_resize( [&]( ui_adaptor & ui ) {
        if( _fresize ) {
            _fresize( this );
        } else {
            resize( _osize, _oorigin );
        }
        _w = catacurses::newwin( _size.y, _size.x, _origin );
        ui.position_from_window( _w );
    } );
    _ui->mark_resize();

    _ui->on_redraw( [&]( const ui_adaptor & /*ui*/ ) {
        werase( _w );
        draw_border( _w, _exit ? c_dark_gray : c_light_gray );
        _print();
        if( _fdraw ) {
            _fdraw( this );
        }
        wnoutrefresh( _w );
    } );
}

template <class Container, typename T>
void advuilist<Container, T>::hide()
{
    _ui.reset();
}

template <class Container, typename T>
void advuilist<Container, T>::resize( point size, point origin, point reserved_rows )
{
    _size = { size.x > 0 ? size.x > TERMX ? TERMX : size.x : TERMX / 4,
              size.y > 0 ? size.y > TERMY ? TERMY : size.y : TERMY / 4
            };
    _origin = { origin.x >= 0 ? origin.x + size.x > TERMX ? 0 : origin.x : TERMX / 2 - _size.x / 2,
                origin.y >= 0 ? origin.y + size.y > TERMY ? 0 : origin.y
                : TERMY / 2 - _size.y / 2
              };
    _headersize = reserved_rows.x > 0 ? reserved_rows.x : _headersize;
    _footersize = reserved_rows.y > 0 ? reserved_rows.y : _footersize;

    _innerw = _size.x - _firstcol * 2;
    // leave space for decorations and column headers
    typename list_t::size_type const npagesize = static_cast<typename list_t::size_type>(
                std::max( 0, _size.y - ( _headersize + _footersize + 1 ) ) );
    if( npagesize != _pagesize ) {
        _pagesize = npagesize;
        rebuild( _olist );
    }
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
std::pair<point, point> advuilist<Container, T>::get_size()
{
    return { _size, _origin };
}

template <class Container, typename T>
void advuilist<Container, T>::savestate( advuilist_save_state *state )
{
    state->idx = static_cast<uint64_t>( _cidx );
    state->sort = static_cast<uint64_t>( _csort );
    state->group = static_cast<uint64_t>( _cgroup );
    state->filter = _filter;
}

template <class Container, typename T>
void advuilist<Container, T>::loadstate( advuilist_save_state *state, bool reb )
{
    _csort = static_cast<typename sortcont_t::size_type>( state->sort );
    _cgroup = static_cast<typename groupercont_t::size_type>( state->group );
    _filter = state->filter;
    if( reb ) {
        rebuild();
    } else {
        _setidx( state->idx );
        _cpage = _idxtopage( _cidx );
    }
}

template <class Container, typename T>
typename advuilist<Container, T>::select_t advuilist<Container, T>::_peek( count_t amount )
{
    if( _list.empty() ) {
        return select_t();
    }

    return select_t{ selection_t{ amount, std::get<ptr_t>( _list[_cidx] ) } };
}

template <class Container, typename T>
typename advuilist<Container, T>::select_t advuilist<Container, T>::_peekall()
{
    select_t ret;
    for( typename list_t::size_type idx = 0; idx < _list.size(); idx++ ) {
        count_t const amount = _count( idx );
        ret.emplace_back( amount, std::get<ptr_t>( _list[idx] ) );
    }

    return ret;
}

template <class Container, typename T>
typename advuilist<Container, T>::count_t
advuilist<Container, T>::_count( typename list_t::size_type idx )
{
    if( _list.empty() ) {
        return 0;
    }
    if( _fcounter ) {
        return _fcounter( *std::get<ptr_t>( _list[idx] ) );
    }
    return 1;
}

template <class Container, typename T>
typename advuilist<Container, T>::count_t advuilist<Container, T>::_peekcount()
{
    return _count( _cidx );
}

template <class Container, typename T>
void advuilist<Container, T>::_initctxt()
{
    using namespace advuilist_literals;
    _ctxt.register_updown();
    _ctxt.register_action( ACTION_PAGE_UP );
    _ctxt.register_action( ACTION_PAGE_DOWN );
    _ctxt.register_action( ACTION_SORT );
    _ctxt.register_action( ACTION_FILTER );
    _ctxt.register_action( ACTION_RESET_FILTER );
    _ctxt.register_action( ACTION_SELECT );
    _ctxt.register_action( ACTION_SELECT_ALL );
    _ctxt.register_action( ACTION_SELECT_PARTIAL );
    _ctxt.register_action( ACTION_SELECT_WHOLE );
    _ctxt.register_action( ACTION_QUIT );
    _ctxt.register_action( ACTION_HELP_KEYBINDINGS );
}

template <class Container, typename T>
void advuilist<Container, T>::_print()
{
    _printheaders();

    point lpos( _firstcol, _headersize );

    nc_color const colcolor = _exit ? c_light_gray : c_white;
    // print column headers
    for( col_t const &col : _columns ) {
        lpos.x += _printcol( col, std::get<std::string>( col ), lpos, colcolor );
    }
    lpos.y += 1;

    // print entries
    typename list_t::size_type const cpagebegin = _pages[_cpage].first;
    typename list_t::size_type const cpageend = _pages[_cpage].second;
    std::string cgroup;
    fglabel_t const &fglabel = std::get<fglabel_t>( _groupers[_cgroup] );
    for( typename list_t::size_type idx = cpagebegin; idx < cpageend; idx++ ) {
        T const &it = *std::get<ptr_t>( _list[idx] );

        // print group header if appropriate
        if( _cgroup != 0 ) {
            std::string const &ngroup = fglabel( it );
            if( ngroup != cgroup ) {
                cgroup = ngroup;
                center_print( _w, lpos.y, c_cyan,
                              string_format( "[%s]", fglabel( it ) ) );
                lpos.y += 1;
            }
        }

        lpos.x = _firstcol;
        nc_color const basecolor = _exit ? c_dark_gray : c_light_gray;
        bool const hilited = idx == _cidx and !_exit;
        nc_color const color = hilited ? hilite( basecolor ) : basecolor;

        if( hilited ) {
            mvwprintz( _w, lpos, color, string_format( "%*s", _innerw, std::string() ) );
        }

        for( col_t const &col : _columns ) {
            lpos.x += _printcol( col, it, lpos, color, hilited );
        }
        lpos.y += 1;
    }

    _printfooters();
}

template <class Container, typename T>
int advuilist<Container, T>::_colwidth( col_t const &col, point const &p )
{
    int const colwidth =
        std::min( _innerw - p.x,
                  static_cast<int>( std::ceil( std::get<cwidth_t>( col ) * _innerw / _tweight ) ) );
    bool const last = p.x + colwidth < _innerw;
    return colwidth - ( last ? _colspacing : 0 );
}

template <class Container, typename T>
int advuilist<Container, T>::_printcol( col_t const &col, std::string const &str, point const &p,
                                        nc_color const &color )
{
    int const colwidth = _colwidth( col, p );
    trim_and_print( _w, p, colwidth, color, str );
    return colwidth + _colspacing;
}

template <class Container, typename T>
int advuilist<Container, T>::_printcol( col_t const &col, T const &it, point const &p,
                                        nc_color const &color, bool hilited )
{
    int const colwidth = _colwidth( col, p );
    std::string const &rawmsg = std::get<fcol_t>( col )( it, colwidth );
    std::string const &msg = hilited ? remove_color_tags( rawmsg ) : rawmsg;
    trim_and_print( _w, p, colwidth, color, msg );
    return colwidth + _colspacing;
}

template <class Container, typename T>
void advuilist<Container, T>::_printheaders()
{
    using namespace advuilist_literals;
    // sort mode
    mvwprintw( _w, { _firstcol, 0 }, _( "< [%s] Sort: %s >" ), _ctxt.get_desc( ACTION_SORT ),
               std::get<std::string>( _sorters[_csort] ) );

    // page index
    typename pagecont_t::size_type const cpage = _cpage + 1;
    typename pagecont_t::size_type const npages = _pages.size();
    std::string const msg2 = string_format( _( "[<] page %1$d of %2$d [>]" ), cpage, npages );
    trim_and_print( _w, { _firstcol, 1 }, _size.x, c_light_blue, msg2 );

    // keybinding hint
    std::string const msg3 = string_format( _( "< [<color_yellow>%s</color>] keybindings >" ),
                                            _ctxt.get_desc( ACTION_HELP_KEYBINDINGS ) );
    right_print( _w, 0, 0, c_white, msg3 );
}

template <class Container, typename T>
void advuilist<Container, T>::_printfooters()
{
    using namespace advuilist_literals;
    // filter
    std::string fprefix = string_format( _( "[%s] Filter" ), _ctxt.get_desc( ACTION_FILTER ) );
    if( !_filter.empty() ) {
        mvwprintw( _w, { _firstcol, _size.y - 1 }, "< %s: %s >", fprefix, _filter );
    } else {
        mvwprintw( _w, { _firstcol, _size.y - 1 }, "< %s >", fprefix );
    }
}

template <class Container, typename T>
void advuilist<Container, T>::_sort( typename sortcont_t::size_type idx )
{
    // FIXME: deduplicate
    if( idx > 0 ) {
        struct cmp {
            fsort_t const &sorter;
            explicit cmp( sorter_t const &_s ) : sorter( std::get<fsort_t>( _s ) ) {}

            constexpr bool operator()( entry_t const &lhs, entry_t const &rhs ) const {
                return sorter( *std::get<ptr_t>( lhs ), *std::get<ptr_t>( rhs ) );
            }
        };
        for( group_t const &v : _groups ) {
            std::stable_sort( v.first, v.second, cmp( _sorters[idx] ) );
        }
    } else {
        // "none" sort mode needs special handling unfortunately
        struct cmp {
            constexpr bool operator()( entry_t const &lhs, entry_t const &rhs ) const {
                return std::get<nidx_t>( lhs ) < std::get<nidx_t>( rhs );
            }
        };
        for( group_t const &v : _groups ) {
            std::stable_sort( v.first, v.second, cmp() );
        }
    }
    _csort = idx;
}

template <class Container, typename T>
void advuilist<Container, T>::_group( typename groupercont_t::size_type idx )
{
    _groups.clear();
    _pages.clear();

    // first sort the list by group id. don't bother sorting for "none" grouping mode
    if( idx != 0 ) {
        struct cmp {
            fgsort_t const &fgsort;
            explicit cmp( grouper_t const &_f ) : fgsort( std::get<fgsort_t>( _f ) ) {}
            constexpr bool operator()( entry_t const &lhs, entry_t const &rhs ) const {
                return fgsort( *std::get<ptr_t>( lhs ), *std::get<ptr_t>( rhs ) );
            }
        };
        std::stable_sort( _list.begin(), _list.end(), cmp( _groupers[idx] ) );
    }

    // build group and page index pairs
    typename list_t::iterator gbegin = _list.begin();
    typename list_t::size_type pbegin = 0;
    // reserve extra space for the first group header;
    typename list_t::size_type lpagesize = _pagesize - ( idx != 0 ? 1 : 0 );
    typename list_t::size_type cpentries = 0;
    fglabel_t const &fglabel = std::get<fglabel_t>( _groupers[idx] );
    for( auto it = _list.begin(); it != _list.end(); ++it ) {
        if( fglabel and fglabel( *it->second ) != fglabel( *gbegin->second ) ) {
            _groups.emplace_back( gbegin, it );
            gbegin = it;
            // group header takes up the space of one entry
            cpentries++;
        }

        cpentries++;

        if( cpentries >= lpagesize ) {
            // avoid printing group headers on the last line of the page
            bool const shiftone = cpentries > lpagesize;
            typename list_t::size_type const ci =
                std::distance( _list.begin(), it ) - ( shiftone ? 1 : 0 );
            _pages.emplace_back( pbegin, ci );
            pbegin = ci;
            cpentries = shiftone ? 1 : 0;
        }
    }
    if( gbegin != _list.end() ) {
        _groups.emplace_back( gbegin, _list.end() );
    }
    if( pbegin < _list.size() or _list.empty() ) {
        _pages.emplace_back( pbegin, _list.size() );
    }
    _cgroup = idx;
}

template <class Container, typename T>
void advuilist<Container, T>::_querysort()
{
    uilist menu;
    menu.text = _( "Sort by…" );
    int const nsorters = static_cast<int>( _sorters.size() );
    int const ngroupers = static_cast<int>( _groupers.size() );
    for( int i = 0; i < nsorters; i++ ) {
        menu.addentry( i, true, MENU_AUTOASSIGN, std::get<std::string>( _sorters[i] ) );
    }
    menu.addentry( nsorters, false, 0, _( "Group by…" ), '-' );
    for( int i = 0; i < ngroupers; i++ ) {
        menu.addentry( nsorters + i, true, MENU_AUTOASSIGN, std::get<std::string>( _groupers[i] ) );
    }
    menu.query();
    if( menu.ret >= 0 ) {
        if( menu.ret < nsorters ) {
            _sort( menu.ret );
        } else {
            _group( menu.ret - nsorters );
            _sort( _csort );
        }
    }
}

template <class Container, typename T>
void advuilist<Container, T>::_queryfilter()
{
    string_input_popup spopup;
    spopup.max_length( 256 ).text( _filter );
    spopup.identifier( _ctxt.get_category() );
    if( !_filterdsc.empty() ) {
        spopup.description( _filterdsc );
    }

    do {
        ui_manager::redraw();
        std::string const nfilter = spopup.query_string( false );
        if( !spopup.canceled() && nfilter != _filter ) {
            _setfilter( nfilter );
        }
    } while( !spopup.canceled() && !spopup.confirmed() );
}

template <class Container, typename T>
typename advuilist<Container, T>::count_t advuilist<Container, T>::_querypartial( count_t max )
{
    string_input_popup spopup;
    spopup.title(
        string_format( _( "How many do you want to select?  [Max %d] (0 to cancel)" ), max ) );
    spopup.width( 20 );
    spopup.only_digits( true );
    count_t amount = spopup.query_int64_t();

    return spopup.canceled() ? 0 : std::min( max, amount );
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
    for( col_t const &col : _columns ) {
        if( lcmatch( remove_color_tags( std::get<fcol_t>( col )( it, 0 ) ), filter ) ) {
            return true;
        }
    }

    return false;
}

template <class Container, typename T>
typename advuilist<Container, T>::pagecont_t::size_type
advuilist<Container, T>::_idxtopage( typename list_t::size_type idx )
{
    typename pagecont_t::size_type cpage = 0;
    while( _pages[cpage].second != 0 and _pages[cpage].second <= idx ) {
        cpage++;
    }
    return cpage;
}

template <class Container, typename T>
void advuilist<Container, T>::_incidx( typename list_t::size_type amount )
{
    if( _pages.back().second == 0 ) {
        _cidx = 0;
        _cpage = 0;
        return;
    }
    if( _cidx == _pages.back().second - 1 ) {
        _cidx = _pages.front().first;
    } else {
        _cidx = std::min( _cidx + amount, _pages.back().second - 1 );
    }
    _cpage = _idxtopage( _cidx );
}

template <class Container, typename T>
void advuilist<Container, T>::_decidx( typename list_t::size_type amount )
{
    if( _pages.back().second == 0 ) {
        _cidx = 0;
        _cpage = 0;
        return;
    }
    if( _cidx == _pages.front().first ) {
        _cidx = _pages.back().second - 1;
    } else {
        _cidx = _pages.front().first + amount > _cidx ? _pages.front().first : _cidx - amount;
    }
    _cpage = _idxtopage( _cidx );
}

template <class Container, typename T>
void advuilist<Container, T>::_setidx( typename list_t::size_type idx )
{
    _cidx = _pages.back().second == 0
            ? 0
            : idx > _pages.back().second - 1 ? _pages.back().second - 1 : idx;
}

#endif // CATA_SRC_ADVUILIST_H
