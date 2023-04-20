#pragma once
#ifndef CATA_SRC_VEH_UTILS_H
#define CATA_SRC_VEH_UTILS_H

#include <iosfwd>
#include <optional>
#include <vector>

#include "input.h"
#include "type_id.h"

class Character;
class vehicle;
class vpart_info;
struct uilist_entry;
struct vehicle_part;

namespace veh_utils
{
/** Calculates xp for interacting with given part. */
int calc_xp_gain( const vpart_info &vp, const skill_id &sk, const Character &who );
/**
 * Returns a part on a given vehicle that a given character can repair.
 * Prefers the most damaged parts that don't need replacements.
 * If no such part exists, returns a null part.
 */
vehicle_part &most_repairable_part( vehicle &veh, Character &who_arg,
                                    bool only_repairable = false );
/**
 * Repairs a given part on a given vehicle by given character.
 * Awards xp and consumes components.
 */
bool repair_part( vehicle &veh, vehicle_part &pt, Character &who, const std::string &variant );
} // namespace veh_utils

struct veh_menu_item {
    std::string _text;
    std::string _desc;
    std::optional<tripoint> _location = std::nullopt;
    bool _enabled = true;
    bool _check_theft = true;
    bool _check_locked = true;
    bool _keep_menu_open = false;
    std::optional<char> _hotkey_char = std::nullopt;
    std::optional<std::string> _hotkey_action = std::nullopt;
    std::optional<input_event> _hotkey_event = std::nullopt;
    std::function<void()> _on_submit;

    veh_menu_item &text( const std::string &text );
    veh_menu_item &desc( const std::string &desc );
    veh_menu_item &enable( bool enable );
    veh_menu_item &skip_theft_check( bool skip_theft_check = true );
    veh_menu_item &skip_locked_check( bool skip_locked_check = true );
    veh_menu_item &hotkey( char hotkey_char );
    veh_menu_item &hotkey( const std::string &action );
    veh_menu_item &hotkey( const input_event &ev );
    veh_menu_item &hotkey_auto();
    veh_menu_item &on_submit( const std::function<void()> &on_submit );
    veh_menu_item &keep_menu_open( bool keep_menu_open = true );
    veh_menu_item &location( const std::optional<tripoint> &location );
};

class veh_menu
{
    public:
        veh_menu( vehicle &veh, const std::string &title );
        veh_menu( vehicle *veh, const std::string &title );
        veh_menu_item &add( const std::string &txt );
        void reset( bool keep_last_selected = true );
        bool query();

        size_t get_items_size() const;
        std::vector<veh_menu_item> get_items() const;

        int desc_lines_hint = 0;

        // don't allow allocating on heap
        static void *operator new( size_t ) = delete;
        static void *operator new[]( size_t ) = delete;
        static void  operator delete( void * )  = delete;
        static void  operator delete[]( void * )  = delete;

    private:
        std::vector<veh_menu_item> items;
        std::string title;
        vehicle &veh;

        std::vector<uilist_entry> get_uilist_entries() const;
        std::vector<tripoint> get_locations() const;

        int last_selected = 0;
};

#endif // CATA_SRC_VEH_UTILS_H
