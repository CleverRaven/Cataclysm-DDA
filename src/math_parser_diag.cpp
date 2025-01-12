#include "math_parser_diag.h"

#include <functional>
#include <string>
#include <vector>

#include "calendar.h"
#include "condition.h"
#include "dialogue.h"
#include "enums.h"
#include "field.h"
#include "game.h"
#include "magic.h"
#include "map.h"
#include "math_parser_diag_value.h"
#include "mod_manager.h"
#include "mongroup.h"
#include "mtype.h"
#include "npc.h"
#include "options.h"
#include "string_input_popup.h"
#include "units.h"
#include "weather.h"
#include "worldfactory.h"

/*
General guidelines for writing dialogue functions

The typical parsing function takes the form:

math_eval_dbl_f myfunction_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{
    diag_value myval = kwargs.kwarg_or( "mykwarg", "default-value" );

    ...parse-time code...

    return[effect_id = params[0], myval, beta = is_beta( scope )]( const_dialogue const & d ) {
        ...run-time code...
    };
}

- Don't validate the number of arguments (params). The math parser already does that
- Only use variadic functions if all arguments are treated the same way,
  regardless of how many there are (including zero)
- Use kwargs for optional arguments
- Prefer splitting functions instead of using mandatory kwargs
  ex: school_level() split from spell_level() instead of spell_level('school':blorg)
- Use parameter-less functions diag_value::str(), dbl(), and var() only at parse-time
- Use conversion functions diag_value::str( d ) and dbl( d ) only at run-time
- throw math::syntax_error for parse-time errors
- throw math::runtime_error for run-time errors
*/

static const json_character_flag json_flag_MUTATION_THRESHOLD( "MUTATION_THRESHOLD" );

namespace
{
bool is_beta( char scope )
{
    switch( scope ) {
        case 'n':
            return true;
        case 'u':
        default:
            return false;
    }
}

constexpr bool is_true( double dbl )
{
    return dbl >= 1 || float_equals( dbl, 1 );
}

template<typename T>
constexpr std::string_view _str_type_of()
{
    if constexpr( std::is_same_v<T, units::energy> ) {
        return "energy";
    } else if constexpr( std::is_same_v<T, time_duration> ) {
        return "time";
    }
    return "cookies";
}

template<typename T>
T _read_from_string( std::string_view s, const std::vector<std::pair<std::string, T>> &units )
{
    auto const error = [s]( char const * suffix, size_t /* offset */ ) {
        throw math::runtime_error( R"(Failed to convert "%s" to a %s value: %s)", s, _str_type_of<T>(),
                                   suffix );
    };
    return detail::read_from_json_string_common<T>( s, units, error );
}

diag_eval_dbl_f u_val( char scope, std::vector<diag_value> const &params,
                       diag_kwargs const & /* kwargs */ )
{
    return conditional_t::get_get_dbl( params[0].str(), scope );
}

diag_assign_dbl_f u_val_ass( char scope, std::vector<diag_value> const &params,
                             diag_kwargs const & /* kwargs */ )
{
    return conditional_t::get_set_dbl( params[0].str(), scope );
}

diag_eval_dbl_f option_eval( char /* scope */, std::vector<diag_value> const &params,
                             diag_kwargs const & /* kwargs */ )
{
    return[option = params[0]]( const_dialogue const & d ) {
        return get_option<float>( option.str( d ), true );
    };
}

diag_eval_dbl_f addiction_intensity_eval( char scope, std::vector<diag_value> const &params,
        diag_kwargs const & /* kwargs */ )
{
    return[ beta = is_beta( scope ), add_value = params[0]]( const_dialogue const & d ) {
        return d.const_actor( beta )->get_addiction_intensity( addiction_id( add_value.str( d ) ) );
    };
}

diag_eval_dbl_f addiction_turns_eval( char scope, std::vector<diag_value> const &params,
                                      diag_kwargs const & /* kwargs */ )
{
    return[ beta = is_beta( scope ), add_value = params[0]]( const_dialogue const & d ) {
        return d.const_actor( beta )->get_addiction_turns( addiction_id( add_value.str( d ) ) );
    };
}

diag_assign_dbl_f addiction_turns_ass( char scope, std::vector<diag_value> const &params,
                                       diag_kwargs const & /* kwargs */ )
{
    return[ beta = is_beta( scope ), add_value = params[0]]( dialogue const & d, double val ) {
        return d.actor( beta )->set_addiction_turns( addiction_id( add_value.str( d ) ), val );
    };
}

diag_eval_dbl_f health_eval( char scope, std::vector<diag_value> const & /* params */,
                             diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope )]( const_dialogue const & d ) {
        return d.const_actor( beta )->get_health();
    };
}

diag_assign_dbl_f health_ass( char scope, std::vector<diag_value> const & /* params */,
                              diag_kwargs const & /* kwargs */ )
{
    return [beta = is_beta( scope )]( dialogue const & d, double val ) {
        const int current_health = d.actor( beta )->get_health();
        return d.actor( beta )->mod_livestyle( val - current_health );
    };
}

diag_eval_dbl_f armor_eval( char scope, std::vector<diag_value> const &params,
                            diag_kwargs const & /* kwargs */ )
{
    return[type = params[0], bpid = params[1], beta = is_beta( scope )]( const_dialogue const & d ) {
        damage_type_id dt( type.str( d ) );
        bodypart_id bp( bpid.str( d ) );
        return d.const_actor( beta )->armor_at( dt, bp );
    };
}

diag_eval_dbl_f charge_count_eval( char scope, std::vector<diag_value> const &params,
                                   diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope ), item_value = params[0]]( const_dialogue const & d ) {
        return d.const_actor( beta )->charges_of( itype_id( item_value.str( d ) ) );
    };
}

diag_eval_dbl_f coverage_eval( char scope, std::vector<diag_value> const &params,
                               diag_kwargs const & /* kwargs */ )
{
    return[bpid = params[0], beta = is_beta( scope )]( const_dialogue const & d ) {
        bodypart_id bp( bpid.str( d ) );
        return d.const_actor( beta )->coverage_at( bp );
    };
}

diag_eval_dbl_f distance_eval( char scope, std::vector<diag_value> const &params,
                               diag_kwargs const & /* kwargs */ )
{
    return[params, beta = is_beta( scope )]( const_dialogue const & d ) {
        const auto get_pos = [&d]( std::string_view str ) {
            if( str == "u" ) {
                return d.const_actor( false )->global_pos();
            } else if( str == "npc" ) {
                return d.const_actor( true )->global_pos();
            }
            return tripoint_abs_ms( tripoint::from_string( str.data() ) );
        };
        return rl_dist( get_pos( params[0].str( d ) ), get_pos( params[1].str( d ) ) );
    };
}

diag_eval_dbl_f damage_level_eval( char scope, std::vector<diag_value> const &params,
                                   diag_kwargs const & /* kwargs */ )
{
    return[params, beta = is_beta( scope )]( const_dialogue const & d ) {
        item_location const *it = d.const_actor( beta )->get_const_item();
        if( !it ) {
            throw math::runtime_error( "subject of damage_level() must be an item" );
        }
        return ( *it )->damage_level();
    };
}

diag_eval_dbl_f effect_intensity_eval( char scope, std::vector<diag_value> const &params,
                                       diag_kwargs const &kwargs )
{
    diag_value bp_val = kwargs.kwarg_or( "bodypart" );
    return[effect_id = params[0], bp_val, beta = is_beta( scope )]( const_dialogue const & d ) {
        std::string const bp_str = bp_val.str( d );
        bodypart_id const bp = bp_str.empty() ? bodypart_str_id::NULL_ID() : bodypart_id( bp_str );
        effect target = d.const_actor( beta )->get_effect( efftype_id( effect_id.str( d ) ), bp );
        return target.is_null() ? -1 : target.get_intensity();
    };
}

diag_eval_dbl_f encumbrance_eval( char scope, std::vector<diag_value> const &params,
                                  diag_kwargs const & /* kwargs */ )
{
    return[bpid = params[0], beta = is_beta( scope )]( const_dialogue const & d ) {
        bodypart_id bp( bpid.str( d ) );
        return d.const_actor( beta )->encumbrance_at( bp );
    };
}

diag_eval_dbl_f faction_like_eval( char /* scope */, std::vector<diag_value> const &params,
                                   diag_kwargs const & /* kwargs */ )
{
    return [fac_val = params[0]]( const_dialogue const & d ) {
        faction *fac = g->faction_manager_ptr->get( faction_id( fac_val.str( d ) ) );
        return fac->likes_u;
    };
}

diag_assign_dbl_f faction_like_ass( char /* scope */, std::vector<diag_value> const &params,
                                    diag_kwargs const & /* kwargs */ )
{
    return [fac_val = params[0]]( dialogue const & d, double val ) {
        faction *fac = g->faction_manager_ptr->get( faction_id( fac_val.str( d ) ) );
        fac->likes_u = val;
    };
}

diag_eval_dbl_f faction_respect_eval( char /* scope */, std::vector<diag_value> const &params,
                                      diag_kwargs const & /* kwargs */ )
{
    return [fac_val = params[0]]( const_dialogue const & d ) {
        faction *fac = g->faction_manager_ptr->get( faction_id( fac_val.str( d ) ) );
        return fac->respects_u;
    };
}

