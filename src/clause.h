#pragma once
#ifndef CATA_SRC_CLAUSE_H
#define CATA_SRC_CLAUSE_H

#include <functional>
#include <string>

#include "optional.h"

class JsonObject;
class translation;
class nc_color;

struct clause_t {
    private:
        struct {
            std::function<cata::optional<translation>()> text;
            std::function<cata::optional<std::string>()> sym;
            std::function<cata::optional<nc_color>()> color;
            std::function<cata::optional<int>()> value;
        } values = { nullptr, nullptr, nullptr, nullptr };
        void load_text_func( const JsonObject &jo );
        void load_sym_func( const JsonObject &jo );
        void load_color_func( const JsonObject &jo );
        void load_value_func( const JsonObject &jo );
    public:
        clause_t() = default;
        explicit clause_t( const JsonObject &jo );

        cata::optional<translation> get_text() const;
        cata::optional<std::string> get_sym() const;
        cata::optional<nc_color> get_color() const;
        cata::optional<int> get_value() const;
};

#endif // CATA_SRC_CLAUSE_H