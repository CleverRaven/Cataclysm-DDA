#ifndef TILE_ID_DATA
#define TILE_ID_DATA

#define toString(x) #x
/*
*********************************************************************************
* Whenever the ter_id, trap_id, field_id, mon_id, furn_id, and/or vpart_id enum *
* values are changed a corresponding change MUST be made in here or the tileset *
* support will become offset (screws up later tiles), or if the id is added to  *
* the end it will just not be shown                                             *
*********************************************************************************
*/

const std::string field_names[num_fields] =
{
    toString(fd_null),
    toString(fd_blood),
    toString(fd_bile),
    toString(fd_gibs_flesh),
    toString(fd_gibs_veggy),
    toString(fd_web),
    toString(fd_slime),
    toString(fd_acid),
    toString(fd_sap),
    toString(fd_sludge),
    toString(fd_fire),
    toString(fd_rubble),
    toString(fd_smoke),
    toString(fd_toxic_gas),
    toString(fd_tear_gas),
    toString(fd_nuke_gas),
    toString(fd_gas_vent),
    toString(fd_fire_vent),
    toString(fd_flame_burst),
    toString(fd_electricity),
    toString(fd_fatigue),
    toString(fd_push_items),
    toString(fd_shock_vent),
    toString(fd_acid_vent),
    toString(fd_plasma),
    toString(fd_laser),
    toString( fd_blood_veggy ),
    toString( fd_blood_insect ),
    toString( fd_blood_invertebrate ),
    toString( fd_gibs_insect ),
    toString( fd_gibs_invertebrate ),
    toString(fd_cigsmoke),
    toString(fd_weedsmoke),
    toString(fd_cracksmoke),
    toString(fd_methsmoke),
    toString(fd_bees),
    toString(fd_incendiary)
};
const std::string multitile_keys[] =
{
    toString(center),
    toString(corner),
    toString(edge),
    toString(t_connection),
    toString(end_piece),
    toString(unconnected),
    toString(open),
    toString(broken)
};

#endif // TILE_ID_DATA
