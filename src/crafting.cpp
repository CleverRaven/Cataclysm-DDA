#include "crafting.h"

#include <climits>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "activity_handlers.h"
#include "bionics.h"
#include "calendar.h"
#include "craft_command.h"
#include "debug.h"
#include "game.h"
#include "game_inventory.h"
#include "handle_liquid.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "rng.h"
#include "translations.h"
#include "ui.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"
#include "veh_type.h"
#include "cata_utility.h"
#include "color.h"
#include "enums.h"
#include "game_constants.h"
#include "item_stack.h"
#include "line.h"
#include "map_selector.h"
#include "mapdata.h"
#include "optional.h"
#include "pimpl.h"
#include "player.h"
#include "player_activity.h"
#include "recipe.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "string_id.h"
#include "units.h"
#include "type_id.h"
#include "clzones.h"
#include "colony.h"
#include "faction.h"
#include "flat_set.h"
#include "iuse.h"
#include "point.h"
#include "weather.h"

class basecamp;

const efftype_id effect_contacts( "contacts" );

void drop_or_handle( const item &newit, player &p );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );
static const trait_id trait_PAWS_LARGE( "PAWS_LARGE" );
static const trait_id trait_PAWS( "PAWS" );
static const trait_id trait_BURROW( "BURROW" );

static bool crafting_allowed( const player &p, const recipe &rec )
{
    if( p.morale_crafting_speed_multiplier( rec ) <= 0.0f ) {
        add_msg( m_info, _( "Your morale is too low to craft such a difficult thing..." ) );
        return false;
    }

    if( p.lighting_craft_speed_multiplier( rec ) <= 0.0f ) {
        add_msg( m_info, _( "You can't see to craft!" ) );
        return false;
    }

    if( rec.category == "CC_BUILDING" ) {
        add_msg( m_info, _( "Overmap terrain building recipes are not implemented yet!" ) );
        return false;
    }
    return true;
}

float player::lighting_craft_speed_multiplier( const recipe &rec ) const
{
    // negative is bright, 0 is just bright enough, positive is dark, +7.0f is pitch black
    float darkness = fine_detail_vision_mod() - 4.0f;
    if( darkness <= 0.0f ) {
        return 1.0f; // it's bright, go for it
    }
    bool rec_blind = rec.has_flag( "BLIND_HARD" ) || rec.has_flag( "BLIND_EASY" );
    if( darkness > 0 && !rec_blind ) {
        return 0.0f; // it's dark and this recipe can't be crafted in the dark
    }
    if( rec.has_flag( "BLIND_EASY" ) ) {
        // 100% speed in well lit area at skill+0
        // 25% speed in pitch black at skill+0
        // skill+2 removes speed penalty
        return 1.0f - ( darkness / ( 7.0f / 0.75f ) ) * std::max( 0,
                2 - exceeds_recipe_requirements( rec ) ) / 2.0f;
    }
    if( rec.has_flag( "BLIND_HARD" ) && exceeds_recipe_requirements( rec ) >= 2 ) {
        // 100% speed in well lit area at skill+2
        // 25% speed in pitch black at skill+2
        // skill+8 removes speed penalty
        return 1.0f - ( darkness / ( 7.0f / 0.75f ) ) * std::max( 0,
                8 - exceeds_recipe_requirements( rec ) ) / 6.0f;
    }
    return 0.0f; // it's dark and you could craft this if you had more skill
}

float player::morale_crafting_speed_multiplier( const recipe &rec ) const
{
    int morale = get_morale_level();
    if( morale >= 0 ) {
        // No bonus for being happy yet
        return 1.0f;
    }

    // Harder jobs are more frustrating, even when skilled
    // For each skill where skill=difficulty, multiply effective morale by 200%
    float morale_mult = std::max( 1.0f, 2.0f * rec.difficulty / std::max( 1,
                                  get_skill_level( rec.skill_used ) ) );
    for( const auto &pr : rec.required_skills ) {
        morale_mult *= std::max( 1.0f, 2.0f * pr.second / std::max( 1, get_skill_level( pr.first ) ) );
    }

    // Halve speed at -50 effective morale, quarter at -150
    float morale_effect = 1.0f + ( morale_mult * morale ) / -50.0f;

    return 1.0f / morale_effect;
}

template<typename T>
static float lerped_multiplier( const T &value, const T &low, const T &high )
{
    // No effect if less than allowed value
    if( value < low ) {
        return 1.0f;
    }
    // Bottom out at 25% speed
    if( value > high ) {
        return 0.25f;
    }
    // Linear interpolation between high and low
    // y = y0 + ( x - x0 ) * ( y1 - y0 ) / ( x1 - x0 )
    return 1.0f + ( value - low ) * ( 0.25f - 1.0f ) / ( high - low );
}

static float workbench_crafting_speed_multiplier( const item &craft, const tripoint &loc )
{
    float multiplier;
    units::mass allowed_mass;
    units::volume allowed_volume;

    if( loc == tripoint_zero ) {
        // tripoint_zero indicates crafting from inventory
        // Use values from f_fake_bench_hands
        const furn_t &f = string_id<furn_t>( "f_fake_bench_hands" ).obj();
        multiplier = f.workbench->multiplier;
        allowed_mass = f.workbench->allowed_mass;
        allowed_volume = f.workbench->allowed_volume;
    } else if( const cata::optional<vpart_reference> vp = g->m.veh_at(
                   loc ).part_with_feature( "WORKBENCH", true ) ) {
        // Vehicle workbench
        const vpart_info &vp_info = vp->part().info();
        if( const cata::optional<vpslot_workbench> &wb_info = vp_info.get_workbench_info() ) {
            multiplier = wb_info->multiplier;
            allowed_mass = wb_info->allowed_mass;
            allowed_volume = wb_info->allowed_volume;
        } else {
            debugmsg( "part '%S' with WORKBENCH flag has no workbench info", vp->part().name() );
            return 0.0f;
        }
    } else if( g->m.furn( loc ).obj().workbench ) {
        // Furniture workbench
        const furn_t &f = g->m.furn( loc ).obj();
        multiplier = f.workbench->multiplier;
        allowed_mass = f.workbench->allowed_mass;
        allowed_volume = f.workbench->allowed_volume;
    } else {
        // Ground
        const furn_t &f = string_id<furn_t>( "f_ground_crafting_spot" ).obj();
        multiplier = f.workbench->multiplier;
        allowed_mass = f.workbench->allowed_mass;
        allowed_volume = f.workbench->allowed_volume;
    }

    const units::mass &craft_mass = craft.weight();
    const units::volume &craft_volume = craft.volume();

    multiplier *= lerped_multiplier( craft_mass, allowed_mass, 1000_kilogram );
    multiplier *= lerped_multiplier( craft_volume, allowed_volume, 1000000_ml );

    return multiplier;
}

float player::crafting_speed_multiplier( const recipe &rec, bool in_progress ) const
{
    const float result = morale_crafting_speed_multiplier( rec ) *
                         lighting_craft_speed_multiplier( rec );
    // Can't start if we'd need 300% time, but we can still finish the job
    if( !in_progress && result < 0.33f ) {
        return 0.0f;
    }
    // If we're working below 10% speed, just give up
    if( result < 0.1f ) {
        return 0.0f;
    }

    return result;
}

float player::crafting_speed_multiplier( const item &craft, const tripoint &loc ) const
{
    if( !craft.is_craft() ) {
        debugmsg( "Can't calculate crafting speed multiplier of non-craft '%s'", craft.tname() );
        return 1.0f;
    }

    const recipe &rec = craft.get_making();

    const float light_multi = lighting_craft_speed_multiplier( rec );
    const float bench_multi = workbench_crafting_speed_multiplier( craft, loc );
    const float morale_multi = morale_crafting_speed_multiplier( rec );

    const float total_multi = light_multi * bench_multi * morale_multi;

    if( light_multi <= 0.0f ) {
        add_msg_if_player( m_bad, _( "You can no longer see well enough to keep crafting." ) );
        return 0.0f;
    }
    if( bench_multi <= 0.1f || ( bench_multi <= 0.33f && total_multi <= 0.2f ) ) {
        add_msg_if_player( m_bad, _( "The %s is too large and/or heavy to work on.  You may want to"
                                     " use a workbench or a smaller batch size" ), craft.tname() );
        return 0.0f;
    }
    if( morale_multi <= 0.2f || ( morale_multi <= 0.33f && total_multi <= 0.2f ) ) {
        add_msg_if_player( m_bad, _( "Your morale is too low to continue crafting." ) );
        return 0.0f;
    }

    // If we're working below 20% speed, just give up
    if( total_multi <= 0.2f ) {
        add_msg_if_player( m_bad, _( "You are too frustrated to continue and just give up." ) );
        return 0.0f;
    }

    if( calendar::once_every( 1_hours ) && total_multi < 0.75f ) {
        if( light_multi <= 0.5f ) {
            add_msg_if_player( m_bad, _( "You can't see well and are working slowly." ) );
        }
        if( bench_multi <= 0.5f ) {
            add_msg_if_player( m_bad,
                               _( "The %s is to large and/or heavy to work on comfortably.  You are"
                                  " working slowly." ), craft.tname() );
        }
        if( morale_multi <= 0.5f ) {
            add_msg_if_player( m_bad, _( "You can't focus and are working slowly." ) );
        }
    }

    return total_multi;
}

bool player::has_morale_to_craft() const
{
    return get_morale_level() >= -50;
}

void player::craft( const tripoint &loc )
{
    int batch_size = 0;
    const recipe *rec = select_crafting_recipe( batch_size );
    if( rec ) {
        if( crafting_allowed( *this, *rec ) ) {
            make_craft( rec->ident(), batch_size, loc );
        }
    }
}

