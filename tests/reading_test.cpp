#include <iosfwd>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "activity_actor_definitions.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "item.h"
#include "itype.h"
#include "map_helpers.h"
#include "morale_types.h"
#include "player_helpers.h"
#include "skill.h"
#include "type_id.h"
#include "value_ptr.h"

static const activity_id ACT_READ( "ACT_READ" );

static const efftype_id effect_darkness( "darkness" );

static const flag_id json_flag_INSPIRATIONAL( "INSPIRATIONAL" );

static const limb_score_id limb_score_vision( "vision" );

static const skill_id skill_chemistry( "chemistry" );

static const trait_id trait_HATES_BOOKS( "HATES_BOOKS" );
static const trait_id trait_HYPEROPIC( "HYPEROPIC" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_LOVES_BOOKS( "LOVES_BOOKS" );
static const trait_id trait_SPIRITUAL( "SPIRITUAL" );

TEST_CASE( "clearing_identified_books", "[reading][book][identify][clear]" )
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

TEST_CASE( "identifying_unread_books", "[reading][book][identify]" )
{
    clear_avatar();
    Character &dummy = get_avatar();
    dummy.worn.wear_item( dummy, item( "backpack" ), false, false );

    GIVEN( "character has some unidentified books" ) {
        item_location book1 = dummy.i_add( item( "novel_western" ) );
        item_location book2 = dummy.i_add( item( "mag_throwing" ) );

        REQUIRE_FALSE( dummy.has_identified( book1->typeId() ) );
        REQUIRE_FALSE( dummy.has_identified( book2->typeId() ) );

        WHEN( "they read the books for the first time" ) {
            dummy.identify( *book1 );
            dummy.identify( *book2 );

            THEN( "the books should be identified" ) {
                CHECK( dummy.has_identified( book1->typeId() ) );
                CHECK( dummy.has_identified( book2->typeId() ) );
            }
        }
    }
}

TEST_CASE( "reading_a_book_for_fun", "[reading][book][fun]" )
{
    clear_avatar();
    Character &dummy = get_avatar();
    dummy.set_body();
    dummy.worn.wear_item( dummy, item( "backpack" ), false, false );

    GIVEN( "a fun book" ) {
        item_location book = dummy.i_add( item( "novel_western" ) );
        REQUIRE( book->type->book );
        REQUIRE( book->type->book->fun > 0 );
        int book_fun = book->type->book->fun;

        WHEN( "character neither loves nor hates books" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_LOVES_BOOKS ) );
            REQUIRE_FALSE( dummy.has_trait( trait_HATES_BOOKS ) );

            THEN( "the book is a normal amount of fun" ) {
                CHECK( dummy.fun_to_read( *book ) );
                CHECK( dummy.book_fun_for( *book, dummy ) == book_fun );
            }
        }

        WHEN( "character loves books" ) {
            dummy.toggle_trait( trait_LOVES_BOOKS );
            REQUIRE( dummy.has_trait( trait_LOVES_BOOKS ) );

            THEN( "the book is extra fun" ) {
                CHECK( dummy.fun_to_read( *book ) );
                CHECK( dummy.book_fun_for( *book, dummy ) == book_fun + 1 );
            }
        }

        WHEN( "character hates books" ) {
            dummy.toggle_trait( trait_HATES_BOOKS );
            REQUIRE( dummy.has_trait( trait_HATES_BOOKS ) );

            THEN( "the book is no fun at all" ) {
                CHECK_FALSE( dummy.fun_to_read( *book ) );
                CHECK( dummy.book_fun_for( *book, dummy ) == 0 );
            }
        }
    }

    GIVEN( "a fun book that is also inspirational" ) {
        item_location book = dummy.i_add( item( "holybook_pastafarian" ) );
        REQUIRE( book->has_flag( json_flag_INSPIRATIONAL ) );
        REQUIRE( book->type->book );
        REQUIRE( book->type->book->fun > 0 );
        int book_fun = book->type->book->fun;

        WHEN( "character is not spiritual" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_SPIRITUAL ) );

            THEN( "the book is a normal amount of fun" ) {
                CHECK( dummy.fun_to_read( *book ) );
                CHECK( dummy.book_fun_for( *book, dummy ) == book_fun );
            }
        }

        WHEN( "character is spiritual" ) {
            dummy.toggle_trait( trait_SPIRITUAL );
            REQUIRE( dummy.has_trait( trait_SPIRITUAL ) );

            THEN( "the book is thrice the fun" ) {
                CHECK( dummy.fun_to_read( *book ) );
                CHECK( dummy.book_fun_for( *book, dummy ) == book_fun * 3 );
            }
        }
    }
}

