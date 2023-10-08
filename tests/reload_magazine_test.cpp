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

static const ammotype ammo_223( "223" );
static const ammotype ammo_38( "38" );

static const itype_id itype_223( "223" );
static const itype_id itype_357mag_fmj( "357mag_fmj" );
static const itype_id itype_38_special( "38_special" );
static const itype_id itype_556( "556" );
static const itype_id itype_9mm( "9mm" );
static const itype_id itype_glockmag( "glockmag" );
static const itype_id itype_m4_carbine( "m4_carbine" );
static const itype_id itype_stanag30( "stanag30" );
static const itype_id itype_sw_619( "sw_619" );

// NOLINTNEXTLINE(readability-function-size)
TEST_CASE( "reload_magazine", "[magazine] [visitable] [item] [item_location] [reload]" )
{
    const itype_id gun_id = itype_m4_carbine;
    const ammotype gun_ammo = ammo_223;
    const itype_id ammo_id = itype_556; // any type of compatible ammo
    const itype_id alt_ammo = itype_223; // any alternative type of compatible ammo
    const itype_id bad_ammo = itype_9mm; // any type of incompatible ammo
    const itype_id mag_id = itype_stanag30; // must be set to default magazine
    const itype_id bad_mag = itype_glockmag; // any incompatible magazine
    const int mag_cap = 30; // amount of bullets that fit into default magazine

    CHECK( ammo_id != alt_ammo );
    CHECK( ammo_id != bad_ammo );
    CHECK( alt_ammo != bad_ammo );
    CHECK( mag_id != bad_mag );
    CHECK( mag_cap > 0 );

    avatar &player_character = get_avatar();
    player_character.clear_worn();
    player_character.inv->clear();
    player_character.remove_weapon();
    player_character.wear_item( item( "backpack" ) ); // so we don't drop anything

    item_location mag = player_character.i_add( item( mag_id ) );
    const item ammo_it( "556" ); // any type of compatible ammo
    const item alt_ammo_it( "223" ); // any alternative type of compatible ammo
    const item bad_ammo_it( "9mm" ); // any type of incompatible ammo
    const item mag_it( "stanag30" ); // must be set to default magazine
    const item bad_mag_it( "glockmag" ); // any incompatible magazine
    CHECK( mag->is_magazine() );
    CHECK( mag->is_reloadable() );
    CHECK( mag->can_reload_with( ammo_it, true ) );
    CHECK( mag->can_reload_with( alt_ammo_it, true ) );
    CHECK_FALSE( mag->can_reload_with( bad_ammo_it, true ) );
    CHECK( player_character.can_reload( mag_it ) );
    CHECK( player_character.can_reload( *mag, &ammo_it ) );
    CHECK( player_character.can_reload( *mag, &alt_ammo_it ) );
    CHECK_FALSE( player_character.can_reload( *mag, &bad_ammo_it ) );
    CHECK( mag->ammo_types().count( gun_ammo ) );
    CHECK( mag->ammo_capacity( gun_ammo ) == mag_cap );
    CHECK( mag->ammo_current().is_null() );
    CHECK( mag->ammo_data() == nullptr );

    GIVEN( "An empty magazine" ) {
        CHECK( mag->ammo_remaining() == 0 );

        WHEN( "the magazine is reloaded with incompatible ammo" ) {
            item_location ammo = player_character.i_add( item( bad_ammo ) );
            bool ok = mag->reload( player_character, ammo, mag->ammo_capacity( gun_ammo ) );
            THEN( "reloading should fail" ) {
                REQUIRE_FALSE( ok );
                REQUIRE( mag->ammo_remaining() == 0 );
            }
        }

        WHEN( "the magazine is loaded with an excess of ammo" ) {
            item_location ammo = player_character.i_add( item( ammo_id, calendar::turn, mag_cap + 5 ) );
            REQUIRE( ammo->charges == mag_cap + 5 );

            bool ok = mag->reload( player_character, ammo, mag->ammo_capacity( gun_ammo ) );
            THEN( "reloading is successful" ) {
                REQUIRE( ok );

                AND_THEN( "the current ammo is updated" ) {
                    REQUIRE( mag->ammo_current() == ammo_id );
                    REQUIRE( mag->ammo_data() );
                }
                AND_THEN( "the magazine is filled to capacity" ) {
                    REQUIRE( mag->remaining_ammo_capacity() == 0 );
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
            item_location ammo = player_character.i_add( item( ammo_id, calendar::turn, mag_cap - 2 ) );
            REQUIRE( ammo->charges == mag_cap - 2 );

            bool ok = mag->reload( player_character, ammo, mag->ammo_capacity( gun_ammo ) );
            THEN( "reloading is successful" ) {
                REQUIRE( ok );

                AND_THEN( "the current ammo is updated" ) {
                    REQUIRE( mag->ammo_current() == ammo_id );
                    REQUIRE( mag->ammo_data() );
                }
                AND_THEN( "the magazine is filled with the correct quantity" ) {
                    REQUIRE( mag->ammo_remaining() == mag_cap - 2 );
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
                item_location ammo = player_character.i_add( item( ammo_id, calendar::turn, 10 ) );
                REQUIRE( ammo->charges == 10 );
                REQUIRE( mag->ammo_remaining() == mag_cap - 2 );

                bool ok = mag->reload( player_character, ammo, mag->ammo_capacity( gun_ammo ) );
                THEN( "further reloading is successful" ) {
                    REQUIRE( ok );

                    AND_THEN( "the magazine is filled to capacity" ) {
                        REQUIRE( mag->remaining_ammo_capacity() == 0 );
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
                item_location ammo = player_character.i_add( item( alt_ammo ) );
                bool ok = mag->reload( player_character, ammo, mag->ammo_capacity( gun_ammo ) );
                THEN( "further reloading should be succesful" ) {
                    REQUIRE( ok );
                    REQUIRE( mag->ammo_remaining() == mag_cap );
                }
            }

            AND_WHEN( "the magazine is further reloaded with incompatible ammo" ) {
                item_location ammo = player_character.i_add( item( bad_ammo ) );
                bool ok = mag->reload( player_character, ammo, mag->ammo_capacity( gun_ammo ) );
                THEN( "further reloading should fail" ) {
                    REQUIRE_FALSE( ok );
                    REQUIRE( mag->ammo_remaining() == mag_cap - 2 );
                }
            }
        }
    }

    GIVEN( "an empty gun without an integral magazine" ) {
        item_location gun = player_character.i_add( item( gun_id ) );
        CHECK( gun->is_gun() );
        CHECK( gun->is_reloadable() );
        CHECK( player_character.can_reload( *gun ) );
        CHECK( player_character.can_reload( *gun, &mag_it ) );
        CHECK_FALSE( player_character.can_reload( *gun, &ammo_it ) );
        CHECK_FALSE( gun->magazine_integral() );
        CHECK( gun->magazine_default() == mag_id );
        CHECK( gun->magazine_compatible().count( mag_id ) == 1 );
        CHECK( gun->magazine_current() == nullptr );
        CHECK( item( gun->magazine_default() ).ammo_types().count( gun_ammo ) );
        CHECK( gun->ammo_capacity( gun_ammo ) == 0 );
        CHECK( gun->ammo_remaining() == 0 );
        CHECK( gun->ammo_current().is_null() );
        CHECK( gun->ammo_data() == nullptr );

        WHEN( "the gun is reloaded with an incompatible magazine" ) {
            item_location mag = player_character.i_add( item( bad_mag ) );
            bool ok = gun->reload( player_character, mag, 1 );
            THEN( "reloading should fail" ) {
                REQUIRE_FALSE( ok );
                REQUIRE_FALSE( gun->magazine_current() );
            }
        }

        WHEN( "the gun is reloaded with an empty compatible magazine" ) {
            CHECK( mag->ammo_remaining() == 0 );

            bool ok = gun->reload( player_character, mag, 1 );
            THEN( "reloading is successful" ) {
                REQUIRE( ok );
                REQUIRE( gun->magazine_current() );

                AND_THEN( "the gun contains the correct magazine" ) {
                    REQUIRE( gun->magazine_current()->typeId() == mag_id );
                }
                AND_THEN( "the ammo type for the gun remains unchanged" ) {
                    REQUIRE( item( gun->magazine_default() ).ammo_types().count( gun_ammo ) );
                }
                AND_THEN( "the ammo capacity is correctly set" ) {
                    REQUIRE( gun->ammo_capacity( gun_ammo ) == mag_cap );
                }
                AND_THEN( "the gun contains no ammo" ) {
                    REQUIRE( gun->ammo_current().is_null() );
                    REQUIRE( gun->ammo_remaining() == 0 );
                    REQUIRE( gun->ammo_data() == nullptr );
                }
            }
        }

        WHEN( "the gun is reloaded with a partially filled compatible magazine" ) {
            mag->ammo_set( ammo_id, mag_cap - 2 );
            CHECK( mag->ammo_current() == ammo_id );
            CHECK( mag->ammo_remaining() == mag_cap - 2 );

            bool ok = gun->reload( player_character, mag, 1 );
            THEN( "reloading is successful" ) {
                REQUIRE( ok );
                REQUIRE( gun->magazine_current() );

                AND_THEN( "the gun contains the correct magazine" ) {
                    REQUIRE( gun->magazine_current()->typeId() == mag_id );
                }
                AND_THEN( "the ammo type for the gun remains unchanged" ) {
                    REQUIRE( item( gun->magazine_default() ).ammo_types().count( gun_ammo ) );
                }
                AND_THEN( "the ammo capacity is correctly set" ) {
                    REQUIRE( gun->ammo_capacity( gun_ammo ) == mag_cap );
                }
                AND_THEN( "the gun contains the correct amount and type of ammo" ) {
                    REQUIRE( gun->ammo_remaining() == mag_cap - 2 );
                    REQUIRE( gun->ammo_current() == ammo_id );
                    REQUIRE( gun->ammo_data() );
                }

                AND_WHEN( "the guns magazine is further reloaded with compatible but different ammo" ) {
                    item_location ammo = player_character.i_add( item( alt_ammo, calendar::turn, 10 ) );
                    bool ok = gun->magazine_current()->reload( player_character, ammo, 10 );
                    THEN( "further reloading should be succesful" ) {
                        REQUIRE( ok );
                        REQUIRE( gun->ammo_remaining() == mag_cap );
                    }
                }

                AND_WHEN( "the guns magazine is further reloaded with incompatible ammo" ) {
                    item_location ammo = player_character.i_add( item( bad_ammo, calendar::turn, 10 ) );
                    bool ok = gun->magazine_current()->reload( player_character, ammo, 10 );
                    THEN( "further reloading should fail" ) {
                        REQUIRE_FALSE( ok );
                        REQUIRE( gun->ammo_remaining() == mag_cap - 2 );
                    }
                }

                AND_WHEN( "the guns magazine is further reloaded with matching ammo" ) {
                    item_location ammo = player_character.i_add( item( ammo_id, calendar::turn, 10 ) );
                    REQUIRE( ammo->charges == 10 );

                    bool ok = gun->magazine_current()->reload( player_character, ammo, 10 );
                    THEN( "further reloading is successful" ) {
                        REQUIRE( ok );

                        AND_THEN( "the guns contained magazine is filled to capacity" ) {
                            REQUIRE( gun->ammo_remaining() == mag_cap );
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
                    item_location another_mag = player_character.i_add( item( mag_id ) );
                    another_mag->ammo_set( ammo_id, mag_cap );
                    std::vector<item::reload_option> ammo_list;
                    CHECK( player_character.list_ammo( gun, ammo_list, false ) );
                    CHECK( !ammo_list.empty() );
                    bool ok = gun->reload( player_character, another_mag, 1 );
                    THEN( "the gun is now loaded with the full magazine" ) {
                        CHECK( ok );
                        CHECK( gun->ammo_remaining() == mag_cap );
                    }
                }
            }
        }
    }
}

TEST_CASE( "reload_revolver", "[visitable] [item] [item_location] [reload]" )
{
    const itype_id gun_id = itype_sw_619;
    const ammotype gun_ammo = ammo_38;
    const itype_id ammo_id = itype_38_special; // any type of compatible ammo
    const itype_id alt_ammo = itype_357mag_fmj; // any alternative type of compatible ammo
    const itype_id bad_ammo = itype_9mm; // any type of incompatible ammo
    const int mag_cap = 7; // amount of bullets that fit into cylinder

    const item ammo_it( "38_special" ); // any type of compatible ammo

    CHECK( ammo_id != alt_ammo );
    CHECK( ammo_id != bad_ammo );
    CHECK( alt_ammo != bad_ammo );

    Character &player_character = get_player_character();
    player_character.clear_worn();
    player_character.inv->clear();
    player_character.remove_weapon();
    player_character.wear_item( item( "backpack" ) ); // so we don't drop anything

    GIVEN( "an empty gun with an integral magazine" ) {
        item_location gun = player_character.i_add( item( gun_id ) );
        CHECK( gun->is_gun() );
        CHECK( gun->is_reloadable() );
        CHECK( player_character.can_reload( *gun ) );
        CHECK( player_character.can_reload( *gun, &ammo_it ) );
        CHECK( gun->magazine_integral() );
        CHECK( gun->ammo_capacity( gun_ammo ) == mag_cap );
        CHECK( gun->ammo_remaining() == 0 );
        CHECK( gun->ammo_current().is_null() );
        CHECK( gun->ammo_data() == nullptr );

        WHEN( "the gun is reloaded with incompatible ammo" ) {
            item_location ammo = player_character.i_add( item( bad_ammo ) );
            bool ok = gun->reload( player_character, ammo, gun->ammo_capacity( gun_ammo ) );
            THEN( "reloading should fail" ) {
                REQUIRE_FALSE( ok );
                REQUIRE( gun->ammo_remaining() == 0 );
            }
        }

        WHEN( "the gun is loaded with an excess of ammo" ) {
            item_location ammo = player_character.i_add( item( ammo_id, calendar::turn, mag_cap + 5 ) );
            REQUIRE( ammo->charges == mag_cap + 5 );

            bool ok = gun->reload( player_character, ammo, gun->ammo_capacity( gun_ammo ) );
            THEN( "reloading is successful" ) {
                REQUIRE( ok );

                AND_THEN( "the current ammo is updated" ) {
                    REQUIRE( gun->ammo_current() == ammo_id );
                    REQUIRE( gun->ammo_data() );
                }
                AND_THEN( "the gun is filled to capacity" ) {
                    REQUIRE( gun->remaining_ammo_capacity() == 0 );
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
            item_location ammo = player_character.i_add( item( ammo_id, calendar::turn, mag_cap - 2 ) );
            REQUIRE( ammo->charges == mag_cap - 2 );

            bool ok = gun->reload( player_character, ammo, gun->ammo_capacity( gun_ammo ) );
            THEN( "reloading is successful" ) {
                REQUIRE( ok == true );

                AND_THEN( "the current ammo is updated" ) {
                    REQUIRE( gun->ammo_current() == ammo_id );
                    REQUIRE( gun->ammo_data() );
                }
                AND_THEN( "the gun is filled with the correct quantity" ) {
                    REQUIRE( gun->ammo_remaining() == mag_cap - 2 );
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
                item_location ammo = player_character.i_add( item( ammo_id, calendar::turn, 10 ) );
                REQUIRE( ammo->charges == 10 );
                REQUIRE( gun->ammo_remaining() == mag_cap - 2 );

                bool ok = gun->reload( player_character, ammo, gun->ammo_capacity( gun_ammo ) );
                THEN( "further reloading is successful" ) {
                    REQUIRE( ok );

                    AND_THEN( "the gun is filled to capacity" ) {
                        REQUIRE( gun->remaining_ammo_capacity() == 0 );
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
                item_location ammo = player_character.i_add( item( alt_ammo ) );
                bool ok = gun->reload( player_character, ammo, gun->ammo_capacity( gun_ammo ) );
                THEN( "further reloading should fail" ) {
                    REQUIRE_FALSE( ok );
                    REQUIRE( gun->ammo_remaining() == mag_cap - 2 );
                }
            }

            AND_WHEN( "the gun is further reloaded with incompatible ammo" ) {
                item_location ammo = player_character.i_add( item( bad_ammo ) );
                bool ok = gun->reload( player_character, ammo, gun->ammo_capacity( gun_ammo ) );
                THEN( "further reloading should fail" ) {
                    REQUIRE_FALSE( ok );
                    REQUIRE( gun->ammo_remaining() == mag_cap - 2 );
                }
            }
        }
    }
}
