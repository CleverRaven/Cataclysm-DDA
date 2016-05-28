#include "uuid.h"

#include <random>
#include <limits>
#include <sstream>
#include <iomanip>

static std::random_device rd;
static std::mt19937 eng;

/** use a distribution as mt19937 only guarantees minimum (not exactly) 32 bits */
static std::uniform_int_distribution<> distrib( 0, std::numeric_limits<uint32_t>::max() );

static inline void gen_uuid4( uint32_t *data ) {
	data[ 0 ] = distrib( eng );
	data[ 1 ] = distrib( eng );
	data[ 2 ] = distrib( eng );
	data[ 3 ] = distrib( eng );
}

void uuid::init() {
	eng.seed( rd() );
}

uuid::uuid() {
	gen_uuid4( data );
}

uuid::uuid( const uuid & ) {
	gen_uuid4( data );
}

uuid &uuid::operator=( const uuid & ) {
	gen_uuid4( data );
	return *this;
}

uuid::operator std::string() const {
	// first serialize as 32 character hexadecimal string
	std::ostringstream str;
	str << std::hex << std::setfill( '0' );
	str << std::setw( 8 ) << data[ 0 ];
	str << std::setw( 8 ) << data[ 1 ];
	str << std::setw( 8 ) << data[ 2 ];
	str << std::setw( 8 ) << data[ 3 ];

    // add separators
	std::string out = str.str();
    out.insert(  8, 1, '-' );
    out.insert( 13, 1, '-' );
    out.insert( 18, 1, '-' );
    out.insert( 23, 1, '-' );

    return out;
}

void uuid::serialize( JsonOut &js ) const {
    js.write( this->operator std::string() );
}

void uuid::deserialize( JsonIn &js ) {
	std::string val;
    js.read( val );

    // validate separators
    if( val[ 23 ] != '-' || val[ 18 ] != '-' ||
    	val[ 13 ] != '-' || val[  8 ] != '-' ) {
    	js.error( "invalid RFC4122 v4 UUID" );
    }

	// remove separators
    val.erase( 23, 1 );
    val.erase( 18, 1 );
    val.erase( 13, 1 );
    val.erase(  8, 1 );

  	// deserialize from 32 character hexadecimal string
  	std::istringstream str( val );
 	str >> data[ 0 ];
 	str >> data[ 1 ];
 	str >> data[ 2 ];
 	str >> data[ 3 ];
}
