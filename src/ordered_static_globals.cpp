#include "ordered_static_globals.h"

#include "ammo_effect.h"
#include "city.h"
#include "field_type.h"

void ordered_static_globals()
{
    get_city_factory();
    get_all_field_types();
    get_all_ammo_effects();
}
