#include <iostream>
#include <sstream>
#include <iterator>

#include "profession.h"

#include "debug.h"
#include "json.h"
#include "player.h"
#include "bionics.h"
#include "mutation.h"
#include "text_snippets.h"
#include "rng.h"
#include "translations.h"
#include "skill.h"
#include "addiction.h"
#include "pldata.h"
#include "itype.h"
#include "generic_factory.h"

namespace
{
generic_factory<profession> all_profs( "profession", "ident" );
const string_id<profession> generic_profession_id( "unemployed" );
}

/**
 * This class is used to replace certain items with other items when the player has
 * a certain combination of traits. There is only one instance of this class, and it
 * is only accessible by the profession module.
*/
static class json_item_substitution {
    public:
        void reset();
        void load( JsonObject &jo );
        void check_consistency();

    private:
        struct substitution {
            std::vector<std::string> traits_present; // If the player has all of these traits
            std::vector<std::string> traits_absent; // And they don't have any of these traits
            itype_id former; // Then replace any starting items with this itype
            int count; // with this amount of items with
            itype_id latter; // this itype
        };
        // Note: If former.empty(), then latter is a bonus item
        std::vector<substitution> substitutions;
        bool meets_trait_conditions( const substitution &sub,
                                     const std::vector<std::string> &traits ) const;
    public:
        std::vector<itype_id> get_bonus_items( const std::vector<std::string> &traits ) const;
        std::vector<item> get_substitution( const item &it, const std::vector<std::string> &traits ) const;
} item_substitutions;

template<>
const profession &string_id<profession>::obj() const
{
    return all_profs.obj( *this );
}

template<>
bool string_id<profession>::is_valid() const
{
    return all_profs.is_valid( *this );
}

profession::profession()
    : id(), _name_male( "null" ), _name_female( "null" ),
      _description_male( "null" ), _description_female( "null" ), _point_cost( 0 )
{
}

void profession::load_profession( JsonObject &jo, const std::string &src )
{
    if( jo.has_array( "substitutions" ) ) {
        item_substitutions.load( jo ); // This isn't actually a profession
    } else {
        all_profs.load( jo, src );
    }
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
            const auto snippet = _( s.c_str() );
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
        _name_male = pgettext( "profession_male", name_obj.get_string( "male" ).c_str() );
        _name_female = pgettext( "profession_female", name_obj.get_string( "female" ).c_str() );
    } else if( jo.has_string( "name" ) ) {
        // Same profession names for male and female in English.
        // Still need to different names in other languages.
        const std::string name = jo.get_string( "name" );
        _name_female = pgettext( "profession_female", name.c_str() );
        _name_male = pgettext( "profession_male", name.c_str() );
    } else if( !was_loaded ) {
        jo.throw_error( "missing mandatory member \"name\"" );
    }

    if( !was_loaded || jo.has_member( "description" ) ) {
        const std::string desc = jo.get_string( "description" );
        _description_male = pgettext( "prof_desc_male", desc.c_str() );
        _description_female = pgettext( "prof_desc_female", desc.c_str() );
    }

    mandatory( jo, was_loaded, "points", _point_cost );

    if( !was_loaded || jo.has_member( "items" ) ) {
        JsonObject items_obj = jo.get_object( "items" );

        if( items_obj.has_array( "both" ) ) {
            optional( items_obj, was_loaded, "both", legacy_starting_items, item_reader{} );
        }
        if( items_obj.has_object( "both" ) ) {
            _starting_items = item_group::load_item_group( *items_obj.get_raw( "both" ), "collection" );
        }
        if( items_obj.has_array( "male" ) ) {
            optional( items_obj, was_loaded, "male", legacy_starting_items_male, item_reader{} );
        }
        if( items_obj.has_object( "male" ) ) {
            _starting_items_male = item_group::load_item_group( *items_obj.get_raw( "male" ), "collection" );
        }
        if( items_obj.has_array( "female" ) ) {
            optional( items_obj, was_loaded, "female",  legacy_starting_items_female, item_reader{} );
        }
        if( items_obj.has_object( "female" ) ) {
            _starting_items_female = item_group::load_item_group( *items_obj.get_raw( "female" ),
                                     "collection" );
        }
    }

    optional( jo, was_loaded, "skills", _starting_skills, skilllevel_reader{} );
    optional( jo, was_loaded, "addictions", _starting_addictions, addiction_reader{} );
    // TODO: use string_id<bionic_type> or so
    optional( jo, was_loaded, "CBMs", _starting_CBMs, auto_flags_reader<> {} );
    // TODO: use string_id<mutation_branch> or so
    optional( jo, was_loaded, "traits", _starting_traits, auto_flags_reader<> {} );
    optional( jo, was_loaded, "flags", flags, auto_flags_reader<> {} );
}

