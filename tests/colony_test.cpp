#include <cstddef>
#include <algorithm> // std::find
#include <functional> // std::greater
#include <utility> // std::move
#include <vector> // range-insert testing

#include "catch/catch.hpp"
#include "colony.h"
#include "colony_list_test_helpers.h"

TEST_CASE( "colony basics", "[colony]" )
{
    cata::colony<int *> test_colony;

    // Starts empty
    CHECK( test_colony.empty() );

    int ten = 10;
    test_colony.insert( &ten );

    // Not empty after insert
    CHECK_FALSE( test_colony.empty() );
    // Begin working
    CHECK( **test_colony.begin() == 10 );
    // End working
    CHECK_FALSE( test_colony.begin() == test_colony.end() );

    test_colony.clear();

    // Begin = End after clear
    CHECK( test_colony.begin() == test_colony.end() );

    int twenty = 20;
    for( int temp = 0; temp < 200; ++temp ) {
        test_colony.insert( &ten );
        test_colony.insert( &twenty );
    }

    int count = 0;
    int sum = 0;
    for( cata::colony<int *>::iterator it = test_colony.begin(); it < test_colony.end(); ++it ) {
        ++count;
        sum += **it;
    }

    // Iterator count
    CHECK( count == 400 );
    // Iterator access
    CHECK( sum == 6000 );

    cata::colony<int *>::iterator plus_20 = test_colony.begin();
    cata::colony<int *>::iterator plus_200 = test_colony.begin();
    test_colony.advance( plus_20, 20 );
    test_colony.advance( plus_200, 200 );

    // Iterator + distance
    CHECK( test_colony.distance( test_colony.begin(), plus_20 ) == 20 );
    // Iterator - distance
    CHECK( test_colony.distance( plus_200, test_colony.begin() ) == -200 );

    cata::colony<int *>::iterator next_iterator = test_colony.next( test_colony.begin(), 5 );
    cata::colony<int *>::const_iterator cprev_iterator = test_colony.prev( test_colony.cend(), 300 );

    // Iterator next
    CHECK( test_colony.distance( test_colony.begin(), next_iterator ) == 5 );
    // Const iterator prev
    CHECK( test_colony.distance( test_colony.cend(), cprev_iterator ) == -300 );

    cata::colony<int *>::iterator prev_iterator = test_colony.prev( test_colony.end(), 300 );

    // Iterator/const iterator equality operator
    CHECK( cprev_iterator == prev_iterator );

    cata::colony<int *> test_colony_2;
    test_colony_2 = test_colony;

    cata::colony<int *> test_colony_3( test_colony );
    cata::colony<int *> test_colony_4( test_colony_2, test_colony_2.get_allocator() );

    // Copy
    CHECK( test_colony_2.size() == 400 );
    // Copy construct
    CHECK( test_colony_3.size() == 400 );
    // Allocator-extended copy construct
    CHECK( test_colony_4.size() == 400 );

    // Equality operator
    CHECK( test_colony == test_colony_2 );
    CHECK( test_colony_2 == test_colony_3 );
    CHECK( test_colony_3 == test_colony_4 );

    test_colony_2.insert( &ten );

    // Inequality operator
    CHECK( test_colony_2 != test_colony_3 );

    count = 0;
    sum = 0;
    for( cata::colony<int *>::reverse_iterator it = test_colony.rbegin(); it != test_colony.rend();
         ++it ) {
        ++count;
        sum += **it;
    }

    // Reverse iterator count
    CHECK( count == 400 );
    // Reverse iterator access
    CHECK( sum == 6000 );

    cata::colony<int *>::reverse_iterator r_iterator = test_colony.rbegin();
    test_colony.advance( r_iterator, 50 );

    // Reverse iterator advance and distance test
    CHECK( test_colony.distance( test_colony.rbegin(), r_iterator ) == 50 );

    cata::colony<int *>::reverse_iterator r_iterator2 = test_colony.next( r_iterator, 2 );

    // Reverse iterator next and distance test
    CHECK( test_colony.distance( test_colony.rbegin(), r_iterator2 ) == 52 );

    count = 0;
    sum = 0;
    for( cata::colony<int *>::iterator it = test_colony.begin(); it < test_colony.end();
         test_colony.advance( it, 2 ) ) {
        ++count;
        sum += **it;
    }

    // Multiple iteration count
    CHECK( count == 200 );
    // Multiple iteration access
    CHECK( sum == 2000 );

    count = 0;
    sum = 0;
    for( int *p : test_colony ) {
        ++count;
        sum += *p;
    }

    // Constant iterator count
    CHECK( count == 400 );
    // Constant iterator access
    CHECK( sum == 6000 );

    count = 0;
    sum = 0;
    for( cata::colony<int *>::const_reverse_iterator it = --cata::colony<int *>::const_reverse_iterator(
                test_colony.crend() ); it != cata::colony<int *>::const_reverse_iterator( test_colony.crbegin() );
         --it ) {
        ++count;
        sum += **it;
    }

    // Const_reverse_iterator -- count
    CHECK( count == 399 );
    //Const_reverse_iterator -- access
    CHECK( sum == 5980 );

    count = 0;
    for( cata::colony<int *>::iterator it = ++cata::colony<int *>::iterator( test_colony.begin() );
         it < test_colony.end(); ++it ) {
        ++count;
        it = test_colony.erase( it );
    }

    // Partial erase iteration test
    CHECK( count == 200 );
    // Post-erase size test
    CHECK( test_colony.size() == 200 );

    const unsigned int temp_capacity = static_cast<unsigned int>( test_colony.capacity() );
    test_colony.shrink_to_fit();
    // Shrink_to_fit test
    CHECK( test_colony.capacity() < temp_capacity );
    CHECK( test_colony.capacity() == 200 );

    count = 0;
    for( cata::colony<int *>::reverse_iterator it = test_colony.rbegin(); it != test_colony.rend();
         ++it ) {
        ++count;
        cata::colony<int *>::iterator temp = it.base();
        it = test_colony.erase( --temp );
    }

    // Full erase reverse iteration test
    CHECK( count == 200 );
    // Post-erase size test
    CHECK( test_colony.empty() );

    // Restore previous state after erasure
    for( int temp = 0; temp < 200; ++temp ) {
        test_colony.insert( &ten );
        test_colony.insert( &twenty );
    }

    count = 0;
    for( cata::colony<int *>::iterator it = --cata::colony<int *>::iterator( test_colony.end() );
         it != test_colony.begin(); --it ) {
        ++count;
    }

    // Negative iteration test
    CHECK( count == 399 );

    count = 0;
    for( cata::colony<int *>::iterator it = --( cata::colony<int *>::iterator( test_colony.end() ) );
         it != test_colony.begin(); test_colony.advance( it, -2 ) ) {
        ++count;
    }

    // Negative multiple iteration test
    CHECK( count == 200 );

    test_colony_2 = std::move( test_colony );

    // Move test
    CHECK( test_colony_2.size() == 400 );

    // NOLINTNEXTLINE(bugprone-use-after-move)
    test_colony.insert( &ten );

    // Insert to post-moved-colony test
    CHECK( test_colony.size() == 1 );

    cata::colony<int *> test_colony_5( test_colony_2 );
    cata::colony<int *> test_colony_6( std::move( test_colony_5 ), test_colony_2.get_allocator() );

    // Allocator-extended move construct test
    CHECK( test_colony_6.size() == 400 );

    test_colony_3 = test_colony_2;

    //Copy test 2
    CHECK( test_colony_3.size() == 400 );

    test_colony_2.insert( &ten );

    test_colony_2.swap( test_colony_3 );

    //Swap test
    CHECK( test_colony_2.size() == test_colony_3.size() - 1 );

    swap( test_colony_2, test_colony_3 );

    //Swap test 2
    CHECK( test_colony_3.size() == test_colony_2.size() - 1 );

    //max_size()
    CHECK( test_colony_2.max_size() > test_colony_2.size() );
}

