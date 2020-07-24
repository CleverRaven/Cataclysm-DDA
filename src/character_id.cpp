#include "character_id.h"

#include <ostream>

std::ostream &operator<<( std::ostream &o, character_id id )
{
    return o << id.get_value();
}
