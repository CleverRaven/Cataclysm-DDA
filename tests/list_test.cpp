#include <algorithm> // std::find
#include <cstdio> // log redirection
#include <functional> // std::greater
#include <initializer_list>
#include <iterator>
#include <string>
#include <utility>
#include <vector> // range-insert testing

#include "cata_catch.h"
#include "colony_list_test_helpers.h"
#include "list.h"

TEST_CASE( "list_basics", "[list]" )
{
    {
        cata::list<int *> test_list;
        int ten = 10;
        int twenty = 20;

        SECTION( "empty()" ) {
            CHECK( test_list.empty() );

            test_list.push_front( &ten );

            CHECK( !test_list.empty() );
        }

        SECTION( "begin() and end()" ) {
            test_list.push_front( &ten );

            CHECK( **test_list.begin() == 10 );
            CHECK_FALSE( test_list.begin() == test_list.end() );

            test_list.clear();

            CHECK( test_list.begin() == test_list.end() );
        }

        for( int i = 0; i < 200; i++ ) {
            test_list.push_back( &ten );
            test_list.push_back( &twenty );
        }

        SECTION( "iterator count/access" ) {
            int count = 0;
            int sum = 0;
            for( int *&it : test_list ) {
                ++count;
                sum += *it;
            }

            CHECK( count == 400 );
            CHECK( sum == 6000 );
        }

        SECTION( "iterator distance" ) {
            cata::list<int *>::iterator plus_twenty = test_list.begin();
            std::advance( plus_twenty, 20 );

            cata::list<int *>::iterator plus_two_hundred = test_list.begin();
            std::advance( plus_two_hundred, 200 );

            CHECK( std::distance( test_list.begin(), plus_twenty ) == 20 );
            CHECK( std::distance( test_list.begin(), plus_two_hundred ) == 200 );
        }

        SECTION( "iterator next/prev" ) {
            cata::list<int *>::iterator next_iterator = std::next( test_list.begin(), 5 );
            cata::list<int *>::const_iterator prev_iterator = std::prev( test_list.cend(), 300 );

            CHECK( std::distance( test_list.begin(), next_iterator ) == 5 );
            CHECK( std::distance( prev_iterator, test_list.cend() ) == 300 );
        }

        SECTION( "iterator/const_iterator equality" ) {
            cata::list<int *>::const_iterator prev_iterator = std::prev( test_list.cend(), 300 );
            cata::list<int *>::iterator prev_iterator2 = std::prev( test_list.end(), 300 );

            CHECK( prev_iterator == prev_iterator2 );
        }

        SECTION( "copy, equality, and inequality" ) {
            cata::list<int *> test_list_2;
            test_list_2 = test_list;
            cata::list<int *> test_list_3( test_list );
            cata::list<int *> test_list_4( test_list_2, test_list_2.get_allocator() );

            cata::list<int *>::iterator it1 = test_list.begin();
            cata::list<int *>::const_iterator cit( it1 );

            CHECK( test_list_2.size() == 400 );
            CHECK( test_list_3.size() == 400 );
            CHECK( test_list_4.size() == 400 );

            CHECK( test_list == test_list_2 );
            CHECK( test_list_2 == test_list_3 );

            test_list_2.push_back( &ten );

            CHECK( test_list_2 != test_list_3 );
        }

        SECTION( "reverse iterator count/access" ) {
            int count = 0;
            int sum = 0;
            for( cata::list<int *>::reverse_iterator it = test_list.rbegin();
                 it != test_list.rend(); ++it ) {
                ++count;
                sum += **it;
            }

            CHECK( count == 400 );
            CHECK( sum == 6000 );
        }

        SECTION( "reverse iterator advance, next, and distance" ) {
            cata::list<int *>::reverse_iterator r_iterator = test_list.rbegin();
            std::advance( r_iterator, 50 );

            CHECK( std::distance( test_list.rbegin(), r_iterator ) == 50 );

            cata::list<int *>::reverse_iterator r_iterator2 = std::next( r_iterator, 2 );

            CHECK( std::distance( test_list.rbegin(), r_iterator2 ) == 52 );
        }

        SECTION( "multiple iteration" ) {
            int count = 0;
            int sum = 0;
            for( cata::list<int *>::iterator it = test_list.begin(); it != test_list.end();
                 std::advance( it, 2 ) ) {
                ++count;
                sum += **it;
            }

            CHECK( count == 200 );
            CHECK( sum == 2000 );
        }

        SECTION( "const iterator count/access" ) {
            int count = 0;
            int sum = 0;
            // NOLINTNEXTLINE(modernize-loop-convert)
            for( cata::list<int *>::const_iterator it = test_list.cbegin();
                 it != test_list.cend(); ++it ) {
                ++count;
                sum += **it;
            }

            CHECK( count == 400 );
            CHECK( sum == 6000 );
        }

        SECTION( "reverse iterator count/access" ) {
            int count = 0;
            int sum = 0;
            for( cata::list<int *>::const_reverse_iterator it = test_list.crbegin();
                 it != test_list.crend(); ++it ) {
                ++count;
                sum += **it;
            }

            CHECK( count == 400 );
            CHECK( sum == 6000 );
        }

        SECTION( "post erase iteration and shrink to fit" ) {
            int count = 0;
            for( cata::list<int *>::iterator it = std::next( test_list.begin() );
                 it != test_list.end(); ++it ) {
                ++count;
                it = test_list.erase( it );

                if( it == test_list.end() ) {
                    break;
                }
            }

            CHECK( count == 200 );
            CHECK( test_list.size() == 200 );

            const size_t prev_capacity = test_list.capacity();
            test_list.shrink_to_fit();

            CHECK( test_list.capacity() < prev_capacity );
            CHECK( test_list.capacity() == 200 );

            count = 0;
            for( cata::list<int *>::reverse_iterator it = test_list.rbegin();
                 it != test_list.rend(); ) {
                ++it;
                cata::list<int *>::iterator it2 = it.base(); // grabs it--, essentially
                test_list.erase( it2 );
                ++count;
            }

            CHECK( count == 200 );
            CHECK( test_list.empty() );
        }

        SECTION( "negative iteration" ) {
            int count = 0;
            for( cata::list<int *>::iterator it = std::prev( test_list.end() );
                 it != test_list.begin(); --it ) {
                ++count;
            }

            CHECK( count == 399 );
        }

        SECTION( "negative multiple iteration" ) {
            int count = 0;
            for( cata::list<int *>::iterator it = std::prev( test_list.end() );
                 it != test_list.begin() &&
                 it != std::next( test_list.begin() ); std::advance( it, -2 ) ) {
                ++count;
            }

            CHECK( count == 199 );
        }

        SECTION( "move" ) {
            cata::list<int *> test_list_2;
            test_list_2 = std::move( test_list );

            CHECK( test_list_2.size() == 400 );

            cata::list<int *> test_list_3( test_list_2 );
            cata::list<int *> test_list_4( std::move( test_list_3 ), test_list_2.get_allocator() );

            CHECK( test_list_4.size() == 400 );
        }

        SECTION( "swap() and max_size()" ) {
            cata::list<int *> test_list_2;
            // NOLINTNEXTLINE(bugprone-use-after-move)
            test_list_2 = test_list;

            CHECK( test_list_2.size() == 400 );

            test_list.push_back( &ten );

            test_list.swap( test_list_2 );

            CHECK( test_list.size() == test_list_2.size() - 1 );

            swap( test_list, test_list_2 );

            CHECK( test_list_2.size() == test_list.size() - 1 );

            CHECK( test_list_2.max_size() > test_list_2.size() );
        }
    }
}

