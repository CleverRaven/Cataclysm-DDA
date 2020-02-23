#include "cata_string_consts.h"
#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "morale.h"
#include "itype.h"
#include "type_id.h"

TEST_CASE( "identifying unread books", "[reading][book][identify]" )
{
    avatar dummy;

    GIVEN( "player has some unidentified books" ) {
        item &book1 = dummy.i_add( item( "novel_western" ) );
        item &book2 = dummy.i_add( item( "mag_throwing" ) );

        REQUIRE( !dummy.has_identified( book1.typeId() ) );
        REQUIRE( !dummy.has_identified( book2.typeId() ) );

        WHEN( "they read the books once" ) {
            dummy.do_read( book1 );
            dummy.do_read( book2 );

            THEN( "the books should be identified" ) {
                CHECK( dummy.has_identified( book1.typeId() ) );
                CHECK( dummy.has_identified( book2.typeId() ) );
            }
        }
    }
}

TEST_CASE( "reading a book for fun", "[reading][book][fun]" )
{
    avatar dummy;

    GIVEN( "a fun book" ) {
        item &book = dummy.i_add( item( "novel_western" ) );

        WHEN( "player neither loves nor hates books" ) {
            REQUIRE( !dummy.has_trait( trait_LOVES_BOOKS ) );
            REQUIRE( !dummy.has_trait( trait_HATES_BOOKS ) );

            THEN( "the book is a normal amount of fun" ) {
                CHECK( dummy.fun_to_read( book ) == true );
                CHECK( dummy.book_fun_for( book, dummy ) == 4 );
            }
        }

        WHEN( "player loves books" ) {
            dummy.toggle_trait( trait_LOVES_BOOKS );
            REQUIRE( dummy.has_trait( trait_LOVES_BOOKS ) );

            THEN( "the book is extra fun" ) {
                CHECK( dummy.fun_to_read( book ) == true );
                CHECK( dummy.book_fun_for( book, dummy ) == 5 );
            }
        }

        WHEN( "player hates books" ) {
            dummy.toggle_trait( trait_HATES_BOOKS );
            REQUIRE( dummy.has_trait( trait_HATES_BOOKS ) );

            THEN( "the book is no fun at all" ) {
                CHECK( dummy.fun_to_read( book ) == false );
                CHECK( dummy.book_fun_for( book, dummy ) == 0 );
            }
        }
    }

    GIVEN( "a fun book that is also inspirational" ) {
        item &book = dummy.i_add( item( "holybook_pastafarian" ) );
        REQUIRE( book.has_flag( flag_INSPIRATIONAL ) );

        WHEN( "player is not spiritual" ) {
            REQUIRE( !dummy.has_trait( trait_SPIRITUAL ) );

            THEN( "the book is a normal amount of fun" ) {
                CHECK( dummy.fun_to_read( book ) == true );
                CHECK( dummy.book_fun_for( book, dummy ) == 1 );
            }
        }

        WHEN( "player is spiritual" ) {
            dummy.toggle_trait( trait_SPIRITUAL );
            REQUIRE( dummy.has_trait( trait_SPIRITUAL ) );

            THEN( "the book is thrice the fun" ) {
                CHECK( dummy.fun_to_read( book ) == true );
                CHECK( dummy.book_fun_for( book, dummy ) == 3 );
            }
        }
    }
}

/*
 * FIXME: Need to figure out how to read the book for a long enough period of
 * time to learn skill from it (both incrementally and a level-up)
 *
 */
TEST_CASE( "reading a book to learn a skill", "[reading][book][skill][!mayfail]" )
{
    avatar dummy;

    GIVEN( "player has identified a skill book" ) {
        item &book = dummy.i_add( item( "mag_throwing" ) );
        REQUIRE( book.type->book != nullptr );
        skill_id skill = book.type->book->skill;
        dummy.do_read( book );
        REQUIRE( dummy.has_identified( book.typeId() ) );

        AND_GIVEN( "they have not learned the skill yet" ) {
            REQUIRE( dummy.get_skill_level( skill ) == 0 );

            WHEN( "they study the book a while" ) {
                int amount = dummy.time_to_read( book, dummy ); // 384000
                dummy.do_read( book ); // FIXME: This doesn't read until leveled

                THEN( "their skill should increase" ) {
                    CHECK( dummy.get_skill_level( skill ) == 1 );
                }
            }
        }
    }
}

