#include "omdata.h" // IWYU pragma: associated

#include <algorithm>
#include <functional>
#include <list>
#include <optional>
#include <unordered_set>
#include <vector>

#include "all_enum_values.h"
#include "basecamp.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "cata_views.h"
#include "city.h"
#include "coordinates.h"
#include "debug.h"
#include "distribution.h"
#include "enum_conversions.h"
#include "generic_factory.h"
#include "map.h"
#include "mapgen_functions.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "rng.h"
#include "string_formatter.h"
#include "weighted_list.h"

template<>
struct enum_traits<join_type> {
    static constexpr join_type last = join_type::last;
};

namespace io
{

template<>
std::string enum_to_string<join_type>( join_type data )
{
    switch( data ) {
        // *INDENT-OFF*
        case join_type::mandatory: return "mandatory";
        case join_type::available: return "available";
        // *INDENT-ON*
        case join_type::last:
            break;
    }
    cata_fatal( "Invalid join_type" );
}

} // namespace io

void mutable_overmap_terrain_join::finalize( const std::string &context,
        const std::unordered_map<std::string, mutable_overmap_join *> &joins )
{
    auto join_it = joins.find( join_id );
    if( join_it != joins.end() ) {
        join = join_it->second;
    } else {
        debugmsg( "invalid join id %s in %s", join_id, context );
    }
    for( const std::string &alt_join_id : alternative_join_ids ) {
        auto alt_join_it = joins.find( alt_join_id );
        if( alt_join_it != joins.end() ) {
            alternative_joins.insert( alt_join_it->second );
        } else {
            debugmsg( "invalid join id %s in %s", alt_join_id, context );
        }
    }
}

std::optional<mutable_overmap_phase_remainder::can_place_result>
mutable_overmap_phase_remainder::can_place(
    const overmap &om, const mutable_overmap_placement_rule_remainder &rule,
    const tripoint_om_omt &origin, om_direction::type dir,
    const joins_tracker &unresolved
) const
{
    int context_mandatory_joins_shortfall = 0;

    for( const mutable_overmap_piece_candidate &piece : rule.pieces( origin, dir ) ) {
        if( !overmap::inbounds( piece.pos ) ) {
            return std::nullopt;
        }
        if( !overmap::is_amongst_locations( om.ter( piece.pos ), piece.overmap->locations ) ) {
            return std::nullopt;
        }
        if( unresolved.any_postponed_at( piece.pos ) ) {
            return std::nullopt;
        }
        context_mandatory_joins_shortfall -= unresolved.count_unresolved_at( piece.pos );
    }

    int num_my_non_available_matched = 0;

    std::vector<om_pos_dir> suppressed_joins;

    for( const std::pair<om_pos_dir, const mutable_overmap_terrain_join *> &p :
         rule.outward_joins( origin, dir ) ) {
        const om_pos_dir &pos_d = p.first;
        const mutable_overmap_terrain_join &ter_join = *p.second;
        const mutable_overmap_join &join = *ter_join.join;
        switch( unresolved.allows( pos_d, ter_join ) ) {
            case joins_tracker::join_status::disallowed:
                return std::nullopt;
            case joins_tracker::join_status::matched_non_available:
                ++context_mandatory_joins_shortfall;
                [[fallthrough]];
            case joins_tracker::join_status::matched_available:
                if( ter_join.type != join_type::available ) {
                    ++num_my_non_available_matched;
                }
                continue;
            case joins_tracker::join_status::mismatched_available:
                suppressed_joins.push_back( pos_d );
                break;
            case joins_tracker::join_status::free:
                break;
        }
        if( ter_join.type == join_type::available ) {
            continue;
        }
        // Verify that the remaining joins lead to
        // suitable locations
        tripoint_om_omt neighbour = pos_d.p + displace( pos_d.dir );
        if( !overmap::inbounds( neighbour ) ) {
            return std::nullopt;
        }
        const oter_id &neighbour_terrain = om.ter( neighbour );
        if( !overmap::is_amongst_locations( neighbour_terrain, join.into_locations ) ) {
            return std::nullopt;
        }
    }
    return mutable_overmap_phase_remainder::can_place_result{ context_mandatory_joins_shortfall,
            num_my_non_available_matched, suppressed_joins };
}

