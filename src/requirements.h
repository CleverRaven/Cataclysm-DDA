#pragma once
#ifndef CATA_SRC_REQUIREMENTS_H
#define CATA_SRC_REQUIREMENTS_H

#include <cstdint>
#include <functional>
#include <map>
#include <numeric>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "crafting.h"
#include "translation.h"
#include "type_id.h"

class Character;
class JsonArray;
class JsonObject;
class JsonOut;
class JsonValue;
class item;
class item_components;
class nc_color;
class read_only_visitable;
template <typename E> struct enum_traits;

enum class available_status : int {
    a_true = +1, // yes, it's available
    a_false = -1, // no, it's not available
    a_insufficient = 0, // nearly, but not enough for tool+component
};

enum class component_type : int {
    ITEM,
    TOOL,
    QUALITY,
};

struct quality {
    bool was_loaded = false;
    quality_id id;
    std::vector<std::pair<quality_id, mod_id>> src;
    translation name;

    std::vector<std::pair<int, std::string>> usages;

    void load( const JsonObject &jo, std::string_view src );

    static void reset();
    static void load_static( const JsonObject &jo, const std::string &src );
};

struct component {
    itype_id type = itype_id::NULL_ID();
    int count = 0;
    // -1 means the player doesn't have the item, 1 means they do,
    // 0 means they have item but not enough for both tool and component
    mutable available_status available = available_status::a_false;
    bool recoverable = true;
    // If true, it's not actually a component but a requirement (list of components)
    bool requirement = false;

    // needs explicit specification due to the mutable member. update this when you add new
    // members!
    bool operator==( const component &rhs ) const {
        return std::forward_as_tuple( type, requirement, count, recoverable )
               == std::forward_as_tuple( rhs.type, rhs.requirement, rhs.count, rhs.recoverable );
    }
    bool operator!=( const component &rhs ) const {
        return !operator==( rhs );
    }
    // lexicographic comparison
    bool operator<( const component &rhs ) const {
        //TODO change to use localized sorting
        return std::forward_as_tuple( type.str(), requirement, count, recoverable )
               < std::forward_as_tuple( rhs.type.str(), rhs.requirement, rhs.count, rhs.recoverable );
    }

    component() = default;
    component( const itype_id &TYPE, int COUNT ) : type( TYPE ), count( COUNT ) { }
    component( const itype_id &TYPE, int COUNT, bool RECOVERABLE ) :
        type( TYPE ), count( COUNT ), recoverable( RECOVERABLE ) { }
    void check_consistency( const std::string &display_name ) const;
};

struct tool_comp : public component {
    tool_comp() = default;
    tool_comp( const itype_id &TYPE, int COUNT ) : component( TYPE, COUNT ) { }

    void load( const JsonValue &value );
    void dump( JsonOut &jsout ) const;
    bool has( const read_only_visitable &crafting_inv,
              const std::function<bool( const item & )> &filter,
              int batch = 1, craft_flags = craft_flags::none,
              const std::function<void( int )> &visitor = std::function<void( int )>() ) const;
    std::string to_string( int batch = 1, int avail = 0 ) const;
    nc_color get_color( bool has_one, const read_only_visitable &crafting_inv,
                        const std::function<bool( const item & )> &filter, int batch = 1 ) const;
    bool by_charges() const;
    component_type get_component_type() const {
        return component_type::TOOL;
    }
};

struct item_comp : public component {
    item_comp() = default;
    item_comp( const itype_id &TYPE, int COUNT ) : component( TYPE, COUNT ) { }

    void load( const JsonValue &value );
    void dump( JsonOut &jsout ) const;
    bool has( const read_only_visitable &crafting_inv,
              const std::function<bool( const item & )> &filter,
              int batch = 1, craft_flags = craft_flags::none,
              const std::function<void( int )> &visitor = std::function<void( int )>() ) const;
    std::string to_string( int batch = 1, int avail = 0 ) const;
    nc_color get_color( bool has_one, const read_only_visitable &crafting_inv,
                        const std::function<bool( const item & )> &filter, int batch = 1 ) const;
    component_type get_component_type() const {
        return component_type::ITEM;
    }
};

