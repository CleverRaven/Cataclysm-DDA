#pragma once
#ifndef CATA_SRC_MOBILE_UI_H
#define CATA_SRC_MOBILE_UI_H

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <functional>

#include "point.h"
#include "sdl_wrappers.h"
#include "input.h"
#include "action.h"

class Font;

namespace cata_mobile
{

enum class button_id {
    NONE = 0,
    // PPSSPP Face Buttons (Right cluster)
    FACE_SOUTH,   // Confirm / Autoattack
    FACE_EAST,    // Cancel / Look
    FACE_WEST,    // Inventory / Drop
    FACE_NORTH,   // Crafting / Info

    // Shoulder Triggers
    SHOULDER_L1,  // Quick Weapon Swap / Prev Tab
    SHOULDER_L2,  // Run/Walk/Crouch Toggle / Search Filter (/)
    SHOULDER_R1,  // Aim & Fire (f) / Next Tab
    SHOULDER_R2,  // Fire Mode / Throw (F/t) / Select All (,)

    // Bottom Center Buttons
    CENTER_WAIT_1T,  // Wait 1 Turn (.) / Decrement count (-)
    CENTER_ADV_INV,  // Advanced Inventory (/) / Sort list (s)

    // Top HUD Elements
    TOP_CHAR_SHEET,  // Red square: Character info (@) & Body Status ({})
    TOP_QUICK_STATS, // Green outline bar: Quick Health & Stamina breakdown
    TOP_MAP_BUTTON,  // Contextual Overmap button (m)
    TOP_HUB_MENU,    // Brown square: Settings, UI customization & Accessibility

    // Dynamic Context Slots (Right Staircase - Combat & Immediate Survival)
    CONTEXT_RIGHT_1,
    CONTEXT_RIGHT_2,
    CONTEXT_RIGHT_3,

    // Dynamic Context Slots (Left Staircase - Secondary / Exploration)
    CONTEXT_LEFT_1,
    CONTEXT_LEFT_2,
    CONTEXT_LEFT_3,

    // Exiled Kingdoms Style Analog D-Pad
    ANALOG_PAD
};

enum class button_visual_style {
    RECTANGLE,
    ROUNDED_RECT,
    CIRCLE,
    OCTAGON,
    PILL
};

struct context_action {
    action_id act_id = ACTION_NULL;
    int fallback_key = 0;
    std::string default_symbol;
    std::string emoji;
    std::string label;
    std::string custom_sprite_path;
};

struct touch_button {
    button_id id = button_id::NONE;
    SDL_Rect bounds { 0, 0, 0, 0 };
    button_visual_style style = button_visual_style::ROUNDED_RECT;

    // Actions & Keys
    action_id game_action = ACTION_NULL;
    int game_fallback_key = 0;
    int menu_fallback_key = 0;

    // Display Content (3 Layers: Key Symbol -> Custom Emoji -> Mod Sprite)
    std::string default_symbol;
    std::string custom_emoji;
    std::string custom_sprite_path;
    std::string description;

    // Runtime state
    bool is_visible = true;
    bool is_pressed = false;
    int active_finger_id = -1;
    uint32_t press_start_time = 0;

    // Color theme
    Uint8 bg_r = 15;
    Uint8 bg_g = 20;
    Uint8 bg_b = 28;
    Uint8 border_r = 240;
    Uint8 border_g = 245;
    Uint8 border_b = 255;
    Uint8 text_r = 255;
    Uint8 text_g = 255;
    Uint8 text_b = 255;
};

struct settings_touch_item {
    std::string opt_name;
    std::string title;
    std::string type; // "float", "int", "bool"
    float min_val = 0.0f;
    float max_val = 1.0f;
    float step = 0.05f;
    SDL_Rect btn_minus { 0, 0, 0, 0 };
    SDL_Rect btn_plus { 0, 0, 0, 0 };
    SDL_Rect btn_toggle { 0, 0, 0, 0 };
};

class mobile_ui_system
{
public:
    static mobile_ui_system &get();

    void init();
    void update_layout( int window_w, int window_h );
    void update_context_actions();
    void draw( const SDL_Renderer_Ptr &renderer, Font *font );

    // Touch Event Handling
    bool handle_touch_down( float x, float y, int finger_id, uint32_t ticks );
    bool handle_touch_motion( float x, float y, int finger_id, uint32_t ticks );
    bool handle_touch_up( float x, float y, int finger_id, uint32_t ticks, input_event &out_event );
    bool handle_touch_repeat( uint32_t ticks, input_event &out_event );

    // Quick Settings Drawer (Hub Marrom)
    bool is_settings_drawer_open() const { return settings_open; }
    void toggle_settings_drawer();
    void draw_settings_drawer( const SDL_Renderer_Ptr &renderer, Font *font );
    bool handle_settings_touch( float x, float y, input_event &out_event );
    void reset_to_defaults();

    // Customization Accessors
    void set_button_emoji( button_id id, const std::string &emoji );
    std::string get_button_display_text( const touch_button &btn ) const;

    // Layout and game state checks
    bool is_gameplay_ready() const;
    bool is_menu_active() const;

private:
    mobile_ui_system();
    ~mobile_ui_system() = default;

    void setup_default_buttons();
    void calculate_geometry( int w, int h );
    void draw_glass_button( const SDL_Renderer_Ptr &renderer, Font *font, const touch_button &btn );
    void draw_exiled_analog( const SDL_Renderer_Ptr &renderer, Font *font );
    void draw_vital_stats_bar( const SDL_Renderer_Ptr &renderer, Font *font );

    // Internal state
    int screen_width = 1280;
    int screen_height = 720;
    bool initialized = false;
    bool settings_open = false;

    // Analog Joystick state
    point analog_center { 120, 580 };
    float analog_radius = 80.0f;
    float analog_knob_x = 0.0f;
    float analog_knob_y = 0.0f;
    bool analog_active = false;
    int analog_finger_id = -1;
    uint32_t analog_last_repeat = 0;
    int analog_current_direction_key = 0;

    // Buttons Collections
    std::vector<touch_button> fixed_buttons;
    std::vector<touch_button> right_context_slots;
    std::vector<touch_button> left_context_slots;

    // Dynamic Actions Queue
    std::vector<context_action> active_right_actions;
    std::vector<context_action> active_left_actions;

    // Settings modal elements
    int settings_active_tab = 0; // 0 = Layout/Sizes, 1 = Accessibility/Vision, 2 = Shortcuts/Reset
    SDL_Rect settings_box { 0, 0, 0, 0 };
    SDL_Rect settings_close_btn { 0, 0, 0, 0 };
    std::array<SDL_Rect, 3> settings_tabs {};
    SDL_Rect settings_reset_btn { 0, 0, 0, 0 };
    std::vector<settings_touch_item> current_tab_items;
};

} // namespace cata_mobile

#endif // CATA_SRC_MOBILE_UI_H