mutable_overmap_phase_remainder::satisfy_result mutable_overmap_phase_remainder::satisfy(
    const overmap &om, const tripoint_om_omt &pos,
    const joins_tracker &unresolved )
{
    weighted_int_list<satisfy_result> options;

    for( mutable_overmap_placement_rule_remainder &rule : rules ) {
        std::vector<satisfy_result> pos_dir_options;
        can_place_result best_result{ 0, 0, {} };

        for( om_direction::type dir : om_direction::all ) {
            for( const tripoint_rel_omt &piece_pos : rule.positions( dir ) ) {
                tripoint_om_omt origin = pos - piece_pos;

                if( std::optional<can_place_result> result = can_place(
                            om, rule, origin, dir, unresolved ) ) {
                    if( best_result < *result ) {
                        pos_dir_options.clear();
                        best_result = *result;
                    }
                    if( *result == best_result ) {
                        pos_dir_options.emplace_back( origin, dir, &rule, result.value().supressed_joins, std::string{} );
                    }
                }
            }
        }

        if( auto chosen_result = random_entry_opt( pos_dir_options ) ) {
            options.add( *chosen_result, rule.get_weight() );
        }
    }
    std::string joins_s = enumerate_as_string( unresolved.all_unresolved_at( pos ),
    []( const joins_tracker::join * j ) {
        return string_format( "%s: %s", io::enum_to_string( j->where.dir ), j->join->id );
    } );

    if( satisfy_result *picked = options.pick() ) {
        om_direction::type dir = picked->dir;
        const mutable_overmap_placement_rule_remainder &rule = *picked->rule;
        picked->description =
            string_format(
                // NOLINTNEXTLINE(cata-translate-string-literal)
                "At %s chose '%s' rot %d with neighbours N:%s E:%s S:%s W:%s and constraints "
                "%s",
                pos.to_string(), rule.description(), static_cast<int>( dir ),
                om.ter( pos + point::north ).id().str(), om.ter( pos + point::east ).id().str(),
                om.ter( pos + point::south ).id().str(), om.ter( pos + point::west ).id().str(),
                joins_s );
        picked->rule->decrement();
        return *picked;
    } else {
        std::string rules_s = enumerate_as_string( rules,
        []( const mutable_overmap_placement_rule_remainder & rule ) {
            if( rule.is_exhausted() ) {
                return string_format( "(%s)", rule.description() );
            } else {
                return rule.description();
            }
        } );
        std::string message =
            string_format(
                // NOLINTNEXTLINE(cata-translate-string-literal)
                "At %s FAILED to match on terrain %s with neighbours N:%s E:%s S:%s W:%s and "
                "constraints %s from amongst rules %s",
                pos.to_string(), om.ter( pos ).id().str(),
                om.ter( pos + point::north ).id().str(), om.ter( pos + point::east ).id().str(),
                om.ter( pos + point::south ).id().str(), om.ter( pos + point::west ).id().str(),
                joins_s, rules_s );
        return satisfy_result{ {}, om_direction::type::invalid, nullptr, std::vector<om_pos_dir>{}, std::move( message ) };
    }
}

void mutable_overmap_terrain_join::deserialize( const JsonValue &jin )
{
    if( jin.test_string() ) {
        jin.read( join_id, true );
    } else if( jin.test_object() ) {
        JsonObject jo = jin.get_object();
        jo.read( "id", join_id, true );
        jo.read( "type", type, true );
        jo.read( "alternatives", alternative_join_ids, true );
    } else {
        jin.throw_error( "Expected string or object" );
    }
}

using join_map = std::unordered_map<cube_direction, mutable_overmap_terrain_join>;

void mutable_overmap_terrain::finalize( const std::string &context,
                                        const std::unordered_map<std::string, mutable_overmap_join *> &special_joins,
                                        const cata::flat_set<string_id<overmap_location>> &default_locations )
{
    if( locations.empty() ) {
        locations = default_locations;
    }
    for( join_map::value_type &p : joins ) {
        mutable_overmap_terrain_join &ter_join = p.second;
        ter_join.finalize( context, special_joins );
    }
}

