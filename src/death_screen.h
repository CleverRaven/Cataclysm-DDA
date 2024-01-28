#pragma once
#ifndef CATA_SRC_DEATH_SCREEN_H
#define CATA_SRC_DEATH_SCREEN_H

#include <iosfwd>
#include <vector>

#include "type_id.h"

class JsonObject;

class death_screen
{
    public:
        static void load_death_screen( const JsonObject &jo, const std::string &src );
        static void reset();

        void load( const JsonObject &jo, std::string_view );
        bool was_loaded = false;

        death_screen_id id;
        std::string picture;
};

#endif // CATA_SRC_DEATH_SCREEN_H
