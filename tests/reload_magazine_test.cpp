#include <functional>
#include <list>
#include <memory>
#include <set>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "pimpl.h"
#include "type_id.h"
#include "visitable.h"

struct itype;

// NOLINTNEXTLINE(readability-function-size)
TEST_CASE( "reload_magazine", "[magazine] [visitable] [item] [item_location]" )
{
    const itype_id gun_id( "nato_assault_rifle" );
    const ammotype gun_ammo( "223" );
    const itype_id ammo_id( "556" ); // any type of compatible ammo
    const itype_id alt_ammo( "223" ); // any alternative type of compatible ammo
    const itype_id bad_ammo( "9mm" ); // any type of incompatible ammo
    const itype_id mag_id( "stanag30" ); // must be set to default magazine
    const itype_id bad_mag( "glockmag" ); // any incompatible magazine
    const int mag_cap = 30; // amount of bullets that fit into default magazine

    CHECK( ammo_id != alt_ammo );
    CHECK( ammo_id != bad_ammo );
    CHECK( alt_ammo != bad_ammo );
    CHECK( mag_id != bad_mag );
    CHECK( mag_cap > 0 );

    avatar &player_character = get_avatar();
    player_character.worn.clear();
    player_character.inv->clear();
    player_character.remove_weapon();
    player_character.wear_item( item( "backpack" ) ); // so we don't drop anything

    item &mag = player_character.i_add( item( mag_id ) );
    CHECK( mag.is_magazine() == true );
    CHECK( mag.is_reloadable() == true );
    CHECK( mag.is_reloadable_with( ammo_id ) == true );
    CHECK( mag.is_reloadable_with( alt_ammo ) == true );
    CHECK( mag.is_reloadable_with( bad_ammo ) == false );
    CHECK( player_character.can_reload( mag ) == true );
    CHECK( player_character.can_reload( mag, ammo_id ) == true );
    CHECK( player_character.can_reload( mag, alt_ammo ) == true );
    CHECK( player_character.can_reload( mag, bad_ammo ) == false );
    CHECK( mag.ammo_types().count( gun_ammo ) );
    CHECK( mag.ammo_capacity( gun_ammo ) == mag_cap );
    CHECK( mag.ammo_current().is_null() );
    CHECK( mag.ammo_data() == nullptr );

    GIVEN( "An empty magazine" ) {
        CHECK( mag.ammo_remaining() == 0 );

        WHEN( "the magazine is reloaded with incompatible ammo" ) {
            item &ammo = player_character.i_add( item( bad_ammo ) );
            bool ok = mag.reload( player_character, item_location( player_character, &ammo ),
                                  mag.ammo_capacity( gun_ammo ) );
            THEN( "reloading should fail" ) {
                REQUIRE_FALSE( ok );
                REQUIRE( mag.ammo_remaining() == 0 );
            }
        }

        WHEN( "the magazine is loaded with an excess of ammo" ) {
            item &ammo = player_character.i_add( item( ammo_id, calendar::turn, mag_cap + 5 ) );
            REQUIRE( ammo.charges == mag_cap + 5 );

            bool ok = mag.reload( player_character, item_location( player_character, &ammo ),
                                  mag.ammo_capacity( gun_ammo ) );
            THEN( "reloading is successful" ) {
                REQUIRE( ok );

                AND_THEN( "the current ammo is updated" ) {
                    REQUIRE( mag.ammo_current() == ammo_id );
                    REQUIRE( mag.ammo_data() );
                }
                AND_THEN( "the magazine is filled to capacity" ) {
                    REQUIRE( mag.remaining_ammo_capacity() == 0 );
                }
                AND_THEN( "a single correctly sized ammo stack remains in the inventory" ) {
                    std::vector<const item *> found;
                    player_character.visit_items( [&ammo_id, &found]( const item * e, item * ) {
                        if( e->typeId() == ammo_id ) {
                            found.push_back( e );
                        }
                        // ignore ammo contained within guns or magazines
                        return ( e->is_gun() || e->is_magazine() ) ? VisitResponse::SKIP : VisitResponse::NEXT;
                    } );
                    REQUIRE( found.size() == 1 );
                    REQUIRE( found[0]->charges == 5 );
                }
            }
        }

        WHEN( "the magazine is partially reloaded with compatible ammo" ) {
            item &ammo = player_character.i_add( item( ammo_id, calendar::turn, mag_cap - 2 ) );
            REQUIRE( ammo.charges == mag_cap - 2 );

            bool ok = mag.reload( player_character, item_location( player_character, &ammo ),
                                  mag.ammo_capacity( gun_ammo ) );
            THEN( "reloading is successful" ) {
                REQUIRE( ok == true );

                AND_THEN( "the current ammo is updated" ) {
                    REQUIRE( mag.ammo_current() == ammo_id );
                    REQUIRE( mag.ammo_data() );
                }
                AND_THEN( "the magazine is filled with the correct quantity" ) {
                    REQUIRE( mag.ammo_remaining() == mag_cap - 2 );
                }
                AND_THEN( "the ammo stack was completely used" ) {
                    std::vector<const item *> found;
                    player_character.visit_items( [&ammo_id, &found]( const item * e, item * ) {
                        if( e->typeId() == ammo_id ) {
                            found.push_back( e );
                        }
                        // ignore ammo contained within guns or magazines
                        return ( e->is_gun() || e->is_magazine() ) ? VisitResponse::SKIP : VisitResponse::NEXT;
                    } );
                    REQUIRE( found.empty() );
                }
            }

            AND_WHEN( "the magazine is further reloaded with matching ammo" ) {
                item &ammo = player_character.i_add( item( ammo_id, calendar::turn, 10 ) );
                REQUIRE( ammo.charges == 10 );
                REQUIRE( mag.ammo_remaining() == mag_cap - 2 );

                bool ok = mag.reload( player_character, item_location( player_character, &ammo ),
                                      mag.ammo_capacity( gun_ammo ) );
                THEN( "further reloading is successful" ) {
                    REQUIRE( ok );

                    AND_THEN( "the magazine is filled to capacity" ) {
                        REQUIRE( mag.remaining_ammo_capacity() == 0 );
                    }
                    AND_THEN( "a single correctly sized ammo stack remains in the inventory" ) {
                        std::vector<const item *> found;
                        player_character.visit_items( [&ammo_id, &found]( const item * e, item * ) {
                            if( e->typeId() == ammo_id ) {
                                found.push_back( e );
                            }
                            // ignore ammo contained within guns or magazines
                            return ( e->is_gun() || e->is_magazine() ) ? VisitResponse::SKIP : VisitResponse::NEXT;
                        } );
                        REQUIRE( found.size() == 1 );
                        REQUIRE( found[0]->charges == 8 );
                    }
                }
            }

            AND_WHEN( "the magazine is further reloaded with compatible but different ammo" ) {
                item &ammo = player_character.i_add( item( alt_ammo ) );
                bool ok = mag.reload( player_character, item_location( player_character, &ammo ),
                                      mag.ammo_capacity( gun_ammo ) );
                THEN( "further reloading should fail" ) {
                    REQUIRE_FALSE( ok );
                    REQUIRE( mag.ammo_remaining() == mag_cap - 2 );
                }
            }

            AND_WHEN( "the magazine is further reloaded with incompatible ammo" ) {
                item &ammo = player_character.i_add( item( bad_ammo ) );
                bool ok = mag.reload( player_character, item_location( player_character, &ammo ),
                                      mag.ammo_capacity( gun_ammo ) );
                THEN( "further reloading should fail" ) {
                    REQUIRE_FALSE( ok );
                    REQUIRE( mag.ammo_remaining() == mag_cap - 2 );
                }
            }
        }
    }

    GIVEN( "an empty gun without an integral magazine" ) {
        item &gun = player_character.i_add( item( gun_id ) );
        CHECK( gun.is_gun() == true );
        CHECK( gun.is_reloadable() == true );
        CHECK( player_character.can_reload( gun ) == true );
        CHECK( player_character.can_reload( gun, mag_id ) == true );
        CHECK( player_character.can_reload( gun, ammo_id ) == false );
        CHECK( gun.magazine_integral() == false );
        CHECK( gun.magazine_default() == mag_id );
        CHECK( gun.magazine_compatible().count( mag_id ) == 1 );
        CHECK( gun.magazine_current() == nullptr );
        CHECK( item( gun.magazine_default() ).ammo_types().count( gun_ammo ) );
        CHECK( gun.ammo_capacity( gun_ammo ) == 0 );
        CHECK( gun.ammo_remaining() == 0 );
        CHECK( gun.ammo_current().is_null() );
        CHECK( gun.ammo_data() == nullptr );

        WHEN( "the gun is reloaded with an incompatible magazine" ) {
            item &mag = player_character.i_add( item( bad_mag ) );
            bool ok = gun.reload( player_character, item_location( player_character, &mag ), 1 );
            THEN( "reloading should fail" ) {
                REQUIRE_FALSE( ok );
                REQUIRE_FALSE( gun.magazine_current() );
            }
        }

        WHEN( "the gun is reloaded with an empty compatible magazine" ) {
            CHECK( mag.ammo_remaining() == 0 );

            bool ok = gun.reload( player_character, item_location( player_character, &mag ), 1 );
            THEN( "reloading is successful" ) {
                REQUIRE( ok );
                REQUIRE( gun.magazine_current() );

                AND_THEN( "the gun contains the correct magazine" ) {
                    REQUIRE( gun.magazine_current()->typeId() == mag_id );
                }
                AND_THEN( "the ammo type for the gun remains unchanged" ) {
                    REQUIRE( item( gun.magazine_default() ).ammo_types().count( gun_ammo ) );
                }
                AND_THEN( "the ammo capacity is correctly set" ) {
                    REQUIRE( gun.ammo_capacity( gun_ammo ) == mag_cap );
                }
                AND_THEN( "the gun contains no ammo" ) {
                    REQUIRE( gun.ammo_current().is_null() );
                    REQUIRE( gun.ammo_remaining() == 0 );
                    REQUIRE( gun.ammo_data() == nullptr );
                }
            }
        }

        WHEN( "the gun is reloaded with a partially filled compatible magazine" ) {
            mag.ammo_set( ammo_id, mag_cap - 2 );
            CHECK( mag.ammo_current() == ammo_id );
            CHECK( mag.ammo_remaining() == mag_cap - 2 );

            bool ok = gun.reload( player_character, item_location( player_character, &mag ), 1 );
            THEN( "reloading is successful" ) {
                REQUIRE( ok );
                REQUIRE( gun.magazine_current() );

                AND_THEN( "the gun contains the correct magazine" ) {
                    REQUIRE( gun.magazine_current()->typeId() == mag_id );
                }
                AND_THEN( "the ammo type for the gun remains unchanged" ) {
                    REQUIRE( item( gun.magazine_default() ).ammo_types().count( gun_ammo ) );
                }
                AND_THEN( "the ammo capacity is correctly set" ) {
                    REQUIRE( gun.ammo_capacity( gun_ammo ) == mag_cap );
                }
                AND_THEN( "the gun contains the correct amount and type of ammo" ) {
                    REQUIRE( gun.ammo_remaining() == mag_cap - 2 );
                    REQUIRE( gun.ammo_current() == ammo_id );
                    REQUIRE( gun.ammo_data() );
                }

                AND_WHEN( "the guns magazine is further reloaded with compatible but different ammo" ) {
                    item &ammo = player_character.i_add( item( alt_ammo, calendar::turn, 10 ) );
                    bool ok = gun.magazine_current()->reload( player_character, item_location( player_character,
                              &ammo ), 10 );
                    THEN( "further reloading should fail" ) {
                        REQUIRE_FALSE( ok );
                        REQUIRE( gun.ammo_remaining() == mag_cap - 2 );
                    }
                }

                AND_WHEN( "the guns magazine is further reloaded with incompatible ammo" ) {
                    item &ammo = player_character.i_add( item( bad_ammo, calendar::turn, 10 ) );
                    bool ok = gun.magazine_current()->reload( player_character, item_location( player_character,
                              &ammo ), 10 );
                    THEN( "further reloading should fail" ) {
                        REQUIRE_FALSE( ok );
                        REQUIRE( gun.ammo_remaining() == mag_cap - 2 );
                    }
                }

                AND_WHEN( "the guns magazine is further reloaded with matching ammo" ) {
                    item &ammo = player_character.i_add( item( ammo_id, calendar::turn, 10 ) );
                    REQUIRE( ammo.charges == 10 );

                    bool ok = gun.magazine_current()->reload( player_character, item_location( player_character,
                              &ammo ), 10 );
                    THEN( "further reloading is successful" ) {
                        REQUIRE( ok );

                        AND_THEN( "the guns contained magazine is filled to capacity" ) {
                            REQUIRE( gun.ammo_remaining() == mag_cap );
                        }
                        AND_THEN( "a single correctly sized ammo stack remains in the inventory" ) {
                            std::vector<const item *> found;
                            player_character.visit_items( [&ammo_id, &found]( const item * e, item * ) {
                                if( e->typeId() == ammo_id ) {
                                    found.push_back( e );
                                }
                                // ignore ammo contained within guns or magazines
                                return ( e->is_gun() || e->is_magazine() ) ?
                                       VisitResponse::SKIP : VisitResponse::NEXT;
                            } );
                            REQUIRE( found.size() == 1 );
                            REQUIRE( found[0]->charges == 8 );
                        }
                    }
                }
                AND_WHEN( "the gun is reloaded with a full magazine" ) {
                    item &another_mag = player_character.i_add( item( mag_id ) );
                    another_mag.ammo_set( ammo_id, mag_cap );
                    std::vector<item::reload_option> ammo_list;
                    CHECK( player_character.list_ammo( gun, ammo_list, false ) );
                    CHECK( !ammo_list.empty() );
                    bool ok = gun.reload( player_character, item_location( player_character, &another_mag ), 1 );
                    THEN( "the gun is now loaded with the full magazine" ) {
                        CHECK( ok );
                        CHECK( gun.ammo_remaining() == mag_cap );
                    }
                }
            }
        }
    }
}

