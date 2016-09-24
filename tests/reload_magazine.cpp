#include "catch/catch.hpp"

#include "game.h"
#include "player.h"
#include "visitable.h"
#include "itype.h"

TEST_CASE( "reload_magazine", "[magazine] [visitable] [item] [item_location]" ) {
    const itype_id gun_id   = "m4a1";
    const ammotype gun_ammo( "223" );
    const itype_id ammo_id  = "556";      // any type of compatible ammo
    const itype_id alt_ammo = "223";      // any alternative type of compatible ammo
    const itype_id bad_ammo = "9mm";      // any type of incompatible ammo
    const itype_id mag_id   = "stanag30"; // must be set to default magazine
    const itype_id bad_mag  = "glockmag"; // any incompatible magazine
    const int mag_cap          = 30;

    CHECK( ammo_id != alt_ammo );
    CHECK( ammo_id != bad_ammo );
    CHECK( alt_ammo != bad_ammo );
    CHECK( mag_id != bad_mag );
    CHECK( mag_cap > 0 );

    player &p = g->u;
    p.worn.clear();
    p.inv.clear();
    p.remove_weapon();
    p.wear_item( item( "backpack" ) ); // so we don't drop anything

    item& mag = p.i_add( item( mag_id ) );
    CHECK( mag.is_magazine() == true );
    CHECK( mag.is_reloadable() == true );
    CHECK( mag.is_reloadable_with( ammo_id ) == true );
    CHECK( mag.is_reloadable_with( alt_ammo ) == true );
    CHECK( mag.is_reloadable_with( bad_ammo ) == false );
    CHECK( p.can_reload( mag ) == true );
    CHECK( p.can_reload( mag, ammo_id ) == true );
    CHECK( p.can_reload( mag, alt_ammo ) == true );
    CHECK( p.can_reload( mag, bad_ammo ) == false );
    CHECK( mag.ammo_type() == gun_ammo );
    CHECK( mag.ammo_capacity() == mag_cap );
    CHECK( mag.ammo_current() == "null" );
    CHECK( mag.ammo_data() == nullptr );

    GIVEN( "An empty magazine" ) {
        CHECK( mag.ammo_remaining() == 0 );

        WHEN( "the magazine is reloaded with incompatible ammo" ) {
            item& ammo = p.i_add( item( bad_ammo ) );
            bool ok = mag.reload( g->u, item_location( p, &ammo ), mag.ammo_capacity() );
            THEN( "reloading should fail" ) {
                REQUIRE_FALSE( ok );
                REQUIRE( mag.ammo_remaining() == 0 );
            }
        }

        WHEN( "the magazine is loaded with an excess of ammo" ) {
            item& ammo = p.i_add( item( ammo_id, calendar::turn, mag_cap + 5 ) );
            REQUIRE( ammo.charges == mag_cap + 5 );

            bool ok = mag.reload( g->u, item_location( p, &ammo ), mag.ammo_capacity() );
            THEN( "reloading is sucessful" ) {
                REQUIRE( ok );

                AND_THEN( "the current ammo is updated" ) {
                    REQUIRE( mag.ammo_current() == ammo_id );
                    REQUIRE( mag.ammo_data() );
                }
                AND_THEN( "the magazine is filled to capacity" ) {
                    REQUIRE( mag.ammo_remaining() == mag.ammo_capacity() );
                }
                AND_THEN( "a single correctly sized ammo stack remains in the inventory" ) {
                    std::vector<const item *> found;
                    p.visit_items( [&ammo_id,&found]( const item *e ) {
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
            item& ammo = p.i_add( item( ammo_id, calendar::turn, mag_cap - 2 ) );
            REQUIRE( ammo.charges == mag_cap - 2 );

            bool ok = mag.reload( g->u, item_location( p, &ammo ), mag.ammo_capacity() );
            THEN( "reloading is sucessful" ) {
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
                    p.visit_items( [&ammo_id,&found]( const item *e ) {
                        if( e->typeId() == ammo_id ) {
                            found.push_back( e );
                        }
                        // ignore ammo contained within guns or magazines
                        return ( e->is_gun() || e->is_magazine() ) ? VisitResponse::SKIP : VisitResponse::NEXT;
                    } );
                    REQUIRE( found.size() == 0 );
                }
            }

            AND_WHEN( "the magazine is further reloaded with matching ammo" ) {
                item& ammo = p.i_add( item( ammo_id, calendar::turn, 10 ) );
                REQUIRE( ammo.charges == 10 );
                REQUIRE( mag.ammo_remaining() == mag_cap - 2 );

                bool ok = mag.reload( g->u, item_location( p, &ammo ), mag.ammo_capacity() );
                THEN( "further reloading is sucessful" ) {
                    REQUIRE( ok );

                    AND_THEN( "the magazine is filled to capacity" ) {
                        REQUIRE( mag.ammo_remaining() == mag.ammo_capacity() );
                    }
                    AND_THEN( "a single correctly sized ammo stack remains in the inventory" ) {
                        std::vector<const item *> found;
                        p.visit_items( [&ammo_id,&found]( const item *e ) {
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
                item& ammo = p.i_add( item( alt_ammo ) );
                bool ok = mag.reload( g->u, item_location( p, &ammo ), mag.ammo_capacity() );
                THEN( "further reloading should fail" ) {
                    REQUIRE_FALSE( ok );
                    REQUIRE( mag.ammo_remaining() == mag_cap - 2 );
                }
            }

            AND_WHEN( "the magazine is further reloaded with incompatible ammo" ) {
                item& ammo = p.i_add( item( bad_ammo ) );
                bool ok = mag.reload( g->u, item_location( p, &ammo ), mag.ammo_capacity() );
                THEN( "further reloading should fail" ) {
                    REQUIRE_FALSE( ok );
                    REQUIRE( mag.ammo_remaining() == mag_cap - 2 );
                }
            }
        }
    }

    GIVEN( "an empty gun without an integral magazine" ) {
        item& gun = p.i_add( item( gun_id ) );
        CHECK( gun.is_gun() == true );
        CHECK( gun.is_reloadable() == true );
        CHECK( p.can_reload( gun ) == true );
        CHECK( p.can_reload( gun, mag_id ) == true );
        CHECK( p.can_reload( gun, ammo_id ) == false );
        CHECK( gun.magazine_integral() == false );
        CHECK( gun.magazine_default() == mag_id );
        CHECK( gun.magazine_compatible().count( mag_id ) == 1 );
        CHECK( gun.magazine_current() == nullptr );
        CHECK( gun.ammo_type() == gun_ammo );
        CHECK( gun.ammo_capacity() == 0 );
        CHECK( gun.ammo_remaining() == 0 );
        CHECK( gun.ammo_current() == "null" );
        CHECK( gun.ammo_data() == nullptr );

        WHEN( "the gun is reloaded with an incompatible magazine" ) {
            item& mag = p.i_add( item( bad_mag ) );
            bool ok = gun.reload( g->u, item_location( p, &mag ), 1 );
            THEN( "reloading should fail" ) {
                REQUIRE_FALSE( ok );
                REQUIRE_FALSE( gun.magazine_current() );
            }
        }

        WHEN( "the gun is reloaded with an empty compatible magazine" ) {
            CHECK( mag.ammo_remaining() == 0 );

            bool ok = gun.reload( g->u, item_location( p, &mag ), 1 );
            THEN( "reloading is sucessful" ) {
                REQUIRE( ok );
                REQUIRE( gun.magazine_current() );

                AND_THEN( "the gun contains the correct magazine" ) {
                    REQUIRE( gun.magazine_current()->typeId() == mag_id );
                }
                AND_THEN( "the ammo type for the gun remains unchanged" ) {
                    REQUIRE( gun.ammo_type() == gun_ammo );
                }
                AND_THEN( "the ammo capacity is correctly set" ) {
                    REQUIRE( gun.ammo_capacity() == mag_cap );
                }
                AND_THEN( "the gun contains no ammo" ) {
                    REQUIRE( gun.ammo_current() == "null" );
                    REQUIRE( gun.ammo_remaining() == 0 );
                    REQUIRE( gun.ammo_data() == nullptr );
                }
            }
        }

        WHEN( "the gun is reloaded with a partially filled compatible magazine" ) {
            mag.ammo_set( ammo_id, mag_cap - 2 );
            CHECK( mag.ammo_current() == ammo_id );
            CHECK( mag.ammo_remaining() == mag_cap - 2 );

            bool ok = gun.reload( g->u, item_location( p, &mag ), 1 );
            THEN( "reloading is sucessful" ) {
                REQUIRE( ok );
                REQUIRE( gun.magazine_current() );

                AND_THEN( "the gun contains the correct magazine" ) {
                    REQUIRE( gun.magazine_current()->typeId() == mag_id );
                }
                AND_THEN( "the ammo type for the gun remains unchanged" ) {
                    REQUIRE( gun.ammo_type() == gun_ammo );
                }
                AND_THEN( "the ammo capacity is correctly set" ) {
                    REQUIRE( gun.ammo_capacity() == mag_cap );
                }
                AND_THEN( "the gun contains the correct amount and type of ammo" ) {
                    REQUIRE( gun.ammo_remaining() == mag_cap - 2 );
                    REQUIRE( gun.ammo_current() == ammo_id );
                    REQUIRE( gun.ammo_data() );
                }

                AND_WHEN( "the guns magazine is further reloaded with compatible but different ammo" ) {
                    item& ammo = p.i_add( item( alt_ammo, calendar::turn, 10 ) );
                    bool ok = gun.magazine_current()->reload( g->u, item_location( p, &ammo ), 10 );
                    THEN( "further reloading should fail" ) {
                        REQUIRE_FALSE( ok );
                        REQUIRE( gun.ammo_remaining() == mag_cap - 2 );
                    }
                }

                AND_WHEN( "the guns magazine is further reloaded with incompatible ammo" ) {
                    item& ammo = p.i_add( item( bad_ammo, calendar::turn, 10 ) );
                    bool ok = gun.magazine_current()->reload( g->u, item_location( p, &ammo ), 10 );
                    THEN( "further reloading should fail" ) {
                        REQUIRE_FALSE( ok );
                        REQUIRE( gun.ammo_remaining() == mag_cap - 2 );
                    }
                }

                AND_WHEN( "the guns magazine is further reloaded with matching ammo" ) {
                    item& ammo = p.i_add( item( ammo_id, calendar::turn, 10 ) );
                    REQUIRE( ammo.charges == 10 );

                    bool ok = gun.magazine_current()->reload( g->u, item_location( p, &ammo ), 10 );
                    THEN( "further reloading is sucessful" ) {
                        REQUIRE( ok );

                        AND_THEN( "the guns contained magazine is filled to capacity" ) {
                            REQUIRE( gun.ammo_remaining() == mag_cap );
                        }
                        AND_THEN( "a single correctly sized ammo stack remains in the inventory" ) {
                            std::vector<const item *> found;
                            p.visit_items( [&ammo_id,&found]( const item *e ) {
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
            }
        }
    }
}