diag_assign_dbl_f faction_respect_ass( char /* scope */, std::vector<diag_value> const &params,
                                       diag_kwargs const & /* kwargs */ )
{
    return [fac_val = params[0]]( dialogue const & d, double val ) {
        faction *fac = g->faction_manager_ptr->get( faction_id( fac_val.str( d ) ) );
        fac->respects_u = val;
    };
}

diag_eval_dbl_f faction_trust_eval( char /* scope */, std::vector<diag_value> const &params,
                                    diag_kwargs const & /* kwargs */ )
{
    return [fac_val = params[0]]( const_dialogue const & d ) {
        faction *fac = g->faction_manager_ptr->get( faction_id( fac_val.str( d ) ) );
        return fac->trusts_u;
    };
}

diag_assign_dbl_f faction_trust_ass( char /* scope */, std::vector<diag_value> const &params,
                                     diag_kwargs const & /* kwargs */ )
{
    return [fac_val = params[0]]( dialogue const & d, double val ) {
        faction *fac = g->faction_manager_ptr->get( faction_id( fac_val.str( d ) ) );
        fac->trusts_u = val;
    };
}

diag_eval_dbl_f faction_food_supply_eval( char /* scope */,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{
    diag_value vit_val = kwargs.kwarg_or( "vitamin" );
    return [fac_val = params[0], vit_val]( const_dialogue const & d ) {
        faction *fac = g->faction_manager_ptr->get( faction_id( fac_val.str( d ) ) );
        if( !vit_val.is_empty() ) {
            return static_cast<double>( fac->food_supply.get_vitamin( vitamin_id( vit_val.str( d ) ) ) );
        }
        return static_cast<double>( fac->food_supply.calories );
    };
}

diag_assign_dbl_f faction_food_supply_ass( char /* scope */,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{
    diag_value vit_val = kwargs.kwarg_or( "vitamin" );
    return [fac_val = params[0], vit_val]( dialogue const & d, double val ) {
        faction *fac = g->faction_manager_ptr->get( faction_id( fac_val.str( d ) ) );
        if( !vit_val.is_empty() ) {
            fac->food_supply.add_vitamin( vitamin_id( vit_val.str( d ) ), val );
            return;
        }
        fac->food_supply.calories = val;
    };
}

diag_eval_dbl_f faction_wealth_eval( char /* scope */, std::vector<diag_value> const &params,
                                     diag_kwargs const & /* kwargs */ )
{
    return [fac_val = params[0]]( const_dialogue const & d ) {
        faction *fac = g->faction_manager_ptr->get( faction_id( fac_val.str( d ) ) );
        return fac->wealth;
    };
}

diag_assign_dbl_f faction_wealth_ass( char /* scope */, std::vector<diag_value> const &params,
                                      diag_kwargs const & /* kwargs */ )
{
    return [fac_val = params[0]]( dialogue const & d, double val ) {
        faction *fac = g->faction_manager_ptr->get( faction_id( fac_val.str( d ) ) );
        fac->wealth = val;
    };
}

diag_eval_dbl_f faction_power_eval( char /* scope */, std::vector<diag_value> const &params,
                                    diag_kwargs const & /* kwargs */ )
{
    return [fac_val = params[0]]( const_dialogue const & d ) {
        faction *fac = g->faction_manager_ptr->get( faction_id( fac_val.str( d ) ) );
        return fac->power;
    };
}

diag_assign_dbl_f faction_power_ass( char /* scope */, std::vector<diag_value> const &params,
                                     diag_kwargs const & /* kwargs */ )
{
    return [fac_val = params[0]]( dialogue const & d, double val ) {
        faction *fac = g->faction_manager_ptr->get( faction_id( fac_val.str( d ) ) );
        fac->power = val;
    };
}

diag_eval_dbl_f faction_size_eval( char /* scope */, std::vector<diag_value> const &params,
                                   diag_kwargs const & /* kwargs */ )
{
    return [fac_val = params[0]]( const_dialogue const & d ) {
        faction *fac = g->faction_manager_ptr->get( faction_id( fac_val.str( d ) ) );
        return fac->size;
    };
}

diag_assign_dbl_f faction_size_ass( char /* scope */, std::vector<diag_value> const &params,
                                    diag_kwargs const & /* kwargs */ )
{
    return [fac_val = params[0]]( dialogue const & d, double val ) {
        faction *fac = g->faction_manager_ptr->get( faction_id( fac_val.str( d ) ) );
        fac->size = val;
    };
}

diag_eval_dbl_f field_strength_eval( char scope, std::vector<diag_value> const &params,
                                     diag_kwargs const &kwargs )
{
    std::optional<var_info> loc_var;
    diag_value loc_val = kwargs.kwarg_or( "location" );

    if( !loc_val.is_empty() ) {
        loc_var = loc_val.var();
    } else if( scope == 'g' ) {
        throw math::syntax_error(
            R"("field_strength" needs either an actor scope (u/n) or a 'location' kwarg)" );
    }

    return [beta = is_beta( scope ), field_value = params[0], loc_var]( const_dialogue const & d ) {
        map &here = get_map();
        tripoint_abs_ms loc;
        if( loc_var.has_value() ) {
            loc = get_tripoint_from_var( loc_var, d, beta );
        } else {
            loc = d.const_actor( beta )->global_pos();
        }
        field_type_id ft = field_type_id( field_value.str( d ) );
        field_entry *fp = here.field_at( here.bub_from_abs( loc ) ).find_field( ft );
        return fp ? fp->get_field_intensity() :  0;
    };
}

diag_eval_dbl_f gun_damage_eval( char scope, std::vector<diag_value> const &params,
                                 diag_kwargs const & /* kwargs */ )
{

    return[dt_val = params[0], beta = is_beta( scope )]( const_dialogue const & d )-> double {
        item_location const *it = d.const_actor( beta )->get_const_item();
        if( it == nullptr )
        {
            throw math::runtime_error( "subject of gun_damage() must be an item" );
        }
        std::string const dt_str = dt_val.str( d );
        if( dt_str == "ALL" )
        {
            return ( *it )->gun_damage( true ).total_damage();
        }
        return ( *it )->gun_damage( true ).type_damage( damage_type_id( dt_str ) );
    };
}

diag_eval_dbl_f has_trait_eval( char scope, std::vector<diag_value> const &params,
                                diag_kwargs const & /* kwargs */ )
{
    return [beta = is_beta( scope ), tid = params[0] ]( const_dialogue const & d ) {
        return d.const_actor( beta )->has_trait( trait_id( tid.str( d ) ) );
    };
}

diag_eval_dbl_f sum_traits_of_category_eval( char scope, std::vector<diag_value> const &params,
        diag_kwargs const &kwargs )
{

    diag_value type = kwargs.kwarg_or( "type", "ALL" );

    return [beta = is_beta( scope ), category = params[0], type]( const_dialogue const & d ) {

        mutation_category_id cat = mutation_category_id( category.str() );
        std::string thing = type.str( d );
        mut_count_type count_type;

        if( thing == "POSITIVE" ) {
            count_type = mut_count_type::POSITIVE;
        } else if( thing == "NEGATIVE" ) {
            count_type = mut_count_type::NEGATIVE;
        } else if( thing == "ALL" ) {
            count_type = mut_count_type::ALL;
        } else {
            throw math::runtime_error( "Incorrect type '%s' in sum_traits_of_category", type.str() );
        }

        return d.const_actor( beta )->get_total_in_category( cat, count_type );
    };
}

diag_eval_dbl_f sum_traits_of_category_char_has_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{

    diag_value type = kwargs.kwarg_or( "type", "ALL" );

    return [beta = is_beta( scope ), category = params[0], type]( const_dialogue const & d ) {

        mutation_category_id cat = mutation_category_id( category.str() );
        std::string thing = type.str( d );
        mut_count_type count_type;

        if( thing == "POSITIVE" ) {
            count_type = mut_count_type::POSITIVE;
        } else if( thing == "NEGATIVE" ) {
            count_type = mut_count_type::NEGATIVE;
        } else if( thing == "ALL" ) {
            count_type = mut_count_type::ALL;
        } else {
            throw math::runtime_error( "Incorrect type '%s' in sum_traits_of_category", type.str() );
        }

        return d.const_actor( beta )->get_total_in_category_char_has( cat, count_type );
    };
}

diag_eval_dbl_f has_flag_eval( char scope, std::vector<diag_value> const &params,
                               diag_kwargs const & /* kwargs */ )
{
    return [beta = is_beta( scope ), fid = params[0] ]( const_dialogue const & d ) -> double {
        const_talker const *actor = d.const_actor( beta );
        json_character_flag jcf( fid.str( d ) );
        if( jcf == json_flag_MUTATION_THRESHOLD )
        {
            return actor->crossed_threshold();
        }
        return actor->has_flag( jcf );
    };
}

diag_eval_dbl_f has_var_eval( char /* scope */, std::vector<diag_value> const &params,
                              diag_kwargs const & /* kwargs */ )
{
    return [var = params[0].var() ]( const_dialogue const & d ) {
        return maybe_read_var_value( var, d ).has_value();
    };
}

diag_eval_dbl_f knows_proficiency_eval( char scope, std::vector<diag_value> const &params,
                                        diag_kwargs const & /* kwargs */ )
{
    return [beta = is_beta( scope ), tid = params[0] ]( const_dialogue const & d ) {
        return d.const_actor( beta )->knows_proficiency( proficiency_id( tid.str( d ) ) );
    };
}

diag_eval_dbl_f hp_eval( char scope, std::vector<diag_value> const &params,
                         diag_kwargs const & /* kwargs */ )
{
    return[bp_val = params[0], beta = is_beta( scope )]( const_dialogue const & d ) {
        std::string const bp_str = bp_val.str( d );
        bool const major = bp_str == "ALL_MAJOR";
        bool const minor = bp_str == "ALL_MINOR";
        if( major || minor ) {
            get_body_part_flags const parts = major ? get_body_part_flags::only_main :
                                              get_body_part_flags::only_minor;
            int ret{};
            for( bodypart_id const &part : d.const_actor( beta )->get_all_body_parts( parts ) ) {
                ret += d.const_actor( beta )->get_cur_hp( part );
            }
            return ret;
        }
        bodypart_id const bp = bp_str == "ALL" ? bodypart_str_id::NULL_ID() : bodypart_id( bp_str );
        return d.const_actor( beta )->get_cur_hp( bp );
    };
}

diag_assign_dbl_f hp_ass( char scope, std::vector<diag_value> const &params,
                          diag_kwargs const & /* kwargs */ )
{
    return [bp_val = params[0], beta = is_beta( scope )]( dialogue const & d, double val ) {
        std::string const bp_str = bp_val.str( d );
        bool const major = bp_str == "ALL_MAJOR";
        bool const minor = bp_str == "ALL_MINOR";
        if( bp_str == "ALL" ) {
            d.actor( beta )->set_all_parts_hp_cur( val );
        } else if( major || minor ) {
            get_body_part_flags const parts = major ? get_body_part_flags::only_main :
                                              get_body_part_flags::only_minor;
            for( bodypart_id const &part : d.actor( beta )->get_all_body_parts( parts ) ) {
                d.actor( beta )->set_part_hp_cur( part, val );
            }
        } else {
            d.actor( beta )->set_part_hp_cur( bodypart_id( bp_str ), val );
        }
    };
}

diag_eval_dbl_f degradation_eval( char scope, std::vector<diag_value> const & /* params */,
                                  diag_kwargs const & )
{
    return[beta = is_beta( scope )]( const_dialogue const & d ) {
        return d.const_actor( beta )->get_degradation();
    };
}
diag_assign_dbl_f degradation_ass( char scope, std::vector<diag_value> const & /* params */,
                                   diag_kwargs const & /* kwargs */ )
{
    return [beta = is_beta( scope )]( dialogue const & d, double val ) {
        d.actor( beta )->set_degradation( val );
    };
}

diag_assign_dbl_f spellcasting_adjustment_ass( char scope, std::vector<diag_value> const &params,
        diag_kwargs const &kwargs )
{
    enum spell_scope : std::uint8_t {
        scope_all,
        scope_mod,
        scope_school,
        scope_spell
    };
    diag_value filter;
    spell_scope spellsearch_scope;
    if( filter = kwargs.kwarg_or( "mod" ); !filter.is_empty() ) {
        spellsearch_scope = scope_mod;
    } else if( filter = kwargs.kwarg_or( "school" ); !filter.is_empty() ) {
        spellsearch_scope = scope_school;
    } else if( filter = kwargs.kwarg_or( "spell" ); !filter.is_empty() ) {
        spellsearch_scope = scope_spell;
    } else {
        spellsearch_scope = scope_all;
    }

    diag_value whitelist = kwargs.kwarg_or( "flag_whitelist" );
    diag_value blacklist = kwargs.kwarg_or( "flag_blacklist" );

    return[beta = is_beta( scope ),
                spellcasting_property = params[0], whitelist, blacklist, spellsearch_scope,
         filter]( dialogue const & d, double val ) {
        std::string const filter_str = filter.str( d );
        switch( spellsearch_scope ) {
            case scope_spell:
                d.actor( beta )->get_character()->magic->get_spell( spell_id( filter_str ) ).set_temp_adjustment(
                    spellcasting_property.str( d ), val );
                break;
            case scope_school: {
                const trait_id school_id( filter_str );
                for( spell *spellIt : d.actor( beta )->get_character()->magic->get_spells() ) {
                    if( spellIt->spell_class() == school_id
                        && ( whitelist.str( d ).empty() || spellIt->has_flag( whitelist.str( d ) ) )
                        && ( blacklist.str( d ).empty() || !spellIt->has_flag( blacklist.str( d ) ) )
                      ) {
                        spellIt->set_temp_adjustment( spellcasting_property.str( d ), val );
                    }
                }
                break;
            }
            case scope_mod: {
                const mod_id target_mod_id( filter_str );
                for( spell *spellIt : d.actor( beta )->get_character()->magic->get_spells() ) {
                    if( get_mod_base_id_from_src( spellIt->get_src() ) == target_mod_id
                        && ( whitelist.str( d ).empty() || spellIt->has_flag( whitelist.str( d ) ) )
                        && ( blacklist.str( d ).empty() || !spellIt->has_flag( blacklist.str( d ) ) )
                      ) {
                        spellIt->set_temp_adjustment( spellcasting_property.str( d ), val );
                    }
                }
                break;
            }
            case scope_all:
                for( spell *spellIt : d.actor( beta )->get_character()->magic->get_spells() ) {
                    if( ( whitelist.str( d ).empty() || spellIt->has_flag( whitelist.str( d ) ) )
                        && ( blacklist.str( d ).empty() || !spellIt->has_flag( blacklist.str( d ) ) )
                      ) {
                        spellIt->set_temp_adjustment( spellcasting_property.str( d ), val );
                    }
                }
                break;
        }
    };
}

diag_eval_dbl_f hp_max_eval( char scope, std::vector<diag_value> const &params,
                             diag_kwargs const & /* kwargs */ )
{
    return[bpid = params[0], beta = is_beta( scope )]( const_dialogue const & d ) {
        bodypart_id bp( bpid.str( d ) );
        return d.const_actor( beta )->get_hp_max( bp );
    };
}

diag_eval_dbl_f item_count_eval( char scope,
                                 std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope ), item_value = params[0]]( const_dialogue const & d ) {
        return d.const_actor( beta )->get_amount( itype_id( item_value.str( d ) ) );
    };
}

