#ifndef CATA_SRC_ADVUILIST_SOURCED_H
#define CATA_SRC_ADVUILIST_SOURCED_H

#include <functional>  // for function
#include <map>         // for map
#include <memory>      // for shared_ptr, __shared_ptr_access
#include <string>      // for string, basic_string, operator==
#include <tuple>       // for tuple
#include <type_traits> // for declval

#include "advuilist.h"        // for advuilist
#include "advuilist_const.h"  // for ACTION_*
#include "color.h"            // for color_manager, c_dark_gray, c_ligh...
#include "cursesdef.h"        // for newwin, werase, wnoutrefresh, window
#include "output.h"           // for draw_border, right_print, TERMX
#include "point.h"            // for point
#include "string_formatter.h" // for string_format
#include "ui_manager.h"       // for ui_adaptor
#include "uistate.h"          // for advuilist_save_state

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
        using slotidx_t = std::size_t;
        using getsource_t = std::pair<slotidx_t, icon_t>;
        // label, icon, source function, availability function. icon must be unique and not zero
        using source_t = std::tuple<std::string, icon_t, fsource_t, fsourceb_t>;
        using fctxt_t = typename advuilist<Container, T>::fctxt_t;
        using select_t = typename advuilist<Container, T>::select_t;

        advuilist_sourced( point const &srclayout, point size = { -9, -9 }, point origin = { -9, -9 },
                           std::string const &ctxtname = advuilist_literals::CTXT_DEFAULT );

        /// binds a new source to slot
        void addSource( slotidx_t slot, source_t const &src );
        /// sets active source slot
        ///@param slot
        ///@param icon
        ///@param fallthrough used internally by rebuild() to ensure that the internal list is valid
        void setSource( slotidx_t slot, icon_t icon = 0, bool fallthrough = false );
        getsource_t getSource();

        select_t select();
        void rebuild( Container *list = nullptr );
        void totop();
        void hide();

        void setctxthandler( fctxt_t const &func );

        void savestate( advuilist_save_state *state );
        void loadstate( advuilist_save_state *state, bool reb = true );

    private:
        using slotcont_t = std::map<icon_t, source_t>;
        /// active source for this slot, container of sources bound to this slot
        using slot_t = std::pair<icon_t, slotcont_t>;
        /// key is slot index
        using srccont_t = std::map<slotidx_t, slot_t>;

        Container _container;
        srccont_t _sources;
        fctxt_t _fctxt;
        point _size;
        point _origin;
        point _map_size;
        slotidx_t _cslot = 0;
        bool needsinit = true;

        catacurses::window _w;
        std::shared_ptr<ui_adaptor> _mapui;

        void _initui();
        void _registerSrc( slotidx_t c );
        void _ctxthandler( advuilist<Container, T> *ui, std::string const &action );
        void _printmap();
        void _cycleslot( slotidx_t idx );
        std::size_t _countactive( slotidx_t idx );

        // used only for source map window
        static constexpr int const _headersize = 1;
        static constexpr int const _footersize = 1;
        static constexpr int const _firstcol = 1;
        static constexpr int const _iconwidth = 3;
};

// *INDENT-OFF*
template <class Container, typename T>
advuilist_sourced<Container, T>::advuilist_sourced( point const &srclayout, point size,
                                                    point origin, std::string const &ctxtname )
    : advuilist<Container, T>( &_container, size, origin, ctxtname, false ),
      _size( size.x > 0 ? size.x : TERMX / 2, size.y > 0 ? size.y : TERMY ),
      _origin( origin.x >= 0 ? origin.x : TERMX / 2 - _size.x / 2, origin.y >= 0 ? origin.y : 0 ),
      _map_size( srclayout )
// *INDENT-ON*

