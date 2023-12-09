#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "character.h"
#include "inventory.h"
#include "item.h"
#include "itype.h"
#include "pimpl.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "skill.h"
#include "type_id.h"
#include "value_ptr.h"

bool Character::has_recipe( const recipe *r, const inventory &crafting_inv,
                            const std::vector<Character *> &helpers ) const
{
    return knows_recipe( r ) || get_available_recipes( crafting_inv, &helpers ).contains( r );
}

bool Character::knows_recipe( const recipe *rec ) const
{
    return get_learned_recipes().contains( rec );
}

void Character::learn_recipe( const recipe *const rec )
{
    if( rec->never_learn ) {
        return;
    }
    learned_recipes->include( rec );
}

void Character::forget_recipe( const recipe *const rec )
{
    learned_recipes->remove( rec );
}

void Character::forget_all_recipes()
{
    learned_recipes->clear();
}

int Character::exceeds_recipe_requirements( const recipe &rec ) const
{
    return get_all_skills().exceeds_recipe_requirements( rec );
}

bool Character::has_recipe_requirements( const recipe &rec ) const
{
    return get_all_skills().has_recipe_requirements( rec );
}

bool Character::can_decomp_learn( const recipe &rec ) const
{
    return !rec.learn_by_disassembly.empty() &&
           meets_skill_requirements( rec.learn_by_disassembly );
}

bool Character::studied_all_recipes( const itype &book ) const
{
    if( !book.book ) {
        return true;
    }
    for( const islot_book::recipe_with_description_t &elem : book.book->recipes ) {
        if( !knows_recipe( elem.recipe ) ) {
            return false;
        }
    }
    return true;
}
const recipe_subset &Character::get_learned_recipes() const
{
    // Cache validity check
    if( !_skills->has_same_levels_as( *valid_autolearn_skills ) ) {
        for( const recipe * const &r : recipe_dict.all_autolearn() ) {
            // skip nested recipes they will be covered in get_available_nested
            if( meets_skill_requirements( r->autolearn_requirements ) && !r->is_nested() ) {
                learned_recipes->include( r );
            }
        }
        *valid_autolearn_skills = *_skills; // Reassign the validity stamp
    }

    return *learned_recipes;
}

static bool check_nested_has_recipes( const recipe *r, const recipe_subset &res )
{
    for( const recipe_id &nestedr : r->nested_category_data ) {
        if( nestedr->is_nested() ) {
            // recursively check for a category that has stuff in it
            if( check_nested_has_recipes( &nestedr.obj(), res ) ) {
                return true;
            }
        } else if( res.contains( &nestedr.obj() ) ) {
            return true;
        }
    }

    return false;
}

recipe_subset Character::get_available_nested( const recipe_subset &res ) const
{
    recipe_subset nested_recipes;
    for( const recipe * const &r : recipe_dict.all_nested() ) {
        // only display a nested category if you know at least one recipe within it
        if( check_nested_has_recipes( r, res ) ) {
            nested_recipes.include( r );
        }
    }

    return nested_recipes;
}

recipe_subset Character::get_recipes_from_books( const inventory &crafting_inv ) const
{
    recipe_subset res;

    for( const auto &stack : crafting_inv.const_slice() ) {
        const item &candidate = stack->front();

        for( std::pair<const recipe *, int> recipe_entry :
             candidate.get_available_recipes( *this ) ) {
            res.include( recipe_entry.first, recipe_entry.second );
        }
    }

    return res;
}

recipe_subset Character::get_recipes_from_ebooks( const inventory &crafting_inv ) const
{
    recipe_subset res;

    for( const std::list<item> *&stack : crafting_inv.const_slice() ) {
        const item &ereader = stack->front();
        if( !ereader.is_ebook_storage() || !ereader.ammo_sufficient( this ) ||
            ereader.is_broken_on_active() ) {
            continue;
        }

        for( const item *it : ereader.get_contents().ebooks() ) {
            for( std::pair<const recipe *, int> recipe_entry :
                 it->get_available_recipes( *this ) ) {
                res.include( recipe_entry.first, recipe_entry.second );
            }
        }
    }

    return res;
}

recipe_subset Character::get_available_recipes( const inventory &crafting_inv,
        const std::vector<Character *> *helpers ) const
{
    recipe_subset res( get_learned_recipes() );
    res.include( get_recipes_from_books( crafting_inv ) );
    res.include( get_recipes_from_ebooks( crafting_inv ) );

    if( helpers != nullptr ) {
        for( Character *guy : *helpers ) {
            // Directly form the helper's inventory
            res.include( get_recipes_from_books( *guy->inv ) );
            // Being told what to do
            res.include_if( guy->get_learned_recipes(), [ this ]( const recipe & r ) {
                return get_knowledge_level( r.skill_used ) >= static_cast<int>( r.get_difficulty(
                            *this ) * 0.8f ); // Skilled enough to understand
            } );
        }
    }

    res.include( get_available_nested( res ) );

    return res;
}

std::set<itype_id> Character::get_books_for_recipe( const inventory &crafting_inv,
        const recipe *r ) const
{
    std::set<itype_id> book_ids;
    const int skill_level = get_knowledge_level( r->skill_used );
    for( const auto &book_lvl : r->booksets ) {
        itype_id book_id = book_lvl.first;
        int required_skill_level = book_lvl.second.skill_req;
        // NPCs don't need to identify books
        if( !has_identified( book_id ) ) {
            continue;
        }

        if( skill_level >= required_skill_level && crafting_inv.amount_of( book_id ) > 0 ) {
            book_ids.insert( book_id );
        }
    }
    return book_ids;
}

int Character::get_num_crafting_helpers( int max ) const
{
    std::vector<Character *> helpers = get_crafting_helpers();
    return std::min( max, static_cast<int>( helpers.size() ) );
}
