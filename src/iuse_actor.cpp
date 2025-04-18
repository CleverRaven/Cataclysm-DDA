#include "iuse_actor.h"

#include <imgui/imgui.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cwctype>
#include <functional>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <unordered_set>

#include "action.h"
#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "assign.h"
#include "avatar.h" // IWYU pragma: keep
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_imgui.h"
#include "catacharset.h"
#include "character.h"
#include "character_id.h"
#include "clothing_mod.h"
#include "clzones.h"
#include "condition.h"
#include "coordinates.h"
#include "crafting.h"
#include "creature.h"
#include "creature_tracker.h"
#include "damage.h"
#include "debug.h"
#include "dialogue.h"
#include "dialogue_helpers.h"
#include "effect.h"
#include "effect_on_condition.h"
#include "enum_conversions.h"
#include "enums.h"
#include "explosion.h"
#include "field_type.h"
#include "flag.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "game_inventory.h"
#include "generic_factory.h"
#include "iexamine.h"
#include "inventory.h"
#include "item.h"
#include "item_components.h"
#include "item_contents.h"
#include "item_group.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "map_selector.h"
#include "mapdata.h"
#include "material.h"
#include "memory_fast.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "music.h"
#include "mutation.h"
#include "npc.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player_activity.h"
#include "pocket_type.h"
#include "point.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "rng.h"
#include "safe_reference.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "talker.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "uilist.h"
#include "units_utility.h"
#include "value_ptr.h"
#include "veh_appliance.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "visitable.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "weather.h"

static const activity_id ACT_FIRSTAID( "ACT_FIRSTAID" );
static const activity_id ACT_REPAIR_ITEM( "ACT_REPAIR_ITEM" );
static const activity_id ACT_SPELLCASTING( "ACT_SPELLCASTING" );
static const activity_id ACT_START_FIRE( "ACT_START_FIRE" );
static const activity_id ACT_STUDY_SPELL( "ACT_STUDY_SPELL" );

static const damage_type_id damage_acid( "acid" );
static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_bullet( "bullet" );
static const damage_type_id damage_cut( "cut" );
static const damage_type_id damage_heat( "heat" );

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

static const furn_str_id furn_f_kiln_empty( "f_kiln_empty" );
static const furn_str_id furn_f_kiln_metal_empty( "f_kiln_metal_empty" );
static const furn_str_id furn_f_kiln_portable_empty( "f_kiln_portable_empty" );
static const furn_str_id furn_f_metal_smoking_rack( "f_metal_smoking_rack" );
static const furn_str_id furn_f_smoking_rack( "f_smoking_rack" );

static const gun_mode_id gun_mode_DEFAULT( "DEFAULT" );

static const itype_id itype_barrel_small( "barrel_small" );
static const itype_id itype_brazier( "brazier" );
static const itype_id itype_char_smoker( "char_smoker" );
static const itype_id itype_fire( "fire" );
static const itype_id itype_power_cord( "power_cord" );
static const itype_id itype_stock_none( "stock_none" );
static const itype_id itype_syringe( "syringe" );

static const json_character_flag json_flag_BIONIC_LIMB( "BIONIC_LIMB" );
static const json_character_flag json_flag_MANUAL_CBM_INSTALLATION( "MANUAL_CBM_INSTALLATION" );

static const morale_type morale_pyromania_nofire( "morale_pyromania_nofire" );
static const morale_type morale_pyromania_startfire( "morale_pyromania_startfire" );

static const proficiency_id proficiency_prof_traps( "prof_traps" );
static const proficiency_id proficiency_prof_trapsetting( "prof_trapsetting" );
static const proficiency_id proficiency_prof_wound_care( "prof_wound_care" );
static const proficiency_id proficiency_prof_wound_care_expert( "prof_wound_care_expert" );

static const quality_id qual_DIG( "DIG" );
static const quality_id qual_MOP( "MOP" );

static const skill_id skill_fabrication( "fabrication" );
static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_survival( "survival" );
static const skill_id skill_traps( "traps" );

