#pragma once
#ifndef CATA_SRC_BODYGRAPH_H
#define CATA_SRC_BODYGRAPH_H

#include <vector>

#include "color.h"
#include "type_id.h"
#include "damage.h"

class effect;
class Character;
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
    resistances worst_case;
    resistances median_case;
    resistances best_case;
    std::string parent_bp_name;
    std::vector<effect> effects;
    std::pair<int, nc_color> temperature;
    std::string temp_approx;
    int avg_coverage = 0;
    int total_encumbrance = 0;
    int part_hp_cur = 0;
    int part_hp_max = 0;
    float wetness = 0;
    bool specific_sublimb = false;
};

struct bodygraph {
    bodygraph_id id;
    std::optional<bodypart_id> parent_bp;
    std::optional<bodygraph_id> mirror;
    std::vector<std::vector<std::string>> rows;
    std::vector<std::vector<std::string>> fill_rows;
    std::map<std::string, bodygraph_part> parts;
    std::string fill_sym;
    nc_color fill_color = c_white;
    bool was_loaded = false;

    static void load_bodygraphs( const JsonObject &jo, const std::string &src );
    static void finalize_all();
    static void check_all();
    static void reset();
    static const std::vector<bodygraph> &get_all();
    void load( const JsonObject &jo, std::string_view src );
    void finalize();
    void check() const;
};

// Draws the bodygraph UI for the given character and bodygraph (defaults to Full Body graph)
void display_bodygraph( const Character &u, const bodygraph_id &id = bodygraph_id::NULL_ID() );

using bodygraph_callback =
    std::function<std::string( const bodygraph_part *, std::string )>;

/**
 * @brief Returns a prepared bodygraph ready to be rendered line-by-line.
 *
 * The callback function is for processing a bodygraph symbol. The args are as follows:
 *
 * @param bodygraph_part points to the graph part represented in the symbol (or nullptr if none)
 * @param string the default symbol to use if no other processing is done
 * @returns the final colorized symbol
 */
std::vector<std::string> get_bodygraph_lines( const Character &u,
        const bodygraph_callback &fragment_cb, const bodygraph_id &id = bodygraph_id::NULL_ID(),
        int width = 0, int height = 0 );

#endif // CATA_SRC_BODYGRAPH_H