void player::recraft( const tripoint &loc )
{
    if( lastrecipe.str().empty() ) {
        popup( _( "Craft something first" ) );
    } else if( making_would_work( lastrecipe, last_batch ) ) {
        last_craft->execute( loc );
    }
}

void player::long_craft( const tripoint &loc )
{
    int batch_size = 0;
    const recipe *rec = select_crafting_recipe( batch_size );
    if( rec ) {
        if( crafting_allowed( *this, *rec ) ) {
            make_all_craft( rec->ident(), batch_size, loc );
        }
    }
}

bool player::making_would_work( const recipe_id &id_to_make, int batch_size )
{
    const auto &making = *id_to_make;
    if( !( making && crafting_allowed( *this, making ) ) ) {
        return false;
    }

    if( !can_make( &making, batch_size ) ) {
        std::ostringstream buffer;
        buffer << _( "You can no longer make that craft!" ) << "\n";
        buffer << making.requirements().list_missing();
        popup( buffer.str(), PF_NONE );
        return false;
    }

    return check_eligible_containers_for_crafting( making, batch_size );
}

int player::available_assistant_count( const recipe &rec ) const
{
    // NPCs around you should assist in batch production if they have the skills
    // TODO: Cache them in activity, include them in modifier calculations
    const auto helpers = get_crafting_helpers();
    return std::count_if( helpers.begin(), helpers.end(),
    [&]( const npc * np ) {
        return np->get_skill_level( rec.skill_used ) >= rec.difficulty;
    } );
}

int player::base_time_to_craft( const recipe &rec, int batch_size ) const
{
    const size_t assistants = available_assistant_count( rec );
    return rec.batch_time( batch_size, 1.0f, assistants );
}

int player::expected_time_to_craft( const recipe &rec, int batch_size, bool in_progress ) const
{
    const size_t assistants = available_assistant_count( rec );
    float modifier = crafting_speed_multiplier( rec, in_progress );
    return rec.batch_time( batch_size, modifier, assistants );
}

bool player::check_eligible_containers_for_crafting( const recipe &rec, int batch_size ) const
{
    std::vector<const item *> conts = get_eligible_containers_for_crafting();
    const std::vector<item> res = rec.create_results( batch_size );
    const std::vector<item> bps = rec.create_byproducts( batch_size );
    std::vector<item> all;
    all.reserve( res.size() + bps.size() );
    all.insert( all.end(), res.begin(), res.end() );
    all.insert( all.end(), bps.begin(), bps.end() );

    for( const item &prod : all ) {
        if( !prod.made_of( LIQUID ) ) {
            continue;
        }

        // we go through half-filled containers first, then go through empty containers if we need
        std::sort( conts.begin(), conts.end(), item_ptr_compare_by_charges );

        int charges_to_store = prod.charges;
        for( const item *cont : conts ) {
            if( charges_to_store <= 0 ) {
                break;
            }

            if( !cont->is_container_empty() ) {
                if( cont->contents.front().typeId() == prod.typeId() ) {
                    charges_to_store -= cont->get_remaining_capacity_for_liquid( cont->contents.front(), true );
                }
            } else {
                charges_to_store -= cont->get_remaining_capacity_for_liquid( prod, true );
            }
        }

        // also check if we're currently in a vehicle that has the necessary storage
        if( charges_to_store > 0 ) {
            if( optional_vpart_position vp = g->m.veh_at( pos() ) ) {
                const itype_id &ftype = prod.typeId();
                int fuel_cap = vp->vehicle().fuel_capacity( ftype );
                int fuel_amnt = vp->vehicle().fuel_left( ftype );

                if( fuel_cap >= 0 ) {
                    int fuel_space_left = fuel_cap - fuel_amnt;
                    charges_to_store -= fuel_space_left;
                }
            }
        }

        if( charges_to_store > 0 ) {
            popup( _( "You don't have anything to store %s in!" ), prod.tname() );
            return false;
        }
    }

    return true;
}

static bool is_container_eligible_for_crafting( const item &cont, bool allow_bucket )
{
    if( cont.is_watertight_container() || ( allow_bucket && cont.is_bucket() ) ) {
        return !cont.is_container_full( allow_bucket );
    }

    return false;
}

std::vector<const item *> player::get_eligible_containers_for_crafting() const
{
    std::vector<const item *> conts;

    if( is_container_eligible_for_crafting( weapon, true ) ) {
        conts.push_back( &weapon );
    }
    for( const auto &it : worn ) {
        if( is_container_eligible_for_crafting( it, false ) ) {
            conts.push_back( &it );
        }
    }
    for( size_t i = 0; i < inv.size(); i++ ) {
        for( const auto &it : inv.const_stack( i ) ) {
            if( is_container_eligible_for_crafting( it, false ) ) {
                conts.push_back( &it );
            }
        }
    }

    // get all potential containers within PICKUP_RANGE tiles including vehicles
    for( const auto &loc : closest_tripoints_first( PICKUP_RANGE, pos() ) ) {
        // can not reach this -> can not access its contents
        if( pos() != loc && !g->m.clear_path( pos(), loc, PICKUP_RANGE, 1, 100 ) ) {
            continue;
        }
        if( g->m.accessible_items( loc ) ) {
            for( const auto &it : g->m.i_at( loc ) ) {
                if( is_container_eligible_for_crafting( it, true ) ) {
                    conts.emplace_back( &it );
                }
            }
        }

        if( const cata::optional<vpart_reference> vp = g->m.veh_at( loc ).part_with_feature( "CARGO",
                true ) ) {
            for( const auto &it : vp->vehicle().get_items( vp->part_index() ) ) {
                if( is_container_eligible_for_crafting( it, false ) ) {
                    conts.emplace_back( &it );
                }
            }
        }
    }

    return conts;
}

bool player::can_make( const recipe *r, int batch_size )
{
    const inventory &crafting_inv = crafting_inventory();

    if( has_recipe( r, crafting_inv, get_crafting_helpers() ) < 0 ) {
        return false;
    }

    return r->requirements().can_make_with_inventory( crafting_inv, r->get_component_filter(),
            batch_size );
}

bool player::can_start_craft( const recipe *rec, int batch_size )
{
    if( !rec ) {
        return false;
    }

    const std::vector<std::vector<tool_comp>> &tool_reqs = rec->requirements().get_tools();

    // For tools adjust the reqired charges
    std::vector<std::vector<tool_comp>> adjusted_tool_reqs;
    for( const std::vector<tool_comp> &alternatives : tool_reqs ) {
        std::vector<tool_comp> adjusted_alternatives;
        for( const tool_comp &alternative : alternatives ) {
            tool_comp adjusted_alternative = alternative;
            if( adjusted_alternative.count > 0 ) {
                adjusted_alternative.count *= batch_size;
                // Only for the first 5% progress
                adjusted_alternative.count = std::max( adjusted_alternative.count / 20, 1 );
            }
            adjusted_alternatives.push_back( adjusted_alternative );
        }
        adjusted_tool_reqs.push_back( adjusted_alternatives );
    }

    const std::vector<std::vector<item_comp>> &comp_reqs = rec->requirements().get_components();

    // For components we need to multiply by batch size to stay even with tools
    std::vector<std::vector<item_comp>> adjusted_comp_reqs;
    for( const std::vector<item_comp> &alternatives : comp_reqs ) {
        std::vector<item_comp> adjusted_alternatives;
        for( const item_comp &alternative : alternatives ) {
            item_comp adjusted_alternative = alternative;
            adjusted_alternative.count *= batch_size;
            adjusted_alternatives.push_back( adjusted_alternative );
        }
        adjusted_comp_reqs.push_back( adjusted_alternatives );
    }

    // Qualities don't need adjustment
    const requirement_data start_reqs( adjusted_tool_reqs,
                                       rec->requirements().get_qualities(),
                                       adjusted_comp_reqs );

    return start_reqs.can_make_with_inventory( crafting_inventory(), rec->get_component_filter() );
}

const inventory &player::crafting_inventory( const tripoint &src_pos, int radius )
{
    tripoint inv_pos = src_pos;
    if( src_pos == tripoint_zero ) {
        inv_pos = pos();
    }
    if( cached_moves == moves
        && cached_time == calendar::turn
        && cached_position == inv_pos ) {
        return cached_crafting_inventory;
    }
    cached_crafting_inventory.form_from_map( inv_pos, radius, false );
    cached_crafting_inventory += inv;
    cached_crafting_inventory += weapon;
    cached_crafting_inventory += worn;
    for( const auto &bio : *my_bionics ) {
        const auto &bio_data = bio.info();
        if( ( !bio_data.activated || bio.powered ) &&
            !bio_data.fake_item.empty() ) {
            cached_crafting_inventory += item( bio.info().fake_item,
                                               calendar::turn, power_level );
        }
    }
    if( has_trait( trait_BURROW ) ) {
        cached_crafting_inventory += item( "pickaxe", calendar::turn );
        cached_crafting_inventory += item( "shovel", calendar::turn );
    }

    cached_moves = moves;
    cached_time = calendar::turn;
    cached_position = inv_pos;
    return cached_crafting_inventory;
}

void player::invalidate_crafting_inventory()
{
    cached_time = calendar::before_time_starts;
}

void player::make_craft( const recipe_id &id_to_make, int batch_size, const tripoint &loc )
{
    make_craft_with_command( id_to_make, batch_size, false, loc );
}

void player::make_all_craft( const recipe_id &id_to_make, int batch_size, const tripoint &loc )
{
    make_craft_with_command( id_to_make, batch_size, true, loc );
}

void player::make_craft_with_command( const recipe_id &id_to_make, int batch_size, bool is_long,
                                      const tripoint &loc )
{
    const auto &recipe_to_make = *id_to_make;

    if( !recipe_to_make ) {
        return;
    }

    *last_craft = craft_command( &recipe_to_make, batch_size, is_long, this, loc );
    last_craft->execute();
}

