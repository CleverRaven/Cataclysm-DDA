#pragma once
#ifndef REQUIREMENTS_H
#define REQUIREMENTS_H

#include <functional>
#include <map>
#include <vector>

#include "string_id.h"

class nc_color;
class JsonObject;
class JsonArray;
class inventory;

struct requirement_data;
using requirement_id = string_id<requirement_data>;

// Denotes the id of an item type
typedef std::string itype_id;
struct quality;
using quality_id = string_id<quality>;

enum available_status {
    a_true = +1, // yes, it's available
    a_false = -1, // no, it's not available
    a_insufficent = 0, // neraly, bt not enough for tool+component
};

struct quality {
    bool was_loaded = false;
    quality_id id;
    // Translated name
    std::string name;

    std::vector<std::pair<int, std::string>> usages;

    void load( JsonObject &jo, const std::string &src );

    static void reset();
    static void load_static( JsonObject &jo, const std::string &src );
};

struct component {
    itype_id type = "null";
    int count = 0;
    // -1 means the player doesn't have the item, 1 means they do,
    // 0 means they have item but not enough for both tool and component
    mutable available_status available = a_false;
    bool recoverable = true;
    // If true, it's not actually a component but a requirement (list of components)
    bool requirement = false;

    component() { }
    component( const itype_id &TYPE, int COUNT ) : type( TYPE ), count( COUNT ) { }
    component( const itype_id &TYPE, int COUNT, bool RECOVERABLE ) :
        type( TYPE ), count( COUNT ), recoverable( RECOVERABLE ) { }
    void check_consistency( const std::string &display_name ) const;
    int operator==( const component &rhs ) const {
        return type == rhs.type && count == rhs.count;
    }
    int operator!=( const component &rhs ) const {
        return !( *this == rhs );
    }
};

struct tool_comp : public component {
    tool_comp() : component() { }
    tool_comp( const itype_id &TYPE, int COUNT ) : component( TYPE, COUNT ) { }

    void load( JsonArray &ja );
    bool has( const inventory &crafting_inv, int batch = 1,
              std::function<void( int )> visitor = std::function<void( int )>() ) const;
    std::string to_string( int batch = 1 ) const;
    nc_color get_color( bool has_one, const inventory &crafting_inv, int batch = 1 ) const;
    bool by_charges() const;
};

struct item_comp : public component {
    item_comp() : component() { }
    item_comp( const itype_id &TYPE, int COUNT ) : component( TYPE, COUNT ) { }

    void load( JsonArray &ja );
    bool has( const inventory &crafting_inv, int batch = 1,
              std::function<void( int )> visitor = std::function<void( int )>() ) const;
    std::string to_string( int batch = 1 ) const;
    nc_color get_color( bool has_one, const inventory &crafting_inv, int batch = 1 ) const;
};

struct quality_requirement {
    quality_id type = quality_id( "UNKNOWN" );
    int count = 1;
    int level = 1;
    mutable available_status available = a_false;
    bool requirement = false; // Currently unused, but here for consistency and templates

    quality_requirement() = default;
    quality_requirement( const quality_id &TYPE, int COUNT, int LEVEL ) : type( TYPE ), count( COUNT ),
        level( LEVEL ) { }

    void load( JsonArray &ja );
    bool has( const inventory &crafting_inv, int = 0,
              std::function<void( int )> visitor = std::function<void( int )>() ) const;
    std::string to_string( int = 0 ) const;
    void check_consistency( const std::string &display_name ) const;
    nc_color get_color( bool has_one, const inventory &crafting_inv, int = 0 ) const;
};

/**
 * The *_vector members represent list of alternatives requirements:
 * alter_tool_comp_vector = { * { { a, b }, { c, d } }
 * That means: the player needs (requirement a or b) and (requirement c or d).
 *
 * Some functions in this struct use template arguments so they can be
 * applied to all three *_vector members. For example:
 * check_consistency iterates over all entries in the supplied vector and calls
 * check_consistency on each entry.
 * If called as <code>check_consistency(tools)</code> this will actually call
 * tool_comp::check_consistency. If called as
 * <code>check_consistency(qualities)</code> it will call
 * quality_requirement:check_consistency.
 *
 * Requirements (item_comp, tool_comp, quality_requirement) must have those
 * functions:
 * Load from the next entry of the json array:
 *   void load(JsonArray &jarr);
 * Check whether the player has fulfills the requirement with this crafting
 * inventory (or by mutation):
 *   bool has(const inventory &crafting_inv) const;
 * A textual representation of the requirement:
 *   std::string to_string() const;
 * Consistency check:
 *   void check_consistency(const std::string &display_name) const;
 * Color to be used for displaying the requirement. has_one is true of the
 * player fulfills an alternative requirement:
 *   nc_color get_color(bool has_one, const inventory &crafting_inv) const;
*/
struct requirement_data {
        // temporarily break encapsulation pending migration of legacy parts
        // @see vpart_info::check
        // @todo: remove once all parts specify installation requirements directly
        friend class vpart_info;