void mutable_overmap_terrain::check( const std::string &context ) const
{
    if( !terrain.is_valid() ) {
        debugmsg( "invalid overmap terrain id %s in %s", terrain.str(), context );
    }

    if( locations.empty() ) {
        debugmsg( "In %s, no locations are defined", context );
    }

    for( const string_id<overmap_location> &loc : locations ) {
        if( !loc.is_valid() ) {
            debugmsg( "invalid overmap location id %s in %s", loc.str(), context );
        }
    }

    for( const std::pair<const cube_direction, mutable_special_connection> &p :
         connections ) {
        p.second.check( string_format( "connection %s in %s", io::enum_to_string( p.first ),
                                       context ) );
    }
    if( camp_owner.has_value() ) {
        if( !camp_owner.value().is_valid() ) {
            debugmsg( "In %s, camp at %s has invalid owner %s", context, terrain.str(),
                      camp_owner.value().c_str() );
        }
        if( camp_name.empty() ) {
            debugmsg( "In %s, camp was defined but missing a camp_name.", context );
        }
    } else if( !camp_name.empty() ) {
        debugmsg( "In %s, camp_name defined but no owner.  Invalid name is discarded.", context );
    }
}

void mutable_overmap_terrain::deserialize( const JsonObject &jo )
{
    jo.read( "overmap", terrain, true );
    jo.read( "locations", locations );
    for( int i = 0; i != static_cast<int>( cube_direction::last ); ++i ) {
        cube_direction dir = static_cast<cube_direction>( i );
        std::string dir_s = io::enum_to_string( dir );
        if( jo.has_member( dir_s ) ) {
            jo.read( dir_s, joins[dir], true );
        }
    }
    jo.read( "connections", connections );
    jo.read( "camp", camp_owner );
    jo.read( "camp_name", camp_name );
}

std::string mutable_overmap_placement_rule::description() const
{
    if( !name.empty() ) {
        return name;
    }
    std::string first_om_id = pieces[0].overmap_id;
    if( pieces.size() == 1 ) {
        return first_om_id;
    } else {
        return "chunk using overmap " + first_om_id;
    }
}

void mutable_overmap_placement_rule::finalize( const std::string &context,
        const std::unordered_map<std::string, mutable_overmap_terrain> &special_overmaps
                                             )
{
    std::unordered_map<tripoint_rel_omt, const mutable_overmap_placement_rule_piece *>
    pieces_by_pos;
    for( mutable_overmap_placement_rule_piece &piece : pieces ) {
        bool inserted = pieces_by_pos.emplace( piece.pos, &piece ).second;
        if( !inserted ) {
            debugmsg( "phase of %s has chunk with duplicated position %s",
                      context, piece.pos.to_string() );
        }
        auto it = special_overmaps.find( piece.overmap_id );
        if( it == special_overmaps.end() ) {
            cata_fatal( "phase of %s specifies overmap %s which is not defined for that "
                        "special", context, piece.overmap_id );
        } else {
            piece.overmap = &it->second;
        }
    }
    for( const mutable_overmap_placement_rule_piece &piece : pieces ) {
        const mutable_overmap_terrain &ter = *piece.overmap;
        for( const join_map::value_type &p : ter.joins ) {
            const cube_direction dir = p.first;
            const mutable_overmap_terrain_join &ter_join = p.second;
            rel_pos_dir this_side{ piece.pos, dir + piece.rot };
            rel_pos_dir other_side = this_side.opposite();
            auto opposite_piece = pieces_by_pos.find( other_side.p );
            if( opposite_piece == pieces_by_pos.end() ) {
                outward_joins.emplace_back( this_side, &ter_join );
            } else {
                const std::string &opposite_join = ter_join.join->opposite_id;
                const mutable_overmap_placement_rule_piece &other_piece =
                    *opposite_piece->second;
                const mutable_overmap_terrain &other_om = *other_piece.overmap;

                auto opposite_om_join =
                    other_om.joins.find( other_side.dir - other_piece.rot );
                if( opposite_om_join == other_om.joins.end() ) {
                    debugmsg( "in phase of %s, %s has adjacent pieces %s at %s and %s at "
                              "%s where the former has a join %s pointed towards the latter, "
                              "but the latter has no join pointed towards the former",
                              context, description(), piece.overmap_id, piece.pos.to_string(),
                              other_piece.overmap_id, other_piece.pos.to_string(),
                              ter_join.join_id );
                } else if( opposite_om_join->second.join_id != opposite_join ) {
                    debugmsg( "in phase of %s, %s has adjacent pieces %s at %s and %s at "
                              "%s where the former has a join %s pointed towards the latter, "
                              "expecting a matching join %s whereas the latter has the join %s "
                              "pointed towards the former",
                              context, description(), piece.overmap_id, piece.pos.to_string(),
                              other_piece.overmap_id, other_piece.pos.to_string(),
                              ter_join.join_id, opposite_join,
                              opposite_om_join->second.join_id );
                }
            }
        }
    }
}
void mutable_overmap_placement_rule::check( const std::string &context ) const
{
    if( pieces.empty() ) {
        cata_fatal( "phase of %s has chunk with zero pieces" );
    }
    int min_max = max.minimum();
    if( min_max < 0 ) {
        debugmsg( "phase of %s specifies max which might be as low as %d; this should "
                  "be a positive number", context, min_max );
    }
}