static const trait_id trait_DEBUG_BIONICS( "DEBUG_BIONICS" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_LIGHTWEIGHT( "LIGHTWEIGHT" );
static const trait_id trait_PYROMANIA( "PYROMANIA" );
static const trait_id trait_TOLERANCE( "TOLERANCE" );

static const trap_str_id tr_firewood_source( "tr_firewood_source" );

static const zone_type_id zone_type_SOURCE_FIREWOOD( "SOURCE_FIREWOOD" );

std::unique_ptr<iuse_actor> iuse_transform::clone() const
{
    return std::make_unique<iuse_transform>( *this );
}

void iuse_transform::load( const JsonObject &obj, const std::string & )
{
    obj.read( "target", target, true );
    obj.read( "target_group", target_group, true );

    if( !target.is_empty() && !target_group.is_empty() ) {
        obj.throw_error_at( "target_group", "Cannot use both target and target_group at once" );
    }

    obj.read( "msg", msg_transform );
    obj.read( "variant_type", variant_type );
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

    obj.read( "target_timer", target_timer );

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

    obj.read( "need_empty", need_empty );

    obj.read( "qualities_needed", qualities_needed );

    obj.read( "menu_text", menu_text );
}

std::optional<int> iuse_transform::use( Character *p, item &it, const tripoint_bub_ms &pos ) const
{
    return iuse_transform::use( p, it, &get_map(), pos );
}

std::optional<int> iuse_transform::use( Character *p, item &it, map *,
                                        const tripoint_bub_ms & ) const
{
    int scale = 1;
    auto iter = it.type->ammo_scale.find( type );
    if( iter != it.type->ammo_scale.end() ) {
        scale = iter->second;
    }
    if( !p ) {
        debugmsg( "%s called action transform that requires character but no character is present",
                  it.typeId().str() );
        return std::nullopt;
    }

    it.set_var( "last_act_by_char_id", p->getID().get_value() );

    int result = 0;

    if( need_fire ) {
        if( !p->use_charges_if_avail( itype_fire, need_fire ) ) {
            p->add_msg_if_player( m_info, need_fire_msg, it.tname() );
            return std::nullopt;
        }
        if( p->cant_do_underwater() ) {
            return std::nullopt;
        }
    }

    if( !msg_transform.empty() ) {
        p->add_msg_if_player( m_neutral, msg_transform, it.tname() );
    }

    // Uses the moves specified by iuse_actor's definition
    p->mod_moves( -moves );

    if( need_fire && p->has_trait( trait_PYROMANIA ) ) {
        if( one_in( 2 ) ) {
            p->add_msg_if_player( m_mixed,
                                  _( "You light a fire, but it isn't enough.  You need to light more." ) );
        } else {
            p->add_msg_if_player( m_good, _( "You happily light a fire." ) );
            p->add_morale( morale_pyromania_startfire, 5, 10, 3_hours, 2_hours );
            p->rem_morale( morale_pyromania_nofire );
        }
    }

    if( target_group.is_empty() && it.count_by_charges() != target->count_by_charges() &&
        it.count() > 1 ) {
        item take_one = it.split( 1 );
        do_transform( p, take_one, variant_type );
        // TODO: Change to map aware operation when available
        p->i_add_or_drop( take_one );
    } else {
        do_transform( p, it, variant_type );
    }

    if( it.is_tool() ) {
        result = scale;
    }
    return result;
}

void iuse_transform::do_transform( Character *p, item &it, const std::string &variant_type ) const
{
    item obj_copy( it );
    item *obj;
    // defined here to allow making a new item assigned to the pointer
    item obj_it;
    if( container.is_empty() ) {
        if( !target_group.is_empty() ) {
            obj = &it.convert( item_group::item_from( target_group ).typeId(), p );
        } else {
            obj = &it.convert( target, p );
            obj->set_itype_variant( variant_type );
        }
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
        obj = &it.convert( container, p );
        obj->set_itype_variant( variant_type );
        int count = std::max( ammo_qty, 1 );
        item cont;
        if( !target_group.is_empty() ) {
            cont = item( item_group::item_from( target_group ).typeId(), calendar::turn );
        } else if( target->count_by_charges() ) {
            cont = item( target, calendar::turn, count );
            count = 1;
        } else {
            cont = item( target, calendar::turn );
        }
        for( int i = 0; i < count; i++ ) {
            if( !it.put_in( cont, pocket_type::CONTAINER ).success() ) {
                it.put_in( cont, pocket_type::MIGRATION );
            }
        }
        if( sealed ) {
            it.seal();
        }
    }

    if( target_timer > 0_seconds ) {
        obj->countdown_point = calendar::turn + target_timer;
    }
    obj->active = active || obj->has_temperature() || target_timer > 0_seconds;
    if( p && p->is_worn( *obj ) ) {
        if( !obj->is_armor() ) {
            item_location il = item_location( *p, obj );
            p->takeoff( il );
        } else {
            p->calc_encumbrance();
            p->update_bodytemp();
            p->on_worn_item_transform( obj_copy, *obj );
        }
    }

    p->clear_inventory_search_cache();
}

ret_val<void> iuse_transform::can_use( const Character &p, const item &it,
                                       const tripoint_bub_ms &pos ) const
{
    return iuse_transform::can_use( p, it, &get_map(), pos );
}

ret_val<void> iuse_transform::can_use( const Character &p, const item &it,
                                       map *here, const tripoint_bub_ms & ) const
{
    if( need_worn && !p.is_worn( it ) ) {
        return ret_val<void>::make_failure( _( "You need to wear the %1$s before activating it." ),
                                            it.tname() );
    }
    if( need_wielding && !p.is_wielding( it ) ) {
        return ret_val<void>::make_failure( _( "You need to wield the %1$s before activating it." ),
                                            it.tname() );
    }
    if( need_empty && !it.empty() ) {
        return ret_val<void>::make_failure( _( "You need to empty the %1$s before activating it." ),
                                            it.tname() );
    }

    if( p.is_worn( it ) ) {
        item tmp = item( target );
        if( !tmp.has_flag( flag_OVERSIZE ) && !tmp.has_flag( flag_INTEGRATED ) &&
            !tmp.has_flag( flag_SEMITANGIBLE ) ) {
            for( const trait_id &mut : p.get_functioning_mutations() ) {
                const mutation_branch &branch = mut.obj();
                if( branch.conflicts_with_item( tmp ) ) {
                    return ret_val<void>::make_failure( _( "Your %1$s mutation prevents you from doing that." ),
                                                        p.mutation_name( mut ) );
                }
            }
        }
    }

    if( need_charges && it.ammo_remaining( &p ) < need_charges ) {
        return ret_val<void>::make_failure( string_format( need_charges_msg, it.tname() ) );
    }

    if( qualities_needed.empty() ) {
        return ret_val<void>::make_success();
    }

    std::map<quality_id, int> unmet_reqs;
    inventory inv;
    inv.form_from_map( p.pos_bub( *here ), 1, &p, true, true );
    for( const auto &quality : qualities_needed ) {
        if( !p.has_quality( quality.first, quality.second ) &&
            !inv.has_quality( quality.first, quality.second ) ) {
            unmet_reqs.insert( quality );
        }
    }
    if( unmet_reqs.empty() ) {
        return ret_val<void>::make_success();
    }
    std::string unmet_reqs_string = enumerate_as_string( unmet_reqs.begin(), unmet_reqs.end(),
    [&]( const std::pair<quality_id, int> &unmet_req ) {
        return string_format( "%s %d", unmet_req.first.obj().name, unmet_req.second );
    } );
    return ret_val<void>::make_failure( n_gettext( "You need a tool with %s.",
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

void iuse_transform::finalize( const itype_id &my_item_type )
{
    if( !item::type_is_defined( target ) && target_group.is_empty() ) {
        debugmsg( "Invalid transform target: %s", target.c_str() );
    }

    if( !container.is_empty() ) {
        if( !item::type_is_defined( container ) ) {
            debugmsg( "Invalid transform container: %s", container.c_str() );
        }

        // todo: check contents fit container?
        // transform uses migration pocket if not
    }

    if( my_item_type.obj().can_use( "link_up" ) ) {
        // The linkage logic currently assumes that the links persist
        // through transformation, and fails pretty badly (segfaults)
        // if that happens to not be the case.
        // It is not unreasonable to want items that violate this assumption (one example is
        // the infamous Apple mouse which cannot be operated while charging),
        // but we don't have any of those implemented right now, so the check stays.
        if( !target.obj().can_use( "link_up" ) ) {
            debugmsg( "Item %s has link_up action, yet transforms into %s which doesn't.",
                      my_item_type.c_str(), target.c_str() );
        }
    }
}

void iuse_transform::info( const item &it, std::vector<iteminfo> &dump ) const
{

    if( !target_group.is_empty() ) {
        dump.emplace_back( "TOOL", _( "Can transform into one of several items" ) );
        return;
    }
    int amount = std::max( ammo_qty, 1 );
    item dummy( target, calendar::turn, target->count_by_charges() ? amount : 1 );
    dummy.set_itype_variant( variant_type );
    // If the variant is to be randomized, use default no-variant name
    if( variant_type == "<any>" ) {
        dummy.clear_itype_variant();
    }
    if( it.has_flag( flag_FIT ) ) {
        dummy.set_flag( flag_FIT );
    }
    if( target->count_by_charges() || !ammo_type.is_empty() ) {
        if( !ammo_type.is_empty() ) {
            dump.emplace_back( "TOOL", _( "<bold>Turns into</bold>: " ),
                               string_format( _( "%s (%d %s)" ), dummy.tname(), amount, ammo_type->nname( amount ) ) );
        } else if( !container.is_empty() ) {
            dump.emplace_back( "TOOL", _( "<bold>Turns into</bold>: " ),
                               amount > 1 ?
                               string_format( _( "%s (%d %s)" ),
                                              container->nname( 1 ), amount, target->nname( amount ) ) :
                               string_format( _( "%s (%s)" ),
                                              container->nname( 1 ), target->nname( amount ) ) );
        } else {
            dump.emplace_back( "TOOL", _( "<bold>Turns into</bold>: " ),
                               string_format( _( "%s (%d)" ), target->nname( amount ), amount ) );
        }
    } else {
        dump.emplace_back( "TOOL", _( "<bold>Turns into</bold>: " ),
                           amount > 1 ?
                           string_format( _( "%d %s" ), amount, target->nname( amount ) ) :
                           string_format( _( "%s" ), target->nname( amount ) ) );
    }

    if( target_timer > 0_seconds ) {
        dump.emplace_back( "TOOL", _( "Countdown: " ), to_seconds<int>( target_timer ) );
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

void unpack_actor::load( const JsonObject &obj, const std::string & )
{
    obj.read( "group", unpack_group );
    obj.read( "items_fit", items_fit );
    assign( obj, "filthy_volume_threshold", filthy_vol_threshold );
}

std::optional<int> unpack_actor::use( Character *p, item &it, const tripoint_bub_ms &pos ) const
{
    return unpack_actor::use( p, it, &get_map(), pos );
}
std::optional<int> unpack_actor::use( Character *p, item &it, map *here,
                                      const tripoint_bub_ms & ) const
{
    std::vector<item> items = item_group::items_from( unpack_group, calendar::turn );
    item last_armor;

    p->add_msg_if_player( _( "You unpack the %s." ), it.tname() );

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

        here->add_item_or_charges( p->pos_bub( *here ), content );
    }

    p->i_rem( &it );

    return 0;
}

void unpack_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    dump.emplace_back( "DESCRIPTION",
                       _( "This item could be unpacked to receive something." ) );
}

std::unique_ptr<iuse_actor> message_iuse::clone() const
{
    return std::make_unique<message_iuse>( *this );
}

void message_iuse::load( const JsonObject &obj, const std::string & )
{
    obj.read( "name", name );
    obj.read( "message", message );
}

std::optional<int> message_iuse::use( Character *p, item &it,
                                      const tripoint_bub_ms &pos ) const
{
    return message_iuse::use( p, it, &get_map(), pos );
}

std::optional<int> message_iuse::use( Character *p, item &it,
                                      map *here, const tripoint_bub_ms &pos ) const
{
    if( !p ) {
        return std::nullopt;
    }

    // TODO: Use map aware 'sees' when available.
    if( p->sees( *here, pos ) && !message.empty() ) {
        p->add_msg_if_player( m_info, message.translated(), it.tname() );
    }

    return 0;
}

std::string message_iuse::get_name() const
{
    if( !name.empty() ) {
        return name.translated();
    }
    return iuse_actor::get_name();
}

std::unique_ptr<iuse_actor> sound_iuse::clone() const
{
    return std::make_unique<sound_iuse>( *this );
}

void sound_iuse::load( const JsonObject &obj, const std::string & )
{
    obj.read( "name", name );
    obj.read( "sound_message", sound_message );
    obj.read( "sound_volume", sound_volume );
    obj.read( "sound_id", sound_id );
    obj.read( "sound_variant", sound_variant );
}

std::optional<int> sound_iuse::use( Character *, item &,
                                    const tripoint_bub_ms &pos ) const
{
    sounds::sound( pos, sound_volume, sounds::sound_t::alarm, sound_message.translated(), true,
                   sound_id, sound_variant );
    return 0;
}

std::optional<int> sound_iuse::use( Character *, item &,
                                    map *here, const tripoint_bub_ms &pos ) const
{
    map &bubble_map = reality_bubble();

    if( bubble_map.inbounds( here->get_abs( pos ) ) ) {
        sounds::sound( bubble_map.get_bub( here->get_abs( pos ) ), sound_volume, sounds::sound_t::alarm,
                       sound_message.translated(), true,
                       sound_id, sound_variant );
    }
    return 0;
}

std::string sound_iuse::get_name() const
{
    if( !name.empty() ) {
        return name.translated();
    }
    return iuse_actor::get_name();
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
static std::vector<tripoint_bub_ms> points_for_gas_cloud( map *here, const tripoint_bub_ms &center,
        int radius )
{
    std::vector<tripoint_bub_ms> result;
    for( const tripoint_bub_ms &p : closest_points_first( center, radius ) ) {
        if( here->impassable( p ) ) {
            continue;
        }
        if( p != center ) {
            if( !here->clear_path( center, p, radius, 1, 100 ) ) {
                // Can not splatter gas from center to that point, something is in the way
                continue;
            }
        }
        result.push_back( p );
    }
    return result;
}

void explosion_iuse::load( const JsonObject &obj, const std::string & )
{
    optional( obj, false, "explosion", explosion );
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
}

std::optional<int> explosion_iuse::use( Character *p, item &it, const tripoint_bub_ms &pos ) const
{
    return explosion_iuse::use( p, it, &get_map(), pos );
}
std::optional<int> explosion_iuse::use( Character *p, item &it, map *here,
                                        const tripoint_bub_ms &pos ) const
{
    if( explosion.power >= 0.0f ) {
        Character *source = p;
        if( it.has_var( "last_act_by_char_id" ) ) {
            character_id thrower( it.get_var( "last_act_by_char_id", 0 ) );
            if( thrower == get_player_character().getID() ) {
                source = &get_player_character();
            } else {
                source = g->find_npc( thrower );
            }
        }
        explosion_handler::explosion( source, here, pos, explosion );
    }

    map &bubble_map = reality_bubble();

    if( draw_explosion_radius >= 0 && bubble_map.inbounds( here->get_abs( pos ) ) ) {
        explosion_handler::draw_explosion( bubble_map.get_bub( here->get_abs( pos ) ),
                                           draw_explosion_radius, draw_explosion_color );
    }
    if( do_flashbang ) {
        // TODO: Use map aware 'flashbang' operation when available.
        explosion_handler::flashbang( pos, flashbang_player_immune );
    }
    if( fields_radius >= 0 && fields_type.id() ) {
        std::vector<tripoint_bub_ms> gas_sources = points_for_gas_cloud( here, pos, fields_radius );
        for( tripoint_bub_ms &gas_source : gas_sources ) {
            const int field_intensity = rng( fields_min_intensity, fields_max_intensity );
            here->add_field( gas_source, fields_type, field_intensity, 1_turns );
        }
    }
    if( scrambler_blast_radius >= 0 ) {
        for( const tripoint_bub_ms &dest : here->points_in_radius( pos, scrambler_blast_radius ) ) {
            // TODO: Use map aware 'scrambler_blast' when available.
            explosion_handler::scrambler_blast( dest );
        }
    }
    if( emp_blast_radius >= 0 ) {
        for( const tripoint_bub_ms &dest : here->points_in_radius( pos, emp_blast_radius ) ) {
            // TODO: Use map aware 'emp_blast' when available.
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

void consume_drug_iuse::load( const JsonObject &obj, const std::string & )
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
    obj.read( "used_up_item", used_up_item );

    for( JsonArray vit : obj.get_array( "vitamins" ) ) {
        int lo = vit.get_int( 1 );
        int hi = vit.size() >= 3 ? vit.get_int( 2 ) : lo;
        vitamins.emplace( vitamin_id( vit.get_string( 0 ) ), std::make_pair( lo, hi ) );
    }
}

void consume_drug_iuse::info( const item &, std::vector<iteminfo> &dump ) const
{
    const std::string vits = enumerate_as_string( vitamins.begin(), vitamins.end(),
    []( const decltype( vitamins )::value_type & v ) {
        const time_duration rate = get_player_character().vitamin_rate( v.first );
        if( rate <= 0_turns ) {
            return std::string();
        }

        const int lo = v.second.first;
        const int hi = v.second.second;

        if( v.first->type() == vitamin_type::VITAMIN ) {
            return string_format( lo == hi ? "%s (%i%%)" : "%s (%i-%i%%)", v.first.obj().name(), lo, hi );
        }
        return string_format( lo == hi ? "%s (%i U)" : "%s (%i-%i U)", v.first.obj().name(), lo, hi );
    } );

    if( !vits.empty() ) {
        dump.emplace_back( "TOOL", _( "Vitamins (RDA) and Compounds (U): " ), vits );
    }

    if( tools_needed.count( itype_syringe ) ) {
        dump.emplace_back( "TOOL", _( "You need a <info>syringe</info> to inject this drug." ) );
    }
}

std::optional<int> consume_drug_iuse::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return consume_drug_iuse::use( p, it, &get_map(), pos );
}

std::optional<int> consume_drug_iuse::use( Character *p, item &it, map *here,
        const tripoint_bub_ms & ) const
{
    auto need_these = tools_needed;

    // Check prerequisites first.
    for( const auto &tool : need_these ) {
        if( !p->has_amount( tool.first, 1 ) ) {
            p->add_msg_player_or_say( _( "You need %1$s to consume %2$s!" ),
                                      _( "I need a %1$s to consume %2$s!" ),
                                      item::nname( tool.first ),
                                      it.type_name( 1 ) );
            return std::nullopt;
        }
    }
    for( const auto &consumable : charges_needed ) {
        // Amount == -1 means need one, but don't consume it.
        if( !p->has_charges( consumable.first, ( consumable.second == -1 ) ?
                             1 : consumable.second ) ) {
            p->add_msg_player_or_say( _( "You need %1$s to consume %2$s!" ),
                                      _( "I need a %1$s to consume %2$s!" ),
                                      item::nname( consumable.first ),
                                      it.type_name( 1 ) );
            return std::nullopt;
        }
    }
    // Apply the various effects.
    for( const effect_data &eff : effects ) {
        time_duration dur = eff.duration;
        if( p->has_trait( trait_TOLERANCE ) ) {
            dur *= .8;
        } else if( p->has_trait( trait_LIGHTWEIGHT ) ) {
            dur *= 1.2;
        }
        p->add_effect( eff.id, dur, eff.bp, eff.permanent );
    }
    //Apply the various damage_over_time
    for( const damage_over_time_data &Dot : damage_over_time ) {
        p->add_damage_over_time( Dot );
    }
    for( const auto &stat_adjustment : stat_adjustments ) {
        p->mod_stat( stat_adjustment.first, stat_adjustment.second );
    }
    for( const auto &field : fields_produced ) {
        const field_type_id fid = field_type_id( field.first );
        for( int i = 0; i < 3; i++ ) {
            point_rel_ms offset( rng( -2, 2 ), rng( -2, 2 ) );
            here->add_field( p->pos_bub( *here ) + offset, fid, field.second );
        }
    }

    for( const auto &v : vitamins ) {
        const int lo = v.first->RDA_to_default( v.second.first );
        const int high = v.first->RDA_to_default( v.second.second );

        // have to update the daily estimate with the vitamins from the drug as well
        p->daily_vitamins[v.first].first += lo;

        p->vitamin_mod( v.first, rng( lo, high ) );
    }

    // Output message.
    p->add_msg_if_player( activation_message.translated(), it.type_name( 1 ) );
    // Consume charges.
    for( const auto &consumable : charges_needed ) {
        if( consumable.second != -1 ) {
            p->use_charges( consumable.first, consumable.second );
        }
    }

    if( !used_up_item.is_null() ) {
        item used_up( used_up_item, it.birthday() );
        // TODO: Use map aware 'i_add_or_drop' when available
        p->i_add_or_drop( used_up );
    }

    // Uses the moves specified by iuse_actor's definition
    p->mod_moves( -moves );
    return 1;
}

std::unique_ptr<iuse_actor> delayed_transform_iuse::clone() const
{
    return std::make_unique<delayed_transform_iuse>( *this );
}

void delayed_transform_iuse::load( const JsonObject &obj, const std::string &src )
{
    iuse_transform::load( obj, src );
    obj.get_member( "not_ready_msg" ).read( not_ready_msg );
    transform_age = obj.get_int( "transform_age" );
}

int delayed_transform_iuse::time_to_do( const item &it ) const
{
    // TODO: change return type to time_duration
    return transform_age - to_turns<int>( it.age() );
}

std::optional<int> delayed_transform_iuse::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return delayed_transform_iuse::use( p, it, &get_map(), pos );
}

std::optional<int> delayed_transform_iuse::use( Character *p, item &it,
        map *here, const tripoint_bub_ms &pos ) const
{
    if( time_to_do( it ) > 0 ) {
        p->add_msg_if_player( m_info, "%s", not_ready_msg );
        return std::nullopt;
    }
    return iuse_transform::use( p, it, here, pos );
}

std::unique_ptr<iuse_actor> place_monster_iuse::clone() const
{
    return std::make_unique<place_monster_iuse>( *this );
}

void place_monster_iuse::load( const JsonObject &obj, const std::string & )
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
            skills.emplace( s.get_string() );
        }
    }
}

std::optional<int> place_monster_iuse::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return place_monster_iuse::use( p, it, &get_map(), pos );
}

std::optional<int> place_monster_iuse::use( Character *p, item &it, map *here,
        const tripoint_bub_ms & ) const
{
    if( here != &reality_bubble() ) { // Because of the g usage below
        debugmsg( "Not supported for maps other than the reality bubble" );
        return std::nullopt;
    }

    if( it.ammo_remaining( ) < need_charges ) {
        p->add_msg_if_player( m_info, _( "This requires %d charges to activate." ), need_charges );
        return std::nullopt;
    }

    shared_ptr_fast<monster> newmon_ptr = make_shared_fast<monster>( mtypeid );
    monster &newmon = *newmon_ptr;
    newmon.init_from_item( it );
    if( place_randomly ) {
        // place_critter_around returns the same pointer as its parameter (or null)
        if( !g->place_critter_around( newmon_ptr, p->pos_bub( *here ), 1 ) ) {
            p->add_msg_if_player( m_info, _( "There is no adjacent square to release the %s in!" ),
                                  newmon.name() );
            return std::nullopt;
        }
    } else {
        const std::string query = string_format( _( "Place the %s where?" ), newmon.name() );
        const std::optional<tripoint_bub_ms> pnt_ = choose_adjacent( query );
        if( !pnt_ ) {
            return std::nullopt;
        }
        // place_critter_at returns the same pointer as its parameter (or null)
        if( !g->place_critter_at( newmon_ptr, *pnt_ ) ) {
            p->add_msg_if_player( m_info, _( "You cannot place a %s there." ), newmon.name() );
            return std::nullopt;
        }
    }
    // Uses the moves specified by iuse_actor's definition
    p->mod_moves( -moves );

    newmon.ammo = newmon.type->starting_ammo;
    if( !newmon.has_flag( mon_flag_INTERIOR_AMMO ) ) {
        for( std::pair<const itype_id, int> &amdef : newmon.ammo ) {
            item ammo_item( amdef.first, calendar::turn_zero );
            const int available = p->charges_of( amdef.first );
            if( available == 0 ) {
                amdef.second = 0;
                p->add_msg_if_player( m_info,
                                      _( "If you had standard factory-built %1$s bullets, you could load the %2$s." ),
                                      ammo_item.type_name( 2 ), newmon.name() );
                continue;
            }
            // Don't load more than the default from the monster definition.
            ammo_item.charges = std::min( available, amdef.second );
            p->use_charges( amdef.first, ammo_item.charges );
            //~ First %s is the ammo item (with plural form and count included), second is the monster name
            p->add_msg_if_player( n_gettext( "You load %1$d x %2$s round into the %3$s.",
                                             "You load %1$d x %2$s rounds into the %3$s.", ammo_item.charges ),
                                  ammo_item.charges, ammo_item.type_name( ammo_item.charges ),
                                  newmon.name() );
            amdef.second = ammo_item.charges;
        }
    }

    float skill_offset = 0;
    for( const skill_id &sk : skills ) {
        skill_offset += p->get_skill_level( sk ) / 2.0f;
    }
    /** @EFFECT_INT increases chance of a placed turret being friendly */
    if( rng( 0, p->int_cur / 2 ) + skill_offset < rng( 0, difficulty ) ) {
        if( hostile_msg.empty() ) {
            p->add_msg_if_player( m_bad, _( "You deploy the %s wrong.  It is hostile!" ), newmon.name() );
        } else {
            p->add_msg_if_player( m_bad, "%s", hostile_msg );
        }
    } else {
        if( friendly_msg.empty() ) {
            p->add_msg_if_player( m_warning, _( "You deploy the %s." ), newmon.name() );
        } else {
            p->add_msg_if_player( m_warning, "%s", friendly_msg );
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

void place_npc_iuse::load( const JsonObject &obj, const std::string & )
{
    npc_class_id = string_id<npc_template>( obj.get_string( "npc_class_id" ) );
    obj.read( "summon_msg", summon_msg );
    obj.read( "moves", moves );
    obj.read( "place_randomly", place_randomly );
}

std::optional<int> place_npc_iuse::use( Character *p, item &it, const tripoint_bub_ms &pos ) const
{
    return place_npc_iuse::use( p, it, &get_map(), pos );
}

std::optional<int> place_npc_iuse::use( Character *p, item &, map *here,
                                        const tripoint_bub_ms & ) const
{
    const tripoint_range<tripoint_bub_ms> target_range = place_randomly ?
            points_in_radius( p->pos_bub( *here ), radius ) :
            points_in_radius( choose_adjacent( _( "Place NPC where?" ) ).value_or( p->pos_bub( *here ) ), 0 );

    const std::optional<tripoint_bub_ms> target_pos =
    random_point( target_range, [here]( const tripoint_bub_ms & t ) {
        return here->passable_through( t ) &&
               !get_creature_tracker().creature_at( t );
    } );

    if( !target_pos.has_value() ) {
        p->add_msg_if_player( m_info, _( "There is no square to spawn NPC in!" ) );
        return std::nullopt;
    }

    here->place_npc( target_pos.value().xy(), npc_class_id );
    p->mod_moves( -moves );
    p->add_msg_if_player( m_info, "%s", summon_msg );
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

void deploy_furn_actor::load( const JsonObject &obj, const std::string & )
{
    furn_type = furn_str_id( obj.get_string( "furn_type" ) );
}


static ret_val<tripoint_bub_ms> check_deploy_square( Character *p, item &it,
        map *here, const tripoint_bub_ms &pos )
{
    if( p->cant_do_mounted() ) {
        return ret_val<tripoint_bub_ms>::make_failure( pos );
    }
    tripoint_bub_ms pnt( pos );
    if( pos == p->pos_bub( *here ) ) {
        // TODO: Use map aware 'choose_adjacent' when available, or reject operation if not reality bubble map
        if( const std::optional<tripoint_bub_ms> pnt_ = choose_adjacent( _( "Deploy where?" ) ) ) {
            pnt = *pnt_;
        } else {
            return ret_val<tripoint_bub_ms>::make_failure( pos );
        }
    }

    if( pnt == p->pos_bub( *here ) ) {
        return ret_val<tripoint_bub_ms>::make_failure( pos,
                _( "You attempt to become one with the %s.  It doesn't work." ), it.tname() );
    }

    tripoint_bub_ms where = pnt;
    tripoint_bub_ms below = pnt + tripoint::below;
    while( here->valid_move( where, below, false, true ) ) {
        where += tripoint::below;
        below += tripoint::below;
    }

    const int height = pnt.z() - where.z();
    if( height > 1 && here->has_flag_ter_or_furn( ter_furn_flag::TFLAG_NO_FLOOR, pnt ) ) {
        if( !query_yn(
                _( "Deploying %s there will make it fall down %i stories.  Do you still want to deploy it?" ),
                it.tname(), height ) ) {
            return ret_val<tripoint_bub_ms>::make_failure( pos );
        }
    }

    optional_vpart_position veh_there = here->veh_at( pnt );
    if( veh_there.has_value() ) {
        // TODO: check for protrusion+short furniture, wheels+tiny furniture, NOCOLLIDE flag, etc.
        // and/or integrate furniture deployment with construction (which already seems to perform these checks sometimes?)
        return ret_val<tripoint_bub_ms>::make_failure( pos,
                _( "The space under %s is too cramped to deploy a %s in." ),
                veh_there.value().vehicle().disp_name(), it.tname() );
    }

    // For example: dirt = 2, long grass = 3
    if( here->move_cost( pnt ) != 2 && here->move_cost( pnt ) != 3 ) {
        return ret_val<tripoint_bub_ms>::make_failure( pos, _( "You can't deploy a %s there." ),
                it.tname() );
    }

    if( here->has_furn( pnt ) ) {
        return ret_val<tripoint_bub_ms>::make_failure( pos,
                _( "The %s at that location is blocking the %s." ),
                here->furnname( pnt ), it.tname() );
    }

    if( here->has_items( pnt ) ) {
        // Check that there are no other people's belongings in the place where the furniture is placed.
        // Avoid easy theft of NPC items (e.g. carton theft).
        for( item &i : here->i_at( pnt ) ) {
            if( !i.is_owned_by( *p, true ) ) {
                return ret_val<tripoint_bub_ms>::make_failure( pos,
                        _( "You can't deploy the %s on other people's belongings!" ), it.tname() );
            }
        }

        // Check that there is no liquid on the floor.
        // If there is, it needs to be mopped dry with a mop.
        if( here->terrain_moppable( pnt ) ) {
            if( get_avatar().crafting_inventory().has_quality( qual_MOP ) ) {
                here->mop_spills( pnt );
                p->add_msg_if_player( m_info, _( "You mopped up the spill with a nearby mop when deploying a %s." ),
                                      it.tname() );
                p->mod_moves( -to_moves<int>( 15_seconds ) );
            } else {
                return ret_val<tripoint_bub_ms>::make_failure( pos,
                        _( "You need a mop to clean up liquids before deploying the %s." ), it.tname() );
            }
        }
    }

    return ret_val<tripoint_bub_ms>::make_success( pnt );
}

std::optional<int> deploy_furn_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return deploy_furn_actor::use( p, it, &get_map(), pos );
}

std::optional<int> deploy_furn_actor::use( Character *p, item &it,
        map *here, const tripoint_bub_ms &pos ) const
{
    ret_val<tripoint_bub_ms> suitable = check_deploy_square( p, it, here, pos );
    if( !suitable.success() ) {
        p->add_msg_if_player( m_info, suitable.str() );
        return std::nullopt;
    }

    here->furn_set( suitable.value(), furn_type, false, false, true );
    it.spill_contents( suitable.value() );
    p->mod_moves( -to_moves<int>( 2_seconds ) );
    return 1;
}

std::unique_ptr<iuse_actor> deploy_appliance_actor::clone() const
{
    return std::make_unique<deploy_appliance_actor>( *this );
}

void deploy_appliance_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    dump.emplace_back( "DESCRIPTION",
                       string_format( _( "Can be <info>activated</info> to deploy as an appliance (<stat>%s</stat>)." ),
                                      vpart_appliance_from_item( appliance_base )->name() ) );
}

void deploy_appliance_actor::load( const JsonObject &obj, const std::string & )
{
    mandatory( obj, false, "base", appliance_base );
}

std::optional<int> deploy_appliance_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return deploy_appliance_actor::use( p, it, &get_map(), pos );
}

std::optional<int> deploy_appliance_actor::use( Character *p, item &it,
        map *here, const tripoint_bub_ms &pos ) const
{
    ret_val<tripoint_bub_ms> suitable = check_deploy_square( p, it, here, pos );
    if( !suitable.success() ) {
        p->add_msg_if_player( m_info, suitable.str() );
        return std::nullopt;
    }

    // TODO: Use map aware operation when available
    it.spill_contents( suitable.value() );
    // TODO: Use map aware operation when available
    if( !place_appliance( *here, suitable.value(),
                          vpart_appliance_from_item( appliance_base ), *p, it ) ) {
        // failed to place somehow, cancel!!
        return 0;
    }
    p->mod_moves( -to_moves<int>( 2_seconds ) );
    return 1;
}

std::unique_ptr<iuse_actor> reveal_map_actor::clone() const
{
    return std::make_unique<reveal_map_actor>( *this );
}

void reveal_map_actor::load( const JsonObject &obj, const std::string & )
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
        if( overmap_buffer.seen( place ) != om_vision_level::full ) {
            // Should be replaced with the character using the item passed as an argument if NPCs ever learn to use maps
            get_avatar().map_revealed_omts.emplace( place );
        }
        overmap_buffer.reveal( place, reveal_distance );
    }
}

std::optional<int> reveal_map_actor::use( Character *p, item &it, const tripoint_bub_ms &pos ) const
{
    return reveal_map_actor::use( p, it, &get_map(), pos );
}

std::optional<int> reveal_map_actor::use( Character *p, item &it, map *,
        const tripoint_bub_ms & ) const
{
    if( it.already_used_by_player( *p ) ) {
        p->add_msg_if_player( _( "There isn't anything new on the %s." ), it.tname() );
        return std::nullopt;
    } else if( p->fine_detail_vision_mod() > 4 ) {
        p->add_msg_if_player( _( "It's too dark to read." ) );
        return std::nullopt;
    }
    const tripoint_abs_omt center( coords::project_to<coords::omt>( it.get_var( "reveal_map_center",
                                   p->pos_abs() ) ) );
    // Clear highlight on previously revealed OMTs before revealing new ones
    p->map_revealed_omts.clear();
    for( const auto &omt : omt_types ) {
        for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
            reveal_targets( tripoint_abs_omt( center.xy(), z ), omt, 0 );
        }
    }
    if( !message.empty() ) {
        p->add_msg_if_player( m_good, "%s", message );
    }
    if( p->map_revealed_omts.empty() ) {
        p->add_msg_if_player( _( "You didn't learn anything new from the %s." ), it.tname() );
    }
    it.mark_as_used_by_player( *p );
    return 0;
}

void firestarter_actor::load( const JsonObject &obj, const std::string & )
{
    moves_cost_fast = obj.get_int( "moves", moves_cost_fast );
    moves_cost_slow = obj.get_int( "moves_slow", moves_cost_fast * 10 );
    need_sunlight = obj.get_bool( "need_sunlight", false );
}

std::unique_ptr<iuse_actor> firestarter_actor::clone() const
{
    return std::make_unique<firestarter_actor>( *this );
}

firestarter_actor::start_type firestarter_actor::prep_firestarter_use( Character &p,
        map *here, tripoint_bub_ms &pos )
{
    if( here != &reality_bubble() ) { // Unless 'choose_adjacent' gets map aware.
        debugmsg( "Usage outside reality bubble is not supported" );
        return start_type::NONE;
    }

    // checks for fuel are handled by use and the activity, not here
    if( pos == p.pos_bub( *here ) ) {
        if( const std::optional<tripoint_bub_ms> pnt_ = choose_adjacent( _( "Light where?" ) ) ) {
            pos = *pnt_;
        } else {
            return start_type::NONE;
        }
    }
    if( pos == p.pos_bub( *here ) ) {
        p.add_msg_if_player( m_info, _( "You would set yourself on fire." ) );
        p.add_msg_if_player( _( "But you're already smokin' hot." ) );
        return start_type::NONE;
    }

    const furn_id &f_id = here->furn( pos );
    const bool is_smoking_rack = f_id == furn_f_metal_smoking_rack ||
                                 f_id == furn_f_smoking_rack;
    const bool is_kiln = f_id == furn_f_kiln_empty ||
                         f_id == furn_f_kiln_metal_empty || f_id == furn_f_kiln_portable_empty;

    if( is_smoking_rack ) {
        return iexamine::smoker_prep( p, pos ) ? start_type::SMOKER : start_type::NONE;
    } else if( is_kiln ) {
        return iexamine::kiln_prep( p, pos ) ? start_type::KILN : start_type::NONE;
    }

    if( here->get_field( pos, fd_fire ) ) {
        // check if there's already a fire
        p.add_msg_if_player( m_info, _( "There is already a fire." ) );
        return start_type::NONE;
    }
    // check if there's a fire fuel source spot
    bool target_is_firewood = false;
    if( here->tr_at( pos ).id == tr_firewood_source ) {
        target_is_firewood = true;
    } else {
        zone_manager &mgr = zone_manager::get_manager();
        auto zones = mgr.get_zones( zone_type_SOURCE_FIREWOOD, here->get_abs( pos ) );
        if( !zones.empty() ) {
            target_is_firewood = true;
        }
    }
    if( target_is_firewood ) {
        if( !query_yn( _( "Do you really want to burn your firewood source?" ) ) ) {
            return start_type::NONE;
        }
    }
    // Check for an adjacent fire container
    for( const tripoint_bub_ms &query : here->points_in_radius( pos, 1 ) ) {
        // Don't ask if we're setting a fire on top of a fireplace
        if( here->has_flag_furn( "FIRE_CONTAINER", pos ) ) {
            break;
        }
        // Skip the position we're trying to light on fire
        if( query == pos ) {
            continue;
        }
        if( here->has_flag_furn( "FIRE_CONTAINER", query ) ) {
            if( !query_yn( _( "Are you sure you want to start fire here?  There's a fireplace adjacent." ) ) ) {
                return start_type::NONE;
            } else {
                // Don't ask multiple times if they say no and there are multiple fireplaces
                break;
            }
        }
    }
    // Check for a brazier.
    bool has_unactivated_brazier = false;
    for( const item &i : here->i_at( pos ) ) {
        if( i.typeId() == itype_brazier ) {
            has_unactivated_brazier = true;
        }
    }
    if( has_unactivated_brazier &&
        !query_yn(
            _( "There's a brazier there but you haven't set it up to contain the fire.  Continue?" ) ) ) {
        return start_type::NONE;
    }

    return start_type::FIRE;
}

