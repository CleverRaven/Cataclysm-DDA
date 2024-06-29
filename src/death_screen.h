#pragma once
#ifndef CATA_SRC_DEATH_SCREEN_H
#define CATA_SRC_DEATH_SCREEN_H

#include <iosfwd>
#include <vector>

#include "ascii_art.h"
#include "effect_on_condition.h"
#include "type_id.h"

class JsonObject;

class death_screen
{
    public:
        static void load_death_screen( const JsonObject &jo, const std::string &src );

        void load( const JsonObject &jo, std::string_view );
        static const std::vector<death_screen>& get_all();
        bool was_loaded = false;

        death_screen_id id;
        ascii_art_id picture_id;
        std::function<bool( dialogue & )> condition;
};

#endif // CATA_SRC_DEATH_SCREEN_H
