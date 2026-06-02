#include "mortar.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <unordered_map>
#include <utility>

#include "cata_utility.h"
#include "debug.h"
#include "generic_factory.h"
#include "item.h"
#include "map_scale_constants.h"
#include "point.h"
#include "rng.h"

namespace
{

generic_factory<mortar_type> mortar_type_factory( "mortar_type" );

std::unordered_map<furn_str_id, const mortar_type *> mortar_by_furniture;

constexpr double circular_cep_sigma_factor = 1.1774;
constexpr double one_dimensional_probable_error_sigma_factor = 0.67448975;
constexpr int mortar_minimum_launcher_skill = 4;
constexpr double mortar_min_skill_error_multiplier = 3.0;
constexpr double mortar_multiplier_soft_cap_threshold = 10.0;
constexpr double mortar_multiplier_hard_cap = 70.0;
constexpr double mortar_multiplier_above_soft_cap_scale = 0.5;

int interpolate_flight_seconds( const int distance, const int lower_distance,
                                const int lower_seconds, const int upper_distance,
                                const int upper_seconds )
{
    const double range_fraction = static_cast<double>( distance - lower_distance ) /
                                  ( upper_distance - lower_distance );
    return static_cast<int>( std::round( lower_seconds +
                                         ( upper_seconds - lower_seconds ) * range_fraction ) );
}

std::pair<int, int> mortar_60mm_flight_time_bounds( const int distance )
{
    const int clamped_distance = std::max( 0, distance );
    if( clamped_distance <= 500 ) {
        return { 10, 20 };
    }
    if( clamped_distance <= 1000 ) {
        return {
            interpolate_flight_seconds( clamped_distance, 500, 10, 1000, 15 ),
            interpolate_flight_seconds( clamped_distance, 500, 20, 1000, 25 )
        };
    }
    if( clamped_distance <= 2000 ) {
        return {
            interpolate_flight_seconds( clamped_distance, 1000, 15, 2000, 25 ),
            interpolate_flight_seconds( clamped_distance, 1000, 25, 2000, 40 )
        };
    }
    if( clamped_distance <= 3000 ) {
        return {
            interpolate_flight_seconds( clamped_distance, 2000, 25, 3000, 35 ),
            interpolate_flight_seconds( clamped_distance, 2000, 40, 3000, 50 )
        };
    }
    return { 35, 50 };
}

time_duration mortar_60mm_flight_time( const int distance )
{
    const std::pair<int, int> bounds = mortar_60mm_flight_time_bounds( distance );
    return time_duration::from_seconds( rng( bounds.first, bounds.second ) );
}

std::pair<double, double> axis_unit( const tripoint_abs_ms &axis_from,
                                     const tripoint_abs_ms &axis_to )
{
    double ux = axis_to.x() - axis_from.x();
    double uy = axis_to.y() - axis_from.y();
    const double axis_length = std::hypot( ux, uy );
    if( axis_length > 0.0 ) {
        ux /= axis_length;
        uy /= axis_length;
    } else {
        ux = 1.0;
        uy = 0.0;
    }
    return { ux, uy };
}

tripoint_abs_ms apply_axis_dispersion( const tripoint_abs_ms &target,
                                       const tripoint_abs_ms &axis_from,
                                       const tripoint_abs_ms &axis_to,
                                       const double major_sigma,
                                       const double minor_sigma )
{
    const auto [ux, uy] = axis_unit( axis_from, axis_to );

    const double major_offset = normal_roll( 0.0, major_sigma );
    const double minor_offset = normal_roll( 0.0, minor_sigma );
    const double dx = major_offset * ux - minor_offset * uy;
    const double dy = major_offset * uy + minor_offset * ux;
    return tripoint_abs_ms( target.x() + static_cast<int>( std::round( dx ) ),
                            target.y() + static_cast<int>( std::round( dy ) ),
                            target.z() );
}

tripoint_abs_ms clamp_to_max_range( const tripoint_abs_ms &origin,
                                    const tripoint_abs_ms &impact,
                                    const int max_range )
{
    const point d( impact.x() - origin.x(), impact.y() - origin.y() );
    const double distance = std::hypot( d.x, d.y );
    if( distance <= max_range || distance == 0.0 ) {
        return impact;
    }

    const double scale = max_range / distance;
    const auto scale_coord = [scale]( const int delta ) {
        const int magnitude = static_cast<int>( std::floor( std::abs( delta ) * scale ) );
        return delta < 0 ? -magnitude : magnitude;
    };
    return tripoint_abs_ms( origin.x() + scale_coord( d.x ),
                            origin.y() + scale_coord( d.y ),
                            impact.z() );
}

double mortar_elliptic_distance( const tripoint_abs_ms &axis_from,
                                 const tripoint_abs_ms &axis_to,
                                 const tripoint_abs_ms &center,
                                 const tripoint_abs_ms &point,
                                 const mortar_error &error )
{
    const auto [ux, uy] = axis_unit( axis_from, axis_to );
    const double dx = point.x() - center.x();
    const double dy = point.y() - center.y();
    const double range_offset = dx * ux + dy * uy;
    const double deflection_offset = -dx * uy + dy * ux;
    const double range_error = std::max( 1.0, error.range );
    const double deflection_error = std::max( 1.0, error.deflection );
    return std::hypot( range_offset / range_error, deflection_offset / deflection_error );
}

} // namespace

