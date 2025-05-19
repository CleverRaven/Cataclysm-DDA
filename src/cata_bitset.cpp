#include "cata_bitset.h"

#include "cata_assert.h"

namespace
{

size_t bit_count_to_block_count( size_t bits )
{
    return ( bits + tiny_bitset::kBitsPerBlock - 1 ) / tiny_bitset::kBitsPerBlock;
}

} // namespace

void tiny_bitset::resize_heap( size_t requested_bits ) noexcept
{
    size_t blocks_needed = bit_count_to_block_count( requested_bits );

    // Round up to a multiple of the block allocation size.
    // The allocation size is stored by malloc so realloc() can be optimized if we keep resizing by small amounts.
    blocks_needed = ( ( blocks_needed + ( kMinimumBlockPerAlloc - 1 ) ) / kMinimumBlockPerAlloc ) *
                    kMinimumBlockPerAlloc + 1;

    if( is_inline() ) {
        // Allocate an extra block for prepending the capacity to.
        block_t *data = static_cast<block_t *>( malloc( sizeof( block_t ) * ( blocks_needed + 1 ) ) );
        cata_assert( data != nullptr );
        // Capacity goes at the front ahead of the actual bits.
        data[ 0 ] = requested_bits;
        // Copy inline bits to heap.
        data[ 1 ] = storage_ & ~kMetaMask;
        set_storage( data + 1 );
    } else {
        block_t *data = static_cast<block_t *>( realloc( real_heap_allocation(),
                                                sizeof( block_t ) * ( blocks_needed + 1 ) ) );
        if( data != real_heap_allocation() ) {
            set_storage( data + 1 );
        }
    }

    set_size( requested_bits );
}

void tiny_bitset::set_storage( block_t *data )
{
    // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
    memcpy( &storage_, &data, sizeof( data ) );
}