diag_eval_dbl_f item_rad_eval( char scope, std::vector<diag_value> const &params,
                               diag_kwargs const &kwargs )
{
    diag_value agg_val  = kwargs.kwarg_or( "aggregate", "min" );

    return [beta = is_beta( scope ), flag = params[0], agg_val]( const_dialogue const & d ) {
        std::optional<aggregate_type> const agg =
            io::string_to_enum_optional<aggregate_type>( agg_val.str( d ) );
        return d.const_actor( beta )->item_rads( flag_id( flag.str( d ) ),
                agg.value_or( aggregate_type::MIN ) );
    };
}

diag_eval_dbl_f num_input_eval( char /*scope*/, std::vector<diag_value> const &params,
                                diag_kwargs const & /* kwargs */ )
{
    return[prompt = params[0], default_val = params[1]]( const_dialogue const & d ) {
        string_input_popup popup;
        double dv = default_val.dbl( d );
        int popup_val = dv;
        popup
        .title( _( "Input a value:" ) )
        .width( 20 )
        .description( prompt.str( d ) )
        .edit( popup_val );
        if( popup.canceled() ) {
            return dv;
        }
        return static_cast<double>( popup_val );
    };
}

diag_eval_dbl_f attack_speed_eval( char scope, std::vector<diag_value> const & /* params */,
                                   diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope )]( const_dialogue const & d ) {
        return d.const_actor( beta )->attack_speed();
    };
}

diag_eval_dbl_f move_speed_eval( char scope, std::vector<diag_value> const & /* params */,
                                 diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope )]( const_dialogue const & d ) {
        return d.const_actor( beta )->get_speed();
    };
}

diag_eval_dbl_f melee_damage_eval( char scope, std::vector<diag_value> const &params,
                                   diag_kwargs const & /* kwargs */ )
{

    return[dt_val = params[0], beta = is_beta( scope )]( const_dialogue const & d ) {
        item_location const *it = d.const_actor( beta )->get_const_item();
        if( it == nullptr ) {
            throw math::runtime_error( "subject of melee_damage() must be an item" );
        }
        std::string const dt_str = dt_val.str( d );
        if( dt_str == "ALL" ) {
            std::vector<damage_type> const &dts = damage_type::get_all();
            return std::accumulate( dts.cbegin(), dts.cend(), 0, [&it]( int a, damage_type const & dt ) {
                return a + ( *it )->damage_melee( dt.id );
            } );
        }
        return ( *it )->damage_melee( damage_type_id( dt_str ) );
    };
}

diag_eval_dbl_f mod_order_eval( char /* scope */, std::vector<diag_value> const &params,
                                diag_kwargs const & /* kwargs */ )
{
    return[mod_val = params[0]]( const_dialogue const & d ) {
        int count = 0;
        mod_id our_mod_id( mod_val.str( d ) );
        for( const mod_id &mod : world_generator->active_world->active_mod_order ) {
            if( our_mod_id == mod ) {
                return count;
            }
            count++;
        }
        return -1;
    };
}

