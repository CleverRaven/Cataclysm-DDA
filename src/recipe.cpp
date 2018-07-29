#include "recipe.h"

#include "calendar.h"
#include "generic_factory.h"
#include "itype.h"
#include "item.h"
#include "string_formatter.h"
#include "output.h"
#include "skill.h"
#include "game_constants.h"

#include <algorithm>
#include <numeric>
#include <math.h>

struct oter_t;
using oter_str_id = string_id<oter_t>;

recipe::recipe() : skill_used( skill_id::NULL_ID() ) {}

int recipe::batch_time( int batch, float multiplier, size_t assistants ) const
{
    // 1.0f is full speed
    // 0.33f is 1/3 speed
    if( multiplier == 0.0f ) {
        // If an item isn't craftable in the dark, show the time to complete as if you could craft it
        multiplier = 1.0f;
    }

    const float local_time = float( time ) / multiplier;

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
    bool strict = src == "dda";

    abstract = jo.has_string( "abstract" );

    if( abstract ) {
        ident_ = recipe_id( jo.get_string( "abstract" ) );
    } else {
        result_ = jo.get_string( "result" );
        ident_ = recipe_id( result_ );
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

    // Never let the player have a debug or NPC recipe
    if( jo.has_bool( "never_learn" ) ) {
        assign( jo, "never_learn", never_learn );
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
            booksets.emplace( arr.get_string( 0 ), arr.size() > 1 ? arr.get_int( 1 ) : -1 );
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
            ident_ = recipe_id( ident_.str() + "_" + jo.get_string( "id_suffix" ) );
        }

        assign( jo, "category", category, strict );
        assign( jo, "subcategory", subcategory, strict );
        assign( jo, "description", description, strict );
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
        container = item::find_type( result_ )->default_container;
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
    if( !item::type_is_defined( result_ )  && category != "CC_BUILDING" ) {
        return "defines invalid result";
    }

    if( category == "CC_BUILDING" && !oter_str_id( result_.c_str() ).is_valid() ) {
        return "defines invalid result";
    }

    if( charges >= 0 && !item::count_by_charges( result_ ) ) {
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
    item newit( result_, calendar::turn, item::default_charges_tag{} );
    if( charges >= 0 ) {
        newit.charges = charges;
    }

    if( !newit.craft_has_charges() ) {
        newit.charges = 0;
    } else if( result_mult != 1 ) {
        // @todo: Make it work for charge-less items
        newit.charges *= result_mult;
    }

    if( newit.has_flag( "VARSIZE" ) ) {
        newit.item_tags.insert( "FIT" );
    }

    if( contained ) {
        newit = newit.in_container( container );
    }

    return newit;
}

std::vector<item> recipe::create_results( int batch ) const
{
    std::vector<item> items;

    const bool by_charges = item::count_by_charges( result_ );
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
    return !byproducts.empty();
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

std::string recipe::result_name() const
{
    return item::nname( result_ );
}
