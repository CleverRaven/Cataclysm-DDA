#include <iosfwd>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "item.h"
#include "itype.h"
#include "morale_types.h"
#include "player_helpers.h"
#include "type_id.h"
#include "value_ptr.h"

class player;

static const trait_id trait_HATES_BOOKS( "HATES_BOOKS" );
static const trait_id trait_HYPEROPIC( "HYPEROPIC" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_LOVES_BOOKS( "LOVES_BOOKS" );
static const trait_id trait_SPIRITUAL( "SPIRITUAL" );

TEST_CASE( "clearing identified books", "[reading][book][identify][clear]" )
{
    item book( "child_book" );
    SECTION( "using local avatar" ) {
        avatar dummy;
        dummy.identify( book );
        dummy.clear_identified();
        REQUIRE_FALSE( dummy.has_identified( book.typeId() ) );
    }
    SECTION( "test helper clear_avatar() also clears items identified" ) {
        avatar &dummy = get_avatar();
        dummy.identify( book );
        clear_avatar();
        REQUIRE_FALSE( dummy.has_identified( book.typeId() ) );
    }
}

TEST_CASE( "identifying unread books", "[reading][book][identify]" )
{
    clear_avatar();
    Character &dummy = get_avatar();
    dummy.worn.emplace_back( "backpack" );

    GIVEN( "character has some unidentified books" ) {
        item &book1 = dummy.i_add( item( "novel_western" ) );
        item &book2 = dummy.i_add( item( "mag_throwing" ) );

        REQUIRE_FALSE( dummy.has_identified( book1.typeId() ) );
        REQUIRE_FALSE( dummy.has_identified( book2.typeId() ) );

        WHEN( "they read the books for the first time" ) {
            dummy.identify( book1 );
            dummy.identify( book2 );

            THEN( "the books should be identified" ) {
                CHECK( dummy.has_identified( book1.typeId() ) );
                CHECK( dummy.has_identified( book2.typeId() ) );
            }
        }
    }
}

TEST_CASE( "reading a book for fun", "[reading][book][fun]" )
{
    clear_avatar();
    Character &dummy = get_avatar();
    dummy.set_body();
    dummy.worn.emplace_back( "backpack" );

    GIVEN( "a fun book" ) {
        item &book = dummy.i_add( item( "novel_western" ) );
        REQUIRE( book.type->book );
        REQUIRE( book.type->book->fun > 0 );
        int book_fun = book.type->book->fun;

        WHEN( "character neither loves nor hates books" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_LOVES_BOOKS ) );
            REQUIRE_FALSE( dummy.has_trait( trait_HATES_BOOKS ) );

            THEN( "the book is a normal amount of fun" ) {
                CHECK( dummy.fun_to_read( book ) == true );
                CHECK( dummy.book_fun_for( book, dummy ) == book_fun );
            }
        }

        WHEN( "character loves books" ) {
            dummy.toggle_trait( trait_LOVES_BOOKS );
            REQUIRE( dummy.has_trait( trait_LOVES_BOOKS ) );

            THEN( "the book is extra fun" ) {
                CHECK( dummy.fun_to_read( book ) == true );
                CHECK( dummy.book_fun_for( book, dummy ) == book_fun + 1 );
            }
        }

        WHEN( "character hates books" ) {
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
        REQUIRE( book.has_flag( flag_id( "INSPIRATIONAL" ) ) );
        REQUIRE( book.type->book );
        REQUIRE( book.type->book->fun > 0 );
        int book_fun = book.type->book->fun;

        WHEN( "character is not spiritual" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_SPIRITUAL ) );

            THEN( "the book is a normal amount of fun" ) {
                CHECK( dummy.fun_to_read( book ) == true );
                CHECK( dummy.book_fun_for( book, dummy ) == book_fun );
            }
        }

        WHEN( "character is spiritual" ) {
            dummy.toggle_trait( trait_SPIRITUAL );
            REQUIRE( dummy.has_trait( trait_SPIRITUAL ) );

            THEN( "the book is thrice the fun" ) {
                CHECK( dummy.fun_to_read( book ) == true );
                CHECK( dummy.book_fun_for( book, dummy ) == book_fun * 3 );
            }
        }
    }
}

