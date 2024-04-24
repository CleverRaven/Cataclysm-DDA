#pragma once
#ifndef CATA_SRC_ASCII_ART_H
#define CATA_SRC_ASCII_ART_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "type_id.h"

class JsonObject;

class ascii_art
{
    public:
        static void load_ascii_art( const JsonObject &jo, const std::string &src );
        static void reset();

        void load( const JsonObject &jo, std::string_view );
        bool was_loaded = false;

        ascii_art_id id;
        std::vector<std::pair<ascii_art_id, mod_id>> src;
        std::vector<std::string> picture;
};

#endif // CATA_SRC_ASCII_ART_H
