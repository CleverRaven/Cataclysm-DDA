#ifndef CATA_SRC_CATA_VOID_H
#define CATA_SRC_CATA_VOID_H

// For some template magic in generic_factory.h, it's much easier to use void_t
// However, C++14 does not have this, so we need our own.
// It's nothing complex, just strip it out when C++17 comes.
namespace cata
{
template<typename...>
using void_t = void;
} // namespace cata

#endif // CATA_SRC_CATA_VOID_H
