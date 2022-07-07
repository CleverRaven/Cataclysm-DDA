#include "iuse_actor.h"

#include <cctype>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <numeric>
#include <sstream>
#include <type_traits>

#include "action.h"
#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "activity_type.h"
#include "assign.h"
#include "avatar.h" // IWYU pragma: keep
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "character_id.h"
#include "clothing_mod.h"
#include "clzones.h"
#include "colony.h"
#include "crafting.h"
#include "creature.h"
#include "creature_tracker.h"
#include "damage.h"
#include "debug.h"
#include "effect.h"
#include "effect_on_condition.h"
#include "enum_conversions.h"
#include "enums.h"
#include "explosion.h"
#include "field_type.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "generic_factory.h"
#include "inventory.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "json.h"
#include "line.h"
#include "magic.h"
#include "map.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapdata.h"
#include "material.h"
#include "memory_fast.h"
#include "messages.h"
#include "monster.h"
#include "morale_types.h"
#include "mtype.h"
#include "music.h"
#include "mutation.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player_activity.h"
#include "point.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "talker.h"
#include "translations.h"
#include "trap.h"
#include "ui.h"
#include "units_utility.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "visitable.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"
#include "weather_type.h"

static const activity_id ACT_FIRSTAID( "ACT_FIRSTAID" );
static const activity_id ACT_REPAIR_ITEM( "ACT_REPAIR_ITEM" );
static const activity_id ACT_SPELLCASTING( "ACT_SPELLCASTING" );
static const activity_id ACT_START_FIRE( "ACT_START_FIRE" );
static const activity_id ACT_STUDY_SPELL( "ACT_STUDY_SPELL" );

