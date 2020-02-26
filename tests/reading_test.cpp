#include "avatar.h"
#include "cata_string_consts.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "morale.h"
#include "player_helpers.h"
#include "skill.h"
#include "type_id.h"
#include "vehicle.h"

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
        REQUIRE( book.type->book );
        REQUIRE( book.type->book->fun > 0 );
        int book_fun = book.type->book->fun;

        WHEN( "player neither loves nor hates books" ) {
            REQUIRE( !dummy.has_trait( trait_LOVES_BOOKS ) );
            REQUIRE( !dummy.has_trait( trait_HATES_BOOKS ) );

            THEN( "the book is a normal amount of fun" ) {
                CHECK( dummy.fun_to_read( book ) == true );
                CHECK( dummy.book_fun_for( book, dummy ) == book_fun );
            }
        }

        WHEN( "player loves books" ) {
            dummy.toggle_trait( trait_LOVES_BOOKS );
            REQUIRE( dummy.has_trait( trait_LOVES_BOOKS ) );

            THEN( "the book is extra fun" ) {
                CHECK( dummy.fun_to_read( book ) == true );
                CHECK( dummy.book_fun_for( book, dummy ) == book_fun + 1 );
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
        REQUIRE( book.type->book );
        REQUIRE( book.type->book->fun > 0 );
        int book_fun = book.type->book->fun;

        WHEN( "player is not spiritual" ) {
            REQUIRE( !dummy.has_trait( trait_SPIRITUAL ) );

            THEN( "the book is a normal amount of fun" ) {
                CHECK( dummy.fun_to_read( book ) == true );
                CHECK( dummy.book_fun_for( book, dummy ) == book_fun );
            }
        }

        WHEN( "player is spiritual" ) {
            dummy.toggle_trait( trait_SPIRITUAL );
            REQUIRE( dummy.has_trait( trait_SPIRITUAL ) );

            THEN( "the book is thrice the fun" ) {
                CHECK( dummy.fun_to_read( book ) == true );
                CHECK( dummy.book_fun_for( book, dummy ) == book_fun * 3 );
            }
        }
    }
}

/*
 * FIXME: Need to figure out how to read the book for a long enough period of
 * time to learn skill from it (both incrementally and a level-up)
 *
 */
static int read_awhile( avatar &dummy, int turn_interrupt )
{
    int turns = 0;
    while( dummy.activity.id() == activity_id( "ACT_READ" ) ) {
        if( turns >= turn_interrupt ) {
            return turns;
        }
        ++turns;
        dummy.moves = 100;
        dummy.activity.do_turn( dummy );
    }
    return turns;
}

