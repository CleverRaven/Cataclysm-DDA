#include "character.h"

#include "itype.h"
#include "recipe_dictionary.h"
#include "recipe.h"
#include "skill.h"
#include "player.h"
#include "npc.h"

int Character::has_recipe( const recipe *r, const inventory &crafting_inv,
                           const std::vector<npc *> &helpers ) const
{
    if( !r->skill_used ) {
        return 0;
    }

    if( knows_recipe( r ) ) {
        return r->difficulty;
    }

    const auto available = get_available_recipes( crafting_inv, &helpers );
    return available.contains( r ) ? available.get_custom_difficulty( r ) : -1;
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
    for( const auto &elem : book.book->recipes ) {
        if( !knows_recipe( elem.recipe ) ) {
            return false;
        }
    }
    return true;
}
const recipe_subset &Character::get_learned_recipes() const
{
    // Cache validity check
    if( *_skills != *valid_autolearn_skills ) {
        for( const auto &r : recipe_dict.all_autolearn() ) {
            if( meets_skill_requirements( r->autolearn_requirements ) ) {
                learned_recipes->include( r );
            }
        }
        *valid_autolearn_skills = *_skills; // Reassign the validity stamp
    }

    return *learned_recipes;
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

recipe_subset Character::get_available_recipes( const inventory &crafting_inv,
        const std::vector<npc *> *helpers ) const
{
    recipe_subset res( get_learned_recipes() );

    res.include( get_recipes_from_books( crafting_inv ) );

    if( helpers != nullptr ) {
        for( npc *np : *helpers ) {
            // Directly form the helper's inventory
            res.include( get_recipes_from_books( np->inv ) );
            // Being told what to do
            res.include_if( np->get_learned_recipes(), [ this ]( const recipe & r ) {
                return get_skill_level( r.skill_used ) >= static_cast<int>( r.difficulty *
                        0.8f ); // Skilled enough to understand
            } );
        }
    }

    return res;
}

std::set<itype_id> Character::get_books_for_recipe( const inventory &crafting_inv,
        const recipe *r ) const
{
    std::set<itype_id> book_ids;
    const int skill_level = get_skill_level( r->skill_used );
    for( const auto &book_lvl : r->booksets ) {
        itype_id book_id = book_lvl.first;
        int required_skill_level = book_lvl.second;
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
    std::vector<npc *> helpers = get_crafting_helpers();
    return std::min( max, static_cast<int>( helpers.size() ) );
}
