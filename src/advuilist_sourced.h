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
class advuilist_sourced : public advuilist<Container, T>
{
  public:
    // source function
    using fsource_t = std::function<Container()>;
    // label, icon, position, function
    using icon_t = char;
    using source_t = std::tuple<std::string, icon_t, point, fsource_t>;
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
    typename advuilist<Container, T>::fctxt_t _fctxt;
    point _size;
    point _origin;

    void _registerSrc( icon_t c );
    void _ctxthandler( std::string const &action );
};

// FIXME: leave room for source map window
template <class Container, typename T>
advuilist_sourced<Container, T>::advuilist_sourced( point const & /*srclayout*/, point size,
                                                    point origin, std::string const &ctxtname )
    : advuilist<Container, T>( &_container, size, origin, ctxtname, false ),
      _size( size.x > 0 ? size.x : TERMX / 2, size.y > 0 ? size.y : TERMY ),
      _origin( origin.x >= 0 ? origin.x : TERMX / 2 - _size.x / 2, origin.y >= 0 ? origin.y : 0 )
{
    this->_init();
    advuilist<Container, T>::setctxthandler(
        [this]( std::string const &action ) { this->_ctxthandler( action ); } );
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
    this->rebuild( &_container );
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::setctxthandler( fctxt_t const &func )
{
    _fctxt = func;
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

#endif // CATA_SRC_ADVUILIST_SOURCED_H