TEST_CASE( "list_insert_and_erase", "[list]" )
{
    cata::list<int> test_list;

    for( int i = 0; i < 500000; i++ ) {
        test_list.push_back( i );
    }

    SECTION( "size after insert" ) {
        CHECK( test_list.size() == 500000 );
    }

    SECTION( "find iterator" ) {
        cata::list<int>::iterator found_item = std::find( test_list.begin(), test_list.end(), 5000 );

        CHECK( *found_item == 5000 );
    }

    SECTION( "find reverse iterator" ) {
        cata::list<int>::reverse_iterator found_item2 = std::find( test_list.rbegin(), test_list.rend(),
                5000 );

        CHECK( *found_item2 == 5000 );
    }

    SECTION( "erase alternating/randomly" ) {
        for( cata::list<int>::iterator it = test_list.begin(); it != test_list.end();
             ++it ) {
            it = test_list.erase( it );
        }

        CHECK( test_list.size() == 250000 );

        do {
            for( cata::list<int>::iterator it = test_list.begin(); it != test_list.end(); ) {
                if( ( xor_rand() & 7 ) == 0 ) {
                    it = test_list.erase( it );
                } else {
                    ++it;
                }
            }

        } while( !test_list.empty() );

        CHECK( test_list.empty() );
    }

    SECTION( "erase randomly till half empty" ) {
        size_t count = 0;
        do {
            for( cata::list<int>::iterator it = test_list.begin(); it != test_list.end(); ) {
                if( ( xor_rand() & 7 ) == 0 ) {
                    it = test_list.erase( it );
                    ++count;
                } else {
                    ++it;
                }
            }

        } while( count < 250000 );

        CHECK( test_list.size() == 500000 - count );

        for( size_t i = 0; i < count; i++ ) {
            test_list.push_front( 1 );
        }

        CHECK( test_list.size() == 500000 );
    }

    SECTION( "alternating insert/erase" ) {
        int count = 0;
        for( cata::list<int>::iterator it = test_list.begin(); it != test_list.end(); ) {
            if( ++count == 5 ) {
                count = 0;
                it = test_list.erase( it );
            } else {
                test_list.insert( it, 1 );
                ++it;
            }
        }

        CHECK( test_list.size() == 800000 );
    }

    test_list.clear();
    for( int i = 0; i < 500000; i++ ) {
        test_list.push_back( 1 );
    }

    SECTION( "large multi increment erasure" ) {
        cata::list<int>::iterator it = test_list.begin();
        std::advance( it, 250000 );

        for( ; it != test_list.end(); ) {
            it = test_list.erase( it );
        }

        CHECK( test_list.size() == 250000 );

        SECTION( "re-insert post heavy erasure" ) {
            for( int i = 0; i < 250000; i++ ) {
                test_list.push_front( 1 );
            }

            int sum = 0;
            for( int &it : test_list ) {
                sum += it;
            }

            CHECK( sum == 500000 );
        }
    }

    SECTION( "large multi decrement erasure" ) {
        cata::list<int>::iterator end_iterator = test_list.end();
        std::advance( end_iterator, -250000 );

        for( cata::list<int>::iterator it = test_list.begin(); it != end_iterator; ) {
            it = test_list.erase( it );
        }

        CHECK( test_list.size() == 250000 );

        SECTION( "re-insert post heavy erasure" ) {
            for( int i = 0; i < 250000; i++ ) {
                test_list.push_front( 1 );
            }

            int sum = 0;
            for( int &it : test_list ) {
                sum += it;
            }

            CHECK( sum == 500000 );
        }
    }

    SECTION( "erase from middle" ) {
        cata::list<int>::iterator end_iterator = test_list.end();
        std::advance( end_iterator, -50001 );
        cata::list<int>::iterator begin_iterator = test_list.begin();
        std::advance( begin_iterator, 300000 );

        for( cata::list<int>::iterator it = begin_iterator; it != end_iterator; ) {
            it = test_list.erase( it );
        }

        CHECK( test_list.size() == 350001 );
    }

    SECTION( "total erase edge case" ) {
        cata::list<int>::iterator temp_iterator = test_list.begin();
        std::advance( temp_iterator, 2 ); // Advance test 1

        test_list.erase( temp_iterator );
        // Check edge-case with advance when erasures present in initial group
        temp_iterator = test_list.begin();
        std::advance( temp_iterator, 500 );

        for( cata::list<int>::iterator it = test_list.begin(); it != test_list.end(); ) {
            it = test_list.erase( it );
        }

        CHECK( test_list.empty() );
    }

    SECTION( "multiple sequential small insert/erase" ) {
        test_list.clear();
        test_list.shrink_to_fit();

        const size_t prev_capacity = test_list.capacity();
        test_list.reserve( 1000 );

        CHECK( prev_capacity != test_list.capacity() );
        CHECK( test_list.capacity() == 1000 );

        size_t count = 0;
        for( int loop1 = 0; loop1 < 50000; loop1++ ) {
            for( int loop = 0; loop < 10; loop++ ) {
                if( ( xor_rand() & 7 ) == 0 ) {
                    test_list.push_back( 1 );
                    ++count;
                }
            }

            for( cata::list<int>::iterator it = test_list.begin(); it != test_list.end(); ) {
                if( ( xor_rand() & 7 ) == 0 ) {
                    it = test_list.erase( it );
                    --count;
                } else {
                    ++it;
                }
            }
        }

        CHECK( count == test_list.size() );
    }
}