enum class character_filter : std::uint8_t {
    allies = 0,
    not_allies,
    hostile,
    any,
};

bool _friend_match_filter_character( Character const &beta, Character const &guy,
                                     character_filter filter )
{
    switch( filter ) {
        case character_filter::allies:
            return guy.is_ally( beta );
        case character_filter::not_allies:
            return !guy.is_ally( beta );
        case character_filter::hostile:
            return guy.attitude_to( beta ) == Creature::Attitude::HOSTILE ||
                   ( beta.is_avatar() && guy.is_npc() && guy.as_npc()->guaranteed_hostile() );
        case character_filter::any:
            return true;
    }
    return false;
}

bool _filter_character( Character const *beta, Character const &guy, int radius,
                        tripoint_abs_ms const &loc, character_filter filter, bool allow_hallucinations )
{
    if( ( !guy.is_hallucination() || allow_hallucinations ) &&
        ( beta == nullptr || beta->getID() != guy.getID() ) ) {
        return beta == nullptr ||
               ( _friend_match_filter_character( *beta, guy, filter ) &&
                 radius >= rl_dist( guy.get_location(), loc ) );
    }
    return false;
}

// TODO: some form of notation or sentinel value for referencing
// the reality bubble size (since it might change from 60, and
// hardcoding that is unfortunate)
diag_eval_dbl_f _characters_nearby_eval( char scope, std::vector<diag_value> const &params,
        diag_kwargs const &kwargs )
{
    diag_value radius_val = kwargs.kwarg_or( "radius", 1000 );
    diag_value filter_val = kwargs.kwarg_or( "attitude", "any" );
    diag_value allow_hallucinations_val = kwargs.kwarg_or( "allow_hallucinations" );
    std::optional<var_info> loc_var;
    diag_value loc_val = kwargs.kwarg_or( "location" );

    if( !loc_val.is_empty() ) {
        loc_var = loc_val.var();
    } else if( scope == 'g' ) {
        throw math::syntax_error(
            R"("characters_nearby" needs either an actor scope (u/n) or a 'location' kwarg)" );
    }

    return [beta = is_beta( scope ), params, loc_var, filter_val, radius_val,
         allow_hallucinations_val ]( const_dialogue const & d ) {
        tripoint_abs_ms loc;
        if( loc_var.has_value() ) {
            loc = get_tripoint_from_var( loc_var, d, beta );
        } else {
            loc = d.const_actor( beta )->global_pos();
        }

        int const radius = static_cast<int>( radius_val.dbl( d ) );
        std::string const filter_str = filter_val.str( d );
        character_filter filter = character_filter::any;
        if( filter_str == "allies" ) {
            filter = character_filter::allies;
        } else if( filter_str == "not_allies" ) {
            filter = character_filter::not_allies;
        } else if( filter_str == "hostile" ) {
            filter = character_filter::hostile;
        } else if( filter_str != "any" ) {
            throw math::runtime_error(
                R"(Unknown attitude filter "%s" for characters_nearby())", filter_str );
        }
        bool allow_hallucinations = false;
        int const hallucinations_int = static_cast<int>( allow_hallucinations_val.dbl( d ) );
        if( hallucinations_int != 0 ) {
            allow_hallucinations = true;
        }

        std::vector<Character *> const targets = g->get_characters_if( [ &beta, &d, &radius,
               &loc, filter, allow_hallucinations ]( const Character & guy ) {
            const_talker const *const tk = d.const_actor( beta );
            return _filter_character( tk->get_const_character(), guy, radius, loc, filter,
                                      allow_hallucinations );
        } );
        return static_cast<double>( targets.size() );
    };
}

diag_eval_dbl_f characters_nearby_eval( char scope, std::vector<diag_value> const &params,
                                        diag_kwargs const &kwargs )
{
    return _characters_nearby_eval( scope, params, kwargs );
}


template<class ID>
using f_monster_match = bool ( * )( Creature const &critter, ID const &id );

bool mon_check_id( Creature const &critter, mtype_id const &id )
{
    return id == critter.as_monster()->type->id;
}

bool mon_check_species( Creature const &critter, species_id const &id )
{
    return critter.as_monster()->in_species( id );
}

bool mon_check_group( Creature const &critter, mongroup_id const &id )
{
    return MonsterGroupManager::IsMonsterInGroup( id, critter.as_monster()->type->id );
}

enum class mon_filter : std::uint8_t {
    enemies = 0,
    friends,
    both,
};

bool _matches_attitude_filter( Creature const &critter, mon_filter filter )
{
    monster const *mon = critter.as_monster();
    switch( filter ) {
        case mon_filter::enemies:
            return mon->friendly == 0;
        case mon_filter::friends:
            return mon->friendly != 0;
        case mon_filter::both:
            return true;
    }
    return false;
}

template<class ID>
bool _filter_monster( Creature const &critter, std::vector<ID> const &ids, int radius,
                      tripoint_abs_ms const &loc, f_monster_match<ID> f, mon_filter filter )
{
    if( critter.is_monster() ) {
        bool const id_filter =
        ids.empty() || std::any_of( ids.begin(), ids.end(), [&critter, f]( ID const & id ) {
            return ( *f )( critter, id );
        } );

        return id_filter && _matches_attitude_filter( critter, filter ) &&
               radius >= rl_dist( critter.get_location(), loc );
    }
    return false;
}

// TODO: some form of notation or sentinel value for referencing
// the reality bubble size (since it might change from 60, and
// hardcoding that is unfortunate)
template <class ID>
diag_eval_dbl_f _monsters_nearby_eval( char scope, std::vector<diag_value> const &params,
                                       diag_kwargs const &kwargs, f_monster_match<ID> f )
{
    diag_value radius_val = kwargs.kwarg_or( "radius", 1000 );
    diag_value filter_val = kwargs.kwarg_or( "attitude", "hostile" );
    diag_value loc_val =  kwargs.kwarg_or( "location" );
    std::optional<var_info> loc_var;

    if( !loc_val.is_empty() ) {
        loc_var = loc_val.var();
    } else if( scope == 'g' ) {
        throw math::syntax_error(
            R"("monsters_nearby" needs either an actor scope (u/n) or a 'location' kwarg)" );
    }

    return [beta = is_beta( scope ), params, loc_var, radius_val, filter_val,
         f]( const_dialogue const & d ) {
        tripoint_abs_ms loc;
        if( loc_var.has_value() ) {
            loc = get_tripoint_from_var( loc_var, d, beta );
        } else {
            loc = d.const_actor( beta )->global_pos();
        }

        int const radius = static_cast<int>( radius_val.dbl( d ) );
        std::vector<ID> mids( params.size() );
        std::transform( params.begin(), params.end(), mids.begin(), [&d]( diag_value const & val ) {
            return ID( val.str( d ) );
        } );

        std::string const filter_str = filter_val.str( d );
        mon_filter filter = mon_filter::enemies;
        if( filter_str == "both" ) {
            filter = mon_filter::both;
        } else if( filter_str == "friendly" ) {
            filter = mon_filter::friends;
        } else if( filter_str != "hostile" ) {
            throw math::runtime_error(
                R"(Unknown attitude filter "%s" for monsters_nearby())", filter_str );
        }

        std::vector<Creature *> const targets = g->get_creatures_if( [&mids, &radius,
               &loc, f, filter]( const Creature & critter ) {
            return _filter_monster( critter, mids, radius, loc, f, filter );
        } );
        return static_cast<double>( targets.size() );
    };
}

diag_eval_dbl_f monsters_nearby_eval( char scope, std::vector<diag_value> const &params,
                                      diag_kwargs const &kwargs )
{
    return _monsters_nearby_eval( scope, params, kwargs, mon_check_id );
}

diag_eval_dbl_f monster_species_nearby_eval( char scope, std::vector<diag_value> const &params,
        diag_kwargs const &kwargs )
{
    return _monsters_nearby_eval( scope, params, kwargs, mon_check_species );
}

diag_eval_dbl_f monster_groups_nearby_eval( char scope, std::vector<diag_value> const &params,
        diag_kwargs const &kwargs )
{
    return _monsters_nearby_eval( scope, params, kwargs, mon_check_group );
}

diag_eval_dbl_f moon_phase_eval( char /* scope */, std::vector<diag_value> const & /* params */,
                                 diag_kwargs const & /* kwargs */ )
{
    return []( const_dialogue const & /* d */ ) {
        return static_cast<int>( get_moon_phase( calendar::turn ) );
    };
}

diag_eval_dbl_f pain_eval( char scope, std::vector<diag_value> const & /* params */,
                           diag_kwargs const &kwargs )
{
    diag_value format_value = kwargs.kwarg_or( "type", "raw" );

    return [format_value, beta = is_beta( scope )]( const_dialogue const & d ) {
        std::string format = format_value.str( d );
        if( format == "perceived" ) {
            return d.const_actor( beta )->perceived_pain_cur();
        } else if( format == "raw" ) {
            return d.const_actor( beta )->pain_cur();
        } else {
            throw math::runtime_error( R"(Unknown type "%s" for pain())", format );
        }
    };
}

