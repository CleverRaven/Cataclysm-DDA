#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "item_factory.h"
#include "itype.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

static const ammotype ammo_battery( "battery" );
static const flag_id json_flag_DEBUG_ONLY( "DEBUG_ONLY" );

struct battery_chemistry_family {
    float max_energy_density_kj_kg;
    float max_energy_density_kj_l;
};

// NOLINTBEGIN(cata-static-string_id-constants)
static const std::map<itype_id, std::string> battery_to_chemistry = {
    {itype_id( "light_minus_battery_cell" ), "LiON"},
    {itype_id( "medium_battery_cell" ), "LiON"},
    {itype_id( "heavy_battery_cell" ), "LiON"},
    {itype_id( "heavy_plus_battery_cell" ), "LiON"},
    {itype_id( "folding_solar_panel_deployed" ), "LiON"},
    {itype_id( "folding_solar_panel_v2_deployed" ), "LiON"},

    {itype_id( "light_cell_rechargeable" ), "Nickel-Metal Hydride"},

    {itype_id( "light_battery_cell" ), "Lithium/Iron Disulfide"},

    {itype_id( "heavy_atomic_battery_cell" ), "atomic"},
    {itype_id( "huge_atomic_battery_cell" ), "atomic"},

    {itype_id( "light_minus_disposable_cell" ), "Lithium-Manganese"},

    {itype_id( "battery_car" ), "Lead-acid"},
    {itype_id( "battery_motorbike" ), "Lead-acid"},
    {itype_id( "battery_motorbike_small" ), "Lead-acid"},

    // These are in a seperate group because the car batteries might
    // end up being switched to something different.
    {itype_id( "large_storage_battery" ), "LiON"},
    {itype_id( "medium_storage_battery" ), "LiON"},
    {itype_id( "small_storage_battery" ), "LiON"},
    {itype_id( "storage_battery" ), "LiON"}
};
// NOLINTEND(cata-static-string_id-constants)


// LiON or LiPO is tough to actually nail down because there are so many different chemistries and everything is flooded with marketing.
// https://en.wikipedia.org/wiki/Energy_density_Extended_Reference_Table
// At the end of the day I compared with several actual 18650 cells which ended up agreeing with the table at Energy_density_Extended_Reference_Table instead of some of the more extreme values I found elsewhere.
// https://www.batteryjunction.com/batteries/li-ion-rechargeable/samsung-inr18650-25r

// There is potential to add hyper-premium batteries for e.g. laptops and small high-powered electric devices based on these figures.
// https://en.wikipedia.org/wiki/Lithium-ion_battery
// Specific energy  100–265 W⋅h/kg (360–950 kJ/kg)
// Energy density   250–693 W⋅h/L (900–2,490 J/cm3)

// Atomic cell energy density is based on theoretical solid state battery density from
// https://en.wikipedia.org/wiki/Solid-state_battery
// i.e. "Thin film type: 300–900 Wh/kg (490–1,470 kJ/lb) Bulk type: 250–500 Wh/kg (410–820 kJ/lb)"

// Lithium–Manganese, which I'm using for "consumer-grade disposable batteries" sourced from
// https://en.wikipedia.org/wiki/Energy_density_Extended_Reference_Table

// Zinc-manganese (alkaline), which I'm using for "consumer grade disposable battery" based on
// https://en.wikipedia.org/wiki/Energy_density_Extended_Reference_Table

// Zinc-Carbon, which I'm using for "player craftable disposable battery" based on
// https://en.wikipedia.org/wiki/Energy_density_Extended_Reference_Table

// Nickel-Metal Hydride
// https://en.wikipedia.org/wiki/Energy_density_Extended_Reference_Table

// Zinc-Manganese Dioxide
// https://en.wikipedia.org/wiki/Energy_density_Extended_Reference_Table

// Lithium/Iron Disulfide gravimetric energy density is picked from enerizer documentation
// https://data.energizer.com/pdfs/lithiuml91l92_appman.pdf
// Volumetric is a lower bound from this article (560 Wh/L), rounded up to 600
// https://www.sciencedirect.com/science/article/abs/pii/S2590116819300104

// Lead-acid specific Energy and specific density calculated from a typical exemplar at
// https://www.amazon.com/ACDelco-48AGM-Professional-Automotive-Battery/dp/B008FWCLHU/
// With primary statistics of:
// dimensions: 7.5"D x 11.9"W x 7.6"H = Volume 11.1153455 liters
// Weight: 45.5 pounds = 20.63845 kg
// Capacity: 70 Amp-Hours x 12V = 3024 kJ
// This also roughly agrees with https://en.wikipedia.org/wiki/Energy_density_Extended_Reference_Table
static const std::map<std::string, battery_chemistry_family> chemistries = {
    {std::string( "LiON" ), { 740.0, 3330.0 }},
    {std::string( "atomic" ), { 3000.0, 5000.0 }},
    {std::string( "Lithium-Manganese" ), { 1010.0, 2090.0}},
    {std::string( "Alkaline" ), { 600.0, 1430.0}},
    {std::string( "Zinc-Carbon" ), { 130.0, 331.0}},
    {std::string( "Nickel-Metal Hydride" ), { 400.0, 1550.0}},
    {std::string( "Zinc-Manganese Dioxide" ), { 590.0, 1430.0}},
    {std::string( "Lithium/Iron Disulfide" ), { 1069.0, 2160.0}},
    {std::string( "Lead-acid" ), { 180, 360 }}
};

static bool is_battery( const itype &type )
{
    // We're only making assertions about the "dda" mod,
    // ignore items from elsewhere, including the test mod.
    if( type.has_flag( json_flag_DEBUG_ONLY ) ||
        type.src.size() > 1 ||
        ( type.src.size() == 1 && type.src.back().second.str() != std::string( "dda" ) ) ) {
        return false;
    }
    if( !!type.magazine && type.magazine->type.count( ammo_battery ) > 0 ) {
        return true;
    }
    return false;
}

TEST_CASE( "battery_density_check" )
{
    for( const itype *id : item_controller->find( is_battery ) ) {
        auto chemistry_name = battery_to_chemistry.find( id->get_id() );
        battery_chemistry_family chemistry;
        if( chemistry_name == battery_to_chemistry.end() ) {
            WARN( "Didn't find a battery chemistry for " << id->get_id().str().c_str() <<
                  ", assuming Alkaline." );
            chemistry = chemistries.at( "Alkaline" );
        } else {
            chemistry = chemistries.at( chemistry_name->second );
        }
        float capacity_in_kj = id->magazine ? id->magazine->capacity : id->tool->max_charges;
        float weight_in_kg = units::to_kilogram( id->weight );
        float volume_in_L = units::to_liter( id->volume );
        INFO( id->get_id().str().c_str() );
        CAPTURE( capacity_in_kj );
        CAPTURE( weight_in_kg );
        CAPTURE( volume_in_L );
        INFO( "Capacity should be: " << std::min( chemistry.max_energy_density_kj_kg * weight_in_kg,
                chemistry.max_energy_density_kj_l * volume_in_L ) << " kJ" );
        CHECK( capacity_in_kj / weight_in_kg <= chemistry.max_energy_density_kj_kg );
        CHECK( capacity_in_kj / volume_in_L <= chemistry.max_energy_density_kj_l );
    }
}