TEST_CASE( "list_merge", "[list]" )
{
    cata::list<int> test_list;
    test_list.insert( test_list.end(), {1, 3, 5, 7, 9} );
    cata::list<int> test_list_2 = {2, 4, 6, 8, 10};

    test_list.merge( test_list_2 );

    bool passed = true;
    int count = 0;
    for( int &it : test_list ) {
        if( ++count != it ) {
            passed = false;
            break;
        }
    }

    CHECK( passed );
}

TEST_CASE( "list_splice", "[list]" )
{
    cata::list<int> test_list = {1, 2, 3, 4, 5};
    cata::list<int> test_list_2 = {6, 7, 8, 9, 10};

    SECTION( "splice at end" ) {
        test_list.splice( test_list.end(), test_list_2 );

        bool passed = true;
        int count = 0;
        for( int &it : test_list ) {
            if( ++count != it ) {
                passed = false;
                break;
            }
        }

        CHECK( passed );
    }

    SECTION( "splice at begin" ) {
        test_list.splice( test_list.begin(), test_list_2 );

        int count = 0;
        for( int &it : test_list ) {
            count += it;
        }

        CHECK( count == 55 );
    }

    SECTION( "splice past middle" ) {
        cata::list<int>::iterator it2 = test_list.begin();
        std::advance( it2, 3 );

        test_list.splice( it2, test_list_2 );

        int count = 0;
        for( int &it : test_list ) {
            count += it;
        }

        test_list.clear();
        test_list_2.clear();

        for( int i = 1; i < 25; i++ ) {
            test_list.push_back( i );
            test_list_2.push_back( i + 25 );
        }

        cata::list<int>::iterator it3 = test_list.begin();
        std::advance( it3, 18 );

        test_list.splice( it3, test_list_2 );

        count = 0;
        for( int &it : test_list ) {
            count += it;
        }

        CHECK( count == 1200 );
    }

    SECTION( "large splice" ) {
        test_list.clear();
        test_list_2.clear();

        for( int i = 5; i < 36; i++ ) {
            test_list.push_back( i );
            test_list_2.push_front( i );
        }

        test_list.splice( test_list.begin(), test_list_2 );

        int count = 0;
        for( int &it : test_list ) {
            count += it;
        }

        CHECK( count == 1240 );
    }
}