static const efftype_id effect_asthma( "asthma" );
static const efftype_id effect_bandaged( "bandaged" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_disinfected( "disinfected" );
static const efftype_id effect_incorporeal( "incorporeal" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_masked_scent( "masked_scent" );
static const efftype_id effect_music( "music" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_playing_instrument( "playing_instrument" );
static const efftype_id effect_recover( "recover" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_stunned( "stunned" );

static const fault_id fault_bionic_salvaged( "fault_bionic_salvaged" );

static const gun_mode_id gun_mode_DEFAULT( "DEFAULT" );

static const itype_id itype_barrel_small( "barrel_small" );
static const itype_id itype_brazier( "brazier" );
static const itype_id itype_char_smoker( "char_smoker" );
static const itype_id itype_fire( "fire" );
static const itype_id itype_stock_none( "stock_none" );
static const itype_id itype_syringe( "syringe" );

static const proficiency_id proficiency_prof_traps( "prof_traps" );
static const proficiency_id proficiency_prof_trapsetting( "prof_trapsetting" );
static const proficiency_id proficiency_prof_wound_care( "prof_wound_care" );
static const proficiency_id proficiency_prof_wound_care_expert( "prof_wound_care_expert" );

static const quality_id qual_DIG( "DIG" );

static const skill_id skill_fabrication( "fabrication" );
static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_survival( "survival" );
static const skill_id skill_traps( "traps" );

static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_DEBUG_BIONICS( "DEBUG_BIONICS" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_LIGHTWEIGHT( "LIGHTWEIGHT" );
static const trait_id trait_MASOCHIST( "MASOCHIST" );
static const trait_id trait_MASOCHIST_MED( "MASOCHIST_MED" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_PYROMANIA( "PYROMANIA" );
static const trait_id trait_TOLERANCE( "TOLERANCE" );

static const trap_str_id tr_firewood_source( "tr_firewood_source" );

static const zone_type_id zone_type_SOURCE_FIREWOOD( "SOURCE_FIREWOOD" );

std::unique_ptr<iuse_actor> iuse_transform::clone() const
{
    return std::make_unique<iuse_transform>( *this );
}

void iuse_transform::load( const JsonObject &obj )
{
    obj.read( "target", target, true );

    obj.read( "msg", msg_transform );
    obj.read( "container", container );
    obj.read( "sealed", sealed );
    if( obj.has_member( "target_charges" ) && obj.has_member( "rand_target_charges" ) ) {
        obj.throw_error_at( "target_charges",
                            "Transform actor specified both fixed and random target charges" );
    }
    obj.read( "target_charges", ammo_qty );
    if( obj.has_array( "rand_target_charges" ) ) {
        for( const int charge : obj.get_array( "rand_target_charges" ) ) {
            random_ammo_qty.push_back( charge );
        }
        if( random_ammo_qty.size() < 2 ) {
            obj.throw_error_at( "rand_target_charges",
                                "You must specify two or more values to choose between" );
        }
    }
    obj.read( "target_ammo", ammo_type );

    obj.read( "countdown", countdown );

    if( !ammo_type.is_empty() && !container.is_empty() ) {
        obj.throw_error_at( "target_ammo", "Transform actor specified both ammo type and container type" );
    }

    obj.read( "active", active );

    obj.read( "moves", moves );
    if( moves < 0 ) {
        obj.throw_error_at( "moves", "transform actor specified negative moves" );
    }

    obj.read( "need_fire", need_fire );
    need_fire = std::max( need_fire, 0 );
    if( !obj.read( "need_charges_msg", need_charges_msg ) ) {
        need_charges_msg = to_translation( "The %s is empty!" );
    }

    obj.read( "need_charges", need_charges );
    need_charges = std::max( need_charges, 0 );
    if( !obj.read( "need_fire_msg", need_fire_msg ) ) {
        need_fire_msg = to_translation( "You need a source of fire!" );
    }

    obj.read( "need_worn", need_worn );
    obj.read( "need_wielding", need_wielding );

    obj.read( "qualities_needed", qualities_needed );

    obj.read( "menu_text", menu_text );
}

cata::optional<int> iuse_transform::use( Character &p, item &it, bool t, const tripoint &pos ) const
{
    float scale = 1;
    auto iter = it.type->ammo_scale.find( type );
    if( iter != it.type->ammo_scale.end() ) {
        scale = iter->second;
    }
    if( t ) {
        return cata::nullopt; // invoked from active item processing, do nothing.
    }

    int result = 0;

    const bool possess = p.has_item( it ) ||
                         ( it.has_flag( flag_ALLOWS_REMOTE_USE ) && square_dist( p.pos(), pos ) <= 1 );

    if( possess && need_worn && !p.is_worn( it ) ) {
        p.add_msg_if_player( m_info, _( "You need to wear the %1$s before activating it." ), it.tname() );
        return cata::nullopt;
    }
    if( possess && need_wielding && !p.is_wielding( it ) ) {
        p.add_msg_if_player( m_info, _( "You need to wield the %1$s before activating it." ), it.tname() );
        return cata::nullopt;
    }

    if( p.is_worn( it ) ) {
        item tmp = item( target );
        if( !tmp.has_flag( flag_OVERSIZE ) && !tmp.has_flag( flag_SEMITANGIBLE ) ) {
            for( const trait_id &mut : p.get_mutations() ) {
                const mutation_branch &branch = mut.obj();
                if( branch.conflicts_with_item( tmp ) ) {
                    p.add_msg_if_player( m_info, _( "Your %1$s mutation prevents you from doing that." ),
                                         p.mutation_name( mut ) );
                    return cata::nullopt;
                }
            }
        }
    }

    if( need_charges && it.ammo_remaining( &p ) < need_charges ) {

        if( possess ) {
            p.add_msg_if_player( m_info, need_charges_msg, it.tname() );
        }
        return cata::nullopt;
    }

    if( need_fire && possess ) {
        if( !p.use_charges_if_avail( itype_fire, need_fire ) ) {
            p.add_msg_if_player( m_info, need_fire_msg, it.tname() );
            return cata::nullopt;
        }
        if( p.is_underwater() ) {
            p.add_msg_if_player( m_info, _( "You can't do that while underwater" ) );
            return cata::nullopt;
        }
    }

    if( possess && !msg_transform.empty() ) {
        p.add_msg_if_player( m_neutral, msg_transform, it.tname() );
    }

    if( possess ) {
        p.moves -= moves;
    }

    if( possess && need_fire && p.has_trait( trait_PYROMANIA ) ) {
        if( one_in( 2 ) ) {
            p.add_msg_if_player( m_mixed,
                                 _( "You light a fire, but it isn't enough.  You need to light more." ) );
        } else {
            p.add_msg_if_player( m_good, _( "You happily light a fire." ) );
            p.add_morale( MORALE_PYROMANIA_STARTFIRE, 5, 10, 3_hours, 2_hours );
            p.rem_morale( MORALE_PYROMANIA_NOFIRE );
        }
    }

    item obj_copy( it );
    item *obj;
    // defined here to allow making a new item assigned to the pointer
    item obj_it;
    if( it.is_tool() ) {
        result = int( it.type->charges_to_use() * double( scale ) );
    }
    if( container.is_empty() ) {
        obj = &it.convert( target );
        if( ammo_qty >= 0 || !random_ammo_qty.empty() ) {
            int qty;
            if( !random_ammo_qty.empty() ) {
                const int index = rng( 1, random_ammo_qty.size() - 1 );
                qty = rng( random_ammo_qty[index - 1], random_ammo_qty[index] );
            } else {
                qty = ammo_qty;
            }
            if( !ammo_type.is_empty() ) {
                obj->ammo_set( ammo_type, qty );
            } else if( obj->is_ammo() ) {
                obj->charges = qty;
            } else if( !obj->ammo_current().is_null() ) {
                obj->ammo_set( obj->ammo_current(), qty );
            } else if( obj->has_flag( flag_RADIO_ACTIVATION ) && obj->has_flag( flag_BOMB ) ) {
                obj->set_countdown( 1 );
            } else {
                obj->set_countdown( qty );
            }
        }
    } else {
        it.convert( container );
        obj_it = item( target, calendar::turn, std::max( ammo_qty, 1 ) );
        obj = &obj_it;
        if( !it.put_in( *obj, item_pocket::pocket_type::CONTAINER ).success() ) {
            it.put_in( *obj, item_pocket::pocket_type::MIGRATION );
        }
        if( sealed ) {
            it.seal();
        }
    }
    obj->item_counter = countdown > 0 ? countdown : obj->type->countdown_interval;
    obj->active = active || obj->item_counter || obj->has_temperature();
    if( p.is_worn( *obj ) ) {
        if( !obj->is_armor() ) {
            item_location il = item_location( p, obj );
            p.takeoff( il );
        } else {
            p.calc_encumbrance();
            p.update_bodytemp();
            p.on_worn_item_transform( obj_copy, *obj );
        }
    }

    return result;
}

ret_val<bool> iuse_transform::can_use( const Character &p, const item &, bool,
                                       const tripoint & ) const
{
    if( qualities_needed.empty() ) {
        return ret_val<bool>::make_success();
    }

    std::map<quality_id, int> unmet_reqs;
    inventory inv;
    inv.form_from_map( p.pos(), 1, &p, true, true );
    for( const auto &quality : qualities_needed ) {
        if( !p.has_quality( quality.first, quality.second ) &&
            !inv.has_quality( quality.first, quality.second ) ) {
            unmet_reqs.insert( quality );
        }
    }
    if( unmet_reqs.empty() ) {
        return ret_val<bool>::make_success();
    }
    std::string unmet_reqs_string = enumerate_as_string( unmet_reqs.begin(), unmet_reqs.end(),
    [&]( const std::pair<quality_id, int> &unmet_req ) {
        return string_format( "%s %d", unmet_req.first.obj().name, unmet_req.second );
    } );
    return ret_val<bool>::make_failure( n_gettext( "You need a tool with %s.",
                                        "You need tools with %s.",
                                        unmet_reqs.size() ), unmet_reqs_string );
}

std::string iuse_transform::get_name() const
{
    if( !menu_text.empty() ) {
        return menu_text.translated();
    }
    return iuse_actor::get_name();
}

void iuse_transform::finalize( const itype_id & )
{
    if( !item::type_is_defined( target ) ) {
        debugmsg( "Invalid transform target: %s", target.c_str() );
    }

    if( !container.is_empty() ) {
        if( !item::type_is_defined( container ) ) {
            debugmsg( "Invalid transform container: %s", container.c_str() );
        }

        item dummy( target );
        if( ammo_qty > 1 && !dummy.count_by_charges() ) {
            debugmsg( "Transform target with container must be an item with charges, got non-charged: %s",
                      target.c_str() );
        }
    }
}

void iuse_transform::info( const item &it, std::vector<iteminfo> &dump ) const
{
    item dummy( target, calendar::turn, std::max( ammo_qty, 1 ) );
    if( it.has_flag( flag_FIT ) ) {
        dummy.set_flag( flag_FIT );
    }
    dump.emplace_back( "TOOL", string_format( _( "<bold>Turns into</bold>: %s" ),
                       dummy.tname() ) );
    if( countdown > 0 ) {
        dump.emplace_back( "TOOL", _( "Countdown: " ), countdown );
    }

    const auto *explosion_use = dummy.get_use( "explosion" );
    if( explosion_use != nullptr ) {
        explosion_use->get_actor_ptr()->info( it, dump );
    }
}

std::unique_ptr<iuse_actor> unpack_actor::clone() const
{
    return std::make_unique<unpack_actor>( *this );
}

void unpack_actor::load( const JsonObject &obj )
{
    obj.read( "group", unpack_group );
    obj.read( "items_fit", items_fit );
    assign( obj, "filthy_volume_threshold", filthy_vol_threshold );
}

cata::optional<int> unpack_actor::use( Character &p, item &it, bool, const tripoint & ) const
{
    std::vector<item> items = item_group::items_from( unpack_group, calendar::turn );
    item last_armor;

    p.add_msg_if_player( _( "You unpack the %s." ), it.tname() );

    map &here = get_map();
    for( item &content : items ) {
        if( content.is_armor() ) {
            if( items_fit ) {
                content.set_flag( flag_FIT );
            } else if( content.typeId() == last_armor.typeId() ) {
                if( last_armor.has_flag( flag_FIT ) ) {
                    content.set_flag( flag_FIT );
                } else if( !last_armor.has_flag( flag_FIT ) ) {
                    content.unset_flag( flag_FIT );
                }
            }
            last_armor = content;
        }

        if( content.get_total_capacity() >= filthy_vol_threshold &&
            it.has_flag( flag_FILTHY ) ) {
            content.set_flag( flag_FILTHY );
        }

        here.add_item_or_charges( p.pos(), content );
    }

    p.i_rem( &it );

    return 0;
}

void unpack_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    dump.emplace_back( "DESCRIPTION",
                       _( "This item could be unpacked to receive something." ) );
}

std::unique_ptr<iuse_actor> countdown_actor::clone() const
{
    return std::make_unique<countdown_actor>( *this );
}

void countdown_actor::load( const JsonObject &obj )
{
    obj.read( "name", name );
    obj.read( "interval", interval );
    obj.read( "message", message );
}

cata::optional<int> countdown_actor::use( Character &p, item &it, bool t,
        const tripoint &pos ) const
{
    if( t ) {
        return cata::nullopt;
    }

    if( it.active ) {
        return cata::nullopt;
    }

    if( p.sees( pos ) && !message.empty() ) {
        p.add_msg_if_player( m_neutral, message.translated(), it.tname() );
    }

    it.item_counter = interval > 0 ? interval : it.type->countdown_interval;
    it.active = true;
    return 0;
}

ret_val<bool> countdown_actor::can_use( const Character &, const item &it, bool,
                                        const tripoint & ) const
{
    if( it.active ) {
        return ret_val<bool>::make_failure( _( "It's already been triggered." ) );
    }

    return ret_val<bool>::make_success();
}

std::string countdown_actor::get_name() const
{
    if( !name.empty() ) {
        return name.translated();
    }
    return iuse_actor::get_name();
}

void countdown_actor::info( const item &it, std::vector<iteminfo> &dump ) const
{
    dump.emplace_back( "TOOL", _( "Countdown: " ),
                       interval > 0 ? interval : it.type->countdown_interval );
    const iuse_actor *countdown_actor = it.type->countdown_action.get_actor_ptr();
    if( countdown_actor != nullptr ) {
        countdown_actor->info( it, dump );
    }
}

std::unique_ptr<iuse_actor> explosion_iuse::clone() const
{
    return std::make_unique<explosion_iuse>( *this );
}

// For an explosion (which releases some kind of gas), this function
// calculates the points around that explosion where to create those
// gas fields.
// Those points must have a clear line of sight and a clear path to
// the center of the explosion.
// They must also be passable.
static std::vector<tripoint> points_for_gas_cloud( const tripoint &center, int radius )
{
    map &here = get_map();
    std::vector<tripoint> result;
    for( const tripoint &p : closest_points_first( center, radius ) ) {
        if( here.impassable( p ) ) {
            continue;
        }
        if( p != center ) {
            if( !here.clear_path( center, p, radius, 1, 100 ) ) {
                // Can not splatter gas from center to that point, something is in the way
                continue;
            }
        }
        result.push_back( p );
    }
    return result;
}

void explosion_iuse::load( const JsonObject &obj )
{
    if( obj.has_object( "explosion" ) ) {
        JsonObject expl = obj.get_object( "explosion" );
        explosion = load_explosion_data( expl );
    }

    obj.read( "draw_explosion_radius", draw_explosion_radius );
    if( obj.has_member( "draw_explosion_color" ) ) {
        draw_explosion_color = color_from_string( obj.get_string( "draw_explosion_color" ) );
    }
    obj.read( "do_flashbang", do_flashbang );
    obj.read( "flashbang_player_immune", flashbang_player_immune );
    obj.read( "fields_radius", fields_radius );
    if( obj.has_member( "fields_type" ) || fields_radius > 0 ) {
        fields_type = field_type_id( obj.get_string( "fields_type" ) );
    }
    obj.read( "fields_min_intensity", fields_min_intensity );
    obj.read( "fields_max_intensity", fields_max_intensity );
    if( fields_max_intensity == 0 ) {
        fields_max_intensity = fields_type.obj().get_max_intensity();
    }
    obj.read( "emp_blast_radius", emp_blast_radius );
    obj.read( "scrambler_blast_radius", scrambler_blast_radius );
    obj.read( "sound_volume", sound_volume );
    obj.read( "sound_msg", sound_msg );
    obj.read( "no_deactivate_msg", no_deactivate_msg );
}

cata::optional<int> explosion_iuse::use( Character &p, item &it, bool t, const tripoint &pos ) const
{
    if( t ) {
        if( sound_volume >= 0 ) {
            sounds::sound( pos, sound_volume, sounds::sound_t::alarm,
                           sound_msg.empty() ? _( "Tick." ) : sound_msg.translated(), true, "misc", "bomb_ticking" );
        }
        return 0;
    }
    if( it.charges > 0 ) {
        if( p.has_item( it ) ) {
            if( no_deactivate_msg.empty() ) {
                p.add_msg_if_player( m_warning,
                                     _( "You've already set the %s's timer you might want to get away from it." ), it.tname() );
            } else {
                p.add_msg_if_player( m_info, no_deactivate_msg.translated(), it.tname() );
            }
        }
        return 0;
    }

    if( explosion.power >= 0.0f ) {
        explosion_handler::explosion( pos, explosion );
    }

    if( draw_explosion_radius >= 0 ) {
        explosion_handler::draw_explosion( pos, draw_explosion_radius, draw_explosion_color );
    }
    if( do_flashbang ) {
        explosion_handler::flashbang( pos, flashbang_player_immune );
    }
    map &here = get_map();
    if( fields_radius >= 0 && fields_type.id() ) {
        std::vector<tripoint> gas_sources = points_for_gas_cloud( pos, fields_radius );
        for( tripoint &gas_source : gas_sources ) {
            const int field_intensity = rng( fields_min_intensity, fields_max_intensity );
            here.add_field( gas_source, fields_type, field_intensity, 1_turns );
        }
    }
    if( scrambler_blast_radius >= 0 ) {
        for( const tripoint &dest : here.points_in_radius( pos, scrambler_blast_radius ) ) {
            explosion_handler::scrambler_blast( dest );
        }
    }
    if( emp_blast_radius >= 0 ) {
        for( const tripoint &dest : here.points_in_radius( pos, emp_blast_radius ) ) {
            explosion_handler::emp_blast( dest );
        }
    }
    return 1;
}

void explosion_iuse::info( const item &, std::vector<iteminfo> &dump ) const
{
    if( explosion.power <= 0 ) {
        // TODO: List other effects, like EMP and clouds
        return;
    }

    dump.emplace_back( "TOOL", _( "Power at epicenter: " ), explosion.power );
    const shrapnel_data &sd = explosion.shrapnel;
    if( sd.casing_mass > 0 ) {
        dump.emplace_back( "TOOL", _( "Casing mass: " ), sd.casing_mass );
        dump.emplace_back( "TOOL", _( "Fragment mass: " ), string_format( "%.2f",
                           sd.fragment_mass ) );
    }
}

std::unique_ptr<iuse_actor> unfold_vehicle_iuse::clone() const
{
    return std::make_unique<unfold_vehicle_iuse>( *this );
}

void unfold_vehicle_iuse::load( const JsonObject &obj )
{
    vehicle_id = vproto_id( obj.get_string( "vehicle_name" ) );
    obj.read( "unfold_msg", unfold_msg );
    obj.read( "moves", moves );
    obj.read( "tools_needed", tools_needed );
}

cata::optional<int> unfold_vehicle_iuse::use( Character &p, item &it, bool, const tripoint & ) const
{
    if( p.is_underwater() ) {
        p.add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return cata::nullopt;
    }
    if( p.is_mounted() ) {
        p.add_msg_if_player( m_info, _( "You cannot do that while mounted." ) );
        return cata::nullopt;
    }
    for( const auto &tool : tools_needed ) {
        // Amount == -1 means need one, but don't consume it.
        if( !p.has_amount( tool.first, 1 ) ) {
            p.add_msg_if_player( _( "You need %s to do it!" ),
                                 item::nname( tool.first ) );
            return cata::nullopt;
        }
    }

    vehicle *veh = get_map().add_vehicle( vehicle_id, p.pos(), 0_degrees, 0, 0, false );
    if( veh == nullptr ) {
        p.add_msg_if_player( m_info, _( "There's no room to unfold the %s." ), it.tname() );
        return cata::nullopt;
    }
    veh->set_owner( p );
    // Set damage and degradation based on source item.
    // This is to preserve the item's state if it has
    // never been unfolded (no saved parts data).
    for( int i = 0; i < veh->part_count(); i++ ) {
        item vp = veh->part( i ).get_base();
        vp.set_damage( it.damage() );
        vp.set_degradation( it.degradation() );
        veh->part( i ).set_base( vp );
    }

    // Mark the vehicle as foldable.
    veh->tags.insert( "convertible" );
    // Store the id of the item the vehicle is made of.
    veh->tags.insert( std::string( "convertible:" ) + it.typeId().str() );
    if( !unfold_msg.empty() ) {
        p.add_msg_if_player( unfold_msg.translated(), it.tname() );
    }
    p.moves -= moves;
    // Restore HP of parts if we stashed them previously.
    if( it.has_var( "folding_bicycle_parts" ) ) {
        // Brand new, no HP stored
        return 1;
    }
    std::istringstream veh_data;
    const auto data = it.get_var( "folding_bicycle_parts" );
    veh_data.str( data );
    if( !data.empty() && data[0] >= '0' && data[0] <= '9' ) {
        // starts with a digit -> old format
        for( const vpart_reference &vpr : veh->get_all_parts() ) {
            int tmp;
            veh_data >> tmp;
            veh->set_hp( vpr.part(), tmp, true, it.degradation() );
        }
    } else {
        try {
            JsonIn json( veh_data );
            // Load parts into a temporary vector to not override
            // cached values (like precalc, passenger_id, ...)
            std::vector<vehicle_part> parts;
            json.read( parts );
            for( size_t i = 0; i < parts.size() && i < static_cast<size_t>( veh->part_count() ); i++ ) {
                const vehicle_part &src = parts[i];
                vehicle_part &dst = veh->part( i );
                // and now only copy values, that are
                // expected to be consistent.
                veh->set_hp( dst, src.hp(), true, it.degradation() );
                dst.blood = src.blood;
                // door state/amount of fuel/direction of headlight
                dst.ammo_set( src.ammo_current(), src.ammo_remaining() );
                dst.flags = src.flags;
            }
        } catch( const JsonError &e ) {
            debugmsg( "Error restoring vehicle: %s", e.c_str() );
        }
    }
    return 1;
}

std::unique_ptr<iuse_actor> consume_drug_iuse::clone() const
{
    return std::make_unique<consume_drug_iuse>( *this );
}

static effect_data load_effect_data( const JsonObject &e )
{
    time_duration time;
    if( e.has_string( "duration" ) ) {
        time = read_from_json_string<time_duration>( e.get_member( "duration" ), time_duration::units );
    } else {
        time = time_duration::from_turns( e.get_int( "duration", 0 ) );
    }
    return effect_data( efftype_id( e.get_string( "id" ) ), time,
                        bodypart_id( e.get_string( "bp", "bp_null" ) ), e.get_bool( "permanent", false ) );
}

void consume_drug_iuse::load( const JsonObject &obj )
{
    obj.read( "activation_message", activation_message );
    obj.read( "charges_needed", charges_needed );
    obj.read( "tools_needed", tools_needed );

    if( obj.has_array( "effects" ) ) {
        for( const JsonObject e : obj.get_array( "effects" ) ) {
            effects.push_back( load_effect_data( e ) );
        }
    }
    optional( obj, false, "damage_over_time", damage_over_time );
    obj.read( "stat_adjustments", stat_adjustments );
    obj.read( "fields_produced", fields_produced );
    obj.read( "moves", moves );

    for( JsonArray vit : obj.get_array( "vitamins" ) ) {
        int lo = vit.get_int( 1 );
        int hi = vit.size() >= 3 ? vit.get_int( 2 ) : lo;
        vitamins.emplace( vitamin_id( vit.get_string( 0 ) ), std::make_pair( lo, hi ) );
    }

    used_up_item = obj.get_string( "used_up_item", used_up_item );

}

void consume_drug_iuse::info( const item &, std::vector<iteminfo> &dump ) const
{
    const std::string vits = enumerate_as_string( vitamins.begin(), vitamins.end(),
    []( const decltype( vitamins )::value_type & v ) {
        const time_duration rate = get_player_character().vitamin_rate( v.first );
        if( rate <= 0_turns ) {
            return std::string();
        }
        const int lo = static_cast<int>( v.second.first  * rate / 1_days * 100 );
        const int hi = static_cast<int>( v.second.second * rate / 1_days * 100 );

        return string_format( lo == hi ? "%s (%i%%)" : "%s (%i-%i%%)", v.first.obj().name(), lo,
                              hi );
    } );

    if( !vits.empty() ) {
        dump.emplace_back( "TOOL", _( "Vitamins (RDA): " ), vits );
    }

    if( tools_needed.count( itype_syringe ) ) {
        dump.emplace_back( "TOOL", _( "You need a <info>syringe</info> to inject this drug." ) );
    }
}

cata::optional<int> consume_drug_iuse::use( Character &p, item &it, bool, const tripoint & ) const
{
    auto need_these = tools_needed;

    // Check prerequisites first.
    for( const auto &tool : need_these ) {
        // Amount == -1 means need one, but don't consume it.
        if( !p.has_amount( tool.first, 1 ) ) {
            p.add_msg_player_or_say( _( "You need %1$s to consume %2$s!" ),
                                     _( "I need a %1$s to consume %2$s!" ),
                                     item::nname( tool.first ),
                                     it.type_name( 1 ) );
            return -1;
        }
    }
    for( const auto &consumable : charges_needed ) {
        // Amount == -1 means need one, but don't consume it.
        if( !p.has_charges( consumable.first, ( consumable.second == -1 ) ?
                            1 : consumable.second ) ) {
            p.add_msg_player_or_say( _( "You need %1$s to consume %2$s!" ),
                                     _( "I need a %1$s to consume %2$s!" ),
                                     item::nname( consumable.first ),
                                     it.type_name( 1 ) );
            return -1;
        }
    }
    // Apply the various effects.
    for( const effect_data &eff : effects ) {
        time_duration dur = eff.duration;
        if( p.has_trait( trait_TOLERANCE ) ) {
            dur *= .8;
        } else if( p.has_trait( trait_LIGHTWEIGHT ) ) {
            dur *= 1.2;
        }
        p.add_effect( eff.id, dur, eff.bp, eff.permanent );
    }
    //Apply the various damage_over_time
    for( const damage_over_time_data &Dot : damage_over_time ) {
        p.add_damage_over_time( Dot );
    }
    for( const auto &stat_adjustment : stat_adjustments ) {
        p.mod_stat( stat_adjustment.first, stat_adjustment.second );
    }
    map &here = get_map();
    for( const auto &field : fields_produced ) {
        const field_type_id fid = field_type_id( field.first );
        for( int i = 0; i < 3; i++ ) {
            here.add_field( {p.posx() + static_cast<int>( rng( -2, 2 ) ), p.posy() + static_cast<int>( rng( -2, 2 ) ), p.posz()},
                            fid,
                            field.second );
        }
    }

    for( const auto &v : vitamins ) {
        p.vitamin_mod( v.first, rng( v.second.first, v.second.second ) );
    }

    // Output message.
    p.add_msg_if_player( activation_message.translated(), it.type_name( 1 ) );
    // Consume charges.
    for( const auto &consumable : charges_needed ) {
        if( consumable.second != -1 ) {
            p.use_charges( consumable.first, consumable.second );
        }
    }

    if( !used_up_item.empty() ) {
        item used_up( used_up_item, it.birthday() );
        p.i_add_or_drop( used_up );
    }

    p.moves -= moves;
    return it.type->charges_to_use();
}

std::unique_ptr<iuse_actor> delayed_transform_iuse::clone() const
{
    return std::make_unique<delayed_transform_iuse>( *this );
}

void delayed_transform_iuse::load( const JsonObject &obj )
{
    iuse_transform::load( obj );
    obj.get_member( "not_ready_msg" ).read( not_ready_msg );
    transform_age = obj.get_int( "transform_age" );
}

int delayed_transform_iuse::time_to_do( const item &it ) const
{
    // TODO: change return type to time_duration
    return transform_age - to_turns<int>( it.age() );
}

cata::optional<int> delayed_transform_iuse::use( Character &p, item &it, bool t,
        const tripoint &pos ) const
{
    if( time_to_do( it ) > 0 ) {
        p.add_msg_if_player( m_info, "%s", not_ready_msg );
        return cata::nullopt;
    }
    return iuse_transform::use( p, it, t, pos );
}

std::unique_ptr<iuse_actor> place_monster_iuse::clone() const
{
    return std::make_unique<place_monster_iuse>( *this );
}

void place_monster_iuse::load( const JsonObject &obj )
{
    mtypeid = mtype_id( obj.get_string( "monster_id" ) );
    obj.read( "friendly_msg", friendly_msg );
    obj.read( "hostile_msg", hostile_msg );
    obj.read( "difficulty", difficulty );
    obj.read( "moves", moves );
    obj.read( "place_randomly", place_randomly );
    obj.read( "is_pet", is_pet );
    obj.read( "need_charges", need_charges );
    need_charges = std::max( need_charges, 0 );

    if( obj.has_array( "skills" ) ) {
        JsonArray skills_ja = obj.get_array( "skills" );
        for( JsonValue s : skills_ja ) {
            skills.emplace( skill_id( s.get_string() ) );
        }
    }
}

cata::optional<int> place_monster_iuse::use( Character &p, item &it, bool, const tripoint & ) const
{
    if( it.ammo_remaining() < need_charges ) {
        p.add_msg_if_player( m_info, _( "This requires %d charges to activate." ), need_charges );
        return cata::nullopt;
    }

    shared_ptr_fast<monster> newmon_ptr = make_shared_fast<monster>( mtypeid );
    monster &newmon = *newmon_ptr;
    newmon.init_from_item( it );
    if( place_randomly ) {
        // place_critter_around returns the same pointer as its parameter (or null)
        if( !g->place_critter_around( newmon_ptr, p.pos(), 1 ) ) {
            p.add_msg_if_player( m_info, _( "There is no adjacent square to release the %s in!" ),
                                 newmon.name() );
            return cata::nullopt;
        }
    } else {
        const std::string query = string_format( _( "Place the %s where?" ), newmon.name() );
        const cata::optional<tripoint> pnt_ = choose_adjacent( query );
        if( !pnt_ ) {
            return cata::nullopt;
        }
        // place_critter_at returns the same pointer as its parameter (or null)
        if( !g->place_critter_at( newmon_ptr, *pnt_ ) ) {
            p.add_msg_if_player( m_info, _( "You cannot place a %s there." ), newmon.name() );
            return cata::nullopt;
        }
    }
    p.moves -= moves;

    newmon.ammo = newmon.type->starting_ammo;
    if( !newmon.has_flag( MF_INTERIOR_AMMO ) ) {
        for( std::pair<const itype_id, int> &amdef : newmon.ammo ) {
            item ammo_item( amdef.first, calendar::turn_zero );
            const int available = p.charges_of( amdef.first );
            if( available == 0 ) {
                amdef.second = 0;
                p.add_msg_if_player( m_info,
                                     _( "If you had standard factory-built %1$s bullets, you could load the %2$s." ),
                                     ammo_item.type_name( 2 ), newmon.name() );
                continue;
            }
            // Don't load more than the default from the monster definition.
            ammo_item.charges = std::min( available, amdef.second );
            p.use_charges( amdef.first, ammo_item.charges );
            //~ First %s is the ammo item (with plural form and count included), second is the monster name
            p.add_msg_if_player( n_gettext( "You load %1$d x %2$s round into the %3$s.",
                                            "You load %1$d x %2$s rounds into the %3$s.", ammo_item.charges ),
                                 ammo_item.charges, ammo_item.type_name( ammo_item.charges ),
                                 newmon.name() );
            amdef.second = ammo_item.charges;
        }
    }

    int skill_offset = 0;
    for( const skill_id &sk : skills ) {
        skill_offset += p.get_skill_level( sk ) / 2;
    }
    /** @EFFECT_INT increases chance of a placed turret being friendly */
    if( rng( 0, p.int_cur / 2 ) + skill_offset < rng( 0, difficulty ) ) {
        if( hostile_msg.empty() ) {
            p.add_msg_if_player( m_bad, _( "You deploy the %s wrong.  It is hostile!" ), newmon.name() );
        } else {
            p.add_msg_if_player( m_bad, "%s", hostile_msg );
        }
    } else {
        if( friendly_msg.empty() ) {
            p.add_msg_if_player( m_warning, _( "You deploy the %s." ), newmon.name() );
        } else {
            p.add_msg_if_player( m_warning, "%s", friendly_msg );
        }
        newmon.friendly = -1;
        if( is_pet ) {
            newmon.add_effect( effect_pet, 1_turns, true );
        }
    }
    return 1;
}

std::unique_ptr<iuse_actor> place_npc_iuse::clone() const
{
    return std::make_unique<place_npc_iuse>( *this );
}

void place_npc_iuse::load( const JsonObject &obj )
{
    npc_class_id = string_id<npc_template>( obj.get_string( "npc_class_id" ) );
    obj.read( "summon_msg", summon_msg );
    obj.read( "moves", moves );
    obj.read( "place_randomly", place_randomly );
}

cata::optional<int> place_npc_iuse::use( Character &p, item &, bool, const tripoint & ) const
{
    map &here = get_map();
    const tripoint_range<tripoint> target_range = place_randomly ?
            points_in_radius( p.pos(), radius ) :
            points_in_radius( choose_adjacent( _( "Place npc where?" ) ).value_or( p.pos() ), 0 );

    const cata::optional<tripoint> target_pos =
    random_point( target_range, [&here]( const tripoint & t ) {
        return here.passable( t ) && here.has_floor_or_support( t ) &&
               !get_creature_tracker().creature_at( t );
    } );

    if( !target_pos.has_value() ) {
        p.add_msg_if_player( m_info, _( "There is no square to spawn npc in!" ) );
        return cata::nullopt;
    }

    here.place_npc( target_pos.value().xy(), npc_class_id );
    p.mod_moves( -moves );
    p.add_msg_if_player( m_info, "%s", summon_msg );
    return 1;
}

std::unique_ptr<iuse_actor> deploy_furn_actor::clone() const
{
    return std::make_unique<deploy_furn_actor>( *this );
}

void deploy_furn_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    std::vector<std::string> can_function_as;
    const furn_t &the_furn = furn_type.obj();
    const std::string furn_name = the_furn.name();

    if( the_furn.workbench ) {
        can_function_as.emplace_back( _( "a <info>crafting station</info>" ) );
    }
    if( the_furn.has_flag( ter_furn_flag::TFLAG_BUTCHER_EQ ) ) {
        can_function_as.emplace_back(
            _( "a place to hang <info>corpses for butchering</info>" ) );
    }
    if( the_furn.has_flag( ter_furn_flag::TFLAG_FLAT_SURF ) ) {
        can_function_as.emplace_back(
            _( "a flat surface to <info>butcher</info> onto or <info>eat meals</info> from" ) );
    }
    if( the_furn.has_flag( ter_furn_flag::TFLAG_CAN_SIT ) ) {
        can_function_as.emplace_back( _( "a place to <info>sit</info>" ) );
    }
    if( the_furn.has_flag( ter_furn_flag::TFLAG_HIDE_PLACE ) ) {
        can_function_as.emplace_back( _( "a place to <info>hide</info>" ) );
    }
    if( the_furn.has_flag( ter_furn_flag::TFLAG_FIRE_CONTAINER ) ) {
        can_function_as.emplace_back( _( "a safe place to <info>contain a fire</info>" ) );
    }
    if( the_furn.crafting_pseudo_item == itype_char_smoker ) {
        can_function_as.emplace_back( _( "a place to <info>smoke or dry food</info> for preservation" ) );
    }

    if( can_function_as.empty() ) {
        dump.emplace_back( "DESCRIPTION",
                           string_format( _( "Can be <info>activated</info> to deploy as furniture (<stat>%s</stat>)." ),
                                          furn_name ) );
    } else {
        std::string furn_usages = enumerate_as_string( can_function_as, enumeration_conjunction::or_ );
        dump.emplace_back( "DESCRIPTION",
                           string_format(
                               _( "Can be <info>activated</info> to deploy as furniture (<stat>%s</stat>), which can then be used as %s." ),
                               furn_name, furn_usages ) );
    }
}

void deploy_furn_actor::load( const JsonObject &obj )
{
    furn_type = furn_str_id( obj.get_string( "furn_type" ) );
}

cata::optional<int> deploy_furn_actor::use( Character &p, item &it, bool,
        const tripoint &pos ) const
{
    if( p.is_mounted() ) {
        p.add_msg_if_player( m_info, _( "You cannot do that while mounted." ) );
        return cata::nullopt;
    }
    tripoint pnt = pos;
    if( pos == p.pos() ) {
        if( const cata::optional<tripoint> pnt_ = choose_adjacent( _( "Deploy where?" ) ) ) {
            pnt = *pnt_;
        } else {
            return cata::nullopt;
        }
    }

    if( pnt == p.pos() ) {
        p.add_msg_if_player( m_info,
                             _( "You attempt to become one with the furniture.  It doesn't work." ) );
        return cata::nullopt;
    }

    map &here = get_map();
    optional_vpart_position veh_there = here.veh_at( pnt );
    if( veh_there.has_value() ) {
        // TODO: check for protrusion+short furniture, wheels+tiny furniture, NOCOLLIDE flag, etc.
        // and/or integrate furniture deployment with construction (which already seems to perform these checks sometimes?)
        p.add_msg_if_player( m_info, _( "The space under %s is too cramped to deploy a %s in." ),
                             veh_there.value().vehicle().disp_name(), it.tname() );
        return cata::nullopt;
    }

    // For example: dirt = 2, long grass = 3
    if( here.move_cost( pnt ) != 2 && here.move_cost( pnt ) != 3 ) {
        p.add_msg_if_player( m_info, _( "You can't deploy a %s there." ), it.tname() );
        return cata::nullopt;
    }

    if( here.has_furn( pnt ) ) {
        p.add_msg_if_player( m_info, _( "There is already furniture at that location." ) );
        return cata::nullopt;
    }
    if( here.has_items( pnt ) ) {
        p.add_msg_if_player( m_info, _( "Before deploying furniture, you need to clear the tile." ) );
        return cata::nullopt;
    }
    here.furn_set( pnt, furn_type );
    it.spill_contents( pnt );
    p.mod_moves( -to_moves<int>( 2_seconds ) );
    return it.type->charges_to_use() != 0 ? it.type->charges_to_use() : 1;
}

std::unique_ptr<iuse_actor> reveal_map_actor::clone() const
{
    return std::make_unique<reveal_map_actor>( *this );
}

void reveal_map_actor::load( const JsonObject &obj )
{
    radius = obj.get_int( "radius" );
    obj.get_member( "message" ).read( message );
    std::string ter;
    ot_match_type ter_match_type;
    for( const JsonValue entry : obj.get_array( "terrain" ) ) {
        if( entry.test_string() ) {
            ter = entry.get_string();
            ter_match_type = ot_match_type::contains;
        } else {
            JsonObject jo = entry.get_object();
            ter = jo.get_string( "om_terrain" );
            ter_match_type = jo.get_enum_value<ot_match_type>( "om_terrain_match_type",
                             ot_match_type::contains );
        }
        omt_types.emplace_back( ter, ter_match_type );
    }
}

void reveal_map_actor::reveal_targets( const tripoint_abs_omt &center,
                                       const std::pair<std::string, ot_match_type> &target,
                                       int reveal_distance ) const
{
    const auto places = overmap_buffer.find_all( center, target.first, radius, false,
                        target.second );
    for( const tripoint_abs_omt &place : places ) {
        overmap_buffer.reveal( place, reveal_distance );
    }
}

cata::optional<int> reveal_map_actor::use( Character &p, item &it, bool, const tripoint & ) const
{
    if( it.already_used_by_player( p ) ) {
        p.add_msg_if_player( _( "There isn't anything new on the %s." ), it.tname() );
        return cata::nullopt;
    } else if( p.fine_detail_vision_mod() > 4 ) {
        p.add_msg_if_player( _( "It's too dark to read." ) );
        return cata::nullopt;
    }
    const tripoint_abs_omt center( it.get_var( "reveal_map_center_omt",
                                   p.global_omt_location().raw() ) );
    for( const auto &omt : omt_types ) {
        for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
            reveal_targets( tripoint_abs_omt( center.xy(), z ), omt, 0 );
        }
    }
    if( !message.empty() ) {
        p.add_msg_if_player( m_good, "%s", message );
    }
    it.mark_as_used_by_player( p );
    return 0;
}

void firestarter_actor::load( const JsonObject &obj )
{
    moves_cost_fast = obj.get_int( "moves", moves_cost_fast );
    moves_cost_slow = obj.get_int( "moves_slow", moves_cost_fast * 10 );
    need_sunlight = obj.get_bool( "need_sunlight", false );
}

std::unique_ptr<iuse_actor> firestarter_actor::clone() const
{
    return std::make_unique<firestarter_actor>( *this );
}

bool firestarter_actor::prep_firestarter_use( const Character &p, tripoint &pos )
{
    // checks for fuel are handled by use and the activity, not here
    if( pos == p.pos() ) {
        if( const cata::optional<tripoint> pnt_ = choose_adjacent( _( "Light where?" ) ) ) {
            pos = *pnt_;
        } else {
            return false;
        }
    }
    if( pos == p.pos() ) {
        p.add_msg_if_player( m_info, _( "You would set yourself on fire." ) );
        p.add_msg_if_player( _( "But you're already smokin' hot." ) );
        return false;
    }
    map &here = get_map();
    if( here.get_field( pos, fd_fire ) ) {
        // check if there's already a fire
        p.add_msg_if_player( m_info, _( "There is already a fire." ) );
        return false;
    }
    // check if there's a fire fuel source spot
    bool target_is_firewood = false;
    if( here.tr_at( pos ).id == tr_firewood_source ) {
        target_is_firewood = true;
    } else {
        zone_manager &mgr = zone_manager::get_manager();
        auto zones = mgr.get_zones( zone_type_SOURCE_FIREWOOD, here.getglobal( pos ) );
        if( !zones.empty() ) {
            target_is_firewood = true;
        }
    }
    if( target_is_firewood ) {
        if( !query_yn( _( "Do you really want to burn your firewood source?" ) ) ) {
            return false;
        }
    }
    // Check for a brazier.
    bool has_unactivated_brazier = false;
    for( const item &i : here.i_at( pos ) ) {
        if( i.typeId() == itype_brazier ) {
            has_unactivated_brazier = true;
        }
    }
    return !has_unactivated_brazier ||
           query_yn(
               _( "There's a brazier there but you haven't set it up to contain the fire.  Continue?" ) );
}

void firestarter_actor::resolve_firestarter_use( Character &p, const tripoint &pos )
{
    if( get_map().add_field( pos, fd_fire, 1, 10_minutes ) ) {
        if( !p.has_trait( trait_PYROMANIA ) ) {
            p.add_msg_if_player( _( "You successfully light a fire." ) );
        } else {
            if( one_in( 4 ) ) {
                p.add_msg_if_player( m_mixed,
                                     _( "You light a fire, but it isn't enough.  You need to light more." ) );
            } else {
                p.add_msg_if_player( m_good, _( "You happily light a fire." ) );
                p.add_morale( MORALE_PYROMANIA_STARTFIRE, 5, 10, 6_hours, 4_hours );
                p.rem_morale( MORALE_PYROMANIA_NOFIRE );
            }
        }
    }
}

ret_val<bool> firestarter_actor::can_use( const Character &p, const item &it, bool,
        const tripoint & ) const
{
    if( p.is_underwater() ) {
        return ret_val<bool>::make_failure( _( "You can't do that while underwater." ) );
    }

    if( !it.ammo_sufficient( &p ) ) {
        return ret_val<bool>::make_failure( _( "This tool doesn't have enough charges." ) );
    }

    if( need_sunlight && light_mod( p.pos() ) <= 0.0f ) {
        return ret_val<bool>::make_failure( _( "You need direct sunlight to light a fire with this." ) );
    }

    return ret_val<bool>::make_success();
}

float firestarter_actor::light_mod( const tripoint &pos ) const
{
    if( !need_sunlight ) {
        return 1.0f;
    }

    const float light_level = g->natural_light_level( pos.z );
    if( get_weather().weather_id->sun_intensity >= sun_intensity_type::normal &&
        light_level >= 60.0f && !get_map().has_flag( ter_furn_flag::TFLAG_INDOORS, pos ) ) {
        return std::pow( light_level / 80.0f, 8 );
    }

    return 0.0f;
}

int firestarter_actor::moves_cost_by_fuel( const tripoint &pos ) const
{
    map &here = get_map();
    if( here.flammable_items_at( pos, 100 ) ) {
        return moves_cost_fast;
    }

    if( here.flammable_items_at( pos, 10 ) ) {
        return ( moves_cost_slow + moves_cost_fast ) / 2;
    }

    return moves_cost_slow;
}

cata::optional<int> firestarter_actor::use( Character &p, item &it, bool t,
        const tripoint &spos ) const
{
    if( t ) {
        return cata::nullopt;
    }

    tripoint pos = spos;
    float light = light_mod( p.pos() );
    if( !prep_firestarter_use( p, pos ) ) {
        return cata::nullopt;
    }

    double skill_level = p.get_skill_level( skill_survival );
    /** @EFFECT_SURVIVAL speeds up fire starting */
    float moves_modifier = std::pow( 0.8, std::min( 5.0, skill_level ) );
    const int moves_base = moves_cost_by_fuel( pos );
    const double moves_per_turn = to_moves<double>( 1_turns );
    const int min_moves = std::min<int>(
                              moves_base, std::sqrt( 1 + moves_base / moves_per_turn ) * moves_per_turn );
    const int moves = std::max<int>( min_moves, moves_base * moves_modifier ) / light;
    if( moves > to_moves<int>( 1_minutes ) ) {
        // If more than 1 minute, inform the player
        const int minutes = moves / to_moves<int>( 1_minutes );
        p.add_msg_if_player( m_info, need_sunlight ?
                             n_gettext( "If the current weather holds, it will take around %d minute to light a fire.",
                                        "If the current weather holds, it will take around %d minutes to light a fire.", minutes ) :
                             n_gettext( "At your skill level, it will take around %d minute to light a fire.",
                                        "At your skill level, it will take around %d minutes to light a fire.", minutes ),
                             minutes );
    } else if( moves < to_moves<int>( 2_turns ) && get_map().is_flammable( pos ) ) {
        // If less than 2 turns, don't start a long action
        resolve_firestarter_use( p, pos );
        p.mod_moves( -moves );
        return it.type->charges_to_use();
    }

    // skill gains are handled by the activity, but stored here in the index field
    const int potential_skill_gain =
        moves_modifier + moves_cost_fast / 100.0 + 2;
    p.assign_activity( ACT_START_FIRE, moves, potential_skill_gain,
                       0, it.tname() );
    p.activity.targets.emplace_back( p, &it );
    p.activity.values.push_back( g->natural_light_level( pos.z ) );
    p.activity.placement = pos;
    // charges to use are handled by the activity
    return 0;
}

void salvage_actor::load( const JsonObject &obj )
{
    assign( obj, "cost", cost );
    assign( obj, "moves_per_part", moves_per_part );

    if( obj.has_array( "material_whitelist" ) ) {
        material_whitelist.clear();
        assign( obj, "material_whitelist", material_whitelist );
    }
}

std::unique_ptr<iuse_actor> salvage_actor::clone() const
{
    return std::make_unique<salvage_actor>( *this );
}

cata::optional<int> salvage_actor::use( Character &p, item &it, bool t, const tripoint & ) const
{
    if( t ) {
        return cata::nullopt;
    }

    item_location item_loc = game_menus::inv::salvage( p, this );
    if( !item_loc ) {
        add_msg( _( "Never mind." ) );
        return cata::nullopt;
    }

    if( !try_to_cut_up( p, *item_loc.get_item() ) ) {
        // Messages should have already been displayed.
        return cata::nullopt;
    }

    return cut_up( p, it, item_loc );
}

// Helper to visit instances of all the sub-materials of an item.
static void visit_salvage_products( const item &it, std::function<void( const item & )> func )
{
    for( const auto &material : it.made_of() ) {
        if( const cata::optional<itype_id> id = material.first->salvaged_into() ) {
            item exemplar( *id );
            func( exemplar );
        }
    }
}

// Helper to find smallest sub-component of an item.
static units::mass minimal_weight_to_cut( const item &it )
{
    units::mass min_weight = units::mass_max;
    visit_salvage_products( it, [&min_weight]( const item & exemplar ) {
        min_weight = std::min( min_weight, exemplar.weight() );
    } );
    return min_weight;
}

int salvage_actor::time_to_cut_up( const item &it ) const
{
    units::mass total_material_weight;
    int num_materials = 0;
    visit_salvage_products( it, [&total_material_weight, &num_materials]( const item & exemplar ) {
        total_material_weight += exemplar.weight();
        num_materials += 1;
    } );
    if( num_materials == 0 ) {
        return 0;
    }
    units::mass average_material_weight = total_material_weight / num_materials;
    int count = it.weight() / average_material_weight;
    return moves_per_part * count;
}

bool salvage_actor::valid_to_cut_up( const item &it ) const
{
    if( it.is_null() ) {
        return false;
    }
    // There must be some historical significance to these items.
    if( !it.is_salvageable() ) {
        return false;
    }
    if( !it.only_made_of( material_whitelist ) ) {
        return false;
    }
    if( !it.empty() ) {
        return false;
    }
    if( it.weight() < minimal_weight_to_cut( it ) ) {
        return false;
    }

    return true;
}

// it here is the item that is a candidate for being chopped up.
// This is the former valid_to_cut_up with all the messages and queries
bool salvage_actor::try_to_cut_up( Character &p, item &it ) const
{
    bool isWearing = p.is_worn( it );

    if( it.is_null() ) {
        add_msg( m_info, _( "You do not have that item." ) );
        return false;
    }
    // There must be some historical significance to these items.
    if( !it.is_salvageable() ) {
        add_msg( m_info, _( "Can't salvage anything from %s." ), it.tname() );
        if( it.is_disassemblable() ) {
            add_msg( m_info, _( "Try disassembling the %s instead." ), it.tname() );
        }
        return false;
    }

    if( !it.only_made_of( material_whitelist ) ) {
        add_msg( m_info, _( "The %s is made of material that cannot be cut up." ), it.tname() );
        return false;
    }
    if( !it.empty() ) {
        add_msg( m_info, _( "Please empty the %s before cutting it up." ), it.tname() );
        return false;
    }
    if( it.weight() < minimal_weight_to_cut( it ) ) {
        add_msg( m_info, _( "The %s is too small to salvage material from." ), it.tname() );
        return false;
    }
    // Softer warnings at the end so we don't ask permission and then tell them no.
    if( &it == &p.get_wielded_item() ) {
        if( !query_yn( _( "You are wielding that, are you sure?" ) ) ) {
            return false;
        }
    } else if( isWearing ) {
        if( !query_yn( _( "You're wearing that, are you sure?" ) ) ) {
            return false;
        }
    }

    return true;
}

// function returns charges from it during the cutting process of the *cut.
// it cuts
// cut gets cut
int salvage_actor::cut_up( Character &p, item &it, item_location &cut ) const
{
    const std::map<material_id, int> cut_material_components = cut.get_item()->made_of();
    const bool filthy = cut.get_item()->is_filthy();
    float remaining_weight = 1;

    // Keep the codes below, use it to calculate component loss

    // Chance of us losing a material component to entropy.
    /** @EFFECT_FABRICATION reduces chance of losing components when cutting items up */
    int entropy_threshold = std::max( 5, 10 - p.get_skill_level( skill_fabrication ) );

    // What materials do we salvage (ids and counts).
    std::map<itype_id, int> materials_salvaged;

    // Final just in case check (that perhaps was not done elsewhere);
    if( cut.get_item() == &it ) {
        add_msg( m_info, _( "You can not cut the %s with itself." ), it.tname() );
        return 0;
    }
    if( !cut.get_item()->empty() ) {
        // Should have been ensured by try_to_cut_up
        debugmsg( "tried to cut a non-empty item %s", cut.get_item()->tname() );
        return 0;
    }

    // Not much practice, and you won't get very far ripping things up.
    p.practice( skill_fabrication, rng( 0, 5 ), 1 );

    // Higher fabrication, less chance of entropy, but still a chance.
    if( rng( 1, 10 ) <= entropy_threshold ) {
        remaining_weight *= 0.99;
    }
    // Fail dex roll, potentially lose more parts.
    /** @EFFECT_DEX randomly reduces component loss when cutting items up */
    if( dice( 3, 4 ) > p.dex_cur ) {
        remaining_weight *= 0.95;
    }
    // If more than 1 material component can still be salvaged,
    // chance of losing more components if the item is damaged.
    // If the item being cut is not damaged, no additional losses will be incurred.
    if( cut.get_item()->damage() > 0 ) {
        float component_success_chance = std::min( std::pow( 0.8, cut.get_item()->damage_level() ),
                                         1.0 );
        remaining_weight *= component_success_chance;
    }

    std::vector<item> stack{ *cut.get_item() }; /* working stack */
    std::map<itype_id, int> salvage_to; /* outcome */
    std::map<material_id, units::mass> mat_to_weight;
    // Decompose the item into irreducible parts
    while( !stack.empty() ) {
        item temp = stack.back();
        stack.pop_back();

        // If it is one of the basic components, add it into the list
        if( temp.type->is_basic_component() ) {
            salvage_to[temp.typeId()] ++;
            continue;
        }
        // Discard invalid component
        std::set<material_id> mat_set;
        for( std::pair<material_id, int> mat : cut_material_components ) {
            mat_set.insert( mat.first );
        }
        if( !temp.made_of_any( mat_set ) ) {
            continue;
        }
        //items count by charges should be even smaller than base materials
        if( !temp.is_salvageable() || temp.count_by_charges() ) {
            const float mat_total = temp.type->mat_portion_total == 0 ? 1 : temp.type->mat_portion_total;
            // non-salvageable items but made of appropriate material, disrtibute uniformly in to all materials
            for( const auto &type : temp.made_of() ) {
                mat_to_weight[type.first] += ( temp.weight() * remaining_weight / temp.made_of().size() ) *
                                             ( static_cast<float>( type.second ) / mat_total );
            }
            continue;
        }
        //check if there are components defined
        if( !temp.components.empty() ) {
            // push components into stack
            for( const item &iter : temp.components ) {
                stack.push_back( iter );
            }
            continue;
        }
        // No available components
        // Try to find an available recipe and "restore" its components
        recipe un_craft;
        auto iter = std::find_if( recipe_dict.begin(),
        recipe_dict.end(), [&]( const std::pair<const recipe_id, recipe> &curr ) {
            if( curr.second.obsolete || curr.second.result() != temp.typeId() ||
                curr.second.makes_amount() > 1 ) {
                return false;
            }
            units::mass weight = 0_gram;
            for( const auto &altercomps : curr.second.simple_requirements().get_components() ) {
                if( !altercomps.empty() && altercomps.front().type ) {
                    weight += ( altercomps.front().type->weight ) * altercomps.front().count;
                }
            }
            return weight <= temp.weight();
        } );
        // No crafting recipe available
        if( iter == recipe_dict.end() ) {
            // Check disassemble recipe too
            const float mat_total = temp.type->mat_portion_total == 0 ? 1 : temp.type->mat_portion_total;
            un_craft = recipe_dictionary::get_uncraft( temp.typeId() );
            if( un_craft.is_null() ) {
                // No recipes found, count weight and go next
                for( const auto &type : temp.made_of() ) {
                    mat_to_weight[type.first] += ( temp.weight() * remaining_weight / temp.made_of().size() ) *
                                                 ( static_cast<float>( type.second ) / mat_total );
                }
                continue;
            }
            // Found disassemble recipe, check if it is valid
            units::mass weight = 0_gram;
            for( const auto &altercomps : un_craft.simple_requirements().get_components() ) {
                weight += ( altercomps.front().type->weight ) * altercomps.front().count;
            }
            if( weight > temp.weight() ) {
                // Bad disassemble recipe.  Count weight and go next
                for( const auto &type : temp.made_of() ) {
                    mat_to_weight[type.first] += ( temp.weight() * remaining_weight / temp.made_of().size() ) *
                                                 ( static_cast<float>( type.second ) / mat_total );
                }
                continue;
            }
        } else {
            //take the chosen crafting recipe
            un_craft = iter->second;
        }
        // If we get here it means we found a recipe
        const requirement_data requirements = un_craft.simple_requirements();
        // find default components set from recipe, push them into stack
        for( const auto &altercomps : requirements.get_components() ) {
            const item_comp &comp = altercomps.front();
            // if count by charges
            if( comp.type->count_by_charges() ) {
                stack.emplace_back( comp.type, calendar::turn, comp.count );
            } else {
                for( int i = 0; i < comp.count; i++ ) {
                    stack.emplace_back( comp.type, calendar::turn );
                }
            }
        }
    }

    // Apply propotional item loss.
    for( auto &iter : salvage_to ) {
        iter.second *= remaining_weight;
    }
    // Item loss for weight was applied before(only round once).
    for( const auto &iter : mat_to_weight ) {
        if( const cata::optional<itype_id> id = iter.first->salvaged_into() ) {
            salvage_to[*id] += ( iter.second / id->obj().weight );
        }
    }

    add_msg( m_info, _( "You try to salvage materials from the %s." ),
             cut.get_item()->tname() );

    item_location::type cut_type = cut.where();
    tripoint pos = cut.position();

    // Clean up before removing the item.
    remove_ammo( *cut.get_item(), p );
    // Original item has been consumed.
    cut.remove_item();
    // Force an encumbrance update in case they were wearing that item.
    p.calc_encumbrance();

    map &here = get_map();
    for( const auto &salvaged : salvage_to ) {
        itype_id mat_name = salvaged.first;
        int amount = salvaged.second;
        item result( mat_name, calendar::turn );
        if( amount > 0 ) {
            // Time based on number of components.
            p.moves -= moves_per_part;
            if( result.count_by_charges() ) {
                result.charges = amount;
                amount = 1;
            }
            add_msg( m_good, n_gettext( "Salvaged %1$i %2$s.", "Salvaged %1$i %2$s.", amount ),
                     amount, result.display_name( amount ) );
            if( filthy ) {
                result.set_flag( flag_FILTHY );
            }
            if( cut_type == item_location::type::character ) {
                p.i_add_or_drop( result, amount );
            } else {
                for( int i = 0; i < amount; i++ ) {
                    here.add_item_or_charges( pos, result );
                }
            }
        } else {
            add_msg( m_bad, _( "Could not salvage a %s." ), result.display_name() );
        }
    }
    // No matter what, cutting has been done by the time we get here.
    return cost >= 0 ? cost : it.ammo_required();
}

void inscribe_actor::load( const JsonObject &obj )
{
    assign( obj, "cost", cost );
    assign( obj, "on_items", on_items );
    assign( obj, "on_terrain", on_terrain );
    assign( obj, "material_restricted", material_restricted );

    if( obj.has_array( "material_whitelist" ) ) {
        material_whitelist.clear();
        assign( obj, "material_whitelist", material_whitelist );
    }

    assign( obj, "verb", verb );
    assign( obj, "gerund", gerund );

    if( !on_items && !on_terrain ) {
        obj.throw_error(
            R"(Tried to create an useless inscribe_actor, at least on of "on_items" or "on_terrain" should be true)" );
    }
}

std::unique_ptr<iuse_actor> inscribe_actor::clone() const
{
    return std::make_unique<inscribe_actor>( *this );
}

bool inscribe_actor::item_inscription( item &tool, item &cut ) const
{
    if( !cut.made_of( phase_id::SOLID ) ) {
        add_msg( m_info, _( "You can't inscribe an item that isn't solid!" ) );
        return false;
    }

    if( material_restricted && !cut.made_of_any( material_whitelist ) ) {
        std::wstring lower_verb = utf8_to_wstr( verb.translated() );
        std::transform( lower_verb.begin(), lower_verb.end(), lower_verb.begin(), towlower );
        add_msg( m_info, _( "You can't %1$s %2$s because of the material it is made of." ),
                 wstr_to_utf8( lower_verb ), cut.display_name() );
        return false;
    }

    enum inscription_type {
        INSCRIPTION_LABEL,
        INSCRIPTION_NOTE,
    };

    uilist menu;
    menu.text = string_format( _( "%s meaning?" ), verb );
    menu.addentry( INSCRIPTION_LABEL, true, -1, _( "It's a label" ) );
    menu.addentry( INSCRIPTION_NOTE, true, -1, _( "It's a note" ) );
    menu.query();

    std::string carving;
    std::string carving_tool;
    switch( menu.ret ) {
        case INSCRIPTION_LABEL:
            carving = "item_label";
            carving_tool = "item_label_tool";
            break;
        case INSCRIPTION_NOTE:
            carving = "item_note";
            carving_tool = "item_note_tool";
            break;
        default:
            return false;
    }

    const bool hasnote = cut.has_var( carving );
    std::string messageprefix = ( hasnote ? _( "(To delete, clear the text and confirm)\n" ) : "" ) +
                                //~ %1$s: gerund (e.g. carved), %2$s: item name
                                string_format( pgettext( "carving", "%1$s on the %2$s is: " ),
                                        gerund, cut.type_name() );

    string_input_popup popup;
    popup.title( string_format( _( "%s what?" ), verb ) )
    .width( 64 )
    .text( hasnote ? cut.get_var( carving ) : std::string() )
    .description( messageprefix )
    .identifier( "inscribe_item" )
    .max_length( 128 )
    .query();
    if( popup.canceled() ) {
        return false;
    }
    const std::string message = popup.text();
    if( message.empty() ) {
        cut.erase_var( carving );
        cut.erase_var( carving_tool );
    } else {
        cut.set_var( carving, message );
        cut.set_var( carving_tool, tool.typeId().str() );
    }

    return true;
}

cata::optional<int> inscribe_actor::use( Character &p, item &it, bool t, const tripoint & ) const
{
    if( t ) {
        return cata::nullopt;
    }

    int choice = INT_MAX;
    if( on_terrain && on_items ) {
        uilist imenu;
        imenu.text = string_format( _( "%s on what?" ), verb );
        imenu.addentry( 0, true, MENU_AUTOASSIGN, _( "The terrain" ) );
        imenu.addentry( 1, true, MENU_AUTOASSIGN, _( "An item" ) );
        imenu.query();
        choice = imenu.ret;
    } else if( on_terrain ) {
        choice = 0;
    } else {
        choice = 1;
    }

    if( choice < 0 || choice > 1 ) {
        return cata::nullopt;
    }

    if( choice == 0 ) {
        const cata::optional<tripoint> dest_ = choose_adjacent( _( "Write where?" ) );
        if( !dest_ ) {
            return cata::nullopt;
        }
        return iuse::handle_ground_graffiti( p, &it, string_format( _( "%s what?" ), verb ),
                                             dest_.value() );
    }

    item_location loc = game_menus::inv::titled_menu( get_avatar(), _( "Inscribe which item?" ) );
    if( !loc ) {
        p.add_msg_if_player( m_info, _( "Never mind." ) );
        return cata::nullopt;
    }
    item &cut = *loc;
    if( &cut == &it ) {
        p.add_msg_if_player( _( "You try to bend your %s, but fail." ), it.tname() );
        return 0;
    }
    // inscribe_item returns false if the action fails or is canceled somehow.

    if( item_inscription( it, cut ) ) {
        return cost >= 0 ? cost : it.ammo_required();
    }

    return cata::nullopt;
}

void cauterize_actor::load( const JsonObject &obj )
{
    assign( obj, "cost", cost );
    assign( obj, "flame", flame );
}

std::unique_ptr<iuse_actor> cauterize_actor::clone() const
{
    return std::make_unique<cauterize_actor>( *this );
}

static heal_actor prepare_dummy()
{
    heal_actor dummy;
    dummy.limb_power = -2;
    dummy.head_power = -2;
    dummy.torso_power = -2;
    dummy.bleed = 25;
    dummy.bite = 0.5f;
    dummy.move_cost = 100;
    return dummy;
}

bool cauterize_actor::cauterize_effect( Character &p, item &it, bool force )
{
    // TODO: Make this less hacky
    static const heal_actor dummy = prepare_dummy();
    bodypart_id hpart = dummy.use_healing_item( p, p, it, force );
    if( hpart != bodypart_id( "bp_null" ) ) {
        p.add_msg_if_player( m_neutral, _( "You cauterize yourself." ) );
        if( !p.has_trait( trait_NOPAIN ) ) {
            p.mod_pain( 15 );
            p.add_msg_if_player( m_bad, _( "It hurts like hell!" ) );
        } else {
            p.add_msg_if_player( m_neutral, _( "It itches a little." ) );
        }
        if( p.has_effect( effect_bleed, hpart ) ) {
            p.add_msg_if_player( m_bad, _( "Bleeding has not stopped completely!" ) );
        }
        if( p.has_effect( effect_bite, hpart ) ) {
            p.add_effect( effect_bite, 260_minutes, hpart, true );
        }

        p.moves = 0;
        return true;
    }

    return false;
}

cata::optional<int> cauterize_actor::use( Character &p, item &it, bool t, const tripoint & ) const
{
    if( t ) {
        return cata::nullopt;
    }
    if( p.is_mounted() ) {
        p.add_msg_if_player( m_info, _( "You cannot do that while mounted." ) );
        return cata::nullopt;
    }
    bool has_disease = p.has_effect( effect_bite ) || p.has_effect( effect_bleed );
    bool did_cauterize = false;

    if( has_disease ) {
        did_cauterize = cauterize_effect( p, it, false );
    } else {
        const bool can_have_fun = p.has_trait( trait_MASOCHIST ) || p.has_trait( trait_MASOCHIST_MED ) ||
                                  p.has_trait( trait_CENOBITE );

        if( can_have_fun && query_yn( _( "Cauterize yourself for fun?" ) ) ) {
            did_cauterize = cauterize_effect( p, it, true );
        }
    }

    if( !did_cauterize ) {
        return cata::nullopt;
    }

    if( flame ) {
        p.use_charges( itype_fire, 4 );
        return 0;

    } else {
        return cost >= 0 ? cost : it.ammo_required();
    }
}

ret_val<bool> cauterize_actor::can_use( const Character &p, const item &it, bool,
                                        const tripoint & ) const
{
    if( !p.has_effect( effect_bite ) &&
        !p.has_effect( effect_bleed ) &&
        !p.has_trait( trait_MASOCHIST ) &&
        !p.has_trait( trait_MASOCHIST_MED ) &&
        !p.has_trait( trait_CENOBITE ) ) {

        return ret_val<bool>::make_failure(
                   _( "You are not bleeding or bitten, there is no need to cauterize yourself." ) );
    }
    if( p.is_mounted() ) {
        return ret_val<bool>::make_failure( _( "You cannot cauterize while mounted." ) );
    }

    if( flame ) {
        if( !p.has_charges( itype_fire, 4 ) ) {
            return ret_val<bool>::make_failure(
                       _( "You need a source of flame (4 charges worth) before you can cauterize yourself." ) );
        }
    } else {
        if( !it.ammo_sufficient( &p ) ) {
            return ret_val<bool>::make_failure( _( "You need at least %d charges to cauterize wounds." ),
                                                it.ammo_required() );
        }
    }

    if( p.is_underwater() ) {
        return ret_val<bool>::make_failure( _( "You can't do that while underwater." ) );
    }

    return ret_val<bool>::make_success();
}

void fireweapon_off_actor::load( const JsonObject &obj )
{
    obj.read( "target_id", target_id, true );
    obj.read( "success_message", success_message );
    obj.get_member( "lacks_fuel_message" ).read( lacks_fuel_message );
    obj.read( "failure_message", failure_message );
    noise               = obj.get_int( "noise", 0 );
    moves               = obj.get_int( "moves", 0 );
    success_chance      = obj.get_int( "success_chance", INT_MIN );
}

std::unique_ptr<iuse_actor> fireweapon_off_actor::clone() const
{
    return std::make_unique<fireweapon_off_actor>( *this );
}

cata::optional<int> fireweapon_off_actor::use( Character &p, item &it, bool t,
        const tripoint & ) const
{
    if( t ) {
        return cata::nullopt;
    }

    if( it.charges <= 0 ) {
        p.add_msg_if_player( "%s", lacks_fuel_message );
        return cata::nullopt;
    }

    p.moves -= moves;
    if( rng( 0, 10 ) - it.damage_level() > success_chance && !p.is_underwater() ) {
        if( noise > 0 ) {
            sounds::sound( p.pos(), noise, sounds::sound_t::combat, success_message );
        } else {
            p.add_msg_if_player( "%s", success_message );
        }

        it.convert( target_id );
        it.active = true;
    } else if( !failure_message.empty() ) {
        p.add_msg_if_player( m_bad, "%s", failure_message );
    }

    return it.type->charges_to_use();
}

ret_val<bool> fireweapon_off_actor::can_use( const Character &p, const item &it, bool,
        const tripoint & ) const
{
    if( it.charges < it.type->charges_to_use() ) {
        return ret_val<bool>::make_failure( _( "This tool doesn't have enough charges." ) );
    }

    if( p.is_underwater() ) {
        return ret_val<bool>::make_failure( _( "You can't do that while underwater." ) );
    }

    return ret_val<bool>::make_success();
}

void fireweapon_on_actor::load( const JsonObject &obj )
{
    obj.read( "noise_message", noise_message );
    obj.get_member( "voluntary_extinguish_message" ).read( voluntary_extinguish_message );
    obj.get_member( "charges_extinguish_message" ).read( charges_extinguish_message );
    obj.get_member( "water_extinguish_message" ).read( water_extinguish_message );
    noise                           = obj.get_int( "noise", 0 );
    noise_chance                    = obj.get_int( "noise_chance", 1 );
    auto_extinguish_chance          = obj.get_int( "auto_extinguish_chance", 0 );
    if( auto_extinguish_chance > 0 ) {
        obj.get_member( "auto_extinguish_message" ).read( auto_extinguish_message );
    }
}

std::unique_ptr<iuse_actor> fireweapon_on_actor::clone() const
{
    return std::make_unique<fireweapon_on_actor>( *this );
}

cata::optional<int> fireweapon_on_actor::use( Character &p, item &it, bool t,
        const tripoint & ) const
{
    bool extinguish = true;
    if( it.charges == 0 ) {
        p.add_msg_if_player( m_bad, "%s", charges_extinguish_message );
    } else if( p.is_underwater() ) {
        p.add_msg_if_player( m_bad, "%s", water_extinguish_message );
    } else if( auto_extinguish_chance > 0 && one_in( auto_extinguish_chance ) ) {
        p.add_msg_if_player( m_bad, "%s", auto_extinguish_message );
    } else if( !t ) {
        p.add_msg_if_player( "%s", voluntary_extinguish_message );
    } else {
        extinguish = false;
    }

    if( extinguish ) {
        it.deactivate( &p, false );

    } else if( one_in( noise_chance ) ) {
        if( noise > 0 ) {
            sounds::sound( p.pos(), noise, sounds::sound_t::combat, noise_message );
        } else {
            p.add_msg_if_player( "%s", noise_message );
        }
    }

    return it.type->charges_to_use();
}

void manualnoise_actor::load( const JsonObject &obj )
{
    obj.get_member( "no_charges_message" ).read( no_charges_message );
    obj.get_member( "use_message" ).read( use_message );
    obj.read( "noise_message", noise_message );
    noise_id            = obj.get_string( "noise_id", "misc" );
    noise_variant       = obj.get_string( "noise_variant", "default" );
    noise               = obj.get_int( "noise", 0 );
    moves               = obj.get_int( "moves", 0 );
}

std::unique_ptr<iuse_actor> manualnoise_actor::clone() const
{
    return std::make_unique<manualnoise_actor>( *this );
}

cata::optional<int> manualnoise_actor::use( Character &p, item &it, bool t, const tripoint & ) const
{
    if( t ) {
        return cata::nullopt;
    }
    if( it.type->charges_to_use() != 0 && it.charges < it.type->charges_to_use() ) {
        p.add_msg_if_player( "%s", no_charges_message );
        return cata::nullopt;
    }
    {
        p.moves -= moves;
        if( noise > 0 ) {
            sounds::sound( p.pos(), noise, sounds::sound_t::activity,
                           noise_message.empty() ? _( "Hsss" ) : noise_message.translated(), true, noise_id, noise_variant );
        }
        p.add_msg_if_player( "%s", use_message );
    }
    return it.type->charges_to_use();
}

ret_val<bool> manualnoise_actor::can_use( const Character &, const item &it, bool,
        const tripoint & ) const
{
    if( it.charges < it.type->charges_to_use() ) {
        return ret_val<bool>::make_failure( _( "This tool doesn't have enough charges." ) );
    }

    return ret_val<bool>::make_success();
}

std::unique_ptr<iuse_actor> musical_instrument_actor::clone() const
{
    return std::make_unique<musical_instrument_actor>( *this );
}

void musical_instrument_actor::load( const JsonObject &obj )
{
    speed_penalty = obj.get_int( "speed_penalty", 10 );
    volume = obj.get_int( "volume" );
    fun = obj.get_int( "fun" );
    fun_bonus = obj.get_int( "fun_bonus", 0 );
    description_frequency = time_duration::from_turns( obj.get_int( "description_frequency", 0 ) );
    if( !obj.read( "description_frequency", description_frequency ) ) {
        obj.throw_error( "missing member \"description_frequency\"" );
    }
    obj.read( "player_descriptions", player_descriptions );
    obj.read( "npc_descriptions", npc_descriptions );
}

cata::optional<int> musical_instrument_actor::use( Character &p, item &it, bool t,
        const tripoint & ) const
{
    if( !p.is_npc() && music::is_active_music_id( music::music_id::instrument ) ) {
        music::deactivate_music_id( music::music_id::instrument );
        // Because musical instrument creates musical sound too
        if( music::is_active_music_id( music::music_id::sound ) ) {
            music::deactivate_music_id( music::music_id::sound );
        }
    }

    if( p.is_mounted() ) {
        p.add_msg_player_or_npc( m_bad, _( "You can't play music while mounted." ),
                                 _( "<npcname> can't play music while mounted." ) );
        it.active = false;
        return cata::nullopt;
    }
    if( p.is_underwater() ) {
        p.add_msg_player_or_npc( m_bad,
                                 _( "You can't play music underwater" ),
                                 _( "<npcname> can't play music underwater" ) );
        it.active = false;
        return cata::nullopt;
    }

    // Stop playing a wind instrument when winded or even eventually become winded while playing it?
    // It's impossible to distinguish instruments for now anyways.
    if( p.has_effect( effect_sleep ) || p.has_effect( effect_stunned ) ||
        p.has_effect( effect_asthma ) ) {
        p.add_msg_player_or_npc( m_bad,
                                 _( "You stop playing your %s" ),
                                 _( "<npcname> stops playing their %s" ),
                                 it.display_name() );
        it.active = false;
        return cata::nullopt;
    }

    if( !t && it.active ) {
        p.add_msg_player_or_npc( _( "You stop playing your %s" ),
                                 _( "<npcname> stops playing their %s" ),
                                 it.display_name() );
        it.active = false;
        return cata::nullopt;
    }

    // Check for worn or wielded - no "floating"/bionic instruments for now
    // TODO: Distinguish instruments played with hands and with mouth, consider encumbrance
    const int inv_pos = p.get_item_position( &it );
    if( inv_pos >= 0 || inv_pos == INT_MIN ) {
        p.add_msg_player_or_npc( m_bad,
                                 _( "You need to hold or wear %s to play it" ),
                                 _( "<npcname> needs to hold or wear %s to play it" ),
                                 it.display_name() );
        it.active = false;
        return cata::nullopt;
    }

    // At speed this low you can't coordinate your actions well enough to play the instrument
    if( p.get_speed() <= 25 + speed_penalty ) {
        p.add_msg_player_or_npc( m_bad,
                                 _( "You feel too weak to play your %s" ),
                                 _( "<npcname> feels too weak to play their %s" ),
                                 it.display_name() );
        it.active = false;
        return cata::nullopt;
    }

    // We can play the music now
    if( !p.is_npc() ) {
        music::activate_music_id( music::music_id::instrument );
    }

    if( !it.active ) {
        p.add_msg_player_or_npc( m_good,
                                 _( "You start playing your %s" ),
                                 _( "<npcname> starts playing their %s" ),
                                 it.display_name() );
        it.active = true;
    }

    if( p.get_effect_int( effect_playing_instrument ) <= speed_penalty ) {
        // Only re-apply the effect if it wouldn't lower the intensity
        p.add_effect( effect_playing_instrument, 2_turns, false, speed_penalty );
    }

    std::string desc = "music";
    /** @EFFECT_PER increases morale bonus when playing an instrument */
    const int morale_effect = fun + fun_bonus * p.per_cur;
    if( morale_effect >= 0 && calendar::once_every( description_frequency ) ) {
        if( !player_descriptions.empty() && p.is_avatar() ) {
            desc = random_entry( player_descriptions ).translated();
        } else if( !npc_descriptions.empty() && p.is_npc() ) {
            //~ %1$s: npc name, %2$s: npc action description
            desc = string_format( pgettext( "play music", "%1$s %2$s" ), p.disp_name( false ),
                                  random_entry( npc_descriptions ) );
        }
    } else if( morale_effect < 0 && calendar::once_every( 1_minutes ) ) {
        // No musical skills = possible morale penalty
        if( p.is_avatar() ) {
            desc = _( "You produce an annoying sound" );
        } else {
            desc = string_format( _( "%s produces an annoying sound" ), p.disp_name( false ) );
        }
    }

    if( morale_effect >= 0 ) {
        sounds::sound( p.pos(), volume, sounds::sound_t::music, desc, true, "musical_instrument",
                       it.typeId().str() );
    } else {
        sounds::sound( p.pos(), volume, sounds::sound_t::music, desc, true, "musical_instrument_bad",
                       it.typeId().str() );
    }

    if( !p.has_effect( effect_music ) && p.can_hear( p.pos(), volume ) ) {
        // Sound code doesn't describe noises at the player position
        if( p.is_avatar() && desc != "music" ) {
            add_msg( m_info, desc );
        }
        p.add_effect( effect_music, 1_turns );
        const int sign = morale_effect > 0 ? 1 : -1;
        p.add_morale( MORALE_MUSIC, sign, morale_effect, 5_minutes, 2_minutes, true );
    }

    return 0;
}

ret_val<bool> musical_instrument_actor::can_use( const Character &p, const item &, bool,
        const tripoint & ) const
{
    // TODO: (maybe): Mouth encumbrance? Smoke? Lack of arms? Hand encumbrance?
    if( p.is_underwater() ) {
        return ret_val<bool>::make_failure( _( "You can't do that while underwater." ) );
    }
    if( p.is_mounted() ) {
        return ret_val<bool>::make_failure( _( "You can't do that while mounted." ) );
    }

    return ret_val<bool>::make_success();
}

std::unique_ptr<iuse_actor> learn_spell_actor::clone() const
{
    return std::make_unique<learn_spell_actor>( *this );
}

void learn_spell_actor::load( const JsonObject &obj )
{
    spells = obj.get_string_array( "spells" );
}

void learn_spell_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    std::string message;
    if( spells.size() == 1 ) {
        message = _( "This can teach you a spell." );
    } else {
        message = _( "This can teach you a number of spells." );
    }
    dump.emplace_back( "DESCRIPTION", message );
    dump.emplace_back( "DESCRIPTION", _( "Spells Contained:" ) );
    avatar &pc = get_avatar();
    std::string spell_text;
    for( const std::string &sp_id_str : spells ) {
        const spell_id sp_id( sp_id_str );
        spell_text = sp_id.obj().name.translated();
        if( pc.has_trait( trait_ILLITERATE ) ) {
            dump.emplace_back( "SPELL", spell_text );
        } else {
            if( pc.magic->knows_spell( sp_id ) ) {
                const spell sp = pc.magic->get_spell( sp_id );
                spell_text += ": " + string_format( _( "Level %u" ), sp.get_level() );
                if( sp.is_max_level() ) {
                    spell_text = string_format( _( "<color_light_green>%1$s (Max)</color>" ), spell_text );
                } else {
                    spell_text = string_format( "<color_yellow>%s</color>", spell_text );
                }
            } else {
                if( pc.magic->can_learn_spell( pc, sp_id ) ) {
                    spell_text = string_format( "<color_light_blue>%s</color>", spell_text );
                }
            }
            dump.emplace_back( "SPELL", spell_text );
        }
    }
}

cata::optional<int> learn_spell_actor::use( Character &p, item &, bool, const tripoint & ) const
{
    if( p.fine_detail_vision_mod() > 4 ) {
        p.add_msg_if_player( _( "It's too dark to read." ) );
        return cata::nullopt;
    }
    if( p.has_trait( trait_ILLITERATE ) ) {
        p.add_msg_if_player( _( "You can't read." ) );
        return cata::nullopt;
    }
    std::vector<uilist_entry> uilist_initializer;
    uilist spellbook_uilist;
    spellbook_callback sp_cb;
    bool know_it_all = true;
    for( const std::string &sp_id_str : spells ) {
        const spell_id sp_id( sp_id_str );
        sp_cb.add_spell( sp_id );
        uilist_entry entry( sp_id.obj().name.translated() );
        if( p.magic->knows_spell( sp_id ) ) {
            const spell sp = p.magic->get_spell( sp_id );
            entry.ctxt = string_format( _( "Level %u" ), sp.get_level() );
            if( sp.is_max_level() ) {
                entry.ctxt += _( " (Max)" );
                entry.enabled = false;
            } else {
                know_it_all = false;
            }
        } else {
            if( p.magic->can_learn_spell( p, sp_id ) ) {
                entry.ctxt = _( "Study to Learn" );
                know_it_all = false;
            } else {
                entry.ctxt = _( "Can't learn!" );
                entry.enabled = false;
            }
        }
        uilist_initializer.emplace_back( entry );
    }

    if( know_it_all ) {
        add_msg( m_info, _( "You already know everything this could teach you." ) );
        return cata::nullopt;
    }

    spellbook_uilist.entries = uilist_initializer;
    spellbook_uilist.w_height_setup = 24;
    spellbook_uilist.w_width_setup = 80;
    spellbook_uilist.callback = &sp_cb;
    spellbook_uilist.title = _( "Study a spell:" );
    spellbook_uilist.pad_left_setup = 38;
    spellbook_uilist.query();
    const int action = spellbook_uilist.ret;
    if( action < 0 ) {
        return cata::nullopt;
    }
    const bool knows_spell = p.magic->knows_spell( spells[action] );
    player_activity study_spell( ACT_STUDY_SPELL,
                                 p.magic->time_to_learn_spell( p, spells[action] ) );
    study_spell.str_values = {
        "", // reserved for "until you gain a spell level" option [0]
        "learn"
    }; // [1]
    study_spell.values = { 0, 0, 0 };
    if( knows_spell ) {
        study_spell.str_values[1] = "study";
        const int study_time = uilist( _( "Spend how long studying?" ), {
            { to_moves<int>( 30_minutes ), true, -1, _( "30 minutes" ) },
            { to_moves<int>( 1_hours ), true, -1, _( "1 hour" ) },
            { to_moves<int>( 2_hours ), true, -1, _( "2 hours" ) },
            { to_moves<int>( 4_hours ), true, -1, _( "4 hours" ) },
            { to_moves<int>( 8_hours ), true, -1, _( "8 hours" ) },
            { 10100, true, -1, _( "Until you gain a spell level" ) }
        } );
        if( study_time <= 0 ) {
            return cata::nullopt;
        }
        study_spell.moves_total = study_time;
        spell &studying = p.magic->get_spell( spell_id( spells[action] ) );
        if( studying.get_difficulty() < p.get_skill_level( studying.skill() ) ) {
            p.handle_skill_warning( studying.skill(),
                                    true ); // show the skill warning on start reading, since we don't show it during
        }
    }
    study_spell.moves_left = study_spell.moves_total;
    if( study_spell.moves_total == 10100 ) {
        study_spell.str_values[0] = "gain_level";
        study_spell.values[0] = 0; // reserved for xp
        study_spell.values[1] = p.magic->get_spell( spell_id( spells[action] ) ).get_level() + 1;
    }
    study_spell.name = spells[action];
    p.assign_activity( study_spell, false );
    return 0;
}

std::unique_ptr<iuse_actor> cast_spell_actor::clone() const
{
    return std::make_unique<cast_spell_actor>( *this );
}

void cast_spell_actor::load( const JsonObject &obj )
{
    no_fail = obj.get_bool( "no_fail" );
    item_spell = spell_id( obj.get_string( "spell_id" ) );
    spell_level = obj.get_int( "level" );
    need_worn = obj.get_bool( "need_worn", false );
    need_wielding = obj.get_bool( "need_wielding", false );
    mundane = obj.get_bool( "mundane", false );
}

void cast_spell_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    if( mundane ) {
        const std::string message = string_format( _( "This item when activated: %1$s" ),
                                    item_spell->description );
        dump.emplace_back( "DESCRIPTION", message );
    } else {
        //~ %1$s: spell name, %2$i: spell level
        const std::string message = string_format( _( "This item casts %1$s at level %2$i." ),
                                    item_spell->name, spell_level );
        dump.emplace_back( "DESCRIPTION", message );
        if( no_fail ) {
            dump.emplace_back( "DESCRIPTION", _( "This item never fails." ) );
        }
    }
}

std::string cast_spell_actor::get_name() const
{
    if( mundane ) {
        return string_format( _( "Activate" ) );
    }

    return string_format( _( "Cast spell" ) );
}

cata::optional<int> cast_spell_actor::use( Character &p, item &it, bool, const tripoint & ) const
{
    if( need_worn && !p.is_worn( it ) ) {
        p.add_msg_if_player( m_info, _( "You need to wear the %1$s before activating it." ), it.tname() );
        return cata::nullopt;
    }
    if( need_wielding && !p.is_wielding( it ) ) {
        p.add_msg_if_player( m_info, _( "You need to wield the %1$s before activating it." ), it.tname() );
        return cata::nullopt;
    }

    spell casting = spell( spell_id( item_spell ) );
    int charges = it.type->charges_to_use();

    player_activity cast_spell( ACT_SPELLCASTING, casting.casting_time( p ) );
    // [0] this is used as a spell level override for items casting spells
    cast_spell.values.emplace_back( spell_level );
    if( no_fail ) {
        // [1] if this value is 1, the spell never fails
        cast_spell.values.emplace_back( 1 );
    } else {
        // [1]
        cast_spell.values.emplace_back( 0 );
    }
    cast_spell.name = casting.id().c_str();
    if( it.has_flag( flag_USE_PLAYER_ENERGY ) ) {
        // [2] this value overrides the mana cost if set to 0
        cast_spell.values.emplace_back( 1 );
        charges = 0;
    } else {
        // [2]
        cast_spell.values.emplace_back( 0 );
    }
    p.assign_activity( cast_spell, false );
    return charges;
}

std::unique_ptr<iuse_actor> holster_actor::clone() const
{
    return std::make_unique<holster_actor>( *this );
}

void holster_actor::load( const JsonObject &obj )
{
    obj.read( "holster_prompt", holster_prompt );
    obj.read( "holster_msg", holster_msg );
}

bool holster_actor::can_holster( const item &holster, const item &obj ) const
{
    if( !holster.can_contain( obj ).success() ) {
        return false;
    }
    if( obj.active ) {
        return false;
    }
    return holster.can_contain( obj ).success();
}

bool holster_actor::store( Character &you, item &holster, item &obj ) const
{
    if( obj.is_null() || holster.is_null() ) {
        debugmsg( "Null item was passed to holster_actor" );
        return false;
    }

    const ret_val<bool> contain = holster.can_contain( obj );
    if( !contain.success() ) {
        you.add_msg_if_player( m_bad, contain.str(), holster.tname(), obj.tname() );
    }
    if( obj.active ) {
        you.add_msg_if_player( m_info, _( "You don't think putting your %1$s in your %2$s is a good idea" ),
                               obj.tname(), holster.tname() );
        return false;
    }
    you.add_msg_if_player( holster_msg.empty() ? _( "You holster your %s" ) : holster_msg.translated(),
                           obj.tname(), holster.tname() );

    // holsters ignore penalty effects (e.g. GRABBED) when determining number of moves to consume
    you.as_character()->store( holster, obj, false, holster.obtain_cost( obj ),
                               item_pocket::pocket_type::CONTAINER, true );
    return true;
}

cata::optional<int> holster_actor::use( Character &you, item &it, bool, const tripoint & ) const
{
    if( you.is_wielding( it ) ) {
        you.add_msg_if_player( _( "You need to unwield your %s before using it." ), it.tname() );
        return cata::nullopt;
    }

    int pos = 0;
    std::vector<std::string> opts;

    std::string prompt = holster_prompt.empty() ? _( "Holster item" ) : holster_prompt.translated();
    opts.push_back( prompt );
    pos = -1;
    std::list<item *> all_items = it.all_items_top(
                                      item_pocket::pocket_type::CONTAINER );
    std::transform( all_items.begin(), all_items.end(), std::back_inserter( opts ),
    []( const item * elem ) {
        return string_format( _( "Draw %s" ), elem->display_name() );
    } );

    item *internal_item = nullptr;
    if( opts.size() > 1 ) {
        int ret = uilist( string_format( _( "Use %s" ), it.tname() ), opts );
        if( ret < 0 ) {
            pos = -2;
        } else {
            pos += ret;
            if( opts.size() != it.num_item_stacks() ) {
                ret--;
            }
            auto iter = std::next( all_items.begin(), ret );
            internal_item = *iter;
        }
    } else {
        internal_item = all_items.front();
    }

    if( pos < -1 ) {
        you.add_msg_if_player( _( "Never mind." ) );
        return cata::nullopt;
    }

    if( pos >= 0 ) {
        // worn holsters ignore penalty effects (e.g. GRABBED) when determining number of moves to consume
        if( you.is_worn( it ) ) {
            you.wield_contents( it, internal_item, false, it.obtain_cost( *internal_item ) );
        } else {
            you.wield_contents( it, internal_item );
        }

    } else {
        if( you.as_avatar() == nullptr ) {
            return cata::nullopt;
        }
        // may not strictly be the correct item_location, but plumbing item_location through all iuse_actor::use won't work
        item_location item_loc( you, &it );
        game_menus::inv::insert_items( *you.as_avatar(), item_loc );
    }

    return 0;
}

void holster_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    std::string message = _( "Can be activated to store suitable items." );
    dump.emplace_back( "DESCRIPTION", message );
}

