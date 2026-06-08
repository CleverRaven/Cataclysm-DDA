#include <cstddef>
#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "type_id.h"

// Detect net-positive craft/uncraft exploits.
//
// Two cases:
// 1. With provenance (player-crafted items remember components):
//    disassembly of uniform groups (all alts same type) must not
//    yield more than the minimum craft input.
// 2. Without provenance (world-spawned items have empty components):
//    disassembly uses front() of each group, which must not exceed
//    the maximum the player could have used for that component.

// Sum min or max counts from uniform groups (all alts same type).
static std::map<itype_id, int> uniform_group_counts(
    const requirement_data::alter_item_comp_vector &groups, bool use_max )
{
    std::map<itype_id, int> result;

    for( const std::vector<item_comp> &alts : groups ) {
        if( alts.empty() ) {
            continue;
        }

        std::set<itype_id> types;
        for( const item_comp &comp : alts ) {
            types.insert( comp.type );
        }
        if( types.size() != 1 ) {
            continue;
        }

        const itype_id &comp_type = *types.begin();
        int count = alts[0].count;
        for( size_t i = 1; i < alts.size(); ++i ) {
            if( use_max ) {
                count = std::max( count, alts[i].count );
            } else {
                count = std::min( count, alts[i].count );
            }
        }
        result[comp_type] += count;
    }
    return result;
}

// Max craft input per component: best count per type in each group, summed.
static std::map<itype_id, int> max_possible_counts(
    const requirement_data::alter_item_comp_vector &groups )
{
    std::map<itype_id, int> result;

    for( const std::vector<item_comp> &alts : groups ) {
        std::map<itype_id, int> group_best;
        for( const item_comp &comp : alts ) {
            auto it = group_best.find( comp.type );
            if( it == group_best.end() ) {
                group_best[comp.type] = comp.count;
            } else {
                it->second = std::max( it->second, comp.count );
            }
        }
        for( const auto &[type, count] : group_best ) {
            result[type] += count;
        }
    }
    return result;
}

TEST_CASE( "no_net_positive_craft_uncraft_cycles", "[recipe]" )
{
    for( const auto &[id, r] : recipe_dict ) {
        if( !r.is_reversible() ) {
            continue;
        }

        const requirement_data::alter_item_comp_vector &craft_comps =
            r.simple_requirements().get_components();
        const requirement_data disas = r.disassembly_requirements();
        const requirement_data::alter_item_comp_vector &disas_comps =
            disas.get_components();

        std::map<itype_id, int> craft_min =
            uniform_group_counts( craft_comps, false );
        std::map<itype_id, int> disas_max =
            uniform_group_counts( disas_comps, true );

        // With-provenance: uniform groups only.
        for( const auto &[comp_id, yield] : disas_max ) {
            auto it = craft_min.find( comp_id );
            if( it == craft_min.end() ) {
                continue;
            }
            INFO( "recipe: " << id.str() );
            INFO( "component: " << comp_id.str() );
            CAPTURE( it->second );
            CAPTURE( yield );
            CHECK( yield <= it->second );
        }

        // No-provenance: front() of each disassembly group vs max craft input.
        std::map<itype_id, int> disas_front;
        for( const std::vector<item_comp> &alts : disas_comps ) {
            if( !alts.empty() ) {
                disas_front[alts.front().type] += alts.front().count;
            }
        }
        std::map<itype_id, int> craft_max = max_possible_counts( craft_comps );
        for( const auto &[comp_id, yield] : disas_front ) {
            auto it = craft_max.find( comp_id );
            if( it == craft_max.end() ) {
                continue;
            }
            INFO( "recipe: " << id.str() );
            INFO( "component: " << comp_id.str() );
            INFO( "(no-provenance front() path)" );
            CAPTURE( it->second );
            CAPTURE( yield );
            CHECK( yield <= it->second );
        }
    }
}
