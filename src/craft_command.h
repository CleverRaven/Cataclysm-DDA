#pragma once
#ifndef CATA_SRC_CRAFT_COMMAND_H
#define CATA_SRC_CRAFT_COMMAND_H

#include <string>
#include <vector>

#include "point.h"
#include "recipe.h"
#include "requirements.h"
#include "type_id.h"

class JsonIn;
class JsonOut;
class inventory;
class item;
class player;
template<typename T> struct enum_traits;

/**
*   enum used by comp_selection to indicate where a component should be consumed from.
*/
enum usage {
    use_from_none = 0,
    use_from_map = 1,
    use_from_player = 2,
    use_from_both = 1 | 2,
    cancel = 4, // FIXME: hacky.
    num_usages
};

template<>
struct enum_traits<usage> {
    static constexpr usage last = usage::num_usages;
};

/**
*   Struct that represents a selection of a component for crafting.
*/
template<typename CompType>
struct comp_selection {
    /** Tells us where the selected component should be used from. */
    usage use_from = use_from_none;
    CompType comp;

    /** provides a translated name for 'comp', suffixed with it's location e.g '(nearby)'. */
    std::string nname() const;

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
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
        craft_command( const recipe *to_make, int batch_size, bool is_long, player *crafter,
                       const tripoint &loc = tripoint_zero ) :
            rec( to_make ), batch_size( batch_size ), longcraft( is_long ), crafter( crafter ), loc( loc ) {}

        /** Selects components to use for the craft, then assigns the crafting activity to 'crafter'. */
        void execute( const tripoint &new_loc = tripoint_zero );

        /**
         * Consumes the selected components and returns the resulting in progress craft item.
         * Must be called after execute().
         */
        item create_in_progress_craft();

        bool is_long() const {
            return longcraft;
        }

        bool has_cached_selections() const {
            return !item_selections.empty() || !tool_selections.empty();
        }

        bool empty() const {
            return rec == nullptr;
        }
        skill_id get_skill_id();

    private:
        const recipe *rec = nullptr;
        int batch_size = 0;
        /**
        * Indicates whether the player has initiated a one off craft or wishes to craft as
        * long as possible.
        */
        bool longcraft = false;
        // This is mainly here for maintainability reasons.
        player *crafter;

        recipe_filter_flags flags = recipe_filter_flags::none;

        // Location of the workbench to place the item on
        // zero_tripoint indicates crafting without a workbench
        tripoint loc = tripoint_zero;

        std::vector<comp_selection<item_comp>> item_selections;
        std::vector<comp_selection<tool_comp>> tool_selections;

        /** Checks if tools we selected in a previous call to execute() are still available. */
        std::vector<comp_selection<item_comp>> check_item_components_missing(
                                                const inventory &map_inv ) const;
        /** Checks if items we selected in a previous call to execute() are still available. */
        std::vector<comp_selection<tool_comp>> check_tool_components_missing(
                                                const inventory &map_inv ) const;

        /** Creates a continue pop up asking to continue crafting and listing the missing components */
        bool query_continue( const std::vector<comp_selection<item_comp>> &missing_items,
                             const std::vector<comp_selection<tool_comp>> &missing_tools );
};

#endif // CATA_SRC_CRAFT_COMMAND_H
