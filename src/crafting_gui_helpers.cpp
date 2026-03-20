#include "crafting_gui_helpers.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include "calendar.h"
#include "character.h"
#include "crafting.h"
#include "inventory.h"
#include "localized_comparator.h"
#include "recipe.h"
#include "requirements.h"
#include "type_id.h"

bool cannot_gain_skill_or_prof( const Character &crafter, const recipe &recp )
{
    if( recp.skill_used &&
        static_cast<int>( crafter.get_skill_level( recp.skill_used ) ) <= recp.get_skill_cap() ) {
        return false;
    }
    for( const proficiency_id &prof : recp.used_proficiencies() ) {
        if( !crafter.has_proficiency( prof ) ) {
            return false;
        }
    }
    return true;
}

float availability::get_proficiency_time_maluses() const
{
    if( proficiency_time_maluses < 0 ) {
        proficiency_time_maluses = rec->proficiency_time_maluses( crafter );
    }

    return proficiency_time_maluses;
}

float availability::get_max_proficiency_time_maluses() const
{
    if( max_proficiency_time_maluses < 0 ) {
        max_proficiency_time_maluses = rec->max_proficiency_time_maluses( crafter );
    }

    return max_proficiency_time_maluses;
}

float availability::get_proficiency_skill_maluses() const
{
    if( proficiency_skill_maluses < 0 ) {
        proficiency_skill_maluses = rec->proficiency_skill_maluses( crafter );
    }

    return proficiency_skill_maluses;
}

float availability::get_max_proficiency_skill_maluses() const
{
    if( max_proficiency_skill_maluses < 0 ) {
        max_proficiency_skill_maluses = rec->max_proficiency_skill_maluses( crafter );
    }

    return max_proficiency_skill_maluses;
}

availability::availability( Character &_crafter, const recipe *r, int batch_size,
                            bool camp_crafting, inventory *inventory_override ) :
    crafter( _crafter )
{
    rec = r;
    inv_override = inventory_override;
    const inventory &inv = camp_crafting ? *inv_override : crafter.crafting_inventory();
    auto all_items_filter = r->get_component_filter( recipe_filter_flags::none );
    auto no_rotten_filter = r->get_component_filter( recipe_filter_flags::no_rotten );
    auto no_favorite_filter = r->get_component_filter( recipe_filter_flags::no_favorite );
    const deduped_requirement_data &req = r->deduped_requirements();
    has_all_skills = r->skill_used.is_null() ||
                     crafter.get_skill_level( r->skill_used ) >= r->get_difficulty( crafter );
    crafter_has_primary_skill = r->skill_used.is_null()
                                || crafter.get_knowledge_level( rec->skill_used )
                                >= static_cast<int>( rec->get_difficulty( crafter ) * 0.8f );
    has_proficiencies = r->character_has_required_proficiencies( crafter );
    std::string reason;
    craft_flags flag = camp_crafting ? craft_flags::none : craft_flags::start_only;

    if( crafter.is_npc() && !r->npc_can_craft( reason ) && !camp_crafting ) {
        can_craft = false;
    } else if( r->is_nested() ) {
        can_craft = check_can_craft_nested( _crafter, *r );
    } else {
        can_craft = ( !r->is_practice() || has_all_skills ) && has_proficiencies &&
                    req.can_make_with_inventory( inv, all_items_filter, batch_size, flag );
    }
    would_use_rotten = !req.can_make_with_inventory( inv, no_rotten_filter, batch_size,
                       flag );
    would_use_favorite = !req.can_make_with_inventory( inv, no_favorite_filter, batch_size,
                         flag );
    useless_practice = r->is_practice() && cannot_gain_skill_or_prof( crafter, *r );
    is_nested_category = r->is_nested();
    const requirement_data &simple_req = r->simple_requirements();
    apparently_craftable = ( !r->is_practice() || has_all_skills ) && has_proficiencies &&
                           simple_req.can_make_with_inventory( inv, all_items_filter, batch_size, flag );
    for( const auto &[skill, skill_lvl] : r->required_skills ) {
        if( crafter.get_skill_level( skill ) < skill_lvl ) {
            has_all_skills = false;
            break;
        }
    }
}

nc_color availability::selected_color() const
{
    if( !can_craft && is_nested_category ) {
        return h_blue;
    } else if( !can_craft ) {
        return h_dark_gray;
    } else if( !crafter_has_primary_skill && is_nested_category ) {
        return h_magenta;
    } else if( !crafter_has_primary_skill ) {
        return h_light_red;
    } else if( is_nested_category ) {
        return h_light_blue;
    } else if( would_use_rotten || useless_practice ) {
        return has_all_skills ? h_brown : h_red;
    } else if( would_use_favorite ) {
        return has_all_skills ? h_pink : h_red;
    } else {
        return has_all_skills ? h_white : h_yellow;
    }
}

nc_color availability::color( bool ignore_missing_skills ) const
{
    if( !can_craft && is_nested_category ) {
        return c_blue;
    } else if( !can_craft ) {
        return c_dark_gray;
    } else if( !crafter_has_primary_skill && is_nested_category ) {
        return c_magenta;
    } else if( !crafter_has_primary_skill ) {
        return c_light_red;
    } else if( is_nested_category ) {
        return c_light_blue;
    } else if( would_use_rotten || useless_practice ) {
        return has_all_skills || ignore_missing_skills ? c_brown : c_red;
    } else if( would_use_favorite ) {
        return has_all_skills ? c_pink : c_red;
    } else {
        return has_all_skills || ignore_missing_skills ? c_white : c_yellow;
    }
}

bool availability::check_can_craft_nested( Character &_crafter, const recipe &r )
{
    for( const recipe_id &nested_r : r.nested_category_data ) {
        if( availability( _crafter, &nested_r.obj() ).can_craft ) {
            return true;
        }
    }
    return false;
}

craft_confirm_result can_start_craft(
    const recipe &rec,
    const availability &avail,
    const Character &crafter )
{
    if( !avail.can_craft || !avail.crafter_has_primary_skill ) {
        return craft_confirm_result::cannot_craft;
    }
    if( crafter.lighting_craft_speed_multiplier( rec ) <= 0.0f ) {
        return craft_confirm_result::too_dark;
    }
    return craft_confirm_result::ok;
}

bool recipe_sort_compare(
    const recipe *a, const recipe *b,
    const availability &avail_a, const availability &avail_b,
    const Character &crafter,
    bool a_read, bool b_read,
    bool unread_first )
{
    if( unread_first ) {
        if( a_read != b_read ) {
            return !a_read;
        }
    }
    if( avail_a.can_craft != avail_b.can_craft ) {
        return avail_a.can_craft;
    }
    if( b->difficulty != a->difficulty ) {
        return b->difficulty < a->difficulty;
    }
    const std::string a_name = a->result_name();
    const std::string b_name = b->result_name();
    if( a_name != b_name ) {
        return localized_compare( a_name, b_name );
    }
    return b->time_to_craft( crafter ) < a->time_to_craft( crafter );
}
