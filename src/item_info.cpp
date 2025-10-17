#include "item.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ammo.h"
#include "ascii_art.h"
#include "avatar.h"
#include "bionics.h"
#include "body_part_set.h"
#include "bodygraph.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "character_martial_arts.h"
#include "color.h"
#include "damage.h"
#include "debug.h"
#include "dialogue.h"
#include "dispersion.h"
#include "enum_traits.h"
#include "enums.h"
#include "fault.h"
#include "flag.h"
#include "flat_set.h"
#include "game.h"
#include "game_constants.h"
#include "global_vars.h"
#include "gun_mode.h"
#include "inventory.h"
#include "item_category.h"
#include "item_components.h"
#include "item_contents.h"
#include "item_factory.h"
#include "item_pocket.h"
#include "iteminfo_query.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "localized_comparator.h"
#include "magic_enchantment.h"
#include "martialarts.h"
#include "material.h"
#include "math_parser_diag_value.h"
#include "mod_manager.h"
#include "mtype.h"
#include "options.h"
#include "output.h"
#include "pimpl.h"
#include "pocket_type.h"
#include "proficiency.h"
#include "ranged.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "ret_val.h"
#include "skill.h"
#include "stomach.h"
#include "string_formatter.h"
#include "string_id_utils.h"
#include "subbodypart.h"
#include "talker.h"
#include "text_snippets.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "units_utility.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vitamin.h"
#include "weighted_list.h"

static const ammo_effect_str_id ammo_effect_BLACKPOWDER( "BLACKPOWDER" );
static const ammo_effect_str_id ammo_effect_IGNITE( "IGNITE" );
static const ammo_effect_str_id ammo_effect_INCENDIARY( "INCENDIARY" );
static const ammo_effect_str_id ammo_effect_MATCHHEAD( "MATCHHEAD" );
static const ammo_effect_str_id ammo_effect_NEVER_MISFIRES( "NEVER_MISFIRES" );
static const ammo_effect_str_id ammo_effect_RECYCLED( "RECYCLED" );

static const bionic_id bio_digestion( "bio_digestion" );

static const bodygraph_id bodygraph_full_body_iteminfo( "full_body_iteminfo" );

static const damage_type_id damage_acid( "acid" );
static const damage_type_id damage_heat( "heat" );

static const item_category_id item_category_container( "container" );

static const itype_id itype_disassembly( "disassembly" );
static const itype_id itype_rad_badge( "rad_badge" );
static const itype_id itype_rm13_armor( "rm13_armor" );

static const json_character_flag json_flag_CANNIBAL( "CANNIBAL" );
static const json_character_flag json_flag_CARNIVORE_DIET( "CARNIVORE_DIET" );
static const json_character_flag json_flag_IMMUNE_SPOIL( "IMMUNE_SPOIL" );
static const json_character_flag json_flag_PSYCHOPATH( "PSYCHOPATH" );
static const json_character_flag json_flag_SAPIOVORE( "SAPIOVORE" );

static const quality_id qual_JACK( "JACK" );
static const quality_id qual_LIFT( "LIFT" );

static const skill_id skill_cooking( "cooking" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_survival( "survival" );

static const vitamin_id vitamin_human_flesh_vitamin( "human_flesh_vitamin" );

static const std::string flag_NO_DISPLAY( "NO_DISPLAY" );

// sorts with localized_compare, and enumerates entries, if more than \p max entries
// the rest are abbreviated into " and %d more"
static std::string enumerate_lcsorted_with_limit( const std::vector<std::string> &v,
        const int max = 5 )
{
    if( max < 1 ) {
        debugmsg( "Expected max to be 1 or more" );
        return std::string();
    }
    std::vector<std::string> c( std::min<size_t>( max, v.size() ) );
    std::partial_sort_copy( v.begin(), v.end(), c.begin(), c.end(), localized_compare );
    const int more = static_cast<int>( v.size() - c.size() );
    if( more == 0 ) {
        return enumerate_as_string( c );
    }
    const std::string res = enumerate_as_string( c, enumeration_conjunction::none );
    return string_format( max == 1 ? _( "%s and %d more" ) : _( "%s, and %d more" ), res, more );
}

// TODO: Get rid of, handle multiple types gracefully
static int get_ranged_pierce( const common_ranged_data &ranged )
{
    if( ranged.damage.empty() ) {
        return 0;
    }

    return ranged.damage.damage_units.front().res_pen;
}

// Generates a long-form description of the freshness of the given rottable food item.
// NB: Doesn't check for non-rottable!
static std::string get_freshness_description( const item &food_item )
{
    // So, skilled characters looking at food that is neither super-fresh nor about to rot
    // can guess its age as one of {quite fresh,midlife,past midlife,old soon}, and also
    // guess about how long until it spoils.
    const double rot_progress = food_item.get_relative_rot();
    const time_duration shelf_life = food_item.get_shelf_life();
    time_duration time_left = shelf_life - ( shelf_life * rot_progress );

    // Correct for an estimate that exceeds shelf life -- this happens especially with
    // fresh items.
    if( time_left > shelf_life ) {
        time_left = shelf_life;
    }

    Character &player_character = get_player_character();
    if( food_item.is_fresh() ) {
        // Fresh food is assumed to be obviously so regardless of skill.
        if( player_character.can_estimate_rot() ) {
            return string_format( _( "* This food looks as <good>fresh</good> as it can be.  "
                                     "It still has <info>%s</info> until it spoils." ),
                                  to_string_approx( time_left ) );
        } else {
            return _( "* This food looks as <good>fresh</good> as it can be." );
        }
    } else if( food_item.is_going_bad() ) {
        // Old food likewise is assumed to be fairly obvious.
        if( player_character.can_estimate_rot() ) {
            return string_format( _( "* This food looks <bad>old</bad>.  "
                                     "It's just <info>%s</info> from becoming inedible." ),
                                  to_string_approx( time_left ) );
        } else {
            return _( "* This food looks <bad>old</bad>.  "
                      "It's on the brink of becoming inedible." );
        }
    }

    if( !player_character.can_estimate_rot() ) {
        // Unskilled characters only get a hint that more information exists...
        return _( "* This food looks <info>fine</info>.  If you were more skilled in "
                  "cooking or survival, you might be able to make a better estimation." );
    }

    // Otherwise, a skilled character can determine the below options:
    if( rot_progress < 0.3 ) {
        //~ here, %s is an approximate time span, e.g., "over 2 weeks" or "about 1 season"
        return string_format( _( "* This food looks <good>quite fresh</good>.  "
                                 "It has <info>%s</info> until it spoils." ),
                              to_string_approx( time_left ) );
    } else if( rot_progress < 0.5 ) {
        //~ here, %s is an approximate time span, e.g., "over 2 weeks" or "about 1 season"
        return string_format( _( "* This food looks like it is reaching its <neutral>midlife</neutral>.  "
                                 "There's <info>%s</info> before it spoils." ),
                              to_string_approx( time_left ) );
    } else if( rot_progress < 0.7 ) {
        //~ here, %s is an approximate time span, e.g., "over 2 weeks" or "about 1 season"
        return string_format( _( "* This food looks like it has <neutral>passed its midlife</neutral>.  "
                                 "Edible, but will go bad in <info>%s</info>." ),
                              to_string_approx( time_left ) );
    } else {
        //~ here, %s is an approximate time span, e.g., "over 2 weeks" or "about 1 season"
        return string_format( _( "* This food looks like it <bad>will be old soon</bad>.  "
                                 "It has <info>%s</info>, so if you plan to use it, it's now or never." ),
                              to_string_approx( time_left ) );
    }
}

static int get_base_env_resist( const item &it )
{
    const islot_armor *t = it.find_armor_data();
    if( t == nullptr ) {
        if( it.is_pet_armor() ) {
            return it.type->pet_armor->env_resist * it.get_relative_health();
        } else {
            return 0;
        }
    }

    return t->avg_env_resist() * it.get_relative_health();
}

static void insert_separation_line( std::vector<iteminfo> &info )
{
    if( info.empty() || info.back().sName != "--" ) {
        info.emplace_back( "DESCRIPTION", "--" );
    }
}

void item::basic_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                       bool /* debug */ ) const
{
    if( parts->test( iteminfo_parts::BASE_MOD_SRC ) ) {
        info.emplace_back( "BASE", get_origin( type->src ) );
        insert_separation_line( info );
    }

    const std::string space = "  ";
    if( parts->test( iteminfo_parts::BASE_MATERIAL ) ) {
        const std::vector<const material_type *> mat_types = made_of_types();
        if( !mat_types.empty() ) {
            const std::string material_list = enumerate_as_string( mat_types,
            []( const material_type * material ) {
                return string_format( "<stat>%s</stat>", material->name() );
            }, enumeration_conjunction::none );
            info.emplace_back( "BASE", string_format( _( "Material: %s" ), material_list ) );
        }
    }
    if( parts->test( iteminfo_parts::BASE_VOLUME ) ) {
        info.push_back( vol_to_info( "BASE", _( "Volume: " ), volume() * batch, 3 ) );
    }
    if( parts->test( iteminfo_parts::BASE_WEIGHT ) ) {
        info.push_back( weight_to_info( "BASE", space + _( "Weight: " ), weight() * batch ) );
        info.back().bNewLine = true;
    }
    if( parts->test( iteminfo_parts::BASE_LENGTH ) && length() > 0_mm ) {
        info.emplace_back( "BASE", _( "Length: " ),
                           string_format( "<num> %s", length_units( length() ) ),
                           iteminfo::lower_is_better,
                           convert_length( length() ), length().value() );
    }
    if( parts->test( iteminfo_parts::BASE_EMEMORY ) && ememory_size() > 0_KB ) {
        info.emplace_back( "BASE", string_format( "%s %s", _( "Memory:" ),
                           units::display( ememory_size() ) ) );
    }
    if( parts->test( iteminfo_parts::BASE_OWNER ) && !owner.is_null() ) {
        info.emplace_back( "BASE", string_format( _( "Owner: %s" ),
                           get_owner_name() ) );
    }
    if( parts->test( iteminfo_parts::BASE_CATEGORY ) ) {
        info.emplace_back( "BASE", _( "Category: " ),
                           "<header>" + get_category_shallow().name_header() + "</header>" );
    }

    if( parts->test( iteminfo_parts::DESCRIPTION ) ) {
        insert_separation_line( info );
        global_variables::impl_t::const_iterator const idescription =
            item_vars.find( "description" );
        const std::optional<translation> snippet = SNIPPET.get_snippet_by_id( snip_id );
        if( snippet.has_value() ) {
            // Just use the dynamic description
            info.emplace_back( "DESCRIPTION", snippet.value().translated() );

            // only ever do the effect for a snippet the first time you see it
            if( !get_avatar().has_seen_snippet( snip_id ) ) {
                // Have looked at the item so call the on examine EOC for the snippet
                const std::optional<talk_effect_t> examine_effect = SNIPPET.get_EOC_by_id( snip_id );
                if( examine_effect.has_value() ) {
                    // activate the effect
                    dialogue d( get_talker_for( get_avatar() ), nullptr );
                    examine_effect.value().apply( d );
                }

                //note that you have seen the snippet
                get_avatar().add_snippet( snip_id );
            }
        } else if( idescription != item_vars.end() ) {
            info.emplace_back( "DESCRIPTION", idescription->second.str() );
        } else if( has_itype_variant() ) {
            info.emplace_back( "DESCRIPTION", variant_description() );
        } else {
            if( has_flag( flag_MAGIC_FOCUS ) ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "This item is a <info>magical focus</info>.  "
                                      "You can cast spells with it in your hand." ) );
            }
            if( is_craft() ) {
                const std::string desc = ( typeId() == itype_disassembly ) ?
                                         _( "This is an in progress disassembly of %s.  "
                                            "It is %d percent complete." ) : _( "This is an in progress %s.  "
                                                    "It is %d percent complete." );
                const int percent_progress = item_counter / 100000;
                info.emplace_back( "DESCRIPTION", string_format( desc,
                                   craft_data_->making->result_name(),
                                   percent_progress ) );
            } else {
                info.emplace_back( "DESCRIPTION", type->description.translated() );
            }
        }
        insert_separation_line( info );
    }

    insert_separation_line( info );

    if( parts->test( iteminfo_parts::BASE_REQUIREMENTS ) ) {
        // Display any minimal stat or skill requirements for the item
        std::vector<std::string> req;
        if( get_min_str() > 0 ) {
            int min_str = 0;
            for( const item *mod : gunmods() ) {
                min_str += mod->type->gunmod->min_str_required_mod_if_prone;
            }
            if( min_str != 0 && !get_player_character().is_prone() ) {
                req.push_back( string_format( "%s %d (%d %s)", _( "strength" ), get_min_str(),
                                              get_min_str() + min_str, _( "if prone" ) ) );
            } else {
                req.push_back( string_format( "%s %d", _( "strength" ), get_min_str() ) );
            }
        }
        if( type->min_dex > 0 ) {
            req.push_back( string_format( "%s %d", _( "dexterity" ), type->min_dex ) );
        }
        if( type->min_int > 0 ) {
            req.push_back( string_format( "%s %d", _( "intelligence" ), type->min_int ) );
        }
        if( type->min_per > 0 ) {
            req.push_back( string_format( "%s %d", _( "perception" ), type->min_per ) );
        }
        for( const std::pair<skill_id, int> &sk : sorted_lex( type->min_skills ) ) {
            req.push_back( string_format( "%s %d", sk.first->name(), sk.second ) );
        }
        if( !req.empty() ) {
            info.emplace_back( "BASE", _( "<bold>Minimum requirements</bold>:" ) );
            info.emplace_back( "BASE", enumerate_as_string( req ) );
            insert_separation_line( info );
        }
    }

    if( has_var( "contained_name" ) && parts->test( iteminfo_parts::BASE_CONTENTS ) ) {
        info.emplace_back( "BASE", string_format( _( "Contains: %s" ),
                           get_var( "contained_name" ) ) );
    }
    if( count_by_charges() && !is_food() && !is_medication() &&
        parts->test( iteminfo_parts::BASE_AMOUNT ) ) {
        info.emplace_back( "BASE", _( "Amount: " ), "<num>", iteminfo::no_flags,
                           charges * batch );
    }
}

void item::debug_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /* batch */,
                       bool debug ) const
{
    if( debug && parts->test( iteminfo_parts::BASE_DEBUG ) ) {
        if( g != nullptr ) {
            info.emplace_back( "BASE", string_format( "itype_id: %s",
                               typeId().str() ) );
            if( !old_owner.is_null() ) {
                info.emplace_back( "BASE", string_format( _( "Old owner: %s" ),
                                   get_old_owner_name() ) );
            }
            info.emplace_back( "BASE", _( "age (hours): " ), "", iteminfo::lower_is_better,
                               to_hours<int>( age() ) );
            info.emplace_back( "BASE", _( "charges: " ), "", iteminfo::lower_is_better,
                               charges );
            info.emplace_back( "BASE", _( "damage: " ), "", iteminfo::lower_is_better,
                               damage_ );
            info.emplace_back( "BASE", _( "degradation: " ), "", iteminfo::lower_is_better,
                               degradation_ );
            info.emplace_back( "BASE", _( "active: " ), "", iteminfo::lower_is_better,
                               active );
            info.emplace_back( "BASE", _( "burn: " ), "", iteminfo::lower_is_better,
                               burnt );
            info.emplace_back( "BASE", _( "counter: " ), "", iteminfo::lower_is_better,
                               item_counter );
            if( countdown_point != calendar::turn_max ) {
                info.emplace_back( "BASE", _( "countdown: " ), "", iteminfo::lower_is_better,
                                   to_seconds<int>( countdown_point - calendar::turn ) );
            }

            const std::string tags_listed = enumerate_as_string( item_tags,
            []( const flag_id & f ) {
                return f.str();
            }, enumeration_conjunction::none );
            info.emplace_back( "BASE", string_format( _( "tags: %s" ), tags_listed ) );

            const std::string flags_listed = enumerate_as_string( type->get_flags(),
            []( const flag_id & f ) {
                return f.str();
            }, enumeration_conjunction::none );

            info.emplace_back( "BASE", string_format( _( "flags: %s" ), flags_listed ) );
            for( auto const &imap : item_vars ) {
                info.emplace_back( "BASE",
                                   string_format( _( "item var: %s, %s" ), imap.first,
                                                  imap.second.to_string() ) );
            }

            info.emplace_back( "BASE", _( "wetness: " ),
                               "", iteminfo::lower_is_better,
                               wetness );

            const std::string space = "  ";
            if( goes_bad() ) {
                info.emplace_back( "BASE", _( "age (turns): " ),
                                   "", iteminfo::lower_is_better,
                                   to_turns<int>( age() ) );
                info.emplace_back( "BASE", _( "rot (turns): " ),
                                   "", iteminfo::lower_is_better,
                                   to_turns<int>( rot ) );
                info.emplace_back( "BASE", space + _( "max rot (turns): " ),
                                   "", iteminfo::lower_is_better,
                                   to_turns<int>( get_shelf_life() ) );
            }
            if( has_temperature() ) {
                info.emplace_back( "BASE", _( "last temp: " ),
                                   "", iteminfo::lower_is_better,
                                   to_turn<int>( last_temp_check ) );
                info.emplace_back( "BASE", _( "Temp: " ), "", iteminfo::lower_is_better | iteminfo::is_decimal,
                                   units::to_kelvin( temperature ) );
                info.emplace_back( "BASE", _( "Spec ener: " ), "",
                                   iteminfo::lower_is_better | iteminfo::is_decimal,
                                   units::to_joule_per_gram( specific_energy ) );
                info.emplace_back( "BASE", _( "Spec heat lq: " ), "",
                                   iteminfo::lower_is_better | iteminfo::is_decimal,
                                   get_specific_heat_liquid() );
                info.emplace_back( "BASE", _( "Spec heat sld: " ), "",
                                   iteminfo::lower_is_better | iteminfo::is_decimal,
                                   get_specific_heat_solid() );
                info.emplace_back( "BASE", _( "latent heat: " ), "",
                                   iteminfo::lower_is_better,
                                   get_latent_heat() );
                info.emplace_back( "BASE", _( "Freeze point: " ), "",
                                   iteminfo::lower_is_better | iteminfo::is_decimal,
                                   units::to_kelvin( get_freeze_point() ) );
            }

            std::string faults;
            for( const std::pair<fault_id, int> &fault : type->faults ) {
                const int weight_percent = static_cast<float>( fault.second ) / type->faults.get_weight() * 100;
                faults += colorize( fault.first.str() + string_format( " (%d, %d%%)\n", fault.second,
                                    weight_percent ), has_fault( fault.first ) ? c_yellow : c_white );
            }
            info.emplace_back( "BASE", string_format( "faults: %s", faults ) );

            units::mass sum_of_components_weight;
            for( const item_comp &c : get_uncraft_components() ) {
                sum_of_components_weight += c.type->weight * c.count;
            }
            info.emplace_back( "BASE", string_format( "weight of uncraft components: %s grams",
                               to_gram( sum_of_components_weight ) ) );
        }
    }
}

void item::med_info( const item *med_item, std::vector<iteminfo> &info, const iteminfo_query *parts,
                     int batch, bool debug ) const
{
    const cata::value_ptr<islot_comestible> &med_com = med_item->get_comestible();
    if( med_com->quench != 0 && parts->test( iteminfo_parts::MED_QUENCH ) ) {
        info.emplace_back( "MED", _( "Quench: " ), med_com->quench );
    }

    Character &player_character = get_player_character();
    if( med_item->get_comestible_fun() != 0 && parts->test( iteminfo_parts::MED_JOY ) ) {
        info.emplace_back( "MED", _( "Enjoyability: " ),
                           player_character.fun_for( *med_item ).first );
    }

    if( parts->test( iteminfo_parts::FOOD_HEALTH ) && med_com->healthy != 0 ) {
        info.emplace_back( "MED", _( "Health: " ), healthy_bar( med_com->healthy ) );
    }

    if( med_com->stim != 0 && parts->test( iteminfo_parts::MED_STIMULATION ) ) {
        std::string name = string_format( "%s<stat>%s</stat>", _( "Stimulation: " ),
                                          med_com->stim > 0 ? _( "Upper" ) : _( "Downer" ) );
        info.emplace_back( "MED", name );
    }

    if( parts->test( iteminfo_parts::MED_PORTIONS ) ) {
        info.emplace_back( "MED", _( "Portions: " ),
                           std::abs( static_cast<int>( med_item->count() ) * batch ) );
    }

    if( parts->test( iteminfo_parts::MED_CONSUME_TIME ) ) {
        info.emplace_back( "MED", _( "Consume time: " ),
                           to_string( player_character.get_consume_time( *med_item ) ) );
    }

    bool addictive = false;
    for( const std::pair<const addiction_id, int> &add : med_com->addictions ) {
        if( add.second > 0 ) {
            addictive = true;
            break;
        }
    }
    if( addictive && parts->test( iteminfo_parts::DESCRIPTION_MED_ADDICTING ) ) {
        info.emplace_back( "DESCRIPTION", _( "* Consuming this item is <bad>addicting</bad>." ) );
    }

    rot_info( med_item, info, parts, batch, debug );
}

