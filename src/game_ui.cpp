#include "game.h"
#include "map_memory.h"

#include <algorithm>
#include <bitset>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cwctype>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#if defined(EMSCRIPTEN)
#include <emscripten.h>
#endif

#include "achievement.h"
#include "action.h"
#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "activity_type.h"
#include "ascii_art.h"
#include "auto_note.h"
#include "auto_pickup.h"
#include "avatar.h"
#include "avatar_action.h"
#include "basecamp.h"
#include "bionics.h"
#include "body_part_set.h"
#include "bodygraph.h"
#include "bodypart.h"
#include "butchery_requirements.h"
#include "butchery.h"
#include "cached_options.h"
#include "cata_imgui.h"
#include "cata_path.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "cata_variant.h"
#include "catacharset.h"
#include "character.h"
#include "character_attire.h"
#include "character_martial_arts.h"
#include "char_validity_check.h"
#include "city.h"
#include "climbing.h"
#include "clzones.h"
#include "colony.h"
#include "color.h"
#include "condition.h"
#include "computer.h"
#include "computer_session.h"
#include "construction.h"
#include "construction_group.h"
#include "contents_change_handler.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "cuboid_rectangle.h"
#include "current_map.h"
#include "cursesport.h" // IWYU pragma: keep
#include "damage.h"
#include "debug.h"
#include "debug_menu.h"
#include "dependency_tree.h"
#include "dialogue.h"
#include "dialogue_chatbin.h"
#include "diary.h"
#include "distraction_manager.h"
#include "editmap.h"
#include "effect.h"
#include "effect_on_condition.h"
#include "end_screen.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "fault.h"
#include "field.h"
#include "field_type.h"
#include "filesystem.h"
#include "flag.h"
#include "flexbuffer_json-inl.h"
#include "flexbuffer_json.h"
#include "flood_fill.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "game_ui.h"
#include "gamemode.h"
#include "gates.h"
#include "get_version.h"
#include "harvest.h"
#include "iexamine.h"
#include "imgui/imgui_stdlib.h"
#include "init.h"
#include "input.h"
#include "input_context.h"
#include "input_enums.h"
#include "input_popup.h"
#include "inventory.h"
#include "item.h"
#include "item_category.h"
#include "item_contents.h"
#include "item_location.h"
#include "item_pocket.h"
#include "item_search.h"
#include "item_stack.h"
#include "iteminfo_query.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "json.h"
#include "kill_tracker.h"
#include "level_cache.h"
#include "lightmap.h"
#include "line.h"
#include "live_view.h"
#include "loading_ui.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "main_menu.h"
#include "map.h"
#include "map_item_stack.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "mapsharing.h"
#include "maptile_fwd.h"
#include "memorial_logger.h"
#include "messages.h"
#include "mission.h"
#include "mod_manager.h"
#include "monexamine.h"
#include "mongroup.h"
#include "monster.h"
#include "monstergenerator.h"
#include "move_mode.h"
#include "mtype.h"
#include "npc.h"
#include "npctrade.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "panels.h"
#include "past_achievements_info.h"
#include "path_info.h"
#include "pathfinding.h"
#include "perf.h"
#include "pickup.h"
#include "player_activity.h"
#include "popup.h"
#include "profession.h"
#include "proficiency.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "ret_val.h"
#include "rng.h"
#include "safemode_ui.h"
#include "scenario.h"
#include "scent_map.h"
#include "scores_ui.h"
#include "sdltiles.h" // IWYU pragma: keep
#include "sounds.h"
#include "start_location.h"
#include "stats_tracker.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "talker.h"
#include "text_snippets.h"
#include "tileray.h"
#include "timed_event.h"
#include "translation.h"
#include "translation_cache.h"
#include "translations.h"
#include "trap.h"
#include "ui_helpers.h"
#include "ui_extended_description.h"
#include "ui_manager.h"
#include "uilist.h"
#include "uistate.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_appliance.h"
#include "veh_interact.h"
#include "veh_type.h"
#include "vehicle.h"
#include "viewer.h"
#include "visitable.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "wcwidth.h"
#include "weakpoint.h"
#include "weather.h"
#include "weather_type.h"
#include "worldfactory.h"
#include "zzip.h"

#if defined(TILES)
#include "sdl_utils.h"
#endif // TILES

#if defined(__clang__) || defined(__GNUC__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

static const ascii_art_id ascii_art_ascii_tombstone( "ascii_tombstone" );

static const climbing_aid_id climbing_aid_default( "default" );

static const efftype_id effect_blind( "blind" );

static const flag_id json_flag_LEVITATION( "LEVITATION" );
static const flag_id json_flag_NO_RELOAD( "NO_RELOAD" );
static const flag_id json_flag_WATERWALKING( "WATERWALKING" );

static const itype_id itype_disassembly( "disassembly" );

static const quality_id qual_BUTCHER( "BUTCHER" );
static const quality_id qual_CUT_FINE( "CUT_FINE" );

static const skill_id skill_gun( "gun" );

static const ter_str_id ter_t_grave_new( "t_grave_new" );
static const ter_str_id ter_t_pit( "t_pit" );
static const ter_str_id ter_t_pit_shallow( "t_pit_shallow" );

static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_INATTENTIVE( "INATTENTIVE" );
static const trait_id trait_NPC_STARTING_NPC( "NPC_STARTING_NPC" );
static const trait_id trait_NPC_STATIC_NPC( "NPC_STATIC_NPC" );

#if defined(TILES)
#include "cata_tiles.h"
#endif // TILES

#if !defined(TILES)

void reinitialize_framebuffer( const bool /*force_invalidate*/ )
{
}

void to_map_font_dim_width( int & )
{
}

void to_map_font_dim_height( int & )
{
}

void to_map_font_dimension( int &, int & )
{
}
void from_map_font_dimension( int &, int & )
{
}
void to_overmap_font_dimension( int &, int & )
{
}

#endif

#if defined(TUI)
// in ncurses_def.cpp
extern void check_encoding(); // NOLINT
extern void ensure_term_size(); // NOLINT
#endif

void game_ui::init_ui()
{
    // clear the screen
    static bool first_init = true;

    if( first_init ) {
#if defined(TUI)
        check_encoding();
#endif

        first_init = false;

#if defined(TILES)
        //class variable to track the option being active
        //only set once, toggle action is used to change during game
        pixel_minimap_option = get_option<bool>( "PIXEL_MINIMAP" );
        if( get_option<std::string>( "PIXEL_MINIMAP_BG" ) == "theme" ) {
            SDL_Color pixel_minimap_color = curses_color_to_SDL( c_black );
            pixel_minimap_r = pixel_minimap_color.r;
            pixel_minimap_g = pixel_minimap_color.g;
            pixel_minimap_b = pixel_minimap_color.b;
            pixel_minimap_a = pixel_minimap_color.a;
        } else {
            pixel_minimap_r = 0x00;
            pixel_minimap_g = 0x00;
            pixel_minimap_b = 0x00;
            pixel_minimap_a = 0xFF;
        }
#endif // TILES
    }

    // First get TERMX, TERMY
#if !defined(TUI)
    TERMX = get_terminal_width();
    TERMY = get_terminal_height();

    get_options().get_option( "TERMINAL_X" ).setValue( TERMX * get_scaling_factor() );
    get_options().get_option( "TERMINAL_Y" ).setValue( TERMY * get_scaling_factor() );
    get_options().save();
#else
    TERMY = getmaxy( catacurses::stdscr );
    TERMX = getmaxx( catacurses::stdscr );

    ensure_term_size();

    // try to make FULL_SCREEN_HEIGHT symmetric according to TERMY
    if( TERMY % 2 ) {
        FULL_SCREEN_HEIGHT = EVEN_MINIMUM_TERM_HEIGHT + 1;
    } else {
        FULL_SCREEN_HEIGHT = EVEN_MINIMUM_TERM_HEIGHT;
    }
#endif
}

void game::toggle_fullscreen()
{
#if !defined(TILES)
    fullscreen = !fullscreen;
    mark_main_ui_adaptor_resize();
#else
    toggle_fullscreen_window();
#endif
}

void game::toggle_pixel_minimap() const
{
#if defined(TILES)
    if( pixel_minimap_option ) {
        clear_window_area( w_pixel_minimap );
    }
    pixel_minimap_option = !pixel_minimap_option;
    mark_main_ui_adaptor_resize();
#endif // TILES
}

bool game::is_tileset_isometric() const
{
#if defined(TILES)
    return use_tiles && tilecontext && tilecontext->is_isometric();
#else
    return false;
#endif
}

void game::reload_tileset()
{
#if defined(TILES)
    // Disable UIs below to avoid accessing the tile context during loading.
    ui_adaptor ui( ui_adaptor::disable_uis_below{} );
    try {
        closetilecontext->reinit();
        closetilecontext->load_tileset( get_option<std::string>( "TILES" ),
                                        /*precheck=*/false, /*force=*/true,
                                        /*pump_events=*/true, /*terrain=*/false );
        closetilecontext->do_tile_loading_report();

        tilecontext = closetilecontext;
    } catch( const std::exception &err ) {
        popup( _( "Loading the tileset failed: %s" ), err.what() );
    }
    if( use_far_tiles ) {
        try {
            if( fartilecontext->is_valid() ) {
                fartilecontext->reinit();
            }
            fartilecontext->load_tileset( get_option<std::string>( "DISTANT_TILES" ),
                                          /*precheck=*/false, /*force=*/true,
                                          /*pump_events=*/true, /*terrain=*/false );
            fartilecontext->do_tile_loading_report();
        } catch( const std::exception &err ) {
            popup( _( "Loading the zoomed out tileset failed: %s" ), err.what() );
        }
    }
    try {
        overmap_tilecontext->reinit();
        overmap_tilecontext->load_tileset( get_option<std::string>( "OVERMAP_TILES" ),
                                           /*precheck=*/false, /*force=*/true,
                                           /*pump_events=*/true, /*terrain=*/true );
        overmap_tilecontext->do_tile_loading_report();
    } catch( const std::exception &err ) {
        popup( _( "Loading the overmap tileset failed: %s" ), err.what() );
    }
    g->reset_zoom();
    g->mark_main_ui_adaptor_resize();
#endif // TILES
}

// temporarily switch out of fullscreen for functions that rely
// on displaying some part of the sidebar
void game::temp_exit_fullscreen()
{
    if( fullscreen ) {
        was_fullscreen = true;
        toggle_fullscreen();
    } else {
        was_fullscreen = false;
    }
}

void game::reenter_fullscreen()
{
    if( was_fullscreen ) {
        if( !fullscreen ) {
            toggle_fullscreen();
        }
    }
}

static hint_rating rate_action_change_side( const avatar &you, const item &it )
{
    if( !it.is_sided() ) {
        return hint_rating::cant;
    }

    return you.is_worn( it ) ? hint_rating::good : hint_rating::iffy;
}

static hint_rating rate_action_disassemble( avatar &you, const item &it )
{
    if( you.can_disassemble( it, you.crafting_inventory() ).success() ) {
        // Possible right now
        return hint_rating::good;
    } else if( it.is_disassemblable() ) {
        // Potentially possible, but we currently lack requirements
        return hint_rating::iffy;
    } else {
        // Never possible
        return hint_rating::cant;
    }
}

static hint_rating rate_action_view_recipe( avatar &you, const item &it )
{
    if( it.is_craft() ) {
        const recipe &craft_recipe = it.get_making();
        if( craft_recipe.is_null() || !craft_recipe.ident().is_valid() ) {
            return hint_rating::cant;
        } else if( you.get_group_available_recipes().contains( &craft_recipe ) ) {
            return hint_rating::good;
        }
    } else {
        itype_id item = it.typeId();
        bool is_byproduct = false;  // product or byproduct
        bool can_craft = false;
        // Does a recipe for the item exist?
        for( const auto& [_, r] : recipe_dict ) {
            if( !r.obsolete && ( item == r.result() || r.in_byproducts( item ) ) ) {
                is_byproduct = true;
                // If a recipe exists, does my group know it?
                if( you.get_group_available_recipes().contains( &r ) ) {
                    can_craft = true;
                    break;
                }
            }
        }
        if( !is_byproduct ) {
            return hint_rating::cant;
        } else if( can_craft ) {
            return hint_rating::good;
        }
    }
    return hint_rating::iffy;
}

static void view_recipe_crafting_menu( const item &it )
{
    avatar &you = get_avatar();
    std::string itname;
    if( it.is_craft() ) {
        recipe_id id = it.get_making().ident();
        if( !you.get_group_available_recipes().contains( &id.obj() ) ) {
            add_msg( m_info, _( "You don't know how to craft the %s!" ), id->result_name() );
            return;
        }
        you.craft( std::nullopt, id );
        return;
    }
    itype_id item = it.typeId();
    itname = item->nname( 1U );

    bool is_byproduct = false;  // product or byproduct
    bool can_craft = false;
    // Does a recipe for the item exist?
    for( const auto& [_, r] : recipe_dict ) {
        if( !r.obsolete && ( item == r.result() || r.in_byproducts( item ) ) ) {
            is_byproduct = true;
            // If a recipe exists, does my group know it?
            if( you.get_group_available_recipes().contains( &r ) ) {
                can_craft = true;
                break;
            }
        }
    }
    if( !is_byproduct ) {
        add_msg( m_info, _( "You wonder if it's even possible to craft the %s…" ), itname );
        return;
    } else if( !can_craft ) {
        add_msg( m_info, _( "You don't know how to craft the %s!" ), itname );
        return;
    }

    std::string filterstring = string_format( "r:%s", itname );
    you.craft( std::nullopt, recipe_id(), filterstring );
}

static hint_rating rate_action_eat( const avatar &you, const item &it )
{
    if( it.is_container() ) {
        hint_rating best_rate = hint_rating::cant;
        it.visit_items( [&you, &best_rate]( item * node, item * ) {
            if( you.can_consume_as_is( *node ) ) {
                ret_val<edible_rating> rate = you.will_eat( *node );
                if( rate.success() ) {
                    best_rate = hint_rating::good;
                    return VisitResponse::ABORT;
                } else if( rate.value() != INEDIBLE && rate.value() != INEDIBLE_MUTATION ) {
                    best_rate = hint_rating::iffy;
                }
            }
            return VisitResponse::NEXT;
        } );
        return best_rate;
    }

    if( !you.can_consume_as_is( it ) ) {
        return hint_rating::cant;
    }

    const auto rating = you.will_eat( it );
    if( rating.success() ) {
        return hint_rating::good;
    } else if( rating.value() == INEDIBLE || rating.value() == INEDIBLE_MUTATION ) {
        return hint_rating::cant;
    }

    return hint_rating::iffy;
}

static hint_rating rate_action_collapse( const item &it )
{
    for( const item_pocket *pocket : it.get_all_standard_pockets() ) {
        if( !pocket->settings.is_collapsed() ) {
            return hint_rating::good;
        }
    }
    return hint_rating::cant;
}

static hint_rating rate_action_expand( const item &it )
{
    for( const item_pocket *pocket : it.get_all_standard_pockets() ) {
        if( pocket->settings.is_collapsed() ) {
            return hint_rating::good;
        }
    }
    return hint_rating::cant;
}

static hint_rating rate_action_mend( const avatar &, const item &it )
{
    // TODO: check also if item damage could be repaired via a tool
    if( !it.faults.empty() ) {
        return hint_rating::good;
    }
    return it.faults_potential().empty() ? hint_rating::cant : hint_rating::iffy;
}

static hint_rating rate_action_read( const avatar &you, const item &it )
{
    if( !it.is_book() ) {
        return hint_rating::cant;
    }

    std::vector<std::string> dummy;
    return you.get_book_reader( it, dummy ) ? hint_rating::good : hint_rating::iffy;
}

static hint_rating rate_action_take_off( const avatar &you, const item &it )
{
    if( !it.is_armor() || it.has_flag( flag_NO_TAKEOFF ) || it.has_flag( flag_INTEGRATED ) ) {
        return hint_rating::cant;
    }

    if( you.is_worn( it ) ) {
        return hint_rating::good;
    }

    return hint_rating::iffy;
}

static hint_rating rate_action_use( const avatar &you, const item &it )
{
    if( it.is_container() && it.item_has_uses_recursive() ) {
        return hint_rating::good;
    }
    if( it.is_broken() ) {
        return hint_rating::iffy;
    } else if( it.is_tool() ) {
        return it.ammo_sufficient( &you ) ? hint_rating::good : hint_rating::iffy;
    } else if( it.is_gunmod() ) {
        /** @EFFECT_GUN >0 allows rating estimates for gun modifications */
        if( static_cast<int>( you.get_skill_level( skill_gun ) ) == 0 ) {
            return hint_rating::iffy;
        } else {
            return hint_rating::good;
        }
    } else if( it.is_food() || it.is_medication() || it.is_book() || it.is_armor() ) {
        if( it.is_medication() && !you.can_use_heal_item( it ) ) {
            return hint_rating::cant;
        }
        if( it.is_comestible() && it.is_frozen_liquid() ) {
            return hint_rating::cant;
        }
        // The rating is subjective, could be argued as hint_rating::cant or hint_rating::good as well
        return hint_rating::iffy;
    } else if( it.type->has_use() ) {
        return hint_rating::good;
    }

    return hint_rating::cant;
}

static hint_rating rate_action_wear( const avatar &you, const item &it )
{
    if( !it.is_armor() ) {
        return hint_rating::cant;
    }

    if( you.is_worn( it ) ) {
        return hint_rating::iffy;
    }

    return you.can_wear( it ).success() ? hint_rating::good : hint_rating::iffy;
}

static hint_rating rate_action_wield( const avatar &you, const item &it )
{
    return you.can_wield( it ).success() ? hint_rating::good : hint_rating::iffy;
}

hint_rating game::rate_action_insert( const avatar &you, const item_location &loc )
{
    if( loc->will_spill_if_unsealed()
        && loc.where() != item_location::type::map
        && !you.is_wielding( *loc ) ) {

        return hint_rating::cant;
    }
    if( loc.get_item()->has_flag( json_flag_NO_RELOAD ) ) {
        return hint_rating::cant;
    }
    return hint_rating::good;
}

