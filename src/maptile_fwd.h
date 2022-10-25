#pragma once
#ifndef CATA_SRC_MAPTILE_FWD_H
#define CATA_SRC_MAPTILE_FWD_H

template<typename Submap>
class maptile_impl;
class submap;

using maptile = maptile_impl<submap>;
using const_maptile = maptile_impl<const submap>;

#endif // CATA_SRC_MAPTILE_FWD_H