TEST_CASE( "character_reading_speed", "[reading][character][speed]" )
{
    clear_avatar();
    Character &dummy = get_avatar();
    dummy.worn.wear_item( dummy, item( "backpack" ), false, false );

    // Note: read_speed() returns number of moves;
    // 6000 == 60 seconds

    WHEN( "character has average intelligence" ) {
        REQUIRE( dummy.get_int() == 8 );

        THEN( "reading speed is normal" ) {
            CHECK( dummy.read_speed() * 60 == 6000 );
        }
    }

    WHEN( "character has below-average intelligence" ) {

        THEN( "reading speed gets slower as intelligence decreases" ) {
            dummy.int_max = 7;
            CHECK( dummy.read_speed() * 60 == 6180 );
            dummy.int_max = 6;
            CHECK( dummy.read_speed() * 60 == 6480 );
            dummy.int_max = 5;
            CHECK( dummy.read_speed() * 60 == 6780 );
            dummy.int_max = 4;
            CHECK( dummy.read_speed() * 60 == 7200 );
        }
    }

    WHEN( "character has above-average intelligence" ) {

        THEN( "reading speed gets faster as intelligence increases" ) {
            dummy.int_max = 9;
            CHECK( dummy.read_speed() * 60 == 5700 );
            dummy.int_max = 10;
            CHECK( dummy.read_speed() * 60 == 5460 );
            dummy.int_max = 12;
            CHECK( dummy.read_speed() * 60 == 5100 );
            dummy.int_max = 14;
            CHECK( dummy.read_speed() * 60 == 4800 );
        }
    }
}

