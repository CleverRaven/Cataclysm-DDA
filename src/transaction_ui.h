#ifndef CATA_SRC_TRANSACTION_UI_H
#define CATA_SRC_TRANSACTION_UI_H

#include <array>      // for array
#include <cstddef>    // for size_t
#include <functional> // for function
#include <queue>      // for queue
#include <string>     // for string, operator==, basic_string

#include "advuilist.h"         // for advuilist
#include "advuilist_const.h"   // for ACTION_SWITCH_PANES, CTXT_DEFAULT
#include "advuilist_sourced.h" // for advuilist_sourced
#include "output.h"            // for TERMX, TERMY
#include "point.h"             // for point
#include "uistate.h"           // for transaction_ui_save_state

/// two-pane transaction ui based on advuilist_sourced
template <class Container, typename T = typename Container::value_type>
class transaction_ui
{
  public:
    using advuilist_t = advuilist_sourced<Container, T>;
    using select_t = typename advuilist_t::select_t;
    using fselect_t = std::function<void( transaction_ui<Container, T> *, select_t )>;
    using fctxt_t = std::function<void( transaction_ui<Container, T> *, std::string const & )>;

    enum class event { QUIT = 0, SWITCH = 1, NEVENTS = 2 };

    transaction_ui( point const &srclayout, point size = { -1, -1 }, point origin = { -1, -1 },
                    std::string const &ctxtname = CTXT_DEFAULT );

    advuilist_t *left();
    advuilist_t *right();
    advuilist_t *curpane();
    advuilist_t *otherpane();

    void setctxthandler( fctxt_t const &func );
    void on_select( fselect_t const &func );
    void pushevent( event const &ev );

    void show();
    void savestate( transaction_ui_save_state *state );
    void loadstate( transaction_ui_save_state *state );

  private:
    constexpr static std::size_t npanes = 2;

    using panecont_t = std::array<advuilist_t, 2>;

    constexpr static typename panecont_t::size_type const _left = 0;
    constexpr static typename panecont_t::size_type const _right = 1;

    point _size;
    point _origin;
    panecont_t _panes;
    fselect_t _fselect;
    std::queue<event> _queue;
    fctxt_t _fctxt;
    typename panecont_t::size_type _cpane = 0;
    bool _exit = true;

    void _ctxthandler( advuilist<Container, T> *ui, std::string const &action );
    void _process( event const &ev );
};

// *INDENT-OFF*
template <class Container, typename T>
transaction_ui<Container, T>::transaction_ui( point const &srclayout, point size, point origin,
                                              std::string const &ctxtname )
    : _size( size.x > 0 ? size.x : ( TERMX * 3 ) / 4, size.y > 0 ? size.y : TERMY ),
      _origin( origin.x >= 0 ? origin.x : TERMX / 2 - _size.x / 2, origin.y >= 0 ? origin.y : 0 ),
      _panes{ advuilist_t{ srclayout, { _size.x / 2, _size.y }, _origin, ctxtname },
              advuilist_t{ srclayout,
                           { _size.x / 2, _size.y },
                           { _origin.x + _size.x / 2, _origin.y },
                           ctxtname } }
// *INDENT-ON*
{
    for( auto &v : _panes ) {
        v.get_ctxt()->register_action( ACTION_SWITCH_PANES );
        v.setctxthandler( [this]( advuilist<Container, T> *ui, std::string const &action ) {
            this->_ctxthandler( ui, action );
        } );
    }
}

template <class Container, typename T>
typename transaction_ui<Container, T>::advuilist_t *transaction_ui<Container, T>::left()
{
    return &_panes[_left];
}

template <class Container, typename T>
typename transaction_ui<Container, T>::advuilist_t *transaction_ui<Container, T>::right()
{
    return &_panes[_right];
}

template <class Container, typename T>
typename transaction_ui<Container, T>::advuilist_t *transaction_ui<Container, T>::curpane()
{
    return &_panes[_cpane];
}

template <class Container, typename T>
typename transaction_ui<Container, T>::advuilist_t *transaction_ui<Container, T>::otherpane()
{
    return &_panes[1 - _cpane];
}

template <class Container, typename T>
void transaction_ui<Container, T>::setctxthandler( fctxt_t const &func )
{
    _fctxt = func;
}

template <class Container, typename T>
void transaction_ui<Container, T>::on_select( fselect_t const &func )
{
    _fselect = func;
}

template <class Container, typename T>
void transaction_ui<Container, T>::pushevent( event const &ev )
{
    _queue.emplace( ev );
}

template <class Container, typename T>
void transaction_ui<Container, T>::show()
{
    // ensure that both panes are visible and our current pane is on top
    _panes[1 - _cpane].totop();
    _panes[_cpane].totop();

    _exit = false;

    while( !_exit ) {
        auto selection = _panes[_cpane].select();
        if( _fselect and !selection.empty() ) {
            _fselect( this, selection );
        }

        while( !_queue.empty() ) {
            event const ev = _queue.front();
            _queue.pop();
            _process( ev );
        }
    }
}

template <class Container, typename T>
void transaction_ui<Container, T>::savestate( transaction_ui_save_state *state )
{
    _panes[_left].savestate( &state->left );
    _panes[_right].savestate( &state->right );
    state->cpane = _cpane;
}

template <class Container, typename T>
void transaction_ui<Container, T>::loadstate( transaction_ui_save_state *state )
{
    _panes[_left].loadstate( &state->left );
    _panes[_right].loadstate( &state->right );
    _cpane = state->cpane;
}

template <class Container, typename T>
void transaction_ui<Container, T>::_ctxthandler( advuilist<Container, T> *ui,
                                                 std::string const &action )
{
    if( action == ACTION_QUIT ) {
        _queue.emplace( event::QUIT );
    }

    if( action == ACTION_SWITCH_PANES ) {
        _queue.emplace( event::SWITCH );
        ui->suspend();
    }

    if( _fctxt ) {
        _fctxt( this, action );
    }
}

template <class Container, typename T>
void transaction_ui<Container, T>::_process( event const &ev )
{
    switch( ev ) {
        case event::QUIT: {
            _panes[_left].hide();
            _panes[_right].hide();
            _exit = true;
            break;
        }
        case event::SWITCH: {
            // redraw darker borders, etc
            _panes[_cpane].get_ui()->redraw();

            _cpane = -_cpane + 1;
            _panes[_cpane].totop();
            break;
        }
        case event::NEVENTS: {
            break;
        }
    }
}

#endif // CATA_SRC_TRANSACTION_UI_H