#include "assign.h"
#include "condition.h"
#include "death_screen.h"
#include "generic_factory.h"

namespace
{
generic_factory<death_screen> death_screen_factory( "death_screen" );
} // namespace

template<>
const death_screen &string_id<death_screen>::obj()const
{
    return death_screen_factory.obj( *this );
}

template<>
bool string_id<death_screen>::is_valid() const
{
    return death_screen_factory.is_valid( *this );
}

void death_screen::load_death_screen( const JsonObject &jo, const std::string &src )
{
    death_screen_factory.load( jo, src );
}

void death_screen::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "picture_id", picture_id );
    read_condition( jo, "condition", condition, false );
}

const std::vector<death_screen> &death_screen::get_all()
{
    return death_screen_factory.get_all();
}
