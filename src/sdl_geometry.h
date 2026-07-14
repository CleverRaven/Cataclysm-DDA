#pragma once
#ifndef CATA_SRC_SDL_GEOMETRY_H
#define CATA_SRC_SDL_GEOMETRY_H

#if defined(TILES)
#include <memory>

#include "sdl_wrappers.h"

struct point;

/// Interface to render geometry with SDL_Renderer.
class GeometryRenderer
{
    public:
        virtual ~GeometryRenderer() = default;

        /// Renders a SDL rectangle with given color.
        virtual void rect( const SDL_Renderer_Ptr &renderer, const SDL_Rect &rect,
                           const SDL_Color &color ) const = 0;

        /// Drop any renderer-owned GPU resources. Safe to call repeatedly.
        /// Default is a no-op for renderers that hold no GPU handles.
        virtual void release_gpu_resources() {}

        /// Recreate renderer-owned GPU resources against `renderer` after a
        /// device reset or renderer recreation. Default is a no-op.
        virtual void rebuild_for_renderer( const SDL_Renderer_Ptr &renderer ) {
            ( void )renderer;
        }

        /// Renders a point+width+height defined rectangle with given color.
        void rect( const SDL_Renderer_Ptr &renderer, const point &pos, int width, int height,
                   const SDL_Color &color ) const;

        /// Renders a straight horizontal line with given thickness and color.
        void horizontal_line( const SDL_Renderer_Ptr &renderer, const point &pos, int x2, int thickness,
                              const SDL_Color &color ) const;

        /// Renders a straight vertical line with given thickness and color.
        void vertical_line( const SDL_Renderer_Ptr &renderer, const point &pos, int y2, int thickness,
                            const SDL_Color &color ) const;
};
using GeometryRenderer_Ptr = std::unique_ptr<GeometryRenderer>;

/// Implementation of a GeometryRenderer using default RenderFillRect.
class DefaultGeometryRenderer : public GeometryRenderer
{
    public:
        void rect( const SDL_Renderer_Ptr &renderer, const SDL_Rect &rect,
                   const SDL_Color &color ) const override;
};

/// Implementation of a GeometryRenderer using color modulated textures if
/// possible, falling back to DefaultGeometryRenderer otherwise.
class ColorModulatedGeometryRenderer: public DefaultGeometryRenderer
{
    public:
        explicit ColorModulatedGeometryRenderer( const SDL_Renderer_Ptr &renderer );

        void rect( const SDL_Renderer_Ptr &renderer, const SDL_Rect &rect,
                   const SDL_Color &color ) const override;

        void release_gpu_resources() override;
        void rebuild_for_renderer( const SDL_Renderer_Ptr &renderer ) override;
    private:
        // Build the 1x1 white modulation texture against `renderer` and
        // verify color modulation is supported; leaves tex null on failure
        // so rect() falls back to DefaultGeometryRenderer.
        void build_texture( const SDL_Renderer_Ptr &renderer );

        SDL_Texture_Ptr tex;
};

#endif // TILES

#endif // CATA_SRC_SDL_GEOMETRY_H