void mutable_overmap_placement_rule::deserialize( const JsonObject &jo )
{
    jo.read( "name", name );
    if( jo.has_member( "overmap" ) ) {
        pieces.emplace_back();
        jo.read( "overmap", pieces.back().overmap_id, true );
    } else if( jo.has_member( "chunk" ) ) {
        jo.read( "chunk", pieces );
    } else {
        jo.throw_error( R"(placement rule must specify at least one of "overmap" or "chunk")" );
    }
    jo.read( "max", max );
    jo.read( "weight", weight );
    if( !jo.has_member( "max" ) && weight == INT_MAX ) {
        jo.throw_error( R"(placement rule must specify at least one of "max" or "weight")" );
    }
}

mutable_overmap_placement_rule_remainder mutable_overmap_placement_rule::realise() const
{
    return mutable_overmap_placement_rule_remainder{ this, max.sample(), weight };
}

cata::views::transform<std::vector<mutable_overmap_placement_rule_piece>, mutable_overmap_piece_candidate>
mutable_overmap_placement_rule_remainder::pieces( const tripoint_om_omt &origin,
        om_direction::type rot ) const
{
    using orig_t = mutable_overmap_placement_rule_piece;
    using dest_t = mutable_overmap_piece_candidate;
    return cata::views::transform < decltype( parent->pieces ), dest_t > ( parent->pieces,
    [origin, rot]( const orig_t &piece ) -> dest_t {
        tripoint_rel_omt rotated_offset = rotate( piece.pos, rot );
        return { piece.overmap, origin + rotated_offset, add( rot, piece.rot ) };
    } );
}

cata::views::transform<std::vector<std::pair<rel_pos_dir, const mutable_overmap_terrain_join *>>,
     mutable_overmap_placement_rule_remainder::dest_outward_t>
     mutable_overmap_placement_rule_remainder::outward_joins( const tripoint_om_omt &origin,
             om_direction::type rot ) const
{
    using orig_t = std::pair<rel_pos_dir, const mutable_overmap_terrain_join *>;
    return cata::views::transform < decltype( parent->outward_joins ),
           dest_outward_t > ( parent->outward_joins,
    [origin, rot]( const orig_t &p ) -> dest_outward_t {
        tripoint_rel_omt rotated_offset = rotate( p.first.p, rot );
        om_pos_dir p_d{ origin + rotated_offset, p.first.dir + rot };
        return { p_d, p.second };
    } );
}

// When building a mutable overmap special we maintain a collection of
// unresolved joins.  We need to be able to index that collection in
// various ways, so it gets its own struct to maintain the relevant invariants.

void joins_tracker::consistency_check() const
{
#if 0 // Enable this to check the class invariants, at the cost of more runtime
    // verify that there are no positions in common between the
    // resolved and postponed lists
    for( const join &j : postponed ) {
        auto j_pos = j.where.p;
        if( unresolved.any_at( j_pos ) ) {
            std::vector<iterator> unr = unresolved.all_at( j_pos );
            if( unr.empty() ) {
                cata_fatal( "inconsistency between all_at and any_at" );
            } else {
                const join &unr_j = *unr.front();
                cata_fatal( "postponed and unresolved should be disjoint but are not at "
                            "%s where unresolved has %s: %s",
                            j_pos.to_string(), unr_j.where.p.to_string(), unr_j.join_id );
            }
        }
    }
#endif
}

joins_tracker::join_status joins_tracker::allows( const om_pos_dir &this_side,
        const mutable_overmap_terrain_join &this_ter_join ) const
{
    om_pos_dir other_side = this_side.opposite();

    auto is_allowed_opposite = [&]( const std::string & candidate ) {
        const mutable_overmap_join &this_join = *this_ter_join.join;

        if( this_join.opposite_id == candidate ) {
            return true;
        }

        for( const mutable_overmap_join *alt_join : this_ter_join.alternative_joins ) {
            if( alt_join->opposite_id == candidate ) {
                return true;
            }
        }

        return false;
    };

    if( const join *existing = resolved.find( other_side ) ) {
        bool other_side_mandatory = unresolved.count( this_side );
        if( is_allowed_opposite( existing->join->id ) ) {
            return other_side_mandatory
                   ? join_status::matched_non_available : join_status::matched_available;
        } else {
            if( other_side_mandatory || this_ter_join.type != join_type::available ) {
                return join_status::disallowed;
            } else {
                return join_status::mismatched_available;
            }
        }
    } else {
        return join_status::free;
    }
}