/* item submenu for 'i' and '/'
* It use draw_item_info to draw item info and action menu
*
* @param locThisItem the item
* @param iStartX Left coordinate of the item info window
* @param iWidth width of the item info window (height = height of terminal)
* @return getch
*/
int game::inventory_item_menu( item_location locThisItem,
                               const std::function<int()> &iStartX,
                               const std::function<int()> &iWidth,
                               UNUSED const inventory_item_menu_position position )
{
    int cMenu = static_cast<int>( '+' );

    item &oThisItem = *locThisItem;
    // u.has_item(oThisItem) do not include mod pockets, where mounted flashlights are
    if( /* u.has_item(oThisItem) */ true ) {
#if defined(__ANDROID__)
        if( get_option<bool>( "ANDROID_INVENTORY_AUTOADD" ) ) {
            add_key_to_quick_shortcuts( oThisItem.invlet, "INVENTORY", false );
        }
#endif
        const bool bHPR = get_auto_pickup().has_rule( &oThisItem );
        std::vector<iteminfo> vThisItem;
        std::vector<iteminfo> vDummy;
        item_info_data data;
        int iScrollPos = 0;
        int iScrollHeight = 0;
        uilist action_menu;
        std::unique_ptr<ui_adaptor> ui;

        bool exit = false;
        bool first_execution = true;
        static int lang_version = detail::get_current_language_version();
        catacurses::window w_info;
        do {
            //lang check here is needed to redraw the menu when using "Toggle language to English" option
            if( first_execution || lang_version != detail::get_current_language_version() ) {

                const hint_rating rate_drop_item = u.get_wielded_item() &&
                                                   u.get_wielded_item()->has_flag( flag_NO_UNWIELD ) ?
                                                   hint_rating::cant : hint_rating::good;
                action_menu.reset();
                action_menu.allow_anykey = true;
                float popup_width = 0.0;
                const auto addentry = [&]( const char key, const std::string & text, const hint_rating hint ) {
                    popup_width = std::max( popup_width, ImGui::CalcTextSize( text.c_str() ).x );
                    // The char is used as retval from the uilist *and* as hotkey.
                    action_menu.addentry( key, true, key, text );
                    auto &entry = action_menu.entries.back();
                    switch( hint ) {
                        case hint_rating::cant:
                            entry.text_color = c_light_gray;
                            break;
                        case hint_rating::iffy:
                            entry.text_color = c_light_red;
                            break;
                        case hint_rating::good:
                            entry.text_color = c_light_green;
                            break;
                    }
                };
                addentry( 'a', pgettext( "action", "activate" ), rate_action_use( u, oThisItem ) );
                addentry( 'R', pgettext( "action", "read" ), rate_action_read( u, oThisItem ) );
                addentry( 'E', pgettext( "action", "eat" ), rate_action_eat( u, oThisItem ) );
                addentry( 'W', pgettext( "action", "wear" ), rate_action_wear( u, oThisItem ) );
                addentry( 'w', pgettext( "action", "wield" ), rate_action_wield( u, oThisItem ) );
                addentry( 't', pgettext( "action", "throw" ), rate_action_wield( u, oThisItem ) );
                addentry( 'c', pgettext( "action", "change side" ), rate_action_change_side( u, oThisItem ) );
                addentry( 'T', pgettext( "action", "take off" ), rate_action_take_off( u, oThisItem ) );
                addentry( 'd', pgettext( "action", "drop" ), rate_drop_item );
                addentry( 'U', pgettext( "action", "unload" ), u.rate_action_unload( oThisItem ) );
                addentry( 'r', pgettext( "action", "reload" ), u.rate_action_reload( oThisItem ) );
                addentry( 'p', pgettext( "action", "part reload" ), u.rate_action_reload( oThisItem ) );
                addentry( 'm', pgettext( "action", "mend" ), rate_action_mend( u, oThisItem ) );
                addentry( 'D', pgettext( "action", "disassemble" ), rate_action_disassemble( u, oThisItem ) );
                if( oThisItem.is_container() && !oThisItem.is_corpse() ) {
                    addentry( 'i', pgettext( "action", "insert" ), rate_action_insert( u, locThisItem ) );
                    if( oThisItem.num_item_stacks() > 0 ) {
                        addentry( 'o', pgettext( "action", "open" ), hint_rating::good );
                    }
                    addentry( 'v', pgettext( "action", "pocket settings" ), hint_rating::good );
                }
                addentry( 'f', oThisItem.is_favorite ? pgettext( "action", "unfavorite" ) : pgettext( "action",
                          "favorite" ),
                          hint_rating::good );
                addentry( 'V', pgettext( "action", "view recipe" ), rate_action_view_recipe( u, oThisItem ) );
                addentry( '>', pgettext( "action", "hide contents" ), rate_action_collapse( oThisItem ) );
                addentry( '<', pgettext( "action", "show contents" ), rate_action_expand( oThisItem ) );
                addentry( '=', pgettext( "action", "reassign" ), hint_rating::good );

                if( bHPR ) {
                    addentry( '-', _( "Auto pickup" ), hint_rating::iffy );
                } else {
                    addentry( '+', _( "Auto pickup" ), hint_rating::good );
                }

                oThisItem.info( true, vThisItem );

                popup_width += ImGui::CalcTextSize( " [X] " ).x + 2 * ( ImGui::GetStyle().WindowPadding.x +
                               ImGui::GetStyle().WindowBorderSize );
                // Filtering isn't needed, the number of entries is manageable.
                action_menu.filtering = false;
                // Default menu border color is different, this matches the border of the item info window.
                action_menu.border_color = BORDER_COLOR;

                data = item_info_data( oThisItem.tname(), oThisItem.type_name(), vThisItem, vDummy, iScrollPos );
                data.without_getch = true;

                ui = std::make_unique<ui_adaptor>();
                float x = 0.0;
                ui->on_screen_resize( [&]( ui_adaptor & ui ) {
                    w_info = catacurses::newwin( TERMY, iWidth(), point( iStartX(), 0 ) );
                    iScrollHeight = TERMY - 2;
                    ui.position_from_window( w_info );

                    switch( position ) {
                        default:
                        case RIGHT_TERMINAL_EDGE:
                            x = 0.0;
                            break;
                        case LEFT_OF_INFO:
                            x = ( iStartX() * ImGui::CalcTextSize( "X" ).x ) - popup_width;
                            break;
                        case RIGHT_OF_INFO:
                            x = ( iStartX() + iWidth() ) * ImGui::CalcTextSize( "X" ).x;
                            break;
                        case LEFT_TERMINAL_EDGE:
                            x = ImGui::GetMainViewport()->Size.x - popup_width;
                            break;
                    }
                    action_menu.desired_bounds = { x, 0.0, popup_width, -1.0 };
                    action_menu.reposition();
                } );
                ui->mark_resize();

                ui->on_redraw( [&]( const ui_adaptor & ) {
                    draw_item_info( w_info, data );
                } );

                action_menu.additional_actions = {
                    { "RIGHT", translation() }
                };

                lang_version = detail::get_current_language_version();
                first_execution = false;
            }

            const int prev_selected = action_menu.selected;
            action_menu.query( true );
            if( action_menu.ret >= 0 ) {
                cMenu = action_menu.ret; /* Remember: hotkey == retval, see addentry above. */
            } else if( action_menu.ret == UILIST_UNBOUND && action_menu.ret_act == "RIGHT" ) {
                // Simulate KEY_RIGHT == '\n' (confirm currently selected entry) for compatibility with old version.
                // TODO: ideally this should be done in the uilist, maybe via a callback.
                cMenu = action_menu.ret = action_menu.entries[action_menu.selected].retval;
            } else if( action_menu.ret_act == "PAGE_UP" || action_menu.ret_act == "PAGE_DOWN" ) {
                cMenu = action_menu.ret_act == "PAGE_UP" ? KEY_PPAGE : KEY_NPAGE;
                // Prevent the menu from scrolling with this key. TODO: Ideally the menu
                // could be instructed to ignore these two keys instead of scrolling.
                action_menu.selected = prev_selected;
                action_menu.fselected = prev_selected;
            } else {
                cMenu = 0;
            }

            if( action_menu.ret != UILIST_WAIT_INPUT && action_menu.ret != UILIST_UNBOUND ) {
                exit = true;
                ui = nullptr;
            }

            switch( cMenu ) {
                case 'a': {
                    contents_change_handler handler;
                    handler.unseal_pocket_containing( locThisItem );
                    if( locThisItem.get_item()->type->has_use() &&
                        !locThisItem.get_item()->item_has_uses_recursive( true ) ) { // NOLINT(bugprone-branch-clone)
                        // Item has uses and none of its contents (if any) has uses.
                        avatar_action::use_item( u, locThisItem );
                    } else if( locThisItem.get_item()->item_has_uses_recursive() ) {
                        game::item_action_menu( locThisItem );
                    } else if( locThisItem.get_item()->has_relic_activation() ) {
                        avatar_action::use_item( u, locThisItem );
                    } else {
                        add_msg( m_info, _( "You can't use a %s there." ), locThisItem->tname() );
                        break;
                    }
                    handler.handle_by( u );
                    break;
                }
                case 'E':
                    if( !locThisItem.get_item()->is_container() ) {
                        avatar_action::eat( u, locThisItem );
                    } else {
                        avatar_action::eat_or_use( u, game_menus::inv::consume( locThisItem ) );
                    }
                    break;
                case 'W': {
                    contents_change_handler handler;
                    handler.unseal_pocket_containing( locThisItem );
                    u.wear( locThisItem );
                    handler.handle_by( u );
                    break;
                }
                case 'w':
                    if( u.can_wield( *locThisItem ).success() ) {
                        contents_change_handler handler;
                        handler.unseal_pocket_containing( locThisItem );
                        u.wield( locThisItem );
                        handler.handle_by( u );
                    } else {
                        add_msg( m_info, "%s", u.can_wield( *locThisItem ).c_str() );
                    }
                    break;
                case 't': {
                    contents_change_handler handler;
                    handler.unseal_pocket_containing( locThisItem );
                    avatar_action::plthrow( u, locThisItem );
                    handler.handle_by( u );
                    break;
                }
                case 'c':
                    u.change_side( locThisItem );
                    break;
                case 'T':
                    u.takeoff( locThisItem.obtain( u ) );
                    break;
                case 'd':
                    u.drop( locThisItem, u.pos_bub() );
                    break;
                case 'U':
                    u.unload( locThisItem );
                    break;
                case 'r':
                    reload( locThisItem );
                    break;
                case 'p':
                    reload( locThisItem, true );
                    break;
                case 'm':
                    avatar_action::mend( u, locThisItem );
                    break;
                case 'R':
                    u.read( locThisItem );
                    break;
                case 'D':
                    u.disassemble( locThisItem, false );
                    break;
                case 'f':
                    oThisItem.is_favorite = !oThisItem.is_favorite;
                    if( locThisItem.has_parent() ) {
                        item_location parent = locThisItem.parent_item();
                        item_pocket *const pocket = locThisItem.parent_pocket();
                        if( pocket ) {
                            pocket->restack();
                        } else {
                            debugmsg( "parent container does not contain item" );
                        }
                    }
                    break;
                case 'v':
                    if( oThisItem.is_container() ) {
                        oThisItem.favorite_settings_menu();
                    }
                    break;
                case 'V': {
                    view_recipe_crafting_menu( oThisItem );
                    break;
                }
                case 'i':
                    if( oThisItem.is_container() ) {
                        game_menus::inv::insert_items( locThisItem );
                    }
                    break;
                case 'o':
                    if( oThisItem.is_container() && oThisItem.num_item_stacks() > 0 ) {
                        game_menus::inv::common( locThisItem );
                    }
                    break;
                case '=':
                    game_menus::inv::reassign_letter( oThisItem );
                    break;
                case KEY_PPAGE:
                    iScrollPos -= iScrollHeight;
                    if( ui ) {
                        ui->invalidate_ui();
                    }
                    break;
                case KEY_NPAGE:
                    iScrollPos += iScrollHeight;
                    if( ui ) {
                        ui->invalidate_ui();
                    }
                    break;
                case '+':
                    if( !bHPR ) {
                        get_auto_pickup().add_rule( &oThisItem, true );
                        add_msg( m_info, _( "'%s' added to character pickup rules." ), oThisItem.tname( 1,
                                 false ) );
                    }
                    break;
                case '-':
                    if( bHPR ) {
                        get_auto_pickup().remove_rule( &oThisItem );
                        add_msg( m_info, _( "'%s' removed from character pickup rules." ), oThisItem.tname( 1,
                                 false ) );
                    }
                    break;
                case '<':
                case '>':
                    for( item_pocket *pocket : oThisItem.get_all_standard_pockets() ) {
                        pocket->settings.set_collapse( cMenu == '>' );
                    }
                    break;
                default:
                    break;
            }
        } while( !exit );
    }
    return cMenu;
}

// Checks input to see if mouse was moved and handles the mouse view box accordingly.
// Returns true if input requires breaking out into a game action.
bool game::handle_mouseview( input_context &ctxt, std::string &action )
{
    std::optional<tripoint_bub_ms> liveview_pos;

    do {
        action = ctxt.handle_input();
        if( action == "MOUSE_MOVE" ) {
            const std::optional<tripoint_bub_ms> mouse_pos = ctxt.get_coordinates( w_terrain,
                    ter_view_p.raw().xy(),
                    true );
            if( mouse_pos && ( !liveview_pos || *mouse_pos != *liveview_pos ) ) {
                liveview_pos = mouse_pos;
                liveview.show( liveview_pos->raw() );
            } else if( !mouse_pos ) {
                liveview_pos.reset();
                liveview.hide();
            }
            ui_manager::redraw();
        }
    } while( action == "MOUSE_MOVE" ); // Freeze animation when moving the mouse

    if( action != "TIMEOUT" ) {
        // Keyboard event, break out of animation loop
        liveview.hide();
        return false;
    }

    // Mouse movement or un-handled key
    return true;
}

std::pair<tripoint_rel_ms, tripoint_rel_ms> game::mouse_edge_scrolling( input_context &ctxt,
        const int speed,
        const tripoint_rel_ms &last, bool iso )
{
    const int rate = get_option<int>( "EDGE_SCROLL" );
    std::pair<tripoint_rel_ms, tripoint_rel_ms> ret = std::make_pair( tripoint_rel_ms::zero, last );
    if( rate == -1 ) {
        // Fast return when the option is disabled.
        return ret;
    }
    // Ensure the parameters are used even if the #if below is false
    ( void )ctxt;
    ( void )speed;
    ( void )iso;
#if !defined(TUI)
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if( now < last_mouse_edge_scroll + std::chrono::milliseconds( rate ) ) {
        return ret;
    } else {
        last_mouse_edge_scroll = now;
    }
    const input_event event = ctxt.get_raw_input();
    if( event.type == input_event_t::mouse ) {
        const point_rel_ms threshold( projected_window_width() / 100, projected_window_height() / 100 );
        if( event.mouse_pos.x <= threshold.x() ) {
            ret.first.x() -= speed;
            if( iso ) {
                ret.first.y() -= speed;
            }
        } else if( event.mouse_pos.x >= projected_window_width() - threshold.x() ) {
            ret.first.x() += speed;
            if( iso ) {
                ret.first.y() += speed;
            }
        }
        if( event.mouse_pos.y <= threshold.y() ) {
            ret.first.y() -= speed;
            if( iso ) {
                ret.first.x() += speed;
            }
        } else if( event.mouse_pos.y >= projected_window_height() - threshold.y() ) {
            ret.first.y() += speed;
            if( iso ) {
                ret.first.x() -= speed;
            }
        }
        ret.second = ret.first;
    } else if( event.type == input_event_t::timeout ) {
        ret.first = ret.second;
    }
#endif
    return ret;
}

std::pair<tripoint_rel_omt, tripoint_rel_omt> game::mouse_edge_scrolling( input_context &ctxt,
        const int speed,
        const tripoint_rel_omt &last, bool iso )
{
    // It's the same operation technically, just using different coordinate systems.
    const std::pair<tripoint_rel_ms, tripoint_rel_ms> temp = game::mouse_edge_scrolling( ctxt, speed,
            tripoint_rel_ms( last.raw() ), iso );

    return { tripoint_rel_omt( temp.first.raw() ), tripoint_rel_omt( temp.second.raw() ) };
}

tripoint_rel_ms game::mouse_edge_scrolling_terrain( input_context &ctxt )
{
    std::pair<tripoint_rel_ms, tripoint_rel_ms> ret = mouse_edge_scrolling( ctxt,
            std::max( DEFAULT_TILESET_ZOOM / tileset_zoom, 1 ),
            last_mouse_edge_scroll_vector_terrain, g->is_tileset_isometric() );
    last_mouse_edge_scroll_vector_terrain = ret.second;
    last_mouse_edge_scroll_vector_overmap = tripoint_rel_omt::zero;
    return ret.first;
}

tripoint_rel_omt game::mouse_edge_scrolling_overmap( input_context &ctxt )
{
    // overmap has no iso mode
    auto ret = mouse_edge_scrolling( ctxt, 2, last_mouse_edge_scroll_vector_overmap, false );
    last_mouse_edge_scroll_vector_overmap = ret.second;
    last_mouse_edge_scroll_vector_terrain = tripoint_rel_ms::zero;
    return ret.first;
}

bool game::try_get_left_click_action( action_id &act, const tripoint_bub_ms &mouse_target )
{
    bool new_destination = true;
    if( !destination_preview.empty() ) {
        auto &final_destination = destination_preview.back();
        if( final_destination.xy() == mouse_target.xy() ) {
            // Second click
            new_destination = false;
            u.set_destination( destination_preview );
            destination_preview.clear();
            act = u.get_next_auto_move_direction();
            if( act == ACTION_NULL ) {
                // Something went wrong
                u.clear_destination();
                return false;
            }
        }
    }

    if( new_destination ) {
        const std::optional<std::vector<tripoint_bub_ms>> try_route =
        safe_route_to( u, mouse_target, 0, []( const std::string & msg ) {
            add_msg( msg );
        } );
        if( try_route.has_value() ) {
            destination_preview = *try_route;
            return true;
        }
        return false;
    }

    return true;
}

bool game::try_get_right_click_action( action_id &act, const tripoint_bub_ms &mouse_target )
{
    map &here = get_map();

    const bool cleared_destination = !destination_preview.empty();
    u.clear_destination();
    destination_preview.clear();

    if( cleared_destination ) {
        // Produce no-op if auto move had just been cleared on this action
        // e.g. from a previous single left mouse click. This has the effect
        // of right-click canceling an auto move before it is initiated.
        return false;
    }

    const bool is_adjacent = square_dist( mouse_target.xy(), u.pos_bub().xy() ) <= 1;
    const bool is_self = square_dist( mouse_target.xy(), u.pos_bub().xy() ) <= 0;
    if( const monster *const mon = get_creature_tracker().creature_at<monster>( mouse_target ) ) {
        if( !u.sees( here, *mon ) ) {
            add_msg( _( "Nothing relevant here." ) );
            return false;
        }

        if( !u.get_wielded_item() || !u.get_wielded_item()->is_gun() ) {
            add_msg( m_info, _( "You are not wielding a ranged weapon." ) );
            return false;
        }

        // TODO: Add weapon range check. This requires weapon to be reloaded.

        act = ACTION_FIRE;
    } else if( is_adjacent &&
               here.close_door( tripoint_bub_ms( mouse_target.xy(), u.posz() ),
                                !here.is_outside( u.pos_bub() ), true ) ) {
        act = ACTION_CLOSE;
    } else if( is_self ) {
        act = ACTION_PICKUP;
    } else if( is_adjacent ) {
        act = ACTION_EXAMINE;
    } else {
        add_msg( _( "Nothing relevant here." ) );
        return false;
    }

    return true;
}

class end_screen_data;

class end_screen_data
{
        friend class end_screen_ui_impl;
    public:
        void draw_end_screen_ui();
};

class end_screen_ui_impl : public cataimgui::window
{
    public:
        std::string text;
        explicit end_screen_ui_impl() : cataimgui::window( _( "The End" ) ) {
        }
    protected:
        void draw_controls() override;
};

void end_screen_data::draw_end_screen_ui()
{
    input_context ctxt;
    ctxt.register_action( "TEXT.CONFIRM" );
#if defined(WIN32) || defined(TILES)
    ctxt.set_timeout( 50 );
#endif
    end_screen_ui_impl p_impl;

    while( true ) {
        ui_manager::redraw_invalidated();
        std::string action = ctxt.handle_input();
        if( action == "TEXT.CONFIRM" || !p_impl.get_is_open() ) {
            break;
        }
    }
    avatar &u = get_avatar();
    const bool is_suicide = g->uquit == QUIT_SUICIDE;
    get_event_bus().send<event_type::game_avatar_death>( u.getID(), u.name, is_suicide, p_impl.text );
}

void end_screen_ui_impl::draw_controls()
{
    avatar &u = get_avatar();
    ascii_art_id art = ascii_art_ascii_tombstone;
    dialogue d( get_talker_for( u ), nullptr );
    std::string input_label;
    std::vector<std::pair<std::pair<int, int>, std::string>> added_info;

    //Sort end_screens in order of decreasing priority
    std::vector<end_screen> sorted_screens = end_screen::get_all();
    std::sort( sorted_screens.begin(), sorted_screens.end(), []( end_screen const & a,
    end_screen const & b ) {
        return a.priority > b.priority;
    } );

    for( const end_screen &e_screen : sorted_screens ) {
        if( e_screen.condition( d ) ) {
            art = e_screen.picture_id;
            if( !e_screen.added_info.empty() ) {
                added_info = e_screen.added_info;
            }
            if( !e_screen.last_words_label.empty() ) {
                input_label = e_screen.last_words_label;
            }
            break;
        }
    }

    if( art.is_valid() ) {
        cataimgui::PushMonoFont();
        int row = 1;
        for( const std::string &line : art->picture ) {
            cataimgui::draw_colored_text( line );

            for( std::pair<std::pair<int, int>, std::string> info : added_info ) {
                if( row == info.first.second ) {
                    parse_tags( info.second, u, u );
                    ImGui::SameLine( str_width_to_pixels( info.first.first ), 0 );
                    cataimgui::draw_colored_text( info.second );
                }
            }
            row++;
        }
        ImGui::PopFont();
    }

    if( !input_label.empty() ) {
        ImGui::NewLine();
        ImGui::AlignTextToFramePadding();
        cataimgui::draw_colored_text( input_label );
        ImGui::SameLine( str_width_to_pixels( input_label.size() + 2 ), 0 );
        ImGui::InputText( "##LAST_WORD_BOX", &text );
        ImGui::SetKeyboardFocusHere( -1 );
    }

}

void game::bury_screen() const
{
    end_screen_data new_instance;
    new_instance.draw_end_screen_ui();

    sfx::do_player_death_hurt( get_player_character(), true );
    sfx::fade_audio_group( sfx::group::weather, 2000 );
    sfx::fade_audio_group( sfx::group::time_of_day, 2000 );
    sfx::fade_audio_group( sfx::group::context_themes, 2000 );
    sfx::fade_audio_group( sfx::group::low_stamina, 2000 );
}

void game::death_screen()
{
    gamemode->game_over();
    Messages::display_messages();
    u.get_avatar_diary()->death_entry();
    show_scores_ui();
    disp_NPC_epilogues();
    follower_ids.clear();
    display_faction_epilogues();
}

void game::disp_NPC_epilogues()
{
    // TODO: This search needs to be expanded to all NPCs
    for( character_id elem : follower_ids ) {
        shared_ptr_fast<npc> guy = overmap_buffer.find_npc( elem );
        if( !guy ) {
            continue;
        }
        const auto new_win = []() {
            return catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                       point( std::max( 0, ( TERMX - FULL_SCREEN_WIDTH ) / 2 ),
                                              std::max( 0, ( TERMY - FULL_SCREEN_HEIGHT ) / 2 ) ) );
        };
        scrollable_text( new_win, guy->disp_name(), guy->get_epilogue() );
    }
}

void game::display_faction_epilogues()
{
    for( const auto &elem : faction_manager_ptr->all() ) {
        if( elem.second.known_by_u ) {
            const std::vector<std::string> epilogue = elem.second.epilogue();
            if( !epilogue.empty() ) {
                const auto new_win = []() {
                    return catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                               point( std::max( 0, ( TERMX - FULL_SCREEN_WIDTH ) / 2 ),
                                                      std::max( 0, ( TERMY - FULL_SCREEN_HEIGHT ) / 2 ) ) );
                };
                scrollable_text( new_win, elem.second.name,
                                 std::accumulate( epilogue.begin() + 1, epilogue.end(), epilogue.front(),
                []( std::string lhs, const std::string & rhs ) -> std::string {
                    return std::move( lhs ) + "\n" + rhs;
                } ) );
            }
        }
    }
}

struct npc_dist_to_player {
    const tripoint_abs_omt ppos{};
    npc_dist_to_player() : ppos( get_player_character().pos_abs_omt() ) {}
    // Operator overload required to leverage sort API.
    bool operator()( const shared_ptr_fast<npc> &a,
                     const shared_ptr_fast<npc> &b ) const {
        const tripoint_abs_omt apos = a->pos_abs_omt();
        const tripoint_abs_omt bpos = b->pos_abs_omt();
        return square_dist( ppos.xy(), apos.xy() ) <
               square_dist( ppos.xy(), bpos.xy() );
    }
};