TEST_CASE( "colony insert and erase", "[colony]" )
{
    cata::colony<int> test_colony;

    for( int i = 0; i < 500000; ++i ) {
        test_colony.insert( i );
    }

    // Size after insert
    CHECK( test_colony.size() == 500000 );

    cata::colony<int>::iterator found = std::find( test_colony.begin(), test_colony.end(), 5000 );
    cata::colony<int>::reverse_iterator found2 = std::find( test_colony.rbegin(), test_colony.rend(),
            5000 );

    // std::find reverse_iterator
    CHECK( *found == 5000 );
    // std::find iterator
    CHECK( *found2 == 5000 );

    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        it = test_colony.erase( it );
    }

    // Erase alternating
    CHECK( test_colony.size() == 250000 );

    do {
        for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ) {
            if( ( xor_rand() & 7 ) == 0 ) {
                it = test_colony.erase( it );
            } else {
                ++it;
            }
        }

    } while( !test_colony.empty() );

    //Erase randomly till-empty
    CHECK( test_colony.empty() );

    test_colony.clear();
    test_colony.change_minimum_group_size( 10000 );

    for( int i = 0; i != 30000; ++i ) {
        test_colony.insert( 1 );
    }

    // Size after reinitialize + insert
    CHECK( test_colony.size() == 30000 );

    int count = 0;
    do {
        for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ) {
            if( ( xor_rand() & 7 ) == 0 ) {
                it = test_colony.erase( it );
                ++count;
            } else {
                ++it;
            }
        }

    } while( count < 15000 );

    // Erase randomly till half-empty
    CHECK( test_colony.size() == static_cast<size_t>( 30000 - count ) );

    for( int i = 0; i < count; ++i ) {
        test_colony.insert( 1 );
    }

    // Size after reinsert
    CHECK( test_colony.size() == 30000 );

    count = 0;
    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ) {
        if( ++count == 3 ) {
            count = 0;
            it = test_colony.erase( it );
        } else {
            test_colony.insert( 1 );
            ++it;
        }
    }

    // Alternating insert/erase
    CHECK( test_colony.size() == 45001 );

    do {
        for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ) {
            if( ( xor_rand() & 3 ) == 0 ) {
                ++it;
                test_colony.insert( 1 );
            } else {
                it = test_colony.erase( it );
            }
        }
    } while( !test_colony.empty() );

    // Random insert/erase till empty
    CHECK( test_colony.empty() );

    for( int i = 0; i != 500000; ++i ) {
        test_colony.insert( 10 );
    }

    // Insert post-erase
    CHECK( test_colony.size() == 500000 );

    cata::colony<int>::iterator it = test_colony.begin();
    test_colony.advance( it, 250000 );

    for( ; it != test_colony.end(); ) {
        it = test_colony.erase( it );
    }

    // Large multi-increment iterator
    CHECK( test_colony.size() == 250000 );

    for( int i = 0; i < 250000; ++i ) {
        test_colony.insert( 10 );
    }

    cata::colony<int>::iterator end_it = test_colony.end();
    test_colony.advance( end_it, -250000 );

    for( cata::colony<int>::iterator it = test_colony.begin(); it != end_it; ) {
        it = test_colony.erase( it );
    }

    // Large multi-decrement iterator
    CHECK( test_colony.size() == 250000 );

    for( int i = 0; i < 250000; ++i ) {
        test_colony.insert( 10 );
    }

    int sum = 0;
    for( int it : test_colony ) {
        sum += it;
    }

    // Re-insert post-heavy-erasure
    CHECK( sum == 5000000 );

    end_it = test_colony.end();
    test_colony.advance( end_it, -50001 );
    cata::colony<int>::iterator begin_it = test_colony.begin();
    test_colony.advance( begin_it, 300000 );

    for( cata::colony<int>::iterator it = begin_it; it != end_it; ) {
        it = test_colony.erase( it );
    }

    // Non-end decrement + erase
    CHECK( test_colony.size() == 350001 );

    for( int i = 0; i < 100000; ++i ) {
        test_colony.insert( 10 );
    }

    begin_it = test_colony.begin();
    test_colony.advance( begin_it, 300001 );

    for( cata::colony<int>::iterator it = begin_it; it != test_colony.end(); ) {
        it = test_colony.erase( it );
    }

    // Non-beginning increment + erase
    CHECK( test_colony.size() == 300001 );

    it = test_colony.begin();
    test_colony.advance( it, 2 ); // Advance test 1
    unsigned int index = static_cast<unsigned int>( test_colony.get_index_from_iterator( it ) );

    // Advance + iterator-to-index test
    CHECK( index == 2 );

    // Check edge-case with advance when erasures present in initial group
    test_colony.erase( it );
    it = test_colony.begin();
    test_colony.advance( it, 500 );
    index = static_cast<unsigned int>( test_colony.get_index_from_iterator( it ) );

    // Advance + iterator-to-index test
    CHECK( index == 500 );

    cata::colony<int>::iterator it2 = test_colony.get_iterator_from_pointer( &( *it ) );

    // Pointer-to-iterator test
    CHECK( it2 != test_colony.end() );

    it2 = test_colony.get_iterator_from_index( 500 );

    // Index-to-iterator test
    CHECK( it2 == it );

    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ) {
        it = test_colony.erase( it );
    }

    // Total erase
    CHECK( test_colony.empty() );

    test_colony.clear();
    test_colony.change_minimum_group_size( 3 );
    const unsigned int temp_capacity2 = static_cast<unsigned int>( test_colony.capacity() );
    test_colony.reserve( 1000 );

    // Colony reserve
    CHECK( temp_capacity2 != test_colony.capacity() );
    CHECK( test_colony.capacity() == 1000 );

    count = 0;
    for( int i = 0; i < 50000; ++i ) {
        for( int j = 0; j < 10; ++j ) {
            if( ( xor_rand() & 7 ) == 0 ) {
                test_colony.insert( 1 );
                ++count;
            }
        }
        int count2 = 0;
        for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ) {
            if( ( xor_rand() & 7 ) == 0 ) {
                it = test_colony.erase( it );
                --count;
            } else {
                ++it;
            }
            ++count2;
        }
    }

    // Multiple sequential small insert/erase commands
    CHECK( test_colony.size() == static_cast<size_t>( count ) );
}

