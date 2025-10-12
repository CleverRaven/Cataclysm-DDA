#include "omdata.h" // IWYU pragma: associated
#include "overmap.h" // IWYU pragma: associated

#include <algorithm>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <vector>

#include "avatar.h"
#include "coordinates.h"
#include "debug.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "enum_conversions.h"
#include "generic_factory.h"
#include "mod_tracker.h"
#include "options.h"
#include "string_formatter.h"
#include "talker.h"

static const overmap_location_id overmap_location_land( "land" );
static const overmap_location_id overmap_location_swamp( "swamp" );

bool overmap::is_amongst_locations( const oter_id &oter,
                                    const cata::flat_set<string_id<overmap_location>> &locations )
{
    return std::any_of( locations.begin(), locations.end(),
    [&oter]( const string_id<overmap_location> &loc ) {
        return loc->test( oter );
    } );
}

namespace
{

generic_factory<overmap_special> specials( "overmap special" );
generic_factory<overmap_special_migration> migrations( "overmap special migration" );

} // namespace

template<>
const overmap_special &overmap_special_id::obj() const
{
    return specials.obj( *this );
}

template<>
bool overmap_special_id::is_valid() const
{
    return specials.is_valid( *this );
}

void overmap_specials::load( const JsonObject &jo, const std::string &src )
{
    specials.load( jo, src );
}

void overmap_specials::finalize()
{
    specials.finalize();
}

void overmap_specials::finalize_mapgen_parameters()
{
    for( const overmap_special &elem : specials.get_all() ) {
        // This cast is ugly, but safe.
        const_cast<overmap_special &>( elem ).finalize_mapgen_parameters();
    }
}

void overmap_specials::check_consistency()
{
    const size_t max_count = ( OMAPX / OMSPEC_FREQ ) * ( OMAPY / OMSPEC_FREQ );
    const size_t actual_count = std::accumulate( specials.get_all().begin(), specials.get_all().end(),
                                static_cast< size_t >( 0 ),
    []( size_t sum, const overmap_special & elem ) {
        size_t min_occur = static_cast<size_t>( std::max( elem.get_constraints().occurrences.min, 0 ) );
        const bool unique = elem.has_flag( "OVERMAP_UNIQUE" ) || elem.has_flag( "GLOBALLY_UNIQUE" );
        return sum + ( unique ? 0 : min_occur );
    } );

    if( actual_count > max_count ) {
        debugmsg( "There are too many mandatory overmap specials (%d > %d). Some of them may not be placed.",
                  actual_count, max_count );
    }

    overmap_special_migration::check();
    specials.check();

    for( const overmap_special &os : specials.get_all() ) {
        overmap_special_id new_id = overmap_special_migration::migrate( os.id );
        if( new_id.is_null() ) {
            debugmsg( "Overmap special id %s has been removed or migrated to a different type.",
                      os.id.str() );
        } else if( new_id != os.id ) {
            debugmsg( "Overmap special id %s has been migrated.  Use %s instead.", os.id.str(),
                      new_id.str() );
        }
        if( static_cast<int>( os.has_flag( "GLOBALLY_UNIQUE" ) ) +
            static_cast<int>( os.has_flag( "OVERMAP_UNIQUE" ) ) +
            static_cast<int>( os.has_flag( "CITY_UNIQUE" ) ) > 1 ) {
            debugmsg( "In special %s, the mutually exclusive flags GLOBALLY_UNIQUE, "
                      "OVERMAP_UNIQUE and CITY_UNIQUE cannot be used together.", os.id.str() );
        }
    }
}

void overmap_specials::reset()
{
    specials.reset();
}

const std::vector<overmap_special> &overmap_specials::get_all()
{
    return specials.get_all();
}

overmap_special_batch overmap_specials::get_default_batch( const point_abs_om &origin )
{
    std::vector<const overmap_special *> res;

    res.reserve( specials.size() );
    for( const overmap_special &elem : specials.get_all() ) {
        if( elem.can_spawn() ) {
            res.push_back( &elem );
        }
    }

    return overmap_special_batch( origin, res );
}

bool overmap_special_locations::can_be_placed_on( const oter_id &oter ) const
{
    return overmap::is_amongst_locations( oter, locations );
}

void overmap_special_locations::deserialize( const JsonArray &ja )
{
    if( ja.size() != 2 ) {
        ja.throw_error( "expected array of size 2" );
    }

    ja.read( 0, p, true );
    ja.read( 1, locations, true );
}

