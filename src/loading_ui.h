#pragma once
#ifndef LOADING_UI_H
#define LOADING_UI_H

#include <functional>
#include <memory>
#include <vector>

class uimenu;

class loading_ui
{
    private:
        struct named_callback {
            std::string name;
            std::function<void()> fun;
        };

        std::unique_ptr<uimenu> menu;
        std::vector<named_callback> entries;
    public:
        loading_ui( bool display );
        ~loading_ui();

        void queue_callback( const std::string &description, std::function<void()> callback );
        void set_menu_description( const std::string &desc );
        void process();
};

#endif