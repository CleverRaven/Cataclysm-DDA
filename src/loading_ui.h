#pragma once
#ifndef CATA_SRC_LOADING_UI_H
#define CATA_SRC_LOADING_UI_H

#include <string>

namespace loading_ui
{
void show( const std::string &context, const std::string &step );
void done();
// Drop the splash GPU texture. Must run while the renderer is live, before a
// device reset/loss invalidates or replaces it, so the texture is destroyed
// against a valid renderer. No-op when no loading session is active.
void release_gpu_resources();
} // namespace loading_ui

#endif // CATA_SRC_LOADING_UI_H