void firestarter_actor::resolve_firestarter_use( Character *p, map *here,
        const tripoint_bub_ms &pos, start_type st )
{
    if( firestarter_actor::resolve_start( p, here, pos, st ) ) {
        if( !p->has_trait( trait_PYROMANIA ) ) {
            p->add_msg_if_player( _( "You successfully light a fire." ) );
        } else {
            if( one_in( 4 ) ) {
                p->add_msg_if_player( m_mixed,
                                      _( "You light a fire, but it isn't enough.  You need to light more." ) );
            } else {
                p->add_msg_if_player( m_good, _( "You happily light a fire." ) );
                p->add_morale( morale_pyromania_startfire, 5, 10, 6_hours, 4_hours );
                p->rem_morale( morale_pyromania_nofire );
            }
        }
    }
}

bool firestarter_actor::resolve_start( Character *p, map *here,
                                       const tripoint_bub_ms &pos, start_type type )
{
    switch( type ) {
        case start_type::FIRE:
            return here->add_field( pos, fd_fire, 1, 10_minutes );
        case start_type::SMOKER:
            return iexamine::smoker_fire( *p, pos );
        case start_type::KILN:
            return iexamine::kiln_fire( *p, pos );
        case start_type::NONE:
        default:
            return false;
    }
}

ret_val<void> firestarter_actor::can_use( const Character &p, const item &it,
        const tripoint_bub_ms &pos ) const
{
    return firestarter_actor::can_use( p, it, &get_map(), pos );
}

ret_val<void> firestarter_actor::can_use( const Character &p, const item &it,
        map *here, const tripoint_bub_ms & ) const
{
    if( p.is_underwater() ) {
        return ret_val<void>::make_failure( _( "You can't do that while underwater." ) );
    }

    if( !it.ammo_sufficient( &p ) ) {
        return ret_val<void>::make_failure( _( "This tool doesn't have enough charges." ) );
    }

    if( need_sunlight && light_mod( here, p.pos_bub( *here ) ) <= 0.0f ) {
        return ret_val<void>::make_failure( _( "You need direct sunlight to light a fire with this." ) );
    }

    return ret_val<void>::make_success();
}

float firestarter_actor::light_mod( map *here, const tripoint_bub_ms &pos ) const
{
    if( here != &reality_bubble() ) {
        debugmsg( "not supported outside the reality bubble" );
        return 0.0f;
    }
    if( !need_sunlight ) {
        return 1.0f;
    }
    if( g->is_sheltered( pos ) ) {
        return 0.0f;
    }

    if( incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::moderate ) {
        return std::pow( g->natural_light_level( pos.z() ) / 80.0f, 8 );
    }

    return 0.0f;
}

int firestarter_actor::moves_cost_by_fuel( const tripoint_bub_ms &pos ) const
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

std::optional<int> firestarter_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return firestarter_actor::use( p, it, &get_map(), pos );
}

std::optional<int> firestarter_actor::use( Character *p, item &it,
        map *here,  const tripoint_bub_ms &spos ) const
{
    if( !p ) {
        debugmsg( "%s called action firestarter that requires character but no character is present",
                  it.typeId().str() );
        return std::nullopt;
    }

    tripoint_bub_ms pos = spos;

    float light = light_mod( here, p->pos_bub( *here ) );
    start_type st = prep_firestarter_use( *p, here, pos );
    if( st == start_type::NONE ) {
        return std::nullopt;
    }

    double skill_level = p->get_skill_level( skill_survival );
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
        p->add_msg_if_player( m_info, need_sunlight ?
                              n_gettext( "If the current weather holds, it will take around %d minute to light a fire.",
                                         "If the current weather holds, it will take around %d minutes to light a fire.", minutes ) :
                              n_gettext( "At your skill level, it will take around %d minute to light a fire.",
                                         "At your skill level, it will take around %d minutes to light a fire.", minutes ),
                              minutes );
    } else if( moves < to_moves<int>( 2_turns ) && here->is_flammable( pos ) ) {
        // If less than 2 turns, don't start a long action
        resolve_firestarter_use( p, here,  pos, st );
        p->mod_moves( -moves );
        return 1;
    }

    // skill gains are handled by the activity, but stored here in the index field
    const int potential_skill_gain = moves_modifier * ( std::min( 10.0, moves_cost_fast / 100.0 ) + 2 );
    p->assign_activity( ACT_START_FIRE, moves, potential_skill_gain,
                        0, it.tname() );
    p->activity.targets.emplace_back( *p, &it );
    p->activity.values.push_back( g->natural_light_level( pos.z() ) );
    p->activity.placement = here->get_abs( pos );
    // charges to use are handled by the activity
    return 0;
}

void salvage_actor::load( const JsonObject &obj, const std::string & )
{
    assign( obj, "cost", cost );
    assign( obj, "moves_per_part", moves_per_part );
}

std::unique_ptr<iuse_actor> salvage_actor::clone() const
{
    return std::make_unique<salvage_actor>( *this );
}

std::optional<int> salvage_actor::use( Character *p, item &cutter, const tripoint_bub_ms & ) const
{
    if( !p ) {
        debugmsg( "%s called action salvage that requires character but no character is present",
                  cutter.typeId().str() );
        return std::nullopt;
    }

    item_location item_loc = game_menus::inv::salvage( *p, this );
    if( !item_loc ) {
        add_msg( _( "Never mind." ) );
        return std::nullopt;
    }

    const item &to_cut = *item_loc;
    if( !to_cut.is_owned_by( *p, true ) ) {
        if( !query_yn( _( "Cutting the %s may anger the people who own it, continue?" ),
                       to_cut.tname() ) ) {
            return false;
        } else {
            if( to_cut.get_owner() ) {
                g->on_witness_theft( to_cut );
            }
        }
    }

    return salvage_actor::try_to_cut_up( *p, cutter, item_loc );
}

std::optional<int> salvage_actor::use( Character *p, item &cutter, map *,
                                       const tripoint_bub_ms & ) const
{
    if( !p ) {
        debugmsg( "%s called action salvage that requires character but no character is present",
                  cutter.typeId().str() );
        return std::nullopt;
    }

    item_location item_loc = game_menus::inv::salvage( *p, this );
    if( !item_loc ) {
        add_msg( _( "Never mind." ) );
        return std::nullopt;
    }

    const item &to_cut = *item_loc;
    if( !to_cut.is_owned_by( *p, true ) ) {
        if( !query_yn( _( "Cutting the %s may anger the people who own it, continue?" ),
                       to_cut.tname() ) ) {
            return false;
        } else {
            if( to_cut.get_owner() ) {
                g->on_witness_theft( to_cut );
            }
        }
    }

    return salvage_actor::try_to_cut_up( *p, cutter, item_loc );
}

std::optional<int> salvage_actor::try_to_cut_up
( Character &p, item &cutter, item_location &cut ) const
{
    if( !valid_to_cut_up( &p, *cut.get_item() ) ) {
        // Messages should have already been displayed.
        return std::nullopt;
    }

    if( &cutter == cut.get_item() ) {
        add_msg( m_info, _( "You can not cut the %s with itself." ), cutter.tname() );
        return std::nullopt;
    }

    salvage_actor::cut_up( p, cut );
    // Return used charges from cutter
    return cost >= 0 ? cost : 1;
}

// Helper to visit instances of all the sub-materials of an item.
static void visit_salvage_products( const item &it,
                                    const std::function<void( const item & )> &func )
{
    for( const auto &material : it.made_of() ) {
        if( const std::optional<itype_id> id = material.first->salvaged_into() ) {
            item exemplar( *id );
            func( exemplar );
        }
    }
}

static units::mass proportional_weight( const item &it, int material_portions )
{
    int total_portions = std::max( 1, it.type->mat_portion_total );
    return it.weight() * material_portions / total_portions;
}

// Helper to determine if there's enough of any material to even produce a result when cut up.
static bool can_produce_results( const item &it )
{
    for( const std::pair<const material_id, int> &mat : it.made_of() ) {
        if( const std::optional<itype_id> mat_id = mat.first->salvaged_into() ) {
            item material( *mat_id );
            if( material.count_by_charges() ) {
                material.charges = 1;
            }
            if( material.weight() <= proportional_weight( it, mat.second ) ) {
                return true;
            }
        }
    }
    return false;
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

// If p is a nullptr, it does not print messages or query for confirmation
// it here is the item that is a candidate for being cut up.
bool salvage_actor::valid_to_cut_up( const Character *const p, const item &it ) const
{
    // There must be some historical significance to these items.
    if( !it.is_salvageable() ) {
        if( p ) {
            add_msg( m_info, _( "Can't salvage anything from %s." ), it.tname() );
        }
        if( it.is_disassemblable() ) {
            if( p ) {
                add_msg( m_info, _( "Try disassembling the %s instead." ), it.tname() );
            }
        }
        return false;
    }

    if( !it.empty() ) {
        if( p ) {
            add_msg( m_info, _( "Please empty the %s before cutting it up." ), it.tname() );
        }
        return false;
    }
    if( !can_produce_results( it ) ) {
        if( p ) {
            add_msg( m_info, _( "The %s is too small to salvage material from." ), it.tname() );
        }
        return false;
    }
    if( p ) {
        // Softer warnings at the end so we don't ask permission and then tell them no.
        if( &it == p->get_wielded_item().get_item() ) {
            return query_yn( _( "You are wielding that, are you sure?" ) );
        } else if( p->is_worn( it ) ) {
            return query_yn( _( "You're wearing that, are you sure?" ) );
        }
    }

    return true;
}

// Find a recipe that can be used to craft item x. Searches craft and uncraft recipes
// Used only by salvage_actor::cut_up
static std::optional<recipe> find_uncraft_recipe( const item &x )
{
    auto is_valid_uncraft = [&x]( const recipe & curr ) -> bool {
        return !( curr.obsolete || curr.result() != x.typeId()
                  || curr.makes_amount() > 1 || curr.is_null() );
    };

    // Check uncraft first, then crafting recipes if none was found
    recipe uncraft = recipe_dictionary::get_uncraft( x.typeId() );
    if( is_valid_uncraft( uncraft ) ) {
        return uncraft;
    }

    auto iter = std::find_if( recipe_dict.begin(), recipe_dict.end(),
    [&]( const std::pair<const recipe_id, recipe> &curr ) {
        return is_valid_uncraft( curr.second );
    } );
    if( iter != recipe_dict.end() ) {
        return iter->second;
    }
    return std::nullopt;
}

void salvage_actor::cut_up( Character &p, item_location &cut ) const
{
    map &here = get_map();

    // Map of salvaged items (id, count)
    std::map<itype_id, int> salvage;
    std::map<material_id, units::mass> mat_to_weight;
    std::set<material_id> mat_set;
    for( const std::pair<const material_id, int> &mat : cut.get_item()->made_of() ) {
        mat_set.insert( mat.first );
    }

    // Calculate efficiency losses
    float efficiency = 1.0;
    // Higher fabrication, less chance of entropy, but still a chance.
    /** @EFFECT_FABRICATION reduces chance of losing components when cutting items up */
    int entropy_threshold = std::max( 0,
                                      5 - static_cast<int>( round( p.get_skill_level( skill_fabrication ) ) ) );
    if( rng( 1, 10 ) <= entropy_threshold ) {
        efficiency *= 0.9;
    }

    // Fail dex roll, potentially lose more parts.
    /** @EFFECT_DEX randomly reduces component loss when cutting items up */
    if( dice( 3, 4 ) > p.dex_cur ) {
        efficiency *= 0.95;
    }

    // If the item being cut is damaged, additional losses will be incurred.
    efficiency *= std::pow( 0.8, cut.get_item()->damage_level() );

    auto distribute_uniformly = [&mat_to_weight]( const item & x, float num_adjusted ) -> void {
        for( const auto &type : x.made_of() )
        {
            mat_to_weight[type.first] += proportional_weight( x, type.second ) * num_adjusted;
        }
    };

    // efficiency is decreased every time the ingredients of a recipe have more mass than the output
    // num_adjusted represents the number of items and efficiency in one value
    std::function<void( item, float )> cut_up_component =
        [&salvage, &mat_set, &distribute_uniformly, &cut_up_component]
    ( const item & curr, float num_adjusted ) -> void {

        // If it is one of the basic components, add it into the list
        if( curr.type->is_basic_component() )
        {
            int num_actual = static_cast<int>( num_adjusted );
            salvage[curr.typeId()] += num_actual;
            return;
        }

        // Discard invalid component
        // Non-salvageable items are discarded even if made of appropriate material
        if( !curr.made_of_any( mat_set ) || !curr.is_salvageable() )
        {
            return;
        }

        // Items count by charges are not always smaller than base materials
        // Necessary for e.g. bones -> bone splinters
        if( curr.count_by_charges() )
        {
            distribute_uniformly( curr, num_adjusted );
            return;
        }

        // All intact components are also cut up and destroyed
        if( !curr.components.empty() )
        {
            for( const item_components::type_vector_pair &tvp : curr.components ) {
                for( const item &iter : tvp.second ) {
                    cut_up_component( iter, num_adjusted );
                }
            }
            return;
        }

        // Try to find an available recipe and "restore" its components
        std::optional<recipe> uncraft = find_uncraft_recipe( curr );
        if( uncraft )
        {
            const requirement_data requirements = uncraft->simple_requirements();

            units::mass ingredient_weight = 0_gram;
            for( const auto &altercomps : requirements.get_components() ) {
                if( !altercomps.empty() && altercomps.front().type ) {
                    ingredient_weight += altercomps.front().type->weight * altercomps.front().count;
                }
            }
            // We decrease efficiency so on avg no more mass is salvaged than the original item weighed
            num_adjusted *= std::min( 1.0f, static_cast<float>( curr.weight().value() )
                                      / static_cast<float>( ingredient_weight.value() ) );

            // Find default components set from recipe
            for( const auto &altercomps : requirements.get_components() ) {
                const item_comp &comp = altercomps.front();
                item next = item( comp.type, calendar::turn );
                if( next.count_by_charges() ) {
                    next.charges = 1;
                }
                cut_up_component( next, num_adjusted * comp.count );
            }
        } else
        {
            // No recipe was found so we guess and distribute the weight uniformly.
            // This is imprecise but it can't be exploited as no recipe exists for the item
            distribute_uniformly( curr, num_adjusted );
        }
    };

    // Decompose the item into irreducible parts
    cut_up_component( *cut.get_item(), efficiency );

    // Not much practice, and you won't get very far ripping things up.
    p.practice( skill_fabrication, rng( 0, 5 ), 1 );

    // Add the uniformly distributed mass to the relevant salvage items
    for( const auto &iter : mat_to_weight ) {
        if( const std::optional<itype_id> id = iter.first->salvaged_into() ) {
            salvage[*id] += iter.second / id->obj().weight;
        }
    }

    add_msg( m_info, _( "You try to salvage materials from the %s." ),
             cut.get_item()->tname() );

    const item_location::type cut_type = cut.where();
    const tripoint_bub_ms pos = cut.pos_bub( here );
    const bool filthy = cut.get_item()->is_filthy();

    // Clean up before removing the item.
    remove_ammo( *cut.get_item(), p );
    // Original item has been consumed.
    cut.remove_item();
    // Force an encumbrance update in case they were wearing that item.
    p.calc_encumbrance();

    for( const auto &salvaged_mat : salvage ) {
        item result( salvaged_mat.first, calendar::turn );
        int amount = salvaged_mat.second;
        if( amount > 0 ) {
            // Time based on number of components.
            p.mod_moves( -moves_per_part );
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
                    put_into_vehicle_or_drop( p, item_drop_reason::deliberate, { result }, &here, pos );
                }
            }
        } else {
            add_msg( m_bad, _( "Could not salvage a %s." ), result.display_name() );
        }
    }
}

void inscribe_actor::load( const JsonObject &obj, const std::string & )
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

std::optional<int> inscribe_actor::use( Character *p, item &it, const tripoint_bub_ms &pos ) const
{
    return inscribe_actor::use( p, it, &get_map(), pos );
}

std::optional<int> inscribe_actor::use( Character *p, item &it, map *here,
                                        const tripoint_bub_ms & ) const
{
    if( here != &reality_bubble() ) { // or make 'choose_adjacent' map aware.
        debugmsg( "%s called action inscribe that can only be performed in the reality bubble",
                  it.typeId().str() );
        return std::nullopt;
    }

    if( !p ) {
        debugmsg( "%s called action inscribe that requires character but no character is present",
                  it.typeId().str() );
        return std::nullopt;
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
        return std::nullopt;
    }

    if( choice == 0 ) {
        const std::optional<tripoint_bub_ms> dest_ = choose_adjacent( _( "Write where?" ) );
        if( !dest_ ) {
            return std::nullopt;
        }
        return iuse::handle_ground_graffiti( *p, &it, string_format( _( "%s what?" ), verb ),
                                             here, dest_.value() );
    }

    item_location loc = game_menus::inv::titled_menu( get_avatar(), _( "Inscribe which item?" ) );
    if( !loc ) {
        p->add_msg_if_player( m_info, _( "Never mind." ) );
        return std::nullopt;
    }
    item &cut = *loc;
    if( &cut == &it ) {
        p->add_msg_if_player( _( "You try to bend your %s, but fail." ), it.tname() );
        return 0;
    }
    // inscribe_item returns false if the action fails or is canceled somehow.

    if( item_inscription( it, cut ) ) {
        return cost >= 0 ? cost : 1;
    }

    return std::nullopt;
}

void fireweapon_off_actor::load( const JsonObject &obj, const std::string & )
{
    obj.read( "target_id", target_id, true );
    obj.read( "success_message", success_message );
    obj.read( "failure_message", failure_message );
    noise               = obj.get_int( "noise", 0 );
    moves               = obj.get_int( "moves", 0 );
    success_chance      = obj.get_int( "success_chance", INT_MIN );
}

std::unique_ptr<iuse_actor> fireweapon_off_actor::clone() const
{
    return std::make_unique<fireweapon_off_actor>( *this );
}

std::optional<int> fireweapon_off_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return fireweapon_off_actor::use( p, it, &get_map(), pos );
}

std::optional<int> fireweapon_off_actor::use( Character *p, item &it,
        map *, const tripoint_bub_ms & ) const
{
    if( !p ) {
        debugmsg( "%s called action fireweapon_off that requires character but no character is present",
                  it.typeId().str() );
        return std::nullopt;
    }

    // Uses the moves specified by iuse_actor's definition
    p->mod_moves( -moves );
    if( rng( 0, 10 ) - it.damage_level() > success_chance && !p->is_underwater() ) {
        if( noise > 0 ) {
            map &bubble_map = reality_bubble();

            if( bubble_map.inbounds( p->pos_abs() ) ) {
                sounds::sound( bubble_map.get_bub( p->pos_abs( ) ), noise, sounds::sound_t::combat,
                               success_message );
            }
        } else {
            p->add_msg_if_player( "%s", success_message );
        }

        it.convert( target_id, p );
        it.active = true;
    } else if( !failure_message.empty() ) {
        p->add_msg_if_player( m_bad, "%s", failure_message );
    }

    return 1;
}

ret_val<void> fireweapon_off_actor::can_use( const Character &p, const item &it,
        const tripoint_bub_ms &pos ) const
{
    return fireweapon_off_actor::can_use( p, it, &get_map(), pos );
}

ret_val<void> fireweapon_off_actor::can_use( const Character &p, const item &it,
        map *, const tripoint_bub_ms & ) const
{
    if( !it.ammo_sufficient( &p ) ) {
        return ret_val<void>::make_failure( _( "This tool doesn't have enough charges." ) );
    }

    if( p.is_underwater() ) {
        return ret_val<void>::make_failure( _( "You can't do that while underwater." ) );
    }

    return ret_val<void>::make_success();
}