{
    using namespace advuilist_literals;
    // leave room for source map window
    point const offset( 0, _headersize + _footersize + _map_size.y );
    advuilist<Container, T>::_resize( _size - offset, _origin + offset );

    advuilist<Container, T>::_init();

    advuilist<Container, T>::setctxthandler(
        [this]( advuilist<Container, T> *ui, std::string const & action )
    {
        advuilist_sourced<Container, T>::_ctxthandler( ui, action );
    } );

    advuilist<Container, T>::get_ctxt()->register_action( ACTION_CYCLE_SOURCES );
    advuilist<Container, T>::get_ctxt()->register_action( ACTION_NEXT_SLOT );
    advuilist<Container, T>::get_ctxt()->register_action( ACTION_PREV_SLOT );
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::addSource( slotidx_t slot, source_t const &src )
{
    icon_t const icon = std::get<icon_t>( src );
    auto it = _sources.find( slot );
    if( it != _sources.end() ) {
        std::get<slotcont_t>( it->second )[icon] = src;
    } else {
        // FIXME: figure out emplace for this
        _sources[slot] = slot_t( icon, slotcont_t() );
        std::get<slotcont_t>( _sources[slot] )[icon] = src;

        _registerSrc( slot );
    }
}

// FIXME: try to clean this up on a fresh day
template <class Container, typename T>
void advuilist_sourced<Container, T>::setSource( slotidx_t slot, icon_t icon, bool fallthrough )
{
    slot_t &_slot = _sources[slot];
    slotcont_t &slotcont = std::get<slotcont_t>( _slot );
    icon_t _icon = icon == 0 ? std::get<icon_t>( _slot ) : icon;
    // only set the icon if it (still) exists in this slot, otherwise use the first one in slot
    // rebuild() needs to additionally check that the source is available
    std::size_t const exists = slotcont.count( _icon );
    if( exists == 0 or ( fallthrough and !std::get<fsourceb_t>( slotcont[_icon] )() ) ) {
        _icon = std::get<icon_t const>( *slotcont.begin() );
        _slot.first = _icon;
    }

    source_t const &src = slotcont[_icon];
    if( std::get<fsourceb_t>( src )() ) {
        _slot.first = _icon;
        _container = std::get<fsource_t>( src )();
        advuilist<Container, T>::rebuild();
        _cslot = slot;
        if( _mapui ) {
            _mapui->invalidate_ui();
        }
    } else if( fallthrough ) {
        // if we still don't have a valid source on rebuild(), empty the internal container
        _container.clear();
    }
}

template <class Container, typename T>
typename advuilist_sourced<Container, T>::getsource_t advuilist_sourced<Container, T>::getSource()
{
    return { _cslot, std::get<icon_t>( _sources[_cslot] ) };
}

template <class Container, typename T>
typename advuilist_sourced<Container, T>::select_t advuilist_sourced<Container, T>::select()
{
    if( !_mapui ) {
        _initui();
    }
    if( needsinit ) {
        rebuild();
    }

    return advuilist<Container, T>::select();
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::rebuild( Container *list )
{
    setSource( _cslot, 0, true );
    needsinit = false;
    advuilist<Container, T>::rebuild( list );
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::hide()
{
    advuilist<Container, T>::hide();
    _mapui.reset();
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::totop()
{
    _initui();
    advuilist<Container, T>::totop();
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::setctxthandler( fctxt_t const &func )
{
    _fctxt = func;
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::savestate( advuilist_save_state *state )
{
    advuilist<Container, T>::savestate( state );
    state->slot = _cslot;
    state->icon = std::get<icon_t>( _sources[_cslot] );
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::loadstate( advuilist_save_state *state, bool reb )
{
    _cslot = state->slot;
    setSource( _cslot, state->icon, true );

    advuilist<Container, T>::loadstate( state, reb );
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::_initui()
{
    _mapui = std::make_shared<ui_adaptor>();
    _mapui->on_screen_resize( [&]( ui_adaptor & ui ) {
        _w = catacurses::newwin( _headersize + _footersize + _map_size.y, _size.x, _origin );
        ui.position_from_window( _w );
    } );
    _mapui->mark_resize();

    _mapui->on_redraw( [&]( const ui_adaptor & ) {
        werase( _w );
        draw_border( _w );
        _printmap();
        wnoutrefresh( _w );
    } );
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::_registerSrc( slotidx_t c )
{
    using namespace advuilist_literals;
    advuilist<Container, T>::get_ctxt()->register_action(
        string_format( "%s%d", ACTION_SOURCE_PRFX, c ) );
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::_ctxthandler( advuilist<Container, T> * /*ui*/,
        std::string const &action )
{
    using namespace advuilist_literals;
    // where is c++20 when you need it?
    if( action.substr( 0, ACTION_SOURCE_PRFX_len ) == ACTION_SOURCE_PRFX ) {
        slotidx_t slotidx = std::stoul( action.substr( ACTION_SOURCE_PRFX_len, action.size() ) );
        setSource( slotidx );
    } else if( action == ACTION_CYCLE_SOURCES ) {
        _cycleslot( _cslot );
    } else if( action == ACTION_NEXT_SLOT ) {
        setSource( _cslot == _sources.size() - 1 ? 0 : _cslot + 1 );
    } else if( action == ACTION_PREV_SLOT ) {
        setSource( _cslot == 0 ? _sources.size() - 1 : _cslot - 1 );
    }

    if( _fctxt ) {
        _fctxt( this, action );
    }
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::_printmap()
{
    // print the name of the current source. we're doing it here instead of down in the loop
    // so that it doesn't cover the source map if it's too long
    icon_t const ci = std::get<icon_t>( _sources[_cslot] );
    mvwprintz( _w, { _firstcol, _headersize }, c_light_gray,
               std::get<std::string>( std::get<slotcont_t>( _sources[_cslot] )[ci] ) );

    for( typename srccont_t::value_type &it : _sources ) {
        slotidx_t const slotidx = it.first;
        slot_t const &slot = it.second;
        icon_t const icon = std::get<icon_t>( slot );
        slotcont_t const &slotcont = std::get<slotcont_t>( slot );
        source_t const &src = slotcont.at( icon );

        // FIXME: maybe cache this?
        std::size_t nactive = _countactive( slotidx );

        std::string const color = string_format(
                                      "<color_%s>",
                                      slotidx == _cslot ? "white" : std::get<fsourceb_t>( src )() ? "light_gray" : "red" );
        point const loc( slotidx % _map_size.x, slotidx / _map_size.x );
        // visually indicate we have more than one available source in this slot
        std::string const fmt = nactive > 1 ? "<%s%c</color>>" : "[%s%c</color>]";
        std::string const msg = string_format( fmt, color, icon );

        right_print( _w, loc.y + _headersize, ( _map_size.x - loc.x ) * _iconwidth, c_dark_gray,
                     msg );
    }
}

template <class Container, typename T>
void advuilist_sourced<Container, T>::_cycleslot( slotidx_t idx )
{
    slot_t &slot = _sources[idx];
    icon_t const icon = std::get<icon_t>( slot );
    slotcont_t &slotcont = std::get<slotcont_t>( slot );

    if( slotcont.size() == 1 ) {
        return;
    }

    // get the next available source in the current slot
    auto it = ++slotcont.find( icon );
    for( ; it != slotcont.end(); ++it ) {
        if( std::get<fsourceb_t>( it->second )() ) {
            break;
        }
    }
    // roll over to the first source
    if( it == slotcont.end() ) {
        it = slotcont.begin();
    }
    // set active icon for this slot
    slot.first = it->first;

    setSource( _cslot, it->first );
}

template <class Container, typename T>
std::size_t advuilist_sourced<Container, T>::_countactive( slotidx_t idx )
{
    std::size_t nactive = 0;
    for( auto const &it : std::get<slotcont_t>( _sources[idx] ) ) {
        nactive += static_cast<std::size_t>( std::get<fsourceb_t>( it.second )() );
    }

    return nactive;
}

#endif // CATA_SRC_ADVUILIST_SOURCED_H
