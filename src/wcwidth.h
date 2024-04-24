#pragma once
#ifndef CATA_SRC_WCWIDTH_H
#define CATA_SRC_WCWIDTH_H

#include <cstdint>

/* Get character width in columns.  See wcwidth.cpp for details.
 */
int mk_wcwidth( uint32_t ucs );

#endif // CATA_SRC_WCWIDTH_H
