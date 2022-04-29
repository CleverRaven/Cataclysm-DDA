#pragma once
#ifndef CATA_SRC_BODYGRAPH_H
#define CATA_SRC_BODYGRAPH_H

#include <vector>

#include "color.h"
#include "type_id.h"
#include "damage.h"

class JsonObject;

struct bodygraph_part {
    // Which main bodyparts are represented in this graph section
    std::vector<bodypart_id> bodyparts;
    // Which sub bodyparts are represented in this graph section
    std::vector<sub_bodypart_id> sub_bodyparts;
    // If selected, which graph gets shown next?
    bodygraph_id nested_graph;
    // What color to use when this graph section is selected
    nc_color sel_color;
    // What symbol to use for fragments that belong to this section
    std::string sym;
};

// info that needs to be passed around
struct bodygraph_info {
    std::vector<std::string> worn_names;
    int avg_coverage = 0;
    int total_encumbrance = 0;
    resistances worst_case;
    resistances median_case;
    resistances best_case;
    bool specific_sublimb = false;
};

struct bodygraph {
    bodygraph_id id;
    cata::optional<bodypart_id> parent_bp;
    cata::optional<bodygraph_id> mirror;
    std::vector<std::vector<std::string>> rows;
    std::map<std::string, bodygraph_part> parts;
    std::string fill_sym;
    nc_color fill_color = c_white;
    bool was_loaded = false;

    static void load_bodygraphs( const JsonObject &jo, const std::string &src );
    static void finalize_all();
    static void check_all();
    static void reset();
    static const std::vector<bodygraph> &get_all();
    void load( const JsonObject &jo, const std::string &src );
    void finalize();
    void check() const;
};

void display_bodygraph( const bodygraph_id &id = bodygraph_id::NULL_ID() );

#endif // CATA_SRC_BODYGRAPH_H