TEST_CASE( "character reading speed", "[reading][character][speed]" )
{
    avatar dummy;

    WHEN( "player has average intelligence" ) {
        REQUIRE( dummy.get_int() == 8 );

        THEN( "reading speed is normal" ) {
            CHECK( 60 == dummy.read_speed() / to_moves<int>( 1_seconds ) );
        }
    }

    WHEN( "player has below-average intelligence" ) {

        THEN( "reading speed is slower" ) {
            dummy.int_max = 7;
            CHECK( 63 == dummy.read_speed() / to_moves<int>( 1_seconds ) );
        }

        THEN( "even slower as intelligence decreases" ) {
            dummy.int_max = 6;
            CHECK( 66 == dummy.read_speed() / to_moves<int>( 1_seconds ) );
            dummy.int_max = 5;
            CHECK( 69 == dummy.read_speed() / to_moves<int>( 1_seconds ) );
            dummy.int_max = 4;
            CHECK( 72 == dummy.read_speed() / to_moves<int>( 1_seconds ) );
        }
    }

    WHEN( "player has above-average intelligence" ) {

        THEN( "reading speed is faster" ) {
            dummy.int_max = 9;
            CHECK( 57 == dummy.read_speed() / to_moves<int>( 1_seconds ) );
        }

        AND_THEN( "even faster as intelligence increases" ) {
            dummy.int_max = 10;
            CHECK( 54 == dummy.read_speed() / to_moves<int>( 1_seconds ) );
            dummy.int_max = 12;
            CHECK( 48 == dummy.read_speed() / to_moves<int>( 1_seconds ) );
            dummy.int_max = 14;
            CHECK( 42 == dummy.read_speed() / to_moves<int>( 1_seconds ) );
        }
    }
}

/*
 * FIXME: Return values from time_to_read are shorter than expected;
 * Book requires intelligence 8, and should take 30 minutes to read, but:
 *
 * INT  8: 24 minutes
 * INT  6: 26 minutes
 * INT 10: 28 minutes
 *
 */
TEST_CASE( "estimated reading time for a book", "[reading][time][!mayfail]" )
{
    avatar dummy;
    int book_time;
    int actual_time;

    GIVEN( "a book requiring average intelligence" ) {
        item &book = dummy.i_add( item( "black_box_transcript" ) );
        REQUIRE( book.type->book != nullptr );
        REQUIRE( book.type->book->intel == 8 );
        book_time = book.type->book->time; // in minutes

        WHEN( "player has average intelligence" ) {
            dummy.int_max = 8;
            REQUIRE( dummy.get_int() == 8 );
            REQUIRE( dummy.read_speed() / to_moves<int>( 1_seconds ) == 60 );

            THEN( "estimated reading time is normal" ) {
                actual_time = dummy.time_to_read( book, dummy ) / to_moves<int>( 1_minutes );
                CHECK( actual_time == book_time );
            }
        }

        WHEN( "player has below average intelligence" ) {
            dummy.int_max = 6;
            REQUIRE( dummy.get_int() == 6 );
            REQUIRE( dummy.read_speed() / to_moves<int>( 1_seconds ) == 66 );

            THEN( "estimated reading time is longer" ) {
                actual_time = dummy.time_to_read( book, dummy ) / to_moves<int>( 1_minutes );
                CHECK( actual_time > book_time );
            }
        }

        WHEN( "player has above average intelligence" ) {
            dummy.int_max = 10;
            REQUIRE( dummy.get_int() == 10 );
            REQUIRE( dummy.read_speed() / to_moves<int>( 1_seconds ) == 54 );

            THEN( "estimated reading time is shorter" ) {
                actual_time = dummy.time_to_read( book, dummy ) / to_moves<int>( 1_minutes );
                CHECK( actual_time < book_time + 23 );
            }
        }
    }
}

