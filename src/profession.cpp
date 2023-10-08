#include "profession.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <map>
#include <memory>

#include "game.h"
#include "achievement.h"
#include "addiction.h"
#include "avatar.h"
#include "calendar.h"
#include "debug.h"
#include "flag.h"
#include "generic_factory.h"
#include "item.h"
#include "item_group.h"
#include "itype.h"
#include "json.h"
#include "magic.h"
#include "mission.h"
#include "mutation.h"
#include "options.h"
#include "past_games_info.h"
#include "pimpl.h"
#include "translations.h"
#include "type_id.h"
#include "visitable.h"

static const achievement_id achievement_achievement_arcade_mode( "achievement_arcade_mode" );

namespace
{
generic_factory<profession> all_profs( "profession" );
} // namespace

static class json_item_substitution
{
    public:
        void reset();
        void load( const JsonObject &jo );
        void check_consistency();

    private:
        struct trait_requirements {
            explicit trait_requirements( const JsonObject &obj );
            trait_requirements() = default;
            std::vector<trait_id> present;
            std::vector<trait_id> absent;
            bool meets_condition( const std::vector<trait_id> &traits ) const;
        };
        struct substitution {
            trait_requirements trait_reqs;
            struct info {
                explicit info( const JsonValue &value );
                info() = default;
                itype_id new_item;
                double ratio = 1.0; // new charges / old charges
            };
            std::vector<info> infos;
        };
        std::map<itype_id, std::vector<substitution>> substitutions;
        std::vector<std::pair<item_group_id, trait_requirements>> bonuses;
    public:
        std::vector<item> get_bonus_items( const std::vector<trait_id> &traits ) const;
        std::vector<item> get_substitution( const item &it, const std::vector<trait_id> &traits ) const;
} item_substitutions;

