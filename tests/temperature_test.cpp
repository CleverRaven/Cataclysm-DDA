#include <string>

#include "game.h"
#include "catch/catch.hpp"
#include "calendar.h"
#include "itype.h"
#include "ret_val.h"
#include "units.h"
#include "item.h"


TEST_CASE( "Item spawns with right thermal attributes" )
{
	item D( "meat_cooked" );
	
    CHECK( D.type->comestible->specific_heat_liquid == 3.7f );
	CHECK( D.type->comestible->specific_heat_solid == 2.15f );
	CHECK( D.type->comestible->latent_heat == 260 );
	
	CHECK( D.temperature == 0 );
	CHECK( D.specific_energy == -10 );
	
	D.update_temp( 122, 1 );
	
	CHECK( abs( D.temperature - 323.15 * 100000 ) < 1 );
}

TEST_CASE( "Rate of temperature change" )
{	
	// Farenheits and kelvins get used and converted around
	// So there are small rounding errors everywhere
	// Check ranges instead of values when rounding errors
	
	SECTION( "Water at 55 C (131 F, 328.15 K) in 20 C (68 F, 293.15 K) ambient" ) {
		// 75 minutes
		// Temperature after should be approx 35 C
		// Checked with incremental updates and whole time at once
		
		item water1( "water" );
		item water2( "water" );
		
		water1.update_temp( 131, 1 );
		water2.update_temp( 131, 1 );
		
		// 55 C
		CHECK( abs( water1.temperature - 328.15 * 100000 ) < 1 );
	
		
		calendar::turn = to_turn<int>( calendar::turn + 100_turns );
		water1.update_temp( 122, 1 );
		// 0 C frozen
		CHECK( water1.temperature == 32815000 );
		
		calendar::turn = to_turn<int>( calendar::turn + 200_turns );
		water1.update_temp( 122, 1 );
		// 0 C not frozen
		CHECK( water1.temperature == 32611274  );
		
		calendar::turn = to_turn<int>( calendar::turn + 300_turns );
		water1.update_temp( 122, 1 );
		// 27.3 C
		CHECK( water1.temperature == 32490558  );
		
		calendar::turn = to_turn<int>( calendar::turn + 150_turns );
		water1.update_temp( 122, 1 );
		// 42.15 C
		CHECK( water1.temperature == 32450140  );
		
		water2.update_temp( 122, 1 );
		CHECK( water2.temperature == water1.temperature  );
		
		
	}
	
	SECTION( "Cooked meat from 50 C (122 F, 323.15 K) to -20 C (-4 F, 253.15 K)" ) {
		// 2x cooked meat (50 C) cooling in -20 C enviroment for ~165 minutes
		// Wait 101 turns x 3 (~30 minutes).
		// Check temperature after each step
		// Then wait 1 hour at once
		// 0 min: 50C and hot
		// 10 min: 33.5 C and not hot
		// 20 min: 21.0 C
		// 30 min: 11.4 C
		// 30 min: Second meat cooled for whole duration in one step.
		// Temperatures should be about same (rounding errors)
		// 100 min: 0C and frozen
		// 165 min: -18,9 C
		
		item meat1( "meat_cooked" );
		item meat2( "meat_cooked" );
		item meat3( "meat_cooked" );
		
		meat1.update_temp( 122, 1 );
		meat2.update_temp( 122, 1 );
		meat3.update_temp( 122, 1 );
		
		// 50 C
		CHECK( abs( meat1.temperature - 323.15 * 100000 ) < 1 );
		CHECK( meat1.item_tags.count( "HOT" ) );
		
		
		calendar::turn = to_turn<int>( calendar::turn + 101_turns );
		meat1.update_temp( -4, 1 );
		// 33.5 C
		CHECK( meat1.temperature == 30673432 );
		CHECK( !meat1.item_tags.count( "HOT" ) );
		
		calendar::turn = to_turn<int>( calendar::turn + 101_turns );
		meat1.update_temp( -4, 1 );
		// 21.0 C
		CHECK(meat1.temperature == 29416828 );
		
		calendar::turn = to_turn<int>( calendar::turn + 101_turns );
		meat1.update_temp( -4, 1 );
		// 11.4 C
		CHECK( meat1.temperature == 28454910 );
		
		meat2.update_temp( -4, 1 );
		// Check to be within 1/100000 K of each other
		CHECK( meat2.temperature == 28454910 );
		CHECK( abs( meat2.temperature - meat1.temperature ) < 1 );
		
		calendar::turn = to_turn<int>( calendar::turn + 700_turns );
		meat1.update_temp( -4, 1 );
		// 11.4 C
		CHECK( meat1.temperature == 27315000 );
		CHECK( meat1.item_tags.count( "FROZEN" ) );
		
		calendar::turn = to_turn<int>( calendar::turn + 630_turns );
		meat1.update_temp( -4, 1 );
		// -18.8 C
		CHECK( meat1.temperature == 25428548 );
		
		meat3.update_temp( -4, 1 );
		// Check to be within 1/100000 K of each other
		CHECK( meat2.temperature == 28454910 );
		CHECK( abs( meat3.temperature - meat1.temperature ) < 1 );
		
		
		
		
	}
	
	SECTION( "Cooked meat from -20 C (-4 F, 253.15 K) to 20 C (122 F, 323.15 K)" ) {
		// Cooked meat (50 C) cooling in -20 C enviroment for ~30 minutes
		// Same cooling as above but in one step
		// Result should be same with some rounding errors
		// 165 min: -20 C and frozen
		
		item meat1( "meat_cooked" );
		item meat2( "meat_cooked" );
		item meat3( "meat_cooked" );
		
		meat1.update_temp( -4, 1 );
		
		// -20 C
		CHECK( abs( meat1.temperature - 253.15 * 100000 ) < 1 );
		CHECK( meat1.item_tags.count( "FROZEN" ) );
	
		
		calendar::turn = to_turn<int>( calendar::turn + 300_turns );
		meat1.update_temp( 122, 1 );
		// 0 C frozen
		CHECK( meat1.temperature == 27315000 );
		CHECK( meat1.item_tags.count( "FROZEN" ) );
		
		calendar::turn = to_turn<int>( calendar::turn + 200_turns );
		meat1.update_temp( 122, 1 );
		// 0 C not frozen
		CHECK( meat1.temperature == 27315000  );
		CHECK( !meat1.item_tags.count( "FROZEN" ) );
		
		calendar::turn = to_turn<int>( calendar::turn + 300_turns );
		meat1.update_temp( 122, 1 );
		// 27.3 C
		CHECK( meat1.temperature == 30054330  );
		
		calendar::turn = to_turn<int>( calendar::turn + 700_turns );
		meat1.update_temp( 122, 1 );
		// 46.4 C
		CHECK( meat1.temperature == 31960300  );
		
		calendar::turn = to_turn<int>( calendar::turn + 700_turns );
		meat1.update_temp( 122, 1 );
		// 49.4 C
		CHECK( meat1.temperature == 32259348  );
		
	}
}