void game::disp_NPCs()
{
    map &here = get_map();

    const tripoint_abs_omt ppos = u.pos_abs_omt();
    const tripoint_bub_ms lpos = u.pos_bub();
    const int scan_range = 120;
    std::vector<shared_ptr_fast<npc>> npcs = overmap_buffer.get_npcs_near_player( scan_range );
    std::sort( npcs.begin(), npcs.end(), npc_dist_to_player() );

    catacurses::window w;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        ui_helpers::full_screen_window( ui, &w );
    } );
    ui.mark_resize();
    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w );
        mvwprintz( w, point::zero, c_white, _( "Your overmap position: %s" ), ppos.to_string() );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( w, point( 0, 1 ), c_white, _( "Your local position: %s" ), lpos.to_string() );
        size_t i;
        int static_npc_count = 0;
        for( i = 0; i < npcs.size(); i++ ) {
            if(
                npcs[i]->has_trait( trait_NPC_STARTING_NPC ) || npcs[i]->has_trait( trait_NPC_STATIC_NPC ) ) {
                static_npc_count++;
            }
        }
        mvwprintz( w, point( 0, 2 ), c_white, _( "Total NPCs within %d OMTs: %d.  %d are static NPCs." ),
                   scan_range, npcs.size(), static_npc_count );
        for( i = 0; i < 20 && i < npcs.size(); i++ ) {
            const tripoint_abs_omt apos = npcs[i]->pos_abs_omt();
            mvwprintz( w, point( 0, i + 4 ), c_white, "%s: %s", npcs[i]->get_name(),
                       apos.to_string() );
        }
        wattron( w, c_white );
        for( const monster &m : all_monsters() ) {
            const tripoint_bub_ms m_pos = m.pos_bub( here );
            mvwprintw( w, point( 0, i + 4 ), "%s: %d, %d, %d", m.name(),
                       m_pos.x(), m_pos.y(), m.posz() );
            ++i;
        }
        wattroff( w, c_white );
        wnoutrefresh( w );
    } );

    input_context ctxt( "DISP_NPCS" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    bool stop = false;
    while( !stop ) {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( action == "CONFIRM" || action == "QUIT" ) {
            stop = true;
        }
    }
}

// A little helper to draw footstep glyphs.
static void draw_footsteps( const catacurses::window &window, const tripoint_rel_ms &offset )
{
    wattron( window, c_yellow );
    for( const tripoint_bub_ms &footstep : sounds::get_footstep_markers() ) {
        char glyph = '?';
        if( footstep.z() != offset.z() ) { // Here z isn't an offset, but a coordinate
            glyph = footstep.z() > offset.z() ? '^' : 'v';
        }

        mvwaddch( window, footstep.raw().xy() + offset.raw().xy(), glyph );
    }
    wattroff( window, c_yellow );
}

shared_ptr_fast<ui_adaptor> game::create_or_get_main_ui_adaptor()
{
    shared_ptr_fast<ui_adaptor> ui = main_ui_adaptor.lock();
    if( !ui ) {
        main_ui_adaptor = ui = make_shared_fast<ui_adaptor>();
        ui->on_redraw( []( ui_adaptor & ui ) {
            g->draw( ui );
        } );
        ui->on_screen_resize( [this]( ui_adaptor & ui ) {
            // remove some space for the sidebar, this is the maximal space
            // (using standard font) that the terrain window can have
            const int sidebar_left = panel_manager::get_manager().get_width_left();
            const int sidebar_right = panel_manager::get_manager().get_width_right();

            TERRAIN_WINDOW_HEIGHT = TERMY;
            TERRAIN_WINDOW_WIDTH = TERMX - ( sidebar_left + sidebar_right );
            TERRAIN_WINDOW_TERM_WIDTH = TERRAIN_WINDOW_WIDTH;
            TERRAIN_WINDOW_TERM_HEIGHT = TERRAIN_WINDOW_HEIGHT;

            /**
             * In tiles mode w_terrain can have a different font (with a different
             * tile dimension) or can be drawn by cata_tiles which uses tiles that again
             * might have a different dimension then the normal font used everywhere else.
             *
             * TERRAIN_WINDOW_WIDTH/TERRAIN_WINDOW_HEIGHT defines how many squares can
             * be displayed in w_terrain (using it's specific tile dimension), not
             * including partially drawn squares at the right/bottom. You should
             * use it whenever you want to draw specific squares in that window or to
             * determine whether a specific square is draw on screen (or outside the screen
             * and needs scrolling).
             *
             * TERRAIN_WINDOW_TERM_WIDTH/TERRAIN_WINDOW_TERM_HEIGHT defines the size of
             * w_terrain in the standard font dimension (the font that everything else uses).
             * You usually don't have to use it, expect for positioning of windows,
             * because the window positions use the standard font dimension.
             *
             * The code here calculates size available for w_terrain, caps it at
             * max_view_size (the maximal view range than any character can have at
             * any time).
             * It is stored in TERRAIN_WINDOW_*.
             */
            to_map_font_dimension( TERRAIN_WINDOW_WIDTH, TERRAIN_WINDOW_HEIGHT );

            // Position of the player in the terrain window, it is always in the center
            POSX = TERRAIN_WINDOW_WIDTH / 2;
            POSY = TERRAIN_WINDOW_HEIGHT / 2;

            w_terrain = w_terrain_ptr = catacurses::newwin( TERRAIN_WINDOW_HEIGHT, TERRAIN_WINDOW_WIDTH,
                                        point( sidebar_left, 0 ) );

            // minimap is always MINIMAP_WIDTH x MINIMAP_HEIGHT in size
            w_minimap = w_minimap_ptr = catacurses::newwin( MINIMAP_HEIGHT, MINIMAP_WIDTH, point::zero );

            // need to init in order to avoid crash. gets updated by the panel code.
            w_pixel_minimap = catacurses::newwin( 1, 1, point::zero );

            ui.position_from_window( catacurses::stdscr );
        } );
        ui->mark_resize();
    }
    return ui;
}

void game::invalidate_main_ui_adaptor() const
{
    shared_ptr_fast<ui_adaptor> ui = main_ui_adaptor.lock();
    if( ui ) {
        ui->invalidate_ui();
    }
}

void game::mark_main_ui_adaptor_resize() const
{
    shared_ptr_fast<ui_adaptor> ui = main_ui_adaptor.lock();
    if( ui ) {
        ui->mark_resize();
    }
}

game::draw_callback_t::draw_callback_t( const std::function<void()> &cb )
    : cb( cb )
{
}

game::draw_callback_t::~draw_callback_t()
{
    if( added ) {
        g->invalidate_main_ui_adaptor();
    }
}

void game::draw_callback_t::operator()()
{
    if( cb ) {
        cb();
    }
}

void game::add_draw_callback( const shared_ptr_fast<draw_callback_t> &cb )
{
    draw_callbacks.erase(
        std::remove_if( draw_callbacks.begin(), draw_callbacks.end(),
    []( const weak_ptr_fast<draw_callback_t> &cbw ) {
        return cbw.expired();
    } ),
    draw_callbacks.end()
    );
    draw_callbacks.emplace_back( cb );
    cb->added = true;
    invalidate_main_ui_adaptor();
}

static void draw_trail( const tripoint_bub_ms &start, const tripoint_bub_ms &end, bool bDrawX );

static shared_ptr_fast<game::draw_callback_t> create_trail_callback(
    const std::optional<tripoint_bub_ms> &trail_start,
    const std::optional<tripoint_bub_ms> &trail_end,
    const bool &trail_end_x
)
{
    return make_shared_fast<game::draw_callback_t>(
    [&]() {
        if( trail_start && trail_end ) {
            draw_trail( trail_start.value(), trail_end.value(), trail_end_x );
        }
    } );
}

void game::init_draw_async_anim_curses( const tripoint_bub_ms &p, const std::string &ncstr,
                                        const nc_color &nccol )
{
    std::pair <std::string, nc_color> anim( ncstr, nccol );
    async_anim_layer_curses[p] = anim;
}

void game::draw_async_anim_curses()
{
    map &here = get_map();
    const tripoint_bub_ms pos = u.pos_bub( here );

    // game::draw_async_anim_curses can be called multiple times, storing each animation to be played in async_anim_layer_curses
    // Iterate through every animation in async_anim_layer
    for( const auto &anim : async_anim_layer_curses ) {
        const tripoint_bub_ms p = anim.first - u.view_offset + tripoint( POSX - pos.x(), POSY - pos.y(),
                                  -pos.z() );
        const std::string ncstr = anim.second.first;
        const nc_color nccol = anim.second.second;

        mvwprintz( w_terrain, p.xy().raw(), nccol, ncstr );
    }
}

void game::void_async_anim_curses()
{
    async_anim_layer_curses.clear();
}

void game::init_draw_blink_curses( const tripoint_bub_ms &p, const std::string &ncstr,
                                   const nc_color &nccol )
{
    std::pair <std::string, nc_color> anim( ncstr, nccol );
    blink_layer_curses[p] = anim;
}

void game::draw_blink_curses()
{
    map &here = get_map();

    // game::draw_blink_curses can be called multiple times, storing each animation to be played in blink_layer_curses
    // Iterate through every animation in async_anim_layer
    const tripoint_bub_ms pos = u.pos_bub( here );

    for( const auto &anim : blink_layer_curses ) {
        const tripoint_bub_ms p = anim.first - u.view_offset + tripoint( POSX - pos.x(), POSY - pos.y(),
                                  -u.posz() );
        const std::string ncstr = anim.second.first;
        const nc_color nccol = anim.second.second;

        mvwprintz( w_terrain, p.raw().xy(), nccol, ncstr );
    }
}

void game::void_blink_curses()
{
    blink_layer_curses.clear();
}

bool game::has_blink_curses()
{
    return !blink_layer_curses.empty();
}

void game::draw( ui_adaptor &ui )
{
    map &here = get_map();

    if( test_mode ) {
        return;
    }

    ter_view_p.z() = ( u.pos_bub() + u.view_offset ).z();
    here.build_map_cache( ter_view_p.z() );
    here.update_visibility_cache( ter_view_p.z() );

    werase( w_terrain );
    void_blink_curses();
    draw_ter();
    for( auto it = draw_callbacks.begin(); it != draw_callbacks.end(); ) {
        shared_ptr_fast<draw_callback_t> cb = it->lock();
        if( cb ) {
            ( *cb )();
            ++it;
        } else {
            it = draw_callbacks.erase( it );
        }
    }
    draw_async_anim_curses();
    // Only draw blinking symbols when in active phase
    if( blink_active_phase ) {
        draw_blink_curses();
    }
    wnoutrefresh( w_terrain );

    draw_panels( true );

    // Ensure that the cursor lands on the character when everything is drawn.
    // This allows screen readers to describe the area around the player, making it
    // much easier to play with them
    // (e.g. for blind players)
    ui.set_cursor( w_terrain, -u.view_offset.xy().raw() + point( POSX, POSY ) );
}

void game::draw_panels( bool force_draw )
{
    static int previous_turn = -1;
    const int current_turn = to_turns<int>( calendar::turn - calendar::turn_zero );
    const bool draw_this_turn = current_turn > previous_turn || force_draw;
    panel_manager &mgr = panel_manager::get_manager();
    int y = 0;
    const bool sidebar_right = get_option<std::string>( "SIDEBAR_POSITION" ) == "right";
    int spacer = get_option<bool>( "SIDEBAR_SPACERS" ) ? 1 : 0;
    // Total up height used by all panels, and see what is left over for log
    int log_height = 0;
    for( const window_panel &panel : mgr.get_current_layout().panels() ) {
        // Skip height processing
        if( !panel.toggle || !panel.render() ) {
            continue;
        }
        // The panel with height -2 is the message log panel
        const int p_height = panel.get_height();
        if( p_height != -2 ) {
            log_height += p_height + spacer;
        }
    }
    log_height = std::max( TERMY - log_height, 3 );
    // Draw each panel having render() true
    for( const window_panel &panel : mgr.get_current_layout().panels() ) {
        if( panel.render() ) {
            // height clamped to window height.
            int h = std::min( panel.get_height(), TERMY - y );
            // The panel with height -2 is the message log panel
            if( h == -2 ) {
                h = log_height;
            }
            h += spacer;
            if( panel.toggle && panel.render() && h > 0 ) {
                if( panel.always_draw || draw_this_turn ) {
                    catacurses::window w = catacurses::newwin( h, panel.get_width(),
                                           point( sidebar_right ? TERMX - panel.get_width() : 0, y ) );
                    int tmp_h = panel.draw( { u, w, panel.get_widget() } );
                    h += tmp_h;
                    // lines skipped for rendering -> reclaim space in the sidebar
                    if( tmp_h < 0 ) {
                        y += h;
                        log_height -= tmp_h;
                        continue;
                    }
                }
                if( show_panel_adm ) {
                    const std::string panel_name = panel.get_name();
                    const int panel_name_width = utf8_width( panel_name );
                    catacurses::window label = catacurses::newwin( 1, panel_name_width, point( sidebar_right ?
                                               TERMX - panel.get_width() - panel_name_width - 1 : panel.get_width() + 1, y ) );
                    werase( label );
                    mvwprintz( label, point::zero, c_light_red, panel_name );
                    wnoutrefresh( label );
                    label = catacurses::newwin( h, 1,
                                                point( sidebar_right ? TERMX - panel.get_width() - 1 : panel.get_width(), y ) );
                    werase( label );
                    wattron( label, c_light_red );
                    if( h == 1 ) {
                        mvwaddch( label, point::zero, LINE_OXOX );
                    } else {
                        mvwaddch( label, point::zero, LINE_OXXX );
                        // NOLINTNEXTLINE(cata-use-named-point-constants)
                        mvwvline( label, point( 0, 1 ), LINE_XOXO, h - 2 );
                        mvwaddch( label, point( 0, h - 1 ), sidebar_right ? LINE_XXOO : LINE_XOOX );
                    }
                    wattroff( label, c_light_red );
                    wnoutrefresh( label );
                }
                y += h;
            }
        }
    }
    previous_turn = current_turn;
}

void game::draw_pixel_minimap( const catacurses::window &w )
{
    w_pixel_minimap = w;
}

void game::draw_critter( const Creature &critter, const tripoint_bub_ms &center )
{
    const map &here = get_map();

    const tripoint_bub_ms critter_pos = critter.pos_bub( here );
    const int my = POSY + ( critter_pos.y() - center.y() );
    const int mx = POSX + ( critter_pos.x() - center.x() );
    if( !is_valid_in_w_terrain( { mx, my } ) ) {
        return;
    }
    if( critter_pos.z() != center.z() ) {
        if( critter_pos.z() == center.z() - 1 &&
            ( debug_mode || u.sees( here, critter ) ) &&
            here.valid_move( critter_pos, critter_pos + tripoint::above, false, true ) ) {
            // Monster is below
            // TODO: Make this show something more informative than just green 'v'
            // TODO: Allow looking at this mon with look command
            init_draw_blink_curses( { critter_pos.xy(), center.z() }, "v", c_green_cyan );
        }
        if( critter_pos.z() == center.z() + 1 &&
            ( debug_mode || u.sees( here, critter ) ) &&
            here.valid_move( critter_pos, critter_pos + tripoint::below, false, true ) ) {
            // Monster is above
            init_draw_blink_curses( { critter_pos.xy(), center.z() }, "^", c_green_cyan );
        }
        return;
    }
    if( u.sees( here, critter ) || &critter == &u ) {
        critter.draw( w_terrain, point_bub_ms( center.xy() ), false );
        return;
    }
    const_dialogue d( get_const_talker_for( u ), get_const_talker_for( critter ) );
    const enchant_cache::special_vision sees_with_special = u.enchantment_cache->get_vision( d );
    if( !sees_with_special.is_empty() ) {
        const enchant_cache::special_vision_descriptions special_vis_desc =
            u.enchantment_cache->get_vision_description_struct( sees_with_special, d );
        mvwputch( w_terrain, point( mx, my ), special_vis_desc.color, special_vis_desc.symbol );
    }
}

bool game::is_in_viewport( const tripoint_bub_ms &p, int margin ) const
{
    map &here = get_map();

    const tripoint_rel_ms diff( u.pos_bub( here ) + u.view_offset - p );

    return ( std::abs( diff.x() ) <= getmaxx( w_terrain ) / 2 - margin ) &&
           ( std::abs( diff.y() ) <= getmaxy( w_terrain ) / 2 - margin );
}

void game::draw_ter( const bool draw_sounds )
{
    map &here = get_map();

    draw_ter( u.pos_bub( here ) + u.view_offset, is_looking,
              draw_sounds );
}

void game::draw_ter( const tripoint_bub_ms &center, const bool looking, const bool draw_sounds )
{
    map &here = get_map();
    const tripoint_bub_ms pos = u.pos_bub();

    ter_view_p = center;

    here.draw( w_terrain, tripoint_bub_ms( center ) );

    if( draw_sounds ) {
        draw_footsteps( w_terrain, tripoint_rel_ms( -center.x(), -center.y(), center.z() ) + point( POSX,
                        POSY ) );
    }

    for( Creature &critter : all_creatures() ) {
        draw_critter( critter, center );
    }

    if( !destination_preview.empty() && u.view_offset.z() == 0 ) {
        // Draw auto move preview trail
        const tripoint_bub_ms &final_destination = destination_preview.back();
        tripoint_bub_ms line_center = pos + u.view_offset;
        draw_line( final_destination, line_center, destination_preview, true );
        // TODO: fix point types
        mvwputch( w_terrain,
                  final_destination.xy().raw() - u.view_offset.xy().raw() +
                  point( POSX - pos.x(), POSY - pos.y() ), c_white, 'X' );
    }

    if( u.controlling_vehicle && !looking ) {
        draw_veh_dir_indicator( false );
        draw_veh_dir_indicator( true );
    }
}

std::optional<tripoint_rel_ms> game::get_veh_dir_indicator_location( bool next ) const
{
    map &here = get_map();

    if( !get_option<bool>( "VEHICLE_DIR_INDICATOR" ) ) {
        return std::nullopt;
    }
    const optional_vpart_position vp = here.veh_at( u.pos_bub() );
    if( !vp ) {
        return std::nullopt;
    }
    vehicle *const veh = &vp->vehicle();
    rl_vec2d face = next ? veh->dir_vec() : veh->face_vec();
    float r = 10.0f;
    return tripoint_rel_ms( static_cast<int>( r * face.x ), static_cast<int>( r * face.y ), u.posz() );
}

void game::draw_veh_dir_indicator( bool next )
{
    if( const std::optional<tripoint_rel_ms> indicator_offset = get_veh_dir_indicator_location(
                next ) ) {
        nc_color col = next ? c_white : c_dark_gray;
        mvwputch( w_terrain, indicator_offset->raw().xy() - u.view_offset.xy().raw() + point( POSX, POSY ),
                  col,
                  'X' );
    }
}

void game::draw_look_around_cursor( const tripoint_bub_ms &lp, const visibility_variables &cache )
{
    const map &here = get_map();

    if( !liveview.is_enabled() ) {
#if defined( TILES )
        if( is_draw_tiles_mode() ) {
            draw_cursor( lp );
            return;
        }
#endif
        const tripoint_bub_ms view_center = u.pos_bub() + u.view_offset;
        visibility_type visibility = visibility_type::HIDDEN;
        const bool inbounds = here.inbounds( lp );
        if( inbounds ) {
            visibility = here.get_visibility( here.apparent_light_at( lp, cache ), cache );
        }
        if( visibility == visibility_type::CLEAR ) {
            const Creature *const creature = get_creature_tracker().creature_at( lp, true );
            if( creature != nullptr && u.sees( here, *creature ) ) {
                creature->draw( w_terrain, view_center, true );
            } else {
                here.drawsq( w_terrain, lp, drawsq_params().highlight( true ).center( view_center ) );
            }
        } else {
            std::string visibility_indicator;
            nc_color visibility_indicator_color = c_white;
            switch( visibility ) {
                case visibility_type::CLEAR:
                    // Already handled by the outer if statement
                    break;
                case visibility_type::BOOMER:
                case visibility_type::BOOMER_DARK:
                    visibility_indicator = '#';
                    visibility_indicator_color = c_pink;
                    break;
                case visibility_type::DARK:
                    visibility_indicator = '#';
                    visibility_indicator_color = c_dark_gray;
                    break;
                case visibility_type::LIT:
                    visibility_indicator = '#';
                    visibility_indicator_color = c_light_gray;
                    break;
                case visibility_type::HIDDEN:
                    visibility_indicator = 'x';
                    visibility_indicator_color = c_white;
                    break;
            }

            const tripoint_rel_ms screen_pos = point_rel_ms( POSX, POSY ) + lp - view_center;
            mvwputch( w_terrain, screen_pos.raw().xy(), visibility_indicator_color, visibility_indicator );
        }
    }
}

void game::print_all_tile_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                                const std::string &area_name, int column,
                                int &line,
                                const int last_line,
                                const visibility_variables &cache )
{
    map &here = get_map();

    visibility_type visibility = visibility_type::HIDDEN;
    const bool inbounds = here.inbounds( lp );
    if( inbounds ) {
        visibility = here.get_visibility( here.apparent_light_at( lp, cache ), cache );
    }
    const Creature *creature = get_creature_tracker().creature_at( lp, true );
    switch( visibility ) {
        case visibility_type::CLEAR: {
            const optional_vpart_position vp = here.veh_at( lp );
            print_terrain_info( lp, w_look, area_name, column, line );
            print_fields_info( lp, w_look, column, line );
            print_trap_info( lp, w_look, column, line );
            print_part_con_info( lp, w_look, column, line );
            print_creature_info( creature, w_look, column, line, last_line );
            print_vehicle_info( veh_pointer_or_null( vp ), vp ? vp->part_index() : -1, w_look, column, line,
                                last_line );
            print_items_info( lp, w_look, column, line, last_line );
            print_graffiti_info( lp, w_look, column, line, last_line );
        }
        break;
        case visibility_type::BOOMER:
        case visibility_type::BOOMER_DARK:
        case visibility_type::DARK:
        case visibility_type::LIT:
        case visibility_type::HIDDEN:
            print_visibility_info( w_look, column, line, visibility );

            static std::string raw_description;
            static std::string parsed_description;
            if( creature != nullptr ) {
                const_dialogue d( get_const_talker_for( u ), get_const_talker_for( *creature ) );
                const enchant_cache::special_vision sees_with_special = u.enchantment_cache->get_vision( d );
                if( !sees_with_special.is_empty() ) {
                    // handling against re-evaluation and snippet replacement on redraw
                    if( raw_description.empty() ) {
                        const enchant_cache::special_vision_descriptions special_vis_desc =
                            u.enchantment_cache->get_vision_description_struct( sees_with_special, d );
                        raw_description = special_vis_desc.description.translated();
                        parse_tags( raw_description, *u.as_character(), *creature );
                        parsed_description = raw_description;
                    }
                    mvwprintw( w_look, point( 1, ++line ), parsed_description );
                }
            } else {
                raw_description.clear();
            }
            break;
    }
    print_debug_info( lp, w_look, column, line );
    if( !inbounds ) {
        return;
    }
    const int max_width = getmaxx( w_look ) - column - 1;

    std::string this_sound = sounds::sound_at( lp );
    if( !this_sound.empty() ) {
        const int lines = fold_and_print( w_look, point( 1, ++line ), max_width, c_light_gray,
                                          _( "From here you heard %s" ),
                                          this_sound );
        line += lines - 1;
    } else {
        // Check other z-levels
        tripoint_bub_ms tmp = lp;
        for( tmp.z() = -OVERMAP_DEPTH; tmp.z() <= OVERMAP_HEIGHT; tmp.z()++ ) {
            if( tmp.z() == lp.z() ) {
                continue;
            }

            std::string zlev_sound = sounds::sound_at( tmp );
            if( !zlev_sound.empty() ) {
                const int lines = fold_and_print( w_look, point( 1, ++line ), max_width, c_light_gray,
                                                  tmp.z() > lp.z() ?
                                                  _( "From above you heard %s" ) : _( "From below you heard %s" ), zlev_sound );
                line += lines - 1;
            }
        }
    }
}