// @param offset is the index of the created item in the range [0, batch_size-1],
// it makes sure that the used items are distributed equally among the new items.
static void set_components( std::list<item> &components, const std::list<item> &used,
                            const int batch_size, const size_t offset )
{
    if( batch_size <= 1 ) {
        components.insert( components.begin(), used.begin(), used.end() );
        return;
    }
    // This count does *not* include items counted by charges!
    size_t non_charges_counter = 0;
    for( auto &tmp : used ) {
        if( tmp.count_by_charges() ) {
            components.push_back( tmp );
            // This assumes all (count-by-charges) items of the same type have been merged into one,
            // which has a charges value that can be evenly divided by batch_size.
            components.back().charges = tmp.charges / batch_size;
        } else {
            if( ( non_charges_counter + offset ) % batch_size == 0 ) {
                components.push_back( tmp );
            }
            non_charges_counter++;
        }
    }
}

static void set_item_food( item &newit )
{
    // TODO: encapsulate this into some function
    int bday_tmp = to_turn<int>( newit.birthday() ) % 3600; // fuzzy birthday for stacking reasons
    newit.set_birthday( newit.birthday() + 3600_turns - time_duration::from_turns( bday_tmp ) );
}

static void finalize_crafted_item( item &newit )
{
    if( newit.is_food() ) {
        set_item_food( newit );
    }
    // TODO for now this assumes player is doing the crafting
    // this will need to be updated when NPCs do crafting
    newit.set_owner( g->faction_manager_ptr->get( faction_id( "your_followers" ) ) );
}

static cata::optional<item_location> wield_craft( player &p, item &craft )
{
    if( p.wield( craft ) ) {
        if( p.weapon.invlet ) {
            p.add_msg_if_player( m_info, _( "Wielding %c - %s" ), p.weapon.invlet, p.weapon.display_name() );
        } else {
            p.add_msg_if_player( m_info, _( "Wielding - %s" ), p.weapon.display_name() );
        }
        return item_location( p, &p.weapon );
    }
    return cata::nullopt;
}

static item *set_item_inventory( player &p, item &newit )
{
    item *ret_val = nullptr;
    if( newit.made_of( LIQUID ) ) {
        liquid_handler::handle_all_liquid( newit, PICKUP_RANGE );
    } else {
        p.inv.assign_empty_invlet( newit, p );
        // We might not have space for the item
        if( !p.can_pickVolume( newit ) ) { //Accounts for result_mult
            put_into_vehicle_or_drop( p, item_drop_reason::too_large, { newit } );
        } else if( !p.can_pickWeight( newit, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) ) {
            put_into_vehicle_or_drop( p, item_drop_reason::too_heavy, { newit } );
        } else {
            ret_val = &p.i_add( newit );
            add_msg( m_info, "%c - %s", ret_val->invlet == 0 ? ' ' : ret_val->invlet,
                     ret_val->tname() );
        }
    }
    return ret_val;
}

/**
 * Helper for @ref set_item_map_or_vehicle
 * This is needed to still get a vaild item_location if overflow occurs
 */
static item_location set_item_map( const tripoint &loc, item &newit )
{
    // Includes loc
    const std::vector<tripoint> tiles = closest_tripoints_first( 2, loc );
    for( const tripoint &tile : tiles ) {
        // Pass false to disallow overflow, null_item_reference indicates failure.
        item *it_on_map = &g->m.add_item_or_charges( tile, newit, false );
        if( it_on_map != &null_item_reference() ) {
            return item_location( map_cursor( tile ), it_on_map );
        }
    }
    debugmsg( "Could not place %s on map near (%d, %d, %d)", newit.tname(), loc.x, loc.y, loc.z );
    return item_location();
}

/**
 * Set an item on the map or in a vehicle and return the new location
 */
static item_location set_item_map_or_vehicle( const player &p, const tripoint &loc, item &newit )
{
    if( const cata::optional<vpart_reference> vp = g->m.veh_at( loc ).part_with_feature( "CARGO",
            false ) ) {

        if( const cata::optional<vehicle_stack::iterator> it = vp->vehicle().add_item( vp->part_index(),
                newit ) ) {
            p.add_msg_player_or_npc(
                string_format( pgettext( "item, furniture", "You put the %s on the %s." ),
                               ( *it )->tname(), vp->part().name() ),
                string_format( pgettext( "item, furniture", "<npcname> puts the %s on the %s." ),
                               ( *it )->tname(), vp->part().name() ) );

            return item_location( vehicle_cursor( vp->vehicle(), vp->part_index() ), & **it );
        }

        // Couldn't add the in progress craft to the target part, so drop it to the map.
        p.add_msg_player_or_npc(
            string_format( pgettext( "furniture, item",
                                     "Not enough space on the %s. You drop the %s on the ground." ),
                           vp->part().name(), newit.tname() ),
            string_format( pgettext( "furniture, item",
                                     "Not enough space on the %s. <npcname> drops the %s on the ground." ),
                           vp->part().name(), newit.tname() ) );

        return set_item_map( loc, newit );

    } else {
        if( g->m.has_furn( loc ) ) {
            const furn_t &workbench = g->m.furn( loc ).obj();
            p.add_msg_player_or_npc(
                string_format( pgettext( "item, furniture", "You put the %s on the %s." ),
                               newit.tname(), workbench.name() ),
                string_format( pgettext( "item, furniture", "<npcname> puts the %s on the %s." ),
                               newit.tname(), workbench.name() ) );
        } else {
            p.add_msg_player_or_npc(
                string_format( pgettext( "item", "You put the %s on the ground." ),
                               newit.tname() ),
                string_format( pgettext( "item", "<npcname> puts the %s on the ground." ),
                               newit.tname() ) );
        }
        return set_item_map( loc, newit );
    }
}

void player::start_craft( craft_command &command, const tripoint &loc )
{
    if( command.empty() ) {
        debugmsg( "Attempted to start craft with empty command" );
        return;
    }

    item craft = command.create_in_progress_craft();

    // In case we were wearing something just consumed
    if( !craft.components.empty() ) {
        reset_encumbrance();
    }

    item_location craft_in_world;

    // Check if we are standing next to a workbench. If so, just use that.
    float best_bench_multi = 0.0;
    tripoint target = loc;
    for( const tripoint &adj : g->m.points_in_radius( pos(), 1 ) ) {
        if( const cata::optional<furn_workbench_info> &wb = g->m.furn( adj ).obj().workbench ) {
            if( wb->multiplier > best_bench_multi ) {
                best_bench_multi = wb->multiplier;
                target = adj;
            }
        } else if( const cata::optional<vpart_reference> vp = g->m.veh_at(
                       adj ).part_with_feature( "WORKBENCH", true ) ) {
            if( const cata::optional<vpslot_workbench> &wb_info = vp->part().info().get_workbench_info() ) {
                if( wb_info->multiplier > best_bench_multi ) {
                    best_bench_multi = wb_info->multiplier;
                    target = adj;
                }
            } else {
                debugmsg( "part '%S' with WORKBENCH flag has no workbench info", vp->part().name() );
            }
        }
    }

    // Crafting without a workbench
    if( target == tripoint_zero ) {
        if( !is_armed() ) {
            if( cata::optional<item_location> it_loc = wield_craft( *this, craft ) ) {
                craft_in_world = *it_loc;
            }  else {
                // This almost certianly shouldn't happen
                put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, {craft} );
            }
        } else {
            enum option : int {
                WIELD_CRAFT = 0,
                DROP_CRAFT,
                STASH,
                DROP
            };

            uilist amenu;
            amenu.text = string_format( pgettext( "in progress craft", "What to do with the %s?" ),
                                        craft.display_name() );
            amenu.addentry( WIELD_CRAFT, !weapon.has_flag( "NO_UNWIELD" ), '1',
                            string_format( _( "Dispose of your wielded %s and start working." ), weapon.tname() ) );
            amenu.addentry( DROP_CRAFT, true, '2', _( "Put it down and start working." ) );
            const bool can_stash = can_pickVolume( craft ) &&
                                   can_pickWeight( craft, !get_option<bool>( "DANGEROUS_PICKUPS" ) );
            amenu.addentry( STASH, can_stash, '3', _( "Store it in your inventory." ) );
            amenu.addentry( DROP, true, '4', _( "Drop it on the ground." ) );

            amenu.query();
            const option choice = amenu.ret == UILIST_CANCEL ? DROP : static_cast<option>( amenu.ret );
            switch( choice ) {
                case WIELD_CRAFT: {
                    if( cata::optional<item_location> it_loc = wield_craft( *this, craft ) ) {
                        craft_in_world = *it_loc;
                    } else {
                        // This almost certianly shouldn't happen
                        put_into_vehicle_or_drop( *this, item_drop_reason::tumbling, {craft} );
                    }
                    break;
                }
                case DROP_CRAFT: {
                    craft_in_world = set_item_map_or_vehicle( *this, pos(), craft );
                    break;
                }
                case STASH: {
                    set_item_inventory( *this, craft );
                    break;
                }
                case DROP: {
                    put_into_vehicle_or_drop( *this, item_drop_reason::deliberate, {craft} );
                    break;
                }
            }
        }
    } else {
        // We have a workbench, put the item there.
        craft_in_world = set_item_map_or_vehicle( *this, target, craft );
    }

    if( !craft_in_world.get_item() ) {
        add_msg_if_player( _( "Wield and activate the %s to start crafting." ), craft.tname() );
        return;
    }

    assign_activity( activity_id( "ACT_CRAFT" ) );
    activity.targets.push_back( craft_in_world );
    activity.values.push_back( command.is_long() );

    add_msg_player_or_npc(
        string_format( pgettext( "in progress craft", "You start working on the %s." ),
                       craft.tname() ),
        string_format( pgettext( "in progress craft", "<npcname> starts working on the %s." ),
                       craft.tname() ) );
}

