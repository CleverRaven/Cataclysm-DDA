#include "butchery_requirements.h"

#include <cstddef>
#include <functional>
#include <string>

#include "activity_handlers.h"
#include "creature.h"
#include "debug.h"
#include "enum_conversions.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "item.h"
#include "requirements.h"

namespace
{
generic_factory<butchery_requirements> butchery_req_factory( "butchery_requirements" );
} // namespace

template<>
const butchery_requirements &string_id<butchery_requirements>::obj() const
{
    return butchery_req_factory.obj( *this );
}

template<>
bool string_id<butchery_requirements>::is_valid() const
{
    return butchery_req_factory.is_valid( *this );
}

void butchery_requirements::load_butchery_req( const JsonObject &jo, const std::string &src )
{
    butchery_req_factory.load( jo, src );
}

const std::vector<butchery_requirements> &butchery_requirements::get_all()
{
    return butchery_req_factory.get_all();
}

void butchery_requirements::reset()
{
    butchery_req_factory.reset();
}

bool butchery_requirements::is_valid() const
{
    return butchery_req_factory.is_valid( this->id );
}

void butchery_requirements::load( const JsonObject &jo, std::string_view )
{
    for( const JsonMember member : jo.get_object( "requirements" ) ) {
        float modifier = std::stof( member.name() );
        requirements.emplace( modifier, std::map<creature_size, std::map<butcher_type, requirement_id>> {} );

        int critter_size = 1;
        for( JsonObject butcher_jo : member.get_array() ) {
            if( critter_size == creature_size::num_sizes ) {
                debugmsg( "ERROR: %s has too many creature sizes.  must have exactly %d",
                          id.c_str(), creature_size::num_sizes - 1 );
                break;
            }
            requirements[modifier].emplace( static_cast<creature_size>( critter_size ),
                                            std::map<butcher_type, requirement_id> {} );

            for( int i_butchery_type = 0; i_butchery_type < static_cast<int>( butcher_type::NUM_TYPES );
                 i_butchery_type++ ) {
                butcher_type converted = static_cast<butcher_type>( i_butchery_type );
                requirements[modifier][static_cast<creature_size>( critter_size )][converted] =
                    requirement_id( butcher_jo.get_string( io::enum_to_string( converted ) ) );
            }

            critter_size++;
        }
    }
}

void butchery_requirements::check_consistency()
{
    for( const butchery_requirements &req : get_all() ) {
        for( const auto &size_req : req.requirements ) {
            if( size_req.second.size() != static_cast<size_t>( creature_size::num_sizes - 1 ) ) {
                debugmsg( "ERROR: %s needs exactly %d entries to cover all creature sizes",
                          req.id.c_str(), static_cast<int>( creature_size::num_sizes ) - 1 );
            }
            for( const auto &butcher_req : size_req.second ) {
                if( butcher_req.second.size() != static_cast<size_t>( butcher_type::NUM_TYPES ) ) {
                    debugmsg( "ERROR: %s needs exactly %d entries to cover all butchery types",
                              req.id.c_str(), static_cast<int>( butcher_type::NUM_TYPES ) );
                }
            }
        }
    }
}

std::pair<float, requirement_id> butchery_requirements::get_fastest_requirements(
    const read_only_visitable &crafting_inv, creature_size size, butcher_type butcher ) const
{
    for( const std::pair<const float, std::map<creature_size, std::map<butcher_type, requirement_id>>>
         &riter : requirements ) {
        if( riter.second.at( size ).at( butcher )->can_make_with_inventory( crafting_inv,
                is_crafting_component ) ) {
            return std::make_pair( riter.first, riter.second.at( size ).at( butcher ) );
        }
    }
    // we didn't find anything we could "craft", so return the requirement that's the fastest
    const auto first = requirements.rbegin();
    return std::make_pair( first->first, first->second.at( size ).at( butcher ) );
}
