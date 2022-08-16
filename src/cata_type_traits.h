#ifndef CATA_SRC_CATA_TYPE_TRAITS_H
#define CATA_SRC_CATA_TYPE_TRAITS_H

// For some template magic in generic_factory.h, it's much easier to use void_t
// However, C++14 does not have this, so we need our own.
// It's nothing complex, just strip it out when C++17 comes.
#include <type_traits>
namespace cata
{
template<typename...>
using void_t = void;

// Copy constness from the first type to the second.
// I think something similar is proposed for a future std library version, but
// I can't find it.
template<typename CopyFrom, typename CopyTo>
using copy_const = std::conditional_t<std::is_const<CopyFrom>::value, const CopyTo, CopyTo>;

} // namespace cata

#endif // CATA_SRC_CATA_TYPE_TRAITS_H
