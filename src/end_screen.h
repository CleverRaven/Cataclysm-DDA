#pragma once
#ifndef CATA_SRC_DEATH_SCREEN_H
#define CATA_SRC_DEATH_SCREEN_H

#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cata_imgui.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
struct const_dialogue;

class end_screen_data
{
        friend class end_screen_ui_impl;
    public:
        void draw_end_screen_ui( bool actually_dead = true );
};

class end_screen_ui_impl : public cataimgui::window
{
    public:
        std::string text;
        explicit end_screen_ui_impl() : cataimgui::window( _( "The End" ) ) {
        }
    protected:
        void draw_controls() override;
};

struct end_screen {
    public:
        static void load_end_screen( const JsonObject &jo, const std::string &src );

        void load( const JsonObject &jo, std::string_view );
        static const std::vector<end_screen> &get_all();
        bool was_loaded = false;

        end_screen_id id;
        ascii_art_id picture_id;
        std::function<bool( const_dialogue const & )> condition;
        int priority;
        std::vector<std::pair<std::pair<int, int>, std::string>> added_info;
        std::string last_words_label;
};

#endif // CATA_SRC_DEATH_SCREEN_H
