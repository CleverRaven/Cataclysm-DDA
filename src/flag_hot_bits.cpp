#include "flag.h"

#include <cstdint>

#include "type_id.h"

static const flag_id json_flag_LOCATION_PRECISE_CLOSEST_CITY( "LOCATION_PRECISE_CLOSEST_CITY" );

uint64_t hot_bit_for( const flag_id &f ) noexcept
{
    if( f == flag_REMOVED_STOCK ) {
        return static_cast<uint64_t>( hot_flag_bit::REMOVED_STOCK );
    }
    if( f == flag_DIAMOND ) {
        return static_cast<uint64_t>( hot_flag_bit::DIAMOND );
    }
    if( f == flag_FIT ) {
        return static_cast<uint64_t>( hot_flag_bit::FIT );
    }
    if( f == flag_VARSIZE ) {
        return static_cast<uint64_t>( hot_flag_bit::VARSIZE );
    }
    if( f == flag_MUSHY ) {
        return static_cast<uint64_t>( hot_flag_bit::MUSHY );
    }
    if( f == flag_DIRTY ) {
        return static_cast<uint64_t>( hot_flag_bit::DIRTY );
    }
    if( f == flag_HIDDEN_POISON ) {
        return static_cast<uint64_t>( hot_flag_bit::HIDDEN_POISON );
    }
    if( f == flag_HIDDEN_HALLU ) {
        return static_cast<uint64_t>( hot_flag_bit::HIDDEN_HALLU );
    }
    if( f == flag_IRRADIATED ) {
        return static_cast<uint64_t>( hot_flag_bit::IRRADIATED );
    }
    if( f == flag_INEDIBLE ) {
        return static_cast<uint64_t>( hot_flag_bit::INEDIBLE );
    }
    if( f == flag_NO_PACKED ) {
        return static_cast<uint64_t>( hot_flag_bit::NO_PACKED );
    }
    if( f == flag_NO_STERILE ) {
        return static_cast<uint64_t>( hot_flag_bit::NO_STERILE );
    }
    if( f == flag_USE_UPS ) {
        return static_cast<uint64_t>( hot_flag_bit::USE_UPS );
    }
    if( f == json_flag_LOCATION_PRECISE_CLOSEST_CITY ) {
        return static_cast<uint64_t>( hot_flag_bit::LOC_CITY );
    }
    if( f == flag_ITEM_BROKEN ) {
        return static_cast<uint64_t>( hot_flag_bit::ITEM_BROKEN );
    }
    return 0;
}