template<>
const mortar_type &string_id<mortar_type>::obj() const
{
    return mortar_type_factory.obj( *this );
}

template<>
const mortar_type &int_id<mortar_type>::obj() const
{
    return mortar_type_factory.obj( *this );
}

template<>
int_id<mortar_type> string_id<mortar_type>::id() const
{
    return mortar_type_factory.convert( *this, int_id<mortar_type>( -1 ) );
}

template<>
const string_id<mortar_type> &int_id<mortar_type>::id() const
{
    return mortar_type_factory.convert( *this );
}

template<>
bool int_id<mortar_type>::is_valid() const
{
    return mortar_type_factory.is_valid( *this );
}

template<>
bool string_id<mortar_type>::is_valid() const
{
    return mortar_type_factory.is_valid( *this );
}

template<>
int_id<mortar_type>::int_id( const string_id<mortar_type> &id ) : _id( id.id() )
{
}

void mortar_type::load_mortar_type( const JsonObject &jo, const std::string &src )
{
    mortar_type_factory.load( jo, src );
}

void mortar_type::finalize_all()
{
    mortar_type_factory.finalize();
    mortar_by_furniture.clear();
    for( const mortar_type &mortar : get_all() ) {
        const auto [iter, inserted] = mortar_by_furniture.emplace( mortar.furniture_, &mortar );
        if( !inserted ) {
            debugmsg( "Mortar type %s and %s both use furniture %s.", mortar.id.c_str(),
                      iter->second->id.c_str(), mortar.furniture_.c_str() );
        }
    }
}

void mortar_type::check_consistency()
{
    for( const mortar_type &mortar : get_all() ) {
        if( !mortar.furniture_.is_valid() ) {
            debugmsg( "Mortar type %s has invalid furniture %s.", mortar.id.c_str(),
                      mortar.furniture_.c_str() );
        }
        if( !mortar.ammo_.is_valid() ) {
            debugmsg( "Mortar type %s has invalid ammo type %s.", mortar.id.c_str(),
                      mortar.ammo_.c_str() );
        }
        if( mortar.range_ <= 0 ) {
            debugmsg( "Mortar type %s has invalid range %d.", mortar.id.c_str(), mortar.range_ );
        }
        if( mortar.range_error_ratio_ <= 0.0 || mortar.deflection_error_mils_ <= 0.0 ) {
            debugmsg( "Mortar type %s has invalid CEP values.", mortar.id.c_str() );
        }
    }
}

void mortar_type::reset()
{
    mortar_type_factory.reset();
    mortar_by_furniture.clear();
}

const std::vector<mortar_type> &mortar_type::get_all()
{
    return mortar_type_factory.get_all();
}

const mortar_type *mortar_type::from_furniture( const furn_str_id &furn )
{
    const auto iter = mortar_by_furniture.find( furn );
    return iter != mortar_by_furniture.end() ? iter->second : nullptr;
}

bool mortar_type::is_mortar_round( const item &it )
{
    return std::any_of( get_all().begin(), get_all().end(), [&it]( const mortar_type & mortar ) {
        return it.ammo_type() == mortar.ammo_;
    } );
}

int mortar_type::minimum_launcher_skill()
{
    return mortar_minimum_launcher_skill;
}

double mortar_type::skill_accuracy_multiplier( const int launcher_skill )
{
    const double skill = clamp<double>( launcher_skill, mortar_minimum_launcher_skill, 10.0 );
    return 1.0 + ( 10.0 - skill ) *
           ( ( mortar_min_skill_error_multiplier - 1.0 ) / ( 10.0 - mortar_minimum_launcher_skill ) );
}

double mortar_type::effective_ballistic_multiplier( const double raw_multiplier )
{
    if( raw_multiplier <= mortar_multiplier_soft_cap_threshold ) {
        return std::max( 1.0, raw_multiplier );
    }
    return std::min( mortar_multiplier_hard_cap,
                     mortar_multiplier_soft_cap_threshold +
                     ( raw_multiplier - mortar_multiplier_soft_cap_threshold ) *
                     mortar_multiplier_above_soft_cap_scale );
}

