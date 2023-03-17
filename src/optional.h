#pragma once
#ifndef CATA_SRC_OPTIONAL_H
#define CATA_SRC_OPTIONAL_H

#include <optional>

namespace cata
{

static constexpr std::nullopt_t nullopt = std::nullopt;
static constexpr std::in_place_t in_place = std::in_place;

template<typename T>
using optional = std::optional<T>;

} // namespace cata

#endif // CATA_SRC_OPTIONAL_H