std::unique_ptr<iuse_actor> ammobelt_actor::clone() const
{
    return std::make_unique<ammobelt_actor>( *this );
}

void ammobelt_actor::load( const JsonObject &obj )
{
    belt = itype_id( obj.get_string( "belt" ) );
}

void ammobelt_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    dump.emplace_back( "AMMO", string_format( _( "Can be used to assemble: %s" ),
                       item::nname( belt ) ) );
}

cata::optional<int> ammobelt_actor::use( Character &p, item &, bool, const tripoint & ) const
{
    item mag( belt );
    mag.ammo_unset();

    if( p.rate_action_reload( mag ) != hint_rating::good ) {
        p.add_msg_if_player( _( "Insufficient ammunition to assemble %s" ), mag.tname() );
        return cata::nullopt;
    }

    item::reload_option opt = p.select_ammo( mag, true );
    std::vector<item_location> targets;
    if( opt ) {
        const int moves = opt.moves();
        item_location loc = p.i_add( mag );
        targets.emplace_back( loc );
        targets.push_back( std::move( opt.ammo ) );
        p.assign_activity( player_activity( reload_activity_actor( moves, opt.qty(), targets ) ) );
    }

    return 0;
}

void repair_item_actor::load( const JsonObject &obj )
{
    // Mandatory:
    for( const std::string line : obj.get_array( "materials" ) ) {
        materials.emplace( line );
    }

    // TODO: Make skill non-mandatory while still erroring on invalid skill
    const std::string skill_string = obj.get_string( "skill" );
    used_skill = skill_id( skill_string );
    if( !used_skill.is_valid() ) {
        obj.throw_error_at( "skill", "Invalid skill" );
    }

    cost_scaling = obj.get_float( "cost_scaling" );

    // Optional
    tool_quality = obj.get_int( "tool_quality", 0 );
    move_cost    = obj.get_int( "move_cost", 500 );
    trains_skill_to = obj.get_int( "trains_skill_to", 5 ) - 1;
}

