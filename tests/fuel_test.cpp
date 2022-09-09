#include "cata_catch.h"
#include "item.h"



TEST_CASE( "Fuel energy", "[energy]" )
{
    item battery( "battery" );
    item gasoline( "gasoline" );

    const int64_t gasoline_per_charge = units::to_millijoule( 34200_J );
    const int64_t battery_per_charge = units::to_millijoule( 1_kJ );

    SECTION( "Energy of 1 unit" ) {
        battery.charges = 1;
        gasoline.charges = 1;

        CHECK( units::to_millijoule( gasoline.fuel_energy() ) == gasoline_per_charge );
        CHECK( units::to_millijoule( battery.fuel_energy() ) == battery_per_charge );
    }

    SECTION( "Energy of 200 units" ) {
        battery.charges = 200;
        gasoline.charges = 200;

        CHECK( units::to_millijoule( gasoline.fuel_energy() ) == gasoline_per_charge * 200 ); // 6840 kJ
        CHECK( units::to_millijoule( battery.fuel_energy() ) == battery_per_charge * 200 ); // 200 kJ
    }

    SECTION( "Energy of gasoline liter" ) {
        gasoline.charges = 1000;

        REQUIRE( units::to_milliliter( gasoline.volume() ) == 1000 );
        // 34200 kJ
        // Fuel data has energy per liter so it should match here.
        CHECK( units::to_millijoule( gasoline.fuel_energy() )
               == units::to_millijoule( gasoline.get_base_material().get_fuel_data().energy ) );
    }
}
