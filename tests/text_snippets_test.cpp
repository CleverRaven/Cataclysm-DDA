#include <new>

#include "catch/catch.hpp"
#include "optional.h"
#include "text_snippets.h"
#include "translations.h"

TEST_CASE( "random_snippet_with_small_seed", "[text_snippets][rng]" )
{
    const int seed_start = -10;
    const int seed_end = 10;
    int snip_change = 0;
    cata::optional<translation> prev_snip;
    for( int seed = seed_start; seed <= seed_end; ++seed ) {
        const cata::optional<translation> snip = SNIPPET.random_from_category( "lab_notes", seed );
        REQUIRE( snip.has_value() );
        if( prev_snip.has_value() && *prev_snip != *snip ) {
            snip_change++;
        }
        prev_snip = snip;
    }
    // Random snippets change at least 90% of the time when the seed has changed.
    // This is a very weak requirement, but should rule out the possibility of
    // using `std::minstd_rand0` with `std::uniform_int_distribution`.
    CHECK( snip_change >= ( seed_end - seed_start ) * 0.9 );
}