struct quality_requirement {
    quality_id type = quality_id( "UNKNOWN" );
    int count = 1;
    int level = 1;
    mutable available_status available = available_status::a_false;
    bool requirement = false; // Currently unused, but here for consistency and templates

    // needs explicit specification due to the mutable member. update this when you add new
    // members!
    bool operator==( const quality_requirement &rhs ) const {
        return std::forward_as_tuple( type, requirement, count, level )
               == std::forward_as_tuple( rhs.type, rhs.requirement, rhs.count, rhs.level );
    }
    bool operator!=( const quality_requirement &rhs ) const {
        return !operator==( rhs );
    }
    // lexicographic comparison
    bool operator<( const quality_requirement &rhs ) const {
        return std::forward_as_tuple( type, requirement, count, level )
               < std::forward_as_tuple( rhs.type, rhs.requirement, rhs.count, rhs.level );
    }

    quality_requirement() = default;
    quality_requirement( const quality_id &TYPE, int COUNT, int LEVEL ) : type( TYPE ), count( COUNT ),
        level( LEVEL ) { }

    void load( const JsonValue &value );
    void dump( JsonOut &jsout ) const;
    bool has( const read_only_visitable &crafting_inv,
              const std::function<bool( const item & )> &filter,
              int = 0, craft_flags = craft_flags::none,
              const std::function<void( int )> &visitor = std::function<void( int )>() ) const;
    std::string to_string( int batch = 1, int avail = 0 ) const;
    std::string to_colored_string() const;
    void check_consistency( const std::string &display_name ) const;
    nc_color get_color( bool has_one, const read_only_visitable &crafting_inv,
                        const std::function<bool( const item & )> &filter, int = 0 ) const;
    component_type get_component_type() const {
        return component_type::QUALITY;
    }
};

enum class requirement_display_flags : int {
    none = 0,
    no_unavailable = 1,
};

template<>
struct enum_traits<requirement_display_flags> {
    static constexpr bool is_flag_enum = true;
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
 * Load from an entry of a json array:
 *   void load(const JsonValue &value);
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
        // TODO: remove once all parts specify installation requirements directly
        friend class vpart_info;

        using alter_tool_comp_vector = std::vector<std::vector<tool_comp> >;
        using alter_quali_req_vector = std::vector<std::vector<quality_requirement> >;
        using alter_item_comp_vector = std::vector<std::vector<item_comp> >;

    private:
        alter_tool_comp_vector tools;
        alter_quali_req_vector qualities;
        alter_item_comp_vector components;

    public:

        requirement_data() = default;
        requirement_data( const alter_tool_comp_vector &tools, const alter_quali_req_vector &qualities,
                          const alter_item_comp_vector &components ) : tools( tools ), qualities( qualities ),
            components( components ) {}

        template <
            typename Container,
            typename = std::enable_if_t <
                std::is_same_v <
                    typename Container::value_type, std::pair<requirement_id, int >> ||
                std::is_same_v <
                    typename Container::value_type, std::pair<const requirement_id, int >>
                >
            >
        explicit requirement_data( const Container &cont ) :
            requirement_data(
                std::accumulate(
                    cont.begin(), cont.end(), requirement_data() ) )
        {}

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

        bool is_blacklisted() const {
            return blacklisted;
        }

        /** Scales tool and component requirements leaving qualities unaffected */
        requirement_data operator*( unsigned scalar ) const;

        /** Combines two sets of requirements */
        requirement_data operator+( const requirement_data &rhs ) const;
        /** Incorporate data from an id and integer.
         * This is helpful when building requirement_data objects from maps
         * loaded from JSON */
        requirement_data operator+( const std::pair<const requirement_id, int> &rhs ) const;
        requirement_data operator+( const std::pair<requirement_id, int> &rhs ) const;