TEST_CASE( "colony range erase", "[colony]" )
{
    cata::colony<int> test_colony;

    int count = 0;
    for( ; count != 1000; ++count ) {
        test_colony.insert( count );
    }

    cata::colony<int>::iterator it1 = test_colony.begin();
    cata::colony<int>::iterator it2 = test_colony.begin();

    test_colony.advance( it1, 500 );
    test_colony.advance( it2, 800 );

    test_colony.erase( it1, it2 );

    count = 0;
    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        ++count;
    }

    //Simple range erase 1
    CHECK( count == 700 );
    CHECK( test_colony.size() == 700 );

    it1 = test_colony.begin();
    it2 = test_colony.begin();

    test_colony.advance( it1, 400 );
    test_colony.advance( it2, 500 ); // This should put it2 past the point of previous erasures

    test_colony.erase( it1, it2 );

    count = 0;
    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        ++count;
    }

    //Simple range erase 2
    CHECK( count == 600 );
    CHECK( test_colony.size() == 600 );

    it2 = it1 = test_colony.begin();

    test_colony.advance( it1, 4 );
    test_colony.advance( it2, 9 ); // This should put it2 past the point of previous erasures

    test_colony.erase( it1, it2 );

    count = 0;
    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        ++count;
    }

    // Simple range erase 3
    CHECK( count == 595 );
    CHECK( test_colony.size() == 595 );

    it1 = test_colony.begin();
    it2 = test_colony.begin();

    test_colony.advance( it2, 50 );

    test_colony.erase( it1, it2 );

    count = 0;
    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        ++count;
    }

    // Range erase from begin()
    CHECK( count == 545 );
    CHECK( test_colony.size() == 545 );

    it1 = test_colony.begin();
    it2 = test_colony.end();

    // Test erasing and validity when it removes the final group in colony
    test_colony.advance( it1, 345 );
    test_colony.erase( it1, it2 );

    count = 0;
    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        ++count;
    }

    // Range erase to end()
    CHECK( count == 345 );
    CHECK( test_colony.size() == 345 );

    test_colony.clear();

    for( count = 0; count != 3000; ++count ) {
        test_colony.insert( count );
    }

    for( cata::colony<int>::iterator it = test_colony.begin(); it < test_colony.end(); ++it ) {
        it = test_colony.erase( it );
    }

    it2 = test_colony.begin();
    it1 = test_colony.begin();

    test_colony.advance( it1, 4 );
    test_colony.advance( it2, 600 );
    test_colony.erase( it1, it2 );

    count = 0;
    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        ++count;
    }

    // Range erase with colony already half-erased, alternating erasures
    CHECK( count == 904 );
    CHECK( test_colony.size() == 904 );

    test_colony.clear();

    for( count = 0; count < 3000; ++count ) {
        test_colony.insert( count );
    }

    for( cata::colony<int>::iterator it = test_colony.begin(); it < test_colony.end(); ++it ) {
        if( ( xor_rand() & 1 ) == 0 ) {
            it = test_colony.erase( it );
        }
    }

    if( test_colony.size() < 400 ) {
        for( count = 0; count != 400; ++count ) {
            test_colony.insert( count );
        }
    }

    it1 = test_colony.begin();
    it2 = test_colony.end();

    test_colony.advance( it1, 400 );
    test_colony.erase( it1, it2 );

    count = 0;

    for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
        ++count;
    }

    // Range-erase with colony already third-erased, randomized erasures
    CHECK( count == 400 );
    CHECK( test_colony.size() == 400 );

    unsigned int size, range1, range2, loop_count, internal_loop_count;
    for( loop_count = 0; loop_count < 50; ++loop_count ) {
        test_colony.clear();

        for( count = 0; count < 1000; ++count ) {
            test_colony.insert( count );
        }

        internal_loop_count = 0;

        while( !test_colony.empty() ) {
            it1 = test_colony.begin();
            it2 = test_colony.begin();

            size = static_cast<unsigned int>( test_colony.size() );
            range1 = xor_rand() % size;
            range2 = range1 + 1 + ( xor_rand() % ( size - range1 ) );
            test_colony.advance( it1, range1 );
            test_colony.advance( it2, range2 );

            test_colony.erase( it1, it2 );

            count = 0;
            for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
                ++count;
            }

            // Fuzz-test range-erase randomly Fails
            CAPTURE( loop_count, internal_loop_count );
            CHECK( test_colony.size() == static_cast<unsigned int>( count ) );

            if( test_colony.size() > 2 ) {
                // Test to make sure our stored erased_locations are valid
                test_colony.insert( 1 );
                test_colony.insert( 10 );
            }

            ++internal_loop_count;
        }
    }

    // "Fuzz-test range-erase randomly until empty"
    CHECK( test_colony.empty() );

    for( loop_count = 0; loop_count < 50; ++loop_count ) {
        test_colony.clear();
        internal_loop_count = 0;

        test_colony.insert( 10000, 10 ); // fill-insert

        while( !test_colony.empty() ) {
            it1 = test_colony.begin();
            it2 = test_colony.begin();

            size = static_cast<unsigned int>( test_colony.size() );
            range1 = xor_rand() % size;
            range2 = range1 + 1 + ( xor_rand() % ( size - range1 ) );
            test_colony.advance( it1, range1 );
            test_colony.advance( it2, range2 );

            test_colony.erase( it1, it2 );

            count = 0;
            for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
                ++count;
            }

            // Fuzz-test range-erase + fill-insert randomly fails during erase
            CAPTURE( loop_count, internal_loop_count );
            CHECK( test_colony.size() == static_cast<unsigned int>( count ) );

            if( test_colony.size() > 100 ) {
                // Test to make sure our stored erased_locations are valid & fill-insert is functioning properly in these scenarios
                const unsigned int extra_size = xor_rand() % 128;
                test_colony.insert( extra_size, 5 );

                // Fuzz-test range-erase + fill-insert randomly fails during insert
                CAPTURE( loop_count, internal_loop_count );
                CHECK( test_colony.size() == static_cast<unsigned int>( count ) + extra_size );

                count = 0;
                for( cata::colony<int>::iterator it = test_colony.begin(); it != test_colony.end(); ++it ) {
                    ++count;
                }

                // Fuzz-test range-erase + fill-insert randomly fails during count-test fill-insert
                CHECK( test_colony.size() == static_cast<unsigned int>( count ) );
            }

            ++internal_loop_count;
        }
    }

    // Fuzz-test range-erase + fill-insert randomly until empty
    CHECK( test_colony.empty() );

    test_colony.erase( test_colony.begin(), test_colony.end() );

    // Range-erase when colony is empty test (crash test)
    CHECK( test_colony.empty() );

    test_colony.insert( 10, 1 );
    test_colony.erase( test_colony.begin(), test_colony.begin() );

    // Range-erase when range is empty test (crash test)
    CHECK( test_colony.size() == 10 );
}

