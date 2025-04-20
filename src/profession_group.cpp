#include "profession_group.h"

#include "assign.h"
#include "debug.h"
#include "generic_factory.h"

class JsonObject;

namespace
{
generic_factory<profession_group> profession_group_factory( "profession_group" );
} // namespace

template<>
const profession_group &string_id<profession_group>::obj() const
{
    return profession_group_factory.obj( *this );
}

template<>
bool string_id<profession_group>::is_valid() const
{
    return profession_group_factory.is_valid( *this );
}

void profession_group::load_profession_group( const JsonObject &jo, const std::string &src )
{
    profession_group_factory.load( jo, src );
}

void profession_group::load( const JsonObject &jo, std::string_view )
{
    assign( jo, "id", id );
    assign( jo, "professions", profession_list );

}

const std::vector<profession_group> &profession_group::get_all()
{
    return profession_group_factory.get_all();
}

void profession_group::check_profession_group_consistency()
{
    for( const profession_group &prof_grp : get_all() ) {
        for( const profession_id prof : prof_grp.profession_list ) {
            if( !prof.is_valid() ) {
                debugmsg( "profession_group %s contains invalid profession_id %s", prof_grp.id.c_str(),
                          prof.c_str() );
            }
        }
    }
}

std::vector<profession_id> profession_group::get_professions() const
{
    return profession_list;
}

profession_group_id profession_group::get_id() const
{
    return id;
}