void game::print_visibility_info( const catacurses::window &w_look, int column, int &line,
                                  visibility_type visibility )
{
    const char *visibility_message = nullptr;
    switch( visibility ) {
        case visibility_type::CLEAR:
            visibility_message = _( "Clearly visible." );
            break;
        case visibility_type::BOOMER:
            visibility_message = _( "A bright pink blur." );
            break;
        case visibility_type::BOOMER_DARK:
            visibility_message = _( "A pink blur." );
            break;
        case visibility_type::DARK:
            visibility_message = _( "Darkness." );
            break;
        case visibility_type::LIT:
            visibility_message = _( "Bright light." );
            break;
        case visibility_type::HIDDEN:
            visibility_message = _( "Unseen." );
            break;
    }

    mvwprintz( w_look, point( line, column ), c_light_gray, visibility_message );
    line += 2;
}

void game::print_terrain_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                               const std::string &area_name, int column, int &line )
{
    map &here = get_map();

    const int max_width = getmaxx( w_look ) - column - 1;

    // Print OMT type and terrain type on first two lines
    // if can't fit in one line.
    std::string tile = uppercase_first_letter( here.tername( lp ) );
    std::string area = uppercase_first_letter( area_name );
    if( const timed_event *e = get_timed_events().get( timed_event_type::OVERRIDE_PLACE ) ) {
        area = e->string_id;
    }
    mvwprintz( w_look, point( column, line++ ), c_yellow, area );
    mvwprintz( w_look, point( column, line++ ), c_white, tile );
    std::string desc = string_format( here.ter( lp ).obj().description );
    std::vector<std::string> lines = foldstring( desc, max_width );
    int numlines = lines.size();
    wattron( w_look, c_light_gray );
    for( int i = 0; i < numlines; i++ ) {
        mvwprintw( w_look, point( column, line++ ), lines[i] );
    }
    wattroff( w_look, c_light_gray );

    // Furniture, if any
    print_furniture_info( lp, w_look, column, line );

    // Cover percentage from terrain and furniture next.
    fold_and_print( w_look, point( column, ++line ), max_width, c_light_gray, _( "Concealment: %d%%" ),
                    here.coverage( lp ) );

    if( here.has_flag( ter_furn_flag::TFLAG_TREE, lp ) ) {
        const int lines = fold_and_print( w_look, point( column, ++line ), max_width, c_light_gray,
                                          _( "Can be <color_green>cut down</color> with the right tools." ) );
        line += lines - 1;
    }

    // Terrain and furniture flags next. These can be several lines for some combinations of
    // furnitures and terrains.
    lines = foldstring( here.features( lp ), max_width );
    numlines = lines.size();
    wattron( w_look, c_light_gray );
    for( int i = 0; i < numlines; i++ ) {
        mvwprintw( w_look, point( column, ++line ), lines[i] );
    }
    wattroff( w_look, c_light_gray );

    // Move cost from terrain and furniture and vehicle parts.
    // Vehicle part information is printed in a different function.
    if( here.impassable( lp ) ) {
        mvwprintz( w_look, point( column, ++line ), c_light_red, _( "Impassable" ) );
    } else {
        mvwprintz( w_look, point( column, ++line ), c_light_gray, _( "Move cost: %d" ),
                   here.move_cost( lp ) * 50 );
    }

    // Next print the string on any SIGN flagged furniture if any.
    std::string signage = here.get_signage( lp );
    if( !signage.empty() ) {
        std::string sign_string = u.has_trait( trait_ILLITERATE ) ? "???" : signage;
        const int lines = fold_and_print( w_look, point( column, ++line ), max_width, c_light_gray,
                                          _( "Sign: %s" ), sign_string );
        line += lines - 1;
    }

    // Print light level on the selected tile.
    std::pair<std::string, nc_color> ll = get_light_level( std::max( 1.0,
                                          LIGHT_AMBIENT_LIT - here.ambient_light_at( lp ) + 1.0 ) );
    mvwprintz( w_look, point( column, ++line ), c_light_gray, _( "Lighting: " ) );
    mvwprintz( w_look, point( column + utf8_width( _( "Lighting: " ) ), line ), ll.second, ll.first );

    // Print the terrain and any furntiure on the tile below and whether it is walkable.
    if( lp.z() > -OVERMAP_DEPTH && !here.has_floor( lp ) ) {
        tripoint_bub_ms below( lp + tripoint::below );
        std::string tile_below = here.tername( below );
        if( here.has_furn( below ) ) {
            tile_below += ", " + here.furnname( below );
        }

        if( !here.has_floor_or_support( lp ) ) {
            fold_and_print( w_look, point( column, ++line ), max_width, c_dark_gray,
                            _( "Below: %s; No support" ), tile_below );
        } else {
            fold_and_print( w_look, point( column, ++line ), max_width, c_dark_gray, _( "Below: %s; Walkable" ),
                            tile_below );
        }
    }

    ++line;
}

void game::print_furniture_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                                 int column,
                                 int &line )
{
    map &here = get_map();

    // Do nothing if there is no furniture here
    if( !here.has_furn( lp ) ) {
        return;
    }
    const int max_width = getmaxx( w_look ) - column - 1;

    // Print furniture name in white
    std::string desc = uppercase_first_letter( here.furnname( lp ) );
    mvwprintz( w_look, point( column, line++ ), c_white, desc );

    // Print each line of furniture description in gray
    const furn_id &f = here.furn( lp );
    desc = string_format( f.obj().description );
    std::vector<std::string> lines = foldstring( desc, max_width );
    int numlines = lines.size();
    wattron( w_look, c_light_gray );
    for( int i = 0; i < numlines; i++ ) {
        mvwprintw( w_look, point( column, line++ ), lines[i] );
    }
    wattroff( w_look, c_light_gray );

    // If this furniture has a crafting pseudo item, check for tool qualities and print them
    if( !f->crafting_pseudo_item.is_empty() ) {
        // Make a pseudo item instance so we can use qualities_info later
        const item pseudo( f->crafting_pseudo_item );
        // Set up iteminfo query to show qualities
        std::vector<iteminfo_parts> quality_part = { iteminfo_parts::QUALITIES };
        const iteminfo_query quality_query( quality_part );
        // Render info into info_vec
        std::vector<iteminfo> info_vec;
        pseudo.qualities_info( info_vec, &quality_query, 1, false );
        // Get a newline-separated string of quality info, then parse and print each line
        std::string quality_string = format_item_info( info_vec, {} );
        size_t strpos = 0;
        while( ( strpos = quality_string.find( '\n' ) ) != std::string::npos ) {
            trim_and_print( w_look, point( column, line++ ), max_width, c_light_gray,
                            quality_string.substr( 0, strpos + 1 ) );
            // Delete used token
            quality_string.erase( 0, strpos + 1 );
        }
    }
}

void game::print_fields_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                              int column,
                              int &line )
{
    map &here = get_map();

    const field &tmpfield = here.field_at( lp );
    for( const auto &fld : tmpfield ) {
        const field_entry &cur = fld.second;
        if( fld.first.obj().has_fire && ( here.has_flag( ter_furn_flag::TFLAG_FIRE_CONTAINER, lp ) ||
                                          here.ter( lp ) == ter_t_pit_shallow || here.ter( lp ) == ter_t_pit ) ) {
            const int max_width = getmaxx( w_look ) - column - 2;
            int lines = fold_and_print( w_look, point( column, ++line ), max_width, cur.color(),
                                        get_fire_fuel_string( lp ) ) - 1;
            line += lines;
        } else {
            mvwprintz( w_look, point( column, ++line ), cur.color(), cur.name() );
        }
    }

    int size = std::distance( tmpfield.begin(), tmpfield.end() );
    if( size > 0 ) {
        mvwprintz( w_look, point( column, ++line ), c_white, "\n" );
    }
}

void game::print_trap_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                            const int column,
                            int &line )
{
    map &here = get_map();

    const trap &tr = here.tr_at( lp );
    if( tr.can_see( lp, u ) ) {
        std::string tr_name = tr.name();
        mvwprintz( w_look, point( column, ++line ), tr.color, tr_name );
    }

    ++line;
}

void game::print_part_con_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                                const int column,
                                int &line )
{
    map &here = get_map();

    partial_con *pc = here.partial_con_at( lp );
    std::string tr_name;
    if( pc != nullptr ) {
        const construction &built = pc->id.obj();
        tr_name = string_format( _( "Unfinished task: %s, %d%% complete" ), built.group->name(),
                                 pc->counter / 100000 );

        int const width = getmaxx( w_look ) - column - 2;
        fold_and_print( w_look, point( column, ++line ), width, c_white, tr_name );

        ++line;
    }
}

void game::print_creature_info( const Creature *creature, const catacurses::window &w_look,
                                const int column, int &line, const int last_line )
{
    const map &here = get_map();

    int vLines = last_line - line;
    if( creature != nullptr && ( u.sees( here, *creature ) || creature == &u ) ) {
        line = creature->print_info( w_look, ++line, vLines, column );
    }
}

void game::print_vehicle_info( const vehicle *veh, int veh_part, const catacurses::window &w_look,
                               const int column, int &line, const int last_line )
{
    if( veh ) {
        // Print the name of the vehicle.
        mvwprintz( w_look, point( column, ++line ), c_light_gray, _( "Vehicle: " ) );
        mvwprintz( w_look, point( column + utf8_width( _( "Vehicle: " ) ), line ), c_white, "%s",
                   veh->name );
        // Then the list of parts on that tile.
        line = veh->print_part_list( w_look, ++line, last_line, getmaxx( w_look ), veh_part );
    }
}

static void add_visible_items_recursive( std::map<std::string, std::pair<int, nc_color>>
        &item_names, const item &it )
{
    ++item_names[it.tname()].first;
    item_names[it.tname()].second = it.color_in_inventory();

    for( const item *content : it.all_known_contents() ) {
        add_visible_items_recursive( item_names, *content );
    }
}

void game::print_items_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                             const int column,
                             int &line,
                             const int last_line )
{
    map &here = get_map();

    if( !here.sees_some_items( lp, u ) ) {
        return;
    } else if( here.has_flag( ter_furn_flag::TFLAG_CONTAINER, lp ) && !here.could_see_items( lp, u ) ) {
        mvwprintw( w_look, point( column, ++line ), _( "You cannot see what is inside of it." ) );
    } else if( u.has_effect( effect_blind ) || u.worn_with_flag( flag_BLIND ) ) {
        mvwprintz( w_look, point( column, ++line ), c_yellow,
                   _( "There's something there, but you can't see what it is." ) );
        return;
    } else {
        std::map<std::string, std::pair<int, nc_color>> item_names;
        for( const item &it : here.i_at( lp ) ) {
            add_visible_items_recursive( item_names, it );
        }

        const int max_width = getmaxx( w_look ) - column - 1;
        for( auto it = item_names.begin(); it != item_names.end(); ++it ) {
            // last line but not last item
            if( line + 1 >= last_line && std::next( it ) != item_names.end() ) {
                mvwprintz( w_look, point( column, ++line ), c_yellow, _( "More items here…" ) );
                break;
            }

            if( it->second.first > 1 ) {
                trim_and_print( w_look, point( column, ++line ), max_width, it->second.second,
                                pgettext( "%s is the name of the item.  %d is the quantity of that item.", "%s [%d]" ),
                                it->first.c_str(), it->second.first );
            } else {
                trim_and_print( w_look, point( column, ++line ), max_width, it->second.second, it->first );
            }
        }
    }
}

void game::print_graffiti_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                                const int column, int &line,
                                const int last_line )
{
    map &here = get_map();

    if( line > last_line ) {
        return;
    }

    const int max_width = getmaxx( w_look ) - column - 2;
    if( here.has_graffiti_at( lp ) ) {
        const int lines = fold_and_print( w_look, point( column, ++line ), max_width, c_light_gray,
                                          here.ter( lp ) == ter_t_grave_new ? _( "Graffiti: %s" ) : _( "Inscription: %s" ),
                                          here.graffiti_at( lp ) );
        line += lines - 1;
    }
}

void game::print_debug_info( const tripoint_bub_ms &lp, const catacurses::window &w_look,
                             const int column, int &line )
{
    if( debug_mode ) {
        const map &here = get_map();

        mvwprintz( w_look, point( column, ++line ), c_white, "tripoint_bub_ms: %s",
                   lp.to_string_writable() );
        mvwprintz( w_look, point( column, ++line ), c_white, "tripoint_abs_ms: %s",
                   here.get_abs( lp ).to_string_writable() );
    }
}

void game::pre_print_all_tile_info( const tripoint_bub_ms &lp, const catacurses::window &w_info,
                                    int &first_line, const int last_line,
                                    const visibility_variables &cache )
{
    map &here = get_map();

    // get global area info according to look_around caret position
    tripoint_abs_omt omp( coords::project_to<coords::omt>( here.get_abs( lp ) ) );
    const oter_id &cur_ter_m = overmap_buffer.ter( omp );
    om_vision_level vision = overmap_buffer.seen( omp );
    // we only need the area name and then pass it to print_all_tile_info() function below
    const std::string area_name = cur_ter_m->get_name( vision );
    print_all_tile_info( lp, w_info, area_name, 1, first_line, last_line, cache );
}

std::optional<tripoint_bub_ms> game::look_around()
{
    tripoint_bub_ms center = u.pos_bub() + u.view_offset;
    look_around_result result = look_around( /*show_window=*/true, center, center, false, false,
                                false );
    return result.position;
}

look_around_result game::look_around(
    const bool show_window, tripoint_bub_ms &center, const tripoint_bub_ms &start_point,
    bool has_first_point,
    bool select_zone, bool peeking, bool is_moving_zone, const tripoint_bub_ms &end_point,
    bool change_lv )
{
    map &here = get_map();

    bVMonsterLookFire = false;

    temp_exit_fullscreen();

    tripoint_bub_ms lp( is_moving_zone ? ( tripoint_bub_ms( ( start_point.raw() + end_point.raw() ) /
                                           2 ) ) : start_point ); // cursor
    int &lx = lp.x();
    int &ly = lp.y();
    int &lz = lp.z();

    int soffset = get_option<int>( "FAST_SCROLL_OFFSET" );
    static bool fast_scroll = false;

    std::unique_ptr<ui_adaptor> ui;
    catacurses::window w_info;
    if( show_window ) {
        ui = std::make_unique<ui_adaptor>();
        ui->on_screen_resize( [&]( ui_adaptor & ui ) {
            int panel_width = panel_manager::get_manager().get_current_layout().panels().begin()->get_width();
            //FIXME: w_pixel_minimap is only reducing the height by one?
            int height = pixel_minimap_option ? TERMY - getmaxy( w_pixel_minimap ) : TERMY;

            // If particularly small, base height on panel width irrespective of other elements.
            // Value here is attempting to get a square-ish result assuming 1x2 proportioned font.
            if( height < panel_width / 2 ) {
                height = panel_width / 2;
            }

            int la_y = 0;
            int la_x = TERMX - panel_width;
            std::string position = get_option<std::string>( "LOOKAROUND_POSITION" );
            if( position == "left" ) {
                if( get_option<std::string>( "SIDEBAR_POSITION" ) == "right" ) {
                    la_x = panel_manager::get_manager().get_width_left();
                } else {
                    la_x = panel_manager::get_manager().get_width_left() - panel_width;
                }
            }
            int la_h = height;
            int la_w = panel_width;
            w_info = catacurses::newwin( la_h, la_w, point( la_x, la_y ) );

            ui.position_from_window( w_info );
        } );
        ui->mark_resize();
    }

    dbg( D_PEDANTIC_INFO ) << ": calling handle_input()";

    std::string action;
    input_context ctxt( "LOOK" );
    ctxt.set_iso( true );
    ctxt.register_directions();
    ctxt.register_action( "COORDINATE" );
    if( change_lv && fov_3d_z_range > 0 ) {
        ctxt.register_action( "LEVEL_UP" );
        ctxt.register_action( "LEVEL_DOWN" );
    }
    ctxt.register_action( "TOGGLE_FAST_SCROLL" );
    if( !has_first_point && !select_zone && !peeking && !is_moving_zone ) {
        ctxt.register_action( "map" );
    }
    ctxt.register_action( "CHANGE_MONSTER_NAME" );
    ctxt.register_action( "EXTENDED_DESCRIPTION" );
    ctxt.register_action( "SELECT" );
    if( peeking ) {
        ctxt.register_action( "throw_blind" );
        ctxt.register_action( "throw_blind_wielded" );
    }
    if( !select_zone ) {
        ctxt.register_action( "TRAVEL_TO" );
        ctxt.register_action( "LIST_ITEMS" );
    }
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "CENTER" );

    ctxt.register_action( "debug_scent" );
    ctxt.register_action( "debug_scent_type" );
    ctxt.register_action( "debug_temp" );
    ctxt.register_action( "debug_visibility" );
    ctxt.register_action( "debug_lighting" );
    ctxt.register_action( "debug_radiation" );
    ctxt.register_action( "debug_hour_timer" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "zoom_out" );
    ctxt.register_action( "zoom_in" );
    ctxt.register_action( "toggle_pixel_minimap" );

    const int old_levz = here.get_abs_sub().z();
    const int min_levz = std::max( old_levz - fov_3d_z_range, -OVERMAP_DEPTH );
    const int max_levz = std::min( old_levz + fov_3d_z_range, OVERMAP_HEIGHT );

    here.update_visibility_cache( old_levz );
    const visibility_variables &cache = here.get_visibility_variables_cache();

    bool blink = true;
    look_around_result result;

    shared_ptr_fast<draw_callback_t> ter_indicator_cb;

    if( show_window && ui ) {
        ui->on_redraw( [&]( const ui_adaptor & ) {
            werase( w_info );
            draw_border( w_info );

            center_print( w_info, 0, c_white, string_format( _( "< <color_green>Look around</color> >" ) ) );

            creature_tracker &creatures = get_creature_tracker();
            monster *const mon = creatures.creature_at<monster>( lp, true );
            if( mon && u.sees( here, *mon ) ) {
                std::string mon_name_text = string_format( _( "%s - %s" ),
                                            ctxt.get_desc( "CHANGE_MONSTER_NAME" ),
                                            ctxt.get_action_name( "CHANGE_MONSTER_NAME" ) );
                mvwprintz( w_info, point( 1, getmaxy( w_info ) - 4 ), c_red, mon_name_text );
            }

            std::string extended_description_text = string_format( _( "%s - %s" ),
                                                    ctxt.get_desc( "EXTENDED_DESCRIPTION" ),
                                                    ctxt.get_action_name( "EXTENDED_DESCRIPTION" ) );
            mvwprintz( w_info, point( 1, getmaxy( w_info ) - 3 ), c_light_green, extended_description_text );

            std::string fast_scroll_text = string_format( _( "%s - %s" ),
                                           ctxt.get_desc( "TOGGLE_FAST_SCROLL" ),
                                           ctxt.get_action_name( "TOGGLE_FAST_SCROLL" ) );
            mvwprintz( w_info, point( 1, getmaxy( w_info ) - 2 ), fast_scroll ? c_light_green : c_green,
                       fast_scroll_text );

            if( !ctxt.keys_bound_to( "toggle_pixel_minimap" ).empty() ) {
                std::string pixel_minimap_text = string_format( _( "%s - %s" ),
                                                 ctxt.get_desc( "toggle_pixel_minimap" ),
                                                 ctxt.get_action_name( "toggle_pixel_minimap" ) );
                right_print( w_info, getmaxy( w_info ) - 2, 1, pixel_minimap_option ? c_light_green : c_green,
                             pixel_minimap_text );
            }

            int first_line = 1;
            const int last_line = getmaxy( w_info ) - 4;
            pre_print_all_tile_info( lp, w_info, first_line, last_line, cache );

            wnoutrefresh( w_info );
        } );
        ter_indicator_cb = make_shared_fast<draw_callback_t>( [&]() {
            draw_look_around_cursor( lp, cache );
        } );
        add_draw_callback( ter_indicator_cb );
    }

    std::optional<tripoint_bub_ms> zone_start;
    std::optional<tripoint_bub_ms> zone_end;
    bool zone_blink = false;
    bool zone_cursor = true;
    shared_ptr_fast<draw_callback_t> zone_cb = zone_manager_ui::create_zone_callback( zone_start,
            zone_end, zone_blink,
            zone_cursor, is_moving_zone );
    add_draw_callback( zone_cb );

    is_looking = true;
    const tripoint_rel_ms prev_offset = u.view_offset;
#if defined(TILES)
    const int prev_tileset_zoom = tileset_zoom;
    while( is_moving_zone && square_dist( start_point, end_point ) > 256 / get_zoom() &&
           get_zoom() != 4 ) {
        zoom_out();
    }
    mark_main_ui_adaptor_resize();