TEST_CASE( "colony sort", "[colony]" )
{
    cata::colony<int> test_colony;

    test_colony.reserve( 50000 );

    for( int i = 0; i < 50000; ++i ) {
        test_colony.insert( xor_rand() & 65535 );
    }

    test_colony.sort();

    bool sorted = true;
    int prev = 0;

    for( int cur : test_colony ) {
        if( prev > cur ) {
            sorted = false;
            break;
        }
        prev = cur;
    }

    // Less-than sort test
    CHECK( sorted );

    test_colony.sort( std::greater<int>() );

    prev = 65536;

    for( int cur : test_colony ) {
        if( prev < cur ) {
            sorted = false;
            break;
        }
        prev = cur;
    }

    // Greater-than sort test
    CHECK( sorted );
}

TEST_CASE( "colony insertion methods", "[colony]" )
{
    cata::colony<int> test_colony = {1, 2, 3};

    // Initializer-list constructor
    CHECK( test_colony.size() == 3 );

    cata::colony<int> test_colony_2( test_colony.begin(), test_colony.end() );

    // Range constructor
    CHECK( test_colony_2.size() == 3 );

    cata::colony<int> test_colony_3( 5000, 2, 100, 1000 );

    // Fill construction
    CHECK( test_colony_3.size() == 5000 );

    test_colony_2.insert( 500000, 5 );

    // Fill insertion
    CHECK( test_colony_2.size() == 500003 );

    std::vector<int> some_ints( 500, 2 );

    test_colony_2.insert( some_ints.begin(), some_ints.end() );

    // Range insertion
    CHECK( test_colony_2.size() == 500503 );

    test_colony_3.clear();
    test_colony_2.clear();
    test_colony_2.reserve( 50000 );
    test_colony_2.insert( 60000, 1 );

    int sum = 0;

    for( int it : test_colony_2 ) {
        sum += it;
    }

    // Reserve + fill insert
    CHECK( test_colony_2.size() == 60000 );
    CHECK( sum == 60000 );

    test_colony_2.clear();
    test_colony_2.reserve( 5000 );
    test_colony_2.insert( 60, 1 );

    sum = 0;

    for( int it : test_colony_2 ) {
        sum += it;
    }

    // Reserve + fill insert
    CHECK( test_colony_2.size() == 60 );
    CHECK( sum == 60 );

    test_colony_2.insert( 6000, 1 );

    sum = 0;

    for( int it : test_colony_2 ) {
        sum += it;
    }

    // Reserve + fill + fill
    CHECK( test_colony_2.size() == 6060 );
    CHECK( sum == 6060 );

    test_colony_2.reserve( 18000 );
    test_colony_2.insert( 6000, 1 );

    sum = 0;
    for( int it : test_colony_2 ) {
        sum += it;
    }

    // Reserve + fill + fill + reserve + fill
    CHECK( test_colony_2.size() == 12060 );
    CHECK( sum == 12060 );
}

