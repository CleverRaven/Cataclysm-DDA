#include "debug_monitor_targets.h"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "coordinates.h"
#include "debug_console_snap.h"
#include "event.h"
#include "point.h"
#include "type_id.h"

namespace debug_menu
{

// Tolerates spaces around commas; nullopt if fewer than two integers
// could be tokenized. Three forms accepted: "x,y", "x,y,z", trailing comma.
static std::optional<tripoint_rel_ms> parse_rel_coord( std::string_view s )
{
    int v[3] = { 0, 0, 0 };
    int n = 0;
    size_t pos = 0;
    while( n < 3 && pos <= s.size() ) {
        const size_t comma = s.find( ',', pos );
        const std::string_view tok = s.substr( pos, comma == std::string_view::npos
                                               ? std::string_view::npos : comma - pos );
        try {
            v[n++] = std::stoi( std::string( tok ) );
        } catch( ... ) {
            return std::nullopt;
        }
        if( comma == std::string_view::npos ) {
            break;
        }
        pos = comma + 1;
    }
    if( n < 2 ) {
        return std::nullopt;
    }
    return tripoint_rel_ms( v[0], v[1], v[2] );
}

static bool validate_rel_coord( std::string_view s )
{
    return parse_rel_coord( s ).has_value();
}

static bool validate_event_type_name( std::string_view s )
{
    for( int i = 0; i < static_cast<int>( event_type::num_event_types ); ++i ) {
        if( io::enum_to_string( static_cast<event_type>( i ) ) == s ) {
            return true;
        }
    }
    return false;
}

const std::vector<monitor_target_entry> &all_monitor_targets()
{
    static const std::vector<monitor_target_entry> table = {
        {
            "Avatar: needs (hp/hunger/thirst/pain)", "", nullptr,
            []( std::string_view )
            {
                return snap::avatar_vitals();
            }
        },
        {
            "Avatar: HP per body part", "", nullptr,
            []( std::string_view )
            {
                return snap::avatar_hp_per_bp();
            }
        },
        {
            "Avatar: stamina/focus/mana/power", "", nullptr,
            []( std::string_view )
            {
                return snap::avatar_stamina_focus_mana_power();
            }
        },
        {
            "Avatar: morale", "", nullptr,
            []( std::string_view )
            {
                return snap::avatar_morale();
            }
        },
        {
            "Avatar: weight/volume carried", "", nullptr,
            []( std::string_view )
            {
                return snap::avatar_weight_volume();
            }
        },
        {
            "Avatar: wielded item", "", nullptr,
            []( std::string_view )
            {
                return snap::avatar_wielded();
            }
        },
        {
            "Avatar: activity", "", nullptr,
            []( std::string_view )
            {
                return snap::avatar_activity();
            }
        },
        {
            "Avatar: position", "", nullptr,
            []( std::string_view )
            {
                return snap::avatar_pos();
            }
        },
        {
            "Avatar: effect intensity", "effect id (e.g. drunk)", nullptr,
            []( std::string_view input )
            {
                return snap::effect_intensity( efftype_id( input ) );
            }
        },
        {
            "Avatar: bionic active + power", "bionic id (e.g. bio_power_storage)", nullptr,
            []( std::string_view input )
            {
                return snap::bionic_state( bionic_id( input ) );
            }
        },
        {
            "Skill level", "skill id (e.g. melee)", nullptr,
            []( std::string_view input )
            {
                return snap::skill( skill_id( input ) );
            }
        },
        {
            "Proficiency progress", "proficiency id", nullptr,
            []( std::string_view input )
            {
                return snap::proficiency_progress( proficiency_id( input ) );
            }
        },
        {
            "Inventory: charges of item", "item id (e.g. battery)", nullptr,
            []( std::string_view input )
            {
                return snap::item_charges( itype_id( input ) );
            }
        },
        {
            "Inventory: count of item", "item id", nullptr,
            []( std::string_view input )
            {
                return snap::item_count( itype_id( input ) );
            }
        },
        {
            "Tile under feet", "", nullptr,
            []( std::string_view )
            {
                return snap::world_tile_under_feet();
            }
        },
        {
            "Items under feet (count + first)", "", nullptr,
            []( std::string_view )
            {
                return snap::world_items_under_feet();
            }
        },
        {
            "Tile at coord (rel to player)", "x,y[,z] (offset from player)", validate_rel_coord,
            []( std::string_view input ) -> std::function<std::string()> {
                if( auto off = parse_rel_coord( input ) )
                {
                    return snap::tile_at_offset( *off );
                }
                return {};
            }
        },
        {
            "Field at coord (rel)", "x,y[,z] (offset)", validate_rel_coord,
            []( std::string_view input ) -> std::function<std::string()> {
                if( auto off = parse_rel_coord( input ) )
                {
                    return snap::field_at_offset( *off );
                }
                return {};
            }
        },
        {
            "Light at coord (rel)", "x,y[,z] (offset)", validate_rel_coord,
            []( std::string_view input ) -> std::function<std::string()> {
                if( auto off = parse_rel_coord( input ) )
                {
                    return snap::light_at_offset( *off );
                }
                return {};
            }
        },
        {
            "Nearest creature (bound by name)", "", nullptr,
            []( std::string_view )
            {
                return snap::nearest_creature();
            }
        },
        {
            "Named creature HP", "creature name", nullptr,
            []( std::string_view input )
            {
                return snap::creature_named( std::string( input ) );
            }
        },
        {
            "NPC distance", "npc name", nullptr,
            []( std::string_view input )
            {
                return snap::npc_distance( std::string( input ) );
            }
        },
        {
            "Creature counts in reality bubble", "", nullptr,
            []( std::string_view )
            {
                return snap::world_creature_counts_in_bubble();
            }
        },
        {
            "Weather (type/temp/wind)", "", nullptr,
            []( std::string_view )
            {
                return snap::world_weather();
            }
        },
        {
            "Calendar (turn/season/day/hour)", "", nullptr,
            []( std::string_view )
            {
                return snap::world_calendar();
            }
        },
        {
            "Current vehicle (vel/battery/pos)", "", nullptr,
            []( std::string_view )
            {
                return snap::vehicle_under_avatar();
            }
        },
        {
            "Faction stats (likes/respects/trusts)", "faction id (e.g. your_followers)", nullptr,
            []( std::string_view input )
            {
                return snap::faction_stats( faction_id( std::string( input ) ) );
            }
        },
        {
            "Active mission count", "", nullptr,
            []( std::string_view )
            {
                return snap::world_active_mission_count();
            }
        },
        {
            "Active EOC queue depth", "", nullptr,
            []( std::string_view )
            {
                return snap::world_active_eoc_depth();
            }
        },
        {
            "Active items registered", "", nullptr,
            []( std::string_view )
            {
                return snap::world_active_items();
            }
        },
        {
            "ImGui FPS", "", nullptr,
            []( std::string_view )
            {
                return snap::imgui_fps();
            }
        },
        {
            "Math expression (math_exp)", "math expression", nullptr,
            []( std::string_view input )
            {
                return snap::math_expr( std::string( input ) );
            }
        },
        {
            "Event count this turn", "event_type name (e.g. character_kills_monster)", validate_event_type_name,
            []( std::string_view input ) -> std::function<std::string()> {
                event_type et = event_type::num_event_types;
                for( int i = 0; i < static_cast<int>( event_type::num_event_types ); ++i )
                {
                    const event_type t = static_cast<event_type>( i );
                    if( io::enum_to_string( t ) == input ) {
                        et = t;
                        break;
                    }
                }
                if( et == event_type::num_event_types )
                {
                    return {};
                }
                return snap::event_count_this_turn(
                           static_cast<int>( et ), std::string( input ) );
            }
        },
        {
            "Global var", "global variable name", nullptr,
            []( std::string_view input )
            {
                return snap::global_var( std::string( input ) );
            }
        },
    };
    return table;
}

} // namespace debug_menu