TEST_CASE( "estimated_reading_time_for_a_book", "[reading][book][time]" )
{
    avatar dummy;
    //Give eyes to our dummy
    dummy.set_body();
    REQUIRE( dummy.has_part( bodypart_id( "eyes" ) ) );
    REQUIRE( dummy.get_limb_score( limb_score_vision ) != 0 );

    dummy.worn.wear_item( dummy, item( "backpack" ), false, false );

    // Easy, medium, and hard books
    item_location child = dummy.i_add( item( "child_book" ) );
    item_location western = dummy.i_add( item( "novel_western" ) );
    item_location alpha = dummy.i_add( item( "recipe_alpha" ) );

    // Ensure the books are actually books
    REQUIRE( child->type->book );
    REQUIRE( western->type->book );
    REQUIRE( alpha->type->book );

    // Convert time to read from minutes to moves, for easier comparison later
    time_duration moves_child = child->type->book->time;
    time_duration moves_western = western->type->book->time;
    time_duration moves_alpha = alpha->type->book->time;

    GIVEN( "some unidentified books and plenty of light" ) {
        REQUIRE_FALSE( dummy.has_identified( child->typeId() ) );
        REQUIRE_FALSE( dummy.has_identified( western->typeId() ) );

        // Get some light
        dummy.i_add( item( "atomic_lamp" ) );
        REQUIRE( dummy.fine_detail_vision_mod() == 1 );

        THEN( "identifying books takes 1/10th of the normal reading time" ) {
            CHECK( dummy.time_to_read( *western, dummy ) == moves_western / 10 );
            CHECK( dummy.time_to_read( *child, dummy ) == moves_child / 10 );
        }
    }

    GIVEN( "some identified books and plenty of light" ) {
        // Identify the books
        dummy.identify( *child );
        dummy.identify( *western );
        dummy.identify( *alpha );
        REQUIRE( dummy.has_identified( child->typeId() ) );
        REQUIRE( dummy.has_identified( western->typeId() ) );
        REQUIRE( dummy.has_identified( alpha->typeId() ) );

        // Get some light
        dummy.i_add( item( "atomic_lamp" ) );
        REQUIRE( dummy.fine_detail_vision_mod() == 1 );

        WHEN( "player has average intelligence" ) {
            dummy.int_max = 8;
            REQUIRE( dummy.get_int() == 8 );
            REQUIRE( dummy.read_speed() * 60 == 6000 ); // 60s, "normal"

            THEN( "they can read books at their reading level in the normal amount time" ) {
                CHECK( dummy.time_to_read( *child, dummy ) == moves_child );
                CHECK( dummy.time_to_read( *western, dummy ) == moves_western );
            }
            AND_THEN( "they can read books above their reading level, but it takes longer" ) {
                CHECK( dummy.time_to_read( *alpha, dummy ) > moves_alpha );
            }
        }

        WHEN( "player has below average intelligence" ) {
            dummy.int_max = 6;
            REQUIRE( dummy.get_int() == 6 );
            REQUIRE( dummy.read_speed() * 60 == 6480 ); // 65s

            THEN( "they take longer than average to read any book" ) {
                CHECK( dummy.time_to_read( *child, dummy ) > moves_child );
                CHECK( dummy.time_to_read( *western, dummy ) > moves_western );
                CHECK( dummy.time_to_read( *alpha, dummy ) > moves_alpha );
            }
        }

        WHEN( "player has above average intelligence" ) {
            dummy.int_max = 10;
            REQUIRE( dummy.get_int() == 10 );
            REQUIRE( dummy.read_speed() * 60 == 5460 ); // 55s

            THEN( "they take less time than average to read any book" ) {
                CHECK( dummy.time_to_read( *child, dummy ) < moves_child );
                CHECK( dummy.time_to_read( *western, dummy ) < moves_western );
                CHECK( dummy.time_to_read( *alpha, dummy ) < moves_alpha );
            }
        }
    }
}

