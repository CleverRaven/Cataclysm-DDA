#include "crafting.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iosfwd>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "avatar.h"
#include "bionics.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "character.h"
#include "colony.h"
#include "color.h"
#include "craft_command.h"
#include "crafting_gui.h"
#include "debug.h"
#include "enum_traits.h"
#include "enums.h"
#include "faction.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "handle_liquid.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "item_stack.h"
#include "itype.h"
#include "iuse.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapdata.h"
#include "messages.h"
#include "morale_types.h"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "pimpl.h"
#include "player_activity.h"
#include "point.h"
#include "proficiency.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "visitable.h"
#include "vpart_position.h"
#include "weather.h"

static const activity_id ACT_CRAFT( "ACT_CRAFT" );
static const activity_id ACT_DISASSEMBLE( "ACT_DISASSEMBLE" );
static const activity_id ACT_MULTIPLE_CRAFT( "ACT_MULTIPLE_CRAFT" );

static const efftype_id effect_contacts( "contacts" );

static const itype_id itype_disassembly( "disassembly" );
static const itype_id itype_plut_cell( "plut_cell" );

static const json_character_flag json_flag_HYPEROPIC( "HYPEROPIC" );

static const limb_score_id limb_score_manip( "manip" );

static const quality_id qual_BOIL( "BOIL" );

static const skill_id skill_electronics( "electronics" );
static const skill_id skill_tailor( "tailor" );

static const string_id<struct furn_t> furn_f_fake_bench_hands( "f_fake_bench_hands" );

static const string_id<struct furn_t> furn_f_ground_crafting_spot( "f_ground_crafting_spot" );

static const trait_id trait_BURROW( "BURROW" );
static const trait_id trait_BURROWLARGE( "BURROWLARGE" );
static const trait_id trait_DEBUG_CNF( "DEBUG_CNF" );
static const trait_id trait_DEBUG_HS( "DEBUG_HS" );
static const trait_id trait_INT_ALPHA( "INT_ALPHA" );

static const std::string flag_BLIND_EASY( "BLIND_EASY" );
static const std::string flag_BLIND_HARD( "BLIND_HARD" );
static const std::string flag_FULL_MAGAZINE( "FULL_MAGAZINE" );
static const std::string flag_NO_RESIZE( "NO_RESIZE" );

class basecamp;

static bool crafting_allowed( const Character &p, const recipe &rec )
{
    if( p.morale_crafting_speed_multiplier( rec ) <= 0.0f ) {
        add_msg( m_info, _( "Your morale is too low to craft such a difficult thing…" ) );
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

float Character::lighting_craft_speed_multiplier( const recipe &rec, const tripoint &p ) const
{
    // negative is bright, 0 is just bright enough, positive is dark, +7.0f is pitch black
    float darkness = fine_detail_vision_mod( p ) - 4.0f;
    if( darkness <= 0.0f ) {
        return 1.0f; // it's bright, go for it
    }
    bool rec_blind = rec.has_flag( flag_BLIND_HARD ) || rec.has_flag( flag_BLIND_EASY );
    if( darkness > 0 && !rec_blind ) {
        return 0.0f; // it's dark and this recipe can't be crafted in the dark
    }
    if( rec.has_flag( flag_BLIND_EASY ) ) {
        // 100% speed in well lit area at skill+0
        // 25% speed in pitch black at skill+0
        // skill+2 removes speed penalty
        if( rec.skill_used ) {
            return 1.0f - ( darkness / ( 7.0f / 0.75f ) ) * std::max( 0,
                    2 - exceeds_recipe_requirements( rec ) ) / 2.0f;
        } else { // If there's no skill involved there shouldn't be any speed reduction due to not having levels in a non existent skill.
            return 1.0f;
        }
    }
    if( rec.has_flag( flag_BLIND_HARD ) && ( !rec.skill_used ||
            exceeds_recipe_requirements( rec ) >= 2 ) ) {
        // 100% speed in well lit area at skill+2
        // 25% speed in pitch black at skill+2
        // skill+8 removes speed penalty
        if( rec.skill_used ) {
            return 1.0f - ( darkness / ( 7.0f / 0.75f ) ) * std::max( 0,
                    8 - exceeds_recipe_requirements( rec ) ) / 6.0f;
        } else { // No skill involved, so apply arbitrary penalty.
            return 0.5f;
        }
    }
    return 0.0f; // it's dark and you could craft this if you had more skill
}

float Character::morale_crafting_speed_multiplier( const recipe &rec ) const
{
    int morale = get_morale_level();
    if( morale >= 0 ) {
        // No bonus for being happy yet
        return 1.0f;
    }

    // Harder jobs are more frustrating, even when skilled
    // For each skill where skill=difficulty, multiply effective morale by 200%
    float morale_mult = std::max( 1.0f, 2.0f * rec.difficulty / std::max( 1.0f,
                                  get_skill_level( rec.skill_used ) ) );
    for( const auto &pr : rec.required_skills ) {
        morale_mult *= std::max( 1.0f, 2.0f * pr.second / std::max( 1.0f, get_skill_level( pr.first ) ) );
    }

    // Halve speed at -50 effective morale, quarter at -150
    float morale_effect = 1.0f + ( morale_mult * morale ) / -50.0f;

    add_msg_debug( debugmode::DF_CHARACTER, "Morale multiplier %.1f", 1.0f / morale_effect );
    return 1.0f / morale_effect;
}

template<typename T>
static float lerped_multiplier( const T &value, const T &low, const T &high )
{
    // No effect if less than allowed value
    if( value <= low ) {
        return 1.0f;
    }
    // Bottom out at 25% speed
    if( value >= high ) {
        return 0.25f;
    }
    // Linear interpolation between high and low
    // y = y0 + ( x - x0 ) * ( y1 - y0 ) / ( x1 - x0 )
    return 1.0f + ( value - low ) * ( 0.25f - 1.0f ) / ( high - low );
}

float Character::workbench_crafting_speed_multiplier( const item &craft,
        const std::optional<tripoint> &loc )const
{
    float multiplier;
    units::mass allowed_mass;
    units::volume allowed_volume;

    map &here = get_map();
    if( !loc ) {
        // std::nullopt indicates crafting from inventory
        // Use values from f_fake_bench_hands
        const furn_t &f = furn_f_fake_bench_hands.obj();
        multiplier = f.workbench->multiplier;
        allowed_mass = f.workbench->allowed_mass;
        allowed_volume = f.workbench->allowed_volume;
    } else if( const std::optional<vpart_reference> vp = here.veh_at(
                   *loc ).part_with_feature( "WORKBENCH", true ) ) {
        // Vehicle workbench
        const vpart_info &vp_info = vp->part().info();
        if( const std::optional<vpslot_workbench> &wb_info = vp_info.workbench_info ) {
            multiplier = wb_info->multiplier;
            allowed_mass = wb_info->allowed_mass;
            allowed_volume = wb_info->allowed_volume;
        } else {
            debugmsg( "part '%S' with WORKBENCH flag has no workbench info", vp->part().name() );
            return 0.0f;
        }
    } else if( here.furn( *loc ).obj().workbench ) {
        // Furniture workbench
        const furn_t &f = here.furn( *loc ).obj();
        multiplier = f.workbench->multiplier;
        allowed_mass = f.workbench->allowed_mass;
        allowed_volume = f.workbench->allowed_volume;
    } else {
        // Ground
        const furn_t &f = furn_f_ground_crafting_spot.obj();
        multiplier = f.workbench->multiplier;
        allowed_mass = f.workbench->allowed_mass;
        allowed_volume = f.workbench->allowed_volume;
    }

    const units::mass &craft_mass = craft.weight();
    const units::volume &craft_volume = craft.volume();

    units::mass lifting_mass = best_nearby_lifting_assist();

    if( lifting_mass > craft_mass ) {
        return multiplier;
    }

    multiplier *= lerped_multiplier( craft_mass, allowed_mass, 1000_kilogram );
    multiplier *= lerped_multiplier( craft_volume, allowed_volume, 1000_liter );

    return multiplier;
}

float Character::crafting_speed_multiplier( const recipe &rec ) const
{
    const float result = morale_crafting_speed_multiplier( rec ) *
                         lighting_craft_speed_multiplier( rec ) *
                         get_limb_score( limb_score_manip );
    add_msg_debug( debugmode::DF_CHARACTER, "Limb score multiplier %.1f, crafting speed multiplier %1f",
                   get_limb_score( limb_score_manip ), result );

    return std::max( result, 0.0f );
}

float Character::crafting_speed_multiplier( const item &craft,
        const std::optional<tripoint> &loc, bool use_cached_workbench_multiplier,
        float cached_workbench_multiplier ) const
{
    if( !craft.is_craft() ) {
        debugmsg( "Can't calculate crafting speed multiplier of non-craft '%s'", craft.tname() );
        return 1.0f;
    }

    const recipe &rec = craft.get_making();

    const float light_multi = lighting_craft_speed_multiplier( rec );
    const float bench_multi = ( use_cached_workbench_multiplier ||
                                cached_workbench_multiplier > 0.0f ) ? cached_workbench_multiplier :
                              workbench_crafting_speed_multiplier( craft, loc );
    const float morale_multi = morale_crafting_speed_multiplier( rec );
    const float mut_multi = mutation_value( "crafting_speed_multiplier" );

    const float total_multi = light_multi * bench_multi * morale_multi * mut_multi *
                              get_limb_score( limb_score_manip );

    if( light_multi <= 0.0f ) {
        add_msg_if_player( m_bad, _( "You can no longer see well enough to keep crafting." ) );
        return 0.0f;
    }
    if( bench_multi <= 0.1f || ( bench_multi <= 0.33f && total_multi <= 0.2f ) ) {
        add_msg_if_player( m_bad, _( "The %s is too large and/or heavy to work on.  You may want to"
                                     " use a workbench or a lifting tool." ), craft.tname() );
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
                               _( "The %s is too large and/or heavy to work on comfortably.  You are"
                                  " working slowly." ), craft.tname() );
        }
        if( morale_multi <= 0.5f ) {
            add_msg_if_player( m_bad, _( "You can't focus and are working slowly." ) );
        }
    }

    return total_multi;
}

bool Character::has_morale_to_craft() const
{
    return get_morale_level() >= -50;
}

void Character::craft( const std::optional<tripoint> &loc, const recipe_id &goto_recipe )
{
    int batch_size = 0;
    const recipe *rec = select_crafting_recipe( batch_size, goto_recipe, *this );
    if( rec ) {
        std::string reason;
        if( is_npc() && !rec->npc_can_craft( reason ) ) {
            add_msg( m_info, reason );
            return;
        }
        if( crafting_allowed( *this, *rec ) ) {
            make_craft( rec->ident(), batch_size, loc );
        }
    }
}

void Character::recraft( const std::optional<tripoint> &loc )
{
    if( lastrecipe.str().empty() ) {
        popup( _( "Craft something first" ) );
    } else if( making_would_work( lastrecipe, last_batch ) ) {
        last_craft->execute( loc );
    }
}

void Character::long_craft( const std::optional<tripoint> &loc, const recipe_id &goto_recipe )
{
    int batch_size = 0;
    const recipe *rec = select_crafting_recipe( batch_size, goto_recipe, *this );
    if( rec ) {
        if( crafting_allowed( *this, *rec ) ) {
            make_all_craft( rec->ident(), batch_size, loc );
        }
    }
}

bool Character::making_would_work( const recipe_id &id_to_make, int batch_size ) const
{
    const recipe &making = *id_to_make;
    if( !( making && crafting_allowed( *this, making ) ) ) {
        return false;
    }

    if( !can_make( &making, batch_size ) ) {
        if( is_avatar() ) {
            std::string buffer = _( "You can no longer make that craft!" );
            buffer += "\n";
            buffer += making.simple_requirements().list_missing();
            popup( buffer, PF_NONE );
        } else {
            add_msg_if_npc( _( "<npcname> can no longer make that craft!" ) );
        }
        return false;
    }

    return check_eligible_containers_for_crafting( making, batch_size );
}

int Character::available_assistant_count( const recipe &rec ) const
// NPCs around you should assist in batch production if they have the skills
{
    if( rec.is_practice() ) {
        return 0;
    }
    // TODO: Cache them in activity, include them in modifier calculations
    const auto helpers = get_crafting_helpers();
    return std::count_if( helpers.begin(), helpers.end(),
    [&]( const npc * np ) {
        return np->get_skill_level( rec.skill_used ) >= rec.difficulty;
    } );
}

int64_t Character::expected_time_to_craft( const recipe &rec, int batch_size ) const
{
    const size_t assistants = available_assistant_count( rec );
    float modifier = crafting_speed_multiplier( rec );
    return rec.batch_time( *this, batch_size, modifier, assistants );
}