/** @relates string_id */
template<>
const profession &string_id<profession>::obj() const
{
    return all_profs.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<profession>::is_valid() const
{
    return all_profs.is_valid( *this );
}

profession::profession()
    : _name_male( no_translation( "null" ) ),
      _name_female( no_translation( "null" ) ),
      _description_male( no_translation( "null" ) ),
      _description_female( no_translation( "null" ) )
{
}

void profession::load_profession( const JsonObject &jo, const std::string &src )
{
    all_profs.load( jo, src );
}

class skilllevel_reader : public generic_typed_reader<skilllevel_reader>
{
    public:
        std::pair<skill_id, int> get_next( JsonValue &jv ) const {
            JsonObject jo = jv.get_object();
            return std::pair<skill_id, int>( skill_id( jo.get_string( "name" ) ), jo.get_int( "level" ) );
        }
        template<typename C>
        void erase_next( std::string &&id_str, C &container ) const {
            const skill_id id = skill_id( std::move( id_str ) );
            reader_detail::handler<C>().erase_if( container, [&id]( const std::pair<skill_id, int> &e ) {
                return e.first == id;
            } );
        }
};

class addiction_reader : public generic_typed_reader<addiction_reader>
{
    public:
        addiction get_next( JsonValue &jv ) const {
            JsonObject jo = jv.get_object();
            return addiction( addiction_id( jo.get_string( "type" ) ), jo.get_int( "intensity" ) );
        }
        template<typename C>
        void erase_next( std::string &&type_str, C &container ) const {
            const addiction_id type( type_str );
            reader_detail::handler<C>().erase_if( container, [&type]( const addiction & e ) {
                return e.type == type;
            } );
        }
};

class item_reader : public generic_typed_reader<item_reader>
{
    public:
        profession::itypedec get_next( JsonValue &jv ) const {
            // either a plain item type id string, or an array with item type id
            // and as second entry the item description.
            if( jv.test_string() ) {
                return profession::itypedec( jv.get_string() );
            }
            JsonArray jarr = jv.get_array();
            const std::string id = jarr.get_string( 0 );
            const snippet_id snippet( jarr.get_string( 1 ) );
            return profession::itypedec( id, snippet );
        }
        template<typename C>
        void erase_next( std::string &&id, C &container ) const {
            reader_detail::handler<C>().erase_if( container, [&id]( const profession::itypedec & e ) {
                return e.type_id.str() == id;
            } );
        }
};

void profession::load( const JsonObject &jo, const std::string_view )
{
    //If the "name" is an object then we have to deal with gender-specific titles,
    if( jo.has_object( "name" ) ) {
        JsonObject name_obj = jo.get_object( "name" );
        // Specifying translation context here to avoid adding unnecessary json code for every profession
        // NOLINTNEXTLINE(cata-json-translation-input)
        _name_male = to_translation( "profession_male", name_obj.get_string( "male" ) );
        // NOLINTNEXTLINE(cata-json-translation-input)
        _name_female = to_translation( "profession_female", name_obj.get_string( "female" ) );
    } else if( jo.has_string( "name" ) ) {
        // Same profession names for male and female in English.
        // Still need to different names in other languages.
        const std::string name = jo.get_string( "name" );
        _name_female = to_translation( "profession_female", name );
        _name_male = to_translation( "profession_male", name );
    } else if( !was_loaded ) {
        jo.throw_error( "missing mandatory member \"name\"" );
    }

    if( !was_loaded || jo.has_member( "description" ) ) {
        std::string desc;
        std::string desc_male;
        std::string desc_female;

        bool use_default_description = true;
        if( jo.has_object( "description" ) ) {
            JsonObject desc_obj = jo.get_object( "description" );
            desc_obj.allow_omitted_members();

            if( desc_obj.has_member( "male" ) && desc_obj.has_member( "female" ) ) {
                use_default_description = false;
                mandatory( desc_obj, false, "male", desc_male, text_style_check_reader() );
                mandatory( desc_obj, false, "female", desc_female, text_style_check_reader() );
            }
        }

        if( use_default_description ) {
            mandatory( jo, false, "description", desc, text_style_check_reader() );
            desc_male = desc;
            desc_female = desc;
        }
        // These also may differ depending on the language settings!
        _description_male = to_translation( "prof_desc_male", desc_male );
        _description_female = to_translation( "prof_desc_female", desc_female );
    }
    optional( jo, was_loaded, "age_lower", age_lower, 16 );
    optional( jo, was_loaded, "age_upper", age_upper, 55 );

    if( jo.has_string( "vehicle" ) ) {
        _starting_vehicle = vproto_id( jo.get_string( "vehicle" ) );
    }
    if( jo.has_array( "pets" ) ) {
        for( JsonObject subobj : jo.get_array( "pets" ) ) {
            int count = subobj.get_int( "amount" );
            mtype_id mon = mtype_id( subobj.get_string( "name" ) );
            for( int start = 0; start < count; ++start ) {
                _starting_pets.push_back( mon );
            }
        }
    }

    if( jo.has_array( "spells" ) ) {
        for( JsonObject subobj : jo.get_array( "spells" ) ) {
            int level = subobj.get_int( "level" );
            spell_id sp = spell_id( subobj.get_string( "id" ) );
            _starting_spells.emplace( sp, level );
        }
    }

    mandatory( jo, was_loaded, "points", _point_cost );

    if( !was_loaded || jo.has_member( "items" ) ) {
        std::string c = "items for profession " + id.str();
        JsonObject items_obj = jo.get_object( "items" );

        if( items_obj.has_array( "both" ) ) {
            optional( items_obj, was_loaded, "both", legacy_starting_items, item_reader {} );
        }
        if( items_obj.has_object( "both" ) ) {
            _starting_items = item_group::load_item_group(
                                  items_obj.get_member( "both" ), "collection", c );
        }
        if( items_obj.has_array( "male" ) ) {
            optional( items_obj, was_loaded, "male", legacy_starting_items_male, item_reader {} );
        }
        if( items_obj.has_object( "male" ) ) {
            _starting_items_male = item_group::load_item_group(
                                       items_obj.get_member( "male" ), "collection", c );
        }
        if( items_obj.has_array( "female" ) ) {
            optional( items_obj, was_loaded, "female",  legacy_starting_items_female, item_reader {} );
        }
        if( items_obj.has_object( "female" ) ) {
            _starting_items_female = item_group::load_item_group( items_obj.get_member( "female" ),
                                     "collection", c );
        }
    }
    optional( jo, was_loaded, "no_bonus", no_bonus );

    optional( jo, was_loaded, "requirement", _requirement );

    optional( jo, was_loaded, "skills", _starting_skills, skilllevel_reader {} );
    optional( jo, was_loaded, "addictions", _starting_addictions, addiction_reader {} );
    // TODO: use string_id<bionic_type> or so
    optional( jo, was_loaded, "CBMs", _starting_CBMs, string_id_reader<::bionic_data> {} );
    optional( jo, was_loaded, "proficiencies", _starting_proficiencies );
    // TODO: use string_id<mutation_branch> or so
    optional( jo, was_loaded, "traits", _starting_traits );
    optional( jo, was_loaded, "forbidden_traits", _forbidden_traits,
              string_id_reader<::mutation_branch> {} );
    optional( jo, was_loaded, "flags", flags, auto_flags_reader<> {} );

    // Flag which denotes if a profession is a hobby
    optional( jo, was_loaded, "subtype", _subtype, "" );
    optional( jo, was_loaded, "missions", _missions, string_id_reader<::mission_type> {} );
}

const profession *profession::generic()
{
    const string_id<profession> generic_profession_id(
        get_option<std::string>( "GENERIC_PROFESSION_ID" ) );
    return &generic_profession_id.obj();
}

const std::vector<profession> &profession::get_all()
{
    return all_profs.get_all();
}

std::vector<string_id<profession>> profession::get_all_hobbies()
{
    std::vector<profession> all = profession::get_all();
    std::vector<profession_id> ret;

    // remove all non-hobbies from list of professions
    const auto new_end = std::remove_if( all.begin(),
    all.end(), [&]( const profession & arg ) {
        return !arg.is_hobby();
    } );
    all.erase( new_end, all.end() );

    // convert to string_id's then return
    for( const profession &p : all ) {
        ret.emplace( ret.end(), p.ident() );
    }
    return ret;
}

void profession::reset()
{
    all_profs.reset();
    item_substitutions.reset();
}

void profession::check_definitions()
{
    item_substitutions.check_consistency();
    for( const profession &prof : all_profs.get_all() ) {
        prof.check_definition();
    }
}

void profession::check_item_definitions( const itypedecvec &items ) const
{
    for( const profession::itypedec &itd : items ) {
        if( !item::type_is_defined( itd.type_id ) ) {
            debugmsg( "profession %s: item %s does not exist", id.str(), itd.type_id.str() );
        } else if( !itd.snip_id.is_null() ) {
            const itype *type = item::find_type( itd.type_id );
            if( type->snippet_category.empty() ) {
                debugmsg( "profession %s: item %s has no snippet category - no description can "
                          "be set", id.str(), itd.type_id.str() );
            } else {
                if( !itd.snip_id.is_valid() ) {
                    debugmsg( "profession %s: there's no snippet with id %s",
                              id.str(), itd.snip_id.str() );
                }
            }
        }
    }
}

void profession::check_definition() const
{
    check_item_definitions( legacy_starting_items );
    check_item_definitions( legacy_starting_items_female );
    check_item_definitions( legacy_starting_items_male );
    if( !no_bonus.is_empty() && !item::type_is_defined( no_bonus ) ) {
        debugmsg( "no_bonus item '%s' is not an itype_id", no_bonus.c_str() );
    }

    if( !item_group::group_is_defined( _starting_items ) ) {
        debugmsg( "_starting_items group is undefined" );
    }
    if( !item_group::group_is_defined( _starting_items_male ) ) {
        debugmsg( "_starting_items_male group is undefined" );
    }
    if( !item_group::group_is_defined( _starting_items_female ) ) {
        debugmsg( "_starting_items_female group is undefined" );
    }
    if( _starting_vehicle && !_starting_vehicle.is_valid() ) {
        debugmsg( "vehicle prototype %s for profession %s does not exist", _starting_vehicle.c_str(),
                  id.c_str() );
    }
    for( const auto &a : _starting_CBMs ) {
        if( !a.is_valid() ) {
            debugmsg( "bionic %s for profession %s does not exist", a.c_str(), id.c_str() );
        }
    }

    for( const proficiency_id &pid : _starting_proficiencies ) {
        if( !pid.is_valid() ) {
            debugmsg( "proficiency %s for profession %s does not exist", pid.str(), id.str() );
        }
    }

    for( const trait_and_var &t : _starting_traits ) {
        if( !t.trait.is_valid() ) {
            debugmsg( "trait %s for profession %s does not exist", t.trait.str(), id.str() );
        }
    }
    for( const auto &elem : _starting_pets ) {
        if( !elem.is_valid() ) {
            debugmsg( "starting pet %s for profession %s does not exist", elem.c_str(), id.c_str() );
        }
    }
    for( const auto &elem : _starting_skills ) {
        if( !elem.first.is_valid() ) {
            debugmsg( "skill %s for profession %s does not exist", elem.first.c_str(), id.c_str() );
        }
    }

    for( const auto &m : _missions ) {
        if( !m.is_valid() ) {
            debugmsg( "starting mission %s for profession %s does not exist", m.c_str(), id.c_str() );
        }

        if( std::find( m->origins.begin(), m->origins.end(), ORIGIN_GAME_START ) == m->origins.end() ) {
            debugmsg( "starting mission %s for profession %s must include an origin of ORIGIN_GAME_START",
                      m.c_str(), id.c_str() );
        }
    }
}

bool profession::has_initialized()
{
    if( !g || g->new_game ) {
        return false;
    }
    const string_id<profession> generic_profession_id(
        get_option<std::string>( "GENERIC_PROFESSION_ID" ) );
    return generic_profession_id.is_valid();
}

const string_id<profession> &profession::ident() const
{
    return id;
}

std::string profession::gender_appropriate_name( bool male ) const
{
    if( male ) {
        return _name_male.translated();
    } else {
        return _name_female.translated();
    }
}

std::string profession::description( bool male ) const
{
    if( male ) {
        return _description_male.translated();
    } else {
        return _description_female.translated();
    }
}

static time_point advanced_spawn_time()
{
    return calendar::start_of_game;
}

signed int profession::point_cost() const
{
    return _point_cost;
}

static void clear_faults( item &it )
{
    if( it.get_var( "dirt", 0 ) > 0 ) {
        it.set_var( "dirt", 0 );
    }
    it.faults.clear();
}

std::list<item> profession::items( bool male, const std::vector<trait_id> &traits ) const
{
    std::list<item> result;
    auto add_legacy_items = [&result]( const itypedecvec & vec ) {
        for( const itypedec &elem : vec ) {
            item it( elem.type_id, advanced_spawn_time(), item::default_charges_tag {} );
            if( !elem.snip_id.is_null() ) {
                it.set_snippet( elem.snip_id );
            }
            it = it.in_its_container();
            result.push_back( it );
        }
    };

    add_legacy_items( legacy_starting_items );
    add_legacy_items( male ? legacy_starting_items_male : legacy_starting_items_female );

    std::vector<item> group_both = item_group::items_from( _starting_items,
                                   advanced_spawn_time() );
    std::vector<item> group_gender = item_group::items_from( male ? _starting_items_male :
                                     _starting_items_female, advanced_spawn_time() );
    result.insert( result.begin(), std::make_move_iterator( group_both.begin() ),
                   std::make_move_iterator( group_both.end() ) );
    result.insert( result.begin(), std::make_move_iterator( group_gender.begin() ),
                   std::make_move_iterator( group_gender.end() ) );

    if( !has_flag( "NO_BONUS_ITEMS" ) ) {
        const std::vector<item> &items = item_substitutions.get_bonus_items( traits );
        for( const item &it : items ) {
            if( it.typeId() != no_bonus ) {
                result.push_back( it );
            }
        }
    }
    for( auto iter = result.begin(); iter != result.end(); ) {
        const std::vector<item> sub = item_substitutions.get_substitution( *iter, traits );
        if( !sub.empty() ) {
            result.insert( result.begin(), sub.begin(), sub.end() );
            iter = result.erase( iter );
        } else {
            ++iter;
        }
    }
    for( item &it : result ) {
        it.visit_items( []( item * it, item * ) {
            clear_faults( *it );
            return VisitResponse::NEXT;
        } );
        if( it.has_flag( flag_VARSIZE ) ) {
            it.set_flag( flag_FIT );
        }
    }

    if( result.empty() ) {
        // No need to do the below stuff. Plus it would cause said below stuff to crash
        return result;
    }

    // Merge charges for items that stack with each other
    for( auto outer = result.begin(); outer != result.end(); ++outer ) {
        if( !outer->count_by_charges() ) {
            continue;
        }
        for( auto inner = std::next( outer ); inner != result.end(); ) {
            if( outer->stacks_with( *inner ) ) {
                outer->merge_charges( *inner );
                inner = result.erase( inner );
            } else {
                ++inner;
            }
        }
    }

    result.sort( []( const item & first, const item & second ) {
        return first.get_layer() < second.get_layer();
    } );
    return result;
}

vproto_id profession::vehicle() const
{
    return _starting_vehicle;
}

std::vector<mtype_id> profession::pets() const
{
    return _starting_pets;
}

std::vector<addiction> profession::addictions() const
{
    return _starting_addictions;
}

std::vector<bionic_id> profession::CBMs() const
{
    return _starting_CBMs;
}

std::vector<proficiency_id> profession::proficiencies() const
{
    return _starting_proficiencies;
}

std::vector<trait_and_var> profession::get_locked_traits() const
{
    return _starting_traits;
}

std::set<trait_id> profession::get_forbidden_traits() const
{
    return _forbidden_traits;
}

profession::StartingSkillList profession::skills() const
{
    return _starting_skills;
}

bool profession::has_flag( const std::string &flag ) const
{
    return flags.count( flag ) != 0;
}

ret_val<void> profession::can_afford( const Character &you, const int points ) const
{
    if( point_cost() - you.prof->point_cost() <= points ) {
        return ret_val<void>::make_success();
    }

    return ret_val<void>::make_failure( _( "You don't have enough points" ) );
}

ret_val<void> profession::can_pick() const
{
    // if meta progression is disabled then skip this
    if( get_past_games().achievement( achievement_achievement_arcade_mode ) ||
        !get_option<bool>( "META_PROGRESS" ) ) {
        return ret_val<void>::make_success();
    }

    if( _requirement ) {
        const achievement_completion_info *other_games = get_past_games().achievement(
                    _requirement.value()->id );
        if( !other_games ) {
            return ret_val<void>::make_failure(
                       _( "You must complete the achievement \"%s\" to unlock this profession." ),
                       _requirement.value()->name() );
        } else if( other_games->games_completed.empty() ) {
            return ret_val<void>::make_failure(
                       _( "You must complete the achievement \"%s\" to unlock this profession." ),
                       _requirement.value()->name() );
        }
    }

    return ret_val<void>::make_success();
}

bool profession::is_locked_trait( const trait_id &trait ) const
{
    auto pred = [&trait]( const trait_and_var & query ) {
        return query.trait == trait;
    };
    return std::find_if( _starting_traits.begin(), _starting_traits.end(),
                         pred ) != _starting_traits.end();
}

bool profession::is_forbidden_trait( const trait_id &trait ) const
{
    return _forbidden_traits.count( trait ) != 0;
}

std::map<spell_id, int> profession::spells() const
{
    return _starting_spells;
}

void profession::learn_spells( avatar &you ) const
{
    for( const std::pair<spell_id, int> spell_pair : spells() ) {
        you.magic->learn_spell( spell_pair.first, you, true );
        spell &sp = you.magic->get_spell( spell_pair.first );
        while( sp.get_level() < spell_pair.second && !sp.is_max_level( you ) ) {
            sp.gain_level( you );
        }
    }
}

// item_substitution stuff:

void profession::load_item_substitutions( const JsonObject &jo )
{
    item_substitutions.load( jo );
}

void json_item_substitution::reset()
{
    substitutions.clear();
    bonuses.clear();
}

json_item_substitution::substitution::info::info( const JsonValue &value )
{
    if( value.test_string() ) {
        value.read( new_item, true );
    } else {
        const JsonObject jo = value.get_object();
        jo.read( "item", new_item, true );
        ratio = jo.get_float( "ratio" );
        if( ratio <= 0.0 ) {
            jo.throw_error_at( "ratio", "Ratio must be positive" );
        }
    }
}

json_item_substitution::trait_requirements::trait_requirements( const JsonObject &obj )
{
    for( const std::string line : obj.get_array( "present" ) ) {
        present.emplace_back( line );
    }
    for( const std::string line : obj.get_array( "absent" ) ) {
        absent.emplace_back( line );
    }
}

void json_item_substitution::load( const JsonObject &jo )
{
    auto check_duplicate_item = [&]( const itype_id & it ) {
        return substitutions.find( it ) != substitutions.end();
    };

    if( jo.has_member( "item" ) ) {
        // items mode
        if( check_duplicate_item( itype_id( jo.get_string( "item" ) ) ) ) {
            jo.throw_error( "Duplicate definition of item" );
        }

        for( const JsonValue sub : jo.get_array( "sub" ) ) {
            substitution s;
            JsonObject obj = sub.get_object();
            s.trait_reqs = trait_requirements( obj );
            for( const JsonValue info : obj.get_array( "new" ) ) {
                s.infos.emplace_back( info );
            }
            substitutions[itype_id( jo.get_string( "item" ) )].push_back( s );
        }
    } else if( jo.has_member( "trait" ) ) {
        // traits mode
        for( const JsonObject sub : jo.get_array( "sub" ) ) {
            substitution s;
            itype_id old_it;
            sub.read( "item", old_it, true );
            if( check_duplicate_item( old_it ) ) {
                sub.throw_error( "Duplicate definition of item" );
            }
            s.trait_reqs.present.emplace_back( jo.get_string( "trait" ) );
            for( const JsonValue info : sub.get_array( "new" ) ) {
                s.infos.emplace_back( info );
            }
            substitutions[old_it].push_back( s );
        }
    } else if( jo.has_member( "bonus" ) ) {
        // bonuses mode
        const item_group_id &bonus_items = item_group::load_item_group( jo.get_member( "group" ),
                                           "collection", "bonus items" );
        bonuses.emplace_back( bonus_items, trait_requirements( jo.get_object( "bonus" ) ) );
    }
}

void json_item_substitution::check_consistency()
{
    auto check_if_trait = []( const trait_id & t ) {
        if( !t.is_valid() ) {
            debugmsg( "%s is not a trait", t.c_str() );
        }
    };
    auto check_if_itype = []( const itype_id & i ) {
        if( !item::type_is_defined( i ) ) {
            debugmsg( "%s is not an itype_id", i.c_str() );
        }
    };
    auto check_if_igroup = []( const item_group_id & gr ) {
        if( !item_group::group_is_defined( gr ) ) {
            debugmsg( "%s is not an item_group_id", gr.c_str() );
        }
    };
    auto check_trait_reqs = [&check_if_trait]( const trait_requirements & tr ) {
        for( const trait_id &str : tr.present ) {
            check_if_trait( str );
        }
        for( const trait_id &str : tr.absent ) {
            check_if_trait( str );
        }
    };

    for( const auto &pair : substitutions ) {
        check_if_itype( pair.first );
        for( const substitution &s : pair.second ) {
            check_trait_reqs( s.trait_reqs );
            for( const substitution::info &inf : s.infos ) {
                check_if_itype( inf.new_item );
            }
        }
    }
    for( const auto &pair : bonuses ) {
        check_if_igroup( pair.first );
        check_trait_reqs( pair.second );
    }
}

bool json_item_substitution::trait_requirements::meets_condition( const std::vector<trait_id>
        &traits ) const
{
    const auto pred = [&traits]( const trait_id & s ) {
        return std::find( traits.begin(), traits.end(), s ) != traits.end();
    };
    return std::all_of( present.begin(), present.end(), pred ) &&
           std::none_of( absent.begin(), absent.end(), pred );
}

std::vector<item> json_item_substitution::get_substitution( const item &it,
        const std::vector<trait_id> &traits ) const
{
    auto iter = substitutions.find( it.typeId() );
    std::vector<item> ret;
    if( iter == substitutions.end() ) {
        for( const item *con : it.all_items_top() ) {
            const auto sub = get_substitution( *con, traits );
            ret.insert( ret.end(), sub.begin(), sub.end() );
        }
        return ret;
    }

    const auto sub = std::find_if( iter->second.begin(),
    iter->second.end(), [&traits]( const substitution & s ) {
        return s.trait_reqs.meets_condition( traits );
    } );
    if( sub == iter->second.end() ) {
        return ret;
    }

    const int old_amt = it.count();
    for( const substitution::info &inf : sub->infos ) {
        item result( inf.new_item, advanced_spawn_time() );
        int new_amount = std::max( 1, static_cast<int>( std::round( inf.ratio * old_amt ) ) );

        if( !result.count_by_charges() ) {
            for( int i = 0; i < new_amount; i++ ) {
                ret.push_back( result.in_its_container() );
            }
        } else {
            while( new_amount > 0 ) {
                const item pushed = result.in_its_container( new_amount );
                new_amount -= pushed.charges_of( inf.new_item );
                ret.push_back( pushed );
            }
        }
    }
    return ret;
}

std::vector<item> json_item_substitution::get_bonus_items( const std::vector<trait_id> &traits )
const
{
    std::vector<item> ret;
    for( const auto &pair : bonuses ) {
        if( pair.second.meets_condition( traits ) ) {
            const std::vector<item> &items = item_group::items_from( pair.first, advanced_spawn_time() );
            // ret = ret + items
            ret.reserve( ret.size() + items.size() );
            ret.insert( ret.end(), items.begin(), items.end() );
        }
    }
    return ret;
}

bool profession::is_hobby() const
{
    return _subtype == "hobby";
}

const std::vector<mission_type_id> &profession::missions() const
{
    return _missions;
}

std::optional<achievement_id> profession::get_requirement() const
{
    return _requirement;
}
