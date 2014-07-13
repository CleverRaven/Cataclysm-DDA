#ifndef _REQUIREMENTS_H_
#define _REQUIREMENTS_H_

#include <string>
#include <vector>
#include <map>
#include "color.h"
#include "output.h"

class JsonObject;
class JsonArray;
class inventory;

// Denotes the id of an item type
typedef std::string itype_id;
// Denotes the id of an item quality
typedef std::string quality_id;

struct quality {
    quality_id id;
    // Translated name
    std::string name;

    typedef std::map<quality_id, quality> quality_map;
    static quality_map qualities;
    static void reset();
    static void load(JsonObject &jo);
    static std::string get_name(const quality_id &id);
    static bool has(const quality_id &id);
};

struct component {
    itype_id type;
    int count;
    // -1 means the player doesn't have the item, 1 means they do,
    // 0 means they have item but not enough for both tool and component
    int available;

    component() : type("null") , count(0) , available(-1) {
    }
    component(const itype_id &TYPE, int COUNT) : type (TYPE), count (COUNT), available(-1) {
    }
};

struct quality_requirement {
    quality_id type;
    int count;
    int level;
    bool available;

    quality_requirement() : type("UNKNOWN"), count(0), level(0), available(false) {
    }
    quality_requirement(const quality_id &TYPE, int COUNT, int LEVEL) : type(TYPE), count(COUNT), level(LEVEL), available(false) {
    }
};

struct requirements {
    // A list of components/tools of which at least one must be available
    typedef std::vector<component> comp_vector;
    // A list of comp_vector of which all must be available
    typedef std::vector<comp_vector> alter_comp_vector;
    // List of qualites, each one must be available
    typedef std::vector<quality_requirement> quality_vector;

    alter_comp_vector tools;
    quality_vector qualities;
    alter_comp_vector components;

    /**
     * Time in movement points (100 movement points per turns) required
     * to do the task.
     */
    int time;
    /**
     * Load @ref tools, @ref qualities and @ref components from
     * the json object. Assumes them to be in sub-objects.
     */
    void load(JsonObject &jsobj);
    static void load_obj_list(JsonArray &jsarr, alter_comp_vector &objs);
    static void load_obj_list(JsonArray &jsarr, quality_vector &objs);
    /**
     * @returns whether any of the listed components have been flagged as available.
     */
    static bool any_marked_available(const comp_vector &comps);
    /**
     * Returns a list of components/tools/qualities that are not available,
     * nicely formatted for popup window or similar.
     */
    std::string list_missing() const;
    static std::string print_missing_objs(const alter_comp_vector &objs, bool is_tools);
    static std::string print_missing_objs(const quality_vector &objs);

    /**
     * Consistency checking
     * @param display_name the string is used when displaying a error about
     * inconsistent data (unknown item id, ...).
     */
    void check_consistency(const std::string &display_name) const;

    static void check_objs(const alter_comp_vector &objs, const std::string &display_name);
    static void check_objs(const quality_vector &objs, const std::string &display_name);

    int print_components(WINDOW *w, int ypos, int xpos, int width, nc_color col, const inventory &crafting_inv) const;
    int print_tools(WINDOW *w, int ypos, int xpos, int width, nc_color col, const inventory &crafting_inv) const;
    int print_time(WINDOW *w, int ypos, int xpos, int width, nc_color col) const;
};

#endif