int mortar_heading_degrees( const tripoint_abs_ms &from, const tripoint_abs_ms &to )
{
    static constexpr double pi = 3.14159265358979323846;
    const double dx = to.x() - from.x();
    const double dy = to.y() - from.y();
    int degrees = static_cast<int>( std::round( std::atan2( dx, -dy ) * 180.0 / pi ) );
    if( degrees < 0 ) {
        degrees += 360;
    }
    return degrees;
}

tripoint_abs_ms mortar_make_creeping_axis_to( const tripoint_abs_ms &target,
        const tripoint_abs_ms &spotter_pos, const tripoint_abs_ms &mortar_pos )
{
    double dx = target.x() - spotter_pos.x();
    double dy = target.y() - spotter_pos.y();
    double length = std::hypot( dx, dy );
    if( length <= 0.0 ) {
        dx = target.x() - mortar_pos.x();
        dy = target.y() - mortar_pos.y();
        length = std::hypot( dx, dy );
    }
    if( length <= 0.0 ) {
        dx = 1.0;
        dy = 0.0;
        length = 1.0;
    }
    dx /= length;
    dy /= length;
    return tripoint_abs_ms( target.x() + static_cast<int>( std::round( dx * 1000.0 ) ),
                            target.y() + static_cast<int>( std::round( dy * 1000.0 ) ),
                            target.z() );
}

mortar_creeping_solution mortar_creeping_adjustment( const tripoint_abs_ms &mortar_pos,
        const tripoint_abs_ms &target, const tripoint_abs_ms &axis_to,
        const tripoint_abs_ms &spotter_pos, const mortar_error &error )
{
    const double player_error_distance = mortar_elliptic_distance( mortar_pos, target, target,
                                         spotter_pos, error );
    const bool danger_close = player_error_distance <= 2.0;
    const double offset_multiplier = danger_close ? ( player_error_distance > 1.0 ? 1.5 : 2.0 ) :
                                     1.0;

    const auto [offset_ux, offset_uy] = axis_unit( target, axis_to );
    const auto [range_ux, range_uy] = axis_unit( mortar_pos, target );
    const double range_component = offset_ux * range_ux + offset_uy * range_uy;
    const double deflection_component = -offset_ux * range_uy + offset_uy * range_ux;
    const double range_error = std::max( 1.0, error.range );
    const double deflection_error = std::max( 1.0, error.deflection );
    const double denominator = std::hypot( range_component / range_error,
                                           deflection_component / deflection_error );
    const double offset_distance = denominator > 0.0 ? offset_multiplier / denominator : 0.0;
    const tripoint_abs_ms center( target.x() + static_cast<int>( std::round( offset_ux *
                                  offset_distance ) ),
                                  target.y() + static_cast<int>( std::round( offset_uy *
                                          offset_distance ) ),
                                  target.z() );
    return mortar_creeping_solution{ center, mortar_heading_degrees( target, center ),
                                     danger_close, offset_multiplier };
}

void mortar_type::load( const JsonObject &jo, std::string_view )
{
    const numeric_bound_reader<int> positive_int{ 1 };
    const numeric_bound_reader<double> positive_double{ std::numeric_limits<double>::min() };

    mandatory( jo, was_loaded, "furniture", furniture_ );
    mandatory( jo, was_loaded, "ammo", ammo_ );
    mandatory( jo, was_loaded, "range", range_, positive_int );
    mandatory( jo, was_loaded, "npc_fire_message_delay", npc_fire_message_delay_ );
    optional( jo, was_loaded, "range_error_ratio", range_error_ratio_, positive_double, 0.015 );
    optional( jo, was_loaded, "deflection_error_mils", deflection_error_mils_, positive_double, 2.0 );
    was_loaded = true;
}

const furn_str_id &mortar_type::furniture() const
{
    return furniture_;
}

const ammotype &mortar_type::ammo() const
{
    return ammo_;
}

int mortar_type::range() const
{
    return range_;
}

time_duration mortar_type::player_flight_time( const int distance ) const
{
    return mortar_60mm_flight_time( distance );
}

time_duration mortar_type::npc_fire_message_delay() const
{
    return npc_fire_message_delay_;
}

time_duration mortar_type::npc_flight_time( const int distance ) const
{
    return mortar_60mm_flight_time( distance );
}

double mortar_type::minimum_range_error( const int distance ) const
{
    return std::max( 1.0, distance * range_error_ratio_ );
}