void joins_tracker::add_joins_for(
    const mutable_overmap_terrain &ter, const tripoint_om_omt &pos,
    om_direction::type rot, const std::vector<om_pos_dir> &suppressed_joins )
{
    consistency_check();

    std::unordered_set<om_pos_dir> avoid(
        suppressed_joins.begin(), suppressed_joins.end() );

    for( const std::pair<const cube_direction, mutable_overmap_terrain_join> &p :
         ter.joins ) {
        cube_direction dir = p.first + rot;
        const mutable_overmap_terrain_join &this_side_join = p.second;

        om_pos_dir this_side{ pos, dir };
        om_pos_dir other_side = this_side.opposite();

        if( const join *other_side_join = resolved.find( other_side ) ) {
            erase_unresolved( this_side );
            if( !avoid.count( this_side ) ) {
                used.emplace_back( other_side, other_side_join->join->id );
                // Because of the existence of alternative joins, we don't
                // simply add this_side_join here, we add the opposite of
                // the opposite that was actually present (this saves us
                // from heaving to search through the alternates to find
                // which one actually matched).
                used.emplace_back( this_side, other_side_join->join->opposite_id );
            }
        } else {
            // If there were postponed joins pointing into this point,
            // so we need to un-postpone them because it might now be
            // possible to satisfy them.
            restore_postponed_at( other_side.p );
            if( this_side_join.type == join_type::mandatory ) {
                if( !overmap::inbounds( other_side.p ) ) {
                    debugmsg( "out of bounds join" );
                    continue;
                }
                const mutable_overmap_join *opposite_join = this_side_join.join->opposite;
                add_unresolved( other_side, opposite_join );
            }
        }
        resolved.add( this_side, this_side_join.join );
    }
    consistency_check();
}

tripoint_om_omt joins_tracker::pick_top_priority() const
{
    cata_assert( any_unresolved() );
    auto priority_it =
        std::find_if( unresolved_priority_index.begin(), unresolved_priority_index.end(),
    []( const cata::flat_set<jl_iterator, compare_iterators> &its ) {
        return !its.empty();
    } );
    cata_assert( priority_it != unresolved_priority_index.end() );
    auto it = random_entry( *priority_it );
    const tripoint_om_omt &pos = it->where.p;
    cata_assert( !postponed.any_at( pos ) );
    return pos;
}
void joins_tracker::postpone( const tripoint_om_omt &pos )
{
    consistency_check();
    for( jl_iterator it : unresolved.all_at( pos ) ) {
        postponed.add( *it );
        [[maybe_unused]] const bool erased = erase_unresolved( it->where );
        cata_assert( erased );
    }
    consistency_check();
}
void joins_tracker::restore_postponed_at( const tripoint_om_omt &pos )
{
    for( jl_iterator it : postponed.all_at( pos ) ) {
        add_unresolved( it->where, it->join );
        postponed.erase( it );
    }
    consistency_check();
}
void joins_tracker::restore_postponed()
{
    consistency_check();
    for( const join &j : postponed ) {
        add_unresolved( j.where, j.join );
    }
    postponed.clear();
}

std::vector<joins_tracker::jl_iterator> joins_tracker::indexed_joins::all_at(
    const tripoint_om_omt &pos ) const
{
    std::vector<jl_iterator> result;
    for( cube_direction dir : all_enum_values<cube_direction>() ) {
        om_pos_dir key{ pos, dir };
        auto pos_it = position_index.find( key );
        if( pos_it != position_index.end() ) {
            result.push_back( pos_it->second );
        }
    }
    return result;
}

bool joins_tracker::indexed_joins::any_at( const tripoint_om_omt &pos ) const
{
    for( cube_direction dir : all_enum_values<cube_direction>() ) {
        if( count( om_pos_dir{ pos, dir } ) ) {
            return true;
        }
    }
    return false;
}