void item::food_info( const item *food_item, std::vector<iteminfo> &info,
                      const iteminfo_query *parts, int batch, bool debug ) const
{
    nutrients min_nutr;
    nutrients max_nutr;

    Character &player_character = get_player_character();
    std::string recipe_exemplar = get_var( "recipe_exemplar", std::string{} );
    if( recipe_exemplar.empty() ) {
        min_nutr = max_nutr = player_character.compute_effective_nutrients( *food_item );
    } else {
        std::map<recipe_id, std::pair<nutrients, nutrients>> rec_cache;
        std::tie( min_nutr, max_nutr ) =
            player_character.compute_nutrient_range( *food_item, recipe_id( recipe_exemplar ), rec_cache );
    }

    bool show_nutr = parts->test( iteminfo_parts::FOOD_NUTRITION ) ||
                     parts->test( iteminfo_parts::FOOD_VITAMINS );
    if( min_nutr != max_nutr && show_nutr ) {
        info.emplace_back(
            "FOOD", _( "Nutrition will <color_cyan>vary with chosen ingredients</color>." ) );
        if( recipe_dict.is_item_on_loop( food_item->typeId() ) ) {
            info.emplace_back(
                "FOOD", _( "Nutrition range cannot be calculated accurately due to "
                           "<color_red>recipe loops</color>." ) );
        }
    }

    if( max_nutr.kcal() != 0 || food_item->get_comestible()->quench != 0 ) {
        if( parts->test( iteminfo_parts::FOOD_NUTRITION ) ) {
            info.emplace_back( "FOOD", _( "<bold>Calories (kcal)</bold>: " ),
                               "", iteminfo::no_newline, min_nutr.kcal() );
            if( max_nutr.kcal() != min_nutr.kcal() ) {
                info.emplace_back( "FOOD", _( "-" ),
                                   "", iteminfo::no_newline, max_nutr.kcal() );
            }
        }
        if( parts->test( iteminfo_parts::FOOD_QUENCH ) ) {
            const std::string space = "  ";
            info.emplace_back( "FOOD", space + _( "Quench: " ),
                               food_item->get_comestible()->quench );
        }
        if( parts->test( iteminfo_parts::FOOD_SATIATION ) ) {

            if( max_nutr.kcal() == min_nutr.kcal() ) {
                info.emplace_back( "FOOD", _( "<bold>Satiety: </bold>" ),
                                   satiety_bar( player_character.compute_calories_per_effective_volume( *food_item ) ) );
            } else {
                info.emplace_back( "FOOD", _( "<bold>Satiety: </bold>" ),
                                   satiety_bar( player_character.compute_calories_per_effective_volume( *food_item, &min_nutr ) ),
                                   iteminfo::no_newline
                                 );
                info.emplace_back( "FOOD", _( " - " ),
                                   satiety_bar( player_character.compute_calories_per_effective_volume( *food_item, &max_nutr ) ) );
            }
        }
    }

    const std::pair<int, int> fun_for_food_item = player_character.fun_for( *food_item );
    if( fun_for_food_item.first != 0 && parts->test( iteminfo_parts::FOOD_JOY ) ) {
        info.emplace_back( "FOOD", _( "Enjoyability: " ), fun_for_food_item.first );
    }

    if( parts->test( iteminfo_parts::FOOD_HEALTH ) && food_item->get_comestible()->healthy != 0 ) {
        info.emplace_back( "MED", _( "Health: " ),
                           healthy_bar( food_item->get_comestible()->healthy ) );
    }

    if( parts->test( iteminfo_parts::FOOD_PORTIONS ) ) {
        info.emplace_back( "FOOD", _( "Portions: " ),
                           std::abs( static_cast<int>( food_item->count() ) * batch ) );
    }
    if( food_item->corpse != nullptr && parts->test( iteminfo_parts::FOOD_SMELL ) &&
        ( debug || ( g != nullptr && player_character.has_flag( json_flag_CARNIVORE_DIET ) ) ) ) {
        info.emplace_back( "FOOD", _( "Smells like: " ) + food_item->corpse->nname() );
    }

    if( parts->test( iteminfo_parts::FOOD_CONSUME_TIME ) ) {
        info.emplace_back( "FOOD", _( "Consume time: " ),
                           to_string( player_character.get_consume_time( *food_item ) ) );
    }

    auto format_vitamin = [&]( const std::pair<vitamin_id, int> &v, bool display_vitamins ) {
        const bool is_vitamin = v.first->type() == vitamin_type::VITAMIN;
        // only display vitamins that we actually require
        if( player_character.vitamin_rate( v.first ) == 0_turns || v.second == 0 ||
            display_vitamins != is_vitamin || v.first->has_flag( flag_NO_DISPLAY ) ) {
            return std::string();
        }
        const int min_value = min_nutr.get_vitamin( v.first );
        const int max_value = v.second;
        const int min_rda = player_character.vitamin_RDA( v.first, min_value );
        const int max_rda = player_character.vitamin_RDA( v.first, max_value );
        std::pair<std::string, std::string> weight_and_units = v.first->mass_str_from_units( v.second );
        std::string weight_str;
        if( !weight_and_units.first.empty() ) {
            if( min_value != max_value ) {
                std::pair<std::string, std::string> min_weight_and_units = v.first->mass_str_from_units(
                            min_nutr.get_vitamin( v.first ) );
                const bool show_first_unit = ( min_value != 0 ) &&
                                             ( min_weight_and_units.second != weight_and_units.second );
                std::string first_string = string_format( !show_first_unit ? "%s" : "%s %s",
                                           min_weight_and_units.first, min_weight_and_units.second );
                weight_str = string_format( "%s-%s %s ", first_string, weight_and_units.first,
                                            weight_and_units.second );
            } else {
                weight_str = string_format( "%s %s ", weight_and_units.first, weight_and_units.second );
            }
        }
        std::string format;
        if( is_vitamin ) {
            format = min_value == max_value ? "%s%s (%i%%)" : "%s%s (%i-%i%%)";
            return string_format( format, weight_str, v.first->name(), min_rda, max_rda );
        }
        format = min_value == max_value ? "%s%s (%i U)" : "%s%s (%i-%i U)";
        return string_format( format, weight_str, v.first->name(), min_value, max_value );
    };

    const auto max_nutr_vitamins = sorted_lex( max_nutr.vitamins() );
    const std::string required_vits = enumerate_as_string( max_nutr_vitamins,
    [&]( const std::pair<vitamin_id, int> &v ) {
        return format_vitamin( v, true );
    } );

    const std::string effect_vits = enumerate_as_string( max_nutr_vitamins,
    [&]( const std::pair<vitamin_id, int> &v ) {
        return format_vitamin( v, false );
    } );

    if( !required_vits.empty() && parts->test( iteminfo_parts::FOOD_VITAMINS ) ) {
        info.emplace_back( "FOOD", _( "Vitamins (RDA): " ), required_vits );
    }

    if( !effect_vits.empty() && parts->test( iteminfo_parts::FOOD_VIT_EFFECTS ) ) {
        info.emplace_back( "FOOD", _( "Other contents: " ), effect_vits );
    }

    insert_separation_line( info );

    if( parts->test( iteminfo_parts::FOOD_ALLERGEN )
        && player_character.allergy_type( *food_item ) != morale_type::NULL_ID() ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This food will cause an <bad>allergic reaction</bad>." ) );
    }

    if( food_item->has_vitamin( vitamin_human_flesh_vitamin ) &&
        parts->test( iteminfo_parts::FOOD_CANNIBALISM ) ) {
        if( !player_character.has_flag( json_flag_CANNIBAL ) &&
            !player_character.has_flag( json_flag_PSYCHOPATH ) &&
            !player_character.has_flag( json_flag_SAPIOVORE ) ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* This food contains <bad>human flesh</bad>." ) );
        } else {
            info.emplace_back( "DESCRIPTION",
                               _( "* This food contains <good>human flesh</good>." ) );
        }
    }

    if( food_item->is_tainted() && parts->test( iteminfo_parts::FOOD_CANNIBALISM ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This food is <bad>tainted</bad> and will poison you." ) );
    }

    ///\EFFECT_SURVIVAL >=3 allows detection of poisonous food
    if( food_item->has_flag( flag_HIDDEN_POISON ) &&
        player_character.get_greater_skill_or_knowledge_level( skill_survival ) >= 3 &&
        parts->test( iteminfo_parts::FOOD_POISON ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* On closer inspection, this appears to be "
                              "<bad>poisonous</bad>." ) );
    }

    ///\EFFECT_SURVIVAL >=5 allows detection of hallucinogenic food
    if( food_item->has_flag( flag_HIDDEN_HALLU ) &&
        player_character.get_greater_skill_or_knowledge_level( skill_survival ) >= 5 &&
        parts->test( iteminfo_parts::FOOD_HALLUCINOGENIC ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* On closer inspection, this appears to be "
                              "<neutral>hallucinogenic</neutral>." ) );
    }

    rot_info( food_item, info, parts, batch, debug );
}

void item::rot_info( const item *const food_item, std::vector<iteminfo> &info,
                     const iteminfo_query *const parts, const int /*batch*/, const bool /*debug*/ ) const
{
    Character &player_character = get_player_character();

    if( food_item->goes_bad() && parts->test( iteminfo_parts::FOOD_ROT ) ) {
        const std::string rot_time = to_string_clipped( food_item->get_shelf_life() );
        info.emplace_back( "DESCRIPTION",
                           string_format( _( "* This item is <neutral>perishable</neutral>, "
                                             "and at room temperature has an estimated nominal "
                                             "shelf life of <info>%s</info>." ), rot_time ) );

        if( !food_item->rotten() ) {
            info.emplace_back( "DESCRIPTION", get_freshness_description( *food_item ) );
        }

        if( food_item->has_flag( flag_FREEZERBURN ) && !food_item->rotten() &&
            !food_item->has_flag( flag_MUSHY ) ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* Quality of this food suffers when it's frozen, and it "
                                  "<neutral>will become mushy after thawing out</neutral>." ) );
        }
        if( food_item->has_flag( flag_MUSHY ) && !food_item->rotten() ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* It was frozen once and after thawing became <bad>mushy and "
                                  "tasteless</bad>.  It will rot quickly if thawed again." ) );
        }
        if( food_item->has_flag( flag_NO_PARASITES ) &&
            player_character.get_greater_skill_or_knowledge_level( skill_cooking ) >= 3 ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* It seems that deep freezing <good>killed all "
                                  "parasites</good>." ) );
        }
        if( food_item->rotten() ) {
            if( player_character.has_bionic( bio_digestion ) ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "This item has started to <neutral>rot</neutral>, "
                                      "but <info>your bionic digestion can tolerate "
                                      "it</info>." ) );
            } else if( player_character.has_flag( json_flag_IMMUNE_SPOIL ) ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "This item has started to <neutral>rot</neutral>, "
                                      "but <info>you can tolerate it</info>." ) );
            } else {
                info.emplace_back( "DESCRIPTION",
                                   _( "This item has started to <bad>rot</bad>. "
                                      "<info>Eating</info> it would be a <bad>very bad "
                                      "idea</bad>." ) );
            }
        }
    }
}

void item::magazine_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /*batch*/,
                          bool /*debug*/ ) const
{
    if( !is_magazine() || has_flag( flag_NO_RELOAD ) || is_tool() ) {
        return;
    }

    if( parts->test( iteminfo_parts::MAGAZINE_COMPATIBLE_GUNS ) &&
        islot_magazine::compatible_guns.find( this->typeId() ) != islot_magazine::compatible_guns.end() ) {
        const auto compare = []( const auto & lhs, const auto & rhs ) {
            return localized_compare( lhs, rhs );
        };
        std::set<std::string, decltype( compare )> compatible_guns( compare );

        for( const itype_id &gun_type_id : islot_magazine::compatible_guns[this->typeId()] ) {
            const itype &gun_type = gun_type_id.obj();
            if( get_option<bool>( "SHOW_GUN_VARIANTS" ) ) {
                if( gun_type.variants.empty() ) {
                    compatible_guns.insert( gun_type.nname( 1 ) );
                }
                for( const itype_variant_data &variant_type : gun_type.variants ) {
                    compatible_guns.insert( variant_type.alt_name.translated() );
                }
            } else {
                compatible_guns.insert( gun_type.nname( 1 ) );
            }
        }

        if( !compatible_guns.empty() ) {
            const std::string mag_names = enumerate_as_string( compatible_guns );
            info.emplace_back( "MAGAZINE", _( "<bold>Compatible guns</bold>: " ) + mag_names );
            insert_separation_line( info );
        }
    }
    if( parts->test( iteminfo_parts::MAGAZINE_CAPACITY ) ) {
        for( const ammotype &at : ammo_types() ) {
            const std::string fmt = string_format( n_gettext( "<num> round of %s",
                                                   "<num> rounds of %s", ammo_capacity( at ) ),
                                                   at->name() );
            info.emplace_back( "MAGAZINE", _( "Capacity: " ), fmt, iteminfo::no_flags,
                               ammo_capacity( at ) );
        }
    }
    if( parts->test( iteminfo_parts::MAGAZINE_COMPATABILITY ) ) {
        for( const ammotype &at : ammo_types() ) {
            std::string ammo_details = print_ammo( at );
            if( !ammo_details.empty() ) {
                info.emplace_back( "MAGAZINE", _( "Variants: " ), ammo_details, iteminfo::no_flags );
            }

        }
    }
    if( parts->test( iteminfo_parts::MAGAZINE_RELOAD ) && type->magazine ) {
        info.emplace_back( "MAGAZINE", _( "Reload time: " ), _( "<num> moves per round" ),
                           iteminfo::lower_is_better, type->magazine->reload_time );
    }
    insert_separation_line( info );
}

void item::ammo_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /* batch */,
                      bool /* debug */ ) const
{
    // Skip this section for guns and items without ammo data
    if( is_gun() || !has_ammo_data() ||
        !parts->test( iteminfo_parts::AMMO_REMAINING_OR_TYPES ) ) {
        return;
    }

    const islot_ammo &ammo = *ammo_data()->ammo;
    if( !ammo.damage.empty() || ammo.force_stat_display ) {
        if( is_ammo() ) {
            info.emplace_back( "AMMO", _( "<bold>Ammunition type</bold>: " ), ammo_type()->name() );
        } else if( ammo_remaining( ) > 0 ) {
            info.emplace_back( "AMMO", _( "<bold>Ammunition</bold>: " ),
                               ammo_data()->nname( ammo_remaining( ) ) );
        }

        const std::string space = "  ";
        if( !ammo.damage.empty() && ammo.damage.damage_units.front().amount > 0 ) {
            if( parts->test( iteminfo_parts::AMMO_DAMAGE_VALUE ) ) {
                info.emplace_back( "AMMO", _( "Damage: " ), "",
                                   iteminfo::no_newline, ammo.damage.total_damage() );
            }
        } else if( parts->test( iteminfo_parts::AMMO_DAMAGE_PROPORTIONAL ) ) {
            const float multiplier = ammo.damage.empty() ? 1.0f
                                     : ammo.damage.damage_units.front().unconditional_damage_mult;
            info.emplace_back( "AMMO", _( "Damage multiplier: " ), "",
                               iteminfo::no_newline | iteminfo::is_decimal,
                               multiplier );
        }
        if( parts->test( iteminfo_parts::AMMO_DAMAGE_AP ) ) {
            info.emplace_back( "AMMO", space + _( "Armor-pierce: " ), get_ranged_pierce( ammo ) );
        }
        if( parts->test( iteminfo_parts::AMMO_DAMAGE_RANGE ) ) {
            info.emplace_back( "AMMO", _( "Range: " ), "<num>" + space,
                               iteminfo::no_newline, ammo.range );
        }
        if( ammo.range_multiplier != 1.0f && parts->test( iteminfo_parts::AMMO_DAMAGE_RANGE_MULTIPLIER ) ) {
            info.emplace_back( "AMMO", _( "Range Multiplier: " ), "<num>",
                               iteminfo::is_decimal, ammo.range_multiplier );
        }
        if( parts->test( iteminfo_parts::AMMO_DAMAGE_DISPERSION ) ) {
            info.emplace_back( "AMMO", _( "Dispersion: " ), "<num> MOA",
                               iteminfo::is_decimal | iteminfo::lower_is_better, ammo.dispersion / 100.0 );
        }
        if( parts->test( iteminfo_parts::AMMO_DAMAGE_RECOIL ) ) {
            info.emplace_back( "AMMO", _( "Recoil: " ), "<num> MOA",
                               iteminfo::is_decimal | iteminfo::lower_is_better | iteminfo::no_newline, ammo.recoil / 100.0 );
        }
        if( parts->test( iteminfo_parts::AMMO_DAMAGE_CRIT_MULTIPLIER ) ) {
            info.emplace_back( "AMMO", space + _( "Critical multiplier: " ), ammo.critical_multiplier );
        }
        if( parts->test( iteminfo_parts::AMMO_BARREL_DETAILS ) ) {
            const units::length small = 150_mm;
            const units::length medium = 400_mm;
            const units::length large = 600_mm;
            const int small_damage = static_cast<int>( ammo.damage.di_considering_length(
                                         small ).total_damage() );
            const int medium_damage = static_cast<int>( ammo.damage.di_considering_length(
                                          medium ).total_damage() );
            const int large_damage = static_cast<int>( ammo.damage.di_considering_length(
                                         large ).total_damage() );
            const int small_dispersion = static_cast<int>( ammo.dispersion_considering_length( small ) );
            const int medium_dispersion = static_cast<int>( ammo.dispersion_considering_length( medium ) );
            const int large_dispersion = static_cast<int>( ammo.dispersion_considering_length( large ) );
            if( small_damage != medium_damage || medium_damage != large_damage ||
                small_dispersion != medium_dispersion || medium_damage != large_dispersion ) {
                info.emplace_back( "AMMO", _( "Damage and dispersion by barrel length: " ) );
                const std::string small_string = string_format( " <info>%d %s</info>: ",
                                                 convert_length( small ),
                                                 length_units( small ) );
                info.emplace_back( "AMMO", small_string, _( " damage = <num>" ), iteminfo::no_newline,
                                   small_damage );
                info.emplace_back( "AMMO", "", _( " dispersion = <num> MOA" ),
                                   iteminfo::no_name | iteminfo::is_decimal | iteminfo::lower_is_better,
                                   small_dispersion / 100.0 );
                const std::string medium_string = string_format( " <info>%d %s</info>: ",
                                                  convert_length( medium ),
                                                  length_units( medium ) );
                info.emplace_back( "AMMO", medium_string, _( " damage = <num>" ), iteminfo::no_newline,
                                   medium_damage );
                info.emplace_back( "AMMO", "", _( " dispersion = <num> MOA" ),
                                   iteminfo::no_name  | iteminfo::is_decimal | iteminfo::lower_is_better,
                                   medium_dispersion / 100.0 );
                const std::string large_string = string_format( " <info>%d %s</info>: ",
                                                 convert_length( large ),
                                                 length_units( large ) );
                info.emplace_back( "AMMO", large_string, _( " damage = <num>" ), iteminfo::no_newline,
                                   large_damage );
                info.emplace_back( "AMMO", "", _( " dispersion = <num> MOA" ),
                                   iteminfo::no_name | iteminfo::is_decimal | iteminfo::lower_is_better,
                                   large_dispersion / 100.0 );
            }
        }
    }

    std::vector<std::string> fx;
    if( ammo.ammo_effects.count( ammo_effect_RECYCLED ) &&
        parts->test( iteminfo_parts::AMMO_FX_RECYCLED ) ) {
        fx.emplace_back(
            _( "This ammo has been <bad>hand-loaded</bad>, "
               "which resulted in slightly inferior performance compared to factory-produced ammo." ) );
    }
    if( ammo.ammo_effects.count( ammo_effect_MATCHHEAD ) &&
        parts->test( iteminfo_parts::AMMO_FX_BLACKPOWDER ) ) {
        fx.emplace_back(
            _( "This ammo has been loaded with <bad>matchhead powder</bad>, and will quickly "
               "clog and rust most guns like blackpowder, but will also rarely cause the gun damage from pressure spikes." ) );
    } else if( ammo.ammo_effects.count( ammo_effect_BLACKPOWDER ) &&
               parts->test( iteminfo_parts::AMMO_FX_BLACKPOWDER ) ) {
        fx.emplace_back(
            _( "This ammo has been loaded with <bad>blackpowder</bad>, and will quickly "
               "clog up most guns, and cause rust if the gun is not cleaned." ) );
    }
    if( ammo.ammo_effects.count( ammo_effect_NEVER_MISFIRES ) &&
        parts->test( iteminfo_parts::AMMO_FX_CANTMISSFIRE ) ) {
        fx.emplace_back( _( "This ammo <good>never misfires</good>." ) );
    }
    if( parts->test( iteminfo_parts::AMMO_FX_RECOVER ) ) {
        if( ammo.recovery_chance <= 75 ) {
            fx.emplace_back( _( "Stands a <bad>very low</bad> chance of remaining intact once fired." ) );
        } else if( ammo.recovery_chance <= 80 ) {
            fx.emplace_back( _( "Stands a <bad>low</bad> chance of remaining intact once fired." ) );
        } else if( ammo.recovery_chance <= 90 ) {
            fx.emplace_back( _( "Stands a <bad>somewhat low</bad> chance of remaining intact once fired." ) );
        } else if( ammo.recovery_chance <= 95 ) {
            fx.emplace_back( _( "Stands a <good>decent</good> chance of remaining intact once fired." ) );
        } else {
            fx.emplace_back( _( "Stands a <good>good</good> chance of remaining intact once fired." ) );
        }
    }
    if( ( ammo.ammo_effects.count( ammo_effect_INCENDIARY ) ||
          ammo.ammo_effects.count( ammo_effect_IGNITE ) ) &&
        parts->test( iteminfo_parts::AMMO_FX_INCENDIARY ) ) {
        fx.emplace_back( _( "This ammo <neutral>may start fires</neutral>." ) );
    }
    if( !fx.empty() ) {
        insert_separation_line( info );
        for( const std::string &e : fx ) {
            info.emplace_back( "AMMO", e );
        }
    }
}