void overmap_special_terrain::deserialize( const JsonObject &om )
{
    om.read( "point", p );
    om.read( "overmap", terrain );
    om.read( "camp", camp_owner );
    om.read( "camp_name", camp_name );
    om.read( "flags", flags );
    om.read( "locations", locations );
}

overmap_special_terrain::overmap_special_terrain(
    const tripoint_rel_omt &p, const oter_str_id &t,
    const cata::flat_set<string_id<overmap_location>> &l,
    const std::set<std::string> &fs )
    : overmap_special_locations{ p, l }
    , terrain( t )
    , flags( fs )
{}


void overmap_special_connection::deserialize( const JsonObject &jo )
{
    optional( jo, false, "point", p );
    optional( jo, false, "terrain", terrain );
    optional( jo, false, "existing", existing, false );
    optional( jo, false, "connection", connection );
    optional( jo, false, "from", from );
}

overmap_special::overmap_special( const overmap_special_id &i, const overmap_special_terrain &ter )
    : id( i )
    , subtype_( overmap_special_subtype::fixed )
    , data_{ make_shared_fast<fixed_overmap_special_data>( ter ) }
{}

bool overmap_special::can_spawn() const
{
    if( get_constraints().occurrences.empty() ) {
        return false;
    }

    const int city_size = get_option<int>( "CITY_SIZE" );
    return city_size != 0 || get_constraints().city_size.min <= city_size;
}

bool overmap_special::requires_city() const
{
    return constraints_.city_size.min > 0 ||
           constraints_.city_distance.max < std::max( OMAPX, OMAPY );
}

bool overmap_special::can_belong_to_city( const tripoint_om_omt &p, const city &cit,
        const overmap &omap ) const
{
    if( !requires_city() ) {
        return true;
    }
    if( !cit || !constraints_.city_size.contains( cit.size ) ) {
        return false;
    }
    if( constraints_.city_distance.max > std::max( OMAPX, OMAPY ) ) {
        // Only care that we're more than min away from a city
        return !omap.approx_distance_to_city( p, constraints_.city_distance.min ).has_value();
    }
    const std::optional<int> dist = omap.approx_distance_to_city( p, constraints_.city_distance.max );
    // Found a city within max and it's greater than min away
    return dist.has_value() && constraints_.city_distance.min < *dist;
}

bool overmap_special::has_flag( const std::string &flag ) const
{
    return flags_.count( flag );
}

int overmap_special::longest_side() const
{
    // Figure out the longest side of the special for purposes of determining our sector size
    // when attempting placements.
    std::vector<overmap_special_locations> req_locations = required_locations();
    auto min_max_x = std::minmax_element( req_locations.begin(), req_locations.end(),
    []( const overmap_special_locations & lhs, const overmap_special_locations & rhs ) {
        return lhs.p.x() < rhs.p.x();
    } );

    auto min_max_y = std::minmax_element( req_locations.begin(), req_locations.end(),
    []( const overmap_special_locations & lhs, const overmap_special_locations & rhs ) {
        return lhs.p.y() < rhs.p.y();
    } );

    const int width = min_max_x.second->p.x() - min_max_x.first->p.x();
    const int height = min_max_y.second->p.y() - min_max_y.first->p.y();
    return std::max( width, height ) + 1;
}

std::vector<overmap_special_terrain> overmap_special::get_terrains() const
{
    return data_->get_terrains();
}

std::vector<overmap_special_terrain> overmap_special::preview_terrains() const
{
    return data_->preview_terrains();
}

std::vector<overmap_special_locations> overmap_special::required_locations() const
{
    return data_->required_locations();
}

int overmap_special::score_rotation_at( const overmap &om, const tripoint_om_omt &p,
                                        om_direction::type r ) const
{
    return data_->score_rotation_at( om, p, r );
}

special_placement_result overmap_special::place(
    overmap &om, const tripoint_om_omt &origin, om_direction::type dir,
    const city &cit, bool must_be_unexplored ) const
{
    if( has_eoc() ) {
        dialogue d( get_talker_for( get_avatar() ), nullptr );
        get_eoc()->apply_true_effects( d );
    }
    const bool blob = has_flag( "BLOB" );
    return data_->place( om, origin, dir, blob, cit, must_be_unexplored );
}

void overmap_special::force_one_occurrence()
{
    constraints_.occurrences.min = 1;
    constraints_.occurrences.max = 1;
}

mapgen_arguments overmap_special::get_args( const mapgendata &md ) const
{
    return mapgen_params_.get_args( md, mapgen_parameter_scope::overmap_special );
}