std::size_t joins_tracker::indexed_joins::count_at( const tripoint_om_omt &pos ) const
{
    std::size_t result = 0;
    for( cube_direction dir : all_enum_values<cube_direction>() ) {
        if( position_index.find( { pos, dir } ) != position_index.end() ) {
            ++result;
        }
    }
    return result;
}

void joins_tracker::add_unresolved( const om_pos_dir &p, const mutable_overmap_join *j )
{
    jl_iterator it = unresolved.add( p, j );
    unsigned priority = it->join->priority;
    if( unresolved_priority_index.size() <= priority ) {
        unresolved_priority_index.resize( priority + 1 );
    }
    [[maybe_unused]] const bool inserted = unresolved_priority_index[priority].insert( it ).second;
    cata_assert( inserted );
}

bool joins_tracker::erase_unresolved( const om_pos_dir &p )
{
    auto pos_it = unresolved.position_index.find( p );
    if( pos_it == unresolved.position_index.end() ) {
        return false;
    }
    jl_iterator it = pos_it->second;
    unsigned priority = it->join->priority;
    cata_assert( priority < unresolved_priority_index.size() );
    [[maybe_unused]] const size_t erased = unresolved_priority_index[priority].erase( it );
    cata_assert( erased );
    unresolved.erase( it );
    return true;
}

void mutable_overmap_special_data::finalize( const std::string &context,
        const cata::flat_set<string_id<overmap_location>> &default_locations )
{
    if( check_for_locations.empty() ) {
        check_for_locations.push_back( root_as_overmap_special_terrain() );
    }
    for( size_t i = 0; i != joins_vec.size(); ++i ) {
        mutable_overmap_join &join = joins_vec[i];
        if( join.into_locations.empty() ) {
            join.into_locations = default_locations;
        }
        join.priority = i;
        joins.emplace( join.id, &join );
    }
    for( mutable_overmap_join &join : joins_vec ) {
        if( join.opposite_id.empty() ) {
            join.opposite_id = join.id;
            join.opposite = &join;
            continue;
        }
        auto opposite_it = joins.find( join.opposite_id );
        if( opposite_it == joins.end() ) {
            // Error reported later in check()
            continue;
        }
        join.opposite = opposite_it->second;
    }
    for( std::pair<const std::string, mutable_overmap_terrain> &p : overmaps ) {
        mutable_overmap_terrain &ter = p.second;
        ter.finalize( string_format( "overmap %s in %s", p.first, context ), joins,
                      default_locations );
    }
    for( mutable_overmap_phase &phase : phases ) {
        for( mutable_overmap_placement_rule &rule : phase.rules ) {
            rule.finalize( context, overmaps );
        }
    }
}

void mutable_overmap_special_data::finalize_mapgen_parameters(
    mapgen_parameters &params, const std::string &context ) const
{
    for( const std::pair<const std::string, mutable_overmap_terrain> &p : overmaps ) {
        const mutable_overmap_terrain &t = p.second;
        std::string mapgen_id = t.terrain->get_mapgen_id();
        params.check_and_merge( get_map_special_params( mapgen_id ), context );
    }
}

void mutable_special_connection::check( const std::string &context ) const
{
    if( !connection.is_valid() ) {
        debugmsg( "invalid connection id %s in %s", connection.str(), context );
    }
}

void mutable_overmap_special_data::check( const std::string &context ) const
{
    if( joins_vec.size() != joins.size() ) {
        debugmsg( "duplicate join id in %s", context );
    }
    for( const mutable_overmap_join &join : joins_vec ) {
        if( join.opposite ) {
            if( join.opposite->opposite_id != join.id ) {
                debugmsg( "in %1$s: join id %2$s specifies its opposite to be %3$s, but "
                          "the opposite of %3$s is %4$s, when it should match the "
                          "original id %2$s",
                          context, join.id, join.opposite_id, join.opposite->opposite_id );
            }
        } else {
            debugmsg( "in %s: join id '%s' specified as opposite of '%s' not valid",
                      context, join.opposite_id, join.id );
        }
    }
    for( const std::pair<const std::string, mutable_overmap_terrain> &p : overmaps ) {
        const mutable_overmap_terrain &ter = p.second;
        ter.check( string_format( "overmap %s in %s", p.first, context ) );
    }
    if( !overmaps.count( root ) ) {
        debugmsg( "root %s is not amongst the defined overmaps for %s", root, context );
    }
    for( const mutable_overmap_phase &phase : phases ) {
        for( const mutable_overmap_placement_rule &rule : phase.rules ) {
            rule.check( context );
        }
    }
}

