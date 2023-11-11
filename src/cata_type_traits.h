#pragma once
#ifndef CATA_SRC_CATA_TYPE_TRAITS_H
#define CATA_SRC_CATA_TYPE_TRAITS_H

#include <type_traits>
namespace cata
{
// Copy constness from the first type to the second.
// I think something similar is proposed for a future std library version, but
// I can't find it.
template<typename CopyFrom, typename CopyTo>
using copy_const = std::conditional_t<std::is_const_v<CopyFrom>, const CopyTo, CopyTo>;

} // namespace cata

#endif // CATA_SRC_CATA_TYPE_TRAITS_H
