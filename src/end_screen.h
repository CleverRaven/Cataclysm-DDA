#pragma once
#ifndef CATA_SRC_DEATH_SCREEN_H
#define CATA_SRC_DEATH_SCREEN_H

#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "type_id.h"

class JsonObject;
struct const_dialogue;

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