TEST_CASE( "list_sort_and_reverse", "[list]" )
{
    cata::list<int> test_list;

    for( int i = 0; i < 500; ++i ) {
        test_list.push_back( xor_rand() & 65535 );
    }

    SECTION( "less than (default)" ) {
        test_list.sort();

        bool passed = true;
        int previous = 0;
        for( int &it : test_list ) {
            if( it < previous ) {
                passed = false;
                break;
            }
            previous = it;
        }

        CHECK( passed );
    }

    SECTION( "greater than (predicate)" ) {
        test_list.sort( std::greater<>() );

        bool passed = true;
        int previous = 65535;
        for( int &it : test_list ) {
            if( it > previous ) {
                passed = false;
                break;
            }
            previous = it;
        }

        CHECK( passed );

        SECTION( "reverse" ) {
            test_list.reverse();

            passed = true;
            previous = 0;
            for( int &it : test_list ) {

                if( it < previous ) {
                    passed = false;
                    break;
                }

                previous = it;
            }

            CHECK( passed );
        }
    }
}

TEST_CASE( "list_unique", "[list]" )
{
    cata::list<int> test_list = {1, 1, 2, 3, 3, 4, 5, 5};

    SECTION( "control case" ) {
        bool passed = true;
        int previous = 0;
        for( int &it : test_list ) {
            if( it == previous ) {
                passed = false;
            }

            previous = it;
        }

        CHECK_FALSE( passed );
    }

    SECTION( "invoke unique" ) {
        test_list.unique();

        bool passed = true;
        int previous = 0;
        for( int &it : test_list ) {
            if( it == previous ) {
                passed = false;
                break;
            }

            previous = it;
        }

        CHECK( passed );
    }
}

