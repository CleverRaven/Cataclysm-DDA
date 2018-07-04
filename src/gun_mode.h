#pragma once
#ifndef GUN_MODE_H
#define GUN_MODE_H

#include <string>
#include <set>

class item;

class gun_mode
{
    private:
        std::string name_;

    public:
        /** pointer to item providing this mode - base gun or attached gunmod */
        item *target = nullptr;
        /** burst size for is_gun() firearms, or melee range for is_melee() weapons */
        int qty = 0;
        /** flags change behavior of gun mode, e.g. MELEE for bayonets that make a reach attack instead of firing - these are **not** equivalent to item flags! */
        std::set<std::string> flags;

        gun_mode() = default;
        gun_mode( const std::string &n, item *target, int qty, const std::set<std::string> &flags ) :
            name_( n ),
            target( target ),
            qty( qty ),
            flags( flags ) {}

        /** if true perform a melee attach as opposed to shooting */
        bool melee() const {
            return flags.count( "MELEE" );
        }

        operator bool() const {
            return target != nullptr;
        }

        item &operator*() {
            return *target;
        }
        const item &operator*() const {
            return *target;
        }

        item *operator->() {
            return target;
        }
        const item *operator->() const {
            return target;
        }

        std::string name() const {
            return name_;
        }
};

#endif
