#pragma once
#ifndef CATA_SRC_UNITS_FWD_H
#define CATA_SRC_UNITS_FWD_H

#include <cstdint>

namespace units
{

template<typename V, typename U>
class quantity;

class volume_in_milliliter_tag
{
};

using volume = quantity<int, volume_in_milliliter_tag>;

class mass_in_milligram_tag
{
};

using mass = quantity<std::int64_t, mass_in_milligram_tag>;

class energy_in_millijoule_tag
{
};

using energy = quantity<int, energy_in_millijoule_tag>;

class money_in_cent_tag
{
};

using money = quantity<int, money_in_cent_tag>;

class length_in_millimeter_tag
{
};

using length = quantity<int, length_in_millimeter_tag>;

class angle_in_radians_tag
{
};

using angle = quantity<double, angle_in_radians_tag>;

} // namespace units

#endif // CATA_SRC_UNITS_FWD_H