double mortar_type::minimum_deflection_error( const int distance ) const
{
    return std::max( 1.0, distance * deflection_error_mils_ / 1000.0 );
}

mortar_error mortar_type::minimum_error( const int distance ) const
{
    return mortar_error{ minimum_range_error( distance ), minimum_deflection_error( distance ) };
}

int mortar_type::minimum_target_distance( const int target_distance,
        const double ballistic_multiplier ) const
{
    const double range_error = minimum_range_error( target_distance ) * ballistic_multiplier;
    return std::min( 1000, MAX_VIEW_DISTANCE + static_cast<int>( std::ceil( range_error * 2.0 ) ) );
}

mortar_error mortar_type::combined_error( const tripoint_abs_ms &mortar_pos,
        const tripoint_abs_ms &target, const mortar_error &ballistic_error,
        const tripoint_abs_ms &location_axis_from,
        const tripoint_abs_ms &location_axis_to,
        const mortar_location_error &location_error ) const
{
    const mortar_error projected_location_error = project_location_error(
                mortar_pos, target, location_axis_from, location_axis_to, location_error );
    return mortar_error{ ballistic_error.range + projected_location_error.range,
                         ballistic_error.deflection + projected_location_error.deflection };
}

mortar_fire_solution mortar_type::make_fire_solution( const tripoint_abs_ms &mortar_pos,
        const tripoint_abs_ms &target, const tripoint_abs_ms &spotter_pos,
        const tripoint_abs_ms &creeping_axis_to,
        const tripoint_abs_ms &location_axis_from,
        const tripoint_abs_ms &location_axis_to,
        const mortar_location_error &location_error, const double total_multiplier,
        const bool round_is_high_explosive, const bool use_creeping_adjustment ) const
{
    mortar_fire_solution result;
    result.target_distance = rl_dist( mortar_pos, target );
    result.minimum_target_distance = round_is_high_explosive ?
                                     minimum_target_distance( result.target_distance, total_multiplier ) :
                                     MAX_VIEW_DISTANCE;
    result.minimum_error = minimum_error( result.target_distance );
    result.ballistic_error = mortar_error{ result.minimum_error.range * total_multiplier,
                                           result.minimum_error.deflection * total_multiplier };
    result.reported_error = combined_error( mortar_pos, target, result.ballistic_error,
                                            location_axis_from, location_axis_to,
                                            location_error );
    result.fire_center = target;
    if( use_creeping_adjustment ) {
        result.creeping_solution = mortar_creeping_adjustment( mortar_pos, target,
                                   creeping_axis_to, spotter_pos, result.reported_error );
        const tripoint_abs_ms unclamped_fire_center = result.creeping_solution->center;
        result.fire_center = clamp_fire_center_to_range( mortar_pos, unclamped_fire_center,
                             target, result.minimum_target_distance );
        if( result.fire_center != unclamped_fire_center ) {
            result.creeping_solution->center = result.fire_center;
            result.creeping_solution->offset_heading = mortar_heading_degrees( target,
                    result.fire_center );
            result.creeping_solution->range_limited = true;
        }
    }
    return result;
}

tripoint_abs_ms mortar_type::clamp_fire_center_to_range( const tripoint_abs_ms &mortar_pos,
        const tripoint_abs_ms &fire_center, const tripoint_abs_ms &fallback_axis_to,
        const int minimum_target_distance ) const
{
    const int current_distance = rl_dist( mortar_pos, fire_center );
    if( current_distance > minimum_target_distance && current_distance <= range_ ) {
        return fire_center;
    }

    const int minimum_valid_distance = std::min( range_,
                                       std::max( 0, minimum_target_distance + 1 ) );
    const int desired_distance = clamp( current_distance, minimum_valid_distance, range_ );
    int dx = fire_center.x() - mortar_pos.x();
    int dy = fire_center.y() - mortar_pos.y();
    int scale_distance = current_distance;
    if( dx == 0 && dy == 0 ) {
        dx = fallback_axis_to.x() - mortar_pos.x();
        dy = fallback_axis_to.y() - mortar_pos.y();
        scale_distance = rl_dist( mortar_pos, fallback_axis_to );
    }

    const auto project_to_distance = [&]( const int distance ) {
        if( dx == 0 && dy == 0 ) {
            return tripoint_abs_ms( mortar_pos.x() + distance, mortar_pos.y(), fire_center.z() );
        }
        const double scale = static_cast<double>( distance ) / std::max( 1, scale_distance );
        return tripoint_abs_ms( mortar_pos.x() + static_cast<int>( std::round( dx * scale ) ),
                                mortar_pos.y() + static_cast<int>( std::round( dy * scale ) ),
                                fire_center.z() );
    };

    const auto valid_distance = [&]( const tripoint_abs_ms & candidate ) {
        const int distance = rl_dist( mortar_pos, candidate );
        return distance > minimum_target_distance && distance <= range_;
    };

    tripoint_abs_ms clamped = project_to_distance( desired_distance );
    if( valid_distance( clamped ) ) {
        return clamped;
    }
    if( current_distance <= minimum_target_distance ) {
        for( int distance = desired_distance + 1; distance <= range_; ++distance ) {
            const tripoint_abs_ms candidate = project_to_distance( distance );
            if( valid_distance( candidate ) ) {
                return candidate;
            }
        }
    } else {
        for( int distance = desired_distance - 1; distance >= minimum_valid_distance; --distance ) {
            const tripoint_abs_ms candidate = project_to_distance( distance );
            if( valid_distance( candidate ) ) {
                return candidate;
            }
        }
    }
    return clamped;
}

