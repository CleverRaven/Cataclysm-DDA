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
            std::function<cata::optional<int>()> val_min;
            std::function<cata::optional<int>()> val_max;
            std::pair<std::function<cata::optional<int>()>, std::function<cata::optional<int>()>> val_norm;
        } values = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, { nullptr, nullptr } };
    public:
        clause_t() = default;
        explicit clause_t( const JsonObject &jo );

        cata::optional<translation> get_text() const;
        cata::optional<std::string> get_sym() const;
        cata::optional<nc_color> get_color() const;
        cata::optional<int> get_value() const;
        cata::optional<int> get_val_min() const;
        cata::optional<int> get_val_max() const;
        std::pair<cata::optional<int>, cata::optional<int>> get_val_norm() const;
};

#endif // CATA_SRC_CLAUSE_H