void item::gun_info( const item *mod, std::vector<iteminfo> &info, const iteminfo_query *parts,
                     int /* batch */, bool /* debug */ ) const
{
    const std::string space = "  ";
    const islot_gun &gun = *mod->type->gun;
    const Skill &skill = *mod->gun_skill();

    // many statistics are dependent upon loaded ammo
    // if item is unloaded (or is RELOAD_AND_SHOOT) shows approximate stats using default ammo
    const item *loaded_mod = mod;
    item tmp;
    const itype *curammo = nullptr;
    if( mod->ammo_required() && !mod->ammo_remaining( ) ) {
        tmp = *mod;
        if( tmp.ammo_types().size() == 1 && *tmp.ammo_types().begin() == ammotype::NULL_ID() ) {
            itype_id default_bore_type_id = itype_id::NULL_ID();
            for( const itype_id &bore_type_id : type->gun->default_mods ) {
                std::map< ammotype, std::set<itype_id> > magazine_adaptor = find_type(
                            bore_type_id )->mod->magazine_adaptor;
                if( !magazine_adaptor.empty() && !magazine_adaptor.begin()->second.empty() ) {
                    default_bore_type_id = bore_type_id;
                    item tmp_mod( bore_type_id );
                    tmp.put_in( tmp_mod, pocket_type::MOD );
                    item tmp_mag( *magazine_adaptor.begin()->second.begin() );
                    tmp_mag.ammo_set( magazine_adaptor.begin()->first->default_ammotype() );
                    tmp.put_in( tmp_mag, pocket_type::MAGAZINE_WELL );
                    break;
                }
            }
            if( parts->test( iteminfo_parts::GUN_DEFAULT_BORE ) ) {
                insert_separation_line( info );
                if( default_bore_type_id != itype_id::NULL_ID() ) {
                    info.emplace_back( "GUN",
                                       _( "Weapon is <bad>not attached a bore mod</bad>, so stats below assume the default bore mod: " ),
                                       string_format( "<stat>%s</stat>",
                                                      default_bore_type_id->nname( 1 ) ) );
                } else {
                    info.emplace_back( "GUN",
                                       _( "Weapon is <bad>not attached a bore mod</bad>. " ) );
                    return;
                }

            }

        }
        itype_id default_ammo = mod->magazine_current() ? tmp.common_ammo_default() : tmp.ammo_default();
        if( !default_ammo.is_null() ) {
            tmp.ammo_set( default_ammo );
        } else if( !tmp.magazine_default().is_null() ) {
            // clear out empty magazines so put_in below doesn't error
            for( item *i : tmp.contents.all_items_top() ) {
                if( i->is_magazine() ) {
                    tmp.remove_item( *i );
                    tmp.on_contents_changed();
                }
            }

            item tmp_mag( tmp.magazine_default() );
            tmp_mag.ammo_set( tmp_mag.ammo_default() );
            tmp.put_in( tmp_mag, pocket_type::MAGAZINE_WELL );
        }
        loaded_mod = &tmp;
        curammo = loaded_mod->ammo_data();
        // TODO: Should this be .is_null(), rather than comparing to "none"?
        if( loaded_mod == nullptr || loaded_mod->typeId().str() == "none" ||
            curammo == nullptr ) {
            if( magazine_current() ) {
                const itype_id mag_default = magazine_current()->ammo_default();
                if( mag_default.is_null() ) {
                    debugmsg( "gun %s has magazine %s with no default ammo",
                              typeId().c_str(), magazine_current()->typeId().c_str() );
                    return;
                }
                curammo = &mag_default.obj();
            } else {
                debugmsg( "loaded a nun or ammo_data() is nullptr" );
                return;
            }
        }
        if( parts->test( iteminfo_parts::GUN_DEFAULT_AMMO ) ) {
            insert_separation_line( info );
            info.emplace_back( "GUN",
                               _( "Weapon is <bad>not loaded</bad>, so stats below assume the default ammo: " ),
                               string_format( "<stat>%s</stat>",
                                              curammo->nname( 1 ) ) );
        }
    } else {
        curammo = loaded_mod->ammo_data();
    }

    if( parts->test( iteminfo_parts::GUN_DAMAGE ) ) {
        insert_separation_line( info );
        info.emplace_back( "GUN", colorize( _( "Ranged damage" ), c_bold ) + ": ", iteminfo::no_newline );
    }

    if( mod->ammo_required() ) {
        // some names are not shown, so no need to translate them

        const int gun_damage = loaded_mod->gun_damage( true, false ).total_damage();
        int pellet_damage = 0;
        const bool pellets = curammo->ammo->count > 1;
        if( parts->test( iteminfo_parts::GUN_DAMAGE_PELLETS ) && pellets ) {
            pellet_damage = loaded_mod->gun_damage( true, true ).total_damage();

            // if shotgun, gun_damage is your point blank damage, and anything further is pellets x damage of a single pellet
            info.emplace_back( "GUN", _( "<bold>Point blank damage:</bold> " ), "<num>", iteminfo::no_flags,
                               gun_damage );
            info.emplace_back( "GUN", "damage_per_pellet", _( "<num> per pellet" ),
                               iteminfo::no_newline | iteminfo::no_name, pellet_damage );
            info.emplace_back( "GUN", ", ", iteminfo::no_newline );
            info.emplace_back( "GUN", "amount_of_pellets", _( "<num> pellets" ),
                               iteminfo::no_newline | iteminfo::no_name, curammo->ammo->count );
            info.emplace_back( "GUN", " = ", iteminfo::no_newline );
        } else {

        }

        if( parts->test( iteminfo_parts::GUN_DAMAGE_TOTAL ) ) {
            info.emplace_back( "GUN", "final_damage", "<num>",
                               iteminfo::no_newline | iteminfo::no_name,
                               pellets ? pellet_damage * curammo->ammo->count : gun_damage );
        }
    } else {
        info.emplace_back( "GUN", "final_damage", "<num>", iteminfo::no_newline | iteminfo::no_name,
                           mod->gun_damage( false ).total_damage() );
    }
    info.back().bNewLine = true;

    if( mod->barrel_length().value() > 0 ) {
        if( parts->test( iteminfo_parts::GUN_BARRELLENGTH ) ) {
            info.emplace_back( "GUN", string_format( _( "Barrel Length: <info>%d %s</info>" ),
                               convert_length( mod->barrel_length() ), length_units( mod->barrel_length() ) ),
                               iteminfo::no_flags );
        }
    }

    if( mod->ammo_required() && curammo->ammo->critical_multiplier != 1.0 ) {
        if( parts->test( iteminfo_parts::AMMO_DAMAGE_CRIT_MULTIPLIER ) ) {
            info.emplace_back( "GUN", _( "Critical multiplier: " ), "<num>",
                               iteminfo::no_flags, curammo->ammo->critical_multiplier );
        }
    }

    avatar &player_character = get_avatar();
    int max_gun_range = loaded_mod->gun_range( &player_character );
    if( max_gun_range > 0 && parts->test( iteminfo_parts::GUN_MAX_RANGE ) ) {
        info.emplace_back( "GUN", _( "Maximum range: " ), "<num>", iteminfo::no_flags,
                           max_gun_range );
    }

    // TODO: This doesn't cover multiple damage types

    int ammo_pierce = 0;
    if( mod->ammo_required() ) {
        ammo_pierce = get_ranged_pierce( *curammo->ammo );
    }

    if( parts->test( iteminfo_parts::GUN_ARMORPIERCE ) ) {
        info.emplace_back( "GUN", _( "Armor-pierce: " ), "",
                           iteminfo::no_newline, get_ranged_pierce( gun ) + ammo_pierce );
    }
    info.back().bNewLine = true;

    double gun_dispersion = mod->gun_dispersion( false, false ) / 100.0;
    if( mod->ammo_required() ) {
        gun_dispersion = loaded_mod->gun_dispersion( true, false ) / 100.0;
    }
    if( parts->test( iteminfo_parts::GUN_DISPERSION ) ) {
        info.emplace_back( "GUN", _( "Dispersion: " ), _( "<num> MOA" ),
                           iteminfo::is_decimal | iteminfo::lower_is_better | iteminfo::no_newline,
                           gun_dispersion );
    }
    info.back().bNewLine = true;

    // if effective sight dispersion differs from actual sight dispersion display both
    std::pair<int, int> disp = mod->sight_dispersion( player_character );
    const int eff_disp = disp.second;
    const int point_shooting_limit = player_character.point_shooting_limit( *mod );

    if( parts->test( iteminfo_parts::GUN_DISPERSION_SIGHT ) ) {
        if( point_shooting_limit <= eff_disp ) {
            info.emplace_back( "GUN", _( "Sight dispersion (point shooting): " ), _( "<num> MOA" ),
                               iteminfo::is_decimal | iteminfo::no_newline | iteminfo::lower_is_better,
                               point_shooting_limit / 100.0 );
        } else {
            info.emplace_back( "GUN", _( "Sight dispersion: " ), _( "<num> MOA" ),
                               iteminfo::is_decimal | iteminfo::no_newline | iteminfo::lower_is_better,
                               eff_disp / 100.0 );
        }
    }

    bool bipod = mod->has_flag( flag_BIPOD );
    info.back().bNewLine = true;
    if( loaded_mod->gun_recoil( player_character ) ) {
        if( parts->test( iteminfo_parts::GUN_RECOIL ) ) {
            info.emplace_back( "GUN", _( "Effective recoil: " ), "<num> MOA",
                               iteminfo::is_decimal | iteminfo::no_newline | iteminfo::lower_is_better,
                               loaded_mod->gun_recoil( player_character ) / 100.0 );
        }
        if( bipod && parts->test( iteminfo_parts::GUN_RECOIL_BIPOD ) ) {
            info.emplace_back( "GUN", "bipod_recoil", _( " (with bipod <num> MOA)" ),
                               iteminfo::is_decimal | iteminfo::lower_is_better | iteminfo::no_name,
                               loaded_mod->gun_recoil( player_character, true ) / 100.0 );
        }

        if( parts->test( iteminfo_parts::GUN_RECOIL_THEORETICAL_MINIMUM ) ) {
            info.back().bNewLine = true;
            info.emplace_back( "GUN", _( "Theoretical minimum recoil: " ), "<num> MOA",
                               iteminfo::is_decimal | iteminfo::no_newline | iteminfo::lower_is_better,
                               loaded_mod->gun_recoil( player_character, true, true ) / 100.0 );
        }
        if( parts->test( iteminfo_parts:: GUN_IDEAL_STRENGTH ) ) {
            info.emplace_back( "GUN", "ideal_strength", _( " (when strength reaches: <num>)" ),
                               iteminfo::lower_is_better | iteminfo::no_name,
                               loaded_mod->gun_base_weight() / 333.0_gram );
        }
    }
    info.back().bNewLine = true;
    std::map<gun_mode_id, gun_mode> fire_modes = mod->gun_all_modes();

    if( parts->test( iteminfo_parts::GUN_RELOAD_TIME ) ) {
        info.emplace_back( "GUN", _( "Reload time: " ),
                           has_flag( flag_RELOAD_ONE ) ? _( "<num> moves per round" ) :
                           _( "<num> moves" ),
                           iteminfo::lower_is_better,  mod->get_reload_time() );
    }

    if( parts->test( iteminfo_parts::GUN_CURRENT_LOUDNESS ) ) {
        const bool is_default_fire_mode = loaded_mod->gun_current_mode().tname() == "DEFAULT";
        //if empty, use the temporary gun loaded with default ammo
        const item::sound_data data = ( mod->ammo_required() &&
                                        !mod->ammo_remaining( ) ) ? tmp.gun_noise( is_default_fire_mode ) : loaded_mod->gun_noise(
                                          is_default_fire_mode );
        const int loudness = data.volume;
        info.emplace_back( "GUN", _( "Loudness with current fire mode: " ), "", iteminfo::lower_is_better,
                           loudness );
    }

    if( parts->test( iteminfo_parts::GUN_USEDSKILL ) ) {
        info.emplace_back( "GUN", _( "Skill used: " ),
                           "<info>" + skill.name() + "</info>" );
    }

    if( mod->magazine_integral() || mod->magazine_current() ) {
        if( mod->magazine_current() && parts->test( iteminfo_parts::GUN_MAGAZINE ) ) {
            info.emplace_back( "GUN", _( "Magazine: " ),
                               string_format( "<stat>%s</stat>",
                                              mod->magazine_current()->tname() ) );
        }
        if( !mod->ammo_types().empty() && parts->test( iteminfo_parts::GUN_CAPACITY ) ) {
            for( const ammotype &at : mod->ammo_types() ) {
                const std::string fmt = string_format( n_gettext( "<num> round of %s",
                                                       "<num> rounds of %s",
                                                       mod->ammo_capacity( at ) ), at->name() );
                info.emplace_back( "GUN", _( "Capacity: " ), fmt, iteminfo::no_flags,
                                   mod->ammo_capacity( at ) );
                std::string ammo_details = print_ammo( at );
                if( !ammo_details.empty() && !magazine_integral() ) {
                    info.emplace_back( "GUN", _( "Variants: " ), ammo_details, iteminfo::no_flags );
                }
            }
        }
    } else if( parts->test( iteminfo_parts::GUN_TYPE ) ) {
        const std::set<ammotype> types_of_ammo = mod->ammo_types();
        if( !types_of_ammo.empty() ) {
            info.emplace_back( "GUN", _( "Type: " ), enumerate_as_string( types_of_ammo,
            []( const ammotype & at ) {
                return at->name();
            }, enumeration_conjunction::none ) );
        }
    }

    if( mod->has_ammo_data() && parts->test( iteminfo_parts::AMMO_REMAINING ) ) {
        info.emplace_back( "AMMO", _( "Ammunition: " ), string_format( "<stat>%s</stat>",
                           mod->ammo_data()->nname( mod->ammo_remaining( ) ) ) );
    }

    if( mod->ammo_required() > 1 && parts->test( iteminfo_parts::AMMO_TO_FIRE ) ) {
        info.emplace_back( "AMMO",  _( "Ammunition consumed per shot: " ), "",
                           iteminfo::lower_is_better, mod->ammo_required() );
    }

    if( mod->get_gun_energy_drain() > 0_kJ && parts->test( iteminfo_parts::AMMO_UPSCOST ) ) {
        info.emplace_back( "AMMO", _( "Energy per shot: " ), string_format( "<stat>%s</stat>",
                           units::display( mod->get_gun_energy_drain() ) ) );
    }

    if( parts->test( iteminfo_parts::GUN_AIMING_STATS ) ) {
        insert_separation_line( info );
        for( const aim_type &type : player_character.get_aim_types( *mod ) ) {
            // Nameless and immediate aim levels don't get an entry.
            if( type.name.empty() || type.threshold == static_cast<int>( MAX_RECOIL ) ) {
                continue;
            }
            // For item comparison to work correctly each info object needs a
            // distinct tag per aim type.
            const std::string tag = "GUN_" + type.name;
            info.emplace_back( tag, string_format( "<info>%s</info>", type.name ) );
            int max_dispersion = player_character.get_weapon_dispersion( *loaded_mod ).max();
            int range = range_with_even_chance_of_good_hit( max_dispersion + type.threshold );
            info.emplace_back( tag, _( "Even chance of good hit at range: " ),
                               _( "<num>" ), iteminfo::no_flags, range );
            int aim_mv = player_character.gun_engagement_moves( *mod, type.threshold );
            info.emplace_back( tag, _( "Time to reach aim level: " ), _( "<num> moves" ),
                               iteminfo::lower_is_better, aim_mv );
        }
    }

    if( parts->test( iteminfo_parts::GUN_FIRE_MODES ) ) {
        std::vector<std::string> fm;
        for( const std::pair<const gun_mode_id, gun_mode> &e : fire_modes ) {
            if( e.second.target == this && !e.second.melee() ) {
                fm.emplace_back( string_format( "%s (%i)", e.second.tname(), e.second.qty ) );
            }
        }
        if( !fm.empty() ) {
            insert_separation_line( info );
            info.emplace_back( "GUN", _( "<bold>Fire modes</bold>: " ) + enumerate_as_string( fm ) );
        }
    }

    if( !magazine_integral() && parts->test( iteminfo_parts::GUN_ALLOWED_MAGAZINES ) ) {
        insert_separation_line( info );
        if( uses_magazine() ) {
            // Keep this identical with tool magazines in item::tool_info
            const std::vector<itype_id> compat_sorted = sorted_lex( magazine_compatible() );
            const std::string mag_names = enumerate_as_string( compat_sorted,
            []( const itype_id & id ) {
                return item::nname( id );
            } );

            const std::set<flag_id> flag_restrictions = contents.magazine_flag_restrictions();
            const std::string flag_names = enumerate_as_string( flag_restrictions,
            []( const flag_id & e ) {
                return e->name();
            } );

            std::string display = _( "<bold>Compatible magazines</bold>:" );
            if( !compat_sorted.empty() ) {
                display += _( "\n<bold>Types</bold>: " ) + mag_names;
            }
            if( !flag_restrictions.empty() ) {
                display += _( "\n<bold>Form factors</bold>: " ) + flag_names;
            }

            info.emplace_back( "DESCRIPTION", display );
        }
    }

    if( !gun.valid_mod_locations.empty() && parts->test( iteminfo_parts::DESCRIPTION_GUN_MODS ) ) {
        insert_separation_line( info );

        std::string mod_str = _( "<bold>Mods</bold>:" );

        std::map<gunmod_location, int> mod_locations = get_mod_locations();

        int iternum = 0;
        mod_str += "\n";
        for( std::pair<const gunmod_location, int> &elem : mod_locations ) {
            if( iternum != 0 ) {
                mod_str += "\n";
            }
            const int free_slots = elem.second - get_free_mod_locations( elem.first );
            mod_str += string_format( "<color_cyan># </color>%s:", elem.first.name() );

            for( const item *gmod : gunmods() ) {
                if( gmod->type->gunmod->location == elem.first ) { // if mod for this location
                    mod_str +=
                        string_format( "\n    <stat>[</stat><color_light_green>● </color><stat>%s]</stat>",
                                       gmod->tname( 1, false ) );
                }
            }
            for( int i = free_slots + 1 ; i <= elem.second ; i++ ) {
                if( i == free_slots + 1 && i <= elem.second ) {
                    mod_str += string_format( "\n   " );
                }
                mod_str += string_format( " <color_dark_gray>[-%s-]</color>", _( "empty" ) );
            }
            iternum++;
        }
        info.emplace_back( "DESCRIPTION", mod_str );
    }

    if( mod->casings_count() && parts->test( iteminfo_parts::DESCRIPTION_GUN_CASINGS ) ) {
        insert_separation_line( info );
        std::string tmp = n_gettext( "Contains <stat>%i</stat> casing",
                                     "Contains <stat>%i</stat> casings", mod->casings_count() );
        info.emplace_back( "DESCRIPTION", string_format( tmp, mod->casings_count() ) );
    }

    if( is_gun() && has_flag( flag_FIRE_TWOHAND ) &&
        parts->test( iteminfo_parts::DESCRIPTION_TWOHANDED ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This weapon needs <info>two free hands</info> to fire." ) );
    }
}

void item::gunmod_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /* batch */,
                        bool /* debug */ ) const
{
    if( !is_gunmod() ) {
        return;
    }
    const islot_gunmod &mod = *type->gunmod;

    if( is_gun() && parts->test( iteminfo_parts::DESCRIPTION_GUNMOD ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This mod <info>must be attached to a gun</info>, "
                              "it can not be fired separately." ) );
    }
    if( has_flag( flag_REACH_ATTACK ) && parts->test( iteminfo_parts::DESCRIPTION_GUNMOD_REACH ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "When attached to a gun, <good>allows</good> making "
                              "<info>reach melee attacks</info> with it." ) );
    }

    if( is_gunmod() && has_flag( flag_DISABLE_SIGHTS ) &&
        parts->test( iteminfo_parts::DESCRIPTION_GUNMOD_DISABLESSIGHTS ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This mod <bad>obscures sights</bad> of the "
                              "base weapon." ) );
    }

    if( is_gunmod() && has_flag( flag_CONSUMABLE ) &&
        parts->test( iteminfo_parts::DESCRIPTION_GUNMOD_CONSUMABLE ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This mod might <bad>suffer wear</bad> when firing "
                              "the base weapon." ) );
    }

    if( mod.dispersion != 0 && parts->test( iteminfo_parts::GUNMOD_DISPERSION ) ) {
        info.emplace_back( "GUNMOD", _( "Dispersion modifier: " ), "<num> MOA",
                           iteminfo::is_decimal | iteminfo::lower_is_better | iteminfo::show_plus,
                           mod.dispersion / 100.0 );
    }
    if( mod.sight_dispersion != -1 && parts->test( iteminfo_parts::GUNMOD_DISPERSION_SIGHT ) ) {
        info.emplace_back( "GUNMOD", _( "Sight dispersion: " ), "<num> MOA",
                           iteminfo::is_decimal | iteminfo::lower_is_better, mod.sight_dispersion / 100.0 );
    }
    if( mod.field_of_view > 0 && parts->test( iteminfo_parts::GUNMOD_FIELD_OF_VIEW ) ) {
        if( mod.field_of_view >= MAX_RECOIL ) {
            info.emplace_back( "GUNMOD", _( "Field of view: <good>No limit</good>" ) );
        } else {
            info.emplace_back( "GUNMOD", _( "Field of view: " ), "",
                               iteminfo::no_flags, mod.field_of_view );
        }
    }
    if( mod.field_of_view > 0 && parts->test( iteminfo_parts::GUNMOD_AIM_SPEED_MODIFIER ) ) {
        info.emplace_back( "GUNMOD", _( "Aim speed modifier: " ), "",
                           iteminfo::no_flags, mod.aim_speed_modifier );
    }
    int total_damage = static_cast<int>( mod.damage.total_damage() );
    if( total_damage != 0 && parts->test( iteminfo_parts::GUNMOD_DAMAGE ) ) {
        info.emplace_back( "GUNMOD", _( "Damage: " ), "", iteminfo::show_plus,
                           total_damage );
    }
    int pierce = get_ranged_pierce( mod );
    if( get_ranged_pierce( mod ) != 0 && parts->test( iteminfo_parts::GUNMOD_ARMORPIERCE ) ) {
        info.emplace_back( "GUNMOD", _( "Armor-pierce: " ), "", iteminfo::show_plus,
                           pierce );
    }
    if( mod.to_hit_mod != 0 && parts->test( iteminfo_parts::GUNMOD_TO_HIT_MODIFIER ) ) {
        info.emplace_back( "GUNMOD", _( "To-hit modifier: " ), "",
                           iteminfo::show_plus, mod.to_hit_mod );
    }
    if( mod.range != 0 && parts->test( iteminfo_parts::GUNMOD_RANGE ) ) {
        info.emplace_back( "GUNMOD", _( "Range: " ), "",
                           iteminfo::show_plus | iteminfo::no_newline, mod.range );
    }
    if( mod.range_multiplier != 1.0f && parts->test( iteminfo_parts::GUNMOD_RANGE_MULTIPLIER ) ) {
        info.emplace_back( "GUNMOD", _( "Range Multiplier: " ), "",
                           iteminfo::is_decimal, mod.range_multiplier );
    }
    if( mod.handling != 0 && parts->test( iteminfo_parts::GUNMOD_HANDLING ) ) {
        info.emplace_back( "GUNMOD", _( "Handling modifier: " ), "",
                           iteminfo::show_plus, mod.handling );
    }
    if( mod.loudness != 0 && parts->test( iteminfo_parts::GUNMOD_LOUDNESS_MODIFIER ) ) {
        info.emplace_back( "GUNMOD", _( "Loudness modifier: " ), "",
                           iteminfo::lower_is_better | iteminfo::show_plus,
                           mod.loudness );
    }
    if( mod.loudness_multiplier != 1.0 && parts->test( iteminfo_parts::GUNMOD_LOUDNESS_MULTIPLIER ) ) {
        info.emplace_back( "GUNMOD", _( "Loudness multiplier: " ), "",
                           iteminfo::lower_is_better | iteminfo::is_decimal,
                           mod.loudness_multiplier );
    }
    if( !type->mod->ammo_modifier.empty() && parts->test( iteminfo_parts::GUNMOD_AMMO ) ) {
        for( const ammotype &at : type->mod->ammo_modifier ) {
            info.emplace_back( "GUNMOD", string_format( _( "Ammo: <stat>%s</stat>" ),
                               at->name() ) );
        }
    }
    if( mod.reload_modifier != 0 && parts->test( iteminfo_parts::GUNMOD_RELOAD ) ) {
        info.emplace_back( "GUNMOD", _( "Reload modifier: " ), _( "<num>%" ),
                           iteminfo::lower_is_better, mod.reload_modifier );
    }
    if( mod.min_str_required_mod > 0 && parts->test( iteminfo_parts::GUNMOD_STRENGTH ) ) {
        info.emplace_back( "GUNMOD", _( "Minimum strength required modifier: " ),
                           mod.min_str_required_mod );
    }
    if( !mod.add_mod.empty() && parts->test( iteminfo_parts::GUNMOD_ADD_MOD ) ) {
        insert_separation_line( info );

        std::string mod_loc_str = _( "<bold>Adds mod locations: </bold> " );

        std::map<gunmod_location, int> mod_locations = mod.add_mod;

        int iternum = 0;
        for( std::pair<const gunmod_location, int> &elem : mod_locations ) {
            if( iternum != 0 ) {
                mod_loc_str += "; ";
            }
            mod_loc_str += string_format( "<bold>%s</bold> %s", elem.second, elem.first.name() );
            iternum++;
        }
        mod_loc_str += ".";
        info.emplace_back( "GUNMOD", mod_loc_str );
    }

    insert_separation_line( info );

    if( parts->test( iteminfo_parts::GUNMOD_USEDON ) ) {
        std::set<std::string> used_on_translated; // uses set to prune duplicates
        for( const gun_type_type &gtt : mod.usable ) {
            used_on_translated.emplace( string_format( "<info>%s</info>", gtt.name() ) );
        }
        info.emplace_back( "GUNMOD", _( "Used on: " ) + enumerate_as_string( used_on_translated ) );
    }

    if( parts->test( iteminfo_parts::GUNMOD_LOCATION ) ) {
        info.emplace_back( "GUNMOD", string_format( _( "Location: %s" ), mod.location.name() ) );
    }

    if( !mod.blacklist_slot.empty() && parts->test( iteminfo_parts::GUNMOD_BLACKLIST_SLOT ) ) {
        std::string mod_black_str = _( "<bold>Incompatible with gunmods: </bold> " );

        int iternum = 0;
        for( const gunmod_location &black : mod.blacklist_slot ) {
            if( iternum != 0 ) {
                mod_black_str += ", ";
            }
            mod_black_str += string_format( "%s", ( black.name() ) );
            iternum++;
        }
        mod_black_str += ".";
        info.emplace_back( "GUNMOD", mod_black_str );
    }
    if( !mod.blacklist_mod.empty() && parts->test( iteminfo_parts::GUNMOD_BLACKLIST_MOD ) ) {
        std::string mod_black_str = _( "<bold>Incompatible with slots: </bold> " );

        int iternum = 0;
        for( const itype_id &black : mod.blacklist_mod ) {
            if( iternum != 0 ) {
                mod_black_str += ", ";
            }
            mod_black_str += string_format( "%s", ( black->nname( 1 ) ) );
            iternum++;
        }
        mod_black_str += ".";
        info.emplace_back( "GUNMOD", mod_black_str );
    }
}

