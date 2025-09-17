#include "activity_handlers.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "activity_actor.h"
#include "avatar.h"
#include "avatar_action.h"
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "character_attire.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "clzones.h"
#include "construction.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "fault.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "iexamine.h"
#include "inventory.h"
#include "inventory_ui.h"
#include "item.h"
#include "item_factory.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "magic.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "map_selector.h"
#include "martialarts.h"
#include "math_parser_diag_value.h"
#include "memory_fast.h"
#include "messages.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "omdata.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "pimpl.h"
#include "player_activity.h"
#include "pocket_type.h"
#include "point.h"
#include "proficiency.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "skill.h"
#include "sounds.h"
#include "string_formatter.h"
#include "talker.h"
#include "text_snippets.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "uilist.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_interact.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

static const activity_id ACT_ARMOR_LAYERS( "ACT_ARMOR_LAYERS" );
static const activity_id ACT_ATM( "ACT_ATM" );
static const activity_id ACT_BUILD( "ACT_BUILD" );
static const activity_id ACT_CONSUME_DRINK_MENU( "ACT_CONSUME_DRINK_MENU" );
static const activity_id ACT_CONSUME_FOOD_MENU( "ACT_CONSUME_FOOD_MENU" );
static const activity_id ACT_CONSUME_MEDS_MENU( "ACT_CONSUME_MEDS_MENU" );
static const activity_id ACT_DISMEMBER( "ACT_DISMEMBER" );
static const activity_id ACT_EAT_MENU( "ACT_EAT_MENU" );
static const activity_id ACT_FERTILIZE_PLOT( "ACT_FERTILIZE_PLOT" );
static const activity_id ACT_FETCH_REQUIRED( "ACT_FETCH_REQUIRED" );
static const activity_id ACT_FILL_LIQUID( "ACT_FILL_LIQUID" );
static const activity_id ACT_FIND_MOUNT( "ACT_FIND_MOUNT" );
static const activity_id ACT_FISH( "ACT_FISH" );
static const activity_id ACT_GAME( "ACT_GAME" );
static const activity_id ACT_GENERIC_GAME( "ACT_GENERIC_GAME" );
static const activity_id ACT_HAND_CRANK( "ACT_HAND_CRANK" );
static const activity_id ACT_HEATING( "ACT_HEATING" );
static const activity_id ACT_JACKHAMMER( "ACT_JACKHAMMER" );
static const activity_id ACT_MEND_ITEM( "ACT_MEND_ITEM" );
static const activity_id ACT_MOVE_LOOT( "ACT_MOVE_LOOT" );
static const activity_id ACT_MULTIPLE_BUTCHER( "ACT_MULTIPLE_BUTCHER" );
static const activity_id ACT_MULTIPLE_CHOP_PLANKS( "ACT_MULTIPLE_CHOP_PLANKS" );
static const activity_id ACT_MULTIPLE_CHOP_TREES( "ACT_MULTIPLE_CHOP_TREES" );
static const activity_id ACT_MULTIPLE_CONSTRUCTION( "ACT_MULTIPLE_CONSTRUCTION" );
static const activity_id ACT_MULTIPLE_CRAFT( "ACT_MULTIPLE_CRAFT" );
static const activity_id ACT_MULTIPLE_DIS( "ACT_MULTIPLE_DIS" );
static const activity_id ACT_MULTIPLE_FARM( "ACT_MULTIPLE_FARM" );
static const activity_id ACT_MULTIPLE_FISH( "ACT_MULTIPLE_FISH" );
static const activity_id ACT_MULTIPLE_MINE( "ACT_MULTIPLE_MINE" );
static const activity_id ACT_MULTIPLE_MOP( "ACT_MULTIPLE_MOP" );
static const activity_id ACT_MULTIPLE_READ( "ACT_MULTIPLE_READ" );
static const activity_id ACT_OPERATION( "ACT_OPERATION" );
static const activity_id ACT_PICKAXE( "ACT_PICKAXE" );
static const activity_id ACT_PLANT_SEED( "ACT_PLANT_SEED" );
static const activity_id ACT_PULL_CREATURE( "ACT_PULL_CREATURE" );
static const activity_id ACT_REPAIR_ITEM( "ACT_REPAIR_ITEM" );
static const activity_id ACT_ROBOT_CONTROL( "ACT_ROBOT_CONTROL" );
static const activity_id ACT_SOCIALIZE( "ACT_SOCIALIZE" );
static const activity_id ACT_SPELLCASTING( "ACT_SPELLCASTING" );
static const activity_id ACT_START_ENGINES( "ACT_START_ENGINES" );
static const activity_id ACT_START_FIRE( "ACT_START_FIRE" );
static const activity_id ACT_STUDY_SPELL( "ACT_STUDY_SPELL" );
static const activity_id ACT_TIDY_UP( "ACT_TIDY_UP" );
static const activity_id ACT_TOOLMOD_ADD( "ACT_TOOLMOD_ADD" );
static const activity_id ACT_TRAIN( "ACT_TRAIN" );
static const activity_id ACT_TRAIN_TEACHER( "ACT_TRAIN_TEACHER" );
static const activity_id ACT_TRAVELLING( "ACT_TRAVELLING" );
static const activity_id ACT_TREE_COMMUNION( "ACT_TREE_COMMUNION" );
static const activity_id ACT_VEHICLE( "ACT_VEHICLE" );
static const activity_id ACT_VEHICLE_DECONSTRUCTION( "ACT_VEHICLE_DECONSTRUCTION" );
static const activity_id ACT_VEHICLE_REPAIR( "ACT_VEHICLE_REPAIR" );
static const activity_id ACT_VIBE( "ACT_VIBE" );
static const activity_id ACT_WAIT( "ACT_WAIT" );
static const activity_id ACT_WAIT_NPC( "ACT_WAIT_NPC" );
static const activity_id ACT_WAIT_WEATHER( "ACT_WAIT_WEATHER" );

static const ammotype ammo_battery( "battery" );

static const bionic_id bio_painkiller( "bio_painkiller" );

