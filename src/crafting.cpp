#include "crafting.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

#include "activity_handlers.h"
#include "ammo.h"
#include "bionics.h"
#include "calendar.h"
#include "craft_command.h"
#include "debug.h"
#include "game.h"
#include "game_inventory.h"
#include "inventory.h"
#include "item.h"
#include "itype.h"
#include "map.h"
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
#include "vpart_position.h"
#include "vpart_reference.h"

const efftype_id effect_contacts( "contacts" );

void drop_or_handle( const item &newit, player &p );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );
static const trait_id trait_PAWS_LARGE( "PAWS_LARGE" );
static const trait_id trait_PAWS( "PAWS" );
static const trait_id trait_BURROW( "BURROW" );

static bool crafting_allowed( const player &p, const recipe &rec )
{
    if( p.morale_crafting_speed_multiplier( rec ) <= 0.0f ) {
        add_msg( m_info, _( "Your morale is too low to craft..." ) );
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

float player::crafting_speed_multiplier( const recipe &rec, bool in_progress ) const
{
    float result = morale_crafting_speed_multiplier( rec ) * lighting_craft_speed_multiplier( rec );
    // Can't start if we'd need 300% time, but we can still finish the job
    if( !in_progress && result < 0.33f ) {
        return 0.0f;
    }
    // If we're working below 20% speed, just give up
    if( result < 0.2f ) {
        return 0.0f;
    }

    return result;
}

bool player::has_morale_to_craft() const
{
    return get_morale_level() >= -50;
}

void player::craft()
{
    int batch_size = 0;
    const recipe *rec = select_crafting_recipe( batch_size );
    if( rec ) {
        if( crafting_allowed( *this, *rec ) ) {
            make_craft( rec->ident(), batch_size );
        }
    }
}

void player::recraft()
{
    if( lastrecipe.str().empty() ) {
        popup( _( "Craft something first" ) );
    } else if( making_would_work( lastrecipe, last_batch ) ) {
        last_craft->execute();
    }
}

void player::long_craft()
{
    int batch_size = 0;
    const recipe *rec = select_crafting_recipe( batch_size );
    if( rec ) {
        if( crafting_allowed( *this, *rec ) ) {
            make_all_craft( rec->ident(), batch_size );
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

size_t available_assistant_count( const player &u, const recipe &rec )
{
    // NPCs around you should assist in batch production if they have the skills
    // @todo Cache them in activity, include them in modifier calculations
    const auto helpers = u.get_crafting_helpers();
    return std::count_if( helpers.begin(), helpers.end(),
    [&]( const npc * np ) {
        return np->get_skill_level( rec.skill_used ) >= rec.difficulty;
    } );
}

int player::base_time_to_craft( const recipe &rec, int batch_size ) const
{
    const size_t assistants = available_assistant_count( *this, rec );
    return rec.batch_time( batch_size, 1.0f, assistants );
}

int player::expected_time_to_craft( const recipe &rec, int batch_size ) const
{
    const size_t assistants = available_assistant_count( *this, rec );
    float modifier = crafting_speed_multiplier( rec );
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

        long charges_to_store = prod.charges;
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
            popup( _( "You don't have anything to store %s in!" ), prod.tname().c_str() );
            return false;
        }
    }

    return true;
}

bool is_container_eligible_for_crafting( const item &cont, bool allow_bucket )
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

    return r->requirements().can_make_with_inventory( crafting_inv, batch_size );
}

const inventory &player::crafting_inventory()
{
    if( cached_moves == moves
        && cached_time == calendar::turn
        && cached_position == pos() ) {
        return cached_crafting_inventory;
    }
    cached_crafting_inventory.form_from_map( pos(), PICKUP_RANGE, false );
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
    cached_position = pos();
    return cached_crafting_inventory;
}

void player::invalidate_crafting_inventory()
{
    cached_time = calendar::before_time_starts;
}

void player::make_craft( const recipe_id &id_to_make, int batch_size )
{
    make_craft_with_command( id_to_make, batch_size );
}

void player::make_all_craft( const recipe_id &id_to_make, int batch_size )
{
    make_craft_with_command( id_to_make, batch_size, true );
}

void player::make_craft_with_command( const recipe_id &id_to_make, int batch_size, bool is_long )
{
    const auto &recipe_to_make = *id_to_make;

    if( !recipe_to_make ) {
        return;
    }

    *last_craft = craft_command( &recipe_to_make, batch_size, is_long, this );
    last_craft->execute();
}

// @param offset is the index of the created item in the range [0, batch_size-1],
// it makes sure that the used items are distributed equally among the new items.
void set_components( std::vector<item> &components, const std::list<item> &used,
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

std::list<item> player::consume_components_for_craft( const recipe &making, int batch_size,
        bool ignore_last )
{
    std::list<item> used;
    if( has_trait( trait_id( "DEBUG_HS" ) ) ) {
        return used;
    }
    if( last_craft->has_cached_selections() && !ignore_last ) {
        used = last_craft->consume_components();
    } else {
        // This should fail and return, but currently crafting_command isn't saved
        // Meaning there are still cases where has_cached_selections will be false
        // @todo: Allow saving last_craft and debugmsg+fail craft if selection isn't cached
        const auto &req = making.requirements();
        for( const auto &it : req.get_components() ) {
            std::list<item> tmp = consume_items( it, batch_size );
            used.splice( used.end(), tmp );
        }
        for( const auto &it : req.get_tools() ) {
            consume_tools( it, batch_size );
        }
    }
    return used;
}

std::list<item> player::consume_some_components_for_craft( const recipe &making, int batch_size )
{
    std::list<item> used;
    if( has_trait( trait_id( "DEBUG_HS" ) ) ) {
        return used;
    }
    const auto &req = making.requirements();
    int cou = 0;
    for( const auto &it : req.get_components() ) {
        // Each component currently has 50% chance of not being consumed
        // Skip first item so failed craft with one item recipe always loses component
        if( cou == 0 || one_in( 2 ) ) {
            std::list<item> tmp = consume_items( it, batch_size );
            used.splice( used.end(), tmp );
        }
        ++cou;
    }
    for( const auto &it : req.get_tools() ) {
        consume_tools( it, batch_size );
    }
    return used;
}

static void set_item_food( item &newit )
{
    //@todo: encapsulate this into some function
    int bday_tmp = to_turn<int>( newit.birthday() ) % 3600; // fuzzy birthday for stacking reasons
    newit.set_birthday( newit.birthday() + 3600_turns - time_duration::from_turns( bday_tmp ) );
}

static void finalize_crafted_item( item &newit )
{
    if( newit.is_food() ) {
        set_item_food( newit );
    }
}

static void set_item_inventory( item &newit )
{
    if( newit.made_of( LIQUID ) ) {
        g->handle_all_liquid( newit, PICKUP_RANGE );
    } else {
        g->u.inv.assign_empty_invlet( newit, g->u );
        // We might not have space for the item
        if( !g->u.can_pickVolume( newit ) ) { //Accounts for result_mult
            put_into_vehicle_or_drop( g->u, item_drop_reason::too_large, { newit } );
        } else if( !g->u.can_pickWeight( newit, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) ) {
            put_into_vehicle_or_drop( g->u, item_drop_reason::too_heavy, { newit } );
        } else {
            newit = g->u.i_add( newit );
            add_msg( m_info, "%c - %s", newit.invlet == 0 ? ' ' : newit.invlet, newit.tname().c_str() );
        }
    }
}

time_duration get_rot_since( const time_point &start, const time_point &end,
                             const tripoint &location ); // weather.cpp

void player::complete_craft()
{
    const recipe &making = recipe_id( activity.name ).obj(); // Which recipe is it?
    int batch_size = activity.values.front();
    if( making.ident().is_null() ) {
        debugmsg( "no recipe with id %s found", activity.name );
        activity.set_to_null();
        return;
    }

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

    auto helpers = g->u.get_crafting_helpers();
    for( const npc *np : helpers ) {
        if( np->get_skill_level( making.skill_used ) >=
            get_skill_level( making.skill_used ) ) {
            // NPC assistance is worth half a skill level
            skill_dice += 2;
            add_msg( m_info, _( "%s helps with crafting..." ), np->name.c_str() );
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
    int skill_sides = 16 + int_cur;

    int diff_dice;
    if( secondary_difficulty > 0 ) {
        diff_dice = making.difficulty * 3 + secondary_difficulty;
    } else {
        // Since skill level is * 4 also
        diff_dice = making.difficulty * 4;
    }

    int diff_sides = 24; // 16 + 8 (default intelligence)

    int skill_roll = dice( skill_dice, skill_sides );
    int diff_roll  = dice( diff_dice,  diff_sides );

    if( making.skill_used ) {
        // normalize experience gain to crafting time, giving a bonus for longer crafting
        const double batch_mult = batch_size + base_time_to_craft( making, batch_size ) / 30000.0;
        const int base_practice = ( making.difficulty * 15 + 10 ) * batch_mult;
        const int skill_cap = static_cast<int>( making.difficulty * 1.25 );
        practice( making.skill_used, base_practice, skill_cap );

        //NPCs assisting or watching should gain experience...
        for( auto &helper : helpers ) {
            //If the NPC can understand what you are doing, they gain more exp
            if( helper->get_skill_level( making.skill_used ) >= making.difficulty ) {
                helper->practice( making.skill_used,
                                  static_cast<int>( base_practice * 0.50 ),
                                  skill_cap );
                if( batch_size > 1 ) {
                    add_msg( m_info, _( "%s assists with crafting..." ), helper->name );
                }
                if( batch_size == 1 ) {
                    add_msg( m_info, _( "%s could assist you with a batch..." ), helper->name );
                }
                //NPCs around you understand the skill used better
            } else {
                helper->practice( making.skill_used,
                                  static_cast<int>( base_practice * 0.15 ),
                                  skill_cap );
                add_msg( m_info, _( "%s watches you craft..." ), helper->name );
            }
        }
    }

    // Messed up badly; waste some components.
    if( making.difficulty != 0 && diff_roll > skill_roll * ( 1 + 0.1 * rng( 1, 5 ) ) ) {
        add_msg( m_bad, _( "You fail to make the %s, and waste some materials." ), making.result_name() );
        consume_some_components_for_craft( making, batch_size );
        activity.set_to_null();
        return;
        // Messed up slightly; no components wasted.
    } else if( diff_roll > skill_roll ) {
        add_msg( m_neutral, _( "You fail to make the %s, but don't waste any materials." ),
                 making.result_name() );
        //this method would only have been called from a place that nulls activity.type,
        //so it appears that it's safe to NOT null that variable here.
        //rationale: this allows certain contexts (e.g. ACT_LONGCRAFT) to distinguish major and minor failures
        return;
    }

    // If we're here, the craft was a success!
    // Use up the components and tools
    std::list<item> used = consume_components_for_craft( making, batch_size );
    if( last_craft->has_cached_selections() && used.empty() ) {
        // This signals failure, even though there seem to be several paths where it shouldn't...
        return;
    }
    if( !used.empty() ) {
        reset_encumbrance();  // in case we were wearing something just consumed up.
    }

    const time_point now = calendar::turn;
    time_point start_turn = now;
    tripoint craft_pos = pos();
    if( activity.values.size() > 1 && !activity.coords.empty() ) {
        start_turn = activity.values.at( 1 );
        craft_pos = activity.coords.at( 0 );
    } else {
        // either something went wrong or player had an old binary and saved
        // the game right in the middle of crafting, and then updated their
        // binary, so we didn't grab these values before starting the craft
        debugmsg( "Missing activity start time and temperature, using current val" );
    }
    const time_duration rot_points = get_rot_since( start_turn, now, craft_pos );
    double max_relative_rot = 0;
    // We need to cycle all the used ingredients and find the most rotten item,
    // this will then set our relative rot for the crafted items.
    for( const item &it : used ) {
        if( !it.goes_bad() ) {
            continue;
        }
        // make a copy of the item so we can play with its rot values
        item it_copy = it;
        it_copy.mod_rot( -rot_points );
        max_relative_rot = std::max( max_relative_rot, it_copy.get_relative_rot() );
    }

    // Set up the new item, and assign an inventory letter if available
    std::vector<item> newits = making.create_results( batch_size );

    // Check if the recipe tools make this food item hot upon making it.
    // We don't actually know which specific tool the player used here, but
    // we're checking for a class of tools; because of the way requirements
    // processing works, the "surface_heat" id gets nuked into an actual
    // list of tools, see data/json/recipes/cooking_tools.json.
    //
    // Currently it's only checking for a hotplate because that's a
    // suitable item in both the "surface_heat" and "water_boiling_heat"
    // tools, and it's usually the first item in a list of tools so if this
    // does get heated we'll find it right away.
    bool should_heat = false;
    if( !newits.empty() && newits.front().is_food() ) {
        const requirement_data::alter_tool_comp_vector &tool_lists = making.requirements().get_tools();
        for( const std::vector<tool_comp> &tools : tool_lists ) {
            for( const tool_comp &t : tools ) {
                if( t.type == "hotplate" ) {
                    should_heat = true;
                    break;
                }
            }
            // if we've already decided to heat it up then we're done
            if( should_heat ) {
                break;
            }
        }
    }

    bool first = true;
    size_t newit_counter = 0;
    for( item &newit : newits ) {
        // messages, learning of recipe, food spoilage calculation only once
        if( first ) {
            first = false;
            if( knows_recipe( &making ) ) {
                add_msg( _( "You craft %s from memory." ), newit.type_name( 1 ).c_str() );
            } else {
                add_msg( _( "You craft %s using a book as a reference." ), newit.type_name( 1 ).c_str() );
                // If we made it, but we don't know it,
                // we're making it from a book and have a chance to learn it.
                // Base expected time to learn is 1000*(difficulty^4)/skill/int moves.
                // This means time to learn is greatly decreased with higher skill level,
                // but also keeps going up as difficulty goes up.
                // Worst case is lvl 10, which will typically take
                // 10^4/10 (1,000) minutes, or about 16 hours of crafting it to learn.
                int difficulty = has_recipe( &making, crafting_inventory(), helpers );
                ///\EFFECT_INT increases chance to learn recipe when crafting from a book
                if( x_in_y( making.time, ( 1000 * 8 *
                                           ( difficulty * difficulty * difficulty * difficulty ) ) /
                            ( std::max( get_skill_level( making.skill_used ), 1 ) * std::max( get_int(), 1 ) ) ) ) {
                    learn_recipe( &making );
                    add_msg( m_good, _( "You memorized the recipe for %s!" ),
                             newit.type_name( 1 ).c_str() );
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
                if( comp.is_comestible() && !comp.type->comestible->cooks_like.empty() ) {
                    comp = item( comp.type->comestible->cooks_like, comp.birthday(), comp.charges );
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
            newit.set_relative_rot( max_relative_rot );
        } else {
            if( newit.is_container() ) {
                for( item &in : newit.contents ) {
                    if( in.goes_bad() ) {
                        in.set_relative_rot( max_relative_rot );
                    }
                }
            }
        }

        if( should_heat ) {
            newit.heat_up();
        } else {
            // Really what we should be doing is averaging the temperatures
            // between the recipe components if we don't have a heat tool, but
            // that's kind of hard.  For now just reset the temperature, don't
            // forget byproducts below either when you fix this.
            //
            // Also, this is going to reset the temperature on non-food items
            // as well.  This isn't a big deal because they don't care about
            // temperature (yet?) and it's actually more costly to check for
            // is_food() than just reset the timer.
            newit.reset_temp_check();
        }

        finalize_crafted_item( newit );
        set_item_inventory( newit );
    }

    if( making.has_byproducts() ) {
        std::vector<item> bps = making.create_byproducts( batch_size );
        for( auto &bp : bps ) {
            if( bp.goes_bad() ) {
                bp.set_relative_rot( max_relative_rot );
            }
            if( should_heat ) {
                bp.heat_up();
            } else {
                bp.reset_temp_check();
            }
            finalize_crafted_item( bp );
            set_item_inventory( bp );
        }
    }

    inv.restack( *this );
}

/* selection of component if a recipe requirement has multiple options (e.g. 'duct tap' or 'welder') */
comp_selection<item_comp> player::select_item_component( const std::vector<item_comp> &components,
        int batch, inventory &map_inv, bool can_cancel,
        const std::function<bool( const item & )> &amount_filter,
        const std::function<bool( const item & )> &charges_filter )
{
    std::vector<item_comp> player_has;
    std::vector<item_comp> map_has;
    std::vector<item_comp> mixed;

    comp_selection<item_comp> selected;

    for( const auto &component : components ) {
        itype_id type = component.type;
        int count = ( component.count > 0 ) ? component.count * batch : abs( component.count );
        bool found = false;

        if( item::count_by_charges( type ) && count > 0 ) {
            long map_charges = map_inv.charges_of( type, std::numeric_limits<long>::max(), charges_filter );

            // If map has infinite charges, just use them
            if( map_charges == item::INFINITE_CHARGES ) {
                selected.use_from = use_from_map;
                selected.comp = component;
                return selected;
            }

            long player_charges = charges_of( type, std::numeric_limits<long>::max(), charges_filter );

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
        } else { // Counting by units, not charges

            // Can't use pseudo items as components
            if( has_amount( type, count, false, amount_filter ) ) {
                player_has.push_back( component );
                found = true;
            }
            if( map_inv.has_components( type, count ) ) {
                map_has.push_back( component );
                found = true;
            }
            if( !found &&
                amount_of( type, false, std::numeric_limits<int>::max(), amount_filter ) +
                map_inv.amount_of( type, false, std::numeric_limits<int>::max(),
                                   amount_filter ) >= count ) {
                mixed.push_back( component );
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
                                                map_inv.charges_of( map_ha.type, std::numeric_limits<int>::max(), charges_filter ) :
                                                map_inv.amount_of( map_ha.type, false, std::numeric_limits<int>::max(), amount_filter ) );
            cmenu.addentry( tmpStr );
        }
        for( auto &player_ha : player_has ) { // Index map_has.size()-(map_has.size()+player_has.size()-1)
            std::string tmpStr = string_format( _( "%s (%d/%d on person)" ),
                                                item::nname( player_ha.type ),
                                                ( player_ha.count * batch ),
                                                item::count_by_charges( player_ha.type ) ?
                                                charges_of( player_ha.type, std::numeric_limits<int>::max(), charges_filter ) :
                                                amount_of( player_ha.type, false, std::numeric_limits<int>::max(), amount_filter ) );
            cmenu.addentry( tmpStr );
        }
        for( auto &component : mixed ) {
            // Index player_has.size()-(map_has.size()+player_has.size()+mixed.size()-1)
            long available = item::count_by_charges( component.type ) ?
                             map_inv.charges_of( component.type, std::numeric_limits<int>::max(), charges_filter ) +
                             charges_of( component.type, std::numeric_limits<int>::max(), charges_filter ) :
                             map_inv.amount_of( component.type, false, std::numeric_limits<int>::max(), amount_filter ) +
                             amount_of( component.type, false, std::numeric_limits<int>::max(), amount_filter );
            std::string tmpStr = string_format( _( "%s (%d/%d nearby & on person)" ),
                                                item::nname( component.type ),
                                                component.count * batch,
                                                available );
            cmenu.addentry( tmpStr );
        }

        // Unlike with tools, it's a bad thing if there aren't any components available
        if( cmenu.entries.empty() ) {
            if( has_trait( trait_id( "DEBUG_HS" ) ) ) {
                selected.use_from = use_from_player;
                return selected;
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
void empty_buckets( player &p )
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
                                       const std::function<bool( const item & )> &amount_filter,
                                       const std::function<bool( const item & )> &charges_filter )
{
    std::list<item> ret;

    if( has_trait( trait_DEBUG_HS ) ) {
        return ret;
    }

    item_comp selected_comp = is.comp;

    const tripoint &loc = pos();
    const bool by_charges = ( item::count_by_charges( selected_comp.type ) && selected_comp.count > 0 );
    // Count given to use_amount/use_charges, changed by those functions!
    long real_count = ( selected_comp.count > 0 ) ? selected_comp.count * batch : abs(
                          selected_comp.count );
    // First try to get everything from the map, than (remaining amount) from player
    if( is.use_from & use_from_map ) {
        if( by_charges ) {
            std::list<item> tmp = g->m.use_charges( loc, PICKUP_RANGE, selected_comp.type, real_count,
                                                    charges_filter );
            ret.splice( ret.end(), tmp );
        } else {
            std::list<item> tmp = g->m.use_amount( loc, PICKUP_RANGE, selected_comp.type,
                                                   real_count, amount_filter );
            remove_ammo( tmp, *this );
            ret.splice( ret.end(), tmp );
        }
    }
    if( is.use_from & use_from_player ) {
        if( by_charges ) {
            std::list<item> tmp = use_charges( selected_comp.type, real_count, charges_filter );
            ret.splice( ret.end(), tmp );
        } else {
            std::list<item> tmp = use_amount( selected_comp.type, real_count, amount_filter );
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
                                       const std::function<bool( const item & )> &amount_filter,
                                       const std::function<bool( const item & )> &charges_filter )
{
    inventory map_inv;
    map_inv.form_from_map( pos(), PICKUP_RANGE );
    return consume_items( select_item_component( components, batch, map_inv, false, amount_filter,
                          charges_filter ),
                          batch, amount_filter, charges_filter );
}

comp_selection<tool_comp>
player::select_tool_component( const std::vector<tool_comp> &tools, int batch, inventory &map_inv,
                               const std::string &hotkeys, bool can_cancel )
{

    comp_selection<tool_comp> selected;

    bool found_nocharge = false;
    std::vector<tool_comp> player_has;
    std::vector<tool_comp> map_has;
    // Use charges of any tools that require charges used
    for( auto it = tools.begin(); it != tools.end() && !found_nocharge; ++it ) {
        itype_id type = it->type;
        if( it->count > 0 ) {
            long count = it->count * batch;
            if( has_charges( type, count ) ) {
                player_has.push_back( *it );
            }
            if( map_inv.has_charges( type, count ) ) {
                map_has.push_back( *it );
            }
        } else if( has_amount( type, 1 ) || map_inv.has_tools( type, 1 ) ) {
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
            if( item::find_type( map_ha.type )->maximum_charges() > 1 ) {
                std::string tmpStr = string_format( _( "%s (%d/%d charges nearby)" ),
                                                    item::nname( map_ha.type ),
                                                    ( map_ha.count * batch ),
                                                    map_inv.charges_of( map_ha.type ) );
                tmenu.addentry( tmpStr );
            } else {
                std::string tmpStr = item::nname( map_ha.type ) + _( " (nearby)" );
                tmenu.addentry( tmpStr );
            }
        }
        for( auto &player_ha : player_has ) {
            if( item::find_type( player_ha.type )->maximum_charges() > 1 ) {
                std::string tmpStr = string_format( _( "%s (%d/%d charges on person)" ),
                                                    item::nname( player_ha.type ),
                                                    ( player_ha.count * batch ),
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

/* we use this if we selected the tool earlier */
void player::consume_tools( const comp_selection<tool_comp> &tool, int batch )
{
    if( has_trait( trait_DEBUG_HS ) ) {
        return;
    }

    if( tool.use_from & use_from_player ) {
        use_charges( tool.comp.type, tool.comp.count * batch );
    }
    if( tool.use_from & use_from_map ) {
        long quantity = tool.comp.count * batch;
        g->m.use_charges( pos(), PICKUP_RANGE, tool.comp.type, quantity );
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

    if( !r ) {
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
            return ret_val<bool>::make_failure( msg, qty, obj.tname().c_str() );
        }
    }

    const auto &dis = r.disassembly_requirements();

    for( const auto &opts : dis.get_qualities() ) {
        for( const auto &qual : opts ) {
            if( !qual.has( inv ) ) {
                // Here should be no dot at the end of the string as 'to_string()' provides it.
                return ret_val<bool>::make_failure( _( "You need %s" ), qual.to_string().c_str() );
            }
        }
    }

    for( const auto &opts : dis.get_tools() ) {
        const bool found = std::any_of( opts.begin(), opts.end(),
        [&]( const tool_comp & tool ) {
            return ( tool.count <= 0 && inv.has_tools( tool.type, 1 ) ) ||
                   ( tool.count >  0 && inv.has_charges( tool.type, tool.count ) );
        } );

        if( !found ) {
            const tool_comp &tool_required = opts.front();
            if( tool_required.count <= 0 ) {
                return ret_val<bool>::make_failure( _( "You need %s." ),
                                                    item::nname( tool_required.type ).c_str() );
            } else {
                return ret_val<bool>::make_failure( ngettext( "You need a %s with %d charge.",
                                                    "You need a %s with %d charges.", tool_required.count ),
                                                    item::nname( tool_required.type ).c_str(),
                                                    tool_required.count );
            }
        }
    }

    return ret_val<bool>::make_success();
}

bool player::disassemble()
{
    auto loc = game_menus::inv::disassemble( *this );

    if( !loc ) {
        add_msg( _( "Never mind." ) );
        return false;
    }

    loc.set_should_stack( false );

    return disassemble( loc.obtain( *this ) );
}

bool player::disassemble( int dis_pos )
{
    return disassemble( i_at( dis_pos ), dis_pos, false );
}

bool player::disassemble( item &obj, int pos, bool ground, bool interactive )
{
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

    activity.values.push_back( pos );
    activity.coords.push_back( ground ? this->pos() : tripoint_min );
    activity.str_values.push_back( r.result() );

    return true;
}

void player::disassemble_all( bool one_pass )
{
    // Reset all the activity values
    assign_activity( activity_id( "ACT_DISASSEMBLE" ), 0 );
    auto items = g->m.i_at( pos() );
    bool found_any = false;
    if( !one_pass ) {
        // Kinda hacky
        // That INT_MIN notes we want infinite uncraft
        // If INT_MIN is reached in complete_disassemble,
        // we will call this function again.
        activity.values.push_back( INT_MIN );
        activity.str_values.push_back( "" );
        activity.coords.push_back( tripoint_min );
    }

    for( size_t i = 0; i < items.size(); i++ ) {
        if( disassemble( items[i], i, true, false ) ) {
            found_any = true;
        }
    }

    if( !found_any ) {
        // Reset the activity - don't loop if there is nothing to do
        activity = player_activity();
    }
}

item &get_item_for_uncraft( player &p, int item_pos,
                            const tripoint &loc, bool from_ground )
{
    item *org_item;
    if( from_ground ) {
        auto items_on_ground = g->m.i_at( loc );
        if( static_cast<size_t>( item_pos ) >= items_on_ground.size() ) {
            return null_item_reference();
        }
        org_item = &items_on_ground[item_pos];
    } else {
        org_item = &p.i_at( item_pos );
    }

    return *org_item;
}

void player::complete_disassemble()
{
    // Clean up old settings
    // Warning: Breaks old saves with disassembly in progress!
    // But so would adding a new recipe...
    if( activity.values.empty() ||
        activity.values.size() != activity.str_values.size() ||
        activity.values.size() != activity.coords.size() ) {
        debugmsg( "bad disassembly activity values" );
        activity.set_to_null();
        return;
    }

    // Disassembly activity is now saved in 3 parallel vectors:
    // item position, tripoint location (tripoint_min for inventory) and recipe

    // Note: we're reading from the back (in inverse order)
    // This is to avoid having to maintain indices
    const int item_pos = activity.values.back();
    const tripoint loc = activity.coords.back();
    const auto recipe_name = activity.str_values.back();

    activity.values.pop_back();
    activity.str_values.pop_back();
    activity.coords.pop_back();

    if( item_pos == INT_MIN ) {
        disassemble_all( false );
        return;
    }

    const bool from_ground = loc != tripoint_min;

    complete_disassemble( item_pos, loc, from_ground, recipe_dictionary::get_uncraft( recipe_name ) );

    if( !activity ) {
        // Something above went wrong, don't continue
        return;
    }

    // Try to get another disassembly target from the activity
    if( activity.values.empty() ) {
        // No more targets
        activity.set_to_null();
        return;
    }

    if( activity.values.back() == INT_MIN ) {
        disassemble_all( false );
        return;
    }

    const auto &next_recipe = recipe_dictionary::get_uncraft( activity.str_values.back() );
    if( !next_recipe ) {
        activity.set_to_null();
        return;
    }

    activity.moves_left = next_recipe.time;
}

// TODO: Make them accessible in a less ugly way
void remove_radio_mod( item &, player & );

void player::complete_disassemble( int item_pos, const tripoint &loc,
                                   bool from_ground, const recipe &dis )
{
    // Get the proper recipe - the one for disassembly, not assembly
    const auto dis_requirements = dis.disassembly_requirements();
    item &org_item = get_item_for_uncraft( *this, item_pos, loc, from_ground );
    bool filthy = org_item.is_filthy();
    if( org_item.is_null() ) {
        add_msg( _( "The item has vanished." ) );
        activity.set_to_null();
        return;
    }

    if( org_item.typeId() != dis.result() ) {
        add_msg( _( "The item might be gone, at least it is not at the expected position anymore." ) );
        activity.set_to_null();
        return;
    }
    // Make a copy to keep its data (damage/components) even after it
    // has been removed.
    item dis_item = org_item;

    float component_success_chance = std::min( std::pow( 0.8, dis_item.damage_level( 4 ) ), 1.0 );

    add_msg( _( "You disassemble the %s into its components." ), dis_item.tname().c_str() );
    // Remove any batteries, ammo and mods first
    remove_ammo( dis_item, *this );
    remove_radio_mod( dis_item, *this );

    if( dis_item.count_by_charges() ) {
        // remove the charges that one would get from crafting it
        org_item.charges -= dis.create_result().charges;
    }
    // remove the item, except when it's counted by charges and still has some
    if( !org_item.count_by_charges() || org_item.charges <= 0 ) {
        if( from_ground ) {
            g->m.i_rem( loc, item_pos );
        } else {
            i_rem( item_pos );
        }
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
    std::vector<item> components = dis_item.components;
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

            for( ; compcount > 0; compcount-- ) {
                components.emplace_back( newit );
            }
        }
    }

    std::list<item> drop_items;

    for( const item &newit : components ) {
        const bool comp_success = ( dice( skill_dice, skill_sides ) > dice( diff_dice,  diff_sides ) );
        if( dis.difficulty != 0 && !comp_success ) {
            add_msg( m_bad, _( "You fail to recover %s." ), newit.tname().c_str() );
            continue;
        }
        const bool dmg_success = component_success_chance > rng_float( 0, 1 );
        if( !dmg_success ) {
            // Show reason for failure (damaged item, tname contains the damage adjective)
            //~ %1s - material, %2$s - disassembled item
            add_msg( m_bad, _( "You fail to recover %1$s from the %2$s." ), newit.tname().c_str(),
                     dis_item.tname().c_str() );
            continue;
        }
        // Use item from components list, or (if not contained)
        // use newit, the default constructed.
        item act_item = newit;

        // Refitted clothing disassembles into refitted components (when applicable)
        if( dis_item.has_flag( "FIT" ) && act_item.has_flag( "VARSIZE" ) ) {
            act_item.item_tags.insert( "FIT" );
        }

        if( filthy ) {
            act_item.item_tags.insert( "FILTHY" );
        }

        for( item::t_item_vector::iterator a = dis_item.components.begin(); a != dis_item.components.end();
             ++a ) {
            if( a->type == newit.type ) {
                act_item = *a;
                dis_item.components.erase( a );
                break;
            }
        }

        if( act_item.made_of( LIQUID ) ) {
            g->handle_all_liquid( act_item, PICKUP_RANGE );
        } else {
            drop_items.push_back( act_item );
        }
    }

    put_into_vehicle_or_drop( *this, item_drop_reason::deliberate, drop_items );

    if( !dis.learn_by_disassembly.empty() && !knows_recipe( &dis ) ) {
        if( can_decomp_learn( dis ) ) {
            // @todo: make this depend on intelligence
            if( one_in( 4 ) ) {
                learn_recipe( &dis.ident().obj() );//@todo: change to forward an id or a reference
                add_msg( m_good, _( "You learned a recipe for %s from disassembling it!" ),
                         dis_item.tname().c_str() );
            } else {
                add_msg( m_info, _( "You might be able to learn a recipe for %s if you disassemble another." ),
                         dis_item.tname().c_str() );
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
    if( newit.made_of( LIQUID ) && &p == &g->u ) { // TODO: what about NPCs?
        g->handle_all_liquid( newit, PICKUP_RANGE );
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
    if( dis_item.is_tool() && dis_item.charges > 0 && dis_item.ammo_type() ) {
        item ammodrop( dis_item.ammo_type()->default_ammotype(), calendar::turn );
        ammodrop.charges = dis_item.charges;
        if( dis_item.ammo_type() == ammotype( "plutonium" ) ) {
            ammodrop.charges /= PLUTONIUM_CHARGES;
        }
        drop_or_handle( ammodrop, p );
        dis_item.charges = 0;
    }
}

std::vector<npc *> player::get_crafting_helpers() const
{
    return g->get_npcs_if( [this]( const npc & guy ) {
        return rl_dist( guy.pos(), pos() ) < PICKUP_RANGE && ( guy.is_friend() ||
                guy.mission == NPC_MISSION_GUARD_ALLY ) &&
               !guy.in_sleep_state() && g->m.clear_path( pos(), guy.pos(), PICKUP_RANGE, 1, 100 );
    } );
}
