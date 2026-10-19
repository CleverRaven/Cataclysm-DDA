#include "mobile_ui.h"

#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

#include "game.h"
#include "character.h"
#include "avatar.h"
#include "options.h"
#include "output.h"
#include "translations.h"
#include "input_context.h"
#include "sdl_font.h"
#include "item.h"
#include "item_location.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace cata_mobile
{

mobile_ui_system &mobile_ui_system::get()
{
    static mobile_ui_system instance;
    return instance;
}

mobile_ui_system::mobile_ui_system()
{
    init();
}

void mobile_ui_system::init()
{
    if( initialized ) {
        return;
    }
    setup_default_buttons();
    calculate_geometry( screen_width, screen_height );
    initialized = true;
}

void mobile_ui_system::setup_default_buttons()
{
    fixed_buttons.clear();
    right_context_slots.clear();
    left_context_slots.clear();

    // 1. PPSSPP Style Face Buttons (Right cluster diamond)
    touch_button btn_south; // Confirm / Autoattack
    btn_south.id = button_id::FACE_SOUTH;
    btn_south.style = button_visual_style::CIRCLE;
    btn_south.game_action = ACTION_AUTOATTACK;
    btn_south.game_fallback_key = '\t';
    btn_south.menu_fallback_key = '\n';
    btn_south.default_symbol = "X";
    btn_south.description = _( "Ataque / Confirmar" );
    btn_south.border_r = 70; btn_south.border_g = 140; btn_south.border_b = 255;
    fixed_buttons.push_back( btn_south );

    touch_button btn_east; // Cancel / Look
    btn_east.id = button_id::FACE_EAST;
    btn_east.style = button_visual_style::CIRCLE;
    btn_east.game_action = ACTION_LOOK;
    btn_east.game_fallback_key = 'x';
    btn_east.menu_fallback_key = KEY_ESCAPE;
    btn_east.default_symbol = "O";
    btn_east.description = _( "Olhar / Cancelar" );
    btn_east.border_r = 255; btn_east.border_g = 70; btn_east.border_b = 70;
    fixed_buttons.push_back( btn_east );

    touch_button btn_west; // Inventory / Drop
    btn_west.id = button_id::FACE_WEST;
    btn_west.style = button_visual_style::CIRCLE;
    btn_west.game_action = ACTION_INVENTORY;
    btn_west.game_fallback_key = 'i';
    btn_west.menu_fallback_key = 'd';
    btn_west.default_symbol = "[]";
    btn_west.description = _( "Mochila / Largar" );
    btn_west.border_r = 240; btn_west.border_g = 90; btn_west.border_b = 240;
    fixed_buttons.push_back( btn_west );

    touch_button btn_north; // Crafting & Build / Details
    btn_north.id = button_id::FACE_NORTH;
    btn_north.style = button_visual_style::CIRCLE;
    btn_north.game_action = ACTION_CRAFT;
    btn_north.game_fallback_key = '&';
    btn_north.menu_fallback_key = '?';
    btn_north.default_symbol = "^";
    btn_north.description = _( "Criacao / Detalhes" );
    btn_north.border_r = 90; btn_north.border_g = 240; btn_north.border_b = 130;
    fixed_buttons.push_back( btn_north );

    // 2. Shoulder Triggers
    touch_button btn_l1; // L1: Quick Weapon Swap / Prev Tab
    btn_l1.id = button_id::SHOULDER_L1;
    btn_l1.style = button_visual_style::PILL;
    btn_l1.game_action = ACTION_WIELD;
    btn_l1.game_fallback_key = 'w';
    btn_l1.menu_fallback_key = KEY_BTAB;
    btn_l1.default_symbol = "L1";
    btn_l1.description = _( "Troca de Arma / Aba Ant." );
    fixed_buttons.push_back( btn_l1 );

    touch_button btn_l2; // L2: Run/Walk/Crouch Toggle / Search Filter
    btn_l2.id = button_id::SHOULDER_L2;
    btn_l2.style = button_visual_style::PILL;
    btn_l2.game_action = ACTION_CYCLE_MOVE;
    btn_l2.game_fallback_key = '"';
    btn_l2.menu_fallback_key = '/';
    btn_l2.default_symbol = "L2";
    btn_l2.description = _( "Marcha / Buscar" );
    fixed_buttons.push_back( btn_l2 );

    touch_button btn_r1; // R1: Aim & Fire / Next Tab
    btn_r1.id = button_id::SHOULDER_R1;
    btn_r1.style = button_visual_style::PILL;
    btn_r1.game_action = ACTION_FIRE;
    btn_r1.game_fallback_key = 'f';
    btn_r1.menu_fallback_key = '\t';
    btn_r1.default_symbol = "R1";
    btn_r1.description = _( "Mirar / Prox. Aba" );
    fixed_buttons.push_back( btn_r1 );

    touch_button btn_r2; // R2: Fire Mode / Throw / Select All
    btn_r2.id = button_id::SHOULDER_R2;
    btn_r2.style = button_visual_style::PILL;
    btn_r2.game_action = ACTION_SELECT_FIRE_MODE;
    btn_r2.game_fallback_key = 'F';
    btn_r2.menu_fallback_key = ',';
    btn_r2.default_symbol = "R2";
    btn_r2.description = _( "Modo Tiro / Selecionar Tudo" );
    fixed_buttons.push_back( btn_r2 );

    // 3. Center Bottom Buttons
    touch_button btn_c1; // C1: Wait 1 Turn / Decrement
    btn_c1.id = button_id::CENTER_WAIT_1T;
    btn_c1.style = button_visual_style::ROUNDED_RECT;
    btn_c1.game_action = ACTION_PAUSE;
    btn_c1.game_fallback_key = '.';
    btn_c1.menu_fallback_key = '-';
    btn_c1.default_symbol = "1T";
    btn_c1.description = _( "Passar 1 Turno" );
    fixed_buttons.push_back( btn_c1 );

    touch_button btn_c2; // C2: Advanced Inventory / Sort
    btn_c2.id = button_id::CENTER_ADV_INV;
    btn_c2.style = button_visual_style::ROUNDED_RECT;
    btn_c2.game_action = ACTION_ADVANCEDINV;
    btn_c2.game_fallback_key = '/';
    btn_c2.menu_fallback_key = 's';
    btn_c2.default_symbol = "ADV";
    btn_c2.description = _( "Inventario Avancado / Ordenar" );
    fixed_buttons.push_back( btn_c2 );

    // 4. Top HUD Buttons
    touch_button btn_char; // Red Square: Character Sheet
    btn_char.id = button_id::TOP_CHAR_SHEET;
    btn_char.style = button_visual_style::ROUNDED_RECT;
    btn_char.game_action = ACTION_PL_INFO;
    btn_char.game_fallback_key = '@';
    btn_char.menu_fallback_key = '@';
    btn_char.default_symbol = "BIO";
    btn_char.description = _( "Ficha do Personagem" );
    btn_char.border_r = 230; btn_char.border_g = 50; btn_char.border_b = 50;
    fixed_buttons.push_back( btn_char );

    touch_button btn_map; // Contextual Map
    btn_map.id = button_id::TOP_MAP_BUTTON;
    btn_map.style = button_visual_style::ROUNDED_RECT;
    btn_map.game_action = ACTION_MAP;
    btn_map.game_fallback_key = 'm';
    btn_map.menu_fallback_key = 'm';
    btn_map.default_symbol = "MAP";
    btn_map.description = _( "Mapa Mundi" );
    fixed_buttons.push_back( btn_map );

    touch_button btn_hub; // Brown Square: App Hub & Settings
    btn_hub.id = button_id::TOP_HUB_MENU;
    btn_hub.style = button_visual_style::ROUNDED_RECT;
    btn_hub.game_action = ACTION_NULL;
    btn_hub.game_fallback_key = 0;
    btn_hub.menu_fallback_key = 0;
    btn_hub.default_symbol = "HUB";
    btn_hub.description = _( "Hub & Configuracoes" );
    btn_hub.border_r = 180; btn_hub.border_g = 110; btn_hub.border_b = 60;
    fixed_buttons.push_back( btn_hub );

    // 5. Dynamic Context Slots (Right: 3 slots)
    for( int i = 0; i < 3; ++i ) {
        touch_button slot;
        slot.id = static_cast<button_id>( static_cast<int>( button_id::CONTEXT_RIGHT_1 ) + i );
        slot.style = button_visual_style::ROUNDED_RECT;
        slot.is_visible = false;
        slot.border_r = 255; slot.border_g = 215; slot.border_b = 0; // Gold accent
        right_context_slots.push_back( slot );
    }

    // 6. Dynamic Context Slots (Left: 3 slots)
    for( int i = 0; i < 3; ++i ) {
        touch_button slot;
        slot.id = static_cast<button_id>( static_cast<int>( button_id::CONTEXT_LEFT_1 ) + i );
        slot.style = button_visual_style::ROUNDED_RECT;
        slot.is_visible = false;
        slot.border_r = 135; slot.border_g = 206; slot.border_b = 250; // Cyan accent
        left_context_slots.push_back( slot );
    }
}

bool mobile_ui_system::is_gameplay_ready() const
{
    return g != nullptr && g->is_core_data_loaded() && bool( g->w_terrain ) && !get_player_character().is_dead_state();
}

bool mobile_ui_system::is_menu_active() const
{
    input_context *top_ctx = input_context::input_context_stack.back();
    if( !top_ctx ) {
        return false;
    }
    const std::string &cat = top_ctx->get_category();
    return cat != "DEFAULTMODE";
}

void mobile_ui_system::update_layout( int window_w, int window_h )
{
    if( window_w <= 0 || window_h <= 0 ) {
        return;
    }
    if( window_w != screen_width || window_h != screen_height ) {
        screen_width = window_w;
        screen_height = window_h;
        calculate_geometry( window_w, window_h );
    }
}

void mobile_ui_system::calculate_geometry( int w, int h )
{
    screen_width = w;
    screen_height = h;

    // Escala de densidade adaptada para telas modernas (Poco X7 Pro, FHD+, 2K, etc.)
    // Poco X7 Pro: 2712 x 1220 => min_dim = 1220 => density_scale = ~1.90f
    float min_dim = static_cast<float>( std::min( w, h ) );
    float density_scale = std::clamp( min_dim / 640.0f, 1.25f, 2.8f );

    // Margens de seguranca para evitar bordas de tela, cortes arredondados e barra de gestos do HyperOS
    float safe_margin_x = 36.0f * density_scale;
    float safe_margin_bottom = 32.0f * density_scale;
    float safe_margin_top = 18.0f * density_scale;

    float dpad_scale = get_option<float>( "MOBILE_DPAD_SCALE" );
    if( dpad_scale <= 0.1f ) { dpad_scale = 1.0f; }

    float face_scale = get_option<float>( "MOBILE_FACE_SCALE" );
    if( face_scale <= 0.1f ) { face_scale = 1.0f; }

    float shoulder_scale = get_option<float>( "MOBILE_SHOULDER_SCALE" );
    if( shoulder_scale <= 0.1f ) { shoulder_scale = 1.0f; }

    float context_scale = get_option<float>( "MOBILE_CONTEXT_SCALE" );
    if( context_scale <= 0.1f ) { context_scale = 1.0f; }

    float center_scale = get_option<float>( "MOBILE_CENTER_SCALE" );
    if( center_scale <= 0.1f ) { center_scale = 1.0f; }

    int dpad_ox = get_option<int>( "MOBILE_DPAD_POS_X" );
    int dpad_oy = get_option<int>( "MOBILE_DPAD_POS_Y" );
    int face_ox = get_option<int>( "MOBILE_FACE_POS_X" );
    int face_oy = get_option<int>( "MOBILE_FACE_POS_Y" );

    bool swap_sides = get_option<bool>( "MOBILE_SWAP_SIDES" );

    // 1. Exiled Kingdoms Analog Pad (Inferior Esquerdo por padrao)
    analog_radius = 58.0f * density_scale * dpad_scale;
    float analog_base_x = safe_margin_x + analog_radius + static_cast<float>( dpad_ox );
    float analog_base_y = static_cast<float>( h ) - safe_margin_bottom - analog_radius + static_cast<float>( dpad_oy );
    if( swap_sides ) {
        analog_base_x = static_cast<float>( w ) - safe_margin_x - analog_radius - static_cast<float>( dpad_ox );
    }
    analog_center = point( static_cast<int>( analog_base_x ), static_cast<int>( analog_base_y ) );

    // 2. PPSSPP Diamond Face Buttons (Inferior Direito por padrao)
    int btn_size = static_cast<int>( 52.0f * density_scale * face_scale );
    int diamond_radius = static_cast<int>( 56.0f * density_scale * face_scale );

    float face_base_x = static_cast<float>( w ) - safe_margin_x - diamond_radius - ( btn_size / 2.0f ) + static_cast<float>( face_ox );
    float face_base_y = static_cast<float>( h ) - safe_margin_bottom - diamond_radius - ( btn_size / 2.0f ) + static_cast<float>( face_oy );
    if( swap_sides ) {
        face_base_x = safe_margin_x + diamond_radius + ( btn_size / 2.0f ) - static_cast<float>( face_ox );
    }

    // Top HUD Button sizes
    int hub_btn_size = static_cast<int>( 46.0f * density_scale );
    int top_y = static_cast<int>( safe_margin_top );

    // Shoulder Buttons
    int sw = static_cast<int>( 72.0f * density_scale * shoulder_scale );
    int sh = static_cast<int>( 36.0f * density_scale * shoulder_scale );
    int shoulder_y = top_y + hub_btn_size + static_cast<int>( 10.0f * density_scale );

    // Center Bottom Buttons
    int cw = static_cast<int>( 68.0f * density_scale * center_scale );
    int ch = static_cast<int>( 34.0f * density_scale * center_scale );
    int center_y = h - static_cast<int>( safe_margin_bottom ) - ch;

    for( auto &btn : fixed_buttons ) {
        switch( btn.id ) {
            case button_id::FACE_SOUTH:
                btn.bounds = { static_cast<int>( face_base_x - btn_size / 2 ),
                               static_cast<int>( face_base_y + diamond_radius - btn_size / 2 ),
                               btn_size, btn_size };
                break;
            case button_id::FACE_EAST:
                btn.bounds = { static_cast<int>( face_base_x + diamond_radius - btn_size / 2 ),
                               static_cast<int>( face_base_y - btn_size / 2 ),
                               btn_size, btn_size };
                break;
            case button_id::FACE_WEST:
                btn.bounds = { static_cast<int>( face_base_x - diamond_radius - btn_size / 2 ),
                               static_cast<int>( face_base_y - btn_size / 2 ),
                               btn_size, btn_size };
                break;
            case button_id::FACE_NORTH:
                btn.bounds = { static_cast<int>( face_base_x - btn_size / 2 ),
                               static_cast<int>( face_base_y - diamond_radius - btn_size / 2 ),
                               btn_size, btn_size };
                break;

            // Shoulder Triggers
            case button_id::SHOULDER_L1:
                btn.bounds = { static_cast<int>( safe_margin_x ), shoulder_y, sw, sh };
                break;
            case button_id::SHOULDER_L2:
                btn.bounds = { static_cast<int>( safe_margin_x + sw + 10 ), shoulder_y, sw, sh };
                break;
            case button_id::SHOULDER_R1:
                btn.bounds = { static_cast<int>( w - safe_margin_x - sw * 2 - 10 ), shoulder_y, sw, sh };
                break;
            case button_id::SHOULDER_R2:
                btn.bounds = { static_cast<int>( w - safe_margin_x - sw ), shoulder_y, sw, sh };
                break;

            // Bottom Center Buttons
            case button_id::CENTER_WAIT_1T:
                btn.bounds = { w / 2 - cw - 8, center_y, cw, ch };
                break;
            case button_id::CENTER_ADV_INV:
                btn.bounds = { w / 2 + 8, center_y, cw, ch };
                break;

            // Top HUD Buttons
            case button_id::TOP_CHAR_SHEET:
                btn.bounds = { static_cast<int>( safe_margin_x ), top_y, hub_btn_size, hub_btn_size };
                break;
            case button_id::TOP_MAP_BUTTON:
                btn.bounds = { static_cast<int>( w - safe_margin_x - hub_btn_size * 2 - 12 ), top_y, hub_btn_size, hub_btn_size };
                break;
            case button_id::TOP_HUB_MENU: // Hub Marrom
                btn.bounds = { static_cast<int>( w - safe_margin_x - hub_btn_size ), top_y, hub_btn_size, hub_btn_size };
                break;

            default:
                break;
        }
    }

    // Dynamic Context Slots (Escada ao redor dos polegares)
    int slot_w = static_cast<int>( 88.0f * density_scale * context_scale );
    int slot_h = static_cast<int>( 36.0f * density_scale * context_scale );
    int slot_gap = static_cast<int>( 8.0f * density_scale );

    if( !swap_sides ) {
        // Right Staircase (Thumb direito)
        right_context_slots[0].bounds = { static_cast<int>( face_base_x - diamond_radius - slot_w - slot_gap ),
                                          static_cast<int>( face_base_y + slot_gap ),
                                          slot_w, slot_h };
        right_context_slots[1].bounds = { static_cast<int>( face_base_x - diamond_radius - slot_w ),
                                          static_cast<int>( face_base_y - slot_h - slot_gap ),
                                          slot_w, slot_h };
        right_context_slots[2].bounds = { static_cast<int>( face_base_x - diamond_radius - slot_w + static_cast<int>( 20.0f * density_scale ) ),
                                          static_cast<int>( face_base_y - slot_h * 2 - slot_gap * 2 ),
                                          slot_w, slot_h };

        // Left Staircase (Thumb esquerdo)
        left_context_slots[0].bounds = { static_cast<int>( analog_base_x + analog_radius + slot_gap ),
                                         static_cast<int>( analog_base_y - slot_h / 2 ),
                                         slot_w, slot_h };
        left_context_slots[1].bounds = { static_cast<int>( analog_base_x + analog_radius ),
                                         static_cast<int>( analog_base_y - slot_h * 2 - slot_gap ),
                                         slot_w, slot_h };
        left_context_slots[2].bounds = { static_cast<int>( analog_base_x + analog_radius - static_cast<int>( 15.0f * density_scale ) ),
                                         static_cast<int>( analog_base_y - slot_h * 3 - slot_gap * 2 ),
                                         slot_w, slot_h };
    } else {
        // Canhoto (inverteu analogico e face)
        right_context_slots[0].bounds = { static_cast<int>( face_base_x + diamond_radius + slot_gap ),
                                          static_cast<int>( face_base_y + slot_gap ),
                                          slot_w, slot_h };
        right_context_slots[1].bounds = { static_cast<int>( face_base_x + diamond_radius ),
                                          static_cast<int>( face_base_y - slot_h - slot_gap ),
                                          slot_w, slot_h };
        right_context_slots[2].bounds = { static_cast<int>( face_base_x + diamond_radius - static_cast<int>( 20.0f * density_scale ) ),
                                          static_cast<int>( face_base_y - slot_h * 2 - slot_gap * 2 ),
                                          slot_w, slot_h };

        left_context_slots[0].bounds = { static_cast<int>( analog_base_x - analog_radius - slot_w - slot_gap ),
                                         static_cast<int>( analog_base_y - slot_h / 2 ),
                                         slot_w, slot_h };
        left_context_slots[1].bounds = { static_cast<int>( analog_base_x - analog_radius - slot_w ),
                                         static_cast<int>( analog_base_y - slot_h * 2 - slot_gap ),
                                         slot_w, slot_h };
        left_context_slots[2].bounds = { static_cast<int>( analog_base_x - analog_radius - slot_w + static_cast<int>( 15.0f * density_scale ) ),
                                         static_cast<int>( analog_base_y - slot_h * 3 - slot_gap * 2 ),
                                         slot_w, slot_h };
    }
}

void mobile_ui_system::update_context_actions()
{
    if( !is_gameplay_ready() ) {
        active_right_actions.clear();
        active_left_actions.clear();
        for( auto &slot : right_context_slots ) {
            slot.is_visible = false;
        }
        for( auto &slot : left_context_slots ) {
            slot.is_visible = false;
        }
        return;
    }

    try {
        active_right_actions.clear();
        active_left_actions.clear();

        Character &u = get_player_character();

        // 1. Right Staircase - Combat & Survival Actions
        // R1: Reload current weapon
        item_location wielded = u.get_wielded_item();
        if( wielded && wielded->is_gun() ) {
            context_action act_reload;
            act_reload.act_id = ACTION_RELOAD_WEAPON;
            act_reload.fallback_key = 'r';
            act_reload.default_symbol = "REC";
            act_reload.label = _( "Recarregar" );
            active_right_actions.push_back( act_reload );
        }

        // R2: Bandage / Medical emergency if wounded
        if( u.get_hp() < u.get_hp_max() ) {
            context_action act_heal;
            act_heal.act_id = ACTION_USE;
            act_heal.fallback_key = 'a';
            act_heal.default_symbol = "CURA";
            act_heal.label = _( "Curativo" );
            active_right_actions.push_back( act_heal );
        }

        // R3: Open / Close Doors or Grab
        context_action act_door;
        act_door.act_id = ACTION_OPEN;
        act_door.fallback_key = 'o';
        act_door.default_symbol = "PORT";
        act_door.label = _( "Abrir/Fechar" );
        active_right_actions.push_back( act_door );

        // 2. Left Staircase - Exploration & Utilities
        // L1: Flashlight / Light toggle
        context_action act_light;
        act_light.act_id = ACTION_USE;
        act_light.fallback_key = 'a';
        act_light.default_symbol = "LUZ";
        act_light.label = _( "Lanterna" );
        active_left_actions.push_back( act_light );

        // L2: Sleep / Rest if tired
        if( u.get_sleepiness() >= 200 ) {
            context_action act_sleep;
            act_sleep.act_id = ACTION_SLEEP;
            act_sleep.fallback_key = '$';
            act_sleep.default_symbol = "SONO";
            act_sleep.label = _( "Dormir" );
            active_left_actions.push_back( act_sleep );
        }

        // L3: Vehicle controls or Read
        context_action act_read;
        act_read.act_id = ACTION_READ;
        act_read.fallback_key = 'R';
        act_read.default_symbol = "LER";
        act_read.label = _( "Ler Livro" );
        active_left_actions.push_back( act_read );

        // Bind right actions to slots
        for( size_t i = 0; i < right_context_slots.size(); ++i ) {
            if( i < active_right_actions.size() ) {
                right_context_slots[i].is_visible = true;
                right_context_slots[i].game_action = active_right_actions[i].act_id;
                right_context_slots[i].game_fallback_key = active_right_actions[i].fallback_key;
                right_context_slots[i].default_symbol = active_right_actions[i].default_symbol;
                right_context_slots[i].description = active_right_actions[i].label;
            } else {
                right_context_slots[i].is_visible = false;
            }
        }

        // Bind left actions to slots
        for( size_t i = 0; i < left_context_slots.size(); ++i ) {
            if( i < active_left_actions.size() ) {
                left_context_slots[i].is_visible = true;
                left_context_slots[i].game_action = active_left_actions[i].act_id;
                left_context_slots[i].game_fallback_key = active_left_actions[i].fallback_key;
                left_context_slots[i].default_symbol = active_left_actions[i].default_symbol;
                left_context_slots[i].description = active_left_actions[i].label;
            } else {
                left_context_slots[i].is_visible = false;
            }
        }
    } catch( ... ) {
        // Prevencao de qualquer excecao em contexto
    }
}

std::string mobile_ui_system::get_button_display_text( const touch_button &btn ) const
{
    if( !btn.custom_emoji.empty() ) {
        return btn.custom_emoji;
    }
    return btn.default_symbol;
}

void mobile_ui_system::draw_glass_button( const SDL_Renderer_Ptr &renderer, Font *font,
                                         const touch_button &btn )
{
    if( !btn.is_visible ) {
        return;
    }

    const int alpha_opt = get_option<int>( "MOBILE_UI_OPACITY" );
    const float alpha_norm = std::clamp( alpha_opt, 10, 100 ) / 100.0f;
    const bool high_contrast = get_option<bool>( "MOBILE_HIGH_CONTRAST" );

    // 1. Fundo de vidro fosco
    Uint8 bg_a = static_cast<Uint8>( 160 * alpha_norm );
    if( btn.is_pressed ) {
        bg_a = static_cast<Uint8>( 235 * alpha_norm );
    }

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
    SetRenderDrawColor( renderer, btn.bg_r, btn.bg_g, btn.bg_b, bg_a );
    RenderFillRect( renderer, &btn.bounds );

    // 2. Contorno
    Uint8 b_a = static_cast<Uint8>( 220 * alpha_norm );
    if( high_contrast ) {
        SetRenderDrawColor( renderer, 255, 255, 0, 255 );
        RenderDrawRect( renderer, &btn.bounds );
        SDL_Rect inner = { btn.bounds.x + 1, btn.bounds.y + 1, btn.bounds.w - 2, btn.bounds.h - 2 };
        RenderDrawRect( renderer, &inner );
    } else {
        SetRenderDrawColor( renderer, btn.border_r, btn.border_g, btn.border_b, b_a );
        RenderDrawRect( renderer, &btn.bounds );
    }

    // 3. Simbolo / Texto
    if( font ) {
        std::string text = get_button_display_text( btn );
        int text_len = utf8_width( text );
        int text_x = btn.bounds.x + ( btn.bounds.w - ( font->width * text_len ) ) / 2;
        int text_y = btn.bounds.y + ( btn.bounds.h - font->height ) / 2;

        font->OutputChar( renderer, text, point( text_x + 1, text_y + 1 ), 0, 0.6f );
        font->OutputChar( renderer, text, point( text_x, text_y ), 15, 1.0f );
    }
    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

void mobile_ui_system::draw_exiled_analog( const SDL_Renderer_Ptr &renderer, Font *font )
{
    const int alpha_opt = get_option<int>( "MOBILE_UI_OPACITY" );
    const float alpha_norm = std::clamp( alpha_opt, 10, 100 ) / 100.0f;
    const bool high_contrast = get_option<bool>( "MOBILE_HIGH_CONTRAST" );

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );

    // Outer Circle Ring (Analog Base)
    SDL_Rect outer_box = {
        static_cast<int>( analog_center.x - analog_radius ),
        static_cast<int>( analog_center.y - analog_radius ),
        static_cast<int>( analog_radius * 2 ),
        static_cast<int>( analog_radius * 2 )
    };

    Uint8 base_a = static_cast<Uint8>( 75 * alpha_norm );
    SetRenderDrawColor( renderer, 25, 30, 40, base_a );
    RenderFillRect( renderer, &outer_box );

    if( high_contrast ) {
        SetRenderDrawColor( renderer, 255, 235, 59, 255 );
        RenderDrawRect( renderer, &outer_box );
    } else {
        SetRenderDrawColor( renderer, 255, 193, 7, static_cast<Uint8>( 160 * alpha_norm ) ); // Ambar fosco
        RenderDrawRect( renderer, &outer_box );
    }

    // Inner Floating Stick Knob (Puck)
    float knob_x = analog_center.x + analog_knob_x;
    float knob_y = analog_center.y + analog_knob_y;
    float knob_radius = analog_radius * 0.38f;

    SDL_Rect knob_box = {
        static_cast<int>( knob_x - knob_radius ),
        static_cast<int>( knob_y - knob_radius ),
        static_cast<int>( knob_radius * 2 ),
        static_cast<int>( knob_radius * 2 )
    };

    Uint8 knob_a = static_cast<Uint8>( ( analog_active ? 220 : 150 ) * alpha_norm );
    SetRenderDrawColor( renderer, 255, 193, 7, knob_a );
    RenderFillRect( renderer, &knob_box );
    SetRenderDrawColor( renderer, 255, 255, 255, 240 );
    RenderDrawRect( renderer, &knob_box );

    if( font && !analog_active ) {
        std::string center_hint = ".";
        int tx = analog_center.x - font->width / 2;
        int ty = analog_center.y - font->height / 2;
        font->OutputChar( renderer, center_hint, point( tx, ty ), 15 );
    }

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

void mobile_ui_system::draw_vital_stats_bar( const SDL_Renderer_Ptr &renderer, Font *font )
{
    if( !is_gameplay_ready() || !font ) {
        return;
    }

    float min_dim = static_cast<float>( std::min( screen_width, screen_height ) );
    float density_scale = std::clamp( min_dim / 640.0f, 1.25f, 2.8f );
    float safe_margin_x = 36.0f * density_scale;
    float safe_margin_top = 18.0f * density_scale;
    int hub_btn_size = static_cast<int>( 46.0f * density_scale );

    // Posicionada logo ao lado do botao de BIO
    int bar_x = static_cast<int>( safe_margin_x ) + hub_btn_size + 10;
    int bar_y = static_cast<int>( safe_margin_top );
    int bar_w = static_cast<int>( 200.0f * density_scale );
    int bar_h = hub_btn_size;

    SDL_Rect bar_rect = { bar_x, bar_y, bar_w, bar_h };

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
    SetRenderDrawColor( renderer, 12, 22, 16, 170 );
    RenderFillRect( renderer, &bar_rect );

    SetRenderDrawColor( renderer, 46, 204, 113, 230 );
    RenderDrawRect( renderer, &bar_rect );

    try {
        Character &u = get_player_character();
        int hp_pct = 100;
        int max_hp = u.get_hp_max();
        if( max_hp > 0 ) {
            hp_pct = std::clamp( ( u.get_hp() * 100 ) / max_hp, 0, 100 );
        }
        int sta_pct = std::clamp( ( u.get_stamina() * 100 ) / std::max( 1, u.get_stamina_max() ), 0, 100 );

        int half_h = ( bar_rect.h - 8 ) / 2;

        // Mini HP Fill Bar
        SDL_Rect hp_fill = { bar_rect.x + 4, bar_rect.y + 3, ( bar_rect.w - 8 ) * hp_pct / 100, half_h };
        SetRenderDrawColor( renderer, 231, 76, 60, 200 );
        RenderFillRect( renderer, &hp_fill );

        // Mini Stamina Fill Bar
        SDL_Rect sta_fill = { bar_rect.x + 4, bar_rect.y + 4 + half_h, ( bar_rect.w - 8 ) * sta_pct / 100, half_h };
        SetRenderDrawColor( renderer, 52, 152, 219, 200 );
        RenderFillRect( renderer, &sta_fill );

        // Text Overlay
        std::string hp_str = "HP " + std::to_string( hp_pct ) + "%";
        std::string sta_str = "STA " + std::to_string( sta_pct ) + "%";
        font->OutputChar( renderer, hp_str, point( bar_rect.x + 8, bar_rect.y + 3 ), 15 );
        font->OutputChar( renderer, sta_str, point( bar_rect.x + 8, bar_rect.y + 4 + half_h ), 15 );
    } catch( ... ) {
        // Safe guard
    }

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

void mobile_ui_system::draw( const SDL_Renderer_Ptr &renderer, Font *font )
{
    if( !initialized ) {
        init();
    }

    const bool gameplay_ready = is_gameplay_ready();

    if( gameplay_ready ) {
        update_context_actions();

        // 1. Barra de status do personagem: aparece apenas em sessao de jogo ativo
        draw_vital_stats_bar( renderer, font );
    }

    // 2. Botoes fixos (Face, Gatilhos, Centrais, Top HUD):
    // Permanecem visiveis nos menus para navegacao padrÃ£o (Enter, Esc, C1, C2, etc.),
    // EXCETO o botao de personagem (TOP_CHAR_SHEET / BIO), que so aparece em jogo ativo!
    for( const auto &btn : fixed_buttons ) {
        if( btn.id == button_id::TOP_CHAR_SHEET && !gameplay_ready ) {
            continue;
        }
        draw_glass_button( renderer, font, btn );
    }

    // 3. Dynamic Context Slots (apenas em jogo ativo)
    if( gameplay_ready ) {
        for( const auto &slot : right_context_slots ) {
            if( slot.is_visible ) {
                draw_glass_button( renderer, font, slot );
            }
        }
        for( const auto &slot : left_context_slots ) {
            if( slot.is_visible ) {
                draw_glass_button( renderer, font, slot );
            }
        }
    }

    // 4. Exiled Kingdoms Analog D-Pad: disponivel sempre (inclusive nos menus para navegacao)
    draw_exiled_analog( renderer, font );

    // 5. Draw Settings Drawer se estiver aberto (Hub Marrom)
    if( settings_open ) {
        draw_settings_drawer( renderer, font );
    }
}

bool mobile_ui_system::handle_touch_down( float x, float y, int finger_id, uint32_t ticks )
{
    if( settings_open ) {
        // Interceptado pelo Hub de configuracoes
        return true;
    }

    const bool gameplay_ready = is_gameplay_ready();

    // 1. Check Analog D-Pad touch (funciona sempre, inclusive para navegar nos menus)
    float dx = x - static_cast<float>( analog_center.x );
    float dy = y - static_cast<float>( analog_center.y );
    float dist = std::sqrt( dx * dx + dy * dy );
    if( dist <= analog_radius * 1.35f && !analog_active ) {
        analog_active = true;
        analog_finger_id = finger_id;
        analog_last_repeat = ticks;
        if( dist > 0.0f ) {
            float norm = std::min( 1.0f, dist / analog_radius );
            analog_knob_x = ( dx / dist ) * ( analog_radius * norm );
            analog_knob_y = ( dy / dist ) * ( analog_radius * norm );
        }
        return true;
    }

    // 2. Check Fixed Buttons (todos, exceto TOP_CHAR_SHEET se fora de jogo)
    for( auto &btn : fixed_buttons ) {
        if( !btn.is_visible ) continue;
        if( btn.id == button_id::TOP_CHAR_SHEET && !gameplay_ready ) continue;

        if( x >= btn.bounds.x && x <= btn.bounds.x + btn.bounds.w &&
            y >= btn.bounds.y && y <= btn.bounds.y + btn.bounds.h ) {
            btn.is_pressed = true;
            btn.active_finger_id = finger_id;
            btn.press_start_time = ticks;
            return true;
        }
    }

    // 3. Dynamic Context Slots (apenas em jogo ativo)
    if( gameplay_ready ) {
        for( auto &slot : right_context_slots ) {
            if( !slot.is_visible ) continue;
            if( x >= slot.bounds.x && x <= slot.bounds.x + slot.bounds.w &&
                y >= slot.bounds.y && y <= slot.bounds.y + slot.bounds.h ) {
                slot.is_pressed = true;
                slot.active_finger_id = finger_id;
                slot.press_start_time = ticks;
                return true;
            }
        }
        for( auto &slot : left_context_slots ) {
            if( !slot.is_visible ) continue;
            if( x >= slot.bounds.x && x <= slot.bounds.x + slot.bounds.w &&
                y >= slot.bounds.y && y <= slot.bounds.y + slot.bounds.h ) {
                slot.is_pressed = true;
                slot.active_finger_id = finger_id;
                slot.press_start_time = ticks;
                return true;
            }
        }
    }

    return false;
}

bool mobile_ui_system::handle_touch_motion( float x, float y, int finger_id, uint32_t /*ticks*/ )
{
    if( settings_open ) {
        return true;
    }

    if( analog_active && finger_id == analog_finger_id ) {
        float dx = x - static_cast<float>( analog_center.x );
        float dy = y - static_cast<float>( analog_center.y );
        float dist = std::sqrt( dx * dx + dy * dy );
        if( dist > 0.0f ) {
            float clamp_dist = std::min( analog_radius, dist );
            analog_knob_x = ( dx / dist ) * clamp_dist;
            analog_knob_y = ( dy / dist ) * clamp_dist;

            float angle = std::atan2( dy, dx ) * 180.0f / static_cast<float>( M_PI );
            if( angle < 0.0f ) angle += 360.0f;

            // 8-way directional mapping
            if( angle >= 337.5f || angle < 22.5f ) {
                analog_current_direction_key = '6'; // East (l)
            } else if( angle >= 22.5f && angle < 67.5f ) {
                analog_current_direction_key = '3'; // South-East (n)
            } else if( angle >= 67.5f && angle < 112.5f ) {
                analog_current_direction_key = '2'; // South (j)
            } else if( angle >= 112.5f && angle < 157.5f ) {
                analog_current_direction_key = '1'; // South-West (b)
            } else if( angle >= 157.5f && angle < 202.5f ) {
                analog_current_direction_key = '4'; // West (h)
            } else if( angle >= 202.5f && angle < 247.5f ) {
                analog_current_direction_key = '7'; // North-West (y)
            } else if( angle >= 247.5f && angle < 292.5f ) {
                analog_current_direction_key = '8'; // North (k)
            } else {
                analog_current_direction_key = '9'; // North-East (u)
            }
        }
        return true;
    }
    return false;
}

bool mobile_ui_system::handle_touch_up( float x, float y, int finger_id, uint32_t ticks,
                                        input_event &out_event )
{
    if( settings_open ) {
        return handle_settings_touch( x, y, out_event );
    }

    const bool gameplay_ready = is_gameplay_ready();

    // 1. Release Analog
    if( analog_active && finger_id == analog_finger_id ) {
        analog_active = false;
        analog_finger_id = -1;
        analog_knob_x = 0.0f;
        analog_knob_y = 0.0f;

        // Central quick tap passes 1 turn
        if( ticks - analog_last_repeat < 200 && std::abs( analog_knob_x ) < 15.0f && std::abs( analog_knob_y ) < 15.0f ) {
            out_event = input_event( '.', input_event_t::keyboard_char );
            return true;
        }
        analog_current_direction_key = 0;
        return true;
    }

    const bool in_menu = is_menu_active() || !gameplay_ready;

    // 2. Check Fixed Buttons
    for( auto &btn : fixed_buttons ) {
        if( btn.is_pressed && btn.active_finger_id == finger_id ) {
            btn.is_pressed = false;
            btn.active_finger_id = -1;

            if( btn.id == button_id::TOP_HUB_MENU ) {
                toggle_settings_drawer();
                return true;
            }

            if( btn.id == button_id::TOP_CHAR_SHEET && !gameplay_ready ) {
                return false;
            }

            int key = in_menu ? btn.menu_fallback_key : btn.game_fallback_key;
            if( key != 0 ) {
                out_event = input_event( key, input_event_t::keyboard_char );
                return true;
            }
        }
    }

    // 3. Dynamic Context Slots (apenas em jogo ativo)
    if( gameplay_ready ) {
        for( auto &slot : right_context_slots ) {
            if( slot.is_pressed && slot.active_finger_id == finger_id ) {
                slot.is_pressed = false;
                slot.active_finger_id = -1;
                int key = slot.game_fallback_key;
                if( key != 0 ) {
                    out_event = input_event( key, input_event_t::keyboard_char );
                    return true;
                }
            }
        }
        for( auto &slot : left_context_slots ) {
            if( slot.is_pressed && slot.active_finger_id == finger_id ) {
                slot.is_pressed = false;
                slot.active_finger_id = -1;
                int key = slot.game_fallback_key;
                if( key != 0 ) {
                    out_event = input_event( key, input_event_t::keyboard_char );
                    return true;
                }
            }
        }
    }

    return false;
}

bool mobile_ui_system::handle_touch_repeat( uint32_t ticks, input_event &out_event )
{
    if( settings_open ) {
        return false;
    }

    if( analog_active && analog_current_direction_key != 0 ) {
        if( ticks - analog_last_repeat >= 180 ) {
            analog_last_repeat = ticks;
            out_event = input_event( analog_current_direction_key, input_event_t::keyboard_char );
            return true;
        }
    }
    return false;
}

void mobile_ui_system::toggle_settings_drawer()
{
    settings_open = !settings_open;
    if( settings_open ) {
        calculate_geometry( screen_width, screen_height );
    }
}

void mobile_ui_system::reset_to_defaults()
{
    get_options().get_option( "MOBILE_DPAD_SCALE" ).setValue( 1.0f );
    get_options().get_option( "MOBILE_FACE_SCALE" ).setValue( 1.0f );
    get_options().get_option( "MOBILE_SHOULDER_SCALE" ).setValue( 1.0f );
    get_options().get_option( "MOBILE_CONTEXT_SCALE" ).setValue( 1.0f );
    get_options().get_option( "MOBILE_CENTER_SCALE" ).setValue( 1.0f );
    get_options().get_option( "MOBILE_DPAD_POS_X" ).setValue( 0 );
    get_options().get_option( "MOBILE_DPAD_POS_Y" ).setValue( 0 );
    get_options().get_option( "MOBILE_FACE_POS_X" ).setValue( 0 );
    get_options().get_option( "MOBILE_FACE_POS_Y" ).setValue( 0 );
    get_options().get_option( "MOBILE_UI_OPACITY" ).setValue( 45 );
    get_options().get_option( "MOBILE_HIGH_CONTRAST" ).setValue( "false" );
    get_options().get_option( "MOBILE_SWAP_SIDES" ).setValue( "false" );
    get_options().get_option( "MOBILE_GLOBAL_FONT_SCALE" ).setValue( 1.0f );
    get_options().get_option( "MOBILE_GAMMA_BOOST" ).setValue( 1.0f );
    get_options().save();
    calculate_geometry( screen_width, screen_height );
}

void mobile_ui_system::draw_settings_drawer( const SDL_Renderer_Ptr &renderer, Font *font )
{
    // Backdrop escuro semi-transparente
    SDL_Rect backdrop = { 0, 0, screen_width, screen_height };
    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
    SetRenderDrawColor( renderer, 0, 0, 0, 205 );
    RenderFillRect( renderer, &backdrop );

    // Caixa central ampla e ergonomica (92% de largura, ate 1200px)
    int box_w = std::min( static_cast<int>( screen_width * 0.92f ), 1200 );
    int box_h = std::min( static_cast<int>( screen_height * 0.90f ), 850 );
    settings_box = { ( screen_width - box_w ) / 2, ( screen_height - box_h ) / 2, box_w, box_h };

    // Fundo do painel
    SetRenderDrawColor( renderer, 18, 24, 34, 252 );
    RenderFillRect( renderer, &settings_box );

    // Borda ambar / marrom estilizada do Hub
    SetRenderDrawColor( renderer, 195, 120, 65, 255 );
    RenderDrawRect( renderer, &settings_box );
    SDL_Rect inner_box = { settings_box.x + 1, settings_box.y + 1, settings_box.w - 2, settings_box.h - 2 };
    RenderDrawRect( renderer, &inner_box );

    if( !font ) {
        SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
        return;
    }

    // Cabecalho & Titulo
    std::string title = _( "HUB MOBILE & ACESSIBILIDADE - TOQUE NATIVO" );
    font->OutputChar( renderer, title, point( settings_box.x + 22, settings_box.y + 16 ), 11 );

    // Botao FECHAR no topo direito (Grande e facil de tocar com o polegar)
    settings_close_btn = { settings_box.x + settings_box.w - 145, settings_box.y + 10, 130, 42 };
    SetRenderDrawColor( renderer, 180, 45, 45, 240 );
    RenderFillRect( renderer, &settings_close_btn );
    SetRenderDrawColor( renderer, 255, 100, 100, 255 );
    RenderDrawRect( renderer, &settings_close_btn );

    std::string close_text = _( "[ X FECHAR ]" );
    int cl_tx = settings_close_btn.x + ( settings_close_btn.w - ( font->width * utf8_width( close_text ) ) ) / 2;
    int cl_ty = settings_close_btn.y + ( settings_close_btn.h - font->height ) / 2;
    font->OutputChar( renderer, close_text, point( cl_tx, cl_ty ), 15 );

    // 3 Abas Tatil Grandes com espacamento adequado (sem encavalar)
    int tab_gap = 12;
    int total_tabs_w = settings_box.w - 44;
    int tab_w = ( total_tabs_w - ( tab_gap * 2 ) ) / 3;
    int tab_h = 42;
    int tab_y = settings_box.y + 60;

    std::array<std::string, 3> tab_labels = {
        _( "1. TAMANHO & LAYOUT" ),
        _( "2. ACESSIBILIDADE" ),
        _( "3. ATALHOS & RESET" )
    };

    for( int i = 0; i < 3; ++i ) {
        settings_tabs[i] = { settings_box.x + 22 + i * ( tab_w + tab_gap ), tab_y, tab_w, tab_h };
        bool is_active = ( settings_active_tab == i );

        if( is_active ) {
            SetRenderDrawColor( renderer, 41, 128, 185, 240 ); // Azul destaque
        } else {
            SetRenderDrawColor( renderer, 32, 42, 56, 200 ); // Cinza escuro inativo
        }
        RenderFillRect( renderer, &settings_tabs[i] );

        SetRenderDrawColor( renderer, is_active ? 255 : 120, is_active ? 215 : 140, is_active ? 0 : 160, 255 );
        RenderDrawRect( renderer, &settings_tabs[i] );

        int t_len = utf8_width( tab_labels[i] );
        int tx = settings_tabs[i].x + ( settings_tabs[i].w - ( font->width * t_len ) ) / 2;
        int ty = settings_tabs[i].y + ( settings_tabs[i].h - font->height ) / 2;
        font->OutputChar( renderer, tab_labels[i], point( tx, ty ), is_active ? 11 : 15 );
    }

    // Prepara a lista de opcoes da aba ativa
    current_tab_items.clear();

    if( settings_active_tab == 0 ) {
        // Layout e Tamanhos
        current_tab_items.push_back( { "MOBILE_DPAD_SCALE", _( "Tamanho do Analogico D-Pad" ), "float", 0.5f, 2.5f, 0.05f } );
        current_tab_items.push_back( { "MOBILE_FACE_SCALE", _( "Tamanho dos Botoes de Acao (Losango)" ), "float", 0.5f, 2.5f, 0.05f } );
        current_tab_items.push_back( { "MOBILE_SHOULDER_SCALE", _( "Tamanho dos Gatilhos Superiores L / R" ), "float", 0.5f, 2.0f, 0.05f } );
        current_tab_items.push_back( { "MOBILE_CONTEXT_SCALE", _( "Tamanho dos Slots Contextuais" ), "float", 0.5f, 2.0f, 0.05f } );
        current_tab_items.push_back( { "MOBILE_CENTER_SCALE", _( "Tamanho dos Botoes Centrais (1T / ADV)" ), "float", 0.5f, 2.0f, 0.05f } );
        current_tab_items.push_back( { "MOBILE_SWAP_SIDES", _( "Modo Canhoto (Inverter Lados na Tela)" ), "bool", 0.0f, 1.0f, 1.0f } );
    } else if( settings_active_tab == 1 ) {
        // Acessibilidade e Visao
        current_tab_items.push_back( { "MOBILE_UI_OPACITY", _( "Opacidade Geral dos Botoes (Fundo)" ), "int", 10.0f, 100.0f, 5.0f } );
        current_tab_items.push_back( { "MOBILE_GLOBAL_FONT_SCALE", _( "Escala Global de Fontes da Interface" ), "float", 0.8f, 2.5f, 0.05f } );
        current_tab_items.push_back( { "MOBILE_GAMMA_BOOST", _( "Brilho e Gama (Luz do Sol)" ), "float", 0.5f, 2.5f, 0.05f } );
        current_tab_items.push_back( { "MOBILE_HIGH_CONTRAST", _( "Modo Alto Contraste (Contorno Neon Amarelo)" ), "bool", 0.0f, 1.0f, 1.0f } );
    }

    // Renderiza cada item da aba com botoes touch dedicados
    int item_start_y = tab_y + tab_h + 20;
    int item_height = 48;

    for( size_t i = 0; i < current_tab_items.size(); ++i ) {
        auto &item = current_tab_items[i];
        int iy = item_start_y + static_cast<int>( i ) * ( item_height + 10 );

        // Fundo suave para a linha
        SDL_Rect row_rect = { settings_box.x + 22, iy, settings_box.w - 44, item_height };
        SetRenderDrawColor( renderer, 24, 32, 44, 180 );
        RenderFillRect( renderer, &row_rect );
        SetRenderDrawColor( renderer, 50, 65, 85, 200 );
        RenderDrawRect( renderer, &row_rect );

        // Nome da opcao a esquerda
        font->OutputChar( renderer, item.title, point( row_rect.x + 14, iy + ( item_height - font->height ) / 2 ), 15 );

        if( item.type == "float" || item.type == "int" ) {
            // Botao [ - ]
            item.btn_minus = { row_rect.x + row_rect.w - 240, iy + 4, 56, item_height - 8 };
            SetRenderDrawColor( renderer, 45, 55, 75, 240 );
            RenderFillRect( renderer, &item.btn_minus );
            SetRenderDrawColor( renderer, 100, 130, 170, 255 );
            RenderDrawRect( renderer, &item.btn_minus );

            std::string minus_lbl = "-";
            int mx = item.btn_minus.x + ( item.btn_minus.w - font->width ) / 2;
            int my = item.btn_minus.y + ( item.btn_minus.h - font->height ) / 2;
            font->OutputChar( renderer, minus_lbl, point( mx, my ), 11 );

            // Valor formatado no centro
            std::string val_str;
            if( item.type == "float" ) {
                float val = get_option<float>( item.opt_name );
                std::ostringstream ss;
                ss << std::fixed << std::setprecision( 2 ) << val << "x";
                val_str = ss.str();
            } else {
                int val = get_option<int>( item.opt_name );
                val_str = std::to_string( val ) + "%";
            }

            int val_x = row_rect.x + row_rect.w - 176 + ( 110 - ( font->width * utf8_width( val_str ) ) ) / 2;
            int val_y = iy + ( item_height - font->height ) / 2;
            font->OutputChar( renderer, val_str, point( val_x, val_y ), 14 );

            // Botao [ + ]
            item.btn_plus = { row_rect.x + row_rect.w - 62, iy + 4, 56, item_height - 8 };
            SetRenderDrawColor( renderer, 45, 55, 75, 240 );
            RenderFillRect( renderer, &item.btn_plus );
            SetRenderDrawColor( renderer, 100, 130, 170, 255 );
            RenderDrawRect( renderer, &item.btn_plus );

            std::string plus_lbl = "+";
            int px = item.btn_plus.x + ( item.btn_plus.w - font->width ) / 2;
            int py = item.btn_plus.y + ( item.btn_plus.h - font->height ) / 2;
            font->OutputChar( renderer, plus_lbl, point( px, py ), 11 );

        } else if( item.type == "bool" ) {
            // Botao largo de Toggle Switch
            bool is_active = get_option<bool>( item.opt_name );
            item.btn_toggle = { row_rect.x + row_rect.w - 240, iy + 4, 234, item_height - 8 };

            if( is_active ) {
                SetRenderDrawColor( renderer, 39, 174, 96, 240 ); // Verde ativo
            } else {
                SetRenderDrawColor( renderer, 55, 65, 80, 240 ); // Cinza inativo
            }
            RenderFillRect( renderer, &item.btn_toggle );

            SetRenderDrawColor( renderer, is_active ? 46 : 90, is_active ? 204 : 100, is_active ? 113 : 110, 255 );
            RenderDrawRect( renderer, &item.btn_toggle );

            std::string toggle_text = is_active ? _( "[ ATIVADO ]" ) : _( "[ DESLIGADO ]" );
            int tg_x = item.btn_toggle.x + ( item.btn_toggle.w - ( font->width * utf8_width( toggle_text ) ) ) / 2;
            int tg_y = item.btn_toggle.y + ( item.btn_toggle.h - font->height ) / 2;
            font->OutputChar( renderer, toggle_text, point( tg_x, tg_y ), 15 );
        }
    }

    if( settings_active_tab == 2 ) {
        // Aba 3: Instrucoes do Mod e Botao de Restaurar Padroes
        int info_y = tab_y + tab_h + 20;
        font->OutputChar( renderer, _( "Instrucoes de Toque Puro (Touch-Native Android):" ), point( settings_box.x + 30, info_y ), 14 );
        font->OutputChar( renderer, _( "- Nao precisa digitar nada no teclado virtual (Gboard)!" ), point( settings_box.x + 30, info_y + 35 ), 15 );
        font->OutputChar( renderer, _( "- Toque direto nos botoes [+] e [-] para alterar o tamanho dos controles." ), point( settings_box.x + 30, info_y + 65 ), 15 );
        font->OutputChar( renderer, _( "- Todas as alteracoes sao salvas automaticamente no mesmo instante." ), point( settings_box.x + 30, info_y + 95 ), 15 );
        font->OutputChar( renderer, _( "- Em jogo, use o D-Pad para andar e os botoes direitos para combate/inventario." ), point( settings_box.x + 30, info_y + 125 ), 15 );

        // Botao de Reset central
        settings_reset_btn = { settings_box.x + ( settings_box.w - 420 ) / 2, info_y + 190, 420, 52 };
        SetRenderDrawColor( renderer, 192, 57, 43, 230 ); // Vermelho
        RenderFillRect( renderer, &settings_reset_btn );
        SetRenderDrawColor( renderer, 231, 76, 60, 255 );
        RenderDrawRect( renderer, &settings_reset_btn );

        std::string reset_lbl = _( "[ RESTAURAR CONFIGURACOES PADRAO ]" );
        int rx = settings_reset_btn.x + ( settings_reset_btn.w - ( font->width * utf8_width( reset_lbl ) ) ) / 2;
        int ry = settings_reset_btn.y + ( settings_reset_btn.h - font->height ) / 2;
        font->OutputChar( renderer, reset_lbl, point( rx, ry ), 15 );
    }

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

bool mobile_ui_system::handle_settings_touch( float x, float y, input_event &/*out_event*/ )
{
    // 1. Fechar botao
    if( x >= settings_close_btn.x && x <= settings_close_btn.x + settings_close_btn.w &&
        y >= settings_close_btn.y && y <= settings_close_btn.y + settings_close_btn.h ) {
        settings_open = false;
        return true;
    }

    // 2. Abas
    for( int i = 0; i < 3; ++i ) {
        if( x >= settings_tabs[i].x && x <= settings_tabs[i].x + settings_tabs[i].w &&
            y >= settings_tabs[i].y && y <= settings_tabs[i].y + settings_tabs[i].h ) {
            settings_active_tab = i;
            return true;
        }
    }

    // 3. Reset Button na Aba 2
    if( settings_active_tab == 2 ) {
        if( x >= settings_reset_btn.x && x <= settings_reset_btn.x + settings_reset_btn.w &&
            y >= settings_reset_btn.y && y <= settings_reset_btn.y + settings_reset_btn.h ) {
            reset_to_defaults();
            return true;
        }
    }

    // 4. Botoes tatil das opcoes [-], [+] e [TOGGLE]
    for( const auto &item : current_tab_items ) {
        if( item.type == "float" || item.type == "int" ) {
            // Toque no botao [ - ]
            if( x >= item.btn_minus.x && x <= item.btn_minus.x + item.btn_minus.w &&
                y >= item.btn_minus.y && y <= item.btn_minus.y + item.btn_minus.h ) {
                if( item.type == "float" ) {
                    float val = get_option<float>( item.opt_name );
                    val = std::clamp( val - item.step, item.min_val, item.max_val );
                    get_options().get_option( item.opt_name ).setValue( val );
                } else {
                    int val = get_option<int>( item.opt_name );
                    val = std::clamp( val - static_cast<int>( item.step ), static_cast<int>( item.min_val ), static_cast<int>( item.max_val ) );
                    get_options().get_option( item.opt_name ).setValue( val );
                }
                get_options().save();
                calculate_geometry( screen_width, screen_height );
                return true;
            }

            // Toque no botao [ + ]
            if( x >= item.btn_plus.x && x <= item.btn_plus.x + item.btn_plus.w &&
                y >= item.btn_plus.y && y <= item.btn_plus.y + item.btn_plus.h ) {
                if( item.type == "float" ) {
                    float val = get_option<float>( item.opt_name );
                    val = std::clamp( val + item.step, item.min_val, item.max_val );
                    get_options().get_option( item.opt_name ).setValue( val );
                } else {
                    int val = get_option<int>( item.opt_name );
                    val = std::clamp( val + static_cast<int>( item.step ), static_cast<int>( item.min_val ), static_cast<int>( item.max_val ) );
                    get_options().get_option( item.opt_name ).setValue( val );
                }
                get_options().save();
                calculate_geometry( screen_width, screen_height );
                return true;
            }
        } else if( item.type == "bool" ) {
            // Toque no Toggle
            if( x >= item.btn_toggle.x && x <= item.btn_toggle.x + item.btn_toggle.w &&
                y >= item.btn_toggle.y && y <= item.btn_toggle.y + item.btn_toggle.h ) {
                get_options().get_option( item.opt_name ).setNext();
                get_options().save();
                calculate_geometry( screen_width, screen_height );
                return true;
            }
        }
    }

    // 5. Toque fora do painel fecha o Hub
    if( x < settings_box.x || x > settings_box.x + settings_box.w ||
        y < settings_box.y || y > settings_box.y + settings_box.h ) {
        settings_open = false;
        return true;
    }

    return true;
}

void mobile_ui_system::set_button_emoji( button_id id, const std::string &emoji )
{
    for( auto &btn : fixed_buttons ) {
        if( btn.id == id ) {
            btn.custom_emoji = emoji;
            return;
        }
    }
}

} // namespace cata_mobile