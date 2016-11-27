#include <chrono>

#include "persistent_handle.h"

namespace persistent_id_sequence {
    long next_id_value = 1;

    void reset( long hint ) {
        using namespace std::chrono;        
        long secs = duration_cast<seconds>( system_clock().now().time_since_epoch() ).count();
        next_id_value = std::max( 1L, std::max( secs, hint ) );
    }
};
