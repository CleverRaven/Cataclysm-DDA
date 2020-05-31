#include "iuse_actor.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <sstream>

#include "action.h"
#include "activity_handlers.h"
#include "ammo.h"
#include "assign.h"
#include "avatar.h"
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "character_id.h"
#include "clothing_mod.h"
#include "crafting.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "effect.h"
#include "enum_conversions.h"
#include "enums.h"
#include "explosion.h"
#include "field_type.h"
#include "flat_set.h"
#include "game.h"
#include "game_inventory.h"
#include "generic_factory.h"
#include "int_id.h"
#include "inventory.h"
#include "item.h"
#include "item_contents.h"
#include "item_factory.h"
#include "item_group.h"
#include "item_location.h"
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
#include "mutation.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "player.h"
#include "player_activity.h"
#include "pldata.h"
#include "point.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "rng.h"
#include "skill.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "timed_event.h"
#include "translations.h"
#include "trap.h"
#include "ui.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "visitable.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "weather.h"

static const activity_id ACT_FIRSTAID( "ACT_FIRSTAID" );
static const activity_id ACT_RELOAD( "ACT_RELOAD" );
static const activity_id ACT_REPAIR_ITEM( "ACT_REPAIR_ITEM" );
static const activity_id ACT_SPELLCASTING( "ACT_SPELLCASTING" );
static const activity_id ACT_STUDY_SPELL( "ACT_STUDY_SPELL" );
static const activity_id ACT_START_FIRE( "ACT_START_FIRE" );