void player::craft_skill_gain( const item &craft, const int &multiplier )
{
    if( !craft.is_craft() ) {
        debugmsg( "craft_skill_check() called on non-craft '%s.' Aborting.", craft.tname() );
        return;
    }

    const recipe &making = craft.get_making();
    const int batch_size = craft.charges;

    std::vector<npc *> helpers = get_crafting_helpers();

    if( making.skill_used ) {
        // Normalize experience gain to crafting time, giving a bonus for longer crafting
        const double batch_mult = batch_size + base_time_to_craft( making, batch_size ) / 30000.0;
        // This is called after every 5% crafting progress, so divide by 20
        const int base_practice = roll_remainder( ( making.difficulty * 15 + 10 ) * batch_mult /
                                  20.0 ) * multiplier;
        const int skill_cap = static_cast<int>( making.difficulty * 1.25 );
        practice( making.skill_used, base_practice, skill_cap );

        // NPCs assisting or watching should gain experience...
        for( auto &helper : helpers ) {
            //If the NPC can understand what you are doing, they gain more exp
            if( helper->get_skill_level( making.skill_used ) >= making.difficulty ) {
                helper->practice( making.skill_used, roll_remainder( base_practice / 2.0 ),
                                  skill_cap );
                if( batch_size > 1 && one_in( 3 ) ) {
                    add_msg( m_info, _( "%s assists with crafting..." ), helper->name );
                }
                if( batch_size == 1 && one_in( 3 ) ) {
                    add_msg( m_info, _( "%s could assist you with a batch..." ), helper->name );
                }
                // NPCs around you understand the skill used better
            } else {
                helper->practice( making.skill_used, roll_remainder( base_practice / 10.0 ),
                                  skill_cap );
                if( one_in( 3 ) ) {
                    add_msg( m_info, _( "%s watches you craft..." ), helper->name );
                }
            }
        }
    }
}

double player::crafting_success_roll( const recipe &making ) const
{
    int secondary_dice = 0;
    int secondary_difficulty = 0;
    for( const auto &pr : making.required_skills ) {
        secondary_dice += get_skill_level( pr.first );
        secondary_difficulty += pr.second;
    }

    // # of dice is 75% primary skill, 25% secondary (unless secondary is null)
    int skill_dice;
    if( secondary_difficulty > 0 ) {
        skill_dice = get_skill_level( making.skill_used ) * 3 + secondary_dice;
    } else {
        skill_dice = get_skill_level( making.skill_used ) * 4;
    }

    for( const npc *np : get_crafting_helpers() ) {
        if( np->get_skill_level( making.skill_used ) >=
            get_skill_level( making.skill_used ) ) {
            // NPC assistance is worth half a skill level
            skill_dice += 2;
            add_msg_if_player( m_info, _( "%s helps with crafting..." ), np->name );
            break;
        }
    }

    // farsightedness can impose a penalty on electronics and tailoring success
    // it's equivalent to a 2-rank electronics penalty, 1-rank tailoring
    if( has_trait( trait_id( "HYPEROPIC" ) ) && !worn_with_flag( "FIX_FARSIGHT" ) &&
        !has_effect( effect_contacts ) ) {
        int main_rank_penalty = 0;
        if( making.skill_used == skill_id( "electronics" ) ) {
            main_rank_penalty = 2;
        } else if( making.skill_used == skill_id( "tailor" ) ) {
            main_rank_penalty = 1;
        }
        skill_dice -= main_rank_penalty * 4;
    }

    // It's tough to craft with paws.  Fortunately it's just a matter of grip and fine-motor,
    // not inability to see what you're doing
    if( has_trait( trait_PAWS ) || has_trait( trait_PAWS_LARGE ) ) {
        int paws_rank_penalty = 0;
        if( has_trait( trait_PAWS_LARGE ) ) {
            paws_rank_penalty += 1;
        }
        if( making.skill_used == skill_id( "electronics" )
            || making.skill_used == skill_id( "tailor" )
            || making.skill_used == skill_id( "mechanics" ) ) {
            paws_rank_penalty += 1;
        }
        skill_dice -= paws_rank_penalty * 4;
    }

    // Sides on dice is 16 plus your current intelligence
    ///\EFFECT_INT increases crafting success chance
    const int skill_sides = 16 + int_cur;

    int diff_dice;
    if( secondary_difficulty > 0 ) {
        diff_dice = making.difficulty * 3 + secondary_difficulty;
    } else {
        // Since skill level is * 4 also
        diff_dice = making.difficulty * 4;
    }

    const int diff_sides = 24; // 16 + 8 (default intelligence)

    const double skill_roll = dice( skill_dice, skill_sides );
    const double diff_roll = dice( diff_dice, diff_sides );

    return skill_roll / diff_roll;
}

int item::get_next_failure_point() const
{
    if( !is_craft() ) {
        debugmsg( "get_next_failure_point() called on non-craft '%s.'  Aborting.", tname() );
        return INT_MAX;
    }
    return next_failure_point >= 0 ? next_failure_point : INT_MAX;
}

void item::set_next_failure_point( const player &crafter )
{
    if( !is_craft() ) {
        debugmsg( "set_next_failure_point() called on non-craft '%s.'  Aborting.", tname() );
        return;
    }

    const int percent_left = 10000000 - item_counter;
    const int failure_point_delta = crafter.crafting_success_roll( get_making() ) * percent_left;

    next_failure_point = item_counter + failure_point_delta;
}

static void destroy_random_component( item &craft, const player &crafter )
{
    if( craft.components.empty() ) {
        debugmsg( "destroy_random_component() called on craft with no components! Aborting" );
        return;
    }

    item destroyed = random_entry_removed( craft.components );

    crafter.add_msg_player_or_npc(
        string_format( _( "You mess up and destroy the %s." ), destroyed.tname() ),
        string_format( _( "<npcname> messes up and destroys the %s" ), destroyed.tname() )
    );
}

void item::handle_craft_failure( player &crafter )
{
    if( !is_craft() ) {
        debugmsg( "handle_craft_failure() called on non-craft '%s.'  Aborting.", tname() );
        return;
    }

    const double success_roll = crafter.crafting_success_roll( get_making() );

    // Destroy at most 75% of the components, always a chance of losing 1 though
    const size_t max_destroyed = std::max<size_t>( 1, components.size() * 3 / 4 );
    for( size_t i = 0; i < max_destroyed; i++ ) {
        // This shouldn't happen
        if( components.empty() ) {
            break;
        }
        // If we roll success, skip destroying a component
        if( x_in_y( success_roll, 1.0 ) ) {
            continue;
        }
        destroy_random_component( *this, crafter );
    }

    // Minimum 25% progress lost, average 35%.  Falls off exponentially
    // Loss is scaled by the success roll
    const double percent_progress_loss = rng_exponential( 0.25, 0.35 ) *
                                         ( 1.0 - std::min( success_roll, 1.0 ) );
    const int progess_loss = item_counter * percent_progress_loss;
    crafter.add_msg_player_or_npc(
        string_format( _( "You mess up and lose %d%% progress." ), progess_loss / 100000 ),
        string_format( _( "<npcname> messes up and loses %d%% progress." ), progess_loss / 100000 )
    );
    item_counter = clamp( item_counter - progess_loss, 0, 10000000 );

    set_next_failure_point( crafter );

    // Check if we can consume a new component and continue
    if( !crafter.can_continue_craft( *this ) ) {
        crafter.cancel_activity();
    }
}

requirement_data item::get_continue_reqs() const
{
    if( !is_craft() ) {
        debugmsg( "get_continue_reqs() called on non-craft '%s.'  Aborting.", tname() );
        return requirement_data();
    }
    return requirement_data::continue_requirements( comps_used, components );
}