TEST_CASE( "character reading speed", "[reading][character][speed]" )
{
    clear_avatar();
    Character &dummy = get_avatar();
    dummy.worn.emplace_back( "backpack" );

    // Note: read_speed() returns number of moves;
    // 6000 == 60 seconds

    WHEN( "character has average intelligence" ) {
        REQUIRE( dummy.get_int() == 8 );

        THEN( "reading speed is normal" ) {
            CHECK( dummy.read_speed() == 6000 );
        }
    }

    WHEN( "character has below-average intelligence" ) {

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

    WHEN( "character has above-average intelligence" ) {

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
    dummy.worn.emplace_back( "backpack" );

    // Easy, medium, and hard books
    item &child = dummy.i_add( item( "child_book" ) );
    item &western = dummy.i_add( item( "novel_western" ) );
    item &alpha = dummy.i_add( item( "recipe_alpha" ) );

    // Ensure the books are actually books
    REQUIRE( child.type->book );
    REQUIRE( western.type->book );
    REQUIRE( alpha.type->book );

    // Convert time to read from minutes to moves, for easier comparison later
    int moves_child = child.type->book->time * to_moves<int>( 1_minutes );
    int moves_western = western.type->book->time * to_moves<int>( 1_minutes );
    int moves_alpha = alpha.type->book->time * to_moves<int>( 1_minutes );

    GIVEN( "some unidentified books and plenty of light" ) {
        REQUIRE_FALSE( dummy.has_identified( child.typeId() ) );
        REQUIRE_FALSE( dummy.has_identified( western.typeId() ) );

        // Get some light
        dummy.i_add( item( "atomic_lamp" ) );
        REQUIRE( dummy.fine_detail_vision_mod() == 1 );

        THEN( "identifying books takes 1/10th of the normal reading time" ) {
            CHECK( dummy.time_to_read( western, dummy ) == moves_western / 10 );
            CHECK( dummy.time_to_read( child, dummy ) == moves_child / 10 );
        }
    }

    GIVEN( "some identified books and plenty of light" ) {
        // Identify the books
        dummy.identify( child );
        dummy.identify( western );
        dummy.identify( alpha );
        REQUIRE( dummy.has_identified( child.typeId() ) );
        REQUIRE( dummy.has_identified( western.typeId() ) );
        REQUIRE( dummy.has_identified( alpha.typeId() ) );

        // Get some light
        dummy.i_add( item( "atomic_lamp" ) );
        REQUIRE( dummy.fine_detail_vision_mod() == 1 );

        WHEN( "player has average intelligence" ) {
            dummy.int_max = 8;
            REQUIRE( dummy.get_int() == 8 );
            REQUIRE( dummy.read_speed() == 6000 ); // 60s, "normal"

            THEN( "they can read books at their reading level in the normal amount time" ) {
                CHECK( dummy.time_to_read( child, dummy ) == moves_child );
                CHECK( dummy.time_to_read( western, dummy ) == moves_western );
            }
            AND_THEN( "they can read books above their reading level, but it takes longer" ) {
                CHECK( dummy.time_to_read( alpha, dummy ) > moves_alpha );
            }
        }

        WHEN( "player has below average intelligence" ) {
            dummy.int_max = 6;
            REQUIRE( dummy.get_int() == 6 );
            REQUIRE( dummy.read_speed() == 6600 ); // 66s

            THEN( "they take longer than average to read any book" ) {
                CHECK( dummy.time_to_read( child, dummy ) > moves_child );
                CHECK( dummy.time_to_read( western, dummy ) > moves_western );
                CHECK( dummy.time_to_read( alpha, dummy ) > moves_alpha );
            }
        }

        WHEN( "player has above average intelligence" ) {
            dummy.int_max = 10;
            REQUIRE( dummy.get_int() == 10 );
            REQUIRE( dummy.read_speed() == 5400 ); // 54s

            THEN( "they take less time than average to read any book" ) {
                CHECK( dummy.time_to_read( child, dummy ) < moves_child );
                CHECK( dummy.time_to_read( western, dummy ) < moves_western );
                CHECK( dummy.time_to_read( alpha, dummy ) < moves_alpha );
            }
        }
    }
}

TEST_CASE( "reasons for not being able to read", "[reading][reasons]" )
{
    avatar dummy;
    dummy.set_body();
    dummy.worn.emplace_back( "backpack" );
    std::vector<std::string> reasons;
    std::vector<std::string> expect_reasons;

    item &child = dummy.i_add( item( "child_book" ) );
    item &western = dummy.i_add( item( "novel_western" ) );
    item &alpha = dummy.i_add( item( "recipe_alpha" ) );

    SECTION( "you cannot read what is not readable" ) {
        item &rag = dummy.i_add( item( "rag" ) );
        REQUIRE_FALSE( rag.is_book() );

        CHECK( dummy.get_book_reader( rag, reasons ) == nullptr );
        expect_reasons = { "Your rag is not good reading material." };
        CHECK( reasons == expect_reasons );
    }

    SECTION( "you cannot read in darkness" ) {
        dummy.add_env_effect( efftype_id( "darkness" ), bodypart_id( "eyes" ), 3, 1_hours );
        REQUIRE( dummy.fine_detail_vision_mod() > 4 );

        CHECK( dummy.get_book_reader( child, reasons ) == nullptr );
        expect_reasons = { "It's too dark to read!" };
        CHECK( reasons == expect_reasons );
    }

    GIVEN( "some identified books and plenty of light" ) {
        // Identify the books
        dummy.identify( child );
        dummy.identify( western );
        dummy.identify( alpha );

        // Get some light
        dummy.i_add( item( "atomic_lamp" ) );
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
            dummy.set_skill_level( skill_id( "chemistry" ), 5 );

            CHECK( dummy.get_book_reader( alpha, reasons ) == nullptr );
            expect_reasons = { "applied science 6 needed to understand.  You have 5" };
            CHECK( reasons == expect_reasons );
        }

        THEN( "you cannot read boring books when your morale is too low" ) {
            dummy.add_morale( MORALE_FEELING_BAD, -50, -100 );
            REQUIRE_FALSE( dummy.has_morale_to_read() );

            CHECK( dummy.get_book_reader( alpha, reasons ) == nullptr );
            expect_reasons = { "What's the point of studying?  (Your morale is too low!)" };
            CHECK( reasons == expect_reasons );
        }

        WHEN( "there is nothing preventing you from reading" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_ILLITERATE ) );
            REQUIRE_FALSE( dummy.has_trait( trait_HYPEROPIC ) );
            REQUIRE_FALSE( dummy.in_vehicle );
            REQUIRE( dummy.has_morale_to_read() );

            THEN( "you can read!" ) {
                CHECK( dummy.get_book_reader( western, reasons ) != nullptr );
                expect_reasons = {};
                CHECK( reasons == expect_reasons );
            }
        }
    }
}