diag_assign_dbl_f pain_ass( char scope, std::vector<diag_value> const & /* params */,
                            diag_kwargs const &kwargs )
{
    diag_value format_value = kwargs.kwarg_or( "type", "raw" );

    return [beta = is_beta( scope ), format_value]( dialogue const & d, double val ) {

        std::string format = format_value.str( d );
        if( format == "perceived" ) {
            d.actor( beta )->mod_pain( val );
        } else if( format == "raw" ) {
            d.actor( beta )->set_pain( val );
        } else {
            throw math::runtime_error( R"(Unknown type "%s" for pain())", format );
        }
    };
}

diag_eval_dbl_f energy_eval( char /* scope */, std::vector<diag_value> const &params,
                             diag_kwargs const & /* kwargs */ )
{
    return [val = params[0]]( const_dialogue const & d ) {
        return static_cast<double>( units::to_millijoule(
                                        _read_from_string<units::energy>( val.str( d ), units::energy_units ) ) );
    };
}

diag_eval_dbl_f school_level_eval( char scope, std::vector<diag_value> const &params,
                                   diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope ), school_value = params[0]]( const_dialogue const & d ) {
        return d.const_actor( beta )->get_spell_level( trait_id( school_value.str( d ) ) );
    };
}

diag_eval_dbl_f school_level_adjustment_eval( char scope, std::vector<diag_value> const &params,
        diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope ), school_value = params[0]]( const_dialogue const & d ) {
        const Character *ch = d.const_actor( beta )->get_const_character();
        if( ch ) {
            const trait_id school( school_value.str( d ) );
            auto it = ch->magic->caster_level_adjustment_by_school.find( school );
            if( it != ch->magic->caster_level_adjustment_by_school.end() ) {
                return it->second;
            } else {
                return 0.0;
            }
        }
        return 0.0;
    };
}

diag_assign_dbl_f school_level_adjustment_ass( char scope, std::vector<diag_value> const &params,
        diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope ), school_value = params[0]]( dialogue const & d, double val ) {
        const Character *ch = d.actor( beta )->get_character();
        if( ch ) {
            const trait_id school( school_value.str( d ) );
            auto it = ch->magic->caster_level_adjustment_by_school.find( school );
            if( it != ch->magic->caster_level_adjustment_by_school.end() ) {
                it->second = val;
            } else {
                ch->magic->caster_level_adjustment_by_school.insert( { school, val } );
            }
        }
        return 0.0;
    };
}

diag_eval_dbl_f get_daily_calories( char scope, std::vector<diag_value> const & /* params */,
                                    diag_kwargs const &kwargs )
{
    diag_value type_val = kwargs.kwarg_or( "type", "total" );
    diag_value day_val = kwargs.kwarg_or( "day" );

    return[beta = is_beta( scope ), day_val, type_val ]( const_dialogue const & d ) {
        std::string type = type_val.str( d );
        int const day = day_val.dbl( d );
        if( day < 0 ) {
            throw math::runtime_error( "get_daily_calories(): cannot access calorie diary from the future (day < 0)" );
        }

        return d.const_actor( beta )->get_daily_calories( day, type );
    };
}

diag_eval_dbl_f skill_eval( char scope, std::vector<diag_value> const &params,
                            diag_kwargs const & /* kwargs */ )
{
    return [beta = is_beta( scope ), sid = params[0] ]( const_dialogue const & d ) {
        return d.const_actor( beta )->get_skill_level( skill_id( sid.str( d ) ) );
    };
}

diag_assign_dbl_f skill_ass( char scope, std::vector<diag_value> const &params,
                             diag_kwargs const & /* kwargs */ )
{
    return [beta = is_beta( scope ), sid = params[0] ]( dialogue const & d, double val ) {
        return d.actor( beta )->set_skill_level( skill_id( sid.str( d ) ), val );
    };
}

diag_eval_dbl_f skill_exp_eval( char scope, std::vector<diag_value> const &params,
                                diag_kwargs const &kwargs )
{
    diag_value format_value = kwargs.kwarg_or( "format", "percentage" );

    return[skill_value = params[0], format_value, beta = is_beta( scope )]( const_dialogue const & d ) {
        skill_id skill( skill_value.str( d ) );
        std::string format = format_value.str( d );
        if( format != "raw" && format != "percentage" ) {
            throw math::runtime_error( R"(Unknown format type "%s" for skill_exp")", format );
        }
        bool raw = format == "raw";
        return d.const_actor( beta )->get_skill_exp( skill, raw );
    };
}

diag_assign_dbl_f skill_exp_ass( char scope, std::vector<diag_value> const &params,
                                 diag_kwargs const &kwargs )
{
    diag_value format_value = kwargs.kwarg_or( "format", "percentage" );

    return [skill_value = params[0], format_value, beta = is_beta( scope ) ]( dialogue const & d,
    double val ) {
        skill_id skill( skill_value.str( d ) );
        std::string format = format_value.str( d );
        if( format != "raw" && format != "percentage" ) {
            throw math::runtime_error( R"(Unknown format type "%s" for skill_exp)", format );
        }
        bool raw = format == "raw";
        return d.actor( beta )->set_skill_exp( skill, val, raw );
    };
}

diag_eval_dbl_f spell_count_eval( char scope, std::vector<diag_value> const & /* params */,
                                  diag_kwargs const &kwargs )
{
    diag_value school_value = kwargs.kwarg_or( "school" );

    return[beta = is_beta( scope ), school_value]( const_dialogue const & d ) {
        std::string school_str = school_value.str( d );
        const trait_id scid = school_str.empty() ? trait_id::NULL_ID() : trait_id( school_str );
        return d.const_actor( beta )->get_spell_count( scid );
    };
}

diag_eval_dbl_f spell_sum_eval( char scope, std::vector<diag_value> const & /* params */,
                                diag_kwargs const &kwargs )
{
    diag_value school_value = kwargs.kwarg_or( "school" );
    diag_value min_level = kwargs.kwarg_or( "level" );

    return[beta = is_beta( scope ), school_value, min_level]( const_dialogue const & d ) {
        std::string school_str = school_value.str( d );
        int const min_spell_level = min_level.dbl( d );
        const trait_id scid = school_str.empty() ? trait_id::NULL_ID() : trait_id( school_str );
        return d.const_actor( beta )->get_spell_sum( scid, min_spell_level );
    };
}

diag_eval_dbl_f spell_exp_eval( char scope, std::vector<diag_value> const &params,
                                diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope ), sid = params[0]]( const_dialogue const & d ) {
        return d.const_actor( beta )->get_spell_exp( spell_id( sid.str( d ) ) );
    };
}

diag_eval_dbl_f spell_exp_for_level_eval( char /* scope */,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[sid = params[0], level = params[1]]( const_dialogue const & d ) -> double {
        std::string sid_str = sid.str( d );
        spell_id spell( sid_str );
        if( spell.is_valid() )
        {
            return spell->exp_for_level( level.dbl( d ) );
        }

        throw math::runtime_error( R"(Unknown spell id "%s" for spell_exp_for_level)", sid_str );
    };
}

diag_assign_dbl_f spell_exp_ass( char scope, std::vector<diag_value> const &params,
                                 diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope ), sid = params[0]]( dialogue const & d, double val ) {
        return d.actor( beta )->set_spell_exp( spell_id( sid.str( d ) ), val );
    };
}

diag_eval_dbl_f spell_level_eval( char scope, std::vector<diag_value> const &params,
                                  diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope ), spell_value = params[0]]( const_dialogue const & d ) {
        const spell_id spell( spell_value.str( d ) );
        if( spell == spell_id::NULL_ID() ) {
            return d.const_actor( beta )->get_highest_spell_level();
        } else {
            return d.const_actor( beta )->get_spell_level( spell );
        }
    };
}

diag_assign_dbl_f spell_level_ass( char scope, std::vector<diag_value> const &params,
                                   diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope ), spell_value = params[0]]( dialogue const & d, double val ) {
        std::string const spell_str = spell_value.str( d );
        const spell_id spell( spell_str );
        if( spell == spell_id::NULL_ID() ) {
            throw math::runtime_error( R"("%s" is not a valid spell)", spell_str );
        } else {
            d.actor( beta )->set_spell_level( spell, val );
        }
        return val;
    };
}

diag_eval_dbl_f spell_level_adjustment_eval( char scope, std::vector<diag_value> const &params,
        diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope ), spell_value = params[0]]( const_dialogue const & d ) {
        const Character *ch = d.const_actor( beta )->get_const_character();
        if( ch ) {
            const spell_id spell( spell_value.str( d ) );
            if( spell == spell_id::NULL_ID() ) {
                return ch->magic->caster_level_adjustment;
            } else {
                std::map<spell_id, double>::iterator it = ch->magic->caster_level_adjustment_by_spell.find( spell );
                if( it != ch->magic->caster_level_adjustment_by_spell.end() ) {
                    return it->second;
                }
            }
        }
        return 0.0;
    };
}

diag_assign_dbl_f spell_level_adjustment_ass( char scope, std::vector<diag_value> const &params,
        diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope ), spell_value = params[0]]( dialogue const & d, double val ) {
        const Character *ch = d.actor( beta )->get_character();
        if( ch ) {
            const spell_id spell( spell_value.str( d ) );
            if( spell == spell_id::NULL_ID() ) {
                ch->magic->caster_level_adjustment = val;
            } else {
                std::map<spell_id, double>::iterator it = ch->magic->caster_level_adjustment_by_spell.find( spell );
                if( it != ch->magic->caster_level_adjustment_by_spell.end() ) {
                    it->second = val;
                } else {
                    ch->magic->caster_level_adjustment_by_spell.insert( { spell, val } );
                }
            }
            return val;
        }
        return 0.0;
    };
}

