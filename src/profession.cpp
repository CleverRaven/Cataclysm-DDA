#include "profession.h"

#include <cmath>
#include <iterator>
#include <map>
#include <algorithm>
#include <memory>

#include "addiction.h"
#include "avatar.h"
#include "debug.h"
#include "generic_factory.h"
#include "item_group.h"
#include "itype.h"
#include "json.h"
#include "mtype.h"
#include "player.h"
#include "pldata.h"
#include "text_snippets.h"
#include "translations.h"
#include "calendar.h"
#include "item.h"
#include "flat_set.h"
#include "type_id.h"

namespace
{
generic_factory<profession> all_profs( "profession", "ident" );
const string_id<profession> generic_profession_id( "unemployed" );
} // namespace

static class json_item_substitution
{
    public:
        void reset();
        void load( JsonObject &jo );
        void check_consistency();

    private:
        void do_load( JsonObject &jo );

        struct trait_requirements {
            static trait_requirements load( JsonArray &arr );
            std::vector<trait_id> present, absent;
            bool meets_condition( const std::vector<trait_id> &traits ) const;
        };
        struct substitution {
            trait_requirements trait_reqs;
            struct info {
                static info load( JsonArray &arr );
                itype_id new_item;
                double ratio = 1.0; // new charges / old charges
            };
            std::vector<info> infos;
        };
        std::map<itype_id, std::vector<substitution>> substitutions;
        std::vector<std::pair<itype_id, trait_requirements>> bonuses;
    public:
        std::vector<itype_id> get_bonus_items( const std::vector<trait_id> &traits ) const;
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
      _description_female( no_translation( "null" ) ),
      _point_cost( 0 )
{
}

void profession::load_profession( JsonObject &jo, const std::string &src )
{
    all_profs.load( jo, src );
}

class skilllevel_reader : public generic_typed_reader<skilllevel_reader>
{
    public:
        std::pair<skill_id, int> get_next( JsonIn &jin ) const {
            JsonObject jo = jin.get_object();
            return std::pair<skill_id, int>( skill_id( jo.get_string( "name" ) ), jo.get_int( "level" ) );
        }
        template<typename C>
        void erase_next( JsonIn &jin, C &container ) const {
            const skill_id id = skill_id( jin.get_string() );
            reader_detail::handler<C>().erase_if( container, [&id]( const std::pair<skill_id, int> &e ) {
                return e.first == id;
            } );
        }
};

class addiction_reader : public generic_typed_reader<addiction_reader>
{
    public:
        addiction get_next( JsonIn &jin ) const {
            JsonObject jo = jin.get_object();
            return addiction( addiction_type( jo.get_string( "type" ) ), jo.get_int( "intensity" ) );
        }
        template<typename C>
        void erase_next( JsonIn &jin, C &container ) const {
            const add_type type = addiction_type( jin.get_string() );
            reader_detail::handler<C>().erase_if( container, [&type]( const addiction & e ) {
                return e.type == type;
            } );
        }
};

class item_reader : public generic_typed_reader<item_reader>
{
    public:
        profession::itypedec get_next( JsonIn &jin ) const {
            // either a plain item type id string, or an array with item type id
            // and as second entry the item description.
            if( jin.test_string() ) {
                return profession::itypedec( jin.get_string(), "" );
            }
            JsonArray jarr = jin.get_array();
            const auto id = jarr.get_string( 0 );
            const auto s = jarr.get_string( 1 );
            const auto snippet = _( s );
            return profession::itypedec( id, snippet );
        }
        template<typename C>
        void erase_next( JsonIn &jin, C &container ) const {
            const std::string id = jin.get_string();
            reader_detail::handler<C>().erase_if( container, [&id]( const profession::itypedec & e ) {
                return e.type_id == id;
            } );
        }
};

void profession::load( JsonObject &jo, const std::string & )
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
        const std::string desc = jo.get_string( "description" );
        // These also may differ depending on the language settings!
        _description_male = to_translation( "prof_desc_male", desc );
        _description_female = to_translation( "prof_desc_female", desc );
    }
    if( jo.has_array( "pets" ) ) {
        JsonArray array = jo.get_array( "pets" );
        while( array.has_more() ) {
            JsonObject subobj = array.next_object();
            int count = subobj.get_int( "amount" );
            mtype_id mon = mtype_id( subobj.get_string( "name" ) );
            for( int start = 0; start < count; ++start ) {
                _starting_pets.push_back( mon );
            }
        }
    }

    if( jo.has_array( "spells" ) ) {
        JsonArray array = jo.get_array( "spells" );
        while( array.has_more() ) {
            JsonObject subobj = array.next_object();
            int level = subobj.get_int( "level" );
            spell_id sp = spell_id( subobj.get_string( "id" ) );
            _starting_spells.emplace( sp, level );
        }
    }

    mandatory( jo, was_loaded, "points", _point_cost );

    if( !was_loaded || jo.has_member( "items" ) ) {
        JsonObject items_obj = jo.get_object( "items" );

        if( items_obj.has_array( "both" ) ) {
            optional( items_obj, was_loaded, "both", legacy_starting_items, item_reader {} );
        }
        if( items_obj.has_object( "both" ) ) {
            _starting_items = item_group::load_item_group( *items_obj.get_raw( "both" ), "collection" );
        }
        if( items_obj.has_array( "male" ) ) {
            optional( items_obj, was_loaded, "male", legacy_starting_items_male, item_reader {} );
        }
        if( items_obj.has_object( "male" ) ) {
            _starting_items_male = item_group::load_item_group( *items_obj.get_raw( "male" ), "collection" );
        }
        if( items_obj.has_array( "female" ) ) {
            optional( items_obj, was_loaded, "female",  legacy_starting_items_female, item_reader {} );
        }
        if( items_obj.has_object( "female" ) ) {
            _starting_items_female = item_group::load_item_group( *items_obj.get_raw( "female" ),
                                     "collection" );
        }
    }
    optional( jo, was_loaded, "no_bonus", no_bonus );

    optional( jo, was_loaded, "skills", _starting_skills, skilllevel_reader {} );
    optional( jo, was_loaded, "addictions", _starting_addictions, addiction_reader {} );
    // TODO: use string_id<bionic_type> or so
    optional( jo, was_loaded, "CBMs", _starting_CBMs, auto_flags_reader<bionic_id> {} );
    // TODO: use string_id<mutation_branch> or so
    optional( jo, was_loaded, "traits", _starting_traits, auto_flags_reader<trait_id> {} );
    optional( jo, was_loaded, "flags", flags, auto_flags_reader<> {} );
}