static bool armor_encumb_header_info( const item &it, std::vector<iteminfo> &info )
{
    std::string format;
    Character &player_character = get_player_character();
    const bool sizing_matters = it.get_sizing( player_character ) != item::sizing::ignore;

    if( it.has_flag( flag_FIT ) ) {
        format = _( " <info>(fits)</info>" );
    } else if( it.has_flag( flag_VARSIZE ) && sizing_matters ) {
        format = _( " <bad>(poor fit)</bad>" );
    }
    if( sizing_matters ) {
        const item::sizing sizing_level = it.get_sizing( player_character );
        //If we have the wrong size, we do not fit so alert the player
        if( sizing_level == item::sizing::human_sized_small_char ) {
            format = _( " <bad>(too big)</bad>" );
        } else if( sizing_level == item::sizing::big_sized_small_char ) {
            format = _( " <bad>(huge!)</bad>" );
        } else if( sizing_level == item::sizing::small_sized_human_char ||
                   sizing_level == item::sizing::human_sized_big_char ) {
            format = _( " <bad>(too small)</bad>" );
        } else if( sizing_level == item::sizing::small_sized_big_char ) {
            format = _( " <bad>(tiny!)</bad>" );
        }
    }
    if( format.empty() ) {
        return false;
    }

    //~ The size or fit of an article of clothing (fits / poor fit)
    info.emplace_back( "ARMOR", _( "<bold>Size/Fit</bold>:" ) + format );
    return true;
}

bool item::armor_full_protection_info( std::vector<iteminfo> &info,
                                       const iteminfo_query *parts ) const
{
    bool divider_needed = false;
    const std::string space = "  ";

    bool ret = false;
    if( const islot_armor *t = find_armor_data() ) {
        if( t->data.empty() ) {
            return ret;
        }

        for( const armor_portion_data &p : t->sub_data ) {
            if( divider_needed ) {
                insert_separation_line( info );
            }

            std::vector<sub_bodypart_id> covered( p.sub_coverage.size() );
            std::transform( p.sub_coverage.begin(), p.sub_coverage.end(),
            covered.begin(), []( sub_bodypart_str_id a ) {
                return a.id();
            } );
            std::set<translation, localized_comparator> to_print = body_part_type::consolidate( covered );
            std::string coverage = _( "<bold>Protection for</bold>:" );
            for( const translation &entry : to_print ) {
                coverage += string_format( _( " The <info>%s</info>." ), entry );
            }
            info.emplace_back( "DESCRIPTION", coverage );

            // the following function need one representative sub limb from which to query data
            armor_material_info( info, parts, 0, false, *p.sub_coverage.begin() );
            armor_attribute_info( info, parts, 0, false, *p.sub_coverage.begin() );
            armor_protection_info( info, parts, 0, false, *p.sub_coverage.begin() );
            ret = true;
            divider_needed = true;
        }
    } else if( is_gun() && has_flag( flag_IS_ARMOR ) ) {
        if( divider_needed ) {
            insert_separation_line( info );
        }
        //right now all eligible gunmods (shoulder_strap, belt_clip) have the is_armor flag and use the torso
        info.emplace_back( "ARMOR", _( "Torso:" ) + space, "",
                           iteminfo::no_flags | iteminfo::lower_is_better, get_avg_encumber( get_avatar() ) );

        //~ Limb-specific coverage (for guns strapped to torso)
        info.emplace_back( "DESCRIPTION", _( "<bold>Torso coverage</bold>:" ) );
        //~ Regular/Default coverage
        info.emplace_back( "ARMOR", space + _( "Default:" ) + space, "",
                           iteminfo::no_flags, get_coverage( body_part_torso.id() ) );
        //~ Melee coverage
        info.emplace_back( "ARMOR", space + _( "Melee:" ) + space, "",
                           iteminfo::no_flags, get_coverage( body_part_torso.id(), cover_type::COVER_MELEE ) );
        //~ Ranged coverage
        info.emplace_back( "ARMOR", space + _( "Ranged:" ) + space, "",
                           iteminfo::no_flags, get_coverage( body_part_torso.id(), cover_type::COVER_RANGED ) );
        //~ Vitals coverage
        info.emplace_back( "ARMOR", space + _( "Vitals:" ) + space, "",
                           iteminfo::no_flags, get_coverage( body_part_torso.id(), cover_type::COVER_VITALS ) );
    }

    return ret;
}

static void armor_protect_dmg_info( int dmg, std::vector<iteminfo> &info )
{
    if( dmg > 0 ) {
        info.emplace_back( "ARMOR", _( "Protection values are <bad>reduced by damage</bad> and "
                                       "you may be able to <info>improve them by repairing this "
                                       "item</info>." ) );
    }
}

std::string item::layer_to_string( layer_level data )
{
    switch( data ) {
        case layer_level::PERSONAL:
            return _( "Personal aura" );
        case layer_level::SKINTIGHT:
            return _( "Close to skin" );
        case layer_level::NORMAL:
            return _( "Normal" );
        case layer_level::WAIST:
            return _( "Waist" );
        case layer_level::OUTER:
            return _( "Outer" );
        case layer_level::BELTED:
            return _( "Strapped" );
        case layer_level::AURA:
            return _( "Outer aura" );
        default:
            return _( "Should never see this" );
    }
}

void item::armor_protection_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                                  int /*batch*/, bool /*debug*/, const sub_bodypart_id &sbp ) const
{
    if( !is_armor() && !is_pet_armor() ) {
        return;
    }

    if( parts->test( iteminfo_parts::ARMOR_PROTECTION ) ) {
        const std::string space = "  ";
        // NOLINTNEXTLINE(cata-translate-string-literal)
        std::string bp_cat = string_format( "ARMOR" );
        std::string layering;

        // get the layers this bit of the armor covers if its unique compared to the rest of the armor
        for( const layer_level &ll : get_layer( sbp ) ) {
            layering += string_format( "<stat>%s</stat>. ", item::layer_to_string( ll ) );
        }
        // remove extra space from the end
        if( !layering.empty() ) {
            layering.pop_back();
        }

        std::string coverage_table;
        //~ Limb-specific coverage (%s = name of limb)
        coverage_table += string_format( _( "<bold>Coverage</bold>;%s\n" ), layering );
        //~ Regular/Default coverage
        coverage_table += string_format( "%s;<color_c_yellow>%d</color>\n", _( "Default" ),
                                         get_coverage( sbp ) );
        if( get_coverage( sbp ) != get_coverage( sbp, item::cover_type::COVER_MELEE ) ) {
            //~ Melee coverage
            coverage_table += string_format( "%s;<color_c_yellow>%d</color>\n", _( "Melee" ), get_coverage( sbp,
                                             item::cover_type::COVER_MELEE ) );
        }
        if( get_coverage( sbp ) != get_coverage( sbp, item::cover_type::COVER_RANGED ) ) {
            //~ Ranged coverage
            coverage_table += string_format( "%s;<color_c_yellow>%d</color>\n", _( "Ranged" ),
                                             get_coverage( sbp,
                                                     item::cover_type::COVER_RANGED ) );
        }
        if( get_coverage( sbp, item::cover_type::COVER_VITALS ) > 0 ) {
            //~ Vitals coverage
            coverage_table += string_format( "%s;<color_c_yellow>%d</color>\n", _( "Vitals" ),
                                             get_coverage( sbp,
                                                     item::cover_type::COVER_VITALS ) );
        }

        info.emplace_back( bp_cat, coverage_table, iteminfo::is_table );

        bool printed_any = false;

        // gather all the protection data
        // the rolls are basically a perfect hit for protection and a
        // worst possible
        resistances worst_res = resistances( *this, false, 99, sbp );
        resistances best_res = resistances( *this, false, 0, sbp );
        resistances median_res = resistances( *this, false, 50, sbp );

        int percent_best = 100;
        int percent_worst = 0;
        const armor_portion_data *portion = portion_for_bodypart( sbp );
        // if there isn't a portion this is probably pet armor
        if( portion ) {
            percent_best = portion->best_protection_chance;
            percent_worst = portion->worst_protection_chance;
        }

        bool display_median = percent_best < 50 && percent_worst < 50;

        std::string protection_table;
        const std::string worst_chance_string = string_format( _( "<bad>%d%%</bad> chance" ),
                                                percent_worst );
        const std::string median_chance = _( "<color_c_yellow>Median</color> chance" );
        const std::string best_chance_string = string_format( _( "<good>%d%%</good> chance" ),
                                               percent_best );
        if( display_median ) {
            protection_table +=
                string_format( "<bold>%s</bold>;%s;%s;%s\n",
                               _( "Protection" ), worst_chance_string, median_chance, best_chance_string );
        } else if( percent_worst > 0 ) {
            protection_table +=
                string_format( "<bold>%s</bold>;%s;%s\n",
                               _( "Protection" ), worst_chance_string, best_chance_string );
        } else {
            protection_table += string_format( "<bold>%s</bold>\n", _( "Protection" ) );
        }

        for( const damage_info_order &dio : damage_info_order::get_all(
                 damage_info_order::info_type::PROT ) ) {
            if( best_res.resist_vals.count( dio.dmg_type ) <= 0 ||
                best_res.type_resist( dio.dmg_type ) < 1.0f ) {
                continue;
            }
            bool skipped_detailed = false;
            if( dio.info_display == damage_info_order::info_disp::DETAILED ) {
                if( display_median ) {
                    protection_table +=
                        string_format( "%s;<bad>%.2f</bad>;<color_c_yellow>%.2f</color>;<good>%.2f</good>\n",
                                       uppercase_first_letter( dio.dmg_type->name.translated() ), worst_res.type_resist( dio.dmg_type ),
                                       median_res.type_resist( dio.dmg_type ), best_res.type_resist( dio.dmg_type ) );
                    printed_any = true;
                } else if( percent_worst > 0 ) {
                    protection_table += string_format( "%s;<bad>%.2f</bad>;<good>%.2f</good>\n",
                                                       uppercase_first_letter( dio.dmg_type->name.translated() ), worst_res.type_resist( dio.dmg_type ),
                                                       best_res.type_resist( dio.dmg_type ) );
                    printed_any = true;
                } else {
                    skipped_detailed = true;
                }
            }
            if( skipped_detailed || dio.info_display == damage_info_order::info_disp::BASIC ) {
                protection_table += string_format( "%s;<color_c_yellow>%.2f</color>\n",
                                                   uppercase_first_letter( dio.dmg_type->name.translated() ),
                                                   best_res.type_resist( dio.dmg_type ) );
                printed_any = true;
            }
        }
        if( get_base_env_resist( *this ) >= 1 ) {
            protection_table += string_format( "%s;<color_c_yellow>%d</color>\n", _( "Environmental" ),
                                               get_base_env_resist( *this ) );
            printed_any = true;
        }

        iteminfo::flags info_flags = iteminfo::is_table;
        if( !printed_any ) {
            info_flags = info_flags | iteminfo::no_newline;
        }
        info.emplace_back( bp_cat, protection_table, info_flags );

        // if we haven't printed any armor data acknowledge that
        if( !printed_any ) {
            info.emplace_back( bp_cat, string_format( "%s%s", space, _( "Negligible Protection" ) ) );
        }
        if( type->can_use( "GASMASK" ) || type->can_use( "DIVE_TANK" ) ) {
            info.emplace_back( "ARMOR", string_format( "<bold>%s</bold>:",
                               _( "Protection when active" ) ) );
            info.emplace_back( bp_cat, space + _( "Acid: " ), "",
                               iteminfo::no_newline | iteminfo::is_decimal,
                               resist( damage_acid, false, sbp, get_base_env_resist_w_filter() ) );
            info.emplace_back( bp_cat, space + _( "Fire: " ), "",
                               iteminfo::no_newline | iteminfo::is_decimal,
                               resist( damage_heat, false, sbp, get_base_env_resist_w_filter() ) );
            info.emplace_back( bp_cat, space + _( "Environmental: " ),
                               get_env_resist( get_base_env_resist_w_filter() ) );
        }

        if( sbp == sub_bodypart_id() && damage() > 0 ) {
            armor_protect_dmg_info( damage(), info );
        }
    }
}
void item::armor_material_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                                int /*batch*/, bool /*debug*/, const sub_bodypart_id &sbp ) const
{
    if( !is_armor() && !is_pet_armor() ) {
        return;
    }

    if( parts->test( iteminfo_parts::ARMOR_MATERIALS ) ) {
        const std::string space = "  ";
        // NOLINTNEXTLINE(cata-translate-string-literal)
        std::string bp_cat = string_format( "DESCRIPTION" );
        std::string materials_list;
        std::string units_info = pgettext( "length unit: milimeter", "mm" );
        for( const part_material *mat : armor_made_of( sbp ) ) {

            materials_list.append( string_format( "  %s %.2f%s (%d%%).", mat->id->name(), mat->thickness,
                                                  units_info, mat->cover ) );
        }

        info.emplace_back( bp_cat, string_format( "%s%s", _( "<bold>Materials</bold>:" ),
                           materials_list ), "", iteminfo::no_flags );
    }
}

void item::armor_attribute_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                                 int /*batch*/, bool /*debug*/, const sub_bodypart_id &sbp ) const
{
    if( !is_armor() && !is_pet_armor() ) {
        return;
    }

    if( parts->test( iteminfo_parts::ARMOR_ADDITIONAL_INFO ) ) {
        const std::string space = "  ";
        // NOLINTNEXTLINE(cata-translate-string-literal)
        std::string bp_cat = string_format( "ARMOR" );

        if( const armor_portion_data *portion_data = portion_for_bodypart( sbp ) ) {
            if( portion_data->rigid_layer_only ) {
                info.emplace_back( bp_cat, _( "<info>Rigid items only conflict on shared layers</info>" ),
                                   "", iteminfo::no_flags );
            }
        }
    }
}

void item::pet_armor_protection_info( std::vector<iteminfo> &info,
                                      const iteminfo_query *parts ) const
{
    if( !is_pet_armor() ) {
        return;
    }

    if( parts->test( iteminfo_parts::ARMOR_PROTECTION ) ) {
        const std::string space = "  ";

        info.emplace_back( "ARMOR", _( "<bold>Protection</bold>:" ), "" );

        for( const damage_info_order &dio : damage_info_order::get_all(
                 damage_info_order::info_type::PET ) ) {
            info.emplace_back( "ARMOR", string_format( "%s%s: ", space,
                               uppercase_first_letter( dio.dmg_type->name.translated() ) ), "",
                               iteminfo::is_decimal, resist( dio.dmg_type ) );
        }

        info.emplace_back( "ARMOR", space + _( "Environmental: " ),
                           get_base_env_resist( *this ) );

        if( damage() > 0 ) {
            info.emplace_back( "ARMOR",
                               _( "Protection values are <bad>reduced by damage</bad> and "
                                  "you may be able to <info>improve them by repairing this "
                                  "item</info>." ) );
        }
    }
}

// simple struct used for organizing encumbrance in an ordered set
struct armor_encumb_data {
    int encumb;
    int encumb_max;
    int encumb_min;

    bool operator==( const armor_encumb_data &a ) const {
        return encumb == a.encumb &&
               encumb_min == a.encumb_min &&
               encumb_max == a.encumb_max;
    }

    armor_encumb_data( int in_encumb_min, int in_encumb, int in_encumb_max ) {
        encumb = in_encumb;
        encumb_min = in_encumb_min;
        encumb_max = in_encumb_max;
    }
};

static bool operator<( const armor_encumb_data &lhs, const armor_encumb_data &rhs )
{
    return lhs.encumb < rhs.encumb;
}

