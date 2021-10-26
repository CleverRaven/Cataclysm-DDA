#pragma once
#ifndef CATA_SRC_SUBBODYPART_H
#define CATA_SRC_SUBBODYPART_H

#include <array>
#include <cstddef>
#include <initializer_list>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

#include "enums.h"
#include "flat_set.h"
#include "int_id.h"
#include "string_id.h"
#include "translations.h"

class JsonObject;
class JsonOut;
class JsonValue;
struct sub_body_part_type;

using sub_bodypart_str_id = string_id<sub_body_part_type>;
using sub_bodypart_id = int_id<sub_body_part_type>;


enum class side : int {
    BOTH,
    LEFT,
    RIGHT,
    num_sides
};

template<>
struct enum_traits<side> {
    static constexpr side last = side::num_sides;
};

struct sub_body_part_type {

    sub_bodypart_str_id id;
    bool was_loaded = false;

    //name of the sub part
    translation name;

    side part_side = side::BOTH;

    static void load_bp( const JsonObject &jo, const std::string &src );

    void load( const JsonObject &jo, const std::string &src );

    // Clears all bps
    static void reset();
    // Post-load finalization
    static void finalize_all();

    static void finalize();
};

#endif // CATA_SRC_SUBBODYPART_H