TEST_CASE( "reading a book to learn a skill", "[reading][book][skill][!mayfail]" )
{
    avatar dummy;

    GIVEN( "player has identified a skill book" ) {
        item &book = dummy.i_add( item( "mag_throwing" ) );
        REQUIRE( book.type->book );
        skill_id skill = book.type->book->skill;
        dummy.do_read( book );
        REQUIRE( dummy.has_identified( book.typeId() ) );

        AND_GIVEN( "they have not learned the skill yet" ) {
            REQUIRE( dummy.get_skill_level( skill ) == 0 );
            REQUIRE( dummy.get_skill_level_object( skill ).can_train() );

            WHEN( "they study the book a while" ) {
                int amount = dummy.time_to_read( book, dummy ); // 384000
                dummy.read( book ); // read continuously?
                REQUIRE( dummy.activity );
                int turns_read = read_awhile( dummy, amount );
                CHECK( turns_read == 1000 );

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

    // Note: read_speed() returns number of moves;
    // 6000 == 60 seconds
    WHEN( "player has average intelligence" ) {
        REQUIRE( dummy.get_int() == 8 );

        THEN( "reading speed is normal" ) {
            CHECK( dummy.read_speed() == 6000 );
        }
    }

    WHEN( "player has below-average intelligence" ) {

        THEN( "reading speed gets slower as intelligence decreases" ) {
            dummy.int_max = 7;
            CHECK( dummy.read_speed() == 6300 );
            dummy.int_max = 6;
            CHECK( dummy.read_speed() == 6600 );
            dummy.int_max = 5;
            CHECK( dummy.read_speed() == 6900 );
            dummy.int_max = 4;
            CHECK( dummy.read_speed() == 7200 );
        }
    }

    WHEN( "player has above-average intelligence" ) {

        THEN( "reading speed gets faster as intelligence increases" ) {
            dummy.int_max = 9;
            CHECK( dummy.read_speed() == 5700 );
            dummy.int_max = 10;
            CHECK( dummy.read_speed() == 5400 );
            dummy.int_max = 12;
            CHECK( dummy.read_speed() == 4800 );
            dummy.int_max = 14;
            CHECK( dummy.read_speed() == 4200 );
        }
    }
}

TEST_CASE( "estimated reading time for a book", "[reading][book][time]" )
{
    avatar dummy;

    item &child = dummy.i_add( item( "child_book" ) );
    item &western = dummy.i_add( item( "novel_western" ) );
    item &alpha = dummy.i_add( item( "recipe_alpha" ) );

    // Ensure the books have expected attributes
    REQUIRE( child.type->book );
    REQUIRE( western.type->book );
    REQUIRE( alpha.type->book );

    // Convert time to read from minutes to moves, for easier comparison later
    int moves_easy = child.type->book->time * to_moves<int>( 1_minutes );
    int moves_avg = western.type->book->time * to_moves<int>( 1_minutes );
    int moves_hard = alpha.type->book->time * to_moves<int>( 1_minutes );

    GIVEN( "some unidentified books and plenty of light" ) {
        item &lamp = dummy.i_add( item( "atomic_lamp" ) );
        REQUIRE( dummy.fine_detail_vision_mod() == 1 );

        REQUIRE( !dummy.has_identified( child.typeId() ) );
        REQUIRE( !dummy.has_identified( western.typeId() ) );

        THEN( "identifying the books takes 1/10th of the normal reading time" ) {
            CHECK( dummy.time_to_read( western, dummy ) == moves_avg / 10 );
            CHECK( dummy.time_to_read( child, dummy ) == moves_easy / 10 );
        }
    }

    GIVEN( "some identified books and plenty of light" ) {
        item &lamp = dummy.i_add( item( "atomic_lamp" ) );
        REQUIRE( dummy.fine_detail_vision_mod() == 1 );

        // Identify the books
        dummy.do_read( child );
        dummy.do_read( western );
        dummy.do_read( alpha );
        REQUIRE( dummy.has_identified( child.typeId() ) );
        REQUIRE( dummy.has_identified( western.typeId() ) );
        REQUIRE( dummy.has_identified( alpha.typeId() ) );

        WHEN( "player has average intelligence" ) {
            dummy.int_max = 8;
            REQUIRE( dummy.get_int() == 8 );
            REQUIRE( dummy.read_speed() == 6000 ); // 60s, "normal"

            THEN( "they can read books at their reading level in the normal amount time" ) {
                CHECK( dummy.time_to_read( child, dummy ) == moves_easy );
                CHECK( dummy.time_to_read( western, dummy ) == moves_avg );
            }
            AND_THEN( "they can read books above their reading level, but it takes longer" ) {
                CHECK( dummy.time_to_read( alpha, dummy ) > moves_hard );
            }
        }

        WHEN( "player has below average intelligence" ) {
            dummy.int_max = 6;
            REQUIRE( dummy.get_int() == 6 );
            REQUIRE( dummy.read_speed() == 6600 ); // 66s

            THEN( "they take longer to read all books" ) {
                CHECK( dummy.time_to_read( child, dummy ) > moves_easy );
                CHECK( dummy.time_to_read( western, dummy ) > moves_avg );
                CHECK( dummy.time_to_read( alpha, dummy ) > moves_hard );
            }
        }

        WHEN( "player has above average intelligence" ) {
            dummy.int_max = 10;
            REQUIRE( dummy.get_int() == 10 );
            REQUIRE( dummy.read_speed() == 5400 ); // 54s

            THEN( "they take less time to read books" ) {
                CHECK( dummy.time_to_read( child, dummy ) < moves_easy );
                CHECK( dummy.time_to_read( western, dummy ) < moves_avg );
                CHECK( dummy.time_to_read( alpha, dummy ) < moves_hard );
            }
        }
    }
}

TEST_CASE( "reasons for not being able to read", "[reading][reasons]" )
{
    // TODO: Check for unreachable NPC code after L256 of src/avatar.cpp : get_book_reader

    avatar dummy;
    std::vector<std::string> reasons;
    std::vector<std::string> expect_reasons;

    item &child = dummy.i_add( item( "child_book" ) );
    item &western = dummy.i_add( item( "novel_western" ) );
    item &alpha = dummy.i_add( item( "recipe_alpha" ) );

    SECTION( "you cannot read what is not readable" ) {
        item &rag = dummy.i_add( item( "rag" ) );

        CHECK( dummy.get_book_reader( rag, reasons ) == nullptr );
        expect_reasons = { "Your rag is not good reading material." };
        CHECK( reasons == expect_reasons );
    }

    SECTION( "you cannot read without enough light" ) {
        REQUIRE( dummy.fine_detail_vision_mod() > 4 );

        CHECK( dummy.get_book_reader( child, reasons ) == nullptr );
        expect_reasons = { "It's too dark to read!" };
        CHECK( reasons == expect_reasons );
    }

    GIVEN( "some identified books and plenty of light" ) {
        dummy.do_read( child );
        dummy.do_read( western );
        dummy.do_read( alpha );

        item &lamp = dummy.i_add( item( "atomic_lamp" ) );
        REQUIRE( dummy.fine_detail_vision_mod() == 1 );

        THEN( "you cannot read while illiterate" ) {
            dummy.toggle_trait( trait_ILLITERATE );
            REQUIRE( dummy.has_trait( trait_ILLITERATE ) );

            CHECK( dummy.get_book_reader( western, reasons ) == nullptr );
            expect_reasons = { "You're illiterate!" };
            CHECK( reasons == expect_reasons );
        }

        THEN( "you cannot read while farsighted without reading glasses" ) {
            dummy.toggle_trait( trait_HYPEROPIC );
            REQUIRE( dummy.has_trait( trait_HYPEROPIC ) );

            CHECK( dummy.get_book_reader( western, reasons ) == nullptr );
            expect_reasons = { "Your eyes won't focus without reading glasses." };
            CHECK( reasons == expect_reasons );
        }

        THEN( "you cannot read without enough skill to understand the book" ) {
            dummy.set_skill_level( skill_id( "cooking" ), 7 );

            CHECK( dummy.get_book_reader( alpha, reasons ) == nullptr );
            expect_reasons = { "cooking 8 needed to understand.  You have 7" };
            CHECK( reasons == expect_reasons );
        }

        THEN( "you cannot read boring books when your morale is too low" ) {
            dummy.add_morale( MORALE_FEELING_BAD, -50, -100 );
            REQUIRE( !dummy.has_morale_to_read() );

            CHECK( dummy.get_book_reader( alpha, reasons ) == nullptr );
            expect_reasons = { "What's the point of studying?  (Your morale is too low!)" };
            CHECK( reasons == expect_reasons );
        }

        /*
        THEN( "you cannot read while driving a vehicle" ) {
            const tripoint test_origin( 0, 0, 0 );
            vehicle *bike = g->m.add_vehicle( vproto_id( "bicycle" ), test_origin, 0, 0, 0 );
            bike->start_engines( true );
            REQUIRE( bike->player_in_control( dummy ) );

            dummy.get_book_reader( child, reasons );
            expect_reasons = { "It's a bad idea to read while driving!" };
            CHECK( reasons == expect_reasons );
        }
        */
    }
}

/*
 * Other tests to consider:
 *
 * - Special reading materials: guide books / maps, magic books, martial arts
 *
 * avatar::get_book_reader()
 * - Reading while driving
 * - Poor lighting
 * - NPC reading or listening, for fun or to learn
 * - NPC reading to deaf player
 *
 * in avatar::read()
 * - Trying to read martial arts books with low stamina
 *
 **/

