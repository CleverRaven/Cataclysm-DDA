#ifndef CATA_SRC_ADVUILIST_SOURCED_H
#define CATA_SRC_ADVUILIST_SOURCED_H

#include <tuple>   // for tuple
#include <utility> // for forward

#include "advuilist.h" // for advuilist
#include "point.h"     // for point

// TODO:
//   UI for source map
//   expand source_t with a function to check availability
//   map sources by name instead of icon. use string_id maybe?

/// wrapper for advuilist that allows for switching between multiple sources
template <class Container, typename T = typename Container::value_type>
class advuilist_sourced
{
  public:
    // source function
    using fsource_t = std::function<Container()>;
    // label, icon, position, function
    using icon_t = char;
    using source_t = std::tuple<std::string, icon_t, point, fsource_t>;

    advuilist_sourced( point const &srclayout, point size = { -1, -1 }, point origin = { -1, -1 },
                       std::string const &ctxtname = "default" );

    void addSource( source_t const &src );
    void setSource( icon_t c );

    // *** forwarders and boilerplate***
    using advuilist_t = advuilist<Container, T>;
    using fcol_t = typename advuilist_t::fcol_t;
    using cwidth_t = typename advuilist_t::cwidth_t;
    using col_t = typename advuilist_t::col_t;
    using fsort_t = typename advuilist_t::fsort_t;
    using sorter_t = typename advuilist_t::sorter_t;
    using ffilter_t = typename advuilist_t::ffilter_t;
    using fctxt_t = typename advuilist_t::fctxt_t;
    using gid_t = typename advuilist_t::gid_t;
    using fgid_t = typename advuilist_t::fgid_t;
    using fglabel_t = typename advuilist_t::fglabel_t;
    using grouper_t = typename advuilist_t::grouper_t;
    using ptr_t = typename advuilist_t::ptr_t;
    using select_t = typename advuilist_t::select_t;

    template <class... Args>
    void setColumns( Args &&... args )
    {
        _a->setColumns( std::forward<Args>( args )... );
    }

    template <class... Args>
    void addSorter( Args &&... args )
    {
        _a->addSorter( std::forward<Args>( args )... );
    }

    template <class... Args>
    void addGrouper( Args &&... args )
    {
        _a->addGrouper( std::forward<Args>( args )... );
    }

    template <class... Args>
    void setfilterf( Args &&... args )
    {
        _a->setfilterf( std::forward<Args>( args )... );
    }

    select_t select()
    {
        return _a->select();
    }

    template <class... Args>
    void sort( Args &&... args )
    {
        _a->sort( std::forward<Args>( args )... );
    }

    select_t peek()
    {
        return _a->peek();
    };

    void suspend()
    {
        _a->suspend();
    }

    input_context *get_ctxt()
    {
        return _a->get_ctxt();
    }

    catacurses::window *get_window()
    {
        return _a->get_window();
    }
    std::shared_ptr<ui_adaptor> get_ui()
    {
        return _a->get_ui();
    }
    // *** end forwarders ***

    void setctxthandler( fctxt_t const &func );

  private:
    using srccont = std::map<icon_t, source_t>;

    Container _container;
    srccont _sources;
    fctxt_t _fctxt;
    point _size;
    point _origin;

    std::unique_ptr<advuilist_t> _a;

    void _registerSrc( icon_t c );
    void _ctxthandler( std::string const &action );
};

template <class Container, typename T>
advuilist_sourced<Container, T>::advuilist_sourced( point const & /*srclayout*/, point size,
                                                    point origin, std::string const &ctxtname )
    : _size( size.x > 0 ? size.x : TERMX / 2, size.y > 0 ? size.y : TERMY ),
      _origin( origin.x >= 0 ? origin.x : TERMX / 2 - _size.x / 2, origin.y >= 0 ? origin.y : 0 )
{
    // FIXME: leave room for source map window
    _a = std::make_unique<advuilist_t>( &_container, _size, _origin, ctxtname );
    _a->setctxthandler( [this]( std::string const &action ) { this->_ctxthandler( action ); } );
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
    _container = std::get<fsource_t>( _sources[c] )();
    _a->rebuild( &_container );
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::setctxthandler( fctxt_t const &func )
{
    _fctxt = func;
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::_registerSrc( icon_t c )
{
    _a->get_ctxt()->register_action( string_format( "%s%c", "SOURCE_", c ) );
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

#endif // CATA_SRC_ADVUILIST_SOURCED_H