static const efftype_id effect_asocial_dissatisfied( "asocial_dissatisfied" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_blind( "blind" );
static const efftype_id effect_controlled( "controlled" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_social_dissatisfied( "social_dissatisfied" );
static const efftype_id effect_social_satisfied( "social_satisfied" );
static const efftype_id effect_under_operation( "under_operation" );

static const flag_id json_flag_IRREMOVABLE( "IRREMOVABLE" );
static const flag_id json_flag_PSEUDO( "PSEUDO" );

static const furn_str_id furn_f_compost_empty( "f_compost_empty" );
static const furn_str_id furn_f_compost_full( "f_compost_full" );
static const furn_str_id furn_f_fvat_empty( "f_fvat_empty" );
static const furn_str_id furn_f_fvat_wood_empty( "f_fvat_wood_empty" );
static const furn_str_id furn_f_fvat_wood_full( "f_fvat_wood_full" );
static const furn_str_id furn_f_kiln_empty( "f_kiln_empty" );
static const furn_str_id furn_f_kiln_metal_empty( "f_kiln_metal_empty" );
static const furn_str_id furn_f_kiln_portable_empty( "f_kiln_portable_empty" );
static const furn_str_id furn_f_metal_smoking_rack( "f_metal_smoking_rack" );
static const furn_str_id furn_f_plant_seed( "f_plant_seed" );
static const furn_str_id furn_f_smoking_rack( "f_smoking_rack" );

static const itype_id itype_animal( "animal" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_muscle( "muscle" );
static const itype_id itype_pseudo_magazine( "pseudo_magazine" );
static const itype_id itype_pseudo_magazine_mod( "pseudo_magazine_mod" );

static const json_character_flag json_flag_ASOCIAL1( "ASOCIAL1" );
static const json_character_flag json_flag_ASOCIAL2( "ASOCIAL2" );
static const json_character_flag json_flag_PAIN_IMMUNE( "PAIN_IMMUNE" );
static const json_character_flag json_flag_SILENT_SPELL( "SILENT_SPELL" );
static const json_character_flag json_flag_SOCIAL1( "SOCIAL1" );
static const json_character_flag json_flag_SOCIAL2( "SOCIAL2" );

static const mongroup_id GROUP_FISH( "GROUP_FISH" );

static const morale_type morale_feeling_good( "morale_feeling_good" );
static const morale_type morale_game( "morale_game" );
static const morale_type morale_tree_communion( "morale_tree_communion" );

static const quality_id qual_FISHING_ROD( "FISHING_ROD" );

static const skill_id skill_computer( "computer" );
static const skill_id skill_survival( "survival" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_tree( "t_tree" );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );
static const trait_id trait_SPIRITUAL( "SPIRITUAL" );
static const trait_id trait_STOCKY_TROGLO( "STOCKY_TROGLO" );

static const zone_type_id zone_type_FARM_PLOT( "FARM_PLOT" );

using namespace activity_handlers;

const std::map< activity_id, std::function<void( player_activity *, Character * )> >
activity_handlers::do_turn_functions = {
    { ACT_FILL_LIQUID, fill_liquid_do_turn },
    { ACT_PICKAXE, pickaxe_do_turn },
    { ACT_GAME, game_do_turn },
    { ACT_GENERIC_GAME, generic_game_do_turn },
    { ACT_START_FIRE, start_fire_do_turn },
    { ACT_VIBE, vibe_do_turn },
    { ACT_HAND_CRANK, hand_crank_do_turn },
    { ACT_MULTIPLE_FISH, multiple_fish_do_turn },
    { ACT_MULTIPLE_CONSTRUCTION, multiple_construction_do_turn },
    { ACT_MULTIPLE_MINE, multiple_mine_do_turn },
    { ACT_MULTIPLE_MOP, multiple_mop_do_turn },
    { ACT_MULTIPLE_BUTCHER, multiple_butcher_do_turn },
    { ACT_MULTIPLE_FARM, multiple_farm_do_turn },
    { ACT_FETCH_REQUIRED, fetch_do_turn },
    { ACT_BUILD, build_do_turn },
    { ACT_EAT_MENU, eat_menu_do_turn },
    { ACT_VEHICLE_DECONSTRUCTION, vehicle_deconstruction_do_turn },
    { ACT_VEHICLE_REPAIR, vehicle_repair_do_turn },
    { ACT_MULTIPLE_CHOP_TREES, chop_trees_do_turn },
    { ACT_CONSUME_FOOD_MENU, consume_food_menu_do_turn },
    { ACT_CONSUME_DRINK_MENU, consume_drink_menu_do_turn },
    { ACT_CONSUME_MEDS_MENU, consume_meds_menu_do_turn },
    { ACT_MOVE_LOOT, move_loot_do_turn },
    { ACT_ARMOR_LAYERS, armor_layers_do_turn },
    { ACT_ATM, atm_do_turn },
    { ACT_FISH, fish_do_turn },
    { ACT_REPAIR_ITEM, repair_item_do_turn },
    { ACT_TRAVELLING, travel_do_turn },
    { ACT_DISMEMBER, dismember_do_turn },
    { ACT_TIDY_UP, tidy_up_do_turn },
    { ACT_TIDY_UP, tidy_up_do_turn },
    { ACT_JACKHAMMER, jackhammer_do_turn },
    { ACT_FIND_MOUNT, find_mount_do_turn },
    { ACT_MULTIPLE_CHOP_PLANKS, multiple_chop_planks_do_turn },
    { ACT_FERTILIZE_PLOT, fertilize_plot_do_turn },
    { ACT_OPERATION, operation_do_turn },
    { ACT_ROBOT_CONTROL, robot_control_do_turn },
    { ACT_TREE_COMMUNION, tree_communion_do_turn },
    { ACT_STUDY_SPELL, study_spell_do_turn },
    { ACT_MULTIPLE_CRAFT, multiple_craft_do_turn },
    { ACT_MULTIPLE_DIS, multiple_dis_do_turn },
    { ACT_MULTIPLE_READ, multiple_read_do_turn },
};

const std::map< activity_id, std::function<void( player_activity *, Character * )> >
activity_handlers::finish_functions = {
    { ACT_FISH, fish_finish },
    { ACT_PICKAXE, pickaxe_finish },
    { ACT_START_FIRE, start_fire_finish },
    { ACT_GENERIC_GAME, generic_game_finish },
    { ACT_TRAIN, train_finish },
    { ACT_TRAIN_TEACHER, teach_finish },
    { ACT_PLANT_SEED, plant_seed_finish },
    { ACT_VEHICLE, vehicle_finish },
    { ACT_START_ENGINES, start_engines_finish },
    { ACT_REPAIR_ITEM, repair_item_finish },
    { ACT_HEATING, heat_item_finish },
    { ACT_MEND_ITEM, mend_item_finish },
    { ACT_TOOLMOD_ADD, toolmod_add_finish },
    { ACT_WAIT, wait_finish },
    { ACT_WAIT_WEATHER, wait_weather_finish },
    { ACT_WAIT_NPC, wait_npc_finish },
    { ACT_SOCIALIZE, socialize_finish },
    { ACT_OPERATION, operation_finish },
    { ACT_VIBE, vibe_finish },
    { ACT_ATM, atm_finish },
    { ACT_EAT_MENU, eat_menu_finish },
    { ACT_CONSUME_FOOD_MENU, eat_menu_finish },
    { ACT_CONSUME_DRINK_MENU, eat_menu_finish },
    { ACT_CONSUME_MEDS_MENU, eat_menu_finish },
    { ACT_JACKHAMMER, jackhammer_finish },
    { ACT_ROBOT_CONTROL, robot_control_finish },
    { ACT_PULL_CREATURE, pull_creature_finish },
    { ACT_SPELLCASTING, spellcasting_finish },
    { ACT_STUDY_SPELL, study_spell_finish },
};

static void assign_multi_activity( Character &you, const player_activity &act )
{
    const bool requires_actor = activity_actors::deserialize_functions.find( act.id() ) !=
                                activity_actors::deserialize_functions.end();
    if( requires_actor ) {
        // the activity uses `activity_actor` and requires `player_activity::actor` to be set
        you.assign_activity( player_activity( act ) );
    } else {
        // the activity uses the older type of player_activity where `player_activity::actor` is not used
        you.assign_activity( act.id() );
    }
}

bool activity_handlers::resume_for_multi_activities( Character &you )
{
    if( !you.backlog.empty() ) {
        player_activity &back_act = you.backlog.front();
        if( back_act.is_multi_type() ) {
            assign_multi_activity( you, back_act );
            you.backlog.pop_front();
            return true;
        }
    }
    return false;
}

void activity_handlers::fill_liquid_do_turn( player_activity *act, Character *you )
{
    player_activity &act_ref = *act;
    try {
        // 1. Gather the source item.
        vehicle *source_veh = nullptr;
        map &here = get_map();
        const tripoint_bub_ms source_pos = here.get_bub( act_ref.coords.at( 0 ) );
        map_stack source_stack = here.i_at( source_pos );
        map_stack::iterator on_ground;
        monster *source_mon = nullptr;
        item liquid;
        const liquid_source_type source_type = static_cast<liquid_source_type>( act_ref.values.at( 0 ) );
        int part_num = -1;
        int veh_charges = 0;
        switch( source_type ) {
            case liquid_source_type::VEHICLE:
                source_veh = veh_pointer_or_null( here.veh_at( source_pos ) );
                if( source_veh == nullptr ) {
                    throw std::runtime_error( "could not find source vehicle for liquid transfer" );
                }
                deserialize_from_string( liquid, act_ref.str_values.at( 0 ) );
                part_num = static_cast<int>( act_ref.values.at( 1 ) );
                veh_charges = liquid.charges;
                break;
            case liquid_source_type::INFINITE_MAP:
                deserialize_from_string( liquid, act_ref.str_values.at( 0 ) );
                liquid.charges = item::INFINITE_CHARGES;
                break;
            case liquid_source_type::MAP_ITEM:
                if( static_cast<size_t>( act_ref.values.at( 1 ) ) >= source_stack.size() ) {
                    throw std::runtime_error( "could not find source item on ground for liquid transfer" );
                }
                on_ground = source_stack.begin();
                std::advance( on_ground, act_ref.values.at( 1 ) );
                liquid = *on_ground;
                break;
            case liquid_source_type::MONSTER:
                Creature *c = get_creature_tracker().creature_at( source_pos );
                source_mon = dynamic_cast<monster *>( c );
                if( source_mon == nullptr ) {
                    debugmsg( "could not find source creature for liquid transfer" );
                    act_ref.set_to_null();
                }
                deserialize_from_string( liquid, act_ref.str_values.at( 0 ) );
                liquid.charges = 1;
                break;
        }

        static const units::volume volume_per_second = units::from_liter( 4.0F / 6.0F );
        const int charges_per_second = std::max( 1, liquid.charges_per_volume( volume_per_second ) );
        liquid.charges = std::min( charges_per_second, liquid.charges );
        const int original_charges = liquid.charges;
        if( liquid.has_temperature() && units::to_joule_per_gram( liquid.specific_energy ) < 0 ) {
            liquid.set_item_temperature( std::max( get_weather().get_temperature( you->pos_bub() ),
                                                   temperatures::cold ) );
        }

        // 2. Transfer charges.
        const vehicle *veh = nullptr;
        size_t part;
        switch( static_cast<liquid_target_type>( act_ref.values.at( 2 ) ) ) {
            case liquid_target_type::VEHICLE: {
                const optional_vpart_position vp = here.veh_at( here.get_bub( act_ref.coords.at( 1 ) ) );
                if( act_ref.values.size() > 4 && vp ) {
                    const vpart_reference vpr( vp->vehicle(), act_ref.values[4] );
                    veh = &vp->vehicle();
                    part = act_ref.values[4];
                    if( source_veh &&
                        source_veh->fuel_left( here, liquid.typeId(), ( veh ? std::function<bool( const vehicle_part & )> { [&]( const vehicle_part & pa )
                {
                    return &veh->part( part ) != &pa;
                    }
                                                                                                                          } : return_true<const vehicle_part &> ) ) <= 0 ) {
                        act_ref.set_to_null();
                        return;
                    }
                    you->pour_into( vpr, liquid );

                } else {
                    throw std::runtime_error( "could not find target vehicle for liquid transfer" );
                }
                break;
            }
            case liquid_target_type::CONTAINER:
                you->pour_into( act_ref.targets.at( 0 ), liquid, true, true );
                break;
            case liquid_target_type::MAP:
                if( iexamine::has_keg( here.get_bub( act_ref.coords.at( 1 ) ) ) ) {
                    iexamine::pour_into_keg( here.get_bub( act_ref.coords.at( 1 ) ), liquid, false );
                } else {
                    here.add_item_or_charges( here.get_bub( act_ref.coords.at( 1 ) ), liquid );
                    you->add_msg_if_player( _( "You pour %1$s onto the ground." ), liquid.tname() );
                    liquid.charges = 0;
                }
                break;
            case liquid_target_type::MONSTER:
                liquid.charges = 0;
                break;
        }

        const int removed_charges = original_charges - liquid.charges;
        if( removed_charges == 0 ) {
            // Nothing has been transferred, target must be full.
            act_ref.set_to_null();
            return;
        }

        // 3. Remove charges from source.
        switch( source_type ) {
            case liquid_source_type::VEHICLE:
                if( part_num != -1 ) {
                    const vehicle_part &pt = source_veh->part( part_num );
                    if( pt.is_leaking() && !pt.ammo_remaining( ) ) {
                        act_ref.set_to_null(); // leaky tank spilled while we were transferring
                        return;
                    }
                    source_veh->drain( here, part_num, removed_charges );
                    liquid.charges = veh_charges - removed_charges;
                    // If there's no liquid left in this tank we're done, otherwise
                    // we need to update our liquid serialization to reflect how
                    // many charges are actually left for the next time we come
                    // around this loop.
                    if( !liquid.charges ) {
                        act_ref.set_to_null();
                    } else {
                        if( act_ref.str_values.empty() ) {
                            act_ref.str_values.emplace_back( );
                        }
                        act_ref.str_values[0] = serialize( liquid );
                    }
                } else {
                    source_veh->drain( here, liquid.typeId(), removed_charges,
                                       ( veh ? std::function<bool( vehicle_part & )> { [&]( vehicle_part & pa )
                    {
                        return &veh->part( part ) != &pa;
                    }
                                                                                     } : return_true<vehicle_part &> ) );
                }
                break;
            case liquid_source_type::MAP_ITEM:
                on_ground->charges -= removed_charges;
                if( on_ground->charges <= 0 ) {
                    source_stack.erase( on_ground );
                    if( here.ter( source_pos )->has_examine( iexamine::gaspump ) ) {
                        add_msg( _( "With a clang and a shudder, the %s pump goes silent." ),
                                 liquid.type_name( 1 ) );
                    } else if( const furn_id &f = here.furn( source_pos ); f->has_examine( iexamine::fvat_full ) ) {
                        add_msg( _( "You squeeze the last drops of %s from the vat." ),
                                 liquid.type_name( 1 ) );
                        map_stack items_here = here.i_at( source_pos );
                        if( items_here.empty() ) {
                            if( f == furn_f_fvat_wood_full ) {
                                here.furn_set( source_pos, furn_f_fvat_wood_empty );
                            } else {
                                here.furn_set( source_pos, furn_f_fvat_empty );
                            }
                        }
                    } else if( f->has_examine( iexamine::compost_full ) ) {
                        add_msg( _( "You squeeze the last drops of %s from the tank." ),
                                 liquid.type_name( 1 ) );
                        map_stack items_here = here.i_at( source_pos );
                        if( items_here.empty() ) {
                            if( f == furn_f_compost_full ) {
                                here.furn_set( source_pos, furn_f_compost_empty );
                            }
                        }
                    }
                    act_ref.set_to_null();
                }
                break;
            case liquid_source_type::INFINITE_MAP:
                // nothing, the liquid source is infinite
                break;
            case liquid_source_type::MONSTER:
                // liquid source charges handled in monexamine::milk_source
                if( liquid.charges == 0 ) {
                    act_ref.set_to_null();
                }
                break;
        }

        if( removed_charges < original_charges ) {
            // Transferred less than the available charges -> target must be full
            act_ref.set_to_null();
        }

    } catch( const std::runtime_error &err ) {
        debugmsg( "error in activity data: \"%s\"", err.what() );
        act_ref.set_to_null();
        return;
    }
}

// Repurposing the activity's index to convey the number of friends participating
void activity_handlers::generic_game_turn_handler( player_activity *act, Character *you,
        int morale_bonus, int morale_max_bonus )
{
    // Consume battery charges for every minute spent playing
    if( calendar::once_every( 1_minutes ) ) {
        if( !act->targets.empty() ) {
            item &game_item = *act->targets.front();
            int req = game_item.ammo_required();
            bool fail = req > 0 && game_item.ammo_consume( req, tripoint_bub_ms::zero, you ) == 0;
            if( fail ) {
                act->moves_left = 0;
                if( you->is_avatar() ) {
                    add_msg( m_info, _( "The %s runs out of batteries." ), game_item.tname() );
                }
                return;
            }
        }
        if( act->index > 0 && act->name.find( "with friends" ) != std::string::npos ) {
            // 1 friend -> x1.2,  2 friends -> x1.4,  3 friends -> x1.6  ...
            float mod = std::sqrt( ( act->index * 0.5f ) + 0.5f ) + 0.2f;
            morale_bonus = std::ceil( morale_bonus * mod );
            // half mult for max bonus
            mod = 1.f + ( mod - 1.f ) * 0.5f;
            morale_max_bonus *= mod;
        }
        // Playing alone - 1 points/min, almost 2 hours to fill
        you->add_morale( morale_game, morale_bonus, morale_max_bonus );
    }
}

// Repurposing the activity's index to convey the number of friends participating
void activity_handlers::generic_game_finish( player_activity *act, Character *you )
{
    // Apply small bonus with diminishing returns for playing with friends
    if( act->index > 0 && act->name.find( "with friends" ) != std::string::npos ) {
        float mod = 1.f;
        float acc = 0.4f;
        for( int i = act->index; i > 0; i-- ) {
            mod += acc;
            acc *= acc;
        }
        if( !act->values.empty() && act->values[0] == you->getID().get_value() ) {
            // A winner is you! Feel more happy!
            mod *= 1.5f;
            you->add_msg_if_player( m_good, _( "You won!" ) );
            you->add_msg_if_npc( m_good, _( "<npcname> won!" ) );
        }
        you->add_morale( morale_game, 4 * mod );
    }
    act->set_to_null();
}

void activity_handlers::generic_game_do_turn( player_activity *act, Character *you )
{
    generic_game_turn_handler( act, you, 4, 60 );
}

void activity_handlers::game_do_turn( player_activity *act, Character *you )
{
    generic_game_turn_handler( act, you, 1, 100 );
}

void activity_handlers::pickaxe_do_turn( player_activity *act, Character * )
{
    const tripoint_bub_ms &pos = get_map().get_bub( act->placement );
    sfx::play_activity_sound( "tool", "pickaxe", sfx::get_heard_volume( pos ) );
    // each turn is too much
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a Pickaxe at work!
        sounds::sound( pos, 30, sounds::sound_t::destructive_activity, _( "CHNK!  CHNK!  CHNK!" ) );
    }
}

void activity_handlers::pickaxe_finish( player_activity *act, Character *you )
{
    map &here = get_map();
    const tripoint_bub_ms pos( here.get_bub( act->placement ) );
    // Invalidate the activity early to prevent a query from mod_pain()
    act->set_to_null();
    if( you->is_avatar() ) {
        const int helpersize = get_player_character().get_num_crafting_helpers( 3 );
        if( here.is_bashable( pos ) && here.has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF, pos ) &&
            here.ter( pos ) != ter_t_tree ) {
            // Tunneling through solid rock is sweaty, backbreaking work
            // Betcha wish you'd opted for the J-Hammer
            if( you->has_trait( trait_STOCKY_TROGLO ) ) {
                you->mod_pain( std::max( 0, ( 1 * static_cast<int>( rng( 0, 3 ) ) ) - helpersize ) );
            } else {
                you->mod_pain( std::max( 0, ( 2 * static_cast<int>( rng( 1, 3 ) ) ) - helpersize ) );
            }
        }
    }
    you->add_msg_player_or_npc( m_good,
                                _( "You finish digging." ),
                                _( "<npcname> finishes digging." ) );
    here.destroy( pos, true );
    if( !act->targets.empty() ) {
        item_location it = act->targets.front();
        if( it ) {
            you->consume_charges( *it, it->ammo_required() );
        }
    } else {
        debugmsg( "pickaxe activity targets empty" );
    }
    if( resume_for_multi_activities( *you ) ) {
        for( item &elem : here.i_at( pos ) ) {
            elem.set_var( "activity_var", you->name );
            you->may_activity_occupancy_after_end_items_loc.emplace_back( map_cursor{here.get_abs( pos )},
                    &elem );
        }
    }
}

void activity_handlers::start_fire_finish( player_activity *act, Character *you )
{
    map &here = get_map();

    static const std::string iuse_name_string( "firestarter" );

    item &it = *act->targets.front();
    item *used_tool = it.get_usable_item( iuse_name_string );
    if( used_tool == nullptr ) {
        debugmsg( "Lost tool used for starting fire" );
        act->set_to_null();
        return;
    }

    const use_function *use_fun = used_tool->get_use( iuse_name_string );
    const firestarter_actor *actor = dynamic_cast<const firestarter_actor *>
                                     ( use_fun->get_actor_ptr() );
    if( actor == nullptr ) {
        debugmsg( "iuse_actor type descriptor and actual type mismatch" );
        act->set_to_null();
        return;
    }

    you->practice( skill_survival, act->index, 5 );

    const furn_id &f_id = here.furn( here.get_bub( act->placement ) );
    const bool is_smoking_rack = f_id == furn_f_metal_smoking_rack ||
                                 f_id == furn_f_smoking_rack;
    const bool is_kiln = f_id == furn_f_kiln_empty ||
                         f_id == furn_f_kiln_metal_empty || f_id == furn_f_kiln_portable_empty;

    firestarter_actor::start_type st = firestarter_actor::start_type::FIRE;

    if( is_smoking_rack ) {
        st = firestarter_actor::start_type::SMOKER;
    } else if( is_kiln ) {
        st = firestarter_actor::start_type::KILN;
    }

    it.activation_consume( 1, you->pos_bub(), you );

    firestarter_actor::resolve_firestarter_use( you, &here, here.get_bub( act->placement ),
            st );

    act->set_to_null();
}

void activity_handlers::start_fire_do_turn( player_activity *act, Character *you )
{
    map &here = get_map();
    tripoint_bub_ms where = here.get_bub( act->placement );
    if( !here.is_flammable( where ) ) {
        try_fuel_fire( *act, *you, true );
        if( !here.is_flammable( where ) ) {
            if( here.has_flag_ter( ter_furn_flag::TFLAG_DEEP_WATER, where ) ||
                here.has_flag_ter( ter_furn_flag::TFLAG_SHALLOW_WATER, where ) ) {
                you->add_msg_if_player( m_info, _( "You can't light a fire on water." ) );
            } else {
                you->add_msg_if_player( m_info, _( "There's nothing to light there." ) );
            }
            you->cancel_activity();
            return;
        }
    }

    // Sometimes when an item is dropped it causes the whole item* to get set to null.
    // This null pointer gets cast to a reference at some point, causing UB and
    // segfaults. It looks like something is supposed to catch this, maybe
    // get_safe_reference in item.cpp, but it's not working so we check for a
    // null pointer here.
    //
    if( act->targets.front().get_item() == nullptr ) {
        add_msg( m_bad, _( "You have lost the item you were using to start the fire." ) );
        you->cancel_activity();
        return;
    }

    item &firestarter = *act->targets.front();

    const furn_id f_id = here.furn( here.get_bub( act->placement ) );
    const bool is_smoker = f_id == furn_f_smoking_rack ||
                           f_id == furn_f_metal_smoking_rack;

    if( firestarter.has_flag( flag_REQUIRES_TINDER ) && !is_smoker ) {
        if( !here.tinder_at( where ) ) {
            inventory_filter_preset preset( []( const item_location & loc ) {
                return loc->has_flag( flag_TINDER );
            } );
            inventory_pick_selector inv_s( *you, preset );
            inv_s.add_nearby_items( PICKUP_RANGE );
            inv_s.add_character_items( *you );

            inv_s.set_title( _( "Select tinder to use for lighting a fire" ) );

            item_location tinder;
            if( inv_s.empty() || !( tinder = inv_s.execute() ) ) {
                you->add_msg_if_player( m_info, _( "This item requires tinder to light." ) );
                you->cancel_activity();
                return;
            }

            item copy = *tinder;
            bool count_by_charges = tinder->count_by_charges();
            if( count_by_charges ) {
                tinder->charges--;
                copy.charges = 1;
            }
            here.add_item_or_charges( where, copy );
            if( !count_by_charges || tinder->charges <= 0 ) {
                tinder.remove_item();
            }
        }
    }

    const use_function *usef = firestarter.type->get_use( "firestarter" );
    if( usef == nullptr || usef->get_actor_ptr() == nullptr ) {
        add_msg( m_bad, _( "You have lost the item you were using to start the fire." ) );
        you->cancel_activity();
        return;
    }

    you->mod_moves( -you->get_moves() );
    const firestarter_actor *actor = dynamic_cast<const firestarter_actor *>( usef->get_actor_ptr() );
    const float light = actor->light_mod( &here, you->pos_bub( here ) );
    act->moves_left -= light * 100;
    if( light < 0.1 ) {
        add_msg( m_bad, _( "There is not enough sunlight to start a fire now.  You stop trying." ) );
        you->cancel_activity();
    }
}

static bool magic_train( player_activity *act, Character *you )
{
    if( !you ) {
        return false;
    }
    const spell_id &sp_id = spell_id( act->name );
    if( sp_id.is_valid() ) {
        const bool knows = you->magic->knows_spell( sp_id );
        if( knows ) {
            spell &studying = you->magic->get_spell( sp_id );
            const int expert_multiplier = act->values.empty() ? 0 : act->values[0];
            const int xp = roll_remainder( studying.exp_modifier( *you ) * expert_multiplier );
            studying.gain_exp( *you, xp );
            you->add_msg_player_or_npc( m_good, _( "You learn a little about the spell: %s" ),
                                        _( "<npcname> learns a little about the spell: %s" ), sp_id->name );
        } else {
            you->magic->learn_spell( act->name, *you );
            // you can decline to learn this spell , as it may lock you out of other magic.
            if( you->magic->knows_spell( sp_id ) ) {
                you->add_msg_player_or_npc( m_good, _( "You learn %s." ),
                                            _( "<npcname> learns %s." ), sp_id->name.translated() );
            } else {
                act->set_to_null();
            }
        }
        return true;
    }
    return false;
}

void activity_handlers::teach_finish( player_activity *act, Character *you )
{
    const skill_id sk( act->name );
    const proficiency_id pr( act->name );
    const matype_id ma( act->name );
    const spell_id sp( act->name );

    std::string subject;
    if( sk.is_valid() ) {
        subject = sk->name();
    } else if( pr.is_valid() ) {
        subject = pr->name();
    } else if( ma.is_valid() ) {
        subject = ma->name.translated();
    } else if( sp.is_valid() ) {
        subject = sp->name.translated();
    } else {
        debugmsg( "teach_finish without a valid skill or style or spell name" );
    }

    if( you->is_avatar() ) {
        add_msg( m_good, _( "You finish teaching %s." ), subject );
    } else {
        add_msg( m_good, _( "%s finishes teaching %s." ), you->name, subject );
    }

    act->set_to_null();
}

void activity_handlers::train_finish( player_activity *act, Character *you )
{
    const std::vector<npc *> teachlist = g->get_npcs_if( [act]( const npc & n ) {
        return n.getID().get_value() == act->index;
    } );
    Character *teacher = &get_player_character();
    if( !teachlist.empty() ) {
        teacher = teachlist.front();
    }
    if( teacher->activity.id() == ACT_TRAIN_TEACHER ) {
        bool all_students_done = true;
        g->get_npcs_if( [&]( const npc & n ) {
            for( int st_id : teacher->activity.values ) {
                if( n.getID().get_value() == st_id && n.activity.id() == ACT_TRAIN ) {
                    all_students_done = false;
                    break;
                }
            }
            return false;
        } );
        if( all_students_done ) {
            teacher->cancel_activity();
        }
    }

    const skill_id sk( act->name );
    if( sk.is_valid() ) {
        const Skill &skill = sk.obj();
        std::string skill_name = skill.name();
        int old_skill_level = you->get_knowledge_level( sk );
        you->practice( sk, 100, old_skill_level + 2 );
        int new_skill_level = you->get_knowledge_level( sk );
        if( old_skill_level != new_skill_level ) {
            if( you->is_avatar() ) {
                add_msg( m_good, _( "You finish training %s to level %d." ),
                         skill_name, new_skill_level );
            }
            get_event_bus().send<event_type::gains_skill_level>( you->getID(), sk, new_skill_level );
        } else if( you->is_avatar() ) {
            add_msg( m_good, _( "You get some training in %s." ), skill_name );
        }
        act->set_to_null();
        return;
    }

    const proficiency_id &proficiency = proficiency_id( act->name );
    if( proficiency.is_valid() ) {
        you->practice_proficiency( proficiency, 15_minutes );
        if( you->is_avatar() ) {
            add_msg( m_good, _( "You get some training in %s." ), proficiency->name() );
        }
        act->set_to_null();
        return;
    }

    const matype_id &ma_id = matype_id( act->name );
    if( ma_id.is_valid() ) {
        const martialart &mastyle = ma_id.obj();
        // Trained martial arts,
        get_event_bus().send<event_type::learns_martial_art>( you->getID(), ma_id );
        you->martial_arts_data->learn_style( mastyle.id, you->is_avatar() );
    } else if( !magic_train( act, you ) ) {
        debugmsg( "train_finish without a valid skill or style or spell name" );
    }

    act->set_to_null();
}

void activity_handlers::vehicle_finish( player_activity *act, Character *you )
{
    map &here = get_map();
    //Grab this now, in case the vehicle gets shifted
    const optional_vpart_position vp = here.veh_at( here.get_bub( tripoint_abs_ms( act->values[0],
                                       act->values[1],
                                       you->posz() ) ) );
    veh_interact::complete_vehicle( here, *you );
    // complete_vehicle set activity type to NULL if the vehicle
    // was completely dismantled, otherwise the vehicle still exist and
    // is to be examined again.
    if( act->is_null() ) {
        if( npc *guy = dynamic_cast<npc *>( you ) ) {
            guy->revert_after_activity();
            guy->set_moves( 0 );
        }
        return;
    }
    act->set_to_null();
    if( !you->is_npc() ) {
        if( act->values.size() < 7 ) {
            dbg( D_ERROR ) << "game:process_activity: invalid ACT_VEHICLE values: "
                           << act->values.size();
            debugmsg( "process_activity invalid ACT_VEHICLE values:%d",
                      act->values.size() );
        } else {
            if( vp ) {
                here.invalidate_map_cache( here.get_abs_sub().z() );
                // TODO: Z (and also where the activity is queued)
                // Or not, because the vehicle coordinates are dropped anyway
                if( !resume_for_multi_activities( *you ) ) {
                    point_rel_ms int_p( act->values[ 2 ], act->values[ 3 ] );
                    if( vp->vehicle().is_appliance() ) {
                        g->exam_appliance( vp->vehicle(), int_p );
                    } else {
                        g->exam_vehicle( vp->vehicle(), int_p );
                    }
                }
                return;
            } else {
                dbg( D_ERROR ) << "game:process_activity: ACT_VEHICLE: vehicle not found";
                debugmsg( "process_activity ACT_VEHICLE: vehicle not found" );
            }
        }
    }
}

void activity_handlers::hand_crank_do_turn( player_activity *act, Character *you )
{
    // Hand-crank chargers seem to range from 2 watt (very common easily verified)
    // to 10 watt (suspicious claims from some manufacturers) sustained output.
    // It takes 2.4 minutes to produce 1kj at just slightly under 7 watts (25 kj per hour)
    // time-based instead of speed based because it's a sustained activity
    item &hand_crank_item = *act->targets.front();

    int time_to_crank = to_seconds<int>( 144_seconds );
    // Modify for weariness
    time_to_crank /= you->exertion_adjusted_move_multiplier( act->exertion_level() );
    if( calendar::once_every( time_duration::from_seconds( time_to_crank ) ) ) {
        if( hand_crank_item.ammo_capacity( ammo_battery ) > hand_crank_item.ammo_remaining( ) ) {
            hand_crank_item.ammo_set( itype_battery, hand_crank_item.ammo_remaining( ) + 1 );
        } else {
            act->moves_left = 0;
            add_msg( m_info, _( "You've charged the battery completely." ) );
        }
    }
    if( you->get_sleepiness() >= sleepiness_levels::DEAD_TIRED ) {
        act->moves_left = 0;
        add_msg( m_info, _( "You're too exhausted to keep cranking." ) );
    }

}

void activity_handlers::vibe_do_turn( player_activity *act, Character *you )
{
    //Using a vibrator takes time (10 minutes), not speed
    //Linear increase in morale during action with a small boost at end
    //Deduct 1 battery charge for every minute in use, or vibrator is much less effective
    item &vibrator_item = *act->targets.front();

    if( you->encumb( bodypart_id( "mouth" ) ) >= 30 ) {
        act->moves_left = 0;
        add_msg( m_bad, _( "You have trouble breathing, and stop." ) );
    }

    if( calendar::once_every( 1_minutes ) ) {
        if( vibrator_item.ammo_remaining( you ) > 0 ) {
            vibrator_item.ammo_consume( 1, you->pos_bub(), you );
            you->add_morale( morale_feeling_good, 3, 40 );
            if( vibrator_item.ammo_remaining( you ) == 0 ) {
                add_msg( m_info, _( "The %s runs out of batteries." ), vibrator_item.tname() );
            }
        } else {
            //twenty minutes to fill
            you->add_morale( morale_feeling_good, 1, 40 );
        }
    }
    // Dead Tired: different kind of relaxation needed
    if( you->get_sleepiness() >= sleepiness_levels::DEAD_TIRED ) {
        act->moves_left = 0;
        add_msg( m_info, _( "You're too tired to continue." ) );
    }

    // Vibrator requires that you be able to move around, stretch, etc, so doesn't play
    // well with roots.  Sorry.  :-(
}

void activity_handlers::start_engines_finish( player_activity *act, Character *you )
{
    act->set_to_null();
    // Find the vehicle by looking for a remote vehicle first, then at
    // act->relative_placement away from avatar
    vehicle *veh = g->remoteveh();
    map &here = get_map();
    if( !veh ) {
        const tripoint_bub_ms pos = get_player_character().pos_bub() + act->relative_placement;
        veh = veh_pointer_or_null( here.veh_at( pos ) );
        if( !veh ) {
            return;
        }
    }

    int attempted = 0;
    int non_muscle_attempted = 0;
    int started = 0;
    int non_muscle_started = 0;
    int non_combustion_started = 0;
    const bool take_control = act->values[0];

    for( const int p : veh->engines ) {
        vehicle_part &vp = veh->part( p );
        if( veh->is_engine_on( vp ) ) {
            attempted++;
            if( !veh->is_engine_type( vp, itype_muscle ) &&
                !veh->is_engine_type( vp, itype_animal ) ) {
                non_muscle_attempted++;
            }
            if( veh->start_engine( here, vp ) ) {
                started++;
                if( !veh->is_engine_type( vp, itype_muscle ) &&
                    !veh->is_engine_type( vp, itype_animal ) ) {
                    non_muscle_started++;
                } else {
                    non_combustion_started++;
                }
            }
        }
    }

    //Did any engines start?
    veh->engine_on = started;
    //init working engine noise
    sfx::do_vehicle_engine_sfx();

    if( attempted == 0 ) {
        you->add_msg_if_player( m_info, _( "The %s doesn't have an engine!" ), veh->name );
    } else if( non_muscle_attempted > 0 ) {
        //Some non-muscle engines tried to start
        if( non_muscle_attempted == non_muscle_started ) {
            //All of the non-muscle engines started
            add_msg_if_player_sees( you->pos_bub(), n_gettext( "The %s's engine starts up.",
                                    "The %s's engines start up.", non_muscle_started ), veh->name );
        } else if( non_muscle_started > 0 ) {
            //Only some of the non-muscle engines started
            add_msg_if_player_sees( you->pos_bub(), n_gettext( "One of the %s's engines start up.",
                                    "Some of the %s's engines start up.", non_muscle_started ), veh->name );
        } else if( non_combustion_started > 0 ) {
            //Non-combustions "engines" started
            you->add_msg_if_player( _( "The %s is ready for movement." ), veh->name );
        } else {
            //All of the non-muscle engines failed
            if( you->is_avatar() ) {
                add_msg( m_bad, n_gettext( "The %s's engine fails to start.",
                                           "The %s's engines fail to start.", non_muscle_attempted ), veh->name );
            } else {
                add_msg_if_player_sees( you->pos_bub(), n_gettext( "The %s's engine fails to start.",
                                        "The %s's engines fail to start.", non_muscle_attempted ), veh->name );
            }
        }
    }

    if( take_control && !veh->engine_on && !veh->velocity ) {
        you->controlling_vehicle = false;
        you->add_msg_if_player( _( "You let go of the controls." ) );
    }
}

enum class repeat_type : int {
    // INIT should be zero. In some scenarios (vehicle welder), activity value default to zero.
    INIT = 0,       // Haven't found repeat value yet.
    ONCE = 1,       // Attempt repair (and refit if possible) just once
    // value 2 obsolete - previously used for reinforcement
    FULL = 3,       // Continue repairing until damage==0 (and until refitted if possible)
    EVENT = 4,      // Continue repairing (and refit if possible) until something interesting happens
    REFIT_ONCE = 5, // Try refitting once, but don't repair
    REFIT_FULL = 6, // Continue refitting until item fits, but don't repair
    CANCEL = 7,     // Stop repeating
};

using I = std::underlying_type_t<repeat_type>;
static constexpr bool operator>=( const I &lhs, const repeat_type &rhs )
{
    return lhs >= static_cast<I>( rhs );
}

static constexpr bool operator<=( const I &lhs, const repeat_type &rhs )
{
    return lhs <= static_cast<I>( rhs );
}

static constexpr I operator-( const repeat_type &lhs, const repeat_type &rhs )
{
    return static_cast<I>( lhs ) - static_cast<I>( rhs );
}

static repeat_type repeat_menu( const std::string &title, repeat_type last_selection,
                                const bool can_refit )
{
    uilist rmenu;
    rmenu.text = title;

    rmenu.addentry( static_cast<int>( repeat_type::ONCE ), true, '1',
                    can_refit ? _( "Attempt to refit or repair once" ) : _( "Attempt to repair once" ) );
    rmenu.addentry( static_cast<int>( repeat_type::FULL ), true, '2',
                    can_refit ? _( "Repeat until refitted and fully repaired" ) : _( "Repeat until fully repaired" ) );
    rmenu.addentry( static_cast<int>( repeat_type::EVENT ), true, '3',
                    can_refit ? _( "Refit or repair until success/failure/level up" ) :
                    _( "Repair until success/failure/level up" ) );
    rmenu.addentry( static_cast<int>( repeat_type::REFIT_ONCE ), can_refit, '4',
                    _( "Attempt to refit once" ) );
    rmenu.addentry( static_cast<int>( repeat_type::REFIT_FULL ), can_refit, '5',
                    _( "Repeat until refitted" ) );
    rmenu.addentry( static_cast<int>( repeat_type::INIT ), true, '6', _( "Back to item selection" ) );

    rmenu.selected = last_selection - repeat_type::ONCE;
    rmenu.query();

    if( rmenu.ret >= repeat_type::INIT && rmenu.ret <= repeat_type::REFIT_FULL ) {
        return static_cast<repeat_type>( rmenu.ret );
    }

    return repeat_type::CANCEL;
}

// HACK: This is a part of a hack to provide pseudo items for long repair activity
// Note: similar hack could be used to implement all sorts of vehicle pseudo-items
//  and possibly CBM pseudo-items too.
struct weldrig_hack {
    std::optional<vpart_reference> part;
    item pseudo;

    weldrig_hack() : part( std::nullopt ) { }

    bool init( const player_activity &act ) {
        map &here = get_map();
        if( act.coords.empty() || act.str_values.size() < 2 ) {
            return false;
        }

        const optional_vpart_position vp = here.veh_at( here.get_bub( act.coords[0] ) );
        if( !vp ) {
            return false;
        }

        itype_id tool_id( act.get_str_value( 1, "" ) );
        pseudo = item( tool_id, calendar::turn );
        part = vp->part_with_tool( here, tool_id );
        return part.has_value();
    }

    item &get_item() {
        map &here = get_map();

        if( !part ) {
            // null item should be handled just fine
            return null_item_reference();
        }
        pseudo.set_flag( json_flag_PSEUDO );
        item mag_mod( itype_pseudo_magazine_mod );
        mag_mod.set_flag( json_flag_IRREMOVABLE );
        if( !pseudo.put_in( mag_mod, pocket_type::MOD ).success() ) {
            debugmsg( "tool %s has no space for a %s, this is likely a bug",
                      pseudo.typeId().str(), mag_mod.type->nname( 1 ) );
        }
        itype_id mag_type;
        if( pseudo.can_link_up() ) {
            mag_type = itype_pseudo_magazine;
        } else {
            mag_type = pseudo.magazine_default();
        }
        item mag( mag_type );
        mag.clear_items(); // no initial ammo
        if( !pseudo.put_in( mag, pocket_type::MAGAZINE_WELL ).success() ) {
            debugmsg( "inserting %s into %s's MAGAZINE_WELL pocket failed",
                      mag.typeId().str(), pseudo.typeId().str() );
            return null_item_reference();
        }
        pseudo.ammo_set( itype_battery, part->vehicle().drain( here,  itype_battery,
                         pseudo.ammo_capacity( ammo_battery ),
                         return_true< vehicle_part &>, false ) ); // no cable loss since all of this is virtual
        return pseudo;
    }

    void clean_up() {
        if( !part ) {
            return;
        }

        map &here = get_map();

        part->vehicle().charge_battery( here, pseudo.ammo_remaining( ),
                                        false ); // return unused charges without cable loss
    }

    ~weldrig_hack() {
        clean_up();
    }
};

void activity_handlers::repair_item_finish( player_activity *act, Character *you )
{
    ::repair_item_finish( act, you, false );
}

void repair_item_finish( player_activity *act, Character *you, bool no_menu )
{
    map &here = get_map();
    const std::string iuse_name_string = act->get_str_value( 0, "repair_item" );
    repeat_type repeat = static_cast<repeat_type>( act->get_value( 0,
                         static_cast<int>( repeat_type::INIT ) ) );
    weldrig_hack w_hack;
    item_location *ploc = nullptr;

    if( !act->targets.empty() ) {
        ploc = act->targets.data();
    }
    item *main_tool = &( !w_hack.init( *act ) ?
                         ploc ?
                         **ploc : you->i_at( act->index ) : w_hack.get_item() );
    if( main_tool == nullptr ) {
        debugmsg( "Empty main tool for repair" );
        act->set_to_null();
        return;
    }
    item *used_tool = main_tool->get_usable_item( iuse_name_string );
    if( used_tool == nullptr ) {
        debugmsg( "Lost tool used for long repair" );
        act->set_to_null();
        return;
    }

    const use_function *use_fun = used_tool->get_use( iuse_name_string );
    // TODO: De-uglify this block. Something like get_use<iuse_actor_type>() maybe?
    const repair_item_actor *actor = dynamic_cast<const repair_item_actor *>
                                     ( use_fun->get_actor_ptr() );
    if( actor == nullptr ) {
        debugmsg( "iuse_actor type descriptor and actual type mismatch" );
        act->set_to_null();
        return;
    }

    // Valid Repeat choice and target, attempt repair.
    if( repeat != repeat_type::INIT && act->targets.size() >= 2 ) {
        item_location &fix_location = act->targets[1];
        if( !fix_location ) {
            // The item could disappear for various reasons: moved by follower, burned up, eaten by a grue, etc.
            you->add_msg_if_player( m_warning, _( "You can no longer find the item to repair." ) );
            act->set_to_null();
            return;
        }

        // Remember our level: we want to stop retrying on level up
        const int old_level = you->get_skill_level( actor->used_skill );
        const repair_item_actor::attempt_hint attempt = actor->repair( *you, *used_tool,
                fix_location, repeat == repeat_type::REFIT_ONCE || repeat == repeat_type::REFIT_FULL );
        // Warning: The above call to `repair_item_actor::repair` might
        // invalidate the item and the item_location, for example when
        // spilling items from spillable containers. It is therefore
        // important that we don't use `fix_location` in code below
        // here without first checking whether it is still valid.

        // If the item being repaired has been destroyed stop further
        // processing in case the items being used for the repair was
        // contained by the item being repaired, which will result
        // in the tool being invalid
        if( attempt == repair_item_actor::AS_DESTROYED ) {
            act->set_to_null();
            return;
        }

        if( attempt != repair_item_actor::AS_CANT ) {
            if( ploc && ploc->where() == item_location::type::map ) {
                used_tool->ammo_consume( used_tool->ammo_required(), ploc->pos_bub( here ), you );
            } else {
                you->consume_charges( *used_tool, used_tool->ammo_required() );
            }
        }

        // TODO: Allow setting this in the actor
        // TODO: Don't use charges_to_use: welder has 50 charges per use, soldering iron has 1
        if( !used_tool->ammo_sufficient( you ) ) {
            you->add_msg_if_player( _( "Your %1$s ran out of charges." ), used_tool->tname() );
            act->set_to_null();
            return;
        }

        // Print message explaining why we stopped
        // But only if we didn't destroy the item (because then it's obvious)
        const bool destroyed = attempt == repair_item_actor::AS_DESTROYED;
        const bool cannot_continue_repair = attempt == repair_item_actor::AS_CANT || destroyed ||
                                            !fix_location ||
                                            !actor->can_repair_target( *you, *fix_location, !destroyed, true );
        if( cannot_continue_repair ) {
            // Cannot continue to repair target, select another target.
            // **Warning**: as soon as the item is popped back, it is destroyed and can't be used anymore!
            act->targets.pop_back();
        }

        const bool event_happened = attempt == repair_item_actor::AS_FAILURE ||
                                    attempt == repair_item_actor::AS_SUCCESS ||
                                    old_level != static_cast<int>( you->get_skill_level( actor->used_skill ) );
        const bool can_refit = !destroyed && !cannot_continue_repair &&
                               fix_location->has_flag( flag_VARSIZE ) &&
                               !fix_location->has_flag( flag_FIT );

        const bool need_input =
            ( repeat == repeat_type::ONCE ) ||
            ( repeat == repeat_type::EVENT && event_happened ) ||
            ( repeat == repeat_type::FULL && cannot_continue_repair ) ||
            ( repeat == repeat_type::REFIT_ONCE ) ||
            ( repeat == repeat_type::REFIT_FULL && !can_refit );
        if( need_input ) {
            repeat = repeat_type::INIT;
            if( no_menu ) {
                act->set_to_null();
                return;
            }
        }
    }
    // Check tool is valid before we query target and Repeat choice.
    if( !actor->can_use_tool( *you, *used_tool, true ) ) {
        act->set_to_null();
        return;
    }

    // target selection and validation.
    while( act->targets.size() < 2 ) {
        item_location item_loc = game_menus::inv::repair( *you, actor, main_tool );

        if( item_loc == item_location::nowhere ) {
            you->add_msg_if_player( m_info, _( "Never mind." ) );
            act->set_to_null();
            return;
        }
        if( actor->can_repair_target( *you, *item_loc, true, true ) ) {
            act->targets.emplace_back( item_loc );
            repeat = repeat_type::INIT;
        }
    }

    const item &fix = *act->targets[1];
    const bool can_refit = fix.has_flag( flag_VARSIZE ) && !fix.has_flag( flag_FIT );
    if( repeat == repeat_type::INIT ) {
        const int level = you->get_skill_level( actor->used_skill );
        repair_item_actor::repair_type action_type = actor->default_action( fix, level );
        if( action_type == repair_item_actor::RT_NOTHING ) {
            you->add_msg_if_player( _( "You won't learn anything more by doing that." ) );
        }

        const std::pair<float, float> chance = actor->repair_chance( *you, fix, action_type );
        if( chance.first <= 0.0f ) {
            action_type = repair_item_actor::RT_PRACTICE;
        }

        std::string title = string_format( _( "%s %s\n" ),
                                           repair_item_actor::action_description( action_type ),
                                           fix.tname() );
        ammotype current_ammo;
        std::string ammo_name;
        if( used_tool->has_flag( flag_USE_UPS ) || used_tool->has_link_data() ) {
            ammo_name = _( "battery" );
            current_ammo = ammo_battery;
        } else if( used_tool->has_flag( flag_USES_BIONIC_POWER ) ) {
            ammo_name = _( "bionic power" );

        } else {
            if( used_tool->ammo_current().is_null() ) {
                current_ammo = item_controller->find_template( used_tool->ammo_default() )->ammo->type;
            } else {
                current_ammo = item_controller->find_template( used_tool->ammo_current() )->ammo->type;
            }
            ammo_name = item::nname( used_tool->ammo_current() );
        }

        int ammo_remaining = used_tool->ammo_remaining_linked( here, you );

        std::set<itype_id> valid_entries = actor->get_valid_repair_materials( fix );
        const inventory &crafting_inv = you->crafting_inventory();
        std::function<bool( const item & )> filter;
        if( fix.is_filthy() ) {
            filter = []( const item & component ) {
                return component.allow_crafting_component();
            };
        } else {
            filter = is_crafting_component;
        }
        std::vector<std::string> material_list;
        for( const auto &component_id : valid_entries ) {
            if( item::count_by_charges( component_id ) ) {
                if( crafting_inv.has_charges( component_id, 1 ) ) {
                    material_list.push_back( string_format( _( "%s (%d)" ), item::nname( component_id ),
                                                            crafting_inv.charges_of( component_id ) ) );
                }
            } else if( crafting_inv.has_amount( component_id, 1, false, filter ) ) {
                material_list.push_back( string_format( _( "%s (%d)" ), item::nname( component_id ),
                                                        crafting_inv.amount_of( component_id, false ) ) );
            }
        }

        title += used_tool->is_tool() && used_tool->has_flag( flag_USES_NEARBY_AMMO )
                 ? string_format( _( "Charges: <color_light_blue>%s</color> %s (%s per use)\n" ),
                                  ammo_remaining,
                                  ammo_name,
                                  used_tool->ammo_required() )
                 : string_format( _( "Charges: <color_light_blue>%s/%s</color> %s (%s per use)\n" ),
                                  ammo_remaining, used_tool->ammo_capacity( current_ammo, true ),
                                  ammo_name,
                                  used_tool->ammo_required() );
        title += string_format( _( "Materials available: %s\n" ), string_join( material_list, ", " ) );
        title += string_format( _( "Skill used: <color_light_blue>%s (%s)</color>\n" ),
                                actor->used_skill.obj().name(), level );
        title += string_format( _( "Success chance: <color_light_blue>%.1f</color>%%\n" ),
                                100.0f * chance.first );
        title += string_format( _( "Damage chance: <color_light_blue>%.1f</color>%%" ),
                                100.0f * chance.second );

        if( act->values.empty() ) {
            act->values.resize( 1 );
        }
        do {
            repeat = repeat_menu( title, repeat, can_refit );

            if( repeat == repeat_type::CANCEL ) {
                act->set_to_null();
                return;
            }
            act->values[0] = static_cast<int>( repeat );
            // BACK selected, redo target selection next.
            if( repeat == repeat_type::INIT ) {
                you->activity.targets.pop_back();
                return;
            }
            if( repeat == repeat_type::FULL && fix.damage() <= fix.degradation() && !can_refit ) {
                const char *msg = fix.damage_level() > 0 ?
                                  _( "Your %s is repaired as much as possible, considering the degradation." ) :
                                  _( "Your %s is already fully repaired." );
                you->add_msg_if_player( m_info, msg, fix.tname() );
                repeat = repeat_type::INIT;
            }
        } while( repeat == repeat_type::INIT );
    }
    // Otherwise keep retrying
    act->moves_left = actor->move_cost;
}

void activity_handlers::heat_item_finish( player_activity *act, Character *you )
{
    act->set_to_null();
    if( act->targets.size() != 1 ) {
        debugmsg( "invalid arguments to ACT_HEATING" );
        return;
    }
    item_location &loc = act->targets[ 0 ];
    item *const heat = loc.get_item();
    if( heat == nullptr ) {
        return;
    }
    if( !heat->has_temperature() ) {
        debugmsg( "item %s is not heatable", heat->typeId().str() );
        return;
    }
    item &target = *heat;
    if( target.has_own_flag( flag_FROZEN ) ) {
        target.apply_freezerburn();
        if( target.has_flag( flag_EATEN_COLD ) ) {
            target.cold_up();
            you->add_msg_if_player( m_info,
                                    _( "You defrost the food, but don't heat it up, since you enjoy it cold." ) );
        } else {
            target.heat_up();
            you->add_msg_if_player( m_info, _( "You defrost and heat up the food." ) );
        }
    } else {
        target.heat_up();
        you->add_msg_if_player( m_info, _( "You heat up the food." ) );
    }
}

void activity_handlers::mend_item_finish( player_activity *act, Character *you )
{
    act->set_to_null();
    if( act->targets.size() != 1 ) {
        debugmsg( "invalid arguments to ACT_MEND_ITEM" );
        return;
    }
    if( !act->targets[0] ) {
        debugmsg( "lost targets[0] item location for ACT_MEND_ITEM" );
        return;
    }
    item &target = *act->targets[0];
    const fault_id fault_id( act->name );
    if( target.faults.count( fault_id ) == 0 ) {
        debugmsg( "item %s does not have fault %s", target.tname(), fault_id.str() );
        return;
    }
    if( act->str_values.empty() ) {
        debugmsg( "missing fault_fix_id for ACT_MEND_ITEM." );
        return;
    }
    const fault_fix_id fix_id( act->str_values[0] );
    if( !fix_id.is_valid() ) {
        debugmsg( "invalid fault_fix_id '%s' for ACT_MEND_ITEM.", fix_id.str() );
        return;
    }
    const fault_fix &fix = *fix_id;
    const requirement_data &reqs = fix.get_requirements();
    const inventory &inv = you->crafting_inventory();
    if( !reqs.can_make_with_inventory( inv, is_crafting_component ) ) {
        add_msg( m_info, _( "You are currently unable to mend the %s." ), target.tname() );
        return;
    }
    for( const auto &e : reqs.get_components() ) {
        you->consume_items( e );
    }
    for( const auto &e : reqs.get_tools() ) {
        you->consume_tools( e );
    }
    you->invalidate_crafting_inventory();

    for( const ::fault_id &id : fix.faults_removed ) {
        target.remove_fault( id );
    }
    for( const ::fault_id &id : fix.faults_added ) {
        target.set_fault( id, true, false );
    }
    for( const auto &[var_name, var_value] : fix.set_variables ) {
        target.set_var( var_name, var_value );
    }
    for( const auto &[var_name, var_value] : fix.adjust_variables_multiply ) {
        const double var_value_multiplier = var_value;
        const double var_oldvalue = target.get_var( var_name, 0.0 );
        target.set_var( var_name, std::round( var_oldvalue * var_value_multiplier ) );
    }

    const std::string start_durability = target.durability_indicator( true );

    if( fix.mod_damage ) {
        target.mod_damage( fix.mod_damage );
    }
    if( fix.mod_degradation ) {
        target.set_degradation( target.degradation() + fix.mod_degradation );
    }

    for( const auto &[skill_id, level] : fix.skills ) {
        you->practice( skill_id, 10, static_cast<int>( level * 1.25 ) );
    }

    for( const auto &[proficiency_id, mult] : fix.time_save_profs ) {
        you->practice_proficiency( proficiency_id, fix.time );
    }

    add_msg( m_good, fix.success_msg.translated(), target.tname( 1, false ),
             start_durability, target.durability_indicator( true ) );
}

void activity_handlers::toolmod_add_finish( player_activity *act, Character *you )
{
    act->set_to_null();
    if( act->targets.size() != 2 || !act->targets[0] || !act->targets[1] ) {
        debugmsg( "Incompatible arguments to ACT_TOOLMOD_ADD" );
        return;
    }
    item &tool = *act->targets[0];
    item &mod = *act->targets[1];
    you->add_msg_if_player( m_good, _( "You successfully attached the %1$s to your %2$s." ),
                            mod.tname(), tool.tname() );
    tool.put_in( mod, pocket_type::MOD );
    tool.on_contents_changed();
    act->targets[1].remove_item();
}

// This activity opens the menu (it's not meant to queue consumption of items)
void activity_handlers::eat_menu_do_turn( player_activity *, Character *you )
{
    if( !you->is_avatar() ) {
        debugmsg( "Character %s somehow opened the eat menu!  Cancelling their activity to prevent infinite loop",
                  you->name );
        you->cancel_activity();
        return;
    }

    avatar &player_character = get_avatar();
    avatar_action::eat_or_use( player_character, game_menus::inv::consume() );
}

void activity_handlers::consume_food_menu_do_turn( player_activity *, Character * )
{
    avatar &player_character = get_avatar();
    item_location loc = game_menus::inv::consume_food();
    avatar_action::eat( player_character, loc );
}

void activity_handlers::consume_drink_menu_do_turn( player_activity *, Character * )
{
    avatar &player_character = get_avatar();
    item_location loc = game_menus::inv::consume_drink();
    avatar_action::eat( player_character, loc );
}

void activity_handlers::consume_meds_menu_do_turn( player_activity *, Character * )
{
    avatar &player_character = get_avatar();
    avatar_action::eat_or_use( player_character, game_menus::inv::consume_meds() );
}

void activity_handlers::move_loot_do_turn( player_activity *act, Character *you )
{
    activity_on_turn_move_loot( *act, *you );
}

void activity_handlers::travel_do_turn( player_activity *act, Character *you )
{
    if( !you->omt_path.empty() ) {
        you->omt_path.pop_back();
        if( you->omt_path.empty() ) {
            you->add_msg_if_player( m_info, _( "You have reached your destination." ) );
            act->set_to_null();
            ui::omap::force_quit();
            return;
        }
        const tripoint_abs_omt next_omt = you->omt_path.back();
        tripoint_abs_ms waypoint;
        if( you->omt_path.size() == 1 ) {
            // if next omt is the final one, target its midpoint
            waypoint = midpoint( project_bounds<coords::ms>( next_omt ) );
        } else {
            // otherwise target the middle of the edge nearest to our current location
            const tripoint_abs_ms cur_omt_mid = midpoint( project_bounds<coords::ms>
                                                ( you->pos_abs_omt() ) );
            waypoint = clamp( cur_omt_mid, project_bounds<coords::ms>( next_omt ) );
        }
        map &here = get_map();
        tripoint_bub_ms centre_sub = here.get_bub( waypoint );
        const std::vector<tripoint_bub_ms> route_to =
            here.route( *you, pathfinding_target::radius( centre_sub, 2 ) );
        if( !route_to.empty() ) {
            const activity_id act_travel = ACT_TRAVELLING;
            you->set_destination( route_to, player_activity( act_travel ) );
        } else {
            you->add_msg_if_player( m_warning, _( "You cannot reach that destination." ) );
            ui::omap::force_quit();
        }
    } else {
        you->add_msg_if_player( m_info, _( "You have reached your destination." ) );
        ui::omap::force_quit();
    }
    act->set_to_null();
}

void activity_handlers::armor_layers_do_turn( player_activity *, Character *you )
{
    you->cancel_activity();
    you->worn.sort_armor( *you );
}

void activity_handlers::atm_do_turn( player_activity *, Character *you )
{
    iexamine::atm( *you, you->pos_bub() );
}

// fish-with-rod fish catching function.
static void rod_fish( Character *you, const std::vector<monster *> &fishables )
{
    map &here = get_map();
    constexpr auto caught_corpse = []( Character * you, map & here, const mtype & corpse_type ) {
        item corpse = item::make_corpse( corpse_type.id,
                                         calendar::turn + rng( 0_turns,
                                                 3_hours ) );
        corpse.set_var( "activity_var", you->name );
        item_location loc = here.add_item_or_charges_ret_loc( you->pos_bub(), corpse );
        you->add_msg_if_player( m_good, _( "You caught a %s." ), corpse_type.nname() );
        if( loc ) {
            you->may_activity_occupancy_after_end_items_loc.push_back( loc );
        }
    };
    //if the vector is empty (no fish around) the player is still given a small chance to get a (let us say it was hidden) fish
    if( fishables.empty() ) {
        const std::vector<mtype_id> fish_group = MonsterGroupManager::GetMonstersFromGroup(
                    GROUP_FISH, true );
        const mtype_id fish_mon = random_entry_ref( fish_group );
        caught_corpse( you, here, fish_mon.obj() );
    } else {
        monster *chosen_fish = random_entry( fishables );
        chosen_fish->fish_population -= 1;
        if( chosen_fish->fish_population <= 0 ) {
            g->catch_a_monster( chosen_fish, you->pos_bub(), you, 50_hours );
        } else {
            if( chosen_fish->type != nullptr ) {
                caught_corpse( you, here, *( chosen_fish->type ) );
            }
        }
    }
}

void activity_handlers::fish_do_turn( player_activity *act, Character *you )
{
    item &it = *act->targets.front();
    float fish_chance = 1.0f;
    float survival_skill = you->get_skill_level( skill_survival );
    switch( it.get_quality( qual_FISHING_ROD ) ) {
        case 1:
            survival_skill += dice( 1, 6 );
            break;
        case 2:
            // Much better chances with a good fishing implement.
            survival_skill += dice( 4, 9 );
            survival_skill *= 2;
            break;
        default:
            debugmsg( "ERROR: Invalid FISHING_ROD tool quality on %s", item::nname( it.typeId() ) );
            break;
    }
    std::vector<monster *> fishables = g->get_fishable_monsters( act->coord_set );
    // Fish are always there, even if it doesn't seem like they are visible!
    if( fishables.empty() ) {
        fish_chance += survival_skill / 2;
    } else {
        // if they are visible however, it implies a larger population
        for( monster *elem : fishables ) {
            fish_chance += elem->fish_population;
        }
        fish_chance += survival_skill;
    }
    // no matter the population of fish, your skill and tool limits the ease of catching.
    fish_chance = std::min( survival_skill * 10, fish_chance );
    if( x_in_y( fish_chance, 600000 ) ) {
        you->add_msg_if_player( m_good, _( "You feel a tug on your line!" ) );
        rod_fish( you, fishables );
    }
    if( calendar::once_every( 60_minutes ) ) {
        you->practice( skill_survival, rng( 1, 3 ) );
    }

}

void activity_handlers::fish_finish( player_activity *act, Character *you )
{
    act->set_to_null();
    you->add_msg_if_player( m_info, _( "You finish fishing" ) );
    if( !you->backlog.empty() && you->backlog.front().id() == ACT_MULTIPLE_FISH ) {
        you->backlog.clear();
        you->assign_activity( ACT_TIDY_UP );
    }
}

void activity_handlers::repair_item_do_turn( player_activity *act, Character *you )
{
    // Moves are decremented based on a combination of speed and good vision (not in the dark, farsighted, etc)
    const float exertion_mult = you->exertion_adjusted_move_multiplier( act->exertion_level() );
    const int effective_moves = you->get_moves() / ( you->fine_detail_vision_mod() * exertion_mult );
    if( effective_moves <= act->moves_left ) {
        act->moves_left -= effective_moves;
        you->set_moves( 0 );
    } else {
        you->mod_moves( -act->moves_left * you->fine_detail_vision_mod() );
        act->moves_left = 0;
    }
}

void activity_handlers::dismember_do_turn( player_activity * /*act*/, Character *you )
{
    you->burn_energy_arms( -20 );
}

void activity_handlers::wait_finish( player_activity *act, Character *you )
{
    you->add_msg_if_player( _( "You finish waiting." ) );
    act->set_to_null();
}

void activity_handlers::wait_weather_finish( player_activity *act, Character *you )
{
    you->add_msg_if_player( _( "You finish waiting." ) );
    act->set_to_null();
}

void activity_handlers::find_mount_do_turn( player_activity *act, Character *you )
{
    //npc only activity
    if( you->is_avatar() ) {
        act->set_to_null();
        return;
    }
    npc &guy = dynamic_cast<npc &>( *you );
    auto strong_monster = guy.chosen_mount.lock();
    monster *mon = strong_monster.get();
    if( !mon ) {
        act->set_to_null();
        guy.revert_after_activity();
        return;
    }
    if( rl_dist( guy.pos_bub(), mon->pos_bub() ) <= 1 ) {
        if( mon->has_effect( effect_controlled ) ) {
            mon->remove_effect( effect_controlled );
        }
        if( you->can_mount( *mon ) ) {
            act->set_to_null();
            guy.revert_after_activity();
            guy.chosen_mount = weak_ptr_fast<monster>();
            you->mount_creature( *mon );
        } else {
            act->set_to_null();
            guy.revert_after_activity();
            return;
        }
    } else {
        const std::vector<tripoint_bub_ms> route =
            route_adjacent( *you, guy.chosen_mount.lock()->pos_bub() );
        if( route.empty() ) {
            act->set_to_null();
            guy.revert_after_activity();
            mon->remove_effect( effect_controlled );
            return;
        } else {
            you->activity = player_activity();
            mon->add_effect( effect_controlled, 40_turns );
            you->set_destination( route, player_activity( ACT_FIND_MOUNT ) );
        }
    }
}

void activity_handlers::wait_npc_finish( player_activity *act, Character *you )
{
    you->add_msg_if_player( _( "%s finishes with you…" ), act->str_values[0] );
    act->set_to_null();
}

void activity_handlers::socialize_finish( player_activity *act, Character *you )
{
    you->add_msg_if_player( _( "%s finishes chatting with you." ), act->str_values[0] );
    act->set_to_null();
}

void activity_handlers::operation_do_turn( player_activity *act, Character *you )
{
    const map &here = get_map();

    /**
    - values[0]: Difficulty
    - values[1]: success
    - values[2]: bionic UID when uninstalling
    - values[3]: pl_skill
    - str_values[0]: install/uninstall
    - str_values[1]: bionic_id when installing
    - str_values[2]: installer_name
    - str_values[3]: bool autodoc
    */
    enum operation_values_ids {
        operation_type = 0,
        cbm_id = 1,
        installer_name = 2,
        is_autodoc = 3
    };
    const bionic_id bid( act->str_values[cbm_id] );
    const bool autodoc = act->str_values[is_autodoc] == "true";
    Character &player_character = get_player_character();
    const bool u_see = player_character.sees( here, you->pos_bub( here ) ) &&
                       ( !player_character.has_effect( effect_narcosis ) ||
                         player_character.has_bionic( bio_painkiller ) ||
                         player_character.has_flag( json_flag_PAIN_IMMUNE ) );

    const int difficulty = act->values.front();

    const std::vector<bodypart_id> bps = get_occupied_bodyparts( bid );

    const time_duration half_op_duration = difficulty * 10_minutes;
    const time_duration message_freq = difficulty * 2_minutes;
    time_duration time_left = time_duration::from_moves( act->moves_left );

    if( autodoc && here.inbounds( you->pos_bub( here ) ) ) {
        const std::list<tripoint_bub_ms> autodocs = here.find_furnitures_with_flag_in_radius(
                    you->pos_bub(), 1,
                    ter_furn_flag::TFLAG_AUTODOC );

        if( !here.has_flag_furn( ter_furn_flag::TFLAG_AUTODOC_COUCH, you->pos_bub() ) ||
            autodocs.empty() ) {
            you->remove_effect( effect_under_operation );
            act->set_to_null();

            if( u_see ) {
                add_msg( m_bad, _( "The Autodoc suffers a catastrophic failure." ) );

                you->add_msg_player_or_npc( m_bad,
                                            _( "The Autodoc's failure damages you greatly." ),
                                            _( "The Autodoc's failure damages <npcname> greatly." ) );
            }
            if( !bps.empty() ) {
                for( const bodypart_id &bp : bps ) {
                    you->add_effect( effect_bleed,  1_minutes * difficulty, bp, true, 1 );
                    you->apply_damage( nullptr, bp, 20 * difficulty );

                    if( u_see ) {
                        you->add_msg_player_or_npc( m_bad, _( "Your %s is ripped open." ),
                                                    _( "<npcname>'s %s is ripped open." ), body_part_name_accusative( bp ) );
                    }

                    if( bp == bodypart_id( "eyes" ) ) {
                        you->add_effect( effect_blind, 1_hours );
                    }
                }
            } else {
                you->add_effect( effect_bleed,  1_minutes * difficulty, bodypart_str_id::NULL_ID(), true, 1 );
                you->apply_damage( nullptr, bodypart_id( "torso" ), 20 * difficulty );
            }
        }
    }

    if( time_left > half_op_duration ) {
        if( !bps.empty() ) {
            for( const bodypart_id &bp : bps ) {
                if( calendar::once_every( message_freq ) && u_see && autodoc ) {
                    you->add_msg_player_or_npc( m_info,
                                                _( "The Autodoc is meticulously cutting your %s open." ),
                                                _( "The Autodoc is meticulously cutting <npcname>'s %s open." ),
                                                body_part_name_accusative( bp ) );
                }
            }
        } else {
            if( calendar::once_every( message_freq ) && u_see ) {
                you->add_msg_player_or_npc( m_info,
                                            _( "The Autodoc is meticulously cutting you open." ),
                                            _( "The Autodoc is meticulously cutting <npcname> open." ) );
            }
        }
    } else if( time_left == half_op_duration ) {
        if( act->str_values[operation_type] == "uninstall" ) {
            if( u_see && autodoc ) {
                add_msg( m_info, _( "The Autodoc attempts to carefully extract the bionic." ) );
            }

            if( std::optional<bionic *> bio = you->find_bionic_by_uid( act->values[2] ) ) {
                you->perform_uninstall( **bio, act->values[0], act->values[1], act->values[3] );
            } else {
                debugmsg( _( "Tried to uninstall bionic with UID %s, but you don't have this bionic installed." ),
                          act->values[2] );
                you->remove_effect( effect_under_operation );
                act->set_to_null();
            }
        } else {
            if( u_see && autodoc ) {
                add_msg( m_info, _( "The Autodoc attempts to carefully insert the bionic." ) );
            }

            if( bid.is_valid() ) {
                const bionic_id upbid = bid->upgraded_bionic;
                // TODO: Let the user pick bionic to upgrade if multiple candidates exist
                bionic_uid upbio_uid = 0;
                if( std::optional<bionic *> bio = you->find_bionic_by_type( upbid ) ) {
                    upbio_uid = ( *bio )->get_uid();
                }

                you->perform_install( bid, upbio_uid, act->values[0], act->values[1], act->values[3],
                                      act->str_values[installer_name], bid->canceled_mutations, you->pos_bub() );
            } else {
                debugmsg( _( "%s is no a valid bionic_id" ), bid.c_str() );
                you->remove_effect( effect_under_operation );
                act->set_to_null();
            }
        }
    } else if( act->values[1] > 0 ) {
        if( !bps.empty() ) {
            for( const bodypart_id &bp : bps ) {
                if( calendar::once_every( message_freq ) && u_see && autodoc ) {
                    you->add_msg_player_or_npc( m_info,
                                                _( "The Autodoc is stitching your %s back up." ),
                                                _( "The Autodoc is stitching <npcname>'s %s back up." ),
                                                body_part_name_accusative( bp ) );
                }
            }
        } else {
            if( calendar::once_every( message_freq ) && u_see && autodoc ) {
                you->add_msg_player_or_npc( m_info,
                                            _( "The Autodoc is stitching you back up." ),
                                            _( "The Autodoc is stitching <npcname> back up." ) );
            }
        }
    } else {
        if( calendar::once_every( message_freq ) && u_see && autodoc ) {
            you->add_msg_player_or_npc( m_bad,
                                        _( "The Autodoc is moving erratically through the rest of its program, not actually stitching your wounds." ),
                                        _( "The Autodoc is moving erratically through the rest of its program, not actually stitching <npcname>'s wounds." ) );
        }
    }

    // Makes sure NPC is still under anesthesia
    if( you->has_effect( effect_narcosis ) ) {
        const time_duration remaining_time = you->get_effect_dur( effect_narcosis );
        if( remaining_time < time_left ) {
            const time_duration top_off_time = time_left - remaining_time;
            you->add_effect( effect_narcosis, top_off_time );
            you->add_effect( effect_sleep, top_off_time );
        }
    } else {
        you->add_effect( effect_narcosis, time_left );
        you->add_effect( effect_sleep, time_left );
    }
}

void activity_handlers::operation_finish( player_activity *act, Character *you )
{
    map &here = get_map();
    if( act->str_values[3] == "true" ) {
        if( act->values[1] > 0 ) {
            add_msg( m_good,
                     _( "The Autodoc returns to its resting position after successfully performing the operation." ) );
            const std::list<tripoint_bub_ms> autodocs = here.find_furnitures_with_flag_in_radius(
                        you->pos_bub(), 1,
                        ter_furn_flag::TFLAG_AUTODOC );
            sounds::sound( autodocs.front(), 10, sounds::sound_t::music,
                           _( "a short upbeat jingle: \"Operation successful\"" ), true,
                           "Autodoc",
                           "success" );
        } else {
            add_msg( m_bad,
                     _( "The Autodoc jerks back to its resting position after failing the operation." ) );
            const std::list<tripoint_bub_ms> autodocs = here.find_furnitures_with_flag_in_radius(
                        you->pos_bub(), 1,
                        ter_furn_flag::TFLAG_AUTODOC );
            sounds::sound( autodocs.front(), 10, sounds::sound_t::music,
                           _( "a sad beeping noise: \"Operation failed\"" ), true,
                           "Autodoc",
                           "failure" );
        }
    } else {
        if( act->values[1] > 0 ) {
            add_msg( m_good,
                     _( "The operation is a success." ) );
        } else {
            add_msg( m_bad,
                     _( "The operation is a failure." ) );
        }
    }
    you->remove_effect( effect_under_operation );
    act->set_to_null();
}

void activity_handlers::plant_seed_finish( player_activity *act, Character *you )
{
    map &here = get_map();
    tripoint_bub_ms examp = here.get_bub( act->placement );
    const itype_id seed_id( act->str_values[0] );
    std::list<item> used_seed;
    if( item::count_by_charges( seed_id ) ) {
        used_seed = you->use_charges( seed_id, 1 );
    } else {
        used_seed = you->use_amount( seed_id, 1 );
    }
    if( !used_seed.empty() ) {
        used_seed.front().set_age( 0_turns );
        if( used_seed.front().has_var( "activity_var" ) ) {
            used_seed.front().erase_var( "activity_var" );
        }
        used_seed.front().set_flag( json_flag_HIDDEN_ITEM );
        here.add_item_or_charges( examp, used_seed.front() );
        if( here.has_flag_furn( seed_id->seed->required_terrain_flag, examp ) ) {
            here.furn_set( examp, furn_str_id( here.furn( examp )->plant->transform ) );
        } else if( seed_id->seed->required_terrain_flag == ter_furn_flag::TFLAG_PLANTABLE ) {
            here.set( examp, ter_t_dirt, furn_f_plant_seed );
        } else {
            here.furn_set( examp, furn_f_plant_seed );
        }
        you->add_msg_player_or_npc( _( "You plant some %s." ), _( "<npcname> plants some %s." ),
                                    item::nname( seed_id ) );
    }
    // Go back to what we were doing before
    // could be player zone activity, or could be NPC multi-farming
    act->set_to_null();
    resume_for_multi_activities( *you );
}

void activity_handlers::build_do_turn( player_activity *act, Character *you )
{
    map &here = get_map();
    partial_con *pc = here.partial_con_at( here.get_bub( act->placement ) );
    // Maybe the player and the NPC are working on the same construction at the same time
    if( !pc ) {
        if( you->is_npc() ) {
            // if player completes the work while NPC still in activity loop
            you->activity = player_activity();
            you->set_moves( 0 );
        } else {
            you->cancel_activity();
        }
        add_msg( m_info, _( "%s did not find an unfinished construction at the activity spot." ),
                 you->disp_name() );
        return;
    }
    you->set_activity_level( pc->id->activity_level );
    // if you ( or NPC ) are finishing someone else's started construction...
    const construction &built = pc->id.obj();
    if( !you->has_trait( trait_DEBUG_HS ) && !you->meets_skill_requirements( built ) ) {
        add_msg( m_info, _( "%s can't work on this construction anymore." ), you->disp_name() );
        you->cancel_activity();
        if( you->is_npc() ) {
            you->activity = player_activity();
            you->set_moves( 0 );
        }
        return;
    }
    // item_counter represents the percent progress relative to the base batch time
    // stored precise to 5 decimal places ( e.g. 67.32 percent would be stored as 6732000 )
    const int old_counter = pc->counter;

    // Base moves for construction with no speed modifier or assistants
    // Clamp to >= 100 to prevent division by 0 or int overflow on characters with high speed;
    const double base_total_moves = std::max( 100, built.time );
    // Current expected total moves, includes construction speed modifiers and assistants
    const double cur_total_moves = std::max( 100, built.adjusted_time() );
    // Delta progress in moves adjusted for current crafting speed
    const double delta_progress = you->get_moves() * base_total_moves / cur_total_moves;
    // Current progress in moves
    const double current_progress = old_counter * base_total_moves / 10000000.0 +
                                    delta_progress;
    you->set_moves( 0 );
    pc->id->do_turn_special( here.get_bub( act->placement ), *you );
    // Current progress as a percent of base_total_moves to 2 decimal places
    pc->counter = std::round( current_progress / base_total_moves * 10000000.0 );
    pc->counter = std::min( pc->counter, 10000000 );
    // If construction_progress has reached 100% or more
    if( pc->counter >= 10000000 ) {
        // Activity is canceled in complete_construction()
        complete_construction( you->as_character() );
    }
}

void activity_handlers::tidy_up_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_fish_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_construction_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_mine_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_mop_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_chop_planks_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_butcher_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_craft_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_dis_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_read_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::vehicle_deconstruction_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::vehicle_repair_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::chop_trees_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_farm_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::fetch_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::vibe_finish( player_activity *act, Character *you )
{
    you->add_msg_if_player( m_good, _( "You feel much better." ) );
    you->add_morale( morale_feeling_good, 10, 40 );
    act->set_to_null();
}

void activity_handlers::atm_finish( player_activity *act, Character * )
{
    // ATM sets index to 0 to indicate it's finished.
    if( !act->index ) {
        act->set_to_null();
    }
}

void activity_handlers::eat_menu_finish( player_activity *, Character * )
{
    // Only exists to keep the eat activity alive between turns
}

void activity_handlers::jackhammer_do_turn( player_activity *act, Character * )
{
    map &here = get_map();
    sfx::play_activity_sound( "tool", "jackhammer",
                              sfx::get_heard_volume( here.get_bub( act->placement ) ) );
    if( calendar::once_every( 1_minutes ) ) {
        sounds::sound( here.get_bub( act->placement ), 15, sounds::sound_t::destructive_activity,
                       //~ Sound of a jackhammer at work!
                       _( "TATATATATATATAT!" ) );
    }
}

void activity_handlers::jackhammer_finish( player_activity *act, Character *you )
{
    map &here = get_map();
    const tripoint_bub_ms &pos = here.get_bub( act->placement );

    here.destroy( pos, true );

    you->add_msg_player_or_npc( m_good,
                                _( "You finish drilling." ),
                                _( "<npcname> finishes drilling." ) );
    act->set_to_null();
    if( !act->targets.empty() ) {
        item &it = *act->targets.front();
        it.ammo_consume( it.ammo_required(), tripoint_bub_ms::zero, you );
    } else {
        debugmsg( "jackhammer activity targets empty" );
    }
    if( resume_for_multi_activities( *you ) ) {
        for( item &elem : here.i_at( pos ) ) {
            elem.set_var( "activity_var", you->name );
            you->may_activity_occupancy_after_end_items_loc.emplace_back( map_cursor{here.get_abs( pos )},
                    &elem );
        }
    }
}

template<typename fn>
static void cleanup_tiles( std::unordered_set<tripoint_abs_ms> &tiles, fn &cleanup )
{
    auto it = tiles.begin();
    map &here = get_map();
    while( it != tiles.end() ) {
        auto current = it++;

        const tripoint_bub_ms &tile_loc = here.get_bub( *current );

        if( cleanup( tile_loc ) ) {
            tiles.erase( current );
        }
    }
}

static void perform_zone_activity_turn(
    Character *you, const zone_type_id &ztype,
    const std::function<bool( const tripoint_bub_ms & )> &tile_filter,
    const std::function<void ( Character &you, const tripoint_bub_ms & )> &tile_action,
    const std::string &finished_msg )
{
    const zone_manager &mgr = zone_manager::get_manager();
    map &here = get_map();
    const tripoint_abs_ms abspos = you->pos_abs();
    std::unordered_set<tripoint_abs_ms> unsorted_tiles = mgr.get_near( ztype, abspos );

    cleanup_tiles( unsorted_tiles, tile_filter );

    // sort remaining tiles by distance
    const std::vector<tripoint_abs_ms> &tiles =
        get_sorted_tiles_by_distance( abspos, unsorted_tiles );

    for( const tripoint_abs_ms &tile : tiles ) {
        const tripoint_bub_ms &tile_loc = here.get_bub( tile );

        std::vector<tripoint_bub_ms> route =
            here.route( *you, pathfinding_target::point( tile_loc ) );
        if( route.size() > 1 ) {
            route.pop_back();

            you->set_destination( route, you->activity );
            you->activity.set_to_null();
            return;
        } else {
            // we are at destination already
            /* Perform action */
            tile_action( *you, tile_loc );
            if( you->get_moves() <= 0 ) {
                return;
            }
        }
    }
    add_msg( m_info, finished_msg );
    you->activity.set_to_null();
}

void activity_handlers::fertilize_plot_do_turn( player_activity *act, Character *you )
{
    itype_id fertilizer;

    auto have_fertilizer = [&]() {
        return !fertilizer.is_empty() && you->has_amount( fertilizer, 1 );
    };

    auto check_fertilizer = [&]( bool ask_user = true ) -> void {
        if( act->str_values.empty() )
        {
            act->str_values.emplace_back( "" );
        }
        fertilizer = itype_id( act->str_values[0] );

        /* If unspecified, or if we're out of what we used before, ask */
        if( ask_user && !have_fertilizer() )
        {
            fertilizer = iexamine::choose_fertilizer( *you, "plant",
                    false /* Don't confirm action with player */ );
            act->str_values[0] = fertilizer.str();
        }
    };


    const auto reject_tile = [&]( const tripoint_bub_ms & tile ) {
        check_fertilizer();
        ret_val<void> can_fert = iexamine::can_fertilize( *you, tile, fertilizer );
        return !can_fert.success();
    };

    const auto fertilize = [&]( Character & you, const tripoint_bub_ms & tile ) {
        check_fertilizer();
        if( have_fertilizer() ) {
            iexamine::fertilize_plant( you, tile, fertilizer );
            if( !have_fertilizer() ) {
                add_msg( m_info, _( "You have run out of %s." ), item::nname( fertilizer ) );
            }
        }
    };

    check_fertilizer();
    if( !have_fertilizer() ) {
        act->set_to_null();
        return;
    }

    perform_zone_activity_turn( you,
                                zone_type_FARM_PLOT,
                                reject_tile,
                                fertilize,
                                _( "You fertilized every plot you could." ) );
}

void activity_handlers::robot_control_do_turn( player_activity *act, Character *you )
{
    if( act->monsters.empty() ) {
        debugmsg( "No monster assigned in ACT_ROBOT_CONTROL" );
        act->set_to_null();
        return;
    }
    const shared_ptr_fast<monster> z = act->monsters[0].lock();

    if( !z || !iuse::robotcontrol_can_target( you, *z ) ) {
        you->add_msg_if_player( _( "Target lost.  IFF override failed." ) );
        act->set_to_null();
        return;
    }

    // TODO: Add some kind of chance of getting the target's attention
}

void activity_handlers::robot_control_finish( player_activity *act, Character *you )
{
    act->set_to_null();

    if( act->monsters.empty() ) {
        debugmsg( "No monster assigned in ACT_ROBOT_CONTROL" );
        return;
    }

    shared_ptr_fast<monster> z = act->monsters[0].lock();
    act->monsters.clear();

    if( !z || !iuse::robotcontrol_can_target( you, *z ) ) {
        you->add_msg_if_player( _( "Target lost.  IFF override failed." ) );
        return;
    }

    you->add_msg_if_player( _( "You unleash your override attack on the %s." ), z->name() );

    /** @EFFECT_INT increases chance of successful robot reprogramming, vs difficulty */
    /** @EFFECT_COMPUTER increases chance of successful robot reprogramming, vs difficulty */
    const float computer_skill = you->get_skill_level( skill_computer );
    const float randomized_skill = rng( 2, you->int_cur ) + computer_skill;
    float success = computer_skill - 3 * z->type->difficulty / randomized_skill;
    if( z->has_flag( mon_flag_RIDEABLE_MECH ) ) {
        success = randomized_skill - rng( 1, 11 );
    }
    // rideable mechs are not hostile, they have no AI, they do not resist control as much.
    if( success >= 0 ) {
        you->add_msg_if_player( _( "You successfully override the %s's IFF protocols!" ),
                                z->name() );
        z->friendly = -1;
        if( z->has_flag( mon_flag_RIDEABLE_MECH ) ) {
            z->add_effect( effect_pet, 1_turns, true );
        }
    } else if( success >= -2 ) {
        //A near success
        you->add_msg_if_player( _( "The %s short circuits as you attempt to reprogram it!" ), z->name() );
        //damage it a little
        z->apply_damage( you, bodypart_id( "torso" ), rng( 1, 10 ) );
        if( z->is_dead() ) {
            you->practice( skill_computer, 10 );
            // Do not do the other effects if the robot died
            return;
        }
        if( one_in( 3 ) ) {
            you->add_msg_if_player( _( "…and turns friendly!" ) );
            //did the robot became friendly permanently?
            if( one_in( 3 ) ) {
                //it did
                z->friendly = -1;
            } else {
                // it didn't
                z->friendly = rng( 5, 40 );
            }
        }
    } else {
        you->add_msg_if_player( _( "…but the robot refuses to acknowledge you as an ally!" ) );
    }
    you->practice( skill_computer, 10 );
}

void activity_handlers::pull_creature_finish( player_activity *act, Character *you )
{
    if( you->is_avatar() ) {
        you->as_avatar()->longpull( act->name );
    } else {
        you->longpull( act->name, get_map().get_bub( act->placement ) );
    }
    act->set_to_null();
}

void activity_handlers::tree_communion_do_turn( player_activity *act, Character *you )
{
    // There's an initial rooting process.
    if( act->values.front() > 0 ) {
        act->values.front() -= 1;
        if( act->values.front() == 0 ) {
            if( you->has_trait( trait_id( trait_SPIRITUAL ) ) ) {
                you->add_msg_if_player( m_good, _( "The ancient tree spirits answer your call." ) );
            } else {
                you->add_msg_if_player( m_good, _( "Your communion with the trees has begun." ) );
            }
        }
        return;
    }
    // Information is received every minute.
    if( !calendar::once_every( 1_minutes ) ) {
        return;
    }
    // Breadth-first search forest tiles until one reveals new overmap tiles.
    std::queue<tripoint_abs_omt> q;
    std::unordered_set<tripoint_abs_omt> seen;
    tripoint_abs_omt loc = you->pos_abs_omt();
    q.push( loc );
    seen.insert( loc );
    const std::function<bool( const oter_id & )> filter = []( const oter_id & ter ) {
        // FIXME: this is terrible and should be a property instead of a name check...
        return ter.obj().is_wooded() || ter.obj().get_name( om_vision_level::full ) == _( "field" );
    };
    while( !q.empty() ) {
        tripoint_abs_omt tpt = q.front();
        if( overmap_buffer.reveal( tpt, 3, filter ) ) {
            if( you->has_trait( trait_SPIRITUAL ) ) {
                you->add_morale( morale_tree_communion, 2, 30, 8_hours, 6_hours );
            } else {
                you->add_morale( morale_tree_communion, 1, 15, 2_hours, 1_hours );
            }
            if( one_in( 128 ) ) {
                if( one_in( 256 ) ) {
                    if( you->has_effect( effect_social_dissatisfied ) ) {
                        you->remove_effect( effect_social_dissatisfied );
                    }
                    if( ( you->has_flag( json_flag_SOCIAL1 ) || you->has_flag( json_flag_SOCIAL2 ) ) &&
                        !you->has_effect( effect_social_satisfied ) ) {
                        you->add_effect( effect_social_satisfied, 3_hours, false, 1 );
                    }
                    if( ( you->has_flag( json_flag_ASOCIAL1 ) || you->has_flag( json_flag_ASOCIAL2 ) ) &&
                        !you->has_effect( effect_asocial_dissatisfied ) ) {
                        you->add_effect( effect_asocial_dissatisfied, 3_hours, false, 1 );
                    }
                }
                you->add_msg_if_player( "%s", SNIPPET.random_from_category( "tree_communion" ).value_or(
                                            translation() ) );
            }
            return;
        }
        for( const tripoint_abs_omt &neighbor : points_in_radius( tpt, 1 ) ) {
            if( seen.find( neighbor ) != seen.end() ) {
                continue;
            }
            seen.insert( neighbor );
            if( !overmap_buffer.ter( neighbor ).obj().is_wooded() ) {
                continue;
            }
            q.push( neighbor );
        }
        q.pop();
    }
    you->add_msg_if_player( m_info, _( "The trees have shown you what they will." ) );
    act->set_to_null();
}

void activity_handlers::spellcasting_finish( player_activity *act, Character *you )
{
    act->set_to_null();
    const int level_override = act->get_value( 0 );
    spell_id sp( act->name );

    // if level is -1 then we know it's a player spell, otherwise we build it from the ground up
    spell temp_spell( sp );
    spell &spell_being_cast = ( level_override == -1 ) ? you->magic->get_spell( sp ) : temp_spell;

    // if level != 1 then we need to set the spell's level
    if( level_override != -1 ) {
        spell_being_cast.set_level( *you, level_override );
    }

    // choose target for spell before continuing
    const std::optional<tripoint_bub_ms> target = act->coords.empty() ? spell_being_cast.select_target(
                you ) : get_map().get_bub( act->coords.front() );
    if( target ) {
        // npcs check for target viability
        if( !you->is_npc() || spell_being_cast.is_valid_target( *you, *target ) ) {
            // no turning back now. it's all said and done.
            bool success = act->get_value( 1 ) == 1 ||
                           rng_float( 0.0f, 1.0f ) >= spell_being_cast.spell_fail( *you );
            int exp_gained = spell_being_cast.casting_exp( *you );
            if( !success ) {
                you->add_msg_if_player( game_message_params{ m_bad, gmf_bypass_cooldown },
                                        _( "You lose your concentration!" ) );
                if( !spell_being_cast.is_max_level( *you ) && level_override == -1 ) {
                    // still get some experience for trying
                    exp_gained *= spell_being_cast.get_failure_exp_percent( *you );
                    spell_being_cast.gain_exp( *you, exp_gained );
                }
                if( act->get_value( 2 ) != 0 ) {
                    spell_being_cast.consume_spell_cost( *you, false );
                }
                dialogue d( get_talker_for( you ), nullptr );
                std::vector<effect_on_condition_id> failure_eocs = spell_being_cast.get_failure_eoc_ids();
                for( effect_on_condition_id failure_eoc : failure_eocs ) {
                    failure_eoc->activate( d );
                }
                get_event_bus().send<event_type::spellcasting_finish>( you->getID(), false, sp,
                        spell_being_cast.spell_class(), spell_being_cast.get_difficulty( *you ),
                        spell_being_cast.energy_cost( *you ), spell_being_cast.casting_time( *you ),
                        spell_being_cast.damage( *you ) );
                return;
            }

            if( spell_being_cast.has_flag( spell_flag::VERBAL ) && !you->has_flag( json_flag_SILENT_SPELL ) ) {
                sounds::sound( you->pos_bub(), you->get_shout_volume() / 2, sounds::sound_t::speech,
                               _( "cast a spell" ),
                               false );
            }

            you->add_msg_if_player( spell_being_cast.message(), spell_being_cast.name() );

            // this is here now so that the spell first consume its components then casts its effects, necessary to cast
            // spells with the components in hand.
            spell_being_cast.use_components( *you );

            if( act->get_value( 2 ) != 0 ) {
                spell_being_cast.consume_spell_cost( *you, true );
            }

            if( !act->targets.empty() ) {
                item *it = act->targets.front().get_item();
                if( it && !it->has_flag( flag_USE_PLAYER_ENERGY ) ) {
                    you->consume_charges( *it, it->type->charges_to_use() );
                }
            }

            spell_being_cast.cast_all_effects( *you, *target );

            if( level_override == -1 ) {
                if( !spell_being_cast.is_max_level( *you ) ) {
                    // reap the reward
                    int old_level = spell_being_cast.get_level();
                    if( old_level == 0 ) {
                        spell_being_cast.gain_level( *you );
                        you->add_msg_if_player( m_good,
                                                _( "Something about how this spell works just clicked!  You gained a level!" ) );
                    } else {
                        spell_being_cast.gain_exp( *you, exp_gained );
                    }
                    if( spell_being_cast.get_level() != old_level ) {
                        // Level 0-1 message is printed above - notify player when leveling up further
                        if( old_level > 0 ) {
                            you->add_msg_if_player( m_good, _( "You gained a level in %s!" ), spell_being_cast.name() );
                        }
                    }
                }
            }
            get_event_bus().send<event_type::spellcasting_finish>( you->getID(), true, sp,
                    spell_being_cast.spell_class(), spell_being_cast.get_difficulty( *you ),
                    spell_being_cast.energy_cost( *you ), spell_being_cast.casting_time( *you ),
                    spell_being_cast.damage( *you ) );
        }
    }
}

void activity_handlers::study_spell_do_turn( player_activity *act, Character *you )
{
    // Stop if there is not enough light to study
    if( you->fine_detail_vision_mod() > 4 ) {
        act->values[2] = -1;
        act->moves_left = 0;
        return;
    }
    // str_value 1 is "study" if we already know the spell, and want to study it more
    if( act->get_str_value( 1 ) == "study" ) {
        spell &studying = you->magic->get_spell( spell_id( act->name ) );
        // If we are studying to gain a level, keep studying until level changes
        if( act->get_str_value( 0 ) == "gain_level" ) {
            if( studying.get_level() < act->get_value( 1 ) ) {
                act->moves_left = 1000000;
            } else {
                act->moves_left = 0;
            }
        }
        const int old_level = studying.get_level();
        // Gain some experience from studying
        const int xp = roll_remainder( studying.exp_modifier( *you ) / to_turns<float>( 6_seconds ) );
        act->values[0] += xp;
        studying.gain_exp( *you, xp );
        bool leveled_up = you->practice( studying.skill(), xp, studying.get_difficulty( *you ), true );
        if( leveled_up &&
            studying.get_difficulty( *you ) < static_cast<int>( you->get_skill_level( studying.skill() ) ) ) {
            you->handle_skill_warning( studying.skill(),
                                       true ); // show the skill warning on level up, since we suppress it in practice() above
        }
        // Notify player if the spell leveled up
        if( studying.get_level() > old_level ) {
            you->add_msg_if_player( m_good, _( "You gained a level in %s!" ), studying.name() );
        }
    }
}

void activity_handlers::study_spell_finish( player_activity *act, Character *you )
{
    act->set_to_null();
    const int total_exp_gained = act->get_value( 0 );

    if( act->get_str_value( 1 ) == "study" ) {
        you->add_msg_if_player( m_good, _( "You gained %i experience from your study session." ),
                                total_exp_gained );
    } else if( act->get_str_value( 1 ) == "learn" && act->values[2] == 0 ) {
        you->magic->learn_spell( act->name, *you );
    }
    if( act->values[2] == -1 ) {
        you->add_msg_if_player( m_bad, _( "It's too dark to read." ) );
    }
}