TEST_CASE( "colony perfect forwarding", "[colony]" )
{
    cata::colony<perfect_forwarding_test> test_colony;

    int lvalue = 0;
    int &lvalueref = lvalue;

    test_colony.emplace( 7, lvalueref );

    CHECK( ( *test_colony.begin() ).success );
    CHECK( lvalueref == 1 );
}

TEST_CASE( "colony emplace", "[colony]" )
{
    cata::colony<small_struct> test_colony;
    int sum1 = 0, sum2 = 0;

    for( int i = 0; i < 100; ++i ) {
        test_colony.emplace( i );
        sum1 += i;
    }

    for( small_struct &s : test_colony ) {
        sum2 += s.number;
    }

    //Basic emplace
    CHECK( sum1 == sum2 );
    //Basic emplace
    CHECK( test_colony.size() == 100 );
}

TEST_CASE( "colony group size and capacity", "[colony]" )
{
    cata::colony<int> test_colony;
    test_colony.change_group_sizes( 50, 100 );

    test_colony.insert( 27 );

    // Change_group_sizes min-size
    CHECK( test_colony.capacity() == 50 );

    for( int i = 0; i != 100; ++i ) {
        test_colony.insert( i );
    }

    // Change_group_sizes max-size
    CHECK( test_colony.capacity() == 200 );

    test_colony.reinitialize( 200, 2000 );

    test_colony.insert( 27 );

    // Reinitialize min-size
    CHECK( test_colony.capacity() == 200 );

    for( int i = 0; i != 3300; ++i ) {
        test_colony.insert( i );
    }

    // Reinitialize max-size
    CHECK( test_colony.capacity() == 5200 );

    test_colony.change_group_sizes( 500, 500 );

    // Change_group_sizes resize
    CHECK( test_colony.capacity() == 3500 );

    test_colony.change_minimum_group_size( 200 );
    test_colony.change_maximum_group_size( 200 );

    // Change_maximum_group_size resize
    CHECK( test_colony.capacity() == 3400 );
}