void overmap_special::load( const JsonObject &jo, const std::string_view src )
{
    // city_building is just an alias of overmap_special
    // TODO: This comparison is a hack. Separate them properly.
    const bool is_special = jo.get_string( "type", "" ) == "overmap_special";

    optional( jo, was_loaded, "subtype", subtype_, overmap_special_subtype::fixed );
    optional( jo, was_loaded, "locations", default_locations_ );
    // FIXME: eoc reader for generic factory
    if( jo.has_member( "eoc" ) ) {
        eoc = effect_on_conditions::load_inline_eoc( jo.get_member( "eoc" ), src );
        has_eoc_ = true;
    }
    switch( subtype_ ) {
        case overmap_special_subtype::fixed: {
            shared_ptr_fast<fixed_overmap_special_data> fixed_data =
                make_shared_fast<fixed_overmap_special_data>();
            optional( jo, was_loaded, "overmaps", fixed_data->terrains );
            if( is_special ) {
                optional( jo, was_loaded, "connections", fixed_data->connections );
            }
            data_ = std::move( fixed_data );
            break;
        }
        case overmap_special_subtype::mutable_: {
            shared_ptr_fast<mutable_overmap_special_data> mutable_data =
                make_shared_fast<mutable_overmap_special_data>( id );
            mutable_data->load( jo, was_loaded );
            data_ = std::move( mutable_data );
            break;
        }
        default:
            jo.throw_error( string_format( "subtype %s not implemented",
                                           io::enum_to_string( subtype_ ) ) );
    }

    optional( jo, was_loaded, "city_sizes", constraints_.city_size, { 0, INT_MAX } );

    if( is_special ) {
        mandatory( jo, was_loaded, "occurrences", constraints_.occurrences );
        optional( jo, was_loaded, "city_distance", constraints_.city_distance, { 0, INT_MAX } );
        optional( jo, was_loaded, "priority", priority_, 0 );
    }

    optional( jo, was_loaded, "spawns", monster_spawns_ );

    optional( jo, was_loaded, "rotate", rotatable_, true );
    optional( jo, was_loaded, "flags", flags_, string_reader{} );
}

void overmap_special::finalize()
{
    const_cast<overmap_special_data &>( *data_ ).finalize(
        "overmap special " + id.str(), default_locations_ );
}

void overmap_special::finalize_mapgen_parameters()
{
    // Extract all the map_special-scoped params from the constituent terrains
    // and put them here
    std::string context = string_format( "overmap_special %s", id.str() );
    data_->finalize_mapgen_parameters( mapgen_params_, context );
}

void overmap_special::check() const
{
    data_->check( string_format( "overmap special %s", id.str() ) );
}

overmap_special_id overmap_specials::create_building_from( const string_id<oter_type_t> &base )
{
    // TODO: Get rid of the hard-coded ids.
    overmap_special_terrain ter;
    ter.terrain = base.obj().get_first().id();
    ter.locations.insert( overmap_location_land );
    ter.locations.insert( overmap_location_swamp );

    overmap_special_id new_id( "FakeSpecial_" + base.str() );
    overmap_special new_special( new_id, ter );
    mod_tracker::assign_src( new_special, base->src.back().second.str() );

    return specials.insert( new_special ).id;
}

void overmap_special_migration::load_migrations( const JsonObject &jo, const std::string &src )
{
    migrations.load( jo, src );
}

void overmap_special_migration::finalize_all()
{
    migrations.finalize();
}

void overmap_special_migration::reset()
{
    migrations.reset();
}

void overmap_special_migration::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "id", id );
    optional( jo, was_loaded, "new_id", new_id, overmap_special_id() );
}

void overmap_special_migration::check()
{
    for( const overmap_special_migration &mig : migrations.get_all() ) {
        if( !mig.new_id.is_null() && !mig.new_id.is_valid() ) {
            debugmsg( "Invalid new_id \"%s\" for overmap special migration \"%s\"", mig.new_id.c_str(),
                      mig.id.c_str() );
        }
    }
}

bool overmap_special_migration::migrated( const overmap_special_id &os_id )
{
    std::vector<overmap_special_migration> migs = migrations.get_all();
    return std::find_if( migs.begin(), migs.end(), [&os_id]( overmap_special_migration & m ) {
        return os_id == overmap_special_id( m.id.str() );
    } ) != migs.end();
}

overmap_special_id overmap_special_migration::migrate( const overmap_special_id &old_id )
{
    for( const overmap_special_migration &mig : migrations.get_all() ) {
        if( overmap_special_id( mig.id.str() ) == old_id ) {
            return mig.new_id;
        }
    }
    return old_id;
}