void player::complete_craft( item &craft, const tripoint &loc )
{
    if( !craft.is_craft() ) {
        debugmsg( "complete_craft() called on non-craft '%s.'  Aborting.", craft.tname() );
        return;
    }

    const recipe &making = craft.get_making();
    const int batch_size = craft.charges;
    std::list<item> &used = craft.components;
    const double relative_rot = craft.get_relative_rot();

    // Set up the new item, and assign an inventory letter if available
    std::vector<item> newits = making.create_results( batch_size );

    const bool should_heat = making.hot_result();

    bool first = true;
    size_t newit_counter = 0;
    for( item &newit : newits ) {
        // messages, learning of recipe, food spoilage calculation only once
        if( first ) {
            first = false;
            // TODO: reconsider recipe memorization
            if( knows_recipe( &making ) ) {
                add_msg( _( "You craft %s from memory." ), making.result_name() );
            } else {
                add_msg( _( "You craft %s using a book as a reference." ), making.result_name() );
                // If we made it, but we don't know it,
                // we're making it from a book and have a chance to learn it.
                // Base expected time to learn is 1000*(difficulty^4)/skill/int moves.
                // This means time to learn is greatly decreased with higher skill level,
                // but also keeps going up as difficulty goes up.
                // Worst case is lvl 10, which will typically take
                // 10^4/10 (1,000) minutes, or about 16 hours of crafting it to learn.
                int difficulty = has_recipe( &making, crafting_inventory(), get_crafting_helpers() );
                ///\EFFECT_INT increases chance to learn recipe when crafting from a book
                const double learning_speed =
                    std::max( get_skill_level( making.skill_used ), 1 ) *
                    std::max( get_int(), 1 );
                const double time_to_learn = 1000 * 8 * pow( difficulty, 4 ) / learning_speed;
                if( x_in_y( making.time, time_to_learn ) ) {
                    learn_recipe( &making );
                    add_msg( m_good, _( "You memorized the recipe for %s!" ),
                             making.result_name() );
                }
            }

            for( auto &component : used ) {
                if( component.has_flag( "HIDDEN_HALLU" ) ) {
                    newit.item_tags.insert( "HIDDEN_HALLU" );
                }
                if( component.has_flag( "HIDDEN_POISON" ) ) {
                    newit.item_tags.insert( "HIDDEN_POISON" );
                    newit.poison = component.poison;
                }
            }
        }

        // Don't store components for things made by charges,
        // Don't store components for things that can't be uncrafted.
        if( recipe_dictionary::get_uncraft( making.result() ) && !newit.count_by_charges() &&
            making.is_reversible() ) {
            // Setting this for items counted by charges gives only problems:
            // those items are automatically merged everywhere (map/vehicle/inventory),
            // which would either lose this information or merge it somehow.
            set_components( newit.components, used, batch_size, newit_counter );
            newit_counter++;
        } else if( newit.is_food() && !newit.has_flag( "NUTRIENT_OVERRIDE" ) ) {
            // if a component item has "cooks_like" it will be replaced by that item as a component
            for( item &comp : used ) {
                // only comestibles have cooks_like.  any other type of item will throw an exception, so filter those out
                if( comp.is_comestible() && !comp.get_comestible()->cooks_like.empty() ) {
                    comp = item( comp.get_comestible()->cooks_like, comp.birthday(), comp.charges );
                }
            }
            // byproducts get stored as a "component" but with a byproduct flag for consumption purposes
            if( making.has_byproducts() ) {
                for( item &byproduct : making.create_byproducts( batch_size ) ) {
                    byproduct.set_flag( "BYPRODUCT" );
                    used.push_back( byproduct );
                }
            }
            // store components for food recipes that do not have the override flag
            set_components( newit.components, used, batch_size, newit_counter );
            // store the number of charges the recipe creates
            newit.recipe_charges = newit.charges / batch_size;
            newit_counter++;
        }

        if( newit.goes_bad() ) {
            newit.set_relative_rot( relative_rot );
        } else {
            if( newit.is_container() ) {
                for( item &in : newit.contents ) {
                    if( in.goes_bad() ) {
                        in.set_relative_rot( relative_rot );
                    }
                }
            }
        }

        if( newit.has_temperature() ) {
            if( should_heat ) {
                newit.heat_up();
            } else {
                // Really what we should be doing is averaging the temperatures
                // between the recipe components if we don't have a heat tool, but
                // that's kind of hard.  For now just set the item to 20 C
                // and reset the temperature, don't
                // forget byproducts below either when you fix this.
                //
                // Temperature is not functional for non-foods
                newit.set_item_temperature( 293.15 );
                newit.reset_temp_check();
            }
        }

        finalize_crafted_item( newit );
        if( newit.made_of( LIQUID ) ) {
            liquid_handler::handle_all_liquid( newit, PICKUP_RANGE );
        } else if( loc == tripoint_zero ) {
            wield_craft( *this, newit );
        } else {
            set_item_map_or_vehicle( *this, loc, newit );
        }
    }

    if( making.has_byproducts() ) {
        std::vector<item> bps = making.create_byproducts( batch_size );
        for( auto &bp : bps ) {
            if( bp.goes_bad() ) {
                bp.set_relative_rot( relative_rot );
            }
            if( bp.has_temperature() ) {
                if( should_heat ) {
                    bp.heat_up();
                } else {
                    bp.set_item_temperature( 293.15 );
                    bp.reset_temp_check();
                }
            }
            finalize_crafted_item( bp );
            if( bp.made_of( LIQUID ) ) {
                liquid_handler::handle_all_liquid( bp, PICKUP_RANGE );
            } else if( loc == tripoint_zero ) {
                set_item_inventory( *this, bp );
            } else {
                set_item_map_or_vehicle( *this, loc, bp );
            }
        }
    }

    inv.restack( *this );
}

bool player::can_continue_craft( item &craft )
{
    if( !craft.is_craft() ) {
        debugmsg( "complete_craft() called on non-craft '%s.'  Aborting.", craft.tname() );
        return false;
    }

    const recipe &rec = craft.get_making();

    const requirement_data continue_reqs = craft.get_continue_reqs();

    // Avoid building an inventory from the map if we don't have to, as it is expensive
    if( !continue_reqs.is_empty() ) {

        const std::function<bool( const item & )> filter = rec.get_component_filter();
        // continue_reqs are for all batches at once
        const int batch_size = 1;

        if( !continue_reqs.can_make_with_inventory( crafting_inventory(), filter, batch_size ) ) {
            std::ostringstream buffer;
            buffer << _( "You don't have the required components to continue crafting!" ) << "\n";
            buffer << continue_reqs.list_missing();
            popup( buffer.str(), PF_NONE );
            return false;
        }

        std::ostringstream buffer;
        buffer << _( "Consume the missing components and continue crafting?" ) << "\n";
        buffer << continue_reqs.list_all() << "\n";
        if( !query_yn( buffer.str() ) ) {
            return false;
        }

        inventory map_inv;
        map_inv.form_from_map( pos(), PICKUP_RANGE );

        std::vector<comp_selection<item_comp>> item_selections;
        for( const auto &it : continue_reqs.get_components() ) {
            comp_selection<item_comp> is = select_item_component( it, batch_size, map_inv, true, filter );
            if( is.use_from == cancel ) {
                cancel_activity();
                add_msg( _( "You stop crafting." ) );
                return false;
            }
            item_selections.push_back( is );
        }
        for( const auto &it : item_selections ) {
            craft.components.splice( craft.components.end(), consume_items( it, batch_size, filter ) );
        }
    }

    if( !craft.has_tools_to_continue() ) {

        const std::vector<std::vector<tool_comp>> &tool_reqs = rec.requirements().get_tools();
        const int batch_size = craft.charges;

        std::vector<std::vector<tool_comp>> adjusted_tool_reqs;
        for( const std::vector<tool_comp> &alternatives : tool_reqs ) {
            std::vector<tool_comp> adjusted_alternatives;
            for( const tool_comp &alternative : alternatives ) {
                tool_comp adjusted_alternative = alternative;
                if( adjusted_alternative.count > 0 ) {
                    adjusted_alternative.count *= batch_size;
                    // Only for the next 5% progress
                    adjusted_alternative.count = std::max( adjusted_alternative.count / 20, 1 );
                }
                adjusted_alternatives.push_back( adjusted_alternative );
            }
            adjusted_tool_reqs.push_back( adjusted_alternatives );
        }

        const requirement_data tool_continue_reqs( adjusted_tool_reqs,
                std::vector<std::vector<quality_requirement>>(),
                std::vector<std::vector<item_comp>>() );

        if( !tool_continue_reqs.can_make_with_inventory( crafting_inventory(), return_true<item> ) ) {
            std::ostringstream buffer;
            buffer << _( "You don't have the necessary tools to continue crafting!" ) << "\n";
            buffer << tool_continue_reqs.list_missing();
            popup( buffer.str(), PF_NONE );
            return false;
        }

        inventory map_inv;
        map_inv.form_from_map( pos(), PICKUP_RANGE );

        std::vector<comp_selection<tool_comp>> new_tool_selections;
        for( const std::vector<tool_comp> &alternatives : tool_reqs ) {
            comp_selection<tool_comp> selection = select_tool_component( alternatives, batch_size,
            map_inv, DEFAULT_HOTKEYS, true, true, []( int charges ) {
                return charges / 20;
            } );
            if( selection.use_from == cancel ) {
                return false;
            }
            new_tool_selections.push_back( selection );
        }

        craft.set_cached_tool_selections( new_tool_selections );
        craft.set_tools_to_continue( true );
    }

    return true;
}

