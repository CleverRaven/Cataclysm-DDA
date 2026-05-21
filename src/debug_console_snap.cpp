#include "debug_console_snap.h"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <deque>
#include <exception>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <imgui/imgui.h>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "character_id.h"
#include "creature.h"
#include "debug_capture.h"
#include "dialogue.h"
#include "effect.h"
#include "faction.h"
#include "field.h"
#include "game.h"
#include "global_vars.h"
#include "item.h"
#include "item_location.h"
#include "item_wakeup.h"
#include "magic.h"
#include "map.h"
#include "math_parser.h"
#include "math_parser_diag_value.h"
#include "npc.h"
#include "output.h"
#include "pimpl.h"
#include "player_activity.h"
#include "point.h"
#include "proficiency.h"
#include "stomach.h"
#include "string_formatter.h"
#include "talker.h"
#include "timed_event.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"

namespace debug_menu::snap
{

std::function<std::string()> avatar_vitals()
{
    return []() {
        const avatar &a = get_avatar();
        return string_format( "hp=%d hunger=%d thirst=%d sleepiness=%d sleep_dep=%d pain=%d morale=%d",
                              a.get_hp(), a.get_hunger(), a.get_thirst(),
                              a.get_sleepiness(), a.get_sleep_deprivation(),
                              a.get_pain(), a.get_morale_level() );
    };
}

std::function<std::string()> avatar_pos()
{
    return []() {
        const avatar &a = get_avatar();
        const map &m = get_map();
        return string_format( "T%d pos=%s sm=%s",
                              to_turns<int>( calendar::turn - calendar::turn_zero ),
                              a.pos_bub( m ).to_string(),
                              m.get_abs_sub().to_string() );
    };
}

std::function<std::string()> avatar_metabolism()
{
    return []() {
        const avatar &a = get_avatar();
        return string_format( "hunger=%d thirst=%d kcal=%d/%d bmi=%.1f bmr=%d",
                              a.get_hunger(), a.get_thirst(),
                              a.get_stored_kcal(), a.get_healthy_kcal(),
                              a.get_bmi_fat(), a.get_bmr() );
    };
}

std::function<std::string()> avatar_gi()
{
    return []() {
        const avatar &a = get_avatar();
        return string_format( "stomach=%d/%d (%d kcal) guts=%d/%d (%d kcal)",
                              units::to_milliliter( a.stomach.contains() ),
                              units::to_milliliter( a.stomach.capacity( a ) ),
                              a.stomach.get_calories(),
                              units::to_milliliter( a.guts.contains() ),
                              units::to_milliliter( a.guts.capacity( a ) ),
                              a.guts.get_calories() );
    };
}

std::function<std::string()> avatar_stamina_focus_mana_power()
{
    return []() {
        const avatar &a = get_avatar();
        return string_format( "stamina=%d focus=%d mana=%d power=%dkJ/%dkJ",
                              a.get_stamina(), a.get_focus(),
                              a.magic->available_mana(),
                              units::to_kilojoule( a.get_power_level() ),
                              units::to_kilojoule( a.get_max_power_level() ) );
    };
}

std::function<std::string()> avatar_morale()
{
    return []() {
        return string_format( "morale=%d", get_avatar().get_morale_level() );
    };
}

std::function<std::string()> avatar_weight_volume()
{
    return []() {
        const avatar &a = get_avatar();
        return string_format( "weight=%dg/%dg volume=%dml/%dml",
                              units::to_gram( a.weight_carried() ),
                              units::to_gram( a.weight_capacity() ),
                              units::to_milliliter( a.volume_carried() ),
                              units::to_milliliter( a.volume_capacity() ) );
    };
}

std::function<std::string()> avatar_wielded()
{
    return []() {
        const avatar &a = get_avatar();
        const item_location wp = a.get_wielded_item();
        if( !wp ) {
            return std::string( "(none)" );
        }
        return string_format( "%s dmg=%d", remove_color_tags( wp->tname() ),
                              wp->damage() );
    };
}

std::function<std::string()> avatar_activity()
{
    return []() {
        const avatar &a = get_avatar();
        if( !a.activity ) {
            return std::string( "(idle)" );
        }
        return string_format( "%s moves_left=%d",
                              a.activity.id().str(),
                              a.activity.moves_left );
    };
}

std::function<std::string()> avatar_hp_per_bp()
{
    return []() {
        const avatar &a = get_avatar();
        std::string out;
        for( const bodypart_id &bp : a.get_all_body_parts(
                 get_body_part_flags::only_main ) ) {
            if( !out.empty() ) {
                out += ' ';
            }
            out += string_format( "%s=%d", bp.id().str(),
                                  a.get_part_hp_cur( bp ) );
        }
        return out;
    };
}

std::function<std::string()> world_creature_count()
{
    return []() {
        int mons = 0;
        int npcs = 0;
        int avs = 0;
        for( Creature &cc : g->all_creatures() ) {
            if( cc.is_monster() ) {
                mons++;
            } else if( cc.is_npc() ) {
                npcs++;
            } else if( cc.is_avatar() ) {
                avs++;
            }
        }
        return string_format( "total=%d mons=%d npcs=%d avatar=%d",
                              static_cast<int>( g->num_creatures() ),
                              mons, npcs, avs );
    };
}

std::function<std::string()> world_npc_count()
{
    return []() {
        avatar &you = get_avatar();
        int total = 0;
        int hostile = 0;
        int friendly = 0;
        int followers = 0;
        for( npc &guy : g->all_npcs() ) {
            total++;
            if( guy.is_player_ally() ) {
                followers++;
            }
            const Creature::Attitude att = guy.attitude_to( you );
            if( att == Creature::Attitude::HOSTILE ) {
                hostile++;
            } else if( att == Creature::Attitude::FRIENDLY ) {
                friendly++;
            }
        }
        return string_format( "npcs=%d hostile=%d friendly=%d followers=%d",
                              total, hostile, friendly, followers );
    };
}

std::function<std::string()> world_timed_event_count()
{
    return []() {
        return string_format( "timed_events=%d",
                              static_cast<int>( get_timed_events().get_all().size() ) );
    };
}

std::function<std::string()> world_item_wakeup_count()
{
    return []() {
        return string_format( "item_wakeups=%d",
                              static_cast<int>( get_item_wakeups().dump().size() ) );
    };
}

std::function<std::string()> world_creature_counts_in_bubble()
{
    return []() {
        int mons = 0;
        int npcs = 0;
        for( Creature &cc : g->all_creatures() ) {
            if( cc.is_monster() ) {
                mons++;
            } else if( cc.is_npc() ) {
                npcs++;
            }
        }
        return string_format( "monsters=%d npcs=%d", mons, npcs );
    };
}

std::function<std::string()> world_active_mission_count()
{
    return []() {
        avatar &a = get_avatar();
        return string_format( "active=%d completed=%d failed=%d",
                              static_cast<int>( a.get_active_missions().size() ),
                              static_cast<int>( a.get_completed_missions().size() ),
                              static_cast<int>( a.get_failed_missions().size() ) );
    };
}

std::function<std::string()> world_active_eoc_depth()
{
    return []() {
        return string_format( "global=%d avatar=%d",
                              static_cast<int>( g->queued_global_effect_on_conditions.list.size() ),
                              static_cast<int>( get_avatar().queued_effect_on_conditions.list.size() ) );
    };
}

std::function<std::string()> world_active_items()
{
    return []() {
        map &m = get_map();
        std::list<item_location> items = m.get_active_items_in_radius(
                                             get_avatar().pos_bub( m ), 60 );
        std::map<itype_id, int> counts;
        for( const item_location &it : items ) {
            if( it.get_item() != nullptr ) {
                counts[it->typeId()]++;
            }
        }
        std::vector<std::pair<itype_id, int>> sorted( counts.begin(), counts.end() );
        std::partial_sort( sorted.begin(),
                           sorted.begin() + std::min<size_t>( 3, sorted.size() ),
                           sorted.end(),
        []( const auto & a, const auto & b ) {
            return a.second > b.second;
        } );
        std::string top;
        const size_t k = std::min<size_t>( 3, sorted.size() );
        for( size_t i = 0; i < k; i++ ) {
            if( !top.empty() ) {
                top += ',';
            }
            top += string_format( "%s:%d", sorted[i].first.str(), sorted[i].second );
        }
        return string_format( "count=%d top=[%s]",
                              static_cast<int>( items.size() ),
                              top.empty() ? "-" : top.c_str() );
    };
}

std::function<std::string()> world_weather()
{
    return []() {
        const weather_manager &w = get_weather();
        return string_format( "type=%s temp=%dC wind=%dmph@%d",
                              w.weather_id.str(),
                              static_cast<int>( units::to_celsius( w.temperature ) ),
                              w.windspeed, w.winddirection );
    };
}

std::function<std::string()> world_calendar()
{
    return []() {
        return string_format( "turn=%d day=%d hour=%d season=%d",
                              to_turns<int>( calendar::turn - calendar::turn_zero ),
                              day_of_season<int>( calendar::turn ),
                              hour_of_day<int>( calendar::turn ),
                              static_cast<int>( season_of_year( calendar::turn ) ) );
    };
}

// Re-resolves the avatar's vehicle on every call so the snap survives the
// avatar driving away or stepping off.
std::function<std::string()> vehicle_under_avatar()
{
    return []() {
        avatar &a = get_avatar();
        map &m = get_map();
        const optional_vpart_position vp = m.veh_at( a.pos_bub( m ) );
        vehicle *v = veh_pointer_or_null( vp );
        if( v == nullptr ) {
            return std::string( "(not in vehicle)" );
        }
        return string_format( "%s vel=%d battery=%d", v->name, v->velocity,
                              static_cast<int>( v->battery_left( m ) ) );
    };
}

std::function<std::string()> world_tile_under_feet()
{
    return []() {
        avatar &a = get_avatar();
        map &m = get_map();
        const tripoint_bub_ms p = a.pos_bub( m );
        const std::string ter_s = m.ter( p ).id().str();
        const std::string furn_s = m.furn( p ).id().str();
        const int items = static_cast<int>( m.i_at( p ).size() );
        const int fields = m.field_at( p ).field_count();
        return string_format( "ter=%s furn=%s items=%d fields=%d",
                              ter_s, furn_s, items, fields );
    };
}

std::function<std::string()> world_items_under_feet()
{
    return []() {
        avatar &a = get_avatar();
        map &m = get_map();
        const tripoint_bub_ms p = a.pos_bub( m );
        map_stack s = m.i_at( p );
        const std::string first = s.empty() ? std::string( "(empty)" )
                                  : remove_color_tags( s.begin()->tname() );
        units::mass total_w = 0_gram;
        units::volume total_v = 0_ml;
        for( const item &it : s ) {
            total_w += it.weight();
            total_v += it.volume();
        }
        return string_format( "n=%d wt=%dg vol=%dml first=%s",
                              static_cast<int>( s.size() ),
                              units::to_gram( total_w ),
                              units::to_milliliter( total_v ),
                              first );
    };
}

std::function<std::string()> imgui_fps()
{
    return []() {
        return string_format( "fps=%.1f dt=%.2fms",
                              ImGui::GetIO().Framerate,
                              ImGui::GetIO().DeltaTime * 1000.0f );
    };
}

std::function<std::string()> skill( skill_id sid )
{
    return [sid]() {
        const avatar &a = get_avatar();
        return string_format( "%s lvl=%d know=%d", sid.str(),
                              a.get_skill_level( sid ),
                              a.get_knowledge_level( sid ) );
    };
}

std::function<std::string()> proficiency_progress( proficiency_id pid )
{
    return [pid]() {
        const avatar &a = get_avatar();
        const bool learned = a.has_proficiency( pid );
        const time_duration prac = a.get_proficiency_practiced_time( pid );
        const time_duration need = pid->time_to_learn();
        const int prac_s = to_seconds<int>( prac );
        const int need_s = to_seconds<int>( need );
        const int pct = need_s > 0 ? ( prac_s * 100 ) / need_s : 100;
        return string_format( "%s learned=%d practiced=%s/%s (%d%%)",
                              pid.str(), learned ? 1 : 0,
                              to_string( prac ), to_string( need ), pct );
    };
}

// Re-resolve each tick by id so the snapshot survives the creature being
// despawned or the npc list reshuffling.
static Character *resolve_char( character_id id )
{
    avatar &a = get_avatar();
    if( a.getID() == id ) {
        return &a;
    }
    return g->find_npc( id );
}

std::function<std::string()> char_vitals( character_id id )
{
    return [id]() {
        const Character *c = resolve_char( id );
        if( !c ) {
            return std::string( "(gone)" );
        }
        return string_format( "hp=%d hunger=%d thirst=%d sleepiness=%d sleep_dep=%d pain=%d morale=%d",
                              c->get_hp(), c->get_hunger(), c->get_thirst(),
                              c->get_sleepiness(), c->get_sleep_deprivation(),
                              c->get_pain(), c->get_morale_level() );
    };
}

std::function<std::string()> char_metabolism( character_id id )
{
    return [id]() {
        const Character *c = resolve_char( id );
        if( !c ) {
            return std::string( "(gone)" );
        }
        return string_format( "hunger=%d thirst=%d kcal=%d/%d bmi=%.1f bmr=%d",
                              c->get_hunger(), c->get_thirst(),
                              c->get_stored_kcal(), c->get_healthy_kcal(),
                              c->get_bmi_fat(), c->get_bmr() );
    };
}

std::function<std::string()> char_gi( character_id id )
{
    return [id]() {
        const Character *c = resolve_char( id );
        if( !c ) {
            return std::string( "(gone)" );
        }
        return string_format( "stomach=%d/%d (%d kcal) guts=%d/%d (%d kcal)",
                              units::to_milliliter( c->stomach.contains() ),
                              units::to_milliliter( c->stomach.capacity( *c ) ),
                              c->stomach.get_calories(),
                              units::to_milliliter( c->guts.contains() ),
                              units::to_milliliter( c->guts.capacity( *c ) ),
                              c->guts.get_calories() );
    };
}

std::function<std::string()> char_skill( character_id id, skill_id sid )
{
    return [id, sid]() {
        const Character *c = resolve_char( id );
        if( !c ) {
            return std::string( "(gone)" );
        }
        return string_format( "%s lvl=%d know=%d", sid.str(),
                              c->get_skill_level( sid ),
                              c->get_knowledge_level( sid ) );
    };
}

std::function<std::string()> char_proficiency( character_id id, proficiency_id pid )
{
    return [id, pid]() {
        const Character *c = resolve_char( id );
        if( !c ) {
            return std::string( "(gone)" );
        }
        const bool learned = c->has_proficiency( pid );
        const time_duration prac = c->get_proficiency_practiced_time( pid );
        const time_duration need = pid->time_to_learn();
        const int prac_s = to_seconds<int>( prac );
        const int need_s = to_seconds<int>( need );
        const int pct = need_s > 0 ? ( prac_s * 100 ) / need_s : 100;
        return string_format( "%s learned=%d practiced=%s/%s (%d%%)",
                              pid.str(), learned ? 1 : 0,
                              to_string( prac ), to_string( need ), pct );
    };
}

std::function<std::string()> item_charges( itype_id iid )
{
    return [iid]() {
        return string_format( "%s charges=%d", iid.str(),
                              get_avatar().charges_of( iid ) );
    };
}

std::function<std::string()> item_count( itype_id iid )
{
    return [iid]() {
        return string_format( "%s count=%d", iid.str(),
                              get_avatar().amount_of( iid ) );
    };
}

std::function<std::string()> effect_intensity( efftype_id eid )
{
    return [eid]() {
        const avatar &a = get_avatar();
        if( !a.has_effect( eid ) ) {
            return string_format( "%s=absent", eid.str() );
        }
        const effect &ef = a.get_effect( eid );
        return string_format( "%s int=%d dur=%s", eid.str(),
                              ef.get_intensity(), to_string( ef.get_duration() ) );
    };
}

std::function<std::string()> bionic_state( bionic_id bid )
{
    return [bid]() {
        const avatar &a = get_avatar();
        return string_format( "%s installed=%d active=%d power=%dkJ/%dkJ",
                              bid.str(),
                              a.has_bionic( bid ) ? 1 : 0,
                              a.has_active_bionic( bid ) ? 1 : 0,
                              units::to_kilojoule( a.get_power_level() ),
                              units::to_kilojoule( a.get_max_power_level() ) );
    };
}

std::function<std::string()> tile_at_offset( tripoint_rel_ms off )
{
    return [off]() {
        map &m = get_map();
        const tripoint_bub_ms p = get_avatar().pos_bub( m ) + off;
        const std::string ter_s = m.ter( p ).id().str();
        const std::string furn_s = m.furn( p ).id().str();
        const int items = static_cast<int>( m.i_at( p ).size() );
        const int fields = m.field_at( p ).field_count();
        return string_format( "ter=%s furn=%s items=%d fields=%d",
                              ter_s, furn_s, items, fields );
    };
}

std::function<std::string()> field_at_offset( tripoint_rel_ms off )
{
    return [off]() {
        map &m = get_map();
        const tripoint_bub_ms p = get_avatar().pos_bub( m ) + off;
        const field &f = m.field_at( p );
        std::string out;
        for( const auto &fpair : f ) {
            if( !out.empty() ) {
                out += ' ';
            }
            out += string_format( "%s=%d", fpair.first.id().str(),
                                  fpair.second.get_field_intensity() );
        }
        return out.empty() ? std::string( "(no fields)" ) : out;
    };
}

std::function<std::string()> light_at_offset( tripoint_rel_ms off )
{
    return [off]() {
        map &m = get_map();
        const tripoint_bub_ms p = get_avatar().pos_bub( m ) + off;
        return string_format( "light=%.2f", m.ambient_light_at( p ) );
    };
}

std::function<std::string()> tile_at_abs( tripoint_bub_ms p )
{
    return [p]() {
        map &m = get_map();
        const std::string ter_s = m.ter( p ).id().str();
        const std::string furn_s = m.furn( p ).id().str();
        const int items = static_cast<int>( m.i_at( p ).size() );
        const int fields = m.field_at( p ).field_count();
        return string_format( "ter=%s furn=%s items=%d fields=%d",
                              ter_s, furn_s, items, fields );
    };
}

std::function<std::string()> field_at_abs( tripoint_bub_ms p )
{
    return [p]() {
        map &m = get_map();
        const field &f = m.field_at( p );
        std::string out;
        for( const auto &fpair : f ) {
            if( !out.empty() ) {
                out += ' ';
            }
            out += string_format( "%s=%d", fpair.first.id().str(),
                                  fpair.second.get_field_intensity() );
        }
        return out.empty() ? std::string( "(no fields)" ) : out;
    };
}

std::function<std::string()> light_at_abs( tripoint_bub_ms p )
{
    return [p]() {
        return string_format( "light=%.2f", get_map().ambient_light_at( p ) );
    };
}

std::function<std::string()> creature_named( std::string name )
{
    return [nm = std::move( name )]() {
        avatar &you = get_avatar();
        map &m = get_map();
        for( Creature &cc : g->all_creatures() ) {
            if( cc.get_name() == nm ) {
                const Creature::Attitude att = cc.attitude_to( you );
                const char *att_s = att == Creature::Attitude::HOSTILE ? "H"
                                    : att == Creature::Attitude::FRIENDLY ? "F" : "N";
                const int d = rl_dist( you.pos_bub( m ), cc.pos_bub( m ) );
                return string_format( "%s [%s] hp=%d/%d dist=%d spd=%d pos=%s",
                                      nm, att_s,
                                      cc.get_hp(), cc.get_hp_max(), d,
                                      cc.get_speed(),
                                      cc.pos_bub( m ).to_string() );
            }
        }
        return string_format( "%s (absent)", nm );
    };
}

std::function<std::string()> npc_distance( std::string name )
{
    return [nm = std::move( name )]() {
        avatar &you = get_avatar();
        map &m = get_map();
        for( npc &n : g->all_npcs() ) {
            if( n.get_name() == nm ) {
                const int d = rl_dist( you.pos_bub( m ), n.pos_bub( m ) );
                const Creature::Attitude att = n.attitude_to( you );
                const char *att_s = att == Creature::Attitude::HOSTILE ? "H"
                                    : att == Creature::Attitude::FRIENDLY ? "F" : "N";
                const faction *f = n.get_faction();
                const std::string fac_s = f ? f->id.str() : std::string( "?" );
                const std::string act_s = n.activity.id().is_null()
                                          ? std::string( "idle" )
                                          : n.activity.id().str();
                return string_format( "%s [%s] hp=%d/%d dist=%d fac=%s act=%s",
                                      nm, att_s, n.get_hp(), n.get_hp_max(),
                                      d, fac_s, act_s );
            }
        }
        return string_format( "%s (absent)", nm );
    };
}

std::function<std::string()> faction_stats( faction_id fid )
{
    return [fid]() {
        faction *f = g->faction_manager_ptr->get( fid, false );
        if( f == nullptr ) {
            return string_format( "%s (unknown)", fid.str() );
        }
        return string_format( "%s likes=%d respects=%d trusts=%d power=%d",
                              fid.str(), f->likes_u, f->respects_u,
                              f->trusts_u, f->power );
    };
}

std::function<std::string()> event_count_this_turn( int event_type_idx,
        std::string ev_name )
{
    return [event_type_idx, ev_name = std::move( ev_name )]() {
        if( !debug_capture::is_initialized() ) {
            return string_format( "%s capture-off", ev_name );
        }
        const int now = to_turns<int>(
                            calendar::turn - calendar::turn_zero );
        int n = 0;
        for( const event_log_entry &e :
             debug_capture::instance().events() ) {
            if( e.event_type_idx == event_type_idx && e.turn_num == now ) {
                n++;
            }
        }
        return string_format( "%s @T%d count=%d", ev_name, now, n );
    };
}

std::function<std::string()> global_var( std::string vname )
{
    return [vname = std::move( vname )]() {
        diag_value const *dv = get_globals().maybe_get_global_value( vname );
        if( dv == nullptr ) {
            return string_format( "%s (unset)", vname );
        }
        return string_format( "%s = %s", vname, dv->to_string() );
    };
}

std::function<std::string()> math_expr( std::string expr )
{
    auto mp = std::make_shared<math_exp>();
    if( !mp->parse( expr, false ) ) {
        return {};
    }
    return [mp, expr = std::move( expr )]() {
        try {
            dialogue d( get_talker_for( get_avatar() ), nullptr );
            const double v = mp->eval( d );
            return string_format( "%s = %g", expr, v );
        } catch( const std::exception &e ) {
            std::string what = e.what();
            const size_t nl = what.find( '\n' );
            if( nl != std::string::npos ) {
                what.resize( nl );
            }
            return string_format( "%s = ERR: %s", expr, what );
        }
    };
}

// Folds per-fire count + last-fire turn into the snap so on_change monitors
// flip when the EOC fires.
std::function<std::string()> eoc_state( effect_on_condition_id eid )
{
    return [eid]() -> std::string {
        const std::string &id = eid.str();
        auto search = []( const std::string & needle, const queued_eocs & q )
        -> std::optional<time_point> {
            for( const queued_eoc &qe : q.list )
            {
                if( qe.eoc.str() == needle ) {
                    return qe.time;
                }
            }
            return std::nullopt;
        };
        std::string base;
        if( auto t = search( id, g->queued_global_effect_on_conditions ) )
        {
            base = string_format( "queued@%s",
                                  to_string( *t - calendar::turn ) );
        } else if( auto t = search( id, get_avatar().queued_effect_on_conditions ) )
        {
            base = string_format( "queued@%s",
                                  to_string( *t - calendar::turn ) );
        } else
        {
            bool inactive = false;
            for( const effect_on_condition_id &eid2 :
                 g->inactive_global_effect_on_condition_vector ) {
                if( eid2.str() == id ) {
                    inactive = true;
                    break;
                }
            }
            if( !inactive ) {
                for( const effect_on_condition_id &eid2 :
                     get_avatar().inactive_effect_on_condition_vector ) {
                    if( eid2.str() == id ) {
                        inactive = true;
                        break;
                    }
                }
            }
            base = inactive ? "inactive" : "loaded";
        }
        if( debug_capture::is_initialized() )
        {
            int fires = 0;
            int last_t = -1;
            bool last_ok = true;
            for( const eoc_trace_entry &tr :
                 debug_capture::instance().eoc_traces() ) {
                if( tr.eoc_id == id ) {
                    fires++;
                    last_t = tr.turn_num;
                    last_ok = tr.success;
                }
            }
            if( fires > 0 ) {
                base += string_format( " fires:%d last@T%d %s", fires,
                                       last_t, last_ok ? "ok" : "fail" );
            }
        }
        return base;
    };
}

std::function<std::string()> nearest_creature()
{
    return []() {
        avatar &you = get_avatar();
        map &m = get_map();
        const tripoint_bub_ms apos = you.pos_bub( m );
        Creature *c = nullptr;
        int best = INT_MAX;
        for( Creature &cand : g->all_creatures() ) {
            if( &cand == &you ) {
                continue;
            }
            const int d = rl_dist( apos, cand.pos_bub( m ) );
            if( d < best ) {
                best = d;
                c = &cand;
            }
        }
        if( c == nullptr ) {
            return std::string( "(none)" );
        }
        const Creature::Attitude att = c->attitude_to( you );
        const char *att_s = att == Creature::Attitude::HOSTILE ? "H"
                            : att == Creature::Attitude::FRIENDLY ? "F" : "N";
        return string_format( "%s [%s] hp=%d/%d dist=%d spd=%d pos=%s",
                              c->get_name(), att_s,
                              c->get_hp(), c->get_hp_max(), best,
                              c->get_speed(),
                              c->pos_bub( m ).to_string() );
    };
}

} // namespace debug_menu::snap
