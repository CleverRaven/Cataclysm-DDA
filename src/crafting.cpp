#include "crafting.h"

#include "catacharset.h"
#include "craft_command.h"
#include "debug.h"
#include "game.h"
#include "input.h"
#include "inventory.h"
#include "itype.h"
#include "json.h"
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
#include "crafting_gui.h"

#include <algorithm> //std::min
#include <iostream>
#include <math.h>    //sqrt
#include <queue>
#include <string>
#include <sstream>
#include <numeric>

const efftype_id effect_contacts( "contacts" );

static const std::string fake_recipe_book = "book";

void remove_from_component_lookup( recipe *r );

static bool crafting_allowed( const player &p, const recipe &rec )
{
    if( !p.has_morale_to_craft() ) {
        add_msg( m_info, _( "Your morale is too low to craft..." ) );
        return false;
    }

    if( p.lighting_craft_speed_multiplier( rec ) <= 0.0f ) {
        add_msg( m_info, _( "You can't see to craft!" ) );
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
    if( lastrecipe.empty() ) {
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

bool player::making_would_work( const std::string &id_to_make, int batch_size )
{
    const auto &making = recipe_dict[ id_to_make ];
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

    return making.check_eligible_containers_for_crafting( batch_size );
}

recipe::recipe() : skill_used( "none" ) {}

bool recipe::check_eligible_containers_for_crafting( int batch ) const
{
    std::vector<item> conts = g->u.get_eligible_containers_for_crafting();
    std::vector<item> res = create_results( batch );
    std::vector<item> bps = create_byproducts( batch );
    std::vector<item> all;
    all.reserve( res.size() + bps.size() );
    all.insert( all.end(), res.begin(), res.end() );
    all.insert( all.end(), bps.begin(), bps.end() );

    for( const item &prod : all ) {
        if( !prod.made_of( LIQUID ) ) {
            continue;
        }

        // we go trough half-filled containers first, then go through empty containers if we need
        std::sort( conts.begin(), conts.end(), item_compare_by_charges );

        long charges_to_store = prod.charges;
        for( const item &cont : conts ) {
            if( charges_to_store <= 0 ) {
                break;
            }

            if( !cont.is_container_empty() ) {
                if( cont.contents.front().typeId() == prod.typeId() ) {
                    charges_to_store -= cont.get_remaining_capacity_for_liquid( cont.contents.front(), true );
                }
            } else {
                charges_to_store -= cont.get_remaining_capacity_for_liquid( prod, true );
            }
        }

        // also check if we're currently in a vehicle that has the necessary storage
        if( charges_to_store > 0 ) {
            vehicle *veh = g->m.veh_at( g->u.pos() );
            if( veh != NULL ) {
                const itype_id &ftype = prod.typeId();
                int fuel_cap = veh->fuel_capacity( ftype );
                int fuel_amnt = veh->fuel_left( ftype );

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

std::vector<item> player::get_eligible_containers_for_crafting()
{
    std::vector<item> conts;

    if( is_container_eligible_for_crafting( weapon, true ) ) {
        conts.push_back( weapon );
    }
    for( item &i : worn ) {
        if( is_container_eligible_for_crafting( i, false ) ) {
            conts.push_back( i );
        }
    }
    for( size_t i = 0; i < inv.size(); i++ ) {
        for( item it : inv.const_stack( i ) ) {
            if( is_container_eligible_for_crafting( it, false ) ) {
                conts.push_back( it );
            }
        }
    }

    // get all potential containers within PICKUP_RANGE tiles including vehicles
    for( const auto &loc : closest_tripoints_first( PICKUP_RANGE, pos() ) ) {
        if( g->m.accessible_items( pos(), loc, PICKUP_RANGE ) ) {
            for( item &it : g->m.i_at( loc ) ) {
                if( is_container_eligible_for_crafting( it, true ) ) {
                    conts.emplace_back( it );
                }
            }
        }

        int part = -1;
        vehicle *veh = g->m.veh_at( loc, part );
        if( veh && part >= 0 ) {
            part = veh->part_with_feature( part, "CARGO" );
            if( part != -1 ) {
                for( item &it : veh->get_items( part ) ) {
                    if( is_container_eligible_for_crafting( it, false ) ) {
                        conts.emplace_back( it );
                    }
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
        && cached_turn == calendar::turn.get_turn()
        && cached_position == pos() ) {
        return cached_crafting_inventory;
    }
    cached_crafting_inventory.form_from_map( pos(), PICKUP_RANGE, false );
    cached_crafting_inventory += inv;
    cached_crafting_inventory += weapon;
    cached_crafting_inventory += worn;
    for( const auto &bio : my_bionics ) {
        const auto &bio_data = bio.info();
        if( ( !bio_data.activated || bio.powered ) &&
            !bio_data.fake_item.empty() ) {
            cached_crafting_inventory += item( bio.info().fake_item,
                                               calendar::turn, power_level );
        }
    }

    cached_moves = moves;
    cached_turn = calendar::turn.get_turn();
    cached_position = pos();
    return cached_crafting_inventory;
}

void player::invalidate_crafting_inventory()
{
    cached_turn = -1;
}

int recipe::batch_time( int batch ) const
{
    // 1.0f is full speed
    // 0.33f is 1/3 speed
    float lighting_speed = g->u.lighting_craft_speed_multiplier( *this );
    if( lighting_speed == 0.0f ) {
        return time * batch; // how did we even get here?
    }

    float local_time = float( time ) / lighting_speed;

    // NPCs around you should assist in batch production if they have the skills
    const auto helpers = g->u.get_crafting_helpers();
    int assistants = std::count_if( helpers.begin(), helpers.end(),
    [this]( const npc * np ) {
        return np->get_skill_level( skill_used ) >= difficulty;
    } );

    // if recipe does not benefit from batching and we have no assistants, don't do unnecessary additional calculations
    if( batch_rscale == 0.0 && assistants == 0 ) {
        return local_time * batch;
    }

    float total_time = 0.0;
    // if recipe does not benefit from batching but we do have assistants, skip calculating the batching scale factor
    if( batch_rscale == 0.0 ) {
        total_time = local_time * batch;
    } else {
        // recipe benefits from batching, so batching scale factor needs to be calculated
        // At batch_rsize, incremental time increase is 99.5% of batch_rscale
        double scale = batch_rsize / 6.0;
        for( double x = 0; x < batch; x++ ) {
            // scaled logistic function output
            double logf = ( 2.0 / ( 1.0 + exp( -( x / scale ) ) ) ) - 1.0;
            total_time += local_time * ( 1.0 - ( batch_rscale * logf ) );
        }
    }

    //Assistants can decrease the time for production but never less than that of one unit
    if( assistants == 1 ) {
        total_time = total_time * .75;
    } else if( assistants >= 2 ) {
        total_time = total_time * .60;
    }
    if( total_time < local_time ) {
        total_time = local_time;
    }

    return int( total_time );
}

bool recipe::has_flag( const std::string &flag_name ) const
{
    return flags.count( flag_name );
}

void player::make_craft( const std::string &id_to_make, int batch_size )
{
    make_craft_with_command( id_to_make, batch_size );
}

void player::make_all_craft( const std::string &id_to_make, int batch_size )
{
    make_craft_with_command( id_to_make, batch_size, true );
}

void player::make_craft_with_command( const std::string &id_to_make, int batch_size, bool is_long )
{
    const auto &recipe_to_make = recipe_dict[ id_to_make ];

    if( !recipe_to_make ) {
        return;
    }

    *last_craft = craft_command( &recipe_to_make, batch_size, is_long, this );
    last_craft->execute();
}

item recipe::create_result() const
{
    item newit( result, calendar::turn, item::default_charges_tag{} );
    if( charges >= 0 ) {
        newit.charges = charges;
    }

    if( !newit.craft_has_charges() ) {
        newit.charges = 0;
    } else if( result_mult != 1 ) {
        // @todo Make it work for charge-less items
        newit.charges *= result_mult;
    }

    if( newit.has_flag( "VARSIZE" ) ) {
        newit.item_tags.insert( "FIT" );
    }

    if( contained == true ) {
        newit = newit.in_container( container );
    }

    return newit;
}

std::vector<item> recipe::create_results( int batch ) const
{
    std::vector<item> items;

    const bool by_charges = item::count_by_charges( result );
    if( contained || !by_charges ) {
        // by_charges items get their charges multiplied in create_result
        const int num_results = by_charges ? batch : batch * result_mult;
        for( int i = 0; i < num_results; i++ ) {
            item newit = create_result();
            items.push_back( newit );
        }
    } else {
        item newit = create_result();
        newit.charges *= batch;
        items.push_back( newit );
    }

    return items;
}

std::vector<item> recipe::create_byproducts( int batch ) const
{
    std::vector<item> bps;
    for( const auto &e : byproducts ) {
        item obj( e.first, calendar::turn, item::default_charges_tag{} );
        if( obj.has_flag( "VARSIZE" ) ) {
            obj.item_tags.insert( "FIT" );
        }

        if( obj.count_by_charges() ) {
            obj.charges *= e.second;
            bps.push_back( obj );

        } else {
            if( !obj.craft_has_charges() ) {
                obj.charges = 0;
            }
            for( int i = 0; i < e.second * batch; ++i ) {
                bps.push_back( obj );
            }
        }
    }
    return bps;
}

bool recipe::has_byproducts() const
{
    return byproducts.size() != 0;
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

void player::complete_craft()
{
    const recipe *making = &recipe_dict[ activity.name ]; // Which recipe is it?
    int batch_size = activity.values.front();
    if( making == nullptr ) {
        debugmsg( "no recipe with id %s found", activity.name.c_str() );
        activity.type = ACT_NULL;
        return;
    }

    int secondary_dice = 0;
    int secondary_difficulty = 0;
    for( const auto &pr : making->required_skills ) {
        secondary_dice += get_skill_level( pr.first );
        secondary_difficulty += pr.second;
    }

    // # of dice is 75% primary skill, 25% secondary (unless secondary is null)
    int skill_dice;
    if( secondary_difficulty > 0 ) {
        skill_dice = get_skill_level( making->skill_used ) * 3 + secondary_dice;
    } else {
        skill_dice = get_skill_level( making->skill_used ) * 4;
    }

    auto helpers = g->u.get_crafting_helpers();
    for( const npc *np : helpers ) {
        if( np->get_skill_level( making->skill_used ) >=
            get_skill_level( making->skill_used ) ) {
            // NPC assistance is worth half a skill level
            skill_dice += 2;
            add_msg( m_info, _( "%s helps with crafting..." ), np->name.c_str() );
            break;
        }
    }

    // farsightedness can impose a penalty on electronics and tailoring success
    // it's equivalent to a 2-rank electronics penalty, 1-rank tailoring
    if( has_trait( "HYPEROPIC" ) && !is_wearing( "glasses_reading" ) &&
        !is_wearing( "glasses_bifocal" ) && !has_effect( effect_contacts ) ) {
        int main_rank_penalty = 0;
        if( making->skill_used == skill_id( "electronics" ) ) {
            main_rank_penalty = 2;
        } else if( making->skill_used == skill_id( "tailor" ) ) {
            main_rank_penalty = 1;
        }
        skill_dice -= main_rank_penalty * 4;
    }

    // It's tough to craft with paws.  Fortunately it's just a matter of grip and fine-motor,
    // not inability to see what you're doing
    if( has_trait( "PAWS" ) || has_trait( "PAWS_LARGE" ) ) {
        int paws_rank_penalty = 0;
        if( has_trait( "PAWS_LARGE" ) ) {
            paws_rank_penalty += 1;
        }
        if( making->skill_used == skill_id( "electronics" )
            || making->skill_used == skill_id( "tailor" )
            || making->skill_used == skill_id( "mechanics" ) ) {
            paws_rank_penalty += 1;
        }
        skill_dice -= paws_rank_penalty * 4;
    }

    // Sides on dice is 16 plus your current intelligence
    ///\EFFECT_INT increases crafting success chance
    int skill_sides = 16 + int_cur;

    int diff_dice;
    if( secondary_difficulty > 0 ) {
        diff_dice = making->difficulty * 3 + secondary_difficulty;
    } else {
        // Since skill level is * 4 also
        diff_dice = making->difficulty * 4;
    }

    int diff_sides = 24; // 16 + 8 (default intelligence)

    int skill_roll = dice( skill_dice, skill_sides );
    int diff_roll  = dice( diff_dice,  diff_sides );

    if( making->skill_used ) {
        //normalize experience gain to crafting time, giving a bonus for longer crafting
        practice( making->skill_used, ( int )( ( making->difficulty * 15 + 10 ) * ( 1 + making->batch_time(
                batch_size ) / 30000.0 ) ),
                  ( int )making->difficulty * 1.25 );

        //NPCs assisting or watching should gain experience...
        for( auto &elem : helpers ) {
            //If the NPC can understand what you are doing, they gain more exp
            if( elem->get_skill_level( making->skill_used ) >= making->difficulty ) {
                elem->practice( making->skill_used,
                                ( int )( ( making->difficulty * 15 + 10 ) * ( 1 + making->batch_time( batch_size ) / 30000.0 ) *
                                         .50 ), ( int )making->difficulty * 1.25 );
                if( batch_size > 1 ) {
                    add_msg( m_info, _( "%s assists with crafting..." ), elem->name.c_str() );
                }
                if( batch_size == 1 ) {
                    add_msg( m_info, _( "%s could assist you with a batch..." ), elem->name.c_str() );
                }
                //NPCs around you understand the skill used better
            } else {
                elem->practice( making->skill_used,
                                ( int )( ( making->difficulty * 15 + 10 ) * ( 1 + making->batch_time( batch_size ) / 30000.0 ) *
                                         .15 ), ( int )making->difficulty * 1.25 );
                add_msg( m_info, _( "%s watches you craft..." ), elem->name.c_str() );
            }
        }

    }

    // Messed up badly; waste some components.
    if( making->difficulty != 0 && diff_roll > skill_roll * ( 1 + 0.1 * rng( 1, 5 ) ) ) {
        add_msg( m_bad, _( "You fail to make the %s, and waste some materials." ),
                 item::nname( making->result ).c_str() );
        if( last_craft->has_cached_selections() ) {
            last_craft->consume_components();
        } else {
            // @todo Guarantee that selections are cached
            const auto &req = making->requirements();
            for( const auto &it : req.get_components() ) {
                consume_items( it, batch_size );
            }
            for( const auto &it : req.get_tools() ) {
                consume_tools( it, batch_size );
            }
        }
        activity.type = ACT_NULL;
        return;
        // Messed up slightly; no components wasted.
    } else if( diff_roll > skill_roll ) {
        add_msg( m_neutral, _( "You fail to make the %s, but don't waste any materials." ),
                 item::nname( making->result ).c_str() );
        //this method would only have been called from a place that nulls activity.type,
        //so it appears that it's safe to NOT null that variable here.
        //rationale: this allows certain contexts (e.g. ACT_LONGCRAFT) to distinguish major and minor failures
        return;
    }

    // If we're here, the craft was a success!
    // Use up the components and tools
    std::list<item> used;
    if( !last_craft->has_cached_selections() ) {
        // This should fail and return, but currently crafting_command isn't saved
        // Meaning there are still cases where has_cached_selections will be false
        // @todo Allow saving last_craft and debugmsg+fail craft if selection isn't cached
        if( !has_trait( "DEBUG_HS" ) ) {
            const auto &req = making->requirements();
            for( const auto &it : req.get_components() ) {
                std::list<item> tmp = consume_items( it, batch_size );
                used.splice( used.end(), tmp );
            }
            for( const auto &it : req.get_tools() ) {
                consume_tools( it, batch_size );
            }
        }
    } else if( !has_trait( "DEBUG_HS" ) ) {
        used = last_craft->consume_components();
        if( used.empty() ) {
            return;
        }
    }

    // Set up the new item, and assign an inventory letter if available
    std::vector<item> newits = making->create_results( batch_size );
    bool first = true;
    float used_age_tally = 0;
    int used_age_count = 0;
    size_t newit_counter = 0;
    for( item &newit : newits ) {
        // messages, learning of recipe, food spoilage calc only once
        if( first ) {
            first = false;
            if( knows_recipe( making ) ) {
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
                int difficulty = has_recipe( making, crafting_inventory(), helpers );
                ///\EFFECT_INT increases chance to learn recipe when crafting from a book
                if( x_in_y( making->time, ( 1000 * 8 *
                                            ( difficulty * difficulty * difficulty * difficulty ) ) /
                            ( std::max( get_skill_level( making->skill_used ).level(), 1 ) * std::max( get_int(), 1 ) ) ) ) {
                    learn_recipe( ( recipe * )making );
                    add_msg( m_good, _( "You memorized the recipe for %s!" ),
                             newit.type_name( 1 ).c_str() );
                }
            }

            for( auto &elem : used ) {
                if( elem.goes_bad() ) {
                    used_age_tally += elem.get_relative_rot();
                    ++used_age_count;
                }
            }
        }

        // Don't store components for things made by charges,
        // don't store components for things that can't be uncrafted.
        if( recipe_dictionary::get_uncraft( making->result ) && !newit.count_by_charges() ) {
            // Setting this for items counted by charges gives only problems:
            // those items are automatically merged everywhere (map/vehicle/inventory),
            // which would either loose this information or merge it somehow.
            set_components( newit.components, used, batch_size, newit_counter );
            newit_counter++;
        }
        finalize_crafted_item( newit, used_age_tally, used_age_count );
        set_item_inventory( newit );
    }

    if( making->has_byproducts() ) {
        std::vector<item> bps = making->create_byproducts( batch_size );
        for( auto &bp : bps ) {
            finalize_crafted_item( bp, used_age_tally, used_age_count );
            set_item_inventory( bp );
        }
    }

    inv.restack( this );
}

void set_item_spoilage( item &newit, float used_age_tally, int used_age_count )
{
    newit.set_relative_rot( used_age_tally / used_age_count );
}

void set_item_food( item &newit )
{
    int bday_tmp = newit.bday % 3600; // fuzzy birthday for stacking reasons
    newit.bday = int( newit.bday ) + 3600 - bday_tmp;
    if( newit.has_flag( "EATEN_HOT" ) ) { // hot foods generated
        newit.item_tags.insert( "HOT" );
        newit.item_counter = 600;
        newit.active = true;
    }
}

void finalize_crafted_item( item &newit, float used_age_tally, int used_age_count )
{
    if( newit.is_food() ) {
        set_item_food( newit );
    }
    if( used_age_count > 0 && newit.goes_bad() ) {
        set_item_spoilage( newit, used_age_tally, used_age_count );
    }
}

void set_item_inventory( item &newit )
{
    if( newit.made_of( LIQUID ) ) {
        g->handle_all_liquid( newit, PICKUP_RANGE );
    } else {
        g->u.inv.assign_empty_invlet( newit );
        // We might not have space for the item
        if( !g->u.can_pickVolume( newit ) ) { //Accounts for result_mult
            add_msg( _( "There's no room in your inventory for the %s, so you drop it." ),
                     newit.tname().c_str() );
            g->m.add_item_or_charges( g->u.pos(), newit );
        } else if( !g->u.can_pickWeight( newit, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) ) {
            add_msg( _( "The %s is too heavy to carry, so you drop it." ),
                     newit.tname().c_str() );
            g->m.add_item_or_charges( g->u.pos(), newit );
        } else {
            newit = g->u.i_add( newit );
            add_msg( m_info, "%c - %s", newit.invlet == 0 ? ' ' : newit.invlet, newit.tname().c_str() );
        }
    }
}

/* selection of component if a recipe requirement has multiple options (e.g. 'duct tap' or 'welder') */
comp_selection<item_comp> player::select_item_component( const std::vector<item_comp> &components,
        int batch, inventory &map_inv, bool can_cancel )
{
    std::vector<item_comp> player_has;
    std::vector<item_comp> map_has;
    std::vector<item_comp> mixed;

    comp_selection<item_comp> selected;

    for( const auto &component : components ) {
        itype_id type = component.type;
        int count = ( component.count > 0 ) ? component.count * batch : abs( component.count );
        bool pl = false, mp = false;

        if( item::count_by_charges( type ) && count > 0 ) {
            if( has_charges( type, count ) ) {
                player_has.push_back( component );
                pl = true;
            }
            if( map_inv.has_charges( type, count ) ) {
                map_has.push_back( component );
                mp = true;
            }
            if( !pl && !mp && charges_of( type ) + map_inv.charges_of( type ) >= count ) {
                mixed.push_back( component );
            }
        } else { // Counting by units, not charges

            if( has_amount( type, count ) ) {
                player_has.push_back( component );
                pl = true;
            }
            if( map_inv.has_components( type, count ) ) {
                map_has.push_back( component );
                mp = true;
            }
            if( !pl && !mp && amount_of( type ) + map_inv.amount_of( type ) >= count ) {
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
        uimenu cmenu;
        // Populate options with the names of the items
        for( auto &map_ha : map_has ) {
            std::string tmpStr = item::nname( map_ha.type ) + _( " (nearby)" );
            cmenu.addentry( tmpStr );
        }
        for( auto &player_ha : player_has ) {
            cmenu.addentry( item::nname( player_ha.type ) );
        }
        for( auto &elem : mixed ) {
            std::string tmpStr = item::nname( elem.type ) + _( " (on person & nearby)" );
            cmenu.addentry( tmpStr );
        }

        // Unlike with tools, it's a bad thing if there aren't any components available
        if( cmenu.entries.empty() ) {
            if( has_trait( "DEBUG_HS" ) ) {
                selected.use_from = use_from_player;
                return selected;
            }

            debugmsg( "Attempted a recipe with no available components!" );
            selected.use_from = cancel;
            return selected;
        }

        if( can_cancel ) {
            cmenu.addentry( -1, true, 'q', _( "Cancel" ) );
        }

        // Get the selection via a menu popup
        cmenu.title = _( "Use which component?" );
        cmenu.query();

        if( cmenu.ret == static_cast<int>( map_has.size() + player_has.size() + mixed.size() ) ) {
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

std::list<item> player::consume_items( const comp_selection<item_comp> &is, int batch )
{
    std::list<item> ret;

    if( has_trait( "DEBUG_HS" ) ) {
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
            std::list<item> tmp = g->m.use_charges( loc, PICKUP_RANGE, selected_comp.type, real_count );
            ret.splice( ret.end(), tmp );
        } else {
            std::list<item> tmp = g->m.use_amount( loc, PICKUP_RANGE, selected_comp.type,
                                                   real_count );
            remove_ammo( tmp, *this );
            ret.splice( ret.end(), tmp );
        }
    }
    if( is.use_from & use_from_player ) {
        if( by_charges ) {
            std::list<item> tmp = use_charges( selected_comp.type, real_count );
            ret.splice( ret.end(), tmp );
        } else {
            std::list<item> tmp = use_amount( selected_comp.type, real_count );
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
    return ret;
}

/* This call is in-efficient when doing it for multiple items with the same map inventory.
In that case, consider using select_item_component with 1 pre-created map inventory, and then passing the results
to consume_items */
std::list<item> player::consume_items( const std::vector<item_comp> &components, int batch )
{
    inventory map_inv;
    map_inv.form_from_map( pos(), PICKUP_RANGE );
    return consume_items( select_item_component( components, batch, map_inv ), batch );
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
        uimenu tmenu( hotkeys );
        for( auto &map_ha : map_has ) {
            std::string tmpStr = item::nname( map_ha.type ) + _( " (nearby)" );
            tmenu.addentry( tmpStr );
        }
        for( auto &player_ha : player_has ) {
            tmenu.addentry( item::nname( player_ha.type ) );
        }

        if( tmenu.entries.empty() ) {  // This SHOULD only happen if cooking with a fire,
            selected.use_from = use_from_none;
            return selected;    // and the fire goes out.
        }

        if( can_cancel ) {
            tmenu.addentry( -1, true, 'q', _( "Cancel" ) );
        }

        // Get selection via a popup menu
        tmenu.title = _( "Use which tool?" );
        tmenu.query();

        if( tmenu.ret == static_cast<int>( map_has.size() + player_has.size() ) ) {
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
    if( has_trait( "DEBUG_HS" ) ) {
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

bool player::can_disassemble( const item &obj, const inventory &inv, bool alert ) const
{
    const auto &r = recipe_dictionary::get_uncraft( obj.typeId() );
    if( !r ) {
        if( alert ) {
            add_msg_if_player( m_info, _( "You cannot disassemble the %s" ), obj.tname().c_str() );
        }
        return false;
    }

    if( obj.count_by_charges() && !r.has_flag( "UNCRAFT_SINGLE_CHARGE" ) ) {
        // Create a new item to get the default charges
        int qty = r.create_result().charges;
        if( obj.charges < qty ) {
            if( alert ) {
                auto msg = ngettext( "You need at least %d charge of %s to disassemble it.",
                                     "You need at least %d charges of %s to disassemble it.", qty );
                add_msg_if_player( m_info, msg, qty, obj.tname().c_str() );
            }
            return false;
        }
    }

    const auto &dis = r.disassembly_requirements();

    for( const auto &opts : dis.get_qualities() ) {
        for( const auto &qual : opts ) {
            if( !qual.has( inv ) ) {
                if( alert ) {
                    add_msg_if_player( m_info, _( "To disassemble %s, you need %s" ),
                                       obj.tname().c_str(), qual.to_string().c_str() );
                }
                return false;
            }
        }
    }

    for( const auto &opts : dis.get_tools() ) {
        if( !std::any_of( opts.begin(), opts.end(), [&]( const tool_comp & tool ) {
        return ( tool.count <= 0 && inv.has_tools( tool.type, 1 ) ) ||
                   ( tool.count >  0 && inv.has_charges( tool.type, tool.count ) );
        } ) ) {
            if( alert ) {
                if( opts[0].count <= 0 ) {
                    add_msg( m_info, _( "You need a %s to disassemble %s." ),
                             item::nname( opts[0].type ).c_str(), obj.tname().c_str() );
                } else {
                    add_msg( m_info, ngettext( "You need a %s with %d charge to disassemble %s.",
                                               "You need a %s with %d charges to disassemble %s.",
                                               opts[0].count ),
                             item::nname( opts[0].type ).c_str(), opts[0].count, obj.tname().c_str() );
                }
            }
            return false;
        }
    }

    return true;
}

bool player::disassemble( int dis_pos )
{
    if( dis_pos == INT_MAX ) {
        dis_pos = g->inv_for_all( _( "Disassemble item:" ),
                                  _( "You don't have any items to disassemble." ) );
    }
    if( dis_pos == INT_MIN ) {
        add_msg_if_player( m_info, _( "Never mind." ) );
        return false;
    }
    return disassemble( i_at( dis_pos ), dis_pos, false );
}

bool player::disassemble( item &obj, int pos, bool ground, bool interactive )
{
    const auto &r = recipe_dictionary::get_uncraft( obj.typeId() );

    // check sufficient light
    if( lighting_craft_speed_multiplier( r ) == 0.0f ) {
        if( interactive ) {
            add_msg_if_player( m_info, _( "You can't see to craft!" ) );
        }
        return false;
    }

    // refuse to disassemble rotten items
    if( obj.goes_bad() || ( obj.is_food_container() && obj.contents.front().goes_bad() ) ) {
        obj.calc_rot( global_square_location() );
        if( obj.rotten() || ( obj.is_food_container() && obj.contents.front().rotten() ) ) {
            if( interactive ) {
                add_msg_if_player( m_info, _( "It's rotten, I'm not taking that apart." ) );
            }
            return false;
        }
    }

    // check sufficient tools for disassembly
    const inventory &inv = crafting_inventory();
    if( !can_disassemble( obj, inv, interactive ) ) {
        return false;
    }

    // last chance to back out
    if( interactive &&
        get_option<bool>( "QUERY_DISASSEMBLE" ) &&
        !query_yn( _( "Disassembling the %s will take about %s. Continue?" ),
                   obj.tname().c_str(), calendar::print_duration( r.time / 100 ).c_str() ) ) {
        return false;
    }

    if( activity.type != ACT_DISASSEMBLE ) {
        assign_activity( ACT_DISASSEMBLE, r.time );
    } else if( activity.moves_left <= 0 ) {
        activity.moves_left = r.time;
    }

    activity.values.push_back( pos );
    activity.coords.push_back( ground ? this->pos() : tripoint_min );
    activity.str_values.push_back( r.result );

    return true;
}

void player::disassemble_all( bool one_pass )
{
    // Reset all the activity values
    assign_activity( ACT_DISASSEMBLE, 0 );
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

// Find out which of the alternative components had been used to craft the item.
item_comp find_component( const std::vector<item_comp> &altercomps, const item &dis_item )
{
    for( auto &comp : altercomps ) {
        for( auto &elem : dis_item.components ) {
            if( elem.typeId() == comp.type ) {
                return comp;
            }
        }
    }
    // Default is the one listed first in json.
    return altercomps.front();
}

item &get_item_for_uncraft( player &p, int item_pos,
                            const tripoint &loc, bool from_ground )
{
    item *org_item;
    if( from_ground ) {
        auto items_on_ground = g->m.i_at( loc );
        if( static_cast<size_t>( item_pos ) >= items_on_ground.size() ) {
            return p.ret_null;
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
        activity.type = ACT_NULL;
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

    if( activity.type == ACT_NULL ) {
        // Something above went wrong, don't continue
        return;
    }

    // Try to get another disassembly target from the activity
    if( activity.values.empty() ) {
        // No more targets
        activity.type = ACT_NULL;
        return;
    }

    if( activity.values.back() == INT_MIN ) {
        disassemble_all( false );
        return;
    }

    const auto &next_recipe = recipe_dictionary::get_uncraft( activity.str_values.back() );
    if( !next_recipe ) {
        activity.type = ACT_NULL;
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
    if( org_item.is_null() ) {
        add_msg( _( "The item has vanished." ) );
        activity.type = ACT_NULL;
        return;
    }

    if( org_item.typeId() != dis.result ) {
        add_msg( _( "The item might be gone, at least it is not at the expected position anymore." ) );
        activity.type = ACT_NULL;
        return;
    }
    // Make a copy to keep its data (damage/components) even after it
    // has been removed.
    item dis_item = org_item;

    float component_success_chance = std::min( std::pow( 0.8, dis_item.damage() ), 1.0 );

    int veh_part = -1;
    vehicle *veh = g->m.veh_at( pos(), veh_part );
    if( veh != nullptr ) {
        veh_part = veh->part_with_feature( veh_part, "CARGO" );
    }

    add_msg( _( "You disassemble the %s into its components." ), dis_item.tname().c_str() );
    // Remove any batteries, ammo and mods first
    remove_ammo( &dis_item, *this );
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

    for( const auto &altercomps : dis_requirements.get_components() ) {
        const item_comp comp = find_component( altercomps, dis_item );
        int compcount = comp.count;
        item newit( comp.type, calendar::turn );
        // Counted-by-charge items that can be disassembled individually
        // have their component count multiplied by the number of charges.
        if( dis_item.count_by_charges() && dis.has_flag( "UNCRAFT_SINGLE_CHARGE" ) ) {
            compcount *= std::min( dis_item.charges, dis.create_result().charges );
        }
        // Compress liquids and counted-by-charges items into one item,
        // they are added together on the map anyway and handle_liquid
        // should only be called once to put it all into a container at once.
        if( newit.count_by_charges() || newit.made_of( LIQUID ) ) {
            newit.charges = compcount;
            compcount = 1;
        } else if( !newit.craft_has_charges() && newit.charges > 0 ) {
            // tools that can be unloaded should be created unloaded,
            // tools that can't be unloaded will keep their default charges.
            newit.charges = 0;
        }

        for( ; compcount > 0; compcount-- ) {
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
            } else if( veh != NULL && veh->add_item( veh_part, act_item ) ) {
                // add_item did put the items in the vehicle, nothing further to be done
            } else {
                g->m.add_item_or_charges( pos(), act_item );
            }
        }
    }

    if( !dis.learn_by_disassembly.empty() && !knows_recipe( &dis ) ) {
        if( can_decomp_learn( dis ) ) {
            // @todo: make this depend on intelligence
            if( one_in( 4 ) ) {
                learn_recipe( &dis );
                add_msg( m_good, _( "You learned a recipe from disassembling it!" ) );
            } else {
                add_msg( m_info, _( "You might be able to learn a recipe if you disassemble another." ) );
            }
        } else {
            add_msg( m_info, _( "If you had better skills, you might learn a recipe next time." ) );
        }
    }
}

void remove_ammo( std::list<item> &dis_items, player &p )
{
    for( auto &dis_item : dis_items ) {
        remove_ammo( &dis_item, p );
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

void remove_ammo( item *dis_item, player &p )
{
    for( auto iter = dis_item->contents.begin(); iter != dis_item->contents.end(); ) {
        if( iter->has_flag( "IRREMOVABLE" ) ) {
            iter++;
            continue;
        }
        drop_or_handle( *iter, p );
        iter = dis_item->contents.erase( iter );
    }

    if( dis_item->has_flag( "NO_UNLOAD" ) ) {
        return;
    }
    if( dis_item->is_gun() && dis_item->ammo_current() != "null" ) {
        item ammodrop( dis_item->ammo_current(), calendar::turn );
        ammodrop.charges = dis_item->charges;
        drop_or_handle( ammodrop, p );
        dis_item->charges = 0;
    }
    if( dis_item->is_tool() && dis_item->charges > 0 && dis_item->ammo_type() ) {
        item ammodrop( default_ammo( dis_item->ammo_type() ), calendar::turn );
        ammodrop.charges = dis_item->charges;
        if( dis_item->ammo_type() == ammotype( "plutonium" ) ) {
            ammodrop.charges /= PLUTONIUM_CHARGES;
        }
        drop_or_handle( ammodrop, p );
        dis_item->charges = 0;
    }
}

std::string recipe::required_skills_string() const
{
    if( required_skills.empty() ) {
        return _( "N/A" );
    }
    return enumerate_as_string( required_skills.begin(), required_skills.end(),
    []( const std::pair<skill_id, int> &skill ) {
        return string_format( "%s (%d)", skill.first.obj().name().c_str(), skill.second );
    } );
}

const std::string &recipe::ident() const
{
    return ident_;
}

std::vector<npc *> player::get_crafting_helpers() const
{
    std::vector<npc *> ret;
    for( auto &elem : g->active_npc ) {
        if( rl_dist( elem->pos(), pos() ) < PICKUP_RANGE && elem->is_friend() &&
            !elem->in_sleep_state() && g->m.clear_path( pos(), elem->pos(), PICKUP_RANGE, 1, 100 ) ) {
            ret.push_back( elem );
        }
    }

    return ret;
}
