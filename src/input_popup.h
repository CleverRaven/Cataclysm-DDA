#pragma once
#ifndef CATA_SRC_INPUT_POPUP_H
#define CATA_SRC_INPUT_POPUP_H

#include <climits>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cata_imgui.h"
#include "color.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "point.h"
#include "translation.h"

struct callback_input {
    std::string action;
    int key = INT_MIN;
    std::optional<translation> description;

    callback_input( const std::string &action, int key,
                    std::optional<translation> description = std::nullopt ) : action( action ), key( key ),
        description( std::move( description ) ) {};
    explicit callback_input( const std::string &action,
                             std::optional<translation> description = std::nullopt ) : action( action ),
        description( std::move( description ) ) {};
    explicit callback_input( int key,
                             std::optional<translation> description = std::nullopt ) : key( key ),
        description( std::move( description ) ) {};

    bool operator==( const callback_input &rhs ) const {
        return ( !action.empty() && action == rhs.action ) ||
               ( key != INT_MIN && key == rhs.key );
    }
};

class input_popup : public cataimgui::window
{
    public:
        explicit input_popup( int width, const std::string &title = "", const point &pos = point::min,
                              ImGuiWindowFlags flags = ImGuiWindowFlags_None );

        // todo: monofont is a hack for easy support of old descriptions and should be replaced by a more universal solution
        void set_description( const std::string &desc, const nc_color &default_color = c_green,
                              bool monofont = false );
        void set_label( const std::string &label, const nc_color &default_color = c_light_red );
        void set_max_input_length( int length );
        int get_max_input_length() const;
        void add_callback( const callback_input &input, const std::function<bool()> &callback_func );
        bool cancelled() const;

    protected:
        virtual void draw_input_control() = 0;
        void draw_controls() override;
        cataimgui::bounds get_bounds() override;

        bool handle_custom_callbacks( const std::string &action );

        input_context ctxt;
        std::string label;
        nc_color label_default_color = c_light_red;
        int max_input_length = 0;
        // set focus on startup, ability to set focus on other occasions
        bool set_focus = true;
        bool is_cancelled = false;
        std::vector<std::pair<callback_input, std::function<bool()>>> callbacks;

    private:
        std::string description;
        nc_color description_default_color = c_green;

        point pos;
        int width;
        bool description_monofont = false;
};

class string_input_popup_imgui : public input_popup
{
    public:
        explicit string_input_popup_imgui( int width, const std::string &old_input = "",
                                           const std::string &title = "", const point &pos = point::min,
                                           ImGuiWindowFlags flags = ImGuiWindowFlags_None );

        std::string query();
        void set_identifier( const std::string &ident );
        void set_text( const std::string &txt );
        void use_uilist_history( bool use_uilist );
        void update_input_history( ImGuiInputTextCallbackData *data );
    protected:
        void draw_input_control() override;
    private:
        void add_to_history( const std::string &value ) const;
        void show_history();
        std::string text;
        // currently entered input for history handling
        std::string session_input;
        std::string old_input;
        std::string identifier;
        bool is_uilist_history = true;
        uint64_t max_history_size = 100;
        int history_index = 0;
};

// todo: make generic for any numeric type?
template<typename T>
class number_input_popup : public input_popup
{
    public:
        explicit number_input_popup( int width, T old_value = 0, const std::string &title = "",
                                     const point &pos = point::min, ImGuiWindowFlags flags = ImGuiWindowFlags_None );
        T query();
        void set_step_size( std::optional<T> step, std::optional<T> fast_step );
    protected:
        void draw_input_control() override;
    private:
        T value;
        T old_value;
        std::optional<T> step_size = std::nullopt;
        std::optional<T> fast_step_size = std::nullopt;
};
#endif // CATA_SRC_INPUT_POPUP_H