double _time_in_unit( double time, std::string_view unit )
{
    if( !unit.empty() ) {
        auto const iter = std::find_if( time_duration::units.cbegin(), time_duration::units.cend(),
        [&unit]( std::pair<std::string, time_duration> const & u ) {
            return u.first == unit;
        } );

        if( iter == time_duration::units.end() ) {
            throw math::runtime_error( R"(Unknown time unit "%s")", unit );
        } else {
            return time / to_turns<double>( iter->second );
        }
    }

    return time;
}

diag_eval_dbl_f time_eval( char /* scope */, std::vector<diag_value> const &params,
                           diag_kwargs const &kwargs )
{
    diag_value unit_val = kwargs.kwarg_or( "unit" );

    return [val = params[0], unit_val]( const_dialogue const & d ) {
        std::string const val_str = val.str( d );
        double ret{};
        if( val_str == "now" ) {
            ret = to_turns<double>( calendar::turn - calendar::turn_zero );
        } else if( val_str == "cataclysm" ) {
            ret = to_turns<double>( calendar::start_of_cataclysm - calendar::turn_zero );
        } else {
            ret = to_turns<double>(
                      _read_from_string<time_duration>( val_str, time_duration::units ) );
        }

        return _time_in_unit( ret, unit_val.str( d ) );
    };
}

diag_assign_dbl_f time_ass( char /* scope */, std::vector<diag_value> const &params,
                            diag_kwargs const & /* kwargs */ )
{
    // intentionally duplicate check for str to avoid the `Expected str, got ...` error and get the nicer one below
    if( params[0].is_str() && params[0] == "now" ) {
        return []( dialogue const &/* d */, double val ) {
            calendar::turn = calendar::turn_zero + time_duration::from_turns<double>( val );
        };
    }

    throw math::syntax_error( "Only time('now') is a valid time() assignment target" );
}

diag_eval_dbl_f time_since_eval( char /* scope */, std::vector<diag_value> const &params,
                                 diag_kwargs const &kwargs )
{
    diag_value unit_val = kwargs.kwarg_or( "unit" );

    return [val = params[0], unit_val]( const_dialogue const & d ) {
        double ret{};
        std::string const val_str = val.str( d );
        if( val_str == "cataclysm" ) {
            ret = to_turns<double>( calendar::turn - calendar::start_of_cataclysm );
        } else if( val_str == "midnight" ) {
            ret = to_turns<double>( time_past_midnight( calendar::turn ) );
        } else if( val_str == "noon" ) {
            ret = to_turns<double>( calendar::turn - noon( calendar::turn ) );
        } else if( val.is_var() && !maybe_read_var_value( val.var(), d ).has_value() ) {
            return -1.0;
        } else {
            ret = to_turn<double>( calendar::turn ) - val.dbl( d );
        }
        return _time_in_unit( ret, unit_val.str( d ) );
    };
}

diag_eval_dbl_f time_until_eval( char /* scope */, std::vector<diag_value> const &params,
                                 diag_kwargs const &kwargs )
{
    diag_value unit_val = kwargs.kwarg_or( "unit" );

    return [val = params[0], unit_val]( const_dialogue const & d ) {
        double ret{};
        std::string const val_str = val.str( d );
        if( val_str == "night_time" ) {
            ret = to_turns<double>( night_time( calendar::turn ) - calendar::turn );
        } else if( val_str == "daylight_time" ) {
            ret = to_turns<double>( daylight_time( calendar::turn ) - calendar::turn );
        } else if( val_str == "sunset" ) {
            ret = to_turns<double>( sunset( calendar::turn ) - calendar::turn );
        } else if( val_str == "sunrise" ) {
            ret = to_turns<double>( sunrise( calendar::turn ) - calendar::turn );
        } else if( val_str == "noon" ) {
            ret = to_turns<double>( calendar::turn - noon( calendar::turn ) ) ;
        } else if( val.is_var() && !maybe_read_var_value( val.var(), d ).has_value() ) {
            return -1.0;
        } else {
            ret = val.dbl( d ) - to_turn<double>( calendar::turn );
        }
        if( val_str == "night_time" || val_str == "daylight_time" || val_str == "sunset" ||
            val_str == "sunrise" ) {
            if( ret < 0 ) {
                ret += to_turns<double>( 1_days );
            }
        }
        return _time_in_unit( ret, unit_val.str( d ) );
    };
}

diag_eval_dbl_f time_until_eoc_eval( char /* scope */, std::vector<diag_value> const &params,
                                     diag_kwargs const &kwargs )
{
    diag_value unit_val = kwargs.kwarg_or( "unit" );

    return [eoc_val = params[0], unit_val]( const_dialogue const & d ) -> double {
        effect_on_condition_id eoc_id( eoc_val.str( d ) );
        auto const &list = g->queued_global_effect_on_conditions.list;
        auto const it = std::find_if( list.cbegin(), list.cend(), [&eoc_id]( queued_eoc const & eoc )
        {
            return eoc.eoc == eoc_id;
        } );

        return it != list.end() ? _time_in_unit( to_turn<double>( it->time ), unit_val.str( d ) ) : -1;
    };
}

diag_eval_dbl_f effect_duration_eval( char scope, std::vector<diag_value> const &params,
                                      diag_kwargs const &kwargs )
{
    diag_value bp_val = kwargs.kwarg_or( "bodypart" );
    diag_value unit_val = kwargs.kwarg_or( "unit" );

    return[effect_id = params[0], bp_val, unit_val,
              beta = is_beta( scope )]( const_dialogue const & d ) {
        std::string const bp_str = bp_val.str( d );
        bodypart_id const bp = bp_str.empty() ? bodypart_str_id::NULL_ID() : bodypart_id( bp_str );
        effect target = d.const_actor( beta )->get_effect( efftype_id( effect_id.str( d ) ), bp );
        return target.is_null() ? -1 : _time_in_unit( to_seconds<double>( target.get_duration() ),
                unit_val.str( d ) );
    };
}

diag_eval_dbl_f proficiency_eval( char scope, std::vector<diag_value> const &params,
                                  diag_kwargs const &kwargs )
{
    diag_value fmt_val = kwargs.kwarg_or( "format", "time_spent" );

    return [beta = is_beta( scope ), prof_value = params[0], fmt_val]( const_dialogue const & d ) {
        proficiency_id prof( prof_value.str( d ) );
        time_duration raw = d.const_actor( beta )->proficiency_practiced_time( prof );
        std::string const format = fmt_val.str( d );
        if( format == "percent" ) {
            return raw * 100.0  / prof->time_to_learn();
        } else if( format == "permille" ) {
            return static_cast<double>( raw * 1000  / prof->time_to_learn() );
        } else if( format == "total_time_required" ) {
            return to_turns<double>( prof->time_to_learn() );
        } else if( format == "time_left" ) {
            return to_turns<double>( prof->time_to_learn() - raw );
        } else {
            if( format != "time_spent" ) {
                throw math::runtime_error( R"(Unknown format type "%s" for proficiency)", format );
            }
            return to_turns<double>( raw );
        }
    };
}

diag_assign_dbl_f proficiency_ass( char scope, std::vector<diag_value> const &params,
                                   diag_kwargs const &kwargs )
{
    diag_value fmt_val = kwargs.kwarg_or( "format", "time_spent" );
    diag_value direct_val = kwargs.kwarg_or( "direct" );

    return [prof_value = params[0], fmt_val, direct_val, beta = is_beta( scope )]( dialogue const & d,
    double val ) {
        proficiency_id prof( prof_value.str( d ) );
        std::string const format = fmt_val.str( d );
        bool const direct = is_true( direct_val.dbl( d ) );
        int to_write = 0;
        if( format == "percent" ) {
            to_write = to_turns<int>( prof->time_to_learn() * val ) / 100;
        } else if( format == "permille" ) {
            to_write = to_turns<int>( prof->time_to_learn() * val ) / 1000;
        } else if( format == "time_left" ) {
            to_write = to_turns<int>( prof->time_to_learn() ) - val;
        } else {
            if( format != "time_spent" ) {
                throw math::runtime_error( R"(Unknown format type "%s" for proficiency)", format );
            }
            to_write = val;
        }
        int before = to_turns<int>( d.actor( beta )->proficiency_practiced_time( prof ) );
        int learned = to_write - before;
        // Due to rounding errors, -1 can occur in normal situations. When that happens, ignore it
        if( !direct && learned < 1 ) {
            if( learned < -1 ) {
                throw math::runtime_error( "For proficiency %s in dialogue, trying to learn negative without direct",
                                           prof.str() );
            }
            return 0;
        }
        if( !direct ) {
            d.actor( beta )->train_proficiency_for( prof, learned );
        } else {
            d.actor( beta )->set_proficiency_practiced_time( prof, to_write );
        }
        return 0;
    };
}