bool Character::check_eligible_containers_for_crafting( const recipe &rec, int batch_size ) const
{
    std::vector<const item *> conts = get_eligible_containers_for_crafting();
    const std::vector<item> res = rec.create_results( batch_size );
    const std::vector<item> bps = rec.create_byproducts( batch_size );
    std::vector<item> all;
    all.reserve( res.size() + bps.size() );
    all.insert( all.end(), res.begin(), res.end() );
    all.insert( all.end(), bps.begin(), bps.end() );

    map &here = get_map();
    for( const item &prod : all ) {
        if( !prod.made_of( phase_id::LIQUID ) ) {
            continue;
        }

        // we go through half-filled containers first, then go through empty containers if we need
        std::sort( conts.begin(), conts.end(), item_ptr_compare_by_charges );

        int charges_to_store = prod.charges;
        for( const item *cont : conts ) {
            if( charges_to_store <= 0 ) {
                break;
            }

            charges_to_store -= cont->get_remaining_capacity_for_liquid( prod, true );
        }

        // also check if we're currently in a vehicle that has the necessary storage
        if( charges_to_store > 0 ) {
            if( optional_vpart_position vp = here.veh_at( pos() ) ) {
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
            if( !query_yn(
                    _( "You don't have anything in which to store %s and may have to pour it out as soon as it is prepared!  Proceed?" ),
                    prod.tname() ) ) {
                return false;
            }
        }
    }

    return true;
}

static bool is_container_eligible_for_crafting( const item &cont, bool allow_bucket )
{
    if( cont.is_watertight_container() && cont.num_item_stacks() <= 1 && ( allow_bucket ||
            !cont.will_spill() ) ) {
        return !cont.is_container_full( allow_bucket );
    }
    return false;
}

static std::vector<const item *> get_eligible_containers_recursive( const item &cont,
        bool allow_bucket )
{
    std::vector<const item *> ret;

    if( is_container_eligible_for_crafting( cont, allow_bucket ) ) {
        ret.push_back( &cont );
    }
    for( const item *it : cont.all_items_top( item_pocket::pocket_type::CONTAINER ) ) {
        //buckets are never allowed when inside another container
        std::vector<const item *> inside = get_eligible_containers_recursive( *it, false );
        ret.insert( ret.end(), inside.begin(), inside.end() );
    }
    return ret;
}

void outfit::get_eligible_containers_for_crafting( std::vector<const item *> &conts ) const
{
    for( const item &it : worn ) {
        std::vector<const item *> eligible = get_eligible_containers_recursive( it, false );
        conts.insert( conts.begin(), eligible.begin(), eligible.end() );
    }
}

std::vector<const item *> Character::get_eligible_containers_for_crafting() const
{
    std::vector<const item *> conts;
    const item_location weapon = get_wielded_item();
    if( weapon ) {
        conts = get_eligible_containers_recursive( *weapon, true );
    }

    worn.get_eligible_containers_for_crafting( conts );

    map &here = get_map();
    // get all potential containers within PICKUP_RANGE tiles including vehicles
    for( const tripoint &loc : closest_points_first( pos(), PICKUP_RANGE ) ) {
        // can not reach this -> can not access its contents
        if( pos() != loc && !here.clear_path( pos(), loc, PICKUP_RANGE, 1, 100 ) ) {
            continue;
        }
        if( here.accessible_items( loc ) ) {
            for( const item &it : here.i_at( loc ) ) {
                std::vector<const item *> eligible = get_eligible_containers_recursive( it, true );
                conts.insert( conts.begin(), eligible.begin(), eligible.end() );
            }
        }

        if( const std::optional<vpart_reference> vp = here.veh_at( loc ).cargo() ) {
            for( const item &it : vp->items() ) {
                std::vector<const item *> eligible = get_eligible_containers_recursive( it, true );
                conts.insert( conts.begin(), eligible.begin(), eligible.end() );
            }
        }
    }

    return conts;
}

bool Character::can_make( const recipe *r, int batch_size ) const
{
    const inventory &crafting_inv = crafting_inventory();

    if( !has_recipe( r, crafting_inv, get_crafting_helpers() ) ) {
        return false;
    }

    if( !r->character_has_required_proficiencies( *this ) ) {
        return false;
    }

    return r->deduped_requirements().can_make_with_inventory(
               crafting_inv, r->get_component_filter(), batch_size );
}

bool Character::can_start_craft( const recipe *rec, recipe_filter_flags flags,
                                 int batch_size ) const
{
    if( !rec ) {
        return false;
    }

    if( !rec->character_has_required_proficiencies( *this ) ) {
        return false;
    }

    const inventory &inv = crafting_inventory();
    return rec->deduped_requirements().can_make_with_inventory(
               inv, rec->get_component_filter( flags ), batch_size, craft_flags::start_only );
}

const inventory &Character::crafting_inventory( bool clear_path ) const
{
    return crafting_inventory( tripoint_zero, PICKUP_RANGE, clear_path );
}

const inventory &Character::crafting_inventory( const tripoint &src_pos, int radius,
        bool clear_path ) const
{
    tripoint inv_pos = src_pos;
    if( src_pos == tripoint_zero ) {
        inv_pos = pos();
    }
    if( crafting_cache.valid
        && moves == crafting_cache.moves
        && radius == crafting_cache.radius
        && calendar::turn == crafting_cache.time
        && inv_pos == crafting_cache.position ) {
        return *crafting_cache.crafting_inventory;
    }
    crafting_cache.crafting_inventory->clear();
    if( radius >= 0 ) {
        crafting_cache.crafting_inventory->form_from_map( inv_pos, radius, this, false, clear_path );
    }

    std::map<itype_id, int> tmp_liq_list;
    // TODO: Add a const overload of all_items_loc() that returns something like
    // vector<const_item_location> in order to get rid of the const_cast here.
    for( const item_location &it : const_cast<Character *>( this )->all_items_loc() ) {
        // add containers separately from their contents
        if( !it->empty_container() ) {
            // is the non-empty container used for BOIL?
            if( !it->is_watertight_container() || it->get_quality( qual_BOIL, false ) <= 0 ) {
                item tmp = item( it->typeId(), it->birthday() );
                tmp.is_favorite = it->is_favorite;
                *crafting_cache.crafting_inventory += tmp;
            }
            continue;
        } else if( it->is_watertight_container() ) {
            const int count = it->count_by_charges() ? it->charges : 1;
            tmp_liq_list[it->typeId()] += count;
        }
        crafting_cache.crafting_inventory->add_item( *it );
    }
    crafting_cache.crafting_inventory->replace_liq_container_count( tmp_liq_list, true );

    for( const item *i : get_pseudo_items() ) {
        *crafting_cache.crafting_inventory += *i;
    }

    if( has_trait( trait_BURROW ) || has_trait( trait_BURROWLARGE ) ) {
        *crafting_cache.crafting_inventory += item( "pickaxe", calendar::turn );
        *crafting_cache.crafting_inventory += item( "shovel", calendar::turn );
    }

    crafting_cache.valid = true;
    crafting_cache.moves = moves;
    crafting_cache.time = calendar::turn;
    crafting_cache.position = inv_pos;
    crafting_cache.radius = radius;
    return *crafting_cache.crafting_inventory;
}

void Character::invalidate_crafting_inventory()
{
    crafting_cache.valid = false;
    crafting_cache.crafting_inventory->clear();
}

void Character::make_craft( const recipe_id &id_to_make, int batch_size,
                            const std::optional<tripoint> &loc )
{
    make_craft_with_command( id_to_make, batch_size, false, loc );
}

void Character::make_all_craft( const recipe_id &id_to_make, int batch_size,
                                const std::optional<tripoint> &loc )
{
    make_craft_with_command( id_to_make, batch_size, true, loc );
}

void Character::make_craft_with_command( const recipe_id &id_to_make, int batch_size, bool is_long,
        const std::optional<tripoint> &loc )
{
    const recipe &recipe_to_make = *id_to_make;

    if( !recipe_to_make ) {
        return;
    }

    *last_craft = craft_command( &recipe_to_make, batch_size, is_long, this, loc );
    last_craft->execute();
}

static std::optional<item_location> wield_craft( Character &p, item &craft )
{
    if( p.wield( craft ) ) {
        item_location weapon = p.get_wielded_item();
        if( weapon->invlet ) {
            p.add_msg_if_player( m_info, _( "Wielding %c - %s" ), weapon->invlet, weapon->display_name() );
        } else {
            p.add_msg_if_player( m_info, _( "Wielding - %s" ), weapon->display_name() );
        }
        return weapon;
    }
    return std::nullopt;
}

static item_location set_item_inventory( Character &p, item &newit )
{
    item_location ret_val = item_location::nowhere;
    if( newit.made_of( phase_id::LIQUID ) ) {
        liquid_handler::handle_all_liquid( newit, PICKUP_RANGE );
    } else {
        p.inv->assign_empty_invlet( newit, p );
        // We might not have space for the item
        if( !p.can_pickVolume( newit ) ) { //Accounts for result_mult
            put_into_vehicle_or_drop( p, item_drop_reason::too_large, { newit } );
        } else if( !p.can_pickWeight( newit, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) ) {
            put_into_vehicle_or_drop( p, item_drop_reason::too_heavy, { newit } );
        } else {
            ret_val = p.i_add( newit );
            add_msg( m_info, "%c - %s", ret_val->invlet == 0 ? ' ' : ret_val->invlet,
                     ret_val->tname() );
        }
    }
    return ret_val;
}

/**
 * Helper for @ref set_item_map_or_vehicle
 * This is needed to still get a valid item_location if overflow occurs
 */
static item_location set_item_map( const tripoint &loc, item &newit )
{
    // Includes loc
    for( const tripoint &tile : closest_points_first( loc, 2 ) ) {
        // Pass false to disallow overflow, null_item_reference indicates failure.
        item *it_on_map = &get_map().add_item_or_charges( tile, newit, false );
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
static item_location set_item_map_or_vehicle( const Character &p, const tripoint &loc, item &newit )
{
    map &here = get_map();
    if( const std::optional<vpart_reference> vp = here.veh_at( loc ).cargo() ) {
        vehicle &veh = vp->vehicle();
        if( const std::optional<vehicle_stack::iterator> it = veh.add_item( vp->part(), newit ) ) {
            p.add_msg_player_or_npc(
                //~ %1$s: name of item being placed, %2$s: vehicle part name
                pgettext( "item, furniture", "You put the %1$s on the %2$s." ),
                pgettext( "item, furniture", "<npcname> puts the %1$s on the %2$s." ),
                ( *it )->tname(), vp->part().name() );

            return item_location( vehicle_cursor( veh, vp->part_index() ), & **it );
        }

        // Couldn't add the in progress craft to the target part, so drop it to the map.
        p.add_msg_player_or_npc(
            //~ %1$s: vehicle part name, %2$s: name of the item being placed
            pgettext( "furniture, item", "Not enough space on the %1$s. You drop the %2$s on the ground." ),
            pgettext( "furniture, item",
                      "Not enough space on the %1$s. <npcname> drops the %2$s on the ground." ),
            vp->part().name(), newit.tname() );

        return set_item_map( loc, newit );

    } else {
        if( here.has_furn( loc ) ) {
            const furn_t &workbench = here.furn( loc ).obj();
            p.add_msg_player_or_npc(
                //~ %1$s: name of item being placed, %2$s: vehicle part name
                pgettext( "item, furniture", "You put the %1$s on the %2$s." ),
                pgettext( "item, furniture", "<npcname> puts the %1$s on the %2$s." ),
                newit.tname(), workbench.name() );
        } else {
            p.add_msg_player_or_npc(
                pgettext( "item", "You put the %s on the ground." ),
                pgettext( "item", "<npcname> puts the %s on the ground." ),
                newit.tname() );
        }
        return set_item_map( loc, newit );
    }
}

static item_location place_craft_or_disassembly(
    Character &ch, item &craft, std::optional<tripoint> target )
{
    item_location craft_in_world;

    // Check if we are standing next to a workbench. If so, just use that.
    float best_bench_multi = 0.0f;
    map &here = get_map();
    for( const tripoint &adj : here.points_in_radius( ch.pos(), 1 ) ) {
        if( here.dangerous_field_at( adj ) ) {
            continue;
        }
        if( const cata::value_ptr<furn_workbench_info> &wb = here.furn( adj ).obj().workbench ) {
            if( wb->multiplier > best_bench_multi ) {
                best_bench_multi = wb->multiplier;
                target = adj;
            }
        } else if( const std::optional<vpart_reference> vp = here.veh_at(
                       adj ).part_with_feature( "WORKBENCH", true ) ) {
            if( const std::optional<vpslot_workbench> &wb_info = vp->part().info().workbench_info ) {
                if( wb_info->multiplier > best_bench_multi ) {
                    best_bench_multi = wb_info->multiplier;
                    target = adj;
                }
            } else {
                debugmsg( "part '%s' with WORKBENCH flag has no workbench info", vp->part().name() );
            }
        }
    }

    // Crafting without a workbench
    if( !target ) {
        if( !ch.has_two_arms_lifting() ) {
            craft_in_world = set_item_map_or_vehicle( ch, ch.pos(), craft );
        } else if( !ch.has_wield_conflicts( craft ) || ch.is_npc() ) {
            // NPC always tries wield craft first
            if( std::optional<item_location> it_loc = wield_craft( ch, craft ) ) {
                craft_in_world = *it_loc;
            }  else {
                // This almost certainly shouldn't happen
                put_into_vehicle_or_drop( ch, item_drop_reason::tumbling, {craft} );
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

            amenu.addentry( WIELD_CRAFT, ch.can_unwield( *ch.get_wielded_item() ).success(),
                            '1', _( "Dispose of your wielded %s and start working." ), ch.get_wielded_item()->tname() );
            amenu.addentry( DROP_CRAFT, true, '2', _( "Put it down and start working." ) );
            const bool can_stash = ch.can_pickVolume( craft ) &&
                                   ch.can_pickWeight( craft, !get_option<bool>( "DANGEROUS_PICKUPS" ) );
            amenu.addentry( STASH, can_stash, '3', _( "Store it in your inventory." ) );
            amenu.addentry( DROP, true, '4', _( "Drop it on the ground." ) );

            amenu.query();
            const option choice = amenu.ret == UILIST_CANCEL ? DROP : static_cast<option>( amenu.ret );
            switch( choice ) {
                case WIELD_CRAFT: {
                    if( std::optional<item_location> it_loc = wield_craft( ch, craft ) ) {
                        craft_in_world = *it_loc;
                    } else {
                        // This almost certainly shouldn't happen
                        put_into_vehicle_or_drop( ch, item_drop_reason::tumbling, {craft} );
                    }
                    break;
                }
                case DROP_CRAFT: {
                    craft_in_world = set_item_map_or_vehicle( ch, ch.pos(), craft );
                    break;
                }
                case STASH: {
                    set_item_inventory( ch, craft );
                    break;
                }
                case DROP: {
                    put_into_vehicle_or_drop( ch, item_drop_reason::deliberate, {craft} );
                    break;
                }
            }
        }
    } else {
        // We have a workbench, put the item there.
        craft_in_world = set_item_map_or_vehicle( ch, *target, craft );
    }

    if( !craft_in_world ) {
        ch.add_msg_if_player( _( "Wield and activate the %s to start working." ), craft.tname() );
    }

    return craft_in_world;
}

void Character::start_craft( craft_command &command, const std::optional<tripoint> &loc )
{
    if( command.empty() ) {
        debugmsg( "Attempted to start craft with empty command" );
        return;
    }

    item craft = command.create_in_progress_craft();
    if( craft.is_null() ) {
        return;
    }
    const recipe &making = craft.get_making();
    if( static_cast<int>( get_skill_level( command.get_skill_id() ) ) > making.get_skill_cap() ) {
        handle_skill_warning( command.get_skill_id(), true );
    }

    // In case we were wearing something just consumed
    if( !craft.components.empty() ) {
        calc_encumbrance();
    }

    item_location craft_in_world = place_craft_or_disassembly( *this, craft, loc );

    if( !craft_in_world ) {
        return;
    }

    if( is_avatar() ) {
        assign_activity( craft_activity_actor( craft_in_world, command.is_long() ) );
    } else {
        // set flag to craft
        craft_in_world.get_item()->set_var( "crafter", name );
        assign_activity( ACT_MULTIPLE_CRAFT );
    }

    add_msg_player_or_npc(
        pgettext( "in progress craft", "You start working on the %s." ),
        pgettext( "in progress craft", "<npcname> starts working on the %s." ),
        craft.tname() );
}

bool Character::craft_skill_gain( const item &craft, const int &num_practice_ticks )
{
    if( !craft.is_craft() ) {
        debugmsg( "craft_skill_check() called on non-craft '%s.' Aborting.", craft.tname() );
        return false;
    }

    const recipe &making = craft.get_making();
    if( !making.skill_used ) {
        return false;
    }

    const int skill_cap = making.get_skill_cap();
    const int batch_size = craft.get_making_batch_size();
    // NPCs assisting or watching should gain experience...
    for( npc *helper : get_crafting_helpers() ) {
        // If the NPC can understand what you are doing, they gain more exp
        if( helper->get_skill_level( making.skill_used ) >= making.difficulty ) {
            helper->practice( making.skill_used, roll_remainder( num_practice_ticks / 2.0 ),
                              skill_cap );
            if( batch_size > 1 && one_in( 300 ) ) {
                add_msg( m_info, _( "%s assists with crafting…" ), helper->get_name() );
            }
            if( batch_size == 1 && one_in( 300 ) ) {
                add_msg( m_info, _( "%s could assist you with a batch…" ), helper->get_name() );
            }
        } else {
            helper->practice( making.skill_used, roll_remainder( num_practice_ticks / 10.0 ),
                              skill_cap );
            if( one_in( 300 ) ) {
                add_msg( m_info, _( "%s watches you craft…" ), helper->get_name() );
            }
        }
    }
    return practice( making.skill_used, num_practice_ticks, skill_cap, true );
}

bool Character::craft_proficiency_gain( const item &craft, const time_duration &time )
{
    if( !craft.is_craft() ) {
        debugmsg( "craft_proficiency_gain() called on non-craft %s", craft.tname() );
        return false;
    }
    const recipe &making = craft.get_making();

    struct learn_subject {
        proficiency_id proficiency;
        float time_multiplier;
        std::optional<time_duration> max_experience;
    };

    const std::vector<npc *> helpers = get_crafting_helpers();
    std::vector<Character *> all_crafters{ helpers.begin(), helpers.end() };
    all_crafters.push_back( this );

    bool player_gained_prof = false;
    for( Character *p : all_crafters ) {
        std::vector<learn_subject> subjects;
        for( const recipe_proficiency &prof : making.proficiencies ) {
            if( !p->_proficiencies->has_learned( prof.id ) &&
                prof.id->can_learn() &&
                p->_proficiencies->has_prereqs( prof.id ) ) {
                learn_subject subject{
                    prof.id,
                    prof.learning_time_mult / prof.time_multiplier,
                    prof.max_experience
                };
                subjects.push_back( subject );
            }
        }

        if( subjects.empty() ) {
            player_gained_prof = false;
            continue;
        }
        const time_duration learn_time = time / subjects.size();

        bool gained_prof = false;
        for( const learn_subject &subject : subjects ) {
            int helper_bonus = 1;
            for( const Character *other : all_crafters ) {
                if( p != other && other->has_proficiency( subject.proficiency ) ) {
                    // Other characters who know the proficiency help you learn faster
                    helper_bonus = 2;
                }
            }
            gained_prof |= p->practice_proficiency( subject.proficiency,
                                                    subject.time_multiplier * learn_time * helper_bonus,
                                                    subject.max_experience );
        }
        // This depends on the player being the last element in the iteration
        player_gained_prof = gained_prof;
    }

    return player_gained_prof;
}

float Character::get_recipe_weighted_skill_average( const recipe &making ) const
{
    float secondary_skill_total = 0;
    int secondary_difficulty = 0;
    for( const std::pair<const skill_id, int> &count_secondaries : making.required_skills ) {
        // the difficulty of each secondary skill, count_secondaries.second, adds weight:
        // skills required at a higher level count more.
        secondary_skill_total += get_skill_level( count_secondaries.first ) * count_secondaries.second;
        secondary_difficulty += count_secondaries.second;
    }
    add_msg_debug( debugmode::DF_CRAFTING,
                   "For craft %s, has %g secondary skills with difficulty sum %d", making.ident().str(),
                   secondary_skill_total, secondary_difficulty );
    // The primary required skill counts extra compared to the secondary skills, before factoring in the
    // weight added by the required level.
    const float weighted_skill_average =
        ( ( 2.0f * std::max( making.difficulty,
                             1 ) * get_skill_level( making.skill_used ) ) + secondary_skill_total ) /
        // No DBZ
        std::max( 1.f, ( 2.0f * std::max( making.difficulty, 1 ) + secondary_difficulty ) );
    add_msg_debug( debugmode::DF_CRAFTING, "Weighted skill average: %g", weighted_skill_average );

    float total_skill_modifiers = 0.0f;

    // farsightedness can impose a penalty on electronics and tailoring success
    // This should be changed to a json-defined penalty by skill (vision_penalty) in skills.json
    if( has_flag( json_flag_HYPEROPIC ) && !worn_with_flag( flag_FIX_FARSIGHT ) &&
        !has_effect( effect_contacts ) ) {
        float vision_penalty = 0.0f;
        if( making.skill_used == skill_electronics ) {
            vision_penalty = 2.0f;
        } else if( making.skill_used == skill_tailor ) {
            vision_penalty = 1.0f;
        }
        total_skill_modifiers -= vision_penalty;
    }

    // Mutations can define specific skill bonuses and penalties.  Gotta include those.
    for( const trait_id &mut : get_mutations() ) {
        for( const std::pair<const skill_id, int> &skill_bonuses : mut->craft_skill_bonus ) {
            if( making.skill_used == skill_bonuses.first ) {
                total_skill_modifiers += skill_bonuses.second * 1.0f;
            }
        }
    }

    // TO DO: Attribute role should also be data-driven either in skills.json or in the recipe itself.
    // For now let's just use Intelligence.  For the average intelligence of 8, give +2.  Inc/dec by 0.25 per stat point.
    // This ensures that at parity, where skill = difficulty, you have a roughly 85% chance of success at average intelligence.
    total_skill_modifiers += int_cur / 4.0f;
    add_msg_debug( debugmode::DF_CRAFTING, "Total skill modifiers: %g (+%g from int)",
                   total_skill_modifiers, int_cur / 4.f );

    // Missing proficiencies penalize skill level
    for( const recipe_proficiency &recip : making.proficiencies ) {
        if( !recip.required && !has_proficiency( recip.id ) ) {
            total_skill_modifiers -= recip.skill_penalty;
        }
    }

    add_msg_debug( debugmode::DF_CHARACTER, "Total skill modifiers after proficiencies: %g",
                   total_skill_modifiers );
    return weighted_skill_average + total_skill_modifiers;
}

Character::craft_roll_data Character::recipe_success_roll_data( const recipe &making ) const
{
    // We're going to use a sqrt( sum of squares ) method here to give diminishing returns for more low level helpers.
    float player_weighted_skill_average = get_recipe_weighted_skill_average( making );
    bool negative = player_weighted_skill_average < 0;
    player_weighted_skill_average = std::pow( player_weighted_skill_average, 2 );
    if( negative ) {
        player_weighted_skill_average *= -1;
    }

    float npc_helpers_weighted_average = 0.0f;
    for( const npc *np : get_crafting_helpers() ) {
        // can the NPC actually craft this recipe?
        bool has_all_skills = np->get_skill_level( making.skill_used ) >= making.difficulty;
        if( has_all_skills ) {
            for( const std::pair<const skill_id, int> &secondary_skills : making.required_skills ) {
                if( np->get_skill_level( secondary_skills.first ) < secondary_skills.second ) {
                    has_all_skills = false;
                }
            }
        }
        if( has_all_skills && !making.character_has_required_proficiencies( *np ) ) {
            has_all_skills = false;
        }
        if( has_all_skills ) {
            const float helper_skill_average = std::max( np->get_recipe_weighted_skill_average( making ),
                                               0.0f );
            npc_helpers_weighted_average += std::pow( helper_skill_average, 2 );
            add_msg_if_player( m_info, _( "%s helps with crafting…" ), np->name );
        }
    }

    // Use a sum of squares to determine the final weighted average.
    negative = false;
    float summed_skills = player_weighted_skill_average + npc_helpers_weighted_average;
    if( summed_skills < 0 ) {
        negative = true;
        // Store that it was negative to reapply later, then make it positive because we need a sqrt.
        summed_skills *= -1;
    }
    float weighted_skill_average = sqrt( summed_skills );
    if( negative ) {
        weighted_skill_average *= -1;
    }
    add_msg_debug( debugmode::DF_CHARACTER, "Skill combined with followers: %f",
                   weighted_skill_average );

    int secondary_difficulty = 0;
    int secondary_level_count = 0;
    for( const std::pair<const skill_id, int> &count_secondaries : making.required_skills ) {
        secondary_level_count += count_secondaries.second;
        secondary_difficulty += std::pow( count_secondaries.second, 2 );
    }

    // The actual difficulty of the skill check is equal to:
    // (2*(primary skill difficulty)^2 + (sum of secondary difficulties)) / (2*(primary skill difficulty) + number of secondary skills)
    // This is basically just a hard to read weighted average formula: the fact that the primary difficulty gets squared
    // and then divided out again makes it a bit messy. Sorry, less mathy friends.
    const float final_difficulty =
        ( 2.0f * making.difficulty * making.difficulty + 1.0f * secondary_difficulty ) /
        // NO DBZ
        std::max( 1.f, ( 2.0f * making.difficulty + 1.0f * secondary_level_count ) );
    add_msg_debug( debugmode::DF_CHARACTER, "Final craft difficulty: %f", final_difficulty );

    // in the future we might want to make the standard deviation vary depending on some feature of the recipe.
    // For now, it varies only depending on your skill relative to the recipe's difficulty.
    float crafting_stddev = 2.0f;
    if( final_difficulty > weighted_skill_average ) {
        // Increase the standard deviation by 0.33 for every point below the difficulty your skill level is.
        // This makes the rolls more random and "swingy" at low levels, based more on luck.
        crafting_stddev += ( final_difficulty - weighted_skill_average ) / 3;
    }
    if( final_difficulty < weighted_skill_average ) {
        // decrease the standard deviation by 0.25 for every point above the difficulty your skill level is, to a cap of 1.
        // This means that luck plays less of a role the more overqualified you are.
        crafting_stddev -= std::min( ( weighted_skill_average - final_difficulty ) / 4, 1.0f );
    }

    // Let's just be careful, I don't want to touch a negative stddev
    crafting_stddev = std::max( crafting_stddev, 1.f );

    craft_roll_data ret;
    ret.center = weighted_skill_average;
    ret.stddev = crafting_stddev;
    ret.final_difficulty = final_difficulty + 1;
    if( has_trait( trait_DEBUG_CNF ) ) {
        ret.center = 2.f;
        ret.stddev = std::numeric_limits<decltype( ret.stddev )>::epsilon();
        ret.final_difficulty = 0.f;
    }
    return ret;
}

Character::craft_roll_data Character::recipe_failure_roll_data( const recipe &making ) const
{
    craft_roll_data data = recipe_success_roll_data( making );
    // Fund the numbers for the outcomes we want
    data.final_difficulty -= 1;
    data.final_difficulty *= 0.25;
    data.stddev *= 0.5;
    return data;
}

float Character::crafting_success_roll( const recipe &making ) const
{
    craft_roll_data data = recipe_success_roll_data( making );
    float craft_roll = std::max( normal_roll( data.center, data.stddev ), 0.0 );

    add_msg_debug( debugmode::DF_CHARACTER, "Crafting skill roll: %f, final difficulty %g", craft_roll,
                   data.final_difficulty );

    return std::max( craft_roll - data.final_difficulty, 0.0f );
}

float Character::crafting_failure_roll( const recipe &making ) const
{
    craft_roll_data data = recipe_failure_roll_data( making );
    float craft_roll = std::max( normal_roll( data.center, data.stddev ), 0.0 );

    add_msg_debug( debugmode::DF_CHARACTER, "Crafting skill roll: %f, final difficulty %g", craft_roll,
                   data.final_difficulty );

    return std::max( craft_roll, 0.0f );
}

float Character::recipe_success_chance( const recipe &making ) const
{
    // We calculate the failure chance of a recipe by performing a normal roll with a given
    // standard deviation and center, then subtracting a "final difficulty" score from that.
    // If that result is above 1, there is no chance of failure.
    craft_roll_data data = recipe_success_roll_data( making );

    return normal_roll_chance( data.center, data.stddev, 1.f + data.final_difficulty );
}

float Character::item_destruction_chance( const recipe &making ) const
{
    // If a normal roll with these parameters rolls over 1, we will not have a catastrophic failure
    // If we roll under one, we will
    craft_roll_data data = recipe_failure_roll_data( making );

    // normal_roll_chance returns the chance that we roll over, we want the chance we roll under
    return 1.f - normal_roll_chance( data.center, data.stddev, 1.f + data.final_difficulty );
}

int item::get_next_failure_point() const
{
    if( !is_craft() ) {
        debugmsg( "get_next_failure_point() called on non-craft '%s.'  Aborting.", tname() );
        return INT_MAX;
    }
    return craft_data_->next_failure_point >= 0 ? craft_data_->next_failure_point : INT_MAX;
}

void item::set_next_failure_point( const Character &crafter )
{
    if( !is_craft() ) {
        debugmsg( "set_next_failure_point() called on non-craft '%s.'  Aborting.", tname() );
        return;
    }

    const int percent = 10000000;
    const float roll = crafter.crafting_success_roll( get_making() );
    const int failure_point_delta = roll * percent;

    craft_data_->next_failure_point = item_counter + failure_point_delta;
    // Accurately prints if we multiply by 100
    const float percent_fp = static_cast<float>( percent ) * 100;
    add_msg_debug( debugmode::DF_CRAFTING,
                   "Set failure point: chose +%g%% for %s, will occur when progress hits %g%% (roll %g)",
                   failure_point_delta / percent_fp, get_making().ident().str(),
                   craft_data_->next_failure_point / percent_fp, roll );
}

static void destroy_random_component( item &craft, const Character &crafter )
{
    if( craft.components.empty() ) {
        debugmsg( "destroy_random_component() called on craft with no components!  Aborting" );
        return;
    }

    item destroyed = craft.components.get_and_remove_random_entry();

    crafter.add_msg_player_or_npc( game_message_params( game_message_type::m_bad ),
                                   _( "You mess up and destroy the %s." ),
                                   _( "<npcname> messes up and destroys the %s" ), destroyed.tname() );
}

bool item::handle_craft_failure( Character &crafter )
{
    if( !is_craft() ) {
        debugmsg( "handle_craft_failure() called on non-craft '%s.'  Aborting.", tname() );
        return false;
    }

    const double success_roll = crafter.crafting_failure_roll( get_making() );
    const int starting_components = this->components.size();
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
        if( crafter.has_trait( trait_INT_ALPHA ) ) {
            crafter.add_morale( MORALE_FAILURE, -10, -50, 10_hours, 5_hours );
        }
    }
    if( starting_components > 0 && this->components.empty() ) {
        // The craft had components and all of them were destroyed.
        return true;
    }

    // If a loss happens, minimum 25% progress lost, average 35%.  Falls off exponentially
    // Loss is scaled by the success roll
    const double percent_progress_loss = rng_exponential( 0.25, 0.35 ) * ( 1.0 - success_roll );
    // Ensure only positive losses have an effect on progress
    if( percent_progress_loss > 0.0 ) {
        const int progress_loss = item_counter * percent_progress_loss;
        crafter.add_msg_player_or_npc( _( "You mess up and lose %d%% progress." ),
                                       _( "<npcname> messes up and loses %d%% progress." ), progress_loss / 100000 );
        item_counter = clamp( item_counter - progress_loss, 0, 10000000 );
        if( crafter.has_trait( trait_INT_ALPHA ) ) {
            crafter.add_msg_player_or_npc( game_message_params( game_message_type::m_bad ),
                                           _( "Ugh, this should be EASY with how smart you are!" ),
                                           _( "<npcname> seems to get really upset over this." ) );
            crafter.add_morale( MORALE_FAILURE, -10, -50, 10_hours, 5_hours );
        }
    }

    set_next_failure_point( crafter );

    // Check if we can consume a new component and continue
    if( !crafter.can_continue_craft( *this ) ) {
        crafter.cancel_activity();
    }
    return false;
}

requirement_data item::get_continue_reqs() const
{
    if( !is_craft() ) {
        debugmsg( "get_continue_reqs() called on non-craft '%s.'  Aborting.", tname() );
        return requirement_data();
    }
    return requirement_data::continue_requirements( craft_data_->comps_used, components );
}

void item::inherit_flags( const item &parent, const recipe &making )
{
    // default behavior is to resize the clothing, which happens elsewhere
    if( making.has_flag( flag_NO_RESIZE ) ) {
        //If item is crafted from poor-fit components, the result is poorly fitted too
        if( parent.has_flag( flag_VARSIZE ) ) {
            unset_flag( flag_FIT );
        }
        //If item is crafted from perfect-fit components, the result is perfectly fitted too
        if( parent.has_flag( flag_FIT ) ) {
            set_flag( flag_FIT );
        }
    }
    for( const flag_id &f : parent.get_flags() ) {
        if( f->craft_inherit() ) {
            set_flag( f );
        }
    }
    for( const flag_id &f : parent.type->get_flags() ) {
        if( f->craft_inherit() ) {
            set_flag( f );
        }
    }
    if( parent.has_flag( flag_HIDDEN_POISON ) ) {
        poison = parent.poison;
    }
}

void item::inherit_flags( const item_components &parents, const recipe &making )
{
    for( const item_components::type_vector_pair &tvp : parents ) {
        for( const item &parent : tvp.second ) {
            inherit_flags( parent, making );
        }
    }
}

static void set_temp_rot( item &newit, const double relative_rot, const bool should_heat )
{
    if( newit.has_temperature() ) {
        if( newit.goes_bad() ) {
            newit.set_relative_rot( relative_rot );
        }
        if( should_heat ) {
            newit.heat_up();
        } else {
            // Really what we should be doing is averaging the temperatures
            // between the recipe components if we don't have a heat tool, but
            // that's kind of hard.  For now just set the item to 20 C
            // and reset the temperature.
            //
            // Temperature is not functional for non-foods
            newit.set_item_temperature( units::from_celsius( 20 ) );
        }
    }
}

static void spawn_items( Character &guy, std::vector<item> &results,
                         const std::optional<tripoint> &loc, const double relative_rot, const bool should_heat,
                         bool allow_wield = false )
{
    for( item &newit : results ) {
        // todo: set this up recursively, who knows what kinda crafts will need it
        if( !newit.empty() ) {
            for( item *new_content : newit.all_items_top() ) {
                set_temp_rot( *new_content, relative_rot, should_heat );
            }
        }
        set_temp_rot( newit, relative_rot, should_heat );

        newit.set_owner( guy.get_faction()->id );
        if( newit.made_of( phase_id::LIQUID ) ) {
            liquid_handler::handle_all_liquid( newit, PICKUP_RANGE );
        } else if( !loc && allow_wield && !guy.has_wield_conflicts( newit ) &&
                   guy.can_wield( newit ).success() ) {
            wield_craft( guy, newit );
        } else if( !loc ) {
            set_item_inventory( guy, newit );
        } else {
            set_item_map_or_vehicle( guy, loc.value_or( guy.pos() ), newit );
        }
    }
}

void Character::complete_craft( item &craft, const std::optional<tripoint> &loc )
{
    if( !craft.is_craft() ) {
        debugmsg( "complete_craft() called on non-craft '%s.'  Aborting.", craft.tname() );
        return;
    }

    const recipe &making = craft.get_making();
    const int batch_size = craft.get_making_batch_size();
    item_components &used = craft.components;
    const double relative_rot = craft.get_relative_rot();
    const bool should_heat = making.hot_result();
    std::vector<item> newits;

    if( making.is_practice() ) {
        add_msg( _( "You finish practicing %s." ), making.result_name() );
        // practice recipes don't produce a result item
    } else if( !making.result().is_null() ) {
        newits = making.create_results( batch_size, &used );
        // only wield crafted items if there's only one
        bool allow_wield = newits.size() == 1;
        spawn_items( *this, newits, loc, relative_rot, should_heat, allow_wield );
    }


    // messages, learning of recipe
    if( !making.is_practice() && ( !newits.empty() || !making.result_eocs.empty() ) ) {
        // TODO: reconsider recipe memorization
        if( knows_recipe( &making ) ) {
            add_msg( _( "You craft %s from memory." ), making.result_name() );
        } else {
            add_msg( _( "You craft %s using a reference." ), making.result_name() );
            // If we made it, but we don't know it, we're using a book, device or NPC
            // as a reference and have a chance to learn it.
            // Base expected time to learn is 1000*(difficulty^4)/skill/int moves.
            // This means time to learn is greatly decreased with higher skill level,
            // but also keeps going up as difficulty goes up.
            // Worst case is lvl 10, which will typically take
            // 10^4/10 (1,000) minutes, or about 16 hours of crafting it to learn.
            int difficulty = making.difficulty;
            ///\EFFECT_INT increases chance to learn recipe when crafting from a book
            const double learning_speed =
                std::max( get_skill_level( making.skill_used ), 1.0f ) *
                std::max( get_int(), 1 );
            const double time_to_learn = 1000 * 8 * std::pow( difficulty, 4 ) / learning_speed;
            if( x_in_y( making.time_to_craft_moves( *this ), time_to_learn ) ) {
                learn_recipe( &making );
                add_msg( m_good, _( "You memorized the recipe for %s!" ),
                         making.result_name() );
            }
        }
    }

    if( making.has_byproducts() ) {
        std::vector<item> bps = making.create_byproducts( batch_size );
        spawn_items( *this, bps, loc, relative_rot, should_heat );
    }

    recoil = MAX_RECOIL;

    inv->restack( *this );
    for( const effect_on_condition_id &eoc : making.result_eocs ) {
        dialogue d( get_talker_for( *this ), nullptr );
        if( eoc->type == eoc_type::ACTIVATION ) {
            eoc->activate( d );
        } else {
            debugmsg( "Must use an activation eoc for a recipe.  If you don't want the effect_on_condition to happen on its own, remove the recurrence min and max.  Otherwise, create a non-recurring effect_on_condition for this recipe with its condition and effects, then have a recurring one queue it." );
        }
    }
}

bool Character::can_continue_craft( item &craft )
{
    if( !craft.is_craft() ) {
        debugmsg( "complete_craft() called on non-craft '%s.'  Aborting.", craft.tname() );
        return false;
    }

    return can_continue_craft( craft, craft.get_continue_reqs() );
}

bool Character::can_continue_craft( item &craft, const requirement_data &continue_reqs )
{
    const recipe &rec = craft.get_making();
    if( !rec.character_has_required_proficiencies( *this ) ) {
        return false;
    }

    // Avoid building an inventory from the map if we don't have to, as it is expensive
    if( !continue_reqs.is_empty() ) {

        auto std_filter = [&rec]( const item & it ) {
            return rec.get_component_filter()( it ) && ( it.is_container_empty() ||
                    !it.is_watertight_container() );
        };
        const std::function<bool( const item & )> no_rotten_filter =
            rec.get_component_filter( recipe_filter_flags::no_rotten );
        const std::function<bool( const item & )> no_favorite_filter =
            rec.get_component_filter( recipe_filter_flags::no_favorite );
        bool use_rotten_filter = true;
        bool use_favorite_filter = true;
        // continue_reqs are for all batches at once
        const int batch_size = 1;

        if( !continue_reqs.can_make_with_inventory( crafting_inventory(), std_filter, batch_size ) ) {
            if( is_avatar() ) {
                std::string buffer = _( "You don't have the required components to continue crafting!" );
                buffer += "\n";
                buffer += continue_reqs.list_missing();
                popup( buffer, PF_NONE );
            } else {
                add_msg_if_npc( _( "<npcname> don't have the required components to continue crafting!" ) );
            }
            return false;
        }

        std::string buffer = _( "Consume the missing components and continue crafting?" );
        buffer += "\n";
        buffer += continue_reqs.list_all();
        if( !query_yn( buffer ) ) {
            return false;
        }

        if( !continue_reqs.can_make_with_inventory( crafting_inventory(), no_rotten_filter, batch_size ) ) {
            if( !query_yn( _( "Some components required to continue are rotten.\n"
                              "Continue crafting anyway?" ) ) ) {
                return false;
            }
            use_rotten_filter = false;
        }

        if( !continue_reqs.can_make_with_inventory( crafting_inventory(), no_favorite_filter,
                batch_size ) ) {
            if( !query_yn( _( "Some components required to continue are favorite.\n"
                              "Continue crafting anyway?" ) ) ) {
                return false;
            }
            use_favorite_filter = false;
        }

        inventory map_inv;
        map_inv.form_from_map( pos(), PICKUP_RANGE, this );

        auto filter = [&]( const item & it ) {
            return std_filter( it ) &&
                   ( !use_rotten_filter || no_rotten_filter( it ) ) &&
                   ( !use_favorite_filter || no_favorite_filter( it ) );
        };

        std::vector<comp_selection<item_comp>> item_selections;
        for( const auto &it : continue_reqs.get_components() ) {
            comp_selection<item_comp> is = select_item_component( it, batch_size, map_inv, true, filter, true,
                                           &rec );
            if( is.use_from == usage_from::cancel ) {
                cancel_activity();
                add_msg( _( "You stop crafting." ) );
                return false;
            }
            item_selections.push_back( is );
        }
        for( const auto &it : item_selections ) {
            for( item &itm : consume_items( it, batch_size, filter ) ) {
                craft.components.add( itm );
            }
        }
    }

    if( !craft.has_tools_to_continue() ) {

        const std::vector<std::vector<tool_comp>> &tool_reqs = rec.simple_requirements().get_tools();
        const int batch_size = craft.get_making_batch_size();

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
            if( is_avatar() ) {
                std::string buffer = _( "You don't have the necessary tools to continue crafting!" );
                buffer += "\n";
                buffer += tool_continue_reqs.list_missing();
                popup( buffer, PF_NONE );
            } else {
                add_msg_if_npc( _( "<npcname> don't have the necessary tools to continue crafting!" ) );
            }
            return false;
        }

        inventory map_inv;
        map_inv.form_from_map( pos(), PICKUP_RANGE, this );

        std::vector<comp_selection<tool_comp>> new_tool_selections;
        for( const std::vector<tool_comp> &alternatives : tool_reqs ) {
            comp_selection<tool_comp> selection = select_tool_component( alternatives, batch_size,
            map_inv, true, true, []( int charges ) {
                return charges / 20;
            } );
            if( selection.use_from == usage_from::cancel ) {
                return false;
            }
            new_tool_selections.push_back( selection );
        }

        craft.set_cached_tool_selections( new_tool_selections );
        craft.set_tools_to_continue( true );
    }

    return true;
}
const requirement_data *Character::select_requirements(
    const std::vector<const requirement_data *> &alternatives, int batch,
    const read_only_visitable &inv,
    const std::function<bool( const item & )> &filter ) const
{
    cata_assert( !alternatives.empty() );
    if( alternatives.size() == 1 || !is_avatar() ) {
        return alternatives.front();
    }

    uilist menu;

    for( const requirement_data *req : alternatives ) {
        // Write with a large width and then just re-join the lines, because
        // uilist does its own wrapping and we want to rely on that.
        std::vector<std::string> component_lines =
            req->get_folded_components_list( TERMX - 4, c_light_gray, inv, filter, batch, "",
                                             requirement_display_flags::no_unavailable );
        menu.addentry_desc( "", string_join( component_lines, "\n" ) );
    }

    menu.allow_cancel = true;
    menu.desc_enabled = true;
    menu.title = _( "Use which selection of components?" );
    menu.query();

    if( menu.ret < 0 || static_cast<size_t>( menu.ret ) >= alternatives.size() ) {
        return nullptr;
    }

    return alternatives[menu.ret];
}

/* selection of component if a recipe requirement has multiple options (e.g. 'duct tap' or 'welder') */
comp_selection<item_comp> Character::select_item_component( const std::vector<item_comp>
        &components,
        int batch, read_only_visitable &map_inv, bool can_cancel,
        const std::function<bool( const item & )> &filter, bool player_inv, const recipe *rec )
{
    Character &player_character = get_player_character();
    std::vector<std::pair<item_comp, std::optional<nc_color>>> player_has;
    std::vector<std::pair<item_comp, std::optional<nc_color>>> map_has;
    std::vector<std::pair<item_comp, std::optional<nc_color>>> mixed;

    comp_selection<item_comp> selected;

    for( const item_comp &component : components ) {
        itype_id type = component.type;
        int count = ( component.count > 0 ) ? component.count * batch : std::abs( component.count );

        if( item::count_by_charges( type ) && count > 0 ) {
            int map_charges = map_inv.charges_of( type, INT_MAX, filter );

            // If map has infinite charges, just use them
            if( map_charges == item::INFINITE_CHARGES ) {
                selected.use_from = usage_from::map;
                selected.comp = component;
                return selected;
            }
            if( player_inv ) {
                int player_charges = charges_of( type, INT_MAX, filter );
                bool found = false;
                if( player_charges >= count ) {
                    player_has.emplace_back( component, std::nullopt );
                    found = true;
                }
                if( map_charges >= count ) {
                    map_has.emplace_back( component, std::nullopt );
                    found = true;
                }
                if( !found && player_charges + map_charges >= count ) {
                    mixed.emplace_back( component, std::nullopt );
                }
            } else {
                if( map_charges >= count ) {
                    map_has.emplace_back( component, std::nullopt );
                }
            }
        } else { // Counting by units, not charges

            // Can't use pseudo items as components
            if( player_inv ) {
                bool found = false;
                const item item_sought( type );
                if( ( item_sought.is_software() && count_softwares( type ) > 0 ) ||
                    has_amount( type, count, false, filter ) ) {
                    std::optional<nc_color> colr = std::nullopt;
                    if( !has_amount( type, count, false, [&filter]( const item & it ) {
                    return filter( it ) && ( it.is_container_empty() || !it.is_watertight_container() );
                    } ) ) {
                        colr = c_magenta;
                    }
                    player_has.emplace_back( component, colr );
                    found = true;
                }
                if( map_inv.has_components( type, count, filter ) ) {
                    std::optional<nc_color> colr = std::nullopt;
                    if( !map_inv.has_components( type, count, [&filter]( const item & it ) {
                    return filter( it ) && ( it.is_container_empty() || !it.is_watertight_container() );
                    } ) ) {
                        colr = c_magenta;
                    }
                    map_has.emplace_back( component, colr );
                    found = true;
                }
                if( !found &&
                    amount_of( type, false, std::numeric_limits<int>::max(), filter ) +
                    map_inv.amount_of( type, false, std::numeric_limits<int>::max(), filter ) >= count ) {
                    std::optional<nc_color> colr = std::nullopt;
                    if( amount_of( type, false, std::numeric_limits<int>::max(), [&filter]( const item & it ) {
                    return filter( it ) && ( it.is_container_empty() || !it.is_watertight_container() );
                    } ) + map_inv.amount_of( type, false,
                    std::numeric_limits<int>::max(), [&filter]( const item & it ) {
                        return filter( it ) && ( it.is_container_empty() || !it.is_watertight_container() );
                    } ) < count ) {
                        colr = c_magenta;
                    }
                    mixed.emplace_back( component, colr );
                }
            } else {
                if( map_inv.has_components( type, count, filter ) ) {
                    std::optional<nc_color> colr = std::nullopt;
                    if( !map_inv.has_components( type, count, [&filter]( const item & it ) {
                    return filter( it ) && ( it.is_container_empty() || !it.is_watertight_container() );
                    } ) ) {
                        colr = c_magenta;
                    }
                    map_has.emplace_back( component, colr );
                }
            }
        }
    }

    /* select 1 component to use */
    if( player_has.size() + map_has.size() + mixed.size() == 1 ) { // Only 1 choice
        if( player_has.size() == 1 ) {
            selected.use_from = usage_from::player;
            selected.comp = player_has[0].first;
        } else if( map_has.size() == 1 ) {
            selected.use_from = usage_from::map;
            selected.comp = map_has[0].first;
        } else {
            selected.use_from = usage_from::both;
            selected.comp = mixed[0].first;
        }
    } else if( is_npc() ) {
        if( !player_has.empty() ) {
            selected.use_from = usage_from::player;
            selected.comp = player_has[0].first;
        } else if( !map_has.empty() ) {
            selected.use_from = usage_from::map;
            selected.comp = map_has[0].first;
        } else {
            debugmsg( "Attempted a recipe with no available components!" );
            selected.use_from = usage_from::cancel;
            return selected;
        }
    } else { // Let the player pick which component they want to use
        uilist cmenu;
        bool is_food = false;
        bool remove_raw = false;
        if( rec ) {
            is_food = !!rec->result()->comestible;
            remove_raw = rec->hot_result() || rec->removes_raw();
        }
        enum class inventory_source : int {
            SELF = 0,
            MAP,
            BOTH
        };
        auto get_ingredient_description = [&player_character, &map_inv, &filter,
                                                              &is_food, &remove_raw]( const inventory_source & inv_source,
        const itype_id & ingredient_type, const int &count ) {
            std::string text;
            int available;
            const item ingredient = item( ingredient_type );
            std::pair<int, int> kcal_values{ 0, 0 };

            switch( inv_source ) {
                case inventory_source::MAP:
                    text = _( "%s (%d/%d nearby)" );
                    kcal_values = map_inv.kcal_range( ingredient_type, filter, player_character );
                    available = item::count_by_charges( ingredient_type ) ?
                                map_inv.charges_of( ingredient_type, INT_MAX, filter ) :
                                map_inv.amount_of( ingredient_type, false, INT_MAX, filter );
                    break;
                case inventory_source::SELF:
                    text = _( "%s (%d/%d on person)" );
                    kcal_values = player_character.kcal_range( ingredient_type, filter, player_character );
                    available = item::count_by_charges( ingredient_type ) ?
                                player_character.charges_of( ingredient_type, INT_MAX, filter ) :
                                player_character.amount_of( ingredient_type, false, INT_MAX, filter );
                    break;
                case inventory_source::BOTH:
                    text = _( "%s (%d/%d nearby & on person)" );
                    kcal_values = map_inv.kcal_range( ingredient_type, filter, player_character );
                    const std::pair<int, int> kcal_values_tmp = player_character.kcal_range( ingredient_type, filter,
                            player_character );
                    kcal_values.first = std::min( kcal_values.first, kcal_values_tmp.first );
                    kcal_values.second = std::max( kcal_values.second, kcal_values_tmp.second );
                    available = item::count_by_charges( ingredient_type ) ?
                                map_inv.charges_of( ingredient_type, INT_MAX, filter ) +
                                player_character.charges_of( ingredient_type, INT_MAX, filter ) :
                                map_inv.amount_of( ingredient_type, false, INT_MAX, filter ) +
                                player_character.amount_of( ingredient_type, false, INT_MAX, filter );
                    break;
            }

            if( is_food && ingredient.is_food() ) {
                text += kcal_values.first == kcal_values.second ? _( " %d kcal" ) : _( " %d-%d kcal" );
                if( ingredient.has_flag( flag_RAW ) && remove_raw ) {
                    //Multiplier for RAW food digestion
                    kcal_values.first /= 0.75f;
                    kcal_values.second /= 0.75f;
                    text += _( " <color_brown> (will be processed)</color>" );
                }
            }
            return string_format( text,
                                  item::nname( ingredient_type ),
                                  count,
                                  available,
                                  kcal_values.first * count,
                                  kcal_values.second * count
                                );
        };
        // Populate options with the names of the items
        for( auto &map_ha : map_has ) {
            // Index 0-(map_has.size()-1)
            cmenu.addentry( get_ingredient_description( inventory_source::MAP, map_ha.first.type,
                            map_ha.first.count * batch ) );
            if( map_ha.second.has_value() ) {
                cmenu.entries.back().text_color = map_ha.second.value();
            }
        }
        for( auto &player_ha : player_has ) {
            // Index map_has.size()-(map_has.size()+player_has.size()-1)
            cmenu.addentry( get_ingredient_description( inventory_source::SELF, player_ha.first.type,
                            player_ha.first.count * batch ) );
            if( player_ha.second.has_value() ) {
                cmenu.entries.back().text_color = player_ha.second.value();
            }
        }
        for( auto &component : mixed ) {
            // Index player_has.size()-(map_has.size()+player_has.size()+mixed.size()-1)
            cmenu.addentry( get_ingredient_description( inventory_source::BOTH, component.first.type,
                            component.first.count * batch ) );
            if( component.second.has_value() ) {
                cmenu.entries.back().text_color = component.second.value();
            }
        }

        // Unlike with tools, it's a bad thing if there aren't any components available
        if( cmenu.entries.empty() ) {
            if( player_inv ) {
                if( has_trait( trait_DEBUG_HS ) ) {
                    selected.use_from = usage_from::player;
                    return selected;
                }
            }
            debugmsg( "Attempted a recipe with no available components!" );
            selected.use_from = usage_from::cancel;
            return selected;
        }

        size_t uselection;
        if( is_avatar() ) {
            cmenu.allow_cancel = can_cancel;

            // Get the selection via a menu popup
            cmenu.title = _( "Use which component?" );
            cmenu.query();

            if( cmenu.ret < 0 ||
                static_cast<size_t>( cmenu.ret ) >= map_has.size() + player_has.size() + mixed.size() ) {
                selected.use_from = usage_from::cancel;
                return selected;
            }

            uselection = static_cast<size_t>( cmenu.ret );
        } else {
            // NPC always picks the first candidate
            uselection = 0;
        }
        if( uselection < map_has.size() ) {
            selected.use_from = usage_from::map;
            selected.comp = map_has[uselection].first;
        } else if( uselection < map_has.size() + player_has.size() ) {
            uselection -= map_has.size();
            selected.use_from = usage_from::player;
            selected.comp = player_has[uselection].first;
        } else {
            uselection -= map_has.size() + player_has.size();
            selected.use_from = usage_from::both;
            selected.comp = mixed[uselection].first;
        }
    }

    return selected;
}

// Prompts player to empty all newly-unsealed containers in inventory
// Called after something that might have opened containers (making them buckets) but not emptied them
static void empty_buckets( Character &p )
{
    // First grab (remove) all items that are non-empty buckets and not wielded
    auto buckets = p.remove_items_with( [&p]( const item & it ) {
        return it.is_bucket_nonempty() && ( !p.get_wielded_item() || &it != &*p.get_wielded_item() );
    }, INT_MAX );
    for( item &it : buckets ) {
        for( const item *in : it.all_items_top() ) {
            drop_or_handle( *in, p );
        }

        it.clear_items();
        drop_or_handle( it, p );
    }
}

std::list<item> Character::consume_items( const comp_selection<item_comp> &is, int batch,
        const std::function<bool( const item & )> &filter, bool select_ind )
{
    return consume_items( get_map(), is, batch, filter, pos(), PICKUP_RANGE, select_ind );
}

std::list<item> Character::consume_items( map &m, const comp_selection<item_comp> &is, int batch,
        const std::function<bool( const item & )> &filter, const tripoint &origin, int radius,
        bool select_ind )
{
    std::list<item> ret;

    if( has_trait( trait_DEBUG_HS ) ) {
        return ret;
    }
    // populate a grid of spots that can be reached
    std::vector<tripoint> reachable_pts;
    m.reachable_flood_steps( reachable_pts, origin, radius, 1, 100 );
    return consume_items( m, is, batch, filter, reachable_pts, select_ind );
}

std::list<item> Character::consume_items( map &m, const comp_selection<item_comp> &is, int batch,
        const std::function<bool( const item & )> &filter, const std::vector<tripoint> &reachable_pts,
        bool select_ind )
{
    std::list<item> ret;

    if( has_trait( trait_DEBUG_HS ) ) {
        return ret;
    }

    item_comp selected_comp = is.comp;

    const bool by_charges = item::count_by_charges( selected_comp.type ) && selected_comp.count > 0;
    // Count given to use_amount/use_charges, changed by those functions!
    int real_count = ( selected_comp.count > 0 ) ? selected_comp.count * batch : std::abs(
                         selected_comp.count );
    // First try to get everything from the map, than (remaining amount) from player
    if( is.use_from & usage_from::map ) {
        if( by_charges ) {
            std::list<item> tmp = m.use_charges( reachable_pts, selected_comp.type, real_count, filter );
            ret.splice( ret.end(), tmp );
        } else {
            std::list<item> tmp = m.use_amount( reachable_pts, selected_comp.type, real_count, filter,
                                                select_ind );
            remove_ammo( tmp, *this );
            ret.splice( ret.end(), tmp );
        }
    }
    if( is.use_from & usage_from::player ) {
        if( by_charges ) {
            std::list<item> tmp = use_charges( selected_comp.type, real_count, filter );
            ret.splice( ret.end(), tmp );
        } else {
            std::list<item> tmp = use_amount( selected_comp.type, real_count, filter, select_ind );
            remove_ammo( tmp, *this );
            ret.splice( ret.end(), tmp );
        }
    }
    // Merge charges for items that stack with each other
    if( by_charges && ret.size() > 1 ) {
        for( auto outer = std::begin( ret ); outer != std::end( ret ); ++outer ) {
            for( auto inner = std::next( outer ); inner != std::end( ret ); ) {
                if( outer->merge_charges( *inner ) ) {
                    inner = ret.erase( inner );
                } else {
                    ++inner;
                }
            }
        }
    }
    for( item &it : ret ) {
        it.spill_contents( *this );
        // todo: make a proper solution that overflows with the proper item_location
        it.overflow( pos() );
    }
    empty_buckets( *this );
    return ret;
}

/* This call is in-efficient when doing it for multiple items with the same map inventory.
In that case, consider using select_item_component with 1 pre-created map inventory, and then passing the results
to consume_items */
std::list<item> Character::consume_items( const std::vector<item_comp> &components, int batch,
        const std::function<bool( const item & )> &filter,
        const std::function<bool( const itype_id & )> &select_ind )
{
    inventory map_inv;
    map_inv.form_from_map( pos(), PICKUP_RANGE, this );
    comp_selection<item_comp> sel = select_item_component( components, batch, map_inv, false, filter );
    return consume_items( sel, batch, filter, select_ind( sel.comp.type ) );
}

bool Character::consume_software_container( const itype_id &software_id )
{
    for( item_location it : all_items_loc() ) {
        if( !it.get_item() ) {
            continue;
        }
        if( it.get_item()->is_software_storage() ) {
            for( const item *soft : it.get_item()->softwares() ) {
                if( soft->typeId() == software_id ) {
                    it.remove_item();
                    return true;
                }
            }
        }
    }
    return false;
}

comp_selection<tool_comp>
Character::select_tool_component( const std::vector<tool_comp> &tools, int batch,
                                  read_only_visitable &map_inv, bool can_cancel, bool player_inv,
                                  const std::function<int( int )> &charges_required_modifier )
{

    comp_selection<tool_comp> selected;
    auto calc_charges = [&]( const tool_comp & t, bool ui = false ) {
        const int full_craft_charges = item::find_type( t.type )->charge_factor() * t.count * batch;
        const int modified_charges = charges_required_modifier( full_craft_charges );
        return std::max( ui ? full_craft_charges : modified_charges, 1 );
    };

    bool found_nocharge = false;
    std::vector<tool_comp> player_has;
    std::vector<tool_comp> map_has;
    std::vector<tool_comp> both_has;
    // Use charges of any tools that require charges used
    for( auto it = tools.begin(); it != tools.end() && !found_nocharge; ++it ) {
        itype_id type = it->type;
        if( it->count > 0 ) {
            const int count = calc_charges( *it );
            if( player_inv && crafting_inventory( pos(), -1 ).has_charges( type, count ) ) {
                player_has.push_back( *it );
            }
            if( map_inv.has_charges( type, count ) ) {
                map_has.push_back( *it );
            }
            // Needed for tools that can have power in a different location, such as a UPS.
            // Will only populate if no other options were found.
            if( player_inv && player_has.size() + map_has.size() == 0 &&
                crafting_inventory().has_charges( type, count ) ) {
                both_has.push_back( *it );
            }
        } else if( ( player_inv && has_amount( type, 1 ) ) || map_inv.has_tools( type, 1 ) ) {
            selected.comp = *it;
            found_nocharge = true;
        }
    }
    if( found_nocharge ) {
        selected.use_from = usage_from::none;
        return selected;    // Default to using a tool that doesn't require charges
    }

    if( ( both_has.size() + player_has.size() + map_has.size() == 1 ) || is_npc() ) {
        if( !both_has.empty() ) {
            selected.use_from = usage_from::both;
            selected.comp = both_has[0];
        } else if( !map_has.empty() ) {
            selected.use_from = usage_from::map;
            selected.comp = map_has[0];
        } else if( !player_has.empty() ) {
            selected.use_from = usage_from::player;
            selected.comp = player_has[0];
        } else {
            selected.use_from = usage_from::none;
            return selected;
        }
    } else { // Variety of options, list them and pick one
        // Populate the list
        uilist tmenu;
        for( tool_comp &map_ha : map_has ) {
            if( item::find_type( map_ha.type )->maximum_charges() > 1 ) {
                std::string tmpStr = string_format( _( "%s (%d/%d charges nearby)" ),
                                                    item::nname( map_ha.type ), calc_charges( map_ha, true ),
                                                    map_inv.charges_of( map_ha.type ) );
                tmenu.addentry( tmpStr );
            } else {
                std::string tmpStr = item::nname( map_ha.type ) + _( " (nearby)" );
                tmenu.addentry( tmpStr );
            }
        }
        for( tool_comp &player_ha : player_has ) {
            if( item::find_type( player_ha.type )->maximum_charges() > 1 ) {
                std::string tmpStr = string_format( _( "%s (%d/%d charges on person)" ),
                                                    item::nname( player_ha.type ), calc_charges( player_ha, true ),
                                                    charges_of( player_ha.type ) );
                tmenu.addentry( tmpStr );
            } else {
                tmenu.addentry( item::nname( player_ha.type ) );
            }
        }
        for( tool_comp &both_ha : both_has ) {
            if( item::find_type( both_ha.type )->maximum_charges() > 1 ) {
                std::string tmpStr = string_format( _( "%s (%d/%d charges nearby or on person)" ),
                                                    item::nname( both_ha.type ), calc_charges( both_ha, true ),
                                                    charges_of( both_ha.type ) );
                tmenu.addentry( tmpStr );
            } else {
                std::string tmpStr = item::nname( both_ha.type ) + _( " (at hand)" );
                tmenu.addentry( tmpStr );
            }
        }

        if( tmenu.entries.empty() ) {  // This SHOULD only happen if cooking with a fire,
            selected.use_from = usage_from::none;
            return selected;    // and the fire goes out.
        }

        size_t uselection;
        if( is_avatar() ) {
            tmenu.allow_cancel = can_cancel;

            // Get selection via a popup menu
            tmenu.title = _( "Use which tool?" );
            tmenu.query();

            if( tmenu.ret < 0 || static_cast<size_t>( tmenu.ret ) >= map_has.size()
                + player_has.size() + both_has.size() ) {
                selected.use_from = usage_from::cancel;
                return selected;
            }

            uselection = static_cast<size_t>( tmenu.ret );
        } else {
            // NPC always picks the first candidate
            uselection = 0;
        }
        if( uselection < map_has.size() ) {
            selected.use_from = usage_from::map;
            selected.comp = map_has[uselection];
        } else if( uselection < map_has.size() + player_has.size() ) {
            uselection -= map_has.size();
            selected.use_from = usage_from::player;
            selected.comp = player_has[uselection];
        } else {
            uselection -= map_has.size() + player_has.size();
            selected.use_from = usage_from::both;
            selected.comp = both_has[uselection];
        }
    }

    return selected;
}

bool Character::craft_consume_tools( item &craft, int multiplier, bool start_craft )
{
    if( !craft.is_craft() ) {
        debugmsg( "craft_consume_tools() called on non-craft '%s.' Aborting.", craft.tname() );
        return false;
    }
    if( has_trait( trait_DEBUG_HS ) ) {
        return true;
    }

    const auto calc_charges = [&craft, &start_craft, &multiplier]( int charges ) {
        int ret = charges;

        if( ret <= 0 ) {
            return ret;
        }

        // Account for batch size
        ret *= craft.get_making_batch_size();

        // In case more than 5% progress was accomplished in one turn
        ret *= multiplier;

        // Only for the next 5% progress
        ret /= 20;

        // If just starting consume the remainder as well
        if( start_craft ) {
            ret += ( charges * craft.get_making_batch_size() ) % 20;
        }
        return ret;
    };

    // First check if we still have our cached selections
    const std::vector<comp_selection<tool_comp>> &cached_tool_selections =
                craft.get_cached_tool_selections();

    inventory map_inv;
    map_inv.form_from_map( pos(), PICKUP_RANGE, this );

    for( const comp_selection<tool_comp> &tool_sel : cached_tool_selections ) {
        itype_id type = tool_sel.comp.type;
        if( tool_sel.comp.count > 0 ) {
            const int count = calc_charges( tool_sel.comp.count );
            switch( tool_sel.use_from ) {
                case usage_from::player:
                    if( !has_charges( type, count ) ) {
                        add_msg_player_or_npc(
                            _( "You have insufficient %s charges and can't continue crafting." ),
                            _( "<npcname> has insufficient %s charges and can't continue crafting." ),
                            item::nname( type ) );
                        craft.set_tools_to_continue( false );
                        return false;
                    }
                    break;
                case usage_from::map:
                    if( !map_inv.has_charges( type, count ) ) {
                        add_msg_player_or_npc(
                            _( "You have insufficient %s charges and can't continue crafting." ),
                            _( "<npcname> has insufficient %s charges and can't continue crafting." ),
                            item::nname( type ) );
                        craft.set_tools_to_continue( false );
                        return false;
                    }
                    break;
                case usage_from::both:
                    if( !crafting_inventory().has_charges( type, count ) ) {
                        add_msg_player_or_npc(
                            _( "You have insufficient %s charges and can't continue crafting." ),
                            _( "<npcname> has insufficient %s charges and can't continue crafting." ),
                            item::nname( type ) );
                        craft.set_tools_to_continue( false );
                        return false;
                    }
                case usage_from::none:
                case usage_from::cancel:
                case usage_from::num_usages_from:
                    break;
            }
        } else if( ( type != itype_id::NULL_ID() ) && !has_amount( type, 1 ) &&
                   !map_inv.has_tools( type, 1 ) ) {
            add_msg_player_or_npc(
                _( "You no longer have a %s and can't continue crafting." ),
                _( "<npcname> no longer has a %s and can't continue crafting." ),
                item::nname( type ) );
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

void Character::consume_tools( const comp_selection<tool_comp> &tool, int batch )
{
    consume_tools( get_map(), tool, batch, pos(), PICKUP_RANGE );
}

/* we use this if we selected the tool earlier */
void Character::consume_tools( map &m, const comp_selection<tool_comp> &tool, int batch,
                               const tripoint &origin, int radius, basecamp *bcp )
{
    if( has_trait( trait_DEBUG_HS ) ) {
        return;
    }

    const itype *tmp = item::find_type( tool.comp.type );
    int quantity = tool.comp.count * batch * tmp->charge_factor();
    if( tool.use_from == usage_from::both ) {
        use_charges( tool.comp.type, quantity, radius );
    } else if( tool.use_from == usage_from::player ) {
        use_charges( tool.comp.type, quantity );
    } else if( tool.use_from == usage_from::map ) {
        m.use_charges( origin, radius, tool.comp.type, quantity, return_true<item>, bcp );
        // Map::use_charges() does not handle UPS charges.
        if( quantity > 0 ) {
            use_charges( tool.comp.type, quantity, radius );
        }
    }

    // else, usage_from::none (or usage_from::cancel), so we don't use up any tools;
}

void Character::consume_tools( map &m, const comp_selection<tool_comp> &tool, int batch,
                               const std::vector<tripoint> &reachable_pts, basecamp *bcp )
{
    if( has_trait( trait_DEBUG_HS ) ) {
        return;
    }
    const itype *tmp = item::find_type( tool.comp.type );
    int quantity = tool.comp.count * batch * tmp->charge_factor();

    if( tool.use_from == usage_from::player || tool.use_from == usage_from::both ) {
        use_charges( tool.comp.type, quantity );
    }
    if( tool.use_from == usage_from::map || tool.use_from == usage_from::both ) {
        m.use_charges( reachable_pts, tool.comp.type, quantity, return_true<item>, bcp );
        // Map::use_charges() does not handle UPS charges.
        if( quantity > 0 ) {
            m.consume_ups( reachable_pts, units::from_kilojoule( quantity ) );
        }
    }

    // else, usage_from::none (or usage_from::cancel), so we don't use up any tools;
}

/* This call is in-efficient when doing it for multiple items with the same map inventory.
In that case, consider using select_tool_component with 1 pre-created map inventory, and then passing the results
to consume_tools */
void Character::consume_tools( const std::vector<tool_comp> &tools, int batch )
{
    inventory map_inv;
    map_inv.form_from_map( pos(), PICKUP_RANGE, this );
    consume_tools( select_tool_component( tools, batch, map_inv ), batch );
}

ret_val<void> Character::can_disassemble( const item &obj, const read_only_visitable &inv ) const
{
    if( !obj.is_disassemblable() ) {
        return ret_val<void>::make_failure( _( "You cannot disassemble this." ) );
    }

    const recipe &r = recipe_dictionary::get_uncraft( ( obj.typeId() == itype_disassembly ) ?
                      obj.components.only_item().typeId() : obj.typeId() );

    // check sufficient light
    if( lighting_craft_speed_multiplier( r ) == 0.0f ) {
        return ret_val<void>::make_failure( _( "You can't see to craft!" ) );
    }

    // refuse to disassemble rotten items
    if( obj.goes_bad() && obj.rotten() ) {
        return ret_val<void>::make_failure( _( "It's rotten, I'm not taking that apart." ) );
    }

    // refuse to disassemble items containing monsters/pets
    std::string monster = obj.get_var( "contained_name" );
    if( !monster.empty() ) {
        return ret_val<void>::make_failure( _( "You must remove the %s before you can disassemble this." ),
                                            monster );
    }

    if( !obj.is_ammo() ) { //we get ammo quantity to disassemble later on
        if( obj.count_by_charges() ) {
            // Create a new item to get the default charges
            int qty = 0;
            // it should always only create one item if batch size is 1, but just to be sure loop over results
            for( item &it : r.create_results() ) {
                qty += it.charges;
            }
            if( obj.charges < qty ) {
                const char *msg = n_gettext( "You need at least %d charge of %s.",
                                             "You need at least %d charges of %s.", qty );
                return ret_val<void>::make_failure( msg, qty, obj.tname() );
            }
        }
    }
    const requirement_data &dis = r.disassembly_requirements();

    for( const auto &opts : dis.get_qualities() ) {
        for( const quality_requirement &qual : opts ) {
            if( !qual.has( inv, return_true<item> ) ) {
                // Here should be no dot at the end of the string as 'to_string()' provides it.
                return ret_val<void>::make_failure( _( "You need %s" ), qual.to_string() );
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
                return ret_val<void>::make_failure( _( "You need %s." ),
                                                    item::nname( tool_required.type ) );
            } else {
                //~ %1$s: tool name, %2$d: needed charges
                return ret_val<void>::make_failure( n_gettext( "You need a %1$s with %2$d charge.",
                                                    "You need a %1$s with %2$d charges.", tool_required.count ),
                                                    item::nname( tool_required.type ),
                                                    tool_required.count );
            }
        }
    }

    return ret_val<void>::make_success();
}

item_location Character::create_in_progress_disassembly( item_location target )
{
    item &orig_item = *target.get_item();
    item new_disassembly;
    if( target->typeId() == itype_disassembly ) {
        new_disassembly = item( orig_item );
    } else {
        const recipe &r = recipe_dictionary::get_uncraft( target->typeId() );
        new_disassembly = item( &r, activity.position, orig_item );

        // Remove any batteries, ammo, contents and mods first
        remove_ammo( orig_item, *this );
        remove_radio_mod( orig_item, *this );
        if( orig_item.is_container() ) {
            orig_item.spill_contents( pos() );
        }
        if( orig_item.count_by_charges() ) {
            //subtract selected number of rounds to disassemble
            orig_item.charges -= activity.position;
        }
    }
    // remove the item, except when it's counted by charges and still has some
    if( !orig_item.count_by_charges() || orig_item.charges <= 0 ||
        target->typeId() == itype_disassembly ) {
        target.remove_item();
    }

    item_location disassembly_in_world = place_craft_or_disassembly( *this, new_disassembly,
                                         std::nullopt );

    if( !disassembly_in_world ) {
        return item_location::nowhere;
    }

    return disassembly_in_world;
}

bool Character::disassemble()
{
    return disassemble( game_menus::inv::disassemble( *this ), false );
}

bool Character::disassemble( item_location target, bool interactive, bool disassemble_all )
{
    if( !target ) {
        add_msg( _( "Never mind." ) );
        return false;
    }

    const item &obj = *target;
    const auto ret = can_disassemble( obj, crafting_inventory() );

    if( !ret.success() ) {
        add_msg_if_player( m_info, "%s", ret.c_str() );
        return false;
    }

    if( obj.is_favorite &&
        !query_yn( _( "You're going to disassemble favorited item.\nAre you sure?" ) ) ) {
        return false;
    }

    if( obj.is_worn_by_player() &&
        !query_yn( _( "You're going to disassemble worn item.\nAre you sure?" ) ) ) {
        return false;
    }

    avatar &player_character = get_avatar();
    const recipe &r = ( obj.typeId() == itype_disassembly ) ? obj.get_making() :
                      recipe_dictionary::get_uncraft( obj.typeId() );
    if( !obj.is_owned_by( player_character, true ) ) {
        if( !query_yn( _( "Disassembling the %s may anger the people who own it, continue?" ),
                       obj.tname() ) ) {
            return false;
        } else {
            if( obj.get_owner() ) {
                std::vector<npc *> witnesses;
                for( npc &elem : g->all_npcs() ) {
                    if( rl_dist( elem.pos(), player_character.pos() ) < MAX_VIEW_DISTANCE && elem.get_faction() &&
                        obj.is_owned_by( elem ) && elem.sees( player_character.pos() ) ) {
                        elem.say( "<witnessed_thievery>", 7 );
                        npc *npc_to_add = &elem;
                        witnesses.push_back( npc_to_add );
                    }
                }
                if( !witnesses.empty() ) {
                    if( player_character.add_faction_warning( obj.get_owner() ) ) {
                        for( npc *elem : witnesses ) {
                            elem->make_angry();
                        }
                    }
                }
            }
        }
    }
    // last chance to back out
    if( interactive && get_option<bool>( "QUERY_DISASSEMBLE" ) && obj.typeId() != itype_disassembly ) {
        std::string list;
        const auto components = obj.get_uncraft_components();
        for( const item_comp &component : components ) {
            list += "- " + component.to_string() + "\n";
        }
        if( !r.learn_by_disassembly.empty() && !knows_recipe( &r ) && can_decomp_learn( r ) ) {
            if( !query_yn(
                    _( "Disassembling the %s may yield:\n%s\nReally disassemble?\nYou feel you may be able to understand this object's construction.\n" ),
                    colorize( obj.tname(), obj.color_in_inventory() ),
                    list ) ) {
                return false;
            }
        } else if( !query_yn( _( "Disassembling the %s may yield:\n%s\nReally disassemble?" ),
                              colorize( obj.tname(), obj.color_in_inventory() ),
                              list ) ) {
            return false;
        }
    }

    if( activity.id() != ACT_DISASSEMBLE ) {
        // When disassembling items with charges, prompt the player to specify amount
        int num_dis = 1;
        if( obj.count_by_charges() ) {
            if( !disassemble_all && obj.charges > 1 ) {
                string_input_popup popup_input;
                const std::string title = string_format( _( "Disassemble how many %s [MAX: %d]: " ),
                                          obj.type_name( 1 ), obj.charges );
                popup_input.title( title ).edit( num_dis );
                if( popup_input.canceled() || num_dis <= 0 ) {
                    add_msg( _( "Never mind." ) );
                    return false;
                }
                num_dis = std::min( num_dis, obj.charges );
            } else {
                num_dis = obj.charges;
            }
        }
        const int64_t craft_moves = r.time_to_craft_moves( *this, recipe_time_flag::ignore_proficiencies );
        const int num = obj.typeId() != itype_disassembly ? num_dis : obj.get_making_batch_size();
        player_activity new_act( disassemble_activity_actor( num * craft_moves ) );
        new_act.targets.emplace_back( std::move( target ) );

        // index is used as a bool that indicates if we want recursive uncraft.
        new_act.index = false;
        new_act.position = num_dis;
        new_act.values.push_back( disassemble_all );
        assign_activity( new_act );
    } else {
        // index is used as a bool that indicates if we want recursive uncraft.
        activity.index = false;
        activity.targets.emplace_back( std::move( target ) );

        if( activity.moves_left <= 0 ) {
            activity.moves_left = r.time_to_craft_moves( *this, recipe_time_flag::ignore_proficiencies );
        }
    }

    return true;
}

void Character::disassemble_all( bool one_pass )
{
    // Reset all the activity values
    assign_activity( player_activity() );

    bool found_any = false;
    std::vector<item_location> to_disassemble;
    for( item &it : get_map().i_at( pos() ) ) {
        to_disassemble.emplace_back( map_cursor( pos() ), &it );
    }
    for( item_location &it_loc : to_disassemble ) {
        // Prevent disassembling an in process disassembly because it could have been created by a previous iteration of this loop
        // and choosing to place it on the ground
        if( disassemble( it_loc, false, true ) ) {
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

void Character::complete_disassemble( item_location target )
{
    // Disassembly has 2 parallel vectors:
    // item location, and recipe id
    item temp = target.get_item()->components.only_item();
    const recipe &rec = recipe_dictionary::get_uncraft( temp.typeId() );

    if( rec ) {
        complete_disassemble( target, rec );
    } else {
        debugmsg( "bad disassembly recipe: %s", temp.type_name() );
        activity.set_to_null();
        return;
    }

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
    item *next_item = activity.targets.back().get_item();
    if( !next_item || next_item->is_null() ) {
        debugmsg( "bad item" );
        activity.set_to_null();
        return;
    }
    // Set get and set duration of next uncraft
    const recipe &next_recipe = recipe_dictionary::get_uncraft( ( next_item->typeId() ==
                                itype_disassembly ) ? next_item->components.only_item().typeId() : next_item->typeId() );

    if( !next_recipe ) {
        debugmsg( "bad disassembly recipe" );
        activity.set_to_null();
        return;
    }
    int num_dis = 1;
    const item &obj = *activity.targets.back().get_item();
    if( obj.count_by_charges() ) {
        // get_value( 0 ) is true if the player wants to disassemble all charges
        if( !activity.get_value( 0 ) && obj.charges > 1 ) {
            string_input_popup popup_input;
            const std::string title = string_format( _( "Disassemble how many %s [MAX: %d]: " ),
                                      obj.type_name( 1 ), obj.charges );
            popup_input.title( title ).edit( num_dis );
            if( popup_input.canceled() || num_dis <= 0 ) {
                add_msg( _( "Never mind." ) );
                activity.set_to_null();
                return;
            }
            num_dis = std::min( num_dis, obj.charges );
        } else {
            num_dis = obj.charges;
        }
    }
    int64_t moves = next_recipe.time_to_craft_moves( *this, recipe_time_flag::ignore_proficiencies );
    player_activity new_act( disassemble_activity_actor( moves * num_dis ) );
    new_act.targets = activity.targets;
    new_act.index = activity.index;
    new_act.position = num_dis;
    assign_activity( new_act );
}

void Character::complete_disassemble( item_location &target, const recipe &dis )
{
    // Get the proper recipe - the one for disassembly, not assembly
    const requirement_data dis_requirements = dis.disassembly_requirements();
    const tripoint_bub_ms loc = target.pos_bub();

    // Get the item to disassemble, and make a copy to keep its data (damage/components)
    // after the original has been removed.
    item org_item = target.get_item()->components.only_item();
    item dis_item = org_item;

    if( this->is_avatar() ) {
        add_msg( _( "You disassemble the %s into its components." ), dis_item.tname() );
    } else {
        add_msg_if_player_sees( *this, _( "%1s disassembles the %2s into its components." ),
                                this->disp_name( false, true ), dis_item.tname() );
    }

    // Get rid of the disassembled item
    target.remove_item();

    // Consume tool charges
    for( const auto &it : dis_requirements.get_tools() ) {
        consume_tools( it );
    }

    // add the components to the map

    // If the components aren't empty, we want items exactly identical to them
    // Even if the best-fit recipe does not involve those items
    item_components components = dis_item.components;

    // If the components are empty, item is the default kind and made of default components
    if( components.empty() ) {
        for( const auto &altercomps : dis_requirements.get_components() ) {
            const item_comp &comp = altercomps.front();
            int compcount = comp.count;
            item newit( comp.type, calendar::turn );
            if( dis_item.count_by_charges() ) {
                compcount *= activity.position;
            }
            const bool is_liquid = newit.made_of( phase_id::LIQUID );
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

            // If the recipe has a `FULL_MAGAZINE` flag, spawn any magazines full of ammo
            if( newit.is_magazine() && dis.has_flag( flag_FULL_MAGAZINE ) ) {
                newit.ammo_set( newit.ammo_default(),
                                newit.ammo_capacity( item::find_type( newit.ammo_default() )->ammo->type ) );
            }

            for( ; compcount > 0; compcount-- ) {
                components.add( newit );
            }
        }
    }

    // Player skills should determine how many components are returned
    int skill_dice = 2 + round( get_skill_level( dis.skill_used ) * 4 );

    // Sides on dice is 16 plus your current intelligence
    ///\EFFECT_INT increases success rate for disassembling items
    int skill_sides = 16 + int_cur;

    int diff_dice = dis.difficulty;
    int diff_sides = 24; // 16 + 8 (default intelligence)

    // disassembly only nets a bit of practice
    if( dis.skill_used ) {
        practice( dis.skill_used, ( dis.difficulty ) * 2, dis.difficulty );
    }

    // Item damage_level (0-4) reduces chance of success (0.8^lvl =~ 100%, 80%, 64%, 51%, 41%)
    const float component_success_chance = std::pow( 0.8, dis_item.damage_level() );

    // Recovered component items to be dropped
    std::list<item> drop_items;

    // Recovered and destroyed item types and count of each
    std::map<itype_id, int> recover_tally;
    std::map<itype_id, int> destroy_tally;

    // Roll skill and damage checks for successful recovery of each component
    for( const item_components::type_vector_pair &tvp : components ) {
        for( const item &newit : tvp.second ) {
            if( newit.has_flag( flag_UNRECOVERABLE ) ) {
                continue;
            }
            // Use item type to index recover/destroy tallies
            const itype_id it_type_id = newit.typeId();
            // Chance of failure based on character skill and recipe difficulty
            const bool comp_success = dice( skill_dice, skill_sides ) > dice( diff_dice, diff_sides );
            // If original item was damaged, there is another chance for recovery to fail
            const bool dmg_success = component_success_chance > rng_float( 0, 1 );

            // If component recovery failed, tally it and continue with the next component
            if( ( dis.difficulty != 0 && !comp_success ) || !dmg_success ) {
                // Count destroyed items
                if( destroy_tally.count( it_type_id ) == 0 ) {
                    destroy_tally[it_type_id] = newit.count();
                } else {
                    destroy_tally[it_type_id] += newit.count();
                }
                continue;
            }

            // Component recovered successfully; add to the tally
            if( recover_tally.count( it_type_id ) == 0 ) {
                recover_tally[it_type_id] = newit.count();
            } else {
                recover_tally[it_type_id] += newit.count();
            }

            // Use item from components list, or (if not contained)
            // use newit, the default constructed.
            item act_item = newit;

            if( act_item.has_temperature() ) {
                // TODO: fix point types
                act_item.set_item_temperature( get_weather().get_temperature( loc.raw() ) );
            }

            // Refitted clothing disassembles into refitted components (when applicable)
            if( dis_item.has_flag( flag_FIT ) && act_item.has_flag( flag_VARSIZE ) ) {
                act_item.set_flag( flag_FIT );
            }

            // Filthy items yield filthy components
            if( dis_item.is_filthy() ) {
                act_item.set_flag( flag_FILTHY );
            }

            act_item.set_relative_rot( dis_item.get_relative_rot() );

            ret_val<item> removed = dis_item.components.remove( newit.typeId() );
            if( removed.success() ) {
                act_item = removed.value();
            }

            //NPCs are too dumb to be able to handle liquid (for now)
            if( act_item.made_of( phase_id::LIQUID ) && !is_npc() ) {
                liquid_handler::handle_all_liquid( act_item, PICKUP_RANGE );
            } else {
                drop_items.push_back( act_item );
            }
        }
    }

    // Log how many of each component failed to be recovered
    for( std::pair<itype_id, int> destroyed : destroy_tally ) {
        // Get name of item, pluralized for its quantity
        const std::string it_name = item( destroyed.first ).type_name( destroyed.second );
        if( this->is_avatar() ) {
            //~ %1$d: quantity destroyed, %2$s: pluralized name of destroyed item
            add_msg( m_bad, _( "You fail to recover %1$d %2$s." ), destroyed.second, it_name );
        } else {
            //~ %1$s: NPC name, %2$d: quantity destroyed, %2$s: pluralized name of destroyed item
            add_msg_if_player_sees( *this, m_bad, _( "%1$s fails to recover %2$d %3$s." ),
                                    this->disp_name( false, true ), destroyed.second, it_name );
        }
    }

    // Log how many of each component were recovered successfully
    for( std::pair<itype_id, int> recovered : recover_tally ) {
        // Get name of item, pluralized for its quantity
        const std::string it_name = item( recovered.first ).type_name( recovered.second );
        // Recovery successful; inform player
        if( this->is_avatar() ) {
            //~ %1$d: quantity recovered, %2$s: pluralized name of recovered item
            add_msg( m_good, _( "You recover %1$d %2$s." ), recovered.second, it_name );
        } else {
            //~ %1$s: NPC name, %2$d: quantity recovered, %2$s: pluralized name of recovered item
            add_msg_if_player_sees( *this, m_good, _( "%1$s recovers %2$d %3$s." ),
                                    this->disp_name( false, true ), recovered.second, it_name );
        }
    }

    // Drop all recovered components
    put_into_vehicle_or_drop( *this, item_drop_reason::deliberate, drop_items, loc );

    if( !dis.learn_by_disassembly.empty() && !knows_recipe( &dis ) ) {
        if( can_decomp_learn( dis ) ) {
            // TODO: make this depend on intelligence
            if( one_in( 4 ) ) {
                // TODO: change to forward an id or a reference
                learn_recipe( &dis.ident().obj() );
                add_msg_if_player( m_good, _( "You learned a recipe for %s from disassembling it!" ),
                                   dis_item.tname() );
            } else {
                add_msg_if_player( m_info,
                                   _( "You might be able to learn a recipe for %s if you disassemble another." ),
                                   dis_item.tname() );
            }
        } else {
            add_msg_if_player( m_info, _( "If you had better skills, you might learn a recipe next time." ) );
        }
    }
}

void remove_ammo( std::list<item> &dis_items, Character &p )
{
    for( item &dis_item : dis_items ) {
        remove_ammo( dis_item, p );
    }
}

void drop_or_handle( const item &newit, Character &p )
{
    if( newit.made_of( phase_id::LIQUID ) && p.is_avatar() ) { // TODO: what about NPCs?
        liquid_handler::handle_all_liquid( newit, PICKUP_RANGE );
    } else {
        item tmp( newit );
        p.i_add_or_drop( tmp );
    }
}

void remove_ammo( item &dis_item, Character &p )
{
    dis_item.remove_items_with( [&p]( const item & it ) {
        if( it.is_irremovable() || !( it.is_gunmod() || it.is_toolmod() || ( it.is_magazine() &&
                                      !it.is_tool() ) ) ) {
            return false;
        }
        drop_or_handle( it, p );
        return true;
    } );

    if( dis_item.has_flag( flag_NO_UNLOAD ) ) {
        return;
    }
    if( dis_item.is_gun() && !dis_item.ammo_current().is_null() ) {
        item ammodrop( dis_item.ammo_current(), calendar::turn );
        ammodrop.charges = dis_item.charges;
        drop_or_handle( ammodrop, p );
        dis_item.charges = 0;
    }
    if( dis_item.is_tool() && dis_item.charges > 0 && !dis_item.ammo_current().is_null() ) {
        item ammodrop( dis_item.ammo_current(), calendar::turn );
        ammodrop.charges = dis_item.charges;
        if( dis_item.ammo_current() == itype_plut_cell ) {
            ammodrop.charges /= PLUTONIUM_CHARGES;
        }
        drop_or_handle( ammodrop, p );
        dis_item.charges = 0;
    }
}

std::vector<npc *> Character::get_crafting_helpers() const
{
    return g->get_npcs_if( [this]( const npc & guy ) {
        // NPCs can help craft if awake, taking orders, within pickup range and have clear path
        return getID() != guy.getID() && !guy.in_sleep_state() && guy.is_obeying( *this ) &&
               rl_dist( guy.pos(), pos() ) < PICKUP_RANGE &&
               get_map().clear_path( pos(), guy.pos(), PICKUP_RANGE, 1, 100 );
    } );
}

static bool is_anyone_crafting( const item_location &itm, const Character *you = nullptr )
{
    for( const npc &guy : g->all_npcs() ) {
        if( !( you && you->getID() == guy.getID() ) &&
            guy.activity.id() == ACT_CRAFT ) {
            item_location target = guy.activity.targets.back();
            if( target == itm ) {
                return true;
            }
        }
    }
    return false;
}

item_location npc::get_item_to_craft()
{
    // check inventory
    item_location to_craft;
    visit_items( [ this, &to_craft ]( item * itm, item * ) {
        if( itm->get_var( "crafter", "" ) == name ) {
            to_craft = item_location( *this, itm );
            if( !is_anyone_crafting( to_craft, this ) ) {
                return VisitResponse::ABORT;
            }
        }
        return VisitResponse::NEXT;
    } );
    if( to_craft ) {
        return to_craft;
    }

    // check items around npc
    map &here = get_map();
    for( const tripoint &adj : here.points_in_radius( pos(), 1 ) ) {
        if( here.dangerous_field_at( adj ) ) {
            continue;
        }
        for( item &itm : here.i_at( adj ) ) {
            if( itm.get_var( "crafter", "" ) == name ) {
                to_craft = item_location( map_cursor( adj ), &itm );
                if( !is_anyone_crafting( to_craft, this ) ) {
                    return to_craft;
                }
            }
        }
    }
    return to_craft;
}

void npc::do_npc_craft( const std::optional<tripoint> &loc, const recipe_id &goto_recipe )
{
    std::vector<item_location> craft_item_list;
    std::string dummy;

    visit_items( [ this, &craft_item_list, &dummy ]( item * itm, item * ) {
        if( itm->is_craft() && itm->get_making().npc_can_craft( dummy ) ) {
            item_location to_craft = item_location( *this, itm );
            if( !is_anyone_crafting( to_craft, this ) ) {
                craft_item_list.push_back( to_craft );
            }
        }
        return VisitResponse::NEXT;
    } );

    map &here = get_map();
    for( const tripoint &adj : here.points_in_radius( pos(), 1 ) ) {
        if( here.dangerous_field_at( adj ) ) {
            continue;
        }
        for( item &itm : here.i_at( adj ) ) {
            if( itm.is_craft() && itm.get_making().npc_can_craft( dummy ) ) {
                item_location to_craft = item_location( map_cursor( adj ), &itm );
                if( !is_anyone_crafting( to_craft, this ) ) {
                    craft_item_list.push_back( to_craft );
                }
            }
        }
    }

    if( craft_item_list.empty() ) {
        craft( loc, goto_recipe );
    } else {
        uilist menu;
        menu.text = "Craft what?";
        menu.addentry( 0, true, MENU_AUTOASSIGN, _( "Craft new item" ) );
        menu.addentry( 1, true, MENU_AUTOASSIGN, _( "Work on craft" ) );
        menu.query();

        if( menu.ret == 0 ) {
            craft( loc, goto_recipe );
        } else if( menu.ret == 1 ) {
            int selected = 0;
            uilist item_selection;
            do {
                item_selection.init();
                item_selection.text = "Craft what?";
                item_selection.selected = selected;
                item_selection.addentry( 0, true, MENU_AUTOASSIGN, "Start crafting" );
                item_selection.addentry( 1, true, MENU_AUTOASSIGN, "Select all" );
                int index = 2;

                for( item_location &itm : craft_item_list ) {
                    // set flag to craft
                    std::string entry;
                    bool enable = itm->get_var( "crafter", "" ) == name;
                    if( selected == 1 ) {
                        itm->set_var( "crafter", name );
                        enable = true;
                    } else if( selected == index ) {
                        if( enable ) {
                            itm->erase_var( "crafter" );
                            enable = false;
                        } else {
                            itm->set_var( "crafter", name );
                            enable = true;
                        }
                    }
                    if( enable ) {
                        entry = string_format( "[x] %s", itm->tname() );
                    } else {
                        entry = string_format( "[ ] %s", itm->tname() );
                    }
                    item_selection.addentry( index, true, MENU_AUTOASSIGN, entry );
                    index++;
                }
                item_selection.query();

                selected = item_selection.ret;
                if( selected == 0 ) {
                    assign_activity( ACT_MULTIPLE_CRAFT );
                }
            } while( selected >= 1 );
        }
    }
}