        /**
         * Load @ref tools, @ref qualities and @ref components from
         * the json object. Assumes them to be in sub-objects.
         * @param jsobj Object to load data from
         * @param id provide (or override) unique id for this instance
         */
        static void load_requirement( const JsonObject &jsobj,
                                      const requirement_id &id = requirement_id::NULL_ID(),
                                      bool check_extend = false );

        /**
         * Store requirement data for future lookup
         * @param req Data to save
         * @param id provide (or override) unique id for this instance
         */
        static void save_requirement( const requirement_data &req,
                                      const requirement_id &id = requirement_id::NULL_ID(),
                                      const requirement_data *extend = nullptr );
        static std::vector<requirement_data> get_all();
        /**
         * Serialize custom created requirement objects for fetch activities
         */
        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );
        /** Get all currently loaded requirements */
        static const std::map<requirement_id, requirement_data> &all();

        /** Finalizes requirements, must be called AFTER finalizing items, but before recipes! */
        static void finalize();

        /** Check consistency of all loaded requirements */
        static void check_consistency();

        /** Clear all loaded requirements (invalidating any pointers) */
        static void reset();

        /**
         * Returns a list of components/tools/qualities that are required,
         * nicely formatted for popup window or similar.
         */
        std::string list_all() const;

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

        /**
         * Replace tools or components using a provided replacement map.
         */
        void replace_items( const std::unordered_map<itype_id, itype_id> &replacements );

        const alter_tool_comp_vector &get_tools() const;
        const alter_quali_req_vector &get_qualities() const;
        const alter_item_comp_vector &get_components() const;
        alter_item_comp_vector &get_components();

        /**
         * Returns true if the requirements are fulfilled by the filtered inventory
         * @param filter should be recipe::get_component_filter() if used with a recipe
         * or is_crafting_component otherwise.
         */
        bool can_make_with_inventory( const read_only_visitable &crafting_inv,
                                      const std::function<bool( const item & )> &filter, int batch = 1,
                                      craft_flags = craft_flags::none, bool restrict_volume = true ) const;

        /** @param filter see @ref can_make_with_inventory */
        std::vector<std::string> get_folded_components_list( int width, nc_color col,
                const read_only_visitable &crafting_inv, const std::function<bool( const item & )> &filter,
                int batch = 1, std::string_view hilite = {},
                requirement_display_flags = requirement_display_flags::none ) const;

        std::vector<std::string> get_folded_tools_list( int width, nc_color col,
                const read_only_visitable &crafting_inv, int batch = 1 ) const;

        /**
         * Gets a variant of this recipe with crafting-only tools replaced by their
         * disassembly equivalents.
         */
        requirement_data disassembly_requirements() const;

        /**
         * Returns the item requirements to continue an in progress craft with the passed components.
         * Returned requirement_data is for *all* batches at once.
         */
        static requirement_data continue_requirements( const std::vector<item_comp> &required_comps,
                const item_components &remaining_comps );

        /**
         * Merge similar quality/tool/component lists.
         * This simplifies the requirement but may make the requirement stricter.
         */
        void consolidate();

        /**
         * Compares if two requirement_data are the same, but does not compare the requirement ids.
         * The order inside requirement vectors does not matter.
         */
        bool has_same_requirements_as( const requirement_data &that ) const;

        /**
         * Dump to json in inline requirements format
         */
        void dump( JsonOut &jsout ) const;

        uint64_t make_hash() const;

    private:
        requirement_id id_ = requirement_id::NULL_ID(); // NOLINT(cata-serialize)

        bool blacklisted = false;

        bool check_enough_materials( const read_only_visitable &crafting_inv,
                                     const std::function<bool( const item & )> &filter, int batch = 1,
                                     bool restrict_volume = true ) const;
        bool check_enough_materials( const item_comp &comp, const read_only_visitable &crafting_inv,
                                     const std::function<bool( const item & )> &filter, int batch = 1 ) const;

        template<typename T>
        static void check_consistency( const std::vector< std::vector<T> > &vec,
                                       const std::string &display_name );
        template<typename T>
        static void finalize( std::vector< std::vector<T> > &vec );
        template<typename T>
        static std::string print_all_objs( const std::string &header,
                                           const std::vector< std::vector<T> > &objs );
        template<typename T>
        static std::string print_missing_objs( const std::string &header,
                                               const std::vector< std::vector<T> > &objs );
        template<typename T>
        static bool has_comps(
            const read_only_visitable &crafting_inv, const std::vector< std::vector<T> > &vec,
            const std::function<bool( const item & )> &filter, int batch = 1,
            craft_flags = craft_flags::none );

        template<typename T>
        std::vector<std::string> get_folded_list( int width, const read_only_visitable &crafting_inv,
                const std::function<bool( const item & )> &filter,
                const std::vector< std::vector<T> > &objs, int batch = 1,
                std::string_view hilite = {},
                requirement_display_flags = requirement_display_flags::none ) const;

        template<typename T>
        static bool any_marked_available( const std::vector<T> &comps );
        template<typename T>
        static void load_obj_list( const JsonArray &jsarr, std::vector< std::vector<T> > &objs );
        template<typename T, typename ID>
        static const T *find_by_type( const std::vector< std::vector<T> > &vec, const ID &type );
};

