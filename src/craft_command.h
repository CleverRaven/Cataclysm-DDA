#pragma once
#ifndef CRAFT_COMMAND_H
#define CRAFT_COMMAND_H

#include <list>
#include <vector>

#include "requirements.h"
#include "string_id.h"

class inventory;
class item;
class player;
class recipe;

struct component;
struct tool_comp;
struct item_comp;

struct requirement_data;
using requirement_id = string_id<requirement_data>;

/**
*   enum used by comp_selection to indicate where a component should be consumed from.
*/
enum usage {
    use_from_map = 1,
    use_from_player = 2,
    use_from_both = 1 | 2,
    use_from_none = 4,
    cancel = 8 // FIXME: hacky.
};

/**
*   Struct that represents a selection of a component for crafting.
*/
template<typename CompType = component>
struct comp_selection {
    /** Tells us where the selected component should be used from. */
    usage use_from = use_from_none;
    CompType comp;

    /** provides a translated name for 'comp', suffixed with it's location e.g '(nearby)'. */
    std::string nname() const;
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
        craft_command( const recipe *to_make, int batch_size, bool is_long, player *crafter ) :
            rec( to_make ), batch_size( batch_size ), is_long( is_long ), crafter( crafter ) {}

        /** Selects components to use for the craft, then assigns the crafting activity to 'crafter'. */
        void execute();
        /** Consumes the selected components. Must be called after execute(). */
        std::list<item> consume_components();

        bool has_cached_selections() const {
            return !item_selections.empty() || !tool_selections.empty();
        }

        bool empty() const {
            return rec == nullptr;
        }
    private:
        const recipe *rec = nullptr;
        int batch_size = 0;
        /** Indicates the activity_type for this crafting job, Either ACT_CRAFT or ACT_LONGCRAFT. */
        bool is_long = false;
        // This is mainly here for maintainability reasons.
        player *crafter;

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

#endif
