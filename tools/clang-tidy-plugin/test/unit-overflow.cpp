// RUN: %check_clang_tidy -allow-stdinc %s cata-unit-overflow %t -- --load=%cata_plugin -- -isystem %cata_include -isystem %cata_include/third-party

#include "units.h"

units::energy f_energy_0()
{
    return units::from_millijoule( 3'000 );
}

units::energy f_energy_1()
{
    return units::from_joule( 3'000 );
}

units::energy f_energy_2()
{
    return units::from_joule( 3'000'000 );
    // CHECK-MESSAGES: warning: constructing energy quantity from 'int' can overflow in 'from_joule' in multiplying with the conversion factor [cata-unit-overflow]
    // CHECK-FIXES: return units::from_joule( 3'000'000LL );
}

units::energy f_energy_3()
{
    int joule = 3'000'000;
    return units::from_joule( joule );
    // CHECK-MESSAGES: warning: constructing energy quantity from 'int' can overflow in 'from_joule' in multiplying with the conversion factor [cata-unit-overflow]
    // CHECK-FIXES: return units::from_joule( static_cast<std::int64_t>( joule ) );
}

units::energy f_energy_4()
{
    return units::from_kilojoule( 2'000 );
}

units::energy f_energy_5()
{
    return units::from_kilojoule( 3'000 );
    // CHECK-MESSAGES: warning: constructing energy quantity from 'int' can overflow in 'from_kilojoule' in multiplying with the conversion factor [cata-unit-overflow]
    // CHECK-FIXES: return units::from_kilojoule( 3'000LL );
}

units::energy f_energy_6()
{
    int kj = 3'000;
    return units::from_kilojoule( kj );
    // CHECK-MESSAGES: warning: constructing energy quantity from 'int' can overflow in 'from_kilojoule' in multiplying with the conversion factor [cata-unit-overflow]
    // CHECK-FIXES: return units::from_kilojoule( static_cast<std::int64_t>( kj ) );
}
