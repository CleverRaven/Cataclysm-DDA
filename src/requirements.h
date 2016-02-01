#ifndef REQUIREMENTS_H
#define REQUIREMENTS_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "color.h"
#include "output.h"

class JsonObject;
class JsonArray;
class inventory;

// Denotes the id of an item type
typedef std::string itype_id;
// Denotes the id of an item quality
typedef std::string quality_id;

enum available_status {
    a_true = +1, // yes, it's available
    a_false = -1, // no, it's not available
    a_insufficent = 0, // neraly, bt not enough for tool+component
};

struct quality {
    quality_id id;
    // Translated name
    std::string name;

    typedef std::map<quality_id, quality> quality_map;
    static quality_map qualities;
    static void reset();
    static void load( JsonObject &jo );
    static std::string get_name( const quality_id &id );
    static bool has( const quality_id &id );
};

struct component {
    itype_id type;
    int count;
    // -1 means the player doesn't have the item, 1 means they do,
    // 0 means they have item but not enough for both tool and component
    mutable available_status available;
    bool recoverable;

    component() : type( "null" ) , count( 0 ) , available( a_false ), recoverable( true ) {
    }
    component( const itype_id &TYPE, int COUNT ) : type( TYPE ), count( COUNT ), available( a_false ),
        recoverable( true ) {
    }
    component( const itype_id &TYPE, int COUNT, bool RECOVERABLE ) : type( TYPE ), count( COUNT ),
        available( a_false ), recoverable( RECOVERABLE ) {
    }
    void check_consistency( const std::string &display_name ) const;
};

struct tool_comp : public component {
    tool_comp() : component() { }
    tool_comp( const itype_id &TYPE, int COUNT ) : component( TYPE, COUNT ) { }

    void load( JsonArray &jarr );
    bool has( const inventory &crafting_inv, int batch = 1 ) const;
    std::string to_string( int batch = 1 ) const;
    std::string get_color( bool has_one, const inventory &crafting_inv, int batch = 1 ) const;
    bool by_charges() const;
};

struct item_comp : public component {
    item_comp() : component() { }
    item_comp( const itype_id &TYPE, int COUNT ) : component( TYPE, COUNT ) { }

    void load( JsonArray &jarr );
    bool has( const inventory &crafting_inv, int batch = 1 ) const;
    std::string to_string( int batch = 1 ) const;
    std::string get_color( bool has_one, const inventory &crafting_inv, int batch = 1 ) const;
};

struct quality_requirement {
    quality_id type;
    int count;
    int level;
    mutable available_status available;

    quality_requirement() : type( "UNKNOWN" ), count( 0 ), level( 0 ), available( a_false ) {
    }
    quality_requirement( const quality_id &TYPE, int COUNT, int LEVEL ) : type( TYPE ), count( COUNT ),
        level( LEVEL ), available( a_false ) {
    }

    void load( JsonArray &jarr );
    bool has( const inventory &crafting_inv, int = 0 ) const;
    std::string to_string( int = 0 ) const;
    void check_consistency( const std::string &display_name ) const;
    std::string get_color( bool has_one, const inventory &crafting_inv, int = 0 ) const;
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
 *   std::string get_color(bool has_one, const inventory &crafting_inv) const;
*/
struct requirement_data {
        typedef std::vector< std::vector<tool_comp> > alter_tool_comp_vector;
        typedef std::vector< std::vector<item_comp> > alter_item_comp_vector;
        typedef std::vector< std::vector<quality_requirement> > alter_quali_req_vector;

    private:
        alter_tool_comp_vector tools;
        alter_quali_req_vector qualities;
        alter_item_comp_vector components;

    public:

        /**
         * Load @ref tools, @ref qualities and @ref components from
         * the json object. Assumes them to be in sub-objects.
         */
        void load( JsonObject &jsobj );
        /**
         * Returns a list of components/tools/qualities that are not available,
         * nicely formatted for popup window or similar.
         */
        std::string list_missing() const;
        /**
         * Consistency checking
         * @param display_name the string is used when displaying a error about
         * inconsistent data (unknown item id, ...).
         */
        void check_consistency( const std::string &display_name ) const;
        /**
         * Remove components (tools/items) of the given item type. Qualities are not
         * changed.
         * @returns true if any the last requirement in a list of alternatives has
         * been removed. This requirement can never be fulfilled and should be discarded.
         */
        bool remove_item( const std::string &type );

        const alter_tool_comp_vector &get_tools() const;
        const alter_quali_req_vector &get_qualities() const;
        const alter_item_comp_vector &get_components() const;

        bool can_make_with_inventory( const inventory &crafting_inv, int batch = 1 ) const;

        int print_components( WINDOW *w, int ypos, int xpos, int width, nc_color col,
                              const inventory &crafting_inv, int batch = 1 ) const;
        std::vector<std::string> get_folded_components_list( int width, nc_color col,
                const inventory &crafting_inv, int batch = 1 ) const;
        int print_tools( WINDOW *w, int ypos, int xpos, int width, nc_color col,
                         const inventory &crafting_inv, int batch = 1 ) const;
        std::vector<std::string> get_folded_tools_list( int width, nc_color col,
                const inventory &crafting_inv, int batch = 1 ) const;

        /**
         * Gets a variant of this recipe with crafting-only tools replaced by their
         * disassembly equivalents.
         */
        const requirement_data disassembly_requirements() const;

    private:
        bool check_enough_materials( const inventory &crafting_inv, int batch = 1 ) const;
        bool check_enough_materials( const item_comp &comp, const inventory &crafting_inv,
                                     int batch = 1 ) const;

        template<typename T>
        static void check_consistency( const std::vector< std::vector<T> > &vec,
                                       const std::string &display_name );
        template<typename T>
        static std::string print_missing_objs( const std::string &header,
                                               const std::vector< std::vector<T> > &objs );
        template<typename T>
        static bool has_comps( const inventory &crafting_inv, const std::vector< std::vector<T> > &vec,
                               int batch = 1 );
        template<typename T>
        static int print_list( WINDOW *w, int ypos, int xpos, int width, nc_color col,
                               const inventory &crafting_inv, const std::vector< std::vector<T> > &objs, int batch = 1 );
        template<typename T>
        static std::vector<std::string> get_folded_list( int width, const inventory &crafting_inv,
                const std::vector< std::vector<T> > &objs, int batch = 1 );
        template<typename T>
        static bool remove_item( const std::string &type, std::vector< std::vector<T> > &vec );
        template<typename T>
        static bool any_marked_available( const std::vector<T> &comps );
        template<typename T>
        static void load_obj_list( JsonArray &jsarr, std::vector< std::vector<T> > &objs );
        template<typename T>
        static const T *find_by_type( const std::vector< std::vector<T> > &vec, const std::string &type );
};

#endif
