#include "recipe.h"

#include "game.h"
#include "itype.h"
#include "map.h"
#include "npc.h"
#include "player.h"
#include "vehicle.h"

#include <algorithm>

recipe::recipe() : skill_used( "none" ) {}

const std::string &recipe::ident() const
{
    return ident_;
}

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

void recipe::finalize()
{
    // concatenate both external and inline requirements
    add_requirements( reqs_external );
    add_requirements( reqs_internal );

    reqs_external.clear();
    reqs_internal.clear();

    if( contained && container == "null" ) {
        container = item::find_type( result )->default_container;
    }

    if( autolearn && autolearn_requirements.empty() ) {
        autolearn_requirements = required_skills;
        if( skill_used ) {
            autolearn_requirements[ skill_used ] = difficulty;
        }
    }
}

void recipe::add_requirements( const std::vector<std::pair<requirement_id, int>> &reqs )
{
    requirements_ = std::accumulate( reqs.begin(), reqs.end(), requirements_,
    []( const requirement_data & lhs, const std::pair<requirement_id, int> &rhs ) {
        return lhs + ( *rhs.first * rhs.second );
    } );
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
            obj.charges *= e.second * batch;
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

int recipe::print_items( WINDOW *w, int ypos, int xpos, nc_color col, int batch ) const
{
    if( !has_byproducts() ) {
        return 0;
    }

    const int oldy = ypos;

    mvwprintz( w, ypos++, xpos, col, _( "Byproducts:" ) );
    for( const auto &bp : byproducts ) {
        mvwprintz( w, ypos++, xpos, col, _( "> %d %s" ), bp.second * batch,
                   item::nname( bp.first ).c_str() );
    }

    return ypos - oldy;
}

int recipe::print_time( WINDOW *w, int ypos, int xpos, int width,
                        nc_color col, int batch ) const
{
    const int turns = batch_time( batch ) / 100;
    std::string text;
    if( turns < MINUTES( 1 ) ) {
        const int seconds = std::max( 1, turns * 6 );
        text = string_format( ngettext( "%d second", "%d seconds", seconds ), seconds );
    } else {
        const int minutes = ( turns % HOURS( 1 ) ) / MINUTES( 1 );
        const int hours = turns / HOURS( 1 );
        if( hours == 0 ) {
            text = string_format( ngettext( "%d minute", "%d minutes", minutes ), minutes );
        } else if( minutes == 0 ) {
            text = string_format( ngettext( "%d hour", "%d hours", hours ), hours );
        } else {
            const std::string h = string_format( ngettext( "%d hour", "%d hours", hours ), hours );
            const std::string m = string_format( ngettext( "%d minute", "%d minutes", minutes ), minutes );
            //~ A time duration: first is hours, second is minutes, e.g. "4 hours" "6 minutes"
            text = string_format( _( "%1$s and %2$s" ), h.c_str(), m.c_str() );
        }
    }
    text = string_format( _( "Time to complete: %s" ), text.c_str() );
    return fold_and_print( w, ypos, xpos, width, col, text );
}