TEST_CASE( "reasons_for_not_being_able_to_read", "[reading][reasons]" )
{
    clear_avatar();
    clear_map();
    Character &dummy = get_avatar();
    dummy.set_body();
    dummy.worn.wear_item( dummy, item( "backpack" ), false, false );
    set_time_to_day();
    std::vector<std::string> reasons;
    std::vector<std::string> expect_reasons;

    item_location child = dummy.i_add( item( "child_book" ) );
    item_location western = dummy.i_add( item( "novel_western" ) );
    item_location alpha = dummy.i_add( item( "recipe_alpha" ) );

    SECTION( "you cannot read what is not readable" ) {
        item_location sheet_cotton = dummy.i_add( item( "sheet_cotton" ) );
        REQUIRE_FALSE( sheet_cotton->is_book() );

        CHECK( dummy.get_book_reader( *sheet_cotton, reasons ) == nullptr );
        expect_reasons = { "Your cotton sheet is not good reading material." };
        CHECK( reasons == expect_reasons );
    }

    SECTION( "you cannot read in darkness" ) {
        if( GENERATE( true, false ) ) {
            dummy.add_env_effect( effect_darkness, bodypart_id( "eyes" ), 3, 1_hours );
            CAPTURE( "darkness effect" );
        } else {
            set_time( calendar::turn - time_past_midnight( calendar::turn ) );
            CAPTURE( "actual darkness" );
        }
        REQUIRE( dummy.fine_detail_vision_mod() > 4 );

        CHECK( dummy.get_book_reader( *child, reasons ) == nullptr );
        expect_reasons = { "It's too dark to read!" };
        CHECK( reasons == expect_reasons );
        set_time_to_day();
    }

    GIVEN( "some identified books and plenty of light" ) {
        // Identify the books
        dummy.identify( *child );
        dummy.identify( *western );
        dummy.identify( *alpha );

        // Get some light
        dummy.i_add( item( "atomic_lamp" ) );
        REQUIRE( dummy.fine_detail_vision_mod() == 1 );

        THEN( "you cannot read while illiterate" ) {
            dummy.toggle_trait( trait_ILLITERATE );
            REQUIRE( dummy.has_trait( trait_ILLITERATE ) );

            CHECK( dummy.get_book_reader( *western, reasons ) == nullptr );
            expect_reasons = { "You're illiterate!" };
            CHECK( reasons == expect_reasons );
        }

        THEN( "you cannot read while farsighted without reading glasses" ) {
            dummy.toggle_trait( trait_HYPEROPIC );
            REQUIRE( dummy.has_trait( trait_HYPEROPIC ) );

            CHECK( dummy.get_book_reader( *western, reasons ) == nullptr );
            expect_reasons = { "Your eyes won't focus without reading glasses." };
            CHECK( reasons == expect_reasons );
        }

        THEN( "you cannot read without enough skill to understand the book" ) {
            dummy.set_knowledge_level( skill_chemistry, 5 );

            CHECK( dummy.get_book_reader( *alpha, reasons ) == nullptr );
            expect_reasons = { "applied science 6 needed to understand.  You have 5" };
            CHECK( reasons == expect_reasons );
        }

        THEN( "you cannot read boring books when your morale is too low" ) {
            dummy.add_morale( MORALE_FEELING_BAD, -50, -100 );
            REQUIRE_FALSE( dummy.has_morale_to_read() );

            CHECK( dummy.get_book_reader( *alpha, reasons ) == nullptr );
            expect_reasons = { "What's the point of studying?  (Your morale is too low!)" };
            CHECK( reasons == expect_reasons );
        }

        WHEN( "there is nothing preventing you from reading" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_ILLITERATE ) );
            REQUIRE_FALSE( dummy.has_trait( trait_HYPEROPIC ) );
            REQUIRE_FALSE( dummy.in_vehicle );
            REQUIRE( dummy.has_morale_to_read() );

            THEN( "you can read!" ) {
                CHECK( dummy.get_book_reader( *western, reasons ) != nullptr );
                expect_reasons = {};
                CHECK( reasons == expect_reasons );
            }
        }
    }
}

TEST_CASE( "determining_book_mastery", "[reading][book][mastery]" )
{
    static const auto book_has_skill = []( const item & book ) -> bool {
        REQUIRE( book.is_book() );
        return bool( book.type->book->skill );
    };

    avatar dummy;
    dummy.set_body();
    dummy.worn.wear_item( dummy, item( "backpack" ), false, false );

    item_location child = dummy.i_add( item( "child_book" ) );
    item_location alpha = dummy.i_add( item( "recipe_alpha" ) );

    SECTION( "you cannot determine mastery for non-book items" ) {
        item_location sheet_cotton = dummy.i_add( item( "sheet_cotton" ) );
        REQUIRE_FALSE( sheet_cotton->is_book() );
        CHECK( dummy.get_book_mastery( *sheet_cotton ) == book_mastery::CANT_DETERMINE );
    }
    SECTION( "you cannot determine mastery for unidentified books" ) {
        REQUIRE( alpha->is_book() );
        REQUIRE_FALSE( dummy.has_identified( alpha->typeId() ) );
        CHECK( dummy.get_book_mastery( *child ) == book_mastery::CANT_DETERMINE );
    }
    GIVEN( "some identified books" ) {
        dummy.identify( *child );
        dummy.identify( *alpha );
        REQUIRE( dummy.has_identified( child->typeId() ) );
        REQUIRE( dummy.has_identified( alpha->typeId() ) );

        WHEN( "it gives/requires no skill" ) {
            REQUIRE_FALSE( book_has_skill( *child ) );
            THEN( "you've already mastered it" ) {
                CHECK( dummy.get_book_mastery( *child ) == book_mastery::MASTERED );
            }
        }
        WHEN( "it gives/requires skills" ) {
            REQUIRE( book_has_skill( *alpha ) );

            THEN( "you won't understand it if your skills are too low" ) {
                dummy.set_knowledge_level( skill_chemistry, 5 );
                CHECK( dummy.get_book_mastery( *alpha ) == book_mastery::CANT_UNDERSTAND );
            }
            THEN( "you can learn from it with enough skill" ) {
                dummy.set_knowledge_level( skill_chemistry, 6 );
                CHECK( dummy.get_book_mastery( *alpha ) == book_mastery::LEARNING );
            }
            THEN( "you already mastered it if you have too much skill" ) {
                dummy.set_knowledge_level( skill_chemistry, 7 );
                CHECK( dummy.get_book_mastery( *alpha ) == book_mastery::MASTERED );
            }
        }
    }
}