/* selection of component if a recipe requirement has multiple options (e.g. 'duct tap' or 'welder') */
comp_selection<item_comp> player::select_item_component( const std::vector<item_comp> &components,
        int batch, inventory &map_inv, bool can_cancel,
        const std::function<bool( const item & )> &filter, bool player_inv )
{
    std::vector<item_comp> player_has;
    std::vector<item_comp> map_has;
    std::vector<item_comp> mixed;

    comp_selection<item_comp> selected;

    for( const auto &component : components ) {
        itype_id type = component.type;
        int count = ( component.count > 0 ) ? component.count * batch : abs( component.count );

        if( item::count_by_charges( type ) && count > 0 ) {
            int map_charges = map_inv.charges_of( type, INT_MAX, filter );

            // If map has infinite charges, just use them
            if( map_charges == item::INFINITE_CHARGES ) {
                selected.use_from = use_from_map;
                selected.comp = component;
                return selected;
            }
            if( player_inv ) {
                int player_charges = charges_of( type, INT_MAX, filter );
                bool found = false;
                if( player_charges >= count ) {
                    player_has.push_back( component );
                    found = true;
                }
                if( map_charges >= count ) {
                    map_has.push_back( component );
                    found = true;
                }
                if( !found && player_charges + map_charges >= count ) {
                    mixed.push_back( component );
                }
            } else {
                if( map_charges >= count ) {
                    map_has.push_back( component );
                }
            }
        } else { // Counting by units, not charges

            // Can't use pseudo items as components
            if( player_inv ) {
                bool found = false;
                if( has_amount( type, count, false, filter ) ) {
                    player_has.push_back( component );
                    found = true;
                }
                if( map_inv.has_components( type, count, filter ) ) {
                    map_has.push_back( component );
                    found = true;
                }
                if( !found &&
                    amount_of( type, false, std::numeric_limits<int>::max(), filter ) +
                    map_inv.amount_of( type, false, std::numeric_limits<int>::max(), filter ) >= count ) {
                    mixed.push_back( component );
                }
            } else {
                if( map_inv.has_components( type, count, filter ) ) {
                    map_has.push_back( component );
                }
            }
        }
    }

    /* select 1 component to use */
    if( player_has.size() + map_has.size() + mixed.size() == 1 ) { // Only 1 choice
        if( player_has.size() == 1 ) {
            selected.use_from = use_from_player;
            selected.comp = player_has[0];
        } else if( map_has.size() == 1 ) {
            selected.use_from = use_from_map;
            selected.comp = map_has[0];
        } else {
            selected.use_from = use_from_both;
            selected.comp = mixed[0];
        }
    } else { // Let the player pick which component they want to use
        uilist cmenu;
        // Populate options with the names of the items
        for( auto &map_ha : map_has ) { // Index 0-(map_has.size()-1)
            std::string tmpStr = string_format( _( "%s (%d/%d nearby)" ),
                                                item::nname( map_ha.type ),
                                                ( map_ha.count * batch ),
                                                item::count_by_charges( map_ha.type ) ?
                                                map_inv.charges_of( map_ha.type, INT_MAX, filter ) :
                                                map_inv.amount_of( map_ha.type, false, INT_MAX, filter ) );
            cmenu.addentry( tmpStr );
        }
        for( auto &player_ha : player_has ) { // Index map_has.size()-(map_has.size()+player_has.size()-1)
            std::string tmpStr = string_format( _( "%s (%d/%d on person)" ),
                                                item::nname( player_ha.type ),
                                                ( player_ha.count * batch ),
                                                item::count_by_charges( player_ha.type ) ?
                                                charges_of( player_ha.type, INT_MAX, filter ) :
                                                amount_of( player_ha.type, false, INT_MAX, filter ) );
            cmenu.addentry( tmpStr );
        }
        for( auto &component : mixed ) {
            // Index player_has.size()-(map_has.size()+player_has.size()+mixed.size()-1)
            int available = item::count_by_charges( component.type ) ?
                            map_inv.charges_of( component.type, INT_MAX, filter ) +
                            charges_of( component.type, INT_MAX, filter ) :
                            map_inv.amount_of( component.type, false, INT_MAX, filter ) +
                            amount_of( component.type, false, INT_MAX, filter );
            std::string tmpStr = string_format( _( "%s (%d/%d nearby & on person)" ),
                                                item::nname( component.type ),
                                                component.count * batch,
                                                available );
            cmenu.addentry( tmpStr );
        }

        // Unlike with tools, it's a bad thing if there aren't any components available
        if( cmenu.entries.empty() ) {
            if( player_inv ) {
                if( has_trait( trait_id( "DEBUG_HS" ) ) ) {
                    selected.use_from = use_from_player;
                    return selected;
                }
            }
            debugmsg( "Attempted a recipe with no available components!" );
            selected.use_from = cancel;
            return selected;
        }

        cmenu.allow_cancel = can_cancel;

        // Get the selection via a menu popup
        cmenu.title = _( "Use which component?" );
        cmenu.query();

        if( cmenu.ret < 0 ||
            static_cast<size_t>( cmenu.ret ) >= map_has.size() + player_has.size() + mixed.size() ) {
            selected.use_from = cancel;
            return selected;
        }

        size_t uselection = static_cast<size_t>( cmenu.ret );
        if( uselection < map_has.size() ) {
            selected.use_from = usage::use_from_map;
            selected.comp = map_has[uselection];
        } else if( uselection < map_has.size() + player_has.size() ) {
            uselection -= map_has.size();
            selected.use_from = usage::use_from_player;
            selected.comp = player_has[uselection];
        } else {
            uselection -= map_has.size() + player_has.size();
            selected.use_from = usage::use_from_both;
            selected.comp = mixed[uselection];
        }
    }

    return selected;
}

// Prompts player to empty all newly-unsealed containers in inventory
// Called after something that might have opened containers (making them buckets) but not emptied them
static void empty_buckets( player &p )
{
    // First grab (remove) all items that are non-empty buckets and not wielded
    auto buckets = p.remove_items_with( [&p]( const item & it ) {
        return it.is_bucket_nonempty() && &it != &p.weapon;
    }, INT_MAX );
    for( auto &it : buckets ) {
        for( const item &in : it.contents ) {
            drop_or_handle( in, p );
        }

        it.contents.clear();
        drop_or_handle( it, p );
    }
}

std::list<item> player::consume_items( const comp_selection<item_comp> &is, int batch,
                                       const std::function<bool( const item & )> &filter )
{
    return consume_items( g->m, is, batch, filter, pos(), PICKUP_RANGE );
}

std::list<item> player::consume_items( map &m, const comp_selection<item_comp> &is, int batch,
                                       const std::function<bool( const item & )> &filter,
                                       const tripoint &origin, int radius )
{
    std::list<item> ret;

    if( has_trait( trait_DEBUG_HS ) ) {
        return ret;
    }

    item_comp selected_comp = is.comp;

    const tripoint &loc = origin;
    const bool by_charges = item::count_by_charges( selected_comp.type ) && selected_comp.count > 0;
    // Count given to use_amount/use_charges, changed by those functions!
    int real_count = ( selected_comp.count > 0 ) ? selected_comp.count * batch : abs(
                         selected_comp.count );
    // First try to get everything from the map, than (remaining amount) from player
    if( is.use_from & use_from_map ) {
        if( by_charges ) {
            std::list<item> tmp = m.use_charges( loc, radius, selected_comp.type, real_count, filter );
            ret.splice( ret.end(), tmp );
        } else {
            std::list<item> tmp = g->m.use_amount( loc, radius, selected_comp.type, real_count, filter );
            remove_ammo( tmp, *this );
            ret.splice( ret.end(), tmp );
        }
    }
    if( is.use_from & use_from_player ) {
        if( by_charges ) {
            std::list<item> tmp = use_charges( selected_comp.type, real_count, filter );
            ret.splice( ret.end(), tmp );
        } else {
            std::list<item> tmp = use_amount( selected_comp.type, real_count, filter );
            remove_ammo( tmp, *this );
            ret.splice( ret.end(), tmp );
        }
    }
    // condense those items into one
    if( by_charges && ret.size() > 1 ) {
        std::list<item>::iterator b = ret.begin();
        b++;
        while( ret.size() > 1 ) {
            ret.front().charges += b->charges;
            b = ret.erase( b );
        }
    }
    lastconsumed = selected_comp.type;
    empty_buckets( *this );
    return ret;
}

/* This call is in-efficient when doing it for multiple items with the same map inventory.
In that case, consider using select_item_component with 1 pre-created map inventory, and then passing the results
to consume_items */
std::list<item> player::consume_items( const std::vector<item_comp> &components, int batch,
                                       const std::function<bool( const item & )> &filter )
{
    inventory map_inv;
    map_inv.form_from_map( pos(), PICKUP_RANGE );
    return consume_items( select_item_component( components, batch, map_inv, false, filter ), batch,
                          filter );
}

comp_selection<tool_comp>
player::select_tool_component( const std::vector<tool_comp> &tools, int batch, inventory &map_inv,
                               const std::string &hotkeys, bool can_cancel, bool player_inv,
                               const std::function<int( int )> charges_required_modifier )
{

    comp_selection<tool_comp> selected;

    bool found_nocharge = false;
    std::vector<tool_comp> player_has;
    std::vector<tool_comp> map_has;
    // Use charges of any tools that require charges used
    for( auto it = tools.begin(); it != tools.end() && !found_nocharge; ++it ) {
        itype_id type = it->type;
        if( it->count > 0 ) {
            const int count = charges_required_modifier( item::find_type( type )->charge_factor() * it->count *
                              batch );
            if( player_inv ) {
                if( has_charges( type, count ) ) {
                    player_has.push_back( *it );
                }
            }
            if( map_inv.has_charges( type, count ) ) {
                map_has.push_back( *it );
            }
        } else if( ( player_inv && has_amount( type, 1 ) ) || map_inv.has_tools( type, 1 ) ) {
            selected.comp = *it;
            found_nocharge = true;
        }
    }
    if( found_nocharge ) {
        selected.use_from = use_from_none;
        return selected;    // Default to using a tool that doesn't require charges
    }

    if( player_has.size() + map_has.size() == 1 ) {
        if( map_has.empty() ) {
            selected.use_from = use_from_player;
            selected.comp = player_has[0];
        } else {
            selected.use_from = use_from_map;
            selected.comp = map_has[0];
        }
    } else { // Variety of options, list them and pick one
        // Populate the list
        uilist tmenu( hotkeys );
        for( auto &map_ha : map_has ) {
            const itype *tmp = item::find_type( map_ha.type );
            if( tmp->maximum_charges() > 1 ) {
                const int charge_count = map_ha.count * batch * tmp->charge_factor();
                std::string tmpStr = string_format( _( "%s (%d/%d charges nearby)" ),
                                                    item::nname( map_ha.type ), charge_count,
                                                    map_inv.charges_of( map_ha.type ) );
                tmenu.addentry( tmpStr );
            } else {
                std::string tmpStr = item::nname( map_ha.type ) + _( " (nearby)" );
                tmenu.addentry( tmpStr );
            }
        }
        for( auto &player_ha : player_has ) {
            const itype *tmp = item::find_type( player_ha.type );
            if( tmp->maximum_charges() > 1 ) {
                const int charge_count = player_ha.count * batch * tmp->charge_factor();
                std::string tmpStr = string_format( _( "%s (%d/%d charges on person)" ),
                                                    item::nname( player_ha.type ), charge_count,
                                                    charges_of( player_ha.type ) );
                tmenu.addentry( tmpStr );
            } else {
                tmenu.addentry( item::nname( player_ha.type ) );
            }
        }

        if( tmenu.entries.empty() ) {  // This SHOULD only happen if cooking with a fire,
            selected.use_from = use_from_none;
            return selected;    // and the fire goes out.
        }

        tmenu.allow_cancel = can_cancel;

        // Get selection via a popup menu
        tmenu.title = _( "Use which tool?" );
        tmenu.query();

        if( tmenu.ret < 0 || static_cast<size_t>( tmenu.ret ) >= map_has.size() + player_has.size() ) {
            selected.use_from = cancel;
            return selected;
        }

        size_t uselection = static_cast<size_t>( tmenu.ret );
        if( uselection < map_has.size() ) {
            selected.use_from = use_from_map;
            selected.comp = map_has[uselection];
        } else {
            uselection -= map_has.size();
            selected.use_from = use_from_player;
            selected.comp = player_has[uselection];
        }
    }

    return selected;
}