const profession *profession::generic()
{
    return &generic_profession_id.obj();
}

// Strategy: a third of the time, return the generic profession.  Otherwise, return a profession,
// weighting 0 cost professions more likely--the weight of a profession with cost n is 2/(|n|+2),
// e.g., cost 1 is 2/3rds as likely, cost -2 is 1/2 as likely.
const profession *profession::weighted_random()
{
    if( one_in( 3 ) ) {
        return generic();
    }

    const auto &list = all_profs.get_all();
    while( true ) {
        auto iter = list.begin();
        std::advance( iter, rng( 0, list.size() - 1 ) );
        const profession &prof = *iter;

        if( x_in_y( 2, abs( prof.point_cost() ) + 2 ) && !prof.has_flag( "SCEN_ONLY" ) ) {
            return &prof;
        }
        // else reroll in the while loop.
    }
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
            debugmsg( "profession %s: item %s does not exist", id.c_str() , itd.type_id.c_str() );
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

    if( !item_group::group_is_defined( _starting_items ) ) {
        debugmsg( "_starting_items group is undefined" );
    }
    if( !item_group::group_is_defined( _starting_items_male ) ) {
        debugmsg( "_starting_items_male group is undefined" );
    }
    if( !item_group::group_is_defined( _starting_items_female ) ) {
        debugmsg( "_starting_items_female group is undefined" );
    }

    for( auto const &a : _starting_CBMs ) {
        if( !is_valid_bionic( a ) ) {
            debugmsg( "bionic %s for profession %s does not exist", a.c_str(), id.c_str() );
        }
    }

    for( auto &t : _starting_traits ) {
        if( !mutation_branch::has( t ) ) {
            debugmsg( "trait %s for profession %s does not exist", t.c_str(), id.c_str() );
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
        return _name_male;
    } else {
        return _name_female;
    }
}

std::string profession::description( bool male ) const
{
    if( male ) {
        return _description_male;
    } else {
        return _description_female;
    }
}

signed int profession::point_cost() const
{
    return _point_cost;
}

std::list<item> profession::items( bool male, const std::vector<std::string> &traits ) const
{
    std::list<item> result;
    auto add_legacy_items = [&result]( const itypedecvec & vec ) {
        for( const itypedec &elem : vec ) {
            item it( elem.type_id, 0, item::default_charges_tag{} );
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
        result.push_back( item( elem, 0, item::default_charges_tag{} ) );
    }
    for( auto iter = result.begin(); iter != result.end(); ) {
        std::vector<item> subs = item_substitutions.get_substitution( *iter, traits );
        if( !subs.empty() ) {
            result.insert( result.begin(), subs.begin(), subs.end() );
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
    for( auto outer = result.begin(); std::next( outer ) != result.end(); ++outer ) {
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

    return result;
}

std::vector<addiction> profession::addictions() const
{
    return _starting_addictions;
}

std::vector<std::string> profession::CBMs() const
{
    return _starting_CBMs;
}

std::vector<std::string> profession::traits() const
{
    return _starting_traits;
}

const profession::StartingSkillList profession::skills() const
{
    return _starting_skills;
}

bool profession::has_flag( std::string flag ) const
{
    return flags.count( flag ) != 0;
}

bool profession::can_pick( player *u, int points ) const
{
    if( point_cost() - u->prof->point_cost() > points ) {
        return false;
    }

    return true;
}

bool profession::locked_traits( const std::string &trait ) const
{
    return std::find( _starting_traits.begin(), _starting_traits.end(), trait ) !=
           _starting_traits.end();
}

// item_substitution stuff:

void json_item_substitution::reset()
{
    substitutions.clear();
}

void json_item_substitution::load( JsonObject &jo )
{
    if( !jo.has_array( "substitutions" ) ) {
        jo.throw_error( "No `substitutions` array found." );
    }
    JsonArray outer_arr = jo.get_array( "substitutions" );
    while( outer_arr.has_more() ) {
        JsonObject subobj = outer_arr.next_object();
        substitution pushed;

        pushed.traits_present = subobj.get_string_array( "traits_present" );
        pushed.traits_absent = subobj.get_string_array( "traits_absent" );
        pushed.former = subobj.get_string( "former", "" );
        pushed.count = subobj.get_int( "count", 1 );
        if( pushed.count <= 0 ) {
            subobj.throw_error( "Count must be positive" );
        }
        pushed.latter = subobj.get_string( "latter", "" );

        substitutions.push_back( pushed );
    }
}

void json_item_substitution::check_consistency()
{
    for( const substitution &sub : substitutions ) {
        for( const std::string &tr : sub.traits_present ) {
            if( !mutation_branch::has( tr ) ) {
                debugmsg( "%s is not a trait", tr.c_str() );
            }
        }
        for( const std::string &tr : sub.traits_absent ) {
            if( !mutation_branch::has( tr ) ) {
                debugmsg( "%s is not a trait", tr.c_str() );
            }
        }
        if( sub.latter.empty() ) {
            debugmsg( "There must be a replacement item" );
        } else if( !item::type_is_defined( sub.latter ) ) {
            debugmsg( "%s is not an itype_id", sub.latter.c_str() );
        }
        if( !sub.former.empty() && !item::type_is_defined( sub.former ) ) {
            debugmsg( "%s is not an itype_id", sub.former.c_str() );
        }
    }
}

bool json_item_substitution::meets_trait_conditions( const substitution &sub,
                                                     const std::vector<std::string> &traits ) const
{
    for( const std::string &tr : sub.traits_present ) {
        if( std::find( traits.begin(), traits.end(), tr ) == traits.end() ) {
            return false;
        }
    }
    for( const std::string &tr : sub.traits_absent ) {
        if( std::find( traits.begin(), traits.end(), tr ) != traits.end() ) {
            return false;
        }
    }
    return true;
}

std::vector<item> json_item_substitution::get_substitution( const item &it, const std::vector<std::string> &traits ) const
{
    std::vector<item> ret;
    for( const substitution &sub : substitutions ) {
        if( it.typeId() == sub.former && meets_trait_conditions( sub, traits ) ) {
            item replacer = it;
            // We must use convert in order to keep properties like damage/burnt/etc the same.
            replacer.convert( sub.latter );

            if( replacer.count_by_charges() ) {
                // The resulting charges must reflect the latter itype's stack size, rather
                // than the property of the item that's being replaced.
                replacer.mod_charges( replacer.type->charges_default() - replacer.charges );
            }

            for( int i = 0; i < sub.count; i++ ) {
                ret.push_back( replacer );
            }
            return ret;
        }
    }
    return ret;
}

std::vector<itype_id> json_item_substitution::get_bonus_items( const std::vector<std::string> &traits ) const
{
    std::vector<itype_id> ret;
    for( const substitution &sub : substitutions ) {
        if( sub.former.empty() && meets_trait_conditions( sub, traits ) ) {
            for( int i = 0; i < sub.count; i++ ) {
                ret.push_back( sub.latter );
            }
        }
    }
    return ret;
}