// Sometimes the requirement_data is problematic, because it has overlapping
// requirements.  For example, a survivor telescope requires
// 1 high-quality lens
// 1 high-quality lens OR 1 small high-quality lens
// If there is just one high-quality lens in the available inventory then it's
// hard to correctly detect that these requirements are impossible to satisfy.
// In general, determining craftability is equivalent to boolean
// satisfiability, and thus NP-hard.
//
// In practice, we don't expect recipes to have too much overlap, so this issue
// should be tractable.
//
// However, to avoid keeping additional state during the process of searching
// for components, we don't make the component search more complex, instead we
// make the requirements more complex.  We replace each requirement_data with a
// deduped_requirement_data, which contains a selection of alternative
// requirement_data objects, each of which contains no overlapping
// requirements.
//
// For the majority of recipes, there are no overlaps, and this will be
// essentially equivalent to just a requirement_data.  However, for the few
// problematic recipes this allows us to calculate crafting requirements more
// accurately.
class deduped_requirement_data
{
    public:
        using alter_item_comp_vector = requirement_data::alter_item_comp_vector;

        deduped_requirement_data() = default;
        deduped_requirement_data( const requirement_data &, const recipe_id &context );

        std::vector<requirement_data> const &alternatives() const {
            return alternatives_;
        }

        std::vector<const requirement_data *> feasible_alternatives(
            const read_only_visitable &crafting_inv, const std::function<bool( const item & )> &filter,
            int batch = 1, craft_flags = craft_flags::none ) const;

        const requirement_data *select_alternative(
            Character &, const std::function<bool( const item & )> &filter, int batch = 1,
            craft_flags = craft_flags::none ) const;

        const requirement_data *select_alternative(
            Character &, const read_only_visitable &, const std::function<bool( const item & )> &filter,
            int batch = 1, craft_flags = craft_flags::none ) const;

        bool can_make_with_inventory(
            const read_only_visitable &crafting_inv, const std::function<bool( const item & )> &filter,
            int batch = 1, craft_flags = craft_flags::none ) const;

        bool is_too_complex() const {
            return is_too_complex_;
        }
    private:
        bool is_too_complex_ = false;
        std::vector<requirement_data> alternatives_;
};

#endif // CATA_SRC_REQUIREMENTS_H