const profession *profession::generic()
{
    return &generic_profession_id.obj();
}

const std::vector<profession> &profession::get_all()
{
    return all_profs.get_all();
}

void profession::reset()
{
    all_profs.reset();
    item_substitutions.reset();
}

void profession::check_definitions()
{
    item_substitutions.check_consistency();
    for( const auto &prof : all_profs.get_all() ) {
        prof.check_definition();
    }
}

void profession::check_item_definitions( const itypedecvec &items ) const
{
    for( auto &itd : items ) {
        if( !item::type_is_defined( itd.type_id ) ) {
            debugmsg( "profession %s: item %s does not exist", id.c_str(), itd.type_id.c_str() );
        } else if( !itd.snippet_id.empty() ) {
            const itype *type = item::find_type( itd.type_id );
            if( type->snippet_category.empty() ) {
                debugmsg( "profession %s: item %s has no snippet category - no description can be set",
                          id.c_str(), itd.type_id.c_str() );
            } else {
                const int hash = SNIPPET.get_snippet_by_id( itd.snippet_id );
                if( SNIPPET.get( hash ).empty() ) {
                    debugmsg( "profession %s: snippet id %s for item %s is not contained in snippet category %s",
                              id.c_str(), itd.snippet_id.c_str(), itd.type_id.c_str(), type->snippet_category.c_str() );
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
    if( !no_bonus.empty() && !item::type_is_defined( no_bonus ) ) {
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

    for( const auto &a : _starting_CBMs ) {
        if( !a.is_valid() ) {
            debugmsg( "bionic %s for profession %s does not exist", a.c_str(), id.c_str() );
        }
    }

    for( auto &t : _starting_traits ) {
        if( !t.is_valid() ) {
            debugmsg( "trait %s for profession %s does not exist", t.c_str(), id.c_str() );
        }
    }
    for( const auto &elem : _starting_pets ) {
        if( !elem.is_valid() ) {
            debugmsg( "startng pet %s for profession %s does not exist", elem.c_str(), id.c_str() );
        }
    }
    for( const auto &elem : _starting_skills ) {
        if( !elem.first.is_valid() ) {
            debugmsg( "skill %s for profession %s does not exist", elem.first.c_str(), id.c_str() );
        }
    }
}

bool profession::has_initialized()
{
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

signed int profession::point_cost() const
{
    return _point_cost;
}

std::list<item> profession::items( bool male, const std::vector<trait_id> &traits ) const
{
    std::list<item> result;
    auto add_legacy_items = [&result]( const itypedecvec & vec ) {
        for( const itypedec &elem : vec ) {
            item it( elem.type_id, 0, item::default_charges_tag {} );
            if( !elem.snippet_id.empty() ) {
                it.set_snippet( elem.snippet_id );
            }
            it = it.in_its_container();
            result.push_back( it );
        }
    };

    add_legacy_items( legacy_starting_items );
    add_legacy_items( male ? legacy_starting_items_male : legacy_starting_items_female );

    const std::vector<item> group_both = item_group::items_from( _starting_items );
    const std::vector<item> group_gender = item_group::items_from( male ? _starting_items_male :
                                           _starting_items_female );
    result.insert( result.begin(), group_both.begin(), group_both.end() );
    result.insert( result.begin(), group_gender.begin(), group_gender.end() );

    std::vector<itype_id> bonus = item_substitutions.get_bonus_items( traits );
    for( const itype_id &elem : bonus ) {
        if( elem != no_bonus ) {
            result.push_back( item( elem, 0, item::default_charges_tag {} ) );
        }
    }
    for( auto iter = result.begin(); iter != result.end(); ) {
        const auto sub = item_substitutions.get_substitution( *iter, traits );
        if( !sub.empty() ) {
            result.insert( result.begin(), sub.begin(), sub.end() );
            iter = result.erase( iter );
        } else {
            ++iter;
        }
    }
    for( item &it : result ) {
        if( it.has_flag( "VARSIZE" ) ) {
            it.item_tags.insert( "FIT" );
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

std::vector<trait_id> profession::get_locked_traits() const
{
    return _starting_traits;
}

profession::StartingSkillList profession::skills() const
{
    return _starting_skills;
}

bool profession::has_flag( const std::string &flag ) const
{
    return flags.count( flag ) != 0;
}

bool profession::can_pick( const player &u, const int points ) const
{
    return point_cost() - u.prof->point_cost() <= points;
}

bool profession::is_locked_trait( const trait_id &trait ) const
{
    return std::find( _starting_traits.begin(), _starting_traits.end(), trait ) !=
           _starting_traits.end();
}

std::map<spell_id, int> profession::spells() const
{
    return _starting_spells;
}

void profession::learn_spells( avatar &you ) const
{
    for( const std::pair<spell_id, int> spell_pair : spells() ) {
        you.magic.learn_spell( spell_pair.first, you, true );
        spell &sp = you.magic.get_spell( spell_pair.first );
        while( sp.get_level() < spell_pair.second && !sp.is_max_level() ) {
            sp.gain_level();
        }
    }
}

// item_substitution stuff:

void profession::load_item_substitutions( JsonObject &jo )
{
    item_substitutions.load( jo );
}

void json_item_substitution::reset()
{
    substitutions.clear();
    bonuses.clear();
}

json_item_substitution::substitution::info json_item_substitution::substitution::info::load(
    JsonArray &arr )
{
    json_item_substitution::substitution::info ret;
    ret.new_item = arr.next_string();
    if( arr.test_float() && ( ret.ratio = arr.next_float() ) <= 0.0 ) {
        arr.throw_error( "Ratio must be positive" );
    }
    return ret;
}

json_item_substitution::trait_requirements json_item_substitution::trait_requirements::load(
    JsonArray &arr )
{
    trait_requirements ret;
    arr.read_next( ret.present );
    if( arr.test_array() ) {
        arr.read_next( ret.absent );
    }
    return ret;
}

void json_item_substitution::load( JsonObject &jo )
{
    if( !jo.has_array( "substitutions" ) ) {
        jo.throw_error( "No `substitutions` array found." );
    }
    JsonArray outer_arr = jo.get_array( "substitutions" );
    while( outer_arr.has_more() ) {
        JsonObject subobj = outer_arr.next_object();
        do_load( subobj );
    }
}

void json_item_substitution::do_load( JsonObject &jo )
{
    const bool item_mode = jo.has_string( "item" );
    const std::string title = jo.get_string( item_mode ? "item" : "trait" );

    auto check_duplicate_item = [&]( const itype_id & it ) {
        return substitutions.find( it ) != substitutions.end() ||
               std::find_if( bonuses.begin(), bonuses.end(),
        [&it]( const std::pair<itype_id, trait_requirements> &p ) {
            return p.first == it;
        } ) != bonuses.end();
    };
    if( item_mode && check_duplicate_item( title ) ) {
        jo.throw_error( "Duplicate definition of item" );
    }

    if( jo.has_array( "bonus" ) ) {
        if( !item_mode ) {
            jo.throw_error( "Bonuses can only be used in item mode" );
        }
        JsonArray arr = jo.get_array( "bonus" );
        bonuses.emplace_back( title, trait_requirements::load( arr ) );
    } else if( !jo.has_array( "sub" ) ) {
        jo.throw_error( "Missing sub array" );
    }

    JsonArray sub = jo.get_array( "sub" );
    while( sub.has_more() ) {
        JsonArray line = sub.next_array();
        substitution s;
        const itype_id old_it = item_mode ? title : line.next_string();
        if( item_mode ) {
            s.trait_reqs = trait_requirements::load( line );
        } else {
            if( check_duplicate_item( old_it ) ) {
                line.throw_error( "Duplicate definition of item" );
            }
            s.trait_reqs.present.push_back( trait_id( title ) );
        }
        // Error if the array doesn't have at least one new_item
        do {
            s.infos.push_back( substitution::info::load( line ) );
        } while( line.has_more() );
        substitutions[old_it].push_back( s );
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
        check_if_itype( pair.first );
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
        for( const item &con : it.contents ) {
            const auto sub = get_substitution( con, traits );
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
        item result( inf.new_item );
        const int new_amt = std::max( 1, static_cast<int>( std::round( inf.ratio * old_amt ) ) );

        if( !result.count_by_charges() ) {
            for( int i = 0; i < new_amt; i++ ) {
                ret.push_back( result.in_its_container() );
            }
        } else {
            result.mod_charges( -result.charges + new_amt );
            while( result.charges > 0 ) {
                const item pushed = result.in_its_container();
                ret.push_back( pushed );
                result.mod_charges( pushed.contents.empty() ? -pushed.charges : -pushed.contents.back().charges );
            }
        }
    }
    return ret;
}

std::vector<itype_id> json_item_substitution::get_bonus_items( const std::vector<trait_id>
        &traits ) const
{
    std::vector<itype_id> ret;
    for( const auto &pair : bonuses ) {
        if( pair.second.meets_condition( traits ) ) {
            ret.push_back( pair.first );
        }
    }
    return ret;
}