void fireweapon_on_actor::load( const JsonObject &obj, const std::string & )
{
    obj.read( "noise_message", noise_message );
    obj.get_member( "charges_extinguish_message" ).read( charges_extinguish_message );
    obj.get_member( "water_extinguish_message" ).read( water_extinguish_message );
    noise_chance                    = obj.get_int( "noise_chance", 1 );
}

std::unique_ptr<iuse_actor> fireweapon_on_actor::clone() const
{
    return std::make_unique<fireweapon_on_actor>( *this );
}

std::optional<int> fireweapon_on_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return fireweapon_on_actor::use( p, it, &get_map(), pos );
}

std::optional<int> fireweapon_on_actor::use( Character *p, item &it,
        map *here, const tripoint_bub_ms &pos ) const
{
    bool extinguish = true;
    translation deactivation_msg;
    if( !it.ammo_sufficient( p ) ) {
        deactivation_msg = charges_extinguish_message;
    } else if( p && p->is_underwater() ) {
        deactivation_msg = water_extinguish_message;
    } else {
        extinguish = false;
    }

    if( !p ) {
        if( here->is_water_shallow_current( pos ) || here->is_divable( pos ) ) {
            // Item is on ground on water
            extinguish = true;
        }
    }

    if( extinguish ) {
        it.deactivate( p, false );
        if( p ) {
            p->add_msg_if_player( m_bad, "%s", deactivation_msg );
        }
        return 0;

    } else if( p && one_in( noise_chance ) ) {
        p->add_msg_if_player( "%s", noise_message );
    }

    return 1;
}

void manualnoise_actor::load( const JsonObject &obj, const std::string & )
{
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

std::optional<int> manualnoise_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return manualnoise_actor::use( p, it, &get_map(), pos );
}

std::optional<int> manualnoise_actor::use( Character *p, item &, map *,
        const tripoint_bub_ms & ) const
{
    map &bubble_map = reality_bubble();

    // Uses the moves specified by iuse_actor's definition
    p->mod_moves( -moves );
    if( noise > 0 && bubble_map.inbounds( p->pos_abs() ) ) {
        sounds::sound( bubble_map.get_bub( p->pos_abs( ) ), noise, sounds::sound_t::activity,
                       noise_message.empty() ? _( "Hsss" ) : noise_message.translated(), true, noise_id, noise_variant );
    }
    p->add_msg_if_player( "%s", use_message );
    return 1;
}

ret_val<void> manualnoise_actor::can_use( const Character &p, const item &it,
        const tripoint_bub_ms &pos ) const
{
    return manualnoise_actor::can_use( p, it, &get_map(), pos );
}

ret_val<void> manualnoise_actor::can_use( const Character &p, const item &it,
        map *, const tripoint_bub_ms & ) const
{
    if( !it.ammo_sufficient( &p ) ) {
        return ret_val<void>::make_failure( _( "This tool doesn't have enough charges." ) );
    }

    return ret_val<void>::make_success();
}

void play_instrument_iuse::load( const JsonObject &, const std::string & )
{
}

std::unique_ptr<iuse_actor> play_instrument_iuse::clone() const
{
    return std::make_unique<play_instrument_iuse>( *this );
}

std::optional<int> play_instrument_iuse::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return play_instrument_iuse::use( p, it, &get_map(), pos );
}

std::optional<int> play_instrument_iuse::use( Character *p, item &it,
        map *, const tripoint_bub_ms & ) const
{
    if( it.active ) {
        it.active = false;
        p->remove_effect( effect_playing_instrument );
        p->add_msg_player_or_npc( _( "You stop playing your %s." ),
                                  _( "<npcname> stops playing their %s." ),
                                  it.display_name() );
    } else {
        p->add_msg_player_or_npc( m_good,
                                  _( "You start playing your %s." ),
                                  _( "<npcname> starts playing their %s." ),
                                  it.display_name() );
        it.active = true;
        p->add_effect( effect_playing_instrument, 1_turns, false, 1 );
    }
    return std::nullopt;
}

ret_val<void> play_instrument_iuse::can_use( const Character &p, const item &it,
        const tripoint_bub_ms &pos ) const
{
    return play_instrument_iuse::can_use( p, it, &get_map(), pos );
}

ret_val<void> play_instrument_iuse::can_use( const Character &p, const item &it,
        map *, const tripoint_bub_ms & ) const
{
    // TODO (maybe): Mouth encumbrance? Smoke? Lack of arms? Hand encumbrance?
    if( p.is_underwater() ) {
        return ret_val<void>::make_failure( _( "You can't do that while underwater." ) );
    }
    if( p.is_mounted() ) {
        return ret_val<void>::make_failure( _( "You can't do that while mounted." ) );
    }
    if( !p.is_worn( it ) && !p.is_wielding( it ) ) {
        return ret_val<void>::make_failure( _( "You need to hold or wear %s to play it." ),
                                            it.type_name() );
    }
    // No one-man band for now
    // Remove/rework this check after we will be able to distinguish between wind, string, and percussion instruments
    // TODO: allow playing several string/percussion instruments if you have additional arms
    if( !it.active && p.has_effect( effect_playing_instrument ) ) {
        return ret_val<void>::make_failure( _( "You can't play multiple musical instruments at once." ) );
    }

    return ret_val<void>::make_success();
}

std::unique_ptr<iuse_actor> musical_instrument_actor::clone() const
{
    return std::make_unique<musical_instrument_actor>( *this );
}

void musical_instrument_actor::load( const JsonObject &obj, const std::string & )
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

std::optional<int> musical_instrument_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return musical_instrument_actor::use( p, it, &get_map(), pos );
}

std::optional<int> musical_instrument_actor::use( Character *p, item &it,
        map *here, const tripoint_bub_ms & ) const
{
    map &bubble_map = reality_bubble();

    if( here != &bubble_map ) { // Or change 'sound', 'can_hear', and 'play' below
        debugmsg( "musical instrument used outside of the reality bubble" );
        return std::nullopt;
    }

    if( !p ) {
        it.active = false;
        return std::nullopt;
    }

    if( !p->is_npc() && music::is_active_music_id( music::music_id::instrument ) ) {
        music::deactivate_music_id( music::music_id::instrument );
        // Because musical instrument creates musical sound too
        if( music::is_active_music_id( music::music_id::sound ) ) {
            music::deactivate_music_id( music::music_id::sound );
        }
    }

    if( p->is_mounted() ) {
        p->add_msg_player_or_npc( m_bad, _( "You can't play music while mounted." ),
                                  _( "<npcname> can't play music while mounted." ) );
        it.active = false;
        return std::nullopt;
    }
    if( p->is_underwater() ) {
        p->add_msg_player_or_npc( m_bad,
                                  _( "You can't play music underwater." ),
                                  _( "<npcname> can't play music underwater." ) );
        it.active = false;
        return std::nullopt;
    }

    // Stop playing a wind instrument when winded or even eventually become winded while playing it?
    // It's impossible to distinguish instruments for now anyways.
    if( p->has_effect( effect_sleep ) || p->has_effect( effect_stunned ) ||
        p->has_effect( effect_asthma ) ) {
        p->add_msg_player_or_npc( m_bad,
                                  _( "You stop playing your %s." ),
                                  _( "<npcname> stops playing their %s." ),
                                  it.display_name() );
        it.active = false;
        return std::nullopt;
    }

    // Check for worn or wielded - no "floating"/bionic instruments for now
    // TODO: Distinguish instruments played with hands and with mouth, consider encumbrance
    const int inv_pos = p->get_item_position( &it );
    if( inv_pos >= 0 || inv_pos == INT_MIN ) {
        p->add_msg_player_or_npc( m_bad,
                                  _( "You need to hold or wear %s to play it." ),
                                  _( "<npcname> needs to hold or wear %s to play it." ),
                                  it.display_name() );
        it.active = false;
        return std::nullopt;
    }

    // At speed this low you can't coordinate your actions well enough to play the instrument
    if( p->get_speed() <= 25 + speed_penalty ) {
        p->add_msg_player_or_npc( m_bad,
                                  _( "You feel too weak to play your %s." ),
                                  _( "<npcname> feels too weak to play their %s." ),
                                  it.display_name() );
        it.active = false;
        return std::nullopt;
    }

    // We can play the music now
    if( !p->is_npc() ) {
        music::activate_music_id( music::music_id::instrument );
    }

    if( p->get_effect_int( effect_playing_instrument ) <= speed_penalty ) {
        // Only re-apply the effect if it wouldn't lower the intensity
        p->add_effect( effect_playing_instrument, 2_turns, false, speed_penalty );
    }

    std::string desc = "music";
    /** @EFFECT_PER increases morale bonus when playing an instrument */
    const int morale_effect = fun + fun_bonus * p->per_cur;
    if( morale_effect >= 0 && calendar::once_every( description_frequency ) ) {
        if( !player_descriptions.empty() && p->is_avatar() ) {
            desc = random_entry( player_descriptions ).translated();
        } else if( !npc_descriptions.empty() && p->is_npc() ) {
            //~ %1$s: npc name, %2$s: npc action description
            desc = string_format( pgettext( "play music", "%1$s %2$s" ), p->disp_name( false ),
                                  random_entry( npc_descriptions ) );
        }
    } else if( morale_effect < 0 && calendar::once_every( 1_minutes ) ) {
        // No musical skills = possible morale penalty
        if( p->is_avatar() ) {
            desc = _( "You produce an annoying sound." );
        } else {
            desc = string_format( _( "%s produces an annoying sound." ), p->disp_name( false ) );
        }
    }

    if( morale_effect >= 0 ) {
        sounds::sound( p->pos_bub( *here ), volume, sounds::sound_t::music, desc, true,
                       "musical_instrument",
                       it.typeId().str() );
    } else {
        sounds::sound( p->pos_bub( *here ), volume, sounds::sound_t::music, desc, true,
                       "musical_instrument_bad",
                       it.typeId().str() );
    }

    if( !p->has_effect( effect_music ) && p->can_hear( p->pos_bub( *here ), volume ) ) {
        // Sound code doesn't describe noises at the player position
        if( desc != "music" ) {
            p->add_msg_if_player( m_info, desc );
        }
    }

    // We already played the sounds, just handle applying effects now
    iuse::play_music( p, p->pos_bub( *here ), volume, morale_effect, /*play_sounds=*/false );

    return 0;
}

ret_val<void> musical_instrument_actor::can_use( const Character &p, const item &it,
        const tripoint_bub_ms &pos ) const
{
    return musical_instrument_actor::can_use( p, it, &get_map(), pos );
}

ret_val<void> musical_instrument_actor::can_use( const Character &p, const item &,
        map *, const tripoint_bub_ms & ) const
{
    // TODO: (maybe): Mouth encumbrance? Smoke? Lack of arms? Hand encumbrance?
    if( p.is_underwater() ) {
        return ret_val<void>::make_failure( _( "You can't do that while underwater." ) );
    }
    if( p.is_mounted() ) {
        return ret_val<void>::make_failure( _( "You can't do that while mounted." ) );
    }

    return ret_val<void>::make_success();
}

std::unique_ptr<iuse_actor> learn_spell_actor::clone() const
{
    return std::make_unique<learn_spell_actor>( *this );
}

void learn_spell_actor::load( const JsonObject &obj, const std::string & )
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
    for( const std::string &sp_id_str : spells ) {
        const spell_id sp_id( sp_id_str );
        std::string spell_text = sp_id->name.translated();
        if( sp_id->spell_class.is_valid() ) {
            spell_text = string_format( pgettext( "spell name, spell class", "%s, %s" ),
                                        spell_text, sp_id->spell_class->name() );
        }
        if( pc.has_trait( trait_ILLITERATE ) ) {
            dump.emplace_back( "SPELL", spell_text );
        } else {
            if( pc.magic->knows_spell( sp_id ) ) {
                const spell sp = pc.magic->get_spell( sp_id );
                spell_text += ": " + string_format( _( "Level %u" ), sp.get_level() );
                if( sp.is_max_level( pc ) ) {
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

std::optional<int> learn_spell_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return learn_spell_actor::use( p, it, &get_map(), pos );
}

std::optional<int> learn_spell_actor::use( Character *p, item &, map *,
        const tripoint_bub_ms & ) const
{
    //TODO: combine/replace the checks below with "checks for conditions" from Character::check_read_condition

    if( p->fine_detail_vision_mod() > 4 ) {
        p->add_msg_if_player( m_bad, _( "It's too dark to read." ) );
        return std::nullopt;
    }
    if( p->has_trait( trait_ILLITERATE ) ) {
        p->add_msg_if_player( m_bad, _( "You can't read." ) );
        return std::nullopt;
    }
    if( !p->has_morale_to_read() ) {
        p->add_msg_if_player( m_bad, _( "What's the point of studying?  (Your morale is too low!)" ) );
        return std::nullopt;
    }
    std::vector<uilist_entry> uilist_initializer;
    uilist spellbook_uilist;
    spellbook_callback sp_cb;
    bool know_it_all = true;
    for( const std::string &sp_id_str : spells ) {
        const spell_id sp_id( sp_id_str );
        sp_cb.add_spell( sp_id );
        uilist_entry entry( sp_id.obj().name.translated() );
        if( p->magic->knows_spell( sp_id ) ) {
            const spell sp = p->magic->get_spell( sp_id );
            entry.ctxt = string_format( _( "Level %u" ), sp.get_level() );
            if( sp.is_max_level( *p ) || ( sp.max_book_level().has_value() &&
                                           sp.get_level() >= sp.max_book_level() ) ) {
                entry.ctxt += _( " (Max)" );
                entry.enabled = false;
            } else {
                know_it_all = false;
            }
        } else {
            if( p->magic->can_learn_spell( *p, sp_id ) ) {
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
        return std::nullopt;
    }

    spellbook_uilist.entries = uilist_initializer;
    spellbook_uilist.desired_bounds = { -1.0, -1.0, 80 * ImGui::CalcTextSize( "X" ).x, 24 * ImGui::GetTextLineHeightWithSpacing() };
    spellbook_uilist.callback = &sp_cb;
    spellbook_uilist.title = _( "Study a spell:" );
    spellbook_uilist.query();
    const int action = spellbook_uilist.ret;
    if( action < 0 ) {
        return std::nullopt;
    }
    const bool knows_spell = p->magic->knows_spell( spells[action] );
    player_activity study_spell( ACT_STUDY_SPELL,
                                 p->magic->time_to_learn_spell( *p, spells[action] ) );
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
            return std::nullopt;
        }
        study_spell.moves_total = study_time;
        spell &studying = p->magic->get_spell( spell_id( spells[action] ) );
        if( studying.get_difficulty( *p ) < static_cast<int>( p->get_skill_level( studying.skill() ) ) ) {
            p->handle_skill_warning( studying.skill(),
                                     true ); // show the skill warning on start reading, since we don't show it during
        }
    }
    study_spell.moves_left = study_spell.moves_total;
    if( study_spell.moves_total == 10100 ) {
        study_spell.str_values[0] = "gain_level";
        study_spell.values[0] = 0; // reserved for xp
        study_spell.values[1] = p->magic->get_spell( spell_id( spells[action] ) ).get_level() + 1;
    }
    study_spell.name = spells[action];
    p->assign_activity( study_spell );
    return 0;
}

std::unique_ptr<iuse_actor> cast_spell_actor::clone() const
{
    return std::make_unique<cast_spell_actor>( *this );
}

void cast_spell_actor::load( const JsonObject &obj, const std::string & )
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
    return mundane ? _( "Activate" ) : _( "Cast spell" );
}

std::optional<int> cast_spell_actor::use( Character *p, item &it, const tripoint_bub_ms &pos ) const
{
    return cast_spell_actor::use( p, it, &get_map(), pos );
}

std::optional<int> cast_spell_actor::use( Character *p, item &it, map * /*here*/,
        const tripoint_bub_ms &pos ) const
{
    if( need_worn && !p->is_worn( it ) ) {
        p->add_msg_if_player( m_info, _( "You need to wear the %1$s before activating it." ), it.tname() );
        return std::nullopt;
    }
    if( need_wielding && !p->is_wielding( it ) ) {
        p->add_msg_if_player( m_info, _( "You need to wield the %1$s before activating it." ), it.tname() );
        return std::nullopt;
    }

    spell casting = spell( spell_id( item_spell ) );

    // Spell is being cast from a non-held item
    if( p == nullptr ) {
        // TODO: Pass map when cast_all_effects is map aware.
        casting.cast_all_effects( pos );
        return 0;
    }

    player_activity cast_spell( ACT_SPELLCASTING, casting.casting_time( *p ) );
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
    } else {
        // [2]
        cast_spell.values.emplace_back( 0 );
    }
    p->assign_activity( cast_spell );
    p->activity.targets.emplace_back( *p, &it );
    // Actual handling of charges_to_use is in activity_handlers::spellcasting_finish
    return 0;
}

std::unique_ptr<iuse_actor> holster_actor::clone() const
{
    return std::make_unique<holster_actor>( *this );
}

void holster_actor::load( const JsonObject &obj, const std::string & )
{
    obj.read( "holster_prompt", holster_prompt );
    obj.read( "holster_msg", holster_msg );
}

bool holster_actor::can_holster( const item &holster, const item &obj ) const
{
    return obj.active ? false : holster.can_contain( obj ).success();
}

bool holster_actor::store( Character &you, item &holster, item &obj ) const
{
    if( obj.is_null() || holster.is_null() ) {
        debugmsg( "Null item was passed to holster_actor" );
        return false;
    }

    const ret_val<void> contain = holster.can_contain( obj );
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
                               pocket_type::CONTAINER, true );
    return true;
}

template<typename T>
static item_location form_loc_recursive( T &loc, item &it )
{
    item *parent = loc.find_parent( it );
    if( parent != nullptr ) {
        return item_location( form_loc_recursive( loc, *parent ), &it );
    }

    return item_location( loc, &it );
}

static item_location form_loc( Character &you, map *here, const tripoint_bub_ms &p, item &it )
{
    if( you.has_item( it ) ) {
        return form_loc_recursive( you, it );
    }
    map_cursor mc( here, p );
    if( mc.has_item( it ) ) {
        return form_loc_recursive( mc, it );
    }
    const optional_vpart_position vp = here->veh_at( p );
    if( vp ) {
        vehicle_cursor vc( vp->vehicle(), vp->part_index() );
        if( vc.has_item( it ) ) {
            return form_loc_recursive( vc, it );
        }
    }

    debugmsg( "Couldn't find item %s to form item_location, forming dummy location to ensure minimum functionality",
              it.display_name() );
    return item_location( you, &it );
}

std::optional<int> holster_actor::use( Character *you, item &it, const tripoint_bub_ms &p ) const
{
    return holster_actor::use( you, it, &get_map(), p );
}

std::optional<int> holster_actor::use( Character *you, item &it, map *here,
                                       const tripoint_bub_ms &p ) const
{
    if( you->is_wielding( it ) ) {
        you->add_msg_if_player( _( "You need to unwield your %s before using it." ), it.tname() );
        return std::nullopt;
    }

    int pos = 0;
    std::vector<std::string> opts;

    std::string prompt = holster_prompt.empty() ? _( "Holster item" ) : holster_prompt.translated();
    opts.push_back( prompt );
    pos = -1;
    std::list<item *> all_items = it.all_items_top(
                                      pocket_type::CONTAINER );
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
        if( !all_items.empty() ) {
            internal_item = all_items.front();
        }
    }

    if( pos < -1 ) {
        you->add_msg_if_player( _( "Never mind." ) );
        return std::nullopt;
    }

    if( pos >= 0 ) {
        item_location weapon =  you->get_wielded_item();
        if( weapon && weapon.get_item()->has_flag( flag_NO_UNWIELD ) ) {
            std::optional<bionic *> bio_opt = you->find_bionic_by_uid( you->get_weapon_bionic_uid() );
            if( !bio_opt || !you->deactivate_bionic( **bio_opt ) ) {
                you->add_msg_if_player( m_bad, _( "You can't unwield your %s." ), weapon.get_item()->tname() );
                return std::nullopt;
            }
        }
        // worn holsters ignore penalty effects (e.g. GRABBED) when determining number of moves to consume
        if( you->is_worn( it ) ) {
            you->wield_contents( it, internal_item, false, it.obtain_cost( *internal_item ) );
        } else {
            you->wield_contents( it, internal_item );
        }

    } else {
        if( you->as_avatar() == nullptr ) {
            return std::nullopt;
        }

        // iuse_actor really needs to work with item_location
        item_location item_loc = form_loc( *you, here, p, it );
        game_menus::inv::insert_items( item_loc );
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

void ammobelt_actor::load( const JsonObject &obj, const std::string & )
{
    belt = itype_id( obj.get_string( "belt" ) );
}

void ammobelt_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    dump.emplace_back( "AMMO", string_format( _( "Can be used to assemble: %s" ),
                       item::nname( belt ) ) );
}