TEST_CASE( "list_remove", "[list]" )
{
    cata::list<int> test_list = {1, 3, 1, 50, 16, 15, 2, 22};

    SECTION( "remove_if()" ) {
        test_list.remove_if( []( int value ) {
            return value > 15;
        } );

        bool passed = true;
        for( int &it : test_list ) {
            if( it > 15 ) {
                passed = false;
                break;
            }
        }

        CHECK( passed );
    }

    SECTION( "remove()" ) {
        test_list.remove( 1 );

        bool passed = true;
        for( int &it : test_list ) {
            if( it == 1 ) {
                passed = false;
                break;
            }
        }

        CHECK( passed );
    }

    SECTION( "remove() till empty" ) {
        test_list.remove( 1 );
        test_list.remove( 22 );
        test_list.remove( 15 );
        test_list.remove( 16 );
        test_list.remove( 3 );
        test_list.remove( 50 );
        test_list.remove( 2 );

        CHECK( test_list.empty() );
    }
}

TEST_CASE( "list_reserve", "[list]" )
{
    cata::list<int> test_list;

    test_list.reserve( 4097 );

    CHECK( test_list.capacity() >= 4097 );

    test_list.push_back( 15 );

    int count = 0;
    for( cata::list<int>::iterator it = test_list.begin(); it != test_list.end(); ++it ) {
        ++count;
    }

    CHECK( test_list.size() == static_cast<size_t>( count ) );

    test_list.insert( test_list.end(), 10000, 15 );

    count = 0;
    for( cata::list<int>::iterator it = test_list.begin(); it != test_list.end(); ++it ) {
        ++count;
    }

    CHECK( test_list.size() == 10001 );
    CHECK( count == 10001 );
    CHECK( test_list.capacity() >= 10001 );

    test_list.reserve( 15000 );

    CHECK( test_list.capacity() >= 15000 );
}

TEST_CASE( "list_resize", "[list]" )
{
    cata::list<int> test_list = { 1, 2, 3, 4, 5, 6, 7 };

    test_list.resize( 2 );

    int count = 0;
    for( cata::list<int>::iterator it = test_list.begin(); it != test_list.end(); ++it ) {
        ++count;
    }

    CHECK( test_list.size() == 2 );
    CHECK( count == 2 );
}

TEST_CASE( "list_assign", "[list]" )
{
    cata::list<int> test_list;

    SECTION( "range assign" ) {
        std::vector<int> test_vector = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        test_list.assign( test_vector.begin(), test_vector.end() );

        bool passed = true;
        int count = 0;
        for( int &it : test_list ) {
            if( ++count != it ) {
                passed = false;
                break;
            }
        }

        CHECK( test_list.size() == 10 );
        CHECK( count == 10 );
        CHECK( passed );
    }

    SECTION( "fill assign" ) {
        test_list.assign( 20, 1 );

        bool passed = true;
        int count = 0;
        for( int &it : test_list ) {
            if( it != 1 ) {
                passed = false;
                break;
            }
            ++count;
        }

        CHECK( test_list.size() == 20 );
        CHECK( count == 20 );
        CHECK( passed );
    }

    SECTION( "initializer list assign" ) {
        std::initializer_list<int> inlist = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

        test_list.assign( inlist );

        bool passed = true;
        int count = 11;
        for( int &it : test_list ) {
            if( --count != it ) {
                passed = false;
                break;
            }
        }

        CHECK( test_list.size() == 10 );
        CHECK( count == 1 );
        CHECK( passed );
    }
}