TEST_CASE( "reload_revolver", "[visitable] [item] [item_location]" )
{
    const itype_id gun_id( "sw_619" );
    const ammotype gun_ammo( "38" );
    const itype_id ammo_id( "38_special" ); // any type of compatible ammo
    const itype_id alt_ammo( "357mag_fmj" ); // any alternative type of compatible ammo
    const itype_id bad_ammo( "9mm" ); // any type of incompatible ammo
    const int mag_cap = 7; // amount of bullets that fit into cylinder

    CHECK( ammo_id != alt_ammo );
    CHECK( ammo_id != bad_ammo );
    CHECK( alt_ammo != bad_ammo );

    Character &player_character = get_player_character();
    player_character.worn.clear();
    player_character.inv->clear();
    player_character.remove_weapon();
    player_character.wear_item( item( "backpack" ) ); // so we don't drop anything

    GIVEN( "an empty gun with an integral magazine" ) {
        item &gun = player_character.i_add( item( gun_id ) );
        CHECK( gun.is_gun() == true );
        CHECK( gun.is_reloadable() == true );
        CHECK( player_character.can_reload( gun ) == true );
        CHECK( player_character.can_reload( gun, ammo_id ) == true );
        CHECK( gun.magazine_integral() == true );
        CHECK( gun.ammo_capacity( gun_ammo ) == mag_cap );
        CHECK( gun.ammo_remaining() == 0 );
        CHECK( gun.ammo_current().is_null() );
        CHECK( gun.ammo_data() == nullptr );

        WHEN( "the gun is reloaded with incompatible ammo" ) {
            item &ammo = player_character.i_add( item( bad_ammo ) );
            bool ok = gun.reload( player_character, item_location( player_character, &ammo ),
                                  gun.ammo_capacity( gun_ammo ) );
            THEN( "reloading should fail" ) {
                REQUIRE_FALSE( ok );
                REQUIRE( gun.ammo_remaining() == 0 );
            }
        }

        WHEN( "the gun is loaded with an excess of ammo" ) {
            item &ammo = player_character.i_add( item( ammo_id, calendar::turn, mag_cap + 5 ) );
            REQUIRE( ammo.charges == mag_cap + 5 );

            bool ok = gun.reload( player_character, item_location( player_character, &ammo ),
                                  gun.ammo_capacity( gun_ammo ) );
            THEN( "reloading is successful" ) {
                REQUIRE( ok );

                AND_THEN( "the current ammo is updated" ) {
                    REQUIRE( gun.ammo_current() == ammo_id );
                    REQUIRE( gun.ammo_data() );
                }
                AND_THEN( "the gun is filled to capacity" ) {
                    REQUIRE( gun.remaining_ammo_capacity() == 0 );
                }
                AND_THEN( "a single correctly sized ammo stack remains in the inventory" ) {
                    std::vector<const item *> found;
                    player_character.visit_items( [&ammo_id, &found]( const item * e, item * ) {
                        if( e->typeId() == ammo_id ) {
                            found.push_back( e );
                        }
                        // ignore ammo contained within guns or magazines
                        return ( e->is_gun() || e->is_magazine() ) ? VisitResponse::SKIP : VisitResponse::NEXT;
                    } );
                    REQUIRE( found.size() == 1 );
                    REQUIRE( found[0]->charges == 5 );
                }
            }
        }

        WHEN( "the gun is partially reloaded with compatible ammo" ) {
            item &ammo = player_character.i_add( item( ammo_id, calendar::turn, mag_cap - 2 ) );
            REQUIRE( ammo.charges == mag_cap - 2 );

            bool ok = gun.reload( player_character, item_location( player_character, &ammo ),
                                  gun.ammo_capacity( gun_ammo ) );
            THEN( "reloading is successful" ) {
                REQUIRE( ok == true );

                AND_THEN( "the current ammo is updated" ) {
                    REQUIRE( gun.ammo_current() == ammo_id );
                    REQUIRE( gun.ammo_data() );
                }
                AND_THEN( "the gun is filled with the correct quantity" ) {
                    REQUIRE( gun.ammo_remaining() == mag_cap - 2 );
                }
                AND_THEN( "the ammo stack was completely used" ) {
                    std::vector<const item *> found;
                    player_character.visit_items( [&ammo_id, &found]( const item * e, item * ) {
                        if( e->typeId() == ammo_id ) {
                            found.push_back( e );
                        }
                        // ignore ammo contained within guns or magazines
                        return ( e->is_gun() || e->is_magazine() ) ? VisitResponse::SKIP : VisitResponse::NEXT;
                    } );
                    REQUIRE( found.empty() );
                }
            }

            AND_WHEN( "the gun is further reloaded with matching ammo" ) {
                item &ammo = player_character.i_add( item( ammo_id, calendar::turn, 10 ) );
                REQUIRE( ammo.charges == 10 );
                REQUIRE( gun.ammo_remaining() == mag_cap - 2 );

                bool ok = gun.reload( player_character, item_location( player_character, &ammo ),
                                      gun.ammo_capacity( gun_ammo ) );
                THEN( "further reloading is successful" ) {
                    REQUIRE( ok );

                    AND_THEN( "the gun is filled to capacity" ) {
                        REQUIRE( gun.remaining_ammo_capacity() == 0 );
                    }
                    AND_THEN( "a single correctly sized ammo stack remains in the inventory" ) {
                        std::vector<const item *> found;
                        player_character.visit_items( [&ammo_id, &found]( const item * e, item * ) {
                            if( e->typeId() == ammo_id ) {
                                found.push_back( e );
                            }
                            // ignore ammo contained within guns or magazines
                            return ( e->is_gun() || e->is_magazine() ) ? VisitResponse::SKIP : VisitResponse::NEXT;
                        } );
                        REQUIRE( found.size() == 1 );
                        REQUIRE( found[0]->charges == 8 );
                    }
                }
            }

            AND_WHEN( "the gun is further reloaded with compatible but different ammo" ) {
                item &ammo = player_character.i_add( item( alt_ammo ) );
                bool ok = gun.reload( player_character, item_location( player_character, &ammo ),
                                      gun.ammo_capacity( gun_ammo ) );
                THEN( "further reloading should fail" ) {
                    REQUIRE_FALSE( ok );
                    REQUIRE( gun.ammo_remaining() == mag_cap - 2 );
                }
            }

            AND_WHEN( "the gun is further reloaded with incompatible ammo" ) {
                item &ammo = player_character.i_add( item( bad_ammo ) );
                bool ok = gun.reload( player_character, item_location( player_character, &ammo ),
                                      gun.ammo_capacity( gun_ammo ) );
                THEN( "further reloading should fail" ) {
                    REQUIRE_FALSE( ok );
                    REQUIRE( gun.ammo_remaining() == mag_cap - 2 );
                }
            }
        }
    }
}