void item::armor_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                       bool debug ) const
{
    if( !is_armor() ) {
        return;
    }

    const std::string space = "  ";
    body_part_set covered_parts = get_covered_body_parts();
    bool covers_anything = covered_parts.any();
    const bool show_bodygraph = get_option<bool>( "ITEM_BODYGRAPH" ) &&
                                parts->test( iteminfo_parts::ARMOR_BODYGRAPH );

    if( parts->test( iteminfo_parts::ARMOR_BODYPARTS ) ) {
        insert_separation_line( info );
        std::vector<sub_bodypart_id> covered = get_covered_sub_body_parts();
        std::string coverage = _( "<bold>Covers</bold>:" );

        if( !covered.empty() ) {
            std::set<translation, localized_comparator> to_print = body_part_type::consolidate( covered );
            for( const translation &entry : to_print ) {
                coverage += string_format( _( " The <info>%s</info>." ), entry );
            }
        }

        if( !covers_anything ) {
            coverage += _( " <info>Nothing</info>." );
        }

        info.emplace_back( "ARMOR", coverage );
    }

    if( parts->test( iteminfo_parts::ARMOR_RIGIDITY ) && ( is_rigid() || is_ablative() ) ) {
        // if the item has no armor data it doesn't cover that part
        const islot_armor *armor = find_armor_data();
        if( armor != nullptr ) {
            if( armor->rigid ) {
                std::string coverage = _( "<bold>This armor is rigid</bold>" );
                info.emplace_back( "ARMOR", coverage );
            } else {
                // only some parts are rigid
                std::string coverage = _( "<bold>Rigid Locations</bold>:" );
                std::set<sub_bodypart_id> covered;
                // if ablative check if there are hard plates in locations
                if( armor->ablative ) {
                    // if item has ablative armor we should check those too.
                    for( const item_pocket *pocket : get_all_contained_pockets() ) {
                        // if the pocket is ablative and not empty we should use its values
                        if( pocket->get_pocket_data()->ablative && !pocket->empty() ) {
                            // get the contained plate
                            const item &ablative_armor = pocket->front();
                            for( const armor_portion_data &entry : ablative_armor.find_armor_data()->sub_data ) {
                                if( entry.rigid ) {
                                    for( const sub_bodypart_str_id &sbp : entry.sub_coverage ) {
                                        covered.emplace( sbp );
                                    }
                                }
                            }
                        }
                    }
                }
                for( const armor_portion_data &entry : armor->sub_data ) {
                    if( entry.rigid ) {
                        for( const sub_bodypart_str_id &sbp : entry.sub_coverage ) {
                            covered.emplace( sbp );
                        }
                    }
                }

                if( !covered.empty() ) {
                    std::vector<sub_bodypart_id> covered_vec( covered.begin(), covered.end() );
                    std::set<translation, localized_comparator> to_print = body_part_type::consolidate( covered_vec );
                    for( const translation &entry : to_print ) {
                        coverage += string_format( _( " The <info>%s</info>." ), entry );
                    }
                    info.emplace_back( "ARMOR", coverage );
                }
            }
        }
    }

    if( parts->test( iteminfo_parts::ARMOR_RIGIDITY ) && is_comfortable() ) {
        // if the item has no armor data it doesn't cover that part
        const islot_armor *armor = find_armor_data();
        if( armor != nullptr ) {
            if( armor->comfortable ) {
                std::string coverage = _( "<bold>This armor is comfortable</bold>" );
                info.emplace_back( "ARMOR", coverage );
            } else {
                // only some parts are comfortable
                std::string coverage = _( "<bold>Comfortable Locations</bold>:" );
                std::vector<sub_bodypart_id> covered;
                for( const armor_portion_data &entry : armor->sub_data ) {
                    if( entry.comfortable ) {
                        for( const sub_bodypart_str_id &sbp : entry.sub_coverage ) {
                            covered.emplace_back( sbp );
                        }
                    }
                }
                if( !covered.empty() ) {
                    std::set<translation, localized_comparator> to_print = body_part_type::consolidate( covered );
                    for( const translation &entry : to_print ) {
                        coverage += string_format( _( " The <info>%s</info>." ), entry );
                    }
                    info.emplace_back( "ARMOR", coverage );
                }
            }
        }
    }

    if( parts->test( iteminfo_parts::ARMOR_COVERAGE ) && covers_anything ) {
        std::map<int, std::vector<bodypart_id>> limb_groups;
        for( const bodypart_str_id &bp : get_covered_body_parts() ) {
            const armor_portion_data *portion = portion_for_bodypart( bp );
            if( portion ) {
                limb_groups[portion->coverage].emplace_back( bp );
            }
        }
        // keep on one line if only 1 entry
        if( limb_groups.size() == 1 ) {
            info.emplace_back( "ARMOR", _( "<bold>Total Coverage</bold>" ), iteminfo::no_newline );
        } else {
            info.emplace_back( "ARMOR", _( "<bold>Total Coverage</bold>:" ) );
        }

        for( auto &entry : limb_groups ) {
            std::set<translation, localized_comparator> to_print = body_part_type::consolidate( entry.second );
            std::string coverage;
            for( const translation &entry : to_print ) {
                coverage += string_format( _( " The <info>%s</info>." ), entry );
            }
            info.emplace_back( "ARMOR", "", string_format( "  <num>%%:%s", coverage ), iteminfo::no_flags,
                               entry.first );
        }
    }

    if( parts->test( iteminfo_parts::ARMOR_ENCUMBRANCE ) && covers_anything ) {
        std::map<armor_encumb_data, std::vector<bodypart_id>> limb_groups;
        const Character &c = get_player_character();

        armor_encumb_header_info( *this, info );
        for( const bodypart_str_id &bp : get_covered_body_parts() ) {
            armor_encumb_data encumbrance( get_encumber( c, bp, item::encumber_flags::assume_empty ),
                                           get_encumber( c, bp ), get_encumber( c, bp, item::encumber_flags::assume_full ) );

            limb_groups[encumbrance].emplace_back( bp );
        }
        // keep on one line if only 1 entry
        if( limb_groups.size() == 1 ) {
            info.emplace_back( "ARMOR", _( "<bold>Encumbrance</bold>" ), iteminfo::no_newline );
        } else {
            info.emplace_back( "ARMOR", _( "<bold>Encumbrance</bold>:" ) );
        }
        for( auto &entry : limb_groups ) {
            std::set<translation, localized_comparator> to_print = body_part_type::consolidate( entry.second );
            std::string coverage;
            std::string encumbrance_info;
            for( const translation &entry : to_print ) {
                coverage += string_format( _( " The <info>%s</info>." ), entry );
            }

            //encumbrance_info = string_format( "%d", entry.first.encumb );

            // NOLINTNEXTLINE(cata-translate-string-literal)
            info.emplace_back( "ARMOR", space, "", iteminfo::no_newline | iteminfo::lower_is_better,
                               entry.first.encumb );

            // if it has a max value
            if( entry.first.encumb != entry.first.encumb_max ) {
                std::string when_full_message = _( ", When full" ) + space;
                info.emplace_back( "ARMOR", when_full_message, "", iteminfo::no_newline | iteminfo::lower_is_better,
                                   entry.first.encumb_max );
            }

            // if it has a min value
            if( entry.first.encumb != entry.first.encumb_min ) {
                std::string when_empty_message = _( ", When empty" ) + space;
                info.emplace_back( "ARMOR", when_empty_message, "",
                                   iteminfo::no_newline | iteminfo::lower_is_better, entry.first.encumb_min );
            }
            info.emplace_back( "ARMOR", string_format( ":%s", coverage ) );
        }
    }

    if( parts->test( iteminfo_parts::ARMOR_WARMTH ) && covers_anything ) {
        // TODO: Make this less inflexible on anatomy
        std::map<int, std::vector<bodypart_id>> limb_groups;
        for( const bodypart_str_id &bp : get_covered_body_parts() ) {
            limb_groups[get_warmth( bp )].emplace_back( bp );
        }
        // keep on one line if only 1 entry
        if( limb_groups.size() == 1 ) {
            info.emplace_back( "ARMOR", _( "<bold>Warmth</bold>" ), iteminfo::no_newline );
        } else {
            info.emplace_back( "ARMOR", _( "<bold>Warmth</bold>:" ) );
        }
        for( auto &entry : limb_groups ) {
            std::set<translation, localized_comparator> to_print = body_part_type::consolidate( entry.second );
            std::string coverage;
            for( const translation &entry : to_print ) {
                coverage += string_format( _( " The <info>%s</info>." ), entry );
            }
            info.emplace_back( "ARMOR", "", string_format( "  <num>:%s", coverage ), iteminfo::no_flags,
                               entry.first );
        }
    }

    if( parts->test( iteminfo_parts::ARMOR_BREATHABILITY ) && covers_anything ) {

        std::map<int, std::vector<bodypart_id>> limb_groups;
        for( const bodypart_str_id &bp : get_covered_body_parts() ) {
            const armor_portion_data *portion = portion_for_bodypart( bp );
            if( portion ) {
                limb_groups[portion->breathability].emplace_back( bp );
            }
        }
        // keep on one line if only 1 entry
        if( limb_groups.size() == 1 ) {
            info.emplace_back( "ARMOR", _( "<bold>Breathability</bold>" ), iteminfo::no_newline );
        } else {
            info.emplace_back( "ARMOR", _( "<bold>Breathability</bold>:" ) );
        }
        for( auto &entry : limb_groups ) {
            std::set<translation, localized_comparator> to_print = body_part_type::consolidate( entry.second );
            std::string coverage;
            for( const translation &entry : to_print ) {
                coverage += string_format( _( " The <info>%s</info>." ), entry );
            }
            info.emplace_back( "ARMOR", "", string_format( "  <num>%%:%s", coverage ), iteminfo::no_flags,
                               entry.first );
        }
    }

    insert_separation_line( info );

    if( covers_anything ) {

        if( is_power_armor() || type->get_id() == itype_rm13_armor ) {
            item tmp = *this;

            //no need to clutter the ui with inactive versions when the armor is already active
            if( !( active || ( type->tool && type->tool->power_draw > 0_W ) ) ) {
                bool print_prot = true;
                if( parts->test( iteminfo_parts::ARMOR_PROTECTION ) ) {
                    print_prot = !tmp.armor_full_protection_info( info, parts );
                }
                if( print_prot ) {
                    tmp.armor_protection_info( info, parts, batch, debug );
                }
                armor_protect_dmg_info( tmp.damage(), info );

                insert_separation_line( info );
                info.emplace_back( "ARMOR", _( "<bold>When active</bold>:" ) );
                tmp = tmp.convert( itype_id( tmp.typeId().str() + "_on" ) );
            }

            bool print_prot = true;
            if( parts->test( iteminfo_parts::ARMOR_PROTECTION ) ) {
                print_prot = !tmp.armor_full_protection_info( info, parts );
            }
            if( print_prot ) {
                tmp.armor_protection_info( info, parts, batch, debug );
            }
            armor_protect_dmg_info( tmp.damage(), info );
        } else {
            bool print_prot = true;
            if( parts->test( iteminfo_parts::ARMOR_PROTECTION ) ) {
                print_prot = !armor_full_protection_info( info, parts );
            }
            if( print_prot ) {
                armor_protection_info( info, parts, batch, debug );
            }
            armor_protect_dmg_info( damage(), info );
        }
    }

    // Whatever the last entry was, we want a newline at this point
    info.back().bNewLine = true;

    if( show_bodygraph ) {
        insert_separation_line( info );

        auto bg_cb = [this]( const bodygraph_part * bgp, const std::string & sym ) {
            if( !bgp ) {
                return colorize( sym, bodygraph_full_body_iteminfo->fill_color );
            }
            std::set<sub_bodypart_id> grp{ bgp->sub_bodyparts.begin(), bgp->sub_bodyparts.end() };
            for( const bodypart_id &bid : bgp->bodyparts ) {
                grp.insert( bid->sub_parts.begin(), bid->sub_parts.end() );
            }
            nc_color clr = c_dark_gray;
            int cov_val = 0;
            for( const sub_bodypart_id &sid : grp ) {
                int tmp = get_coverage( sid );
                cov_val = tmp > cov_val ? tmp : cov_val;
            }
            if( cov_val <= 5 ) {
                clr = c_dark_gray;
            } else if( cov_val <= 10 ) {
                clr = c_light_gray;
            } else if( cov_val <= 25 ) {
                clr = c_light_red;
            } else if( cov_val <= 60 ) {
                clr = c_yellow;
            } else if( cov_val <= 80 ) {
                clr = c_green;
            } else {
                clr = c_light_green;
            }
            return colorize( sym, clr );
        };
        std::vector<std::string> bg_lines = get_bodygraph_lines( get_player_character(), bg_cb,
                                            bodygraph_full_body_iteminfo );
        for( const std::string &line : bg_lines ) {
            info.emplace_back( "ARMOR", line, iteminfo::is_art );
        }
        insert_separation_line( info );
    }
}

void item::animal_armor_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                              int /*batch*/, bool /*debug*/ ) const
{
    if( !is_pet_armor() ) {
        return;
    }
    pet_armor_protection_info( info, parts );
}

void item::armor_fit_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /*batch*/,
                           bool /*debug*/ ) const
{
    if( !is_armor() ) {
        return;
    }

    Character &player_character = get_player_character();
    const sizing sizing_level = get_sizing( player_character );

    if( parts->test( iteminfo_parts::DESCRIPTION_FLAGS_FITS ) ) {
        switch( sizing_level ) {
            case sizing::human_sized_human_char:
                if( has_flag( flag_FIT ) ) {
                    info.emplace_back( "DESCRIPTION",
                                       _( "* This clothing <info>fits</info> you perfectly." ) );
                }
                break;
            case sizing::big_sized_big_char:
                if( has_flag( flag_FIT ) ) {
                    info.emplace_back( "DESCRIPTION", _( "* This clothing <info>fits</info> "
                                                         "your large frame perfectly." ) );
                }
                break;
            case sizing::small_sized_small_char:
                if( has_flag( flag_FIT ) ) {
                    info.emplace_back( "DESCRIPTION", _( "* This clothing <info>fits</info> "
                                                         "your small frame perfectly." ) );
                }
                break;
            case sizing::big_sized_human_char:
                info.emplace_back( "DESCRIPTION", _( "* This clothing is <bad>oversized</bad> "
                                                     "and does <bad>not fit</bad> you." ) );
                break;
            case sizing::big_sized_small_char:
                info.emplace_back( "DESCRIPTION",
                                   _( "* This clothing is hilariously <bad>oversized</bad> "
                                      "and does <bad>not fit</bad> your <info>abnormally "
                                      "small mutated anatomy</info>." ) );
                break;
            case sizing::human_sized_big_char:
                info.emplace_back( "DESCRIPTION",
                                   _( "* This clothing is <bad>normal sized</bad> and does "
                                      "<bad>not fit</info> your <info>abnormally large "
                                      "mutated anatomy</info>." ) );
                break;
            case sizing::human_sized_small_char:
                info.emplace_back( "DESCRIPTION",
                                   _( "* This clothing is <bad>normal sized</bad> and does "
                                      "<bad>not fit</bad> your <info>abnormally small "
                                      "mutated anatomy</info>." ) );
                break;
            case sizing::small_sized_big_char:
                info.emplace_back( "DESCRIPTION",
                                   _( "* This clothing is hilariously <bad>undersized</bad> "
                                      "and does <bad>not fit</bad> your <info>abnormally "
                                      "large mutated anatomy</info>." ) );
                break;
            case sizing::small_sized_human_char:
                info.emplace_back( "DESCRIPTION", _( "* This clothing is <bad>undersized</bad> "
                                                     "and does <bad>not fit</bad> you." ) );
                break;
            default:
                break;
        }
    }

    if( parts->test( iteminfo_parts::DESCRIPTION_FLAGS_VARSIZE ) ) {
        if( has_flag( flag_VARSIZE ) ) {
            std::string resize_str;
            if( has_flag( flag_FIT ) ) {
                switch( sizing_level ) {
                    case sizing::small_sized_human_char:
                        resize_str = _( "<info>can be upsized</info>" );
                        break;
                    case sizing::human_sized_small_char:
                        resize_str = _( "<info>can be downsized</info>" );
                        break;
                    case sizing::big_sized_human_char:
                    case sizing::big_sized_small_char:
                        resize_str = _( "<bad>can not be downsized</bad>" );
                        break;
                    case sizing::small_sized_big_char:
                    case sizing::human_sized_big_char:
                        resize_str = _( "<bad>can not be upsized</bad>" );
                        break;
                    default:
                        break;
                }
                if( !resize_str.empty() ) {
                    std::string info_str = string_format( _( "* This clothing %s." ), resize_str );
                    info.emplace_back( "DESCRIPTION", info_str );
                }
            } else {
                switch( sizing_level ) {
                    case sizing::small_sized_human_char:
                        resize_str = _( " and <info>upsized</info>" );
                        break;
                    case sizing::human_sized_small_char:
                        resize_str = _( " and <info>downsized</info>" );
                        break;
                    case sizing::big_sized_human_char:
                    case sizing::big_sized_small_char:
                        resize_str = _( " but <bad>not downsized</bad>" );
                        break;
                    case sizing::small_sized_big_char:
                    case sizing::human_sized_big_char:
                        resize_str = _( " but <bad>not upsized</bad>" );
                        break;
                    default:
                        break;
                }
                std::string info_str = string_format( _( "* This clothing <info>can be "
                                                      "refitted</info>%s." ), resize_str );
                info.emplace_back( "DESCRIPTION", info_str );
            }
        } else {
            info.emplace_back( "DESCRIPTION", _( "* This clothing <bad>can not be refitted, "
                                                 "upsized, or downsized</bad>." ) );
        }
    }

    if( is_sided() && parts->test( iteminfo_parts::DESCRIPTION_FLAGS_SIDED ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This item can be worn on <info>either side</info> of "
                              "the body." ) );
    }
    if( is_power_armor() && parts->test( iteminfo_parts::DESCRIPTION_FLAGS_POWERARMOR ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This gear is a part of power armor." ) );
        if( parts->test( iteminfo_parts::DESCRIPTION_FLAGS_POWERARMOR_RADIATIONHINT ) ) {
            if( covers( bodypart_id( "head" ) ) ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "* When worn with a power armor suit, it will "
                                      "<good>fully protect</good> you from "
                                      "<info>radiation</info>." ) );
            } else {
                info.emplace_back( "DESCRIPTION",
                                   _( "* When worn with a power armor helmet, it will "
                                      "<good>fully protect</good> you from "
                                      "<info>radiation</info>." ) );
            }
        }
    }

    if( typeId() == itype_rad_badge && parts->test( iteminfo_parts::DESCRIPTION_IRRADIATION ) ) {
        info.emplace_back( "DESCRIPTION",
                           string_format( _( "* The film strip on the badge is %s." ),
                                          rad_badge_color( irradiation ).first ) );
    }
}

void item::book_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /* batch */,
                      bool /* debug */ ) const
{
    if( !is_book() ) {
        return;
    }

    insert_separation_line( info );
    const islot_book &book = *type->book;
    avatar &player_character = get_avatar();
    // Some things about a book you CAN tell by it's cover.
    if( !book.skill && !type->can_use( "MA_MANUAL" ) && parts->test( iteminfo_parts::BOOK_SUMMARY ) &&
        player_character.studied_all_recipes( *type ) ) {
        info.emplace_back( "BOOK", _( "Just for fun." ) );
    }

    if( type->can_use( "MA_MANUAL" ) && parts->test( iteminfo_parts::BOOK_SUMMARY ) ) {
        info.emplace_back( "BOOK",
                           _( "Some sort of <info>martial arts training "
                              "manual</info>." ) );
        if( player_character.has_identified( typeId() ) ) {
            const matype_id style_to_learn = martial_art_learned_from( *type );
            info.emplace_back( "BOOK",
                               string_format( _( "You can learn <info>%s</info> style "
                                                 "from it." ), style_to_learn->name ) );
            info.emplace_back( "BOOK",
                               string_format( _( "This fighting style is <info>%s</info> "
                                                 "to learn." ),
                                              martialart_difficulty( style_to_learn ) ) );
            info.emplace_back( "BOOK",
                               string_format( _( "It'd be easier to master if you'd have "
                                                 "skill expertise in <info>%s</info>." ),
                                              style_to_learn->primary_skill->name() ) );
        }
    }
    if( book.req == 0 && parts->test( iteminfo_parts::BOOK_REQUIREMENTS_BEGINNER ) ) {
        info.emplace_back( "BOOK", _( "It can be <info>understood by "
                                      "beginners</info>." ) );
    }
    if( player_character.has_identified( typeId() ) ) {
        if( book.skill ) {
            const SkillLevel &skill = player_character.get_skill_level_object( book.skill );
            if( skill.can_train() && parts->test( iteminfo_parts::BOOK_SKILLRANGE_MAX ) ) {
                const std::string skill_name = book.skill->name();
                std::string fmt;
                if( book.level != 0 ) {
                    fmt = string_format( _( "Can bring your <info>%s skill to</info> "
                                            "<num>." ), skill_name );
                    info.emplace_back( "BOOK", "", fmt, iteminfo::no_flags, book.level );
                }
                fmt = string_format( _( "Your current <stat>%s skill</stat> is <num>." ),
                                     skill_name );
                info.emplace_back( "BOOK", "", fmt, iteminfo::no_flags, skill.knowledgeLevel() );
            }

            if( book.req != 0 && parts->test( iteminfo_parts::BOOK_SKILLRANGE_MIN ) ) {
                const std::string fmt = string_format(
                                            _( "<info>Requires %s level</info> <num> to "
                                               "understand." ), book.skill.obj().name() );
                info.emplace_back( "BOOK", "", fmt,
                                   iteminfo::lower_is_better, book.req );
            }
        }

        if( book.intel != 0 && parts->test( iteminfo_parts::BOOK_REQUIREMENTS_INT ) ) {
            info.emplace_back( "BOOK", "",
                               _( "Requires <info>intelligence of</info> <num> to easily "
                                  "read." ), iteminfo::lower_is_better, book.intel );
        }
        if( player_character.book_fun_for( *this, player_character ) != 0 &&
            parts->test( iteminfo_parts::BOOK_MORALECHANGE ) ) {
            info.emplace_back( "BOOK", "",
                               _( "Reading this book affects your morale by <num>" ),
                               iteminfo::show_plus, player_character.book_fun_for( *this, player_character ) );
        }
        if( parts->test( iteminfo_parts::BOOK_TIMEPERCHAPTER ) ) {
            std::string fmt = n_gettext(
                                  "A chapter of this book takes <num> <info>minute to "
                                  "read</info>.",
                                  "A chapter of this book takes <num> <info>minutes to "
                                  "read</info>.", to_minutes<int>( book.time ) );
            if( type->use_methods.count( "MA_MANUAL" ) ) {
                fmt = n_gettext(
                          "<info>A training session</info> with this book takes "
                          "<num> <info>minute</info>.",
                          "<info>A training session</info> with this book takes "
                          "<num> <info>minutes</info>.", to_minutes<int>( book.time ) );
            }
            info.emplace_back( "BOOK", "", fmt,
                               iteminfo::lower_is_better, to_minutes<int>( book.time ) );
        }

        if( book.chapters > 0 && parts->test( iteminfo_parts::BOOK_NUMUNREADCHAPTERS ) ) {
            const int unread = get_remaining_chapters( player_character );
            std::string fmt = n_gettext( "This book has <num> <info>unread chapter</info>.",
                                         "This book has <num> <info>unread chapters</info>.",
                                         unread );
            info.emplace_back( "BOOK", "", fmt, iteminfo::no_flags, unread );
        }

        if( !book.proficiencies.empty() ) {
            const auto enumerate_profs = []( const book_proficiency_bonus & prof ) {
                return prof.id->name();
            };
            const std::string profs = string_format(
                                          _( "This book can help with the following proficiencies: %s" ),
                                          enumerate_as_string( book.proficiencies, enumerate_profs ) );
            info.emplace_back( "BOOK", profs );
        }

        if( parts->test( iteminfo_parts::BOOK_INCLUDED_RECIPES ) ) {
            std::vector<std::string> known_recipe_list;
            std::vector<std::string> learnable_recipe_list;
            std::vector<std::string> practice_recipe_list;
            std::vector<std::string> unlearnable_recipe_list;
            for( const islot_book::recipe_with_description_t &elem : book.recipes ) {
                const bool knows_it = player_character.knows_recipe( elem.recipe );
                // If the player knows it, they recognize it even if it's not clearly stated.
                if( elem.is_hidden() && !knows_it ) {
                    continue;
                }
                const bool can_learn = player_character.get_knowledge_level( elem.recipe->skill_used ) >=
                                       elem.skill_level;
                if( elem.recipe->is_practice() ) {
                    const char *const format = can_learn ? "<dark>%s</dark>" : "<color_brown>%s</color>";
                    practice_recipe_list.emplace_back( string_format( format, elem.recipe->result_name() ) );
                } else if( knows_it ) {
                    // In case the recipe is known, but has a different name in the book, use the
                    // real name to avoid confusing the player.
                    known_recipe_list.emplace_back( string_format( "<bold>%s</bold>", elem.recipe->result_name() ) );
                } else if( !can_learn ) {
                    unlearnable_recipe_list.emplace_back( string_format( "<color_brown>%s</color>", elem.name() ) );
                } else {
                    learnable_recipe_list.emplace_back( string_format( "<dark>%s</dark>", elem.name() ) );
                }
            }

            const std::size_t num_crafting_recipes = known_recipe_list.size() + learnable_recipe_list.size() +
                    unlearnable_recipe_list.size();
            const std::size_t num_total_recipes = num_crafting_recipes + practice_recipe_list.size();
            if( num_total_recipes > 0 && parts->test( iteminfo_parts::DESCRIPTION_BOOK_RECIPES ) ) {
                std::vector<std::string> lines;
                if( num_crafting_recipes > 0 ) {
                    lines.emplace_back(
                        string_format( n_gettext( "This book contains %u crafting recipe.",
                                                  "This book contains %u crafting recipes.",
                                                  num_crafting_recipes ), num_crafting_recipes ) );
                }
                if( !known_recipe_list.empty() ) {
                    lines.emplace_back( _( "You already know how to craft:" ) );
                    lines.emplace_back( enumerate_lcsorted_with_limit( known_recipe_list ) );
                }
                if( !learnable_recipe_list.empty() ) {
                    lines.emplace_back( _( "You understand how to craft:" ) );
                    lines.emplace_back( enumerate_lcsorted_with_limit( learnable_recipe_list ) );
                }
                if( !unlearnable_recipe_list.empty() ) {
                    lines.emplace_back( _( "You lack the skills to understand:" ) );
                    lines.emplace_back( enumerate_lcsorted_with_limit( unlearnable_recipe_list ) );
                }
                if( !practice_recipe_list.empty() ) {
                    lines.emplace_back( _( "This book can help you practice:" ) );
                    lines.emplace_back( enumerate_lcsorted_with_limit( practice_recipe_list ) );
                }
                insert_separation_line( info );
                for( const std::string &recipe_line : lines ) {
                    info.emplace_back( "DESCRIPTION", recipe_line );
                }
            }

            if( num_total_recipes < book.recipes.size() &&
                parts->test( iteminfo_parts::DESCRIPTION_BOOK_ADDITIONAL_RECIPES ) ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "It might help you figuring out some <good>more recipes</good>." ) );
            }
        }

    } else {
        if( parts->test( iteminfo_parts::BOOK_UNREAD ) ) {
            info.emplace_back( "BOOK", _( "You need to <info>read this book to see its contents</info>." ) );
        }
    }
}