TEST_CASE( "determining book mastery", "[reading][book][mastery]" )
{
    static const auto book_has_skill = []( const item & book ) -> bool {
        REQUIRE( book.is_book() );
        return bool( book.type->book->skill );
    };

    avatar dummy;
    dummy.set_body();
    dummy.worn.emplace_back( "backpack" );

    item &child = dummy.i_add( item( "child_book" ) );
    item &alpha = dummy.i_add( item( "recipe_alpha" ) );

    SECTION( "you cannot determine mastery for non-book items" ) {
        item &rag = dummy.i_add( item( "rag" ) );
        REQUIRE_FALSE( rag.is_book() );
        CHECK( dummy.get_book_mastery( rag ) == book_mastery::CANT_DETERMINE );
    }
    SECTION( "you cannot determine mastery for unidentified books" ) {
        REQUIRE( alpha.is_book() );
        REQUIRE_FALSE( dummy.has_identified( alpha.typeId() ) );
        CHECK( dummy.get_book_mastery( child ) == book_mastery::CANT_DETERMINE );
    }
    GIVEN( "some identified books" ) {
        dummy.do_read( child );
        dummy.do_read( alpha );
        REQUIRE( dummy.has_identified( child.typeId() ) );
        REQUIRE( dummy.has_identified( alpha.typeId() ) );

        WHEN( "it gives/requires no skill" ) {
            REQUIRE_FALSE( book_has_skill( child ) );
            THEN( "you've already mastered it" ) {
                CHECK( dummy.get_book_mastery( child ) == book_mastery::MASTERED );
            }
        }
        WHEN( "it gives/requires skills" ) {
            REQUIRE( book_has_skill( alpha ) );

            THEN( "you won't understand it if your skills are too low" ) {
                dummy.set_skill_level( skill_id( "chemistry" ), 5 );
                CHECK( dummy.get_book_mastery( alpha ) == book_mastery::CANT_UNDERSTAND );
            }
            THEN( "you can learn from it with enough skill" ) {
                dummy.set_skill_level( skill_id( "chemistry" ), 6 );
                CHECK( dummy.get_book_mastery( alpha ) == book_mastery::LEARNING );
            }
            THEN( "you already mastered it if you have too much skill" ) {
                dummy.set_skill_level( skill_id( "chemistry" ), 7 );
                CHECK( dummy.get_book_mastery( alpha ) == book_mastery::MASTERED );
            }
        }
    }
}