double _test_add( diag_value const &v, const_dialogue const &d )
{
    double ret{};
    if( v.is_array() ) {
        for( diag_value const &w : v.array() ) {
            ret += _test_add( w, d );
        }
    } else {
        ret += v.dbl( d );
    }
    return ret;
}
double _test_len( diag_value const &v, const_dialogue const &d )
{
    double ret{};
    for( diag_value const &w : v.array( d ) ) {
        ret += w.str( d ).length();
    }
    return ret;
}
diag_eval_dbl_f _test_func( std::vector<diag_value> const &params, diag_kwargs const &kwargs,
                            double ( *f )( diag_value const &v, const_dialogue const &d ) )
{
    std::vector<diag_value> all_params( params );
    for( diag_kwargs::impl_t::value_type const &v : kwargs.kwargs ) {
        if( v.first != "test_unused_kwarg" ) {
            all_params.emplace_back( *v.second );
        }
    }
    return [all_params, f]( const_dialogue const & d ) {
        double ret = 0;
        for( diag_value const &v : all_params ) {
            ret += f( v, d );
        }
        return ret;
    };
}

diag_eval_dbl_f test_diag( char /* scope */, std::vector<diag_value> const &params,
                           diag_kwargs const &kwargs )
{
    return _test_func( params, kwargs, _test_add );
}

diag_eval_dbl_f test_str_len( char /* scope */, std::vector<diag_value> const &params,
                              diag_kwargs const &kwargs )
{
    return _test_func( params, kwargs, _test_len );
}

diag_eval_dbl_f ugliness_eval( char scope, std::vector<diag_value> const & /* params */,
                               diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope )]( const_dialogue const & d ) {
        return d.const_actor( beta )->get_ugliness();
    };
}

diag_eval_dbl_f value_or_eval( char /* scope */, std::vector<diag_value> const &params,
                               diag_kwargs const & /* kwargs */ )
{
    return[var = params[0].var(),
        vor = params[1]]( const_dialogue const & d ) -> double {
        if( std::optional<std::string> has = maybe_read_var_value( var, d ); has )
        {
            return diag_value{ *has }.dbl( d );
        }
        return vor.dbl( d );
    };
}

diag_eval_dbl_f vision_range_eval( char scope, std::vector<diag_value> const & /* params */,
                                   diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope )]( const_dialogue const & d ) {
        const_talker const *const actor = d.const_actor( beta );
        if( Character const *const chr = actor->get_const_character(); chr != nullptr ) {
            return chr->unimpaired_range();
        } else if( monster const *const mon = actor->get_const_monster(); mon != nullptr ) {
            map &here = get_map();
            tripoint_bub_ms tripoint = get_map().bub_from_abs( mon->get_location() );
            return mon->sight_range( here.ambient_light_at( tripoint ) );
        }
        throw math::runtime_error( "Tried to access vision range of a non-Character talker" );
    };
}

diag_eval_dbl_f npc_anger_eval( char scope, std::vector<diag_value> const & /* params */,
                                diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope )]( const_dialogue const & d ) {
        if( d.const_actor( beta ) ) {
            return d.const_actor( beta )->get_npc_anger();
        } else {
            return 0;
        }
    };
}

diag_eval_dbl_f npc_fear_eval( char scope, std::vector<diag_value> const & /* params */,
                               diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope )]( const_dialogue const & d ) {
        if( d.const_actor( beta ) ) {
            return d.const_actor( beta )->get_npc_fear();
        } else {
            return 0;
        }
    };
}

diag_eval_dbl_f npc_value_eval( char scope, std::vector<diag_value> const & /* params */,
                                diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope )]( const_dialogue const & d ) {
        if( d.const_actor( beta ) ) {
            return d.const_actor( beta )->get_npc_value();
        } else {
            return 0;
        }
    };
}

diag_eval_dbl_f npc_trust_eval( char scope, std::vector<diag_value> const & /* params */,
                                diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope )]( const_dialogue const & d ) {
        if( d.const_actor( beta ) ) {
            return d.const_actor( beta )->get_npc_trust();
        } else {
            return 0;
        }
    };
}

diag_assign_dbl_f npc_anger_ass( char scope, std::vector<diag_value> const & /* params */,
                                 diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope )]( dialogue const & d, double val ) {
        return d.actor( beta )->set_npc_anger( val );
    };
}

diag_assign_dbl_f npc_fear_ass( char scope, std::vector<diag_value> const & /* params */,
                                diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope )]( dialogue const & d, double val ) {
        return d.actor( beta )->set_npc_fear( val );
    };
}

diag_assign_dbl_f npc_value_ass( char scope, std::vector<diag_value> const & /* params */,
                                 diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope )]( dialogue const & d, double val ) {
        return d.actor( beta )->set_npc_value( val );
    };
}

diag_assign_dbl_f npc_trust_ass( char scope, std::vector<diag_value> const & /* params */,
                                 diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope )]( dialogue const & d, double val ) {
        return d.actor( beta )->set_npc_trust( val );
    };
}

diag_eval_dbl_f calories_eval( char scope, std::vector<diag_value> const & /* params */,
                               diag_kwargs const &kwargs )
{
    diag_value format_value = kwargs.kwarg_or( "format", "raw" );
    // dummy kwarg, intentionally discarded!
    diag_value ignore_weariness_val = kwargs.kwarg_or( "dont_affect_weariness" );

    return[format_value, beta = is_beta( scope )]( const_dialogue const & d ) -> double {
        std::string format = format_value.str( d );
        if( format != "raw" && format != "percent" )
        {
            throw math::runtime_error( R"(Unknown format type "%s" for calories)", format );
        }

        if( format == "percent" )
        {
            if( d.const_actor( beta )->get_const_character() ) {
                double divisor = d.const_actor( beta )->get_healthy_kcal() / 100.0;
                //if no data, default to default height of 175cm
                if( divisor == 0 ) {
                    debugmsg( "Can't get healthy amount of calories, return raw calories instead" );
                    return d.const_actor( beta )->get_stored_kcal();
                }
                return d.const_actor( beta )->get_stored_kcal() / divisor;
            } else {
                throw math::runtime_error( "Percent can be used only with character" );
            }
        } else if( format == "raw" )
        {
            if( d.const_actor( beta )->get_const_character() ) {
                return d.const_actor( beta )->get_stored_kcal();
            }
            item_location const *it = d.const_actor( beta )->get_const_item();
            if( it && *it ) {
                return default_character_compute_effective_nutrients( *it->get_item() ).kcal();
            }
        }
        throw math::runtime_error( "For calories(), talker is not character nor item" );
    };
}

diag_assign_dbl_f calories_ass( char scope, std::vector<diag_value> const & /* params */,
                                diag_kwargs const &kwargs )
{
    diag_value ignore_weariness_val = kwargs.kwarg_or( "dont_affect_weariness" );

    return[ignore_weariness_val, beta = is_beta( scope ) ]( dialogue const & d, double val ) {
        const bool ignore_weariness = is_true( ignore_weariness_val.dbl( d ) );
        int current_kcal = d.actor( beta )->get_stored_kcal();
        int difference = val - current_kcal;
        d.actor( beta )->mod_stored_kcal( difference, ignore_weariness );
    };
}

diag_eval_dbl_f weight_eval( char scope, std::vector<diag_value> const & /* params */,
                             diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope )]( const_dialogue const & d ) {
        if( d.const_actor( beta )->get_const_character() || d.const_actor( beta )->get_const_monster() ) {
            return d.const_actor( beta )->get_weight();
        }
        item_location const *it = d.const_actor( beta )->get_const_item();
        if( it && *it ) {
            return static_cast<int>( to_milligram( it->get_item()->weight() ) );
        }
        throw math::runtime_error( "For weight(), talker is not character nor item" );
    };
}

diag_eval_dbl_f volume_eval( char scope, std::vector<diag_value> const & /* params */,
                             diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope )]( const_dialogue const & d ) {
        if( d.const_actor( beta )->get_const_character() || d.const_actor( beta )->get_const_monster() ) {
            return d.const_actor( beta )->get_volume();
        }
        item_location const *it = d.const_actor( beta )->get_const_item();
        if( it && *it ) {
            return to_milliliter( it->get_item()->volume() );
        }
        throw math::runtime_error( "For volume(), talker is not character nor item" );
    };
}

diag_eval_dbl_f vitamin_eval( char scope, std::vector<diag_value> const &params,
                              diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope ), id = params[0]]( const_dialogue const & d ) {
        const_talker const *const actor = d.const_actor( beta );
        if( Character const *const chr = actor->get_const_character(); chr != nullptr ) {
            return chr->vitamin_get( vitamin_id( id.str( d ) ) );
        }
        if( item_location const *const itm = actor->get_const_item(); itm != nullptr ) {
            const std::map<vitamin_id, int> &vitamin_data =
                default_character_compute_effective_nutrients( *itm->get_item() ).vitamins();
            const auto &v = vitamin_data.find( vitamin_id( id.str( d ) ) );
            return v != vitamin_data.end() ? v->second : 0;
        }
        throw math::runtime_error( "Tried to access vitamins of a non-Character/non-item talker" );
    };
}

