#include "character_mutations.h"

character_mutations::trait_data &character_mutations::get_trait_data( const trait_id &mut )
{
    return my_mutations[mut];
}

character_mutations::trait_data character_mutations::get_trait_data( const trait_id &mut ) const
{
    const auto iter = my_mutations.find( mut );
    if( iter == my_mutations.cend() ) {
        return trait_data();
    } else {
        return iter->second;
    }
}