#endif
    do {
        u.view_offset = center - u.pos_bub();
        if( select_zone ) {
            if( has_first_point ) {
                zone_start = start_point;
                zone_end = lp;
            } else {
                zone_start = lp;
                zone_end = std::nullopt;
            }
            // Actually accessed from the terrain overlay callback `zone_cb` in the
            // call to `ui_manager::redraw`.
            //NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
            zone_blink = blink;
        }

        if( is_moving_zone ) {
            zone_start = lp - ( start_point.raw() + end_point.raw() ) / 2 + start_point.raw();
            zone_end = lp - ( start_point.raw() + end_point.raw() ) / 2 + end_point.raw();
            // Actually accessed from the terrain overlay callback `zone_cb` in the
            // call to `ui_manager::redraw`.
            //NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
            zone_blink = blink;
        }
#if defined(TILES)
        // Mark cata_tiles draw caches as dirty
        tilecontext->set_draw_cache_dirty();
#endif
        invalidate_main_ui_adaptor();
        ui_manager::redraw();

        if( ( select_zone && has_first_point ) || is_moving_zone ) {
            ctxt.set_timeout( get_option<int>( "BLINK_SPEED" ) );
        }

        //Wait for input
        // only specify a timeout here if "EDGE_SCROLL" is enabled
        // otherwise use the previously set timeout
        const tripoint_rel_ms edge_scroll = mouse_edge_scrolling_terrain( ctxt );
        const int scroll_timeout = get_option<int>( "EDGE_SCROLL" );
        const bool edge_scrolling = edge_scroll != tripoint_rel_ms::zero && scroll_timeout >= 0;
        if( edge_scrolling ) {
            action = ctxt.handle_input( scroll_timeout );
        } else {
            action = ctxt.handle_input();
        }
        if( ( action == "LEVEL_UP" || action == "LEVEL_DOWN" || action == "MOUSE_MOVE" ||
              ctxt.get_direction_rel_ms( action ) ) && ( ( select_zone && has_first_point ) ||
                      is_moving_zone ) ) {
            blink = true; // Always draw blink symbols when moving cursor
        } else if( action == "TIMEOUT" ) {
            blink = !blink;
        }
        if( action == "LIST_ITEMS" ) {
            list_items_monsters();
        } else if( action == "TOGGLE_FAST_SCROLL" ) {
            fast_scroll = !fast_scroll;
        } else if( action == "map" ) {
            uistate.open_menu = [center, &here]() {
                ui::omap::look_around_map( here.get_abs( center ) );
            };
            break;
        } else if( action == "toggle_pixel_minimap" ) {
            toggle_pixel_minimap();

            if( show_window && ui ) {
                ui->mark_resize();
            }
        } else if( action == "LEVEL_UP" || action == "LEVEL_DOWN" ) {
            const int dz = action == "LEVEL_UP" ? 1 : -1;
            lz = clamp( lz + dz, min_levz, max_levz );
            center.z() = clamp( center.z() + dz, min_levz, max_levz );

            add_msg_debug( debugmode::DF_GAME, "levx: %d, levy: %d, levz: %d",
                           here.get_abs_sub().x(), here.get_abs_sub().y(), center.z() );
            u.view_offset.z() = center.z() - u.posz();
            here.invalidate_map_cache( center.z() );
            // Fix player character not visible from above
            here.build_map_cache( u.posz() );
            here.invalidate_visibility_cache();
        } else if( action == "TRAVEL_TO" ) {
            const std::optional<std::vector<tripoint_bub_ms>> try_route = safe_route_to( u, lp,
            0, []( const std::string & msg ) {
                add_msg( msg );
            } );
            if( try_route.has_value() ) {
                u.set_destination( *try_route );
                continue;
            }
        } else if( action == "debug_hour_timer" ) {
            toggle_debug_hour_timer();
        } else if( action == "EXTENDED_DESCRIPTION" ) {
            extended_description_window ext_desc( lp );
            ext_desc.show();
        } else if( action == "CHANGE_MONSTER_NAME" ) {
            creature_tracker &creatures = get_creature_tracker();
            monster *const mon = creatures.creature_at<monster>( lp, true );
            if( mon ) {
                string_input_popup popup;
                popup
                .title( _( "Nickname:" ) )
                .width( 85 )
                .edit( mon->nickname );
            }
        } else if( action == "CENTER" ) {
            center = u.pos_bub();
            lp = u.pos_bub();
            u.view_offset.z() = 0;
        } else if( action == "MOUSE_MOVE" || action == "TIMEOUT" ) {
            // This block is structured this way so that edge scroll can work
            // whether the mouse is moving at the edge or simply stationary
            // at the edge. But even if edge scroll isn't in play, there's
            // other things for us to do here.

            if( edge_scrolling ) {
                center += action == "MOUSE_MOVE" ? edge_scroll * 2 : edge_scroll;
            } else if( action == "MOUSE_MOVE" ) {
                const std::optional<tripoint_bub_ms> mouse_pos = ctxt.get_coordinates( w_terrain,
                        ter_view_p.raw().xy(),
                        true );
                if( mouse_pos ) {
                    lx = mouse_pos->x();
                    ly = mouse_pos->y();
                }
            }
        } else if( std::optional<tripoint_rel_ms> vec = ctxt.get_direction_rel_ms( action ) ) {
            if( fast_scroll ) {
                vec->x() *= soffset;
                vec->y() *= soffset;
            }

            lx = lx + vec->x();
            ly = ly + vec->y();
            center.x() = center.x() + vec->x();
            center.y() = center.y() + vec->y();
        } else if( action == "throw_blind" ) {
            result.peek_action = PA_BLIND_THROW;
        } else if( action == "throw_blind_wielded" ) {
            result.peek_action = PA_BLIND_THROW_WIELDED;
        } else if( action == "CONFIRM" && peeking ) {
            result.peek_action = PA_MOVE;
        } else if( action == "zoom_in" ) {
            center.xy() = lp.xy();
            zoom_in();
            mark_main_ui_adaptor_resize();
        } else if( action == "zoom_out" ) {
            center.xy() = lp.xy();
            zoom_out();
            mark_main_ui_adaptor_resize();
        }
    } while( action != "QUIT" && action != "CONFIRM" && action != "SELECT" && action != "TRAVEL_TO" &&
             action != "throw_blind" && action != "throw_blind_wielded" );

    if( center.z() != old_levz ) {
        here.invalidate_map_cache( old_levz );
        here.build_map_cache( old_levz );
        u.view_offset.z() = 0;
    }

    ctxt.reset_timeout();
    u.view_offset = prev_offset;
    zone_cb = nullptr;
    is_looking = false;

    reenter_fullscreen();
    bVMonsterLookFire = true;

    if( action == "CONFIRM" || action == "SELECT" ) {
        result.position = is_moving_zone ? zone_start.value() : lp;
    }

#if defined(TILES)
    if( is_moving_zone && get_zoom() != prev_tileset_zoom ) {
        // Reset the tileset zoom to the previous value
        set_zoom( prev_tileset_zoom );
        mark_main_ui_adaptor_resize();
    }
#endif

    return result;
}

look_around_result game::look_around( look_around_params looka_params )
{
    return look_around( looka_params.show_window, looka_params.center, looka_params.start_point,
                        looka_params.has_first_point, looka_params.select_zone, looka_params.peeking, false,
                        tripoint_bub_ms::zero,
                        looka_params.change_lv );
}

void draw_trail( const tripoint_bub_ms &start, const tripoint_bub_ms &end, const bool bDrawX )
{
    map &here = get_map();

    std::vector<tripoint_bub_ms> pts;
    avatar &player_character = get_avatar();
    const tripoint_bub_ms pos = player_character.pos_bub( here );
    tripoint_bub_ms center = pos + player_character.view_offset;
    if( start != end ) {
        //Draw trail
        pts = line_to( start, end, 0, 0 );
    } else {
        //Draw point
        pts.emplace_back( start );
    }

    g->draw_line( end, center, pts );
    if( bDrawX ) {
        char sym = 'X';
        if( end.z() > center.z() ) {
            sym = '^';
        } else if( end.z() < center.z() ) {
            sym = 'v';
        }
        if( pts.empty() ) {
            mvwputch( g->w_terrain, point( POSX, POSY ), c_white, sym );
        } else {
            mvwputch( g->w_terrain, pts.back().raw().xy() - player_character.view_offset.xy().raw() +
                      point( POSX - pos.x(), POSY - pos.y() ),
                      c_white, sym );
        }
    }
}

void game::draw_trail_to_square( const tripoint_rel_ms &t, bool bDrawX )
{
    ::draw_trail( u.pos_bub(), u.pos_bub() + t, bDrawX );
}

static void centerlistview( const tripoint &active_item_position, int ui_width )
{
    Character &u = get_avatar();
    if( get_option<std::string>( "SHIFT_LIST_ITEM_VIEW" ) != "false" ) {
        u.view_offset.z() = active_item_position.z;
        if( get_option<std::string>( "SHIFT_LIST_ITEM_VIEW" ) == "centered" ) {
            u.view_offset.x() = active_item_position.x;
            u.view_offset.y() = active_item_position.y;
        } else {
            point pos( active_item_position.xy() + point( POSX, POSY ) );

            // item/monster list UI is on the right, so get the difference between its width
            // and the width of the sidebar on the right (if any)
            int sidebar_right_adjusted = ui_width - panel_manager::get_manager().get_width_right();
            // if and only if that difference is greater than zero, use that as offset
            int right_offset = sidebar_right_adjusted > 0 ? sidebar_right_adjusted : 0;

            // Convert offset to tile counts, calculate adjusted terrain window width
            // This lets us account for possible differences in terrain width between
            // the normal sidebar and the list-all-whatever display.
            to_map_font_dim_width( right_offset );
            int terrain_width = TERRAIN_WINDOW_WIDTH - right_offset;

            if( pos.x < 0 ) {
                u.view_offset.x() = pos.x;
            } else if( pos.x >= terrain_width ) {
                u.view_offset.x() = pos.x - ( terrain_width - 1 );
            } else {
                u.view_offset.x() = 0;
            }

            if( pos.y < 0 ) {
                u.view_offset.y() = pos.y;
            } else if( pos.y >= TERRAIN_WINDOW_HEIGHT ) {
                u.view_offset.y() = pos.y - ( TERRAIN_WINDOW_HEIGHT - 1 );
            } else {
                u.view_offset.y() = 0;
            }
        }
    }

}

#if defined(TILES)
static constexpr int MAXIMUM_ZOOM_LEVEL = 4;
#endif
void game::zoom_out()
{
#if defined(TILES)
    if( tileset_zoom > MAXIMUM_ZOOM_LEVEL ) {
        tileset_zoom = tileset_zoom / 2;
    } else {
        tileset_zoom = 64;
    }
    rescale_tileset( tileset_zoom );
#endif
}

void game::zoom_out_overmap()
{
#if defined(TILES)
    if( overmap_tileset_zoom > MAXIMUM_ZOOM_LEVEL ) {
        overmap_tileset_zoom /= 2;
    } else {
        overmap_tileset_zoom = 64;
    }
    overmap_tilecontext->set_draw_scale( overmap_tileset_zoom );
#endif
}

void game::zoom_in()
{
#if defined(TILES)
    if( tileset_zoom == 64 ) {
        tileset_zoom = MAXIMUM_ZOOM_LEVEL;
    } else {
        tileset_zoom = tileset_zoom * 2;
    }
    rescale_tileset( tileset_zoom );
#endif
}

void game::zoom_in_overmap()
{
#if defined(TILES)
    if( overmap_tileset_zoom == 64 ) {
        overmap_tileset_zoom = MAXIMUM_ZOOM_LEVEL;
    } else {
        overmap_tileset_zoom *= 2;
    }
    overmap_tilecontext->set_draw_scale( overmap_tileset_zoom );
#endif
}

void game::reset_zoom()
{
#if defined(TILES)
    tileset_zoom = DEFAULT_TILESET_ZOOM;
    rescale_tileset( tileset_zoom );
#endif // TILES
}

void game::set_zoom( const int level )
{
#if defined(TILES)
    if( tileset_zoom != level ) {
        tileset_zoom = level;
        rescale_tileset( tileset_zoom );
    }
#else
    static_cast<void>( level );
#endif // TILES
}

int game::get_zoom() const
{
#if defined(TILES)
    return tileset_zoom;
#else
    return DEFAULT_TILESET_ZOOM;
#endif
}

#if defined(TILES)
bool game::take_screenshot( const std::string &path ) const
{
    return save_screenshot( path );
}

bool game::take_screenshot() const
{
    // check that the current '<world>/screenshots' directory exists
    cata_path map_directory = PATH_INFO::world_base_save_path() / "screenshots";
    assure_dir_exist( map_directory );

    // build file name: <map_dir>/screenshots/[<character_name>]_<date>.png
    // NOLINTNEXTLINE(cata-translate-string-literal)
    const std::string tmp_file_name = string_format( "[%s]_%s.png", get_player_character().get_name(),
                                      timestamp_now() );

    std::string file_name = ensure_valid_file_name( tmp_file_name );
    cata_path current_file_path = map_directory / file_name;

    // Take a screenshot of the viewport.
    if( take_screenshot( current_file_path.generic_u8string() ) ) {
        popup( _( "Successfully saved your screenshot to: %s" ), map_directory );
        return true;
    } else {
        popup( _( "An error occurred while trying to save the screenshot." ) );
        return false;
    }
}
#else
bool game::take_screenshot( const std::string &/*path*/ ) const
{
    return false;
}

bool game::take_screenshot() const
{
    popup( _( "This binary was not compiled with tiles support." ) );
    return false;
}
#endif

//helper method so we can keep list_items shorter
void game::reset_item_list_state( const catacurses::window &window, int height,
                                  list_item_sort_mode sortMode )
{
    const int width = getmaxx( window );
    wattron( window, c_light_gray );

    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwhline( window, point( 1, 0 ), LINE_OXOX, width - 1 ); // -
    mvwhline( window, point( 1, TERMY - height - 1 ), LINE_OXOX, width - 1 ); // -
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwvline( window, point( 0, 1 ), LINE_XOXO, TERMY - height - 1 ); // |
    mvwvline( window, point( width - 1, 1 ), LINE_XOXO, TERMY - height - 1 ); // |

    mvwaddch( window, point::zero, LINE_OXXO ); // |^
    mvwaddch( window, point( width - 1, 0 ), LINE_OOXX ); // ^|

    mvwaddch( window, point( 0, TERMY - height - 1 ), LINE_XXXO ); // |-
    mvwaddch( window, point( width - 1, TERMY - height - 1 ), LINE_XOXX ); // -|
    wattroff( window, c_light_gray );

    mvwprintz( window, point( 2, 0 ), c_light_green, "<Tab> " );
    wprintz( window, c_white, _( "Items" ) );

    std::string sSort;
    switch( sortMode ) {
        // Sort by distance only
        case list_item_sort_mode::count:
        case list_item_sort_mode::DISTANCE:
            sSort = _( "<s>ort: dist" );
            break;
        // Sort by name only
        case list_item_sort_mode::NAME:
            sSort = _( "<s>ort: name" );
            break;
        // Group by category, sort by distance
        case list_item_sort_mode::CATEGORY_DISTANCE:
            sSort = _( "<s>ort: cat-dist" );
            break;
        // Group by category, sort by item name
        case list_item_sort_mode::CATEGORY_NAME:
            sSort = _( "<s>ort: cat-name" );
            break;
    }

    int letters = utf8_width( sSort );

    shortcut_print( window, point( getmaxx( window ) - letters, 0 ), c_white, c_light_green, sSort );

    std::vector<std::string> tokens;
    tokens.reserve( 5 + ( !sFilter.empty() ? 1 : 0 ) );
    if( !sFilter.empty() ) {
        tokens.emplace_back( _( "<R>eset" ) );
    }

    tokens.emplace_back( _( "<E>xamine" ) );
    tokens.emplace_back( _( "<C>ompare" ) );
    tokens.emplace_back( _( "<F>ilter" ) );
    tokens.emplace_back( _( "<+/->Priority" ) );
    tokens.emplace_back( _( "<T>ravel to" ) );

    int gaps = tokens.size() + 1;
    letters = 0;
    int n = tokens.size();
    for( int i = 0; i < n; i++ ) {
        letters += utf8_width( tokens[i] ) - 2; //length ignores < >
    }

    int usedwidth = letters;
    const int gap_spaces = ( width - usedwidth ) / gaps;
    usedwidth += gap_spaces * gaps;
    point pos( gap_spaces + ( width - usedwidth ) / 2, TERMY - height - 1 );

    for( int i = 0; i < n; i++ ) {
        pos.x += shortcut_print( window, pos, c_white, c_light_green,
                                 tokens[i] ) + gap_spaces;
    }
}

template<>
std::string io::enum_to_string<list_item_sort_mode>( list_item_sort_mode data )
{
    switch( data ) {
        case list_item_sort_mode::count:
        case list_item_sort_mode::DISTANCE:
            return "DISTANCE";
        case list_item_sort_mode::NAME:
            return "NAME";
        case list_item_sort_mode::CATEGORY_DISTANCE:
            return "CATEGORY_DISTANCE";
        case list_item_sort_mode::CATEGORY_NAME:
            return "CATEGORY_NAME";
    }
    cata_fatal( "Invalid list item sort mode" );
}

void game::list_items_monsters()
{
    // Search whole reality bubble because each function internally verifies
    // the visibility of the items / monsters in question.
    std::vector<Creature *> mons = u.get_visible_creatures( MAX_VIEW_DISTANCE );
    const std::vector<map_item_stack> items = find_nearby_items( MAX_VIEW_DISTANCE );

    if( mons.empty() && items.empty() ) {
        add_msg( m_info, _( "You don't see any items or monsters around you!" ) );
        return;
    }

    std::sort( mons.begin(), mons.end(), [&]( const Creature * lhs, const Creature * rhs ) {
        if( !u.has_trait( trait_INATTENTIVE ) ) {
            const Creature::Attitude att_lhs = lhs->attitude_to( u );
            const Creature::Attitude att_rhs = rhs->attitude_to( u );

            return att_lhs < att_rhs || ( att_lhs == att_rhs
                                          && rl_dist( u.pos_bub(), lhs->pos_bub() ) < rl_dist( u.pos_bub(), rhs->pos_bub() ) );
        } else { // Sort just by ditance if player has inattentive trait
            return ( rl_dist( u.pos_bub(), lhs->pos_bub() ) < rl_dist( u.pos_bub(), rhs->pos_bub() ) );
        }

    } );

    // If the current list is empty, switch to the non-empty list
    if( uistate.vmenu_show_items ) {
        if( items.empty() ) {
            uistate.vmenu_show_items = false;
        }
    } else if( mons.empty() ) {
        uistate.vmenu_show_items = true;
    }

    temp_exit_fullscreen();
    game::vmenu_ret ret;
    while( true ) {
        ret = uistate.vmenu_show_items ? list_items( items ) : list_monsters( mons );
        if( ret == game::vmenu_ret::CHANGE_TAB ) {
            uistate.vmenu_show_items = !uistate.vmenu_show_items;
        } else {
            break;
        }
    }

    if( ret == game::vmenu_ret::FIRE ) {
        avatar_action::fire_wielded_weapon( u );
    }
    reenter_fullscreen();
}

static std::string list_items_filter_history_help()
{
    input_context ctxt( "STRING_INPUT" );
    std::vector<std::string> act_descs;
    const auto add_action_desc = [&]( const std::string & act, const std::string & txt ) {
        act_descs.emplace_back( ctxt.get_desc( act, txt, input_context::allow_all_keys ) );
    };
    add_action_desc( "HISTORY_UP", pgettext( "string input", "History" ) );
    add_action_desc( "TEXT.CLEAR", pgettext( "string input", "Clear text" ) );
    add_action_desc( "TEXT.QUIT", pgettext( "string input", "Abort" ) );
    add_action_desc( "TEXT.CONFIRM", pgettext( "string input", "Save" ) );
    return colorize( enumerate_as_string( act_descs, enumeration_conjunction::none ), c_green );
}

/// return content_newness based on if item is known and nested items are known
static content_newness check_items_newness( const item *itm )
{
    if( !uistate.read_items.count( itm->typeId() ) ) {
        return content_newness::NEW;
    }
    return itm->get_contents().get_content_newness( uistate.read_items );
}

/// add item and all visible nested items inside to known items
static void mark_items_read_rec( const item *itm )
{
    uistate.read_items.insert( itm->typeId() );
    for( const item *child : itm->all_known_contents() ) {
        mark_items_read_rec( child );
    }
}