bool player::craft_consume_tools( item &craft, int mulitplier, bool start_craft )
{
    if( !craft.is_craft() ) {
        debugmsg( "craft_consume_tools() called on non-craft '%s.' Aborting.", craft.tname() );
        return false;
    }

    const auto calc_charges = [&craft, &start_craft, &mulitplier]( int charges ) {
        int ret = charges;

        if( ret <= 0 ) {
            return ret;
        }

        // Account for batch size
        ret *= craft.charges;

        // Only for the next 5% progress
        ret /= 20;

        // In case more than 5% progress was accomplished in one turn
        ret *= mulitplier;

        // If just starting consume the remainder as well
        if( start_craft ) {
            ret += ( charges * craft.charges ) % 20;
        }
        return ret;
    };

    // First check if we still have our cached selections
    const std::vector<comp_selection<tool_comp>> &cached_tool_selections =
                craft.get_cached_tool_selections();

    inventory map_inv;
    map_inv.form_from_map( pos(), PICKUP_RANGE );

    for( const comp_selection<tool_comp> &tool_sel : cached_tool_selections ) {
        itype_id type = tool_sel.comp.type;
        if( tool_sel.comp.count > 0 ) {
            const int count = calc_charges( tool_sel.comp.count );
            switch( tool_sel.use_from ) {
                case use_from_player:
                    if( !has_charges( type, count ) ) {
                        add_msg_player_or_npc(
                            string_format( _( "You have insufficient %s charges and can't continue crafting" ),
                                           item::nname( type ) ),
                            string_format( _( "<npcname> has insufficient %s charges and can't continue crafting" ),
                                           item::nname( type ) ) );
                        craft.set_tools_to_continue( false );
                        return false;
                    }
                    break;
                case use_from_map:
                    if( !map_inv.has_charges( type, count ) ) {
                        add_msg_player_or_npc(
                            string_format( _( "You have insufficient %s charges and can't continue crafting" ),
                                           item::nname( type ) ),
                            string_format( _( "<npcname> has insufficient %s charges and can't continue crafting" ),
                                           item::nname( type ) ) );
                        craft.set_tools_to_continue( false );
                        return false;
                    }
                    break;
                case use_from_both:
                case use_from_none:
                case cancel:
                case num_usages:
                    break;
            }
        } else if( !has_amount( type, 1 ) && !map_inv.has_tools( type, 1 ) ) {
            add_msg_player_or_npc(
                string_format( _( "You no longer have a %s and can't continue crafting" ),
                               item::nname( type ) ),
                string_format( _( "<npcname> no longer has a %s and can't continue crafting" ),
                               item::nname( type ) ) );
            craft.set_tools_to_continue( false );
            return false;
        }
    }

    // We have the selections, so consume them
    for( const comp_selection<tool_comp> &tool : cached_tool_selections ) {
        comp_selection<tool_comp> to_consume = tool;
        to_consume.comp.count = calc_charges( to_consume.comp.count );
        consume_tools( to_consume, 1 );
    }
    return true;
}

void player::consume_tools( const comp_selection<tool_comp> &tool, int batch )
{
    consume_tools( g->m, tool, batch, pos(), PICKUP_RANGE );
}

/* we use this if we selected the tool earlier */
void player::consume_tools( map &m, const comp_selection<tool_comp> &tool, int batch,
                            const tripoint &origin, int radius, basecamp *bcp )
{
    if( has_trait( trait_DEBUG_HS ) ) {
        return;
    }

    const itype *tmp = item::find_type( tool.comp.type );
    int quantity = tool.comp.count * batch * tmp->charge_factor();
    if( tool.use_from & use_from_player ) {
        use_charges( tool.comp.type, quantity );
    }
    if( tool.use_from & use_from_map ) {
        m.use_charges( origin, radius, tool.comp.type, quantity, return_true<item>, bcp );
    }

    // else, use_from_none (or cancel), so we don't use up any tools;
}

/* This call is in-efficient when doing it for multiple items with the same map inventory.
In that case, consider using select_tool_component with 1 pre-created map inventory, and then passing the results
to consume_tools */
void player::consume_tools( const std::vector<tool_comp> &tools, int batch,
                            const std::string &hotkeys )
{
    inventory map_inv;
    map_inv.form_from_map( pos(), PICKUP_RANGE );
    consume_tools( select_tool_component( tools, batch, map_inv, hotkeys ), batch );
}

ret_val<bool> player::can_disassemble( const item &obj, const inventory &inv ) const
{
    const auto &r = recipe_dictionary::get_uncraft( obj.typeId() );

    if( !r || obj.has_flag( "ETHEREAL_ITEM" ) ) {
        return ret_val<bool>::make_failure( _( "You cannot disassemble this." ) );
    }

    // check sufficient light
    if( lighting_craft_speed_multiplier( r ) == 0.0f ) {
        return ret_val<bool>::make_failure( _( "You can't see to craft!" ) );
    }
    // refuse to disassemble rotten items
    if( obj.goes_bad() || ( obj.is_food_container() && obj.contents.front().goes_bad() ) ) {
        if( obj.rotten() || ( obj.is_food_container() && obj.contents.front().rotten() ) ) {
            return ret_val<bool>::make_failure( _( "It's rotten, I'm not taking that apart." ) );
        }
    }

    if( obj.count_by_charges() && !r.has_flag( "UNCRAFT_SINGLE_CHARGE" ) ) {
        // Create a new item to get the default charges
        int qty = r.create_result().charges;
        if( obj.charges < qty ) {
            auto msg = ngettext( "You need at least %d charge of %s.",
                                 "You need at least %d charges of %s.", qty );
            return ret_val<bool>::make_failure( msg, qty, obj.tname() );
        }
    }

    const auto &dis = r.disassembly_requirements();

    for( const auto &opts : dis.get_qualities() ) {
        for( const auto &qual : opts ) {
            if( !qual.has( inv, return_true<item> ) ) {
                // Here should be no dot at the end of the string as 'to_string()' provides it.
                return ret_val<bool>::make_failure( _( "You need %s" ), qual.to_string() );
            }
        }
    }

    for( const auto &opts : dis.get_tools() ) {
        const bool found = std::any_of( opts.begin(), opts.end(),
        [&]( const tool_comp & tool ) {
            return ( tool.count <= 0 && inv.has_tools( tool.type, 1 ) ) ||
                   ( tool.count  > 0 && inv.has_charges( tool.type, tool.count ) );
        } );

        if( !found ) {
            const tool_comp &tool_required = opts.front();
            if( tool_required.count <= 0 ) {
                return ret_val<bool>::make_failure( _( "You need %s." ),
                                                    item::nname( tool_required.type ) );
            } else {
                return ret_val<bool>::make_failure( ngettext( "You need a %s with %d charge.",
                                                    "You need a %s with %d charges.", tool_required.count ),
                                                    item::nname( tool_required.type ),
                                                    tool_required.count );
            }
        }
    }

    return ret_val<bool>::make_success();
}

bool player::disassemble()
{
    return disassemble( game_menus::inv::disassemble( *this ), false );
}

bool player::disassemble( item_location target, bool interactive )
{
    if( !target ) {
        add_msg( _( "Never mind." ) );
        return false;
    }

    const item &obj = *target;
    const auto ret = can_disassemble( obj, crafting_inventory() );

    if( !ret.success() ) {
        if( interactive ) {
            add_msg_if_player( m_info, "%s", ret.c_str() );
        }
        return false;
    }

    const auto &r = recipe_dictionary::get_uncraft( obj.typeId() );
    // last chance to back out
    if( interactive && get_option<bool>( "QUERY_DISASSEMBLE" ) ) {
        std::ostringstream list;
        const auto components = obj.get_uncraft_components();
        for( const auto &component : components ) {
            list << "- " << component.to_string() << std::endl;
        }

        if( !r.learn_by_disassembly.empty() && !knows_recipe( &r ) && can_decomp_learn( r ) ) {
            if( !query_yn(
                    _( "Disassembling the %s may yield:\n%s\nReally disassemble?\nYou feel you may be able to understand this object's construction.\n" ),
                    colorize( obj.tname(), obj.color_in_inventory() ),
                    list.str() ) ) {
                return false;
            }
        } else if( !query_yn( _( "Disassembling the %s may yield:\n%s\nReally disassemble?" ),
                              colorize( obj.tname(), obj.color_in_inventory() ),
                              list.str() ) ) {
            return false;
        }
    }

    if( activity.id() != activity_id( "ACT_DISASSEMBLE" ) ) {
        assign_activity( activity_id( "ACT_DISASSEMBLE" ), r.time );
    } else if( activity.moves_left <= 0 ) {
        activity.moves_left = r.time;
    }

    // index is used as a bool that indicates if we want recursive uncraft.
    activity.index = false;
    activity.targets.emplace_back( std::move( target ) );
    activity.str_values.push_back( r.result() );

    return true;
}

void player::disassemble_all( bool one_pass )
{
    // Reset all the activity values
    assign_activity( activity_id( "ACT_DISASSEMBLE" ), 0 );

    bool found_any = false;
    for( item &it : g->m.i_at( pos() ) ) {
        if( disassemble( item_location( map_cursor( pos() ), &it ), false ) ) {
            found_any = true;
        }
    }

    // index is used as a bool that indicates if we want recursive uncraft.
    // Upon calling complete_disassemble, if we have no targets left,
    // we will call this function again.
    activity.index = !one_pass;

    if( !found_any ) {
        // Reset the activity - don't loop if there is nothing to do
        activity = player_activity();
    }
}

