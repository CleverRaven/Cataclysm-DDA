#pragma once
#ifndef WCWIDTH_H
#define WCWIDTH_H

#include <cstdint>

/* Get character width in columns.  See wcwidth.cpp for details.
 */
int mk_wcwidth( uint32_t ucs );

#endif