game::vmenu_ret game::list_items( const std::vector<map_item_stack> &item_list )
{
    std::vector<map_item_stack> ground_items = item_list;
    int iInfoHeight = 0;
    int iMaxRows = 0;
    int width = 0;
    int width_nob = 0;  // width without borders
    int max_name_width = 0;

    const bool highlight_unread_items = get_option<bool>( "HIGHLIGHT_UNREAD_ITEMS" );
    const nc_color item_new_col = c_light_green;
    const nc_color item_maybe_new_col = c_light_gray;
    const std::string item_new_str = pgettext( "crafting gui", "NEW!" );
    const std::string item_maybe_new_str = pgettext( "crafting gui", "hides contents" );
    const int item_new_str_width = utf8_width( item_new_str );
    const int item_maybe_new_str_width = utf8_width( item_maybe_new_str );

    // Constants for content that is always displayed in w_items
    // on left: 1 space
    const int left_padding = 1;
    // on right: 2 digit distance 1 space 2 direction 1 space
    const int right_padding = 2 + 1 + 2 + 1;
    const int padding = left_padding + right_padding;

    //find max length of item name and resize window width
    for( const map_item_stack &cur_item : ground_items ) {
        max_name_width = std::max( max_name_width,
                                   utf8_width( remove_color_tags( cur_item.example->display_name() ) ) );
    }
    // + 6 as estimate for `iThisPage` I guess
    max_name_width = max_name_width + padding + 6 + ( highlight_unread_items ? std::max(
                         item_new_str_width, item_maybe_new_str_width ) : 0 );

    tripoint_rel_ms active_pos;
    map_item_stack *activeItem = nullptr;

    catacurses::window w_items;
    catacurses::window w_items_border;
    catacurses::window w_item_info;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        iInfoHeight = std::min( 25, TERMY / 2 );
        iMaxRows = TERMY - iInfoHeight - 2;

        width = clamp( max_name_width, 55, TERMX / 3 );
        width_nob = width - 2;

        const int offsetX = TERMX - width;

        w_items = catacurses::newwin( TERMY - 2 - iInfoHeight,
                                      width_nob, point( offsetX + 1, 1 ) );
        w_items_border = catacurses::newwin( TERMY - iInfoHeight,
                                             width, point( offsetX, 0 ) );
        w_item_info = catacurses::newwin( iInfoHeight, width,
                                          point( offsetX, TERMY - iInfoHeight ) );

        if( activeItem ) {
            centerlistview( active_pos.raw(), width );
        }

        ui.position( point( offsetX, 0 ), point( width, TERMY ) );
    } );
    ui.mark_resize();

    // reload filter/priority settings on the first invocation, if they were active
    if( !uistate.list_item_init ) {
        if( uistate.list_item_filter_active ) {
            sFilter = uistate.list_item_filter;
        }
        if( uistate.list_item_downvote_active ) {
            list_item_downvote = uistate.list_item_downvote;
        }
        if( uistate.list_item_priority_active ) {
            list_item_upvote = uistate.list_item_priority;
        }
        uistate.list_item_init = true;
    }

    //this stores only those items that match our filter
    std::vector<map_item_stack> filtered_items =
        !sFilter.empty() ? filter_item_stacks( ground_items, sFilter ) : ground_items;
    int highPEnd = list_filter_high_priority( filtered_items, list_item_upvote );
    int lowPStart = list_filter_low_priority( filtered_items, highPEnd, list_item_downvote );
    int iItemNum = ground_items.size();

    const tripoint_rel_ms stored_view_offset = u.view_offset;

    u.view_offset = tripoint_rel_ms::zero;

    int iActive = 0; // Item index that we're looking at
    int page_num = 0;
    int iCatSortNum = 0;
    int iScrollPos = 0;
    std::map<int, std::string> mSortCategory;

    ui.on_redraw( [&]( ui_adaptor & ui ) {
        reset_item_list_state( w_items_border, iInfoHeight, uistate.list_item_sort );

        int iStartPos = 0;
        if( ground_items.empty() ) {
            wnoutrefresh( w_items_border );
            mvwprintz( w_items, point( 2, 10 ), c_white, _( "You don't see any items around you!" ) );
        } else {
            werase( w_items );
            calcStartPos( iStartPos, iActive, iMaxRows, iItemNum );
            int iNum = 0;
            // ITEM LIST
            int index = 0;
            int iCatSortOffset = 0;

            for( int i = 0; i < iStartPos; i++ ) {
                if( !mSortCategory[i].empty() ) {
                    iNum++;
                }
            }
            for( auto iter = filtered_items.begin(); iter != filtered_items.end(); ++index, iNum++ ) {
                if( iNum < iStartPos || iNum >= iStartPos + std::min( iMaxRows, iItemNum ) ) {
                    ++iter;
                    continue;
                }
                int iThisPage = 0;
                if( !mSortCategory[iNum].empty() ) {
                    iCatSortOffset++;
                    mvwprintz( w_items, point( 1, iNum - iStartPos ), c_magenta, mSortCategory[iNum] );
                    continue;
                }
                if( iNum == iActive ) {
                    iThisPage = page_num;
                }
                std::string sText;
                if( iter->vIG.size() > 1 ) {
                    sText += string_format( "[%d/%d] (%d) ", iThisPage + 1, iter->vIG.size(), iter->totalcount );
                }
                sText += iter->example->tname();
                if( iter->vIG[iThisPage].count > 1 ) {
                    sText += string_format( "[%d]", iter->vIG[iThisPage].count );
                }

                nc_color col = c_light_gray;
                if( iNum == iActive ) {
                    col = hilite( c_white );
                } else if( highPEnd > 0 && index < highPEnd + iCatSortOffset ) { // priority high
                    col = c_yellow;
                } else if( index >= lowPStart + iCatSortOffset ) { // priority low
                    col = c_red;
                } else { // priority medium
                    col = iter->example->color_in_inventory();
                }
                bool print_new = highlight_unread_items;
                const std::string *new_str = nullptr;
                // 1 make space between item description and right padding (distance)
                int new_width = 1;
                const nc_color *new_col = nullptr;
                if( print_new ) {
                    switch( check_items_newness( iter->example ) ) {
                        case content_newness::NEW:
                            new_str = &item_new_str;
                            // +1 make space between item description and "new"
                            new_width += item_new_str_width + 1;
                            new_col = &item_new_col;
                            break;
                        case content_newness::MIGHT_BE_HIDDEN:
                            new_str = &item_maybe_new_str;
                            new_width += item_maybe_new_str_width + 1;
                            new_col = &item_maybe_new_col;
                            break;
                        case content_newness::SEEN:
                            print_new = false;
                            break;
                    }
                }
                trim_and_print( w_items, point( 1, iNum - iStartPos ), width_nob - padding - new_width, col,
                                sText );
                const point_rel_ms p( iter->vIG[iThisPage].pos.xy() );
                if( print_new ) {
                    // +1 move space between item description and "new"
                    mvwprintz( w_items, point( width_nob - right_padding - new_width + 1, iNum - iStartPos ), *new_col,
                               *new_str );
                }
                mvwprintz( w_items, point( width_nob - right_padding, iNum - iStartPos ),
                           iNum == iActive ? c_light_green : c_light_gray,
                           "%2d %s", rl_dist( point_rel_ms::zero, p ),
                           direction_name_short( direction_from( point_rel_ms::zero, p ) ) );
                ++iter;
            }
            // ITEM DESCRIPTION
            iNum = 0;
            for( int i = 0; i < iActive; i++ ) {
                if( !mSortCategory[i].empty() ) {
                    iNum++;
                }
            }
            const int current_i = iItemNum > 0 ? iActive - iNum + 1 : 0;
            const int numd = current_i > 999 ? 4 :
                             current_i > 99 ? 3 :
                             current_i > 9 ? 2 : 1;
            mvwprintz( w_items_border, point( width / 2 - numd - 2, 0 ), c_light_green, " %*d", numd,
                       current_i );
            wprintz( w_items_border, c_white, " / %*d ", numd, iItemNum - iCatSortNum );
            werase( w_item_info );

            if( iItemNum > 0 && activeItem ) {
                std::vector<iteminfo> vThisItem;
                std::vector<iteminfo> vDummy;
                activeItem->vIG[page_num].it->info( true, vThisItem );

                item_info_data dummy( "", "", vThisItem, vDummy, iScrollPos );
                dummy.without_getch = true;
                dummy.without_border = true;

                draw_item_info( w_item_info, dummy );
            }
            draw_scrollbar( w_items_border, iActive, iMaxRows, iItemNum, point::south );
            wnoutrefresh( w_items_border );
        }

        const bool bDrawLeft = ground_items.empty() || filtered_items.empty() || !activeItem;
        draw_custom_border( w_item_info, bDrawLeft, 1, 1, 1, LINE_XXXO, LINE_XOXX, 1, 1 );

        if( iItemNum > 0 && activeItem ) {
            // print info window title: < item name >
            mvwprintw( w_item_info, point( 2, 0 ), "< " );
            trim_and_print( w_item_info, point( 4, 0 ), width_nob - padding,
                            activeItem->vIG[page_num].it->color_in_inventory(),
                            activeItem->vIG[page_num].it->display_name() );
            wprintw( w_item_info, " >" );
            // move the cursor to the selected item (for screen readers)
            ui.set_cursor( w_items, point( 1, iActive - iStartPos ) );
        }

        wnoutrefresh( w_items );
        wnoutrefresh( w_item_info );
    } );

    std::optional<tripoint_bub_ms> trail_start;
    std::optional<tripoint_bub_ms> trail_end;
    bool trail_end_x = false;
    shared_ptr_fast<draw_callback_t> trail_cb = create_trail_callback( trail_start, trail_end,
            trail_end_x );
    add_draw_callback( trail_cb );

    bool addCategory = uistate.list_item_sort == list_item_sort_mode::CATEGORY_DISTANCE ||
                       uistate.list_item_sort == list_item_sort_mode::CATEGORY_NAME;
    bool refilter = true;

    std::string action;
    input_context ctxt( "LIST_ITEMS" );
    ctxt.register_action( "UP", to_translation( "Move cursor up" ) );
    ctxt.register_action( "DOWN", to_translation( "Move cursor down" ) );
    ctxt.register_action( "LEFT", to_translation( "Previous item" ) );
    ctxt.register_action( "RIGHT", to_translation( "Next item" ) );
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "SCROLL_ITEM_INFO_DOWN" );
    ctxt.register_action( "SCROLL_ITEM_INFO_UP" );
    ctxt.register_action( "zoom_in" );
    ctxt.register_action( "zoom_out" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "EXAMINE" );
    ctxt.register_action( "COMPARE" );
    ctxt.register_action( "PRIORITY_INCREASE" );
    ctxt.register_action( "PRIORITY_DECREASE" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "TRAVEL_TO" );

    do {
        bool recalc_unread = false;
        if( action == "COMPARE" && activeItem ) {
            game_menus::inv::compare( active_pos );
            recalc_unread = highlight_unread_items;
        } else if( action == "FILTER" ) {
            ui.invalidate_ui();
            string_input_popup_imgui imgui_popup( 70, sFilter, _( "Filter items" ) );
            imgui_popup.set_label( _( "Filter:" ) );
            imgui_popup.set_description( item_filter_rule_string( item_filter_type::FILTER ) + "\n\n" +
                                         list_items_filter_history_help(), c_white, true );
            imgui_popup.set_identifier( "item_filter" );
            sFilter = imgui_popup.query();
            refilter = true;
            uistate.list_item_filter_active = !sFilter.empty();
        } else if( action == "RESET_FILTER" ) {
            sFilter.clear();
            filtered_items = ground_items;
            refilter = true;
            uistate.list_item_filter_active = false;
        } else if( action == "EXAMINE" && !filtered_items.empty() && activeItem ) {
            std::vector<iteminfo> vThisItem;
            std::vector<iteminfo> vDummy;
            activeItem->vIG[page_num].it->info( true, vThisItem );

            item_info_data info_data( activeItem->vIG[page_num].it->tname(),
                                      activeItem->vIG[page_num].it->type_name(), vThisItem,
                                      vDummy );
            info_data.handle_scrolling = true;

            draw_item_info( [&]() -> catacurses::window {
                return catacurses::newwin( TERMY, width - 5, point::zero );
            }, info_data );
            recalc_unread = highlight_unread_items;
        } else if( action == "PRIORITY_INCREASE" ) {
            ui.invalidate_ui();
            list_item_upvote = string_input_popup()
                               .title( _( "High Priority:" ) )
                               .width( 55 )
                               .text( list_item_upvote )
                               .description( item_filter_rule_string( item_filter_type::HIGH_PRIORITY ) + "\n\n"
                                             + list_items_filter_history_help() )
                               .desc_color( c_white )
                               .identifier( "list_item_priority" )
                               .max_length( 256 )
                               .query_string();
            refilter = true;
            uistate.list_item_priority_active = !list_item_upvote.empty();
        } else if( action == "PRIORITY_DECREASE" ) {
            ui.invalidate_ui();
            list_item_downvote = string_input_popup()
                                 .title( _( "Low Priority:" ) )
                                 .width( 55 )
                                 .text( list_item_downvote )
                                 .description( item_filter_rule_string( item_filter_type::LOW_PRIORITY ) + "\n\n"
                                               + list_items_filter_history_help() )
                                 .desc_color( c_white )
                                 .identifier( "list_item_downvote" )
                                 .max_length( 256 )
                                 .query_string();
            refilter = true;
            uistate.list_item_downvote_active = !list_item_downvote.empty();
        } else if( action == "SORT" ) {
            switch( uistate.list_item_sort ) {
                case list_item_sort_mode::count:
                case list_item_sort_mode::DISTANCE:
                    uistate.list_item_sort = list_item_sort_mode::NAME;
                    break;
                case list_item_sort_mode::NAME:
                    uistate.list_item_sort = list_item_sort_mode::CATEGORY_DISTANCE;
                    break;
                case list_item_sort_mode::CATEGORY_DISTANCE:
                    uistate.list_item_sort = list_item_sort_mode::CATEGORY_NAME;
                    break;
                case list_item_sort_mode::CATEGORY_NAME:
                    uistate.list_item_sort = list_item_sort_mode::DISTANCE;
                    break;
            }

            highPEnd = -1;
            lowPStart = -1;
            iCatSortNum = 0;

            mSortCategory.clear();
            refilter = true;
        } else if( action == "TRAVEL_TO" && activeItem ) {
            // try finding route to the tile, or one tile away from it
            const std::optional<std::vector<tripoint_bub_ms>> try_route =
            safe_route_to( u, u.pos_bub() + active_pos, 1, []( const std::string & msg ) {
                popup( msg );
            } );
            recalc_unread = highlight_unread_items;
            if( try_route.has_value() ) {
                u.set_destination( *try_route );
                break;
            }
        }

        if( refilter ) {
            switch( uistate.list_item_sort ) {
                case list_item_sort_mode::count:
                case list_item_sort_mode::DISTANCE:
                    ground_items = item_list;
                    break;
                case list_item_sort_mode::NAME:
                    std::sort( ground_items.begin(), ground_items.end(), map_item_stack::map_item_stack_sort_name );
                    break;
                case list_item_sort_mode::CATEGORY_DISTANCE:
                    std::sort( ground_items.begin(), ground_items.end(),
                               map_item_stack::map_item_stack_sort_category_distance );
                    break;
                case list_item_sort_mode::CATEGORY_NAME:
                    std::sort( ground_items.begin(), ground_items.end(),
                               map_item_stack::map_item_stack_sort_category_name );
                    break;
            }

            refilter = false;
            addCategory = uistate.list_item_sort == list_item_sort_mode::CATEGORY_DISTANCE ||
                          uistate.list_item_sort == list_item_sort_mode::CATEGORY_NAME;
            filtered_items = filter_item_stacks( ground_items, sFilter );
            highPEnd = list_filter_high_priority( filtered_items, list_item_upvote );
            lowPStart = list_filter_low_priority( filtered_items, highPEnd, list_item_downvote );
            iActive = 0;
            page_num = 0;
            iItemNum = filtered_items.size();
        }

        if( addCategory ) {
            addCategory = false;
            iCatSortNum = 0;
            mSortCategory.clear();
            if( highPEnd > 0 ) {
                mSortCategory[0] = _( "HIGH PRIORITY" );
                iCatSortNum++;
            }
            std::string last_cat_name;
            for( int i = std::max( 0, highPEnd );
                 i < std::min( lowPStart, static_cast<int>( filtered_items.size() ) ); i++ ) {
                const std::string &cat_name =
                    filtered_items[i].example->get_category_of_contents().name_header();
                if( cat_name != last_cat_name ) {
                    mSortCategory[i + iCatSortNum++] = cat_name;
                    last_cat_name = cat_name;
                }
            }
            if( lowPStart < static_cast<int>( filtered_items.size() ) ) {
                mSortCategory[lowPStart + iCatSortNum++] = _( "LOW PRIORITY" );
            }
            if( !mSortCategory[0].empty() ) {
                iActive++;
            }
            iItemNum = static_cast<int>( filtered_items.size() ) + iCatSortNum;
        }

        const int item_info_scroll_lines = catacurses::getmaxy( w_item_info ) - 4;
        const int scroll_rate = iItemNum > 20 ? 10 : 3;

        if( action == "UP" ) {
            do {
                iActive--;
            } while( !mSortCategory[iActive].empty() );
            iScrollPos = 0;
            page_num = 0;
            if( iActive < 0 ) {
                iActive = iItemNum - 1;
            }
            recalc_unread = highlight_unread_items;
        } else if( action == "DOWN" ) {
            do {
                iActive++;
            } while( !mSortCategory[iActive].empty() );
            iScrollPos = 0;
            page_num = 0;
            if( iActive >= iItemNum ) {
                iActive = mSortCategory[0].empty() ? 0 : 1;
            }
            recalc_unread = highlight_unread_items;
        } else if( action == "PAGE_DOWN" ) {
            iScrollPos = 0;
            page_num = 0;
            if( iActive == iItemNum - 1 ) {
                iActive = mSortCategory[0].empty() ? 0 : 1;
            } else if( iActive + scroll_rate >= iItemNum ) {
                iActive = iItemNum - 1;
            } else {
                iActive += +scroll_rate - 1;
                do {
                    iActive++;
                } while( !mSortCategory[iActive].empty() );
            }
        } else if( action == "PAGE_UP" ) {
            iScrollPos = 0;
            page_num = 0;
            if( mSortCategory[0].empty() ? iActive == 0 : iActive == 1 ) {
                iActive = iItemNum - 1;
            } else if( iActive <= scroll_rate ) {
                iActive = mSortCategory[0].empty() ? 0 : 1;
            } else {
                iActive += -scroll_rate + 1;
                do {
                    iActive--;
                } while( !mSortCategory[iActive].empty() );
            }
        } else if( action == "RIGHT" ) {
            if( !filtered_items.empty() && activeItem ) {
                if( ++page_num >= static_cast<int>( activeItem->vIG.size() ) ) {
                    page_num = activeItem->vIG.size() - 1;
                }
            }
            recalc_unread = highlight_unread_items;
        } else if( action == "LEFT" ) {
            page_num = std::max( 0, page_num - 1 );
            recalc_unread = highlight_unread_items;
        } else if( action == "SCROLL_ITEM_INFO_UP" ) {
            iScrollPos -= item_info_scroll_lines;
            recalc_unread = highlight_unread_items;
        } else if( action == "SCROLL_ITEM_INFO_DOWN" ) {
            iScrollPos += item_info_scroll_lines;
            recalc_unread = highlight_unread_items;
        } else if( action == "zoom_in" ) {
            zoom_in();
            mark_main_ui_adaptor_resize();
        } else if( action == "zoom_out" ) {
            zoom_out();
            mark_main_ui_adaptor_resize();
        } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
            u.view_offset = stored_view_offset;
            return game::vmenu_ret::CHANGE_TAB;
        }

        active_pos = tripoint_rel_ms::zero;
        activeItem = nullptr;

        if( mSortCategory[iActive].empty() ) {
            auto iter = filtered_items.begin();
            for( int iNum = 0; iter != filtered_items.end() && iNum < iActive; iNum++ ) {
                if( mSortCategory[iNum].empty() ) {
                    ++iter;
                }
            }
            if( iter != filtered_items.end() ) {
                active_pos = iter->vIG[page_num].pos;
                activeItem = &( *iter );
            }
        }

        if( activeItem ) {
            centerlistview( active_pos.raw(), width );
            trail_start = u.pos_bub();
            trail_end = u.pos_bub() + active_pos;
            // Actually accessed from the terrain overlay callback `trail_cb` in the
            // call to `ui_manager::redraw`.
            //NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
            trail_end_x = true;
            if( recalc_unread ) {
                mark_items_read_rec( activeItem->example );
            }
        } else {
            u.view_offset = stored_view_offset;
            trail_start = trail_end = std::nullopt;
        }
        invalidate_main_ui_adaptor();

        ui_manager::redraw();

        action = ctxt.handle_input();
    } while( action != "QUIT" );

    u.view_offset = stored_view_offset;
    return game::vmenu_ret::QUIT;
}

