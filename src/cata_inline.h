#pragma once
#ifndef CATA_SRC_CATA_INLINE_H
#define CATA_SRC_CATA_INLINE_H

#ifndef CATA_FORCEINLINE
#ifdef _MSC_VER
#define CATA_FORCEINLINE __forceinline
#else
#define CATA_FORCEINLINE inline __attribute__((always_inline))
#endif
#endif

#endif // CATA_SRC_CATA_INLINE_H