static const efftype_id effect_asthma( "asthma" );
static const efftype_id effect_bandaged( "bandaged" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_disinfected( "disinfected" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_music( "music" );
static const efftype_id effect_playing_instrument( "playing_instrument" );
static const efftype_id effect_recover( "recover" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_stunned( "stunned" );

static const fault_id fault_bionic_salvaged( "fault_bionic_salvaged" );

static const bionic_id bio_syringe( "bio_syringe" );

static const itype_id itype_barrel_small( "barrel_small" );
static const itype_id itype_brazier( "brazier" );
static const itype_id itype_char_smoker( "char_smoker" );
static const itype_id itype_fire( "fire" );
static const itype_id itype_syringe( "syringe" );
static const itype_id itype_UPS( "UPS" );

static const skill_id skill_fabrication( "fabrication" );
static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_lockpick( "lockpick" );
static const skill_id skill_survival( "survival" );

static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_DEBUG_BIONICS( "DEBUG_BIONICS" );
static const trait_id trait_TOLERANCE( "TOLERANCE" );
static const trait_id trait_LIGHTWEIGHT( "LIGHTWEIGHT" );
static const trait_id trait_PYROMANIA( "PYROMANIA" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_MASOCHIST( "MASOCHIST" );
static const trait_id trait_MASOCHIST_MED( "MASOCHIST_MED" );
static const trait_id trait_MUT_JUNKIE( "MUT_JUNKIE" );
static const trait_id trait_SELFAWARE( "SELFAWARE" );
static const trait_id trait_SMALL_OK( "SMALL_OK" );
static const trait_id trait_SMALL2( "SMALL2" );

static const quality_id qual_LOCKPICK( "LOCKPICK" );

static const std::string flag_FIT( "FIT" );
static const std::string flag_OVERSIZE( "OVERSIZE" );
static const std::string flag_UNDERSIZE( "UNDERSIZE" );
static const std::string flag_VARSIZE( "VARSIZE" );

class npc;

std::unique_ptr<iuse_actor> iuse_transform::clone() const
{
    return std::make_unique<iuse_transform>( *this );
}

void iuse_transform::load( const JsonObject &obj )
{
    obj.read( "target", target, true );

    obj.read( "msg", msg_transform );
    obj.read( "container", container );
    if( obj.has_member( "target_charges" ) && obj.has_member( "rand_target_charges" ) ) {
        obj.throw_error( "Transform actor specified both fixed and random target charges",
                         "target_charges" );
    }
    obj.read( "target_charges", ammo_qty );
    if( obj.has_array( "rand_target_charges" ) ) {
        for( const int charge : obj.get_array( "rand_target_charges" ) ) {
            random_ammo_qty.push_back( charge );
        }
        if( random_ammo_qty.size() < 2 ) {
            obj.throw_error( "You must specify two or more values to choose between", "rand_target_charges" );
        }
    }
    obj.read( "target_ammo", ammo_type );

    obj.read( "countdown", countdown );

    if( !ammo_type.is_empty() && !container.is_empty() ) {
        obj.throw_error( "Transform actor specified both ammo type and container type", "target_ammo" );
    }

    obj.read( "active", active );

    obj.read( "moves", moves );
    if( moves < 0 ) {
        obj.throw_error( "transform actor specified negative moves", "moves" );
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

int iuse_transform::use( player &p, item &it, bool t, const tripoint &pos ) const
{
    if( t ) {
        return 0; // invoked from active item processing, do nothing.
    }

    const bool possess = p.has_item( it ) ||
                         ( it.has_flag( "ALLOWS_REMOTE_USE" ) && square_dist( p.pos(), pos ) == 1 );

    if( possess && need_worn && !p.is_worn( it ) ) {
        p.add_msg_if_player( m_info, _( "You need to wear the %1$s before activating it." ), it.tname() );
        return 0;
    }
    if( possess && need_wielding && !p.is_wielding( it ) ) {
        p.add_msg_if_player( m_info, _( "You need to wield the %1$s before activating it." ), it.tname() );
        return 0;
    }
    if( need_charges && it.units_remaining( p ) < need_charges ) {
        if( possess ) {
            p.add_msg_if_player( m_info, need_charges_msg, it.tname() );
        }
        return 0;
    }

    if( need_fire && possess ) {
        if( !p.use_charges_if_avail( itype_fire, need_fire ) ) {
            p.add_msg_if_player( m_info, need_fire_msg, it.tname() );
            return 0;
        }
        if( p.is_underwater() ) {
            p.add_msg_if_player( m_info, _( "You can't do that while underwater" ) );
            return 0;
        }
    }

    if( possess && !msg_transform.empty() ) {
        p.add_msg_if_player( m_neutral, msg_transform, it.tname() );
    }

    if( possess ) {
        p.moves -= moves;
    }

    item obj_copy( it );
    item *obj;
    // defined here to allow making a new item assigned to the pointer
    item obj_it;
    if( container.is_empty() ) {
        obj = &it.convert( target );
        if( ammo_qty >= 0 || !random_ammo_qty.empty() ) {
            int qty;
            if( !random_ammo_qty.empty() ) {
                const auto index = rng( 1, random_ammo_qty.size() - 1 );
                qty = rng( random_ammo_qty[index - 1], random_ammo_qty[index] );
            } else {
                qty = ammo_qty;
            }
            if( !ammo_type.is_empty() ) {
                obj->ammo_set( ammo_type, qty );
            } else if( !obj->ammo_current().is_null() ) {
                obj->ammo_set( obj->ammo_current(), qty );
            } else {
                obj->set_countdown( qty );
            }
        }
    } else {
        it.convert( container );
        obj_it = item( target, calendar::turn, std::max( ammo_qty, 1 ) );
        obj = &obj_it;
        it.put_in( *obj, item_pocket::pocket_type::CONTAINER );
    }
    if( p.is_worn( *obj ) ) {
        p.reset_encumbrance();
        p.update_bodytemp();
        p.on_worn_item_transform( obj_copy, *obj );
    }
    obj->item_counter = countdown > 0 ? countdown : obj->type->countdown_interval;
    obj->active = active || obj->item_counter;

    return 0;
}

ret_val<bool> iuse_transform::can_use( const Character &p, const item &, bool,
                                       const tripoint & ) const
{
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
    return ret_val<bool>::make_failure( ngettext( "You need a tool with %s.", "You need tools with %s.",
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
    if( it.has_flag( "FIT" ) ) {
        dummy.item_tags.insert( "FIT" );
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

int unpack_actor::use( player &p, item &it, bool, const tripoint & ) const
{
    std::vector<item> items = item_group::items_from( unpack_group, calendar::turn );
    item last_armor;

    p.add_msg_if_player( _( "You unpack the %s." ), it.tname() );

    for( item &content : items ) {
        if( content.is_armor() ) {
            if( items_fit ) {
                content.set_flag( "FIT" );
            } else if( content.typeId() == last_armor.typeId() ) {
                if( last_armor.has_flag( "FIT" ) ) {
                    content.set_flag( "FIT" );
                } else if( !last_armor.has_flag( "FIT" ) ) {
                    content.unset_flag( "FIT" );
                }
            }
            last_armor = content;
        }

        if( content.contents.total_container_capacity() >= filthy_vol_threshold &&
            it.has_flag( "FILTHY" ) ) {
            content.set_flag( "FILTHY" );
        }

        g->m.add_item_or_charges( p.pos(), content );
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

int countdown_actor::use( player &p, item &it, bool t, const tripoint &pos ) const
{
    if( t ) {
        return 0;
    }

    if( it.active ) {
        return 0;
    }

    if( p.sees( pos ) && !message.empty() ) {
        p.add_msg_if_player( m_neutral, _( message ), it.tname() );
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
        return name;
    }
    return iuse_actor::get_name();
}

void countdown_actor::info( const item &it, std::vector<iteminfo> &dump ) const
{
    dump.emplace_back( "TOOL", _( "Countdown: " ),
                       interval > 0 ? interval : it.type->countdown_interval );
    const auto countdown_actor = it.type->countdown_action.get_actor_ptr();
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
    std::vector<tripoint> result;
    for( const auto &p : closest_tripoints_first( center, radius ) ) {
        if( g->m.impassable( p ) ) {
            continue;
        }
        if( p != center ) {
            if( !g->m.clear_path( center, p, radius, 1, 100 ) ) {
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
        auto expl = obj.get_object( "explosion" );
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

int explosion_iuse::use( player &p, item &it, bool t, const tripoint &pos ) const
{
    if( t ) {
        if( sound_volume >= 0 ) {
            sounds::sound( pos, sound_volume, sounds::sound_t::alarm,
                           sound_msg.empty() ? _( "Tick." ) : _( sound_msg ), true, "misc", "bomb_ticking" );
        }
        return 0;
    }
    if( it.charges > 0 ) {
        if( p.has_item( it ) ) {
            if( no_deactivate_msg.empty() ) {
                p.add_msg_if_player( m_warning,
                                     _( "You've already set the %s's timer you might want to get away from it." ), it.tname() );
            } else {
                p.add_msg_if_player( m_info, _( no_deactivate_msg ), it.tname() );
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
    if( fields_radius >= 0 && fields_type.id() ) {
        std::vector<tripoint> gas_sources = points_for_gas_cloud( pos, fields_radius );
        for( auto &gas_source : gas_sources ) {
            const int field_intensity = rng( fields_min_intensity, fields_max_intensity );
            g->m.add_field( gas_source, fields_type, field_intensity, 1_turns );
        }
    }
    if( scrambler_blast_radius >= 0 ) {
        for( const tripoint &dest : g->m.points_in_radius( pos, scrambler_blast_radius ) ) {
            explosion_handler::scrambler_blast( dest );
        }
    }
    if( emp_blast_radius >= 0 ) {
        for( const tripoint &dest : g->m.points_in_radius( pos, emp_blast_radius ) ) {
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
    const auto &sd = explosion.shrapnel;
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

int unfold_vehicle_iuse::use( player &p, item &it, bool, const tripoint & ) const
{
    if( p.is_underwater() ) {
        p.add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    if( p.is_mounted() ) {
        p.add_msg_if_player( m_info, _( "You cannot do that while mounted." ) );
        return 0;
    }
    for( const auto &tool : tools_needed ) {
        // Amount == -1 means need one, but don't consume it.
        if( !p.has_amount( tool.first, 1 ) ) {
            p.add_msg_if_player( _( "You need %s to do it!" ),
                                 item::nname( tool.first ) );
            return 0;
        }
    }

    vehicle *veh = g->m.add_vehicle( vehicle_id, p.pos(), 0, 0, 0, false );
    if( veh == nullptr ) {
        p.add_msg_if_player( m_info, _( "There's no room to unfold the %s." ), it.tname() );
        return 0;
    }
    veh->set_owner( p );

    // Mark the vehicle as foldable.
    veh->tags.insert( "convertible" );
    // Store the id of the item the vehicle is made of.
    veh->tags.insert( std::string( "convertible:" ) + it.typeId().str() );
    if( !unfold_msg.empty() ) {
        p.add_msg_if_player( _( unfold_msg ), it.tname() );
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
        for( auto &elem : veh->parts ) {
            int tmp;
            veh_data >> tmp;
            veh->set_hp( elem, tmp );
        }
    } else {
        try {
            JsonIn json( veh_data );
            // Load parts into a temporary vector to not override
            // cached values (like precalc, passenger_id, ...)
            std::vector<vehicle_part> parts;
            json.read( parts );
            for( size_t i = 0; i < parts.size() && i < veh->parts.size(); i++ ) {
                const vehicle_part &src = parts[i];
                vehicle_part &dst = veh->parts[i];
                // and now only copy values, that are
                // expected to be consistent.
                veh->set_hp( dst, src.hp() );
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
        time = read_from_json_string<time_duration>( *e.get_raw( "duration" ), time_duration::units );
    } else {
        time = time_duration::from_turns( e.get_int( "duration", 0 ) );
    }
    return effect_data( efftype_id( e.get_string( "id" ) ), time,
                        get_body_part_token( e.get_string( "bp", "NUM_BP" ) ), e.get_bool( "permanent", false ) );
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
        auto lo = vit.get_int( 1 );
        auto hi = vit.size() >= 3 ? vit.get_int( 2 ) : lo;
        vitamins.emplace( vitamin_id( vit.get_string( 0 ) ), std::make_pair( lo, hi ) );
    }

    used_up_item = obj.get_string( "used_up_item", used_up_item );

}

void consume_drug_iuse::info( const item &, std::vector<iteminfo> &dump ) const
{
    const std::string vits = enumerate_as_string( vitamins.begin(), vitamins.end(),
    []( const decltype( vitamins )::value_type & v ) {
        const time_duration rate = g->u.vitamin_rate( v.first );
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

int consume_drug_iuse::use( player &p, item &it, bool, const tripoint & ) const
{
    auto need_these = tools_needed;
    if( need_these.count( itype_syringe ) && p.has_bionic( bio_syringe ) ) {
        need_these.erase( itype_syringe ); // no need for a syringe with bionics like these!
    }
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
    for( const auto &eff : effects ) {
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
    for( const auto &field : fields_produced ) {
        const field_type_id fid = field_type_id( field.first );
        for( int i = 0; i < 3; i++ ) {
            g->m.add_field( {p.posx() + static_cast<int>( rng( -2, 2 ) ), p.posy() + static_cast<int>( rng( -2, 2 ) ), p.posz()},
                            fid,
                            field.second );
        }
    }

    // for vitamins that accumulate (max > 0) multivitamins risk causing hypervitaminosis
    for( const auto &v : vitamins ) {
        // players with mutations that remove the requirement for a vitamin cannot suffer accumulation of it
        p.vitamin_mod( v.first, rng( v.second.first, v.second.second ),
                       p.vitamin_rate( v.first ) <= 0_turns );
    }

    // Output message.
    p.add_msg_if_player( _( activation_message ), it.type_name( 1 ) );
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
    not_ready_msg = obj.get_string( "not_ready_msg" );
    transform_age = obj.get_int( "transform_age" );
}

int delayed_transform_iuse::time_to_do( const item &it ) const
{
    // TODO: change return type to time_duration
    return transform_age - to_turns<int>( it.age() );
}

int delayed_transform_iuse::use( player &p, item &it, bool t, const tripoint &pos ) const
{
    if( time_to_do( it ) > 0 ) {
        p.add_msg_if_player( m_info, _( not_ready_msg ) );
        return 0;
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
    if( obj.has_array( "skills" ) ) {
        JsonArray skills_ja = obj.get_array( "skills" );
        for( JsonValue s : skills_ja ) {
            skills.emplace( skill_id( s.get_string() ) );
        }
    }
}

int place_monster_iuse::use( player &p, item &it, bool, const tripoint & ) const
{
    shared_ptr_fast<monster> newmon_ptr = make_shared_fast<monster>( mtypeid );
    monster &newmon = *newmon_ptr;
    newmon.init_from_item( it );
    if( place_randomly ) {
        // place_critter_around returns the same pointer as its parameter (or null)
        if( !g->place_critter_around( newmon_ptr, p.pos(), 1 ) ) {
            p.add_msg_if_player( m_info, _( "There is no adjacent square to release the %s in!" ),
                                 newmon.name() );
            return 0;
        }
    } else {
        const std::string query = string_format( _( "Place the %s where?" ), newmon.name() );
        const cata::optional<tripoint> pnt_ = choose_adjacent( query );
        if( !pnt_ ) {
            return 0;
        }
        // place_critter_at returns the same pointer as its parameter (or null)
        if( !g->place_critter_at( newmon_ptr, *pnt_ ) ) {
            p.add_msg_if_player( m_info, _( "You cannot place a %s there." ), newmon.name() );
            return 0;
        }
    }
    p.moves -= moves;
    if( !newmon.has_flag( MF_INTERIOR_AMMO ) ) {
        for( auto &amdef : newmon.ammo ) {
            item ammo_item( amdef.first, 0 );
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
            p.add_msg_if_player( ngettext( "You load %1$d x %2$s round into the %3$s.",
                                           "You load %1$d x %2$s rounds into the %3$s.", ammo_item.charges ),
                                 ammo_item.charges, ammo_item.type_name( ammo_item.charges ),
                                 newmon.name() );
            amdef.second = ammo_item.charges;
        }
    } else {
        newmon.ammo = newmon.type->starting_ammo;
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
            p.add_msg_if_player( m_bad, "%s", _( hostile_msg ) );
        }
    } else {
        if( friendly_msg.empty() ) {
            p.add_msg_if_player( m_warning, _( "You deploy the %s." ), newmon.name() );
        } else {
            p.add_msg_if_player( m_warning, "%s", _( friendly_msg ) );
        }
        newmon.friendly = -1;
    }
    return 1;
}

std::unique_ptr<iuse_actor> ups_based_armor_actor::clone() const
{
    return std::make_unique<ups_based_armor_actor>( *this );
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

int place_npc_iuse::use( player &p, item &, bool, const tripoint & ) const
{
    cata::optional<tripoint> target_pos;
    if( place_randomly ) {
        const tripoint_range target_range = points_in_radius( p.pos(), 1 );
        target_pos = random_point( target_range, []( const tripoint & t ) {
            return !g->m.passable( t );
        } );
    } else {
        const std::string query = _( "Place npc where?" );
        target_pos = choose_adjacent( _( "Place npc where?" ) );
    }
    if( !target_pos ) {
        return 0;
    }
    if( !g->m.passable( target_pos.value() ) ) {
        p.add_msg_if_player( m_info, _( "There is no square to spawn npc in!" ) );
        return 0;
    }

    g->m.place_npc( target_pos.value().xy(), npc_class_id );
    p.mod_moves( -moves );
    p.add_msg_if_player( m_info, "%s", _( summon_msg ) );
    return 1;
}

void ups_based_armor_actor::load( const JsonObject &obj )
{
    obj.read( "activate_msg", activate_msg );
    obj.read( "deactive_msg", deactive_msg );
    obj.read( "out_of_power_msg", out_of_power_msg );
}

static bool has_powersource( const item &i, const player &p )
{
    if( i.is_power_armor() && p.can_interface_armor() && p.has_power() ) {
        return true;
    }
    return p.has_charges( itype_UPS, 1 );
}

int ups_based_armor_actor::use( player &p, item &it, bool t, const tripoint & ) const
{
    if( t ) {
        // Normal, continuous usage, do nothing. The item is *not* charge-based.
        return 0;
    }
    if( p.get_item_position( &it ) >= -1 ) {
        p.add_msg_if_player( m_info, _( "You should wear the %s before activating it." ),
                             it.tname() );
        return 0;
    }
    if( !it.active && !has_powersource( it, p ) ) {
        p.add_msg_if_player( m_info,
                             _( "You need some source of power for your %s (a simple UPS will do)." ), it.tname() );
        if( it.is_power_armor() ) {
            p.add_msg_if_player( m_info,
                                 _( "There is also a certain bionic that helps with this kind of armor." ) );
        }
        return 0;
    }
    it.active = !it.active;
    p.reset_encumbrance();
    if( it.active ) {
        if( activate_msg.empty() ) {
            p.add_msg_if_player( m_info, _( "You activate your %s." ), it.tname() );
        } else {
            p.add_msg_if_player( m_info, _( activate_msg ), it.tname() );
        }
    } else {
        if( deactive_msg.empty() ) {
            p.add_msg_if_player( m_info, _( "You deactivate your %s." ), it.tname() );
        } else {
            p.add_msg_if_player( m_info, _( deactive_msg ), it.tname() );
        }
    }
    return 0;
}

std::unique_ptr<iuse_actor> pick_lock_actor::clone() const
{
    return std::make_unique<pick_lock_actor>( *this );
}

void pick_lock_actor::load( const JsonObject &obj )
{
    pick_quality = obj.get_int( "pick_quality" );
}

int pick_lock_actor::use( player &p, item &it, bool, const tripoint & ) const
{
    if( p.is_npc() ) {
        return 0;
    }

    avatar &you = dynamic_cast<avatar &>( p );
    cata::optional<tripoint> target = lockpick_activity_actor::select_location( you );
    if( !target.has_value() ) {
        return 0;
    }

    /** @EFFECT_DEX speeds up door lock picking */
    /** @EFFECT_LOCKPICK speeds up door lock picking */
    const int duration = std::max( to_moves<int>( 10_seconds ),
                                   to_moves<int>( 10_minutes - time_duration::from_minutes( it.get_quality( qual_LOCKPICK ) ) ) -
                                   ( p.dex_cur + p.get_skill_level( skill_lockpick ) ) * 2300 );
    you.assign_activity( lockpick_activity_actor( duration, item_location( p, &it ), cata::nullopt,
                         g->m.getabs( *target ) ) );
    return it.type->charges_to_use();
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
    if( the_furn.has_flag( "BUTCHER_EQ" ) ) {
        can_function_as.emplace_back(
            _( "a place to hang <info>corpses for butchering</info>" ) );
    }
    if( the_furn.has_flag( "FLAT_SURF" ) ) {
        can_function_as.emplace_back(
            _( "a flat surface to <info>butcher</info> onto or <info>eat meals</info> from" ) );
    }
    if( the_furn.has_flag( "CAN_SIT" ) ) {
        can_function_as.emplace_back( _( "a place to <info>sit</info>" ) );
    }
    if( the_furn.has_flag( "HIDE_PLACE" ) ) {
        can_function_as.emplace_back( _( "a place to <info>hide</info>" ) );
    }
    if( the_furn.has_flag( "FIRE_CONTAINER" ) ) {
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

int deploy_furn_actor::use( player &p, item &it, bool, const tripoint &pos ) const
{
    if( p.is_mounted() ) {
        p.add_msg_if_player( m_info, _( "You cannot do that while mounted." ) );
        return 0;
    }
    tripoint pnt = pos;
    if( pos == p.pos() ) {
        if( const cata::optional<tripoint> pnt_ = choose_adjacent( _( "Deploy where?" ) ) ) {
            pnt = *pnt_;
        } else {
            return 0;
        }
    }

    if( pnt == p.pos() ) {
        p.add_msg_if_player( m_info,
                             _( "You attempt to become one with the furniture.  It doesn't work." ) );
        return 0;
    }

    optional_vpart_position veh_there = g->m.veh_at( pnt );
    if( veh_there.has_value() ) {
        // TODO: check for protrusion+short furniture, wheels+tiny furniture, NOCOLLIDE flag, etc.
        // and/or integrate furniture deployment with construction (which already seems to perform these checks sometimes?)
        p.add_msg_if_player( m_info, _( "The space under %s is too cramped to deploy a %s in." ),
                             veh_there.value().vehicle().disp_name(), it.tname() );
        return 0;
    }

    // For example: dirt = 2, long grass = 3
    if( g->m.move_cost( pnt ) != 2 && g->m.move_cost( pnt ) != 3 ) {
        p.add_msg_if_player( m_info, _( "You can't deploy a %s there." ), it.tname() );
        return 0;
    }

    if( g->m.has_furn( pnt ) ) {
        p.add_msg_if_player( m_info, _( "There is already furniture at that location." ) );
        return 0;
    }

    g->m.furn_set( pnt, furn_type );
    p.mod_moves( to_turns<int>( 2_seconds ) );
    return 1;
}

std::unique_ptr<iuse_actor> reveal_map_actor::clone() const
{
    return std::make_unique<reveal_map_actor>( *this );
}

void reveal_map_actor::load( const JsonObject &obj )
{
    radius = obj.get_int( "radius" );
    message = obj.get_string( "message" );
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
        omt_types.push_back( std::make_pair( ter, ter_match_type ) );
    }
}

void reveal_map_actor::reveal_targets( const tripoint &center,
                                       const std::pair<std::string, ot_match_type> &target,
                                       int reveal_distance ) const
{
    const auto places = overmap_buffer.find_all( center, target.first, radius, false,
                        target.second );
    for( auto &place : places ) {
        overmap_buffer.reveal( place, reveal_distance );
    }
}

int reveal_map_actor::use( player &p, item &it, bool, const tripoint & ) const
{
    if( it.already_used_by_player( p ) ) {
        p.add_msg_if_player( _( "There isn't anything new on the %s." ), it.tname() );
        return 0;
    } else if( g->get_levz() < 0 ) {
        p.add_msg_if_player( _( "You should read your %s when you get to the surface." ),
                             it.tname() );
        return 0;
    } else if( p.fine_detail_vision_mod() > 4 ) {
        p.add_msg_if_player( _( "It's too dark to read." ) );
        return 0;
    }
    const tripoint &center = it.get_var( "reveal_map_center_omt",
                                         p.global_omt_location() );
    for( auto &omt : omt_types ) {
        for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
            reveal_targets( tripoint( center.xy(), z ), omt, 0 );
        }
    }
    if( !message.empty() ) {
        p.add_msg_if_player( m_good, "%s", _( message ) );
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

bool firestarter_actor::prep_firestarter_use( const player &p, tripoint &pos )
{
    // checks for fuel are handled by use and the activity, not here
    if( pos == p.pos() ) {
        if( const cata::optional<tripoint> pnt_ = choose_adjacent( _( "Light where?" ) ) ) {
            pos = *pnt_;
        } else {
            g->refresh_all();
            return false;
        }
    }
    if( pos == p.pos() ) {
        p.add_msg_if_player( m_info, _( "You would set yourself on fire." ) );
        p.add_msg_if_player( _( "But you're already smokin' hot." ) );
        return false;
    }
    if( g->m.get_field( pos, fd_fire ) ) {
        // check if there's already a fire
        p.add_msg_if_player( m_info, _( "There is already a fire." ) );
        return false;
    }
    // Check for a brazier.
    bool has_unactivated_brazier = false;
    for( const auto &i : g->m.i_at( pos ) ) {
        if( i.typeId() == itype_brazier ) {
            has_unactivated_brazier = true;
        }
    }
    return !has_unactivated_brazier ||
           query_yn(
               _( "There's a brazier there but you haven't set it up to contain the fire.  Continue?" ) );
}

void firestarter_actor::resolve_firestarter_use( player &p, const tripoint &pos )
{
    if( g->m.add_field( pos, fd_fire, 1, 10_minutes ) ) {
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

    if( it.ammo_remaining() < it.ammo_required() ) {
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
    if( ( g->weather.weather == WEATHER_CLEAR || g->weather.weather == WEATHER_SUNNY ) &&
        light_level >= 60.0f && !g->m.has_flag( TFLAG_INDOORS, pos ) ) {
        return std::pow( light_level / 80.0f, 8 );
    }

    return 0.0f;
}

int firestarter_actor::moves_cost_by_fuel( const tripoint &pos ) const
{
    if( g->m.flammable_items_at( pos, 100 ) ) {
        return moves_cost_fast;
    }

    if( g->m.flammable_items_at( pos, 10 ) ) {
        return ( moves_cost_slow + moves_cost_fast ) / 2;
    }

    return moves_cost_slow;
}

int firestarter_actor::use( player &p, item &it, bool t, const tripoint &spos ) const
{
    if( t ) {
        return 0;
    }

    tripoint pos = spos;
    float light = light_mod( p.pos() );
    if( !prep_firestarter_use( p, pos ) ) {
        return 0;
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
        p.add_msg_if_player( m_info, need_sunlight ?
                             _( "If the current weather holds, it will take around %d minutes to light a fire." ) :
                             _( "At your skill level, it will take around %d minutes to light a fire." ),
                             moves / to_moves<int>( 1_minutes ) );
    } else if( moves < to_moves<int>( 2_turns ) && g->m.is_flammable( pos ) ) {
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
    p.activity.targets.push_back( item_location( p, &it ) );
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

int salvage_actor::use( player &p, item &it, bool t, const tripoint & ) const
{
    if( t ) {
        return 0;
    }

    auto item_loc = game_menus::inv::salvage( p, this );
    if( !item_loc ) {
        add_msg( _( "Never mind." ) );
        return 0;
    }

    if( !try_to_cut_up( p, *item_loc.get_item() ) ) {
        // Messages should have already been displayed.
        return 0;
    }

    return cut_up( p, it, item_loc );
}

static const units::volume minimal_volume_to_cut = 250_ml;

int salvage_actor::time_to_cut_up( const item &it ) const
{
    int count = it.volume() / minimal_volume_to_cut;
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
    if( !it.contents.empty() ) {
        return false;
    }
    if( it.volume() < minimal_volume_to_cut ) {
        return false;
    }

    return true;
}

// it here is the item that is a candidate for being chopped up.
// This is the former valid_to_cut_up with all the messages and queries
bool salvage_actor::try_to_cut_up( player &p, item &it ) const
{
    int pos = p.get_item_position( &it );

    if( it.is_null() ) {
        add_msg( m_info, _( "You do not have that item." ) );
        return false;
    }
    // There must be some historical significance to these items.
    if( !it.is_salvageable() ) {
        add_msg( m_info, _( "Can't salvage anything from %s." ), it.tname() );
        if( p.rate_action_disassemble( it ) != hint_rating::cant ) {
            add_msg( m_info, _( "Try disassembling the %s instead." ), it.tname() );
        }
        return false;
    }

    if( !it.only_made_of( material_whitelist ) ) {
        add_msg( m_info, _( "The %s is made of material that cannot be cut up." ), it.tname() );
        return false;
    }
    if( !it.contents.empty() ) {
        add_msg( m_info, _( "Please empty the %s before cutting it up." ), it.tname() );
        return false;
    }
    if( it.volume() < minimal_volume_to_cut ) {
        add_msg( m_info, _( "The %s is too small to salvage material from." ), it.tname() );
        return false;
    }
    // Softer warnings at the end so we don't ask permission and then tell them no.
    if( &it == &p.weapon ) {
        if( !query_yn( _( "You are wielding that, are you sure?" ) ) ) {
            return false;
        }
    } else if( pos == INT_MIN ) {
        // Not in inventory
        return true;
    } else if( pos < -1 ) {
        if( !query_yn( _( "You're wearing that, are you sure?" ) ) ) {
            return false;
        }
    }

    return true;
}

// function returns charges from it during the cutting process of the *cut.
// it cuts
// cut gets cut
int salvage_actor::cut_up( player &p, item &it, item_location &cut ) const
{
    const bool filthy = cut.get_item()->is_filthy();
    // total number of raw components == total volume of item.
    // This can go awry if there is a volume / recipe mismatch.
    int count = cut.get_item()->volume() / minimal_volume_to_cut;
    // Chance of us losing a material component to entropy.
    /** @EFFECT_FABRICATION reduces chance of losing components when cutting items up */
    int entropy_threshold = std::max( 5, 10 - p.get_skill_level( skill_fabrication ) );
    // What material components can we get back?
    std::vector<material_id> cut_material_components = cut.get_item()->made_of();
    // What materials do we salvage (ids and counts).
    std::map<itype_id, int> materials_salvaged;

    // Final just in case check (that perhaps was not done elsewhere);
    if( cut.get_item() == &it ) {
        add_msg( m_info, _( "You can not cut the %s with itself." ), it.tname() );
        return 0;
    }
    if( !cut.get_item()->contents.empty() ) {
        // Should have been ensured by try_to_cut_up
        debugmsg( "tried to cut a non-empty item %s", cut.get_item()->tname() );
        return 0;
    }

    // Time based on number of components.
    p.moves -= moves_per_part * count;
    // Not much practice, and you won't get very far ripping things up.
    p.practice( skill_fabrication, rng( 0, 5 ), 1 );

    // Higher fabrication, less chance of entropy, but still a chance.
    if( rng( 1, 10 ) <= entropy_threshold ) {
        count -= 1;
    }
    // Fail dex roll, potentially lose more parts.
    /** @EFFECT_DEX randomly reduces component loss when cutting items up */
    if( dice( 3, 4 ) > p.dex_cur ) {
        count -= rng( 0, 2 );
    }
    // If more than 1 material component can still be salvaged,
    // chance of losing more components if the item is damaged.
    // If the item being cut is not damaged, no additional losses will be incurred.
    if( count > 0 && cut.get_item()->damage() > 0 ) {
        float component_success_chance = std::min( std::pow( 0.8, cut.get_item()->damage_level( 4 ) ),
                                         1.0 );
        for( int i = count; i > 0; i-- ) {
            if( component_success_chance < rng_float( 0, 1 ) ) {
                count--;
            }
        }
    }

    // Decided to split components evenly. Since salvage will likely change
    // soon after I write this, I'll go with the one that is cleaner.
    for( const material_id &material : cut_material_components ) {
        if( const auto id = material->salvaged_into() ) {
            materials_salvaged[*id] = std::max( 0, count / static_cast<int>( cut_material_components.size() ) );
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
    p.reset_encumbrance();

    for( const auto &salvaged : materials_salvaged ) {
        itype_id mat_name = salvaged.first;
        int amount = salvaged.second;
        item result( mat_name, calendar::turn );
        if( amount > 0 ) {
            add_msg( m_good, ngettext( "Salvaged %1$i %2$s.", "Salvaged %1$i %2$s.", amount ),
                     amount, result.display_name( amount ) );
            if( filthy ) {
                result.item_tags.insert( "FILTHY" );
            }
            if( cut_type == item_location::type::character ) {
                p.i_add_or_drop( result, amount );
            } else {
                for( int i = 0; i < amount; i++ ) {
                    g->m.spawn_an_item( pos.xy(), result, amount, 0 );
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
        std::string lower_verb = verb.translated();
        std::transform( lower_verb.begin(), lower_verb.end(), lower_verb.begin(), ::tolower );
        add_msg( m_info, _( "You can't %1$s %2$s because of the material it is made of." ),
                 lower_verb, cut.display_name() );
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

int inscribe_actor::use( player &p, item &it, bool t, const tripoint & ) const
{
    if( t ) {
        return 0;
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
        return 0;
    }

    if( choice == 0 ) {
        const cata::optional<tripoint> dest_ = choose_adjacent( _( "Write where?" ) );
        if( !dest_ ) {
            return 0;
        }
        return iuse::handle_ground_graffiti( p, &it, string_format( _( "%s what?" ), verb ),
                                             dest_.value() );
    }

    item_location loc = game_menus::inv::titled_menu( g->u, _( "Inscribe which item?" ) );
    if( !loc ) {
        p.add_msg_if_player( m_info, _( "Never mind." ) );
        return 0;
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

    return 0;
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
    dummy.bleed = 1.0f;
    dummy.bite = 0.5f;
    dummy.move_cost = 100;
    return dummy;
}

bool cauterize_actor::cauterize_effect( player &p, item &it, bool force )
{
    // TODO: Make this less hacky
    static const heal_actor dummy = prepare_dummy();
    hp_part hpart = dummy.use_healing_item( p, p, it, force );
    if( hpart != num_hp_parts ) {
        p.add_msg_if_player( m_neutral, _( "You cauterize yourself." ) );
        if( !( p.has_trait( trait_NOPAIN ) ) ) {
            p.mod_pain( 15 );
            p.add_msg_if_player( m_bad, _( "It hurts like hell!" ) );
        } else {
            p.add_msg_if_player( m_neutral, _( "It itches a little." ) );
        }
        const body_part bp = player::hp_to_bp( hpart );
        if( p.has_effect( effect_bite, bp ) ) {
            p.add_effect( effect_bite, 260_minutes, bp, true );
        }

        p.moves = 0;
        return true;
    }

    return false;
}

int cauterize_actor::use( player &p, item &it, bool t, const tripoint & ) const
{
    if( t ) {
        return 0;
    }
    if( p.is_mounted() ) {
        p.add_msg_if_player( m_info, _( "You cannot do that while mounted." ) );
        return 0;
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
        return 0;
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
        if( !it.units_sufficient( p ) ) {
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
    success_message     = obj.get_string( "success_message", "hsss" );
    lacks_fuel_message  = obj.get_string( "lacks_fuel_message" );
    failure_message     = obj.get_string( "failure_message", "hsss" );
    noise               = obj.get_int( "noise", 0 );
    moves               = obj.get_int( "moves", 0 );
    success_chance      = obj.get_int( "success_chance", INT_MIN );
}

std::unique_ptr<iuse_actor> fireweapon_off_actor::clone() const
{
    return std::make_unique<fireweapon_off_actor>( *this );
}

int fireweapon_off_actor::use( player &p, item &it, bool t, const tripoint & ) const
{
    if( t ) {
        return 0;
    }

    if( it.charges <= 0 ) {
        p.add_msg_if_player( _( lacks_fuel_message ) );
        return 0;
    }

    p.moves -= moves;
    if( rng( 0, 10 ) - it.damage_level( 4 ) > success_chance && !p.is_underwater() ) {
        if( noise > 0 ) {
            sounds::sound( p.pos(), noise, sounds::sound_t::combat, _( success_message ) );
        } else {
            p.add_msg_if_player( _( success_message ) );
        }

        it.convert( target_id );
        it.active = true;
    } else if( !failure_message.empty() ) {
        p.add_msg_if_player( m_bad, _( failure_message ) );
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
    noise_message                   = obj.get_string( "noise_message", "hsss" );
    voluntary_extinguish_message    = obj.get_string( "voluntary_extinguish_message" );
    charges_extinguish_message      = obj.get_string( "charges_extinguish_message" );
    water_extinguish_message        = obj.get_string( "water_extinguish_message" );
    noise                           = obj.get_int( "noise", 0 );
    noise_chance                    = obj.get_int( "noise_chance", 1 );
    auto_extinguish_chance          = obj.get_int( "auto_extinguish_chance", 0 );
    if( auto_extinguish_chance > 0 ) {
        auto_extinguish_message         = obj.get_string( "auto_extinguish_message" );
    }
}

std::unique_ptr<iuse_actor> fireweapon_on_actor::clone() const
{
    return std::make_unique<fireweapon_on_actor>( *this );
}

int fireweapon_on_actor::use( player &p, item &it, bool t, const tripoint & ) const
{
    bool extinguish = true;
    if( it.charges == 0 ) {
        p.add_msg_if_player( m_bad, _( charges_extinguish_message ) );
    } else if( p.is_underwater() ) {
        p.add_msg_if_player( m_bad, _( water_extinguish_message ) );
    } else if( auto_extinguish_chance > 0 && one_in( auto_extinguish_chance ) ) {
        p.add_msg_if_player( m_bad, _( auto_extinguish_message ) );
    } else if( !t ) {
        p.add_msg_if_player( _( voluntary_extinguish_message ) );
    } else {
        extinguish = false;
    }

    if( extinguish ) {
        it.deactivate( &p, false );

    } else if( one_in( noise_chance ) ) {
        if( noise > 0 ) {
            sounds::sound( p.pos(), noise, sounds::sound_t::combat, _( noise_message ) );
        } else {
            p.add_msg_if_player( _( noise_message ) );
        }
    }

    return it.type->charges_to_use();
}

void manualnoise_actor::load( const JsonObject &obj )
{
    no_charges_message  = obj.get_string( "no_charges_message" );
    use_message         = obj.get_string( "use_message" );
    noise_message       = obj.get_string( "noise_message", "hsss" );
    noise_id            = obj.get_string( "noise_id", "misc" );
    noise_variant       = obj.get_string( "noise_variant", "default" );
    noise               = obj.get_int( "noise", 0 );
    moves               = obj.get_int( "moves", 0 );
}

std::unique_ptr<iuse_actor> manualnoise_actor::clone() const
{
    return std::make_unique<manualnoise_actor>( *this );
}

int manualnoise_actor::use( player &p, item &it, bool t, const tripoint & ) const
{
    if( t ) {
        return 0;
    }
    if( it.type->charges_to_use() != 0 && it.charges < it.type->charges_to_use() ) {
        p.add_msg_if_player( _( no_charges_message ) );
        return 0;
    }
    {
        p.moves -= moves;
        if( noise > 0 ) {
            sounds::sound( p.pos(), noise, sounds::sound_t::activity,
                           noise_message.empty() ? _( "Hsss" ) : _( noise_message ), true, noise_id, noise_variant );
        }
        p.add_msg_if_player( _( use_message ) );
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
    player_descriptions = obj.get_string_array( "player_descriptions" );
    npc_descriptions = obj.get_string_array( "npc_descriptions" );
}

int musical_instrument_actor::use( player &p, item &it, bool t, const tripoint & ) const
{
    if( p.is_mounted() ) {
        p.add_msg_player_or_npc( m_bad, _( "You can't play music while mounted." ),
                                 _( "<npcname> can't play music while mounted." ) );
        it.active = false;
        return 0;
    }
    if( p.is_underwater() ) {
        p.add_msg_player_or_npc( m_bad,
                                 _( "You can't play music underwater" ),
                                 _( "<npcname> can't play music underwater" ) );
        it.active = false;
        return 0;
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
        return 0;
    }

    if( !t && it.active ) {
        p.add_msg_player_or_npc( _( "You stop playing your %s" ),
                                 _( "<npcname> stops playing their %s" ),
                                 it.display_name() );
        it.active = false;
        return 0;
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
        return 0;
    }

    // At speed this low you can't coordinate your actions well enough to play the instrument
    if( p.get_speed() <= 25 + speed_penalty ) {
        p.add_msg_player_or_npc( m_bad,
                                 _( "You feel too weak to play your %s" ),
                                 _( "<npcname> feels too weak to play their %s" ),
                                 it.display_name() );
        it.active = false;
        return 0;
    }

    // We can play the music now
    if( !it.active ) {
        p.add_msg_player_or_npc( m_good,
                                 _( "You start playing your %s" ),
                                 _( "<npcname> starts playing their %s" ),
                                 it.display_name() );
        it.active = true;
    }

    if( p.get_effect_int( effect_playing_instrument ) <= speed_penalty ) {
        // Only re-apply the effect if it wouldn't lower the intensity
        p.add_effect( effect_playing_instrument, 2_turns, num_bp, false, speed_penalty );
    }

    std::string desc = "music";
    /** @EFFECT_PER increases morale bonus when playing an instrument */
    const int morale_effect = fun + fun_bonus * p.per_cur;
    if( morale_effect >= 0 && calendar::once_every( description_frequency ) ) {
        if( !player_descriptions.empty() && p.is_player() ) {
            desc = _( random_entry( player_descriptions ) );
        } else if( !npc_descriptions.empty() && p.is_npc() ) {
            desc = string_format( _( "%1$s %2$s" ), p.disp_name( false ),
                                  random_entry( npc_descriptions ) );
        }
    } else if( morale_effect < 0 && calendar::once_every( 1_minutes ) ) {
        // No musical skills = possible morale penalty
        if( p.is_player() ) {
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
        if( p.is_player() && desc != "music" ) {
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
    for( const std::string &sp : spells ) {
        dump.emplace_back( "SPELL", spell_id( sp ).obj().name.translated() );
    }
}

int learn_spell_actor::use( player &p, item &, bool, const tripoint & ) const
{
    if( p.fine_detail_vision_mod() > 4 ) {
        p.add_msg_if_player( _( "It's too dark to read." ) );
        return 0;
    }
    if( p.has_trait( trait_id( "ILLITERATE" ) ) ) {
        p.add_msg_if_player( _( "You can't read." ) );
        return 0;
    }
    std::vector<uilist_entry> uilist_initializer;
    uilist spellbook_uilist;
    spellbook_callback sp_cb;
    bool know_it_all = true;
    for( const std::string &sp_id_str : spells ) {
        const spell_id sp_id( sp_id_str );
        sp_cb.add_spell( sp_id );
        uilist_entry entry( sp_id.obj().name.translated() );
        if( p.magic.knows_spell( sp_id ) ) {
            const spell sp = p.magic.get_spell( sp_id );
            entry.ctxt = string_format( _( "Level %u" ), sp.get_level() );
            if( sp.is_max_level() ) {
                entry.ctxt += _( " (Max)" );
                entry.enabled = false;
            } else {
                know_it_all = false;
            }
        } else {
            if( p.magic.can_learn_spell( p, sp_id ) ) {
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
        return 0;
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
        return 0;
    }
    const bool knows_spell = p.magic.knows_spell( spells[action] );
    player_activity study_spell( ACT_STUDY_SPELL,
                                 p.magic.time_to_learn_spell( p, spells[action] ) );
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
            return 0;
        }
        study_spell.moves_total = study_time;
    }
    study_spell.moves_left = study_spell.moves_total;
    if( study_spell.moves_total == 10100 ) {
        study_spell.str_values[0] = "gain_level";
        study_spell.values[0] = 0; // reserved for xp
        study_spell.values[1] = p.magic.get_spell( spell_id( spells[action] ) ).get_level() + 1;
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
}

void cast_spell_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    //~ %1$s: spell name, %2$i: spell level
    const std::string message = string_format( _( "This item casts %1$s at level %2$i." ),
                                item_spell->name, spell_level );
    dump.emplace_back( "DESCRIPTION", message );
    if( no_fail ) {
        dump.emplace_back( "DESCRIPTION", _( "This item never fails." ) );
    }
}

int cast_spell_actor::use( player &p, item &it, bool, const tripoint & ) const
{
    if( need_worn && !p.is_worn( it ) ) {
        p.add_msg_if_player( m_info, _( "You need to wear the %1$s before activating it." ), it.tname() );
        return 0;
    }
    if( need_wielding && !p.is_wielding( it ) ) {
        p.add_msg_if_player( m_info, _( "You need to wield the %1$s before activating it." ), it.tname() );
        return 0;
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
    if( it.has_flag( "USE_PLAYER_ENERGY" ) ) {
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
    holster_prompt = obj.get_string( "holster_prompt", "" );
    holster_msg = obj.get_string( "holster_msg", "" );
}

bool holster_actor::can_holster( const item &holster, const item &obj ) const
{
    if( !holster.contents.can_contain( obj ).success() ) {
        return false;
    }
    if( obj.active ) {
        return false;
    }
    return holster.contents.can_contain( obj ).success();
}

bool holster_actor::store( player &p, item &holster, item &obj ) const
{
    if( obj.is_null() || holster.is_null() ) {
        debugmsg( "Null item was passed to holster_actor" );
        return false;
    }

    const ret_val<bool> contain = holster.contents.can_contain( obj );
    if( !contain.success() ) {
        p.add_msg_if_player( m_bad, contain.str(), holster.tname(), obj.tname() );
    }
    if( obj.active ) {
        p.add_msg_if_player( m_info, _( "You don't think putting your %1$s in your %2$s is a good idea" ),
                             obj.tname(), holster.tname() );
        return false;
    }
    p.add_msg_if_player( holster_msg.empty() ? _( "You holster your %s" ) : _( holster_msg ),
                         obj.tname(), holster.tname() );

    // holsters ignore penalty effects (e.g. GRABBED) when determining number of moves to consume
    p.store( holster, obj, false, holster.contents.obtain_cost( obj ),
             item_pocket::pocket_type::CONTAINER );
    return true;
}

int holster_actor::use( player &p, item &it, bool, const tripoint & ) const
{
    if( p.is_wielding( it ) ) {
        p.add_msg_if_player( _( "You need to unwield your %s before using it." ), it.tname() );
        return 0;
    }

    int pos = 0;
    std::vector<std::string> opts;

    std::string prompt = holster_prompt.empty() ? _( "Holster item" ) : _( holster_prompt );
    opts.push_back( prompt );
    pos = -1;
    std::list<item *> all_items = it.contents.all_items_top(
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
            if( opts.size() != it.contents.num_item_stacks() ) {
                ret--;
            }
            auto iter = std::next( all_items.begin(), ret );
            internal_item = *iter;
        }
    } else {
        internal_item = all_items.front();
    }

    if( pos < -1 ) {
        p.add_msg_if_player( _( "Never mind." ) );
        return 0;
    }

    if( pos >= 0 ) {
        // worn holsters ignore penalty effects (e.g. GRABBED) when determining number of moves to consume
        if( p.is_worn( it ) ) {
            p.wield_contents( it, internal_item, false, it.contents.obtain_cost( *internal_item ) );
        } else {
            p.wield_contents( it, internal_item );
        }

    } else {
        if( p.as_avatar() == nullptr ) {
            return 0;
        }
        // may not strictly be the correct item_location, but plumbing item_location through all iuse_actor::use won't work
        item_location item_loc( p, &it );
        game_menus::inv::insert_items( *p.as_avatar(), item_loc );
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

int ammobelt_actor::use( player &p, item &, bool, const tripoint & ) const
{
    item mag( belt );
    mag.ammo_unset();

    if( p.rate_action_reload( mag ) != hint_rating::good ) {
        p.add_msg_if_player( _( "Insufficient ammunition to assemble %s" ), mag.tname() );
        return 0;
    }

    item::reload_option opt = p.select_ammo( mag, true );
    if( opt ) {
        p.assign_activity( ACT_RELOAD, opt.moves(), opt.qty() );
        p.activity.targets.emplace_back( p, &p.i_add( mag ) );
        p.activity.targets.push_back( std::move( opt.ammo ) );
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
        obj.throw_error( "Invalid skill", "skill" );
    }

    cost_scaling = obj.get_float( "cost_scaling" );

    // Optional
    tool_quality = obj.get_int( "tool_quality", 0 );
    move_cost    = obj.get_int( "move_cost", 500 );
    trains_skill_to = obj.get_int( "trains_skill_to", 5 ) - 1;
}

bool repair_item_actor::can_use_tool( const player &p, const item &tool, bool print_msg ) const
{
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
    if( !tool.units_sufficient( p ) ) {
        if( print_msg ) {
            p.add_msg_if_player( m_info, _( "Your tool does not have enough charges to do that." ) );
        }
        return false;
    }

    return true;
}

static item_location get_item_location( player &p, item &it, const tripoint &pos )
{
    // Item on a character
    if( p.has_item( it ) ) {
        return item_location( p, &it );
    }

    // Item in a vehicle
    if( const optional_vpart_position &vp = g->m.veh_at( pos ) ) {
        vehicle_cursor vc( vp->vehicle(), vp->part_index() );
        bool found_in_vehicle = false;
        vc.visit_items( [&]( const item * e ) {
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
    return item_location( pos, &it );
}

int repair_item_actor::use( player &p, item &it, bool, const tripoint &position ) const
{
    if( !can_use_tool( p, it, true ) ) {
        return 0;
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

bool repair_item_actor::handle_components( player &pl, const item &fix,
        bool print_msg, bool just_check ) const
{
    // Entries valid for repaired items
    std::set<material_id> valid_entries;
    for( const auto &mat : materials ) {
        if( fix.made_of( mat ) ) {
            valid_entries.insert( mat );
        }
    }

    if( valid_entries.empty() ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _( "Your %s is not made of any of:" ),
                                  fix.tname() );
            for( const auto &mat_name : materials ) {
                const auto &mat = mat_name.obj();
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
                                            std::ceil( fix.volume() / 250_ml * cost_scaling ) :
                                            roll_remainder( fix.volume() / 250_ml * cost_scaling ) );

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
    for( const auto &entry : valid_entries ) {
        const itype_id &component_id = entry.obj().repaired_with();
        // Certain (different!) materials are repaired with the same components (steel, iron, hard steel use scrap metal).
        // This checks avoids adding the same component twice, which is annoying to the user.
        if( std::find_if( comps.begin(), comps.end(), [&]( const item_comp & ic ) {
        return ic.type == component_id;
    } ) != comps.end() ) {
            continue;
        }
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
            for( const auto &entry : valid_entries ) {
                const auto &mat_comp = entry.obj().repaired_with();
                pl.add_msg_if_player( m_info,
                                      _( "You don't have enough %s to do that.  Have: %d, need: %d" ),
                                      item::nname( mat_comp, 2 ),
                                      item::find_type( mat_comp )->count_by_charges() ?
                                      crafting_inv.amount_of( mat_comp, false ) :
                                      crafting_inv.charges_of( mat_comp, items_needed ),
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
static int find_repair_difficulty( const player &pl, const itype_id &id, bool training )
{
    // If the recipe is not found, this will remain unchanged
    int min = -1;
    for( const auto &e : recipe_dict ) {
        const auto r = e.second;
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
int repair_item_actor::repair_recipe_difficulty( const player &pl,
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

bool repair_item_actor::can_repair_target( player &pl, const item &fix,
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
    if( fix.count_by_charges() || fix.has_flag( "NO_REPAIR" ) ) {
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

    const bool can_be_refitted = fix.has_flag( "VARSIZE" );
    if( can_be_refitted && !fix.has_flag( "FIT" ) ) {
        return true;
    }

    const bool resizing_matters = fix.get_encumber( pl ) != 0;
    const bool small = pl.has_trait( trait_SMALL2 ) || pl.has_trait( trait_SMALL_OK );
    const bool can_resize = small != fix.has_flag( "UNDERSIZE" );
    if( can_be_refitted && resizing_matters && can_resize ) {
        return true;
    }

    if( fix.damage() > 0 ) {
        return true;
    }

    if( fix.damage() <= fix.min_damage() ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _( "Your %s is already enhanced to its maximum potential." ),
                                  fix.tname() );
        }
        return false;
    }

    if( fix.has_flag( "PRIMITIVE_RANGED_WEAPON" ) || !fix.reinforceable() ) {
        if( print_msg ) {
            pl.add_msg_if_player( m_info, _( "You cannot improve your %s any more this way." ),
                                  fix.tname() );
        }
        return false;
    }

    return true;
}

std::pair<float, float> repair_item_actor::repair_chance(
    const player &pl, const item &fix, repair_item_actor::repair_type action_type ) const
{
    /** @EFFECT_TAILOR randomly improves clothing repair efforts */
    /** @EFFECT_MECHANICS randomly improves metal repair efforts */
    const int skill = pl.get_skill_level( used_skill );
    const int recipe_difficulty = repair_recipe_difficulty( pl, fix );
    int action_difficulty = 0;
    switch( action_type ) {
        case RT_REPAIR:
            action_difficulty = fix.damage_level( 4 );
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
    if( fix.damage() > 0 ) {
        return RT_REPAIR;
    }

    const bool can_be_refitted = fix.has_flag( flag_VARSIZE );
    const bool doesnt_fit = !fix.has_flag( flag_FIT );
    if( doesnt_fit && can_be_refitted ) {
        return RT_REFIT;
    }

    const bool smol = g->u.has_trait( trait_SMALL2 ) ||
                      g->u.has_trait( trait_SMALL_OK );

    const bool is_undersized = fix.has_flag( flag_UNDERSIZE );
    const bool is_oversized = fix.has_flag( flag_OVERSIZE );
    const bool resizing_matters = fix.get_encumber( g->u ) != 0;

    const bool too_big_while_smol = smol && !is_undersized && !is_oversized;
    if( too_big_while_smol && can_be_refitted && resizing_matters ) {
        return RT_DOWNSIZING;
    }

    const bool too_small_while_big = !smol && is_undersized && !is_oversized;
    if( too_small_while_big && can_be_refitted && resizing_matters ) {
        return RT_UPSIZING;
    }

    if( fix.damage() > fix.min_damage() ) {
        return RT_REINFORCE;
    }

    if( current_skill_level <= trains_skill_to ) {
        return RT_PRACTICE;
    }

    return RT_NOTHING;
}

static bool damage_item( player &pl, item_location &fix )
{
    const std::string startdurability = fix->durability_indicator( true );
    const auto destroyed = fix->inc_damage();
    const std::string resultdurability = fix->durability_indicator( true );
    pl.add_msg_if_player( m_bad, _( "You damage your %s!  ( %s-> %s)" ), fix->tname( 1, false ),
                          startdurability, resultdurability );
    if( destroyed ) {
        pl.add_msg_if_player( m_bad, _( "You destroy it!" ) );
        if( fix.where() == item_location::type::character ) {
            pl.i_rem_keep_contents( pl.get_item_position( fix.get_item() ) );
        } else {
            for( const item *it : fix->contents.all_items_top() ) {
                put_into_vehicle_or_drop( pl, item_drop_reason::deliberate, { *it }, fix.position() );
            }
            fix.remove_item();
        }

        return true;
    }

    return false;
}

repair_item_actor::attempt_hint repair_item_actor::repair( player &pl, item &tool,
        item_location &fix ) const
{
    if( !can_use_tool( pl, tool, true ) ) {
        return AS_CANT_USE_TOOL;
    }
    if( !can_repair_target( pl, *fix, true ) ) {
        return AS_CANT;
    }

    const int current_skill_level = pl.get_skill_level( used_skill );
    const auto action = default_action( *fix, current_skill_level );
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
        return damage_item( pl, fix ) ? AS_DESTROYED : AS_FAILURE;
    }

    if( action == RT_PRACTICE ) {
        return AS_RETRY;
    }

    if( action == RT_REPAIR ) {
        if( roll == SUCCESS ) {
            const std::string startdurability = fix->durability_indicator( true );
            const auto damage = fix->damage();
            handle_components( pl, *fix, false, false );
            fix->set_damage( std::max( damage - itype::damage_scale, 0 ) );
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
            if( !fix->has_flag( "FIT" ) ) {
                pl.add_msg_if_player( m_good, _( "You take your %s in, improving the fit." ),
                                      fix->tname() );
                fix->item_tags.insert( "FIT" );
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
            fix->item_tags.insert( "UNDERSIZE" );
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
            fix->item_tags.erase( "UNDERSIZE" );
            handle_components( pl, *fix, false, false );
            return AS_SUCCESS;
        }
        return AS_RETRY;
    }

    if( action == RT_REINFORCE ) {
        if( fix->has_flag( "PRIMITIVE_RANGED_WEAPON" ) || !fix->reinforceable() ) {
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
        return _( mid->name() );
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

    bleed = obj.get_float( "bleed", 0.0f );
    bite = obj.get_float( "bite", 0.0f );
    infect = obj.get_float( "infect", 0.0f );

    long_action = obj.get_bool( "long_action", false );

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
        used_up_item_flags = u.get_tags( "flags" );
    }
}

static player &get_patient( player &healer, const tripoint &pos )
{
    if( healer.pos() == pos ) {
        return healer;
    }

    player *const person = g->critter_at<player>( pos );
    if( !person ) {
        // Default to heal self on failure not to break old functionality
        add_msg( m_debug, "No heal target at position %d,%d,%d", pos.x, pos.y, pos.z );
        return healer;
    }

    return *person;
}

int heal_actor::use( player &p, item &it, bool, const tripoint &pos ) const
{
    if( p.is_underwater() ) {
        p.add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    if( p.is_mounted() ) {
        p.add_msg_if_player( m_info, _( "You can't do that while mounted." ) );
        return 0;
    }
    if( it.is_filthy() ) {
        p.add_msg_if_player( m_info, _( "You can't use filthy items for healing." ) );
        return 0;
    }

    player &patient = get_patient( p, pos );
    const hp_part hpp = use_healing_item( p, patient, it, false );
    if( hpp == num_hp_parts ) {
        return 0;
    }

    int cost = move_cost;
    if( long_action ) {
        // A hack: long action healing on NPCs isn't done yet.
        // So just heal at start and paralyze the player for 5 minutes.
        cost /= std::min( 10, p.get_skill_level( skill_firstaid ) + 1 );
    }

    // NPCs can use first aid now, but they can't perform long actions
    if( long_action && &patient == &p && !p.is_npc() ) {
        // Assign first aid long action.
        /** @EFFECT_FIRSTAID speeds up firstaid activity */
        p.assign_activity( ACT_FIRSTAID, cost, 0, 0, it.tname() );
        p.activity.targets.push_back( item_location( p, &it ) );
        p.activity.values.push_back( hpp );
        p.moves = 0;
        return 0;
    }

    p.moves -= cost;
    p.add_msg_if_player( m_good, _( "You use your %s." ), it.tname() );
    return it.type->charges_to_use();
}

std::unique_ptr<iuse_actor> heal_actor::clone() const
{
    return std::make_unique<heal_actor>( *this );
}

int heal_actor::get_heal_value( const player &healer, hp_part healed ) const
{
    int heal_base;
    float bonus_mult;
    if( healed == hp_head ) {
        heal_base = head_power;
        bonus_mult = head_scaling;
    } else if( healed == hp_torso ) {
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

int heal_actor::get_bandaged_level( const player &healer ) const
{
    if( bandages_power > 0 ) {
        /** @EFFECT_FIRSTAID increases healing item effects */
        return bandages_power + bandages_scaling * healer.get_skill_level( skill_firstaid );
    }

    return bandages_power;
}

int heal_actor::get_disinfected_level( const player &healer ) const
{
    if( disinfectant_power > 0 ) {
        /** @EFFECT_FIRSTAID increases healing item effects */
        return disinfectant_power + disinfectant_scaling * healer.get_skill_level( skill_firstaid );
    }

    return disinfectant_power;
}

int heal_actor::finish_using( player &healer, player &patient, item &it, hp_part healed ) const
{
    float practice_amount = limb_power * 3.0f;
    const int dam = get_heal_value( healer, healed );

    if( ( patient.hp_cur[healed] >= 1 ) && ( dam > 0 ) ) { // Prevent first-aid from mending limbs
        patient.heal( healed, dam );
    } else if( ( patient.hp_cur[healed] >= 1 ) && ( dam < 0 ) ) {
        const bodypart_id bp = convert_bp( player::hp_to_bp( healed ) ).id();
        patient.apply_damage( nullptr, bp, -dam ); //hurt takes + damage
    }

    const body_part bp_healed = player::hp_to_bp( healed );

    const bool u_see = healer.is_player() || patient.is_player() ||
                       g->u.sees( healer ) || g->u.sees( patient );
    const bool player_healing_player = healer.is_player() && patient.is_player();
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

    if( patient.has_effect( effect_bleed, bp_healed ) ) {
        if( x_in_y( bleed, 1.0f ) ) {
            patient.remove_effect( effect_bleed, bp_healed );
            heal_msg( m_good, _( "You stop the bleeding." ), _( "The bleeding is stopped." ) );
        } else {
            heal_msg( m_warning, _( "You fail to stop the bleeding." ), _( "The wound still bleeds." ) );
        }

        practice_amount += bleed * 3.0f;
    }
    if( patient.has_effect( effect_bite, bp_healed ) ) {
        if( x_in_y( bite, 1.0f ) ) {
            patient.remove_effect( effect_bite, bp_healed );
            heal_msg( m_good, _( "You clean the wound." ), _( "The wound is cleaned." ) );
        } else {
            heal_msg( m_warning, _( "Your wound still aches." ), _( "The wound still looks bad." ) );
        }

        practice_amount += bite * 3.0f;
    }
    if( patient.has_effect( effect_infected, bp_healed ) ) {
        if( x_in_y( infect, 1.0f ) ) {
            const time_duration infected_dur = patient.get_effect_dur( effect_infected, bp_healed );
            patient.remove_effect( effect_infected, bp_healed );
            patient.add_effect( effect_recover, infected_dur );
            heal_msg( m_good, _( "You disinfect the wound." ), _( "The wound is disinfected." ) );
        } else {
            heal_msg( m_warning, _( "Your wound still hurts." ), _( "The wound still looks nasty." ) );
        }

        practice_amount += infect * 10.0f;
    }

    if( long_action ) {
        healer.add_msg_if_player( _( "You finish using the %s." ), it.tname() );
    }

    for( const auto &eff : effects ) {
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
        patient.add_effect( effect_bandaged, 1_turns, bp_healed );
        effect &e = patient.get_effect( effect_bandaged, bp_healed );
        e.set_duration( e.get_int_dur_factor() * bandages_intensity );
        patient.damage_bandaged[healed] = patient.hp_max[healed] - patient.hp_cur[healed];
        practice_amount += 2 * bandages_intensity;
    }
    if( disinfectant_power > 0 ) {
        int disinfectant_intensity = get_disinfected_level( healer );
        patient.add_effect( effect_disinfected, 1_turns, bp_healed );
        effect &e = patient.get_effect( effect_disinfected, bp_healed );
        e.set_duration( e.get_int_dur_factor() * disinfectant_intensity );
        patient.damage_disinfected[healed] = patient.hp_max[healed] - patient.hp_cur[healed];
        practice_amount += 2 * disinfectant_intensity;
    }
    practice_amount = std::max( 9.0f, practice_amount );

    healer.practice( skill_firstaid, static_cast<int>( practice_amount ) );
    return it.type->charges_to_use();
}

static hp_part pick_part_to_heal(
    const player &healer, const player &patient,
    const std::string &menu_header,
    int limb_power, int head_bonus, int torso_bonus,
    float bleed_chance, float bite_chance, float infect_chance,
    bool force, float bandage_power, float disinfectant_power )
{
    const bool bleed = bleed_chance > 0.0f;
    const bool bite = bite_chance > 0.0f;
    const bool infect = infect_chance > 0.0f;
    const bool precise = &healer == &patient ?
                         patient.has_trait( trait_SELFAWARE ) :
                         /** @EFFECT_PER slightly increases precision when using first aid on someone else */

                         /** @EFFECT_FIRSTAID increases precision when using first aid on someone else */
                         ( healer.get_skill_level( skill_firstaid ) * 4 + healer.per_cur >= 20 );
    while( true ) {
        hp_part healed_part = patient.body_window( menu_header, force, precise,
                              limb_power, head_bonus, torso_bonus,
                              bleed_chance, bite_chance, infect_chance, bandage_power, disinfectant_power );
        if( healed_part == num_hp_parts ) {
            return num_hp_parts;
        }

        body_part bp = player::hp_to_bp( healed_part );
        if( ( infect && patient.has_effect( effect_infected, bp ) ) ||
            ( bite && patient.has_effect( effect_bite, bp ) ) ||
            ( bleed && patient.has_effect( effect_bleed, bp ) ) ) {
            return healed_part;
        }

        if( patient.is_limb_broken( healed_part ) ) {
            if( healed_part == hp_arm_l || healed_part == hp_arm_r ) {
                add_msg( m_info, _( "That arm is broken.  It needs surgical attention or a splint." ) );
            } else if( healed_part == hp_leg_l || healed_part == hp_leg_r ) {
                add_msg( m_info, _( "That leg is broken.  It needs surgical attention or a splint." ) );
            } else {
                add_msg( m_info, "That body part is bugged.  It needs developer's attention." );
            }

            continue;
        }

        if( force || patient.hp_cur[healed_part] < patient.hp_max[healed_part] ) {
            return healed_part;
        }
    }
}

hp_part heal_actor::use_healing_item( player &healer, player &patient, item &it, bool force ) const
{
    hp_part healed = num_hp_parts;
    const int head_bonus = get_heal_value( healer, hp_head );
    const int limb_power = get_heal_value( healer, hp_arm_l );
    const int torso_bonus = get_heal_value( healer, hp_torso );

    if( !patient.can_use_heal_item( it ) ) {
        patient.add_msg_player_or_npc( m_bad,
                                       _( "Your biology is not compatible with that item." ),
                                       _( "<npcname>'s biology is not compatible with that item." ) );
        return num_hp_parts; // canceled
    }

    if( healer.is_npc() ) {
        // NPCs heal whatever has sustained the most damaged that they can heal but never
        // rebandage parts
        int highest_damage = 0;
        for( int i = 0; i < num_hp_parts; i++ ) {
            int damage = 0;
            const body_part i_bp = player::hp_to_bp( static_cast<hp_part>( i ) );
            if( ( !patient.has_effect( effect_bandaged, i_bp ) && bandages_power > 0 ) ||
                ( !patient.has_effect( effect_disinfected, i_bp ) && disinfectant_power > 0 ) ) {
                damage += patient.hp_max[i] - patient.hp_cur[i];
                damage += bleed * patient.get_effect_dur( effect_bleed, i_bp ) / 5_minutes;
                damage += bite * patient.get_effect_dur( effect_bite, i_bp ) / 10_minutes;
                damage += infect * patient.get_effect_dur( effect_infected, i_bp ) / 10_minutes;
            }
            if( damage > highest_damage ) {
                highest_damage = damage;
                healed = static_cast<hp_part>( i );
            }
        }
    } else if( patient.is_player() ) {
        // Player healing self - let player select
        if( healer.activity.id() != ACT_FIRSTAID ) {
            const std::string menu_header = _( "Select a body part for: " ) + it.tname();
            healed = pick_part_to_heal( healer, patient, menu_header,
                                        limb_power, head_bonus, torso_bonus,
                                        bleed, bite, infect, force,
                                        get_bandaged_level( healer ),
                                        get_disinfected_level( healer ) );
            if( healed == num_hp_parts ) {
                add_msg( m_info, _( "Never mind." ) );
                return num_hp_parts; // canceled
            }
        }
        // Brick healing if using a first aid kit for the first time.
        if( long_action && healer.activity.id() != ACT_FIRSTAID ) {
            // Cancel and wait for activity completion.
            return healed;
        } else if( healer.activity.id() == ACT_FIRSTAID ) {
            // Completed activity, extract body part from it.
            healed = static_cast<hp_part>( healer.activity.values[0] );
        }
    } else {
        // Player healing NPC
        // TODO: Remove this hack, allow using activities on NPCs
        const std::string menu_header = string_format( pgettext( "healing",
                                        //~ %1$s: patient name, %2$s: healing item name
                                        "Select a body part of %1$s for %2$s:" ),
                                        patient.disp_name(), it.tname() );
        healed = pick_part_to_heal( healer, patient, menu_header,
                                    limb_power, head_bonus, torso_bonus,
                                    bleed, bite, infect, force,
                                    get_bandaged_level( healer ),
                                    get_disinfected_level( healer ) );
    }

    if( healed != num_hp_parts ) {
        finish_using( healer, patient, it, healed );
    }

    return healed;
}

void heal_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    if( head_power > 0 || torso_power > 0 || limb_power > 0 || bandages_power > 0 ||
        disinfectant_power > 0 || bleed > 0.0f || bite > 0.0f || infect > 0.0f ) {
        dump.emplace_back( "HEAL", _( "<bold>Healing effects</bold> " ) );
    }

    if( head_power > 0 || torso_power > 0 || limb_power > 0 ) {
        dump.emplace_back( "HEAL", _( "Base healing: " ) );
        dump.emplace_back( "HEAL_BASE", _( "Head: " ), "", iteminfo::no_newline, head_power );
        dump.emplace_back( "HEAL_BASE", _( "  Torso: " ), "", iteminfo::no_newline, torso_power );
        dump.emplace_back( "HEAL_BASE", _( "  Limbs: " ), limb_power );
        if( g != nullptr ) {
            dump.emplace_back( "HEAL", _( "Actual healing: " ) );
            dump.emplace_back( "HEAL_ACT", _( "Head: " ), "", iteminfo::no_newline,
                               get_heal_value( g->u, hp_head ) );
            dump.emplace_back( "HEAL_ACT", _( "  Torso: " ), "", iteminfo::no_newline,
                               get_heal_value( g->u, hp_torso ) );
            dump.emplace_back( "HEAL_ACT", _( "  Limbs: " ), get_heal_value( g->u, hp_arm_l ) );
        }
    }

    if( bandages_power > 0 ) {
        dump.emplace_back( "HEAL", _( "Base bandaging quality: " ),
                           texitify_base_healing_power( static_cast<int>( bandages_power ) ) );
        if( g != nullptr ) {
            dump.emplace_back( "HEAL", _( "Actual bandaging quality: " ),
                               texitify_healing_power( get_bandaged_level( g->u ) ) );
        }
    }

    if( disinfectant_power > 0 ) {
        dump.emplace_back( "HEAL", _( "Base disinfecting quality: " ),
                           texitify_base_healing_power( static_cast<int>( disinfectant_power ) ) );
        if( g != nullptr ) {
            dump.emplace_back( "HEAL", _( "Actual disinfecting quality: " ),
                               texitify_healing_power( get_disinfected_level( g->u ) ) );
        }
    }

    if( bleed > 0.0f || bite > 0.0f || infect > 0.0f ) {
        dump.emplace_back( "HEAL", _( "Chance to heal (percent): " ) );
        if( bleed > 0.0f ) {
            dump.emplace_back( "HEAL", _( "* Bleeding: " ),
                               static_cast<int>( bleed * 100 ) );
        }
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

static bool is_solid_neighbor( const tripoint &pos, const int offset_x, const int offset_y )
{
    const tripoint a = pos + tripoint( offset_x, offset_y, 0 );
    const tripoint b = pos - tripoint( offset_x, offset_y, 0 );
    return g->m.move_cost( a ) != 2 && g->m.move_cost( b ) != 2;
}

static bool has_neighbor( const tripoint &pos, const ter_id &terrain_id )
{
    for( const tripoint &t : g->m.points_in_radius( pos, 1, 0 ) ) {
        if( g->m.ter( t ) == terrain_id ) {
            return true;
        }
    }
    return false;
}

bool place_trap_actor::is_allowed( player &p, const tripoint &pos, const std::string &name ) const
{
    if( !allow_under_player && pos == p.pos() ) {
        p.add_msg_if_player( m_info, _( "Yeah.  Place the %s at your feet.  Real damn smart move." ),
                             name );
        return false;
    }
    if( g->m.move_cost( pos ) != 2 ) {
        p.add_msg_if_player( m_info, _( "You can't place a %s there." ), name );
        return false;
    }
    if( needs_solid_neighbor ) {
        if( !is_solid_neighbor( pos, 1, 0 ) && !is_solid_neighbor( pos, 0, 1 ) &&
            !is_solid_neighbor( pos, 1, 1 ) && !is_solid_neighbor( pos, 1, -1 ) ) {
            p.add_msg_if_player( m_info, _( "You must place the %s between two solid tiles." ), name );
            return false;
        }
    }
    if( needs_neighbor_terrain && !has_neighbor( pos, needs_neighbor_terrain ) ) {
        p.add_msg_if_player( m_info, _( "The %s needs a %s adjacent to it." ), name,
                             needs_neighbor_terrain.obj().name() );
        return false;
    }
    const trap &existing_trap = g->m.tr_at( pos );
    if( !existing_trap.is_null() ) {
        if( existing_trap.can_see( pos, p ) ) {
            p.add_msg_if_player( m_info, _( "You can't place a %s there.  It contains a trap already." ),
                                 name );
        } else {
            p.add_msg_if_player( m_bad, _( "You trigger a %s!" ), existing_trap.name() );
            existing_trap.trigger( pos, &p );
        }
        return false;
    }
    return true;
}

static void place_and_add_as_known( player &p, const tripoint &pos, const trap_str_id &id )
{
    g->m.trap_set( pos, id );
    const trap &tr = g->m.tr_at( pos );
    if( !tr.can_see( pos, p ) ) {
        p.add_known_trap( pos, tr );
    }
}

int place_trap_actor::use( player &p, item &it, bool, const tripoint & ) const
{
    const bool could_bury = !bury_question.empty();
    if( !allow_underwater && p.is_underwater() ) {
        p.add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    if( p.is_mounted() ) {
        p.add_msg_if_player( m_info, _( "You can't do that while mounted." ) );
        return 0;
    }
    const cata::optional<tripoint> pos_ = choose_adjacent( string_format( _( "Place %s where?" ),
                                          it.tname() ) );
    if( !pos_ ) {
        return 0;
    }
    tripoint pos = *pos_;

    if( !is_allowed( p, pos, it.tname() ) ) {
        return 0;
    }

    int distance_to_trap_center = unburied_data.trap.obj().get_trap_radius() +
                                  outer_layer_trap.obj().get_trap_radius() + 1;
    if( unburied_data.trap.obj().get_trap_radius() > 0 ) {
        // Math correction for multi-tile traps
        pos.x = ( pos.x - p.posx() ) * distance_to_trap_center + p.posx();
        pos.y = ( pos.y - p.posy() ) * distance_to_trap_center + p.posy();
        for( const tripoint &t : g->m.points_in_radius( pos, outer_layer_trap.obj().get_trap_radius(),
                0 ) ) {
            if( !is_allowed( p, t, it.tname() ) ) {
                p.add_msg_if_player( m_info,
                                     _( "That trap needs a space in %d tiles radius to be clear, centered %d tiles from you." ),
                                     outer_layer_trap.obj().get_trap_radius(), distance_to_trap_center );
                return 0;
            }
        }
    }

    const bool has_shovel = p.has_quality( quality_id( "DIG" ), 3 );
    const bool is_diggable = g->m.has_flag( "DIGGABLE", pos );
    bool bury = false;
    if( could_bury && has_shovel && is_diggable ) {
        bury = query_yn( _( bury_question ) );
    }
    const auto &data = bury ? buried_data : unburied_data;

    p.add_msg_if_player( m_info, _( data.done_message ), distance_to_trap_center );
    p.practice( skill_id( "traps" ), data.practice );
    p.mod_moves( -data.moves );

    place_and_add_as_known( p, pos, data.trap );
    for( const tripoint &t : g->m.points_in_radius( pos, data.trap.obj().get_trap_radius(), 0 ) ) {
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

int emit_actor::use( player &, item &it, bool, const tripoint &pos ) const
{
    const float scaling = scale_qty ? it.charges : 1;
    for( const auto &e : emits ) {
        g->m.emit_field( pos, e, scaling );
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

int saw_barrel_actor::use( player &p, item &it, bool t, const tripoint & ) const
{
    if( t ) {
        return 0;
    }

    auto loc = game_menus::inv::saw_barrel( p, it );

    if( !loc ) {
        p.add_msg_if_player( _( "Never mind." ) );
        return 0;
    }

    item &obj = *loc.obtain( p );
    p.add_msg_if_player( _( "You saw down the barrel of your %s." ), obj.tname() );
    obj.put_in( item( "barrel_small", calendar::turn ), item_pocket::pocket_type::MOD );

    return 0;
}

ret_val<bool> saw_barrel_actor::can_use_on( const player &, const item &, const item &target ) const
{
    if( !target.is_gun() ) {
        return ret_val<bool>::make_failure( _( "It's not a gun." ) );
    }

    if( target.type->gun->barrel_length <= 0_ml ) {
        return ret_val<bool>::make_failure( _( "The barrel is too short." ) );
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

int install_bionic_actor::use( player &p, item &it, bool, const tripoint & ) const
{
    if( p.can_install_bionics( *it.type, p, false ) ) {
        p.consume_installation_requirment( it.type->bionic->id );
        p.consume_anesth_requirment( *it.type, p );
        return p.install_bionics( *it.type, p, false ) ? it.type->charges_to_use() : 0;
    } else {
        return 0;
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
        } else  if( it.has_flag( "FILTHY" ) ) {
            return ret_val<bool>::make_failure( _( "You can't install a filthy CBM!" ) );
        } else if( it.has_flag( "NO_STERILE" ) ) {
            return ret_val<bool>::make_failure( _( "This CBM is not sterile, you can't install it." ) );
        } else if( it.has_fault( fault_bionic_salvaged ) ) {
            return ret_val<bool>::make_failure(
                       _( "This CBM is already deployed.  You need to reset it to factory state." ) );
        } else if( units::energy_max - p.get_max_power_level() < bid->capacity ) {
            return ret_val<bool>::make_failure( _( "Max power capacity already reached" ) );
        }
    }

    if( p.has_bionic( bid ) ) {
        return ret_val<bool>::make_failure( _( "You have already installed this bionic." ) );
    } else if( bid->upgraded_bionic && !p.has_bionic( bid->upgraded_bionic ) ) {
        return ret_val<bool>::make_failure( _( "There is nothing to upgrade." ) );
    } else {
        const bool downgrade = std::any_of( bid->available_upgrades.begin(), bid->available_upgrades.end(),
                                            std::bind( &player::has_bionic, &p, std::placeholders::_1 ) );

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

int detach_gunmods_actor::use( player &p, item &it, bool, const tripoint & ) const
{
    auto mods = it.gunmods();

    mods.erase( std::remove_if( mods.begin(), mods.end(), std::bind( &item::is_irremovable,
                                std::placeholders::_1 ) ), mods.end() );

    uilist prompt;
    prompt.text = _( "Remove which modification?" );

    for( size_t i = 0; i != mods.size(); ++i ) {
        prompt.addentry( i, true, -1, mods[ i ]->tname() );
    }

    prompt.query();

    if( prompt.ret >= 0 ) {
        item *gm = mods[ prompt.ret ];
        p.gunmod_remove( it, *gm );
    } else {
        p.add_msg_if_player( _( "Never mind." ) );
    }

    return 0;
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

std::unique_ptr<iuse_actor> mutagen_actor::clone() const
{
    return std::make_unique<mutagen_actor>( *this );
}

void mutagen_actor::load( const JsonObject &obj )
{
    mutation_category = obj.get_string( "mutation_category", "ANY" );
    is_weak = obj.get_bool( "is_weak", false );
    is_strong = obj.get_bool( "is_strong", false );
}

int mutagen_actor::use( player &p, item &it, bool, const tripoint & ) const
{
    mutagen_attempt checks =
        mutagen_common_checks( p, it, false, mutagen_technique::consumed_mutagen );

    if( !checks.allowed ) {
        return checks.charges_used;
    }

    if( is_weak && !one_in( 3 ) ) {
        // Nothing! Mutagenic flesh often just fails to work.
        return it.type->charges_to_use();
    }

    const mutation_category_trait &m_category = mutation_category_trait::get_category(
                mutation_category );

    if( p.has_trait( trait_MUT_JUNKIE ) ) {
        p.add_msg_if_player( m_good, _( "You quiver with anticipation…" ) );
        p.add_morale( MORALE_MUTAGEN, 5, 50 );
    }

    p.add_msg_if_player( m_category.mutagen_message() );

    if( one_in( 6 ) ) {
        p.add_msg_player_or_npc( m_bad,
                                 _( "You suddenly feel dizzy, and collapse to the ground." ),
                                 _( "<npcname> suddenly collapses to the ground!" ) );
        p.add_effect( effect_downed, 1_turns, num_bp, false, 0, true );
    }

    int mut_count = 1 + ( is_strong ? one_in( 3 ) : 0 );

    for( int i = 0; i < mut_count; i++ ) {
        p.mutate_category( m_category.id );
        p.mod_pain( m_category.mutagen_pain * rng( 1, 5 ) );
    }
    // burn calories directly
    p.mod_stored_nutr( m_category.mutagen_hunger * mut_count );
    p.mod_thirst( m_category.mutagen_thirst * mut_count );
    p.mod_fatigue( m_category.mutagen_fatigue * mut_count );

    return it.type->charges_to_use();
}

std::unique_ptr<iuse_actor> mutagen_iv_actor::clone() const
{
    return std::make_unique<mutagen_iv_actor>( *this );
}

void mutagen_iv_actor::load( const JsonObject &obj )
{
    mutation_category = obj.get_string( "mutation_category", "ANY" );
}

int mutagen_iv_actor::use( player &p, item &it, bool, const tripoint & ) const
{
    mutagen_attempt checks =
        mutagen_common_checks( p, it, false, mutagen_technique::injected_mutagen );

    if( !checks.allowed ) {
        return checks.charges_used;
    }

    const mutation_category_trait &m_category = mutation_category_trait::get_category(
                mutation_category );

    if( p.has_trait( trait_MUT_JUNKIE ) ) {
        p.add_msg_if_player( m_category.junkie_message() );
    } else {
        p.add_msg_if_player( m_category.iv_message() );
    }

    // try to cross the threshold to be able to get post-threshold mutations this iv.
    test_crossing_threshold( p, m_category );

    // TODO: Remove the "is_player" part, implement NPC screams
    if( p.is_player() && !( p.has_trait( trait_NOPAIN ) ) && m_category.iv_sound ) {
        p.mod_pain( m_category.iv_pain );
        /** @EFFECT_STR increases volume of painful shouting when using IV mutagen */
        sounds::sound( p.pos(), m_category.iv_noise + p.str_cur, sounds::sound_t::alert,
                       m_category.iv_sound_message(), true, m_category.iv_sound_id(), m_category.iv_sound_variant() );
    }

    int mut_count = m_category.iv_min_mutations;
    for( int i = 0; i < m_category.iv_additional_mutations; ++i ) {
        if( !one_in( m_category.iv_additional_mutations_chance ) ) {
            ++mut_count;
        }
    }

    for( int i = 0; i < mut_count; i++ ) {
        p.mutate_category( m_category.id );
        p.mod_pain( m_category.iv_pain  * rng( 1, 5 ) );
    }

    p.mod_hunger( m_category.iv_hunger * mut_count );
    p.mod_thirst( m_category.iv_thirst * mut_count );
    p.mod_fatigue( m_category.iv_fatigue * mut_count );

    if( m_category.id == "CHIMERA" ) {
        p.add_morale( MORALE_MUTAGEN_CHIMERA, m_category.iv_morale, m_category.iv_morale_max );
    } else if( m_category.id == "ELFA" ) {
        p.add_morale( MORALE_MUTAGEN_ELF, m_category.iv_morale, m_category.iv_morale_max );
    } else if( m_category.iv_morale > 0 ) {
        p.add_morale( MORALE_MUTAGEN_MUTATION, m_category.iv_morale, m_category.iv_morale_max );
    }

    if( m_category.iv_sleep && !one_in( 3 ) ) {
        p.add_msg_if_player( m_bad, m_category.iv_sleep_message() );
        /** @EFFECT_INT reduces sleep duration when using IV mutagen */
        p.fall_asleep( time_duration::from_turns( m_category.iv_sleep_dur - p.int_cur * 5 ) );
    }

    // try crossing again after getting new in-category mutations.
    test_crossing_threshold( p, m_category );

    return it.type->charges_to_use();
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

int deploy_tent_actor::use( player &p, item &it, bool, const tripoint & ) const
{
    int diam = 2 * radius + 1;
    if( p.is_mounted() ) {
        p.add_msg_if_player( _( "You cannot do that while mounted." ) );
        return 0;
    }
    const cata::optional<tripoint> dir = choose_direction( string_format(
            _( "Put up the %s where (%dx%d clear area)?" ), it.tname(), diam, diam ) );
    if( !dir ) {
        return 0;
    }
    const tripoint direction = *dir;

    // We place the center of the structure (radius + 1)
    // spaces away from the player.
    // First check there's enough room.
    const tripoint center = p.pos() + tripoint( ( radius + 1 ) * direction.x,
                            ( radius + 1 ) * direction.y, 0 );
    for( const tripoint &dest : g->m.points_in_radius( center, radius ) ) {
        if( const auto vp = g->m.veh_at( dest ) ) {
            add_msg( m_info, _( "The %s is in the way." ), vp->vehicle().name );
            return 0;
        }
        if( const Creature *const c = g->critter_at( dest ) ) {
            add_msg( m_info, _( "The %s is in the way." ), c->disp_name() );
            return 0;
        }
        if( g->m.impassable( dest ) || !g->m.has_flag( "FLAT", dest ) ) {
            add_msg( m_info, _( "The %s in that direction isn't suitable for placing the %s." ),
                     g->m.name( dest ), it.tname() );
            return 0;
        }
        if( g->m.has_furn( dest ) ) {
            add_msg( m_info, _( "There is already furniture (%s) there." ), g->m.furnname( dest ) );
            return 0;
        }
    }
    // Make a square of floor surrounded by wall.
    for( const tripoint &dest : g->m.points_in_radius( center, radius ) ) {
        g->m.furn_set( dest, wall );
    }
    for( const tripoint &dest : g->m.points_in_radius( center, radius - 1 ) ) {
        g->m.furn_set( dest, floor );
    }
    // Place the center floor and the door.
    if( floor_center ) {
        g->m.furn_set( center, *floor_center );
    }
    g->m.furn_set( p.pos() + direction, door_closed );
    add_msg( m_info, _( "You set up the %s on the ground." ), it.tname() );
    add_msg( m_info, _( "Examine the center square to pack it up again." ) );
    return 1;
}

bool deploy_tent_actor::check_intact( const tripoint &center ) const
{
    for( const tripoint &dest : g->m.points_in_radius( center, radius ) ) {
        const furn_id fid = g->m.furn( dest );
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

int weigh_self_actor::use( player &p, item &, bool, const tripoint & ) const
{
    if( p.is_mounted() ) {
        p.add_msg_if_player( m_info, _( "You cannot weigh yourself while mounted." ) );
        return 0;
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
        clothing_mods.push_back( clothing_mod_id( line ) );
    }

    // TODO: Make skill non-mandatory while still erroring on invalid skill
    const std::string skill_string = obj.get_string( "skill" );
    used_skill = skill_id( skill_string );
    if( !used_skill.is_valid() ) {
        obj.throw_error( "Invalid skill", "skill" );
    }
}

int sew_advanced_actor::use( player &p, item &it, bool, const tripoint & ) const
{
    if( p.is_npc() ) {
        return 0;
    }
    if( p.is_mounted() ) {
        p.add_msg_if_player( m_info, _( "You cannot do that while mounted." ) );
        return 0;
    }
    if( p.is_underwater() ) {
        p.add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }

    if( p.fine_detail_vision_mod() > 4 ) {
        add_msg( m_info, _( "You can't see to sew!" ) );
        return 0;
    }

    auto filter = [this]( const item & itm ) {
        return itm.is_armor() && !itm.is_firearm() && !itm.is_power_armor() &&
               itm.made_of_any( materials );
    };
    // note: if !p.is_npc() then p is avatar.
    item_location loc = game_menus::inv::titled_filter_menu(
                            filter, *p.as_avatar(), _( "Enhance which clothing?" ) );
    if( !loc ) {
        p.add_msg_if_player( m_info, _( "You do not have that item!" ) );
        return 0;
    }
    item &mod = *loc;
    if( &mod == &it ) {
        p.add_msg_if_player( m_info,
                             _( "This can be used to repair or modify other items, not itself." ) );
        return 0;
    }

    // Gives us an item with the mod added or removed (toggled)
    const auto modded_copy = []( const item & proto, const std::string & mod_type ) {
        item mcopy = proto;
        if( mcopy.item_tags.count( mod_type ) == 0 ) {
            mcopy.item_tags.insert( mod_type );
        } else {
            mcopy.item_tags.erase( mod_type );
        }

        return mcopy;
    };

    // Cache available materials
    std::map< itype_id, bool > has_enough;
    const int items_needed = mod.volume() / 750_ml + 1;
    const inventory &crafting_inv = p.crafting_inventory();
    // Go through all discovered repair items and see if we have any of them available
    for( auto &cm : clothing_mods::get_all() ) {
        has_enough[cm.item_string] = crafting_inv.has_amount( cm.item_string, items_needed );
    }

    int mod_count = 0;
    for( auto &cm : clothing_mods::get_all() ) {
        mod_count += mod.item_tags.count( cm.flag );
    }

    // We need extra thread to lose it on bad rolls
    const int thread_needed = mod.volume() / 125_ml + 10;

    const auto valid_mods = mod.find_armor_data()->valid_mods;

    const auto get_compare_color = [&]( const int before, const int after,
    const bool higher_is_better ) {
        return before == after ? c_unset : ( ( after > before ) == higher_is_better ? c_light_green :
                                             c_red );
    };

    const auto format_desc_string = [&]( const std::string & label, const int before, const int after,
    const bool higher_is_better ) {
        return colorize( string_format( "%s: %d->%d\n", label, before, after ), get_compare_color( before,
                         after, higher_is_better ) );
    };

    uilist tmenu;
    tmenu.text = _( "How do you want to modify it?" );

    int index = 0;
    for( const clothing_mod_id &cm : clothing_mods ) {
        auto obj = cm.obj();
        item temp_item = modded_copy( mod, obj.flag );
        temp_item.update_clothing_mod_val();

        bool enab = false;
        std::string prompt;
        if( mod.item_tags.count( obj.flag ) == 0 ) {
            // TODO: Fix for UTF-8 strings
            // TODO: find other places where this is used and make a global function for all
            static const auto tolower = []( std::string t ) {
                if( !t.empty() ) {
                    t.front() = std::tolower( t.front() );
                }
                return t;
            };
            // Mod not already present, check if modification is possible
            if( it.charges < thread_needed ) {
                //~ %1$s: modification desc, %2$d: number of thread needed
                prompt = string_format( _( "Can't %1$s (need %2$d thread loaded)" ),
                                        tolower( obj.implement_prompt ), thread_needed );
            } else if( !has_enough[obj.item_string] ) {
                //~ %1$s: modification desc, %2$d: number of items needed, %3$s: items needed
                prompt = string_format( _( "Can't %1$s (need %2$d %3$s)" ), tolower( obj.implement_prompt ),
                                        items_needed, item::nname( obj.item_string, items_needed ) );
            } else if( obj.restricted &&
                       std::find( valid_mods.begin(), valid_mods.end(), obj.flag ) == valid_mods.end() ) {
                //~ %1$s: modification desc, %2$s: mod name
                prompt = string_format( _( "Can't %1$s (incompatible with %2$s)" ), tolower( obj.implement_prompt ),
                                        mod.tname( 1, false ) );
            } else {
                // Modification is possible
                enab = true;
                //~ %1$s: modification desc, %2$d: number of items needed, %3$s: items needed, %4$s: number of thread needed
                prompt = string_format( _( "%1$s (%2$d %3$s and %4$d thread)" ), tolower( obj.implement_prompt ),
                                        items_needed, item::nname( obj.item_string, items_needed ), thread_needed );
            }

        } else {
            // Mod already present, give option to destroy
            enab = true;
            prompt = _( obj.destroy_prompt );
        }
        std::string desc;
        desc += format_desc_string( _( "Bash" ), mod.bash_resist(), temp_item.bash_resist(), true );
        desc += format_desc_string( _( "Cut" ), mod.cut_resist(), temp_item.cut_resist(), true );
        desc += format_desc_string( _( "Ballistic" ), mod.bullet_resist(), temp_item.bullet_resist(),
                                    true );
        desc += format_desc_string( _( "Acid" ), mod.acid_resist(), temp_item.acid_resist(), true );
        desc += format_desc_string( _( "Fire" ), mod.fire_resist(), temp_item.fire_resist(), true );
        desc += format_desc_string( _( "Warmth" ), mod.get_warmth(), temp_item.get_warmth(), true );
        desc += format_desc_string( _( "Encumbrance" ), mod.get_encumber( p ), temp_item.get_encumber( p ),
                                    false );

        tmenu.addentry_desc( index++, enab, MENU_AUTOASSIGN, prompt, desc );
    }
    tmenu.textwidth = 80;
    tmenu.desc_enabled = true;
    tmenu.query();
    const int choice = tmenu.ret;

    if( choice < 0 || choice >= static_cast<int>( clothing_mods.size() ) ) {
        return 0;
    }

    // The mod player picked
    const std::string &the_mod = clothing_mods[choice].obj().flag;

    // If the picked mod already exists, player wants to destroy it
    if( mod.item_tags.count( the_mod ) ) {
        if( query_yn( _( "Are you sure?  You will not gain any materials back." ) ) ) {
            mod.item_tags.erase( the_mod );
        }
        mod.update_clothing_mod_val();

        return 0;
    }

    // Get the id of the material used
    const auto &repair_item = clothing_mods[choice].obj().item_string;

    std::vector<item_comp> comps;
    comps.push_back( item_comp( repair_item, items_needed ) );
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
        const auto destroyed = mod.inc_damage();
        const std::string resultdurability = mod.durability_indicator( true );
        p.add_msg_if_player( m_bad, _( "You damage your %s trying to modify it!  ( %s-> %s)" ),
                             mod.tname( 1, false ), startdurability, resultdurability );
        if( destroyed ) {
            p.add_msg_if_player( m_bad, _( "You destroy it!" ) );
            p.i_rem_keep_contents( p.get_item_position( &mod ) );
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
        mod.item_tags.insert( the_mod );
        mod.update_clothing_mod_val();
        return thread_needed;
    }

    p.add_msg_if_player( m_good, _( "You modify your %s!" ), mod.tname() );
    mod.item_tags.insert( the_mod );
    mod.update_clothing_mod_val();
    p.consume_items( comps, 1, is_crafting_component );
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
        obj.throw_error( "Invalid scent type id.", "scent_typeid" );
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

int change_scent_iuse::use( player &p, item &it, bool, const tripoint & ) const
{
    p.set_value( "prev_scent", p.get_type_of_scent().c_str() );
    if( waterproof ) {
        p.set_value( "waterproof_scent", "true" );
    }
    p.add_effect( efftype_id( "masked_scent" ), duration, num_bp, false, scent_mod );
    p.set_type_of_scent( scenttypeid );
    p.mod_moves( -moves );
    add_msg( m_info, _( "You use the %s to mask your scent" ), it.tname() );

    // Apply the various effects.
    for( const auto &eff : effects ) {
        p.add_effect( eff.id, eff.duration, eff.bp, eff.permanent );
    }
    return charges_to_use;
}

std::unique_ptr<iuse_actor> change_scent_iuse::clone() const
{
    return std::make_unique<change_scent_iuse>( *this );
}
