#ifndef RANGED_H
#define RANGED_H

#include "item_location.h"

class ranged
{
    public:
        ranged() = default;
        ranged( const ranged & ) = delete;
        ranged &operator=( const ranged & ) = delete;
        ranged( ranged && ) = default;
        ranged &operator=( ranged && ) = default;
        virtual ~ranged() = default;

        ranged( item_location &&loc ) : loc( std::move( loc ) ) {}

        /** Is this a valid instance? */
        explicit operator bool() const {
            return loc;
        }

        virtual std::string name() const {
            return loc ? loc->type_name( 1 ) : std::string();
        }

        /** Get base item location */
        virtual item_location &base() {
            return loc;
        }

        /** Quantity of ammunition available for use */
        virtual long ammo_remaining() const {
            return loc ? loc->ammo_remaining() : 0;
        }

        /** Maximum quantity of ammunition */
        virtual long ammo_capacity() const {
            return loc ? loc->ammo_capacity() : 0;
        }

        /** Quantity of ammunition consumed per shot  */
        virtual long ammo_required() const {
            return loc ? loc->ammo_required() : 0;
        }

        /** Are all the requirements met for @ref qty shots? */
        virtual bool ammo_sufficient( int qty = 1 ) const {
            return loc ? loc->ammo_sufficient( qty ) : false;
        }

        /** Specific ammo data or returns nullptr if no ammo available */
        virtual const itype *ammo_data() const {
            return loc ? loc->ammo_data() : nullptr;
        }

        /** Specific ammo type or returns "null" if no ammo available */
        virtual itype_id ammo_current() const {
            return loc ? loc->ammo_current() : "null";
        }

        /** The @ref ammunition_type or "NULL" if none applicable */
        virtual ammotype ammo_type() const {
            return loc ? loc->ammo_type() : ammotype( "NULL" );
        }

        /** Ammo effects inclusive of any resulting from loaded ammo */
        virtual std::set<std::string> ammo_effects() const {
            return loc ? loc->ammo_effects() : std::set<std::string>();
        }

        /** Maximum range considering current ammo (if any) */
        virtual int range() const {
            return loc ? loc->gun_range() : 0;
        }

    protected:
        item_location loc;
};

#endif
