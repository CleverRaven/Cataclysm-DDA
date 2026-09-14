#pragma once
#ifndef CATA_SRC_IMGUI_TEXTURE_H
#define CATA_SRC_IMGUI_TEXTURE_H

#include "cata_imgui.h"
#ifdef TILES
#include "cata_tiles.h"
#endif
#include "coords_fwd.h"
#include "sdltiles.h"

#ifdef TILES
template<typename T>
static TILE_CATEGORY get_category( const string_id<T> & )
{
    if( std::is_same_v<T, ter_t> ) {
        return TILE_CATEGORY::TERRAIN;
    }
    if( std::is_same_v<T, furn_t> ) {
        return TILE_CATEGORY::FURNITURE;
    }
    if( std::is_same_v<T, itype> ) {
        return TILE_CATEGORY::ITEM;
    }
    if( std::is_same_v<T, mtype> ) {
        return TILE_CATEGORY::MONSTER;
    }
    if( std::is_same_v<T, character_portrait> ) {
        return TILE_CATEGORY::PORTRAIT;
    }

    return TILE_CATEGORY::NONE;
}
#endif

namespace cataimgui
{
// does nothing if not a tiles build
// todo: overload for non-string_id based categories (e.g. NPCs)
template<typename T>
// nolint for curses build...
// NOLINTBEGIN(unused-parameter)
void draw_texture( const string_id<T> &id, const tripoint_bub_ms &p, ImVec2 size = ImVec2()
                 )
{
#ifdef TILES
    TILE_CATEGORY category = get_category( id );

    std::optional<texture_draw_data> data = tilecontext->get_texture_draw_data( id.str(), category, p );

    switch( category ) {
        case TILE_CATEGORY::PORTRAIT:
            data = portrait_tilecontext->get_texture_draw_data( id.str(), category, p );
            break;
        default:
            data = tilecontext->get_texture_draw_data( id.str(), category, p );
    }

    if( !data ) {
        return;
    }

    ImVec2 actual_size;
    actual_size.x = size.x > 0 ? size.x : data->dimensions.w;
    actual_size.y = size.y > 0 ? size.y : data->dimensions.h;

    ImGui::Image( reinterpret_cast<ImTextureID>( data->texture ), actual_size, { data->uv0.first, data->uv0.second }, { data->uv1.first, data->uv1.second } );
#endif
}
// NOLINTEND(unused-parameter)
} // namespace cataimgui

#endif // CATA_SRC_IMGUI_TEXTURE_H
