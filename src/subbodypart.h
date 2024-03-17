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
#include "type_id.h"

class JsonObject;
class JsonOut;
class JsonValue;
struct sub_body_part_type;
struct body_part_type;

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
    std::vector<std::pair<sub_bodypart_str_id, mod_id>> src;
    sub_bodypart_str_id opposite;

    bool was_loaded = false;

    // name of the sub part
    translation name;

    translation name_multiple;

    side part_side = side::BOTH;

    // the body part this belongs to
    string_id<body_part_type> parent;

    // this flags that the sub location
    // is in addition to the normal limb
    // parts. Currently used for torso for
    // hanging locations.
    bool secondary = false;

    // the maximum coverage value for this part
    // if something entirely covered this part it
    // would have this value
    int max_coverage = 0;

    // the locations that are under this location
    // used with secondary locations to define what sublocations
    // exist bellow them for things like discomfort
    std::vector<sub_bodypart_str_id> locations_under;

    // These subparts act like this limb for armor coverage
    // TODO: Coverage/Encumbrance multiplier
    std::vector<sub_bodypart_str_id> similar_bodyparts;

    static void load_bp( const JsonObject &jo, const std::string &src );

    void load( const JsonObject &jo, std::string_view src );

    // combine matching body part strings together for printing
    static std::vector<translation> consolidate( std::vector<sub_bodypart_id> &covered );

    // Clears all bps
    static void reset();
    // Post-load finalization
    static void finalize_all();

    static void finalize();
};

#endif // CATA_SRC_SUBBODYPART_H