TEST_CASE( "list_insert", "[list]" )
{
    cata::list<int> test_list;

    std::vector<int> test_vector = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    test_list.insert( test_list.end(), test_vector.begin(), test_vector.end() );

    SECTION( "range insert" ) {

        bool passed = true;
        int count = 0;
        for( int &it : test_list ) {
            if( ++count != it ) {
                passed = false;
            }
        }

        CHECK( passed );
    }

    test_list.insert( test_list.begin(), 50, 50000 );

    SECTION( "fill insert" ) {
        int count = 0;
        for( cata::list<int>::iterator it = test_list.begin(); it != test_list.end(); ++it ) {
            ++count;
        }

        CHECK( count == 60 );
        CHECK( test_list.size() == 60 );
    }

    SECTION( "erase/insert randomly til empty" ) {
        while( !test_list.empty() ) {
            for( cata::list<int>::iterator it = test_list.begin(); it != test_list.end(); ) {
                if( ( xor_rand() & 15 ) == 0 ) {
                    test_list.insert( it, 13 );
                }

                if( ( xor_rand() & 7 ) == 0 ) {
                    it = test_list.erase( it );
                } else {
                    ++it;
                }
            }
        }

        CHECK( test_list.empty() );
    }
}

TEST_CASE( "list_emplace_move_copy_and_reverse_iterate", "[list]" )
{
    cata::list<small_struct> test_list;

    for( int counter = 0; counter < 254; counter++ ) {
        test_list.emplace_back( counter );
    }

    SECTION( "emplace_back() success" ) {
        bool passed = true;
        int count = 0;
        for( small_struct &it : test_list ) {
            if( count++ != it.number ) {
                passed = false;
                break;
            }
        }

        CHECK( passed );
    }

    SECTION( "emplace_back() return value" ) {
        small_struct &temp = test_list.emplace_back( 254 );
        CHECK( temp.number == 254 );
    }

    SECTION( "reverse iteration" ) {
        bool passed = true;
        int count = 254;
        for( cata::list<small_struct>::reverse_iterator rit = test_list.rbegin();
             rit != test_list.rend(); ++rit ) {
            if( --count != rit->number ) {
                passed = false;
                break;
            }
        }

        CHECK( passed );
    }

    for( int counter = -1; counter != -255; --counter ) {
        test_list.emplace_front( counter );
    }

    SECTION( "emplace_front()" ) {
        bool passed = true;
        int count = -255;
        for( small_struct &it : test_list ) {
            if( ++count != it.number ) {
                passed = false;
                break;
            }
        }

        CHECK( passed );
    }

    SECTION( "emplace_front() return value" ) {
        small_struct &temp = test_list.emplace_front( -255 );
        CHECK( temp.number == -255 );
    }

    SECTION( "reverse iteration 2" ) {
        bool passed = true;
        int count = 254;
        for( cata::list<small_struct>::reverse_iterator rit = test_list.rbegin();
             rit != test_list.rend(); ++rit ) {
            if( --count != rit->number ) {
                passed = false;
                break;
            }
        }

        CHECK( passed );
    }

    cata::list<small_struct> test_list_2( std::move( test_list ) );

    SECTION( "move constructor" ) {
        bool passed = true;
        int count = 254;
        for( cata::list<small_struct>::reverse_iterator rit = test_list_2.rbegin();
             rit != test_list_2.rend(); ++rit ) {
            if( --count != rit->number ) {
                passed = false;
                break;
            }
        }

        CHECK( passed );
        // NOLINTNEXTLINE(bugprone-use-after-move)
        CHECK( test_list.empty() );
    }

    SECTION( "emplace post-moved list" ) {
        // Reuse the moved list will cause segmentation fault
        // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
        test_list.emplace_back( 3 );
        CHECK( test_list.size() == 1 );
    }

    test_list = std::move( test_list_2 );

    SECTION( "move assignment" ) {
        bool passed = true;
        int count = 254;
        for( cata::list<small_struct>::reverse_iterator rit = test_list.rbegin();
             rit != test_list.rend(); ++rit ) {
            if( --count != rit->number ) {
                passed = false;
                break;
            }
        }

        CHECK( passed );
        // NOLINTNEXTLINE(bugprone-use-after-move)
        CHECK( test_list_2.empty() );
    }

    SECTION( "copy assignment" ) {
        test_list_2 = test_list;

        bool passed = true;
        int count = 254;
        for( cata::list<small_struct>::reverse_iterator rit = test_list_2.rbegin();
             rit != test_list_2.rend();
             ++rit ) {
            if( --count != rit->number ) {
                passed = false;
                break;
            }
        }

        CHECK( passed );
    }

    SECTION( "copy constructor" ) {
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        cata::list<small_struct> list3( test_list );

        bool passed = true;
        int count = 254;
        for( cata::list<small_struct>::reverse_iterator rit = list3.rbegin();
             rit != list3.rend(); ++rit ) {
            if( --count != rit->number ) {
                passed = false;
                break;
            }
        }

        CHECK( passed );
    }
}