bool repair_item_actor::can_use_tool( const Character &p, const item &tool, bool print_msg ) const
{
    if( p.has_effect( effect_incorporeal ) ) {
        if( print_msg ) {
            p.add_msg_player_or_npc( m_bad, _( "You can't do that while incorporeal." ),
                                     _( "<npcname> can't do that while incorporeal." ) );
        }
        return false;
    }
    if( p.is_underwater() ) {
        if( print_msg ) {
            p.add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        }
        return false;
    }
    if( p.is_mounted() ) {
        if( print_msg ) {
            p.add_msg_player_or_npc( m_bad, _( "You can't do that while mounted." ),
                                     _( "<npcname> can't do that while mounted." ) );
        }
        return false;
    }
    if( p.fine_detail_vision_mod() > 4 ) {
        if( print_msg ) {
            p.add_msg_if_player( m_info, _( "You can't see to do that!" ) );
        }
        return false;
    }
    if( !tool.ammo_sufficient( &p ) ) {
        if( print_msg ) {
            p.add_msg_if_player( m_info, _( "Your tool does not have enough charges to do that." ) );
        }
        return false;
    }

    return true;
}

static item_location get_item_location( Character &p, item &it, const tripoint &pos )
{
    // Item on a character
    if( p.has_item( it ) ) {
        return item_location( p, &it );
    }

    // Item in a vehicle
    if( const optional_vpart_position &vp = get_map().veh_at( pos ) ) {
        vehicle_cursor vc( vp->vehicle(), vp->part_index() );
        bool found_in_vehicle = false;
        vc.visit_items( [&]( const item * e, item * ) {
            if( e == &it ) {
                found_in_vehicle = true;
                return VisitResponse::ABORT;
            }
            return VisitResponse::NEXT;
        } );
        if( found_in_vehicle ) {
            return item_location( vc, &it );
        }
    }

    // Item on the map
    return item_location( map_cursor( pos ), &it );
}

