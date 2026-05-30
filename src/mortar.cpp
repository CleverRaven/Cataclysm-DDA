#include "mortar.h"

#include <algorithm>
#include <cmath>
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

static constexpr double circular_cep_sigma_factor = 1.1774;
static constexpr double one_dimensional_probable_error_sigma_factor = 0.67448975;
static constexpr int mortar_minimum_launcher_skill = 4;
static constexpr double mortar_min_skill_error_multiplier = 3.0;
static constexpr double mortar_multiplier_soft_cap_threshold = 10.0;
static constexpr double mortar_multiplier_hard_cap = 70.0;
static constexpr double mortar_multiplier_above_soft_cap_scale = 0.5;

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
        if( mortar.cep_baseline_ <= 0.0 || mortar.cep_min_floor_ <= 0.0 ||
            mortar.cep_min_base_ <= 0.0 || mortar.range_error_ratio_ <= 0.0 ||
            mortar.deflection_error_mils_ <= 0.0 ) {
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

void mortar_type::load( const JsonObject &jo, std::string_view )
{
    const numeric_bound_reader<int> positive_int{ 1 };
    const numeric_bound_reader<double> positive_double{ std::numeric_limits<double>::min() };
    const numeric_bound_reader<double> non_negative_double{ 0.0 };

    mandatory( jo, was_loaded, "furniture", furniture_ );
    mandatory( jo, was_loaded, "ammo", ammo_ );
    mandatory( jo, was_loaded, "range", range_, positive_int );
    optional( jo, was_loaded, "player_flight_time", player_flight_time_, 0_seconds );
    optional( jo, was_loaded, "npc_fire_message_delay", npc_fire_message_delay_, 0_seconds );
    optional( jo, was_loaded, "npc_impact_delay", npc_impact_delay_, 0_seconds );
    optional( jo, was_loaded, "npc_impact_message_delay", npc_impact_message_delay_,
              npc_impact_delay_ );
    optional( jo, was_loaded, "cep_baseline", cep_baseline_, positive_double, 100.0 );
    optional( jo, was_loaded, "cep_min_base", cep_min_base_, positive_double, 20.0 );
    optional( jo, was_loaded, "cep_min_skill_scale", cep_min_skill_scale_, non_negative_double, 1.0 );
    optional( jo, was_loaded, "cep_min_floor", cep_min_floor_, positive_double, 5.0 );
    optional( jo, was_loaded, "axis_ratio_baseline", axis_ratio_baseline_, positive_double, 4.0 );
    optional( jo, was_loaded, "axis_ratio_final_base", axis_ratio_final_base_, positive_double, 2.5 );
    optional( jo, was_loaded, "axis_ratio_skill_scale", axis_ratio_skill_scale_,
              non_negative_double, 0.1 );
    optional( jo, was_loaded, "axis_ratio_floor", axis_ratio_floor_, positive_double, 1.2 );
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

time_duration mortar_type::player_flight_time() const
{
    return player_flight_time_;
}

time_duration mortar_type::npc_fire_message_delay() const
{
    return npc_fire_message_delay_;
}

time_duration mortar_type::npc_impact_delay() const
{
    return npc_impact_delay_;
}

time_duration mortar_type::npc_impact_message_delay() const
{
    return npc_impact_message_delay_;
}

double mortar_type::baseline_cep() const
{
    return cep_baseline_;
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

double mortar_type::minimum_cep( const int launcher_skill ) const
{
    return std::max( cep_min_floor_, cep_min_base_ - launcher_skill * cep_min_skill_scale_ );
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

double mortar_type::player_cep( const double aim_deviation, const int distance,
                                const int launcher_skill ) const
{
    return clamp( aim_deviation * distance, minimum_cep( launcher_skill ), baseline_cep() );
}

double mortar_type::axis_ratio( const double cep, const int launcher_skill ) const
{
    const double minimum = minimum_cep( launcher_skill );
    const double final_ratio = std::max( axis_ratio_floor_,
                                         axis_ratio_final_base_ - launcher_skill *
                                         axis_ratio_skill_scale_ );
    const double span = std::max( 1.0, baseline_cep() - minimum );
    const double progress = clamp( ( cep - minimum ) / span, 0.0, 1.0 );
    return final_ratio + ( axis_ratio_baseline_ - final_ratio ) * progress;
}

tripoint_abs_ms mortar_type::apply_dispersion( const tripoint_abs_ms &target,
        const tripoint_abs_ms &axis_from, const tripoint_abs_ms &axis_to, const double cep,
        const int launcher_skill, double *minor_cep ) const
{
    const double ratio = axis_ratio( cep, launcher_skill );
    const double minor = cep / ratio;
    if( minor_cep != nullptr ) {
        *minor_cep = minor;
    }
    return apply_axis_dispersion( target, axis_from, axis_to, cep / circular_cep_sigma_factor,
                                  minor / circular_cep_sigma_factor );
}

tripoint_abs_ms mortar_type::apply_dispersion( const tripoint_abs_ms &target,
        const tripoint_abs_ms &axis_from, const tripoint_abs_ms &axis_to,
        const mortar_error &error, double *deflection_error ) const
{
    if( deflection_error != nullptr ) {
        *deflection_error = error.deflection;
    }
    return apply_axis_dispersion( target, axis_from, axis_to,
                                  error.range / one_dimensional_probable_error_sigma_factor,
                                  error.deflection / one_dimensional_probable_error_sigma_factor );
}

tripoint_abs_ms mortar_type::apply_circular_error( const tripoint_abs_ms &target,
        const double cep ) const
{
    const double sigma = cep / circular_cep_sigma_factor;
    return tripoint_abs_ms( target.x() + static_cast<int>( std::round( normal_roll( 0.0, sigma ) ) ),
                            target.y() + static_cast<int>( std::round( normal_roll( 0.0, sigma ) ) ),
                            target.z() );
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
