#pragma once
#ifndef CATA_SRC_CRAFT_COMMAND_H
#define CATA_SRC_CRAFT_COMMAND_H

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "coordinates.h"
#include "recipe.h"
#include "requirements.h"
#include "type_id.h"

class Character;
class JsonObject;
class JsonOut;
class item;
class read_only_visitable;
class inventory;
template<typename T> struct enum_traits;

/**
*   enum used by comp_selection to indicate where a component should be consumed from.
*/
enum class usage_from : int {
    none = 0,
    map = 1,
    player = 2,
    both = 1 | 2,
    cancel = 4, // FIXME: hacky.
    num_usages_from = 5
};

template<>
struct enum_traits<usage_from> {
    static constexpr usage_from last = usage_from::num_usages_from;
    static constexpr bool is_flag_enum = true;
};

/**
*   Struct that represents a selection of a component for crafting.
*/
template<typename CompType>
struct comp_selection {
    /** Tells us where the selected component should be used from. */
    usage_from use_from = usage_from::none;
    CompType comp;

    /** provides a translated name for 'comp', suffixed with it's location e.g '(nearby)'. */
    std::string nname() const;

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonObject &data );
};


struct craft_selection {
    /** Tells us where the selected component should be used from. */
    item_comp comp;
    const recipe *rec;

    bool cancled = false;
    /** provides a translated name for 'comp', suffixed with it's location e.g '(nearby)'. */
    std::string nname() const;

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonObject &data );
};




struct craft_step_data {
    craft_step_data() = default;
    craft_step_data( const recipe *rec, int batch, const requirement_data *req ): rec( rec ),
        batch( batch ), req( req ) {};
    const recipe *rec = nullptr;
    int batch = 0;
    const requirement_data *req = nullptr;
};

/**
*   Class that describes a crafting job.
*
*   The class has functions to execute the crafting job.
*/
class craft_command
{
    public:
        /** Instantiates an empty craft_command, which can't be executed. */
        craft_command() = default;
        craft_command( const recipe *to_make, int batch_size, bool is_long, Character *crafter,
                       const std::optional<tripoint_bub_ms> &loc ) :
            to_make{to_make, batch_size, &( to_make->simple_requirements() ) }, longcraft( is_long ),
            crafter( crafter ), loc( loc ) {

        }

        /**
         * Selects components to use for the craft, then assigns the crafting activity to 'crafter'.
         * Executes with supplied location, std::nullopt means crafting from inventory.
         */
        void execute( const std::optional<tripoint_bub_ms> &new_loc );
        /** Executes with saved location, NOT the same as execute( std::nullopt )! */
        void execute( bool only_cache_comps = false );

        /**
         * Consumes the selected components and returns the resulting in progress craft item.
         * Must be called after execute().
         */
        item create_in_progress_craft();

        bool is_long() const {
            return longcraft || craft_queue.size() > 1;
        }

        bool has_cached_selections() const {
            return !item_selections.empty() || !tool_selections.empty();
        }

        bool empty() const {
            return to_make.rec == nullptr;
        }
        skill_id get_skill_id();

        bool continue_prompt_liquids( const std::function<bool( const item & )> &filter,
                                      bool no_prompt = false );
        static bool safe_to_unload_comp( const item &it );

    private:
        craft_step_data to_make;
        /**
        * Indicates whether the player has initiated a one off craft or wishes to craft as
        * long as possible.
        */
        bool longcraft = false;
        // This is mainly here for maintainability reasons.
        Character *crafter;

        recipe_filter_flags flags = recipe_filter_flags::none;

        // Location of the workbench to place the item on
        // zero_tripoint indicates crafting without a workbench
        std::optional<tripoint_bub_ms> loc;

        craft_step_data current_rec() const {
            return craft_queue.empty() ? to_make : craft_queue.back();
        };


        std::vector<craft_step_data> craft_queue;
        std::vector<comp_selection<item_comp>> item_selections;
        std::vector<comp_selection<tool_comp>> tool_selections;

        /** Checks if tools we selected in a previous call to execute() are still available. */
        std::vector<comp_selection<item_comp>> check_item_components_missing(
                                                const read_only_visitable &map_inv ) const;
        /** Checks if items we selected in a previous call to execute() are still available. */
        std::vector<comp_selection<tool_comp>> check_tool_components_missing(
                                                const read_only_visitable &map_inv ) const;

        /** Creates a continue pop up asking to continue crafting and listing the missing components */
        bool query_continue( const std::vector<comp_selection<item_comp>> &missing_items,
                             const std::vector<comp_selection<tool_comp>> &missing_tools );
};

#endif // CATA_SRC_CRAFT_COMMAND_H