void player::complete_disassemble()
{
    // Cancel the activity if we have bad (possibly legacy) values
    if( activity.targets.empty() || !activity.targets.back() ||
        activity.targets.size() != activity.str_values.size() ) {
        debugmsg( "bad disassembly activity values" );
        activity.set_to_null();
        return;
    }

    // Disassembly has 2 parallel vectors:
    // item location, and recipe id
    const recipe &rec = recipe_dictionary::get_uncraft( activity.str_values.back() );

    if( rec ) {
        complete_disassemble( activity.targets.back(), rec );
    } else {
        debugmsg( "bad disassembly recipe: %d", activity.str_values.back() );
        activity.set_to_null();
        return;
    }

    activity.targets.pop_back();
    activity.str_values.pop_back();

    // If we have no more targets end the activity or start a second round
    if( activity.targets.empty() ) {
        // index is used as a bool that indicates if we want recursive uncraft.
        if( activity.index ) {
            disassemble_all( false );
            return;
        } else {
            // No more targets
            activity.set_to_null();
            return;
        }
    }

    // Set get and set duration of next uncraft
    const recipe &next_recipe = recipe_dictionary::get_uncraft( activity.str_values.back() );

    if( !next_recipe ) {
        debugmsg( "bad disassembly recipe: %d", activity.str_values.back() );
        activity.set_to_null();
        return;
    }

    activity.moves_left = next_recipe.time;
}

void player::complete_disassemble( item_location &target, const recipe &dis )
{
    // Get the proper recipe - the one for disassembly, not assembly
    const auto dis_requirements = dis.disassembly_requirements();
    item &org_item = *target;
    const bool filthy = org_item.is_filthy();
    const tripoint loc = target.position();

    // Make a copy to keep its data (damage/components) even after it
    // has been removed.
    item dis_item = org_item;

    float component_success_chance = std::min( std::pow( 0.8, dis_item.damage_level( 4 ) ), 1.0 );

    add_msg( _( "You disassemble the %s into its components." ), dis_item.tname() );
    // Remove any batteries, ammo and mods first
    remove_ammo( dis_item, *this );
    remove_radio_mod( dis_item, *this );

    if( dis_item.count_by_charges() ) {
        // remove the charges that one would get from crafting it
        org_item.charges -= dis.create_result().charges;
    }
    // remove the item, except when it's counted by charges and still has some
    if( !org_item.count_by_charges() || org_item.charges <= 0 ) {
        target.remove_item();
    }

    // Consume tool charges
    for( const auto &it : dis_requirements.get_tools() ) {
        consume_tools( it );
    }

    // add the components to the map
    // Player skills should determine how many components are returned

    int skill_dice = 2 + get_skill_level( dis.skill_used ) * 3;
    skill_dice += get_skill_level( dis.skill_used );

    // Sides on dice is 16 plus your current intelligence
    ///\EFFECT_INT increases success rate for disassembling items
    int skill_sides = 16 + int_cur;

    int diff_dice = dis.difficulty;
    int diff_sides = 24; // 16 + 8 (default intelligence)

    // disassembly only nets a bit of practice
    if( dis.skill_used ) {
        practice( dis.skill_used, ( dis.difficulty ) * 2, dis.difficulty );
    }

    // If the components aren't empty, we want items exactly identical to them
    // Even if the best-fit recipe does not involve those items
    std::list<item> components = dis_item.components;
    // If the components are empty, item is the default kind and made of default components
    if( components.empty() ) {
        const bool uncraft_liquids_contained = dis.has_flag( "UNCRAFT_LIQUIDS_CONTAINED" );
        for( const auto &altercomps : dis_requirements.get_components() ) {
            const item_comp &comp = altercomps.front();
            int compcount = comp.count;
            item newit( comp.type, calendar::turn );
            // Counted-by-charge items that can be disassembled individually
            // have their component count multiplied by the number of charges.
            if( dis_item.count_by_charges() && dis.has_flag( "UNCRAFT_SINGLE_CHARGE" ) ) {
                compcount *= std::min( dis_item.charges, dis.create_result().charges );
            }
            const bool is_liquid = newit.made_of( LIQUID );
            if( uncraft_liquids_contained && is_liquid && newit.charges != 0 ) {
                // Spawn liquid item in its default container
                compcount = compcount / newit.charges;
                if( compcount != 0 ) {
                    newit = newit.in_its_container();
                }
            } else {
                // Compress liquids and counted-by-charges items into one item,
                // they are added together on the map anyway and handle_liquid
                // should only be called once to put it all into a container at once.
                if( newit.count_by_charges() || is_liquid ) {
                    newit.charges = compcount;
                    compcount = 1;
                } else if( !newit.craft_has_charges() && newit.charges > 0 ) {
                    // tools that can be unloaded should be created unloaded,
                    // tools that can't be unloaded will keep their default charges.
                    newit.charges = 0;
                }
            }

            // If the recipe has a `FULL_MAGAZINE` flag, spawn any magazines full of ammo
            if( newit.is_magazine() && dis.has_flag( "FULL_MAGAZINE" ) ) {
                newit.ammo_set( newit.ammo_default(), newit.ammo_capacity() );
            }

            for( ; compcount > 0; compcount-- ) {
                components.emplace_back( newit );
            }
        }
    }

    std::list<item> drop_items;

    for( const item &newit : components ) {
        const bool comp_success = ( dice( skill_dice, skill_sides ) > dice( diff_dice,  diff_sides ) );
        if( dis.difficulty != 0 && !comp_success ) {
            add_msg( m_bad, _( "You fail to recover %s." ), newit.tname() );
            continue;
        }
        const bool dmg_success = component_success_chance > rng_float( 0, 1 );
        if( !dmg_success ) {
            // Show reason for failure (damaged item, tname contains the damage adjective)
            //~ %1s - material, %2$s - disassembled item
            add_msg( m_bad, _( "You fail to recover %1$s from the %2$s." ), newit.tname(),
                     dis_item.tname() );
            continue;
        }
        // Use item from components list, or (if not contained)
        // use newit, the default constructed.
        item act_item = newit;

        if( act_item.has_temperature() ) {
            act_item.set_item_temperature( temp_to_kelvin( g->weather.get_temperature( loc ) ) );
        }

        // Refitted clothing disassembles into refitted components (when applicable)
        if( dis_item.has_flag( "FIT" ) && act_item.has_flag( "VARSIZE" ) ) {
            act_item.item_tags.insert( "FIT" );
        }

        if( filthy ) {
            act_item.item_tags.insert( "FILTHY" );
        }

        for( std::list<item>::iterator a = dis_item.components.begin(); a != dis_item.components.end();
             ++a ) {
            if( a->type == newit.type ) {
                act_item = *a;
                dis_item.components.erase( a );
                break;
            }
        }

        if( act_item.made_of( LIQUID ) ) {
            liquid_handler::handle_all_liquid( act_item, PICKUP_RANGE );
        } else {
            drop_items.push_back( act_item );
        }
    }

    put_into_vehicle_or_drop( *this, item_drop_reason::deliberate, drop_items );

    if( !dis.learn_by_disassembly.empty() && !knows_recipe( &dis ) ) {
        if( can_decomp_learn( dis ) ) {
            // TODO: make this depend on intelligence
            if( one_in( 4 ) ) {
                // TODO: change to forward an id or a reference
                learn_recipe( &dis.ident().obj() );
                add_msg( m_good, _( "You learned a recipe for %s from disassembling it!" ),
                         dis_item.tname() );
            } else {
                add_msg( m_info, _( "You might be able to learn a recipe for %s if you disassemble another." ),
                         dis_item.tname() );
            }
        } else {
            add_msg( m_info, _( "If you had better skills, you might learn a recipe next time." ) );
        }
    }
}

void remove_ammo( std::list<item> &dis_items, player &p )
{
    for( auto &dis_item : dis_items ) {
        remove_ammo( dis_item, p );
    }
}

void drop_or_handle( const item &newit, player &p )
{
    if( newit.made_of( LIQUID ) && p.is_player() ) { // TODO: what about NPCs?
        liquid_handler::handle_all_liquid( newit, PICKUP_RANGE );
    } else {
        item tmp( newit );
        p.i_add_or_drop( tmp );
    }
}

void remove_ammo( item &dis_item, player &p )
{
    for( auto iter = dis_item.contents.begin(); iter != dis_item.contents.end(); ) {
        if( iter->is_irremovable() ) {
            iter++;
            continue;
        }
        drop_or_handle( *iter, p );
        iter = dis_item.contents.erase( iter );
    }

    if( dis_item.has_flag( "NO_UNLOAD" ) ) {
        return;
    }
    if( dis_item.is_gun() && dis_item.ammo_current() != "null" ) {
        item ammodrop( dis_item.ammo_current(), calendar::turn );
        ammodrop.charges = dis_item.charges;
        drop_or_handle( ammodrop, p );
        dis_item.charges = 0;
    }
    if( dis_item.is_tool() && dis_item.charges > 0 && dis_item.ammo_current() != "null" ) {
        item ammodrop( dis_item.ammo_current(), calendar::turn );
        ammodrop.charges = dis_item.charges;
        if( dis_item.ammo_current() == "plut_cell" ) {
            ammodrop.charges /= PLUTONIUM_CHARGES;
        }
        drop_or_handle( ammodrop, p );
        dis_item.charges = 0;
    }
}

std::vector<npc *> player::get_crafting_helpers() const
{
    return g->get_npcs_if( [this]( const npc & guy ) {
        // NPCs can help craft if awake, taking orders, within pickup range and have clear path
        return !guy.in_sleep_state() && guy.is_obeying( *this ) &&
               rl_dist( guy.pos(), pos() ) < PICKUP_RANGE &&
               g->m.clear_path( pos(), guy.pos(), PICKUP_RANGE, 1, 100 );
    } );
}