game::vmenu_ret game::list_monsters( const std::vector<Creature *> &monster_list )
{
    const map &here = get_map();

    const int iInfoHeight = 15;
    const int width = 55;
    int offsetX = 0;
    int iMaxRows = 0;

    catacurses::window w_monsters;
    catacurses::window w_monsters_border;
    catacurses::window w_monster_info;
    catacurses::window w_monster_info_border;

    Creature *cCurMon = nullptr;
    tripoint iActivePos;

    bool hide_ui = false;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        if( hide_ui ) {
            ui.position( point::zero, point::zero );
        } else {
            offsetX = TERMX - width;
            iMaxRows = TERMY - iInfoHeight - 1;

            w_monsters = catacurses::newwin( iMaxRows, width - 2, point( offsetX + 1,
                                             1 ) );
            w_monsters_border = catacurses::newwin( iMaxRows + 1, width, point( offsetX,
                                                    0 ) );
            w_monster_info = catacurses::newwin( iInfoHeight - 2, width - 2,
                                                 point( offsetX + 1, TERMY - iInfoHeight + 1 ) );
            w_monster_info_border = catacurses::newwin( iInfoHeight, width, point( offsetX,
                                    TERMY - iInfoHeight ) );

            if( cCurMon ) {
                centerlistview( iActivePos, width );
            }

            ui.position( point( offsetX, 0 ), point( width, TERMY ) );
        }
    } );
    ui.mark_resize();

    int max_gun_range = 0;
    if( u.get_wielded_item() ) {
        max_gun_range = u.get_wielded_item()->gun_range( &u );
    }

    const tripoint_rel_ms stored_view_offset = u.view_offset;
    u.view_offset = tripoint_rel_ms::zero;

    int iActive = 0; // monster index that we're looking at

    std::string action;
    input_context ctxt( "LIST_MONSTERS" );
    ctxt.register_navigate_ui_list();
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "zoom_in" );
    ctxt.register_action( "zoom_out" );
    ctxt.register_action( "SAFEMODE_BLACKLIST_ADD" );
    ctxt.register_action( "SAFEMODE_BLACKLIST_REMOVE" );
    ctxt.register_action( "QUIT" );
    if( bVMonsterLookFire ) {
        ctxt.register_action( "look" );
        ctxt.register_action( "fire" );
    }
    ctxt.register_action( "HELP_KEYBINDINGS" );

    // first integer is the row the attitude category string is printed in the menu
    std::map<int, Creature::Attitude> mSortCategory;
    const bool player_knows = !u.has_trait( trait_INATTENTIVE );
    if( player_knows ) {
        for( int i = 0, last_attitude = -1; i < static_cast<int>( monster_list.size() ); i++ ) {
            const Creature::Attitude attitude = monster_list[i]->attitude_to( u );
            if( static_cast<int>( attitude ) != last_attitude ) {
                mSortCategory[i + mSortCategory.size()] = attitude;
                last_attitude = static_cast<int>( attitude );
            }
        }
    }

    ui.on_redraw( [&]( const ui_adaptor & ) {
        if( !hide_ui ) {
            draw_custom_border( w_monsters_border, 1, 1, 1, 1, 1, 1, LINE_XOXO, LINE_XOXO );
            draw_custom_border( w_monster_info_border, 1, 1, 1, 1, LINE_XXXO, LINE_XOXX, 1, 1 );

            mvwprintz( w_monsters_border, point( 2, 0 ), c_light_green, "<Tab> " );
            wprintz( w_monsters_border, c_white, _( "Monsters" ) );

            if( monster_list.empty() ) {
                werase( w_monsters );
                mvwprintz( w_monsters, point( 2, iMaxRows / 3 ), c_white,
                           _( "You don't see any monsters around you!" ) );
            } else {
                werase( w_monsters );

                const int iNumMonster = monster_list.size();
                const int iMenuSize = monster_list.size() + mSortCategory.size();

                const int numw = iNumMonster > 999 ? 4 :
                                 iNumMonster > 99 ? 3 :
                                 iNumMonster > 9 ? 2 : 1;

                // given the currently selected monster iActive. get the selected row
                int iSelPos = iActive;
                for( auto &ia : mSortCategory ) {
                    int index = ia.first;
                    if( index <= iSelPos ) {
                        ++iSelPos;
                    } else {
                        break;
                    }
                }
                int iStartPos = 0;
                // use selected row get the start row
                calcStartPos( iStartPos, iSelPos, iMaxRows - 1, iMenuSize );

                // get first visible monster and category
                int iCurMon = iStartPos;
                auto CatSortIter = mSortCategory.cbegin();
                while( CatSortIter != mSortCategory.cend() && CatSortIter->first < iStartPos ) {
                    ++CatSortIter;
                    --iCurMon;
                }

                std::string monNameSelected;
                std::string sSafemode;
                const int endY = std::min<int>( iMaxRows - 1, iMenuSize );
                for( int y = 0; y < endY; ++y ) {

                    if( player_knows && CatSortIter != mSortCategory.cend() ) {
                        const int iCurPos = iStartPos + y;
                        const int iCatPos = CatSortIter->first;
                        if( iCurPos == iCatPos ) {
                            const std::string cat_name = Creature::get_attitude_ui_data(
                                                             CatSortIter->second ).first.translated();
                            mvwprintz( w_monsters, point( 1, y ), c_magenta, cat_name );
                            ++CatSortIter;
                            continue;
                        }
                    }

                    // select current monster
                    Creature *critter = monster_list[iCurMon];
                    const bool selected = iCurMon == iActive;
                    ++iCurMon;
                    if( critter->sees( here, u ) && player_knows ) {
                        mvwprintz( w_monsters, point( 0, y ), c_yellow, "!" );
                    }
                    bool is_npc = false;
                    const monster *m = dynamic_cast<monster *>( critter );
                    const npc *p = dynamic_cast<npc *>( critter );
                    nc_color name_color = critter->basic_symbol_color();

                    if( selected ) {
                        name_color = hilite( name_color );
                    }

                    if( m != nullptr ) {
                        trim_and_print( w_monsters, point( 1, y ), width - 32, name_color, m->name() );
                    } else {
                        trim_and_print( w_monsters, point( 1, y ), width - 32, name_color, critter->disp_name() );
                        is_npc = true;
                    }

                    if( selected && !get_safemode().empty() ) {
                        monNameSelected = is_npc ? get_safemode().npc_type_name() : m->name();

                        if( get_safemode().has_rule( monNameSelected, Creature::Attitude::ANY ) ) {
                            sSafemode = _( "<R>emove from safe mode blacklist" );
                        } else {
                            sSafemode = _( "<A>dd to safe mode blacklist" );
                        }
                    }

                    nc_color color = c_white;
                    std::string sText;

                    if( m != nullptr ) {
                        m->get_HP_Bar( color, sText );
                    } else {
                        std::tie( sText, color ) =
                            ::get_hp_bar( critter->get_hp(), critter->get_hp_max(), false );
                    }
                    mvwprintz( w_monsters, point( width - 31, y ), color, sText );
                    const int bar_max_width = 5;
                    const int bar_width = utf8_width( sText );
                    wattron( w_monsters, c_white );
                    for( int i = 0; i < bar_max_width - bar_width; ++i ) {
                        mvwprintw( w_monsters, point( width - 27 - i, y ), "." );
                    }
                    wattron( w_monsters, c_white );

                    if( m != nullptr ) {
                        const auto att = m->get_attitude();
                        sText = att.first;
                        color = att.second;
                    } else if( p != nullptr ) {
                        sText = npc_attitude_name( p->get_attitude() );
                        color = p->symbol_color();
                    }
                    if( !player_knows ) {
                        sText = _( "Unknown" );
                        color = c_yellow;
                    }
                    mvwprintz( w_monsters, point( width - 19, y ), color, sText );

                    const int mon_dist = rl_dist( u.pos_bub(), critter->pos_bub() );
                    const int numd = mon_dist > 999 ? 4 :
                                     mon_dist > 99 ? 3 :
                                     mon_dist > 9 ? 2 : 1;

                    trim_and_print( w_monsters, point( width - ( 5 + numd ), y ), 6 + numd,
                                    selected ? c_light_green : c_light_gray,
                                    "%*d %s",
                                    numd, mon_dist,
                                    direction_name_short( direction_from( u.pos_bub(), critter->pos_bub() ) ) );
                }

                mvwprintz( w_monsters_border, point( ( width / 2 ) - numw - 2, 0 ), c_light_green, " %*d", numw,
                           iActive + 1 );
                wprintz( w_monsters_border, c_white, " / %*d ", numw, static_cast<int>( monster_list.size() ) );

                werase( w_monster_info );
                if( cCurMon ) {
                    cCurMon->print_info( w_monster_info, 1, iInfoHeight - 3, 1 );
                }

                draw_custom_border( w_monster_info_border, 1, 1, 1, 1, LINE_XXXO, LINE_XOXX, 1, 1 );

                if( bVMonsterLookFire ) {
                    mvwprintw( w_monster_info_border, point::east, "< " );
                    wprintz( w_monster_info_border, c_light_green, ctxt.press_x( "look" ) );
                    wprintz( w_monster_info_border, c_light_gray, " %s", _( "to look around" ) );

                    if( cCurMon && rl_dist( u.pos_bub(), cCurMon->pos_bub() ) <= max_gun_range ) {
                        std::string press_to_fire_text = string_format( _( "%s %s" ),
                                                         ctxt.press_x( "fire" ),
                                                         string_format( _( "<color_light_gray>to shoot</color>" ) ) );
                        right_print( w_monster_info_border, 0, 3, c_light_green, press_to_fire_text );
                    }
                    wprintw( w_monster_info_border, " >" );
                }

                if( !get_safemode().empty() ) {
                    if( get_safemode().has_rule( monNameSelected, Creature::Attitude::ANY ) ) {
                        sSafemode = _( "<R>emove from safe mode blacklist" );
                    } else {
                        sSafemode = _( "<A>dd to safe mode blacklist" );
                    }

                    shortcut_print( w_monsters, point( 2, getmaxy( w_monsters ) - 1 ),
                                    c_white, c_light_green, sSafemode );
                }

                draw_scrollbar( w_monsters_border, iActive, iMaxRows, static_cast<int>( monster_list.size() ),
                                point::south );
            }

            wnoutrefresh( w_monsters_border );
            wnoutrefresh( w_monster_info_border );
            wnoutrefresh( w_monsters );
            wnoutrefresh( w_monster_info );
        }
    } );

    std::optional<tripoint_bub_ms> trail_start;
    std::optional<tripoint_bub_ms> trail_end;
    bool trail_end_x = false;
    shared_ptr_fast<draw_callback_t> trail_cb = create_trail_callback( trail_start, trail_end,
            trail_end_x );
    add_draw_callback( trail_cb );
    const int recmax = static_cast<int>( monster_list.size() );
    const int scroll_rate = recmax > 20 ? 10 : 3;

    do {
        if( navigate_ui_list( action, iActive, scroll_rate, recmax, true ) ) {
        } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
            u.view_offset = stored_view_offset;
            return game::vmenu_ret::CHANGE_TAB;
        } else if( action == "zoom_in" ) {
            zoom_in();
            mark_main_ui_adaptor_resize();
        } else if( action == "zoom_out" ) {
            zoom_out();
            mark_main_ui_adaptor_resize();
        } else if( action == "SAFEMODE_BLACKLIST_REMOVE" ) {
            const monster *m = dynamic_cast<monster *>( cCurMon );
            const std::string monName = ( m != nullptr ) ? m->name() : "human";

            if( get_safemode().has_rule( monName, Creature::Attitude::ANY ) ) {
                get_safemode().remove_rule( monName, Creature::Attitude::ANY );
            }
        } else if( action == "SAFEMODE_BLACKLIST_ADD" ) {
            if( !get_safemode().empty() ) {
                const monster *m = dynamic_cast<monster *>( cCurMon );
                const std::string monName = ( m != nullptr ) ? m->name() : "human";

                get_safemode().add_rule( monName, Creature::Attitude::ANY, get_option<int>( "SAFEMODEPROXIMITY" ),
                                         rule_state::BLACKLISTED );
            }
        } else if( action == "look" ) {
            hide_ui = true;
            ui.mark_resize();
            look_around();
            hide_ui = false;
            ui.mark_resize();
        } else if( action == "fire" ) {
            if( cCurMon != nullptr && rl_dist( u.pos_bub(), cCurMon->pos_bub() ) <= max_gun_range ) {
                u.last_target = shared_from( *cCurMon );
                u.recoil = MAX_RECOIL;
                u.view_offset = stored_view_offset;
                return game::vmenu_ret::FIRE;
            }
        }

        if( iActive >= 0 && static_cast<size_t>( iActive ) < monster_list.size() ) {
            cCurMon = monster_list[iActive];
            iActivePos = ( cCurMon->pos_bub() - u.pos_bub() ).raw();
            centerlistview( iActivePos, width );
            trail_start = u.pos_bub();
            trail_end = cCurMon->pos_bub();
            // Actually accessed from the terrain overlay callback `trail_cb` in the
            // call to `ui_manager::redraw`.
            //NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
            trail_end_x = false;
        } else {
            cCurMon = nullptr;
            iActivePos = tripoint::zero;
            u.view_offset = stored_view_offset;
            trail_start = trail_end = std::nullopt;
        }
        invalidate_main_ui_adaptor();

        ui_manager::redraw();

        action = ctxt.handle_input();
    } while( action != "QUIT" );

    u.view_offset = stored_view_offset;

    return game::vmenu_ret::QUIT;
}

// Used to set up the first Hotkey in the display set
static std::optional<input_event> get_initial_hotkey( const size_t menu_index )
{
    return menu_index == 0
           ? hotkey_for_action( ACTION_BUTCHER, /*maximum_modifier_count=*/1 )
           : std::nullopt;
}

// Returns a vector of pairs.
//    Pair.first is the iterator to the first item with a unique tname.
//    Pair.second is the number of equivalent items per unique tname
// There are options for optimization here, but the function is hit infrequently
// enough that optimizing now is not a useful time expenditure.
static std::vector<std::pair<map_stack::iterator, int>> generate_butcher_stack_display(
            const std::vector<map_stack::iterator> &its )
{
    std::vector<std::pair<map_stack::iterator, int>> result;
    std::vector<std::string> result_strings;
    result.reserve( its.size() );
    result_strings.reserve( its.size() );

    for( const map_stack::iterator &it : its ) {
        const std::string tname = it->tname();
        size_t s = 0;
        // Search for the index with a string equivalent to tname
        for( ; s < result_strings.size(); ++s ) {
            if( result_strings[s] == tname ) {
                break;
            }
        }
        // If none is found, this is a unique tname so we need to add
        // the tname to string vector, and make an empty result pair.
        // Has the side effect of making 's' a valid index
        if( s == result_strings.size() ) {
            // make a new entry
            result.emplace_back( it, 0 );
            // Also push new entry string
            result_strings.push_back( tname );
        }
        result[s].second += it->count_by_charges() ? it->charges : 1;
    }

    return result;
}

// Corpses are always individual items
// Just add them individually to the menu
static void add_corpses( uilist &menu, const std::vector<map_stack::iterator> &its,
                         size_t &menu_index )
{
    std::optional<input_event> hotkey = get_initial_hotkey( menu_index );

    for( const map_stack::iterator &it : its ) {
        menu.addentry( menu_index++, true, hotkey, it->get_mtype()->nname() );
        hotkey = std::nullopt;
    }
}

// Salvagables stack so we need to pass in a stack vector rather than an item index vector
static void add_salvagables( uilist &menu,
                             const std::vector<std::pair<map_stack::iterator, int>> &stacks,
                             size_t &menu_index, const salvage_actor &salvage_iuse )
{
    if( !stacks.empty() ) {
        std::optional<input_event> hotkey = get_initial_hotkey( menu_index );

        for( const auto &stack : stacks ) {
            const item &it = *stack.first;

            //~ Name and number of items listed for cutting up
            const std::string &msg = string_format( pgettext( "butchery menu", "Cut up %s (%d)" ),
                                                    it.tname(), stack.second );
            menu.addentry_col( menu_index++, true, hotkey, msg,
                               to_string_clipped( time_duration::from_turns( salvage_iuse.time_to_cut_up( it ) / 100 ) ) );
            hotkey = std::nullopt;
        }
    }
}

// Disassemblables stack so we need to pass in a stack vector rather than an item index vector
static void add_disassemblables( uilist &menu,
                                 const std::vector<std::pair<map_stack::iterator, int>> &stacks, size_t &menu_index )
{
    if( !stacks.empty() ) {
        std::optional<input_event> hotkey = get_initial_hotkey( menu_index );

        for( const auto &stack : stacks ) {
            const item &it = *stack.first;

            //~ Name, number of items and time to complete disassembling
            const std::string &msg = string_format( pgettext( "butchery menu", "Disassemble %s (%d)" ),
                                                    it.tname(), stack.second );
            recipe uncraft_recipe;
            if( it.typeId() == itype_disassembly ) {
                uncraft_recipe = it.get_making();
            } else {
                uncraft_recipe = recipe_dictionary::get_uncraft( it.typeId() );
            }
            menu.addentry_col( menu_index++, true, hotkey, msg,
                               to_string_clipped( uncraft_recipe.time_to_craft( get_player_character(),
                                                  recipe_time_flag::ignore_proficiencies ) ) );
            hotkey = std::nullopt;
        }
    }
}

void game::butcher()
{
    map &here = get_map();

    static const std::string salvage_string = "salvage";
    if( u.controlling_vehicle ) {
        add_msg( m_info, _( "You can't butcher while driving!" ) );
        return;
    }

    const int factor = u.max_quality( qual_BUTCHER, PICKUP_RANGE );
    const int factorD = u.max_quality( qual_CUT_FINE, PICKUP_RANGE );
    const std::string no_knife_msg = _( "You don't have a butchering tool." );
    const std::string no_corpse_msg = _( "There are no corpses here to butcher." );

    //You can't butcher on sealed terrain- you have to smash/shovel/etc it open first
    if( here.has_flag( ter_furn_flag::TFLAG_SEALED, u.pos_bub() ) ) {
        if( here.sees_some_items( u.pos_bub(), u ) ) {
            add_msg( m_info, _( "You can't access the items here." ) );
        } else if( factor > INT_MIN || factorD > INT_MIN ) {
            add_msg( m_info, no_corpse_msg );
        } else {
            add_msg( m_info, no_knife_msg );
        }
        return;
    }

    const item *first_item_without_tools = nullptr;
    // Indices of relevant items
    std::vector<map_stack::iterator> corpses;
    std::vector<map_stack::iterator> disassembles;
    std::vector<map_stack::iterator> salvageables;
    map_stack items = here.i_at( u.pos_bub() );
    const inventory &crafting_inv = u.crafting_inventory();

    // TODO: Properly handle different material whitelists
    // TODO: Improve quality of this section
    auto salvage_filter = []( const item & it ) {
        const item *usable = it.get_usable_item( salvage_string );
        return usable != nullptr;
    };

    std::vector< item * > salvage_tools = u.items_with( salvage_filter );
    int salvage_tool_index = INT_MIN;
    item *salvage_tool = nullptr;
    const salvage_actor *salvage_iuse = nullptr;
    if( !salvage_tools.empty() ) {
        salvage_tool = salvage_tools.front();
        salvage_tool_index = u.get_item_position( salvage_tool );
        item *usable = salvage_tool->get_usable_item( salvage_string );
        salvage_iuse = dynamic_cast<const salvage_actor *>(
                           usable->get_use( salvage_string )->get_actor_ptr() );
    }

    // Reserve capacity for each to hold entire item set if necessary to prevent
    // reallocations later on
    corpses.reserve( items.size() );
    salvageables.reserve( items.size() );
    disassembles.reserve( items.size() );

    // Split into corpses, disassemble-able, and salvageable items
    // It's not much additional work to just generate a corpse list and
    // clear it later, but does make the splitting process nicer.
    for( map_stack::iterator it = items.begin(); it != items.end(); ++it ) {
        if( it->is_corpse() ) {
            corpses.push_back( it );
        } else {
            if( ( salvage_tool_index != INT_MIN ) && salvage_iuse->valid_to_cut_up( nullptr, *it ) ) {
                salvageables.push_back( it );
            }
            if( u.can_disassemble( *it, crafting_inv ).success() ) {
                disassembles.push_back( it );
            } else if( !first_item_without_tools ) {
                first_item_without_tools = &*it;
            }
        }
    }

    // Clear corpses if butcher and dissect factors are INT_MIN
    if( factor == INT_MIN && factorD == INT_MIN ) {
        corpses.clear();
    }

    if( corpses.empty() && disassembles.empty() && salvageables.empty() ) {
        if( factor > INT_MIN || factorD > INT_MIN ) {
            add_msg( m_info, no_corpse_msg );
        } else {
            add_msg( m_info, no_knife_msg );
        }

        if( first_item_without_tools ) {
            add_msg( m_info, _( "You don't have the necessary tools to disassemble any items here." ) );
            // Just for the "You need x to disassemble y" messages
            const auto ret = u.can_disassemble( *first_item_without_tools, crafting_inv );
            if( !ret.success() ) {
                add_msg( m_info, "%s", ret.c_str() );
            }
        }
        return;
    }

    Creature *hostile_critter = is_hostile_very_close( true );
    if( hostile_critter != nullptr ) {
        if( !query_yn( _( "You see %s nearby!  Start butchering anyway?" ),
                       hostile_critter->disp_name() ) ) {
            return;
        }
    }

    // Magic indices for special butcher options
    enum : int {
        MULTISALVAGE = MAX_ITEM_IN_SQUARE + 1,
        MULTIBUTCHER,
        MULTIDISASSEMBLE_ONE,
        MULTIDISASSEMBLE_ALL,
        NUM_BUTCHER_ACTIONS
    };
    // What are we butchering (i.e.. which vector to pick indices from)
    enum {
        BUTCHER_CORPSE,
        BUTCHER_DISASSEMBLE,
        BUTCHER_SALVAGE,
        BUTCHER_OTHER // For multisalvage etc.
    } butcher_select = BUTCHER_CORPSE;
    // Index to std::vector of iterators...
    int indexer_index = 0;

    // Generate the indexed stacks so we can display them nicely
    const auto disassembly_stacks = generate_butcher_stack_display( disassembles );
    const auto salvage_stacks = generate_butcher_stack_display( salvageables );
    // Always ask before cutting up/disassembly, but not before butchery
    size_t ret = 0;
    if( !corpses.empty() || !disassembles.empty() || !salvageables.empty() ) {
        uilist kmenu;
        kmenu.text = _( "Choose corpse to butcher / item to disassemble" );

        size_t i = 0;
        // Add corpses, disassembleables, and salvagables to the UI
        add_corpses( kmenu, corpses, i );
        add_disassemblables( kmenu, disassembly_stacks, i );
        if( salvage_iuse && !salvageables.empty() ) {
            add_salvagables( kmenu, salvage_stacks, i, *salvage_iuse );
        }

        if( corpses.size() > 1 ) {
            kmenu.addentry( MULTIBUTCHER, true, 'b', _( "Butcher everything" ) );
        }

        if( disassembly_stacks.size() > 1 || ( disassembly_stacks.size() == 1 &&
                                               disassembly_stacks.front().second > 1 ) ) {
            int time_to_disassemble_once = 0;
            int time_to_disassemble_recursive = 0;
            for( const auto &stack : disassembly_stacks ) {
                recipe uncraft_recipe;
                if( stack.first->typeId() == itype_disassembly ) {
                    uncraft_recipe = stack.first->get_making();
                } else {
                    uncraft_recipe = recipe_dictionary::get_uncraft( stack.first->typeId() );
                }

                const int time = uncraft_recipe.time_to_craft_moves(
                                     get_player_character(), recipe_time_flag::ignore_proficiencies );
                time_to_disassemble_once += time * stack.second;
                if( stack.first->typeId() == itype_disassembly ) {
                    item test( uncraft_recipe.result(), calendar::turn, 1 );
                    time_to_disassemble_recursive += test.get_recursive_disassemble_moves(
                                                         get_player_character() ) * stack.second;
                } else {
                    time_to_disassemble_recursive += stack.first->get_recursive_disassemble_moves(
                                                         get_player_character() ) * stack.second;
                }

            }

            kmenu.addentry_col( MULTIDISASSEMBLE_ONE, true, 'D', _( "Disassemble everything once" ),
                                to_string_clipped( time_duration::from_moves( time_to_disassemble_once ) ) );
            kmenu.addentry_col( MULTIDISASSEMBLE_ALL, true, 'd', _( "Disassemble everything recursively" ),
                                to_string_clipped( time_duration::from_moves( time_to_disassemble_recursive ) ) );
        }
        if( salvage_iuse && salvageables.size() > 1 ) {
            int time_to_salvage = 0;
            for( const auto &stack : salvage_stacks ) {
                time_to_salvage += salvage_iuse->time_to_cut_up( *stack.first ) * stack.second;
            }

            kmenu.addentry_col( MULTISALVAGE, true, 'z', _( "Cut up everything" ),
                                to_string_clipped( time_duration::from_moves( time_to_salvage ) ) );
        }

        kmenu.query();

        if( kmenu.ret < 0 || kmenu.ret >= NUM_BUTCHER_ACTIONS ) {
            return;
        }

        ret = static_cast<size_t>( kmenu.ret );
        if( ret >= MULTISALVAGE && ret < NUM_BUTCHER_ACTIONS ) {
            butcher_select = BUTCHER_OTHER;
            indexer_index = ret;
        } else if( ret < corpses.size() ) {
            butcher_select = BUTCHER_CORPSE;
            indexer_index = ret;
        } else if( ret < corpses.size() + disassembly_stacks.size() ) {
            butcher_select = BUTCHER_DISASSEMBLE;
            indexer_index = ret - corpses.size();
        } else if( ret < corpses.size() + disassembly_stacks.size() + salvage_stacks.size() ) {
            butcher_select = BUTCHER_SALVAGE;
            indexer_index = ret - corpses.size() - disassembly_stacks.size();
        } else {
            debugmsg( "Invalid butchery index: %d", ret );
            return;
        }
    }

    if( !u.has_morale_to_craft() ) {
        if( butcher_select == BUTCHER_CORPSE || indexer_index == MULTIBUTCHER ) {
            add_msg( m_info,
                     _( "You are not in the mood and the prospect of guts and blood on your hands convinces you to turn away." ) );
        } else {
            add_msg( m_info,
                     _( "You are not in the mood and the prospect of work stops you before you begin." ) );
        }
        return;
    }
    const std::vector<Character *> helpers = u.get_crafting_helpers();
    for( std::size_t i = 0; i < helpers.size() && i < 3; i++ ) {
        add_msg( m_info, _( "%s helps with this task…" ), helpers[i]->get_name() );
    }
    switch( butcher_select ) {
        case BUTCHER_OTHER:
            switch( indexer_index ) {
                case MULTISALVAGE:
                    u.assign_activity( longsalvage_activity_actor( salvage_tool_index ) );
                    break;
                case MULTIBUTCHER: {
                    const std::optional<butcher_type> bt = butcher_submenu( corpses );
                    if( bt.has_value() ) {
                        std::vector<butchery_data> bd;
                        for( map_stack::iterator &it : corpses ) {
                            item_location corpse_loc = item_location( map_cursor( u.pos_abs() ), &*it );
                            bd.emplace_back( corpse_loc, bt.value() );
                        }
                        u.assign_activity( butchery_activity_actor( bd ) );
                    }
                    break;
                }
                case MULTIDISASSEMBLE_ONE:
                    u.disassemble_all( true );
                    break;
                case MULTIDISASSEMBLE_ALL:
                    u.disassemble_all( false );
                    break;
                default:
                    debugmsg( "Invalid butchery type: %d", indexer_index );
                    return;
            }
            break;
        case BUTCHER_CORPSE: {
            const std::optional<butcher_type> bt = butcher_submenu( corpses, indexer_index );
            if( bt.has_value() ) {
                item_location corpse_loc = item_location( map_cursor( u.pos_abs() ), &*corpses[indexer_index] );
                butchery_data bd( corpse_loc, bt.value() );
                u.assign_activity( butchery_activity_actor( bd ) );
            }
        }
        break;
        case BUTCHER_DISASSEMBLE: {
            // Pick index of first item in the disassembly stack
            item *const target = &*disassembly_stacks[indexer_index].first;
            u.disassemble( item_location( map_cursor( u.pos_abs() ), target ), true );
        }
        break;
        case BUTCHER_SALVAGE: {
            if( !salvage_iuse || !salvage_tool ) {
                debugmsg( "null salvage_iuse or salvage_tool" );
            } else {
                // Pick index of first item in the salvage stack
                item *const target = &*salvage_stacks[indexer_index].first;
                item_location item_loc( map_cursor( u.pos_abs() ), target );
                salvage_iuse->try_to_cut_up( u, *salvage_tool, item_loc );
            }
        }
        break;
    }
}

