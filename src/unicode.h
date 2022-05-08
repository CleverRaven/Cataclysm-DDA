#pragma once
#ifndef CATA_SRC_UNICODE_H
#define CATA_SRC_UNICODE_H

#include <cstdint>

/**
 * Used temporarily for determining line breaking opportunities. Ultimatedly this
 * should be changed to use the data from https://unicode.org/Public/UNIDATA/LineBreak.txt
 * for strict unicode conformance.
 */
bool is_cjk_or_emoji( uint32_t ch );

#endif // CATA_SRC_UNICODE_H
