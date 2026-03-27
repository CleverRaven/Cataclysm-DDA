#include "vehicle_part_location.h"

#include "generic_factory.h"

class JsonObject;

void vpart_location::load( const JsonObject &jo, const std::string_view & )
{
    mandatory( jo, was_loaded, "name", name );

    optional( jo, was_loaded, "desc", description );
    optional( jo, was_loaded, "z_order", z_order, z_order );
    optional( jo, was_loaded, "list_order", list_order, list_order );
}

namespace
{

generic_factory<vpart_location> vehicle_part_location_factory( "vehicle_part_location" );

} // namespace

/** @relates string_id */
template<>
const vpart_location &string_id<vpart_location>::obj() const
{
    return vehicle_part_location_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<vpart_location>::is_valid() const
{
    return vehicle_part_location_factory.is_valid( *this );
}

void vpart_location::load_vehicle_part_locations( const JsonObject &jo, const std::string &src )
{
    vehicle_part_location_factory.load( jo, src );
}

void vpart_location::reset()
{
    vehicle_part_location_factory.reset();
}

void vpart_location::finalize_all()
{
    vehicle_part_location_factory.finalize();
}

void vpart_location::check_all()
{
    vehicle_part_location_factory.check();
}

void vpart_location::check() const
{
}
