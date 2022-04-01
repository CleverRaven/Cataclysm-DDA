#pragma once
#ifndef CATA_SRC_UI_WINDOW_H
#define CATA_SRC_UI_WINDOW_H

#include "cursesdef.h"
#include "input.h"
#include "memory_fast.h"
#include "output.h"
#include "ui_layout.h"

class ui_adaptor;

class size_scalar
{
    public:
        explicit size_scalar( int val ) : _fun( [val]() {
            return val;
        } ) {};
        explicit size_scalar( std::function<int()> fun ) : _fun( std::move( fun ) ) {};
        explicit size_scalar() = default;
        explicit operator bool() {
            return bool( _fun );
        };
        int operator()() {
            return _fun();
        };


    private:
        std::function<int()> const _fun;
};

template<typename T>
class ui_window
{
    public:
        ui_window() : x( TERMY / 4 ), y( TERMY / 4 ), width( TERMX / 2 ), height( TERMY / 2 ),
            layout( point_zero, width(), height() ) {};
        ui_window( point pos, int width, int height ) : x( pos.x ), y( pos.y ), width( width ),
            height( height ), layout( point_zero, width, height ) {};
        virtual T show() = 0;
        void add_element( shared_ptr_fast<ui_element> &element );

        size_scalar x;
        size_scalar y;
        size_scalar width;
        size_scalar height;
    protected:
        shared_ptr_fast<ui_adaptor> create_or_get_ui_adaptor();
        input_context ctxt;
        // todo: abstract layout class for different possible layouts
        absolute_layout layout;
    private:
        void draw();
        void resize( ui_adaptor &ui );

        weak_ptr_fast<ui_adaptor> ui;
        catacurses::window window;
};

#endif // CATA_SRC_UI_WINDOW_H