cata::optional<int> repair_item_actor::use( Character &p, item &it, bool,
        const tripoint &position ) const
{
    if( !can_use_tool( p, it, true ) ) {
        return cata::nullopt;
    }

    p.assign_activity( ACT_REPAIR_ITEM, 0, p.get_item_position( &it ), INT_MIN );
    // We also need to store the repair actor subtype in the activity
    p.activity.str_values.push_back( type );
    // storing of item_location to support repairs by tools on the ground
    p.activity.targets.emplace_back( get_item_location( p, it, position ) );
    // All repairs are done in the activity, including charge cost and target item selection
    return 0;
}

std::unique_ptr<iuse_actor> repair_item_actor::clone() const
{
    return std::make_unique<repair_item_actor>( *this );
}

std::set<itype_id> repair_item_actor::get_valid_repair_materials( const item &fix ) const
{
    std::set<itype_id> valid_entries;
    for( const auto &mat : materials ) {
        if( fix.made_of( mat ) ) {
            const itype_id &component_id = mat.obj().repaired_with();
            // Certain (different!) materials are repaired with the same components (steel, iron, hard steel use scrap metal).
            // This checks avoids adding the same component twice, which is annoying to the user.
            if( valid_entries.find( component_id ) != valid_entries.end() ) {
                continue;
            }
            valid_entries.insert( component_id );
        }
    }
    return valid_entries;
}

bool repair_item_actor::handle_components( Character &pl, const item &fix,
        bool print_msg, bool just_check ) const
{
    // Entries valid for repaired items
    std::set<itype_id> valid_entries = get_valid_repair_materials( fix );

    if( valid_entries.empty() ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _( "Your %s is not made of any of:" ),
                                  fix.tname() );
            for( const auto &mat_name : materials ) {
                const material_type &mat = mat_name.obj();
                pl.add_msg_if_player( m_info, _( "%s (repaired using %s)" ), mat.name(),
                                      item::nname( mat.repaired_with(), 2 ) );
            }
        }

        return false;
    }

    const inventory &crafting_inv = pl.crafting_inventory();

    // Repairing or modifying items requires at least 1 repair item,
    //  otherwise number is related to size of item
    // Round up if checking, but roll if actually consuming
    // TODO: should 250_ml be part of the cost_scaling?
    const int items_needed = std::max<int>( 1, just_check ?
                                            std::ceil( fix.base_volume() / 250_ml * cost_scaling ) :
                                            roll_remainder( fix.base_volume() / 250_ml * cost_scaling ) );

    std::function<bool( const item & )> filter;
    if( fix.is_filthy() ) {
        filter = []( const item & component ) {
            return component.allow_crafting_component();
        };
    } else {
        filter = is_crafting_component;
    }

    // Go through all discovered repair items and see if we have any of them available
    std::vector<item_comp> comps;
    for( const auto &component_id : valid_entries ) {
        if( item::count_by_charges( component_id ) ) {
            if( crafting_inv.has_charges( component_id, items_needed ) ) {
                comps.emplace_back( component_id, items_needed );
            }
        } else if( crafting_inv.has_amount( component_id, items_needed, false, filter ) ) {
            comps.emplace_back( component_id, items_needed );
        }
    }

    if( comps.empty() ) {
        if( print_msg ) {
            for( const auto &mat_comp : valid_entries ) {
                pl.add_msg_if_player( m_info,
                                      _( "You don't have enough %s to do that.  Have: %d, need: %d" ),
                                      item::nname( mat_comp, 2 ),
                                      item::find_type( mat_comp )->count_by_charges() ?
                                      crafting_inv.charges_of( mat_comp, items_needed ) :
                                      crafting_inv.amount_of( mat_comp, false ),
                                      items_needed );
            }
        }

        return false;
    }

    if( !just_check ) {
        if( comps.empty() ) {
            // This shouldn't happen - the check in can_repair_target should prevent it
            // But report it, just in case
            debugmsg( "Attempted repair with no components" );
        }

        pl.consume_items( comps, 1, filter );
    }

    return true;
}

// Find the difficulty of the recipes that result in id
// If the recipe is not known by the player, +1 to difficulty
// If player doesn't meet the requirements of the recipe, +1 to difficulty
// Returns -1 if no recipe is found
static int find_repair_difficulty( const Character &pl, const itype_id &id, bool training )
{
    // If the recipe is not found, this will remain unchanged
    int min = -1;
    for( const auto &e : recipe_dict ) {
        const recipe r = e.second;
        if( id != r.result() ) {
            continue;
        }
        // If this is the first time we found a recipe
        if( min == -1 ) {
            min = 5;
        }

        int cur_difficulty = r.difficulty;
        if( !training && !pl.knows_recipe( &r ) ) {
            cur_difficulty++;
        }

        if( !training && !pl.has_recipe_requirements( r ) ) {
            cur_difficulty++;
        }

        min = std::min( cur_difficulty, min );
    }

    return min;
}

// Returns the level of the lowest level recipe that results in item of `fix`'s type
// Or if it has a repairs_like, the lowest level recipe that results in that.
// If the recipe doesn't exist, difficulty is 10
int repair_item_actor::repair_recipe_difficulty( const Character &pl,
        const item &fix, bool training ) const
{
    int diff = find_repair_difficulty( pl, fix.typeId(), training );

    // If we don't find a recipe, see if there's a repairs_like that has a recipe
    if( diff == -1 && !fix.type->repairs_like.is_empty() ) {
        diff = find_repair_difficulty( pl, fix.type->repairs_like, training );
    }

    // If we still don't find a recipe, difficulty is 10
    if( diff == -1 ) {
        diff = 10;
    }

    return diff;
}

bool repair_item_actor::can_repair_target( Character &pl, const item &fix,
        bool print_msg ) const
{
    // In some rare cases (indices getting scrambled after inventory overflow)
    //  our `fix` can be a different item.
    if( fix.is_null() ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _( "You do not have that item!" ) );
        }
        return false;
    }
    if( fix.is_firearm() ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _( "That requires gunsmithing tools." ) );
        }
        return false;
    }
    if( fix.count_by_charges() || fix.has_flag( flag_NO_REPAIR ) ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _( "You cannot repair this type of item." ) );
        }
        return false;
    }

    if( any_of( materials.begin(), materials.end(), [&fix]( const material_id & mat ) {
    return mat.obj()
               .repaired_with() == fix.typeId();
    } ) ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _( "This can be used to repair other items, not itself." ) );
        }
        return false;
    }

    if( !handle_components( pl, fix, print_msg, true ) ) {
        return false;
    }

    const bool can_be_refitted = fix.has_flag( flag_VARSIZE );
    if( can_be_refitted && !fix.has_flag( flag_FIT ) ) {
        return true;
    }

    const bool resizing_matters = fix.get_sizing( pl ) != item::sizing::ignore;
    const bool small = pl.get_size() == creature_size::tiny;
    const bool can_resize = small != fix.has_flag( flag_UNDERSIZE );
    if( can_be_refitted && resizing_matters && can_resize ) {
        return true;
    }

    if( fix.damage() > fix.damage_floor( false ) ) {
        return true;
    }

    if( fix.damage() <= fix.damage_floor( true ) ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _( "Your %s is already enhanced to its maximum potential." ),
                                  fix.tname() );
        }
        return false;
    }

    if( fix.has_flag( flag_PRIMITIVE_RANGED_WEAPON ) || !fix.reinforceable() ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _( "You cannot improve your %s any more this way." ),
                                  fix.tname() );
        }
        return false;
    }

    return true;
}

std::pair<float, float> repair_item_actor::repair_chance(
    const Character &pl, const item &fix, repair_item_actor::repair_type action_type ) const
{
    /** @EFFECT_TAILOR randomly improves clothing repair efforts */
    /** @EFFECT_MECHANICS randomly improves metal repair efforts */
    const int skill = pl.get_skill_level( used_skill );
    const int recipe_difficulty = repair_recipe_difficulty( pl, fix );
    int action_difficulty = 0;
    switch( action_type ) {
        case RT_REPAIR:
            action_difficulty = fix.damage_level();
            break;
        case RT_REFIT:
            // Let's make refitting as hard as recovering an almost-wrecked item
            action_difficulty = fix.max_damage() / itype::damage_scale;
            break;
        case RT_REINFORCE:
            // Reinforcing is at least as hard as refitting
            action_difficulty = std::max( fix.max_damage() / itype::damage_scale, recipe_difficulty );
            break;
        case RT_PRACTICE:
            // Skill gain scales with recipe difficulty, so practice difficulty should too
            action_difficulty = recipe_difficulty;
        default:
            ;
    }

    const int difficulty = recipe_difficulty + action_difficulty;
    // Sample numbers:
    // Item   | Damage | Skill | Dex | Success | Failure
    // Hoodie |    2   |   3   |  10 |   6%    |   0%
    // Hazmat |    1   |   10  |  10 |   8%    |   0%
    // Hazmat |    1   |   5   |  20 |   0%    |   2%
    // t-shirt|    4   |   1   |  5  |   2%    |   3%
    // Duster |    2   |   5   |  5  |   10%   |   0%
    // Duster |    2   |   2   |  10 |   4%    |   1%
    // Duster | Refit  |   2   |  10 |   0%    |   N/A
    float success_chance = ( 10 + 2 * skill - 2 * difficulty + tool_quality / 5.0f ) / 100.0f;
    /** @EFFECT_DEX reduces the chances of damaging an item when repairing */
    float damage_chance = ( difficulty - skill - ( tool_quality + pl.dex_cur ) / 5.0f ) / 100.0f;

    damage_chance = std::max( 0.0f, std::min( 1.0f, damage_chance ) );
    success_chance = std::max( 0.0f, std::min( 1.0f - damage_chance, success_chance ) );

    return std::make_pair( success_chance, damage_chance );
}

repair_item_actor::repair_type repair_item_actor::default_action( const item &fix,
        int current_skill_level ) const
{
    if( fix.damage() > fix.damage_floor( false ) ) {
        return RT_REPAIR;
    }

    const bool can_be_refitted = fix.has_flag( flag_VARSIZE );
    const bool doesnt_fit = !fix.has_flag( flag_FIT );
    if( doesnt_fit && can_be_refitted ) {
        return RT_REFIT;
    }

    Character &player_character = get_player_character();
    const bool smol = player_character.get_size() == creature_size::tiny;

    const bool is_undersized = fix.has_flag( flag_UNDERSIZE );
    const bool is_oversized = fix.has_flag( flag_OVERSIZE );
    const bool resizing_matters = fix.get_sizing( player_character ) != item::sizing::ignore;

    const bool too_big_while_smol = smol && !is_undersized && !is_oversized;
    if( too_big_while_smol && can_be_refitted && resizing_matters ) {
        return RT_DOWNSIZING;
    }

    const bool too_small_while_big = !smol && is_undersized && !is_oversized;
    if( too_small_while_big && can_be_refitted && resizing_matters ) {
        return RT_UPSIZING;
    }

    if( fix.damage() > fix.damage_floor( true ) && fix.damage_floor( true ) < 0 ) {
        return RT_REINFORCE;
    }

    if( current_skill_level <= trains_skill_to ) {
        return RT_PRACTICE;
    }

    return RT_NOTHING;
}

static bool damage_item( Character &pl, item_location &fix )
{
    const std::string startdurability = fix->durability_indicator( true );
    const bool destroyed = fix->inc_damage();
    const std::string resultdurability = fix->durability_indicator( true );
    pl.add_msg_if_player( m_bad, _( "You damage your %s!  ( %s-> %s)" ), fix->tname( 1, false ),
                          startdurability, resultdurability );
    if( destroyed ) {
        pl.add_msg_if_player( m_bad, _( "You destroy it!" ) );
        if( fix.where() == item_location::type::character ) {
            pl.i_rem_keep_contents( fix.get_item() );
        } else {
            for( const item *it : fix->all_items_top() ) {
                if( it->has_flag( flag_NO_DROP ) ) {
                    continue;
                }
                put_into_vehicle_or_drop( pl, item_drop_reason::tumbling, { *it }, fix.position() );
            }
            fix.remove_item();
        }

        pl.calc_encumbrance();
        pl.calc_discomfort();

        return true;
    }

    return false;
}

