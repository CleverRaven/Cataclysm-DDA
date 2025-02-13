#include "condition.h"
#include "end_screen.h"
#include "generic_factory.h"

namespace
{
generic_factory<end_screen> end_screen_factory( "end_screen" );
} // namespace

template<>
const end_screen &string_id<end_screen>::obj()const
{
    return end_screen_factory.obj( *this );
}

template<>
bool string_id<end_screen>::is_valid() const
{
    return end_screen_factory.is_valid( *this );
}

void end_screen::load_end_screen( const JsonObject &jo, const std::string &src )
{
    end_screen_factory.load( jo, src );
}

void end_screen::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "picture_id", picture_id );
    mandatory( jo, was_loaded, "priority", priority );
    read_condition( jo, "condition", condition, false );

    optional( jo, was_loaded, "added_info", added_info );
    optional( jo, was_loaded, "last_words_label", last_words_label );
}

const std::vector<end_screen> &end_screen::get_all()
{
    return end_screen_factory.get_all();
}
