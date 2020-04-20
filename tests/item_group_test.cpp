#include "catch/catch.hpp"
#include "item.h"
#include "item_group.h"

TEST_CASE( "spawn with default charges and with ammo", "[item_group]" )
{
    Item_modifier default_charges;
    default_charges.with_ammo = 100;
    SECTION( "tools without ammo" ) {
        item matches( "matches" );
        REQUIRE( matches.ammo_default() == "NULL" );
        default_charges.modify( matches );
        CHECK( matches.ammo_remaining() == matches.ammo_capacity() );
    }

    SECTION( "gun with ammo type" ) {
        item glock( "glock_19" );
        REQUIRE( glock.ammo_default() != "NULL" );
        default_charges.modify( glock );
        CHECK( glock.ammo_remaining() == glock.ammo_capacity() );
    }
}

TEST_CASE( "Item_modifier damages item", "[item_group]" )
{
    Item_modifier damaged;
    damaged.damage.first = 1;
    damaged.damage.second = 1;
    SECTION( "except when it's an ammunition" ) {
        item rock( "rock" );
        REQUIRE( rock.damage() == 0 );
        REQUIRE( rock.max_damage() == 0 );
        damaged.modify( rock );
        CHECK( rock.damage() == 0 );
    }
    SECTION( "when it can be damaged" ) {
        item glock( "glock_19" );
        REQUIRE( glock.damage() == 0 );
        REQUIRE( glock.max_damage() > 0 );
        damaged.modify( glock );
        CHECK( glock.damage() == 1 );
    }
}

TEST_CASE( "Item_modifier gun fouling", "[item_group]" )
{
    Item_modifier fouled;
    fouled.dirt.first = 1;
    SECTION( "guns can be fouled" ) {
        item glock( "glock_19" );
        REQUIRE( !glock.has_flag( "PRIMITIVE_RANGED_WEAPON" ) );
        REQUIRE( !glock.has_var( "dirt" ) );
        fouled.modify( glock );
        CHECK( glock.get_var( "dirt", 0.0 ) > 0.0 );
    }
    SECTION( "bows can't be fouled" ) {
        item bow( "longbow" );
        REQUIRE( !bow.has_var( "dirt" ) );
        REQUIRE( bow.has_flag( "PRIMITIVE_RANGED_WEAPON" ) );
        fouled.modify( bow );
        CHECK( !bow.has_var( "dirt" ) );
    }
}

TEST_CASE( "item_modifier modifies charges for item", "[item_group]" )
{
    GIVEN( "an ammo item that uses charges" ) {
        const std::string item_id = "40x46mm_m1006";
        item subject( item_id );

        const int default_charges = 6;

        REQUIRE( subject.is_ammo() );
        REQUIRE( subject.count_by_charges() );
        REQUIRE( subject.count() == default_charges );
        REQUIRE( subject.charges == default_charges );

        AND_GIVEN( "a modifier that does not modify charges" ) {
            Item_modifier modifier;

            WHEN( "the item is modified" ) {
                modifier.modify( subject );

                THEN( "charges should be unchanged" ) {
                    CHECK( subject.charges == default_charges );
                }
            }
        }

        AND_GIVEN( "a modifier that sets min and max charges to 0" ) {
            const int min_charges = 0;
            const int max_charges = 0;
            Item_modifier modifier;
            modifier.charges = { min_charges, max_charges };

            WHEN( "the item is modified" ) {
                modifier.modify( subject );

                THEN( "charges are set to 1" ) {
                    CHECK( subject.charges == 1 );
                }
            }
        }

        AND_GIVEN( "a modifier that sets min and max charges to -1" ) {
            const int min_charges = -1;
            const int max_charges = -1;
            Item_modifier modifier;
            modifier.charges = { min_charges, max_charges };

            WHEN( "the item is modified" ) {
                modifier.modify( subject );

                THEN( "charges should be unchanged" ) {
                    CHECK( subject.charges == default_charges );
                }
            }
        }

        AND_GIVEN( "a modifier that sets min and max charges to a range [1, 4]" ) {
            const int min_charges = 1;
            const int max_charges = 4;
            Item_modifier modifier;
            modifier.charges = { min_charges, max_charges };

            WHEN( "the item is modified" ) {
                // We have to repeat this a bunch because the charges assignment
                // actually does rng between min and max, and if we only checked once
                // we might get a false success.
                std::vector<int> results;
                results.reserve( 100 );
                for( int i = 0; i < 100; i++ ) {
                    modifier.modify( subject );
                    results.emplace_back( subject.charges );
                }

                THEN( "charges are set to the expected range of values" ) {
                    CHECK( std::all_of( results.begin(), results.end(), [&]( int v ) {
                        return v >= min_charges && v <= max_charges;
                    } ) );
                }
            }
        }

        AND_GIVEN( "a modifier that sets min and max charges to 4" ) {
            const int expected = 4;
            const int min_charges = expected;
            const int max_charges = expected;
            Item_modifier modifier;
            modifier.charges = { min_charges, max_charges };

            WHEN( "the item is modified" ) {
                // We have to repeat this a bunch because the charges assignment
                // actually does rng between min and max, and if we only checked once
                // we might get a false success.
                std::vector<int> results;
                results.reserve( 100 );
                for( int i = 0; i < 100; i++ ) {
                    modifier.modify( subject );
                    results.emplace_back( subject.charges );
                }

                THEN( "charges are set to the expected value" ) {
                    CHECK( std::all_of( results.begin(), results.end(), [&]( int v ) {
                        return v == expected;
                    } ) );
                }
            }
        }
    }
}
