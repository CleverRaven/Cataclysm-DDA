#include "compatibility.h"

#if defined(_WIN32) && defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#include <cstdint>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif

void reset_floating_point_mode()
{
    const std::uint16_t cw = 0x27f;
    __asm__ __volatile__( "fninit" );
    __asm__ __volatile__( "fldcw %0" : : "m"( cw ) );
#if defined(__SSE__)
    const std::uint32_t csr = 0x1f80;
    _mm_setcsr( csr );
#endif
}

#else

void reset_floating_point_mode() {}

#endif