double mortar_type::repeat_cep_multiplier( const int launcher_skill ) const
{
    const double skill = clamp<double>( launcher_skill, 1.0, 10.0 );
    if( skill <= 2.0 ) {
        return 0.7 + ( 2.0 - skill ) * 0.05;
    }
    if( skill <= 5.0 ) {
        return 0.7 - ( skill - 2.0 ) * ( 0.2 / 3.0 );
    }
    return 0.5 - ( skill - 5.0 ) * ( 0.2 / 5.0 );
}

tripoint_abs_ms mortar_type::apply_dispersion( const tripoint_abs_ms &target,
        const tripoint_abs_ms &axis_from, const tripoint_abs_ms &axis_to,
        const mortar_error &error, double *deflection_error ) const
{
    if( deflection_error != nullptr ) {
        *deflection_error = error.deflection;
    }
    const tripoint_abs_ms impact = apply_axis_dispersion(
                                       target, axis_from, axis_to,
                                       error.range / one_dimensional_probable_error_sigma_factor,
                                       error.deflection / one_dimensional_probable_error_sigma_factor );
    return clamp_to_max_range( axis_from, impact, range_ );
}

tripoint_abs_ms mortar_type::apply_location_error( const tripoint_abs_ms &target,
        const tripoint_abs_ms &axis_from, const tripoint_abs_ms &axis_to,
        const mortar_location_error &error ) const
{
    return apply_axis_dispersion( target, axis_from, axis_to,
                                  error.range / circular_cep_sigma_factor,
                                  error.deflection / circular_cep_sigma_factor );
}

mortar_error mortar_type::project_location_error( const tripoint_abs_ms &axis_from,
        const tripoint_abs_ms &axis_to, const tripoint_abs_ms &location_axis_from,
        const tripoint_abs_ms &location_axis_to,
        const mortar_location_error &location_error ) const
{
    const auto [ux, uy] = axis_unit( axis_from, axis_to );
    const auto [loc_ux, loc_uy] = axis_unit( location_axis_from, location_axis_to );
    const double cos_angle = std::abs( ux * loc_ux + uy * loc_uy );
    const double sin_angle = std::abs( -ux * loc_uy + uy * loc_ux );

    return mortar_error{
        std::hypot( location_error.range * cos_angle, location_error.deflection * sin_angle ),
        std::hypot( location_error.range * sin_angle, location_error.deflection * cos_angle )
    };
}

bool mortar_type::point_in_probable_impact_area( const tripoint_abs_ms &target,
        const tripoint_abs_ms &axis_from, const tripoint_abs_ms &axis_to,
        const mortar_error &error, const tripoint_abs_ms &location_axis_from,
        const tripoint_abs_ms &location_axis_to, const mortar_location_error &location_error,
        const tripoint_abs_ms &point, const double scale ) const
{
    const auto [ux, uy] = axis_unit( axis_from, axis_to );
    const mortar_error projected_location_error = project_location_error(
                axis_from, axis_to, location_axis_from, location_axis_to, location_error );

    const double dx = point.x() - target.x();
    const double dy = point.y() - target.y();
    const double range_offset = dx * ux + dy * uy;
    const double deflection_offset = -dx * uy + dy * ux;
    const double range_radius = std::max( 1.0,
                                          ( error.range + projected_location_error.range ) * scale );
    const double deflection_radius = std::max(
                                         1.0, ( error.deflection + projected_location_error.deflection ) * scale );
    const double normalized_range = range_offset / range_radius;
    const double normalized_deflection = deflection_offset / deflection_radius;
    return normalized_range * normalized_range + normalized_deflection * normalized_deflection <= 1.0;
}