overmap_special_terrain mutable_overmap_special_data::root_as_overmap_special_terrain() const
{
    auto it = overmaps.find( root );
    if( it == overmaps.end() ) {
        debugmsg( "root '%s' is not an overmap in this special", root );
        return {};
    }
    const mutable_overmap_terrain &root_om = it->second;
    return { tripoint_rel_omt::zero, root_om.terrain, root_om.locations, {} };
}


std::vector<overmap_special_terrain> mutable_overmap_special_data::get_terrains() const
{
    debugmsg( "currently not supported" );
    return std::vector<overmap_special_terrain> { root_as_overmap_special_terrain() };
}

std::vector<overmap_special_terrain> mutable_overmap_special_data::preview_terrains() const
{
    return std::vector<overmap_special_terrain> { root_as_overmap_special_terrain() };
}

std::vector<overmap_special_locations> mutable_overmap_special_data::required_locations() const
{
    return check_for_locations;
}

int mutable_overmap_special_data::score_rotation_at( const overmap &, const tripoint_om_omt &,
        om_direction::type ) const
{
    // TODO: worry about connections for mutable specials
    // For now we just allow all rotations, but will be restricted by
    // can_place_special
    return 0;
}

// Returns a list of the points placed and a list of the joins used
special_placement_result mutable_overmap_special_data::place(
    overmap &om, const tripoint_om_omt &origin, om_direction::type dir, bool /*blob*/,
    const city &cit, bool must_be_unexplored ) const
{
    // TODO: respect must_be_unexplored
    std::vector<tripoint_om_omt> result;

    auto it = overmaps.find( root );
    if( it == overmaps.end() ) {
        debugmsg( "Invalid root %s", root );
        return { result, {} };
    }

    joins_tracker unresolved;

    struct placed_connection {
        overmap_connection_id connection;
        pos_dir<tripoint_om_omt> where;
    };

    std::vector<placed_connection> connections_placed;

    // This is for debugging only, it tracks a human-readable description
    // of what happened to be put in the debugmsg in the event of failure.
    std::vector<std::string> descriptions;

    // Helper function to add a particular mutable_overmap_terrain at a
    // particular place.
    auto add_ter = [&](
                       const mutable_overmap_terrain & ter, const tripoint_om_omt & pos,
    om_direction::type rot, const std::vector<om_pos_dir> &suppressed_joins ) {
        const oter_id tid = ter.terrain->get_rotated( rot );
        om.ter_set( pos, tid );
        if( ter.camp_owner.has_value() ) {
            tripoint_abs_omt camp_loc =  {project_combine( om.pos(), pos.xy() ), pos.z()};
            get_map().add_camp( camp_loc, "faction_camp", false );
            std::optional<basecamp *> bcp = overmap_buffer.find_camp( camp_loc.xy() );
            if( !bcp ) {
                debugmsg( "Camp placement during special generation failed at %s", camp_loc.to_string() );
            } else {
                basecamp *temp_camp = *bcp;
                temp_camp->set_owner( ter.camp_owner.value() );
                temp_camp->set_name( ter.camp_name.translated() );
                // FIXME? Camp types are raw strings! Not ideal.
                temp_camp->define_camp( camp_loc, "faction_base_bare_bones_NPC_camp_0", false );
            }
        }
        unresolved.add_joins_for( ter, pos, rot, suppressed_joins );
        result.push_back( pos );

        // Accumulate connections to be dealt with later
        for( const std::pair<const cube_direction, mutable_special_connection> &p :
             ter.connections ) {
            cube_direction base_dir = p.first;
            const mutable_special_connection &conn = p.second;
            cube_direction dir = base_dir + rot;
            tripoint_om_omt conn_pos = pos + displace( dir );
            if( overmap::inbounds( conn_pos ) ) {
                connections_placed.push_back( { conn.connection, { conn_pos, dir } } );
            }
        }
    };

    const mutable_overmap_terrain &root_omt = it->second;
    add_ter( root_omt, origin, dir, {} );

    auto current_phase = phases.begin();
    mutable_overmap_phase_remainder phase_remaining = current_phase->realise();

    while( unresolved.any_unresolved() ) {
        tripoint_om_omt next_pos = unresolved.pick_top_priority();
        mutable_overmap_phase_remainder::satisfy_result satisfy_result =
            phase_remaining.satisfy( om, next_pos, unresolved );
        descriptions.push_back( std::move( satisfy_result.description ) );
        const mutable_overmap_placement_rule_remainder *rule = satisfy_result.rule;
        if( rule ) {
            const tripoint_om_omt &satisfy_origin = satisfy_result.origin;
            om_direction::type rot = satisfy_result.dir;
            for( const mutable_overmap_piece_candidate &piece : rule->pieces( satisfy_origin, rot ) ) {
                const mutable_overmap_terrain &ter = *piece.overmap;
                add_ter( ter, piece.pos, piece.rot, satisfy_result.suppressed_joins );
            }
        } else {
            unresolved.postpone( next_pos );
        }
        if( !unresolved.any_unresolved() || phase_remaining.all_rules_exhausted() ) {
            ++current_phase;
            if( current_phase == phases.end() ) {
                break;
            }
            descriptions.push_back(
                // NOLINTNEXTLINE(cata-translate-string-literal)
                string_format( "## Entering phase %td", current_phase - phases.begin() ) );
            phase_remaining = current_phase->realise();
            unresolved.restore_postponed();
        }
    }

    if( unresolved.any_postponed() ) {
        // This is an error in the JSON; extract some useful info to help
        // the user debug it
        unresolved.restore_postponed();
        tripoint_om_omt p = unresolved.pick_top_priority();

        const oter_id &current_terrain = om.ter( p );
        std::string joins = enumerate_as_string( unresolved.all_unresolved_at( p ),
        []( const joins_tracker::join * dir_join ) {
            // NOLINTNEXTLINE(cata-translate-string-literal)
            return string_format( "%s: %s", io::enum_to_string( dir_join->where.dir ),
                                  dir_join->join->id );
        } );

        debugmsg( "Spawn of mutable special %s had unresolved joins.  Existing terrain "
                  "at %s was %s; joins were %s\nComplete record of placement follows:\n%s",
                  parent_id.str(), p.to_string(), current_terrain.id().str(), joins,
                  string_join( descriptions, "\n" ) );

        om.add_note(
            p, string_format(
                // NOLINTNEXTLINE(cata-translate-string-literal)
                "U:R;DEBUG: unresolved joins %s at %s placing %s",
                joins, p.to_string(), parent_id.str() ) );
    }

    // Deal with connections
    // TODO: JSONification of logic + don't treat non roads like roads + deduplicate with fixed data
    for( const placed_connection &elem : connections_placed ) {
        const tripoint_om_omt &pos = elem.where.p;
        cube_direction connection_dir = elem.where.dir;

        point_om_omt target;
        if( cit ) {
            target = cit.pos;
        } else {
            target = om.get_fallback_road_connection_point();
        }
        om.build_connection( target, pos.xy(), pos.z(), *elem.connection, must_be_unexplored,
                             connection_dir );
    }

    return { result, unresolved.all_used() };
}

