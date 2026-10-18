#include "mobile_ui.h"

#include <cmath>
#include <algorithm>
#include <sstream>
#include <vector>

#include "game.h"
#include "character.h"
#include "map.h"
#include "vpart_position.h"
#include "options.h"
#include "translations.h"
#include "input_context.h"
#include "sdl_font.h"
#include "cursesdef.h"
#include "color.h"
#include "type_id.h"

namespace cata_mobile
{

static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_infected( "infected" );

mobile_ui_system &mobile_ui_system::get()
{
    static mobile_ui_system instance;
    return instance;
}

mobile_ui_system::mobile_ui_system()
{
    setup_default_buttons();
}

void mobile_ui_system::init()
{
    if( initialized ) {
        return;
    }
    setup_default_buttons();
    initialized = true;
}

void mobile_ui_system::setup_default_buttons()
{
    fixed_buttons.clear();
    right_context_slots.clear();
    left_context_slots.clear();

    // 1. PPSSPP Style Face Buttons (Right cluster diamond)
    touch_button btn_south; // ✕ - Smart Action / Confirm
    btn_south.id = button_id::FACE_SOUTH;
    btn_south.style = button_visual_style::CIRCLE;
    btn_south.game_action = ACTION_AUTOATTACK;
    btn_south.game_fallback_key = '\t';
    btn_south.menu_fallback_key = '\n';
    btn_south.default_symbol = "X";
    btn_south.description = _( "Ataque / Confirmar" );
    btn_south.border_r = 70; btn_south.border_g = 140; btn_south.border_b = 255;
    fixed_buttons.push_back( btn_south );

    touch_button btn_east; // ○ - Examine / Cancel
    btn_east.id = button_id::FACE_EAST;
    btn_east.style = button_visual_style::CIRCLE;
    btn_east.game_action = ACTION_LOOK;
    btn_east.game_fallback_key = 'x';
    btn_east.menu_fallback_key = KEY_ESCAPE;
    btn_east.default_symbol = "O";
    btn_east.description = _( "Olhar / Cancelar" );
    btn_east.border_r = 255; btn_east.border_g = 70; btn_east.border_b = 70;
    fixed_buttons.push_back( btn_east );

    touch_button btn_west; // □ - Inventory / Drop
    btn_west.id = button_id::FACE_WEST;
    btn_west.style = button_visual_style::CIRCLE;
    btn_west.game_action = ACTION_INVENTORY;
    btn_west.game_fallback_key = 'i';
    btn_west.menu_fallback_key = 'd';
    btn_west.default_symbol = "[]";
    btn_west.description = _( "Mochila / Largar" );
    btn_west.border_r = 240; btn_west.border_g = 90; btn_west.border_b = 240;
    fixed_buttons.push_back( btn_west );

    touch_button btn_north; // △ - Crafting & Build / Details
    btn_north.id = button_id::FACE_NORTH;
    btn_north.style = button_visual_style::CIRCLE;
    btn_north.game_action = ACTION_CRAFT;
    btn_north.game_fallback_key = '&';
    btn_north.menu_fallback_key = '?';
    btn_north.default_symbol = "^";
    btn_north.description = _( "Criação / Detalhes" );
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
    btn_c2.description = _( "Inventário Avançado / Ordenar" );
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
    btn_hub.description = _( "Hub & Configurações" );
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
    screen_width = window_w;
    screen_height = window_h;
    calculate_geometry( window_w, window_h );
}

void mobile_ui_system::calculate_geometry( int w, int h )
{
    // Retrieve custom scale factors with defaults
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

    // 1. Exiled Kingdoms Analog Pad (Bottom Left by default)
    analog_radius = 65.0f * dpad_scale;
    float analog_base_x = 110.0f + static_cast<float>( dpad_ox );
    float analog_base_y = static_cast<float>( h ) - 110.0f + static_cast<float>( dpad_oy );
    if( swap_sides ) {
        analog_base_x = static_cast<float>( w ) - 110.0f - static_cast<float>( dpad_ox );
    }
    analog_center = point( static_cast<int>( analog_base_x ), static_cast<int>( analog_base_y ) );

    // 2. PPSSPP Diamond Face Buttons (Bottom Right by default)
    float face_base_x = static_cast<float>( w ) - 120.0f + static_cast<float>( face_ox );
    float face_base_y = static_cast<float>( h ) - 120.0f + static_cast<float>( face_oy );
    if( swap_sides ) {
        face_base_x = 120.0f - static_cast<float>( face_ox );
    }

    int btn_size = static_cast<int>( 58.0f * face_scale );
    int diamond_radius = static_cast<int>( 55.0f * face_scale );

    for( auto &btn : fixed_buttons ) {
        switch( btn.id ) {
            case button_id::FACE_SOUTH: // ✕
                btn.bounds = { static_cast<int>( face_base_x - btn_size / 2 ),
                               static_cast<int>( face_base_y + diamond_radius - btn_size / 2 ),
                               btn_size, btn_size };
                break;
            case button_id::FACE_EAST: // ○
                btn.bounds = { static_cast<int>( face_base_x + diamond_radius - btn_size / 2 ),
                               static_cast<int>( face_base_y - btn_size / 2 ),
                               btn_size, btn_size };
                break;
            case button_id::FACE_WEST: // □
                btn.bounds = { static_cast<int>( face_base_x - diamond_radius - btn_size / 2 ),
                               static_cast<int>( face_base_y - btn_size / 2 ),
                               btn_size, btn_size };
                break;
            case button_id::FACE_NORTH: // △
                btn.bounds = { static_cast<int>( face_base_x - btn_size / 2 ),
                               static_cast<int>( face_base_y - diamond_radius - btn_size / 2 ),
                               btn_size, btn_size };
                break;

            // Shoulder Triggers
            case button_id::SHOULDER_L1: {
                int sw = static_cast<int>( 75.0f * shoulder_scale );
                int sh = static_cast<int>( 36.0f * shoulder_scale );
                btn.bounds = { 20, 75, sw, sh };
                break;
            }
            case button_id::SHOULDER_L2: {
                int sw = static_cast<int>( 75.0f * shoulder_scale );
                int sh = static_cast<int>( 36.0f * shoulder_scale );
                btn.bounds = { 105, 75, sw, sh };
                break;
            }
            case button_id::SHOULDER_R1: {
                int sw = static_cast<int>( 75.0f * shoulder_scale );
                int sh = static_cast<int>( 36.0f * shoulder_scale );
                btn.bounds = { w - 190, 75, sw, sh };
                break;
            }
            case button_id::SHOULDER_R2: {
                int sw = static_cast<int>( 75.0f * shoulder_scale );
                int sh = static_cast<int>( 36.0f * shoulder_scale );
                btn.bounds = { w - 105, 75, sw, sh };
                break;
            }

            // Bottom Center Buttons
            case button_id::CENTER_WAIT_1T: {
                int cw = static_cast<int>( 60.0f * center_scale );
                int ch = static_cast<int>( 32.0f * center_scale );
                btn.bounds = { w / 2 - cw - 8, h - ch - 12, cw, ch };
                break;
            }
            case button_id::CENTER_ADV_INV: {
                int cw = static_cast<int>( 60.0f * center_scale );
                int ch = static_cast<int>( 32.0f * center_scale );
                btn.bounds = { w / 2 + 8, h - ch - 12, cw, ch };
                break;
            }

            // Top HUD Buttons
            case button_id::TOP_CHAR_SHEET: // Red Square
                btn.bounds = { 18, 12, 54, 50 };
                break;
            case button_id::TOP_MAP_BUTTON:
                btn.bounds = { w - 110, 12, 44, 44 };
                break;
            case button_id::TOP_HUB_MENU: // Brown Square
                btn.bounds = { w - 58, 12, 44, 44 };
                break;

            default:
                break;
        }
    }

    // 3. Dynamic Context Slots (Right Staircase: from bottom-left of face diamond upwards)
    int slot_w = static_cast<int>( 90.0f * context_scale );
    int slot_h = static_cast<int>( 36.0f * context_scale );

    // Slot 1 (Primary - closest to right thumb):
    right_context_slots[0].bounds = { static_cast<int>( face_base_x - diamond_radius - slot_w - 15 ),
                                      static_cast<int>( face_base_y + 15 ),
                                      slot_w, slot_h };
    // Slot 2 (Middle of staircase):
    right_context_slots[1].bounds = { static_cast<int>( face_base_x - diamond_radius - slot_w + 10 ),
                                      static_cast<int>( face_base_y - slot_h - 15 ),
                                      slot_w, slot_h };
    // Slot 3 (Top of staircase):
    right_context_slots[2].bounds = { static_cast<int>( face_base_x - diamond_radius - slot_w + 35 ),
                                      static_cast<int>( face_base_y - slot_h * 2 - 40 ),
                                      slot_w, slot_h };

    // 4. Dynamic Context Slots (Left Staircase: above/right of analog)
    left_context_slots[0].bounds = { static_cast<int>( analog_base_x + analog_radius + 15 ),
                                     static_cast<int>( analog_base_y - slot_h / 2 ),
                                     slot_w, slot_h };
    left_context_slots[1].bounds = { static_cast<int>( analog_base_x + analog_radius - 5 ),
                                     static_cast<int>( analog_base_y - slot_h * 2 ),
                                     slot_w, slot_h };
    left_context_slots[2].bounds = { static_cast<int>( analog_base_x + analog_radius - 25 ),
                                     static_cast<int>( analog_base_y - slot_h * 3 - 15 ),
                                     slot_w, slot_h };
}

void mobile_ui_system::update_context_actions()
{
    active_right_actions.clear();
    active_left_actions.clear();

    if( !g || is_menu_active() ) {
        for( auto &slot : right_context_slots ) { slot.is_visible = false; }
        for( auto &slot : left_context_slots ) { slot.is_visible = false; }
        return;
    }

    Character &u = get_player_character();
    map &m = get_map();
    const tripoint_bub_ms p_bub = u.pos_bub();

    // --- RIGHT SLOTS (Combat, Weapons & Immediate Health) ---
    // 1. Reload Weapon
    item_location wielded = u.get_wielded_item();
    if( wielded && wielded->is_gun() ) {
        context_action act_reload;
        act_reload.act_id = ACTION_RELOAD_WEAPON;
        act_reload.fallback_key = 'r';
        act_reload.default_symbol = "RELOAD";
        act_reload.label = _( "Recarregar" );
        active_right_actions.push_back( act_reload );
    }

    // 2. Health / Bandage (Check if hurt or bleeding)
    if( u.has_effect( effect_bleed ) || u.has_effect( effect_bite ) || u.has_effect( effect_infected ) ) {
        context_action act_medical;
        act_medical.act_id = ACTION_MEDICAL;
        act_medical.fallback_key = 'N';
        act_medical.default_symbol = "HEAL";
        act_medical.label = _( "Curativo" );
        active_right_actions.push_back( act_medical );
    }

    // 3. Butcher / Smash Corpse on own tile or adjacent
    bool found_corpse = false;
    for( int dx = -1; dx <= 1 && !found_corpse; ++dx ) {
        for( int dy = -1; dy <= 1 && !found_corpse; ++dy ) {
            const tripoint_bub_ms check_p( p_bub.x() + dx, p_bub.y() + dy, p_bub.z() );
            if( can_interact_at( ACTION_BUTCHER, m, check_p ) ) {
                context_action act_butcher;
                act_butcher.act_id = ACTION_BUTCHER;
                act_butcher.fallback_key = 'B';
                act_butcher.default_symbol = "BUTCH";
                act_butcher.label = _( "Carnear" );
                active_right_actions.push_back( act_butcher );
                found_corpse = true;
            }
        }
    }

    // --- LEFT SLOTS (Exploration, Doors, Items & Sleeping) ---
    // 1. Nearby Door to Open or Close
    bool found_door = false;
    for( int dx = -1; dx <= 1 && !found_door; ++dx ) {
        for( int dy = -1; dy <= 1 && !found_door; ++dy ) {
            if( dx == 0 && dy == 0 ) continue;
            const tripoint_bub_ms check_p( p_bub.x() + dx, p_bub.y() + dy, p_bub.z() );
            if( can_interact_at( ACTION_CLOSE, m, check_p ) ) {
                context_action act_close;
                act_close.act_id = ACTION_CLOSE;
                act_close.fallback_key = 'c';
                act_close.default_symbol = "CLOSE";
                act_close.label = _( "Fechar" );
                active_left_actions.push_back( act_close );
                found_door = true;
            } else if( can_interact_at( ACTION_OPEN, m, check_p ) ) {
                context_action act_open;
                act_open.act_id = ACTION_OPEN;
                act_open.fallback_key = 'o';
                act_open.default_symbol = "OPEN";
                act_open.label = _( "Abrir" );
                active_left_actions.push_back( act_open );
                found_door = true;
            }
        }
    }

    // 2. Items on Floor to Pick up
    if( can_interact_at( ACTION_PICKUP, m, p_bub ) ) {
        context_action act_pickup;
        act_pickup.act_id = ACTION_PICKUP;
        act_pickup.fallback_key = 'g';
        act_pickup.default_symbol = "GET";
        act_pickup.label = _( "Pegar" );
        active_left_actions.push_back( act_pickup );
    }

    // 3. Sleep / Rest (Available if fatigued or on bed/seat)
    if( u.get_sleepiness() >= static_cast<int>( sleepiness_levels::TIRED ) || m.has_flag_ter_or_furn( "BED", p_bub ) ) {
        context_action act_sleep;
        act_sleep.act_id = ACTION_SLEEP;
        act_sleep.fallback_key = '$';
        act_sleep.default_symbol = "SLEEP";
        act_sleep.label = _( "Dormir" );
        active_left_actions.push_back( act_sleep );
    }

    // --- POPULATE SLOTS QUEUE (Sliding from Right to Left) ---
    // Right slots:
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

    // Left slots:
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

    // 1. Frosted Glass Background
    Uint8 bg_a = static_cast<Uint8>( 160 * alpha_norm );
    if( btn.is_pressed ) {
        bg_a = static_cast<Uint8>( 230 * alpha_norm );
    }

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
    SetRenderDrawColor( renderer, btn.bg_r, btn.bg_g, btn.bg_b, bg_a );
    RenderFillRect( renderer, &btn.bounds );

    // 2. Soft Outline Border (or 2px neon outline in high contrast)
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

    // 3. Button Symbol / Emoji / Text
    if( font ) {
        std::string text = get_button_display_text( btn );
        int text_len = utf8_width( text );
        int text_x = btn.bounds.x + ( btn.bounds.w - ( font->width * text_len ) ) / 2;
        int text_y = btn.bounds.y + ( btn.bounds.h - font->height ) / 2;

        font->OutputChar( renderer, text, point( text_x + 1, text_y + 1 ), 0, 0.6f ); // Shadow
        font->OutputChar( renderer, text, point( text_x, text_y ), 15, 1.0f );         // White Text
    }
    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

void mobile_ui_system::draw_exiled_analog( const SDL_Renderer_Ptr &renderer, Font *font )
{
    const int alpha_opt = get_option<int>( "MOBILE_UI_OPACITY" );
    const float alpha_norm = std::clamp( alpha_opt, 10, 100 ) / 100.0f;
    const bool high_contrast = get_option<bool>( "MOBILE_HIGH_CONTRAST" );

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );

    // Outer Yellow Ring
    SDL_Rect ring_rect = {
        static_cast<int>( analog_center.x - analog_radius ),
        static_cast<int>( analog_center.y - analog_radius ),
        static_cast<int>( analog_radius * 2 ),
        static_cast<int>( analog_radius * 2 )
    };

    // Frosted Backing
    SetRenderDrawColor( renderer, 25, 20, 5, static_cast<Uint8>( 140 * alpha_norm ) );
    RenderFillRect( renderer, &ring_rect );

    // Yellow Golden Rim
    Uint8 gold_a = static_cast<Uint8>( high_contrast ? 255 : 220 * alpha_norm );
    SetRenderDrawColor( renderer, 255, 215, 0, gold_a );
    RenderDrawRect( renderer, &ring_rect );

    // Draw 4 Cardinal Arrow Markers
    if( font ) {
        font->OutputChar( renderer, "^", point( analog_center.x - font->width / 2, analog_center.y - analog_radius + 4 ), 11 );
        font->OutputChar( renderer, "v", point( analog_center.x - font->width / 2, analog_center.y + analog_radius - font->height - 4 ), 11 );
        font->OutputChar( renderer, "<", point( analog_center.x - analog_radius + 6, analog_center.y - font->height / 2 ), 11 );
        font->OutputChar( renderer, ">", point( analog_center.x + analog_radius - font->width - 6, analog_center.y - font->height / 2 ), 11 );
    }

    // Inner Knob / Stick Indicator
    float knob_r = analog_radius * 0.35f;
    SDL_Rect knob_rect = {
        static_cast<int>( analog_center.x + analog_knob_x - knob_r ),
        static_cast<int>( analog_center.y + analog_knob_y - knob_r ),
        static_cast<int>( knob_r * 2 ),
        static_cast<int>( knob_r * 2 )
    };
    SetRenderDrawColor( renderer, 255, 235, 70, static_cast<Uint8>( 200 * alpha_norm ) );
    RenderFillRect( renderer, &knob_rect );
    SetRenderDrawColor( renderer, 255, 255, 255, 240 );
    RenderDrawRect( renderer, &knob_rect );

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

void mobile_ui_system::draw_vital_stats_bar( const SDL_Renderer_Ptr &renderer, Font *font )
{
    // Green Outline Bar in Top Left
    SDL_Rect bar_rect = { 80, 16, 210, 42 };

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
    SetRenderDrawColor( renderer, 12, 22, 16, 170 );
    RenderFillRect( renderer, &bar_rect );

    // Bright Green Border
    SetRenderDrawColor( renderer, 46, 204, 113, 230 );
    RenderDrawRect( renderer, &bar_rect );

    if( g && font ) {
        Character &u = get_player_character();
        int hp_pct = 100;
        int max_hp = u.get_hp_max();
        if( max_hp > 0 ) {
            hp_pct = std::clamp( ( u.get_hp() * 100 ) / max_hp, 0, 100 );
        }
        int sta_pct = std::clamp( ( u.get_stamina() * 100 ) / std::max( 1, u.get_stamina_max() ), 0, 100 );

        // Draw Mini HP Fill Bar
        SDL_Rect hp_fill = { bar_rect.x + 4, bar_rect.y + 4, ( bar_rect.w - 8 ) * hp_pct / 100, 14 };
        SetRenderDrawColor( renderer, 231, 76, 60, 200 ); // Red HP Fill
        RenderFillRect( renderer, &hp_fill );

        // Draw Mini Stamina Fill Bar
        SDL_Rect sta_fill = { bar_rect.x + 4, bar_rect.y + 22, ( bar_rect.w - 8 ) * sta_pct / 100, 14 };
        SetRenderDrawColor( renderer, 52, 152, 219, 200 ); // Blue Stamina Fill
        RenderFillRect( renderer, &sta_fill );

        // Text Overlay
        std::string hp_str = "HP " + std::to_string( hp_pct ) + "%";
        std::string sta_str = "STA " + std::to_string( sta_pct ) + "%";
        font->OutputChar( renderer, hp_str, point( bar_rect.x + 8, bar_rect.y + 4 ), 15 );
        font->OutputChar( renderer, sta_str, point( bar_rect.x + 8, bar_rect.y + 22 ), 15 );
    }

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

void mobile_ui_system::draw( const SDL_Renderer_Ptr &renderer, Font *font )
{
    if( !initialized ) {
        init();
    }

    update_context_actions();

    // 1. Draw Top Vital Stats & Bars
    draw_vital_stats_bar( renderer, font );

    // 2. Draw 10 Fixed Action Buttons
    for( const auto &btn : fixed_buttons ) {
        draw_glass_button( renderer, font, btn );
    }

    // 3. Draw Dynamic Context Slots (Right Staircase)
    for( const auto &slot : right_context_slots ) {
        if( slot.is_visible ) {
            draw_glass_button( renderer, font, slot );
        }
    }

    // 4. Draw Dynamic Context Slots (Left Staircase)
    for( const auto &slot : left_context_slots ) {
        if( slot.is_visible ) {
            draw_glass_button( renderer, font, slot );
        }
    }

    // 5. Draw Exiled Kingdoms Analog D-Pad
    draw_exiled_analog( renderer, font );

    // 6. Draw Settings Drawer if opened (Hub Marrom)
    if( settings_open ) {
        draw_settings_drawer( renderer, font );
    }
}

bool mobile_ui_system::handle_touch_down( float x, float y, int finger_id, uint32_t ticks )
{
    if( settings_open ) {
        // Intercepted by settings drawer
        return true;
    }

    // 1. Check Analog D-Pad touch
    float dx = x - static_cast<float>( analog_center.x );
    float dy = y - static_cast<float>( analog_center.y );
    float dist = std::sqrt( dx * dx + dy * dy );
    if( dist <= analog_radius * 1.3f && !analog_active ) {
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

    // 2. Check Fixed Buttons
    for( auto &btn : fixed_buttons ) {
        if( !btn.is_visible ) continue;
        if( x >= btn.bounds.x && x <= btn.bounds.x + btn.bounds.w &&
            y >= btn.bounds.y && y <= btn.bounds.y + btn.bounds.h ) {
            btn.is_pressed = true;
            btn.active_finger_id = finger_id;
            btn.press_start_time = ticks;
            return true;
        }
    }

    // 3. Check Right Context Slots
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

    // 4. Check Left Context Slots
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

            // Determine directional angle for 8-way movement
            float angle = std::atan2( dy, dx ) * 180.0f / static_cast<float>( M_PI );
            if( angle < 0.0f ) angle += 360.0f;

            if( angle >= 337.5f || angle < 22.5f ) {
                analog_current_direction_key = KEY_RIGHT;
            } else if( angle >= 22.5f && angle < 67.5f ) {
                analog_current_direction_key = JOY_LS_DOWN_RIGHT;
            } else if( angle >= 67.5f && angle < 112.5f ) {
                analog_current_direction_key = KEY_DOWN;
            } else if( angle >= 112.5f && angle < 157.5f ) {
                analog_current_direction_key = JOY_LS_DOWN_LEFT;
            } else if( angle >= 157.5f && angle < 202.5f ) {
                analog_current_direction_key = KEY_LEFT;
            } else if( angle >= 202.5f && angle < 247.5f ) {
                analog_current_direction_key = JOY_LS_UP_LEFT;
            } else if( angle >= 247.5f && angle < 292.5f ) {
                analog_current_direction_key = KEY_UP;
            } else {
                analog_current_direction_key = JOY_LS_UP_RIGHT;
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

    // 1. Release Analog
    if( analog_active && finger_id == analog_finger_id ) {
        analog_active = false;
        analog_finger_id = -1;
        analog_knob_x = 0.0f;
        analog_knob_y = 0.0f;

        // Check if it was a quick central tap to pass 1 turn
        if( ticks - analog_last_repeat < 200 && std::abs( analog_knob_x ) < 15.0f && std::abs( analog_knob_y ) < 15.0f ) {
            out_event = input_event( '.', input_event_t::keyboard_char );
            return true;
        }
        analog_current_direction_key = 0;
        return true;
    }

    const bool in_menu = is_menu_active();

    // 2. Check Fixed Buttons
    for( auto &btn : fixed_buttons ) {
        if( btn.is_pressed && btn.active_finger_id == finger_id ) {
            btn.is_pressed = false;
            btn.active_finger_id = -1;

            if( btn.id == button_id::TOP_HUB_MENU ) {
                toggle_settings_drawer();
                return true;
            }

            int key = in_menu ? btn.menu_fallback_key : btn.game_fallback_key;
            if( key != 0 ) {
                out_event = input_event( key, input_event_t::keyboard_char );
                return true;
            }
        }
    }

    // 3. Check Right Context Slots
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

    // 4. Check Left Context Slots
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

    return false;
}

bool mobile_ui_system::handle_touch_repeat( uint32_t ticks, input_event &out_event )
{
    if( settings_open ) {
        return false;
    }

    if( analog_active && analog_current_direction_key != 0 ) {
        const uint32_t repeat_rate = 140; // milliseconds per movement tick
        if( ticks - analog_last_repeat >= repeat_rate ) {
            analog_last_repeat = ticks;
            input_event_t ev_type = input_event_t::keyboard_char;
            if( analog_current_direction_key == JOY_LS_DOWN_RIGHT ||
                analog_current_direction_key == JOY_LS_DOWN_LEFT ||
                analog_current_direction_key == JOY_LS_UP_LEFT ||
                analog_current_direction_key == JOY_LS_UP_RIGHT ) {
                ev_type = input_event_t::gamepad;
            }
            out_event = input_event( analog_current_direction_key, ev_type );
            return true;
        }
    }
    return false;
}

void mobile_ui_system::toggle_settings_drawer()
{
    settings_open = !settings_open;
}

void mobile_ui_system::draw_settings_drawer( const SDL_Renderer_Ptr &renderer, Font *font )
{
    // Semi-transparent backdrop over whole screen
    SDL_Rect backdrop = { 0, 0, screen_width, screen_height };
    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
    SetRenderDrawColor( renderer, 0, 0, 0, 180 );
    RenderFillRect( renderer, &backdrop );

    // Center Dialog Box
    int box_w = std::min( 600, screen_width - 40 );
    int box_h = std::min( 480, screen_height - 40 );
    SDL_Rect box = { ( screen_width - box_w ) / 2, ( screen_height - box_h ) / 2, box_w, box_h };

    SetRenderDrawColor( renderer, 18, 24, 34, 245 );
    RenderFillRect( renderer, &box );
    SetRenderDrawColor( renderer, 180, 110, 60, 255 ); // Brown Hub Accent Border
    RenderDrawRect( renderer, &box );

    if( font ) {
        std::string title = _( "=== HUB MOBILE & ACESSIBILIDADE ===" );
        font->OutputChar( renderer, title, point( box.x + 20, box.y + 16 ), 11 );

        // Tabs
        std::string tab0 = settings_active_tab == 0 ? _( "[1. LAYOUT & TAMANHO]" ) : _( " 1. Layout & Tamanho " );
        std::string tab1 = settings_active_tab == 1 ? _( "[2. ACESSIBILIDADE]" ) : _( " 2. Acessibilidade " );
        std::string tab2 = settings_active_tab == 2 ? _( "[3. ICONES & EMOJIS]" ) : _( " 3. Icones & Emojis " );

        font->OutputChar( renderer, tab0, point( box.x + 20, box.y + 44 ), settings_active_tab == 0 ? 11 : 15 );
        font->OutputChar( renderer, tab1, point( box.x + 220, box.y + 44 ), settings_active_tab == 1 ? 11 : 15 );
        font->OutputChar( renderer, tab2, point( box.x + 400, box.y + 44 ), settings_active_tab == 2 ? 11 : 15 );

        // Close Button
        std::string close_btn = _( "[ X FECHAR ]" );
        font->OutputChar( renderer, close_btn, point( box.x + box.w - 110, box.y + 16 ), 12 );

        int content_y = box.y + 90;
        if( settings_active_tab == 0 ) {
            font->OutputChar( renderer, _( "Ajustes de Tamanho e Layout:" ), point( box.x + 25, content_y ), 14 );
            font->OutputChar( renderer, _( "Analogico D-Pad:  [ - ]  [ + ]  (Escala dinamica)" ), point( box.x + 25, content_y + 35 ), 15 );
            font->OutputChar( renderer, _( "Botoes de Acao:   [ - ]  [ + ]  (Losango direito)" ), point( box.x + 25, content_y + 70 ), 15 );
            font->OutputChar( renderer, _( "Gatilhos L / R:   [ - ]  [ + ]  (Botoes superiores)" ), point( box.x + 25, content_y + 105 ), 15 );
            font->OutputChar( renderer, _( "Opacidade Geral:  [ - ]  [ + ]  (Vidro fosco/alfa)" ), point( box.x + 25, content_y + 140 ), 15 );
            font->OutputChar( renderer, _( "Inverter Lados:   [ ALTERNAR MODO CANHOTO ]" ), point( box.x + 25, content_y + 175 ), 13 );
        } else if( settings_active_tab == 1 ) {
            font->OutputChar( renderer, _( "Acessibilidade Visual & Visao:" ), point( box.x + 25, content_y ), 14 );
            font->OutputChar( renderer, _( "Tamanho da Fonte Global: [ - ]  [ + ]" ), point( box.x + 25, content_y + 35 ), 15 );
            font->OutputChar( renderer, _( "Brilho e Gama no Jogo:   [ - ]  [ + ]" ), point( box.x + 25, content_y + 70 ), 15 );
            font->OutputChar( renderer, _( "Modo Alto Contraste:     [ ATIVAR / DESATIVAR ]" ), point( box.x + 25, content_y + 105 ), 15 );
            font->OutputChar( renderer, _( "Realce Noturno:          [ NORMAL / REALCADO ]" ), point( box.x + 25, content_y + 140 ), 15 );
        } else {
            font->OutputChar( renderer, _( "Customizacao de Icones e Emojis:" ), point( box.x + 25, content_y ), 14 );
            font->OutputChar( renderer, _( "Toque em qualquer botao para digitar um Emoji personalizado" ), point( box.x + 25, content_y + 35 ), 15 );
            font->OutputChar( renderer, _( "Exemplos: Mochila=[🎒] Ataque=[⚔️] Curativo=[🩹] Mapa=[🗺️]" ), point( box.x + 25, content_y + 70 ), 13 );
            font->OutputChar( renderer, _( "Suporte a Mods: Texturas PNG em data/mods/mobile_ui/icons/" ), point( box.x + 25, content_y + 115 ), 10 );
            font->OutputChar( renderer, _( "[ RESTAURAR LETRAS PADRAO DO TECLADO ]" ), point( box.x + 25, content_y + 160 ), 11 );
        }
    }

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

bool mobile_ui_system::handle_settings_touch( float x, float y, input_event &/*out_event*/ )
{
    int box_w = std::min( 600, screen_width - 40 );
    int box_h = std::min( 480, screen_height - 40 );
    SDL_Rect box = { ( screen_width - box_w ) / 2, ( screen_height - box_h ) / 2, box_w, box_h };

    // Close button hit
    if( x >= box.x + box.w - 120 && x <= box.x + box.w - 10 &&
        y >= box.y + 10 && y <= box.y + 40 ) {
        settings_open = false;
        return true;
    }

    // Tabs hits
    if( y >= box.y + 40 && y <= box.y + 70 ) {
        if( x < box.x + 200 ) {
            settings_active_tab = 0;
        } else if( x < box.x + 380 ) {
            settings_active_tab = 1;
        } else {
            settings_active_tab = 2;
        }
        return true;
    }

    // Touch outside box closes drawer
    if( x < box.x || x > box.x + box.w || y < box.y || y > box.y + box.h ) {
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