diag_assign_dbl_f vitamin_ass( char scope, std::vector<diag_value> const &params,
                               diag_kwargs const & /* kwargs */ )
{
    return[beta = is_beta( scope ), id = params[0]]( dialogue const & d, double val ) {
        if( d.actor( beta )->get_character() ) {
            d.actor( beta )->get_character()->vitamin_set( vitamin_id( id.str( d ) ), val );
        }
    };
}

diag_eval_dbl_f warmth_eval( char scope, std::vector<diag_value> const &params,
                             diag_kwargs const & /* kwargs */ )
{
    return[bpid = params[0], beta = is_beta( scope )]( const_dialogue const & d ) {
        bodypart_id bp( bpid.str( d ) );
        return units::to_legacy_bodypart_temp( d.const_actor( beta )->get_cur_part_temp( bp ) );
    };
}

diag_eval_dbl_f weather_eval( char /* scope */, std::vector<diag_value> const &params,
                              diag_kwargs const & /* kwargs */ )
{
    if( params[0] == "temperature" ) {
        return []( const_dialogue const & ) {
            return units::to_kelvin( get_weather().weather_precise->temperature );
        };
    }
    if( params[0] == "windpower" ) {
        return []( const_dialogue const & ) {
            return get_weather().weather_precise->windpower;
        };
    }
    if( params[0] == "humidity" ) {
        return []( const_dialogue const & ) {
            return get_weather().weather_precise->humidity;
        };
    }
    if( params[0] == "pressure" ) {
        return []( const_dialogue const & ) {
            return get_weather().weather_precise->pressure;
        };
    }
    if( params[0] == "precipitation" ) {
        return []( const_dialogue const & ) {
            return precip_mm_per_hour( get_weather().weather_id->precip );
        };
    }
    throw math::syntax_error( "Unknown weather aspect %s", params[0].str() );
}

diag_assign_dbl_f weather_ass( char /* scope */, std::vector<diag_value> const &params,
                               diag_kwargs const & /* kwargs */ )
{
    if( params[0] == "temperature" ) {
        return []( dialogue const &, double val ) {
            get_weather().weather_precise->temperature = units::from_kelvin( val );
            get_weather().temperature = units::from_kelvin( val );
            get_weather().clear_temp_cache();
        };
    }
    if( params[0] == "windpower" ) {
        return []( dialogue const &, double val ) {
            get_weather().weather_precise->windpower = val;
            get_weather().clear_temp_cache();
        };
    }
    if( params[0] == "humidity" ) {
        return []( dialogue const &, double val ) {
            get_weather().weather_precise->humidity = val;
            get_weather().clear_temp_cache();
        };
    }
    if( params[0] == "pressure" ) {
        return []( dialogue const &, double val ) {
            get_weather().weather_precise->pressure = val;
            get_weather().clear_temp_cache();
        };
    }
    throw math::syntax_error( "Unknown weather aspect %s", params[0].str() );
}

diag_eval_dbl_f climate_control_str_heat_eval( char scope,
        std::vector<diag_value> const &/* params */, diag_kwargs const &/* kwargs */ )
{
    return [beta = is_beta( scope )]( const_dialogue const & d ) {
        return d.const_actor( beta )->climate_control_str_heat();
    };
}

diag_eval_dbl_f climate_control_str_chill_eval( char scope,
        std::vector<diag_value> const &/* params */, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope )]( const_dialogue const & d ) {
        return d.const_actor( beta )->climate_control_str_chill();
    };
}

// { "name", { "scopes", num_args, eval function, assign function } }
// kwargs are not included in num_args
std::map<std::string_view, dialogue_func> const dialogue_funcs{
    { "_test_diag_", { "g", -1, test_diag } },
    { "_test_str_len_", { "g", -1, test_str_len } },
    { "addiction_intensity", { "un", 1, addiction_intensity_eval } },
    { "addiction_turns", { "un", 1, addiction_turns_eval, addiction_turns_ass } },
    { "armor", { "un", 2, armor_eval } },
    { "attack_speed", { "un", 0, attack_speed_eval } },
    { "speed", { "un", 0, move_speed_eval } },
    { "characters_nearby", { "ung", 0, characters_nearby_eval } },
    { "charge_count", { "un", 1, charge_count_eval } },
    { "coverage", { "un", 1, coverage_eval } },
    { "damage_level", { "un", 0, damage_level_eval } },
    { "degradation", { "un", 0, degradation_eval, degradation_ass } },
    { "distance", { "g", 2, distance_eval } },
    { "effect_intensity", { "un", 1, effect_intensity_eval } },
    { "effect_duration", { "un", 1, effect_duration_eval } },
    { "health", { "un", 0, health_eval, health_ass } },
    { "encumbrance", { "un", 1, encumbrance_eval } },
    { "energy", { "g", 1, energy_eval } },
    { "faction_like", { "g", 1, faction_like_eval, faction_like_ass } },
    { "faction_respect", { "g", 1, faction_respect_eval, faction_respect_ass } },
    { "faction_trust", { "g", 1, faction_trust_eval, faction_trust_ass } },
    { "faction_food_supply", { "g", 1, faction_food_supply_eval, faction_food_supply_ass } },
    { "faction_wealth", { "g", 1, faction_wealth_eval, faction_wealth_ass } },
    { "faction_power", { "g", 1, faction_power_eval, faction_power_ass } },
    { "faction_size", { "g", 1, faction_size_eval, faction_size_ass } },
    { "field_strength", { "ung", 1, field_strength_eval } },
    { "gun_damage", { "un", 1, gun_damage_eval } },
    { "game_option", { "g", 1, option_eval } },
    { "has_flag", { "un", 1, has_flag_eval } },
    { "has_trait", { "un", 1, has_trait_eval } },
    { "sum_traits_of_category", { "un", 1, sum_traits_of_category_eval } },
    { "sum_traits_of_category_char_has", { "un", 1, sum_traits_of_category_char_has_eval } },
    { "has_proficiency", { "un", 1, knows_proficiency_eval } },
    { "has_var", { "g", 1, has_var_eval } },
    { "hp", { "un", 1, hp_eval, hp_ass } },
    { "hp_max", { "un", 1, hp_max_eval } },
    { "item_count", { "un", 1, item_count_eval } },
    { "item_rad", { "un", 1, item_rad_eval } },
    { "melee_damage", { "un", 1, melee_damage_eval } },
    { "mod_load_order", { "g", 1, mod_order_eval } },
    { "monsters_nearby", { "ung", -1, monsters_nearby_eval } },
    { "mon_species_nearby", { "ung", -1, monster_species_nearby_eval } },
    { "mon_groups_nearby", { "ung", -1, monster_groups_nearby_eval } },
    { "moon_phase", { "g", 0, moon_phase_eval } },
    { "num_input", { "g", 2, num_input_eval } },
    { "pain", { "un", 0, pain_eval, pain_ass } },
    { "school_level", { "un", 1, school_level_eval}},
    { "school_level_adjustment", { "un", 1, school_level_adjustment_eval, school_level_adjustment_ass } },
    { "spellcasting_adjustment", { "u", 1, nullptr, spellcasting_adjustment_ass } },
    { "get_calories_daily", { "g", 0, get_daily_calories } },
    { "skill", { "un", 1, skill_eval, skill_ass } },
    { "skill_exp", { "un", 1, skill_exp_eval, skill_exp_ass } },
    { "spell_count", { "un", 0, spell_count_eval}},
    { "spell_level_sum", { "un", 0, spell_sum_eval}},
    { "spell_exp", { "un", 1, spell_exp_eval, spell_exp_ass }},
    { "spell_exp_for_level", { "g", 2, spell_exp_for_level_eval}},
    { "spell_level", { "un", 1, spell_level_eval, spell_level_ass }},
    { "spell_level_adjustment", { "un", 1, spell_level_adjustment_eval, spell_level_adjustment_ass } },
    { "time", { "g", 1, time_eval, time_ass } },
    { "time_since", { "g", 1, time_since_eval } },
    { "time_until", { "g", 1, time_until_eval } },
    { "time_until_eoc", { "g", 1, time_until_eoc_eval } },
    { "proficiency", { "un", 1, proficiency_eval, proficiency_ass } },
    { "val", { "un", 1, u_val, u_val_ass } },
    { "npc_anger", { "un", 0, npc_anger_eval, npc_anger_ass } },
    { "npc_fear", { "un", 0, npc_fear_eval, npc_fear_ass } },
    { "npc_value", { "un", 0, npc_value_eval, npc_value_ass } },
    { "npc_trust", { "un", 0, npc_trust_eval, npc_trust_ass } },
    { "ugliness", { "un", 0, ugliness_eval } },
    { "value_or", { "g", 2, value_or_eval } },
    { "vision_range", { "un", 0, vision_range_eval } },
    { "vitamin", { "un", 1, vitamin_eval, vitamin_ass } },
    { "calories", { "un", 0, calories_eval, calories_ass } },
    { "weight", { "un", 0, weight_eval } },
    { "volume", { "un", 0, volume_eval } },
    { "warmth", { "un", 1, warmth_eval } },
    { "weather", { "g", 1, weather_eval, weather_ass } },
    { "climate_control_str_heat", { "un", 0, climate_control_str_heat_eval } },
    { "climate_control_str_chill", { "un", 0, climate_control_str_chill_eval } },
};

} // namespace

std::map<std::string_view, dialogue_func> const &get_all_diag_funcs()
{
    return dialogue_funcs;
}
