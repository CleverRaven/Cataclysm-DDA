#include "recipe.h"

#include "game.h"
#include "generic_factory.h"
#include "itype.h"
#include "map.h"
#include "npc.h"
#include "player.h"
#include "vehicle.h"

#include <algorithm>

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

void recipe::load( JsonObject &jo, const std::string &src )
{
    bool strict = src == "core";

    abstract = jo.has_string( "abstract" );

    if( abstract ) {
        ident_ = jo.get_string( "abstract" );
    } else {
        ident_ = result = jo.get_string( "result" );
    }

    assign( jo, "time", time, strict, 0 );
    assign( jo, "difficulty", difficulty, strict, 0, MAX_SKILL );
    assign( jo, "flags", flags );

    // automatically set contained if we specify as container
    assign( jo, "contained", contained, strict );
    contained |= assign( jo, "container", container, strict );

    if( jo.has_array( "batch_time_factors" ) ) {
        auto batch = jo.get_array( "batch_time_factors" );
        batch_rscale = batch.get_int( 0 ) / 100.0;
        batch_rsize  = batch.get_int( 1 );
    }

    assign( jo, "charges", charges );
    assign( jo, "result_mult", result_mult );

    assign( jo, "skill_used", skill_used, strict );

    if( jo.has_member( "skills_required" ) ) {
        auto sk = jo.get_array( "skills_required" );
        required_skills.clear();

        if( sk.empty() ) {
            // clear all requirements

        } else if( sk.has_array( 0 ) ) {
            // multiple requirements
            while( sk.has_more() ) {
                auto arr = sk.next_array();
                required_skills[skill_id( arr.get_string( 0 ) )] = arr.get_int( 1 );
            }

        } else {
            // single requirement
            required_skills[skill_id( sk.get_string( 0 ) )] = sk.get_int( 1 );
        }
    }

    // simplified autolearn sets requirements equal to required skills at finalization
    if( jo.has_bool( "autolearn" ) ) {
        assign( jo, "autolearn", autolearn );

    } else if( jo.has_array( "autolearn" ) ) {
        autolearn = true;
        auto sk = jo.get_array( "autolearn" );
        while( sk.has_more() ) {
            auto arr = sk.next_array();
            autolearn_requirements[skill_id( arr.get_string( 0 ) )] = arr.get_int( 1 );
        }
    }

    if( jo.has_member( "decomp_learn" ) ) {
        learn_by_disassembly.clear();

        if( jo.has_int( "decomp_learn" ) ) {
            if( !skill_used ) {
                jo.throw_error( "decomp_learn specified with no skill_used" );
            }
            assign( jo, "decomp_learn", learn_by_disassembly[skill_used] );

        } else if( jo.has_array( "decomp_learn" ) ) {
            auto sk = jo.get_array( "decomp_learn" );
            while( sk.has_more() ) {
                auto arr = sk.next_array();
                learn_by_disassembly[skill_id( arr.get_string( 0 ) )] = arr.get_int( 1 );
            }
        }
    }

    if( jo.has_member( "book_learn" ) ) {
        auto bk = jo.get_array( "book_learn" );
        booksets.clear();

        while( bk.has_more() ) {
            auto arr = bk.next_array();
            booksets.emplace( arr.get_string( 0 ), arr.get_int( 1 ) );
        }
    }

    // recipes not specifying any external requirements inherit from their parent recipe (if any)
    if( jo.has_string( "using" ) ) {
        reqs_external = { { requirement_id( jo.get_string( "using" ) ), 1 } };

    } else if( jo.has_array( "using" ) ) {
        auto arr = jo.get_array( "using" );
        reqs_external.clear();

        while( arr.has_more() ) {
            auto cur = arr.next_array();
            reqs_external.emplace_back( requirement_id( cur.get_string( 0 ) ), cur.get_int( 1 ) );
        }
    }

    const std::string type = jo.get_string( "type" );

    if( type == "recipe" ) {
        if( jo.has_string( "id_suffix" ) ) {
            if( abstract ) {
                jo.throw_error( "abstract recipe cannot specify id_suffix", "id_suffix" );
            }
            ident_ += "_" + jo.get_string( "id_suffix" );
        }

        assign( jo, "category", category, strict );
        assign( jo, "subcategory", subcategory, strict );
        assign( jo, "reversible", reversible, strict );

        if( jo.has_member( "byproducts" ) ) {
            auto bp = jo.get_array( "byproducts" );
            byproducts.clear();
            while( bp.has_more() ) {
                auto arr = bp.next_array();
                byproducts[ arr.get_string( 0 ) ] += arr.size() == 2 ? arr.get_int( 1 ) : 1;
            }
        }
    } else if( type == "uncraft" ) {
        reversible = true;
    } else {
        jo.throw_error( "unknown recipe type", "type" );
    }

    // inline requirements are always replaced (cannot be inherited)
    const auto req_id = string_format( "inline_%s_%s", type.c_str(), ident_.c_str() );
    requirement_data::load_requirement( jo, req_id );
    reqs_internal = { { requirement_id( req_id ), 1 } };
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

std::string recipe::get_consistency_error() const
{
    if( !item::type_is_defined( result ) ) {
        return "defines invalid result";
    }

    if( charges >= 0 && !item::count_by_charges( result ) ) {
        return "specifies charges but result is not counted by charges";
    }

    const auto is_invalid_bp = []( const std::pair<itype_id, int> &elem ) {
        return !item::type_is_defined( elem.first );
    };

    if( std::any_of( byproducts.begin(), byproducts.end(), is_invalid_bp ) ) {
        return "defines invalid byproducts";
    }

    if( !contained && container != "null" ) {
        return "defines container but not contained";
    }

    if( !item::type_is_defined( container ) ) {
        return "specifies unknown container";
    }

    const auto is_invalid_skill = []( const std::pair<skill_id, int> &elem ) {
        return !elem.first.is_valid();
    };

    if( ( skill_used && !skill_used.is_valid() ) ||
        std::any_of( required_skills.begin(), required_skills.end(), is_invalid_skill ) ) {
        return "uses invalid skill";
    }

    const auto is_invalid_book = []( const std::pair<itype_id, int> &elem ) {
        return !item::find_type( elem.first )->book;
    };

    if( std::any_of( booksets.begin(), booksets.end(), is_invalid_book ) ) {
        return "defines invalid book";
    }

    return std::string();
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
