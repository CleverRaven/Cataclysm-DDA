#include "start_location.h"

#include <algorithm>
#include <climits>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "city.h"
#include "clzones.h"
#include "colony.h"
#include "coordinates.h"
#include "creature.h"
#include "debug.h"
#include "effect_source.h"
#include "enum_conversions.h"
#include "field_type.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "json.h"
#include "map.h"
#include "map_extras.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "rng.h"

class item;

static const efftype_id effect_bleed( "bleed" );

static const ter_str_id ter_t_curtains( "t_curtains" );
static const ter_str_id ter_t_door_boarded( "t_door_boarded" );
static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_door_c_peep( "t_door_c_peep" );
static const ter_str_id ter_t_door_locked( "t_door_locked" );
static const ter_str_id ter_t_door_o( "t_door_o" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_window( "t_window" );
static const ter_str_id ter_t_window_boarded( "t_window_boarded" );
static const ter_str_id ter_t_window_domestic( "t_window_domestic" );
static const ter_str_id ter_t_window_no_curtains( "t_window_no_curtains" );

static const zone_type_id zone_type_ZONE_START_POINT( "ZONE_START_POINT" );

namespace
{
generic_factory<start_location> all_start_locations( "start locations" );
} // namespace

/** @relates string_id */
template<>
const start_location &string_id<start_location>::obj() const
{
    return all_start_locations.obj( *this );
}

/** @relates int_id */
template<>
int_id<start_location> string_id<start_location>::id() const
{
    return all_start_locations.convert( *this, int_id<start_location>( -1 ) );
}

/** @relates string_id */
template<>
bool string_id<start_location>::is_valid() const
{
    return all_start_locations.is_valid( *this );
}

std::string start_location::name() const
{
    return _name.translated();
}

int start_location::targets_count() const
{
    return _locations.size();
}

omt_types_parameters start_location::random_target() const
{
    return random_entry( _locations );
}

bool start_location::requires_city() const
{
    return constraints_.city_size.min > 0 ||
           constraints_.city_distance.max < std::max( OMAPX, OMAPY );
}

bool start_location::can_belong_to_city( const tripoint_om_omt &p, const city &cit ) const
{
    if( !requires_city() ) {
        return true;
    }
    if( !cit || !constraints_.city_size.contains( cit.size ) ) {
        return false;
    }
    return constraints_.city_distance.contains( cit.get_distance_from( p ) - ( cit.size ) );
}

const std::set<std::string> &start_location::flags() const
{
    return _flags;
}

void start_location::load( const JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";

    mandatory( jo, was_loaded, "name", _name );
    std::string ter;
    for( const JsonValue entry : jo.get_array( "terrain" ) ) {
        ot_match_type ter_match_type = ot_match_type::type;
        std::unordered_map<std::string, std::string> parameter_map;
        if( entry.test_string() ) {
            ter = entry.get_string();
        } else {
            JsonObject jot = entry.get_object();
            ter = jot.get_string( "om_terrain" );
            if( jot.has_string( "om_terrain_match_type" ) ) {
                ter_match_type = jot.get_enum_value<ot_match_type>( "om_terrain_match_type", ter_match_type );
            }
            if( jot.has_object( "parameters" ) ) {
                std::unordered_map<std::string, std::string> parameter_map;
                jot.read( "parameters", parameter_map );
            }
        }
        _locations.emplace_back( omt_types_parameters{ ter, ter_match_type, parameter_map } );
    }
    if( jo.has_array( "city_sizes" ) ) {
        assign( jo, "city_sizes", constraints_.city_size, strict );
    }
    if( jo.has_array( "city_distance" ) ) {
        assign( jo, "city_distance", constraints_.city_distance, strict );
    }
    if( jo.has_array( "allowed_z_levels" ) ) {
        assign( jo, "allowed_z_levels", constraints_.allowed_z_levels, strict );
    }
    optional( jo, was_loaded, "flags", _flags, auto_flags_reader<> {} );
}

void start_location::check() const
{
}

void start_location::finalize()
{
}

// check if tile at p should be boarded with some kind of furniture.
static void add_boardable( const tinymap &m, const tripoint &p, std::vector<tripoint> &vec )
{
    if( m.has_furn( p ) ) {
        // Don't need to board this up, is already occupied
        return;
    }
    if( m.ter( p ) != ter_t_floor ) {
        // Other terrain (door, wall, ...), not boarded either
        return;
    }
    if( m.is_outside( p ) ) {
        // Don't board up the outside
        return;
    }
    if( std::find( vec.begin(), vec.end(), p ) != vec.end() ) {
        // Already registered to be boarded
        return;
    }
    vec.push_back( p );
}

static void board_up( tinymap &m, const tripoint_range<tripoint> &range )
{
    std::vector<tripoint> furnitures1;
    std::vector<tripoint> furnitures2;
    std::vector<tripoint> boardables;
    for( const tripoint &p : range ) {
        bool must_board_around = false;
        const ter_id t = m.ter( p );
        if( t == ter_t_window_domestic || t == ter_t_window || t == ter_t_window_no_curtains ) {
            // Windows are always to the outside and must be boarded
            must_board_around = true;
            m.ter_set( p, ter_t_window_boarded );
        } else if( t == ter_t_door_c || t == ter_t_door_locked || t == ter_t_door_c_peep ) {
            // Only board up doors that lead to the outside
            if( m.is_outside( p + tripoint_north ) || m.is_outside( p + tripoint_south ) ||
                m.is_outside( p + tripoint_east ) || m.is_outside( p + tripoint_west ) ) {
                m.ter_set( p, ter_t_door_boarded );
                must_board_around = true;
            } else {
                // internal doors are opened instead
                m.ter_set( p, ter_t_door_o );
            }
        }
        if( must_board_around ) {
            // Board up the surroundings of the door/window
            for( const tripoint &neigh : points_in_radius( p, 1 ) ) {
                if( neigh == p ) {
                    continue;
                }
                add_boardable( m, neigh, boardables );
            }
        }
    }
    // Find all furniture that can be used to board up some place
    for( const tripoint &p : range ) {
        if( std::find( boardables.begin(), boardables.end(), p ) != boardables.end() ) {
            continue;
        }
        if( !m.has_furn( p ) ) {
            continue;
        }
        // If the furniture is movable and the character can move it, use it to barricade
        //  is workable here as NPCs by definition are not starting the game.  (Let's hope.)
        ///\EFFECT_STR determines what furniture might be used as a starting area barricade
        if( m.furn( p ).obj().is_movable() &&
            m.furn( p ).obj().move_str_req < get_player_character().get_str() ) {
            if( m.furn( p ).obj().movecost == 0 ) {
                // Obstacles are better, prefer them
                furnitures1.push_back( p );
            } else {
                furnitures2.push_back( p );
            }
        }
    }
    while( ( !furnitures1.empty() || !furnitures2.empty() ) && !boardables.empty() ) {
        const tripoint fp = random_entry_removed( furnitures1.empty() ? furnitures2 : furnitures1 );
        const tripoint bp = random_entry_removed( boardables );
        m.furn_set( bp, m.furn( fp ) );
        m.furn_set( fp, furn_str_id::NULL_ID() );
        map_stack destination_items = m.i_at( bp );
        for( const item &moved_item : m.i_at( fp ) ) {
            destination_items.insert( moved_item );
        }
        m.i_clear( fp );
    }
}

void start_location::prepare_map( tinymap &m ) const
{
    const int z = m.get_abs_sub().z();
    if( flags().count( "BOARDED" ) > 0 ) {
        m.build_outside_cache( z );
        board_up( m, m.points_on_zlevel( z ) );
    } else {
        m.translate( ter_t_window_domestic, ter_t_curtains );
    }
}

std::pair<tripoint_abs_omt, std::unordered_map<std::string, std::string>>
        start_location::find_player_initial_location( const point_abs_om &origin ) const
{
    // Spiral out from the world origin scanning for a compatible starting location,
    // creating overmaps as necessary.
    const int radius = 3;
    const omt_types_parameters chosen_target = random_target();
    for( const point_abs_om &omp : closest_points_first( origin, radius ) ) {
        overmap &omap = overmap_buffer.get( omp );
        const tripoint_om_omt omtstart = omap.find_random_omt( std::make_pair( chosen_target.omt,
                                         chosen_target.omt_type ) );
        if( omtstart.raw() != tripoint_min ) {
            return std::make_pair( project_combine( omp, omtstart ), chosen_target.parameters );
        }
    }
    // Should never happen, if it does we messed up.
    popup( _( "Unable to generate a valid starting location %s [%s] in a radius of %d overmaps, please report this failure." ),
           name(), id.str(), radius );
    return std::make_pair( overmap::invalid_tripoint, chosen_target.parameters );
}

std::pair<tripoint_abs_omt, std::unordered_map<std::string, std::string>>
        start_location::find_player_initial_location( const city &origin ) const
{
    overmap &omap = overmap_buffer.get( origin.pos_om );
    std::vector<std::pair<tripoint_om_omt, omt_types_parameters>> valid;
    for( const point_om_omt &omp : closest_points_first( origin.pos, origin.size ) ) {
        for( int k = constraints_.allowed_z_levels.min; k <= constraints_.allowed_z_levels.max; k++ ) {
            tripoint_om_omt p( omp, k );
            if( !can_belong_to_city( p, origin ) ) {
                continue;
            }
            auto target_is_ot_match = [&]( const omt_types_parameters & target ) {
                return is_ot_match( target.omt, omap.ter( p ), target.omt_type );
            };
            auto it = std::find_if( _locations.begin(), _locations.end(),
                                    target_is_ot_match );
            if( it != _locations.end() ) {
                valid.emplace_back( p, *it );
            }
        }
    }
    const std::pair<tripoint_om_omt, omt_types_parameters> random_valid = random_entry( valid,
            std::make_pair( tripoint_om_omt( tripoint_min ), omt_types_parameters() ) );
    const tripoint_om_omt omtstart = random_valid.first;
    if( omtstart.raw() != tripoint_min ) {
        return std::make_pair( project_combine( origin.pos_om, omtstart ), random_valid.second.parameters );
    }
    // Should never happen, if it does we messed up.
    popup( _( "Unable to generate a valid starting location %s [%s] in a city [%s], please report this failure." ),
           name(), id.str(), origin.name );
    return std::make_pair( overmap::invalid_tripoint, random_valid.second.parameters );
}

void start_location::set_parameters( const tripoint_abs_omt &omtstart,
                                     const std::unordered_map<std::string, std::string> &parameters_to_set ) const
{
    if( parameters_to_set.empty() ) {
        return;
    }
    overmap_buffer.externally_set_args = true;
    std::optional<mapgen_arguments> *maybe_args = overmap_buffer.mapgen_args( omtstart );
    if( !maybe_args ) {
        debugmsg( "No overmap special args at start location." );
        return;
    }
    std::optional<overmap_special_id> s = overmap_buffer.overmap_special_at( omtstart );
    if( !s ) {
        debugmsg( "No overmap special at start location from which to fetch parameters." );
        return;
    }
    const overmap_special &special = **s;
    const mapgen_parameters &params = special.get_params();
    mapgen_arguments args;
    for( const auto &param_to_set : parameters_to_set ) {
        const std::string &param_name_to_set = param_to_set.first;
        const std::string &value_to_set = param_to_set.second;
        auto param_it = params.map.find( param_name_to_set );
        if( param_it == params.map.end() ) {
            debugmsg( "Parameter %s not found", param_name_to_set );
            continue;
        }
        const mapgen_parameter &param = param_it->second;
        if( param.scope() != mapgen_parameter_scope::overmap_special ) {
            debugmsg( "Parameter %s is not of scope overmap_special", param_name_to_set );
            continue;
        }
        std::vector<std::string> possible_values = param.all_possible_values( params );
        auto value_it = std::find( possible_values.begin(), possible_values.end(), value_to_set );
        if( value_it == possible_values.end() ) {
            debugmsg( "Parameter value %s for parameter %s not found", value_to_set, param_name_to_set );
            continue;
        }
        if( *maybe_args ) {
            maybe_args->value().map[param_name_to_set] =
                cata_variant::from_string( param.type(), std::move( *value_it ) );
        } else {
            mapgen_arguments args;
            args.map[param_name_to_set] =
                cata_variant::from_string( param.type(), std::move( *value_it ) );
            *maybe_args = args;
        }
    }
}

void start_location::prepare_map( const tripoint_abs_omt &omtstart ) const
{
    // Now prepare the initial map (change terrain etc.)
    tinymap player_start;
    player_start.load( omtstart, false );
    prepare_map( player_start );
    player_start.save();
}

/** Helper for place_player
 * Flood-fills the area from a given point, then returns the area it found.
 * Considers unpassable tiles passable if they can be bashed down or opened.
 * Will return INT_MAX if it reaches upstairs or map edge.
 * We can't really use map::route here, because it expects a destination
 * Maybe TODO: Allow "picking up" items or parts of bashable furniture
 *             and using them to help with bash attempts.
 */
static int rate_location( map &m, const tripoint &p,
                          const bool must_be_inside, const bool accommodate_npc,
                          const int bash_str, const int attempt,
                          cata::mdarray<int, point_bub_ms> &checked )
{
    const auto invalid_char_pos = [&]( const tripoint & tp ) -> bool {
        return ( must_be_inside && m.is_outside( tp ) ) ||
        m.impassable( tp ) || m.is_divable( tp ) ||
        m.has_flag( ter_furn_flag::TFLAG_NO_FLOOR, tp );
    };

    if( checked[p.x][p.y] > 0 || invalid_char_pos( p ) ||
        ( accommodate_npc && invalid_char_pos( p + point_north_west ) ) ) {
        return 0;
    }

    // Vector that will be used as a stack
    std::vector<tripoint> st;
    st.reserve( MAPSIZE_X * MAPSIZE_Y );
    st.push_back( p );

    // If not checked yet and either can be moved into, can be bashed down or opened,
    // add it on the top of the stack.
    const auto maybe_add = [&]( const point & add_p, const tripoint & from ) {
        if( checked[add_p.x][add_p.y] >= attempt ) {
            return;
        }

        const tripoint pt( add_p, p.z );
        if( m.passable( pt ) ||
            m.bash_resistance( pt ) <= bash_str ||
            m.open_door( get_avatar(), pt, !m.is_outside( from ), true ) ) {
            st.push_back( pt );
        }
    };

    int area = 0;
    while( !st.empty() ) {
        area++;
        const tripoint cur = st.back();
        st.pop_back();

        checked[cur.x][cur.y] = attempt;
        if( cur.x == 0 || cur.x == MAPSIZE_X - 1 ||
            cur.y == 0 || cur.y == MAPSIZE_Y - 1 ||
            m.has_flag( ter_furn_flag::TFLAG_GOES_UP, cur ) ) {
            return INT_MAX;
        }

        maybe_add( cur.xy() + point_west, cur );
        maybe_add( cur.xy() + point_north, cur );
        maybe_add( cur.xy() + point_east, cur );
        maybe_add( cur.xy() + point_south, cur );
        maybe_add( cur.xy() + point_north_west, cur );
        maybe_add( cur.xy() + point_north_east, cur );
        maybe_add( cur.xy() + point_south_west, cur );
        maybe_add( cur.xy() + point_south_east, cur );
    }

    return area;
}

void start_location::place_player( avatar &you, const tripoint_abs_omt &omtstart ) const
{
    // Need the "real" map with it's inside/outside cache and the like.
    map &here = get_map();
    // Start us off somewhere in the center of the map
    you.move_to( midpoint( project_bounds<coords::ms>( omtstart ) ) );
    here.invalidate_map_cache( here.get_abs_sub().z() );
    here.build_map_cache( here.get_abs_sub().z() );
    const bool must_be_inside = flags().count( "ALLOW_OUTSIDE" ) == 0;
    const bool accommodate_npc = flags().count( "LONE_START" ) == 0;
    ///\EFFECT_STR allows player to start behind less-bashable furniture and terrain
    // TODO: Allow using items here
    const int bash = you.get_str();

    // Remember biggest found location
    // Sometimes it may be impossible to automatically found an ideal location
    // but the player may be more creative than this algorithm and do away with just "good"
    int best_rate = 0;
    tripoint best_spot = you.pos();
    // In which attempt did this area get checked?
    // We can overwrite earlier attempts, but not start in them
    cata::mdarray<int, point_bub_ms> checked = {};

    bool found_good_spot = false;

    //Check if a start_point zone exists first
    const zone_manager &mgr = zone_manager::get_manager();
    for( const auto &i : mgr.get_zones() ) {
        const zone_data &zone = i.get();
        if( zone.get_type() == zone_type_ZONE_START_POINT ) {
            if( here.inbounds( zone.get_center_point() ) ) {
                found_good_spot = true;
                best_spot = here.getlocal( zone.get_center_point() );
                break;
            }
        }
    }

    // Otherwise, find a random starting spot

    int tries = 0;
    const auto check_spot = [&]( const tripoint & pt ) {
        ++tries;
        const int rate = rate_location( here, pt, must_be_inside, accommodate_npc, bash, tries, checked );
        if( best_rate < rate ) {
            best_rate = rate;
            best_spot = pt;
            if( rate == INT_MAX ) {
                return true;
            }
        }
        return false;
    };

    while( !found_good_spot && tries < 100 ) {
        tripoint rand_point( HALF_MAPSIZE_X + rng( 0, SEEX * 2 - 1 ),
                             HALF_MAPSIZE_Y + rng( 0, SEEY * 2 - 1 ),
                             you.posz() );
        found_good_spot = check_spot( rand_point );
    }
    // If we haven't got a good location by now, screw it and brute force it
    // This only happens in exotic locations (deep of a science lab), but it does happen
    if( !found_good_spot ) {
        tripoint tmp = you.pos();
        int &x = tmp.x;
        int &y = tmp.y;
        for( x = 0; x < MAPSIZE_X; x++ ) {
            for( y = 0; y < MAPSIZE_Y && !found_good_spot; y++ ) {
                found_good_spot = check_spot( tmp );
            }
        }
    }

    you.setpos( best_spot );

    if( !found_good_spot ) {
        debugmsg( "Could not find a good starting place for character" );
    }
}

void start_location::burn( const tripoint_abs_omt &omtstart, const size_t count,
                           const int rad ) const
{
    tinymap m;
    m.load( omtstart, false );
    m.build_outside_cache( m.get_abs_sub().z() );
    point player_pos = get_player_character().pos().xy();
    const point u( player_pos.x % HALF_MAPSIZE_X, player_pos.y % HALF_MAPSIZE_Y );
    std::vector<tripoint> valid;
    for( const tripoint &p : m.points_on_zlevel() ) {
        if( !( m.has_flag_ter( ter_furn_flag::TFLAG_DOOR, p ) ||
               m.has_flag_ter( ter_furn_flag::TFLAG_OPENCLOSE_INSIDE, p ) ||
               m.is_outside( p ) ||
               ( p.x >= u.x - rad && p.x <= u.x + rad && p.y >= u.y - rad && p.y <= u.y + rad ) ) ) {
            if( m.has_flag( ter_furn_flag::TFLAG_FLAMMABLE, p ) ||
                m.has_flag( ter_furn_flag::TFLAG_FLAMMABLE_ASH, p ) ) {
                valid.push_back( p );
            }
        }
    }
    std::shuffle( valid.begin(), valid.end(), rng_get_engine() );
    for( size_t i = 0; i < std::min( count, valid.size() ); i++ ) {
        m.add_field( valid[i], fd_fire, 3 );
    }
    m.save();
}

void start_location::add_map_extra( const tripoint_abs_omt &omtstart,
                                    const map_extra_id &map_extra ) const
{
    tinymap m;
    m.load( omtstart, false );

    MapExtras::apply_function( map_extra, m, omtstart );

    m.save();
}

void start_location::handle_heli_crash( avatar &you ) const
{
    for( const bodypart_id &bp : you.get_all_body_parts( get_body_part_flags::only_main ) ) {
        if( bp == bodypart_id( "head" ) || bp == bodypart_id( "torso" ) ) {
            continue;// Skip head + torso for balance reasons.
        }
        const int roll = static_cast<int>( rng( 1, 8 ) );
        switch( roll ) {
            // Damage + Bleed
            case 1:
            case 2:
                you.add_effect( effect_bleed, 6_minutes, bp );
            /* fallthrough */
            case 3:
            case 4:
            // Just damage
            case 5: {
                const int maxHp = you.get_hp_max( bp );
                // Body part health will range from 33% to 66% with occasional bleed
                const int dmg = static_cast<int>( rng( maxHp / 3, maxHp * 2 / 3 ) );
                you.apply_damage( nullptr, bp, dmg );
                break;
            }
            // No damage
            default:
                break;
        }
    }
}

static void add_monsters( const tripoint_abs_omt &omtstart, const mongroup_id &type,
                          float expected_points )
{
    tinymap m;
    m.load( omtstart, false );
    // map::place_spawns internally multiplies density by rng(10, 50)
    const float density = expected_points / ( ( 10 + 50 ) / 2.0 );
    m.place_spawns( type, 1, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), density );
    m.save();
}

void start_location::surround_with_monsters(
    const tripoint_abs_omt &omtstart, const mongroup_id &type, float expected_points ) const
{
    for( const tripoint_abs_omt &p : points_in_radius( omtstart, 1 ) ) {
        if( p != omtstart ) {
            add_monsters( p, type, roll_remainder( expected_points / 8.0f ) );
        }
    }
}

void start_locations::load( const JsonObject &jo, const std::string &src )
{
    all_start_locations.load( jo, src );
}

void start_locations::finalize_all()
{
    all_start_locations.finalize();
    for( const start_location &start_loc : all_start_locations.get_all() ) {
        const_cast<start_location &>( start_loc ).finalize();
    }
}

void start_locations::check_consistency()
{
    all_start_locations.check();
}

void start_locations::reset()
{
    all_start_locations.reset();
}

const std::vector<start_location> &start_locations::get_all()
{
    return all_start_locations.get_all();
}