void item::battery_info( std::vector<iteminfo> &info, const iteminfo_query * /*parts*/,
                         int /*batch*/, bool /*debug*/ ) const
{
    if( !is_vehicle_battery() ) {
        return;
    }

    std::string info_string;
    if( type->battery->max_capacity < 1_J ) {
        info_string = string_format( _( "<bold>Capacity</bold>: %dmJ" ),
                                     to_millijoule( type->battery->max_capacity ) );
    } else if( type->battery->max_capacity < 1_kJ ) {
        info_string = string_format( _( "<bold>Capacity</bold>: %dJ" ),
                                     to_joule( type->battery->max_capacity ) );
    } else if( type->battery->max_capacity >= 1_kJ ) {
        info_string = string_format( _( "<bold>Capacity</bold>: %dkJ" ),
                                     to_kilojoule( type->battery->max_capacity ) );
    }
    insert_separation_line( info );
    info.emplace_back( "BATTERY", info_string );
}

void item::tool_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /*batch*/,
                      bool /*debug*/ ) const
{
    avatar &player_character = get_avatar();

    if( !is_tool() ) {
        return;
    }

    insert_separation_line( info );
    if( !ammo_types().empty() && parts->test( iteminfo_parts::TOOL_CHARGES ) ) {
        info.emplace_back( "TOOL", string_format( _( "<bold>Charges</bold>: %d" ),
                           ammo_remaining( ) ) );
    }

    if( !magazine_integral() ) {
        if( magazine_current() && parts->test( iteminfo_parts::TOOL_MAGAZINE_CURRENT ) ) {
            info.emplace_back( "TOOL", _( "Magazine: " ),
                               string_format( "<stat>%s</stat>", magazine_current()->tname() ) );
        }

        if( parts->test( iteminfo_parts::TOOL_MAGAZINE_COMPATIBLE ) ) {
            if( uses_magazine() ) {
                // Keep this identical with gun magazines in item::gun_info
                const std::vector<itype_id> compat_sorted = sorted_lex( magazine_compatible() );
                const std::string mag_names = enumerate_as_string( compat_sorted,
                []( const itype_id & id ) {
                    return item::nname( id );
                } );

                const std::set<flag_id> flag_restrictions = contents.magazine_flag_restrictions();
                const std::string flag_names = enumerate_as_string( flag_restrictions,
                []( const flag_id & e ) {
                    return e->name();
                } );

                std::string display = _( "<bold>Compatible magazines</bold>:" );
                if( !compat_sorted.empty() ) {
                    display += _( "\n<bold>Types</bold>: " ) + mag_names;
                }
                if( !flag_restrictions.empty() ) {
                    display += _( "\n<bold>Form factors</bold>: " ) + flag_names;
                }

                info.emplace_back( "DESCRIPTION", display );
            }
        }
    } else if( !ammo_types().empty() && parts->test( iteminfo_parts::TOOL_CAPACITY ) ) {
        for( const ammotype &at : ammo_types() ) {
            info.emplace_back(
                "TOOL",
                "",
                string_format(
                    n_gettext(
                        "Maximum <num> charge of %s.",
                        "Maximum <num> charges of %s.",
                        ammo_capacity( at ) ),
                    at->name() ),
                iteminfo::no_flags,
                ammo_capacity( at ) );
        }
    }

    // UPS, rechargeable power cells, and bionic power
    if( has_flag( flag_USE_UPS ) && parts->test( iteminfo_parts::DESCRIPTION_RECHARGE_UPSMODDED ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This tool has been modified to use a <info>universal "
                              "power supply</info> and is <neutral>not compatible"
                              "</neutral> with <info>standard batteries</info>." ) );
    } else if( has_flag( flag_RECHARGE ) && has_flag( flag_NO_RELOAD ) &&
               parts->test( iteminfo_parts::DESCRIPTION_RECHARGE_NORELOAD ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This tool has a <info>rechargeable power cell</info> "
                              "and is <neutral>not compatible</neutral> with "
                              "<info>standard batteries</info>." ) );
    } else if( has_flag( flag_RECHARGE ) &&
               parts->test( iteminfo_parts::DESCRIPTION_RECHARGE_UPSCAPABLE ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This tool has a <info>rechargeable power cell</info> "
                              "and can be recharged in any <neutral>UPS-compatible "
                              "recharging station</neutral>. You could charge it with "
                              "<info>standard batteries</info>, but unloading it is "
                              "impossible." ) );
    } else if( has_flag( flag_USES_BIONIC_POWER ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This tool <info>runs on bionic power</info>." ) );
    } else if( has_flag( flag_BURNOUT ) && parts->test( iteminfo_parts::TOOL_BURNOUT ) ) {
        int percent_left = 0;
        if( has_ammo_data() ) {
            // Candle items that use ammo instead of charges
            // The capacity should never be zero but clang complains about it
            percent_left = 100 * ammo_remaining( ) / std::max( ammo_capacity( ammo_data()->ammo->type ),
                           1 );
        } else if( type->maximum_charges() > 0 ) {
            // Candle items that use charges instead of ammo
            percent_left = 100 * ammo_remaining( ) / type->maximum_charges();
        }

        std::string feedback;
        if( percent_left == 100 ) {
            feedback = _( "It's new, and ready to burn." );
        } else if( percent_left >= 75 ) {
            feedback = _( "Almost new, with much material to burn." );
        } else if( percent_left >= 50 ) {
            feedback = _( "More than a quarter has burned away." );
        } else if( percent_left >= 25 ) {
            feedback = _( "More than half has burned away." );
        } else if( percent_left >= 10 ) {
            feedback = _( "Less than a quarter left to burn." );
        } else {
            feedback = _( "Almost completely burned out." );
        }
        feedback = _( "<bold>Fuel</bold>: " ) + feedback;
        info.emplace_back( "DESCRIPTION", feedback );
    }

    std::string &eport = type->tool->e_port;
    if( !eport.empty() ) {
        std::string compat;
        compat += _( "* This device has electronic port type: " ) + colorize( _( eport ), c_light_green ) ;
        std::vector<std::string> &eport_banned = type->tool->e_ports_banned;
        if( !eport_banned.empty() ) {
            compat += _( "\n* This device does not support transfer to e-port types: " );
            for( std::string &s : eport_banned ) {
                compat += colorize( _( s ), c_yellow );
            }
        }
        info.emplace_back( "DESCRIPTION", compat );
    }

    //TODO: REMOVE THIS
    // Display e-ink tablet copied recipes from SD cards
    const std::set<recipe_id> saved_recipes = get_saved_recipes();
    if( !saved_recipes.empty() ) {
        std::vector<std::string> known_recipe_list;
        std::vector<std::string> learnable_recipe_list;
        std::vector<std::string> unlearnable_recipe_list;

        for( const recipe_id &rid : saved_recipes ) {
            const recipe *r = &rid.obj();

            const bool knows_it = player_character.knows_recipe( r );
            const bool can_learn = player_character.get_skill_level( r->skill_used ) >= r->difficulty;

            // In case the recipe is known, but has a different name in the item, use the
            // real name to avoid confusing the player.
            const std::string name = r->result_name();

            if( knows_it ) {
                known_recipe_list.emplace_back( "<bold>" + name + "</bold>" );
            } else if( !can_learn ) {
                unlearnable_recipe_list.emplace_back( "<color_brown>" + name + "</color>" );
            } else {
                learnable_recipe_list.emplace_back( "<dark>" + name + "</dark>" );
            }
        }

        int total_recipes = known_recipe_list.size() + learnable_recipe_list.size() +
                            unlearnable_recipe_list.size();

        if( ( !known_recipe_list.empty() || !learnable_recipe_list.empty() ||
              !unlearnable_recipe_list.empty() ) &&
            parts->test( iteminfo_parts::DESCRIPTION_BOOK_RECIPES ) ) {
            const std::string recipe_line =
                string_format( n_gettext( "Contains %1$d copied crafting recipe:",
                                          "Contains %1$d copied crafting recipes:",
                                          total_recipes ), total_recipes );

            insert_separation_line( info );
            info.emplace_back( "DESCRIPTION", recipe_line );

            if( !known_recipe_list.empty() ) {
                const std::string recipe_line =
                    string_format( n_gettext( "\nYou already know %1$d recipe:\n%2$s",
                                              "\nYou already know %1$d recipes:\n%2$s",
                                              known_recipe_list.size() ),
                                   known_recipe_list.size(), enumerate_lcsorted_with_limit( known_recipe_list ) );

                info.emplace_back( "DESCRIPTION", recipe_line );
            }

            if( !learnable_recipe_list.empty() ) {
                const std::string recipe_line =
                    string_format( n_gettext( "\nYou have the skills to craft %1$d recipe:\n%2$s",
                                              "\nYou have the skills to craft %1$d recipes:\n%2$s",
                                              learnable_recipe_list.size() ),
                                   learnable_recipe_list.size(), enumerate_lcsorted_with_limit( learnable_recipe_list ) );

                info.emplace_back( "DESCRIPTION", recipe_line );
            }

            if( !unlearnable_recipe_list.empty() ) {
                const std::string recipe_line =
                    string_format( n_gettext( "\nYou lack the skills to craft %1$d recipe:\n%2$s",
                                              "\nYou lack the skills to craft %1$d recipes:\n%2$s",
                                              unlearnable_recipe_list.size() ),
                                   unlearnable_recipe_list.size(), enumerate_lcsorted_with_limit( unlearnable_recipe_list ) );

                info.emplace_back( "DESCRIPTION", recipe_line );
            }
        }
    }

    // Display e-ink tablet ebook recipes
    if( is_estorage() && !is_broken_on_active() && is_browsed() ) {
        std::vector<std::string> known_recipe_list;
        std::vector<std::string> learnable_recipe_list;
        std::vector<std::string> unlearnable_recipe_list;
        std::vector<const item *> book_list = ebooks();
        int total_ebooks = book_list.size();

        for( const item *ebook : book_list ) {
            const islot_book &book = *ebook->type->book;
            for( const islot_book::recipe_with_description_t &elem : book.recipes ) {
                const bool knows_it = player_character.knows_recipe( elem.recipe );
                const bool can_learn = player_character.get_skill_level( elem.recipe->skill_used ) >=
                                       elem.skill_level;
                // If the player knows it, they recognize it even if it's not clearly stated.
                if( elem.is_hidden() && !knows_it ) {
                    continue;
                }

                // In case the recipe is known, but has a different name in the book, use the
                // real name to avoid confusing the player.
                const std::string name = elem.recipe->result_name( /*decorated=*/true );

                if( knows_it ) {
                    std::string recipe_formated = "<bold>" + name + "</bold>";
                    if( known_recipe_list.end() == std::find( known_recipe_list.begin(), known_recipe_list.end(),
                            name ) ) {
                        known_recipe_list.emplace_back( "<bold>" + name + "</bold>" );
                    }
                } else if( !can_learn ) {
                    std::string recipe_formated = "<color_brown>" + elem.name() + "</color>";
                    if( unlearnable_recipe_list.end() == std::find( unlearnable_recipe_list.begin(),
                            unlearnable_recipe_list.end(), recipe_formated ) ) {
                        unlearnable_recipe_list.emplace_back( recipe_formated );
                    }
                } else {
                    std::string recipe_formated = "<dark>" + elem.name() + "</dark>";
                    if( learnable_recipe_list.end() == std::find( learnable_recipe_list.begin(),
                            learnable_recipe_list.end(), recipe_formated ) ) {
                        learnable_recipe_list.emplace_back( "<dark>" + elem.name() + "</dark>" );
                    }
                }
            }
        }

        int total_recipes = known_recipe_list.size() + learnable_recipe_list.size() +
                            unlearnable_recipe_list.size();

        if( ( !known_recipe_list.empty() || !learnable_recipe_list.empty() ||
              !unlearnable_recipe_list.empty() ) &&
            parts->test( iteminfo_parts::DESCRIPTION_BOOK_RECIPES ) ) {
            std::string recipe_line =
                string_format( n_gettext( "Contains %1$d unique crafting recipe,",
                                          "Contains %1$d unique crafting recipes,",
                                          total_recipes ), total_recipes );
            std::string source_line =
                string_format( n_gettext( "from %1$d stored e-book:",
                                          "from %1$d stored e-books:",
                                          total_ebooks ), total_ebooks );

            insert_separation_line( info );
            info.emplace_back( "DESCRIPTION", recipe_line );
            info.emplace_back( "DESCRIPTION", source_line );

            if( !known_recipe_list.empty() ) {
                const std::string recipe_line =
                    string_format( n_gettext( "\nYou already know %1$d recipe:\n%2$s",
                                              "\nYou already know %1$d recipes:\n%2$s",
                                              known_recipe_list.size() ),
                                   known_recipe_list.size(), enumerate_lcsorted_with_limit( known_recipe_list ) );

                info.emplace_back( "DESCRIPTION", recipe_line );
            }

            if( !learnable_recipe_list.empty() ) {
                std::string recipe_line =
                    string_format( n_gettext( "\nYou have the skills to craft %1$d recipe:\n%2$s",
                                              "\nYou have the skills to craft %1$d recipes:\n%2$s",
                                              learnable_recipe_list.size() ),
                                   learnable_recipe_list.size(), enumerate_lcsorted_with_limit( learnable_recipe_list ) );

                info.emplace_back( "DESCRIPTION", recipe_line );
            }

            if( !unlearnable_recipe_list.empty() ) {
                std::string recipe_line =
                    string_format( n_gettext( "\nYou lack the skills to craft %1$d recipe:\n%2$s",
                                              "\nYou lack the skills to craft %1$d recipes:\n%2$s",
                                              unlearnable_recipe_list.size() ),
                                   unlearnable_recipe_list.size(), enumerate_lcsorted_with_limit( unlearnable_recipe_list ) );

                info.emplace_back( "DESCRIPTION", recipe_line );
            }
        }
    }
}

void item::actions_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                         int /*batch*/, bool /*debug*/ ) const
{

    const std::map<std::string, use_function> &use_methods = type->use_methods;
    if( use_methods.empty() || !parts->test( iteminfo_parts::ACTIONS ) ) {
        return;
    }
    insert_separation_line( info );
    std::string actions = enumerate_as_string( use_methods,
    []( const std::pair<std::string, use_function> &val ) {
        return string_format( "<info>%s</info>", val.second.get_name() );
    } );
    info.emplace_back( "DESCRIPTION", string_format( _( "<bold>Actions</bold>: %s" ), actions ) );
}

void item::component_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /*batch*/,
                           bool /*debug*/ ) const
{
    if( components.empty() || !parts->test( iteminfo_parts::DESCRIPTION_COMPONENTS_MADEFROM ) ) {
        return;
    }
    if( is_craft() ) {
        info.emplace_back( "DESCRIPTION", string_format( _( "Using: %s" ),
                           components_to_string() ) );
    } else {
        info.emplace_back( "DESCRIPTION", string_format( _( "Made from: %s" ),
                           components_to_string() ) );
    }
}

static void enchantment_info_helper( const item &it,
                                     enchantment::has type,
                                     std::vector<std::string> &always,
                                     std::vector<std::string> &active,
                                     std::vector<std::string> &inactive )
{
    for( const enchantment &ench : it.get_defined_enchantments() ) {
        // only check conditions matching the given type
        if( ench.active_conditions.first == type ) {
            // if a description is provided use that
            if( !ench.description.empty() ) {
                std::string info;
                if( !ench.name.empty() ) {
                    info = string_format( "<info>%s</info>: %s", ench.name.translated(),
                                          ench.description.translated() );
                } else {
                    info = string_format( _( "<info>Description</info>: %s" ), ench.description.translated() );
                }
                switch( ench.active_conditions.second ) {
                    case enchantment::condition::ALWAYS:
                        always.push_back( info );
                        break;
                    case enchantment::condition::ACTIVE:
                        active.push_back( info );
                        break;
                    case enchantment::condition::INACTIVE:
                        inactive.push_back( info );
                        break;
                    case enchantment::condition::DIALOG_CONDITION:
                    case enchantment::condition::NUM_CONDITION:
                        // skip for dialog conditions
                        break;
                }
            }
        }
    }
    for( const enchant_cache &ench : it.get_proc_enchantments() ) {
        // only check conditions matching the given type
        if( ench.active_conditions.first == type ) {
            // if a description is provided use that
            if( !ench.description.empty() ) {
                std::string info;
                if( !ench.name.empty() ) {
                    info = string_format( "<info>%s</info>: %s", ench.name.translated(),
                                          ench.description.translated() );
                } else {
                    info = string_format( _( "<info>Description</info>: %s" ), ench.description.translated() );
                }
                switch( ench.active_conditions.second ) {
                    case enchantment::condition::ALWAYS:
                        always.push_back( info );
                        break;
                    case enchantment::condition::ACTIVE:
                        active.push_back( info );
                        break;
                    case enchantment::condition::INACTIVE:
                        inactive.push_back( info );
                        break;
                    case enchantment::condition::DIALOG_CONDITION:
                    case enchantment::condition::NUM_CONDITION:
                        // skip for dialog conditions
                        break;
                }
            }
        }
    }
}

static void enchantment_info_printer( std::vector<iteminfo> &info,
                                      std::vector<std::string> &always,
                                      std::vector<std::string> &active,
                                      std::vector<std::string> &inactive )
{
    if( !always.empty() ) {
        info.emplace_back( "DESCRIPTION", string_format( _( "<bold>Always</bold>:" ) ) );
        for( const std::string &entry : always ) {
            info.emplace_back( "DESCRIPTION", string_format( "* %s", entry ) );
        }
    }
    if( !active.empty() ) {
        info.emplace_back( "DESCRIPTION", string_format( _( "<bold>When Active</bold>:" ) ) );
        for( const std::string &entry : active ) {
            info.emplace_back( "DESCRIPTION", string_format( "* %s", entry ) );
        }
    }
    if( !inactive.empty() ) {
        info.emplace_back( "DESCRIPTION", string_format( _( "<bold>When Inactive</bold>:" ) ) );
        for( const std::string &entry : inactive ) {
            info.emplace_back( "DESCRIPTION", string_format( "* %s", entry ) );
        }
    }
}

void item::enchantment_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                             int /*batch*/, bool /*debug*/ ) const
{
    if( !parts->test( iteminfo_parts::DESCRIPTION_ENCHANTMENTS ) ) {
        return;
    }
    insert_separation_line( info );
    // if the item has relic data and the item is mundane show some details
    if( type->relic_data ) {
        // check WIELD
        std::vector<std::string> always;
        std::vector<std::string> active;
        std::vector<std::string> inactive;
        enchantment_info_helper( *this, enchantment::has::WIELD, always, active, inactive );
        if( !always.empty() || !active.empty() || !inactive.empty() ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "When <bold>wielded</bold> this item provides:" ) ) );
            enchantment_info_printer( info, always, active, inactive );
        }
        always.clear();
        active.clear();
        inactive.clear();
        enchantment_info_helper( *this, enchantment::has::WORN, always, active, inactive );
        if( !always.empty() || !active.empty() || !inactive.empty() ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "When <bold>worn</bold> this item provides:" ) ) );
            enchantment_info_printer( info, always, active, inactive );
        }
        always.clear();
        active.clear();
        inactive.clear();
        enchantment_info_helper( *this, enchantment::has::HELD, always, active, inactive );
        if( !always.empty() || !active.empty() || !inactive.empty() ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "When <bold>carried</bold> this item provides:" ) ) );
            enchantment_info_printer( info, always, active, inactive );
        }
    }
}

void item::repair_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                        int /*batch*/, bool /*debug*/ ) const
{
    if( !parts->test( iteminfo_parts::DESCRIPTION_REPAIREDWITH ) ) {
        return;
    }
    insert_separation_line( info );
    const std::vector<itype_id> rep = sorted_lex( repaired_with() );
    if( !rep.empty() ) {
        info.emplace_back( "DESCRIPTION", string_format( _( "<bold>Repair</bold> using %s." ),
                           enumerate_as_string( rep,
        []( const itype_id & e ) {
            return nname( e );
        }, enumeration_conjunction::or_ ) ) );

        const std::string repairs_with = enumerate_as_string( type->repairs_with,
        []( const material_id & e ) {
            return string_format( "<info>%s</info>", e->name() );
        } );
        if( !repairs_with.empty() ) {
            info.emplace_back( "DESCRIPTION", string_format( _( "<bold>With</bold> %s." ), repairs_with ) );
        }
        if( degradation() > 0 && damage() == degradation() ) {
            info.emplace_back( "DESCRIPTION",
                               string_format(
                                   _( "<color_c_red>Degraded and cannot be repaired beyond the current level.</color>" ) ) );
        }
    } else {
        info.emplace_back( "DESCRIPTION", _( "* This item is <bad>not repairable</bad>." ) );
    }
}

void item::disassembly_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                             int /*batch*/, bool /*debug*/ ) const
{
    if( !components.empty() && parts->test( iteminfo_parts::DESCRIPTION_COMPONENTS_MADEFROM ) ) {
        return;
    }
    if( !parts->test( iteminfo_parts::DESCRIPTION_COMPONENTS_DISASSEMBLE ) ) {
        return;
    }

    const recipe &dis = ( typeId() == itype_disassembly ) ? get_making() :
                        recipe_dictionary::get_uncraft( typeId() );
    const requirement_data &req = dis.disassembly_requirements();
    if( !req.is_empty() ) {
        const std::string approx_time = to_string_approx( dis.time_to_craft( get_player_character(),
                                        recipe_time_flag::ignore_proficiencies ) );

        const requirement_data::alter_item_comp_vector &comps_list = req.get_components();
        const std::string comps_str = enumerate_as_string( comps_list,
        []( const std::vector<item_comp> &comp_opts ) {
            return comp_opts.front().to_string();
        } );

        std::vector<std::string> reqs_list;
        const requirement_data::alter_tool_comp_vector &tools_list = req.get_tools();
        for( const std::vector<tool_comp> &it : tools_list ) {
            if( !it.empty() ) {
                reqs_list.push_back( it.front().to_string() );
            }
        }
        const requirement_data::alter_quali_req_vector &quals_list = req.get_qualities();
        for( const std::vector<quality_requirement> &it : quals_list ) {
            if( !it.empty() ) {
                reqs_list.push_back( it.front().to_colored_string() );
            }
        }

        std::string descr;
        if( reqs_list.empty() ) {
            //~ 1 is approx. time (e.g. 'about 5 minutes'), 2 is a list of items
            descr = string_format( _( "<bold>Disassembly</bold> takes %1$s and might yield: %2$s." ),
                                   approx_time, comps_str );
        } else {
            descr = string_format(
                        //~ 1 is approx. time, 2 is a list of items and tools with qualities, 3 is a list of items.
                        //~ Bold text in the middle makes it easier to see where the second list starts.
                        _( "<bold>Disassembly</bold> takes %1$s, requires %2$s and <bold>might yield</bold>: %3$s." ),
                        approx_time, enumerate_as_string( reqs_list ), comps_str );
        }

        insert_separation_line( info );
        info.emplace_back( "DESCRIPTION", descr );
    }
}

