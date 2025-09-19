#include "horde_entity.h"

#include "monster.h"
#include "mtype.h"

horde_entity::horde_entity( const monster &original )
{
    if( original.has_dest() ) {
        destination = original.get_dest();
        // TODO figure out a value? go here no matter what?
        tracking_intensity = 1000;
    } else if( original.wandf > 0 ) {
        destination = original.wander_pos;
        tracking_intensity = original.wandf;
    }
    moves = original.get_moves();
    type_id = original.type;
    monster_data = std::make_unique<monster>( original );
}

horde_entity::horde_entity( const mtype_id &original )
{
    type_id = &original.obj();
}

const mtype *horde_entity::get_type() const
{
    return type_id ? type_id : monster_data->type;
}

bool horde_entity::is_active() const
{
    return tracking_intensity > 0;
}