repair_item_actor::attempt_hint repair_item_actor::repair( Character &pl, item &tool,
        item_location &fix, bool refit_only ) const
{
    if( !can_use_tool( pl, tool, true ) ) {
        return AS_CANT_USE_TOOL;
    }
    if( !can_repair_target( pl, *fix, true ) ) {
        return AS_CANT;
    }

    const int current_skill_level = pl.get_skill_level( used_skill );
    repair_item_actor::repair_type action;
    if( refit_only ) {
        if( fix->has_flag( flag_FIT ) ) {
            pl.add_msg_if_player( m_warning, _( "The %s already fits!" ), fix->tname() );
            return AS_CANT;
        } else if( !fix->has_flag( flag_VARSIZE ) ) {
            pl.add_msg_if_player( m_bad, _( "This %s cannot be refitted!" ), fix->tname() );
            return AS_CANT;
        } else {
            action = RT_REFIT;
        }
    } else {
        action = default_action( *fix, current_skill_level );
    }
    const auto chance = repair_chance( pl, *fix, action );
    int practice_amount = repair_recipe_difficulty( pl, *fix, true ) / 2 + 1;
    float roll_value = rng_float( 0.0, 1.0 );
    enum roll_result {
        SUCCESS,
        FAILURE,
        NEUTRAL
    } roll;

    if( roll_value > 1.0f - chance.second ) {
        roll = FAILURE;
    } else if( roll_value < chance.first ) {
        roll = SUCCESS;
    } else {
        roll = NEUTRAL;
    }

    if( action == RT_NOTHING ) {
        pl.add_msg_if_player( m_bad, _( "You won't learn anything more by doing that." ) );
        return AS_CANT;
    }

    // If not for this if, it would spam a lot
    if( current_skill_level > trains_skill_to ) {
        practice_amount = 0;
    }
    pl.practice( used_skill, practice_amount, trains_skill_to );

    if( roll == FAILURE ) {
        if( chance.first <= 0.0f ) {
            pl.add_msg_if_player( m_bad,
                                  _( "It seems working on your %s is just beyond your current level of expertise." ),
                                  fix->tname() );
        }
        return damage_item( pl, fix ) ? AS_DESTROYED : AS_FAILURE;
    }

    if( action == RT_PRACTICE ) {
        return AS_RETRY;
    }

    if( action == RT_REPAIR ) {
        if( roll == SUCCESS ) {
            const std::string startdurability = fix->durability_indicator( true );
            const int damage = fix->damage();
            handle_components( pl, *fix, false, false );
            fix->mod_damage( -std::min( static_cast<int>( itype::damage_scale ), damage ) );
            const std::string resultdurability = fix->durability_indicator( true );
            if( damage > itype::damage_scale ) {
                pl.add_msg_if_player( m_good, _( "You repair your %s!  ( %s-> %s)" ), fix->tname( 1, false ),
                                      startdurability, resultdurability );
            } else {
                pl.add_msg_if_player( m_good, _( "You repair your %s completely!  ( %s-> %s)" ), fix->tname( 1,
                                      false ), startdurability, resultdurability );
            }
            return AS_SUCCESS;
        }

        return AS_RETRY;
    }

    if( action == RT_REFIT ) {
        if( roll == SUCCESS ) {
            if( !fix->has_flag( flag_FIT ) ) {
                pl.add_msg_if_player( m_good, _( "You take your %s in, improving the fit." ),
                                      fix->tname() );
                fix->set_flag( flag_FIT );

                pl.calc_encumbrance();
            }
            handle_components( pl, *fix, false, false );
            return AS_SUCCESS;
        }

        return AS_RETRY;
    }

    if( action == RT_DOWNSIZING ) {
        //We don't need to check for smallness or undersize because DOWNSIZING already guarantees that
        if( roll == SUCCESS ) {
            pl.add_msg_if_player( m_good, _( "You resize the %s to accommodate your tiny build." ),
                                  fix->tname().c_str() );
            fix->set_flag( flag_UNDERSIZE );
            pl.calc_encumbrance();
            handle_components( pl, *fix, false, false );
            return AS_SUCCESS;
        }
        return AS_RETRY;
    }

    if( action == RT_UPSIZING ) {
        //We don't need to check for smallness or undersize because UPSIZING already guarantees that
        if( roll == SUCCESS ) {
            pl.add_msg_if_player( m_good, _( "You adjust the %s back to its normal size." ),
                                  fix->tname().c_str() );
            fix->unset_flag( flag_UNDERSIZE );
            pl.calc_encumbrance();
            handle_components( pl, *fix, false, false );
            return AS_SUCCESS;
        }
        return AS_RETRY;
    }

    if( action == RT_REINFORCE ) {
        if( fix->has_flag( flag_PRIMITIVE_RANGED_WEAPON ) || !fix->reinforceable() ) {
            pl.add_msg_if_player( m_info, _( "You cannot improve your %s any more this way." ),
                                  fix->tname() );
            return AS_CANT;
        }

        if( roll == SUCCESS ) {
            pl.add_msg_if_player( m_good, _( "You make your %s extra sturdy." ), fix->tname() );
            fix->mod_damage( -itype::damage_scale );
            handle_components( pl, *fix, false, false );
            return AS_SUCCESS;
        }

        return AS_RETRY;
    }

    pl.add_msg_if_player( m_info, _( "Your %s is already enhanced." ), fix->tname() );
    return AS_CANT;
}

std::string repair_item_actor::action_description( repair_item_actor::repair_type rt )
{
    static const std::array<std::string, NUM_REPAIR_TYPES> arr = {{
            translate_marker( "Nothing" ),
            translate_marker( "Repairing" ),
            translate_marker( "Refitting" ),
            translate_marker( "Downsizing" ),
            translate_marker( "Upsizing" ),
            translate_marker( "Reinforcing" ),
            translate_marker( "Practicing" )
        }
    };

    return _( arr[rt] );
}

std::string repair_item_actor::get_name() const
{
    const std::string mats = enumerate_as_string( materials.begin(), materials.end(),
    []( const material_id & mid ) {
        return mid->name();
    } );
    return string_format( _( "Repair %s" ), mats );
}

void heal_actor::load( const JsonObject &obj )
{
    // Mandatory
    move_cost = obj.get_int( "move_cost" );
    limb_power = obj.get_float( "limb_power", 0 );

    // Optional
    bandages_power = obj.get_float( "bandages_power", 0 );
    bandages_scaling = obj.get_float( "bandages_scaling", 0.25f * bandages_power );
    disinfectant_power = obj.get_float( "disinfectant_power", 0 );
    disinfectant_scaling = obj.get_float( "disinfectant_scaling", 0.25f * disinfectant_power );

    head_power = obj.get_float( "head_power", 0.8f * limb_power );
    torso_power = obj.get_float( "torso_power", 1.5f * limb_power );

    limb_scaling = obj.get_float( "limb_scaling", 0.25f * limb_power );
    double scaling_ratio = limb_power < 0.0001f ? 0.0 :
                           static_cast<double>( limb_scaling / limb_power );
    head_scaling = obj.get_float( "head_scaling", scaling_ratio * head_power );
    torso_scaling = obj.get_float( "torso_scaling", scaling_ratio * torso_power );

    bleed = obj.get_int( "bleed", 0 );
    bite = obj.get_float( "bite", 0.0f );
    infect = obj.get_float( "infect", 0.0f );

    if( obj.has_array( "effects" ) ) {
        for( const JsonObject e : obj.get_array( "effects" ) ) {
            effects.push_back( load_effect_data( e ) );
        }
    }

    if( obj.has_string( "used_up_item" ) ) {
        obj.read( "used_up_item", used_up_item_id, true );
    } else if( obj.has_object( "used_up_item" ) ) {
        JsonObject u = obj.get_object( "used_up_item" );
        u.read( "id", used_up_item_id, true );
        used_up_item_quantity = u.get_int( "quantity", used_up_item_quantity );
        used_up_item_charges = u.get_int( "charges", used_up_item_charges );
        used_up_item_flags = u.get_tags<flag_id>( "flags" );
    }
}

static Character &get_patient( Character &healer, const tripoint &pos )
{
    if( healer.pos() == pos ) {
        return healer;
    }

    Character *const person = get_creature_tracker().creature_at<Character>( pos );
    if( !person ) {
        // Default to heal self on failure not to break old functionality
        add_msg_debug( debugmode::DF_IUSE, "No heal target at position %d,%d,%d", pos.x, pos.y, pos.z );
        return healer;
    }

    return *person;
}

cata::optional<int> heal_actor::use( Character &p, item &it, bool, const tripoint &pos ) const
{
    if( p.is_underwater() ) {
        p.add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return cata::nullopt;
    }
    if( p.is_mounted() ) {
        p.add_msg_if_player( m_info, _( "You can't do that while mounted." ) );
        return cata::nullopt;
    }
    if( it.is_filthy() ) {
        p.add_msg_if_player( m_info, _( "You can't use filthy items for healing." ) );
        return cata::nullopt;
    }

    Character &patient = get_patient( p, pos );
    const bodypart_str_id hpp = use_healing_item( p, patient, it, false ).id();
    if( hpp == bodypart_str_id::NULL_ID() ) {
        return cata::nullopt;
    }

    // each tier of proficiency cuts required time by half
    int cost = move_cost;
    cost = p.has_proficiency( proficiency_prof_wound_care_expert ) ? cost / 2 : cost;
    cost = p.has_proficiency( proficiency_prof_wound_care ) ? cost / 2 : cost;

    p.assign_activity( player_activity( firstaid_activity_actor( cost, it.tname(),
                                        patient.getID() ) ) );

    // Player: Only time this item_location gets used in firstaid::finish() is when activating the item's
    // container from the inventory window, so an item_on_person impl is all that is needed.
    // Otherwise the proper item_location provided by menu selection supercedes it in consume::finish().
    // NPC: Will only use its inventory for first aid items.
    p.activity.targets.emplace_back( p, &it );
    p.activity.str_values.emplace_back( hpp.c_str() );
    p.moves = 0;
    return 0;
}

std::unique_ptr<iuse_actor> heal_actor::clone() const
{
    return std::make_unique<heal_actor>( *this );
}

int heal_actor::get_heal_value( const Character &healer, bodypart_id healed ) const
{
    int heal_base;
    float bonus_mult;
    if( healed == bodypart_id( "head" ) ) {
        heal_base = head_power;
        bonus_mult = head_scaling;
    } else if( healed == bodypart_id( "torso" ) ) {
        heal_base = torso_power;
        bonus_mult = torso_scaling;
    } else {
        heal_base = limb_power;
        bonus_mult = limb_scaling;
    }

    if( heal_base > 0 ) {
        /** @EFFECT_FIRSTAID increases healing item effects */
        return heal_base + bonus_mult * healer.get_skill_level( skill_firstaid );
    }

    return heal_base;
}

int heal_actor::get_bandaged_level( const Character &healer ) const
{
    if( bandages_power > 0 ) {
        int prof_bonus = healer.get_skill_level( skill_firstaid );
        prof_bonus = healer.has_proficiency( proficiency_prof_wound_care ) ?
                     prof_bonus + 1 : prof_bonus;
        prof_bonus = healer.has_proficiency( proficiency_prof_wound_care_expert ) ?
                     prof_bonus + 2 : prof_bonus;
        /** @EFFECT_FIRSTAID increases healing item effects */
        return bandages_power + bandages_scaling * prof_bonus;
    }

    return bandages_power;
}

int heal_actor::get_disinfected_level( const Character &healer ) const
{
    if( disinfectant_power > 0 ) {
        /** @EFFECT_FIRSTAID increases healing item effects */
        int prof_bonus = healer.get_skill_level( skill_firstaid );
        prof_bonus = healer.has_proficiency( proficiency_prof_wound_care ) ?
                     prof_bonus + 1 : prof_bonus;
        prof_bonus = healer.has_proficiency( proficiency_prof_wound_care_expert ) ?
                     prof_bonus + 2 : prof_bonus;
        return disinfectant_power + disinfectant_scaling * prof_bonus;
    }

    return disinfectant_power;
}

int heal_actor::get_stopbleed_level( const Character &healer ) const
{
    if( bleed > 0 ) {
        /** @EFFECT_FIRSTAID increases healing item effects */
        int prof_bonus = healer.get_skill_level( skill_firstaid ) / 2;
        prof_bonus = healer.has_proficiency( proficiency_prof_wound_care ) ?
                     prof_bonus + 1 : prof_bonus;
        prof_bonus = healer.has_proficiency( proficiency_prof_wound_care_expert ) ?
                     prof_bonus + 2 : prof_bonus;
        return bleed + prof_bonus;
    }

    return bleed;
}

int heal_actor::finish_using( Character &healer, Character &patient, item &it,
                              bodypart_id healed ) const
{
    float practice_amount = limb_power * 3.0f;
    const int dam = get_heal_value( healer, healed );
    const int cur_hp = patient.get_part_hp_cur( healed );

    if( ( cur_hp >= 1 ) && ( dam > 0 ) ) { // Prevent first-aid from mending limbs
        patient.heal( healed, dam );
    } else if( ( cur_hp >= 1 ) && ( dam < 0 ) ) {
        patient.apply_damage( nullptr, healed, -dam ); //hurt takes + damage
    }

    Character &player_character = get_player_character();
    const bool u_see = healer.is_avatar() || patient.is_avatar() ||
                       player_character.sees( healer ) || player_character.sees( patient );
    const bool player_healing_player = healer.is_avatar() && patient.is_avatar();
    // Need a helper here - messages are from healer's point of view
    // but it would be cool if NPCs could use this function too
    const auto heal_msg = [&]( game_message_type msg_type,
    const char *player_player_msg, const char *other_msg ) {
        if( !u_see ) {
            return;
        }

        if( player_healing_player ) {
            add_msg( msg_type, player_player_msg );
        } else {
            add_msg( msg_type, other_msg );
        }
    };

    if( patient.has_effect( effect_bleed, healed ) ) {
        // small band-aids won't stop big arterial bleeding, but with tourniquet they just might
        int pwr = 3 * get_stopbleed_level( healer );
        if( patient.worn_with_flag( flag_TOURNIQUET,  healed ) ) {
            pwr *= 2;
        }
        if( pwr > patient.get_effect_int( effect_bleed, healed ) ) {
            effect &wound = patient.get_effect( effect_bleed, healed );
            time_duration dur = wound.get_duration() - ( get_stopbleed_level( healer ) *
                                wound.get_int_dur_factor() );
            wound.set_duration( std::max( 0_turns, dur ) );
            if( wound.get_duration() == 0_turns ) {
                heal_msg( m_good, _( "You stop the bleeding." ), _( "The bleeding is stopped." ) );
            } else {
                heal_msg( m_good, _( "You reduce the bleeding, but it's not stopped yet." ),
                          _( "The bleeding is reduced, but not stopped." ) );
            }
        } else {
            heal_msg( m_warning,
                      _( "Your dressing is too ineffective for a bleeding of this extent, and you fail to stop it." ),
                      _( "The wound still bleeds." ) );
        }
        practice_amount += bleed / 3.0f;
    }
    if( patient.has_effect( effect_bite, healed ) ) {
        if( x_in_y( bite, 1.0f ) ) {
            patient.remove_effect( effect_bite, healed );
            heal_msg( m_good, _( "You clean the wound." ), _( "The wound is cleaned." ) );
        } else {
            heal_msg( m_warning, _( "Your wound still aches." ), _( "The wound still looks bad." ) );
        }

        practice_amount += bite * 3.0f;
    }
    if( patient.has_effect( effect_infected, healed ) ) {
        if( x_in_y( infect, 1.0f ) ) {
            const time_duration infected_dur = patient.get_effect_dur( effect_infected, healed );
            patient.remove_effect( effect_infected, healed );
            patient.add_effect( effect_recover, infected_dur );
            heal_msg( m_good, _( "You disinfect the wound." ), _( "The wound is disinfected." ) );
        } else {
            heal_msg( m_warning, _( "Your wound still hurts." ), _( "The wound still looks nasty." ) );
        }

        practice_amount += infect * 10.0f;
    }

    healer.add_msg_if_player( _( "You finish using the %s." ), it.tname() );

    if( u_see && !healer.is_avatar() ) {
        //~ Healing complete message. %1$s is healer name, %2$s is item name.
        add_msg( _( "%1$s finishes using the %2$s." ), healer.disp_name(), it.tname() );
    }

    for( const effect_data &eff : effects ) {
        patient.add_effect( eff.id, eff.duration, eff.bp, eff.permanent );
    }

    if( !used_up_item_id.is_empty() ) {
        // If the item is a tool, `make` it the new form
        // Otherwise it probably was consumed, so create a new one
        if( it.is_tool() ) {
            it.convert( used_up_item_id );
            for( const auto &flag : used_up_item_flags ) {
                it.set_flag( flag );
            }
        } else {
            item used_up( used_up_item_id, it.birthday() );
            used_up.charges = used_up_item_charges;
            for( const auto &flag : used_up_item_flags ) {
                used_up.set_flag( flag );
            }
            healer.i_add_or_drop( used_up, used_up_item_quantity );
        }
    }

    // apply healing over time effects
    if( bandages_power > 0 ) {
        int bandages_intensity = get_bandaged_level( healer );
        patient.add_effect( effect_bandaged, 1_turns, healed );
        effect &e = patient.get_effect( effect_bandaged, healed );
        e.set_duration( e.get_int_dur_factor() * bandages_intensity );
        patient.set_part_damage_bandaged( healed,
                                          patient.get_part_hp_max( healed ) - patient.get_part_hp_cur( healed ) );
        practice_amount += 2 * bandages_intensity;
    }
    if( disinfectant_power > 0 ) {
        int disinfectant_intensity = get_disinfected_level( healer );
        patient.add_effect( effect_disinfected, 1_turns, healed );
        effect &e = patient.get_effect( effect_disinfected, healed );
        e.set_duration( e.get_int_dur_factor() * disinfectant_intensity );
        patient.set_part_damage_disinfected( healed,
                                             patient.get_part_hp_max( healed ) - patient.get_part_hp_cur( healed ) );
        practice_amount += 2 * disinfectant_intensity;
    }
    practice_amount = std::max( 9.0f, practice_amount );

    healer.practice( skill_firstaid, static_cast<int>( practice_amount ) );
    healer.practice_proficiency( proficiency_prof_wound_care,
                                 time_duration::from_turns( practice_amount ) );
    healer.practice_proficiency( proficiency_prof_wound_care_expert,
                                 time_duration::from_turns( practice_amount ) );
    return it.type->charges_to_use();
}

static bodypart_id pick_part_to_heal(
    const Character &healer, const Character &patient,
    const std::string &menu_header,
    int limb_power, int head_bonus, int torso_bonus,
    int bleed_stop, float bite_chance, float infect_chance,
    bool force, float bandage_power, float disinfectant_power )
{
    const bool bleed = bleed_stop > 0;
    const bool bite = bite_chance > 0.0f;
    const bool infect = infect_chance > 0.0f;
    const bool precise = &healer == &patient ? false :
                         /** @EFFECT_PER slightly increases precision when using first aid on someone else */
                         /** @EFFECT_FIRSTAID increases precision when using first aid on someone else */
                         ( ( healer.get_skill_level( skill_firstaid ) +
                             ( healer.has_proficiency( proficiency_prof_wound_care ) ? 0 : 1 ) +
                             ( healer.has_proficiency( proficiency_prof_wound_care ) ? 0 : 2 ) ) * 4 +
                           healer.per_cur >= 20 );
    while( true ) {
        bodypart_id healed_part = patient.body_window( menu_header, force, precise,
                                  limb_power, head_bonus, torso_bonus,
                                  bleed_stop, bite_chance, infect_chance, bandage_power, disinfectant_power );
        if( healed_part == bodypart_id( "bp_null" ) ) {
            return bodypart_id( "bp_null" );
        }

        if( ( infect && patient.has_effect( effect_infected, healed_part ) ) ||
            ( bite && patient.has_effect( effect_bite, healed_part ) ) ||
            ( bleed && patient.has_effect( effect_bleed, healed_part ) ) ) {
            return healed_part;
        }

        if( patient.is_limb_broken( healed_part ) ) {
            if( healed_part == bodypart_id( "arm_l" ) || healed_part == bodypart_id( "arm_r" ) ) {
                add_msg( m_info, _( "That arm is broken.  It needs surgical attention or a splint." ) );
            } else if( healed_part == bodypart_id( "leg_l" ) || healed_part == bodypart_id( "leg_r" ) ) {
                add_msg( m_info, _( "That leg is broken.  It needs surgical attention or a splint." ) );
            } else {
                debugmsg( "That body part is bugged.  It needs developer's attention." );
            }

            continue;
        }

        if( force || patient.get_part_hp_cur( healed_part ) < patient.get_part_hp_max( healed_part ) ) {
            return healed_part;
        }
    }
}

bodypart_id heal_actor::use_healing_item( Character &healer, Character &patient, item &it,
        bool force ) const
{
    bodypart_id healed = bodypart_id( "bp_null" );
    const int head_bonus = get_heal_value( healer, bodypart_id( "head" ) );
    const int limb_power = get_heal_value( healer, bodypart_id( "arm_l" ) );
    const int torso_bonus = get_heal_value( healer, bodypart_id( "torso" ) );

    if( !patient.can_use_heal_item( it ) ) {
        patient.add_msg_player_or_npc( m_bad,
                                       _( "Your biology is not compatible with that item." ),
                                       _( "<npcname>'s biology is not compatible with that item." ) );
        return bodypart_id( "bp_null" ); // canceled
    }

    if( healer.is_npc() ) {
        // NPCs heal whatever has sustained the most damage that they can heal but don't
        // rebandage parts unless they are bleeding significantly
        int highest_damage = 0;
        for( bodypart_id part_id : patient.get_all_body_parts( get_body_part_flags::only_main ) ) {
            int damage = 0;
            if( ( !patient.has_effect( effect_bandaged, part_id ) && bandages_power > 0 ) ||
                ( !patient.has_effect( effect_disinfected, part_id ) && disinfectant_power > 0 ) ) {
                damage += patient.get_part_hp_max( part_id ) - patient.get_part_hp_cur( part_id );
                damage += infect * patient.get_effect_dur( effect_infected, part_id ) / 10_minutes;
            }
            if( patient.get_effect_int( effect_bleed, part_id ) > 5 && bleed > 0 ) {
                damage += bleed * patient.get_effect_dur( effect_bleed, part_id ) / 5_minutes;
            }
            if( patient.has_effect( effect_bite, part_id ) && disinfectant_power > 0 ) {
                damage += bite * patient.get_effect_dur( effect_bite, part_id ) / 1_minutes;
            }
            if( damage > highest_damage ) {
                highest_damage = damage;
                healed = part_id;
            }
        }
    } else if( patient.is_avatar() ) {
        // Player healing self - let player select
        if( healer.activity.id() != ACT_FIRSTAID ) {
            const std::string menu_header = _( "Select a body part for: " ) + it.tname();
            healed = pick_part_to_heal( healer, patient, menu_header, limb_power, head_bonus, torso_bonus,
                                        get_stopbleed_level( healer ), bite, infect, force, get_bandaged_level( healer ),
                                        get_disinfected_level( healer ) );
            if( healed == bodypart_id( "bp_null" ) ) {
                add_msg( m_info, _( "Never mind." ) );
                return bodypart_id( "bp_null" ); // canceled
            }
            return healed;
        } else {
            // Completed activity, extract body part from it.
            healed =  bodypart_id( healer.activity.str_values[0] );
        }
    } else {
        // Player healing NPC
        // TODO: Remove this hack, allow using activities on NPCs
        const std::string menu_header = string_format( pgettext( "healing",
                                        //~ %1$s: patient name, %2$s: healing item name
                                        "Select a body part of %1$s for %2$s:" ),
                                        patient.disp_name(), it.tname() );
        healed = pick_part_to_heal( healer, patient, menu_header, limb_power, head_bonus, torso_bonus,
                                    get_stopbleed_level( healer ), bite, infect, force, get_bandaged_level( healer ),
                                    get_disinfected_level( healer ) );
    }

    return healed;
}

