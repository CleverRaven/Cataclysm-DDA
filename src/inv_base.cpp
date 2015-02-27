#include "inv_base.h"


void inv_base::restack(const player *p)
{
    // tasks that the old restack seemed to do:
    // 1. reassign inventory letters
    // 2. remove items from non-matching stacks
    // 3. combine matching stacks

    std::list<item> to_restack;

    if (p) {
        int idx = 0;
        for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter, ++idx) {
            const int ipos = p->invlet_to_position(iter->front().invlet);
            if (!iter->front().invlet_is_okay() || ( ipos != INT_MIN && ipos != idx ) ) {
                assign_empty_invlet(iter->front());
                for( std::list<item>::iterator stack_iter = iter->begin();
                     stack_iter != iter->end(); ++stack_iter ) {
                    stack_iter->invlet = iter->front().invlet;
                }
            }

            // remove non-matching items, stripping off end of stack so the first item keeps the invlet.
            while( iter->size() > 1 && !iter->front().stacks_with(iter->back()) ) {
                to_restack.splice(to_restack.begin(), *iter, --iter->end());
            }
        }
    }

    // combine matching stacks
    // separate loop to ensure that ALL stacks are homogeneous
    for (invstack::iterator iter = items.begin(); iter != items.end(); ++iter) {
        for (invstack::iterator other = iter; other != items.end(); ++other) {
            if (iter != other && iter->front().stacks_with( other->front() ) ) {
                if( other->front().count_by_charges() ) {
                    iter->front().charges += other->front().charges;
                } else {
                    iter->splice(iter->begin(), *other);
                }
                other = items.erase(other);
                --other;
            }
        }
    }

    //re-add non-matching items
    for( auto &elem : to_restack ) {
        add_item( elem );
    }
}