std::optional<tripoint_bub_ms> game::point_selection_menu( const std::vector<tripoint_bub_ms> &pts,
        bool up )
{
    if( pts.empty() ) {
        debugmsg( "point_selection_menu called with empty point set" );
        return std::nullopt;
    }

    if( pts.size() == 1 ) {
        return pts[0];
    }

    const tripoint_bub_ms upos = get_player_character().pos_bub();
    uilist pmenu;
    pmenu.title = _( "Climb where?" );
    int num = 0;
    for( const tripoint_bub_ms &pt : pts ) {
        // TODO: Sort the menu so that it can be used with numpad directions
        const std::string &dir_arrow = direction_arrow( direction_from( upos.xy(), pt.xy() ) );
        const std::string &dir_name = direction_name( direction_from( upos.xy(), pt.xy() ) );
        const std::string &up_or_down = up ? _( "Climb up %s (%s)" ) : _( "Climb down %s (%s)" );
        // TODO: Inform player what is on said tile
        // But don't just print terrain name (in many cases it will be "open air")
        pmenu.addentry( num++, true, MENU_AUTOASSIGN, string_format( up_or_down, dir_name, dir_arrow ) );
    }

    pmenu.query();
    const int ret = pmenu.ret;
    if( ret < 0 || ret >= num ) {
        return std::nullopt;
    }

    return pts[ret];
}

// Redraw window and show spinner, so cata window doesn't look frozen while pathfinding on overmap
void game::display_om_pathfinding_progress( size_t /* open_set */, size_t /* known_size */ )
{
    ui_adaptor dummy( ui_adaptor::disable_uis_below{} );
    static_popup pop;
    pop.on_top( true ).wait_message( "%s", _( "Hang on a bit…" ) );
    ui_manager::redraw();
    refresh_display();
    inp_mngr.pump_events();
}

void game::wait_popup_reset()
{
    wait_popup.reset();
}

bool game::display_overlay_state( const action_id action )
{
    return displaying_overlays && *displaying_overlays == action;
}

void game::display_toggle_overlay( const action_id action )
{
    if( display_overlay_state( action ) ) {
        displaying_overlays.reset();
    } else {
        displaying_overlays = action;
    }
}

void game::display_scent()
{
    if( !use_tiles ) {
        int div = 0;
        if( !query_int( div, false, _( "Set scent map sensitivity to?" ) ) || div != 0 ) {
            return;
        }
        shared_ptr_fast<game::draw_callback_t> scent_cb = make_shared_fast<game::draw_callback_t>( [&]() {
            scent.draw( w_terrain, div * 2, u.pos_bub() + u.view_offset );
        } );
        g->add_draw_callback( scent_cb );

        ui_manager::redraw();
        inp_mngr.wait_for_any_key();
    }
}

void game::display_visibility()
{
    if( use_tiles ) {
        display_toggle_overlay( ACTION_DISPLAY_VISIBILITY );
        if( display_overlay_state( ACTION_DISPLAY_VISIBILITY ) ) {
            std::vector< tripoint_bub_ms > locations;
            uilist creature_menu;
            int num_creatures = 0;
            creature_menu.addentry( num_creatures++, true, MENU_AUTOASSIGN, "%s", _( "You" ) );
            locations.emplace_back( get_player_character().pos_bub() ); // add player first.
            for( const Creature &critter : g->all_creatures() ) {
                if( critter.is_avatar() ) {
                    continue;
                }
                creature_menu.addentry( num_creatures++, true, MENU_AUTOASSIGN, critter.disp_name() );
                locations.emplace_back( critter.pos_bub() );
            }

            pointmenu_cb callback( locations );
            creature_menu.callback = &callback;
            creature_menu.query();
            if( creature_menu.ret >= 0 && static_cast<size_t>( creature_menu.ret ) < locations.size() ) {
                Creature *creature = get_creature_tracker().creature_at<Creature>( locations[creature_menu.ret] );
                displaying_visibility_creature = creature;
            }
        } else {
            popup( _( "No support for curses mode" ) );
            displaying_visibility_creature = nullptr;
        }
    }
}

void game::display_lighting()
{
    if( use_tiles ) {
        display_toggle_overlay( ACTION_DISPLAY_LIGHTING );
        if( !g->display_overlay_state( ACTION_DISPLAY_LIGHTING ) ) {
            return;
        }
        uilist lighting_menu;
        std::vector<std::string> lighting_menu_strings{
            "Global lighting conditions"
        };

        int count = 0;
        for( const auto &menu_str : lighting_menu_strings ) {
            lighting_menu.addentry( count++, true, MENU_AUTOASSIGN, "%s", menu_str );
        }

        lighting_menu.query();
        if( ( lighting_menu.ret >= 0 ) &&
            ( static_cast<size_t>( lighting_menu.ret ) < lighting_menu_strings.size() ) ) {
            g->displaying_lighting_condition = lighting_menu.ret;
        }
    } else {
        popup( _( "No support for curses mode" ) );
    }
}

// These helpers map climbing aid IDs to/from integers for use as return values in uilist.
//   The integers are offset by 4096 to avoid collision with other options (see iexamine::ledge)
static int climb_affordance_menu_encode( const climbing_aid_id &aid_id )
{
    return 0x1000 + int_id<climbing_aid>( aid_id ).to_i();
}
static bool climb_affordance_menu_decode( int retval, climbing_aid_id &aid_id )
{
    int_id< climbing_aid > as_int_id( retval - 0x1000 );
    if( as_int_id.is_valid() ) {
        aid_id = as_int_id.id();
        return true;
    }
    return false;
}

void game::climb_down_menu_gen( const tripoint_bub_ms &examp, uilist &cmenu )
{
    // NOTE this menu may be merged with the iexamine::ledge menu, manage keys carefully!

    map &here = get_map();
    Character &you = get_player_character();

    if( !here.valid_move( you.pos_bub(), examp, false, true ) ) {
        // Can't move horizontally to the ledge
        return;
    }

    // Scan the height of the drop and what's in the way.
    const climbing_aid::fall_scan fall( examp );

    add_msg_debug( debugmode::DF_IEXAMINE, "Ledge height %d", fall.height );
    if( fall.height == 0 ) {
        you.add_msg_if_player( _( "You can't climb down there." ) );
        return;
    }

    // This is used to mention object names.  TODO make this more flexible.
    std::string target_disp_name = here.disp_name( fall.pos_furniture_or_floor() );

    climbing_aid::condition_list conditions = climbing_aid::detect_conditions( you, examp );

    climbing_aid::aid_list aids = climbing_aid::list( conditions );

    // Debug:
    {
        add_msg_debug( debugmode::DF_IEXAMINE, "Climbing aid conditions: %d", conditions.size() );
        for( climbing_aid::condition &cond : conditions ) {
            add_msg_debug( debugmode::DF_IEXAMINE, cond.category_string() + ": " + cond.flag );
        }
        add_msg_debug( debugmode::DF_IEXAMINE, "Climbing aids available: %d", aids.size() );
        for( const climbing_aid *aid : climbing_aid::list_all( conditions ) ) {
            add_msg_debug( debugmode::DF_IEXAMINE, "#%d %s: %s (%s)",
                           climb_affordance_menu_encode( aid->id ), aid->id.str(),
                           aid->down.menu_text.translated(), target_disp_name );
        }
        add_msg_debug( debugmode::DF_IEXAMINE, "%d-level drop; %d until furniture; %d until creature.",
                       fall.height, fall.height_until_furniture, fall.height_until_creature );
    }

    for( const climbing_aid *aid : aids ) {
        // Enable climbing aid unless it would deploy furniture in occupied tiles.
        bool enable_aid = true;
        if( aid->down.deploys_furniture() &&
            fall.height_until_furniture < std::min( fall.height, aid->down.max_height ) ) {
            // Can't deploy because it would overwrite existing furniture.
            enable_aid = false;
        }

        // Certain climbing aids can't be used for partial descent.
        if( !aid->down.allow_remaining_height && aid->down.max_height < fall.height ) {
            // TODO this check could block the safest non-deploying aid!
            enable_aid = false;
        }

        int hotkey = aid->down.menu_hotkey;
        if( hotkey == 0 ) {
            // Deployables require a hotkey def and we only show one non-deployable; use 'c' for it.
            hotkey = 'c';
        }

        std::string text_translated = enable_aid ?
                                      aid->down.menu_text.translated() : aid->down.menu_cant.translated();
        cmenu.addentry( climb_affordance_menu_encode( aid->id ), enable_aid, hotkey,
                        string_format( text_translated, target_disp_name ) );
    }
}

bool game::climb_down_menu_pick( const tripoint_bub_ms &examp, int retval )
{
    climbing_aid_id aid_id = climbing_aid_default;

    if( climb_affordance_menu_decode( retval, aid_id ) ) {
        climb_down_using( examp, aid_id );
        return true;
    } else {
        return false;
    }
}

void game::climb_down( const tripoint_bub_ms &examp )
{
    uilist cmenu;
    cmenu.text = _( "How would you prefer to climb down?" );

    climb_down_menu_gen( examp, cmenu );

    // If there would only be one choice, skip the menu.
    if( cmenu.entries.size() == 1 ) {
        climb_down_menu_pick( examp, cmenu.entries[0].retval );
    } else {
        cmenu.query();
        climb_down_menu_pick( examp, cmenu.ret );
    }
}

void game::climb_down_using( const tripoint_bub_ms &examp, climbing_aid_id aid_id,
                             bool deploy_affordance )
{
    const climbing_aid &aid = aid_id.obj();

    map &here = get_map();
    Character &you = get_player_character();

    // If player is grabbed, trapped, or somehow otherwise movement-impeded, first try to break free
    if( !you.move_effects( false ) ) {
        // move_effects determined we could not move, waste all moves
        you.set_moves( 0 );
        return;
    }

    if( !here.valid_move( you.pos_bub(), examp, false, true ) ) {
        // Can't move horizontally to the ledge
        return;
    }

    // Scan the height of the drop and what's in the way.
    const climbing_aid::fall_scan fall( examp );

    int estimated_climb_cost = you.climbing_cost( tripoint_bub_ms( fall.pos_bottom() ), examp );
    const float fall_mod = you.fall_damage_mod();
    add_msg_debug( debugmode::DF_IEXAMINE, "Climb cost %d", estimated_climb_cost );
    add_msg_debug( debugmode::DF_IEXAMINE, "Fall damage modifier %.2f", fall_mod );

    std::string query;

    // The most common query reads as follows:
    //   It [seems somewhat risky] to climb down like this.
    //   Falling [would hurt].
    //   You [probably won't be able to climb back up].
    //   Climb down the rope?

    // Calculate chance of slipping.  Prints possible causes to log.
    float slip_chance = slip_down_chance( climb_maneuver::down, aid_id, true );

    // Roughly estimate damage if we should fall.
    int damage_estimate = 10 * fall.height;
    if( damage_estimate <= 30 ) {
        damage_estimate *= fall_mod;
    } else {
        damage_estimate *= std::pow( fall_mod, 30.f / damage_estimate );
    }

    // Rough messaging about safety.  "seems safe" can leave a 1-2% chance unlike "perfectly safe".
    bool levitating = u.has_flag( json_flag_LEVITATION );
    bool seems_perfectly_safe = slip_chance < -5 && aid.down.max_height >= fall.height;
    if( !levitating ) {
        if( seems_perfectly_safe ) {
            query = _( "It <color_green>seems perfectly safe</color> to climb down like this." );
        } else if( slip_chance < 3 ) {
            query = _( "It <color_green>seems safe</color> to climb down like this." );
        } else if( slip_chance < 8 ) {
            query = _( "It <color_yellow>seems a bit tricky</color> to climb down like this." );
        } else if( slip_chance < 20 ) {
            query = _( "It <color_yellow>seems somewhat risky</color> to climb down like this." );
        } else if( slip_chance < 50 ) {
            query = _( "It <color_red>seems very risky</color> to climb down like this." );
        } else if( slip_chance < 80 ) {
            query = _( "It <color_pink>looks like you'll slip</color> if you climb down like this." );
        } else {
            query = _( "It <color_pink>doesn't seem possible to climb down safely</color>." );
        }
    }

    if( !seems_perfectly_safe && !levitating ) {
        std::string hint_fall_damage;
        if( damage_estimate >= 100 ) {
            hint_fall_damage = _( "Falling <color_pink>would kill you</color>." );
        } else if( damage_estimate >= 60 ) {
            hint_fall_damage = _( "Falling <color_pink>could cripple or kill you</color>." );
        } else if( damage_estimate >= 30 ) {
            hint_fall_damage = _( "Falling <color_pink>would break bones.</color>." );
        } else if( damage_estimate >= 15 ) {
            hint_fall_damage = _( "Falling <color_red>would hurt badly</color>." );
        } else if( damage_estimate >= 5 ) {
            hint_fall_damage = _( "Falling <color_red>would hurt</color>." );
        } else {
            hint_fall_damage = _( "Falling <color_green>wouldn't hurt much</color>." );
        }
        query += "\n";
        query += hint_fall_damage;
    }

    if( fall.height > aid.down.max_height && !levitating ) {
        // Warn the player that they will fall even after a successful climb
        int remaining_height = fall.height - aid.down.max_height;
        query += "\n";
        query += string_format( n_gettext(
                                    "Even if you climb down safely, you will fall <color_yellow>at least %d story</color>.",
                                    "Even if you climb down safely, you will fall <color_red>at least %d stories</color>.",
                                    remaining_height ), remaining_height );
    }

    // Certain climbing aids make it easy to climb back up, usually by making furniture.
    if( aid.down.easy_climb_back_up >= fall.height ) {
        estimated_climb_cost = 50;
    }

    // Note, this easy_climb_back_up can be set by factors other than the climbing aid.
    bool easy_climb_back_up = false;
    std::string hint_climb_back;
    if( !levitating ) {
        if( estimated_climb_cost <= 0 ) {
            hint_climb_back = _( "You <color_red>probably won't be able to climb back up</color>." );
        } else if( estimated_climb_cost < 200 ) {
            hint_climb_back = _( "You <color_green>should be easily able to climb back up</color>." );
            easy_climb_back_up = true;
        } else {
            hint_climb_back = _( "You <color_yellow>may have problems trying to climb back up</color>." );
        }
        query += "\n";
        query += hint_climb_back;
    }

    if( here.dangerous_field_at( fall.pos_bottom() ) ) {
        for( const std::pair<const field_type_id, field_entry> &danger_field : here.field_at(
                 fall.pos_bottom() ) ) {
            if( danger_field.first->is_dangerous() ) {
                query += "\n";
                query += string_format(
                             _( "There appears to be a dangerous <color_red>%s</color> at your destination." ),
                             danger_field.second.name() );
            }
        }
    }

    std::string query_prompt = _( "Climb down?" );
    if( !aid.down.confirm_text.empty() ) {
        query_prompt = aid.down.confirm_text.translated();
    }
    query += "\n\n";
    query += query_prompt;

    add_msg_debug( debugmode::DF_GAME, "Generated climb_down prompt for the player." );
    add_msg_debug( debugmode::DF_GAME, "Climbing aid: %s / deploy furniture %d", std::string( aid_id ),
                   int( deploy_affordance ) );
    add_msg_debug( debugmode::DF_GAME, "Slip chance %d / est damage %d", slip_chance, damage_estimate );
    add_msg_debug( debugmode::DF_GAME, "We can descend %d / total height %d", aid.down.max_height,
                   fall.height );

    if( !levitating && ( !seems_perfectly_safe || !easy_climb_back_up ) ) {

        // This is used to mention object names.  TODO make this more flexible.
        std::string target_disp_name = here.disp_name( fall.pos_furniture_or_floor() );

        // Show the risk prompt.
        if( !query_yn( query.c_str(), target_disp_name ) ) {
            return;
        }
    }

    you.set_activity_level( ACTIVE_EXERCISE );
    float weary_mult = 1.0f / you.exertion_adjusted_move_multiplier( ACTIVE_EXERCISE );

    you.mod_moves( -to_moves<int>( 1_seconds + 1_seconds * fall_mod ) * weary_mult );
    you.setpos( here, examp, false );

    // Pre-descent message.
    if( !aid.down.msg_before.empty() ) {
        you.add_msg_if_player( aid.down.msg_before.translated() );
    }

    // Descent: perform one slip check per level
    tripoint_bub_ms descent_pos = examp;
    for( int i = 0; i < fall.height && i < aid.down.max_height; ++i ) {
        if( g->slip_down( climb_maneuver::down, aid_id, false ) ) {
            // The player has slipped and probably fallen.
            return;
        } else {
            descent_pos.z()--;
            if( aid.down.deploys_furniture() ) {
                here.furn_set( descent_pos, aid.down.deploy_furn );
            }
        }
    }

    int descended_levels = examp.z() - descent_pos.z();
    add_msg_debug( debugmode::DF_IEXAMINE, "Safe movement down %d Z-levels", descended_levels );

    // Post-descent logic...

    // Use up items after successful climb
    if( aid.base_condition.cat == climbing_aid::category::item && aid.base_condition.uses_item > 0 ) {
        for( item &used_item : you.use_amount( itype_id( aid.base_condition.flag ),
                                               aid.base_condition.uses_item ) ) {
            used_item.spill_contents( you );
        }
    }

    // Pre-descent message.
    if( !aid.down.msg_after.empty() ) {
        you.add_msg_if_player( aid.down.msg_after.translated() );
    }

    // You ride the ride, you pay the tithe.
    if( aid.down.cost.damage > 0 ) {
        you.apply_damage( nullptr, bodypart_id( "torso" ), aid.down.cost.damage );
    }
    if( aid.down.cost.pain > 0 ) {
        you.mod_pain( aid.down.cost.pain );
    }
    if( aid.down.cost.kcal > 0 ) {
        you.mod_stored_kcal( -aid.down.cost.kcal );
    }
    if( aid.down.cost.thirst > 0 ) {
        you.mod_thirst( aid.down.cost.thirst );
    }

    // vertical_move with force=true triggers traps at the end of the move.
    g->vertical_move( -descended_levels, true );

    if( here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, you.pos_bub() ) ) {
        if( you.has_flag( json_flag_WATERWALKING ) ) {
            you.add_msg_if_player( _( "You climb down and stand on the water's surface." ) );
        } else {
            you.set_underwater( true );
            g->water_affect_items( you );
            you.add_msg_if_player( _( "You climb down and dive underwater." ) );
        }
    }
}
