#include "character_mutations.h"

character_mutations::trait_data &character_mutations::get_trait_data( const trait_id &mut ) const
{
    return &my_mutations[mut];
}