void heal_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    if( head_power > 0 || torso_power > 0 || limb_power > 0 || bandages_power > 0 ||
        disinfectant_power > 0 || bleed > 0 || bite > 0.0f || infect > 0.0f ) {
        dump.emplace_back( "HEAL", _( "<bold>Healing effects</bold> " ) );
    }

    Character &player_character = get_player_character();
    if( head_power > 0 || torso_power > 0 || limb_power > 0 ) {
        dump.emplace_back( "HEAL", _( "Base healing: " ) );
        dump.emplace_back( "HEAL_BASE", _( "Head: " ), "", iteminfo::no_newline, head_power );
        dump.emplace_back( "HEAL_BASE", _( "  Torso: " ), "", iteminfo::no_newline, torso_power );
        dump.emplace_back( "HEAL_BASE", _( "  Limbs: " ), limb_power );
        if( g != nullptr ) {
            dump.emplace_back( "HEAL", _( "Actual healing: " ) );
            dump.emplace_back( "HEAL_ACT", _( "Head: " ), "", iteminfo::no_newline,
                               get_heal_value( player_character, bodypart_id( "head" ) ) );
            dump.emplace_back( "HEAL_ACT", _( "  Torso: " ), "", iteminfo::no_newline,
                               get_heal_value( player_character, bodypart_id( "torso" ) ) );
            dump.emplace_back( "HEAL_ACT", _( "  Limbs: " ), get_heal_value( player_character,
                               bodypart_id( "arm_l" ) ) );
        }
    }

    if( bandages_power > 0 ) {
        dump.emplace_back( "HEAL", _( "Base bandaging quality: " ),
                           texitify_base_healing_power( static_cast<int>( bandages_power ) ) );
        if( g != nullptr ) {
            dump.emplace_back( "HEAL", _( "Actual bandaging quality: " ),
                               texitify_healing_power( get_bandaged_level( player_character ) ) );
        }
    }

    if( disinfectant_power > 0 ) {
        dump.emplace_back( "HEAL", _( "Base disinfecting quality: " ),
                           texitify_base_healing_power( static_cast<int>( disinfectant_power ) ) );
        if( g != nullptr ) {
            dump.emplace_back( "HEAL", _( "Actual disinfecting quality: " ),
                               texitify_healing_power( get_disinfected_level( player_character ) ) );
        }
    }
    if( bleed > 0 ) {
        dump.emplace_back( "HEAL", _( "Effect on bleeding: " ), texitify_bandage_power( bleed ) );
        if( g != nullptr ) {
            dump.emplace_back( "HEAL", _( "Actual effect on bleeding: " ),
                               texitify_healing_power( get_stopbleed_level( get_player_character() ) ) );
        }
    }
    if( bite > 0.0f || infect > 0.0f ) {
        dump.emplace_back( "HEAL", _( "Chance to heal (percent): " ) );
        if( bite > 0.0f ) {
            dump.emplace_back( "HEAL", _( "* Bite: " ),
                               static_cast<int>( bite * 100 ) );
        }
        if( infect > 0.0f ) {
            dump.emplace_back( "HEAL", _( "* Infection: " ),
                               static_cast<int>( infect * 100 ) );
        }
    }
    dump.emplace_back( "HEAL", _( "Moves to use: " ), move_cost );
}

place_trap_actor::place_trap_actor( const std::string &type ) :
    iuse_actor( type ), needs_neighbor_terrain( ter_str_id::NULL_ID() ),
    outer_layer_trap( trap_str_id::NULL_ID() ) {}

place_trap_actor::data::data() : trap( trap_str_id::NULL_ID() ) {}

void place_trap_actor::data::load( const JsonObject &obj )
{
    assign( obj, "trap", trap );
    assign( obj, "done_message", done_message );
    assign( obj, "practice", practice );
    assign( obj, "moves", moves );
}

void place_trap_actor::load( const JsonObject &obj )
{
    assign( obj, "allow_underwater", allow_underwater );
    assign( obj, "allow_under_player", allow_under_player );
    assign( obj, "needs_solid_neighbor", needs_solid_neighbor );
    assign( obj, "needs_neighbor_terrain", needs_neighbor_terrain );
    assign( obj, "bury_question", bury_question );
    if( !bury_question.empty() ) {
        JsonObject buried_json = obj.get_object( "bury" );
        buried_data.load( buried_json );
    }
    unburied_data.load( obj );
    assign( obj, "outer_layer_trap", outer_layer_trap );
}

std::unique_ptr<iuse_actor> place_trap_actor::clone() const
{
    return std::make_unique<place_trap_actor>( *this );
}

static bool is_solid_neighbor( const tripoint &pos, const point &offset )
{
    map &here = get_map();
    const tripoint a = pos + tripoint( offset, 0 );
    const tripoint b = pos - tripoint( offset, 0 );
    return here.move_cost( a ) != 2 && here.move_cost( b ) != 2;
}

static bool has_neighbor( const tripoint &pos, const ter_id &terrain_id )
{
    map &here = get_map();
    for( const tripoint &t : here.points_in_radius( pos, 1, 0 ) ) {
        if( here.ter( t ) == terrain_id ) {
            return true;
        }
    }
    return false;
}

bool place_trap_actor::is_allowed( Character &p, const tripoint &pos,
                                   const std::string &name ) const
{
    if( !allow_under_player && pos == p.pos() ) {
        p.add_msg_if_player( m_info, _( "Yeah.  Place the %s at your feet.  Real damn smart move." ),
                             name );
        return false;
    }
    map &here = get_map();
    if( here.move_cost( pos ) != 2 ) {
        p.add_msg_if_player( m_info, _( "You can't place a %s there." ), name );
        return false;
    }
    if( needs_solid_neighbor ) {
        if( !is_solid_neighbor( pos, point_east ) && !is_solid_neighbor( pos, point_south ) &&
            !is_solid_neighbor( pos, point_south_east ) && !is_solid_neighbor( pos, point_north_east ) ) {
            p.add_msg_if_player( m_info, _( "You must place the %s between two solid tiles." ), name );
            return false;
        }
    }
    if( needs_neighbor_terrain && !has_neighbor( pos, needs_neighbor_terrain ) ) {
        p.add_msg_if_player( m_info, _( "The %s needs a %s adjacent to it." ), name,
                             needs_neighbor_terrain.obj().name() );
        return false;
    }
    const trap &existing_trap = here.tr_at( pos );
    if( !existing_trap.is_null() ) {
        if( existing_trap.can_see( pos, p ) ) {
            p.add_msg_if_player( m_info, _( "You can't place a %s there.  It contains a trap already." ),
                                 name );
        } else {
            p.add_msg_if_player( m_bad, _( "You trigger a %s!" ), existing_trap.name() );
            existing_trap.trigger( pos, p );
        }
        return false;
    }
    return true;
}

static void place_and_add_as_known( Character &p, const tripoint &pos, const trap_str_id &id )
{
    map &here = get_map();
    here.trap_set( pos, id );
    const trap &tr = here.tr_at( pos );
    if( !tr.can_see( pos, p ) ) {
        p.add_known_trap( pos, tr );
    }
}

cata::optional<int> place_trap_actor::use( Character &p, item &it, bool, const tripoint & ) const
{
    const bool could_bury = !bury_question.empty();
    if( !allow_underwater && p.is_underwater() ) {
        p.add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return cata::nullopt;
    }
    if( p.is_mounted() ) {
        p.add_msg_if_player( m_info, _( "You can't do that while mounted." ) );
        return cata::nullopt;
    }
    const cata::optional<tripoint> pos_ = choose_adjacent( string_format( _( "Place %s where?" ),
                                          it.tname() ) );
    if( !pos_ ) {
        return cata::nullopt;
    }
    tripoint pos = *pos_;

    if( !is_allowed( p, pos, it.tname() ) ) {
        return cata::nullopt;
    }

    map &here = get_map();
    int distance_to_trap_center = unburied_data.trap.obj().get_trap_radius() +
                                  outer_layer_trap.obj().get_trap_radius() + 1;
    if( unburied_data.trap.obj().get_trap_radius() > 0 ) {
        // Math correction for multi-tile traps
        pos.x = ( pos.x - p.posx() ) * distance_to_trap_center + p.posx();
        pos.y = ( pos.y - p.posy() ) * distance_to_trap_center + p.posy();
        for( const tripoint &t : here.points_in_radius( pos, outer_layer_trap.obj().get_trap_radius(),
                0 ) ) {
            if( !is_allowed( p, t, it.tname() ) ) {
                p.add_msg_if_player( m_info,
                                     _( "That trap needs a space in %d tiles radius to be clear, centered %d tiles from you." ),
                                     outer_layer_trap.obj().get_trap_radius(), distance_to_trap_center );
                return cata::nullopt;
            }
        }
    }

    const bool has_shovel = p.has_quality( qual_DIG, 3 );
    const bool is_diggable = here.has_flag( ter_furn_flag::TFLAG_DIGGABLE, pos );
    bool bury = false;
    if( could_bury && has_shovel && is_diggable ) {
        bury = query_yn( "%s", bury_question );
    }
    const place_trap_actor::data &data = bury ? buried_data : unburied_data;

    p.add_msg_if_player( m_info, data.done_message.translated(), distance_to_trap_center );
    p.practice( skill_traps, data.practice );
    p.practice_proficiency( proficiency_prof_traps, time_duration::from_seconds( data.practice * 30 ) );
    p.practice_proficiency( proficiency_prof_trapsetting,
                            time_duration::from_seconds( data.practice * 30 ) );

    //Total time to set the trap will be determined by player's skills and proficiencies
    int move_cost_final = std::round( ( data.moves * std::min( 1,
                                        ( data.practice ^ 2 ) ) ) / ( p.get_skill_level( skill_traps ) <= 1 ? 1 : p.get_skill_level(
                                                skill_traps ) ) );
    if( !p.has_proficiency( proficiency_prof_trapsetting ) ) {
        move_cost_final = move_cost_final * 2;
    }
    if( !p.has_proficiency( proficiency_prof_traps ) ) {
        move_cost_final = move_cost_final * 4;
    }

    //This probably needs to be done via assign_activity
    p.mod_moves( -move_cost_final );

    place_and_add_as_known( p, pos, data.trap );
    for( const tripoint &t : here.points_in_radius( pos, data.trap.obj().get_trap_radius(), 0 ) ) {
        if( t != pos ) {
            place_and_add_as_known( p, t, outer_layer_trap );
        }
    }
    return 1;
}

void emit_actor::load( const JsonObject &obj )
{
    assign( obj, "emits", emits );
    assign( obj, "scale_qty", scale_qty );
}

cata::optional<int> emit_actor::use( Character &, item &it, bool, const tripoint &pos ) const
{
    map &here = get_map();
    const float scaling = scale_qty ? it.charges : 1.0f;
    for( const auto &e : emits ) {
        here.emit_field( pos, e, scaling );
    }

    return 1;
}

std::unique_ptr<iuse_actor> emit_actor::clone() const
{
    return std::make_unique<emit_actor>( *this );
}

void emit_actor::finalize( const itype_id &my_item_type )
{
    /*
    // TODO: This must be called after all finalization
    for( const auto& e : emits ) {
        if( !e.is_valid() ) {
            debugmsg( "Item %s has unknown emit source %s", my_item_type.c_str(), e.c_str() );
        }
    }
    */

    if( scale_qty && !item::count_by_charges( my_item_type ) ) {
        debugmsg( "Item %s has emit_actor with scale_qty, but is not counted by charges",
                  my_item_type.c_str() );
        scale_qty = false;
    }
}

void saw_barrel_actor::load( const JsonObject &jo )
{
    assign( jo, "cost", cost );
}

//Todo: Make this consume charges if performed with a tool that uses charges.
cata::optional<int> saw_barrel_actor::use( Character &p, item &it, bool t, const tripoint & ) const
{
    if( t ) {
        return cata::nullopt;
    }

    item_location loc = game_menus::inv::saw_barrel( p, it );

    if( !loc ) {
        p.add_msg_if_player( _( "Never mind." ) );
        return cata::nullopt;
    }

    item &obj = *loc.obtain( p );
    p.add_msg_if_player( _( "You saw down the barrel of your %s." ), obj.tname() );
    obj.put_in( item( "barrel_small", calendar::turn ), item_pocket::pocket_type::MOD );

    return 0;
}

ret_val<bool> saw_barrel_actor::can_use_on( const Character &, const item &,
        const item &target ) const
{
    if( !target.is_gun() ) {
        return ret_val<bool>::make_failure( _( "It's not a gun." ) );
    }

    if( target.type->gun->barrel_volume <= 0_ml ) {
        return ret_val<bool>::make_failure( _( "The barrel is too small." ) );
    }

    if( target.gunmod_find( itype_barrel_small ) ) {
        return ret_val<bool>::make_failure( _( "The barrel is already sawn-off." ) );
    }

    const auto gunmods = target.gunmods();
    const bool modified_barrel = std::any_of( gunmods.begin(), gunmods.end(),
    []( const item * mod ) {
        return mod->type->gunmod->location == gunmod_location( "barrel" );
    } );

    if( modified_barrel ) {
        return ret_val<bool>::make_failure( _( "Can't saw off modified barrels." ) );
    }

    return ret_val<bool>::make_success();
}

std::unique_ptr<iuse_actor> saw_barrel_actor::clone() const
{
    return std::make_unique<saw_barrel_actor>( *this );
}

void saw_stock_actor::load( const JsonObject &jo )
{
    assign( jo, "cost", cost );
}

//Todo: Make this consume charges if performed with a tool that uses charges.
cata::optional<int> saw_stock_actor::use( Character &p, item &it, bool t, const tripoint & ) const
{
    if( t ) {
        return cata::nullopt;
    }

    item_location loc = game_menus::inv::saw_stock( p, it );

    if( !loc ) {
        p.add_msg_if_player( _( "Never mind." ) );
        return cata::nullopt;
    }

    item &obj = *loc.obtain( p );
    p.add_msg_if_player( _( "You saw down the stock of your %s." ), obj.tname() );
    obj.put_in( item( "stock_none", calendar::turn ), item_pocket::pocket_type::MOD );

    return 0;
}

ret_val<bool> saw_stock_actor::can_use_on( const Character &, const item &,
        const item &target ) const
{
    if( !target.is_gun() ) {
        return ret_val<bool>::make_failure( _( "It's not a gun." ) );
    }

    if( target.gunmod_find( itype_stock_none ) ) {
        return ret_val<bool>::make_failure( _( "The stock is already sawn-off." ) );
    }

    const auto gunmods = target.gunmods();
    const bool modified_stock = std::any_of( gunmods.begin(), gunmods.end(),
    []( const item * mod ) {
        return mod->type->gunmod->location == gunmod_location( "stock" );
    } );

    const bool accessorized_stock = std::any_of( gunmods.begin(), gunmods.end(),
    []( const item * mod ) {
        return mod->type->gunmod->location == gunmod_location( "stock accessory" );
    } );

    if( target.get_free_mod_locations( gunmod_location( "stock mount" ) ) < 1 ) {
        return ret_val<bool>::make_failure(
                   _( "Can't cut off modern composite stocks (must have an empty stock mount)." ) );
    }

    if( modified_stock ) {
        return ret_val<bool>::make_failure( _( "Can't cut off modified stocks." ) );
    }

    if( accessorized_stock ) {
        return ret_val<bool>::make_failure( _( "Can't cut off accessorized stocks." ) );
    }

    return ret_val<bool>::make_success();
}

std::unique_ptr<iuse_actor> saw_stock_actor::clone() const
{
    return std::make_unique<saw_stock_actor>( *this );
}

void molle_attach_actor::load( const JsonObject &jo )
{
    assign( jo, "size", size );
    assign( jo, "moves", moves );
}

cata::optional<int> molle_attach_actor::use( Character &p, item &it, bool t,
        const tripoint & ) const
{
    if( t ) {
        return cata::nullopt;
    }



    item_location loc = game_menus::inv::molle_attach( p, it );

    if( !loc ) {
        p.add_msg_if_player( _( "Never mind." ) );
        return cata::nullopt;
    }

    item &obj = *loc.get_item();
    p.add_msg_if_player( _( "You attach %s to your MOLLE webbing." ), obj.tname() );

    it.get_contents().add_pocket( obj );

    // the item has been added to the vest it should no longer exist in the world
    loc.remove_item();



    return 0;
}

std::unique_ptr<iuse_actor> molle_attach_actor::clone() const
{
    return std::make_unique<molle_attach_actor>( *this );
}

cata::optional<int> molle_detach_actor::use( Character &p, item &it, bool,
        const tripoint & ) const
{

    std::vector<const item *> items_attached = it.get_contents().get_added_pockets();
    uilist prompt;
    prompt.text = _( "Remove which accessory?" );

    for( size_t i = 0; i != items_attached.size(); ++i ) {
        prompt.addentry( i, true, -1, items_attached[i]->tname() );
    }

    prompt.query();


    if( prompt.ret >= 0 ) {
        p.i_add( it.get_contents().remove_pocket( prompt.ret ) );
        p.add_msg_if_player( _( "You remove the item from your %s." ), it.tname() );
        return 0;
    }


    p.add_msg_if_player( _( "Never mind." ) );
    return cata::nullopt;
}

std::unique_ptr<iuse_actor> molle_detach_actor::clone() const
{
    return std::make_unique<molle_detach_actor>( *this );
}

void molle_detach_actor::load( const JsonObject &jo )
{
    assign( jo, "moves", moves );
}

cata::optional<int> install_bionic_actor::use( Character &p, item &it, bool,
        const tripoint & ) const
{
    if( p.can_install_bionics( *it.type, p, false ) ) {
        if( !p.has_trait( trait_DEBUG_BIONICS ) ) {
            p.consume_installation_requirement( it.type->bionic->id );
            p.consume_anesth_requirement( *it.type, p );
        }
        return p.install_bionics( *it.type, p, false ) ? it.type->charges_to_use() : 0;
    } else {
        return cata::nullopt;
    }
}

ret_val<bool> install_bionic_actor::can_use( const Character &p, const item &it, bool,
        const tripoint & ) const
{
    if( !it.is_bionic() ) {
        return ret_val<bool>::make_failure();
    }
    const bionic_id &bid = it.type->bionic->id;
    if( p.is_mounted() ) {
        return ret_val<bool>::make_failure( _( "You can't install bionics while mounted." ) );
    }
    if( !p.has_trait( trait_DEBUG_BIONICS ) ) {
        if( bid->installation_requirement.is_empty() ) {
            return ret_val<bool>::make_failure( _( "You can't self-install this CBM." ) );
        } else  if( it.has_flag( flag_FILTHY ) ) {
            return ret_val<bool>::make_failure( _( "You can't install a filthy CBM!" ) );
        } else if( it.has_flag( flag_NO_STERILE ) ) {
            return ret_val<bool>::make_failure( _( "This CBM is not sterile, you can't install it." ) );
        } else if( it.has_fault( fault_bionic_salvaged ) ) {
            return ret_val<bool>::make_failure(
                       _( "This CBM is already deployed.  You need to reset it to factory state." ) );
        } else if( units::energy( std::numeric_limits<int>::max(), units::energy::unit_type{} ) -
                   p.get_max_power_level() < bid->capacity ) {
            return ret_val<bool>::make_failure( _( "Max power capacity already reached" ) );
        }
    }

    if( p.has_bionic( bid ) && !bid->dupes_allowed ) {
        return ret_val<bool>::make_failure( _( "You have already installed this bionic." ) );
    } else if( bid->upgraded_bionic && !p.has_bionic( bid->upgraded_bionic ) ) {
        return ret_val<bool>::make_failure( _( "There is nothing to upgrade." ) );
    } else {
        const bool downgrade = std::any_of( bid->available_upgrades.begin(), bid->available_upgrades.end(),
                                            std::bind( &Character::has_bionic, &p, std::placeholders::_1 ) );

        if( downgrade ) {
            return ret_val<bool>::make_failure( _( "You have a superior version installed." ) );
        }
    }

    return ret_val<bool>::make_success();
}

std::unique_ptr<iuse_actor> install_bionic_actor::clone() const
{
    return std::make_unique<install_bionic_actor>( *this );
}

void install_bionic_actor::finalize( const itype_id &my_item_type )
{
    if( !item::find_type( my_item_type )->bionic ) {
        debugmsg( "Item %s has install_bionic actor, but it's not a bionic.", my_item_type.c_str() );
    }
}