void item::qualities_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /*batch*/,
                           bool /*debug*/ ) const
{
    // Inline function to emplace a given quality (and its level) onto the info vector
    auto name_quality = [&info]( const std::pair<quality_id, int> &q ) {
        std::string str;
        if( q.first == qual_JACK || q.first == qual_LIFT ) {
            //~ %1$d is the numeric quality level, and %2$s is the name of the quality.
            //~ %3$d is the amount of weight the jack can lift, and %4$s is the units of that weight.
            str = string_format( _( "Level <info>%1$d %2$s</info> quality, "
                                    "rated at <info>%3$d</info> %4$s" ),
                                 q.second, q.first.obj().name,
                                 static_cast<int>( convert_weight( lifting_quality_to_mass( q.second ) ) ),
                                 weight_units() );
        } else {
            //~ %1$d is the numeric quality level, and %2$s is the name of the quality
            str = string_format( _( "Level <info>%1$d %2$s</info> quality" ),
                                 q.second, q.first.obj().name );
        }
        info.emplace_back( "QUALITIES", "", str );
    };

    // List all qualities of this item granted by its item type
    const bool has_any_qualities = !type->qualities.empty() || !type->charged_qualities.empty();
    if( parts->test( iteminfo_parts::QUALITIES ) && has_any_qualities ) {
        insert_separation_line( info );
        // List all inherent (unconditional) qualities
        if( !type->qualities.empty() ) {
            info.emplace_back( "QUALITIES", "", _( "<bold>Has qualities</bold>:" ) );
            for( const std::pair<quality_id, int> &q : sorted_lex( type->qualities ) ) {
                name_quality( q );
            }
        }
        // Tools with "charged_qualities" defined may have additional qualities when charged.
        // List them, and show whether there is enough charge to use those qualities.
        if( !type->charged_qualities.empty() && type->charges_to_use() > 0 ) {
            // Use ammo_sufficient() with player character to include bionic/UPS power
            if( ammo_sufficient( &get_player_character() ) ) {
                info.emplace_back( "QUALITIES", "", _( "<good>Has enough charges</good> for qualities:" ) );
            } else {
                info.emplace_back( "QUALITIES", "",
                                   string_format( _( "<bad>Needs %d or more charges</bad> for qualities:" ),
                                                  type->charges_to_use() ) );
            }
            for( const std::pair<quality_id, int> &q : sorted_lex( type->charged_qualities ) ) {
                name_quality( q );
            }
        }
    }

    // Accumulate and list all qualities of items contained within this item
    if( parts->test( iteminfo_parts::QUALITIES_CONTAINED ) &&
    contents.has_any_with( []( const item & e ) {
    return !e.type->qualities.empty();
    }, pocket_type::CONTAINER ) ) {

        info.emplace_back( "QUALITIES", "", _( "Contains items with qualities:" ) );
        std::map<quality_id, int, quality_id::LexCmp> most_quality;
        for( const item *e : contents.all_items_top() ) {
            for( const std::pair<const quality_id, int> &q : e->type->qualities ) {
                auto emplace_result = most_quality.emplace( q );
                if( !emplace_result.second &&
                    most_quality.at( emplace_result.first->first ) < q.second ) {
                    most_quality[ q.first ] = q.second;
                }
            }
        }
        for( const std::pair<const quality_id, int> &q : most_quality ) {
            name_quality( q );
        }
    }
}

void item::bionic_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /*batch*/,
                        bool /*debug*/ ) const
{
    if( !is_bionic() ) {
        return;
    }

    insert_separation_line( info );

    // TODO: Unhide when enforcing limits
    if( get_option < bool >( "CBM_SLOTS_ENABLED" )
        && parts->test( iteminfo_parts::DESCRIPTION_CBM_SLOTS ) ) {
        info.emplace_back( "DESCRIPTION", list_occupied_bps( type->bionic->id,
                           _( "This bionic is installed in the following body "
                              "part(s):" ) ) );
    }

    if( is_bionic() && has_flag( flag_NO_STERILE ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This bionic is <bad>not sterile</bad>, use an <info>autoclave</info> and an <info>autoclave pouch</info> to sterilize it. " ) );
    }
    insert_separation_line( info );

    const bionic_id bid = type->bionic->id;
    const std::vector<material_id> &fuels = bid->fuel_opts;
    if( !fuels.empty() ) {
        const std::string fuels_str = enumerate_as_string( fuels,
        []( const material_id & id ) {
            return "<info>" + id->name() + "</info>";
        } );

        info.emplace_back( "DESCRIPTION",
                           n_gettext( "* This bionic can produce power from the following fuel: ",
                                      "* This bionic can produce power from the following fuels: ", fuels.size() )
                           + fuels_str );
    }

    insert_separation_line( info );

    if( bid->capacity > 0_mJ ) {
        info.emplace_back( "CBM", _( "<bold>Power Capacity</bold>:" ), _( " <num> mJ" ),
                           iteminfo::no_newline,
                           static_cast<double>( units::to_millijoule( bid->capacity ) ) );
    }

    insert_separation_line( info );

    if( bid->required_bionic ) {
        info.emplace_back( "CBM", string_format( "* This CBM requires another CBM to also be installed: %s",
                           bid->required_bionic->name ) );
    }
    insert_separation_line( info );

    // TODO refactor these almost identical blocks
    if( !bid->encumbrance.empty() ) {
        info.emplace_back( "DESCRIPTION", _( "<bold>Encumbrance</bold>:" ),
                           iteminfo::no_newline );
        for( const std::pair<bodypart_str_id, int> &element : sorted_lex( bid->encumbrance ) ) {
            info.emplace_back( "CBM", " " + body_part_name_as_heading( element.first.id(), 1 ),
                               " <num>", iteminfo::no_newline, element.second );
        }
    }

    if( !bid->env_protec.empty() ) {
        info.emplace_back( "DESCRIPTION",
                           _( "<bold>Environmental Protection</bold>:" ),
                           iteminfo::no_newline );
        for( const std::pair< bodypart_str_id, size_t > &element : sorted_lex( bid->env_protec ) ) {
            info.emplace_back( "CBM", " " + body_part_name_as_heading( element.first, 1 ),
                               " <num>", iteminfo::no_newline, static_cast<double>( element.second ) );
        }
    }

    for( const damage_info_order &dio : damage_info_order::get_all(
             damage_info_order::info_type::BIO ) ) {
        std::map<bodypart_str_id, float> dt_protec;
        for( const std::pair<const bodypart_str_id, resistances> &prot : bid->protec ) {
            float res = prot.second.type_resist( dio.dmg_type );
            if( res >= 0.f ) {
                dt_protec.emplace( prot.first, res );
            }
        }
        if( !dt_protec.empty() ) {
            //~ Type of protection (ex: "Bash Protection")
            std::string label = string_format( _( "<bold>%s Protection</bold>:" ),
                                               uppercase_first_letter( dio.dmg_type->name.translated() ) );
            info.emplace_back( "DESCRIPTION", label, iteminfo::no_newline );
            for( const std::pair<bodypart_str_id, float> &element : sorted_lex( dt_protec ) ) {
                info.emplace_back( "CBM", " " + body_part_name_as_heading( element.first, 1 ),
                                   " <num>", iteminfo::no_newline, element.second );
            }
        }
    }

}

void item::melee_combat_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                              int /*batch*/,
                              bool /*debug*/ ) const
{
    const std::string space = "  ";

    std::map<damage_type_id, int> dmg_types;
    for( const damage_type &dt : damage_type::get_all() ) {
        int val = damage_melee( dt.id );
        if( val ) {
            dmg_types.emplace( dt.id, val );
        }
    }

    Character &player_character = get_player_character();
    if( parts->test( iteminfo_parts::BASE_DAMAGE ) ) {
        insert_separation_line( info );
        std::string sep;
        if( !dmg_types.empty() ) {
            info.emplace_back( "BASE", _( "<bold>Melee damage</bold>: " ), "", iteminfo::no_newline );
        }
        for( const auto &dmg_type : dmg_types ) {
            info.emplace_back( "BASE", sep + uppercase_first_letter( dmg_type.first->name.translated() ) + ": ",
                               "", iteminfo::no_newline, dmg_type.second );
            sep = space;
        }
    }

    if( !dmg_types.empty() ) {
        int stam = 0;
        float stam_pct = 0.0f;
        std::map<std::string, double> dps_data;

        bool base_tohit = parts->test( iteminfo_parts::BASE_TOHIT );
        bool base_moves = parts->test( iteminfo_parts::BASE_MOVES );
        bool base_dps = parts->test( iteminfo_parts::BASE_DPS );
        bool base_stamina = parts->test( iteminfo_parts::BASE_STAMINA );
        bool base_dpstam = parts->test( iteminfo_parts::BASE_DPSTAM );

        if( base_stamina || base_dpstam ) {
            stam = player_character.get_total_melee_stamina_cost( this ) * -1;
            stam_pct = std::truncf( stam * 1000.0 / player_character.get_stamina_max() ) / 10;
        }

        if( base_dps || base_dpstam ) {
            dps_data = dps( true, false );
        }

        if( base_tohit ) {
            info.emplace_back( "BASE", space + _( "To-hit bonus: " ), "",
                               iteminfo::show_plus, get_to_hit() );
        }

        if( base_moves ) {
            info.emplace_back( "BASE", _( "Base moves per attack: " ), "",
                               iteminfo::lower_is_better, attack_time( player_character ) );
        }

        if( base_dps ) {
            info.emplace_back( "BASE", _( "Typical damage per second:" ), "" );
            std::string sep;
            for( const std::pair<const std::string, double> &dps_entry : dps_data ) {
                info.emplace_back( "BASE", sep + dps_entry.first + ": ", "",
                                   iteminfo::no_newline | iteminfo::is_decimal,
                                   dps_entry.second );
                sep = space;
            }
            info.emplace_back( "BASE", "" );
        }

        if( base_stamina ) {
            info.emplace_back( "BASE", _( "<bold>Stamina use</bold>: Costs " ), "", iteminfo::no_newline );
            info.emplace_back( "BASE", ( stam_pct ? _( "about " ) : _( "less than " ) ), "",
                               iteminfo::no_newline | iteminfo::is_decimal | iteminfo::lower_is_better,
                               ( stam_pct > 0.1 ? stam_pct : 0.1 ) );
            // xgettext:no-c-format
            info.emplace_back( "BASE", _( "% stamina to swing." ), "" );
        }

        if( base_dpstam ) {
            info.emplace_back( "BASE", _( "Typical damage per stamina:" ), "" );
            std::string sep;
            for( const std::pair<const std::string, double> &dps_entry : dps_data ) {
                info.emplace_back( "BASE", sep + dps_entry.first + ": ", "",
                                   iteminfo::no_newline | iteminfo::is_decimal,
                                   100 * dps_entry.second / stam );
                sep = space;
            }
            info.emplace_back( "BASE", "" );
        }
    }

    if( parts->test( iteminfo_parts::DESCRIPTION_TECHNIQUES ) ) {
        std::set<matec_id> all_techniques = type->techniques;
        all_techniques.insert( techniques.begin(), techniques.end() );

        if( !all_techniques.empty() ) {
            const std::vector<matec_id> all_tec_sorted = sorted_lex( all_techniques );
            insert_separation_line( info );
            info.emplace_back( "DESCRIPTION", _( "<bold>Techniques when wielded</bold>: " ) +
            enumerate_as_string( all_tec_sorted, []( const matec_id & tid ) {
                return string_format( "<stat>%s</stat>: <info>%s</info> <info>%s</info>", tid.obj().name,
                                      tid.obj().description, _( tid.obj().condition_desc ) );
            } ) );
        }
    }

    // display which martial arts styles character can use with this weapon
    if( parts->test( iteminfo_parts::DESCRIPTION_APPLICABLEMARTIALARTS ) ) {
        const std::string valid_styles = player_character.martial_arts_data->enumerate_known_styles(
                                             typeId() );
        if( !valid_styles.empty() ) {
            insert_separation_line( info );
            info.emplace_back( "DESCRIPTION",
                               _( "You know how to use this with these martial arts "
                                  "styles: " ) + valid_styles );
        }
    }

    if( !is_gunmod() && has_flag( flag_REACH_ATTACK ) &&
        parts->test( iteminfo_parts::DESCRIPTION_GUNMOD_ADDREACHATTACK ) ) {
        insert_separation_line( info );
        if( has_flag( flag_REACH3 ) ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* This item can be used to make <stat>long reach "
                                  "attacks</stat>." ) );
        } else {
            info.emplace_back( "DESCRIPTION",
                               _( "* This item can be used to make <stat>reach "
                                  "attacks</stat>." ) );
        }
    }

    ///\EFFECT_MELEE >2 allows seeing melee damage stats on weapons
    if( ( player_character.get_skill_level( skill_melee ) >= 3 &&
          ( !dmg_types.empty() || get_to_hit() > 0 ) ) || debug_mode ) {
        bodypart_id bp = bodypart_id( "torso" );
        damage_instance non_crit;
        player_character.roll_all_damage( false, non_crit, true, *this, attack_vector_id::NULL_ID(),
                                          sub_bodypart_str_id::NULL_ID(), nullptr, bp );
        damage_instance crit;
        player_character.roll_all_damage( true, crit, true, *this, attack_vector_id::NULL_ID(),
                                          sub_bodypart_str_id::NULL_ID(), nullptr, bp );
        int attack_cost = player_character.attack_speed( *this );
        insert_separation_line( info );
        if( parts->test( iteminfo_parts::DESCRIPTION_MELEEDMG ) ) {
            info.emplace_back( "DESCRIPTION", _( "<bold>Average melee damage</bold>:" ) );
        }
        // Chance of critical hit
        if( parts->test( iteminfo_parts::DESCRIPTION_MELEEDMG_CRIT ) ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "Critical hit chance <neutral>%d%% - %d%%</neutral>" ),
                                              static_cast<int>( player_character.crit_chance( 0, 100, *this ) *
                                                      100 ),
                                              static_cast<int>( player_character.crit_chance( 100, 0, *this ) *
                                                      100 ) ) );
        }
        if( parts->test( iteminfo_parts::DESCRIPTION_MELEEDMG_TYPES ) ) {
            for( const damage_info_order &dio : damage_info_order::get_all(
                     damage_info_order::info_type::MELEE ) ) {
                if( non_crit.type_damage( dio.dmg_type ) > 0 || crit.type_damage( dio.dmg_type ) > 0 ) {
                    // NOTE: Using "BASE" instead of "DESCRIPTION", so numerical formatting will work
                    // (output.cpp:format_item_info does not interpolate <num> for DESCRIPTION info)
                    info.emplace_back( "BASE", string_format( "%s: ",
                                       uppercase_first_letter( dio.dmg_type->name.translated() ) ),
                                       "<num>", iteminfo::no_newline, non_crit.type_damage( dio.dmg_type ) );
                    //~ Label used in the melee damage section in the item info screen (ex: "  Critical bash: ")
                    //~ %1$s = a prepended space, %2$s = the name of the damage type (bash, cut, pierce, etc.)
                    info.emplace_back( "BASE", string_format( _( "%1$sCritical %2$s: " ), space,
                                       dio.dmg_type->name.translated() ),
                                       "<num>", iteminfo::no_flags, crit.type_damage( dio.dmg_type ) );
                }
            }
        }
        // Moves
        if( parts->test( iteminfo_parts::DESCRIPTION_MELEEDMG_MOVES ) ) {
            info.emplace_back( "BASE", _( "Adjusted moves per attack: " ), "<num>",
                               iteminfo::lower_is_better, attack_cost );
        }
        insert_separation_line( info );
    } else if( player_character.get_skill_level( skill_melee ) < 3 &&
               ( !dmg_types.empty() || get_to_hit() > 0 ) ) {
        insert_separation_line( info );
        if( parts->test( iteminfo_parts::DESCRIPTION_MELEEDMG ) ) {
            info.emplace_back( "DESCRIPTION", _( "<bold>Average melee damage</bold>:" ) );
            info.emplace_back( "BASE",
                               _( "You don't know enough about fighting to know how effectively you could use this." ) );
        }
    }
}

std::vector<std::pair<const item *, int>> get_item_duplicate_counts(
        const std::list<const item *> &items )
{
    std::vector<std::pair<item const *, int>> counted_items;
    for( const item *contents_item : items ) {
        bool found = false;
        for( std::pair<const item *, int> &content : counted_items ) {
            if( content.first->display_stacked_with( *contents_item ) ) {
                content.second += 1;
                found = true;
            }
        }
        if( !found ) {
            std::pair<const item *, int> new_content( contents_item, 1 );
            counted_items.push_back( new_content );
        }
    }
    return counted_items;
}


void item::contents_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                          bool /*debug*/ ) const
{
    if( ( toolmods().empty() && gunmods().empty() && contents.empty() ) ||
        !parts->test( iteminfo_parts::DESCRIPTION_CONTENTS ) ) {
        return;
    }
    const std::string space = "  ";

    for( const item *mod : is_gun() ? gunmods() : toolmods() ) {
        // mod_str = "(Integrated) Mod: %mod_name% (%mod_location%) "
        std::string mod_str;
        if( mod->is_irremovable() ) {
            mod_str = _( "Integrated mod: " );
        } else {
            mod_str = _( "Mod: " );
        }
        mod_str += string_format( "<bold>%s</bold>", mod->tname() );
        if( mod->type->gunmod ) {
            mod_str += string_format( " (%s) ", mod->type->gunmod->location.name() );
        }
        insert_separation_line( info );
        info.emplace_back( "DESCRIPTION", mod_str );
        info.emplace_back( "DESCRIPTION", mod->type->description.translated() );
    }

    const std::list<const item *> all_contents = contents.all_items_top();
    const std::vector<std::pair<item const *, int>> counted_contents = get_item_duplicate_counts(
                all_contents );
    bool contents_header = false;
    for( const std::pair<const item *, int> &content_w_count : counted_contents ) {
        const item *contents_item = content_w_count.first;
        int count = content_w_count.second;
        if( !contents_header ) {
            insert_separation_line( info );
            info.emplace_back( "DESCRIPTION", _( "<bold>Contents of this item</bold>:" ) );
            contents_header = true;
        } else {
            // Separate items with a blank line
            info.emplace_back( "DESCRIPTION", space );
        }

        const translation &description = contents_item->type->description;

        if( contents_item->made_of_from_type( phase_id::LIQUID ) ) {
            units::volume contents_volume = contents_item->volume() * batch;
            info.emplace_back( "DESCRIPTION",
                               colorize( contents_item->display_name(), contents_item->color_in_inventory() ) );
            info.emplace_back( vol_to_info( "CONTAINER", description + space, contents_volume ) );
        } else {
            std::string name;
            if( count > 1 ) {
                name += std::to_string( count ) + " ";
            }
            name += colorize( contents_item->display_name( count ), contents_item->color_in_inventory() );
            info.emplace_back( "DESCRIPTION", name );
            info.emplace_back( "DESCRIPTION", description.translated() );
        }
    }
}

void item::properties_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                            bool debug ) const
{
    if( is_null() ) {
        return;
    }

    insert_separation_line( info );

    if( parts->test( iteminfo_parts::BASE_RIGIDITY ) ) {
        bool not_rigid = false;
        if( const islot_armor *t = find_armor_data() ) {
            bool any_encumb_increase = std::any_of( t->data.begin(), t->data.end(),
            []( const armor_portion_data & data ) {
                return data.encumber != data.max_encumber;
            } );
            if( any_encumb_increase ) {
                info.emplace_back( "BASE",
                                   _( "* This item's pockets are <info>not rigid</info>.  Its"
                                      " volume and encumbrance increase with contents." ) );
                not_rigid = true;
            }
        }
        if( !not_rigid && !all_pockets_rigid() && !is_corpse() ) {
            info.emplace_back( "BASE",
                               _( "* This item's pockets are <info>not rigid</info>.  Its"
                                  " volume increases with contents." ) );
        }
    }

    if( parts->test( iteminfo_parts::DESCRIPTION_CONDUCTIVITY ) ) {
        if( !conductive() ) {
            info.emplace_back( "BASE", _( "* This item <good>does not "
                                          "conduct</good> electricity." ) );
        } else if( has_flag( flag_CONDUCTIVE ) ) {
            info.emplace_back( "BASE",
                               _( "* This item effectively <bad>conducts</bad> "
                                  "electricity, as it has no guard." ) );
        } else {
            info.emplace_back( "BASE", _( "* This item <bad>conducts</bad> electricity." ) );
        }
    }

    if( parts->test( iteminfo_parts::DESCRIPTION_FLAGS ) ) {
        // concatenate base and acquired flags...
        cata::flat_set<flag_id> flags;
        flags.insert( get_flags().begin(), get_flags().end() );
        flags.insert( type->get_flags().begin(), type->get_flags().end() );
        flags.insert( inherited_tags_cache.begin(), inherited_tags_cache.end() );

        // ...and display those which have an info description
        for( const flag_id &e : sorted_lex( flags ) ) {
            const json_flag &f = e.obj();
            if( !f.info().empty() ) {
                info.emplace_back( "DESCRIPTION", string_format( "* %s", f.info() ) );
            }
        }
    }

    armor_fit_info( info, parts, batch, debug );

    if( ethereal ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This item disappears as soon as its timer runs out, unless it is permanent or a comestible." ) );
    }

    if( has_flag( flag_RADIO_ACTIVATION ) &&
        parts->test( iteminfo_parts::DESCRIPTION_RADIO_ACTIVATION ) ) {
        if( has_flag( flag_RADIO_MOD ) ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* This item has been modified to listen to <info>radio "
                                  "signals</info>.  It can still be activated manually." ) );
        } else {
            info.emplace_back( "DESCRIPTION",
                               _( "* This item can only be activated by a <info>radio "
                                  "signal</info>." ) );
        }

        std::string signame;
        if( has_flag( flag_RADIOSIGNAL_1 ) ) {
            signame = _( "<color_c_red>red</color> radio signal" );
        } else if( has_flag( flag_RADIOSIGNAL_2 ) ) {
            signame = _( "<color_c_blue>blue</color> radio signal" );
        } else if( has_flag( flag_RADIOSIGNAL_3 ) ) {
            signame = _( "<color_c_green>green</color> radio signal" );
        }
        if( parts->test( iteminfo_parts::DESCRIPTION_RADIO_ACTIVATION_CHANNEL ) ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "* It will be activated by the %s." ),
                                              signame ) );
        }

        if( has_flag( flag_BOMB ) && has_flag( flag_RADIO_INVOKE_PROC ) &&
            parts->test( iteminfo_parts::DESCRIPTION_RADIO_ACTIVATION_PROC ) ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* Activating this item with a <info>radio signal</info> will "
                                  "<neutral>detonate</neutral> it immediately." ) );
        }
    }

    if( parts->test( iteminfo_parts::DESCRIPTION_DIE ) && this->get_var( "die_num_sides", 0 ) != 0 ) {
        info.emplace_back( "DESCRIPTION",
                           string_format( _( "* This item can be used as a <info>die</info>, "
                                             "and has <info>%d</info> sides." ),
                                          static_cast<int>( this->get_var( "die_num_sides",
                                                  0 ) ) ) );
    }
    diag_value const *activity_var_may = maybe_get_value( "activity_var" );
    if( activity_var_may != nullptr && activity_var_may->is_str() ) {
        info.emplace_back( "DESCRIPTION",
                           string_format(
                               _( "* This item is currently used by %s" ),
                               activity_var_may->str() ) );
    }

}

