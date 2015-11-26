#ifndef ITEM_STACK_H
#define ITEM_STACK_H

#include <list>

class item;

// A wrapper class to bundle up the references needed for a caller to safely manipulate
// items at a particular map x/y location.
// Note this does not expose the container itself,
// which means you cannot call e.g. vector::erase() directly.

// Pure virtual base class for a collection of items with origin information.
// Only a subset of the functionality is callable without casting to the specific
// subclass, e.g. not begin()/end() or range loops.
class item_stack
{
    public:
        virtual size_t size() const = 0;
        virtual bool empty() const = 0;
        virtual std::list<item>::iterator erase( std::list<item>::iterator it ) = 0;
        virtual void push_back( const item &newitem ) = 0;
        virtual void insert_at( std::list<item>::iterator, const item &newitem ) = 0;
        virtual item &front() = 0;
        virtual item &operator[]( size_t index ) = 0;
};

#endif
