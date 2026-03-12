#pragma once
#ifndef CATA_SRC_OVERMAP_DEBUG_H
#define CATA_SRC_OVERMAP_DEBUG_H

#include <string>

#include "coords_fwd.h"

namespace om_noise
{
class om_noise_layer;
} // namespace om_noise

namespace om_debug
{

void print_noise_maps( const tripoint_abs_omt &cursor );

void export_raw_noise( const std::string &filename, const om_noise::om_noise_layer &noise,
                       int width, int height );

void export_interpreted_noise( const std::string &filename, const om_noise::om_noise_layer &noise,
                               int width, int height, float threshold, bool invert );

} // namespace om_debug

#endif // CATA_SRC_OVERMAP_DEBUG_H