// Cache for can_craft in final_info.
static std::unordered_map<const recipe *, bool> can_craft_recipe_cache;
static time_point cache_valid_turn;

static bool can_craft_recipe( const recipe *r, const inventory &crafting_inv )
{
    if( cache_valid_turn != calendar::turn ) {
        cache_valid_turn = calendar::turn;
        can_craft_recipe_cache.clear();
    }
    if( can_craft_recipe_cache.count( r ) > 0 ) {
        return can_craft_recipe_cache.at( r );
    }
    can_craft_recipe_cache[r] = r->deduped_requirements().can_make_with_inventory( crafting_inv,
                                r->get_component_filter() );
    return can_craft_recipe_cache.at( r );
}

void item::final_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                       bool /* debug */ ) const
{
    if( is_null() ) {
        return;
    }

    avatar &player_character = get_avatar();

    if( has_flag( flag_LEAK_DAM ) && has_flag( flag_RADIOACTIVE ) && damage() > 0
        && parts->test( iteminfo_parts::DESCRIPTION_RADIOACTIVITY_DAMAGED ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* The casing of this item has <neutral>cracked</neutral>, "
                              "revealing an <info>ominous green glow</info>." ) );
    }

    if( has_flag( flag_LEAK_ALWAYS ) && has_flag( flag_RADIOACTIVE ) &&
        parts->test( iteminfo_parts::DESCRIPTION_RADIOACTIVITY_ALWAYS ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This object is <neutral>surrounded</neutral> by a "
                              "<info>sickly green glow</info>." ) );
    }

    if( type->milling_data && !type->milling_data->into_.is_null() ) {
        if( parts->test( iteminfo_parts::DESCRIPTION_MILLEABLE ) ) {

            const islot_milling &mdata = *type->milling_data;
            const recipe rec = *mdata.recipe_;

            if( rec.is_null() ) {
                debugmsg( _( "Failed to find milling recipe for %s." ),
                          this->tname( 1, false ) );
            } else {
                const int product_size = rec.makes_amount();
                const requirement_data::alter_item_comp_vector &components =
                    rec.simple_requirements().get_components();
                int lot_size = 0;

                // Making the assumption that milling only uses a single type of input product. Support for mixed products would require additional logic.
                // We also make the assumption that this is the only relevant input, so if lubricants etc. was to be added more logic would be needed.
                for( const std::vector<item_comp> &component : components ) {
                    for( const item_comp &comp : component ) {
                        if( comp.type == this->typeId() ) {
                            lot_size = comp.count;
                            break;
                        }
                    }

                    if( lot_size > 0 ) {
                        break;
                    }
                }

                if( lot_size == 0 ) {
                    debugmsg( _( "Failed to find milling recipe for %s. It can't be milled." ),
                              this->display_name().c_str() );
                } else {
                    std::string rate_info = string_format(
                                                _( "* <neutral>%d %s</neutral> can be turned into <neutral>%d %s</neutral>." ),
                                                lot_size,
                                                type->nname( lot_size ), product_size,
                                                mdata.into_->nname( product_size ) );

                    info.emplace_back( "DESCRIPTION",
                                       string_format( _( "* This item can be <info>milled</info>." ) ) );
                    info.emplace_back( "DESCRIPTION", rate_info );
                }
            }
        }
    }

    if( is_brewable() ) {
        const item &brewed = *this;
        if( parts->test( iteminfo_parts::DESCRIPTION_BREWABLE_DURATION ) ) {
            const time_duration btime = brewed.brewing_time();
            int btime_i = to_days<int>( btime );
            if( btime <= 2_days ) {
                btime_i = to_hours<int>( btime );
                info.emplace_back( "DESCRIPTION",
                                   string_format( n_gettext( "* Once set in a vat, this "
                                                  "will ferment in around %d hour.",
                                                  "* Once set in a vat, this will ferment in "
                                                  "around %d hours.", btime_i ), btime_i ) );
            } else {
                info.emplace_back( "DESCRIPTION",
                                   string_format( n_gettext( "* Once set in a vat, this "
                                                  "will ferment in around %d day.",
                                                  "* Once set in a vat, this will ferment in "
                                                  "around %d days.", btime_i ), btime_i ) );
            }
        }
        if( parts->test( iteminfo_parts::DESCRIPTION_BREWABLE_PRODUCTS ) ) {
            for( const std::pair<const itype_id, int> &res : brewed.brewing_results() ) {
                info.emplace_back( "DESCRIPTION",
                                   string_format( _( "* Fermenting this will produce "
                                                     "<neutral>%s</neutral>." ),
                                                  res.first->nname( res.second ) ) );
            }
        }
    }

    if( is_compostable() ) {
        const item &composted = *this;
        if( parts->test( iteminfo_parts::DESCRIPTION_COMPOSTABLE_DURATION ) ) {
            const time_duration btime = composted.composting_time();
            int btime_i = to_days<int>( btime );
            if( btime <= 2_days ) {
                btime_i = to_hours<int>( btime );
                info.emplace_back( "DESCRIPTION",
                                   string_format( n_gettext( "* Once set in an anaerobic digester, this "
                                                  "will ferment in around %d hour.",
                                                  "* Once set in an anaerobic digester, this will ferment in "
                                                  "around %d hours.", btime_i ), btime_i ) );
            } else {
                info.emplace_back( "DESCRIPTION",
                                   string_format( n_gettext( "* Once set in an anaerobic digester, this "
                                                  "will ferment in around %d day.",
                                                  "* Once set in an anaerobic digester, this will ferment in "
                                                  "around %d days.", btime_i ), btime_i ) );
            }
        }
        if( parts->test( iteminfo_parts::DESCRIPTION_COMPOSTABLE_PRODUCTS ) ) {
            for( const std::pair<const itype_id, int> &res : composted.composting_results() ) {
                info.emplace_back( "DESCRIPTION",
                                   string_format( _( "* Fermenting this will produce "
                                                     "<neutral>%s</neutral>." ),
                                                  res.first->nname( res.second ) ) );
            }
        }
    }

    if( parts->test( iteminfo_parts::DESCRIPTION_FAULTS ) ) {
        for( const fault_id &e : faults ) {
            //~ %1$s is the name of a fault and %2$s is the description of the fault
            info.emplace_back( "DESCRIPTION", string_format( _( "* <bad>%1$s</bad>.  %2$s" ),
                               e.obj().name(), get_fault_description( e ) ) );
        }
    }

    // Does the item fit into any armor?
    std::vector<std::reference_wrapper<const item>> armor_containers;
    auto /*std::vector<item>::const_iterator*/ [begin, end] =
        item_controller->get_armor_containers( volume() );
    for( std::vector<item>::const_iterator iter = begin; iter != end; ++iter ) {
        if( iter->can_contain_directly( *this ).success() ) {
            armor_containers.emplace_back( *iter );
        }
    }

    if( !armor_containers.empty() && parts->test( iteminfo_parts::DESCRIPTION_ARMOR_CONTAINERS ) ) {
        insert_separation_line( info );
        const bool too_many_items = armor_containers.size() > 100;
        std::vector<std::string> armor_containers_str;
        for( const item &it : armor_containers ) {
            // This sorts the green worn items in front of others, as '<' sorts before 'a', 'B' etc.
            if( player_character.is_wearing( it.type->get_id() ) ) {
                armor_containers_str.emplace_back( string_format( "<good>%s</good>", it.type->nname( 1 ) ) );
            } else if( !too_many_items ) {
                armor_containers_str.emplace_back( it.type->nname( 1 ) );
            }
        }
        const int unworn_items = armor_containers.size() - armor_containers_str.size();
        if( too_many_items && armor_containers_str.empty() ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "<bold>Can be stored in %d unworn wearables.</bold>" ), unworn_items ) );
        } else if( too_many_items && unworn_items > 0 ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "<bold>Can be stored in (worn)</bold>: %s.  And %d unworn wearables." ),
                                              enumerate_lcsorted_with_limit( armor_containers_str, 30 ), unworn_items ) );
        } else {
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "<bold>Can be stored in (wearables)</bold>: %s." ),
                                              enumerate_lcsorted_with_limit( armor_containers_str, 30 ) ) );
        }
    }

    if( parts->test( iteminfo_parts::DESCRIPTION_ACTIVATABLE_TRANSFORMATION ) ) {
        insert_separation_line( info );
        for( const auto &u : type->use_methods ) {
            const delayed_transform_iuse *tt = dynamic_cast<const delayed_transform_iuse *>
                                               ( u.second.get_actor_ptr() );
            if( tt == nullptr ) {
                continue;
            }
            const int time_to_do = tt->time_to_do( *this );
            if( time_to_do <= 0 ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "It's done and <info>can be activated</info>." ) );
            } else {
                const std::string time = to_string_clipped( time_duration::from_turns( time_to_do ) );
                info.emplace_back( "DESCRIPTION",
                                   string_format( _( "It will be done in %s." ),
                                                  time.c_str() ) );
            }
        }
    }

    auto const item_note = item_vars.find( "item_note" );

    if( item_note != item_vars.end() && parts->test( iteminfo_parts::DESCRIPTION_NOTES ) ) {
        insert_separation_line( info );
        std::string ntext;
        auto const item_note_tool = item_vars.find( "item_note_tool" );
        const use_function *use_func =
            item_note_tool != item_vars.end() ?
            item_controller->find_template(
                itype_id( item_note_tool->second.str() ) )->get_use( "inscribe" ) :
            nullptr;
        const inscribe_actor *use_actor =
            use_func ? dynamic_cast<const inscribe_actor *>( use_func->get_actor_ptr() ) : nullptr;
        if( use_actor ) {
            //~ %1$s: gerund (e.g. carved), %2$s: item name, %3$s: inscription text
            ntext = string_format( pgettext( "carving", "%1$s on the %2$s is: %3$s" ),
                                   use_actor->gerund, tname(), item_note->second.str() );
        } else {
            //~ %1$s: inscription text
            ntext = string_format( pgettext( "carving", "Note: %1$s" ), item_note->second.str() );
        }
        info.emplace_back( "DESCRIPTION", ntext );
    }

    // Price and barter value
    const int price_preapoc = price( false ) * batch;
    const int price_postapoc = price( true ) * batch;
    if( parts->test( iteminfo_parts::BASE_PRICE ) ) {
        insert_separation_line( info );
        info.emplace_back( "BASE", _( "Price: " ), _( "$<num>" ),
                           iteminfo::is_decimal | iteminfo::no_newline,
                           static_cast<double>( price_preapoc ) / 100 );
    }
    if( parts->test( iteminfo_parts::BASE_BARTER ) ) {
        const std::string space = "  ";
        info.emplace_back( "BASE", space + _( "Barter value: " ), _( "$<num>" ),
                           iteminfo::is_decimal,
                           static_cast<double>( price_postapoc ) / 100 );
    }

    // Recipes using this item as an ingredient
    if( parts->test( iteminfo_parts::DESCRIPTION_APPLICABLE_RECIPES ) ) {
        // with the inventory display allowing you to select items, showing the things you could make with contained items could be confusing.
        const itype_id &tid = typeId();
        const inventory &crafting_inv = player_character.crafting_inventory();
        const recipe_subset &available_recipe_subset = player_character.get_group_available_recipes();
        const std::set<const recipe *> &item_recipes = available_recipe_subset.of_component( tid );

        insert_separation_line( info );
        if( item_recipes.empty() ) {
            info.emplace_back( "DESCRIPTION", _( "You know of nothing you could craft with it." ) );
        } else {
            std::vector<std::string> crafts;
            crafts.reserve( item_recipes.size() );
            for( const recipe *r : item_recipes ) {
                const bool can_make = can_craft_recipe( r, crafting_inv );
                // Endarken recipes that can't be crafted with the survivor's inventory
                const std::string name = r->result_name( /* decorated = */ true );
                crafts.emplace_back( can_make ? name : string_format( "<dark>%s</dark>", name ) );
            }
            const std::string recipes = enumerate_lcsorted_with_limit( crafts, 15 );
            info.emplace_back( " DESCRIPTION", string_format( _( "You could use it to craft: %s" ), recipes ) );
        }
    }

    if( is_armor() ) {
        const ret_val<void> can_wear = player_character.can_wear( *this, true );
        if( !can_wear.success() ) {
            insert_separation_line( info );
            // can_wear returns a translated string
            info.emplace_back( "DESCRIPTION", string_format( "<bad>%s</bad>", can_wear.str() ) );
        }
    }

    // Vehicle parts or appliances using this item as a component
    if( parts->test( iteminfo_parts::DESCRIPTION_VEHICLE_PARTS ) ) {
        const itype_id tid = typeId();
        std::vector<vpart_info> vparts;

        for( const vpart_info &vpi : vehicles::parts::get_all() ) {
            if( vpi.base_item == tid ) {
                vparts.push_back( vpi );
            }
        }

        const auto print_parts = [&info, &player_character](
                                     const std::vector<vpart_info> &vparts,
                                     std::string_view install_where,
                                     const std::function<bool( const vpart_info & )> &predicate
        ) {
            std::vector<std::string> result_parts;
            for( const vpart_info &vp : vparts ) {
                const bool is_duplicate = std::any_of( result_parts.begin(), result_parts.end(),
                [name = vp.name()]( const std::string & r ) {
                    return r == name;
                } );

                if( is_duplicate || !predicate( vp ) ) {
                    continue; // skip part variants, they have same part names
                }

                const bool can_install = player_character.meets_skill_requirements( vp.install_skills );
                const std::string name = vp.name();

                // Endarken parts that can't be installed with the character's skills
                result_parts.emplace_back( can_install ? name : string_format( "<dark>%s</dark>", name ) );
            }

            if( result_parts.empty() ) {
                return;
            }

            const std::string installable_parts = enumerate_lcsorted_with_limit( result_parts, 15 );

            insert_separation_line( info );
            info.emplace_back( " DESCRIPTION", string_format( install_where, installable_parts ) );
        };

        print_parts( vparts, _( "You could install it in a vehicle: %s" ),
        []( const vpart_info & vp ) {
            return !vp.has_flag( vpart_bitflags::VPFLAG_APPLIANCE ) &&
                   !vp.has_flag( "NO_INSTALL_HIDDEN" ) &&
                   !vp.has_flag( "NO_INSTALL_PLAYER" );
        } );

        print_parts( vparts, _( "You could install it as an appliance: %s" ),
        []( const vpart_info & vp ) {
            return vp.has_flag( vpart_bitflags::VPFLAG_APPLIANCE );
        } );
    }
}

void item::ascii_art_info( std::vector<iteminfo> &info, const iteminfo_query * /* parts */,
                           int  /* batch */,
                           bool /* debug */ ) const
{
    if( is_null() ) {
        return;
    }

    if( get_option<bool>( "ENABLE_ASCII_ART" ) ) {
        ascii_art_id art = type->picture_id;
        if( has_itype_variant() && itype_variant().art.is_valid() ) {
            art = itype_variant().art;
        }
        if( art.is_valid() ) {
            insert_separation_line( info );
            for( const std::string &line : art->picture ) {
                info.emplace_back( "DESCRIPTION", line, iteminfo::is_art );
            }
        }
    }
}

std::vector<iteminfo> item::get_info( bool showtext ) const
{
    return get_info( showtext, 1 );
}

std::vector<iteminfo> item::get_info( bool showtext, int batch ) const
{
    return get_info( showtext ? &iteminfo_query::all : &iteminfo_query::notext, batch );
}

std::vector<iteminfo> item::get_info( const iteminfo_query *parts, int batch ) const
{
    std::vector<iteminfo> info = {};
    const bool debug = g != nullptr && debug_mode;

    if( parts == nullptr ) {
        parts = &iteminfo_query::all;
    }

    if( !is_null() ) {
        basic_info( info, parts, batch, debug );
        debug_info( info, parts, batch, debug );
    }

    // Different items might want to present the info blocks in different order, depending on what
    // they consider more relevant.
    std::vector<std::string> all_info_blocks = {"big_block_of_stuff", "contents", "melee_combat_info", "footer"};
    std::vector<std::string> prio_info_blocks = { };
    item_category_id my_cat_id = get_category_shallow().id;
    if( is_maybe_melee_weapon() ) {
        prio_info_blocks.emplace_back( "melee_combat_info" );
    } else if( my_cat_id == item_category_container ) {
        prio_info_blocks.emplace_back( "contents" );
    }
    std::vector<std::string> info_block_ordered = prio_info_blocks;
    for( const std::string &blockname : all_info_blocks ) {
        if( std::find( info_block_ordered.begin(), info_block_ordered.end(),
                       blockname ) == info_block_ordered.end() ) {
            info_block_ordered.push_back( blockname );
        }
    }

    for( const auto &blockname : info_block_ordered ) {
        if( blockname == "melee_combat_info" ) {

            melee_combat_info( info, parts, batch, debug );

        } else if( blockname == "big_block_of_stuff" ) {

            if( is_medication() ) {
                med_info( this, info, parts, batch, debug );
            }

            if( is_food() ) {
                food_info( this, info, parts, batch, debug );
            }

            magazine_info( info, parts, batch, debug );
            ammo_info( info, parts, batch, debug );

            const item *gun = nullptr;
            if( is_gun() ) {
                gun = this;
                const gun_mode aux = gun_current_mode();
                // if we have an active auxiliary gunmod display stats for this instead
                if( aux && aux->is_gunmod() && aux->is_gun() &&
                    parts->test( iteminfo_parts::DESCRIPTION_AUX_GUNMOD_HEADER ) ) {
                    gun = &*aux;
                    info.emplace_back( "DESCRIPTION",
                                       string_format( _( "Stats of the active <info>gunmod (%s)</info> "
                                                         "are shown." ), gun->tname() ) );
                }
            }
            if( gun != nullptr ) {
                gun_info( gun, info, parts, batch, debug );
            }

            gunmod_info( info, parts, batch, debug );
            armor_info( info, parts, batch, debug );
            animal_armor_info( info, parts, batch, debug );
            book_info( info, parts, batch, debug );
            battery_info( info, parts, batch, debug );
            tool_info( info, parts, batch, debug );
            actions_info( info, parts, batch, debug );
            component_info( info, parts, batch, debug );
            qualities_info( info, parts, batch, debug );

            // Uses for item (bandaging quality, holster capacity, grenade activation)
            if( parts->test( iteminfo_parts::DESCRIPTION_USE_METHODS ) ) {
                for( const std::pair<const std::string, use_function> &method : type->use_methods ) {
                    insert_separation_line( info );
                    method.second.dump_info( *this, info );
                }
            }

            repair_info( info, parts, batch, debug );
            enchantment_info( info, parts, batch, debug );
            disassembly_info( info, parts, batch, debug );
            properties_info( info, parts, batch, debug );
            bionic_info( info, parts, batch, debug );

        } else if( blockname == "contents" ) {

            contents.info( info, parts );
            contents_info( info, parts, batch, debug );

        } else if( blockname == "footer" ) {

            final_info( info, parts, batch, debug );
            ascii_art_info( info, parts, batch, debug );

        } else {

            debugmsg( "Trying to show info block named %s which is not valid.", blockname );

        }
    }

    if( !info.empty() && info.back().sName == "--" ) {
        info.pop_back();
    }
    return info;
}

std::string item::info( bool showtext ) const
{
    std::vector<iteminfo> dummy;
    return info( showtext, dummy );
}

std::string item::info( bool showtext, std::vector<iteminfo> &iteminfo ) const
{
    return info( showtext, iteminfo, 1 );
}

std::string item::info( bool showtext, std::vector<iteminfo> &iteminfo, int batch ) const
{
    return info( iteminfo, showtext ? &iteminfo_query::all : &iteminfo_query::notext, batch );
}

std::string item::info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch ) const
{
    info = get_info( parts, batch );
    return format_item_info( info, {} );
}

iteminfo::iteminfo( const std::string &Type, const std::string &Name, const std::string &Fmt,
                    flags Flags, double Value, double UnitVal )
{
    sType = Type;
    sName = replace_colors( Name );
    sFmt = replace_colors( Fmt );
    is_int = !( Flags & is_decimal || Flags & is_three_decimal );
    three_decimal = ( Flags & is_three_decimal );
    dValue = Value;
    dUnitAdjustedVal = UnitVal < std::numeric_limits<float>::epsilon() &&
                       UnitVal > -std::numeric_limits<float>::epsilon() ? Value : UnitVal;
    bShowPlus = static_cast<bool>( Flags & show_plus );
    std::stringstream convert;
    if( bShowPlus ) {
        convert << std::showpos;
    }
    if( is_int ) {
        convert << std::setprecision( 0 );
    } else if( three_decimal ) {
        convert << std::setprecision( 3 );
    } else {
        convert << std::setprecision( 2 );
    }
    convert << std::fixed << Value;
    sValue = convert.str();
    bNewLine = !( Flags & no_newline );
    bLowerIsBetter = static_cast<bool>( Flags & lower_is_better );
    bDrawName = !( Flags & no_name );
    bIsArt = Flags & is_art;
    this->isTable = Flags & is_table;
}

iteminfo::iteminfo( const std::string &Type, const std::string &Name, flags Flags )
    : iteminfo( Type, Name, "", Flags )
{
}

iteminfo::iteminfo( const std::string &Type, const std::string &Name, double Value,
                    double UnitVal )
    : iteminfo( Type, Name, "", no_flags, Value, UnitVal )
{
}

iteminfo vol_to_info( const std::string &type, const std::string &left,
                      const units::volume &vol, int decimal_places, bool lower_is_better )
{
    iteminfo::flags f = iteminfo::no_newline;
    if( lower_is_better ) {
        f |= iteminfo::lower_is_better;
    }
    int converted_volume_scale = 0;
    const double converted_volume =
        round_up( convert_volume( vol.value(), &converted_volume_scale ), decimal_places );
    if( converted_volume_scale != 0 ) {
        f |= iteminfo::is_decimal;
    }
    return iteminfo( type, left, string_format( "<num> %s", volume_units_abbr() ), f,
                     converted_volume );
}

iteminfo weight_to_info( const std::string &type, const std::string &left,
                         const units::mass &weight, int /* decimal_places */, bool lower_is_better )
{
    iteminfo::flags f = iteminfo::no_newline;
    if( lower_is_better ) {
        f |= iteminfo::lower_is_better;
    }
    const double converted_weight = convert_weight( weight );
    f |= iteminfo::is_decimal;
    return iteminfo( type, left, string_format( "<num> %s", weight_units() ), f,
                     converted_weight );
}