cata::optional<int> detach_gunmods_actor::use( Character &p, item &it, bool,
        const tripoint & ) const
{
    auto filter_irremovable = []( std::vector<item *> &gunmods ) {
        gunmods.erase( std::remove_if( gunmods.begin(), gunmods.end(), std::bind( &item::is_irremovable,
                                       std::placeholders::_1 ) ), gunmods.end() );
    };

    item gun_copy = item( it );
    std::vector<item *> mods = it.gunmods();
    std::vector<item *> mods_copy = gun_copy.gunmods();

    filter_irremovable( mods );
    filter_irremovable( mods_copy );

    uilist prompt;
    prompt.text = _( "Remove which modification?" );

    for( size_t i = 0; i != mods.size(); ++i ) {
        prompt.addentry( i, true, -1, mods[ i ]->tname() );
    }

    prompt.query();

    if( prompt.ret >= 0 ) {
        gun_copy.remove_item( *mods_copy[prompt.ret] );

        if( p.meets_requirements( *mods[prompt.ret], gun_copy ) ||
            query_yn( _( "Are you sure?  You may be lacking the skills needed to reattach this modification." ) ) ) {

            if( game_menus::inv::compare_items( it, gun_copy, _( "Remove modification?" ) ) ) {
                p.gunmod_remove( it, *mods[prompt.ret] );
                return 0;
            }
        }
    }

    p.add_msg_if_player( _( "Never mind." ) );
    return cata::nullopt;
}

ret_val<bool> detach_gunmods_actor::can_use( const Character &p, const item &it, bool,
        const tripoint & ) const
{
    const auto mods = it.gunmods();

    if( mods.empty() ) {
        return ret_val<bool>::make_failure( _( "Doesn't appear to be modded." ) );
    }

    const bool no_removables = std::all_of( mods.begin(), mods.end(), std::bind( &item::is_irremovable,
                                            std::placeholders::_1 ) );

    if( no_removables ) {
        return ret_val<bool>::make_failure( _( "None of the mods can be removed." ) );
    }

    if( p.is_worn(
            it ) ) { // Prevent removal of shoulder straps and thereby making the gun un-wearable again.
        return ret_val<bool>::make_failure( _( "Has to be taken off first." ) );
    }

    return ret_val<bool>::make_success();
}

std::unique_ptr<iuse_actor> detach_gunmods_actor::detach_gunmods_actor::clone() const
{
    return std::make_unique<detach_gunmods_actor>( *this );
}

void detach_gunmods_actor::finalize( const itype_id &my_item_type )
{
    if( !item::find_type( my_item_type )->gun ) {
        debugmsg( "Item %s has detach_gunmods_actor actor, but it's a gun.", my_item_type.c_str() );
    }
}

cata::optional<int> modify_gunmods_actor::use( Character &p, item &it, bool,
        const tripoint &pnt ) const
{

    std::vector<item *> mods;
    for( item *mod : it.gunmods() ) {
        if( mod->is_transformable() ) {
            mods.push_back( mod );
        }
    }

    uilist prompt;
    prompt.text = _( "Modify which part" );

    for( size_t i = 0; i != mods.size(); ++i ) {
        prompt.addentry( i, true, -1, string_format( "%s: %s", mods[i]->tname(),
                         mods[i]->get_use( "transform" )->get_name() ) );
    }

    prompt.query();

    if( prompt.ret >= 0 ) {
        // set gun to default in case this changes anything
        it.gun_set_mode( gun_mode_DEFAULT );
        p.invoke_item( mods[prompt.ret], "transform", pnt );
        return 0;
    }

    p.add_msg_if_player( _( "Never mind." ) );
    return cata::nullopt;
}

ret_val<bool> modify_gunmods_actor::can_use( const Character &p, const item &it, bool,
        const tripoint & ) const
{
    if( !p.is_wielding( it ) ) {
        return ret_val<bool>::make_failure( _( "Need to be wielding." ) );
    }
    const auto mods = it.gunmods();

    if( mods.empty() ) {
        return ret_val<bool>::make_failure( _( "Doesn't appear to be modded." ) );
    }

    const bool modifiables = std::any_of( mods.begin(), mods.end(),
                                          std::bind( &item::is_transformable, std::placeholders::_1 ) );

    if( !modifiables ) {
        return ret_val<bool>::make_failure( _( "None of the mods can be modified." ) );
    }

    if( p.is_worn(
            it ) ) { // I don't know if modifying really needs this but its for future proofing.
        return ret_val<bool>::make_failure( _( "Has to be taken off first." ) );
    }

    return ret_val<bool>::make_success();
}

std::unique_ptr<iuse_actor> modify_gunmods_actor::modify_gunmods_actor::clone() const
{
    return std::make_unique<modify_gunmods_actor>( *this );
}

void modify_gunmods_actor::finalize( const itype_id &my_item_type )
{
    if( !item::find_type( my_item_type )->gun ) {
        debugmsg( "Item %s has modify_gunmods_actor actor, but it's a gun.", my_item_type.c_str() );
    }
}

std::unique_ptr<iuse_actor> deploy_tent_actor::clone() const
{
    return std::make_unique<deploy_tent_actor>( *this );
}

void deploy_tent_actor::load( const JsonObject &obj )
{
    assign( obj, "radius", radius );
    assign( obj, "wall", wall );
    assign( obj, "floor", floor );
    assign( obj, "floor_center", floor_center );
    assign( obj, "door_opened", door_opened );
    assign( obj, "door_closed", door_closed );
    assign( obj, "broken_type", broken_type );
}

cata::optional<int> deploy_tent_actor::use( Character &p, item &it, bool, const tripoint & ) const
{
    int diam = 2 * radius + 1;
    if( p.is_mounted() ) {
        p.add_msg_if_player( _( "You cannot do that while mounted." ) );
        return cata::nullopt;
    }
    const cata::optional<tripoint> dir = choose_direction( string_format(
            _( "Put up the %s where (%dx%d clear area)?" ), it.tname(), diam, diam ) );
    if( !dir ) {
        return cata::nullopt;
    }
    const tripoint direction = *dir;

    map &here = get_map();
    // We place the center of the structure (radius + 1)
    // spaces away from the player.
    // First check there's enough room.
    const tripoint center = p.pos() + tripoint( ( radius + 1 ) * direction.x,
                            ( radius + 1 ) * direction.y, 0 );
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &dest : here.points_in_radius( center, radius ) ) {
        if( const optional_vpart_position vp = here.veh_at( dest ) ) {
            add_msg( m_info, _( "The %s is in the way." ), vp->vehicle().name );
            return cata::nullopt;
        }
        if( const Creature *const c = creatures.creature_at( dest ) ) {
            add_msg( m_info, _( "The %s is in the way." ), c->disp_name() );
            return cata::nullopt;
        }
        if( here.impassable( dest ) || !here.has_flag( ter_furn_flag::TFLAG_FLAT, dest ) ) {
            add_msg( m_info, _( "The %s in that direction isn't suitable for placing the %s." ),
                     here.name( dest ), it.tname() );
            return cata::nullopt;
        }
        if( here.has_furn( dest ) ) {
            add_msg( m_info, _( "There is already furniture (%s) there." ), here.furnname( dest ) );
            return cata::nullopt;
        }
    }

    //checks done start activity:
    player_activity new_act = player_activity( tent_placement_activity_actor( to_moves<int>
                              ( 20_minutes ), direction, radius, it, wall, floor, floor_center, door_closed ) );
    get_player_character().assign_activity( new_act, false );

    return 1;
}

bool deploy_tent_actor::check_intact( const tripoint &center ) const
{
    map &here = get_map();
    for( const tripoint &dest : here.points_in_radius( center, radius ) ) {
        const furn_id fid = here.furn( dest );
        if( dest == center && floor_center ) {
            if( fid != *floor_center ) {
                return false;
            }
        } else if( square_dist( dest, center ) < radius ) {
            // So we are inside the tent
            if( fid != floor ) {
                return false;
            }
        } else {
            // We are on the border of the tent
            if( fid != wall && fid != door_opened && fid != door_closed ) {
                return false;
            }
        }
    }
    return true;
}

void weigh_self_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    dump.emplace_back( "DESCRIPTION",
                       _( "Use this item to weigh yourself.  Includes everything you are wearing." ) );
}

cata::optional<int> weigh_self_actor::use( Character &p, item &, bool, const tripoint & ) const
{
    if( p.is_mounted() ) {
        p.add_msg_if_player( m_info, _( "You cannot weigh yourself while mounted." ) );
        return cata::nullopt;
    }
    // this is a weight, either in kgs or in lbs.
    double weight = convert_weight( p.get_weight() );
    if( weight > convert_weight( max_weight ) ) {
        popup( _( "ERROR: Max weight of %.0f %s exceeded" ), convert_weight( max_weight ), weight_units() );
    } else {
        popup( "%.0f %s", weight, weight_units() );
    }
    return 0;
}

void weigh_self_actor::load( const JsonObject &jo )
{
    assign( jo, "max_weight", max_weight );
}

std::unique_ptr<iuse_actor> weigh_self_actor::clone() const
{
    return std::make_unique<weigh_self_actor>( *this );
}

void sew_advanced_actor::load( const JsonObject &obj )
{
    // Mandatory:
    for( const std::string line : obj.get_array( "materials" ) ) {
        materials.emplace( line );
    }
    for( const std::string line : obj.get_array( "clothing_mods" ) ) {
        clothing_mods.emplace_back( line );
    }

    // TODO: Make skill non-mandatory while still erroring on invalid skill
    const std::string skill_string = obj.get_string( "skill" );
    used_skill = skill_id( skill_string );
    if( !used_skill.is_valid() ) {
        obj.throw_error_at( "skill", "Invalid skill" );
    }
}

cata::optional<int> sew_advanced_actor::use( Character &p, item &it, bool, const tripoint & ) const
{
    if( p.is_npc() ) {
        return cata::nullopt;
    }
    if( p.is_mounted() ) {
        p.add_msg_if_player( m_info, _( "You cannot do that while mounted." ) );
        return cata::nullopt;
    }
    if( p.is_underwater() ) {
        p.add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return cata::nullopt;
    }

    if( p.fine_detail_vision_mod() > 4 ) {
        add_msg( m_info, _( "You can't see to sew!" ) );
        return cata::nullopt;
    }

    auto filter = [this]( const item & itm ) {
        return itm.is_armor() && !itm.is_firearm() && !itm.is_power_armor() && !itm.is_gunmod() &&
               itm.made_of_any( materials ) && !itm.has_flag( flag_INTEGRATED );
    };
    // note: if !p.is_npc() then p is avatar.
    item_location loc = game_menus::inv::titled_filter_menu(
                            filter, *p.as_avatar(), _( "Enhance which clothing?" ) );
    if( !loc ) {
        p.add_msg_if_player( m_info, _( "You do not have that item!" ) );
        return cata::nullopt;
    }
    item &mod = *loc;
    if( &mod == &it ) {
        p.add_msg_if_player( m_info,
                             _( "This can be used to repair or modify other items, not itself." ) );
        return cata::nullopt;
    }

    // Gives us an item with the mod added or removed (toggled)
    const auto modded_copy = []( const item & proto, const flag_id & mod_type ) {
        item mcopy = proto;
        if( mcopy.has_own_flag( mod_type ) == 0 ) {
            mcopy.set_flag( mod_type );
        } else {
            mcopy.unset_flag( mod_type );
        }

        return mcopy;
    };

    // Cache available materials
    std::map< itype_id, bool > has_enough;
    const int items_needed = mod.base_volume() / 750_ml + 1;
    const inventory &crafting_inv = p.crafting_inventory();
    const std::function<bool( const item & )> is_filthy_filter = is_crafting_component;

    // Go through all discovered repair items and see if we have any of them available
    for( const clothing_mod &cm : clothing_mods::get_all() ) {
        has_enough[cm.item_string] = crafting_inv.has_amount( cm.item_string, items_needed, false,
                                     is_filthy_filter );
    }

    int mod_count = 0;
    for( const clothing_mod &cm : clothing_mods::get_all() ) {
        mod_count += mod.has_own_flag( cm.flag );
    }

    // We need extra thread to lose it on bad rolls
    const int thread_needed = mod.base_volume() / 125_ml + 10;

    const auto valid_mods = mod.find_armor_data()->valid_mods;

    const auto get_compare_color = [&]( const float before, const float after,
    const bool higher_is_better ) {
        return before == after ? c_unset : ( ( after > before ) == higher_is_better ? c_light_green :
                                             c_red );
    };

    uilist tmenu;
    tmenu.text = _( "How do you want to modify it?" );

    int index = 0;
    for( const clothing_mod_id &cm : clothing_mods ) {
        clothing_mod obj = cm.obj();
        item temp_item = modded_copy( mod, obj.flag );
        temp_item.update_clothing_mod_val();

        bool enab = false;
        std::string prompt;
        if( !mod.has_own_flag( obj.flag ) ) {
            // Mod not already present, check if modification is possible
            if( obj.restricted &&
                std::find( valid_mods.begin(), valid_mods.end(), obj.flag.str() ) == valid_mods.end() ) {
                //~ %1$s: modification desc, %2$s: mod name
                prompt = string_format( _( "Can't %1$s (incompatible with %2$s)" ),
                                        lowercase_first_letter( obj.implement_prompt.translated() ),
                                        mod.tname( 1, false ) );
            } else if( !it.ammo_sufficient( &p, thread_needed ) ) {
                //~ %1$s: modification desc, %2$d: number of thread needed
                prompt = string_format( _( "Can't %1$s (need %2$d thread loaded)" ),
                                        lowercase_first_letter( obj.implement_prompt.translated() ), thread_needed );
            } else if( !has_enough[obj.item_string] ) {
                //~ %1$s: modification desc, %2$d: number of items needed, %3$s: items needed
                prompt = string_format( _( "Can't %1$s (need %2$d %3$s)" ),
                                        lowercase_first_letter( obj.implement_prompt.translated() ),
                                        items_needed, item::nname( obj.item_string, items_needed ) );
            } else {
                // Modification is possible
                enab = true;
                //~ %1$s: modification desc, %2$d: number of items needed, %3$s: items needed, %4$s: number of thread needed
                prompt = string_format( _( "%1$s (%2$d %3$s and %4$d thread)" ),
                                        lowercase_first_letter( obj.implement_prompt.translated() ),
                                        items_needed, item::nname( obj.item_string, items_needed ), thread_needed );
            }

        } else {
            // Mod already present, give option to destroy
            enab = true;
            prompt = obj.destroy_prompt.translated();
        }
        std::string desc;
        desc += colorize( string_format( "%s: %.2f->%.2f\n", _( "Bash" ), mod.bash_resist(),
                                         temp_item.bash_resist() ), get_compare_color( mod.bash_resist(), temp_item.bash_resist(), true ) );
        desc += colorize( string_format( "%s: %.2f->%.2f\n", _( "Cut" ), mod.cut_resist(),
                                         temp_item.cut_resist() ), get_compare_color( mod.cut_resist(), temp_item.cut_resist(), true ) );
        desc += colorize( string_format( "%s: %.2f->%.2f\n", _( "Ballistic" ), mod.bullet_resist(),
                                         temp_item.bullet_resist() ), get_compare_color( mod.bullet_resist(), temp_item.bullet_resist(),
                                                 true ) );
        desc += colorize( string_format( "%s: %.2f->%.2f\n", _( "Acid" ), mod.acid_resist(),
                                         temp_item.acid_resist() ), get_compare_color( mod.acid_resist(), temp_item.acid_resist(), true ) );
        desc += colorize( string_format( "%s: %.2f->%.2f\n", _( "Fire" ), mod.fire_resist(),
                                         temp_item.fire_resist() ), get_compare_color( mod.fire_resist(), temp_item.fire_resist(), true ) );
        desc += colorize( string_format( "%s: %d->%d\n", _( "Warmth" ), mod.get_warmth(),
                                         temp_item.get_warmth() ), get_compare_color( mod.get_warmth(), temp_item.get_warmth(), true ) );
        desc += colorize( string_format( "%s: %d->%d\n", _( "Encumbrance" ), mod.get_avg_encumber( p ),
                                         temp_item.get_avg_encumber( p ) ), get_compare_color( mod.get_avg_encumber( p ),
                                                 temp_item.get_avg_encumber( p ), false ) );

        tmenu.addentry_desc( index++, enab, MENU_AUTOASSIGN, prompt, desc );
    }
    tmenu.textwidth = 80;
    tmenu.desc_enabled = true;
    tmenu.query();
    const int choice = tmenu.ret;

    if( choice < 0 || choice >= static_cast<int>( clothing_mods.size() ) ) {
        return cata::nullopt;
    }

    // The mod player picked
    const flag_id &the_mod = clothing_mods[choice].obj().flag;

    // If the picked mod already exists, player wants to destroy it
    if( mod.has_own_flag( the_mod ) ) {
        if( query_yn( _( "Are you sure?  You will not gain any materials back." ) ) ) {
            mod.unset_flag( the_mod );
        }
        mod.update_clothing_mod_val();

        return 0;
    }

    // Get the id of the material used
    const itype_id &repair_item = clothing_mods[choice].obj().item_string;

    std::vector<item_comp> comps;
    comps.emplace_back( repair_item, items_needed );
    p.moves -= to_moves<int>( 30_seconds * p.fine_detail_vision_mod() );
    p.practice( used_skill, items_needed * 3 + 3 );
    /** @EFFECT_TAILOR randomly improves clothing modification efforts */
    int rn = dice( 3, 2 + p.get_skill_level( used_skill ) ); // Skill
    /** @EFFECT_DEX randomly improves clothing modification efforts */
    rn += rng( 0, p.dex_cur / 2 );                    // Dexterity
    /** @EFFECT_PER randomly improves clothing modification efforts */
    rn += rng( 0, p.per_cur / 2 );                    // Perception
    rn -= mod_count * 10;                              // Other mods

    if( rn <= 8 ) {
        const std::string startdurability = mod.durability_indicator( true );
        const bool destroyed = mod.inc_damage();
        const std::string resultdurability = mod.durability_indicator( true );
        p.add_msg_if_player( m_bad, _( "You damage your %s trying to modify it!  ( %s-> %s)" ),
                             mod.tname( 1, false ), startdurability, resultdurability );
        if( destroyed ) {
            p.add_msg_if_player( m_bad, _( "You destroy it!" ) );
            p.i_rem_keep_contents( &mod );
            p.calc_encumbrance();
            p.calc_discomfort();
        }
        return thread_needed / 2;
    } else if( rn <= 10 ) {
        p.add_msg_if_player( m_bad,
                             _( "You fail to modify the clothing, and you waste thread and materials." ) );
        p.consume_items( comps, 1, is_crafting_component );
        return thread_needed;
    } else if( rn <= 14 ) {
        p.add_msg_if_player( m_mixed, _( "You modify your %s, but waste a lot of thread." ),
                             mod.tname() );
        p.consume_items( comps, 1, is_crafting_component );
        mod.set_flag( the_mod );
        mod.update_clothing_mod_val();
        p.calc_encumbrance();
        p.calc_discomfort();
        return thread_needed;
    }

    p.add_msg_if_player( m_good, _( "You modify your %s!" ), mod.tname() );
    mod.set_flag( the_mod );
    mod.update_clothing_mod_val();
    p.consume_items( comps, 1, is_crafting_component );
    p.calc_encumbrance();
    p.calc_discomfort();
    return thread_needed / 2;
}

std::unique_ptr<iuse_actor> sew_advanced_actor::clone() const
{
    return std::make_unique<sew_advanced_actor>( *this );
}

void change_scent_iuse::load( const JsonObject &obj )
{
    scenttypeid = scenttype_id( obj.get_string( "scent_typeid" ) );
    if( !scenttypeid.is_valid() ) {
        obj.throw_error_at( "scent_typeid", "Invalid scent type id." );
    }
    if( obj.has_array( "effects" ) ) {
        for( JsonObject e : obj.get_array( "effects" ) ) {
            effects.push_back( load_effect_data( e ) );
        }
    }
    assign( obj, "moves", moves );
    assign( obj, "charges_to_use", charges_to_use );
    assign( obj, "scent_mod", scent_mod );
    assign( obj, "duration", duration );
    assign( obj, "waterproof", waterproof );
}

cata::optional<int> change_scent_iuse::use( Character &p, item &it, bool, const tripoint & ) const
{
    p.set_value( "prev_scent", p.get_type_of_scent().c_str() );
    if( waterproof ) {
        p.set_value( "waterproof_scent", "true" );
    }
    p.add_effect( effect_masked_scent, duration, false, scent_mod );
    p.set_type_of_scent( scenttypeid );
    p.mod_moves( -moves );
    add_msg( m_info, _( "You use the %s to mask your scent" ), it.tname() );

    // Apply the various effects.
    for( const effect_data &eff : effects ) {
        p.add_effect( eff.id, eff.duration, eff.bp, eff.permanent );
    }
    return charges_to_use;
}

std::unique_ptr<iuse_actor> change_scent_iuse::clone() const
{
    return std::make_unique<change_scent_iuse>( *this );
}

std::unique_ptr<iuse_actor> effect_on_conditons_actor::clone() const
{
    return std::make_unique<effect_on_conditons_actor>( *this );
}

void effect_on_conditons_actor::load( const JsonObject &obj )
{
    description = obj.get_string( "description" );
    for( JsonValue jv : obj.get_array( "effect_on_conditions" ) ) {
        eocs.emplace_back( effect_on_conditions::load_inline_eoc( jv, "" ) );
    }
}

void effect_on_conditons_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    dump.emplace_back( "DESCRIPTION", description );
}

cata::optional<int> effect_on_conditons_actor::use( Character &p, item &it, bool,
        const tripoint & ) const
{
    Character *char_ptr = nullptr;
    if( avatar *u = p.as_avatar() ) {
        char_ptr = u;
    } else if( npc *n = p.as_npc() ) {
        char_ptr = n;
    }

    item_location loc( *p.as_character(), &it );
    dialogue d( get_talker_for( char_ptr ), get_talker_for( loc ) );
    for( const effect_on_condition_id &eoc : eocs ) {
        if( eoc->type == eoc_type::ACTIVATION ) {
            eoc->activate( d );
        } else {
            debugmsg( "Must use an activation eoc for activation.  If you don't want the effect_on_condition to happen on its own (without the item's involvement), remove the recurrence min and max.  Otherwise, create a non-recurring effect_on_condition for this item with its condition and effects, then have a recurring one queue it." );
        }
    }
    return it.type->charges_to_use();
}