TEST_CASE( "list_reorder", "[list]" )
{
    cata::list<int> test_list;

    for( int i = 0; i < 255; ++i ) {
        test_list.push_back( i );
    }

    // Used for the post reorder data consistency test
    int original_sum = 0;
    for( int &it : test_list ) {
        original_sum += it;
    }

    cata::list<int>::iterator it1 = test_list.begin();
    cata::list<int>::iterator it2 = test_list.begin();
    cata::list<int>::iterator it3 = test_list.begin();

    std::advance( it1, 25 );
    std::advance( it2, 5 );

    test_list.reorder( it2, it1 );

    it1 = test_list.begin();
    std::advance( it1, 5 );

    SECTION( "single reorder" ) {
        CHECK( *it1 == 25 );
    }

    it1 = test_list.begin();
    std::advance( it1, 152 );

    test_list.reorder( test_list.begin(), it1 );

    SECTION( "single reorder to begin" ) {
        CHECK( test_list.front() == 152 );
    }

    test_list.reorder( test_list.end(), it2 );

    SECTION( "single reorder to end" ) {
        it1 = std::prev( test_list.end() );
        CHECK( *it1 == 5 );
    }

    it1 = test_list.begin();
    it2 = test_list.begin();

    std::advance( it1, 50 );
    std::advance( it2, 60 );
    std::advance( it3, 70 );

    test_list.reorder( it3, it1, it2 );

    SECTION( "range reorder" ) {
        it3 = test_list.begin();
        std::advance( it3, 60 );

        bool passed = true;
        for( int test = 50; test < 60; test++ ) {
            if( *it3 != test ) {
                passed = false;
            }
            ++it3;
        }

        CHECK( passed == true );
    }

    it1 = test_list.begin();
    it2 = test_list.begin();

    std::advance( it1, 80 );
    std::advance( it2, 120 );

    test_list.reorder( test_list.end(), it1, it2 );

    SECTION( "range reorer to end" ) {
        it3 = test_list.end();
        std::advance( it3, -41 );

        bool passed = true;
        for( int test = 80; test < 120; test++ ) {
            if( *it3 != test ) {
                passed = false;
            }
            ++it3;
        }

        CHECK( passed == true );
    }

    it1 = test_list.begin();
    it2 = test_list.begin();

    std::advance( it1, 40 );
    std::advance( it2, 45 );

    test_list.reorder( test_list.begin(), it1, it2 );

    SECTION( "range reorder to begin" ) {
        it3 = test_list.begin();

        bool passed = true;
        for( int test = 40; test < 45; test++ ) {
            if( *it3 != test ) {
                passed = false;
            }
            ++it3;
        }

        CHECK( passed == true );
    }

    SECTION( "post reorder data consistency" ) {
        int sum = 0;
        for( it1 = test_list.begin(); it1 != test_list.end(); ++it1 ) {
            sum += *it1;
        }

        CHECK( sum == original_sum );
    }
}

TEST_CASE( "list_insertion_styles", "[list]" )
{
    cata::list<int> test_list = {1, 2, 3};

    CHECK( test_list.size() == 3 );

    cata::list<int> test_list_2( test_list.begin(), test_list.end() );

    CHECK( test_list_2.size() == 3 );

    cata::list<int> test_list_3( 5000, 2 );

    CHECK( test_list_3.size() == 5000 );

    test_list_2.insert( test_list_2.end(), 500000, 5 );

    CHECK( test_list_2.size() == 500003 );

    std::vector<int> some_ints( 500, 2 );

    test_list_2.insert( test_list_2.begin(), some_ints.begin(), some_ints.end() );

    CHECK( test_list_2.size() == 500503 );
}

TEST_CASE( "list_perfect_forwarding", "[list]" )
{
    cata::list<perfect_forwarding_test> test_list;

    int lvalue = 0;
    int &lvalueref = lvalue;

    test_list.emplace( test_list.end(), 7, lvalueref );

    CHECK( ( *test_list.begin() ).success );
    CHECK( lvalueref == 1 );
}
