#ifndef CATA_SRC_ADVUILIST_SOURCED_H
#define CATA_SRC_ADVUILIST_SOURCED_H

#include <functional>  // for function
#include <map>         // for map
#include <memory>      // for shared_ptr, __shared_ptr_access
#include <string>      // for string, basic_string, operator==
#include <tuple>       // for tuple
#include <type_traits> // for declval

#include "advuilist.h"        // for advuilist
#include "color.h"            // for color_manager, c_dark_gray, c_ligh...
#include "cursesdef.h"        // for newwin, werase, wnoutrefresh, window
#include "output.h"           // for draw_border, right_print, TERMX
#include "point.h"            // for point
#include "string_formatter.h" // for string_format
#include "ui_manager.h"       // for ui_adaptor

/// wrapper for advuilist that allows for switching between multiple sources
template <class Container, typename T = typename Container::value_type>
class advuilist_sourced : public advuilist<Container, T>
{
  public:
    // source function
    using fsource_t = std::function<Container()>;
    // source availability function
    using fsourceb_t = std::function<bool()>;
    using icon_t = char;
    // label, icon, position, source function, availability function. icon must be unique
    using source_t = std::tuple<std::string, icon_t, point, fsource_t, fsourceb_t>;
    using fctxt_t = typename advuilist<Container, T>::fctxt_t;

    advuilist_sourced( point const &srclayout, point size = { -1, -1 }, point origin = { -1, -1 },
                       std::string const &ctxtname = "default" );

    void addSource( source_t const &src );
    void setSource( icon_t c );

    void setctxthandler( fctxt_t const &func );

  private:
    using srccont = std::map<icon_t, source_t>;

    Container _container;
    srccont _sources;
    fctxt_t _fctxt;
    point _size;
    point _origin;
    point _map_size;
    icon_t _csrc = 0;

    catacurses::window _w;
    std::shared_ptr<ui_adaptor> _ui;

    void _initui();
    void _registerSrc( icon_t c );
    void _ctxthandler( std::string const &action );
    void _printmap();

    // used only for source map window
    static constexpr int const _headersize = 1;
    static constexpr int const _footersize = 1;
    static constexpr int const _firstcol = 1;
    static constexpr int const _iconwidth = 3;
};

template <class Container, typename T>
advuilist_sourced<Container, T>::advuilist_sourced( point const &srclayout, point size,
                                                    point origin, std::string const &ctxtname )
    : advuilist<Container, T>( &_container, size, origin, ctxtname, false ),
      _size( size.x > 0 ? size.x : TERMX / 2, size.y > 0 ? size.y : TERMY ),
      _origin( origin.x >= 0 ? origin.x : TERMX / 2 - _size.x / 2, origin.y >= 0 ? origin.y : 0 ),
      _map_size( srclayout.x * _iconwidth, srclayout.y ), _ui( std::make_shared<ui_adaptor>() )
{
    // leave room for source map window
    point const offset( 0, _headersize + _footersize + _map_size.y );
    this->_resize( _size - offset, _origin + offset );

    this->_init();

    advuilist<Container, T>::setctxthandler(
        [this]( std::string const &action ) { this->_ctxthandler( action ); } );

    _initui();
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::addSource( source_t const &src )
{
    _sources[std::get<icon_t>( src )] = src;

    _registerSrc( std::get<icon_t>( src ) );
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::setSource( icon_t c )
{
    source_t const &src = _sources[c];
    if( std::get<fsourceb_t>( src )() ) {
        _container = std::get<fsource_t>( src )();
        this->rebuild( &_container );
        _csrc = c;
        _ui->invalidate_ui();
    }
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::setctxthandler( fctxt_t const &func )
{
    _fctxt = func;
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::_initui()
{
    _ui->on_screen_resize( [&]( ui_adaptor &ui ) {
        _w = catacurses::newwin( _headersize + _footersize + _map_size.y, _size.x, _origin );
        ui.position_from_window( _w );
    } );
    _ui->mark_resize();

    _ui->on_redraw( [&]( const ui_adaptor & ) {
        werase( _w );
        draw_border( _w );
        _printmap();
        wnoutrefresh( _w );
    } );
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::_registerSrc( icon_t c )
{
    this->get_ctxt()->register_action( string_format( "%s%c", "SOURCE_", c ) );
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::_ctxthandler( std::string const &action )
{
    // where is c++20 when you need it?
    if( action.substr( 0, 7 ) == "SOURCE_" ) {
        icon_t c = action.back();
        setSource( c );
    }

    if( _fctxt ) {
        _fctxt( action );
    }
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::_printmap()
{
    for( auto &it : _sources ) {
        source_t &src = it.second;
        icon_t const icon = std::get<icon_t>( src );
        std::string const color = string_format(
            "<color_%s>",
            icon == _csrc ? "white" : std::get<fsourceb_t>( src )() ? "light_gray" : "red" );
        point const loc = std::get<point>( src );
        std::string const msg = string_format( "[%s%c</color>]", color, icon );

        right_print( _w, loc.y + _headersize, _map_size.x - loc.x * _iconwidth, c_dark_gray, msg );

        if( icon == _csrc ) {
            mvwprintz( _w, { _firstcol, _headersize }, c_light_gray, std::get<std::string>( src ) );
        }
    }
}

#endif // CATA_SRC_ADVUILIST_SOURCED_H