        typedef std::vector< std::vector<tool_comp> > alter_tool_comp_vector;
        typedef std::vector< std::vector<item_comp> > alter_item_comp_vector;
        typedef std::vector< std::vector<quality_requirement> > alter_quali_req_vector;

    private:
        alter_tool_comp_vector tools;
        alter_quali_req_vector qualities;
        alter_item_comp_vector components;

    public:
        const requirement_id &id() const {
            return id_;
        }

        /** null requirements are always empty (were never initialized) */
        bool is_null() const {
            return id_.is_null();
        }

        /** empty requirements are not necessary null */
        bool is_empty() const {
            return tools.empty() && components.empty() && qualities.empty();
        }

        /** check if removal of items via @ref blacklist_item left no alternatives in group */
        bool is_blacklisted() const {
            return blacklisted;
        }

        /** Scales tool and component requirements leaving qualities unaffected */
        requirement_data operator*( unsigned scalar ) const;

        /** Combines two sets of requirements */
        requirement_data operator+( const requirement_data &rhs ) const;

        /**
         * Load @ref tools, @ref qualities and @ref components from
         * the json object. Assumes them to be in sub-objects.
         * @param jsobj Object to load data from
         * @param id provide (or override) unique id for this instance
         */
        static void load_requirement( JsonObject &jsobj,
                                      const requirement_id &id = requirement_id::NULL_ID() );

        /**
         * Store requirement data for future lookup
         * @param req Data to save
         * @param id provide (or override) unique id for this instance
         */
        static void save_requirement( const requirement_data &req,
                                      const requirement_id &id = requirement_id::NULL_ID() );

        /** Get all currently loaded requirements */
        static const std::map<requirement_id, requirement_data> &all();

        /** Finalizes requirements, must be called AFTER finalizing items, but before recipes! */
        static void finalize();

        /** Check consistency of all loaded requirements */
        static void check_consistency();

        /** Clear all loaded requirements (invalidating any pointers) */
        static void reset();

        /**
         * Returns a list of components/tools/qualities that are not available,
         * nicely formatted for popup window or similar.
         */
        std::string list_missing() const;

        /**
         * Remove tools or components of given type leaving qualities unchanged
         * @note if the last available component of a grouping is removed the recipe
         * will be marked as @ref blacklisted
         */
        void blacklist_item( const itype_id &id );

        const alter_tool_comp_vector &get_tools() const;
        const alter_quali_req_vector &get_qualities() const;
        const alter_item_comp_vector &get_components() const;
        alter_item_comp_vector &get_components();

        bool can_make_with_inventory( const inventory &crafting_inv, int batch = 1 ) const;

        std::vector<std::string> get_folded_components_list( int width, nc_color col,
                const inventory &crafting_inv, int batch = 1, std::string hilite = "" ) const;

        std::vector<std::string> get_folded_tools_list( int width, nc_color col,
                const inventory &crafting_inv, int batch = 1 ) const;

        /**
         * Gets a variant of this recipe with crafting-only tools replaced by their
         * disassembly equivalents.
         */
        requirement_data disassembly_requirements() const;

    private:
        requirement_id id_ = requirement_id::NULL_ID();

        bool blacklisted = false;

        bool check_enough_materials( const inventory &crafting_inv, int batch = 1 ) const;
        bool check_enough_materials( const item_comp &comp, const inventory &crafting_inv,
                                     int batch = 1 ) const;

        template<typename T>
        static void check_consistency( const std::vector< std::vector<T> > &vec,
                                       const std::string &display_name );
        template<typename T>
        static void finalize( std::vector< std::vector<T> > &vec );
        template<typename T>
        static std::string print_missing_objs( const std::string &header,
                                               const std::vector< std::vector<T> > &objs );
        template<typename T>
        static bool has_comps( const inventory &crafting_inv, const std::vector< std::vector<T> > &vec,
                               int batch = 1 );

        template<typename T>
        std::vector<std::string> get_folded_list( int width, const inventory &crafting_inv,
                const std::vector< std::vector<T> > &objs, int batch = 1, std::string hilite = "" ) const;

        template<typename T>
        static bool any_marked_available( const std::vector<T> &comps );
        template<typename T>
        static void load_obj_list( JsonArray &jsarr, std::vector< std::vector<T> > &objs );
        template<typename T, typename ID>
        static const T *find_by_type( const std::vector< std::vector<T> > &vec, const ID &type );
};

#endif
