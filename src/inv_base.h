#ifndef  INV_BASE_INC
#define  INV_BASE_INC

#include "item.h"
#include "enums.h"

#include <list>
#include <string>
#include <utility>
#include <vector>

// TODO: come up with a better name?
class inv_base
{
        // save on the clickity clacks
        typedef std::list<item>::iterator                       item_iterator;
        typedef std::list<item>::const_iterator                 item_const_iterator;
        typedef std::list<item *>::iterator                     item_pointer_iterator;
        typedef std::list<item *>::const_iterator               item_const_pointer_iterator;
        // setup is like inventory in a way
        typedef std::list<std::list<item>>                      invstack;
        typedef std::vector<std::list<item>*>                   invslice;
        typedef std::vector<std::pair<std::list<item>*, int>>   indexed_invslice;

    private:

    protected:
        invstack items;

    public:

};

#endif   /* ----- #ifndef INV_BASE_INC  ----- */