std::optional<int> ammobelt_actor::use( Character *p, item &it, const tripoint_bub_ms &pos ) const
{
    return ammobelt_actor::use( p, it, &get_map(), pos );
}

std::optional<int> ammobelt_actor::use( Character *p, item &, map *, const tripoint_bub_ms & ) const
{
    item mag( belt );
    mag.ammo_unset();

    if( p->rate_action_reload( mag ) != hint_rating::good ) {
        p->add_msg_if_player( _( "Insufficient ammunition to assemble %s" ), mag.tname() );
        return std::nullopt;
    }

    item_location loc = p->i_add( mag );
    item::reload_option opt = p->select_ammo( loc, true );
    if( opt ) {
        p->assign_activity( reload_activity_actor( std::move( opt ) ) );
    } else {
        loc.remove_item();
    }

    return 0;
}

void repair_item_actor::load( const JsonObject &obj, const std::string & )
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
    if( p.cant_do_underwater( print_msg ) ) {
        return false;
    }
    if( p.cant_do_mounted( print_msg ) ) {
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

static item_location get_item_location( Character &p, item &it, map *here,
                                        const tripoint_bub_ms &pos )
{
    // Item on a character
    if( p.has_item( it ) ) {
        return item_location( p, &it );
    }

    // Item in a vehicle
    if( const optional_vpart_position &vp = here->veh_at( pos ) ) {
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
    return item_location( map_cursor( here, pos ), &it );
}

std::optional<int> repair_item_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return repair_item_actor::use( p, it, &get_map(), pos );
}

std::optional<int> repair_item_actor::use( Character *p, item &it,
        map *here, const tripoint_bub_ms &pos ) const
{
    if( !can_use_tool( *p, it, true ) ) {
        return std::nullopt;
    }

    p->assign_activity( ACT_REPAIR_ITEM, 0, p->get_item_position( &it ), INT_MIN );
    // We also need to store the repair actor subtype in the activity
    p->activity.str_values.push_back( type );
    // storing of item_location to support repairs by tools on the ground
    p->activity.targets.emplace_back( get_item_location( *p, it, here, pos ) );
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
        if( fix.can_repair_with( mat ) ) {
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
        bool print_msg, bool just_check, bool check_consumed_available ) const
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
                                            std::ceil( fix.base_volume() * cost_scaling / 250_ml ) :
                                            roll_remainder( fix.base_volume() * cost_scaling / 250_ml ) );

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
    for( const itype_id &component_id : valid_entries ) {
        if( item::count_by_charges( component_id ) ) {
            if( crafting_inv.has_charges( component_id, items_needed ) ) {
                comps.emplace_back( component_id, items_needed );
            }
        } else if( crafting_inv.has_amount( component_id, items_needed, false, filter ) ) {
            comps.emplace_back( component_id, items_needed );
        }
    }

    if( check_consumed_available && comps.empty() ) {
        if( print_msg ) {
            for( const itype_id &mat_comp : valid_entries ) {
                pl.add_msg_if_player( m_info,
                                      _( "You don't have enough clean %s to do that.  Have: %d, need: %d" ),
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

// Find the difficulty of the recipe for the item type.
// Base difficulty is the repair difficulty of the hardest thing to repair it is made of.
// if the training variable is true, then we're just repairing the easiest part of the thing.
// so instead take the easiest thing to repair it is made of.
static std::pair<int, bool> find_repair_difficulty( const itype &it )
{
    int difficulty = -1;
    bool difficulty_defined = false;

    if( !it.materials.empty() ) {
        for( const auto &mats : it.materials ) {
            if( difficulty < mats.first->repair_difficulty() ) {
                difficulty = mats.first->repair_difficulty();
                difficulty_defined = true;
            }
        }
    }

    return { difficulty, difficulty_defined };
}

// Returns the level of the most difficult material to repair in the item
// Or if it has a repairs_like, the lowest level recipe that results in that.
// If none exist the difficulty is 10
int repair_item_actor::repair_recipe_difficulty( const item &fix ) const
{
    std::pair<int, bool> ret;
    ret = find_repair_difficulty( *fix.type );
    int diff = ret.first;
    bool defined = ret.second;

    // See if there's a repairs_like that has a defined difficulty
    if( !defined && !fix.type->repairs_like.is_empty() ) {
        ret = find_repair_difficulty( fix.type->repairs_like.obj() );
        if( ret.second ) {
            diff = ret.first;
            defined = true;
        }
    }

    // If we still don't find a difficulty, difficulty is 10
    if( !defined ) {
        diff = 10;
    }

    return diff;
}

bool repair_item_actor::can_repair_target( Character &pl, const item &fix, bool print_msg,
        bool check_consumed_available ) const
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

    if( !handle_components( pl, fix, print_msg, true, check_consumed_available ) ) {
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

    if( fix.damage() > fix.degradation() ) {
        return true;
    }

    if( print_msg ) {
        pl.add_msg_if_player( m_info, _( "You cannot improve your %s any more this way." ), fix.tname() );
    }
    return false;
}

std::pair<float, float> repair_item_actor::repair_chance(
    const Character &pl, const item &fix, repair_item_actor::repair_type action_type ) const
{
    /** @EFFECT_TAILOR randomly improves clothing repair efforts */
    /** @EFFECT_MECHANICS randomly improves metal repair efforts */
    const float skill = pl.get_skill_level( used_skill );
    const int material_difficulty = repair_recipe_difficulty( fix );
    int action_difficulty = 0;
    switch( action_type ) {
        case RT_NOTHING: /* fallthrough */
        case RT_DOWNSIZING: /* fallthrough */
        case RT_UPSIZING:
            break;
        case RT_REPAIR:
            action_difficulty = fix.damage_level();
            break;
        case RT_REFIT:
            // Let's make refitting as hard as recovering an almost-wrecked item
            action_difficulty = fix.max_damage() / itype::damage_scale;
            break;
        case RT_PRACTICE:
            // Skill gain scales with recipe difficulty, so practice difficulty should too
            action_difficulty = material_difficulty;
            break;
        default:
            // 5 is obsoleted reinforcing, remove after 0.H
            action_difficulty = 1000000; // ensure failure
            break;
    }

    const int difficulty = material_difficulty + action_difficulty;
    // Sample numbers - cotton and wool have diff 1, plastic diff 3 and kevlar diff 4:
    // This assumes that training is false, and under almost all circumstances that should be the case
    // Item     | Damage | Skill | Dex | Success | Failure
    // Hoodie   |   1    |   0   |  8  |  4.0%   |  1.4%
    // Hoodie   | Refit  |   0   |  8  |  2.0%   |  2.4%
    // Hoodie   | Refit  |   2   |  8  |  6.0%   |  0.4%
    // Hoodie   | Refit  |   2   |  12 |  6.0%   |  0.0%
    // Hoodie   | Refit  |   4   |  8  |  10.0%  |  0.0%
    // Socks    |   3    |   0   |  8  |  2.0%   |  2.4%
    // Boots    |   1    |   0   |  8  |  0.0%   |  3.4%
    // Raincoat | Refit  |   2   |  8  |  2.0%   |  2.4%
    // Raincoat | Refit  |   2   |  12 |  2.0%   |  1.6%
    // Ski mask | Refit  |   2   |  8  |  4.0%   |  1.4%
    // Raincoat | Refit  |   2   |  8  |  4.0%   |  1.4%
    // Turnout  | Refit  |   2   |  8  |  0.0%   |  4.4%
    // Turnout  | Refit  |   4   |  8  |  2.0%   |  2.4%

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
    if( fix.damage() > fix.degradation() ) {
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

    if( current_skill_level <= trains_skill_to ) {
        return RT_PRACTICE;
    }

    return RT_NOTHING;
}

static bool damage_item( Character &pl, item_location &fix )
{
    map &here = get_map();

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
                put_into_vehicle_or_drop( pl, item_drop_reason::tumbling, { *it }, &here, fix.pos_bub( here ) );
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
    if( !can_repair_target( pl, *fix, true, true ) ) {
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
    int practice_amount = repair_recipe_difficulty( *fix ) / 2 + 1;
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
            handle_components( pl, *fix, false, false, true );

            fix->mod_damage( -itype::damage_scale );

            const std::string resultdurability = fix->durability_indicator( true );
            if( fix->repairable_levels() ) {
                pl.add_msg_if_player( m_good, _( "You repair your %s!  ( %s-> %s)" ),
                                      fix->tname( 1, false ), startdurability, resultdurability );
            } else {
                pl.add_msg_if_player( m_good, _( "You repair your %s completely!  ( %s-> %s)" ),
                                      fix->tname( 1, false ), startdurability, resultdurability );
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
            handle_components( pl, *fix, false, false, true );
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
            handle_components( pl, *fix, false, false, true );
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
            handle_components( pl, *fix, false, false, true );
            return AS_SUCCESS;
        }
        return AS_RETRY;
    }

    pl.add_msg_if_player( m_info, _( "You cannot improve your %s any more this way." ), fix->tname() );
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
            "Obsolete",
            translate_marker( "Practicing" )
        }
    };

    return _( arr[rt] );
}

std::string repair_item_actor::get_name() const
{
    return _( "Repair" );
}

std::string repair_item_actor::get_description() const
{
    const std::string mats = enumerate_as_string( materials.begin(), materials.end(),
    []( const material_id & mid ) {
        return mid->name();
    } );
    return string_format( _( "Repair %s" ), mats );
}

void heal_actor::load( const JsonObject &obj, const std::string & )
{
    // Mandatory
    move_cost = obj.get_int( "move_cost" );

    // Optional
    limb_power = obj.get_float( "limb_power", 0 );
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

    const bool does_instant_healing = limb_power || head_power || torso_power;
    const bool heal_over_time = bandages_power;
    const bool stops_bleed = bleed;
    const bool has_any_disinfect = disinfectant_power || bite || infect;
    const bool has_scripted_effect = obj.has_array( "effects" );
    if( !does_instant_healing && !heal_over_time && !stops_bleed
        && !has_any_disinfect && !has_scripted_effect ) {
        obj.throw_error( _( "Heal actor is missing any valid healing effect" ) );
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

static Character &get_patient( Character &healer, map *here, const tripoint_bub_ms &pos )
{
    if( healer.pos_bub( *here ) == pos ) {
        return healer;
    }

    Character *const person = get_creature_tracker().creature_at<Character>( here->get_abs( pos ) );
    if( !person ) {
        // Default to heal self on failure not to break old functionality
        add_msg_debug( debugmode::DF_IUSE, "No heal target at position %d,%d,%d", pos.x(), pos.y(),
                       pos.z() );
        return healer;
    }

    return *person;
}

std::optional<int> heal_actor::use( Character *p, item &it, const tripoint_bub_ms &pos ) const
{
    return heal_actor::use( p, it, &get_map(), pos );
}

std::optional<int> heal_actor::use( Character *p, item &it, map *here,
                                    const tripoint_bub_ms &pos ) const
{
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    if( it.is_filthy() ) {
        p->add_msg_if_player( m_info, _( "You can't use filthy items for healing." ) );
        return std::nullopt;
    }

    Character &patient = get_patient( *p, here, pos );
    const bodypart_str_id hpp = use_healing_item( *p, patient, it, false ).id();
    if( hpp == bodypart_str_id::NULL_ID() ) {
        return std::nullopt;
    }

    // each tier of proficiency cuts required time by half
    int cost = move_cost;
    cost = p->has_proficiency( proficiency_prof_wound_care_expert ) ? cost / 2 : cost;
    cost = p->has_proficiency( proficiency_prof_wound_care ) ? cost / 2 : cost;

    p->assign_activity( firstaid_activity_actor( cost, it.tname(), patient.getID() ) );

    // Player: Only time this item_location gets used in firstaid::finish() is when activating the item's
    // container from the inventory window, so an item_on_person impl is all that is needed.
    // Otherwise the proper item_location provided by menu selection supercedes it in consume::finish().
    // NPC: Will only use its inventory for first aid items.
    p->activity.targets.emplace_back( *p, &it );
    p->activity.str_values.emplace_back( hpp.c_str() );
    p->set_moves( 0 );
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
        return round( heal_base + bonus_mult * healer.get_skill_level( skill_firstaid ) );
    }

    return heal_base;
}

int heal_actor::get_bandaged_level( const Character &healer ) const
{
    if( bandages_power > 0 ) {
        float prof_bonus = healer.get_skill_level( skill_firstaid );
        prof_bonus = healer.has_proficiency( proficiency_prof_wound_care ) ?
                     prof_bonus + 1 : prof_bonus;
        prof_bonus = healer.has_proficiency( proficiency_prof_wound_care_expert ) ?
                     prof_bonus + 2 : prof_bonus;
        /** @EFFECT_FIRSTAID increases healing item effects */
        float total_bonus = bandages_power + bandages_scaling * prof_bonus;
        total_bonus = healer.enchantment_cache->modify_value( enchant_vals::mod::BANDAGE_BONUS,
                      total_bonus );
        return round( total_bonus );
    }

    return bandages_power;
}

int heal_actor::get_disinfected_level( const Character &healer ) const
{
    if( disinfectant_power > 0 ) {
        /** @EFFECT_FIRSTAID increases healing item effects */
        float prof_bonus = healer.get_skill_level( skill_firstaid );
        prof_bonus = healer.has_proficiency( proficiency_prof_wound_care ) ?
                     prof_bonus + 1 : prof_bonus;
        prof_bonus = healer.has_proficiency( proficiency_prof_wound_care_expert ) ?
                     prof_bonus + 2 : prof_bonus;
        float total_bonus = disinfectant_power + disinfectant_scaling * prof_bonus;
        total_bonus = healer.enchantment_cache->modify_value( enchant_vals::mod::DISINFECTANT_BONUS,
                      total_bonus );
        return round( total_bonus );
    }

    return disinfectant_power;
}

int heal_actor::get_stopbleed_level( const Character &healer ) const
{
    if( bleed > 0 ) {
        /** @EFFECT_FIRSTAID increases healing item effects */
        float prof_bonus = healer.get_skill_level( skill_firstaid ) / 2;
        prof_bonus = healer.has_proficiency( proficiency_prof_wound_care ) ?
                     prof_bonus + 1 : prof_bonus;
        prof_bonus = healer.has_proficiency( proficiency_prof_wound_care_expert ) ?
                     prof_bonus + 2 : prof_bonus;
        float total_bonus = bleed * prof_bonus;
        total_bonus = healer.enchantment_cache->modify_value( enchant_vals::mod::BLEED_STOP_BONUS,
                      total_bonus );
        return round( total_bonus );
    }

    return bleed;
}

int heal_actor::finish_using( Character &healer, Character &patient, item &it,
                              bodypart_id healed ) const
{
    const map &here = get_map();

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
                       player_character.sees( here, healer ) || player_character.sees( here,  patient );
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
                patient.remove_effect( effect_bleed, healed );
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
            it.convert( used_up_item_id, &healer );
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
        int bandages_intensity = std::max( 1, get_bandaged_level( healer ) );
        patient.add_effect( effect_bandaged, 1_turns, healed );
        effect &e = patient.get_effect( effect_bandaged, healed );
        e.set_duration( e.get_int_dur_factor() * bandages_intensity );
        patient.set_part_damage_bandaged( healed,
                                          patient.get_part_hp_max( healed ) - patient.get_part_hp_cur( healed ) );
        practice_amount += 2 * bandages_intensity;
    }
    if( disinfectant_power > 0 ) {
        int disinfectant_intensity = std::max( 1, get_disinfected_level( healer ) );
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
    return 1;
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
        if( healed_part == bodypart_str_id::NULL_ID() ) {
            return healed_part;
        }

        if( healed_part->has_flag( json_flag_BIONIC_LIMB ) ) {
            add_msg( m_info, _( "You can't use first aid on a bionic limb." ) );
            continue;
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
    bodypart_id healed = bodypart_str_id::NULL_ID();
    const int head_bonus = get_heal_value( healer, bodypart_id( "head" ) );
    const int limb_power = get_heal_value( healer, bodypart_id( "arm_l" ) );
    const int torso_bonus = get_heal_value( healer, bodypart_id( "torso" ) );

    if( !patient.can_use_heal_item( it ) ) {
        patient.add_msg_player_or_npc( m_bad,
                                       _( "Your biology is not compatible with that item." ),
                                       _( "<npcname>'s biology is not compatible with that item." ) );
        return bodypart_str_id::NULL_ID().id(); // canceled
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
            if( healed == bodypart_str_id::NULL_ID() ) {
                add_msg( m_info, _( "Never mind." ) );
                return bodypart_str_id::NULL_ID().id(); // canceled
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

void place_trap_actor::load( const JsonObject &obj, const std::string & )
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

static bool is_solid_neighbor( const tripoint_bub_ms &pos, const point_rel_ms &offset )
{
    map &here = get_map();
    const tripoint_bub_ms a = pos + offset;
    const tripoint_bub_ms b = pos - offset;
    return here.move_cost( a ) != 2 && here.move_cost( b ) != 2;
}

static bool has_neighbor( const tripoint_bub_ms &pos, const ter_id &terrain_id )
{
    map &here = get_map();
    for( const tripoint_bub_ms &t : here.points_in_radius( pos, 1, 0 ) ) {
        if( here.ter( t ) == terrain_id ) {
            return true;
        }
    }
    return false;
}

bool place_trap_actor::is_allowed( Character &p, const tripoint_bub_ms &pos,
                                   const std::string &name ) const
{
    if( !allow_under_player && pos == p.pos_bub() ) {
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
        if( !is_solid_neighbor( pos, point_rel_ms::east ) &&
            !is_solid_neighbor( pos, point_rel_ms::south ) &&
            !is_solid_neighbor( pos, point_rel_ms::south_east ) &&
            !is_solid_neighbor( pos, point_rel_ms::north_east ) ) {
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
            p.add_msg_if_player( m_info,
                                 existing_trap.is_benign()
                                 ? _( "You can't place a %s there.  It contains a deployed object already." )
                                 : _( "You can't place a %s there.  It contains a trap already." ),
                                 name );
        } else {
            p.add_msg_if_player( m_bad, _( "You trigger a %s!" ), existing_trap.name() );
            existing_trap.trigger( pos, p );
        }
        return false;
    }
    return true;
}

static void place_and_add_as_known( Character &p, const tripoint_bub_ms &pos,
                                    const trap_str_id &id )
{
    map &here = get_map();
    here.trap_set( pos, id );
    const trap &tr = here.tr_at( pos );
    if( !tr.can_see( pos, p ) ) {
        p.add_known_trap( pos, tr );
    }
}

std::optional<int> place_trap_actor::use( Character *p, item &it, const tripoint_bub_ms &pos ) const
{
    return place_trap_actor::use( p, it, &get_map(), pos );
}

std::optional<int> place_trap_actor::use( Character *p, item &it, map *here,
        const tripoint_bub_ms & ) const
{
    map &bubble_map = reality_bubble();

    if( here != &bubble_map ) { // Or make 'choose_adjacent' and 'is_allowed' map aware.
        debugmsg( "place_trap_actor::use cannot act on non reality bubble map." );
        return std::nullopt;
    }

    const tripoint_bub_ms p_pos = p->pos_bub( *here );

    const bool could_bury = !bury_question.empty();
    if( !allow_underwater && p->cant_do_underwater() ) {
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    const std::optional<tripoint_bub_ms> pos_ = choose_adjacent( string_format(
                _( "Place %s where?" ),
                it.tname() ) );
    if( !pos_ ) {
        return std::nullopt;
    }
    tripoint_bub_ms pos = *pos_;

    if( !is_allowed( *p, pos, it.tname() ) ) {
        return std::nullopt;
    }

    int distance_to_trap_center = unburied_data.trap.obj().get_trap_radius() +
                                  outer_layer_trap.obj().get_trap_radius() + 1;
    if( unburied_data.trap.obj().get_trap_radius() > 0 ) {
        // Math correction for multi-tile traps
        pos.x() = ( pos.x() - p_pos.x() ) * distance_to_trap_center + p_pos.x();
        pos.y() = ( pos.y() - p_pos.y() ) * distance_to_trap_center + p_pos.y();
        for( const tripoint_bub_ms &t : here->points_in_radius( pos,
                outer_layer_trap.obj().get_trap_radius(),
                0 ) ) {
            if( !is_allowed( *p, t, it.tname() ) ) {
                p->add_msg_if_player( m_info,
                                      _( "That trap needs a space in %d tiles radius to be clear, centered %d tiles from you." ),
                                      outer_layer_trap.obj().get_trap_radius(), distance_to_trap_center );
                return std::nullopt;
            }
        }
    }

    const bool has_shovel = p->has_quality( qual_DIG, 3 );
    const bool is_diggable = here->has_flag( ter_furn_flag::TFLAG_DIGGABLE, pos );
    bool bury = false;
    if( could_bury && has_shovel && is_diggable ) {
        bury = query_yn( "%s", bury_question );
    }
    const place_trap_actor::data &data = bury ? buried_data : unburied_data;

    p->add_msg_if_player( m_info, data.done_message.translated(), here->tername( pos ) );
    p->practice( skill_traps, data.practice );
    p->practice_proficiency( proficiency_prof_traps,
                             time_duration::from_seconds( data.practice * 30 ) );
    p->practice_proficiency( proficiency_prof_trapsetting,
                             time_duration::from_seconds( data.practice * 30 ) );

    //Total time to set the trap will be determined by player's skills and proficiencies
    int move_cost_final = std::round( ( data.moves * std::min( 1,
                                        ( data.practice ^ 2 ) ) ) / ( p->get_skill_level( skill_traps ) <= 1 ? 1 : p->get_skill_level(
                                                skill_traps ) ) );
    if( !p->has_proficiency( proficiency_prof_trapsetting ) ) {
        move_cost_final = move_cost_final * 2;
    }
    if( !p->has_proficiency( proficiency_prof_traps ) ) {
        move_cost_final = move_cost_final * 4;
    }

    //This probably needs to be done via assign_activity
    p->mod_moves( -move_cost_final );

    place_and_add_as_known( *p, pos, data.trap );
    const trap &placed_trap = here->tr_at( pos );
    if( !placed_trap.is_null() ) {
        const_cast<trap &>( placed_trap ).set_trap_data( it.typeId() );
    }
    for( const tripoint_bub_ms &t : here->points_in_radius( pos, data.trap.obj().get_trap_radius(),
            0 ) ) {
        if( t != pos ) {
            place_and_add_as_known( *p, t, outer_layer_trap );
        }
    }
    return 1;
}

void emit_actor::load( const JsonObject &obj, const std::string & )
{
    assign( obj, "emits", emits );
    assign( obj, "scale_qty", scale_qty );
}

std::optional<int> emit_actor::use( Character *p, item &it, const tripoint_bub_ms &pos ) const
{
    return emit_actor::use( p, it, &get_map(), pos );
}

std::optional<int> emit_actor::use( Character *, item &it, map *here,
                                    const tripoint_bub_ms &pos ) const
{
    const float scaling = scale_qty ? it.charges : 1.0f;
    for( const auto &e : emits ) {
        here->emit_field( pos, e, scaling );
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

void saw_barrel_actor::load( const JsonObject &jo, const std::string & )
{
    assign( jo, "cost", cost );
}

//Todo: Make this consume charges if performed with a tool that uses charges.
std::optional<int> saw_barrel_actor::use( Character *p, item &it, const tripoint_bub_ms &pos ) const
{
    return saw_barrel_actor::use( p, it, &get_map(), pos );
}

std::optional<int> saw_barrel_actor::use( Character *p, item &it, map *,
        const tripoint_bub_ms & ) const
{
    if( !p ) {
        debugmsg( "%s called action saw_barrel that requires character but no character is present",
                  it.typeId().str() );
        return std::nullopt;
    }

    item_location loc = game_menus::inv::saw_barrel( *p, it );

    if( !loc ) {
        p->add_msg_if_player( _( "Never mind." ) );
        return std::nullopt;
    }

    item &obj = *loc.obtain( *p );
    p->add_msg_if_player( _( "You saw down the barrel of your %s." ), obj.tname() );
    obj.put_in( item( itype_barrel_small, calendar::turn ), pocket_type::MOD );

    return 0;
}

ret_val<void> saw_barrel_actor::can_use_on( const Character &, const item &,
        const item &target ) const
{
    if( !target.is_gun() ) {
        return ret_val<void>::make_failure( _( "It's not a gun." ) );
    }

    if( target.type->gun->barrel_volume <= 0_ml ) {
        return ret_val<void>::make_failure( _( "The barrel is too small." ) );
    }

    if( target.gunmod_find( itype_barrel_small ) ) {
        return ret_val<void>::make_failure( _( "The barrel is already sawn-off." ) );
    }

    const auto gunmods = target.gunmods();
    const bool modified_barrel = std::any_of( gunmods.begin(), gunmods.end(),
    []( const item * mod ) {
        return mod->type->gunmod->location == gunmod_location( "barrel" );
    } );

    if( modified_barrel ) {
        return ret_val<void>::make_failure( _( "Can't saw off modified barrels." ) );
    }

    return ret_val<void>::make_success();
}

std::unique_ptr<iuse_actor> saw_barrel_actor::clone() const
{
    return std::make_unique<saw_barrel_actor>( *this );
}

void saw_stock_actor::load( const JsonObject &jo, const std::string & )
{
    assign( jo, "cost", cost );
}

//Todo: Make this consume charges if performed with a tool that uses charges.
std::optional<int> saw_stock_actor::use( Character *p, item &it, const tripoint_bub_ms &pos ) const
{
    return saw_stock_actor::use( p, it, &get_map(), pos );
}

std::optional<int> saw_stock_actor::use( Character *p, item &it, map *,
        const tripoint_bub_ms & ) const
{
    if( !p ) {
        debugmsg( "%s called action saw_stock that requires character but no character is present",
                  it.typeId().str() );
        return std::nullopt;
    }

    item_location loc = game_menus::inv::saw_stock( *p, it );

    if( !loc ) {
        p->add_msg_if_player( _( "Never mind." ) );
        return std::nullopt;
    }

    item &obj = *loc.obtain( *p );
    p->add_msg_if_player( _( "You saw down the stock of your %s." ), obj.tname() );
    obj.put_in( item( itype_stock_none, calendar::turn ), pocket_type::MOD );

    return 0;
}

ret_val<void> saw_stock_actor::can_use_on( const Character &, const item &,
        const item &target ) const
{
    if( !target.is_gun() ) {
        return ret_val<void>::make_failure( _( "It's not a gun." ) );
    }

    if( target.gunmod_find( itype_stock_none ) ) {
        return ret_val<void>::make_failure( _( "The stock is already sawn-off." ) );
    }

    if( target.gun_type() == gun_type_type( "pistol" ) ) {
        return ret_val<void>::make_failure( _( "This gun doesn't have a stock." ) );
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
        return ret_val<void>::make_failure(
                   _( "Can't cut off modern composite stocks (must have an empty stock mount)." ) );
    }

    if( modified_stock ) {
        return ret_val<void>::make_failure( _( "Can't cut off modified stocks." ) );
    }

    if( accessorized_stock ) {
        return ret_val<void>::make_failure( _( "Can't cut off accessorized stocks." ) );
    }

    return ret_val<void>::make_success();
}

std::unique_ptr<iuse_actor> saw_stock_actor::clone() const
{
    return std::make_unique<saw_stock_actor>( *this );
}

void molle_attach_actor::load( const JsonObject &jo, const std::string & )
{
    assign( jo, "size", size );
    assign( jo, "moves", moves );
}

std::optional<int> molle_attach_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return molle_attach_actor::use( p, it, &get_map(), pos );
}

std::optional<int> molle_attach_actor::use( Character *p, item &it,
        map *, const tripoint_bub_ms & ) const
{
    if( !p ) {
        debugmsg( "%s called action molle_attach that requires character but no character is present",
                  it.typeId().str() );
        return std::nullopt;
    }

    item_location loc = game_menus::inv::molle_attach( *p, it );

    if( !loc ) {
        p->add_msg_if_player( _( "Never mind." ) );
        return std::nullopt;
    }

    item &obj = *loc.get_item();
    p->add_msg_if_player( _( "You attach %s to your MOLLE webbing." ), obj.tname() );

    it.get_contents().add_pocket( obj );

    // the item has been added to the vest it should no longer exist in the world
    loc.remove_item();

    return 0;
}

std::unique_ptr<iuse_actor> molle_attach_actor::clone() const
{
    return std::make_unique<molle_attach_actor>( *this );
}

std::optional<int> molle_detach_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return molle_detach_actor::use( p, it, &get_map(), pos );
}

std::optional<int> molle_detach_actor::use( Character *p, item &it,
        map *, const tripoint_bub_ms & ) const
{

    std::vector<const item *> items_attached = it.get_contents().get_added_pockets();
    uilist prompt;
    prompt.text = _( "Remove which accessory?" );

    for( size_t i = 0; i != items_attached.size(); ++i ) {
        prompt.addentry( i, true, -1, items_attached[i]->tname() );
    }

    prompt.query();

    if( prompt.ret >= 0 ) {
        p->i_add( it.get_contents().remove_pocket( prompt.ret ) );
        p->add_msg_if_player( _( "You remove the item from your %s." ), it.tname() );
        return 0;
    }

    p->add_msg_if_player( _( "Never mind." ) );
    return std::nullopt;
}

std::unique_ptr<iuse_actor> molle_detach_actor::clone() const
{
    return std::make_unique<molle_detach_actor>( *this );
}

void molle_detach_actor::load( const JsonObject &jo, const std::string & )
{
    assign( jo, "moves", moves );
}

std::optional<int> install_bionic_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return install_bionic_actor::use( p, it, &get_map(), pos );
}

std::optional<int> install_bionic_actor::use( Character *p, item &it,
        map *, const tripoint_bub_ms & ) const
{
    if( p->can_install_bionics( *it.type, *p, false ) ) {
        if( !p->has_trait( trait_DEBUG_BIONICS ) && !p->has_flag( json_flag_MANUAL_CBM_INSTALLATION ) ) {
            p->consume_installation_requirement( it.type->bionic->id );
            p->consume_anesth_requirement( *it.type, *p );
        }
        if( p->install_bionics( *it.type, *p, false ) ) {
            return 1;
        }
    }
    return std::nullopt;
}

ret_val<void> install_bionic_actor::can_use( const Character &p, const item &it,
        const tripoint_bub_ms &pos ) const
{
    return install_bionic_actor::can_use( p, it, &get_map(), pos );
}

ret_val<void> install_bionic_actor::can_use( const Character &p, const item &it,
        map *, const tripoint_bub_ms & ) const
{
    if( !it.is_bionic() ) {
        return ret_val<void>::make_failure();
    }
    const bionic_id &bid = it.type->bionic->id;
    if( p.is_mounted() ) {
        return ret_val<void>::make_failure( _( "You can't install bionics while mounted." ) );
    }
    if( !p.has_trait( trait_DEBUG_BIONICS ) ) {
        if( bid->installation_requirement.is_empty() && !p.has_flag( json_flag_MANUAL_CBM_INSTALLATION ) ) {
            return ret_val<void>::make_failure( _( "You can't self-install this CBM." ) );
        } else  if( it.has_flag( flag_FILTHY ) ) {
            return ret_val<void>::make_failure( _( "You can't install a filthy CBM!" ) );
        } else if( it.has_flag( flag_NO_STERILE ) ) {
            return ret_val<void>::make_failure( _( "This CBM is not sterile, you can't install it." ) );
        } else if( it.has_fault( fault_bionic_salvaged ) ) {
            return ret_val<void>::make_failure(
                       _( "This CBM is already deployed.  You need to reset it to factory state." ) );
        } else if( units::energy( std::numeric_limits<int>::max(), units::energy::unit_type{} ) -
                   p.get_max_power_level() < bid->capacity ) {
            return ret_val<void>::make_failure( _( "Max power capacity already reached" ) );
        }
    }

    if( p.has_bionic( bid ) && !bid->dupes_allowed ) {
        return ret_val<void>::make_failure( _( "You have already installed this bionic." ) );
    } else if( bid->upgraded_bionic && !p.has_bionic( bid->upgraded_bionic ) ) {
        return ret_val<void>::make_failure( _( "There is nothing to upgrade." ) );
    } else {
        const bool downgrade =
            std::any_of( bid->available_upgrades.begin(), bid->available_upgrades.end(),
        [&p]( const bionic_id & b ) {
            return p.has_bionic( b );
        } );

        if( downgrade ) {
            return ret_val<void>::make_failure( _( "You have a superior version installed." ) );
        }
    }

    return ret_val<void>::make_success();
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

std::optional<int> detach_gunmods_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return detach_gunmods_actor::use( p, it, &get_map(), pos );
}

std::optional<int> detach_gunmods_actor::use( Character *p, item &it,
        map *, const tripoint_bub_ms & ) const
{
    auto filter_irremovable = []( std::vector<item *> &gunmods ) {
        gunmods.erase(
            std::remove_if(
        gunmods.begin(), gunmods.end(), []( const item * i ) {
            return i->is_irremovable();
        } ),
        gunmods.end() );
    };

    item gun_copy = item( it );
    std::vector<item *> mods = it.gunmods();
    std::vector<item *> mods_copy = gun_copy.gunmods();

    filter_irremovable( mods );
    filter_irremovable( mods_copy );

    item_location mod_loc = game_menus::inv::gunmod_to_remove( *p, it );

    if( !mod_loc ) {
        p->add_msg_if_player( _( "Never mind." ) );
        return std::nullopt;
    }

    // Find the index of the mod to be removed
    // used in identifying the mod in mods and mods_copy
    int mod_index = -1;
    for( size_t i = 0; i != mods.size(); ++i ) {
        if( mods[ i ] == mod_loc.get_item() ) {
            mod_index = i;
            break;
        }
    }

    if( mod_index >= 0 ) {
        gun_copy.remove_item( *mods_copy[mod_index] );

        if( p->meets_requirements( *mods[mod_index], gun_copy ) ||
            query_yn( _( "Are you sure?  You may be lacking the skills needed to reattach this modification." ) ) ) {

            if( game_menus::inv::compare_item_menu( it, gun_copy, _( "Remove modification?" ) ).show() ) {
                p->gunmod_remove( it, *mods[mod_index] );
                return 0;
            }
        }
    }

    p->add_msg_if_player( _( "Never mind." ) );
    return std::nullopt;
}

ret_val<void> detach_gunmods_actor::can_use( const Character &p, const item &it,
        const tripoint_bub_ms &pos ) const
{
    return detach_gunmods_actor::can_use( p, it, &get_map(), pos );
}

ret_val<void> detach_gunmods_actor::can_use( const Character &p, const item &it,
        map *, const tripoint_bub_ms & ) const
{
    const std::vector<const item *> mods = it.gunmods();

    if( mods.empty() ) {
        return ret_val<void>::make_failure( _( "Doesn't appear to be modded." ) );
    }

    const bool no_removables =
        std::all_of( mods.begin(), mods.end(),
    []( const item * mod ) {
        return mod->is_irremovable();
    } );

    if( no_removables ) {
        return ret_val<void>::make_failure( _( "None of the mods can be removed." ) );
    }

    if( p.is_worn(
            it ) ) { // Prevent removal of shoulder straps and thereby making the gun un-wearable again.
        return ret_val<void>::make_failure( _( "Has to be taken off first." ) );
    }

    return ret_val<void>::make_success();
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

std::optional<int> modify_gunmods_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return modify_gunmods_actor::use( p, it, &get_map(), pos );
}

std::optional<int> modify_gunmods_actor::use( Character *p, item &it,
        map */*here*/, const tripoint_bub_ms &pos ) const
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
        // TODO: make 'invoke_item' map aware.
        p->invoke_item( mods[prompt.ret], "transform", pos );
        it.on_contents_changed();
        return 0;
    }

    p->add_msg_if_player( _( "Never mind." ) );
    return std::nullopt;
}

ret_val<void> modify_gunmods_actor::can_use( const Character &p, const item &it,
        const tripoint_bub_ms &pos ) const
{
    return modify_gunmods_actor::can_use( p, it, &get_map(), pos );
}

ret_val<void> modify_gunmods_actor::can_use( const Character &p, const item &it,
        map *, const tripoint_bub_ms & ) const
{
    if( !p.is_wielding( it ) ) {
        return ret_val<void>::make_failure( _( "Need to be wielding." ) );
    }
    const std::vector<const item *> mods = it.gunmods();

    if( mods.empty() ) {
        return ret_val<void>::make_failure( _( "Doesn't appear to be modded." ) );
    }

    const bool modifiables = std::any_of( mods.begin(), mods.end(),
    []( const item * mod ) {
        return mod->is_transformable();
    } );

    if( !modifiables ) {
        return ret_val<void>::make_failure( _( "None of the mods can be modified." ) );
    }

    if( p.is_worn(
            it ) ) { // I don't know if modifying really needs this but its for future proofing.
        return ret_val<void>::make_failure( _( "Has to be taken off first." ) );
    }

    return ret_val<void>::make_success();
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

void link_up_actor::load( const JsonObject &jo, const std::string & )
{
    jo.read( "cable_length", cable_length );
    jo.read( "charge_rate", charge_rate );
    jo.read( "efficiency", efficiency );
    jo.read( "move_cost", move_cost );
    jo.read( "menu_text", menu_text );
    jo.read( "targets", targets );
    jo.read( "can_extend", can_extend );
}

std::unique_ptr<iuse_actor> link_up_actor::clone() const
{
    return std::make_unique<link_up_actor>( *this );
}

std::string link_up_actor::get_name() const
{
    if( !menu_text.empty() ) {
        return menu_text.translated();
    }
    return iuse_actor::get_name();
}

void link_up_actor::info( const item &it, std::vector<iteminfo> &dump ) const
{
    std::vector<std::string> targets_strings;
    bool appliance = false;
    if( targets.count( link_state::vehicle_port ) > 0 ) {
        targets_strings.emplace_back( _( "vehicle controls" ) );
        appliance = true;
    }
    if( targets.count( link_state::vehicle_battery ) > 0 ) {
        targets_strings.emplace_back( _( "vehicle battery" ) );
        appliance = true;
    }
    if( appliance ) {
        targets_strings.emplace_back( _( "appliance" ) );
    }
    if( targets.count( link_state::bio_cable ) > 0 ) {
        targets_strings.emplace_back( _( "bionic" ) );
    }
    if( targets.count( link_state::ups ) > 0 ) {
        targets_strings.emplace_back( _( "UPS" ) );
    }
    if( targets.count( link_state::solarpack ) > 0 ) {
        targets_strings.emplace_back( _( "solar pack" ) );
    }
    if( targets.count( link_state::vehicle_tow ) > 0 ) {
        dump.emplace_back( "TOOL", _( "<bold>Can tow a vehicle</bold>." ) );
    }
    if( !targets_strings.empty() ) {
        std::string targets_string = enumerate_as_string( targets_strings, enumeration_conjunction::or_ );
        dump.emplace_back( "TOOL",
                           string_format( _( "<bold>Can be plugged into</bold>: %s." ), targets_string ) );
    }

    const bool no_extensions = it.cables().empty();
    item dummy( it );
    dummy.update_link_traits();

    std::string length_all_info = string_format( _( "<bold>Cable length</bold>: %d" ),
                                  dummy.max_link_length() );
    std::string length_solo_info = no_extensions ? "" : string_format( _( " (%d without extensions)" ),
                                   cable_length != -1 ? cable_length : dummy.type->maximum_charges() );
    dump.emplace_back( "TOOL", length_all_info, length_solo_info );

    if( charge_rate != 0_W ) {
        //~ Power in Watts. %+4.1f is a 4 digit number with 1 decimal point (ex: 4737.3 W)
        dump.emplace_back( "TOOL", _( "<bold>Wattage</bold>: " ), string_format( _( "%+4.1f W" ),
                           units::to_milliwatt( charge_rate ) / 1000.f ) );
    }

    if( !can_extend.empty() ) {
        std::vector<std::string> cable_types;
        cable_types.reserve( can_extend.size() );
        for( const std::string &cable_type : can_extend ) {
            cable_types.emplace_back( cable_type == "ELECTRICAL_DEVICES" ? _( "electrical device cables" ) :
                                      itype_id( cable_type )->nname( 1 ) );
        }
        std::string cable_type_list = enumerate_as_string( cable_types, enumeration_conjunction::or_ );
        dump.emplace_back( "TOOL", string_format( _( "<bold>Can extend</bold>: %s" ), cable_type_list ) );
    }
}

std::optional<int> link_up_actor::use( Character *p, item &it, const tripoint_bub_ms &pos ) const
{
    return link_up_actor::use( p, it, &get_map(), pos );
}

std::optional<int> link_up_actor::use( Character *p, item &it, map *here,
                                       const tripoint_bub_ms &pos ) const
{
    if( !p ) {
        debugmsg( "%s called action link_up that requires character but no character is present",
                  it.typeId().str() );
        return std::nullopt;
    }

    if( targets.empty() ) {
        debugmsg( "Link up action for %s doesn't have any targets!", it.tname() );
        return std::nullopt;
    }

    const bool is_cable_item = it.has_flag( flag_CABLE_SPOOL );
    const bool unspooled = it.link_length() == -1;
    const bool has_loose_end = !unspooled && is_cable_item ?
                               it.link_has_state( link_state::no_link ) : it.has_no_links();

    const int respool_time_per_square = 200;
    const bool past_respool_threshold = it.link_length() > item::LINK_RESPOOL_THRESHOLD;
    const int respool_time_total = !past_respool_threshold ? 0 :
                                   ( it.link_length() - item::LINK_RESPOOL_THRESHOLD ) * respool_time_per_square;

    vehicle *t_veh = it.has_link_data() ? it.link().t_veh.get() : nullptr;

    uilist link_menu;
    if( !is_cable_item || it.has_no_links() ) {
        // This is either a device or a cable item without any connections.
        link_menu.text = string_format( _( "What to do with the %s?%s" ), it.link_name(), t_veh ?
                                        string_format( _( "\nAttached to: %s" ), t_veh->name ) : "" );
        if( targets.count( link_state::vehicle_port ) > 0 ) {
            link_menu.addentry( 0, has_loose_end, -1,
                                _( "Attach to dashboard, electronics control unit or appliance" ) );
        }
        if( targets.count( link_state::vehicle_battery ) > 0 ) {
            link_menu.addentry( 1, has_loose_end, -1, _( "Attach to vehicle battery or appliance" ) );
        }
        if( targets.count( link_state::vehicle_tow ) > 0 ) {
            link_menu.addentry( 10, has_loose_end, -1, _( "Attach tow cable to towing vehicle" ) );
            link_menu.addentry( 11, has_loose_end, -1, _( "Attach tow cable to towed vehicle" ) );
        }
        if( targets.count( link_state::bio_cable ) > 0 ) {
            if( !p->get_remote_fueled_bionic().is_empty() ) {
                link_menu.addentry( 20, has_loose_end, -1, _( "Attach to Cable Charger System CBM" ) );
            }
        }
        if( targets.count( link_state::ups ) > 0 && p->cache_has_item_with( flag_IS_UPS ) ) {
            link_menu.addentry( 21, has_loose_end, -1, _( "Attach to UPS" ) );
        }
        if( targets.count( link_state::solarpack ) > 0 ) {
            const bool has_solar_pack_on = p->worn_with_flag( flag_SOLARPACK_ON );
            if( has_solar_pack_on || p->worn_with_flag( flag_SOLARPACK ) ) {
                link_menu.addentry( 22, has_loose_end && has_solar_pack_on, -1, _( "Attach to solar pack" ) );
            }
        }
        if( !is_cable_item || !can_extend.empty() ) {
            const bool has_extensions = !unspooled &&
                                        !it.all_items_top( pocket_type::CABLE ).empty();
            link_menu.addentry( 30, has_loose_end, -1,
                                is_cable_item ? _( "Extend another cable" ) : _( "Extend with another cable" ) );
            link_menu.addentry( 31, has_extensions, -1, _( "Remove cable extensions" ) );
        }
        if( targets.count( link_state::no_link ) > 0 ) {
            if( unspooled ) {
                link_menu.addentry( 998, true, -1, _( "Re-spool" ) );
            } else {
                link_menu.addentry( 999, !it.has_no_links(), -1,
                                    past_respool_threshold ? _( "Detach and re-spool" ) : _( "Detach" ) );
            }
        }

    } else if( it.link_has_state( link_state::vehicle_tow ) ) {
        // Cables that started a tow can finish one or detach; nothing else.
        link_menu.text = string_format( _( "What to do with the %s?%s" ), it.link_name(), t_veh ?
                                        string_format( _( "\nAttached to: %s" ), t_veh->name ) : "" );

        link_menu.addentry( 10, has_loose_end && it.link().target == link_state::vehicle_tow, -1,
                            _( "Attach loose end to towing vehicle" ) );
        link_menu.addentry( 11, has_loose_end && it.link().source == link_state::vehicle_tow, -1,
                            _( "Attach loose end to towed vehicle" ) );
        if( targets.count( link_state::no_link ) > 0 ) {
            if( unspooled ) {
                link_menu.addentry( 998, true, -1, _( "Re-spool" ) );
            } else {
                link_menu.addentry( 999, true, -1,
                                    past_respool_threshold ? _( "Detach and re-spool" ) : _( "Detach" ) );
            }
        }

    } else {
        // This is a cable item with at least one connection already:
        std::string state_desc_lhs;
        std::string state_desc_rhs;
        if( it.link_has_state( link_state::no_link ) ) {
            state_desc_lhs = _( "\nAttached to " );
            if( t_veh ) {
                state_desc_rhs = it.link().t_veh->name;
            } else if( it.link_has_state( link_state::bio_cable ) ) {
                state_desc_rhs = _( "Cable Charger System" );
            } else if( it.link_has_state( link_state::ups ) ) {
                state_desc_rhs = _( "Unified Power Supply" );
            } else if( it.link_has_state( link_state::solarpack ) ) {
                state_desc_rhs = _( "solar backpack" );
            }
        } else {
            if( it.link().source ==  link_state::bio_cable ) {
                state_desc_lhs = _( "\nConnecting Cable Charger System to " );
            } else if( it.link().source == link_state::ups ) {
                state_desc_lhs = _( "\nConnecting UPS to " );
            } else if( it.link().source == link_state::solarpack ) {
                state_desc_lhs = _( "\nConnecting solar backpack to " );
            }
            if( it.link().t_veh && it.link().source != link_state::needs_reeling ) {
                state_desc_rhs = it.link().t_veh->name;
            } else if( it.link().target == link_state::bio_cable ) {
                state_desc_rhs = _( "Cable Charger System" );
            }
        }
        link_menu.text = string_format( _( "What to do with the %s?%s%s" ), it.link_name(),
                                        state_desc_lhs, state_desc_rhs );

        // TODO: Allow plugging UPSes and Solar Packs into more than just bionics.
        // There is already code to support setting up a link, but none for actual functionality.
        if( targets.count( link_state::vehicle_port ) > 0 ) {
            link_menu.addentry( 0, has_loose_end && !it.link_has_state( link_state::ups ) &&
                                !it.link_has_state( link_state::solarpack ),
                                -1, _( "Attach loose end to vehicle controls or appliance" ) );
        }
        if( targets.count( link_state::vehicle_battery ) > 0 ) {
            link_menu.addentry( 1, has_loose_end && !it.link_has_state( link_state::ups ) &&
                                !it.link_has_state( link_state::solarpack ),
                                -1, _( "Attach loose end to vehicle battery or appliance" ) );
        }
        if( targets.count( link_state::bio_cable ) > 0 && !p->get_remote_fueled_bionic().is_empty() ) {
            link_menu.addentry( 20, has_loose_end && !it.link_has_state( link_state::bio_cable ),
                                -1, _( "Attach loose end to Cable Charger System CBM" ) );
        }
        if( targets.count( link_state::ups ) > 0 && p->cache_has_item_with( flag_IS_UPS ) ) {
            link_menu.addentry( 21, has_loose_end && it.link_has_state( link_state::bio_cable ),
                                -1, _( "Attach loose end to UPS" ) );
        }
        if( targets.count( link_state::solarpack ) > 0 ) {
            const bool has_solar_pack_on = p->worn_with_flag( flag_SOLARPACK_ON );
            if( has_solar_pack_on || p->worn_with_flag( flag_SOLARPACK ) ) {
                link_menu.addentry( 22, has_loose_end && has_solar_pack_on &&
                                    it.link_has_state( link_state::bio_cable ),
                                    -1, _( "Attach loose end to solar pack" ) );
            }
        }
        if( !can_extend.empty() ) {
            const bool has_extensions = !unspooled &&
                                        !it.all_items_top( pocket_type::CABLE ).empty();
            link_menu.addentry( 30, has_loose_end, -1, _( "Extend another cable" ) );
            link_menu.addentry( 31, has_extensions, -1, _( "Remove cable extensions" ) );
        }
        if( targets.count( link_state::no_link ) > 0 ) {
            if( unspooled ) {
                link_menu.addentry( 998, true, -1, _( "Re-spool" ) );
            } else {
                link_menu.addentry( 999, true, -1,
                                    past_respool_threshold ? _( "Detach and re-spool" ) : _( "Detach" ) );
            }
        }
    }

    int choice = -1;
    if( targets.size() == 1 ) {
        choice = link_menu.entries.begin()->retval;
    } else {
        link_menu.query();
        choice = link_menu.ret;
    }

    if( choice < 0 ) {
        // Cancelled selection.
        return std::nullopt;

    } else if( choice >= 998 ) {
        // Selection: Detach & respool.

        // Reopen the menu after respooling.
        p->assign_activity( invoke_item_activity_actor( item_location{*p, &it}, "link_up" ) );
        p->activity.auto_resume = true;

        if( t_veh ) {
            // Cancel out the linked device's power draw so the vehicle's power display will be accurate.
            int power_draw = it.charge_linked_batteries( *t_veh, 0 );
            t_veh->linked_item_epower_this_turn += units::from_milliwatt( power_draw );
        }

        it.reset_link( true, p );
        // Cables that are too long need to be manually rewound before reuse.
        if( it.link_length() == -1 ) {
            p->assign_activity( player_activity( reel_cable_activity_actor( respool_time_total, item_location{*p, &it} ) ) );
            return 0;
        } else {
            p->add_msg_if_player( m_info, string_format( is_cable_item ? _( "You detach the %s." ) :
                                  _( "You gather the %s up with it." ), it.link_name() ) );
        }
        return 0;
    }

    if( choice == 0 || choice == 1 ) {
        // Selection: Attach electrical cable to vehicle ports / appliances, OR vehicle batteries.
        return link_to_veh_app( p, it, choice == 0 );

    } else if( choice == 10 || choice == 11 ) {
        // Selection: Attach tow cable to towing/towed vehicle.
        return link_tow_cable( p, it, choice == 10 );

    } else if( choice == 30 ) {
        // Selection: Attach to another cable, resulting in a longer one.
        return link_extend_cable( p, it, here, pos );

    } else if( choice == 31 ) {
        // Selection: Remove all cable extensions and give the individual cables to the player.
        return remove_extensions( p, it );
    }

    if( choice == 20 ) {
        // Selection: Attach electrical cable to Cable Charger System CBM.
        if( it.has_no_links() ) {
            it.link().target = link_state::bio_cable;
            p->add_msg_if_player( m_info, _( "You attach the cable to your Cable Charger System." ) );
        } else if( it.link().source == link_state::ups ) {
            it.link().target = link_state::bio_cable;
            p->add_msg_if_player( m_good, _( "You are now plugged into the UPS." ) );
        } else if( it.link().source == link_state::solarpack ) {
            it.link().target = link_state::bio_cable;
            p->add_msg_if_player( m_good, _( "You are now plugged into the solar backpack." ) );
        } else if( it.link().target == link_state::vehicle_port ||
                   it.link().target == link_state::vehicle_battery ) {
            it.link().source = link_state::bio_cable;
            p->add_msg_if_player( m_good, _( "You are now plugged into the vehicle." ) );
        }

        it.update_link_traits();
        it.process( *here, p, p->pos_bub( *here ) );
        p->mod_moves( -move_cost );
        return 0;

    } else if( choice == 21 ) {
        // Selection: Attach electrical cable to ups.
        item_location loc;
        avatar *you = p->as_avatar();
        const std::string choose_ups = _( "Choose UPS:" );
        const std::string dont_have_ups = _( "You need an active UPS." );
        auto ups_filter = [&]( const item & itm ) {
            return itm.has_flag( flag_IS_UPS );
        };

        if( you != nullptr ) {
            loc = game_menus::inv::titled_filter_menu( ups_filter, *you, choose_ups, -1, dont_have_ups );
        }
        if( !loc ) {
            p->add_msg_if_player( _( "Never mind." ) );
            return std::nullopt;
        }

        if( it.has_no_links() ) {
            p->add_msg_if_player( m_info, _( "You attach the cable to the UPS." ) );
        } else if( it.link().target == link_state::bio_cable ) {
            p->add_msg_if_player( m_good, _( "You are now plugged into the UPS." ) );
        } else if( it.link().source == link_state::solarpack ) {
            p->add_msg_if_player( m_good, _( "You link up the UPS and the solar backpack." ) );
        } else if( it.link().target == link_state::vehicle_port ||
                   it.link().target == link_state::vehicle_battery ) {
            p->add_msg_if_player( m_good, _( "You link up the UPS and the vehicle." ) );
        }

        it.link().source = link_state::ups;
        loc->set_var( "cable", "plugged_in" );
        it.update_link_traits();
        it.process( *here, p, p->pos_bub( *here ) );
        p->mod_moves( -move_cost );
        return 0;

    } else if( choice == 22 ) {
        // Selection: Attach electrical cable to solar pack.
        item_location loc;
        avatar *you = p->as_avatar();
        const std::string choose_solar = _( "Choose solar pack:" );
        const std::string dont_have_solar = _( "You need an unfolded solar pack." );
        auto solar_filter = [&]( const item & itm ) {
            return itm.has_flag( flag_SOLARPACK_ON );
        };

        if( you != nullptr ) {
            loc = game_menus::inv::titled_filter_menu( solar_filter, *you, choose_solar, -1, dont_have_solar );
        }
        if( !loc ) {
            p->add_msg_if_player( _( "Never mind." ) );
            return std::nullopt;
        }

        if( it.has_no_links() ) {
            p->add_msg_if_player( m_info, _( "You attach the cable to the solar pack." ) );
        } else if( it.link().target == link_state::bio_cable ) {
            p->add_msg_if_player( m_good, _( "You are now plugged into the solar pack." ) );
        } else if( it.link().source == link_state::ups ) {
            p->add_msg_if_player( m_good, _( "You link up the solar pack and the UPS." ) );
        } else if( it.link().target == link_state::vehicle_port ||
                   it.link().target == link_state::vehicle_battery ) {
            p->add_msg_if_player( m_good, _( "You link up the solar pack and the vehicle." ) );
        }

        it.link().source = link_state::solarpack;
        loc->set_var( "cable", "plugged_in" );
        it.update_link_traits();
        it.process( *here, p, p->pos_bub( *here ) );
        p->mod_moves( -move_cost );
        return 0;
    }
    return std::nullopt;
}

std::optional<int> link_up_actor::link_to_veh_app( Character *p, item &it,
        const bool to_ports ) const
{
    map &here = get_map();
    // Selection: Attach electrical cable to vehicle ports / appliances, OR vehicle batteries.

    const auto can_link = [&here, &to_ports]( const tripoint_bub_ms & point ) {
        const optional_vpart_position ovp = here.veh_at( point );
        return ovp && ovp->vehicle().avail_linkable_part( ovp->mount_pos(), to_ports ) != -1;
    };
    const std::optional<tripoint_bub_ms> pnt_ = choose_adjacent_highlight(
                here, _( "Attach the cable where?" ),
                "", can_link, false, false );
    if( !pnt_ ) {
        p->add_msg_if_player( _( "Never mind." ) );
        return std::nullopt;
    }
    const tripoint_bub_ms &selection = *pnt_;
    const optional_vpart_position sel_vp = here.veh_at( selection );
    if( !sel_vp ) {
        p->add_msg_if_player( _( "There's no vehicle there." ) );
        return std::nullopt;
    }

    // You used to be able to plug cables in anywhere on a vehicle, so there's extra effort here
    // to inform players that they can only plug them into dashboards or electrical controls now.
    if( !can_link( selection ) ) {
        if( to_ports && sel_vp && sel_vp->vehicle().has_part( "CABLE_PORTS" ) ) {
            p->add_msg_if_player( m_info,
                                  _( "You can't attach it there - try the dashboard or electronics controls." ) );
        } else if( !to_ports && sel_vp && !sel_vp->vehicle().batteries.empty() ) {
            p->add_msg_if_player( m_info,
                                  _( "You can't attach it there - try the battery." ) );
        } else {
            p->add_msg_if_player( m_info, _( "You can't attach it there." ) );
        }
        return std::nullopt;
    }

    if( !it.link_has_state( link_state::vehicle_port ) &&
        !it.link_has_state( link_state::vehicle_battery ) ) {

        // Starting a new connection to a vehicle or connecting a cable CBM to a vehicle.
        bool had_bio_link = it.link_has_state( link_state::bio_cable );
        ret_val<void> result = it.link_to( sel_vp, to_ports ? link_state::vehicle_port :
                                           link_state::vehicle_battery );
        if( !result.success() ) {
            p->add_msg_if_player( m_bad, result.str() );
            return 0;
        }

        // Get the part name for the connection message, using the vehicle name as a fallback.
        const int part_index = sel_vp->vehicle().avail_linkable_part( sel_vp->mount_pos(), to_ports );
        const std::string sel_vp_name = part_index == -1 ? sel_vp->vehicle().name :
                                        sel_vp->vehicle().part( part_index ).name( false );

        if( had_bio_link ) {
            p->add_msg_if_player( m_good, _( "You are now plugged into the %s." ), sel_vp_name );
            it.link().source = link_state::bio_cable;
        } else {
            p->add_msg_if_player( _( "You connect the %1$s to the %2$s." ), it.type_name(), sel_vp_name );
        }

        it.process( here, p, p->pos_bub() );
        p->mod_moves( -move_cost );
        return 0;

    } else {

        // Connecting two vehicles together.
        const bool using_power_cord = it.typeId() == itype_power_cord;
        if( using_power_cord && it.link().t_veh->is_powergrid() && sel_vp->vehicle().is_powergrid() ) {
            // If both vehicles are adjacent power grids, try to merge them together first.
            const point_bub_ms prev_pos = here.get_bub( it.link().t_veh->coord_translate(
                                              it.link().t_mount ) +
                                          it.link().t_abs_pos ).xy();
            if( selection.xy().raw().distance( prev_pos.raw() ) <= 1.5f &&
                it.link().t_veh->merge_appliance_into_grid( &here,  sel_vp->vehicle() ) ) {
                it.link().t_veh->part_removal_cleanup( here );
                p->add_msg_if_player( _( "You merge the two power grids." ) );
                return 1;
            }
            // Unable to merge, so connect them with a power cord instead.
        }
        ret_val<void> result = it.link_to( sel_vp, to_ports ? link_state::vehicle_port :
                                           link_state::vehicle_battery );
        if( !result.success() ) {
            p->add_msg_if_player( m_bad, result.str() );
            return 0;
        }
        if( using_power_cord || p->has_item( it ) ) {
            p->add_msg_if_player( m_good, result.str() );
        }

        if( using_power_cord ) {
            // Remove linked_flag from attached parts - the just-added cable vehicle parts do the same thing.
            it.reset_link( true, p );
        }
        p->mod_moves( -move_cost );
        return 1; // Let the cable be destroyed.
    }
}

std::optional<int> link_up_actor::link_tow_cable( Character *p, item &it,
        const bool to_towing ) const
{
    map &here = get_map();

    const auto can_link = [&here]( const tripoint_bub_ms & point ) {
        const optional_vpart_position ovp = here.veh_at( point );
        return ovp && ovp->vehicle().is_external_part( &here,  point );
    };

    const std::optional<tripoint_bub_ms> pnt_ = choose_adjacent_highlight(
                here, to_towing ? _( "Attach cable to the vehicle that will do the towing." ) :
                _( "Attach cable to the vehicle that will be towed." ), "", can_link, false, false );
    if( !pnt_ ) {
        p->add_msg_if_player( _( "Never mind." ) );
        return std::nullopt;
    }
    const tripoint_bub_ms &selection = *pnt_;
    const optional_vpart_position sel_vp = here.veh_at( selection );
    if( !sel_vp ) {
        p->add_msg_if_player( _( "There's no vehicle there." ) );
        return std::nullopt;
    }

    if( it.has_no_links() ) {

        // Starting a new tow cable connection.
        ret_val<void> result = it.link_to( sel_vp, link_state::vehicle_tow );
        if( !result.success() ) {
            p->add_msg_if_player( m_bad, result.str() );
            return 0;
        }
        if( to_towing ) {
            it.link().source = link_state::vehicle_tow;
            it.link().target = link_state::no_link;
        } else {
            it.link().source = link_state::no_link;
            it.link().target = link_state::vehicle_tow;
        }

        p->add_msg_if_player( _( "You connect the %1$s to the %2$s." ), it.type_name(),
                              sel_vp->vehicle().name );

        it.process( here, p, p->pos_bub() );
        p->mod_moves( -move_cost );
        return 0;

    } else {

        // Connecting two vehicles with tow cable.
        ret_val<void> result = it.link_to( sel_vp, link_state::vehicle_tow );
        if( !result.success() ) {
            p->add_msg_if_player( m_bad, result.str() );
            return 0;
        }
        if( p->has_item( it ) ) {
            p->add_msg_if_player( m_good, result.str() );
        }

        p->mod_moves( -move_cost );
        return 1; // Let the cable be destroyed.
    }
}

std::optional<int> link_up_actor::link_extend_cable( Character *p, item &it,
        map *here, const tripoint_bub_ms &pnt ) const
{
    avatar *you = p->as_avatar();
    if( !you ) {
        p->add_msg_if_player( m_info, _( "Never mind." ) );
        return std::nullopt;
    }

    const bool is_cable_item = it.has_flag( flag_CABLE_SPOOL );
    item_location selected;
    if( is_cable_item ) {
        const bool can_extend_devices = can_extend.find( "ELECTRICAL_DEVICES" ) != can_extend.end();
        const auto filter = [this, &it, &can_extend_devices]( const item & inv ) {
            if( !inv.can_link_up() || inv.link_has_state( link_state::needs_reeling ) ||
                ( !inv.has_flag( flag_CABLE_SPOOL ) && !can_extend_devices ) ) {
                return false;
            }
            return can_extend.find( inv.typeId().c_str() ) != can_extend.end() && &inv != &it;
        };
        selected = game_menus::inv::titled_filter_menu( filter, *you, _( "Extend which cable?" ), -1,
                   _( "You don't have a compatible cable." ) );
    } else {
        const auto filter = []( const item & inv ) {
            if( !inv.can_link_up() || inv.link_has_state( link_state::needs_reeling ) ||
                !inv.has_flag( flag_CABLE_SPOOL ) ) {
                return false;
            }
            const link_up_actor *actor = static_cast<const link_up_actor *>
                                         ( inv.get_use( "link_up" )->get_actor_ptr() );
            return actor->can_extend.find( "ELECTRICAL_DEVICES" ) != actor->can_extend.end();
        };
        selected = game_menus::inv::titled_filter_menu( filter, *you, _( "Extend with which cable?" ), -1,
                   _( "You don't have a compatible cable." ) );
    }
    if( !selected ) {
        p->add_msg_if_player( m_info, _( "Never mind." ) );
        return std::nullopt;
    }

    item_location extension = is_cable_item ? form_loc( *p, here, pnt, it ) : selected;
    item_location extended = is_cable_item ? selected : form_loc( *p, here, pnt, it );
    std::optional<item> extended_copy;

    // We'll make a copy of the extended item and check pocket weight/volume capacity if:
    //   1. The extended item is in a container,
    //   2. The extended item and extension cord(s) aren't in the same pocket, and
    //   3. The extended item is in a pocket without enough remaining room for the extension cord(s).
    if( extended.where() == item_location::type::container &&
        extended.parent_pocket() != extension.parent_pocket() &&
        ( extended.volume_capacity() - extension->volume() < 0_ml ||
          extended.weight_capacity() - extension->weight() < 0_gram ) ) {

        extended_copy = *extended;
    }

    item *extended_ptr = extended_copy ? &extended_copy.value() : &*extended;

    // Put the extension cable and all of its attached cables, if any, into the extended item's CABLE pocket.
    std::vector<const item *> all_cables = extension->cables();
    all_cables.emplace_back( &*extension );
    for( const item *cable : all_cables ) {
        item cable_copy( *cable );
        cable_copy.get_contents().clear_items();
        cable_copy.reset_link();
        if( !extended_ptr->put_in( cable_copy, pocket_type::CABLE ).success() ) {
            debugmsg( "Failed to put %s inside %s!", cable_copy.type_name(), extended_ptr->type_name() );
        }
    }
    if( extension->has_link_data() ) {
        extended_ptr->link() = extension->link();
    }
    extended_ptr->update_link_traits();
    extended_ptr->process( *here, p, p->pos_bub( *here ) );

    if( extended_copy ) {
        // Check if there's another pocket on the same container that can hold the extended item, respecting pocket settings.
        item_location parent = extended.parent_item();
        if( parent->can_contain( *extended_ptr, false, false, false, false,
                                 item_location(), 10000000_ml, false ).success() ) {
            if( !parent->put_in( *extended_ptr, pocket_type::CONTAINER ).success() ) {
                debugmsg( "Failed to put %s inside %s!", extended_ptr->type_name(),
                          parent->type_name() );
                return std::nullopt;
            }
        } else {
            if( !query_yn( _( "The %1$s can't contain the %2$s with the %3$s attached.  Continue?" ),
                           parent->type_name(), extended_ptr->type_name(), extension->type_name() ) ) {
                return std::nullopt;
            }
            // Attach the cord, even though it won't fit, and let it spill out naturally.
            extended.parent_pocket()->add( *extended_ptr );
        }
        extended.make_active();
        extended.remove_item();
    }

    p->add_msg_if_player( _( "You extend the %1$s with the %2$s." ),
                          extended_ptr->link_name(), extension->type_name() );
    extension.remove_item();
    p->invalidate_inventory_validity_cache();
    p->drop_invalid_inventory();
    p->mod_moves( -move_cost );
    return 0;
}

std::optional<int> link_up_actor::remove_extensions( Character *p, item &it ) const
{
    std::list<item *> all_cables = it.all_items_ptr( pocket_type::CABLE );
    all_cables.remove_if( []( const item * cable ) {
        return !cable->has_flag( flag_CABLE_SPOOL ) || !cable->can_link_up();
    } );

    if( all_cables.empty() ) {
        // Delete any non-cables that somehow got into the pocket.
        it.get_contents().clear_pockets_if( []( item_pocket const & pocket ) {
            return pocket.is_type( pocket_type::CABLE );
        } );
        return 0;
    }

    item cable_main_copy( *all_cables.back() );
    all_cables.pop_back();
    if( !all_cables.empty() ) {
        for( item *cable : all_cables ) {
            item cable_copy( *cable );
            cable_copy.get_contents().clear_items();
            cable_copy.reset_link();
            if( !cable_main_copy.put_in( cable_copy, pocket_type::CABLE ).success() ) {
                debugmsg( "Failed to put %s inside %s!", cable_copy.tname(), cable_main_copy.tname() );
            }
        }
    }
    p->add_msg_if_player( _( "You disconnect the %1$s from the %2$s." ),
                          cable_main_copy.type_name(), it.type_name() );

    it.get_contents().clear_pockets_if( []( item_pocket const & pocket ) {
        return pocket.is_type( pocket_type::CABLE );
    } );

    if( it.has_link_data() ) {
        // If the item was linked, keep the extension cables linked.
        cable_main_copy.link() = it.link();
        cable_main_copy.update_link_traits();
        cable_main_copy.process( get_map(), p, p->pos_bub() );
        it.reset_link( true, p );
    }

    p->i_add_or_drop( cable_main_copy );
    p->mod_moves( -move_cost );
    return 0;
}

std::unique_ptr<iuse_actor> deploy_tent_actor::clone() const
{
    return std::make_unique<deploy_tent_actor>( *this );
}

void deploy_tent_actor::load( const JsonObject &obj, const std::string & )
{
    assign( obj, "radius", radius );
    assign( obj, "wall", wall );
    assign( obj, "floor", floor );
    assign( obj, "floor_center", floor_center );
    assign( obj, "door_opened", door_opened );
    assign( obj, "door_closed", door_closed );
    assign( obj, "broken_type", broken_type );
}

std::optional<int> deploy_tent_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return deploy_tent_actor::use( p, it, &get_map(), pos );
}

std::optional<int> deploy_tent_actor::use( Character *p, item &it, map *here,
        const tripoint_bub_ms & ) const
{
    map &bubble_map = reality_bubble();

    if( here != &bubble_map ) { // Or make 'choose_direction' map aware.
        debugmsg( "deply_tent_actor::use can only be called from the reality bubble" );
        return std::nullopt;
    }

    int diam = 2 * radius + 1;
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    const std::optional<tripoint_rel_ms> dir = choose_direction( string_format(
                _( "Put up the %s where (%dx%d clear area)?" ), it.tname(), diam, diam ) );
    if( !dir ) {
        return std::nullopt;
    }
    const tripoint_rel_ms direction = *dir;

    // We place the center of the structure (radius + 1)
    // spaces away from the player.
    // First check there's enough room.
    const tripoint_bub_ms center = p->pos_bub( *here ) + point_rel_ms( ( radius + 1 ) * direction.x(),
                                   ( radius + 1 ) * direction.y() );
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint_bub_ms &dest : here->points_in_radius( center, radius ) ) {
        if( const optional_vpart_position vp = here->veh_at( dest ) ) {
            add_msg( m_info, _( "The %s is in the way." ), vp->vehicle().name );
            return std::nullopt;
        }
        if( const Creature *const c = creatures.creature_at( dest ) ) {
            add_msg( m_info, _( "The %s is in the way." ), c->disp_name() );
            return std::nullopt;
        }
        if( here->impassable( dest ) || !here->has_flag( ter_furn_flag::TFLAG_FLAT, dest ) ) {
            add_msg( m_info, _( "The %s in that direction isn't suitable for placing the %s." ),
                     here->name( dest ), it.tname() );
            return std::nullopt;
        }
        if( here->has_furn( dest ) ) {
            add_msg( m_info, _( "There is already furniture (%s) there." ), here->furnname( dest ) );
            return std::nullopt;
        }
    }

    //checks done start activity:
    tent_placement_activity_actor actor( to_moves<int>( 20_minutes ), direction, radius, it, wall,
                                         floor, floor_center, door_closed );
    get_player_character().assign_activity( actor );
    p->i_rem( &it );
    return 0;
}

bool deploy_tent_actor::check_intact( const tripoint_bub_ms &center ) const
{
    map &here = get_map();
    for( const tripoint_bub_ms &dest : here.points_in_radius( center, radius ) ) {
        const furn_id &fid = here.furn( dest );
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

std::optional<int> weigh_self_actor::use( Character *p, item &it, const tripoint_bub_ms &pos ) const
{
    return weigh_self_actor::use( p, it, &get_map(), pos );
}

std::optional<int> weigh_self_actor::use( Character *p, item &, map *,
        const tripoint_bub_ms & ) const
{
    if( p->is_mounted() ) {
        p->add_msg_if_player( m_info, _( "You cannot weigh yourself while mounted." ) );
        return std::nullopt;
    }
    // this is a weight, either in kgs or in lbs.
    double weight = convert_weight( p->get_weight() );
    if( weight > convert_weight( max_weight ) ) {
        popup( _( "ERROR: Max weight of %.0f %s exceeded" ), convert_weight( max_weight ), weight_units() );
    } else {
        popup( "%.0f %s", weight, weight_units() );
    }
    return 0;
}

void weigh_self_actor::load( const JsonObject &jo, const std::string & )
{
    assign( jo, "max_weight", max_weight );
}

std::unique_ptr<iuse_actor> weigh_self_actor::clone() const
{
    return std::make_unique<weigh_self_actor>( *this );
}

void sew_advanced_actor::load( const JsonObject &obj, const std::string & )
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

std::optional<int> sew_advanced_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return sew_advanced_actor::use( p, it, &get_map(), pos );
}

std::optional<int> sew_advanced_actor::use( Character *p, item &it, map *here,
        const tripoint_bub_ms & ) const
{
    if( p->is_npc() ) {
        return std::nullopt;
    }
    if( p->cant_do_mounted() ) {
        return std::nullopt;
    }
    if( p->cant_do_underwater() ) {
        return std::nullopt;
    }

    if( p->fine_detail_vision_mod() > 4 ) {
        add_msg( m_info, _( "You can't see to sew!" ) );
        return std::nullopt;
    }

    auto filter = [this]( const item & itm ) {
        return itm.is_armor() && !itm.is_firearm() && !itm.is_power_armor() && !itm.is_gunmod() &&
               itm.made_of_any( materials ) && !itm.has_flag( flag_INTEGRATED );
    };
    // note: if !p.is_npc() then p is avatar.
    item_location loc = game_menus::inv::titled_filter_menu(
                            filter, *p->as_avatar(), _( "Enhance which clothing?" ) );
    if( !loc ) {
        p->add_msg_if_player( m_info, _( "You do not have that item!" ) );
        return std::nullopt;
    }
    item &mod = *loc;
    if( &mod == &it ) {
        p->add_msg_if_player( m_info,
                              _( "This can be used to repair or modify other items, not itself." ) );
        return std::nullopt;
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
    const inventory &crafting_inv = p->crafting_inventory( here );
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
            } else if( !it.ammo_sufficient( p, thread_needed ) ) {
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
        // FIXME: Remove sew_advanced_actor, no longer used since before 0.G
        desc += colorize( string_format( "%s: %.2f->%.2f\n", _( "Bash" ), mod.resist( damage_bash ),
                                         temp_item.resist( damage_bash ) ), get_compare_color( mod.resist( damage_bash ),
                                                 temp_item.resist( damage_bash ), true ) );
        desc += colorize( string_format( "%s: %.2f->%.2f\n", _( "Cut" ), mod.resist( damage_cut ),
                                         temp_item.resist( damage_cut ) ), get_compare_color( mod.resist( damage_cut ),
                                                 temp_item.resist( damage_cut ), true ) );
        desc += colorize( string_format( "%s: %.2f->%.2f\n", _( "Ballistic" ),
                                         mod.resist( damage_bullet ),
                                         temp_item.resist( damage_bullet ) ), get_compare_color( mod.resist( damage_bullet ),
                                                 temp_item.resist( damage_bullet ),
                                                 true ) );
        desc += colorize( string_format( "%s: %.2f->%.2f\n", _( "Acid" ), mod.resist( damage_acid ),
                                         temp_item.resist( damage_acid ) ), get_compare_color( mod.resist( damage_acid ),
                                                 temp_item.resist( damage_acid ), true ) );
        desc += colorize( string_format( "%s: %.2f->%.2f\n", _( "Fire" ), mod.resist( damage_heat ),
                                         temp_item.resist( damage_heat ) ), get_compare_color( mod.resist( damage_heat ),
                                                 temp_item.resist( damage_heat ), true ) );
        desc += colorize( string_format( "%s: %d->%d\n", _( "Warmth" ), mod.get_warmth(),
                                         temp_item.get_warmth() ), get_compare_color( mod.get_warmth(), temp_item.get_warmth(), true ) );
        desc += colorize( string_format( "%s: %d->%d\n", _( "Encumbrance" ), mod.get_avg_encumber( *p ),
                                         temp_item.get_avg_encumber( *p ) ), get_compare_color( mod.get_avg_encumber( *p ),
                                                 temp_item.get_avg_encumber( *p ), false ) );

        tmenu.addentry_desc( index++, enab, MENU_AUTOASSIGN, prompt, desc );
    }
    tmenu.desc_enabled = true;
    tmenu.query();
    const int choice = tmenu.ret;

    if( choice < 0 || choice >= static_cast<int>( clothing_mods.size() ) ) {
        return std::nullopt;
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
    p->mod_moves( -to_moves<int>( 30_seconds * p->fine_detail_vision_mod() ) );
    p->practice( used_skill, items_needed * 3 + 3 );
    /** @EFFECT_TAILOR randomly improves clothing modification efforts */
    int rn = dice( 3, 2 + round( p->get_skill_level( used_skill ) ) ); // Skill
    /** @EFFECT_DEX randomly improves clothing modification efforts */
    rn += rng( 0, p->dex_cur / 2 );                    // Dexterity
    /** @EFFECT_PER randomly improves clothing modification efforts */
    rn += rng( 0, p->per_cur / 2 );                    // Perception
    rn -= mod_count * 10;                              // Other mods

    if( rn <= 8 ) {
        const std::string startdurability = mod.durability_indicator( true );
        const bool destroyed = mod.inc_damage();
        const std::string resultdurability = mod.durability_indicator( true );
        p->add_msg_if_player( m_bad, _( "You damage your %s trying to modify it!  ( %s-> %s)" ),
                              mod.tname( 1, false ), startdurability, resultdurability );
        if( destroyed ) {
            p->add_msg_if_player( m_bad, _( "You destroy it!" ) );
            p->i_rem_keep_contents( &mod );
            p->calc_encumbrance();
            p->calc_discomfort();
        }
        return thread_needed / 2;
    } else if( rn <= 10 ) {
        p->add_msg_if_player( m_bad,
                              _( "You fail to modify the clothing, and you waste thread and materials." ) );
        p->consume_items( comps, 1, is_crafting_component );
        return thread_needed;
    } else if( rn <= 14 ) {
        p->add_msg_if_player( m_mixed, _( "You modify your %s, but waste a lot of thread." ),
                              mod.tname() );
        p->consume_items( comps, 1, is_crafting_component );
        mod.set_flag( the_mod );
        mod.update_clothing_mod_val();
        p->calc_encumbrance();
        p->calc_discomfort();
        return thread_needed;
    }

    p->add_msg_if_player( m_good, _( "You modify your %s!" ), mod.tname() );
    mod.set_flag( the_mod );
    mod.update_clothing_mod_val();
    p->consume_items( comps, 1, is_crafting_component );
    p->calc_encumbrance();
    p->calc_discomfort();
    return thread_needed / 2;
}

std::unique_ptr<iuse_actor> sew_advanced_actor::clone() const
{
    return std::make_unique<sew_advanced_actor>( *this );
}

void change_scent_iuse::load( const JsonObject &obj, const std::string & )
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

std::optional<int> change_scent_iuse::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return change_scent_iuse::use( p, it, &get_map(), pos );
}

std::optional<int> change_scent_iuse::use( Character *p, item &it, map *,
        const tripoint_bub_ms & ) const
{
    p->set_value( "prev_scent", p->get_type_of_scent().c_str() );
    if( waterproof ) {
        p->set_value( "waterproof_scent", "true" );
    }
    p->add_effect( effect_masked_scent, duration, false, scent_mod );
    p->set_type_of_scent( scenttypeid );
    p->mod_moves( -moves );
    add_msg( m_info, _( "You use the %s to mask your scent" ), it.tname() );

    // Apply the various effects.
    for( const effect_data &eff : effects ) {
        p->add_effect( eff.id, eff.duration, eff.bp, eff.permanent );
    }
    return charges_to_use;
}

std::unique_ptr<iuse_actor> change_scent_iuse::clone() const
{
    return std::make_unique<change_scent_iuse>( *this );
}

std::unique_ptr<iuse_actor> effect_on_conditions_actor::clone() const
{
    return std::make_unique<effect_on_conditions_actor>( *this );
}

void effect_on_conditions_actor::load( const JsonObject &obj, const std::string &src )
{
    obj.read( "description", description );
    obj.read( "menu_text", menu_text );
    need_worn = obj.get_bool( "need_worn", false );
    need_wielding = obj.get_bool( "need_wielding", false );
    for( JsonValue jv : obj.get_array( "effect_on_conditions" ) ) {
        eocs.emplace_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }
}

std::string effect_on_conditions_actor::get_name() const
{
    if( !menu_text.empty() ) {
        return menu_text.translated();
    }
    return iuse_actor::get_name();
}

void effect_on_conditions_actor::info( const item &, std::vector<iteminfo> &dump ) const
{
    dump.emplace_back( "DESCRIPTION", description.translated() );
}

std::optional<int> effect_on_conditions_actor::use( Character *p, item &it,
        const tripoint_bub_ms &pos ) const
{
    return effect_on_conditions_actor::use( p, it, &get_map(), pos );
}

std::optional<int> effect_on_conditions_actor::use( Character *p, item &it,
        map *here, const tripoint_bub_ms &pos ) const
{
    if( it.type->comestible ) {
        debugmsg( "Comestibles are not properly consumed via effect_on_conditions and effect_on_conditions should not be used on items of type comestible until/unless this is resolved.  Rather than a use_action, use the consumption_effect_on_conditions JSON parameter on the comestible" );
        return 0;
    }

    if( need_worn && !p->is_worn( it ) ) {
        p->add_msg_if_player( m_info, _( "You need to wear the %1$s before activating it." ), it.tname() );
        return std::nullopt;
    }
    if( need_wielding && !p->is_wielding( it ) ) {
        p->add_msg_if_player( m_info, _( "You need to wield the %1$s before activating it." ), it.tname() );
        return std::nullopt;
    }
    Character *char_ptr = nullptr;
    item_location loc;
    if( p ) {
        if( avatar *u = p->as_avatar() ) {
            char_ptr = u;
        } else if( npc *n = p->as_npc() ) {
            char_ptr = n;
        }
        loc = item_location( *p->as_character(), &it );
    } else {
        loc = item_location( map_cursor( here, pos ), &it );
    }

    dialogue d( ( char_ptr == nullptr ? nullptr : get_talker_for( char_ptr ) ), get_talker_for( loc ) );
    write_var_value( var_type::context, "id", &d, it.typeId().str() );
    for( const effect_on_condition_id &eoc : eocs ) {
        if( eoc->type == eoc_type::ACTIVATION ) {
            eoc->activate( d );
        } else {
            debugmsg( "Must use an activation eoc for activation.  If you don't want the effect_on_condition to happen on its own (without the item's involvement), remove the recurrence min and max.  Otherwise, create a non-recurring effect_on_condition for this item with its condition and effects, then have a recurring one queue it." );
        }
    }
    // Prevents crash from trying to spend charge with item removed
    // NOTE: Because this section and/or calling stack does not check if the item exists in the surrounding tiles
    // it will not properly decrement any item of type `comestible` if consumed via the `E` `Consume item` menu.
    // Therefore, it is not advised to use items of type `comestible` with a `use_action` of type
    // `effect_on_conditions` until/unless this section is properly updated to actually consume said item.
    if( p && !p->has_item( it ) ) {
        return 0;
    }
    return 1;
}