void mutable_overmap_special_data::load( const JsonObject &jo, bool was_loaded )
{
    optional( jo, was_loaded, "check_for_locations", check_for_locations );
    if( jo.has_array( "check_for_locations_area" ) ) {
        JsonArray jar = jo.get_array( "check_for_locations_area" );
        while( jar.has_more() ) {
            JsonObject joc = jar.next_object();

            cata::flat_set<string_id<overmap_location>> type;
            tripoint_rel_omt from;
            tripoint_rel_omt to;
            mandatory( joc, was_loaded, "type", type );
            mandatory( joc, was_loaded, "from", from );
            mandatory( joc, was_loaded, "to", to );
            if( from.x() > to.x() ) {
                std::swap( from.x(), to.x() );
            }
            if( from.y() > to.y() ) {
                std::swap( from.y(), to.y() );
            }
            if( from.z() > to.z() ) {
                std::swap( from.z(), to.z() );
            }
            for( int x = from.x(); x <= to.x(); x++ ) {
                for( int y = from.y(); y <= to.y(); y++ ) {
                    for( int z = from.z(); z <= to.z(); z++ ) {
                        overmap_special_locations loc;
                        loc.p = tripoint_rel_omt( x, y, z );
                        loc.locations = type;
                        check_for_locations.push_back( loc );
                    }
                }
            }
        }
    }
    mandatory( jo, was_loaded, "joins", joins_vec );
    mandatory( jo, was_loaded, "overmaps", overmaps );
    mandatory( jo, was_loaded, "root", root );
    mandatory( jo, was_loaded, "phases", phases );
}