TEST_CASE( "colony splice", "[colony]" )
{
    cata::colony<int> test_colony_1, test_colony_2;

    SECTION( "small splice 1" ) {
        int i = 0;
        for( ; i < 20; ++i ) {
            test_colony_1.insert( i );
            test_colony_2.insert( i + 20 );
        }

        test_colony_1.splice( test_colony_2 );

        i = 0;
        for( cata::colony<int>::iterator it = test_colony_1.begin(); it < test_colony_1.end(); ++it ) {
            CHECK( i++ == *it );
        }
    }

    SECTION( "small splice 2" ) {
        int i = 0;
        for( ; i < 100; ++i ) {
            test_colony_1.insert( i );
            test_colony_2.insert( i + 100 );
        }

        test_colony_1.splice( test_colony_2 );

        i = 0;
        for( int it : test_colony_1 ) {
            CHECK( i++ == it );
        }
    }

    SECTION( "large splice" ) {
        int i = 0;
        for( ; i < 100000; ++i ) {
            test_colony_1.insert( i );
            test_colony_2.insert( i + 100000 );
        }

        test_colony_1.splice( test_colony_2 );

        i = 0;
        for( int it : test_colony_1 ) {
            CHECK( i++ == it );
        }
    }

    SECTION( "erase and splice 1" ) {
        for( int i = 0; i < 100; ++i ) {
            test_colony_1.insert( i );
            test_colony_2.insert( i + 100 );
        }

        for( cata::colony<int>::iterator it = test_colony_2.begin(); it != test_colony_2.end(); ) {
            if( ( xor_rand() & 7 ) == 0 ) {
                it = test_colony_2.erase( it );
            } else {
                ++it;
            }
        }

        test_colony_1.splice( test_colony_2 );

        int prev = -1;
        for( int it : test_colony_1 ) {
            CHECK( prev < it );
            prev = it;
        }
    }

    SECTION( "erase and splice 2" ) {
        for( int i = 0; i != 100; ++i ) {
            test_colony_1.insert( i );
            test_colony_2.insert( i + 100 );
        }

        for( cata::colony<int>::iterator it = test_colony_2.begin(); it != test_colony_2.end(); ) {
            if( ( xor_rand() & 3 ) == 0 ) {
                it = test_colony_2.erase( it );
            } else {
                ++it;
            }
        }

        for( cata::colony<int>::iterator it = test_colony_1.begin(); it != test_colony_1.end(); ) {
            if( ( xor_rand() & 1 ) == 0 ) {
                it = test_colony_1.erase( it );
            } else {
                ++it;
            }
        }

        test_colony_1.splice( test_colony_2 );

        int prev = -1;
        for( int it : test_colony_1 ) {
            CHECK( prev < it );
            prev = it;
        }
    }

    SECTION( "unequal size splice 1" ) {
        test_colony_1.change_group_sizes( 200, 200 );
        test_colony_2.change_group_sizes( 200, 200 );

        for( int i = 0; i < 100; ++i ) {
            test_colony_1.insert( i + 150 );
        }

        for( int i = 0; i < 150; ++i ) {
            test_colony_2.insert( i );
        }

        test_colony_1.splice( test_colony_2 );

        int prev = -1;
        for( int it : test_colony_1 ) {
            CHECK( prev < it );
            prev = it;
        }
    }

    SECTION( "unequal size splice 2" ) {
        test_colony_1.reinitialize( 200, 200 );
        test_colony_2.reinitialize( 200, 200 );

        for( int i = 0; i < 100; ++i ) {
            test_colony_1.insert( 100 - i );
        }

        for( int i = 0; i < 150; ++i ) {
            test_colony_2.insert( 250 - i );
        }

        test_colony_1.splice( test_colony_2 );

        int prev = 255;
        for( int it : test_colony_1 ) {
            CHECK( prev >= it );
            prev = it;
        }
    }

    SECTION( "large unequal size erase and splice" ) {
        for( int i = 0; i < 100000; ++i ) {
            test_colony_1.insert( i + 200000 );
        }

        for( int i = 0; i < 200000; ++i ) {
            test_colony_2.insert( i );
        }

        for( cata::colony<int>::iterator it = test_colony_2.begin(); it != test_colony_2.end(); ) {
            if( ( xor_rand() & 1 ) == 0 ) {
                it = test_colony_2.erase( it );
            } else {
                ++it;
            }
        }

        for( cata::colony<int>::iterator it = test_colony_1.begin(); it != test_colony_1.end(); ) {
            if( ( xor_rand() & 1 ) == 0 ) {
                it = test_colony_1.erase( it );
            } else {
                ++it;
            }
        }

        test_colony_1.erase( --( test_colony_1.end() ) );
        test_colony_2.erase( --( test_colony_2.end() ) );

        // splice should swap the order at this point due to differences in numbers of unused elements at end of final group in each colony
        test_colony_1.splice( test_colony_2 );

        int prev = -1;
        for( int it : test_colony_1 ) {
            CHECK( prev < it );
            prev = it;
        }

        do {
            for( cata::colony<int>::iterator it = test_colony_1.begin(); it != test_colony_1.end(); ) {
                if( ( xor_rand() & 3 ) == 0 ) {
                    it = test_colony_1.erase( it );
                } else if( ( xor_rand() & 7 ) == 0 ) {
                    test_colony_1.insert( 433 );
                    ++it;
                } else {
                    ++it;
                }
            }

        } while( !test_colony_1.empty() );

        // Post-splice insert-and-erase randomly till-empty
        CHECK( test_colony_1.empty() );
    }
}