TEST_CASE( "reading_a_book_for_skill", "[reading][book][skill]" )
{
    clear_avatar();
    Character &dummy = get_avatar();
    dummy.set_body();
    dummy.worn.wear_item( dummy, item( "backpack" ), false, false );

    item_location alpha = dummy.i_add( item( "recipe_alpha" ) );
    REQUIRE( alpha->is_book() );

    dummy.identify( *alpha );
    REQUIRE( dummy.has_identified( alpha->typeId() ) );

    GIVEN( "a book you can learn from" ) {
        dummy.set_knowledge_level( skill_chemistry, 6 );
        REQUIRE( dummy.get_book_mastery( *alpha ) == book_mastery::LEARNING );

        dummy.set_focus( 100 );
        WHEN( "reading the book 100 times" ) {
            const cata::value_ptr<islot_book> bkalpha_islot = alpha->type->book;
            SkillLevel &avatarskill = dummy.get_skill_level_object( bkalpha_islot->skill );

            for( int i = 0; i < 100; ++i ) {
                read_activity_actor::read_book( *dummy.as_character(), bkalpha_islot, avatarskill, 1.0 );
            }

            THEN( "gained a skill level" ) {
                CHECK( dummy.get_knowledge_level( skill_chemistry ) > 6 );
                CHECK( static_cast<int>( dummy.get_skill_level( skill_chemistry ) ) < 6 );
                CHECK( dummy.get_book_mastery( *alpha ) == book_mastery::MASTERED );
            }
        }
    }
}

TEST_CASE( "reading_a_book_with_an_ebook_reader", "[reading][book][ereader]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();

    WHEN( "reading a book" ) {

        dummy.worn.wear_item( dummy, item( "backpack" ), false, false );
        dummy.i_add( item( "atomic_lamp" ) );
        REQUIRE( dummy.fine_detail_vision_mod() == 1 );

        item_location ereader = dummy.i_add( item( "test_ebook_reader" ) );

        item book{"test_textbook_fabrication"};
        ereader->put_in( book, pocket_type::EBOOK );

        item battery( "test_battery_disposable" );
        battery.ammo_set( battery.ammo_default(), 100 );
        ereader->put_in( battery, pocket_type::MAGAZINE_WELL );

        THEN( "player can read the book" ) {

            item_location booklc{dummy, &book};
            read_activity_actor actor( dummy.time_to_read( *booklc, dummy ), booklc, ereader, true );
            dummy.activity = player_activity( actor );

            REQUIRE( ereader->ammo_remaining() == 100 );

            dummy.activity.start_or_resume( dummy, false );
            REQUIRE( dummy.activity.id() == ACT_READ );

            CHECK( ereader->ammo_remaining() == 99 );

            dummy.activity.do_turn( dummy );

            CHECK( dummy.activity.id() == ACT_READ );

            AND_THEN( "ereader has spent a charge while reading" ) {
                CHECK( ereader->ammo_remaining() == 98 );

                AND_THEN( "ereader runs out of battery" ) {
                    ereader->ammo_consume( ereader->ammo_remaining(), dummy.pos(), &dummy );
                    dummy.activity.do_turn( dummy );

                    THEN( "reading stops" ) {
                        CHECK( dummy.activity.id() != ACT_READ );
                    }
                }
            }
        }
    }
}
